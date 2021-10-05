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
 * File: api_unit_test.c
 * Component: server
 * SubComponent: server_ismc
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#include <ismutil.h>
#include <ismc_p.h>
#include <api_unit_test.h>

/**
 * Test array for API tests.
 */
CU_TestInfo ISM_api_tests[] = {
		{ "--- Testing connection-related API     ---", testConnection    },
		{ "--- Testing producer-related API       ---", testProducer      },
		{ "--- Testing consumer-related API       ---", testConsumer      },
		{ "--- Testing subscription-related API   ---", testSubscribe     },
		{ "--- Testing receive-related API        ---", testReceive       },
		{ "--- Testing error listener             ---", testErrorListener },
		CU_TEST_INFO_NULL
};

static const char * CID = "API_UNIT_TEST_CLIENT_ID";

void errorListener(int rc, const char * reason, ismc_connection_t * connect, ismc_session_t * session, void * userdata);

int sendMessage(ism_timer_t key, ism_time_t timestamp, void * userdata);

void testProducer(void) {

	ismc_connection_t * conn;
	ismc_session_t * session;
	ismc_producer_t * prod;
	char * server = getenv("ISMServer");
	char * client_port = getenv("ISMPort");
	char buf[512];
	ismc_destination_t * dest1 = ismc_createTopic("test1");

	CU_ASSERT(ismc_createProducer(NULL, dest1) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_NullPointer);
	CU_ASSERT(ismc_createProducer((ismc_session_t *)"test1234", dest1) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_ObjectNotValid);

	conn = ismc_createConnection();
	CU_ASSERT_FATAL(conn != NULL);
	if (server) {
		ismc_setStringProperty(conn, "Server", server);
	}

	if (client_port) {
		ismc_setStringProperty(conn, "Port", client_port);
	}
	ismc_setStringProperty(conn, "Protocol", "@@@");
	ismc_setStringProperty(conn, "ClientID", CID);

	CU_ASSERT_FATAL(ismc_connect(conn) == 0);

	session = ismc_createSession(conn, 0, SESSION_AUTO_ACKNOWLEDGE);
	CU_ASSERT_FATAL(session != NULL);

	prod = ismc_createProducer(session, dest1);
	CU_ASSERT_FATAL(prod != NULL);
	CU_ASSERT(ismc_closeProducer(prod) == 0);
	ismc_free(prod);

	ismc_closeConnection(conn);

	ismc_free(conn);
	ismc_free(session);
}

void testConsumer(void) {
	ismc_connection_t * conn;
	ismc_session_t * session;
	ismc_consumer_t * cons;
	ismc_producer_t * prod;
	ism_timer_t       key;
	ismc_message_t * message;
	char * server = getenv("ISMServer");
	char * client_port = getenv("ISMPort");
	char buf[512];
	ismc_destination_t * dest1 = ismc_createTopic("test1");

	CU_ASSERT(ismc_createConsumer(NULL, dest1, NULL, 0) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_NullPointer);
	CU_ASSERT(ismc_createConsumer((ismc_session_t *)"test1234", dest1, NULL, 0) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_ObjectNotValid);

	conn = ismc_createConnection();
	CU_ASSERT_FATAL(conn != NULL);
	if (server) {
		ismc_setStringProperty(conn, "Server", server);
	}

	if (client_port) {
		ismc_setStringProperty(conn, "Port", client_port);
	}
	ismc_setStringProperty(conn, "Protocol", "@@@");
	ismc_setStringProperty(conn, "ClientID", CID);

	CU_ASSERT_FATAL(ismc_connect(conn) == 0);

	session = ismc_createSession(conn, 0, SESSION_AUTO_ACKNOWLEDGE);
	CU_ASSERT_FATAL(session != NULL);

	cons = 	ismc_createConsumer(session, dest1, NULL, 0);
	CU_ASSERT_FATAL(cons != NULL);
	CU_ASSERT_FATAL(cons->messages != NULL);
	CU_ASSERT(cons->disableACK == session->disableACK);

	prod = ismc_createProducer(session, dest1);

	ismc_startConnection(conn);

	key = ism_common_setTimerOnce(ISM_TIMER_HIGH,
			sendMessage,
			prod,
			1000000000);
	CU_ASSERT(ismc_receive(cons, -1, &message) == ISMRC_NoMsgAvail);
	CU_ASSERT(message == NULL);
	ism_common_cancelTimer(key);

	key = ism_common_setTimerOnce(ISM_TIMER_HIGH,
			sendMessage,
			prod,
			1000000000);
	CU_ASSERT(ismc_receive(cons, 0, &message) == ISMRC_OK);
	CU_ASSERT(message != NULL);

	key = ism_common_setTimerOnce(ISM_TIMER_HIGH,
			sendMessage,
			prod,
			1000000000);
	CU_ASSERT(ismc_receive(cons, 2000, &message) == ISMRC_OK);
	CU_ASSERT(message != NULL);

	key = ism_common_setTimerOnce(ISM_TIMER_HIGH,
			sendMessage,
			prod,
			4000000000);
	CU_ASSERT(ismc_receive(cons, 3000, &message) == ISMRC_TimeOut);
	CU_ASSERT(message == NULL);
	ism_common_cancelTimer(key);

	CU_ASSERT(ismc_closeConsumer(cons) == 0);
	ismc_free(cons);

	ismc_closeConnection(conn);

	ismc_free(conn);
	ismc_free(session);
	ismc_free(prod);
}

void testSubscribe(void) {
	ismc_connection_t * conn;
	ismc_session_t * session;
	ismc_consumer_t * cons;
	char * server = getenv("ISMServer");
	char * client_port = getenv("ISMPort");
	char buf[512];

	CU_ASSERT(ismc_subscribe(NULL, "topic2", "subname", NULL, 0) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_NullPointer);
	CU_ASSERT(ismc_subscribe((ismc_session_t *)"test1234", "topic2", "subname", NULL, 0) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_ObjectNotValid);

	conn = ismc_createConnection();
	CU_ASSERT_FATAL(conn != NULL);
	if (server) {
		ismc_setStringProperty(conn, "Server", server);
	}

	if (client_port) {
		ismc_setStringProperty(conn, "Port", client_port);
	}
	ismc_setStringProperty(conn, "Protocol", "@@@");
	ismc_setStringProperty(conn, "ClientID", CID);

	CU_ASSERT_FATAL(ismc_connect(conn) == 0);

	session = ismc_createSession(conn, 0, SESSION_AUTO_ACKNOWLEDGE);
	CU_ASSERT_FATAL(session != NULL);

	CU_ASSERT(ismc_subscribe_with_options(session, "topic2", "subname", NULL, 0, -1) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_ArgNotValid);

	cons = ismc_subscribe(session, "topic2", "subname", NULL, 0);
	CU_ASSERT_FATAL(cons != NULL);
	CU_ASSERT(cons->disableACK == session->disableACK);
	CU_ASSERT(ismc_closeConsumer(cons) == 0);
	CU_ASSERT(ismc_unsubscribe(session, "subname") == 0);
	ismc_free(cons);

	session->disableACK = 0;
	cons = ismc_subscribe(session, "topic2", "subname", NULL, 0);
	CU_ASSERT_FATAL(cons != NULL);
	CU_ASSERT(cons->disableACK == session->disableACK);
	CU_ASSERT(cons->domain == ismc_Topic);
	CU_ASSERT(ismc_closeConsumer(cons) == 0);
	CU_ASSERT(ismc_unsubscribe(session, "subname") == 0);
	ismc_free(cons);

	session->disableACK = 1;
	cons = ismc_subscribe(session, "topic2", "subname", NULL, 0);
	CU_ASSERT_FATAL(cons != NULL);
	CU_ASSERT(cons->disableACK == session->disableACK);
	CU_ASSERT(ismc_closeConsumer(cons) == 0);
	CU_ASSERT(ismc_unsubscribe(session, "subname") == 0);
	ismc_free(cons);

	CU_ASSERT(ismc_subscribe(session, NULL, "subname", NULL, 0) == NULL);
	CU_ASSERT(ismc_subscribe(session, "topic2", NULL, NULL, 0) == NULL);
	CU_ASSERT(ismc_subscribe(session, NULL, NULL, NULL, 0) == NULL);
	CU_ASSERT(ismc_getLastError(buf, sizeof(buf)) == ISMRC_NoDestination);

	cons = ismc_subscribe_with_options(session, "topic2", "subname", NULL, 0, 12603);
	CU_ASSERT_FATAL(cons != NULL);
	CU_ASSERT(cons->disableACK == session->disableACK);
	CU_ASSERT(ismc_closeConsumer(cons) == 0);
	CU_ASSERT(ismc_unsubscribe(session, "subname") == 0);
	ismc_free(cons);

	CU_ASSERT(ismc_unsubscribe(NULL, "subname") == ISMRC_NullPointer);
	CU_ASSERT(ismc_unsubscribe(session, NULL) == ISMRC_NoDestination);

	ismc_closeConnection(conn);

	ismc_free(conn);
	ismc_free(session);
}

void testConnection(void) {
	ismc_connection_t * conn;
	char * server = getenv("ISMServer");
	char * client_port = getenv("ISMPort");

	conn = ismc_createConnection();
	CU_ASSERT_FATAL(conn != NULL);
	CU_ASSERT(conn->h.id == OBJID_Connection);

	if (server) {
		ismc_setStringProperty(conn, "Server", server);
	}

	if (client_port) {
		ismc_setStringProperty(conn, "Port", client_port);
	}
	ismc_setStringProperty(conn, "Protocol", "@@@");

	/* Try to connect without client ID */
	CU_ASSERT(ismc_connect(conn) == ISMRC_ClientIDRequired);
	/* Try to connect without connection object */
	CU_ASSERT(ismc_connect(NULL) == ISMRC_NullPointer);
	/* Try to connect with bad connection object */
	CU_ASSERT(ismc_connect((ismc_connection_t *)"test12345") == ISMRC_ObjectNotValid);

	ismc_setStringProperty(conn, "ClientID", CID);
	CU_ASSERT_FATAL(ismc_connect(conn) == 0);
	CU_ASSERT(ismc_startConnection(NULL) == ISMRC_NullPointer);
	CU_ASSERT(ismc_startConnection((ismc_connection_t *)"test12345") == ISMRC_ObjectNotValid);

	CU_ASSERT(ismc_stopConnection(NULL) == ISMRC_NullPointer);
	CU_ASSERT(ismc_stopConnection((ismc_connection_t *)"test12345") == ISMRC_ObjectNotValid);
	ism_common_setError(0);
	CU_ASSERT_FATAL(ismc_startConnection(conn) == 0);
	CU_ASSERT(ismc_stopConnection(conn) == 0);

	ismc_closeConnection(conn);

	ismc_free(conn);
}

void testReceive(void) {

}

void testErrorListener(void) {
	ismc_connection_t * conn;
	char * server = getenv("ISMServer");
	char * client_port = getenv("ISMPort");
	char * userdata = "testdata";

	conn = ismc_createConnection();
	CU_ASSERT_FATAL(conn != NULL);

	ismc_setStringProperty(conn, "ClientID", CID);

	if (server) {
		ismc_setStringProperty(conn, "Server", server);
	}

	if (client_port) {
		ismc_setStringProperty(conn, "Port", client_port);
	}
	ismc_setStringProperty(conn, "Protocol", "@@@");

	CU_ASSERT(ismc_setErrorListener(conn, errorListener, userdata) == NULL);
	CU_ASSERT(ismc_getLastError(NULL, 0) == ISMRC_NotConnected);

	CU_ASSERT(ismc_setErrorListener(NULL, errorListener, userdata) == NULL);
	CU_ASSERT(ismc_getLastError(NULL, 0) == ISMRC_NullPointer);

	CU_ASSERT(ismc_connect(conn) == 0);

	CU_ASSERT(ismc_setErrorListener(conn, errorListener, userdata) == NULL);
	CU_ASSERT(ismc_setErrorListener(conn, errorListener, userdata) == errorListener);
	CU_ASSERT(ismc_setErrorListener(conn, NULL, userdata) == errorListener);
	CU_ASSERT(ismc_setErrorListener(conn, NULL, userdata) == NULL);

	ismc_closeConnection(conn);
	ismc_free(conn);
}

void errorListener(int rc, const char * reason, ismc_connection_t * connect, ismc_session_t * session, void * userdata) {

}

int sendMessage(ism_timer_t key, ism_time_t timestamp, void * userdata) {
	ismc_producer_t * prod = userdata;

	ismc_message_t * bmsg = ismc_createMessage(prod->session, MTYPE_BytesMessage);
	ismc_send(prod, bmsg);
	ism_common_cancelTimer(key);

	return 0;
}
