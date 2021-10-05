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

/*
 * This file is included from test_mqtt.c
 */

extern int g_fake_max_expire;

void setupConnection5(char conn_buf[]) {
    memcpy(conn_buf, "\0\4MQTT\5\0\0\0\0\0\1c", 14);
}

void mqttv5_connect(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    transport->listener->usePasswordAuth = 1;
    // Protocol length
    conn_buf = "\0\4MQTT\5\xC0\0\7"       /* v5 user,pw keepalive=7 */
               "\5\x11\0\0\0\4"           /* expire = 4 */
               "\0\1c"                    /* ClientID = c */
               "\0\1z\0\1x";              /* userid and password */

    transport->suballoc.size = 1024;
    g_entered_authenticate_user = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 25, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(transport->pobj->mqtt_version == 5);
    CU_ASSERT(transport->name && !strcmp(transport->name, "c"));
    CU_ASSERT(g_cmd_result > 0 && g_entered_authenticate_user > 0);
    CU_ASSERT(transport->pobj->cleansession == 0);
    CU_ASSERT(transport->durable == 1);
    CU_ASSERT(transport->userid != NULL && *transport->userid == 'z' && transport->userid[1] == 0);
    CU_ASSERT(transport->pobj->expireTTL == 4);
    my_freeTransport(transport);
}

/*
 * Good connection non-durable
 */
void mqttv5_connectND(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    transport->listener->usePasswordAuth = 0;
    // Protocol length
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\0"                       /* No properties */
               "\0\1c";                   /* ClientID = c */

    g_entered_authenticate_user = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 14, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(transport->pobj->mqtt_version == 5);
    CU_ASSERT(!strcmp(transport->name, "c"));
    CU_ASSERT(g_cmd_result > 0);
    CU_ASSERT(transport->pobj->cleansession == 1);
    CU_ASSERT(transport->durable == 0);
    CU_ASSERT(transport->userid == NULL);
    CU_ASSERT(transport->pobj->expireTTL == 0);
    my_freeTransport(transport);
}

/*
 * Good connection with expire
 */
void mqttv5_connectExp(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    char * cmd_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    transport->listener->usePasswordAuth = 0;
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\5\x11\0\0\1\0"           /* expire = 0x100 */
               "\0\1c";                   /* ClientID = c */

    g_entered_authenticate_user = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 19, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(transport->pobj->mqtt_version == 5);
    CU_ASSERT(!strcmp(transport->name, "c"));
    CU_ASSERT(g_cmd_result > 0);
    CU_ASSERT(transport->pobj->cleansession == 0);
    CU_ASSERT(transport->durable == 1);
    CU_ASSERT(transport->userid == NULL);
    CU_ASSERT(transport->pobj->expireTTL == 0x100);

    /* Now disconnect setting the expire */
    cmd_buf = "\0\5\x11\0\0\2\0";    /* rc=0, expire=256 */
    CU_ASSERT(mqttRx_test(0, transport, NULL, cmd_buf, 7, 0xE0, /* MT_DISCONNECT */ 0) == 0);
    CU_ASSERT(transport->pobj->expireTTL == 512);
    CU_ASSERT(transport->closestate[3] == 1);
    my_freeTransport(transport);
}
/*
 * Good connection with expire
 */
void mqttv5_connectExpMax(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    char * cmd_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    transport->listener->usePasswordAuth = 0;
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\5\x11\0\1\0\0"           /* expire = 0x100 */
               "\0\1c";                   /* ClientID = c */

    g_fake_max_expire = 1000;
    g_entered_authenticate_user = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 19, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_entered_authenticate_user > 0);
    g_fake_max_expire = 0;
    CU_ASSERT(g_cmd_result > 0);
    CU_ASSERT(transport->pobj->cleansession == 0);
    CU_ASSERT(transport->durable == 1);
    CU_ASSERT(transport->userid == NULL);
    CU_ASSERT(transport->pobj->expireTTL == 1000);
    if (g_verbose)
        printf("expire=%d max=%d\n", transport->pobj->expireTTL, ism_security_context_getClientStateExpiry(transport->security_context));

    /* Now disconnect setting the expire to more than max */
    cmd_buf = "\0\5\x11\0\3\0\0";    /* rc=0, expire=0x30000 */
    CU_ASSERT(mqttRx_test(0, transport, NULL, cmd_buf, 7, 0xE0, /* MT_DISCONNECT */ 0) == 0);
    CU_ASSERT(transport->pobj->expireTTL == 1000);    /* Still max from policy */
    CU_ASSERT(transport->closestate[3] == 1);
    my_freeTransport(transport);
}

/*
 * Good connection with expire
 */
void mqttv5_connectExpMax2(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    char * cmd_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    transport->listener->usePasswordAuth = 0;
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\5\x11\0\0\2\0"           /* expire = 512 */
               "\0\1c";                   /* ClientID = c */

    g_fake_max_expire = 1000;
    g_entered_authenticate_user = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 19, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_entered_authenticate_user > 0);
    g_fake_max_expire = 0;
    CU_ASSERT(g_cmd_result > 0);
    CU_ASSERT(transport->pobj->cleansession == 0);
    CU_ASSERT(transport->durable == 1);
    CU_ASSERT(transport->userid == NULL);
    CU_ASSERT(transport->pobj->expireTTL == 512);
    CU_ASSERT(transport->pobj->maxExpire == 1000);
    if (g_verbose)
        printf("expire=%d max=%d\n", transport->pobj->expireTTL, ism_security_context_getClientStateExpiry(transport->security_context));

    /* Now disconnect setting the expire to less than max */
    cmd_buf = "\0\5\x11\0\0\3\0";    /* rc=0, expire=768 */
    CU_ASSERT(mqttRx_test(0, transport, NULL, cmd_buf, 7, 0xE0, /* MT_DISCONNECT */ 0) == 0);
    CU_ASSERT(transport->pobj->expireTTL == 768);    /* Still max from policy */
    CU_ASSERT(transport->closestate[3] == 1);
    my_freeTransport(transport);
}


/*
 * Connect too big
 */
void mqttv5_connectTooBig(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    int    rc;

    setupTransport(transport, protocol, NULL, __LINE__);
    transport->listener->usePasswordAuth = 0;
    transport->listener->maxMsgSize = 88;
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\5\x11\0\0\2\0"           /* expire = 512 */
               "\0\x46x1234567890123456789012345678901234567890123456789012344567890123456789";    /* ClientID */

    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, NULL, 88, 0x10, /* MT_CONNECT */ 1)) == ISMRC_MsgTooBig);
    // printf("Line %d: rc=%d\n", __LINE__, rc);
    my_freeTransport(transport);
}

/*
 * Connect with AuthMethod
 */
void mqttv5_connectAuthMethod(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    int    rc;

    setupTransport(transport, protocol, NULL, __LINE__);
    transport->listener->usePasswordAuth = 0;
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\x0D\x11\0\0\2\0\x15\0\5PLAIN"     /* expire = 512 authmethod=plain */
               "\0\1c";    /* ClientID */

    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, NULL, 26, 0x10, /* MT_CONNECT */ 1)) == ISMRC_NotAuthorized);
    // printf("Line %d: rc=%d\n", __LINE__, rc);
    my_freeTransport(transport);
}

/*
 * Connect with AuthMethod
 */
void mqttv5_connectAuthData(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    int    rc;

    setupTransport(transport, protocol, NULL, __LINE__);
    transport->listener->usePasswordAuth = 0;
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\x0D\x11\0\0\2\0\x16\0\5ghijk"    /* expire = 512, authdata = ghijk  */
               "\0\1c";    /* ClientID */

    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, NULL, 26, 0x10, /* MT_CONNECT */ 1)) == ISMRC_NotAuthorized);
    // printf("Line %d: rc=%d\n", __LINE__, rc);
    my_freeTransport(transport);
}

/*
 * Connect with AuthMethod
 */
void mqttv5_Auth(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    char * cmd_buf;
    int    rc;

    setupTransport(transport, protocol, NULL, __LINE__);
    transport->listener->usePasswordAuth = 0;
    conn_buf = "\0\4MQTT\5\x02\0\7"       /* v5 user,pw,clean keepalive=7 */
               "\x05\x11\0\0\0\0"         /* expire = 0  */
               "\0\1c";                   /* ClientID */

    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, NULL, 19, 0x10, /* MT_CONNECT */ 1)) == 0);
    cmd_buf = "\x18\x08\x15\0\5PLAIN";
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, cmd_buf, 10, 0xF0, /* MT_AUTH */ 1)) == ISMRC_NotAuthorized);

    // printf("Line %d: rc=%d\n", __LINE__, rc);
    my_freeTransport(transport);
}

/*
 * Test normal path
 */
void testmqttv5(void) {
    /* Set up for MQTTv5 testing */
    ism_field_t f;
    f.type = VT_Integer;
    f.val.i = 1;
    ism_common_setProperty(ism_common_getConfigProperties(), "AllowMQTTv5", &f);
    ism_common_setProperty(ism_common_getConfigProperties(), "Protocol.AllowMqttProxyProtocol", &f);
    ism_protocol_initMQTT();

    mqttv5_connect();
    mqttv5_connectND();
    mqttv5_connectExp();
    mqttv5_connectExpMax();
    mqttv5_connectExpMax2();
}

void testmqttv5_errors(void) {
    /* Set up for MQTTv5 testing */
    ism_field_t f;
    f.type = VT_Integer;
    f.val.i = 1;
    ism_common_setProperty(ism_common_getConfigProperties(), "AllowMQTTv5", &f);
    ism_common_setProperty(ism_common_getConfigProperties(), "Protocol.AllowMqttProxyProtocol", &f);
    ism_protocol_initMQTT();

    if (g_verbose)
        ism_common_setTraceLevel(6);

    mqttv5_connectTooBig();
    mqttv5_Auth();
    mqttv5_connectAuthMethod();
    mqttv5_connectAuthData();

    ism_common_setTraceLevel(1);
}
