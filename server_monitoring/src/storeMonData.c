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

#include <monitoring.h>
#include <ismjson.h>
#include <errno.h>
#include <ismutil.h>
#include <monitoringutil.h>
#include <store.h>

int haMode = 0;

extern pthread_key_t monitoring_localekey;

static ism_time_t lastime=0, currenttime=0;

/*
 * Holding monitoring data for each snap shot of memory
 */
typedef struct ism_mondata_store_t {
  	
  	int64_t StoreMemUsagePct;
  	int64_t StoreDiskUsagePct;
  	int64_t DiskFreeSpaceBytes;
  	
  	int64_t MemoryTotalBytes;
  	int64_t MemoryUsedPercent;
  	int64_t MemoryFreeBytes;
  	int64_t IncomingMessageAcksBytes;
  	int64_t QueuesBytes;
  	int64_t SubscriptionsBytes;
  	int64_t TopicsBytes;
  	int64_t TransactionsBytes;
  	int64_t MQConnectivityBytes;
  	int64_t ClientStatesBytes;

  	int32_t Pool1RecordSizeBytes;           /* size of owner record          */
  	int64_t Pool1TotalBytes;      /* Pool1 = small granules pool   */
  	int64_t Pool1UsedBytes;       /* small granules pool usage     */
  	int64_t Pool1RecordsLimitBytes; /* owner limit                 */
  	int64_t Pool1RecordsUsedBytes;  /* memory taken by owners      */
  	int64_t Pool2TotalBytes;      /* Pool2 = large granules pool   */
  	int64_t Pool2UsedBytes;       /* large granules pool usage     */
  	int8_t  Pool1UsedPercent;     /* small granules pool usage pct */
  	int8_t  Pool2UsedPercent;     /* large granules pool usage pct */

    
}ism_mondata_store_t;

static ism_monitoring_snap_list_t * monitoringStoreSnapList = NULL;

static int updateStoreDataNode(ismStore_Statistics_t * storeStats, ism_mondata_store_t *data);

static int32_t getStoreHistoryStats(ism_monitoring_snap_t *list, char * types, int duration, ism_snaptime_t interval, concat_alloc_t * output_buffer);

/**
 * Create the Statistics JSON String for Store
 * This function doesn't include the open and closing brackets
 */
static void createMemoryDetailStoreStatString(concat_alloc_t * outputBuffer,  ism_mondata_store_t * data)
{
	char rbuf[1024];

	sprintf(rbuf, "\"MemoryUsedPercent\":%u,\"MemoryTotalBytes\":%lu,\"Pool1TotalBytes\":%lu,\"Pool1UsedBytes\":%lu,\"Pool1UsedPercent\":%d,\"Pool1RecordSizeBytes\":%d,\"Pool1RecordsLimitBytes\":%lu,\"Pool1RecordsUsedBytes\":%lu,\"ClientStatesBytes\":%lu,\"SubscriptionsBytes\":%lu,\"TopicsBytes\":%lu,\"TransactionsBytes\":%lu,\"QueuesBytes\":%lu,\"MQConnectivityBytes\":%lu,\"Pool2TotalBytes\":%lu,\"Pool2UsedBytes\":%lu,\"Pool2UsedPercent\":%d,\"IncomingMessageAcksBytes\":%lu",
				(unsigned)data->StoreMemUsagePct,
				data->MemoryTotalBytes,
				data->Pool1TotalBytes,
				data->Pool1UsedBytes,
				data->Pool1UsedPercent,
				data->Pool1RecordSizeBytes,
				data->Pool1RecordsLimitBytes,
				data->Pool1RecordsUsedBytes,
				data->ClientStatesBytes,
				data->SubscriptionsBytes,
				data->TopicsBytes,
				data->TransactionsBytes,
				data->QueuesBytes,
				data->MQConnectivityBytes,
				data->Pool2TotalBytes,
				data->Pool2UsedBytes,
				data->Pool2UsedPercent,
				data->IncomingMessageAcksBytes
				);

	ism_common_allocBufferCopyLen(outputBuffer,rbuf,strlen(rbuf));

}

/**
 * Create the Statistics JSON String for Store
 * This function doesn't include the open and closing brackets
 */
static void createCurrentStoreStatString(int isExternalMonitoring, concat_alloc_t * outputBuffer,  ism_mondata_store_t * data)
{
	char rbuf[1024];

	if(isExternalMonitoring == 0){
		sprintf(rbuf, "\"DiskUsedPercent\":%u,\"DiskFreeBytes\":%lu,\"MemoryUsedPercent\":%u,\"MemoryTotalBytes\":%lu,\"Pool1TotalBytes\":%lu,\"Pool1UsedBytes\":%lu,\"Pool1UsedPercent\":%d,\"Pool1RecordsLimitBytes\":%lu,\"Pool1RecordsUsedBytes\":%lu,\"Pool2TotalBytes\":%lu,\"Pool2UsedBytes\":%lu,\"Pool2UsedPercent\":%d",
				(unsigned)data->StoreDiskUsagePct,
				data->DiskFreeSpaceBytes,
				(unsigned)data->StoreMemUsagePct,
				data->MemoryTotalBytes,
				data->Pool1TotalBytes,
				data->Pool1UsedBytes,
				data->Pool1UsedPercent,
				data->Pool1RecordsLimitBytes,
				data->Pool1RecordsUsedBytes,
				data->Pool2TotalBytes,
				data->Pool2UsedBytes,
				data->Pool2UsedPercent

				);
	}else{
		sprintf(rbuf, "\"DiskUsedPercent\":%u,\"DiskFreeBytes\":%lu,\"MemoryUsedPercent\":%u,\"MemoryTotalBytes\":%lu,\"Pool1TotalBytes\":%lu,\"Pool1UsedBytes\":%lu,\"Pool1UsedPercent\":%d,\"Pool1RecordsLimitBytes\":%lu,\"Pool1RecordsUsedBytes\":%lu,\"Pool2TotalBytes\":%lu,\"Pool2UsedBytes\":%lu,\"Pool2UsedPercent\":%d",
						(unsigned)data->StoreDiskUsagePct,
						data->DiskFreeSpaceBytes,
						(unsigned)data->StoreMemUsagePct,
						data->MemoryTotalBytes,
						data->Pool1TotalBytes,
						data->Pool1UsedBytes,
						data->Pool1UsedPercent,
						data->Pool1RecordsLimitBytes,
						data->Pool1RecordsUsedBytes,
						data->Pool2TotalBytes,
						data->Pool2UsedBytes,
						data->Pool2UsedPercent

						);
	}
	ism_common_allocBufferCopyLen(outputBuffer,rbuf,strlen(rbuf));

}

/**
 * Returns basic store statistics
 */
XAPI int ism_monitoring_getStoreStats(char * action, ism_json_parse_t * inputJSONObj, concat_alloc_t * outputBuffer, int isExternalMonitoring)
{
	ismStore_Statistics_t storeStats = {0};
	int rc = ISMRC_OK;
	char rbuf[1024];
	const char * repl[3];
	char  msgID[12];
    char tmpbuf[128];
    char  lbuf[1024];
    memset(lbuf, 0, sizeof(lbuf));
    int xlen;
    
    memset(&rbuf, 0, sizeof(rbuf));
    
     char *subtype=NULL;
    if(inputJSONObj!=NULL){
    	subtype = (char *)ism_json_getString(inputJSONObj, "SubType");
    }
    if(subtype==NULL || *subtype=='\0'){
    	subtype="current";
    }
    
	rc = ism_store_getStatistics(&storeStats);
	
	if ( rc != ISMRC_OK ) {
		
		ism_monitoring_getMsgId(6516, msgID);
        
        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 0);
        } else {
        	sprintf(tmpbuf, "Failed to query the store statistics.");
        }
        ism_monitoring_setReturnCodeAndStringJSON(outputBuffer, rc, tmpbuf);
    		
    	return rc;
		
	} else {
	
		//Update History Data for Store for every 6 seconds
    	currenttime = (uint64_t) ism_common_readTSC() ;
	    if ( currenttime >= ((DEFAULT_SNAPSHOTS_SHORTRANGE6_INTERVAL) + lastime)) {
			TRACE(8, "Start process the short range snap shot for Store data.\n");
	        int irc = ism_monitoring_updateSnapshot(currenttime, &storeStats, ismMON_STAT_Store, monitoringStoreSnapList);
	        if (irc) {
			    TRACE(8, "Failed to  process the short range snap shot for Store data.\n");
	        }else{
	        	TRACE(8, "End process the short range snap shot for Store data.\n");
	        }
	          
	        lastime = currenttime;
	    }
	
		if(!strcmpi(subtype, "current")){
    		//Get Current Memory Data
			ism_mondata_store_t data;
			memset(&data, 0, sizeof(ism_mondata_store_t));
			updateStoreDataNode(&storeStats, &data);

			if(isExternalMonitoring==0){
				ism_common_allocBufferCopyLen(outputBuffer,"{",1);
				createCurrentStoreStatString(isExternalMonitoring, outputBuffer,  &data);
			    ism_common_allocBufferCopyLen(outputBuffer," }",2);

			}else{
				char msgPrefixBuf[256];
	       		concat_alloc_t prefixbuf = { NULL, 0, 0, 0, 0 };
	       		prefixbuf.buf = (char *) &msgPrefixBuf;
	       		prefixbuf.len = sizeof(msgPrefixBuf);
	       		
	       		uint64_t currentTime = ism_common_currentTimeNanos();
	       		
	       		ism_monitoring_getMsgExternalMonPrefix(ismMonObjectType_Store,
	       											 currentTime,
													 NULL,
													 &prefixbuf);
				
				char  * msgPrefix = alloca(prefixbuf.used+1);
				memcpy(msgPrefix, prefixbuf.buf, prefixbuf.used);
				msgPrefix[prefixbuf.used]='\0';
	       		
				sprintf(rbuf, "{ %s,",msgPrefix);
	   			ism_common_allocBufferCopyLen(outputBuffer,rbuf,strlen(rbuf));
	   			createCurrentStoreStatString(isExternalMonitoring, outputBuffer,  &data);
	   			ism_common_allocBufferCopyLen(outputBuffer," }",2);
				
				if(prefixbuf.inheap) ism_common_freeAllocBuffer(&prefixbuf);
			
			}

		}else if(!strcmpi(subtype, "MemoryDetail")){
			ism_mondata_store_t data;
			memset(&data, 0, sizeof(ism_mondata_store_t));
			updateStoreDataNode(&storeStats, &data);
			ism_common_allocBufferCopyLen(outputBuffer,"{",1);
			createMemoryDetailStoreStatString(outputBuffer,  &data);
			ism_common_allocBufferCopyLen(outputBuffer," }",2);
		}else{
			//Get Historical data

       		char *dur = (char *)ism_json_getString(inputJSONObj, "Duration");
			if (dur == NULL) {
				dur="1800";
			}
			int duration = atoi(dur);
			if (duration > 0 && duration < 5) {
				duration = 5;
			}
			int interval = 0;
			
			if (duration <= 60*60) {
				//TRACE(9, "Monitoring: get data from short list\n");
				interval = DEFAULT_SNAPSHOTS_SHORTRANGE6_INTERVAL;
			} else {
				//TRACE(9, "Monitoring: get data from long list\n");
				interval = DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL;
			}
			ism_monitoring_snap_t * list = ism_monitoring_getSnapshotListByInterval(interval, monitoringStoreSnapList);
			
			char * types = (char *)ism_json_getString(inputJSONObj, "StatType");
			
			rc = getStoreHistoryStats(list, types, duration, interval, outputBuffer);
		}
	}
	
	return rc;
}

/**
 * Returns HA monitoring data
 * - Current HA mode
 * - Status of data replication
 */
XAPI int ism_monitoring_getHAStats(char * action, ism_json_parse_t * inputJSONObj, concat_alloc_t * outputBuffer)
{
	/* HA stats */
	int rc = ISMRC_OK;
	char rbuf[1024];
	sprintf(rbuf, "{ \"HAMode\":\"%d\" }", haMode);
	ism_common_allocBufferCopyLen(outputBuffer,rbuf,strlen(rbuf));
	return rc;
}


/*******************************************************SNAPSHOT OF STORE***********************/


/*Initialize the snapshot list for Memory*/
int ism_monitoring_initStoreHistMonitoringList(void)  {
	return ism_monitoring_initMonitoringSnapList(&monitoringStoreSnapList, DEFAULT_SNAPSHOTS_SHORTRANGE6_INTERVAL, DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL);
}

/*Create new Memory Historical object*/
void * ism_monitoring_newStoreHistDataObject(void)
{	
	ism_mondata_store_t * ret = ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,1),1, sizeof(ism_mondata_store_t));
	memset(ret, -1, sizeof(ism_mondata_store_t));
	return (void *)ret;
}

static int updateStoreDataNode(ismStore_Statistics_t * storeStats, ism_mondata_store_t *data)
{
	if(!storeStats || !data){
		return -1;
	}

	//data->StoreMemUsagePct = storeStats->StoreMemUsagePct;
	data->StoreMemUsagePct = storeStats->MemStats.MemoryUsedPercent;
	data->StoreDiskUsagePct =  storeStats->StoreDiskUsagePct;
	data->DiskFreeSpaceBytes= storeStats->DiskFreeSpaceBytes;

	data->MemoryTotalBytes= storeStats->MemStats.MemoryTotalBytes;
	data->MemoryUsedPercent= storeStats->MemStats.MemoryUsedPercent;
	data->MemoryFreeBytes= storeStats->MemStats.MemoryFreeBytes;

	data->IncomingMessageAcksBytes =storeStats->MemStats.IncomingMessageAcksBytes;
	data->ClientStatesBytes= storeStats->MemStats.ClientStatesBytes;
	data->MQConnectivityBytes= storeStats->MemStats.MQConnectivityBytes;
	data->QueuesBytes= storeStats->MemStats.QueuesBytes;
	data->SubscriptionsBytes= storeStats->MemStats.SubscriptionsBytes;
	data->TopicsBytes = storeStats->MemStats.TopicsBytes;
	data->TransactionsBytes= storeStats->MemStats.TransactionsBytes;

	data->Pool1RecordSizeBytes= storeStats->MemStats.RecordSize;
	data->Pool1TotalBytes= storeStats->MemStats.Pool1TotalBytes;
	data->Pool1UsedBytes= storeStats->MemStats.Pool1UsedBytes;
	data->Pool1RecordsLimitBytes= storeStats->MemStats.Pool1RecordsLimitBytes;
	data->Pool1RecordsUsedBytes= storeStats->MemStats.Pool1RecordsUsedBytes;
	data->Pool2TotalBytes= storeStats->MemStats.Pool2TotalBytes;
	data->Pool2UsedBytes= storeStats->MemStats.Pool2UsedBytes;
	data->Pool1UsedPercent= storeStats->MemStats.Pool1UsedPercent;
	data->Pool2UsedPercent= storeStats->MemStats.Pool2UsedPercent;


	return 0;
}


/*
 * The new node has already been allocated or resigned as the first in the list in initNewData
 */
static int storeStoreMonData(ism_snapshot_range_t ** list, ismStore_Statistics_t * storeStats) {
	ism_snapshot_range_t *sp;

	if(list!=NULL && *list==NULL){
		ism_monitoring_newSnapshotRange(list, ismMON_STAT_Store, free);
	}

    /*For Memory snapshot, there is only 1  for snapshot range list*/
    sp = *list;					/* BEAM suppression: dereferencing NULL */
    
	pthread_spin_lock(&sp->snplock);
	
	ism_snapshot_data_node_t * node = (ism_snapshot_data_node_t *)sp->data_nodes;
	
	updateStoreDataNode(storeStats, (ism_mondata_store_t *)node->data);
	
	if (sp->node_idle > 0)
             sp->node_idle--;
	pthread_spin_unlock(&sp->snplock);
	
	return ISMRC_NotFound;
}


/**
 * Record Memory Snapshot
 */
XAPI int ism_monitoring_recordStoreSnapShot(ism_monitoring_snap_t * slist, ism_monitoring_snap_t * llist, void *stat) {
	
	int rc = ISMRC_OK;
	ismStore_Statistics_t storeStats = {0};
	ismStore_Statistics_t *storeStat=NULL;
	
	
	if(stat!=NULL){
		storeStat = (ismStore_Statistics_t *)stat;
	}else{
		rc = ism_store_getStatistics(&storeStats);
		if(rc!=ISMRC_OK){
			return rc;
		}
		storeStat =  (ismStore_Statistics_t *)&storeStats;
	}
	
	storeStoreMonData(&(slist->range_list), storeStat);
	
	if(llist!=NULL){
		storeStoreMonData(&(llist->range_list), storeStat);
	}
  
	return rc;
}

/*Memory Data Type for Historical data*/
typedef enum {
	
    ismMonStore_NONE          	    	= 0,
    ismMonStore_MEM_USAGE_PCT  			= 1,   	//StoreMemUsagePct
    ismMonStore_DISK_USAGE_PCT  		= 2,   	//StoreDiskUsagePct
    ismMonStore_DISK_FREE_SPACE_BYTES 	= 3,   	//DiskFreeSpaceBytes
    ismMonStore_IN_MSG_ACK_BYTES		= 4,   	//IncomingMessageAcksBytes
    ismMonStore_QUEUES_BYTES			= 5,   	//QueuesMemBytes
    ismMonStore_SUBS_BYTES				= 6,   	//SubscriptionsMemBytes
    ismMonStore_TOPICS_BYTES			= 7,   	//TopicsMemBytes
    ismMonStore_TRANSACTIONS_BYTES 		= 8,   	//TransactionsMemBytes
    ismMonStore_MQCONN_BYTES 			= 9,   	//MQBridgeMemBytes
    ismMonStore_CLIENTSTATE_BYTES 		= 10,   	//ClientStateMemBytes
    ismMonStore_MEMORY_TOTAL_BYTES		= 11,   	//MemoryTotalBytes
    ismMonStore_POOL1_RECORD_SIZE		= 12,
    ismMonStore_POOL1_TOTAL_BYTES		= 13,
    ismMonStore_POOL1_USED_BYTES		= 14,
    ismMonStore_POOL1_REC_LIMIT_BYTES	= 15,
    ismMonStore_POOL1_REC_USED_BYTES	= 16,
    ismMonStore_POOL2_TOTAL_BYTES		= 17,
    ismMonStore_POOL2_USED_BYTES		= 18,
    ismMonStore_POOL1_USED_PERCENT		= 19,
    ismMonStore_POOL2_USED_PERCENT		= 20,

    
    
} ismMonitoringStoreDataType_t;

/*Get Memory Data Type*/
static int getMemDataType(char *type) {

	
	if (!strcasecmp(type, "StoreMemUsagePct") || !strcasecmp(type, "MemoryUsedPercent") )
		return ismMonStore_MEM_USAGE_PCT;
	if (!strcasecmp(type, "StoreDiskUsagePct") || !strcasecmp(type, "DiskUsedPercent"))
		return ismMonStore_DISK_USAGE_PCT;
	if (!strcasecmp(type, "DiskFreeSpaceBytes") || !strcasecmp(type, "DiskFreeBytes"))
		return ismMonStore_DISK_FREE_SPACE_BYTES;
	if (!strcasecmp(type, "IncomingMessageAcksBytes"))
		return ismMonStore_IN_MSG_ACK_BYTES;
	if (!strcasecmp(type, "ClientStatesBytes"))
		return ismMonStore_CLIENTSTATE_BYTES;
	if (!strcasecmp(type, "QueuesBytes"))
		return ismMonStore_QUEUES_BYTES;
	if (!strcasecmp(type, "SubscriptionsBytes"))
		return ismMonStore_SUBS_BYTES;
	if (!strcasecmp(type, "TopicsBytes"))
		return ismMonStore_TOPICS_BYTES;
	if (!strcasecmp(type, "TransactionsBytes"))
		return ismMonStore_TRANSACTIONS_BYTES;
	if (!strcasecmp(type, "MQConnectivityBytes"))
		return ismMonStore_MQCONN_BYTES;
	if (!strcasecmp(type, "MemoryTotalBytes"))
		return ismMonStore_MEMORY_TOTAL_BYTES;
	if (!strcasecmp(type, "MemoryTotalBytes"))
		return ismMonStore_MEMORY_TOTAL_BYTES;
	if (!strcasecmp(type, "Pool1RecordSize"))
		return ismMonStore_POOL1_RECORD_SIZE;
	if (!strcasecmp(type, "Pool1TotalBytes"))
		return ismMonStore_POOL1_TOTAL_BYTES;
	if (!strcasecmp(type, "Pool1UsedBytes"))
		return ismMonStore_POOL1_USED_BYTES;
	if (!strcasecmp(type, "Pool1RecordsLimitBytes"))
		return ismMonStore_POOL1_REC_LIMIT_BYTES;
	if (!strcasecmp(type, "Pool1RecordsUsedBytes"))
		return ismMonStore_POOL1_REC_USED_BYTES;
	if (!strcasecmp(type, "Pool2TotalBytes"))
		return ismMonStore_POOL2_TOTAL_BYTES;
	if (!strcasecmp(type, "Pool2UsedBytes"))
		return ismMonStore_POOL2_USED_BYTES;
	if (!strcasecmp(type, "Pool1UsedPercent"))
		return ismMonStore_POOL1_USED_PERCENT;
	if (!strcasecmp(type, "Pool2UsedPercent"))
		return ismMonStore_POOL2_USED_PERCENT;
	
	return ismMonStore_NONE;
}

/*Get Memory Historical data and set the output into the output_buffer for return*/
static int32_t getStoreHistoryStats(ism_monitoring_snap_t *list, char * types, int duration, ism_snaptime_t interval, concat_alloc_t * output_buffer) 
{
	ism_snapshot_range_t *sp;
	int query_count = 0;
	char buf[4096];
	int rc = ISMRC_OK;
	int i = 0;
	ismMonitoringHistoryData_t histResData[128];
	int count=0;
	
    /*
	 * calculate how many noded need to be returned.
	 */
	
	int master_count = duration/interval + 1;
	query_count = master_count;
	
	int typesLen = strlen(types);
	char * typesTmp = alloca(typesLen+1);
	memcpy(typesTmp, types, typesLen);
	typesTmp[typesLen]='\0';
	
	int typeCount=0;
	char * typeToken = ism_common_getToken(typesTmp, " \t,", " \t,", &typesTmp);
	while (typeToken) {
			histResData[typeCount].typeStr=typeToken;
			histResData[typeCount].type=getMemDataType(typeToken);
			histResData[typeCount].darray = alloca(query_count*sizeof(int64_t));
			for (i = 0; i< master_count; i++)
				histResData[typeCount].darray[i] = -1;
		 	typeCount++;
		 	typeToken = ism_common_getToken(typesTmp, " \t,", " \t,", &typesTmp);
	}


	for(count=0; count < typeCount; count++){
		ismMonitoringHistoryData_t history = histResData[count];		/* BEAM suppression: uninitialized */
		
		if(history.type!=ismMonStore_NONE){
			sp = list->range_list;
			while(sp) {
				if (query_count > sp->node_count) {
			    	query_count = sp->node_count;
			    }
				pthread_spin_lock(&sp->snplock);
				ism_snapshot_data_node_t * dataNode = sp->data_nodes;
			    ism_mondata_store_t * mdata = NULL;
			    if (!dataNode) {
			        pthread_spin_unlock(&sp->snplock);
		
			    	rc = ISMRC_NullPointer;
			    	TRACE(1, "Monitoring: getAllfromList cannot find the monitoring data.\n");
			    	
			    	int xlen;
					char  msgID[12];
					ism_monitoring_getMsgId(6518, msgID);
					char  lbuf[1024];
    				const char * repl[3];
					char mtmpbuf[1024];
					if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
						repl[0]="store";
						ism_common_formatMessage(mtmpbuf, sizeof(mtmpbuf), lbuf, repl, 1);
					} else {
						sprintf(mtmpbuf, "The store historical statistics is not available.");
					}
			    	
			    	snprintf(buf, sizeof(buf), "{ \"RC\":\"%d\", \"ErrorString\":\"%s\" }", rc, mtmpbuf);
			    	ism_common_allocBufferCopyLen(output_buffer,buf,strlen(buf));
			    	output_buffer->used = strlen(output_buffer->buf);
			    	return rc;
			    }
			    
			    switch(history.type) {
		        case ismMonStore_MEM_USAGE_PCT:
		          	for (i= 0; i < query_count; i++) {
		          		mdata = (ism_mondata_store_t *)dataNode->data;
		          		if(mdata->StoreMemUsagePct>-1)
		          			history.darray[i] = mdata->StoreMemUsagePct;
		            	dataNode = dataNode->next;
		            }
		            break;
		        case ismMonStore_DISK_USAGE_PCT:
		        	for (i= 0; i < query_count; i++) {
		        		mdata = (ism_mondata_store_t*)dataNode->data;
		        		if(mdata->StoreDiskUsagePct>-1)
		        	    	history.darray[i] = mdata->StoreDiskUsagePct;
		        	    dataNode = dataNode->next;
		        	}
		        	break;
			    case ismMonStore_DISK_FREE_SPACE_BYTES:
		            for (i= 0; i < query_count; i++) {
		            	mdata = (ism_mondata_store_t *)dataNode->data;
		            	if(mdata->DiskFreeSpaceBytes>-1)
		            		history.darray[i] = mdata->DiskFreeSpaceBytes;
		            	dataNode = dataNode->next;
		            }
		            break;
			    case ismMonStore_IN_MSG_ACK_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->IncomingMessageAcksBytes>-1)
							history.darray[i] = mdata->IncomingMessageAcksBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_CLIENTSTATE_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->ClientStatesBytes>-1)
							history.darray[i] = mdata->ClientStatesBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_MEMORY_TOTAL_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->MemoryTotalBytes>-1)
							history.darray[i] = mdata->MemoryTotalBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_MQCONN_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->MQConnectivityBytes>-1)
							history.darray[i] = mdata->MQConnectivityBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_QUEUES_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->QueuesBytes>-1)
							history.darray[i] = mdata->QueuesBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_SUBS_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->SubscriptionsBytes>-1)
							history.darray[i] = mdata->SubscriptionsBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_TOPICS_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->TopicsBytes>-1)
							history.darray[i] = mdata->TopicsBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_TRANSACTIONS_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->TransactionsBytes>-1)
							history.darray[i] = mdata->TransactionsBytes;
						dataNode = dataNode->next;
					}
					break;

			   case ismMonStore_POOL1_TOTAL_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->Pool1TotalBytes>-1)
							history.darray[i] = mdata->Pool1TotalBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_POOL1_USED_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->Pool1UsedBytes>-1)
							history.darray[i] = mdata->Pool1UsedBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_POOL1_REC_LIMIT_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->Pool1RecordsLimitBytes>-1)
							history.darray[i] = mdata->Pool1RecordsLimitBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_POOL1_REC_USED_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->Pool1RecordsUsedBytes>-1)
							history.darray[i] = mdata->Pool1RecordsUsedBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_POOL2_TOTAL_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->Pool2TotalBytes>-1)
							history.darray[i] = mdata->Pool2TotalBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_POOL2_USED_BYTES:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->Pool2UsedBytes>-1)
							history.darray[i] = mdata->Pool2UsedBytes;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_POOL1_USED_PERCENT:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->Pool1UsedPercent>-1)
							history.darray[i] = mdata->Pool1UsedPercent;
						dataNode = dataNode->next;
					}
					break;
			    case ismMonStore_POOL2_USED_PERCENT:
					for (i= 0; i < query_count; i++) {
						mdata = (ism_mondata_store_t *)dataNode->data;
						if(mdata->Pool2UsedPercent>-1)
							history.darray[i] = mdata->Pool2UsedPercent;
						dataNode = dataNode->next;
					}
					break;

		       	default:
		       		TRACE(1, "Monitoring: getDatafromList does not support monitoring type %s\n", history.typeStr);
		       		break;
		        }
		
			    pthread_spin_unlock(&sp->snplock);
				sp = sp->next;
				query_count = master_count;
			}
		}
	}
	
	
	/*Construction the Header for the Response*/
	
	memset(buf, 0, sizeof(buf));
    sprintf(buf, "{ \"Duration\":%d, \"Interval\":%llu", duration, (ULL)interval);
    ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
    
    
    /*
     * print out the darray
     */
    for(count=0; count < typeCount; count++){
		ismMonitoringHistoryData_t history = histResData[count];		/* BEAM suppression: uninitialized */
		
		memset(buf, 0, sizeof(buf));
		sprintf(buf, ", \"%s\":\"",history.typeStr);
        ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
		
		memset(buf, 0, sizeof(buf));
	    char *p = buf;
	    for (i= 0; i < master_count; i++) {
	    	if (i > 0)
	    	    p += sprintf(p, ",");
	    	p += sprintf(p, "%ld", history.darray[i]);
	    	if( p - buf > (sizeof(buf)-512)) {
	    	    ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
	    	    memset(buf, 0, sizeof(buf));
	    	    p = buf;
	    	}
	    }
	    
	    if (p != buf)
	        ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
	
	    /*
	     * Add the end of the json string
	     */
	    sprintf(buf, "\"");
	    ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
		
	}
   
    sprintf(buf, "}");
	ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
  

	return ISMRC_OK;
}
/*******************************************************END SNAPSHOT OF STORE***********************/
