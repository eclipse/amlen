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
static int  msglen = 100;
static int  count = 100000;
static int  thread_count = 1;

static int donecount = 0;
static uint64_t * threadtime = NULL;
static int * threadcount = NULL;

static pthread_mutex_t * countMutex = NULL;
static pthread_cond_t  * countCond  = NULL;

static void syntaxhelp(const char * msg);
static int parseArgs(int argc, char * * argv);
static void * topicTimerTest(void * arg, void * context, int value);

#define CLIENT_ID			"TopicTimerTest"
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
 * Test an echoed send and receive with commit
 */
int main(int argc, char * * argv) {
	uint64_t nanos = 0;
	uint64_t i;
	ism_threadh_t tid;
	int totalcount = 0;

	if (parseArgs(argc, argv) < 0) {
		return -1;
	}

	countMutex = malloc(sizeof(pthread_mutex_t));
	countCond  = malloc(sizeof(pthread_cond_t));

	pthread_mutex_init(countMutex, NULL);
	pthread_cond_init(countCond, NULL);

	threadtime = (uint64_t *)calloc(thread_count, sizeof(uint64_t));
	threadcount = (int *)calloc(thread_count, sizeof(int));

	for (i = 0; i < thread_count; i++) {
        ism_common_startThread(&tid, topicTimerTest, (void *)i, NULL, 0,
                   ISM_TUSAGE_NORMAL, 0, "topic timer test", "sender thread");
	}

	pthread_mutex_lock(countMutex);
	while (donecount < thread_count) {			/* BEAM suppression: infinite loop */
		pthread_cond_wait(countCond, countMutex);
	}
	pthread_mutex_unlock(countMutex);

	for (i = 0; i < thread_count; i++) {
		nanos += threadtime[i];
		if (threadcount[i] != count)
			printf("Count [%ld] = %d\n", i, threadcount[i]);
		totalcount += threadcount[i];
	}

	/*
	 * Print out statistics
	 */
	printf("Threads = %d\n", thread_count);
	printf("Time    = %.3f mils\n", nanos / 1000000.0);
	printf("Count   = %d\n", totalcount);
	printf("Rate    = %.3f / second\n", ((((double)totalcount/(double)nanos)*thread_count)*1000000000.0));

	free(threadcount);
	free(threadtime);

	return 0;
}

/*
 * Parse arguments
 */
static int parseArgs(int argc, char * * argv) {
	if (argc > 1) {
		msglen = atoi(argv[1]);
		if (msglen == 0) {
			syntaxhelp("Message length not valid");
			return -1;
		}
	}
	if (argc > 2) {
		count = atoi(argv[2]);
		if (count == 0) {
			syntaxhelp("");
			return -1;
		}
	}
	if (argc > 3) {
		thread_count = atoi(argv[3]);
		if (thread_count == 0) {
			syntaxhelp("");
			return -1;
		}
	}

	return 0;
}

/*
 * Put out a simple syntax help
 */
static void syntaxhelp(const char * msg)  {
	if (msg != NULL)
		printf("%s\n", msg);
	printf("TimerTest  msglen  count\n");
}

/*
 *
 * Send messages
 */
static void * topicTimerTest(void * arg, void * context, int value) {
	uint64_t action = (uint64_t)arg;
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
	ism_time_t startTime, endTime;
	ismc_message_t * bmsg, * rmsg;
	char * msgbytes;
	unsigned char b;

	sprintf(clientId, "%s%lu", CLIENT_ID, action);

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

		donecount++;
		pthread_cond_broadcast(countCond);

		return NULL;
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
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	sess = ismc_createSession(conn, 0, SESSION_AUTO_ACKNOWLEDGE);
	if (sess == NULL) {
		printf("Cannot create session\n");
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	requestT = ismc_createTopic("RequestT");
	if (requestT == NULL) {
		printf("Cannot create topic\n");
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	prod = ismc_createProducer(sess, requestT);
	if (prod == NULL) {
		printf("Cannot create producer\n");
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	cons = ismc_createConsumer(sess, requestT, NULL, 0);
	if (cons == NULL) {
		printf("Cannot create consumer\n");
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	rc = ismc_startConnection(conn);
	if (rc != 0) {
		printf("Cannot start connection\n");
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	startTime = ism_common_currentTimeNanos();

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
			printf("Send failed in %ld\n", action);
			break;
		}
		sendcount++;
	}
	for (i = 0; i < count; i++) {
		rc = ismc_receive(cons, 1000, &rmsg);
		if (rc == ISMRC_TimeOut) {
			printf("Timed out in %ld\n", action);
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

	free(msgbytes);

	endTime = ism_common_currentTimeNanos();

	ismc_closeConnection(conn);

//	printf("thread %ld sendcount %d recvcount %d time=%lu\n", action, sendcount, recvcount, (endTime-startTime));

	threadcount[action] = recvcount;
	threadtime[action] = (endTime - startTime);
	pthread_mutex_lock(countMutex);
	donecount++;
	pthread_cond_broadcast(countCond);
	pthread_mutex_unlock(countMutex);

	ismc_free(conn);
	ismc_free(sess);
	ismc_free(requestT);
	ismc_freeMessage(bmsg);
	ismc_free(prod);
	ismc_free(cons);

	return NULL;
}
