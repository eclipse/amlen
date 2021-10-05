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

DEFINE_CONNECTION_MONITORING_OBJ(Xism_connect_mon_t);


void testSerialziation_basic(void) {
    
	concat_alloc_t buf;
	char bufn[1024];
	ismJsonSerializerData iSerUserData;
    ismSerializerData iSerData;
    
  	ism_connect_mon_t connect_mon_in;
  	
  	char * output;
  	
  	memset(&iSerUserData, 0, sizeof(ismJsonSerializerData));
  	
  	memset(&connect_mon_in,0, sizeof(ism_connect_mon_t)); 
  	
  	
  	 /*
     * init return buffer
     */
    buf.buf     = bufn;
    buf.len     = sizeof(bufn);
    buf.used    = 0;
    buf.inheap  = 0;
    
  	
  	iSerUserData.outbuf = &buf;
  	iSerData.serializer_userdata = &iSerUserData;
  	
  	connect_mon_in.name="Conn1";
  	connect_mon_in.protocol="JMS";
  	connect_mon_in.userid="userid";
  	connect_mon_in.index=1;
  	connect_mon_in.port=16101;
  	connect_mon_in.read_bytes=10000;
  	connect_mon_in.write_bytes=10000;
  	connect_mon_in.isSuspended=1;
  	connect_mon_in.duration=1;
  	
  	
  	output = ism_common_serializeMonJson(Xism_connect_mon_t,(void *)&connect_mon_in,buf.buf, 2500,&iSerData );
  	printf("\nOutput: %s\n", output);
	CU_ASSERT(output!=NULL);
	CU_ASSERT(strlen(output)>100);
	  	
  	
  	
}

void testSerialziation_max(void) {
    
	concat_alloc_t buf;
	char bufn[1024*100];
	ismJsonSerializerData iSerUserData;
    ismSerializerData iSerData;
    int count=0;
    char conname[63];
  	ism_connect_mon_t connect_mon_in[100];
  	
  	char * output;
  	
  	memset(&iSerUserData, 0, sizeof(ismJsonSerializerData));
  	
  	 /*
     * init return buffer
     */
    buf.buf     = bufn;
    buf.len     = sizeof(bufn);
    buf.used    = 0;
    buf.inheap  = 0;
    
  	
  	iSerUserData.outbuf = &buf;
  	iSerData.serializer_userdata = &iSerUserData;
  	
  	
  	for(count=0; count< 100; count++){
  		memset(&connect_mon_in[count],0, sizeof(ism_connect_mon_t)); 
  	
  		sprintf((char *)&conname, "Conn.%d", count);
		connect_mon_in[count].name=(char *)&conname;
	  	connect_mon_in[count].protocol="JMS";
	  	connect_mon_in[count].userid="userid";
	  	connect_mon_in[count].index=1;
	  	connect_mon_in[count].port=16101;
	  	connect_mon_in[count].read_bytes=10000;
	  	connect_mon_in[count].write_bytes=10000;
	  	connect_mon_in[count].isSuspended=1;
	  	connect_mon_in[count].duration=1;
  		output = ism_common_serializeMonJson(Xism_connect_mon_t,(void *)&connect_mon_in[count],buf.buf, 2500,&iSerData );
  		//printf("Output: %s\n", output);
		CU_ASSERT(output!=NULL);
		CU_ASSERT(strlen(output)>100);
  	}
  	
  	
  	
	  	
  	
  	
}

void testSerialziation(void) {

	
	testSerialziation_basic();
	testSerialziation_max();
	return;
}
