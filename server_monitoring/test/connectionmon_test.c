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
#include <connectionMonData.h>

extern ism_connect_mon_t * connTimeBestArray[50];
extern int connTimeBestArray_size;
extern ism_connect_mon_t * connTimeWorstArray[50];
extern int connTimeWorstArray_size;
extern ism_connect_mon_t * connTPutBytesBestArray[50];
extern int connTPutBytesBestArray_size;
extern ism_connect_mon_t * connTPutBytesWorstArray[50];
extern int connTPutBytesWorstArray_size;
extern ism_connect_mon_t * connTPutMsgBestArray[50];
extern int connTPutMsgBestArray_size;
extern ism_connect_mon_t * connTPutMsgWorstArray[50];
extern int connTPutMsgWorstArray_size;

void test_ism_monitoring_getConnectionMonData(void) {
      	
}


void test_ism_monitoring_sortComparatorConnTimeWorst(void) {
	char conname[63];
    ism_connect_mon_t * conn1[2]; 
    
    
    conn1[0] = calloc(1, sizeof(ism_connect_mon_t));
    conn1[1] = calloc(1, sizeof(ism_connect_mon_t));
    
    sprintf((char *)&conname, "Conn.%d", 1);
	conn1[0]->name=(char *)&conname;
  	conn1[0]->protocol="JMS";
  	conn1[0]->userid="userid";
  	conn1[0]->index=1;
  	conn1[0]->port=16101;
  	conn1[0]->read_bytes=10000;
  	conn1[0]->write_bytes=10000;
  	conn1[0]->isSuspended=0;
  	conn1[0]->duration=100;
  	
  	
  	sprintf((char *)&conname, "Conn.%d", 2);
	conn1[1]->name=(char *)&conname;
  	conn1[1]->protocol="JMS";
  	conn1[1]->userid="userid";
  	conn1[1]->index=1;
  	conn1[1]->port=16101;
  	conn1[1]->read_bytes=10000;
  	conn1[1]->write_bytes=10000;
  	conn1[1]->isSuspended=0;
  	conn1[1]->duration=200;
  	
  	int result = ism_monitoring_sortComparatorConnTimeWorst((const void*)&conn1[0], (const void*)&conn1[1]);
  
  	//Expect -1 because the conn1[0] goes before conn1[1]
  	CU_ASSERT(result==-1);
  	
  	conn1[0]->duration=200;
  	conn1[1]->duration=100;
  	result = ism_monitoring_sortComparatorConnTimeWorst((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 1 because the conn1[0] go after conn1[1]
  	CU_ASSERT(result==1);
  	
  	
  	conn1[0]->duration=200;
  	conn1[1]->duration=200;
  	result = ism_monitoring_sortComparatorConnTimeWorst((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 0
  	CU_ASSERT(result==0);
  	
  	free(conn1[0]);
  	free(conn1[1]);
    
    
  		
    
	
  	
}

void test_ism_monitoring_sortComparatorConnTimeBest(void) {
    
	char conname[63];
    ism_connect_mon_t * conn1[2]; 
    
    
    conn1[0] = calloc(1, sizeof(ism_connect_mon_t));
    conn1[1] = calloc(1, sizeof(ism_connect_mon_t));
    
    sprintf((char *)&conname, "Conn.%d", 1);
	conn1[0]->name=(char *)&conname;
  	conn1[0]->protocol="JMS";
  	conn1[0]->userid="userid";
  	conn1[0]->index=1;
  	conn1[0]->port=16101;
  	conn1[0]->read_bytes=10000;
  	conn1[0]->write_bytes=10000;
  	conn1[0]->isSuspended=0;
  	conn1[0]->duration=100;
  	
  	
  	sprintf((char *)&conname, "Conn.%d", 2);
	conn1[1]->name=(char *)&conname;
  	conn1[1]->protocol="JMS";
  	conn1[1]->userid="userid";
  	conn1[1]->index=1;
  	conn1[1]->port=16101;
  	conn1[1]->read_bytes=10000;
  	conn1[1]->write_bytes=10000;
  	conn1[1]->isSuspended=0;
  	conn1[1]->duration=200;
  	
  	int result = ism_monitoring_sortComparatorConnTimeBest((const void*)&conn1[0], (const void*)&conn1[1]);
  
  	//Expect 1 because the conn1[0] goes after conn1[1]
  	CU_ASSERT(result==1);
  	
  	conn1[0]->duration=200;
  	conn1[1]->duration=100;
  	result = ism_monitoring_sortComparatorConnTimeBest((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 1 because the conn1[0] go before conn1[1]
  	CU_ASSERT(result==-1);
  	
  	
  	conn1[0]->duration=200;
  	conn1[1]->duration=200;
  	result = ism_monitoring_sortComparatorConnTimeBest((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 0
  	CU_ASSERT(result==0);
  	
  	free(conn1[0]);
  	free(conn1[1]);
  	
}

void test_ism_monitoring_sortComparatorTPutMsgBest(void) {
    char conname[63];
    ism_connect_mon_t * conn1[2]; 
    
    
    conn1[0] = calloc(1, sizeof(ism_connect_mon_t));
    conn1[1] = calloc(1, sizeof(ism_connect_mon_t));
    
    sprintf((char *)&conname, "Conn.%d", 1);
	conn1[0]->name=(char *)&conname;
  	conn1[0]->protocol="JMS";
  	conn1[0]->userid="userid";
  	conn1[0]->index=1;
  	conn1[0]->port=16101;
  	conn1[0]->read_bytes=10000;
  	conn1[0]->write_bytes=10000;
  	conn1[0]->isSuspended=0;
  	conn1[0]->duration=1;
  	conn1[0]->read_msg=10;
  	conn1[0]->write_msg=10;
  	
  	
  	sprintf((char *)&conname, "Conn.%d", 2);
	conn1[1]->name=(char *)&conname;
  	conn1[1]->protocol="JMS";
  	conn1[1]->userid="userid";
  	conn1[1]->index=1;
  	conn1[1]->port=16101;
  	conn1[1]->read_bytes=10000;
  	conn1[1]->write_bytes=10000;
  	conn1[1]->isSuspended=0;
  	conn1[1]->duration=1;
  	conn1[1]->read_msg=1;
  	conn1[1]->write_msg=1;
  	
  	int result = ism_monitoring_sortComparatorTPutMsgBest((const void*)&conn1[0], (const void*)&conn1[1]);
  
  	//Expect -1 because the conn1[0] goes before conn1[1]
  	CU_ASSERT(result==-1);
  	
	conn1[0]->read_msg=1;
  	conn1[0]->write_msg=1;
  	conn1[1]->read_msg=10;
  	conn1[1]->write_msg=10;
  	result = ism_monitoring_sortComparatorTPutMsgBest((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 1 because the conn1[0] go after conn1[1]
  	CU_ASSERT(result==1);
  	
  	conn1[0]->read_msg=10;
  	conn1[0]->write_msg=10;
  	conn1[1]->read_msg=10;
  	conn1[1]->write_msg=10;
  	
  	result = ism_monitoring_sortComparatorTPutMsgBest((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 0
  	CU_ASSERT(result==0);
  	
  	free(conn1[0]);
  	free(conn1[1]);
	
  	
}
void test_ism_monitoring_sortComparatorTPutMsgWorst(void) {
    
	char conname[63];
    ism_connect_mon_t * conn1[2]; 
    
    
    conn1[0] = calloc(1, sizeof(ism_connect_mon_t));
    conn1[1] = calloc(1, sizeof(ism_connect_mon_t));
    
    sprintf((char *)&conname, "Conn.%d", 1);
	conn1[0]->name=(char *)&conname;
  	conn1[0]->protocol="JMS";
  	conn1[0]->userid="userid";
  	conn1[0]->index=1;
  	conn1[0]->port=16101;
  	conn1[0]->read_bytes=10000;
  	conn1[0]->write_bytes=10000;
  	conn1[0]->isSuspended=0;
  	conn1[0]->duration=1;
  	conn1[0]->read_msg=1;
  	conn1[0]->write_msg=1;
  	
  	
  	sprintf((char *)&conname, "Conn.%d", 2);
	conn1[1]->name=(char *)&conname;
  	conn1[1]->protocol="JMS";
  	conn1[1]->userid="userid";
  	conn1[1]->index=1;
  	conn1[1]->port=16101;
  	conn1[1]->read_bytes=10000;
  	conn1[1]->write_bytes=10000;
  	conn1[1]->isSuspended=0;
  	conn1[1]->duration=1;
  	conn1[1]->read_msg=10;
  	conn1[1]->write_msg=10;
  	
  	int result = ism_monitoring_sortComparatorTPutMsgWorst((const void*)&conn1[0], (const void*)&conn1[1]);
  
  	//Expect -1 because the conn1[0] goes before conn1[1]
  	CU_ASSERT(result==-1);
  	
	conn1[0]->read_msg=10;
  	conn1[0]->write_msg=10;
  	conn1[1]->read_msg=1;
  	conn1[1]->write_msg=1;
  	result = ism_monitoring_sortComparatorTPutMsgWorst((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 1 because the conn1[0] go after conn1[1]
  	CU_ASSERT(result==1);
  	
  	conn1[0]->read_msg=10;
  	conn1[0]->write_msg=10;
  	conn1[1]->read_msg=10;
  	conn1[1]->write_msg=10;
  	
  	result = ism_monitoring_sortComparatorTPutMsgWorst((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 0
  	CU_ASSERT(result==0);
  	
  	free(conn1[0]);
  	free(conn1[1]);
  	
}

void test_ism_monitoring_sortComparatorTPutBytesBest(void) {
    
	  char conname[63];
    ism_connect_mon_t * conn1[2]; 
    
    
    conn1[0] = calloc(1, sizeof(ism_connect_mon_t));
    conn1[1] = calloc(1, sizeof(ism_connect_mon_t));
    
    sprintf((char *)&conname, "Conn.%d", 1);
	conn1[0]->name=(char *)&conname;
  	conn1[0]->protocol="JMS";
  	conn1[0]->userid="userid";
  	conn1[0]->index=1;
  	conn1[0]->port=16101;
  	conn1[0]->read_bytes=10;
  	conn1[0]->write_bytes=10;
  	conn1[0]->isSuspended=0;
  	conn1[0]->duration=1;
  	conn1[0]->read_msg=10;
  	conn1[0]->write_msg=10;
  	
  	
  	sprintf((char *)&conname, "Conn.%d", 2);
	conn1[1]->name=(char *)&conname;
  	conn1[1]->protocol="JMS";
  	conn1[1]->userid="userid";
  	conn1[1]->index=1;
  	conn1[1]->port=16101;
  	conn1[1]->read_bytes=1;
  	conn1[1]->write_bytes=1;
  	conn1[1]->isSuspended=0;
  	conn1[1]->duration=1;
  	conn1[1]->read_msg=1;
  	conn1[1]->write_msg=1;
  	
  	int result = ism_monitoring_sortComparatorTPutBytesBest((const void*)&conn1[0], (const void*)&conn1[1]);
  
  	//Expect -1 because the conn1[0] goes before conn1[1]
  	CU_ASSERT(result==-1);
  	
	conn1[0]->read_bytes=1;
  	conn1[0]->write_bytes=1;
  	conn1[1]->read_bytes=10;
  	conn1[1]->write_bytes=10;
  	result = ism_monitoring_sortComparatorTPutBytesBest((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 1 because the conn1[0] go after conn1[1]
  	CU_ASSERT(result==1);
  	
  	conn1[0]->read_bytes=10;
  	conn1[0]->write_bytes=10;
  	conn1[1]->read_bytes=10;
  	conn1[1]->write_bytes=10;
  	
  	result = ism_monitoring_sortComparatorTPutBytesBest((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 0
  	CU_ASSERT(result==0);
  	
  	free(conn1[0]);
  	free(conn1[1]);
	
  	
}


void test_ism_monitoring_sortComparatorTPutBytesWorst(void) {
    char conname[63];
    ism_connect_mon_t * conn1[2]; 
    
    
    conn1[0] = calloc(1, sizeof(ism_connect_mon_t));
    conn1[1] = calloc(1, sizeof(ism_connect_mon_t));
    
    sprintf((char *)&conname, "Conn.%d", 1);
	conn1[0]->name=(char *)&conname;
  	conn1[0]->protocol="JMS";
  	conn1[0]->userid="userid";
  	conn1[0]->index=1;
  	conn1[0]->port=16101;
  	conn1[0]->read_bytes=1;
  	conn1[0]->write_bytes=1;
  	conn1[0]->isSuspended=0;
  	conn1[0]->duration=1;
  	conn1[0]->read_msg=1;
  	conn1[0]->write_msg=1;
  	
  	
  	sprintf((char *)&conname, "Conn.%d", 2);
	conn1[1]->name=(char *)&conname;
  	conn1[1]->protocol="JMS";
  	conn1[1]->userid="userid";
  	conn1[1]->index=1;
  	conn1[1]->port=16101;
  	conn1[1]->read_bytes=10;
  	conn1[1]->write_bytes=10;
  	conn1[1]->isSuspended=0;
  	conn1[1]->duration=1;
  	conn1[1]->read_msg=10;
  	conn1[1]->write_msg=10;
  	
  	int result = ism_monitoring_sortComparatorTPutBytesWorst((const void*)&conn1[0], (const void*)&conn1[1]);
  
  	//Expect -1 because the conn1[0] goes before conn1[1]
  	CU_ASSERT(result==-1);
  	
	conn1[0]->read_bytes=10;
  	conn1[0]->write_bytes=10;
  	conn1[1]->read_bytes=1;
  	conn1[1]->write_bytes=1;
  	result = ism_monitoring_sortComparatorTPutBytesWorst((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 1 because the conn1[0] go after conn1[1]
  	CU_ASSERT(result==1);
  	
  	conn1[0]->read_bytes=10;
  	conn1[0]->write_bytes=10;
  	conn1[1]->read_bytes=10;
  	conn1[1]->write_bytes=10;
  	
  	result = ism_monitoring_sortComparatorTPutBytesWorst((const void*)&conn1[0], (const void*)&conn1[1]);
  	
  	//Expect 0
  	CU_ASSERT(result==0);
  	
  	free(conn1[0]);
  	free(conn1[1]);
  	
	
  	
}

void test_ism_monitoring_getConnectionMonDataByEndPoint(void) {
    
    ism_connect_mon_data_t mon_data;
    char nbuf[512];
    concat_alloc_t output_buffer = { nbuf, sizeof(nbuf), 0, 0 };
	char * cacheString=NULL;
	int rc=0;  	
    
    memset(&mon_data, 0, sizeof( ism_connect_mon_data_t ));
    pthread_spin_init(&mon_data.staginglock,0);
	pthread_spin_init(&mon_data.lock,0);
    
    cacheString = "bestTimeCache";  
    strcpy(&mon_data.bestTimeCache.buf, cacheString);
    mon_data.bestTimeCache.buf_len =(int) strlen(cacheString);
  
    rc = ism_monitoring_getConnectionMonDataByEndPoint(&mon_data, &output_buffer,CONNCACHE_TYPE_BESTTIME );
    CU_ASSERT(rc==ISMRC_OK);
    CU_ASSERT(!strcmp(cacheString,output_buffer.buf));
    memset(&output_buffer, 0, sizeof(concat_alloc_t));
    
    cacheString = "worstTimeCache";  
    strcpy(&mon_data.worstTimeCache.buf, cacheString);
    mon_data.worstTimeCache.buf_len =(int) strlen(cacheString);
  
    rc = ism_monitoring_getConnectionMonDataByEndPoint(&mon_data, &output_buffer,CONNCACHE_TYPE_WORSETIME );
    CU_ASSERT(rc==ISMRC_OK);
    CU_ASSERT(!strcmp(cacheString,output_buffer.buf));
    memset(&output_buffer, 0, sizeof(concat_alloc_t));
    
    
    cacheString = "bestTputMsgCache";  
    strcpy(&mon_data.bestTputMsgCache.buf, cacheString);
    mon_data.bestTputMsgCache.buf_len =(int) strlen(cacheString);
  
    rc = ism_monitoring_getConnectionMonDataByEndPoint(&mon_data, &output_buffer,CONNCACHE_TYPE_BESTTPUTMSG );
    CU_ASSERT(rc==ISMRC_OK);
    CU_ASSERT(!strcmp(cacheString,output_buffer.buf));
    memset(&output_buffer, 0, sizeof(concat_alloc_t));
  	
  	cacheString = "worstTputMsgCache";  
    strcpy(&mon_data.worstTputMsgCache.buf, cacheString);
    mon_data.worstTputMsgCache.buf_len =(int) strlen(cacheString);
  
    rc = ism_monitoring_getConnectionMonDataByEndPoint(&mon_data, &output_buffer,CONNCACHE_TYPE_WORSTTPUTMSG );
    CU_ASSERT(rc==ISMRC_OK);
    CU_ASSERT(!strcmp(cacheString,output_buffer.buf));
    memset(&output_buffer, 0, sizeof(concat_alloc_t));
    
    cacheString = "bestTputBytesCache";  
    strcpy(&mon_data.bestTputBytesCache.buf, cacheString);
    mon_data.bestTputBytesCache.buf_len =(int) strlen(cacheString);
  
    rc = ism_monitoring_getConnectionMonDataByEndPoint(&mon_data, &output_buffer,CONNCACHE_TYPE_BESTTPUTBYTES );
    CU_ASSERT(rc==ISMRC_OK);
    CU_ASSERT(!strcmp(cacheString,output_buffer.buf));
    memset(&output_buffer, 0, sizeof(concat_alloc_t));
    
    
     cacheString = "worstTputBytesCache";  
    strcpy(&mon_data.worstTputBytesCache.buf, cacheString);
    mon_data.worstTputBytesCache.buf_len =(int) strlen(cacheString);
  
    rc = ism_monitoring_getConnectionMonDataByEndPoint(&mon_data, &output_buffer,CONNCACHE_TYPE_WORSTTPUTBYTES );
    CU_ASSERT(rc==ISMRC_OK);
    CU_ASSERT(!strcmp(cacheString,output_buffer.buf));
    memset(&output_buffer, 0, sizeof(concat_alloc_t));
  
  
  	pthread_spin_destroy(&mon_data.staginglock);
	pthread_spin_destroy(&mon_data.lock);
}

extern void processConnMonitoringDataResult(ism_connect_mon_t * resultMonArray, int result_count, ism_CONNECTION_CACHE_TYPE type);
extern void ism_monitoring_connectionMonDataInit(void);
#define MAX_CONN_SIZE 2000
#define MAX_RESULT_SIZE 50

void test_processConnMonitoringDataResult(void)
{	
	int count=0;
	char conname[63];
	ism_connect_mon_t conarray[MAX_CONN_SIZE];
	ism_connect_mon_t * monconlist;
	monconlist = &conarray;
	
	char * connFirst50ExpectResult[MAX_RESULT_SIZE];
	for(count=0; count< MAX_RESULT_SIZE; count++){
		connFirst50ExpectResult[count]=calloc(1, 20);
		sprintf(connFirst50ExpectResult[count], "Conn.%d", count+1);
	}
	
	char * connLast50ExpectResult[MAX_RESULT_SIZE];
	for(count=0; count< MAX_RESULT_SIZE; count++){
		connLast50ExpectResult[count]=calloc(1, 20);
		sprintf(connLast50ExpectResult[count], "Conn.%d", MAX_CONN_SIZE-count);
	}
	
	ism_monitoring_connectionMonDataInit();
	
	for(count=0; count< MAX_CONN_SIZE; count++){	
    	conarray[count].name=calloc(1, 20);
	    sprintf(conarray[count].name, "Conn.%d", count+1);
		conarray[count].protocol="JMS";
	  	conarray[count].userid="userid";
	  	conarray[count].client_addr="client_addr";
	  	conarray[count].listener="listener";
	  	conarray[count].index=1;
	  	conarray[count].port=16101;
	  	conarray[count].read_bytes=count+1;
	  	conarray[count].write_bytes=count+1;
	  	conarray[count].read_msg=count+1;
	  	conarray[count].write_msg=count+1;
	  	conarray[count].isSuspended=0;
	  	conarray[count].duration=MAX_CONN_SIZE - count;
	}
	processConnMonitoringDataResult(monconlist, MAX_CONN_SIZE, CONNCACHE_TYPE_BESTTIME);
	
	CU_ASSERT(connTimeBestArray_size==MAX_RESULT_SIZE);
	for(count=0; count< MAX_RESULT_SIZE; count++){	
		printf("Best Tiem: Name: %s. Duration: %d\n", connTimeBestArray[count]->name, connTimeBestArray[count]->duration);
		CU_ASSERT(!strcmp( connTimeBestArray[count]->name,connFirst50ExpectResult[count] ));
	}
	
	processConnMonitoringDataResult(monconlist, MAX_CONN_SIZE, CONNCACHE_TYPE_WORSETIME);
	
	CU_ASSERT(connTimeWorstArray_size==MAX_RESULT_SIZE);
	for(count=0; count< MAX_RESULT_SIZE; count++){	
		printf("Worst Time: Name: %s. Duration: %d\n", connTimeWorstArray[count]->name, connTimeWorstArray[count]->duration);
		CU_ASSERT(!strcmp( connTimeWorstArray[count]->name,connLast50ExpectResult[count] ));
	}
	
	processConnMonitoringDataResult(monconlist, MAX_CONN_SIZE, CONNCACHE_TYPE_BESTTPUTMSG);
	CU_ASSERT(connTPutMsgBestArray_size==MAX_RESULT_SIZE);
	for(count=0; count< MAX_RESULT_SIZE; count++){	
		printf("Best Msgs: Name: %s. Msgs: %d\n", connTPutMsgBestArray[count]->name, connTPutMsgBestArray[count]->read_msg+connTPutMsgBestArray[count]->write_msg);
		CU_ASSERT(!strcmp( connTPutMsgBestArray[count]->name,connLast50ExpectResult[count] ));
	}
	
	processConnMonitoringDataResult(monconlist, MAX_CONN_SIZE, CONNCACHE_TYPE_WORSTTPUTMSG);
	CU_ASSERT(connTPutMsgWorstArray_size==MAX_RESULT_SIZE);
	for(count=0; count< MAX_RESULT_SIZE; count++){	
		printf("Worst Msgs: Name: %s. Msgs: %d\n", connTPutMsgWorstArray[count]->name, connTPutMsgWorstArray[count]->read_msg+connTPutMsgWorstArray[count]->write_msg);
		CU_ASSERT(!strcmp( connTPutMsgWorstArray[count]->name,connFirst50ExpectResult[count] ));
	}
	
	
	processConnMonitoringDataResult(monconlist, MAX_CONN_SIZE, CONNCACHE_TYPE_BESTTPUTBYTES);
	CU_ASSERT(connTPutBytesBestArray_size==MAX_RESULT_SIZE);
	for(count=0; count< MAX_RESULT_SIZE; count++){	
		printf("Best Bytes: Name: %s. Bytes: %d\n", connTPutBytesBestArray[count]->name, connTPutBytesBestArray[count]->read_msg+connTPutBytesBestArray[count]->write_msg);
		CU_ASSERT(!strcmp( connTPutBytesBestArray[count]->name,connLast50ExpectResult[count] ));
	}
	
	processConnMonitoringDataResult(monconlist, MAX_CONN_SIZE, CONNCACHE_TYPE_WORSTTPUTBYTES);
	CU_ASSERT(connTPutBytesWorstArray_size==MAX_RESULT_SIZE);
	for(count=0; count< MAX_RESULT_SIZE; count++){	
		printf("Worst Bytes: Name: %s. Bytes: %d\n", connTPutBytesWorstArray[count]->name, connTPutBytesWorstArray[count]->read_msg+connTPutBytesWorstArray[count]->write_msg);
		CU_ASSERT(!strcmp( connTPutBytesWorstArray[count]->name,connFirst50ExpectResult[count] ));
	}
	
	//Free Resources
	for(count=0; count< MAX_CONN_SIZE; count++){	
    	free(conarray[count].name);
	}
	for(count=0; count< MAX_RESULT_SIZE; count++){
		free(connFirst50ExpectResult[count]);
		free(connLast50ExpectResult[count]);
		
	}
}
void testConnectionMon(void) 
{

	test_ism_monitoring_sortComparatorConnTimeWorst();
	test_ism_monitoring_sortComparatorConnTimeBest();
	test_ism_monitoring_sortComparatorTPutMsgBest();
	test_ism_monitoring_sortComparatorTPutMsgWorst();
	test_ism_monitoring_sortComparatorTPutBytesBest();
	test_ism_monitoring_sortComparatorTPutBytesWorst();
	// test_ism_monitoring_getConnectionMonDataByEndPoint();
	// test_ism_monitoring_getConnectionMonData();
	// test_processConnMonitoringDataResult();
	
	return;
}
