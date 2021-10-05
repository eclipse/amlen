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
 * Test a point-to-point send and receive with acknowledge
 *
 * There are three positional arguments which can be set when running this test:
 * 1. The size of the message (default = 128)
 * 2. The number of messages (default = 100)
 * 3. The name of the topic (default = "RequestT")
 *
 * The location of the ISM server can be specified using two environment variables:
 * ISMServer - The IP address of the server (default = 127.0.0.1)
 * ISMPort   - The port number of the server (default = 16102)
 *
 * The default can also be modified in the section below.
 */

static int  msglen = 128;
static int  count = 100;
static const char * topicName = "RequestT";

static ismc_connection_t * conn;
static volatile int recvdone1 = 0;
static volatile int recvdone2 = 0;

static int  sendcount = 0;
static int  recvcount1 = 0;
static int  recvcount2 = 0;

#define CLIENT_ID			"TopicListenerTest"
#define SERVER_ADDRESS		"127.0.0.1"
#define DEFAULT_PORT		16102

int parseArgs(int argc, char * * argv);
void syntaxhelp(const char * msg);
uint64_t doSend(ismc_session_t * sess, ismc_producer_t * prod);
void messageListener(ismc_message_t * message, ismc_consumer_t * consumer, void * userdata);
void ism_engine_threadInit(uint8_t isStoreCrit)
{
}
int ism_process_monitoring_action(ism_transport_t *transport, const char *inpbuf, int buflen, concat_alloc_t *output_buffer, int *rc)
{
	return 0;
}
/*
 * Main method
 */
int main(int argc, char * * argv) {
	uint64_t millis = 0;
	ismc_session_t *  sendSess = NULL;
	ismc_session_t *  recvSess1 = NULL;
	ismc_session_t *  recvSess2 = NULL;
	ismc_producer_t * prod = NULL;
	ismc_consumer_t * cons1 = NULL;
	ismc_consumer_t * cons2 = NULL;
	ismc_destination_t * prodT;
	ismc_destination_t * consT1;
	ismc_destination_t * consT2;
	char * server;
	char * port;
	char * transport;
	char * recvBuffer;
	char * sendBuffer;
	int    serverPort;
	ism_field_t field;
	int rc;
	int id1 = 1, id2 = 2;

	if (parseArgs(argc, argv) < 0) {
		return -1;
	}

	/*
	 * Create connection
	 */

	server = getenv("ISMServer");
	port   = getenv("ISMPort");
	transport = getenv("ISMTransport");
	recvBuffer = getenv("ISMRecvBuffer");
	sendBuffer = getenv("ISMSendBuffer");

	if (!server) {
		server = SERVER_ADDRESS;
	}

	if (port != NULL) {
		serverPort = atoi(port);
	} else {
		serverPort = DEFAULT_PORT;
	}

	conn = ismc_createConnectionX(CLIENT_ID, NULL, server, serverPort);
	if (conn == NULL) {
		printf("Cannot create connection\n");
		return -1;
	}

	if (port) {
		ismc_setStringProperty(conn, "Port", port);
	}

	if (transport) {
		ismc_setStringProperty(conn, "Protocol", transport);
	}

	if (recvBuffer) {
		ismc_setStringProperty(conn, "RecvBuffer", recvBuffer);
	}

	if (sendBuffer) {
		ismc_setStringProperty(conn, "SendBuffer", sendBuffer);
	}

	field.type = VT_Boolean;
	field.val.i = 1;
	ismc_setProperty(conn, "DisableMessageTimestamp", &field);
	ismc_setProperty(conn, "DisableMessageID", &field);

	rc = ismc_connect(conn);
	if (rc != 0) {
		printf("Cannot connect\n");
		return -1;
	}

	sendSess  = ismc_createSession(conn, 0, 1);	// Auto-ACK
	recvSess1 = ismc_createSession(conn, 0, 1);	// Auto-ACK
	recvSess2 = ismc_createSession(conn, 0, 1);	// Auto-ACK
	if (!sendSess || !recvSess1 || !recvSess2) {
		printf("Cannot create session\n");
		return -1;
	}

	prodT  = ismc_createTopic(topicName);
	consT1 = ismc_createTopic(topicName);
	consT2 = ismc_createTopic(topicName);
	if (!prodT || !consT1 || !consT2) {
		printf("Cannot create topic\n");
		return -1;
	}

//	ismc_setProperty(consT1, "DisableACK", &field);
//	ismc_setProperty(consT2, "DisableACK", &field);

	prod = ismc_createProducer(sendSess, prodT);
	if (!prod) {
		printf("Cannot create producer\n");
		return -1;
	}

	cons1 = ismc_createConsumer(recvSess1, consT1, NULL, 0);
	cons2 = ismc_createConsumer(recvSess2, consT2, NULL, 0);
	if (!cons1 || !cons2) {
		printf("Cannot create consumer\n");
		return -1;
	}

	ismc_setlistener(cons1, messageListener, &id1);
	ismc_setlistener(cons2, messageListener, &id2);

	/*
	 * Run the send
	 */
	millis = doSend(sendSess, prod);

	/*
	 * Print out statistics
	 */
	printf("Time = %ld\n", millis);
	printf("Send = %d\n", sendcount);
	printf("Recv(1) = %d\n", recvcount1);
	printf("Recv(2) = %d\n", recvcount2);

	ismc_closeConsumer(cons1);
	ismc_closeConnection(conn);
	ismc_free(prod);
	ismc_free(prodT);
	ismc_free(cons1);
	ismc_free(cons2);
	ismc_free(consT1);
	ismc_free(consT2);
	ismc_free(sendSess);
	ismc_free(recvSess1);
	ismc_free(recvSess2);
	ismc_free(conn);

	return 0;
}

/*
 * Parse arguments
 */
int parseArgs(int argc, char * * argv) {
 	if (argc > 1) {
		msglen = atoi(argv[1]);
		if (msglen <= 0) {
			syntaxhelp("Message length not valid");
			return -1;
		}
    }
	if (argc > 2) {
		count = atoi(argv[2]);
		if (count <= 0) {
			syntaxhelp("");
			return -1;
		}
    }
	if (argc > 3) {
		topicName = argv[3];
	}

	return 0;
}

/*
 * Put out a simple syntax help
 */
void syntaxhelp(const char * msg)  {
	if (msg != NULL)
        printf("%s\n", msg);
	printf("TopicListenerTest  msglen  count\n");
}

void messageListener(ismc_message_t * message, ismc_consumer_t * consumer, void * userdata) {
	int inst = *(int *)userdata;
    int recvcount;
    int done = 0;
    unsigned char b;

    if(inst == 1) {
        recvcount = recvcount1;
        done = recvdone1;
    } else {
        recvcount = recvcount2;
        done = recvdone2;
    }

    /* Validate the message */
	ismc_getContent(message, (char*)&b, 0, 1);
	if (ismc_getContentSize(message) != msglen || b != (unsigned char)(recvcount & 0xFF)) {
		printf("Message received in %d is not the correct message: "
			   "expected(%d) found(%d) "
			   "expected length(%d) found length(%d)\n",
			   inst, (unsigned char)(recvcount&0xFF), b, msglen, ismc_getContentSize(message));

	    if(inst == 1) {
	        recvdone1 = 1;
	    } else {
	        recvdone2 = 1;
	    }

		return;
	}

    recvcount++;
    if (recvcount == count) {
        done = 1;
    }
    if(inst == 1) {
        recvcount1 = recvcount;
        recvdone1 = done;
    } else {
        recvcount2 = recvcount;
        recvdone2 = done;
    }
}

/*
 * Send messages
 */
uint64_t doSend(ismc_session_t * sess, ismc_producer_t * prod) {
	int rc;
	int i;
	ism_time_t startTime, endTime;
	ismc_message_t * bmsg;
	char * msgbytes;
	int wait = 800;

	ism_common_sleep(100000);

	rc = ismc_startConnection(conn);
//	if (rc != 0) {
//		printf("Cannot start connection: %d\n", rc);
//		return -1;
//	}

	startTime = ism_common_currentTimeNanos();

	bmsg = ismc_createMessage(sess, MTYPE_BytesMessage);
	ismc_setDeliveryMode(bmsg, ISMC_NON_PERSISTENT);		// Non-persistent
	msgbytes = calloc(msglen, 1);

	for (i = 0; i < count; i++) {
		ismc_clearContent(bmsg);
		msgbytes[0] = (i&0xFF);
		ismc_setContent(bmsg, msgbytes, msglen);

		rc = ismc_send(prod, bmsg);
		if (rc) {
			printf("Send %d failed\n", i);
			break;
		}

		sendcount++;
		if ((sendcount % 500) == 0) {
			ism_common_sleep(3000);
		}
	}

	free(msgbytes);

	while (wait-- > 0 && !recvdone1 && !recvdone2) {
		ism_common_sleep(50000);
	}
	endTime = ism_common_currentTimeNanos();

	ismc_freeMessage(bmsg);

	if (!recvdone1) {
		printf("Recv(1) - Messages were not all received\n");
	}
	if (!recvdone2) {
		printf("Recv(2) - Messages were not all received\n");
	}
	return (endTime - startTime) / 1000000;
}
