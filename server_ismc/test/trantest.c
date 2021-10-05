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

#include <ismc.h>
#include <ismc_p.h>

#include <ismutil.h>
#include <transport.h>
/*
 */

static int  msglen = 128;
static int  count = 100;

#define CLIENT_ID			"TranTest"
#define SERVER_ADDRESS		"127.0.0.1"
#define DEFAULT_PORT		16102
void ism_engine_threadInit(uint8_t isStoreCrit)
{
}
int ism_process_monitoring_action(ism_transport_t *transport, const char *inpbuf, int buflen, concat_alloc_t *output_buffer, int *rc)
{
	return 0;
}
int main(int argc, char **argv) {
	int  sendcount = 0;
	int  recvcount = 0;
	char clientId[sizeof(CLIENT_ID) + 1];

	ismc_connection_t 	* conn;
	ismc_session_t 		* sess;
	ismc_destination_t 	* requestT;
	ismc_producer_t 	* prod;
	ismc_consumer_t 	* cons;

	char * server = getenv("ISMServer");
	char * port   = getenv("ISMPort");
	char * transport = getenv("ISMTransport");
	char * recvBuffer = getenv("ISMRecvBuffer");
	char * sendBuffer = getenv("ISMSendBuffer");
	int    serverPort;

	ism_field_t field;
	int rc;
	int i;
	xUNUSED ism_time_t startTime, endTime;
	ismc_message_t * bmsg, * rmsg;
	char * msgbytes;
	unsigned char b;

	ism_xid_t xid;
	const int xidCount = 10;
	ism_xid_t xidBuffer[xidCount];

	ism_common_initUtil();
	ism_common_setTraceLevel(7);

	sprintf(clientId, "%s", CLIENT_ID);

	if (!server) {
		server = SERVER_ADDRESS;
	}

	if (port != NULL) {
		serverPort = atoi(port);
	} else {
		serverPort = DEFAULT_PORT;
	}

	conn = ismc_createConnectionX(clientId, NULL, server, serverPort);
	if (conn == NULL) {
		printf("Cannot create connection\n");
		return 0;
	}

	if (port != NULL) {
		ismc_setStringProperty(conn, "Port", port);
	}

	if (transport != NULL) {
		ismc_setStringProperty(conn, "Protocol", transport);
	}

	if (recvBuffer) {
		ismc_setStringProperty(conn, "RecvBufferSize", recvBuffer);
		ismc_setStringProperty(conn, "RecvSockBuffer", recvBuffer);
	}

	if (sendBuffer) {
		ismc_setStringProperty(conn, "SendSockBuffer", recvBuffer);
	}

	field.type = VT_Boolean;
	field.val.i = 1;
	ismc_setProperty(conn, "DisableMessageTimestamp", &field);
	ismc_setProperty(conn, "DisableMessageID", &field);

	rc = ismc_connect(conn);
	if (rc != 0) {
		printf("Cannot connect\n");
		return 0;
	}

	sess = ismc_createSession(conn, 1, SESSION_TRANSACTED_GLOBAL);
	if (sess == NULL) {
		printf("Cannot create session\n");
		return 0;
	}

	requestT = ismc_createTopic("RequestT");
	if (requestT == NULL) {
		printf("Cannot create topic\n");
		return 0;
	}

	prod = ismc_createProducer(sess, requestT);
	if (prod == NULL) {
		printf("Cannot create producer\n");
		return 0;
	}

	cons = ismc_createConsumer(sess, requestT, NULL, 0);
	if (cons == NULL) {
		printf("Cannot create consumer\n");
		return 0;
	}

	rc = ismc_startConnection(conn);
	if (rc != 0) {
		printf("Cannot start connection\n");
		return 0;
	}

	startTime = ism_common_currentTimeNanos();

	xid.formatID = 0;
	xid.gtrid_length = 4;
	xid.bqual_length = 0;
	memcpy(xid.data, "abcd", 4);
	rc = ismc_startGlobalTransaction(sess, &xid);
	if (rc != 0) {
		printf("Cannot create global transaction\n");
		return 0;
	}

	bmsg = ismc_createMessage(sess, MTYPE_BytesMessage);
	ismc_setDeliveryMode(bmsg, ISMC_PERSISTENT);
	msgbytes = calloc(msglen, 1);

	ismc_setIntProperty(bmsg, "fred", 4, VT_Integer);

	field.type = VT_Double;
	field.val.d = 3.1415926535;
	ismc_setProperty(bmsg, "pi", &field);

	for (i = 0; i < count; i++) {
		msgbytes[0] = i&0xFF;
		ismc_setContent(bmsg, msgbytes, msglen);
		rc = ismc_send(prod, bmsg);
		if (rc) {
			printf("Send failed\n");
			break;
		}
		sendcount++;
	}
    rc = ismc_endGlobalTransaction(sess);
    if (rc != 0) {
        printf("Cannot end global transaction\n");
        return 0;
    }

	rc = ismc_prepareGlobalTransaction(sess,&xid);
	if (rc != 0) {
		printf("Cannot prepare global transaction - %d\n", rc);
		return 0;
	}

	rc = ismc_recoverGlobalTransactions(sess, &xidBuffer[0], xidCount, TMSTARTRSCAN|TMENDRSCAN);
	printf("Recovered %d transactions (before commit1)\n", rc);
	for (i = 0; i < rc; i++) {
	    printf("XID[%d] = %d, %d-%d, %.*s-%.*s\n",
	    		i, xidBuffer[i].formatID,
	    		xidBuffer[i].gtrid_length, xidBuffer[i].bqual_length,
	    		xidBuffer[i].gtrid_length, xidBuffer[i].data,
	    		xidBuffer[i].bqual_length,
	    		&xidBuffer[i].data[xidBuffer[i].gtrid_length]);
	}

	rc = ismc_commitGlobalTransaction(sess,&xid,0);
	if (rc != 0) {
		printf("Cannot commit global transaction\n");
		return 0;
	}

	rc = ismc_recoverGlobalTransactions(sess, &xidBuffer[0], xidCount, TMSTARTRSCAN|TMENDRSCAN);
	printf("Recovered %d transactions (before commit1)\n", rc);
	for (i = 0; i < rc; i++) {
	    printf("XID[%d] = %d, %d-%d, %.*s-%.*s\n",
	    		i, xidBuffer[i].formatID,
	    		xidBuffer[i].gtrid_length, xidBuffer[i].bqual_length,
	    		xidBuffer[i].gtrid_length, xidBuffer[i].data,
	    		xidBuffer[i].bqual_length,
	    		&xidBuffer[i].data[xidBuffer[i].gtrid_length]);
	}

	xid.formatID = 0;
	xid.gtrid_length = 4;
	xid.bqual_length = 0;
	memcpy(xid.data, "1234", 4);
	rc = ismc_startGlobalTransaction(sess, &xid);
	if (rc != 0) {
		printf("Cannot create global transaction\n");
		return 0;
	}

	for (i = 0; i < count; i++) {
		rc = ismc_receive(cons, 500, &rmsg);
		if (rc == ISMRC_TimeOut) {
			printf("Timed out\n");
			break;
		}

		/*
		 * Validate the message
		 * TODO Check content correctness - currently every thread receives
		 * the same published message, so data can repeat.
		 */
		ismc_getContent(rmsg, (char*)&b, 0, 1);
		if (ismc_getContentSize(rmsg) != msglen) {
			printf("Message received is not the correct message.\n");
			break;
		}

		ismc_freeMessage(rmsg);

		recvcount++;
	}
    rc = ismc_endGlobalTransaction(sess);
    if (rc != 0) {
        printf("Cannot end global transaction\n");
        return 0;
    }


	rc = ismc_prepareGlobalTransaction(sess,&xid);
	if (rc != 0) {
		printf("Cannot prepare global transaction2\n");
		return 0;
	}

	rc = ismc_recoverGlobalTransactions(sess, &xidBuffer[0], xidCount, TMSTARTRSCAN|TMENDRSCAN);
	printf("Recovered %d transactions (before commit1)\n", rc);
	for (i = 0; i < rc; i++) {
	    printf("XID[%d] = %d, %d-%d, %.*s-%.*s\n",
	    		i, xidBuffer[i].formatID,
	    		xidBuffer[i].gtrid_length, xidBuffer[i].bqual_length,
	    		xidBuffer[i].gtrid_length, xidBuffer[i].data,
	    		xidBuffer[i].bqual_length,
	    		&xidBuffer[i].data[xidBuffer[i].gtrid_length]);
	}

	rc = ismc_commitGlobalTransaction(sess,&xid,0);
	if (rc != 0) {
		printf("Cannot commit global transaction2\n");
		return 0;
	}

	endTime = ism_common_currentTimeNanos();

	ismc_closeConnection(conn);

	ismc_free(conn);
	ismc_free(sess);
	ismc_free(requestT);
	ismc_freeMessage(bmsg);
	ismc_free(prod);
	ismc_free(cons);

	return 0;
}
