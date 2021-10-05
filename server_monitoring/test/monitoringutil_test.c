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

/*
 * File: config_test.c
 * Component: server
 * SubComponent: server_admin
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <monitoring.h>
#include <monserialization.h>
#include <monitoringutil.h>
#include <ismmonitoringobjs.h>




void test_ism_monitoring_getExtMonTopic(void) {
	  	char buf[1024]={0};
	  	
		ism_monitoring_getExtMonTopic(ismMonObjectType_Memory, buf);
	  	CU_ASSERT(!strcmp(buf, "$SYS/ResourceStatistics/Memory"));
	
		ism_monitoring_getExtMonTopic(ismMonObjectType_Topic, buf);
	  	CU_ASSERT(!strcmp(buf, "$SYS/ResourceStatistics/Topic"));
	  	
	  	ism_monitoring_getExtMonTopic(ismMonObjectType_Store, buf);
	  	CU_ASSERT(!strcmp(buf, "$SYS/ResourceStatistics/Store"));
	  	
	  	ism_monitoring_getExtMonTopic(ismMonObjectType_Endpoint, buf);
	  	CU_ASSERT(!strcmp(buf, "$SYS/ResourceStatistics/Endpoint"));
	  	
	  	ism_monitoring_getExtMonTopic(ismMonObjectType_Connection, buf);
	  	CU_ASSERT(!strcmp(buf, "$SYS/ResourceStatistics/Connection"));
	  	
	  	ism_monitoring_getExtMonTopic(ismMonObjectType_Queue, buf);
	  	CU_ASSERT(!strcmp(buf, "$SYS/ResourceStatistics/Queue"));
	  	
	  	ism_monitoring_getExtMonTopic(ismMonObjectType_DestinationMappingRule, buf);
	  	CU_ASSERT(!strcmp(buf, "$SYS/ResourceStatistics/DestinationMappingRule"));
	  	
	  	ism_monitoring_getExtMonTopic(1000, buf);
	  	CU_ASSERT(!strcmp(buf, "$SYS/ResourceStatistics/Unknown"));

}


void test_ism_monitoring_getMonObjectType(void)
{
	char * result = NULL;
	
	result = ism_monitoring_getMonObjectType(ismMonObjectType_Memory);
	CU_ASSERT(!strcmp(result, "Memory"));
	
	result = ism_monitoring_getMonObjectType(ismMonObjectType_Store);
	CU_ASSERT(!strcmp(result, "Store"));
	
	result = ism_monitoring_getMonObjectType(ismMonObjectType_Topic);
	CU_ASSERT(!strcmp(result, "Topic"));
	
	result = ism_monitoring_getMonObjectType(ismMonObjectType_Queue);
	CU_ASSERT(!strcmp(result, "Queue"));
	
	result = ism_monitoring_getMonObjectType(ismMonObjectType_Connection);
	CU_ASSERT(!strcmp(result, "Connection"));
	
	result = ism_monitoring_getMonObjectType(ismMonObjectType_DestinationMappingRule);
	CU_ASSERT(!strcmp(result, "DestinationMappingRule"));
	
	result = ism_monitoring_getMonObjectType(100);
	CU_ASSERT(!strcmp(result, "Unknown"));
}

void test_ism_monitoring_getNodeName(void)
{
	char nodeName[1024];
	char inodeName[1024];
	
	ism_monitoring_getNodeName((char *)&nodeName, sizeof(nodeName), 0);
	gethostname(inodeName, sizeof(inodeName));
	
	CU_ASSERT(!strcmp(inodeName, nodeName));
	
	ism_monitoring_getNodeName((char *)&nodeName, sizeof(nodeName), 1);
	CU_ASSERT(!strcmp(inodeName, nodeName));
	
	
}


/**
 * This test valid the the JSON format of String
 */
void test_ism_monitoring_getMsgExternalMonPrefix(void)
{
	char buf[1024];
	concat_alloc_t outbuf;
	outbuf.buf = &buf;
	outbuf.len = sizeof(buf);
	char rresult[1024];
	
	uint64_t currentTime = ism_common_currentTimeNanos();
	
	ism_common_allocBufferCopyLen(&outbuf, "{", 1);
	ism_monitoring_getMsgExternalMonPrefix(ismMonObjectType_Memory,
											currentTime,
											NULL,
											&outbuf);
	ism_common_allocBufferCopyLen(&outbuf, "}", 1);
	
	char * version = ism_common_getVersion();
	
	ism_ts_t *ts = ism_common_openTimestamp(ISM_TZF_UTC);
	char *timeValue = NULL;
	char tbuffer[80];
	
	if ( ts != NULL) {
            ism_common_setTimestamp(ts, currentTime);
            ism_common_formatTimestamp(ts, tbuffer, 80, 0, ISM_TFF_ISO8601);
            timeValue = tbuffer;
            ism_common_closeTimestamp(ts);
    }
    
    char nodeName[1024];
	ism_monitoring_getNodeName((char*)&nodeName, 1024, 0);
    
    
     //{"Version":"1.0.1 Beta","NodeName":"mar145","TimeStamp":"2013-06-12T15:29:59.632Z","ObjectType":"Memory"}
     
    sprintf(&rresult, "{\"Version\":\"%s\",\"NodeName\":\"%s\",\"TimeStamp\":\"%s\",\"ObjectType\":\"%s\"}", version, nodeName,timeValue, "Memory");
	//printf("RRREsult: %s\n", rresult);
	//printf("Outbuf: %s\n", outbuf.buf);
	CU_ASSERT(!strncmp(rresult, outbuf.buf, outbuf.used));
	
	ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[20];
    parseobj.ent = ents;
    parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseobj.source = (char *) outbuf.buf;
    parseobj.src_len = outbuf.used + 1;
    int rc = ism_json_parse(&parseobj);
    CU_ASSERT(rc==0);
    
}

void testMonitoringUtil(void) 
{
	ism_monitoring_initNotification();
	
	test_ism_monitoring_getExtMonTopic();
	test_ism_monitoring_getMonObjectType();
	test_ism_monitoring_getNodeName();
	test_ism_monitoring_getMsgExternalMonPrefix();
	
	
	ism_monitoring_termNotification();
	
	return;
}
