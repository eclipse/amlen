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

#ifndef __MQTTBENCH_H
#define __MQTTBENCH_H

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <dirent.h>
#include <assert.h>
#include <ismutil.h>
#include <ismregex.h>
#include <protoex.h>

#include "mbconstants.h"
#include "mqttpacket.h"

/* ***************************************************************************************
 * Typedefs
 * ***************************************************************************************/
typedef struct transport_t transport;
typedef struct mqttclient_t mqttclient;
typedef struct mqttbench_t mqttbench;
typedef struct ioProcThread_t ioProcThread;
typedef struct mqttbenchInfo_t mbInfo;
typedef struct destTuple_t destTuple;
typedef struct dstIPList_t destIPList;

/* ------------------------------------------------------------------------------------
 *  Define for mqttmessage for mqttbench.
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	int       		qos;					/* TX/RX - PUBLISH/SUBSRIBE */
	int       		retain;					/* TX	 - PUBLISH	 		*/
	int       		marked;					/* TX 	 - PUBLISH			*/
	int       		useseqnum;				/* TX	 - PUBLISH	 		*/
	int       		payloadlen;     		/* TX/RX - PUBLISH - length of the payload */
	uint16_t  		msgid;					/* TX/RX - PUBLISH/SUBSCRIBE/PUBXXX  */
    uint16_t  		topic_alias;			/* TX/RX - PUBLISH			*/
    uint32_t  		subID;					/* TX/RX - SUBSCRIBE/PUBLISH*/
    double  		senderTimestamp; 		/* RX	 - PUBLISH - time units determined on send side */
    uint64_t  		streamID;				/* RX    - PUBLISH - identifier of the publisher/topic combination from which this message came */
    uint64_t  		seqnum;					/* RX	 - PUBLISH - monotonically increasing sequence number per stream */
	uint32_t  		sessionExpiryTTL;		/* RX	 - CONNACK - client session state TTL on server */
	uint16_t  		serverMaxInflight;		/* RX	 - CONNACK - server specified max inflight for QoS > 0 */
	uint16_t  		topicAliasMaxOut;		/* RX 	 - CONNACK - the highest number topic alias that the server will accept */
	uint8_t   		serverMaxQoS;			/* RX 	 - CONNACK - the max QoS supported by the server */
	uint8_t	  		serverSupportRetain;	/* RX 	 - CONNACK - indicates if the server supports retained messages */
    char     		*payload;				/* TX	 - PUBLISH 	*/
    uint8_t         dupflag;                /* TX    - PUBLISH  */
    mqttclient   	*client;				/* reference to the mqtt client object */
    uint8_t			sendTopicName;			/* flag to indicate that the topic name should be sent in the PUBLISH message */
    uint8_t			sendTopicAlias;			/* flag to indicate that the topic alias should be sent in the PUBLISH message */
    char            *msgType;               /* String that contains the message type for Trace messages. */
    char            reasonMsg[REASON_STR_LEN]; /* Reason string provided by the server. */
    uint16_t		reasonMsgLen;			/* Length of the Reason string provided by the server */
    uint8_t         generatedMsg;			/* flag to indicate that this is a generatedMsg (i.e. from the -s <min>-<max> cmdline param) */
} mqttmessage_t;

typedef struct messagedir_object_t messageDirObj;

typedef struct message_object_t {
	char		     *filePath;			// file path to the JSON message file which is the serialized form of this message object
    									/* there are 3 ways for the user to specify the message payload source: external file path, inline, or size.  They
    									 * are mutually exclusive. */
	char		     *payloadFilePath;	// the user specified a payloadFile as the source of the message payload, this is the path to the payload file
    char		     *payloadStr;		// the user specified the message payload inline in the payload field of the message file
    int   		      payloadSizeBytes; // the user specified a message payload size (bytes), mqttbench will generate a binary payload of the specified size

	concat_alloc_t   *payloadBuf;		// buffer used to hold the message payload
	destTuple	     *topic;			// the message file may optionally specify a single topic to publish this message to
										// If specified, this topic will override the default round robin selection from the clients
										// publish topics list (i.e. destTxList)
	messageDirObj    *msgDirObj;		// a reference to the messagedir_object_t that this object belongs to

	/* MQTT v5 specific fields */
	uint8_t		       numUserProps;	// number of MQTT User-Defined properties
	mqtt_user_prop_t **userPropsArray;	// array of MQTT User-Defined properties provided in the message file
	uint8_t			   numMQTTProps;	// number of MQTT Spec-Defined property IDs
	uint8_t			  *propIDsArray;	// array of MQTT Spec-Defined property IDs provided in the message file (value is stored in propsBuf)
	concat_alloc_t    *propsBuf;		// buffer used to hold Spec-Defined AND User-Defined MQTT properties
} message_object_t;

typedef struct messagedir_object_t {
	uint8_t			  generated;		// flag to indicated whether these messages are binary generated messages
	char		     *dirPath;			// path of the mqttbench message file directory (maybe be NULL, e.g. in the case of -s parameter)
	int				  maxPayloadLen;    // the largest message payload of the message files processed in this directory
	int				  maxTopicLen;    	// the largest topic length of the message files processed in this directory (if a topic is specified in the message file)
	int               maxPropsLen;      // the largest MQTT properties buffer of the message files processed in this directory
	int				  numMsgObj;		// number of objects in the msgObjArray
	message_object_t *msgObjArray;		// array of message objects
} messagedir_object_t;

typedef struct {
	uint8_t    msgSource;                /* The source for this particular array of messages. */
	uint8_t    startIndex;               /* Used for initialization of the file to be sent. */
	uint8_t    numMsgSizes;              /* Number of different message sizes. */
	uint8_t    rsrvd;

	int        minMsg;                   /* The minimum message size. */
	int        maxMsg;                   /* The maximum message size. */
	int       *msgSizeArray;             /* Array that contains the sizes of the message. */

	char      *msgDirectory;             /* Directory Name(s) for the predefined messages. */
	char     **msgFilenameArray;         /* Array that contains the filenames for the predefined msg files. */
	char     **msgBfrArray;
} messagePayload_t;

typedef struct {
	uint8_t     srcOfAllMsgs;            /* Indicates all the type of messages being used. */
	uint8_t     rsrvd[3];

	int         numMsgPayloads;
	messagePayload_t **arrayMsgPtr;
} arrayMsgPayload_t;

/* ------------------------------------------------------------------------------------
 * Structure that contains message configuration information
 * ------------------------------------------------------------------------------------ */
typedef struct {
	int		   specSendMsgLenZero;     	 /* -s 0-0 was specified on the command line, i.e. send only zero-length messages */
	int        minMsg;                   /* min range of message size (bytes) -s <min>-<max> */
	int        maxMsg;                   /* max range of message size (bytes) -s <min>-<max> */

	ismHashMap *msgDirMap;     			 /* hashmap of message directory objects, created from -s or -M command line params, or messageDirPath field of client in client list */

	/* deprecate these fields */
	int        numMsgSizes;              /* Number of different message sizes. */
	int       *msgSizeArray;
	char     **msgFilenameArray;
	arrayMsgPayload_t *pArrayMsg;
} mbMsgConfigInfo_t;

/* ------------------------------------------------------------------------------------
 * Structure that contains system environment settings.
 * ------------------------------------------------------------------------------------ */
typedef struct {
	uint8_t  autoRenameFiles;            /* 1 = rename latency and csv files every 24 hrs. */
	uint8_t  batchingDelay;              /* 1 = add a delay in IOProc threads to allow batching ; 0 = no delay in IOProc threads */
	uint8_t  mqttbenchTLSBfrPool;        /* Manage TLS Buffer Pool : 1 = mqttbench ; 0 = Server Utils/OpenSSL. */
	uint8_t  clkSrc;                     /* Clock source to be used: 0=TSC, 1=GTOD */

	uint8_t  disableLatencyWarn;         /* If set = 1 then will NOT schedule a timer for warning latency numbers are greater than histogram. */
	uint8_t  disableMessageTimeStamp;
	uint8_t	 disableMessageID;
	uint8_t  rsrvd_1;

	uint8_t  numIOListenerThreads;       /* Number of I/O Listener Threads. */
	uint8_t  numIOProcThreads;           /* Number of I/O Processor Threads. */
	uint8_t  rsrvd[2];

	uint8_t  pipeCommands;               /* 0 = read commands from STDIN (i.e. it is run interactively in the foreground), 1 = read commands from a named pipe (background) */
	uint8_t  portRangeOverlap;           /* Flag to indicate there is a port range overlap (kernel setting) */
	uint8_t  useEphemeralPorts;          /* Indication to let OS select the source port using the ephemeral source port selection algorithm (do NOT bind source port) */
	uint8_t  useNagle;                   /* Whether to enable (1) or disable (0) Nagles Algorithm (default = 0) */

	uint64_t lingerTime_ns;              /* Time (Secs) converted to nano secs to linger before disconnecting. */
	uint64_t totalSystemMemoryMB;        /* Amount of memory on the machine. */
	uint16_t assignedCPU;				 /* Number of CPU that mqttbench is assigned to */
	uint8_t  numSMT;					 /* the number of hardware threads per core (1 = no hyperthreading, 2 = hyperthreading enabled) */

    int32_t  maxNumTXBfrs;               /* Maximum # of TX Buffers to allocate */
    int32_t  tx_NumAllocatedBfrs;

	int      addRetry;
	int	 	 recvBufferSize;
	int      sendBufferSize;
	int		 recvSockBuffer;
	int		 sendSockBuffer;
	int      buffersPerReq;              /* # of buffers per request from IOP TX Buffer Pool. */

	int      delaySendTimeUsecs;         /* Number of microseconds to sleep between initial TCP connections PER SUBMITTER THREAD */
	int      delaySendCount;             /* Number of consecutive initial TCP connections per sleep PER SUBMITTER THREAD */

	int      mqttKeepAlive;              /* Value to set the keep alive for the mqtt connection. */
	int      numUsers;                   /* Identifies how many userids to use for LDAP Authentication. */
	int      initConnRetryDelayTime_ns;  /* The initial retry delay to be used for retrying connections.  Stored in nanoSeconds. */
	int      sourcePortLo;               /* Store the sourcePortLo value. */

	int      maxConnHistogramSize;       /* Max size of Connection Latency histogram.  Can be specified in env var (default: 100000) */
	int      maxMsgHistogramSize;        /* Max size of Message Latency (RTT) histogram.  Can be specified in env var (default: 100000) */
	int      maxReconnHistogramSize;     /* Max size of Message Latency (RTT) during reconnects histogram.  Can be specified in env var (default: 100000) */
	int      maxHgramSize;               /* The size of the largest of the 2 histograms. */
	int      maxRetryAttempts;           /* Max number of retry attempts before disconnecting */
	int      memTrimSecs;                /* Time (Secs) interval between performing malloc_trim */
    int      numEnvSourceIPs;            /* Number of Source IP addresses set in the SIPList Env Variable. */
	int      sampleRate;                 /* Sampling rate latency measurements per second (i.e. number of times to read the clock per second) */
	char    *sslClientMethod;		     /* Possible values: SSLv23, SSLv3, TLSv1, TLSv11, TLSv12 (default) */
	char    *sslCipher;				     /* Possible values: RSA (default) */
	int      streamIDMapSize;            /* Size of the per IOP thread hash map used for tracking out of order message delivery per stream */
	char    *negotiated_sslCipher;	     /* Negotiated SSLCipher */
	char    *strSIPList;                 /* SIPList */
	char    *LTPAToken;                  /* The LTPA Token which is read in. */
	char    *GraphiteIP;				 /* IPv4 address of the Graphite server to send metrics to */
	int		 GraphitePort;				 /* Port number that the Graphite server is listening on, currently supports Non-TLS connections only */
	char    *GraphiteMetricRoot;		 /* A user provided metric path root to which all metrics will be appended
	 	 	 	 	 	 	 	 	 	 	<GraphiteMetricRoot>.<mqttbench instance id>.<metric name>, the default metric root is loadtests.mqttbench */
	double   connRetryFactor;
} environmentSet_t;

/* ------------------------------------------------------------------------------------
 * Structure that contains the command line arguments
 * ------------------------------------------------------------------------------------ */
typedef struct {
	uint8_t  determineJitter;            /* Flag to indicate jitter is enabled (1) or not (0) */
	uint8_t  displayEnvInfo;
	uint8_t  dumpClientInfo;             /* Dump the client information to a file (mb_ClientInfo.txt) */

	uint8_t  exitOnFirstDisconnect;      /* If set then exit on the first disconnect. */

	int8_t   rateControl;                /* Determines what form of rate control to use by producer threads in mqttbench
										  *      rateControl == -2          ; no rate control with random client/topic selection
	                                      *      rateControl == -1          ; no rate control
	                                      *      rateControl == 0 (default) ; individual producer threads control their own rate */
	uint8_t  reconnectEnabled;           /* -noreconn     = Disable reconnect. */
	uint8_t  disconnectType;         	 /* -quit         = When terminating only close connection if set = 1. */
	uint8_t  roundRobinSendSub;          /* -sseq         = Flag to indicate that subscription should be done in round robin fashion. */
    uint8_t  noWaitFlag;				 /* -nw           = Flag to indicate that submitter threads should NOT wait for all clients to be connected/subscribed
                                                            before publishing can begin */

	uint8_t  snapStatTypes;              /* -snap         = The type of stats to collect for snap. */
	uint8_t  traceLevel;                 /* -tl           = The level to set the tracing at. */
	uint8_t  msgSizeRangeParmFlag;		 /* -s		      = Indicates whether or not the -s parameter was passed to mqttbench */
	uint8_t  msgDirParmFlag;			 /* -M		      = Indicates whether or not the -M parameter was passed to mqttbench */

	uint8_t  reresolveDNS;               /* -nodnscache   = Indicates whether to resolve the DNS on a reconnect. */
	uint8_t  rsrvd[3];

	int		 mqttbenchID;				 /* -i            = The ID of this instance of mqttbench, default is 0 */
	int      snapInterval;               /* -snap         = The interval between snaps, which is collecting stats. */

	int      appSimLoop;                 /* -as           = busy loop used to simulate an application processing a message */
	int      beginStatsSecs;             /* -bs           = How long to wait before resetting clock to restart statistics */
	int      latencyMask;                /*               = Mask of latency measurements (see CHK* constants in the constants section above) */
	int      orgLatencyMask;             /*               = Mask of latency measurements (see CHK* constants in the constants section above) */
	int      globalMsgPubCount;          /* -c            = The total number of messages sent by all producers.  message count (-c)
	                                      *                 and test duration (-d) are mutually exclusive,  test duration takes
	                                      *                 precedence over msg count */
	int      testDuration;               /* -d            = Duration in seconds of the test */
	int      maxInflightMsgs;            /* -mim          = Maximum number of inflight message IDs (default = 65535 == (64K - 1)). */
	int      maxMsgIDSearch;             /* -mim          = Maximum number of Msg ID Searches to make. */
	int      maxPubRecSearch;            /* -mim          = Maximum number of PUBREC Searches to make. */
	int      sendPingReqFreq;            /* -p            = Frequency to send PINGREQ to server from client. */

	int      resetLatStatsSecs;          /* -rl           = Number of seconds to wait before clearing latency stats. */

	int      resetTStampXMsgs;           /* -rr           = Number of messages to receive before starting statistics on receiver */
	int      numSubmitterThreads;        /* -st           = Number of submitter threads to be used for this instance (default = 1) */
	int      chkMsgMask;                 /* -V            = Mask for checking info (i.e. length, seq #) of a message. */

	uint64_t lingerTime_ns;              /* -l            = Time (Secs) converted to nano seconds to linger around before disconnecting. */

	char        *clientListPath;         /* -cl           = The fully qualified name of the client list file. */
	char        *crange;                 /* -crange       = The range of clients to use from the client list. */
	ism_regex_t clientTraceRegex;        /* --clientTrace = A regular expression used to match client IDs for client for which low level trace should be enabled */
	char        *clientTraceRegexStr;    /*               = the string passed on the command line used to compile the regex */
	char        *stringCmdLineArgs;      /* The command line arguments passed. */
} mbCmdLineArgs_t;

/* ------------------------------------------------------------------------------------
 * Structure that contains the information needed for burst mode.
 * ------------------------------------------------------------------------------------ */
typedef struct {
	uint64_t burstDuration;              /* How long burst mode should last (in seconds) */
	uint64_t burstInterval;              /* How often burst mode should occur (in seconds) */

	double   baseMsgRate;                /* Base Message Rate. */
	double   burstMsgRate;               /* Burst Message Rate. */

	uint8_t  currMode;                   /* Flag: 0=Base Rate, 1=Burst Rate */
	uint8_t  rsrvd[3];

	struct TimerTask_t * timerTask;
} mbBurstInfo_t;

/* ------------------------------------------------------------------------------------
 * Structure that contains Timer Information.
 * ------------------------------------------------------------------------------------ */
typedef struct {
	ism_timer_t clientScanKey;
	ism_timer_t snapTimerKey;
	ism_timer_t delayTimeoutTimerKey;
	ism_timer_t resetLatTimerKey;
	ism_timer_t autoRenameTimerKey;
	ism_timer_t memTrimTimerKey;
	ism_timer_t burstModeTimerKey;
	ism_timer_t warningLatTimerKey;
	ism_timer_t jitterKey;
} mbTimerInfo_t;

/* ------------------------------------------------------------------------------------
 * Structure that contains SSL Buffer Pool Info.
 * ------------------------------------------------------------------------------------ */
typedef struct {
	uint32_t sslBufferPoolMemory;        /* # of buffers to use for SSL Buffer Pool set by env var (TLSBufferPoolSize). */
	uint32_t pool64B_numBfrs;
	uint32_t pool128B_numBfrs;
	uint32_t pool256B_numBfrs;

	uint32_t pool512B_numBfrs;
	uint32_t pool1KB_numBfrs;
	uint32_t pool2KB_numBfrs;
	uint32_t rsrvd;

	uint64_t pool64B_totalSize;
	uint64_t pool128B_totalSize;
	uint64_t pool256B_totalSize;
	uint64_t pool512B_totalSize;
	uint64_t pool1KB_totalSize;
	uint64_t pool2KB_totalSize;
} mbSSLBufferInfo_t;

/* ------------------------------------------------------------------------------------
 * Structure that contains list information about the client threads
 * ------------------------------------------------------------------------------------ */
typedef struct {
	uint8_t  numConsThreads;             /* Number of threads with just consumers. */
	uint8_t  numProdThreads;             /* Number of threads with just producers. */
	uint8_t  numDualThreads;             /* Number of threads with mixed clients. */
	uint8_t  rsrvd;
} mbThreadInfo_t;

/* ------------------------------------------------------------------------------------
 * Structure for holding Jitter Data
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	int      sampleCt;
	int      currIndex;

	/* Consumer/Subscriber Min and Max Rates based on QoS */
	int      cons_PUBLISH_MinRate;
	int      cons_PUBLISH_MaxRate;
	int      cons_PUBACK_MinRate;
	int      cons_PUBACK_MaxRate;
	int      cons_PUBREC_MinRate;
	int      cons_PUBREC_MaxRate;
	int      cons_PUBREL_MinRate;
	int      cons_PUBREL_MaxRate;
	int      cons_PUBCOMP_MinRate;
	int      cons_PUBCOMP_MaxRate;

	/* Producer/Publisher Min and Max Rates based on QoS */
	int      prod_PUBLISH_MinRate;
	int      prod_PUBLISH_MaxRate;
	int      prod_PUBACK_MinRate;
	int      prod_PUBACK_MaxRate;
	int      prod_PUBREC_MinRate;
	int      prod_PUBREC_MaxRate;
	int      prod_PUBREL_MinRate;
	int      prod_PUBREL_MaxRate;
	int      prod_PUBCOMP_MinRate;
	int      prod_PUBCOMP_MaxRate;

	int     *rateArray_Cons_PUB;
	int     *rateArray_Cons_PUBACK;
	int     *rateArray_Cons_PUBREC;
	int     *rateArray_Cons_PUBREL;
	int     *rateArray_Cons_PUBCOMP;

	int     *rateArray_Prod_PUB;
	int     *rateArray_Prod_PUBACK;
	int     *rateArray_Prod_PUBREC;
	int     *rateArray_Prod_PUBREL;
	int     *rateArray_Prod_PUBCOMP;

	int8_t   rxQoSMask;
	int8_t   txQoSMask;
	uint8_t  wrapped;
	uint8_t  rsvrd;

	uint64_t interval;

	uint64_t prod_Tx_Prev_PUB;
	uint64_t prod_Rx_Prev_PUBACK;
	uint64_t prod_Rx_Prev_PUBREC;
	uint64_t prod_Tx_Prev_PUBREL;
	uint64_t prod_Rx_Prev_PUBCOMP;

	uint64_t cons_Rx_Prev_PUB;
	uint64_t cons_Tx_Prev_PUBACK;
	uint64_t cons_Tx_Prev_PUBREC;
	uint64_t cons_Rx_Prev_PUBREL;
	uint64_t cons_Tx_Prev_PUBCOMP;

	double   prevTime;

	struct TimerTask_t * timerTask;
} mbJitterInfo_t;

/* ------------------------------------------------------------------------------------
 * mqttbench_t structure
 * ------------------------------------------------------------------------------------ */
typedef struct mqttbench_t {
	ism_byteBuffer txBfr[MAX_IO_PROC_THREADS];   /* An array of linked-list of buffers, one linked-list per IOP TX Buffer Pool */

	double     tokenIn;                  /* putting tokens in the bucket allows the producer to go faster */
	double     tokenOut;                 /* taking tokens out of the bucket slows the producer down */
	double     prevtime;                 /* previous time at which statistics were taken */
	double     tcpConnTime;              /* The total calculated tcp connection time for this thread. */
	double     mqttConnTime;             /* The total calculated mqtt connection time for this thread. */
	double     subscribeTime;            /* The total calculated subscribe time for this thread. */
	double     startSubscribeTime;       /* Initial Subscribe Start Time. */
	double     startTCPConnTime;         /* Initial TCP Start Connection time. */
	double     startMQTTConnTime;        /* Initial MQTT Start Connection time. */

	uint8_t    rateChanged;              /* flag to indicate the rate has changed */
    uint8_t    clean;                    /* if not set (0) then server must store the subscription */
	uint8_t    clientType;               /* Client Types associated with this mbinst (thread). */

    uint8_t    rxQoSBitMap;              /* QoS bit is set if any QoS 0, 1, or 2 messages subscribed by ANY client assigned to this submitter thread (e.g. 1 = QoS 0 only, 3 = QoS 0 and 1, and 7 = QoS 0, 1, and 2 */
    uint8_t    txQoSBitMap;				 /* QoS bit is set if any QoS 0, 1, or 2 messages published by ANY client assigned to this submitter thread (e.g. 1 = QoS 0 only, 3 = QoS 0 and 1, and 7 = QoS 0, 1, and 2 */
	uint8_t    id;                       /* id of submitter thread. */
    uint8_t    rsvrd;

	uint64_t   submitCnt;                /* per producer thread submit msg count */
	uint64_t   connRetries;		         /* The sum of number of connection retries of all the clients serviced by this mbinst thread */

	int        currConnectMsgCount;      /* Total Number of MQTT CONNECT Messages sent for clients assigned to this submitter thread. */
	int        currConnAckMsgCount;      /* Total Number of MQTT CONNACK Messages received for clients assigned to this submitter thread. */
	int		   currDisconnectMsgCount;	 /* Total Number of MQTT DISCONNECT Messages received for clients assigned to this submitter thread. */
	int        numTCPConnects;           /* Total Number of TCP Connections for this thread. */
	int        numMQTTConnects;          /* Total Number of MQTT Connections for this thread. */
	int        numSubscribes;            /* Total Number of Subscribes for this thread. */
	int        numClients;               /* Number of clients for this thread. */
	int        maxSubsPerClient;      	 /* Maximum # of subscriptions per client */
	int        numSubsPerThread;         /* Number of Subscriptions per thread */
	int 	   numPubTopicsPerThread;    /* Number of publish topics per thread */
	int        sendrate;                 /* calculated send rate (msgs/sec) for this producer thread */
	int        msgsPerClientSess;        /* Number of messages per session. */
	int        numBfrs;                  /* Number of buffers to request when additional buffers are needed. */

	ism_threadh_t thrdHandle;            /* Thread Handle for this thread.. */
	ism_common_list *clientTuples;       /* List of clients for this thread */
	mbInfo *mqttbenchInfo;
} mqttbench_t;

/* ------------------------------------------------------------------------------------
 * Structure that contains client list information, pointers to various structures,
 * and snap information.
 * ------------------------------------------------------------------------------------ */
typedef struct {
	uint32_t instanceID;				 /* ID of this instance of mqttbench (-i <ID> command line parameter), default is 0 */

	uint8_t  burstModeEnabled;           /* Flag to indicate that burst mode is enabled. */
	uint8_t  instanceClientType;         /* The types of clients for this instance of mqttbench. */
	uint8_t  useSecureConnections;	     /* At least 1 client is using TLS */
	uint8_t  useWebSockets;              /* At least 1 client is configured to use web sockets. */

	int      clientListNumEntries;       /* Number of entries from the Client list used. */
	int      histogramCtr;
	int      timerThreadsInit;           /* Flag to indicate whether the timer threads have been created. */

	mqttbench_t         **mbInstArray;   /* Array of the mqttbench_t (mbInst). */
	environmentSet_t     *mbSysEnvSet;
	mbMsgConfigInfo_t    *mbMsgCfgInfo;
	mbBurstInfo_t        *mbBurstInfo;
	mbCmdLineArgs_t      *mbCmdLineArgs;
	mbJitterInfo_t       *mbJitterInfo;
	mbSSLBufferInfo_t    *mbSSLBfrEnv;
	mbThreadInfo_t       *mbThreadInfo;
	mbTimerInfo_t        *mbTimerInfo;
} mqttbenchInfo_t;

/* ------------------------------------------------------------------------------------
 * Structure for Latency Units (-u option, -cu option and -rtt option).
 * ------------------------------------------------------------------------------------ */
typedef struct latencyUnits {
	double    ConnUnits;
	double    RTTUnits;
	double    ReconnRTTUnits;
} latencyUnits_t;

/* ------------------------------------------------------------------------------------
 * Structure for Latency Statistics
 * ------------------------------------------------------------------------------------ */
typedef struct latencystat_t {
	volatile double       latResetTime;  /* The time at which the latency data was last reset. */
	double     histUnits;                /* Units of the histogram in seconds (e.g. 1e-6 is microseconds) */
	int        histSize;                 /* The size of the histogram */
	uint32_t   totalSampleCt;            /* Total Number of Samples Received - Includes fitting in & out of histogram */
	uint32_t  *histogram;                /* contains the histogram data */
	uint32_t   big;                      /* Number of measurements larger than the size of the histogram */
	uint32_t   max;                      /* The max latency */
	uint32_t   count1Sec;                /* Count of latency > 1 sec and larger than histogram.  */
	uint32_t   count5Sec;                /* Count of latency > 5 sec and larger than histogram. */
} latencystat_t;

/* ------------------------------------------------------------------------------------
 * Structure to hold the current IOP message counts for PUBLISH, PUBACK, PUBREC, PUBREL
 * and PUBCOMP.  This is used for the jitter feature.
 * ------------------------------------------------------------------------------------- */
typedef struct
{
	uint64_t prod_Tx_Curr_PUB;
	uint64_t prod_Rx_Curr_PUBACK;
	uint64_t prod_Rx_Curr_PUBREC;
	uint64_t prod_Tx_Curr_PUBREL;
	uint64_t prod_Rx_Curr_PUBCOMP;

	uint64_t cons_Rx_Curr_PUB;
	uint64_t cons_Tx_Curr_PUBACK;
	uint64_t cons_Tx_Curr_PUBREC;
	uint64_t cons_Rx_Curr_PUBREL;
	uint64_t cons_Tx_Curr_PUBCOMP;
} currMessageCounts_t;

/* ------------------------------------------------------------------------------------
 * Used to hold timer data that will be used in generating report statistics.
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	double   total;   /* Total running time since the base time (base) in seconds */
	double   delta;	  /* The delta time in seconds since the last call to getTimer */
	double   base;	  /* The start or initial base time in seconds that is set in setTimer */
	double   last;	  /* The last time in seconds that getTimer was called */
} Timer;

/* Callback for application data processing */
typedef void (*do_data_t)(mqttclient *client, ism_byte_buffer_t *bb) ;

/* ------------------------------------------------------------------------------------
 * PSK Array (pskArray_t) structure
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	uint8_t  pskID_ArrayEntries_Allocated;
	uint8_t  pskKey_ArrayEntries_Allocated;
	uint8_t  rsrvd[2];

	int      id_Count;
	int      id_Longest;
	int      key_Count;
	int      key_Longest;

	char   **pskIDArray;
	char   **pskKeyArray;
} pskArray_t;


/* ***************************************************************************************
 * Macros
 * ***************************************************************************************/
xUNUSED static char * VERSION_STRING = "version_string: mqttbench " XSTR(ISM_VERSION) " " XSTR(BUILD_LABEL) " " XSTR(ISMDATE) " " XSTR(ISMTIME);

#define SHOWVERSION   {fprintf(stdout, "\n%s (v %s)\n", VERSION_STRING, MB_VERSION); fflush(stdout);}

/* ***************************************************************************************
 * Function Prototypes
 * ***************************************************************************************/
void   checkClients(mqttbenchInfo_t *);
int    checkFileExists (char *);
int    checkForEnvConflicts (mqttbenchInfo_t *);
int    checkLatencyHistograms (int latencyBitSet, int numIOPThrds, uint8_t mqttClientType);
int    checkMqttPropFields(const char * bufp, int len, mqtt_prop_ctx_t * ctx, int flags, ism_check_ext_f check, void * userdata);
void   checkSystemSettings(void);
int    countConnections (mqttbench_t *, int, int, int, int *);
int    createFile (FILE **, char *);
int    createFirstMsgCtrs (void);
void   createLogTimeStamp (char *, int, int);
int    determineNumOfChars (int);
int    determineNumOfTopics (char *);
int    determineNumOfTopicStrings (char *);
int    determineNumSubmitThreadsStarted (void);
int    determinePortRangeOverlap (int);
void   displayAndLogEnvVarInfo (int);
void   displayAndLogError (char *);
void   displayJitter (mbJitterInfo_t *, int *, uint8_t, uint8_t);
int    doChangeMessageRate (int, mqttbenchInfo_t *, uint8_t);
void   doCleanUp (void);
void   doCleanUpArrayList (int);
void   doCleanUpHashMap (ismHashMap *);
int    doHandleHashMapEntry (ismHashMap *, char *, int);
void   doQuiesceSystem (void);
void   doRemoveHashMaps (void);
void   doStartTermination (int);
void   getAmountOfFreeMemory (int);
void   getCurrCounts (int, currMessageCounts_t *);
double getCurrTime (void);  /* get time in seconds */
int    getAllThreadsQoS (int, mqttbenchInfo_t *);
int    getSystemMemory(uint64_t *, char *, long int);
int    getStats (mqttbenchInfo_t *, uint8_t, double, FILE *);
void   initTimerThreads (void);
void   initMqttbenchLocks(int);
int    parseArgs (int, char **);
void   perfSleep (int, int);
int    processServerDestination(char *dest, int destPort, destIPList **dstIPList, int line);
int    provideAllocErrorMsg (char *, int, char *, int);
void   provideCharInfo (int, FILE *, char *, char *);
void   provideDoubleInfo (int, FILE *, char *, double);
void   provideEnvInfo (int, int, mqttbenchInfo_t *);
void   provideIntInfo (int, FILE *, char *, int);
int    provisionJitterArrays (int, int, mbJitterInfo_t *, mqttbench_t **);
double read_cpu_proc_freq (void);
int    read_psk_lists_from_file (int, char *, pskArray_t *);
int    readEnv (void);
int    readFile(const char *path, char **buff, int *bytesRead);
int    readLTPAToken (environmentSet_t *, char *);
void   removeFile (char *, int);
int    renameFile (char *, FILE **, int);
void   resetJitterInfo (int, mbJitterInfo_t *);
void   resetLogFiles (void);
void   setCPU (void);
void   setTimer (Timer *);
int    setMBTraceLevel (int);
void   showJitterInfo (int, mbJitterInfo_t *);
void   showVersion (char *);
void   syntaxhelp (int, char *);
double sysTime (void);
int    validateRate (char *);
uint8_t valStrIsNum (char *);


/* Client, Connection and Message processing related */;
int    connectClients (mqttbench_t *, ism_byteBufferPool);
int    doCloseConnections (ism_common_list *, int);
int    doCloseAllConnections (mqttbench_t *, int);
int    doCloseThreadConnections (ism_common_list *, int);

void * doCommands (void * thread_parm, void * context, int value);
void   doData (mqttclient *, ism_byte_buffer_t *);
int    doDetermineNumClientTypeThreads (int, int, mqttbench_t **, mbThreadInfo_t *);
void   doDisconnectAllMQTTClients (mqttbench_t *, uint8_t);
void   doDumpClientInfo (mqttbenchInfo_t *);
void   doDumpClients (mqttbenchInfo_t *, FILE *);
void * doHandleClients (void * thread_parm, void * context, int value);
int    doStartClients (mqttbenchInfo_t *, mqttbench_t *);
void   stopSubmitterThreads (mqttbenchInfo_t *);
void   doUnSubscribeAllTopics (mqttbench_t *, int *);
void   doPing (mqttclient *, ism_byte_buffer_t *);
HOT int onMessage (mqttclient *, ism_byte_buffer_t *, char *, int, Header *, mqttmessage_t *);

mbThreadInfo_t * initMBThreadInfo (int, mqttbenchInfo_t *);

/* Latency Related. */
void   calculateLatency (mqttbenchInfo_t *, int, int, int, FILE *, FILE *);
int    getConnectionTimes (mqttbenchInfo_t *, int, double *, uint8_t);
int    getRRConnectionTimes (mqttbenchInfo_t *, int, int);
int    prep2PrintHistogram (FILE **, char *, int);
void   printHistogram (mqttbenchInfo_t *, int, int, FILE *);
void   printRemaining (char *);
void   removeIOPThreadHistograms (environmentSet_t *, ioProcThread **);
int    setupClientThreadHistogram (mqttbench_t *);
int    setupIOPThreadHistograms (ioProcThread *, environmentSet_t *, latencyUnits_t *, int, int);
int    provideConnStats (mqttbenchInfo_t *, uint8_t, double, double, FILE *, FILE **);
void   provideLatencyWarning (environmentSet_t *, int);

/* mqttbench_t specific */
int    setMqttbenchLogFile (char *);
int    initMQTTBench (void);
mqttbench_t * createMQTTBench (void);
void   cloneMQTTBench (mqttbench_t *, mqttbench_t *);

/* Config and Trace functions */
void setMBConfig (char *, char *);

/* Routines needed for -M option */
int    chkMsgFile (char *, char *);
int    determineNumDatFiles (char *);

/* Routines needed to validate IP Addresses */
int    validateSrcIPs (char **, int, char **, int);
char ** getListOfInterfaceIPs (int *);

/* Prototypes - Adding Timers */
void   addAutoRenameTimerTask (mqttbenchInfo_t *);
int    addBurstTimerTask (mbTimerInfo_t *, mqttbenchInfo_t *);
void   addDelayTimerTask (mqttbenchInfo_t *, transport *, uint64_t);
int    addJitterTimerTask (mqttbenchInfo_t *);
void   addLingerTimerTask (mqttclient *);
int    addWarningTimerTask (mqttbenchInfo_t *);
int    addMqttbenchTimerTasks (mqttbenchInfo_t *);

/* Prototypes - Actual Timer Routines */
int onTimerAutoRenameLogFiles (ism_timer_t, ism_time_t, void *);
int onTimerBurstMode (ism_timer_t, ism_time_t, void *);
int onTimerLatencyWarning (ism_timer_t, ism_time_t, void *);

#endif /* __MQTTBENCH_H */
