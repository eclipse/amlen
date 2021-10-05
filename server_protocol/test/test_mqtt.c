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

/*
 * File: test_mqtt.c
 * Component: server
 * SubComponent: server_protocol
 *
 */
// #include <float.h>
// TODO:
//  1. Rework unsubscribe tests to work the way the other tests do.  For unit test
//     purposes, subscribe is not needed before unsubscribe.


extern int g_verbose;

#include <mqtt.c>
#include <test_mqtt.h>
#include <fakeengine.h>


/**
 * Test functions
 */
void mqttRx_pass_connect_keepalive(void);

/* Helper functions */
int mqttRx_test(int do_connect, ism_transport_t * transport, char * conn_buf, char * cmd_buf, int buflen, int kind, int check_response);
void setupTransport(ism_transport_t *transport, const char * protocol,struct ism_protobj_t * pobj,int line);
void setupConnection(char conn_buf[]);
void setupConnectionWithFlags(char conn_buf[], int flags);
void setupConnectionWithWillMsg(char conn_buf[], int qos);
void setupConnection4(char conn_buf[]);
void setAllowDurable(int value);
void setAllowPersistentMessages(int value);
void testmqttv5(void);
void testmqttv5_errors(void);
void testPutString(void);


/**
 * Transport functions
 */
int mclose(ism_transport_t * trans, int rc, int clean, const char * reason);
int mclosed(ism_transport_t * trans);
int msend(ism_transport_t * trans, char * buf, int len, int kind, int flags);

CU_TestInfo ISM_Protocol_CUnit_MqttBasic[] = {
    {"TestPutString      ",  testPutString },
    {"MqttGetError       ",  getmqtterror },
    {"MqttGetActionName  ",  getmqttactionname },
    {"MqttConnect        ",  testmqttconnect },
    {"MqttPublish        ",  testmqttpublish },
    {"MqttSubscribe      ",  testmqttsubscribe },
    {"MqttUnsubscribe    ",  testmqttunsubscribe },
    {"Mqttv3.1.1         ",  testmqtt4     },
    {"Mqttv5.0           ",  testmqttv5    },
    {"Mqttv5.0_Errors    ",  testmqttv5_errors    },
    {"CheckString        ",  testcheckstring },
    {"ParseTopic         ",  testparsetopic },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Protocol_CUnit_MqttFull[] = {
    {"MqttGetError       ",  getmqtterror },
    {"MqttGetActionName  ",  getmqttactionname },
    {"MqttConnect        ",  testmqttconnect },
    {"MqttPublish        ",  testmqttpublish },
    {"MqttSubscribe      ",  testmqttsubscribe },
    {"MqttUnsubscribe    ",  testmqttunsubscribe },
    {"Mqttv3.1.1         ",  testmqtt4     },
    {"Mqttv5.0           ",  testmqttv5    },
    {"Mqttv5.0_Errors    ",  testmqttv5    },
    {"CheckString        ",  testcheckstring },
    {"ParseTopic         ",  testparsetopic },
    {"MqttKeepalive      ",  mqttRx_pass_connect_keepalive },
    CU_TEST_INFO_NULL
};

/**
 * Test that return strings for known errors are correct and unchanged.
 * NOTE: This test will need to be updated if additinal enums are added
 * to conack_e.
 */
void getmqtterror(void) {
    char xbuf[200];

    CU_ASSERT(getMQTTErrorString(CRC_Accepted, xbuf, sizeof xbuf) == NULL);
    CU_ASSERT(strcmp(getMQTTErrorString(CRC_InvalidVersion, xbuf, sizeof xbuf), "The MQTT client version is not supported.") == 0);
    CU_ASSERT(strcmp(getMQTTErrorString(CRC_BadIdentifier, xbuf, sizeof xbuf), "The client ID is not valid.") == 0);
    CU_ASSERT(strcmp(getMQTTErrorString(CRC_BadUser, xbuf, sizeof xbuf), "The user name or password is not valid.") == 0);
    CU_ASSERT(strcmp(getMQTTErrorString(CRC_NotAuthorized, xbuf, sizeof xbuf), "The connection is not authorized.") == 0);
}

/**
 * Test that return strings for known actions are correct and unchanged.
 * NOTE: This will need to be updated if action_e is updated.
 */
void getmqttactionname(void) {
    CU_ASSERT(strcmp(getActionName(Action_message),"message") == 0);
    CU_ASSERT(strcmp(getActionName(Action_messageWait),"messageWait") == 0);
    CU_ASSERT(strcmp(getActionName(Action_ack),"ack") == 0);
    CU_ASSERT(strcmp(getActionName(Action_reply),"reply") == 0);
    CU_ASSERT(strcmp(getActionName(Action_receiveMsg),"receiveMsg") == 0);
    CU_ASSERT(strcmp(getActionName(Action_receiveMsgNoWait),"receiveMsgNoWait") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createConnection),"createConnection") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createSession),"createSession") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createConsumer),"createConsumer") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createBrowser),"createBrowser") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createDurable),"createDurable") == 0);
    CU_ASSERT(strcmp(getActionName(Action_closeConsumer),"closeConsumer") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createProducer),"createProducer") == 0);
    CU_ASSERT(strcmp(getActionName(Action_closeProducer),"closeProducer") == 0);
    CU_ASSERT(strcmp(getActionName(Action_setMsgListener),"setMsgListener") == 0);
    CU_ASSERT(strcmp(getActionName(Action_removeMsgListener),"removeMsgListener") == 0);
    CU_ASSERT(strcmp(getActionName(Action_rollbackSession),"rollbackSession") == 0);
    CU_ASSERT(strcmp(getActionName(Action_commitSession),"commitSession") == 0);
    CU_ASSERT(strcmp(getActionName(Action_ackSync),"ackSync") == 0);
    CU_ASSERT(strcmp(getActionName(Action_recover),"recover") == 0);
    CU_ASSERT(strcmp(getActionName(Action_getTime),"getTime") == 0);
    CU_ASSERT(strcmp(getActionName(Action_createTransaction),"createTransaction") == 0);
    CU_ASSERT(strcmp(getActionName(Action_sendWill),"sendWill") == 0);
    CU_ASSERT(strcmp(getActionName(Action_initConnection),"initConnection") == 0);
    CU_ASSERT(strcmp(getActionName(Action_termConnection),"termConnection") == 0);

    CU_ASSERT(strcmp(getActionName(-1),"Unknown") == 0);
    CU_ASSERT(strcmp(getActionName(MAX_ACTION_VALUE+1),"Unknown") == 0);
}


/*
 * Set the the AllowDurable value that determines whether a connection can
 * be established when cleansession is set to 0.
 */
void setAllowDurable(int value) {
    g_allowdurable = value;
}

/*
 * Set the the AllowPersistentMessages value that determines whether
 *   1. A connection can * be established when the WillMessage QoS > 0
 *   2. An individual message can be pubished with QoS > 0
 */
void setAllowPersistentMessages(int value) {
    g_allowpersistentmessages = value;
}

/*
 * Test MT_CONNECT with version set to 4 and client ID set.
 * Username and password flags set with username and password lengths of 0 provided.
 * Should succeed.
 */
void mqtt4_pass_connect(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    transport->listener->usePasswordAuth = 1;
    // Protocol length
    conn_buf = "\0\4MQTT\4\xC0\0\0\0\1c"  /* With clientID */
               "\0\0\0\0";                /* Zero length userid and password */

    g_entered_authenticate_user = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 17, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0 && g_entered_authenticate_user > 0);
}

/*
 * Test a generated clientID
 */
void mqtt4_pass_connect_nocid(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    transport->client_addr = "10.10.0.7";
    transport->listener->usePasswordAuth = 1;
    // Protocol length
    conn_buf = "\0\4MQTT\4\x02\0\0\0\0";  /* With clientID */

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 12, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(memcmp(transport->name, "_10.10.0.7_", 11) == 0);
    CU_ASSERT(strlen(transport->name)==19);
}

/*
 * Test MT_CONNECT version set to 4 but invalid bits in command.
 */
void mqtt4_fail_connect_badbits(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    conn_buf = "\0\4MQTT\4\0\0\0\0\1c";  /* With clientID */

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 13, 0x11, /* MT_CONNECT */ 1) == ISMRC_BadClientData);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test MT_CONNECT version set to 4 but invalid bits in connect flags.
 */
void mqtt4_fail_connect_badbits2(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    conn_buf = "\0\4MQTT\4\x3\0\0\0\1c";  /* With clientID */

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 13, 0x10, /* MT_CONNECT */ 1) == ISMRC_BadClientData);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test MT_CONNECT version set to 4 but invalid bits in will
 */
void mqtt4_fail_connect_badbits3(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    conn_buf = "\0\4MQTT\4\x28\0\0\0\1c";  /* Will, WillRetain, WillQoS=1, Will=0 */

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 13, 0x10, /* MT_CONNECT */ 1) == ISMRC_BadClientData);
    CU_ASSERT(g_entered_close_cb == 1);
}
/*
 * Test MT_CONNECT version set to 4 but userid set with no userid
 */
void mqtt4_fail_connect_badbits4(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    conn_buf = "\0\4MQTT\4\x80\0\0\0\1c";  /* userID, no UserID */

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 13, 0x10, /* MT_CONNECT */ 1) == ISMRC_BadClientData);
    CU_ASSERT(g_entered_close_cb == 1);
}
/*
 * Test MT_CONNECT version set to 4 with userid and pw bits set, with userid but without password.
 */
void mqtt4_fail_connect_badbits5(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    conn_buf = "\0\4MQTT\4\xC0\0\0\0\1c\0\1ux";  /* userID and pw bits, userid but no pw */

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 17, 0x10, /* MT_CONNECT */ 1) == ISMRC_BadClientData);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test MT_PUBLISH version set to 4 but invalid bits in subscribe command.
 */
void mqtt4_fail_publish_badbits(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection4(conn_buf);

    sub_buf = "\0\1\0\1t\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 6, 0x38, /* MT_PUBLISH + dup */ 1) == ISMRC_BadClientData);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test MT_SUBSCRIBE version set to 4 but invalid bits in subscribe command.
 */
void mqtt4_fail_subscribe_badbits(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection4(conn_buf);

    sub_buf = "\0\1\0\1t\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 6, 0x83, /* MT_SUBSCRIBE */ 1) == ISMRC_BadClientData);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test MT_UNSUBSCRIBE version set to 4 but invalid bits in command.
 */
void mqtt4_fail_unsubscribe_badbits(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection4(conn_buf);

    sub_buf = "\0\1\0\1t";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 5, 0xA3, /* MT_UNSUBSCRIBE */ 1) == ISMRC_BadClientData);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test MT_PINGREQ version set to 4 but invalid bits in command.
 */
void mqtt4_fail_ping_badbits(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection4(conn_buf);

    sub_buf = "";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 0, 0xC1, /* MT_PINGREQ */ 1) == ISMRC_BadClientData);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test MT_DISCONNECT version set to 4 but invalid bits in command.
 */
void mqtt4_fail_disconnect_badbits(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection4(conn_buf);

    sub_buf = "";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 0, 0xE1, /* MT_DISCONNECT */ 1) == ISMRC_BadClientData);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test MT_CONNECT version set to 6
 */
void mqtt4_fail_connect_badversion(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    conn_buf = "\0\4MQTT\6\xC0\0\0\0\1c\0\1ux";  /* userID and pw bits, userid but no pw */

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 17, 0x10, /* MT_CONNECT */ 1) == CRC_InvalidVersion);
    CU_ASSERT(g_entered_close_cb == 1);
}
/*
 * Test MT_CONNECT version set to 6
 */
void mqtt4_fail_connect_badid(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    conn_buf = "\0\4MQTT\4\x00\0\0\0\0";  /* cleansession=0 0 length clientid */

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 12, 0x10, /* MT_CONNECT */ 1) == CRC_BadIdentifier);
    CU_ASSERT(g_entered_close_cb == 1);
}
/*
 * Test MT_CONNECT version set to 6
 */
void mqtt4_fail_connect_badid2(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char * conn_buf;
    setupTransport(transport, protocol, NULL, __LINE__);
    conn_buf = "\0\4MQTT\4\x02\0\0\0\1\xff";  /* bad char in clientid */

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 13, 0x10, /* MT_CONNECT */ 1) == CRC_BadIdentifier);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test MT_PUBLISH version set to 4 but too many slashes
 */
void mqtt4_fail_publish_toomanyslashes(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection4(conn_buf);

    sub_buf = "\0\x30////////////////////////////////////////////////";

    xUNUSED int rc;
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, sub_buf, 50, 0x30, /* MT_PUBLISH */ 1)) == 0);
    // printf("%d=%d\n", __LINE__, rc);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test MT_PUBLISH version set to 4 but too many slashes
 */
void mqtt4_fail_publish_toolarge(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection4(conn_buf);
    transport->listener->maxMsgSize = 30;

    sub_buf = "\0\1zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz";

    xUNUSED int rc;
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, sub_buf, 50, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_MsgTooBig);
    // printf("%d=%d\n", __LINE__, rc);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test MT_PUBLISH version set to 4 but too many slashes
 */
void mqtt4_fail_publish_invalid(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection4(conn_buf);

    sub_buf = "\0\1\xff";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 3, 0x30, /* MT_PUBLISH */ 1) == ISMRC_UnicodeNotValid);
    CU_ASSERT(g_entered_close_cb == 1);

    sub_buf = "\0\1\x01";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 3, 0x30, /* MT_PUBLISH */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);
}
/*
 * Test MT_PUBLISH version set to 4 but too many slashes
 */
void mqtt4_fail_publish_systemTopic(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection4(conn_buf);

    sub_buf = "\0\5$SYSX";

    xUNUSED int rc;
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, sub_buf, 7, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_BadSysTopic);
    // printf("%d=%d\n", __LINE__, rc);
    CU_ASSERT(g_entered_close_cb == 1);
}

/*
 * Test mqtt v3.1.1
 */
void testmqtt4(void) {
    mqtt4_pass_connect();
    mqtt4_pass_connect_nocid();
    mqtt4_fail_connect_badbits();
    mqtt4_fail_connect_badbits2();
    mqtt4_fail_connect_badbits3();
    mqtt4_fail_connect_badbits4();
    mqtt4_fail_connect_badbits5();
    mqtt4_fail_publish_badbits();
    mqtt4_fail_subscribe_badbits();
    mqtt4_fail_unsubscribe_badbits();
    mqtt4_fail_ping_badbits();
    mqtt4_fail_disconnect_badbits();
    mqtt4_fail_connect_badversion();
    mqtt4_fail_connect_badid();
    mqtt4_fail_connect_badid2();
    mqtt4_fail_publish_toomanyslashes();
    mqtt4_fail_publish_toolarge();
    mqtt4_fail_publish_invalid();
    mqtt4_fail_publish_systemTopic();
}

/*
 *
 */
void testcheckstring(void) {
    mqttMsg_t mmsg = {0};
    int  rc = 0;

    mmsg.version = 4;
    mmsg.v5_shared = 1;
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/", 20,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/", 22,  CHK_SUBTOPIC)) == ISMRC_BadTopic );
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/$", 23,  CHK_SUBTOPIC)) == ISMRC_BadTopic );
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/a", 23,  CHK_SUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/abc/a", 25,  CHK_SUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/abc//a", 25,  CHK_SUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/\x61\x62\x63/a", 25,  CHK_SUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/a\xc2\xa3\nx/a", 27,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/\x00\x41\xf0\xa0\xa0\x80/a", 28,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/\xc2\x80/a", 24,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/z\xef\xbf\xbe/a", 26,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/z\xef\xb7\x90/a", 26,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/abc", 25,  CHK_SUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/\x61\x62\x63", 25,  CHK_SUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/a\xc2\xa3\nx", 27,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/\x00\x41\xf0\xa0\xa0\x80", 28,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/\xc2\x80", 24,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/z\xef\xbf\xbe", 26,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/z\xef\xb7\x90", 26,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/#", 23,  CHK_SUBTOPIC)) == ISMRC_OK );
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/+", 23,  CHK_SUBTOPIC)) == ISMRC_OK );
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/this:is:a:very:long:subname/x+", 50,  CHK_SUBTOPIC)) == ISMRC_BadTopic );
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$select/QoS=0/$SharedSubscription/abc/abc", 41,  CHK_SUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$select/QoS>0/a", 15,  CHK_SUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$select/QoS=1/a", 15,  CHK_SUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$select/QoS>0/$SharedSubscription/abc/def", 41,  CHK_SUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$select/QoS=0/$share/abc/a", 26,  CHK_SUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$select/QoS=3/$share/abc/a", 26,  CHK_SUBTOPIC)) == ISMRC_BadTopic);

    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/x/a", 23,  CHK_PUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$SharedSubscription/", 20,  CHK_PUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "abc/a", 5,  CHK_PUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$select/QoS=0/$SharedSubscription/abc/abc", 41,  CHK_PUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "$select/QoS=0/a", 15,  CHK_PUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "abc/a", 5,  CHK_PUBTOPIC)) == 0);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "\x61\x62\x63/a\0\0", 7,  CHK_PUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "a\xc2\xa3\nx/a", 7,  CHK_PUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "\xfd\xd0\x00\x41\xf0\xa0\xa0\x80/a", 10,  CHK_PUBTOPIC)) == ISMRC_UnicodeNotValid);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "\xc2\x80/a", 4,  CHK_PUBTOPIC)) == ISMRC_BadTopic);
    mmsg.rc = 0;
    CU_ASSERT((rc = checkString(&mmsg, "z\xef\xbf\xbe/a", 6,  CHK_PUBTOPIC)) == ISMRC_BadTopic);
}

/*
 *
 */
void testparsetopic(void) {
    const char * topic;
    const char * subname;
    int subopt = 3;
    int subname_len;
    topic = parseTopic("$SharedSubscription/", &subname,  &subname_len, &subopt);
    CU_ASSERT(!strcmp("$SharedSubscription/",topic));
    CU_ASSERT(subname == NULL);
    CU_ASSERT(!subname_len);
    CU_ASSERT(!subopt);

    topic = parseTopic("$SharedSubscription/x/", &subname,  &subname_len, &subopt);
    CU_ASSERT(!strcmp("$SharedSubscription/x/",topic));
    CU_ASSERT(subname == NULL);
    CU_ASSERT(!subname_len);
    CU_ASSERT(!subopt);

    topic = parseTopic("$SharedSubscription/x/a", &subname,  &subname_len, &subopt);
    CU_ASSERT(!strcmp("a",topic));
    CU_ASSERT(!strncmp("x", subname, subname_len));
    CU_ASSERT(subname_len == 1);
    CU_ASSERT(!subopt);

    topic = parseTopic("$SharedSubscription/\x61\x62\x63/a", &subname, &subname_len, &subopt);
    CU_ASSERT(!strcmp("a",topic));
    CU_ASSERT(!strncmp("abc", subname, subname_len));
    CU_ASSERT(subname_len == 3);
    CU_ASSERT(!subopt);

    topic = parseTopic("$SharedSubscription/a\xc2\xa3\nx/a", &subname, &subname_len, &subopt);
    CU_ASSERT(!strcmp("a",topic));
    CU_ASSERT(!strncmp("a\xc2\xa3\nx", subname, subname_len));
    CU_ASSERT(subname_len == 5);
    CU_ASSERT(!subopt);

    topic = parseTopic("$select/QoS>0/$SharedSubscription/x/a", &subname,  &subname_len, &subopt);
    CU_ASSERT(!strcmp("a",topic));
    CU_ASSERT(!strncmp("x", subname, subname_len));
    CU_ASSERT(subname_len == 1);
    CU_ASSERT(subopt != 0);

    topic = parseTopic("$select/QoS=0/$share/x/a", &subname,  &subname_len, &subopt);
    CU_ASSERT(!strcmp("a",topic));
    CU_ASSERT(!strncmp("/x/a", subname, subname_len));
    CU_ASSERT(subname_len == 4);
    CU_ASSERT(subopt != 0);
}

/**
 * The close method for the transport object used for tests in
 * this source file.
 */
int mclose(ism_transport_t * transport, int rc, int clean, const char * reason) {
    g_entered_close_cb++;
    if (g_verbose > 1 && g_entered_close_cb == 1)
        printf("close connect=%d rc=%d reason=%s\n", transport->serverport, rc, reason);
    if (transport->state == ISM_TRANST_Open) {
        transport->state = ISM_TRANST_Closing;
        transport->closing(transport, rc, clean, reason);
    }

    return 0;
}

/**
 * The closed method for the transport object used for tests in * this source file.
 */
int mclosed(ism_transport_t * trans) {
    return 0;
}

/**
 * The send method for the transport object used for tests in
 * this source file.
 */
int msend(ism_transport_t * trans, char * buf, int len, int kind, int flags) {
    int i;
    if (g_cmd) {
        /* If CONNECT is sent, CONNACK should be returned */
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
        if (!strcmp(g_cmd,"CONNECT")) {
            g_cmd_result = kind>>4 == MT_CONNACK;
        }

        /* If PINREQ is sent, PINGRESP should be returned */
        if (!strcmp(g_cmd,"PINGREQ")) {
            g_cmd_result = kind>>4 == MT_PINGRESP;
        }

        /* If PUBLISH is sent, PUBACK should be returned */
        if (!strcmp(g_cmd,"PUBLISH")) {
            if (g_qos == 1) {
                g_cmd_result = kind>>4 == MT_PUBACK;
            }
            if (g_qos == 2) {
                g_cmd_result = kind>>4 == MT_PUBREC;
            }
        }

        /* If PUBREC is sent, PUBREL should be returned */
        if (!strcmp(g_cmd,"PUBREC")) {
            g_cmd_result = kind>>4 == MT_PUBREL;
        }

        /* If PUBREL is sent, PUBCOMP should be returned */
        if (!strcmp(g_cmd,"PUBREL")) {
            g_cmd_result = kind>>4 == MT_PUBCOMP;
        }

        /* If SUBSCRIBE is sent, SUBACK should be returned */
        if (!strcmp(g_cmd,"SUBSCRIBE")) {
            g_cmd_result = kind>>4 == MT_SUBACK;
        }

        /* If UNSUBSCRIBE is sent, UNSUBACK should be returned */
        if (!strcmp(g_cmd,"UNSUBSCRIBE")) {
            g_cmd_result = kind>>4 == MT_UNSUBACK;
        }
        g_ret_cmd = kind;
        if (g_verbose) {
            if (g_verbose > 1) {
                char zbuf [256];
                sprintf(zbuf, "command=%s response=%s %02x connect=%d", g_cmd, mqttCommand(kind), kind, trans->serverport);
                ism_common_traceData(zbuf, 0x08, "", 0, buf, len, len);
            } else {
                printf("command=%s response=%s %02x len=%d connect=%d\n", g_cmd, mqttCommand(kind), kind, len, trans->serverport);
            }
        }
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

void my_freeTransport(ism_transport_t * transport) {
    struct suballoc_t * suba = transport->suballoc.next;
    if (transport->state == ISM_TRANST_Open) {
        mclose(transport, 0, 1, "");
    }
    while (suba) {
        struct suballoc_t * freesub = suba;
        suba = suba->next;
        freesub->next = NULL;
        ism_common_free_anyType(freesub);
    }
    if (transport->listener)
        free(transport->listener);
    free(transport);
}

int mqttRx_test(int do_connect, ism_transport_t * transport, char * conn_buf, char * cmd_buf, int buflen, int kind, int check_response) {
    g_cmd = NULL;
    g_cmd_result = -1;

    char * l_conn_buf;
    char * l_cmd_buf = NULL;

    g_entered_close_cb = 0;
    if (do_connect)
        CU_ASSERT(ism_mqtt_connection(transport) == 0);

    if (conn_buf && cmd_buf) {
        int clen = 13;
        if (conn_buf[1] == 6)
            clen = 15;
        if (conn_buf[6]==5)
            clen = 14;
        l_conn_buf = alloca(clen);
        memcpy(l_conn_buf,  conn_buf, clen);
        CU_ASSERT(ism_mqtt_receive(transport, l_conn_buf, clen, 0x10 /* MT_CONNECT */) == 0);
        CU_ASSERT(transport->pobj->inprogress == 0);
    }

    if (cmd_buf) {
        l_cmd_buf = alloca(buflen);
        memcpy(l_cmd_buf,  cmd_buf, buflen);
    } else if (conn_buf) {
        l_cmd_buf = alloca(buflen);
        memcpy(l_cmd_buf,  conn_buf, buflen);
    }
    if (check_response) {
        g_cmd = mqttCommand((uint8_t)((kind>>4) & 15));
    }

    /* For cases where we want a valid message ID with PUBACK/PUBREC */
    if (transport->pobj && ((kind == 0x40)
                         || (kind == 0x50)
                         || (kind == 0x60)
                         || (kind == 0x70))) {
    	mqttProtoObj_t * pobj = (mqttProtoObj_t *)transport->pobj;
        uint16_t msgId = 1;
        uint16_t state = (kind == 0x70) ? ISM_MQTT_PUBREL /* Simulate that PUBREL was already sent */: ISM_MQTT_PUBLISH;
        ism_msgid_addMsgIdInfo(pobj->msgids,msgId,msgId, state);
        l_cmd_buf[1] = msgId & 0xFF;
        if (l_cmd_buf[0] == 0)
            l_cmd_buf[0] = (msgId >> 8) & 0xFF;
    }

    int rc = ism_mqtt_receive(transport, l_cmd_buf, buflen, kind);
    //char xbuf[256];
    //xbuf[0] = 0;
    //printf("result=%d %s\n", rc, ism_common_getErrorString(rc, xbuf, sizeof xbuf));
    return rc;
}


/* Test pobj closed causes failure return 1 */
void mqttRx_fail_pobjclosed(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    struct ism_protobj_t pobj = {0};
    transport->protocol = "mqtt-tcp";
    transport->trclevel = ism_defaultTrace;
    pobj.closed = 1;
    transport->pobj = &pobj;
    pobj.transport = transport;
    transport->closestate[3] = 1;
    pobj.msgids = ism_create_msgid_list(transport,0,0xFFFF);
    CU_ASSERT(mqttRx_test(0, transport, NULL, NULL, 0, 0, 0) == ISMRC_Closed);
    my_freeTransport(transport);
}

/* Test invalid command value as first command.
 * Should cause failure return ISMRC_ConnectFirst.
 */
void mqttRx_fail_invalidcommandfirst(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    setupTransport(transport, protocol, NULL,__LINE__);
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, NULL, NULL, 0, 9999, /* Invalid command */ 0) == ISMRC_ConnectFirst);
    CU_ASSERT(g_entered_close_cb == 1);
}

/* Test MT_PUBLISH as first command.
 * Should cause failure return ISMRC_ConnectFirst.
 */
void mqttRx_fail_mustconnectfirst(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    setupTransport(transport, protocol,NULL,__LINE__);
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1,transport, NULL, NULL, 0, 0x30, /* MT_PUBLISH */ 0) == ISMRC_ConnectFirst);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen of 0.
 * Should fail with ISMRC_BadLength.
 */
void mqttRx_fail_connectnodata(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    setupTransport(transport, protocol,NULL,__LINE__);
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, NULL, NULL, 0, 0x10, /* MT_CONNECT */ 0) == ISMRC_BadLength);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=3 and buf is values of 0 and 3.
 * Should fail with CRC_InvalidVersion because protocol name is required.
 */
void mqttRx_fail_connectnoprotname(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 3, 0x10, /* MT_CONNECT */ 1) == ISMRC_BadClientData);
    /* No call to transport->close() as CONNACK is sent to client so that
     * cleint can correct its input.
     */
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=4 and buf is values of 1, "p" and 3.
 * Should fail with CRC_InvalidVersion because protocol name is required and
 * must be six characters long.
 */
void mqttRx_fail_connectbadprotname1(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    // Protocol length
    conn_buf[0] = 0;
    conn_buf[1] = 1;

    // Protocol name
    conn_buf[2] = 'p';

    // Version
    conn_buf[3] = 3;
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 4, 0x10, /* MT_CONNECT */ 1) == ISMRC_BadClientData);
    /* No call to transport->close() as CONNACK is sent to client so that
     * cleint can correct its input.
     */
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=9 and buf is values of 6, "mQISdp" and 3.
 * Should fail with CRC_InvalidVersion because protocol name is required and
 * and the value is case sensitive.  (Correct name is "MQIsdp".)
 */
void mqttRx_fail_connectbadprotname2(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    // Protocol length
    conn_buf[0] = 0;
    conn_buf[1] = 6;

    // Protocol name
    conn_buf[2] = 'm';
    conn_buf[3] = 'Q';
    conn_buf[4] = 'I';
    conn_buf[5] = 'S';
    conn_buf[6] = 'd';
    conn_buf[7] = 'p';

    // Version - remove version value by setting to null
    conn_buf[8] = 3;
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 9, 0x10, /* MT_CONNECT */ 1) == ISMRC_BadClientData);
    /* No call to transport->close() as CONNACK is sent to client so that
     * cleint can correct its input.
     */
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=8 (to exclude version value) and buf is values of 6
 * and "MQIsdp (but no value for version).
 * Should fail with CRC_InvalidVersion because version value 3 is required.
 */
void mqttRx_fail_connectnoversion(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol, NULL ,__LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 8, 0x10, /* MT_CONNECT */ 1) == ISMRC_BadLength);
    /* No call to transport->close() as CONNACK is sent to client so that
     * cleint can correct its input.
     */
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=3 and version byte set to 2.
 * Should fail with CRC_InvalidVersion.
 */
void mqttRx_fail_connectwrongversion(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 2;
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 9, 0x10, /* MT_CONNECT */ 1) == CRC_InvalidVersion);
    /* No call to transport->close() as CONNACK is sent to client so that
     * cleint can correct its input.
     */
    CU_ASSERT(g_entered_close_cb == 1);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=15, version byte set to 3 and will flag set.
 * Should fail because if will flag is set, then will topic and will
 * message must also be provided.  Fails with ISMRC_BadLength due to missing
 * fields.
 */
void mqttRx_fail_connectbadwillmissingdata(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0x04;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 15, 0x10, /* MT_CONNECT */ 0) == ISMRC_BadLength);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=21, version byte set to 3, will flag set,
 * will QoS set to illegal value of 3 and will topic specified.
 * Should fail because will QoS of 3 is not allowed to be used.
 * Fails with ISMRC_InvalidQoS.
 */
void mqttRx_fail_connectbadwillqos1(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    // Protocol length
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0x1C;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';

    // Will topic length
    conn_buf[15] = 0;
    conn_buf[16] = 2;

    // Will topic
    conn_buf[17] = 'w';
    conn_buf[18] = 't';

    // Will message lentgh
    conn_buf[19] = 0;
    conn_buf[20] = 0;
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 21, 0x10, /* MT_CONNECT */ 0) == ISMRC_InvalidQoS);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=21, version byte set to 3, will flag set,
 * will QoS set to 1, will topic length set to 2, will topic "wt",
 * will message length set to 65535 with no will message.
 * Should fail with ISMRC_Badlength.
 */
void mqttRx_fail_connect_willmessagetoolong(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0x0C;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';

    // Will topic lentgh
    conn_buf[15] = 0;
    conn_buf[16] = 2;

    // Will topic
    conn_buf[17] = 'w';
    conn_buf[18] = 't';

    // Will message lentgh
    conn_buf[19] = 255;
    conn_buf[20] = 255;

    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 21, 0x10, /* MT_CONNECT */ 1) == ISMRC_BadLength);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=21, version byte set to 3, will flag set,
 * will QoS set to 1, will topic length set to 2, will topic "wt",
 * will message length set to 0 with no will message.
 * Should pass.
 */
void mqttRx_pass_connectwillnomessage(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol, NULL, __LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0x0C;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';

    // Will topic lentgh
    conn_buf[15] = 0;
    conn_buf[16] = 2;

    // Will topic
    conn_buf[17] = 'w';
    conn_buf[18] = 't';

    // Will message lentgh
    conn_buf[19] = 0;
    conn_buf[20] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 21, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=65556, version byte set to 3, will flag set,
 * will QoS set to 1, will topic length set to 2, will topic "wt",
 * will message length set to 65535 with corresponding will message.
 * Should pass.
 */
void mqttRx_pass_connectwillmaxmessagelen(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[70000] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0x0C;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';

    // Will topic lentgh
    conn_buf[15] = 0;
    conn_buf[16] = 2;

    // Will topic
    conn_buf[17] = 'w';
    conn_buf[18] = 't';

    // Will message lentgh
    conn_buf[19] = 255;
    conn_buf[20] = 255;
    memset(&conn_buf[21], 'm', 65535);

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 65556, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=23, version set to 3, keep alive set, client ID set,
 * will topic and will message set.
 * Should succeed.
 */
void mqttRx_pass_connectkpalwltpwlmsg(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0x0C;

    // Keepalive
    conn_buf[10] = 255;
    conn_buf[11] = 255;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';

    // Will topic lentgh
    conn_buf[15] = 0;
    conn_buf[16] = 2;

    // Will topic
    conn_buf[17] = 'w';
    conn_buf[18] = 't';

    // Will message lentgh
    conn_buf[19] = 0;
    conn_buf[20] = 2;

    // Will message
    conn_buf[21] = 'w';
    conn_buf[22] = 'm';
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 23, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=15, version set to 3 and client ID set.
 * Password flag set but unsername flag not set and no password provided.
 * Should fail with ISMRC_UsernameRequired.
 */
void mqttRx_fail_connectmissingpw(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0x40;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 15, 0x10, /* MT_CONNECT */ 0) == ISMRC_UsernameRequired);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=17, version set to 3 and client ID set.
 * Password flag set and password length of 0 provided but username flag not set.
 * Should fail with ISMRC_UsernameRequired.
 */
void mqttRx_fail_connectpwonly(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0x40;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';

    // Password length
    conn_buf[15] = 0;
    conn_buf[16] = 0;
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 17, 0x10, /* MT_CONNECT */ 0) == ISMRC_UsernameRequired);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=19, version set to 3 and client ID set.
 * Username and password flags set with username and password lengths of 0 provided.
 * Should succeed.
 */
void mqttRx_pass_connectunamewithpw(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    transport->listener->usePasswordAuth = 1;
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0xC0;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';

    // Username length
    conn_buf[15] = 0;
    conn_buf[16] = 0;

    // Password length
    conn_buf[17] = 0;
    conn_buf[18] = 0;

    g_entered_authenticate_user = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 19, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0 && g_entered_authenticate_user > 0);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with buflen=15, version set to 3 and client ID set.
 * Username flag set but no username provided.
 * Should succeed
 */
void mqttRx_pass_connectmissinguname(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    transport->listener->usePasswordAuth = 1;
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0x80;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 15, 0x10, /* MT_CONNECT */ 0) == 0);
    CU_ASSERT(g_entered_close_cb == 0);

}

/* Test MT_CONNECT with buflen=17, version set to 3 and client ID set.
 * Username flag set but and username length of 0 provided.
 * Should succeed.
 */
void mqttRx_pass_connectwithuname(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    transport->listener->usePasswordAuth = 1;
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0x80;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';

    // Username length
    conn_buf[15] = 0;
    conn_buf[16] = 0;

    g_entered_authenticate_user = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 17, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0 && g_entered_authenticate_user > 0);
    my_freeTransport(transport);
}


    /* Test MT_CONNECT with buflen=15, version set to 3 and client ID set.
     * Client ID length will be specified to be 23 even though the buflen
     * value will be set to 15.
     * Should fail with ISMRC_BadLength.
     */
    void mqttRx_fail_connect_clidfieldtoolong(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 23;

    // Client ID
    conn_buf[14] = 'c';

    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 15, 0x10, /* MT_CONNECT */ 1) == ISMRC_BadLength);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}


/* Test MT_CONNECT with buflen=15, version set to 3 and client ID set.
 * Should succeed.  This is the minimum data required for a connection.
 */
void mqttRx_pass_connect(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 15, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with different settings for AllowDurable
 */
void mqttRx_connect_allowdurable(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    /* Test AllowDurable=False with cleansession=0 */
    g_entered_close_cb = 0;
    setAllowDurable(0);
    /* Connect request should fail because cleansession=0 for this test case. */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 15, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    /* Confirm the connection was NOT closed so CONNACK can report authorization
     *failure.
     */
    CU_ASSERT(g_entered_close_cb == 0);
    /* Confirm that the connection was closed immediately by attempting to disconnect
     * after authorization failure on connect.
     */
    CU_ASSERT(mqttRx_test(0, transport, NULL, "", 0, 0xE0, /* MT_DISCONNECT */ 1) == ISMRC_Closed);

    /* Test AllowDurable=False with cleansession=1 */
    memset(conn_buf, 0, sizeof(conn_buf));
    setAllowDurable(0);
    /* Set up connect data with cleansession=1. */
    setupConnectionWithFlags(conn_buf, CFLAG_Clean /* 0x02 */);
    /* Connect request should succeed because cleansession=1. */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 15, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    /* Confirm that the connection succeeded by disconnecting without error. */
    CU_ASSERT(mqttRx_test(0, transport, NULL, "", 0, 0xE0, /* MT_DISCONNECT */ 1) == ISMRC_OK);

    /* Test AllowDurable=True with cleansession=0 */
    memset(conn_buf, 0, sizeof(conn_buf));
    setAllowDurable(1);
    setupConnection(conn_buf);
    /* Connect request should succeed because cleansession=0 is allowed now. */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 15, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    /* Confirm the connection was NOT closed */
    CU_ASSERT(g_entered_close_cb == 0);
    /* Confirm that the connection succeeded by disconnecting without error. */
    CU_ASSERT(mqttRx_test(0, transport, NULL, "", 0, 0xE0, /* MT_DISCONNECT */ 1) == ISMRC_OK);

    /* Test AllowDurable=True with cleansession=1 */
    memset(conn_buf, 0, sizeof(conn_buf));
    setAllowDurable(1);
    /* Set up connect data with cleansession=1. */
    setupConnectionWithFlags(conn_buf, CFLAG_Clean /* 0x02 */);
    /* Connect request should succeed because cleansession=0 is allowed now. */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 15, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    /* Confirm the connection was NOT closed */
    CU_ASSERT(g_entered_close_cb == 0);
    /* Confirm that the connection succeeded by disconnecting without error. */
    CU_ASSERT(mqttRx_test(0, transport, NULL, "", 0, 0xE0, /* MT_DISCONNECT */ 1) == ISMRC_OK);
    my_freeTransport(transport);
}

/* Test MT_CONNECT with will message and different settings for
 * AllowPersistentMessages.
 */
void mqttRx_connect_willmsg_allowpersistent(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);

    /* Test AllowPersistentMessages=False with will message QoS 2 */
    setupConnectionWithWillMsg(conn_buf, 2);
    g_entered_close_cb = 0;
    setAllowPersistentMessages(0);
    /* Connect request should fail because AllowPersistentMessages is false with QoS 2. */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 21, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    /* Confirm the connection was NOT closed so CONNACK can report authorization
     *failure.
     */
    CU_ASSERT(g_entered_close_cb == 0);
    /* Confirm that the connection was closed immediately by attempting to disconnect
     * after authorization failure on connect.
     */
    CU_ASSERT(mqttRx_test(0, transport, NULL, "", 0, 0xE0, /* MT_DISCONNECT */ 1) == ISMRC_Closed);

    /* Test AllowPersistentMessages=False with will message QoS 1 */
    setupConnectionWithWillMsg(conn_buf, 1);
    g_entered_close_cb = 0;
    setAllowPersistentMessages(0);
    /* Connect request should fail because AllowPersistentMessages is false with QoS 1. */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 21, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    /* Confirm the connection was NOT closed so CONNACK can report authorization
     *failure.
     */
    CU_ASSERT(g_entered_close_cb == 0);
    /* Confirm that the connection was closed immediately by attempting to disconnect
     * after authorization failure on connect.
     */
    CU_ASSERT(mqttRx_test(0, transport, NULL, "", 0, 0xE0, /* MT_DISCONNECT */ 1) == ISMRC_Closed);

    /* Test AllowPersistentMessages=False with will message QoS 0 */
    setupConnectionWithWillMsg(conn_buf, 0);
    g_entered_close_cb = 0;
    setAllowPersistentMessages(0);
    /* Connect request should succeed for AllowPersistentMessages=false because QoS=0. */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 21, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    /* Confirm the connection was NOT closed */
    CU_ASSERT(g_entered_close_cb == 0);
    /* Confirm that the connection succeeded by disconnecting without error. */
    CU_ASSERT(mqttRx_test(0, transport, NULL, "", 0, 0xE0, /* MT_DISCONNECT */ 1) == ISMRC_OK);

    /* Test AllowPersistentMessages=True with will message QoS 2 */
    setupConnectionWithWillMsg(conn_buf, 2);
    g_entered_close_cb = 0;
    setAllowPersistentMessages(1);
    /* Connect request should succeed for AllowPersistentMessages=true. */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 21, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    /* Confirm the connection was NOT closed */
    CU_ASSERT(g_entered_close_cb == 0);
    /* Confirm that the connection succeeded by disconnecting without error. */
    CU_ASSERT(mqttRx_test(0, transport, NULL, "", 0, 0xE0, /* MT_DISCONNECT */ 1) == ISMRC_OK);
    my_freeTransport(transport);
}

/*
 * Test MT_CONNECT with keepalive specified. Ensure that the connection is
 * closed after 2xtimeout without action.
 */
void mqttRx_pass_connect_keepalive(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    const int timeout = 2;	/* 2-second connection keepalive timeout */

    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    conn_buf[10] = 0;
    conn_buf[11] = timeout;

    char l_conn_buf[70000];
    char l_cmd_buf[70000];
    memset(l_conn_buf, 0, sizeof(l_conn_buf));
    memset(l_cmd_buf, 0, sizeof(l_cmd_buf));


    CU_ASSERT(ism_mqtt_connection(transport) == 0);

    memcpy(l_conn_buf,  conn_buf, 15);
    g_entered_close_cb = 0;
    CU_ASSERT(ism_mqtt_receive(transport, l_conn_buf, 15, 0x10 /* MT_CONNECT */) == 0);
    int i;
    for (i=0; i<120; i++) {          /* 12 seconds */
        if (g_entered_close_cb)
            break;
        ism_common_sleep(100000);    /* 100 milliseconds */
    }

    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);

}

/* Test MT_CONNECT with buflen=37, version set to 3, client ID set with clientID
 * length of 23 (max length allowed).
 * Should succeed.
 */
void mqttRx_pass_connect_clidmax(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 23;
    memset(&conn_buf[14], 'c', 23);

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 37, 0x10, /* MT_CONNECT */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}


/* Test MT_CONNECT with buflen=14, version set to 3, client ID set with clientID
 * length of 0 (less than min length allowed).
 * Should fail with CRC_BadIdentifier.
 */
void mqttRx_fail_connect_clidtooshort(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    memcpy(conn_buf, "\0\6MQIsdp", 8);

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 0;

    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, NULL, 14, 0x10, /* MT_CONNECT */ 1) == CRC_BadIdentifier);
    /* No call to transport->close() as CONNACK is sent to client so that
     * cleint can correct its input.
     */
    CU_ASSERT(g_entered_close_cb == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test invalid command after connect.
 * Should fail with ISMRC_InvalidCommand.
 */
void mqttRx_fail_invalidcommandafterconnect(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};

    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, "", 0, 9999, /* Invalid command */ 0) == ISMRC_InvalidCommand);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with buflen of 0.
 * Should fail with ISMRC_BadLength.
 */
void mqttRx_fail_publishnodata(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};

    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, "", 0, 0x30, /* MT_PUBLISH */ 1) == ISMRC_BadLength);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with buflen of 2.
 * Should fail with ISMRC_BadTopic.
 */
void mqttRx_fail_publishnulltopic(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pub Topic length
    pub_buf[0] = 0;
    pub_buf[1] = 0;

    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 2, 0x30, /* MT_PUBLISH */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with buflen of 5, non-null topic, invalid message ID.
 * To be sent with QoS set to 1 in the fixed header (0x32).
 * Should fail with ISMRC_InvalidID.
 */
void mqttRx_fail_publishbadid(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pub Topic length
    pub_buf[0] = 0;
    pub_buf[1] = 1;

    // Pub Topic
    pub_buf[2] = 't';

    // Message ID (0 is invalid)
    pub_buf[3] = 0;
    pub_buf[4] = 0;
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 5, 0x32, /* MT_PUBLISH */ 1) == ISMRC_InvalidID);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with buflen of 3, non-null topic, no message ID.
 * To be sent with QoS set to 1 in the fixed header.
 * Should fail with ISMRC_InvalidID.
 */
void mqttRx_fail_publishqos1nomsgid(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pub Topic length
    pub_buf[0] = 0;
    pub_buf[1] = 1;

    // Pub Topic
    pub_buf[2] = 't';

    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 3, 0x32, /* MT_PUBLISH */ 1) == ISMRC_InvalidID);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with buflen of 3 and non-null topic.
 * Invalid QoS of 3.
 * Should fail with ISMRC_InvalidQoS.
 */
void mqttRx_fail_publish_badqos(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pub Topic length
    pub_buf[0] = 0;
    pub_buf[1] = 1;

    // Pub Topic
    pub_buf[2] = 't';

    g_entered_close_cb = 0;
    /* QoS 3 */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 3, 0x36, /* MT_PUBLISH */ 1) == ISMRC_InvalidQoS);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with buflen of 3 but topic length set to 255.
 * Should fail with ISMRC_BadTopic.
 */
void mqttRx_fail_publish_topictoolong(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pub Topic length
    pub_buf[0] = 0;
    pub_buf[1] = 255;

    // Pub Topic
    pub_buf[2] = 't';

    g_entered_close_cb = 0;
    /* QoS 0 */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 3, 0x30, /* MT_PUBLISH */ 1) == ISMRC_BadLength);
    CU_ASSERT(g_cmd_result < 1);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with buflen of 3 and non-null topic.
 * Should succeed. This is the smallest possible publish message.
 * Also check responses for QoS 0 (none) 1 (PUBACK) and 2 (PUBREC).
 */
void mqttRx_pass_publish(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pub Topic length
    pub_buf[0] = 0;
    pub_buf[1] = 1;

    // Pub Topic
    pub_buf[2] = 't';

    /* QoS 0 */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 3, 0x30, /* MT_PUBLISH */ 1) == 0);
    CU_ASSERT(g_cmd_result < 1);

    pub_buf[3] = 0;
    pub_buf[4] = 1;
    /* QoS 1 */
    g_qos = 1;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 5, 0x32, /* MT_PUBLISH */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);

    /* QoS 2 */
    g_qos = 2;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 5, 0x34, /* MT_PUBLISH */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with message too big.
 * Should succeed. Also check responses
 * for QoS 0 (none) 1 (PUBACK) and 2 (PUBREC).
 */
void mqttRx_pass_publish_msgtoobig(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);
    transport->listener->maxMsgSize = 2;
    // Pub Topic length
    pub_buf[0] = 0;
    pub_buf[1] = 1;

    // Pub Topic
    pub_buf[2] = 't';

    // Message payload
    pub_buf[3] = '1';
    pub_buf[4] = '2';
    pub_buf[5] = '3';


    /* QoS 0 */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 6, 0x30, /* MT_PUBLISH */ 1) == 0);
    CU_ASSERT(g_cmd_result < 1);
    CU_ASSERT(ism_common_getLastError() == ISMRC_MsgTooBig);

    pub_buf[3] = 0;
    pub_buf[4] = 1;

    // Message payload
    pub_buf[5] = '1';
    pub_buf[6] = '2';
    pub_buf[7] = '3';

    /* QoS 1 */
    g_qos = 1;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 8, 0x32, /* MT_PUBLISH */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    CU_ASSERT(ism_common_getLastError() == ISMRC_MsgTooBig);

    /* QoS 2 */
    g_qos = 2;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 8, 0x34, /* MT_PUBLISH */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    CU_ASSERT(ism_common_getLastError() == ISMRC_MsgTooBig);
    my_freeTransport(transport);
}

/* Test MT_PUBACK with buflen of 2 and a message ID that does not exist.
 * Should pass (invalid message ID should just be ignored).
 */
void mqttRx_pass_puback_msgdne(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char puback_buf[100] = {0};
    struct ism_protobj_t pobj = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    transport->pobj = &pobj;
    setupConnection(conn_buf);

    // Puback message ID
    puback_buf[0] = 1;
    // puback_buf[1] will be set in mqttRx_test()

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, puback_buf, 2, 0x40, /* MT_PUBACK */ 0) == 0);
    my_freeTransport(transport);
}

/* Test MT_PUBACK with buflen of 2 and a valid message ID.
 * Should pass.
 */
void mqttRx_pass_puback(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char puback_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Puback message ID
    puback_buf[0] = 0;
    // puback_buf[1] will be set in mqttRx_test()

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, puback_buf, 2, 0x40, /* MT_PUBACK */ 0) == 0);
    my_freeTransport(transport);
}

/* Test MT_PUBREC with buflen of 2 and a message ID that does not exist.
 * Should pass (invalid message ID should just be ignored).
 */
void mqttRx_pass_pubrec_msgdne(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pubrec_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pubrec message ID
    pubrec_buf[0] = 1;
    // pubrec_buf[1] will be set in mqttRx_test()

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pubrec_buf, 2, 0x50, /* MT_PUBREC */ 1) == 0);
    /* NOTE: This behavior might change.
     *       For now, test to the implementation which will NOT provide PUBREL
     *       when message ID in PUBREC does not exist.
     */
    // CU_ASSERT(g_cmd_result == -1);
    my_freeTransport(transport);
}

/* Test MT_PUBREC with buflen of 2 and a valid message ID.
 * Should pass.
 */
void mqttRx_pass_pubrec(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pubrec_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pubrec message ID
    pubrec_buf[0] = 0;
    // pubrec_buf[1] will be set in mqttRx_test()

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pubrec_buf, 2, 0x50, /* MT_PUBREC */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_PUBREL with buflen of 2 and a message ID that does not exist.
 * Should pass (invalid message ID should just be ignored).
 */
void mqttRx_pass_pubrel_msgdne(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pubrel_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pubrel message ID
    pubrel_buf[0] = 1;
    // pubrel_buf[1] will be set in mqttRx_test()

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pubrel_buf, 2, 0x60, /* MT_PUBREL */ 1) == 0);
    /* NOTE: This behavior might change.
     *       It is not consistent with the behavior for missing ID with PUBREC.
     *       TODO: Follow up on consistency of behaviors.
     */
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_PUBREL with buflen of 2 and a valid message ID.
 * Should pass.
 */
void mqttRx_pass_pubrel(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pubrel_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pubrel message ID
    pubrel_buf[0] = 0;
    // pubrel_buf[1] will be set in mqttRx_test()

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pubrel_buf, 2, 0x60, /* MT_PUBREL */ 1) == 0);
    my_freeTransport(transport);
}

void mqttRx_pass_pubcomp_msgdne(void) {
    g_entered_confirmDelivery = 0;
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pubcomp_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pubcomp message ID
    pubcomp_buf[0] = 1;
    // pubcomp_buf[1] will be set in mqttRx_test()

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pubcomp_buf, 2, 0x70, /* MT_PUBCOMP */ 0) == 0);
    CU_ASSERT(g_entered_confirmDelivery == 0);
    my_freeTransport(transport);
}

void mqttRx_pass_pubcomp(void) {
    g_entered_confirmDelivery = 0;
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pubcomp_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pubcomp message ID
    pubcomp_buf[0] = 0;
    // pubcomp_buf[1] will be set in mqttRx_test()

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pubcomp_buf, 2, 0x70, /* MT_PUBCOMP */ 0) == 0);
    CU_ASSERT(g_entered_confirmDelivery == 1);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with buflen of 32770 (topic length will be 32768 - just
 * over the length limit of 32767).
 * As of MQTT 3.1.1 this now works
 */
void mqttRx_fail_publish_invalidtopiclen(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pub_buf[70000] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pub Topic length
    pub_buf[0] = 128;
    pub_buf[1] = 0;

    // Pub Topic
    memset(&pub_buf[2], 't', 32768);

    g_entered_close_cb = 0;
    /* QoS 0 */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 32770, 0x30, /* MT_PUBLISH */ 1) == 0);
    CU_ASSERT(g_cmd_result < 1);
    CU_ASSERT(g_entered_close_cb == 0);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with buflen of 32769 (topic length will be 32767).
 * Should succeed.
 */
void mqttRx_pass_publish_maxtopiclen(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pub_buf[70000] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pub Topic length
    pub_buf[0] = 127;
    pub_buf[1] = 255;

    // Pub Topic
    memset(&pub_buf[2], 't', 32767);

    /* QoS 0 */
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 32769, 0x30, /* MT_PUBLISH */ 1) == 0);
    CU_ASSERT(g_cmd_result < 1);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with buflen of 5 and wildcard topic (#).
 * Should fail with ISMRC_BadTopic.
 */
void mqttRx_fail_publish_wildcardtopic1(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pub Topic length
    pub_buf[0] = 0;
    pub_buf[1] = 3;

    // Pub Topic
    pub_buf[2] = 't';
    pub_buf[3] = '/';
    pub_buf[4] = '#';
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 5, 0x30, /* MT_PUBLISH */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);
}

/* Test MT_PUBLISH with buflen of 5 and wildcard topic (+).
 * Should fail with ISMRC_BadTopic.
 */
void mqttRx_fail_publish_wildcardtopic2(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char pub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Pub Topic length
    pub_buf[0] = 0;
    pub_buf[1] = 3;

    // Pub Topic
    pub_buf[2] = 't';
    pub_buf[3] = '/';
    pub_buf[4] = '+';
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, pub_buf, 5, 0x30, /* MT_PUBLISH */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/*
 * Test MT_PUBLISH to $SharedSubscription
 */
void mqttRx_publish_shrdsub(void) {
    xUNUSED int rc;
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * pub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    pub_buf = "\0\x13$SharedSubscription";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 21, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(g_entered_close_cb == 0);

    pub_buf = "\0\x14$SharedSubscription/";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 22, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);

    pub_buf = "\0\x15$SharedSubscription/x";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 23, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);

    pub_buf = "\0\x16$SharedSubscription/x/";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 24, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);

    pub_buf = "\0\x17$SharedSubscription/x/a";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 25, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);

    pub_buf = "\0\x17$SharedSubscription/a\xc2\xa3\nx/a";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 29, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);

    pub_buf = "\0\x17$SharedSubscription/x/a\xc2\xa3\nx";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 29, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);

    pub_buf = "\0\x17$SharedSubscription/\x00\x41\xf0\xa0\xa0\x80/a";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 30, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_UnicodeNotValid);
    CU_ASSERT(g_entered_close_cb == 1);

    pub_buf = "\0\x17$SharedSubscription/x/\x00\x41\xf0\xa0\xa0\x80";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 30, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);

    pub_buf = "\0\x17$SharedSubscription/z\xef\xbf\xbe/a";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 28, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_UnicodeNotValid);
    CU_ASSERT(g_entered_close_cb == 1);

    pub_buf = "\0\x17$SharedSubscription/x/z\xef\xbf\xbe";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 28, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_BadTopic);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/*
 * Test MT_PUBLISH dynamic producer creation
 */
void mqttRx_publish_dynprod(void) {
    xUNUSED int rc;
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * pub_buf;
    char max_pub_buf[1027] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    /* Publish 4 messages to the same topic.
     * 4th message results in creation of a new producer.
     * The new producer is used to publish the 4th message.
     */
    pub_buf = "\0\x06topic1";
    g_entered_putMessageWithDeliveryId = 0;
    /* Send 1st msg */
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 8, 0x30, /* MT_PUBLISH */ 1)) == 0);
    /* Initialize publisher count, etc. to track last published topic */
    CU_ASSERT(transport->pobj->publisher[0].count == 1);
    /* Assure the alloc value is the correct multiple of 8 given the topic name lenght */
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_alloc == 8);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_len == 6);
    CU_ASSERT(transport->pobj->publisher[0].producerh == NULL);
    CU_ASSERT(memcmp(transport->pobj->publisher[0].lasttopic,"topic1",6) == 0);
    CU_ASSERT(g_entered_putMessageWithDeliveryId == 0);

    /* Send 2nd msg - confirm that consecutive count is incermented */
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 8, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].count == 2);
    CU_ASSERT(g_entered_putMessageWithDeliveryId == 0);

    /* Send 3rd msg - confirm that consecutive count is incermented */
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 8, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].count == 3);
    /* Confirm producer handle is no longer null because a producer was created. */
    CU_ASSERT(transport->pobj->publisher[0].producerh != NULL);
    CU_ASSERT(memcmp(transport->pobj->publisher[0].lasttopic,"topic1",6) == 0);
    /* Confirm that the producer was used to publish this message */
    CU_ASSERT(g_entered_putMessageWithDeliveryId == 1);
    g_entered_putMessageWithDeliveryId = 0;
    /* Send 4th msg - confirm that consecutive count is incermented and that
     * producer object is created.
     */
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 8, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_alloc == 8);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_len == 6);
    /* Confirm that the producer was used to publish this message */
    CU_ASSERT(g_entered_putMessageWithDeliveryId == 1);
    g_entered_putMessageWithDeliveryId = 0;

    /* Publish to a different topic where topic name length matches
     * the length of the previous topic name and confirm that
     * all publisher information is reinitialized.
     */
    g_entered_destroyProducer = 0;
    pub_buf = "\0\x06topic2";
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 8, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].count == 1);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_alloc == 8);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_len == 6);
    CU_ASSERT(transport->pobj->publisher[0].producerh == NULL);
    CU_ASSERT(memcmp(transport->pobj->publisher[0].lasttopic,"topic2",6) == 0);
    /* Confirm the previously created producer was destroyed */
    CU_ASSERT(g_entered_destroyProducer == 1);
    g_entered_destroyProducer = 0;

    /* Publish to a different topic where topic name length is larger than
     * the previous topic and is an exact multiple of 8 but is less than the
     * maximum alloc size of 1024.  Confirm the resulting alloc is the topic name
     * length + 8.
     */
    pub_buf = "\0\x10topic67890123456";
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 18, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].count == 1);
    /* Confirm alloc value is topic name length + 8. */
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_alloc == 24);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_len == 16);
    CU_ASSERT(transport->pobj->publisher[0].producerh == NULL);
    CU_ASSERT(memcmp(transport->pobj->publisher[0].lasttopic,"topic67890123456",16) == 0);
    /* No producer was created for previous topic so there is no need to enter
     * destroyProducer in this case.
     */
    CU_ASSERT(g_entered_destroyProducer == 0);

    /* Publish to a different topic where topic name length less than
     * the length of the previous topic name and confirm that
     * all publisher information is reinitialized but that the alloc value
     * remains unchanged.
     */
    pub_buf = "\0\x09topic0123";
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 11, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].count == 1);
    /* Assure the alloc value is unchanged for the shorter topic name length. */
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_alloc == 24);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_len == 9);
    CU_ASSERT(transport->pobj->publisher[0].producerh == NULL);
    CU_ASSERT(memcmp(transport->pobj->publisher[0].lasttopic,"topic0123",9) == 0);
    /* No producer was created for previous topic so there is no need to enter
     * destroyProducer in this case.
     */
    CU_ASSERT(g_entered_destroyProducer == 0);

    /* Publish to a different topic where topic name length is
     * maximum allowed length of 1024 bytes and confirm that
     * all publisher information is reinitialized.
     *
     * Publish 4 messages to this topic.
     * 4th message results in creation of a new producer.
     * The new producer is used to publish the 4th message.
     */
    memset(&max_pub_buf[0], 0x04, 1);
    memset(&max_pub_buf[1], 0x00, 1);
    memset(&max_pub_buf[2], 't', 1024);
    pub_buf = max_pub_buf;
    /* Send 1st msg */
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 1026, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].count == 1);
    /* Assure the alloc value does not add 8 bytes when topic length is max alloc size */
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_alloc == 1024);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_len == 1024);
    CU_ASSERT(transport->pobj->publisher[0].producerh == NULL);
    CU_ASSERT(memcmp(transport->pobj->publisher[0].lasttopic,&max_pub_buf[2],1024) == 0);
    CU_ASSERT(g_entered_destroyProducer == 0);
    CU_ASSERT(g_entered_putMessageWithDeliveryId == 0);

    /* Send 2nd msg - confirm that consecutive count is incermented */
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 1026, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].count == 2);
    CU_ASSERT(g_entered_putMessageWithDeliveryId == 0);

    /* Send 3rd msg - confirm that consecutive count is incermented */
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 1026, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].count == 3);
    /* Confirm producer handle is no longer null because a producer was created. */
    CU_ASSERT(transport->pobj->publisher[0].producerh != NULL);
    CU_ASSERT(g_entered_putMessageWithDeliveryId == 1);
    g_entered_putMessageWithDeliveryId = 0;
    /* Send 4th msg - confirm that consecutive count is incermented and that
     * producer object is created.
     */
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 1026, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_alloc == 1024);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_len == 1024);
    CU_ASSERT(memcmp(transport->pobj->publisher[0].lasttopic,&max_pub_buf[2],1024) == 0);
    /* Confirm that the producer was used to publish this message */
    CU_ASSERT(g_entered_putMessageWithDeliveryId == 1);
    g_entered_putMessageWithDeliveryId = 0;

    /* Publish to a different topic where topic name length exceeds
     * the maximum allowed length of 1024 bytes and confirm that
     * all publisher information from the previously cached last topic
     * remains unchanged except for the producer handle (prodcuer handle
     * needs to be discarded so that we do not publish to it).
     *
     * Publish 2 messages to this same "too large" topic and confirm
     * that information for previously cached topic remains in place.
     */
    memset(&max_pub_buf[0], 0x04, 1);
    memset(&max_pub_buf[1], 0x01, 1);
    memset(&max_pub_buf[2], 't', 1025);
    pub_buf = max_pub_buf;
    /* Publish 1st message - info for previous cached topic should remain except for producerh.
     */
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 1027, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].count == 3);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_alloc == 1024);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_len == 1024);
    CU_ASSERT(transport->pobj->publisher[0].producerh != NULL);
    CU_ASSERT(memcmp(transport->pobj->publisher[0].lasttopic,&max_pub_buf[2],1024) == 0);
    /* Confirm destoryProducer is not called and that the message is not published
     * using the existing producer.
     */
    CU_ASSERT(g_entered_destroyProducer == 0);
    g_entered_destroyProducer = 0;
    CU_ASSERT(g_entered_putMessageWithDeliveryId == 0);

    /* Publish 2nd message for the topic with a name that is too large to cache
     * - info for previously cached topic should remain. Producer handle should
     * still be null.
     */
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 1027, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].count == 3);
    CU_ASSERT(transport->pobj->publisher[0].producerh != NULL);
    /* Confirm destoryProducer is called and that the message is not published
     * using the previously existing producer.
     */
    CU_ASSERT(g_entered_putMessageWithDeliveryId == 0);

    /* Now publish to another message to the topic that is still cached as
     * lasttopic.  All publisher information should remain unchanged because
     * the producer already exists and will used to publish the message to consumers.
     */
    memset(&max_pub_buf[0], 0x04, 1);
    memset(&max_pub_buf[1], 0x00, 1);
    memset(&max_pub_buf[2], 't', 1024);
    pub_buf = max_pub_buf;
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 1026, 0x30, /* MT_PUBLISH */ 1)) == 0);
    CU_ASSERT(transport->pobj->publisher[0].count == 3);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_alloc == 1024);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_len == 1024);
    CU_ASSERT(transport->pobj->publisher[0].producerh != NULL);
    CU_ASSERT(memcmp(transport->pobj->publisher[0].lasttopic,&max_pub_buf[2],1024) == 0);
    /* Confirm that message is published using the existing producer */
    CU_ASSERT(g_entered_putMessageWithDeliveryId == 1);
    g_entered_putMessageWithDeliveryId = 0;

    /* Publish to a different topic where topic name length is the
     * minimum length of 1.
     */
    pub_buf = "\0\x01q";
    CU_ASSERT((rc = mqttRx_test(0, transport, NULL, pub_buf, 3, 0x30, /* MT_PUBLISH */ 1)) == 0);
    /* Confirm all values except for alloc are reinitialized to reflect the newly
     * cached topic.
     */
    CU_ASSERT(transport->pobj->publisher[0].count == 1);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_alloc == 1024);
    CU_ASSERT(transport->pobj->publisher[0].lasttopic_len == 1);
    CU_ASSERT(transport->pobj->publisher[0].producerh == NULL);
    CU_ASSERT(memcmp(transport->pobj->publisher[0].lasttopic,"q",1) == 0);
    CU_ASSERT(g_entered_destroyProducer == 1);

    CU_ASSERT(g_entered_close_cb == 0);
    my_freeTransport(transport);
}

/* Test MT_PUBLISH with different settings for AllowPersistentMessages */
void mqttRx_publish_allowpersistentmsgs(void) {
    xUNUSED int rc;
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * pub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);

    /* Test publish msg with AllowPersistentMessages=False and QoS=2. */
    setupConnection(conn_buf);
    setAllowPersistentMessages(0);
    pub_buf = "\0\x06topic1";
    /* Send QoS 2 msg */
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 8, 0x34, /* MT_PUBLISH */ 1)) == ISMRC_InvalidQoS);
    CU_ASSERT(g_entered_close_cb == 1);

    /* Test publish msg with AllowPersistentMessages=False and QoS=1. */
    g_entered_close_cb = 0;
    setupConnection(conn_buf);
    setAllowPersistentMessages(0);
    pub_buf = "\0\x06topic1";
    /* Send QoS 1 msg */
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 8, 0x32, /* MT_PUBLISH */ 1)) == ISMRC_InvalidQoS);
    CU_ASSERT(g_entered_close_cb == 1);

    /* Test publish msg with AllowPersistentMessages=False and QoS=0. */
    g_entered_close_cb = 0;
    setupConnection(conn_buf);
    setAllowPersistentMessages(0);
    pub_buf = "\0\x06topic1";
    /* Send QoS 0 msg */
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 8, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_OK);
    CU_ASSERT(g_entered_close_cb == 0);
    /* Confirm that the connection remained active by disconnecting without error. */
    CU_ASSERT(mqttRx_test(0, transport, NULL, "", 0, 0xE0, /* MT_DISCONNECT */ 1) == ISMRC_OK);
    CU_ASSERT(g_entered_close_cb == 1);

    /* Test publish msg with AllowPersistentMessages=True and QoS=2. */
    g_entered_close_cb = 0;
    setupConnection(conn_buf);
    setAllowPersistentMessages(1);
    pub_buf = "\0\x06topic1";
    /* Send QoS 0 msg */
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, pub_buf, 8, 0x30, /* MT_PUBLISH */ 1)) == ISMRC_OK);
    CU_ASSERT(g_entered_close_cb == 0);
    /* Confirm that the connection remained active by disconnecting without error. */
    CU_ASSERT(mqttRx_test(0, transport, NULL, "", 0, 0xE0, /* MT_DISCONNECT */ 1) == ISMRC_OK);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with QoS 0 fixed header.
 * Should fail with ISMRC_InvalidValue.
 */
void mqttRx_fail_subscribebadqos(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, "", 0, 0x80, /* MT_SUBSRIBE */ 1) == ISMRC_InvalidValue);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with QoS 1 in fixed header but no other data.
 * Should fail with ISMRC_BadLength.
 */
void mqttRx_fail_subscribenodata(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, "", 0, 0x82, /* MT_SUBSRIBE */ 1) == ISMRC_BadLength);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with bad message ID 0, topic and QoS specifided.
 * Should fail with ISMRC_InvalidID.
 */
void mqttRx_fail_subscribebadid1(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 0;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 't';

    // Requested QoS for Topic
    sub_buf[5] = 0;
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 6, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_InvalidID);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with message ID, topic and QoS specifided in fixed header.
 * Requested QoS set to invalid value 3.
 * Should fail with ISMRC_InvalidQoS.
 */
void mqttRx_fail_subscribe_badrequestedqos(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 't';

    // Requested QoS for Topic
    sub_buf[5] = 3;
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 6, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_InvalidQoS);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with message ID, topic and QoS specifided.
 * Topic length will be set to 255 but buffer length will only be 6.
 * Should fail with ISMRC_BadTopic.
 */
void mqttRx_fail_subscribe_topictoolong(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);
    int rc;

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 255;

    // Sub Topic
    sub_buf[4] = 't';

    // Requested QoS for Topic
    sub_buf[5] = 0;
    g_entered_close_cb = 0;
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, sub_buf, 6, 0x82, /* MT_SUBSCRIBE */ 1)) == ISMRC_BadClientData);
    // printf("Line %d: rc=%d\n", __LINE__, rc);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}


/* Test MT_SUBSCRIBE with message ID, topic and QoS specifided.
 * Should succeed. This is the smallest possible subscribe message.
 */
void mqttRx_pass_subscribe(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 0x01;

    // Requested QoS for Topic
    sub_buf[5] = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 6, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with message ID, topic and QoS specifided.
 * Topic length will be specified as 32768 (just over the the maxixum
 * of 32767 allowed).
 * This was changed in 3.1.1 to be allowed.
 */
void mqttRx_fail_subscribe_invalidtopiclen(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[70000] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 128;
    sub_buf[3] = 0;

    // Sub Topic
    memset(&sub_buf[4], 't', 32768);

    // Requested QoS for Topic
    sub_buf[32772] = 0;
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 32773, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_entered_close_cb == 0);

}

/* Test MT_SUBSCRIBE with message ID, topic and QoS specifided.
 * Topic length will be specified as 32767 (the maxixum allowed).
 * Should succeed.
 */
void mqttRx_pass_subscribe_maxtopiclen(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[70000] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 127;
    sub_buf[3] = 255;

    // Sub Topic
    memset(&sub_buf[4], 't', 32767);

    // Requested QoS for Topic
    sub_buf[32771] = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 32772, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with message ID set to max value (65535), topic and QoS specifided.
 * Should succeed. This is the smallest possible subscribe message.
 */
void mqttRx_pass_subscribeidmax(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 255;
    sub_buf[1] = 255;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 't';

    // Sub QoS for Topic
    sub_buf[5] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 6, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with valid multilevel wildcard (#) topics.
 * Should succeed.
 */
void mqttRx_pass_subscribe_wildcardtopicpound(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 255;
    sub_buf[1] = 255;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = '#';

    // Sub QoS for Topic
    sub_buf[5] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 6, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 3;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '#';

    // Sub QoS for Topic
    sub_buf[7] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 8, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with valid multilevel wildcard (#) topics.
 * Should succeed.
 */
void mqttRx_pass_subscribe_wildcardtopicplus(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 255;
    sub_buf[1] = 255;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = '+';

    // Sub QoS for Topic
    sub_buf[5] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 6, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 3;

    // Sub Topic
    sub_buf[4] = '+';
    sub_buf[5] = '/';
    sub_buf[6] = 't';

    // Sub QoS for Topic
    sub_buf[7] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 8, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 3;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '+';

    // Sub QoS for Topic
    sub_buf[7] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 8, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 5;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '+';
    sub_buf[7] = '/';
    sub_buf[8] = 't';

    // Sub QoS for Topic
    sub_buf[9] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 10, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with topics containing valid mixes of multilevel (#)
 * single-level wildcard (+) topics.
 * Should succeed.
 */
void mqttRx_pass_subscribe_wildcardtopicmixed(void) {
    int  rc;
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 255;
    sub_buf[1] = 255;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 3;

    // Sub Topic
    sub_buf[4] = '+';
    sub_buf[5] = '/';
    sub_buf[6] = '#';

    // Sub QoS for Topic
    sub_buf[7] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 8, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 5;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '+';
    sub_buf[7] = '/';
    sub_buf[8] = '#';

    // Sub QoS for Topic
    sub_buf[9] = 0;

    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, sub_buf, 10, 0x82, /* MT_SUBSCRIBE */ 1)) == 0);
    CU_ASSERT(g_cmd_result > 0);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 7;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '+';
    sub_buf[7] = '/';
    sub_buf[8] = 't';
    sub_buf[9] = '/';
    sub_buf[10] = '#';

    // Sub QoS for Topic
    sub_buf[11] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 12, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 9;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '+';
    sub_buf[7] = '/';
    sub_buf[8] = '+';
    sub_buf[9] = '/';
    sub_buf[10] = 't';
    sub_buf[11] = '/';
    sub_buf[12] = '#';

    // Sub QoS for Topic
    sub_buf[13] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 14, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

void mqttRx_subscribe_multi(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    sub_buf = "\xff\xff\0\3$SY\0\0\4$SYS\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 15, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadSysTopic);
    CU_ASSERT(g_cmd_result < 1);

    sub_buf = "\xff\xff\0\3top\0\0\4/top\0\0\12/topic/sub\00\13topic/sub/1\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 28, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);

    sub_buf = "\xff\xff\0\5t/#/t\0\0\3ttt\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 16, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);

    sub_buf = "\xff\xff\0\3$SY\0\0\3ttt\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 14, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadSysTopic);
    CU_ASSERT(g_cmd_result < 1);

    sub_buf = "\xff\xff\0\6topi/#\0\0\6top/+/\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 20, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

void mqttRx_subscribe_sys(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    sub_buf = "\xff\xff\0\4$SYS\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 9, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadSysTopic);
    CU_ASSERT(g_cmd_result < 1);

    sub_buf = "\xff\xff\0\5$SYS_\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 10, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadSysTopic);
    CU_ASSERT(g_cmd_result < 1);

    sub_buf = "\xff\xff\0\3$SY\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 8, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadSysTopic);
    CU_ASSERT(g_cmd_result < 1);

    sub_buf = "\xff\xff\0\6$SYS/#\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 11, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

void mqttRx_subscribe_shrdsub(void) {
    int rc;
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    g_entered_createConsumer = 0;
    sub_buf = "\xff\xff\0\x13$SharedSubscription\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 24, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadSysTopic);
    CU_ASSERT(g_entered_createConsumer == 0);
    CU_ASSERT(g_cmd_result < 1);

    g_entered_createConsumer = 0;
    sub_buf = "\xff\xff\0\x14$SharedSubscription/\0";
    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, sub_buf, 25, 0x82, /* MT_SUBSCRIBE */ 1)) == ISMRC_BadTopic);
    // printf("Line %d: %d\n", __LINE__, rc);
    CU_ASSERT(g_cmd_result < 1);
    CU_ASSERT(g_entered_createConsumer == 0);

    g_entered_createConsumer = 0;
    sub_buf = "\xff\xff\0\x15$SharedSubscription/x\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 26, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);
    CU_ASSERT(g_entered_createConsumer == 0);

    g_entered_createConsumer = 0;
    sub_buf = "\xff\xff\0\x16$SharedSubscription/x/\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 27, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);
    CU_ASSERT(g_entered_createConsumer == 0);

    g_entered_createConsumer = 0;
    sub_buf = "\xff\xff\0\x17$SharedSubscription/x/a\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 28, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    CU_ASSERT(g_entered_createConsumer == 1);
    my_freeTransport(transport);
}

void mqttRx_subscribe_multishrdsub(void) {
    int rc;
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char * sub_buf;
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    g_entered_createConsumer = 0;
    sub_buf = "\xff\xff\0\x13$SharedSubscription\0\0\x13$Sharedxxxscription\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 46, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadSysTopic);
    CU_ASSERT(g_cmd_result < 1);
    CU_ASSERT(g_entered_createConsumer == 0);

    g_entered_createConsumer = 0;
    sub_buf = "\xff\xff\0\x17$SharedSubscription/x/a\0\0\x17$SharedSubscription/a/b\0\0\x17$SharedSubscription/b/c\0\0\x17$SharedSubscription/c/d\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 106, 0x82, /* MT_SUBSCRIBE */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    CU_ASSERT(g_entered_createConsumer == 4);

    g_entered_createConsumer = 0;
    sub_buf = "\xff\xff\0\x17$Shared/#/scription/x/a\0\0\x17$SharedSubscription/a/b\0\0\x17$SharedSubscription/b/c\0\0\x17$SharedSubscription/c/d\0";
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 106, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadSysTopic);
    CU_ASSERT(g_cmd_result < 1);
    CU_ASSERT(g_entered_createConsumer == 0);

    g_entered_createConsumer = 0;
    sub_buf = "\xff\xff\0\x17$SharedSubscription/x/a\0\0\x17$SharedSubscription/a/b\0\0\x17$SharedSubscription/b/c\0\0\x19$SharedSubscription/c/#/d\0";
    CU_ASSERT((rc =mqttRx_test(1, transport, conn_buf, sub_buf, 106, 0x82, /* MT_SUBSCRIBE */ 1)) == ISMRC_BadClientData);
    // printf("Line %d: rc=%d\n", __LINE__, rc);
    CU_ASSERT(g_cmd_result < 1);
    CU_ASSERT(g_entered_createConsumer == 0);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with invalid multilevel wildcard (#) topics.
 * Should fail with ISMRC_BadTopic.
 */
void mqttRx_fail_subscribe_wildcardtopicpound(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 255;
    sub_buf[1] = 255;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 2;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '#';

    // Sub QoS for Topic
    sub_buf[6] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 7, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 2;

    // Sub Topic
    sub_buf[4] = '#';
    sub_buf[5] = 't';

    // Sub QoS for Topic
    sub_buf[6] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 7, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 3;

    // Sub Topic
    sub_buf[4] = '#';
    sub_buf[5] = '/';
    sub_buf[6] = 't';

    // Sub QoS for Topic
    sub_buf[7] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 8, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 4;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '#';
    sub_buf[7] = 't';

    // Sub QoS for Topic
    sub_buf[8] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 9, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 5;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '#';
    sub_buf[7] = '/';
    sub_buf[8] = 't';

    // Sub QoS for Topic
    sub_buf[9] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 10, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with invalid single-level wildcard (+) topics.
 * Should fail with ISMRC_BadTopic.
 */
void mqttRx_fail_subscribe_wildcardtopicplus(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 255;
    sub_buf[1] = 255;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 2;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '+';

    // Sub QoS for Topic
    sub_buf[6] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 7, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 2;

    // Sub Topic
    sub_buf[4] = '+';
    sub_buf[5] = 't';

    // Sub QoS for Topic
    sub_buf[6] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 7, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 4;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '+';
    sub_buf[7] = 't';

    // Sub QoS for Topic
    sub_buf[8] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 9, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 6;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '+';
    sub_buf[7] = '/';
    sub_buf[8] = '+';
    sub_buf[9] = 't';

    // Sub QoS for Topic
    sub_buf[10] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 11, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 6;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '+';
    sub_buf[7] = '/';
    sub_buf[8] = 't';
    sub_buf[9] = '+';

    // Sub QoS for Topic
    sub_buf[10] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 11, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);
    my_freeTransport(transport);
}

/* Test MT_SUBSCRIBE with topics containing invalid mixes of multilevel (#)
 * single-level wildcard (+) topics.
 * Should faile with ISMRC_BadTopic.
 */
void mqttRx_fail_subscribe_wildcardtopicmixed(void) {
    int rc;
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 255;
    sub_buf[1] = 255;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 2;

    // Sub Topic
    sub_buf[4] = '+';
    sub_buf[5] = '#';

    // Sub QoS for Topic
    sub_buf[6] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 7, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 5;

    // Sub Topic
    sub_buf[4] = '+';
    sub_buf[5] = '/';
    sub_buf[6] = '#';
    sub_buf[7] = '/';
    sub_buf[8] = '+';

    // Sub QoS for Topic
    sub_buf[9] = 0;

    CU_ASSERT(mqttRx_test(1, transport, conn_buf, sub_buf, 10, 0x82, /* MT_SUBSCRIBE */ 1) == ISMRC_BadTopic);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 7;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '+';
    sub_buf[7] = '/';
    sub_buf[8] = '#';
    sub_buf[9] = '/';
    sub_buf[10] = 't';

    // Sub QoS for Topic
    sub_buf[11] = 0;

    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, sub_buf, 11, 0x82, /* MT_SUBSCRIBE */ 1)) == ISMRC_BadClientData);
    // printf("Line %d: rc=%d\n", __LINE__, rc);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 6;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '+';
    sub_buf[7] = '/';
    sub_buf[8] = 't';
    sub_buf[9] = '#';

    // Sub QoS for Topic
    sub_buf[10] = 0;

    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, sub_buf, 11, 0x82, /* MT_SUBSCRIBE */ 1)) == ISMRC_BadTopic);
    // printf("Line %d: rc=%d\n", __LINE__, rc);
    CU_ASSERT(g_cmd_result < 1);

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 9;

    // Sub Topic
    sub_buf[4] = 't';
    sub_buf[5] = '/';
    sub_buf[6] = '+';
    sub_buf[7] = '/';
    sub_buf[8] = '+';
    sub_buf[9] = '#';

    // Sub QoS for Topic
    sub_buf[10] = 0;

    CU_ASSERT((rc = mqttRx_test(1, transport, conn_buf, sub_buf, 11, 0x82, /* MT_SUBSCRIBE */ 1)) == ISMRC_BadClientData);
    // printf("Line %d: rc=%d\n", __LINE__, rc);
    CU_ASSERT(g_cmd_result < 1);
    my_freeTransport(transport);
}

/* Test MT_PINGREQ with data.
 * Should fail with ISMRC_BadLength.
 */
void mqttRx_fail_ping(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char cmd_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    cmd_buf[0] = 1;

    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, cmd_buf, 1, 0xC0, /* MT_PINGREQ */ 1) == ISMRC_BadLength);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_PINGREQ.
 * Should succeed.
 */
void mqttRx_pass_ping(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, "", 0, 0xC0, /* MT_PINGREQ */ 1) == 0);
    CU_ASSERT(g_cmd_result > 0);
    my_freeTransport(transport);
}

/* Test MT_UNSUBSCRIBE with QoS 0 fixed header.
 * Should fail with ISMRC_InvalidValue.
 */
void mqttRx_fail_unsubscribebadqos(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 255;
    sub_buf[1] = 255;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 't';

    // Sub QoS for Topic
    sub_buf[5] = 0;

    g_entered_close_cb = 0;
    CU_ASSERT(ism_mqtt_connection(transport) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, conn_buf, 15, 0x10 /* MT_CONNECT */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, sub_buf, 6, 0x82 /* MT_SUBSCRIBE */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, NULL, 0, 0xA0 /* MT_UNSUBSRIBE */ ) == ISMRC_InvalidValue);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_UNSUBSCRIBE with QoS 1 in fixed header but no other data.
 * Should fail with ISMRC_BadLength.
 */
void mqttRx_fail_unsubscribenodata(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    char unsub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 't';

    // Sub QoS for Topic
    sub_buf[5] = 0;

    g_entered_close_cb = 0;
    CU_ASSERT(ism_mqtt_connection(transport) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, conn_buf, 15, 0x10 /* MT_CONNECT */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, sub_buf, 6, 0x82 /* MT_SUBSCRIBE */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, unsub_buf, 0, 0xA2 /* MT_UNSUBSRIBE */ ) == ISMRC_BadLength);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_UNSUBSCRIBE with QoS 1 in fixed header but invalid message ID.
 * Should fail with ISMRC_BadTopic.
 */
void mqttRx_fail_unsubscribebadid(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    char unsub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 't';

    // Sub QoS for Topic
    sub_buf[5] = 0;

    memcpy(unsub_buf,  sub_buf, 5);

    /* Change message ID to invalid value of 0 for unsubscribe */
    unsub_buf[0] = 0;
    unsub_buf[1] = 0;

    g_entered_close_cb = 0;
    CU_ASSERT(ism_mqtt_connection(transport) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, conn_buf, 15, 0x10 /* MT_CONNECT */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, sub_buf, 6, 0x82 /* MT_SUBSCRIBE */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, unsub_buf, 5, 0xA2 /* MT_UNSUBSRIBE */ ) == ISMRC_InvalidID);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_UNSUBSCRIBE with QoS 1 in fixed header, valid message ID but no topic.
 * Should fail with ISMRC_InvalidValue.
 */
void mqttRx_fail_unsubscribenotopic(void) {
    int  rc;
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    char unsub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 't';

    // Sub QoS for Topic
    sub_buf[5] = 0;

    memcpy(unsub_buf,  sub_buf, 5);

    /* Change topic size to 0 for unsubscribe */
    unsub_buf[2] = 0;
    unsub_buf[3] = 0;

    g_entered_close_cb = 0;
    CU_ASSERT(ism_mqtt_connection(transport) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, conn_buf, 15, 0x10 /* MT_CONNECT */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, sub_buf, 6, 0x82 /* MT_SUBSCRIBE */ ) == 0);
    CU_ASSERT((rc = ism_mqtt_receive(transport, unsub_buf, 4, 0xA2 /* MT_UNSUBSRIBE */ )) == ISMRC_ProtocolError);
    // printf("Line %d: rc=%d\n", __LINE__, rc);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}

/* Test MT_UNSUBSCRIBE with QoS 1 in fixed header and required data.
 * But length specified for topic will be 255 and will exceed length specified
 * for buffer.
 * Should fail with ISMRC_BadTopic.
 */
void mqttRx_fail_unsubscribe_topictoolong(void) {
    int  rc;
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    char unsub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 't';

    // Sub QoS for Topic
    sub_buf[5] = 0;

    memcpy(unsub_buf,  sub_buf, 5);
    unsub_buf[2] = 0;
    unsub_buf[3] = 255;

    g_entered_close_cb = 0;
    CU_ASSERT(ism_mqtt_connection(transport) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, conn_buf, 15, 0x10 /* MT_CONNECT */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, sub_buf, 6, 0x82 /* MT_SUBSCRIBE */ ) == 0);
    CU_ASSERT((rc = ism_mqtt_receive(transport, unsub_buf, 5, 0xA2 /* MT_UNSUBSRIBE */ )) == ISMRC_BadClientData);
    // printf("Line %d: rc=%d\n", __LINE__, rc);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}


/* Test MT_UNSUBSCRIBE with QoS 1 in fixed header and required, valid data.
 * Should succeed.
 */
void mqttRx_pass_unsubscribe(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    char unsub_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 't';

    // Sub QoS for Topic
    sub_buf[5] = 0;

    memcpy(unsub_buf,  sub_buf, 5);

    CU_ASSERT(ism_mqtt_connection(transport) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, conn_buf, 15, 0x10 /* MT_CONNECT */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, sub_buf, 6, 0x82 /* MT_SUBSCRIBE */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, unsub_buf, 5, 0xA2 /* MT_UNSUBSRIBE */ ) == 0);
    my_freeTransport(transport);
}

/* Test MT_UNSUBSCRIBE with QoS 1 in fixed header and other required data.
 * Topic length will be specified as 32767 (the maximum allowed).
 * Should succeed.
 */
void mqttRx_pass_unsubscribe_maxtopiclen(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    char unsub_buf[70000] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 't';

    // Sub QoS for Topic
    sub_buf[5] = 0;

    memcpy(unsub_buf,  sub_buf, 5);
    // Unsub Topic length
    unsub_buf[2] = 127;
    unsub_buf[3] = 255;

    // Unsub Topic
    memset(&unsub_buf[4], 't', 32767);

    CU_ASSERT(ism_mqtt_connection(transport) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, conn_buf, 15, 0x10 /* MT_CONNECT */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, sub_buf, 6, 0x82 /* MT_SUBSCRIBE */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, unsub_buf, 32771, 0xA2 /* MT_UNSUBSRIBE */ ) == 0);
    my_freeTransport(transport);
}

/* Test MT_UNSUBSCRIBE with QoS 1 in fixed header and other required data.
 * Topic length will be specified as 32768 (just over the the maximum, 32767,
 * allowed).
 * Should fail with ISMRC_BadTopic.
 */
void mqttRx_fail_unsubscribe_invalidtopiclen(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    char sub_buf[100] = {0};
    char unsub_buf[70000] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);

    // Message ID
    sub_buf[0] = 0;
    sub_buf[1] = 1;

    // Sub Topic length
    sub_buf[2] = 0;
    sub_buf[3] = 1;

    // Sub Topic
    sub_buf[4] = 't';

    // Sub QoS for Topic
    sub_buf[5] = 0;

    memcpy(unsub_buf,  sub_buf, 5);
    // Unsub Topic length
    unsub_buf[2] = 128;
    unsub_buf[3] = 0;

    // Unsub Topic
    memset(&unsub_buf[4], 't', 32768);


    g_entered_close_cb = 0;
    CU_ASSERT(ism_mqtt_connection(transport) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, conn_buf, 15, 0x10 /* MT_CONNECT */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, sub_buf, 6, 0x82 /* MT_SUBSCRIBE */ ) == 0);
    CU_ASSERT(ism_mqtt_receive(transport, unsub_buf, 32772, 0xA2 /* MT_UNSUBSRIBE */ ) == 0);
    CU_ASSERT(g_entered_close_cb == 0);
    my_freeTransport(transport);
}

/* Test MT_DISCONNECT.
 * Should succeed.
 */
void mqttRx_pass_disconnect(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t)+1024);
    const char * protocol = "mqtt-tcp";
    char conn_buf[100] = {0};
    setupTransport(transport, protocol,NULL,__LINE__);
    setupConnection(conn_buf);
    g_entered_close_cb = 0;
    CU_ASSERT(mqttRx_test(1, transport, conn_buf, "", 0, 0xE0, /* MT_DISCONNECT */ 1) == 0);
    CU_ASSERT(g_entered_close_cb == 1);
    my_freeTransport(transport);
}


void setupTransport(ism_transport_t *transport, const char * protocol, struct ism_protobj_t * pobj, int line) {
    //printf("transport line=%d\n", line);
    transport->protocol = protocol;
    transport->close = mclose;
    transport->closed = mclosed;
    transport->send = msend;
    transport->pobj = pobj;
    pthread_spin_init(&transport->lock, 0);
    transport->serverport = line;
    transport->state = ISM_TRANST_Open;
    transport->trclevel = ism_defaultTrace;
    if (pobj){
    	pobj->transport = transport;
    	pobj->startState = 0;
    	fprintf(stderr,"setupTransport called from line %d, trans = %p\n",line,transport);
    } else {
        transport->suballoc.size = 1024;
    }
    ism_listener_t * listener = calloc(1, sizeof(ism_listener_t) + sizeof(ism_endstat_t));
    listener->stats = (ism_endstat_t *)(listener+1);
    transport->listener = listener;
    transport->listener->name = "DemoEndpoint";
    transport->listener->port = 16102;
    if (transport->security_context==NULL) {
    	ism_security_create_context(ismSEC_POLICY_CONNECTION,
    									transport,
    									&transport->security_context);
    }
}

void setupConnection4(char conn_buf[]) {
    memcpy(conn_buf, "\0\4MQTT\4\0\0\0\0\1c", 13);
}

void setupConnection(char conn_buf[]) {
    // Protocol length
    conn_buf[0] = 0;
    conn_buf[1] = 6;

    // Protocol name
    conn_buf[2] = 'M';
    conn_buf[3] = 'Q';
    conn_buf[4] = 'I';
    conn_buf[5] = 's';
    conn_buf[6] = 'd';
    conn_buf[7] = 'p';

    // Version
    conn_buf[8] = 3;

    // Flags
    conn_buf[9] = 0;

    // Keepalive
    conn_buf[10] = 0;
    conn_buf[11] = 0;

    // Client ID length
    conn_buf[12] = 0;
    conn_buf[13] = 1;

    // Client ID
    conn_buf[14] = 'c';
}

void setupConnectionWithFlags(char conn_buf[], int flags) {
   setupConnection(conn_buf);
   conn_buf[9] = flags;
}

void setupConnectionWithWillMsg(char conn_buf[], int qos) {
   setupConnection(conn_buf);

    // Flags
    conn_buf[9] = 0x04 | (qos<<3);

    // Will topic lentgh
    conn_buf[15] = 0;
    conn_buf[16] = 2;

    // Will topic
    conn_buf[17] = 'w';
    conn_buf[18] = 't';

    // Will message lentgh
    conn_buf[19] = 0;
    conn_buf[20] = 0;

}

/**
 * The send method for the transport object used for tests in
 * this source file.
 */
int jsend(ism_transport_t * trans, char * buf, int len, int kind, int flags) {
    char * tbuf = alloca(len+1);
    memcpy(tbuf, buf, len);

    g_entered_send_cb = 1;

    tbuf[len] = 0;
    if(g_verbose)
        printf("send: %s\n", tbuf);
    return 0;
}

/**
 * The close method for the transport object used for tests in
 * this source file.
 */
int jclose(ism_transport_t * trans, int rc, int clean, const char * reason) {
    g_entered_close_cb = 1;
    if(g_verbose)
        printf("Connection closing: rc=%d clean=%d reason=%s\n", rc, clean, reason);
    trans->closing(trans, rc, clean, reason);
    return 0;
}

/**
 * The closed method for the transport object used for tests in
 * this source file.
 */
int jclosed(ism_transport_t * trans) {
    g_entered_closed_cb = 1;
    if(g_verbose)
        printf("Connection closed\n");
    return 0;
}

int jresume(ism_transport_t * transport, void * userdata) {
    g_entered_startMessageDelivery = 1;
    if(g_verbose)
        printf("Message delivery started\n");

    return 0;
}

void testmqttconnect(void) {
    /* Test pobj closed causes failure return 1 */
    mqttRx_fail_pobjclosed();

    /* Test invalid command value as first command.
     * Should cause failure return ISMRC_ConnectFirst.
     */
    mqttRx_fail_invalidcommandfirst();

    /* Test MT_PUBLISH as first command.
     * Should cause failure return ISMRC_ConnectFirst.
     */
    mqttRx_fail_mustconnectfirst();

    /* Test MT_CONNECT with buflen of 0.
     * Should fail with ISMRC_BadLength.
     */
    mqttRx_fail_connectnodata();

    /* Test MT_CONNECT with buflen=3 and buf is values of 0 and 3.
     * Should fail with CRC_InvalidVersion because protocol name is required.
     */
    mqttRx_fail_connectnoprotname();

    /* Test MT_CONNECT with buflen=4 and buf is values of 1, "p" and 3.
     * Should fail with CRC_InvalidVersion because protocol name is required and
     * must be six characters long.
     */
    mqttRx_fail_connectbadprotname1();

    /* Test MT_CONNECT with buflen=9 and buf is values of 6, "mQISdp" and 3.
     * Should fail with CRC_InvalidVersion because protocol name is required and
     * and the value is case sensitive.  (Correct name is "MQIsdp".)
     */
    mqttRx_fail_connectbadprotname2();

    /* Test MT_CONNECT with buflen=8 (to exclude version value) and buf is values of 6
     * and "MQIsdp (but no value for version).
     * Should fail with CRC_InvalidVersion because version value 3 is required.
     */
    mqttRx_fail_connectnoversion();

    /* Test MT_CONNECT with buflen=9 and version byte set to 2.
     * Should fail with CRC_InvalidVersion.
     */
    mqttRx_fail_connectwrongversion();

    /* Test MT_CONNECT with buflen=15, version byte set to 3 and will flag set.
     * Should fail because if will flag is set, then will topic and will
     * message must also be provided.  Fails with ISMRC_BadLength due to missing
     * fields.
     */
    mqttRx_fail_connectbadwillmissingdata();

    /* Test MT_CONNECT with buflen=21, version byte set to 3, will flag set,
     * will QoS set to illegal value of 3 and will topic specified.
     * Should fail because will QoS of 3 is not allowed to be used.
     * Fails with ISMRC_InvalidQoS.
     */
    mqttRx_fail_connectbadwillqos1();

    /* Test MT_CONNECT with buflen=21, version byte set to 3, will flag set,
     * will QoS set to 1, will topic length set to 2, will topic "wt",
     * will message length set to 65535 with no will message.
     * Should fail because will message length will be longer than buflen.
     */
    mqttRx_fail_connect_willmessagetoolong();

    /* Test MT_CONNECT with buflen=21, version byte set to 3, will flag set,
     * will QoS set to 1, will topic length set to 2, will topic "wt",
     * will message length set to 0 with no will message.
     * Should pass.
     */
    mqttRx_pass_connectwillnomessage();

    /* Test MT_CONNECT with buflen=65556, version byte set to 3, will flag set,
     * will QoS set to 1, will topic length set to 2, will topic "wt",
     * will message length set to 65535 with corresponding will message.
     * Should pass.
     */
    mqttRx_pass_connectwillmaxmessagelen();


    /* Test MT_CONNECT with buflen=23, version set to 3, keep alive set, client ID set,
     * will topic and will message set.
     * Should succeed.
     */
    mqttRx_pass_connectkpalwltpwlmsg();

    /* Test MT_CONNECT with buflen=15, version set to 3 and client ID set.
     * Password flag set but unsername flag not set and no password provided.
     * Should fail with ISMRC_UsernameRequired.
     */
    mqttRx_fail_connectmissingpw();

    /* Test MT_CONNECT with buflen=17, version set to 3 and client ID set.
     * Password flag set and password length of 0 provided but username flag not set.
     * Should fail with ISMRC_UsernameRequired.
     */
    mqttRx_fail_connectpwonly();

    /* Test MT_CONNECT with buflen=19, version set to 3 and client ID set.
     * Username and password flags set with username and password lengths of 0 provided.
     * Should succeed.
     */
    mqttRx_pass_connectunamewithpw();

    /* Test MT_CONNECT with buflen=15, version set to 3 and client ID set.
     * Username flag set but no username provided.
     * Should fail with ISMRC_BadLength.
     */
    mqttRx_pass_connectmissinguname();

    /* Test MT_CONNECT with buflen=17, version set to 3 and client ID set.
     * Username flag set but and username length of 0 provided.
     * Should succeed.
     */
    mqttRx_pass_connectwithuname();

    /* Test MT_CONNECT with buflen=37, version set to 3, client ID set with clientID
     * length of 23 (max length allowed).
     * Should succeed.
     */
    mqttRx_pass_connect_clidmax();

    /* Test MT_CONNECT with buflen=14, version set to 3, client ID set with clientID
     * length of 0 (less than min length allowed).
     * Should fail with CRC_BadIdentifier.
     */
    mqttRx_fail_connect_clidtooshort();

    /* Test MT_CONNECT with buflen=15, version set to 3 and client ID set.
     * Client ID length will be specified to be 23 even though the buflen
     * value will be set to 15.
     * Should fail with ISMRC_BadLength.
     */
    mqttRx_fail_connect_clidfieldtoolong();

    /* Test MT_CONNECT with buflen=15, version set to 3 and client ID set.
     * Should succeed.  This is the minimum data required for a connection.
     */
    mqttRx_pass_connect();

    /* Test MT_CONNECT with different settings for AllowDurable */
    mqttRx_connect_allowdurable();

    /* Test MT_CONNECT with will message and different settings for
     * AllowPersistentMessages.
     */
    mqttRx_connect_willmsg_allowpersistent();
}

void testmqttpublish(void) {
    /*
     * Test MT_CONNECT with keepalive specified. Ensure that the connection is
     * closed after 2xtimeout without action.
     */
   //  mqttRx_pass_connect_keepalive();

    /* Test invalid command after connect.
     * Should fail with ISMRC_InvalidCommand.
     */
    mqttRx_fail_invalidcommandafterconnect();
    /* Test MT_PUBLISH with buflen of 0.
     * Should fail with ISMRC_BadLength.
     */
    mqttRx_fail_publishnodata();

    /* Test MT_PUBLISH with buflen of 2.
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_publishnulltopic();

    /* Test MT_PUBLISH with buflen of 5, non-null topic, invalid message ID.
     * To be sent with QoS set to 1 in the fixed header.
     * Should fail with ISMRC_InvalidID.
     */
    mqttRx_fail_publishbadid();

    /* Test MT_PUBLISH with buflen of 3, non-null topic, no message ID.
     * To be sent with QoS set to 1 in the fixed header.
     * Should fail with ISMRC_InvalidID.
     */
    mqttRx_fail_publishqos1nomsgid();

    /* Test MT_PUBLISH with buflen of 5 and wildcard (with #) topic.
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_publish_wildcardtopic1();

    /* Test MT_PUBLISH with buflen of 5 and wildcard (with +) topic.
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_publish_wildcardtopic2();

    /* Test MT_PUBLISH with buflen of 3 and non-null topic.
     * Invalid QoS of 3.
     * Should fail with ISMRC_InvalidQoS.
     */
    mqttRx_fail_publish_badqos();

    /* Test MT_PUBLISH with buflen of 3 but topic length set to 255.
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_publish_topictoolong();

    /* Test MT_PUBLISH with buflen of 3 and non-null topic.
     * Should succeed. This is the smallest possible publish message.
     */
    mqttRx_pass_publish();

    /* Test MT_PUBACK with buflen of 2 and a message ID that does not exist.
     * Should pass (invalid message ID should just be ignored).
     */
    mqttRx_pass_puback_msgdne();

    /* Test MT_PUBACK with buflen of 2 and a valid message ID.
     * Should pass.
     */
    mqttRx_pass_puback();

    /* Test MT_PUBREC with buflen of 2 and a message ID that does not exist.
     * Should pass (invalid message ID should just be ignored).
     */
    mqttRx_pass_pubrec_msgdne();

    /* Test MT_PUBREC with buflen of 2 and a valid message ID.
     * Should pass.
     */
    mqttRx_pass_pubrec();

    /* Test MT_PUBREL with buflen of 2 and a message ID that does not exist.
     * Should pass (invalid message ID should just be ignored).
     */
    mqttRx_pass_pubrel_msgdne();

    /* Test MT_PUBREL with buflen of 2 and a valid message ID.
     * Should pass.
     */
    mqttRx_pass_pubrel();

    /* Test MT_PUBCOMP with buflen of 2 and a message ID that does not exist.
     * Should pass (invalid message ID should just be ignored).
     */
    mqttRx_pass_pubcomp_msgdne();

    /* Test MT_PUBCOMP with buflen of 2 and a valid message ID.
     * Should pass.
     */
    mqttRx_pass_pubcomp();

    /* Test MT_PUBLISH with buflen of 32770 (topic length will be 32768 - just
     * over the length limit of 32767).
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_publish_invalidtopiclen();

    /* Test MT_PUBLISH with buflen of 32769 (topic length will be 32767).
     * Should succeed.
     */
    mqttRx_pass_publish_maxtopiclen();

    /*
     * Test for publish using $SharedSubscription
     */
    mqttRx_publish_shrdsub();

    /*
     * Test for publish with dynamically created producers
     */
    mqttRx_publish_dynprod();

    /* Test MT_PUBLISH with different settings for AllowPersistentMessages */
    mqttRx_publish_allowpersistentmsgs();

}

void testmqttsubscribe(void) {
    /* Test MT_SUBSCRIBE with QoS 0 in fixed header.
     * Should fail with ISMRC_InvalidValue.
     */
    mqttRx_fail_subscribebadqos();

    /* Test MT_SUBSCRIBE with QoS 1 in fixed header but no other data.
     * Should fail with ISMRC_BadLength.
     */
    mqttRx_fail_subscribenodata();

    /* Test MT_SUBSCRIBE with bad message ID 0, topic and QoS specifided.
     * Should fail with ISMRC_InvalidID.
     */
    mqttRx_fail_subscribebadid1();

    /* Test MT_SUBSCRIBE with message ID, topic and QoS specifided in fixed header.
     * Requested QoS set to invalid value 3.
     * Should fail with ISMRC_InvalidQoS.
     */
    mqttRx_fail_subscribe_badrequestedqos();

    /* Test MT_SUBSCRIBE with message ID, topic and QoS specifided.
     * Topic length will be set to 255 but buffer length will only be 6.
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_subscribe_topictoolong();

    /* Test MT_SUBSCRIBE with message ID, topic and QoS specifided.
     * Should succeed. This is the smallest possible subscribe message.
     */
    mqttRx_pass_subscribe();

    /* Test MT_SUBSCRIBE with valid multilevel wildcard (#) topics.
     * Should succeed.
     */
    mqttRx_pass_subscribe_wildcardtopicpound();

    /* Test MT_SUBSCRIBE with valid single-level wildcard (+) topics.
     * Should succeed.
     */
    mqttRx_pass_subscribe_wildcardtopicplus();

    /* Test MT_SUBSCRIBE with topics containing valid mixes of multilevel (#)
     * single-level wildcard (+) topics.
     * Should succeed.
     */
    mqttRx_pass_subscribe_wildcardtopicmixed();

    /* Test MT_SUBSCRIBE with invalid multilevel wildcard (#) topics.
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_subscribe_wildcardtopicpound();

    /* Test MT_SUBSCRIBE with invalid single-level wildcard (+) topics.
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_subscribe_wildcardtopicplus();

    /* Test MT_SUBSCRIBE with topics containing invalid mixes of multilevel (#)
     * single-level wildcard (+) topics.
     * Should faile with ISMRC_BadTopic.
     */
    mqttRx_fail_subscribe_wildcardtopicmixed();

    /* Test MT_SUBSCRIBE with message ID, topic and QoS specifided.
     * Topic length will be specified as 32768 (just over the the maxixum
     * of 32767 allowed).
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_subscribe_invalidtopiclen();

    /* Test MT_SUBSCRIBE with message ID, topic and QoS specifided.
     * Topic length will be specified as 32767 (the maxixum allowed).
     * Should succeed.
     */
    mqttRx_pass_subscribe_maxtopiclen();

    mqttRx_pass_subscribeidmax();

    /*
     * Test for subscribe to a system topic
     */
    mqttRx_subscribe_sys();

    /*
     * Test for subscribe to multiple topics
     */
    mqttRx_subscribe_multi();

    /*
     * Test for subscription using $SharedSubscription
     */
    mqttRx_subscribe_shrdsub();

    /*
     * Test for multi-subscription using $SharedSubscription
     */
    mqttRx_subscribe_multishrdsub();

    /* Test MT_PINGREQ with data.
     * Should fail with ISMRC_BadLength.
     */
    mqttRx_fail_ping();

    /* Test MT_PINGREQ.
     * Should succeed.
     */
    mqttRx_pass_ping();

}

void testmqttunsubscribe(void) {
    /* Test MT_UNSUBSCRIBE with QoS 0 in fixed header.
     * Should fail with ISMRC_InvalidValue.
     */
    mqttRx_fail_unsubscribebadqos();

    /* Test MT_UNSUBSCRIBE with QoS 1 in fixed header but no other data.
     * Should fail with ISMRC_BadLength.
     */
    mqttRx_fail_unsubscribenodata();

    /* Test MT_UNSUBSCRIBE with QoS 1 in fixed header but invalid message ID.
     * Should fail with ISMRC_InvalidID.
     */
    mqttRx_fail_unsubscribebadid();

    /* Test MT_UNSUBSCRIBE with QoS 1 in fixed header, valid message ID but no topic.
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_unsubscribenotopic();

    /* Test MT_UNSUBSCRIBE with QoS 1 in fixed header and required data.
     * But length specified for topic will be 255 and will exceed length specified
     * for buffer.
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_unsubscribe_topictoolong();

    /* Test MT_UNSUBSCRIBE with QoS 1 in fixed header and required, valid data.
     * Should succeed.
     */
    mqttRx_pass_unsubscribe();

    /* Test MT_UNSUBSCRIBE with QoS 1 in fixed header and other required data.
     * Topic length will be specified as 32767 (the maximum allowed).
     * Should succeed.
     */
    mqttRx_pass_unsubscribe_maxtopiclen();

    /* Test MT_UNSUBSCRIBE with QoS 1 in fixed header and other required data.
     * Topic length will be specified as 32768 (just over the the maximum, 32767,
     * allowed).
     * Should fail with ISMRC_BadTopic.
     */
    mqttRx_fail_unsubscribe_invalidtopiclen();

    /* Test MT_DISCONNECT.
     * Should succeed.
     */
    mqttRx_pass_disconnect();
}

void testPutString(void) {
    const char * serverUID = "ThisIsTheUIDforTheServer";
    concat_alloc_t xprops= {0};
    xprops.len = strlen(serverUID) + 30;
    xprops.buf = alloca(xprops.len);
    ism_protocol_putNameIndex(&xprops, ID_ServerTime);
    ism_protocol_putLongValue(&xprops, ism_common_currentTimeNanos());
    ism_protocol_putNameIndex(&xprops, ID_OriginServer);
    ism_protocol_putStringValue(&xprops, serverUID);
    CU_ASSERT(!xprops.inheap);
}

#include "test_mqttv5.c"

