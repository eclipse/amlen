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
 * Test an echoed send and receive with commit
 *
 * There are four positional arguments which can be set when running this test:
 * 1. The size of the message (default = 128)
 * 2. The number of messages (default = 100)
 * 3. The number of messages to send before a commit (default = 1)
 * 4. The number of messages to echo before a commit (default = 1)
 *
 * The location of the ISM server can be specified using two environment variables:
 * ISMServer - The IP address of the server (default = 127.0.0.1)
 * ISMPort   - The port number of the server (default = 16102)
 *
 * The default can also be modified in the section below.
 */

static const uint64_t RECV = 1;
static const uint64_t ECHO = 2;

static int  msglen = 128;
static int  count = 100;
static int  sendcommit = 1;
static int  echocommit = 1;

static ismc_connection_t * conn;
static int  recvdone;
static int  echodone;

static int  sendcount = 0;
static int  echocount = 0;
static int  recvcount = 0;

static pthread_mutex_t * countMutex = NULL;
static pthread_barrier_t barrier;

int parseArgs(int argc, char ** argv);
void syntaxhelp(const char * msg);
void doEcho(void);
void doReceive(void);
uint64_t doSend(void);
void * commitTest(void * arg, void * context, int value);

#define CLIENT_ID			"CommitTest"
#define SERVER_ADDRESS		"127.0.0.1"
#define DEFAULT_PORT		16102
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
int main(int argc, char **argv) {
	uint64_t	millis = 0;
	char 		* server;
	char 		* port;
	char 		* transport;
	int    		serverPort;
	ism_field_t field;
	ism_threadh_t tid;

	if (parseArgs(argc - 1, argv) < 0) {
		return 0;
	}

	countMutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(countMutex, NULL);

	/*
	 * Create connection
	 */
	server = getenv("ISMServer");
	port   = getenv("ISMPort");
	transport = getenv("ISMTransport");

	if (!server) {
		server = SERVER_ADDRESS;
	}

	if (port != NULL) {
		ismc_setStringProperty(conn, "Port", port);
		serverPort = atoi(port);
	} else {
		serverPort = DEFAULT_PORT;
	}

	if (transport != NULL) {
		ismc_setStringProperty(conn, "Protocol", transport);
	}

	conn = ismc_createConnectionX(CLIENT_ID, NULL, server, serverPort);
	if (conn == NULL) {
		printf("Cannot create connection\n");
		return 0;
	}

	field.type = VT_Boolean;
	field.val.i = 1;
	ismc_setProperty(conn, "DisableMessageTimestamp", &field);
	ismc_setProperty(conn, "DisableMessageID", &field);

	ismc_connect(conn);

	/*
	 * Start the receive threads
	 */
	pthread_barrier_init(&barrier, NULL, 3);

    ism_common_startThread(&tid, commitTest, (void *)RECV, NULL, 0,
               ISM_TUSAGE_NORMAL, 0, "commit test receiver", "receiver thread");
    ism_common_startThread(&tid, commitTest, (void *)ECHO, NULL, 0,
               ISM_TUSAGE_NORMAL, 0, "commit test echo", "echo thread");

	/*
	 * Run the send
	 */
	millis = doSend();

	/*
	 * Print out statistics
	 */
	pthread_mutex_lock(countMutex);
	printf("Time = %ld\n", millis);
	printf("Send = %d\n", sendcount);
	printf("Echo = %d\n", echocount);
	printf("Recv = %d\n", recvcount);
	pthread_mutex_unlock(countMutex);

	ismc_stopConnection(conn);
	ismc_closeConnection(conn);

	exit(0);
}



void * commitTest(void * arg, void * context, int value) {
	/*
	 * We only get one run, but choose which function to execute in this thread
	 */
	uint64_t action = (uint64_t)arg;

	pthread_barrier_wait(&barrier);

	if (action == RECV) {
		doReceive();
	} else {
		doEcho();
	}

	return NULL;
}


/*
 * Send messages
 */
uint64_t doSend(void)  {
	ismc_session_t 		* sess;
	ismc_destination_t 	* requestQ;
	ismc_producer_t 	* prod;
	int					rc;
	int					commitcount;
	char 				* msgbytes;
	int					i;
	ismc_message_t 		* bmsg;
	ism_time_t 			startTime;
	ism_time_t 			endTime;
	int 				wait = 1000;

	sess = ismc_createSession(conn, 1, 0);
	if (sess == NULL) {
		printf("Cannot create session at %d\n", __LINE__);
		return -1;
	}

	requestQ = ismc_createQueue("RequestQ");
	if (requestQ == NULL) {
		printf("Cannot create queue at %d\n", __LINE__);
		return -1;
	}

	prod = ismc_createProducer(sess, requestQ);
	if (prod == NULL) {
		printf("Cannot create producer at %d\n", __LINE__);
		return -1;
	}

	rc = ismc_startConnection(conn);
	if (rc != 0) {
		printf("Cannot connect\n");
		return 0;
	}

	commitcount = 0;
	pthread_barrier_wait(&barrier);
	pthread_barrier_destroy(&barrier);

	startTime = ism_common_currentTimeNanos();

	bmsg = ismc_createMessage(sess, MTYPE_BytesMessage);
	ismc_setDeliveryMode(bmsg, ISMC_PERSISTENT);
	msgbytes = calloc(msglen, 1);

	for (i = 0; i < count; i++) {
		msgbytes[0] = i&0xFF;
		ismc_setContent(bmsg, msgbytes, msglen);
		ismc_send(prod, bmsg);

		if (++commitcount == sendcommit) {
			ismc_commitSession(sess);
			commitcount = 0;
		}

		pthread_mutex_lock(countMutex);
		sendcount++;
		if (sendcount % 500 == 0) {
			pthread_mutex_unlock(countMutex);
			ism_common_sleep(3000);
		} else {
			pthread_mutex_unlock(countMutex);
		}
	}
	ismc_commitSession(sess);

	free(msgbytes);

	pthread_mutex_lock(countMutex);

	while ((wait-- > 0) && (!recvdone || !echodone)) {
		pthread_mutex_unlock(countMutex);
		ism_common_sleep(50000);
		pthread_mutex_lock(countMutex);
	}
	pthread_mutex_unlock(countMutex);

	endTime = ism_common_currentTimeNanos();
	if (!recvdone || !echodone) {
		printf("Messages were not all received\n");
	}
	return (endTime-startTime) / 1000000;
}

/*
 * Receive messages
 */
void doReceive(void) {
	ismc_session_t 		* sess;
	ismc_destination_t 	* responseQ;
	ismc_consumer_t 	* cons;
	unsigned char 		b;
	int 				commitcount = 0;
	int 				i = 0;

	sess = ismc_createSession(conn, (echocommit != 0), 3);
	if (sess == NULL) {
		printf("Cannot create session at %d\n", __LINE__);
		return;
	}

	responseQ = ismc_createQueue("ResponseQ");
	if (responseQ == NULL) {
		printf("Cannot create queue at %d\n", __LINE__);
		return;
	}

	cons = ismc_createConsumer(sess, responseQ, NULL, 0);
	if (cons == NULL) {
		printf("Cannot create consumer at %d\n", __LINE__);
		return;
	}

	do {
		ismc_message_t * msg;
		int rc = ismc_receive(cons, 500, &msg);

		if (rc == ISMRC_TimeOut) {
			printf("Timeout in receive message: %d\n", i);
			break;
		}

		/* Validate the message */
		ismc_getContent(msg, (char*)&b, 0, 1);
		if (ismc_getContentSize(msg) != msglen || b != (i&0xFF)) {
			printf("Message received is not the correct message. Expected: %d Found: %d\n", i, b);
			break;
		}

		if (++commitcount == echocommit) {
			ismc_commitSession(sess);
			commitcount = 0;
		}
		recvcount++;
		i++;
	} while(recvcount < count);
	ismc_commitSession(sess);

	pthread_mutex_lock(countMutex);
	recvdone = 1;
	if (recvcount < count) {
		printf("doReceive: Not all messages were received: %d\n", recvcount);
	}
	pthread_mutex_unlock(countMutex);
}

/*
 * Echo messages
 */
void doEcho(void) {
	ismc_session_t 		* sess;
	ismc_destination_t 	* requestQ;
	ismc_destination_t 	* responseQ;
	ismc_consumer_t 	* cons;
	ismc_producer_t 	* prod;
	unsigned char 		b;
	int 				commitcount = 0;
	int 				i = 0;


	sess = ismc_createSession(conn, (echocommit != 0), 3);
	if (sess == NULL) {
		printf("Cannot create session at %d\n", __LINE__);
		return;
	}

	requestQ = ismc_createQueue("RequestQ");
	if (requestQ == NULL) {
		printf("Cannot create queue at %d\n", __LINE__);
		return;
	}

	responseQ = ismc_createQueue("ResponseQ");
	if (responseQ == NULL) {
		printf("Cannot create queue at %d\n", __LINE__);
		return;
	}

	cons = ismc_createConsumer(sess, requestQ, NULL, 0);
	if (cons == NULL) {
		printf("Cannot create consumer at %d\n", __LINE__);
		return;
	}

	prod = ismc_createProducer(sess, responseQ);
	if (prod == NULL) {
		printf("Cannot create producer at %d\n", __LINE__);
		return;
	}

	do {
		ismc_message_t * msg;
		int rc = ismc_receive(cons, 500, &msg);

		if (rc == ISMRC_TimeOut) {
			printf("Timeout in echo message: %d\n", i);
			break;
		}

		ismc_getContent(msg, (char*)&b, 0, 1);
//		if ((i % 50) == 0) {
//			printf("!!Received message %d in echo (%d)\n", i, b);
//		}

		/* Validate the message */
		if (ismc_getContentSize(msg) != msglen || b != (i&0xFF)) {
			printf("Message to echo is not the correct message. - "
				   "Expected value %d, found: %d - "
				   "Expected length %d, found %d\n", i, b, msglen, ismc_getContentSize(msg));
			break;
		}

		echocount++;
		ismc_setDeliveryMode(msg, 2);
		ismc_send(prod, msg);
		if (++commitcount == echocommit) {
			ismc_commitSession(sess);
			commitcount = 0;
		}
		i++;
	} while (echocount < count);
	ismc_commitSession(sess);
	pthread_mutex_lock(countMutex);
	echodone = 1;
	if (echocount < count){
		printf("doEcho: Not all messages were received: %d\n", echocount);
	}
	pthread_mutex_unlock(countMutex);
}

/*
 * Parse arguments
 */
int parseArgs(int argc, char ** argv) {
	if (argc > 0) {
		msglen = atoi(argv[1]);

		if (msglen <= 0) {
			syntaxhelp("Message length not valid");
			return -1;
		}
	}

	if (argc > 1) {
		count = atoi(argv[2]);

		if (count <= 0) {
			syntaxhelp("");
			return -1;
		}
	}
	if (argc > 2) {
		sendcommit = atoi(argv[3]);
		if (sendcommit < 0) {
			syntaxhelp("Send commit count not valid");
			return -1;
		}

		if (sendcommit < 1) {
			sendcommit = 1;
		}
	}
	if (argc > 3) {
		echocommit = atoi(argv[4]);
		if (echocommit < 0) {
			syntaxhelp("Echo commit count not valid");
			return -1;
		}

		if (echocommit < 1)
			echocommit = 1;
	}

	return 0;
}

/*
 * Put out a simple syntax help
 */
void syntaxhelp(const char * msg)  {
	if (msg != NULL)
		printf("%s\n", msg);
	printf("CommitTest  msglen  count send_commit  echo_commit\n");
}
