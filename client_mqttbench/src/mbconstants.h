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

#ifndef MBCONSTANTS_H_
#define MBCONSTANTS_H_

/************************************* DEV Constants** ***************************************/
//#define _TEST_PING                          1     /* Test PINGREQ and PINGRESP calls */
//#define _CHECK_BFR_PTRS                     1     /* Check buffer pointers in creating mqtt msg */
//#define _NEED_TIME                          1     /* Used for determining disconnect time */

/*************************************DEBUG Constants ****************************************/
#define _SHORT_TERM_FIX                     1
#define _SINGLE_SUBSCRIPTION                1

/************************************* String Constants ***************************************/
#ifndef XSTR
  #define XSTR(s) STR(s)
  #define STR(s) #s
#endif

/************************************** MBTRACE Macro *****************************************/
#define MBINFO                              "INFO:  "
#define MBWARN                              "WARN:  "
#define MBERROR                             "ERROR: "
#define MBDEBUG                             "DEBUG: "

#define MBTRACE(mbType, trclvl, fmt...)     TRACE(trclvl, mbType fmt)


/***************************************** Constants ******************************************/
#define ClientVersion 1000000

/* MQTT Specific defines */
#define MB_VERSION                          "5.2.3"

/***************************************** Constants ******************************************/

/* ------------------------------------------------------------------------------------
 * The following constant is used for the sprintf needed to create the clientID and
 * topic name.   The following provides information on how many topics are supported
 * based on the value:
 *
 *   Value    # Topics     Description
 *  -------    --------    ------------------------------------------------------------
 *  "%07d"       10 M      Provides the range of: 0 to 9999999
 *  "%08d"      100 M      Provides the range of: 0 to 99999999
 * ------------------------------------------------------------------------------------ */
#define TOPIC_SPRINTF_LEN                   "%08d"      /* Constant used for sprintf for topic names to achieve 100M topics */

#define MAX_INT8                            0x7F
#define MIN_INT8                            0xFF
#define MAX_INT16                           0x7FFF
#define MIN_INT16                           0xFFFF
#define MAX_INT32                           0x7FFFFFFF
#define MIN_INT32                           0xFFFFFFFF

#define MAX_UINT8                           0xFF
#define MAX_UINT16                          0xFFFF
#define MAX_UINT32                          0xFFFFFFFF

#define MAX_GLOBAL_MSGID					1e7         /* do not allocate more than this number of msgid objects */

#define UNKNOWN_PROTOCOL_VER                0x00
#define MQTT_PROTOCOL_VER_31                0x03
#define MQTT_PROTOCOL_VER_311               0x04
#define MQTT_PROTOCOL_VER_5                 0x05
#define MQTT_PROTOCOL_VERSION_DEFAULT       MQTT_PROTOCOL_VER_5

#define STR_MQTT_VERSION_31                 "3.1"
#define STR_MQTT_VERSION_311                "3.1.1"
#define STR_MQTT_VERSION_5                  "5"
#define STR_MQTT_VERSION_50                 "5.0"
#define STR_MQTT_VERSION_DEFAULT            STR_MQTT_VERSION_5

#define MQTT_MAX_MSGID          			65535
#define MQTT_MIN_TOPICS_PER_SUBSCRIBE       10
#define MQTT_MAX_TOPICS_PER_SUBSCRIBE 		32          /* maximum number of subscriptions that can be sent in 1 SUBSCRIBE packet */
#define MQTT_MAX_SUBSCRIPTIONS_PER_CLIENT   500         /* Appliance allows a max of 500 subscriptions per client. */
#define MQTT_SUBSCRIBE_LARGE_NUM_TOPICS     100000000   /* maximum number of topics */
#define MQTT_MAX_TOPIC_NAME 				256        	/* the maximum length of an MQTT topic name allowed in mqttbench */
#define MQTT_MAX_TOPIC_NAME_PAD128   		384
#define MQTT_CONN_DEFAULT_TIMEOUT			120			/* how long the client will wait for a CONNACK after sending the CONNECT message, or wait in HANDSHAKE_IN_PROCESS state */
#define MQTT_PING_DEFAULT_TIMEOUT			60			/* how long the client will wait for a PINGRESP after sending the PINGREQ message */
#define MQTT_CONN_RETRIES					3			/* how many time to retransmit the CONNECT message */
#define MAX_CONN_RETRIES					10000   	/* maximum number of times to call createConnection per client without receiving EINPROGRESS from connect call */
#define MAX_CONN_BASIC_RETRIES				3			/* maximum number of reconnect attempts for a basic TCP client connection */
#define MQTT_KEEPALIVE_TIMER    			0x0      	/* mqttbench does not currently implement PINGREQ/PINGRESP so
 	 	 	 	 	 	 	 	 	 	 	    	   	 * to avoid a disconnect in absence of data the keepalive is set to zero so that it will not
 	 	 	 	 	 	 	 	 	 	 	    	   	 * expire/disconnected by the server. */
#define MQTT_STRING_LEN						2			/* All character string fields in MQTT are preceded with a 2 byte length */
#define MQTT_BINARYDATA_LEN					2			/* All binary data fields in MQTT are preceded with a 2 byte length */
#define MQTT_FIXED_HEADER_LEN				1			/* Length of the Fixed header field */
#define MQTT_MAX_NO_OF_REMAINING_LEN_BYTES  4			/* Maximum length of the remaining length bytes field */
#define MQTT_MAX_VARINT_LEN				    4           /* Maximum length of a variable integer field */
#define MQTT_TOPIC_NAME_HDR_LEN   			2			/* field containing the topic name length */
#define SUBOPTIONS_LEN						1			/* Length of subcsription options field */
#define MQTT_MSGID_SIZE         			2			/* Length of the message ID field */
#define MQTT_REQ_QOS_SIZE       			1			/* Length of the per Topic requested QoS field */
#define MQTT_SUBOPT_SIZE                    1
#define MQTT_PROT_NAME_LEN_SIZE				2
#define MQTT_PROT_NAME_MQTT31               "MQIsdp"
#define MQTT_PROT_NAME_MQTT31_LEN	        6
#define MQTT_PROT_NAME_MQTT311              "MQTT"
#define MQTT_PROT_NAME_MQTT311_LEN	        4
#define MQTT_PROT_NAME_MQTT5                "MQTT"
#define MQTT_PROT_NAME_MQTT5_LEN	        4
#define MQTT_PROT_VERS_NUM_LEN				1
#define MQTT_CONN_FLAGS_LEN					1
#define MQTT_KEEP_ALIVE_LEN					2
#define MQTT_RC_LEN							1


/* Header Length Macros */
#define MQTT_CONNECT_V31_VAR_HDR_LEN	    (MQTT_PROT_NAME_LEN_SIZE + \
											 MQTT_PROT_NAME_MQTT31_LEN + \
											 MQTT_PROT_VERS_NUM_LEN + \
											 MQTT_CONN_FLAGS_LEN + \
											 MQTT_KEEP_ALIVE_LEN)
#define MQTT_CONNECT_V311_VAR_HDR_LEN	    (MQTT_PROT_NAME_LEN_SIZE + \
											 MQTT_PROT_NAME_MQTT311_LEN + \
											 MQTT_PROT_VERS_NUM_LEN + \
											 MQTT_CONN_FLAGS_LEN + \
											 MQTT_KEEP_ALIVE_LEN)
#define MQTT_CONNECT_V5_VAR_HDR_LEN 	    (MQTT_PROT_NAME_LEN_SIZE + \
											 MQTT_PROT_NAME_MQTT5_LEN + \
											 MQTT_PROT_VERS_NUM_LEN + \
											 MQTT_CONN_FLAGS_LEN + \
											 MQTT_KEEP_ALIVE_LEN)
#define MQTT_PUB_MSG_LEN(topicLen, propsLen, payloadLen)  (MQTT_FIXED_HEADER_LEN + \
				       	   	   	   			               MQTT_MAX_NO_OF_REMAINING_LEN_BYTES + \
				       	   	   	   			               MQTT_TOPIC_NAME_HDR_LEN + \
														   topicLen + \
				       	   	   	   			               MQTT_MSGID_SIZE + \
														   (propsLen > 0 ? MQTT_MAX_NO_OF_REMAINING_LEN_BYTES : 0) + \
														   propsLen + \
				       	   	   	   			               payloadLen) 		 /* Calculate the size of an MQTT PUBLISH message buffer */
#define MQTT_SUB_MSG_LEN(topicsLen, propsLen)   		(MQTT_FIXED_HEADER_LEN + \
					    					            MQTT_MAX_NO_OF_REMAINING_LEN_BYTES + \
														MQTT_MSGID_SIZE + \
														(propsLen > 0 ? MQTT_MAX_NO_OF_REMAINING_LEN_BYTES : 0) + \
														propsLen + \
														topicsLen) /* Calculate the size of an MQTT SUBSCRIBE message buffer */
#define MQTT_WILL_MSG_LEN(topicLen, payloadLen, propsLen) 	((propsLen > 0 ? MQTT_MAX_NO_OF_REMAINING_LEN_BYTES : 0) + \
															propsLen + \
															MQTT_STRING_LEN + \
															topicLen + \
															MQTT_BINARYDATA_LEN + \
															payloadLen) /* Calculate the size of an LWT message buffer */

#define MQTT_CONNECT_MSG_LEN(varHdrLen, willMsgLen, propsLen, clientIdLen, userNameLen, passwordLen) \
																				(MQTT_FIXED_HEADER_LEN + \
																				MQTT_MAX_NO_OF_REMAINING_LEN_BYTES + \
																				varHdrLen + \
																				(propsLen > 0 ? MQTT_MAX_NO_OF_REMAINING_LEN_BYTES : 0) + \
																				propsLen + \
																				MQTT_STRING_LEN + \
																				clientIdLen + \
																				willMsgLen + \
																				MQTT_STRING_LEN + \
																				userNameLen + \
																				MQTT_STRING_LEN + \
																				passwordLen) /* Calculate the size of an MQTT CONNECT message buffer */

#define MQTT_DISCONNECT_MSG_LEN(version) 			(MQTT_FIXED_HEADER_LEN + \
											 	 	MQTT_MAX_NO_OF_REMAINING_LEN_BYTES + \
													(version >= MQTT_V5 ? MQTT_RC_LEN : 0) + \
											 	 	(version >= MQTT_V5 ? REASON_STR_LEN + MQTT_MAX_VARINT_LEN : 0) )  /* Calculate the size of an MQTT DISCONNECT message buffer */


/************************** Pluggable Protocol Macro and Constants ****************************/
#define PLUGPROTOCOL_MAX_LEN                6
#define CONNECT_VAR_HDR_LEN(plugproto)	    (MQTT_PROT_NAME_LEN_SIZE + \
											 strlen(plugproto) + \
											 MQTT_PROT_VERS_NUM_LEN + \
											 MQTT_CONN_FLAGS_LEN + \
											 MQTT_KEEP_ALIVE_LEN)
#define BAD_MQTT_MSG                     	-1
#define SUBACK_FAILURE                      0x80
#define PARTIAL_MQTT_MSG                  	0

/* MQTT Return codes */
#define RC_SUCCESSFUL                        0
#define RC_MQTTCLIENT_FAILURE                2 /* Generic error code. */
#define RC_TCP_SOCK_ERROR                    3 /* Client had a SOCK_ERROR */
#define RC_TCP_SOCK_DISCONNECTED             4 /* TCP disconnect */
#define RC_MQTTCLIENT_DISCONNECTED           5 /* MQTT disconnect */
#define RC_MQTTCLIENT_DISCONNECT_IP          6
#define RC_UNABLE_TO_SUBMITJOB               7
#define RC_MQTTCLIENT_UNKNOWN_MSG_TYPE       8 /* Unable to identify the type of message. */
#define RC_BAD_UTF8_STRING       	         9 /* Invalid UTF-8 string detected */
#define RC_FILE_NOT_FOUND                   10
#define RC_FILEIO_ERR						11
#define RC_FILENAME_ERR						12
#define RC_ERR_OPEN_CPUINFO_FILE            13
#define RC_ERR_OPEN_LATENCY_FILE            14
#define RC_REQUESTED_QUIT					15
#define RC_MKFIFO_ERR						16
#define RC_READ_KERN_ERR					17
#define RC_MSGID_NONE_AVAILABLE				18
#define RC_MSGID_MAXSEARCH					19
#define RC_MSGID_BAD						20
#define RC_DIRECTORY_NOT_FOUND              21
#define RC_UNABLE_DETERMINE_DIRECTORY       22
#define RC_UNABLE_TO_HANDLE_DIRECTORY       23
#define RC_UNABLE_DETERMINE_QOS             24
#define RC_BAD_MSGDIR						25

#define RC_UNEXPECTED_MSG					28
#define RC_BAD_MSGLEN						29
#define RC_MEMORY_ALLOC_ERR					30
#define RC_INVALID_ENV_SETTING              31
#define RC_INSUFFICIENT_CLIENTIDS           32
#define RC_INSUFFICIENT_INFO                33
#define RC_EXCEEDED_MAX_VALUE               34
#define RC_STRING_TOO_LONG                  35
#define RC_INVALID_REQUEST                  36
#define RC_UNABLE_TO_INIT_BUFFERS           37
#define RC_INVALID_TOPIC					38

#define RC_BAD_CLIENTLIST					40
#define RC_NO_CLIENTLIST_SPECIFIED          41
#define RC_INVALID_VALUE                    42
#define RC_INVALID_OPTION                   43
#define RC_INVALID_PARAMETER                44
#define RC_NO_ARGUMENT                      45
#define RC_PARAMETER_ALREADY_SPECIFIED      46
#define RC_PARAMETER_CONFLICT               47
#define RC_PARAMETER_MISMATCH               48

#define RC_FAILED_TO_CREATE_MBINST          50
#define RC_HASHMAP_IS_NULL                  51
#define RC_UNABLE_TO_ADD_TO_HASHMAP         52
#define RC_UNABLE_TO_FIND_HASHMAP_ENTRY     53
#define RC_IOLISTENER_FAILED                54
#define RC_IOPROCESSOR_FAILED               55
#define RC_UNABLE_TO_SUBMIT_JOB             56
#define RC_IOP_NOT_FOUND                    57

#define RC_PSK_TOO_LARGE                    60
#define RC_INVALID_PSK                      61
#define RC_INSUFFICIENT_NUM_PSK_IDS         62
#define RC_CERTIFICATE_NOT_FOUND            63
#define RC_PRIVATE_KEY_NOT_FOUND            64
#define RC_ERR_ALLOCATE_PLUGPROTOCOL        65

#define RC_INSUFFICIENT_IP_ADDRESSES        70
#define RC_NO_IP_ADDRESSES                  71
#define RC_INVALID_IPV4_ADDRESS             72
#define RC_BIND_ERROR                       73
#define RC_SET_NONBLOCKING_ERROR            74
#define RC_SO_REUSEADDR_ERROR               75
#define RC_PORTRANGE_OVERLAP                76
#define RC_UNABLE_TO_RESOLVE_DNS            77
#define RC_EXHAUSTED_IP_ADDRESSES           78
#define RC_EPOLL_ERROR                      79
#define RC_INTERFACE_ERROR                  80
#define RC_DEST_UNREACHABLE					81
#define RC_PUB_BUT_NO_MSGS					82
#define RC_TCPIP_ERROR						83

#define RC_INTERNAL_ERROR                   90
#define RC_EXIT_ON_ERROR                    99
#define RC_ERROR_PROCESS_FAILURE            99

#define RC_ERROR_RATE_CONTROL_TIMER        100
#define RC_ERROR_SET_TIMER                 101
#define RC_ERROR_READ_TIMER                102
#define RC_TIMER_KEY_NULL                  103

#define RC_SSL_TRUSTSTORE_ERROR            109
#define RC_SSL_CONTEXT_NULL                110
#define RC_UNDETERMINISTIC_DATA            111

#define RC_RANGE_OVERLAP                   120
#define RC_UNSUPPORTED_FUNCTION            121
#define RC_UNABLE_TO_LOAD_PAYLOAD          122
#define RC_NO_TOPICS                       123
#define RC_HISTOGRAM_NOT_FOUND             124

#define RC_RTT_LATENCY_NO_CONSUMERS        130
#define RC_SEND_LATENCY_NO_PRODUCERS       131
#define RC_SUBSCRIBE_LATENCY_NO_SUBS       132

/* TCP Related defines */
#define QUIESCE_TIMEOUT         	10       /* 10 Seconds */
#define DEFAULT_PORT            	16102
#define MAX_PORT_NUMBER             65535

/* Default values for Timers. */
#define DEFAULT_RETRY_DELAY_USECS   100

/* Default values for thread safe random_r */
#define CACHE_LINE_SIZE    64
#define PRNG_BUFSZ          8

#define PUB 0
#define SUB 1
#define UD 0 // user-defined mqtt properties
#define SD 1 // spec-defined mqtt properties

/* mqttbench generic constants */
#define MIN_MSG_SIZE                32		 	/* this is the smallest generated message allowed */
#define INITIAL_RX_PKT_SIZE			64			/* a default initial RX packet size per consumer client */
#define MSG_SIZE_10MB               10000000
#define MAX_THREAD_NAME         	16
#define WAIT_TIMEOUT            	60       	/* Number of secs to wait for ACK from Server. */
#define CONNECT_TIMEOUT         	30000L
#define MAX_AFFINITY_NAME           64
#define MAX_AFFINITY_MAP            64
#define MAX_TIMER_THREADS           2

#define MEM_IN_KB                   1024
#define MEM_IN_MB                   (1024 * 1024)
#define MEM_IN_GB                   (1024 * 1024 * 1024)

#define REASON_STR_LEN              128
#define MIN_ERROR_STRING            512
#define MAX_ERROR_STRING            1024
#define INC_BFR_BYTES_SIZE          1024
#define KERNEL_LINE_SIZE            1024
#define PSK_LINE_SIZE               1024
#define CLIENTLIST_LINE_SIZE        16384
#define MAX_NUM_HISTOGRAMS          4       /* Maximum # of histogram files. */

#define BITS_PER_BYTE               8
#define MINLATENCY              	0x7FFFFFFF
#define MAX_CONN_HISTOGRAM_SIZE     100000
#define MAX_MSG_HISTOGRAM_SIZE      100000
#define MAXGLOBALRATE           	10000000
#define MICROS_PER_SECOND       	1000000.0
#define MICROS_PER_MILLI        	1000.0
#define NANO_PER_MICRO          	1000.0
#define NANO_PER_MILLI          	1000000.0
#define NANO_PER_SECOND         	1000000000.0 /* number of nanoseconds per second */
#define MILLI_PER_SECOND        	1000.0
#define MILLISECOND_EXP           	1.0e-3 /* millisecond represented in exponent */
#define INV_MICROSECOND_EXP         1.0e6
#define SECONDS_PER_MINUTE      	60.0
#define SECONDS_IN_24HRS            (60 * 60 * 24)
#define SECONDS_IN_30MINS           (60 * 30)
#define UNSUBSCRIBE_RETRY        	3
#define DISCONNECT_SLEEP_RETRY  	2
#define DISCONNECT_RETRY        	3
#define SLEEP_DISCONNECT        	5
#define DISCONNECT_ATTEMPTS         5
#define CLIENT_SCHYLD_DIV       	16384
#define NUM_CLIENTS_PER_CHUNK       50000
#define MAX_BIT_ARRAY               256
#define MAX_CLIENT_PREFIX_LEN       256
#define MAX_HOSTNAME_LEN            64
#define MAX_DNS_RECORDS				128
#define MAX_HISTOGRAM_NAME_LEN      256
#define MAX_CMDLINE_HISTORY_LEN     64
#define MAX_DOCOMMANDS_CMDLINE_LEN  1024
#define MAX_CHAR_STRING_LEN         512
#define MAX_CMDLINE_LEN             512
#define MAX_DIRECTORY_LEN           256
#define MAX_DISPLAYLINE_LEN         256
#define MAX_PROMPT_LEN              128
#define MAX_TIME_STRING_LEN         80
#define MAX_HGRAM_TITLE             128
#define MAX_HDRLINE                 1024
#define MAX_SNAP_STATLINE           4096
#define MAX_SNAP_CURRLINE           512
#define MAX_SNAP_RTTLINE            512
#define MAX_SNAP_RECONN_RTTLINE     512
#define MAX_METRIC_VALUE_LEN		64
#define MAX_METRIC_NAME_LEN			512
#define MAX_METRIC_TUPLE_LEN		1024
#define MAX_LOGDATA_LINE            2048
#define MAX_INFO_LINE               256
#define MAX_COLUMN_NAME             64
#define MAX_VERSION_STRING_LEN      8
#define MIN_SECURITY_LEVEL          0
#define MAX_SECURITY_LEVEL          2
#define MAX_DIRECTORY_NAME          1024
#define MAX_FILENAME                1024
#define MAX_INFO_FILENAME           512
#define MAX_JSON_ENTS				1024ULL
#define MAX_PROPIDS                 64
#define MIN_RECONNECT_DELAY_NANOS   50000

#define DEFAULT_TRACELEVEL          "5"
#define CHAR2NUM(a)                 (uint8_t)(atoi(a))
#define DEFAULT_LINGER_SECS         0

/* The following are sleep intervals in microseconds for ism_common_sleep() */
#define SLEEP_1_USEC                1
#define SLEEP_10_USEC               10
#define SLEEP_100_USEC              100
#define SLEEP_1_MSEC                1000
#define SLEEP_10_MSEC               10000
#define SLEEP_100_MSEC              100000
#define SLEEP_250_MSEC              250000
#define SLEEP_500_MSEC              500000
#define SLEEP_1_SEC                 1000000

#define WAIT_NUM_SECS               1
#define THREADBATCHINGRATE			100000
#define EPOLLERR_MAX_PRINT			5       /* print only the first 5 EPOLL errors */

#define IO_PROC_RX_POOL_SIZE_MIN	64
#define IO_PROC_RX_POOL_SIZE_MAX	128
//#define IO_PROC_TX_POOL_SIZE_MIN	1024
//#define IO_PROC_TX_POOL_SIZE_MAX	4096
#define IO_PROC_TX_POOL_SIZE_MIN	4096
#define IO_PROC_TX_POOL_SIZE_MAX	32767

#define LG_IO_PROC_RX_POOL_SIZE_MIN	10
#define LG_IO_PROC_RX_POOL_SIZE_MAX	100
#define LG_IO_PROC_TX_POOL_SIZE_MIN	10
#define LG_IO_PROC_TX_POOL_SIZE_MAX	100

#define MAX_SUBMITTER_THREADS       4
#define MAX_IO_PROC_THREADS         32      /* Max number of I/O Proc Threads supported */
#define MAX_IO_LISTENER_THREADS     2       /* Max number of I/O Proc Threads supported */
#define DEF_NUM_TX_BFRS             320     /* Default for # of Bfrs per request from IOP */
//#define DEF_NUM_IOP_TX_BFRS         160     /* Default for # of Bfrs per request from IOP for IOP */
#define DEF_TX_BFR_SIZE_CONSUMERS   1024    /* Default size of TX Buffers for consumer. */
#define NUM_RETRIES_GETBUFFER       3       /* # of retries for getting IOP TX Buffers */
#define GETBUFFER_FAIL_CTR          100     /* # of failures reached when to print error message. */

#define INITIAL_NUM_EPOLL_EVENTS	65536
#define LOCAL_PORT_RANGE_LO			5000
#define LOCAL_PORT_RANGE_HI			65000
#define SKIP_PORT					2
#define MAX_LOCAL_IP_ADDRESSES		24

/* EAK - 20180823, today we found that the single buffer pool lock is REALLY detrimental to
 * multithreaded performance so disabling the TLS buffer pool by default for the forseeable future */
#define INIT_SSL_BUFFERPOOL         0

/* IOP Job deferal constants*/
# define NODEFERJOB					0
# define DEFERJOB					1

/* Payload length fields for mqttbench */
#define TSF_LEN                     1
#define TIMESTAMP_LEN               8
#define APP_SEQNUM_LEN              8
#define PAYLOAD_MSGLEN              4
#define MIN_MSGLEN                  (TSF_LEN + TIMESTAMP_LEN + APP_SEQNUM_LEN + PAYLOAD_MSGLEN)
#define PAYLOAD_MSGLEN_OFFSET       (TSF_LEN + TIMESTAMP_LEN + APP_SEQNUM_LEN)
/* These 2 are currently not used.  These would be for sequence numbering feature. */
#define PAYLOAD_TSF_TS_SEQNUM       (TSF_LEN + TIMESTAMP_LEN + APP_SEQNUM_LEN)
#define PAYLOAD_SEQNUM_OFFSET       (TSF_LEN + TIMESTAMP_LEN)

/* Length of some common messages */
#define ACK_MSG_LENGTH              2
#define DISCONNECT_MSG_LENGTH       2
#define PING_MSG_LENGTH             2

/* Logging information */
#define CONSTANT_5K                 5000

/* Latency measurements */
#define LAT_MASK_SIZE				8       /* size of the latency mask (number of bits) */
#define CHKTIMERTT					0x1		/* measure round trip latency per destination */
#define CHKTIMESEND   				0x2     /* latency of send call */
#define CHKTIMETCPCONN 		    	0x4     /* latency of creating a TCP connection */
#define CHKTIMEMQTTCONN  	    	0x8     /* latency of creating a MQTT connection */
#define CHKTIMESUBSCRIBE            0x10    /* latency of creating a subscription */
#define CHKTIMETCP2MQTTCONN         0x20    /* latency of creating a TCP connection thru MQTT connection */
#define CHKTIMETCP2SUBSCRIBE        0x40    /* latency of creating a TCP connection thru subscription */
#define PRINTHISTOGRAM				0x80	/* print the latency histogram for specified type. */

/* Latency measurements combinations of masks. */
#define LAT_TCP_COMBO               (CHKTIMETCPCONN | CHKTIMETCP2MQTTCONN | CHKTIMETCP2SUBSCRIBE)
#define LAT_MQTT_COMBO              (CHKTIMEMQTTCONN | CHKTIMETCP2MQTTCONN)
#define LAT_CONN_AND_SUBSCRIBE      (CHKTIMETCPCONN | CHKTIMEMQTTCONN | CHKTIMESUBSCRIBE | CHKTIMETCP2MQTTCONN | CHKTIMETCP2SUBSCRIBE)
#define LAT_IOP_HISTOGRAMS          (CHKTIMETCPCONN | CHKTIMEMQTTCONN | CHKTIMESUBSCRIBE | CHKTIMETCP2MQTTCONN | CHKTIMETCP2SUBSCRIBE | CHKTIMERTT | CHKTIMESEND)
#define LAT_TCP_ONLY                (CHKTIMETCPCONN | PRINTHISTOGRAM)

/* Histogram Titles */
#define HGRAM_TCP_CONN              "TCP Connections:"
#define HGRAM_MQTT_CONN             "MQTT Connections:"
#define HGRAM_SUBS                  "Subscriptions:"
#define HGRAM_TCP_MQTT_CONN         "TCP - MQTT Connections:"
#define HGRAM_TCP_SUBS              "TCP - Subscriptions:"
#define HGRAM_RTT                   "Round Trip Latency:"
#define HGRAM_RECONN_RTT            "Reconnect RTT Latency:"
#define HGRAM_SEND                  "Send Call Latency:"

/* Histogram Title Lengths */
#define LEN_HGRAM_TCP_CONN          strlen(HGRAM_TCP_CONN)
#define LEN_HGRAM_MQTT_CONN         strlen(HGRAM_MQTT_CONN)
#define LEN_HGRAM_SUBS              strlen(HGRAM_SUBS)
#define LEN_HGRAM_TCP_MQTT_CONN     strlen(HGRAM_TCP_MQTT_CONN)
#define LEN_HGRAM_TCP_SUBS          strlen(HGRAM_TCP_SUBS)
#define LEN_HGRAM_RTT               strlen(HGRAM_RTT)
#define LEN_HGRAM_RECONN_RTT        strlen(HGRAM_RECONN_RTT)
#define LEN_HGRAM_SEND              strlen(HGRAM_SEND)

/* Check Message Masks */
#define CHKMSGLENGTH				0x1		/* compare message length received with stored length in message. */
#define CHKMSGSEQNUM 				0x2     /* check sequencing for gaps. */

/* Client Constants */
#define MAX_MSG_ID					65535   /* MQTT - MQTTProtocol.h */
#define MAX_CLIENTID_LEN			64      /* MQTT - MQTTProtocol.h */
#define TMP_MAX_CLIENTID_LEN        64      /* V3.1.1 and 3.1 now support larger than 23 characters clientIds. */
#define PARTIAL_BUF_PADDING     	32      /* Additional Bytes to grow partial msg buffer. */
#define SUBSCRIBE_PADDING           8       /* Padding for when calculating subscribe message IDs needed. */
#define MAX_NUM_QOS                 3

#define MQTT_QOS_0                  0
#define MQTT_QOS_1					1
#define MQTT_QOS_2                  2
#define MQTT_CLEANSESSION_NO        0
#define MQTT_CLEANSESSION_YES       1
#define MQTT_RETAIN_NO              0
#define MQTT_RETAIN_YES             1
#define MQTT_WEBSOCKET_NO           0
#define MQTT_WEBSOCKET_YES          1

/* QoS Mask Bits */
#define MQTT_QOS_0_BITMASK          0x1
#define MQTT_QOS_1_BITMASK          0x2
#define MQTT_QOS_2_BITMASK          0x4
#define MQTT_VALID_QOS              (MQTT_QOS_0_BITMASK | MQTT_QOS_1_BITMASK | MQTT_QOS_2_BITMASK)

/* ------------------------------------------------------------------------------------
 * NOTE:  This needs to be changed to 8 when the LDAP database is rebuilt to support a
 *        total of 9 characters which is comprised up of 8 digits.
 * ------------------------------------------------------------------------------------ */
#define USERNAME_NUM_LENGTH         8       /* Length of the number appended to Username */
#define USERNAME_NUM_MAX            256     /* Maximum number value appended to Username */
#define MAX_PUBREC_SEARCH           256     /* Maximum number of searches for PUBREC msg ids */
#define MAX_MSGID_SEARCH            256     /* Maximum number of searches for msg ids */

/* Thread Constants */
#define THREAD_RETURN_TYPE void *
#define THREAD_RETURN_VALUE ((void *) -1)	/* -1 is the equivalent of PTHREAD_CANCELED in pthread.ha */

#define MQTT_UNKNOWN            	0x0
#define MQTT_CONNECT_IN_PROCESS    	0x1
#define MQTT_CONNECTED             	0x2
#define MQTT_PUBSUB                	0x4

#define MQTT_CONNECT_OR_PUBSUB      (MQTT_CONNECTED | MQTT_PUBSUB)

#define MQTT_DOUNSUBSCRIBE          0x10
#define MQTT_UNSUBSCRIBE_IN_PROCESS 0x20
#define MQTT_UNSUBSCRIBED           0x40
#define MQTT_DISCONNECT_IN_PROCESS 	0x80
#define MQTT_DISCONNECTED          	0x100

#define DISCONNECT_WITHOUT_UNSUBSCRIBE 0
#define DISCONNECT_WITH_UNSUBSCRIBE    1

#define MQTT_RETRY_DISCONNECT       0x800   /* Special state for when disconnect 2nd time with reconnect */

#define ALL_TOPICS_UNSUBACK         0xFFFE  /*      */
#define ALL_CONNECT_STATES          0xFFFF  /* Status to collect all connection states for all clients. */

#define DEF_LTPA_VERISON            1       /* LTPA Default Version */
#define LTPA_USERID                 "IMA_LTPA_AUTH"
#define OAUTH_USERID                "IMA_OAUTH_ACCESS_TOKEN"

/* Check for the end of a line */
#define isEOL(c) ((c)=='\0'||(c)=='\r'||(c)=='\n')

/* MQTT RC Check
 * return 0 = valid RC
 *        1 = invalid RC */
#define MQTT_RC_CHECK(rc) (rc < 0 || rc > 162)

/* ------------------------------------------------------------------------------------
 * Special Macro for Connect Message Length with UserName and Password. The 6 bytes at
 * the end of the macro is due to the 2 byte length fields for the clientID, Username
 * and Password.
 * ------------------------------------------------------------------------------------ */
#define MQTT_CONNECT_MAX_MSG_LEN(extraLen) (MQTT_FIXED_HEADER_LEN + MQTT_MAX_NO_OF_REMAINING_LEN_BYTES + \
		                                    MQTT_CONNECT_V31_VAR_HDR_LEN + MAX_CLIENTID_LEN + (extraLen) + 6);

#define WS_SERVER_GUID              "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define MAX_WS_FRAME_LEN            14
#define WS_FRAME_LEN_126            8
#define MIN_WS_FRAME_LEN            6
#define WS_FRAME_SIZE               2
#define WS_FRAME_2BYTE              2
#define WS_FRAME_4BYTE              4
#define WS_FRAME_10BYTE             10
#define WS_FRAME_ALL_INITIALIZE     0x8280    /* Equivalent to wsframe.all=0, wsframe.bits.final=1,
                                               * wsframe.bits.opCode=2 and wsframe.bits.mask=1 */

#define WS_PAYLOAD_MASK_LESS_64K    0x7E
#define WS_PAYLOAD_MASK_MORE_64K    0x7F
#define WS_PAYLOAD_64K              64*1024

#define DEFAULT_SNAP_INTERVAL       5
#define DEFAULT_GRAPHITE_METRIC_ROOT "loadtests"
#define DEFAULT_GRAPHITE_PORT		2003

/* Snap types for type of data to log */
#define SNAP_CONNECT                0x1
#define SNAP_RATE                   0x2
#define SNAP_COUNT                  0x4
#define SNAP_LATENCY                0x8
#define SNAP_CONNECT_LATENCY        0x10

/* Jitter defaults */
#define MB_JITTER_INTERVAL          5000
#define MB_JITTER_SAMPLECT          10000

/* Source of messages including Message Directories from Client List. */
#define PARM_S_OPTION               0x1
#define PREDEFINED                  0x2
#define CL_MESSAGE_DIRECTORIES      0x4
#define NUMBER_OF_SOURCES           3

#define PARM_MSG_SIZES              "-S_PARM_SPECIFIED"

/* Default Filenames */
#define MQTTBENCH_TRACE_FNAME       "mqttbench_trace.log"
#define CLIENT_INFO_FNAME           "mqttbench_clientdump.log"
#define CSV_STATS_FNAME             "mqttbench_stats.csv"
#define CSV_LATENCY_FNAME           "mqttbench_latstats.csv"
#define LTPA_VERS_1_FNAME           "ltpav1token.dat"
#define LTPA_VERS_2_FNAME           "ltpav2token.dat"

/* System Filenames or System Variables Used */
#define PROC_CPUINFO                "/proc/cpuinfo"
#define CPU_MHZ                     "CPU MHZ"
#define KRN_PORT_RANGE              "/proc/sys/net/ipv4/ip_local_port_range"
#define SYSTEM_MEMORY               "/proc/meminfo"
#define MEM_TOTAL                   "MemTotal"
#define MEM_AVAIL                   "MemAvailable"
#define DAT_FILE_EXT                ".dat"

/* Column Names */
#define COL_NM_CID                  "ClientID"
#define COL_NM_CLN_SESS             "Clean Session"
#define COL_NM_DEST_IP              "Destination IP"
#define COL_NM_DEST_PORT            "Destination Port"
#define COL_NM_LINGER               "Linger"
#define COL_NM_PSWD                 "Password"
#define COL_NM_RX_TOPIC             "RX TopicName"
#define COL_NM_SECURE               "Non-TLS/TLS"
#define COL_NM_TX_TOPIC             "TX TopicName"
#define COL_NM_UNAME                "Username"
#define COL_NM_WEBSOCKET            "WebSocket"
#define COL_NM_SSL_CCERT            "SSL Certificate"
#define COL_NM_SSL_CPKEY            "SSL Key"
#define COL_NM_MSG_DIR              "Message Directory"

/* Column Names Len */
#define LEN_COL_NM_CID              strlen(COL_NM_CID)
#define LEN_COL_NM_CLN_SESS         strlen(COL_NM_CLN_SESS)
#define LEN_COL_NM_DEST_IP          strlen(COL_NM_DEST_IP)
#define LEN_COL_NM_DEST_PORT        strlen(COL_NM_DEST_PORT)
#define LEN_COL_NM_LINGER           strlen(COL_NM_LINGER)
#define LEN_COL_NM_PSWD             strlen(COL_NM_PSWD)
#define LEN_COL_NM_RX_TOPIC         strlen(COL_NM_RX_TOPIC)
#define LEN_COL_NM_SECURE           strlen(COL_NM_SECURE)
#define LEN_COL_NM_TX_TOPIC         strlen(COL_NM_TX_TOPIC)
#define LEN_COL_NM_UNAME            strlen(COL_NM_UNAME)
#define LEN_COL_NM_WEBSOCKET        strlen(COL_NM_WEBSOCKET)
#define LEN_COL_NM_SSL_CCERT        strlen(COL_NM_SSL_CCERT)
#define LEN_COL_NM_SSL_CPKEY        strlen(COL_NM_SSL_CPKEY)
#define LEN_COL_NM_MSG_DIR          strlen(COL_NM_MSG_DIR)

/* CL_NUM_OF_COLS ('|') */
/* The following are the number of columns per enum clientListFormats; */
#define NUM_COLS_NO_LONGER_SUPPORTED           4
#define NUM_COLS_MTOPIC_EITHER_RX_OR_TX        6
#define NUM_COLS_MTOPIC_RXTX_DEST             10
#define NUM_COLS_MTOPIC_RXTX_DEST_LINGER      11
#define NUM_COLS_MTOPIC_RXTX_CERT_MSG         14

/* The type of clients that a thread contains. */
#define CONSUMERS_ONLY              0x01
#define CONTAINS_CONSUMERS          0x01
#define PRODUCERS_ONLY              0x02
#define CONTAINS_PRODUCERS          0x02
#define CONTAINS_DUALCLIENTS        0x03

#define MSG_SKIP                    0x0
#define MSG_SEND                    0x1

/**************************************** Enumerations *****************************************/
typedef enum {
	MQTT_CLIENT_BINARY = 1,			/* IoT (binary MQTT client) */
	MQTT_CLIENT_WSJSON				/* Mobile (JSON/WebSockets MQTT client */
} ENUM_MQTT_CLIENT_t;

/* Actions */
typedef enum {
	ACTION_BLOCKRECV = 0,			/* Not implemented */
	ACTION_BLOCKRECVTO,				/* Not implemented */
	ACTION_NONBLOCKRECV,			/* Not implemented */
	ACTION_MSGLISTENER,				/* asynchronous message delivery */
	ACTION_SEND 					/* send */
} ENUM_ACTION_t;

/* The type of latencies. */
typedef enum {
	LATENCY_RTT = 1,
	LATENCY_RECONN_RTT,
	LATENCY_CONN_SUB,
	LATENCY_SEND
} ENUM_LATENCY_TYPES;

/* 0 and 15 are "reserved" message types (i.e. should not receive these types) */
#define MQTT_NUM_MSG_TYPES 16

/* QoS Publish Types */
typedef enum {
	QOS_0_PUBLISH = 0,
	QOS_1_PUBLISH,
	QOS_2_PUBLISH
} consQoSPublishTypes;

/* Message Types */
typedef enum {
	CONNECT = 1,
	CONNACK,
	PUBLISH,
	PUBACK,
	PUBREC,
	PUBREL,
	PUBCOMP,
	SUBSCRIBE,
	SUBACK,
	UNSUBSCRIBE,
	UNSUBACK,
	PINGREQ,
	PINGRESP,
	DISCONNECT
} msgTypes;

/* Publish Message ID States */
typedef enum {
	PUB_MSGID_AVAIL = 0,
	PUB_MSGID_PUBLISH,
	PUB_MSGID_PUBACK,
	PUB_MSGID_PUBREC,
	PUB_MSGID_PUBREL,
	PUB_MSGID_PUBCOMP
} publishMsgIDStates;

#define PUB_MSGID_UNKNOWN_STATE 0x80

/* Subscribe Message ID States */
typedef enum {
	SUB_MSGID_AVAIL = 0,
	SUB_MSGID_SUBSCRIBE,
	SUB_MSGID_SUBACK
} subscribeMsgIDStates;

/* UnSubscribe Message ID States */
typedef enum {
	SUB_MSGID_UNSUBSCRIBE = 1,
	SUB_MSGID_UNSUBACK
} unSubscribeMsgIDStates;

/* Output locations */
typedef enum {
	OUTPUT_CONSOLE = 1,
	OUTPUT_CSVFILE
} outputLocations;

/* Syntax Help Display Types */
typedef enum {
	SHOW_COMMANDS_ONLY = 1,
	SHOW_ENV_VARS_ONLY,
	SHOW_ALL
} syntaxHelpDisplayTypes;

typedef enum {
	NO_HISTOGRAM = 0,
	PRINT_HISTOGRAM
} handleHistogram;

/* Display Types for GetStats */
typedef enum {
	DISPLAY_CONSUMER_ONLY = 1,
	DISPLAY_PRODUCER_ONLY,
	DISPLAY_BOTH
} outputDisplayTypes;

/* Values for displaying connections */
typedef enum {
	DONT_DISPLAY_GOOD_CONN = 0,
	DISPLAY_GOOD_CONN
} displayConnection;

/* The states for connection. */
typedef enum {
	CONN_SOCK_HSIP = 0,
	CONN_SOCK_CONN,
	CONN_SOCK_SDIP,
	CONN_SOCK_DISC,
	CONN_SOCK_ERR,
	CONN_SOCK_CREATE,
	CONN_MQTT_CIP,
	CONN_MQTT_CONN,
	CONN_MQTT_PS,
	CONN_MQTT_DIP,
	CONN_MQTT_DISC,
	NUM_CONNECT_STATES
} getStatConnectionStates;

/* Specific State & Condition for Client */
typedef enum {
	STATE_TYPE_TCP = 0,
	STATE_TYPE_MQTT,
	STATE_TYPE_UNIQUE
} getStateType;

/* Client Types */
typedef enum {
	CONNECTONLY = 0,
	CONSUMER,
	PRODUCER,
	DUAL_CLIENT
} clientTypes;

/* Type of work to perform */
typedef enum {
	SET_VAR = 1,
	TEST_VAR
}functionType;

/* How to Subscribe/Unsubscribe in regards to topics */
typedef enum {
	ALL_TOPICS = 1
} topicChoices;

/* Reset Type for mqttclient */
typedef enum {
	RESET_RECONNECT = 0,
	RESET_DISCONNECT
} resetType;

/* Clock Sources */
typedef enum {
	TSC = 0,
	GTOD
} clockSources;

/* File Removal Types */
typedef enum {
	REMOVAL_UNKNOWN = 0,
	REMOVE_EMPTY_FILE,
	REMOVE_FILE_EXISTS
} fileRemovalTypes;

/* File Functions */
typedef enum {
	FILEFUNC_UNKNOWN = 0,
	FIND_LONGEST_ENTRY,
	LOAD_ARRAY
} fileFunctionRequests;

/* File Types */
typedef enum {
	LOGFILE = 0,
	CLIENTINFO
} mqttbenchFileTypes;

/* Log Format Types */
/*
 *   1 - "%FT%T"
 *   2 - "%I:%M:%S %p"
 *   3 - "%c"
 */
typedef enum {
	LOGFORMAT_UNKNOWN = 0,
	LOGFORMAT_YY_MM_DD,
	LOGFORMAT_HH_MM_SS,
	LOGFORMAT_PREFERRED_DATE_TIME
} logformatType;

/* Kind of HashMap Entry Type */
typedef enum {
	FREE_HASHMAP_ENTRIES = 0,
	CLIENT_CERTIFICATE,
	CLIENT_PRIVATE_KEY,
	MESSAGE_DIRECTORY
} hashMapType;

/* Whether Client List IP Addrs were passed to preInitialize() */
typedef enum {
	UNKNOWN_INVOCATION = 0,
	CLIENTLIST_SIPLIST_IPS,
	IMASERVER_IPS,
	MACHINE_IPS
} routineInvocation;

/* Requested State Types */
typedef enum {
	UNKNOWN_TYPE = 0,
	CLIENT,
	TRANSPORT
} requestedStateTypes;

/* Time when requesting Thread Types */
typedef enum {
	THREADTYPE_UNDECLARED_TIME = 0,
	INIT_STRUCTURE_ONLY,
	PRELIM_THREADTYPE,
	ACTUAL_THREADTYPE
} requestedTimeThreadTypes;

/* Type of logging */
typedef enum {
	LOGONLY = 0,
	DISPLAYONLY,
	LOGANDDISPLAY
} logClassification;

/* */
typedef enum {
	PLACE_IN_TRACE = 1,
	LOG2TRACE_AND_DISPLAY,
	DISPLAY_STDOUT
} handleLogData;

/* Type of run info. */
typedef enum {
	INTERACTIVESHELL = 0,
	GENERALENV
} runTypeData;

/* Type of latency */
typedef enum {
	UNKNOWN_LATENCY = 0,
	RTT_LATENCY,
	SEND_LATENCY,
	TCPCONN_LATENCY,
	MQTTCONN_LATENCY,
	SUB_LATENCY
} histogramLatencyType;

/* Type of allocation for Jitter Arrays */
typedef enum {
	CREATION = 0,
	REALLOCATION
} jitterAllocationType;

/* Change Rate Origination */
typedef enum {
	UNKNOWN = 0,
	TIMERTASK,
	DOCOMMANDS
} changeRateOrigin;

/* Type of Burst Mode */
typedef enum {
	BASE_MODE = 0,
	BURST_MODE
} burstModeType;

typedef enum {
	LOG_DATA = 0,
	DISPLAY_DATA
} cmdsFreeMemory;

typedef enum {
	DISCONNECT_ONLY = 1,
	UNSUBSCRIBE_W_DISCONNECT
} shutdownTypes;

typedef enum {
	PARM_HELP_START = 0,
	PARM_CLIENTLIST,
	PARM_CRANGE,
	PARM_DURATION,
	PARM_COUNT,
	PARM_MSGRATE,
	PARM_MSGSIZE,
	PARM_PREDEFINED,
	PARM_MIM,
	PARM_NUMSUBTHRDS,
	PARM_MBID,
	PARM_TRACELEVEL,
	PARM_CLIENTTRACE,
	PARM_NODNSCACHE,
	PARM_NORECONN,
	PARM_RATECONTROL,
	PARM_VALIDATION,
	PARM_LATENCY,
	PARM_CUNITS,
	PARM_RUNITS,
	PARM_UNITS,
	PARM_LCSV,
	PARM_RESETLAT,
	PARM_SNAP,
	PARM_CSV,
	PARM_BURST,
	PARM_RSTATS,
	PARM_BSTATS,
	PARM_LINGER,
	PARM_APPSIM,
	PARM_SSEQ,
	PARM_NOWAIT,
	PARM_QUIT,
	PARM_DCI,
	PARM_ENV,
	PARM_PSK,
	PARM_JITTER,
	PARM_VERSION,
	PARM_HELP,
	PARM_HELP_END,
	HELP_ENV,
	HELP_ALL
} helpCommandParms;

typedef enum {
	ENV_HELP_START = 100,
	ENV_SIPLIST,
	ENV_IOPROCTHRDS,
	ENV_DELAYTIME,
	ENV_DELAYCOUNT,
	ENV_KEEPALIVE,
	ENV_SSLCIPHER,
	ENV_SSLMETHOD,
	ENV_SOURCEPORTLO,
	ENV_EPHEMERALPORTS,
	ENV_MAXIOPTXBUFS,
	ENV_LOGPATH,
	ENV_PIPECMDS,
	ENV_CONNHISTSIZE,
	ENV_MSGHISTSIZE,
	ENV_RETRYDELAY,
	ENV_RETRYBACKOFF,
	ENV_HELP_END
} helpEnvVars;

#endif /* MBCONSTANTS_H_ */
