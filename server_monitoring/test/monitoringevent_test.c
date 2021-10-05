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



/**
 * This test valid the the JSON format of String
 */
void test_ism_monitoring_createEngineMessage(void)
{
	printf("Inside test_ism_monitoring_createEngineMessage\n");
	int rc=0;
	
	char*  topicStr = "topic/subtopic";
	char * messageData ="This is Message Data 123456";
	
	
	ismEngine_MessageHandle_t message=NULL;
		
	rc = ism_monitoring_createEngineMessage( 	&message, 
												(const char *)topicStr,
                                				(void *)&messageData, 
                                				strlen(messageData));
	
	
	CU_ASSERT(rc==0);
	CU_ASSERT(message!=NULL);
    
}

void testMonitoringEvent(void) 
{
	printf("Inside Monitoring Event Tests\n");
	ism_monitoring_init();

	test_ism_monitoring_createEngineMessage();
	
	
	ism_monitoring_term();
	
	return;
}
