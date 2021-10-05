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
#ifndef TRACE_COMP
#define TRACE_COMP Routing
#endif
#include <ismutil.h>
#include <imacontent.h>
#include <ismjson.h>
#include <log.h>
#include <selector.h>
#include <pxtransport.h>
#include <pxrouting.h>
#include <pxkafka.h>
#include <pxmhub.h>
#include <pxmqtt.h>

int ism_proxy_parseMQTTTopic(char * topic, const char * * devtype, const char * * devid, const char * * event, const char * * fmt);
int ism_route_routing_start(void);
int ism_route_config_startConnections(ism_routing_config_t * routeconfig);
extern int ism_proxy_routeToKafka(ism_tenant_t * tenant, int qos);

extern const char * getJsonValue(ism_json_entry_t * ent);
extern void ism_kafka_makeEventKey(concat_alloc_t * buf, const char * org, const char * type, const char * id, const char * event,
        const char * fmt, const char * uuid_bin);
int ism_kafka_makeKafkaHeaders(ism_transport_t * transport, concat_alloc_t * buf, mqtt_pmsg_t * pmsg,
        concat_alloc_t * sysprops, concat_alloc_t * userprops, int msgver);
int ism_mqtt_getPublishXPayload(ism_transport_t * transport, mqtt_pmsg_t * pmsg) ;
extern int ism_mhub_selectMessages(ism_mhub_t * mhub, uint16_t * topicix, int count, const char * type, const char * event, mqtt_pmsg_t * pmsg);
extern void ism_mhub_makeKey(ism_mhub_t * mhub, concat_alloc_t * buf, const char * org, const char * type, const char * id,
        const char * event, const char * fmt) ;
void ism_mhub_makeKeyMap(ism_mhub_t * mhub, concat_alloc_t * buf, ism_transport_t * transport, mqtt_pmsg_t * pmsg);
int ism_mhub_publishEvent(ism_mhub_t * mhub, mqtt_pmsg_t * pmsg, const char * clientID, int topic_index, int partition);
void ism_mhub_lock(ism_mhub_t * mhub);
void ism_mhub_unlock(ism_mhub_t * mhub);
uint32_t ism_mhub_getPartition(const char * type, const char * id);
uint32_t ism_mhub_getPartitionMap(ism_mhub_t * mhub, concat_alloc_t * buf, ism_transport_t * transport, mqtt_pmsg_t * pmsg);


extern int g_monitor_started;
extern int ism_proxy_routeSendMQTT(int msgRouting, int qos);
extern int g_useKafkaIMMessaging;
extern ismRule_t *  g_IMMessagingSelector;
extern int g_mhubEnabled;
extern pthread_spinlock_t   g_mhubStatLock;
extern px_kafka_messaging_stat_t mhubMessagingStats ;
extern ism_tenant_t eventTenant;
extern int g_useMHUBKafkaConnection;

ism_routing_route_t * g_internalKafkaRoute=NULL;

/*Global Route Config*/
ism_routing_config_t *       g_routeconfig=NULL;

int g_msgRoutingEnabled=0;
int g_msgRoutingSysPropsEnabled=0;
int g_msgRoutingDefaultSendMQTT=1;
int g_messageRoutingDefault=0;

int g_msgRoutingNeedStart =1;

static ism_route_Type getRouteType(const char * typeStr)
{
	if(typeStr!=NULL){
		if (!strcmpi(typeStr, "kafka")) {
			return ROUTE_TYPE_KAFKA;
		}else if (!strcmpi(typeStr, "mqtt")) {
			return ROUTE_TYPE_MQTT;
		}else if (!strcmpi(typeStr, "awssqs")) {
			return ROUTE_TYPE_SQS;
		}

	}
	return ROUTE_TYPE_UNKNOWN;
}

void ism_route_route_destroy(void * inroute)
{
	if(inroute){
		ism_routing_route_t * route = (ism_routing_route_t *) inroute;
		if(route->name) ism_common_free(ism_memory_proxy_routing,route->name);
		if(route->kafka_key) ism_common_free(ism_memory_proxy_routing,route->kafka_key);
		if(route->kafka_topic) ism_common_free(ism_memory_proxy_routing,route->kafka_topic);

	}

}

void ism_route_routerule_destroy(void * inrouteRule)
{
	if(inrouteRule){
		ism_routing_rule_t * routeRule = (ism_routing_rule_t *) inrouteRule;
		if(routeRule->name) ism_common_free(ism_memory_proxy_routing,routeRule->name);
		if(routeRule->routeNames) ism_common_free(ism_memory_proxy_routing,routeRule->routeNames);
		if(routeRule->routes) ism_common_destroyArray(routeRule->routes);
		if (routeRule->selector) ism_common_freeSelectRule(routeRule->selector);
		if(routeRule->userProperties){
			ism_common_freeAllocBuffer(routeRule->userProperties);
			ism_common_free(ism_memory_proxy_routing,routeRule->userProperties);
		}
		if(routeRule->sysProperties){
			ism_common_freeAllocBuffer(routeRule->sysProperties);
			ism_common_free(ism_memory_proxy_routing,routeRule->sysProperties);
		}

	}

}


int ism_route_config_init(ism_routing_config_t * routeConfig)
{
	if(routeConfig){
		if(!routeConfig->inited){
			pthread_rwlock_init(&routeConfig->lock, 0);
			routeConfig->routemap = ism_common_createHashMap(128,HASH_STRING);
			routeConfig->routerulemap = ism_common_createHashMap(128,HASH_STRING);
			TRACE(5, "Route Config Object is initialized\n");
			routeConfig->inited=1;
		}
	}else{
		TRACE(2, "Route Config Object is NULL\n");
		return 1;
	}

	return 0;
}

int ism_route_config_destroy(ism_routing_config_t * routeConfig)
{
	if(routeConfig){
		pthread_rwlock_wrlock(&routeConfig->lock);
		//Destroy RouteRules
		if(routeConfig->routerulemap){
			ism_common_destroyHashMapAndFreeValues(routeConfig->routerulemap, ism_route_routerule_destroy);
		}
		//Destroy Routes
		if(routeConfig->routemap){
			ism_common_destroyHashMapAndFreeValues(routeConfig->routemap, ism_route_route_destroy);
		}
		pthread_rwlock_unlock(&routeConfig->lock);
		pthread_rwlock_destroy(&routeConfig->lock);
		ism_common_free(ism_memory_proxy_routing,routeConfig);
	}else{
		TRACE(2, "Route Config Object is NULL\n");
		return 1;
	}
	return 0;
}

/*
 * Put one character to a concat buf
 */
static void bputchar(concat_alloc_t * buf, char ch) {
    if (buf->used + 1 < buf->len) {
        buf->buf[buf->used++] = ch;
    } else {
        char chx = ch;
        ism_common_allocBufferCopyLen(buf, &chx, 1);
    }
}


int putUserPropertyPair(concat_alloc_t * buf, const char * name, int namelen, const char * value, int len) {

	if (len < 0)
		len = strlen(value);
	if (namelen< 0)
		namelen = strlen(name);
	bputchar(buf, namelen>>8);
	bputchar(buf, namelen);
	if (namelen)
		ism_common_allocBufferCopyLen(buf, name, namelen);
	bputchar(buf, len>>8);
	bputchar(buf, len);
	if (len)
		ism_common_allocBufferCopyLen(buf, value, len);

    return 0;
}

/*
 * Make a user object from the configuration.
 * TODO: validate the values
 */
int makeUserProperties(ism_json_parse_t * parseobj, int where, concat_alloc_t *	 userProperties) {
    int endloc;
    int  rc = 0;
    char xbuf[1024];

    if (!parseobj || where > parseobj->ent_count || userProperties==NULL)
        return 1;
    ism_json_entry_t * ent = parseobj->ent+where;
    endloc = where + ent->count;
    where++;


    int savewhere = where;
    while (where <= endloc) {
        ent = parseobj->ent + where;
        // printf("ent->name: %s\n", ent->name);
		if (ent->objtype != JSON_String) {
			ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", ent->name, getJsonValue(ent));
			rc = ISMRC_BadPropertyValue;
		}

        if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
            where += ent->count + 1;
        else
            where++;
    }

    if (rc == 0) {
        where = savewhere;
        while (where <= endloc) {
            ent = parseobj->ent + where;
            //printf("ent->name: %s ent->value=%s\n", ent->name, ent->value);
            //Encode the buffer
            if(ent->value!=NULL)
            		putUserPropertyPair(userProperties, ent->name, -1, ent->value, -1);
            if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
                where += ent->count + 1;
            else
                where++;
        }
    } else {
        ism_common_formatLastError(xbuf, sizeof xbuf);
        LOG(ERROR, Server, 934, "%-s%u", "User Property configuration error:  Error={1} Code={2}",
                             xbuf, ism_common_getLastError());
    }


    return rc;
}

int makeRoute(ism_json_parse_t * parseobj, int where, const char * name, ism_routing_config_t * routeConfig)
{

	int endloc;
	int  rc = 0;
	ism_route_Type routeType = ROUTE_TYPE_UNKNOWN;

	if (!parseobj || where > parseobj->ent_count)
		return 1;
	ism_json_entry_t * ent = parseobj->ent+where;
	endloc = where + ent->count;
	where++;
	if (!name)
		name = ent->name;

	pthread_rwlock_wrlock(&routeConfig->lock);

	ism_routing_route_t * route =  ism_common_getHashMapElement(routeConfig->routemap, name, strlen(name));
	if(route==NULL){
		 if (ent->objtype == JSON_Object) {
			 route = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_routing,1),1, sizeof(ism_routing_route_t));
			 route->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_routing,1000),name);
			 route->name_len = strlen(name);
			 ism_common_putHashMapElement(routeConfig->routemap, route->name, route->name_len, (void *)route, NULL);
			 TRACE(8,"Proxy Routing: Route: Route Object is created: name=%s\n", route->name);
		 }
	}else{
		routeType = route->route_type;
	}
	int savewhere = where;
	while (where <= endloc) {
		ent = parseobj->ent + where;
		if (!strcmpi(ent->name, "RouteType")) {
			//printf("RouteType: %s\n", ent->value);
			if (ent->objtype != JSON_String) {
			  ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "RouteType", getJsonValue(ent));
			  rc = ISMRC_BadPropertyValue;
			}else{
				routeType = getRouteType(ent->value);
				if(routeType == ROUTE_TYPE_UNKNOWN){
					ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "RouteType", getJsonValue(ent));
					rc = ISMRC_BadPropertyValue;
				}
			}
		} else if (!strcmpi(ent->name, "KafkaTopic")) {
		   //printf("Topic: %s\n", ent->value);
		   if (ent->objtype != JSON_String) {
			    ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KafkaTopic", getJsonValue(ent));
				rc = ISMRC_BadPropertyValue;
			}
		}else if (!strcmpi(ent->name, "KafkaKey")) {
		   //printf("Key: %s\n", ent->value);
		   if (ent->objtype != JSON_String) {
			  ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KafkaKey", getJsonValue(ent));
			  rc = ISMRC_BadPropertyValue;
			}
		} else if (!strcmpi(ent->name, "Hosts")) {
		   //printf("Key: %s\n", ent->value);
		   if (ent->objtype != JSON_String) {
			  ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Hosts", getJsonValue(ent));
			  rc = ISMRC_BadPropertyValue;
			}
		} else if (!strcmpi(ent->name, "UseTLS")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "UseTLS", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        } else if (!strcmpi(ent->name, "TLSMethod")) {
  		   //printf("Key: %s\n", ent->value);
  		   if (ent->objtype != JSON_String) {
  			  ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "TLSMethod", getJsonValue(ent));
  			  rc = ISMRC_BadPropertyValue;
  			}
  		} else if (!strcmpi(ent->name, "TLSCiphers")) {
   		   //printf("Key: %s\n", ent->value);
   		   if (ent->objtype != JSON_String) {
   			  ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "TLSCiphers", getJsonValue(ent));
   			  rc = ISMRC_BadPropertyValue;
   			}
   		} else if (!strcmpi(ent->name, "KafkaAuthID")) {
 		   //printf("Key: %s\n", ent->value);
 		   if (ent->objtype != JSON_String) {
 			  ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KafkaAuthID", getJsonValue(ent));
 			  rc = ISMRC_BadPropertyValue;
 			}
 		} else if (!strcmpi(ent->name, "KafkaAuthPW")) {
  		   //printf("Key: %s\n", ent->value);
  		   if (ent->objtype != JSON_String) {
  			  ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KafkaAuthPW", getJsonValue(ent));
  			  rc = ISMRC_BadPropertyValue;
  			}
  		} else if (!strcmpi(ent->name, "KafkaAuthPW")) {
   		   //printf("Key: %s\n", ent->value);
   		   if (ent->objtype != JSON_String) {
   			  ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KafkaAuthPW", getJsonValue(ent));
   			  rc = ISMRC_BadPropertyValue;
   			}
   		} else if (!strcmpi(ent->name, "KafkaBatchTimeMillis")) {
   		   //printf("Key: %s\n", ent->value);
   			if (ent->objtype != JSON_Integer || ent->count < 0) {
				ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KafkaBatchTimeMillis", getJsonValue(ent));
				rc = ISMRC_BadPropertyValue;
			}
   		} else if (!strcmpi(ent->name, "KafkaBatchSize")) {
    		   //printf("Key: %s\n", ent->value);
   			if (ent->objtype != JSON_Integer || ent->count < 0) {
				ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KafkaBatchSize", getJsonValue(ent));
				rc = ISMRC_BadPropertyValue;
			}
    		} else if (!strcmpi(ent->name, "KafkaProduceAck")) {
		   //printf("Key: %s\n", ent->value);
    			if (ent->objtype != JSON_Integer || ent->count < 0) {
				ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KafkaProduceAck", getJsonValue(ent));
				rc = ISMRC_BadPropertyValue;
			}
		} else if (!strcmpi(ent->name, "KafkaDistMethod")) {
		   //printf("Key: %s\n", ent->value);
			if (ent->objtype != JSON_Integer || ent->count < 0) {
				ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "KafkaDistMethod", getJsonValue(ent));
				rc = ISMRC_BadPropertyValue;
			}
		} else if (!strcmpi(ent->name, "SendMQTT")) {
            if (ent->objtype != JSON_True && ent->objtype != JSON_False) {
                ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "SendMQTT", getJsonValue(ent));
                rc = ISMRC_BadPropertyValue;
            }
        }

		if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
			where += ent->count + 1;
		else
			where++;
	}

	if(routeType==ROUTE_TYPE_UNKNOWN){
		ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "RouteType", getJsonValue(ent));
		rc = ISMRC_BadPropertyValue;
	}else{
		route->route_type = routeType;
	}

	if (rc == 0) {
		where = savewhere;
		while (where <= endloc) {
			ent = parseobj->ent + where;
			if (!strcmpi(ent->name, "KafkaTopic")) {
			   TRACE(8,"Proxy Routing: Route: KafkaTopic: %s\n", ent->value);
			   route->kafka_topic = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_routing,1000),ent->value);
			   route->kafka_topic_len = strlen(route->kafka_topic);

			} else if (!strcmpi(ent->name, "KafkaKey")) {
				TRACE(8,"Proxy Route: Route: KafkaKey: %s\n", ent->value);
				route->kafka_key = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_routing,1000),ent->value);

			} else if (!strcmpi(ent->name, "Hosts")) {
				TRACE(8,"Proxy Routing: Route: Hosts: %s\n", ent->value);
				route->hosts = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_routing,1000),ent->value);

			} else if (!strcmpi(ent->name, "UseTLS")) {
				TRACE(8,"Proxy Routing: Route: UseTLS: %s\n", ent->value);
				route->useTLS = ent->objtype == JSON_True;

			} else if (!strcmpi(ent->name, "TLSMethod")) {
				TRACE(8,"Proxy Route: Route: TLSMethod: %s\n", ent->value);
				route->tlsMethod = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_routing,1000),ent->value);

			} else if (!strcmpi(ent->name, "TLSCiphers")) {
				TRACE(8,"Proxy Route: Route: TLSCiphers: %s\n", ent->value);
				route->tlsCiphers = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_routing,1000),ent->value);

			} else if (!strcmpi(ent->name, "KafkaAuthID")) {
				TRACE(8,"Proxy Route: Route: KafkaAuthID: %s\n", ent->value);
				route->kafka_auth_id = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_routing,1000),ent->value);

			} else if (!strcmpi(ent->name, "KafkaAuthPW")) {
				TRACE(8,"Proxy Route: Route: KafkaAuthPW: %s\n", ent->value);
				route->kafka_auth_pw = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_routing,1000),ent->value);

			} else if (!strcmpi(ent->name, "KafkaKeyType")) {
				TRACE(8,"Proxy Route: Route: KafkaKeyType: %s\n", ent->value);
				ism_kafka_key_Type keyType=KAFKA_KEYTYPE_BINARY;
				if(!strcmpi(ent->value, "BINARY")){
					keyType=KAFKA_KEYTYPE_BINARY;
				} else if (!strcmpi(ent->value, "JSON")){
					keyType=KAFKA_KEYTYPE_JSON;
				}
				route->kafka_key_type = keyType;

			} else if (!strcmpi(ent->name, "KafkaBatchTimeMillis")) {
				TRACE(8,"Proxy Route: Route: KafkaBatchTimeMillis: %s\n", ent->value);
				route->kafka_batch_time_millis = ent->count;

			} else if (!strcmpi(ent->name, "KafkaBatchSize")) {
				TRACE(8,"Proxy Route: Route: KafkaBatchSize: %s\n", ent->value);
				route->kafka_batch_size = ent->count;

			} else if (!strcmpi(ent->name, "KafkaProduceAck")) {
				TRACE(8,"Proxy Route: Route: KafkaProduceAck: %s\n", ent->value);
				route->kafka_produce_ack = ent->count;
				if(route->kafka_produce_ack != 0 && route->kafka_produce_ack != 1 && route->kafka_produce_ack != -1){
					TRACE(1,"Proxy Route: Route: KafkaProduceAck value is not valid %s\n", ent->value);
					route->kafka_produce_ack=0;
				}

			} else if (!strcmpi(ent->name, "KafkaDistMethod")) {
				TRACE(8,"Proxy Route: Route: KafkaDistMethod: %s\n", ent->value);
				route->kafka_dist_method = ent->count;
			} else if (!strcmpi(ent->name, "SendMQTT")) {
				TRACE(8,"Proxy Route: Route: SendMQTT: %s\n", ent->value);
				route->send_mqtt= ent->count;
	        }

			if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
				where += ent->count + 1;
			else
				where++;
		}
	}

	pthread_rwlock_unlock(&routeConfig->lock);

	return rc;
}
/**
 * parse Routing Rule
 */

int parseRoute(ism_json_parse_t * parseobj, int where, ism_routing_config_t * routeConfig)
{

	int   rc = 0;
	ism_json_entry_t * ent;
	int endloc;
	if (!parseobj || where > parseobj->ent_count) {
		TRACE(2, "Route onfig JSON not correct\n");
		return 1;
	}
	ent = parseobj->ent+where;
	if (!ent->name || strcmp(ent->name, "Route") || ent->objtype != JSON_Object) {
		TRACE(2, "Route config JSON invoked for config which is not a Route object\n");
		return 2;
	}

	endloc = where + ent->count;
	where++;
	ent++;
	while (where <= endloc) {
		int xrc = makeRoute(parseobj, where, NULL, routeConfig);
		if (rc == 0)
			rc = xrc;
		ent = parseobj->ent+where;
		if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
			where += ent->count + 1;
		else
			where++;
	}



	return 0;

}


int makeRouteRule(ism_json_parse_t * parseobj, int where, const char * name, ism_routing_config_t * routeConfig)
{

	int endloc;
	int  rc = 0;
	char valcopy[1024];

	if (!parseobj || where > parseobj->ent_count)
		return 1;
	ism_json_entry_t * ent = parseobj->ent+where;
	endloc = where + ent->count;
	where++;
	if (!name)
		name = ent->name;

	pthread_rwlock_wrlock(&routeConfig->lock);

	ism_routing_rule_t * routeRule =  ism_common_getHashMapElement(routeConfig->routerulemap, name, strlen(name));
	if(routeRule==NULL){
		 if (ent->objtype == JSON_Object) {
			 routeRule = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_routing,2),1, sizeof(ism_routing_route_t));
			 routeRule->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_routing,1000),name);
			 routeRule->name_len = strlen(name);
			 routeRule->routes = ism_common_createArray(32);
			 ism_common_putHashMapElement(routeConfig->routerulemap, routeRule->name, routeRule->name_len, (void *)routeRule, NULL);

			 if(routeRule->sysProperties!=NULL){
				 ism_common_freeAllocBuffer(routeRule->sysProperties);
				 ism_common_free(ism_memory_proxy_routing,routeRule->sysProperties);
			 }

			char*  tbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_routing,3),256);
			memset(tbuf, 0, 256);
			routeRule->sysProperties = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_routing,4),1,sizeof(concat_alloc_t));
			routeRule->sysProperties->buf = tbuf;
			routeRule->sysProperties->len = 256;
			routeRule->sysProperties->buf[0]=0;
			routeRule->sysProperties->used++;


		 }
	}else{
		//Clear the routes info
		ism_common_destroyArray(routeRule->routes);
		routeRule->routes = ism_common_createArray(32);

	}


	int savewhere = where;
	while (where <= endloc) {
		ent = parseobj->ent + where;
		if (!strcmpi(ent->name, "Selector")) {
		  if (ent->objtype != JSON_String) {
			  ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Selector", getJsonValue(ent));
			  rc = ISMRC_BadPropertyValue;
		  }
		}else if (!strcmpi(ent->name, "Routes")) {
			if (ent->objtype != JSON_String) {
			  ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", "Route", getJsonValue(ent));
			  rc = ISMRC_BadPropertyValue;
			}
		}
		if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
			where += ent->count + 1;
		else
			where++;
	}

	if (rc == 0) {
		where = savewhere;
		while (where <= endloc) {
			ent = parseobj->ent + where;
			if (!strcmpi(ent->name, "Selector")) {

				int ruleLen;
				rc = ism_common_compileSelectRuleOpt(&routeRule->selector,
				                                              (int *)&ruleLen,
															  ent->value, SELOPT_Internal);
				TRACE(8, "Proxy Routing: Rule: Selector: %s compileRC=%d ruleLen=%d\n", ent->value, rc, ruleLen);

			} else if (!strcmpi(ent->name, "Routes")) {
				routeRule->routeNames = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_routing,1000),ent->value);
				routeRule->routeNames_len = strlen(routeRule->routeNames);
				TRACE(8,"Proxy Routing: Route: %s\n", ent->value);
				/*Create the Check if Route Existed*/
				 char * more = (char *)routeRule->routeNames;
				 char * token=NULL;

				 while ((token = ism_common_getToken(more, " \t,", " \t,", &more))) {
					 strcpy(valcopy, token);
					 TRACE(8,"Proxy Route: Rule: %s\n", valcopy);
					 ism_routing_route_t * aroute =  ism_common_getHashMapElement(routeConfig->routemap, valcopy, strlen(valcopy));
					 if(aroute!=NULL){
						 TRACE(8, "Found Route object Token: routeName=%s\n", aroute->name);
						 ism_common_addToArray(routeRule->routes, aroute);
					 }
				 }

			}else if (!strcmpi(ent->name, "UserProperties")) {
				if(routeRule->userProperties!=NULL){
					ism_common_freeAllocBuffer(routeRule->userProperties);
					ism_common_free(ism_memory_proxy_routing,routeRule->userProperties);
				}
				if (ent->objtype == JSON_Object) {
					routeRule->userProperties = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_routing,5),1,sizeof(concat_alloc_t));
					ism_common_allocAllocBufferOnHeap(routeRule->userProperties,256);
				}

				if(routeRule->userProperties!=NULL){
					makeUserProperties(parseobj,where, routeRule->userProperties);
				}

			}else if (!strcmpi(ent->name, "Processor")) {

				if(routeRule->sysProperties!=NULL){
					//Encode Processor Property
					 routeRule->sysProperties->buf[0]+=1;
					 char * value = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_routing,1000),ent->value);
					 ism_field_t field = {0};
					 if (value) {
						 field.type = VT_String;
						 field.val.s = (char *)value;
					 }
					 ism_common_putProperty(routeRule->sysProperties, ent->name, &field);
				}else{
					//Empty out user properties
				}

			}

			if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
				where += ent->count + 1;
			else
				where++;
		}
	}
	pthread_rwlock_unlock(&routeConfig->lock);
	return rc;
}

/**
 * parse Routing Config
 */

int parseRouteRule(ism_json_parse_t * parseobj, int where, ism_routing_config_t * routeConfig)
{

	int   rc = 0;
	ism_json_entry_t * ent;
	int endloc;
	if (!parseobj || where > parseobj->ent_count) {
		TRACE(2, "RouteRule onfig JSON not correct\n");
		return 1;
	}
	ent = parseobj->ent+where;
	if (!ent->name || strcmp(ent->name, "RouteRule") || ent->objtype != JSON_Object) {
		TRACE(2, "RouteRule config JSON invoked for config which is not a Route object\n");
		return 2;
	}

	endloc = where + ent->count;
	where++;
	ent++;
	while (where <= endloc) {
		int xrc = makeRouteRule(parseobj, where, NULL, routeConfig);
		if (rc == 0)
			rc = xrc;
		ent = parseobj->ent+where;
		if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
			where += ent->count + 1;
		else
			where++;
	}



	return 0;

}

/**
 * Parse Route Config from JSON Object
 */
int ism_proxy_route_json(ism_json_parse_t * parseobj, int where, ism_routing_config_t * routeConfig) {
	int   rc = 0;
	int   xrc;
	int   endloc;
	ism_json_entry_t * ent;

	TRACE(5,"Proxy Route: Parsing Route Configuration.\n");

	if (!parseobj || where > parseobj->ent_count) {
		TRACE(2, "Proxy config JSON not correct\n");
		return 1;
	}

	ent = parseobj->ent+where;

    endloc = where + ent->count;
	where++;
	//ent++;

	while (where <= endloc) {
		ent = parseobj->ent + where;
		if (ent->name) {
			if (!strcmp(ent->name, "Route")) {
				xrc = parseRoute(parseobj, where, routeConfig);
				if (rc == 0)
					rc = xrc;
			}else if (!strcmp(ent->name, "RouteRule")) {
				xrc = parseRouteRule(parseobj, where, routeConfig);
				if (rc == 0)
					rc = xrc;
			}
		 }
		if (ent->objtype == JSON_Object || ent->objtype == JSON_Array)
			where += ent->count + 1;
		else
			where++;
	}

	TRACE(5,"Proxy Route: Parsing Route Configuration completed. RC=%d\n", rc);

	if(g_monitor_started){
		ism_route_config_startConnections(routeConfig);
	}

	return rc;

}

/**
 * Parse Route Config from Raw JSON String
 */
int ism_proxy_parseRouteConfig(const char * routing_config, ism_routing_config_t * routeConfig)
{

	int rc = 0;
	//int xrc = 0;
	ism_json_parse_t parseobj = { 0 };
	ism_json_entry_t ents[500];
	parseobj.ent_alloc = 500;
	parseobj.ent = ents;
	int len = strlen(routing_config);
	parseobj.source = ism_common_malloc(ISM_MEM_PROBE(ism_memory_proxy_routing,6),len);
	memcpy(parseobj.source, routing_config, len);
	parseobj.src_len = len;
	parseobj.options = JSON_OPTION_COMMENT;   /* Allow C style comments */
	rc = ism_json_parse(&parseobj);
	if (rc) {
		TRACE(5, "Unable to parse Routing config : rc=%d at line %d\n", rc, parseobj.line);


	} else {

		 int entloc = 1;
		 ism_proxy_route_json(&parseobj, entloc, routeConfig);
	}

	ism_common_free(ism_memory_proxy_routing,parseobj.source);

	return 0;
}

int ism_route_config_startConnections(ism_routing_config_t * routeconfig)
{
	ismHashMapEntry ** array;
	int i=0;
	ism_routing_route_t * route;
	if(routeconfig!=NULL && routeconfig->inited){
		pthread_rwlock_rdlock(&routeconfig->lock);
		if(g_msgRoutingNeedStart){
			array = ism_common_getHashMapEntriesArray(routeconfig->routemap);
			while(array[i] != ((void*)-1)){
				route = (ism_routing_route_t *)array[i]->value;
				if(route->route_type == ROUTE_TYPE_KAFKA){
					ism_kafka_authenticator_t  authenticator=NULL;
					if(route->authenticator!=NULL && !strcmp(route->authenticator, "SASL_PLAIN")){
						authenticator = ism_kafka_metadata_conn_authenticator_sasl_plain;
					}
					int krc = ism_kafka_createTopicRecord((const char *)route->hosts, (const char *)route->kafka_topic, authenticator, (const char *)route->kafka_auth_id, (const char *)route->kafka_auth_pw,
							route->useTLS, route->tlsMethod, route->tlsCiphers, route->kafka_batch_time_millis, route->kafka_batch_size, route->kafka_batch_size_bytes, route->kafka_recovery_batch_size, route->kafka_produce_ack,
							route->kafka_dist_method, route->kafka_shutdown_onerror_time
							);
					if(krc){
						TRACE(5, "Failed to create Kafka Topic Record. topic=%s\n", route->kafka_topic);
					}else{
						TRACE(5, "Kafka Topic Record is created. topic=%s\n", route->kafka_topic);
					}
				}
				i++;
			}
			ism_common_freeHashMapEntriesArray(array);

		}
		pthread_rwlock_unlock(&routeconfig->lock);
	}
	return 0;
}


/**
 * Start the Routing Process
 */
int ism_route_routing_start(void)
{
		//Get if Routing is enabled
	g_msgRoutingEnabled =ism_common_getIntConfig("ProxyMsgRoutingEnabled", 0);
	g_msgRoutingSysPropsEnabled =ism_common_getIntConfig("ProxyMsgRoutingSysPropsEnabled", 1);
	g_msgRoutingDefaultSendMQTT =ism_common_getIntConfig("ProxyMsgRoutingDefaultSendMQTT", 1);
	g_messageRoutingDefault =ism_common_getIntConfig("ProxyMessageRoutingDefault", 0);
	TRACE(5, "Proxy Routing Start. enabled=%d messageRouting=%d\n", g_msgRoutingEnabled, g_messageRoutingDefault);
	ism_route_config_startConnections(g_routeconfig);
	return 0;
}

static int constructKafkaEventKey(ism_transport_t * transport, concat_alloc_t * buf,  const char * intopic, int topiclen)
{
	const char * devtype;
	const char * devid;
	const char * evnt;
	const char * fmt;
	int rc = 0;
	char * topic = alloca(topiclen+1);
	memcpy(topic, intopic, topiclen);
	topic[topiclen] = 0;

	rc = ism_proxy_parseMQTTTopic(topic, &devtype, &devid, &evnt, &fmt);

	if (rc==0) {
		ism_kafka_makeEventKey(buf, transport->org,  devtype, devid, evnt, fmt, NULL);
	}

	return rc;
}


static int messageRoutingInternal(ism_transport_t * transport, const char * buf, int buf_len, int kind,
        int *sendMQTT, ism_routing_config_t * routeConfig)
{
	int i=0;
	ismHashMapEntry ** array;
	ism_routing_rule_t * routeRule;
	int sent=0;
	int rulesMatched = 0;
	mqtt_pmsg_t pmsg = {buf, buf_len, kind};
	if (kind != 0x3F){
		//Regular PUBLISH
		ism_mqtt_getPublishPayload(transport, &pmsg);
	}else{
		//PUBLISHX kind
		pmsg.cmd = *buf;
		pmsg.buf = buf;
		pmsg.buflen = buf_len;
		ism_mqtt_getPublishXPayload(transport, &pmsg);
	}

	/* Null terminate the topic */
	char * topic = alloca(pmsg.topic_len+1);
	memcpy(topic, pmsg.topic, pmsg.topic_len);
	topic[pmsg.topic_len] = 0;
	pmsg.topic = topic;

	uint8_t qos =0;
	if (kind != 0x3F){
		//Regular PUBLISH
		qos = (uint8_t)((kind >> 1) & 3);
	}else{
		//PUBLISHX
		qos =  (uint8_t)((*buf >> 1) & 3);
	}

	TRACE(7, "ism_route_routeMessage: topic=%s qos=%d\n", topic, qos);

	if(routeConfig && routeConfig->inited){
		pthread_rwlock_rdlock(&routeConfig->lock);
		array = ism_common_getHashMapEntriesArray(routeConfig->routerulemap);
		while(array[i] != ((void*)-1)){
			routeRule = (ism_routing_rule_t *)array[i]->value;
			ismMessageHeader_t         hdr = {0L, 0, 0, 4, 0, 0, 0, 0};
			ismMessageAreaType_t       areatype[2] = { ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
			size_t                     areasize[2] = {0, 0};
			void *                     areaptr[2] = {NULL, NULL};
			int                        selected;
			char                       xbuf [1024];
			concat_alloc_t             props = {xbuf, sizeof(xbuf)};

			hdr.Reliability = qos;
			areasize[0] = props.used;
			areaptr[0] = props.buf;

			selected = ism_common_selectMessage(&hdr, 2, areatype, areasize, areaptr, (const char*)topic, routeRule->selector, 0);
			TRACE(7, "ism_route_routeMessage: Selection topic=%s qos=%d selected=%d\n", topic, qos, selected );
			if(selected==SELECT_TRUE){
				rulesMatched++;
				//Route the Message Based on the Rule and Route
				if(routeRule->routes ){
					int numRoute = ism_common_getArrayNumElements(routeRule->routes);
					int which =0;

					for(which=0; which < numRoute; which++){
						ism_routing_route_t * route =  ism_common_getArrayElement(routeRule->routes, which+1);
						__sync_add_and_fetch(&route->stats.C2PMsgsTotalReceived, 1);
						if(route->route_type == ROUTE_TYPE_KAFKA){
							TRACE(7, "ism_route_routeMessage: Publish to Kafka topic=%s qos=%d  selected=%d\n", topic, qos, selected );
							char xxbuf[512];
							concat_alloc_t keybuf = {xxbuf, sizeof xxbuf,0};
							int rc = constructKafkaEventKey(transport, &keybuf , pmsg.topic, pmsg.topic_len);
							if (rc==0) {
							    /* TODO: check RC */
							    int keylen = keybuf.used;
							    int hdrcount=0;
							    if (pmsg.prop_len || routeRule->sysProperties || routeRule->userProperties) {
							        hdrcount = ism_kafka_makeKafkaHeaders(transport, &keybuf, &pmsg,
							                routeRule->sysProperties, routeRule->userProperties, -1);
							    }
								ism_kafka_publishEvent(transport, &pmsg, route->kafka_topic, keybuf.buf, keylen,
								        hdrcount, keybuf.buf+keylen, keybuf.used-keylen);
								sent=1;
							} else {
								TRACE(7, "ism_route_routeMessage: Failed to create the Kafka Key. topic=%s qos=%d  selected=%d \n", topic, qos, selected );
							}
							if (keybuf.inheap)
								ism_common_freeAllocBuffer(&keybuf);

						}else if(route->route_type == ROUTE_TYPE_MQTT){
							//Send to MQTT Broker
							if(route->send_mqtt && sendMQTT!=NULL){
								if(sendMQTT!=NULL)
									*sendMQTT=1;
							}else{
								if(sendMQTT!=NULL)
									*sendMQTT=0;
							}
						}

						if(sent)
						    __sync_add_and_fetch(&route->stats.C2PMsgsTotalSent, 1);
						TRACE(7, "ism_route_routeMessage: Route stats: routeName=%s msgReceived=%llu msgSent=%llu\n",
									route->name, (ULL)route->stats.C2PMsgsTotalReceived, (ULL)route->stats.C2PMsgsTotalSent );

					}
				}
			}
			i++;
		}
		pthread_rwlock_unlock(&routeConfig->lock);
		ism_common_freeHashMapEntriesArray(array);
	}


	return rulesMatched;

}

/**
 * This function routes the message for mhub based on the rules
 * @return the count of success to publish or queue for mhub
 */

static int mhubRoutingInternal(ism_transport_t * transport, const char * buf, int buf_len, int kind,
        int *sendMQTT,  ism_mhub_t * mhub) {
	int rvalue = 0;
	mqtt_pmsg_t pmsg = {buf, buf_len, kind};
	if (kind != 0x3F) {
		//Regular PUBLISH
		ism_mqtt_getPublishPayload(transport, &pmsg);
	} else {
		//PUBLISHX kind
		pmsg.cmd = *buf;
		pmsg.buf = buf;
		pmsg.buflen = buf_len;
		ism_mqtt_getPublishXPayload(transport, &pmsg);
	}

	/* Null terminate the topic */
	char * topic = alloca(pmsg.topic_len+1);
	memcpy(topic, pmsg.topic, pmsg.topic_len);
	topic[pmsg.topic_len] = 0;
	pmsg.topic = topic;

	TRACE(9, "ism_route_routeMessage: topic=%s\n", topic);

	if (mhub && mhub->enabled == 1) {
        char xxbuf[2048];
		//const char * devtype;
		//const char * devid;
		//const char * evnt;
		//const char * fmt;
		int rc = 0;
		int part;
		int j;
		uint16_t  topiclist[100];
		int count=0;
		int prc=0;
		int pubError=0;
		int pubSucceed=0;
		char * ttopic = alloca(pmsg.topic_len+1);
		memcpy(ttopic, pmsg.topic, pmsg.topic_len);
		ttopic[pmsg.topic_len] = 0;
		concat_alloc_t keybuf = {xxbuf, sizeof xxbuf};

		/* Parse the topic, we only publish events with the defined topic structure */
		rc = ism_proxy_parseMQTTTopic(ttopic, &pmsg.type, &pmsg.id, &pmsg.event, &pmsg.format);
		//rc = ism_proxy_parseMQTTTopic(ttopic, &devtype, &devid, &evnt, &fmt);
		if (rc == 0) {
		    /* Select the topics to send the message to */
		    ism_mhub_lock(mhub);
		    count = ism_mhub_selectMessages(mhub, topiclist, 100,  pmsg.type, pmsg.event, &pmsg);
		    ism_mhub_unlock(mhub);

		    if (count > 0) {
		        /* Construct the key */
		        //ism_mhub_makeKey(mhub, &keybuf, transport->org, devtype, devid, evnt, fmt);
		        if (mhub->use_keymap)
					ism_mhub_makeKeyMap(mhub, &keybuf, transport, &pmsg);
				else
					ism_mhub_makeKey(mhub, &keybuf, transport->org, pmsg.type, pmsg.id, pmsg.event, pmsg.format);
		        pmsg.key_len = keybuf.used;
                /* If there are properties, make the kafka header */
                if (pmsg.prop_len) {
                    pmsg.hdr_count = ism_kafka_makeKafkaHeaders(transport, &keybuf, &pmsg, NULL, NULL, mhub->messageVersion);
                    pmsg.headers = keybuf.buf + pmsg.key_len;
                    pmsg.hdr_len = keybuf.used - pmsg.key_len;
                }
                pmsg.key = keybuf.buf;
                part = ism_mhub_getPartition(pmsg.type, pmsg.id);

                /* For each selected topic, publish the message to that mhub topic */
                for (j=0; j<count; j++){
                    prc=ism_mhub_publishEvent(mhub, &pmsg, transport->name, topiclist[j], part);
                    if (prc > 1){
                    	pubError++;
                    } else {
                    	pubSucceed++;
                    }
                }
                if(pubError>0){
                	TRACE(7,"Failed to publish: ClientID=%s TopicCount=%d, ErrorCount=%d SuccessCount=%d\n", transport->clientID,  count, pubError,  pubSucceed);
                }
                rvalue = pubSucceed;
		    }

		} else {
			TRACE(7, "The message is not a standard WIoTP event: client=%s topic=%s\n", transport->name, topic);
		}

		if (keybuf.inheap)
			ism_common_freeAllocBuffer(&keybuf);
	}
	return rvalue;
}


/*
 * Iterate thru Mhub list and send the messages into each MHUB
 */
static int routeToMHUBs(ism_transport_t * transport, mqtt_pmsg_t * pmsg, ism_mhub_t * mhub) {
    int count = 0;
    int pubSucceed = 0;
    int part;
    int j;
    int prc;
    uint16_t  topiclist[100];
    char xbuf [2048];
    concat_alloc_t keybuf = {xbuf, sizeof xbuf};

    if (!mhub || !transport || !pmsg)
        return 0;

    while (mhub) {
        if (!mhub->disabled) {
            ism_mhub_lock(mhub);
            count = ism_mhub_selectMessages(mhub, topiclist, 100, pmsg->type, pmsg->event, pmsg);
            ism_mhub_unlock(mhub);
            if (count > 0) {
                /* If ACK wait is needed construct it */
                if (pmsg->waitValue) {
                    pmsg->waitID = ism_transport_getWaitID(transport);
                }

                /* Construct the key */
                if (mhub->use_keymap)
                    ism_mhub_makeKeyMap(mhub, &keybuf, transport, pmsg);
                else
                    ism_mhub_makeKey(mhub, &keybuf, transport->org, pmsg->type, pmsg->id, pmsg->event, pmsg->format);
                pmsg->key_len = keybuf.used;
                /* If there are properties, make the kafka header */
                if (pmsg->prop_len) {
                    pmsg->hdr_count = ism_kafka_makeKafkaHeaders(transport, &keybuf, pmsg, NULL, NULL, mhub->messageVersion);
                    pmsg->headers = keybuf.buf + pmsg->key_len;
                    pmsg->hdr_len = keybuf.used - pmsg->key_len;
                }
                pmsg->key = keybuf.buf;
                if (mhub->partitionMap)
                    part = ism_mhub_getPartitionMap(mhub, &keybuf, transport, pmsg);
                else
                    part = ism_mhub_getPartition(pmsg->type, pmsg->id);

                /* For each selected topic, publish the message to that mhub topic */
                for (j=0; j<count; j++) {
                    prc = ism_mhub_publishEvent(mhub, pmsg, transport->name, topiclist[j], part);
                    pmsg->waitValue = 0;    /* Wait for just the first mhub we publish to */
                    if (prc <= 1) {
                        pubSucceed++;
                    }
                }
            }
        }
        mhub = mhub->next;
    }

    if (keybuf.inheap)
    			ism_common_freeAllocBuffer(&keybuf);

    return pubSucceed;
}

/*
 *
 */
int ism_route_mhubMessage(ism_transport_t * transport, mqtt_pmsg_t * pmsg) {
    int pubSucceed = 0;
    ism_mhub_t * mhub;

    if (!transport->tenant || !transport->tenant->mhublist)
        return 0;

    mhub = transport->tenant->mhublist;
    pubSucceed += routeToMHUBs(transport, pmsg, mhub);
#ifndef HAS_BRIDGE
    //Send Msgs that define for Internal Tenant proxy
    mhub = eventTenant.mhublist;
    pubSucceed += routeToMHUBs(transport, pmsg, mhub);
#endif
    return pubSucceed;
}

/*
 * Route Messages
 *
 */
int ism_route_routeMessage(ism_transport_t * transport, const char * buf, int buf_len, int kind, int *sendMQTT) {
	ism_routing_config_t * routeConfig;
	int rulesMatched = 0;

	if(g_msgRoutingEnabled){
		//If Message Routing is enabled, check for Org or Global Routing Config
		TRACE(9,"Message\n.");
		return 1;

		//Get the RouteConfig from Org first;
		routeConfig = transport->tenant->routeConfig;
		if(routeConfig && routeConfig->inited){
			rulesMatched = messageRoutingInternal(transport, buf, buf_len, kind, sendMQTT,routeConfig);
		}

		//If no rules matched from Org, use Global Rules
		if(rulesMatched==0){
			rulesMatched = messageRoutingInternal(transport, buf, buf_len, kind, sendMQTT,g_routeconfig);

		}
	}
	//If no rules match from Org and Global Rules. Use default configuration for Message routing
	if(rulesMatched==0 && g_internalKafkaRoute!=NULL && g_useKafkaIMMessaging && !g_useMHUBKafkaConnection){
		//Use Internal Route
		uint8_t qos =0;
		if (kind != 0x3F){
			//Regular PUBLISH
			qos = (uint8_t)((kind >> 1) & 3);
		}else{
			//PUBLISHX
			qos =  (uint8_t)((*buf >> 1) & 3);
		}

		if (ism_proxy_routeToKafka(transport->tenant, qos)) {
			int sent = 0;

			ismMessageHeader_t         hdr = {0L, 0, 0, 4, 0, 0, 0, 0};
			ismMessageAreaType_t       areatype[2] = { ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
			size_t                     areasize[2] = {0, 0};
			void *                     areaptr[2] = {NULL, NULL};
			int                        selected;
			char                       xbuf [1024];
			concat_alloc_t             props = {xbuf, sizeof(xbuf)};

			hdr.Reliability = qos;
			areasize[0] = props.used;
			areaptr[0] = props.buf;
			mqtt_pmsg_t pmsg = {buf, buf_len, kind};
			if (kind != 0x3F){
				//Regular PUBLISH
				ism_mqtt_getPublishPayload(transport, &pmsg);
			}else{
				//PUBLISHX kind
				pmsg.cmd = *buf;
				pmsg.buf = buf;
				pmsg.buflen = buf_len;
				ism_mqtt_getPublishXPayload(transport, &pmsg);
			}

			/* Null terminate the topic name */
			char * topic = alloca(pmsg.topic_len + 1);
			memcpy(topic, pmsg.topic, pmsg.topic_len);
			topic[pmsg.topic_len] = 0;
			pmsg.topic = topic;
			ism_routing_route_t * route = g_internalKafkaRoute;

			if(g_IMMessagingSelector != NULL){
				selected = ism_common_selectMessage(&hdr, 2, areatype, areasize, areaptr, (const char*)topic, g_IMMessagingSelector, 0);
			}else{
				selected = SELECT_TRUE;
			}

			if(selected==SELECT_TRUE){

				__sync_add_and_fetch(&route->stats.C2PMsgsTotalReceived, 1);
				__sync_add_and_fetch(&route->stats.C2PBytesTotalReceived, buf_len);

				TRACE(7, "ism_route_routeMessage: Publish to internal Kafka topic=%s qos=%d\n", topic, qos );
				char xxbuf[512];
				concat_alloc_t keybuf = {xxbuf, sizeof xxbuf};
				int rc = constructKafkaEventKey(transport, &keybuf , topic, pmsg.topic_len);
				if (rc==0) {
				    int keylen = keybuf.used;
				    int hdrcount=0;
				    if (pmsg.prop_len) {
				        hdrcount = ism_kafka_makeKafkaHeaders(transport, &keybuf, &pmsg, NULL, NULL, -1);
				    }
					ism_kafka_publishEvent(transport, &pmsg, route->kafka_topic, keybuf.buf, keylen,
					        hdrcount, keybuf.buf+keylen, keybuf.used-keylen);
					sent = 1;
				} else {
					TRACE(7, "ism_route_routeMessage: Failed to create the Kafka Key. topic=%s qos=%d\n", topic, qos );
				}
				if (keybuf.inheap)
					ism_common_freeAllocBuffer(&keybuf);

				if (sent) {
					__sync_add_and_fetch(&route->stats.C2PMsgsTotalSent, 1);
					__sync_add_and_fetch(&route->stats.C2PBytesTotalSent, buf_len);
				}
			}
			TRACE(7, "ism_route_routeMessage: Internal Route stats: routeName=%s msgReceived=%llu msgSent=%llu\n",
								route->name, (ULL)route->stats.C2PMsgsTotalReceived, (ULL)route->stats.C2PMsgsTotalSent );

		}
	}

	//MessageHub Routing
	if (g_mhubEnabled) {
		ism_tenant_t * tenant = transport->tenant;
		int pubCount=0;
		if (tenant->mhublist) {
			ism_mhub_t * mhub = tenant->mhublist;
			while (mhub) {
				if(!mhub->disabled){
					//Publish The message
					pubCount += mhubRoutingInternal(transport, buf, buf_len, kind, sendMQTT, mhub);
				}
				mhub = mhub->next;
			}
		}
#ifndef HAS_BRIDGE
		//Route to eventTenant which is the internal Kafka if it is enabled
		ism_tenant_t* evtTenant = &eventTenant;
		if (g_useMHUBKafkaConnection && evtTenant->mhublist) {
			ism_mhub_t * mhub = evtTenant->mhublist;
			while (mhub) {
				if(!mhub->disabled){
					//Publish The message
					pubCount += mhubRoutingInternal(transport, buf, buf_len, kind, sendMQTT, mhub);
				}
				mhub = mhub->next;
			}
		}
#endif
		if(pubCount==0){
			//If not selected by any mhub and rules, consider filtered.
			pthread_spin_lock(&g_mhubStatLock);
			mhubMessagingStats.kafkaC2PMsgsTotalFiltered++;
			pthread_spin_unlock(&g_mhubStatLock);
		}
	}

	return 0;

}

int ism_route_setMsgRoutingEnabled(int value)
{
	if(g_msgRoutingEnabled!=value){
		g_msgRoutingEnabled = value;
		TRACE(5,"ProxyMsgRoutingEnabled configuration is updated. new value=%d\n", value);
		if(g_msgRoutingEnabled){
			ism_route_routing_start();
		}
	}
	return 0;
}

int ism_route_setMsgRoutingSysPropsEnabled(int value)
{
	if(g_msgRoutingSysPropsEnabled!=value){
		g_msgRoutingSysPropsEnabled = value;
		TRACE(5,"g_msgRoutingSysPropsEnabled configuration is updated. new value=%d\n", value);
	}
	return 0;
}
