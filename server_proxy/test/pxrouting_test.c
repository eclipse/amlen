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
#include <pxrouting.h>
void ism_proxy_nojavainit(void);

const char * route_rule_config_routeOnly=
"{\n"
"        \"RouteConfig\": {\n"
"        	\"Route\": {\n"
"                \"kafka-1\": {\n"
"                        \"RouteType\": \"kafka\",\n"
"                        \"KafkaTopic\": \"kevents\",\n"
"                        \"Hosts\": \"Hosts1\",\n"
"                        \"KafkaKey\":\"keu1\"\n"
"                }\n"
"        	}\n"
"        }\n"
"}\n"
;

const char * route_rule_config_routeRuleOnly=
"{\n"
"        \"RouteConfig\": {\n"
"        	\"RouteRule\":{\n"
"               \"Rule1\":{\n"
"                       \"Selector\":\"QoS>0 and Org='testorg1' and Event = 'abctype' and Type like 'type%' and ID='id1'\",\n"
"                 		 \"Processor\":\"ow-1\",\n"
"                        \"Route\":\"kafka-1\"\n"
"                }\n"
"        	}\n"
"        }\n"
"}\n"
;


const char * route_rule_config_combine1=
"{\n"
"        \"RouteConfig\": {\n"
"        	\"Route\": {\n"
"                \"kafka-1\": {\n"
"                        \"RouteType\": \"kafka\",\n"
"                        \"KafkaTopic\": \"kevents\",\n"
"                        \"KafkaKey\":\"keu1\"\n"
"                }\n"
"        	},\n"
"        	\"RouteRule\":{\n"
"               \"Rule1\":{\n"
"                       \"Selector\":\"Org='testorg1' and Event = 'abctype' and Type like 'type%' and ID = 'id1'\",\n"
"                        \"Routes\":\"kafka-1\",\n"
"                        \"Processor\":\"ow-1\",\n"
"                        \"UserProperties\":{\"owaction\":\"testowaction\", \"testpropkey2\":\"testspropvalue2\", \"testpropkey3\":\"testspropvalue3\"}\n"
"                }\n"
"       	 	}\n"
"        }\n"
"}\n"
;

void pxrouting_test_parseRouteAll(void){
	ism_routing_config_t * routeConfig;
	routeConfig = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_routing,203),1, sizeof(ism_routing_config_t));
	ism_route_config_init(routeConfig);
	ism_proxy_parseRouteConfig(route_rule_config_combine1, routeConfig);
	CU_ASSERT( routeConfig->inited == 1);
	CU_ASSERT( routeConfig->routemap != NULL);

	int routeRuleMapNum =ism_common_getHashMapNumElements(routeConfig->routerulemap);
	CU_ASSERT( routeRuleMapNum == 1);
	int routeMapNum =ism_common_getHashMapNumElements(routeConfig->routemap);
	CU_ASSERT( routeMapNum == 1);


	ismHashMapEntry ** array = ism_common_getHashMapEntriesArray(routeConfig->routemap);
	int i=0;
	ism_routing_route_t * route;
	//Only 1 Route
	if(array[i] != ((void*)-1)){
		route = (ism_routing_route_t *)array[i]->value;
		CU_ASSERT( route->route_type == ROUTE_TYPE_KAFKA);
		//printf("RouteName: %s\n", route->name);
		CU_ASSERT( strcmp(route->name,"kafka-1") == 0 );
		//printf("KafkaTopic: %s\n", route->kafka_topic);
		CU_ASSERT( strcmp(route->kafka_topic,"kevents") ==0);
	}

	ism_common_freeHashMapEntriesArray(array);

	array = ism_common_getHashMapEntriesArray(routeConfig->routerulemap);
	i=0;
	ism_routing_rule_t * routeRule;
	//Only 1 Route
	if(array[i] != ((void*)-1)){
		routeRule = (ism_routing_rule_t *)array[i]->value;
		CU_ASSERT(routeRule->userProperties!=NULL);
		if(routeRule->userProperties!=NULL){
			//printf("UserProperties is NOT NULL\n");
			concat_alloc_t * userProperties = routeRule->userProperties;
			int proplen = 0;
			char * prop = NULL;
			char * propbuf = userProperties->buf; //Skip UUID
			int obuflen=0;
			int props_count=0;
			while(obuflen<userProperties->used){
				//Check Org
				proplen = (uint16_t)BIGINT16(propbuf);
				propbuf+=2;
				obuflen+=2;
				prop = alloca(proplen+1);
				memcpy(prop, propbuf , proplen);
				prop[proplen]=0;
				propbuf+=proplen;
				obuflen+=proplen;

				if (g_verbose)
				    printf("PropKey: len=%d value=%s\n", proplen, prop);

				proplen = (uint16_t)BIGINT16(propbuf);
				propbuf+=2;
				obuflen+=2;
				prop = alloca(proplen+1);
				memcpy(prop, propbuf , proplen);
				prop[proplen]=0;
				propbuf+=proplen;
				obuflen+=proplen;
				if (g_verbose)
				    printf("PropValue: len=%d value=%s\n", proplen, prop);
				//printf("Prop: len=%d buflen=%d\n", obuflen, userProperties->used);
				props_count++;
			}
			CU_ASSERT( props_count ==3);

		}else{
		    if (g_verbose)
			    printf("UserProperties is  NULL\n");
		}


		if(routeRule->sysProperties!=NULL){
			int sysPropCount = (int)routeRule->sysProperties->buf[0];
			if (g_verbose)
			    printf("SystemProperties Count: %d\n",(int)routeRule->sysProperties->buf[0] );
			CU_ASSERT( sysPropCount ==1);
			concat_alloc_t * sysProperties = routeRule->sysProperties;
			char * propbuf = sysProperties->buf; //Skip UUID
			propbuf+=1; //Skip first byte of properties count
			ism_field_t field;
			concat_alloc_t sysPropertiesTemp = {sysProperties->buf+1,sysProperties->len-1, sysProperties->used};
			ism_findPropertyName(&sysPropertiesTemp, "Processor", &field);
			if (g_verbose)
			    printf("SysProcessor: %s\n", field.val.s);
			CU_ASSERT( !strcmp(field.val.s, "ow-1"));

		}else{
		    if (g_verbose)
			    printf("SystemProperties Count is zero\n" );
		}

	}

	ism_common_freeHashMapEntriesArray(array);

	ism_route_config_destroy(routeConfig);

}



void pxrouting_test_parseRouteRuleOnly(void){
	ism_routing_config_t * routeConfig;
	char                       dbuf [4096];
	int i=0;
	ismHashMapEntry ** array;
	ism_routing_rule_t * routeRule;

	const char * topic ="iot-2/testorg1/type/type1/id/id1/evt/abctype/fmt/json";

	routeConfig = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_routing,200),1, sizeof(ism_routing_config_t));
	ism_route_config_init(routeConfig);
	ism_proxy_parseRouteConfig(route_rule_config_routeRuleOnly, routeConfig);
	CU_ASSERT( routeConfig->inited == 1);
	CU_ASSERT( routeConfig->routemap != NULL);
	int routeRuleMapNum =ism_common_getHashMapNumElements(routeConfig->routerulemap);
	CU_ASSERT( routeRuleMapNum == 1);

	array = ism_common_getHashMapEntriesArray(routeConfig->routerulemap);
	while(array[i] != ((void*)-1)){
		routeRule = (ism_routing_rule_t *)array[i]->value;
		ism_common_dumpSelectRule(routeRule->selector, dbuf, sizeof dbuf);
		if (g_verbose)
		    printf("\nrule 0 = %s\n", dbuf);
		uint8_t qos=1;
		ismMessageHeader_t         hdr = {0L, 0, 0, 4, 0, 0, 0, 0};
		hdr.Reliability = qos;
		if (g_verbose)
		    printf("Reliability: %d\n", hdr.Reliability);
		ismMessageAreaType_t       areatype[2] = { ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
		size_t                     areasize[2] = {0, 0};
		void *                    areaptr[2] = {NULL, NULL};
		int                        selected;
		char                       xbuf [1024];
		concat_alloc_t             props = {xbuf, sizeof(xbuf)};

		areasize[0] = props.used;
		areaptr[0] = props.buf;

		int cindex=0;
		uint64_t elapsed=0;
		uint64_t total=0;
		int totalRun=1;
		for (cindex=0; cindex < totalRun; cindex++){
			ism_time_t beginTime = ism_common_currentTimeNanos();
			selected = ism_common_selectMessage(&hdr, 2, areatype, areasize, areaptr, topic, routeRule->selector, 0);
			ism_time_t endTime = ism_common_currentTimeNanos();
			elapsed = (endTime-beginTime);
			total+=elapsed;
			//printf("\nSelected: %d. Elapsed=%llu\n", selected, (ULL)elapsed);
		}
		uint64_t average = total / totalRun;
		if (g_verbose)
		    printf("\nSelection TRUE: totalCalls=%d average=%llu (ns)\n", totalRun,  (ULL)average);

		CU_ASSERT(selected == SELECT_TRUE);


		i++;
	}
	ism_common_freeHashMapEntriesArray(array);


	ism_route_config_destroy(routeConfig);

}

void pxrouting_test_parseRouteRuleOnlyFalseSelection(void){
	ism_routing_config_t * routeConfig;
	char                       dbuf [4096];
	int i=0;
	ismHashMapEntry ** array;
	ism_routing_rule_t * routeRule;

	const char * topic ="iot-2/testorg1/type/type1/id/id1/evt/abctypeFALSE/fmt/json";

	routeConfig = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_routing,202),1, sizeof(ism_routing_config_t));
	ism_route_config_init(routeConfig);
	ism_proxy_parseRouteConfig(route_rule_config_routeRuleOnly, routeConfig);
	CU_ASSERT( routeConfig->inited == 1);
	CU_ASSERT( routeConfig->routemap != NULL);
	int routeRuleMapNum =ism_common_getHashMapNumElements(routeConfig->routerulemap);
	CU_ASSERT( routeRuleMapNum == 1);

	array = ism_common_getHashMapEntriesArray(routeConfig->routerulemap);
	while(array[i] != ((void*)-1)){
		routeRule = (ism_routing_rule_t *)array[i]->value;
		ism_common_dumpSelectRule(routeRule->selector, dbuf, sizeof dbuf);
		if (g_verbose)
		    printf("\nrule 0 = %s\n", dbuf);
		uint8_t qos=1;
		ismMessageHeader_t         hdr = {0L, 0, 0, 4, 0, 0, 0, 0};
		hdr.Reliability = qos;
		if (g_verbose)
		    printf("Reliability: %d\n", hdr.Reliability);
		ismMessageAreaType_t       areatype[2] = { ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
		size_t                     areasize[2] = {0, 0};
		void *                    areaptr[2] = {NULL, NULL};
		int                        selected;
		char                       xbuf [1024];
		concat_alloc_t             props = {xbuf, sizeof(xbuf)};

		areasize[0] = props.used;
		areaptr[0] = props.buf;

		int cindex=0;
		uint64_t elapsed=0;
		uint64_t total=0;
		int totalRun=1;
		for (cindex=0; cindex < totalRun; cindex++){
			ism_time_t beginTime = ism_common_currentTimeNanos();
			selected = ism_common_selectMessage(&hdr, 2, areatype, areasize, areaptr, topic, routeRule->selector, 0);
			ism_time_t endTime = ism_common_currentTimeNanos();
			elapsed = (endTime-beginTime);
			total+=elapsed;
			//printf("\nSelected: %d. Elapsed=%llu\n", selected, (ULL)elapsed);
		}
		uint64_t average = total / totalRun;
		if (g_verbose)
		    printf("\nSelection FALSE: totalCalls=%d average=%llu (ns)\n", totalRun,  (ULL)average);

		CU_ASSERT(selected == SELECT_FALSE);


		i++;
	}
	ism_common_freeHashMapEntriesArray(array);


	ism_route_config_destroy(routeConfig);

}

void pxrouting_test_parseRouteOnly(void){
	ism_routing_config_t * routeConfig;
	routeConfig = ism_common_calloc(ISM_MEM_PROBE(ism_memory_proxy_routing,201),1, sizeof(ism_routing_config_t));
	ism_route_config_init(routeConfig);
	ism_proxy_parseRouteConfig(route_rule_config_routeOnly, routeConfig);
	CU_ASSERT( routeConfig->inited == 1);
	CU_ASSERT( routeConfig->routemap != NULL);
	int routeMapNum =ism_common_getHashMapNumElements(routeConfig->routemap);
	CU_ASSERT( routeMapNum == 1);
	ism_route_config_destroy(routeConfig);

}

/*
 * Run the MQTTv5 tests
 */
void pxrouting_test(void) {
	ism_proxy_nojavainit();

    ism_common_setTraceLevel(2);
    //pxrouting_test_parseRouteOnly();
    pxrouting_test_parseRouteRuleOnly();
    pxrouting_test_parseRouteRuleOnlyFalseSelection();
    pxrouting_test_parseRouteAll();


}

