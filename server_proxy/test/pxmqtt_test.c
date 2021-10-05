/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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

/* This file is included from proxy_test.c */

int          g_entered_send_cb;
int          g_entered_close_cb;
int          g_entered_closed_cb;
const char * g_cmd;
int          g_cmd_result;
int          g_qos;
int          g_retlen;
char         g_retbuf[1024];
int          g_msg_received =0;
int          g_msg_received_len=0;


#define ISM_MQTT_PUBLISH    1000
#define ISM_MQTT_PUBREL     1001
#define ISM_MQTT_PUBREC     1002

extern int g_msgRoutingEnabled;
extern int g_useKafkaIMMessaging;
extern int g_mhubEnabled;
/**
 * The close method for the transport object used for tests
 */
static int mclose(ism_transport_t * transport, int rc, int clean, const char * reason) {
    g_entered_close_cb++;
    if (g_verbose > 1 && g_entered_close_cb == 1)
        printf("close connect=%d rc=%d reason=%s\n", transport->serverport, rc, reason);
    transport->closing(transport, rc, clean, reason);
    return 0;
}

/*
 * The closed method for the transport object used for tests
 */
static int mclosed(ism_transport_t * trans) {
    return 0;
}

/*
 * The send method for the transport object used for tests
 */
static int msend(ism_transport_t * transport, char * buf, int len, int kind, int flags) {
    int i;
    //printf("msend received: kind=%d len=%d g_cmd=%s\n", kind, len, g_cmd);
    g_msg_received_len +=len;
    g_msg_received++;
    if (g_cmd) {
        if (flags & SFLAG_HASFRAME && len > 0) {
            kind = *buf;
            for (i=1; i<len; i++) {
                if ((buf[i]&0x80) == 0)
                    break;
            }
            i++;
            len -= i;
            buf += i;
        }
        if (!strcmp(g_cmd, "CONNECT")) {
            if (transport->pobj->client_transport) {
                transport->pobj->client_transport->pobj->connectdata = NULL;
            }
        }
        if (g_verbose) {
            if (g_verbose > 1) {
                char zbuf [256];
                sprintf(zbuf, "command=%s response=%s %02x len=%d", g_cmd, ism_mqtt_mqttCommand(kind), kind, len);
                ism_common_traceData(zbuf, 0x08, "", 0, buf, len, len);
            } else {
                printf("command=%s response=%s %02x len=%d\n", g_cmd, ism_mqtt_mqttCommand(kind), kind, len);
            }
        }
        g_cmd_result = kind;
        g_cmd = NULL;
        g_retlen = len;
        if (len > sizeof g_retbuf)
            len = sizeof g_retbuf;
        if (len < 0)
            len = 0;
        memcpy(g_retbuf, buf, len);
    }
    return 0;
}

/*
 * Free up the transports created for a test
 */
void my_freeTransport(ism_transport_t * transport, ism_transport_t * stransport) {
    struct suballoc_t * suba;

    if (transport) {
        suba = transport->suballoc.next;
        while (suba) {
            struct suballoc_t * freesub = suba;
            suba = suba->next;
            freesub->next = NULL;
            ism_common_free_anyType(freesub);
        }
    }
    if (stransport) {
        suba = stransport->suballoc.next;
        while (suba) {
            struct suballoc_t * freesub = suba;
            suba = suba->next;
            freesub->next = NULL;
            ism_common_free_anyType(freesub);
        }
        free(stransport);
    }
}

/*
 * Send on packet to the MQTT receive method
 */
static int mqttRx_test(ism_transport_t * transport, char * cmd_buf, int buflen, int cmd, int check_response) {
    g_cmd = NULL;
    g_cmd_result = -1;

    char * l_cmd_buf = NULL;

    g_entered_close_cb = 0;

    l_cmd_buf = alloca(buflen);
    memcpy(l_cmd_buf,  cmd_buf, buflen);
    if (check_response) {
        g_cmd = ism_mqtt_mqttCommand((uint8_t)(cmd>>4));
    }

    int rc = ism_mqtt_receive(transport, l_cmd_buf, buflen, cmd);
    if (rc && g_verbose>1) {
        char xbuf[256];
        xbuf[0] = 0;
        printf("Line=%d rc=%d %s\n", transport->endpoint->port, rc, ism_common_getErrorString(rc, xbuf, sizeof xbuf));
    }
    return rc;
}

/*
 * Test endpoint
 */
static ism_endstat_t g_endpointStat ={0};
static ism_endpoint_t g_testEndpoint = {
    .name         = "!Test",
    .ipaddr       = "",
    .transport_type = "TCP",
    .protomask    = PMASK_Internal,
    .thread_count = 1,
    .separator    = '2',
    .clientclass = "iot2",
};

/*
 * Setu8p
 */
static void setupTransport(ism_transport_t * transport, ism_transport_t * stransport, int line) {
    g_testEndpoint.port = line;

    g_testEndpoint.stats = &g_endpointStat;
    if (g_verbose > 1)
        printf("setupTransport line=%d\n", line);
    transport->protocol = "mqtt-tcp";
    transport->close = mclose;
    transport->closed = mclosed;
    transport->send = msend;
    transport->trclevel = ism_defaultTrace;
    ism_mqtt_connection(transport);
    transport->serverport = line;
    transport->pobj->server_transport = stransport;
    transport->endpoint = &g_testEndpoint;

    if (stransport) {
        stransport->protocol = "mqtt-tcp";
        stransport->close = mclose;
        stransport->closed = mclosed;
        stransport->send = msend;
        stransport->trclevel = ism_defaultTrace;
        ism_mqtt_connection(stransport);
        stransport->serverport = line;
        stransport->pobj->client_transport = transport;
        stransport->endpoint = &g_testEndpoint;
        stransport->pobj->connectPending = 1;
        stransport->originated = 1;
    }
}


/*
 * Get the props from an MQTTv5 CONNECT
 */
static int getConnect5Props(const char * buf, int len, const char * * xprops) {
    int proplen = 0;
    int pxprot = 0;
    const char * props;
    if (len > 15) {
        int plen = BIGINT16(buf);
        if (plen <= 6) {
            if (plen == 6 && buf[6]=='p')
                pxprot = 1;
            len -= (plen + 2);
            buf += (plen + 2);

            if (*buf == 5) {
                int vilen;
                len -= (4 + pxprot);
                buf += (4 + pxprot);
                proplen = ism_common_getMqttVarIntExp(buf, len, &vilen);
                if (proplen < 0) {
                    proplen = 0;
                } else {
                    props = buf+vilen;
                }
            }
        }
    }
    if (xprops)
        *xprops = props;
    return proplen;
}

/*
 * Get the props from an MQTTv5 DISCONNECT
 */
static int getDisconnect5Props(const char * buf, int len, const char * * xprops) {
    int proplen = 0;
    const char * props;
    if (len > 1) {
        concat_alloc_t rbuf = {(char *)buf+1, len-1, len-1};
        proplen = ism_common_getMqttVarInt(&rbuf);
        if (proplen < 0) {
            proplen = 0;
        } else {
            props = buf+rbuf.pos+1;
        }
    }
    if (xprops)
        *xprops = props;
    return proplen;
}

/*
 * Simple connection string for MQTTv3.1.1
 */
xUNUSED static int setupConnection4(char * * conn_buf) {
    *conn_buf = "\0\4MQTT\4\0\0\0\0\7d:x:t:i";
    return 19;
}

/*
 * Simple connection string for MQTTv3.1
 */
xUNUSED static int setupConnection3(char * * conn_buf) {
    *conn_buf = "\0\6MQIsdp\3\0\0\0\7d:x:t:i";
    return 21;
}

/*
 * Simple connection string for MQTTv5
 */
xUNUSED static int setupConnection5(char * * conn_buf) {
    *conn_buf = "\0\4MQTT\5\0\0\0\0\0\7d:x:t:i";
    return 20;
}

/*
 * Make a simple tenant
 * This can later be modified
 */
static ism_tenant_t * makeTenant(const char * name, const char * rule) {
    ism_tenant_t * tenant;
    tenant = ism_tenant_getTenant(name);
    if (tenant)
        unlinkTenant(tenant);
    tenant = calloc(1, sizeof(ism_tenant_t));
    strcpy(tenant->structid, "IoTTEN");
    tenant->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_tenant,1000),name);
    tenant->namelen = (int)strlen(name);
    tenant->check_user = 0;   /* Defaults true */
    tenant->allow_anon = 1;
    tenant->allow_durable = 1;
    tenant->remove_user = 1;  /* Defaults true */
    tenant->enabled = 1;      /* Defaults true */
    tenant->max_qos = 2;
    tenant->topicrule = ism_proxy_getTopicRule(rule);
    linkTenant(tenant);
    return tenant;
}

/*
 * Test a simple connect
 */
static void mqttv5_connect(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_transport_t * stransport = calloc(1, sizeof(ism_transport_t));
    char * conn_buf;
    setupTransport(transport, stransport, __LINE__);
    conn_buf = "\0\4MQTT\5\xC0\0\7"       /* v5 user,pw keepalive=7 */
               "\0"                       /* No properties */
               "\0\7d:x:t:i"              /* ClientID */
               "\0\0\0\0";                /* Zero length userid and password */

    CU_ASSERT(mqttRx_test(transport, conn_buf, 24, MT_CONNECT<<4, 1) == 0);
    CU_ASSERT(transport->pobj->mqtt_version == 5);
    CU_ASSERT(transport->name && !strcmp(transport->name, "d:x:t:i"));
    CU_ASSERT(g_cmd_result = MT_CONNECT);
    CU_ASSERT(transport->pobj->cleansession == 0);
    CU_ASSERT(transport->userid != NULL && *transport->userid == 0);
    CU_ASSERT(mqttRx_test(transport, conn_buf, 24, MT_CONNECT<<4, 1) == ISMRC_ConnectFirst);
    my_freeTransport(transport, stransport);
}

/*
 * Test a non-durable connect
 */
static void mqttv5_connectND(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_transport_t * stransport = calloc(1, sizeof(ism_transport_t));
    char * conn_buf;
    const char * props;
    int proplen;
    setupTransport(transport, stransport, __LINE__);
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw keepalive=7 */
               "\5\x11\0\0\0\0"           /* expire = 0 */
               "\0\7d:x:t:i" ;            /* ClientID */

    CU_ASSERT(mqttRx_test(transport, conn_buf, 25, MT_CONNECT<<4, 1) == 0);
    CU_ASSERT(g_cmd_result = MT_CONNECT);
    CU_ASSERT(transport->pobj->mqtt_version == 5);
    CU_ASSERT(transport->name && !strcmp(transport->name, "d:x:t:i"));
    CU_ASSERT(transport->pobj->cleansession == 1);
    CU_ASSERT(transport->userid && *transport->userid == 0);
    proplen = getConnect5Props(g_retbuf, g_retlen, &props);
    CU_ASSERT(proplen > 0);
    if (proplen > 0) {
        int sexpire = ism_common_getMqttPropField(props, proplen, MPI_SessionExpire, g_ctx5, -3);
        CU_ASSERT(sexpire == -3 || sexpire == 0);
    }
    my_freeTransport(transport, stransport);
}

/*
 * Test a connect with an expire
 */
static void mqttv5_connectExp(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_transport_t * stransport = calloc(1, sizeof(ism_transport_t));
    char * conn_buf;
    const char * props;
    int proplen;
    setupTransport(transport, stransport, __LINE__);
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\5\x11\0\0\1\0"           /* expire = 0x100 */
               "\0\7d:x:t:i" ;            /* ClientID */

    CU_ASSERT(mqttRx_test(transport, conn_buf, 25, MT_CONNECT<<4, 1) == 0);
    CU_ASSERT(g_cmd_result == (MT_CONNECT<<4));
    CU_ASSERT(transport->pobj->mqtt_version == 5);
    CU_ASSERT(transport->name && !strcmp(transport->name, "d:x:t:i"));
    proplen = getConnect5Props(g_retbuf, g_retlen, &props);
    CU_ASSERT(proplen > 0);
    if (proplen > 0) {
        int sexpire = ism_common_getMqttPropField(props, proplen, MPI_SessionExpire, g_ctx5, -3);
        CU_ASSERT(sexpire == 256);
    }
    transport->pobj->inprogress = 0;
    transport->pobj->connectdata = NULL;
    conn_buf = "\0\5\x11\0\0\2\0";    /* rc=0, expire=512 */
    CU_ASSERT(mqttRx_test(transport, conn_buf, 7, MT_DISCONNECT<<4, 1) == 0);
    CU_ASSERT(g_cmd_result == (MT_DISCONNECT<<4));
    proplen = getDisconnect5Props(g_retbuf, g_retlen, &props);
    CU_ASSERT(proplen == 5);
    if (proplen > 0) {
        int sexpire = ism_common_getMqttPropField(props, proplen, MPI_SessionExpire, g_ctx5, -3);
        CU_ASSERT(sexpire == 512);
    }
    my_freeTransport(transport, stransport);
}

/*
 * Test sending an auth packet which should fail
 */
static void mqttv5_auth(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_transport_t * stransport = calloc(1, sizeof(ism_transport_t));
    int  rc;
    char * conn_buf;
    setupTransport(transport, stransport, __LINE__);
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\x05\x11\0\0\0\0"         /* expire = 0  */
               "\0\7d:x:t:i";              /* ClientID */

    CU_ASSERT(mqttRx_test(transport, conn_buf, 25, MT_CONNECT<<4, 1) == 0);
    conn_buf = "\x18\x08\x15\0\5PLAIN";
    CU_ASSERT((rc = mqttRx_test(transport, conn_buf, 10, 0xF0, /* MT_AUTH */ 1)) == ISMRC_NotAuthorized);
    if (g_verbose)
        printf("Line=%d rc=%d\n", __LINE__, rc);
    CU_ASSERT((rc = mqttRx_test(transport, conn_buf, 10, 0xF0, /* MT_AUTH */ 1)) == ISMRC_Closed);
    my_freeTransport(transport, stransport);
}

/*
 * Test a connect with an AuthMethod (this fails as we do not support enhanced auth)
 */
static void mqttv5_authMethod(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_transport_t * stransport = calloc(1, sizeof(ism_transport_t));
    char * conn_buf;
    setupTransport(transport, stransport, __LINE__);
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\x0D\x11\0\0\2\0\x15\0\5PLAIN"     /* expire = 512 authmethod=plain */
               "\0\7d:x:t:i";              /* ClientID */

    CU_ASSERT(mqttRx_test(transport, conn_buf, 33, MT_CONNECT<<4, 1) == ISMRC_NotAuthorized);
    my_freeTransport(transport, stransport);
}

/*
 * Test a connect with an AuthData (this fails as we do not support enhanced auth)
 */
static void mqttv5_authData(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_transport_t * stransport = calloc(1, sizeof(ism_transport_t));
    char * conn_buf;
    setupTransport(transport, stransport, __LINE__);
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\x0D\x11\0\0\2\0\x16\0\5abcde"     /* expire = 512 authmethod=plain */
               "\0\7d:x:t:i";              /* ClientID */

    CU_ASSERT(mqttRx_test(transport, conn_buf, 33, MT_CONNECT<<4, 1) == ISMRC_NotAuthorized);
    my_freeTransport(transport, stransport);
}



/* Test will topic removal with buflen=23, version byte set to 3, clean session set to 1, will flag set to 1,
 * will QoS set to 1, will topic length set to 2, will topic "wt",
 * will message length set to 0 with no will message.
 * Validation:
 * After the will flags removal connect flags 0x0E(1110) should get updated to 0010 (Having set with clean session).
 * Should pass.
 */
static void mqtt_pass_removeWillmsg(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_transport_t * stransport = calloc(1, sizeof(ism_transport_t));
    setupTransport(transport, stransport, __LINE__);
    char conn_buf[100] = {0};
    conn_buf[0] = 0;
    conn_buf[1] = 23;
    memcpy(conn_buf+2, "\0\6MQIsdp", 8);

    // Version
    conn_buf[10] = 3;
    // Flags
    conn_buf[11] = 0x0E;
    // Keepalive
    conn_buf[12] = 255;
    conn_buf[13] = 255;
    // Client ID length
    conn_buf[14] = 0;
    conn_buf[15] = 1;
    // Client ID
    conn_buf[16] = 'c';
    // Will topic lentgh
    conn_buf[17] = 0;
    conn_buf[18] = 2;
    // Will topic
    conn_buf[19] = 'w';
    conn_buf[20] = 't';
    // Will message lentgh
    conn_buf[21] = 0;
    conn_buf[22] = 2;
    // Will message
    conn_buf[23] = 'w';
    conn_buf[24] = 'm';
    transport->pobj->connectdata = (char*) malloc(25 * sizeof(char));
    memcpy(transport->pobj->connectdata, conn_buf, 25);
    transport->pobj->connectlen = 25;
    removeWill(transport);
    CU_ASSERT(transport->pobj->connectdata[11] == 2);
    my_freeTransport(transport, stransport);
}

static void mqtt_pass_cleansessionTrue(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_transport_t * stransport = calloc(1, sizeof(ism_transport_t));
    setupTransport(transport, stransport, __LINE__);
    char conn_buf[100] = {0};
    conn_buf[0] = 0;
    conn_buf[1] = 23;
    memcpy(conn_buf+2, "\0\6MQIsdp", 8);

    // Version
    conn_buf[10] = 3;
    // Flags
    conn_buf[11] = 6;
    // Keepalive
    conn_buf[12] = 255;
    conn_buf[13] = 255;
    // Client ID length
    conn_buf[14] = 0;
    conn_buf[15] = 1;
    // Client ID
    conn_buf[16] = 'c';
    // Will topic lentgh
    conn_buf[17] = 0;
    conn_buf[18] = 2;
    // Will topic
    conn_buf[19] = 'w';
    conn_buf[20] = 't';
    // Will message lentgh
    conn_buf[21] = 0;
    conn_buf[22] = 2;
    // Will message
    conn_buf[23] = 'w';
    conn_buf[24] = 'm';
    transport->pobj->connectdata = (char*) malloc(25 * sizeof(char));
    memcpy(transport->pobj->connectdata, conn_buf, 25);
    transport->pobj->connectlen = 25;
    removeWill(transport);
    CU_ASSERT(transport->pobj->connectdata[11] == 2);
    my_freeTransport(transport, stransport);
}

XAPI void ism_ssl_init(int useFips, int useBufferPool);
XAPI int ism_ssl_SNI_init(void);
XAPI int ism_ssl_test_addWaiter(ima_transport_info_t * transport, const char * org);


void stopCrlTest(void) {
    ism_transport_t * transport1 = calloc(1, sizeof(ism_transport_t));
    ism_transport_t * transport2 = calloc(1, sizeof(ism_transport_t));
    setupTransport(transport1, NULL, __LINE__);
    transport1->org = "fred";
    transport1->crtChckStatus = 9;
    setupTransport(transport2, NULL, __LINE__);
    transport2->org = "fred";
    transport2->crtChckStatus = 9;

    ism_ssl_init(0, 0);
    ism_ssl_SNI_init();

    ism_ssl_setSNIConfig("fred", NULL, NULL, NULL, NULL, 9, NULL);   /* Create test orgConfig */
    CU_ASSERT(ism_ssl_stopCrlWait(transport1, "fred") == 0);

    ism_ssl_test_addWaiter((ima_transport_info_t *)transport1, "fred");
    ism_ssl_test_addWaiter((ima_transport_info_t *)transport2, "fred");
    CU_ASSERT(ism_ssl_stopCrlWait(transport2, "fred") == 1);
    CU_ASSERT(ism_ssl_stopCrlWait(transport1, "fred") == 1);
    CU_ASSERT(ism_ssl_stopCrlWait(transport2, "fred") == 0);
    CU_ASSERT(ism_ssl_stopCrlWait(transport1, "fred") == 0);

    ism_ssl_test_addWaiter((ima_transport_info_t *)transport1, "fred");
    ism_ssl_test_addWaiter((ima_transport_info_t *)transport2, "fred");
    transport1->crtChckStatus = 0;
    transport2->crtChckStatus = 0;
    CU_ASSERT(ism_ssl_stopCrlWait(transport2, "fred") == 0);
    CU_ASSERT(ism_ssl_stopCrlWait(transport1, "fred") == 0);
    transport1->crtChckStatus = 9;
    transport2->crtChckStatus = 9;
    CU_ASSERT(ism_ssl_stopCrlWait(transport1, "fred") == 1);
    CU_ASSERT(ism_ssl_stopCrlWait(transport1, "sam") == 0);

    my_freeTransport(transport1, NULL);
    my_freeTransport(transport2, NULL);
}

/**
 * Test Revalidation of SaveData buffer
 *
 */

void testRevalidateSaveDataForDevice(void) {
    char xbuf[2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    const char * topic;
    char * payload;
    int topiclen;
    int payloadlen;

    int cmd;
    int rc;

    //Turn Off Kafka publishing
    g_msgRoutingEnabled=0;
    g_useKafkaIMMessaging=0;
    g_mhubEnabled=0;
    g_cmd = NULL;
    g_cmd_result = -1;

    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_transport_t * stransport = calloc(1, sizeof(ism_transport_t));
    setupTransport(transport, stransport, __LINE__);
    transport->pobj->mqtt_version = 4;
    transport->tenant = makeTenant("x", "iot2");
    transport->org = "x";
    transport->client_class = 'd';
    transport->name=transport->clientID="d:x:devtype1:devid1";
    transport->pobj->connectlen=1; //Assumed connect is done.

    /* QoS = 0 MQTTv3.1.1 */
    topic = "iot-2/evt/testevent0/fmt/json";
    payload = "This is the payload";
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t  xpmsg1 = {buf.buf+16, buf.used-16, cmd};
    mqtt_pmsg_t * pmsg = &xpmsg1;

    rc = ism_mqtt_getPublishPayload(transport, pmsg);

    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload, payloadlen));

    //Test: Validate the content that sent and received.
    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);
    g_cmd = ism_mqtt_mqttCommand((uint8_t)(cmd>>4));
    int xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);

    mqtt_pmsg_t  xpmsg10 = {g_retbuf, sizeof g_retbuf, cmd};
    mqtt_pmsg_t * pmsg2 = &xpmsg10;

    //Validate the received content which sent by revalidate method
    rc = ism_mqtt_getPublishPayload(transport, pmsg2);
    CU_ASSERT(transport->pobj->savelen-2 == g_retlen); //-2 for the MQTTframe at the beginning of the received buffer
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload, payloadlen));

    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;



    /* QoS = 1 MQTTv3.1.1 */
    topic = "this/is/a/topic";
    payload = "This is the payload";
    cmd = MT_PUBLISH<<4 | 2;   /* QoS = 1 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    buf.used += 2;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t  xpmsg2 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg2;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 0);
    CU_ASSERT(pmsg->props == NULL);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload, payloadlen));

    /* QoS=0 MQTTv5 proplen=0 */
    transport->pobj->mqtt_version = 5;
    topic = "iot-2/evt/event1/fmt/json";
    payload = "This is the payload";
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    xbuf[buf.used++] = 0;    /* proplen */
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t  xpmsg3 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg3;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 0);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));


    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    /* QoS=0 MQTTv5 proplen=147 */
    topic = "iot-2/evt/events2/fmt/json";
    int len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg4 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg4;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    g_msg_received_len=0;
    g_msg_received=0;
    //printf("\ntestRevalidateSaveData: savedatalen=%d savedatacount=%d\n", transport->pobj->savelen, transport->pobj->savecount);
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);

    //printf("\ntestRevalidateSaveData: savedatalen=%d savedatacount=%d revalidate_rc=%d g_msg_received=%d g_msg_received_len=%d\n",
    //        transport->pobj->savelen, transport->pobj->savecount, xrc, g_msg_received, g_msg_received_len);
    //Test: Check if the number of msgs sent and the length to make sure that total received are matched.
    CU_ASSERT(xrc == 0);
    CU_ASSERT(g_msg_received == transport->pobj->savecount);
    CU_ASSERT(g_msg_received_len == transport->pobj->savelen);

    //Bad Topic Test
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-3/evt/events2/fmt/json";
    len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg11 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg11;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    /* QoS=0 MQTTv5 proplen=147 */
    /* INvalid Topic. Expect no msg sent if the first message is not authorized*/
    topic = "iot-2/evt/events2/fmt/json";
    len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg12 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg12;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);

    CU_ASSERT(xrc != 0);
    CU_ASSERT(g_msg_received == 0);
    CU_ASSERT(g_msg_received_len == 0);

    //Test when invalid buffer is after the first one
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/evt/events2/fmt/json";
    len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg13 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg13;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    /* QoS=0 MQTTv5 proplen=147 */
    /* INvalid Topic. Expect no msg sent if the first message is not authorized*/
    topic = "iot-3/evt/events2/fmt/json";
    len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg14 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg14;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);

    CU_ASSERT(xrc != 0);
    CU_ASSERT(g_msg_received == 1);
    CU_ASSERT(g_msg_received_len != transport->pobj->savelen);
    //Test with lowest permission
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/evt/events2/fmt/json";
    len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg20 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg20;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    //Set Transport permission to lowest
    int old_permissions =  transport->auth_permissions;
    transport->auth_permissions=0;
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d\n", xrc, g_msg_received, g_msg_received_len);
    //Set Transport permission back
    transport->auth_permissions=old_permissions;

    CU_ASSERT(xrc == ISMRC_BadTopic);
    CU_ASSERT(g_msg_received == 0);
    CU_ASSERT(g_msg_received_len != transport->pobj->savelen);

    //Test SUBSCRIBE with mqttv5
    transport->pobj->mqtt_version = 5;
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/cmd/+/fmt/+";
    topiclen = strlen(topic);
    cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
    cmd |= 0xF;
    buf.used = 18;
    //Put MsgID
    xbuf[16] = (uint8_t)3>>8;
    xbuf[17] = (uint8_t)3;
    //ism_common_allocBufferCopyLen(&buf, msgID, 3);

    //Put Variable Props
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;

    //ADD Topic Filters
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //buf.used+=1; //Add Sub Option Byte
    int savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    int qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    int extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    int totalexlen = 0;
    totalexlen += (extlen+2);

    //ADD Topic Filters
    topic = "iot-2/mon";
    topiclen = strlen(topic);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen += (extlen+2); //+2 for the 2-byte len hodler

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension
    //Set Transport permission to lowest
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
    //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

    CU_ASSERT(xrc == ISMRC_OK);
    CU_ASSERT(g_msg_received == 1);
    CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);




    //Test SUBSCRIBE with mqttv5 with No Permission
    transport->pobj->mqtt_version = 5;
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/cmd/+/fmt/+";
    topiclen = strlen(topic);
    cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
    cmd |= 0xF;
    buf.used = 18;
    //Put MsgID
    xbuf[16] = (uint8_t)3>>8;
    xbuf[17] = (uint8_t)3;
    //ism_common_allocBufferCopyLen(&buf, msgID, 3);

    //Put Variable Props
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;

    //ADD Topic Filters
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen = 0;
    totalexlen += (extlen+2);

    //ADD Topic Filters
    topic = "iot-2/mon";
    topiclen = strlen(topic);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen += (extlen+2); //+2 for the 2-byte len hodler

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension

    old_permissions =  transport->auth_permissions;
    transport->auth_permissions=0;
    //Set Transport permission to lowest
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    transport->auth_permissions=old_permissions;
    //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
    //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

    //Expect xrc is OK and 1 Msgs received for the SUBACK since this is MQTTv5, and all filters are bad
    CU_ASSERT(xrc == ISMRC_OK);
    CU_ASSERT(g_msg_received == 1);
    //CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);


    //Test SUBSCRIBE with mqttv3.1.1
    transport->pobj->mqtt_version = 4;
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/cmd/+/fmt/+";
    topiclen = strlen(topic);
    cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
    cmd |= 0xF;
    buf.used = 18;
    //Put MsgID
    xbuf[16] = (uint8_t)3>>8;
    xbuf[17] = (uint8_t)3;
    //ism_common_allocBufferCopyLen(&buf, msgID, 3);

    //Put Variable Props
    //ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    //buf.used += 147;

    //ADD Topic Filters
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen = 0;
    totalexlen += (extlen+2);

    //ADD Topic Filters
    topic = "iot-2/mon";
    topiclen = strlen(topic);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen += (extlen+2); //+2 for the 2-byte len hodler

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension
    //Set Transport permission to lowest
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
    //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

    CU_ASSERT(xrc == ISMRC_OK);
    CU_ASSERT(g_msg_received == 1);
    CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);


    //Test SUBSCRIBE with mqttv3.1.1 with No Permission
    transport->pobj->mqtt_version = 4;
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/cmd/+/fmt/+";
    topiclen = strlen(topic);
    cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
    cmd |= 0xF;
    buf.used = 18;
    //Put MsgID
    xbuf[16] = (uint8_t)3>>8;
    xbuf[17] = (uint8_t)3;
    //ism_common_allocBufferCopyLen(&buf, msgID, 3);

    //Put Variable Props
    //ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    //buf.used += 147;

    //ADD Topic Filters
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen = 0;
    totalexlen += (extlen+2);

    //ADD Topic Filters
    topic = "iot-2/mon";
    topiclen = strlen(topic);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen += (extlen+2); //+2 for the 2-byte len hodler

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension

    old_permissions =  transport->auth_permissions;
    transport->auth_permissions=0;
    //Set Transport permission to lowest
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    transport->auth_permissions=old_permissions;
    //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
    //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

    //Expect XRC is non zero and no msgs sent
    CU_ASSERT(xrc != ISMRC_OK);
    CU_ASSERT(g_msg_received == 0);
    //CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);


    //Test SUBSCRIBE with mqttv5 with One or More Filters invalid
   transport->pobj->mqtt_version = 5;
   g_msg_received_len=0;
   g_msg_received=0;
   transport->pobj->savedata=NULL;
   transport->pobj->savelen=0;
   transport->pobj->savecount=0;


   cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
   cmd |= 0xF;
   buf.used = 18;
   //Put MsgID
   xbuf[16] = (uint8_t)3>>8;
   xbuf[17] = (uint8_t)3;
   //ism_common_allocBufferCopyLen(&buf, msgID, 3);

   //Put Variable Props
   ism_common_putMqttVarInt(&buf, 147);   /* proplen */
   buf.used += 147;

   //ADD Topic Filters
   //xbuf[buf.used]=(uint8_t)topiclen>>8;
   //xbuf[buf.used+1] = (uint8_t)topiclen;
   //buf.used+=2;
   //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
   topic = "iot-2/cmd/+/fmt/+";
   topiclen = strlen(topic);
   convertTopic(transport, &buf, topic, topiclen, 1);
   //buf.used+=1; //Add Sub Option Byte
   savepos = buf.used;
   buf.used += 2;    /* Skip over len, add later */
   /* Put out the QoS byte */
   qos=1;
   ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

   extlen = buf.used - savepos - 2;
   buf.buf[savepos] = (char)(extlen<<8);
   buf.buf[savepos+1] = (char)extlen;
   totalexlen = 0;
   totalexlen += (extlen+2);

   //ADD Topic Filters
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    topic = "iot-2/cmd/cmand2/fmt/+";
    topiclen = strlen(topic);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen = 0;
    totalexlen += (extlen+2);

   //ADD Topic Filters
   topic = "iot-2/mon";
   topiclen = strlen(topic);
   convertTopic(transport, &buf, topic, topiclen, 1);
   //xbuf[buf.used]=(uint8_t)topiclen>>8;
   //xbuf[buf.used+1] = (uint8_t)topiclen;
   //buf.used+=2;
   //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
   //buf.used+=1; //Add Sub Option Byte
   savepos = buf.used;
   buf.used += 2;    /* Skip over len, add later */
   /* Put out the QoS byte */
   qos=1;
   ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

   extlen = buf.used - savepos - 2;
   buf.buf[savepos] = (char)(extlen<<8);
   buf.buf[savepos+1] = (char)extlen;
   totalexlen += (extlen+2); //+2 for the 2-byte len hodler

   appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

   totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension

   old_permissions =  transport->auth_permissions;
   transport->auth_permissions=8;
   //Set Transport permission to lowest
   xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
   transport->auth_permissions=old_permissions;
   //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
   //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

   //Expect Only one message receive
   CU_ASSERT(xrc == ISMRC_OK);
   CU_ASSERT(g_msg_received == 1);
   //CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);


   //Test SUBSCRIBE with mqttv5 with One or More Filters invalid
     transport->pobj->mqtt_version = 4;
     g_msg_received_len=0;
     g_msg_received=0;
     transport->pobj->savedata=NULL;
     transport->pobj->savelen=0;
     transport->pobj->savecount=0;


     cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
     cmd |= 0xF;
     buf.used = 18;
     //Put MsgID
     xbuf[16] = (uint8_t)3>>8;
     xbuf[17] = (uint8_t)3;
     //ism_common_allocBufferCopyLen(&buf, msgID, 3);

     //Put Variable Props
     //ism_common_putMqttVarInt(&buf, 147);   /* proplen */
     //buf.used += 147;

     //ADD Topic Filters
     //xbuf[buf.used]=(uint8_t)topiclen>>8;
     //xbuf[buf.used+1] = (uint8_t)topiclen;
     //buf.used+=2;
     //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
     topic = "iot-2/cmd/+/fmt/+";
     topiclen = strlen(topic);
     convertTopic(transport, &buf, topic, topiclen, 1);
     //buf.used+=1; //Add Sub Option Byte
     savepos = buf.used;
     buf.used += 2;    /* Skip over len, add later */
     /* Put out the QoS byte */
     qos=1;
     ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

     extlen = buf.used - savepos - 2;
     buf.buf[savepos] = (char)(extlen<<8);
     buf.buf[savepos+1] = (char)extlen;
     totalexlen = 0;
     totalexlen += (extlen+2);

     //ADD Topic Filters
      //xbuf[buf.used]=(uint8_t)topiclen>>8;
      //xbuf[buf.used+1] = (uint8_t)topiclen;
      //buf.used+=2;
      //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
      topic = "iot-2/cmd/cmand2/fmt/+";
      topiclen = strlen(topic);
      convertTopic(transport, &buf, topic, topiclen, 1);
      //buf.used+=1; //Add Sub Option Byte
      savepos = buf.used;
      buf.used += 2;    /* Skip over len, add later */
      /* Put out the QoS byte */
      qos=1;
      ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

      extlen = buf.used - savepos - 2;
      buf.buf[savepos] = (char)(extlen<<8);
      buf.buf[savepos+1] = (char)extlen;
      totalexlen = 0;
      totalexlen += (extlen+2);

     //ADD Topic Filters
     topic = "iot-2/mon";
     topiclen = strlen(topic);
     convertTopic(transport, &buf, topic, topiclen, 1);
     //xbuf[buf.used]=(uint8_t)topiclen>>8;
     //xbuf[buf.used+1] = (uint8_t)topiclen;
     //buf.used+=2;
     //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
     //buf.used+=1; //Add Sub Option Byte
     savepos = buf.used;
     buf.used += 2;    /* Skip over len, add later */
     /* Put out the QoS byte */
     qos=1;
     ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

     extlen = buf.used - savepos - 2;
     buf.buf[savepos] = (char)(extlen<<8);
     buf.buf[savepos+1] = (char)extlen;
     totalexlen += (extlen+2); //+2 for the 2-byte len hodler

     appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

     totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension

     old_permissions =  transport->auth_permissions;
     transport->auth_permissions=8;
     //Set Transport permission to lowest
     xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
     transport->auth_permissions=old_permissions;
     //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
     //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

     //Expect non zero RC and No msgs received
     CU_ASSERT(xrc != ISMRC_OK);
     CU_ASSERT(g_msg_received == 0);
     //CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);

    my_freeTransport(transport, stransport);
}

/**
 * Test Revalidation of SaveData buffer for GW
 *
 */
void testRevalidateSaveDataForGateway(void) {
    char xbuf[2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    const char * topic;
    char * payload;
    int topiclen;
    int payloadlen;

    int cmd;
    int rc;

    //Turn Off Kafka publishing
    g_msgRoutingEnabled=0;
    g_useKafkaIMMessaging=0;
    g_mhubEnabled=0;
    g_cmd = NULL;
    g_cmd_result = -1;

    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_transport_t * stransport = calloc(1, sizeof(ism_transport_t));
    setupTransport(transport, stransport, __LINE__);
    transport->pobj->mqtt_version = 4;
    transport->tenant = makeTenant("x", "iot2");
    transport->org = "x";
    transport->client_class = 'g';
    transport->name=transport->clientID="g:x:gwtype1:gwid1";
    transport->pobj->connectlen=1; //Assumed connect is done.

    /* QoS = 0 MQTTv3.1.1 */
    topic = "iot-2/x/type/type1/id/id1/evt/testevent0/fmt/json";
    payload = "This is the payload";
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t  xpmsg1 = {buf.buf+16, buf.used-16, cmd};
    mqtt_pmsg_t * pmsg = &xpmsg1;

    rc = ism_mqtt_getPublishPayload(transport, pmsg);

    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload, payloadlen));

    //Test: Validate the content that sent and received.
    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);
    g_cmd = ism_mqtt_mqttCommand((uint8_t)(cmd>>4));
    int xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);

    //printf("testRevalidateSaveDataForGateway: savedatalen=%d g_retlen=%d\n", transport->pobj->savelen, g_retlen);
    mqtt_pmsg_t  xpmsg10 = {g_retbuf, sizeof g_retbuf, cmd};
    mqtt_pmsg_t * pmsg2 = &xpmsg10;


    //Validate the received content which sent by revalidate method
    rc = ism_mqtt_getPublishPayload(transport, pmsg2);
    CU_ASSERT(transport->pobj->savelen-2 == g_retlen); //-2 for the MQTTframe at the beginning of the received buffer
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload, payloadlen));

    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;



    /* QoS = 1 MQTTv3.1.1 */
    topic = "this/is/a/topic";
    payload = "This is the payload";
    cmd = MT_PUBLISH<<4 | 2;   /* QoS = 1 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    buf.used += 2;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t  xpmsg2 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg2;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 0);
    CU_ASSERT(pmsg->props == NULL);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload, payloadlen));

    /* QoS=0 MQTTv5 proplen=0 */
    transport->pobj->mqtt_version = 5;
    topic = "iot-2/x/type/type1/id/id1/evt/event1/fmt/json";
    payload = "This is the payload";
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    xbuf[buf.used++] = 0;    /* proplen */
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t  xpmsg3 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg3;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 0);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));


    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    /* QoS=0 MQTTv5 proplen=147 */
    topic = "iot-2/x/type/type1/id/id1/evt/events2/fmt/json";
    int len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg4 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg4;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    g_msg_received_len=0;
    g_msg_received=0;
    //printf("\ntestRevalidateSaveData: savedatalen=%d savedatacount=%d\n", transport->pobj->savelen, transport->pobj->savecount);
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);

    //printf("\ntestRevalidateSaveData: savedatalen=%d savedatacount=%d revalidate_rc=%d g_msg_received=%d g_msg_received_len=%d\n",
    //        transport->pobj->savelen, transport->pobj->savecount, xrc, g_msg_received, g_msg_received_len);
    //Test: Check if the number of msgs sent and the length to make sure that total received are matched.
    CU_ASSERT(xrc == 0);
    CU_ASSERT(g_msg_received == transport->pobj->savecount);
    CU_ASSERT(g_msg_received_len == transport->pobj->savelen);

    //Bad Topic Test
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-3/x/type/type1/id/id1/evt/events2/fmt/json";
    len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg11 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg11;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    /* QoS=0 MQTTv5 proplen=147 */
    /* INvalid Topic. Expect no msg sent if the first message is not authorized*/
    topic = "iot-2/x/type/type1/id/id1/evt/events2/fmt/json";
    len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg12 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg12;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));
    int prev_savedatalen = transport->pobj->savelen;
    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);
    int comp_savelen = transport->pobj->savelen - prev_savedatalen;

    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    //printf("\ntestRevalidateSaveData: savedatalen=%d savedatacount=%d comp_savelen=%d revalidate_rc=%d g_msg_received=%d g_msg_received_len=%d\n",
    //            transport->pobj->savelen, transport->pobj->savecount, comp_savelen ,xrc, g_msg_received, g_msg_received_len);

    CU_ASSERT(xrc == 0);
    CU_ASSERT(g_msg_received == 1);
    CU_ASSERT(g_msg_received_len  == comp_savelen); //-3 from g_msg_received_len for the frame bytes

    //Test when invalid buffer is after the first one
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/x/type/type1/id/id1/evt/events2/fmt/json";
    len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg13 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg13;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    /* QoS=0 MQTTv5 proplen=147 */
    /* INvalid Topic. Expect no msg sent if the first message is not authorized*/
    topic = "iot-3/x/type/type1/id/id1/evt/events2/fmt/json";
    len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg14 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg14;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    prev_savedatalen = transport->pobj->savelen;
    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);
    comp_savelen =  prev_savedatalen;

    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    //printf("\ntestRevalidateSaveData: savedatalen=%d savedatacount=%d comp_savelen=%d revalidate_rc=%d g_msg_received=%d g_msg_received_len=%d\n",
    //            transport->pobj->savelen, transport->pobj->savecount, comp_savelen ,xrc, g_msg_received, g_msg_received_len);


    CU_ASSERT(xrc == 0);
    CU_ASSERT(g_msg_received == 1);
    CU_ASSERT(g_msg_received_len == comp_savelen);


    //Test with lowest permission
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/x/type/type1/id/id1/evt/events2/fmt/json";
    len=1024;
    payload = alloca(len); //Create large payload so the MQTTFrame goes beyond 2 bytes
    memset(payload, 'z', len);
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg20 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg20;
    rc = ism_mqtt_getPublishPayload(transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    //Set Transport permission to lowest
    int old_permissions =  transport->auth_permissions;
    transport->auth_permissions=0;
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d\n", xrc, g_msg_received, g_msg_received_len);
    //Set Transport permission back
    transport->auth_permissions=old_permissions;

    CU_ASSERT(xrc == 0);
    CU_ASSERT(g_msg_received == 0);
    CU_ASSERT(g_msg_received_len != transport->pobj->savelen);

    //Test SUBSCRIBE with mqttv5
    transport->pobj->mqtt_version = 5;
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/type/type1/id/id1/cmd/+/fmt/+";
    topiclen = strlen(topic);
    cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
    cmd |= 0xF;
    buf.used = 18;
    //Put MsgID
    xbuf[16] = (uint8_t)3>>8;
    xbuf[17] = (uint8_t)3;
    //ism_common_allocBufferCopyLen(&buf, msgID, 3);

    //Put Variable Props
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;

    //ADD Topic Filters
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //buf.used+=1; //Add Sub Option Byte
    int savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    int qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    int extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    int totalexlen = 0;
    totalexlen += (extlen+2);

    //ADD Topic Filters
    topic = "iot-2/mon";
    topiclen = strlen(topic);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen += (extlen+2); //+2 for the 2-byte len hodler

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension
    //Set Transport permission to lowest
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
    //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

    CU_ASSERT(xrc == ISMRC_OK);
    CU_ASSERT(g_msg_received == 1);
    CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);




    //Test SUBSCRIBE with mqttv5 with No Permission
    transport->pobj->mqtt_version = 5;
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/type/type1/id/id1/cmd/+/fmt/+";
    topiclen = strlen(topic);
    cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
    cmd |= 0xF;
    buf.used = 18;
    //Put MsgID
    xbuf[16] = (uint8_t)3>>8;
    xbuf[17] = (uint8_t)3;
    //ism_common_allocBufferCopyLen(&buf, msgID, 3);

    //Put Variable Props
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;

    //ADD Topic Filters
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen = 0;
    totalexlen += (extlen+2);

    //ADD Topic Filters
    topic = "iot-2/mon";
    topiclen = strlen(topic);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen += (extlen+2); //+2 for the 2-byte len hodler

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension

    old_permissions =  transport->auth_permissions;
    transport->auth_permissions=0;
    //Set Transport permission to lowest
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    transport->auth_permissions=old_permissions;
    //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
    //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

    //Expect xrc is OK and 1 Msgs received for SUBACK since this is MQTTv5 and all msgs are bad
    CU_ASSERT(xrc == ISMRC_OK);
    CU_ASSERT(g_msg_received == 1);
    //CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);


    //Test SUBSCRIBE with mqttv3.1.1
    transport->pobj->mqtt_version = 4;
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/type/type1/id/id1/cmd/+/fmt/+";
    topiclen = strlen(topic);
    cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
    cmd |= 0xF;
    buf.used = 18;
    //Put MsgID
    xbuf[16] = (uint8_t)3>>8;
    xbuf[17] = (uint8_t)3;
    //ism_common_allocBufferCopyLen(&buf, msgID, 3);

    //Put Variable Props
    //ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    //buf.used += 147;

    //ADD Topic Filters
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen = 0;
    totalexlen += (extlen+2);

    //ADD Topic Filters
    topic = "iot-2/mon";
    topiclen = strlen(topic);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen += (extlen+2); //+2 for the 2-byte len hodler

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension
    //Set Transport permission to lowest
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
    //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

    CU_ASSERT(xrc == ISMRC_OK);
    CU_ASSERT(g_msg_received == 1);
    CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);


    //Test SUBSCRIBE with mqttv3.1.1 with No Permission
    transport->pobj->mqtt_version = 4;
    g_msg_received_len=0;
    g_msg_received=0;
    transport->pobj->savedata=NULL;
    transport->pobj->savelen=0;
    transport->pobj->savecount=0;

    topic = "iot-2/type/type1/id/id1/cmd/+/fmt/+";
    topiclen = strlen(topic);
    cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
    cmd |= 0xF;
    buf.used = 18;
    //Put MsgID
    xbuf[16] = (uint8_t)3>>8;
    xbuf[17] = (uint8_t)3;
    //ism_common_allocBufferCopyLen(&buf, msgID, 3);

    //Put Variable Props
    //ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    //buf.used += 147;

    //ADD Topic Filters
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen = 0;
    totalexlen += (extlen+2);

    //ADD Topic Filters
    topic = "iot-2/mon";
    topiclen = strlen(topic);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen += (extlen+2); //+2 for the 2-byte len hodler

    appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

    totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension

    old_permissions =  transport->auth_permissions;
    transport->auth_permissions=0;
    //Set Transport permission to lowest
    xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
    transport->auth_permissions=old_permissions;
    //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
    //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

    //Expect XRC is non zero and 1 msg sent for SUBACk with qos1
    CU_ASSERT(xrc == ISMRC_OK);
    CU_ASSERT(g_msg_received == 1);
    //CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);


    //Test SUBSCRIBE with mqttv5 with One or More Filters invalid
   transport->pobj->mqtt_version = 5;
   g_msg_received_len=0;
   g_msg_received=0;
   transport->pobj->savedata=NULL;
   transport->pobj->savelen=0;
   transport->pobj->savecount=0;


   cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
   cmd |= 0xF;
   buf.used = 18;
   //Put MsgID
   xbuf[16] = (uint8_t)3>>8;
   xbuf[17] = (uint8_t)3;
   //ism_common_allocBufferCopyLen(&buf, msgID, 3);

   //Put Variable Props
   ism_common_putMqttVarInt(&buf, 147);   /* proplen */
   buf.used += 147;

   //ADD Topic Filters
   //xbuf[buf.used]=(uint8_t)topiclen>>8;
   //xbuf[buf.used+1] = (uint8_t)topiclen;
   //buf.used+=2;
   //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
   topic = "iot-2/type/type1/id/id1/cmd/+/fmt/+";
   topiclen = strlen(topic);
   convertTopic(transport, &buf, topic, topiclen, 1);
   //buf.used+=1; //Add Sub Option Byte
   savepos = buf.used;
   buf.used += 2;    /* Skip over len, add later */
   /* Put out the QoS byte */
   qos=1;
   ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

   extlen = buf.used - savepos - 2;
   buf.buf[savepos] = (char)(extlen<<8);
   buf.buf[savepos+1] = (char)extlen;
   totalexlen = 0;
   totalexlen += (extlen+2);

   //ADD Topic Filters
    //xbuf[buf.used]=(uint8_t)topiclen>>8;
    //xbuf[buf.used+1] = (uint8_t)topiclen;
    //buf.used+=2;
    //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    topic = "iot-2/type/type1/id/id1/cmd/cmand2/fmt/+";
    topiclen = strlen(topic);
    convertTopic(transport, &buf, topic, topiclen, 1);
    //buf.used+=1; //Add Sub Option Byte
    savepos = buf.used;
    buf.used += 2;    /* Skip over len, add later */
    /* Put out the QoS byte */
    qos=1;
    ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

    extlen = buf.used - savepos - 2;
    buf.buf[savepos] = (char)(extlen<<8);
    buf.buf[savepos+1] = (char)extlen;
    totalexlen = 0;
    totalexlen += (extlen+2);

   //ADD Topic Filters
   topic = "iot-2/mon";
   topiclen = strlen(topic);
   convertTopic(transport, &buf, topic, topiclen, 1);
   //xbuf[buf.used]=(uint8_t)topiclen>>8;
   //xbuf[buf.used+1] = (uint8_t)topiclen;
   //buf.used+=2;
   //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
   //buf.used+=1; //Add Sub Option Byte
   savepos = buf.used;
   buf.used += 2;    /* Skip over len, add later */
   /* Put out the QoS byte */
   qos=1;
   ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

   extlen = buf.used - savepos - 2;
   buf.buf[savepos] = (char)(extlen<<8);
   buf.buf[savepos+1] = (char)extlen;
   totalexlen += (extlen+2); //+2 for the 2-byte len hodler

   appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

   totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension

   old_permissions =  transport->auth_permissions;
   transport->auth_permissions=8;
   //Set Transport permission to lowest
   xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
   transport->auth_permissions=old_permissions;
   //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
   //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

   //Expect Only one message receive
   CU_ASSERT(xrc == ISMRC_OK);
   CU_ASSERT(g_msg_received == 1);
   //CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);


   //Test SUBSCRIBE with mqttv3.1.1 with One or More Filters invalid
     transport->pobj->mqtt_version = 4;
     g_msg_received_len=0;
     g_msg_received=0;
     transport->pobj->savedata=NULL;
     transport->pobj->savelen=0;
     transport->pobj->savecount=0;


     cmd = MT_SUBSCRIBE<<4;   /* QoS = 0 */
     cmd |= 0xF;
     buf.used = 18;
     //Put MsgID
     xbuf[16] = (uint8_t)3>>8;
     xbuf[17] = (uint8_t)3;
     //ism_common_allocBufferCopyLen(&buf, msgID, 3);

     //Put Variable Props
     //ism_common_putMqttVarInt(&buf, 147);   /* proplen */
     //buf.used += 147;

     //ADD Topic Filters
     //xbuf[buf.used]=(uint8_t)topiclen>>8;
     //xbuf[buf.used+1] = (uint8_t)topiclen;
     //buf.used+=2;
     //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
     topic = "iot-2/type/type1/id/id1/cmd/+/fmt/+";
     topiclen = strlen(topic);
     convertTopic(transport, &buf, topic, topiclen, 1);
     //buf.used+=1; //Add Sub Option Byte
     savepos = buf.used;
     buf.used += 2;    /* Skip over len, add later */
     /* Put out the QoS byte */
     qos=1;
     ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

     extlen = buf.used - savepos - 2;
     buf.buf[savepos] = (char)(extlen<<8);
     buf.buf[savepos+1] = (char)extlen;
     totalexlen = 0;
     totalexlen += (extlen+2);

     //ADD Topic Filters
      //xbuf[buf.used]=(uint8_t)topiclen>>8;
      //xbuf[buf.used+1] = (uint8_t)topiclen;
      //buf.used+=2;
      //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
      topic = "iot-2/type/type1/id/id1/cmd/cmand2/fmt/+";
      topiclen = strlen(topic);
      convertTopic(transport, &buf, topic, topiclen, 1);
      //buf.used+=1; //Add Sub Option Byte
      savepos = buf.used;
      buf.used += 2;    /* Skip over len, add later */
      /* Put out the QoS byte */
      qos=1;
      ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

      extlen = buf.used - savepos - 2;
      buf.buf[savepos] = (char)(extlen<<8);
      buf.buf[savepos+1] = (char)extlen;
      totalexlen = 0;
      totalexlen += (extlen+2);

     //ADD Topic Filters
     topic = "iot-2/mon";
     topiclen = strlen(topic);
     convertTopic(transport, &buf, topic, topiclen, 1);
     //xbuf[buf.used]=(uint8_t)topiclen>>8;
     //xbuf[buf.used+1] = (uint8_t)topiclen;
     //buf.used+=2;
     //ism_common_allocBufferCopyLen(&buf, topic, topiclen);
     //buf.used+=1; //Add Sub Option Byte
     savepos = buf.used;
     buf.used += 2;    /* Skip over len, add later */
     /* Put out the QoS byte */
     qos=1;
     ism_common_putExtensionValue(&buf, EXIV_SubscribeOpt, qos);

     extlen = buf.used - savepos - 2;
     buf.buf[savepos] = (char)(extlen<<8);
     buf.buf[savepos+1] = (char)extlen;
     totalexlen += (extlen+2); //+2 for the 2-byte len hodler

     appendSavedData(transport,buf.buf+16, buf.used-16, cmd);

     totalexlen -=2; //Minus 2bytes to store QoS when it is not Extension

     old_permissions =  transport->auth_permissions;
     transport->auth_permissions=8;
     //Set Transport permission to lowest
     xrc = revalidateAndSendSavedData(transport, transport->pobj->savedata, transport->pobj->savelen, transport->pobj->savecount);
     transport->auth_permissions=old_permissions;
     //printf("xrc=%d g_msg_received:%d g_msg_received_len:%d savedlen=%d totalext_len=%d\n",
     //        xrc, g_msg_received, g_msg_received_len, transport->pobj->savelen, totalexlen);

     //Expect non zero RC and 1 msg received for SUBACK since it is qos1
     CU_ASSERT(xrc == ISMRC_OK);
     CU_ASSERT(g_msg_received == 1);
     //CU_ASSERT(g_msg_received_len == transport->pobj->savelen-totalexlen);

    my_freeTransport(transport, stransport);
}

void testRevalidateSaveData(void) {
    testRevalidateSaveDataForDevice();
    testRevalidateSaveDataForGateway();
}

void ism_proxy_nojavainit(void);

/*
 * Run the MQTTv5 tests
 */
void mqttv5_test(void) {
    ism_field_t f;
    f.type = VT_Integer;
    f.val.i = 1;
    ism_common_setProperty(ism_common_getConfigProperties(), "MQTTv5", &f);
    ism_protocol_initMQTT();
    ism_proxy_nojavainit();

    makeTenant("x", "iot2");
    ism_common_setTraceLevel(2);
    mqttv5_connect();
    mqttv5_connectND();
    mqttv5_connectExp();
    mqttv5_auth();
    mqttv5_authMethod();
    mqttv5_authData();
    mqtt_pass_removeWillmsg();
    mqtt_pass_cleansessionTrue();
    ism_common_setTraceLevel(2);
}

