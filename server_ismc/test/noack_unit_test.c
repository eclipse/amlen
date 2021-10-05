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
 * File: noack_unit_test.c
 * Component: client
 * SubComponent: client_ismc
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#include <ismutil.h>
#include <ismc_p.h>
#include <noack_unit_test.h>

/**
 * Test array for destination tests without acknowledgment (noack).
 */
CU_TestInfo ISM_noack_tests[] = {
		{ "--- Testing topic with no ack          ---", testTopicNoAck },
		{ "--- Testing queue with no ack          ---", testQueueNoAck },
		CU_TEST_INFO_NULL
};

static const char         * CID     = "testClientID";

/**
 * Verifies how DisableACK property is inherited from connection, session and topic
 * to the message consumer and whether the engine restriction (no acks for topics)
 * is enforced.
 */
void testTopicNoAck(void) {
	ism_field_t field;
	ismc_connection_t * conn, * conn2;
	ismc_session_t * session, * session2;
	ismc_destination_t * topic, * topic2;
	ismc_producer_t * prod;
	ismc_consumer_t * cons, * cons1, * cons2, * cons22;

	char * server = getenv("ISMServer");
	char * client_port = getenv("ISMPort");

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

	field.type = VT_Boolean;
	field.val.i = 1;
	ismc_setProperty(conn, "DisableACK", &field);
	ismc_setStringProperty(conn, "protocol", "dummy");

	CU_ASSERT(ismc_getProperty(conn, "DisableACK", &field) == 0);
	CU_ASSERT(field.type == VT_Boolean && field.val.i == 1);

	CU_ASSERT_FATAL(ismc_connect(conn) == 0);

	session = ismc_createSession(conn, 0, SESSION_AUTO_ACKNOWLEDGE);
	CU_ASSERT(ismc_getProperty(session, "DisableACK", &field) == 0);
	CU_ASSERT(field.type == VT_Boolean && field.val.i == 1);

	topic = ismc_createTopic("topic1");
	CU_ASSERT_FATAL(topic != NULL);
	CU_ASSERT(ismc_getProperty(topic, "DisableACK", &field) == ISMRC_PropertyNotFound);

	prod = ismc_createProducer(session, topic);
	CU_ASSERT_FATAL(prod != NULL);
	CU_ASSERT(ismc_getProperty(prod, "DisableACK", &field) == 0);
	CU_ASSERT(field.type == VT_Boolean && field.val.i == 1);

	cons = ismc_createConsumer(session, topic, NULL, 0);
	CU_ASSERT_FATAL(cons != NULL);
	CU_ASSERT(ismc_getProperty(cons, "DisableACK", &field) == 0);
	CU_ASSERT(field.type == VT_Boolean && field.val.i == 1);

	field.type = VT_Boolean;
	field.val.i = 0;
	ismc_setProperty(topic, "DisableACK", &field);
	CU_ASSERT(ismc_getProperty(topic, "DisableACK", &field) == 0);
	CU_ASSERT(field.type == VT_Boolean && field.val.i == 0);

	cons1 = ismc_createConsumer(session, topic, NULL, 0);
	CU_ASSERT_FATAL(cons1 != NULL);
	CU_ASSERT(ismc_getProperty(cons1, "DisableACK", &field) == 0);
	CU_ASSERT(field.type == VT_Boolean && field.val.i == 0);

	ismc_closeConnection(conn);

	conn2 = ismc_createConnection();
	CU_ASSERT_FATAL(conn2 != NULL);
	CU_ASSERT(ismc_getProperty(conn2, "DisableACK", &field) == ISMRC_PropertyNotFound);

	if (server) {
		ismc_setStringProperty(conn2, "Server", server);
	}
	if (client_port) {
		ismc_setStringProperty(conn2, "Port", client_port);
	}
	ismc_setStringProperty(conn2, "protocol", "dummy");
	ismc_setStringProperty(conn2, "ClientID", CID);

	CU_ASSERT_FATAL(ismc_connect(conn2) == 0);

	session2 = ismc_createSession(conn2, 0, SESSION_AUTO_ACKNOWLEDGE);
	CU_ASSERT(ismc_getProperty(session2, "DisableACK", &field) == ISMRC_PropertyNotFound);

	topic2 = ismc_createTopic("topic2");
	CU_ASSERT_FATAL(topic2 != NULL);
	CU_ASSERT(ismc_getProperty(topic2, "DisableACK", &field) == ISMRC_PropertyNotFound);

	cons2 = ismc_createConsumer(session2, topic2, NULL, 0);
	CU_ASSERT_FATAL(cons2 != NULL);
	CU_ASSERT(ismc_getProperty(cons2, "DisableACK", &field) == ISMRC_PropertyNotFound);

	field.type = VT_Boolean;
	field.val.i = 0;
	ismc_setProperty(topic2, "DisableACK", &field);
	CU_ASSERT(ismc_getProperty(topic2, "DisableACK", &field) == 0);
	CU_ASSERT(field.type == VT_Boolean && field.val.i == 0);

	cons22 = ismc_createConsumer(session2, topic2, NULL, 0);
	CU_ASSERT_FATAL(cons22 != NULL);
	CU_ASSERT(ismc_getProperty(cons22, "DisableACK", &field) == 0);
	CU_ASSERT(field.type == VT_Boolean && field.val.i == 0);

	ismc_closeConnection(conn2);

	ismc_free(conn);
	ismc_free(conn2);
	ismc_free(session);
	ismc_free(session2);
	ismc_free(topic);
	ismc_free(topic2);
	ismc_free(cons);
	ismc_free(cons1);
	ismc_free(cons2);
	ismc_free(cons22);
	ismc_free(prod);
}

/**
 * Verifies how DisableACK property is inherited from connection, session and queue
 * to the message consumer.
 */
void testQueueNoAck(void) {
	/* TODO Enable when temporary queue could be created by ismc in the engine */

//  ism_field_t field;
//	ismc_connection_t * conn, * conn2, * conn3;
//	ismc_session_t * session, * session2, * session3;
//	ismc_destination_t * queue1, * queue2, * queue3;
//	ismc_producer_t * prod1;
//	ismc_consumer_t * cons, * cons1, * cons2, * cons22, * cons3;
//
//	char * server = getenv("ISMServer");
//	char * client_port = getenv("ISMPort");
//
//	conn = ismc_createConnection();
//	CU_ASSERT_FATAL(conn != NULL);
//
//	if (server) {
//		ismc_setStringProperty(conn, "Server", server);
//	}
//
//	if (client_port) {
//		ismc_setStringProperty(conn, "Port", client_port);
//	}
//	ismc_setStringProperty(conn, "Protocol", "@@@");
//
//	field.type = VT_Boolean;
//	field.val.i = 1;
//	ismc_setProperty(conn, "DisableACK", &field);
//	ismc_setStringProperty(conn, "protocol", "dummy");
//	ismc_setStringProperty(conn, "ClientID", CID);
//
//	CU_ASSERT(ismc_getProperty(conn, "DisableACK", &field) == 0);
//	CU_ASSERT(field.type == VT_Boolean && field.val.i == 1);
//
//	CU_ASSERT_FATAL(ismc_connect(conn) == 0);
//
//	session = ismc_createSession(conn, 0, SESSION_AUTO_ACKNOWLEDGE);
//	CU_ASSERT(ismc_getProperty(session, "DisableACK", &field) == 0);
//	CU_ASSERT(field.type == VT_Boolean && field.val.i == 1);
//
//	queue1 = ismc_createQueue("queue1");
//	CU_ASSERT_FATAL(queue1 != NULL);
//	CU_ASSERT(ismc_getProperty(queue1, "DisableACK", &field) == ISMRC_PropertyNotFound);
//
//	prod1 = ismc_createProducer(session, queue1);
//	CU_ASSERT(ismc_getProperty(prod1, "DisableACK", &field) == 0);
//	CU_ASSERT(field.type == VT_Boolean && field.val.i == 1);
//
//	cons = ismc_createConsumer(session, queue1, NULL, 0);
//	CU_ASSERT(ismc_getProperty(cons, "DisableACK", &field) == 0);
//	CU_ASSERT(field.type == VT_Boolean && field.val.i == 1);
//
//	field.type = VT_Boolean;
//	field.val.i = 0;
//	ismc_setProperty(queue1, "DisableACK", &field);
//	CU_ASSERT(ismc_getProperty(queue1, "DisableACK", &field) == 0);
//	CU_ASSERT(field.type == VT_Boolean && field.val.i == 0);
//
//	cons1 = ismc_createConsumer(session, queue1, NULL, 0);
//	CU_ASSERT(ismc_getProperty(cons1, "DisableACK", &field) == 0);
//	CU_ASSERT(field.type == VT_Boolean && field.val.i == 0);
//
//	ismc_closeConnection(conn);
//
//	conn2 = ismc_createConnection();
//	CU_ASSERT_FATAL(conn2 != NULL);
//	CU_ASSERT(ismc_getProperty(conn2, "DisableACK", &field) == ISMRC_PropertyNotFound);
//
//	if (server) {
//		ismc_setStringProperty(conn2, "Server", server);
//	}
//	if (client_port) {
//		ismc_setStringProperty(conn2, "Port", client_port);
//	}
//	ismc_setStringProperty(conn2, "protocol", "dummy");
//	ismc_setStringProperty(conn2, "ClientID", CID);
//
//	CU_ASSERT_FATAL(ismc_connect(conn2) == 0);
//
//	session2 = ismc_createSession(conn2, 0, SESSION_AUTO_ACKNOWLEDGE);
//	CU_ASSERT(ismc_getProperty(session2, "DisableACK", &field) == ISMRC_PropertyNotFound);
//
//	queue2 = ismc_createQueue("queue2");
//	CU_ASSERT_FATAL(queue2 != NULL);
//	CU_ASSERT(ismc_getProperty(queue2, "DisableACK", &field) == ISMRC_PropertyNotFound);
//
//	cons2 = ismc_createConsumer(session2, queue2, NULL, 0);
//    // No forced default for DisableACK property for queue destinations
//	CU_ASSERT(ismc_getProperty(cons2, "DisableACK", &field) == ISMRC_PropertyNotFound);
//
//	field.type = VT_Boolean;
//	field.val.i = 0;
//	ismc_setProperty(queue2, "DisableACK", &field);
//	CU_ASSERT(ismc_getProperty(queue2, "DisableACK", &field) == 0);
//	CU_ASSERT(field.type == VT_Boolean && field.val.i == 0);
//
//	cons22 = ismc_createConsumer(session2, queue2, NULL, 0);
//	CU_ASSERT(ismc_getProperty(cons22, "DisableACK", &field) == 0);
//	CU_ASSERT(field.type == VT_Boolean && field.val.i == 0);
//
//	ismc_closeConnection(conn2);
//
//	conn3 = ismc_createConnection();
//	CU_ASSERT_FATAL(conn3 != NULL);
//	CU_ASSERT(ismc_getProperty(conn3, "DisableACK", &field) == ISMRC_PropertyNotFound);
//
//	field.type = VT_Boolean;
//	field.val.i = 1;
//	ismc_setProperty(conn3, "DisableACK", &field);
//	ismc_setStringProperty(conn3, "protocol", "dummy");
//
//	if (server) {
//		ismc_setStringProperty(conn3, "Server", server);
//	}
//	if (client_port) {
//		ismc_setStringProperty(conn3, "Port", client_port);
//	}
//	ismc_setStringProperty(conn3, "ClientID", CID);
//
//	CU_ASSERT_FATAL(ismc_connect(conn3) == 0);
//
//	session3 = ismc_createSession(conn3, 0, SESSION_AUTO_ACKNOWLEDGE);
//	CU_ASSERT(ismc_getProperty(session3, "DisableACK", &field) == 0);
//	CU_ASSERT(field.type == VT_Boolean && field.val.i == 1);
//
//	queue3 = ismc_createQueue("queue3");
//	CU_ASSERT_FATAL(queue3 != NULL);
//	CU_ASSERT(ismc_getProperty(queue3, "DisableACK", &field) == ISMRC_PropertyNotFound);
//
//	cons3 = ismc_createConsumer(session3, queue3, NULL, 0);
//	CU_ASSERT(ismc_getProperty(cons3, "DisableACK", &field) == 0);
//	CU_ASSERT(field.type == VT_Boolean && field.val.i == 1);
//
//	ismc_closeConnection(conn3);
//
//	ismc_free(conn);
//	ismc_free(conn2);
//	ismc_free(conn3);
//	ismc_free(session);
//	ismc_free(session2);
//	ismc_free(session3);
//	ismc_free(queue1);
//	ismc_free(queue2);
//	ismc_free(queue3);
//	ismc_free(cons);
//	ismc_free(cons1);
//	ismc_free(cons2);
//	ismc_free(cons22);
//	ismc_free(cons3);
//	ismc_free(prod1);
}
