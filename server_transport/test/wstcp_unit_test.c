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
#include <wstcp_unit_test.h>

#include <wstcp.c>

#define STR(str) #str
#define QUOTE(str) STR(str)

#define GOOD_HOST_VALUE1        "9.3.179.145:16102"
#define GOOD_HOST_VALUE2        "example.com"

#define GOOD_UPGRADE_VALUE1     "Websocket"
#define GOOD_UPGRADE_VALUE2     " WebsockEt"
#define GOOD_UPGRADE_VALUE3     ",, WEBsocket ,"
#define GOOD_UPGRADE_VALUE4     "websocket, other,,"

#define BAD_UPGRADE_VALUE1      "None"
#define BAD_UPGRADE_VALUE2      "websocket1"

#define GOOD_CONNECTION_VALUE1  "Upgrade"
#define GOOD_CONNECTION_VALUE2  "test, uPGrade"
#define GOOD_CONNECTION_VALUE3  "test1, upgrade, test2"
#define GOOD_CONNECTION_VALUE4  "UPGRADE, test"

#define BAD_CONNECTION_VALUE1   "test"
#define BAD_CONNECTION_VALUE2   "websockets, test"

#define GOOD_WSKEY_VALUE1       "AQIDBAUGBwgJCgsMDQ4PEC=="

#define BAD_WSKEY_VALUE1        "WlpaWlpaWlpaWlpaWlpaWlo="      // 17 'Z's
#define BAD_WSKEY_VALUE2        "//:1,23GBwgJCgsMDQ4PEC=="
#define BAD_WSKEY_VALUE3        "ABCDEFGBwgJCgsMDQ4PEC123"

#define GOOD_WSVERSION_VALUE1   13

#define BAD_WSVERSION_VALUE1    12

#define REQUESTED_PROTOCOL1     "monitoring.ism.ibm.com"
#define REQUESTED_PROTOCOL2     "monitoring"
#define WSKEY                   "sz+ZCx+7QOVLcjZ5XiIkaA=="

#define FRAGMENTED_MESSAGE      "ABCDEFGH"
#define PINGPONG_MESSAGE        "PP"
#define CLOSE_REASON_CODE       10

/*
 * Array that carries all the wstcp tests for APIs to CUnit framework.
 */
CU_TestInfo ISM_wstcp_tests[] = {
    { "--- Testing WS header field processing ---", testProcessHeader },
    { "--- Testing full WS header processing  ---", testCheckHeader },
    { "--- Testing WS handshake parsing       ---", testParseWSHandshake },
    { "--- Testing WS framing                 ---", testWSFraming },
    { "--- Testing parseCookie                ---", testParseCookie },
    { "--- Testing HTTP canonicalization      ---", testCanonicalizeHttp },
    { "--- Testing valid authority            ---", testValidAuthority },
    { "--- Testing parse URI                  ---", testParseURI },
    { "--- Testing extract resource           ---", testExtractResource },
    { "--- Testing fragmented WS messages     ---", testWSFragments },
    CU_TEST_INFO_NULL
};

void testHost(ism_transport_t * transport, ws_frame_t * frame);
void testUpgrade(ism_transport_t * transport, ws_frame_t * frame);
void testConnection(ism_transport_t * transport, ws_frame_t * frame);
void testOrigin(ism_transport_t * transport, ws_frame_t * frame);
void testSecWSKey(ism_transport_t * transport, ws_frame_t * frame);
void testSecWSProt(ism_transport_t * transport, ws_frame_t * frame);
void testSecWSVer(ism_transport_t * transport, ws_frame_t * frame);

void initTransport(ism_transport_t * transport);

void testGoodHandshake(ism_transport_t * transport);
void testBadHandshake(ism_transport_t * transport);

void testOverFlowHandshake(ism_transport_t * transport);
void testWSHandshakeResponse(void);
void testWSHandshakeResponseNoServer(void);
int emptySender(ism_transport_t * transport, char * buf, int len, int frame, int flags);
int emptyClose(ism_transport_t * transport, int rc, int clean, const char * reason);
int emptyReceive(ism_transport_t * transport, char * buf, int len, int protval);
int simpleReceive(ism_transport_t * transport, char * buf, int len, int protval);
int simpleSend(ism_transport_t * transport, char * buf, int len, int frame, int flags);
int testResponseWithoutOrigin(ism_transport_t * transport, char * buf, int len, int frame, int flags);
int testResponseWithOrigin(ism_transport_t * transport, char * buf, int len, int frame, int flags);
int monitorProtocolHandler(ism_transport_t * transport);
int buildMessage(int kind, char *data, int dataLen, char *buffer, int mask, int maxFragLen);

void testAddWSFrame(void);
void testFrameWS(void);

/**
 * Verifies how various HTTP header fields are processed
 */
void testProcessHeader(void) {
    ism_transport_t * transport;
    ism_frameobj_t fobj = {{0}};
    ws_frame_t frame = {0};
    ism_field_t field;
    ism_listener_t listener;
    memset(&listener, 0, sizeof(listener));
    listener.port = 12345;
    listener.name = "test";

    ism_common_setTraceLevel(2);
    ism_common_initUtil();

    field.type = VT_String;
    field.val.s = "/mqtt ws:* public";
    ism_common_setProperty(ism_common_getConfigProperties(), "Alias.0", &field);
    field.val.s = "/monitoring ws:* public";
    ism_common_setProperty(ism_common_getConfigProperties(), "Alias.1", &field);
    field.val.s = "/admin ws:* public";
    ism_common_setProperty(ism_common_getConfigProperties(), "Alias.2", &field);
    ism_transport_wsframe_init();

    transport = ism_transport_newTransport(&listener, 64, 0);
    initTransport(transport);

    transport->fobj = &fobj;
    transport->fobj->frame = &frame;
    frame.transport = transport;

    CU_ASSERT(processHeader(transport, "random_value_to_be_ignored", "testval", &frame) == 0);

    testHost(transport, &frame);
    testUpgrade(transport, &frame);
    testConnection(transport, &frame);
    testOrigin(transport, &frame);
    testSecWSKey(transport, &frame);
    testSecWSProt(transport, &frame);
    testSecWSVer(transport, &frame);
}

/**
 * Tests whether valid values for host are accepted.
 * @param transport  A pointer to the transport instance to be
 *                   passed to processHeader
 * @param frame      A pointer to the frame instance for
 *                   processHeader to update
 */
void testHost(ism_transport_t * transport, ws_frame_t * frame) {
    CU_ASSERT(processHeader(transport, "Host", GOOD_HOST_VALUE1, frame) == 0);
    CU_ASSERT((frame->wshost != NULL) && strcmp(frame->wshost, GOOD_HOST_VALUE1) == 0);
    frame->wshost = NULL;

    CU_ASSERT(processHeader(transport, "Host", GOOD_HOST_VALUE2, frame) == 0);
    CU_ASSERT((frame->wshost != NULL) && strcmp(frame->wshost, GOOD_HOST_VALUE2) == 0);
    frame->wshost = NULL;

    CU_ASSERT(processHeader(transport, "Host", "c: invalid_url", frame) == 400);
    CU_ASSERT(frame->wshost == NULL);
}

/**
 * Tests whether valid values for Upgrade header field are accepted.
 * @param transport  A pointer to the transport instance to be
 *                   passed to processHeader
 * @param frame      A pointer to the frame instance for
 *                   processHeader to update
 */
void testUpgrade(ism_transport_t * transport, ws_frame_t * frame) {
    char tmp[200];
    frame->http_op = 'G';
    strcpy(tmp, GOOD_UPGRADE_VALUE1);
    CU_ASSERT(processHeader(transport, "Upgrade", tmp, frame) == 0);
    CU_ASSERT(frame->upgrade_found == 1);
    frame->upgrade_found = 0;

    strcpy(tmp, GOOD_UPGRADE_VALUE2);
    CU_ASSERT(processHeader(transport, "upgrade", tmp, frame) == 0);
    CU_ASSERT(frame->upgrade_found == 1);
    frame->upgrade_found = 0;

    strcpy(tmp, GOOD_UPGRADE_VALUE3);
    CU_ASSERT(processHeader(transport, "UPgrade", tmp, frame) == 0);
    CU_ASSERT(frame->upgrade_found == 1);
    frame->upgrade_found = 0;

    strcpy(tmp, GOOD_UPGRADE_VALUE4);
    CU_ASSERT(processHeader(transport, "UPGRADE", tmp, frame) == 0);
    CU_ASSERT(frame->upgrade_found == 1);
    frame->upgrade_found = 0;

    strcpy(tmp, BAD_UPGRADE_VALUE1);
    CU_ASSERT(processHeader(transport, "Upgrade", tmp, frame) == 0);
    CU_ASSERT(frame->upgrade_found == 0);
    frame->upgrade_found = 0;

    strcpy(tmp, BAD_UPGRADE_VALUE2);
    CU_ASSERT(processHeader(transport, "Upgrade", tmp, frame) == 0);
    CU_ASSERT(frame->upgrade_found == 0);
    frame->upgrade_found = 0;
}

/**
 * Tests whether valid values for Connection header field are accepted.
 * @param transport  A pointer to the transport instance to be
 *                   passed to processHeader
 * @param frame      A pointer to the frame instance for
 *                   processHeader to update
 */

void testConnection(ism_transport_t * transport, ws_frame_t * frame) {
    char tmp[200];
    strcpy(tmp, GOOD_CONNECTION_VALUE1);
    CU_ASSERT(processHeader(transport, "Connection", tmp, frame) == 0);
    CU_ASSERT(frame->connection_found == 1);
    frame->connection_found = 0;

    strcpy(tmp, GOOD_CONNECTION_VALUE2);
    CU_ASSERT(processHeader(transport, "connection", tmp, frame) == 0);
    CU_ASSERT(frame->connection_found == 1);
    frame->connection_found = 0;

    strcpy(tmp, GOOD_CONNECTION_VALUE3);
    CU_ASSERT(processHeader(transport, "CONNECTION", tmp, frame) == 0);
    CU_ASSERT(frame->connection_found == 1);
    frame->connection_found = 0;

    strcpy(tmp, GOOD_CONNECTION_VALUE4);
    CU_ASSERT(processHeader(transport, "Connection", tmp, frame) == 0);
    CU_ASSERT(frame->connection_found == 1);
    frame->connection_found = 0;

    strcpy(tmp, BAD_CONNECTION_VALUE1);
    CU_ASSERT(processHeader(transport, "Connection", tmp, frame) == 0);
    CU_ASSERT(frame->connection_found == 0);
    frame->connection_found = 0;

    strcpy(tmp, BAD_CONNECTION_VALUE2);
    CU_ASSERT(processHeader(transport, "Connection", tmp, frame) == 0);
    CU_ASSERT(frame->connection_found == 0);
    frame->connection_found = 0;
}

/**
 * Tests whether valid values for Origin header field are accepted.
 * @param transport  A pointer to the transport instance to be
 *                   passed to processHeader
 * @param frame      A pointer to the frame instance for
 *                   processHeader to update
 */
void testOrigin(ism_transport_t * transport, ws_frame_t * frame) {
    char  tmp[256];
    strcpy(tmp, "https://fred.com");
    CU_ASSERT(processHeader(transport, "Origin", tmp, frame) == 0);
    strcpy(tmp, "http://fred.com:8123");
    CU_ASSERT(processHeader(transport, "origin", tmp, frame) == 0);
    strcpy(tmp, "fred.com");
    CU_ASSERT(processHeader(transport, "Origin", tmp, frame) != 0);
    strcpy(tmp, "fred.com:8080/path");
    CU_ASSERT(processHeader(transport, "Origin", tmp, frame) != 0);
}

/**
 * Tests whether valid values for Sec-WebSocket-Key header field are accepted.
 * @param transport  A pointer to the transport instance to be
 *                   passed to processHeader
 * @param frame      A pointer to the frame instance for
 *                   processHeader to update
 */
void testSecWSKey(ism_transport_t * transport, ws_frame_t * frame) {
    CU_ASSERT(processHeader(transport, "sec-WebSocket-Key", GOOD_WSKEY_VALUE1, frame) == 0);
    CU_ASSERT((frame->wskey != NULL) && (strcmp(frame->wskey, GOOD_WSKEY_VALUE1) == 0));
    frame->wskey = NULL;

    CU_ASSERT(processHeader(transport, "Sec-WebSocket-Key", BAD_WSKEY_VALUE1, frame) != 0);
    CU_ASSERT(frame->wskey == NULL);
    frame->wskey = NULL;

    CU_ASSERT(processHeader(transport, "Sec-WebSocket-Key", BAD_WSKEY_VALUE2, frame) != 0);
    CU_ASSERT(frame->wskey == NULL);
    frame->wskey = NULL;

    CU_ASSERT(processHeader(transport, "Sec-WebSocket-Key", BAD_WSKEY_VALUE3, frame) != 0);
    CU_ASSERT(frame->wskey == NULL);
    frame->wskey = NULL;
}

/**
 * Tests whether valid values for Sec-WebSocket-Key header field are accepted.
 * Does not do anything - protocols are validated elsewhere.
 * @param transport  A pointer to the transport instance to be
 *                   passed to processHeader
 * @param frame      A pointer to the frame instance for
 *                   processHeader to update
 */
void testSecWSProt(ism_transport_t * transport, ws_frame_t * frame) {

}

/**
 * Tests whether valid values for Sec-WebSocket-Version header field are accepted.
 * Only version 13 is accepted.
 * @param transport  A pointer to the transport instance to be
 *                   passed to processHeader
 * @param frame      A pointer to the frame instance for
 *                   processHeader to update
 */
void testSecWSVer(ism_transport_t * transport, ws_frame_t * frame) {
    CU_ASSERT(processHeader(transport, "Sec-WebSocket-Version", QUOTE(GOOD_WSVERSION_VALUE1), frame) == 0);
    CU_ASSERT(transport->fobj->wsversion == GOOD_WSVERSION_VALUE1);
    transport->fobj->wsversion = 0;

    CU_ASSERT(processHeader(transport, "Sec-WebSocket-Version", QUOTE(BAD_WSVERSION_VALUE1), frame) != 0);
    transport->fobj->wsversion = 0;
}

/**
 * Verifies checkHeader function
 */
void testCheckHeader(void) {
    ism_transport_t * transport;
    ism_frameobj_t fobj = {{0}};
    ws_frame_t frame;
    ism_listener_t listener;
    memset(&listener, 0, sizeof(listener));
    listener.port = 12345;
    listener.name = "test";

    transport = ism_transport_newTransport(&listener, 64, 0);
    initTransport(transport);

    transport->fobj = &fobj;

    memset(&frame, 0x00, sizeof(frame));

    // Test with all required fields specified
    frame.wshost = GOOD_HOST_VALUE1;
    frame.upgrade_found = 1;
    frame.connection_found = 1;
    frame.wskey = GOOD_WSKEY_VALUE1;
    frame.wsorigin = NULL;
    transport->fobj->wsversion = GOOD_WSVERSION_VALUE1;
    CU_ASSERT(checkHeader(transport, &frame) == 0);

    frame.wsorigin = GOOD_HOST_VALUE2;
    CU_ASSERT(checkHeader(transport, &frame) == 0);

    // Test with optional fields
    frame.wsprotocol[frame.protocount++] = "admin";
    CU_ASSERT(checkHeader(transport, &frame) == 0);

    // Omit one of required fields

    frame.connection_found = 0;
    CU_ASSERT(checkHeader(transport, &frame) != 0);
    frame.connection_found = 1;

    frame.wskey = NULL;
    CU_ASSERT(checkHeader(transport, &frame) != 0);
    frame.wshost = GOOD_WSKEY_VALUE1;

    transport->fobj->wsversion = BAD_WSVERSION_VALUE1;
    CU_ASSERT(checkHeader(transport, &frame) != 0);
    transport->fobj->wsversion = GOOD_WSVERSION_VALUE1;
}

/*
 * Test parseCookie
 */
void testParseCookie(void) {
    const char * cookie;
    const char * cvalue;
    char * more;
    char value [256];
    int  rc;

    strcpy(value, "cookie=value");
    CU_ASSERT((rc=parseCookie(value, &cookie, &cvalue, &more)) == 0);
    if (rc == 0) {
        CU_ASSERT(!strcmp(cookie, "cookie"));
        CU_ASSERT(!strcmp(cvalue, "value"));
        CU_ASSERT(more == NULL);
        CU_ASSERT(parseCookie(more, &cookie, &cvalue, &more) != 0);
    }


    strcpy(value, "cookie =value");
    CU_ASSERT((rc=parseCookie(value, &cookie, &cvalue, &more)) == 0);
    if (rc == 0) {
        CU_ASSERT(!strcmp(cookie, "cookie"));
        CU_ASSERT(!strcmp(cvalue, "value"));
        CU_ASSERT(more == NULL);
    }

    strcpy(value, "cookie= value");
    CU_ASSERT((rc=parseCookie(value, &cookie, &cvalue, &more)) == 0);
    if (rc == 0) {
        CU_ASSERT(!strcmp(cookie, "cookie"));
        CU_ASSERT(!strcmp(cvalue, "value"));
        CU_ASSERT(more == NULL);
    }

    strcpy(value, "cookie = value");
    CU_ASSERT((rc=parseCookie(value, &cookie, &cvalue, &more)) == 0);
    if (rc == 0) {
        CU_ASSERT(!strcmp(cookie, "cookie"));
        CU_ASSERT(!strcmp(cvalue, "value"));
        CU_ASSERT(more == NULL);
    }

    strcpy(value, "cookie = value ");
    CU_ASSERT((rc=parseCookie(value, &cookie, &cvalue, &more)) == 0);
    if (rc == 0) {
        CU_ASSERT(!strcmp(cookie, "cookie"));
        CU_ASSERT(!strcmp(cvalue, "value"));
        CU_ASSERT(more != NULL);
        CU_ASSERT(parseCookie(more, &cookie, &cvalue, &more) != 0);
    }


    strcpy(value, "cookie=value cookie2==value");
    CU_ASSERT((rc=parseCookie(value, &cookie, &cvalue, &more)) == 0);
    if (rc == 0) {
        CU_ASSERT(!strcmp(cookie, "cookie"));
        CU_ASSERT(!strcmp(cvalue, "value"));
        CU_ASSERT(more != NULL);
        CU_ASSERT((rc=parseCookie(more, &cookie, &cvalue, &more)) == 0);
        if (rc == 0) {
            CU_ASSERT(!strcmp(cookie, "cookie2"));
            CU_ASSERT(!strcmp(cvalue, "=value"));
            CU_ASSERT(more == NULL);
        }

    }
}


/**
 * Tests WS handshake handling
 */
void testParseWSHandshake(void) {
    ism_transport_t * transport;
    ism_listener_t listener;
    memset(&listener, 0, sizeof(listener));
    listener.port = 12345;
    listener.name = "test";

    transport = ism_transport_newTransport(&listener, 64, 0);
    initTransport(transport);
    ism_transport_registerProtocol(NULL, monitorProtocolHandler);

    ism_common_setTraceLevel(2);

    testGoodHandshake(transport);
    testBadHandshake(transport);
    listener.usePasswordAuth = 1;
    testOverFlowHandshake(transport);
    testWSHandshakeResponse();
    testWSHandshakeResponseNoServer();

    ism_transport_freeTransport(transport);
}

/**
 * Tests how good handshake requests are handled.
 * @param transport A pointer to a transport object to be
 *                  passed to wsframer and parseWSHandshake
 */
void testGoodHandshake(ism_transport_t * transport) {
    const char * goodHandshake1 =
            "GET ws://9.3.179.145:16102/admin HTTP/1.1\r\n"
            "Origin: null\r\n"
            "Connection: Upgrade\r\n"
            "Host: 9.3.179.145:16102\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Protocol: monitoring.ism.ibm.com, admin.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
    const char * goodHandshake2 =
            "GET /monitoring HTTP/1.1\r\n"
            "Origin: null\r\n"
            "Connection: Upgrade\r\n"
            "Host: 9.3.179.145:16102\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
    const char * goodHandshake3 =
            "GET ws://9.3.179.145:16102/admin HTTP/1.1\r\n"
            "Origin: null\r\n"
            "Connection: Upgrade\r\n"
            "Host:   \t  9.3.179.145:16102\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Upgrade: websocket\t\r\n"
            "Sec-WebSocket-Protocol:    monitoring.ism.ibm.com, admin.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
    int len;
    char * buffer;
    int    used = 0;

    len = strlen(goodHandshake1);
    buffer = malloc(len + 1);
    memcpy(buffer, goodHandshake1, len);
    buffer[len] = 0;
    initTransport(transport);

    memcpy(buffer, goodHandshake1, len);
    buffer[len] = 0;
    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 101);
    initTransport(transport);

    len = strlen(goodHandshake2);
    buffer = realloc(buffer, len + 1);
    memcpy(buffer, goodHandshake2, len);
    buffer[len] = 0;

    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 101);
    initTransport(transport);

    len = strlen(goodHandshake3);
    buffer = realloc(buffer, len + 1);
    memcpy(buffer, goodHandshake3, len);
    buffer[len] = 0;

    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 101);
    initTransport(transport);

    free(buffer);
}

/**
 * Tests how bad handshake requests are handled.
 * @param transport A pointer to a transport object to be
 *                  passed to parseWSHandshake
 */
void testBadHandshake(ism_transport_t * transport) {
    const char * badHandshake1 =  /* HTTP 1.0 */
            "GET ws://9.3.179.145:16102/admin HTTP/1.0\r\n"
            "Origin: null\r\n"
            "Connection: Upgrade\r\n"
            "Host: 9.3.179.145:16102\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Protocol: monitoring.ism.ibm.com, admin.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";

    const char * badHandshake2 =  /* PUT */
            "PUT ws://9.3.179.145:16102/admin HTTP/1.1\r\n"
            "Origin: null\r\n"
            "Connection: Upgrade\r\n"
            "Host: 9.3.179.145:16102\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Protocol: monitoring.ism.ibm.com, admin.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
    const char * badHandshake3 =  /* No Host */
            "GET ws://9.3.179.145:16102/admin HTTP/1.1\r\n"
            "Origin: null\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Protocol: monitoring.ism.ibm.com, admin.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
    const char * badHandshake4 =  /* No Connection */
            "GET ws://9.3.179.145:16102/admin HTTP/1.1\r\n"
            "Origin: null\r\n"
            "Host: 9.3.179.145:16102\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Protocol: monitoring.ism.ibm.com, admin.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
    const char * badHandshake5 =  /* key missing */
            "GET ws://9.3.179.145:16102/admin HTTP/1.1\r\n"
            "Origin: null\r\n"
            "Connection: Upgrade\r\n"
            "Host: 9.3.179.145:16102\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Protocol: monitoring.ism.ibm.com, admin.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
    const char * badHandshake6 =  /* missing protocol */
            "GET ws://9.3.179.145:16102/admin HTTP/1.1\r\n"
            "Origin: null"
            "Connection: Upgrade\r\n"
            "Host: 9.3.179.145:16102\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";
    const char * badHandshake7 =  /* Version missing */
            "GET ws://9.3.179.145:16102/admin HTTP/1.0\r\n"
            "Origin: null\r\n"
            "Connection: Upgrade\r\n"
            "Host: 9.3.179.145:16102\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Upgrade: websocket\r\n"
            "\r\n";
    const char * badHandshake8 =   /* Bad version */
            "GET ws://9.3.179.145:16102/admin HTTP/1.1\r\n"
            "Origin: null\r\n"
            "Connection: Upgrade\r\n"
            "Host: 9.3.179.145:16102\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Version: 12\r\n"
            "\r\n";
    /*
     * badHandshake9   All content is correct but the trailing blank
     *                 line is missing.
     */
    const char * badHandshake9 =
            "GET https://9.2.3.4/admin HTTP/1.1\r\n"
            "connection: Upgrade\r\n"
            "Host: 9.2.3.4\r\n"
            "Sec-WebSocket-Key: " WSKEY "\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Protocol: admin.ism.ibm.com, monitoring.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 13\r\n";

    int len = strlen(badHandshake1);
    int used = 0;

    char * buffer = malloc(len + 1);

    memcpy(buffer, badHandshake1, len);
    buffer[len] = 0;

    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 400);
    initTransport(transport);

    len = strlen(badHandshake2);
    buffer = realloc(buffer, len + 1);
    memcpy(buffer, badHandshake2, len);
    buffer[len] = 0;

    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 404);
    initTransport(transport);

    len = strlen(badHandshake3);
    buffer = realloc(buffer, len + 1);
    memcpy(buffer, badHandshake3, len);
    buffer[len] = 0;

    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 400);
    initTransport(transport);

    len = strlen(badHandshake4);
    buffer = realloc(buffer, len + 1);
    memcpy(buffer, badHandshake4, len);
    buffer[len] = 0;

    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 400);
    initTransport(transport);

    len = strlen(badHandshake5);
    buffer = realloc(buffer, len + 1);
    memcpy(buffer, badHandshake5, len);
    buffer[len] = 0;

    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 400);
    initTransport(transport);

    len = strlen(badHandshake6);
    buffer = realloc(buffer, len + 1);
    memcpy(buffer, badHandshake6, len);
    buffer[len] = 0;

    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 400);
    initTransport(transport);

    len = strlen(badHandshake7);
    buffer = realloc(buffer, len + 1);
    memcpy(buffer, badHandshake7, len);
    buffer[len] = 0;

    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 400);
    initTransport(transport);

    len = strlen(badHandshake8);
    buffer = realloc(buffer, len + 1);
    memcpy(buffer, badHandshake8, len);
    buffer[len] = 0;

    // printf("rc=%d %d\n", parseWSHandshake(transport, buffer, len), __LINE__);
    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 426);
    initTransport(transport);

    len = strlen(badHandshake9);
    buffer = realloc(buffer, len + 1);
    memcpy(buffer, badHandshake9, len);
    buffer[len] = 0;

    //printf("rc=%d %d\n", parseWSHandshake(transport, buffer, len), __LINE__);
    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == -1);
    initTransport(transport);
    free(buffer);
}

void testOverFlowHandshake(ism_transport_t * transport) {

    const char * overflowHandShake =
            "GET ws://9.3.179.145:16102/admin HTTP/1.1\r\n"
            "Origin: http://AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n"
            "Connection: Upgrade\r\n"
            "Host:   \t  9.3.179.145:16102\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Sec-WebSocket-Protocol:    monitoring.ism.ibm.com, admin.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";

    int len;
    char * buffer;
    int    used = 0;

    len = strlen(overflowHandShake);
    buffer = malloc(len + 1);
    initTransport(transport);

    memcpy(buffer, overflowHandShake, len);
    buffer[len] = 0;
    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 401);
    initTransport(transport);

    free(buffer);
}

/**
 * Verifies WS handshake response
 */
void testWSHandshakeResponse(void) {
    const char * goodHandshake1 =
            "GET https://9.2.3.4/admin HTTP/1.1\r\n"
            "connection: Upgrade\r\n"
            "Host: 9.2.3.4\r\n"
            "Sec-WebSocket-Key: " WSKEY "\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Protocol: admin.ism.ibm.com, monitoring.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n";

    const char * goodHandshake2 =
            "GET http://9.2.3.4/admin HTTP/1.1\r\n"
            "Connection:    Upgrade\r\n"
            "Origin: null\r\n"
            "host: 9.2.3.4:8080\r\n"
            "Sec-WebSocket-Key: " WSKEY "\r\n"
            "Upgrade: \r\n webSOCKet\r\n"
            "Sec-WebSocket-Protocol: unknown, monitoring.ism.ibm.com,\r\n admin.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 0013\r\n"
            "\r\n";
    ism_listener_t listener;
    memset(&listener, 0, sizeof(listener));
    listener.port = 12345;
    listener.name = "test";
    int used = 0;

    ism_transport_t * transport = ism_transport_newTransport(&listener, 64, 0);

    initTransport(transport);

    ism_transport_registerProtocol(NULL, monitorProtocolHandler);

    transport->send = testResponseWithoutOrigin;
    int len = strlen(goodHandshake1);
    char * buffer = malloc(len + 1);
    memcpy(buffer, goodHandshake1, len);
    buffer[len] = 0;
    CU_ASSERT(parseWSHandshake(transport, buffer, len, &used) == 101);
    initTransport(transport);

    transport->send = testResponseWithOrigin;
    len = strlen(goodHandshake2);
    buffer = realloc(buffer, len + 1);
    memcpy(buffer, goodHandshake2, len);
    buffer[len] = 0;
    int rc;
    CU_ASSERT((rc = parseWSHandshake(transport, buffer, len, &used)) == 101);
    if (rc != 101) {
        printf("failed 629: %d\n",rc);
    }
    initTransport(transport);

    free(buffer);
    ism_transport_freeTransport(transport);
}

/**
 * Verifies WS handshake response (variant when we suppress the Server header)
 */
void testWSHandshakeResponseNoServer(void) {
    g_sendServerHTTPHeader = 0;
    testWSHandshakeResponse();
    g_sendServerHTTPHeader = 1;
}
/*
 * Verifies that the WS handshake response is correct when origin is not present.
 * @param transport  The transport object
 * @param buf        The data to send to the client
 * @param len        The length of the data
 * @param frame      A frame specific value.  This is used when one of the protocol fields is in the frame.
 * @param flags      A set of send flags (see SFLAG_)
 * @return A return code: 0=good
 */
int testResponseWithoutOrigin(ism_transport_t * transport, char * buf, int len, int frame, int flags) {
    char    expectedResponse[1024];
    char    b64buf[1024];
    uint8_t sbuf[20];
    char  * ptr;
    int     off;

    memset(sbuf, 0x00, sizeof(sbuf));

    int keysize = (int)(strlen(WSKEY) + strlen(SERVER_GUID));
    char *key = alloca(keysize+1);
    strcpy(key, WSKEY);
    strcat(key, SERVER_GUID);
    SHA1((uint8_t *)key, keysize, sbuf);
    ism_common_toBase64((char *)sbuf, b64buf, 20);

    sprintf(expectedResponse, WEBSOCKET_SERVER_RESPONSE_HEADER, REQUESTED_PROTOCOL1, b64buf,
            getServerHTTPHeaderString(),"now");

    ptr = strstr(buf, "HTTP/1.1 101 Switching Protocols");
    CU_ASSERT(ptr != NULL);
    if (ptr == NULL) {
        return 0;
    }
    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, off) == 0);

    ptr = strstr(ptr, "Upgrade: websocket");
    CU_ASSERT(ptr != NULL);
    if (ptr == NULL) {
        return 0;
    }
    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, off) == 0);

    ptr = strstr(ptr, "Connection: Upgrade");
    CU_ASSERT(ptr != NULL);
    if (ptr == NULL) {
        return 0;
    }
    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, off) == 0);

    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, off) == 0);

    ptr = strstr(ptr, "Sec-WebSocket-Protocol: "REQUESTED_PROTOCOL1);

    CU_ASSERT(ptr != NULL);
    if (ptr == NULL) {
        return 0;
    }
    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, off) == 0);

    /*
     * One character in base64-encoded value could be different for identical
     * input values, hence string comparison is slightly different here.
     */
    ptr = strstr(ptr, "Sec-WebSocket-Accept: 9HjK6km3c7ZT3snnGIJ7FyK5iR");
    CU_ASSERT(ptr != NULL);
    if (ptr == NULL) {
        return 0;
    }
    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, strlen("Sec-WebSocket-Accept: 9HjK6km3c7ZT3snnGIJ7FyK5iR")) == 0);

    ptr = strstr(ptr, "Server: ");
    if (g_sendServerHTTPHeader) {
        CU_ASSERT(ptr != NULL);
        if (ptr == NULL) {
            return 0;
        }
    } else {
        CU_ASSERT(ptr == NULL);
        if (ptr != NULL) {
            return 0;
        }
    }
    return 0;
}

/**
 * Verifies that the WS handshake response is correct when origin is present.
 * @param transport  The transport object
 * @param buf        The data to send to the client
 * @param len        The length of the data
 * @param frame      A frame specific value.  This is used when one of the protocol fields is in the frame.
 * @param flags      A set of send flags (see SFLAG_)
 * @return A return code: 0=good
 */
int testResponseWithOrigin(ism_transport_t * transport, char * buf, int len, int frame, int flags) {
    char    expectedResponse[1024];
    char    b64buf[1024];
    uint8_t sbuf[20];
    char  * ptr;
    int     off;

    memset(sbuf, 0x00, sizeof(sbuf));

    int keysize = (int)(strlen(WSKEY) + strlen(SERVER_GUID));
    char *key = alloca(keysize+1);
    strcpy(key, WSKEY);
    strcat(key, SERVER_GUID);
    SHA1((uint8_t *)key, keysize, sbuf);
    ism_common_toBase64((char *)sbuf, b64buf, 20);

    sprintf(expectedResponse, WEBSOCKET_SERVER_RESPONSE_HEADER_ORIGIN, "null", REQUESTED_PROTOCOL1, b64buf,
            getServerHTTPHeaderString(), "now");

    ptr = strstr(buf, "HTTP/1.1 101 Switching Protocols");
    CU_ASSERT(ptr != NULL);
    if (ptr == NULL) {
        printf("fail 740: [%s]\n", buf);
        return 0;
    }
    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, off) == 0);

    ptr = strstr(ptr, "Upgrade: websocket");
    CU_ASSERT(ptr != NULL);
    if (ptr == NULL) {
        return 0;
    }
    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, off) == 0);

    ptr = strstr(ptr, "Connection: Upgrade");
    CU_ASSERT(ptr != NULL);
    if (ptr == NULL) {
        return 0;
    }
    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, off) == 0);

    ptr = strstr(ptr, "Origin: null");
    CU_ASSERT(ptr != NULL);
    if (ptr == NULL) {
        return 0;
    }
    // printf("%d [%s]\n", __LINE__, ptr);
    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, off) == 0);

    ptr = strstr(ptr, "Sec-WebSocket-Protocol: "REQUESTED_PROTOCOL1);
    CU_ASSERT(ptr != NULL);
    if (ptr == NULL) {
        return 0;
    }
    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, off) == 0);

    /*
     * One character in base64-encoded value could be different for identical
     * input values, hence string comparison is slightly different here.
     */
    ptr = strstr(ptr, "Sec-WebSocket-Accept: 9HjK6km3c7ZT3snnGIJ7FyK5iR");
    CU_ASSERT(ptr != NULL);
    if (ptr == NULL) {
        return 0;
    }
    off = strcspn(ptr, "\r\n");
    CU_ASSERT(strncmp(expectedResponse + (ptr - buf), ptr, strlen("Sec-WebSocket-Accept: 9HjK6km3c7ZT3snnGIJ7FyK5iR")) == 0);
    ptr = strstr(ptr, "Server: ");
    if (g_sendServerHTTPHeader) {
        CU_ASSERT(ptr != NULL);
        if (ptr == NULL) {
            return 0;
        }
    } else {
        CU_ASSERT(ptr == NULL);
        if (ptr != NULL) {
            return 0;
        }
    }

    return 0;
}

/**
 * Protocol handler matching "monitoring.ism.ibm.com" and "monitoring"
 * @param transport A pointer to a transport object to check
 */
int monitorProtocolHandler(ism_transport_t * transport) {
    int rc = !((strcmp(transport->protocol, REQUESTED_PROTOCOL1) == 0) ||
            (strcmp(transport->protocol, REQUESTED_PROTOCOL2) == 0));

    return rc;
}

/**
 * Initialize transport object with empty send/receive/close functions
 * and localhost IP address and default port
 * @param transport A pointer to a transport object to initialize
 */
void initTransport(ism_transport_t * transport) {
    transport->send     = emptySender;
    transport->receive  = emptyReceive;
    transport->close    = emptyClose;
    transport->state    = ISM_TRANST_Open;

    transport->server_addr = "127.0.0.1";
    transport->serverport  = 16102;
    transport->client_addr = "fromip";
    transport->clientport  = 5;
    transport->protocol = "unknown";

}

/**
 * Tests WS framing functions
 */
void testWSFraming(void) {
    testAddWSFrame();
    testFrameWS();
}

/**
 * Ensures that WS frames are added to the message correctly (no checks for masking)
 */
void testAddWSFrame(void) {
    char buffer[512];
    int len, kind;
    uint16_t shortLen;
    uint64_t longLen;
    int rc;

    memset(buffer, 0x00, sizeof(buffer));

    // Try short length, ping
    len = 84;
    kind = 0x9;
    rc = ism_transport_addWSFrame(NULL, buffer + 2, len, kind);
    CU_ASSERT(rc == 2);
    CU_ASSERT(buffer[0] & 0x80);
    CU_ASSERT((buffer[0] & 0x7F) == kind);
    CU_ASSERT((buffer[1] & 0x7F) == len);
    CU_ASSERT(buffer[2] == 0x00);
    CU_ASSERT(buffer[3] == 0x00);
    CU_ASSERT(buffer[4] == 0x00);
    CU_ASSERT(buffer[5] == 0x00);

    len = 125;
    kind = 0x9;
    rc = ism_transport_addWSFrame(NULL, buffer + 2, len, kind);
    CU_ASSERT(rc == 2);
    CU_ASSERT(buffer[0] & 0x80);
    CU_ASSERT((buffer[0] & 0x7F) == kind);
    CU_ASSERT((buffer[1] & 0x7F) == len);
    CU_ASSERT(buffer[2] == 0x00);
    CU_ASSERT(buffer[3] == 0x00);
    CU_ASSERT(buffer[4] == 0x00);
    CU_ASSERT(buffer[5] == 0x00);

    // Try medium length (>125 and <=0xFFFF)
    len = 126;
    kind = 0x9;
    rc = ism_transport_addWSFrame(NULL, buffer + 4, len, kind);
    CU_ASSERT(rc == 4);
    CU_ASSERT(buffer[0] & 0x80);
    CU_ASSERT((buffer[0] & 0x7F) == kind);
    CU_ASSERT((buffer[1] & 0x7F) == 126);
    shortLen = htons(len);
    CU_ASSERT(memcmp(&shortLen, buffer + 2, 2) == 0);
    CU_ASSERT(buffer[4] == 0x00);
    CU_ASSERT(buffer[5] == 0x00);

    len = 0xFFFF;
    kind = 0x9;
    rc = ism_transport_addWSFrame(NULL, buffer + 4, len, kind);
    CU_ASSERT(rc == 4);
    CU_ASSERT(buffer[0] & 0x80);
    CU_ASSERT((buffer[0] & 0x7F) == kind);
    CU_ASSERT((buffer[1] & 0x7F) == 126);
    shortLen = htons(len);
    CU_ASSERT(memcmp(&shortLen, buffer + 2, 2) == 0);
    CU_ASSERT(buffer[4] == 0x00);
    CU_ASSERT(buffer[5] == 0x00);

    // Try long length (>0xFFFF)
    len = 0xFFFF + 1;
    kind = 0x9;
    rc = ism_transport_addWSFrame(NULL, buffer + 10, len, kind);
    CU_ASSERT(rc == 10);
    CU_ASSERT(buffer[0] & 0x80);
    CU_ASSERT((buffer[0] & 0x7F) == kind);
    CU_ASSERT((buffer[1] & 0x7F) == 127);
    longLen = htonll(len);
    CU_ASSERT(memcmp(&longLen, buffer + 2, 8) == 0);
    CU_ASSERT((buffer[2] & 0x80) == 0);
    CU_ASSERT(buffer[10] == 0x00);
    CU_ASSERT(buffer[11] == 0x00);

    len = 123456789;
    kind = 0x9;
    rc =ism_transport_addWSFrame(NULL, buffer + 10, len, kind);
    CU_ASSERT(rc == 10);
    CU_ASSERT(buffer[0] & 0x80);
    CU_ASSERT((buffer[0] & 0x7F) == kind);
    CU_ASSERT((buffer[1] & 0x7F) == 127);
    longLen = htonll(len);
    CU_ASSERT(memcmp(&longLen, buffer + 2, 8) == 0);
    CU_ASSERT((buffer[2] & 0x80) == 0);
    CU_ASSERT(buffer[10] == 0x00);
    CU_ASSERT(buffer[11] == 0x00);
}

/**
 * Test WS framing implementation (with masking).
 * transport's close and receive are noop'ed.
 */
void testFrameWS(void) {
    ism_transport_t * transport;
    const int BUF_SIZE = 100000;
    char *buffer = malloc(BUF_SIZE);
    int rc;
    int len, kind, used;
    xUNUSED int save_msgcnt;
    const int MAX_PAYLOAD_SIZE = 70000;
    char *payload = malloc(MAX_PAYLOAD_SIZE);
    unsigned int mask = 0x12345678;
    ism_listener_t listener;
    memset(&listener, 0, sizeof(listener));
    listener.port = 12345;
    listener.name = "test";

    transport = ism_transport_newTransport(&listener, 64, 0);
    initTransport(transport);

    memset(buffer, 'A', BUF_SIZE);

    // Add WS frame
    len = 84;
    kind = WS_TEXT;     // Text frame
    ism_transport_addWSFrame(NULL, buffer + 2, len, kind);

    // Try unmasked message
    rc = ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used);
    CU_ASSERT(rc == -1);

    // Try masked message
    buffer[1] |= 0x80;  // Masked
    rc = ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used);
    CU_ASSERT(rc == 0);

    buffer[0] |= 0x80;  // Final frame

    // Try setting reserved fields
    buffer[0] |= 0x70;
    rc = ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used);
    CU_ASSERT(rc == -1);

    memset(payload, 'A', MAX_PAYLOAD_SIZE);

    // Try mask request with short length and verify payload
    len = 10;
    buildMessage(WS_TEXT, payload, len, buffer, mask, len);
    used = 0;
    rc = ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used);
    CU_ASSERT(rc == 0);
    CU_ASSERT(used == 16);
    CU_ASSERT(strspn(buffer + 6, "A") == len);

    // Verify that small buffer fails and indicates correct required length
    rc = ism_transport_frameWS(transport, buffer, 0, 5, &used);
    CU_ASSERT(rc == 16);

    // Try mask request with medium length and verify payload
    // Mask payload of 1024 'A's using masking key of 0x12345678
    memset(buffer, 0x00, BUF_SIZE);
    len = 1024;
    buildMessage(WS_TEXT, payload, len, buffer, mask, len);
    used = 0;
    rc = ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used);
    CU_ASSERT(rc == 0);
    CU_ASSERT(used == len + 8);
    CU_ASSERT(strspn(buffer + 8, "A") == len);

    // Verify that small buffer fails and indicates correct required length
    rc = ism_transport_frameWS(transport, buffer, 0, len, &used);
    CU_ASSERT(rc == (len + 8));

    // Try mask request with long length and verify payload
    // Mask payload of 70000 'A's using masking key of 0x12345678
    memset(buffer, 0x00, BUF_SIZE);
    len = MAX_PAYLOAD_SIZE;
    buildMessage(WS_TEXT, payload, len, buffer, mask, len);
    used = 0;

    save_msgcnt = transport->read_msg;
    rc = ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used);
    CU_ASSERT(rc == 0);
    CU_ASSERT(used == MAX_PAYLOAD_SIZE + 14);
    CU_ASSERT(strspn(buffer + 14, "A") == len);

    // Verify that small buffer fails and indicates correct required length
    rc = ism_transport_frameWS(transport, buffer, 0, len, &used);
    CU_ASSERT(rc == (len + 14));

    free(buffer);
    free(payload);
    ism_transport_freeTransport(transport);
}

/**
 * Test whether HTTP canonicalization works as intended:
 * 1. Change all runs of linear white space to a single space.
 * 2. Remove continuation lines
 * 3. Change all CR/LF to LF character.
 * 4. Detect invalid control characters
 */
void testCanonicalizeHttp(void) {
    const char *TEST1 = "1234     456   56   56656  56 \t t\tt\t \tf  ";
    const char *TEST1_RESULT = "1234 456 56 56656 56 t t f ";

    const char *TEST2 = "1234  \r\n   456 \r\n  56   56656  56 \t t\tt\t \tf  ";
    const char *TEST2_RESULT = "1234 456 56 56656 56 t t f ";

    const char *TEST3 = "1234\r\n123   123";
    const char *TEST3_RESULT = "1234\n123 123";

    const char *TEST4 = "1234  \n   456 \n  56   56656\b  56 \t t\tt\t \tf  ";

    char buffer[255];

    strcpy(buffer, TEST1);
    CU_ASSERT(canonicalizeHttp(buffer, strlen(buffer)) > 0);
    CU_ASSERT(strcmp(buffer, TEST1_RESULT) == 0);

    strcpy(buffer, TEST2);
    CU_ASSERT(canonicalizeHttp(buffer, strlen(buffer)) > 0);
    CU_ASSERT(strcmp(buffer, TEST2_RESULT) == 0);

    strcpy(buffer, TEST3);
    CU_ASSERT(canonicalizeHttp(buffer, strlen(buffer)) > 0);
    CU_ASSERT(strcmp(buffer, TEST3_RESULT) == 0);

    strcpy(buffer, TEST4);
    CU_ASSERT(canonicalizeHttp(buffer, strlen(buffer)) < 0);
}

/*
 * Test the validAuthority method
 */
void testValidAuthority(void) {
    CU_ASSERT(validAuthority("abc.com:123") == 1);
    CU_ASSERT(validAuthority(":123") == 0);
    CU_ASSERT(validAuthority("abc.com") == 1);
    CU_ASSERT(validAuthority("abc:def") == 0);
    CU_ASSERT(validAuthority(" ") == 0);
    CU_ASSERT(validAuthority("abc.com:123/path") == 0);
    CU_ASSERT(validAuthority("[abcd::ab]:12345") == 1);
    CU_ASSERT(validAuthority("abc.com:0") == 0);
    CU_ASSERT(validAuthority("def.net:755g") == 0);
    CU_ASSERT(validAuthority("ghi.biz:679999") == 0);
}

/*
 * Test the parseuri method
 */
void testParseURI(void) {
    char * scheme;
    char * authority;
    char * path;
    char * query;
    char * fragment;
    char   pathsep;
    int    rc;
    char   copy [156];

    strcpy(copy, " ");
    rc = parseuri(copy, &scheme, &authority, &pathsep, &path, &query, &fragment);
    CU_ASSERT(rc != 0);

    strcpy(copy, "http://auth:port/path?query#frag");
    rc = parseuri(copy, &scheme, &authority, &pathsep, &path, &query, &fragment);
    CU_ASSERT(rc == 0);
    CU_ASSERT(!strcmp(scheme, "http"));
    CU_ASSERT(!strcmp(authority, "auth:port"));
    CU_ASSERT(pathsep == '/');
    CU_ASSERT(!strcmp(path, "path"));
    CU_ASSERT(!strcmp(query, "query"));
    CU_ASSERT(!strcmp(fragment, "frag"));

    strcpy(copy, "hTTps://9.2.3.4:8080/chat");
    rc = parseuri(copy, &scheme, &authority, &pathsep, &path, &query, &fragment);
    CU_ASSERT(rc == 0);
    CU_ASSERT(!strcmp(scheme, "https"));
    CU_ASSERT(!strcmp(authority, "9.2.3.4:8080"));
    CU_ASSERT(pathsep == '/');
    CU_ASSERT(!strcmp(path, "chat"));
    CU_ASSERT(query == NULL);
    CU_ASSERT(fragment == NULL);

    strcpy(copy, "HTTP:/path/morepath");
    rc = parseuri(copy, &scheme, &authority, &pathsep, &path, &query, &fragment);
    CU_ASSERT(rc == 0);
    CU_ASSERT(!strcmp(scheme, "http"));
    CU_ASSERT(pathsep = '/');
    CU_ASSERT(authority == NULL);
    CU_ASSERT(!strcmp(path, "path/morepath"));

    strcpy(copy, "rmm:path?query");
    rc = parseuri(copy, &scheme, &authority, &pathsep, &path, &query, &fragment);
    CU_ASSERT(rc == 0);
    CU_ASSERT(!strcmp(scheme, "rmm"));
    CU_ASSERT(authority == NULL);
    CU_ASSERT(pathsep == 0);
    CU_ASSERT(!strcmp(path, "path"));
    CU_ASSERT(!strcmp(query, "query"));
    CU_ASSERT(fragment == NULL);

    strcpy(copy, "xyz:?query#frag");
    rc = parseuri(copy, &scheme, &authority, &pathsep, &path, &query, &fragment);
    CU_ASSERT(rc == 0);
    CU_ASSERT(rc == 0);
    CU_ASSERT(!strcmp(scheme, "xyz"));
    CU_ASSERT(authority == NULL);
    CU_ASSERT(pathsep == 0);
    CU_ASSERT(path == NULL);
    CU_ASSERT(!strcmp(query, "query"));
    CU_ASSERT(!strcmp(fragment, "frag"));

    strcpy(copy, "path?query");
    rc = parseuri(copy, &scheme, &authority, &pathsep, &path, &query, &fragment);
    CU_ASSERT(rc == 0);
    CU_ASSERT(scheme==NULL);
    CU_ASSERT(authority==NULL);
    CU_ASSERT(pathsep == 0);
    CU_ASSERT(!strcmp(path, "path"));
    CU_ASSERT(!strcmp(query, "query"));
}

/*
 * Test the extractResource method
 */
void testExtractResource(void) {
    char buf[100];
    ism_listener_t listener;
    memset(&listener, 0, sizeof(listener));
    listener.port = 12345;
    listener.name = "test";
    ism_transport_t * transport = ism_transport_newTransport(&listener, 64, 0);
    initTransport(transport);

    CU_ASSERT(extractResource(transport, " ") == NULL);
    CU_ASSERT(!strcmp(extractResource(transport, strcpy(buf, "/chat")), "/chat"));
    CU_ASSERT(!strcmp(extractResource(transport, strcpy(buf, "chat")), "chat"));
    CU_ASSERT(!strcmp(extractResource(transport, strcpy(buf, "http://fred/sam")), "/sam"));
    CU_ASSERT(!strcmp(extractResource(transport, strcpy(buf, "/fred?sam")), "/fred"));
    /* TODO: */
 //   printf("?sam 11162: [%s]\n", extractResource(transport, strcpy(buf, "?sam")));
 //   CU_ASSERT(!strcmp(extractResource(transport, strcpy(buf, "?sam")), "sam"));
    CU_ASSERT(extractResource(transport, strcpy(buf, "http://abc/path#frag")) == NULL);
}




int emptyClose(ism_transport_t * transport, int rc, int clean, const char * reason) {
    return 0;
}


int emptyReceive(ism_transport_t * transport, char * buf, int len, int protval) {
    return 0;
}


int emptySender(ism_transport_t * transport, char * buf, int len, int frame, int flags) {
    return 0;
}

int simpleReceive(ism_transport_t * transport, char * buf, int len, int protval) {
    CU_ASSERT((len == 8) && (strncmp(buf, FRAGMENTED_MESSAGE, len) == 0));
    return 0;
}

int simpleSend(ism_transport_t * transport, char * buf, int len, int frame, int flags) {
    CU_ASSERT((frame == WS_PONG) && (len == 2) && (strncmp(buf, PINGPONG_MESSAGE, len) == 0));
    return 0;
}

int simpleSendClose(ism_transport_t * transport, char * buf, int len, int frame, int flags) {
    unsigned short rc = CLOSE_REASON_CODE;
    CU_ASSERT((frame == WS_CONNECTION_CLOSE) && (len == 2) && (memcmp(buf, &rc, len) == 0));

    transport->state = ISM_TRANST_Closing;

    return 0;
}


/**
 * Try fragmented messages:
 * 1. With 2 fragments and a receiver function expecting "ABCDEFGH".
 * 2. With 3 fragments and a receiver function expecting "ABCDEFGH".
 * 3. With 3 fragments, ping after the 1st fragment and a receiver function
 *    expecting "ABCDEFGH".
 * 4. With 3 fragments, pong after the 2nd fragment (should be ignored).
 * 5. With 3 fragments, after the 1st fragment and failure on the next call
 *    to frameWS.
 */
void testWSFragments(void) {
    ism_transport_t * transport;
    const int BUF_SIZE = 100000;
    char *buffer = malloc(BUF_SIZE);
    int len, used;
    const unsigned int MASK = 0x12345678;
    ism_listener_t listener;
    memset(&listener, 0, sizeof(listener));
    listener.port = 12345;
    listener.name = "test";

    transport = ism_transport_newTransport(&listener, 64, 0);
    initTransport(transport);
    transport->receive = simpleReceive;
    transport->send = simpleSend;

    /*
     * Populate the buffer with 2 fragments.
     * First, try if full buffer succeeds.
     * Next, ensure small buffer fails.
     */
    int msgSize = buildMessage(WS_TEXT, FRAGMENTED_MESSAGE, strlen(FRAGMENTED_MESSAGE),
            buffer, MASK, strlen(FRAGMENTED_MESSAGE)/2);

    CU_ASSERT(ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used) == 0);

    int i;
    for (i = 0; i < msgSize; i++) {
        buildMessage(WS_TEXT, FRAGMENTED_MESSAGE, strlen(FRAGMENTED_MESSAGE),
                buffer, MASK, strlen(FRAGMENTED_MESSAGE)/2);
        int rc = ism_transport_frameWS(transport, buffer, 0, i, &used);
        if (i < 2) {
            CU_ASSERT(rc == 2);
        } else if (i < 10) {
            CU_ASSERT(rc == 10);
        } else if (i < 12) {
            CU_ASSERT(rc == 12);
        } else {
            CU_ASSERT(rc == msgSize);
        }
    }

    buildMessage(WS_TEXT, FRAGMENTED_MESSAGE, 8, buffer, MASK, 4);
    CU_ASSERT(ism_transport_frameWS(transport, buffer, 0, msgSize, &used) == 0);

    /*
     * Populate the buffer with 3 fragments (3-3-2).
     */
    buildMessage(WS_TEXT, FRAGMENTED_MESSAGE, 8, buffer, MASK, 3);

    /* Should not fail */
    CU_ASSERT(ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used) == 0);

    /*
     * 3. Populate the buffer with 3 fragments (3-3-2), with ping after #1.
     */
    msgSize = buildMessage(WS_TEXT, FRAGMENTED_MESSAGE, 8, buffer, MASK, 3);

    /* Insert ping (length 8 at offset 9) */
    int offset = 9;
    memmove(buffer + offset + 8, buffer + offset, msgSize - offset);
    len = 2;
    ism_transport_addWSFrame(NULL, buffer + offset + 2, len, WS_PING);
    memcpy(buffer + offset + 6, PINGPONG_MESSAGE, 2);
    // Mask the message
    buffer[offset + 1] |= 0x80;
    memcpy(buffer + offset + 2, &MASK, 4);
    char *mask = buffer + offset + 2;
    buffer[offset + 6] ^= mask[0]; // Masked payload
    buffer[offset + 7] ^= mask[1];

    /* Should not fail */
    CU_ASSERT(ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used) == 0);

    /*
     * 4. With 3 fragments, pong after the 2nd fragment (should be ignored).
     */
    msgSize = buildMessage(WS_TEXT, FRAGMENTED_MESSAGE, 8, buffer, MASK, 3);

    /* Insert pong (length 8 at offset 18) */
    offset = 18;
    memmove(buffer + offset + 8, buffer + offset, msgSize - offset);
    len = 2;
    ism_transport_addWSFrame(NULL, buffer + offset + 2, len, WS_PONG);
    memcpy(buffer + offset + 6, PINGPONG_MESSAGE, 2);
    // Mask the message
    buffer[offset + 1] |= 0x80;
    memcpy(buffer + offset + 2, &MASK, 4);
    mask = buffer + offset + 2;
    buffer[offset + 6] ^= mask[0]; // Masked payload
    buffer[offset + 7] ^= mask[1];

    /* Should not fail */
    CU_ASSERT(ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used) == 0);

    /*
     * 5. Populate the buffer with 3 fragments, with close after #1.
     */
    msgSize = buildMessage(WS_TEXT, FRAGMENTED_MESSAGE, 8, buffer, MASK, 3);

    /* Insert close (length 8 at offset 9) */
    unsigned short reasonCode = CLOSE_REASON_CODE;
    offset = 9;
    memmove(buffer + offset + 8, buffer + offset, msgSize - offset);
    len = 2;
    ism_transport_addWSFrame(NULL, buffer + offset + 2, len, WS_CONNECTION_CLOSE);
    memcpy(buffer + offset + 6, &reasonCode, 2);

    // Mask the message
    buffer[offset + 1] |= 0x80;
    memcpy(buffer + offset + 2, &MASK, 4);
    mask = buffer + offset + 2;
    buffer[offset + 6] ^= mask[0];
    buffer[offset + 7] ^= mask[1];

    // Should force connection closed
    transport->send = simpleSendClose;
    CU_ASSERT(ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used) == 0);

    // Now send a single frame to ensure connection is indeed closed
    buildMessage(WS_TEXT, "AAAAAAAA", 8, buffer, MASK, 8);
    transport->receive = emptyReceive;
    CU_ASSERT(ism_transport_frameWS(transport, buffer, 0, BUF_SIZE, &used) == -1);

    free(buffer);
    ism_transport_freeTransport(transport);
}

/**
 * Build a fragmented or non-fragmented WS message in a buffer.
 * @param kind       A message type (should be valid WebSocket frame type)
 * @param data       A pointer to the data to be inserted in a message
 * @param dataLen    The length of the data
 * @param buffer     A pointer to the location of the resulting message
 * @param mask       A mask used to mask the payload
 * @param maxFragLen A maximum amount of data (payload) in a message fragment
 * @return The length of the message (including control info and payload)
 */
int buildMessage(int kind, char *data, int dataLen, char *buffer, int mask, int maxFragLen) {
    int offset = 0;
    char *ptr = data;
    int fragments = (dataLen + maxFragLen - 1) / maxFragLen;
    int i;

    for (i = 0; i < fragments; i++) {
        int len, disp;
        if (i == 0) {
            len = (dataLen < maxFragLen)?dataLen:maxFragLen;
        } else if (i == fragments - 1) {
            len = dataLen - (fragments - 1) * maxFragLen;
        } else {
            len = maxFragLen;
        }

        if (len < 125) {
            disp = 2;
        } else if (len < (64*1024)) {
            disp = 4;
        } else {
            disp = 10;
        }

        ism_transport_addWSFrame(NULL, buffer + offset + disp, len, kind);

        if (fragments > 1) {
            // Fragmented message
            if (i == 0) {
                // This is the first fragment
                buffer[offset] &= 0x7F;
            } else if (i == fragments - 1) {
                // This is the last fragment
                buffer[offset] = 0x80;
            } else {
                // This is the middle fragment
                buffer[offset] = 0x00;
            }
        }

        memcpy(buffer + offset + 4 + disp, ptr, len);

        // Mask the message
        buffer[offset + 1] |= 0x80;

        memcpy(buffer + offset + disp, &mask, 4);
        char * maskPtr = buffer + offset + disp;

        int j;
        for (j = 0; j < len; j++) {
            buffer[offset + 4 + disp + j] ^= maskPtr[j&3];
        }

        offset += (4 + disp + len);
        ptr += len;
    }

    return offset;
}
