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

#ifndef IOTROUTING_H
#define IOTROUTING_H

#include <ismutil.h>


#ifdef __cplusplus
extern "C" {
#endif
/**
 * Route Types
 */
typedef enum {
	ROUTE_TYPE_UNKNOWN,
    ROUTE_TYPE_KAFKA,
    ROUTE_TYPE_MQTT,
    ROUTE_TYPE_SQS,
	ROUTE_TYPE_MESSAGEHUB,
} ism_route_Type;


/**
 * Kafka Key Type
 */
typedef enum {
	KAFKA_KEYTYPE_BINARY,
    KAFKA_KEYTYPE_JSON
} ism_kafka_key_Type;


/*Kafka IM Messaging Stats*/
typedef struct px_routing_messaging_stat_t {
    volatile uint64_t C2PMsgsTotalSent;
    volatile uint64_t C2PBytesTotalSent;
    volatile uint64_t C2PMsgsTotalReceived;
    volatile uint64_t C2PBytesTotalReceived;

} px_routing_messaging_stat_t;
/**
 * Route Object
 */
typedef  struct ism_routing_route_t {
	char * 						name;
	int 		     				name_len;
	ism_route_Type 				route_type;
    char * 						kafka_topic;
    int          				kafka_topic_len;
    char * 						kafka_key;
    int							kafka_key_len;
    ism_kafka_key_Type			kafka_key_type;
    char * 						hosts;
    char * 						kafka_auth_id;
    char * 						kafka_auth_pw;
    int							useTLS;
    char * 						tlsMethod;
    char *						tlsCiphers;
    char *          				authenticator;
    int							kafka_batch_time_millis;
    int							kafka_batch_size;
    int							kafka_batch_size_bytes;
    int							kafka_recovery_batch_size;
    int							kafka_produce_ack;
    int							kafka_dist_method;
    int							kafka_shutdown_onerror_time;
    int 							send_mqtt;
    px_routing_messaging_stat_t stats;

} ism_routing_route_t;

/**
 * Routing rule object
 */
typedef  struct ism_routing_rule_t {
    char * 						name;   				//Routing Rule name
    int          				name_len;			//Routing Rule name length
    ismRule_t * 					selector; 			//Selector rule
    char * 						routeNames;
    int							routeNames_len;
    ismArray_t 					routes;   			//Array of ism_route_route_t objects
    concat_alloc_t *				userProperties;
    concat_alloc_t *				sysProperties;
} ism_routing_rule_t;

/**
 * Routing rule object
 */
typedef  struct ism_routing_config_t {
	int 				   enabled;
	int				   inited;
	//pthread_mutex_t    lock;
	pthread_rwlock_t	   lock;
	ismHashMap *       routemap;
	ismHashMap *       routerulemap;
} ism_routing_config_t;


/**
 * Initialized the routing config object
 */
int ism_route_config_init(ism_routing_config_t * routeConfig);

/**
 * Destroy the routing config object
 */
int ism_route_config_destroy(ism_routing_config_t * routeConfig);




/**
 * Parse Route Config from JSON Parse Object
 */
int ism_proxy_route_json(ism_json_parse_t * parseobj, int where, ism_routing_config_t * routeConfig) ;

/**
 * Parse Route Config from JSOn string
 */
int ism_proxy_parseRouteConfig(const char * routing_config, ism_routing_config_t * routeConfig);


/**
 * Route Message function
 */
//int ism_route_routeMessage(ism_transport_t * transport, const char * buf, int buf_len, int kind, int *sendMQTT);



#ifdef __cplusplus
}
#endif

#endif /* IOTROUTING_H */
