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

#ifndef __TCPCLIENT_H
#define __TCPCLIENT_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>

#include "mqttbench.h"
#include "mqttclient.h"

//#define IO_PROC_THREAD_MAX_JOBS 102400   /* can hold up to 100K jobs */
#define IO_PROC_THREAD_MAX_JOBS  500000
//#define IO_PROC_THREAD_MAX_JOBS   3000000  /* can hold up to 3M jobs */

#define INVALID_SOCKET SOCKET_ERROR
#define closesocket(s) close(s)
#define TCPSOCKET_COMPLETE 0        /* socket operation completed successfully */
#define TCPSOCKET_INTERRUPTED -2	/* must be the same as SOCKETBUFFER_INTERRUPTED */
#if !defined(SOCKET_ERROR)
	#define SOCKET_ERROR -1			/* error in socket operation */
#endif

#if !defined(max)
#define max(A,B) ( (A) > (B) ? (A):(B))
#endif

/* Maximum number of Stored Socket Errors. */
#define MAX_NUM_SOCKET_ERRORS	10
#define MAX_ERROR_LINE_LENGTH   256

/***************************************************************************************
 * transport_t states
 *
 * The following constants define the transport states
 *
 * In addition, these constants are returned from OpenSSL API calls (defined in
 * openssl/ssl.h):
 *
 *      SSL_ERROR_NONE,
 *      SSL_ERROR_WANT_READ,
 *      SSL_ERROR_WANT_WRITE,
 *      SSL_ERROR_ZERO_RETURN
 *
 ***************************************************************************************/
#define READ_WANT_READ			   0x1
#define READ_WANT_WRITE			   0x2
#define WRITE_WANT_READ			   0x4
#define WRITE_WANT_WRITE		   0x8
#define SOCK_CAN_READ			   0x10
#define SOCK_CAN_WRITE			   0x20
#define HANDSHAKE_IN_PROCESS	   0x40
#define SHUTDOWN_IN_PROCESS		   0x80
#define SOCK_CONNECTED			   0x100
#define SOCK_DISCONNECTED		   0x200
#define SOCK_ERROR				   0x400
#define SOCK_NEED_CONNECT          0x800
#define SOCK_NEED_CREATE           0x1000
#define SOCK_WS_IN_PROCESS         0x2000
#define DEFERRED_SHUTDOWN		   0x4000

#define TRANS_CHECK_STATES      (HANDSHAKE_IN_PROCESS | SOCK_CONNECTED | SOCK_DISCONNECTED | SOCK_ERROR | SHUTDOWN_IN_PROCESS)
#define IOP_TRANS_CONTINUE      0
#define IOP_TRANS_SHUTDOWN      1
#define IOP_TRANS_IGNORE       -1
#define IOP_TRANS_CONTINUE_TLS  2
#define APP_DISCONN_REQ			0xFFFFFFFF

#define CAN_READ(STATE)		((((STATE) & READ_WANT_WRITE) && ((STATE) & SOCK_CAN_WRITE)) || (((STATE) & SOCK_CAN_READ) && !((STATE) & WRITE_WANT_READ)))
#define CAN_WRITE(STATE)	((((STATE) & WRITE_WANT_READ) && ((STATE) & SOCK_CAN_READ)) || ((STATE) & SOCK_CAN_WRITE))

/***************************************************************************************
 * Important note on the I/O Processor Callback.  There are 2 parameters that will
 * be passed in, but one of them may be a NULL.  The actual callback needs either the
 * IOP or the Transport and it must check prior to moving forward.
 ***************************************************************************************/
/* I/O Processor Callback. */
typedef void (*iopjob_callback_t)(void * dataIOP, void * dataTrans);

typedef struct randomRStruct {
	char *randomIOProcStateBfrs;
	struct random_data *randomIOProcData;
} randomRStruct;

/* I/O Processor job */
typedef struct ioProcJob {
	transport		   *trans;			/* client transport to be serviced */
	uint32_t			events;		    /* I/O event types which occurred on this client */
	int 				rsrv;
	iopjob_callback_t   callback;
} ioProcJob;

typedef struct ioProcJobsList {
	ioProcJob     *jobs;
	int            allocated;
	int            used;
} ioProcJobsList;


/*
 * I/O Processor thread object
 */
typedef struct ioProcThread_t {
	uint64_t 			 currRxMsgCounts[MQTT_NUM_MSG_TYPES];	/* RX array of MQTT message types counters for receive (per I/O processor thread) */
	uint64_t 			 currTxMsgCounts[MQTT_NUM_MSG_TYPES];	/* TX array of MQTT message types counters for send (per I/O processor thread) */
	uint64_t             currRxQoSPublishCounts[MAX_NUM_QOS];   /* RX array for QoS publish message type counters (per I/O processor thread) */
	uint64_t             currTxQoSPublishCounts[MAX_NUM_QOS];   /* TX array for QoS publish message type counters (per I/O processor thread) */

	uint64_t 			 prevRxMsgCounts[MQTT_NUM_MSG_TYPES];	/* RX array of MQTT message types counters for receive (per I/O processor thread) to keep counts from last time statistics were calculated */
    uint64_t 			 prevTxMsgCounts[MQTT_NUM_MSG_TYPES];	/* TX array of MQTT message types counters for send (per I/O processor thread) to keep counts from last time statistics were calculated */
	uint64_t             prevRxQoSPublishCounts[MAX_NUM_QOS];   /* RX array for QoS publish message type counters (per I/O processor thread) */
	uint64_t             prevTxQoSPublishCounts[MAX_NUM_QOS];   /* TX array for QoS publish message type counters (per I/O processor thread) */

	ism_byteBufferPool   recvBuffPool;							/* buffer pool used for network reads. Recv buffers also used for sending ACKs */
	ism_byteBufferPool   sendBuffPool;                          /* TX buffer pool for the iop, consumers and producers */
	ism_byteBuffer       txBuffer;                              /* TX Buffer List for iop. */
	int                  numBfrs;
	pthread_spinlock_t	 jobListLock;							/* provides synchronized access to the jobs list */
	pthread_spinlock_t   counterslock;                          /* provides synchronized access to the counters. */
	ioProcJobsList       jobsList[2]; 							/* Array for job lists. Currently supports 2. */
    ioProcJobsList	    *currentJobs;							/* current list of client transports to be serviced by this I/O processor thread */
    ioProcJobsList	    *nextJobs;								/* 2nd/next list of client transports to be serviced by this I/O processor thread */
    transport 		    *cleanupReqList;						/* list of transports that are in shutdown and need to be cleaned up */
    volatile int  	     isStopped;
    ism_threadh_t 	     threadHandle;
    double        	     prevtime;           					/* previous time at which statistics were taken */
	latencystat_t       *rttHist;                               /* latency histogram of round trip latency CHKTIMERTT this is PUBLISH messages*/
	latencystat_t       *sendHist;                              /* latency histogram of PUB - PUBACK time (enabled with mask bit CHKTIMESEND) */
	latencystat_t       *tcpConnHist;                           /* latency histogram of createConnection call on a producer or consumer thread CHKTIMECONN */
	latencystat_t       *mqttConnHist;                          /* latency histogram of createConnection call on a producer or consumer thread CHKTIMECONN */
	latencystat_t       *subscribeHist;                         /* latency histogram of createConnection call on a producer or consumer thread CHKTIMECONN */
	latencystat_t       *reconnectMsgHist;                      /* Latency histogram of message latency during reconnect (pub timestamp is older than last MQTT CONNACK) */
	uint64_t             currRxDupCount;                        /* counter for rx duplicate messages */
	uint64_t			 currRxRetainCount;						/* counter for rx retained messages */

    /* The 2 counters for CONNECT and CONNACK are special for consumers. The 3rd one is for Disconnects. */
	uint64_t             currConsConnectCounts;                 /* counter for the number of CONNECT messages sent by consumers */
	uint64_t             currConsConnAckCounts;                 /* counter for the number of CONNACK messages recvd by consumers */
	uint64_t             currDisconnectCounts;                  /* counter for the number of Disconnects for clients assigned to this IOP. */
	uint64_t			 currDisconnectMsgCounts;				/* counter for the number of DISCONNECT messages recvd for clients assigned to this IOP. */

	uint64_t             currBadRxRCCount;                		/* counter for the number of bad return codes received by clients assigned to this IOP. */
	uint64_t             currBadTxRCCount;                		/* counter for the number of bad return codes transmitted by clients assigned to this IOP. */
	uint64_t			 currOOOCount;							/* counter for the number of out of order messages received assigned to this IOP thread */
	uint64_t			 currRedeliveredCount;					/* counter for the number of redelivered messages received assigned to this IOP thread */
	uint64_t             currFailedSubscriptions;               /* counter for the number of failed subscriptions by clients assigned to this IOP thread */

	ismHashMap			*streamIDHashMap;						/* Hash map of message stream objects, msgStreamObj, used for tracking out of order message delivery */
	int                  nextErrorElement;
	char               **socketErrorArray;
	struct random_data  *ioProcRandomData;
} ioProcThread_t;

/*
 * I/O Listener thread object
 */
typedef struct ioListenerThread_t {
	pthread_spinlock_t   cleanupReqListLock;		/* provides synchronized access to the cleanup list */
	transport           *cleanupReqList;			/* list of transports that are in shutdown and should be removed from epoll list */
	int                  efd;						/* epoll file descriptor */
    int                  pipe_wfd;                  /* Pipe for sending requests and shutdown */
    volatile int 		 isStopped;
    ism_threadh_t		 threadHandle;
} ioListenerThread_t;

/* Function signatures for reading and writing data on a transport */
typedef int (*read_data_t)(transport *trans, ism_byte_buffer_t *bb) ;
typedef int (*write_data_t)(transport *trans, ism_byte_buffer_t *bb, int numBytesToWrite) ;
typedef int (*on_connect_t)(transport *trans) ;
typedef int (*on_shutdown_t)(transport *trans) ;
typedef int (*on_ws_connect_t)(transport *trans);

/**
 * Structure to hold all transport related data for the mqttclient
 */
typedef struct transport_t {
	uint8_t isProcessing;    	        	/* This client is currently being processed an I/O Processor thread */
	uint8_t reconnect;                      /* Indicates if the reconnect is enabled (default: 1). */
	uint8_t sockInited;                     /* Socket options have been initialized */

	uint64_t processCount;					/* number of times this client has been processed as a job by an I/O Processor thread */
	int64_t epollCount;						/* this count is incremented whenever the transport is added to on epoll waiter thread list
	 	 	 	 	 	 	 	 	 	 	   and is decremented whenever the transport is removed from an epoll waiter thread list (not decremented if connection is closed)*/
	int     socket;					 		/* fd for the connection for this mqttclient */
	int     serverPort;						/* Destination port of ISM server */
	int     srcPort;					    /* Client source port (bind is done as an availability check and reduce time in TCP stack during connect) */
	char   *serverIP;						/* Destination IP address of the MQTT message broker */

	int jobIdx;								/* place in the IOP joblist */
	volatile int state;						/* Transport state used for secure and non-secure connections */
	uint64_t bindFailures;					/* Count of bind failures */

	volatile ioProcThread_t *ioProcThread;  /* The I/O processor thread assigned to handle this client */
	ioListenerThread_t *ioListenerThread;   /* The I/O listener thread assigned to handle this client */
	write_data_t doWrite;	          		/* callback used by this mqttclient to send messages (currently there is a secure and non-secure flavor) */
	read_data_t doRead; 			 		/* callback used by this mqttclient to receive messages (currently there is a secure and non-secure flavor)*/
	on_connect_t onConnect;					/* callback used to notify the application that a client transport has completed the SSL handshake */
	on_shutdown_t onShutdown;				/* callback used to shutdown the client transport (called from the I/O processor)*/
	on_ws_connect_t onWSConnect;            /* callback used to check the Server's response for a WebSocket connection. */
	ism_byte_buffer_t *currentSendBuff;     /* Current send buffer to be sent to network by I/O processor thread (dedicated single buffer per client transport) */
	ism_byte_buffer_t *pendingSendListHead; /* Head of the list of pending messages submitted by application producer thread */
	ism_byte_buffer_t *pendingSendListTail; /* Tail of the list of pending messages submitted by application producer thread */
	ism_byte_buffer_t *currentRecvBuff;     /* The buffer used to read data from the network (from pool) */
	SSL *sslHandle;                         /* one SSL handle per mqttclient */
	BIO *bioHandle;                         /* one SSL BIO handle per mqttclient */
	pthread_spinlock_t slock;				/* Controls concurrent access to the transport object of the mqttclient from ioListenerThreads, ioProcessorThreads, and publisher threads */
	mqttclient *client;                     /* A reference to the MQTT client which owns this transport object */
	transport *next;						/* Used for building a list of transports for cleanup */
	struct sockaddr_in serverSockAddr;
	struct sockaddr_in clientSockAddr;	    /* Used for explicit bind on a specified port */

	char    *wsClientKey;			   		/* The WebSocket client key */
	char    *wsAcceptStr;			   		/* The expected WebSocket accept string from the server response */
	char    *wsHandshakeMsg;		   		/* The WebSocket client handshake message */
	int		 wsHandshakeMsgLen;				/* Length of the WebSocket client handshake message */

	X509 *ccert;                            /* The fully qualified path of the SSL Certificate. */
	EVP_PKEY *cpkey;                        /* The fully qualified path of the SSL Key. */
} transport_t;

/* ***************************************************************************************
 * Exposed SPIs from tcpclient
 * ***************************************************************************************/
int  tcpInit(mqttbenchInfo_t *);	    									        /* Initialize TCP and SSL library initialization routines */
int  initWebSockets (mqttclient *client, int line);                                 /* Initialize WebSockets for the specified client */
SSL_CTX * initSSLCtx (char * sslClientMethod, const char * ciphers);                /* Initialize an SSL context for secure connections */
void tcpCleanup (void);															    /* cleanup TCP client */
int  removeTransportFromIOThread (transport_t *);
int  startIOThreads (mqttbenchInfo_t *, int);	                                    /* Allocate I/O Processor and Listener threads and start the threads */
int  stopIOThreads (mqttbenchInfo_t *);     									    /* Stop I/O Processor and Listener threads */
int  preInitializeTransport (mqttclient *, char *, int, char *, int, int, int);     /* Create and initialize an mqttclient transport object */
void addJob2IOP (volatile ioProcThread_t *, transport_t *, uint32_t, iopjob_callback_t);     /* Used to schedule a job on the IOP Job list. */
int  createSocket (transport_t * trans);                                            /* Create a socket */
int  createConnection (mqttclient *);											    /* create a connection to an ISM server */
int  submitMQTTConnect (transport *);
void performMQTTDisconnect (void *, void *);										/* iopjob_callback_t */
void performMQTTUnSubscribe (void *, void *);										/* iopjob_callback_t */
void resetLatencyStats (void *, void *);											/* iopjob_callback_t */
void scheduleReconnectCallback (void *dataIOP, void *dataTrans);					/* iopjob_callback_t */
void schedulePingCallback (void *dataIOP, void *dataTrans);							/* iopjob_callback_t */
int  submitCreateConnectionJob (transport_t * trans);
int  submitCreateSocketJob (transport_t * trans);
int  submitIOJob (transport_t * trans, ism_byte_buffer_t *bb);						/* Application producer and consumer threads submit I/O jobs to the transport layer through this interface */
int  submitJob4IOP (transport_t *);
int  submitMQTTDisconnectJob (transport_t * trans, int deferJob);
int  submitMQTTSubscribeJob (transport_t * trans);
int  submitMQTTUnSubscribeJob (transport_t * trans);
void submitResetLatencyJob (void);
void submitResetStatsJob (void);
int  submitSendRequest (transport_t * trans);

int  doRead (transport_t *trans, ism_byte_buffer_t *bb);							/* read data from a non-secure network connection */
int  doSecureRead (transport_t *trans, ism_byte_buffer_t *bb);						/* read data from a secure network connection */
int  doSecureWrite (transport_t *trans, ism_byte_buffer_t *bb, int numBytesToWrite); /* write data to a secure network connection */
int  doWrite (transport_t *trans, ism_byte_buffer_t *bb, int numBytesToWrite);		/* write data to a non-secure network connection */

int  connectionShutdown (transport_t *);
void forceDisconnect (mqttbench_t *);

int  obtainSocketErrors (char **, int);
int  resolveDNS2IPAddress (char *, destIPList **);
int  updateDNS2IPAddr (char *, char *);
int  testConnectSrcToDest (char *, struct sockaddr_in *, uint8_t);

int  getBufferSize (const char * ssize, const char * defaultSize);					/* read the buffer size configuration string (e.g "16K" is 16KB or "8M" is 8MB */
int  initIOPTxBuffers (mqttbench_t *, int);                                         /* obtains buffers from IOPs for the mbinst (producer, consumer) */
ism_byteBuffer getIOPTxBuffers (ism_byteBufferPool, int, int);                      /* obtain buffers from the IOP buffer pool. */

int basicConnectAndSend(char *dstIP, uint16_t dstPort, char *buf, int buflen);

#endif  /* __TCPCLIENT_H */
