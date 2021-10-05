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
#define TRACE_COMP Mqtt
#define ACTION_NAMES
#define MQTT_RC_STRINGS
#include "mqtt.h"
#include <security.h>
#include <admin.h>
#include "imacontent.h"
#include <assert.h>
#include <pthread.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>
#include <fendian.h>
#include <throttle.h>
#include <protoex.h>
#include <selector.h>

/*
 * The MQTT protocol handles two methods of framing MQTT:
 * - A native tcp binary frame
 * - A WebSockets frame
 *
 * The processing done is the same regardless of the frame
 *
 * There are three version of the MQTT spec supported by this code specified in the version byte of the CONNECT.
 * 3 = v3.1   - The Paho level of the specification
 * 4 = v3.1.1 - The OASIS standardization of the specification
 * 5 = v5     - The second OASIS version
 *
 * There are only minor differences between versions 3.1 and 3.1.1, but v3.1.1 is a lot more strict about requirements
 * to check reserved bits.  It also removes a couple of limits imposed by v3.1.  The check for v3.1.1 is
 * normally version > 3.  The version is kept in both mmsg->version and pobj->mqtt_version as not all methods
 * have both.  When both exist they should be the same.
 *
 * Version 5 adds significant new function.
 *
 * The design of ISM allows work to be done as scheduled jobs or inline.
 * The result is that many calls to other components including the engine
 * do not return a result but call a callback function when they are
 * complete, which might not be in the same thread.
 *
 * The MQTT bridge protocol extension is supported for versions 3 and 4.
 * This is specified by setting the high order bit of the version.
 * When this is specified, NOLOCAL is asserted on subscriptions, and the retain
 * on publish is used as the retain bit on outgoing publish.
 * As this function is now available in MQTTv5 we do not support the bridge
 * protocol for that version.
 *
 * The MQTT proxy protocol is also supported.  This is a private extension to MQTT to allow
 * additional information to be passed between the proxy and the server.  This is not exposed in any
 * way to a client.  This is selected as using the protocol string MQTTpx and can be any of the
 * supported versions.  Using the proxy protocol allows a domain and max connection count
 * to be passed to the server, and allows additional return codes to be returned on CONNACK.  It also
 * allows a an PINGREQ from the server to the proxy, and a DISCONNECT from the server to the proxy
 * which also contains a return code byte matching CONNACK.
 */
#define PROT_MQTT_BIN    1  /* Binary with native framing         */
#define PROT_MQTT_WSBIN  2  /* Binary with WebSockets framing     */

/*
 * An action header used for subscribe and unsubscribe
 */
typedef struct {
    ism_transport_t * transport;
    mqtt_prodcons_t * consumer;
    ismEngine_ConsumerHandle_t consumerh;
    int action;          /* Action for reply                           */
    int which;           /* How far along are we                       */
    int count;           /* Count of subscriptions                     */
    int msgid;           /* ID of the subscription                     */
    char * qos;          /* Input QoS, one byte for each subscription  */
    char * ack;          /* Output QoS, one byte for each subscription */
    char qosbuf[8];
    char ackbuf[8];
    char locale[8];
    uint8_t sub_found;
    uint8_t flags;
    uint8_t resv1[2];
    int32_t rc;
    int32_t xrc;         /* Local return code */
    int32_t subid;
    int32_t subopt;
    const char * subname;  /* Only valid during listSubscriptions */
    char * freebuf;      /* Free if non-zero */
    char * * topic;      /* Free when done   */
    char * * filter;     /* Free when done   */
} subjob_t;

/*
 * Information for tenants.
 * This is used in the proxy protocol only.
 */
typedef struct ima_tenantInfo_t {
    volatile uint32_t       conCount;
    volatile uint32_t       maxConAllowed;
    char *          name;
    uint32_t        nameLen;
    uint32_t        rsrv;
} ima_TenantInfo_t;

/*
 * Map the 128 bit delivery handle
 */
typedef union {
    ismEngine_DeliveryHandle_t Full;
    struct {
        void * Handle1;
        void * Handle2;
    } Parts;
} ieDeliveryHandle_t;


#undef TRACE_DOMAIN
#define TRACE_DOMAIN transport->trclevel

/*
 * Global static variables
 */
static mqttProtoObj_t * clientsListHead = NULL;
static mqttProtoObj_t * clientsListTail = NULL;
static pthread_spinlock_t clientsListLock;
static pthread_t timerCallbackThread = 0;
static uint64_t clientIDCounter = 0;             /* Used for generated client IDs */
static int      mqttMaxSubs = 500;               /* Maximum subscriptions in a connection, defaults to 500 */
static int mqttMaxKeepAlive = 0;
static int mqtt_unit_test = 0;                   /* Used to indicate we are in unit test    */
static int g_AllowMQTTpxProtocol = 0;            /* Allow a proxy protocol to connect       */
static int g_AllowMQTTv5 = 0;                    /* Allow experimental MQTTv5.0 to connect  */
static int g_AllowNewShared = 0;                 /* Allow $share style shared subscriptions */
static mqtt_prop_ctx_t * g_ctx5;                  /* MQTTv5 Identifier/Value context         */
static ismHashMap  * tenantsMap = NULL;          /* Tenant map                              */
ismEngine_SessionHandle_t g_monitor_session = NULL;   /* Session for monitor publish      */
static int g_MaxTopicAliasIn = 8;
static int g_MaxTopicAliasOut = 8;
static int g_verifyReasonCode = 0;
static int g_verifyPayload = 1;

#define CLIENT_STATE_IN_PROGRESS (void *)0xdead3579a640beef
#define USE_SPIN_LOCK 0
#define USE_MUTEX 1
static int g_msgIdlockType = USE_MUTEX;

static inline void msgIdLock(mqttProtoObj_t *pobj) {
    if(LIKELY(g_msgIdlockType == USE_MUTEX))
    	pthread_mutex_lock(&pobj->msgids_mutex);
    else
    	pthread_spin_lock(&pobj->msgids_spinlock);
}

static inline void msgIdUnlock(mqttProtoObj_t *pobj) {
	if(LIKELY(g_msgIdlockType == USE_MUTEX))
	    pthread_mutex_unlock(&pobj->msgids_mutex);
	else
		pthread_spin_unlock(&pobj->msgids_spinlock);
}

/*
 * Max number of consecutive messages sent to same topic without
 * creating a message producer for more efficient publishing.
 */
#define MAX_NONPROD_COUNT   2
int mqttMaxNonprodCount = MAX_NONPROD_COUNT;

/*
 * Internal session states
 */
static ismEngine_ClientStateHandle_t client_Shared = NULL;
static ismEngine_ClientStateHandle_t client_SharedND = NULL;
static ismEngine_ClientStateHandle_t client_SharedM = NULL;

/* If in debug, always set DEBUG_INPROGRESS */
#ifndef DEBUG_INPROGRESS
//#ifdef DEBUG
#define DEBUG_INPROGRESS 1
//#endif
#endif

/*
 * Forward references
 */
static void parseConnectPayload(mqttMsg_t * mmsg, ism_transport_t * transport);
static const char * mqttCommand(int ix);
static int findConsumer(ism_transport_t * transport, const char * topic);
static int checkString(mqttMsg_t * mmsg, const char * s, int len, int topicchk);
static int chkmsgID(mqttMsg_t * mmsg);
static int mqttTimerDisconnect(ism_timer_t key, ism_time_t timestamp, void * userdata);
extern void ism_mqtt_error(mqtt_act_t * act);
extern void ism_mqtt_setWill(ism_transport_t * transport, mqttMsg_t * mmsg, mqtt_act_t * act);
void ism_mqtt_doPubRec(ism_transport_t * transport, uint16_t msgid, int mqttrc);
void ism_mqtt_doPubComp(ism_transport_t * transport, uint16_t msgid, int mqttrc);
static void parseGETX(ism_transport_t * transport, uint8_t * bp, int len, mqttMsg_t * mmsg);
static int  doGETX(ism_transport_t * transport, mqttMsg_t * mmsg, uint32_t ackID, int subID, const char * topic,
        uint32_t timeout, uint32_t subopt, const char * filter);
void ism_mqtt_replyPubRec(int32_t rc, void * handle, void * vaction);
void ism_mqtt_replyRemoveUnreleasedDeliveryId(int32_t rc, void * handle, void * vaction);
void ism_mqtt_replyCreateSubscription(int32_t rc, void * handle, void * vaction);
void ism_mqtt_replyReSubscribe(int32_t rc, void * handle, void * vaction);
static void mqttDestroyProducer(mqtt_pubobj_t * publisher);
void ism_mqtt_reSubscribe(ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName, const char *pTopicString,
        void * properties, size_t propertiesLength, const ismEngine_SubscriptionAttributes_t *pSubAttributes,
        uint32_t consumerCount, void * vaction);
static void ism_mqtt_replyPublishSimple(mqtt_act_t * act, int times);
static void ism_mqtt_deleteSubscription(int32_t rc, void * handle, void * vaction);
static int previouslyLogged(mqttProtoObj_t * pobj, int msgcode);
static int mqttDumpPobj(ism_transport_t * transport, char * buf, int len);
static void mqttReplyAuth(int authrc, void * callbackParam);
static int mqttReplyAuthTT(ism_transport_t * transport, void * callbackParam, uint64_t async) ;
static void ism_mqtt_getIncompleteMsgIds(uint32_t deliveryId, ismEngine_UnreleasedHandle_t hUnrel, void * pContext);
extern int ism_protocol_auth(char * username, int isoauth, int isltpa);
void ism_transport_restartStream(ism_transport_t * transport, ism_transport_AsyncJob_t job, void * param1, int param2);
static int doSendACL(ism_transport_t * transport, mqttMsg_t * mmsg);
int ism_iot_jsonMessage(concat_alloc_t * buf, ism_transport_t * transport, int which, int rc, const char * reason);
void ism_iot_connectMsg(ism_transport_t * transport);
void ism_iot_failedMsg(ism_transport_t * transport, int rc, const char * reason);
void ism_iot_disconnectMsg(ism_transport_t * transport, int rc, const char * reason);
void ism_mqtt_replyPublishNAK(ism_transport_t * transport, mqttMsg_t * mmsg);
int ism_iot_reconcile(void);
static int mpropCheck(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value);
XAPI int ism_transport_closeWS(ism_transport_t * transport, int rc);
void  ism_fwd_replaceString(const char * * oldval, const char * val);
static int mapToMqttRc(int rc, int version, int cp);
static int mapToIsmRc(int mqttrc, int version);
extern int ism_protocol_selectMessage(
        ismMessageHeader_t *       hdr,
        uint8_t                    areas,
        ismMessageAreaType_t       areatype[areas],
        size_t                     areasize[areas],
        void *                     areaptr[areas],
        const char *               topic,
        void *                     rule,
        size_t                     rulelen);
HOT bool ism_mqtt_replyMessage(ismEngine_ConsumerHandle_t consumerh,
        ismEngine_DeliveryHandle_t deliveryh, ismEngine_MessageHandle_t msgh,
        uint32_t seqnum, ismMessageState_t state, uint32_t options,
        ismMessageHeader_t * hdr, uint8_t areas,
        ismMessageAreaType_t areatype[areas], size_t areasize[areas],
        void * areaptr[areas], void * vaction);
static int packetLength(int buflen) ;
const char * ism_common_getErrorRepl(int which);

/*
 * Settings for checkString
 */
#define CHK_NOT_TOPIC 0   /* No topic checking */
#define CHK_TOPIC     1
#define CHK_PUBTOPIC  2   /* Do topic checking for publish   */
#define CHK_SUBTOPIC  3   /* Do topic checking for subscribe */


/*
 * MQTT properties
 * This must match the table in pxmqtt.c
 */
static mqtt_prop_field_t mqtt_propFields [] = {
    { MPI_PayloadFormat,     MPT_Int1,     5, CPOI_PUBLISH | CPOI_WILLPROP,       "PayloadFormat"    },
    { MPI_MsgExpire,         MPT_Int4,     5, CPOI_PUBLISH | CPOI_WILLPROP,       "MessageExpire"    },
    { MPI_ContentType,       MPT_String,   5, CPOI_PUBLISH | CPOI_WILLPROP,       "ContentType"      },
    { MPI_ReplyTopic,        MPT_String,   5, CPOI_PUBLISH | CPOI_WILLPROP,       "ResponseTopic"    },
    { MPI_Correlation,       MPT_Bytes,    5, CPOI_PUBLISH | CPOI_WILLPROP,       "CorrelationData"  },
    { MPI_SubID,             MPT_VarInt,   5, CPOI_S_PUBLISH | CPOI_SUBSCRIBE | CPOI_ALT_MULTI,    "SubscriptionID"   },
    { MPI_SessionExpire,     MPT_Int4,     5, CPOI_CONNECT | CPOI_CONNACK | CPOI_C_DISCONNECT,     "SessionExpire"    },
    { MPI_ClientID,          MPT_String,   5, CPOI_CONNACK,                       "AssignedClientID" },
    { MPI_KeepAlive,         MPT_Int2,     5, CPOI_CONNACK,                       "ServerKeepAlive"  },
    { MPI_AuthMethod,        MPT_String,   5, CPOI_AUTH_CONN_ACK,                 "AuthenticationMethod" },
    { MPI_AuthData,          MPT_Bytes,    5, CPOI_AUTH_CONN_ACK,                 "AuthenticationData"   },
    { MPI_RequestReason,     MPT_Int1,     5, CPOI_CONNECT,                       "RequestReason"    },
    { MPI_WillDelay,         MPT_Int4,     5, CPOI_WILLPROP,                      "WillDelay"        },
    { MPI_RequestReplyInfo,  MPT_Int1,     5, CPOI_CONNECT,                       "RequestReplyInfo" },
    { MPI_ReplyInfo,         MPT_String,   5, CPOI_CONNACK,                       "ReplyInfo"        },
    { MPI_ServerReference,   MPT_String,   5, CPOI_CONNACK | CPOI_S_DISCONNECT,   "ServerReference" },
    { MPI_Reason,            MPT_String,   5, CPOI_ACK,                           "Reason"           },
    { MPI_MaxReceive,        MPT_Int2,     5, CPOI_CONNECT_CONNACK,               "MaxInflight"      },
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
        allow = CPOI_PUBACK;                           break;
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
 * Add a connection to the list of clients
 */
static void addToClientsList(mqttProtoObj_t * pobj, int lock, ism_transport_t * transport) {
    if (lock)
        pthread_spin_lock(&clientsListLock);
    if (pobj->keepAlive < 0) {
        if ( pobj->next != NULL || pobj->prev != NULL )
        {
            assert(true);
            bool found = false;
            mqttProtoObj_t * curr = clientsListHead;
            while (curr) {
                if (curr == pobj) {
                    found = true;
                    break;
                }
                curr = curr->next;
            }

            if ( found ) {
                TRACE(3, "Attempting to add mqttProtoObj_t %p to client list but already on list!", pobj);
                if (lock)
                        pthread_spin_unlock(&clientsListLock);
                return;
            } else {
                TRACE(3, "Attempting to add mqttProtoObj_t %p with next and prev set but not on list!", pobj);
            }
        }

        pobj->next = NULL;
        if (clientsListTail) {
            pobj->prev = clientsListTail;
            clientsListTail->next = pobj;
        } else {
            pobj->prev = NULL;
            clientsListHead = pobj;
        }
        clientsListTail = pobj;
        pobj->keepAlive = -pobj->keepAlive;
    }
    if (lock)
        pthread_spin_unlock(&clientsListLock);
}


/*
 * Remove a connection from the list of clients
 */
static void removeFromClientsList(mqttProtoObj_t * pobj, int lock) {
    /* timerCallbackThread is only set in CUnit mode */
    lock = lock && (timerCallbackThread != pthread_self());
    if (lock)
        pthread_spin_lock(&clientsListLock);
    if (pobj && pobj->keepAlive > 0) {
        if (pobj->prev) {
            pobj->prev->next = pobj->next;
        } else {
            assert(pobj == clientsListHead);
            clientsListHead = pobj->next;
        }
        if (pobj->next) {
            pobj->next->prev = pobj->prev;
        } else {
            assert(pobj == clientsListTail);
            clientsListTail = pobj->prev;
        }
        pobj->next = pobj->prev = NULL;
    }
    pobj->keepAlive = 0;
    if (lock)
        pthread_spin_unlock(&clientsListLock);
}


/*
 * Time out a connection
 */
static int connectionTimeout(ism_transport_t * transport, void * userdata, int flags) {
    transport->close(transport, ISMRC_ConnectTimedOut, 0, "The connection timed out");
    return 0;
}


/*
 * Check the last access for a connection
 */
static int checkLastAccessTime(mqttProtoObj_t * pobj, uint64_t currTime) {
    uint64_t lastAccessTime = pobj->lastAccessTime;
    ism_transport_t * transport = pobj->transport;
    if (lastAccessTime && !pobj->closed && pobj->keepAlive > 0) {
        /* If last access + (1.5 * keep alive interval) < current time */
        if (UNLIKELY((lastAccessTime + pobj->keepAlive + (pobj->keepAlive >> 1)) < currTime)) {
            TRACE(3, "MQTT connection has timed out: connect=%u client=%s time=%llu keepalive=%lld\n",
                    transport->index, transport->clientID, (ULL)(currTime-lastAccessTime), (ULL)pobj->keepAlive);
            if (transport->addwork) {
                transport->addwork(transport, connectionTimeout, NULL);
            } else { /* For CUnit */
                timerCallbackThread = pthread_self();
                if (pobj) {
                    connectionTimeout(transport, NULL, 0);
                }
                timerCallbackThread = 0;
            }
            return 0;
        }
    }
    return 1;
}


/*
 * Send a PINGRESP and see if the send works
 */
static int mqttPingClient(ism_transport_t * transport) {
    char msg[32];
    /* MQTT server is not allowed to send PINGREQ, so we are going to send PINGRESP */
    int rc = transport->send(transport, msg+16, 0, MT_PINGRESP << 4, SFLAG_FRAMESPACE);
    if (rc && (rc != SRETURN_SUSPEND)) {
        TRACEL(7, transport->trclevel, "Failed to ping client: connect=%u client=%s\n", transport->index, transport->name);
        transport->close(transport, ISMRC_PingFailed, 0, "Failed to ping client.");
        return 0;
    }
    TRACEL(8, transport->trclevel, "PINGREQ was sent to client: connect=%u client=%s ip=%s port=%u rc=%d\n", transport->index, transport->name,
            transport->client_addr, transport->clientport, rc);
    return 1;
}


/*
 * Check that the connection is alive
 */
static int mqttCheckLiveness(ism_transport_t * transport) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    /* If keepAlive != 0 we are checking the liveness already */
    if (!pobj->closed) {
        mqttPingClient(transport);
    }
    return 0;
}


/*
 * Show the name of an action
 */
xUNUSED static const char * getActionName(int action) {
    if (action < 0 || action > MAX_ACTION_VALUE) {
        return "Unknown";
    }
    return ism_action_names[action];
}


/*
 * Allocate a producer or consumer ID
 */
static int findProdcons(ism_transport_t * transport) {
    int i;
    int newSize;
    mqtt_prodcons_t * * newArray;
    mqtt_prodcons_t * consumer;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    pthread_spin_lock(&(pobj->lock));

    for (i = 1; i < pobj->prodcons_alloc; i++) {
        mqtt_prodcons_t * prodcon = (mqtt_prodcons_t *) pobj->prodcons[i];
        if (prodcon == NULL) {
            prodcon = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,70),1, sizeof(mqtt_prodcons_t));
            if (prodcon == NULL) {
                i = 0;
                ism_common_setError(ISMRC_AllocateError);
                break;
            }
            pobj->prodcons[i] = prodcon;
            prodcon->closed = 1;
            break;
        } else if (prodcon->closed && !prodcon->topic) {
            break;
        }
    }

    /* If error (i = 0) or empty entry found, return. */
    if (i < pobj->prodcons_alloc) {
        pthread_spin_unlock(&(pobj->lock));
        return i;
    }

    /*
     * Extend array of consumers, if existing array is insufficient.
     */
    newSize = pobj->prodcons_alloc * 2;
    newArray = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,71),newSize, sizeof(mqtt_prodcons_t *));
    if (newArray == NULL) {
        pthread_spin_unlock(&(pobj->lock));
        ism_common_setError(ISMRC_AllocateError);
        return 0;
    }

    consumer = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,72),1, sizeof(mqtt_prodcons_t));
    if (consumer == NULL) {
        pthread_spin_unlock(&(pobj->lock));
        ism_common_free(ism_memory_protocol_misc,newArray);
        ism_common_setError(ISMRC_AllocateError);
        return 0;
    }

    memcpy(newArray, pobj->prodcons, pobj->prodcons_alloc * sizeof(mqtt_prodcons_t *));

    ism_common_free(ism_memory_protocol_misc,pobj->prodcons);
    pobj->prodcons = newArray;
    pobj->prodcons_alloc = newSize;
    pobj->prodcons[i] = consumer;
    pthread_spin_unlock(&(pobj->lock));

    return i;
}


/*
 * Get a producer or consumer handle from a session
 * In MQTT this is only a consumer.
 */
xUNUSED static mqtt_prodcons_t * getProdcons(ism_transport_t * transport, int pc_id) {
    return ((mqttProtoObj_t *) transport->pobj)->prodcons[pc_id];
}

/*
 * Log a protocol error
 * For now we do not log as the connection will always be closed at this point.
 * This gives a common spt for a debug stop.
 */
static void logProtocolError(mqttMsg_t * mmsg) {
   xUNUSED int code = 2113;
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
            freebuf = newbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,75),transport->pobj->morelen + buflen);
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
                        ism_common_free(ism_memory_protocol_misc,freebuf);
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
                    ism_common_free(ism_memory_protocol_misc,freebuf);
                return -1;
            }
        }

        if (buflen < len) {
            buflen += count;
            break;
        }

        /* Call mqtt binary receive */
        ism_mqtt_receive(transport, buf + count, len, kind);
        buf += count + len;
        buflen -= len;
        count = 0;
    }
    if (buflen > 0) {
    savebuf:
        if (buflen > transport->pobj->morealloc) {
            transport->pobj->morebuf = ism_common_realloc(ISM_MEM_PROBE(ism_memory_protocol_misc,78),transport->pobj->morebuf, buflen);
            transport->pobj->morealloc = buflen;
        }
        memcpy(transport->pobj->morebuf, buf, buflen);
        transport->pobj->morelen = buflen;
    }
    if (freebuf)
        ism_common_free(ism_memory_protocol_misc,freebuf);
    return ret;
}


/*
 * Put one character to a concat buf
 */
static void bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}


/*
 * Process saved data.
 * This is used for data which arrives after the CONNECT and before the CONNACK is sent.
 * The size of this data is limited to prevent DoS problems.
 */
static void processSavedData(ism_transport_t * transport) {
    mqttProtoObj_t * pobj = transport->pobj;
    mqttSavedData_t * sd = pobj->savedDataHead;
    int rc = 0;
    int counter = 0;
    if (!sd)
        return;
    while (sd) {
        mqttSavedData_t * next = sd->next;
        if (!rc)
            rc = ism_mqtt_receive(transport, sd->buf, sd->buflen, sd->kind);
        ism_common_free(ism_memory_protocol_misc,sd);
        sd = next;
        counter++;
    }
    pobj->savedDataHead = NULL;
    pobj->savedDataTail = NULL;
    pobj->savedSize = 0;
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in processSavedData: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-counter);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, counter) < 0)) {   /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
    }
}

/*
 * Implement common code for parsing an ACK
 * This deals with the return code and properties
 *
 * @param bp     The command buffer (following the remaining length)
 * @param buflen The remaining length
 * @param mmsg   The MQTT message object
 * @param cpoi   Which set of properties
 * @return The buffer length which is 0 for good cases
 */
static int checkACK(uint8_t * bp, int buflen,  mqttMsg_t * mmsg, int cpoi) {
    /*
     * Handle the most common case of having only a packet ID
     */
    if (LIKELY(buflen == 2)) {
        mmsg->msgid = (uint16_t)BIGINT16(bp);
        mmsg->mqtt_rc = 0;
        mmsg->rc = 0;
        return 0;
    }
    if (mmsg->version >= 5) {
        if (buflen >= 3) {
            mmsg->msgid = (uint16_t)BIGINT16(bp);
            mmsg->mqtt_rc = bp[2];
            mmsg->rc = mapToIsmRc(mmsg->mqtt_rc, mmsg->version);
            buflen -= 3;
            bp += 3;
            if (buflen) {
                int proplen;
                if (*bp >=  128) {
                    proplen = *bp++;
                    buflen--;
                } else {
                    int vilen;
                    proplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                    bp += vilen;
                    buflen -= vilen;
                }
                if (proplen) {
                    if (proplen < 0 || proplen > buflen) {
                        mmsg->rc = ISMRC_BadLength;
                        ism_common_setErrorData(mmsg->rc, "%s", "MQTTPropLen");
                    } else {
                        mmsg->rc = ism_common_checkMqttPropFields((char *)bp, proplen,
                            g_ctx5, cpoi, mpropCheck, mmsg);
                        if (mmsg->rc) {
                            logProtocolError(mmsg);
                        }
                    }
                    bp += proplen;
                    buflen -= proplen;
                }
            }
            return buflen;
        }
    }
    mmsg->rc = ISMRC_BadLength;
    ism_common_setError(mmsg->rc);
    return buflen-2;
}

/*
 * Callback from scan of publish extension
 */
static void publishExt(int item, const char * s, int64_t value, void * userdata) {
    mqttMsg_t * mmsg = (mqttMsg_t *)userdata;
    switch(item) {
    case EXIV_Locale:
        if (s && value > 0) {
            if (value > 7)
                value = 7;

            memcpy(mmsg->locale, s, value);
            mmsg->locale[value] = 0;
        }
        break;
    case EXIV_ExpireTTL:
        mmsg->isMsgExpire = value != 0;
        mmsg->msgExpire = (uint32_t) value;
        break;
    case EXIV_Property:
        mmsg->has_props = 1;
        break;
    }
}

/*
 * Compute the packet length from a buffer length not including the fixed header
 */
static int packetLength(int buflen) {
    int chklen = buflen + 2;
    if (buflen >= 128) {
        chklen++;
        if (buflen >= 16384) {
            chklen++;
            if (buflen >= 2097152)
                chklen++;
        }
    }
    return chklen;
}

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
HOT int ism_mqtt_receive(ism_transport_t * transport, char * buf, int buflen, int kind) {
    uint8_t * bp = (uint8_t *) buf;
    mqttMsg_t mmsg = { 0 };
    char obuf[64];
    int  rc = 0;
    int  savebuflen = buflen;
    mqtt_act_t act = { 0 };
    mqttProtoObj_t * pobj = transport->pobj;
    int  mproplen;
    int  vilen;

    if (pobj == NULL) {
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }

    /* If protocol is closing the connection, reject the message */
    if (UNLIKELY((pobj->closed && transport->closestate[3]))) {
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }

    if (pobj->pingTimer) {
        /* This is a ping timer */
        ism_common_cancelTimer(pobj->pingTimer);
        pobj->pingTimer = NULL;
    }

    /* If we are in the process of closing, return closed */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Increment inprogress in ism_mqtt_receive: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress+1);
#endif
    if (UNLIKELY(__sync_fetch_and_add(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
        __sync_fetch_and_sub(&pobj->inprogress, 1);
        ism_common_setError(ISMRC_Closed);
        return ISMRC_Closed;
    }

    mmsg.command = (uint8_t)((kind >> 4) & 15);
    mmsg.version = pobj->mqtt_version;    /* Version comes from pobj except for CONNECT */
#ifdef DEBUG
    TRACE(8, "---------ism_mqtt_receive action=%s connect=%u(%p) inprogress=%d\n",
        mqttCommand(mmsg.command), transport->index, transport, pobj->inprogress);
#endif

    /*
     * For MQTTv5 it is a protocol error if the packet exceeds our specified max packet size.
     * We make the same check for a CONNECT where we do not yet know the version.
     */
    if (transport->listener->maxMsgSize && (pobj->mqtt_version >= 5 || mmsg.command == MT_CONNECT)) {
        int chklen = packetLength(buflen);
        if (chklen > transport->listener->maxMsgSize) {
            mmsg.rc = ISMRC_MsgTooBig;
            ism_common_setErrorData(mmsg.rc, "%u%u", chklen, transport->listener->maxMsgSize);
        }
    }

    /*
     * Check to make sure we are not connected
     */
    if (UNLIKELY(!mmsg.rc && (pobj->startState != MQTT_CONNECTED) && (mmsg.command != MT_CONNECT))) {
        mmsg.rc = (pobj->startState == MQTT_DISCONNECTED) ? ISMRC_Closed : ISMRC_ConnectFirst; /* Connect must be first */
        if (pobj->startState == MQTT_IN_PROGRESS) {
            if ((pobj->savedSize + buflen) < 64*1024) {
                /* CONNECT is in progress - save the data and return */
                mqttSavedData_t * sd = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,81),sizeof(mqttSavedData_t) + buflen);
                if (!sd) {
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_receive: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
                    __sync_fetch_and_sub(&pobj->inprogress, 1);
                    ism_common_setError(ISMRC_AllocateError);
                    return ISMRC_AllocateError;
                }
                sd->next = NULL;
                sd->kind = kind;
                sd->buflen = buflen;
                sd->buf = (char*) (sd+1);
                memcpy(sd->buf,buf,buflen);
                if (pobj->savedDataTail) {
                    pobj->savedDataTail->next = sd;
                } else {
                    pobj->savedDataHead = sd;
                }
                pobj->savedDataTail = sd;
                pobj->savedSize += buflen;
                return ISMRC_OK;
            }
            mmsg.rc = ISMRC_TooMuchSavedData; /* Too much data was received before connection request completed */
        }
        ism_common_setError(mmsg.rc);
    }

    /*
     * If we have not yet seen a problem, do the command
     */
    if (mmsg.rc == 0) {
        /* Do the receive trace */
        if (SHOULD_TRACE(9) && mmsg.command != MT_CONNECT) {
            int maxsize = ism_common_getTraceMsgData();
            if (maxsize==0 && mmsg.command != MT_PUBLISH)
                maxsize = 1000;
            sprintf(obuf, "MQTT receive %02x %s connect=%u", (uint8_t)kind, mqttCommand(kind), transport->index);
            traceDataFunction(obuf, 0, __FILE__, __LINE__, buf, buflen, maxsize);
        }

        /*
         * Process the commands
         */
        switch (mmsg.command) {

        /*
         * Publish
         */
        case MT_PUBLISH:
            /*
             * Process a PUBLISHX which is only in proxy protocol
             */
            if (pobj->mqtt_proxy && kind == 0x3F) {
                mmsg.command = MTX_PUBLISHX;
                if (buflen > 3) {
                    kind = *bp++;
                    buflen--;
                    mmsg.qos = (uint8_t)((kind >> 1) & 3);
                    mmsg.isMsgid = (uint8_t)((kind>>4) & 1);
                    /* Process extension */
                    if (kind&0x20) {
                        mmsg.ext_len = BIGINT16(bp);
                        bp += 2;
                        buflen -= 2;
                        mmsg.ext = (char *)bp;
                        if (mmsg.ext_len < buflen) {
                            ism_common_scanExtension(mmsg.ext, mmsg.ext_len, publishExt, &mmsg);
                            bp += mmsg.ext_len;
                            buflen -= mmsg.ext_len;
                        } else {
                            mmsg.rc = ISMRC_BadClientData;
                            ism_common_setError(mmsg.rc);
                            break;
                        }
                    }
                } else {
                    mmsg.rc = ISMRC_BadClientData;
                    ism_common_setError(mmsg.rc);
                    break;
                }
            } else {
                /* Normal MQTT PUBLISH */
                mmsg.qos = (uint8_t)((kind >> 1) & 3);
                mmsg.isMsgid = mmsg.qos>0;
            }
            mmsg.retain = (uint8_t)(kind & 1);
            mmsg.dup = (uint8_t)((kind >> 3) & 1);
            mmsg.msgtype = MTYPE_MQTT_Binary;

            if (UNLIKELY(mmsg.qos == 3)) {
                mmsg.rc = ISMRC_InvalidQoS;
                ism_common_setError(mmsg.rc);
            } else if (!pobj->allow_persistent && mmsg.qos > 0) {
                mmsg.rc = ISMRC_InvalidQoS;
                ism_common_setError(mmsg.rc);
            } else if (UNLIKELY(mmsg.version > 3 && mmsg.qos == 0 && mmsg.dup)) {
                /* In V3.1.1 it is not valid to specify QoS=0 and dup */
                mmsg.rc = ISMRC_BadClientData;
                ism_common_setError(mmsg.rc);
            } else {
                mproplen = 0;
                if (LIKELY(buflen > 1)) {
                    mmsg.topic_len = BIGINT16(bp);
                    buflen -= 2;
                    bp += 2;
                    mmsg.topic = (char *) bp;
                    bp += mmsg.topic_len;
                    buflen -= mmsg.topic_len;
                    if (UNLIKELY(buflen < 0)) {
                        mmsg.rc = ISMRC_BadLength;
                        ism_common_setErrorData(mmsg.rc, "%s", "TopicLength");
                    } else {
                        if (mmsg.isMsgid > 0) {
                            if (buflen > 1)
                                mmsg.msgid = (uint16_t)BIGINT16(bp);
                            buflen -= 2;
                            bp += 2;
                        }
                        if (buflen > 0 && pobj->mqtt_version >= 5) {
                            if (*bp != 0) {
                                mproplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                                bp += vilen;
                                buflen -= vilen;
                                if (mproplen < 0 || mproplen > buflen) {
                                    ism_common_setErrorData(ISMRC_BadLength, "%s%s", "PUBLISH", "PropLen");
                                    mmsg.rc = ISMRC_BadLength;
                                    break;
                                } else {
                                    /* Check the extension.  If it is good we will just copy it as we got it */
                                    mmsg.v5props = (const char *)bp;
                                    mmsg.v5prop_len = mproplen;
                                    int xrc = ism_common_checkMqttPropFields((char *)bp, mproplen,
                                        g_ctx5, CPOI_C_PUBLISH, mpropCheck, &mmsg);
                                    if (xrc) {
                                        mmsg.rc = xrc;
                                        logProtocolError(&mmsg);
                                        break;   /* setError must already be done */
                                    }
                                }
                                bp += mproplen;
                                buflen -= mproplen;
                            } else {
                                bp++;
                                buflen--;
                                mmsg.v5prop_len = 0;
                            }
                        }
                        mmsg.payload = (char *) bp;
                        mmsg.payload_len = buflen;
                        if (buflen==0 && mmsg.retain)
                            mmsg.msgtype = MTYPE_NullRetained;
                        ism_mqtt_doPublish(transport, &mmsg);
                    }
                } else {
                    mmsg.rc = ISMRC_BadLength;
                    ism_common_setError(mmsg.rc);
                }
            }
            break;

        /*
         * Publish acknowledge (QoS=1)
         */
        case MT_PUBACK: /* ACK publish from QoS=1 */
            if (pobj->mqtt_version > 3 && (kind&0x0f)) {
                /* In V3.1.1 check the reserved bits */
                mmsg.rc = ISMRC_BadClientData;
                ism_common_setError(mmsg.rc);
            } else {
                /* Handle rc and properties */
                buflen = checkACK(bp, buflen, &mmsg, CPOI_PUBACK);
                if (buflen == 0 && !chkmsgID(&mmsg)) {
                    ism_mqtt_doPubAck(transport, mmsg.msgid, mmsg.mqtt_rc);
                }
            }
            break;

        /*
         * Publish received (first response for QoS=2)
         */
        case MT_PUBREC: /* Message received for QoS=2 */
            if ((kind & 0x0f) == 0x0f) {
                mmsg.command = MTX_GETX;
                parseGETX(transport, bp, buflen, &mmsg);
            } else {
                if (pobj->mqtt_version > 3 && (kind&0x0f)) {
                    /* In V3.1.1 check the reserved bits */
                    mmsg.rc = ISMRC_BadClientData;
                    ism_common_setError(mmsg.rc);
                } else {
                    /* handle rc and properties */
                    buflen = checkACK(bp, buflen, &mmsg, CPOI_PUBACK);
                    if (buflen == 0 && !chkmsgID(&mmsg)) {
                        ism_mqtt_doPubRec(transport, mmsg.msgid, mmsg.mqtt_rc);
                    }
                }
            }
            break;

        /*
         * Publish release (second resonse for QoS=2)
         */
        case MT_PUBREL: /* Release publish for QoS=2 */
            if (pobj->mqtt_version > 3 && (kind&0x0f) != 2) {
                /* In V3.1.1 check the reserved bits */
                mmsg.rc = ISMRC_BadClientData;
                ism_common_setError(mmsg.rc);
            } else {
                buflen = checkACK(bp, buflen, &mmsg, CPOI_PUBCOMP);
                if (buflen == 0 && !chkmsgID(&mmsg)) {
                    ismEngine_UnreleasedHandle_t phUnrel = NULL;
                    ism_msgid_info_t * pMsgInfo = NULL;
                    act.transport = transport;
                    act.msgid = mmsg.msgid;
                    act.isMsgid = mmsg.isMsgid;
                    rc = 0;

                    msgIdLock(pobj);
                    pMsgInfo = ism_msgid_getMsgIdInfo(pobj->incompmsgids, (uint16_t)mmsg.msgid);
                    if (pMsgInfo) {
                        if (mmsg.mqtt_rc >= 128 && pobj->mqtt_version >= 5) {
                            phUnrel = (void*) ((uintptr_t)pMsgInfo->handle);
                            ism_msgid_delMsgIdInfo(pobj->incompmsgids, mmsg.msgid, NULL);
                            msgIdUnlock(pobj);
                            if (phUnrel && pobj->session_handle) {
                                xUNUSED int zrc = ism_engine_removeUnreleasedDeliveryId(pobj->session_handle, NULL, phUnrel,
                                        NULL, 0, NULL);
                            }
                            TRACEL(8, transport->trclevel, "PUBRL delete packetID connect=%u client=%s msgid=%u \n",
                                      transport->index, transport->name, (uint32_t)mmsg.msgid);
                            phUnrel = NULL;
                            /* Send no response */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_receive: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
                            if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
                                TRACE(9, "replyClose from connect=%u\n", transport->index);
                                ism_mqtt_replyCloseClient(transport);
                            }
                            break;
                        } else {
                            switch(pMsgInfo->state) {
                            case ISM_MQTT_PUBREC:
                                pMsgInfo->state = ISM_MQTT_PUBREL;
                                pMsgInfo->pending = 1;
                                phUnrel = (void*) ((uintptr_t)pMsgInfo->handle);
                                pMsgInfo->handle = 0;
                                msgIdUnlock(pobj);
                                break;
                            case ISM_MQTT_PUBREL:
                                pMsgInfo->pending++;
                                msgIdUnlock(pobj);
                                rc = ISMRC_AsyncCompletion;
                                TRACE(5, "DUPLICATE PUBREL received for: connect=%u client=%s msgid=%u pending=%d\n",
                                    transport->index, transport->name, mmsg.msgid, pMsgInfo->pending);
                                break;
                            case ISM_MQTT_PUBLISH:
                            default:
                            	msgIdUnlock(pobj);
                                TRACE(5, "PUBREL received for: connect=%u client=%s msgid=%u state=%u\n",
                                    transport->index, transport->name, mmsg.msgid, pMsgInfo->state);
                                rc = ISMRC_BadClientData;
                                ism_common_setError(rc);
                                break;
                            }
                        }
                    } else {
                    	msgIdUnlock(pobj);
                        TRACE(7, "PUBREL of unknown ID: connect=%u client=%s msgid=%u\n",
                                transport->index, transport->name, mmsg.msgid);
                        ism_mqtt_replyRemoveUnreleasedDeliveryId(MQTTRC_PacketIDNotFound, NULL, &act);
                        break;
                    }

                    if (rc == ISMRC_BadClientData) {
                        mmsg.rc = rc;
                        break;
                    }
                    if (phUnrel) {
                        TRACE(8, "PUBREL received for: connect=%u client=%s msgid=%u phUnrel=%p inProgress=%d\n",
                            transport->index, transport->name, act.msgid, phUnrel, pobj->inprogress);
                        if (pobj->session_handle) {
                            rc = ism_engine_removeUnreleasedDeliveryId(pobj->session_handle, NULL, phUnrel, &act,
                                    sizeof(act), ism_mqtt_replyRemoveUnreleasedDeliveryId);
                        } else {
                            TRACE(5, "PUBREL session not found: connect=%u client=%s id=%d\n",
                                transport->index, transport->name, act.msgid);
                        }
                    }
                    if (rc != ISMRC_AsyncCompletion) {
                        ism_mqtt_replyRemoveUnreleasedDeliveryId(rc, NULL, &act);
                    }
                }
            }
            break;

        /*
         * Publish complete (third response for publish)
         */
        case MT_PUBCOMP: /* Message is consumed for QoS=2 */
            if (pobj->mqtt_version > 3 && (kind&0x0f)) {
                /* In V3.1.1 check the reserved bits */
                mmsg.rc = ISMRC_BadClientData;
                ism_common_setError(mmsg.rc);
                break;
            } else {
                buflen = checkACK(bp, buflen, &mmsg, CPOI_PUBCOMP);
                if (buflen == 0 && !chkmsgID(&mmsg)) {
                    ism_mqtt_doPubComp(transport, mmsg.msgid, mmsg.mqtt_rc);
                }
            }
            break;

        /*
         * Subscribe
         */
        case MT_SUBSCRIBE:
            if (pobj->mqtt_client) {
                mmsg.rc = ISMRC_BadClientData;
                ism_common_setError(mmsg.rc);
                break;
            }
            if (pobj->mqtt_proxy && (kind&0xF)==0xF) {
                mmsg.command = MTX_SUBSCRIBEX;
            } else {
                if (pobj->mqtt_version > 3) {
                    /* In V3.1.1 check the reserved bits */
                    if ((kind&0x0f) != 2) {
                        mmsg.rc = ISMRC_BadClientData;
                        ism_common_setError(mmsg.rc);
                        break;
                    }
                } else {
                    /* In V3.1 retain legacy check that for QoS=1 used to show msgid exists */
                    if ((kind & 0xf) != 2) {
                        mmsg.rc = ISMRC_InvalidValue;
                        break;
                    }
                }
            }
            if (buflen > 1) {
                mmsg.msgid = (uint16_t) BIGINT16(bp);
                buflen -= 2;
                bp += 2;
                if (mmsg.version >= 5) {
                    if (*bp != 0) {
                        mproplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                        bp += vilen;
                        buflen -= vilen;
                        if (mproplen < 0 || mproplen > buflen) {
                            ism_common_setErrorData(ISMRC_BadLength, "%s%s", "SUBSCRIBE", "MQTTPropLen");
                            return ISMRC_BadLength;
                        } else {
                            /* Check the properties.  If it is good we will just copy it as we got it */
                            int xrc = ism_common_checkMqttPropFields((char *)bp, mproplen,
                                g_ctx5, CPOI_SUBSCRIBE, mpropCheck, &mmsg);
                            if (xrc) {
                                mmsg.rc = xrc;   /* setError must already be done */
                                logProtocolError(&mmsg);
                                break;
                            }
                        }
                        bp += mproplen;
                        buflen -= mproplen;
                    } else {
                        bp++;
                        buflen--;
                    }
                }
                mmsg.payload = (char *)bp;
                mmsg.payload_len = buflen;
                ism_mqtt_doSubscribe(transport, &mmsg);
                if (mmsg.rc) {
                    int msgcode;
                    msgcode = 2201000 + mmsg.rc;

                    if (!previouslyLogged(pobj, msgcode)) {
                        char xerrbuf[4096];

                        if (ism_common_getLastError() != mmsg.rc)
                            ism_common_setError(mmsg.rc);

                        ism_common_formatLastError(xerrbuf, sizeof xerrbuf);

                        LOG(WARN, Connection, 2201, "%u%-s%-s%-s%-s%s%d",
                                "A message consumer could not be created: "
                                "ConnectionID={0} ClientID={1} Endpoint={2} UserID={3} Protocol={4} Error={5} RC={6}.",
                                transport->index, transport->name, transport->listener->name,
                                transport->userid ? transport->userid : "", transport->protocol,
                                        xerrbuf, mmsg.rc);
                    }
                    break;
                }
            } else {
                if (!mmsg.rc) {
                    mmsg.rc = ISMRC_BadLength;
                    ism_common_setError(mmsg.rc);
                }
            }
            break;

        /*
         * Unsubscribe
         */
        case MT_UNSUBSCRIBE:
            if (pobj->mqtt_client) {
                mmsg.rc = ISMRC_BadClientData;
                ism_common_setError(mmsg.rc);
                break;
            }
            if (pobj->mqtt_version > 3) {
                /* In V3.1.1 check the reserved bits */
                if ((kind&0x0f) != 2) {
                    mmsg.rc = ISMRC_BadClientData;
                    ism_common_setError(mmsg.rc);
                    break;
                }
            } else {
                /* In V3.1 retain legacy check that for QoS=1 used to show msgid exists */
                if ((kind & 0xf) != 2) {
                    mmsg.rc = ISMRC_InvalidValue;
                    break;
                }
            }
            if (buflen > 1) {
                mmsg.msgid = (uint16_t) BIGINT16(bp);
                buflen -= 2;
                bp += 2;
                if (mmsg.version >= 5) {
                    if (*bp != 0) {
                        mproplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                        bp += vilen;
                        buflen -= vilen;
                        if (mproplen < 0 || mproplen > buflen) {
                            ism_common_setErrorData(ISMRC_BadLength, "%s%s", "UNSUBSCRIBE", "MQTTPropLen");
                            return ISMRC_BadLength;
                        } else {
                            /* Check the properties.  If it is good we will just copy it as we got it */
                            int xrc = ism_common_checkMqttPropFields((char *)bp, mproplen,
                                g_ctx5, CPOI_SUBSCRIBE, mpropCheck, &mmsg);
                            if (xrc) {
                                mmsg.rc = xrc;   /* setError must already be done */
                                logProtocolError(&mmsg);
                                break;
                            }
                        }
                        bp += mproplen;
                        buflen -= mproplen;
                    } else {
                        bp++;
                        buflen--;
                    }
                }
                mmsg.payload = (char *)bp;
                mmsg.payload_len = buflen;
                ism_mqtt_doUnsubscribe(transport, &mmsg);
            } else {
                if (!mmsg.rc) {
                    mmsg.rc = ISMRC_BadLength;
                    ism_common_setError(mmsg.rc);
                }
            }
            break;

        /*
         * Ping request
         */
        case MT_PINGREQ:
            if (pobj->mqtt_proxy && (uint8_t)kind == 0xCF) {
            	////0xCF == 11001111 == PINGREQ + right 4 bits on
                mmsg.command = MTX_SENDACL;
                if (buflen>1) {
                    mmsg.ext_len = BIGINT16(bp);
                    bp += 2;
                    buflen -= 2;
                    mmsg.ext = (char *)bp;
                    if (buflen < mmsg.ext_len) {
                        mmsg.rc = ISMRC_BadLength;
                        ism_common_setError(mmsg.rc);
                        break;
                    }
                    bp += mmsg.ext_len;
                    buflen -= mmsg.ext_len;
                    mmsg.payload = (const char *)bp;
                    mmsg.payload_len = buflen;
                    if (!doSendACL(transport, &mmsg)) {    /* If it does not go async */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_receive: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
                        if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) {   /* BEAM suppression: constant condition */
                            TRACE(9, "replyClose from connect=%u\n", transport->index);
                            ism_mqtt_replyCloseClient(transport);
                        }
                    }
                } else {
                    mmsg.rc = ISMRC_BadLength;
                    ism_common_setError(mmsg.rc);
                    break;
                }
            } else if (pobj->mqtt_proxy && (uint8_t)kind == MT_GW_PINGREG) {
            	//if PINGREQ from Proxy for Gateway when PUBLISH failed
            	//0xCE == 11001110 == PINGREQ + right 3 bits on
            	TRACE(7, "MT_GW_PINGREG: connect=%u client=%s version=%u\n",
            	            	transport->index, transport->name, pobj->mqtt_proxy);
            	transport->send(transport, obuf + 16, 0, MT_GW_PINGRES , SFLAG_FRAMESPACE );

#ifdef DEBUG_INPROGRESS
            	TRACE(9, "Decrement inprogress in ism_mqtt_receive: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
                if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) {   /* BEAM suppression: constant condition */
                    TRACE(9, "replyClose from connect=%u\n", transport->index);
                    ism_mqtt_replyCloseClient(transport);
                }

        	}else {
                if (buflen != 0) {
                    mmsg.rc = ISMRC_BadLength;
                    ism_common_setError(mmsg.rc);
                    break;
                }
                if (pobj->mqtt_version > 3 && (kind&0x0f)) {
                    /* In v3.1.1 check reserved bits */
                    mmsg.rc = ISMRC_BadClientData;
                    ism_common_setError(mmsg.rc);
                    break;
                }
                transport->send(transport, obuf + 16, 0, MT_PINGRESP << 4,
                        SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_receive: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
                if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) {   /* BEAM suppression: constant condition */
                    TRACE(9, "replyClose from connect=%u\n", transport->index);
                    ism_mqtt_replyCloseClient(transport);
                }
            }
            break;

        /*
         * Ping response.  We should not get this as a normal server
         */
        case MT_PINGRESP:
            if (buflen != 0) {
                mmsg.rc = ISMRC_BadLength;
                ism_common_setError(mmsg.rc);
            }
            if (pobj->mqtt_version > 3 && (kind&0x0f)) {
                /* In v3.1.1 check reserved bits */
                mmsg.rc = ISMRC_BadClientData;
                ism_common_setError(mmsg.rc);
                break;
            }
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_receive: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
            if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) {   /* BEAM suppression: constant condition */
                TRACE(9, "replyClose from connect=%u\n", transport->index);
                ism_mqtt_replyCloseClient(transport);
            }
            break;

        /*
         * Connect.  This must be the first control packet in the sequence
         */
        case MT_CONNECT:
            if (__sync_bool_compare_and_swap(&pobj->startState, MQTT_NEW, MQTT_IN_PROGRESS)) {
                if (buflen > 1) {
                    uint8_t * verpos = NULL;
                    mmsg.topic_len = BIGINT16(bp);
                    buflen -= 2;
                    bp += 2;
                    mmsg.topic = (char *) bp;
                    bp += mmsg.topic_len;
                    buflen -= mmsg.topic_len;
                    if (buflen <= 0) {
                        mmsg.rc = ISMRC_BadLength;
                        ism_common_setError(mmsg.rc);
                    } else {
                        mmsg.version = *bp;
                        verpos = bp;
                        /* Set bridge mode extension */
                        if (mmsg.version == 0x83 || mmsg.version == 0x84) {
                            mmsg.version &= 0x7f;
                            pobj->mqtt_bridge = 1;
                        }
                        pobj->mqtt_version = mmsg.version;
                        *bp = 0;
                        /* MQTT v3.1 */
                        if (mmsg.topic_len == 6 && !strcmp(mmsg.topic, "MQIsdp") ) {
                            if (mmsg.version != 3 )
                                mmsg.version = 0;
                        }
                        /* MQTT v3.1.1 */
                        else if (mmsg.topic_len == 4 && !strcmp(mmsg.topic, "MQTT") ) {
                            if (mmsg.version != 4 &&
                                (mmsg.version != 5 || !g_AllowMQTTv5))
                                mmsg.version = 0;
                        }
                        /* MQTT proxy protocol.  We allow either V3.1 (3) or V3.1.1 (4) or V5 (5) */
                        else if (g_AllowMQTTpxProtocol && mmsg.topic_len == 6 && !strcmp(mmsg.topic, "MQTTpx") ) {
                            if (mmsg.version != 3 && mmsg.version != 4 && mmsg.version != 5)
                                mmsg.version = 0;
                            pobj->mqtt_proxy = 1;
                        }
                        else {
                            mmsg.version = 0;
                            mmsg.rc = ISMRC_BadClientData;
                            ism_common_setError(mmsg.rc);
                        }
                    }

                    if (mmsg.version > 3 && (kind&0x0f)) {
                        /* In v3.1.1 check reserved bits */
                        mmsg.rc = ISMRC_BadClientData;
                        ism_common_setError(mmsg.rc);
                        break;
                    }

                    /*
                     * If this is an MQTT connect then parse it
                     */
                    if (buflen > 3 && (mmsg.version == 3 || mmsg.version == 4 || mmsg.version == 5)) {
                        mmsg.flags = bp[1];
                        if (pobj->mqtt_proxy) {
                            mmsg.more_flags = bp[2];
                            bp++;
                            buflen--;
                        }
                        if (!strcmp(transport->protocol, "mqtt-tcp")) {
                            switch (mmsg.version) {
                            case 5:
                                transport->protocol = (pobj->mqtt_proxy) ? "mqtt5_px": "mqtt5_tcp";
                                break;
                            case 4:
                                if (pobj->mqtt_proxy)
                                    transport->protocol = "mqtt4-px";
                                else
                                    transport->protocol = (pobj->mqtt_bridge) ? "mqtt4-br" : "mqtt4-tcp";
                                break;
                            default:
                                if (pobj->mqtt_proxy)
                                    transport->protocol = "mqtt-px";
                                 else
                                    transport->protocol = (pobj->mqtt_bridge) ? "mqtt-br" : "mqtt-tcp";
                            }
                        }
                        if (mmsg.version>3 && mmsg.flags&0x01) {
                            /* In v3.1.1 and above check reserved bits */
                            mmsg.rc = ISMRC_BadClientData;
                            ism_common_setError(mmsg.rc);
                        } else {
                            bp += 2;
                            mmsg.keepalive = (uint16_t) BIGINT16(bp);
                            buflen -= 4;
                            bp += 2;

                            /*
                             * Process MQTTv5 properties
                             */
                            mmsg.debug_lvl = 2;           /* Not set */
                            if (mmsg.version >= 5) {
                                mmsg.isExpire = 1;
                                mmsg.msgtype = MTYPE_MQTT_Binary;
                                if (*bp != 0) {
                                    mproplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                                    bp += vilen;
                                    buflen -= vilen;
                                    if (mproplen < 0 || mproplen > buflen) {
                                        mmsg.rc = ISMRC_BadLength;
                                        ism_common_setErrorData(mmsg.rc, "%s", "MQTTPropLen");
                                    } else {
                                        mmsg.rc = ism_common_checkMqttPropFields((char *)bp, mproplen,
                                            g_ctx5, CPOI_CONNECT, mpropCheck, &mmsg);
                                        if (mmsg.rc) {
                                            logProtocolError(&mmsg);
                                            break;
                                        }
                                    }
                                    bp += mproplen;
                                    buflen -= mproplen;
                                } else {
                                    bp++;
                                    buflen--;
                                }
                                pobj->requestReason = mmsg.debug_lvl;
                                pobj->requestReplyInfo = mmsg.request_reply_info;
                                pobj->maxActive = mmsg.maxActive;
                            }

                            /*
                             * Parse the payload
                             */
                            mmsg.payload = (char *) bp;
                            mmsg.payload_len = buflen;
                            parseConnectPayload(&mmsg, transport);

                            /*
                             * Check for max connections
                             */
                            if (pobj->mqtt_proxy && mmsg.domain) {
                                ism_common_HashMapLock(tenantsMap);
                                ima_TenantInfo_t * ti =  (ima_TenantInfo_t*)ism_common_getHashMapElement(tenantsMap,mmsg.domain,mmsg.domain_len);
                                if (ti == NULL) {
                                    ti = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,82),sizeof(ima_TenantInfo_t)+mmsg.domain_len+1);
                                    char * name = (char*) (ti+1);
                                    memcpy(name,mmsg.domain,mmsg.domain_len);
                                    name[mmsg.domain_len] = '\0';
                                    ti->name = name;
                                    ti->nameLen = mmsg.domain_len;
                                    ti->conCount = 0;
                                    ism_common_putHashMapElement(tenantsMap,mmsg.domain,mmsg.domain_len, ti, NULL);
                                }
                                pobj->tenantInfo = ti;
                                ti->maxConAllowed = mmsg.max_connect;
                                if (ti->maxConAllowed && (ti->conCount >= ti->maxConAllowed)) {
                                    transport->name = ism_transport_allocBytes(transport, mmsg.clientid_len + 1, 0);
                                    memcpy((char *) transport->name, mmsg.clientid, mmsg.clientid_len);
                                    ((char *) transport->name)[mmsg.clientid_len] = 0;
                                    transport->clientID = transport->name;
                                    TRACE(5, "Too many connections (%d > %d) for tenant %s. New connection is not allowed: connect=%u\n",
                                            ti->conCount, ti->maxConAllowed, ti->name, transport->index);
                                    mmsg.rc = ISMRC_TooManyConnections;
                                    ism_common_setError(mmsg.rc);
                                    pobj->tenantInfo = NULL;
                                } else {
                                    ti->conCount++;
                                    TRACE(7, "Add connection to tenant %s. allowed=%d, count=%d, connect=%u\n",
                                            ti->name, ti->maxConAllowed, ti->conCount, transport->index);
                                }
                                ism_common_HashMapUnlock(tenantsMap);
                            }
                        }
                    }

                    /*
                     * On a CONNECT we wait until after the parse to make sure we do not trace
                     * a password.
                     */
                    if (SHOULD_TRACE(9)) {
                        int maxlen = ism_common_getTraceMsgData();
                        if (mmsg.rc == 0) {
                            if (maxlen == 0)
                                maxlen = 1000;
                            if (mmsg.password) {
                                if (mmsg.password-buf < maxlen)
                                    maxlen = mmsg.password - buf;
                            }
                        }
                        if (verpos) {
                            *verpos = mmsg.version;
                            if (pobj->mqtt_bridge)
                                *verpos |= 0x80;
                        }
                        sprintf(obuf, "MQTT receive %02x %s connect=%u rc=%d", kind & 0xff,
                                mqttCommand(mmsg.command), transport->index, mmsg.rc);
                        traceDataFunction(obuf, 0, __FILE__, __LINE__, buf, savebuflen, maxlen);
                        if (verpos)
                            *verpos = 0;
                    }

                    /*
                     * Call the common connect processing.  This will raise a lot of the errors
                     */
                    if (!mmsg.rc) {
                        ism_mqtt_doConnect(transport, &mmsg);
                    }
                } else {
                    mmsg.rc = ISMRC_BadLength;
                    ism_common_setError(mmsg.rc);
                }
            } else {
                mmsg.rc = ISMRC_ConnectFirst;
                ism_common_setError(mmsg.rc);
                TRACE(4,  "MQTT ERROR: Got MT_CONNECT while current startState is %d connect=%u client=%s\n",
                        pobj->startState, transport->index, transport->name);
            }
            break;

        /*
         * Disconnect
         */
        case MT_DISCONNECT:
            if (pobj->mqtt_version > 3 && (kind&0x0f)) {
                /* In v3.1.1 check reserved bits */
                mmsg.rc = ISMRC_BadClientData;
                ism_common_setError(mmsg.rc);
                break;
            }
            if (mmsg.version >= 5 && buflen > 0) {
                mmsg.mqtt_rc = *bp++;
                buflen--;
                if (*bp != 0) {
                    mproplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                    bp += vilen;
                    buflen -= vilen;
                    if (mproplen < 0 || mproplen > buflen) {
                        mmsg.rc = ISMRC_BadLength;
                        ism_common_setErrorData(mmsg.rc, "%s", "MQTTPropLen");
                    } else {
                        mmsg.rc = ism_common_checkMqttPropFields((char *)bp, mproplen,
                            g_ctx5, CPOI_DISCONNECT, mpropCheck, &mmsg);
                        if (mmsg.rc) {
                            logProtocolError(&mmsg);
                        }
                    }
                    bp += mproplen;
                    buflen -= mproplen;
                } else {
                    bp++;
                    buflen--;
                }
            }

            /* Stop the keepalive timer for this connection */
            removeFromClientsList(pobj, 1);

            ism_mqtt_doDisconnect(transport, &mmsg);
            break;

        case MT_AUTH:
            if (pobj->mqtt_version >= 5) {
                mmsg.rc = ISMRC_NotAuthorized;
                ism_common_setErrorData(mmsg.rc, "%s", "AUTH");
                break;
            }
            /* Fall thru for version < 5 */

        /*
         * All other packets are an error
         */
        default:
            mmsg.rc = ISMRC_InvalidCommand; /* Invalid command */
            ism_common_setError(mmsg.rc);
            break;
        }
    }

    /*
     * If we detect an error disconnect.
     * It this is a connection request, we have already handled the error.
     */
    if (UNLIKELY(mmsg.rc >= ISMRC_Error)) {
        char * xbuf = alloca(2048);
        if (mmsg.rc != ism_common_getLastError())
            ism_common_setError(mmsg.rc);
        if (xbuf)
            ism_common_formatLastError(xbuf, 2048);
        else
            xbuf= "";
        TRACE(4, "MQTT error: rc=%s (%d) command=%s connect=%u client=%s\n",
                xbuf, mmsg.rc, mqttCommand(mmsg.command), transport->index, transport->name);
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_receive: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
        if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
            TRACE(9, "replyClose from connect=%u\n", transport->index);
            ism_mqtt_replyCloseClient(transport);
        } else {
            transport->close(transport, mmsg.rc, 0, xbuf);
        }
    } else {
        /*
         * Update the last access time (except for a disconnect)
         */
        if (LIKELY(mmsg.command != MT_DISCONNECT)) {
            pobj->messageCount++;
            if (pobj->lastAccessTime)
                pobj->lastAccessTime = (uint64_t) ism_common_readTSC();
        }
    }
    return mmsg.rc;
}

/*
 * Parse a GETX which is used to implement HTTP outbound
 */
static void parseGETX(ism_transport_t * transport, uint8_t * bp, int buflen, mqttMsg_t * mmsg) {
    uint32_t uint;
    uint32_t ackID = 0;
    uint32_t subID = 0;
    uint32_t subopt = 0;
    int32_t  filterlen = 0;
    char * filter  = NULL;
    char * topic = NULL;
    int32_t  topiclen = 0;
    uint32_t timeout = 0;

    if (buflen > 0) {
        int opt = *bp++;
        buflen--;
        /* Parse the ACK ID */
        if (opt&1) {
            if (buflen >= 4) {
                memcpy(&uint, bp, 4);
                ackID = endian_int32(uint);
            }
            bp += 4;
            buflen -= 4;
        }
        /* Parse the Subscription ID */
        if (opt&2) {
            if (buflen >= 2) {
                subID = BIGINT16(bp);
            }
            bp += 2;
            buflen -= 2;
        }
        /* Parse the topic name */
        if (opt&4) {
            if (buflen >= 2) {
                topiclen = BIGINT16(bp);
                if (buflen >= (topiclen+2)) {
                    topic = alloca(topiclen+1);
                    memcpy(topic, bp+2, topiclen);
                    topic[topiclen] = 0;
                }
                bp += (topiclen+2);
                buflen -= (topiclen+2);
            }
        }
        /* Parse the timeout */
        if (opt&8) {
            if (buflen >= 4) {
                memcpy(&uint, bp, 4);
                timeout = endian_int32(uint);
            }
            bp += 4;
            buflen -= 4;
        }
        /* Parse subscription options */
        if (opt&16) {
            if (buflen >= 2) {
                subopt = BIGINT16(bp);
            }
            bp += 2;
            buflen -= 2;
        }
        /* Parse subscription filter */
        if (opt&32) {
            if (buflen >= 2) {
                filterlen = BIGINT16(bp);
                if (buflen >= (filterlen+2)) {
                    filter = alloca(filterlen+1);
                    memcpy(filter, bp+2, filterlen);
                    filter[filterlen] = 0;
                }
                bp += (filterlen+2);
                buflen -= (filterlen+2);
            }
        }
    } else {
        buflen--;
    }

    if (buflen != 0) {
        mmsg->rc = ISMRC_BadClientData;
        ism_common_setErrorData(mmsg->rc, "%s", "GETX");
    } else {
        doGETX(transport, mmsg, ackID, subID, topic, timeout, subopt, filter);
    }
}


/*
 * Reply with a PUBLISHX for a failure
 */
static void replyGetx(int rc, void * handle, void * vaction) {
    mqtt_prodcons_t * consumer = (mqtt_prodcons_t *)vaction;
    ism_transport_t * transport = consumer->transport;

    if (consumer->publishX != 2)
        consumer = transport->pobj->prodcons[consumer->which];   /* Make sure we have the real object */
    if (rc) {
        char xbuf[2048];
        char ebuf[1024];
        concat_alloc_t   zbuf = {xbuf, sizeof xbuf, 16};
        concat_alloc_t * buf = &zbuf;
        int  extlen;
        if (rc != ism_common_getLastError())
            ism_common_setError(rc);
        ism_common_formatLastError(ebuf, sizeof ebuf);

        bputchar(buf, 0x20);       /* Has extension */
        buf->used += 2;            /* Extension length */
        ism_common_putExtensionValue(buf, EXIV_ServerRC, rc);
        ism_common_putExtensionString(buf, EXIV_ReasonString, ebuf);
        extlen = buf->used-19;
        buf->buf[17] = (char)(extlen>>8);
        buf->buf[18] = (char)extlen;
        bputchar(buf, 0);          /* Zero length topic */
        bputchar(buf, 0);
        transport->send(transport, buf->buf+16, buf->used-16, MT_CMD_PUBLISHX, SFLAG_FRAMESPACE);
    }
    /*
     * Close the connection in the case of invalid CONNECT
     * If close is in progress, proceed with it instead of this close
     */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in replyGetx: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, transport->pobj->inprogress, transport->pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&transport->pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
        return;
    }
}

/*
 * Get a message
 */
static int getMessage(ism_transport_t * transport, mqtt_prodcons_t * consumer, uint32_t timeout) {
    int  rc;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
        (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE+consumer->qos);
    if (consumer->flags & MSUBOPT_NoLocal)
        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL;
    if (consumer->subID)
        subAttrs.subId = consumer->subID;
    rc = ism_engine_getMessage(transport->pobj->session_handle, ismDESTINATION_SUBSCRIPTION , consumer->topic,
                             &subAttrs, timeout, NULL,
                             consumer, sizeof(*consumer), ism_mqtt_replyMessage, NULL, 0,
                             consumer, sizeof(*consumer), replyGetx);
    if (rc != ISMRC_AsyncCompletion) {
        replyGetx(rc, NULL, consumer);
    }
    return rc;
}


/*
 * Reply from subscribe, get the message
 */
static void replyGetxSubscription(int rc, void * handle, void * vaction) {
    mqtt_prodcons_t * consumer = (mqtt_prodcons_t *)vaction;
    ism_transport_t * transport = consumer->transport;
    consumer = transport->pobj->prodcons[consumer->which];   /* Make sure we have the real object */
    getMessage(transport, consumer, transport->pobj->timeout);
}


/*
 * Implement a GETX
 */
static int  doGETX(ism_transport_t * transport, mqttMsg_t * mmsg, uint32_t ackID, int subID, const char * topic,
        uint32_t timeout, uint32_t subopt, const char * filter) {
    int i;
    int id;
    int rc;
    ism_field_t   field;
    mqtt_prodcons_t * consumer;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    ism_prop_t * cprops = NULL;
    ismRule_t *  rule = NULL;
    int          rulelen = 0;

    /* Find the subscription */
    for (i= 1; i < transport->pobj->prodcons_alloc; i++) {
        consumer = transport->pobj->prodcons[i];
        if (consumer && (consumer->closed == 0) && consumer->topic) {
            if ((subID != 0 && consumer->subID == subID) ||
                (topic && *topic && !strcmp(consumer->topic, topic))) {
                getMessage(transport, consumer, timeout);
                return 0;
            }
        }
    }

    /* If the subscription does not exist, create the subscription and consumer */

    /* Process a filter */
    if (filter) {
        rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, filter, SELOPT_Internal);
        if (rc) {
            mqtt_prodcons_t xconsumer = {0};
            char * xxbuf = alloca(1024);
            ism_common_formatLastError(xxbuf, 1024);
            TRACE(2, "GETX filter not valid: connect=%u client=%s rc=%d reason=%s\n",
                    transport->index, transport->name, rc, xxbuf);
            xconsumer.transport = transport;
            xconsumer.publishX = 2;
            replyGetx(rc, NULL, &xconsumer);
        }
    }

    /* Set up consumer object */
    id = findProdcons(transport);
    consumer = transport->pobj->prodcons[id];
    memset(consumer, 0, sizeof(mqtt_prodcons_t));
    consumer->transport = transport;
    consumer->topic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),topic);
    consumer->qos = subopt&3;
    consumer->flags = subopt&0xFC;   /* Zero out CONFLAG_UseFilter and CONFLAG_RerunFilter */
    consumer->subID = subID;
    consumer->publishX = 1;
    if (rule && rulelen)
        consumer->flags |= CONFLAG_UseFilter;       /* Shared sub, not recovered */
    consumer->which = id;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE+(subopt&0x03));
    subAttrs.subId = subID;

    /* Set nolocal */
    if (subopt & MSUBOPT_NoLocal) {
        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL;
    }

    /* Set Subscription ID */
    cprops = ism_common_newProperties(20);
    if (subID) {
        field.type = VT_Integer;
        field.val.i = subID;
        consumer->subID = subID;
        ism_common_setProperty(cprops, "SubID", &field);
    }
    /* Set filter */
    if (rulelen > 0) {
        field.type = VT_ByteArray;
        field.val.s = (char *)rule;
        field.len   = rulelen;
        ism_common_setProperty(cprops, "Selector", &field);
        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION;
    }
    field.type = VT_Byte;
    field.val.i = consumer->publishX;
    ism_common_setProperty(cprops, "GETX", &field);

    transport->pobj->timeout = timeout;

    rc = ism_engine_createSubscription(transport->pobj->handle,
             consumer->topic, cprops, ismDESTINATION_TOPIC, topic, &subAttrs,
             transport->pobj->handle, consumer, sizeof(*consumer), replyGetxSubscription);
    if (cprops) {
        ism_common_freeProperties(cprops);
        cprops = NULL;
        if (rule && rulelen) {
            ism_common_freeSelectRule(rule);
        }
    }
    if (rc != ISMRC_AsyncCompletion) {
        replyGetxSubscription(rc, NULL, consumer);
    }
    return 0;
}


/*
 * Parse the binary mqtt CONNECT payload in binary MQTT
 */
static void parseConnectPayload(mqttMsg_t * mmsg, ism_transport_t * transport) {
    uint8_t * bp = (uint8_t *) mmsg->payload;
    int buflen = mmsg->payload_len;
    transport->pobj->maxExpire = 0xffffffffu;

    /* Parse the client id */
    if (buflen > 1)
        mmsg->clientid_len = BIGINT16(bp);
    bp += 2;
    mmsg->clientid = (char *) bp;
    buflen -= (2 + mmsg->clientid_len);
    bp += mmsg->clientid_len;

    /* Parse the will topic and message */
    if (mmsg->flags & CFLAG_Will) {
        /* Will Properties */
        mmsg->msgtype = MTYPE_MQTT_Binary;
        if (transport->pobj->mqtt_version >= 5) {
            if (buflen > 0) {
                int wproplen = *bp;
                if (bp) {
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
                            mmsg->willproplen = wproplen;
                            mmsg->willprops = (char *)bp;
                            if (mmsg->rc == 0) {
                                mmsg->rc = ism_common_checkMqttPropFields((char *)bp, wproplen,
                                    g_ctx5, CPOI_WILLPROP, mpropCheck, mmsg);
                                if (mmsg->rc) {
                                    logProtocolError(mmsg);
                                }
                            }
                        }
                        bp += wproplen;
                        buflen -= wproplen;
                    }
                } else {
                    /* For now, allow with or without Will Properties */
                    if (g_AllowMQTTv5 > 1 || bp[1]==0) {
                        buflen--;
                        bp++;
                    }
                }
            }
        }

        /* Will Topic */
        if (buflen > 1)
            mmsg->willtopic_len = BIGINT16(bp);
        bp += 2;
        mmsg->willtopic = (char *) bp;
        buflen -= (2 + mmsg->willtopic_len);
        bp += mmsg->willtopic_len;

        /* Will Payload */
        if (buflen > 1)
            mmsg->willpayload_len = BIGINT16(bp);
        bp += 2;
        mmsg->willpayload = (char *) bp;
        buflen -= (2 + mmsg->willpayload_len);
        bp += mmsg->willpayload_len;
    } else {
        if (mmsg->version > 3 && (mmsg->flags & (CFLAG_WillRetain|CFLAG_WillQoS))) {
            /* In v3.1.1 check reserved bits */
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
            /* In v3.1.1 check reserved bits */
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
            bp += mmsg->password_len;
        }  else if (mmsg->version > 3) {   /* In v3.1 we ignore this */
            /* In v3.1.1 check reserved bits */
            mmsg->rc = ISMRC_BadClientData;
            ism_common_setErrorData(mmsg->rc, "%s", "password");
        }
    }

    if (mmsg->more_flags) {
        if (mmsg->more_flags & XCFLAG_Domain) {
            mmsg->domain_len = BIGINT16(bp);
            bp += 2;
            mmsg->domain = (char *) bp;
            buflen -= (2 + mmsg->domain_len);
            bp += mmsg->domain_len;
        }
        if (mmsg->more_flags & XCFLAG_MaxConn) {
            int  xint;
            memcpy((char *)&xint, bp, 4);
            mmsg->max_connect = endian_int32(xint);
            bp += 4;
            buflen -= 4;
        }
        if (mmsg->more_flags & XCFLAG_ExtraFlags) {    /* This was NotifyTopic in older proxies */
            int maxActive;
            mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
            mmsg->ext_len = BIGINT16(bp);
            bp += 2;
            buflen -= 2;
            mmsg->ext = (char *) bp;
            bp += mmsg->ext_len;
            buflen -= mmsg->ext_len;
            pobj->publishNAK =   ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_SendNAK, 0);
            pobj->qos0NAK =      ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_SendQoS0NAK, 0);
            pobj->subscribeNAK = ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_SubscribeNAK, 0);
            pobj->serverInfo =   ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_ServerInfo, 0);
            pobj->sendSubs =     ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_SendSubs, 0);
            uint8_t aclwait =    ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_ACLWait, 0);
            if (aclwait) {
                pthread_spin_lock(&transport->lock);
                pobj->aclwait = aclwait;
                pthread_spin_unlock(&transport->lock);
            }
            pobj->msgRate =      ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_ExpectedMsgRate, 0);
            maxActive =          ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_MaxActive, pobj->maxActive);
            pobj->maxExpire =    ism_common_getExtensionLong( mmsg->ext, mmsg->ext_len, EXIV_ExpireTTL, 0xffffffffu);
            if (!pobj->maxActive || maxActive < pobj->maxActive)
                pobj->maxActive = maxActive;
            pobj->monitor_opt =  ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_MonitorOpt, 0);
            int monlen;
            const char * montopic = ism_common_getExtensionString(mmsg->ext, mmsg->ext_len, EXIV_MonitorTopic, &monlen);
            if (montopic) {
                pobj->monitor_topic = (char *)ism_transport_allocBytes(transport, monlen+1, 0);
                memcpy(pobj->monitor_topic, montopic, monlen);
                pobj->monitor_topic[monlen] = 0;
            }
            const char * locale = ism_common_getExtensionString(mmsg->ext, mmsg->ext_len, EXIV_Locale, &monlen);
            if (locale && monlen > 0) {
                if (monlen > 7)
                    monlen = 7;
                memcpy(transport->pobj->locale, locale, monlen);
                transport->pobj->locale[monlen] = 0;
            }
        }
        if (mmsg->more_flags & XCFLAG_ClientAddr) {
            int xlen = BIGINT16(bp);
            bp += 2;
            transport->client_host = ism_transport_allocBytes(transport, xlen+1, 0);
            memcpy((char *)transport->client_host, bp, xlen);
            memset((char *)(transport->client_host+xlen), 0, 1);  /* null terminate */
            buflen -= (2 + xlen);
            bp += xlen;
        }
        if (mmsg->more_flags & XCFLAG_CertName) {
            int xlen = BIGINT16(bp);
            bp += 2;
            transport->cert_name = ism_transport_allocBytes(transport, xlen+1, 0);
            memcpy((char *)transport->cert_name, bp, xlen);
            memset((char *)(transport->cert_name+xlen), 0, 1);   /* null terminate */
            buflen -= (2 + xlen);
            bp += xlen;
        }
        if (mmsg->more_flags & XCFLAG_Port) {
            uint16_t  xshort;
            memcpy((char *)&xshort, bp, 2);
            transport->serverport = endian_int16(xshort);
            bp += 2;
            buflen -= 2;
        }
        if (mmsg->more_flags & XCFLAG_NoLog) {
            transport->nolog = 1;
        }
    }
    if (mmsg->rc == 0) {
        if ((buflen < 0)) {
            mmsg->rc = ISMRC_BadLength;
            ism_common_setErrorData(mmsg->rc, "%s", "short");
        }
        if (mmsg->version > 3 && buflen != 0) {
            /* In v3.1.1 check reserved bits */
            mmsg->rc = ISMRC_BadLength;
            ism_common_setErrorData(mmsg->rc, "%s", "extra");
        }
    }
}


/*
 * Return the name of a command given the value.
 * This is used for trace and debug.
 */
static const char * mqttCommand(int ixin) {
    int ix;
    ixin &= 0xff;
    if (ixin < 0x20 && ixin != 0x10) {
        ix = ixin;
    } else {
        ix = (ixin>>4) & 0xf;
        if ((ixin & 0xF) == 0xF) {
            switch (ix) {
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
 * Validate a message ID.
 * The message ID must be between 1 and 65535.
 * @param mmsg   A MQTT message object with the msdid field set
 * @return The mmsg return code which may now contain ISMRC_InvalidID
 */
static int chkmsgID(mqttMsg_t * mmsg) {
    if (mmsg->rc == 0) {
        if (mmsg->msgid < 1 || mmsg->msgid > 65535) {
            mmsg->rc = ISMRC_InvalidID;
            ism_common_setErrorData(mmsg->rc, "%u", mmsg->msgid);
        }
    }
    return mmsg->rc;
}

/*
 * Fix problems with the topic for trace and log.
 * Replace invalid characters and all characters >= 0x80 with ?
 */
static const char * fixtopic(const char * topic, int len, char * out, int uni) {
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
 * Parse the selector specification
 *
 * For now only two selectors are allowed:
 *     $select/QoS=0/
 *     $select/Qos>0/
 */
static int parseSelect(int len, const char * path, int * selectors) {
    int retlen = 0;
    int retsel = 0;
    if (len > 8 && !memcmp(path, "$select/", 8)) {
        if (len >= 15 && !memcmp(path+8, "QoS=0/", 6)) {
            retlen = 14;
            retsel = ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY;
        } else if (len >= 15 && !memcmp(path+8, "QoS>0/", 6)) {
            retlen = 14;
            retsel = ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY;
        } else {
            retlen = -1;
        }
    }
    if (selectors)
        *selectors = retsel;
    return retlen;
}


/*
 * Check if the String is valid.
 * We need to validate some strings, including those which contain topic names.
 * Topic names have additional restrictions including whether the wildcards are
 * allowed as if they are valid.
 *
 * This is only a check of limits imposed by the MQTT spec, not those imposed
 * by the ISM engine.
 */
static int checkString(mqttMsg_t * mmsg, const char * s, int len, int topicchk) {
    const char * pos;
    if (mmsg->rc == 0) {
        /*
         * In V3.1.1 we do not allow Unicode non-characters or control characters
         * In V3.1 we only prohibit a 0 character
         */
        int check = mmsg->version > 3 ? UR_NoControl | UR_NoNonchar : UR_NoNull;
        if (topicchk == CHK_PUBTOPIC)
            check |= UR_NoWildcard;
        int count = ism_common_validUTF8Restrict(s, len, check);
        if (count < 0) {
            count = ism_common_validUTF8(s, len);
            if (count < 0) {
                mmsg->rc = ISMRC_UnicodeNotValid;
                ism_common_setErrorData(mmsg->rc, "%s", FIXUNICODE(s, len));
            } else {
                if (topicchk == 0) {
                    if (mmsg->version >= 5)
                        mmsg->rc = ISMRC_ProtocolError;
                    else
                        mmsg->rc = ISMRC_NotAuthorized;
                } else {
                    mmsg->rc = ISMRC_BadTopic;
                }
                ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len));
            }
        } else {
            if (topicchk) {
                if (count < 1) {
                    mmsg->rc = ISMRC_BadTopic;
                    ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len));
                } else if (topicchk == CHK_PUBTOPIC) {
                    if (s[0] == '$') {
                        if (count >= 20 && !memcmp(s, "$SharedSubscription/", 20)) {
                            mmsg->rc = ISMRC_BadTopic;
                            ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len));
                        } else if (count >= 7 && !memcmp(s, "$share/", 7)) {
                            mmsg->rc = ISMRC_BadTopic;
                            ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len));
                        } else if (count >= 8 && !memcmp(s, "$select/", 8)) {
                            mmsg->rc = ISMRC_BadTopic;
                            ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len));
                        }
                    }
                } else if (topicchk == CHK_SUBTOPIC) {
                    int i;
                    if (s[0] == '$') {
                        if (len >= 8 && !memcmp(s, "$select/", 8)) {
                            int skip = parseSelect(len, s, NULL);
                            if (skip > 0) {
                                s += skip;
                                len -= skip;
                            } else {
                                mmsg->rc = ISMRC_BadTopic;
                                ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len));
                            }
                        }
                    }
                    if (mmsg->rc == 0 && s[0] == '$') {
                        if (len >= 20 && !memcmp(s, "$SharedSubscription/", 20)) {
                            pos = memchr(s+20, '/', len-20);
                            if (!pos || pos==(s+20)) {
                                mmsg->rc = ISMRC_BadTopic;
                                ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len));
                            } else {
                                pos++;
                                len -= (pos-s);
                                if (len == 0 || *pos == '$') {
                                    mmsg->rc = ISMRC_BadTopic;
                                    ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len+(pos-s)));
                                }
                                s = pos;
                                count = len;        /* Just need it to be zero or not */
                            }
                        } else if (mmsg->v5_shared && len >= 7 && !memcmp(s, "$share/", 7)) {
                            pos = memchr(s+7, '/', len-7);
                            if (!pos || pos==(s+7)) {
                                mmsg->rc = ISMRC_BadTopic;
                                ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len));
                            } else {
                                pos++;
                                len -= (pos-s);
                                if (len == 0 || *pos == '$') {
                                    mmsg->rc = ISMRC_BadTopic;
                                    ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len+(pos-s)));
                                }
                                s = pos;
                                count = len;        /* Just need it to be zero or not */
                            }
                        } else {
                            if (len<5 || s[1]!='S' || s[2]!='Y' || s[3]!='S' || s[4]!='/') {
                                if (len !=5 || s[1] != '_' || s[2] != '$') {
                                    mmsg->rc = ISMRC_BadSysTopic;
                                    ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len));
                                }
                            }
                        }
                    }
                    if (mmsg->rc == 0) {
                        for (i = 0; i < len; i++) {
                            if (s[i] == '#') {
                                if ((i > 0 && s[i - 1] != '/')
                                        || (i + 1 != len)) {
                                    mmsg->rc = ISMRC_BadTopic;
                                    ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len));
                                    break;
                                }
                            } else if (s[i] == '+') {
                                if ((i > 0 && s[i - 1] != '/')
                                        || (i + 1 != len && s[i + 1] != '/')) {
                                    mmsg->rc = ISMRC_BadTopic;
                                    ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(s, len));
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return mmsg->rc;
}


/*
 * Validate a namepair (user property)
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
 * Convert proxy protocol properties to internal properties.
 * All other extension values are ignored here as they have been dealt with previously
 *
 * We do not check the validity of the bytes in the property as we assume that proxy
 * protocol comes from a trusted source.o
 */
static void extPublishProps(int item, const char * ptr, int64_t value, void * userdata) {
    concat_alloc_t * buf = (concat_alloc_t *)userdata;
    if (item == EXIV_Property) {
        int len = (int)value;
        const char * vp = memchr(ptr, 0, len);
        if (vp) {
            ism_protocol_putNameValue(buf, ptr);
            vp++;
            ism_protocol_putStringLenValue(buf, vp, len-(vp-ptr));
        }
    }
}

/*
 * Convert MQTTv5 properties to internal properties.
 * We have already validated them for correctness
 */
static int mpropPublishProps(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value) {
    int namelen;
    int valuelen;
    concat_alloc_t * buf = (concat_alloc_t *)userdata;
    switch (fld->type) {
    case MPT_String:
        switch (fld->id) {
        case MPI_ReplyTopic:
            ism_protocol_putNameIndex(buf, ID_ReplyToT);
            ism_protocol_putStringLenValue(buf, ptr, len);
            break;
        case MPI_ContentType:
            ism_protocol_putNameIndex(buf, ID_JMSType);
            ism_protocol_putStringLenValue(buf, ptr, len);
            break;
        }
        break;
    case MPT_Bytes:
        switch (fld->id) {
        case MPI_Correlation:
            ism_protocol_putNameIndex(buf, ID_CorrID);
            ism_protocol_putByteArrayValue(buf, ptr, len);
            break;
        }
        break;
    case MPT_NamePair:
        namelen = BIGINT16(ptr);
        valuelen = BIGINT16(ptr+2+namelen);
        ism_protocol_putNameLenValue(buf, ptr+2, namelen);
        ism_protocol_putStringLenValue(buf, ptr+4+namelen, valuelen);
        break;
    }
    return 0;
}


/*
 * MQTTv5 property checker
 * This is a callback used for checking identifier/value pairs.
 * It can also be used to store the item.
 */
static int mpropCheck(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value) {
    mqttMsg_t * mmsg = (mqttMsg_t *) userdata;
    if (fld->type==MPT_String) {
        if (ism_common_validUTF8Restrict(ptr, len, UR_NoControl | UR_NoNonchar) < 0) {
            char * out;
            if (len > 256)
                len = 256;
            out = alloca(len+1);
            ism_common_validUTF8Replace(ptr, len, out, UR_NoControl | UR_NoNonchar, '?');
            ism_common_setErrorData(ISMRC_UnicodeNotValid, "%s%s", fld->name, out);
            return ISMRC_UnicodeNotValid;
        }
    }
    if (fld->type == MPT_NamePair) {
        return validateNamePair(ptr, len);
    }
    switch (fld->id) {
    case MPI_PayloadFormat:
        if (value>1) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s%u", fld->name, value);
            return ISMRC_ProtocolError;
        }
        mmsg->msgfmt = (uint8_t)value;
        if (value==1)
            mmsg->msgtype = MTYPE_MQTT_Text;
        break;
    case MPI_MsgExpire:
        mmsg->isMsgExpire = value != 0;
        mmsg->msgExpire = value;
        break;
    case MPI_SessionExpire:
        mmsg->isExpire = 1;
        mmsg->expireTTL = value;
        break;
    case MPI_MaxReceive:
        if (value == 0) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s%u", fld->name, value);
            return ISMRC_ProtocolError;
        }
        mmsg->maxActive = value < 64535 ? value: 0;;
        break;
    case MPI_MaxTopicAlias:
        mmsg->topic_alias = value;
        break;
    case MPI_TopicAlias:
        mmsg->topic_alias = value;
        break;
    case MPI_ReplyTopic:
        if (ism_common_validUTF8Restrict(ptr, len, UR_NoControl | UR_NoWildcard) < 0) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s%s", fld->name, ptr);
            return ISMRC_ProtocolError;
        }
        break;
    case MPI_MaxPacketSize:
        if (value == 0) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s", "Maximum Packet Size");
            return ISMRC_ProtocolError;
        }
        if (value < 64) {
            ism_common_setErrorData(ISMRC_Error, "%s", "The Maximum Packet Size is too small");
            return ISMRC_Error;
        }
        mmsg->maxPacketSize = value;
        break;
    case MPI_SubID:
        if (value == 0) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s%u", fld->name, value);
            return ISMRC_ProtocolError;
        }
        mmsg->subID = value;
        break;
    case MPI_RequestReplyInfo:
        if (value>1) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s%u", fld->name, value);
            return ISMRC_ProtocolError;
        }
        mmsg->request_reply_info = value;
        break;
    case MPI_RequestReason:
        /* We allow a special value of 9 to send server info */
        if (value > 1 && value != 9) {
            ism_common_setErrorData(ISMRC_ProtocolError, "%s%u", fld->name, value);
            return ISMRC_ProtocolError;
        }
        mmsg->debug_lvl = value;
        break;
    case MPI_WillDelay:
        mmsg->will_delay = value;
        break;
    case MPI_Reason:
        mmsg->reason = ptr;
        mmsg->reason_len = len;
        break;

    case MPI_AuthMethod:    /* Extended auth not supported */
    case MPI_AuthData:
        ism_common_setErrorData(ISMRC_NotAuthorized, "%s", fld->name);
        return ISMRC_NotAuthorized;
    }
    return 0;
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

/*
 * Base52 is used to remove vowels which removes almost all objectionable character
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
static int generate_cid(ism_transport_t * transport, char * buf) {
    uint64_t rval;
    uint8_t * randbuf = (uint8_t *)&rval;
    char * bp = buf + 1;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    RAND_pseudo_bytes(randbuf, 8);
#else
    RAND_bytes(randbuf, 8);
#endif
    buf[0] = '_';
    if (transport->client_addr && *transport->client_addr)
        strcpy(bp, transport->client_addr);
    else
        strcpy(bp, "client");
    bp += strlen(bp);
    *bp++ = '_';
    for (int i=0; i<8; i++) {
        *bp++ = base52[(int)(rval%52)];
        rval /= 52;
    }
    *bp++ = 0;
    return bp-buf;
}


/*
 * Authenticate with delay
 */
static int mqtt_authenticate(ism_transport_t * transport, mqttMsg_t * mmsg, mqtt_act_t * act, int authnRequired) {
    ism_time_t delay_nanos = 0;
    int limit = 0;

    if (ism_throttle_isEnabled()) {
            limit = ism_throttle_getThrottleLimit(transport->clientID, THROTTLET_HIGHEST_COUNT);
            if (limit>0) {
                    delay_nanos = ism_throttle_getDelayTimeInNanos(limit);
            }
    }

    act->rc = 0;
    TRACE(7, "Authentication: submit MQTT async authentication: connect=%u client=%s user=%s required=%u, throttle.limit=%d, delay=%llu\n" ,
                    transport->index, transport->name, transport->userid, authnRequired, limit, (ULL)delay_nanos);

    ism_common_setError(0);
    ism_security_authenticate_user_async(transport->security_context,
            transport->userid, mmsg->username_len, (const char*) mmsg->password, mmsg->password_len,
            act, sizeof(mqtt_act_t), mqttReplyAuth, authnRequired, delay_nanos);
    return 0;
}

/*
 * Process an MQTT connect message.
 * We start this here, but must complete it in the engine reply.
 *
 * MQTTv3 only has a small number of errors which are returned as return codes on
 * CONACK.  All other errors just cause the connection to be terminated with notification.
 * We do put an entry in our log about why this occurred.
 */
void ism_mqtt_doConnect(ism_transport_t * transport, mqttMsg_t * mmsg) {
    mqtt_act_t act = {0};
    int count = 0;
    int maxcid = 1023;      /* Max clientID size in all cases */
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

    ism_common_setError(0);

    /*
     * Check if the server is ready for messaging.
     */
    if (!(serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED)) {
        mmsg->rc = CRC_NotAvailable;
        ism_common_setError(mmsg->rc);
        memset(&act, 0, sizeof(mqtt_act_t));
        act.transport = transport;
        act.command = mmsg->command;
        act.rc = mmsg->rc;
        ism_mqtt_error(&act);
        return;
    }

    /*
     * Check valid connection
     */
    if (mmsg->clientid != NULL) {
        /*
         * In V3.1.1 we allow a zero length clientID but only for cleansession=1.
         * In this case we generate a clientID.
         */
        if (mmsg->clientid_len == 0 &&
                (((mmsg->flags & CFLAG_Clean) && mmsg->version > 3) || (mmsg->version >= 5))) {
            char * cidbuf = alloca((transport->client_addr ? strlen(transport->client_addr) : 6) + 12);
            mmsg->clientid_len = count = generate_cid(transport, cidbuf);
            mmsg->clientid = (const char *) cidbuf;
            transport->pobj->generated_cid = 1;
        } else {
            /*
             * Check the clientID.  We do not allow clientIDs starting with a double
             * underscore as these are reserved for system use.
             */
            int check = mmsg->version > 3 ? UR_NoControl | UR_NoNonchar : UR_NoNull;
            count = ism_common_validUTF8Restrict(mmsg->clientid, mmsg->clientid_len, check);
            if (count > 1 && mmsg->clientid[0]=='_' && mmsg->clientid[1]=='_') {
                mmsg->rc = CRC_BadIdentifier;
                ism_common_setError(ISMRC_BadClientID);
            }
        }
    }


    /*
     * Change the rule in all cases to make the max clientID size 1023.
     */
    if (pobj->handle) {
        mmsg->rc = ISMRC_ConnectFirst;
        ism_common_setError(mmsg->rc);
    } else if (mmsg->version != 3 && mmsg->version != 4 && mmsg->version != 5) {
        /* Version must be either 3 (V3.1) or 4 (V3.1.1) or 5 (V5) */
        ism_common_setError(ISMRC_ProtocolVersion);
        mmsg->rc = CRC_InvalidVersion;
        mmsg->version = pobj->mqtt_version;
    } else if (mmsg->clientid == NULL || count < 1 || count > maxcid) {
        ism_common_setError(ISMRC_BadClientID);
        mmsg->rc = CRC_BadIdentifier;
    } else {
        /* Save the client ID */
        transport->name = ism_transport_allocBytes(transport, mmsg->clientid_len + 1, 0);
        memcpy((char *) transport->name, mmsg->clientid, mmsg->clientid_len);
        ((char *) transport->name)[mmsg->clientid_len] = 0;


        if (mmsg->flags & CFLAG_Will) {
            if (!mmsg->willpayload || !mmsg->willtopic) {
                mmsg->rc = ISMRC_WillRequired;
                ism_common_setError(mmsg->rc);
            } else {
                mmsg->v5_shared = transport->pobj->v5_shared;
                checkString(mmsg, mmsg->willtopic, mmsg->willtopic_len, CHK_PUBTOPIC);
                /*
                 * Use the same rule for will topics as for normal topics:
                 * The will topic name cannot start with a $
                 */
                if (mmsg->willtopic[0] == '$') {
                    int msgcode = 2210;
                    if (mmsg->version > 3) {
                        mmsg->rc = ISMRC_BadSysTopic;     /* For V3.1.1 close the connection */
                        ism_common_setErrorData(mmsg->rc, "%s", FIXTOPIC(mmsg->willtopic, mmsg->willtopic_len));
                    }
                    TRACE(4, "A system topic is not valid as a will topic: connect=%u client=%s\n", transport->index, transport->name);

                    if (!previouslyLogged(pobj, msgcode)) {
                        LOG(WARN, Connection, 2110, "%u%-s%-s%-s",
                                "The client request to publish a message to the server failed because the topic is a system topic: "
                                "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.",
                                transport->index, transport->name, transport->protocol, transport->listener->name);
                    }
                    mmsg->flags &= ~CFLAG_Will;   /* Do not send will */
                }
            }
            /* Copy the will delay */
            if (mmsg->will_delay) {
                pobj->will_delay = mmsg->will_delay;
            }
        }

        transport->userid = NULL;
        if (mmsg->username && mmsg->username_len > 0
                && checkString(mmsg, mmsg->username, mmsg->username_len, 0) == 0) {
            transport->userid = ism_transport_allocBytes(transport, mmsg->username_len + 1, 0);
            memcpy((char *) transport->userid, mmsg->username, mmsg->username_len);
            ((char *) transport->userid)[mmsg->username_len] = 0;
        } else if (mmsg->username && mmsg->username_len == 0) {
            transport->userid = "";
        }
        transport->clientID = transport->name;

        /* Check for not allowed */
        if (mmsg->rc == 0) {
            mmsg->rc = ism_transport_clientAllowed(transport);
            if (mmsg->rc == ISMRC_ServerNotAvailable)
                mmsg->rc = CRC_NotAvailable;
        }
    }

    /* Create the action */
    memset(&act, 0, sizeof(mqtt_act_t));
    act.transport = transport;
    act.command = mmsg->command;
    act.rc = mmsg->rc;
    if (act.rc) {
        if ((act.rc > CRC_MinConnectRc && act.rc <= CRC_MaxConnectRc) || (mmsg->version >= 5))
            ism_mqtt_error(&act);
        return;
    }

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
     * Save the will information
     */
    if (mmsg->rc == 0 && (mmsg->flags & CFLAG_Will)) {
        if (((mmsg->flags >> 3) & 3) == 3) {
            mmsg->rc = ISMRC_InvalidQoS;
            ism_common_setError(mmsg->rc);
            return;
        }
        ism_mqtt_setWill(transport, mmsg, &act);
    }

    /*
     * Start keepalive timer, if necessary
     */
    if (mmsg->keepalive) {
        if (mqttMaxKeepAlive && (!mmsg->keepalive || (mmsg->keepalive > mqttMaxKeepAlive))) {
            mmsg->keepalive = mqttMaxKeepAlive;
            pobj->send_keepalive = 1;
        }
        pobj->messageCount = 0;
        if (!pobj->keepAlive)
            pobj->keepAlive = -mmsg->keepalive;
        pobj->lastAccessTime = (uint64_t)(ism_common_readTSC());
    }
    pobj->transport = transport;
    pobj->maxActive = mmsg->maxActive;
    pobj->maxPacketSize = mmsg->maxPacketSize;
    transport->closestate[3] = 0;     /* Connection complete */

    /* Set state for the engine client object */
    if (!(mmsg->flags & CFLAG_Clean)) {
        pobj->cleansession = 0;
        transport->durable = 1;
    } else {
        pobj->cleansession = 1;
    }
    if (g_AllowNewShared)
        pobj->v5_shared = 1;

    /* Copy more fields from the message parse */
    pobj->expire_set = mmsg->isExpire;
    pobj->expireTTL = mmsg->expireTTL;
    if ((pobj->expire_set || pobj->mqtt_version >= 5) && pobj->expireTTL > 0)
        transport->durable = 1;

    transport->ready = 1;       /* We no longer need the DoS timer  */
    TRACE(7, "Connect MQTT: connect=%u client=%s clean=%d expire=%u keepalive=%d version=%d\n",
            transport->index, transport->name, pobj->cleansession,
            pobj->expire_set ? pobj->expireTTL : -1, (int)pobj->keepAlive, pobj->mqtt_version);

    /*
     * Call asynchronous authentication if the usePasswordAuth is true or the username is specified
     * for non-TLSL connections, but not if no user check is requested.
     */
    int authnRequired = transport->listener->usePasswordAuth != 0;
    if (!transport->listener->secure && transport->userid && !(mmsg->more_flags & XCFLAG_NoUserCheck)) {
        authnRequired = 1;
    }

    if (authnRequired) {
        int isltpa = ism_security_context_isLTPA(transport->security_context);
        int isoauth = ism_security_context_isOAuth(transport->security_context);
        if (isoauth || isltpa) {
            if (ism_protocol_auth((char *)transport->userid, isoauth, isltpa)) {
                mmsg->rc = CRC_NotAuthorized;
                ism_common_setError(ISMRC_NotAuthorized);
                act.rc = mmsg->rc;
                ism_mqtt_error(&act);
                return;
            }
        }
    }
    mqtt_authenticate(transport, mmsg, &act, authnRequired);
}


/*
 * Process stolen clientID.
 * If a clientID is reused in MQTT, the old connection is closed and the clientID
 * is stolen.  This differs from how JMS works where we ping the old client to see if
 * it still exists.
 */
void ism_mqtt_replySteal(int32_t rc, ismEngine_ClientStateHandle_t handle, uint32_t options, void * vaction) {
    ism_transport_t * transport = (ism_transport_t *) vaction;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    char xbuf [2048];
    void * handleCheck;
    const char * name = transport->name;

    /*
     * If connect is not finished yet, indicate that the client state
     * was stolen and needs to be destroyed.  First check that we do not have
     * a stale pointer or the handle is not yet set in the
     */
    handleCheck = __sync_or_and_fetch(&transport->handleCheck, NULL);
    if (handleCheck != handle) {
        mqttProtoObj_t * goodPobj = pobj;
        /* The pobj is good if it is within the transport object */
        if ((void *)pobj < (void *)transport || (void *)pobj > (void *)((char *)transport)+2000)
            goodPobj = NULL;
        /* The name is good if it is within the transport object.  This is not true for a very long transport name */
        if (name < (const char *)transport || name > ((const char *)transport)+2000)
            name = "*BAD*";
        if (goodPobj && handleCheck == CLIENT_STATE_IN_PROGRESS) {
            /*
             * There is a windows between when the engine records the clientState and when
             * protocol fills in pobj->handle.  If this happens, use the handle we
             * got from the engine in this steal notification.
             */
            transport->handleCheck = pobj->handle = handle;
            __sync_synchronize();
        } else {
            TRACE(1, "ClientID steal victim invalid: connect=%u name=%s transport=%p pobj=%p handle=%p handleCheck=%p\n",
                    transport->index, name, transport, pobj, handle, handleCheck);
#ifdef DEBUG
            /* In DEBUG mode, core when this happens */
            ism_common_shutdown(1);
#endif
            LOG(ERROR, Server, 2999, "%u%s%p%p%p%p", "ClientID steal victim invalid: Connect={0} Client={1} transport={2} pobj={3} handle={4} handleCheck={5}",
                transport->index, name, transport, pobj, handle, handleCheck);
            return;
        }
    }

    if (__sync_bool_compare_and_swap(&pobj->startState, MQTT_IN_PROGRESS, MQTT_STOLEN)) {
        /* Connect in progress, do cleanup when complete */
        TRACE(7, "ClientID stolen while connect in progress, assume self steal: connect=%u client=%s state=%u\n",
                transport->index, transport->name, pobj->startState);
        return;
    } else {
        /* Set state to "Disconnected" */
        pobj->startState = MQTT_DISCONNECTED;
    }
    if (rc == ISMRC_ResumedClientState)
        rc = ISMRC_ClientIDReused;
    ism_common_getErrorString(rc, xbuf, sizeof xbuf);
    TRACE(9, "ism_mqtt_replySteal: Close victim connection: connect=%u client=%s inprogress=%d\n", transport->index, transport->name, pobj->inprogress);
    transport->close(transport, rc, !rc, xbuf);
}


/*
 * Remove unreleased id handle from the list of incomplete QoS 2 publications
 */
void ism_mqtt_replyRemoveUnreleasedDeliveryId(int32_t rc, void * handle, void * vaction) {
    mqtt_act_t * act = (mqtt_act_t *) vaction;
    ism_transport_t * transport = act->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    int pending = 1;
    msgIdLock(pobj);
    if (pobj->incompmsgids)
        ism_msgid_delMsgIdInfo(pobj->incompmsgids, (uint16_t)act->msgid, &pending);
    msgIdUnlock(pobj);

    char buf[100];

    if (!rc || rc == MQTTRC_PacketIDNotFound) {
        int sendlen = 2;
        buf[16] = (char) (act->msgid >> 8);
        buf[17] = (char) act->msgid;
        if (pobj->mqtt_version >= 5) {
            if (rc) {
                buf[18] = rc;
                sendlen = 3;
            } else {
                if (pobj->flow_max > 0) {
                    pthread_spin_lock(&transport->lock);
                    if (pobj->flow_count > 0) {      /* Flow control decrement in case of error */
                        pobj->flow_count--;
                    }
                    pthread_spin_unlock(&transport->lock);
                }
            }
        }

        transport->send(transport, buf + 16, sendlen, MT_PUBCOMP << 4, SFLAG_SYNC | SFLAG_FRAMESPACE);

        TRACE(8, "PUBCOMP was sent for for: connect=%u client=%s id=%d \n",
            transport->index, transport->name, act->msgid);
    } else {
        /* Engine reported a problem */
        ism_common_setError(rc);
        TRACE(5, "Failed to process PUBREL request for: connect=%u client=%s id=%d pending=%d inProgress=%d\n",
            transport->index, transport->name, act->msgid, pending, pobj->inprogress);
        transport->close(transport, ISMRC_MsgidRemove, 0, "Unable to remove message ID at server");
    }

    /* If close is in progress, proceed with it instead of this close */
    if (pending) {
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_replyRemoveUnreleasedDeliveryId: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
        if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, pending) < 0)) { /* BEAM suppression: constant condition */
            TRACE(9, "replyClose from connect=%u\n", transport->index);
            ism_mqtt_replyCloseClient(transport);
        }
    }
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
        case MQTTRC_PacketIDInUse:                   /* No explicit ISMRC for this */
            ret = MQTTRC_PacketIDInUse;      break;

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
        else
            return 0x80;   /* Only proxy protocol */
    }
}

/*
 * Map an MQTT Return code to an ISMRC
 */
static int mapToIsmRc(int mqttrc, int version) {
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
    }
    return ISMRC_Error;
}

/*
 * Put out the MQTTv5 CONNACK properties
 */
static void putConnackProperties(concat_alloc_t * rbuf, ism_transport_t * transport, const char * reason) {
    char xbuf [4096];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;


    if (pobj->generated_cid)
        ism_common_putMqttPropString(&buf, MPI_ClientID, g_ctx5, transport->name, -1);
    if (reason) {
        ism_common_putMqttPropString(&buf, MPI_Reason, g_ctx5, reason, -1);
    } else {
        int maxReceive = 0;
        switch (pobj->msgRate) {
        case EXPECTEDMSGRATE_LOW:       maxReceive = 10;    break;
        case EXPECTEDMSGRATE_UNDEFINED:
        case EXPECTEDMSGRATE_DEFAULT:   maxReceive = 256;   break;
        case EXPECTEDMSGRATE_HIGH:      maxReceive = 2048;  break;
        }
        if (maxReceive)
            ism_common_putMqttPropField(&buf, MPI_MaxReceive,    g_ctx5, maxReceive);
        pobj->flow_max = maxReceive;
        ism_common_putMqttPropField(&buf, MPI_MaxTopicAlias, g_ctx5, pobj->topicalias_count_in);
        ism_common_putMqttPropField(&buf, MPI_MaxPacketSize, g_ctx5, transport->listener->maxMsgSize);
        if (pobj->send_keepalive)
            ism_common_putMqttPropField(&buf, MPI_KeepAlive, g_ctx5, pobj->keepAlive);
        if (pobj->modifiedExpire)
            ism_common_putMqttPropField(&buf, MPI_SessionExpire, g_ctx5, pobj->modifiedExpire);

    }

    if (pobj->requestReason == 9) {
        int nouserprop = buf.used;
        ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "ServerName",    -1, ism_common_getServerName(), -1);
        ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "ServerProduct", -1, "" IMA_PRODUCTNAME_FULL "", -1);
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
 * Get the subscriptions for a connection
 */
void getSubs(concat_alloc_t * buf, ism_transport_t * transport, int non_filtered) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    int i;
    for (i=0; i<pobj->prodcons_alloc; i++) {
        mqtt_prodcons_t * pc = pobj->prodcons[i];
        int len;
        int subid;
        /* If the slot is not in use, ignore it */
        if (!pc || pc->closed || pc->closing || !pc->topic || !*pc->topic)
            continue;
        /* If we only want non-filtered subs, ignore filtered subs */
        if (non_filtered && (pc->flags&CONFLAG_UseFilter))
            continue;
        len = strlen(pc->topic);
        ism_protocol_ensureBuffer(buf, len+7);
        buf->buf[buf->used++] = (char)(len>>8);
        buf->buf[buf->used++] = (char)len;
        ism_json_putBytes(buf, pc->topic);
        subid = endian_int32(pc->subID);
        memcpy(buf->buf+buf->used, &subid, 4);                /* SubID, but first byte is always zero */
        buf->buf[buf->used+4] = (pc->flags&0xFC) | pc->qos;   /* Send original flags */
        buf->used += 5;
    }
}


/*
 * Process an MQTT error.
 *
 * If this is one of the connection returns, send the CONNACK, otherwise
 * close the connection.  This is also used to sent the CONACK for a
 * good connection.
 */
void ism_mqtt_error(mqtt_act_t * act) {
    ism_transport_t * transport = act->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

    char response[2048];
    char xbuf[2048];
    concat_alloc_t rbuf = {response, sizeof response};

    if (act->rc) {
        TRACE(3, "MQTT error: rc=%s (%d) command=%s connect=%u client=%s inprogress=%d\n",
                getMQTTErrorString(act->rc, xbuf, sizeof xbuf), act->rc, mqttCommand(act->command), transport->index,
                transport->name, pobj->inprogress);
    }
#ifdef DEBUG
    else {
        TRACE(8, "---------ism_mqtt_error action=%s connect=%u(%p) inprogress=%d\n",
        mqttCommand(act->command), transport->index, transport, pobj->inprogress);
    }
#endif

    /*
     * If this is a connect or a failed self steal return a CONNACK
     */
    if (act->command == MT_CONNECT) {
        if (act->handle) {
            ism_engine_releaseMessage(act->handle);
            act->handle = NULL;
        }
        if (act->topic) {
            ism_common_free(ism_memory_protocol_misc,act->topic);
            act->topic = NULL;
        }
        if (transport->pobj->mqtt_proxy || !act->rc || pobj->mqtt_version >= 5 ||
                (act->rc > CRC_MinConnectRc && act->rc <= CRC_MaxConnectRc)) {
            if (act->rc == 0) {
                if (!__sync_bool_compare_and_swap(&pobj->startState, MQTT_IN_PROGRESS, MQTT_CONNECTED)) {
                    /* Client-state was stolen, do cleanup */
                    pobj->startState = MQTT_DISCONNECTED;
                    act->rc = ISMRC_ClientIDReused;
                }
            } else {
                if (act->rc != ISMRC_ClientIDInUse)
                    pobj->startState = MQTT_DISCONNECTED;
            }

            /*
             * In V3.1.1 and above bit 0 of the first byte of the CONNACK indicates if the session is resumed
             * and this is a cleansession=0. In V3.1 this byte is always 0.
             */
            if (act->resumed_session) {
                pobj->session_existed = 1;
            } else {
                if (act->inherit_durable) {
                    pobj->cleansession = 1;
                }
            }
            if (pobj->mqtt_version > 3 && act->resumed_session && pobj->cleansession == 0)
                response[16] = 0x01;    /* Set session present flag */
            else
                response[16] = 0x00;    /* Reset session present flag */
            response[17] = (char)mapToMqttRc(act->rc, pobj->mqtt_version, CPOI_CONNACK);

            if (transport->closestate[3] == 0)
                transport->closestate[3] = (char)act->rc;
            rbuf.used = 18;

            if (act->rc) {
                switch(act->rc) {
                case CRC_InvalidVersion: act->rc = ISMRC_ProtocolVersion;     break;
                case CRC_BadIdentifier:  act->rc = ISMRC_BadClientID;         break;
                case CRC_BadUser:
                case CRC_NotAuthorized:  act->rc = ISMRC_NotAuthorized;       break;
                }
                if (ism_common_getLastError() == 0)
                    ism_common_getErrorString(act->rc, xbuf, sizeof xbuf);
                else
                    ism_common_formatLastError(xbuf, sizeof xbuf);
                pobj->send_disconnect = 0;
            } else {
                pobj->send_disconnect = 1;
            }
            if (pobj->mqtt_version >= 5) {
                putConnackProperties(&rbuf, transport, act->rc ? xbuf : NULL);
            }

            if (transport->pobj->mqtt_proxy) {
                ism_common_putExtensionValue(&rbuf, EXIV_ProxyProtLevel, 6);   /* Current proxy protocol support level */
                if (act->rc) {

                    ism_common_putExtensionValue(&rbuf, EXIV_ServerRC, (int16_t)act->rc);
                    ism_common_putExtensionString(&rbuf, EXIV_ReasonString, xbuf);

                    /* Send monitoring failed if requested */
                    ism_iot_failedMsg(transport, act->rc, xbuf);
                } else {
                    /* Send monitoring connect if requested */
                    ism_iot_connectMsg(transport);
                    if (pobj->serverInfo) {
                        ism_common_putExtensionString(&rbuf, EXIV_ServerProduct, "" IMA_PRODUCTNAME_FULL "");
                        ism_common_putExtensionString(&rbuf, EXIV_ServerVersion, ism_common_getVersion());
                        ism_common_putExtensionString(&rbuf, EXIV_ServerDetails, ism_common_getBuildLabel());
                        ism_common_putExtensionString(&rbuf, EXIV_ServerName, ism_common_getServerName());
                    }
                    if (pobj->sendSubs && pobj->session_existed ) {
                        concat_alloc_t sbuf = {xbuf, sizeof xbuf};
                        getSubs(&sbuf, transport, 0);
                        if (sbuf.used) {
                            ism_common_putExtensionByteArray(&rbuf, EXIV_Subscriptions, sbuf.buf, sbuf.used);
                            ism_common_freeAllocBuffer(&sbuf);
                        }

                    }
                }
                transport->send(transport, rbuf.buf + 16, rbuf.used - 16, MT_CONNACK << 4,
                        SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
            } else {
                transport->send(transport, rbuf.buf+16, rbuf.used-16, MT_CONNACK << 4,
                        SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
            }

            /*
             * Close the connection in the case of invalid CONNECT
             * If close is in progress, proceed with it instead of this close
             */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_error: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
            if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
                TRACE(9, "replyClose from connect=%u\n", transport->index);
                ism_mqtt_replyCloseClient(transport);
                return;
            }
            if (act->rc) {
                /*
                 * For V3.1 only close for InvalidVersion.  In other cases allow the client to close.
                 * For V3.1.1 close the connection for all non-zero return codes on CONNACK
                 */
                if (act->rc == ISMRC_ProtocolVersion || act->transport->pobj->mqtt_version > 3) {
                    __sync_add_and_fetch(&transport->listener->stats->bad_connect_count, 1);
                    ism_common_getErrorString(act->rc, xbuf, sizeof xbuf);
                    transport->close(transport, act->rc, 0, xbuf);
                } else {
                    transport->closestate[3] = (uint8_t)act->rc;
                    __sync_add_and_fetch(&transport->listener->stats->bad_connect_count, 1);
                }
            }
        }
    } else {
        ism_common_formatLastError(xbuf, sizeof xbuf);

        /* If close is in progress, proceed with it instead of this close */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress ism_mqtt_error: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
        if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
            TRACE(9, "replyClose from connect=%u\n", transport->index);
            ism_mqtt_replyCloseClient(transport);
        } else {
            /*
             * Map the MQTT CONNACK code to an ISMRC as a closing reason code
             */
            int xrc = act->rc;
            switch (act->rc) {
            case CRC_InvalidVersion:   xrc = ISMRC_ProtocolVersion;            break;
            case CRC_BadIdentifier:    xrc = ISMRC_BadClientID;                break;
            case CRC_NotAvailable:     xrc = ISMRC_ServerNotAvailable;         break;
            case CRC_BadUser:
            case CRC_NotAuthorized:    xrc = ISMRC_ConnectNotAuthorized;       break;
            }
            if (ism_common_getLastError() == 0)
                ism_common_getErrorString(act->rc, xbuf, sizeof xbuf);
            else
                ism_common_formatLastError(xbuf, sizeof xbuf);
            transport->close(transport, xrc, 0, xbuf);
        }
    }
}


/*
 * We always use two message areas, one for properties and the other for payload
 */
static ismMessageAreaType_t MsgAreas[2] = {
    ismMESSAGE_AREA_PROPERTIES,
    ismMESSAGE_AREA_PAYLOAD
};


/*
 * Replace a topic alias
 */
static void replaceTopicAlias(const char * * alias, int which, const char * topic, int topic_len) {
    char * newalias;
    if (alias[which]) {
        if (strlen(alias[which]) == topic_len && !memcmp(alias[which], topic, topic_len))
            return;
        ism_common_free(ism_memory_protocol_misc,(char *)alias[which]);
        alias[which] = NULL;
    }
    newalias = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,85),topic_len + 1);
    memcpy(newalias, topic, topic_len);
    newalias[topic_len] = 0;
    alias[which] = newalias;
}


/*
 * Publish a message
 */
void ism_mqtt_doPublish(ism_transport_t * transport, mqttMsg_t * mmsg) {
    int bodylen;
    int propslen = 0;
    mqtt_act_t act = { 0 };
    char xbuf[1000];
    concat_alloc_t buf = { 0 };
    size_t areasize[2];
    void * areaptr[2];
    ismEngine_MessageHandle_t msgh;
    ismMessageHeader_t hdr;
    int rc;
    int chklen;
    ismEngine_UnreleasedHandle_t phUnrel = NULL;
    ismEngine_UnreleasedHandle_t * ptrPhUnrel = NULL;
    int msgId = 0;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

    /*
     * Check the session
     */
    if (!pobj->session_handle) {
        mmsg->rc = ISMRC_Closed;
        transport->listener->stats->count[transport->tid].lost_msg++;
        ism_common_setError(mmsg->rc);
        TRACE(4, "The session is closed on a PUBLISH: connect=%u client=%s\n", transport->index, transport->name);
        return;
    }

    /*
     * Check the topic
     */
    mmsg->v5_shared = transport->pobj->v5_shared;

    /* Implement topic alias */
    if (mmsg->topic_alias) {
        if (mmsg->topic_alias > pobj->topicalias_count_in) {
            mmsg->rc = ISMRC_TopicAliasNotValid;
            ism_common_setError(ISMRC_TopicAliasNotValid);
            return;
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

    if (mmsg->topic == NULL || checkString(mmsg, mmsg->topic, mmsg->topic_len, CHK_PUBTOPIC)) {
        int msgcode = 2104;

        if (mmsg->topic == NULL) {
            mmsg->rc = ISMRC_BadTopic;
            ism_common_setError(mmsg->rc);
        }
        char * errbuf = alloca(4096);
        if (errbuf)
            ism_common_formatLastError(errbuf, 4096);
        else
            errbuf = "";

        transport->listener->stats->count[transport->tid].lost_msg++;
        TRACE(4, "The topic name is not valid on a PUBLISH: connect=%u client=%s reason%s\n",
                transport->index, transport->name, errbuf);

        if (!previouslyLogged(pobj, msgcode)) {
            LOG(WARN, Connection, 2104, "%u%-s%-s%-s%-s",
                    "The client request to publish a message to the server failed because the destination is not valid: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3} Reason={4}.", transport->index,
                    transport->name, transport->protocol, transport->listener->name, errbuf);
        }
        ism_mqtt_replyPublishNAK(transport, mmsg);
        return;
    }

    /*
     * Do not allow a user publish on a $ topic.
     * In V3.1.1 change the rule to not allow a user publish on any topic starting with $
     */
    if (mmsg->topic[0] == '$') {
        int msgcode = 2110;

        transport->listener->stats->count[transport->tid].lost_msg++;
        ism_common_setError(ISMRC_BadSysTopic);
        TRACE(4, "Publishing on system topic is not allowed: connect=%u client=%s\n", transport->index, transport->name);

        if (!previouslyLogged(pobj, msgcode)) {
            LOG(WARN, Connection, 2110, "%u%-s%-s%-s",
                    "The client request to publish a message to the server failed because the topic is a system topic: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.", transport->index, transport->name, transport->protocol, transport->listener->name);
        }
        mmsg->rc = ISMRC_BadSysTopic;
        ism_mqtt_replyPublishNAK(transport, mmsg);
        return;
    }

    /*
     * Check max message length for this endpoint
     */
    if (pobj->mqtt_version < 5 && transport->listener->maxMsgSize) {
        chklen = mmsg->payload_len;
        if (mmsg->topic_len > 200)
            chklen += mmsg->topic_len - 200;
        if (chklen > transport->listener->maxMsgSize) {
            int msgcode = 2102;

            transport->listener->stats->count[transport->tid].lost_msg++;
            ism_common_setError(ISMRC_MsgTooBig);

            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2102, "%u%-s%-s%-s%d",
                        "The client request to publish a message to the server failed because it is too large for the endpoint: "
                        "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3} MsgSize={4}.", transport->index, transport->name, transport->protocol, transport->listener->name, chklen);
            }

            mmsg->rc = ISMRC_MsgTooBig;
            ism_mqtt_replyPublishNAK(transport, mmsg);
            return;
        }
    }
    TRACE(8, "ism_mqtt_doPublish: connect=%u client=%s id=%d inProgress=%d\n",
                transport->index, transport->clientID, mmsg->msgid, pobj->inprogress);

    /*
     * Check that the payload is valid UTF-8
     */
    if (g_verifyPayload && mmsg->msgfmt==1 && mmsg->payload_len) {
        int pcount = ism_common_validUTF8(mmsg->payload, mmsg->payload_len);
        if (pcount < 0) {
            mmsg->rc = ISMRC_InvalidPayload;
            ism_common_setErrorData(mmsg->rc, "%s%s", "Publish", "PayloadFormat");
            int msgcode = 2112;
            transport->listener->stats->count[transport->tid].lost_msg++;

            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2110, "%u%-s%-s%-s",
                    "The client request to publish a message to the server failed because the payload does not conform to the payload format: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.",
                    transport->index, transport->name, transport->protocol, transport->listener->name);
            }
            ism_mqtt_replyPublishNAK(transport, mmsg);
            return;
        }
    }

    /*
     * Handle messages which must not be thrown away
     */
    if (mmsg->qos > 0) {
        if (chkmsgID(mmsg)) {
            TRACE(5, "The MQTT message ID is not valid on a PUBLISH: connect=%u client=%s msgid=%u\n",
                    transport->index, transport->name, mmsg->msgid);
            return;
        }

        /*
         * Duplicates are OK for QoS 1. But for QoS 2, check if this is a duplicate.
         * If so, re-send the acknowledgment. Otherwise, assume this is a new message.
         */
        if (mmsg->qos == 2) {
            ism_msgid_info_t * pMsgInfo;
            msgIdLock(pobj);
            pMsgInfo = ism_msgid_getMsgIdInfo(pobj->incompmsgids, (uint16_t)mmsg->msgid);
            if (pMsgInfo) {
                if (!mmsg->dup) {
                    if (pobj->mqtt_version >= 5) {
                        msgIdUnlock(pobj);
                        mmsg->rc = MQTTRC_PacketIDInUse;
                        ism_mqtt_replyPublishNAK(transport, mmsg);
                        return;
                    }
                    /* Revert behavior for MQTT v3 (to fix customer issue). So for MQTT v3 duplicate message is silently dropped */
                    if (pobj->mqtt_version == 4) {
                        /*
                         * If the dup flag is not set and we get a message with an existing message ID, assume the client
                         * cache is incorrect and force a disconnect.  It is expected that then the client reconnects, it
                         * will set the duplicate flag and the message will be discarded.
                         *
                         * NOTE ABOUT RISK: This approach is effectively the same as discarding a second message with the same
                         * message ID as was done in 13b.  If the second message is, in fact, a "new" message, it will be discarded
                         * without the client's knowledge and only the first message with this ID will be delivered to consumers.
                         */
                    	msgIdUnlock(pobj);
                        TRACE(5, "MQTT PUBLISH received using existing message ID and without the duplicate flag set. Assuming client error and forcing disconnect: connect=%u client=%s\n",
                                transport->index, transport->name);
                        LOG(WARN, Connection, 2111, "%u%-s%-s%-s%u",
                                  "The client was disconnected because an attempt was made to publish a message with a message ID that is in use. "
                                  "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3} Message ID={4}.",
                                  transport->index, transport->name, transport->protocol, transport->listener->name, mmsg->msgid);
                        mmsg->rc = ISMRC_BadClientData;
                        ism_common_setError(mmsg->rc);
                        return;
                    }
                }
                if (pMsgInfo->state == ISM_MQTT_PUBLISH) {
                    pMsgInfo->pending++;
                    msgIdUnlock(pobj);
                    TRACE(5, "MQTT PUBLISH received using existing message ID while publish is still in process: connect=%u client=%s\n",
                            transport->index, transport->name);
                    return;
                }
                msgIdUnlock(pobj);
                /* This message is already in PUBREC state, simply resend PUBREC */
                TRACE(5, "Duplicate MQTT PUBLISH received, PUBREC will be re-sent: connect=%u client=%s msgid=%u\n",
                        transport->index, transport->name, mmsg->msgid);

                act.qos = mmsg->qos;
                act.transport = transport;
                act.msgid = mmsg->msgid;
                act.isMsgid = mmsg->isMsgid;
                ism_mqtt_replyPublishSimple(&act, 1);
                return;
            }
            ism_msgid_addMsgIdInfo(pobj->incompmsgids, 0, (uint16_t)mmsg->msgid, ISM_MQTT_PUBLISH);
            msgIdUnlock(pobj);
        }
        /* Enfoce flow control */
        if (pobj->flow_max) {
            pthread_spin_lock(&transport->lock);
            if (pobj->flow_count >= pobj->flow_max) {
                mmsg->rc = ISMRC_ReceiveMaxExceeded;
                ism_common_setErrorData(mmsg->rc, "%u", pobj->flow_max);
                pthread_spin_unlock(&transport->lock);
                return;
            }
            pthread_spin_unlock(&transport->lock);
        }
    }

    /*
     * Construct the properties with topic
     */
    buf.buf = xbuf;
    buf.len = sizeof(xbuf);

    ism_protocol_putNameIndex(&buf, ID_Topic);
    ism_protocol_putStringLenValue(&buf, mmsg->topic, mmsg->topic_len);
    if (mmsg->retain) {
        ism_protocol_putNameIndex(&buf, ID_ServerTime);
        ism_protocol_putLongValue(&buf, ism_common_currentTimeNanos());
        ism_protocol_putNameIndex(&buf, ID_OriginServer);
        ism_protocol_putStringValue(&buf, ism_common_getServerUID());
    }
    if (mmsg->has_props) {
        ism_common_scanExtension(mmsg->ext, mmsg->ext_len, extPublishProps, &buf);
    }
    if (pobj->mqtt_version >= 5 && mmsg->v5prop_len) {
        ism_common_checkMqttPropFields((char *)mmsg->v5props, mmsg->v5prop_len,
            g_ctx5, CPOI_C_PUBLISH, mpropPublishProps, &buf);
    }
    propslen = buf.used;

    /*
     * Set up the action
     */
    bodylen = mmsg->payload_len;
    act.transport = transport;
    act.msgid = mmsg->msgid;
    act.isMsgid = mmsg->isMsgid;
    act.qos = mmsg->qos;
    act.unset_retained = (!bodylen && mmsg->retain);

    /*
     * Set up the header
     */
    memset(&hdr, 0, sizeof hdr);
    hdr.Persistence = mmsg->qos >= 1 || mmsg->retain;
    hdr.Reliability = mmsg->qos;
    hdr.Priority = 4;
    hdr.RedeliveryCount = 0;
    hdr.Expiry = 0;
    if (mmsg->isMsgExpire) {
        hdr.Expiry = ism_common_getExpire(((ism_time_t)mmsg->msgExpire * 1000000000) +
                ism_common_currentTimeNanos());
    }
    hdr.Flags = (mmsg->retain) ? ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN : 0;
    hdr.MessageType = mmsg->msgtype;

    /* Set the properties and body area */
    areasize[0] = propslen;
    areaptr[0] = propslen ? buf.buf : NULL;
    areasize[1] = bodylen;
    areaptr[1] = bodylen ? (char *) mmsg->payload : NULL;

    /*
     * Create the message
     */
    if (pobj->session_handle) {                         /* BEAM suppression: constant condition */
        rc = ism_engine_createMessage(&hdr, 2, MsgAreas, areasize, areaptr, &msgh);
    } else {
        mmsg->rc = ISMRC_Closed;
        transport->listener->stats->count[transport->tid].lost_msg++;
        ism_common_setError(mmsg->rc);
        TRACE(5, "The session is closed on a PUBLISH: connect=%u client=%s\n", transport->index, transport->name);
        if (buf.inheap)
            ism_common_freeAllocBuffer(&buf);
        return;
    }
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);

    if (rc) {
        /* Engine reported error when creating message, set an error and ACK */
        ism_mqtt_replyPublish(rc, NULL, &act);
        return;
    }

    char * xtopic = alloca(mmsg->topic_len + 1);
    memcpy(xtopic, mmsg->topic, mmsg->topic_len);
    xtopic[mmsg->topic_len] = 0;

    transport->read_msg++;
    transport->listener->stats->count[transport->tid].read_msg++;

    /* For QoS 2, create an unreleased message handle */
    if (mmsg->qos == 2) {
        ptrPhUnrel = &phUnrel;
        msgId = mmsg->msgid;
    }

    if (LIKELY(pobj->session_handle != NULL)) {         /* BEAM suppression: constant condition */
        ismEngine_ProducerHandle_t producerh = NULL;
        if (LIKELY(mmsg->topic_len <= 1024)) {  /* Do not attempt to cache topic names unless they are within the 1024 byte limit. */
            mqtt_pubobj_t * publisher = &pobj->publisher[0];
            if (LIKELY(publisher->lasttopic != NULL)) {
                if ((publisher->lasttopic_len != mmsg->topic_len) ||
                    (memcmp(publisher->lasttopic, mmsg->topic, mmsg->topic_len))) {
                    /* Topics are not the same - destroy old producer */
                    mqttDestroyProducer(publisher);
                    /* Save new topic name */
                    if (publisher->lasttopic_alloc > mmsg->topic_len) {
                        memcpy(publisher->lasttopic, mmsg->topic, mmsg->topic_len);
                        publisher->lasttopic_len = mmsg->topic_len;
                    } else {
                        int new_size = (mmsg->topic_len < 1024) ? (mmsg->topic_len + 8) & 0xFFFFFFF8 : 1024;
                        char * p = ism_common_realloc(ISM_MEM_PROBE(ism_memory_protocol_misc,86),publisher->lasttopic, new_size);
                        if (p) {
                            publisher->lasttopic = p;
                            publisher->lasttopic_alloc = new_size;
                            memcpy(publisher->lasttopic, mmsg->topic, mmsg->topic_len);
                            publisher->lasttopic_len = mmsg->topic_len;
                        }
                    }
                }
            } else {
                int new_size = (mmsg->topic_len < 1024) ? (mmsg->topic_len + 8) & 0xFFFFFFF8 : 1024;
                publisher->lasttopic = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,87),new_size);
                if (publisher->lasttopic != NULL) {
                    publisher->lasttopic_alloc = new_size;
                    memcpy(publisher->lasttopic, mmsg->topic, mmsg->topic_len);
                    publisher->lasttopic_len = mmsg->topic_len;
                }
            }
            if ((publisher->producerh == NULL) && (publisher->lasttopic_len != 0)) {
                publisher->count++;
                if (publisher->count > mqttMaxNonprodCount) {
                    rc = ism_engine_createProducer(pobj->session_handle,ismDESTINATION_TOPIC, xtopic, &publisher->producerh,NULL, 0, NULL);
                    if (rc != ISMRC_OK) {
                        ism_common_setError(rc);
                        rc = ISMRC_OK;
                    }
                }
            }
            producerh = publisher->producerh;
        }

        /*
         * Unset the retained message
         */
        if (UNLIKELY(act.unset_retained)) {
            uint32_t unsetOption = transport->pobj->mqtt_bridge ? ismENGINE_UNSET_RETAINED_OPTION_NONE : ismENGINE_UNSET_RETAINED_OPTION_PUBLISH;
            if (producerh) {
                /* With producer */
                rc = ism_engine_unsetRetainedMessageWithDeliveryId(pobj->session_handle,
                        producerh, unsetOption, NULL, msgh, msgId, ptrPhUnrel,
                        &act, sizeof act, ism_mqtt_replyPublish);
            } else {
                /* Without producer */
                rc = ism_engine_unsetRetainedMessageWithDeliveryIdOnDestination(pobj->session_handle,
                        ismDESTINATION_TOPIC, xtopic, unsetOption,
                        NULL, msgh, msgId, ptrPhUnrel, &act, sizeof act, ism_mqtt_replyPublish);
            }
        }

        /*
         * Send the message
         */
        else {
            if (pobj->mqtt_version >= 5 && act.qos > 0 && pobj->flow_max) {
                pthread_spin_lock(&transport->lock);
                pobj->flow_count++;
                pthread_spin_unlock(&transport->lock);
            }
            if (producerh) {
                rc = ism_engine_putMessageWithDeliveryId(pobj->session_handle, producerh, NULL, msgh, msgId, ptrPhUnrel, &act, sizeof act, ism_mqtt_replyPublish); /* BEAM suppression: uninitialized */
            } else {
                rc = ism_engine_putMessageWithDeliveryIdOnDestination(pobj->session_handle,
                      ismDESTINATION_TOPIC, xtopic, NULL, msgh, msgId, ptrPhUnrel, &act,     /* BEAM suppression: uninitialized */
                      sizeof act, ism_mqtt_replyPublish);
            }
        }
        if (rc != ISMRC_AsyncCompletion) {
            ism_mqtt_replyPublish(rc, phUnrel, &act);
        }
    } else {
        /* The session is closed */
        mmsg->rc = ISMRC_Closed;
        transport->listener->stats->count[transport->tid].lost_msg++;
        ism_common_setError(mmsg->rc);
        TRACE(5, "The session is closed on a PUBLISH: connect=%u client=%s\n", transport->index, transport->name);
        return;
    }
}


/*
 * When we receive a PUBACK, release the engine message ID reference
 */
void ism_mqtt_doPubAck(ism_transport_t * transport, uint16_t msgid, int mqttrc) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    ismEngine_DeliveryHandle_t msgh = 0;
    msgIdLock(pobj);
    msgh = ism_msgid_delMsgIdInfo(pobj->msgids, msgid, NULL);
    msgIdUnlock(pobj);

    if (LIKELY(msgh != 0)) {
        int rc = 0;
        if (LIKELY(pobj->session_handle > 0)) {
            mqtt_act_t act = { 0 };
            act.transport = transport;
            act.msgid = msgid;
            rc = ism_engine_confirmMessageDelivery(pobj->session_handle,
                    NULL, msgh, ismENGINE_CONFIRM_OPTION_CONSUMED,
                    &act, sizeof(act), ism_mqtt_replyPubAck);
        }
        if (UNLIKELY(rc == ISMRC_AsyncCompletion)) {
            return;
        }
    }

    /* If close is in progress, proceed with it */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_doPubAck: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
    }
}


/*
 * When we receive a PUBREC, change the message status to RECEIVED and
 * respond with PUBREL.
 */
void ism_mqtt_doPubRec(ism_transport_t * transport, uint16_t msgid, int mqttrc) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t*) transport->pobj;
    ismEngine_DeliveryHandle_t msgh = 0;
    ism_msgid_info_t * pMsgInfo = NULL;
    msgIdLock(pobj);
    pMsgInfo = ism_msgid_getMsgIdInfo(pobj->msgids, msgid);
    char buf [32];
    if (pMsgInfo) {
        if (mqttrc >= 128 && pobj->mqtt_version >= 5) {
            msgh = pMsgInfo->handle;
            ism_msgid_delMsgIdInfo(pobj->msgids, msgid, NULL);
            msgIdUnlock(pobj);
            if (msgh && pobj->session_handle) {
                xUNUSED int zrc = ism_engine_confirmMessageDelivery(pobj->session_handle,
                        NULL, msgh, ismENGINE_CONFIRM_OPTION_RECEIVED, NULL, 0, NULL);
            }
            TRACE(8, "ism_mqtt_doPubRec:  delete packetID connect=%u clientID=%s msgid=%u \n",
                    transport->index, transport->clientID, (uint32_t)msgid);
            msgh = 0;
        } else {
            switch(pMsgInfo->state) {
            case ISM_MQTT_PUBLISH:
                pMsgInfo->state = ISM_MQTT_PUBREC;
                pMsgInfo->pending = 1;
                msgh = pMsgInfo->handle;
                msgIdUnlock(pobj);
                TRACE(8, "ism_mqtt_doPubRec:  connect=%u client=%s msgid=%u \n",
                        transport->index, transport->name, (uint32_t)msgid);
                break;
            case ISM_MQTT_PUBREC:
                pMsgInfo->pending++;
                msgIdUnlock(pobj);
                TRACE(4, "ism_mqtt_doPubRec: DUPLICATE PUBREC while PUBREC is in process connect=%u client=%s msgid=%u pending=%d inprogress=%d\n",
                        transport->index, transport->name, (uint32_t)msgid, pMsgInfo->pending, pobj->inprogress);
                break;
            default:
            	msgIdUnlock(pobj);
                TRACE(5, "ism_mqtt_doPubRec - Duplicate PUBREC: connect=%u client=%s msgid=%d inprogress=%d\n",
                        transport->index, transport->name, msgid, pobj->inprogress);
                buf[16] = (char) (msgid >> 8);
                buf[17] = (char) msgid;
                transport->send(transport, buf + 16, 2, MT_PUBREL << 4 | 0x02, SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
            }
        }
    } else {
    	msgIdUnlock(pobj);
        if (pobj->mqtt_version >= 5) {
            TRACE(5, "ism_mqtt_doPubRec - PacketID not found: connect=%u client=%s msgid=%d inprogress=%d\n",
                    transport->index, transport->name, msgid, pobj->inprogress);
            buf[16] = (char) (msgid >> 8);
            buf[17] = (char) msgid;
            buf[18] = (char) MQTTRC_PacketIDNotFound;
            transport->send(transport, buf + 16, 3, MT_PUBREL << 4 | 0x02, SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
        }
    }

    if (LIKELY(msgh != 0)) {
        int rc = 0;
        mqtt_act_t act = { 0 };
        act.transport = transport;
        act.msgid = msgid;
        if (LIKELY(pobj->session_handle != NULL)) {
            rc = ism_engine_confirmMessageDelivery(pobj->session_handle,
                    NULL, msgh, ismENGINE_CONFIRM_OPTION_RECEIVED,
                    &act, sizeof(act), ism_mqtt_replyPubRec);
        }
        if (LIKELY(rc != ISMRC_AsyncCompletion))
            ism_mqtt_replyPubRec(rc, NULL, &act);
        return;
    }
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress ism_mqtt_doPubRec: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
    }
    return;
}


/*
 * When we receive a PUBCOMP, change the state to CONSUMED and
 * release the engine message ID reference
 */
void ism_mqtt_doPubComp(ism_transport_t * transport, uint16_t msgid, int mqttrc) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    ismEngine_DeliveryHandle_t msgh = 0;
    msgIdLock(pobj);
    msgh = ism_msgid_delMsgIdInfo(pobj->msgids, msgid, NULL);
    TRACE(8, "ism_mqtt_doPubComp: connect=%u client=%s msgid=%u\n",
            transport->index, transport->name, (uint32_t)msgid );
    msgIdUnlock(pobj);
    if (LIKELY(msgh != 0)) {
        int rc = 0;
        if (LIKELY(pobj->session_handle != NULL)) {
            mqtt_act_t act = { 0 };
            act.msgid = msgid;
            act.transport = transport;
            TRACE(8, "Got PUBCOMP for: connect=%u client=%s msgid=%d\n",
                    transport->index, transport->name, msgid);
            rc = ism_engine_confirmMessageDelivery(pobj->session_handle,
                    NULL, msgh, ismENGINE_CONFIRM_OPTION_CONSUMED,
                    &act, sizeof(act), ism_mqtt_replyPubAck);    /* ism_mqtt_replyPubAck frees the msgid  */
        }

        if (UNLIKELY(rc == ISMRC_AsyncCompletion))
            return;
    }

    /* If close is in progress, proceed with it */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_doPubComp: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
    }
}


/*
 * In case the engine call for PUBACK goes async.
 */
void ism_mqtt_replyPubAck(int32_t rc, void * handle, void * vaction) {
    mqtt_act_t * act = (mqtt_act_t *) vaction;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) act->transport->pobj;
    ism_transport_t * transport = act->transport;
    /* If close is in progress, proceed with it */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_replyPubAck: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
    }
}

/*
 * Respond to PUBREC message. Send PUBREL to the subscriber.
 */
void ism_mqtt_replyPubRec(int32_t rc, void * handle, void * vaction) {
    mqtt_act_t * act = (mqtt_act_t *) vaction;
    ism_transport_t * transport = act->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    int pending = 1;
    ism_msgid_info_t * pMsgInfo = NULL;

    msgIdLock(pobj);
    pMsgInfo = ism_msgid_getMsgIdInfo(pobj->msgids, act->msgid);
    if (pMsgInfo) {
        pMsgInfo->state = ISM_MQTT_PUBREL;
        pending = pMsgInfo->pending;
        pMsgInfo->pending = 1;
    }
    msgIdUnlock(pobj);
    if (LIKELY(pMsgInfo != NULL)) {
        char buf[100];
        int i;
        TRACE(8, "Send PUBREL: connect=%u client=%s msgid=%d pending=%d inprogress=%d\n",
                transport->index, transport->name, act->msgid, pending, pobj->inprogress);
        buf[16] = (char) (act->msgid >> 8);
        buf[17] = (char) act->msgid;
        for(i = 0; i < pending; i++) {
            transport->send(transport, buf + 16, 2, MT_PUBREL << 4 | 0x02, SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
        }
    }

    /* If close is in progress, proceed with it */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_replyPubRec: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
    }
}

/*
 * Return a hex value
 */
static int hexval(char ch) {
    if (ch >= '0' && ch <= '9')
        return ch-'0';
    if (ch >= 'A' && ch <= 'F')
        return ch-'A'+10;
    if (ch >= 'a' && ch <= 'f')
        return ch-'a'+10;
    return -1;
}

/*
 * Subscribe to a topic.
 * In binary mqtt multiple topics can be specified.
 */
void ism_mqtt_doSubscribe(ism_transport_t * transport, mqttMsg_t * mmsg) {
    char * pp;
    int left;
    int count = 1;
    int i;
    int rc;
    char xbuf [1200];

    if (chkmsgID(mmsg)) {
        TRACE(5, "The MQTT message ID is not valid on a SUBSCRIBE: connect=%u client=%s\n", transport->index, transport->name);
        return;
    }
    mmsg->v5_shared = transport->pobj->v5_shared;

    /*
     * Get a count of the number of subscriptions
     */
    count = 0;
    pp = (char *) mmsg->payload;
    left = mmsg->payload_len;
    while (left > 1) {
        int len = (uint16_t)BIGINT16(pp);
        if (len == 0) {
            mmsg->rc = ISMRC_ProtocolError;
            break;
        }
        if (len + 3 > left)
            break;
        if (mmsg->command == MTX_SUBSCRIBEX) {
            if (len + 4 > left)
                break;
            len += BIGINT16(pp+len+2) + 4;
            if (len > left)
                break;
        } else {
            len += 3;
        }
        left -= len;
        pp += len;
        count++;
    }

    /*
     * Check that we are at the end of the payload and have at least one topic
     */
    if (mmsg->rc || left != 0 || count == 0) {
        if (!mmsg->rc)
            mmsg->rc = left ? ISMRC_BadClientData : ISMRC_ProtocolError;
        ism_common_setErrorData(mmsg->rc, "%s", "SUBSCRIBE");
    }

    /*
     * Fill in the job object
     */
    if (!mmsg->rc) {
        subjob_t * job = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,88),1, sizeof(subjob_t));
        job->count = count;
        job->transport = transport;
        job->msgid = mmsg->msgid;
        if (count > 8) {
            job->freebuf = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,89),2, count);
            job->qos = job->freebuf;
            job->ack = job->freebuf + count;
        } else {
            job->qos = job->qosbuf;
            job->ack = job->ackbuf;
        }
        memset(job->ack, 0, count);

        job->topic = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,90),job->count, sizeof(char *));
        job->subid = mmsg->subID;

        pp = (char *) mmsg->payload;
        left = mmsg->payload_len;
        for (i = 0; i < count; i++) {
            int    badmask;
            int    next;
            char * op;
            int len = BIGINT16(pp);
            mmsg->rc = 0;
            if (transport->pobj->mqtt_proxy) {
                if (len == 5 && !memcmp(pp+2, "$_$", 3)) {
                    int v1 = hexval(pp[5]);
                    int v2 = hexval(pp[6]);
                    if (v1 >= 0 && v2 >=0) {
                        job->ack[i] = (char)((v1<<4) + v2);
                    }
                }
            }
            rc = checkString(mmsg, pp + 2, len, CHK_SUBTOPIC);    /* Validate the topic */
            if (rc) {
                ism_common_formatLastError(xbuf, sizeof xbuf);
                TRACE(5, "Subscribe error: connect=%u client=%s count=%d of %d rc=%d reason=\"%s\"\n",
                        transport->index, transport->name, i+1, count, rc, xbuf);
                if ((rc != ISMRC_BadTopic && rc != ISMRC_BadSysTopic && rc != ISMRC_ShareMismatch && rc != ISMRC_TooManyProdCons) ||
                        transport->pobj->mqtt_version < 5) {
                    break;   /* mmsg->rc set by checkString */
                }
                job->ack[i] = transport->pobj->mqtt_version >= 5 ? MQTTRC_TopicFilterInvalid : 0x80;
                mmsg->rc = 0;
            }
            op = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,91),len + 1);
            if (!op) {
                mmsg->rc = ISMRC_AllocateError;
                ism_common_setError(mmsg->rc);
                break;
            }
            memcpy(op, pp + 2, len);
            op[len] = 0;
            job->topic[i] = op;

            if (mmsg->command == MTX_SUBSCRIBEX) {
                const char * filter;
                int          filterlen;
                int extlen = BIGINT16(pp+len+2);
                const char * ext = pp+len+4;
                next = len+4+extlen;
                job->qos[i] = ism_common_getExtensionValue(ext, extlen, EXIV_SubscribeOpt, 0);
                filter = ism_common_getExtensionString(ext, extlen, EXIV_ACLFilter, &filterlen);
                if (filter && filterlen>0) {
                    if (!job->filter)
                        job->filter = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,92),job->count, sizeof(char *));
                    char * fp = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,93),filterlen+1);
                    memcpy(fp, filter, filterlen);
                    fp[filterlen] = 0;
                    job->filter[i] = fp;
                }
            } else {
                job->qos[i] = pp[len + 2];
                next = len+3;
            }
            switch (transport->pobj->mqtt_version) {
            default:
            case 3:  badmask = 0;      break;
            case 4:  badmask = 0xFC;   break;
            case 5:  badmask = 0xC0;   break;
            }
            if (job->qos[i]&badmask) {
                mmsg->rc = ISMRC_BadClientData;
                ism_common_setErrorData(mmsg->rc, "%s", "SubscribeQoS");
                break;
            }
            if ((job->qos[i]&3) == 3) {
                mmsg->rc = ISMRC_InvalidQoS;
                ism_common_setErrorData(mmsg->rc, "%s", "SubscribeQoS");
                break;
            }
            if (badmask && (job->qos[i]&0x30) == 0x30) {
                mmsg->rc = ISMRC_ProtocolError;
                ism_common_setErrorData(mmsg->rc, "%s", "RetainHandling");
                break;
            }
            pp += next;
        }
        job->which = -1;

        /*
         * Do the actual work in the reply method
         */
        if (mmsg->rc == 0)
            ism_mqtt_replySubscribe(0, NULL, &job);
        else {
            if (job->freebuf)
                ism_common_free(ism_memory_protocol_misc,job->freebuf);
            for (i = 0; i < count; i++) {
                if (job->topic[i])
                    ism_common_free(ism_memory_protocol_misc,job->topic[i]);
                if (job->filter) {
                    if (job->filter[i])
                        ism_common_free(ism_memory_protocol_misc,job->filter[i]);
                }
            }
            ism_common_free(ism_memory_protocol_misc,job->topic);
            if (job->filter)
                ism_common_free(ism_memory_protocol_misc,job->filter);
            ism_common_free(ism_memory_protocol_misc,job);
        }
    }
}


/*
 * Check if error returned by the engine can/should be ignored for subscription.
 */
static inline bool rcIgnoredOnSubscribe(int rc, subjob_t * job) {
    int msgcode;
    ism_transport_t * transport;
    mqttProtoObj_t * pobj;
    if (rc == ISMRC_OK)
        return true;
    transport = job->transport;

    TRACE(5, "Unable to create subscription rc=%d topic=%s connect=%u client=%s\n", rc,
            job->topic[job->which], transport->index, transport->name);

    pobj = (mqttProtoObj_t *) transport->pobj;
    if (rc >= ISMRC_UserNotAuthorized && rc <= ISMRC_TopCLNotAuthorized)
        rc = ISMRC_NotAuthorized;

    switch (rc) {
    case ISMRC_NotAuthorized:
        msgcode = 2202;
        if (!previouslyLogged(pobj, msgcode)) {
            LOG(WARN, Connection, 2202, "%u%-s%-s%-s%-s",
                    "Unable to create a consumer due to an authorization failure: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3} UserID={4}.",
                    transport->index, transport->name, transport->protocol, transport->listener->name,
                    transport->userid ? transport->userid : "");
        }
        return true;

    case ISMRC_DestNotValid:
        msgcode = 2204;
        if (!previouslyLogged(pobj, msgcode)) {
            LOG(WARN, Connection, 2204, "%u%-s%-s%-s",
                    "Unable to create a consumer because the topic name includes more than 32 levels: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.", transport->index, transport->name,
                    transport->protocol, transport->listener->name);
        }
        return true;

    case ISMRC_BadSysTopic:
        msgcode = 2206;
        if (!previouslyLogged(pobj, msgcode)) {
            LOG(WARN, Connection, 2206, "%u%-s%-s%-s",
                    "A durable subscription is not allowed on a system topic: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.", transport->index, transport->name,
                    transport->protocol, transport->listener->name);
        }
        return true;

    case ISMRC_ShareMismatch:
        /*
         * The log message for this case is written when the problem is detected
         * so that we have access to the topic name and selector in the existing
         * subscription.
         */
        return true;
    case ISMRC_AllocateError:
    case ISMRC_ServerCapacity:
    case ISMRC_TooManyConsumers:
        if (pobj->mqtt_version > 3) { /* In V3.1.1 return failure code and do not force a disconnect */
            /*
             * The log message for this case is written when the problem is detected
             */
            return true;
        }
    default:                            /* BEAM suppression: fall through */
        msgcode = 2201000 + rc;
        if (!previouslyLogged(pobj, msgcode)) {
            char xerrbuf[4096];
            if (ism_common_getLastError() != rc)
                ism_common_setError(rc);
            ism_common_formatLastError(xerrbuf, 4096);
            LOG(WARN, Connection, 2201, "%u%-s%-s%-s%-s%s%d", "A message consumer could not be created: "
                    "ConnectionID={0} ClientID={1} Endpoint={2} UserID={3} Protocol={4} Error={5} RC={6}.",
                    transport->index, transport->name, transport->listener->name, transport->userid ? transport->userid : "",
                    transport->protocol, xerrbuf, rc);
        }
        break;
    }
    /*
     * Return good for auth errors in MQTTv3.1, but return error in MQTTv3.1.1.
     */
    if (rc == ISMRC_NotAuthorized || rc ==ISMRC_TooManyProdCons || rc == ISMRC_ExistingSubscription)
        return true;
    return false;
}


/*
 * Process a subscription error
 */
static void ism_mqtt_onSubscribeError(int32_t rc, subjob_t * job) {
    mqtt_act_t act = { 0 };
    ism_transport_t * transport = job->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    ism_common_setError(rc);

    /* Durable subscription could have been created in the engine, destroy it */
    if (pobj->handle && job->topic[job->which]) {
        TRACE(7, "Destroy subscription on failure: connect=%u client=%s rc=%d topic=%s\n",
                transport->index, transport->name, rc, job->topic[job->which]);
        xUNUSED int zrc = ism_engine_destroySubscription(pobj->handle,                                  /* BEAM suppression: no effect */
                job->topic[job->which], pobj->handle, NULL, 0, NULL);
    }
    if (job->consumer) {
        pobj->prodcons[job->consumer->which]->handle = NULL;
        pobj->prodcons[job->consumer->which]->closed = 1;
        job->consumer = NULL;
    }

    /* Error that require disconnect returned by the engine */
    act.rc = rc;
    act.transport = transport;
    ism_mqtt_error(&act);           /* Assume reason is already traced */
    return;
}


/*
 * Parse a topic to separate subscription name and topic filter
 */
static const char * parseTopic(const char * fulltopic, const char * * subname, int * subname_len, int * subopt) {
    int   len;
    char * pos;
    const char * topic = fulltopic;
    int   subopt_local = 0;
    if (*topic == '$') {
        len = strlen(topic);
        if (len >= 8 && !memcmp(topic, "$select/", 8)) {
            int skip = parseSelect(len, topic, &subopt_local);
            if (skip > 0) {
                len -= skip;
                topic += skip;
            }
        }
        if (len > 22 && !memcmp(topic, "$SharedSubscription/", 20)) {
            pos = strchr(topic+20, '/');
            if (pos) {
                *subname = topic+20;
                *subname_len = pos-(topic+20);
                if (subopt)
                    *subopt = subopt_local;
                return pos+1;
            }
        } else if (len > 9 && !memcmp(topic, "$share/", 7)) {
            pos = strchr(topic+7, '/');
            if (pos) {
                *subname = topic+6;
                *subname_len = len-6;    /* Include filter */
                if (subopt)
                    *subopt = subopt_local;
                return pos+1;
            }
        }
    }
    *subname = NULL;
    *subname_len = 0;
    if (subopt)
        *subopt = subopt_local;
    return topic;
}

#define SPLIT_QOS (ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY | ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY)

/*
 * Implement an MQTT subscribe
 */
static void mqttSubscribe(ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName, const char * oldTopicName,
        void * xproperties, size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes, uint32_t consumerCount, void * vjob) {
    int   rc;
    subjob_t * job = (subjob_t *) vjob;
    ism_transport_t * transport = job->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    ism_prop_t * props = (ism_prop_t *)xproperties;
    ism_field_t oldSelector = {0};
    ism_field_t newSelector = {0};
    int          rulelen = 0;
    const char * newTopicName = job->topic[job->which];

    TRACE(6, "mqttSubscribe: connect=%u subname=%s newSubname=%s oldTopic=%s newTopic=%s subopt=%x\n",
            transport->index, pSubName, job->subname, oldTopicName, newTopicName, pSubAttributes->subOptions);
    if (!strcmp(job->subname, pSubName)) {
        ismRule_t *  rule = NULL;
        if (job->filter && job->filter[job->which]) {
            int crc = ism_common_compileSelectRuleOpt(&rule, &rulelen, job->filter[job->which], SELOPT_Internal);
            if (crc) {
                newSelector.type = VT_Null;
            } else {
                newSelector.type  = VT_ByteArray;
                newSelector.val.s = (char *)rule;
                newSelector.len   = rulelen;
            }
        }
        ism_common_getProperty(props, "Selector", &oldSelector);
        int selectorMismatch = (oldSelector.type != newSelector.type) ||
                           ((pSubAttributes->subOptions & SPLIT_QOS) != (job->subopt & SPLIT_QOS)) ||
                           (oldSelector.type == VT_ByteArray &&
                           (oldSelector.len != newSelector.len || memcmp(oldSelector.val.s, newSelector.val.s, oldSelector.len)));
        if (rule) {
            ism_common_freeSelectRule(rule);
            rule = NULL;
        }

        if (!strcmp(oldTopicName, newTopicName) && !selectorMismatch) {
            ismEngine_ClientStateHandle_t kind;
            /* Supply mode which specifies existing durable / shared options and our QoS */
            ismEngine_SubscriptionAttributes_t subAttrs = { pSubAttributes->subOptions & (ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                            ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY) };
            subAttrs.subOptions |= (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + job->consumer->qos) |
                                    ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT | ismENGINE_SUBSCRIPTION_OPTION_SHARED;
            if (job->subname[0] == '/') {
                kind = client_SharedM;
                if (pobj->cleansession)
                    subAttrs.subOptions &= ~ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
                else
                    subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
                /* sanity check MIXED_DURABILITY option is set */
                assert((subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY) != 0);
            } else {
                kind = subAttrs.subOptions&ismENGINE_SUBSCRIPTION_OPTION_DURABLE ? client_Shared : client_SharedND;
            }
            subAttrs.subOptions |= pSubAttributes->subOptions & SPLIT_QOS;
            subAttrs.subId = job->subid;

            TRACE(7, "Reuse subscription: connect=%u client=%s topic=%s subopt=%x subid=%d\n",
                    transport->index, transport->name, job->subname, subAttrs.subOptions, subAttrs.subId);
            rc = ism_engine_reuseSubscription(pobj->handle, job->subname, &subAttrs, kind);
            if (!rc) {
                job->sub_found = SUB_Found;
                return;
            }
            if (kind != client_SharedND && rc == ISMRC_NotAuthorized) {
                /* If reauthorization fails, destroy the subscription */
                xUNUSED int zrc = ism_engine_destroySubscription(pobj->handle,                          /* BEAM suppression: no effect */
                    job->subname, kind, NULL, 0, NULL);
            }
        } else {
            job->sub_found = SUB_Error;
                char * oldSelectorStr = oldSelector.type != VT_Null ? "selector" : "null";
            TRACE(6, "mqttSubscribe with different parameters: connect=%u client=%s subscription=%s"
                " oldTopic=%s newTopic=%s oldSelector=%s newSelector=null\n",
                transport->index, transport->name, pSubName,
                oldTopicName, newTopicName, oldSelectorStr);
            /*
             * Do not limit this log message with previously logged because the same
             * client may send more than one subscription request that has this error.
             */
            LOG(WARN, Connection, 2290, "%-s%-s%-s%-s%u%-s%-s%-s%-s",
                "The topic filter and selector values for a shared subscription must match the values in the existing subscription. "
                    "Subscription={8} Existing topic filter={0} New topic filter={1} Existing selector={2} "
                "ConnectionID={4} ClientID={5} Protocol={6} Endpoint={7}.", oldTopicName, newTopicName, oldSelectorStr, "",
                transport->index, transport->name, transport->protocol, transport->listener->name, pSubName);
        }
    }
    return;
}


/*
 * Restart a subscribe.
 * This is used to keep the stack depth lower.
 */
int ism_mqtt_restartSubscribe(ism_transport_t * transport, void * callbackParam, uint64_t param) {
    subjob_t * job = (subjob_t *) callbackParam;
    int32_t    rc  = job->xrc;
    ism_mqtt_replySubscribe(rc, (void *)(uintptr_t *)param, &job);
    return 0;
}


/*
 * Subscripbe to a nondurable nonshared subscription
 */
static int subscribeNondurableNonshared(subjob_t * job, const char * topicfilter, ism_prop_t * cprops) {
    int rc = 0;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    mqtt_prodcons_t * consumer = job->consumer;
    ism_transport_t * transport = job->transport;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + consumer->qos;
    subAttrs.subOptions |= job->subopt;
    subAttrs.subId = consumer->subID;
    uint32_t consumerOption = ismENGINE_CONSUMER_OPTION_NONE;

    if (consumer->qos)
        consumerOption |= ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;

    if (cprops) {
        TRACEL(7, transport->trclevel, "Subscribe nondurable with props: connect=%u client=%s topic=%s subopt=%x\n",
                transport->index, transport->name, consumer->topic, subAttrs.subOptions);
        rc = ism_engine_createSubscription(transport->pobj->handle,
                  topicfilter, cprops, ismDESTINATION_TOPIC, topicfilter, &subAttrs,
                  transport->pobj->handle, &job, sizeof(subjob_t * *), ism_mqtt_replyCreateSubscription);
        if (rc != ISMRC_AsyncCompletion) {
            ism_mqtt_replyCreateSubscription(rc, transport->pobj->handle, &job);
            rc = ISMRC_AsyncCompletion;   /* We have already scheduled the restart */
        }
    } else {
        TRACEL(7, transport->trclevel, "Subscribe nondurable: connect=%u client=%s topic=%s subopt=%x\n",
                transport->index, transport->name, consumer->topic, subAttrs.subOptions);
        rc = ism_engine_createConsumer(transport->pobj->session_handle,
                ismDESTINATION_TOPIC, topicfilter, &subAttrs, NULL,
                consumer, sizeof(*consumer), ism_mqtt_replyMessage, NULL,
                consumerOption, &job->consumerh, &job,
                sizeof(subjob_t * *), ism_mqtt_replySubscribe);
    }
    return rc;
}

/*
 * Subscribe to a nondurable shared subscription
 */
int subscribeNondurableShared(subjob_t * job, const char * subname, const char * topicfilter, ism_prop_t * cprops) {
    int rc = 0;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    mqtt_prodcons_t * consumer = job->consumer;
    ism_transport_t * transport = job->transport;
    uint32_t consumerOption = ismENGINE_CONSUMER_OPTION_NONE;
    ismEngine_ClientStateHandle_t kind;

    job->sub_found = SUB_NotFound;
    kind = subname[0] == '/' ? client_SharedM : client_SharedND;
    job->topic[job->which] = (char *)topicfilter;
    rc = ism_engine_listSubscriptions(kind, (char *)subname, job, mqttSubscribe);
    job->topic[job->which] = NULL;
    if (job->sub_found == SUB_Error) {
        rc = ISMRC_ShareMismatch;
        ism_common_setError(rc);
    }
    if (consumer->flags & MSUBOPT_NoLocal) {
        rc = ISMRC_ProtocolError;
        ism_common_setErrorData(rc, "%s", "NoLocal");
    }
    if (!rc) {
        /*
         * Create the subscription
         */
        if (job->sub_found == SUB_NotFound) {
            subAttrs.subOptions = job->subopt;
            subAttrs.subOptions |= (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + consumer->qos) |
                                   ismENGINE_SUBSCRIPTION_OPTION_SHARED | ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT;
            if (subname[0] == '/') {
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY;
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS;
            }
            if (((consumer->flags&MSUBOPT_RetainHandling)>>4) == MSUBOPT_SendNoRetained)
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS;

            subAttrs.subId = consumer->subID;

            TRACEL(7, transport->trclevel, "Subscribe non-durable shared: connect=%u client=%s topic=%s subopt=%x\n",
                    transport->index, transport->name, consumer->topic, subAttrs.subOptions);

            rc = ism_engine_createSubscription(transport->pobj->handle,
                     subname, cprops, ismDESTINATION_TOPIC, topicfilter, &subAttrs,
                     kind, &job, sizeof(subjob_t * *), ism_mqtt_replyCreateSubscription);

            if (rc == ISMRC_ExistingSubscription) {
                job->topic[job->which] = (char *)topicfilter;
                rc = ism_engine_listSubscriptions(kind, (char *)subname, job, mqttSubscribe);
                job->topic[job->which] = NULL;
                if (job->sub_found == SUB_Error) {
                    rc = ISMRC_ShareMismatch;
                    ism_common_setError(rc);
                }
            }
        }
        /* We can continue and create consumer only if subscription was created successfully. */
        if (!rc) {
            /*
             * Create the consumer
             */
            if (consumer->qos)
                consumerOption |= ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;
            rc = ism_engine_createConsumer(transport->pobj->session_handle,
                   ismDESTINATION_SUBSCRIPTION, subname, NULL, NULL,
                   consumer, sizeof(*consumer), ism_mqtt_replyMessage, NULL,
                   consumerOption, &job->consumerh, &job,
                   sizeof(subjob_t * *), (job->sub_found == SUB_NotFound)? ism_mqtt_replySubscribe : ism_mqtt_replyReSubscribe);
        } else {
            job->consumerh = NULL;
        }
    }
    return rc;
}

/*
 * Subscribe to a durable shared
 */
int subscribeDurableShared(subjob_t * job, const char * subname, const char * topicfilter, ism_prop_t * cprops) {
    int rc = 0;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    mqtt_prodcons_t * consumer = job->consumer;
    ism_transport_t * transport = job->transport;
    uint32_t consumerOption = ismENGINE_CONSUMER_OPTION_NONE;
    ismEngine_ClientStateHandle_t kind;

    kind = subname[0] == '/' ? client_SharedM : client_Shared;
    job->sub_found = SUB_NotFound;
    job->topic[job->which] = (char *)topicfilter;
    rc = ism_engine_listSubscriptions(kind, (char *)subname, job, mqttSubscribe);
    job->topic[job->which] = NULL;
    if (job->sub_found == SUB_Error) {
        rc = ISMRC_ShareMismatch;
        ism_common_setErrorData(rc, "%s", "NoLocal");
    }
    if (consumer->flags & MSUBOPT_NoLocal) {
        rc = ISMRC_ProtocolError;
        ism_common_setErrorData(rc, "%s", "NoLocal");
    }
    if (!rc) {
         /*
          * Create the subscription
          */
        if (job->sub_found == SUB_NotFound) {
            subAttrs.subOptions = (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + consumer->qos) |
                                   ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                   ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT |
                                   ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
            subAttrs.subOptions |= job->subopt;
            subAttrs.subId = consumer->subID;
            if (subname[0] == '/') {
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY;
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS;
            }
            if (((consumer->flags&MSUBOPT_RetainHandling)>>4) == MSUBOPT_SendNoRetained)
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS;


            TRACEL(7, transport->trclevel, "Subscribe durable shared: connect=%u client=%s topic=%s subopt=%x\n",
                    transport->index, transport->name, consumer->topic, subAttrs.subOptions);
            rc = ism_engine_createSubscription(transport->pobj->handle,
                      subname, cprops, ismDESTINATION_TOPIC, topicfilter, &subAttrs,
                      kind, &job, sizeof(subjob_t * *), ism_mqtt_replyCreateSubscription);

            if (rc == ISMRC_ExistingSubscription) {
                job->topic[job->which] = (char *)topicfilter;
                rc = ism_engine_listSubscriptions(kind, (char *)subname, job, mqttSubscribe);
                job->topic[job->which] = NULL;
                if (job->sub_found == SUB_Error) {
                    rc = ISMRC_ShareMismatch;
                    ism_common_setError(rc);
                }
            }
        }

        /* We can continue and create consumer only if subscription was created successfully. */
        if (!rc) {
            /*
             * Create the consumer
             */
            if (consumer->qos)
                consumerOption |= ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;
            rc = ism_engine_createConsumer(transport->pobj->session_handle,
                    ismDESTINATION_SUBSCRIPTION, subname,
                    NULL, kind, consumer, sizeof(*consumer), ism_mqtt_replyMessage,
                    NULL, consumerOption, &job->consumerh,
                    &job, sizeof(subjob_t * *), (job->sub_found == SUB_NotFound) ? ism_mqtt_replySubscribe : ism_mqtt_replyReSubscribe);

            if (rc != 0 && rc != ISMRC_AsyncCompletion) {
                consumer->closed = 1;
                xUNUSED int zrc = ism_engine_destroySubscription(transport->pobj->handle, subname, kind, NULL, 0, NULL);
            }
        } else {
            job->consumerh = NULL;
        }
    }
    return rc;
}


/*
 * Subscribe to durable nonshared
 */
static int subscribeDurableNonshared(subjob_t * job, const char * topicfilter, ism_prop_t * cprops) {
    int rc = 0;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    mqtt_prodcons_t * consumer = job->consumer;
    ism_transport_t * transport = job->transport;
    uint32_t consumerOption = ismENGINE_CONSUMER_OPTION_NONE;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                              (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE+consumer->qos);
    subAttrs.subOptions |= job->subopt;
    subAttrs.subId = consumer->subID;

    TRACEL(7, transport->trclevel, "Subscribe durable: connect=%u client=%s topic=%s subopt=%x\n",
            transport->index, transport->name, topicfilter, subAttrs.subOptions);
    rc = ism_engine_createSubscription(transport->pobj->handle,
             topicfilter, cprops, ismDESTINATION_TOPIC, topicfilter, &subAttrs,
             transport->pobj->handle, &job, sizeof(subjob_t * *), ism_mqtt_replyCreateSubscription);


    /*
     * We can continue and create consumer only if subscription was created successfully.
     */
    if (!rc) {
        if (consumer->qos) {
            consumerOption |= ismENGINE_CONSUMER_OPTION_ACK |
                ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;
        }
        rc = ism_engine_createConsumer(transport->pobj->session_handle,
                ismDESTINATION_SUBSCRIPTION, topicfilter,
                NULL, NULL,
                consumer, sizeof(*consumer), ism_mqtt_replyMessage,
                NULL, consumerOption, &job->consumerh,
                &job, sizeof(subjob_t * *), ism_mqtt_replySubscribe);
    } else {
        job->consumerh = NULL;
    }
    return rc;
}

/*
 * Check for a mismatch between subscription ID in the consumer and job objects
 */
static int suboptsDiffer(mqtt_prodcons_t * consumer, subjob_t * job) {
    if (consumer->subID != job->subid)
        return 1;
    if ((consumer->flags & MSUBOPT_AsPublished) !=
             (job->flags & MSUBOPT_AsPublished))
        return 1;
    return 0;
}

/*
 * Check if we need to destroy and recreate th
 */
static int needResubscribe(mqtt_prodcons_t * consumer, subjob_t * job, const char * subname) {
    if (subname && *subname != '/')
        return 0;
    if (consumer->qos != (job->flags & 0x03))
        return 1;
    if (!subname && (consumer->flags & MSUBOPT_NoLocal) != (job->flags & MSUBOPT_NoLocal))
        return 1;
    return 0;
}


/*
 * Create the engine properties based on the subscription options
 */
static ism_prop_t * suboptProps(subjob_t * job) {
    ism_prop_t * cprops = ism_common_newProperties(2);
    ism_field_t f;
    int subopt = 0;
    if (job->subid) {
        f.type = VT_Integer;
        f.val.i = job->subid;
        ism_common_setProperty(cprops, "SubId", &f);
    }
    if (job->flags & MSUBOPT_NoLocal)
        subopt |= ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL;
    if (job->flags & MSUBOPT_AsPublished)
        subopt |= ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED;
    if (subopt) {
        f.type = VT_Integer;
        f.val.i = subopt;
        ism_common_setProperty(cprops, "SubOptions", &f);
    }
    return cprops;
}


/*
 * Find an existing subscription
 *
 * We do not allow the QoS to be modified as the engine uses different implementation
 * classes depending on QoS.  Thus if the QoS is modified we delete and recreate the
 * subscription possibly losing messages.
 *
 * The subscriptionID, NoLocal, andRetainAsPublished can be modified for an existing
 * subscription.  This is handled somewhat differntly based on whether this is a shared
 * subscription or not.
 *
 * @return A return code
 * 0 = Exact match found if consumerh is not NULL, or no match found if it is NULL
 * ISMRC_AsyncCompletion = replySubscription will be re-scheduled asynchronously
 * other = Error found
 */
static int findExistingSubscription(subjob_t * job) {
    const char * subname = NULL;
    int   subname_len;
    int   subopt;
    ism_transport_t * transport = job->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    mqtt_prodcons_t * consumer = NULL;
    ism_prop_t * cprops = NULL;
    int i;
    int rc = 0;

    job->consumer = NULL;

    for (i = 1; i < pobj->prodcons_alloc; i++) {
        consumer = pobj->prodcons[i];
        if (consumer && (consumer->closed == 0) && (consumer->handle != 0) && consumer->topic
                && !strcmp(consumer->topic, job->topic[job->which])) {

            const char * topicfilter = parseTopic(consumer->topic, &subname, &subname_len, &subopt);
            job->consumer = consumer;
            job->flags = job->qos[job->which];

            /*
             * If the change to the subscription is not allowed by the engine,
             * delete the subscription and resubscribe.  Note that doing this
             * violates the MQTT spec, but in many years nobody has complained.
             */
            if (needResubscribe(consumer, job, subname)) {
                TRACEL(5, transport->trclevel, "Modifying subscription which will lose messages in the durable queue: topic=%s qos=%x wasqos=%x connect=%u client=%s\n",
                        job->topic[job->which], job->qos[job->which], consumer->qos, transport->index, transport->name);
                /*
                 * ism_mqtt_deleteSubscription will eventually make a recursive call
                 * to ism_mqtt_replySubscribe.
                 */
                ism_mqtt_deleteSubscription(0, NULL, &job);
                return ISMRC_AsyncCompletion;
            }

            /*
             * If this is a shared subscription and either the subID or RetainAsPublished is changed,
             * call the engine to update the subscription.
             */
            if (subname) {
                uint32_t subopts = 0;
                TRACE(7, "Shared subscription already exists: topic=%s connect=%u client=%s subopt=%x\n",
                        consumer->topic, transport->index, transport->name, subopt);
                /* If subId or suboptions changed */

                if (!pobj->cleansession && suboptsDiffer(consumer, job)) {
                    ismEngine_ClientStateHandle_t kind;
                    subopts |= (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + consumer->qos) |
                                ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT |
                                ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
                    if (subname[0] == '/') {
                        subopts |= ismENGINE_SUBSCRIPTION_OPTION_SHARED_MIXED_DURABILITY;
                        subopts |= ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS;
                        kind = client_SharedM;
                    } else {
                        kind = client_Shared;
                    }
                    if (((job->flags&MSUBOPT_RetainHandling)>>4) == MSUBOPT_SendNoRetained)
                        subopts |= ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS;
                    subAttrs.subId = job->subid;
                    subAttrs.subOptions = subopts;

                    char * subnamez = alloca(subname_len + 1);
                    memcpy(subnamez, subname, subname_len);
                    subnamez[subname_len] = 0;
                    TRACE(7, "Modify durable shared subscription: connect=%u client=%s topic=%s subid=%d ssubopt=%x\n",
                        transport->index, transport->name, subnamez, subAttrs.subId, subAttrs.subOptions);
                    rc = ism_engine_reuseSubscription(pobj->handle, subnamez, &subAttrs, kind);
                }
                consumer->subID = job->subid;
                consumer->flags = job->flags;
                break;
            }

            /*
             * For non-shared durable subscription if we change the subID or RetainAsPublished
             * call the engine to update the persistent subscripotion.
             */
            if (!pobj->cleansession && suboptsDiffer(consumer, job)) {
                cprops = suboptProps(job);
                rc = ism_engine_updateSubscription(pobj->handle, topicfilter, cprops, pobj->handle,
                         &job, sizeof(subjob_t * *), ism_mqtt_replySubscribe);
                if (cprops)
                    ism_common_freeProperties(cprops);
                if (rc == ISMRC_AsyncCompletion)
                    return rc;
            }

            /*
             * Send retained messages if requested
             */
            consumer->subID = job->subid;     /* Update subid */
            consumer->flags = job->qos[job->which]&0xFC;   /* Zero out CONFLAG_UseFilter and CONFLAG_RerunFilter */
            if (((consumer->flags&MSUBOPT_RetainHandling)>>4) == MSUBOPT_SendRetained && !pobj->mqtt_bridge) {
                TRACE(7, "Subscription already exists so send retained messages: topic=%s connect=%u client=%s qos=%d flags=%x subopt=%x subid=%d\n",
                        job->topic[job->which], transport->index, transport->name, consumer->qos, consumer->flags, subopt, consumer->subID);
                rc = ism_engine_republishRetainedMessages(pobj->session_handle, consumer->handle, &job,
                        sizeof(subjob_t * *), ism_mqtt_replySubscribe);
                /* If async completion, this will return and process the next entry in the subscribe list */
                if (rc == ISMRC_AsyncCompletion)
                    return rc;
            } else {
                TRACE(7, "Subscription exists but retained messages are not sent: topic=%s connect=%u client=%s qos=%d flags=%x subid=%d\n",
                        consumer->topic, transport->index, transport->name, consumer->qos, consumer->flags,  consumer->subID);
            }
            break;
        }
    }
    return rc;
}


/*
 * Reply to an subscribe
 * Much of the work of a subscribe is done here
 */
void ism_mqtt_replySubscribe(int32_t rc, void * handle, void * vaction) {
    subjob_t * job = *((subjob_t * *) vaction);
    ism_transport_t * transport = job->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    mqtt_prodcons_t * consumer = NULL;
    int          subopt = 0;
    int          subopt_share;
    const char * subname = NULL;
    const char * topicfilter;
    ismRule_t *  rule = NULL;
    int          rulelen = 0;
    ism_prop_t * cprops = NULL;
    ism_field_t field;
    int   subname_len;
    int id;
    int len;
    char xbuf[512];
    concat_alloc_t buf = {xbuf, sizeof xbuf};

    if (pobj->closed){
        TRACE(6, "replySubscribe: pobj is closed. rc=%d connect=%u\n", rc, transport->index);

        //Need to decrement the count since :
        //1. We incremented when submit to Engine.
        //2. when call ism_mqtt_restartSubscribe, we don't increment and only
        //decrement at the end when there is no more work for the Subscribe.

        /* If close is in progress, proceed with it */
#ifdef DEBUG_INPROGRESS
        TRACE(9, "Decrement inprogress in ism_mqtt_replySubscribe: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
        if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
           TRACE(9, "replyClose from connect=%u\n", transport->index);
           ism_mqtt_replyCloseClient(transport);
        }
        return;
    }
    TRACE(9, "replySubscribe rc=%d connect=%u handle=%p job=%p\n", rc, transport->index, handle, job);

    /*
     * Complete the subscribe
     */
    if (!rcIgnoredOnSubscribe(rc, job)) {
        ism_mqtt_onSubscribeError(rc,job);
        //We don't decrement the inprogress in this return, because
        //1. ism_mqtt_onSubscribeError will call down to  ism_mqtt_error
        //which will decrement the inprogress
        return;
    } else {
        while (job->which < job->count) {
            job->consumerh = NULL;

            /* If we now have a completed consumer object, set it in the transport object */
            if (job->consumer) {
                if (!rc) {
                    TRACEL(8, transport->trclevel, "Set consumer handle: connect=%u client=%s consumer=%d handle=%p\n",
                            transport->index, transport->name, job->consumer->which, handle);
                    pobj->prodcons[job->consumer->which]->handle = handle;
                    job->ack[job->which] = job->qos[job->which]&3; /* For now we set the result = request in all cases */
                } else {
                    pobj->prodcons[job->consumer->which]->handle = NULL;
                    pobj->prodcons[job->consumer->which]->closed = 1;
                    job->ack[job->which] = mapToMqttRc(rc, pobj->mqtt_version, CPOI_SUBACK);
                }
                job->consumer = NULL;
            }

            subopt = 0;
            if (++job->which < job->count) {
                /* Ignore any entries which alredy have a bad rc */
                if ((uint8_t)job->ack[job->which] >= 0x80)
                    continue;
                /*
                 * Check if subscribed to this topic already.
                 */
                rc = findExistingSubscription(job);
                if (!rc && job->consumer) {
                    handle = job->consumer->handle;
                    continue;    /* Go on to next topic in job */
                }

                if (!rc) {
                    TRACEL(8, transport->trclevel, "MQTT createConsumer: connect=%u client=%s topic=%s\n",
                            transport->index, transport->name, job->topic[job->which]);
                    if (__sync_add_and_fetch(&pobj->consumer_count, 1) > mqttMaxSubs) {
                        job->xrc = ISMRC_TooManyProdCons;
                        ism_transport_submitAsyncJobRequest(job->transport, ism_mqtt_restartSubscribe, job, 0);
                        __sync_add_and_fetch(&pobj->consumer_count, -1);
                        if (buf.inheap)
                            ism_common_freeAllocBuffer(&buf);
                       //We don't decrement the inprogress in this return, because
                       //1. ism_mqtt_restartSubscribe is a recursive call to replySubscribe again.
                       //2. We only want to decrement 1 since we only increment 1 when we submit to Engine with replySubscribe callback
                        return;
                    }

                    /* Process a filter */
                    if (job->filter && job->filter[job->which]) {
                        const char * thisfilter = job->filter[job->which];
                        rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, thisfilter, SELOPT_Internal);
                        if (rc) {
                            job->xrc = rc;
                            ism_transport_submitAsyncJobRequest(job->transport, ism_mqtt_restartSubscribe, job, 0);
                            __sync_add_and_fetch(&pobj->consumer_count, -1);
                            if (buf.inheap)
                                ism_common_freeAllocBuffer(&buf);
                            //We don't decrement the inprogress in this return, because
                          //1. ism_mqtt_restartSubscribe is a recursive call to replySubscribe again.
                          //2. We only want to decrement 1 since we only increment 1 when we submit to Engine with replySubscribe callback
                            return;
                        }
                        if (rulelen > 0) {
                            cprops = ism_common_newProperties(20);
                            field.type = VT_ByteArray;
                            field.val.s = (char *)rule;
                            field.len   = rulelen;
                            ism_common_setProperty(cprops, "Selector", &field);
                            subopt |= ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION;
                        }
                    }

                    /*
                     * Set up consumer object
                     */
                    id = findProdcons(transport);
                    consumer = pobj->prodcons[id];
                    memset(consumer, 0, sizeof(mqtt_prodcons_t));
                    consumer->transport = transport;
                    consumer->topic = job->topic[job->which];
                    job->topic[job->which] = NULL;
                    consumer->qos = job->qos[job->which]&3;
                    consumer->flags = job->qos[job->which]&0xFC;   /* Zero out CONFLAG_UseFilter and CONFLAG_RerunFilter */
                    consumer->subID = job->subid;
                    if (rule && rulelen)
                        consumer->flags |= CONFLAG_UseFilter;       /* Shared sub, not recovered */
                    consumer->which = id;
                    job->consumer = consumer;

                    /*
                     * Parse the topic to see if this is a shared sub
                     */
                    topicfilter = parseTopic(consumer->topic, &subname, &subname_len, &subopt_share);
                    subopt |= subopt_share;

                    if (subname) {
                        buf.used = 0;
                        ism_common_allocBufferCopyLen(&buf, subname, subname_len);
                        bputchar(&buf, 0);
                        subname = (const char *)buf.buf;
                    } else {
                        if ((consumer->flags&MSUBOPT_NoLocal) || transport->pobj->mqtt_bridge) {    /* For bridge, set nolocal */
                            subopt |= ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL;
                        }
                    }
                    ism_fwd_replaceString(&job->subname, subname);

                    /* Set RetainAsPublished */
                    if (consumer->flags&MSUBOPT_AsPublished) {
                        subopt |= ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED;
                    }
                    if (((consumer->flags&MSUBOPT_RetainHandling)>>4) == MSUBOPT_SendNoRetained)
                        subopt |= ismENGINE_SUBSCRIPTION_OPTION_NO_RETAINED_MSGS;
                    job->subopt = subopt;

                    /* Create a non-durable subscription */
                    if (pobj->cleansession) {
                        if (subname) {
                            rc = subscribeNondurableShared(job, subname, topicfilter, cprops);
                        } else {
                            rc = subscribeNondurableNonshared(job, topicfilter, cprops);
                        }
                    }

                    /* Create a durable subscription */
                    else {
                        if (consumer->topic[0]=='$' && !subname && !subopt) {
                            /* Cannot subscribe to a topic starting with $ */
                            rc = ISMRC_BadSysTopic;
                            ism_common_setError(rc);
                        } else {
                            if (subname) {
                                rc = subscribeDurableShared(job, subname, topicfilter, cprops);
                            } else {
                                rc = subscribeDurableNonshared(job, topicfilter, cprops);
                            }
                        }
                    }
                    if (cprops) {
                        ism_common_freeProperties(cprops);
                        cprops = NULL;
                        if (rulelen) {
                            rulelen = 0;
                            if (rule)
                                ism_common_freeSelectRule(rule);
                        }
                    }
                }

                if (rc == ISMRC_AsyncCompletion) {
                    if (buf.inheap)
                        ism_common_freeAllocBuffer(&buf);
                    //We don't decrement the inprogress in this return, because
                  //1. ism_mqtt_restartSubscribe is a recursive call to replySubscribe again.
                  //2. We only want to decrement 1 since we only increment 1 when we submit to Engine with replySubscribe callback
                    return;
                }
                if (!rcIgnoredOnSubscribe(rc, job)) {
                    job->xrc = rc;
                    ism_transport_submitAsyncJobRequest(job->transport, ism_mqtt_restartSubscribe, job, 0);
                    if (buf.inheap)
                        ism_common_freeAllocBuffer(&buf);
                    //We don't decrement the inprogress in this return, because
                  //1. ism_mqtt_restartSubscribe is a recursive call to replySubscribe again.
                  //2. We only want to decrement 1 since we only increment 1 when we submit to Engine with replySubscribe callback
                    return;
                }
                if (rc)
                    handle = NULL;
                else
                    handle = job->consumerh;
            }
        }

        /*
         * Send the subscribe reply
         */
        if (job->which >= job->count) {
            char * sbuf;
            char jbuf[86];
            int i = 0;
            int pos = 18;
            len = job->count + 2;
            TRACE(8, "MQTT subscribe: connect=%u client=%s count=%d\n", transport->index, transport->name, job->which);
            sbuf = (job->count < 60) ? jbuf : alloca(26 + job->count);
            sbuf[16] = (char) (job->msgid >> 8);
            sbuf[17] = (char) job->msgid;
            if (pobj->mqtt_version >= 5) {
                sbuf[18] = 0;
                pos++;
                len++;
            }
            memcpy(sbuf + pos, job->ack, job->count);
            transport->send(transport, sbuf + 16, len, MT_SUBACK << 4,
                    SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);

            /* Free the job object */
            for (i = 0; i < job->count; i++) {
                if (job->topic[i])
                    ism_common_free(ism_memory_protocol_misc,job->topic[i]);
            }
            if (job->freebuf)
                ism_common_free(ism_memory_protocol_misc,job->freebuf);
            if (job->topic)
                ism_common_free(ism_memory_protocol_misc,job->topic);
            if (job->subname)
                ism_common_free(ism_memory_protocol_misc,(char *)job->subname);
            ism_common_free(ism_memory_protocol_misc,job);
        }
    }
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);

    /* If close is in progress, proceed with it */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_replySubscribe: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
    }
}


/*
 * Reply to a create durable subscription request.
 * A durable subscription is created if the connection is cleansession=0.
 */
void ism_mqtt_replyCreateSubscription(int32_t rc, void * handle, void * vaction) {
    subjob_t * job = *(subjob_t * *) vaction;
    ism_transport_t * transport = job->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    mqtt_prodcons_t * consumer = job->consumer;
    uint32_t consumerOption = ismENGINE_CONSUMER_OPTION_NONE;
    const char * subname;
    const char * topicfilter;
    int   subname_len;
    int   subopt;
    char xbuf[512];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    ismEngine_ClientStateHandle_t kind;

    TRACE(9, "replyCreateSubscription rc=%d connect=%u handle=%p job=%p\n", rc, transport->index, handle, job);
    topicfilter = parseTopic(consumer->topic, &subname, &subname_len, &subopt);
    if (subname) {
        buf.used = 0;
        ism_common_allocBufferCopyLen(&buf, subname, subname_len);
        bputchar(&buf, 0);
        subname = (const char *)buf.buf;

        if (rc == ISMRC_ExistingSubscription) {
            job->topic[job->which] = (char *)topicfilter;
            if (subname[0] == '/')
                kind = client_SharedM;
            else
                kind = (pobj->cleansession) ? client_SharedND : client_Shared;
            rc = ism_engine_listSubscriptions(kind, (char *)subname, job, mqttSubscribe);
            job->topic[job->which] = NULL;
            if (job->sub_found == SUB_Error) {
                rc = ISMRC_ShareMismatch;
                ism_common_setError(rc);
            }
        }
    }

    if (rc) {
        job->xrc = rc;
        ism_transport_submitAsyncJobRequest(job->transport, ism_mqtt_restartSubscribe, job, 0);
        return;
    }

    if (consumer->qos)
        consumerOption |= ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;

    if (subname) {
        ismEngine_ClientStateHandle_t owningClient = NULL;
        if (!pobj->cleansession) {
            owningClient = subname[0] == '/' ? client_SharedM : client_Shared;
        }
        rc = ism_engine_createConsumer(pobj->session_handle,
                ismDESTINATION_SUBSCRIPTION, subname,
                NULL, owningClient,
                consumer, sizeof(*consumer), ism_mqtt_replyMessage, NULL,
                consumerOption, &job->consumerh, vaction,
                sizeof(subjob_t * *), ism_mqtt_replySubscribe);
    } else {
        rc = ism_engine_createConsumer(pobj->session_handle,
                ismDESTINATION_SUBSCRIPTION, topicfilter,
                NULL, NULL,
                consumer, sizeof(*consumer), ism_mqtt_replyMessage, NULL,
                consumerOption, &job->consumerh, vaction,
                sizeof(subjob_t * *), ism_mqtt_replySubscribe);
    }

    if (rc != ISMRC_AsyncCompletion) {
        if (rc) {
            consumer->closed = 1;
            xUNUSED int zrc = ism_engine_destroySubscription(pobj->handle, subname, client_Shared, NULL, 0, NULL);
        }
        job->xrc = rc;
        ism_transport_submitAsyncJobRequest(job->transport, ism_mqtt_restartSubscribe, job, (uint64_t)(uintptr_t)job->consumerh);
    }
}


/*
 * Unsubscribe to a topic.
 * In binary mqtt multiple topics can be specified and they are in the payload.
 */
void ism_mqtt_doUnsubscribe(ism_transport_t * transport, mqttMsg_t * mmsg) {
    char * pp;
    int left;
    int count = 1;
    int i;
    int rc = 0;
    char xbuf[1024];
    subjob_t * job = NULL;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

    if (!transport->pobj->mqtt_proxy && chkmsgID(mmsg)) {
        TRACE(4, "The MQTT message ID is not valid on a UNSUBSCRIBE: connect=%u client=%s id=%u\n",
                transport->index, transport->name, mmsg->msgid);
        return;
    }
    mmsg->v5_shared = pobj->v5_shared;

    /*
     * Get a count of the topics for binary mqtt.
     */
    count = 0;
    pp = (char *) mmsg->payload;
    left = mmsg->payload_len;
    while (left > 1) {
        int len = BIGINT16(pp);
        if (len == 0) {
            mmsg->rc = ISMRC_ProtocolError;
            break;
        }
        if (len + 2 > left)
            break;
        len += 2;
        left -= len;
        pp += len;
        count++;
    }
    if (mmsg->rc || left != 0 || count == 0) {
        if (!mmsg->rc)
            mmsg->rc = left ? ISMRC_BadClientData : ISMRC_ProtocolError;
        ism_common_setErrorData(mmsg->rc, "%s%s", "Unsubscribe", "topics" );
    }

    /*
     * Create the job object used for reply processing
     */
    if (!mmsg->rc) {
        job = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,105),1, sizeof(subjob_t));
        job->count = count;
        job->topic = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,106),job->count, sizeof(char *));
        job->transport = transport;
        job->msgid = mmsg->msgid;

        if (count > 8) {
            job->freebuf = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,107),1, count);
            job->ack = job->freebuf;
        } else {
            job->ack = job->ackbuf;
        }
        memset(job->ack, 0, count);

        /*
         * Fill in the job object
         */
        pp = (char *) mmsg->payload;
        left = mmsg->payload_len;
        for (i = 0; i < count; i++) {
            char * op;
            int len = BIGINT16(pp);
            mmsg->rc = 0;
            if (transport->pobj->mqtt_proxy) {
                if (len == 5 && !memcmp(pp+2, "$_$", 3)) {
                    int v1 = hexval(pp[5]);
                    int v2 = hexval(pp[6]);
                    if (v1 >= 0 && v2 >=0) {
                        job->ack[i] = (char)((v1<<4) + v2);
                    }
                }
            }
            int crc = checkString(mmsg, pp + 2, len, CHK_SUBTOPIC);    /* Validate the topic */
            if (crc) {
                ism_common_formatLastError(xbuf, sizeof xbuf);
                TRACEL(5, transport->trclevel, "Unsubscribe error: connect=%u client=%s count=%d of %d rc=%d reason=\"%s\"\n",
                        transport->index, transport->name, i+1, count, rc, xbuf);
                if ((crc != ISMRC_BadTopic && crc != ISMRC_BadSysTopic) || pobj->mqtt_version < 5) {
                    return;   /* checkString set mmsg->rc */
                }
                job->ack[i] = MQTTRC_TopicFilterInvalid;
                mmsg->rc = 0;
            }
            op = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,108),len + 1);
            if (!op) {
                mmsg->rc = ISMRC_AllocateError;
                ism_common_setError(mmsg->rc);
            }
            memcpy(op, pp + 2, len);
            op[len] = 0;
            job->topic[i] = op;
            pp += (len + 2);
        }
    }

    if (!mmsg->rc) {
        /* Do the actual work in the reply method */
        job->which = -1;
        rc = 0;

        if (pobj->session_handle) {
            if (!(__sync_fetch_and_or(&pobj->suspended, SUSPENDED_BY_PROTOCOL))) {
                rc = ism_engine_stopMessageDelivery(pobj->session_handle, &job,
                        sizeof(subjob_t * *), ism_mqtt_replyUnSubscribe);
            }
        }

        if (rc != ISMRC_AsyncCompletion) {
            ism_mqtt_replyUnSubscribe(rc, NULL, &job);
        }
    } else {
        if (job) {
            if (job->freebuf)
                ism_common_free(ism_memory_protocol_misc,job->freebuf);
            if (job->topic) {
                for (i = 0; i < count; i++) {
                    if (job->topic[i])
                        ism_common_free(ism_memory_protocol_misc,job->topic[i]);
                }
                ism_common_free(ism_memory_protocol_misc,job->topic);
            }
            ism_common_free(ism_memory_protocol_misc,job);
        }
    }
}


/*
 * Restart a subscribe.
 * This is used to keep the stack depth lower.
 */
int ism_mqtt_restartUnsubscribe(ism_transport_t * transport, void * callbackParam, uint64_t param) {
    subjob_t * job = (subjob_t *) callbackParam;
    int32_t    rc  = job->xrc;
    ism_mqtt_replyUnSubscribe(rc, (void *)(uintptr_t *)param, &job);
    return 0;
}


/*
 * Reply to an unsubscribe
 */
void ism_mqtt_replyUnSubscribe(int32_t rc, void * handle, void * vaction) {
    subjob_t * job = *((subjob_t * *) vaction);
    ism_transport_t * transport = job->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    mqtt_prodcons_t * consumer;
    int i;
    int consumer_id;
    const char * subname;
    int   subname_len;
    char * xbuf;
    char sbuf[256];
    concat_alloc_t buf = {sbuf, sizeof sbuf};

    TRACE(9, "replyUnSubscribe: connect=%u client=%s which=%d\n", transport->index, transport->name, job->which);

    /*
     * Stop message delivery. If the request is async, let
     * unsubscribe to proceed only after stop is complete.
     */
    if (rc) {
        if (pobj->mqtt_version >= 5) {
            if (job->which < 0) {
                xbuf = alloca(1024);
                ism_common_formatLastError(xbuf, 1024);
                transport->close(transport, rc, 0, xbuf);
                return;
            } else {
                job->ack[job->which] = mapToMqttRc(rc, pobj->mqtt_version, CPOI_UNSUBACK);
            }
        } else {
            TRACEL(4, transport->trclevel, "Unable to unsubscribe: rc=%d connect=%u client=%s", rc,
                    transport->index, transport->name);

            ism_common_setError(rc);

            /* Error found, send UNSUBACK, but do not unsubscribe */
            sbuf[16] = (char) (job->msgid >> 8);
            sbuf[17] = (char) job->msgid;
            transport->send(transport, sbuf + 16, 2, MT_UNSUBACK << 4,
                    SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
        }

    } else {
        while (rc != ISMRC_AsyncCompletion && job->which < job->count) {
            if (job->consumer && job->consumer->topic) {
                const char * topicfilter = parseTopic(job->consumer->topic, &subname, &subname_len, NULL);
                TRACEL(7, transport->trclevel, "Unsubscribe: connect=%u client=%s topic=%s filter=%s\n",
                        transport->index, transport->name, job->consumer->topic, topicfilter);
                if (subname) {
                    buf.used = 0;
                    ism_common_allocBufferCopyLen(&buf, subname, subname_len);
                    bputchar(&buf, 0);
                    subname = (const char *)buf.buf;
                }
                if (subname) {
                    /*
                     * Destroy shared subscription
                     *
                     * 1. Destroy the message consumer.
                     * 2. Attempt to destroy a durable subscription. This might fail,
                     *    if there are other active consumers.
                     */
                    ismEngine_ClientStateHandle_t kind;
                    if (subname[0] == '/')
                        kind = client_SharedM;
                    else
                        kind = pobj->cleansession ? client_SharedND : client_Shared;
                    xUNUSED int zrc = ism_engine_destroySubscription(pobj->handle, subname, kind, NULL, 0, NULL);
                } else {
                    if (!pobj->cleansession) {
                        /*
                         * Destroy durable subscription
                         *
                         * 1. Destroy the message consumer.
                         * 2. Attempt to destroy a durable subscription. This might fail,
                         *    if there are other active consumers.
                         */
                        xUNUSED int zrc = ism_engine_destroySubscription(pobj->handle, topicfilter, pobj->handle, NULL, 0, NULL);
                    }
                }

                ism_common_free(ism_memory_protocol_misc,(void *) job->consumer->topic);
                job->consumer->topic = NULL;
                job->consumer->handle = 0;
                job->consumer->closed = 1;
            }

            consumer = NULL;
            for (i = job->which + 1; i < job->count; i++) {
                if (job->ack[i])
                    continue;
                consumer_id = findConsumer(transport, job->topic[i]);
                job->topic[i] = NULL;
                if (consumer_id) {
                    consumer = pobj->prodcons[consumer_id];
                    job->which = i;
                    break;
                } else {
                    job->ack[i] = MQTTRC_NoSubExisted;
                }
            }
            if (!consumer) {
                job->which = job->count;   /* We are done */
            } else {
                rc = 0;
                if (consumer->handle || consumer->closed) {
                    ismEngine_ConsumerHandle_t consumerh = consumer->handle;
                    consumer->handle = 0;
                    job->consumer = consumer;

                    if (!consumer->closed) {
                        /*
                         * For a non-durable subscription, destroying the message consumer
                         * removes the subscription too
                         */
                        __sync_sub_and_fetch(&pobj->consumer_count, 1);
                        rc = ism_engine_destroyConsumer(consumerh, vaction,
                                sizeof(subjob_t * *), ism_mqtt_replyUnSubscribe);
                        if (rc == ISMRC_AsyncCompletion) {
                            ism_common_freeAllocBuffer(&buf);
                            return;
                        } else if (rc) {
                            job->xrc = rc;
                            if (job->transport->tobj)
                                ism_transport_submitAsyncJobRequest(job->transport, ism_mqtt_restartUnsubscribe, job, 0);
                            else
                                ism_mqtt_replyUnSubscribe(rc, NULL, &job);
                            ism_common_freeAllocBuffer(&buf);
                            return;
                        }
                    } else {
                        job->xrc = rc;
                        if (job->transport->tobj)
                            ism_transport_submitAsyncJobRequest(job->transport, ism_mqtt_restartUnsubscribe, job, 0);
                        else
                            ism_mqtt_replyUnSubscribe(rc, NULL, &job);
                        ism_common_freeAllocBuffer(&buf);
                        return;
                    }
                }
            }
        }

        /*
         * Send the unsubscribe reply
         */
        if (job->which >= job->count) {
            TRACEL(8, transport->trclevel, "MQTT unsubscribe: connect=%u count=%d\n", transport->index, job->which);
            if (job->msgid) {    /* No UNSUBACK if msgid = 0 (proxy protocol only) */
                sbuf[16] = (char) (job->msgid >> 8);
                sbuf[17] = (char) job->msgid;
                buf.used = 18;
                if (pobj->mqtt_version >= 5) {
                    sbuf[buf.used++] = 0;   /* No properties */
                    ism_common_allocBufferCopyLen(&buf, job->ack, job->count);
                }
                transport->send(transport, buf.buf + 16, buf.used - 16, MT_UNSUBACK << 4,
                        SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
            }
            /* Free the topic array only when we are completely done */
            if (job->topic)
                ism_common_free(ism_memory_protocol_misc,job->topic);
            ism_common_free(ism_memory_protocol_misc,job);
        }
    }
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);

    /* If close is in progress, proceed with it */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_replyUnSubscribe: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
    } else {
        transport->resume(transport, NULL);
    }
}


/*
 * Job to post a replyAction to run on an IOP thread
 */
typedef struct replyActionJob_t {
    mqtt_act_t  action;
    void *      handle;
    int32_t     rc;
} replyActionJob_t;


/*
 * Run a reply action on the IOP thread
 */
static int mqttReplyActionJob(ism_transport_t * transport, void * callbackParam, uint64_t param) {
    replyActionJob_t * job = (replyActionJob_t*)callbackParam;
    ism_mqtt_replyAction(job->rc, job->handle, &job->action);
    ism_common_free(ism_memory_protocol_misc,job);
    return 0;
}


/*
 * Submit a reply action to run on an IOP thred
 */
static void mqttReplyActionAsync(int32_t rc, void * handle, void * vaction) {
    replyActionJob_t * job =  ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,117),sizeof(replyActionJob_t));
    memcpy(&job->action, vaction, sizeof(mqtt_act_t));
    job->handle = handle;
    job->rc = rc;
    ism_transport_submitAsyncJobRequest(job->action.transport, mqttReplyActionJob, job, 0);
}


/*
 * Reply to an connect action.
 * The reply methods are used as callback targets.
 */
void ism_mqtt_replyAction(int32_t rc, void * handle, void * vaction) {
    mqtt_act_t * act = (mqtt_act_t *) vaction;
    ism_transport_t * transport = act->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    ismEngine_SessionHandle_t sessh;

    TRACE(9, "replyAction rc=%d handle=%p connect=%u client=%s user=%s inprogress=%d\n",
            rc, handle, transport->index, transport->name, transport->userid, pobj->inprogress);
    switch (act->action) {

    /* We are creating a connection, create the session */
    case Action_createConnection:
        if (rc == ISMRC_ResumedClientState) {    /* Ignore resumed state return code */
            act->resumed_session = 1;
            rc = 0;
        } else {
            if (!rc && act->inherit_durable) {
                transport->durable = 0;
                TRACE(5, "Set connection not durable with clean=0: connect=%u client=%s\n", transport->index, transport->name);
            }
        }
        if (rc) {
            act->rc = ISMRC_CreateConnection;
            if (rc == ISMRC_ClientIDInUse)
                ism_transport_checkClientLiveness(transport->clientID, transport->index);
            ism_common_setErrorData(act->rc, "%s%u", transport->name, transport->index);
            ism_mqtt_error(act);
            processSavedData(transport);
            break;
        } else {
            xUNUSED int listrc = OK;
            pobj->handle = handle;
            transport->handleCheck = (void *)handle;
            __sync_synchronize();

            /*
             * Currently the return for ism_engine_listUnreleasedDeliveryIds() will
             * always be 0.  If this changes in the future, we might want to add trace
             * output to report the non-zero return.
             */
            listrc = ism_engine_listUnreleasedDeliveryIds((ismEngine_ClientStateHandle_t)pobj->handle, (void *)pobj, (ismEngine_UnreleasedCallback_t)ism_mqtt_getIncompleteMsgIds);
            pobj->suspended = SUSPENDED_BY_PROTOCOL;
            pobj->prodcons_alloc = 100;
            pobj->prodcons = ism_common_calloc(ISM_MEM_PROBE(ism_memory_protocol_misc,118),100, sizeof(mqtt_prodcons_t *));

            /* Log connection info */
            ism_transport_connectionReady(transport);

            /*
             * Set the will message in the engine
             */
            if (act->handle) {
                act->action = Action_sendWill;
                /* Change expire to form used by engine: 0=no exiire else expire in seconds */
                uint32_t engine_expire = act->msgExpire;
                rc = ism_engine_setWillMessage(handle, ismDESTINATION_TOPIC,
                        act->topic, act->handle, pobj->will_delay, engine_expire,
                        act, sizeof(mqtt_act_t), mqttReplyActionAsync);
                act->handle = NULL;
                if (act->topic)
                    ism_common_free(ism_memory_protocol_misc,act->topic);
                act->topic = NULL;
                if (rc == ISMRC_AsyncCompletion)
                    break;
            }
        }
        /* Fall thru */

    case Action_sendWill:
        act->action = Action_createSession;
        rc = ism_engine_createSession(handle, ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND, &sessh, act, sizeof(mqtt_act_t),
                mqttReplyActionAsync);
        if (rc == ISMRC_AsyncCompletion)
            break;
        handle = sessh;
        /* Fall thru */

    case Action_createSession:
        if (rc) {
            act->rc = ISMRC_CreateSession;
            ism_common_setError(act->rc);
            ism_mqtt_error(act);
        } else {
            xUNUSED int zrc;
            /*
             * Recover durable subscriptions, if any, before starting
             * message delivery.
             */
            pobj->session_handle = handle;

            /* Re-subscribe */
            zrc = ism_engine_listSubscriptions(pobj->handle, NULL, act, ism_mqtt_reSubscribe);

            /* Send the CONNACK */
            ism_mqtt_error(act);

            /* Start or resume message delivery */
            pthread_spin_lock(&transport->lock);
            if (!pobj->aclwait) {
                pobj->aclwait = 0;
                pthread_spin_unlock(&transport->lock);
                transport->resume(transport, NULL);
            } else {
                pthread_spin_unlock(&transport->lock);
            }
        }
        processSavedData(transport);
        break;

    case Action_termConnection:
        ism_common_setErrorData(ISMRC_ClientIDInUse, "%s%s", transport->name, transport->userid);
        act->rc = ISMRC_ClientIDInUse;
        act->command = MT_CONNECT;
        ism_mqtt_error(act);
        break;
    }
}


/*
 * Re-subscribe callback from engine
 * This is an instance of ism_engine_SubscriptionCallback_t
 *
 * This is called for each existing durable dubscription at the time of a reconnect.
 * It creates the transient MQTT protocol consumer object, and the transient engine
 * consumer object.
 */
void ism_mqtt_reSubscribe(ismEngine_SubscriptionHandle_t subHandle, const char * pSubName, const char *pTopicString,
        void * properties, size_t propertiesLength, const ismEngine_SubscriptionAttributes_t *pSubAttributes, uint32_t consumerCount, void * vaction) {
    mqtt_act_t * act = (mqtt_act_t *) vaction;
    ism_transport_t * transport = act->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

    mqtt_prodcons_t * consumer;
    int id;
    ismEngine_ConsumerHandle_t consumerh;
    uint32_t consumerOption = ismENGINE_CONSUMER_OPTION_NONE;
    int32_t rc;
    ism_field_t selector = {0};
    ism_field_t field = {0};
    ism_prop_t * props = (ism_prop_t *)properties;

    id = findProdcons(transport);
    consumer = pobj->prodcons[id];
    memset(consumer, 0, sizeof(mqtt_prodcons_t));
    consumer->transport = transport;
    if (pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVER_RETAIN_AS_PUBLISHED)
        consumer->flags |=  MSUBOPT_AsPublished;
    if (pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL)
        consumer->flags |= MSUBOPT_NoLocal;

    /*
     * Reconstruct the subscription flags and subscription ID
     */
    if (properties) {
        rc = ism_common_getProperty(props, "Selector", &selector);
        if (!rc && selector.type == VT_ByteArray)
            consumer->flags |= (CONFLAG_UseFilter | CONFLAG_RerunFilter);
        rc = ism_common_getProperty(props, "GETX", &field);
        if (!rc && field.type == VT_Byte)
            consumer->publishX = (uint8_t)field.val.i;
    }

    /* Regenerate the original name */
    int sellen = 0;
    const char * seltxt = "";
    if (pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY) {
        sellen = 14;
        seltxt = "$select/QoS=0/";
    } else if (pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY) {
        sellen = 14;
        seltxt = "$select/QoS>0/";
    }

    if (!(pSubAttributes->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED)) {
        consumer->topic = (const char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,120),sellen + 5 + strlen(pTopicString) + selector.len);
        if (sellen)
            memcpy((char *)consumer->topic, seltxt, sellen);
        strcpy((char *)consumer->topic+sellen, pTopicString);
    } else {
        if (pSubName[0] == '/') {
            consumer->topic = (const char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,121),sellen + 12 + strlen(pSubName) + selector.len);
            sprintf((char *)consumer->topic, "%s$share%s", seltxt, pSubName);
        } else {
            consumer->topic = (const char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,122),sellen + 28 + strlen(pSubName) + strlen(pTopicString) + selector.len);
            sprintf((char *)consumer->topic, "%s$SharedSubscription/%s/%s", seltxt, pSubName, pTopicString);
        }
    }

    /* Make a copy of the selector */
    if (selector.type == VT_ByteArray) {
        char * sel = (char *)(consumer->topic + strlen(consumer->topic)+1);
        sel[0] = 0xfd;
        sel[1] = 0x9b;
        sel[2] = (char)(selector.len>>8);
        sel[3] = (char)selector.len;
        memcpy(sel+4, selector.val.s, selector.len);
    }
    consumer->qos = (pSubAttributes->subOptions&0x03) - ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    consumer->which = id;
    consumer->subID = pSubAttributes->subId;

    __sync_add_and_fetch(&pobj->consumer_count, 1);
    if (consumer->qos)
        consumerOption |= ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;

    TRACEL(6, transport->trclevel, "Resubscribe: connect=%d client=%s topic=%s qos=%d selopt=%x publishX=%d\n", transport->index, transport->name,
            consumer->topic, consumer->qos, pSubAttributes->subOptions, consumer->publishX);

    /* Do not create a consumer if the subscription is for a GETX */
    if (!consumer->publishX) {
        rc = ism_engine_createConsumer(pobj->session_handle,
                ismDESTINATION_SUBSCRIPTION, pSubName, NULL, NULL,
                consumer, sizeof(*consumer), ism_mqtt_replyMessage,
                NULL, consumerOption, &consumerh,
                consumer, sizeof(*consumer), ism_mqtt_replyReSubscribe);
        if (rc != ISMRC_AsyncCompletion) {
            ism_mqtt_replyReSubscribe(rc, consumerh, (void *) consumer);
        }
    }
}


/*
 * Handle the reply from the createConsumer for a re-subscribe.
 */
void ism_mqtt_replyReSubscribe(int32_t rc, void * handle, void * vaction) {
     mqtt_prodcons_t * consumer = (mqtt_prodcons_t *) vaction;
     if (!rc) {
         consumer->handle = handle;
         return;
    }
    consumer->closed = 1;
}


/*
 * Find a consumer by name.
 * This is normally only used during unsubscribe
 */
static int findConsumer(ism_transport_t * transport, const char * topic) {
    int i;
    int consumer_id = 0;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

    if (topic) {
        pthread_spin_lock(&(pobj->lock));
        for (i = 1; i < pobj->prodcons_alloc; i++) {
            mqtt_prodcons_t * consumer = pobj->prodcons[i];
            if (consumer && consumer->topic) {
                if (!strcmp(topic, consumer->topic)) {
                    consumer_id = i;
                    break;
                }
            }
        }
        pthread_spin_unlock(&(pobj->lock));
    }
    return consumer_id;
}


/*
 * Reply to a publish
 */
void ism_mqtt_replyPublish(int32_t rc, void * handle, void * vaction) {
    mqtt_act_t * act = (mqtt_act_t *) vaction;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) act->transport->pobj;
    ism_transport_t * transport = act->transport;
    int msgcode;
    const char * disconnect = NULL;
    char buf[1024];
    char xbuf[4096];
    int pending = 1;
    int goodRc = 0;

    if (UNLIKELY(rc == ISMRC_SomeDestinationsFull)) {
        __sync_add_and_fetch(&transport->listener->stats->count[transport->tid].warn_msg,  1);
        /* ISMRC_SomeDestinationsFull provides a warning statistic for
         * the server.  From the client perspective, the request has
         * succeeded. So change the rc to 0 now that the stat has been
         * recorded.
         */
        rc = 0;
    } else if (rc == ISMRC_NoMatchingDestinations || rc == ISMRC_NoMatchingLocalDestinations) {
        /* This should result in a different response for MQTT v5 */
        if (pobj->mqtt_version >= 5) {
            if (rc == ISMRC_NoMatchingDestinations)
                goodRc = MQTTRC_NoSubscription;
        }
        rc = 0;
    }

    if (act->qos == 2) {
        ism_msgid_info_t * pMsgInfo;
        msgIdLock(pobj);
        pMsgInfo = ism_msgid_getMsgIdInfo(pobj->incompmsgids, act->msgid);
        if (pMsgInfo) {
            pending = pMsgInfo->pending;
            pMsgInfo->pending = 1;
            pMsgInfo->state = ISM_MQTT_PUBREC;
            pMsgInfo->handle = (__uint128_t)((uintptr_t)handle);
        }
        msgIdUnlock(pobj);
        TRACEL(8, transport->trclevel, "ism_mqtt_replyPublish: Store unreleased delivery handle=%p connection=%u client=%s msgid=%u pending=%d inProgress=%d\n",
                handle, transport->index, transport->name, (uint32_t)act->msgid, pending, pobj->inprogress);
    }


    /*
     * Handle error in publish
     */
    if (UNLIKELY(rc != 0)) {
        if (ism_common_getLastError() != rc)
            ism_common_setError(rc);
        if (*act->locale)
            ism_common_formatLastErrorByLocale(act->locale, xbuf, sizeof xbuf);
        else
            ism_common_formatLastError(xbuf, sizeof xbuf);

        __sync_add_and_fetch(&transport->listener->stats->count[transport->tid].lost_msg, 1);
        switch (rc) {
        /* For MQTT, DestinationFull triggers a disconnect.  So we do not need
         * to track this message code in pobj->errors.
         */
        case ISMRC_DestinationFull:
            LOG(WARN, Connection, 2103, "%u%-s%-s%-s",
                    "The client request to publish a message to the server failed because the destination is full: "
                    "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.", transport->index, transport->name, transport->protocol, transport->listener->name);
            disconnect = xbuf;
            goodRc = MQTTRC_QuotaExceeded;
            break;

        case ISMRC_DestNotValid:
            msgcode = 2105;
            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2105, "%u%-s%-s%-s",
                        "The client request to publish a message to the server failed because the topic name includes more than 32 levels: "
                        "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.", transport->index, transport->name, transport->protocol, transport->listener->name);
            }
            if (pobj->mqtt_version > 3) {     /* In V3.1.1 force a disconnect with this reason code */
                disconnect = xbuf;
                goodRc = MQTTRC_TopicInvalid;
            }
            break;

        case ISMRC_BadSysTopic:
            msgcode = 2110;
            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2110, "%u%-s%-s%-s",
                        "The client request to publish a message to the server failed because the topic is a system topic: "
                        "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3}.", transport->index, transport->name, transport->protocol, transport->listener->name);
            }
            if (pobj->mqtt_version > 3) {     /* In V3.1.1 force a disconnect with this reason code */
                disconnect = xbuf;
                goodRc = MQTTRC_TopicInvalid;
            }
            break;

        case ISMRC_NotAuthorized:
            msgcode = 2106;

            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2106, "%u%-s%-s%-s%-s",
                        "The client request to publish a message to the server failed due to an authorization failure: "
                        "ConnectionID={0} ClientID={1} Protocol={2} Endpoint={3} UserID={4}.", transport->index, transport->name, transport->protocol, transport->listener->name, transport->userid ? transport->userid : "");
            }
            if (pobj->mqtt_version > 3) {     /* In V3.1.1 force a disconnect with this reason code */
                disconnect = xbuf;
                goodRc = MQTTRC_NotAuthorized;
            }
            break;

        default:
            msgcode = 2101000 + rc;
            if (!previouslyLogged(pobj, msgcode)) {
                LOG(WARN, Connection, 2101, "%u%-s%-s%-s%-s%s%d",
                        "The client request to publish a message to the server failed: ConnectionID={0} ClientID={1} Endpoint={2} UserID={3} Protocol={4} Error={5} RC={6}.",
                        transport->index, transport->name, transport->listener->name, transport->userid ? transport->userid : "", transport->protocol, xbuf, rc);
                switch (rc) {
                /* Force a disconnect for the following errors. */
                case ISMRC_AllocateError:
                case ISMRC_ServerCapacity:
                case ISMRC_StoreOwnerLimit:
                case ISMRC_Error:
                    disconnect = xbuf;
                    break;
                default:
                    if (pobj->mqtt_version > 3)     /* In V3.1.1 force a disconnect with this reason code */
                        disconnect = xbuf;
                    break;
                }
            }
            break;
        }
    }

    if (act->isMsgid) {
        buf[16] = (char) (act->msgid >> 8);
        buf[17] = (char) act->msgid;

        /* For proxy protocol with NAK, add NAK fields */
        if (rc && goodRc && (pobj->mqtt_version >= 5 || pobj->mqtt_proxy)) {
            concat_alloc_t rbuf = {buf, sizeof buf};
            rbuf.used = 18;
            if (pobj->mqtt_version >= 5) {
                buf[rbuf.used++] = goodRc;
                if (act->rc && *xbuf && pobj->requestReason) {
                    int reasonlen = strlen(xbuf);
                    if (reasonlen && (!transport->pobj->maxPacketSize || (reasonlen+11 < transport->pobj->maxPacketSize))) {
                        ism_common_putMqttVarInt(&rbuf, reasonlen+3);
                        ism_common_putMqttPropString(&rbuf, MPI_Reason, g_ctx5, xbuf, reasonlen);
                    } else {
                        bputchar(&rbuf, 0);         /* proplen = 0 */
                    }
                } else {
                    bputchar(&rbuf, 0);
                }
                if (act->qos == 2 && goodRc >= 128 && pobj->flow_count > 0) {
                    pthread_spin_lock(&transport->lock);
                    pobj->flow_count--;
                    pthread_spin_unlock(&transport->lock);
                }
            } else if (pobj->mqtt_proxy) {
                ism_common_putExtensionValue(&rbuf, EXIV_ServerRC, (int16_t)act->rc);
                ism_common_putExtensionString(&rbuf, EXIV_ReasonString, xbuf);
            }
            act->transport->send(act->transport, rbuf.buf+16, rbuf.used-16, act->qos == 2 ? (MT_PUBREC << 4) : (MT_PUBACK << 4),
                SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
            TRACEL(7, act->transport->trclevel, "Send %s: connect=%u client=%s rc=%d reason=%s\n", act->qos==2 ? "PUBREL" : "PUBACK",
                    act->transport->index, act->transport->name, goodRc, xbuf);
            if (UNLIKELY(rbuf.inheap))
                ism_common_freeAllocBuffer(&rbuf);
            disconnect = NULL;
            rc = 0;
        } else if (!disconnect) {
            int outlen = 2;
            if (goodRc && pobj->mqtt_version >= 5) {
                buf[18] = goodRc;
                buf[19] = 0;       /* No props */
                outlen = 4;
            }
            if (act->qos < 2) {
                if (pobj->flow_max > 0) {
                    pthread_spin_lock(&transport->lock);
                    if (pobj->flow_count > 0) {
                        pobj->flow_count--;
                    }
                    pthread_spin_unlock(&transport->lock);
                }
                act->transport->send(act->transport, buf + 16, outlen, (MT_PUBACK << 4),
                    SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
            } else {
                int i;
                for (i = 0; i < pending; i++) {
                    act->transport->send(act->transport, buf + 16, outlen, (MT_PUBREC << 4),
                        SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
                }
            }
        }
    }

    if (disconnect) {
        if (!pobj->publishNAK) {
            /* Disconnect */
            TRACEL(6, transport->trclevel, "ism_mqtt_replyPublish: Disconnect due to failure to publish a message=%u connect=%u client=%s rc=%d\n",
                    (uint32_t)act->msgid,transport->index, transport->name, rc);
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_replyPublish: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
            if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, pending) < 0)) { /* BEAM suppression: constant condition */
                /* Closing process is already in progress */
                TRACE(9, "replyClose from connect=%u\n", transport->index);
                ism_mqtt_replyCloseClient(transport);
            } else {
                act->transport->close(act->transport, rc, 0, disconnect);
            }
            return;
        } else {
            TRACE(6, "ism_mqtt_replyPublish: Failed to publish a message=%u connect=%u client=%s rc=%d pending=%u\n",
                            (uint32_t)act->msgid,transport->index, transport->name, rc, pending);
            ism_mqtt_replyPublishSimple(act, pending);
            return;
        }
    }
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_replyPublish: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, pending) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(act->transport);
    }
}


/*
 * We need to destroy the producer if the call to create it goes
 * asynchronous.  We also destroy the producer when it's no longer
 * needed because the publish topic has changed or because the connection
 * is closed.
 */
static void mqttDestroyProducer(mqtt_pubobj_t * publisher) {
    if (publisher->producerh) {
        xUNUSED int zrc = ism_engine_destroyProducer(publisher->producerh, NULL, 0, NULL);
        publisher->producerh = NULL;
    }
    publisher->count = 0;
    publisher->lasttopic_len = 0;
}

/*
 * Put out the properties for a publish
 */
void putPublishProperties(concat_alloc_t * buf, ism_transport_t * transport, int topic_alias,
        int subID, int msgtype, uint32_t outexpire, char * propp, int proplen) {
    ism_field_t obj;
    char tbuf[64];
    char xbuf[2048];
    concat_alloc_t outbuf = {xbuf, sizeof xbuf};
    int contentTypeSent = 0;

    if (topic_alias)
        ism_common_putMqttPropField(&outbuf, MPI_TopicAlias, g_ctx5, topic_alias);
    if (subID)
        ism_common_putMqttPropField(&outbuf, MPI_SubID, g_ctx5, subID);
    if (msgtype == MTYPE_MQTT_Text)
        ism_common_putMqttPropField(&outbuf, MPI_PayloadFormat, g_ctx5, 1);
    if (outexpire)
        ism_common_putMqttPropField(&outbuf, MPI_MsgExpire, g_ctx5, outexpire);

    if (proplen) {
        concat_alloc_t mapbuf = {propp, proplen, proplen};
        while (mapbuf.pos < mapbuf.used) {
            int rc = ism_protocol_getObjectValue(&mapbuf, &obj);
            if (rc)
                break;
            /* Put out user properties */
            if (obj.type == VT_Name) {
                const char * value;
                const char * key = obj.val.s;
                if (key) {
                    ism_protocol_getObjectValue(&mapbuf, &obj);
                    value = ism_common_fieldToString(&obj, tbuf);
                    if (value) {
                        ism_common_putMqttPropNamePair(&outbuf, MPI_UserProperty, g_ctx5, key, -1, value, -1);
                    } else if (obj.type == VT_ByteArray) {
                        /* Send byte array as string if it is valid UTF-8 */
                        if (ism_common_validUTF8Restrict(obj.val.s, obj.len, UR_NoControl | UR_NoNonchar) >= 0)
                            ism_common_putMqttPropNamePair(&outbuf, MPI_UserProperty, g_ctx5, key, -1, obj.val.s, obj.len);
                    }
                }
            }

            /* Put out the system properties */
            else if (obj.type == VT_NameIndex) {
                /* Put out the Reply Topic */
                if (obj.val.i == ID_ReplyToT) {
                    rc = ism_protocol_getObjectValue(&mapbuf, &obj);
                    if (obj.type == VT_String &&
                            ism_common_validUTF8Restrict(obj.val.s, -1, UR_NoControl | UR_NoNonchar | UR_NoWildcard) >= 0) {
                        ism_common_putMqttPropString(&outbuf, MPI_ReplyTopic, g_ctx5, obj.val.s, -1);
                    }
                }

                /* Put out the Correlation. This is a string in JMS and byte array in MQTT */
                else if (obj.val.i == ID_CorrID) {
                    rc = ism_protocol_getObjectValue(&mapbuf, &obj);
                    if (obj.type == VT_String || obj.type == VT_ByteArray) {
                        if (obj.type == VT_String)
                            obj.len = strlen(obj.val.s);
                        ism_common_putMqttPropString(&outbuf, MPI_Correlation, g_ctx5, obj.val.s, obj.len);
                    }
                }

                else if (contentTypeSent==0 && (obj.val.i == ID_JMSType || obj.val.i == ID_ContentType)) {
                    rc = ism_protocol_getObjectValue(&mapbuf, &obj);
                    if (obj.type == VT_String) {
                        contentTypeSent = 1;
                        ism_common_putMqttPropString(&outbuf, MPI_ContentType, g_ctx5, obj.val.s, -1);
                    }
                }

                else if (obj.val.i == ID_MsgID) {
                    rc = ism_protocol_getObjectValue(&mapbuf, &obj);
                    if (obj.type == VT_String) {
                        ism_common_putMqttPropNamePair(&outbuf, MPI_UserProperty, g_ctx5, "JMSMessageID", -1, obj.val.s, -1);
                    }
                }

                else if (obj.val.i == ID_Timestamp) {
                   rc = ism_protocol_getObjectValue(&mapbuf, &obj);
                    if (obj.type == VT_Long && obj.val.l > 0) {
                        ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
                        ism_common_setTimestamp(ts, obj.val.l * 1000000L);   /* Convert to nanos */
                        ism_common_formatTimestamp(ts, tbuf, 64, 7, ISM_TFF_ISO8601_2);
                        ism_common_putMqttPropNamePair(&outbuf, MPI_UserProperty, g_ctx5, "JMSTimestamp", -1, tbuf, -1);
                        ism_common_closeTimestamp(ts);
                    }
                }
            }
        }
    }
    /* Output properties */
    ism_common_putMqttVarInt(buf, outbuf.used);
    ism_common_allocBufferCopyLen(buf, outbuf.buf, outbuf.used);
}



/*
 * Reply with a received message.  This is the engine callback from the message consumer.
 */
HOT bool ism_mqtt_replyMessage(ismEngine_ConsumerHandle_t consumerh,
        ismEngine_DeliveryHandle_t deliveryh, ismEngine_MessageHandle_t msgh,
        uint32_t seqnum, ismMessageState_t state, uint32_t options,
        ismMessageHeader_t * hdr, uint8_t areas,
        ismMessageAreaType_t areatype[areas], size_t areasize[areas],
        void * areaptr[areas], void * vaction) {
    mqtt_prodcons_t * consumer = (mqtt_prodcons_t *) vaction;
    ism_transport_t * transport = consumer->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    consumer = pobj->prodcons[consumer->which];
    int command = 1;
    int32_t msgid = 0;
    uint32_t proplen = 0;
    uint32_t bodylen = 0;
    char * propp = NULL;
    char * bodyp = NULL;
    int i;
    int qos;
    int count;
    int check;
    int topicpos;
    int topicalias;
    bool returncode = true;
    char xbuf[12000] __attribute__((aligned(8)));

    /*
     * Find the props and body
     */
    for (i = 0; i < areas; i++) {
        if (areatype[i] == ismMESSAGE_AREA_PROPERTIES) {
            proplen = (uint32_t) areasize[i];
            propp = (char *) areaptr[i];
        } else if (areatype[i] == ismMESSAGE_AREA_PAYLOAD) {
            bodylen = (uint32_t) areasize[i];
            bodyp = (char *) areaptr[i];
        }
    }

    /*
     * The output QSoS is the min of the message and subscription QoS
     */
    qos = hdr->Reliability;
    if (qos > consumer->qos) {
        TRACE(9, "Decrease MQTT message QoS from %d to %d: connect=%u client=%s\n",
                qos, (int)consumer->qos, transport->index, transport->name);
        qos = consumer->qos;
    }

    if (qos >= ismMESSAGE_RELIABILITY_EXACTLY_ONCE
            && hdr->Persistence == ismMESSAGE_PERSISTENCE_NONPERSISTENT) {
        qos = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    }

    /*
     * Assume we have a message
     */
    assert(msgh);

    if (UNLIKELY(qos > 1 && state == ismMESSAGE_STATE_RECEIVED)) {
        /*
         * Delivery is being resumed following reconnection,
         * send PUBREL.  For MQTTv3.1 set the DUP flag but not for other versions.
         */
        char mbuf[100];
        msgid = seqnum;
        memset(mbuf, 0x00, sizeof(mbuf));
        TRACE(8, "Set the PUBREL sent state of the msgID for message: connect=%u client=%s msgid=%u\n",
                            transport->index, transport->name, msgid);
        msgIdLock(pobj);
        ism_msgid_addMsgIdInfo(pobj->msgids, deliveryh, msgid, ISM_MQTT_PUBREL);
        msgIdUnlock(pobj);

        mbuf[16] = (char) (msgid >> 8);
        mbuf[17] = (char) msgid;
        int cmd = (MT_PUBREL << 4) | 0x02;
        if (pobj->mqtt_version == 3)
            cmd |= 0x08;   /* DUP flag set but only on MQTTv3.1 */
        transport->send(transport, mbuf + 16, 2, cmd, SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
        ism_engine_releaseMessage(msgh);
        return true;
    }
    {
        int  topiclen;
        concat_alloc_t buf = { xbuf, sizeof xbuf };
        ism_field_t ftopic;
        concat_alloc_t pbuf;
        int rc = 0;

        /*
         * Find the topic name in the properties
         */
        if (proplen) {
            memset(&pbuf, 0, sizeof(pbuf));
            pbuf.buf = propp;
            pbuf.len = proplen;
            pbuf.used = proplen;
            ism_findPropertyNameIndex(&pbuf, ID_Topic, &ftopic);
#if 0
            /* temp */
            ism_field_t f;
            ism_findPropertyNameIndex(&pbuf, ID_OriginServer, &f);
            if (f.type == VT_String)
                printf("OriginServer=%s\n", f.val.s);
            ism_findPropertyNameIndex(&pbuf, ID_ServerTime, &f);
            if (f.type == VT_Long)
                printf("ServerTime=%lu\n", f.val.l);
#endif
        } else {
            ftopic.type = VT_Null;
        }


        /*
         * If we have a selector after restart filter the message again
         */
        const char * sel;
        if ((consumer->flags&CONFLAG_RerunFilter) && consumer->topic) {
            sel = consumer->topic + strlen(consumer->topic) + 1;
            if (sel[0] == (char)0xfd && sel[1] == (char)0x9b) {
                int sellen = BIGINT16(sel+2);
                memcpy(xbuf, sel+4, sellen);   /* Align the rule */
                rc = ism_protocol_selectMessage(hdr, areas, areatype, areasize, areaptr,
                        ftopic.type == VT_String ? ftopic.val.s : NULL, xbuf, sellen);
                if (rc == SELECT_FALSE) {
                    pthread_spin_lock(&pobj->sessionlock);
                    if (deliveryh && pobj->session_handle) {
                        xUNUSED int xrc = ism_engine_confirmMessageDelivery(pobj->session_handle,
                            NULL, deliveryh, ismENGINE_CONFIRM_OPTION_CONSUMED, NULL, 0, NULL);
                    }
                    pthread_spin_unlock(&pobj->sessionlock);
                    return true;
                }
            }
        }

        /*
         * If the topic name is missing, use the name of the subscription.  This is a problem as
         * it might contain wildcard characters which we will need to replace later.
         */
        if (ftopic.type != VT_String || ftopic.val.s == NULL) {
            ftopic.val.s = (char *) consumer->topic;
            TRACEL(8, transport->trclevel, "Topic name was not in the properties, using topic in consumer object: connect=%u client=%s topic=%s\n",
                    transport->index, transport->name, ftopic.val.s);
        }

        /*
         * Validate that the topic name is valid Unicode and does not contain prohibited characters.
         * If it does, replace them so we know the topic is valid.
         */
        topiclen = (int)strlen(ftopic.val.s);

        /*
         * In V3.1.1 do not allow Unicode non-characters, control characters, or wildcards
         * In V3.1 only restrict the 0 character and wildcards
         */
        check = pobj->mqtt_version > 3 ? (UR_NoControl | UR_NoNonchar | UR_NoWildcard) : (UR_NoWildcard | UR_NoNull);
        count = ism_common_validUTF8Restrict(ftopic.val.s, topiclen, check);

        if (consumer->publishX) {
            buf.buf[16] = qos<<1;
            if (consumer->subID) {
                buf.buf[16] |= 0x20;
                buf.buf[17] = 0;
                buf.buf[18] = 3;
                buf.used = 19;
                ism_common_putExtensionValue(&buf, EXIV_SubID, consumer->subID);
                topicpos = 22;
                buf.used = 24;
            } else {
                topicpos = 17;
                buf.used = 19;
            }
            ism_engine_suspendMessageDelivery(consumerh, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
            returncode = false;
        } else {
            topicpos = 16;
            buf.used = 18;     /* Skip to after topic length */
        }

        topicalias = 0;
        if (UNLIKELY(count < 0)) {
            /* Replace any invalid codepoints with the replacement char */
            int  savelen = buf.used;
            char * op = ism_common_allocAllocBuffer(&buf, topiclen+1, 0);  /* For 1 byte replacement char */
            topiclen = ism_common_validUTF8Replace(ftopic.val.s, topiclen, op, check, '?');
            buf.used = savelen+topiclen;
        } else {
            /* Implement outgoing topic alias */
            if (pobj->topicalias_count_out) {
                int xx;
                for (xx=0; xx<pobj->topicalias_count_out; xx++) {
                    if (pobj->topicalias_out[xx] == NULL) {
                        pobj->topicalias_out[xx] = ism_transport_putString(transport, ftopic.val.s);
                        topicalias = xx+1;
                        break;
                    }
                    if (!strcmp(pobj->topicalias_out[xx], ftopic.val.s)) {
                        topicalias = xx+1;
                        topiclen = 0;
                        break;
                    }
                }
            }
            /* Just copy the buffer */
            ism_common_allocBufferCopyLen(&buf, ftopic.val.s, topiclen);
        }
        buf.buf[topicpos] = (char) (topiclen >> 8);
        buf.buf[topicpos+1] = (char) topiclen;

        /*
         * Put in the msgid for qos>0
         */
        if (qos) {
            msgid = seqnum;
            msgIdLock(pobj);
            ism_msgid_addMsgIdInfo(pobj->msgids, deliveryh, msgid, ISM_MQTT_PUBLISH);
            msgIdUnlock(pobj);
            char * p = ism_common_allocAllocBuffer(&buf, 2, 0);
            p[0] = (char) (msgid >> 8);
            p[1] = (char) msgid;
        }

        /*
         * Put out the properties for this message for MQTTv5 and above
         */
        if (pobj->mqtt_version >= 5) {
            uint32_t outexpire = 0;
            if (hdr->Expiry) {
                uint32_t nowexpire = ism_common_nowExpire();
                if (nowexpire >= hdr->Expiry)
                    outexpire = 1;
                else
                    outexpire = hdr->Expiry - nowexpire;
            }
            putPublishProperties(&buf, transport, topicalias, consumer->subID, hdr->MessageType, outexpire, propp, proplen);
        }

        /*
         * Copy the payload.  In the normal case this is byte we just do a copy.
         * If the source is a JMS Map or Stream message, convert it to JSON.
         */
        switch (hdr->MessageType) {
        case MTYPE_MapMessage:
        case MTYPE_StreamMessage:
            ism_mqtt_putJsonPayloadContent(transport, &buf, bodyp, bodylen, hdr->MessageType != MTYPE_MapMessage);
            break;
        default:
            ism_common_allocBufferCopyLen(&buf, bodyp, bodylen);
            break;
        }

        /*
         * Set the bits in the PUBLISH command
         */
        if (consumer->publishX) {
            command = 0x3F;
        } else {
            command = (MT_PUBLISH << 4) | (qos << 1);
            if (consumer->flags&MSUBOPT_AsPublished || pobj->mqtt_bridge) {   /* If bridge, use the retain flag from the publish */
                if (hdr->Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN)
                    command |= 0x01;   /* Retain */
            } else {                   /* If not bridge, use the retain flag of the message */
                if (hdr->Flags & ismMESSAGE_FLAGS_RETAINED)
                    command |= 0x01;   /* Retain */
            }
            if (hdr->RedeliveryCount)
                command |= 0x08;       /* Dup flag */
        }

        /*
         * Send the message unless it is too big
         */
        if (pobj->maxPacketSize && pobj->mqtt_version >= 5) {
            int chklen = packetLength(buf.used-16);
            if (chklen > pobj->maxPacketSize) {
                rc = ISMRC_MsgTooBig;
                ism_common_setErrorData(rc, "%u%u", chklen, pobj->maxPacketSize);
                __sync_add_and_fetch(&transport->lost_msg, 1);
                if (qos) {
                    msgIdLock(pobj);
                    ism_msgid_delMsgIdInfo(pobj->msgids, msgid, NULL);
                    msgIdUnlock(pobj);
                    pthread_spin_lock(&pobj->sessionlock);
                    if (deliveryh && pobj->session_handle) {
                        xUNUSED int xrc = ism_engine_confirmMessageDelivery(pobj->session_handle,
                            NULL, deliveryh, ismENGINE_CONFIRM_OPTION_CONSUMED, NULL, 0, NULL);
                    }
                    pthread_spin_unlock(&pobj->sessionlock);
                }
            }
        }
        if (rc != ISMRC_MsgTooBig) {
            pthread_spin_lock(&pobj->sessionlock);
            rc = transport->send(transport, buf.buf + 16, buf.used - 16, command, SFLAG_FRAMESPACE);
            if (UNLIKELY(rc == SRETURN_SUSPEND)) {
                TRACEL(6, transport->trclevel, "Consumer was suspended by transport:  connect=%u client=%s topic=%s consumer=%p(%d)\n",
                        transport->index, transport->name, consumer->topic, consumer, consumer->which);
                __sync_fetch_and_or(&pobj->suspended, SUSPENDED_BY_TRANSPORT);
                ism_engine_suspendMessageDelivery(consumerh, ismENGINE_SUSPEND_DELIVERY_OPTION_NONE);
                returncode = false;
            }
            pthread_spin_unlock(&pobj->sessionlock);
            uint64_t * p1 = &transport->listener->stats->count[transport->tid].write_msg;
            uint64_t * p2 = &transport->write_msg;
            __sync_add_and_fetch(p1, 1);
            __sync_add_and_fetch(p2, 1);
        }
        if (UNLIKELY(buf.inheap != 0))
            ism_common_freeAllocBuffer(&buf);

        /*
         * Release the message
         */
        ism_engine_releaseMessage(msgh);

        return returncode;
    }
}


/*
 * Convert the JMS map or stream message to a JSON payload
 */
void ism_mqtt_putJsonPayload(ism_transport_t * transport, concat_alloc_t * buf, char * body,
        uint32_t bodylen, char inarray) {
    ism_json_putBytes(buf, ", \"payload\": ");
    ism_mqtt_putJsonPayloadContent(transport, buf, body, bodylen, inarray);
    bputchar(buf, '}');       /* End of message */
    bputchar(buf, 0);         /* Null terminate for debug */
    buf->used--;
}


/*
 * Put out the contents of a map or stream message as the payload contents
 */
void ism_mqtt_putJsonPayloadContent(ism_transport_t * transport, concat_alloc_t * buf, char * body,
        uint32_t bodylen, char inarray) {
    ism_field_t field;
    int first = 1;
    concat_alloc_t in;
    memset(&in, 0, sizeof(in));
    in.buf = body;
    in.used = bodylen;
    if (in.used < 1 || (uint8_t) * in.buf != (inarray ? 0x9e : 0x9f)) {
        TRACE(3, "Invalid JMS content when converting to MQTT.  The payload is ignored: connect=%u client=%s\n",
                transport->index, transport->name);
        return;
    }
    in.pos = 1;
    bputchar(buf, inarray ? '[' : '{');
    while (ism_protocol_getObjectValue(&in, &field) == 0) {
        if (field.type == VT_Name) {
            if (!first)
                bputchar(buf, ',');
            else
                first = 0;
            bputchar(buf, '"');
            ism_json_putEscapeBytes(buf, field.val.s,
                    (int) strlen(field.val.s));
            bputchar(buf, '"');
            bputchar(buf, ':');
        } else {
            if (inarray) {
                if (!first)
                    bputchar(buf, ',');
                else
                    first = 0;
            }
            ism_json_put(buf, NULL, &field, 0);
        }
    }
    bputchar(buf, inarray ? ']' : '}');
    bputchar(buf, 0); /* Null terminate for debug */
    buf->used--;
}


/*
 * Receive a connection closing notification for the MQTT protocol.
 * We start the closing and it is completed in replyClosed().
 */
int ism_mqtt_closing(ism_transport_t * transport, int rc, int clean,
        const char * reason) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    int32_t count;
    char response[1024];

    TRACEL(8, transport->trclevel, "ism_mqtt_closing: connect=%u client=%s rc=%d clean=%d reason=%s inprogress=%d\n",
            transport->index, transport->name, rc, clean, reason, transport->pobj->inprogress);

    removeFromClientsList(pobj, 1);

    /* Send disconnect monitoring message */
    ism_iot_disconnectMsg(transport, rc, reason);

    /* Send a server DISCONNECT */
    if (pobj->send_disconnect && (pobj->mqtt_version >= 5 || pobj->mqtt_proxy)) {
        pobj->send_disconnect = 0;
        concat_alloc_t rbuf = {response, sizeof response};

        response[16] = mapToMqttRc(rc, pobj->mqtt_version, CPOI_DISCONNECT);
        response[17] = 0;

        if (pobj->mqtt_version >= 5) {
            rbuf.used = 17;
            if (reason && rc) {
                int reasonlen = strlen(reason);
                if (reasonlen && (!transport->pobj->maxPacketSize || (reasonlen+11 < transport->pobj->maxPacketSize))) {
                    ism_common_putMqttVarInt(&rbuf, reasonlen+3);
                    ism_common_putMqttPropString(&rbuf, MPI_Reason, g_ctx5, reason, reasonlen);
                } else {
                    rbuf.used++;            /* proplen = 0 */
                }
            } else {
                rbuf.used++;
            }
        } else if (pobj->mqtt_proxy) {
            response[17] = 0;
            rbuf.used = 18;
            ism_common_putExtensionValue(&rbuf, EXIV_ServerRC, rc);
            ism_common_putExtensionString(&rbuf, EXIV_ReasonString, reason);
        }
        transport->send(transport, rbuf.buf + 16, rbuf.used-16, MT_DISCONNECT << 4, SFLAG_SYNC | SFLAG_FRAMESPACE);

        if (rbuf.inheap)
            ism_common_freeAllocBuffer(&rbuf);
    }

    /* Transport throttle */
    if (ism_throttle_isEnabled()) {
        int needSetLastClientRC = 1;
        if (rc == ISMRC_ClientIDReused) {
            ism_throttle_incrementClienIDStealCount(transport->clientID);
            needSetLastClientRC=0;
        }
        if (needSetLastClientRC)
            ism_throttle_setLastCloseRC(transport->clientID, rc);
    }

    /*
     * Set the indicator that close is in progress. If set failed,
     * then this has been done before and we don't need to proceed.
     */
    if (!__sync_bool_compare_and_swap(&pobj->closed, 0, 1)) {
        TRACE(9, "ism_mqtt_closing: pobj is closed. connect=%u inprogress=%d\n", transport->index, pobj->inprogress);
        return 0;
    }

    /* Subtract the "in progress" indicator. If it becomes negative,
     * no actions are in progress, so it is safe to clean up protocol data
     * and close the connection. If it is non-negative, there are
     * actions in progress. The action that sets this value to 0
     * would re-invoke closing().
     */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_closing: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    count = __sync_sub_and_fetch(&pobj->inprogress, 1);
    if (count >= 0) {
        if (__sync_bool_compare_and_swap(&pobj->getx_active, 1, 0)) {
            count = __sync_sub_and_fetch(&pobj->inprogress, 1);
        }
        TRACEL(8, transport->trclevel, "ism_mqtt_closing postponed as there are %d actions/messages in progress: connect=%u client=%s\n",
                count+1, transport->index, transport->name);
        return 0;
    }

    pobj->clean = clean;

    /* Stop message delivery, destroy session and client state */
    TRACE(9, "replyClose from connect=%u\n", transport->index);
    ism_mqtt_replyCloseClient(transport);
    return 0;
}


/*
 * Session closed.  If last one, close the connection
 */
void ism_mqtt_replyCloseClient(ism_transport_t * transport) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    void * handle;
    mqtt_act_t act = { 0 };
    act.transport = transport;

    TRACE(9, "ism_mqtt_replyCloseClient: connect=%u client=%s inprogress=%d\n", transport->index, transport->name, transport->pobj->inprogress);

    if (!__sync_bool_compare_and_swap(&pobj->closed, 1, 2)) {
        TRACE(4, "ism_mqtt_replyCloseClient called more than once for: connect=%u client=%s\n", transport->index, transport->name);
        return;
    }

    ism_security_returnAuthHandle(transport->security_context);

    pthread_spin_lock(&pobj->sessionlock);
    pobj->session_handle = 0;

    /* Ensure that we don't call ism_engine_destroyClientState twice. */
    handle = pobj->handle;
    pobj->handle = 0;

    pthread_spin_unlock(&pobj->sessionlock);
    if (pobj->publisher[0].lasttopic) {
        ism_common_free(ism_memory_protocol_misc,pobj->publisher[0].lasttopic);
        pobj->publisher[0].lasttopic = NULL;
    }

    if (handle) {
        int mode =ismENGINE_DESTROY_CLIENT_OPTION_NONE;
        mqttDestroyProducer(&pobj->publisher[0]);
        TRACE(9, "ism_mqtt_replyCloseClient: ism_engine_destroyClientState: connect=%u client=%s inprogress=%d\n", transport->index, transport->name, transport->pobj->inprogress);
        int rc = ism_engine_destroyClientState(handle, mode,
                &act, sizeof(act), ism_mqtt_replyDoneConnection);
        if (rc == ISMRC_AsyncCompletion)
            return;
    }else{
        TRACE(9, "ism_mqtt_replyCloseClient: pobj->handle is null. Skip destroy client state: connect=%u client=%s inprogress=%d\n", transport->index, transport->name, transport->pobj->inprogress);
    }
    ism_mqtt_replyDoneConnection(0, NULL, &act);
}


/*
 * The engine connection is closed, close our connection, and tell the transport it can close its connection
 */
void ism_mqtt_replyDoneConnection(int32_t rc, void * handle, void * vaction) {
    mqtt_act_t * act = (mqtt_act_t *) vaction;
    ism_transport_t * transport = act->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    mqtt_prodcons_t * consumer;
    int i;

    if (pobj->pingTimer) {
        /* This is a ping timer */
        ism_common_cancelTimer(pobj->pingTimer);
        pobj->pingTimer = NULL;
    }

    /* Do the WebSockets close */
    if (rc==0 && pobj->prot == PROT_MQTT_WSBIN) {
        ism_transport_closeWS(transport, 1000);
    }
    TRACE(7, "close MQTT connection: connect=%u client=%s inprogress=%d\n", transport->index, transport->name, transport->pobj->inprogress);

    /* Delete the connection artifacts */
    for (i = 0; i < pobj->prodcons_alloc; i++) {
        consumer = pobj->prodcons[i];
        if (consumer && consumer->topic) {
            ism_common_free(ism_memory_protocol_misc,(void *) consumer->topic);
            consumer->topic = NULL;
        }
        ism_common_free(ism_memory_protocol_misc,consumer);
    }
    pobj->prodcons_alloc = 0;
    ism_common_free(ism_memory_protocol_misc,pobj->prodcons);
    pobj->prodcons = NULL;

    msgIdLock(pobj);
    ism_msgid_freelist(pobj->msgids);
    pobj->msgids = NULL;
    ism_msgid_freelist(pobj->incompmsgids);
    pobj->incompmsgids = NULL;
    msgIdUnlock(pobj);

    if (pobj->errors) {
        ism_common_destroyHashMap(pobj->errors);
        pobj->errors = NULL;
    }
    if (pobj->tenantInfo) {
        ism_common_HashMapLock(tenantsMap);
        pobj->tenantInfo->conCount--;
        TRACE(7, "Close connection to tenant %s: allowed=%d count=%d connect=%u client=%s\n",
                  pobj->tenantInfo->name, pobj->tenantInfo->conCount, pobj->tenantInfo->maxConAllowed, transport->index, transport->name);
        ism_common_HashMapUnlock(tenantsMap);
        pobj->tenantInfo = NULL;
    }
    while (pobj->savedDataHead) {
        mqttSavedData_t * sd = pobj->savedDataHead;
        pobj->savedDataHead = sd->next;
        ism_common_free(ism_memory_protocol_misc,sd);
    }
    pobj->savedDataTail = NULL;
    pobj->savedSize = 0;
    pthread_spin_destroy(&pobj->lock);
    pthread_spin_destroy(&pobj->msgids_spinlock);
    pthread_mutex_destroy(&pobj->msgids_mutex);
    pthread_spin_destroy(&pobj->sessionlock);

    /* Tell the transport we are done */
    transport->closed(transport);
}

/*
 * Send the will.
 * The will is sent if the connection is closed abnormally (without first receiving a DISCONNECT)
 * This will be replaced by engine function when it is done.
 */
void ism_mqtt_setWill(ism_transport_t * transport, mqttMsg_t * mmsg, mqtt_act_t * act) {
    size_t areasize[2];
    void * areaptr[2];
    ismMessageHeader_t hdr;
    ismEngine_MessageHandle_t msgh;
    concat_alloc_t buf = { 0 };
    int rc;
    const char * serverUID = NULL;

    /*
     * Set up the header
     */
    memset(&hdr, 0, sizeof hdr);
    hdr.Reliability = (uint8_t)(mmsg->flags >> 3) & 3;
    hdr.Persistence = hdr.Reliability != 0;
    hdr.Priority = 4;
    hdr.Flags = (mmsg->flags&CFLAG_WillRetain) ? ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN : 0;
    hdr.MessageType = mmsg->msgtype;

    /*
     * Put the topic name into the properties.
     */
    buf.len = mmsg->willtopic_len + 64;            /* Extra for ensure capacity */
    if (mmsg->flags&CFLAG_WillRetain) {
        serverUID = ism_common_getServerUID();
        buf.len += strlen(serverUID ? serverUID : 0);
        if (mmsg->willtopic_len == 0)
            hdr.MessageType = MTYPE_NullRetained;
    }
    buf.buf = alloca(buf.len);
    ism_protocol_putNameIndex(&buf, ID_Topic);
    ism_protocol_putStringLenValue(&buf, mmsg->willtopic, mmsg->willtopic_len);

    /*
     * If the will message is retained, put in the OriginServer, but do not set the ServerTime.
     * The ServerTime will be added when the message is sent by the engine.
     */
    if (mmsg->flags&CFLAG_WillRetain) {
        ism_protocol_putNameIndex(&buf, ID_OriginServer);
        ism_protocol_putStringValue(&buf, serverUID);
    }

    /*
     * Copy any MQTTv5 sting and user properties into the internal will message
     */
    if (transport->pobj->mqtt_version >= 5 && mmsg->willproplen) {
        ism_common_checkMqttPropFields((char *)mmsg->willprops, mmsg->willproplen,
            g_ctx5, CPOI_WILLPROP, mpropPublishProps, &buf);
    }

    areasize[0] = buf.used;
    areaptr[0] = buf.buf;
    areasize[1] = mmsg->willpayload_len;
    areaptr[1] = (char *) mmsg->willpayload;

    /*
     * Create the message
     */
    rc = ism_engine_createMessage(&hdr, 2, MsgAreas, areasize, areaptr, &msgh);
    if (rc) {
        act->rc = rc;
        ism_common_setError(act->rc);
        ism_mqtt_error(act);
        return;
    }

    act->handle = msgh;
    act->topic = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,128),mmsg->willtopic_len + 1);
    if (!act->topic) {
        act->rc = ISMRC_AllocateError;
        ism_common_setError(act->rc);
        ism_mqtt_error(act);
    } else {
        memcpy(act->topic, mmsg->willtopic, mmsg->willtopic_len);
        act->topic[mmsg->willtopic_len] = 0;
    }
    act->will_qos = hdr.Reliability;
    if (mmsg->isMsgExpire) {
        act->msgExpire = mmsg->msgExpire == 0 ? 1 : mmsg->msgExpire;
    }
    /* The buffer should always remain in the heap, but check to make sure */
    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
}


/*
 * The actual frame support is in frame.c in transport
 */
XAPI int ism_transport_addMqttFrame(ism_transport_t * transport, char * buf,
        int len, int command);
XAPI int ism_transport_addWSFrame(ism_transport_t * transport, char * buffer,
        int len, int kind);


/*
 * For a WebSockets Binary MQTT we need to add both the MQTT frame and the WebSockets frame
 */
int ism_mqtt_addwsbframe(ism_transport_t * transport, char * buf, int len,
        int command) {
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
 * Start message delivery for all registered MQTT subscribers.
 * @param transport  The transport object
 * @param userdata   The user object
 * @param flags      The option flags
 * @return A return code 0=good.
 */
static int startMqttDelivery(ism_transport_t * transport, void * userdata, uint64_t flags) {
    int rc = 0;
    mqttProtoObj_t * pobj;
    if (userdata)
        transport = (ism_transport_t *)userdata;
    pobj = (mqttProtoObj_t *) transport->pobj;

    /* If the connection or session has been closed, do not try
     * to start message delivery. */
    if (!pobj) {
        return 0;
    }
    TRACE(6, "startMqttDelivery: connect=%u client=%s flags=%lu\n", transport->index, transport->name, (uint64_t)flags);

    pthread_spin_lock(&pobj->sessionlock);
    if (pobj->session_handle && !pobj->closed) {
        int mask = (flags) ? (~SUSPENDED_BY_TRANSPORT) : (~SUSPENDED_BY_PROTOCOL);
        if (__sync_and_and_fetch(&pobj->suspended, mask) == 0) {
            pthread_spin_unlock(&pobj->sessionlock);
            TRACE(8, "mqttResume connect=%u client=%s mask=%8x\n", transport->index, transport->name, mask);
            rc = ism_engine_startMessageDelivery(pobj->session_handle,
                    ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
        } else {
            pthread_spin_unlock(&pobj->sessionlock);
            TRACE(8, "mqttResume not connect=%u client=%s mask=%8x suspended=%u\n",
                    transport->index, transport->name, mask, pobj->suspended);
        }
    } else {
        pthread_spin_unlock(&pobj->sessionlock);
    }

    if (rc == 0) {
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in startMqttDelivery: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
        if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
            TRACE(9, "replyClose from connect=%u\n", transport->index);
            ism_mqtt_replyCloseClient(transport);
        }
        return 0;
    }
    /* Check if we are closing */
    if (pobj->inprogress < 1) {
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in startMqttDelivery: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
        if (__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)
            TRACE(9, "replyClose from connect=%u\n", transport->index);
            ism_mqtt_replyCloseClient(transport);
        return 0;
    }
    return rc;
}


/*
 * Resume message delivery for the MQTT session for the transport.
 *
 * @param transport  The transport object
 * @param userdata   The data context for the work item
 * @return A return code: 0=good
 */
int ism_mqtt_resume(ism_transport_t * transport, void * userdata) {
    if (transport->pobj) {
        mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Increment inprogress in ism_mqtt_resume: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress+1);
#endif
        if (UNLIKELY(__sync_fetch_and_add(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
            __sync_fetch_and_sub(&pobj->inprogress, 1);
            return 0;
        }
        if (!mqtt_unit_test) {
            ism_transport_submitAsyncJobRequest(transport, startMqttDelivery, NULL, userdata!=NULL);
        } else {
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_resume: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
            __sync_fetch_and_sub(&pobj->inprogress, 1);
        }
        return 0;
    } else {
        return -1;
    }
}


/*
 * This function performs disconnect, if the client fails to communicate
 * within the Keep Alive timer schedule.
 */
static int mqttTimerDisconnect(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    mqttProtoObj_t * curr;
    mqttProtoObj_t * next;
    pthread_spin_lock(&clientsListLock);
    uint64_t currTime = (uint64_t) ism_common_readTSC();
    curr = clientsListHead;
    while (curr) {
        next = curr->next;
        checkLastAccessTime(curr, currTime);
        curr = next;
    }
    pthread_spin_unlock(&clientsListLock);
    return 1;
}


/*
 * Start disconnect processing by stopping message delivery.
 */
void ism_mqtt_doDisconnect(ism_transport_t * transport, mqttMsg_t * mmsg) {
    mqtt_act_t act = { 0 };
    uint32_t rc = 0;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

    pobj->send_disconnect = 0;
    act.transport = transport;
    act.rc = mmsg->mqtt_rc;
    if (mmsg->reason && mmsg->reason_len) {
        act.topic = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,129),mmsg->reason_len + 1);
        memcpy(act.topic, mmsg->reason, mmsg->reason_len);
        act.topic[mmsg->reason_len] = 0;
    }
    if (act.rc < 0x80)
        transport->closestate[3] = 1;           /* Protocol disconnected */

    /* Override expire time at disconnect */
    if (mmsg->isExpire) {
        if (pobj->expireTTL == 0 && mmsg->expireTTL) {
            /* This is a protocol error, but there is no good way to report it */
            TRACE(5, "Session Expiry Interval cannot be changed at disconnect for a connection with a zero Session Expiry Interval at connect: connect=%u client=%s\n",
                transport->index, transport->name);
        } else {
            uint32_t expire = mmsg->expireTTL;
            if (expire > pobj->maxExpire)
                expire = pobj->maxExpire;
            ism_security_context_setClientStateExpiry(transport->security_context, expire);
            pobj->expireTTL = expire;   /* Save the new value for test */
        }
    }

    if (pobj && pobj->session_handle) {
        rc = ism_engine_stopMessageDelivery(pobj->session_handle, &act,
                sizeof(act), ism_mqtt_replyDisconnect);
        if (rc == ISMRC_AsyncCompletion) {
            return;
        }
    }

    if (rc == 0) {
        ism_mqtt_replyDisconnect(0, NULL, &act);
    }
}


/*
 * Finish disconnect processing (including NACKing messages and destroying session).
 */
void ism_mqtt_replyDisconnect(int32_t rc, void * handle, void * vaction) {
    mqtt_act_t * act = (mqtt_act_t *) vaction;
    ism_transport_t * transport = act->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

    if (pobj->handle && act->rc == 0) {
        xUNUSED int zrc = ism_engine_unsetWillMessage(pobj->handle, NULL, 0, NULL);                /* BEAM suppression: no effect */
    }
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_replyDisconnect: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
    } else {
        if (act->rc == 0 || act->rc == 4) {
            transport->close(transport, 0, 1, "The connection has completed normally.");
        } else {
            const char * reason = act->topic;
            int    ismrc = mapToIsmRc(act->rc, pobj->mqtt_version);
            if (reason == NULL) {
                char * xbuf = alloca(1024);
                reason = ism_common_getErrorString(mapToIsmRc(act->rc, pobj->mqtt_version), xbuf, 1024);
            }
            transport->close(transport, ismrc, 0, reason);
        }
    }
    if (act->topic) {
        ism_common_free(ism_memory_protocol_misc,act->topic);
        act->topic = NULL;
    }
}


/*
 * Send publish acknowledgment in response to conditions like
 * duplicate publish or errors like not authorized and message too
 * big. In these cases no additional actions like storing undelivered
 * message handles are required.
 *
 * @param act  A pointer to the action structure that contains
 *                relevant information.
 */
void ism_mqtt_replyPublishSimple(mqtt_act_t * act, int times) {
    ism_transport_t * transport = act->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

    if (act->isMsgid > 0) {
        int i;
        char buf[32];
        buf[16] = (char) (act->msgid >> 8);
        buf[17] = (char) act->msgid;
        for (i = 0; i < times; i++) {
            transport->send(transport, buf + 16, 2,
                act->qos == 2 ? (MT_PUBREC << 4) : (MT_PUBACK << 4),
                SFLAG_SYNC | SFLAG_FRAMESPACE | SFLAG_OUTOFORDER);
        }
    }

    /* If close is in progress, proceed with it */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_replyPublishSimple: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, times) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
    }
}

/*
 * Reply with a simple publish NAK
 * This only works when we are in synchronous checking of the publish
 */
void ism_mqtt_replyPublishNAK(ism_transport_t * transport, mqttMsg_t * mmsg) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    int sendnak = 0;
    int rc = mmsg->rc;

    switch (pobj->mqtt_version) {
    case 3:             /* MQTT v3.1  */
        if (mmsg->rc == ISMRC_BadSysTopic) {
            sendnak = 1;
            mmsg->rc = 0;
        }
        break;
    case 4:             /* Leave the RC set bad causing a connection close */
        break;
    case 5:             /* Send the nak with rc and continue for QoS>0 */
    default:
        if (mmsg->isMsgid) {
            sendnak = 1;
            mmsg->rc = 0;
        }
    }

    if (mmsg->isMsgid && sendnak) {
        int cmd = mmsg->qos == 2 ? (MT_PUBREC << 4) : (MT_PUBACK << 4);
        char xbuf[2092];
        char mbuf[2048];
        concat_alloc_t rbuf = {xbuf, sizeof xbuf};
        ism_common_formatLastError(mbuf, sizeof mbuf);
        xbuf[16] = (char) (mmsg->msgid >> 8);
        xbuf[17] = (char) mmsg->msgid;
        rbuf.used = 18;
        if (pobj->mqtt_version >= 5) {
            xbuf[18] = mapToMqttRc(rc, pobj->mqtt_version, CPOI_PUBACK);
            rbuf.used++;
            if (rc && *mbuf && pobj->requestReason) {
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
            TRACE(7, "Send %s: connect=%u client=%s rc=%d reason=%s\n", mmsg->qos==2 ? "PUBREL" : "PUBACK",
                    transport->index, transport->name, (uint8_t)xbuf[18], mbuf);
        }

        transport->send(transport, rbuf.buf + 16, rbuf.used - 16, cmd, SFLAG_SYNC | SFLAG_FRAMESPACE);

        /* If close is in progress, proceed with it */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in ism_mqtt_replyPublishNAK: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
        if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
            TRACE(9, "replyClose from connect=%u\n", transport->index);
            ism_mqtt_replyCloseClient(transport);
        }
    }

}


/*
 * Continue subscription processing
 */
static void ism_mqtt_continueSubscriptionProccessing(int32_t rc, void * handle, void * vaction) {
    subjob_t * job = *((subjob_t * *) vaction);
    if (rc == 0 || rcIgnoredOnSubscribe(rc, job)) {
        rc = 0;
        job->which--;
        ism_mqtt_replySubscribe(rc, handle, vaction);
        return;
    }
    ism_mqtt_onSubscribeError(rc,job);
}

/*
 * Delete existing subscription as part of re-subscribe logic
 * (subscribing using the same subscription name, but
 * different QoS).
 */
void ism_mqtt_deleteSubscription(int32_t rc, void * handle, void * vaction) {
    subjob_t * job = *((subjob_t * *) vaction);
    ism_transport_t * transport = job->transport;
    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    mqtt_prodcons_t * consumer = job->consumer;

    if (consumer) {
        if (consumer->handle) {
            ismEngine_ConsumerHandle_t consumerh = consumer->handle;
            consumer->handle = 0;
            consumer->closed = 1;
            rc = ism_engine_destroyConsumer(consumerh, vaction, sizeof(subjob_t * *),
                    ism_mqtt_deleteSubscription);
            if (rc == ISMRC_AsyncCompletion)
                return;
        }
        /*
         * If there are ACKs pending, clean them up.
         */
        job->consumer = NULL;
        if (!rc && !pobj->cleansession && consumer->topic) {
            char *topic = (char*)consumer->topic;
            consumer->topic = NULL;
            TRACEL(7, transport->trclevel, "Destroy subscription: connect=%u client=%s topic=%s\n",
                    transport->index, transport->name, topic);
            rc = ism_engine_destroySubscription(pobj->handle, topic, pobj->handle,
                        vaction, sizeof(subjob_t * *), ism_mqtt_continueSubscriptionProccessing);
            ism_common_free(ism_memory_protocol_misc,topic);
        }
        if (rc != ISMRC_AsyncCompletion) {
            ism_mqtt_continueSubscriptionProccessing(rc, NULL, vaction);
        }
    }
}


/*
 * Process a sendACL
 *
 * Updating the ACLs will affect any filtered subscriptions as the common filter is to
 * do an acl check.
 *
 * This packet allows an optional extension to send back the subscriptions, or to send back
 * only the non-filtered subscriptions.  This is done to allow the proxy to unsubscribe from
 * them if they are no longer allowed by the new ACLs.
 */
static int doSendACL(ism_transport_t * transport, mqttMsg_t * mmsg) {
    xUNUSED int rc;
    int  ret = 0;

    mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
    int  aclwait = ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_ACLWait, 0);
    int  sendnfsubs = ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_SendNFSubs, 0);
    int  sendsubs = ism_common_getExtensionValue(mmsg->ext, mmsg->ext_len, EXIV_SendSubs, 0);

    /* Set the ACLs  */
    if (mmsg->payload_len)
        rc = ism_protocol_setACL(mmsg->payload, mmsg->payload_len, 0, NULL);

    /* Return subs if requested */
    if (sendsubs || sendnfsubs) {
        char * xbuf = alloca(4096);
        xbuf[16] = 0;    /* No extensions */
        xbuf[17] = 0;
        concat_alloc_t sbuf = {xbuf, 4096, 18};
        getSubs(&sbuf, transport, sendnfsubs);
        if (sbuf.used > 18) {
            transport->send(transport, sbuf.buf+16, sbuf.used-16, MT_SENDSUBS, SFLAG_FRAMESPACE);
            if (sbuf.inheap)
                ism_common_freeAllocBuffer(&sbuf);
        }
    }

    /* Complete aclwait */
    else {
        pthread_spin_lock(&transport->lock);
        /* If we are in aclwait, and we have not been requested to continue waiting, start up message delivery now */
        if (!aclwait && pobj->aclwait) {
            pobj->aclwait = 0;
            pthread_spin_unlock(&transport->lock);
            transport->resume(transport, NULL);
        } else {
            pthread_spin_unlock(&transport->lock);
        }
    }
    return ret;
}


/*
 * Check if this error was previously logged
 */
static int previouslyLogged(mqttProtoObj_t * pobj, int msgcode) {
    void * alreadylogged = NULL;
    pthread_spin_lock(&(pobj->lock));
    if (!pobj->errors) {
        pobj->errors = ism_common_createHashMap(5,HASH_INT32);
    }
    ism_common_putHashMapElement(pobj->errors, &msgcode, sizeof(msgcode),
            (void *) 1, &alreadylogged);
    pthread_spin_unlock(&(pobj->lock));

    if (alreadylogged)
        return 1;
    return 0;
}


/*
 * Reply from asynchronous authentication.
 * This is run as a iop async job so that it runs in a thread with the store initialized.
 * At this point the security context is filled in so we can process items in it.
 */
static int mqttReplyAuthTT(ism_transport_t * transport, void * callbackParam, uint64_t async) {
    mqtt_act_t * act = (mqtt_act_t *) callbackParam;
    int authrc = act->rc;
    mqttProtoObj_t * pobj;
    int rc = 0;
    uint32_t options;
    ismEngine_ClientStateHandle_t client;
    int  msgRate = 0;

    transport = act->transport;
    pobj = (mqttProtoObj_t *) transport->pobj;

    /* Check/set additional authorization parameters */
    if (authrc == ISMRC_OK) {
        /*
         * Set the expected messageF rate.
         * Allow for the proxy and server to be at different levels
         */
        if (pobj->msgRate) {    /* Set expected message rate if specified from proxy */
            if (pobj->msgRate < EXPECTEDMSGRATE_DEFAULT)
                msgRate = EXPECTEDMSGRATE_LOW;
            else if (pobj->msgRate == EXPECTEDMSGRATE_DEFAULT)
                msgRate = EXPECTEDMSGRATE_DEFAULT;
            else if (pobj->msgRate >= EXPECTEDMSGRATE_MAX)
                msgRate = EXPECTEDMSGRATE_MAX;
            else
                msgRate = EXPECTEDMSGRATE_HIGH;
            ism_security_context_setExpectedMsgRate(transport->security_context, (ExpectedMsgRate_t)pobj->msgRate);
        } else {
            pobj->msgRate = ism_security_context_getExpectedMsgRate(transport->security_context);
        }

        /*
         * For cleansession=false MQTT sessions, we need to consider when the session should expire.
         *
         * If an expiration time as been explicitly set, we can use that (limiting it to the maximum
         * value in the connection policy). If an expiration has not been set we use the limit in the
         * connection policy.
         */
        if (!pobj->cleansession || pobj->mqtt_version >= 5) {
            uint32_t expire = ism_security_context_getMaxSessionExpiryInterval(transport->security_context);
            if (expire > pobj->maxExpire) {
                expire = pobj->maxExpire;
            } else {
                pobj->maxExpire = expire;
            }

            if (pobj->expire_set && pobj->expireTTL <= expire) {
                expire = pobj->expireTTL;
            } else {
                pobj->modifiedExpire = expire;
            }
            ism_security_context_setClientStateExpiry(transport->security_context, expire);
            pobj->expireTTL = expire;
        }

        /*
         * Set the max in-flight message size.
         * This is done by proxy or by MQTTv5
         */
        if (pobj->maxActive) {   /* Set max active size if specified from proxy */
            int maxActive = ism_security_context_getInflightMsgLimit(transport->security_context);
            if (!maxActive || maxActive > pobj->maxActive) {
                ism_security_context_setInflightMsgLimit(transport->security_context, (uint32_t)pobj->maxActive);
                maxActive = (uint32_t)pobj->maxActive;
            }

            /* Default expected message rate from maxActive (MQTTv5) */
            if (pobj->msgRate == 0) {
                if (!maxActive)
                    maxActive = 128;
                else if (maxActive > 1024)
                    msgRate = EXPECTEDMSGRATE_HIGH;
                else if (maxActive > 64)
                    msgRate = EXPECTEDMSGRATE_DEFAULT;
                else
                    msgRate = EXPECTEDMSGRATE_LOW;
                ism_security_context_setExpectedMsgRate(transport->security_context, (ExpectedMsgRate_t)msgRate);
            }
        }

        /* Set max TCP buffer based on expected message rate */
        ism_protocol_setSocketBuffer(transport);

        /*
         * Check if peristent sessions are allowed
         */
        if (pobj->cleansession == 0) {
            /* Check to see whether the connection policy permits this (ie, permits durable subscription) */
            int allow_durable = ism_security_context_getAllowDurable(transport->security_context);
            if (allow_durable < 1) {
                if (!allow_durable) {
                    authrc = ISMRC_DurableNotAllowed;
                    TRACE(4, "Durable session is not allowed: connect=%u client=%s\n", transport->index, transport->name);
                    ism_common_setErrorData(ISMRC_DurableNotAllowed, "%s", transport->name);
                } else {
                    /* This should never happen but trace it if it does. */
                    authrc = ISMRC_Error;
                    TRACEL(2, transport->trclevel, "Failed to process check for allow durable: connect=%u client=%s\n",
                            transport->index, transport->name);
                }
            }
        }
        if (authrc == ISMRC_OK) {
            /* Set allow_persistent to determine whether pulish actions are allows to send messages with QoS > 0 */
            transport->pobj->allow_persistent = ism_security_context_getAllowPersistentMessages(transport->security_context);

            if (UNLIKELY(transport->pobj->allow_persistent < 0)) {
                /* This should never happen but trace it if it does. */
                authrc = ISMRC_Error;
                TRACE(2, "Failed to process check for allow persistent messages: connect=%u client=%s\n", transport->index, transport->name);
            }
        }

        /*
         * Check for QoS on will message
         */
        if ((authrc == ISMRC_OK) && !transport->pobj->allow_persistent && act->will_qos) {
            authrc = ISMRC_InvalidQoS;
            TRACEL(4, transport->trclevel, "QoS not allowed for will message : connect=%u client=%s will_qos=%d\n", transport->index, transport->name, act->will_qos);
            ism_common_setErrorData(ISMRC_InvalidQoS, "%s", transport->name);
        }
    }

    if (authrc != ISMRC_OK) {
        int d;
        /* Failed to authenticate. Set the error code */
        if (authrc != ISMRC_Closed) {
            act->rc = CRC_NotAuthorized;
            ism_common_setError(ISMRC_NotAuthorized);
            ism_mqtt_error(act);
            d = 1;
        } else {
            pobj->startState = MQTT_DISCONNECTED;
            d = 2;
        }
        if (async)
            ism_common_free(ism_memory_protocol_misc,act);
        processSavedData(transport);
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in mqttReplyAuthTT: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
        if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, d) < 0)) { /* BEAM suppression: constant condition */
            TRACE(9, "replyClose from connect=%u\n", transport->index);
            ism_mqtt_replyCloseClient(transport);
        }
        return 0;
    } else {
        TRACE(8, "User is authenticated and authorized: connect=%u client=%s user=%s\n",
                transport->index, transport->name, transport->userid ? transport->userid : "null");
    }


    options = ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL;
    if (pobj->mqtt_version >= 5) {
        /* MQTTv5 clean start and expire are separate */
        if (pobj->cleansession == 1)
            options |= ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART;
        if (!pobj->expire_set || pobj->expireTTL > 0) {
            pobj->cleansession = 0;    /* Change the bit to its legacy meaning */
            options |= ismENGINE_CREATE_CLIENT_OPTION_DURABLE;
        } else {
            if (!pobj->cleansession) {
                /* If expire=0 and !cleanstart, the durability is inherited */
                options |= ismENGINE_CREATE_CLIENT_OPTION_INHERIT_DURABILITY;
                act->inherit_durable = 1;
            }
        }
    } else {
        /* MQTTv3.1 and MQTTv3.1.1 clean session bit controls both */
        if (pobj->cleansession == 0)
            options |= ismENGINE_CREATE_CLIENT_OPTION_DURABLE;
        else
           options |= ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART;
    }

    addToClientsList(pobj, 1, transport);

    act->action = Action_createConnection;
    transport->handleCheck = CLIENT_STATE_IN_PROGRESS;   /* Indicate we are setting the client state */
    __sync_synchronize();
    TRACE(9, "call createClientState: connect=%u client=%s options=%x\n", transport->index, transport->name, options);
    rc = ism_engine_createClientState(transport->clientID, PROTOCOL_ID_MQTT, options, transport,
            ism_mqtt_replySteal, transport->security_context, &client, act,
            sizeof(mqtt_act_t), mqttReplyActionAsync);
    if (rc != ISMRC_AsyncCompletion) {
        ism_mqtt_replyAction(rc, client, act);
    }
    if (async)
        ism_common_free(ism_memory_protocol_misc,act);
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Decrement inprogress in mqttReplyAuthTT: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress-1);
#endif
    if (UNLIKELY(__sync_sub_and_fetch(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
        TRACE(9, "replyClose from connect=%u\n", transport->index);
        ism_mqtt_replyCloseClient(transport);
    }
    return 0;
}


/*
 * Reply from asynchronous authorization request.
 * This is commonly called in one of the security worker threads, but move further
 * processing to a timer thread as the store is initialized there.
 */
static void mqttReplyAuth(int authrc, void * callbackParam) {
    mqtt_act_t * act = (mqtt_act_t *) callbackParam;
    if (UNLIKELY(mqtt_unit_test)) {
        /* CUNIT */
        ism_transport_t * transport = act->transport;
        mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;
        /* -1 was already reached, so closing is in progress already */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Increment inprogress in mqttReplyAuth: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress+1);
#endif
        if (LIKELY(__sync_fetch_and_add(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
            __sync_fetch_and_sub(&pobj->inprogress, 1);
            return;
        }
        act->rc = authrc;
        mqttReplyAuthTT(transport,act,0);
    } else {
        mqtt_act_t * action = NULL;
        ism_transport_t * transport = act->transport;
        mqttProtoObj_t * pobj = (mqttProtoObj_t *) transport->pobj;

        if (transport->client_host)
            transport->client_addr = transport->client_host;
        if (authrc==ISMRC_OK) {
            ism_throttle_removeThrottleObj(transport->name);
        } else {
            ism_throttle_incrementAuthFailedCount(transport->name);
        }
        action = ism_common_malloc(ISM_MEM_PROBE(ism_memory_protocol_misc,134),sizeof(mqtt_act_t));
        memcpy(action, act, sizeof(mqtt_act_t));
        action->rc = authrc;

        /* -1 was already reached, so closing is in progress already */
#ifdef DEBUG_INPROGRESS
		TRACE(9, "Increment inprogress in mqttReplyAuth: connect=%u inprogress=%d inprogress_next=%d\n", transport->index, pobj->inprogress, pobj->inprogress+1);
#endif
        if (UNLIKELY(__sync_fetch_and_add(&pobj->inprogress, 1) < 0)) { /* BEAM suppression: constant condition */
            __sync_fetch_and_sub(&pobj->inprogress, 1);
            return;
        }
        ism_transport_submitAsyncJobRequest(transport, mqttReplyAuthTT, action, 1);
    }
}


/*
 * Format the MQTT protocol specific object in the transport object
 */
static int mqttDumpPobj(ism_transport_t * transport, char * buf, int len) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t*) transport->pobj;
    int n, i;
    if (!buf || len<8)
        return pobj ? pobj->inprogress : 0;
    if (!pobj) {
        sprintf(buf, "null");
        return 0;
    }
    n = snprintf(buf, len - 1, "MQTT pobj: startState=%d closed=%d suspended=%d client=%p cleansession=%d inprogress=%d\n",
                 (int) pobj->startState, (int) pobj->closed, pobj->suspended,
                 pobj->handle, (int) pobj->cleansession, pobj->inprogress);
    if (n >= len) {
        buf[len - 1] = '\0';
        return 0;
    }
    for (i = 0; i < pobj->prodcons_alloc; i++) {
        mqtt_prodcons_t * pc = pobj->prodcons[i];
        if (pc == NULL)
            continue;
        buf += n;
        len -= n;
        n = snprintf(buf, len, "Consumer %d: closing=%d closed=%d qos=%d handle=%p topic=%s\n",
                pc->which, (int) pc->closing, (int) pc->closed, (int) pc->qos,
                pc->handle, ((pc->topic) ? pc->topic : "null"));
        if (n >= len) {
            buf[len - 1] = '\0';
            return 0;
        }
    }
    return 0;
}

/*
 * Callback function for ism_engine_listUnreleasedDeliveryIds().  Assures that previously pending
 * undelivered messages are available in pobj->incompmsgids to avoid sending duplicate messages.
 */
static void ism_mqtt_getIncompleteMsgIds(uint32_t deliveryId, ismEngine_UnreleasedHandle_t hUnrel, void * pContext) {
    mqttProtoObj_t * pobj = (mqttProtoObj_t *)pContext;
    ism_transport_t * transport = pobj->transport;
    TRACEL(6, transport->trclevel, "ism_mqtt_getIncompleteMsgIds: connectn=%u client=%s hUnrel=%p msgid=%u\n",
            transport->index, transport->name, hUnrel, deliveryId);
    msgIdLock(pobj);
    ism_msgid_addMsgIdInfo(pobj->incompmsgids, (__uint128_t)((uintptr_t)hUnrel), (uint16_t)deliveryId, ISM_MQTT_PUBREC);
    msgIdUnlock(pobj);
}

static ism_timer_t keepAliveTimer;
static uint16_t msgIdRange;


/*
 * Reply from setting up the monitoring session
 */
static void mqttReplyStart(int rc, void * handle, void * vaction) {
    if (rc) {
        TRACEL(2, ism_defaultTrace, "Unable to create the monitor session: rc=%d\n", rc);
    } else {
        TRACEL(6, ism_defaultTrace, "Create the monitor session and reconcile monitor messages\n");
        g_monitor_session = handle;
        ism_iot_reconcile();
    }
}

/*
 * Start message processing
 */
static int mqttStartMessaging(void) {
    int   rc;
    ismEngine_SessionHandle_t handle;

    client_Shared = ism_protocol_getSharedClient(1);
    client_SharedND = ism_protocol_getSharedClient(0);
    client_SharedM = ism_protocol_getSharedClient(2);   /* Create in case we backlevel */
    if (g_AllowNewShared) {
        rc = ism_engine_createSession(client_SharedM, 0, &handle, NULL, 0, mqttReplyStart);
        if (rc != ISMRC_AsyncCompletion)
            mqttReplyStart(rc, handle, NULL);
    }
    return 0;
}


/*
 * Process a new MQTT connection.
 *
 * MQTT is accepted with multiple encodings and framing.
 * Multiple versions of the MQTT spec are supported, as well as the proxy protocol
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
        transport->actionname = mqttCommand;
        transport->checkLiveness = mqttCheckLiveness;
        if (!strcmpi(transport->protocol, "mqtt") || !strcmpi(transport->protocol, "mqtt4"))
            transport->protocol = "mqtt";
        else
            transport->protocol = "mqttv3.1"; /* Make the string constant */
        rc = 0;
    }

    /*
     * Binary MQTT over tcp
     * These strings have previously been made constant
     */
    else if (!strcmp(transport->protocol, "mqtt-tcp") || !strcmp(transport->protocol, "mqtt4-tcp") ||
             !strcmp(transport->protocol, "mqtt-br") || !strcmp(transport->protocol, "mqtt4-br") ||
             !strcmp(transport->protocol, "mqtt-px") || !strcmp(transport->protocol, "mqtt4-px") ||
             !strcmp(transport->protocol, "mqtt5-tcp") || !strcmp(transport->protocol, "mqtt5-px")) {
        pobj = (mqttProtoObj_t *) ism_transport_allocBytes(transport, sizeof(mqttProtoObj_t), 1);
        memset((char *) pobj, 0, sizeof(mqttProtoObj_t));
        transport->pobj = pobj;
        transport->dumpPobj = mqttDumpPobj;
        transport->receive = ism_mqtt_receive;
        pobj->prot = PROT_MQTT_BIN;
        transport->actionname = mqttCommand;
        transport->checkLiveness = mqttCheckLiveness;
        rc = 0;
    }

    /*
     * If we have a valid protocol, do common initialization
     */
    if (rc == 0) {
        uint64_t client_uid = __sync_add_and_fetch(&clientIDCounter, 1)& 0x0000FFFFFFFFFFFF;
        transport->protocol_family = "mqtt"; /* Constant string */
        transport->closing = ism_mqtt_closing;
        transport->resume = ism_mqtt_resume;
        pthread_spin_init(&pobj->lock, 0);
        pthread_spin_init(&pobj->msgids_spinlock, 0);
        pthread_mutex_init(&pobj->msgids_mutex, NULL);
        pthread_spin_init(&pobj->sessionlock, 0);
        pobj->client_uid = client_uid;
        msgIdLock(pobj);
        pobj->msgids = ism_create_msgid_list(transport, 0, msgIdRange);
        pobj->incompmsgids = ism_create_msgid_list(transport, 1, 0xFFFF);
        msgIdUnlock(pobj);
        pobj->errors = NULL;
    }
    return rc;
}

#undef TRACE_DOMAIN
#define TRACE_DOMAIN ism_defaultTrace

/*
 * Initialize the MQTT protocol
 */
int ism_protocol_initMQTT(void) {
    mqtt_unit_test = (getenv("CUNIT") != NULL);
    pthread_spin_init(&clientsListLock, 0);

    //if(ism_common_getBooleanConfig("UseSpinLocks", 0))
    if(ism_common_getBooleanConfig("UseMsgIdSpinLock", 0))
    	g_msgIdlockType = USE_SPIN_LOCK;
    else
    	g_msgIdlockType = USE_MUTEX;

    ism_msgid_init();
    ism_transport_registerProtocol(mqttStartMessaging, ism_mqtt_connection);
    keepAliveTimer = ism_common_setTimerRate(ISM_TIMER_LOW, mqttTimerDisconnect,
            NULL, 1, 10, TS_SECONDS);
    msgIdRange = ism_common_getIntConfig("mqttMsgIdRange", 1000);
    tenantsMap = ism_common_createHashMap(4096, HASH_BYTE_ARRAY);
    mqttMaxSubs = ism_common_getIntConfig("MqttMaxSubscriptions", MQTT_MAX_SUBS);
    mqttMaxKeepAlive = ism_common_getIntConfig("MqttMaxKeepAlive", 0);
    mqttMaxNonprodCount = ism_common_getIntConfig("MaxNonprodCount", MAX_NONPROD_COUNT);

    /*
     * Set Capabilities for the MQTT protocol
     * Capabilities: UseTopic and UseShared
     */
    int capability =  ISM_PROTO_CAPABILITY_USETOPIC | ISM_PROTO_CAPABILITY_USESHARED;
    ism_admin_updateProtocolCapabilities("mqtt", capability);

    /*
     * Set the global variables for MQTT protocol options
     */
    g_AllowMQTTpxProtocol = ism_common_getBooleanConfig("Protocol.AllowMqttProxyProtocol", 0);
    g_AllowMQTTv5         = ism_common_getIntConfig("AllowMQTTv5", 1);    /* Default ON if not specified */
    g_AllowNewShared      = g_AllowMQTTpxProtocol || g_AllowMQTTv5;
    TRACE(3, "MQTT protocol options:  AllowProxy=%u AllowMQTTv5=%u AllowNewShared=%u\n",
            g_AllowMQTTpxProtocol, g_AllowMQTTv5, g_AllowNewShared);
    g_ctx5 = ism_common_makeMqttPropCtx(mqtt_propFields, 5);

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
    TRACE(3, "MqttTopicAliasIn=%d MqttTopicAliasOut=%d\n", g_MaxTopicAliasIn, g_MaxTopicAliasOut);

    g_verifyPayload = ism_common_getBooleanConfig("MqttVerifyPayload", g_verifyPayload);
    g_verifyReasonCode = ism_common_getBooleanConfig("MqttVerifyReasonCode", g_verifyPayload);

    return 0;
}
