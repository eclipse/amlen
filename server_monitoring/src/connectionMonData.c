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
 */
#define TRACE_COMP Monitoring

#include <ismutil.h>
#include "monitoringutil.h"
#include <ismmonitoringobjs.h>
#include <monserialization.h>
#include "connectionMonData.h"
#include <security.h>
#include <admin.h>
#include <janssonConfig.h>

/** Global hashtable for the connection data objects**/
ismHashMap * connectionDataEndpointMap;

DEFINE_CONNECTION_MONITORING_OBJ(Xism_connect_mon_t);
DEFINE_CONNECTION_VOLUME_MONITORING_OBJ(XismConnectionVolumeData);
DEFINE_CONNECTION_SECURITY_MONITORING_OBJ(Xsecurity_stat_t);

ism_connect_mon_t * connTimeBestArray[50];
int connTimeBestArray_size=0;
ism_connect_mon_t * connTimeWorstArray[50];
int connTimeWorstArray_size=0;
ism_connect_mon_t * connTPutBytesBestArray[50];
int connTPutBytesBestArray_size=0;
ism_connect_mon_t * connTPutBytesWorstArray[50];
int connTPutBytesWorstArray_size=0;
ism_connect_mon_t * connTPutMsgBestArray[50];
int connTPutMsgBestArray_size=0;
ism_connect_mon_t * connTPutMsgWorstArray[50];
int connTPutMsgWorstArray_size=0;


#define DEFAULT_ORPHANCLEAN_LONGRANGE_INTERVAL   120         //in second
static ism_time_t orphancleanlastime=0;
static const double SECOND_TO_NANOS = 1.0 * 1000000000.0;

/*
 * Monitoring action types
 */
typedef enum {
	CONN_OBJ_TYPE_CONNECTION    	= 0,
	CONN_OBJ_TYPE_NAME		    	= 1,
	CONN_OBJ_TYPE_USERID	   		= 2,
	CONN_OBJ_TYPE_ENDPOINT_NAME	    = 3,
	CONN_OBJ_TYPE_PROTOCOL		    = 4,
	CONN_OBJ_TYPE_CLIENT_ADDR	    = 5,
	
} ismConnectionObjectType_t;

#define ENDPOINT_NAME_DEFAULT_LEN 	256
#define USERID_DEFAULT_LEN 			256
#define NAME_DEFAULT_LEN			1024
#define PROTOCOL_LEN				64
#define CLIENT_ADDR_LEN				64

static int nextItem = 0;
static int nextNameItem = 0;
static int nextClientIdItem = 0;
static int nextProtocolItem = 0;
static int nextEndpointItem = 0;
static int nextUserIdItem = 0;

static ism_connect_mon_t ** connectionDataObjectsPool = NULL;
static char ** connNameObjectsPool = NULL;
static char ** clientAddrNameObjectsPool = NULL;
static char ** protocolNameObjectsPool = NULL;
static char ** endpointNameObjectsPool = NULL;
static char ** userIdObjectsPool = NULL;

extern pthread_key_t monitoring_localekey;

static void ism_monitoring_initConnectionDataObjectsPool(void);
static void ism_monitoring_freeConnectionDataObjectsPool(void);

static void ism_monitoring_returnObjectToPool(ismConnectionObjectType_t type, void * param);
//static void  ism_monitoring_returnConnectionObjectToPool(void * param);


#define BILLION 1000000000

static void getMsgId(ism_rc_t code, char * buffer) {
    sprintf(buffer, "CWLNA%04d", code%10000);
}

/**
 * Initialize the connection data object pool 
 */
static void ism_monitoring_initConnectionDataObjectsPool(void)
{
	int x=0;
	int poolSize = DEFAULT_OBJECTS_POOL_SIZE + 1;
	
	connectionDataObjectsPool = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,48),poolSize*sizeof(ism_connect_mon_t*));
    //TRACE(5, "CONNECTION pool created: pool=%p size=%d\n", connectionDataObjectsPool, poolSize);
	for(x=0; x< poolSize; x++)
	{
		connectionDataObjectsPool[x]=ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,49),1, sizeof(ism_connect_mon_t));
        //TRACE(5, "Add CONNECTION object to the pool : index=%d obj=%p\n", x, connectionDataObjectsPool[x]);
	}
	
	connNameObjectsPool = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,50),poolSize*sizeof(char *));
    //TRACE(5, "NAME pool created: pool=%p size=%d\n", connectionDataObjectsPool, poolSize);
	for(x=0; x< poolSize; x++)
	{
		connNameObjectsPool[x]=ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,51),1, NAME_DEFAULT_LEN+1);
        //TRACE(5, "Add NAME object to the pool : index=%d obj=%p\n", x, connNameObjectsPool[x]);
	}

	clientAddrNameObjectsPool = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,52),poolSize*sizeof(char *));
    //TRACE(5, "CLIENT_ADDR pool created: pool=%p size=%d\n", clientAddrNameObjectsPool, poolSize);
	for(x=0; x< poolSize; x++)
	{
		clientAddrNameObjectsPool[x]=ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,53),1, CLIENT_ADDR_LEN+1);
        //TRACE(5, "Add CLIENT_ADDR object to the pool : index=%d obj=%p\n", x, clientAddrNameObjectsPool[x]);
	}

	protocolNameObjectsPool = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,54),poolSize*sizeof(char *));
    //TRACE(5, "PROTOCOL pool created: pool=%p size=%d\n", protocolNameObjectsPool, poolSize);
	for(x=0; x< poolSize; x++)
	{
		protocolNameObjectsPool[x]=ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,55),1, PROTOCOL_LEN+1);
        //TRACE(5, "Add PROTOCOL object to the pool : index=%d obj=%p\n", x, protocolNameObjectsPool[x]);
	}
	
	endpointNameObjectsPool = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,56),poolSize*sizeof(char *));
    //TRACE(5, "ENDPOINT pool created: pool=%p size=%d\n", endpointNameObjectsPool, poolSize);
	for(x=0; x< poolSize; x++)
	{
		endpointNameObjectsPool[x]=ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,57),1, ENDPOINT_NAME_DEFAULT_LEN+1);
        //TRACE(5, "Add ENDPOINT object to the pool : index=%d obj=%p\n", x, endpointNameObjectsPool[x]);
	}

	userIdObjectsPool = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,58),poolSize*sizeof(char *));
    //TRACE(5, "USER_ID pool created: pool=%p size=%d\n", userIdObjectsPool, poolSize);
	for(x=0; x< poolSize; x++)
	{
		userIdObjectsPool[x]=ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,59),1, USERID_DEFAULT_LEN+1);
        //TRACE(5, "Add USER_ID object to the pool : index=%d obj=%p\n", x, userIdObjectsPool[x]);
	}
	
}
/**
 * Free the connection data object pool 
 */
static void ism_monitoring_freeConnectionDataObjectsPool(void)
{
	int x=0;
	int poolSize = DEFAULT_OBJECTS_POOL_SIZE + 1;

	for(x=0; x< poolSize; x++)
	{
		if(connectionDataObjectsPool[x]!=NULL)
		{
	        //TRACE(5, "Free CONNECTION object from the pool : index=%d obj=%p\n", x, connectionDataObjectsPool[x]);
			ism_common_free(ism_memory_monitoring_misc,connectionDataObjectsPool[x]);
			connectionDataObjectsPool[x] =NULL;
		}
	}
	ism_common_free(ism_memory_monitoring_misc,connectionDataObjectsPool);
    //TRACE(5, "CONNECTION pool freed: pool=%p size=%d\n", userIdObjectsPool, poolSize);
	
	for(x=0; x< poolSize; x++)
	{
		if(connNameObjectsPool[x]!=NULL)
		{
            //TRACE(5, "Free NAME object from the pool : index=%d obj=%p\n", x, connNameObjectsPool[x]);
			ism_common_free(ism_memory_monitoring_misc,connNameObjectsPool[x]);
			connNameObjectsPool[x] =NULL;
		}
	}
	ism_common_free(ism_memory_monitoring_misc,connNameObjectsPool);
    //TRACE(5, "NAME pool freed: pool=%p size=%d\n", userIdObjectsPool, poolSize);
	
	for(x=0; x< poolSize; x++)
	{
		if(clientAddrNameObjectsPool[x]!=NULL)
		{
            //TRACE(5, "Free CLIENT_ADDR object from the pool : index=%d obj=%p\n", x, clientAddrNameObjectsPool[x]);
			ism_common_free(ism_memory_monitoring_misc,clientAddrNameObjectsPool[x]);
			clientAddrNameObjectsPool[x] =NULL;
		}
	}
	ism_common_free(ism_memory_monitoring_misc,clientAddrNameObjectsPool);
    //TRACE(5, "CLIENT_ADDR pool freed: pool=%p size=%d\n", userIdObjectsPool, poolSize);
	
	for(x=0; x< poolSize; x++)
	{
		if(protocolNameObjectsPool[x]!=NULL)
		{
            //TRACE(5, "Free PROTOCOL object from the pool : index=%d obj=%p\n", x, protocolNameObjectsPool[x]);
			ism_common_free(ism_memory_monitoring_misc,protocolNameObjectsPool[x]);
			protocolNameObjectsPool[x] =NULL;
		}
	}
	ism_common_free(ism_memory_monitoring_misc,protocolNameObjectsPool);
    //TRACE(5, "PROTOCOL pool freed: pool=%p size=%d\n", userIdObjectsPool, poolSize);
	
	for(x=0; x< poolSize; x++)
	{
		if(endpointNameObjectsPool[x]!=NULL)
		{
            //TRACE(5, "Free ENDPOINT object from the pool : index=%d obj=%p\n", x, endpointNameObjectsPool[x]);
			ism_common_free(ism_memory_monitoring_misc,endpointNameObjectsPool[x]);
			endpointNameObjectsPool[x] =NULL;
		}
	}
	ism_common_free(ism_memory_monitoring_misc,endpointNameObjectsPool);
    //TRACE(5, "ENDPOINT pool freed: pool=%p size=%d\n", userIdObjectsPool, poolSize);
	
	for(x=0; x< poolSize; x++)
	{
		if(userIdObjectsPool[x]!=NULL)
		{
            //TRACE(5, "Free USER_ID object from the pool : index=%d obj=%p\n", x, userIdObjectsPool[x]);
			ism_common_free(ism_memory_monitoring_misc,userIdObjectsPool[x]);
			userIdObjectsPool[x] =NULL;
		}
	}
    //TRACE(5, "USER_ID pool freed: pool=%p size=%d\n", userIdObjectsPool, poolSize);
	ism_common_free(ism_memory_monitoring_misc,userIdObjectsPool);
}

/**
 * Get a connection data object from the pool
 */
static void *  ism_monitoring_getObjectFromPool(ismConnectionObjectType_t type)
{
	void * retObject=NULL;

	switch(type){
        case CONN_OBJ_TYPE_CONNECTION:
            if (nextItem < DEFAULT_OBJECTS_POOL_SIZE) {
                retObject = connectionDataObjectsPool[nextItem];
                connectionDataObjectsPool[nextItem] = NULL;
                nextItem++;
            }
            //TRACE(5, "getObjectFromPool: CONNECTION  index=%d obj=%p\n", index, retObject);
            break;
        case CONN_OBJ_TYPE_NAME:
            if (nextNameItem < DEFAULT_OBJECTS_POOL_SIZE) {
                retObject = connNameObjectsPool[nextNameItem];
                connNameObjectsPool[nextNameItem] = NULL;
                nextNameItem++;
            }
            //TRACE(5, "getObjectFromPool: NAME  index=%d obj=%p\n", index, retObject);
            break;
        case CONN_OBJ_TYPE_USERID:
            if (nextUserIdItem < DEFAULT_OBJECTS_POOL_SIZE) {
                retObject = userIdObjectsPool[nextUserIdItem];
                userIdObjectsPool[nextUserIdItem] = NULL;
                nextUserIdItem++;
            }
            //TRACE(5, "getObjectFromPool: USER_ID  index=%d obj=%p\n", index, retObject);
            break;
        case CONN_OBJ_TYPE_ENDPOINT_NAME:
            if (nextEndpointItem < DEFAULT_OBJECTS_POOL_SIZE) {
                retObject = endpointNameObjectsPool[nextEndpointItem];
                endpointNameObjectsPool[nextEndpointItem] = NULL;
                nextEndpointItem++;
            }
            //TRACE(5, "getObjectFromPool: ENDPOINT  index=%d obj=%p\n", index, retObject);
            break;
        case CONN_OBJ_TYPE_PROTOCOL:
            if (nextProtocolItem < DEFAULT_OBJECTS_POOL_SIZE) {
                retObject = protocolNameObjectsPool[nextProtocolItem];
                protocolNameObjectsPool[nextProtocolItem] = NULL;
                nextProtocolItem++;
            }
            //TRACE(5, "getObjectFromPool: PROTOCOL  index=%d obj=%p\n", index, retObject);
            break;
        case CONN_OBJ_TYPE_CLIENT_ADDR:
            if (nextClientIdItem < DEFAULT_OBJECTS_POOL_SIZE) {
                retObject = clientAddrNameObjectsPool[nextClientIdItem];
                clientAddrNameObjectsPool[nextClientIdItem] = NULL;
                nextClientIdItem++;
            }
            //TRACE(5, "getObjectFromPool: CLIENT_ADDR  index=%d obj=%p\n", index, retObject);
            break;
        default:
            retObject = NULL;
	}
	
	return retObject;
}


/**
 * Get a connection data object from the pool
 */
static void ism_monitoring_returnObjectToPool(ismConnectionObjectType_t type, void * param)
{
	ism_connect_mon_t * connmon =NULL;
	char * obj=NULL;
    if(param == NULL)
        return;

    switch(type){
	
		case CONN_OBJ_TYPE_CONNECTION :
			connmon = (ism_connect_mon_t *)param;
			nextItem--;
            //TRACE(5, "returnObjectToPool: CONNECTION  index=%d obj=%p oldObj=%p name=%p userid=%p listener=%p protocol=%p client_addr=%p\n",
                    //nextItem, connmon, connectionDataObjectsPool[nextItem], connmon->name, connmon->userid, connmon->listener, connmon->protocol, connmon->client_addr);
			connectionDataObjectsPool[nextItem]=connmon;
			
			ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_NAME, (void *)connmon->name );
			ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_USERID, (void *)connmon->userid );
			ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_ENDPOINT_NAME,(void *)connmon->listener );
			ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_PROTOCOL, (void *)connmon->protocol );
			ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_CLIENT_ADDR, (void *)connmon->client_addr );
	
		    memset(connmon,0, sizeof(ism_connect_mon_t));
			break;
			
		case CONN_OBJ_TYPE_NAME :
			obj=(char *) param;
			nextNameItem--;
            //TRACE(5, "returnObjectToPool: NAME  index=%d obj=%p oldObj=%p\n", nextNameItem, obj, connNameObjectsPool[nextNameItem]);
			connNameObjectsPool[nextNameItem]=obj;
			memset(obj,0, NAME_DEFAULT_LEN);
			break;
			
			
		case CONN_OBJ_TYPE_USERID :
			obj=(char *) param;
			nextUserIdItem--;
            //TRACE(5, "returnObjectToPool: USER_ID  index=%d obj=%p oldObj=%p\n", nextUserIdItem, obj, userIdObjectsPool[nextUserIdItem]);
			userIdObjectsPool[nextUserIdItem]=obj;
			memset(obj,0, USERID_DEFAULT_LEN);
			break;
			
		case CONN_OBJ_TYPE_ENDPOINT_NAME :
			obj=(char *) param;
			nextEndpointItem--;
            //TRACE(5, "returnObjectToPool: ENDPOINT  index=%d obj=%p oldObj=%p\n", nextEndpointItem, obj, endpointNameObjectsPool[nextEndpointItem]);
			endpointNameObjectsPool[nextEndpointItem]=obj;
			memset(obj,0, ENDPOINT_NAME_DEFAULT_LEN);
			break;
				
		case CONN_OBJ_TYPE_PROTOCOL :
			obj=(char *) param;
			nextProtocolItem--;
            //TRACE(5, "returnObjectToPool: PROTOCOL  index=%d obj=%p oldObj=%p\n", nextProtocolItem, obj, protocolNameObjectsPool[nextProtocolItem]);
			protocolNameObjectsPool[nextProtocolItem]=obj;
			memset(obj,0, PROTOCOL_LEN);
			break;
		
		case CONN_OBJ_TYPE_CLIENT_ADDR :
			obj=(char *) param;
			nextClientIdItem--;
            //TRACE(5, "returnObjectToPool: CLIENT_ADDR  index=%d obj=%p oldObj=%p\n", nextClientIdItem, obj, clientAddrNameObjectsPool[nextClientIdItem]);
			clientAddrNameObjectsPool[nextClientIdItem]=obj;
			memset(obj,0, CLIENT_ADDR_LEN);
			break;
			
		default:
			break;
			
	}
	
}

/**
 * Return a connection data object pool the pool
 */
/*static void  ism_monitoring_returnConnectionObjectToPool(void * param)
{	
    //ism_connect_mon_t * connmon = (ism_connect_mon_t *)param;
    //TRACE(5, "returnConnectionObjectToPool: obj=%p name=%p userid=%p listener=%p protocol=%p client_addr=%p\n",
            //connmon, connmon->name, connmon->userid, connmon->listener, connmon->protocol, connmon->client_addr);
	ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_CONNECTION, param);
}*/

/**
 * Initialize the connection data component
 */
void ism_monitoring_connectionMonDataInit(void)
{
	/*Create the hashtable*/
	connectionDataEndpointMap = ism_common_createHashMap(128,HASH_STRING);
	ism_monitoring_initConnectionDataObjectsPool();
}

/* Aggregated data of all connections from Endpoint/Listener monitoring Data */
static int ism_monitoring_connectionVolumeData(concat_alloc_t *output_buffer) {
    char rbuf[1024];
    ism_listener_mon_t * monlist;
    int nrec = 0;
    int rc = ISMRC_OK;
    int i = 0;
    
    ismConnectionVolumeData connVolume;
    memset(&connVolume, 0, sizeof(ismConnectionVolumeData));
    
    ismJsonSerializerData iSerUserData;
    ismSerializerData iSerData;
    memset(&iSerUserData,0, sizeof(ismJsonSerializerData));
    memset(&iSerData,0, sizeof(ismSerializerData));
    
    iSerUserData.isExternalMonitoring = 0;
    iSerUserData.outbuf = output_buffer;
    iSerData.serializer_userdata = &iSerUserData;

    /* get stats from engine if messaging has started */
    int sState = ism_admin_get_server_state();
    ismEngine_MessagingStatistics_t pStatistics = {0};
    if (sState == ISM_SERVER_RUNNING || sState == ISM_MESSAGING_STARTED) {
        ism_engine_getMessagingStatistics(&pStatistics);
    }

    if ( (nrec = ism_transport_getListenerMonitor(&monlist, NULL)) > 0 ) {
    	ism_listener_mon_t *mon = monlist;
        for (i=0; i<nrec; i++) {
            
            connVolume.MsgRead += mon->read_msg_count;
            connVolume.MsgWrite += mon->write_msg_count;
            connVolume.BytesRead += mon->read_bytes_count;
            connVolume.BytesWrite += mon->write_bytes_count;
            connVolume.ActiveConnections += mon->connect_active;
            connVolume.TotalConnections += mon->connect_count;
            connVolume.BadConnCount += mon->bad_connect_count;

            mon++;
        }
		connVolume.TotalEndpoints=nrec;
		
		/* add engine stats if available */
        connVolume.BufferedMessages = pStatistics.BufferedMessages;
        connVolume.RetainedMessages = pStatistics.RetainedMessages;
        connVolume.ExpiredMessages = pStatistics.ExpiredMessages;
        connVolume.ClientSessions = pStatistics.ClientStates;
        connVolume.ExpiredClientSessions = pStatistics.ExpiredClientStates;
        connVolume.Subscriptions = pStatistics.Subscriptions;

        if (connVolume.ActiveConnections < 0 ) connVolume.ActiveConnections = 0;


		ism_common_allocBufferCopyLen(iSerUserData.outbuf, "[", 1);
	    /* Serialize data */
	    ism_common_serializeMonJson(XismConnectionVolumeData, (void *)&connVolume, output_buffer->buf, 2500, &iSerData );
	    ism_common_allocBufferCopyLen(iSerUserData.outbuf, "]", 1);
		
        ism_transport_freeEndpointMonitor(monlist);
    } else {
        rc = ISMRC_EndpointNotFound;
		char ebuf[1024];
		char *errstr = (char *)ism_common_getErrorStringByLocale(rc, ism_common_getRequestLocale(monitoring_localekey), ebuf, 1024);
		if (errstr) {
            sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":", rc);
            ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
            /*JSON encoding for the Error String.*/
            ism_json_putString(output_buffer, errstr);
            ism_common_allocBufferCopyLen(output_buffer, " }", 2);
		}
    }
    return rc;
}
/**
 * Terminate the Connection Data Component
 */
void ism_monitoring_connectionMonDataDestroy(void)
{
	if(connectionDataEndpointMap!=NULL){
		ismHashMapEntry ** array;
		ism_connect_mon_data_t * connectionMonData=NULL;
		int i=0;
		
		/*Need to take a lock since the object/value will be used and destroyed*/
		ism_common_HashMapLock(connectionDataEndpointMap);
		
		array = ism_common_getHashMapEntriesArray(connectionDataEndpointMap);
		while(array[i] != ((void*)-1)){
			connectionMonData = (ism_connect_mon_data_t *)array[i]->value;
			pthread_spin_destroy(&connectionMonData->staginglock);
			pthread_spin_destroy(&connectionMonData->lock);
			ism_common_free(ism_memory_monitoring_misc,connectionMonData);
			connectionMonData=NULL;
			i++;
		}
		ism_common_freeHashMapEntriesArray(array);
		
		ism_common_HashMapUnlock(connectionDataEndpointMap);
		
		ism_common_destroyHashMap(connectionDataEndpointMap);
	}
	
	ism_monitoring_freeConnectionDataObjectsPool();	
}

/**
 * Get the connection data in the cache
 */
int ism_monitoring_getConnectionMonData (
		const char       *action,
		ism_json_parse_t *parseobj,
		concat_alloc_t   *output_buffer )
{
	int rc = ISMRC_OK;
	ism_CONNECTION_CACHE_TYPE _type;
	ism_connect_mon_data_t * connectionMonData=NULL;
	char rbuf[256];
	if(output_buffer==NULL){
		return ISMRC_Error;
	}

	const char * type = NULL;
	const char * endpoint = NULL;

	char *option = (char *)ism_json_getString(parseobj, "Option");
	
	if( (option && (*option == 'v' || *option == 'V') ) ){
		rc = ism_monitoring_connectionVolumeData(output_buffer);
		return rc;
	}else{
		/*Process connection monitoring data*/
		type = (char *)ism_json_getString(parseobj, "StatType");
		endpoint = (char *)ism_json_getString(parseobj, "Endpoint");
		//TRACE(5, "Connection Monitoring Data: %s\n", output_buffer->buf);
	}

	if(type==NULL){
		type="OldestConnection";
	}
	
	if(!strcmp(type,"OldestConnection")){
		_type = CONNCACHE_TYPE_BESTTIME;
	}else if(!strcmp(type,"NewestConnection")){
		_type = CONNCACHE_TYPE_WORSETIME;
	}else if(!strcmp(type,"LowestThroughputMsgs")){
		_type = CONNCACHE_TYPE_WORSTTPUTMSG;
	}else if(!strcmp(type,"HighestThroughputMsgs")){
		_type = CONNCACHE_TYPE_BESTTPUTMSG;
	}else if(!strcmp(type,"LowestThroughputKB")){
		_type = CONNCACHE_TYPE_WORSTTPUTBYTES;
	}else if(!strcmp(type,"HighestThroughputKB")){
		_type = CONNCACHE_TYPE_BESTTPUTBYTES;
	}else{
		_type = CONNCACHE_TYPE_BESTTIME;
	}
	
	ism_common_HashMapLock(connectionDataEndpointMap);

	/*Get the object from the table*/
	if(endpoint==NULL || !strcmp(endpoint,"all"))
	{
		connectionMonData = (ism_connect_mon_data_t *)ism_common_getHashMapElement(connectionDataEndpointMap,(void *) DEFAULT_ALL_ENDPOINT_NAME, 0);
	  	
	} else {
	    if ( ism_config_json_getComposite("Endpoint", endpoint, 1) == NULL ) {
	        connectionMonData = NULL;
	        rc = ISMRC_EndpointNotFound;
	        ism_common_HashMapUnlock(connectionDataEndpointMap);
	        return rc;
	    } else {
		    connectionMonData = (ism_connect_mon_data_t *)ism_common_getHashMapElement(connectionDataEndpointMap,(void *) endpoint, 0);
	    }
	}
	
	if ( connectionMonData != NULL ) {

		rc = ism_monitoring_getConnectionMonDataByEndPoint(connectionMonData, output_buffer, _type);
		
	} else {
		rc = ISMRC_ConnectionNotFound;
		char ebuf[1024];
		char *errstr = (char *)ism_common_getErrorStringByLocale(rc, ism_common_getRequestLocale(monitoring_localekey), ebuf, 1024);
		if (errstr) {
            sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":", rc);
            ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
            /*JSON encoding for the Error String.*/
            ism_json_putString(output_buffer, errstr);
            ism_common_allocBufferCopyLen(output_buffer, " }", 2);
		} else {
			 sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"No connection data is found.\" }", rc);
			 ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
		}
	}
	
	ism_common_HashMapUnlock(connectionDataEndpointMap);
	return rc;
}

/**
 * Get the Connection Data for an endpoint from the cache
 */
int ism_monitoring_getConnectionMonDataByEndPoint(ism_connect_mon_data_t * connectionMonData, concat_alloc_t *output_buffer, ism_CONNECTION_CACHE_TYPE type)
{
	int rc=ISMRC_OK;
	ism_connect_mon_data_cache_item_t * concache=NULL;
	
	if(connectionMonData==NULL){
		rc = ISMRC_NotFound;
        int xlen;
		char msgID[12];
		char  lbuf[1024];
		const char * repl[3];
		char tmpbuf[1024];
		getMsgId(6507, msgID);
		if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey),
				lbuf, sizeof(lbuf), &xlen) != NULL) {
			
			ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
		} else {
			 sprintf(tmpbuf, "No connection data is found");
		}
        
        ism_monitoring_setReturnCodeAndStringJSON(output_buffer, rc, tmpbuf);
        return rc;
	}
	
	pthread_spin_lock(&connectionMonData->lock);
	switch(type){
		case CONNCACHE_TYPE_BESTTIME: 
			concache= &connectionMonData->bestTimeCache;
			break;
		case CONNCACHE_TYPE_WORSETIME:
			concache= &connectionMonData->worstTimeCache;
			break;
		case CONNCACHE_TYPE_BESTTPUTMSG:
			concache= &connectionMonData->bestTputMsgCache;
			break;
		case CONNCACHE_TYPE_WORSTTPUTMSG:
			concache= &connectionMonData->worstTputMsgCache;
			break;
		case CONNCACHE_TYPE_BESTTPUTBYTES:
			concache= &connectionMonData->bestTputBytesCache;
			break;
			
		case CONNCACHE_TYPE_WORSTTPUTBYTES:
			concache= &connectionMonData->worstTputBytesCache;
			break;
		default:
			concache=NULL;
			break;
		
	}
	
	if(concache!=NULL && concache->buf_len>0){
		ism_common_allocBufferCopyLen(output_buffer, concache->buf, concache->buf_len);
	}else{
		rc = ISMRC_NotFound;
      	int xlen;
      	const char * repl[3];
		char msgID[12];
		char  lbuf[1024];
		char tmpbuf[1024];
		getMsgId(6507, msgID);
		if (ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey),
				lbuf, sizeof(lbuf), &xlen) != NULL) {
			
			ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
		} else {
			 sprintf(tmpbuf, "No connection data is found.");
		}
		ism_monitoring_setReturnCodeAndStringJSON(output_buffer, rc, tmpbuf);
        
	}
	
	TRACE(8, "GetConnectionMonDataFromConCache: size=%d. buffer:%s\n", concache->buf_len, concache->buf);
	TRACE(8, "GetConnectionMonDataFromOutBuffer: size=%d. buffer:%s\n", output_buffer->used, output_buffer->buf);
	
	
	pthread_spin_unlock(&connectionMonData->lock);
	
	
	return rc;
}
/**
 * Copy the Staging cache buffer to the Production Cache buffer
 * for the connection data.
 */
static void ism_monitoring_copyStagingToFinalCache(ism_connect_mon_data_t * connectionMonData)
{
	/*Lock & save the cache*/
	pthread_spin_lock(&connectionMonData->lock);
	
	memcpy(connectionMonData->bestTimeCache.buf,connectionMonData->bestTimeStagingCache.buf, connectionMonData->bestTimeStagingCache.buf_len );
	connectionMonData->bestTimeCache.buf_len = connectionMonData->bestTimeStagingCache.buf_len;
	
	memcpy(connectionMonData->worstTimeCache.buf,connectionMonData->worstTimeStagingCache.buf, connectionMonData->worstTimeStagingCache.buf_len );
	connectionMonData->worstTimeCache.buf_len = connectionMonData->worstTimeStagingCache.buf_len;
	
	memcpy(connectionMonData->worstTputMsgCache.buf,connectionMonData->worstTputMsgStagingCache.buf, connectionMonData->worstTputMsgStagingCache.buf_len );
	connectionMonData->worstTputMsgCache.buf_len = connectionMonData->worstTputMsgStagingCache.buf_len;
	
	memcpy(connectionMonData->bestTputMsgCache.buf,connectionMonData->bestTputMsgStagingCache.buf, connectionMonData->bestTputMsgStagingCache.buf_len );
	connectionMonData->bestTputMsgCache.buf_len = connectionMonData->bestTputMsgStagingCache.buf_len;
	
	memcpy(connectionMonData->bestTputBytesCache.buf,connectionMonData->bestTputBytesStagingCache.buf, connectionMonData->bestTputBytesStagingCache.buf_len );
	connectionMonData->bestTputBytesCache.buf_len = connectionMonData->bestTputBytesStagingCache.buf_len;
	
	memcpy(connectionMonData->worstTputBytesCache.buf,connectionMonData->worstTputBytesStagingCache.buf, connectionMonData->worstTputBytesStagingCache.buf_len );
	connectionMonData->worstTputBytesCache.buf_len = connectionMonData->worstTputBytesStagingCache.buf_len;
	
	/*UnLock*/
	pthread_spin_unlock(&connectionMonData->lock);
	
}

/**
 * Update the Staging cache from the List
 */
static void ism_monitoring_updateConCacheFromList(ism_connect_mon_data_t * connectionMonData, ism_connect_mon_t** connarray, int size, ism_CONNECTION_CACHE_TYPE type)
{
	concat_alloc_t buf = {0};
	char bufn[DEFAULT_CACHE_SIZE];
	ismJsonSerializerData iSerUserData;
    ismSerializerData iSerData;
    memset(&iSerUserData,0, sizeof(ismJsonSerializerData));
    memset(&iSerData,0, sizeof(ismSerializerData));
    ism_connect_mon_data_cache_item_t * concache=NULL;
    int i=0;
	
	if(size>0){
		iSerUserData.isExternalMonitoring = 0;
		iSerUserData.outbuf = &buf;
	  	iSerData.serializer_userdata = &iSerUserData;
	  	
	  	
		buf.buf     = bufn;
	    buf.len     = sizeof(bufn);
	    buf.used    = 0;
	    buf.inheap  = 0;
	    
	   
		ism_common_allocBufferCopyLen(iSerUserData.outbuf, "[", 1);
	
		for(i=0; i< size; i++){
			if (i > 0)
				ism_common_allocBufferCopyLen(iSerUserData.outbuf, ",", 1);
			ism_connect_mon_t * moncon =connarray[i];
			ism_common_serializeMonJson(Xism_connect_mon_t,(void *)moncon,buf.buf, 2500,&iSerData );
		
		}
	  	
		//Insert ']'
		ism_common_allocBufferCopyLen(iSerUserData.outbuf, "]", 1);
	
	}
	
	
	
	
	/*Lock & save the cache*/
	switch(type){
		case CONNCACHE_TYPE_BESTTIME: 
			concache= &connectionMonData->bestTimeStagingCache;
			break;
		case CONNCACHE_TYPE_WORSETIME:
			concache= &connectionMonData->worstTimeStagingCache;
			break;
		case CONNCACHE_TYPE_BESTTPUTMSG:
			concache= &connectionMonData->bestTputMsgStagingCache;
			break;
		case CONNCACHE_TYPE_WORSTTPUTMSG:
			concache= &connectionMonData->worstTputMsgStagingCache;
			break;
		case CONNCACHE_TYPE_BESTTPUTBYTES:
			concache= &connectionMonData->bestTputBytesStagingCache;
			break;
		case CONNCACHE_TYPE_WORSTTPUTBYTES:
			concache= &connectionMonData->worstTputBytesStagingCache;
			break;
		default:
			concache=NULL;
			break;
	}
	
	if(concache!=NULL){
		
		if(size>0 && buf.used>0){
			memcpy(concache->buf,buf.buf, buf.used );
			concache->buf_len = buf.used;
		}else{
			memset(concache->buf,0, sizeof(concache->buf) );
			concache->buf_len = 0;
		}
		
	}
	if(buf.inheap)
	    ism_common_free(ism_memory_monitoring_misc,buf.buf);
	
	
} 

/**
 * Clone the connection monitoring data object
 */
static ism_connect_mon_t * cloneConnectionMonitoringObject(ism_connect_mon_t * conmon,ism_connect_mon_t * conmon2 ) 
{
	ism_connect_mon_t * _conmon=conmon2;

	if(conmon!=NULL && _conmon!=NULL )
	{
        int len=0;
        char * ptr;
        //TRACE(5, "cloneConnectionMonitoringObject > : from=%p to=%p name=%p userid=%p listener=%p protocol=%p client_addr=%p\n",
                //conmon, _conmon, conmon->name, conmon->userid, conmon->listener, conmon->protocol, conmon->client_addr);
        memcpy(_conmon, conmon, sizeof(ism_connect_mon_t));
        /*Deep Copy the object*/
        if (conmon->name != NULL) {
            ptr = (char*) ism_monitoring_getObjectFromPool(CONN_OBJ_TYPE_NAME);
            if(ptr) {
                len = strlen(conmon->name);
                if (len > NAME_DEFAULT_LEN) {
                    len = NAME_DEFAULT_LEN;
                }
                memcpy(ptr, conmon->name, len);
                ptr[len] = '\0';
            }
            _conmon->name = ptr;
        }

        if (conmon->protocol != NULL) {
            ptr = (char *) ism_monitoring_getObjectFromPool(CONN_OBJ_TYPE_PROTOCOL);
            if(ptr) {
                len = strlen(conmon->protocol);
                if (len > PROTOCOL_LEN) {
                    len = PROTOCOL_LEN;
                }
                memcpy(ptr, conmon->protocol, len);
                ptr[len] = '\0';
            }
            _conmon->protocol = ptr;
        }

        if (conmon->client_addr != NULL) {
            ptr = (char *) ism_monitoring_getObjectFromPool(CONN_OBJ_TYPE_CLIENT_ADDR);
            if(ptr) {
                len = strlen(conmon->client_addr);
                if (len > CLIENT_ADDR_LEN) {
                    len = CLIENT_ADDR_LEN;
                }
                memcpy(ptr, conmon->client_addr, len);
                ptr[len] = '\0';
            }
            _conmon->client_addr = ptr;
        }
        if (conmon->userid != NULL) {
            ptr = (char *) ism_monitoring_getObjectFromPool(CONN_OBJ_TYPE_USERID);
            if(ptr) {
                len = strlen(conmon->userid);
                if (len > USERID_DEFAULT_LEN) {
                    len = USERID_DEFAULT_LEN;
                }
                memcpy(ptr, conmon->userid, len);
                ptr[len] = '\0';
            }
            _conmon->userid = ptr;
        }
        if (conmon->listener != NULL) {
            ptr = (char *) ism_monitoring_getObjectFromPool(CONN_OBJ_TYPE_ENDPOINT_NAME);
            if(ptr) {
                len = strlen(conmon->listener);
                if (len > ENDPOINT_NAME_DEFAULT_LEN) {
                    len = ENDPOINT_NAME_DEFAULT_LEN;
                }
                memcpy(ptr, conmon->listener, len);
                ptr[len] = '\0';
            }
            _conmon->listener = ptr;
        }
        //TRACE(5, "cloneConnectionMonitoringObject < : from=%p to=%p name=%p userid=%p listener=%p protocol=%p client_addr=%p\n",
                //conmon, _conmon, _conmon->name, _conmon->userid, _conmon->listener, _conmon->protocol, _conmon->client_addr);
	}
	return _conmon;
}

void processConnMonitoringDataResult(ism_connect_mon_t * resultMonArray, int result_count, ism_CONNECTION_CACHE_TYPE type)
{
	int i,j;
	ism_connect_mon_t * tmpArray[2050];
	
	ism_connect_mon_t **tmpTypeArray;
	ism_connect_mon_t * tmpConnArray[50];
	int * typeArraySize=0;
	int (*comparator)(const void *, const void *);
	
	
	if(result_count<=0) return;
	
	/*Determine the type*/
	
	switch(type){
		case CONNCACHE_TYPE_BESTTIME: 
			tmpTypeArray=connTimeBestArray;
			typeArraySize=&connTimeBestArray_size;
			comparator = ism_monitoring_sortComparatorConnTimeBest;
			break;
		case CONNCACHE_TYPE_WORSETIME:
			tmpTypeArray=connTimeWorstArray;
			typeArraySize=&connTimeWorstArray_size;
			comparator = ism_monitoring_sortComparatorConnTimeWorst;
			break;
		case CONNCACHE_TYPE_BESTTPUTMSG:
			tmpTypeArray=connTPutMsgBestArray;
			typeArraySize=&connTPutMsgBestArray_size;
			comparator = ism_monitoring_sortComparatorTPutMsgBest;
			break;
		case CONNCACHE_TYPE_WORSTTPUTMSG:
			tmpTypeArray=connTPutMsgWorstArray;
			typeArraySize=&connTPutMsgWorstArray_size;
			comparator = ism_monitoring_sortComparatorTPutMsgWorst;
			break;
		case CONNCACHE_TYPE_BESTTPUTBYTES:
			tmpTypeArray=connTPutBytesBestArray;
			typeArraySize=&connTPutBytesBestArray_size;
			comparator = ism_monitoring_sortComparatorTPutBytesBest;
			break;
		case CONNCACHE_TYPE_WORSTTPUTBYTES:
			tmpTypeArray=connTPutBytesWorstArray;
			typeArraySize=&connTPutBytesWorstArray_size;
			comparator = ism_monitoring_sortComparatorTPutBytesWorst;
			break;
		default:
			tmpTypeArray=connTimeBestArray;
			typeArraySize=&connTimeBestArray_size;
			comparator = ism_monitoring_sortComparatorConnTimeBest;
			break;
	}
	
	
	/*Backup the previous array so the objects can be returned.*/
	int retArraySize=0;
	if(typeArraySize!=NULL){
		retArraySize= *typeArraySize;
		for(i=0; i < retArraySize; i++)
			tmpConnArray[i] =tmpTypeArray[i];
	}
	
  	/*Combine the array from transport component and current hold objects into an array*/
	for(i = 0; i < result_count; i++)
		tmpArray[i] = resultMonArray + i;
	for(j=0; j< *typeArraySize; j++)
		tmpArray[i++] = tmpTypeArray[j];
	
	/*Sort the result*/
	qsort(tmpArray, i, sizeof(ism_connect_mon_t *),comparator);
	result_count = i;
	if ( i > 50 ) result_count = 50;
	
	/*Get and store maximum 50 objects*/
	if(result_count >0) *typeArraySize=0;
	for(i=0; i<result_count; i++){
		tmpTypeArray[ *typeArraySize] = cloneConnectionMonitoringObject(tmpArray[i] , (ism_connect_mon_t *)ism_monitoring_getObjectFromPool(CONN_OBJ_TYPE_CONNECTION));
		*typeArraySize =*typeArraySize+1;
	}
	
	/*Return previous array of objects to the pool.*/
	for(i=0; i < retArraySize; i++)
		ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_CONNECTION,(void*) tmpConnArray[i]);

}

static void cleanConnectionObjectsArray(void)
{

	int count=0;
		/*Clear the array*/
  	for(count=0; count < connTimeWorstArray_size; count++){
		ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_CONNECTION,(void*) connTimeWorstArray[count]);
		connTimeWorstArray[count]=NULL;
	}
  	connTimeWorstArray_size=0;
  
  	
  	for(count=0; count < connTimeBestArray_size; count++){
		ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_CONNECTION,(void*) connTimeBestArray[count]);
		connTimeBestArray[count]=NULL;
	}
  	connTimeBestArray_size=0;
  	
  		
  	for(count=0; count < connTPutMsgBestArray_size; count++){
		ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_CONNECTION,(void*) connTPutMsgBestArray[count]);
		connTPutMsgBestArray[count]=NULL;
	}
  	connTPutMsgBestArray_size=0;
  	
  	
  	for(count=0; count < connTPutMsgWorstArray_size; count++){
		ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_CONNECTION,(void*) connTPutMsgWorstArray[count]);
		connTPutMsgWorstArray[count]=NULL;
	}
  	connTPutMsgWorstArray_size=0;
  	
  	for(count=0; count < connTPutBytesWorstArray_size; count++){
		ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_CONNECTION,(void*) connTPutBytesWorstArray[count]);
		connTPutBytesWorstArray[count]=NULL;
	}
  	connTPutBytesWorstArray_size=0;
  		
  	for(count=0; count< connTPutBytesBestArray_size; count++){
		ism_monitoring_returnObjectToPool(CONN_OBJ_TYPE_CONNECTION,(void*) connTPutBytesBestArray[count]);
		connTPutBytesBestArray[count]=NULL;
	}
  	connTPutBytesBestArray_size=0;
}
 

/**
 * Update cache by endpoint
 */
void ism_monitoring_connectionCacheUpdate_endpoint(const char *endpoint)
{
	
	ism_connect_mon_t * monconlist;
	int maxcount=2000;
		
	int position=0; 
	int options=0;
	int result_count=1;	
	const char * _endpoint;
	const char * key;	
	
	ism_connect_mon_data_t * connectionMonData;
	
	
	
	if(endpoint!=NULL && strcmp(DEFAULT_ALL_ENDPOINT_NAME, endpoint)){
			_endpoint=endpoint;	
	}else{
			_endpoint= NULL;
	}
	key = endpoint;
	
	
	ism_common_HashMapLock(connectionDataEndpointMap);
  	/*Get the endpoint from hashtable, if not existed, create the connection data obj*/
  	connectionMonData = (ism_connect_mon_data_t *)ism_common_getHashMapElement(connectionDataEndpointMap,(void *) key, 0); /* BEAM suppression: passing null object */
  	if(connectionMonData==NULL){
  		connectionMonData = ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,74),1, sizeof(ism_connect_mon_data_t));
		pthread_spin_init(&connectionMonData->lock, 0);
		pthread_spin_init(&connectionMonData->staginglock, 0);
		
		ism_common_putHashMapElement(connectionDataEndpointMap, (void *)key, 0,(void *)connectionMonData, NULL ); /* BEAM suppression: passing null object */
		
  	}

  	
	/*Process for all endpoints*/
	while(result_count>0){
	    monconlist = NULL;
		result_count = ism_transport_getConnectionMonitor(&monconlist, maxcount, &position, options, 
													"*", DEFAULT_CONN_MON_FILTER_PROTOCOL, _endpoint,
													NULL, NULL, 0, 0xffff, ISM_INCOMING_CONNECTION | ISM_OUTGOING_CONNECTION);
									
	    
	    processConnMonitoringDataResult(monconlist, result_count, CONNCACHE_TYPE_WORSETIME);
	    processConnMonitoringDataResult(monconlist, result_count, CONNCACHE_TYPE_BESTTIME);
	    processConnMonitoringDataResult(monconlist, result_count, CONNCACHE_TYPE_BESTTPUTMSG);
	    processConnMonitoringDataResult(monconlist, result_count, CONNCACHE_TYPE_WORSTTPUTMSG);
	    processConnMonitoringDataResult(monconlist, result_count, CONNCACHE_TYPE_BESTTPUTBYTES);
	    processConnMonitoringDataResult(monconlist, result_count, CONNCACHE_TYPE_WORSTTPUTBYTES);
	    
		//Free result
		if(result_count>0)
			ism_transport_freeConnectionMonitor(monconlist);
													
	}
		
	 /*Update Cache*/
    ism_monitoring_updateConCacheFromList(connectionMonData,(ism_connect_mon_t **) &connTimeWorstArray,connTimeWorstArray_size, CONNCACHE_TYPE_WORSETIME);

	ism_monitoring_updateConCacheFromList(connectionMonData, (ism_connect_mon_t **)&connTimeBestArray,connTimeBestArray_size,CONNCACHE_TYPE_BESTTIME);

	ism_monitoring_updateConCacheFromList(connectionMonData,(ism_connect_mon_t **)&connTPutMsgBestArray,connTPutMsgBestArray_size,CONNCACHE_TYPE_BESTTPUTMSG);

	ism_monitoring_updateConCacheFromList(connectionMonData,(ism_connect_mon_t **)&connTPutMsgWorstArray,connTPutMsgWorstArray_size,CONNCACHE_TYPE_WORSTTPUTMSG);

	ism_monitoring_updateConCacheFromList(connectionMonData, (ism_connect_mon_t **)&connTPutBytesWorstArray,connTPutBytesWorstArray_size,CONNCACHE_TYPE_WORSTTPUTBYTES);

	ism_monitoring_updateConCacheFromList(connectionMonData, (ism_connect_mon_t **)&connTPutBytesBestArray,connTPutBytesBestArray_size,CONNCACHE_TYPE_BESTTPUTBYTES);
	
	
	/*Finish processing the objects and stored int he staging cache. do the final copy*/
	pthread_spin_lock(&connectionMonData->staginglock);
	ism_monitoring_copyStagingToFinalCache(connectionMonData);
	pthread_spin_unlock(&connectionMonData->staginglock);
	
	ism_common_HashMapUnlock(connectionDataEndpointMap);
	
	cleanConnectionObjectsArray();
  
  
  	
}
/**
 * Connection Cache Update
 */
void ism_monitoring_connectionCacheUpdate(void)
{
	ism_endpoint_mon_t * monlis;
	int result_count=1;
	int pcount=0;
	
	
	/*Process the data from all the endpoints*/
	ism_monitoring_connectionCacheUpdate_endpoint(DEFAULT_ALL_ENDPOINT_NAME);
	
	/*Get all the end points and process the connection data*/
	result_count = ism_transport_getEndpointMonitor(&monlis, NULL);
	if(result_count>0)
	{	
		/*Remove Orphan Connection Data (element)
		 *Optimize by do it every 100 iteration 
		 */
		if(connectionDataEndpointMap!=NULL){
			
			uint64_t currenttime = (uint64_t) ism_common_readTSC() ;
			
			/*Do the orphanage clean up every 2 minutes (120 seconds).*/  	
       		if ( currenttime >= ((DEFAULT_ORPHANCLEAN_LONGRANGE_INTERVAL) + orphancleanlastime))
       		{
      
				
				orphancleanlastime=currenttime;
				
				ismHashMapEntry ** array;
				ism_connect_mon_data_t * connectionMonData=NULL;
				int i=0;

				/*Need to take the lock since the value will be used.*/
				ism_common_HashMapLock(connectionDataEndpointMap);
				
				array = ism_common_getHashMapEntriesArray(connectionDataEndpointMap);
				while(array[i] != ((void*)-1)){
					connectionMonData = (ism_connect_mon_data_t *)array[i]->value;
					int found=0;
					for(pcount=0; pcount< result_count; pcount++)
					{
						ism_endpoint_mon_t * epmon = &monlis[pcount];
						if(!strncmp(epmon->name,(char *)array[i]->key, array[i]->key_len) ||
						   !strncmp(DEFAULT_ALL_ENDPOINT_NAME,(char *)array[i]->key, array[i]->key_len))
						{
							found=1;
							break;
						}
						
					}
					
					if(found==0)
					{
						
						/*it is an orphan in the hashtable remove the item.*/
						ism_common_removeHashMapElement(connectionDataEndpointMap, array[i]->key, array[i]->key_len);
						
						/*Make sure that the object is release from other thread first*/
						pthread_spin_lock(&connectionMonData->lock);
						pthread_spin_unlock(&connectionMonData->lock);
	
						
						pthread_spin_destroy(&connectionMonData->staginglock);
						pthread_spin_destroy(&connectionMonData->lock);
						ism_common_free(ism_memory_monitoring_misc,connectionMonData);
						connectionMonData=NULL;
						
					}
					
					i++;
				}
				ism_common_freeHashMapEntriesArray(array);
				
				ism_common_HashMapUnlock(connectionDataEndpointMap);
			}
		}
		
		/*Process Cache Update*/
		for(pcount=0; pcount< result_count; pcount++)
		{
			ism_endpoint_mon_t * epmon = &monlis[pcount];
			ism_monitoring_connectionCacheUpdate_endpoint(epmon->name);
		}
		
	}else{
	
		/*Remove all connection cache because there is no endpoint.*/
		ismHashMapEntry ** array;
		ism_connect_mon_data_t * connectionMonData=NULL;
		int i=0;
		ism_common_HashMapLock(connectionDataEndpointMap);
		
		array = ism_common_getHashMapEntriesArray(connectionDataEndpointMap);
		while(array[i] != ((void*)-1)){
			connectionMonData = (ism_connect_mon_data_t *)array[i]->value;
									
			/*it is an orphan in the hashtable remove the item.*/
			ism_common_removeHashMapElement(connectionDataEndpointMap, array[i]->key, array[i]->key_len);
			
			/*Make sure that the object is release from other thread first*/
			pthread_spin_lock(&connectionMonData->lock);
			pthread_spin_unlock(&connectionMonData->lock);

			
			pthread_spin_destroy(&connectionMonData->staginglock);
			pthread_spin_destroy(&connectionMonData->lock);
			ism_common_free(ism_memory_monitoring_misc,connectionMonData);
			connectionMonData=NULL;
				
			i++;
		}
		ism_common_freeHashMapEntriesArray(array);
		
		ism_common_HashMapUnlock(connectionDataEndpointMap);
	}
	
	if(monlis)
		ism_transport_freeEndpointMonitor(monlis);
	
	
}

/* Security Stats */
XAPI int ism_monitoring_getSecurityStats(char * action, ism_json_parse_t * inputJSONObj, concat_alloc_t * outputBuffer)
{
    int rc = ISMRC_OK;
    char rbuf[4096];
    security_stat_t * stats = ism_security_getStat();
    if ( stats == NULL ) {
    	sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"Failed to query security statistics.\" }", rc);
        ism_common_allocBufferCopyLen(outputBuffer,rbuf,strlen(rbuf));
    } else {
        ismJsonSerializerData iSerUserData;
        ismSerializerData iSerData;
        memset(&iSerUserData,0, sizeof(ismJsonSerializerData));
        memset(&iSerData,0, sizeof(ismSerializerData));

        iSerUserData.isExternalMonitoring = 0;
        iSerUserData.outbuf = outputBuffer;
        iSerData.serializer_userdata = &iSerUserData;

	    /* Serialize data */
	    ism_common_serializeMonJson(Xsecurity_stat_t, (void *)stats, outputBuffer->buf, 2500, &iSerData );
    }
    return rc;
}


