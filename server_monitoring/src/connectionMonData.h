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
#ifndef __ISM_CONNECTIONMONDATA_DEFINED
#define __ISM_CONNECTIONMONDATA_DEFINED

#include <pthread.h>
#define DEFAULT_CONN_DATA_UPDATE_INTERVAL 60
#define DEFAULT_CACHE_ITEM_SIZE 50
#define DEFAULT_CACHE_SIZE		512*DEFAULT_CACHE_ITEM_SIZE
#define DEFAULT_ALL_ENDPOINT_NAME "ISM_ALL_ENDPOINT"
#define DEFAULT_OBJECTS_POOL_SIZE 8 * DEFAULT_CACHE_ITEM_SIZE

/*Define Fitlers*/
#define DEFAULT_CONN_MON_FILTER_PROTOCOL NULL


/**
 * Connection Data Cache Type
 */
typedef enum ism_CONNECTION_CACHE_TYPE {
    CONNCACHE_TYPE_BESTTIME         	= 0,
    CONNCACHE_TYPE_WORSETIME         	= 1,
    CONNCACHE_TYPE_BESTTPUTMSG         	= 2,                  
    CONNCACHE_TYPE_WORSTTPUTMSG        	= 3,
    CONNCACHE_TYPE_BESTTPUTBYTES        = 4,
    CONNCACHE_TYPE_WORSTTPUTBYTES       = 5,
   
}ism_CONNECTION_CACHE_TYPE;

typedef struct ism_connect_mon_data_cache_item_t
{
	char buf[DEFAULT_CACHE_SIZE];
	int buf_len;
}ism_connect_mon_data_cache_item_t;

/**
 * The connection monitoring data object for monitoring
 */
typedef struct ism_connect_mon_data_t {
   pthread_spinlock_t    lock;         /**< Lock over snapshots  */
   pthread_spinlock_t    staginglock;  /**< Lock over snapshots  */
      
   ism_connect_mon_data_cache_item_t bestTimeStagingCache;
   ism_connect_mon_data_cache_item_t worstTimeStagingCache;
   ism_connect_mon_data_cache_item_t bestTputMsgStagingCache;
   ism_connect_mon_data_cache_item_t worstTputMsgStagingCache;
   ism_connect_mon_data_cache_item_t bestTputBytesStagingCache;
   ism_connect_mon_data_cache_item_t worstTputBytesStagingCache;
 
   ism_connect_mon_data_cache_item_t bestTimeCache;
   ism_connect_mon_data_cache_item_t worstTimeCache;
   ism_connect_mon_data_cache_item_t bestTputMsgCache;
   ism_connect_mon_data_cache_item_t worstTputMsgCache;
   ism_connect_mon_data_cache_item_t bestTputBytesCache;
   ism_connect_mon_data_cache_item_t worstTputBytesCache;
} ism_connect_mon_data_t;

/** Global hashtable for the connection data objects**/
extern ismHashMap * connectionDataEndpointMap;

/**
 * Initialize the Connection Data objects
 */
 
void ism_monitoring_connectionMonDataInit(void);
/**
 * Terminate the Connection Monitoring Data. 
 */
void ism_monitoring_connectionMonDataDestroy(void);
/**
 * Update Internal Cache
 */
void ism_monitoring_connectionCacheUpdate(void);

/**
 * Get Connection Data For the endpoint. 
 */
int ism_monitoring_getConnectionMonDataByEndPoint(ism_connect_mon_data_t * connectionMonData, concat_alloc_t *output_buffer, ism_CONNECTION_CACHE_TYPE type);

/**
 * Get the Connection Monitoring data. 
 * This API will be called from the IO thread to get the last updated cache
 */
int ism_monitoring_getConnectionMonData ( const char *action, ism_json_parse_t *parseobj, concat_alloc_t *output_buffer );


#endif
