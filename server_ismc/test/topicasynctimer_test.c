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

static volatile long sendcount = 0;
static volatile long recvcount = 0;
static int  recvdone = 0;

static int donecount = 0;
static uint64_t * threadtime = NULL;
static int * threadcount = NULL;

static pthread_mutex_t * countMutex = NULL;
static pthread_cond_t  * countCond  = NULL;

static void syntaxhelp(const char * msg);
static int parseArgs(int argc, char * * argv);
static void * topicAsyncTimerTest(void * arg, void * context, int value);
static void messageListener(ismc_message_t * message, ismc_consumer_t * consumer, void * userdata);
static void errorListener(int rc, const char * error,
                       ismc_connection_t * connect, ismc_session_t * session, void * userdata);

#define CLIENT_ID			"TopicAsyncTimerTest"
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
	uint64_t millis = 0;
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
        ism_common_startThread(&tid, topicAsyncTimerTest, (void *)i, NULL, 0,
                   ISM_TUSAGE_NORMAL, 0, "topic async timer test", "sender thread");
	}

	pthread_mutex_lock(countMutex);
	while (donecount < thread_count) {               /* BEAM suppression: infinite loop */
		pthread_cond_wait(countCond, countMutex);
	}
	pthread_mutex_unlock(countMutex);

	for (i = 0; i < thread_count; i++) {
		millis += threadtime[i];
		if (threadcount[i] != count)
			printf("Count [%ld] = %d\n", i, threadcount[i]);
		totalcount += threadcount[i];
	}

	/*
	 * Print out statistics
	 */
	printf("Threads = %d\n", thread_count);
	printf("Time    = %ld\n", millis);
	printf("Count   = %d\n", totalcount);
	printf("Rate    = %.3f / second\n", ((((double)totalcount/(double)millis)*thread_count)*1000.0));

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
 * Send messages
 */
static void * topicAsyncTimerTest(void * arg, void * context, int value) {
	uint64_t action = (uint64_t)arg;
	char clientId[sizeof(CLIENT_ID) + 1];

	ismc_connection_t 	* conn;
	ismc_session_t 		* sess;
	ismc_destination_t 	* requestT;
	ismc_producer_t 	* prod;
	ismc_consumer_t 	* cons;

	ism_field_t field;

	int    rc;
	char * server = getenv("ISMServer");
	char * port   = getenv("ISMPort");
	char * transport = getenv("ISMTransport");
	char * recv_buffer = getenv("ISMRecvBuffer"); /* Receive buffer */
	char * send_buffer = getenv("ISMSendBuffer"); /* Send buffer */
	int    serverPort;

	ism_time_t startTime, endTime;
	ismc_message_t * bmsg;
	char * msgbytes;
	int i;

	sprintf(clientId, "%s%lu", CLIENT_ID, action);

	if (!server) {
		server = SERVER_ADDRESS;
	}

	if (port) {
		serverPort = atoi(port);
	} else {
		serverPort = DEFAULT_PORT;
	}

	conn = ismc_createConnectionX(clientId, NULL, server, serverPort);
	if (!conn) {
		printf("Cannot create connection\n");

		donecount++;
		pthread_cond_broadcast(countCond);

		return NULL;
	}

	if (port) {
		ismc_setStringProperty(conn, "Port", port);
	}

	if (transport) {
		ismc_setStringProperty(conn, "Protocol", transport);
	}

	if (recv_buffer) {
		ismc_setIntProperty(conn, "RecvSockBuffer", atoi(recv_buffer), VT_Integer);
	}
	if (send_buffer) {
		ismc_setIntProperty(conn, "SendSockBuffer", atoi(send_buffer), VT_Integer);
	}

	field.type = VT_Boolean;
	field.val.i = 1;
	ismc_setProperty(conn, "DisableMessageTimestamp", &field);
	ismc_setProperty(conn, "DisableMessageID", &field);

	rc = ismc_connect(conn);
	if (rc) {
		printf("Cannot connect\n");
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	ismc_setErrorListener(conn, errorListener, NULL);

	sess = ismc_createSession(conn, 0, SESSION_AUTO_ACKNOWLEDGE);
	if (!sess) {
		printf("Cannot create session\n");
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	requestT = ismc_createTopic("RequestT");
	if (!requestT) {
		printf("Cannot create topic\n");
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	prod = ismc_createProducer(sess, requestT);
	if (!prod) {
		printf("Cannot create producer\n");
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	cons = ismc_createConsumer(sess, requestT, NULL, 0);
	if (!cons) {
		printf("Cannot create consumer\n");
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	ismc_setlistener(cons, messageListener, NULL);

	rc = ismc_startConnection(conn);
	if (rc) {
		printf("Cannot connect\n");
		donecount++;
		pthread_cond_broadcast(countCond);
		return NULL;
	}

	startTime = ism_common_currentTimeNanos();

	bmsg = ismc_createMessage(sess, MTYPE_BytesMessage);
	ismc_setDeliveryMode(bmsg, ISMC_NON_PERSISTENT);
	msgbytes = calloc(msglen, 1);

	ismc_setIntProperty(bmsg, "fred", 4, VT_Integer);

	field.type = VT_Double;
	field.val.d = 3.1415926535;
	ismc_setProperty(bmsg, "pi", &field);

	for (i = 0; i < count; i++) {
		int oldval;

		msgbytes[0] = i&0xFF;
		ismc_setContent(bmsg, msgbytes, msglen);
		rc = ismc_send(prod, bmsg);
		if (rc) {
			char buf[512];
			ismc_getLastError(buf, sizeof(buf));
			printf("Send failed in %ld (#%d) - %s\n", action, i, buf);
			recvdone = 1;
			break;
		}

		oldval = __sync_fetch_and_add(&sendcount, 1);
		if (((oldval + 1) % 500) == 0) {
			ism_common_sleep(100000);
		}
	}

	free(msgbytes);

	while (!recvdone) {              /* BEAM suppression: infinite loop */
		ism_common_sleep(100000);
	}

	endTime = ism_common_currentTimeNanos();

	ismc_closeProducer(prod);
	ismc_closeConsumer(cons);
	ismc_closeSession(sess);
	ismc_closeConnection(conn);

	ismc_free(prod);
	ismc_free(cons);
	ismc_free(sess);
	ismc_free(conn);
	ismc_freeMessage(bmsg);

	printf("thread %ld sendcount %ld recvcount %ld time=%ld\n", action, sendcount, recvcount, (endTime-startTime) / 1000000);

	pthread_mutex_lock(countMutex);
	threadcount[action] = recvcount;
	threadtime[action] = (endTime - startTime) / 1000000;
	donecount++;
	pthread_cond_broadcast(countCond);
	pthread_mutex_unlock(countMutex);

	return NULL;
}

static void messageListener(ismc_message_t * message, ismc_consumer_t * consumer, void * userdata) {
    /* Validate the message */
    unsigned char b;
    int oldval;

    if (recvdone) {
    	return;
    }

	ismc_getContent(message, (char*)&b, 0, 1);
//	if (ismc_getContentSize(message) != msglen || b != (unsigned char)(recvcount & 0xFF)) {
	if (ismc_getContentSize(message) != msglen) {
		printf("Message received is not the correct message: "
			   "expected(%d) found(%d) "
			   "expected length(%d) found length(%d)\n",
			   (unsigned char)(recvcount&0xFF), b, msglen, ismc_getContentSize(message));

	    recvdone = 1;
		return;
	}

	oldval = __sync_fetch_and_add(&recvcount, 1);
    if (++oldval == (count * thread_count)) {
        recvdone = 1;
    }
}

static void errorListener(int rc, const char * error,
                       ismc_connection_t * connect, ismc_session_t * session, void * userdata) {
	printf("Error: %s in session %d\n", error, (session?session->sessionid:0));
}
