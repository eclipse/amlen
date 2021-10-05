/*
 * Copyright (c) 2018-2021 Contributors to the Eclipse Foundation
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

/*
 * TODOs:
 * - Test Event Streams destination
 * - Additional error handling (such as better choice between FAIL and WAIT)
 * - MaxPacket size is not implemented (optional)
 * - Implement topic alias (this is optional)
 */
#define TRACE_COMP Proxy
#include <ismutil.h>
#include <pxtransport.h>
#include <pxtcp.h>
#include <assert.h>
#include <pthread.h>
#include <tenant.h>
#include <pxmqtt.h>
#include <protoex.h>
#define NO_KAFKA_POBJ
#include <pxmhub.h>
#include <selector.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/opensslv.h>
#include <netdb.h>
#include <errno.h>
#include <byteswap.h>
#define endian_int16(x)  bswap_16(x)
#define endian_int32(x)  bswap_32(x)
#define endian_int64(x)  bswap_64(x)

#define ENFORCE_LICENSE

#ifndef XSTR
#define XSTR(s) STR(s)
#define STR(s) #s
#endif

xUNUSED static char * versionString = "version_string: libimabridge.so " XSTR(ISM_VERSION) " " XSTR(BUILD_LABEL) " " XSTR(ISMDATE) " " XSTR(ISMTIME);

#define MAX_MQTT_SERVERS 16

/*
 * Get the version of libimaproxy.so
 */
const char * ism_bridge_getVersion(void) {
    return versionString;
}

/*
 * Forwarder object
 *
 * This describes both the config object for a forwarder and the operational object
 */
typedef struct ism_forwarder_t {
    char structid [8];             /* BrFwd */
    struct ism_forwarder_t * next; /* Single linked list, new ones are added at the end */
    const char * name;             /* The name of the forwarder */
    pthread_spinlock_t lock;
    int32_t   instances;           /* How many instances */
    int32_t   instof;              /* Which instance */
    int       topicCount;          /* Count of used topic names */
    uint8_t   enabled;             /* Config enabled value */
    uint8_t   active;              /* Forwarder state (modified only when holding the bridgelock */
    uint8_t   subQoS;
    uint8_t   need;                /* Need updates if active (or on next active) */
    uint8_t   evst_need;            /* Need bits for Event Streams */
    uint8_t   evst_ver;             /* Event Streams API version */
    uint8_t   evst_dest;            /* Has Event Streams destination */
    uint8_t   evst_partrule;       /* Partition rule type */
    uint32_t  evst_part;           /* Which partition for preset partition */

    const char * source;           /* source connection */
    const char * destination;      /* destination connection */
    const char * topic [16];       /* Topic names to subscribe (at most 16) */
    const char * selectors;        /* Selector source (not yet implemented) */
    ismRule_t * selector;          /* Compiled selector (not yet implemented) */
    uint32_t    selector_len;      /* Length of the selector for when we need to copy it */
    uint32_t    resvii;
    uint32_t    rulecount;         /* Event Streams rule count */
    uint32_t    rulealloc;         /* Event Streams rules allocated */
    const char * * rules;          /* Event Streams rules */
    const char * * rulenames;      /* Event Streams rule names (kafka topic) */
    ism_prop_t * props;            /* Properties for selection */
    const char * topicmap;         /* Topic mapper */
    const char * keymap;           /* MHub key mapper */
    const char * partitionmap;     /* MHub partition mapper */
    struct ism_mhub_t * mhub;      /* Associated MHub object */

    int          source_rc;        /* Last RC for the source as an ISMRC */
    int          dest_rc;          /* Last RC for the destination as an ISMRC */
    const char * source_reason;    /* Last reason for the source */
    const char * dest_reason;      /* Last reason for the destination */
    ism_transport_t * s_transport; /* Source transport object (valid when active==BCS_Active) */
    ism_transport_t * d_transport; /* Destination transport object (valid when active==BCS_Active) */
    int          subcount;         /* Count of subscriptions */
    int          retrycount[2];    /* Retry connection count 0=source 1=dest */
    uint8_t      retryLogged[2];   /* Whether we have logged the connection failure 0=source 1=dest */
    uint8_t      resvx[2];
    ism_time_t   waitfrom;         /* Min time to wait for reconfigure */
    ism_time_t   waituntil;        /* Min time to wait for a retry with same config */

    /* Forwarder stats */
    uint64_t     source_msgs;
    uint64_t     source_bytes;
    uint64_t     dest_msgs;
    uint64_t     dest_bytes;
    uint64_t     error_count;
} ism_forwarder_t;

/*
 * Connection object
 */
typedef struct ism_connection_t {
    char structid [8];               /* "BrConn" */
    struct ism_connection_t * next;
    const char * name;
    uint8_t      last_good;          /* 0=ipaddr, 1=backup */
    uint8_t      useTLS;             /* 0=none, 1=fast, 2=best */
    uint8_t      serverKind;         /* 0=MessageSight, 1=Kafka, 2=SQS, 3=bridge_connection, 5=mhub */
    uint8_t      disabled;           /* Not used */
    uint32_t     port;               /* server port number */
    pthread_spinlock_t  lock;
    struct ssl_ctx_st * tlsCTX;      /* TLS Context for this connection object */
    ism_getAddress_f    getAddress;  /* Method to get addrs when this is cast to ism_server_t */
    /* The fields before this point must match the ism_server_t object in tenant.h */

    const char * serverlist [16];    /* Server list */
    uint32_t     servercount;        /* Count of servers */
    uint8_t      version;            /* MQTT version */
    uint8_t      bridge;             /* Use bridge protocol */
    uint8_t      secure;             /* TLS version or none */
    uint8_t      isEventStream;      /* 0=mqtt, 1=EventStreams */
    const char * ciphers;            /* Ciphers to use in openssl form.  If NULL use default for TLS version */
    const char * clientID;           /* Prefix for clientID */
    const char * keystore;           /* File containing client certs (relative to base keystore) */
    const char * username;
    const char * password;
    const char * serverName;         /* The SNI value for the connection  */
    int   sessionExpiry;
    int   maxPacketSize;
    int   maxBatchTimeMS;
    int   resvi;
    int   rc;                       /* Failure RC as ISMRC */
    const char * reason;            /* Failure reason string */
} ism_connection_t;

/*
 * Connection states
 */
enum br_conn_state_e {
    BCS_None           = 0x00,        /* Configured and not yet started */
    BCS_Active         = 0x01,
    BCS_Failed         = 0x02,        /* Failed and will not restart */
    BCS_Deleted        = 0x03,        /* No longer exists, is unliked from forwarder list */
    BCS_Wait           = 0x04,        /* Wait for a retry time */
    BCS_CleanSource    = 0x05,        /* Connect in V3 to clean the session */
    BCS_ConnectDest    = 0x06,        /* Connecting the destination */
    BCS_ConnectSource  = 0x07,        /* Connection the source */
    BCS_Subscribe      = 0x08,        /* Sending subscriptions */
    BCS_Disabling      = 0x09,        /* Opening to disable */
    BCS_Disabled       = 0x0A         /* Completed disable */
};

static const char * bridge_state_str(int active) {
    switch (active) {
    case BCS_None:          return "Config";        /* Configured and not yet started */
    case BCS_Active:        return "Active";
    case BCS_Failed:        return "Failed";        /* Failed and will not restart */
    case BCS_Deleted:       return "Deleted";       /* No longer exists, is unliked from forwarder list */
    case BCS_Wait:          return "Wait";          /* Wait for a retry time */
    case BCS_CleanSource:   return "Reset";         /* Connect in V3 to clean the session */
    case BCS_ConnectDest:   return "ConnectDest";   /* Connecting the destination */
    case BCS_ConnectSource: return "ConnectSrc";    /* Connection the source */
    case BCS_Subscribe:     return "Subscribe";     /* Subscribing */
    case BCS_Disabling:     return "Disabling";
    case BCS_Disabled:      return "Disabled";
    }
    return "Unknown";
}

/*
 * Load a big endian 2 byte integer
 */
#define BIGINT16(p) (((int)(uint8_t)(p)[0]<<8) | (uint8_t)(p)[1])

#define BILLION 1000000000L

/*
 * Parse the various mqtt message/action types.
 *
 * Note that the pointers are into the original message buffer.
 * This is a synchronous only structure.
 */
typedef struct mqttbrMsg_t {
    ism_transport_t * transport;
    int             payload_len;
    int             topic_len;
    const char *    payload;        /* The message body       */
    const char *    topic;
    const char *    props;
    int             prop_len;
    int             prop_needed;
    uint8_t         available;
    uint8_t         maxqos;
    uint8_t         qos;
    uint8_t         kind;
    uint8_t         resvi[6];
    uint16_t        topic_alias;
    uint32_t        receive_max;
    char *          topic_alias_loc;
    uint32_t        packetid;
    uint32_t        maxPacketSize;
    uint32_t        sessionExpiry;
    uint32_t        keepAlive;
    const char *    reason;
} mqttbrMsg_t;

#define AVAIL_Retain    1
#define AVAIL_Wildcard  2
#define AVAIL_SubID     4
#define AVAIL_Shared    8
#define AVAIL_All     0xF

/*
 * The MQTT protocol specific area of the transport object
 */
typedef struct ism_protobj_t {
    ism_transport_t *  transport;
    ism_forwarder_t *  forwarder;
    volatile int       closed;            /* Connection is not is use */
    uint8_t            clean;             /* Disconnect is clean      */
    uint8_t            cleansession;      /* Clean session indicator (0 - durable client, 1 - non-durable) */
    uint8_t            startState;        /* Start state 0=not yet, 1=in progress, 2=stolen, 3=done */
    uint8_t            mqtt_version;      /* 3=3.1, 4=3.1.1 5=5          */
    uint8_t            isSource;          /* True if this is the source connection, false if destination */
    uint8_t            connectPending;
    uint8_t            disabling;
    uint8_t            resv[1];
    pthread_spinlock_t lock;
    ism_time_t         restart_time;
    ism_transport_t *  source_transport;  /* transport of the source if this is the dest */
    ism_transport_t *  dest_transport;    /* transport of the dest if this is the source */
    char *             senddata;          /* Saved data to be sent */
    uint32_t           senddata_len;      /* Length of sent data */
    uint32_t           senddata_alloc;    /* Buffer allocated for sent data */
    uint32_t           keepalive;         /* Keepalive in seconds */
    uint32_t           maxPacketSize;     /* client max packet size */
    uint8_t            maxqos;
    uint8_t            mqtt_rc;
    uint8_t            available;         /* */
    uint8_t            resvx[1];
    uint32_t           receiveQuota;
    uint32_t           receiveMax;
    /* TODO: topic alias is not yet implemented */
    uint16_t           topicalias_count_src;
    uint16_t           topicalias_count_dest;
    const char * *     topicalias_src;
    const char * *     topicalias_dest;
} ism_protobj_t;
typedef ism_protobj_t mqttbr_pobj_t;

/* Global lock for bridge config objects */
static pthread_mutex_t     bridgelock;

ism_forwarder_t * ismForwarders = NULL;     /* List of forwarder objects */
ism_connection_t * ismConnections = NULL;   /* List of connection objects */
static int     ismForwardersCount = 0;
static int     ismConnectionsCount = 0;
static mqtt_prop_ctx_t * g_ctx5;            /* MQTTv5 property context object */
ism_tenant_t * g_bridge_tenant = NULL;
extern int g_shuttingDown;
int g_need_dyn_write = 0;
extern int g_licensedUsage;
extern ism_enumList enum_licenses [];
extern int g_dynamic_loglevel;
extern int g_dynamic_tracelevel;
static int g_keepalive = 0;
/*
 * External and forward definitions
 */
static int restartTimer(ism_timer_t key, ism_time_t now, void * userdata);
static void appendConnectData(ism_transport_t * transport, char * buf, int length, int kind);
struct ssl_ctx_st * ism_transport_clientTlsContext(const char * name, const char * tlsversion, const char * cipher);
int ism_mqtt_reasonCodeAllowed(int rc);  /* Allowed RC in MQTTv5 */
int ism_bridge_startActions(void);
int ism_bridge_connection(ism_transport_t * transport);
static int  replaceString(const char * * oldval, const char * val);
ism_connection_t * ism_bridge_getConnection(const char * name);
mqtt_prop_ctx_t * ism_proxy_getMqttContext(int version);
static int createMqttConnectPacket(concat_alloc_t * buf, ism_forwarder_t * forwarder,
        ism_connection_t * connection, const char * clientID, int clean, int keepalive);
extern void ism_tcp_init_transport(ism_transport_t * transport);
int  ism_proxy_createMQTTConnection(ism_transport_t * transport, const char * servername);
static int mpropCheck(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value);
int ism_proxy_mapToIsmRC(int mqttrc, int version);
int ism_proxy_mapToMqttRC(int ismrc, int version);
static void unlinkForwarder(ism_forwarder_t * rfwd);
void ism_bridge_getForwarderStatus(concat_alloc_t * buf, ism_forwarder_t * forwarder);
const char * ism_mqtt_mqttCommand(int ixin);
int ism_transport_addMqttFrame(ism_transport_t * transport, char * buf, int len, int command);
ism_forwarder_t * ism_bridge_getForwarder(const char * name);
static int makeForwarderFrom(ism_forwarder_t * forwarder, int which);
int ism_bridge_makeForwarder(ism_json_parse_t * parseobj, int where, const char * name, int checkonly, int keepgoing);
int ism_bridge_getForwarderList(const char * match, ism_json_t * jobj, int json, const char * name);
void ism_bridge_getForwarderJson(ism_json_t * jobj, ism_forwarder_t * forwarder, const char * name);
ism_connection_t * ism_bridge_getConnection(const char * name);
int ism_bridge_makeConnection(ism_json_parse_t * parseobj, int where, const char * name, int checkonly, int keepgoing);
int ism_bridge_getConnectionList(const char * match, ism_json_t * jobj, int json, const char * name);
void ism_bridge_getConnectionJson(ism_json_t * jobj, ism_connection_t * connection, const char * name);
static void completeQoS2NotPresent(ism_transport_t * transport);
static void completeQoS2Present(ism_transport_t * transport, ism_transport_t * dtransport);
int ism_mqtt_propgen(ism_prop_t * xprops, ism_emsg_t * emsg, const char * name, ism_field_t * f, void * extra);
extern void ism_protocol_ensureBuffer(concat_alloc_t * buf, int len);
int  ism_tenant_createObfus(const char * user, const char * pw, char * buf, int buflen, int otype);
const char * ism_transport_makepw(const char * data, char * buf, int len, int dir);
void ism_bridge_setLastGoodAddress(ism_connection_t * connection, int trynext);
int ism_mqtt_mpropSet(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value);
void ism_tenant_init(void);
void makeBridgeTenant(void);
static int createMHubDest(ism_timer_t key, ism_time_t timestamp, void * userdata);
ism_mhub_t * ism_mhub_newMhub(const char * name, ism_tenant_t * tenant, int version);
void ism_mhub_lock(ism_mhub_t * mhub);
void ism_mhub_unlock(ism_mhub_t * mhub);
int ism_mhub_updateSelRules(ism_mhub_t * mhub, const char * * rulenames, const char * * rules, int count);
int ism_mhub_init(void);
int ism_mhub_start(void);
int ism_mhub_selectMessages(ism_mhub_t * mhub, uint16_t * topicix, int count, const char * type,
        const char * event, mqtt_pmsg_t * pmsg);
int ism_kafka_makeKafkaHeaders(ism_transport_t * transport, concat_alloc_t * buf, mqtt_pmsg_t * pmsg,
        concat_alloc_t * sysprops, concat_alloc_t * userprops, int msgver);
int ism_mhub_publishEvent(ism_mhub_t * mhub, mqtt_pmsg_t * pmsg, const char * clientID, int topic_index, int partition);
int ism_mhub_getMaxBatchSizeBytes(void);
int ism_mhub_getMaxBatchSizeMsgs(void);
int ism_mhub_getMaxBatchTimeMillis(void);
int mapToIsmRC(int mqttrc, int version);



/*
 * Initialize bridge processing
 */
int ism_bridge_init(void) {
    pthread_mutex_init(&bridgelock, NULL);
    TRACE(3, "Initialize bridge processing\n");
    g_ctx5 = ism_proxy_getMqttContext(5);
    g_keepalive = ism_common_getIntConfig("KeepAlive", 300);
    makeBridgeTenant();
    ism_mhub_init();
    return 0;
}

/*
 * Start bridge processing.
 * This is done at start messaging.
 */
int ism_bridge_start(void) {
    TRACE(3, "Start bridge processing\n");
    /* Run the restart and keepalive timer every 2 seconds */
    ism_common_setTimerRate(ISM_TIMER_LOW, restartTimer, NULL, 10, 2, TS_SECONDS);

    /* Start any required actions */
    ism_bridge_startActions();

    ism_mhub_start();

    return 0;
}

/*
 * Terminate the bridge
 */
int ism_bridge_term(void) {
    TRACE(3, "Terminate bridge processing\n");
    return 0;
}

/*
 * Externalize the bridge lock
 */
void ism_bridge_lock(void) {
    pthread_mutex_lock(&bridgelock);
}

/*
 * Externalize unlock of the bridge lock
 */
void ism_bridge_unlock(void) {
    pthread_mutex_unlock(&bridgelock);
}

/*
 * Make a fake tenant for MHub processing
 */
void makeBridgeTenant(void) {
    ism_tenant_t * tenant;

    ism_tenant_init();
    tenant = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_br_misc,2),1, sizeof(ism_tenant_t));
    strcpy(tenant->structid, "IoTTEN");
    tenant->name = "_Bridge";
    tenant->namelen = (int)strlen(tenant->name);
    pthread_spin_init(&tenant->lock, 0);
    g_bridge_tenant = tenant;
}

static ism_endstat_t outStat = { 0 };
/*
 * Dummy endpoint so we do not segfault
 */
static ism_endpoint_t bridgeEndpoint = {
    .name        = "!Bridge",
    .ipaddr      = "",
    .transport_type = "TCP",
    .protomask   = PMASK_Internal,
    .thread_count = 1,
    .stats = &outStat,
};

/*
 * Create an outgoing TCP connection
 * Set up the transport object and call transport to start the connection
 *
 * This is called both on a timer and directly, when directly the key and timestamp are zero.
 */
static int createConnect(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    char * clientID;
    int    clientIDlen;
    ism_transport_t * transport;
    char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 16};
    ism_forwarder_t * forwarder = userdata;
    ism_connection_t * connection = NULL;
    int isdest = 0;
    int clean = 0;
    int keepalive = g_keepalive;
    int rc;
    const char * connection_name;

    /* If this is a timer, it is only run once */
    if (key)
        ism_common_cancelTimer(key);

    if (g_shuttingDown)
        return 0;

    /* Determine the setting of CleanStart / CleanSession bit */
    if (forwarder->active == BCS_CleanSource || forwarder->active == BCS_ConnectSource) {
        connection_name = forwarder->source;
        if (forwarder->active == BCS_CleanSource)
            clean = 1;
    } else {
        connection_name = forwarder->destination;
        isdest = 1;
    }

    if (!connection_name)
        connection_name = "NotSet";    /* This will generate an error */
    else
        connection = ism_bridge_getConnection(connection_name);

    /* If the named connection is not found, set state to Failed, only a config change will cause a retry */
    if (!connection) {
        forwarder->active = BCS_Failed;
        LOG(NOTICE, Server, 984, "%s%s", "The state of forwarder {0} is now: {1}",
                forwarder->name, bridge_state_str(forwarder->active));
        rc = ISMRC_NotFound;
        if (isdest) {
            forwarder->dest_rc = rc;
            snprintf(xbuf, sizeof xbuf, "Connection object not found: Connection=%s Forwarder=%s",
                    connection_name, forwarder->name);
            replaceString(&forwarder->dest_reason, xbuf);
        } else {
            forwarder->source_rc = rc;
            snprintf(xbuf, sizeof xbuf, "Connection object not found: Connection=%s Forwarder=%s",
                    forwarder->name, forwarder->source);
            replaceString(&forwarder->source_reason, xbuf);
        }
        ism_common_setErrorData(rc, "%s%s", connection_name, forwarder->name);
        return rc;
    }

    /* Make sure the connection object is complete.  If not, set the state to Failed and retry only if the config changes */
    if (connection->servercount > 0 && !connection->clientID) {
        TRACE(6, "Connection=%s server0=%s server1=%s clientID=%s\n", connection->name,
                connection->serverlist[0], (connection->servercount ? connection->serverlist[1] : ""),
                connection->clientID);
        forwarder->active = BCS_Failed;
        TRACE(6, "Change forwarder state: forwarder=%s state=%s\n", forwarder->name, bridge_state_str(forwarder->active));
        LOG(NOTICE, Server, 984, "%s%s", "The state of forwarder {0} is now: {1}",
                forwarder->name, bridge_state_str(forwarder->active));
        if (isdest) {
            rc = forwarder->dest_rc = ISMRC_InvalidObjectConfig;
            snprintf(xbuf, sizeof xbuf, "Connection object not complete: Connection=%s Forwarder=%s",
                    connection_name, forwarder->destination);
            replaceString(&forwarder->dest_reason, xbuf);
        } else {
            rc = forwarder->source_rc = ISMRC_InvalidObjectConfig;
            snprintf(xbuf, sizeof xbuf, "Connection object not complete: Connection=%s Forwarder=%s",
                    connection_name, forwarder->source);
            replaceString(&forwarder->source_reason, xbuf);
        }
        ism_common_setErrorData(rc, "%s%s", connection_name, forwarder->name);
        return rc;
    }

    /* Construct the clientID */
    clientIDlen = strlen(connection->clientID) + strlen(forwarder->name);
    clientID = alloca(clientIDlen+1);
    strcpy(clientID, connection->clientID);
    strcat(clientID, forwarder->name);

    /* Create outgoing CONNECT packet */
    createMqttConnectPacket(&buf, forwarder, connection, clientID, clean, keepalive);

    /*
     * Fill in the transport object
     *
     * We determine that the transport object is TCP transport, MQTT frame, and bridge protocol
     */
    transport = ism_transport_newOutgoing(&bridgeEndpoint, 1);         /* Allocate the transport   */
    ism_tcp_init_transport(transport);                      /* Set TCP as the transport */
    transport->name = ism_transport_putString(transport, clientID);  /* Give the transport object a name */
    transport->clientID = ism_transport_putString(transport, clientID);
    /*
     * Set the protocol based on the MQTT version.  All of these will
     * go to the bridge protocol, so this is just used as info in the log.
     */
    switch (connection->version) {
    case 3:  transport->protocol = "mqtt-tcp";   break;
    case 5:  transport->protocol = "mqtt5-tcp";  break;
    default:
    case 4:  transport->protocol = "mqtt4-tcp";  break;
    }
    ism_bridge_connection(transport);        /* Fill in protocol fields */

    /* Save the connect packet to send when the connection is complete */
    appendConnectData(transport, buf.buf+16, buf.used-16, MT_CONNECT<<4);

    /* Fill in more transport fields */
    transport->pobj->mqtt_version = connection->version;
    transport->originated = 1;
    transport->pobj->connectPending = 1;
    transport->server = (ism_server_t *)connection;
    transport->pobj->forwarder = forwarder;
    transport->pobj->keepalive = keepalive;
    transport->pobj->maxqos = 2;
    transport->pobj->available = AVAIL_All;
    if (connection->username)
        transport->userid = ism_transport_putString(transport, connection->username);
    if (isdest) {
        forwarder->d_transport = transport;
    } else {
        transport->pobj->isSource = 1;
        forwarder->s_transport = transport;
        transport->pobj->dest_transport = forwarder->d_transport;
    }

    TRACE(6, "Make outgoing bridge connection: connect=%u clientID=%s\n",
            transport->index, transport->name);

    /* Do the connect, this completes in ism_mqtt_connected()  */
    transport->ready = 0;    /* Client connection authorized */
    rc = ism_proxy_createMQTTConnection(transport, connection->serverName);


    TRACE(7, "Create connection: forwarder=%s connection=%s dest=%u connect=%u client=%s rc=%d\n",
            forwarder->name, connection->name, isdest, transport->index, transport->name, rc);

    /* Check synchronous errors in making a connection */
    if (rc) {
        ism_common_formatLastError(xbuf, sizeof xbuf);
        if (isdest) {
            forwarder->dest_rc = rc;
            replaceString(&forwarder->dest_reason, xbuf);
        } else {
            forwarder->source_rc = rc;
            replaceString(&forwarder->source_reason, xbuf);
        }
        ism_common_setError(rc);
        transport->close(transport, rc, 0, xbuf);
    }
    return rc;
}

/*
 * Notification of the outgoing connection complete.
 *
 * This is the transport->connected callback for a bridge connection
 * If there is a saved packet send it at this point.
 */
int ism_bridge_connected(ism_transport_t * transport, int rc) {
    if (__sync_bool_compare_and_swap(&transport->pobj->connectPending, 1, 0)) {
        TRACE(7, "Outgoing bridge connection complete: connect=%d ip=%s port=%u rc=%d senddata=%d\n",
                transport->index, transport->server_addr, transport->serverport, rc, transport->pobj->senddata_len);
        if (rc == 0) {
            mqttbr_pobj_t * pobj = transport->pobj;
            transport->state = ISM_TRANST_Open;
            if (pobj->senddata_len) {
                transport->send(transport, pobj->senddata, pobj->senddata_len, 0, SFLAG_HASFRAME);
                ism_common_free(ism_memory_proxy_br_misc,pobj->senddata);
            }
            pobj->senddata = NULL;
            pobj->senddata_len = 0;
            pobj->senddata_alloc = 0;
            transport->ready = 5;   /* TCP and TLS connection, but no MQTT connection */
        }

        if (rc) {
            ism_forwarder_t * forwarder = transport->pobj->forwarder;
            /* Try the next server */
            ism_bridge_setLastGoodAddress((ism_connection_t *)transport->server, transport->connect_order+1);
            int isdest = !transport->pobj->isSource;
            if (forwarder->retrycount[isdest] > 2 && !forwarder->retryLogged[isdest]) {
                forwarder->retryLogged[isdest] = 1;
                LOG(WARN, Server, 982, "%s%s%s", "Unable to connect to MQTT server: Forwarder={0} Name={1} Server={2}",
                        forwarder->name, transport->name, transport->client_host ? transport->client_host : transport->server_addr);
            } else if (forwarder->retrycount[isdest]) {
                LOG(NOTICE, Server, 982, "%s%s%s", "Unable to connect to MQTT server: Forwarder={0} Name={1} Server={2}",
                        forwarder->name, transport->name, transport->client_host ? transport->client_host : transport->server_addr);
            }
            forwarder->retrycount[isdest]++;
        }
        return 0;
    }
    return 1;
}

/*
 * Check if a keepalive is needed
 */
static void checkKeepalive(ism_transport_t * transport) {
    char xbuf[32];
    double now = ism_common_readTSC();
    if (transport) {
        if (transport->state == ISM_TRANST_Open) {
            if (transport->ready==1 && transport->pobj->keepalive) {
                if (now > (transport->lastAccessTime + transport->pobj->keepalive)) {
                    /* We can send from any thread, the actual send will be moved to an IoP */
                    transport->send(transport, xbuf+16, 0, MT_PINGREQ<<4, SFLAG_FRAMESPACE);
                    transport->lastAccessTime = ism_common_readTSC();
                }
            }
        }
    }
}


/*
 * Implement a timer to check for keepalive and restart
 */
static int restartTimer(ism_timer_t key, ism_time_t now, void * userdata) {
    ism_forwarder_t * forwarder;
    pthread_mutex_lock(&bridgelock);
    TRACE(8, "Run retry and keepalive checker\n");
    forwarder = ismForwarders;

    /* Check all forwarders */
    while (forwarder) {
        pthread_spin_lock(&forwarder->lock);
        if (forwarder->active == BCS_Wait) {
            if (now > forwarder->waituntil) {
                forwarder->active = BCS_ConnectDest;
                TRACE(6, "Change forwarder state: forwarder=%s state=%s\n",
                            forwarder->name, bridge_state_str(forwarder->active));
                /* Schedule a timer to open the dest connection */
                forwarder->evst_need = forwarder->need;
                forwarder->need = 0;
                if (forwarder->evst_dest) {
                    ism_common_setTimerOnce(ISM_TIMER_LOW, createMHubDest, forwarder, 1000);
                } else {
                    ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)createConnect, forwarder, 1000);
                }
            }
        }
        /* For active forwarders, check keepalive */
        if (forwarder->active == BCS_Active) {
            if (!forwarder->evst_dest)
                checkKeepalive(forwarder->d_transport);
            checkKeepalive(forwarder->s_transport);
        }
        pthread_spin_unlock(&forwarder->lock);
        forwarder = forwarder->next;
    }
    pthread_mutex_unlock(&bridgelock);
    return 1;    /* Continue the timer */
}

/*
 * Fix the number of instances after a config change
 */
int fixInstances(ism_forwarder_t * base) {
    ism_forwarder_t * forwarder;
    ism_transport_t * transport;
    int  i;
    int  namelen = strlen(base->name);
    char * name = alloca(namelen + 3);
    strcpy(name, base->name);
    strcat(name, "00");
    int startdisable;

    if (base->need & 0x03) {
        /* Make sure no instances exist and start from scratch */
        startdisable = 0;
    } else {
        startdisable = base->instances;
    }


    /* Make sure that instances above the instance count do not exist or are disabled */
    for (i=startdisable; i<100; i++) {
        name[namelen] = '0' + (i/10);
        name[namelen+1] = '0' + (i%10);
        forwarder = ism_bridge_getForwarder(name);
        if (forwarder && forwarder->active != BCS_Disabled) {
            forwarder->active = BCS_Disabled;
            /* Close the destination connection which will also cause the source to close */
            transport = forwarder->s_transport;
            if (!transport && !forwarder->evst_dest)
                transport = forwarder->d_transport;
            if (transport)
                transport->close(transport, ISMRC_EndpointDisabled, 0, "Connection closed as the forwarder is disabled");
        }
        unlinkForwarder(forwarder);
    }

    /* Create or activate instances below the instance count */
    for (i=0; i<base->instances; i++) {
        name[namelen] = '0' + (i/10);
        name[namelen+1] = '0' + (i%10);
        forwarder = ism_bridge_getForwarder(name);
        if (!forwarder)
            makeForwarderFrom(base, i);
        else if (forwarder->active == BCS_Disabled) {
            forwarder->active = BCS_None;
        }
    }
    return 0;
}


/*
 * Start actions
 *
 * Check after a config change what actions need to be updated
 *
 * Any actual work is done on a timer thread
 */
int ism_bridge_startActions(void) {
    uint64_t delay_ms = 1000000;
    ism_forwarder_t * forwarder;
    ism_transport_t * transport;
    pthread_mutex_lock(&bridgelock);
    forwarder = ismForwarders;
    ism_time_t now = ism_common_currentTimeNanos();

    TRACE(5, "Start bridge actions\n");
    forwarder = ismForwarders;
    while (forwarder) {
        /* Need instance check */
        if (forwarder->need & 0x04) {
            if (forwarder->instof < 0)
                fixInstances(forwarder);
            forwarder->need &= ~4;
        }
        forwarder = forwarder->next;
    }

    forwarder = ismForwarders;
    while (forwarder) {
        if (forwarder->need) {
            if (forwarder->active == BCS_Active) {
                if (!forwarder->enabled) {
                    forwarder->s_transport->pobj->disabling = 1;
                    if (!forwarder->evst_dest)
                        forwarder->d_transport->pobj->disabling = 1;
                    forwarder->need = 0;
                    forwarder->active = BCS_Disabled;
                    /* Close the destination connection which will also cause the source to close */
                    transport = forwarder->s_transport;
                    transport->close(transport, ISMRC_EndpointDisabled, 0, "Connection closed as the forwarder is disabled");
                } else {
                    /* Close the destination connection which will also cause the source to close */
                    transport = forwarder->s_transport;
                    transport->close(transport, ISMRC_EndpointDisabled, 0, "Connection closed due to a config change.");
                    forwarder->d_transport = NULL;
                    forwarder->s_transport = NULL;
                    if (forwarder->need & 2) {
                        forwarder->active = BCS_CleanSource;
                        TRACE(6, "Change forwarder state: forwarder=%s state=%s\n",
                                forwarder->name, bridge_state_str(forwarder->active));
                    } else {
                        forwarder->active = BCS_ConnectDest;
                        TRACE(6, "Change forwarder state: forwarder=%s state=%s\n",
                                forwarder->name, bridge_state_str(forwarder->active));
                    }
                    /* Schedule a timer to open the dest connection */
                    forwarder->need = 0;
                    if (forwarder->active == BCS_ConnectDest && forwarder->evst_dest) {
                        ism_common_setTimerOnce(ISM_TIMER_LOW, createMHubDest, forwarder, 2*delay_ms);
                    } else {
                        ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)createConnect, forwarder, 2*delay_ms);
                    }
                }
            } else if (forwarder->active == BCS_None ||
                    forwarder->active == BCS_Failed ||
                    (forwarder->active == BCS_Wait && now > forwarder->waitfrom)) {
#ifdef ENFORCE_LICENSE
                if (g_licensedUsage == LICENSE_None && forwarder->enabled) {
                    if (forwarder->source_rc != ISMRC_LicenseError) {
                        forwarder->source_rc = ISMRC_LicenseError;
                        replaceString(&forwarder->source_reason, "IBM MessageSight is not fully functional until you accept the license agreement.");
                    }
                } else
#endif
                if (forwarder->enabled && forwarder->source && forwarder->destination && forwarder->topicCount) {
                    if (forwarder->active == BCS_None) {
                        ism_connection_t * dest = ism_bridge_getConnection(forwarder->destination);
                        ism_connection_t * src = ism_bridge_getConnection(forwarder->source);
                         if (!dest || !src)
                             goto notconfig;
                         if (dest->isEventStream) {
                             forwarder->evst_dest = 1;
                             if (forwarder->rulecount == 0)
                                 goto notconfig;
                         }
                    }
                    if (forwarder->instances > 0 && forwarder->instof < 0)
                        goto notconfig;
                    forwarder->active = BCS_ConnectDest;
                    TRACE(6, "Change forwarder state: forwarder=%s state=%s\n",
                            forwarder->name, bridge_state_str(forwarder->active));
                    /* Schedule a timer to open the dest connection */
                    forwarder->evst_need = forwarder->need;
                    forwarder->need = 0;
                    if (forwarder->evst_dest) {
                        ism_common_setTimerOnce(ISM_TIMER_LOW, createMHubDest, forwarder, 1000);
                    } else {
                        ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)createConnect, forwarder, 1000);
                    }
                } else {
                    /* Config not complete so we cannot start it */
                notconfig:
                    forwarder->active = BCS_None;
                    forwarder->need = 0;
                }
            }
        }
        forwarder = forwarder->next;
    }
    pthread_mutex_unlock(&bridgelock);
    return 0;
}


/*
 * If we change the license to or from None, then set need=1 on all forwarders
 */
void ism_bridge_changeLicense(int old, int new) {
    ism_forwarder_t * forwarder;

    if (old == LICENSE_None || new == LICENSE_None) {
        pthread_mutex_lock(&bridgelock);
        forwarder = ismForwarders;
        while (forwarder) {
            forwarder->need |= 1;
            if (new != LICENSE_None && forwarder->source_rc == ISMRC_LicenseError) {
                forwarder->source_rc = 0;
                replaceString(&forwarder->source_reason, NULL);
            }
            forwarder = forwarder->next;
        }
        pthread_mutex_unlock(&bridgelock);
        ism_bridge_startActions();
    }
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
 * Append new data to the saved data
 */
static void appendConnectData(ism_transport_t * transport, char * buf, int length, int kind) {
    int flen = ism_transport_addMqttFrame(transport, buf, length, kind|0x100);
    buf -= flen;
    length += flen;
    mqttbr_pobj_t * pobj = transport->pobj;

    pthread_spin_lock(&transport->lock);
    if (!pobj->senddata) {
        pobj->senddata_alloc = ((kind>>4)==MT_CONNECT) ? length : 4096;
        pobj->senddata = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_br_misc,4),pobj->senddata_alloc);
        pobj->senddata_len = length;
        memcpy(pobj->senddata, buf, length);
    } else {
        if (pobj->senddata_len + length > pobj->senddata_alloc) {
            while (pobj->senddata_len + length > pobj->senddata_alloc)
                pobj->senddata_alloc *= 2;
            pobj->senddata = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_br_misc,5),pobj->senddata, pobj->senddata_alloc);  /* assume we do not fail */
        }
        memcpy(pobj->senddata+pobj->senddata_len, buf, length);
        pobj->senddata_len += length;
    }
    pthread_spin_unlock(&transport->lock);
}


/*
 * Create an MQTT connection packet.
 *
 * This is done before the connection is created and is sent on TCP connect.
 * We do not support Will Messages
 */
static int createMqttConnectPacket(concat_alloc_t * buf, ism_forwarder_t * forwarder,
        ism_connection_t * connection, const char * clientID, int clean, int keepalive) {
    int flags = 0;
    int clientID_len;
    int username_len;
    int password_len;

    /* Compute flags byte */
    if (connection->sessionExpiry == 0 || clean)
        flags |= CFLAG_Clean;
    if ((connection->username) ||
        (connection->password && connection->version < 5))
        flags |= CFLAG_UserName;
    if (connection->password)
        flags |= CFLAG_Password;

    /*
     * Initial string and version
     */
    if (connection->version == 3) {
        ism_common_allocBufferCopyLen(buf, "\0\6MQIsdp\3", 9);
    } else {
        ism_common_allocBufferCopyLen(buf, "\0\4MQTT", 6);
        bputchar(buf, (char)connection->version);
    }
    bputchar(buf, (char)flags);             /* Connection flags */
    bputchar(buf, (char)(keepalive>>8));    /* Keepalive */
    bputchar(buf, (char)keepalive);

    /* Add MQTTv5 properties */
    if (connection->version >= 5) {
        /* Assume all connect metadata is < 128 bytes */
        int metapos = buf->used++;
        uint32_t expire = connection->sessionExpiry;
        if (expire)
            ism_common_putMqttPropField(buf, MPI_SessionExpire, g_ctx5, expire);
        ism_common_putMqttPropField(buf, MPI_MaxTopicAlias, g_ctx5, 0);     /* No topic alias for now */
        if (connection->maxPacketSize)
            ism_common_putMqttPropField(buf, MPI_MaxPacketSize, g_ctx5, connection->maxPacketSize);
        buf->buf[metapos] = (char)(buf->used-metapos-1);
    }

    /* Add clientID */
    clientID_len = strlen(clientID);
    bputchar(buf, (char)clientID_len>>8);
    bputchar(buf, (char)clientID_len);
    ism_common_allocBufferCopyLen(buf, clientID, clientID_len);

    /* Add username */
    if (flags & CFLAG_UserName) {
        username_len = connection->username ? strlen(connection->username) : 0;
        bputchar(buf, (char)(username_len>>8));
        bputchar(buf, (char)username_len);
        if (username_len)
            ism_common_allocBufferCopyLen(buf, connection->username, username_len);
    }

    /* Add password */
    if (flags & CFLAG_Password) {
        char * pw;
        if (*connection->password == '!') {
            int outlen = strlen(connection->password);
            pw = alloca(outlen);
            ism_transport_makepw(connection->password, pw, outlen, 1);
        } else {
            pw = (char *)connection->password;
        }
        password_len = strlen(pw);
        bputchar(buf, (char)(password_len>>8));
        bputchar(buf, (char)password_len);
        ism_common_allocBufferCopyLen(buf, pw, password_len);
    }
    return 0;
}


/*
 * Parse a publish packet and validate any properties
 */
static int parsePublish(ism_transport_t * transport, mqttbrMsg_t * mmsg, uint8_t * bp, int buflen) {
    mqttbr_pobj_t * pobj = transport->pobj;
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

    /* Get the packet ID */
    if (mmsg->qos > 0) {
        if (buflen > 1) {
            mmsg->packetid = (uint16_t)BIGINT16(bp);
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
            if (mmsg->prop_len < 0 || mmsg->prop_len >= buflen) {
                ism_common_setErrorData(ISMRC_BadLength, "%s%s", "PUBLISH", "MQTTPropLen");
                return ISMRC_BadLength;
            } else {
                /* Check the properties.  If it is good we will just copy it as we got it */
                int xrc = ism_common_checkMqttPropFields((char *)bp, mmsg->prop_len,
                    g_ctx5, CPOI_S_PUBLISH, mpropCheck, NULL);
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
 * Send subscriptions
 */
static int doSubscribe(ism_transport_t * transport, ism_forwarder_t * forwarder) {
    char xbuf[4096];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 16};
    int packetid = 10000;
    int topiclen;
    int i;
    uint8_t subopt = transport->pobj->mqtt_version >= 5 ? 0x28 : 0;
    int qos = forwarder->subQoS;
    if (forwarder->evst_dest && qos > 1)
        qos = 1;                       /* QoS=2 not supported for Event Streams destination */
    if (transport->pobj->maxqos > qos)
        qos = transport->pobj->maxqos; /* QoS cannot be greater than allowd by the source server */

    forwarder->subcount = 0;
    for (i=0; i<forwarder->topicCount; i++) {
        buf.used = 16;                          /* Space for the framing to be added */
        bputchar(&buf, (char)(packetid>>8));    /* PacketID */
        bputchar(&buf, (char)packetid);
        packetid++;                             /* Use next packetID for another subscription */
        if (transport->pobj->mqtt_version >= 5)
            bputchar(&buf, 0);                  /* No properties */
        topiclen = strlen(forwarder->topic[i]);
        bputchar(&buf, (char)(topiclen>>8));    /* Topic length */
        bputchar(&buf, (char)topiclen);
        ism_common_allocBufferCopyLen(&buf, forwarder->topic[i], topiclen);
        bputchar(&buf, subopt + qos);   /* QoS and suboptions */
        transport->lastAccessTime = ism_common_readTSC();
        transport->send(transport, buf.buf+16, buf.used-16, (MT_SUBSCRIBE<<4)+2, SFLAG_FRAMESPACE);
    }
    return 0;
}


/*
 * Set the next server to try.
 * On connection success we set it to the one which is good
 * On connection failure we set it to the next one
 */
void ism_bridge_setLastGoodAddress(ism_connection_t * connection, int trynext) {
    if (connection->servercount > 1) {
        pthread_spin_lock(&connection->lock);
        int oldGood = connection->last_good;


        if (trynext >= connection->servercount)
            trynext = 0;
        connection->last_good = trynext;
        pthread_spin_unlock(&connection->lock);
        if (oldGood != trynext) {
            TRACE(4, "Set server to try next: connection=%s old=%d new=%d\n", connection->name, oldGood, trynext);
        }
    }
}

/*
 * Handle the receive of a CONNACK
 *
 * This can come from either source or destination
 */
static int doConnack(ism_transport_t * transport, uint8_t * bp, int buflen, int kind) {
    int rc = 0;
    int present = 0;
    int mqttrc = 0;
    mqttbrMsg_t mmsg = {transport};
    mmsg.available = AVAIL_All;    /* Available flags default to true */
    if (buflen >= 2) {
        present = bp[0];     /* Session present byte */
        mqttrc = bp[1];      /* MQTT rc */
        bp += 2;
        buflen -= 2;

        /* Handle the MQTTv5 properties */
        if (transport->pobj->mqtt_version >= 5 && buflen) {
            int proplen = *bp;
            if (proplen & 0x80) {
                int vilen;
                proplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                bp += vilen;
                buflen -= vilen;
            } else {
                buflen--;
                bp++;
            }

            /* Minimal check of the properties plus record a few of the values in mmsg */
            mmsg.maxqos = transport->pobj->maxqos;
            if (proplen > 0 && proplen <= buflen) {
                rc = ism_common_checkMqttPropFields((char *)bp, proplen,
                        g_ctx5, CPOI_CONNACK, mpropCheck, &mmsg);
                bp += proplen;
                buflen -= proplen;
            }
            transport->pobj->available = mmsg.available;
            if (mmsg.keepAlive)
                transport->pobj->keepalive = mmsg.keepAlive;
            if (mmsg.maxqos != transport->pobj->maxqos)
                transport->pobj->maxqos = mmsg.maxqos;
            if (mmsg.maxPacketSize)
                transport->pobj->maxPacketSize = mmsg.maxPacketSize;
        }
    }
    if (buflen) {
        rc = ISMRC_BadLength;
        ism_common_setError(rc);
    }
    if (!rc) {
        if (mqttrc) {
            rc = ism_proxy_mapToIsmRC(mqttrc, transport->pobj->mqtt_version);
            ism_common_setError(rc);
            /* TODO: This really should not happen unless the server is messed up, but be should move to FAILED */
        } else {
            ism_forwarder_t * forwarder = transport->pobj->forwarder;

            LOG(INFO, Connection, 1301, "%s%d%s%s%s", "Create MQTT connection: Forwarder={0} Dest={1} Name={2} Protocol={3} Secure={4}",
                forwarder->name, !transport->pobj->isSource, transport->name, transport->protocol, ism_transport_getTLSMethodName(transport));
            /* Remember we got a good connection */
            ism_bridge_setLastGoodAddress((ism_connection_t *)transport->server, transport->connect_order);

            transport->ready = 1;
            transport->lastAccessTime = ism_common_readTSC();

            /* This can be a little long to hold a spin lock, but there should be little contention */
            pthread_spin_lock(&forwarder->lock);
            switch (forwarder->active) {
            /* We are cleaning the source, close the connection and then open the destination */
            case BCS_CleanSource:
                forwarder->active = BCS_ConnectDest;
                TRACE(6, "Change forwarder state: forwarder=%s state=%s\n",
                        forwarder->name, bridge_state_str(forwarder->active));
                transport->close(transport, 0, 1, "The connection has closed normally.");
                if (forwarder->evst_dest) {
                    ism_common_setTimerOnce(ISM_TIMER_LOW, createMHubDest, forwarder, 1000);
                } else {
                    ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)createConnect, forwarder, 1000);
                }
                break;

            /* We are connected to the destination, so connect to the source */
            case BCS_ConnectDest:
                forwarder->active = BCS_ConnectSource;
                forwarder->retrycount[1] = 0;
                forwarder->retryLogged[1] = 0;
                TRACE(6, "Change forwarder state: forwarder=%s state=%s\n",
                        forwarder->name, bridge_state_str(forwarder->active));
                ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)createConnect, forwarder, 1000);
                break;

            /* We are now connected to the source, so move the state to subscribe or active */
            case BCS_ConnectSource:
                if (!forwarder->evst_dest) {
                    forwarder->d_transport->pobj->source_transport = transport;
                    transport->pobj->dest_transport = forwarder->d_transport;
                }
                forwarder->retrycount[0] = 0;
                forwarder->retryLogged[0] = 0;
                forwarder->source_rc = 0;
                forwarder->dest_rc = 0;
                replaceString(&forwarder->source_reason, NULL);
                replaceString(&forwarder->dest_reason, NULL);
                /* No session present, do subscribes */
                if (!present) {
                    if (!forwarder->evst_dest && forwarder->d_transport->pobj->senddata_len) {
                        completeQoS2NotPresent(forwarder->d_transport);
                    }
                    forwarder->active = BCS_Subscribe;
                    TRACE(6, "Change forwarder state: forwarder=%s state=%s\n",
                            forwarder->name, bridge_state_str(forwarder->active));
                    if (!forwarder->evst_dest)
                        completeQoS2Present(transport, forwarder->d_transport);
                    rc = doSubscribe(transport, forwarder);
                } else {
                    /* No subscribes needed, move directly to active */
                    forwarder->active = BCS_Active;

                    TRACE(5, "Change forwarder state: forwarder=%s state=%s\n",
                            forwarder->name, bridge_state_str(forwarder->active));
                    LOG(NOTICE, Server, 984, "%s%s", "The state of forwarder {0} is now: {1}",
                            forwarder->name, bridge_state_str(forwarder->active));
                }
                break;
            }
            pthread_spin_unlock(&forwarder->lock);
        }
    }
    return rc;
}

/*
 * Return a part of a topic
 *
 * We return the pointer and the length but the topic part is not zero termainated
 */
static int topicpart(const char * topic, const char * * part, int which) {
    const char * tp = topic;
    const char * endp;
    int found = 0;
    while (*tp && found < which) {
        if (*tp == '/')
            found++;
        tp++;
    }
    if (*tp) {
        *part = tp;
        endp = strchr(tp, '/');
        if (endp)
            return endp-tp;
        else
            return (int)strlen(tp);
    } else {
        *part = NULL;
        return 0;
    }
}

/*
 * Put a replacement var into a buffer.
 *
 * For normal variables this is put directly into the output buffer
 * For JSON variables this is put into a temp buffer
 */
static int putVar(concat_alloc_t * buf, const char * token, int tokenlen, const char * topic, ism_forwarder_t * forwarder, mqttbrMsg_t * mmsg) {
    int rc = 0;
    if (tokenlen == 1 && *token == '$') {
        bputchar(buf, '$');
    } else if (tokenlen >= 5 && !memcmp(token, "Topic", 5)) {
        if (tokenlen == 5) {
            if (topic)
                ism_json_putBytes(buf, topic);
            else
                rc = 1;
        } else {
            /* Substitute for a topic part.  If the part ends in '*' copy the rest of the topic */
            const char * part;
            char * eos;
            int which = strtoul(token+5, &eos, 10);
            int partlen = topicpart(topic, &part, which);
            if (part)
                ism_common_allocBufferCopyLen(buf, part, *eos=='*' ? strlen(part) : partlen);
            else
                rc = 1;
        }
    } else if (tokenlen > 4 && !memcmp(token, "Time", 4) &&
            (!strcmp(token+4, "ISO") || !strcmp(token+4, "MS"))) {
        if (token[4] == 'I') {
            ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_UTC);
            ism_protocol_ensureBuffer(buf, 32);
            ism_common_formatTimestamp(ts, buf->buf + buf->used, 32, 7, ISM_TFF_ISO8601);
            ism_common_closeTimestamp(ts);
            buf->used += strlen(buf->buf+buf->used);
        } else {
            uint64_t now = ism_common_currentTimeNanos() / 1000000;
            ism_protocol_ensureBuffer(buf, 16);
            sprintf(buf->buf + buf->used, "%lu", now);
            buf->used += strlen(buf->buf+buf->used);
        }
    } else if (tokenlen == 3 && !memcmp(token, "QoS", 3)) {
        bputchar(buf, '0' + mmsg->qos);
    } else if (mmsg && forwarder && mmsg->prop_len) {
        ism_field_t f;
        pthread_spin_lock(&forwarder->lock);
        if (mmsg->prop_needed) {
            if (!forwarder->props) {
                forwarder->props = ism_common_newProperties(100);
            } else {
                ism_common_clearProperties(forwarder->props);
            }
            ism_common_checkMqttPropFields((char *)mmsg->props, mmsg->prop_len, g_ctx5, 0xFFFF, ism_mqtt_mpropSet, forwarder->props);
            mmsg->prop_needed = 0;
        }
        ism_common_getProperty(forwarder->props, token, &f);
        if (f.type == VT_String) {
            ism_json_putBytes(buf, f.val.s);
        } else {
            rc = 1;
        }
        pthread_spin_unlock(&forwarder->lock);
    }
    return rc;
}

/*
 * Simple topic or key mapper
 *
 * @param buf   An allocation buffer
 * @param topic The topic (must not be NULL)
 * @param tmp   The topic map (must not be NULL)
 * @param forwarder  The forwarder object. This can be null if properties are not used
 * @param mmsg  The message object.  This can be null if properties are not used
 *
 * Replacement values are of the form:
 *     Topic# where # is a number.  If followed by an asterisk it is the rest of the topic
 *     TimeISO.  The current UTC time as an ISO8601 string to milliseconds
 *     TimeMS    The integer count of milliseconds since 1970-01-01T00Z
 *     QoS       The QoS
 *     properties  Any other property name
 *
 * A JSON replacement value consists of the form JSON:name:field or JSON?:name:field.
 * This will put an item into the output in JSON format (correctly escaped). If the name
 * field is not zero length, the JSON item will be put out as a named item.  If the field
 * does not have a value, when the JSON form is used the value will be null, and when JSON?
 * is used no item will be added, and the preceeding whitespace and comma (if any) will
 * be removed from the output.
 *
 */
void ism_bridge_topicMapper(concat_alloc_t * buf, const char * topic, const char * tmap, ism_forwarder_t * forwarder, mqttbrMsg_t * mmsg) {
    const char * tp = tmap;
    char token [128];
    char xbuf [2048];
    const char * endp;
    int  tokenlen;
    if (mmsg) {
        mmsg->prop_needed = mmsg->prop_len;
    }

    int  nocomma = 0;
    while (*tp) {
        /* Copy any constant bytes */
        while (*tp && (*tp != '$' || tp[1] != '{')) {
            if (*tp == ',' && nocomma) {
                nocomma = 0;
            } else {
                bputchar(buf, *tp);
            }
            tp++;
        }

        /* Parse replacement variable */
        if (*tp == '$') {
            endp = strchr(tp+1, '}');
            if (!endp)
                endp = tp+strlen(tp);
            tp += 2;
            tokenlen = endp-tp;
            if (tokenlen < 128) {
                memcpy(token, tp, endp-tp);
                token[endp-tp] = 0;
                tp += tokenlen;
                if ((tokenlen > 5 && !memcmp(token, "JSON:", 5)) || (tokenlen > 6 && !memcmp(token, "JSON?:", 6))) {
                    int optional = 0;
                    concat_alloc_t vbuf = {xbuf, sizeof xbuf};
                    char * iname = token+5;
                    if (token[4]=='?') {
                        iname++;
                        optional = 1;
                    }
                    char * itoken = strchr(iname, ':');
                    if (itoken) {
                        *itoken++ = 0;
                        tokenlen = strlen(itoken);
                        int missing = putVar(&vbuf, itoken, tokenlen, topic, forwarder, mmsg);
                        if (!missing || !optional) {
                            if (*iname) {
                                ism_json_putString(buf, iname);
                                ism_json_putBytes(buf, ":");
                            }
                            ism_json_putNull(&vbuf);
                            if (missing)
                                ism_json_putBytes(buf, "null");
                            else
                                ism_json_putString(buf, vbuf.buf);
                            if (vbuf.inheap)
                                ism_common_freeAllocBuffer(&vbuf);
                        } else {
                            /* remove preceeding comma if there is one */
                            char * bp = buf->buf + buf->used - 1;
                            while (bp >= buf->buf) {
                                if ((uint8_t)*bp <= 0x20) {    /* Ignore white space */
                                    bp--;
                                } else {
                                    if (*bp == ',') {
                                        buf->used = bp-buf->buf;
                                    } else if(*bp == '{' || *bp == '[') {
                                        nocomma = 1;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    putVar(buf, token, tokenlen, topic, forwarder, mmsg);
                }

            } else {
                tp += tokenlen;
            }
        }
        if (*tp == '}')
            tp++;
    }
}


/*
 * Select a message
 */
int selectMsg(ism_forwarder_t * forwarder, mqttbrMsg_t * mmsg) {
    ism_emsg_t emsg = {0};
    ismMessageHeader_t hdr = {0};
    int selected;

    pthread_spin_lock(&forwarder->lock);
    if (!forwarder->props) {
        forwarder->props = ism_common_newProperties(100);
    } else {
        ism_common_clearProperties(forwarder->props);
    }
    emsg.hdr = &hdr;
    hdr.Reliability = mmsg->qos;
    emsg.otherprops = (char *)mmsg->props;
    emsg.proplen = mmsg->prop_len;
    emsg.topic = mmsg->topic;
    selected = ism_common_filter(forwarder->selector, forwarder->props, ism_mqtt_propgen, &emsg);
    pthread_spin_unlock(&forwarder->lock);
    return selected;
}


/*
 * Get the partition.  We do this even if the part count is 1 because we do
 * not yet know where we are going.
 */
uint32_t getPartition(ism_forwarder_t * forwarder, mqttbrMsg_t * mmsg, concat_alloc_t * buf) {
    uint64_t now;
    switch (forwarder->evst_partrule) {
    case PART_RULE_HASH:   /* Hash a string */
        if (forwarder->partitionmap)
            ism_bridge_topicMapper(buf, mmsg->topic, forwarder->partitionmap, forwarder, mmsg);
        return ism_memhash_fnv1a_32(buf->buf, buf->used);

    case PART_RULE_SINGLE: /* Chose a single part */
    case PART_RULE_NUMBER: /* Select a part number */
        return forwarder->evst_part;

    default:
    case PART_RULE_ANY:    /* Distribute between all parts */
        now = ism_common_currentTimeNanos();
        return ism_memhash_fnv1a_32(&now, 8);

    case PART_RULE_INSTANCE: /* Use instance as the part number */
        return forwarder->instof;
    }
}


/*
 * Publish a message to a Event Streams destination
 */
int publishToMhub(ism_forwarder_t * forwarder, mqttbrMsg_t * mmsg, mqtt_pmsg_t * pmsg, concat_alloc_t * buf) {
    ism_mhub_t * mhub = forwarder->mhub;
    uint16_t  topiclist[100];
    char * topic;
    int count;
    int part;
    int i;

    pmsg->payload = mmsg->payload;
    pmsg->payload_len = mmsg->payload_len;
    pmsg->props = mmsg->props;
    pmsg->prop_len = mmsg->prop_len;

    /* Zero terminate the topic in both message structures */
    pmsg->topic_len = mmsg->topic_len;
    pmsg->topic = topic = alloca(mmsg->topic_len + 1);
    memcpy(topic, mmsg->topic, mmsg->topic_len);
    topic[pmsg->topic_len] = 0;
    mmsg->topic = topic;

    ism_mhub_lock(mhub);
    count = ism_mhub_selectMessages(mhub, topiclist, 100, NULL, NULL, pmsg);
    ism_mhub_unlock(mhub);

    if (count > 0) {
        /* Do the partition map */
        buf->used = 0;
        part = getPartition(forwarder, mmsg, buf);
        buf->used = 0;
        /* Construct the key */
        if (forwarder->keymap)
            ism_bridge_topicMapper(buf, pmsg->topic, forwarder->keymap, forwarder, mmsg);
        pmsg->key_len = buf->used;
        /* If there are properties, make the kafka header */
        if (pmsg->prop_len) {
            pmsg->hdr_count = ism_kafka_makeKafkaHeaders(NULL, buf, pmsg, NULL, NULL, mhub->messageVersion);
            pmsg->headers = buf->buf + pmsg->key_len;
            pmsg->hdr_len = buf->used - pmsg->key_len;
        }
        pmsg->key = buf->buf;

        /* For each selected topic, publish the message to that mhub topic */
        for (i=0; i<count; i++){
            ism_mhub_publishEvent(mhub, pmsg, forwarder->name, topiclist[i], part);
        }

        if (buf->inheap)
            ism_common_freeAllocBuffer(buf);
    }
    return 0;
}

/*
 * Handle the receive of a publish
 *
 * Since we only do one way connections, this is always from the source.
 */
int doPublish(ism_transport_t * transport, uint8_t * bp, int buflen, int kind) {
    int rc = 0;
    mqttbrMsg_t mmsg = {transport};
    mqttbr_pobj_t * pobj = transport->pobj;
    ism_transport_t * dtransport = pobj->dest_transport;
    ism_forwarder_t * forwarder = pobj->forwarder;
    int selected = SELECT_TRUE;

    mmsg.kind = kind;
    /* Parse the publish topic */
    rc = parsePublish(transport, &mmsg, bp, buflen);
    if (!rc) {
        char xbuf [4096];
        concat_alloc_t buf = {xbuf, sizeof xbuf, 16};

        forwarder->source_msgs++;
        transport->read_msg++;
        forwarder->source_bytes += buflen;

        /* TODO: implement topic alias */

        /* Select the message     */
        if (forwarder->selector) {
            selected = selectMsg(forwarder, &mmsg);
        }

        if (selected == SELECT_TRUE) {

            if (forwarder->evst_dest) {
                mqtt_pmsg_t pmsg = {(char *)bp, buflen, kind};
                rc = publishToMhub(forwarder, &mmsg, &pmsg, &buf);
                return rc;
            }

            /* Put out the topic if no topic map */
            if (!forwarder->topicmap || !*forwarder->topicmap) {
                bputchar(&buf, (char)(mmsg.topic_len>>8));
                bputchar(&buf, (char)mmsg.topic_len);
                ism_common_allocBufferCopyLen(&buf, mmsg.topic, mmsg.topic_len);
            }
            /* Use the topic mapper to convert the topic */
            else {
                int topic_loc = buf.used;
                char * ttopic = alloca(mmsg.topic_len + 1);
                memcpy(ttopic, mmsg.topic, mmsg.topic_len);
                ttopic[mmsg.topic_len] = 0;
                buf.used += 2;
                ism_bridge_topicMapper(&buf, ttopic, forwarder->topicmap, forwarder, &mmsg);
                int topic_len = buf.used-topic_loc-2;
                buf.buf[topic_loc] = (char)(topic_len>>8);
                buf.buf[topic_loc+1] = (char)topic_len;
            }

            /* Put out the packet id */
            if (mmsg.qos > 0) {
                bputchar(&buf, (char)(mmsg.packetid>>8));
                bputchar(&buf, (char)mmsg.packetid);
            }

            /* Write out properties */
            if (dtransport->pobj->mqtt_version >= 5) {
                if (!mmsg.prop_len) {
                    bputchar(&buf, 0);       /* No metadata */
                } else {
                    /*
                     * Remove the topic alias.
                     * All of the other metadata fields are passed from client to server.
                     */
                    if (mmsg.topic_alias_loc) {
                        int pos = mmsg.topic_alias_loc - mmsg.props;
                        assert(pos >= 0 && pos+3 <= mmsg.prop_len);
                        memmove((char *)mmsg.props+pos, mmsg.props+pos+3, mmsg.prop_len-pos-3);
                        mmsg.prop_len -= 3;
                    }
                    ism_common_putMqttVarInt(&buf, mmsg.prop_len);
                    ism_common_allocBufferCopyLen(&buf, mmsg.props, mmsg.prop_len);
                }
            }

            /* Write out the body */
            if (mmsg.payload_len)
                ism_common_allocBufferCopyLen(&buf, mmsg.payload, mmsg.payload_len);

            /* Send the publish */
            forwarder->dest_msgs++;
            forwarder->dest_bytes += buf.used-16;
            dtransport->lastAccessTime = ism_common_readTSC();
            __sync_add_and_fetch(&dtransport->write_msg, 1);
            dtransport->send(dtransport, buf.buf+16, buf.used-16, kind, SFLAG_FRAMESPACE);
        }

        /* Free up the buffer if necessary */
        if (buf.inheap)
            ism_common_freeAllocBuffer(&buf);
    }
    return 0;
}


/*
 * Complete QoS2 flow when the destination session is present
 * but the source destination is not.  We send any PUBRECs back
 * as PUBREL and just ignore any PUBCOMPs.  In MQTTv5 we can set
 * and error and will never get the PUBCOMP.  This is a case that
 * will work most of the time.  The other solution is to close the
 * destination connection and reconnect.  That is as likely to
 * lose messages.
 */
static void completeQoS2NotPresent(ism_transport_t * transport) {
    char xbuf[32];
    int  acklen;
    int  command;
    pthread_spin_lock(&transport->lock);
    if (transport->pobj->senddata_len) {
        /* Send any PUBREC in the list back to the destination as PUBREL */
        int  len = transport->pobj->senddata_len;
        char * ackdata = transport->pobj->senddata;
        while (len >= 4) {
            command = (*ackdata >> 4) & 0x0f;
            if (command == MT_PUBREC) {
                acklen = 2;
                xbuf[16] = ackdata[2];
                xbuf[17] = ackdata[3];
                if (transport->pobj->mqtt_version >= 5) {
                    xbuf[18] = 0x80;
                    acklen = 3;
                }
                transport->send(transport, xbuf+16, acklen, (MT_PUBREL<<4)+2, SFLAG_FRAMESPACE);
            }
            ackdata += ackdata[1]+2;
            len -= ackdata[1]+2;
        }
    }
    pthread_spin_unlock(&transport->lock);
}

/*
 * Send on the saved ACKs to the source
 */
static void completeQoS2Present(ism_transport_t * transport, ism_transport_t * dtransport) {
    mqttbr_pobj_t * pobj = dtransport->pobj;
    pthread_spin_lock(&dtransport->lock);
    if (pobj->senddata_len) {
        transport->send(transport, pobj->senddata, pobj->senddata_len, 0, SFLAG_HASFRAME);
        pobj->senddata_len = 0;
        pobj->senddata_alloc = 0;
        ism_common_free(ism_memory_proxy_br_misc,pobj->senddata);
        pobj->senddata = NULL;
    }
    pthread_spin_unlock(&dtransport->lock);
}

/*
 * Handle the receive of an ACK
 *
 * This can come from either source or destination
 *
 */
int doACK(ism_transport_t * transport, uint8_t * bp, int buflen, int kind) {
    int rc = 0;
    char xbuf [1032];
    mqttbr_pobj_t * pobj = transport->pobj;
    ism_transport_t * dtransport = pobj->isSource ? pobj->dest_transport : pobj->source_transport;
    int packetid = 0;
    int proplen = 0;
    int mqttrc = 0;
    int acklen;

    if (buflen >= 2) {
        packetid = (uint16_t)BIGINT16(bp);
        bp += 2;
        buflen -= 2;
        if (buflen > 0 && transport->pobj->mqtt_version >= 5) {
            mqttrc = *bp++;
            buflen--;
            if (buflen > 0) {
                proplen = *bp;
                if (proplen & 0x80) {
                    int vilen;
                    proplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                    bp += vilen;
                    buflen -= vilen;
                } else {
                    buflen--;
                    bp++;
                }
                if (proplen > 0 && proplen <= buflen) {
                    /* Do not bother with prop checking for now since we do not need any field in the props */
                    //rc = ism_common_checkMqttPropFields((char *)bp, proplen,
                    //    g_ctx5, CPOI_PUBACK | CPOIL_PUBREL, mpropCheck, &mmsg);
                    bp += proplen;
                    buflen -= proplen;
                }
            }
        }
    }

    if (buflen || proplen < 0) {
        rc = ISMRC_BadLength;
        ism_common_setError(rc);
    }
    if (!rc) {
        xbuf[16] = (char)(packetid >> 8);
        xbuf[17] = (char)packetid;
        acklen = 2;
        if (mqttrc && dtransport->pobj->mqtt_version >= 5) {
            acklen = 3;
            xbuf[18] = mqttrc;       /* TODO: more QoS=2 error handling */
        }
        if (dtransport && dtransport->ready == 1) {
            dtransport->lastAccessTime = ism_common_readTSC();
            dtransport->send(dtransport, xbuf+16, acklen, kind, SFLAG_FRAMESPACE);
        } else {
            appendConnectData(transport, xbuf+16, acklen, kind);
        }
    }
    return rc;
}


/*
 * Handle the receive of a SUBACK
 *
 * This always comes from the source
 */
int doSuback(ism_transport_t * transport, uint8_t * bp, int buflen, int kind) {
    int rc = 0;
    int mqttrc = 0;
    int packetid = 0;
    int proplen = 0;

    if (buflen >= 2) {
        packetid = (uint16_t)BIGINT16(bp);
        bp += 2;
        buflen -= 2;
        if (buflen > 0 && transport->pobj->mqtt_version >= 5) {
            if (buflen > 0) {
                proplen = *bp;
                if (proplen & 0x80) {
                    int vilen;
                    proplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                    bp += vilen;
                    buflen -= vilen;
                } else {
                    buflen--;
                    bp++;
                }
                if (proplen > 0 && proplen <= buflen) {
                    /* Ignore the properties except to skip over them */
                    bp += proplen;
                    buflen -= proplen;
                }
            }
        }
        /* We only do one subscribe at a time, so the length must be 1 */
        if (buflen == 1 && proplen >= 0) {
            ism_forwarder_t * forwarder = transport->pobj->forwarder;
            pthread_spin_lock(&forwarder->lock);
            int which = packetid-10000;
            if (which > forwarder->topicCount)
                which = -1;

            mqttrc = *bp;
            buflen--;
            if (mqttrc >= 128) {
                TRACE(4, "Subscription failure: Forwarder=%s Topic=%s RC=%02x (%d)\n",
                        forwarder->name, (which >= 0 ? forwarder->topic[which] : ""), mqttrc, mqttrc);
                 /* TODO Error handling for a bad subscribe */
            } else {
                TRACE(7, "Subscription complete: Forwarder=%s Topic=%s QoS=%d\n",
                        forwarder->name, (which >= 0 ? forwarder->topic[which] : ""), mqttrc);
            }
            forwarder->subcount++;
            if (forwarder->subcount >= forwarder->topicCount) {
                /* On the final subscribe, change state to active */
                forwarder->active = BCS_Active;
                forwarder->retrycount[0] = 0;
                forwarder->retrycount[1] = 0;
                forwarder->retryLogged[0] = 0;
                 forwarder->retryLogged[1] = 0;
                forwarder->source_rc = 0;
                forwarder->dest_rc = 0;
                replaceString(&forwarder->source_reason, NULL);
                replaceString(&forwarder->dest_reason, NULL);
                TRACE(6, "Change forwarder state: forwarder=%s state=%s\n", forwarder->name,
                    bridge_state_str(forwarder->active));
                LOG(NOTICE, Server, 984, "%s%s", "The state of forwarder {0} is now: {1}",
                        forwarder->name, bridge_state_str(forwarder->active));
            }
            pthread_spin_unlock(&forwarder->lock);
        }
    }

    if (buflen || proplen < 0) {
        rc = ISMRC_BadLength;
        ism_common_setError(rc);
    }
    return rc;
}

/*
 * Handle a disconnect
 *
 * This could come from either source or destination.
 * The server to client disconnect is only in MQTTv5
 */
int doDisconnect(ism_transport_t * transport, uint8_t * bp, int buflen, int kind) {
    int rc = 0;
    mqttbrMsg_t mmsg = {transport};
    xUNUSED int mqttrc = 0;
    int proplen = 0;

    if (transport->pobj->mqtt_version >= 5 && buflen > 0) {
        mqttrc = *bp++;
        buflen--;
        if (buflen >= 0) {
            proplen = *bp;
            if (proplen & 0x80) {
                int vilen;
                proplen = ism_common_getMqttVarIntExp((const char *)bp, buflen, &vilen);
                bp += vilen;
                buflen -= vilen;
            } else {
                buflen--;
                bp++;
            }
            if (proplen > 0 && proplen <= buflen) {
                rc = ism_common_checkMqttPropFields((char *)bp, proplen,
                        g_ctx5, CPOI_DISCONNECT, mpropCheck, &mmsg);
                bp += proplen;
                buflen -= proplen;
            }
        }
        if (mmsg.reason)
            transport->reason = mmsg.reason;
        if (mqttrc) {
            transport->pobj->mqtt_rc = mqttrc;
            rc = mapToIsmRC(mqttrc, transport->pobj->mqtt_version);
        }
    }

    if (buflen) {
        rc = ISMRC_BadLength;
        ism_common_setError(rc);
    }
    return rc;
}


/*
 * Receive MQTT bridge.
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
int ism_bridge_receive(ism_transport_t * transport, char * inbuf, int buflen, int kind) {
    uint8_t * bp = (uint8_t *) inbuf;
    int rc = 0;
    uint8_t command = (uint8_t)((kind >> 4) & 15);

    /* Do the receive trace */
    if (SHOULD_TRACEC(9, Mqtt)) {
        char obuf[64];
        sprintf(obuf, "MQTTBR receive %02x %s connect=%u", kind & 0xff, ism_mqtt_mqttCommand(command), transport->index);
        traceDataFunction(obuf, 0, __FILE__, __LINE__, inbuf, buflen, ism_common_getTraceMsgData());
    }

    if (!rc) {
        switch (command) {
        case MT_CONNACK:
            rc = doConnack(transport, bp, buflen, kind);
            break;
        case MT_PUBLISH:
            rc = doPublish(transport, bp, buflen, kind);
            break;
        case MT_PUBACK:
        case MT_PUBREC:
        case MT_PUBREL:
        case MT_PUBCOMP:
            rc = doACK(transport, bp, buflen, kind);
            break;
        case MT_SUBACK:
            rc = doSuback(transport, bp, buflen, kind);
            break;
        case MT_PINGRESP:
            break;             /* Do nothing */
        case MT_DISCONNECT:
            rc = doDisconnect(transport, bp, buflen, kind);
            break;
        default:
            rc = ISMRC_InvalidCommand; /* Invalid command */
            ism_common_setError(rc);
            break;
        }
    }
    if (rc) {
        /* On failure, close the connection */
        char * xbuf = alloca(4096);
        if (transport->reason) {
            transport->close(transport, rc, 0, transport->reason);
        } else {
            ism_common_formatLastError(xbuf, 4096);
            transport->close(transport, rc, 0, xbuf);
        }
    }
    return rc;
}

/*
 * Check for MQTTv5 properties
 * This does only minimal validation of the properties and gathers a few values
 */
static int mpropCheck(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value) {
    mqttbrMsg_t * mmsg = (mqttbrMsg_t *) userdata;
    if (fld->type == MPT_String) {
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
    switch (fld->id) {

    case MPI_SessionExpire:
        if (mmsg) {
            mmsg->sessionExpiry = value;
        }
        break;
    case MPI_MaxTopicAlias:
    case MPI_TopicAlias:
        if (mmsg) {
            mmsg->topic_alias = value;
            mmsg->topic_alias_loc = (char *)(ptr-1);
        }
        break;
    case MPI_MaxPacketSize:
        if (mmsg)
            mmsg->maxPacketSize = value;
        break;
    case MPI_KeepAlive:
        if (mmsg)
            mmsg->keepAlive = (int32_t)value;
        break;
    case MPI_MaxReceive:
        if (mmsg)
            mmsg->receive_max = (int32_t)value;
        break;
    case MPI_MaxQoS:
        if (mmsg)
            mmsg->maxqos = (uint8_t)value;
        break;
    case MPI_RetainAvailable:
        if (mmsg && !value)
            mmsg->available &= ~AVAIL_Retain;
        break;
    case MPI_WildcardAvailable:
        if (mmsg && !value)
            mmsg->available &= ~AVAIL_Wildcard;
        break;
    case MPI_SubIDAvailable:
        if (mmsg && !value)
            mmsg->available &= ~AVAIL_SubID;
        break;
    case MPI_SharedAvailable:
        if (mmsg && !value)
            mmsg->available &= ~AVAIL_Shared;
        break;
        break;
    }
    return 0;
}

/*
 * Configure a bridge object  (Forwarder or Connection)
 */
int ism_bridge_config_json(ism_json_parse_t * parseobj, int where, int checkonly, int keepgoing) {
    int   rc = 0;
    int   xrc;
    int   endloc;
    ism_json_entry_t * ent;
    int isForwarder;
    int isConnection;

    pthread_mutex_lock(&bridgelock);

    if (!parseobj || where > parseobj->ent_count) {
        TRACE(2, "Bridge config JSON not correct\n");
        pthread_mutex_unlock(&bridgelock);
        return 1;
    }
    ent = parseobj->ent+where;
    isForwarder = !strcmp(ent->name, "Forwarder");
    isConnection = !strcmp(ent->name, "Connection");
    if (!ent->name || (!isForwarder && !isConnection) || ent->objtype != JSON_Object) {
        TRACE(2, "Bridge config JSON invoked for config which is not a server, tenant, or user\n");
        pthread_mutex_unlock(&bridgelock);
        return 2;
    }
    endloc = where + ent->count;
    where++;
    ent++;
    while (where <= endloc) {
        if (isForwarder) {
            xrc = ism_bridge_makeForwarder(parseobj, where, NULL, checkonly, keepgoing);
            if (xrc)
                rc = xrc;
        }
        if (isConnection) {
            xrc = ism_bridge_makeConnection(parseobj, where, NULL, checkonly, keepgoing);
            if (xrc)
                rc = xrc;
        }

        ent = parseobj->ent+where;
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }
    pthread_mutex_unlock(&bridgelock);
    return rc;
}


/*
 * Do a strcmp allowing for NULLs
 */
static int my_strcmp(const char * str1, const char * str2) {
    if (str1 == NULL || str2 == NULL) {
        if (str1 == NULL && str2 == NULL)
            return 0;
        return str1 ? 1 : -1;
    }
    return strcmp(str1, str2);
}


/*
 * Replace a string
 * Return 1 if the value is changed, 0 if it has not changed
 */
static int  replaceString(const char * * oldval, const char * val) {
    if (!my_strcmp(*oldval, val))
        return 0;
    if (*oldval) {
        char * freeval = (char *)*oldval;
        if (val)
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_br_misc,1000),val);
        else
            *oldval = NULL;
        ism_common_free(ism_memory_proxy_br_misc,freeval);
    } else {
        if (val)
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_br_misc,1000),val);
        else
            *oldval = NULL;
    }
    return 1;
}


/*
 * TLS methods
 */
static ism_enumList enum_methods [] = {
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
    { "Method",    5,                 },
#else
    { "Method",    4,                 },
#endif
    { "None",      0,                 },
    { "TLSv1",     SecMethod_TLSv1,   },
    { "TLSv1.1",   SecMethod_TLSv1_1, },
    { "TLSv1.2",   SecMethod_TLSv1_2, },
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
    { "TLSv1.3",   SecMethod_TLSv1_3, },
#endif
};

/*
 * MQTT versions, we allow both string and numeric
 */
static ism_enumList enum_versions [] = {
    { "Version",    3,                 },
    { "3.1",       3,   },
    { "3.1.1",     4,   },
    { "5.0",       5,   },
};

extern int ism_tcp_getIOPcount(void);

/*
 * Link this forwarder at the end of the list
 */
static void linkForwarder(ism_forwarder_t * rfwd) {
    rfwd->next = NULL;
    ismForwardersCount++;
    if (!ismForwarders) {
        ismForwarders = rfwd;
    } else {
        ism_forwarder_t * forwarder = ismForwarders;
        while (forwarder->next) {
            forwarder = forwarder->next;
        }
        forwarder->next = rfwd;
    }
}

/*
 * Unlink this forwarder
 */
static void unlinkForwarder(ism_forwarder_t * rfwd) {
    if (ismForwarders && rfwd) {
        rfwd->enabled = 0;
        ismForwardersCount--;
        if (ismForwarders == rfwd) {
            ismForwarders = ismForwarders->next;
        } else {
            ism_forwarder_t * forwarder = ismForwarders;
            while (forwarder->next) {
                if (forwarder->next == rfwd) {
                    forwarder->next = rfwd->next;
                    break;
                }
                forwarder = forwarder->next;
            }
        }
    }
}

/*
 * Free a forwarder object
 * Generally we only do this if the config fails, once we link
 * in a forwarder we do not delete it.
 */
static void freeForwarder(ism_forwarder_t * forwarder) {
    int i;
    if (forwarder) {
        for (i=0; i<forwarder->topicCount; i++) {
            replaceString(&forwarder->topic[i], NULL);
        }
        replaceString(&forwarder->name, NULL);
        replaceString(&forwarder->source, NULL);
        replaceString(&forwarder->destination, NULL);
        replaceString(&forwarder->selectors, NULL);
        replaceString(&forwarder->topicmap, NULL);
        ism_common_free(ism_memory_proxy_br_misc,forwarder);
    }
}

/*
 * Find the forwarder by name
 */
ism_forwarder_t * ism_bridge_getForwarder(const char * name) {
    ism_forwarder_t * ret = ismForwarders;
    if (!name)
        return NULL;
    while (ret) {
        if (!strcmp(name, ret->name))
            break;
        ret = ret->next;
    }
    return ret;
}

/*
 * Make a forwarder instance from a configured instance
 */
static int makeForwarderFrom(ism_forwarder_t * base, int instof) {
    if (base->instances < 1 || instof < 0 || instof >= base->instances || !base->enabled)
        return 1;

    char * name;
    int    i;
    ism_forwarder_t * forwarder = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_br_misc,9),1, sizeof(ism_forwarder_t));
    int namelen = strlen(base->name);
    strcpy(forwarder->structid, "BrFwd");
    forwarder->name = name = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_br_misc,10),1, strlen(base->name)+ 3);
    strcpy(name, base->name);
    name[namelen] = '0' + (instof / 10);
    name[namelen+1] = '0' + (instof % 10);
    pthread_spin_init(&forwarder->lock, 0);
    forwarder->instances = base->instances;
    forwarder->instof = instof;                 /* Which instance */
    forwarder->topicCount = base->topicCount;   /* Count of used topic names */
    forwarder->subQoS = base->subQoS;
    forwarder->need = base->need & ~4;
    replaceString(&forwarder->source, base->source);
    replaceString(&forwarder->destination, base->destination);
    for (i=0; i<forwarder->topicCount; i++)
        replaceString(forwarder->topic+i, base->topic[i]);
    replaceString(&forwarder->selectors, base->selectors);
    forwarder->selectors = base->selectors;   /* Just point to the compiled rule */
    if (base->selector_len) {
        forwarder->selector = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_br_misc,11),base->selector_len);
        memcpy(forwarder->selector, base->selector, base->selector_len);
        forwarder->selector_len = base->selector_len;
    }
    forwarder->enabled = base->enabled;
    forwarder->evst_dest = base->evst_dest;
    forwarder->evst_partrule = base->evst_partrule;
    forwarder->evst_part = base->evst_part;
    if (base->evst_partrule == PART_RULE_SINGLE) {
        uint32_t hash = ism_strhash_fnv1a_32(ism_common_getHostnameInfo());
        hash = ism_strhash_fnv1a_32_more(forwarder->name, hash);
        forwarder->evst_part = hash;
    }
    if (base->evst_partrule == PART_RULE_INSTANCE) {
        forwarder->evst_part = instof;
    }
    replaceString(&forwarder->topicmap, base->topicmap);
    linkForwarder(forwarder);
    return 0;
}


/*
 * Make a tenant object from the configuration.
 */
int ism_bridge_makeForwarder(ism_json_parse_t * parseobj, int where, const char * name, int checkonly, int keepgoing) {
    int endloc;
    int endarray;
    int awhere;
    ism_forwarder_t * forwarder;
    int  i;
    int  j;
    int  rc = 0;
    int  created = 0;
    int  namelen;
    char xbuf[1024];
    ism_json_entry_t * aent;
    int  need = 0;
    int  needlog = 1;
    ismRule_t * rule = NULL;
    int rulelen = 0;

    if (!parseobj || where > parseobj->ent_count)
        return 1;
    ism_json_entry_t * ent = parseobj->ent+where;
    endloc = where + ent->count;
    where++;
    if (!name)
        name = ent->name;
    namelen = name ? strlen(name) : 0;
    if (namelen < 1 || namelen > 32 || ism_common_validUTF8Restrict(name, namelen, (UR_NoControl | UR_NoNonchar))<0) {
        if (namelen > 500)
            namelen = 500;
        char * nbuf = alloca(namelen + 1);
        ism_common_validUTF8Replace(name, namelen, nbuf, (UR_NoControl | UR_NoNonchar), '?');
        LOG(ERROR, Server, 918, "%-s", "The name of a Forwarder is not valid: {0}", nbuf);
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", nbuf);
        return ISMRC_ArgNotValid;
    }
    forwarder = ism_bridge_getForwarder(name);
    if (forwarder) {
        if (ent->objtype != JSON_Object) {
            if (forwarder->active == BCS_Active) {
                forwarder->s_transport->pobj->disabling = 1;
                if (!forwarder->evst_dest)
                    forwarder->d_transport->pobj->disabling = 1;
                /* TODO close mhub */
                forwarder->active = BCS_Disabling;
                forwarder->s_transport->close(forwarder->s_transport, ISMRC_EndpointDisabled, 0,
                        "Connection closing because the forwarder is deleted");
            }
            unlinkForwarder(forwarder);
            return 0;
        }
    } else {
        if (ent->objtype == JSON_Object) {
            forwarder = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_br_misc,12),1, sizeof(ism_server_t));
            strcpy(forwarder->structid, "BrFwd");
            pthread_spin_init(&forwarder->lock, 0);
            forwarder->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_br_misc,1000),name);
            forwarder->enabled = 1;
            forwarder->subQoS = 1;
            forwarder->evst_ver = 0xff;
            forwarder->instances = 0;
            forwarder->instof = -1;
            created = 1;
        } else {
            if (ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Forwarder", name);
                return ISMRC_BadPropertyValue;
            }
            TRACE(4, "Forwarder does not exist: %s\n", name);
            return 1;
        }
    }
    int savewhere = where;
    while (where <= endloc && rc == 0) {
        ent = parseobj->ent + where;
        if (ent->objtype == JSON_Null)
            ent->count = 0;
        if (!strcmp(ent->name, "Topic")) {
            if (ent->objtype != JSON_String &&
                (ent->objtype != JSON_Array || ent->count > 16)) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Topic", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
            if (rc == 0) {
                if (ent->objtype == JSON_String) {
                    forwarder->topicCount = 1;
                } else {
                    forwarder->topicCount = ent->count;
                    awhere = where+1;
                    endarray = awhere + ent->count;
                    while (awhere < endarray) {
                        aent = parseobj->ent + awhere;
                        if (aent->objtype != JSON_String) {
                            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Topic", ism_json_getJsonValue(ent));
                            rc = ISMRC_BadPropertyValue;
                        }
                        awhere++;
                    }
                }
            }
        } else if (!strcmp(ent->name, "Source")) {
            if (ent->objtype != JSON_String) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Source", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "Destination")) {
            if (ent->objtype != JSON_String) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Destination", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "Enabled")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Enabled", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "Selector")) {
            if (ent->objtype == JSON_String) {
                rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, ent->value, SELOPT_Internal);
            }
            if (ent->objtype == JSON_Null) {
                rule = (ismRule_t *)"";
                rulelen = 0;
            }
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Selector", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "TopicMap")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "TopicMap", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "RoutingRule")) {
            if (ent->objtype != JSON_Object && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            } else {
                if (ent->objtype == JSON_Object) {
                    int addrcount = ent->count;
                    for (i=0; i<addrcount; i++) {
                        if (parseobj->ent[where+i+1].objtype != JSON_String && parseobj->ent[where+i+1].objtype != JSON_Null) {
                            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                            rc = ISMRC_BadPropertyValue;
                            break;
                        }
                    }
                }
            }
        } else if (!strcmp(ent->name, "KeyMap")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KeyMap", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "PartitionRule")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Integer && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "PartitionRule", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "Instances")) {
            if ((ent->objtype != JSON_Integer || ent->count < 0 || ent->count > 100) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Instances", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "SourceQoS")) {
            if ((ent->objtype != JSON_Integer || ent->count < 0 || ent->count > 2) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "SourceQoS", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "KafkaAPIVersion")) {
            if ((ent->objtype != JSON_Integer || ent->count < 0 || ent->count > 2) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else {
            LOG(WARN, Server, 938, "%s%-s", "Unknown forwarder property: Forwarder={0} Property={1}",
                    forwarder->name, ent->name);
            needlog = 0;
            if (!keepgoing) {
                rc = ISMRC_BadPropertyName;
                ism_common_setErrorData(rc, "%s", ent->name);
            }
        }
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }

    /* Implement verification only */
    if (checkonly) {
        if (rule)
           ism_common_freeSelectRule(rule);
        return rc;
    }

    if (rc == 0) {
        where = savewhere;
        while (where <= endloc) {
            ent = parseobj->ent + where;
            if (!strcmp(ent->name, "Topic")) {
                if (ent->objtype == JSON_String) {
                    if (replaceString(&forwarder->topic[0], ent->value))
                        need |= forwarder->topic[0] ? 2 : 1;
                } else {
                    int count = ent->count;
                    ent++;
                    for (i=0; i<count; i++) {
                        if (replaceString(&forwarder->topic[i], parseobj->ent[where+i+1].value))
                            need |= forwarder->topic[0] ? 2 : 1;
                    }
                    ent = parseobj->ent + where;
                }
            } else if (!strcmp(ent->name, "Source")) {
                need |= replaceString(&forwarder->source, ent->value);
            } else if (!strcmp(ent->name, "Destination")) {
                need |= replaceString(&forwarder->destination, ent->value);
            } else if (!strcmp(ent->name, "Enabled")) {
                int enabled = ent->objtype == JSON_True;
                if (forwarder->enabled != enabled)
                    need |= 1;
                forwarder->enabled = ent->objtype == JSON_True;
            } else if (!strcmp(ent->name, "TopicMap")) {
                if (ent->value && !*ent->value)
                    ent->value = NULL;
                need |= replaceString(&forwarder->topicmap, ent->value);
            } else if (!strcmp(ent->name, "RoutingRule")) {
                int rulecount = ent->count;
                if (ent->objtype == JSON_Null) {
                    for (i=0; i<forwarder->rulecount; i++) {
                        replaceString(&forwarder->rules[i], NULL);
                    }
                    forwarder->rulecount = 0;
                    need |= 2;
                } else {
                    ent++;
                    for (i=0; i<rulecount; i++) {
                        for (j=0; j<forwarder->rulecount; j++) {
                            if (!strcmp(forwarder->rulenames[j], ent->name)) {
                                if (replaceString(&forwarder->rules[j], ent->value)) {
                                    need |= ent->value ? 2 : 1;
                                }
                                break;
                            }
                        }
                        if (j == forwarder->rulecount) {
                            if (forwarder->rulecount >= forwarder->rulealloc) {
                                int newcnt = forwarder->rulecount + 8;
                                if (newcnt < 8)
                                    newcnt = 8;
                                forwarder->rules = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_br_misc,13),forwarder->rules, newcnt*sizeof(char *));
                                forwarder->rulenames = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_br_misc,14),forwarder->rulenames, newcnt*sizeof(char *));
                                int zerosize = (newcnt-forwarder->rulealloc) * sizeof(char *);
                                int pos = forwarder->rulealloc*sizeof(char *);
                                memset(((char *)forwarder->rules)+pos, 0, zerosize);
                                memset(((char *)forwarder->rulenames)+pos, 0, zerosize);
                                need |= 1;
                                forwarder->rulealloc = newcnt;
                            }
                            replaceString(&forwarder->rulenames[j], ent->name);
                            replaceString(&forwarder->rules[j], ent->value);
                            forwarder->rulecount++;
                        }
                        ent++;
                    }
                    ent = parseobj->ent + where;
                }
            } else if (!strcmp(ent->name, "KeyMap")) {
                if (ent->value && !*ent->value)
                    ent->value = NULL;
                need |= replaceString(&forwarder->keymap, ent->value);
            } else if (!strcmp(ent->name, "PartitionRule")) {
                if (ent->objtype == JSON_Integer) {
                    forwarder->evst_partrule = PART_RULE_NUMBER;
                    forwarder->evst_part = ent->count;
                } else {
                    if (ent->value && !*ent->value)
                        ent->value = NULL;
                    if (replaceString(&forwarder->partitionmap, ent->value)) {
                        need |= 1;
                        if (forwarder->partitionmap) {
                            if (!strcmpi(forwarder->partitionmap, "any")) {
                                forwarder->evst_partrule = PART_RULE_ANY;
                            } else if (!strcmpi(forwarder->partitionmap, "single")) {
                                uint32_t hash = ism_strhash_fnv1a_32(ism_common_getHostnameInfo());
                                hash = ism_strhash_fnv1a_32_more(forwarder->name, hash);
                                forwarder->evst_part = hash;
                                forwarder->evst_partrule = PART_RULE_SINGLE;
                            } else if (!strcmpi(forwarder->partitionmap, "instance")) {
                                forwarder->evst_part = ism_strhash_fnv1a_32(forwarder->name);
                                forwarder->evst_partrule = PART_RULE_INSTANCE;
                            } else {
                                forwarder->evst_partrule = PART_RULE_HASH;
                            }
                        } else {
                            forwarder->evst_partrule = PART_RULE_ANY;
                        }
                    }
                }
            } else if (!strcmp(ent->name, "Selector")) {
                replaceString(&forwarder->selectors, ent->value);
            } else if (!strcmp(ent->name, "SourceQoS")) {
                if (ent->objtype == JSON_Null)
                    ent->count = 1;
                if (forwarder->subQoS != ent->count)
                    need |= 1;
                forwarder->subQoS = ent->count;
            } else if (!strcmp(ent->name, "KafkaAPIVersion")) {
                if (ent->objtype == JSON_Null)
                    ent->count = 0xff;
                if (forwarder->evst_ver != ent->count)
                    need |= 1;
                forwarder->evst_ver = ent->count;
            } else if (!strcmp(ent->name, "Instances")) {
                if (forwarder->instances != ent->count)
                    need |= 4;
                forwarder->instances = ent->count;
            }
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }
    } else {
        if (needlog) {
            ism_common_formatLastError(xbuf, sizeof xbuf);
            LOG(ERROR, Server, 952, "%s%u%-s", "Forwarder configuration error: Forwarder={0} Error={2} Code={1}",
                            forwarder->name, ism_common_getLastError(), xbuf);
        }
    }

    if (rc == 0) {
        forwarder->need = need;
        if (rule) {
            pthread_spin_lock(&forwarder->lock);
            if (forwarder->selector)
                ism_common_freeSelectRule(forwarder->selector);
            if (!forwarder->selectors || !*forwarder->selectors) {
                forwarder->selector = NULL;
                forwarder->selector_len = 0;
            } else {
                forwarder->selector = rule;
                forwarder->selector_len = rulelen;
            }
            pthread_spin_unlock(&forwarder->lock);
        }
        if (created) {
            linkForwarder(forwarder);
        }
    } else {
        if (rule)
            ism_common_freeSelectRule(rule);
        if (created) {
            freeForwarder(forwarder);
        }
    }
    return rc;
}


/*
 * Delete matching forwarders
 */
int ism_bridge_deleteAllForwarder(const char * match, ism_json_parse_t * parseobj) {
    ism_forwarder_t * forwarder;
    int rc = 0;
    forwarder = ismForwarders;
    while (forwarder) {
        if (ism_common_match(forwarder->name, match)) {
            int xrc = ism_bridge_makeForwarder(parseobj, 0, forwarder->name, 0, 0);
            if (xrc)
                rc = xrc;
        }
        forwarder = forwarder->next;
    }
    return rc;
}

/*
 * Put out a forwarder list in JSON form
 *
 * This is used by the REST GET support
 */
int ism_bridge_getForwarderList(const char * match, ism_json_t * jobj, int json, const char * name) {
    ism_forwarder_t * forwarder;
    if (json)
        ism_json_startObject(jobj, name);
    else
        ism_json_startArray(jobj, name);

    pthread_mutex_lock(&bridgelock);
    forwarder = ismForwarders;
    while (forwarder) {
        if (ism_common_match(forwarder->name, match)) {
            if (json) {
                ism_bridge_getForwarderJson(jobj, forwarder, forwarder->name);
            } else {
                ism_json_putStringItem(jobj, NULL, forwarder->name);
            }
        }
        forwarder = forwarder->next;
    }
    pthread_mutex_unlock(&bridgelock);
    if (json)
        ism_json_endObject(jobj);
    else
        ism_json_endArray(jobj);
    return 0;
}


/*
 * Write out a forwarder as JSON
 *
 * The invoker should set the buf->compact as required
 * This is used by the REST GET support
 */
void ism_bridge_getForwarderJson(ism_json_t * jobj, ism_forwarder_t * forwarder, const char * name) {
    int i;

    ism_json_startObject(jobj, name);
    ism_json_startArray(jobj, "Topic");
    for (i=0; i<forwarder->topicCount; i++) {
        ism_json_putStringItem(jobj, NULL, forwarder->topic[i]);
    }
    ism_json_endArray(jobj);
    if (forwarder->enabled > 1)
        ism_json_putIntegerItem(jobj, "Enabled", forwarder->enabled);
    else
        ism_json_putBooleanItem(jobj, "Enabled", forwarder->enabled);
    ism_json_putIntegerItem(jobj, "SourceQoS", forwarder->subQoS);
    if (forwarder->instances)
        ism_json_putIntegerItem(jobj, "Instances", forwarder->instances);
    if (forwarder->source)
        ism_json_putStringItem(jobj, "Source", forwarder->source);
    if (forwarder->destination)
        ism_json_putStringItem(jobj, "Destination", forwarder->destination);
    if (forwarder->selectors)
        ism_json_putStringItem(jobj, "Selector", forwarder->selectors);
    if (forwarder->topicmap)
        ism_json_putStringItem(jobj, "TopicMap", forwarder->topicmap);
    if (forwarder->rulecount) {
        ism_json_startObject(jobj, "RoutingRule");
        for (i=0; i<forwarder->rulecount; i++) {
            ism_json_putStringItem(jobj, forwarder->rulenames[i], forwarder->rules[i]);
        }
        ism_json_endObject(jobj);
    }
    if (forwarder->keymap)
        ism_json_putStringItem(jobj, "KeyMap", forwarder->keymap);

    const char * pmap = forwarder->partitionmap;
    switch (forwarder->evst_partrule) {
    case PART_RULE_SINGLE:   pmap = "single";             break;
    case PART_RULE_ANY:      pmap = pmap ? "any" : NULL;  break;
    case PART_RULE_INSTANCE: pmap = "instance";           break;
    case PART_RULE_NUMBER:
        pmap = NULL;
        ism_json_putIntegerItem(jobj, "PartitionRule", forwarder->evst_part);
        break;
    }
    if (pmap)
        ism_json_putStringItem(jobj, "PartitionRule", pmap);

    if (forwarder->evst_ver >= 0 && forwarder->evst_ver <= 2)
        ism_json_putIntegerItem(jobj, "KafkaAPIVersion", forwarder->evst_ver);
    ism_json_endObject(jobj);
}

/*
 * Put out int16_t value as a JSON number
 */
void putJSONLong(ism_json_t * jobj, const char * name, uint64_t val) {
    char xbuf[64];
    printf(xbuf, "%ld", val);
    ism_json_putNumberItem(jobj, name, xbuf);
}

/*
 * Get a JSON forwarder status
 */
void ism_bridge_getForwarderStatus(concat_alloc_t * buf, ism_forwarder_t * forwarder) {
    int indent = (buf->compact & 0x20) ? 4 : 0;
    int opt = (buf->compact & 0xF) ? JSON_OUT_COMPACT : 0;
    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, buf, indent, opt);

    ism_json_startObject(jobj, NULL);
    ism_json_putStringItem(jobj, "Status", bridge_state_str(forwarder->active));
    if (forwarder->enabled > 1)
        ism_json_putIntegerItem(jobj, "Enabled", forwarder->enabled);
    else
        ism_json_putBooleanItem(jobj, "Enabled", forwarder->enabled);
    if (forwarder->source)
        ism_json_putStringItem(jobj, "Source", forwarder->source);
    if (forwarder->destination)
        ism_json_putStringItem(jobj, "Destination", forwarder->destination);

    putJSONLong(jobj, "SourceBytes", forwarder->source_bytes);
    putJSONLong(jobj, "SourceMsgs", forwarder->source_msgs);
    putJSONLong(jobj, "DestBytes", forwarder->dest_bytes);
    putJSONLong(jobj, "DestMsgs", forwarder->dest_msgs);
    putJSONLong(jobj, "SourceBytes", forwarder->source_bytes);

    if (forwarder->source_rc) {
        ism_json_putIntegerItem(jobj, "SourceRC", forwarder->source_rc);
        if (forwarder->source_reason) {
            ism_json_putStringItem(jobj, "SourceReason", forwarder->source_reason);
        }
    }
    if (forwarder->dest_rc) {
        ism_json_putIntegerItem(jobj, "DestRC", forwarder->dest_rc);
        if (forwarder->dest_reason) {
            ism_json_putStringItem(jobj, "DestReason", forwarder->dest_reason);
        }
    }
    ism_json_endObject(jobj);

    /* NUL terminate */
    ism_json_putNull(buf);
}

/*
 * Get an address from a connection object
 */
/*
 * Callback at completion of getaddrinfo_a
 */
#ifdef GAI_SIG
static int addrinfo_callback(void * xtransport) {
    ism_transport_t * transport = (ism_transport_t *)xtransport;
    struct gaicb * req = (struct gaicb *)transport->getAddrCB;
    struct addrinfo * info = req->ar_result;

    int grc = gai_error(req);
    if (grc != EAI_INPROGRESS) {
        if (grc == 0)
            grc = transport->slotused;
        transport->gotAddress(transport, grc, info);
        freeaddrinfo(info);
        ism_common_free(ism_memory_proxy_br_misc,req);
        return -1;
    }
    return 0;
}
#else
static void addrinfo_callback(union sigval xtransport) {
    ism_transport_t * transport = (ism_transport_t *)xtransport.sival_ptr;
    struct gaicb * req = (struct gaicb *)transport->getAddrCB;
    struct sigevent * sigevt = sigevt = (struct sigevent *)(req+1);
    struct addrinfo * info = req->ar_result;

    int grc = gai_error(req);
    if (grc == 0)
        grc = transport->slotused;
    transport->gotAddress(transport, grc, info);
    freeaddrinfo(info);
    ism_common_free(ism_memory_proxy_br_misc,req);           /* This includes the hints and sigevent */
}
#endif

/*
 * Get an address from a MHub object
 *
 */
static int getConnectionAddress(ism_server_t * server,  ism_transport_t * transport, ism_gotAddress_f gotAddress) {
    ism_connection_t * connection = (ism_connection_t *)server;
    int port;
    char * addr;
    int rc;
    struct gaicb * req = {0};
    struct sigevent * sigevt;
    struct addrinfo * hints;

    if (server)
        transport->server = server;
    else
        server = transport->server;
    if (gotAddress)
        transport->gotAddress = gotAddress;
    if (!server || !transport->gotAddress) {
        ism_common_setError(ISMRC_Error);
        return ISMRC_Error;
    }

    if (transport->client_host) {
        transport->server_addr = transport->client_host;
        port = transport->clientport;
        transport->slotused = 0;
    } else {
        char * pos;
        addr = alloca(strlen(connection->serverlist[connection->last_good]) + 1);
        strcpy(addr, connection->serverlist[connection->last_good]);
        transport->slotused = connection->last_good;
        pos = strrchr(addr, ':');
        if (pos) {
            *pos++ = 0;
            port = atoi(pos);
        } else {
            port = connection->secure ? 8883 : 1883;
        }
        transport->server_addr = ism_transport_putString(transport, addr);
    }
    transport->serverport = port;

    req = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_br_misc,17),1, sizeof(*req)+sizeof(*sigevt)+sizeof(*hints)+16);
    sigevt = (struct sigevent *)(req+1);
    hints = (struct addrinfo *)(sigevt+1);
    transport->getAddrCB = req;
    transport->slotused = connection->last_good;
    hints->ai_family = AF_INET6;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = AI_V4MAPPED;
    req->ar_name = transport->server_addr;
    req->ar_request = hints;
    req->__return = EAI_INPROGRESS;
#ifdef GAI_SIG
    sigevt->sigev_notify = SIGEV_SIGNAL;
    sigevt->sigev_signo = ism_common_userSignal();
    ism_handler_t handler = ism_common_addUserHandler(addrinfo_callback, transport);
#else
    sigevt->sigev_notify = SIGEV_THREAD;
    sigevt->sigev_value.sival_ptr = (void *)transport;
    sigevt->sigev_notify_function = addrinfo_callback;
#endif
    rc = getaddrinfo_a(GAI_NOWAIT, &req, 1, sigevt);
    if (rc) {
#ifdef GAI_SIG
        ism_common_removeUserHandler(handler);
#endif
        ism_common_free(ism_memory_proxy_br_misc,transport->getAddrCB);
        transport->getAddrCB = NULL;
        ism_common_setErrorData(ISMRC_Error, "%s%i", "getaddrinfo_a", rc);
        return ISMRC_Error;
    }
    return 0;
}


/*
 * Link this connection at the end of the list
 */
static void linkConnection(ism_connection_t * rconn) {
    rconn->next = NULL;
    ismConnectionsCount++;
    if (!ismConnections) {
        ismConnections = rconn;
    } else {
        ism_connection_t * connection = ismConnections;
        while (connection->next) {
            connection = connection->next;
        }
        connection->next = rconn;
    }
}

/*
 * Unlink this connection
 */
static void unlinkConnection(ism_connection_t * rconn) {
    if (ismConnections && rconn) {
        ismConnectionsCount--;
        if (ismConnections == rconn) {
            ismConnections = ismConnections->next;
        } else {
            ism_connection_t * connection = ismConnections;
            while (connection->next) {
                if (connection->next == rconn) {
                    connection->next = rconn->next;
                    break;
                }
                connection = connection->next;
            }
        }
    }
}


/*
 * Free a connection object.
 *
 * This is done only for failed configures as once we link in a connection
 * we do not ever free it.
 */
static void freeConnection(ism_connection_t * connection) {
    int  i;
    if (connection) {
        for (i=0; i<connection->servercount; i++) {
            replaceString(connection->serverlist+i, NULL);
        }
        replaceString(&connection->name, NULL);
        replaceString(&connection->ciphers, NULL);
        replaceString(&connection->keystore, NULL);
        replaceString(&connection->username, NULL);
        replaceString(&connection->password, NULL);
        ism_common_free(ism_memory_proxy_br_misc,connection);
    }
}

/*
 * Find the forwarder by name
 */
ism_connection_t * ism_bridge_getConnection(const char * name) {
    ism_connection_t * ret = ismConnections;
    if (!name)
        return NULL;
    while (ret) {
        if (!strcmp(name, ret->name))
            break;
        ret = ret->next;
    }
    return ret;
}


/*
 * Give the default ciphers based on TLS level
 */
static const char * defaultCiphers(int method) {
    return (method >= SecMethod_TLSv1_2) ?
        "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA:AES128-SHA" :   /* TLSv1.2 */
        "ECDHE-RSA-AES128-SHA:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA:AES128-SHA";                     /* TLSv1 and TLSv1.1 */
}

/*
 * Set the need field in a forwarder when a Connection changes
 *
 * This field indicates that the config has changed and we need to take further action.
 */
static void setForwarderNeed(const char * name, int need) {
    ism_forwarder_t * forwarder = ismForwarders;
    if (!name)
        return;
    while (forwarder) {
        if (forwarder->source && !strcmp(name, forwarder->source))
            forwarder->need |= need;
        forwarder = forwarder->next;
    }
}

/*
 * Make a tenant object from the configuration.
 */
int ism_bridge_makeConnection(ism_json_parse_t * parseobj, int where, const char * name, int checkonly, int keepgoing) {
    int endloc;
    ism_connection_t * connection;
    int  rc = 0;
    int  i;
    int  created = 0;
    int  namelen;
    int  needlog = 1;
    int  already_serverlist = 0;
    int  need = 0;    /* Need to update active connections (something significant changed) */
    char xbuf[1024];

    if (!parseobj || where > parseobj->ent_count)
        return 1;
    ism_json_entry_t * ent = parseobj->ent+where;
    endloc = where + ent->count;
    where++;
    if (!name)
        name = ent->name;
    namelen = name ? strlen(name) : 0;
    if (namelen < 1 || namelen > 32 || ism_common_validUTF8Restrict(name, namelen, (UR_NoControl | UR_NoNonchar))<0) {
        if (namelen > 500)
            namelen = 500;
        char * nbuf = alloca(namelen + 1);
        ism_common_validUTF8Replace(name, namelen, nbuf, (UR_NoControl | UR_NoNonchar), '?');
        LOG(ERROR, Server, 919, "%-s", "The name of a Connection is not valid: {0}", nbuf);
        ism_common_setErrorData(ISMRC_ArgNotValid, "%s", nbuf);
        return ISMRC_ArgNotValid;
    }
    connection = ism_bridge_getConnection(name);
    if (connection) {
        if (ent->objtype != JSON_Object) {
            unlinkConnection(connection);
            setForwarderNeed(name, 2);
            /*
             * We do not free up the connection object, so this memory will be seen as lost.
             * The assumption is that servers are not deleted often, and we do
             * not delete it as it might still be in used.  At some point we can
             * add in code to check if the server is in use and free it when it is
             * not.
             */
            return 0;
        }
    } else {
        if (ent->objtype == JSON_Object) {
            connection = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_br_misc,20),1, sizeof(ism_connection_t));
            strcpy(connection->structid, "BrConn");
            connection->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_br_misc,1000),name);
            connection->serverKind = 3;
            connection->version = 5;
            connection->getAddress = getConnectionAddress;
            pthread_spin_init(&connection->lock, 0);
            created = 1;
        } else {
            if (ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Connection", name);
                return ISMRC_BadPropertyValue;
            }
            TRACE(4, "Connection does not exist: %s\n", name);
            return 1;
        }
    }
    int savewhere = where;
    while (where <= endloc) {
        ent = parseobj->ent + where;
        if (ent->objtype == JSON_Null)
            ent->count = 0;
        if (!strcmp(ent->name, "MQTTServerList") |
                   !strcmp(ent->name, "EventStreamsBrokerList")
                   ) {
            if (ent->objtype != JSON_Array) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            } else {
                int addrcount = ent->count;
                if(addrcount > MAX_MQTT_SERVERS){ // We support at most 16 MQTT servers
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                    rc = ISMRC_BadPropertyValue;
                }
                else {
                    for (i=0; i<addrcount; i++) {
                        if (parseobj->ent[where+i+1].objtype != VT_String) {
                            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                            rc = ISMRC_BadPropertyValue;
                            break;
                        }
                    }
                }
            }
            /* Do not allow both forms of server list to be in the same JSON object */
            if (rc == 0 && already_serverlist) {
                g_need_dyn_write = 1;
                LOG(WARN, Server, 953, "%s%u%-s", "Connection configuration error: Connection={0} Error={2} Code={1}",
                            connection->name, ISMRC_BadPropertyName, "MQTTServerList and EventStreams cannot both be used");
                if (!keepgoing) {
                    rc = ISMRC_BadPropertyName;
                    ism_common_setErrorData(rc, "%s", ent->name);
                }
            }
            already_serverlist = 1;
        } else if (!strcmp(ent->name, "Username")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Username", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "Password")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Password", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "ServerName")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ServerName", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "ClientID")) {
            if (ent->objtype != JSON_String) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "ClientID", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "MaxPacketSize")) {
            if ((ent->objtype != JSON_Integer || ent->count < 0) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MaxPacketSize", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "MaxBatchTimeMS")) {
            if ((ent->objtype != JSON_Integer || ent->count < 0) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MaxBatchTimeMS", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "SessionExpiry")) {
            if ((ent->objtype != JSON_Integer || ent->count < 0) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "SessionExpiry", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "Version")) {
            if ((ent->objtype != JSON_String || ism_common_enumValue(enum_versions, ent->value)==INVALID_ENUM) &&
                (ent->objtype != JSON_Integer || ent->count < 3 || ent->count > 5) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Version", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "TLS")) {
            if ((ent->objtype != JSON_String || ism_common_enumValue(enum_methods, ent->value)==INVALID_ENUM) &&
                 ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "TLS", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "Ciphers")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Ciphers", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "Keystore") && ent->objtype != JSON_Null) {
            if (ent->objtype != JSON_String) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Keystore", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else {
            LOG(ERROR, Server, 939, "%s%-s", "Unknown connection property: Connection={0} Property={1}",
                    connection->name, ent->name);
            needlog = 0;
            g_need_dyn_write = 1;
            if (!keepgoing) {
                rc = ISMRC_BadPropertyName;
                ism_common_setErrorData(rc, "%s", ent->name);
            }
        }
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }

    /* Do only the checking */
    if (checkonly)
        return rc;

    if (rc == 0) {
        where = savewhere;
        while (where <= endloc) {
            ent = parseobj->ent + where;
            if (!strcmp(ent->name, "MQTTServerList") |
                   !strcmp(ent->name, "EventStreamsBrokerList")
                   ) {
                int addrcount = ent->count;
                if (addrcount != connection->servercount)
                    need |= 1;
                for (i=0; i<addrcount; i++) {
                    if (replaceString(connection->serverlist+i, parseobj->ent[where+i+1].value))
                        need |= 1;
                }
                for (; i<16; i++) {
                    replaceString(connection->serverlist+i, NULL);
                }
                if (ent->name[1] == 'Q' || ent->name[1] == 'q')
                    connection->isEventStream = 0;
                else
                    connection->isEventStream = 1;
                connection->servercount = addrcount;
            } else if (!strcmp(ent->name, "MaxPacketSize")) {
                if (connection->maxPacketSize != ent->count)
                    need |= 1;
                connection->maxPacketSize = ent->count;
            } else if (!strcmp(ent->name, "MaxBatchTimeMS")) {
                if (connection->maxBatchTimeMS != ent->count)
                    need |= 1;
                connection->maxBatchTimeMS = ent->count;
            } else if (!strcmp(ent->name, "SessionExpiry")) {
                if (connection->sessionExpiry != ent->count)
                    need |= 1;
                connection->sessionExpiry = ent->count;
            } else if (!strcmp(ent->name, "Version")) {
                int version = ent->count;
                if (ent->objtype == JSON_String)
                    version = ism_common_enumValue(enum_versions, ent->value);
                if (ent->objtype == JSON_Null)
                    version = 5;
                if (connection->version != version)
                    need |= 1;
                connection->version = version;
            } else if (!strcmp(ent->name, "ClientID")) {
                need |= replaceString(&connection->clientID, ent->value);
            } else if (!strcmp(ent->name, "Username")) {
                if (ent->value && !*ent->value)
                    ent->value = NULL;
                need |= replaceString(&connection->username, ent->value);
            } else if (!strcmp(ent->name, "Password")) {
                if (ent->value && !*ent->value)
                    ent->value = NULL;
                need |= replaceString(&connection->password, ent->value);
                if (ent->value && *ent->value != '!')
                     g_need_dyn_write = 1;
            } else if (!strcmp(ent->name, "ServerName")) {
                if (ent->value && !*ent->value)
                    ent->value = NULL;
                replaceString(&connection->serverName, ent->value);
            } else if (!strcmp(ent->name, "Ciphers")) {
                if (ent->value && !*ent->value)
                    ent->value = NULL;
                need |= replaceString(&connection->ciphers, ent->value);
            } else if (!strcmp(ent->name, "Keystore")) {
                need |= replaceString(&connection->keystore, ent->value);
            } else if (!strcmp(ent->name, "BridgeProtocol")) {
                uint8_t bval = ent->objtype == JSON_True;
                if (connection->bridge != bval)
                    need |= 1;
                connection->bridge = bval;
            } else if (!strcmp(ent->name, "TLS")) {
                int tlsmethod = 0;
                if (ent->objtype != JSON_Null)
                    tlsmethod = ism_common_enumValue(enum_methods, ent->value);
                if (connection->secure != tlsmethod)
                    need |= 1;
                connection->secure = tlsmethod;
            }
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }
    } else {
        if (needlog) {
            ism_common_formatLastError(xbuf, sizeof xbuf);
            LOG(ERROR, Server, 953, "%s%u%-s", "Connection configuration error: Connection={0} Error={2} Code={1}",
                            connection->name, ism_common_getLastError(), xbuf);
        }
    }

    if (rc == 0) {
        if (need) {
            int updateRc = 0;
            if (connection->tlsCTX) {
                // ism_common_destroy_ssl_ctx(connection->tlsCTX);
                connection->tlsCTX = NULL;
            }
            if (connection->secure) {
                connection->tlsCTX = ism_transport_clientTlsContext(connection->name,
                    ism_common_enumName(enum_methods, connection->secure),
                    connection->ciphers ? connection->ciphers : defaultCiphers(connection->secure));
                if (!connection->tlsCTX) {
                    LOG(ERROR, Server, 912, "%s", "The client TLS Context cannot be created: Connection={0}",
                            connection->name);
                    TRACE(2, "Unable to create client TLS context: connection=%s\n", connection->name);
                    connection->rc = ISMRC_CreateSSLContext;
                    updateRc = ISMRC_CreateSSLContext;
                    replaceString(&connection->reason, "Unable to create client TLS context");
                } else {
                    TRACE(5, "Create TLS context: server=%s method=%s\n",
                             connection->name, ism_common_enumName(enum_methods, connection->secure));
                }
            }

            /* Update forwarders which use this connection */
            setForwarderNeed(name, need);

            if (updateRc == 0) {
                connection->rc = 0;
                replaceString(&connection->reason, NULL);
            }
        }

        if (created) {
            linkConnection(connection);
        }
    } else {
        if (created) {
            freeConnection(connection);
        }
    }
    return rc;
}

/*
 * Delete matching connections
 */
int ism_bridge_deleteAllConnection(const char * match, ism_json_parse_t * parseobj) {
    ism_connection_t * connection;
    int rc = 0;
    connection = ismConnections;
    while (connection) {
        if (ism_common_match(connection->name, match)) {
            int xrc = ism_bridge_makeConnection(parseobj, 0, connection->name, 0, 0);
            if (xrc)
                rc = xrc;
        }
        connection = connection->next;
    }
    return rc;
}

/*
 * Get the file contents into a buffer
 */
int ism_bridge_getFileContents(const char * fname, concat_alloc_t * buf) {
    int len;
    FILE * f;

    /* Open the file and get the length */
    f = fopen(fname, "rb");
    if (!f) {
        return 1;    /* File cannot be opened */
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    ism_protocol_ensureBuffer(buf, len+1);
    rewind(f);

    /* Read the file */
    buf->used = fread(buf->buf, 1, len, f);
    buf->buf[buf->used] = 0;   /* Null terminate */
    if (buf->used != len) {
        fclose(f);
        return 3;
    }
    buf->buf[buf->used] = 0;   /* Null terminate */
    fclose(f);
    return 0;
}

/*
 * Get the dynamic config into a buffer
 */
void ism_bridge_getDynamicConfig(ism_json_t * jobj) {
    const char * val;
    ism_json_startObject(jobj, NULL);
    ism_json_putStringItem(jobj, "LicensedUsage", ism_common_enumName(enum_licenses, g_licensedUsage));
    if (g_dynamic_loglevel) {
        val = ism_common_getStringConfig("LogLevel");
        if (val)
            ism_json_putStringItem(jobj, "LogLevel", val);
    }
    if (g_dynamic_tracelevel) {
        val = ism_common_getStringConfig("TraceLevel");
        if (val)
            ism_json_putStringItem(jobj, "TraceLevel", val);
    }
    ism_bridge_getConnectionList("*", jobj, 1, "Connection");
    ism_bridge_getForwarderList("*", jobj, 1, "Forwarder");
    ism_transport_getEndpointList("*", jobj, 1, "Endpoint");
    ism_tenant_getUserList("*", NULL, jobj, 1, "User");
    ism_json_endObject(jobj);
}


/*
 * Update the config file.
 * The config file is updated in place after
 */
int ism_bridge_updateDynamicConfig(int doBackup) {
    int nobackup;
    int bwritten;
    FILE * configf;
    char xbuf[8192];
    char cbuf[8192];
    char tbuf[64];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    concat_alloc_t curbuf = {cbuf, sizeof cbuf};

    const char * dyncfg = ism_common_getStringConfig("DynamicConfig");
    if (!dyncfg)
        dyncfg = "bridge.cfg";
    nobackup  = ism_bridge_getFileContents(dyncfg, &curbuf);
    configf = fopen(dyncfg, "wb");
    if (!configf) {
        LOG(ERROR, Server, 971,"%s", "Configuration updates were not written; unable to open dynamic config file for update: {0}", dyncfg);
        return ISMRC_Error;
    }

    /* Get the new config into a buffer */
    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
    ism_common_formatTimestamp(ts, tbuf, sizeof(tbuf), 6, ISM_TFF_ISO8601_2);
    sprintf(buf.buf, "/* imabridge dynamic config updated %s */\n", tbuf);
    buf.used = strlen(buf.buf);
    ism_common_closeTimestamp(ts);
    ism_json_t xjobj = {0};
    ism_json_t * jobj = ism_json_newWriter(&xjobj, &buf, 4, 0);
    ism_bridge_getDynamicConfig(jobj);

    /* Backup the config */
    if  (!nobackup && curbuf.used > 0) {
        FILE * backf;
        char * backup = alloca(strlen(dyncfg)+16);
        strcpy(backup, dyncfg);
        char * ext = strrchr(backup, '/');
        if (!ext)
            ext = backup;
        ext = strchr(backup, '.');
        if (ext) {
            ext = strchr(ext, '.');
            if (!ext)
                ext = backup + strlen(backup);
            strcpy(ext, ".bak");
        }
        backf = fopen(backup, "wb");
        if (backf) {
            fwrite(curbuf.buf, curbuf.used, 1, backf);
            fclose(backf);
        }
    }

    if (SHOULD_TRACEC(9, Admin) || SHOULD_TRACEC(9, Mqtt)) {
        TRACE(1, "Dynamic configuration updated: %s", buf.buf);
    }
    /* Update the config */
    bwritten = fwrite(buf.buf, 1, buf.used, configf);
    fclose(configf);
    if (bwritten != buf.used) {
        LOG(ERROR, Server, 972,"%s", "Configuration updates were not written; unable to write dynamic config file for update: {0}", dyncfg);
        return 1;
    }
    LOG(NOTICE, Server, 973,"%s", "The configuration is updated: {0}", dyncfg);
    return 0;
}

/*
 * Put out a connection list in JSON form
 *
 * This is used by the REST GET support
 */
int ism_bridge_getConnectionList(const char * match, ism_json_t * jobj, int json, const char * name) {
    ism_connection_t * connection;
    if (json)
        ism_json_startObject(jobj, name);
    else
        ism_json_startArray(jobj, name);

    pthread_mutex_lock(&bridgelock);
    connection = ismConnections;
    while (connection) {
        if (ism_common_match(connection->name, match)) {
            if (json) {
                ism_bridge_getConnectionJson(jobj, connection, connection->name);
            } else {
                ism_json_putStringItem(jobj, NULL, connection->name);
            }
        }
        connection = connection->next;
    }
    pthread_mutex_unlock(&bridgelock);
    if (json)
        ism_json_endObject(jobj);
    else
        ism_json_endArray(jobj);
    return 0;
}

/*
 * Write out a forwarder as JSON
 * The invoker should set the buf->compact as required
 *
 * This is used by the REST GET support
 */
void ism_bridge_getConnectionJson(ism_json_t * jobj, ism_connection_t * connection, const char * name) {
    int i;

    ism_json_startObject(jobj, name);

    if (connection->isEventStream)
        ism_json_startArray(jobj, "EventStreamsBrokerList");
    else
        ism_json_startArray(jobj, "MQTTServerList");
    for (i=0; i<connection->servercount; i++) {
         ism_json_putStringItem(jobj, NULL, connection->serverlist[i]);
    }
    ism_json_endArray(jobj);

    if (connection->serverName)
        ism_json_putStringItem(jobj, "ServerName",connection->serverName);
    if (connection->clientID)
        ism_json_putStringItem(jobj, "ClientID", connection->clientID);
    if (connection->version)
        ism_json_putStringItem(jobj, "Version", ism_common_enumName(enum_versions, connection->version));
    if (connection->secure)
        ism_json_putStringItem(jobj, "TLS", ism_common_enumName(enum_methods, connection->secure));
    if (connection->ciphers)
        ism_json_putStringItem(jobj, "Ciphers",connection->ciphers);
    if (connection->keystore)
        ism_json_putStringItem(jobj, "Keystore", connection->keystore);
    if (connection->username)
        ism_json_putStringItem(jobj, "Username", connection->username);
    if (connection->password)  {
        if (*connection->password != '!') {
            int obfus_len = strlen(connection->password) * 2 + 40;
            char * obfus = alloca(obfus_len);
            ism_transport_makepw(connection->password, obfus, obfus_len-1, 0);
            ism_json_putStringItem(jobj, "Password", obfus);
        } else {
            ism_json_putStringItem(jobj, "Password", connection->password);
        }
    }
    ism_json_putIntegerItem(jobj, "SessionExpiry", connection->sessionExpiry);
    if (connection->maxPacketSize > 0)
        ism_json_putIntegerItem(jobj, "MaxPacketSize", connection->maxPacketSize);
    if (connection->maxBatchTimeMS > 0)
        ism_json_putIntegerItem(jobj, "MaxBatchTimeMS", connection->maxBatchTimeMS);
    ism_json_endObject(jobj);
}


/*
 * Stop all source connections
 */
void ism_bridge_stopSource(void) {
    pthread_mutex_lock(&bridgelock);
    g_shuttingDown = 1;
    ism_forwarder_t * forwarder = ismForwarders;

    while (forwarder) {
        if (forwarder->s_transport) {
             ism_transport_t * transport = forwarder->s_transport;
             transport->close(transport, ISMRC_ServerTerminating, 0, "The connection was closed due to shutdown.");
        }
        forwarder = forwarder->next;
    }
    pthread_mutex_unlock(&bridgelock);
    ism_common_sleep(10000);    /* 10 ms */
}


/*
 * Command to print tenants
 *
 * This is used by the non-daemon command line support for debugging
 */
void ism_bridge_printForwarders(const char * pattern, int status) {
    int i;
    pthread_mutex_lock(&bridgelock);
    ism_forwarder_t * forwarder = ismForwarders;
    int nullonly = 0;
    if (!pattern)
        pattern = "*";
    if (pattern[0]=='.' && pattern[1]==0)
        nullonly = 1;
    while (forwarder) {
        int selected = 0;
        if (nullonly) {
            if (forwarder->name[0]==0)
                selected = 1;
        } else {
            if (ism_common_match(forwarder->name, pattern))
                selected = 1;
        }
        if (*pattern == '*') {
            if (forwarder->instof < 0 && forwarder->instances > 0 && forwarder->active == BCS_None)     /* Base of a multi instance forwarder */
                selected = 0;
            if (forwarder->active == BCS_Disabled)
                selected = 0;
        }
        if (selected) {
            if (status) {
                printf("Forwarder \"%s\" State=%s Source=%s Dest=%s\n",
                       forwarder->name, bridge_state_str(forwarder->active), forwarder->source, forwarder->destination);
                printf("    SourceBytes=%lu SourceMsgs=%lu  DestBytes=%lu DestMsgs=%lu\n",
                            forwarder->source_bytes, forwarder->source_msgs,
                            forwarder->dest_bytes, forwarder->dest_msgs);
            } else {
                printf("Forwarder \"%s\" State=%s Enabled=%u Need=%u SourceQos=%u\n",
                       forwarder->name, bridge_state_str(forwarder->active), forwarder->enabled, forwarder->need, forwarder->subQoS);
                printf("    Source=%s Destination=%s\n", forwarder->source, forwarder->destination);
                for (i=0; i<forwarder->topicCount; i++)
                    printf("    Topic%u=\"%s\"\n", i, forwarder->topic[i]);
                if (forwarder->selectors)
                    printf("    Selector=\"%s\"\n", forwarder->selectors);
                if (forwarder->topicmap)
                    printf("    TopicMap=\"%s\"\n", forwarder->topicmap);
                for (i=0; i<forwarder->rulecount; i++)
                    printf("    RoutingRule%u=\"%s: %s\"\n", i, forwarder->rulenames[i], forwarder->rules[i]);
                if (forwarder->keymap)
                    printf("    KeyMap=\"%s\"\n", forwarder->keymap);
                if (forwarder->partitionmap)
                    printf("    PartitionRule=\"%s\"\n", forwarder->keymap);
                if (forwarder->source_rc) {
                    printf("    SourceRC=%u Reason=%s\n", forwarder->source_rc,
                            forwarder->source_reason ? forwarder->source_reason : "");
                }
                if (forwarder->dest_rc) {
                    printf("    DestRC=%u Reason=%s\n", forwarder->dest_rc,
                            forwarder->dest_reason ? forwarder->dest_reason : "");
                }
                if (forwarder->source_bytes != 0) {
                    printf("    SourceBytes=%lu SourceMsgs=%lu  DestBytes=%lu DestMsgs=%lu\n",
                            forwarder->source_bytes, forwarder->source_msgs,
                            forwarder->dest_bytes, forwarder->dest_msgs);
                }
            }
        }
        forwarder = forwarder->next;
    }
    pthread_mutex_unlock(&bridgelock);
}

/*
 * Command to print tenants
 *
 * This is used by the non-daemon command line support for debugging
 */
void ism_bridge_printConnections(const char * pattern) {
    pthread_mutex_lock(&bridgelock);
    ism_connection_t * connection = ismConnections;
    int nullonly = 0;
    if (!pattern)
        pattern = "*";
    if (pattern[0]=='.' && pattern[1]==0)
        nullonly = 1;
    while (connection) {
        int selected = 0;
        if (nullonly) {
            if (connection->name[0]==0)
                selected = 1;
        } else {
            if (ism_common_match(connection->name, pattern))
                selected = 1;
        }
        if (selected) {
            printf("Connection \"%s\" Server1=%s Server2=%s\n", connection->name,
                   connection->serverlist[0],
                   connection->servercount > 1 ? connection->serverlist[1] : "");
            if (connection->serverName) {
                printf("    ServerName=%s\n", connection->serverName);
            }
            printf("    ClientID=%s SessionExpiry=%u MaxPacketSize=%u Username=%s\n", connection->clientID,
                    connection->sessionExpiry, connection->maxPacketSize,
                    connection->username ? connection->username : "");
            if (connection->secure) {
                printf("    TLS=%s Keystore=%s Ciphers=%s\n",
                        ism_common_enumName(enum_methods, connection->secure),
                        connection->keystore ? connection->keystore : "",
                        connection->ciphers ? connection->ciphers : "");
            }
            if (*pattern != '*') {
                char xbuf [4096];
                concat_alloc_t buf = {xbuf, sizeof xbuf};
                ism_json_t xjobj = {0};
                ism_json_t * jobj = ism_json_newWriter(&xjobj, &buf, 4, 0);
                ism_bridge_getConnectionJson(jobj, connection, NULL);
                printf(buf.buf);
            }
        }
        connection = connection->next;
    }
    pthread_mutex_unlock(&bridgelock);
}



/*
 * Define how long to delay retry based on retrycount
 */
static int delaysec(int retries) {
    int ret = (retries * 2) + 1;
    if (ret > 60)
        ret = 60;
    return ret;
}


/*
 * Implement transport->closing for bridge connections
 */
int ism_bridge_closing(ism_transport_t * transport, int rc, int good, const char * reason) {
    mqttbr_pobj_t * pobj = transport->pobj;
    ism_time_t now = ism_common_currentTimeNanos();
    ism_forwarder_t * forwarder;
    char response [1024];
    uint8_t oldstate = 0;
    uint8_t newstate = 0;

    /*
     * Set the indicator that close is in progress. If set failed,
     * then this has been done before and we don't need to proceed.
     */
    if (!__sync_bool_compare_and_swap(&pobj->closed, 0, 1)) {
        return 0;
    }

    TRACE(7, "Bridge outgoing connection closing: connect=%u client=%s source=%u rc=%d reason=%s\n",
            transport->index, transport->name, pobj->isSource, rc, reason);

    /*
     * Send a DISCONNECT with the RC and reason
     * If we are disabling or deleting the connection, then set SessionExpiry=0 to close the session
     */
    if (rc == 0 && pobj->mqtt_version >= 5 && transport->ready == 1) {
        transport->ready = 2;
        concat_alloc_t rbuf = {response, sizeof response};

        response[16] = ism_proxy_mapToMqttRC(rc, pobj->mqtt_version);
        response[17] = 0;
        rbuf.used = 17;
        if (rc && (reason || pobj->disabling)) {
            int expirylen = pobj->disabling ? 5 : 0;
            int reasonlen = reason ? strlen(reason) + 3 : 0;
            ism_common_putMqttVarInt(&rbuf, reasonlen+expirylen);
            if (expirylen)
                ism_common_putMqttPropField(&rbuf, MPI_SessionExpire, g_ctx5, 0);    /* Clean the session */
            if (reasonlen)
                ism_common_putMqttPropString(&rbuf, MPI_Reason, g_ctx5, reason, reasonlen-3);
        } else {
            rbuf.used++;            /* proplen = 0 */
        }
        transport->send(transport, rbuf.buf + 16, rbuf.used-16, MT_DISCONNECT << 4, SFLAG_FRAMESPACE);

        if (rbuf.inheap)
            ism_common_freeAllocBuffer(&rbuf);
    }

    transport->close_rc = rc;
    if (rc && reason && *reason && !transport->reason)
        transport->reason = ism_transport_putString(transport, reason);

    forwarder = pobj->forwarder;

    if (transport->ready) {
        uint32_t uptime = (uint32_t)(((ism_common_currentTimeNanos()-transport->connect_time)+500000000)/1000000000);  /* in seconds */
        LOG(NOTICE, Connection, 1302, "%s%d%s%s%u%d%-s%lu%lu%lu%lu%s", "Closing MQTT connection: Forwarder={0} Dest={1} Name={2} Protocol={3}"
                " Secure={11} Uptime={4} RC={5} Reason={6} ReadBytes={7} ReadMsg={8} WriteBytes={9} WriteMsg={10}",
                forwarder->name, !transport->pobj->isSource, transport->name, transport->protocol, uptime, rc, reason,
                transport->read_bytes, transport->read_msg, transport->write_bytes, transport->write_msg,
                ism_transport_getTLSMethodName(transport));
    }

    /*
     * Close the other side of the forwarder connection
     */
    if (pobj->isSource) {
        if (pobj->dest_transport && pobj->forwarder && pobj->dest_transport == pobj->forwarder->d_transport && !g_shuttingDown) {
            pobj->dest_transport->close(pobj->dest_transport, rc, rc==0, reason);
        }
        pthread_spin_lock(&forwarder->lock);
        oldstate = newstate = forwarder->active;
        forwarder->source_rc = rc;
        replaceString(&forwarder->source_reason, reason);
        if (forwarder->active != BCS_Disabled) {
            forwarder->active = newstate = BCS_Wait;
            if (rc) {
                if (forwarder->retrycount[0] > 2 && !forwarder->retryLogged[0]) {
                    forwarder->retryLogged[0] = 1;
                    LOG(WARN, Server, 982, "%s%s%s", "Unable to connect to MQTT server: Forwarder={0} Name={1} Server={2}",
                        forwarder->name, transport->name, transport->client_host ? transport->client_host : transport->server_addr);
                } else if (forwarder->retrycount[0]) {
                    LOG(NOTICE, Server, 982, "%s%s%s", "Unable to connect to MQTT server: Forwarder={0} Name={1} Server={2}",
                        forwarder->name, transport->name, transport->client_host ? transport->client_host : transport->server_addr);
                }
            }
            forwarder->waitfrom = now + BILLION;      /* one second */
            forwarder->waituntil = now + (delaysec(forwarder->retrycount[0]++) * BILLION);
        }
        pthread_spin_unlock(&forwarder->lock);
    } else {
        if (pobj->source_transport && !g_shuttingDown) {
            pobj->source_transport->close(pobj->source_transport, rc, rc==0, reason);
        }
        pthread_spin_lock(&forwarder->lock);
        oldstate = newstate = forwarder->active;
        forwarder->dest_rc = rc;
        replaceString(&forwarder->dest_reason, reason);
        if (forwarder->active != BCS_Disabled) {
            forwarder->active = BCS_Wait;
            if (rc) {
                if (forwarder->retrycount[1] > 2 && !forwarder->retryLogged[1]) {
                    forwarder->retryLogged[1] = 1;
                    LOG(WARN, Server, 982, "%s%s%s", "Unable to connect to MQTT server: Forwarder={0} Name={1} Server={2}",
                        forwarder->name, transport->name, transport->client_host ? transport->client_host : transport->server_addr);
                } else if (forwarder->retrycount[1]) {
                    LOG(NOTICE, Server, 982, "%s%s%s", "Unable to connect to MQTT server: Forwarder={0} Name={1} Server={2}",
                        forwarder->name, transport->name, transport->client_host ? transport->client_host : transport->server_addr);
                }
            }
            forwarder->waitfrom = now + BILLION;      /* one second */
            forwarder->waituntil = now + (delaysec(forwarder->retrycount[1]++) * BILLION);
        }
        pthread_spin_unlock(&forwarder->lock);
    }
    if (newstate != oldstate) {
        TRACE(6, "Change forwarder state at closing: forwarder=%s state=%s oldstate=%s dest=%u\n", forwarder->name,
                bridge_state_str(newstate), bridge_state_str(oldstate), !pobj->isSource);
        LOG(NOTICE, Server, 984, "%s%s", "The state of forwarder {0} is now: {1}",
                            forwarder->name, bridge_state_str(newstate));
    } else {
        TRACE(9, "Forwarder state not changed at closing: forwarder=%s state=%s dest=%u\n",
                forwarder->name, bridge_state_str(oldstate), !pobj->isSource);
    }

    /*
     * Free up any saved send data
     */
    if (pobj->senddata_len) {
        pobj->senddata_len = 0;
        ism_common_free(ism_memory_proxy_br_misc,pobj->senddata);
        pobj->senddata = NULL;
        pobj->senddata_alloc = 0;
    }

    pthread_spin_destroy(&pobj->lock);
    transport->closed(transport);

    return 0;
}


/*
 * Reflect mhub state changes into the forwarder
 */
void mhubStateChanged(ism_mhub_t * mhub) {
    ism_forwarder_t * forwarder = (ism_forwarder_t *)mhub->userdata;
    if (mhub->state == MHS_Active) {
        if (forwarder->active == BCS_ConnectDest) {
            forwarder->active = BCS_ConnectSource;
            TRACE(6, "Change forwarder state: forwarder=%s state=%s\n",
                    forwarder->name, bridge_state_str(forwarder->active));
            ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)createConnect, forwarder, 1000);
        }
    }
}

/*
 * Create or activate a MHub destination
 */
static int createMHubDest(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    ism_forwarder_t * forwarder = (ism_forwarder_t *) userdata;
    ism_connection_t * connection = NULL;
    const char * connection_name;
    int rc;
    int i;
    ism_mhub_t * mhub;
    char xbuf [2048];

    /* If this is a timer, it is only run once */
    if (key)
        ism_common_cancelTimer(key);

    /* If we are shutting down, do nothing */
    if (g_shuttingDown)
        return 0;

    connection_name = forwarder->destination;
    if (!connection_name)
        connection_name = "NotSet";    /* This will generate an error */
    else
        connection = ism_bridge_getConnection(connection_name);

    /* If the named connection is not found, set state to Failed, only a config change will cause a retry */
    if (!connection) {
        forwarder->active = BCS_Failed;
        LOG(NOTICE, Server, 984, "%s%s", "The state of forwarder {0} is now: {1}",
                forwarder->name, bridge_state_str(forwarder->active));
        rc = ISMRC_NotFound;
        forwarder->dest_rc = rc;
        snprintf(xbuf, sizeof xbuf, "Connection object not found: Connection=%s Forwarder=%s",
                connection_name, forwarder->name);
        replaceString(&forwarder->dest_reason, xbuf);
        ism_common_setErrorData(rc, "%s%s", connection_name, forwarder->name);
        return rc;
    }

    /*
     * Check whether we already have an mhub object
     */
    mhub = forwarder->mhub;
    if (!mhub) {
        mhub =  ism_mhub_newMhub(forwarder->name, g_bridge_tenant, forwarder->evst_ver);
        mhub->tlsCTX = connection->tlsCTX;
        mhub->need = 2;
        strcpy(mhub->serviceid, "bridge");
        forwarder->mhub = mhub;
        mhub->mhubACK = 1;
        mhub->moreLogs = 1;
        mhub->userdata = (void *)forwarder;
        mhub->stateChanged = mhubStateChanged;
        mhub->maxBatchBytes = connection->maxPacketSize ? connection->maxPacketSize : ism_mhub_getMaxBatchSizeBytes();
        mhub->maxBatchMsgs = ism_mhub_getMaxBatchSizeMsgs();
        mhub->maxBatchTimeMS = connection->maxBatchTimeMS > 0 ? connection->maxBatchTimeMS : ism_mhub_getMaxBatchTimeMillis();
        if (connection->username)
            replaceString(&mhub->username, connection->username);
        if (connection->password) {
            if (connection->password && *connection->password == '!') {
                int pwbuflen = strlen(connection->password) + 32;
                char * pw = alloca(pwbuflen);
                ism_transport_makepw(connection->password, pw, pwbuflen, 1);
                replaceString(&mhub->password, pw);
            } else {
                replaceString(&mhub->password, connection->password);
            }
            mhub->mhubSASL = 1;
        }
    }
    ism_mhub_lock(mhub);

    /*
     * Check whether the destination mhub is already active
     */
    if (forwarder->evst_need) {
        for (i=0; i<connection->servercount; i++) {
            replaceString(&mhub->brokers[i], connection->serverlist[i]);
        }
        mhub->broker_count = connection->servercount;

        ism_mhub_updateSelRules(mhub, forwarder->rulenames, forwarder->rules, forwarder->rulecount);
    }
    mhub->enabled = 1;
    ism_mhub_unlock(mhub);
    return 0;
}

/*
 * Setup protocol portion of pobj
 *
 * This is the common mechanism for incomming connections, but we use it for outgoing as well.
 * The purpose is to fill in the transport fields set by protocol.  Any string in the transport
 * object must either be constant or allocated within the transport object.
 */
int ism_bridge_connection(ism_transport_t * transport) {
    int rc = 1;
    mqttbr_pobj_t * pobj = NULL;
    /*
     * Binary MQTT over tcp
     */
    if (!strcmp(transport->protocol, "mqtt-tcp") || !strcmp(transport->protocol, "mqtt4-tcp") ||
             !strcmp(transport->protocol, "mqtt5-tcp")) {
        pobj = (mqttbr_pobj_t *) ism_transport_allocBytes(transport, sizeof(mqttbr_pobj_t), 1);
        memset((char *) pobj, 0, sizeof(mqttbr_pobj_t));
        transport->pobj = pobj;
        transport->dumpPobj = NULL;
        transport->receive = ism_bridge_receive;
        transport->actionname = ism_mqtt_mqttCommand;
        rc = 0;
    }

    /*
     * If we have a valid protocol, do common initialization
     */
    if (rc == 0) {
        transport->protocol_family = "mqtt"; /* Constant string */
        transport->auth_permissions = 0xffff;
        transport->closing = ism_bridge_closing;
        transport->connected = ism_bridge_connected;
        transport->pobj->transport = transport;
        transport->tid = transport->index % ism_tcp_getIOPcount();
        pthread_spin_init(&pobj->lock, 0);
    }
    return rc;
}
