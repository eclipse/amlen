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
 * File: message_unit_test.c
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
#include <message_unit_test.h>

static void startConnection(void);
static void stopConnection(void);


/*
 * Array that carries all message tests for APIs to CUnit framework.
 */
CU_TestInfo ISM_message_tests[] = {
		{ "--- Testing ISM message support        ---", testProperties          },
		{ "--- Testing JMS header properties      ---", testHeaderProperties    },
		{ "--- Testing text message receive       ---", testTextMessageReceive  },
		{ "--- Testing bytes message receive      ---", testBytesMessageReceive },
		{ "--- Testing generic message receive    ---", testMessageReceive },
		CU_TEST_INFO_NULL
};

static int logLevel = 6;

static const char         * CID     = "testClientID";
static ismc_connection_t  * conn    = NULL;
static ismc_session_t     * session = NULL;
static ismc_producer_t    * output  = NULL;
static ismc_consumer_t    * cons    = NULL;
static ismc_destination_t * topic   = NULL;
static const char         * TOPIC1  = "OneTopic";

static const char * xmlmsg = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<!-- \n"
		" This file specifies the IP address of the ISM-CM server.\n"
		" Several test cases use this file as the include file.   \n"
		" Required Action:\n"
		" 1. Replace CMIP with the IP address of the ISM-CM server.\n"
		"-->\n"
		"<param type=\"structure_map\">ip_address=CMIP</param>";


void testProperties(void) {
	ism_field_t field;
	int i;
	int count = 0;
	const char * name;
	ismc_message_t * bmsg;

	bmsg = ismc_createMessage(NULL, MTYPE_BytesMessage);

	CU_ASSERT(ismc_getRedelivered(bmsg) == 0);
	ismc_setDeliveryMode(bmsg, 2);		// Default delivery mode
	CU_ASSERT(ismc_getDeliveryMode(bmsg) == 2);
	CU_ASSERT(ismc_getPriority(bmsg) == 4);		// Default priority
	CU_ASSERT(bmsg->ttl == 0);

	CU_ASSERT(ismc_setStringProperty(bmsg, "fred", "sam") == 0);

    field.type = VT_Boolean;
    field.val.i = 1;
	CU_ASSERT(ismc_setProperty(bmsg, "bool", &field) == 0);

	CU_ASSERT(ismc_getProperty(bmsg, "sam", &field) == ISMRC_PropertyNotFound);
	CU_ASSERT(ismc_setIntProperty(bmsg, "sam", 3, VT_Integer) == 0);
	CU_ASSERT(ismc_getProperty(bmsg, "sam", &field) == 0);

    field.type = VT_Integer;
    field.val.i = 3;
	CU_ASSERT(ismc_setProperty(bmsg, "sam", &field) == 0);
	CU_ASSERT(ismc_setIntProperty(bmsg, "bytep", (char)-3, VT_Byte) == 0);
	CU_ASSERT(ismc_setStringProperty(bmsg, "numval", "314") == 0);

	field.type = VT_Double;
	field.val.d = 3.14;
	CU_ASSERT(ismc_setProperty(bmsg, "tim", &field) == 0);

	field.type = VT_Long;
	field.val.l = 1234567890123L;
	CU_ASSERT(ismc_setProperty(bmsg, "longp", &field) == 0);

	field.type = VT_Float;
	field.val.f = 3.0f;
	CU_ASSERT(ismc_setProperty(bmsg, "kurt", &field) == 0);

	CU_ASSERT(ismc_setIntProperty(bmsg, "shrt", (short)55, VT_Short) == 0);

	for (i=0; ism_common_getPropertyIndex(bmsg->h.props, i, &name) == 0; i++) {
		if (!strcmp(name, "sam") ||
			!strcmp(name, "numval") ||
			!strcmp(name, "shrt")) {
			count++;
		}
	}

	CU_ASSERT(i == 9);
	CU_ASSERT(count == 3);

    /* Check float conversions */
	field.type = VT_Null;
	CU_ASSERT(ismc_getProperty(bmsg, "kurt", &field) == 0);
	CU_ASSERT(field.type == VT_Float);
	CU_ASSERT_DOUBLE_EQUAL(3.0f, field.val.f, 0.01);
	CU_ASSERT(ismc_getStringProperty(bmsg, "kurt") != NULL &&
			  !strcmp(ismc_getStringProperty(bmsg, "kurt"), "3.0"));

    /* Check double conversions */
	field.type = VT_Null;
	CU_ASSERT(ismc_getProperty(bmsg, "tim", &field) == 0);
	CU_ASSERT(field.type == VT_Double);
	CU_ASSERT_DOUBLE_EQUAL(3.14, field.val.d, 0.001);
	CU_ASSERT(ismc_getStringProperty(bmsg, "tim") != NULL &&
			  !strcmp(ismc_getStringProperty(bmsg, "tim"), "3.14"));

    /* Check int conversions */
	CU_ASSERT(ismc_getIntProperty(bmsg, "sam", -1) == 3);
	CU_ASSERT(ismc_getStringProperty(bmsg, "sam") != NULL &&
			  !strcmp(ismc_getStringProperty(bmsg, "sam"), "3"));

    /* Check string property conversions */
	field.type = VT_Null;
	CU_ASSERT(ismc_getProperty(bmsg, "numval", &field) == 0);
	CU_ASSERT(field.type == VT_String);
	CU_ASSERT(ismc_getIntProperty(bmsg, "numval", -1) == 314);
	CU_ASSERT(ismc_getStringProperty(bmsg, "numval") != NULL &&
			  !strcmp("314", ismc_getStringProperty(bmsg, "numval")));

    /* Check byte property conversions */
	field.type = VT_Null;
	CU_ASSERT(ismc_getProperty(bmsg, "bytep", &field) == 0);
	CU_ASSERT(field.type == VT_Byte);
	CU_ASSERT(ismc_getIntProperty(bmsg, "bytep", -1) == -3);
	CU_ASSERT(ismc_getStringProperty(bmsg, "bytep") != NULL &&
			  !strcmp("-3", ismc_getStringProperty(bmsg, "bytep")));

    /* Check short property conversions */
	field.type = VT_Null;
	CU_ASSERT(ismc_getProperty(bmsg, "shrt", &field) == 0);
	CU_ASSERT(field.type == VT_Short);

	CU_ASSERT(ismc_getIntProperty(bmsg, "shrt", -1) == 55);
	CU_ASSERT(ismc_getStringProperty(bmsg, "shrt") != NULL &&
			  !strcmp("55", ismc_getStringProperty(bmsg, "shrt")));

    /* Check long property conversions */
	field.type = VT_Null;
	CU_ASSERT(ismc_getProperty(bmsg, "longp", &field) == 0);
	CU_ASSERT(field.type == VT_Long);
	CU_ASSERT(field.val.l == 1234567890123L);
	CU_ASSERT(ismc_getStringProperty(bmsg, "longp") != NULL &&
			  !strcmp("1234567890123", ismc_getStringProperty(bmsg, "longp")));

    /* Check boolean property conversions */
	field.type = VT_Null;
	CU_ASSERT(ismc_getProperty(bmsg, "bool", &field) == 0);
	CU_ASSERT(field.type == VT_Boolean);
	CU_ASSERT(field.val.i == 1);
	CU_ASSERT(ismc_getStringProperty(bmsg, "bool") != NULL &&
			  !strcmp("true", ismc_getStringProperty(bmsg, "bool")));

	ismc_freeMessage(bmsg);
}

/**
 * Test message headers/properties over send/receive.
 * The server must be running on port 16102.
 */
void testHeaderProperties(void) {
	const char * jmsxprop[] = {
			"JMSXGroupID",
			"JMSXGroupSeq"
	};

	const char * CORR_ID  = "MapCorrelationID_4567";
	const char * JMS_TYPE = "<type=ISM_JMS_$_@_0987/>";

	ismc_message_t * rmsg;
//	ismc_destination_t * pd;
	ismc_message_t * msg;
	ismc_destination_t * dest;
	const char * spr0;
	const char * spr1;
	int mode;
//	int domain;
//	const char * replyTo;
	ism_field_t field;


	/*
	 * Set up the client
	 */
	startConnection();

    /*
     * Test JMS message Header
     *     JMSDestination
     *     JMSDeliveryMode
     *     JMSMessageID
     *     JMSTimestamp
     *     JMSCorrelationID
     *     JMSReplyTo
     *     JMSRedelivered
     *     JMSType
     *     JMSExpiration
     *     JMSPriority
     */

//	pd = output->dest;
	msg = ismc_createMessage(session, MTYPE_BytesMessage);
	ismc_setCorrelationID(msg, CORR_ID);
	ismc_setTypeString(msg, JMS_TYPE);
	ismc_setDeliveryMode(msg, ISMC_NON_PERSISTENT);

	dest = ismc_createTopic("rmm://127.0.0.1:34567");
//        	msg.setJMSDestination(dest);

	ismc_setExpiration(msg, 9876543219876);
	ismc_setPriority(msg, 9);
	ismc_setRedelivered(msg, 0);
	ismc_setReplyTo(msg, dest->name, dest->domain);
	ismc_setTimestamp(msg, 89234);
	ismc_setMessageID(msg, "JMSTestID");

	ismc_setStringProperty(msg, jmsxprop[0], "groupIDtest");
	ismc_setIntProperty(msg, jmsxprop[1], 2867249, VT_Integer);

	ismc_send(output, msg);

	ismc_receive(cons, 500, &rmsg);

	spr0 = ismc_getCorrelationID(rmsg);
	spr1 = ismc_getTypeString(rmsg);
	mode = ismc_getDeliveryMode(rmsg);
//	replyTo = ismc_getReplyTo(rmsg, &domain);

	CU_ASSERT(spr0 != NULL && !strcmp(spr0, CORR_ID));
	CU_ASSERT(spr1 != NULL && !strcmp(spr1, JMS_TYPE));
	/*
	 * TODO In Java, producer has control of the delivery mode and overwrites the setting.
	 * Need to check what we want to have here
	 */
	CU_ASSERT(mode == ISMC_NON_PERSISTENT);

	/*
	 * TODO Enable when implemented
	 */
//	CU_ASSERT(!strcmp(dest->name, replyTo) && (domain == dest->domain));

	/*
	 * Make sure we can still set delivery Mode on received msg
	 */
	ismc_setDeliveryMode(rmsg, 2);
	CU_ASSERT(ismc_getDeliveryMode(rmsg) == 2);

	/*
	 * TODO Enable when implemented
	 */
//	        assertNotSame(dest.toString(), rdest.toString());
//	        assertEquals(pd.toString(), rdest.toString());

	/* Expiration is updated with the timestamp at send time */
	CU_ASSERT(234567890 != ismc_getExpiration(rmsg));

	/* Priority is set on the message itself */
	CU_ASSERT(ismc_getPriority(rmsg) == 9);

    /* jmstimestamp is set by producer using ism_common_currentTimeNanos. */
	CU_ASSERT(89234 != ismc_getTimestamp(rmsg));

//	        assertEquals(dest.toString(), replyto.toString());

	CU_ASSERT(ismc_getMessageID(rmsg) && strcmp("JMSTestID", ismc_getMessageID(rmsg)));
//	CU_ASSERT(ismc_getStringProperty(rmsg, jmsxprop[0]) &&
//			!strcmp("groupIDtest", ismc_getStringProperty(rmsg, jmsxprop[0])));
	CU_ASSERT(2867249 == ismc_getIntProperty(rmsg, jmsxprop[1], -1));

	ismc_freeMessage(rmsg);

	/*
	 * Test default values for timestamp, message id and expiration.
	 */
	field.type = VT_Boolean;
	field.val.i = 1;
	ismc_setProperty(output, "DisableMessageTimestamp", &field);
	ismc_setProperty(output, "DisableMessageID", &field);

	ismc_send(output, msg);
	ismc_receive(cons, 500, &rmsg);

	CU_ASSERT(ismc_getMessageID(rmsg) == NULL);
	CU_ASSERT(ismc_getTimestamp(rmsg) == 0);
	CU_ASSERT(ismc_getExpiration(rmsg) == 0);

	ismc_freeMessage(msg);
	ismc_freeMessage(rmsg);

	stopConnection();
}

/*
 * Test text Message
 */
void testTextMessageReceive(void) {
	const char * textmsg[] = {
			"Hello, World",
			"&lt&tdISMJMS is a project for this release\t",
			xmlmsg,
			"@123%$3",
			"   123  5&*\" ",
			" ",
			"",
			NULL,
	};
	int size = sizeof(textmsg) / sizeof(char *);
	int i;
	char buf[255];
	ismc_message_t * rmsg;
	ismc_message_t * jmsg = NULL;

	startConnection();

	/*
	 * Send messages
	 */

	for (i = 0; i < size; i++) {
		int len;
		int rc;

		CU_ASSERT((jmsg = ismc_createMessage(session, MTYPE_TextMessage)) != NULL);
		ismc_setContentString(jmsg, textmsg[i]);
		rc = ismc_send(output, jmsg);
		CU_ASSERT(rc == 0);

		ismc_receive(cons, 500, &rmsg);
		CU_ASSERT_FATAL(rmsg != NULL);
		len = ismc_getContent(rmsg, buf, 0, 255);
		if (textmsg[i]) {
		    CU_ASSERT(!strncmp(textmsg[i], buf, len));
        } else {
            CU_ASSERT(len == 0);
        }

		ismc_freeMessage(rmsg);
	}

	ismc_clearContent(jmsg);
	ismc_send(output, jmsg);

	ismc_receive(cons, 500, &rmsg);
	CU_ASSERT(ismc_getContent(rmsg, buf, 0, 255) <= 0);
	ismc_freeMessage(rmsg);
	ismc_freeMessage(jmsg);

	jmsg = ismc_createMessage(session, MTYPE_TextMessage);
	ismc_setQuality(jmsg, 1);
	ismc_send(output, jmsg);
	ismc_receive(cons, 500, &rmsg);
	CU_ASSERT(ismc_getQuality(rmsg) == 1);
	CU_ASSERT(ismc_getContent(rmsg, buf, 0, 255) <= 0);
	ismc_freeMessage(rmsg);
	ismc_freeMessage(jmsg);

	jmsg = ismc_createMessage(session, MTYPE_TextMessage);
	ismc_setContentString(jmsg, "fred");
	ismc_setContentString(jmsg, NULL);
	ismc_setQuality(jmsg, 2);
	ismc_send(output, jmsg);
	ismc_receive(cons, 500, &rmsg);
	/* Non-persistent messages cannot be delivered with QoS > 1. */
	CU_ASSERT(ismc_getQuality(rmsg) == 1);
	CU_ASSERT(ismc_getDeliveryMode(rmsg) == ISMC_NON_PERSISTENT);
	CU_ASSERT(ismc_getContent(rmsg, buf, 0, 255) <= 0);
	ismc_freeMessage(rmsg);
	ismc_freeMessage(jmsg);

	jmsg = ismc_createMessage(session, MTYPE_TextMessage);
	ismc_setContentString(jmsg, "fred");
	ismc_setContentString(jmsg, NULL);
	ismc_setQuality(jmsg, 0);
	ismc_send(output, jmsg);
	ismc_receive(cons, 500, &rmsg);
	CU_ASSERT(ismc_getQuality(rmsg) == 0);
	CU_ASSERT(ismc_getDeliveryMode(rmsg) == ISMC_NON_PERSISTENT);
	CU_ASSERT(ismc_getContent(rmsg, buf, 0, 255) <= 0);
	ismc_freeMessage(rmsg);
	ismc_freeMessage(jmsg);

	jmsg = ismc_createMessage(session, MTYPE_TextMessage);
	ismc_setContentString(jmsg, "fred");
	ismc_setContentString(jmsg, NULL);
	ismc_setQuality(jmsg, 1);
	ismc_send(output, jmsg);
	ismc_receive(cons, 500, &rmsg);
	CU_ASSERT(ismc_getQuality(rmsg) == 1);
	CU_ASSERT(ismc_getDeliveryMode(rmsg) == ISMC_NON_PERSISTENT);
	CU_ASSERT(ismc_getContent(rmsg, buf, 0, 255) <= 0);
	ismc_freeMessage(rmsg);
	ismc_freeMessage(jmsg);

	stopConnection();
}

/*
 * Test BytesMessage
 * A BytesMessage is an undifferentiated stream of bytes that can
 * be read in various formats. To get correct value, need to
 * be read in the same order in which they were
 * written.
 */
void testBytesMessageReceive(void) {
	int i;
	ismc_message_t * rmsg;
	ismc_message_t * bytesmsg;
	char bmsg[120];
	char b = 0, br = 1;
	ism_field_t field, fieldr;
	double d = 123.456789e22, dr = 0;
	char outbuf[512], inbuf[512];
	char c, cr;
	int32_t ir, rc;
	uint64_t lg, lr;
	float f, fr;
	const char * rid;
	const char * tid;
	const char * rbpd;

	startConnection();

	for (i = 0; i < sizeof(bmsg); i++) {
		bmsg[i] = (i & 0xFF);
	}

	/* Test boolean */
	CU_ASSERT((bytesmsg = ismc_createMessage(session, MTYPE_BytesMessage)) != NULL);
	ismc_setContent(bytesmsg, &b, 1);

	field.type = VT_Float;
	field.val.f = 12.0;
	ismc_setProperty(bytesmsg, "ISMJmsSendReceiveTest", &field);
	CU_ASSERT(ismc_send(output, bytesmsg) == 0);

	CU_ASSERT_FATAL((ismc_receive(cons, 500, &rmsg)) == ISMRC_OK);
	CU_ASSERT(ismc_getContent(rmsg, &br, 0, 1) == 1);
	CU_ASSERT(br == b);

	ismc_getProperty(rmsg, "ISMJmsSendReceiveTest", &fieldr);
	CU_ASSERT_DOUBLE_EQUAL(field.val.f, fieldr.val.f, 0.1);

	ismc_freeMessage(rmsg);

	ismc_clearContent(bytesmsg);
	ismc_clearProperties(bytesmsg);

	/* Test double */
	ismc_setContent(bytesmsg, (char*)&d, sizeof(d));
	field.type = VT_Boolean;
	field.val.i = 1;
	ismc_setProperty(bytesmsg, "ISMTest", &field);
	CU_ASSERT(ismc_send(output, bytesmsg) == 0);

    CU_ASSERT_FATAL((ismc_receive(cons, 500, &rmsg)) == ISMRC_OK);
	CU_ASSERT(ismc_getContent(rmsg, (char*)&dr, 0, sizeof(dr)) == sizeof(dr));
	CU_ASSERT_DOUBLE_EQUAL(d, dr, 1);

	ismc_getProperty(rmsg, "ISMTest", &fieldr);
	CU_ASSERT(field.val.i == fieldr.val.i);

	ismc_freeMessage(rmsg);

	ismc_clearContent(bytesmsg);
	ismc_clearProperties(bytesmsg);

	/* Test int, char and byte array*/
	i = 77889900;
	c = 'x';
	memcpy(outbuf, &i, sizeof(i));
	memcpy(outbuf + sizeof(i), &c, sizeof(c));
	memcpy(outbuf + sizeof(i) + sizeof(c), bmsg, sizeof(bmsg));
	ismc_setContent(bytesmsg, outbuf, sizeof(i) + sizeof(c) + sizeof(bmsg));
	ismc_setCorrelationID(bytesmsg, "ByteMessageTest_ID");
	CU_ASSERT(ismc_send(output, bytesmsg) == 0);

    CU_ASSERT_FATAL((ismc_receive(cons, 500, &rmsg)) == ISMRC_OK);
	ismc_getContent(rmsg, (char*)&ir, 0, sizeof(ir));
	ismc_getContent(rmsg, (char*)&cr, sizeof(ir), sizeof(cr));
	rc = ismc_getContent(rmsg, inbuf, sizeof(ir) + sizeof(cr), sizeof(inbuf));

	CU_ASSERT(ir == i);
	CU_ASSERT(cr == c);
	CU_ASSERT(rc == sizeof(bmsg));

	for (i = 0; i < rc; i++) {
		CU_ASSERT(bmsg[i] == inbuf[i]);
	}

	rbpd = ismc_getCorrelationID(rmsg);
	CU_ASSERT(rbpd && !strcmp(rbpd, "ByteMessageTest_ID"));

	ismc_freeMessage(rmsg);

	ismc_clearContent(bytesmsg);
	ismc_clearProperties(bytesmsg);

	/* Test hex, float and bytes array property using obj property*/
	i = 0x7f800000;
	lg = ism_common_currentTimeNanos();
	f = (float) 0.00000000000000000000034;
	c = 9;

	memcpy(outbuf, &i, sizeof(i));
	memcpy(outbuf + sizeof(i), &lg, sizeof(lg));
	memcpy(outbuf + sizeof(i) + sizeof(lg), &f, sizeof(f));
	memcpy(outbuf + sizeof(i) + sizeof(lg) + sizeof(f), &c, sizeof(c));
	ismc_setContent(bytesmsg, outbuf, sizeof(i) + sizeof(lg) + sizeof(f) + sizeof(c));

	field.type = VT_Short;
	field.val.i = 1;
	ismc_setProperty(bytesmsg, "$Using_to_send4property", &field);
	CU_ASSERT(ismc_send(output, bytesmsg) == 0);

    CU_ASSERT_FATAL((ismc_receive(cons, 500, &rmsg)) == ISMRC_OK);

	ismc_getContent(rmsg, (char*)&ir, 0, sizeof(ir));
	ismc_getContent(rmsg, (char*)&lr, sizeof(ir), sizeof(lr));
	ismc_getContent(rmsg, (char*)&fr, sizeof(ir) + sizeof(lr), sizeof(fr));
	ismc_getContent(rmsg, (char*)&cr, sizeof(ir) + sizeof(lr) + sizeof(fr), sizeof(cr));

	ismc_getProperty(rmsg, "$Using_to_send4property", &fieldr);

	CU_ASSERT_DOUBLE_EQUAL(f, fr, 1e-24);
	CU_ASSERT_EQUAL(c, cr);
	CU_ASSERT_EQUAL(lg, lr);
	CU_ASSERT_EQUAL(i, ir);
	CU_ASSERT(field.type == VT_Short && field.val.i == fieldr.val.i);

	ismc_freeMessage(rmsg);

	ismc_clearContent(bytesmsg);
	ismc_clearProperties(bytesmsg);

	/*
	 * Test hex, float and bytes array property using obj property
	 */
	memcpy(outbuf, &i, sizeof(i));
	memcpy(outbuf + sizeof(i), &f, sizeof(f));
	memcpy(outbuf + sizeof(i) + sizeof(f), &c, sizeof(c));
	ismc_setContent(bytesmsg, outbuf, sizeof(i) + sizeof(f) + sizeof(c));

	ismc_setTypeString(bytesmsg, "Using_ID34345");
	ismc_setCorrelationID(bytesmsg, "$Bytes@thisLocation");

	CU_ASSERT(ismc_send(output, bytesmsg) == 0);

    CU_ASSERT_FATAL((ismc_receive(cons, 500, &rmsg)) == ISMRC_OK);

	ismc_getContent(rmsg, (char*)&ir, 0, sizeof(ir));
	ismc_getContent(rmsg, (char*)&fr, sizeof(ir), sizeof(fr));
	ismc_getContent(rmsg, (char*)&cr, sizeof(ir) + sizeof(fr), sizeof(cr));

	rid = ismc_getTypeString(rmsg);
	tid = ismc_getCorrelationID(rmsg);

	CU_ASSERT_DOUBLE_EQUAL(f, fr, 1e-24);
	CU_ASSERT_EQUAL(c, cr);
	CU_ASSERT_EQUAL(i, ir);
	CU_ASSERT(rid && !strcmp(rid, "Using_ID34345"));
	CU_ASSERT(tid && !strcmp(tid, "$Bytes@thisLocation"));

	ismc_freeMessage(rmsg);
	ismc_clearContent(bytesmsg);

	CU_ASSERT(ismc_send(output, bytesmsg) == 0);

    CU_ASSERT_FATAL((ismc_receive(cons, 500, &rmsg)) == ISMRC_OK);

	CU_ASSERT_EQUAL(0, ismc_getContentSize(rmsg));

	ismc_freeMessage(rmsg);
	ismc_freeMessage(bytesmsg);

	CU_ASSERT((bytesmsg = ismc_createMessage(session, MTYPE_BytesMessage)) != NULL);
	CU_ASSERT(ismc_send(output, bytesmsg) == 0);

    CU_ASSERT_FATAL((ismc_receive(cons, 500, &rmsg)) == ISMRC_OK);
	CU_ASSERT_EQUAL(0, ismc_getContentSize(rmsg));

	ismc_freeMessage(rmsg);
	ismc_freeMessage(bytesmsg);

	stopConnection();
}

/*
 * Test Message
 */
void testMessageReceive(void) {

	ismc_message_t * msg, * rmsg;
	ism_field_t field, fieldr;

	startConnection();

	CU_ASSERT((msg = ismc_createMessage(session, MTYPE_Message)) != NULL);
	field.type = VT_Boolean;
	field.val.i = 0;
	ismc_setProperty(msg, "MapBooleanValue", &field);
	ismc_setIntProperty(msg, "$amount_share", 7, VT_Byte);
	field.type = VT_Double;
	field.val.d = 160.75;
	ismc_setProperty(msg, "IBM_stock_price_in_$", &field);
	ismc_setIntProperty(msg, "MapStreamID", 30557, VT_Integer);
	ismc_setCorrelationID(msg, "MapCorrelationID_4567");
	ismc_setTypeString(msg, "<type=ISM_JMS_$_@_0987/>");
	CU_ASSERT_FATAL(ismc_send(output, msg) == 0);

    CU_ASSERT_FATAL((ismc_receive(cons, 500, &rmsg)) == ISMRC_OK);
	CU_ASSERT_EQUAL(7, ismc_getIntProperty(rmsg, "$amount_share", -1));
	CU_ASSERT_EQUAL(30557, ismc_getIntProperty(rmsg, "MapStreamID", -1));
	CU_ASSERT_STRING_EQUAL("MapCorrelationID_4567", ismc_getCorrelationID(rmsg));
	CU_ASSERT_STRING_EQUAL("<type=ISM_JMS_$_@_0987/>", ismc_getTypeString(rmsg));

	ismc_getProperty(rmsg, "MapBooleanValue", &fieldr);
	CU_ASSERT(fieldr.type == VT_Boolean && fieldr.val.i == 0);
	ismc_getProperty(rmsg, "IBM_stock_price_in_$", &fieldr);
	CU_ASSERT(fieldr.type == VT_Double);
	CU_ASSERT_DOUBLE_EQUAL(fieldr.val.d, 160.75, 0.001);

	ismc_freeMessage(rmsg);
	ismc_freeMessage(msg);

	stopConnection();
}

/**
 * Creates and starts the connection.
 * Also creates message producer, consumer and a topic.
 */
static void startConnection(void) {
	char * port = getenv("ISMPort");

	ismc_setError(0, "");

	/*
	 * Set up the JMS connection
	 */
	conn = ismc_createConnection();
	CU_ASSERT_FATAL(conn != NULL);

	ismc_setStringProperty(conn, "ClientID", CID);
	if (port)
		ismc_setStringProperty(conn, "Port", port);
	ismc_setStringProperty(conn, "Protocol", "@@@");
	ismc_setIntProperty(conn, "LogLevel", logLevel, VT_Integer);

	CU_ASSERT_FATAL(ismc_connect(conn) == 0);

	session = ismc_createSession(conn, 0, 1);
	CU_ASSERT_FATAL(session != NULL);

	/*
	 * Create some input destinations
	 */
	topic = ismc_createTopic(TOPIC1);
	CU_ASSERT_FATAL(topic != NULL);

	output = ismc_createProducer(session, topic);
	CU_ASSERT_FATAL(output != NULL);

	cons = ismc_createConsumer(session, topic, NULL, 0);
	CU_ASSERT_FATAL(cons != NULL);

	CU_ASSERT_FATAL(ismc_startConnection(conn) == 0);
}

/**
 * Stops and closes the connection.
 * Resets errors, if any
 */
static void stopConnection(void) {
	ismc_closeConnection(conn);

	CU_ASSERT(ismc_disconnect(conn) == 0);

	ismc_free(session);
	ismc_free(topic);
	ismc_free(output);
	ismc_free(cons);

	ismc_setError(0, "");
}


