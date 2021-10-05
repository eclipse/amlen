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
#include <tcp_unit_test.h>

#include <transport.c>
#include <tcp.c>
#include <config.h>
#include <sys/time.h>
void ism_engine_threadInit(uint8_t isStoreCrit){
    return;
}
void ism_engine_threadTerm(uint8_t closeStoreStream){
    return;
}
extern int ism_log_init(void);
extern int ism_log_createLogger(int which, ism_prop_t * props);

/*
 * Array that carries all simple tcp tests for APIs to CUnit framework.
 */
CU_TestInfo ISM_tcp_tests[] = {
        { "--- Testing MQTT framing               ---", testMqttFraming },
        { "--- Testing handshake processing       ---", testHandshake },
  //    { "--- Testing TCP start and term         ---", testStartStop },
        { "--- Testing multiple port support      ---", testMultiplePorts },
        CU_TEST_INFO_NULL
};

/*
 * Array that carries all client-server tcp tests for APIs to CUnit framework.
 */
CU_TestInfo ISM_client_server_tcp_tests[] = {
        { "--- Testing send/receive (1 thread)    ---", testSendReceiveBytes },
        { "--- Testing send/receive (20 threads)  ---", testSendReceiveBytesMulti },
        CU_TEST_INFO_NULL
};

/**
 * WebSockets handshake request
 */
const char * WS_HANDSHAKE_REQUEST =
        "GET ws://127.0.0.1/admin HTTP/1.1\r\n"
        "Origin: null\r\n"
        "Connection: Upgrade\r\n"
        "Host: 127.0.0.1\r\n"
        "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
        "Upgrade: websocket\r\n"
        "Sec-WebSocket-Protocol: monitoring.ism.ibm.com, admin.ism.ibm.com\r\n"
        "Sec-WebSocket-Version: 13\r\n";

/**
  * Beginning of the expected WebSockets handshake response.
  * The order of header fields matches what wstcp.c does.
  */
const char * WS_HANDSHAKE_RESPONSE =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n";

/**
 * Number of client threads to use in multi-threaded client-server tests
 */
#define NUM_THREADS     20

/**
 * Server port for client-server tests
 */
const int SERVER_PORT = 54321;

/**
 * Conditional variable used to notify the test case when
 * the server has started.
 */
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/**
 * Mutex for setting serverStarted to 1 (server started).
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Indicator that the server is started/stopped
 */
static int serverStarted;

/*
 * Indicator that transport->resume has been called
 */
static int transportResumed;

void  testaddMqttFrame(void);
void  testFrameMqtt(void);
void  testSaveArea(void);

int   dummyProtocolHandler(ism_transport_t * transport);

int   emptySender(ism_transport_t * transport, char * buf, int len, int frame, int flags);
int   emptyClose(ism_transport_t * transport, int rc, int clean, const char * reason);
int   emptyReceive(ism_transport_t * transport, char * buf, int len, int protval);
int   emptyClosing(ism_transport_t * transport, int rc, int clean, const char * reason);
int   emptyClosed(ism_transport_t * transport);
void  initTransport(ism_transport_t * transport);
int   ism_transport_addWSFrame(ism_transport_t * transport, char * buffer, int len, int kind);
int addNoFrame(ism_transport_t * transport, char * buffer, int len, int kind);
int   ism_transport_frameWS(ism_transport_t * transport, char * buffer, int pos, int avail, int * used);
int   httpframer(ism_transport_t * transport, char * buffer, int pos, int avail, int * used);
void  ism_transport_wsframe_init(void);
static int echoReceive(ism_transport_t * transport, char * buf, int buflen, int kind);
static int echoProtocol(ism_transport_t * transport);
static void * startServer(void * arg);
static void * clientTest(void * arg);

int sendWithTimeout(int sock, char * buffer, int len, int mils);
int recvWithTimeout(int sock, char * buffer, int len, int mils);

/**
 * Test MQTT framing support
 */
void testMqttFraming(void) {
    testaddMqttFrame();
    testFrameMqtt();
}

/**
 * Test TCP handshake processing
 */
void testHandshake(void) {
    int  rc;
    char buffer[1024];
    int used = 0;
    ism_listener_t * listener = calloc(1, sizeof(ism_listener_t) + sizeof(ism_endstat_t));
    listener->stats = (ism_endstat_t *)(listener+1);
    listener->transmask = TMASK_AnyTrans;
    listener->protomask = PMASK_AnyProtocol;
    listener->port = 12345;
    listener->name = "test";
    ism_transport_t * transport = ism_transport_newTransport(listener, 64,0);

    ism_transport_registerProtocol(NULL, dummyProtocolHandler);
    ism_transport_wsframe_init();

    // Test JMS
    initTransport(transport);
    transport->tobj     = malloc(sizeof(struct ism_transobj));
    memset(transport->tobj, 0x00, sizeof(struct ism_transobj));
    transport->tobj->listener = malloc(sizeof(ism_listener_t));
    memset(transport->tobj->listener, 0x00, sizeof(ism_listener_t));
    transport->tobj->listener->port = 12345;
    transport->state = ISM_TRANST_Opening;

    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0;
    buffer[4] = 55;
    buffer[5] = 40;
    CU_ASSERT((rc=handshake(transport, buffer, 1, sizeof(buffer) - 1, &used)) == 0);
    // printf("jrc=%d\n", rc);
    CU_ASSERT(transport->addframe != NULL && transport->addframe == addJmsFrame);
    CU_ASSERT(transport->frame != NULL && transport->frame == frameJms);

    transport->state = ISM_TRANST_Opening;
    // Test MQTT
    memcpy(buffer+10, "\x10\x10\0\4MQTT", 8);
    CU_ASSERT((rc=handshake(transport, buffer, 10, sizeof(buffer) - 10, &used)) == 0);
    // printf("mrc=%d\n", rc);
    CU_ASSERT(transport->addframe != NULL && transport->addframe == ism_transport_addMqttFrame);
    CU_ASSERT(transport->frame != NULL && transport->frame == frameMqtt);
    memcpy(buffer+10, "\x10\x10\0\6MQIsdp", 10);

    transport->state = ISM_TRANST_Opening;
    CU_ASSERT((rc=handshake(transport, buffer, 10, sizeof(buffer) - 10, &used)) == 0);
    // printf("mrc=%d\n", rc);
    CU_ASSERT(transport->addframe != NULL && transport->addframe == ism_transport_addMqttFrame);
    CU_ASSERT(transport->frame != NULL && transport->frame == frameMqtt);

    transport->state = ISM_TRANST_Opening;
    // Test WebSockets
    strcpy(buffer,
            "GET ws://9.3.179.145:16102/admin HTTP/1.1\r\n"
            "Origin: null\r\n"
            "Connection: Upgrade\r\n"
            "Host: 9.3.179.145:16102\r\n"
            "Sec-WebSocket-Key: sz+ZCx+7QOVLcjZ5XiIkaA==\r\n"
            "Upgrade: websocket\r\n"
            "Sec-WebSocket-Protocol: monitoring.ism.ibm.com, admin.ism.ibm.com\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n");

    int dataLen = strlen(buffer);
    CU_ASSERT((rc = handshake(transport, buffer, 0, dataLen, &used)) == 0);
    // printf("rc=%d\n", rc);
    CU_ASSERT(transport->addframe != NULL && transport->addframe == ism_transport_addNoFrame);
    CU_ASSERT(transport->frame != NULL && transport->frame == ism_transport_httpframer);
    transport->closed = emptyClosed;
    CU_ASSERT(used == 0);

    // Test unknown framer (transport is freed if assertion succeeds)
    memset(buffer, 0x00, sizeof(buffer));
    buffer[0] = 0x55;
    rc = handshake(transport, buffer, 0, sizeof(buffer), &used);
    CU_ASSERT(rc == -1);
    if (rc != -1) {
        ism_transport_freeTransport(transport);
    }

}

/**
 * Test whether MQTT frame is being added correctly for various message sizes
 */
void testaddMqttFrame(void) {
    const int SIZE1 = 50;
    const int SIZE2 = 9876;
    const int SIZE3 = 69001;
    const int SIZE4 = 6004231;
    const int SIZE5 = 268435456;
    int len;
    char command = 1;
    char buffer[512];
    ism_transport_t * transport;

    ism_listener_t * listener = calloc(1, sizeof(ism_listener_t) + sizeof(ism_endstat_t));
    listener->stats = (ism_endstat_t *)(listener+1);
    listener->port = 12345;
    listener->name = "test";
    transport = ism_transport_newTransport(listener, 64, 0);
    initTransport(transport);

    memset(buffer, 'Z', sizeof(buffer));

    len = SIZE1;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 2, len, command) == 2);
    CU_ASSERT(buffer[0] == command);
    CU_ASSERT((unsigned char)buffer[1] == SIZE1);

    len = SIZE2;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 3, len, command) == 3);
    CU_ASSERT(buffer[0] == command);
    CU_ASSERT((unsigned char)buffer[1] == ((SIZE2 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[2] == ((SIZE2 / 128 % 128)));

    len = SIZE3;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 4, len, command) == 4);
    CU_ASSERT(buffer[0] == command);
    CU_ASSERT((unsigned char)buffer[1] == ((SIZE3 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[2] == ((SIZE3 / 128 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[3] == ((SIZE3 / 128 / 128 % 128)));

    len = SIZE4;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 5, len, command) == 5);
    CU_ASSERT(buffer[0] == command);
    CU_ASSERT((unsigned char)buffer[1] == ((SIZE4 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[2] == ((SIZE4 / 128 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[3] == ((SIZE4 / 128 / 128 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[4] == ((SIZE4 / 128 / 128 / 128 % 128)));

    len = SIZE5;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 5, len, command) == -1);
    ism_transport_freeTransport(transport);
}

/**
 * Test MQTT framer processing
 */
void testFrameMqtt(void) {
    ism_transport_t * transport;
    const int SIZE1 = 15;
    const int SIZE2 = 5813;
    const int SIZE3 = 89000;
    const int SIZE4 = 3050411;
    const int BUF_SIZE = 4194304;
    char *buffer = malloc(BUF_SIZE);
    int len, kind, used, save_msgcnt, save_len;

    ism_listener_t * listener = calloc(1, sizeof(ism_listener_t) + sizeof(ism_endstat_t));
    listener->stats = (ism_endstat_t *)(listener+1);
    listener->port = 12345;
    listener->name = "test";
    transport = ism_transport_newTransport(listener, 64, 0);
    initTransport(transport);

    memset(buffer, 'Q', BUF_SIZE);

    // Add MQTT frame
    len = SIZE1;
    kind = 1;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 2, len, kind) == 2);

    // Try incomplete message
    save_msgcnt = transport->read_msg;
    save_len = transport->read_bytes;
    CU_ASSERT(frameMqtt(transport, buffer, 0, SIZE1 - 2, &used) == SIZE1 + 2);
    CU_ASSERT(save_len == transport->read_bytes);

    // Try complete message
    used = 0;
    save_msgcnt = transport->read_msg;
    save_len = transport->read_bytes;
    CU_ASSERT(frameMqtt(transport, buffer, 0, BUF_SIZE, &used) == 0);
    CU_ASSERT(used == (SIZE1 + 2));

    // Add MQTT frame
    memset(buffer, 'Q', SIZE2);
    len = SIZE2;
    kind = 1;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 3, len, kind) == 3);

    // Try incomplete message
    save_msgcnt = transport->read_msg;
    CU_ASSERT(frameMqtt(transport, buffer, 0, SIZE2 - 2, &used) == SIZE2 + 3);

    // Try complete message
    used = 0;
    save_msgcnt = transport->read_msg;
    CU_ASSERT(frameMqtt(transport, buffer, 0, BUF_SIZE, &used) == 0);
    CU_ASSERT(used == (SIZE2 + 3));

    // Add MQTT frame
    memset(buffer, 'Q', SIZE3);
    len = SIZE3;
    kind = 1;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 4, len, kind) == 4);

    // Try incomplete message
    save_msgcnt = transport->read_msg;
    CU_ASSERT(frameMqtt(transport, buffer, 0, SIZE3 - 2, &used) == SIZE3 + 4);
    CU_ASSERT(save_msgcnt == transport->read_msg);

    // Try complete message
    used = 0;
    save_msgcnt = transport->read_msg;
    CU_ASSERT(frameMqtt(transport, buffer, 0, BUF_SIZE, &used) == 0);
    CU_ASSERT(used == (SIZE3 + 4));

    // Add MQTT frame
    memset(buffer, 'Q', SIZE4);
    len = SIZE4;
    kind = 1;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 5, len, kind) == 5);

    // Try incomplete message
    save_msgcnt = transport->read_msg;
    CU_ASSERT(frameMqtt(transport, buffer, 0, SIZE4 - 2, &used) == SIZE4 + 5);
    CU_ASSERT(save_msgcnt == transport->read_msg);

    // Try complete message
    used = 0;
    save_msgcnt = transport->read_msg;
    CU_ASSERT(frameMqtt(transport, buffer, 0, BUF_SIZE, &used) == 0);
    CU_ASSERT(used == (SIZE4 + 5));

    used = 0;
    buffer[4] = 0x80;
    CU_ASSERT(frameMqtt(transport, buffer, 0, 10, &used) == -1);

    free(buffer);
    ism_transport_freeTransport(transport);
}

/**
 * Tests init and start for the TCP server
 */
void testStartStop(void) {
#if 0
    ism_field_t f;

    ism_common_list * indexList = calloc(1, sizeof(ism_common_list));

    ism_common_initTimers();
    ism_common_initUtil();
    ism_common_setTraceLevel(0);

    CU_ASSERT(ism_transport_initTCP() == 0);

    f.type = VT_String;
    f.val.s = "tcp";
    ism_common_setProperty( ism_common_getConfigProperties(), "Transport.0", &f);
    ism_common_setProperty( ism_common_getConfigProperties(), "Transport.1", &f);
    ism_common_setProperty( ism_common_getConfigProperties(), "Transport.2", &f);
    f.type = VT_Integer;
    f.val.i = 12345;
    ism_common_setProperty( ism_common_getConfigProperties(), "Port.0", &f);
    f.val.i = 22346;
    ism_common_setProperty( ism_common_getConfigProperties(), "Port.1", &f);
    f.val.i = 32347;
    ism_common_setProperty( ism_common_getConfigProperties(), "Port.2", &f);

    ism_common_list_init(indexList, 0, NULL);
    ism_common_list_insert_tail(indexList, (void *)0);
    ism_common_list_insert_tail(indexList, (void *)1);
    ism_common_list_insert_tail(indexList, (void *)2);

    CU_ASSERT(ism_transport_startTCP(indexList) == 0);
    CU_ASSERT(ism_transport_startTCP(indexList) == -1);
    ism_transport_termTCP();
    CU_ASSERT(ism_transport_startTCP(indexList) == 0);

    f.val.i = 73345;
    ism_common_setProperty( ism_common_getConfigProperties(), "Port.0", &f);
    f.val.i = 83346;
    ism_common_setProperty( ism_common_getConfigProperties(), "Port.1", &f);
    f.val.i = 93347;
    ism_common_setProperty( ism_common_getConfigProperties(), "Port.2", &f);

    CU_ASSERT(ism_transport_startTCP(indexList) == -1);
    ism_transport_termTCP();
    CU_ASSERT(ism_transport_startTCP(indexList) == 0);

    ism_transport_termTCP();

    ism_common_list_destroy(indexList);
    free(indexList);

    ism_common_stopTimers();
#endif
}


/**
 * Tests receiveBytes and sendBytes functions along with connectionListener, ioReader and ioListener
 */
void testSendReceiveBytes(void) {
    pthread_t tid;

    /*
     * Start the server in a separate thread on port SERVER_PORT
     * Connect to that port (localhost connection)
     * Send WS-framed messages
     *   a. Set up receive function in transport to echo messages back
     *   b. Send 1000 small, < snd/rcv buffer, verify receipt
     *   c. Send 5 small, exceed snd/rcv buffer after 3, verify receipt
     *   d. Send 100 large, each > buffer to force 2+ resizes, verify receipt
     *   e. Send 2 good, 2 bad and 2 good messages, verify receipt and failures
     */
    serverStarted = 0;
    CU_ASSERT(pthread_create(&tid, NULL, startServer, NULL) == 0);
    pthread_mutex_lock(&mutex);
    while (!serverStarted) {                /* BEAM suppression: infinite loop */
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    int64_t input = 'Z';
    clientTest((void*)input);

    pthread_kill(tid, SIGINT);

    CU_ASSERT(pthread_join(tid, NULL) == 0);
}

void testSendReceiveBytesMulti(void) {
    pthread_t tid;
    pthread_t client_tid[NUM_THREADS];
    int i;

    /*
     * Start the server in a separate thread on port SERVER_PORT
     * Connect to that port (localhost connection)
     * Send WS-framed messages in 20 different threads using different payloads
     *   a. Set up receive function in transport to echo messages back
     *   b. Send 1000 small, < snd/rcv buffer, verify receipt
     *   c. Send 5 small, exceed snd/rcv buffer after 3, verify receipt
     *   d. Send 100 large, each > buffer to force 2+ resizes, verify receipt
     *   e. Send 2 good, 2 bad and 2 good messages, verify receipt and failures
     */

    serverStarted = 0;
    CU_ASSERT(pthread_create(&tid, NULL, startServer, NULL) == 0);
    pthread_mutex_lock(&mutex);
    while (!serverStarted) {                  /* BEAM suppression: infinite loop */
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    for (i = 0; i < NUM_THREADS; i++) {
        int64_t input = 'A' + i;
        CU_ASSERT(pthread_create(&client_tid[i], NULL, clientTest, (void*)input) == 0);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        CU_ASSERT(pthread_join(client_tid[i], NULL) == 0);
    }

    pthread_kill(tid, SIGINT);
    CU_ASSERT(pthread_join(tid, NULL) == 0);
}

/**
 * Dummy protocol handler matching "jms", "tcpjms", "mqtt-tcp" and "monitoring.ism.ibm.com"
 * @param transport A pointer to a transport object to check
 * @return 0, if match found, 1 otherwise
 */
int dummyProtocolHandler(ism_transport_t * transport) {
    int rc = !((strcmp(transport->protocol, "jms") == 0) ||
            (strcmp(transport->protocol, "tcpjms") == 0) ||
            (strcmp(transport->protocol, "mqtt-tcp") == 0) ||
            (strcmp(transport->protocol, "monitoring.ism.ibm.com") == 0));

    return rc;
}

/*
 * Copy the input buffer to the output buffer, and send back.  This function is
 * used by the echo protocol.
 * @param transport A pointer to a transport instance
 * @param buf       A pointer to a buffer with the data to send
 * @param buflen    Number of bytes of data to send
 * @param kind      Not used
 * @return 0
 */
static int echoReceive(ism_transport_t * transport, char * buf, int buflen, int kind) {
    transport->send(transport, (char *)buf, buflen, kind, 0);
    return 0;
}

/*
 * Set up receive and closing protocols for a transport.
 * @param transport A pointer to a transport instance to set protocols for
 * @return 0
 */
static int echoProtocol(ism_transport_t * transport) {
    transport->receive = echoReceive;
    transport->closing = emptyClosing;
    transport->closed = emptyClosed;
    return 0;
}

/**
 * Function containing start/stop for TCP server.
 * Once the server is started, it waits for SIGINT before terminating it.
 * @param arg Input parameter - not used
 * @return NULL
 */
static void * startServer(void * arg) {
    sigset_t sigs;
    siginfo_t info;
    ism_field_t f;
    ism_common_list * indexList = calloc(1, sizeof(ism_common_list));

    /* Add SIGINT to the thread signal mask to enable termination by signal */
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigs, 0);

    ism_log_init();

    ism_transport_registerProtocol(NULL, echoProtocol);
    ism_transport_wsframe_init();

    ism_transport_initTCP();

    f.type = VT_String;
    f.val.s = "tcp";
    ism_common_setProperty( ism_common_getConfigProperties(), "Transport.0", &f);
    f.type = VT_Integer;
    f.val.i = SERVER_PORT;
    ism_common_setProperty( ism_common_getConfigProperties(), "Port.0", &f);

    ism_transport_start();

    pthread_mutex_lock(&mutex);
    serverStarted = 1;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    /* Wait for SIGINT */
    sigwaitinfo(&sigs, &info);

    pthread_mutex_lock(&mutex);
    serverStarted = 0;
    pthread_mutex_unlock(&mutex);

    ism_transport_termTCP();

    ism_common_list_destroy(indexList);
    free(indexList);

    ism_common_stopTimers();

    return NULL;
}

/**
 * Client-side test for receiveBytes
 * @param arg  A character for fill the payload with
 * @return NULL
 */
static void * clientTest(void * arg) {
    char fill = (char)(int64_t)arg;

    struct sockaddr_in server;
    const int BUFFER_SIZE = 102400;
    char *buffer = malloc(BUFFER_SIZE);
    int nRead;

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CU_ASSERT(sock != -1);
    if (sock == -1) {
        free(buffer);
        return NULL;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Attempt empty connect/close to ensure nothing crashes/fails
    CU_ASSERT(connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0);
    close(sock);

    // Send WS handshake
    CU_ASSERT((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != -1);
    CU_ASSERT(connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0);

    // Switch to non-blocking mode
    int x = fcntl(sock, F_GETFL, 0);            /* BEAM suppression: num args*/
    fcntl(sock, F_SETFL, x | O_NONBLOCK);

    // Ensure WebSockets connection is established
    strcpy(buffer, WS_HANDSHAKE_REQUEST);

    CU_ASSERT(sendWithTimeout(sock, buffer, strlen(buffer), 200) == strlen(buffer));
    CU_ASSERT((nRead = recvWithTimeout(sock, buffer, BUFFER_SIZE, 1000)) > 0);

    CU_ASSERT(strstr(buffer, "HTTP/1.1 101 Switching Protocols\r\n") != NULL);
    CU_ASSERT(strstr(buffer, "Upgrade: websocket\r\n") != NULL);
    CU_ASSERT(strstr(buffer, "Connection: Upgrade\r\n") != NULL);

    const int MSG_TYPE = 0x82;  // binary, not fragmented

    /*
     * a. Send 1000 small, < rcv buffer, verify receipt
     */
    int msgLen = 2;
    const unsigned int MASK = 58976521;
    char data[] = { fill, 0 };
    ism_transport_addWSFrame(NULL, buffer + 2, msgLen, MSG_TYPE);
    buffer[0] |= 0x80;  // Not cont frame
    buffer[1] |= 0x80;  // Masked
    memcpy(buffer+2, &MASK, 4);
    char * mask = buffer + 2;
    buffer[6] = data[0] ^ mask[0]; // Masked payload
    buffer[7] = data[1] ^ mask[1];

    // Send 2 byte payload + 2-byte header + 4-byte mask
    int i, count = 1000;
    for (i = 0; i < count; i++) {
        CU_ASSERT(sendWithTimeout(sock, buffer, 8, 200) == 8);
    }

    // Receive 2 byte payload + 2-byte header, no mask
    for (i = 0; i < count; i++) {
        memset(buffer, 0x00, BUFFER_SIZE);
        CU_ASSERT(recvWithTimeout(sock, buffer, msgLen + 2, 200) == (msgLen + 2));
        CU_ASSERT((unsigned char)buffer[0] == MSG_TYPE && (unsigned char)buffer[1] == msgLen);
        CU_ASSERT((unsigned char)buffer[2] == data[0] && (unsigned char)buffer[3] == data[1]);
    }

    /*
     * c. Send 5 small, exceed snd and rcv buffers after 3, verify receipt
     * Each message should be 10900 bytes long (all 'Z's)
     */
    msgLen = 10900;
    ism_transport_addWSFrame(NULL, buffer + 4, msgLen, MSG_TYPE);
    buffer[0] |= 0x80;  // Not cont frame
    buffer[1] |= 0x80;  // Masked
    memcpy(buffer + 4, &MASK, 4);
    mask = buffer + 4;
    for (i = 0; i < msgLen; i++) {
        buffer[8 + i] = fill ^ mask[i & 3]; // Masked payload
    }

    count = 5;
    // Send 10900 byte payload + 4-byte header + 4-byte mask
    for (i = 0; i < count; i++) {
        CU_ASSERT(sendWithTimeout(sock, buffer, msgLen + 8, 500) == msgLen + 8);
    }

    // Receive 10900 byte payload + 4-byte header, no mask
    for (i = 0; i < count; i++) {
        memset(buffer, 0x00, BUFFER_SIZE);
        CU_ASSERT(recvWithTimeout(sock, buffer, msgLen + 4, 200) == (msgLen + 4));
        CU_ASSERT((unsigned char)buffer[0] == MSG_TYPE &&
                (unsigned char)buffer[1] == 126 &&
                memcmp(buffer + 2, &msgLen, 2));
        CU_ASSERT(strspn(buffer + 4, &fill) == msgLen);
    }

    /*
     *   d. Send 100 large, each > buffer to force 2+ resizes, verify receipt
     */
    msgLen = 50000;

    // Send 50000 byte payload + 4-byte header + 4-byte mask
    // Receive 50000 byte payload + 4-byte header, no mask
    count = 100;
    for (i = 0; i < count; i++) {
        CU_ASSERT(ism_transport_addWSFrame(NULL, buffer + 4, msgLen, MSG_TYPE) == 4);
        buffer[0] |= 0x80;  // Not cont frame
        buffer[1] |= 0x80;  // Masked
        memcpy(buffer + 4, &MASK, 4);
        mask = buffer + 4;
        int j;
        for (j = 0; j < msgLen; j++) {
            buffer[8 + j] = fill ^ mask[j & 3]; // Masked payload
        }

        CU_ASSERT(sendWithTimeout(sock, buffer, msgLen + 8, 200) == msgLen + 8);
        memset(buffer, 0x00, BUFFER_SIZE);
        CU_ASSERT(recvWithTimeout(sock, buffer, msgLen + 4, 200) == (msgLen + 4));
        CU_ASSERT((unsigned char)buffer[0] == MSG_TYPE &&
                (unsigned char)buffer[1] == 126 &&
                memcmp(buffer + 2, &msgLen, 2));
        CU_ASSERT(strspn(buffer + 4, &fill) == msgLen);
    }

    /*
     *   e. Send 2 good, 2 bad and 2 good messages, verify receipt and failures
     */
    msgLen = 2;
    ism_transport_addWSFrame(NULL, buffer + 2, msgLen, MSG_TYPE);
    buffer[0] |= 0x80;  // Not cont frame
    buffer[1] |= 0x80;  // Masked
    memcpy(buffer+2, &MASK, 4);
    mask = buffer + 2;
    buffer[6] = data[0] ^ mask[0]; // Masked payload
    buffer[7] = data[1] ^ mask[1];

    // Send 2 byte payload + 2-byte header + 4-byte mask
    for (i = 0; i < count; i++) {
        CU_ASSERT(sendWithTimeout(sock, buffer, 8, 200) == 8);
    }

    // Receive 2 byte payload + 2-byte header, no mask
    for (i = 0; i < count; i++) {
        memset(buffer, 0x00, BUFFER_SIZE);
        CU_ASSERT(recvWithTimeout(sock, buffer, msgLen + 2, 200) == (msgLen + 2));
        CU_ASSERT((unsigned char)buffer[0] == MSG_TYPE && (unsigned char)buffer[1] == msgLen);
        CU_ASSERT((unsigned char)buffer[2] == data[0] && (unsigned char)buffer[3] == data[1]);
    }

    ism_transport_addWSFrame(NULL, buffer + 2, msgLen, MSG_TYPE);
    buffer[0] |= 0x80;  // Not cont frame
    buffer[1] |= 0x00;  // Not masked
    buffer[2] = data[0];
    buffer[3] = data[1];

    /*
     * Send bad WS messages with 2 byte payload + 2-byte header,
     * should be no response
     */
    count = 2;
    for (i = 0; i < count; i++) {
        CU_ASSERT(sendWithTimeout(sock, buffer, 4, 200) == 4);
    }

    // Now send 2 good messages - should not get any response back
    ism_transport_addWSFrame(NULL, buffer + 2, msgLen, MSG_TYPE);
    buffer[0] |= 0x80;  // Not cont frame
    buffer[1] |= 0x80;  // Masked
    memcpy(buffer+2, &MASK, 4);
    mask = buffer + 2;
    buffer[6] = data[1] ^ mask[0]; // Masked payload
    buffer[7] = data[0] ^ mask[1];

    // Send 2 byte payload + 2-byte header + 4-byte mask
    for (i = 0; i < count; i++) {
        CU_ASSERT(sendWithTimeout(sock, buffer, 8, 200) == 8);
    }

    /*
     * Should not receive anything back, WS framer should force connection close
     * after the bad message
     */
    for (i = 0; i < count; i++) {
        memset(buffer, 0x00, BUFFER_SIZE);
        CU_ASSERT(recvWithTimeout(sock, buffer, msgLen + 2, 200) == 0);
    }

    close(sock);

    free(buffer);

    return NULL;
}

/**
 * Dummy function for transport->closing
 */
int emptyClosing(ism_transport_t * transport, int rc, int clean, const char * reason) {
    return 0;
}
/**
 * Dummy function for transport->closed
 */
int emptyClosed(ism_transport_t * transport) {
    return 0;
}

/**
 * Dummy function for transport->resume
 */
int emptyResume(ism_transport_t * transport, void * userdata) {
    transportResumed = 1;
    return 0;
}

/**
 * Send data to a socket with timeout.
 * @param sock   A socket file descriptor
 * @param buffer A pointer to a buffer with data
 * @param len    Number of bytes to send
 * @param mils   A timeout in milliseconds (maximum time with socket
 *               not ready for writing)
 * @return The number of bytes written or -1 if failure occurred
 */
int sendWithTimeout(int sock, char * buffer, int len, int mils) {
    int nWritten = 0;
    fd_set writeFDs;
    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = mils * 1000;

    FD_ZERO(&writeFDs);
    FD_SET(sock, &writeFDs);

    while (nWritten < len) {
        int rc = select(sock + 1, NULL, &writeFDs, NULL, &timeout);
        if (rc < 0 && (errno != EAGAIN)) {
            return -1;
        }
        if (rc == 0) {
            break;
        }

        int n = send(sock, buffer + nWritten, len - nWritten, 0);
        if (n < 0) {
            return -1;
        }

        nWritten += n;
    }

    return nWritten;
}

/**
 * Receive data from a socket with timeout.
 * @param sock   A socket file descriptor
 * @param buffer A pointer to a buffer where to store the data
 * @param len    Amount of data to be received (should not exceed buffer size)
 * @param mils   A timeout in milliseconds (maximum time with not new data
 *               available for reading)
 * @return The number of bytes read or -1 if failure occurred
 */
int recvWithTimeout(int sock, char * buffer, int len, int mils) {
    int nRead = 0;
    fd_set readFDs;
    struct timeval timeout;

    timeout.tv_sec = 0;
    timeout.tv_usec = mils * 1000;

    FD_ZERO(&readFDs);
    FD_SET(sock, &readFDs);

    while (nRead < len) {
        int rc = select(sock + 1, &readFDs, NULL, NULL, &timeout);
        if (rc < 0 && (errno != EAGAIN)) {
            return -1;
        }
        if (rc == 0) {
            break;
        }

        int n = recv(sock, buffer + nRead, len - nRead, 0);
        if (n < 0) {
            return -1;
        }

        nRead += n;
    }

    return nRead;
}


/**
 * Test configuration parsing for ports/transports.
 * Set transport to TCP for ports 12345, 22345 and 32345.
 * Set transport to Test for ports 11111, 22222 and 33333.
 * Set TcpThreads to 12.
 *
 * Call ism_transport_initTCP and ism_transport_startTCP,
 * ensure that port structure and other global variables
 * are properly set.
 */
void testMultiplePorts(void) {
    ism_field_t f;
    int sock;
    struct sockaddr_in server;

    const short TCP_PORT0 = 12345;
    const short TCP_PORT2 = 32345;
    const short BAD_PORT0 = 11111;
    const short TCP_PORT1 = 22222;
    const short BAD_PORT2 = 33333;
    const int TCP_THREADS = 5;

    ism_common_setTraceLevel(0);

    ism_common_initUtil();
    ism_config_init();


    f.type = VT_Integer;
    f.val.i = TCP_PORT0;
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Port.E0", &f);
    f.val.i = TCP_PORT1;
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Port.E1", &f);
    f.val.i = BAD_PORT0;
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Port.E2", &f);
    f.val.i = TCP_PORT1;
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Port.E3", &f);
    f.val.i = TCP_PORT2;
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Port.E4", &f);
    f.val.i = BAD_PORT2;
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Port.E5", &f);
    f.type = VT_String;
    f.val.s = "E0";
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Name.E0", &f);
    f.val.s = "E1";
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Name.E1", &f);
    f.val.s = "E2";
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Name.E2", &f);
    f.val.s = "E3";
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Name.E3", &f);
    f.val.s = "E4";
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Name.E4", &f);
    f.val.s = "E5";
    ism_common_setProperty( ism_common_getConfigProperties(), "Endpoint.Name.E5", &f);

    f.type = VT_Integer;
    f.val.i = TCP_THREADS;
    ism_common_setProperty( ism_common_getConfigProperties(), "TcpThreads", &f);

    ism_transport_init();
    ism_transport_start();
    ism_transport_startMessaging();

    // ism_transport_printListeners(NULL);

    /* Try connecting to ports 12345, 22345 and 32345 */
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CU_ASSERT_FATAL(sock != -1);
    server.sin_family = AF_INET;
    server.sin_port = htons(TCP_PORT0);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Attempt empty connect/close to ensure nothing crashes/fails
    CU_ASSERT_FATAL(connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0);
    close(sock);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CU_ASSERT_FATAL(sock != -1);
    server.sin_family = AF_INET;
    server.sin_port = htons(TCP_PORT1);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    // Attempt empty connect/close to ensure nothing crashes/fails
    CU_ASSERT_FATAL(connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0);
    close(sock);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CU_ASSERT_FATAL(sock != -1);
    server.sin_family = AF_INET;
    server.sin_port = htons(TCP_PORT2);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    // Attempt empty connect/close to ensure nothing crashes/fails
    CU_ASSERT_FATAL(connect(sock, (struct sockaddr *)&server, sizeof(server)) == 0);
    close(sock);

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CU_ASSERT_FATAL(sock != -1);
    server.sin_family = AF_INET;
    server.sin_port = htons(666);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    // Attempt empty connect/close to ensure nothing crashes/fails
    CU_ASSERT_FATAL(connect(sock, (struct sockaddr *)&server, sizeof(server)) == -1);
    close(sock);


    ism_transport_term();

}
