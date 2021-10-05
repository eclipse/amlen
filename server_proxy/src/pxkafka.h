/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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

#ifndef IOTKAFKA_H
#define IOTKAFKA_H

#include <ismutil.h>
#include <sasl_scram.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KafkaMetadata 1
#define KafkaMetering 2
#define KafkaIMMessaging 3
#define KafkaMessageHub 4



/**
 * Kafka Error Codes
 */
#define KAFKA_ERROR_UNKNOW				   		   	-1
#define	KAFKA_ERROR_NOERROR							0
#define	KAFKA_ERROR_OFFSETOUTOFRANGE					1
#define KAFKA_ERROR_INVALIDMESSAGE					2
#define KAFKA_ERROR_UNKNOWTOPICORPARTITION			3
#define KAFKA_ERROR_INVALIDMESSAGESIZE				4
#define KAFKA_ERROR_LEADERNOTAVAILABLE				5
#define	KAFKA_ERROR_NOTLEADERFORPARTITION			6
#define	KAFKA_ERROR_REQUESTTIMEOUT					7
#define	KAFKA_ERROR_NETWORKEXCEPTION					13
#define KAFKA_ERROR_NOTENOUGHREPLICAS				19
#define	KAFKA_ERROR_NOTENOUGHREPLICASAFTERAPPEND		20



/**
 * State of the Kafka Connection
 */
typedef enum ism_kafka_broker_state_e {
	KAFKA_STATE_BROKER_UNKNOWN	 		= 0,
	KAFKA_STATE_BROKER_CONNECTING  	 	= 1,       /* An unused transport object                */
	KAFKA_STATE_BROKER_AUTH_REQUEST    	= 2,       /* Messages are processed only in this state */
	KAFKA_STATE_BROKER_AUTHENTICATING  	= 3,       /* In the process of starting up             */
	KAFKA_STATE_BROKER_AUTHENTICATED	= 4,        /* In the process of closing                 */
	KAFKA_STATE_BROKER_NOT_AUTHORIZED	= 5        /* In the process of closing                 */
}ism_kafka_broker_state_e;
/*
 * Binary im message or event entry.
 */
typedef struct kafka_produce_msg_t {
    struct kafka_produce_msg_t * next;
    uint64_t  				time;
    char * 					buf;           /* Payload */
    int 						buflen;
    int						key_len;
    char *  					key;
    uint32_t 				dcrc;
    uint32_t					waitValue;
    int     					hdr_len;
    int     					hdrcount;
    uint8_t * 				hdr;
    uint64_t  				waitID;
} kafka_produce_msg_t;

#define    KAFKA_PARTITION_CONN_OPEN   		0x1
#define    KAFKA_PARTITION_CONN_OPENING   	0x2
#define    KAFKA_PARTITION_CONN_CLOSING   	0x4
#define    KAFKA_PARTITION_CONN_CLOSED   	0x8


/*Kafka IM Messaging Stats*/
typedef struct px_kafka_im_messaging_stat_t {
	int connected;
	int partid;
	char * topic;
	volatile uint64_t kakfaC2PMsgsTotalSent;
    volatile uint64_t kakfaC2PBytesTotalSent;
    volatile uint64_t kakfaC2PMsgsTotalReSent;
    volatile uint64_t kakfaC2PBytesTotalReSent;
    volatile uint64_t kakfaC2PMsgsTotalReceived;
    volatile uint64_t kakfaC2PBytesTotalReceived;
    volatile uint64_t kakfaTotalPendingMsgsCount;
    volatile uint64_t kakfaTotalBatchProduceCount;
    volatile uint64_t kakfaTotalBatchReProduceCount;
    volatile uint64_t kakfaTotalBatchProduceAckReceivedCount;
    volatile uint64_t kakfaTotalBatchProduceAckErrorCount;
    volatile uint64_t kafkaProduceLatencyMS;
    volatile uint64_t kafkaTotalProduceLatencyMS;
    volatile uint64_t kafkaProduceMaxLatencyMS;
    volatile uint64_t kafkaProduceTotalThrottleTimeMS;
    volatile uint64_t kafkaMsgInBatchWaitTimeMS;
    volatile uint64_t kafkaMsgInBatchMaxWaitTimeMS;
    volatile uint64_t kafkaPartitionChangedCount;
    volatile uint64_t kafkaPartitionMsgsTransferredCount;
    volatile uint64_t kafkaC2PMsgsTotalFiltered;
} px_kafka_messaging_stat_t;

XAPI int ism_proxy_getKafkaIMMessagingStats(px_kafka_messaging_stat_t * stats) ;

/*
 * The Authenticator which authenticate the Kafka Connection
 *
 * @param callback_data
 * @return A return code: 0=good
 */
typedef int  (* ism_kafka_authenticator_t)(ism_transport_t * transport, void  * callback_data);



/*
 * Callback when the Kafka Authentication step is completed.
 * @param rc
 * @param callback_data
 * @return A return code: 0=good
 */
typedef int  (* ism_kafka_authenticated_t)(int rc, void  * callback_data);



typedef struct kafkaTopicRecord_t {
	pthread_spinlock_t 		lock;
    ism_server_t *          server;
    ism_transport_t *      	metadata_transport;
    int						needmetadata;
    int                     index;
    char * 					topic;
    ismArray_t              partitionConnections;
    int						connectionCount;
    int 					topic_part_count;
    int						kafka_batch_time;       	//Batch Time in Nanos
    int						kafka_batch_time_millis; //Batch Time in Millis
   	int						kafka_batch_size;
   	int						kafka_batch_size_bytes;
   	int						kafka_recovery_batch_size;
   	int						kafka_produce_ack;
   	int						kafka_dist_method;
   	ism_kafka_authenticator_t     authenticator;       /**< Kafka Connector Authenticator*/
	void *						  authenticator_data;
    int                     connectionErrorCount;
	ism_time_t				connectionErrorFirstTimeInNanos;
	int						kafka_shutdown_onerror_time;

} kafkaTopicRecord_t;

typedef struct kafkaConnectionRecord_t {
	pthread_mutex_t 		lock;
    ism_server_t *          server;
    ism_transport_t *       produce_transport;
    uint8_t             	state;
    int                     index;
    int                     nodeID;
    int                     partID;
    char * 					topic;
    ism_timer_t             kafka_timer;
    kafkaTopicRecord_t * 	topicRecord;
    /*List of messages for this connection*/
    int						hasackwait;
    kafka_produce_msg_t	* 	kafka_ackwait_msg_first;
	kafka_produce_msg_t	* 	kafka_ackwait_msg_last;
	kafka_produce_msg_t	* 	kafka_msg_first;
	kafka_produce_msg_t	* 	kafka_msg_last;
	int						kafka_msg_count;
	uint64_t				kafka_msg_first_time;
	px_kafka_messaging_stat_t stats;
	int 					produceErrorCount;
	int 					produceErrorTotalCount;
	int 					produceLastRC;
	ism_time_t				produceErrorFirstTimeInNanos;
	double	 				lastProduceTime;
	double					sumProduceLatency;
} kafkaConnectionRecord_t;

#ifndef NO_KAFKA_POBJ
/*
 * pobj structure for kafka connections
 */
typedef struct ism_protobj_t {
    ism_transport_t *        transport;
    ism_server_t *           server;
    pthread_spinlock_t       lock;
    int						 auth_require;
    char * 					 topic;
    volatile ism_kafka_broker_state_e broker_state;
    volatile ism_serverConnection_State  state;
    int                      kafka_type;
    int                      nodeID;
    int                      partID;
    int						 index;
    ism_kafka_authenticator_t     authenticator;       /**< Kafka Connector Authenticator*/
    void *						  authenticator_data;
    ism_kafka_authenticated_t     authenticated;       /**< Method to call to when authentication is completed*/
    kafkaConnectionRecord_t * kafkaConnRecord;
    kafkaTopicRecord_t * 	topicRecord;
    uint64_t                lastOffset;
    ism_sasl_scram_context * sals_scram_context;
} ism_kafka_con_t;
#endif


int ism_kafka_authenticate(ism_transport_t * transport, int rc);

int ism_kafka_metadata_conn_authenticated(int rc, void * data);

int ism_kafka_messaging_conn_authenticated(int rc, void * data);

typedef struct kafkaSASLAuth_t {

	char *					authid;
	int						authid_len;
	char *					password;
	int						password_len;
} kafkaSASLAuth_t;

int ism_kafka_metadata_conn_authenticator_sasl_plain(ism_transport_t *        transport, void * data);

int ism_kafka_addTopicRecord(const char * kafkaTopic, kafkaTopicRecord_t * topicRecord);

kafkaTopicRecord_t *  ism_kafka_removeTopicRecord(const char * kafkaTopic);

kafkaTopicRecord_t * ism_kafka_getTopicRecord(const char * kafkaTopic);

int ism_kafka_createTopicRecord(	const char * kafkaBrokers, 	const char * kafkaTopic, ism_kafka_authenticator_t  authenticator, const char * kafkaAuthID, 	const char * kafkaAuthPW,
							int useTLS, const char * tlsMethod, const char * tlsCiphers,
							int kafka_batch_time_millis, int kafka_batch_size, int kafka_recovery_batch_size,  int kafka_recovery_batch_size_bytes, int kafka_produce_ack,
							int kafka_dist_method, int kafka_shutdown_onerror_time);


#ifdef __cplusplus
}
#endif

#endif /* IOTREST_H */
