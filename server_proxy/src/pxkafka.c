/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
#include <pxmqtt.h>
#include <protoex.h>

#ifndef NO_PROXY

#define BILLION        1000000000L

#define KAFKA_LOG_MSG_OVERHEAD		34

static px_kafka_messaging_stat_t kafkaIMMessagingStats = {0};

/*
 * Externals
 */
int ism_proxy_sendBytes(ism_transport_t * transport, char * buf, int len, int protval, int flags);
extern void ism_tcp_init_transport(ism_transport_t * transport);
struct ssl_ctx_st * ism_transport_clientTlsContext(const char * name, const char * tlsversion, const char * cipher);
int  ism_kafka_createConnection(ism_transport_t * transport, ism_server_t * server);
static int kafkaMetadataConnected(ism_transport_t * transport, int rc) ;
static const char * getJsonValue(ism_json_entry_t * ent);
int ism_kafka_startMeteringMetadataConnect(ism_server_t * server);
int ism_kafka_startMeteringPublishConnect(ism_server_t * server);
int ism_kafka_term(void);
static int kafkaProduceTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata);
int ism_kafka_setKafkaAPIVersion(int api_version) ;
int ism_kafka_im_messaging_init(int useKafkaIMKessaging);
int ism_kafka_startEventProduceConnect( kafkaConnectionRecord_t * connectionRecord, ism_kafka_authenticator_t  authenticator, void * authenticator_data) ;
int ism_kafka_startEventMetadataConnect(ism_server_t * server, kafkaTopicRecord_t * topicRecord, ism_kafka_authenticator_t  authenticator, void *authenticator_data) ;
int ism_kafka_setKafkaBroker(ism_json_entry_t * ents);
static void freeIMEvent(kafka_produce_msg_t * msg);
static int startKafkaIMMessagingTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) ;
int ism_proxy_parseMQTTTopic(char * topic, const char * * devtype, const char * * devid, const char * * event, const char * * fmt);
int ism_kafka_message_produce(ism_transport_t * transport,  kafkaConnectionRecord_t * connectionRecord, kafka_produce_msg_t * msgs, int * producedMsgsCount, int isResend) ;
int ism_kafka_initMessageHubLocal(void) ;
int ism_kafka_setKafkaBrokerInternal(ism_json_entry_t * ents) ;
#ifdef GAI_SIG
static int addrinfo_callback(void * xtransport);
#else
static void addrinfo_callback(union sigval xtransport);
#endif

extern ism_routing_route_t * g_internalKafkaRoute;
extern ism_routing_config_t *       g_routeconfig;
int ism_kafka_getAddress(ism_server_t * server, ism_transport_t * transport,  ism_gotAddress_f gotAddress);
#endif
void ism_kafka_putVarInt(concat_alloc_t * buf, int32_t len);
mqtt_prop_ctx_t * ism_proxy_getMqttContext(int version);

#ifndef NO_PROXY
/*
 * Binary metering entry.
 * This is used to save a metering message from when it is produced until it is sent.
 */
typedef struct kafka_meter_t {
    struct kafka_meter_t * next;
    uint64_t  time;
    uint64_t  readbytes;
    uint64_t  writebytes;
    uint8_t   state;
    char      clientID [63];
} kafka_meter_t;


/*
 * Kafka Broker structure
 */
typedef struct kafka_broker_t {
    int nodeid;
	char * hoststr;
	int hostlen;
	int port;
} kafka_broker_t;


/*
 * Load a big endian 2 byte integer
 */
#define BIGINT16(p) (((int)(uint8_t)(p)[0]<<8) | (uint8_t)(p)[1])


/*
 * Globals
 */
static const char * host;
static ism_server_t *     kafka_server = NULL;      /* Server structure containing kafka brokers */
static pthread_mutex_t    kafka_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_spinlock_t kafka_spin;
static ism_transport_t *  kafka_metadata = NULL;    /* Connection to get metadata */
static ism_transport_t *  kafka_publish = NULL;     /* Connection to produce metering */
static kafka_meter_t *    kafka_first = NULL;       /* First un-sent message */
static kafka_meter_t *    kafka_last = NULL;        /* Last un-sent message */
       int                g_useKafka = 0;           /* Use kafka metering */
       int                g_useKafkaTLS = 0;        /* Use TLS for kafka metering **/
static const char *       kafka_tls_cipher = "AES128-GCM-SHA256:AES128+AESGCM:!aNULL";
static const char *       kafka_tls_method = "TLSv1.2";
static const char *       kafka_metering_topic = NULL;
static ism_time_t         kafka_first_time;         /* Time of first un-sent message */
static int                kafka_meter_count = 0;    /* Count of un-sent messages */
static int                kafka_batch_size = 100;   /* Message count at which to send batch */
static int                kafka_batch_seconds = 5;  /* Time delta in seconds at which to send batch */
static uint64_t           kafka_batch_time;         /* Time delat in nanos at which to send batch */
static uint32_t           kafka_part_crc;           /* Hash of this client used to select partition */
static uint32_t           kafka_leader_part;        /* Selected leader partition */
static uint32_t           kafka_leader_node;        /* Selected leader node */
static int                kafka_needmetadata = 1;   /* Need metadata */
ism_timer_t               kafka_timer = NULL;


/*IM Messaging configurations*/
int			              g_useKafkaIMMessaging = 0;           /* Use kafka IM Messaging */
int			              g_checkKafkaIMMsgRouting = 0;           /* Use kafka IM Messaging */
int                		  g_useKafkaIMMessagingTLS = 0;        /* Use TLS for kafka IM Messaging **/
int                		  g_instanceIndex = -1;       		 /* msproxy instance ID index **/
char * 					  g_IMMessagingSelectorStr = NULL;
ismRule_t * 				  g_IMMessagingSelector; 			//Selector rule

/*This kafka_im_lock lock protects the Metadata resources*/
static pthread_mutex_t    kafka_im_lock = PTHREAD_MUTEX_INITIALIZER;
static char *       kafka_im_messaging_topic = NULL;
static char *       kafka_im_messaging_hosts = NULL;
static const char *       kafka_im_messaging_tls_cipher = "AES128-GCM-SHA256:AES128+AESGCM:!aNULL";
static const char *       kafka_im_messaging_tls_method = "TLSv1.2";
static int                kafka_im_messaging_batch_size = 100;   /* Message count at which to send batch */
static int 				  kafka_im_messaging_batch_size_bytes = 250000;  //Maximum number of bytes per produce
static int                kafka_im_messaging_recovery_batch_size = 100;   /* Message count at which to send batch */
static int                kafka_im_messaging_batch_millis =  250;  /* Time delta in millis at which to send batch */
static uint64_t           kafka_im_messaging_batch_time;
static ism_server_t *     kafka_im_messaging_server = NULL;      /* Server structure containing kafka brokers */
//static ism_transport_t *  kafka_im_messaging_metadata = NULL;    /* Connection to get metadata for*/
//static int                kafka_im_messaging_needmetadata = 1;   /* Need metadata */
static int                kafka_metering_ack = 0;
static int 		 		  kafka_im_produce_should_ack = 0;
static int 		 		  kafka_im_shutdown_onerror_time = 0;
static int				  kafka_im_produce_error_log_interval_count = 10;
static  ismArray_t        kafkaPartitionConnections;
ism_timer_t               kafka_im_messaging_timer = NULL;
int g_kafkaIMConnDetailStatsEnabled = 0;

/*
 * Global MessageHub Configurations
 */
static int useKafkaMessageHub = 0;
static int kafkaMessageHubBatchTimeMillis = 250;
static int kafkaMessageHubBatchSize = 100;
static int kafkaMessageHubUseTLS = 0;
const char * kafkaMessageHubTLSCiphers =  "AES128-GCM-SHA256:AES128+AESGCM:!aNULL";
const char * kafkaMessageHubTLSMethod = "TLSv1.2";
static int kafkaMessageHubProduceAck = 0;
static int kafkaMessageHubDistMethod = 0;

/*
 * Table to keep the Kafka Topic and The TopicRecord object
 */
ismHashMap *       g_kafkaTopicRecordTable;

/*Number of msproxies in the env*/
float g_msproxies_count = 1;

static int                kafka_api_version = 0;
static uint16_t           kafka_produce_version = 0;
static uint16_t           kafka_metadata_version = 0;
#endif
static uint8_t            kafka_message_version = 0;
#ifndef NO_PROXY

/* API values */
#define ProduceRequest    0x0000
#define MetadataRequest   0x0003
#define MetadataRequest0  0x00030000   /* with version 0 */


/*
 * Initialize kafka metering
 */
int ism_kafka_metering_init(int useKafka, int useTLS) {

    const char * topic;
    g_useKafka = useKafka;
    g_useKafkaTLS = useTLS;
    pthread_spin_init(&kafka_spin, 0);

    int kapi_version = ism_common_getIntConfig("KafkaAPIVersion", 0);
    ism_kafka_setKafkaAPIVersion(kapi_version);

    kafka_batch_seconds = ism_common_getIntConfig("KafkaMeteringBatchTime", kafka_batch_seconds);
    if (kafka_batch_seconds < 1)
        kafka_batch_seconds = 5;
    kafka_batch_time = kafka_batch_seconds * 1000000000L;
    kafka_batch_size = ism_common_getIntConfig("KafkaMeteringBatchSize", kafka_batch_size);
    if (kafka_batch_size < 1)
        kafka_batch_size = 1;
    host = ism_common_getHostnameInfo();
    if (!host)
        host = "imaproxy";
    kafka_part_crc = ism_common_crc(0, (char *)host, strlen(host));
    if (useTLS) {
        const char * ciphers = ism_common_getStringConfig("KafkaMeteringCipher");
        const char * method = ism_common_getStringConfig("KafkaMeteringTLSMethod");
        if (ciphers && *ciphers) {
            kafka_tls_cipher = ciphers;
        }
        if (method && *method) {
            kafka_tls_method = method;
        }
    }
    if (!kafka_metering_topic) {
        topic = ism_common_getStringConfig("KafkaMeteringTopic");
        if (topic) {
            kafka_metering_topic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka_global,1000),topic);
            if (kafka_server) {
                kafka_server->metering_topic = kafka_metering_topic;
            }
        }
    }
    if (g_useKafka) {
        TRACE(3, "Initialize kafka metering: useKafka=%u useTLS=%u kafkaAPIVersion=%d topic=%s batch=%u time=%usec\n",
                g_useKafka, g_useKafkaTLS, kafka_api_version, kafka_metering_topic, kafka_batch_size, kafka_batch_seconds);
        kafka_timer = ism_common_setTimerRate(ISM_TIMER_LOW, kafkaProduceTimer, NULL, 2100, 900, TS_MILLISECONDS);
    }
    if (g_useKafkaTLS)
        TRACE(3, "Set kafka metering TLS: method=%s cipher=%s\n", kafka_tls_method, kafka_tls_cipher);
    if (kafka_server && g_useKafka && kafka_metering_topic) {
        ism_kafka_startMeteringMetadataConnect(kafka_server);
    }

    return 0;
}

/*
 * Initialize Kafka for metering and event processing
 *
 */
int ism_kafka_init(int useKafka, int useTLS) {

	/* Init Table of TopicRecord */
	g_kafkaTopicRecordTable = ism_common_createHashMap(128,HASH_STRING);

	/*Init Kafka Metering*/
	ism_kafka_metering_init(useKafka, useTLS);


	/*
	 * Initialize which instance of the messaging proxy this is.
	 * If the instnace is not known, then we cannot select only
	 * some partitions to write to.
	 */
	char * instanceIDVar = getenv("INSTANCE_ID");
	if (instanceIDVar && *instanceIDVar) {
	    int value;
	    char * eos;
	    int instanceIDlen = (int)strlen(instanceIDVar);
        if (instanceIDlen > 8 && memcmp(instanceIDVar, "msproxy.", 8)) {
            value = strtoul(instanceIDVar+8, &eos, 10);
            if (*eos == 0)
                g_instanceIndex = value;
        }
        TRACE(3, "Kafka msproxy instance index=%s index=%d\n", instanceIDVar, g_instanceIndex);
	} else {
	    TRACE(3, "Kafka msproxy instance is not set\n");
	}

    /*
     * Init the IM Messaging if enabled
     */
    int useKafkaIMMessaging = ism_common_getIntConfig("UseKafkaIMMessaging", 0);
    ism_kafka_im_messaging_init(useKafkaIMMessaging);

    /* Init Message HUB */
    ism_kafka_initMessageHubLocal();

    return 0;
}



/*
 * Change the setting of useKafka
 */
int ism_kafka_setUseKafkaMetering(int useKafka) {
    int oldkafka = g_useKafka;
    g_useKafka = useKafka;
    if ((useKafka && !oldkafka) || (!useKafka && oldkafka)) {
        if (useKafka) {
            g_useKafka = useKafka;
            TRACE(3, "Initialize kafka metering: useKafka=%u useTLS=%u topic=%s batch=%u time=%usec\n",
                    g_useKafka, g_useKafkaTLS, kafka_metering_topic, kafka_batch_size, kafka_batch_seconds);
            if (kafka_server && kafka_metering_topic) {
                ism_kafka_startMeteringMetadataConnect(kafka_server);
            }
        } else {
            TRACE(3, "Stop kafka metering\n");
            ism_kafka_term();
            g_useKafka = useKafka;
        }
    } else {
        g_useKafka = useKafka;
    }
    return 0;
}


/*
 * Set the global kafka metering topic.
 * This can only be set once but can be set dynamically.
 */
int ism_kafka_setMeteringTopic(const char * topic) {
    if (!kafka_metering_topic && topic) {
        kafka_metering_topic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka_global,1000),topic);
        TRACE(3, "Set kafka metering topic: %s\n", topic);
        if (kafka_server) {
            kafka_server->metering_topic = kafka_metering_topic;
            if (g_useKafka) {
                ism_kafka_startMeteringMetadataConnect(kafka_server);
            }
        }
    }
    return 0;
}


/*
 * Check the port value.
 */
static int portValue(const char * value) {
    if (!value || strlen(value) > 1023)
        return -2;
    const char * pos = strrchr(value, ':');
    if (pos) {
        return atoi(pos+1);
    } else {
        return -1;
    }
}

/*
 * Set the list of kafka brokers.
 * This can only be set once but can be set dynamically.
 */
int ism_kafka_setBroker(ism_json_entry_t * ents) {
    ism_json_entry_t * ent;
    int count = ents->count;
    int i;
    char valcopy[1024];
    ism_server_t * server;
    int  rc = 0;

    pthread_mutex_lock(&kafka_lock);

    /*
     * For now do not allow kafka broker list to be changed
     */
    if (kafka_server) {
        TRACE(2, "KafkaBroker can only be set one\n");
        rc = ISMRC_BadPropertyValue;
        pthread_mutex_unlock(&kafka_lock);
        return rc;
    }

    ent = ents+1;
    for (i=0; i<count; i++) {
        if (ent->objtype != JSON_String || portValue(ent->value) <= 0) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ents->name, getJsonValue(ent));
            rc = ISMRC_BadPropertyValue;
            break;
        }
        ent++;
    }
    if (count > 32) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ents->name, "");
        rc = ISMRC_BadPropertyValue;
    }
    if (rc == 0) {
        const char * topic;
        server = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_kafka,1),1, sizeof(ism_server_t));
        strcpy(server->structid, "IoTKSRV");
        server->name = "_Kafka";
        server->metering_topic = kafka_metering_topic;
        pthread_spin_init(&server->lock, 0);
        server->serverKind = 1;
        server->getAddress = ism_kafka_getAddress;
        if (g_useKafkaTLS) {
            server->tlsCTX = ism_transport_clientTlsContext("kafka", kafka_tls_method, kafka_tls_cipher);
        }
        server->ipaddr_count = count;
        topic = ism_common_getStringConfig("KafkaMeteringTopic");
        if (topic)
            server->metering_topic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),topic);
        ent = ents+1;
        for (i=0; i<count; i++) {
            char * pos;
            strcpy(valcopy, ent->value);
            pos = strrchr(valcopy, ':');
            *pos++ = 0;
            server->address[i] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),valcopy);
            server->a_port[i] = atoi(pos);
            TRACE(4, "Set kafka metering broker: %u %s:%u\n", i, server->address[i], server->a_port[i]);
            ent++;
        }
        kafka_server = server;
        /*For now, Assume Metering and IM Messaging are using same broker*/
        if (g_useKafka && kafka_metering_topic)
            ism_kafka_startMeteringMetadataConnect(server);
    }
    pthread_mutex_unlock(&kafka_lock);

    //Assume Metering and IM Messaging are using the same broker
    //ism_kafka_setKafkaBroker(ents);
    ism_kafka_setKafkaBrokerInternal(ents);

    return 0;
}


/*
 * Format a metering messages
 */
static inline void formatMeteringMessage(concat_alloc_t * buf, kafka_meter_t * msg, ism_ts_t * ts) {
    char xbuf[256];
    char tbuf[32];
    /* Format the timestamp to milliseconds in UTC */
    ism_common_setTimestamp(ts, msg->time);
    ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 6, ISM_TFF_ISO8601);
    sprintf(xbuf, "{\"T\":\"%s\",\"C\":", tbuf);
    ism_json_putBytes(buf, xbuf);
    ism_json_putString(buf, msg->clientID);

    /* format the statistics */
    sprintf(xbuf, ",\"RB\":%lu,\"WB\":%lu,\"TP\":0}", msg->readbytes, msg->writebytes);
    ism_json_putBytes(buf, xbuf);
}


/*
 * Add a metering message to the produce message set.
 */
static void addMeteringMessage(concat_alloc_t * buf, kafka_meter_t * msg, ism_ts_t * ts) {
    int  msgsize;
    int  crcpos;
    int  bodylen;

    ism_kafka_putInt8(buf, 0);        /* offset, not set on produce */
    msgsize = ism_protocol_reserveBuffer(buf, 8); /* reserve space for msgsize AND crc */
    crcpos = msgsize+4;
    ism_kafka_putInt1(buf, kafka_message_version);        /* magic */
    ism_kafka_putInt1(buf, 0);                        /* attr */
    if (kafka_message_version > 0) {
        ism_kafka_putInt8(buf, ism_common_convertTimeToJTime(msg->time));        /* timestamp in milliseconds*/
    }
    ism_kafka_putBytes(buf, NULL, 0); /* Use a null key */
    bodylen = ism_protocol_reserveBuffer(buf, 4);
    formatMeteringMessage(buf, msg, ts);

    ism_kafka_putInt4At(buf, bodylen, buf->used-bodylen-4);  /* Fill in body length */
    uint32_t crc = ism_common_crc(0, buf->buf+crcpos+4, buf->used-crcpos-4);
    ism_kafka_putInt4At(buf, crcpos, crc);                   /* Fill in CRC */
    ism_kafka_putInt4At(buf, msgsize, buf->used-msgsize-4);  /* Fill in message size */
}

/*
 * Add a kafka record batch which is the message data for v2
 * @param buf        The output buffer
 * @param msgs       The linked list of messages to write
 * @param ts         An open time stamp object
 */
int ism_kafka_addMeteringRecordBatch(ism_transport_t * transport, concat_alloc_t * buf, kafka_meter_t * msgs, ism_ts_t * ts) {
    char xmbuf[512];
    char xbbuf[512];
    int  startused = buf->used;
    int  batchsize;
    int  crcpos;
    int  recordcount;
    int  count;
    int  tsloc;
    uint64_t mintime = 0;
    uint64_t maxtime = 0;
    kafka_meter_t * msg;

    /* Message buffer to handle all of the varint lengths */
    concat_alloc_t mbuf = {xmbuf, sizeof xmbuf};
    concat_alloc_t bbuf = {xbbuf, sizeof xbbuf};

    ism_kafka_putInt8(buf, 0);        /* baseOffset */
    batchsize = ism_protocol_reserveBuffer(buf, 4);
    ism_kafka_putInt4(buf, 0);        /* partitionLeaderEpoch: not set on produce */
    ism_kafka_putInt1(buf, kafka_message_version);        /* magic */
    crcpos = ism_protocol_reserveBuffer(buf, 4);
    ism_kafka_putInt2(buf, 0);        /* attributes: no compress, client timestamp, no transaction  */
    tsloc = ism_protocol_reserveBuffer(buf, 20);
    ism_kafka_putInt8(buf, -1L);        /* producerId: must set to use exactly once */
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
        ism_kafka_putVarInt(&mbuf, count);
        count++;
        ism_kafka_putVarInt(&mbuf, 0);                   /* keyLength */
        bbuf.used = 0;
        formatMeteringMessage(&bbuf, msg, ts);
        ism_kafka_putVarInt(&mbuf, bbuf.used);           /* body length */
        ism_common_allocBufferCopyLen(&mbuf, bbuf.buf, bbuf.used);     /* body */
        ism_kafka_putVarInt(&mbuf, 0);                   /* header length */
        ism_kafka_putVarInt(buf, mbuf.used);
        ism_common_allocBufferCopyLen(buf, mbuf.buf, mbuf.used);
        //TODO: Check if need to free msg when ack is 0
        if (kafka_metering_ack==0) {
            kafka_meter_t * oldmsg = msg;
            msg = msg->next;
            ism_common_free(ism_memory_proxy_kafka,oldmsg);
        } else {
            msg = msg->next;;
        }
    }
    ism_kafka_putInt4At(buf, tsloc, count-1);
    transport->pobj->lastOffset += count;
    ism_kafka_putInt8At(buf, tsloc+4, mintime);
    ism_kafka_putInt8At(buf, tsloc+12, maxtime);
    ism_kafka_putInt4At(buf, recordcount, count);
    uint32_t crc = ism_common_crc32c(0, buf->buf+crcpos+4, buf->used-crcpos-4);
    ism_kafka_putInt4At(buf, crcpos, crc);                   /* Fill in CRC */
    ism_kafka_putInt4At(buf, batchsize, buf->used-batchsize-4);  /* Fill in message size */
    if (mbuf.inheap)
        ism_common_freeAllocBuffer(&mbuf);
    return buf->used - startused;
}

/*
 * Send a set of messages to kafka
 */
int ism_kafka_produceMetering(ism_transport_t * transport, kafka_meter_t * msgs) {
    int msgcnt = 0;
    int rc;
    char xbuf [16*1024];   /* room for 100 metering messages */
    concat_alloc_t cbuf = {xbuf, sizeof xbuf, 4};
    concat_alloc_t * buf = &cbuf;
    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_UTC);

    //ism_kafka_putInt4(buf, ProduceRequest0);
    ism_kafka_putInt2(buf, ProduceRequest);        /*api key*/
    ism_kafka_putInt2(buf, kafka_produce_version);      /*api version*/
    ism_kafka_putInt4(buf, 1);        /* CorrID   */
    ism_kafka_putString(buf, ism_common_getHostnameInfo(), -1);

    ism_kafka_putInt2(buf, kafka_metering_ack);        /* ack*/
    ism_kafka_putInt4(buf, 5000);     /* timeout*/

    ism_kafka_putInt4(buf, 1);        /* topic count*/
    ism_kafka_putString(buf, kafka_metering_topic, -1);
    ism_kafka_putInt4(buf, 1);        /* partition count */
    ism_kafka_putInt4(buf, kafka_leader_part);   /* partition*/
    int setsize = ism_protocol_reserveBuffer(buf, 4);
    if (kafka_message_version >= 2) {
        ism_kafka_addMeteringRecordBatch(transport, buf, msgs, ts);
    } else {
        while (msgs) {
            kafka_meter_t * msg;
            msg = msgs;
            addMeteringMessage(buf, msg, ts);
            msgs = msgs->next;
            msgcnt++;
            ism_common_free(ism_memory_proxy_kafka,msg);
        }
    }
    ism_common_closeTimestamp(ts);

    TRACE(8, "Produce kafka metering: count=%u\n", msgcnt);

    ism_kafka_putInt4At(buf, setsize, buf->used-setsize-4);
    rc = transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);
#if 0
    static int counter = 0;
    char name[16];
    sprintf(name, "kafka%d", counter++);
    FILE * f = fopen(name, "wb");
    fwrite(buf->buf+4, buf->used-4, 1, f);
    printf("write %s of size %d\n", name, buf->used-4);
    fclose(f);
#endif

    if (buf->inheap)
        ism_common_freeAllocBuffer(buf);
    return rc;
}


/*
 * Check to see if a metering batch is full, and return the batch if it is.
 * This is called both when we sava a message and when start a kafka connection.
 *
 * This must be called with the kafka_spin lock held.
 */
static inline kafka_meter_t *  checkMeteringBatch(ism_time_t now) {
    kafka_meter_t * doProduce = NULL;
    if (kafka_meter_count > kafka_batch_size ||
        (now - kafka_first_time) > kafka_batch_time ) {
        doProduce = kafka_first;
        kafka_last = NULL;
        kafka_first = NULL;
    }
    return doProduce;
}

/*
 * Send a kafka metering message.
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
int ism_kafka_sendMetering(ism_transport_t * transport, concat_alloc_t * buf) {
    int  rc = 1;
    kafka_meter_t * doProduce = NULL;
    int  extralen = 0;
    int  cidlen;
    kafka_meter_t * meter;
    uint64_t  readb;
    uint64_t  writeb;

    if (*transport->protocol_family=='k' || *transport->protocol_family=='a')
        return 0;

    /*
     * We use a fixed size message to make malloc and free for efficient, but handle
     * the case where the ClientID is too long.
     */
    if (transport->name) {
        cidlen = strlen(transport->name);
        if (cidlen > 62)
            extralen = cidlen-62;
    }
    meter = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_kafka,2),1, sizeof *meter + extralen);

    /* Create the binary metering message */
    pthread_spin_lock(&kafka_spin);
    readb = transport->read_bytes;
    writeb = transport->write_bytes;
    meter->time = ism_common_currentTimeNanos();
    meter->readbytes = readb - transport->read_bytes_prev;
    meter->writebytes = writeb - transport->write_bytes_prev;
    meter->state = 0;     /* Future use for ACK */
    transport->read_bytes_prev = readb;
    transport->write_bytes_prev = writeb;
    if (transport->name)
        strcpy(meter->clientID, transport->name);
    else
        *meter->clientID = 0;
    TRACE(9, "kafka metering entry: C=%s WB=%lu RB=%lu\n", meter->clientID, meter->writebytes, meter->readbytes);

    /* Link it in */
    if (kafka_last)
        kafka_last->next = meter;
    kafka_last = meter;
    if (!kafka_first) {
        kafka_first = meter;
        kafka_first_time = meter->time;
        kafka_meter_count = 1;
    } else {
        kafka_meter_count++;
    }

    /* If the kafka publishing connection is open, we return a 0 return code */
    if (kafka_publish && kafka_publish->state == ISM_TRANST_Open)
        rc = 0;

    /* If connection is open check if batch is full */
    if (rc == 0) {
        doProduce = checkMeteringBatch(meter->time);
    }
    TRACE(9, "sendMetering rc=%u kafka_publish=%p state=%u doProduce=%p count=%u\n", rc, kafka_publish,
            kafka_publish ? kafka_publish->state : -1, doProduce, kafka_meter_count);
    pthread_spin_unlock(&kafka_spin);

    /* If we have a batch to send, do the kafka produce now */
    if (doProduce) {
        ism_kafka_produceMetering(kafka_publish, doProduce);
    }
    return rc;
}

/*
 * Run a timer to make sure metering records are sent even if there are no more produced.
 * This is mostly useful for testing.
 */
static int kafkaProduceTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    kafka_meter_t * doProduce = NULL;
    pthread_spin_lock(&kafka_spin);
    if (kafka_publish && kafka_publish->state == ISM_TRANST_Open) {
        doProduce = checkMeteringBatch(ism_common_currentTimeNanos());
    }
    pthread_spin_unlock(&kafka_spin);
    if (doProduce) {
        ism_kafka_produceMetering(kafka_publish, doProduce);
    }
    return 1;
}

/*
 * Terminate kafka processing
 */
int ism_kafka_term(void) {
    kafka_meter_t * doProduce = NULL;
    if (g_useKafka) {
        TRACE(5, "Terminate kafka\n");
        /*
         * If we have any metering messages to send, send them now
         */
        pthread_spin_lock(&kafka_spin);
        if (kafka_publish && kafka_publish->state == ISM_TRANST_Open && kafka_first) {
            doProduce = checkMeteringBatch(0x7FFFFFFFFFFFFFFFL);
        }
        pthread_spin_unlock(&kafka_spin);
        if (doProduce) {
            ism_kafka_produceMetering(kafka_publish, doProduce);
        }
        usleep(500000);
        if (kafka_metadata) {
            kafka_metadata->close(kafka_metadata, ISMRC_ServerTerminating, 0, "The connection was closed because the proxy was shutdown.");
            kafka_metadata = NULL;
        }
        if (kafka_publish) {
            kafka_publish->close(kafka_publish, ISMRC_ServerTerminating, 0, "The connection was closed because the proxy was shutdown.");
            kafka_publish = NULL;
        }
    }
    return 0;
}


/*
 * On the TCP connection of the kafka metadata connection, send a metadata request
 */
static int kafkaMetadataConnected(ism_transport_t * transport, int rc) {
    char xbuf [1024];
    concat_alloc_t cbuf = {xbuf, sizeof xbuf, 4};
    concat_alloc_t * buf = &cbuf;
    transport->lastAccessTime = ism_common_readTSC();
    if (rc == 0) {
        TRACE(5, "kafka metadata connected: connect=%u broker=%s:%u host=%s\n",
                transport->index, transport->server_addr, transport->serverport,
                transport->client_host ? transport->client_host : "");
        transport->ready = 10;
        transport->state = ISM_TRANST_Open;
        ism_kafka_putInt4(buf,MetadataRequest0);
        ism_kafka_putInt4(buf, 0x10000);   /* Correlation ID */
        ism_kafka_putString(buf, ism_common_getHostnameInfo(), -1);   /* Use our hostname as the kafka clientID */
        ism_kafka_putInt4(buf, 1);         /* Count of topics */
        ism_kafka_putString(buf, kafka_metering_topic, -1);  /* Metering topic */
        transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);
    } else {
        /* The invoker will call close right after this, so nothing to do here */
        TRACE(7, "kafka metadata connecttion failed: connect=%u rc=%u\n", transport->index, rc);
    }
    return 0;
}


/*
 * Send Kafka Metadata Request for IM
 */
static int kafkaIMMessagingMetadataRequest(ism_transport_t * transport) {
    char xbuf [1024];
    concat_alloc_t cbuf = {xbuf, sizeof xbuf, 4};
    concat_alloc_t * buf = &cbuf;
	TRACE(5, "kafkaIMMessagingMetadataRequest: connect=%u broker=%s:%u host=%s\n",
			transport->index, transport->server_addr, transport->serverport,
			transport->client_host ? transport->client_host : "");
	ism_kafka_putInt4(buf,MetadataRequest0);
	ism_kafka_putInt4(buf, 0x20000);   /* Correlation ID */
	ism_kafka_putString(buf, ism_common_getHostnameInfo(), -1);   /* Use our hostname as the kafka clientID */
	ism_kafka_putInt4(buf, 1);         /* Count of topics */
	ism_kafka_putString(buf, transport->pobj->topic, -1);  /* IM Messaging topic */
	transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);
    return 0;
}

/*
 * On the TCP connection of the kafka metadata connection, send a metadata request
 */
static int kafkaIMMessagingMetadataConnected(ism_transport_t * transport, int rc) {
    char xbuf [1024];
    concat_alloc_t cbuf = {xbuf, sizeof xbuf, 4};
    concat_alloc_t * buf = &cbuf;
    transport->lastAccessTime = ism_common_readTSC();
    if (rc == 0) {
        TRACE(5, "kafkaIMMessagingMetadataConnected: kafka IMMessaging metadata connected: connect=%u broker=%s:%u host=%s\n",
                transport->index, transport->server_addr, transport->serverport,
                transport->client_host ? transport->client_host : "");
        transport->ready = 10;
        transport->state = ISM_TRANST_Open;
        transport->pobj->state= TCP_CONNECTED;
        ism_kafka_con_t * pobj = (ism_kafka_con_t *) transport->pobj;
        kafkaTopicRecord_t * topicRecord = pobj->topicRecord;
        pthread_spin_lock(&topicRecord->lock);
        topicRecord->connectionErrorCount=0;
        topicRecord->connectionErrorFirstTimeInNanos=0;
        pthread_spin_unlock(&topicRecord->lock);
        ism_kafka_putInt4(buf,MetadataRequest0);
        ism_kafka_putInt4(buf, 0x20000);   /* Correlation ID */
        ism_kafka_putString(buf, ism_common_getHostnameInfo(), -1);   /* Use our hostname as the kafka clientID */
        ism_kafka_putInt4(buf, 1);         /* Count of topics */
        ism_kafka_putString(buf, transport->pobj->topic, -1);  /* IM Messaging topic */
        transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);
    } else {
        /* The invoker will call close right after this, so nothing to do here */
        TRACE(5, "kafkaIMMessagingMetadataConnected: kafka metadata connection failed: connect=%u rc=%u\n", transport->index, rc);
    }
    return 0;
}


/*
 * Complete a kafka publisher connection.
 *
 * Kafka has not connect logic, so a soon as we get a connection with a good rc we can do a publish.
 */
static int kafkaConnected(ism_transport_t * transport, int rc) {
	 transport->lastAccessTime = ism_common_readTSC();
    if (rc == 0) {
        TRACE(5, "kafka producer connected: connect=%u node=%u broker=%s:%u host=%s partition=%u\n",
                transport->index, transport->pobj->nodeID, transport->server_addr, transport->serverport,
                transport->client_host ? transport->client_host : "", transport->pobj->partID);
        kafka_meter_t * doProduce;
        transport->ready = 10;
        transport->state = ISM_TRANST_Open;
        /* Check if we have a batch full of message waiting to be sent */
        pthread_spin_unlock(&kafka_spin);
        doProduce = checkMeteringBatch(ism_common_currentTimeNanos());
        pthread_spin_unlock(&kafka_spin);
        if (doProduce) {
            ism_kafka_produceMetering(kafka_publish, doProduce);
        }
    } else {
        /* The invoker will call close right after this, so nothing to do here */
        TRACE(7, "kafka producer connecttion failed: connect=%u rc=%u\n", transport->index, rc);
    }
    return 0;
}


/*
 * Fill in a server address
 */
static int getServerAddress(ism_transport_t * transport) {
    int rc;
    struct gaicb * req = {0};
    struct sigevent * sigevt;
    struct addrinfo * hints;

    req = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_kafka,3),1, sizeof(*req)+sizeof(*sigevt)+sizeof(*hints)+16);
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
        ism_common_free(ism_memory_proxy_kafka,transport->getAddrCB);
#ifdef GAI_SIG
        ism_common_removeUserHandler(handler);
#endif
        transport->getAddrCB = NULL;
        ism_common_setErrorData(ISMRC_Error, "%s%i", "getaddrinfo_a", rc);
        return ISMRC_Error;
    }
    return 0;
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
 */
static void  replaceString(const char * * oldval, const char * val) {
    if (!my_strcmp(*oldval, val))
        return;
    if (*oldval) {
        char * freeval = (char *)*oldval;
        if (val && !strcmp(freeval, val))
            return;
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
}

/*
 * Insert a slot in the kafka server table
 */
static int insertServerSlot(ism_server_t * server, int nodeid, const char * nodehost, int port) {
    if (nodeid >= 0 && nodehost && port) {
        int slot = nodeid % 32;
        int startslot = slot;
        while (server->node[slot] != -1) {
        	   if (server->node[slot] == nodeid) return slot;
            if (++slot == 32)
                slot = 0;
            if (slot == startslot) {
                return -1;
            }
        }
        server->node[slot] = nodeid;
        server->n_port[slot] = port;
        replaceString((const char * *)&server->nodeaddr[slot], nodehost);
        /* Set ipInfo and ip */
        return slot;
    }
    return -1;
}

/*
 * Find a nodeID in the slot table
 */
static int findSlot(ism_server_t * server, int nodeid) {
    int slot = nodeid % 32;
    int startslot = slot;
    while (server->node[slot] != nodeid) {
    		if (++slot == 32)
            slot = 0;
        if (slot == startslot) {
            return -1;
        }
    }
    return slot;
}


/*
 * Receive a kafka record for metadata
 */
static int receiveKafka(ism_transport_t * transport, char * inbuf, int buflen, int kind) {
    concat_alloc_t cbuf = {inbuf, buflen, buflen};
    concat_alloc_t * buf = &cbuf;
    int  i;
    int  partcount = 0;
    int  leaders[16];
    int  partids[16];
    int  rc = 0;

    transport->lastAccessTime = ism_common_readTSC();

    int  corrid = ism_kafka_getInt4(buf);
    if (corrid == 0x10000) {             /* Metadata */
        int broker_cnt = ism_kafka_getInt4(buf);
        pthread_mutex_lock(&kafka_lock);
        if (!kafka_needmetadata) {
            TRACE(3, "kafka metadata already processed\n");
            /* In the unlikely case we already just got the metadata, use what we have */
            if (!kafka_publish)
                ism_kafka_startMeteringPublishConnect(kafka_server);
            pthread_mutex_unlock(&kafka_lock);
            return 0;
        }
        for (i=0; i<32; i++) {
            kafka_server->node[i] = -1;
            replaceString(&kafka_server->nodeaddr[i], NULL);
        }
        for (i=0; i<broker_cnt; i++) {
            char * hoststr;
            int nodeid  = ism_kafka_getInt4(buf);
            int hostlen = ism_kafka_getString(buf, &hoststr);
            int port    = ism_kafka_getInt4(buf);
            if (ism_kafka_more(buf)<0 || hostlen==0 || port < 1 || port > 65535) {
                TRACE(5, "Kafka metadata incomplete parsing broker\n");
                rc = 1;
            } else {
                int slot = insertServerSlot(kafka_server, nodeid, hoststr, port);
                TRACE(7, "Kafka metadata for broker slot=%u node=%u %s:%u\n", slot, nodeid, hoststr, port);
            }
        }
        buf->pos += 4;    /* Assume 1 topic */
        if (rc == 0) {
            char * topicstr;
            int topicrc = ism_kafka_getInt2(buf);
            int topiclen = ism_kafka_getString(buf, &topicstr);
            int part_cnt = ism_kafka_getInt4(buf);
            TRACE(7, "Kafka metadata: topic=%s rc=%d part_count=%d\n", topicstr?topicstr:"", topicrc, part_cnt);
            if (ism_kafka_more(buf)<0 || part_cnt >= 0x1000000 || topiclen < 1 || topicrc != 0) {
                 TRACE(5, "Kafka metadata incomplete while parsing topic. parts=%u topiclen=%u rc=%d\n", part_cnt, topiclen, topicrc);
                 rc = 2;
            } else {
                for (i=0; i<part_cnt; i++) {
                    int isr;
                    int partrc =  ism_kafka_getInt2(buf);
                    int partid =  ism_kafka_getInt4(buf);
                    int leader =  ism_kafka_getInt4(buf);
                    int replica = ism_kafka_getInt4(buf) & 0xffff;
                    buf->pos += 4 * replica;   /* Skip over replica info */
                    isr =         ism_kafka_getInt4(buf) & 0xffff;
                    buf->pos += 4 * isr;       /* Skip over isr info */
                    if (ism_kafka_more(buf)<0) {
                         TRACE(5, "Kafka metadata incomplete while parsing partition info\n");
                    } else {
                        TRACE(7, "Kafka metadata: slot=%u partiton=%d rc=%d leader=%d replicas=%d isr=%d\n",
                                i, partid, partrc, leader, replica, isr);
                        if ((partrc == 0 || partrc == 9) && partcount<16) {
                            partids[partcount] = partid;
                            leaders[partcount] = leader;
                            partcount++;                    }
                    }
                }
            }
        }
        if (partcount == 0) {
            if (rc==0)
                TRACE(5, "Kafka metering server no valid leader found\n");
            pthread_mutex_unlock(&kafka_lock);
            transport->close(transport, ISMRC_ServerNotAvailable, 0, "Server not available");
        } else {
            /* Select part and leader */
            int which = kafka_part_crc % partcount;
            kafka_leader_part = partids[which];
            kafka_leader_node = leaders[which];
            kafka_needmetadata = 0;
            /* Start publish connection */
            if (!kafka_publish)
                ism_kafka_startMeteringPublishConnect(kafka_server);
            pthread_mutex_unlock(&kafka_lock);
        }

    } else {
        /* Process anything not a metadata request */
    }

    return 0;
}

/*
 * Start a kafka metadata on a timer
 */
static int startKafkaTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    TRACE(8, "Start kafka metadata timer\n");
    ism_kafka_startMeteringMetadataConnect(kafka_server);
    ism_common_cancelTimer(timer);
    return 0;
}

/*
 * Handle the disconnect of the kafka metadata connection.
 */
static int metadataDisconnected(ism_transport_t * transport, int rc, int clean, const char * reason) {
    ism_kafka_con_t * pobj = (ism_kafka_con_t *) transport->pobj;
    TRACE(4, "kafka metadata connection closed: connect=%u rc=%d reason=%s need=%u state=%u\n",
            transport->index, rc, reason, kafka_needmetadata, pobj->state);
    pthread_spin_lock(&pobj->server->lock);
    if (pobj->state == TCP_DISCONNECTED) {
        pthread_spin_unlock(&pobj->server->lock);
        TRACE(4, "kafka metadata state was disconnect\n");
        return 0;
    }
    pobj->state = TCP_DISCONNECTED;
    transport->ready = 0;
    pthread_spin_unlock(&pobj->server->lock);

    //If server is shutting down. No need to metadata request
    if(g_useKafka && rc != ISMRC_ServerTerminating){
		pthread_mutex_lock(&kafka_lock);
		kafka_metadata = NULL;
		if (kafka_needmetadata) {
			uint64_t retry_time = clean ? 100000000L : 5000000000L;  /* 0.1 sec or 5 sec */
			TRACE(8, "schedule kafka retry: %g\n", (double)retry_time/1e9);
			ism_common_setTimerOnce(ISM_TIMER_HIGH, startKafkaTimer, NULL, retry_time);
		}
		pthread_mutex_unlock(&kafka_lock);
    }
    transport->closed(transport);
    return 0;
}

/*
 * The publish connection is disconnected
 */
static int publishDisconnected(ism_transport_t * transport, int rc, int clean, const char * reason) {
    ism_kafka_con_t * pobj = (ism_kafka_con_t *) transport->pobj;
    TRACE(4, "kafka publish connection closed: connect=%u rc=%d reason=%s\n", transport->index, rc, reason);
    pthread_spin_lock(&pobj->server->lock);
    if (pobj->state == TCP_DISCONNECTED) {
        pthread_spin_unlock(&pobj->server->lock);
        return 0;
    }
    pobj->state = TCP_DISCONNECTED;
    transport->ready = 0;
    pthread_spin_unlock(&pobj->server->lock);

    //If server is shutting down. No need to metadata request
    if(g_useKafka && rc != ISMRC_ServerTerminating ){
		pthread_mutex_lock(&kafka_lock);
		kafka_publish = NULL;
		kafka_needmetadata = 1;

		/* If we have a metadata connection, request metadata */
		if (kafka_metadata) {
			/* TODO: schedule on metadata IOP */
			kafkaMetadataConnected(kafka_metadata, 0);
		} else {
			ism_common_setTimerOnce(ISM_TIMER_LOW, startKafkaTimer, NULL, 5000000000L);
		}
		pthread_mutex_unlock(&kafka_lock);
    }
    transport->closed(transport);
    return 0;
}

/*
 * Find the address for a kafka broker
 */
 int ism_kafka_getAddress(ism_server_t * server, ism_transport_t * transport,  ism_gotAddress_f gotAddress) {
    ism_kafka_con_t * pobj = (ism_kafka_con_t *)transport->pobj;
    int  slot=-1;
    int  last_good=-1;
    int  rc = 0;

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

    pthread_spin_lock(&server->lock);
    if (pobj->kafka_type == KafkaMetadata) {
        if (transport->connect_tried > server->ipaddr_count) {
            TRACE(4, "Unable to find a kafka metadata server: count=%d\n", server->ipaddr_count);
            ism_common_setError(ISMRC_UnableToConnect);
            rc = ISMRC_UnableToConnect;
        } else {
            last_good = server->last_good;
            slot = last_good + 1;

            if (slot >= server->ipaddr_count)
                slot -= server->ipaddr_count;
            server->last_good = slot;

            TRACE(5, "Try kafka metadata connection: slot=%u last_good=%u tried=%u count=%u\n",
                    slot, last_good, transport->connect_tried, server->ipaddr_count);
            transport->slotused = slot;
            transport->serverport = server->a_port[slot];
            transport->server_addr =  ism_transport_putString(transport, server->address[slot]);
            rc = getServerAddress(transport);
        }
    } else {
        slot = findSlot(server, pobj->nodeID);
        if (slot < 0) {
            TRACE(5, "Unable to find a kafka publishing broker: node=%d\n", pobj->nodeID);
            ism_common_setError(ISMRC_UnableToConnect);
            rc = ISMRC_UnableToConnect;
        } else {
            transport->serverport = server->n_port[slot];
            transport->server_addr = ism_transport_putString(transport, server->nodeaddr[slot]);
            rc = getServerAddress(transport);
        }
    }
    pthread_spin_unlock(&server->lock);
    return rc;
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
        if (grc == 0)
            grc = transport->slotused;
        transport->gotAddress(transport, grc, info);
        freeaddrinfo(info);
        ism_common_free(ism_memory_proxy_kafka,req);
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
    ism_common_free(ism_memory_proxy_kafka,req);           /* This includes the hints and sigevent */
}
#endif


/*
 * Start a kafka metadata
 */
int ism_kafka_startMeteringMetadataConnect(ism_server_t * server) {
    ism_transport_t * transport = ism_transport_newOutgoing(NULL, 1);
    TRACE(6, "Start kafka metadata connection\n");
    ism_tcp_init_transport(transport);
    transport->originated = 1;
    transport->protocol = "kafka_metadata";
    transport->protocol_family = "kafka";
    transport->tid = 0;
    transport->ready = 7;  //Set ready for Connect Timeout. See ddosTimer
    transport->connected = kafkaMetadataConnected;
    ism_kafka_con_t * pobj = (ism_kafka_con_t *) ism_transport_allocBytes(transport, sizeof(ism_kafka_con_t), 1);
    transport->pobj = pobj;
    transport->receive = receiveKafka;
    transport->closing = metadataDisconnected;
    pobj->server = server;
    pobj->transport = transport;
    transport->clientID = "kafka_metadata";
    transport->name = "kafka_metadata";
    pobj->state = TCP_CON_IN_PROCESS;
    pobj->nodeID = 0;
    pobj->kafka_type = KafkaMetadata;
    kafka_metadata = transport;
    kafka_needmetadata = 1;
    int rc = ism_kafka_createConnection(transport, server);
       if (rc) {
       	 	 char xbuf [2048];
       		TRACE(5, "ism_kafka_startMeteringMetadataConnect: Kafka Metadata Connection failed. server=%p\n", server);
   		ism_common_formatLastError(xbuf, sizeof xbuf);
   		ism_common_setError(rc);
   		transport->close(transport, rc, 0, xbuf);
   	}else{
   		TRACE(5, "ism_kafka_startMeteringMetadataConnect: Kafka Metadata Connection is started. server=%p\n", server);
   	}

    return rc;
}

/*
 * Start a kafka publish connection
 */
int ism_kafka_startMeteringPublishConnect(ism_server_t * server) {
    char xbuf[256];
    ism_transport_t * transport = ism_transport_newOutgoing(NULL, 1);
    ism_tcp_init_transport(transport);
    TRACE(6, "Start kafka publish connection\n");
    transport->originated = 1;
    transport->protocol = "kafka_publish";
    transport->protocol_family = "kafka";
    transport->tid = 0;
    transport->ready = 7;  //Set ready for Connect Timeout. See ddosTimer
    transport->connected = kafkaConnected;
    ism_kafka_con_t * pobj = (ism_kafka_con_t *) ism_transport_allocBytes(transport, sizeof(ism_kafka_con_t), 1);
    transport->pobj = pobj;
    transport->receive = receiveKafka;
    transport->closing = publishDisconnected;
    pobj->server = server;
    pobj->transport = transport;
    snprintf(xbuf, sizeof xbuf, "%s:%u:%u", kafka_metering_topic, kafka_leader_part, kafka_leader_node);
    transport->name = ism_transport_putString(transport, xbuf);
    transport->clientID = transport->name;
    transport->name = transport->clientID;
    pobj->state = TCP_CON_IN_PROCESS;
    pobj->kafka_type = KafkaMetering;
    pobj->nodeID = kafka_leader_node;
    pobj->partID = kafka_leader_part;
    kafka_publish = transport;
    int rc = ism_kafka_createConnection(transport, server);
	if (rc) {
		char ebuf [2048];
		TRACE(5, "ism_kafka_startMeteringPublishConnect: Kafka Data Connection failed. server=%p\n", server);
		ism_common_formatLastError(ebuf, sizeof ebuf);
		ism_common_setError(rc);
		transport->close(transport, rc, 0, ebuf);
	}else{
		TRACE(5, "ism_kafka_startMeteringPublishConnect: Kafka Data Connection is started. server=%p\n", server);
	}
   return rc;
}


/*
 * Return a string from a JSON entry even for things without a value.
 */
static const char * getJsonValue(ism_json_entry_t * ent) {
    if (!ent)
        return "";
    switch (ent->objtype) {
    case JSON_True:    return "true";
    case JSON_False:   return "false";
    case JSON_Null:    return "null";
    case JSON_Object:  return "object";
    case JSON_Array:   return "array";
    case JSON_Integer:
    case JSON_String:
    case JSON_Number:
        return ent->value;
    }
    return "";
}

/*
 * Set the kafka versions. This is set globally
 * Each Kafka API has a version, and the message format is yet another version.
 * It is common in kafka to update these independently.  However, we will only update
 * as required.
 *
 * The following Kafka API versions are suported
 * 0 = Original message format without timestamp, produce without thorttle time in response
 * 1 = Message format with timestamp, produce response with throttle time
 * 2 = Record batch message format, and produce response with throttle time
 */
int ism_kafka_setKafkaAPIVersion(int api_version) {
	int rc;
	if (api_version == 0 || api_version == 1 || api_version == 2) {
		TRACE(3, "setKafkaAPIVersion: old api_version=%d. new api_version=%d\n", kafka_api_version, api_version );
		kafka_api_version = (uint32_t)api_version;
		kafka_metadata_version = 0;
		switch (api_version) {
		case 0:
		    kafka_message_version = 0;
		    kafka_produce_version = 0;
		    break;
		case 1:   /* 0.10 */
		    kafka_message_version = 1;
            kafka_produce_version = 1;    /* Add throttle to produce response */
            break;
		case 2:   /* 0.11 */
		    kafka_message_version = 2;    /* Use record batch  */
            kafka_produce_version = 1;
            break;
		}
		rc = 0;
	} else {
		TRACE(3, "The kafka api version is not valid: api_version=%d. current version=%d\n",api_version, kafka_api_version );
		rc = 1;
	}
	return rc;

}


/*
 * Put a string for the Kafka key
 */
static void putKeyString(concat_alloc_t * buf, const char * str) {
    int len;
    char * bp;

    len = str ? strlen(str) : 0;
    if (buf->used + 2 + len > buf->len)
        ism_protocol_ensureBuffer(buf, 2 + len);
    bp = buf->buf + buf->used;
    bp[0] = (char)(len>>8);
    bp[1] = (char)len;
    if (len)
        memcpy(bp+2, str, len);
    buf->used += (2+len);
}


/*
 * Make a kafka event key
 * All of the items must be filled in except uuid.
 * If the uuid is not set, the uuid will be created
 */
void ism_kafka_makeEventKey(concat_alloc_t * buf, const char * org, const char * type, const char * id, const char * event,
        const char * fmt, const char * uuid_bin) {
    char xuuid [16];
    if (!uuid_bin) {
        uuid_bin = ism_common_newUUID(xuuid, 16, 17, 0);
    }
    if (buf->used + 16 > buf->len) {
        ism_protocol_ensureBuffer(buf, 16);
    }
    memcpy(buf->buf+buf->used, uuid_bin+8, 8);    /* LSV first */
    memcpy(buf->buf+buf->used+8, uuid_bin, 8);    /* MSV last */
    buf->used += 16;

    putKeyString(buf, org);
    putKeyString(buf, type);
    putKeyString(buf, id);
    putKeyString(buf, event);;
    putKeyString(buf, fmt);
}

/*
 * Check to see if a im messaging batch is full, and return the batch if it is.
 * This is called both when we sava a message and when start a kafka connection.
 */
static kafka_produce_msg_t *  checkIMEventBatch(kafkaConnectionRecord_t * kafkaConnectionRecord, ism_time_t now, int closing) {
	kafka_produce_msg_t * doProduce = NULL;
	kafka_produce_msg_t * produce_msg_first = NULL;
	//kafka_produce_msg_t * produce_msg_last = NULL;
	kafkaTopicRecord_t * topicRecord = kafkaConnectionRecord->topicRecord;


    if ( closing || (kafkaConnectionRecord->kafka_msg_first!=NULL && (kafkaConnectionRecord->kafka_msg_count > topicRecord->kafka_batch_size ||
        (now - kafkaConnectionRecord->kafka_msg_first_time) > topicRecord->kafka_batch_time ) )) {

        /*
         * If there is a message list that is in waiting for ACK state. Use that first
         * There is ackwai msgs. No need to produce further untilt he ackwait msgs are cleared
         */
        if (!closing && topicRecord->kafka_produce_ack && kafkaConnectionRecord->kafka_ackwait_msg_first!=NULL) {
        		TRACE(6, "checkIMEventBatch: Ackwait Msg is not NULL. Wait for ACK. PartID=%d NodeID=%d  TotalMsgs=%d\n", kafkaConnectionRecord->partID,
        					kafkaConnectionRecord->nodeID,kafkaConnectionRecord->kafka_msg_count );
            doProduce = NULL;
        } else {
            /* Need to support the case the queue had millions of messages. batch every batch size allow */
            if (kafkaConnectionRecord->kafka_msg_count > topicRecord->kafka_batch_size) {
                int count=0;
                kafka_produce_msg_t * msg = produce_msg_first = kafkaConnectionRecord->kafka_msg_first;
                kafka_produce_msg_t * msg_prev = NULL;

                int batch_size = topicRecord->kafka_batch_size;
                /* For RECOVERY case, need to put more messages into the packat */
                if (kafkaConnectionRecord->kafka_msg_count > (topicRecord->kafka_batch_size * 100)) {
                    batch_size = topicRecord->kafka_recovery_batch_size; //set Recovery Batch Size.
                }
                while (count <  batch_size ) {
                    msg_prev  = msg;
                    msg = msg->next;
                    count++;
                }
                if (msg_prev!=NULL)
                    msg_prev->next = NULL;
                kafkaConnectionRecord->kafka_msg_first  = msg;
                kafkaConnectionRecord->kafka_msg_count -= count;
                if (msg)
                    kafkaConnectionRecord->kafka_msg_first_time = msg->time;
                //produce_msg_last = msg_prev;
            } else {
                produce_msg_first = kafkaConnectionRecord->kafka_msg_first;
                //produce_msg_last = kafkaConnectionRecord->kafka_msg_last;
                kafkaConnectionRecord->kafka_msg_first = NULL;
                kafkaConnectionRecord->kafka_msg_last = NULL;
                kafkaConnectionRecord->kafka_msg_count =0;
            }
            doProduce = produce_msg_first;

            if (!closing && topicRecord->kafka_produce_ack) {
                //kafkaConnectionRecord->kafka_ackwait_msg_first = produce_msg_first;
                //kafkaConnectionRecord->kafka_ackwait_msg_last = produce_msg_last;
            		kafkaConnectionRecord->hasackwait=1;
            }

        }

    }
    return doProduce;
}




/*
 * Run a timer to make sure im messaging records per partition are sent even if there are no more produced.
 */
int kafkaIMMessagingPartitionProduceTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
	kafka_produce_msg_t * doProduce = NULL;
    ism_transport_t * transport=NULL;
    int producedMsgsCount=0;
    kafkaConnectionRecord_t * kafkaConnectionRecord = (kafkaConnectionRecord_t *)userdata;

	if (kafkaConnectionRecord) {
	    if (kafkaConnectionRecord->kafka_msg_count)
		    TRACE(7, "kafkaIMMessagingPartitionProduceTimer: Start processing. PartID=%d NodeID=%d MsgCount=%d\n", kafkaConnectionRecord->partID,
		    		kafkaConnectionRecord->nodeID, kafkaConnectionRecord->kafka_msg_count);
		pthread_mutex_lock(&kafkaConnectionRecord->lock);
		transport = kafkaConnectionRecord->produce_transport;
		if (transport!=NULL && transport->state == ISM_TRANST_Open ) {
			doProduce=checkIMEventBatch(kafkaConnectionRecord, ism_common_currentTimeNanos(), 0);
			if (doProduce) {
				ism_kafka_message_produce(transport, kafkaConnectionRecord, doProduce, &producedMsgsCount, 0);
				if(!kafkaConnectionRecord->topicRecord->kafka_produce_ack){
					kafkaConnectionRecord->stats.kakfaTotalPendingMsgsCount -= producedMsgsCount;
					//Update Overall Kafkaf Stats
					__sync_sub_and_fetch(&kafkaIMMessagingStats.kakfaTotalPendingMsgsCount, producedMsgsCount);
				}
				TRACE(7, "kafkaIMMessagingPartitionProduceTimer: Produced Msgs. PartID=%d NodeID=%d MsgCount=%d ProducedMsgCount=%d pendingMsgs=%llu\n", kafkaConnectionRecord->partID,
				    		kafkaConnectionRecord->nodeID,kafkaConnectionRecord->kafka_msg_count , producedMsgsCount,  (ULL)kafkaConnectionRecord->stats.kakfaTotalPendingMsgsCount);
			}else{
				TRACE(7, "kafkaIMMessagingPartitionProduceTimer: doProduce is NULL. PartID=%d NodeID=%d MsgCount=%d ProducedMsgCount=%d pendingMsgs=%llu\n", kafkaConnectionRecord->partID,
										    		kafkaConnectionRecord->nodeID,kafkaConnectionRecord->kafka_msg_count , producedMsgsCount,  (ULL)kafkaConnectionRecord->stats.kakfaTotalPendingMsgsCount);
			}
		}else{
			TRACE(7, "kafkaIMMessagingPartitionProduceTimer: Transport is not open. PartID=%d NodeID=%d MsgCount=%d ProducedMsgCount=%d pendingMsgs=%llu\n", kafkaConnectionRecord->partID,
						    		kafkaConnectionRecord->nodeID,kafkaConnectionRecord->kafka_msg_count , producedMsgsCount,  (ULL)kafkaConnectionRecord->stats.kakfaTotalPendingMsgsCount);
		}
		pthread_mutex_unlock(&kafkaConnectionRecord->lock);
		if (kafkaConnectionRecord->kafka_msg_count)
            TRACE(7, "kafkaIMMessagingPartitionProduceTimer: End. PartID=%d NodeID=%d MsgCount=%d\n", kafkaConnectionRecord->partID,
                kafkaConnectionRecord->nodeID, kafkaConnectionRecord->kafka_msg_count);
	}
    return 1;
}

static void stopKafkaIMMessaging(void)
{
    TRACE(5, "stopKafkaIMMessaging: start.\n");
    if(kafka_im_messaging_topic==NULL){
        TRACE(5, "stopKafkaIMMessaging: IM Messaging Topic is not set.\n");
        return;
    }

    kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(kafka_im_messaging_topic);
    if(topicRecord!=NULL){
        pthread_spin_lock(&topicRecord->lock);
        ism_transport_t * transport=topicRecord->metadata_transport;
        if(transport!=NULL){
            pthread_spin_unlock(&topicRecord->lock);
            transport->close(transport, ISMRC_ServerTerminating, 0, "Kafka IM Messaging is disabled");
            pthread_spin_lock(&topicRecord->lock);
            topicRecord->metadata_transport=NULL;
        }

        int connCount = topicRecord->connectionCount;


        for (int which = 0; which < connCount; which++){
            kafkaConnectionRecord_t * kafkaConnectionRecord =  ism_common_getArrayElement(topicRecord->partitionConnections, which+1);
            pthread_spin_unlock(&topicRecord->lock);

            if(kafkaConnectionRecord->produce_transport)
                kafkaConnectionRecord->produce_transport->close(kafkaConnectionRecord->produce_transport, ISMRC_ServerTerminating, 1, "Kafka IM Messaging is disabled");

            //Stop the produce timer
            pthread_mutex_lock(&kafkaConnectionRecord->lock);
            if (kafkaConnectionRecord->kafka_timer){
                ism_common_cancelTimer(kafkaConnectionRecord->kafka_timer);
                kafkaConnectionRecord->kafka_timer=NULL;
            }
            pthread_mutex_unlock(&kafkaConnectionRecord->lock);

            pthread_spin_lock(&topicRecord->lock);
        }
        pthread_spin_unlock(&topicRecord->lock);
    }
    TRACE(5, "stopKafkaIMMessaging: finished.\n");
}
/*
 * Change the setting of useIMKafka
 */
int ism_kafka_setUseKafkaIMMessaging(int useKafkaIMMessaging) {
    int oldkafkaimmessage = g_useKafkaIMMessaging;
    if (oldkafkaimmessage != useKafkaIMMessaging) {
        g_useKafkaIMMessaging = useKafkaIMMessaging;
        if (useKafkaIMMessaging) {
            TRACE(3, "Initialize kafka event messaging: useKafkaIMMessaging=%u useTLS=%u batch=%u time=%usec\n",
            		g_useKafkaIMMessaging, g_useKafkaTLS, kafka_batch_size, kafka_batch_seconds);
            ism_kafka_im_messaging_init(useKafkaIMMessaging);

            //Create Topic Record
            if (g_internalKafkaRoute && g_internalKafkaRoute->hosts!=NULL && g_internalKafkaRoute->kafka_topic!=NULL){
                ism_routing_route_t * route = g_internalKafkaRoute;
                kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(route->kafka_topic);
                if(topicRecord==NULL){
                    int krc = ism_kafka_createTopicRecord((const char *)route->hosts, (const char *)route->kafka_topic, NULL, (const char *)route->kafka_auth_id, (const char *)route->kafka_auth_pw,
                            route->useTLS, route->tlsMethod, route->tlsCiphers, route->kafka_batch_time_millis, route->kafka_batch_size, route->kafka_batch_size_bytes, route->kafka_recovery_batch_size, route->kafka_produce_ack,
                            route->kafka_dist_method, route->kafka_shutdown_onerror_time
                            );
                    if (krc) {
                        TRACE(3, "Failed to create Kafka Topic Record. topic=%s\n", route->kafka_topic);
                        useKafkaIMMessaging = 0;
                    } else {
                        TRACE(5, "Kafka Topic Record is created. topic=%s\n", route->kafka_topic);
                    }
            }else{
                //Restart
                ism_common_setTimerOnce(ISM_TIMER_HIGH, startKafkaIMMessagingTimer, ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),topicRecord->topic), 1);
            }
          }
        } else {
            TRACE(3, "Stop kafka event messaging\n");
            stopKafkaIMMessaging();
        }
    }

    return 0;
}


/*
 * Set the global kafka IM Messaging Default topic.
 * This can only be set once but can be set dynamically.
 */
int ism_kafka_setIMMessagingTopic(const char * topic) {
    if (topic) {
    	if (kafka_im_messaging_topic!=NULL)
    		ism_common_free(ism_memory_proxy_kafka_global,kafka_im_messaging_topic);
        kafka_im_messaging_topic = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka_global,1000),topic);
        TRACE(3, "Set kafka IM Messaging topic: %s\n", topic);
        /*Initialize the IM Messaging Topic in the Kafka IM Messaging Servers*/
        if (g_useKafkaIMMessaging && g_internalKafkaRoute!=NULL){
			ism_routing_route_t * route = g_internalKafkaRoute;
			if(route->kafka_topic!=NULL) ism_common_free(ism_memory_proxy_kafka,route->kafka_topic);
			route->kafka_topic = (char *) ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),topic);
			if(g_internalKafkaRoute->hosts!=NULL){
				int krc = ism_kafka_createTopicRecord((const char *)route->hosts, (const char *)route->kafka_topic, NULL, (const char *)route->kafka_auth_id, (const char *)route->kafka_auth_pw,
							route->useTLS, route->tlsMethod, route->tlsCiphers, route->kafka_batch_time_millis, route->kafka_batch_size,route->kafka_batch_size_bytes,
							 route->kafka_recovery_batch_size, route->kafka_produce_ack,
							route->kafka_dist_method, route->kafka_shutdown_onerror_time
							);
				if(krc){
					TRACE(5, "Failed to create Kafka Topic Record. topic=%s\n", route->kafka_topic);
				}else{
					TRACE(5, "Kafka Topic Record is created. topic=%s\n", route->kafka_topic);
				}
			}
		}

    }
    return 0;
}


/*
 * Init IM Messaging
 */
int ism_kafka_im_messaging_init(int useKafkaIMKessaging) {
	 const char * topic;

	 g_useKafkaIMMessaging = useKafkaIMKessaging;

	 if (g_useKafkaIMMessaging && !g_internalKafkaRoute) {
         if (g_routeconfig==NULL) {
             g_routeconfig = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_kafka_global,1),1, sizeof(ism_routing_config_t));
             ism_route_config_init(g_routeconfig);
             g_routeconfig->enabled = 1;
         }

		//pthread_spin_init(&kafkaPartitionConnections_lock, 0);
		/*IM Messaging Configuratins*/
		kafka_im_messaging_batch_millis = ism_common_getIntConfig("KafkaIMMessagingBatchTimeMillis", 250); //Default to 250 millis
		if (kafka_im_messaging_batch_millis < 1)
			kafka_im_messaging_batch_millis = 250;
		kafka_im_messaging_batch_time = kafka_im_messaging_batch_millis * 1000000L;
		kafka_im_messaging_batch_size = ism_common_getIntConfig("KafkaIMMessagingBatchSize", 200); //Default to 200 messages
		if (kafka_im_messaging_batch_size < 1)
			kafka_im_messaging_batch_size = 1;

		kafka_im_messaging_batch_size_bytes = ism_common_getIntConfig("KafkaIMMessagingBatchSizeBytes", kafka_im_messaging_batch_size_bytes);

		kafka_im_messaging_recovery_batch_size = ism_common_getIntConfig("KafkaIMMessagingRecoveryBatchSize", 0);

		/*The number of errors before print the Error Log. Default is 10*/
		kafka_im_produce_error_log_interval_count = ism_common_getIntConfig("KafkaIMProduceErrorLogIntervalCount", 10);

		g_useKafkaIMMessagingTLS = ism_common_getBooleanConfig("UseKafkaIMMessagingTLS", 0);
		if (g_useKafkaIMMessagingTLS) {
			const char * immsg_ciphers = ism_common_getStringConfig("KafkaIMMessagingCipher");
			const char * immsg_method = ism_common_getStringConfig("KafkaIMMessagingTLSMethod");
			if (immsg_ciphers && *immsg_ciphers) {
			   kafka_im_messaging_tls_cipher = immsg_ciphers;
			}
			if (immsg_method && *immsg_method) {
			   kafka_im_messaging_tls_method = immsg_method;
			}
		}

		g_checkKafkaIMMsgRouting = ism_common_getIntConfig("KafkaIMCheckMsgRouting", 0);
		kafka_im_produce_should_ack =ism_common_getIntConfig("KafkaIMProduceAck", 0); 				//Value is 0, 1, or -1
		kafka_im_shutdown_onerror_time =ism_common_getIntConfig("KafkaIMServerShutdownOnErrorTime", 300); //Value is in Seconds

		if (g_useKafkaTLS){
			TRACE(3, "Set kafka IMMessaging TLS: method=%s cipher=%s\n", kafka_im_messaging_tls_method, kafka_im_messaging_tls_cipher);
		}

		/**
		 * Distribution method
		 * 0 = Distribute to all partitions
		 * 1 = Divide partition evenly based on the msproxy instance index
		 */
		int partitionDistributionMethod = ism_common_getIntConfig("KafkaIMPartDistMethod", 0);
		TRACE(3, "Kafka IM Messaging Distribution Method: method=%d\n", partitionDistributionMethod);

		g_kafkaIMConnDetailStatsEnabled = ism_common_getIntConfig("KafkaIMConnDetailStatsEnabled", 0);

		//Initialize the Internal Kafka Route

		g_internalKafkaRoute = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_kafka_global,2),1,sizeof(ism_routing_route_t));
		g_internalKafkaRoute->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka_global,1000),"__InternalKafkaRoute__");
		g_internalKafkaRoute->route_type = ROUTE_TYPE_KAFKA;
		g_internalKafkaRoute->kafka_dist_method = partitionDistributionMethod;
		if (kafka_im_messaging_topic!=NULL)
		    g_internalKafkaRoute->kafka_topic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka_global,1000),kafka_im_messaging_topic);
		if (g_useKafkaIMMessagingTLS) {
			g_internalKafkaRoute->useTLS = g_useKafkaIMMessagingTLS;
			g_internalKafkaRoute->tlsMethod = (char *) kafka_im_messaging_tls_method;
			g_internalKafkaRoute->tlsCiphers = (char *)kafka_im_messaging_tls_cipher;
		}
		g_internalKafkaRoute->kafka_batch_size = kafka_im_messaging_batch_size;
		g_internalKafkaRoute->kafka_batch_size_bytes = kafka_im_messaging_batch_size_bytes;
		g_internalKafkaRoute->kafka_batch_time_millis = kafka_im_messaging_batch_millis;
		g_internalKafkaRoute->kafka_produce_ack = kafka_im_produce_should_ack;
		g_internalKafkaRoute->kafka_shutdown_onerror_time = kafka_im_shutdown_onerror_time;
		if(g_internalKafkaRoute->kafka_produce_ack != 0 && g_internalKafkaRoute->kafka_produce_ack != 1 && g_internalKafkaRoute->kafka_produce_ack!=-1){
			TRACE(5, "Kafka Produce Ack value is not valid. value=%d. Produce Ack will be disabled.\n",g_internalKafkaRoute->kafka_produce_ack );
			g_internalKafkaRoute->kafka_produce_ack=0;
		}
		g_internalKafkaRoute->kafka_recovery_batch_size = kafka_im_messaging_recovery_batch_size;

		if (kafka_im_messaging_hosts) {
			g_internalKafkaRoute->hosts = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka_global,1000),kafka_im_messaging_hosts);
		}

		if (!kafka_im_messaging_topic) {
            topic = ism_common_getStringConfig("KafkaIMMessagingTopic");
            if (topic) {
                ism_kafka_setIMMessagingTopic(topic);
            }
        }

        TRACE(3, "Initialize kafka IMMessaging: useKafka=%u useTLS=%u topic=%s batch=%u batchBytes=%d recovery_batch=%d time=%umilli\n",
                    g_useKafkaIMMessaging, g_useKafkaIMMessagingTLS, kafka_im_messaging_topic, kafka_im_messaging_batch_size,
					kafka_im_messaging_batch_size_bytes , kafka_im_messaging_recovery_batch_size, kafka_im_messaging_batch_millis);

		if (g_routeconfig && g_routeconfig->inited) {
            int i=0;
            ismHashMapEntry ** array;
            ism_routing_rule_t * routeRule;
            pthread_rwlock_rdlock(&g_routeconfig->lock);
            array = ism_common_getHashMapEntriesArray(g_routeconfig->routerulemap);
            while(array[i] != ((void*)-1)){
                routeRule = (ism_routing_rule_t *)array[i]->value;
                if(routeRule->routes){
                    int numRoute = ism_common_getArrayNumElements(routeRule->routes);

                    if(numRoute==0){
                        if(g_internalKafkaRoute!=NULL){
                            TRACE(5, "Assign default route to rule: rulename=%s routeName=%s\n",routeRule->name,  g_internalKafkaRoute->name);
                            ism_common_addToArray(routeRule->routes, g_internalKafkaRoute);
                        }else{
                            TRACE(5, "Assign default route to rule. global route is not set yet: rulename=%s\n",routeRule->name);
                        }
                    }

                }
                i++;
            }
            pthread_rwlock_unlock(&g_routeconfig->lock);
            ism_common_freeHashMapEntriesArray(array);
		}

	    return 0;
	 } else {
		 TRACE(3, "Kafka IM Messaging is disabled. useKafkaIMMessaging=%d\n", g_useKafkaIMMessaging);
		 //g_internalKafkaRoute=NULL;
	 }

	 return 1;

}

/* Complete a kafka publisher connection.
 *
 * Kafka has not connect logic, so a soon as we get a connection with a good rc we can do a publish.
 */
static int kafkaIMMessagingConnected(ism_transport_t * transport, int rc) {
   kafka_produce_msg_t * doProduce=NULL;

   transport->lastAccessTime = ism_common_readTSC();

   if (rc == 0) {
       TRACE(5, "kafkaIMMessagingConnected: kafka IM Messaging producer connected: connect=%u node=%u broker=%s:%u host=%s partition=%u\n",
               transport->index, transport->pobj->nodeID, transport->server_addr, transport->serverport,
               transport->client_host ? transport->client_host : "", transport->pobj->partID);
       //kafka_meter_t * doProduce;
       transport->ready = 10;
       transport->state = ISM_TRANST_Open;
       /* Check if we have a batch full of message waiting to be sent */
       ism_kafka_con_t * pobj = (ism_kafka_con_t *) transport->pobj;
       kafkaConnectionRecord_t * kafkaConnectionRecord = pobj->kafkaConnRecord;
       if(kafkaConnectionRecord){
    	   	   	pthread_mutex_lock(&kafkaConnectionRecord->lock);
			kafkaConnectionRecord->state = KAFKA_PARTITION_CONN_OPEN;
			kafkaConnectionRecord->stats.connected = 1;

			if(kafkaConnectionRecord->kafka_ackwait_msg_first!=NULL){
				doProduce=kafkaConnectionRecord->kafka_ackwait_msg_first;
				kafkaConnectionRecord->kafka_ackwait_msg_first=NULL;
				kafkaConnectionRecord->kafka_ackwait_msg_last=NULL;
			}else{
				doProduce=checkIMEventBatch(kafkaConnectionRecord, ism_common_currentTimeNanos(), 0);
			}
			if (doProduce) {
				 int producedMsgsCount=0;
				 ism_kafka_message_produce(transport, kafkaConnectionRecord, doProduce, &producedMsgsCount, 1);
				 if(!kafkaConnectionRecord->topicRecord->kafka_produce_ack){
					 kafkaConnectionRecord->stats.kakfaTotalPendingMsgsCount -= producedMsgsCount;
					 //Update Overall Kafkaf Stats
					 __sync_sub_and_fetch(&kafkaIMMessagingStats.kakfaTotalPendingMsgsCount, producedMsgsCount);
				 }
				 TRACE(5, "kafkaIMMessagingConnected: Produced Msgs: connect=%u producedMsgsCount=%d pendingMsgs=%llu\n", transport->index, producedMsgsCount,
						 (ULL)kafkaConnectionRecord->stats.kakfaTotalPendingMsgsCount);

			}
			if(kafkaConnectionRecord->kafka_timer==NULL){
				int time = kafkaConnectionRecord->topicRecord->kafka_batch_time_millis / 4;
				kafkaConnectionRecord->kafka_timer = ism_common_setTimerRate(ISM_TIMER_LOW, kafkaIMMessagingPartitionProduceTimer, kafkaConnectionRecord, kafkaConnectionRecord->topicRecord->kafka_batch_time_millis *5,
																			time, TS_MILLISECONDS);
			}

			pthread_mutex_unlock(&kafkaConnectionRecord->lock);
       }

   } else {
       /* The invoker will call close right after this, so nothing to do here */
       TRACE(5, "kafkaIMMessagingConnected: kafka IM Messaging producer connection failed: connect=%u rc=%u\n", transport->index, rc);
   }
   return 0;
}

int ism_kafka_setKafkaBrokerInternal(ism_json_entry_t * ents) {
	ism_json_entry_t * ent;
	int count = ents->count;
	int i;
	int  rc = 0;

	pthread_mutex_lock(&kafka_im_lock);
	ent = ents+1;
	for (i=0; i<count; i++) {
		if (ent->objtype != JSON_String || portValue(ent->value) <= 0) {
			ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ents->name, getJsonValue(ent));
			rc = ISMRC_BadPropertyValue;
			break;
		}
		ent++;
	}
	if (count > 32) {
		ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ents->name, "");
		rc = ISMRC_BadPropertyValue;
	}
	ent = ents+1;
	if (rc == 0) {
		char xxbuf[512];
		concat_alloc_t cpropbuf = {xxbuf, sizeof xxbuf,0};
		//kafka_im_messaging_hosts = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),ent->value);
		for (i=0; i<count; i++) {
			ism_common_allocBufferCopyLen(&cpropbuf,ent->value, strlen(ent->value));
			ism_common_allocBufferCopyLen(&cpropbuf,",", 1);
			ent++;
		}
		if(cpropbuf.used>0){
			kafka_im_messaging_hosts=(char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_kafka_global,3),cpropbuf.used + 1);
			memcpy(kafka_im_messaging_hosts, cpropbuf.buf, cpropbuf.used);
			kafka_im_messaging_hosts[cpropbuf.used]='\0';
		}
		if(cpropbuf.inheap){
			ism_common_freeAllocBuffer(&cpropbuf);
		}
		TRACE(5, "Internal Kafka Brokers=%s\n", kafka_im_messaging_hosts);
	}

	if(kafka_im_messaging_hosts && g_internalKafkaRoute){
		g_internalKafkaRoute->hosts = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka_global,1000),kafka_im_messaging_hosts);
		if (g_internalKafkaRoute->kafka_topic!=NULL){
			ism_routing_route_t * route = g_internalKafkaRoute;
			int krc = ism_kafka_createTopicRecord((const char *)route->hosts, (const char *)route->kafka_topic, NULL, (const char *)route->kafka_auth_id, (const char *)route->kafka_auth_pw,
					route->useTLS, route->tlsMethod, route->tlsCiphers, route->kafka_batch_time_millis, route->kafka_batch_size,route->kafka_batch_size_bytes, route->kafka_recovery_batch_size, route->kafka_produce_ack,
					route->kafka_dist_method, route->kafka_shutdown_onerror_time
					);
			if(krc){
				TRACE(5, "Failed to create Kafka Topic Record. topic=%s\n", route->kafka_topic);
			}else{
				TRACE(5, "Kafka Topic Record is created. topic=%s\n", route->kafka_topic);
			}

		}
	}

	pthread_mutex_unlock(&kafka_im_lock);
	return 0;

}


/*
 * Start a kafka metadata on a timer
 * @param timer
 * @param timestamp
 * @userdata the kafka Topic
 */
static int startKafkaIMMessagingTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    TRACE(5, "startKafkaIMMessagingTimer: Start kafka IM Messaging metadata timer\n");
    char * kafkaTopic = (char *)userdata;
    kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(kafkaTopic);
	if(topicRecord){
		pthread_spin_lock(&topicRecord->lock);
		if(topicRecord->needmetadata)
			ism_kafka_startEventMetadataConnect(topicRecord->server, topicRecord, topicRecord->authenticator, topicRecord->authenticator_data);
		pthread_spin_unlock(&topicRecord->lock);
	}else{
		TRACE(5, "startKafkaIMMessagingTimer: Failed to start. TopicRecord is NULL.\n");
	}

    ism_common_cancelTimer(timer);
    if(kafkaTopic!=NULL) ism_common_free(ism_memory_proxy_kafka,kafkaTopic);
    return 0;
}

/*
 * Request a kafka metadata on a timer
 * @param timer
 * @param timestamp
 * @userdata the kafka Topic
 */
static int requestMetadataKafkaIMMessagingTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    TRACE(5, "requestMetadataKafkaIMMessagingTimer: IM Messaging MetadataRequest timer\n");
    char * kafkaTopic = (char *)userdata;
    ism_transport_t * metadata_transport = NULL;
    kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(kafkaTopic);
	if(topicRecord){
		pthread_spin_lock(&topicRecord->lock);
		if(topicRecord->needmetadata){
			metadata_transport=topicRecord->metadata_transport;
			pthread_spin_unlock(&topicRecord->lock);
			if (metadata_transport!=NULL && !strcmp("kafka_metadata",metadata_transport->protocol) && metadata_transport->ready==10) {
				kafkaIMMessagingMetadataRequest(metadata_transport);
			}
		}else{
			pthread_spin_unlock(&topicRecord->lock);
		}
	}else{
		TRACE(5, "requestMetadataKafkaIMMessagingTimer: Failed to send metadata request. TopicRecord is NULL.\n");
	}

    ism_common_cancelTimer(timer);
    if(kafkaTopic!=NULL) ism_common_free(ism_memory_proxy_kafka,kafkaTopic);
    return 0;
}

/*
 * Request a kafka metadata on a timer
 * @param timer
 * @param timestamp
 * @userdata the kafka Topic
 */
static int requestShutdownTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    TRACE(5, "requestShutdownTimer: Shutting down msproxy.\n");
    ism_common_cancelTimer(timer);
    ism_common_shutdown(0);
    return 0;
}


/*
 * Free up a Kafka connection record
 */
static void freeKafkaConnectionRecord(void * ptr) {
	if(ptr){
		kafkaConnectionRecord_t * connectionRecord = (kafkaConnectionRecord_t *)ptr;
		pthread_mutex_lock(&connectionRecord->lock);
		if(connectionRecord->topic!=NULL)
			ism_common_free(ism_memory_proxy_kafka,connectionRecord->topic);
		if(connectionRecord->kafka_timer!=NULL)
			ism_common_cancelTimer(connectionRecord->kafka_timer);
		pthread_mutex_unlock(&connectionRecord->lock);
		pthread_mutex_destroy(&connectionRecord->lock);
		ism_common_free(ism_memory_proxy_kafka,connectionRecord);
	}

}


/*
 * Process the Partition data from Metadata
 */
static int processKafkaPartitionsFromMetadata(ism_transport_t * transport, char * topicstr, int partcount, int  * partids, int * leaders, int * needmetadata){
	int i=0;
	uint32_t           leader_part=-1;     /* Selected leader partition */
	uint32_t           leader_node=-1;        /* Selected leader node */

	kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(topicstr);

	pthread_spin_lock(&topicRecord->lock);

	topicRecord->needmetadata=0;


	if(topicRecord->topic_part_count !=  partcount){
		/*Received Partition Count is different.  Redo everything*/
		 TRACE(5, "processKafkaPartitionsFromMetadata: new partion count is different. oldcount=%d newcount=%d\n", topicRecord->topic_part_count, partcount);

		/* Select part and leader */
		int select_part_count = partcount;
		int startIndex = 0;

		/**
		 * Establish N Kafka Connections based on the number select partition count
		 */
		int kafka_im_messaging_connections_count_old = topicRecord->connectionCount;
		topicRecord->connectionCount = select_part_count;

		ismArray_t newKafkaPartitionConnections = ism_common_createArray(select_part_count+1);

		TRACE(5, "processKafkaPartitionsFromMetadata: newKafkaPartitionConn=%p partcount=%d selectedPartitionsCount=%d\n",
				 newKafkaPartitionConnections, partcount, select_part_count);

		for(i=0; i < select_part_count ; i++){

			kafkaConnectionRecord_t * kafkaConnRecord = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_kafka,4),1, sizeof(kafkaConnectionRecord_t));
			kafkaConnRecord->topic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),topicstr);
			kafkaConnRecord->topicRecord = topicRecord;
			kafkaConnRecord->index = ism_common_addToArray(newKafkaPartitionConnections, kafkaConnRecord);

			pthread_mutex_init(&kafkaConnRecord->lock, 0);

			int which = (startIndex+i) % partcount;
			/*Select a Partition and a leader node for this Connection*/
			leader_part = partids[which];
			leader_node = leaders[which];

			kafkaConnRecord->server = topicRecord->server;
			kafkaConnRecord->partID = leader_part;
			kafkaConnRecord->nodeID = leader_node;
			kafkaConnRecord->stats.partid = leader_part;
			kafkaConnRecord->stats.topic = kafkaConnRecord->topic;


			pthread_spin_unlock(&topicRecord->lock);

			/* Start publish connection */
			pthread_mutex_lock(&kafkaConnRecord->lock);
			ism_kafka_startEventProduceConnect(kafkaConnRecord, transport->pobj->authenticator, transport->pobj->authenticator_data);
			pthread_mutex_unlock(&kafkaConnRecord->lock);
			TRACE(5, "processKafkaPartitionsFromMetadata: startPublishIMMessagingConnect: kafkaConnRecord=%p server=%p transport=%p partition=%d partitionNode=%d\n",
										 kafkaConnRecord, kafka_im_messaging_server, kafkaConnRecord->produce_transport, leader_part, leader_node);
			pthread_spin_lock(&topicRecord->lock);

		}

		if (kafka_im_messaging_connections_count_old > 0){
			//There are still messages for the record. Transfer over to the new one

			int count= 0;
			int which =0;
			for(count=0; count < kafka_im_messaging_connections_count_old; count++){
				kafkaConnectionRecord_t * oldPartitionConnRecord =  ism_common_getArrayElement(topicRecord->partitionConnections, count+1);
				int connectionCount =  topicRecord->connectionCount;

				pthread_spin_unlock(&topicRecord->lock);

				TRACE(5, "processKafkaPartitionsFromMetadata: Process Old Connection:  kafkaPartitionConnections=%p kafkaConnRecord=%p server=%p transport=%p  partition=%d partitionNode=%d msgcount=%d\n",
						kafkaPartitionConnections, oldPartitionConnRecord, oldPartitionConnRecord->server,
						oldPartitionConnRecord->produce_transport, oldPartitionConnRecord->partID, oldPartitionConnRecord->nodeID, oldPartitionConnRecord->kafka_msg_count);


				pthread_mutex_lock(&oldPartitionConnRecord->lock);
				//Need to process the ACKWait Msgs
				kafka_produce_msg_t * old_msgs = oldPartitionConnRecord->kafka_ackwait_msg_first;
				int total_msg_transferred = 0;
				while (old_msgs) {
					kafka_produce_msg_t * msg;
					msg = old_msgs;
					old_msgs = old_msgs->next;
					which= msg->dcrc % connectionCount;
					kafkaConnectionRecord_t * newRecord =  ism_common_getArrayElement(newKafkaPartitionConnections, which+1);
					if (newRecord->kafka_msg_last)
						newRecord->kafka_msg_last->next = msg;
					newRecord->kafka_msg_last = msg;
					if (!newRecord->kafka_msg_first) {
						newRecord->kafka_msg_first = msg;
						newRecord->kafka_msg_first_time = msg->time;
						newRecord->kafka_msg_count = 1;
					} else {
						newRecord->kafka_msg_count++;
					}
					total_msg_transferred++;

				}
				TRACE(5, "processKafkaPartitionsFromMetadata: messages transferred for Ackwait:  kafkaPartitionConnections=%p kafkaConnRecord=%p server=%p transport=%p  partition=%d partitionNode=%d transferred=%d\n",
							kafkaPartitionConnections, oldPartitionConnRecord, oldPartitionConnRecord->server,
							oldPartitionConnRecord->produce_transport, oldPartitionConnRecord->partID, oldPartitionConnRecord->nodeID, total_msg_transferred);

				//Process msgs in the regular queue
				old_msgs = oldPartitionConnRecord->kafka_msg_first;
				total_msg_transferred = 0;
				while (old_msgs) {
					kafka_produce_msg_t * msg;
					msg = old_msgs;
					old_msgs = old_msgs->next;
					which= msg->dcrc % connectionCount;
					kafkaConnectionRecord_t * newRecord =  ism_common_getArrayElement(newKafkaPartitionConnections, which+1);
					if (newRecord->kafka_msg_last)
						newRecord->kafka_msg_last->next = msg;
					newRecord->kafka_msg_last = msg;
					if (!newRecord->kafka_msg_first) {
						newRecord->kafka_msg_first = msg;
						newRecord->kafka_msg_first_time = msg->time;
						newRecord->kafka_msg_count = 1;
					} else {
						newRecord->kafka_msg_count++;
					}
					total_msg_transferred++;

				}
				TRACE(5, "processKafkaPartitionsFromMetadata: messages transferred for pending queue:  kafkaPartitionConnections=%p kafkaConnRecord=%p server=%p transport=%p  partition=%d partitionNode=%d msgcount=%d transferred=%d\n",
								kafkaPartitionConnections, oldPartitionConnRecord, oldPartitionConnRecord->server,
								oldPartitionConnRecord->produce_transport, oldPartitionConnRecord->partID, oldPartitionConnRecord->nodeID, oldPartitionConnRecord->kafka_msg_count, total_msg_transferred);


				if(oldPartitionConnRecord->produce_transport){
					TRACE(5, "processKafkaPartitionsFromMetadata: Closing Kafka old Connection: kafkaPartitionConnections=%p kafkaConnRecord=%p partitionID=%d oldNodeID=%d\n", kafkaPartitionConnections, oldPartitionConnRecord,
							oldPartitionConnRecord->partID, oldPartitionConnRecord->nodeID);
					oldPartitionConnRecord->state = KAFKA_PARTITION_CONN_CLOSED;
					pthread_mutex_unlock(&oldPartitionConnRecord->lock);
					oldPartitionConnRecord->produce_transport->close(oldPartitionConnRecord->produce_transport, ISMRC_OK, 1, "Kafka Connection Close for partition change.");
					pthread_mutex_lock(&oldPartitionConnRecord->lock);

				}
				pthread_mutex_unlock(&oldPartitionConnRecord->lock);

				pthread_spin_lock(&topicRecord->lock);

			}


		}

		ismArray_t  oldkafkaPartitionConnections_delete = topicRecord->partitionConnections;
		topicRecord->partitionConnections = newKafkaPartitionConnections;
		topicRecord->topic_part_count = partcount;

		if(oldkafkaPartitionConnections_delete)
			ism_common_destroyArrayAndFreeValues(oldkafkaPartitionConnections_delete, freeKafkaConnectionRecord);


	}else{

		/*Same partition count. Check if the Transport is still valid and re-establish connection if needed. */
		for(i=0; i < topicRecord->connectionCount; i++){

			kafkaConnectionRecord_t * kafkaPartitionConnRecord =  ism_common_getArrayElement(topicRecord->partitionConnections, i+1);

			pthread_spin_unlock(&topicRecord->lock);

			int found_part=0;

			pthread_mutex_lock(&kafkaPartitionConnRecord->lock);
			/*Select a Partition and a leader node for this Connection*/
			for(int xcount=0; xcount<partcount; xcount++){
				if(partids[xcount]==kafkaPartitionConnRecord->partID){
					leader_part = partids[xcount];
					leader_node = leaders[xcount];
					found_part=1;
					break;
				}
			}

			//leader node of -1 means no leader yet. Skip or wait until there is a leader
			if(!found_part || leader_node==-1 ){
				pthread_mutex_unlock(&kafkaPartitionConnRecord->lock);
				//Need to set needmeta data so the metadata can be requested.
				if (leader_node == -1)
					*needmetadata = 1;
				continue;
			}

			kafkaPartitionConnRecord->partID = leader_part;

			/* If leader node had change, need to point the connection to that leader node*/
			if(kafkaPartitionConnRecord->nodeID != leader_node){

				/*Close the current connection*/
				if (kafkaPartitionConnRecord->produce_transport != NULL && (kafkaPartitionConnRecord->produce_transport->state == ISM_TRANST_Open ||
						kafkaPartitionConnRecord->produce_transport->state == ISM_TRANST_Opening) ){
					TRACE(5, "processKafkaPartitionsFromMetadata: Closing Kafka Existing Connection:kafkaPartitionConnections=%p kafkaConnRecord=%p partitionID=%d oldNodeID=%d leaderID=%d\nb", kafkaPartitionConnections, kafkaPartitionConnRecord,
							kafkaPartitionConnRecord->partID, kafkaPartitionConnRecord->nodeID, leader_node);
					kafkaPartitionConnRecord->state = KAFKA_PARTITION_CONN_CLOSED;
					pthread_mutex_unlock(&kafkaPartitionConnRecord->lock);
					kafkaPartitionConnRecord->produce_transport->close(kafkaPartitionConnRecord->produce_transport, ISMRC_OK, 1, "Kafka Connection Close for partition change.");
					pthread_mutex_lock(&kafkaPartitionConnRecord->lock);


				}
				pthread_spin_lock(&topicRecord->lock);
				kafkaPartitionConnRecord->server = topicRecord->server;
				pthread_spin_unlock(&topicRecord->lock);

				kafkaPartitionConnRecord->index = i;

				int oldNodeId = kafkaPartitionConnRecord->nodeID ;
				kafkaPartitionConnRecord->nodeID = leader_node;
				/* Start publish connection */
				ism_kafka_startEventProduceConnect(kafkaPartitionConnRecord, transport->pobj->authenticator, transport->pobj->authenticator_data);

				TRACE(5, "processKafkaPartitionsFromMetadata: Leader Node Changed:  kafkaPartitionConnections=%p kafkaConnRecord=%p server=%p transport=%p partition=%d partitionNode=%d oldPartitionNode=%d msgcount=%d\n",
						kafkaPartitionConnections, kafkaPartitionConnRecord, kafkaPartitionConnRecord->server,
						kafkaPartitionConnRecord->produce_transport,  kafkaPartitionConnRecord->partID,
						kafkaPartitionConnRecord->nodeID, oldNodeId, kafkaPartitionConnRecord->kafka_msg_count);





			}else{
				/*If leader is the same make sure the connection is still active. If not start a new connection*/
				if (kafkaPartitionConnRecord->produce_transport == NULL || (kafkaPartitionConnRecord->state != KAFKA_PARTITION_CONN_OPEN &&
							kafkaPartitionConnRecord->state != KAFKA_PARTITION_CONN_OPENING)){
					ism_kafka_startEventProduceConnect(kafkaPartitionConnRecord,  transport->pobj->authenticator, transport->pobj->authenticator_data);
					TRACE(5, "processKafkaPartitionsFromMetadata: ReEstablished Connection:  kafkaPartitionConnections=%p kafkaConnRecord=%p server=%p transport=%p partition=%d partitionNode=%d  msgcount=%d\n",
								kafkaPartitionConnections, kafkaPartitionConnRecord, kafkaPartitionConnRecord->server,
								kafkaPartitionConnRecord->produce_transport, kafkaPartitionConnRecord->partID,
								kafkaPartitionConnRecord->nodeID,  kafkaPartitionConnRecord->kafka_msg_count);

				}
			}

			pthread_mutex_unlock(&kafkaPartitionConnRecord->lock);

			pthread_spin_lock(&topicRecord->lock);

		}


	}
	pthread_spin_unlock(&topicRecord->lock);
	return 0;

}

/*
 * Receive a kafka record for metadata
 */
static int receiveKafkaMessage(ism_transport_t * transport, char * inbuf, int buflen, int kind) {
    concat_alloc_t cbuf = {inbuf, buflen, buflen};
    concat_alloc_t * buf = &cbuf;
    int  i;
    int  partcount = 0;
    int * partids_ptr=NULL;
    int * leaders_ptr=NULL;
    int  rc = 0;
    char * topicstr;
    kafka_broker_t * brokers=NULL;
    double receivedMsgTime = ism_common_readTSC();

    transport->lastAccessTime = receivedMsgTime;

    int  corrid = ism_kafka_getInt4(buf);

    /*
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
    if (corrid == 0x20000) {             /* Metadata */
        int broker_cnt = ism_kafka_getInt4(buf);
        pthread_mutex_lock(&kafka_im_lock);

        pthread_spin_lock(&transport->pobj->server->lock);
		for (i=0; i<32; i++) {
			transport->pobj->server->node[i] = -1;
			replaceString(&transport->pobj->server->nodeaddr[i], NULL);
		}
		pthread_spin_unlock(&transport->pobj->server->lock);

		brokers = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_kafka,5),broker_cnt , sizeof(kafka_broker_t));

		for (i=0; i<broker_cnt; i++) {
            char * hoststr;
            int nodeid  = ism_kafka_getInt4(buf);
            int hostlen = ism_kafka_getString(buf, &hoststr);
            int port    = ism_kafka_getInt4(buf);

            brokers[i].nodeid = nodeid;
            brokers[i].hostlen = hostlen;
            brokers[i].hoststr = alloca(hostlen+1);
            memcpy(brokers[i].hoststr,hoststr, hostlen);
            brokers[i].hoststr[hostlen] = '\0';
            brokers[i].port = port;

            if (ism_kafka_more(buf)<0 || hostlen==0 || port < 1 || port > 65535) {
                TRACE(5, "Kafka metadata incomplete parsing broker\n");
                rc = 1;
            } /*else {
                //int slot = insertServerSlot(kafka_im_messaging_server, nodeid, hoststr, port);
                int slot = insertServerSlot(transport->pobj->server, nodeid, hoststr, port);
                TRACE(7, "Kafka metadata for broker slot=%u node=%u %s:%u\n", slot, nodeid, hoststr, port);
            }*/
        }


        buf->pos += 4;    /* Assume 1 topic */
        if (rc == 0) {
            int topicrc = ism_kafka_getInt2(buf);
            int topiclen = ism_kafka_getString(buf, &topicstr);
            int part_cnt = ism_kafka_getInt4(buf);
            TRACE(5, "Kafka im messaging metadata: topic=%s rc=%d part_count=%d\n", topicstr?topicstr:"", topicrc, part_cnt);
            if (ism_kafka_more(buf)<0 || part_cnt >= 0x1000000 || topiclen < 1 || topicrc != 0) {
                 TRACE(5, "Kafka im messaging metadata incomplete while parsing topic. parts=%u topiclen=%u rc=%d\n", part_cnt, topiclen, topicrc);
                 rc = 2;
            } else {
            		partids_ptr = alloca(sizeof(int) * part_cnt);
            		leaders_ptr = alloca(sizeof(int) * part_cnt);
                for (i=0; i<part_cnt; i++) {
                    int isr;
                    int partrc =  ism_kafka_getInt2(buf);
                    int partid =  ism_kafka_getInt4(buf);
                    int leader =  ism_kafka_getInt4(buf);
                    int replica = ism_kafka_getInt4(buf) & 0xffff;
                    buf->pos += 4 * replica;   /* Skip over replica info */
                    isr =         ism_kafka_getInt4(buf) & 0xffff;
                    buf->pos += 4 * isr;       /* Skip over isr info */
                    if (ism_kafka_more(buf)<0) {
                         TRACE(5, "Kafka metadata incomplete while parsing partition info\n");
                    } else {
                        TRACE(5, "receiveIMMessage: Kafka IMMessaging metadata: slot=%u partiton=%d rc=%d leader=%d replicas=%d isr=%d\n",
                                i, partid, partrc, leader, replica, isr);

                        int partrc_check = 0;
                        if(partrc_check && (partrc != 0 && partrc!=9)){
                        		//For optimize configuration, if Partition RC check is enabled.
                        		//And rc is not zero or 9. Then skip the partition and allow
                        		//to change partition count
                        		continue;
                        }

                        partids_ptr[partcount] = partid;
						leaders_ptr[partcount] = leader;
						//Insert into the IP list for the leader node
						for (int bcount=0; bcount<broker_cnt; bcount++) {
								kafka_broker_t broker = brokers[bcount];
								if(broker.nodeid == leader){
									pthread_spin_lock(&transport->pobj->server->lock);
									int slot = insertServerSlot(transport->pobj->server, broker.nodeid, broker.hoststr, broker.port);
									pthread_spin_unlock(&transport->pobj->server->lock);
									TRACE(5, "Kafka broker for leader: brokertotal=%u slot=%u node=%u %s:%u\n", broker_cnt, slot, broker.nodeid, broker.hoststr, broker.port);
								}
						}

						partcount++;

                    }
                }

                //Fill Some node for metadata as well
                int metadatabrokercount = 32 - partcount;
                for (int bcount=0; bcount<metadatabrokercount && bcount<broker_cnt; bcount++) {
					kafka_broker_t broker = brokers[bcount];
					int slot = insertServerSlot(transport->pobj->server, broker.nodeid, broker.hoststr, broker.port);
					TRACE(7, "Kafka broker for metadata  brokertotal=%u slot=%u node=%u %s:%u\n", broker_cnt, slot, broker.nodeid, broker.hoststr, broker.port);

			   }
            }
        }
        pthread_mutex_unlock(&kafka_im_lock);
        if (partcount == 0) {
            if (rc==0)
                TRACE(5, "Kafka IM Messaging server no valid leader found\n");
            transport->close(transport, ISMRC_ServerNotAvailable, 0, "Server not available");
        } else {
			int needmetadata=0;
			processKafkaPartitionsFromMetadata(transport, topicstr, partcount, partids_ptr, leaders_ptr, &needmetadata);
			if(needmetadata){
				kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(topicstr);
				if(topicRecord!=NULL){
					pthread_spin_lock(&topicRecord->lock);
					topicRecord->needmetadata=1;
					pthread_spin_unlock(&topicRecord->lock);
					char * kafkaTopic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),topicstr);
					TRACE(5,"receiveKafkaMessage: MetaData Request: topic=%s\n", kafkaTopic);
					//Request Metadata  (one second)
					ism_common_setTimerOnce(ISM_TIMER_HIGH, requestMetadataKafkaIMMessagingTimer, kafkaTopic,  1000000000L);
				}
			}

        }

    }

    /* Parse Produce Response
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
    else  if (corrid == 1) {
        int topiclen = 0;;
        int partid = 0;;
        int res_rc = 0;
        int64_t offset = 0;
        xUNUSED int64_t timestamp = 0;    /* Server assigned timestamp: v2 and above */
        int throttle_time = 0;    /* Time the request was throttled due to quota: v1 and above */
        int found = 0;
        int needmetadata = 0;
        int produce_rc = 0;
        char * topic = NULL;
        ism_time_t currTimeInNanos = ism_common_currentTimeNanos();

        int response_cnt = ism_kafka_getInt4(buf);
        for(i=0; i< response_cnt; i++){
            found=0;
            int pcount=0;
            int connectionCount=0;
            int kafka_shutdown_onerror_time=0;
            ismArray_t partitionConnections;
            topiclen 	= ism_kafka_getString(buf, &topicstr);
            int part_cnt = ism_kafka_getInt4(buf);  //Get Partition Count
            kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(topicstr);

            pthread_spin_lock(&topicRecord->lock);
            connectionCount = topicRecord->connectionCount;
            partitionConnections = topicRecord->partitionConnections;
            topic = topicRecord->topic;
            kafka_shutdown_onerror_time = topicRecord->kafka_shutdown_onerror_time;
            pthread_spin_unlock(&topicRecord->lock);



            for (pcount=0; pcount<part_cnt; pcount++) {
                partid   	= ism_kafka_getInt4(buf);
                res_rc 		= ism_kafka_getInt2(buf);
                offset 		= ism_kafka_getInt8(buf);
                if (kafka_produce_version >= 2) {
                    timestamp = ism_kafka_getInt8(buf);
                }
                produce_rc = res_rc;
                TRACE(6, "KafkaProduceResponse: topiclen=%d topic=%s partid=%d rc=%d offset=%ld\n", topiclen, topicstr, partid, res_rc, offset);
                for(int count=0; count < connectionCount; count++){
                    kafkaConnectionRecord_t * partitionConnRecord =  ism_common_getArrayElement(partitionConnections, count+1);
                    TRACE(7, "KafkaProduceResponse: Process Connection Record for Response: kafkaPartitionConnections=%p kafkaConnRecord=%p server=%p transport=%p partition=%d partitionNode=%d msgcount=%d\n",
                    			partitionConnections, partitionConnRecord, partitionConnRecord->server,
                            partitionConnRecord->produce_transport, partitionConnRecord->partID, partitionConnRecord->nodeID, partitionConnRecord->kafka_msg_count);


                    pthread_mutex_lock(&partitionConnRecord->lock);
                    if(partitionConnRecord->partID == partid){
                        TRACE(6, "KafkaProduceResponse: Found and Set ACKWait: kafkaPartitionConnections=%p kafkaConnRecord=%p server=%p transport=%p partition=%d partitionNode=%d msgcount=%d\n",
                                partitionConnections, partitionConnRecord, partitionConnRecord->server,
                                partitionConnRecord->produce_transport, partitionConnRecord->partID, partitionConnRecord->nodeID, partitionConnRecord->kafka_msg_count);

                        kafka_produce_msg_t * msgs = partitionConnRecord->kafka_ackwait_msg_first;
                        partitionConnRecord->stats.kakfaTotalBatchProduceAckReceivedCount++;
						__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaTotalBatchProduceAckReceivedCount, 1);
                        partitionConnRecord->produceLastRC=res_rc;
					   partitionConnRecord->sumProduceLatency += (receivedMsgTime - partitionConnRecord->lastProduceTime);

					   if(res_rc == KAFKA_ERROR_NOERROR){
                            partitionConnRecord->kafka_ackwait_msg_first=NULL;
                            partitionConnRecord->kafka_ackwait_msg_last=NULL;
                            int msgcnt=0;
                            while (msgs) {
                                kafka_produce_msg_t * msg;
                                msg = msgs;
                                msgs = msgs->next;
                                msgcnt++;
                                freeIMEvent(msg);

                            }
                            partitionConnRecord->stats.kakfaTotalPendingMsgsCount -= msgcnt;
                            //Reset produceErrorCount
                            partitionConnRecord->produceErrorCount=0;
                            partitionConnRecord->produceErrorTotalCount=0;
                            partitionConnRecord->produceErrorFirstTimeInNanos = 0;
                            //Update Overall Kafkaf Stats
							__sync_sub_and_fetch(&kafkaIMMessagingStats.kakfaTotalPendingMsgsCount, msgcnt);
                            TRACE(6, "KafkaProduceResponse: AckWait Messages Removed: kafkaPartitionConnections=%p kafkaConnRecord=%p server=%p transport=%p  partition=%d partitionNode=%d removedMsgCount=%d pendMsgs=%llu lastOffset=%ld\n",
                                    			partitionConnections, partitionConnRecord, partitionConnRecord->server,
                                            partitionConnRecord->produce_transport, partitionConnRecord->partID, partitionConnRecord->nodeID, msgcnt,
                                            (ULL) partitionConnRecord->stats.kakfaTotalPendingMsgsCount, offset);
                        } else if (res_rc==KAFKA_ERROR_UNKNOWTOPICORPARTITION || res_rc==KAFKA_ERROR_LEADERNOTAVAILABLE ||
                                   res_rc==KAFKA_ERROR_NOTLEADERFORPARTITION || res_rc == KAFKA_ERROR_REQUESTTIMEOUT ||
                                   res_rc==KAFKA_ERROR_NOTENOUGHREPLICAS || res_rc==KAFKA_ERROR_NOTENOUGHREPLICASAFTERAPPEND){
                            //Request updated Metadata. These might be the cases of leadership change.
                        	   pthread_spin_lock(&topicRecord->lock);
                            topicRecord->needmetadata=1;
                            pthread_spin_unlock(&topicRecord->lock);
                            needmetadata = 1;
                            partitionConnRecord->produceErrorCount++;
                            partitionConnRecord->produceErrorTotalCount++;
							partitionConnRecord->stats.kakfaTotalBatchProduceAckErrorCount++;
							__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaTotalBatchProduceAckErrorCount, 1);


                            /*Close the current connection, and RETRY TO CONNECT AND rePRODUCE.*/
                            if (partitionConnRecord->produce_transport != NULL){
                                TRACE(5, "Produce Response Error: Closing Kafka Existing Connection: kafkaConnRecord=%p topic=%s partitionID=%d leaderID=%d ResponseRC=%d\n", partitionConnRecord,
                                        topic , partitionConnRecord->partID, partitionConnRecord->nodeID,res_rc);
                                partitionConnRecord->state = KAFKA_PARTITION_CONN_CLOSED;
                                pthread_mutex_unlock(&partitionConnRecord->lock);
                                partitionConnRecord->produce_transport->close(partitionConnRecord->produce_transport, ISMRC_OK, 1, "Kafka Connection Close for Produce Error.");
                                pthread_mutex_lock(&partitionConnRecord->lock);

                            }


                        }else{
                            //Other Error. No Retries. Keep counting error and time
                            TRACE(5, "KafkaProduceResponse: Produce Response Error. PartID=%d NodeID=%d MsgCount=%d lastOffset=%ld res_rc=%d\n", partitionConnRecord->partID,
                                                                    partitionConnRecord->nodeID,partitionConnRecord->kafka_msg_count , offset, res_rc);
                            partitionConnRecord->produceErrorCount++;
                            partitionConnRecord->produceErrorTotalCount++;
							partitionConnRecord->stats.kakfaTotalBatchProduceAckErrorCount++;
							__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaTotalBatchProduceAckErrorCount, 1);
                        }

                        if(res_rc != KAFKA_ERROR_NOERROR && partitionConnRecord->produceErrorFirstTimeInNanos==0){
                            //Set the Error Initial Time.
                            partitionConnRecord->produceErrorFirstTimeInNanos= currTimeInNanos;
                        }

                        //LOG the Produce Error.BAO
                        if(partitionConnRecord->produceErrorCount>kafka_im_produce_error_log_interval_count){
                            partitionConnRecord->produceErrorCount=0;
                            char tbuf[64];
                            ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
                            ism_common_setTimestamp(ts,partitionConnRecord->produceErrorFirstTimeInNanos);
                            ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 7, ISM_TFF_ISO8601);
                            ism_common_closeTimestamp(ts);
                            LOG(ERROR, Connection, 2601, "%-s%d%d%d%d%-s", "Kafka produce response with Error received. Topic={0} partition={1} leader={2} errorTotal={3} lastRC={4} errorStartTime={5}.",
                                    topic, partitionConnRecord->partID, partitionConnRecord->nodeID, partitionConnRecord->produceErrorTotalCount,
                                    partitionConnRecord->produceLastRC, tbuf);
                        }

                        //Check if there is maximum allowed time for Error before Shutdown
                        if(partitionConnRecord->produceErrorTotalCount>0 && kafka_shutdown_onerror_time>0){
                            ism_time_t maxTimeInNanos = kafka_shutdown_onerror_time * BILLION;
                            if(partitionConnRecord->produceErrorTotalCount && ((currTimeInNanos - partitionConnRecord->produceErrorFirstTimeInNanos) > maxTimeInNanos )){
                                //Still Having Errors after max time. Need to shutdown msproxy
                                char tbuf[64];
                                ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
                                ism_common_setTimestamp(ts, partitionConnRecord->produceErrorFirstTimeInNanos);
                                ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 7, ISM_TFF_ISO8601);
                                ism_common_closeTimestamp(ts);
                                LOG(ERROR, Connection, 2602, "%-s%d%d%d%d%-s", "Kafka Produce Response Error time is more than maximum time allowed. Shutting down msproxy. Topic={0} partition={1} leader={2} errorTotal={3} lastRC={4} errorStartTime={5}.",
                                                                        topic, partitionConnRecord->partID, partitionConnRecord->nodeID, partitionConnRecord->produceErrorTotalCount,
                                                                        partitionConnRecord->produceLastRC, tbuf);

                                ism_common_setTimerOnce(ISM_TIMER_HIGH, requestShutdownTimer, NULL,  2000000000L);
                            }
                        }


                        //If First Error Time beyond the limit. Shutdown proxy

                        found=1;
                    }
                    pthread_mutex_unlock(&partitionConnRecord->lock);
                    if(found) break;
                }
            }


            if(produce_rc > 0 && needmetadata){
                char * kafkaTopic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),topicstr);
                TRACE(5,"KafkaProduceResponse: Produce Response Error. Request new metadata. topic=%s error=%d\n", kafkaTopic,produce_rc);
                //Request Metadata  (one second)
                ism_common_setTimerOnce(ISM_TIMER_HIGH, requestMetadataKafkaIMMessagingTimer, kafkaTopic,  1000000000L);
            }


        }

        if (kafka_produce_version >= 1) {
            throttle_time = ism_kafka_getInt4(buf);    /* We do not use this */
            if (throttle_time != 0)
                TRACE(6, "KafkaProduceResponse: throttle=%u\n", (uint32_t)throttle_time);
        }
    } else {
        TRACE(5, "receiveKafkaMessage: Unknown Correlation ID=%d\n", corrid);
    }


    if(brokers!=NULL){
		ism_common_free(ism_memory_proxy_kafka,brokers);
		brokers=NULL;
	}
    return 0;
}


/*
 * Handle the disconnect of the kafka metadata connection.
 */
static int metadataIMMessagingDisconnected(ism_transport_t * transport, int rc, int clean, const char * reason) {
    ism_kafka_con_t * pobj = (ism_kafka_con_t *) transport->pobj;
    ism_time_t currTimeInNanos = ism_common_currentTimeNanos();
    int kafka_shutdown_onerror_time=0;

 	TRACE(4, "metadataIMMessagingDisconnected: kafka IM Messaging metadata connection closed: transport=%p connect=%u rc=%d reason=%s pobj->state=%u\n",transport,  transport->index, rc, reason, pobj->state);

    // get the topic record
	kafkaTopicRecord_t * topicRecord = pobj->topicRecord;
   	pthread_spin_lock(&pobj->server->lock);

   	if (rc != 0 && pobj->state == TCP_CON_IN_PROCESS) {

   	    /*
   	     * if the connection is still in progress we need to determine
   	     * if this is the first time we've detected this. If it is we
   	     * will record the current time. If not we need to check if the
   	     * maximum allowable time has passed for not being able to connect.
   	     * If the maximum allowable time has passed we will shutdown the proxy.
   	     */
        pthread_spin_unlock(&pobj->server->lock);
        pthread_spin_lock(&topicRecord->lock);
        TRACE(5,"metadataIMMessagingDisconnected: TCP connection in progress: metadata_transport=%p topic=%s\n", topicRecord->metadata_transport, topicRecord->topic);

        //Check if there is maximum allowed time for Error before Shutdown
        if(topicRecord->connectionErrorCount>0) {
            /*
             * Not being able to connect has been detected before. We will check
             * if the maximum allowable time has passed.
             */
            TRACE(5,"metadataIMMessagingDisconnected: More than one connection error has occurred: %d\n", topicRecord->connectionErrorCount);
            kafka_shutdown_onerror_time = topicRecord->kafka_shutdown_onerror_time;
       	    ism_time_t maxTimeInNanos = kafka_shutdown_onerror_time * BILLION;
            if((currTimeInNanos - topicRecord->connectionErrorFirstTimeInNanos) > maxTimeInNanos){
                //Still Having Errors after max time. Need to shutdown msproxy
                char tbuf[64];
                ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
                ism_common_setTimestamp(ts, topicRecord->connectionErrorFirstTimeInNanos);
                ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 7, ISM_TFF_ISO8601);
                ism_common_closeTimestamp(ts);
                LOG(ERROR, Connection, 2603, "%-s%d%-s", "Kafka Meta-Data Response Error time is more than maximum time allowed. Shutting down msproxy. Topic={0}  errorTotal={1}  errorStartTime={2}.",
                            topicRecord->topic, topicRecord->connectionErrorCount, tbuf);

                ism_common_setTimerOnce(ISM_TIMER_HIGH, requestShutdownTimer, NULL,  2000000000L);
            }
        } else {
        	// first time we detected connection error - record time
            topicRecord->connectionErrorFirstTimeInNanos = currTimeInNanos;
        }
        // make sure we indicate we saw a connection error
        topicRecord->connectionErrorCount++;

        pthread_spin_unlock(&topicRecord->lock);
        pthread_spin_lock(&pobj->server->lock);

   	} else if (pobj->state == TCP_DISCONNECTED) {
        pthread_spin_unlock(&pobj->server->lock);
        TRACE(4, "kafka im messaging metadata state was disconnect\n");
        return 0;
    }
    pobj->state = TCP_DISCONNECTED;
    transport->ready = 0;
    pthread_spin_unlock(&pobj->server->lock);

    pthread_spin_lock(&topicRecord->lock);
    topicRecord->metadata_transport = NULL;
    topicRecord->needmetadata = 1;
    pthread_spin_unlock(&topicRecord->lock);


    //If closing not because server shutdown, Try to reconnect to metadata
    if(rc!= ISMRC_ServerTerminating && g_useKafkaIMMessaging){
        char * kafkaTopic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),pobj->topic);

        if (kafkaTopic) {
            uint64_t retry_time = clean ? 100000000L : 5000000000L;  /* 0.1 sec or 5 sec */
            TRACE(5, "metadataIMMessagingDisconnected: schedule kafka im messaging retry: %g\n", (double)retry_time/1e9);
            ism_common_setTimerOnce(ISM_TIMER_HIGH, startKafkaIMMessagingTimer, kafkaTopic, retry_time);
        } else {
            TRACE(5, "metadataIMMessagingDisconnected : Failed to schedule metadata timer for im messaging. topic=%s\n",  kafkaTopic );
            if(kafkaTopic) ism_common_free(ism_memory_proxy_kafka,kafkaTopic);
        }

    }

    transport->closed(transport);
    return 0;
}


/*
 * The publish connection is disconnected
 */
static int publishIMMessagingDisconnected(ism_transport_t * transport, int rc, int clean, const char * reason) {
    ism_kafka_con_t * pobj = (ism_kafka_con_t *) transport->pobj;
    kafkaConnectionRecord_t * kafkaConnectionRecord = pobj->kafkaConnRecord;
    TRACE(4, "publishIMMessagingDisconnected: kafka IM Messaging publish connection closed: transport=%p connect=%u rc=%d reason=%s\n",transport,  transport->index, rc, reason);
    char * kafkaTopic = NULL;
    kafka_produce_msg_t * doProduce=NULL;
    ism_transport_t * metadata_transport = NULL;
    int produceRC=0;
    ism_time_t currTimeInNanos = ism_common_currentTimeNanos();
    int kafka_shutdown_onerror_time=0;
    int partID;
    int nodeID;
    int produceLastRC;

    pthread_mutex_lock(&kafkaConnectionRecord->lock);

    partID = kafkaConnectionRecord->partID;
    nodeID = kafkaConnectionRecord->nodeID;
    produceLastRC = kafkaConnectionRecord->produceLastRC;

    // get the topic record 
    kafkaTopicRecord_t * topicRecord = kafkaConnectionRecord->topicRecord;

    if(kafkaConnectionRecord->state != KAFKA_PARTITION_CONN_CLOSED){
        kafkaConnectionRecord->state = KAFKA_PARTITION_CONN_CLOSED;
        kafkaConnectionRecord->stats.connected = 0;
        kafkaConnectionRecord->produce_transport=NULL;

	    pthread_spin_lock(&topicRecord->lock);

	    TRACE(5,"publishIMMessagingDisconnected: ConnectionRecord=%p partID=%d topicRecord=%p topic=%s metadata_transport=%p msgsInQueue=%d\n", kafkaConnectionRecord, kafkaConnectionRecord->partID,
                                               topicRecord, topicRecord->topic,topicRecord->metadata_transport , kafkaConnectionRecord->kafka_msg_count);
		pthread_spin_unlock(&topicRecord->lock);

        if(rc== ISMRC_ServerTerminating){
			//Msproxy is shutdown. Need to send all messages out.

			{
				doProduce=checkIMEventBatch(kafkaConnectionRecord, ism_common_currentTimeNanos(), 1);

				if (doProduce) {
					 int producedMsgsCount=0;
					 produceRC = ism_kafka_message_produce(transport, kafkaConnectionRecord, doProduce, &producedMsgsCount, 0);
					 kafkaConnectionRecord->stats.kakfaTotalPendingMsgsCount -= producedMsgsCount;
					 //Update Overall Kafkaf Stats
					 __sync_sub_and_fetch(&kafkaIMMessagingStats.kakfaTotalPendingMsgsCount, producedMsgsCount);
					TRACE(5, "publishIMMessagingDisconnected: Produced Msgs: connect=%u producedMsgsCount=%d pendingMsgs=%d produceRC=%d\n", transport->index, producedMsgsCount,
							kafkaConnectionRecord->kafka_msg_count, produceRC);

				}
			} while (kafkaConnectionRecord->kafka_msg_count>0);

			TRACE(5, "publishIMMessagingDisconnected: flush kafka messages during shutdown completed: ConnectionRecord=%p\n", kafkaConnectionRecord);

		}else if(g_useKafkaIMMessaging){
			//If the connection terminated by network, then try to get the metadata again
			pthread_spin_lock(&topicRecord->lock);
			kafkaTopic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),topicRecord->topic);
			topicRecord->needmetadata=1;
			metadata_transport=topicRecord->metadata_transport;
			pthread_spin_unlock(&topicRecord->lock);

			/* If we have a metadata connection, request metadata */
			if (metadata_transport!=NULL && !strcmp("kafka_metadata",metadata_transport->protocol) && metadata_transport->ready==10) {
				TRACE(5,"publishIMMessagingDisconnected: MetaData Request: ConnectionRecord=%p partID=%d metadata_transport=%p topic=%s\n", kafkaConnectionRecord, kafkaConnectionRecord->partID, metadata_transport, kafkaTopic);
				ism_common_setTimerOnce(ISM_TIMER_HIGH, requestMetadataKafkaIMMessagingTimer, kafkaTopic,  100000000L);
			} else {
				//Check if the metadata connection is in progress. If yes, then no need to create another request to create the metadata connection
				if(metadata_transport==NULL || (metadata_transport!=NULL &&  metadata_transport->pobj!=NULL &&  metadata_transport->pobj->state != TCP_CON_IN_PROCESS)){
					TRACE(5,"publishIMMessagingDisconnected: Start Metadata Connection: ConnectionRecord=%p partID=%d topic=%s\n", kafkaConnectionRecord, kafkaConnectionRecord->partID, kafkaTopic);
					ism_common_setTimerOnce(ISM_TIMER_HIGH, startKafkaIMMessagingTimer, kafkaTopic,  100000000L);
				}else{
					TRACE(5,"publishIMMessagingDisconnected: Metadata Connection is in progress: ConnectionRecord=%p  partID=%d\n", kafkaConnectionRecord, kafkaConnectionRecord->partID);
				}
			}
		}

   	}

   	pthread_mutex_unlock(&kafkaConnectionRecord->lock);

   	pthread_spin_lock(&pobj->server->lock);
   	
    if (pobj->state == TCP_DISCONNECTED) {
		TRACE(5,"publishIMMessagingDisconnected: TCP already disconnected: ConnectionRecord=%p partID=%d metadata_transport=%p topic=%s\n", kafkaConnectionRecord, partID, metadata_transport, kafkaTopic);
		pthread_spin_unlock(&pobj->server->lock);
		return 0;
	}
	pobj->state = TCP_DISCONNECTED;
	transport->ready = 0;

	pthread_spin_unlock(&pobj->server->lock);

    transport->closed(transport);
    return 0;
}


/*
 * Start a kafka metadata for events
 * Note: This function is called with TopicRecord Lock. Need to unlock when do transport->close
 */
int ism_kafka_startEventMetadataConnect(ism_server_t * server, kafkaTopicRecord_t * topicRecord , ism_kafka_authenticator_t  authenticator, void * authenticator_data) {
    ism_transport_t * transport = ism_transport_newOutgoing(NULL, 1);
    TRACE(7, "ism_kafka_startEventMetadataConnect: Start kafka IM Messaging metadata connection\n");
    ism_tcp_init_transport(transport);
    transport->originated = 1;
    transport->protocol = "kafka_metadata";
    transport->protocol_family = "kafka";
    transport->tid = 0;
    transport->ready = 7;  //Set ready for Connect Timeout. See ddosTimer
    transport->connected = ism_kafka_authenticate;
    ism_kafka_con_t * pobj = (ism_kafka_con_t *) ism_transport_allocBytes(transport, sizeof(ism_kafka_con_t), 1);
    pobj->topic = topicRecord->topic;
    pobj->topicRecord = topicRecord;
    transport->pobj = pobj;
    transport->receive = receiveKafkaMessage;
    transport->closing = metadataIMMessagingDisconnected;
    pobj->server = server;
    pobj->transport = transport;
    transport->clientID = "kafka_im_messaging_metadata";
    transport->name = "kafka_im_messaging_metadata";
    pobj->state = TCP_CON_IN_PROCESS;
    pobj->nodeID = 0;
    pobj->kafka_type = KafkaMetadata;
    pobj->authenticated = ism_kafka_metadata_conn_authenticated;
    pobj->authenticator = authenticator;
    pobj->authenticator_data = authenticator_data;
    if(pobj->authenticator!=NULL)
        pobj->auth_require=1;
    topicRecord->metadata_transport = transport;
    topicRecord->needmetadata = 1;
    int rc = ism_kafka_createConnection(transport, server);
    if (rc) {
        char xbuf [2048];
        TRACE(5, "ism_kafka_startEventMetadataConnect: Kafka Metadata Connection failed: topic=%s topicRecord=%p server=%p metadata_transport=%p\n", topicRecord->topic, topicRecord, topicRecord->server, topicRecord->metadata_transport);
		ism_common_formatLastError(xbuf, sizeof xbuf);
		ism_common_setError(rc);
		pthread_spin_unlock(&topicRecord->lock);
		transport->close(transport, rc, 0, xbuf);
		pthread_spin_lock(&topicRecord->lock);
	}else{
		TRACE(5, "ism_kafka_startEventMetadataConnect: Kafka Metadata Connection is started: topic=%s topicRecord=%p server=%p metadata_transport=%p\n", topicRecord->topic, topicRecord, topicRecord->server, topicRecord->metadata_transport);
	}

    return rc;
}

/*
 * Start a kafka publish connection
 * Note: connectionRecord lock is taken. Need to unlock before doing transport close.
 */
int ism_kafka_startEventProduceConnect( kafkaConnectionRecord_t * connectionRecord, ism_kafka_authenticator_t  authenticator, void * authenticator_data) {
    char xbuf [256];
	ism_server_t * server = connectionRecord->server;
	int index = connectionRecord->index;
    ism_transport_t * transport = ism_transport_newOutgoing(NULL, 1);
    ism_tcp_init_transport(transport);
    TRACE(7, "ism_kafka_startEventProduceConnect: Start kafka IM Messaging publish connection\n");
    transport->originated = 1;
    transport->protocol = "kafka_publish";
    transport->protocol_family = "kafka";
    transport->tid = 0;
    transport->ready = 7;  //Set ready for Connect Timeout. See ddosTimer
    transport->connected = ism_kafka_authenticate;
    ism_kafka_con_t * pobj = (ism_kafka_con_t *) ism_transport_allocBytes(transport, sizeof(ism_kafka_con_t), 1);
    transport->pobj = pobj;
    transport->receive = receiveKafkaMessage;
    transport->closing = publishIMMessagingDisconnected;
    pobj->server = server;
    pobj->transport = transport;
    pobj->index = index;
    snprintf(xbuf, sizeof xbuf, "%s:%u:%u", connectionRecord->topic, connectionRecord->partID, connectionRecord->nodeID);
    transport->clientID = ism_transport_putString(transport, xbuf);
    transport->name = transport->clientID;
    pobj->state = TCP_CON_IN_PROCESS;
    pobj->kafka_type = KafkaIMMessaging;
    pobj->nodeID = connectionRecord->nodeID;
    pobj->partID = connectionRecord->partID;
    pobj->kafkaConnRecord = connectionRecord;
    pobj->topicRecord = connectionRecord->topicRecord;
    connectionRecord->produce_transport = transport;
    pobj->authenticated = ism_kafka_messaging_conn_authenticated;
    pobj->authenticator = authenticator;
	pobj->authenticator_data = authenticator_data;
	if(pobj->authenticator!=NULL)
	    pobj->auth_require=1;
	connectionRecord->state = KAFKA_PARTITION_CONN_OPENING;
    int rc = ism_kafka_createConnection(transport, server);
    if (rc) {
       	char ebuf [2048];
       	TRACE(5, "ism_kafka_startEventProduceConnect: Kafka Produce Connection failed: connectionRecord=%p server=%p produce_transport=%p\n", connectionRecord, connectionRecord->server, connectionRecord->produce_transport);
   		ism_common_formatLastError(ebuf, sizeof ebuf);
   		ism_common_setError(rc);
   		pthread_mutex_unlock(&connectionRecord->lock);
   		transport->close(transport, rc, 0, ebuf);
   		pthread_mutex_lock(&connectionRecord->lock);
   	}else{
   		TRACE(5, "ism_kafka_startEventProduceConnect: Kafka Produce Connection is started: connectionRecord=%p server=%p produce_transport=%p\n", connectionRecord, connectionRecord->server, connectionRecord->produce_transport);
   	}
    return rc;
}

/*
 * Add a message to the produce message set.
 * The saved message is
 * @return message size
 */
static int addKafkaBufferMessage(concat_alloc_t * buf, kafka_produce_msg_t * msg) {
    int  msgsize=0;
    int  crcpos;
    int  bodylen;

    ism_kafka_putInt8(buf, 0);        /* offset, not set on produce */
    msgsize = ism_protocol_reserveBuffer(buf, 8);
    crcpos = msgsize+4;
    ism_kafka_putInt1(buf, kafka_message_version);        /* magic */
    ism_kafka_putInt1(buf, 0);        /* attr */
    if(kafka_message_version>0){
		ism_kafka_putInt8(buf, ism_common_convertTimeToJTime(msg->time));        /* timestamp in milliseconds*/
    }
    ism_kafka_putBytes(buf, msg->key, msg->key_len); /* Event Key */
    bodylen = ism_protocol_reserveBuffer(buf, 4);

    ism_common_allocBufferCopyLen(buf, msg->buf, msg->buflen);

    ism_kafka_putInt4At(buf, bodylen, buf->used-bodylen-4);  /* Fill in body length */
    uint32_t crc = ism_common_crc(0, buf->buf+crcpos+4, buf->used-crcpos-4);
    ism_kafka_putInt4At(buf, crcpos, crc);                   /* Fill in CRC */
    ism_kafka_putInt4At(buf, msgsize, buf->used-msgsize-4);  /* Fill in message size */
    return msgsize;
}

/*
 * Put back the messages to the head of the pending queue
 *
 */
static void ism_kafka_enqueueMsgsPendingHead( kafkaConnectionRecord_t * connRecord, kafka_produce_msg_t * msgs) {
	int count=0;
	if (!msgs) {
		return;
	}
	 //Take the first time of first message
	connRecord->kafka_msg_first_time = msgs->time;
	kafka_produce_msg_t * first_msgs = msgs;
	 //Go to the last of the message in the left-over list
	count++;
	while(msgs!=NULL && msgs->next!=NULL){
	    msgs = msgs->next;
		count++;
	}

	 //Set Last if NULL
	if (!connRecord->kafka_msg_last) {
		connRecord->kafka_msg_last = msgs;
	}

	 //Link to the current head at the last of the left-over
	msgs->next =  connRecord->kafka_msg_first;
	connRecord->kafka_msg_first  = first_msgs;
	connRecord->kafka_msg_count += count;

	TRACE(7, "ism_kafka_enqueueMsgsPendingHead: partID=%d TotalEnqueueCount=%d TotalMsgCount=%d\n",
			connRecord->partID, count, connRecord->kafka_msg_count);

}

/*
 * Add msg to Ackwait list
 */
static int ism_kafka_addMsgToAckWaitList(kafkaConnectionRecord_t * connRecord, kafka_produce_msg_t * msg) {
	if (connRecord!=NULL && msg!=NULL) {
		msg->next = NULL;

		if ( connRecord->kafka_ackwait_msg_last)
			connRecord->kafka_ackwait_msg_last->next = msg;
		connRecord->kafka_ackwait_msg_last = msg;

		if (!connRecord->kafka_ackwait_msg_first) {
			connRecord->kafka_ackwait_msg_first = msg;
		}


	}
	return 0;
}


/*
 * Add a kafka record batch which is the message data for v2
 * @param transport  The transport object
 * @param connRecord The kafka connection record
 * @param buf        The output buffer
 * @param msgs       The linked list of messages to write
 * @param msgcnt     (output) the count of messages in the batch
 */
int ism_kafka_addEventRecordBatch(ism_transport_t * transport, kafkaConnectionRecord_t * connRecord, concat_alloc_t * buf,
        kafka_produce_msg_t * msgs, int * msgcnt) {
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
    ism_kafka_putInt1(buf, kafka_message_version);        /* magic */
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
        ism_kafka_putVarInt(&mbuf, count);
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
        if (!connRecord->topicRecord->kafka_produce_ack) {
            freeIMEvent(xmsg);
        } else {
        		ism_kafka_addMsgToAckWaitList(connRecord, xmsg);
        }

        //Check if more bytes allow. If not, put back the head of the queue.
	   if(msg!=NULL){
			currBufSize = buf->used-batchsize-4;
			totalMsgSize = currBufSize + msg->buflen + msg->key_len + msg->hdr_len + KAFKA_LOG_MSG_OVERHEAD;
			if(totalMsgSize >= connRecord->topicRecord->kafka_batch_size_bytes){
					//put to the head of the queue
					TRACE(5, "Maximum Batch Bytes is reached. partID=%d TotalMsgs=%d TotalMsgSizeBytes=%d MaxBytesAllowed=%d PendingMsgs=%d\n",
							connRecord->partID, count, totalMsgSize, connRecord->topicRecord->kafka_batch_size_bytes, connRecord->kafka_msg_count);
					ism_kafka_enqueueMsgsPendingHead(connRecord, msg);
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
 * Free an event
 *
 * As we now allocate the data as part of the event itself, we only need to
 * free the message.
 */
static void freeIMEvent(kafka_produce_msg_t * msg){
	if (msg) {
		ism_common_free(ism_memory_proxy_kafka_message,msg);
	}
}

/*
 * Send a set of messages to kafka
 */
int ism_kafka_message_produce(ism_transport_t * transport, kafkaConnectionRecord_t * connRecord, kafka_produce_msg_t * msgs, int * producedMsgsCount, int isResend) {
    int msgcnt = 0;
    int msgsize=0;
    uint64_t totalSent=0;
    uint64_t totalBytesSent = 0;
    uint64_t totalBatchProduceCount= 0;
    int totalMsgSize=0;
    int rc=0;
    char xbuf [16*1024];   /* room for 100 metering messages */
    concat_alloc_t cbuf = {xbuf, sizeof xbuf, 4};
    concat_alloc_t * buf = &cbuf;

    ism_kafka_putInt2(buf, ProduceRequest);         /*api key */
    ism_kafka_putInt2(buf, kafka_produce_version);  /* produce version */
    ism_kafka_putInt4(buf, 1);                      /* CorrID   */
    ism_kafka_putString(buf, ism_common_getHostnameInfo(), -1);

    /* 
     * Set Produce Ack.
     * 0 = no ack
     * 1 = ack when the data is written to the leader log
     * -1 = ack when the data is written to all in sync replicas
     */
    ism_kafka_putInt2(buf, connRecord->topicRecord->kafka_produce_ack);

    ism_kafka_putInt4(buf, 5000);     /* timeout*/

    ism_kafka_putInt4(buf, 1);        /* topic count*/
    ism_kafka_putString(buf, connRecord->topic, -1);
    ism_kafka_putInt4(buf, 1);        /* partition count */
    ism_kafka_putInt4(buf, transport->pobj->partID);   /* partition*/
    int setsize = ism_protocol_reserveBuffer(buf, 4);

    /*
     * Kafka 0.11 introduces a new message format which is radically different from
     * previous versions.  This uses the record batch and moves some of the items
     * which were in each individual message into the record batch object.
     * This also defines headers which is where to put properties.
     */
    if (kafka_message_version >= 2) {
        msgsize += ism_kafka_addEventRecordBatch(transport, connRecord, buf, msgs, &msgcnt);
    } else {
        /* Implement message versions 0 and 1 */
        while (msgs) {
            kafka_produce_msg_t * msg;
            msg = msgs;
            msgsize += addKafkaBufferMessage(buf, msg);
            msgs = msgs->next;
            msgcnt++;
            //If Wait ACk is enabled. Do not clean the message
            if(!connRecord->topicRecord->kafka_produce_ack)
                freeIMEvent(msg);
            else{
            		ism_kafka_addMsgToAckWaitList(connRecord, msg);
            }

            //Check if more bytes allow. If not, put back the head of the queue.
			if (msgs != NULL) {
				totalMsgSize = msgsize + msgs->buflen + msgs->key_len + msgs->hdr_len + KAFKA_LOG_MSG_OVERHEAD;
				if(totalMsgSize >= connRecord->topicRecord->kafka_batch_size_bytes){
						//put to the head of the queue
						ism_kafka_enqueueMsgsPendingHead(connRecord, msgs);
						break;
				}

			}
        }
    }

    ism_kafka_putInt4At(buf, setsize, buf->used-setsize-4);
    rc = transport->send(transport, buf->buf+4, buf->used-4, 0, SFLAG_FRAMESPACE);

    if(producedMsgsCount!=NULL)
    	*producedMsgsCount=msgcnt;

    connRecord->lastProduceTime = ism_common_readTSC();

    /*
     * Update ConnectionRecord Stats
     */
    if(isResend){
    	totalBatchProduceCount = __sync_add_and_fetch(&connRecord->stats.kakfaTotalBatchReProduceCount, 1);
     	totalSent= __sync_add_and_fetch(&connRecord->stats.kakfaC2PMsgsTotalReSent, msgcnt);
       	totalBytesSent= __sync_add_and_fetch(&connRecord->stats.kakfaC2PBytesTotalReSent, buf->used);

       	/* Update Overall Kafka Stats */
       	__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaC2PMsgsTotalReSent, msgcnt);
       	__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaC2PBytesTotalReSent, buf->used);
       	__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaTotalBatchReProduceCount, 1);
       	TRACE(7, "ism_kafka_message_produce: ReProduce Kafka Messages connect=%u partID=%d nodeID=%d topic=%s sentCount=%u  totalReSent=%llu totalReBytesSent=%llu totalReBatchProduceCount=%llu produceRC=%d\n",
       				transport->index, transport->pobj->partID, transport->pobj->nodeID, connRecord->topic, msgcnt,
       				(ULL)totalSent, (ULL)totalBytesSent , (ULL)totalBatchProduceCount, rc);

    }else{
    	totalBatchProduceCount = __sync_add_and_fetch(&connRecord->stats.kakfaTotalBatchProduceCount, 1);
   	    totalSent= __sync_add_and_fetch(&connRecord->stats.kakfaC2PMsgsTotalSent, msgcnt);
   	    totalBytesSent= __sync_add_and_fetch(&connRecord->stats.kakfaC2PBytesTotalSent, buf->used);
   	    /* Update Overall Kafka Stats */
    	__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaC2PMsgsTotalSent, msgcnt);
    	__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaC2PBytesTotalSent, buf->used);
    	__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaTotalBatchProduceCount, 1);
	    TRACE(7, "ism_kafka_message_produce: Produce Kafka Messages connect=%u partID=%d nodeID=%d topic=%s sentCount=%u  totalSent=%llu totalBytesSent=%llu totalBatchProduceCount=%llu produceRC=%d\n",
			transport->index, transport->pobj->partID, transport->pobj->nodeID, connRecord->topic, msgcnt,
			(ULL)totalSent, (ULL)totalBytesSent , (ULL)totalBatchProduceCount, rc);
    }

    if (buf->inheap)
        ism_common_freeAllocBuffer(buf);


    return rc;
}
#endif

/*
 * Put out one kafka header
 *
 * Header => HeaderKey HeaderVal
 * HeaderKeyLen => varint
 * HeaderKey => string
 * HeaderValueLen => varint
 * HeaderValue => data
 */
static void putKafkaHeader(concat_alloc_t * buf, const char * key, int keylen, const char * value, int len) {
    ism_kafka_putVarInt(buf, keylen);
    ism_common_allocBufferCopyLen(buf, key, keylen);
    ism_kafka_putVarInt(buf, len);
    ism_common_allocBufferCopyLen(buf, value, len);
    buf->pos++;
}


/*
 * Put out a set of concise encoded properties to a kafka header
 */
static void mapConciseToKafka(concat_alloc_t * buf, concat_alloc_t * props) {
    concat_alloc_t action = *props;
    ism_field_t f;
    ism_field_t val;
    int  rc;
    action.pos = 0;

    rc = 0;
    while (rc >= 0) {
        rc = ism_protocol_getObjectValue(&action, &f);
        if (rc < 0)
            break;
        if (f.type == VT_Name && f.val.s) {    /* user property */
            ism_protocol_getObjectValue(&action, &val);
            if (val.type == VT_String && val.val.s) {
                putKafkaHeader(buf, f.val.s, strlen(f.val.s), val.val.s, strlen(val.val.s));
            }
        } else if (f.type == VT_NameIndex) {   /* system property */
            ism_protocol_getObjectValue(&action, &val);
            switch (f.val.i) {
            case ID_Processor:
                if (val.type == VT_String && val.val.s) {
                    putKafkaHeader(buf, "_Processor", 9, val.val.s, strlen(val.val.s));
                }
            }
        }
    }
}

/*
 * This is called for each property
 */
static int putKafkaProp(mqtt_prop_ctx_t * ctx, void * userdata, mqtt_prop_field_t * fld,
        const char * ptr, int len, uint32_t value) {
    concat_alloc_t * buf = (concat_alloc_t *) userdata;
    char tbuf[16];

    /* User property */
    if (fld->type == MPT_NamePair) {
        int namelen = BIGINT16(ptr);
        int valuelen = BIGINT16(ptr+2+namelen);
        putKafkaHeader(buf, ptr+2, namelen, ptr+4+namelen, valuelen);
    } else {
        switch (fld->id) {
        case MPI_PayloadFormat:
            if (value)
                putKafkaHeader(buf, "_Format", 7,  "text", 4);
            break;
        case MPI_MsgExpire:
            sprintf(tbuf, "%u", value);
            putKafkaHeader(buf, "_Expire", 7, tbuf, strlen(tbuf));
            break;
        case MPI_ContentType:
            putKafkaHeader(buf, "_ContentType", 12,  ptr, len);
            break;
        case MPI_ReplyTopic:
            putKafkaHeader(buf, "_ReplyTo", 8, ptr, len);
            break;
        case MPI_Correlation:
            putKafkaHeader(buf, "_Correlation", 12, ptr, len);
            break;
        }
    }
    return 0;
}


/*
 * Construct the kafka headers
 * The properties can come from thre locations
 * - The MQTTv5 properties MQTT encoding
 * - The route rule system properties in concise encoding
 * - The route rule user properties in concise encoding
 *
 * The kafka headers are written in kafka message format.  They do not exist in versions less than 2.
 */
int ism_kafka_makeKafkaHeaders(ism_transport_t * transport, concat_alloc_t * buf, mqtt_pmsg_t * pmsg,
        concat_alloc_t * sysprops, concat_alloc_t * userprops, int msgver) {
    int count;

    if (msgver < 0)
        msgver = kafka_message_version;
    buf->pos = 0;
    if (msgver < 2)
        return 0;         /* No headers before v2 */
    if (sysprops && sysprops->used > 0)
        mapConciseToKafka(buf, sysprops);
    if (pmsg->prop_len) {
        /* Get some things only known to mqtt */
        mqtt_prop_ctx_t * ctx = ism_proxy_getMqttContext(5);  /* Fix when there are others */
        if (ctx) {
            ism_common_checkMqttPropFields(pmsg->props, pmsg->prop_len, ctx, CPOI_PUBLISH, putKafkaProp, buf);
        }
    }
    if (userprops && userprops->used > 0)
        mapConciseToKafka(buf, userprops);
    count = buf->pos;
    buf->pos = 0;
    return count;
}

#ifndef NO_PROXY
/*
 * Publish a kafka event
 */
int ism_kafka_publishEvent(ism_transport_t * transport, mqtt_pmsg_t * pmsg, const char * kafkaTopic,
        const char * kafkaKey, int kafkaKey_Len, int hdrcount, const char * headers, int hdr_len) {
	int  rc = 1;
	//uint32_t cid_crc=0;
	kafka_produce_msg_t * event;
	kafka_produce_msg_t * doProduce = NULL;
	int needmetadata=0;

	if (*transport->protocol_family=='k' || *transport->protocol_family=='a')
		return 0;

	//transport->tenant->messageRouting=0;
	const char * devtype;
	const char * devid;
	const char * evnt;
	const char * fmt;
	const char * clientID=transport->clientID;
    char * topic = alloca(pmsg->topic_len + 1);
    memcpy(topic, pmsg->topic, pmsg->topic_len);
    topic[pmsg->topic_len] = 0;

	rc = ism_proxy_parseMQTTTopic(topic, &devtype, &devid, &evnt, &fmt);

	if (rc == 0 && evnt!=NULL) {

		TRACE(7, "ism_kafka_publishMessage:  rc=%u clientID=%s topic=%s devtype=%s devid=%s evnt=%s fmt=%s\n", rc, clientID, topic, devtype, devid, evnt, fmt);
		/*  Create eventke */
		char xxbuf[512];
		concat_alloc_t keybuf = {xxbuf, sizeof xxbuf};
        int eventlen;

		if (kafkaKey == NULL) {
			ism_kafka_makeEventKey(&keybuf, transport->org,  devtype, devid, evnt, fmt, NULL);
			kafkaKey = keybuf.buf;
			kafkaKey_Len = keybuf.used;
		}
		eventlen = kafkaKey_Len + pmsg->payload_len + hdr_len;
		event = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_kafka_message,1),sizeof(kafka_produce_msg_t) + eventlen);
		memset(event, 0, sizeof(kafka_produce_msg_t));

		event->buf = (char *)(event+1);
		if (pmsg->payload && pmsg->payload_len)
		    memcpy(event->buf, pmsg->payload, pmsg->payload_len);
		event->buflen = pmsg->payload_len;

		event->key = event->buf + pmsg->payload_len;
		memcpy(event->key, kafkaKey, kafkaKey_Len);
		event->key_len = kafkaKey_Len;

		event->hdr = (uint8_t *)(event->key + event->key_len);
		if (hdr_len)
		    memcpy(event->hdr, headers, hdr_len);
		event->hdr_len = hdr_len;
		event->hdrcount = hdrcount;

		/* Set the message timestamp */
		event->time = ism_common_currentTimeNanos();

		if (keybuf.inheap)
			ism_common_freeAllocBuffer(&keybuf);

		/*
		 * Determine which Kafka Connection needs to use
		 * use Org, DevType, and DevID
		 */
		int dcrcNameLen= strlen(transport->org) + strlen(devtype)+ strlen(devid) + 2 ; // +2 slashes
		char * dcrcName = (char *)alloca(dcrcNameLen + 1);
		sprintf(dcrcName, "%s/%s/%s", transport->org, devtype, devid);
		uint32_t did_crc = ism_common_crc(0, dcrcName,dcrcNameLen);
		event->dcrc = did_crc;

		kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord((kafkaTopic!=NULL)? kafkaTopic : kafka_im_messaging_topic);

		if(topicRecord==NULL){
			TRACE(6, "publishEvent:  Failed to publish. TopicRecord is NULL. topic=%s\n", kafkaTopic);
			freeIMEvent(event);
			return 1;
		}

		pthread_spin_lock(&topicRecord->lock);

		if(topicRecord->connectionCount < 1){
			TRACE(6, "publishEvent:  No Kafka Partition Connections.\n");
			freeIMEvent(event);
			pthread_spin_unlock(&topicRecord->lock);
			return 1;
		}
		needmetadata = topicRecord->needmetadata;
		//int which = did_crc % kafka_im_messaging_connections_count;
		int which = did_crc % topicRecord->connectionCount;
		kafkaConnectionRecord_t * kafkaConnectionRecord =  ism_common_getArrayElement(topicRecord->partitionConnections, which+1);

		if(kafkaConnectionRecord == NULL){
		    TRACE(6, "publishEvent: No Kafka Connection for this partition %d.\n", which);
		    freeIMEvent(event);
		    pthread_spin_unlock(&topicRecord->lock);
		    return 1;
		}

		pthread_spin_unlock(&topicRecord->lock);

		//Take the lock for the kafka Connection Record.
		pthread_mutex_lock(&kafkaConnectionRecord->lock);

		ism_transport_t *  kafka_transport = kafkaConnectionRecord->produce_transport;

		if ( kafkaConnectionRecord->kafka_msg_last)
			kafkaConnectionRecord->kafka_msg_last->next = event;

		kafkaConnectionRecord->kafka_msg_last = event;

		if (!kafkaConnectionRecord->kafka_msg_first) {
			kafkaConnectionRecord->kafka_msg_first = event;
			kafkaConnectionRecord->kafka_msg_first_time = event->time;
			kafkaConnectionRecord->kafka_msg_count = 1;
		} else {
			kafkaConnectionRecord->kafka_msg_count++;
		}

		//Update Total Kafka (for all topic) Stats
		__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaC2PMsgsTotalReceived, 1);
		__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaC2PBytesTotalReceived, pmsg->buflen);
		__sync_add_and_fetch(&kafkaIMMessagingStats.kakfaTotalPendingMsgsCount, 1);

		//Update Connection Record Stats
		kafkaConnectionRecord->stats.kakfaC2PMsgsTotalReceived++;
		kafkaConnectionRecord->stats.kakfaTotalPendingMsgsCount++;
		kafkaConnectionRecord->stats.kakfaC2PBytesTotalReceived += pmsg->buflen;

		/* If the kafka publishing connection is open, we return a 0 return code */
		if (kafkaConnectionRecord->state == KAFKA_PARTITION_CONN_OPEN && kafka_transport && kafka_transport->state == ISM_TRANST_Open)
			rc = 0;
		else{
			//The Connection Record is not in the open state. or TCP connection is not in Open State
			//Still keep the message in the Pending Q
			//Will move it once the connection established.
			if(kafka_transport){
				TRACE(5, "publishEvent: Partition Connection is not open. which=%d pending_msg_count=%d connectionState=%d kafka_transport=%p kafka_transport.index=%d kafka_transport.state=%d kafka_transport.ready=%d needmetadata=%d\n",
						which, kafkaConnectionRecord->kafka_msg_count, kafkaConnectionRecord->state, kafka_transport, kafka_transport->index, kafka_transport->state, kafka_transport->ready, needmetadata);
			}else{
				TRACE(5, "publishEvent: Partition Connection is not open. which=%d pending_msg_count=%d connectionState=%d transport=null  needmetadata=%d\n",
										which, kafkaConnectionRecord->kafka_msg_count, kafkaConnectionRecord->state,  needmetadata);
			}
			rc = 1;
		}

		/* If connection is open check if batch is full */
		if (rc == 0) {
			doProduce = checkIMEventBatch(kafkaConnectionRecord, event->time, 0);

			kafka_transport->read_msg++;
			kafka_transport->read_bytes += event->buflen;

			/* If we have a batch to send, do the kafka produce now */
			if (doProduce) {
				int producedMsgsCount=0;
				ism_kafka_message_produce(kafka_transport, kafkaConnectionRecord, doProduce, &producedMsgsCount,0);
				if(!kafkaConnectionRecord->topicRecord->kafka_produce_ack){
					kafkaConnectionRecord->stats.kakfaTotalPendingMsgsCount -= producedMsgsCount;
					//Update Over Kafkaf Stats
					__sync_sub_and_fetch(&kafkaIMMessagingStats.kakfaTotalPendingMsgsCount, producedMsgsCount);
				}

				TRACE(7, "Kafka IM Messages Produced:  topic=%s partid=%d leader=%d  currentMsgCount=%u totalReceived=%llu producedMsgsCount=%d pendingMsgs=%llu\n",
									kafkaConnectionRecord->topicRecord->topic, kafkaConnectionRecord->partID, kafkaConnectionRecord->nodeID,
									kafkaConnectionRecord->kafka_msg_count, (ULL)kafkaConnectionRecord->stats.kakfaC2PMsgsTotalReceived, producedMsgsCount,
									(ULL)kafkaConnectionRecord->stats.kakfaTotalPendingMsgsCount);

			}

		}

		pthread_mutex_unlock(&kafkaConnectionRecord->lock);
	}
	return rc;
}


/*
 * Get Kafka IM Messaging statistics
 */
int ism_proxy_getKafkaIMMessagingStats(px_kafka_messaging_stat_t * stats) {
    memcpy(stats, &kafkaIMMessagingStats, sizeof(px_kafka_messaging_stat_t));
    return 0;
}


int ism_kafka_metadata_conn_authenticated(int rc , void * data)
{
	ism_transport_t * transport = (ism_transport_t*) data;
	TRACE(9, "ism_kafka_metadata_conn_authenticated: rc=%d connection=%u\n", rc, transport->index);
	if (rc==ISMRC_OK){
		/*Send request for Metadata*/
		transport->pobj->broker_state=KAFKA_STATE_BROKER_AUTHENTICATED;
	}else{
		transport->pobj->broker_state=KAFKA_STATE_BROKER_NOT_AUTHORIZED;
	}
	return kafkaIMMessagingMetadataConnected(transport, rc);

}

int ism_kafka_messaging_conn_authenticated(int rc , void * data)
{
	ism_transport_t * transport = (ism_transport_t*) data;
	TRACE(5, "ism_kafka_messaging_conn_authenticated: rc=%d connection=%u\n", rc, transport->index);
	if (rc==ISMRC_OK){
		/*Set the Status and continue*/
		transport->pobj->broker_state=KAFKA_STATE_BROKER_AUTHENTICATED;
	}else{
		transport->pobj->broker_state=KAFKA_STATE_BROKER_NOT_AUTHORIZED;
	}
	return  kafkaIMMessagingConnected(transport, rc);

}


int ism_kafka_addTopicRecord(const char * kafkaTopic, kafkaTopicRecord_t * topicRecord)
{
	int rc = 0;
	if(g_kafkaTopicRecordTable){
		ism_common_HashMapLock(g_kafkaTopicRecordTable);
		rc = ism_common_putHashMapElement(g_kafkaTopicRecordTable, kafkaTopic, strlen(kafkaTopic), topicRecord, NULL);
		ism_common_HashMapUnlock(g_kafkaTopicRecordTable);
		TRACE(6, "ism_kafka_addTopicRecord. topic=%s topicRecord=%p\n", kafkaTopic, topicRecord );
			return rc;
	}
	return 0;
}

kafkaTopicRecord_t *  ism_kafka_removeTopicRecord(const char * kafkaTopic)
{
	kafkaTopicRecord_t * topicRecord=NULL;
	if(g_kafkaTopicRecordTable){
		ism_common_HashMapLock(g_kafkaTopicRecordTable);
		topicRecord = ism_common_removeHashMapElement(g_kafkaTopicRecordTable, kafkaTopic, strlen(kafkaTopic));
		ism_common_HashMapUnlock(g_kafkaTopicRecordTable);
	}
	TRACE(9, "ism_kafka_removeTopicRecord. topic=%s topicRecord=%p\n", kafkaTopic,  topicRecord);
	return topicRecord;
}

kafkaTopicRecord_t * ism_kafka_getTopicRecord(const char * kafkaTopic)
{
	kafkaTopicRecord_t * topicRecord =NULL;
	if(g_kafkaTopicRecordTable){
		ism_common_HashMapLock(g_kafkaTopicRecordTable);
		topicRecord = ism_common_getHashMapElement(g_kafkaTopicRecordTable, kafkaTopic, strlen(kafkaTopic));
		ism_common_HashMapUnlock(g_kafkaTopicRecordTable);
	}
	TRACE(9, "ism_kafka_getTopicRecord. topic=%s topicRecord=%p\n", kafkaTopic,  topicRecord);

	return topicRecord;
}


/*
 * Parse the list of kafka brokers from JSON
 */
ism_server_t * ism_kafka_parseBroker(ism_json_entry_t * ents) {
    ism_json_entry_t * ent;
    int count = ents->count;
    int i;
    char valcopy[1024];
    ism_server_t * server=NULL;
    int  rc = 0;

    ent = ents+1;
    for (i=0; i<count; i++) {
        if (ent->objtype != JSON_String || portValue(ent->value) <= 0) {
            ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ents->name, getJsonValue(ent));
            rc = ISMRC_BadPropertyValue;
            break;
        }
        ent++;
    }
    if (count > 32) {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ents->name, "");
        rc = ISMRC_BadPropertyValue;
    }
    if (rc == 0) {
        server = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_kafka,7),1, sizeof(ism_server_t));
        strcpy(server->structid, "IoTKSRV");
        server->name = "_Kafka";
        pthread_spin_init(&server->lock, 0);
        server->serverKind = 1;
        server->getAddress = ism_kafka_getAddress;
        server->ipaddr_count = count;
        ent = ents+1;
        for (i=0; i<count; i++) {
            char * pos;
            strcpy(valcopy, ent->value);
            pos = strrchr(valcopy, ':');
            *pos++ = 0;
            replaceString(&server->address[i], valcopy);
            server->a_port[i] = atoi(pos);
            TRACE(4, "Parse kafka broker: %u %s:%u\n", i, server->address[i], server->a_port[i]);
            ent++;
        }
    }

    return server;
}

int ism_kafka_createTopicRecord(	const char * kafkaBrokers, 	const char * kafkaTopic, ism_kafka_authenticator_t  authenticator, const char * kafkaAuthID, 	const char * kafkaAuthPW,
								int useTLS, const char * tlsMethod, const char * tlsCiphers,
								int kafka_batch_time_millis_in, int kafka_batch_size_in, int kafka_batch_size_bytes_in, int kafka_batch_size_recovery_in,
								int kafka_produce_ack, int kafka_dist_method, int kafka_shutdown_onerror_time)
{
	int i=0;
	ism_server_t * server;
	int  rc = 0;
	char valcopy[1024];
	int existed=0;


	if(kafkaTopic!=NULL && kafkaBrokers!=NULL){

		kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(kafkaTopic);
		if(topicRecord!=NULL){
			if(kafkaBrokers!=NULL){
				 char * more = (char *)kafkaBrokers;
				 char * token=NULL;
				 i=0;
				 pthread_spin_lock(&topicRecord->lock);
				 ism_server_t * topicrecord_server = topicRecord->server;
				 while ((token = ism_common_getToken(more, " \t,", " \t,", &more))) {
					 char * pos;
					 strcpy(valcopy, token);
					 pos = strrchr(valcopy, ':');
					 *pos++ = 0;

					 char * server_ip = valcopy;
					 int ipcount=0;
					 for (ipcount=0; ipcount<topicrecord_server->ipaddr_count; ipcount++ ){
						 if (topicrecord_server->address[ipcount]) {
							 if (!strcmp(topicrecord_server->address[ipcount], server_ip)){
							     existed=1;
						         break;
							 }
						 }
					 }
					 if (existed)
					     break;
					 i++;
				 }
				 pthread_spin_unlock(&topicRecord->lock);
			} else {
				existed=1;
			}
			if(existed){
				TRACE(5, "New Kafka configuration: Already existed: topic=%s topicRecord=%p\n", kafkaTopic,  topicRecord);
				return 2;
			}
		}



		TRACE(5, "New Kafka configuration: topic=%s brokers=%s\n", kafkaTopic,  kafkaBrokers);

		server = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_kafka,8),1, sizeof(ism_server_t));
		strcpy(server->structid, "IoTKSRV");
		server->name = "_Kafka";
		pthread_spin_init(&server->lock, 0);
		server->serverKind = 1;
		server->getAddress = ism_kafka_getAddress;
		if (useTLS) {
			server->tlsCTX = ism_transport_clientTlsContext("kafka", tlsMethod, tlsCiphers);
		}
		 char * more = (char *)kafkaBrokers;
		 char * token=NULL;
		 i=0;
		 while ((token = ism_common_getToken(more, " \t,", " \t,", &more))) {
			 char * pos;
			 strcpy(valcopy, token);
			 pos = strrchr(valcopy, ':');
			 *pos++ = 0;

			 replaceString(&server->address[i], valcopy);
			 server->a_port[i] = atoi(pos);
			 TRACE(4, "Parse kafka broker string: %s:%u\n", server->address[i], server->a_port[i]);
			 i++;
		 }
		 //if(i>0)
		//	 server->ipaddr_count = i+1;
		 server->ipaddr_count=i;

		if(!topicRecord){
			topicRecord = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_kafka,9),1, sizeof(kafkaTopicRecord_t));
			topicRecord->partitionConnections = ism_common_createArray(100);
			pthread_spin_init(&topicRecord->lock, 0);
			topicRecord->topic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),kafkaTopic);
			ism_kafka_addTopicRecord(topicRecord->topic, topicRecord);
		}
		topicRecord->server = server;
		kafkaSASLAuth_t * saslAuth = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_kafka,10),1,sizeof (kafkaSASLAuth_t));
		if(kafkaAuthID!=NULL){
			saslAuth->authid=ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),kafkaAuthID);
			saslAuth->authid_len = strlen(kafkaAuthID);
		}
		if(kafkaAuthPW!=NULL){
			saslAuth->password = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),kafkaAuthPW);
			saslAuth->password_len = strlen(kafkaAuthPW);
		}
		topicRecord->kafka_batch_time_millis = (kafka_batch_time_millis_in>0)?kafka_batch_time_millis_in:250;
		topicRecord->kafka_batch_time = topicRecord->kafka_batch_time_millis * 1000000L;
		topicRecord->kafka_batch_size = (kafka_batch_size_in>0)?kafka_batch_size_in:100;
		topicRecord->kafka_batch_size_bytes = (kafka_batch_size_bytes_in>0)?kafka_batch_size_bytes_in:kafka_im_messaging_batch_size_bytes;
		topicRecord->kafka_recovery_batch_size =(kafka_batch_size_recovery_in>0)?kafka_batch_size_recovery_in: topicRecord->kafka_batch_size;
		topicRecord->kafka_produce_ack = kafka_produce_ack;
		topicRecord->kafka_shutdown_onerror_time = kafka_shutdown_onerror_time;
		if(kafka_produce_ack != 1 && kafka_produce_ack != -1 && kafka_produce_ack != 0){
			topicRecord->kafka_produce_ack=0;
		}
		topicRecord->kafka_dist_method = kafka_dist_method;
		topicRecord->authenticator = authenticator;
		topicRecord->authenticator_data = saslAuth;
		topicRecord->needmetadata = 1;

		ism_common_setTimerOnce(ISM_TIMER_HIGH, startKafkaIMMessagingTimer, ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),topicRecord->topic), 1);
	}else{
		TRACE(5, "New Kafka configuration is not valid.\n");
		rc=1;
	}
	return rc;
}

/*
 */
int ism_kafka_initMessageHubLocal(void) {



    useKafkaMessageHub = ism_common_getIntConfig("UseKafkaMessageHub", 0);


    if(!useKafkaMessageHub){
    	TRACE(7, "Kafka MessageHub is disabled\n");
    	return 1;
    }
	TRACE(5, "Kafka MessageHub is enabled");

	kafkaMessageHubBatchTimeMillis = ism_common_getIntConfig("UseKafkaMessageHubBatchTimeMillis", 0);
	kafkaMessageHubBatchSize = ism_common_getIntConfig("UseKafkaMessageHubBatchSize", 0);
	kafkaMessageHubUseTLS = ism_common_getIntConfig("UseKafkaMessageHubTLS", 0);
	kafkaMessageHubTLSCiphers = ism_common_getStringConfig("KafkaMessageHubCipher");
	kafkaMessageHubTLSMethod = ism_common_getStringConfig("KafkaMessageHubTLSMethod");
	kafkaMessageHubProduceAck = ism_common_getIntConfig("KafkaMessageHubProduceAck",0);
	kafkaMessageHubDistMethod = ism_common_getIntConfig("KafkaMessageHubDistMethod",0);
	int useMessageHubTestInstance = ism_common_getIntConfig("useMessageHubTestInstance",0);


    pthread_mutex_lock(&kafka_im_lock);


    if (useMessageHubTestInstance) {
			const char * kafkaBrokers = ism_common_getStringConfig("KafkaMessageHubTestBrokers");
			const char * kafkaTopic = ism_common_getStringConfig("KafkaMessageHubTestTopic");
			const char * kafkaAuthID = ism_common_getStringConfig("KafkaMessageHubTestAuthID");
			const char * kafkaAuthPW = ism_common_getStringConfig("KafkaMessageHubTestAuthPW");
			ism_kafka_createTopicRecord(kafkaBrokers, kafkaTopic, ism_kafka_metadata_conn_authenticator_sasl_plain,
									kafkaAuthID, kafkaAuthPW,
									kafkaMessageHubUseTLS, kafkaMessageHubTLSMethod,kafkaMessageHubTLSCiphers,
									kafkaMessageHubBatchTimeMillis, kafkaMessageHubBatchSize,250000, 0, kafkaMessageHubProduceAck, kafkaMessageHubDistMethod, 0);


    }
    pthread_mutex_unlock(&kafka_im_lock);
    return 0;
}

/**
 * Set IM Messaging Selector
 */
void ism_kafka_setIMMessagingSelector(const char * in_selector)
{
	if(in_selector != NULL){
		char * old_selector = g_IMMessagingSelectorStr;
		int ruleLen=0;
		int rc =0;
		g_IMMessagingSelectorStr = (char *) ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_kafka,1000),in_selector);
		TRACE(5, "IMMessaging Selector String = %s.\n",g_IMMessagingSelectorStr);
		rc = ism_common_compileSelectRuleOpt(&g_IMMessagingSelector, (int *)&ruleLen,
											g_IMMessagingSelectorStr, SELOPT_Internal);
		TRACE(5, "IMMessaging Selector: %s compileRC=%d ruleLen=%d\n", g_IMMessagingSelectorStr, rc, ruleLen);
		if(rc!=0){
			g_IMMessagingSelector=NULL;
		}
		if(old_selector!=NULL){
			ism_common_free(ism_memory_proxy_kafka,old_selector);
			old_selector=NULL;
		}
	}else{
		TRACE(5, "New Selector is NULL. Current IMMessaging Selector = %s.\n",g_IMMessagingSelectorStr);
	}
}

/*
 * Enabled/Disabled the Per Connection Stats for Kafka
 * for IM Messaging
 */
void ism_kafka_setKafkaIMConnDetailStatsEnabled(int enabled)
{
	if(g_kafkaIMConnDetailStatsEnabled!=enabled){
		g_kafkaIMConnDetailStatsEnabled=enabled;
	}
	TRACE(5, "setKafkaIMConnDetailStatsEnabled: enabled=%d.\n", g_kafkaIMConnDetailStatsEnabled);
}

/*
 * Enabled/Disabled the Produce ACKing
 * for IM Messaging
 */
int ism_kafka_setKafkaIMProduceAck(int acks)
{
	if(kafka_im_messaging_topic==NULL){
		TRACE(5, "ism_kafka_setKafkaIMProduceAck: IM Messaging Topic is not set.\n");
		return 1;
	}
	if(acks!=0 && acks!=1 && acks!=-1){
		TRACE(5, "ism_kafka_setKafkaIMProduceAck: value is not valid. value=%d\n", acks);
		return 1;
	}
	kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(kafka_im_messaging_topic);
	if(topicRecord!=NULL){
		pthread_spin_lock(&topicRecord->lock);
		topicRecord->kafka_produce_ack = acks;
		pthread_spin_unlock(&topicRecord->lock);
	}
	TRACE(5, "ism_kafka_setKafkaIMProduceAck: value=%d.\n", acks);
	return 0;
}

/*
 * Set IM Messaging Batch Size
 */
int ism_kafka_setKafkaIMBatchSize(int batchsize)
{
	if(kafka_im_messaging_topic==NULL){
		TRACE(5, "ism_kafka_setKafkaIMProduceAck: IM Messaging Topic is not set.\n");
		return 1;
	}
	if(batchsize<1){
		TRACE(5, "ism_kafka_setKafkaIMBatchSize: value is not valid. value=%d\n", batchsize);
		return 1;
	}
	kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(kafka_im_messaging_topic);
	if(topicRecord!=NULL){
		pthread_spin_lock(&topicRecord->lock);
		topicRecord->kafka_batch_size = batchsize;
		if(topicRecord->kafka_batch_size > topicRecord->kafka_recovery_batch_size){
			topicRecord->kafka_recovery_batch_size = topicRecord->kafka_batch_size;
		}
		TRACE(5, "ism_kafka_setKafkaIMBatchSize: BatchSize=%d recoveryBatchSize=%d.\n", topicRecord->kafka_batch_size, topicRecord->kafka_recovery_batch_size);
		pthread_spin_unlock(&topicRecord->lock);
	}else{
		TRACE(5, "ism_kafka_setKafkaIMBatchSize: TopicRecord is NULL.\n");
	}
	return 0;
}
/*
 * Set IM Messaging Batch Size
 */
int ism_kafka_setKafkaIMBatchSizeBytes(int batchsizebytes)
{
	if(kafka_im_messaging_topic==NULL){
		TRACE(5, "ism_kafka_setKafkaIMBatchSizeBytes: IM Messaging Topic is not set.\n");
		return 1;
	}
	if(batchsizebytes<1){
		TRACE(5, "ism_kafka_setKafkaIMBatchSizeBytes: value is not valid. value=%d\n", batchsizebytes);
		return 1;
	}
	kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(kafka_im_messaging_topic);
	if(topicRecord!=NULL){
		pthread_spin_lock(&topicRecord->lock);
		topicRecord->kafka_batch_size_bytes = batchsizebytes;

		TRACE(5, "ism_kafka_setKafkaIMBatchSizeBytes: BatchSizeBytes=%d\n", topicRecord->kafka_batch_size_bytes);
		pthread_spin_unlock(&topicRecord->lock);
	}else{
		TRACE(5, "ism_kafka_setKafkaIMBatchSizeBytes: TopicRecord is NULL.\n");
	}
	return 0;
}

/*
 * Set IM Messaging Recover Batch Size
 */
int ism_kafka_setKafkaIMRecoverBatchSize(int batchsize)
{
	if(kafka_im_messaging_topic==NULL){
		TRACE(5, "ism_kafka_setKafkaIMRecoverBatchSize: IM Messaging Topic is not set.\n");
		return 1;
	}
	if(batchsize<1){
		TRACE(5, "ism_kafka_setKafkaIMRecoverBatchSize: value is not valid. value=%d\n", batchsize);
		return 1;
	}
	kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(kafka_im_messaging_topic);
	if(topicRecord!=NULL){
		pthread_spin_lock(&topicRecord->lock);
		topicRecord->kafka_recovery_batch_size = batchsize;
		if(topicRecord->kafka_batch_size > topicRecord->kafka_recovery_batch_size){
			topicRecord->kafka_recovery_batch_size = topicRecord->kafka_batch_size;
		}
		TRACE(5, "ism_kafka_setKafkaIMRecoverBatchSize: BatchSize=%d recoveryBatchSize=%d.\n", topicRecord->kafka_batch_size, topicRecord->kafka_recovery_batch_size);
		pthread_spin_unlock(&topicRecord->lock);
	}else{
		TRACE(5, "ism_kafka_setKafkaIMRecoverBatchSize: TopicRecord is NULL.\n");
	}

	return 0;
}

/*
 * Set IM Messaging Server Shutdown on Error
 * 0=no time
 * >0=time in secs
 */
int ism_kafka_setKafkaIMServerShutdownOnErrorTime(int time_in_secs)
{
	if(kafka_im_messaging_topic==NULL){
		TRACE(5, "ism_kafka_setKafkaIMServerShutdownOnErrorTime: IM Messaging Topic is not set.\n");
		return 1;
	}
	if(time_in_secs<0){
		TRACE(5, "ism_kafka_setKafkaIMServerShutdownOnErrorTime: value is not valid. value=%d\n", time_in_secs);
		return 1;
	}
	kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord(kafka_im_messaging_topic);
	if(topicRecord!=NULL){
		pthread_spin_lock(&topicRecord->lock);
		topicRecord->kafka_shutdown_onerror_time = time_in_secs;
		TRACE(5, "ism_kafka_setKafkaIMServerShutdownOnErrorTime: time=%d.\n", topicRecord->kafka_shutdown_onerror_time);
		pthread_spin_unlock(&topicRecord->lock);
	}else{
		TRACE(5, "ism_kafka_setKafkaIMServerShutdownOnErrorTime: TopicRecord is NULL.\n");
	}

	return 0;
}

/*
 * Get Connection Stats for the Kafka Topic
 */
int ism_proxy_getKafkaConnectionStats(px_kafka_messaging_stat_t * stats, const char * kafkaTopic , int * pCount) {

	if(kafkaTopic==NULL && kafka_im_messaging_topic==NULL){
		TRACE(6, "Connection Stats: Topic is NULL and IMMessaging Topic is also NULL\n");
		*pCount=0;
		return 0;
	}
	kafkaTopicRecord_t * topicRecord = ism_kafka_getTopicRecord((kafkaTopic!=NULL)? kafkaTopic : kafka_im_messaging_topic);
	if(topicRecord==NULL){
		TRACE(6, "Connection Stats: TopicRecord is NULL. topic=%s\n", kafkaTopic);
		*pCount=0;
		return 0;
	}

	pthread_spin_lock(&topicRecord->lock);
	if(topicRecord->connectionCount < 1){
		TRACE(6, "Connection Stats:  No Kafka Partition Connections.\n");
		pthread_spin_unlock(&topicRecord->lock);
		*pCount=0;
		return 0;
	}
	int connCount = topicRecord->connectionCount;

	if(*pCount < connCount){
		*pCount = connCount;
		pthread_spin_unlock(&topicRecord->lock);
		return 1;
	}

	for (int which = 0; which < connCount; which++){
		kafkaConnectionRecord_t * kafkaConnectionRecord =  ism_common_getArrayElement(topicRecord->partitionConnections, which+1);
		pthread_spin_unlock(&topicRecord->lock);
		pthread_mutex_lock(&kafkaConnectionRecord->lock);
		//Calculate the avg Latency time for Produce
		if(kafkaConnectionRecord->topicRecord->kafka_produce_ack != 0 && kafkaConnectionRecord->stats.kakfaTotalBatchProduceAckReceivedCount>0){
			kafkaConnectionRecord->stats.kafkaProduceLatencyMS = (uint64_t) (ceil(1000 * kafkaConnectionRecord->sumProduceLatency / kafkaConnectionRecord->stats.kakfaTotalBatchProduceAckReceivedCount));
		}
		memcpy(&stats[which], &kafkaConnectionRecord->stats, sizeof(px_kafka_messaging_stat_t));
		pthread_mutex_unlock(&kafkaConnectionRecord->lock);

		pthread_spin_lock(&topicRecord->lock);
	}
	 *pCount = connCount;
	pthread_spin_unlock(&topicRecord->lock);
	return 0;

}
#endif

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
    if(topic!=NULL && segs<count){
         segments[segs]=topic;
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
int ism_proxy_parseMQTTTopic(char * topic, const char * * devtype, const char * * devid, const char * * event, const char * * fmt) {
    char * topicseg[10];
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
    count = topicSegment(topic, topicseg, 10);
    if (count > 5) {
        if (!strcmp(topicseg[2], "type") && !strcmp(topicseg[4], "id") &&
                (!strcmp(topicseg[6], "evt") || !strcmp(topicseg[6], "cmd")) &&
                !strcmp(topicseg[8], "fmt")) {
            *devtype = topicseg[3];
            *devid = topicseg[5];
            *event = topicseg[7];
            *fmt = topicseg[9];
            if (*topicseg[3] && *topicseg[3] != '+' &&
                *topicseg[5] && *topicseg[5] != '+' &&
                *topicseg[7] && *topicseg[7] != '+' &&
                *topicseg[9] && *topicseg[9] != '+') {
                return *topicseg[6]=='c' ? 3: 0;
            }
            return 2;
        }
    }
    return 1;
}


/*
 * Put a kafka var int from a unit32
 *
 * The value is represented as a set of bytes each containing 7 bits of data
 * in litle endian order.  The high order bit indicates that there is more data.
 * This uses the zig-zag method where the value is shifted left 1 bit and negative
 * numbers are the two's compliment with the lower bit set.
 */
void ism_kafka_putVarInt(concat_alloc_t * buf, int32_t ilen) {
    uint32_t len = ilen << 1;
    if (ilen < 0)
        len ^= 1;
    if (len <= 127) {
        bputchar(buf, len);
    } else if (len <= 16383) {
        bputchar(buf, (len&0x7f)|0x80);
        bputchar(buf, (char)(len>>7));
    } else if (len <= 2097151) {
        bputchar(buf, (char)((len&0x7f)|0x80));
        bputchar(buf, (char)((len>>7)|0x80));
        bputchar(buf, (char)(len>>14));
    } else if (len <= 268435455) {
        bputchar(buf, (char)((len&0x7f)|0x80));
        bputchar(buf, (char)((len>>7)|0x80));
        bputchar(buf, (char)((len>>14)|0x80));
        bputchar(buf, (char)(len>>21));
    } else {
        bputchar(buf, (char)((len&0x7f)|0x80));
        bputchar(buf, (char)((len>>7)|0x80));
        bputchar(buf, (char)((len>>14)|0x80));
        bputchar(buf, (char)((len>>21)|0x80));
        bputchar(buf, (char)(len>>28));
    }
}

/*
 * Put a kafka var long from a unit32
 * This uses the zig-sag method
 */
void ism_kafka_putVarLong(concat_alloc_t * buf, int64_t ilen) {
    uint64_t len = ilen << 1;
    if (ilen < 0)
        len ^= 1;
    if ((len>>32) == 0) {
        ism_kafka_putVarInt(buf, (int)ilen);
    } else {
        while ((len & 0xffffffffffffff80UL) != 0UL) {
            bputchar(buf, (char)((len&0x7f) | 0x80));
            len >>= 7;
        }
        bputchar(buf, (char)len);
    }
}


