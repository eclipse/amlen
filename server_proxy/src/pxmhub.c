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
 * This file contains methods related to direct write from the messaging
 * proxy to Event Streams.  This is also used by Bridge.
 * While this is designed to send to IBM Event Streams, it works for other kafka instances
 * which are similarly configured.  The Kafka must be at least version 0.10 as we determine
 * the Kafka support by doing an ApiVersions request.
 *
 * If the Kafka version is 1.0 or above, we assume that msgfmt=2 is enabled.  This is supported
 * by Kafka 0.11 and above, and should be enabled before moving to 1.0.  msgfmt=2 implements
 * headers and is used as part of the MQTTv5 support.
 *
 * If the user supplies a password we assume that the Kafka instance uses SASL PLAIN as it is
 * the only SASL method we support.  If there is no password we assume no authentication is
 * required.
 *
 * We only support TLS
 */
#ifndef TRACE_COMP
#define TRACE_COMP Kafka
#endif
#include <ismutil.h>
#include <ismjson.h>
#include <pxtransport.h>
#include <pxtcp.h>
#include <tenant.h>
#include <imacontent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pxkafka.h>
#include <pxrouting.h>
#include <pxmhub.h>
#include <pxmqtt.h>
#include <protoex.h>
#include <sasl_scram.h>
#define MAX_MQTT_SERVERS 16


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
 * Return the string representing the Mhub state
 */
static const char * mhubStateString(int state) {
    switch (state) {
    case MHS_Config:  return "Config";   /* Configured but not started */
    case MHS_Failed:  return "Failed";   /* Broker connection failed, need new config */
    case MHS_Wait:    return "Wait";     /* Waiting for a broker connection */
    case MHS_Opening: return "Opening";  /* Some data connections are not open */
    case MHS_Closing: return "Closing";  /* Closing connections due to shutdown or config change */
    case MHS_Active:  return "Active";   /* All data connections are open */
    default:  return "Unknown";
    }
}

/* API values */
#define ProduceRequest          0x0000
#define FetchRequest            0x0001
#define ListOffsetsRequest      0x0002
#define MetadataRequest         0x0003
#define SaslHandshakeRequest    0x0011
#define InitProducerIdRequest   0x0016
#define ApiVersionsRequest      0x0012
#define SaslAuthenticateRequest 0x0024
#define DescribeConfigsRequest  0x0020
#define ElectPreferredLeader    0x002B

#define MetadataRequest0  0x00030000   /* with version 0 */


#define BILLION        	1000000000L
#define MILLION			1000000L

#define KAFKA_LOG_MSG_OVERHEAD		34


/*
 * External and forward references
 */
extern void ism_protocol_ensureBuffer(ism_actionbuf_t * buf, int len);
extern void ism_kafka_putVarInt(concat_alloc_t * buf, int32_t ilen) ;
char * ism_mhub_toString(ism_mhub_t * mhub);
static int compareRule(mhub_rule_t * rule1, mhub_rule_t * rule2);
static int mhubStateCheck(ism_timer_t key, ism_time_t now, void * userdata);
struct ssl_ctx_st * ism_transport_clientTlsContext(const char * name, const char * tlsversion, const char * cipher);
static int mhubGetAddress(ism_server_t * server,  ism_transport_t * transport, ism_gotAddress_f gotAddress);
void ism_tcp_init_transport(ism_transport_t * transport);
static int ism_mhub_connected(ism_transport_t * transport, int rc);
static int ism_mhub_closing(ism_transport_t * transport, int rc, int clear, const char * reason);
int  ism_kafka_createConnection(ism_transport_t * transport, ism_server_t * server);
void ism_mhub_mapKafkaVersion(ism_mhub_t * mhub, int version);
static int mhubCreateData(ism_timer_t key, ism_time_t now, void * userdata);
static int mhubRetryConnect(ism_timer_t key, ism_time_t now, void * userdata);
static int mhubReceiveMetadata(ism_transport_t * transport, char * inbuf, int buflen, int kind);
static void freeKafkaEvent(kafka_produce_msg_t * msg);
static void mhubMetadataRequest(ism_mhub_t * mhub, ism_transport_t * transport);
int ism_mhub_message_produce(ism_transport_t * transport, ism_mhub_t * mhub, mhub_part_t * mhub_part,
        kafka_produce_msg_t * msgs, int * producedMsgsCount, int isResend);
static kafka_produce_msg_t *  checkMHubEventBatch(ism_mhub_t * mhub, mhub_part_t * mhub_part, ism_time_t now, int closing) ;
static int mhubPartitionProduceTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) ;
int ism_mqtt_propgen(ism_prop_t * xprops, ism_emsg_t * emsg, const char * name, ism_field_t * f, void * extra, ismMessageSelectionLockStrategy_t * lockStrategy);
static int needMHubBatch(ism_mhub_t * mhub, mhub_part_t * mhub_part, ism_time_t now);
static int  mhubProduceJob(ism_transport_t * transport, void * param1, uint64_t param2);
static int addMhub(ism_mhub_t * mhub);
static int moreConnected(ism_transport_t * transport);
static void freeMTopic(mhub_topic_t * topic);
int ism_transport_frameKafka(ism_transport_t * transport, char * buffer, int pos, int avail, int * used);
int ism_transport_frameNone(ism_transport_t * transport, char * buffer, int pos, int avail, int * used);
static int receiveSASL(ism_transport_t * transport, char * inbuf, int buflen, int kind);
static void sendSASL(ism_transport_t * transport, concat_alloc_t * buf, int kind);
static int createMetadataConnection(ism_mhub_t * mhub);
static void mhubDescribeConfigsRequest(ism_mhub_t * mhub, mhub_part_t * mhub_part, ism_transport_t * transport) ;
const char * ism_transport_makepw(const char * data, char * buf, int len, int dir);
mqtt_prop_ctx_t * ism_mqtt_getPropContext(ism_transport_t * transport);
int ism_mqtt_mpropSet(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value);



/*
 * Static and global variables
 */
static int mhubBatchTimeMillis = 250;
static int mhubBatchSize = 100;   //100 msgs max
static int mhubBatchSizeBytes = 250000;  //Maximum number of bytes per produce
static const char * mhubCiphers = "ECDHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-SHA";
static const char * mhubTLS = "TLSv1.2";
static int  mhubACKs = 1;
static int  mhubVersion = 1;
static struct ssl_ctx_st * mhubTLSCTX;
static int  g_mhubStarted = 0;
static int  g_mhubInited = 0;
static int  g_produce_error_log_interval_count = 1;
static int g_shutdown_onerror_time = 300; //Value in seconds
static ism_time_t g_shutdown_onerror_time_nanos = 0;
int g_mhubEnabled = 0;
extern int g_shuttingDown;
pthread_spinlock_t   g_mhubStatLock;
extern int g_need_dyn_write;

#ifndef HAS_BRIDGE
extern ism_tenant_t eventTenant;
extern ism_tenant_t meterTenant;
static uint32_t		mhub_meter_part_crc; //Use the same scheme to for partition
static void disableEnableInternalHub(ism_tenant_t * tenant, int enabled);
int ism_mhub_setUseMHUBKafkaConnection(int enabled) ;
#endif
int g_useMHUBKafkaConnection=0;

px_kafka_messaging_stat_t mhubMessagingStats = {0};

#define BIGINT16(p) (((int)(uint8_t)(p)[0]<<8) | (uint8_t)(p)[1])

/*
 * Initialize MessageHub services
 */
int ism_mhub_init(void) {
    g_mhubInited = 1;
    g_mhubEnabled = 1;
    mhubBatchTimeMillis = ism_common_getIntConfig("MessageHubBatchTimeMillis", mhubBatchTimeMillis);
    mhubBatchSize = ism_common_getIntConfig("MessageHubBatchSize", mhubBatchSize);
    mhubBatchSizeBytes = ism_common_getIntConfig("MessageHubBatchSizeBytes", mhubBatchSizeBytes);

#ifndef NO_PROXY
    const char * prop;
    prop = ism_common_getStringConfig("MessageHubTLS");   /* Allow TLS disable for testing */
    if (prop) {
        if (!strcmpi(prop, "TLSv1.2"))
            mhubTLS = "TLSv1.2";
        else if (!strcmpi(prop, "TLSv1.1"))
            mhubTLS = "TLSv1.1";
        else if (!strcmpi(prop, "None"))
            mhubTLS = NULL;
    }
    mhubVersion = ism_common_getIntConfig("MessageHubAPIVersion", mhubVersion);
    if (mhubVersion < 0 && mhubVersion > 2)
        mhubVersion = 1;
    TRACE(3, "MessageHubAPIVersion=%u\n", mhubVersion);
    mhubACKs = ism_common_getIntConfig("MessageHubACKs", mhubACKs);
    if (mhubACKs < -1 || mhubACKs > 1)
        mhubACKs = 1;

#endif
    /* Make the TLS context */
    if (mhubTLS)
        mhubTLSCTX = ism_transport_clientTlsContext("MHub", mhubTLS, mhubCiphers);

    pthread_spin_init(&g_mhubStatLock, 0);

    /*The number of errors before print the Error Log. Default is 10*/
    g_produce_error_log_interval_count = ism_common_getIntConfig("MessageHubProduceErrorLogIntervalCount", 1);

    g_shutdown_onerror_time =ism_common_getIntConfig("MessageHubShutdownOnErrorTime", 300); //Value is in Seconds
    g_shutdown_onerror_time_nanos = g_shutdown_onerror_time * BILLION;

#ifndef HAS_BRIDGE
    const char * host = ism_common_getHostnameInfo();
    if (!host)
       host = "imaproxy";
    mhub_meter_part_crc = ism_common_crc(0, (char *)host, strlen(host));

    int mhubKafkaConn = ism_common_getIntConfig("UseMHUBKafkaConnection", 0);
    ism_mhub_setUseMHUBKafkaConnection(mhubKafkaConn);

#endif
    return 0;
}

/*
 * Initialize MessageHub services
 */
int ism_mhub_start(void) {
	TRACE(5, "Event Streams support is started\n");
    g_mhubStarted = 1;
    return 0;
}

/*
 * Terminate MessageHub services
 */
int ism_mhub_term(int fast) {
    return  0;
}


/*
 * Lock the mhub object
 *
 * This is a short term lock when processing an mhub object.  It must be held to guard against changes
 * to the mhub object.  You can lock a partition when holding this lock, but should then free it.  You
 * may not aquire this lock while holding a partition lock.
 */
void ism_mhub_lock(ism_mhub_t * mhub) {
    pthread_spin_lock(&mhub->lock);
}

/*
 * Unlock the mhub object
 */
void ism_mhub_unlock(ism_mhub_t * mhub) {
    pthread_spin_unlock(&mhub->lock);

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
 * Get configured default values for Event Streams 
 */
int ism_mhub_getMaxBatchSizeBytes(void) {
    return mhubBatchSizeBytes;    
}
int ism_mhub_getMaxBatchSizeMsgs(void) {
    return mhubBatchSize;
}
int ism_mhub_getMaxBatchTimeMillis(void) {
    return mhubBatchTimeMillis;
}

/*
 * Make the kafka key for a MessageHub event.
 *
 * This key is a json object with the fields
 * - orgID      - The org name
 * - deviceType - The type field
 * - deviceId   - The id field
 * - eventType  - The evt field
 * - format     - The fmt field
 * - timestamp  - An ISO8601 timestamp in the form 2018-09-15T11:23:15.356-04 .
 *
 * The fields org, type, and id are assumed to not have any restricted characters (and have been checked)
 * The fields event and fmt have not.
 *
 * This is the equivalent of the keymap:
 * {${JSON:org:Org},${JSON:deviceType:Type},${JSON:deviceId:Id},${JSON:eventType:Event},${JSON:format:Format},${JSON:timestamp:TimeISO}}
 */
void ism_mhub_makeKey(ism_mhub_t * mhub, concat_alloc_t * buf, const char * org, const char * type, const char * id,
        const char * event, const char * fmt) {
    char tbuf[64];
    ism_field_t f;
    ism_time_t now = ism_common_currentTimeNanos();


    /*
     * Allow multiple local time zones and do the fast check for time zone offset change
     * assuming that we are always formatting a newer timestamp than before.
     * We must lock the timestamp during set and format.
     */
    if (now >= mhub->valid_until) {
        ism_time_t until;
        int offset = ism_common_checkTimeZone(mhub->timezone, now, &until);
        pthread_spin_lock(&mhub->tslock);
        mhub->valid_until = until;
        ism_common_setTimezoneOffset(mhub->ts, offset);
    } else {
        pthread_spin_lock(&mhub->tslock);
    }
    ism_common_setTimestamp(mhub->ts, now);
    ism_common_formatTimestamp(mhub->ts, tbuf, 64, 7, ISM_TFF_ISO8601_2);
    pthread_spin_unlock(&mhub->tslock);

    buf->compact = 2;
    ism_json_putBytes(buf, "{");
    f.type = VT_String;
    f.val.s = (char *)org;
    ism_json_put(buf, "org", &f, 0);
    if (type) {
        f.val.s = (char *)type;
        ism_json_put(buf, "deviceType", &f, 1);
    }
    if (id) {
        f.val.s = (char *)id;
        ism_json_put(buf, "deviceId", &f, 1);
    }
    if (event) {
        f.val.s = (char *)event;
        ism_json_put(buf, "eventType", &f, 1);
    }
    if (fmt) {
        f.val.s = (char *)fmt;
        ism_json_put(buf, "format", &f, 1);
    }
    f.val.s = tbuf;
    ism_json_put(buf, "timestamp", &f, 1);
    ism_json_putBytes(buf, "}");
}

/*
 * Return a part of a topic
 *
 * We return the pointer and the length but the topic part is not zero termainated
 */
static int topicpart(const char * topic, int topic_len, const char * * part, int which) {
    const char * tp = topic;
    const char * topic_end = tp+topic_len;
    const char * endp;
    int found = 0;
    while (tp < topic_end && found < which) {
        if (*tp == '/')
            found++;
        tp++;
    }
    if (tp < topic_end) {
        *part = tp;
        endp = strchr(tp, '/');
        if (endp)
            return endp-tp;
        else
            return topic_end-tp;
    } else {
        *part = NULL;
        return 0;
    }
}

/*
 * Put a replacement var into a buffer for the mhub maps
 *
 * For normal variables this is put directly into the output buffer
 * For JSON variables this is put into a temp buffer
 */
static int putVar(concat_alloc_t * buf, const char * token, int tokenlen, mqtt_pmsg_t * pmsg, ism_mhub_t * mhub, ism_transport_t * transport) {
    int rc = 0;
    if (tokenlen == 1 && *token == '$') {
        bputchar(buf, '$');
    } else if (tokenlen > 5 && !memcmp(token, "Topic", 5)) {
        /* Substitute for a topic part.  If the part ends in '*' copy the rest of the topic */
        const char * part;
        char * eos;
        int which = strtoul(token+5, &eos, 10);
        int partlen = topicpart(pmsg->topic, pmsg->topic_len, &part, which);
        if (part)
            ism_common_allocBufferCopyLen(buf, part, *eos=='*' ? (pmsg->topic_len - (part-pmsg->topic)) : partlen);
        else
            rc = 1;
    } else if (tokenlen > 4 && !memcmp(token, "Time", 4) &&
            (!strcmp(token+4, "ISO") || !strcmp(token+4, "MS") || !strcmp(token+4, "UUID"))) {
        if (token[4] == 'I') {
            ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_UTC);
            ism_protocol_ensureBuffer(buf, 32);
            ism_common_formatTimestamp(ts, buf->buf + buf->used, 32, 7, ISM_TFF_ISO8601);
            ism_common_closeTimestamp(ts);
            buf->used += strlen(buf->buf+buf->used);
        } else if (token[4] == 'U') {
            char xuuid [16];
            ism_common_newUUID(xuuid, 16, 17, 0);
            if (buf->used + 16 > buf->len)
                ism_protocol_ensureBuffer(buf, 16);
            memcpy(buf->buf+buf->used, xuuid+8, 8);    /* LSV first */
            memcpy(buf->buf+buf->used+8, xuuid, 8);    /* MSV last */
            buf->used += 16;
        } else {
            uint64_t now = ism_common_currentTimeNanos() / 1000000;
            ism_protocol_ensureBuffer(buf, 16);
            sprintf(buf->buf + buf->used, "%lu", now);
            buf->used += strlen(buf->buf+buf->used);
        }
    } else if (tokenlen == 3 && !memcmp(token, "Org", 3)) {
        if (transport && transport->org)
            ism_json_putBytes(buf, transport->org);
        else
            rc = 1;
    } else if (tokenlen == 4 && !memcmp(token, "Type", 4)) {
        if (pmsg->type)
            ism_json_putBytes(buf, pmsg->type);
        else
            rc = 1;
    } else if (tokenlen == 2 && !memcmp(token, "Id", 2)) {
        if (pmsg->id)
            ism_json_putBytes(buf, pmsg->id);
        else
            rc = 1;
    } else if (tokenlen == 5 && !memcmp(token, "Event", 5)) {
        if (pmsg->event)
            ism_json_putBytes(buf, pmsg->event);
        else
            rc = 1;
    } else if (tokenlen == 6 && !memcmp(token, "Format", 6)) {
        if (pmsg && pmsg->format)
            ism_json_putBytes(buf, pmsg->format);
        else
            rc = 1;
    } else if (tokenlen == 3 && !memcmp(token, "QoS", 3)) {
        bputchar(buf, '0' + ((pmsg->cmd >> 1)&3));
    } else if (pmsg->prop_len) {
        ism_field_t f;
        if (!pmsg->msgprops) {
            pmsg->msgprops = ism_common_newProperties(100);
            ism_common_checkMqttPropFields((char *)pmsg->props, pmsg->prop_len, ism_mqtt_getPropContext(transport),
                    0xFFFF, ism_mqtt_mpropSet, pmsg->msgprops);
        }
        ism_common_getProperty(pmsg->msgprops, token, &f);
        if (f.type == VT_String) {
            ism_json_putBytes(buf, f.val.s);
            rc = 1;
        }
    }
    return rc;
}

/*
 * Implement a mhub mapper.
 * This is used for keymap and partitionmap
 */
void ism_mhub_mapper(ism_mhub_t * mhub, concat_alloc_t * buf, const char * map, ism_transport_t * transport, mqtt_pmsg_t * pmsg) {
    const char * tp = map;
    char token [128];
    char xbuf [2048];
    const char * endp;
    int  tokenlen;
    int  nocomma = 0;

    if (!map || !pmsg)
        return;
    pmsg->msgprops = NULL;     /* This field is private to a single mapper call */

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
                        int missing = putVar(&vbuf, itoken, tokenlen, pmsg, mhub, transport);
                        if (!missing || !optional) {
                            if (*iname) {
                                ism_json_putString(buf, iname);
                                ism_json_putBytes(buf, ":");
                            }
                            bputchar(&vbuf, 0);
                            vbuf.used--;
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
                } else if ((tokenlen > 4 && !memcmp(token, "USTR:", 5)) || (tokenlen > 6 && !memcmp(token, "USTR?:", 6))) {
                    int optional = 0;
                    char * iname = token + 5;
                    if (token[4]=='?') {
                        iname++;
                        optional = 1;
                        tokenlen -= 6;
                    } else {
                        tokenlen -= 5;
                    }
                    int saveused = buf->used;
                    buf->used += 2;
                    int missing = putVar(buf, iname, tokenlen, pmsg, mhub, transport);
                    if (missing && optional) {
                        buf->used -= 2;         /* Put in nothing */
                    } else {
                        tokenlen = buf->used - saveused - 2;
                        buf->buf[saveused] = (char)(tokenlen<<8);
                        buf->buf[saveused+1] = (char)tokenlen;
                    }
                } else {
                    putVar(buf, token, tokenlen, pmsg, mhub, transport);
                }
            } else {
                tp += tokenlen;
            }
        }
        if (*tp == '}')
            tp++;
    }
    if (pmsg->msgprops) {
        ism_common_freeProperties(pmsg->msgprops);
        pmsg->msgprops = NULL;
    }
}


/*
 * Map a key using a keymap
 */
void ism_mhub_makeKeyMap(ism_mhub_t * mhub, concat_alloc_t * buf, ism_transport_t * transport, mqtt_pmsg_t * pmsg) {
    ism_mhub_mapper(mhub, buf, mhub->keymap, transport, pmsg);
}

/*
 * Determine the partition to use by hashing the type and id.
 * This is the default if no partition map is specified.  Note however that this
 * only works if the message in fact has type and/or id specified.
 *
 * This is equivalent of using a PartitionMap of "${Type}${Id}"
 *
 * If the partition count is 1, doing this is unnecessary.
 */
uint32_t ism_mhub_getPartition(const char * type, const char * id) {
   uint32_t hash = 0;
   hash = ism_strhash_fnv1a_32(type ? type : "");
   hash = ism_strhash_fnv1a_32_more(id ? id: "", hash);
   return hash;
}

/*
 * Map a key using a keymap
 * This runs a mapper to define partitioning and then hashes the result
 */
uint32_t ism_mhub_getPartitionMap(ism_mhub_t * mhub, concat_alloc_t * buf, ism_transport_t * transport, mqtt_pmsg_t * pmsg) {
    int start = buf->used;
    ism_mhub_mapper(mhub, buf, mhub->partitionMap, transport, pmsg);
    // ism_common_allocBufferCopyLen(buf, "", 1);
    // buf->used--;
    return ism_memhash_fnv1a_32(buf->buf+start, buf->used-start);
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
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_utils,1000),val);
        else
            *oldval = NULL;
        ism_common_free(ism_memory_proxy_utils,freeval);
    } else {
        if (val)
            *oldval = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_utils,1000),val);
        else
            *oldval = NULL;
    }
    return 1;
}


/*
 * Find the MessageHub binding for this tenant and name.
 */
static ism_mhub_t * findMhub(ism_tenant_t * tenant, const char * id) {
    ism_mhub_t * mhub;
    if (!tenant || ! tenant->mhublist)
        return NULL;
    ism_tenant_lock();
    mhub = tenant->mhublist;
    while (mhub) {
        if (!strcmp(mhub->id, id)){
        		ism_tenant_unlock();
            return mhub;
        }
        mhub = mhub->next;
    }
    ism_tenant_unlock();
    return NULL;
}

ism_mhub_t * ism_mhub_findMhub(ism_tenant_t * tenant, const char * id) {
    return findMhub(tenant, id);
}

/*
 * Find the MessageHub binding for this tenant and serviceID
 *
 * TODO: if the same credentials can be used by several MHub objects, we need to
 * change this logic a bit.
 */
static ism_mhub_t * findMhubServiceID(ism_tenant_t * tenant, const char * id) {
    ism_mhub_t * mhub;
    if (!tenant || ! tenant->mhublist)
        return NULL;
    ism_tenant_lock();
    mhub = tenant->mhublist;
    while (mhub) {
        if (!strcmp(mhub->serviceid, id)){
        	 	ism_tenant_unlock();
            return mhub;
        }
        mhub = mhub->next;
    }
    ism_tenant_unlock();
    return NULL;
}


/*
 * Free an MHub object.
 *
 * An MHub object is only freed if the config fails.  Once fully created it is
 * orphaned but not freed.  This is because we might still have references to it.
 */
static void freeMhub(ism_mhub_t * mhub) {
    int i;
    if (mhub) {
    		mhub->state = MHS_Closing;
        if (mhub->rulecount && mhub->rules) {
            for (i=0; i<mhub->rulecount; i++) {
                ism_common_free(ism_memory_proxy_eventstreams,mhub->rules[i]);
            }
        }
        if (mhub->topiccount && mhub->topics) {
            for (i=0; i<mhub->topiccount; i++) {
                freeMTopic(mhub->topics[i]);
            }
            mhub->topiccount=0;
            mhub->topics=NULL;
        }
        if (mhub->topicalloc)
            ism_common_free(ism_memory_proxy_eventstreams,mhub->topics);
        if (mhub->broker_count && mhub->brokers) {
            for (i=0; i<mhub->broker_count; i++) {
                ism_common_free(ism_memory_proxy_eventstreams,(char *)mhub->brokers[i]);
            }
            mhub->broker_count=0;
        }
        if (mhub->rulealloc){
            ism_common_free(ism_memory_proxy_eventstreams,mhub->rules);
            mhub->rules=NULL;
            mhub->rulealloc=0;
        }

        replaceString(&mhub->username, NULL);
        replaceString(&mhub->password, NULL);

        if (mhub->ts)
            ism_common_closeTimestamp(mhub->ts);
        if (mhub->produce_timer)
            ism_common_cancelTimer(mhub->produce_timer);
        ism_common_free(ism_memory_proxy_eventstreams,mhub);
    }
}


/*
 * Make a selection rule from the bridge style definition
 * @param selrule  (output) pointer to selection rule
 * @param name     The name of the rule
 * @param ruledef  The rule definition as SQL92
 * @return A return code 0=good
 *
 */
int ism_mhub_makeSelectionRule(mhub_rule_t * * selrule, const char * name, const char * ruledef) {
    mhub_rule_t * ret;
    int    rc;
    int    rulelen;
    int    namelen;
    int    namealloc;
    ismRule_t * rule;

    namelen = name ? strlen(name) : 0;
    if (namelen > 0 && namelen <= 200) {
        namealloc = ((namelen+4) & ~3) + 1;   /* One byte past a 4 byte boundary.  The selection has a 3 byte header */
        rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, ruledef, SELOPT_Internal);
        if (rc) {
            *selrule = NULL;
        } else {
            int retlen = namealloc + namelen + rulelen + 6;
            ret = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,1),1, sizeof(mhub_rule_t) + retlen);
            uint8_t * up = (uint8_t *)(ret+1);
            /* Rule name */
            ret->rulelen = retlen;
            ret->enabled = 1;
            ret->name_len = namelen;
            ret->name_alloc = namealloc;
            memcpy(up, name, namelen);
            up += namealloc;
            /* Selector */
            *up++ = 0x40;
            *up++ = (uint8_t)(rulelen >> 8);
            *up++ = (uint8_t)rulelen;
            memcpy(up, rule, rulelen);
            up += rulelen;
            /* Topic name */
            *up++ = 0;
            *up++ = (uint8_t)namelen;
            memcpy(up, name, namelen);
            ism_common_freeSelectRule(rule);
            *selrule = ret;
        }
    } else {
        rc = 1;
        *selrule = NULL;
    }
    return rc;
}

/*
 * Make a forwardingRule
 */
static mhub_rule_t * makeForwardingRule(ism_json_parse_t * parseobj, int where, const char * name) {
    mhub_rule_t * ruleobj = NULL;
    uint8_t * rule = NULL;
    const char * topic = NULL;
    const char * eventFilter = NULL;
    const char * typeFilter = NULL;
    int event_len;
    int type_len;
    int topic_len;
    int name_len;
    int name_alloc;
    int enabled;
    int endloc;

    ism_json_entry_t * ent = parseobj->ent+where;
    if (ent->objtype == JSON_Object) {
        endloc = where + ent->count;
        where++;
        enabled = 1;
        while (where <= endloc) {
            ent = parseobj->ent + where;
            if (!strcmp(ent->name, "disabled")) {
                 enabled = 0;
            } else if (!strcmp(ent->name, "topic") && ent->objtype == JSON_String) {
                topic = ent->value;
            } else if ((!strcmp(ent->name, "deviceType"))
                    && ent->objtype == JSON_String) {
                if (strcmp(ent->value, "*"))
                    typeFilter = ent->value;
            } else if ((!strcmp(ent->name, "eventType"))
                    && ent->objtype == JSON_String) {
                if (strcmp(ent->value, "*"))
                    eventFilter = ent->value;
            }
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }
        if (topic != NULL && name != NULL) {
            name_len = strlen(name);
            if (name_len > 100)
                name_len = 100;
            name_alloc = name_len + 1;   /* No need to pad */
            topic_len = strlen(topic);
            if (topic_len > 100)
                topic_len = 100;
            type_len = typeFilter ? strlen(typeFilter) + 3 : 0;
            event_len = eventFilter ? strlen(eventFilter) + 3 : 0;
            if (type_len < 253 && event_len < 253) {
                int rulelen = name_alloc + topic_len + event_len + type_len + 3;
                ruleobj = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,2),1, sizeof(mhub_rule_t) + rulelen);
                rule = (uint8_t *)(ruleobj+1);
                ruleobj->rulelen = rulelen;
                ruleobj->enabled = enabled;
                ruleobj->name_len = name_len;
                ruleobj->name_alloc = name_alloc;

                /* Put out the name */
                int rulepos = 0;
                memcpy((char *)rule+rulepos, name, name_len);
                rulepos += name_len;
                rule[rulepos++] = 0;

                /* Put out the event */
                if (event_len) {
                    rule[rulepos] = strchr(eventFilter, '*') ? 0x11 : 0x10;
                    rule[rulepos+1] = (uint8_t)event_len-3;
                    strcpy((char *)rule+rulepos+2, eventFilter);
                    rulepos += event_len;
                }

                /* Put out the devtype */
                if (type_len) {
                    rule[rulepos] = strchr(typeFilter, '*') ? 0x21 : 0x20;
                    rule[rulepos+1] = (uint8_t)type_len-3;
                    strcpy((char *)rule+rulepos+2, typeFilter);
                    rulepos += type_len;
                }

                /* Put out the topic */
                rule[rulepos++] = 0;
                rule[rulepos++] = (uint8_t)topic_len;
                strcpy((char *)rule+rulepos, topic);
            }
        }
    }
    return ruleobj;
}

/*
 * Make a topic object
 */
static mhub_topic_t * makeTopic(const char * name, int partcount) {
    int i;
    int namelen = name ? strlen(name) : 0;
    if (partcount < 1)
        partcount = 1;

    mhub_topic_t * mtopic = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,3),1, sizeof(mhub_topic_t) + ((partcount-1)*sizeof(mhub_part_t)) + namelen + 1);
    mtopic->name = ((const char *)(mtopic+1)) + sizeof(mhub_part_t)*(partcount-1);
    mtopic->partcount = partcount;
    for (i=0; i<partcount; i++) {
        mtopic->partitions[i].leader = -1;     /* Leader not set */
        pthread_mutex_init(&mtopic->partitions[i].lock, 0);
        mtopic->partitions[i].topic = mtopic;
    }
    strcpy((char *)mtopic->name, name ? name : "");
    return mtopic;
}

/**
 * Free MHub Topic object.
 */
static void freeMTopic(mhub_topic_t * topic){
	if(topic!=NULL){
		for(int pindex = 0; pindex < topic->partcount ; pindex++){
			mhub_part_t * partition = &topic->partitions[pindex];
			//Destroy the partition lock
			pthread_mutex_lock(&partition->lock);
			pthread_mutex_unlock(&partition->lock);
			pthread_mutex_destroy(&partition->lock);
		}
		ism_common_free(ism_memory_proxy_eventstreams,topic);
	}
}

static int partitionMsgsTransfer( mhub_topic_t * topic, kafka_produce_msg_t * old_msgs){
	int which =0;
	int total_msg_transferred =0;

	if(topic!=NULL && old_msgs != NULL){
		while (old_msgs) {
			kafka_produce_msg_t * msg;
			msg = old_msgs;
			old_msgs = old_msgs->next;
			msg->next = NULL;
			which= msg->dcrc % topic->partcount;
			mhub_part_t * new_part = &topic->partitions[which];
			if (new_part->kafka_msg_last)
				new_part->kafka_msg_last->next = msg;
			new_part->kafka_msg_last = msg;
			if (!new_part->kafka_msg_first) {
				new_part->kafka_msg_first = msg;
				new_part->kafka_msg_first_time = msg->time;
				new_part->kafka_msg_count = 1;
			} else {
				new_part->kafka_msg_count++;
			}
			total_msg_transferred++;
		}
	}
	return total_msg_transferred;
}
	

/*
 * Change the partition count for topic
 *
 * The topic object can change as it contains its partition objects.  However, once the index
 * in the MHub object is created the topic is not moved.  Because of this you can look up the
 * topic pointer in the mhub object even if the mhub object is not locked.
 *
 */
static mhub_topic_t * changePartitions(ism_mhub_t * mhub, mhub_topic_t * mtopic, int partcount) {
    int i;
    int count= 0;
    int total_msg_transferred=0;
    int oldPartCount = mtopic->partcount;
    if (partcount < 1)
        partcount = 1;
    if (partcount == mtopic->partcount)
        return mtopic;



	/* close open connections  */
    if (partcount < mtopic->partcount) {
        for (i = partcount; i < mtopic->partcount; i++) {
            mhub_part_t * part = mtopic->partitions+i;
        		pthread_mutex_lock(&part->lock);
            if (part->transport) {
                ism_transport_t * transport = part->transport;
                pthread_mutex_unlock(&part->lock);
                transport->close(transport, ISMRC_EndpointDisabled, 0, "Change in partition count");
                pthread_mutex_lock(&part->lock);
            }
            pthread_mutex_unlock(&part->lock);
        }

    }

    mhub_topic_t * ret = makeTopic(mtopic->name, partcount);

    //Transfer the messages from old partitions over
    for(count=0; count < oldPartCount; count++){
    		mhub_part_t * old_part = &mtopic->partitions[count];
    		pthread_mutex_lock(&old_part->lock);

    		//Process Message in the WaitACK Queue & Regular Msg Queue
    		total_msg_transferred = partitionMsgsTransfer(ret, old_part->kafka_ackwait_msg_first);
		total_msg_transferred+= partitionMsgsTransfer(ret, old_part->kafka_msg_first);

    		pthread_mutex_unlock(&old_part->lock);
    }

    //Put new topic into the mhub object
    for (i=0; i<mhub->topiccount; i++) {
		mhub_topic_t * thistopic = mhub->topics[i];
		if (!strcmp(thistopic->name, ret->name)) {
			mhub->topics[i] = ret;
			break;
		}
	}
    freeMTopic(mtopic);

    pthread_spin_lock(&g_mhubStatLock);
    	mhubMessagingStats.kafkaPartitionChangedCount++;
    mhubMessagingStats.kafkaPartitionMsgsTransferredCount+=total_msg_transferred;
    	pthread_spin_unlock(&g_mhubStatLock);

    return ret;
}

/*
 * Delete a rule by deactivating it
 * TODO: use this
 */
xUNUSED static int deleteRule(ism_mhub_t * mhub, const char * name) {
    int i;
    for (i=0; i<mhub->rulecount; i++) {
        mhub_rule_t * thisrule = mhub->rules[i];
        const char * thisrulename = ((const char *)(thisrule+1));
        if (!strcmp(thisrulename, name)) {
            thisrule->enabled = 5;
            break;
        }
    }
    return 0;
}


/*
 * Update a rule
 */
static int updateRule(ism_mhub_t * mhub, mhub_rule_t * rule) {
    int i;
    const char * rulename = ((const char *)(rule+1))+1;
    for (i=0; i<mhub->rulecount; i++) {
        mhub_rule_t * thisrule = mhub->rules[i];
        const char * thisrulename = ((const char *)(thisrule+1));
        if (!strcmp(thisrulename, rulename)) {
             if (compareRule(thisrule, rule)) {
                 ism_common_free(ism_memory_proxy_eventstreams,rule);
                 return 0;
             } else {
                 mhub->rules[i] = rule;
                 ism_common_free(ism_memory_proxy_eventstreams,thisrule);
                 return 0;
             }
        }
    }
    if (mhub->rulecount+1 >= mhub->rulealloc) {
        int newAlloc = mhub->rulealloc ? 4*mhub->rulealloc : 8;
        mhub_rule_t * * newRules = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,4),mhub->rules, newAlloc * sizeof(mhub_rule_t *));
        if (newRules) {
            for (i = mhub->rulealloc; i < newAlloc; i++) {
                newRules[i] = NULL;
            }
            mhub->rules = newRules;
            mhub->rulealloc = newAlloc;
            mhub->rules[mhub->rulecount++] = rule;
        }
    } else {
         mhub->rules[mhub->rulecount++] = rule;
    }
    return 1;
}

/*
 * Update a topic
 */
static int updateTopic(ism_mhub_t * mhub, mhub_topic_t * topic) {
    int i;
    for (i=0; i<mhub->topiccount; i++) {
        mhub_topic_t * thistopic = mhub->topics[i];
        if (!strcmp(thistopic->name, topic->name)) {
            /* Update the partcount unless we have metadata in which case we believe it */
            if (thistopic->partcount != topic->partcount && !thistopic->valid) {
                changePartitions(mhub, thistopic, topic->partcount);
                ism_common_free(ism_memory_proxy_eventstreams,topic);
                return 1;
            }
            ism_common_free(ism_memory_proxy_eventstreams,topic);
            return 0;
        }
    }
    if (mhub->topiccount+1 >= mhub->topicalloc) {
        int newAlloc = mhub->topicalloc ? 4*mhub->topicalloc : 8;
        mhub_topic_t * * newTopics = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,5),mhub->topics, newAlloc * sizeof(mhub_topic_t *));
        if (newTopics) {
            for (i = mhub->topicalloc; i < newAlloc; i++) {
                newTopics[i] = NULL;
            }
            mhub->topics = newTopics;
            mhub->topicalloc = newAlloc;
            mhub->topics[mhub->topiccount++] = topic;
        }
    } else {
        mhub->topics[mhub->topiccount++] = topic;
    }
    return 1;
}


/*
 * Update the rules to point directly to the topics, and identify unused topics
 *
 * This must be done holding the tenant lock and the lock for this mhub
 */
static int fixRules(ism_mhub_t * mhub) {
    int  i;
    int  j;
    int  badcount = 0;
    const char * topicname;
    mhub_topic_t * topic;
    for (i=0; i<mhub->rulecount; i++) {
        mhub_rule_t * rule = mhub->rules[i];
        char * rp = (char *)(rule+1) + rule->name_alloc;
        while (*rp) {
            switch (*rp) {
            case 0x10:
            case 0x11:
            case 0x20:
            case 0x21:
                rp += ((uint8_t)rp[1]) + 3;
                break;
            case 0x40:
                rp += BIGINT16(rp+1) + 1;
                break;
            default:
                TRACE(1, "Invalid mhub selection rule: mhub=%s rule=%s\n", mhub->id, (char *)(rule+1));
                rule->enabled = 5;
                goto nextrule;
            }
        }
        topicname = rp + 2;
        for (j=0; j<mhub->topiccount; j++) {
            topic = mhub->topics[j];
            if (!strcmp(topic->name, topicname))
                break;
        }
        if (j >= mhub->topiccount) {    /* If not found, disable */
            if (rule->enabled == 1)
                rule->enabled = 3;
            rule->topic_num = -1;
            badcount++;
        } else {
            if (rule->enabled == 0)
                rule->enabled = 1;
            rule->topic_num = (int16_t)j;        /* Put topic index into rule */
        }
        TRACE(8, "fixRule: mhub=%s topic=%s index=%d enabled=%u\n", mhub->id, topicname, rule->topic_num, rule->enabled);
nextrule:
        ;
    }
    return badcount;
}


/*
 * Update a set of selection rules
 *
 * This is called with the mhub locked
 */
int ism_mhub_updateSelRules(ism_mhub_t * mhub, const char * * rulenames, const char * * rules, int count) {
    int i;
    int j;
    int rc;
    mhub_rule_t * selrule;

    if (mhub->rulealloc < count) {
        int newAlloc = mhub->rulealloc ? 4*mhub->rulealloc : 8;
        if (newAlloc < count)
            newAlloc = count;
        mhub_rule_t * * newRules = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,6),mhub->rules, newAlloc * sizeof(mhub_rule_t *));
        if (newRules) {
            for (i = mhub->rulealloc; i < newAlloc; i++) {
                newRules[i] = NULL;
            }
            mhub->rules = newRules;
            mhub->rulealloc = newAlloc;
        } else {
            ism_common_setError(ISMRC_AllocateError);
            return ISMRC_AllocateError;
        }
    }
    mhub->rulecount = 0;

    for (i=0; i<count; i++) {
        rc = ism_mhub_makeSelectionRule(&selrule, rulenames[i], rules[i]);
        if (rc) {
            /* We assume that forwarder already checked the rule, so just trace here */
            TRACE(3, "MessageHub selection rule error: mhub=%s name=%s rule=%s\n",
                    mhub->id, rulenames[i], rules[i]);
        }
        if (compareRule(mhub->rules[i], selrule)) {
            ism_common_free(ism_memory_proxy_eventstreams,selrule);
            selrule = mhub->rules[i];
        } else {
            if (mhub->rules[i])
                ism_common_free(ism_memory_proxy_eventstreams,mhub->rules[i]);
            mhub->rules[i] = selrule;
        }
        if (!rc) {
            const char * topicname = rulenames[i];
            for (j=0; j<mhub->topiccount; j++) {
                mhub_topic_t * thistopic = mhub->topics[j];
                if (!strcmp(thistopic->name, topicname)) {
                    if (selrule)
                        selrule->topic_num = j;
                    break;
                }
            }
            if (j >= mhub->topiccount) {
                int newAlloc = mhub->topicalloc ? 4*mhub->topicalloc : 8;
                mhub_topic_t * * newTopics = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,7),mhub->topics, newAlloc * sizeof(mhub_topic_t *));
                if (newTopics) {
                    if (selrule)
                        selrule->topic_num = j;
                    for (j = mhub->topicalloc; j < newAlloc; j++) {
                        newTopics[j] = NULL;
                    }
                    mhub->topics = newTopics;
                    mhub->topicalloc = newAlloc;
                    /* Make a one partiton topic for now.  We expect metadata before we queue any messages */
                    mhub->topics[mhub->topiccount++] = makeTopic(topicname, 1);
                }
            }
        }
    }
    mhub->rulecount = count;
    return 0;
}
/*
 * Update a set of selection rules for the KafkaConnection mhub object
 *
 * This is called with the mhub locked
 */
int ism_mhub_updateRoutingRules(ism_mhub_t * mhub) {
    int i;
    int j;
    int rc;
    mhub_rule_t * selrule;

    for (i=0; i<mhub->rulealloc; i++) {
        if (!mhub->rulestr[i] || mhub->rules[i])
            continue;
        rc = ism_mhub_makeSelectionRule(&selrule, mhub->rulenames[i], mhub->rulestr[i]);
        if (rc) {
            TRACE(3, "MessageHub selection rule error: mhub=%s name=%s rule=%s\n",
                    mhub->id, mhub->rulenames[i], mhub->rulestr[i]);
        } else {
            mhub->rules[i] = selrule;
            const char * topicname = mhub->rulenames[i];
            for (j=0; j<mhub->topiccount; j++) {
                mhub_topic_t * thistopic = mhub->topics[j];
                if (!strcmp(thistopic->name, topicname)) {
                    if (selrule)
                        selrule->topic_num = j;
                    break;
                }
            }
            if (j >= mhub->topiccount) {
                int newAlloc = mhub->topicalloc ? 4*mhub->topicalloc : 8;
                mhub_topic_t * * newTopics = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,8),mhub->topics, newAlloc * sizeof(mhub_topic_t *));
                if (newTopics) {
                    if (selrule)
                        selrule->topic_num = j;
                    for (j = mhub->topicalloc; j < newAlloc; j++) {
                        newTopics[j] = NULL;
                    }
                    mhub->topics = newTopics;
                    mhub->topicalloc = newAlloc;
                    /* Make a one partiton topic for now.  We expect metadata before we queue any messages */
                    mhub->topics[mhub->topiccount++] = makeTopic(topicname, 1);
                }
            }
        }
    }
    return 0;
}

/*
 * Select a message
 *
 * @param mhub     The MessageHub object
 * @param topicix  (output) An array of topic indexes
 * @param count    The count of topic indexes filled in.  There will be no duplicates.
 * @param type     The type field from the MQTT topic
 * @param event    The event field from the MQTT topic
 * @param pmsg     The pointer to the parsed MQTT message
 * Return The count of topic indexes found (0 = no match).  This will not exceed the input count.
 *
 * This must be called with the MHub object lock held
 */
int ism_mhub_selectMessages(ism_mhub_t * mhub, uint16_t * topicix, int count, const char * type, const char * event, mqtt_pmsg_t * pmsg) {
    int i;
    int found = 0;
    int rulelen;
    if (count == 0)
        return 0;
    for (i=0; i<mhub->rulecount; i++) {
        mhub_rule_t * rule = mhub->rules[i];
        if (rule && rule->enabled == 1) {
            char * rp = (char *)(rule+1) + rule->name_alloc;
            int selected = 0;
            if (*rp == 0)
                selected = 1;
            while (selected >= 0 && *rp) {
                switch (*rp) {
                case 0x10:
                    if (event && !strcmp(event, rp+2))
                        selected = 1;
                    else
                        selected = -1;
                    rp += (rp[1] + 3);
                    break;
                case 0x11:
                    if (event && ism_common_match(event, rp+2))
                        selected = 1;
                    else
                        selected = -1;
                    rp += (rp[1] + 3);
                    break;
                case 0x20:
                    if (type && !strcmp(type, rp+2))
                        selected = 1;
                    else
                        selected = -1;
                    rp += (rp[1] + 3);
                    break;
                case 0x21:
                    if (type && ism_common_match(type, rp+2))
                        selected = 1;
                    else
                        selected = -1;
                    rp += (rp[1] + 3);
                    break;
                case 0x40:                         /* Select by SQL92 selection rule */
                    rulelen = BIGINT16(rp+1);
                    {
                        ism_emsg_t emsg = {0};
                        ismMessageHeader_t hdr = {0};
                        char * topic;
                        if (!mhub->props) {
                            mhub->props = ism_common_newProperties(100);
                        } else {
                            ism_common_clearProperties(mhub->props);
                        }
                        emsg.hdr = &hdr;
                        hdr.Reliability = pmsg->cmd & 0x03;
                        emsg.otherprops = (char *)pmsg->props;
                        emsg.proplen = pmsg->prop_len;
                        emsg.topic = topic = alloca(strlen(pmsg->topic) + 1);
                        memcpy(topic, pmsg->topic, pmsg->topic_len);
                        topic[pmsg->topic_len] = 0;
                        selected = ism_common_filter((ismRule_t *)(rp+3), mhub->props, ism_mqtt_propgen, &emsg, NULL) == SELECT_TRUE;
                    }
                    rp += rulelen + 3;    /* Two byte rule len but no trailing null */
                    break;
                default:
                    selected = -1;
                    break;
                }

            }
            if (selected == 1) {
                int j;
                for (j=0; j<found; j++) {
                    if (topicix[j] == rule->topic_num)
                        break;
                }
                if (j == found) {
                    topicix[found++] = rule->topic_num;
                    if (found == count)
                        return found;
                }
            }
        }
    }
    return found;
}

/*
 * Select a single topic routing.
 *
 * Process the rules and select the first topic which succeeds
 */
int ism_mhub_selectMessage(ism_mhub_t * mhub, const char * type, const char * event, mqtt_pmsg_t * pmsg) {
    uint16_t which;
    int count;

    count = ism_mhub_selectMessages(mhub, &which, 1, type, event, pmsg);
    return count==0 ? -1 : which;
}

/*
 * Create a new MHub which is not linked in
 */
ism_mhub_t * ism_mhub_newMhub(const char * name, ism_tenant_t * tenant, int version) {
    ism_mhub_t * mhub = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,9),1, sizeof(ism_mhub_t));
    strcpy(mhub->structid, "MHub");
    mhub->name = mhub->id;
    mhub->tenant = tenant;
    ism_common_strlcpy(mhub->id, name, sizeof mhub->id);
    pthread_spin_init(&mhub->tslock, 0);
    pthread_spin_init(&mhub->lock, 0);
    mhub->ts = ism_common_openTimestamp(ISM_TZF_UTC);
    mhub->getAddress = mhubGetAddress;
    mhub->serverKind = 5;
    ism_mhub_mapKafkaVersion(mhub, version);
    mhub->verSaslHandshake = 0xff;
    mhub->timezone = ism_common_getTimeZone(NULL);
    addMhub(mhub);
    return mhub;
}

#ifndef HAS_BRIDGE

/**
 * SASL Mechanism Enum List
 */
static ism_enumList enum_sasl_mechanism_list [] = {
    { "Method",    3,                 },
    { "PLAIN",             SASL_MECHANISM_PLAIN,   },
    { "SCRAM-SHA-256",     SASL_MECHANISM_SCRAM_SHA_256,  },
    { "SCRAM-SHA-512",     SASL_MECHANISM_SCRAM_SHA_512, },
};

/*
 * Make a forwarder connection
 */
static int makeKafkaCon(ism_json_parse_t * parseobj, int where, ism_tenant_t * tenant, const char * name, int * created,
        ism_mhub_t * * pmhub, int checkonly, int keepgoing) {
    int endloc;
    ism_mhub_t * mhub;
    int  i;
    int  j;
    int  rc = 0;
    int  namelen;
    char xbuf[1024];
    int  need = 0;
    int  needlog = 1;
    int  maxBytesSet = 0;

    if (!parseobj || where > parseobj->ent_count)
        return 1;
    ism_json_entry_t * ent = parseobj->ent+where;
    if (ent->objtype != JSON_Object)
        return 1;
    endloc = where + ent->count;
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
    mhub = findMhub(tenant, name);
    if (mhub) {
        *created = 0;
        need = mhub->need;
    } else {
        *created = 1;
        mhub = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,10),1, sizeof(ism_mhub_t));
        strcpy(mhub->structid, "MHub");
        mhub->name = mhub->id;
        ism_common_strlcpy(mhub->id, ent->name, sizeof mhub->id);
        pthread_spin_init(&mhub->tslock, 0);
        pthread_spin_init(&mhub->lock, 0);
        mhub->ts = ism_common_openTimestamp(ISM_TZF_UTC);
        mhub->tlsCTX = mhubTLSCTX;   /* TODO: fix this. */
        mhub->getAddress = mhubGetAddress;
        mhub->serverKind = 5;
        mhub->use_keymap = 1;    /* Use the keymap rather than the internal key map */
        mhub->enabled = 1;
        mhub->apiVersion = 0x20;
        ism_mhub_mapKafkaVersion(mhub, mhub->apiVersion);
        mhub->tenant = tenant;
        mhub->mhubACK = mhubACKs;    /* TODO */

        //Default to SASL-PLAIN
        mhub->sasl_mechanism = SASL_MECHANISM_PLAIN;

    }
    where++;
    int savewhere = where;
    while (where <= endloc && rc == 0) {
        ent = parseobj->ent + where;
        if (ent->objtype == JSON_Null)
            ent->count = 0;
        if (!strcmp(ent->name, "BrokerList")) {
            if (ent->objtype != JSON_Array) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            } else {
                int addrcount = ent->count;
                if(addrcount > MAX_MQTT_SERVERS){ /* We support at most 16 MQTT servers */
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                    rc = ISMRC_BadPropertyValue;
                } else {
                    for (i=0; i<addrcount; i++) {
                        if (parseobj->ent[where+i+1].objtype != VT_String) {
                            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                            rc = ISMRC_BadPropertyValue;
                            break;
                        }
                    }
                }
            }
        } else if (!strcmp(ent->name, "Enabled")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Enabled", ism_json_getJsonValue(ent));
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
        } else if (!strcmp(ent->name, "MaxBatchSize")) {
            if ((ent->objtype != JSON_Integer || ent->count < 0) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MaxBatchSize", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "MaxBatchBytes")) {
            if ((ent->objtype != JSON_Integer || ent->count < 0) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MaxBatchBytes", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "MaxBatchTimeMS")) {
            if ((ent->objtype != JSON_Integer || ent->count < 0) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "MaxBatchTimeMS", ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "TimeZone")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
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
                        if (parseobj->ent[where+i+1].objtype != JSON_String) {
                            ismRule_t * rule = NULL;
                            int rulelen;
                            rc = ism_common_compileSelectRuleOpt(&rule, &rulelen, ent->value, SELOPT_Internal);
                            if (rc)
                                break;
                            ism_common_freeSelectRule(rule);
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
        } else if (!strcmp(ent->name, "KafkaAPIVersion")) {
            if ((ent->objtype != JSON_Integer || ent->count < 0 || ent->count > 2) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "ProduceAck")) {
            if ((ent->objtype != JSON_Integer || ent->count < -1 || ent->count > 1) && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "TrustCertificates")) {
            if (ent->objtype != JSON_String && ent->objtype != JSON_Null) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, ism_json_getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmp(ent->name, "SASLMechanism")) {
            if ((ent->objtype != JSON_String || ism_common_enumValue(enum_sasl_mechanism_list, ent->value)==INVALID_ENUM) &&
                   ent->objtype != JSON_Null) {
                  ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "SASLMechanism", ism_json_getJsonValue(ent));
                  rc = ISMRC_BadPropertyValue;
              }
        }else {
            LOG(ERROR, Server, 939, "%s%-s", "Unknown connection property: Connection={0} Property={1}",
                    mhub->name, ent->name);
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
        return rc;
    }

    if (rc == 0) {
        where = savewhere;
        while (where <= endloc) {
            ent = parseobj->ent + where;
            if (!strcmp(ent->name, "BrokerList")) {
                int addrcount = ent->count;
                if (addrcount != mhub->broker_count)
                    need |= 1;
                for (i=0; i<addrcount; i++) {
                    if (replaceString(mhub->brokers+i, parseobj->ent[where+i+1].value))
                        need |= 1;
                }
                for (; i<16; i++) {
                    replaceString(mhub->brokers+i, NULL);
                }
                mhub->broker_count = addrcount;
            } else if (!strcmp(ent->name, "Enabled")) {
                int enabled = ent->objtype != JSON_False;
                if (mhub->enabled != enabled)
                    need |= 1;
                mhub->enabled = enabled;
            } else if (!strcmp(ent->name, "TLS")) {
                int tlsmethod = 0;
                if (ent->objtype != JSON_Null)
                    tlsmethod = ism_common_enumValue(enum_methods, ent->value);
                if (mhub->useTLS != tlsmethod)
                    need |= UPDATE_Secure;
                mhub->useTLS = tlsmethod;
            } else if (!strcmp(ent->name, "Ciphers")) {
                if (ent->value && !*ent->value)
                    ent->value = NULL;
                if (replaceString(&mhub->ciphers, ent->value))
                    need |= UPDATE_Secure;
            } else if (!strcmp(ent->name, "Username")) {
                if (ent->value && !*ent->value)
                    ent->value = NULL;
                need |= replaceString(&mhub->username, ent->value);
            } else if (!strcmp(ent->name, "Password")) {
                if (ent->value && !*ent->value)
                    ent->value = NULL;
                need |= replaceString(&mhub->password, ent->value);
                if (mhub->password &&  *mhub->password != 0)
                		mhub->mhubSASL = 2;
                if (ent->value && *ent->value != '!')
                     g_need_dyn_write = 1;
            } else if (!strcmp(ent->name, "ServerName")) {
                if (ent->value && !*ent->value)
                    ent->value = NULL;
                replaceString(&mhub->serverName, ent->value);
            } else if (!strcmp(ent->name, "MaxBatchBytes")) {
                if (mhub->maxBatchBytes != ent->count)
                    need |= 1;
                mhub->maxBatchBytes = ent->count;
                maxBytesSet=1;
            } else if (!strcmp(ent->name, "MaxBatchSize")) {
                if (mhub->maxBatchMsgs != ent->count)
                    need |= 1;
                mhub->maxBatchMsgs = ent->count;
            } else if (!strcmp(ent->name, "TimeZone")) {
                mhub->timezone = ism_common_getTimeZone(ent->value);
            } else if (!strcmp(ent->name, "MaxBatchTimeMS")) {
                if (mhub->maxBatchTimeMS != ent->count)
                    need |= 1;
                mhub->maxBatchTimeMS = ent->count;
            } else if (!strcmp(ent->name, "RoutingRule")) {
                int rulecount = ent->count;
                if (ent->objtype == JSON_Null) {
                    for (i=0; i<mhub->rulecount; i++) {
                        replaceString(&mhub->rulestr[i], NULL);
                        replaceString((const char * *)&mhub->rules[i], NULL);
                    }
                    mhub->rulecount = 0;
                    need |= 2;
                } else {
                    ent++;
                    for (i=0; i<rulecount; i++) {
                        for (j=0; j<mhub->rulecount; j++) {
                            if (!strcmp(mhub->rulenames[j], ent->name)) {
                                if (replaceString(&mhub->rulestr[j], ent->value)) {
                                    need |= ent->value ? 2 : 1;
                                }
                                replaceString((const char * *)&mhub->rules[j], NULL);
                                break;
                            }
                        }
                        if (j == mhub->rulecount) {
                            if (mhub->rulecount+1 >= mhub->rulealloc) {
                                int newcnt = mhub->rulecount + 8;
                                if (newcnt < 8)
                                    newcnt = 8;
                                mhub->rules = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,11),mhub->rules, newcnt*sizeof(char *));
                                mhub->rulestr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,12),mhub->rulestr, newcnt*sizeof(char *));
                                mhub->rulenames = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,13),mhub->rulenames, newcnt*sizeof(char *));
                                memset((char *)(mhub->rules)+mhub->rulealloc*sizeof(char *), 0, (newcnt-mhub->rulealloc) * sizeof(char *));
                                memset((char *)(mhub->rulestr)+mhub->rulealloc*sizeof(char *), 0, (newcnt-mhub->rulealloc) * sizeof(char *));
                                memset((char *)(mhub->rulenames)+mhub->rulealloc*sizeof(char *), 0, (newcnt-mhub->rulealloc) * sizeof(char *));
                                need |= 1;
                                mhub->rulealloc = newcnt;
                            }
                            replaceString(&mhub->rulenames[j], ent->name);
                            replaceString(&mhub->rulestr[j], ent->value);
                            replaceString((const char * *)&mhub->rules[j], NULL);
                            mhub->rulecount++;
                        }
                        ent++;
                    }
                    ism_mhub_updateRoutingRules(mhub);
                    ent = parseobj->ent + where;
                }
            } else if (!strcmp(ent->name, "KeyMap")) {
                if (ent->value && !*ent->value)
                    ent->value = NULL;
                need |= replaceString(&mhub->keymap, ent->value);
            } else if (!strcmp(ent->name, "PartitionRule")) {
                replaceString(&mhub->partitionMap, ent->value);
            } else if (!strcmp(ent->name, "TrustCertificates")) {
                if (replaceString(&mhub->trustCerts, ent->value))
                    need |= UPDATE_Secure;
            } else if (!strcmp(ent->name, "KafkaAPIVersion")) {
                if (ent->objtype == JSON_Null)
                    ent->count = 0x20;
                if (mhub->apiVersion != ent->count)
                    need |= 1;
                mhub->apiVersion = ent->count;
                ism_mhub_mapKafkaVersion(mhub, ent->count);
            }  else if (!strcmp(ent->name, "ProduceAck")) {
                if (ent->objtype == JSON_Null)
                    ent->count = mhubACKs;
                if (mhub->mhubACK != ent->count)
                    need |= 1;
                mhub->mhubACK = ent->count;
            } else if (!strcmp(ent->name, "SASLMechanism")) {
                int sasl_mechanism = mhub->sasl_mechanism;
                if (ent->objtype != JSON_Null)
                    sasl_mechanism = ism_common_enumValue(enum_sasl_mechanism_list, ent->value);
                //if (mhub->sasl_mechanism != sasl_mechanism)
                //    need |= UPDATE_Secure;
                mhub->sasl_mechanism = sasl_mechanism;
            }
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }
        mhub->need = need;
        //If max byte is not set. Use default
        if(!maxBytesSet){
        		mhub->maxBatchBytes=mhubBatchSizeBytes;
        }
        //Set default maximum limits
        if(!mhub->maxBatchMsgs)
             mhub->maxBatchMsgs=mhubBatchSize;
        if(!mhub->maxBatchTimeMS)
        		mhub->maxBatchTimeMS = mhubBatchTimeMillis;
        mhub->isKafkaConn = 1;
        *pmhub = mhub;
    } else {
        if (needlog) {
            ism_common_formatLastError(xbuf, sizeof xbuf);
            LOG(ERROR, Server, 953, "%s%u%-s", "Connection configuration error: Connection={0} Error={2} Code={1}",
                            mhub->name, ism_common_getLastError(), xbuf);
        }
    }

    return rc;
}

/*
 * Write out a kafka connection as JSON
 *
 * The invoker should set the buf->compact as required
 * This is used by the REST GET support
 */
void ism_mhub_getKafkaConnectionJson(ism_json_t * jobj, ism_mhub_t * mhub, const char * name) {
    int i;

    ism_json_startObject(jobj, name);
    ism_json_startArray(jobj, "BrokerList");
    for (i=0; i<mhub->broker_count; i++) {
        ism_json_putStringItem(jobj, NULL, mhub->brokers[i]);
    }
    ism_json_endArray(jobj);
    if (mhub->enabled > 1)
        ism_json_putIntegerItem(jobj, "Enabled", mhub->enabled);
    else
        ism_json_putBooleanItem(jobj, "Enabled", mhub->enabled);
    if (mhub->useTLS)
        ism_json_putStringItem(jobj, "TLS", ism_common_enumName(enum_methods, mhub->useTLS));
    if (mhub->ciphers)
        ism_json_putStringItem(jobj, "Ciphers", mhub->ciphers);
    if (mhub->username)
        ism_json_putStringItem(jobj, "Username", mhub->username);

    if (mhub->password)  {
        if (*mhub->password != '!') {
            int obfus_len = strlen(mhub->password) * 2 + 40;
            char * obfus = alloca(obfus_len);
            ism_transport_makepw(mhub->password, obfus, obfus_len-1, 0);
            ism_json_putStringItem(jobj, "Password", obfus);
        } else {
            ism_json_putStringItem(jobj, "Password", mhub->password);
        }
    }
    if (mhub->serverName)
        ism_json_putStringItem(jobj, "ServerName", mhub->serverName);
    if (mhub->maxBatchBytes > 0)
        ism_json_putIntegerItem(jobj, "MaxBatchSize", mhub->maxBatchBytes);
     if (mhub->maxBatchTimeMS > 0)
        ism_json_putIntegerItem(jobj, "MaxBatchTimeMS", mhub->maxBatchTimeMS);
    const char * tzname = ism_common_getTimeZoneName(mhub->timezone);
    if (tzname)
        ism_json_putStringItem(jobj, "TimeZone", tzname);
    if (mhub->trustCerts)
        ism_json_putStringItem(jobj, "TrustCertificates",mhub->trustCerts);
    if (mhub->rulecount) {
        ism_json_startObject(jobj, "RoutingRule");
        for (i=0; i<mhub->rulecount; i++) {
            if (mhub->rulestr[i]) {
                ism_json_putStringItem(jobj, mhub->rulenames[i], mhub->rulestr[i]);
            }
        }
        ism_json_endObject(jobj);
    }
    if (mhub->keymap)
        ism_json_putStringItem(jobj, "KeyMap", mhub->keymap);
    if (mhub->partitionMap)
        ism_json_putStringItem(jobj, "PartitionRule",mhub->partitionMap);
    if (mhub->apiVersion >= 0 && mhub->apiVersion <= 2) {
        ism_json_putIntegerItem(jobj, "KafkaAPIVersion", mhub->apiVersion);
    }
    ism_json_endObject(jobj);
}

/*
 * Put out a tenant list in JSON form
 */
int ism_mhub_getKafkaConList(const char * match, ism_tenant_t * tenant, ism_json_t * jobj, const char * name) {
    ism_mhub_t * mhub;

    ism_json_startObject(jobj, name);
    /* Lock? */
    mhub = tenant->mhublist;
    while (mhub) {
        if (ism_common_match(mhub->name, match)) {
            ism_mhub_getKafkaConnectionJson(jobj, mhub, mhub->name);
        }
        mhub = mhub->next;
    }
    ism_json_endObject(jobj);
    return 0;
}
#endif

/*
 * Parse and create an mhub entry
 *
 * This must be called with the tenant lock held
 */
static ism_mhub_t * makeMhub(ism_json_parse_t * parseobj, int where, ism_tenant_t * tenant, const char * name, int * created) {
    mhub_rule_t * rules [256];
    mhub_topic_t * topics [256];
    int i;
    int isnew = 0;
    int rc = 0;
    int rulecount = 0;
    int topiccount = 0;
    int maxrulecount = 254;
    int maxtopiccount = 254;
    int endloc;
    int need;
    ism_mhub_t * mhub = NULL;
    int badrules = 0;
    int newEnabled = -1;
    ism_timezone_t * newTimezone = NULL;
    const char * newServiceID;

    ism_json_entry_t * ent = parseobj->ent+where;
    if (ent->objtype == JSON_Object) {
        if (!name)
            name = ent->name;
        if (!name) {
            TRACE(3, "MessageHub binding as no name: tenant=%s\n", tenant->name);
            return NULL;
        }
        mhub = findMhub(tenant, name);
        if (!mhub) {
             mhub = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,14),1, sizeof(ism_mhub_t));
             strcpy(mhub->structid, "MHub");
             mhub->name = mhub->id;
             ism_common_strlcpy(mhub->id, ent->name, sizeof mhub->id);
             pthread_spin_init(&mhub->tslock, 0);
             pthread_spin_init(&mhub->lock, 0);
             mhub->ts = ism_common_openTimestamp(ISM_TZF_UTC);
             mhub->tlsCTX = mhubTLSCTX;   /* Use a common context for all connections */
             if (mhubTLS)
                 mhub->useTLS = 1;
             mhub->getAddress = mhubGetAddress;
             mhub->serverKind = 5;
             mhub->enabled = 1;
             mhub->mhubACK = mhubACKs;
             ism_mhub_mapKafkaVersion(mhub, mhubVersion);
             *created = isnew = 1;
        }
        need = mhub->need;
        endloc = where + ent->count;
        where++;
        while (where <= endloc) {
            ent = parseobj->ent + where;

            if (!strcmp(ent->name, "disabled")) {
                if (ent->objtype != JSON_True && ent->objtype != JSON_False &&
                    (ent->objtype != JSON_Integer || ent->count != 2)) {
                    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "disabled", ism_json_getJsonValue(ent));
                    rc = ISMRC_BadPropertyValue;
                }
                newEnabled = (ent->objtype == JSON_Integer) ? ent->count : ent->objtype == JSON_False;
                if (mhub->enabled != newEnabled)
                    need |= UPDATE_Status;
            } else  if (!strcmp(ent->name, "timezone")) {
                if (ent->objtype == JSON_String) {
                    newTimezone = ism_common_getTimeZone(ent->value);
                }
            } else if (!strcmp(ent->name, "keyMap")) {
                if (ent->objtype == JSON_String || ent->objtype == JSON_Null) {
                    need |= replaceString(&mhub->keymap, ent->value);
                    mhub->use_keymap = ent->value != NULL;
                }
            } else if (!strcmp(ent->name, "partitionRule")) {
                if (ent->objtype == JSON_String || ent->objtype == JSON_Null) {
                    replaceString(&mhub->partitionMap, ent->value);
                }
            } else if (!strcmp(ent->name, "serviceID"))  {
                if (ent->objtype == JSON_String) {
                    int sidlen = strlen(ent->value);
                    if (sidlen < 1 || sidlen+1 >= sizeof(mhub->serviceid)) {
                        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "serviceID", ism_json_getJsonValue(ent));
                        rc = ISMRC_BadPropertyValue;
                    } else {
                        newServiceID = ent->value;
                        if (strcmp(newServiceID, ent->value))
                            need |= UPDATE_Auth;
                    }
                }
            } else if (!strcmp(ent->name, "rules") && ent->objtype == JSON_Object) {
                int arrayloc = where+1;
                int arrayend = where + ent->count;
                while (arrayloc <= arrayend) {
                    ism_json_entry_t * ruleent = parseobj->ent+arrayloc;
                    if (ruleent->objtype == JSON_Object) {
                        rules[rulecount] = makeForwardingRule(parseobj, arrayloc, ruleent->name);
                        if (rules[rulecount] == NULL) {
                            LOG(WARN, Server, 965, "%s%-s%-s", "MessageHub forwarding rule not valid: Org={0} ID={1} Rule={2}",
                                tenant->name, mhub->id, ruleent->name);
                        } else if (rulecount <= maxrulecount) {
                            rulecount++;
                        } else {
                            badrules++;
                        }
                    }
                    if (ruleent->objtype == JSON_Object || ruleent->objtype == JSON_Array)
                        arrayloc += ruleent->count + 1;
                    else
                        arrayloc++;
                }
            } else if (!strcmp(ent->name, "topics") && ent->objtype == JSON_Array) {
                int arrayloc = where+1;
                int arrayend = where + ent->count;
                char * topicname = NULL;
                while (arrayloc <= arrayend) {
                    ism_json_entry_t * topicent = parseobj->ent+arrayloc;
                    if (topicent->objtype == VT_String) {
                        topicname = (char *)topicent->value;
                        char * colonpos = strchr(topicname, ':');
                        int partcount = 1;
                        if (colonpos) {
                            *colonpos = 0;
                            partcount = atoi(colonpos+1);
                        }
                        if (topicname && partcount && topiccount <= maxtopiccount) {
                            topics[topiccount] = makeTopic(topicname, topicent->count);
                            topiccount ++;
                        }
                        if (colonpos)
                            *colonpos = ':';
                    }
                    arrayloc++;
                }
            } else {
                rc = ISMRC_BadPropertyName;
                ism_common_setErrorData(rc, "%s", ent->name);
            }


            if (ent->objtype == JSON_Array || ent->objtype == JSON_Object)
                where += ent->count + 1;
            else
                where++;
        }
        if (rc) {
            LOG(ERROR, Server, 964, "%s%-s", "MessageHub binding not valid: Org={0} ID={1}", tenant->name, mhub->id);
            if (isnew) {
                TRACE(3, "MessageHub object not created due to config errors: org=%s mhub=%s\n", tenant->name, mhub->id);
                freeMhub(mhub);
            } else {
                TRACE(3, "MessageHub object not updated due to config errors: org=%s mhub=%s\n", tenant->name, mhub->id);
            }
            return NULL;
        }

        /*
         * Fill in object as we have validated it
         */
        ism_mhub_lock(mhub);
        if (newEnabled >= 0)
            mhub->enabled = newEnabled;
        if (newServiceID)
            strcpy(mhub->serviceid, newServiceID);
        if (newTimezone)
            mhub->timezone = newTimezone;
        if (!mhub->timezone)
            mhub->timezone = ism_common_getTimeZone(NULL);
        mhub->tenant = tenant;
        for (i=0; i<rulecount; i++) {
            if (updateRule(mhub, rules[i]))
                need |= UPDATE_Rules;
        }
        for (i=0; i<topiccount; i++) {
            if (updateTopic(mhub, topics[i]))
                need |= UPDATE_Topics;
        }

        /* Update rules to index directly to topics, and identify unused topics */
        badrules = fixRules(mhub);
        if (badrules > 0) {
            LOG(WARN, Server, 966, "%s%-s", "Could not match up MessageHub forwarding rules and topics: Org={0} ID={1}",
                                tenant->name, mhub->id);
        }
        if (!*mhub->serviceid) {
            LOG(WARN, Server, 967, "%s%-s", "MessageHub binding is missing a serviceID: Org={0} ID={1}",
                                tenant->name, mhub->id);
        }
        mhub->need = need;

        //Set default maximum limits
        mhub->maxBatchMsgs=mhubBatchSize;
        mhub->maxBatchBytes=mhubBatchSizeBytes;
        mhub->maxBatchTimeMS = mhubBatchTimeMillis;

        ism_mhub_unlock(mhub);
    }
    return mhub;
}


/*
 * Add an mhub to the existing mhub entry
 * The new mhub object has been unlinked from any list
 */
static int addMhub(ism_mhub_t * mhub) {
    ism_tenant_t * tenant = mhub->tenant;
    ism_mhub_t * mhub_prev;

    mhub->next = NULL;

    /*
     * Add the new mhub object at the end of the existing list
     */
    ism_tenant_lock();
    if (tenant->mhublist == NULL) {
        tenant->mhublist = mhub;
    } else {
        mhub_prev = tenant->mhublist;
        while (mhub_prev->next) {
            mhub_prev = mhub_prev->next;
        }
        mhub_prev->next = mhub;
    }
    TRACE(6, "Add MessageHub binding: org=%s id=%s serviceID=%s\n", tenant->name, mhub->id, mhub->serviceid);
    if (g_mhubInited) {
        if (!tenant->mhubtimer) {
            TRACE(6, "Add MessageHub timer for org %s\n", tenant->name);
            tenant->mhubtimer = ism_common_setTimerRate(ISM_TIMER_LOW, mhubStateCheck, tenant, 1, 2, TS_SECONDS);
        }

    }
    ism_tenant_unlock();
#ifndef HAS_BRIDGE
    //If KafkaConnection Enabled or Disabled, make sure Event and Metering reflect that.
    if(g_mhubInited && g_useMHUBKafkaConnection>0){
        if(tenant == &eventTenant ){
            if( g_useMHUBKafkaConnection>0){
                disableEnableInternalHub(tenant, 1);
            }
            else{
                disableEnableInternalHub(tenant, 0);
            }
        }else if(tenant == &meterTenant ){
            if( g_useMHUBKafkaConnection>1){
                disableEnableInternalHub(tenant, 1);
            }
            else{
                disableEnableInternalHub(tenant, 0);
            }
        }

    }
#endif
    return 0;
}

/**
 * Close an MHub Connection
 */
static int  closeConnectionJob(ism_transport_t * transport, void * param1, uint64_t param2) {
	transport->close(transport, ISMRC_EndpointDisabled, 0, "The connection was closed by an administrator");
	return 0;
}

/*
 * Close any connections in an mhub
 * Discard msgs in the partitions
 * Mhub lock will be taken in this function.
 * So need to unlock mhub before calling this function
 */
static int closeConnections(ism_mhub_t * mhub) {
	ism_transport_t * transport=NULL;
	if (mhub!=NULL) {
		ism_mhub_lock(mhub);
		//close all data connections
		/* For Each Topic, Go thru partitions for publishing */
		for (int tcount = 0; tcount < mhub->topiccount; tcount++) {

			mhub_topic_t	* mhub_topic = mhub->topics[tcount];
			for (int pcount = 0; pcount < mhub_topic->partcount; pcount++) {
				mhub_part_t * mhub_part = (mhub_part_t * )&mhub_topic->partitions[pcount];

				pthread_mutex_lock(&mhub_part->lock);

				//Discard msgs
				if(mhub_part->kafka_ackwait_msg_first){
					kafka_produce_msg_t * msgs = mhub_part->kafka_ackwait_msg_first;
					mhub_part->kafka_ackwait_msg_first=NULL;
					mhub_part->kafka_ackwait_msg_last=NULL;
					mhub_part->hasackwait=0;
					int msgcnt=0;
					while (msgs) {
						kafka_produce_msg_t * msg;
						msg = msgs;
						msgs = msgs->next;
						msgcnt++;
						freeKafkaEvent(msg);

					}
					//mhub_part->kafka_msg_count -= msgcnt;
					pthread_spin_lock(&g_mhubStatLock);
					mhubMessagingStats.kakfaTotalPendingMsgsCount -= msgcnt;
					pthread_spin_unlock(&g_mhubStatLock);
				}
				if(mhub_part->kafka_msg_first){
					kafka_produce_msg_t * msgs = mhub_part->kafka_msg_first;
					mhub_part->kafka_msg_first=NULL;
					mhub_part->kafka_msg_last=NULL;
					int msgcnt=0;
					while (msgs) {
						kafka_produce_msg_t * msg;
						msg = msgs;
						msgs = msgs->next;
						msgcnt++;
						freeKafkaEvent(msg);

					}
					//mhub_part->kafka_msg_count -= msgcnt;
					pthread_spin_lock(&g_mhubStatLock);
					mhubMessagingStats.kakfaTotalPendingMsgsCount -= msgcnt;
					pthread_spin_unlock(&g_mhubStatLock);
				}

				transport = mhub_part->transport;
				if (transport) {
					 ism_transport_submitAsyncJobRequest(transport, closeConnectionJob, 0, 0);
				}
				pthread_mutex_unlock(&mhub_part->lock);
			}
		}

		//Close Metadata Connection
		transport = mhub->metadata;
		if (transport) {
			ism_transport_submitAsyncJobRequest(transport, closeConnectionJob, 0, 0);
		}
		ism_mhub_unlock(mhub);
	}

    return 0;
}

/*
 * Delete an mhub object
 */
static int deleteMhub(ism_mhub_t * mhub) {
    ism_tenant_t * tenant = mhub->tenant;
    ism_mhub_t * mhub_prev;

    /*
     * Unlink the Mhub object from the current tenant
     */
    if (mhub == tenant->mhublist) {
        tenant->mhublist = mhub->next;
    } else {
        mhub_prev = tenant->mhublist;
        while (mhub_prev->next) {
            if (mhub_prev->next == mhub) {
                mhub_prev->next = mhub->next;
                break;
            }
            mhub_prev = mhub_prev->next;
        }
        if (mhub_prev->next  == NULL) {
            TRACE(3, "Delete Mhub object not found: org=%s id=%s\n", tenant->name, mhub->id);
            return -1;
        }
    }

    TRACE(3, "Delete Mhub object: org=%s name=%s\n", tenant->name, mhub->id);

    /*
     * Close any open data and metadata connections.
     * Once we issue the close we do not need addressability to them from a Mhub object
     */
    closeConnections(mhub);

    return 0;
}



/*
 * A MesasgeHub binding is a JSON array which must be completely replaced on each call.
 * Each object in the array is a service binding. Not all of the bindings are for MessageHub.
 * This code igonored all non-MessageHub service bindings.
 *
 * The code parses the service bindings, and then checks for any changes from the existing
 * MessageHub bindings for this tenant.
 */
int ism_mhub_parseBindings(ism_json_parse_t * parseobj, int where, const char * name) {
    ism_mhub_t * mhub;
    int  endloop;
    ism_tenant_t * tenant = NULL;
    const char * mhubname = NULL;

    if (!parseobj || where > parseobj->ent_count)
        return 1;


    ism_json_entry_t * ent = parseobj->ent+where;
    if (!name)
        name = ent->name;
    if (!name) {
        TRACE(2, "MessageHub bindings with no name\n");
        return 1;
    }
    mhubname = strchr(name, '/');
    if (mhubname) {
        int len = mhubname-name;
        char * temp = alloca(len+1);
        memcpy(temp, name, len);
        temp[len] = 0;
        name = temp;
        mhubname++;
    }

    ism_tenant_lock();
    tenant = ism_tenant_getTenant(name);
    ism_tenant_unlock();
    if (!tenant) {
        TRACE(2, "MessageHub bindings unknown tenant: %s\n", name);
        return 1;
    }
    if (ent->objtype != JSON_Object) {
        TRACE(2, "MessageHub binding not an object: %s\n", name);
        return 1;
    }
    endloop = where + ent->count;
    if (!mhubname)
        where++;
    while (where <= endloop) {
        if (ent->objtype == JSON_Object) {
            int created = 0;
            ent = parseobj->ent + where;
            mhub = makeMhub(parseobj, where, tenant, mhubname, &created);
            if (mhub) {   /* If good link it in */
                if (created) {
                    addMhub(mhub);
                }
            }
        } else {
            if (ent->objtype == JSON_Null) {
                mhub = findMhub(tenant, ent->name);
                if (mhub) {
                    deleteMhub(mhub);
                }
            } else {
                TRACE(2, "MessageHub binding is not an object: tenant=%s\n", tenant->name);
                return 1;
            }
        }
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }
    return 0;
}

#ifndef HAS_BRIDGE
/*
 * KakfaConnection is a JSON object
 */
int ism_mhub_parseKafkaConnection(ism_json_parse_t * parseobj, int where, const char * name, int checkonly, int keepgoing) {
    int  rc = 0;
    ism_mhub_t * mhub;
    int endloop;
    ism_tenant_t * tenant = NULL;
    const char * mhubname;
    int namelen;
    int created = 0;

    if (!parseobj || where > parseobj->ent_count)
        return 1;

    ism_json_entry_t * ent = parseobj->ent + where;
    if (!name)
        name = ent->name;

    mhubname = strchr(name, '/');
    if (mhubname) {
        int len = mhubname-name;
        char * temp = alloca(len+1);
        memcpy(temp, name, len);
        temp[len] = 0;
        name = temp;
        mhubname++;
    }

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

    ism_tenant_lock();
    tenant = ism_tenant_getTenant(name);
    ism_tenant_unlock();
    if (!tenant) {
        TRACE(2, "KafkaConnection unknown tenant: %s\n", name);
        /* TODO: log */
        return 1;
    }

    endloop = where + ent->count;
    if (!mhubname)
        where++;
    while (where <= endloop) {
        if (ent->objtype == JSON_Object) {
            ent = parseobj->ent + where;
            int xrc = makeKafkaCon(parseobj, where, tenant, mhubname, &created, &mhub, checkonly, keepgoing);
            if (!rc)
                rc = xrc;
            if (xrc == 0 && !checkonly) {   /* If good link it in */
                if (created) {
                    addMhub(mhub);
                }
            }
        } else if (ent->objtype == JSON_Null && !checkonly) {
            mhub = findMhub(tenant, ent->name);
            if (mhub) {
                deleteMhub(mhub);
            } else {
                TRACE(2, "KafkaConnection is not an object: tenant=%s\n", tenant->name);
                /* TODO: LOG */
            }
        }
        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }
    return rc;
}
#endif

/*
 * A MessageHub credentials is a JSON object.  This is configured separately from the binding
 * which uses it but the binding must be configured first.  If the binding is removed the
 * credentials are removed as well.
 *
 * The code parses the credentials and compares them to the existing credentials.
 */
int ism_mhub_parseCredentials(ism_json_parse_t * parseobj, int where, const char * name) {
    int  rc = 0;
    int  i;
    ism_mhub_t * mhub;
    const char * brokers[32];
    int brokercount = 0;
    int maxbrokercount = 31;
    int endloc;
    int endloop;
    int need;
    ism_tenant_t * tenant = NULL;
    const char * mhubname;

    if (!parseobj || where > parseobj->ent_count)
        return 1;

    ism_json_entry_t * ent = parseobj->ent + where;
    if (!name)
        name = ent->name;
    if (!name) {
        TRACE(2, "MessageHub credentials with no name\n");
        return 1;
    }
    mhubname = strchr(name, '/');
    if (mhubname) {
        int len = mhubname-name;
        char * temp = alloca(len+1);
        memcpy(temp, name, len);
        temp[len] = 0;
        name = temp;
        mhubname++;
    }

    ism_tenant_lock();
    tenant = ism_tenant_getTenant(name);
    ism_tenant_unlock();
    if (!tenant) {
        TRACE(2, "MesasgeHub credentials unknown tenant: %s\n", name);
        return 1;
    }
    if (ent->objtype != JSON_Object) {
        TRACE(2, "MessageHub binding not an object: %s\n", name);
        return 1;
    }
    endloop = where + ent->count;
    if (!mhubname)
        where++;
    while (where <= endloop) {
        ent = parseobj->ent+where;
        mhub = findMhubServiceID(tenant, mhubname ? mhubname : ent->name);
        if (!mhub) {
            TRACE(2, "MessageHub credentials for unknown binding: %s\n", ent->name? ent->name : "");
            return 1;
        }

        need = mhub->need;
        endloc = where + ent->count;
        where++;
        while (where <= endloc) {
            ent = parseobj->ent + where;

            if (!strcmp(ent->name, "brokers") && ent->objtype == JSON_Array) {
                int arrayloc = where+1;
                int arrayend = where + ent->count;
                while (arrayloc <= arrayend) {
                    ism_json_entry_t * brokerent = parseobj->ent+arrayloc;
                    if (brokerent->objtype == JSON_String) {
                        if (brokercount < maxbrokercount)
                            brokers[brokercount++] = brokerent->value;
                    }
                    if (brokerent->objtype == JSON_Object || brokerent->objtype == JSON_Array)
                       arrayloc += brokerent->count + 1;
                    else
                       arrayloc++;
                }
            } else if (!strcmp(ent->name, "user") && ent->objtype == JSON_String) {
                if (replaceString(&mhub->username, ent->value))
                    need |= UPDATE_Auth;
            } else if (!strcmp(ent->name, "password") && ent->objtype == JSON_String) {
                if (replaceString(&mhub->password, ent->value))
                    need |= UPDATE_Auth;
                if (mhub->password)
                    mhub->mhubSASL = 2;
            }
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }
        if (mhub->broker_count != brokercount)
            need |= UPDATE_Brokers;
        mhub->broker_count = brokercount;
        for (i=0; i<brokercount; i++) {
            if (replaceString(&mhub->brokers[i], brokers[i]))
                need |= UPDATE_Brokers;
        }
        mhub->need = need;
    }
    return rc;
}

/*
 * Compare two rules for equality
 */
static int compareRule(mhub_rule_t * rule1, mhub_rule_t * rule2) {
    if (!rule1  && !rule2)
        return 0;
    if (rule1 && rule2 && rule1->rulelen == rule2->rulelen &&
            !memcmp((char *)(rule1+1), (char *)(rule2+1), rule1->rulelen)) {
        return 1;
    }
    return 0;
}


/*
 * Put out a rule as a string
 */
static void ruleToString(concat_alloc_t * buf, mhub_rule_t * ruleobj) {
    int rulelen;
    int done = 0;
    uint8_t * rule = (uint8_t *)(ruleobj+1) + ruleobj->name_alloc;
    ism_json_putBytes(buf, "Rule name=");
    ism_json_putBytes(buf, (char *)(ruleobj+1));
    while (!done) {
        switch (*rule) {
        case 0x00:
            ism_json_putBytes(buf, "topic=");
            ism_common_allocBufferCopyLen(buf, (char *)rule+2, rule[1]);
            done = 1;
            break;
        case 0x10:
        case 0x11:
            ism_json_putBytes(buf, "event=");
            ism_common_allocBufferCopyLen(buf, (char *)rule+2, rule[1]);
            ism_json_putBytes(buf, " ");
            rule += rule[1] + 3;
            break;
        case 0x20:
        case 0x21:
            ism_json_putBytes(buf, "type=");
            ism_common_allocBufferCopyLen(buf, (char *)rule+2, rule[1]);
            ism_json_putBytes(buf, " ");
            rule += rule[1] + 3;
            break;
        case 0x40:
            rulelen = BIGINT16(rule+1);
            ism_json_putBytes(buf, " selector ");
            rule += rulelen + 3;        /* 2 byte rulelen */
            break;
        default:
            ism_json_putBytes(buf, "badRule");
            done = 1;
        }
    }
    ism_common_allocBufferCopyLen(buf, "", 1);
    buf->used--;
}

/*
 * Return the mhub object as a string.
 *
 * The returned values must be freed by the caller.
 */
char * ism_mhub_toString(ism_mhub_t * mhub) {
    char xbuf [4096];
    char pbuf [2048];
    int  i;
    concat_alloc_t zbuf = {xbuf, sizeof xbuf};
    concat_alloc_t * buf = &zbuf;

    sprintf(buf->buf, "MessageHub name=\"%s\" org=\"%s\" enabled=%s state=%s\n           need=%02x open=%u notopen=%u\n",
            (mhub->id ? mhub->id : ""),
            (mhub->tenant ? mhub->tenant->name : ""),
            (mhub->enabled ? (mhub->enabled == 2 ? "adminDisabled" : "enabled") : "disabled"),
            mhubStateString(mhub->state), mhub->need, mhub->open, mhub->notopen);
    buf->used += strlen(buf->buf);
    if (mhub->serviceid) {
        sprintf(pbuf, "  serviceID=\"%s\"\n", mhub->serviceid);
        ism_json_putBytes(buf, pbuf);
    }
    if (mhub->timezone) {
        sprintf(pbuf, "  timezone=%s\n", ism_common_getTimeZoneName(mhub->timezone));
        ism_json_putBytes(buf, pbuf);
    }

    if (mhub->username) {
        sprintf(pbuf, "  user=%s password=%s\n", mhub->username, mhub->password ? mhub->password : "");
        ism_json_putBytes(buf, pbuf);
    }
    if (mhub->broker_count) {
        ism_json_putBytes(buf, "  brokers=\n");
        for (i=0; i<mhub->broker_count; i++) {
            ism_json_putBytes(buf, "    ");
            ism_json_putBytes(buf, mhub->brokers[i]);
            ism_json_putBytes(buf, "\n");
        }
    }
    if (mhub->rulecount) {
        ism_json_putBytes(buf, "  rules=\n");
        for (i=0; i<mhub->rulecount; i++) {
            ism_json_putBytes(buf, "    ");
            if (mhub->rules[i] == NULL) {
                sprintf(pbuf, "%s: %s", mhub->rulenames[i], mhub->rulestr[i]);
                ism_json_putBytes(buf, pbuf);
            } else {
                ruleToString(buf, mhub->rules[i]);
            }
            ism_json_putBytes(buf, "\n");
        }
    }
    if (mhub->topiccount) {
        ism_json_putBytes(buf, "  topics=\n");
        for (i=0; i<mhub->topiccount; i++) {
            sprintf(pbuf, "    %s:%u\n", mhub->topics[i]->name, mhub->topics[i]->partcount);
            ism_json_putBytes(buf, pbuf);
        }
    }
    char * ret = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,15),buf->used + 1);
    memcpy(ret, buf->buf, buf->used);
    ret[buf->used] = 0;
    return ret;
}

/*
 * Map from a single API version to the other versions to use
 */
void ism_mhub_mapKafkaVersion(ism_mhub_t * mhub, int version) {
     if (version >= 0x20)
         version = 2;
     else
         mhub->apiVersion = version;
     mhub->describeConfigsVersion = 0;
     switch (version) {
     case 0:
         mhub->messageVersion = 0;
         mhub->produceVersion = 0;
         break;
     case 1:   /* 0.10 */
     default:
         mhub->messageVersion = 1;
         mhub->produceVersion = 1;
         break;
     case 2:   /* 0.11 */
         mhub->messageVersion = 2;
         mhub->produceVersion = 1;
         break;
     }
}


#define MILLISECONDS * 1000000L
/*
 * Specify a retry delay in nanoseconds
 */
static uint64_t retryDelay(int retries) {
    switch (retries) {
    case 0: return 1 MILLISECONDS;
    case 1: return 100 MILLISECONDS;
    case 2: return 1000 MILLISECONDS;
    case 4: return 5000 MILLISECONDS;
    case 5: return 10000 MILLISECONDS;
    case 6: return 20000 MILLISECONDS;
    case 7: return 30000 MILLISECONDS;
    }
    return 60000 MILLISECONDS;
}

/*
 * Find a topic in the mhub
 */
static mhub_topic_t * findTopic(ism_mhub_t * mhub, const char * topicname) {
    int i;
    for (i=0; i<mhub->topiccount; i++) {
        if (!strcmp(mhub->topics[i]->name, topicname))
            return mhub->topics[i];
    }
    return NULL;
}

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
        transport->gotAddress(transport, grc, info);
        freeaddrinfo(info);
        ism_common_free(ism_memory_proxy_eventstreams,req);
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
    transport->gotAddress(transport, grc, info);
    freeaddrinfo(info);
    ism_common_free(ism_memory_proxy_eventstreams,req);           /* This includes the hints and sigevent */
}
#endif

/*
 * Get an address from a MHub object
 *
 */
static int mhubGetAddress(ism_server_t * server,  ism_transport_t * transport, ism_gotAddress_f gotAddress) {
    int port;
    const char * addr;
    ism_mhub_t * mhub = (ism_mhub_t *)server;
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
    } else {
        char * pos;
        char * broker = alloca(strlen(mhub->brokers[mhub->trybroker]) + 1);
        strcpy(broker, mhub->brokers[mhub->trybroker]);
        pos = strrchr(broker, ':');
        if (pos) {
            *pos++ = 0;
            port = atoi(pos);
        } else {
            port = 9093;
        }
        addr = broker;
        if (++mhub->trybroker >= mhub->broker_count)
            mhub->trybroker = 0;
        transport->server_addr = ism_transport_putString(transport, addr);
    }
    transport->serverport = port;

    req = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,16),1, sizeof(*req)+sizeof(*sigevt)+sizeof(*hints)+16);
    sigevt = (struct sigevent *)(req+1);
    hints = (struct addrinfo *)(sigevt+1);
    transport->getAddrCB = req;

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
        ism_common_free(ism_memory_proxy_eventstreams,transport->getAddrCB);
        transport->getAddrCB = NULL;
        ism_common_setErrorData(ISMRC_Error, "%s%i", "getaddrinfo_a", rc);
        return ISMRC_Error;
    }
    return 0;
}

/*
 * Request a kafka metadata on a timer
 * @param timer
 * @param timestamp
 */
static int requestShutdownTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    TRACE(5, "requestShutdownTimer: Shutting down msproxy.\n");
    ism_common_cancelTimer(timer);
    ism_common_shutdown(0);
    return 0;
}


/*
 * process a produce response
 *
 * @param mhub      The MessageHub object
 * @param topicn    The topic name which is not null terminated
 * @param topiclen  The length in bytes of the topic
 * @param partid    The part id
 * @param partrc    The part return code
 * @param offset    The returned offset of the produce
 * @param timestamp The returned timestamp of the produce (v2 and above only)
 * @return 1 if we need to request metadata
 */
static int produceResponse(ism_mhub_t * mhub, const char * topicn, int topiclen, int partid, int partrc, uint64_t offset, int64_t timestamp) {
	int needmetadata=0;
	char * topic = (char * )alloca(topiclen+1);
	memcpy(topic, topicn, topiclen);
	topic[topiclen] = '\0';
	uint64_t diff=0.0;
	double receivedMsgTime = ism_common_readTSC();
	ism_time_t currTimeInNanos = ism_common_currentTimeNanos();
	int shutdown_mhub = 0;

	TRACE(7, "produceResponse: topic=%s partid=%d partrc=%d\n", topic, partid, partrc);

	/* Get the right partition */
	mhub_part_t * partition = NULL;

	for (int tcount =0 ; tcount< mhub->topiccount; tcount++) {
		mhub_topic_t * mhub_topic = mhub->topics[tcount];
		if (!strcmp(mhub_topic->name, topic)) {
			partition = (mhub_part_t *)&mhub_topic->partitions[partid];
			break;
		}
	}

	pthread_spin_lock(&g_mhubStatLock);
	mhubMessagingStats.kakfaTotalBatchProduceAckReceivedCount++;
	diff = ((receivedMsgTime - partition->lastProduceTime) * 1e3) + 0.5; //millis
	mhubMessagingStats.kafkaTotalProduceLatencyMS += diff; //millis
	if (diff > mhubMessagingStats.kafkaProduceMaxLatencyMS) {
		mhubMessagingStats.kafkaProduceMaxLatencyMS = diff;
	}
	pthread_spin_unlock(&g_mhubStatLock);

	pthread_mutex_lock(&partition->lock);
	
	if (partrc == KAFKA_ERROR_NOERROR) {
		kafka_produce_msg_t * msgs = partition->kafka_ackwait_msg_first;
		int msgcnt=0;
		while (msgs) {
			kafka_produce_msg_t * msg;
			msg = msgs;
			msgs = msgs->next;
			msgcnt++;
			if (msg->waitValue && msg->waitID) {
			    ism_transport_ack(msg->waitID, msg->waitValue, 0, NULL);
			}
			freeKafkaEvent(msg);

		}
		partition->kafka_ackwait_msg_first=NULL;
		partition->kafka_ackwait_msg_last=NULL;
		partition->hasackwait=0;

		/* Reset produceErrorCount */
		partition->produceErrorCount=0;
		partition->produceErrorTotalCount=0;
		partition->produceErrorFirstTimeInNanos = 0;

		pthread_spin_lock(&g_mhubStatLock);
		mhubMessagingStats.kakfaTotalPendingMsgsCount -= msgcnt;
        pthread_spin_unlock(&g_mhubStatLock);
		TRACE(7, "produceResponse: topic=%s partid=%d partrc=%d msgcount=%d pendMsgs=%d totalPending=%llu\n", topic, partid, partrc, msgcnt,
										partition->kafka_msg_count, (ULL) mhubMessagingStats.kakfaTotalPendingMsgsCount );

	} else {
		pthread_spin_lock(&g_mhubStatLock);
		mhubMessagingStats.kakfaTotalBatchProduceAckErrorCount++;
        pthread_spin_unlock(&g_mhubStatLock);

        if (partition->produceErrorFirstTimeInNanos==0) {
            /* Set the Error Initial Time */
            partition->produceErrorFirstTimeInNanos= currTimeInNanos;
		}
        partition->produceErrorCount++;
        partition->produceErrorTotalCount++;
        partition->needreproduce=1; //Need to reproduce if any
		TRACE(6, "produceResponse Error: topic=%s partid=%d partrc=%d pendMsgs=%d totalProduceErrors=%llu\n", topic, partid, partrc,
												partition->kafka_msg_count, (ULL) mhubMessagingStats.kakfaTotalBatchProduceAckErrorCount );

		char tbuf[64];
		ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
		ism_common_setTimestamp(ts,partition->produceErrorFirstTimeInNanos);
		ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 7, ISM_TFF_ISO8601);
		ism_common_closeTimestamp(ts);

		 if (partition->produceErrorCount> g_produce_error_log_interval_count) {
			 partition->produceErrorCount=0;
			 LOG(ERROR, Connection, 2611, "%-s%-s%d%d%d%d%d%-s", "Event Streams produce response with Error received. Instance={0} Topic={1} partition={2} leader={3} errorTotal={4} errorCode={5} lastRC={6} errorStartTime={7}.",
							mhub->name, topic, partid, partition->leader, partition->produceErrorTotalCount, partrc, partition->produceLastRC, tbuf);
		 }

		if ((partrc==KAFKA_ERROR_UNKNOWTOPICORPARTITION || partrc==KAFKA_ERROR_LEADERNOTAVAILABLE ||
				partrc==KAFKA_ERROR_NOTLEADERFORPARTITION || partrc == KAFKA_ERROR_REQUESTTIMEOUT ||
				partrc==KAFKA_ERROR_NOTENOUGHREPLICAS || partrc==KAFKA_ERROR_NOTENOUGHREPLICASAFTERAPPEND)) {
			needmetadata=1;
		}

	    /*
		 * Disable MHub if certain amount of error reached.
		 * Check if there is maximum allowed time for Error before Shutdown
		 */
		if (partition->produceErrorTotalCount>0 && g_shutdown_onerror_time_nanos>0) {
			if ((currTimeInNanos - partition->produceErrorFirstTimeInNanos) > g_shutdown_onerror_time_nanos) {
				/* Still Having Errors after max time. Need to shutdown msproxy */
				LOG(ERROR, Connection, 2612, "%-s%-s%d%d%d%d%d%-s", "Event Streams Response Error time is more than maximum time allowed. Shutting down Event Streams. Instance={0} Topic={1} partition={2} leader={3} errorTotal={4} errorCode={5} lastRC={6} errorStartTime={7}.",
						mhub->name, topic, partid, partition->leader, partition->produceErrorTotalCount, partrc, partition->produceLastRC, tbuf);

				shutdown_mhub=1;
			}
		}
	}

	partition->produceLastRC=partrc;

	pthread_mutex_unlock(&partition->lock);

	if (shutdown_mhub) {

		ism_mhub_lock(mhub);
		if(!mhub->isKafkaConn){
			/* This is MHUB configuration.  Disabled the Mhub only without restart msproxy*/
			if (mhub->enabled==1) {
				mhub->enabled = 2;
				mhub->prev_state = mhub->state;
				mhub->state = MHS_Failed;
				if (mhub->stateChanged) {
					mhub->stateChanged(mhub);       /* Notify of state change */
				}
				ism_mhub_unlock(mhub);
				closeConnections(mhub); //Close all mhub connections.
				ism_mhub_lock(mhub);
			}
		}else{
			//This is a system wide kafka connection. restart msproxy
			ism_common_setTimerOnce(ISM_TIMER_HIGH, requestShutdownTimer, NULL,  2000000000L);
		}
		ism_mhub_unlock(mhub);
		needmetadata=0; //no need to request metadata
	}

    return needmetadata;
}

/*
 * Process a DescribeConfigsResponse
 * DescribeConfigs Response (Version: 0) => throttle_time_ms [resources]
 * throttle_time_ms => INT32
 * resources => error_code error_message resource_type resource_name [config_entries]
 *   error_code => INT16
 *   error_message => NULLABLE_STRING
 *   resource_type => INT8
 *   resource_name => STRING
 *   config_entries => config_name config_value read_only is_default is_sensitive
 *     config_name => STRING
 *     config_value => NULLABLE_STRING
 *     read_only => BOOLEAN
 *     is_default => BOOLEAN
 *     is_sensitive => BOOLEAN
 *
 */
xUNUSED static int describeConfigsResponse(ism_transport_t * transport, concat_alloc_t * buf) {
	int i = 0;
	int y=0;
	int err_code;
	char * err_msg;
	int err_len;
	int resource_type;
	char * iresource_name;
	char * resource_name = NULL;
	int resource_name_len;
	int config_count;
	char * iconfig_name;
	char * config_name;
	int config_name_len;
	char * iconfig_value;
	char * config_value;
	int config_value_len;
	int read_only;
	int is_default;
	int is_sensitive;

	int throttle_time = ism_kafka_getInt4(buf);
	int resource_count = ism_kafka_getInt4(buf);
	TRACE(5,"Received DescribeConfigs Response: ThrottleTime=%d Resource Count=%d\n", throttle_time, resource_count);

	 for (i=0; i< resource_count; i++) {
		 err_code = ism_kafka_getInt2(buf);
		 err_len = ism_kafka_getString(buf, &err_msg);
		 if(err_code==0){
			 resource_type = ism_kafka_getInt1(buf);
			 resource_name_len = ism_kafka_getString(buf, &iresource_name);
			 config_count = ism_kafka_getInt4(buf);

			 if(resource_name_len>0){
				 resource_name = alloca(resource_name_len+1);
				 memcpy(resource_name, iresource_name,resource_name_len );
				 resource_name[resource_name_len]='\0';
			 }
			 TRACE(5,"DescribeConfigs Resource: count=%d err=%d resource_type=%d resource_name=%s config_count=%d\n", i, err_code, resource_type, (resource_name!=NULL)?resource_name:"", config_count);
			 for (y=0; y < config_count; y++) {
				 config_name_len = ism_kafka_getString(buf, &iconfig_name);
				 if(config_name_len>0){
					 config_name = alloca(config_name_len+1);
					 memcpy(config_name, iconfig_name,config_name_len );
					 config_name[config_name_len]='\0';
				 }
				 config_value_len = ism_kafka_getString(buf, &iconfig_value);
				 if(config_value_len>0){
					 config_value = alloca(config_value_len+1);
					 memcpy(config_value, iconfig_value,config_value_len );
					 config_value[config_value_len]='\0';
				 }

				 read_only = ism_kafka_getInt1(buf);
				 is_default = ism_kafka_getInt1(buf);
				 is_sensitive = ism_kafka_getInt1(buf);

				 TRACE(5,"DescribeConfigs Config: count=%d config_name=%s config_value=%s read_only=%d is_default=%d is_sensitive=%d\n", y,config_name, config_value, read_only, is_default, is_sensitive );

				 //TODO: Store the Config Somewhere.
			 }
		 } else {
			 char *error_msg = alloca(err_len +1);
			 memcpy(error_msg, err_msg, err_len);
			 error_msg[err_len]='\0';
			 TRACE(5,"DescribeConfigs Response Error: count=%d error_code=%d error_msg=%s\n", i,err_code, error_msg );
		 }
	 }

	 return 0;

}


/*
 * We expect to receive the ACK response here
 *
 * V0 format: ProduceResponse => [TopicName [Partition ErrorCode Offset]]
 * V1 format: ProduceResponse => [TopicName [Partition ErrorCode Offset]] ThrottleTime
 * V2 format: ProduceResponse => [TopicName [Partition ErrorCode Offset Timestamp]] ThrottleTime
 * TopicName => string
 * Partition => int32
 * ErrorCode => int16
 * Offset => int64
 * Timestamp +> int64
 * ThrottleTime => int32
 */
static int mhubReceiveKafka(ism_transport_t * transport, char * inbuf, int buflen, int kind) {
    concat_alloc_t cbuf = {inbuf, buflen, buflen};
    concat_alloc_t * buf = &cbuf;
    ism_mhub_t * mhub = (ism_mhub_t *)(transport->server);
    int topiclen;;
    char * topicname;
    int partid;;
    int part_count;
    int partrc;
    int64_t offset;
    int64_t timestamp = 0;        /* Server assigned timestamp: v2 and above */
    int throttle_time;            /* Time the request was throttled due to quota: v1 and above */
    int needmetadata = 0;
    int response_count;
    xUNUSED ism_time_t now;
    int i;
    int p;

    transport->lastAccessTime = ism_common_readTSC();

    /*
     * Check if we are expecting a SASL response.  This does not come with a kafka header
     * so we cannot look at the Correlation ID
     */
    if (transport->pobj->auth_require) {
        return receiveSASL(transport, inbuf, buflen, kind);
    }

    /*
     * Check for the type of response based on the correlation ID
     */
    int  corrid = ism_kafka_getInt4(buf);

    /*
     * Parse metadata and any SASL responses
     */
    if (corrid >= 0x20000) {
        return mhubReceiveMetadata(transport, inbuf, buflen, kind);
    }

    /*
     * Process DescribeConfigs Response
     */
   // if (corrid == 0x10005) {
   // 		describeConfigsResponse(transport, buf);
   // 		return 0;
   // }

    /*
     * Parse an ProduceResponse (ACK)
     */
    now = ism_common_currentTimeNanos();
    response_count = ism_kafka_getInt4(buf);
    for (i=0; i< response_count; i++) {
        topiclen = ism_kafka_getString(buf, &topicname);
        part_count = ism_kafka_getInt4(buf);       /* partition count */
        for (p=0; p<part_count; p++) {
            partid = ism_kafka_getInt4(buf);
            partrc = ism_kafka_getInt2(buf);
            offset = ism_kafka_getInt8(buf);
            if (mhub->produceVersion >= 2) {
                timestamp = ism_kafka_getInt8(buf);
            }
            needmetadata += produceResponse(mhub, topicname, topiclen, partid, partrc, offset, timestamp);
        }
    }
    if (mhub->produceVersion >= 1) {
        throttle_time = ism_kafka_getInt4(buf);    /* We do not use this */
        if (throttle_time != 0) {
            pthread_spin_lock(&g_mhubStatLock);
            uint64_t total_throttle_time =  mhubMessagingStats.kafkaProduceTotalThrottleTimeMS += throttle_time;
            pthread_spin_unlock(&g_mhubStatLock);
            TRACE(7, "MHUB KafkaProduceResponse: throttle=%u totalThrottleTime=%llu\n", (uint32_t)throttle_time, (ULL) total_throttle_time);
        }
    }

    /*
     * Request metadata
     */
    if (needmetadata > 0) {
         ism_mhub_lock(mhub);
         if (mhub->enabled==1 && !mhub->expectingMetadata) {
             if (mhub->metadata && mhub->metadata->pobj->state == TCP_CONNECTED) {
                 mhub->expectingMetadata = 1;
                 mhubMetadataRequest(mhub, transport);
             } else {
                //Metadata is broken, need new transport.
                 mhub->prev_state = mhub->state;
                 mhub->state = MHS_Opening;
                 if (mhub->stateChanged) {
                    mhub->stateChanged(mhub);       /* Notify of state change */
                 }
                 ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)mhubRetryConnect, mhub, retryDelay(0));
             }
         }
         ism_mhub_unlock(mhub);
    }
    return 0;
}


/*
 * Process metadata for a topic
 */
static int processTopicMetadata(ism_mhub_t * mhub, const char * topicn, int topiclen, int topicrc, int partcount) {
	int rc=0;
    char * topicname = alloca(topiclen + 1);
    memcpy(topicname, topicn, topiclen);
    topicname[topiclen] = 0;
    mhub_topic_t * topic = findTopic(mhub, topicname);
    TRACE(9, "Topic metadata: mhub=%s, topic=%s rc=%d partcount=%d\n", mhub->id, topicname, topicrc, partcount);
    if (topic) {
        if (topicrc == 0) {
            if (topic->partcount != partcount) {
                topic = changePartitions(mhub, topic, partcount);
            }
            topic->valid = 1;
        } else {
            if (topic->valid < 2) {
                LOG(WARN, Server, 974, "%s%-s%-s%d", "MessageHub topic metadata error: Org={0} ID={1} Topic={2} RC={3}",
                                mhub->tenant->name, mhub->id, topic->name, topicrc);

                //Determine if the Error is retriable or not before set the topic to INVALID permanently
                //If internal MHUB Kafka Connection, we need to keep retry.
                if(mhub->isKafkaConn || topicrc == KAFKA_ERROR_LEADERNOTAVAILABLE || topicrc==KAFKA_ERROR_REQUESTTIMEOUT
                	   || topicrc==KAFKA_ERROR_NETWORKEXCEPTION){
                		rc = topicrc; //Mark to RETRY
                }else{
                		//Other errrors are consider NOT RETRIABLE.
                		//Will not retry to request the metadata and
                		//mark the topic to be invalid.
                		rc=0;  //Mark not to RETRY
                		topic->valid = 2;   /* Topic not valid */
                }

            }
        }
    }
    return rc;
}

/*
 * Structure to store info for an async timer job
 */
struct mhub_dataConnectInfo {
    ism_mhub_t * mhub;
    char *   topicname;
    char *   broker;
    int      partid;
    int      port;
    uint32_t leader;
    uint32_t resv;
};

/*
 * Process metadata for a part
 */
static int processPartMetadata(ism_mhub_t * mhub, mhub_broker_list_t * brokers, int brokercnt,
        const char * topicn, int topiclen, int partid, int partrc, int leader) {
    int i;
    char * topicname = alloca(topiclen + 1);
    memcpy(topicname, topicn, topiclen);
    topicname[topiclen] = 0;
    mhub_topic_t * topic = findTopic(mhub, topicname);
    TRACE(9, "Partition metadata: mhub=%s, topic=%s partid=%u rc=%d leader=%d\n", mhub->id, topicname, partid, partrc, leader);
    if (topic) {
        mhub_part_t * part = topic->partitions+partid;
        if (partrc == 0 && leader < brokercnt) {
            part->valid = 1;
            part->leader = leader;
            if (!part->transport) {
                int brokerlen = 0;
                for (i = 0; i < brokercnt; i++){
                    if (brokers[i].nodeid == leader){
                        brokerlen = strlen(brokers[i].broker);
                    }
                }
                struct mhub_dataConnectInfo * info = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,17),sizeof(*info) + topiclen + brokerlen + 2);
                info->mhub = mhub;
                info->topicname = (char *)(info+1);
                strcpy(info->topicname, topicname);
                info->partid = partid;
                info->broker = info->topicname + topiclen + 1;
                info->leader = leader;
                for (i = 0; i < brokercnt; i++){
                	if (brokers[i].nodeid == leader){
                		strcpy(info->broker, brokers[i].broker);
                		info->port = brokers[i].port;
                	}
                }
                ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)mhubCreateData, info, 1000000);
            }
        } else {
            if (part->valid < 2) {
                LOG(WARN, Server, 975, "%s%-s%-s%u%d", "MessageHub partition metadata error: Org={0} ID={1} Topic={2} Part={3} RC={4}",
                                mhub->tenant->name, mhub->id, topic->name, partid, partrc);
                part->valid = 2;   /* Topic not valid */
            }
        }
    }
    return 0;
}

/*
 * Receive the ApiVersionsResponse
 *
 * We only care about a few of the APIs
 */
static int receiveApiVersions(ism_transport_t * transport, concat_alloc_t * buf) {
    ism_mhub_t * mhub = (ism_mhub_t *)(transport->server);
    int  i;
    int  verElect = -1;
    int krc = ism_kafka_getInt2(buf);
    if (!krc) {
         int count = ism_kafka_getInt4(buf);
         ism_mhub_lock(mhub);
         if (!mhub->versionKnown) {
             mhub->verSaslAuthenticate = 0xff;
             mhub->verSaslHandshake = 0xff;
             mhub->verInitProducerId = 0xff;
             mhub->verDescribeConfigs = 0xff;
             mhub->versionKnown = 1;
             for (i=0; i<count; i++) {
                 int api = ism_kafka_getInt2(buf);
                 int minver = ism_kafka_getInt2(buf);
                 int maxver = ism_kafka_getInt2(buf);
                 if (minver != 0) {
                     TRACE(9, "Kafka ApiVersions API=%u min=%u max=%u\n", api, minver, maxver);
                 }
                 if (api == ProduceRequest)
                     mhub->verProduce = maxver;
                 else if (api == FetchRequest)
                     mhub->verFetch = maxver;
                 else if (api == MetadataRequest)
                     mhub->verMetadata = maxver;
                 else if (api == InitProducerIdRequest)
                     mhub->verInitProducerId = maxver;
                 else if (api == SaslHandshakeRequest)
                     mhub->verSaslHandshake = maxver;
                 else if (api == SaslAuthenticateRequest)
                     mhub->verSaslAuthenticate = maxver;
                 else if (api == DescribeConfigsRequest)
                     mhub->verDescribeConfigs = maxver;
                 else if (api == ElectPreferredLeader)
                     verElect = maxver;
             }
         }

         /*
          * Find kafka version from ApiVersions output.
          * This is just used for log and trace information
          */
         const char * kver = "before 0.10";
         switch (mhub->verProduce) {
         case 2: kver = "0.10";   break;
         case 3: kver = "0.11";   break;
         case 4: kver = "1.0";    break;
         case 5: kver = "1.1";    break;
         case 6: kver = "2.0";    break;
         default:
             kver = "after 2.3";
             if (mhub->verFetch == 10) {
                 if (verElect == -1)
                     kver = "2.1";
                 else
                     kver = "2.2";
             }
             if (mhub->verFetch == 11) {
                  kver = "2.3";
             }
             break;
         }
         mhub->kafka_version = kver;   /* Value is always a constant */
         /* IF we are version 1.0 or above, map to msgfmt=2 unless it is explicitly overridden */
         if (mhub->verProduce >= 4 && mhub->apiVersion < 2) {
             int mhubver = ism_common_getIntConfig("MessageHubAPIVersion", 2);
             if (mhubver < 0 && mhubver > 2)
                 mhubver = 2;
             ism_mhub_mapKafkaVersion(mhub, mhubver);
         }
         ism_mhub_unlock(mhub);
         TRACE(4, "Kafka version %s: mhub=%s produce=%u fetch=%u metadata=%u saslHandshake=%u saslAuthenticate=%u\n",
                 kver, mhub->id, mhub->verProduce, mhub->verFetch, mhub->verMetadata, mhub->verSaslHandshake, mhub->verSaslAuthenticate);
    }
    return 0;
}


/*
 * Receive a SASL response.
 *
 * For now we assume the SASL is not in a Kafka response header.  This can change
 * based on how the SASL is invoked.
 */
static int receiveSASL(ism_transport_t * transport, char * inbuf, int buflen, int kind) {
    concat_alloc_t cbuf = {inbuf, buflen, buflen};
    concat_alloc_t * buf = &cbuf;
    ism_mhub_t * mhub = (ism_mhub_t *)(transport->server);
    char * reason;
    int rc = 0;
    int reasonlen;

    /*
     * Handle SASL which comes in without any frame (legacy before Kafka 1.0)
     */
    if (transport->pobj->auth_require) {
        if (SHOULD_TRACEC(9, Kafka)) {
            char trcbuf[64];
            int maxsize = maxsize = ism_common_getTraceMsgData()+64;
            sprintf(trcbuf, "Kafka noframe %d receive connect=%u", transport->pobj->auth_require, transport->index);
            traceDataFunction(trcbuf, 0, __FILE__, __LINE__, inbuf, buflen, maxsize);
        }
        if (transport->pobj->auth_require == 1) {
            /* Initial response */
            transport->pobj->auth_require = 2;
            sendSASL(transport, buf, 2);
            return 0;
        }
        if (transport->pobj->auth_require == 2) {
            /* Final response */
            transport->frame = ism_transport_frameKafka;
            rc  = ism_kafka_getInt2(buf);
            reasonlen = ism_kafka_getString(buf, &reason);
            if (reasonlen && reason) {
                char * temp = alloca(reasonlen + 1);
                memcpy(temp, reason, reasonlen);
                temp[reasonlen] = 0;
                reason = temp;
            } else {
                reason = NULL;
            }
        }
    }

    /*
     * Handle SASL and related items which come in with a frame (Kafka 1.0 or later)
     */
    else {
        char xbuf [2048];
        concat_alloc_t obuf = {xbuf, sizeof xbuf};
        int  corrid = ism_kafka_getInt4(buf);
        switch (corrid) {

        /*
         * SaslHandshakeResponse
         */
        case 0x30000:
            rc = ism_kafka_getInt2(&obuf);
            if (rc == 0) {
                sendSASL(transport, &obuf, 2);
                return 0;
            }
            break;

        /*
         * SaslAuthenticateResponse
         */
        case 0x30002:

            rc  = ism_kafka_getInt2(buf);
            reasonlen = ism_kafka_getString(buf, &reason);
            if (reasonlen && reason) {
                char * temp = alloca(reasonlen + 1);
                memcpy(temp, reason, reasonlen);
                temp[reasonlen] = 0;
                reason = temp;
            } else {
                reason = NULL;
            }

            if(rc != 0 ||  (mhub->sasl_mechanism == SASL_MECHANISM_PLAIN)){
                //If it is SASL PLain. This is the last reponse from the server.
                break;
            }

            //For SASL SCRAM, there are more request/response needed.
            if (mhub->sasl_mechanism == SASL_MECHANISM_SCRAM_SHA_512 || mhub->sasl_mechanism == SASL_MECHANISM_SCRAM_SHA_256){

                ism_sasl_scram_context * context = transport->pobj->sals_scram_context;

               if (context->state == SASL_SCRAM_STATE_CLIENT_FIRST_MESSAGE){
                   //HANDLE: Server First Message
                    char * receivedSASLBytes;
                   int len = ism_kafka_getBytes(buf, &receivedSASLBytes );
                   char * receivedStr = alloca(len+1);
                   memcpy(receivedStr, receivedSASLBytes, len);
                   receivedStr[len]=0;

                   //Need server first message for later processing
                   ism_sasl_scram_server_set_first_message(context,receivedStr );

                   ism_prop_t * props = context->server_response_props;
                   ism_sasl_scram_parse_properties(receivedStr, props);

                   sendSASL(transport, buf, 2);

                   //Need the server final message for SASL SCRAM. So just return.
                   return 0;

               } else if(context->state == SASL_SCRAM_STATE_CLIENT_FINAL_MESSAGE){
                    //HANDLE: Server Final Message.
                    char * receivedSASLBytes;
                    int len = ism_kafka_getBytes(buf, &receivedSASLBytes );
                    char * receivedStr = alloca(len+1);
                    memcpy(receivedStr, receivedSASLBytes, len);
                    receivedStr[len]=0;
                    ism_prop_t * props = context->server_response_props;
                    ism_sasl_scram_parse_properties(receivedStr, props);

                    const char * verification = ism_common_getStringProperty(props, "v");
                    if(verification!=NULL){
                        TRACE(7, "SASL SCRAM Response Received: verification=%s \n", verification);
                    }

                    char error_buf[512];
                    concat_alloc_t error_outbuf={error_buf, sizeof(error_buf)};

                    if(ism_sasl_scram_server_final_verify(context, context->server_response_props, &error_outbuf)){
                        //SASL Authentication Failed
                        rc=ISMRC_NotAuthorized;
                        char * error = NULL;
                        if(error_outbuf.used>0){
                            error = alloca(error_outbuf.used +1);
                            memcpy(error, error_outbuf.buf, error_outbuf.used);
                            error[error_outbuf.used]=0;
                        }
                        TRACE(5, "SASL Authentication failed: error=%s\n", (error)?error: "");

                    }else{
                        //SASL Authentication Succeed
                        TRACE(7, "SASL Authentication succeeded\n");
                        rc=0;

                    }
                    ism_common_freeAllocBuffer(&error_outbuf);
                    //Clean context for next authentication.
                   if(transport->pobj->sals_scram_context){
                       ism_sasl_scram_context_destroy(transport->pobj->sals_scram_context);
                       transport->pobj->sals_scram_context = NULL;
                   }

               }
            }

            break;

        /*
         * ApiVersionsResponse
         */
        case 0x30003:
            receiveApiVersions(transport, buf);
            if (mhub->mhubSASL) {
                sendSASL(transport, &obuf, 1);
            } else {
                moreConnected(transport);
            }
            return 0;
        }
    }

    if (reason && *reason) {
        TRACE(7, "receiveSASL: connect=%d name=%s rc=%d reason=%s\n", transport->index, transport->name, rc, reason);
    } else {
        TRACE(7, "receiveSASL: connect=%d name=%s rc=%d\n", transport->index, transport->name, rc);
    }

    if (rc) {
        ism_common_setErrorData(ISMRC_NotAuthorized, "%s%d", reason, rc);
        char * xbuf = alloca(2048);
        ism_common_formatLastError(xbuf, 2048);
        transport->close(transport, ISMRC_NotAuthorized, 0, xbuf);
        /* Retry, but wait a while since not authorized rarely just goes away */
        /* Disable this topic */
        ism_mhub_lock(mhub);
        if (transport->pobj->kafka_type == KafkaMetadata) {
            mhub->enabled = 2;
            mhub->prev_state = mhub->state;
            mhub->state = MHS_Failed;
            if (mhub->stateChanged) {
                  mhub->stateChanged(mhub);       /* Notify of state change */
            }
        } else {
            if (transport->pobj->topic) {
                mhub_topic_t * topic = findTopic(mhub, transport->pobj->topic);
                topic->valid = 2;
            }
        }
        ism_mhub_unlock(mhub);
        return ISMRC_NotAuthorized;
    } else {
        rc = moreConnected(transport);
    }
    return rc;
}


/*
 * We expect to receive metadata, SASL, and a few other responses here
 *
 * Parse Metadata response
 *
 * MetadataResponse => [Broker][TopicMetadata]
 * Broker => NodeId Host Port  (any number of brokers may be returned)
 *   NodeId => int32
 *   Host => string
 *   Port => int32
 * TopicMetadata => TopicErrorCode TopicName [PartitionMetadata]
 *   TopicErrorCode => int16
 * PartitionMetadata => PartitionErrorCode PartitionId Leader Replicas Isr
 *   PartitionErrorCode => int16
 *   PartitionId => int32
 *   Leader => int32
 *   Replicas => [int32]
 *   Isr => [int32]
 */
static int mhubReceiveMetadata(ism_transport_t * transport, char * inbuf, int buflen, int kind) {
    concat_alloc_t cbuf = {inbuf, buflen, buflen};
    concat_alloc_t * buf = &cbuf;
    ism_mhub_t * mhub = (ism_mhub_t *)(transport->server);
    int  i;
    int  j;
    int rc = 0;
    transport->lastAccessTime = ism_common_readTSC();
    /*
     * Check if we are expecting a SASL response.  This does not come with a kafka header
     */
    if (transport->pobj->auth_require) {
        return receiveSASL(transport, inbuf, buflen, kind);
    }

    /* Get the correlation ID */
    int  corrid = ism_kafka_getInt4(buf);

    /*
     * Check for SASL responses
     */
    if (corrid >= 0x30000 && corrid <= 0x3000F) {
        return receiveSASL(transport, inbuf, buflen, kind);
    }

    /*
     * Check for the type of response based on the correlation ID
     */
    if (corrid == 0x20000) {             /* Metadata */
        int broker_cnt = ism_kafka_getInt4(buf);
        mhub_broker_list_t * brokers = alloca(broker_cnt * sizeof(mhub_broker_list_t));
        for (i=0; i<broker_cnt; i++) {
            brokers[i].nodeid  = ism_kafka_getInt4(buf);
            brokers[i].broker_len = ism_kafka_getString(buf, (char * *)&brokers[i].broker);
            brokers[i].port = (uint16_t) ism_kafka_getInt4(buf);
        }
        int topic_count = ism_kafka_getInt4(buf);
        for (i=0; rc==0 && i<topic_count; i++) {
            char * topicstr;
            int topicrc = ism_kafka_getInt2(buf);
            int topiclen = ism_kafka_getString(buf, &topicstr);
            int part_cnt = ism_kafka_getInt4(buf);
            if (ism_kafka_more(buf)<0 || part_cnt >= 0x10000 || topiclen < 1) {
                 TRACE(5, "MessageHub metadata incomplete: connect=%u name=%s\n", transport->index, transport->name);
                 rc = 2;
            } else {
                rc = processTopicMetadata(mhub, topicstr, topiclen, topicrc, part_cnt);
                for (j=0; rc==0 && j<part_cnt; j++) {
                    int isr;
                    int partrc =  ism_kafka_getInt2(buf);
                    int partid =  ism_kafka_getInt4(buf);
                    int leader =  ism_kafka_getInt4(buf);
                    int replica = ism_kafka_getInt4(buf) & 0xffff;
                    buf->pos += 4 * replica;   /* Skip over replica info */
                    isr =         ism_kafka_getInt4(buf) & 0xffff;
                    buf->pos += 4 * isr;       /* Skip over isr info */
                    if (ism_kafka_more(buf) >= 0) {
                        processPartMetadata(mhub, brokers, broker_cnt, topicstr, topiclen, partid, partrc, leader);
                    } else {
                        TRACE(5, "MessageHub metadata incomplete: connect=%u name=%s\n", transport->index, transport->name);
                        rc = 2;
                    }
                }
            }
        }
        mhub->expectingMetadata = 0;
    }
    if (rc) {
        transport->close(transport, ISMRC_BadClientData, 0, "MessageHub metadata incomplete");

        ism_mhub_lock(mhub);
        if(!g_shuttingDown && mhub->enabled==1)
        		ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)mhubRetryConnect, mhub, retryDelay(mhub->retry++));
        ism_mhub_unlock(mhub);

        return 1;
    } else {
        pthread_spin_lock(&mhub->lock);
        mhub->prev_state = mhub->state;
        mhub->state = MHS_Opening;
        if (mhub->stateChanged) {
            mhub->stateChanged(mhub);       /* Notify of state change */
        }
        pthread_spin_unlock(&mhub->lock);
        TRACE(9, "MessageHub metadata process complete: connect=%u name=%s\n", transport->index, transport->name);
    }
    return 0;
}

/*
 * Create an outgoing metadata connection
 *
 * This must be done in the low timer thread or another similar thread where it is
 * OK to wait.
 */
static int createMetadataConnection(ism_mhub_t * mhub) {
    char xbuf[256];
    if (g_shuttingDown)
        return 0;

    ism_transport_t * transport = ism_transport_newOutgoing(NULL, 1);
    TRACE(5, "Start mhub metadata connection: org=%s mhub=%s\n", mhub->tenant->name, mhub->id);
    ism_tcp_init_transport(transport);
    transport->originated = 1;
    transport->protocol = "mhub_metadata";
    transport->protocol_family = "kafka";
    transport->tid = 0;
    transport->ready = 7;  //Set ready for Connect Timeout. See ddosTimer
    transport->connected = ism_mhub_connected;
    ism_kafka_con_t * pobj = (ism_kafka_con_t *) ism_transport_allocBytes(transport, sizeof(ism_kafka_con_t), 1);
    transport->pobj = pobj;
    transport->receive = mhubReceiveMetadata;
    transport->closing = (ism_transport_closing_t)ism_mhub_closing;
    transport->server = pobj->server = (ism_server_t *)mhub;
    pobj->transport = transport;
    sprintf(xbuf, "%s:%s:meta", mhub->tenant->name, mhub->id);
    transport->name = ism_transport_putString(transport, xbuf);
    transport->clientID = transport->name;
    pobj->state = TCP_CON_IN_PROCESS;
    pobj->nodeID = 0;
    pobj->kafka_type = KafkaMetadata;
    mhub->metadata = transport;
    int rc = ism_kafka_createConnection(transport, (ism_server_t *)mhub);
    if(rc){
    		char * erbuf = alloca(2048);
		ism_common_formatLastError(erbuf, 2048);
		TRACE(7, "Failed create the metadata connection. name=%s rc=%d errmsg=%s\n", transport->clientID,  rc, erbuf);
		transport->close(transport, rc, 0, erbuf);
    }else{
    		TRACE(6, "Start mhub metadata connection: connect=%u name=%s\n", transport->index, transport->name);
    }
    return 0;
}


/*
 * Retry the connection on a timer
 */
static int mhubRetryConnect(ism_timer_t key, ism_time_t now, void * userdata) {
    ism_mhub_t * mhub = (ism_mhub_t *) userdata;
    /* If this is a timer, it is only run once */
    if (key)
        ism_common_cancelTimer(key);

    //Only create connection if MSPROXY is not shutdown
    if(!g_shuttingDown){
    		createMetadataConnection(mhub);
    }else{
    		TRACE(5, "Failed to connect metadata connection. Msproxy is shutting down. tenant=%s name=%s\n",
    					(mhub->tenant!=NULL)?mhub->tenant->name:"", mhub->name);
    	}
    return 0;
}

/*
 * Retry the data connection on a timer
 */
xUNUSED static int mhubDataRetryConnect(ism_timer_t key, ism_time_t now, void * userdata) {
	 mhub_part_t * mhub_part = (mhub_part_t *) userdata;
	/* If this is a timer, it is only run once */
    if (key)
        ism_common_cancelTimer(key);
    pthread_mutex_lock(&mhub_part->lock);
    ism_transport_t * transport= mhub_part->transport;
    pthread_mutex_unlock(&mhub_part->lock);

    if(!g_shuttingDown){
    	 	 transport->ready = 7;  //Set ready for Connect Timeout. See ddosTimer
		int rc = ism_kafka_createConnection(transport, transport->pobj->server);
		if(rc){
			char * erbuf = alloca(2048);
			ism_common_formatLastError(erbuf, 2048);
			TRACE(7, "Failed create the data connection. name=%s rc=%d errmsg=%s\n", transport->clientID, rc, erbuf);
			transport->close(transport, rc, 0,  erbuf);
		}
    } else {
    		TRACE(5, "Failed to connect. Msproxy is shutting down. name=%s\n", transport->clientID);
    }
    return 0;
}


/*
 * Create an outgoing data connection
 *
 * This must be done in the low timer thread or another similar thread where it is
 * OK to wait.
 */
static int createDataConnection(ism_mhub_t * mhub, mhub_topic_t * topic, int part, int nodeid, mhub_broker_list_t * broker) {
    char xbuf[256];
    char * name;
    ism_transport_t * transport = ism_transport_newOutgoing(NULL, 1);
    ism_tcp_init_transport(transport);
    transport->originated = 1;
    transport->protocol = "mhub_data";
    transport->protocol_family = "kafka";
    transport->tid = 0;
    transport->ready = 7;  //Set ready for Connect Timeout. See ddosTimer
    transport->connected = ism_mhub_connected;

    /* Store target server and port */
    transport->client_host = name = ism_transport_allocBytes(transport, broker->broker_len + 1, 0);
    memcpy(name, broker->broker, broker->broker_len);
    name[broker->broker_len] = 0;
    transport->clientport = broker->port;

    ism_kafka_con_t * pobj = (ism_kafka_con_t *) ism_transport_allocBytes(transport, sizeof(ism_kafka_con_t), 1);
    transport->pobj = pobj;
    transport->receive = mhubReceiveKafka;
    transport->closing = ism_mhub_closing;
    transport->server = pobj->server = (ism_server_t *)mhub;
    pobj->transport = transport;
    sprintf(xbuf, "%s:%s:%d", mhub->tenant->name, topic->name, part);
    transport->name = ism_transport_putString(transport, xbuf);
    transport->clientID = transport->name;
    transport->pobj->topic = (char *)ism_transport_putString(transport, topic->name);
    pobj->state = TCP_CON_IN_PROCESS;
    pobj->nodeID = nodeid;
    pobj->kafka_type = KafkaMessageHub;
    pobj->partID = part;
    topic->partitions[part].transport = transport;
    int rc = ism_kafka_createConnection(transport, (ism_server_t *)mhub);
    if (rc) {
		//Close the connection
		char * erbuf = alloca(2048);
		ism_common_formatLastError(erbuf, 2048);
		TRACE(7, "Failed create the data connection. name=%s partition=%d nodeid=%d rc=%d errmsg=%s\n", transport->clientID, part, nodeid, rc, erbuf);
		transport->close(transport, rc, 0, erbuf);
	} else {
		TRACE(6, "Start mhub data connection: connect=%u name=%s\n", transport->index, transport->name);
	}
    return 0;
}

/*
 * Timer method to allow a createDataConnection
 *
 * This is done as we do not want the createDataConnection to run in an IoP thread.
 * This should be done on the  timer thread.
 */
static int mhubCreateData(ism_timer_t key, ism_time_t now, void * userdata) {
     if (key)
         ism_common_cancelTimer(key);
     struct mhub_dataConnectInfo * info = (struct mhub_dataConnectInfo *)userdata;
     mhub_topic_t * topic = findTopic(info->mhub, info->topicname);
     mhub_broker_list_t broker;
     broker.broker = info->broker;
     broker.broker_len = strlen(info->broker);
     broker.port = info->port;
     TRACE(8, "mhubCreateData mhub=%s topic=%s broker=%s broker_len=%d port=%d\n", info->mhub->id, (topic ? topic->name : ""),
             broker.broker, broker.broker_len, broker.port);
     if (topic) {
         createDataConnection(info->mhub, topic, info->partid, info->leader, &broker);
     }
     ism_common_free(ism_memory_proxy_eventstreams,info);
     return 0;
}

/*
 * Make the metadata request
 */
static void mhubMetadataRequest(ism_mhub_t * mhub, ism_transport_t * transport) {
    char xbuf [1024];
    concat_alloc_t cbuf = {xbuf, sizeof xbuf, 4};
    concat_alloc_t * buf = &cbuf;
    int topic_count = 0;
    int topic_count_pos;
    int i;

    if (g_shuttingDown)
        return;

    TRACE(6, "MessageHub MetadataRequest: connect=%u name=%s broker=%s:%u host=%s\n",
            transport->index, transport->name, transport->server_addr, transport->serverport,
            transport->client_host ? transport->client_host : "");
    ism_kafka_putInt4(buf, MetadataRequest0);
    ism_kafka_putInt4(buf, 0x20000);   /* Correlation ID */
    ism_kafka_putString(buf, ism_common_getHostnameInfo(), -1);   /* Use our hostname as the kafka clientID */
    topic_count_pos = buf->used;
    buf->used += 4;
    for (i=0; i<mhub->topiccount; i++) {
        mhub_topic_t * topic = mhub->topics[i];
        ism_kafka_putString(buf, topic->name, -1);
        TRACE(8, "MessageHub MetadataRequest for topic: %s\n", topic->name);
        topic_count++;
    }
    ism_kafka_putInt4At(buf, topic_count_pos, topic_count);
    ism_kafka_putString(buf, transport->pobj->topic, -1);
    transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);
}

/*
 * Make the DescribeConfigs request
 * DescribeConfigs Request (Version: 0) => [resources]
    resources => resource_type resource_name [config_names]
    resource_type => INT8
    resource_name => STRING
    config_names => STRING
 */
xUNUSED static void mhubDescribeConfigsRequest(ism_mhub_t * mhub, mhub_part_t * mhub_part, ism_transport_t * transport) {
    char xbuf [1024];
    concat_alloc_t cbuf = {xbuf, sizeof xbuf, 4};
    concat_alloc_t * buf = &cbuf;
    char nodeid_str[32];

    if (g_shuttingDown)
        return;

    //Request Headers
    ism_kafka_putInt2(buf, DescribeConfigsRequest);         /* Api key */
    ism_kafka_putInt2(buf, mhub->describeConfigsVersion);  /*  DescribeConfigs version */
    ism_kafka_putInt4(buf, 0x10005);                      /* CorrID   */
    ism_kafka_putString(buf, transport->name, -1);

    //DesribeConfigs
    ism_kafka_putInt4(buf, 1);   // Do for 1 Broker Resource.
    ism_kafka_putInt1(buf, 4);  //Resource Type. Topic-0. Broker=4
    sprintf(nodeid_str, "%d", transport->pobj->nodeID);
    ism_kafka_putString(buf,nodeid_str, -1); //Put broker nodeid

    ism_kafka_putInt4(buf, 1);   //Config Array Count. Need to increment this if add more config
    ism_kafka_putString(buf,"message.max.bytes", -1); //ConfigName: message.max.bytes

    TRACE(5, "MessageHub DescribeConfigs Request: connect=%u broker=%s:%u host=%s node_id=%s\n",
            transport->index, transport->server_addr, transport->serverport,
            transport->client_host ? transport->client_host : "", nodeid_str);

    transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);
}


/*
 * Continue with connection which might be after SASL or right after connect.
 *
 * The rc is known to be zero when this is called.
 */
static int moreConnected(ism_transport_t * transport) {
    ism_mhub_t * mhub = (ism_mhub_t *)transport->server;
    transport->ready = 1;
    transport->pobj->auth_require = 0;
    transport->lastAccessTime = ism_common_readTSC();
    TRACE(5, "Event Streams connection successful: connect=%u, name=%s\n", transport->index, transport->name);
    if (mhub->moreLogs)
        LOG(INFO, Server, 977, "%s%s%s" "Event Streams connection started: Name={0} ID={2} Broker={1}",
            transport->name, mhub->name, transport->client_host ? transport->client_host : transport->server_addr, mhub->name);
    if (transport->pobj->kafka_type == KafkaMetadata) {
        mhub->expectingMetadata = 1;
        mhubMetadataRequest(mhub, transport);
    } else {
        if (transport->pobj->topic) {
            mhub_topic_t * topic = findTopic(mhub, transport->pobj->topic);
            if (topic && topic->partcount > (uint32_t)transport->pobj->partID) {
                mhub_part_t * part = topic->partitions + transport->pobj->partID;
                pthread_mutex_lock(&part->lock);
                part->open = 1;
                part->needreproduce=1;
                pthread_mutex_unlock(&part->lock);
                mhub->open +=1;
                mhub->notopen -=1;
            }
        }

        ism_mhub_lock(mhub);
        if (!mhub->produce_timer) {
            int time = mhubBatchTimeMillis / 3;
            mhub->produce_timer = ism_common_setTimerRate(ISM_TIMER_HIGH,  mhubPartitionProduceTimer,
                            mhub, mhubBatchTimeMillis, time, TS_MILLISECONDS);
        }
        ism_mhub_unlock(mhub);
    }
    return 0;
}

/*
 * Send authentication related requests
 *
 * If kind==2 we are sending
 */
static void sendSASL(ism_transport_t * transport, concat_alloc_t * buf, int kind) {
    ism_mhub_t * mhub = (ism_mhub_t *)(transport->server);
    int userlen;
    int passwordlen;
    buf->used = 4;
    if (kind == 2) {
        userlen = mhub->username ? strlen(mhub->username) : 0;

        if(mhub->sasl_mechanism == SASL_MECHANISM_PLAIN){
            //Sasl PLAIN
            passwordlen = mhub->password ? strlen(mhub->password) : 0;
            if (!transport->pobj->auth_require) {
                ism_kafka_putInt2(buf, 36);  /* SaslAuthenticate */
                ism_kafka_putInt2(buf, 0);
                ism_kafka_putInt4(buf, 0x30002);  /* SaslAuthenticte response */
                ism_kafka_putString(buf, ism_common_getHostnameInfo(), -1);
                ism_kafka_putInt4(buf, 2 + userlen + passwordlen);
            }
            ism_kafka_putInt1(buf, 0);
            if (userlen)
                ism_json_putBytes(buf, mhub->username);
            ism_kafka_putInt1(buf, 0);
            if (passwordlen)
                ism_json_putBytes(buf, mhub->password);
            transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);
            TRACE(7, "Send SaslAuthenticate: connect=%u name=%s\n", transport->index, transport->name);

        }else if(mhub->sasl_mechanism == SASL_MECHANISM_SCRAM_SHA_512  || mhub->sasl_mechanism == SASL_MECHANISM_SCRAM_SHA_256) {
            //Sasl SCRAM
            if(!transport->pobj->sals_scram_context){
                  //New Context
                  transport->pobj->sals_scram_context = ism_sasl_scram_context_new(mhub->sasl_mechanism);
            }
            ism_sasl_scram_context * context = transport->pobj->sals_scram_context;
            if(context->state == SASL_SCRAM_STATE_NEW){

                char xbuf[256];
                concat_alloc_t first_msg_buf={xbuf, sizeof(xbuf)};

                ism_sasl_scram_build_client_first_message(transport->pobj->sals_scram_context, mhub->username, &first_msg_buf);

                if (!transport->pobj->auth_require) {
                    ism_kafka_putInt2(buf, 36);  /* API Key SaslAuthenticate */
                    ism_kafka_putInt2(buf, 0);   /* API version*/
                    ism_kafka_putInt4(buf, 0x30002);  /*CorrelationID: SaslAuthenticte response */
                    ism_kafka_putString(buf, ism_common_getHostnameInfo(), -1); /*Client ID*/
                    ism_kafka_putInt4(buf, first_msg_buf.used); //body length
                }
                char * first_message = alloca(first_msg_buf.used+1);
                memcpy(first_message, first_msg_buf.buf, first_msg_buf.used);
                first_message[first_msg_buf.used]=0;

                TRACE(9, "SaslAuthenticate for SASL SCRAM: connect=%u name=%s clientFirstMsg=%s firstMsgLen=%d\n", transport->index, transport->name, context->client_first_msg_bare, context->client_first_msg_bare_size);

                ism_common_allocBufferCopyLen(buf, first_msg_buf.buf, first_msg_buf.used );

                //Free temp buffer
                ism_common_freeAllocBuffer(&first_msg_buf);

                transport->pobj->sals_scram_context->state = SASL_SCRAM_STATE_CLIENT_FIRST_MESSAGE;
                TRACE(7, "Send SaslAuthenticate for SASL SCRAM: connect=%u name=%s sasl_state=%d\n", transport->index, transport->name, context->state);

                transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);

            }else if(context->state == SASL_SCRAM_STATE_CLIENT_FIRST_MESSAGE){

                ism_prop_t * props = context->server_response_props;

                const char * serverNonce = ism_common_getStringProperty(props, "r");
                const char * salt = ism_common_getStringProperty(props, "s");
                int iteration = ism_common_getIntProperty(props, "i", 0);
                TRACE(9, "SaslAuthenticate for SASL SCRAM: RECEVED SERVER FIRST MESSAGE: serverNonce=%s salt=%s iteration=%d server_first_msg=%s\n", serverNonce, salt,
                                iteration, context->server_first_msg);

                char client_final_message_buf[512];
                concat_alloc_t client_final_message_outbuf={client_final_message_buf, sizeof(client_final_message_buf)};
                ism_sasl_scram_build_client_final_message_buffer(context,mhub->password, context->server_response_props, &client_final_message_outbuf);

                if (!transport->pobj->auth_require) {
                   ism_kafka_putInt2(buf, 36);  /* API Key SaslAuthenticate */
                   ism_kafka_putInt2(buf, 0);   /* API version*/
                   ism_kafka_putInt4(buf, 0x30002);  /*CorrelationID: SaslAuthenticte response */
                   ism_kafka_putString(buf, ism_common_getHostnameInfo(), -1); /*Client ID*/
                   ism_kafka_putInt4(buf, client_final_message_outbuf.used); //body length
               }

                ism_common_allocBufferCopyLen(buf, client_final_message_outbuf.buf, client_final_message_outbuf.used );

                //Free temp buffer
                ism_common_freeAllocBuffer(&client_final_message_outbuf);

                context->state = SASL_SCRAM_STATE_CLIENT_FINAL_MESSAGE;
                TRACE(7, "Send SaslAuthenticate for SASL SCRAM: connect=%u name=%s sasl_state=%d \n", transport->index, transport->name,
                                                                          context->state);
                transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);
            }
        }
    } else {
        if (mhub->verSaslHandshake == 0xFF) {
            /* Legacy support, directly send user:pw */
            transport->pobj->auth_require = 1;
            userlen = mhub->username ? strlen(mhub->username) : 0;
            passwordlen = mhub->password ? strlen(mhub->password) : 0;
            ism_kafka_putInt1(buf, 0);
            if (userlen)
                ism_json_putBytes(buf, mhub->username);
            ism_kafka_putInt1(buf, 0);
            if (passwordlen)
                ism_json_putBytes(buf, mhub->password);
            transport->frame = ism_transport_frameNone;
            transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);
            TRACE(7, "Send SASL connect=%u name=%s\n", transport->index, transport->name);
        } else {
            ism_kafka_putInt2(buf, 17);    /* SaslHandshakeRequest */
            if (mhub->verSaslHandshake == 0) {
                ism_kafka_putInt2(buf, 0);
                transport->pobj->auth_require = 1;
                transport->frame = ism_transport_frameNone;
            } else {
                ism_kafka_putInt2(buf, 1);
                transport->pobj->auth_require = 0;
            }
            ism_kafka_putInt4(buf, 0x30000);  /* SaslHandshakeRequest */
            ism_kafka_putString(buf, transport->name, -1);
            if(mhub->sasl_mechanism == SASL_MECHANISM_PLAIN){
                ism_kafka_putString(buf, "PLAIN", -1);
            }else if(mhub->sasl_mechanism == SASL_MECHANISM_SCRAM_SHA_512){
                ism_kafka_putString(buf, "SCRAM-SHA-512", -1);
            }else if(mhub->sasl_mechanism == SASL_MECHANISM_SCRAM_SHA_256){
                ism_kafka_putString(buf, "SCRAM-SHA-256", -1);
            }
            transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);
            TRACE(7, "Send SaslHandshake: connect=%u name=%s\n", transport->index, transport->name);
        }
    }
}


/*
 * Outgoing connection complete
 */
static int ism_mhub_connected(ism_transport_t * transport, int rc) {
    ism_mhub_t * mhub = (ism_mhub_t *)transport->server;
    char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 4};
    transport->lastAccessTime = ism_common_readTSC();
    TRACE(7, "Event Streams connected (before auth): connect=%u name=%s rc=%d\n", transport->index, transport->name, rc);
    //Handle the case of msproxy shutdown
    if(g_shuttingDown){
    	 	 TRACE(5, "Msproxy is shutting down. Stop all further process for connected connections: connect=%u name=%s rc=%d\n", transport->index, transport->name, rc);
    	 	 return 1;
    }

    if (rc) {
        /* The connection failed so try the next broker after a delay */
        if (mhub->moreLogs && mhub->retry == 2) {
            LOG(WARN, Server, 990, "%s%s", "Unable to connect to an Event Streams broker: Name={0} Broker={1}",
                        transport->name, transport->client_host ? transport->client_host : transport->server_addr);
        } else {
            LOG(NOTICE, Server, 990, "%s%s", "Unable to connect to an Event Streams broker: Name={0} Broker={1}",
                        transport->name, transport->client_host ? transport->client_host : transport->server_addr);
        }
        ism_mhub_lock(mhub);
        if(!g_shuttingDown && mhub->enabled==1)
        		ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)mhubRetryConnect, mhub, retryDelay(mhub->retry++));
        ism_mhub_unlock(mhub);
        return 1;
    }
    transport->pobj->state = TCP_CONNECTED;
    mhub->retry = 0;
    transport->pobj->auth_require = 0;

    if (mhub->versionKnown == 0) {
        ism_kafka_putInt2(&buf, 18);    /* ApiVersionReqeuset */
        ism_kafka_putInt2(&buf, 0);     /* No headers */
        ism_kafka_putInt4(&buf, 0x30003);
        ism_kafka_putString(&buf, transport->name, -1);
        transport->send(transport, buf.buf+4, buf.used-4, 0, SFLAG_FRAMESPACE);
        return 0;
    } else {
        if (mhub->mhubSASL) {
            //DO: Determine wether SASL_PLAIN or SASL_SCRAME
            sendSASL(transport, &buf, 1);

        } else {
            moreConnected(transport);
        }
    }
    return 0;
}


/*
 * Connection closing
 */
static int ism_mhub_closing(ism_transport_t * transport, int rc, int clear, const char * reason) {
    int i;
    ism_mhub_t * mhub = (ism_mhub_t *)transport->server;
    int part_closed_count=0;
	TRACE(5, "Event Streams connection closing: connect=%u name=%s rc=%d reason=%s\n", transport->index, transport->name, rc, reason);
	if (mhub->moreLogs && transport->ready) {
        LOG(INFO, Server, 978, "%s%s%d%-s%s%lu%lu", "Event Streams connection closing: Name={0} ID={1} RC={2} Reason={3} Broker={4} WriteMsg={5} WriteBytes={6}",
            transport->name, mhub->name, rc, reason, transport->server_addr,
            transport->write_msg, transport->write_bytes);
	}


	ism_mhub_lock(mhub);

	if(transport->pobj->sals_scram_context!=NULL){
	    //Destroy SASL SCRAM context
        ism_sasl_scram_context_destroy(transport->pobj->sals_scram_context);
        transport->pobj->sals_scram_context = NULL;
	}

	//Need to handle the case of MSPROXY Shutdown.
	//Ensure no more connection can be created.
	if (g_shuttingDown || rc == ISMRC_ServerTerminating ){
		TRACE(5, "Msproxy is shutting down: connect=%u name=%s rc=%d\n", transport->index, transport->name, rc);
		if(mhub->state != MHS_Closing){
			mhub->prev_state = mhub->state;
			mhub->state = MHS_Closing;
			if (mhub->stateChanged) {
				mhub->stateChanged(mhub);       /* Notify of state change */
			}
			mhub->enabled=0; //Disable the mhub since proxy is shutting down
		}
	}


    if (transport->pobj->state == TCP_DISCONNECTED) {
        ism_mhub_unlock(mhub);
        return 0;
    }

    if (transport->pobj->kafka_type == KafkaMetadata) {
        mhub->expectingMetadata = 0;
        mhub->metadata=NULL;
    }

    /*
     * For a data connection, remove the transport object from the
     */
    if (transport->pobj->kafka_type == KafkaMessageHub && transport->pobj->topic) {
        int  topic_ix;
        mhub_topic_t * topic = NULL;
        for (i=0; i<mhub->topiccount; i++) {
            if (!strcmp(mhub->topics[i]->name, transport->pobj->topic)) {
                topic = mhub->topics[i];
                topic_ix = i;
            }
        }

        ism_mhub_unlock(mhub);

        if (topic && topic->partcount > transport->pobj->partID) {
            mhub_part_t * part = topic->partitions+transport->pobj->partID;

            if (part->transport == transport) {
					//Flush messages if this is msproxy shutdown
					if (rc== ISMRC_ServerTerminating &&  transport->pobj && transport->pobj->state==TCP_CONNECTED) {
						TRACE(7, "Run job at close to produce: mhub=%s topic=%s partition=%u\n",
										mhub->id, topic->name, transport->pobj->partID);
						/* Run this here.  We are safe in the closing code */
						mhubProduceJob(transport, (void *)mhub, (((uint64_t)topic_ix+0x10000)<<32) + transport->pobj->partID);
					}
            } else {
                TRACE(3, "The mhub partition object is not correct: name=%s mhub=%s topic=%s part=%u\n",
                        transport->name, mhub->id, transport->pobj->topic, KafkaMessageHub);
            }

            pthread_mutex_lock(&part->lock);
            part->open = 0;
            part->valid = 0;
            part->transport = NULL;
            pthread_mutex_unlock(&part->lock);
            part_closed_count++;

        } else {
            /* TODO: It is possible when we are changing part count that this happens */
        }

        ism_mhub_lock(mhub);

        mhub->notopen += part_closed_count;
        mhub->open -= part_closed_count;
    }

    if (!g_shuttingDown){
		mhub->prev_state = mhub->state;
		mhub->state = MHS_Opening;
		if (mhub->stateChanged) {
			mhub->stateChanged(mhub);       /* Notify of state change */
		}
    }

    ism_mhub_unlock(mhub);

    transport->pobj->state = TCP_DISCONNECTED;
    transport->ready = 0;

    transport->closed(transport);
    return 0;
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
 * Timer method to check the start of MHub entries
 *
 * This method is used after configuration and to do connection retries.
 * There is a timer job for each tenant with MessageHub configured.
 *
 * When this is called other than by the timer, the key and now are 0.
 *
 * When an MHub object is first configured, it is in Config state and has all of the need bits set.
 * As soon as we are ready to create outgoing connections, we change the state to Wait and try to
 * form a broker connection on which we will get the metadata.  If this connection fails we try other
 * brokers.  If we determine a config change is needed for the broker connection to succeed we set the
 * state to Failed, otherwise we keep the state as Wait until we get a broker connection and receive
 * the metadata.
 *
 * When we have the metadata we update all of the topic and partition objects and change the state
 * to Opening.  We then attempt to open a connection for all partitions.  When all of the partitions have
 * valid data connections, we change the state to Active.  If we need to close some or all of the data
 * connections, we change the state to Closing.
 *
 * Any actual work is done in another timer job.
 */
static int mhubStateCheck(ism_timer_t key, ism_time_t now, void * userdata) {
    ism_tenant_t * tenant = (ism_tenant_t *) userdata;
    ism_mhub_t * mhub;
    int  i;
    int  j;
    int  notopen = 0;
    int  open = 0;
    int  needmetadata = 0;

    if (!g_mhubStarted)
        return 1;

    ism_tenant_lock();
    mhub = tenant->mhublist;
    while (mhub) {
        ism_mhub_lock(mhub);

        /*
         * Handle Shutdown of Proxy
         */
        if (g_shuttingDown) {
			//msproxy is shutting down. Need to put mhub into closing state
			if(mhub->state != MHS_Closing){
				mhub->prev_state = mhub->state;
				mhub->state = MHS_Closing;
				if (mhub->stateChanged) {
					mhub->stateChanged(mhub);       /* Notify of state change */
				}
				mhub->enabled = 0;  //Disable the mhub
			}
		}
		if (mhub->enabled == 1 && mhub->broker_count > 0) {
			if (mhub->need) {
			    if (mhub->need & UPDATE_Secure) {
			        if (mhub->tlsCTX)
			            mhub->tlsCTX = NULL;
			        mhub->tlsCTX = ism_transport_clientTlsContext(mhub->name,
                        ism_common_enumName(enum_methods, mhub->useTLS),
                        mhub->ciphers ? mhub->ciphers : defaultCiphers(mhub->useTLS));
			        if (mhub->trustCerts)
			            ism_common_addTrustCerts(mhub->tlsCTX, mhub->name, mhub->trustCerts);
			        mhub->need &= ~UPDATE_Secure;
			    }
				/* Handle changes after configuration */
				if (mhub->state != MHS_Closing && mhub->state != MHS_Config && mhub->state != MHS_Failed) {
					/* For now, just close everything down and restart for any config change */
					ism_mhub_unlock(mhub);
					closeConnections(mhub);
					ism_mhub_lock(mhub);
				} else {
					mhub->need = 0;
					mhub->prev_state = mhub->state;
					mhub->state = MHS_Wait;
					if (mhub->stateChanged) {
						mhub->stateChanged(mhub);       /* Notify of state change */
					}
					/* Open a broker connection. We will eventually get into Opening state when the metadata completes */
					ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)mhubRetryConnect, mhub, retryDelay(0));
				}

			} else {
				/* Handle retries.  If any partition */
				if (mhub->state == MHS_Opening) {
					/* Look thru all topics.  If the metadata is valid  */
					for (i=0; i<mhub->topiccount; i++) {
						mhub_topic_t * topic = mhub->topics[i];
						if (topic->valid == 1) {  /* We have metadata with rc=0 */
							 for (j=0; j<topic->partcount; j++) {
								 mhub_part_t * part = topic->partitions+j;
								 pthread_mutex_lock(&part->lock);
								 if (part->valid==1) {
									 if (part->open) {
										 open++;
									 } else {
										 /* retry as required */
											/*Partition is not open try to open the connection.*/
											if(part->transport!=NULL && part->transport->pobj && part->transport->pobj->state !=TCP_CON_IN_PROCESS
													&& part->transport->pobj->state  != TCP_CONNECTED){
												ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)mhubDataRetryConnect, part,  retryDelay(0));
											}else if(part->transport == NULL){
												//Need to get metadata
												needmetadata++;
											}
											notopen++;

									 }
								 } else {
									 needmetadata++;
									 notopen++;
								 }
								 pthread_mutex_unlock(&part->lock);
							 }
						} else {
							notopen += topic->partcount;   /* No metadata yet */
						}
					}
					mhub->notopen = notopen;
					mhub->open = open;
					if (needmetadata) {
						if (!mhub->expectingMetadata) {
								//Metadata is broken, need new transport.
							mhub->prev_state = mhub->state;
							mhub->state = MHS_Opening;
							if (mhub->stateChanged) {
								mhub->stateChanged(mhub);       /* Notify of state change */
							}
							//Make Sure the transport for metadata is still valid.
							if (mhub->metadata && mhub->metadata->pobj && mhub->metadata->pobj->state==TCP_CONNECTED) {
								//if still in SASL Auth, skip the request.
								if(!mhub->metadata->pobj->auth_require){
									mhub->expectingMetadata = 1;
									mhubMetadataRequest(mhub, mhub->metadata);
								}else{
									TRACE(5,"Metadata process is in Authentication. Skip metadata request. Name=%s Broker=%s:%u\n",
											mhub->metadata->name, mhub->metadata->server_addr, mhub->metadata->serverport );
								}
							} else {
								ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)mhubRetryConnect, mhub, retryDelay(0));
							}
						}else{
							//If MHUB expect metadata and transport is not there, create it and make the request
							if(!mhub->metadata || !mhub->metadata->pobj){
								ism_common_setTimerOnce(ISM_TIMER_LOW, (ism_attime_t)mhubRetryConnect, mhub, retryDelay(0));
							}
						}
					} else if (notopen == 0) {
						mhub->prev_state = mhub->state;
						mhub->state = MHS_Active;
						if (mhub->stateChanged) {
							mhub->stateChanged(mhub);       /* Notify of state change */
						}
					}
				}
			}
		}

        ism_mhub_unlock(mhub);
        mhub = mhub->next;

    }
    ism_tenant_unlock();
    return 1;
}



/*
 * Free an event
 *
 * As we now allocate the data as part of the event itself, we only need to
 * free the message.
 */
static void freeKafkaEvent(kafka_produce_msg_t * msg){
	if (msg) {
		ism_common_free(ism_memory_proxy_eventstreams,msg);
	}
}

/*
 * Put back the messages to the head of the pending queue
 *
 */
static void ism_mhub_enqueueMsgsPendingHead(mhub_part_t * mhub_part, kafka_produce_msg_t * msgs) {
	int count=0;
	if (!msgs) {
		return;
	}
	pthread_mutex_lock(&mhub_part->lock);
	 //Take the first time of first message
	mhub_part->kafka_msg_first_time = msgs->time;
	kafka_produce_msg_t * first_msgs = msgs;
	 //Go to the last of the message in the left-over list
	count++;
	while(msgs!=NULL && msgs->next!=NULL){
	    msgs = msgs->next;
		count++;
	}

	 //Set Last if NULL
	if (!mhub_part->kafka_msg_last) {
	    mhub_part->kafka_msg_last = msgs;
	}

	 //Link to the current head at the last of the left-over
	msgs->next =  mhub_part->kafka_msg_first;
	mhub_part->kafka_msg_first  = first_msgs;
	mhub_part->kafka_msg_count += count;
	pthread_mutex_unlock(&mhub_part->lock);
}

/*
 * Add msg to Ackwait list
 */
static int ism_mhub_addMsgToAckWaitList(mhub_part_t * mhub_part, kafka_produce_msg_t * msg) {
	if (mhub_part!=NULL && msg!=NULL) {
		pthread_mutex_lock(&mhub_part->lock);
		msg->next = NULL;

		if ( mhub_part->kafka_ackwait_msg_last)
		        mhub_part->kafka_ackwait_msg_last->next = msg;
		mhub_part->kafka_ackwait_msg_last = msg;

		if (!mhub_part->kafka_ackwait_msg_first) {
			mhub_part->kafka_ackwait_msg_first = msg;
		}

		pthread_mutex_unlock(&mhub_part->lock);
	}
	return 0;
}

/*
 * Add a kafka record batch which is the message data for v2
 * @param connRecord The kafka connection record
 * @param buf        The output buffer
 * @param msgs       The linked list of messages to write
 * @param msgcnt     (output) the count of messages produced
 */
int ism_mhub_addEventRecordBatch(ism_transport_t * transport, ism_mhub_t * mhub, mhub_part_t * mhub_part,
        concat_alloc_t * buf, kafka_produce_msg_t * msgs, int * msgcnt) {
    int  startused = buf->used;
    int  batchsize;
    int  crcpos;
    int  recordcount;
    int  count;
    int  tsloc;
    int  currBufSize=0;
    int  totalMsgSize=0;
    uint64_t mintime = 0;
    uint64_t maxtime = 0;
    kafka_produce_msg_t * msg;
    char xbuf[16*1024];
    /* Message buffer to handle all of the varint lengths */
    concat_alloc_t mbuf = {xbuf, sizeof xbuf};

    //ism_kafka_putInt8(buf, transport->pobj->lastOffset+1);        /* baseOffset */
    ism_kafka_putInt8(buf, 0);        /* baseOffset */
    batchsize = ism_protocol_reserveBuffer(buf, 4);
    ism_kafka_putInt4(buf, 0);        /* partitionLeaderEpoch: not set on produce */
    ism_kafka_putInt1(buf, mhub->messageVersion);        /* magic */
    crcpos = ism_protocol_reserveBuffer(buf, 4);
    ism_kafka_putInt2(buf, 0);        /* attributes: no compress, client timestamp, no transaction  */
    tsloc = ism_protocol_reserveBuffer(buf, 20);
    ism_kafka_putInt8(buf, -1L);       /* producerId: must set to use exactly once */
    ism_kafka_putInt2(buf, -1);        /* producerEpoch: must set to use exactly once */
    ism_kafka_putInt4(buf, -1);        /* baseSequence: must be set to use exactly once  */
    recordcount = ism_protocol_reserveBuffer(buf, 4);
    msg = msgs;
    count = 0;
    while (msg) {
        uint64_t mtime = ism_common_convertTimeToJTime(msg->time);
        if (!count) {
            mintime = mtime;
        } else {
            if (mtime < mintime)
                mtime = mintime;
        }
        if (mtime > maxtime)
            maxtime = mtime;

        mbuf.buf[0] = 0;                                 /* attributes */
        mbuf.used = 1;
        ism_kafka_putVarInt(&mbuf, mtime-mintime);       /* timestamDelta: */
        ism_kafka_putVarInt(&mbuf, count);                   /* offsetDelta */
        count++;
        ism_kafka_putVarInt(&mbuf, msg->key_len);        /* keyLength */
        ism_common_allocBufferCopyLen(&mbuf, msg->key, msg->key_len);    /* Key */
        ism_kafka_putVarInt(&mbuf, msg->buflen);
        ism_common_allocBufferCopyLen(&mbuf, msg->buf, msg->buflen);     /* Body */
        ism_kafka_putVarInt(&mbuf, msg->hdrcount);      /* header count */
        if (msg->hdr_len)
            ism_common_allocBufferCopyLen(&mbuf, (char *)msg->hdr, msg->hdr_len);
        ism_kafka_putVarInt(buf, mbuf.used);
        ism_common_allocBufferCopyLen(buf, mbuf.buf, mbuf.used);

        kafka_produce_msg_t * xmsg = msg;
        msg = msg->next;
        if (!mhub->mhubACK) {
            freeKafkaEvent(xmsg);
        } else {
            ism_mhub_addMsgToAckWaitList(mhub_part, xmsg);
        }

        //Check if more bytes allow. If not, put back the head of the queue.
        if(msg!=NULL){
        		currBufSize = buf->used-batchsize-4;
			totalMsgSize = currBufSize + msg->buflen + msg->key_len + msg->hdr_len + KAFKA_LOG_MSG_OVERHEAD;
			if(totalMsgSize >= mhub->maxBatchBytes){
					//put to the head of the queue
					ism_mhub_enqueueMsgsPendingHead(mhub_part, msg);
					break;
			}

        }
    }
    ism_kafka_putInt4At(buf, tsloc, count-1);         /* LastOffsetDelta */
    transport->pobj->lastOffset += count;
    ism_kafka_putInt8At(buf, tsloc+4, mintime);
    ism_kafka_putInt8At(buf, tsloc+12, maxtime);
    ism_kafka_putInt4At(buf, recordcount, count);
    if (msgcnt)
        *msgcnt = count;
    uint32_t crc = ism_common_crc32c(0, buf->buf+crcpos+4, buf->used-crcpos-4);
    ism_kafka_putInt4At(buf, crcpos, crc);                   /* Fill in CRC */
    ism_kafka_putInt4At(buf, batchsize, buf->used-batchsize-4);  /* Fill in message size */
    if (mbuf.inheap)
        ism_common_freeAllocBuffer(&mbuf);
    return buf->used - startused;
}


/*
 * Run a timer to make sure im messaging records per partition are sent even if there are no more produced.
 */
int mhubPartitionProduceTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    ism_transport_t * transport;
    ism_mhub_t * mhub = (ism_mhub_t *)userdata;
	// TRACE(9, "mhubPartitionProduceTimer:  mhub_name=%s topic_count=%d\n", mhub->name, mhub->topiccount);

    if (!mhub->disabled) {
        /* For Each Topic, Go thru partitions for publishing */
        for (int tcount = 0; tcount < mhub->topiccount; tcount++) {
            mhub_topic_t	* mhub_topic = mhub->topics[tcount];
            for (int pcount = 0; pcount < mhub_topic->partcount; pcount++) {
                mhub_part_t * mhub_part = (mhub_part_t * )&mhub_topic->partitions[pcount];

                pthread_mutex_lock(&mhub_part->lock);
                if (mhub_part->valid) {
                    transport = mhub_part->transport;
                    if (transport!=NULL && transport->ready == 1 && transport->pobj->kafka_type == KafkaMessageHub && transport->pobj->state == TCP_CONNECTED) {
                        int need = needMHubBatch(mhub, mhub_part, ism_common_currentTimeNanos());
                        if (need > 0) {
                            /* Move to the IOP of the outbound transport */
                            TRACE(9, "Schedule job from timer to produce: mhub=%s topic=%s partition=%u need=%u\n",
                                    mhub->id, mhub_topic->name, pcount, need);
                            ism_transport_submitAsyncJobRequest(transport, mhubProduceJob, (void *)mhub, (((uint64_t)tcount)<<32) + pcount);
                        }
                    } else{
                    		TRACE(5, "Data transport is not ready to produce: mhub=%s topic=%s partition=%u\n",
                    	                                    mhub->id, mhub_topic->name, pcount);

                    }
                }
                pthread_mutex_unlock(&mhub_part->lock);
            }
        }
    }
    return 1;
}


/*
 * Check if we need to produce a batch
 *
 * If we need to send a batch, we schedule the work in the IOP of the transport object of the outgoing transport
 * This is called holding the mhub_part lock
 */
static int needMHubBatch(ism_mhub_t * mhub, mhub_part_t * mhub_part, ism_time_t now) {

	/* Check if we need to re-produce a batch */
	if (mhub_part->needreproduce) {
		return 3;
	}
    /* If we are waiting for ACKs, do not produce */
    if (mhub->mhubACK && mhub_part->hasackwait)
        return -1;
    /* If a batch is full complete produce */
    if (mhub_part->kafka_msg_first && (mhub_part->kafka_msg_count > mhub->maxBatchMsgs))
        return 1;
    /* If time is exceeded produce */
    if (mhub_part->kafka_msg_first && (now - mhub_part->kafka_msg_first_time) > (mhub->maxBatchTimeMS*MILLION) )
        return 2;
    /* Otherwise do not produce */
    return 0;
}

/*
 * Check to see if a im messaging batch is full, and return the batch if it is.
 * This is called both when we send a message and when start a kafka connection.
 */
static kafka_produce_msg_t *  checkMHubEventBatch(ism_mhub_t * mhub, mhub_part_t * mhub_part, ism_time_t now, int closing) {
	kafka_produce_msg_t * doProduce = NULL;
	kafka_produce_msg_t * produce_msg_first = NULL;

	/*If need to reproduce, send the wait queue for reproduce if any*/
	if (mhub_part->needreproduce) {
		mhub_part->needreproduce=0;
		doProduce = mhub_part->kafka_ackwait_msg_first;
		if(!closing && mhub->mhubACK && doProduce!=NULL){
			mhub_part->hasackwait = 1;
		}
		mhub_part->kafka_ackwait_msg_first=NULL;
		mhub_part->kafka_ackwait_msg_last=NULL;
		return doProduce;
	}

	if ( closing || (mhub_part->kafka_msg_first!=NULL && (mhub_part->kafka_msg_count > mhub->maxBatchMsgs ||
        (now - mhub_part->kafka_msg_first_time) > (mhub->maxBatchTimeMS*MILLION) ) )) {
	    int count = 0;

        /*
         * If there is a message list that is in waiting for ACK state. Use that first
         * There is ackwait msgs. No need to produce further until he ackwait msgs are cleared
         */
        if (!closing && mhub->mhubACK && mhub_part->hasackwait) {
            doProduce = NULL;
        } else {
            /* Need to support the case the queue had millions of messages. batch every batch size allow */
            if (mhub_part->kafka_msg_count > mhub->maxBatchMsgs &&  mhub->maxBatchMsgs > 0) {
                kafka_produce_msg_t * msg = produce_msg_first = mhub_part->kafka_msg_first;
                kafka_produce_msg_t * msg_prev = NULL;

                int batch_size = mhub->maxBatchMsgs;
                /* For RECOVERY case, need to put more messages into the packat */
                if (mhub_part->kafka_msg_count > (mhub->maxBatchMsgs * 10)) {
                		batch_size = mhub->maxBatchMsgs * 4;
                }
                while (count <  batch_size ) {
                    msg_prev  = msg;
                    msg = msg->next;
                    count++;
                }
                if (msg_prev!=NULL)
                    msg_prev->next = NULL;
                mhub_part->kafka_msg_first  = msg;
                mhub_part->kafka_msg_count -= count;
                if (msg)
                    mhub_part->kafka_msg_first_time = msg->time;
            } else {
				produce_msg_first = mhub_part->kafka_msg_first;
				mhub_part->kafka_msg_first = NULL;
				mhub_part->kafka_msg_last = NULL;
				mhub_part->kafka_msg_count =0;
			}
            	doProduce = produce_msg_first;
            	if(!closing && mhub->mhubACK){
            		mhub_part->hasackwait = 1;
            	}


        }
    }

    return doProduce;
}

/*
 * Add a message to the produce message set.
 * The saved message is
 * @return message size
 */
int ism_mhub_addKafkaBufferMessage(ism_mhub_t * mhub, concat_alloc_t * buf, kafka_produce_msg_t * msg) {
    int  msgsize=0;
    int  crcpos;
    int  bodylen;
    int  startused = buf->used;
    ism_kafka_putInt8(buf, 0);        /* offset, not set on produce */
    msgsize = ism_protocol_reserveBuffer(buf, 8);
    crcpos = msgsize+4;
    ism_kafka_putInt1(buf, mhub->messageVersion);        /* magic */
    ism_kafka_putInt1(buf, 0);        /* attr */
    if (mhub->produceVersion > 0) {
		ism_kafka_putInt8(buf, ism_common_convertTimeToJTime(msg->time));        /* timestamp in milliseconds*/
    }
    ism_kafka_putBytes(buf, msg->key, msg->key_len); /* Event Key */
    bodylen = ism_protocol_reserveBuffer(buf, 4);

    ism_common_allocBufferCopyLen(buf, msg->buf, msg->buflen);

    ism_kafka_putInt4At(buf, bodylen, buf->used-bodylen-4);  /* Fill in body length */
    uint32_t crc = ism_common_crc(0, buf->buf+crcpos+4, buf->used-crcpos-4);
    ism_kafka_putInt4At(buf, crcpos, crc);                   /* Fill in CRC */
    ism_kafka_putInt4At(buf, msgsize, buf->used-msgsize-4);  /* Fill in message size */
    return buf->used - startused;
}


/*
 * Send a set of messages to kafka
 */
int ism_mhub_message_produce(ism_transport_t * transport, ism_mhub_t * mhub, mhub_part_t * mhub_part,
        kafka_produce_msg_t * msgs, int * producedMsgsCount, int isResend) {
    int msgcnt = 0;
    int msgsize = 0;
    int rc=0;
    uint64_t totalSent=0;
	uint64_t totalBytesSent = 0;
	uint64_t totalBatchProduceCount= 0;
	ism_time_t firsMsgTimeInBatch = 0;
	uint64_t totalMsgInBatchWaitTimeMS = 0;
	int totalMsgSize=0;

    char xbuf [16*1024];   /* room for 100 metering messages */
    concat_alloc_t cbuf = {xbuf, sizeof xbuf, 4};
    concat_alloc_t * buf = &cbuf;

    ism_kafka_putInt2(buf, ProduceRequest);         /* Api key */
    ism_kafka_putInt2(buf, mhub->produceVersion);  /* produce version */
    ism_kafka_putInt4(buf, 1);                      /* CorrID   */
    ism_kafka_putString(buf, transport->name, -1);

    /*
     * Set Produce Ack.
     * 0 = no ack
     * 1 = ack when the data is written to the leader log
     * -1 = ack when the data is written to all in sync replicas
     */
    ism_kafka_putInt2(buf, mhub->mhubACK);

    ism_kafka_putInt4(buf, 5000);     /* timeout*/

    ism_kafka_putInt4(buf, 1);        /* topic count*/
    ism_kafka_putString(buf, mhub_part->topic->name, -1);
    ism_kafka_putInt4(buf, 1);        /* partition count */
    ism_kafka_putInt4(buf, transport->pobj->partID);   /* partition*/
    int setsize = ism_protocol_reserveBuffer(buf, 4);

    //Get the time of the first message for stat
    if(msgs!=NULL){
    	firsMsgTimeInBatch = msgs->time;
    }

    /*
     * Kafka 0.11 introduces a new message format which is radically different from
     * previous versions.  This uses the record batch and moves some of the items
     * which were in each individual message into the record batch object.
     * This also defines headers which is where to put properties.
     */
    if (mhub->messageVersion >= 2) {
        msgsize += ism_mhub_addEventRecordBatch(transport, mhub, mhub_part, buf, msgs, &msgcnt);
    } else {
        /* Implement message versions 0 and 1 */
        while (msgs) {
            kafka_produce_msg_t * msg;
            msg = msgs;
            msgsize += ism_mhub_addKafkaBufferMessage(mhub, buf, msg);
            msgs = msgs->next;
            msgcnt++;
            //If Wait ACk is enabled. Do not clean the message
            if (!mhub->mhubACK)
                freeKafkaEvent(msg);
            else{
               ism_mhub_addMsgToAckWaitList(mhub_part, msg);
            }

            //Check if more bytes allow. If not, put back the head of the queue.
			if (msgs != NULL) {
				totalMsgSize = msgsize + msgs->buflen + msgs->key_len + msgs->hdr_len + KAFKA_LOG_MSG_OVERHEAD;
				if(totalMsgSize >= mhub->maxBatchBytes){
						//put to the head of the queue
						ism_mhub_enqueueMsgsPendingHead(mhub_part, msgs);
						break;
				}

			}
        }
    }

    //Record the BatchWait of the batch
    if (!isResend && firsMsgTimeInBatch > 0) {
    	uint64_t diff = (uint64_t) (ism_common_currentTimeNanos() - firsMsgTimeInBatch) / MILLION; //milliseconds

    	pthread_spin_lock(&g_mhubStatLock);
    	totalMsgInBatchWaitTimeMS = mhubMessagingStats.kafkaMsgInBatchWaitTimeMS += diff;
    	if (diff > mhubMessagingStats.kafkaMsgInBatchMaxWaitTimeMS){
		    mhubMessagingStats.kafkaMsgInBatchMaxWaitTimeMS= diff;
	    }
    	pthread_spin_unlock(&g_mhubStatLock);
    }

    int sendBytesSize = buf->used-setsize-4;
    ism_kafka_putInt4At(buf, setsize, sendBytesSize);
    rc = transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);
    TRACE(9, "Produce Event Streams messages: connect=%u name=%s messages=%u bytes=%u\n",
            transport->index, transport->name, msgcnt, buf->used-4);
    if (producedMsgsCount!=NULL) {
    		*producedMsgsCount = msgcnt;
    }


    mhub_part->lastProduceTime=ism_common_readTSC();


    /*
	 * Update ConnectionRecord Stats
	 */
	if (isResend) {
		/* Update Overall Kafka Stats */
		totalSent = __sync_add_and_fetch(&mhubMessagingStats.kakfaC2PMsgsTotalReSent, msgcnt);
		totalBytesSent = __sync_add_and_fetch(&mhubMessagingStats.kakfaC2PBytesTotalReSent, buf->used);
		totalBatchProduceCount = __sync_add_and_fetch(&mhubMessagingStats.kakfaTotalBatchReProduceCount, 1);
		TRACE(7, "ReProduce mhub messages connect=%u partID=%d nodeID=%d topic=%s sentCount=%u  totalReSent=%llu totalReBytesSent=%llu totalReBatchProduceCount=%llu produceRC=%d\n",
					transport->index, transport->pobj->partID, transport->pobj->nodeID, mhub_part->topic->name, msgcnt,
					(ULL)totalSent, (ULL)totalBytesSent , (ULL)totalBatchProduceCount, rc);
	} else {
		/* Update Overall Kafka Stats */
		pthread_spin_lock(&g_mhubStatLock);
		totalSent = mhubMessagingStats.kakfaC2PMsgsTotalSent += msgcnt;
		totalBytesSent = mhubMessagingStats.kakfaC2PBytesTotalSent += sendBytesSize;
		totalBatchProduceCount = mhubMessagingStats.kakfaTotalBatchProduceCount++;
		pthread_spin_unlock(&g_mhubStatLock);

		TRACE(7, "Produce mhub messages connect=%u partID=%d nodeID=%d topic=%s sentCount=%u totalSent=%llu totalBytesSent=%llu totalBatchProduceCount=%llu firstMsgBatchWaitTime=%llu produceRC=%d\n",
			transport->index, transport->pobj->partID, transport->pobj->nodeID, mhub_part->topic->name, msgcnt,
			(ULL)totalSent, (ULL)totalBytesSent , (ULL)totalBatchProduceCount, (ULL)totalMsgInBatchWaitTimeMS, rc);
	}

    if (buf->inheap)
        ism_common_freeAllocBuffer(buf);

    return rc;
}

/*
 * Produce the kafka batch, having moved it to the outgoing transport IOP thread
 */
static int  mhubProduceJob(ism_transport_t * transport, void * param1, uint64_t param2) {
    kafka_produce_msg_t * doProduce;
    ism_mhub_t * mhub = (ism_mhub_t *)param1;
    uint32_t topic_ix = (uint32_t)(param2>>32);
    uint32_t partition = (uint32_t)param2;
    int closing = 0;
    int producedMsgsCount = 0;
    int isResend=0;

    if (topic_ix & 0x10000)
        closing = 1;
    topic_ix &= 0xffff;

    /* Get topic and partition */
    mhub_topic_t * mhub_topic = mhub->topics[topic_ix];
    int which = mhub_topic->partcount > 1 ? partition % mhub_topic->partcount : 0;
    mhub_part_t * mhub_part = (mhub_part_t *)&mhub_topic->partitions[which];

    pthread_mutex_lock(&mhub_part->lock);
    isResend = mhub_part->needreproduce;
    doProduce = checkMHubEventBatch(mhub, mhub_part, ism_common_currentTimeNanos(), closing);
    pthread_mutex_unlock(&mhub_part->lock);

    /* If we have a batch to send, do the kafka produce now */
    if (doProduce) {
        ism_mhub_message_produce(transport, mhub, mhub_part, doProduce, &producedMsgsCount, isResend);
        if (!mhub->mhubACK) {
				pthread_spin_lock(&g_mhubStatLock);
				mhubMessagingStats.kakfaTotalPendingMsgsCount -= producedMsgsCount;
				pthread_spin_unlock(&g_mhubStatLock);
				transport->write_msg += producedMsgsCount;
        }
    }
    if (closing) {
        TRACE(3, "Flush messages at closing: name=%s mhub=%s msgcount=%d\n",
               transport->name, mhub->id, producedMsgsCount);
    }
    return 0;
}


/*
 * Publish a kafka event message
 *
 * @param mhub   The MessageHub object
 * @param pmsg   The parsed MQTT message
 * @param clientID  The clientID of the source of the message
 * @param topic_index  Which topic in the mhub to use
 * @param partition   Which partition to use.
 *
 * The partition specified can be any value, and is mod by the partition count to construct
 * the actual partition. Note that this can change between this publish and when the
 * message is actually produced into MessageHub.
 */
int ism_mhub_publishEvent(ism_mhub_t * mhub, mqtt_pmsg_t * pmsg, const char * clientID, int topic_index, int partition) {
	int  rc = 1;
	kafka_produce_msg_t * event;
	int eventlen;

	/*
     * Get topic and partition
     */
    mhub_topic_t * mhub_topic = mhub->topics[topic_index];
    if (mhub_topic && mhub_topic->valid == 2) {
        TRACE(8, "Event Streams message not published because topic does not exist: mhub=%s topic=%s\n", mhub->id, mhub_topic->name);
        return 2;
    }


    /* Update Total Kafka (for all topic) Stats */
	pthread_spin_lock(&g_mhubStatLock);
	mhubMessagingStats.kakfaC2PMsgsTotalReceived++;
	mhubMessagingStats.kakfaC2PBytesTotalReceived += pmsg->buflen;
	mhubMessagingStats.kakfaTotalPendingMsgsCount++;
	pthread_spin_unlock(&g_mhubStatLock);

	TRACE(9, "Publish Event Streams message: mhub=%s clientID=%s fromTopic=%s toTopic=%s\n",
		        mhub->id, clientID, pmsg->topic, mhub_topic->name);

	/*  Create kafka message */
    eventlen = pmsg->key_len + pmsg->payload_len + pmsg->hdr_len;
	event = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,2),sizeof(kafka_produce_msg_t) + eventlen);
	memset(event, 0, sizeof(kafka_produce_msg_t));

	event->buf = (char *)(event+1);
	if (pmsg->payload && pmsg->payload_len)
	    memcpy(event->buf, pmsg->payload, pmsg->payload_len);
    event->buflen = pmsg->payload_len;

    event->key = event->buf + pmsg->payload_len;
    if (pmsg->key_len)
        memcpy(event->key, pmsg->key, pmsg->key_len);
    event->key_len = pmsg->key_len;

    event->hdr = (uint8_t *)(event->key + event->key_len);
    if (pmsg->hdr_len)
        memcpy(event->hdr, pmsg->headers, pmsg->hdr_len);
    event->hdr_len = pmsg->hdr_len;
    event->hdrcount = pmsg->hdr_count;
    
    if (pmsg->waitValue) {                /* Send an ACK on response */
        event->waitID = pmsg->waitID;
        event->waitValue = pmsg->waitValue;
        pmsg->waitValue = 0;
    }

    /* Set the message timestamp */
    event->time = ism_common_currentTimeNanos();

    int which = mhub_topic->partcount > 1 ? partition % mhub_topic->partcount : 0;
    mhub_part_t * mhub_part = (mhub_part_t *)&mhub_topic->partitions[which];

    pthread_mutex_lock(&mhub_part->lock);

    ism_transport_t *  transport = mhub_part->transport;
    event->dcrc = partition;

    /* Link in this event */
    if ( mhub_part->kafka_msg_last)
        mhub_part->kafka_msg_last->next = event;
    mhub_part->kafka_msg_last = event;

    if (!mhub_part->kafka_msg_first) {
        mhub_part->kafka_msg_first = event;
        mhub_part->kafka_msg_first_time = event->time;
        if (!mhub->mhubACK)
        	mhub_part->kafka_msg_count = 1;
        else
        	mhub_part->kafka_msg_count++;

    } else {
        mhub_part->kafka_msg_count++;
    }

    /* If the kafka publishing connection is open, continue*/
    if (transport && transport->pobj && transport->pobj->state==TCP_CONNECTED) {
        rc = 0;
    } else {
        //The Connection Record is not in the open state. or TCP connection is not in Open State
        //Still keep the message in the Pending Q
        //Will move it once the connection established.
    		if(transport!=NULL){
    			TRACE(5, "publishEvent: Partition Connection is not open. which=%d transport.index=%d transport.state=%d transport.ready=%d pending_msg_count=%d\n", which, transport->index, transport->state, transport->ready, mhub_part->kafka_msg_count);
    		}else{
    			TRACE(5, "publishEvent: Partition Connection is not open. which=%d pending_msg_count=%d\n", which, mhub_part->kafka_msg_count);
    		}
        rc = 1;
    }

    /* If connection is open check if batch is full */
    if (rc == 0) {
        int need = needMHubBatch(mhub, mhub_part, event->time);
        if (need > 0) {
            /* Move to the IOP of the outbound transport */
            TRACE(8, "Schedule job to produce: mhub=%s topic=%s partition=%u\n", mhub->id, mhub_topic->name, which);
            ism_transport_submitAsyncJobRequest(transport, mhubProduceJob, (void *)mhub, (((uint64_t)topic_index)<<32) + which);
        }
    }

	pthread_mutex_unlock(&mhub_part->lock);
	return rc;
}

/*
 * Print support for mhub
 */
void ism_mhub_printMhub(struct ism_mhub_t * mhub, const char * xmhub) {
    if (!xmhub || !*xmhub)
        xmhub = "*";
    while (mhub) {
        if (ism_common_match(mhub->name, xmhub)) {
            char * buf = ism_mhub_toString(mhub);
            printf("%s\n", buf);
            ism_common_free(ism_memory_proxy_eventstreams,buf);
        }
        mhub = mhub->next;
    }
}

/*
 * Print out some global MHub stats
 */
void ism_mhub_printStats(void) {
    px_kafka_messaging_stat_t * s = &mhubMessagingStats;
    printf("ReadMsg=%lu ReadBytes=%lu WriteMsg=%lu WriteBytes=%lu\n",
                s->kakfaC2PMsgsTotalReceived, s->kakfaC2PBytesTotalReceived,
                s->kakfaC2PMsgsTotalSent, s->kakfaC2PBytesTotalSent);
    double latency = 0.0;
    if (s->kakfaTotalBatchProduceAckReceivedCount)
        latency = (double)s->kafkaMsgInBatchWaitTimeMS/s->kakfaTotalBatchProduceAckReceivedCount;
    printf("ProduceRequest=%lu ProduceResponce=%lu Pending=%lu Latency=%0.3g ms\n",
        s->kakfaTotalBatchProduceCount, s->kakfaTotalBatchProduceAckReceivedCount,
        s->kakfaTotalPendingMsgsCount, latency);
    if (s->kakfaTotalBatchProduceAckErrorCount)
        printf("ProduceResponseError=%lu\n", s->kakfaTotalBatchProduceAckErrorCount);
}

/*
 * Get Kafka MHUB Messaging statistics
 */
int ism_mhub_getMessagingStats(px_kafka_messaging_stat_t * stats) {
	//produce Lantency for global
	pthread_spin_lock(&g_mhubStatLock);
	memcpy(stats, &mhubMessagingStats, sizeof(px_kafka_messaging_stat_t));
    //  reset some stats to get fresh data.
    mhubMessagingStats.kafkaMsgInBatchMaxWaitTimeMS=0;
    mhubMessagingStats.kafkaMsgInBatchWaitTimeMS=0;
    mhubMessagingStats.kafkaProduceLatencyMS=0;
    mhubMessagingStats.kafkaTotalProduceLatencyMS=0;
    mhubMessagingStats.kafkaProduceMaxLatencyMS=0;
    mhubMessagingStats.kakfaTotalBatchProduceAckReceivedCount=0;
    mhubMessagingStats.kafkaProduceTotalThrottleTimeMS=0;
    pthread_spin_unlock(&g_mhubStatLock);

    return 0;
}


/*
 * Set MessageHubEnabled
 */
int ism_mhub_setMessageHubEnabled(int enabled) {
	TRACE(5, "ism_mhub_setMessageHubEnabled: Enabled=%d gEnabled=%d.\n", enabled, g_mhubEnabled );
	if(enabled != g_mhubEnabled){
		g_mhubEnabled = enabled;
		if(g_mhubEnabled && !g_mhubInited){
			ism_mhub_init();
			ism_mhub_start();
		}
	}
	return 0;
}

#ifndef HAS_BRIDGE
static void disableEnableInternalHub(ism_tenant_t * tenant, int enabled)
{
    if(!tenant){
        TRACE(5, "disableEnableInternalHub: tenant is NULL.\n");
        return;
    }

    ism_tenant_lock();
    if(enabled){
        ism_mhub_t * mhub = tenant->mhublist;
        while (mhub) {
           ism_mhub_lock(mhub);
           if(!mhub->enabled){
               mhub->enabled = 1;
               mhub->need=1;
               if (mhub->useTLS)
                   mhub->need |= UPDATE_Secure;
               TRACE(5, "disableEnableInternalHub: Enabled MHUB: tenant=%s name=%s.\n", tenant->name, mhub->name);
            }else{
                TRACE(5, "disableEnableInternalHub: MHUB is already enabled: tenant=%s name=%s.\n", tenant->name, mhub->name);
            }
           ism_mhub_unlock(mhub);
           mhub = mhub->next;
        }
        if (!tenant->mhubtimer) {
            TRACE(5, "Add MessageHub timer for org %s\n", tenant->name);
            tenant->mhubtimer = ism_common_setTimerRate(ISM_TIMER_LOW, mhubStateCheck, tenant, 1, 2, TS_SECONDS);
        }


    }else{
        //Disable Event MHUB if not already
        ism_mhub_t * mhub = tenant->mhublist;
        while (mhub) {
           ism_mhub_lock(mhub);
           mhub->enabled = 0;
           TRACE(5, "disableEnableInternalHub: Disable MHUB: tenant=%s name=%s.\n", tenant->name, mhub->name);
           ism_mhub_unlock(mhub);
           mhub = mhub->next;
        }
        if (tenant->mhubtimer){
        	   TRACE(5, "Cancel MessageHub timer for org %s\n", tenant->name);
           ism_common_cancelTimer(tenant->mhubtimer);
           tenant->mhubtimer=NULL;
       }
    }

    ism_tenant_unlock();
}
#endif
/*
 * Set UseMHUBKafkaConnection
 * This configuration exclusive for Event and Metering internal tenants.
 */
int ism_mhub_setUseMHUBKafkaConnection(int enabled) {
	TRACE(5, "ism_mhub_setUseMHUBKafkaConnection: Enabled=%d gEnabled=%d.\n", enabled, g_useMHUBKafkaConnection );
	if(enabled != g_useMHUBKafkaConnection){
		g_useMHUBKafkaConnection = enabled;
#ifndef HAS_BRIDGE
		 //Event Tenant
		if(g_useMHUBKafkaConnection>0){
			TRACE(5, "ism_mhub_setUseMHUBKafkaConnection: enable event mhub.\n");
		    disableEnableInternalHub(&eventTenant,1);
		}else{
			TRACE(5, "ism_mhub_setUseMHUBKafkaConnection: disable event mhub.\n");
		    disableEnableInternalHub(&eventTenant,0);
		}

		//Meter Tenant
		if(g_useMHUBKafkaConnection>1){
			TRACE(5, "ism_mhub_setUseMHUBKafkaConnection: enable metering mhub.\n");
		    disableEnableInternalHub(&meterTenant, 1);
		}else{
			TRACE(5, "ism_mhub_setUseMHUBKafkaConnection: disable metering mhub.\n");
		    disableEnableInternalHub(&meterTenant,0);
		}


#endif
	}
	return 0;
}

/*
 * Set MessageHubACKs
 */
int ism_mhub_setMessageHubACK(int acks) {
    TRACE(5, "ism_mhub_setMessageHubACK: ACK=%d currentACK=%d.\n", acks, mhubACKs );

    if(acks!=0 && acks!=1 && acks!=-1){
        TRACE(5, "ism_mhub_setMessageHubACK: value is not valid. value=%d\n", acks);
        return 1;
    }
    if(acks != mhubACKs){
        mhubACKs = acks;
#ifndef HAS_BRIDGE
      ism_tenant_lock();
      ism_tenant_t * tenant = &eventTenant;
      ism_mhub_t * mhub = tenant->mhublist;
      while (mhub) {
          ism_mhub_lock(mhub);
          mhub->mhubACK = mhubACKs;
          TRACE(5, "ism_mhub_setMessageHubACK:  name=%s currentACK=%d.\n", mhub->name,  mhub->mhubACK);
          ism_mhub_unlock(mhub);
          mhub = mhub->next;
      }

      tenant = &meterTenant;
      mhub = tenant->mhublist;
      while (mhub) {
          ism_mhub_lock(mhub);
          mhub->mhubACK = mhubACKs;
          TRACE(5, "ism_mhub_setMessageHubACK:  name=%s currentACK=%d.\n", mhub->name,  mhub->mhubACK);
          ism_mhub_unlock(mhub);
          mhub = mhub->next;
      }
      ism_tenant_unlock();
#endif
    }
    return 0;
}

/*
 * Set MessageHubBatchSize
 */
int ism_mhub_setMessageHubBatchSize(int batchSize) {
    TRACE(5, "ism_mhub_setMessageHubBatchSize: batchSize=%d currentBatchSize=%d.\n", batchSize, mhubBatchSize );

    if(batchSize <= 0){
        TRACE(5, "ism_mhub_setMessageHubBatchSize: value is not valid. value=%d\n", batchSize);
        return 1;
    }
    if(batchSize != mhubBatchSize){
        mhubACKs = batchSize;
#ifndef HAS_BRIDGE
      ism_tenant_lock();
      ism_tenant_t * tenant = &eventTenant;
      ism_mhub_t * mhub = tenant->mhublist;
      while (mhub) {
          ism_mhub_lock(mhub);
          mhub->maxBatchMsgs = batchSize;
          TRACE(5, "ism_mhub_setMessageHubBatchSize:  name=%s maxBatchMsgs=%d.\n", mhub->name,  mhub->maxBatchMsgs);
          ism_mhub_unlock(mhub);
          mhub = mhub->next;
      }

      tenant = &meterTenant;
      mhub = tenant->mhublist;
      while (mhub) {
          ism_mhub_lock(mhub);
          mhub->maxBatchMsgs = batchSize;
          TRACE(5, "ism_mhub_setMessageHubACK:  name=%s maxBatchMsgs=%d.\n", mhub->name,  mhub->maxBatchMsgs);
          ism_mhub_unlock(mhub);
          mhub = mhub->next;
      }
      ism_tenant_unlock();
#endif
    }
    return 0;
}

/*
 * Set MessageHubBatchTimeMillis
 */
int ism_mhub_setMessageHubBatchTimeMillis(int batchmillis) {
    TRACE(5, "ism_mhub_setMessageHubBatchTimeMillis: batchmillis=%d currentBatchSize=%d.\n", batchmillis, mhubBatchTimeMillis );

    if(batchmillis <= 0){
        TRACE(5, "ism_mhub_setMessageHubBatchTimeMillis: value is not valid. value=%d\n", batchmillis);
        return 1;
    }
    if(batchmillis != mhubBatchTimeMillis){
        mhubBatchTimeMillis = batchmillis;
#ifndef HAS_BRIDGE
      ism_tenant_lock();
      ism_tenant_t * tenant = &eventTenant;
      ism_mhub_t * mhub = tenant->mhublist;
      while (mhub) {
          ism_mhub_lock(mhub);
          mhub->maxBatchTimeMS = batchmillis;
          TRACE(5, "ism_mhub_setMessageHubBatchTimeMillis:  name=%s maxBatchTimeMS=%d.\n", mhub->name,  mhub->maxBatchTimeMS);
          ism_mhub_unlock(mhub);
          mhub = mhub->next;
      }

      tenant = &meterTenant;
      mhub = tenant->mhublist;
      while (mhub) {
          ism_mhub_lock(mhub);
          mhub->maxBatchTimeMS = batchmillis;
          TRACE(5, "ism_mhub_setMessageHubBatchTimeMillis:  name=%s maxBatchTimeMS=%d.\n", mhub->name,  mhub->maxBatchTimeMS);
          ism_mhub_unlock(mhub);
          mhub = mhub->next;
      }
      ism_tenant_unlock();
#endif
    }
    return 0;
}

/*
 * Set MessageHubBatchTimeMillis
 */
int ism_mhub_setMessageHubBatchSizeBytes(int batchbytes) {
    TRACE(5, "ism_mhub_setMessageHubBatchSizeBytes: batchbytes=%d currentBatchSize=%d.\n", batchbytes, mhubBatchSizeBytes );

    if(batchbytes <= 0){
        TRACE(5, "ism_mhub_setMessageHubBatchSizeBytes: value is not valid. value=%d\n", batchbytes);
        return 1;
    }
    if(batchbytes != mhubBatchSizeBytes){
        mhubBatchSizeBytes = batchbytes;
#ifndef HAS_BRIDGE
      ism_tenant_lock();
      ism_tenant_t * tenant = &eventTenant;
      ism_mhub_t * mhub = tenant->mhublist;
      while (mhub) {
          ism_mhub_lock(mhub);
          mhub->maxBatchBytes = batchbytes;
          TRACE(5, "ism_mhub_setMessageHubBatchSizeBytes:  name=%s maxBatchBytes=%d.\n", mhub->name,  mhub->maxBatchBytes);
          ism_mhub_unlock(mhub);
          mhub = mhub->next;
      }

      tenant = &meterTenant;
      mhub = tenant->mhublist;
      while (mhub) {
          ism_mhub_lock(mhub);
          mhub->maxBatchBytes = batchbytes;
          TRACE(5, "ism_mhub_setMessageHubBatchSizeBytes:  name=%s maxBatchBytes=%d.\n", mhub->name,  mhub->maxBatchBytes);
          ism_mhub_unlock(mhub);
          mhub = mhub->next;
      }
      ism_tenant_unlock();
#endif
    }
    return 0;
}




#ifndef HAS_BRIDGE

/*
 * Format a metering messages
 */
static inline void formatMeteringMessage(concat_alloc_t * buf, char * clientID, uint64_t readbytes, uint64_t writebytes,  uint64_t time) {
    char xbuf[256];
    char tbuf[32];
    /* Format the timestamp to milliseconds in UTC */
    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_UTC);
    ism_common_setTimestamp(ts, time);
    ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 6, ISM_TFF_ISO8601);
    sprintf(xbuf, "{\"T\":\"%s\",\"C\":", tbuf);
    ism_json_putBytes(buf, xbuf);
    ism_json_putString(buf, clientID);

    /* format the statistics */
    sprintf(xbuf, ",\"RB\":%lu,\"WB\":%lu,\"TP\":0}", readbytes, writebytes);
    ism_json_putBytes(buf, xbuf);
    ism_common_closeTimestamp(ts);
}

/**************************************************************************************/
/* Metering Producing Functions */
/**************************************************************************************/

static int sendMetering(ism_mhub_t * mhub, ism_transport_t * transport, concat_alloc_t * buf, int topic_index, int partition) {
    int  rc = 1;
    kafka_produce_msg_t * meter;
    uint64_t  old_readb;
    uint64_t  old_writeb;
    uint64_t  curr_readb;
    uint64_t  curr_writeb;
    char      clientID [63];
    char body_xbuf [2048];
    concat_alloc_t body_buf = {body_xbuf, sizeof body_xbuf};

    if (*transport->protocol_family=='k' || *transport->protocol_family=='a')
        return 0;

    /*
	 * Get topic and partition
	 */
	mhub_topic_t * mhub_topic = mhub->topics[topic_index];
	if (mhub_topic && mhub_topic->valid == 2) {
		TRACE(8, "Event Streams Metering message not published because topic does not exist: mhub=%s topic=%s\n", mhub->id, mhub_topic->name);
		return 2;
	}

	/* Update Total Kafka (for all topic) Stats */
	pthread_spin_lock(&g_mhubStatLock);
	mhubMessagingStats.kakfaTotalPendingMsgsCount++;
	pthread_spin_unlock(&g_mhubStatLock);


    /* Create the binary metering message and body*/
    old_readb = transport->read_bytes;
    old_writeb = transport->write_bytes;

    curr_readb = old_readb - transport->read_bytes_prev;
    curr_writeb = old_writeb - transport->write_bytes_prev;
    transport->read_bytes_prev = old_readb;
    transport->write_bytes_prev = old_writeb;

    if (transport->name)
        strcpy(clientID, transport->name);
    else
        *clientID = 0;

    uint64_t time =  ism_common_currentTimeNanos();

    //Format and Set into the body
    formatMeteringMessage(&body_buf, clientID, curr_readb, curr_writeb,  time);

    meter = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_eventstreams,18),1, sizeof *meter + body_buf.used);
    meter->time = time;
    meter->buf = (char *)(meter+1);
    memcpy(meter->buf, body_buf.buf, body_buf.used);
    meter->buflen = body_buf.used;

    if (SHOULD_TRACEC(9, Kafka)) {
    		char * payload = alloca(meter->buflen+1);
    		memcpy(payload, meter->buf,  meter->buflen);
    		payload[meter->buflen] = '\0';
    		TRACE(9, "MHUB metering payload:  len=%d payload=%s\n", meter->buflen, payload);
    }

    int which = mhub_topic->partcount > 1 ? partition % mhub_topic->partcount : 0;
	mhub_part_t * mhub_part = (mhub_part_t *)&mhub_topic->partitions[which];

	pthread_mutex_lock(&mhub_part->lock);

	ism_transport_t *  kafka_transport = mhub_part->transport;
	meter->dcrc = partition;

    /* Link it in */
    /* Link in this event */
	if ( mhub_part->kafka_msg_last)
	   mhub_part->kafka_msg_last->next = meter;
	mhub_part->kafka_msg_last = meter;

	if (!mhub_part->kafka_msg_first) {
	   mhub_part->kafka_msg_first = meter;
	   mhub_part->kafka_msg_first_time = meter->time;
	   if (!mhub->mhubACK)
		mhub_part->kafka_msg_count = 1;
	   else
		mhub_part->kafka_msg_count++;

	} else {
	   mhub_part->kafka_msg_count++;
	}

	/* If the kafka publishing connection is open, continue*/
	if (kafka_transport && kafka_transport->pobj && kafka_transport->pobj->state==TCP_CONNECTED) {
		rc = 0;
	} else {
		//The Connection Record is not in the open state. or TCP connection is not in Open State
		//Still keep the message in the Pending Q
		//Will move it once the connection established.
		TRACE(8, "sendMetering: Partition Connection is not open. which=%d pending_msg_count=%d\n", which, mhub_part->kafka_msg_count);
		rc = 1;
	}

	/* If connection is open check if batch is full */
	if (rc == 0) {
		int need = needMHubBatch(mhub, mhub_part, meter->time);
		if (need > 0) {
			/* Move to the IOP of the outbound transport */
			TRACE(8, "Schedule job to produce metering message: mhub=%s topic=%s partition=%u\n", mhub->id, mhub_topic->name, which);
			ism_transport_submitAsyncJobRequest(kafka_transport, mhubProduceJob, (void *)mhub, (((uint64_t)topic_index)<<32) + which);
		}
	}

	pthread_mutex_unlock(&mhub_part->lock);

    return rc;
}

/*
 * Send a kafka metering message to Event Streams using Kafka Connection config
 *
 * The message is not actually sent to kafka at this point but is saved as a binary record.
 * When there are enough messages in this list and the kafka connection is up we actually
 * send the message.
 *
 * If the kafka connection is not up, save the message and return a non-zero return code.
 * This is used to allow the invoker to delay creating metering messages if this is possible.
 * At connection close we create metering anyway, but we can just stop the timer loop and
 * do that processing later when the kafka connection is available.
 */

int ism_mhub_sendMetering(ism_transport_t * transport, concat_alloc_t * buf) {
	int rc=0;
	// topic index, and partition
	int topic_index = 0; //Assume there is only one topic for metering
	if (g_mhubEnabled) {
		ism_tenant_t* meteringTenant = &meterTenant;
		if (meteringTenant->mhublist) {
			ism_mhub_t * mhub = meteringTenant->mhublist;
			while (mhub) {
				if(!mhub->disabled){
					//Publish The message
					int part = mhub_meter_part_crc % mhub->topics[topic_index]->partcount;
					sendMetering(mhub, transport, buf,  topic_index, part);
				}
				mhub = mhub->next;
			}
		}
	}

	return rc;
}

#endif
