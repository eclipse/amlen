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
 * File: test_iotmonitor.c
 * Component: server
 * SubComponent: server_protocol
 *
 */


extern int g_verbose;

#include <ismutil.h>
#include <ismjson.h>
#include <transport.h>

int ism_iot_jsonMessage(concat_alloc_t * buf,
                        int which,
                        ism_transport_t * transport,
                        ism_json_parse_t *parseObj,
                        ism_ts_t *ts,
                        uint64_t msgTime,
                        int rc,
                        const char * reason);

#include <iotmonitor.h>
#include <iotmonitor.c>
#include <test_iotmonitor.h>
#include <fakeengine.h>

/**
 * Test functions
 */

CU_TestInfo ISM_Protocol_CUnit_IoTMonitorBasic[] = {
    {"IoTMonitorMsg      ",  testiotmonitormsg },
    {"IoTMonitorMsgAPIs  ",  testiotmonitorAPIs },
    {"IoTMonitorReconcile",  testiotmonitorReconcile},
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Protocol_CUnit_IoTMonitorFull[] = {
    {"IoTMonitorMsg      ",  testiotmonitormsg },
    {"IoTMonitorMsgAPIs  ",  testiotmonitorAPIs },
    {"IoTMonitorReconcile",  testiotmonitorReconcile},
    CU_TEST_INFO_NULL
};

/**
 * The close method for the transport object used for tests in
 * this source file.
 */
static int mclose(ism_transport_t * trans, int rc, int clean, const char * reason) {
    g_entered_close_cb += 1;
    trans->closing(trans, rc, clean, reason);
    return 0;
}

/**
 * The closed method for the transport object used for tests in * this source file.
 */
static int mclosed(ism_transport_t * trans) {
    return 0;
}

/**
 * The send method for the transport object used for tests in
 * this source file.
 */
static int msend(ism_transport_t * trans, char * buf, int len, int kind, int flags) {
    char * tbuf = alloca(len+1);
    if (g_cmd) {
        /* If CONNECT is sent, CONNACK should be returned */
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

        g_cmd = NULL;
    }

    memcpy(tbuf, buf, len);
    tbuf[len] = 0;
    return 0;
}

static void setupTransportForIoTMonitor(ism_transport_t *transport, const char * protocol, struct ism_protobj_t * pobj, int line) {
    //printf("transport line=%d\n", line);
    transport->protocol = protocol;
    transport->protocol_family = "mqtt";
    transport->close = mclose;
    transport->closed = mclosed;
    transport->send = msend;
    transport->pobj = pobj;
    transport->serverport = line;
    transport->trclevel = ism_defaultTrace;
    if (pobj){
        pobj->transport = transport;
        pobj->startState = 0;
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
    transport->client_addr = "1.1.1.1";
    transport->clientID = "IoTMonitorID";
}

void testiotmonitormsg(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t));
    struct ism_protobj_t pobj = {0};

    setupTransportForIoTMonitor(transport, "mqtt-test", &pobj, __LINE__);

    char xbuf1 [4096];
    char xbuf2 [4096];
    concat_alloc_t buf1 = {0};
    concat_alloc_t buf2 = {0};

    uint64_t msgTime;
    int rc;
    const char * reason;

    ism_json_parse_t parseObj1 = {0};
    ism_json_parse_t parseObj2 = {0};
    ism_json_entry_t ents1[10], ents2[10];

    parseObj1.ent = ents1;
    parseObj1.ent_alloc = (int)(sizeof(ents1)/sizeof(ents1[0]));
    parseObj1.source = NULL;
    parseObj2.ent = ents2;
    parseObj2.ent_alloc = (int)(sizeof(ents1)/sizeof(ents1[0]));
    parseObj2.source = NULL;

    msgTime = ism_common_currentTimeNanos();
    rc = ISMRC_Error;
    reason = "Test Monitoring Messages";

    int fromType, toType, msgrc = OK;

    /* create messages of all types, both from transport object and from parsed json */
    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
    CU_ASSERT_PTR_NOT_NULL(ts);
    for(int testNo=0; testNo<7; testNo++) {
        fromType = toType = testNo;

        /* Play with the transport object and from/to types */
        switch(testNo)
        {
            case 0:
                transport->userid = "IotUserId";
                break;
            case 1:
                msgrc = ISMRC_Error;
                transport->userid = "use-token-auth";
                break;
            case 2:
                msgrc = ISMRC_AllocateError;
                transport->userid = NULL;
                transport->cert_name = "IotCertificate";
                break;
            case 3:
                msgrc = OK;
                transport->cert_name = NULL;
                break;
            case 6:
                fromType = JM_Connect;
                toType = JM_Disconnect;
                transport->pobj->monitor_opt |= 0x40;
                break;
            default:
                break;
        }

        memset(&buf1, 0, sizeof(concat_alloc_t));
        buf1.buf = xbuf1; buf1.len = sizeof(xbuf1); buf1.compact = (char)3;
        memset(&buf2, 0, sizeof(concat_alloc_t));
        buf2.buf = xbuf2; buf2.len = sizeof(xbuf2); buf2.compact = (char)3;

        /* Produce a message from the transport object */
        rc = ism_iot_jsonMessage(&buf1, fromType, transport, NULL, ts, msgTime, msgrc, reason);
        CU_ASSERT(rc == OK);

        parseObj1.source = realloc(parseObj1.source, buf1.used);
        parseObj1.src_len = buf1.used;
        parseObj1.ent_count = 0;

        memcpy(parseObj1.source, buf1.buf, buf1.used);

        rc = ism_json_parse(&parseObj1);
        CU_ASSERT_EQUAL(rc, OK);

        /* Produce a message of same type from the parsed JSON */
        rc = ism_iot_jsonMessage(&buf2, toType, NULL, &parseObj1, ts, msgTime, msgrc, reason);
        CU_ASSERT_EQUAL(rc, OK);

        parseObj2.source = realloc(parseObj2.source, buf2.used);
        parseObj2.src_len = buf2.used;
        parseObj2.ent_count = 0;

        memcpy(parseObj2.source, buf2.buf, buf2.used);

        rc = ism_json_parse(&parseObj2);
        CU_ASSERT_EQUAL(rc, OK);

        /* Compare the two messages */
        if (fromType == toType && toType != JM_Disconnect) {
            CU_ASSERT_EQUAL(buf1.used, buf2.used);
            CU_ASSERT_EQUAL(strcmp(buf1.buf, buf2.buf), 0);
            if (buf1.used != buf2.used || strcmp(buf1.buf, buf2.buf)) {
                printf("\nbuf1=%s\n", buf1.buf);
                printf("buf2=%s\n", buf2.buf);
            }
        } else {
            const char *f1, *f2;

            if (fromType == JM_Disconnect) {
                f1 = ism_json_getString(&parseObj1, MONITOR_MSG_VALUE_READBYTES);
                CU_ASSERT_PTR_NOT_NULL(f1);
                f1 = ism_json_getString(&parseObj1, MONITOR_MSG_VALUE_READMSG);
                CU_ASSERT_PTR_NOT_NULL(f1);
                f1 = ism_json_getString(&parseObj1, MONITOR_MSG_VALUE_WRITEBYTES);
                CU_ASSERT_PTR_NOT_NULL(f1);
                f1 = ism_json_getString(&parseObj1, MONITOR_MSG_VALUE_WRITEMSG);
                CU_ASSERT_PTR_NOT_NULL(f1);
            }

            f2 = ism_json_getString(&parseObj2, MONITOR_MSG_VALUE_READBYTES);
            CU_ASSERT_PTR_NULL(f2);
            f2 = ism_json_getString(&parseObj2, MONITOR_MSG_VALUE_READMSG);
            CU_ASSERT_PTR_NULL(f2);
            f2 = ism_json_getString(&parseObj2, MONITOR_MSG_VALUE_WRITEBYTES);
            CU_ASSERT_PTR_NULL(f2);
            f2 = ism_json_getString(&parseObj2, MONITOR_MSG_VALUE_WRITEMSG);
            CU_ASSERT_PTR_NULL(f2);
        }

        ism_common_freeAllocBuffer(&buf1);
        ism_common_freeAllocBuffer(&buf2);
    }

    ism_common_closeTimestamp(ts);

    if (parseObj1.free_ent) ism_common_free(ism_memory_utils_parser,parseObj1.ent);
    free(parseObj1.source);
    if (parseObj2.free_ent) ism_common_free(ism_memory_utils_parser,parseObj2.ent);
    free(parseObj2.source);

    free(transport);
}

int32_t createMessageRC = OK;
uint32_t createMessageCount = 0;
uint32_t retainedMessagesCreated = 0;
uint32_t connectMessagesCreated = 0;
uint32_t disconnectMessagesCreated = 0;
XAPI int32_t ism_engine_createMessageAPI_CB(ismMessageHeader_t *pHeader,
                                            uint8_t areaCount,
                                            ismMessageAreaType_t areaTypes[areaCount],
                                            size_t areaLengths[areaCount],
                                            void * pAreaData[areaCount],
                                            ismEngine_MessageHandle_t * phMessage) {

    int32_t rc;
    ism_json_parse_t parseObj = {0};
    ism_json_entry_t ents[10];

    __sync_add_and_fetch(&createMessageCount, 1);

    if (pHeader->Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) {
        retainedMessagesCreated++;
    }

    /* Verify the JSON is valid and determine which type of message this is */
    for(uint8_t i=0; i<areaCount; i++) {
        if (areaTypes[i] == ismMESSAGE_AREA_PAYLOAD) {
            parseObj.ent = ents;
            parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
            parseObj.source = ism_common_strdup(ISM_MEM_PROBE(ism_memory_protocol_misc,1000),pAreaData[i]);
            parseObj.src_len = strlen(pAreaData[i]);

            rc = ism_json_parse(&parseObj);
            CU_ASSERT_EQUAL(rc, OK);

            const char *testVal = ism_json_getString(&parseObj, MONITOR_MSG_VALUE_ACTION);
            CU_ASSERT_PTR_NOT_NULL(testVal); /* Must have an Action */

            /* Count certain types of message for later verification */
            if (strcmp(testVal, MONITOR_MSG_ACTION_CONNECT) == 0) {
                __sync_add_and_fetch(&connectMessagesCreated, 1);
            } else if (strcmp(testVal, MONITOR_MSG_ACTION_DISCONNECT) == 0) {
                __sync_add_and_fetch(&disconnectMessagesCreated, 1);
            }
        }
    }

    CU_ASSERT_PTR_NOT_NULL(parseObj.source);

    ism_common_free(ism_memory_protocol_misc, parseObj.source);

    phMessage = (ismEngine_MessageHandle_t *)1;

    return  createMessageRC;

}

int32_t putMessageRC = OK;
uint32_t putMessageCount = 0;
int32_t ism_engine_putMessageOnDestinationAPI_CB( ismEngine_SessionHandle_t hSession,
                                                  ismDestinationType_t destinationType,
                                                  const char *pDestinationName,
                                                  ismEngine_TransactionHandle_t hTran,
                                                  ismEngine_MessageHandle_t pMessage,
                                                  void * pContext,
                                                  size_t contextLength,
                                                  ismEngine_CompletionCallback_t cb) {

    CU_ASSERT_EQUAL(destinationType, ismDESTINATION_TOPIC);

    __sync_add_and_fetch(&putMessageCount, 1);

    return putMessageRC;
}

typedef struct ism_engine_getRetainedMessage_FakeData_t {
    const char *topic;
    uint32_t msgsToFake;
    char **payloads;
    char *props;
    size_t propsLen;
} ism_engine_getRetainedMessage_FakeData_t;

ism_engine_getRetainedMessage_FakeData_t *getRetainedFakeData = NULL;
int32_t getRetainedRC = OK;
uint32_t getRetainedCount = 0;
int32_t ism_engine_getRetainedMessageAPI_CB(ismEngine_SessionHandle_t hSession,
                                            const char *topicString,
                                            void *pMessageContext,
                                            size_t messageContextLength,
                                            ismEngine_MessageCallback_t pMessageCallbackFn,
                                            void *pContext,
                                            size_t contextLength,
                                            ismEngine_CompletionCallback_t  pCallbackFn) {

    __sync_add_and_fetch(&getRetainedCount, 1);

    if (getRetainedFakeData) {
        for(int32_t i=0; getRetainedFakeData[i].topic != NULL; i++) {
            if (strcmp(topicString, getRetainedFakeData[i].topic) == 0) {
                int32_t x = 0;
                bool keepRunning = true;

                /* Fake data */
                ismMessageHeader_t header = {0};
                uint8_t areaCount = 2;
                ismMessageAreaType_t areaTypes[] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
                size_t areaLengths[] = {getRetainedFakeData[i].propsLen, 0};
                void *areaData[] = {getRetainedFakeData[i].props, NULL};

                for(uint32_t m=0; keepRunning && m<getRetainedFakeData[i].msgsToFake; m++) {
                    if (x == 0) {
                        if (getRetainedFakeData[i].payloads[m] == NULL) {
                            x = m;
                        } else {
                            areaData[1] = getRetainedFakeData[i].payloads[m];
                        }
                    }

                    if (x != 0) {
                        areaData[1] = getRetainedFakeData[i].payloads[m%x];
                    }

                    CU_ASSERT_PTR_NOT_NULL(areaData[1]);
                    areaLengths[1] = strlen(areaData[1]);

                    keepRunning = pMessageCallbackFn(NULL,
                                                     ismENGINE_NULL_DELIVERY_HANDLE,
                                                     (ismEngine_MessageHandle_t)1,
                                                     0,
                                                     ismMESSAGE_STATE_CONSUMED,
                                                     ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                                     &header,
                                                     areaCount,
                                                     areaTypes,
                                                     areaLengths,
                                                     areaData,
                                                     pMessageContext,
                                                     NULL);
                }
            }
        }
    }

    return getRetainedRC;
}

int32_t unsetRetainedRC = OK;
uint32_t unsetRetainedCount = 0;
int32_t genericEngineAPI_Callback( const char *func,
                                   void *context, size_t contextLength,
                                   void *parm1, void *parm2, void **retValue) {

    int32_t rc = OK;

    // printf("%s: %s\n", __func__, func);

    if (strcmp(func, "ism_engine_unsetRetainedMessageOnDestination") == 0) {
        __sync_add_and_fetch(&unsetRetainedCount, 1);
        rc = unsetRetainedRC;
    } else if (strcmp(func, "ism_engine_getMessagingStatistics") == 0) {
        ismEngine_MessagingStatistics_t *pStatistics = (ismEngine_MessagingStatistics_t *)retValue;
        // Fake shutdown time as 10 seconds ago...
        pStatistics->ServerShutdownTime = (ism_time_t)(ism_common_currentTimeNanos()-10000000000UL);
    }

    return rc;
}

#define LONG_USERID_LENGTH 10240
void testiotmonitorAPIs(void) {
    ism_transport_t * transport = calloc(1,sizeof(ism_transport_t));
    struct ism_protobj_t pobj = {0};

    setupTransportForIoTMonitor(transport, "mqtt", &pobj, __LINE__);

    // Fake up a session handle
    g_monitor_session = (ismEngine_SessionHandle_t)1;
    createMessageCount = 0;
    retainedMessagesCreated = 0;
    connectMessagesCreated = 0;
    disconnectMessagesCreated = 0;
    putMessageCount = 0;
    unsetRetainedCount = 0;
    getRetainedCount = 0;
    g_genericEngineAPI_CB = genericEngineAPI_Callback;
    g_ism_engine_putMessageOnDestinationAPI_CB = ism_engine_putMessageOnDestinationAPI_CB;
    g_ism_engine_createMessageAPI_CB = ism_engine_createMessageAPI_CB;
    g_ism_engine_getRetainedMessageAPI_CB = ism_engine_getRetainedMessageAPI_CB;

    transport->pobj->monitor_topic = "test/iotmonitor";

    uint32_t expectCreateMessageCount = 0;
    uint32_t expectConnectMessageCount = 0;
    uint32_t expectDisconnectMessageCount = 0;
    uint32_t expectPutMessageCount = 0;
    uint32_t expectUnsetRetainedCount = 0;

    /* Use a very long userid to push us into using heap JSON */
    char *longUserId = malloc(LONG_USERID_LENGTH+1);
    CU_ASSERT_PTR_NOT_NULL(longUserId);
    memset(longUserId, 'U', LONG_USERID_LENGTH);
    longUserId[LONG_USERID_LENGTH] = '\0';
    transport->userid = longUserId;

    // ism_iot_connectMsg
    int32_t testNo;
    for(testNo=0; testNo<6; testNo++) {
        transport->pobj->monitor_opt = 3 | (testNo%3)<<8;
        ism_iot_connectMsg(transport);
        switch(testNo)
        {
            case 2:
                createMessageRC = ISMRC_AllocateError;
                break;
            case 3:
                createMessageRC = OK;
                putMessageRC = ISMRC_Error;
                break;
            case 5:
                putMessageRC = OK;
                break;
        }
    }

    expectCreateMessageCount += testNo;
    expectConnectMessageCount += testNo;
    expectPutMessageCount += testNo-1;

    CU_ASSERT_EQUAL(createMessageCount, expectCreateMessageCount);
    CU_ASSERT_EQUAL(connectMessagesCreated, expectConnectMessageCount);
    CU_ASSERT_EQUAL(disconnectMessagesCreated, expectDisconnectMessageCount);
    CU_ASSERT_EQUAL(putMessageCount, expectPutMessageCount);
    CU_ASSERT_EQUAL(unsetRetainedCount, expectUnsetRetainedCount);

    // ism_iot_disconnectMsg
    for(testNo=0; testNo<6; testNo++) {
        transport->pobj->monitor_opt = 3 | (testNo%3)<<8;
        ism_iot_disconnectMsg(transport, testNo/2, "Fake reason");
        switch(testNo)
        {
            case 2:
                createMessageRC = ISMRC_AllocateError;
                break;
            case 3:
                createMessageRC = OK;
                putMessageRC = ISMRC_Error;
                break;
            case 5:
                putMessageRC = OK;
                break;
        }
    }

    expectCreateMessageCount += testNo;
    expectDisconnectMessageCount += testNo;
    expectPutMessageCount += testNo-1;
    expectUnsetRetainedCount += 2;

    CU_ASSERT_EQUAL(createMessageCount, expectCreateMessageCount);
    CU_ASSERT_EQUAL(connectMessagesCreated, expectConnectMessageCount);
    CU_ASSERT_EQUAL(disconnectMessagesCreated, expectDisconnectMessageCount);
    CU_ASSERT_EQUAL(putMessageCount, expectPutMessageCount);
    CU_ASSERT_EQUAL(unsetRetainedCount, expectUnsetRetainedCount);

    // Force an unset to fail
    unsetRetainedRC = ISMRC_AllocateError;
    transport->pobj->monitor_opt = 3 | (1<<8);
    ism_iot_disconnectMsg(transport, testNo/2, "Fake reason");
    unsetRetainedRC = OK;

    expectCreateMessageCount++;
    expectPutMessageCount++;
    expectUnsetRetainedCount++;

    CU_ASSERT_EQUAL(createMessageCount, expectCreateMessageCount);
    CU_ASSERT_EQUAL(putMessageCount, expectPutMessageCount);
    CU_ASSERT_EQUAL(unsetRetainedCount, expectUnsetRetainedCount);

    // ism_iot_failedMsg
    ism_iot_failedMsg(transport, ISMRC_ProtocolMismatch, "Protocol mismatch");

    expectCreateMessageCount++;
    expectPutMessageCount++;

    CU_ASSERT_EQUAL(createMessageCount, expectCreateMessageCount);
    CU_ASSERT_EQUAL(putMessageCount, expectPutMessageCount);
    CU_ASSERT_EQUAL(unsetRetainedCount, expectUnsetRetainedCount);

    g_genericEngineAPI_CB = NULL;

    free(longUserId);
    transport->userid = NULL;
    free(transport);
}

#define GET_RETAINED_TOPIC1 "iot-2/+/type/#"
#define GRT1_PAYLOAD1 "{\"Action\":\"Connect\",\"ClientID\":\"c:testorg:ABC\",\"Time\":\"2017-02-15T12:23:19.959Z\", \"ClientAddr\":\"127.0.0.1\",\"Port\":8883,\"Secure\":true,\"Protocol\":\"mqtt0\",\"User\":\"use-token-auth\",\"Durable\":true,\"NewSession\":true }"
#define GRT1_PAYLOAD2 "{\"Action\":\"Connect\",\"ClientID\":\"d:testorg:911\",\"Time\":\"2017-02-15T12:23:19.959Z\", \"ClientAddr\":\"127.0.0.1\",\"Port\":8883,\"Secure\":true,\"Protocol\":\"mqtt0\",\"User\":\"use-token-auth\",\"Durable\":true,\"NewSession\":true }"
#define GRT1_PAYLOAD3 "{\"Action\":\"Disconnect\",\"ClientID\":\"c:testorg:123\",\"Time\":\"2017-02-15T12:23:19.959Z\",\"ClientAddr\":\"127.0.0.1\",\"Port\":8883,\"Secure\":true,\"Protocol\":\"mqtt0\",\"User\":\"use-token-auth\",\"ReadBytes\":0,\"ReadMsg\":0,\"WriteBytes\":0,\"WriteMsg\":0,\"ConnectTime\":\"2017-02-15T12:21:10.101Z\"}"
#define GRT1_PAYLOAD4 "{\"Action\":\"Disconnect\",\"ClientID\":\"d:testorg:XYZ\",\"Time\":\"2017-02-15T12:23:19.959Z\",\"ClientAddr\":\"127.0.0.1\",\"Port\":8883,\"Secure\":true,\"Protocol\":\"mqtt0\",\"User\":\"use-token-auth\",\"ReadBytes\":0,\"ReadMsg\":0,\"WriteBytes\":0,\"WriteMsg\":0,\"ConnectTime\":\"2017-02-15T12:21:10.101Z\"}"
#define GRT1_PAYLOAD5 "{\"Action\":\"Unknown\",\"ClientID\":\"c:testorg:678\",\"Time\":\"2017-02-15T12:23:19.959Z\",\"ClientAddr\":\"127.0.0.1\",\"Port\":8883,\"Secure\":true,\"Protocol\":\"mqtt0\",\"User\":\"use-token-auth\"}"
#define GET_RETAINED_RULE1  GET_RETAINED_TOPIC1 ",c:*,0,0,*,2,45"

#define GET_RETAINED_TOPIC2 "iot-2/+/app/#"
#define GRT2_PAYLOAD1 "{\"Action\":\"Connect\",\"ClientID\":\"a:testorg:111\",\"Time\":\"2017-02-15T12:23:19.959Z\", \"ClientAddr\":\"127.0.0.1\",\"Port\":8883,\"Secure\":true,\"Protocol\":\"mqtt0\",\"User\":\"use-token-auth\",\"Durable\":true,\"NewSession\":true }"
#define GRT2_PAYLOAD2 "{\"Action\":\"Disconnect\",\"ClientID\":\"a:testorg:222\",\"Time\":\"2017-02-15T12:23:19.959Z\",\"ClientAddr\":\"127.0.0.1\",\"Port\":8883,\"Secure\":true,\"Protocol\":\"mqtt0\",\"User\":\"use-token-auth\",\"ReadBytes\":0,\"ReadMsg\":0,\"WriteBytes\":0,\"WriteMsg\":0,\"ConnectTime\":\"2017-02-15T12:21:10.101Z\"}"
#define GET_RETAINED_RULE2  GET_RETAINED_TOPIC2 ",*,1,45"

#define GET_RETAINED_TOPIC3 "iot-2/nomatch/app/#"
#define GET_RETAINED_RULE3  GET_RETAINED_TOPIC3 ",nomatch,1, 0x10" /* NOTE: Expiry specified in hex */
#define GRT3_PAYLOAD1 "{\"Action\":\"Connect\",\"ClientID\":\"c:testorg:ABC\",\"Time\":\"2017-02-15T12:23:19.959Z\", \"ClientAddr\":\"127.0.0.1\",\"Port\":8883,\"Secure\":true,\"Protocol\":\"mqtt0\",\"User\":\"use-token-auth\",\"Durable\":true,\"NewSession\":true }"

#define GET_RETAINED_TOPIC4 "iot-2/testOrg/type/#" /* Reuses payloads from 1 */
#define GET_RETAINED_RULE4 GET_RETAINED_TOPIC4 ", c:*,0,0,*,2,45"

void testiotmonitorReconcile(void) {

    // Fake up a session handle
    g_monitor_session = (ismEngine_SessionHandle_t)1;
    createMessageCount = 0;
    retainedMessagesCreated = 0;
    putMessageCount = 0;
    unsetRetainedCount = 0;
    getRetainedCount = 0;
    g_genericEngineAPI_CB = genericEngineAPI_Callback;
    g_ism_engine_putMessageOnDestinationAPI_CB = ism_engine_putMessageOnDestinationAPI_CB;
    g_ism_engine_createMessageAPI_CB = ism_engine_createMessageAPI_CB;
    g_ism_engine_getRetainedMessageAPI_CB = ism_engine_getRetainedMessageAPI_CB;

    // Call with no entries in config
    int rc = ism_iot_reconcile();
    CU_ASSERT_EQUAL(rc, OK);

    // Call with several single malformed entries in config
    ism_prop_t *configProps = ism_common_getConfigProperties();

    ism_field_t field;
    field.type = VT_String;

    char *malformedEntries[] = {"",
                                "just/a/topic",
                                "too/few/values, c:* ,2 ",
                                "too/few/values,c:* ,1",
                                "invalid/retainType,c:*,X,43",
                                "invalid/retainType,* ,-10,45",
                                "invalid/retainType,* , 5, 45",
                                "empty/retainType, *,,45",
                                "invalid/expiry,*,1,BLAH",
                                "empty/expiry,*,2,",
                                "one/fine/+/one/incomplete,c:*,0,0,*,2",
                                "empty/clientId,   ,1,30",
                                "empty/clientId,,2,1",
                                NULL};

    char **entry = malformedEntries;

    uint32_t expectGetRetainedCount = 0;

    while(*entry != NULL) {
        field.val.s = *entry;
        rc = ism_common_setProperty(configProps, RECONCILE_CONFIG_NAME ".0", &field);
        CU_ASSERT_EQUAL(rc, OK);

        /* None of these entries should result in any calls to getRetained */
        rc = ism_iot_reconcile();
        CU_ASSERT_EQUAL(getRetainedCount, expectGetRetainedCount);

        entry++;
    }

    field.val.s = GET_RETAINED_RULE1;
    rc = ism_common_setProperty(configProps, RECONCILE_CONFIG_NAME ".0", &field);
    CU_ASSERT_EQUAL(rc, OK);
    field.val.s = GET_RETAINED_RULE2;
    rc = ism_common_setProperty(configProps, RECONCILE_CONFIG_NAME ".1", &field);
    CU_ASSERT_EQUAL(rc, OK);
    field.val.s = GET_RETAINED_RULE3;
    rc = ism_common_setProperty(configProps, RECONCILE_CONFIG_NAME ".2", &field);
    CU_ASSERT_EQUAL(rc, OK);
    field.val.s = GET_RETAINED_RULE4;
    rc = ism_common_setProperty(configProps, RECONCILE_CONFIG_NAME ".3", &field);
    CU_ASSERT_EQUAL(rc, OK);

    /* This should result in 2 calls to getRetained (one for each rule) */
    expectGetRetainedCount += 4;
    rc = ism_iot_reconcile();
    CU_ASSERT_EQUAL(getRetainedCount, expectGetRetainedCount);

    /* Fake getRetained returning an error (again, once for each rule but carries on) */
    expectGetRetainedCount += 4;
    getRetainedRC = ISMRC_AllocateError;
    rc = ism_iot_reconcile();
    CU_ASSERT_EQUAL(getRetainedCount, expectGetRetainedCount);
    getRetainedRC = ISMRC_OK;

    /* Reconcile with some faked messages */
    char xb1[1024],xb2[1024],xb3[1024],xb4[1024];

    concat_alloc_t b1 = {xb1, sizeof xb1};
    ism_protocol_putNameIndex(&b1, ID_ServerTime);
    ism_protocol_putLongValue(&b1, ism_common_currentTimeNanos()-60000000000UL);
    ism_protocol_putNameIndex(&b1, ID_Topic);
    ism_protocol_putStringValue(&b1, "iot-2/testorg/type/devType/id/XXX/mon");
    ism_protocol_putNameIndex(&b1, ID_OriginServer);
    ism_protocol_putStringValue(&b1, ism_common_getServerUID());

    concat_alloc_t b2 = {xb2, sizeof xb2};
    ism_protocol_putNameIndex(&b2, ID_ServerTime);
    ism_protocol_putLongValue(&b2, ism_common_currentTimeNanos()-60000000000UL);
    ism_protocol_putNameIndex(&b2, ID_Topic);
    ism_protocol_putStringValue(&b2, "iot-2/testorg/type/devType/id/yyy/mon");
    ism_protocol_putNameIndex(&b2, ID_OriginServer);
    ism_protocol_putStringValue(&b2, "SomeOtherServer");

    concat_alloc_t b3 = {xb3, sizeof xb3};
    ism_protocol_putNameIndex(&b3, ID_ServerTime);
    ism_protocol_putLongValue(&b3, ism_common_currentTimeNanos()-60000000000UL);
    ism_protocol_putNameIndex(&b3, ID_Topic);
    ism_protocol_putStringValue(&b3, "iot-2/testorg/app/id/mon");
    ism_protocol_putNameIndex(&b3, ID_OriginServer);
    ism_protocol_putStringValue(&b3, ism_common_getServerUID());

    concat_alloc_t b4 = {xb4, sizeof xb4};
    ism_protocol_putNameIndex(&b4, ID_ServerTime);
    ism_protocol_putLongValue(&b4, ism_common_currentTimeNanos()-60000000000UL);
    ism_protocol_putNameIndex(&b4, ID_Topic);
    ism_protocol_putStringValue(&b4, "iot-2/nomatch/app/id/mon");
    ism_protocol_putNameIndex(&b4, ID_OriginServer);
    ism_protocol_putStringValue(&b4, ism_common_getServerUID());

    size_t grtPropsLen[] = {b1.used, b2.used, b3.used, b4.used};
    char *grtProps[] = {b1.buf, b2.buf, b3.buf, b4.buf};

    expectGetRetainedCount += 4;
    char *grt1Payloads[] = {GRT1_PAYLOAD1, GRT1_PAYLOAD2, GRT1_PAYLOAD3, GRT1_PAYLOAD4, GRT1_PAYLOAD5, NULL};
    char *grt2Payloads[] = {GRT2_PAYLOAD1, GRT2_PAYLOAD2, NULL };
    char *grt3Payloads[] = {GRT3_PAYLOAD1, NULL};
    ism_engine_getRetainedMessage_FakeData_t fakeData[] = { {GET_RETAINED_TOPIC1, 5, grt1Payloads, grtProps[0], grtPropsLen[0]},
                                                            {GET_RETAINED_TOPIC4, 5, grt1Payloads, grtProps[1], grtPropsLen[1]},
                                                            {GET_RETAINED_TOPIC2, 2, grt2Payloads, grtProps[2], grtPropsLen[2]},
                                                            {GET_RETAINED_TOPIC3, 1, grt3Payloads, grtProps[3], grtPropsLen[3]},
                                                            {NULL} };

    getRetainedFakeData = fakeData;
    rc = ism_iot_reconcile();
    CU_ASSERT_EQUAL(getRetainedCount, expectGetRetainedCount);
    getRetainedFakeData = NULL;

    /* TODO: Check the correct number of messages were created, published, and unset */

    // printf("%d C:%u (r:%u) P:%u U:%u\n", __LINE__, createMessageCount, retainedMessagesCreated, putMessageCount, unsetRetainedCount);

    ism_common_freeAllocBuffer(&b1);
    ism_common_freeAllocBuffer(&b2);
    ism_common_freeAllocBuffer(&b3);
    ism_common_freeAllocBuffer(&b4);
}
