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
#include <perfstat.h>
#include <ismjson.h>
#include <security.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ismutil.h>
#include <transport.h>
#include <monitoringutil.h>
#include <janssonConfig.h>

extern ism_ts_t *monEventTimeStampObj;
extern pthread_spinlock_t monEventTimeStampObjLock;
extern pthread_key_t monitoring_localekey;

/*
 * Holding monitoring data for each snap shot.
 */
struct ism_mondata_t {
    uint8_t                  enabled;             /**< Is enabled                   */
    uint32_t                 errcode;             /**< Last error code              */
    uint64_t				 timestamp;			  /**< Timestamp when the data is collected. */
    uint64_t                 connect_active;      /**< Currently active connections */
    uint64_t                 connect_count;       /**< Connection count since reset */
    uint64_t                 bad_connect_count;   /**< Count of connections which have failed to connect since reset */
    uint64_t                 lost_msg_count;      /**< Count of messages lost for size or queue depth */
    uint64_t                 warn_msg_count;      /**< Count of messages lost for size or queue depth for some but not all destinations */
    uint64_t                 read_msg_count;      /**< Message count since reset    */
    uint64_t                 read_bytes_count;    /**< Bytes count since reset      */
    uint64_t                 write_msg_count;     /**< Message count since reset    */
    uint64_t                 write_bytes_count;   /**< Bytes count since reset      */
    struct ism_mondata_t  *  prev;
    struct ism_mondata_t  *  next;
};

/*
 * The snap shot range object is used to monitoring an endpoint object
 * Throughput will be calculated using bytes_count/interval
 */
struct ism_srange_t {
    const char *          name;              /**< Endpoint name                */
    const char *          ipaddr;            /**< IP address or null for any   */
    uint64_t              node_count;        /**< Count of nodes in the mondata list             */
    uint64_t              node_idle;         /**< Count of nodes do not updated. If it is exceed max_count, it will be removed */
    ism_mondata_t *       monitoring_data;   /**< A double link list holding the monitroing data */
    ism_mondata_t *       tail;              /**< A point to the last node of monitoring_data    */
    pthread_spinlock_t    snplock;           /**< Lock over snapshots                            */
    struct ism_srange_t * next;               /**< next endpoint                                 */
};


/*
 * monitoring object.
 * It will contains all aggregate data for an endpoint. It is up to the GUI/CLI to define
 * the intervals for each snapshot.
 * TODO: History data?
 */
struct ism_monitoring_t {
	ism_snaptime_t   last_snap_shot;      /**< Time in second of last snapshot    */
	ism_snaptime_t   snap_interval;       /**< Time in second of update interval  */
    uint64_t         max_count;           /**< Maximum count of nodes in the mondata list     */
    ism_srange_t     * ept_list;          /**< List of the endpoints in the monitoring obj    */
};


/*
 * The main monitoring List that hold all the monitoring aggregation data
 */
struct ism_monitoringList_t {
	ism_monitoring_t   **monlist;      /**< Time in second of last snapshot    */
	int                numOfList;
};

/*
 * The main list of the endpoints to be monitored.
 */
static ism_monitoringList_t * monitoringList = NULL;
static int32_t getDatafromList(ism_monitoring_t *list, const char * name, char * type, int duration, ism_snaptime_t interval, concat_alloc_t * output_buffer);


/* Listener Monitoring Data for External Monitoring*/
int ism_monitoring_getListenerMonitoringDataExternal(ism_endpoint_mon_t * mon ,  uint64_t currentTime, concat_alloc_t *output_buffer) {
    char rbuf[10240];
	int rc = ISMRC_OK;


	/* get listener monitoring data and add to output_buffer */
    TRACE(9, "Get listener stats for %s for external monitoring\n", mon->name);

	char msgPrefixBuf[256];
	concat_alloc_t prefixbuf = { NULL, 0, 0, 0, 0 };
	prefixbuf.buf = (char *) &msgPrefixBuf;
	prefixbuf.len = sizeof(msgPrefixBuf);


	ism_monitoring_getMsgExternalMonPrefix(ismMonObjectType_Endpoint,
										 currentTime,
										 NULL,
										 &prefixbuf);

	char  * msgPrefix = alloca(prefixbuf.used+1);
	memcpy(msgPrefix, prefixbuf.buf, prefixbuf.used);
	msgPrefix[prefixbuf.used]='\0';

	sprintf(rbuf, "{ %s, \"Name\":", msgPrefix);
	ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));

    /* add endpoint name */
	ism_json_putString(output_buffer,(const char *) mon->name);

	/*Format Timestamp*/
	char *timeValue = NULL;
	char tbuffer[80];
	if ( monEventTimeStampObj != NULL) {
		/*Lock the timestamp object because this object will be used from multiple threads*/
		pthread_spin_lock(&monEventTimeStampObjLock);
        ism_common_setTimestamp(monEventTimeStampObj, (ULL)mon->config_time);
        ism_common_formatTimestamp(monEventTimeStampObj, tbuffer, 80, 0, ISM_TFF_ISO8601);
        pthread_spin_unlock(&monEventTimeStampObjLock);
        timeValue = tbuffer;
    }

	/* add rest of the data */
	if (timeValue != NULL) {
		sprintf( rbuf, ", \"Interface\":\"%s\", \"Enabled\":%s, \"TotalConnections\":%llu, \"ActiveConnections\":%llu, \"BadConnections\":%llu, \"MsgRead\":%llu, \"MsgWrite\":%llu, \"BytesRead\":%llu, \"BytesWrite\":%llu, \"LostMessageCount\":%llu, \"WarnMessageCount\":%llu, \"ResetTime\":\"%s\" }",
				mon->ipaddr?mon->ipaddr:"All", mon->enabled?"true":"false", (ULL)mon->connect_count, (ULL)mon->connect_active, (ULL)mon->bad_connect_count, (ULL)mon->read_msg_count, (ULL)mon->write_msg_count, (ULL)mon->read_bytes_count, (ULL)mon->write_bytes_count, (ULL)mon->lost_msg_count, (ULL)mon->warn_msg_count, timeValue );
	} else {
		sprintf( rbuf, ", \"Interface\":\"%s\", \"Enabled\":%s, \"TotalConnections\":%llu, \"ActiveConnections\":%llu, \"BadConnections\":%llu, \"MsgRead\":%llu, \"MsgWrite\":%llu, \"BytesRead\":%llu, \"BytesWrite\":%llu, \"LostMessageCount\":%llu, \"WarnMessageCount\":%llu, \"ResetTime\":null }",
				mon->ipaddr?mon->ipaddr:"All", mon->enabled?"true":"false", (ULL)mon->connect_count, (ULL)mon->connect_active, (ULL)mon->bad_connect_count, (ULL)mon->read_msg_count, (ULL)mon->write_msg_count, (ULL)mon->read_bytes_count, (ULL)mon->write_bytes_count, (ULL)mon->lost_msg_count, (ULL)mon->warn_msg_count);
	}

	ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));

	if(prefixbuf.inheap) ism_common_freeAllocBuffer(&prefixbuf);

	return rc;
}



/* Listener Monitoring Data */
int ism_monitoring_getListenerMonitoringData(const char *name, concat_alloc_t *output_buffer) {
    int rbufSize = 1024;
	char * rbuf = alloca(rbufSize);
	ism_listener_mon_t * monlist;
	int nrec = 0;
	int rc = ISMRC_OK;
	int found = 0;
	int isBracketSet=0;

	/* get listener monitoring data and add to output_buffer */
    TRACE(9, "Get Listener stats for %s\n", name?name:"N/A");

	if ( (nrec = ism_transport_getListenerMonitor(&monlist, NULL)) > 0 ) {
        TRACE(9, "Received Listeren stats: %d\n", nrec);
		int i = 0;
		int addNext = 0;
		ism_listener_mon_t *mon = monlist;
		for (i=0; i<nrec; i++) {
			if (name && *name!= '\0')  {
					if(strcmp(name, mon->name)) {
						if(i<nrec) mon++;
						continue;
					}
			}

			if ( !strcmp(mon->name, "!Forwarder")) {
				if(i<nrec) mon++;
				continue;
			}

			if( isBracketSet == 0){
				sprintf(rbuf,"[ ");
				ism_common_allocBufferCopyLen(output_buffer,rbuf,strlen(rbuf));
				isBracketSet = 1;
			}
			if ( addNext == 1 ) {
				sprintf( rbuf, ", \n");
				ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
			}
			/* add name, connect_active, connect_count, msg_count, bytes_count */
			uint64_t msgCount = mon->read_msg_count + mon->write_msg_count;
			uint64_t bytesCount = mon->read_bytes_count + mon->write_bytes_count;

			/* TODO; escape endpoint name */

		    /*
		     * Add the end of the json string
		     * LastUpdateTimestamp in ISO8601
		     */
		    char config_time[30];
		    char reset_time[30];
		    ism_ts_t *ts =  ism_common_openTimestamp(ISM_TZF_LOCAL);
		    ism_common_setTimestamp(ts, mon->config_time);
		    ism_common_formatTimestamp(ts, config_time, 30, 0, ISM_TFF_ISO8601);
		    if (mon->reset_time) {
		    	ism_common_setTimestamp(ts, mon->reset_time);
		    	ism_common_formatTimestamp(ts, reset_time, 30, 0, ISM_TFF_ISO8601);
		    }
		    ism_common_closeTimestamp(ts);

			int l;
			if (mon->reset_time) {
				l = snprintf( rbuf, rbufSize, "{ \"Name\":\"%s\",\"IPAddr\":\"%s\",\"Enabled\":%s,\"Total\":%llu,\"Active\":%llu,\"Messages\":%llu,\"Bytes\":%llu,\"LastErrorCode\":%u,\"ConfigTime\":\"%s\",\"ResetTime\":\"%s\",\"BadConnections\":%llu }\n",
				    mon->name, mon->ipaddr?mon->ipaddr:"ALL", mon->enabled?"true":"false", (ULL)mon->connect_count, (ULL)mon->connect_active, (ULL)msgCount, (ULL)bytesCount, mon->errcode, config_time, reset_time, (ULL)mon->bad_connect_count);
			} else {
				l = snprintf( rbuf, rbufSize, "{ \"Name\":\"%s\",\"IPAddr\":\"%s\",\"Enabled\":%s,\"Total\":%llu,\"Active\":%llu,\"Messages\":%llu,\"Bytes\":%llu,\"LastErrorCode\":%u,\"ConfigTime\":\"%s\",\"ResetTime\":null,\"BadConnections\":%llu }\n",
				    mon->name, mon->ipaddr?mon->ipaddr:"ALL", mon->enabled?"true":"false", (ULL)mon->connect_count, (ULL)mon->connect_active, (ULL)msgCount, (ULL)bytesCount, mon->errcode, config_time, (ULL)mon->bad_connect_count);
			}
			if (l + 1 > rbufSize) {
				rbufSize = l + 1;
				rbuf = alloca(rbufSize);
				if (mon->reset_time) {
					sprintf( rbuf, "{ \"Name\":\"%s\",\"IPAddr\":\"%s\",\"Enabled\":%s,\"Total\":%llu,\"Active\":%llu,\"Messages\":%llu,\"Bytes\":%llu,\"LastErrorCode\":%u,\"ConfigTime\":\"%s\",\"ResetTime\":\"%s\",\"BadConnections\":%llu }\n",
									    mon->name, mon->ipaddr?mon->ipaddr:"ALL", mon->enabled?"true":"false", (ULL)mon->connect_count, (ULL)mon->connect_active, (ULL)msgCount, (ULL)bytesCount, mon->errcode, config_time, reset_time, (ULL)mon->bad_connect_count);
				} else {
					sprintf( rbuf, "{ \"Name\":\"%s\",\"IPAddr\":\"%s\",\"Enabled\":%s,\"Total\":%llu,\"Active\":%llu,\"Messages\":%llu,\"Bytes\":%llu,\"LastErrorCode\":%u,\"ConfigTime\":\"%s\",\"ResetTime\":null,\"BadConnections\":%llu }\n",
									    mon->name, mon->ipaddr?mon->ipaddr:"ALL", mon->enabled?"true":"false", (ULL)mon->connect_count, (ULL)mon->connect_active, (ULL)msgCount, (ULL)bytesCount, mon->errcode, config_time, (ULL)mon->bad_connect_count);
				}
			}

			ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
			addNext = 1;
			found++;
			mon++;
		}
		ism_transport_freeEndpointMonitor(monlist);
		if( isBracketSet == 1 ) {
			sprintf(rbuf, " ]");
			ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
		}
	}
	if (found == 0) {
		rc = ISMRC_EndpointNotFound;
		char ebuf[2048];
		char *errstr = (char *)ism_common_getErrorStringByLocale(rc, ism_common_getRequestLocale(monitoring_localekey), ebuf, sizeof(ebuf));
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


static ism_monitoring_t * ism_monitoring_getListByInterval(ism_snaptime_t snap_interval)  {
	int i;
	ism_monitoringList_t * p = monitoringList;

	if (!p) {
	    TRACE(9, "Monitoring: monitoring list has not been initialed\n");
		return NULL;
	}
	for (i = 0; i < p->numOfList; i++) {
	    if ((p->monlist[i])->snap_interval == snap_interval)
			   return p->monlist[i];
	}
	return NULL;
}

static int createNewMonList(ism_monitoring_t ** monlist, ism_snaptime_t snap_interval) {
	(*monlist)                 = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,39),sizeof(ism_monitoring_t));
	if (!(*monlist)) {
		TRACE(9, "Monitoring: monitoring list initial failed on memory allocation\n");
	    return ISMRC_AllocateError;
	}
	(*monlist)->last_snap_shot = ism_monitoring_currentTimeSec();
	(*monlist)->snap_interval  = snap_interval;
	if (snap_interval == DEFAULT_SNAPSHOTS_SHORTRANGE_INTERVAL)
	    (*monlist)->max_count      = 60*60/snap_interval +1;

	else if (snap_interval == DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL)
		(*monlist)->max_count      = 24*60*60/snap_interval + 1;
	(*monlist)->ept_list       = NULL;
	return ISMRC_OK;
}
/*
 * MonitoringList will be hardcoded for now.
 * 2 sets of aggregate data list will be stored in the memory for monitoring thread:
 *   1.  The last 1 hour data for all endpoints (include a special overall one). The data will be colleceted every 5 seconds
 *   2.  The last 24 hour data for all endpoints (include a special overall one). The data will be collected every minute.
 * Once the list reach its max_count, no more memory will be allocated and the earliest data will be recycled.
 *
 *  It will provide a fine grain (at least 100 point/per show) for planned GUI.
 *  GUI will call:
 *  ism_monitoring_getData(const char * name, ism_snaptime_t interval, ism_snaptime_t start, ism_snaptime_t end, int points)
 *  to get an array of data with size of points.
 *  The finest grain will be 5 sec.
 */
XAPI int ism_monitoring_initMonitoringList(void)  {
	int i;

	monitoringList = (ism_monitoringList_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,40),sizeof(ism_monitoringList_t)*NUM_MONITORING_LIST);
	monitoringList->numOfList = NUM_MONITORING_LIST;
	monitoringList->monlist = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,41),sizeof(ism_monitoring_t *)*NUM_MONITORING_LIST);
	for (i = 0; i < NUM_MONITORING_LIST; i++) {
		if(i == 0)
		    createNewMonList(&(monitoringList->monlist[i]), DEFAULT_SNAPSHOTS_SHORTRANGE_INTERVAL);
		else if(i == 1)
			createNewMonList(&(monitoringList->monlist[i]), DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL);
		else {
			TRACE(9, "Monitoring: monitoring list initial failed\n");
		    return ISMRC_NotImplemented;
		}
	}
	return ISMRC_OK;
}


static ism_mondata_t * newMonDataNode(void) {
	ism_mondata_t *nmd = (ism_mondata_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,42),sizeof(ism_mondata_t));
	if (!nmd) {
		TRACE(9, "Monitoring: ism monitoring data memory allocation failed\n");
		return NULL;
	}
    nmd->enabled = 0;
    nmd->errcode = 0;
    nmd->connect_active    = 0;    /**< Currently active connections */
    nmd->connect_count     = 0;    /**< Connection count since reset */
    nmd->bad_connect_count = 0;    /**< Count of connections which have failed to connect since reset */
    nmd->lost_msg_count    = 0;    /**< Count of messages lost for size or queue depth */
    nmd->warn_msg_count    = 0;    /**< Count of messages lost for size or queue depth for some but not all destinations */
    nmd->read_msg_count    = 0;    /**< Message count since reset    */
    nmd->read_bytes_count  = 0;    /**< Bytes count since reset      */
    nmd->write_msg_count   = 0;    /**< Message count since reset    */
    nmd->write_bytes_count = 0;    /**< Bytes count since reset      */
    nmd->timestamp 		   = 0;
    nmd->prev = NULL;
    nmd->next = NULL;

    return nmd;
}

static int freeSRange(ism_srange_t * node) {
	ism_mondata_t * tp, * mp;
	int count= 0;

	if (!node) {
		TRACE(9, "Monitoring: nothing to be freed\n");
		return ISMRC_NullArgument;
	}
	pthread_spin_lock(&node->snplock);
	tp = node->monitoring_data;
	while(tp) {
		mp = tp;
		tp = tp->next;
		ism_common_free(ism_memory_monitoring_misc,mp);
		count++;
	}
	if (count != node->node_count) {
		TRACE(9, "Monitoring: inconsistence of node_count %llu and actual count %d\n", (unsigned long long)node->node_count, count);
	}
	if (node->name)
	    ism_common_free(ism_memory_monitoring_misc,(void*)node->name);
	if (node->ipaddr)
	    ism_common_free(ism_memory_monitoring_misc,(void *)node->ipaddr);
	pthread_spin_unlock(&node->snplock);
	node->next = NULL;
	ism_common_free(ism_memory_monitoring_misc,node);

	return ISMRC_OK;
}

/*
 * remove expired endpoint and assoicated mondata from endpoint list
 */
static int cleanExpiredEpt(ism_srange_t * ept_list, uint64_t max_count) {
	ism_srange_t * tmp = NULL, *p, *s;

	/*
	 * First clean all unused srange_t object.
	 */
	while(ept_list && (ept_list->node_idle >= max_count)) {
		tmp = ept_list;
		ept_list=ept_list->next;
		freeSRange(tmp);
	}

	p = ept_list;
	if (p) {
	    tmp=ept_list->next;
	} else {
		tmp = NULL;
	}
	while(tmp) {
        //TRACE(9, "Monitoring: clean an expired endpoint:%s\n", tmp->name);
        if(tmp->node_idle >= max_count) {
        	s = tmp;
        	tmp = tmp->next;
        	p->next = tmp;
        	freeSRange(s);
        }
        p = tmp;
        tmp = tmp->next;
	}
	return ISMRC_OK;
}
static int createNewNode(ism_srange_t *sp) {
	ism_mondata_t *md = newMonDataNode();
	if (!md) {
		return ISMRC_AllocateError;
	}
	pthread_spin_lock(&sp->snplock);
	if (!sp->monitoring_data) {
		sp->monitoring_data = md;
	} else {
	   sp->monitoring_data->prev = md;
	   md->next = sp->monitoring_data;
	   sp->monitoring_data = md;
	}
	sp->node_idle++;
	sp->node_count++;
	pthread_spin_unlock(&sp->snplock);
   // TRACE(9, "Monitoring: createNewNode, node_idle=%llu, node_count=%llu\n", (unsigned long long)sp->node_idle, (unsigned long long)sp->node_count );
	return ISMRC_OK;
}

static int reuseLastNode(ism_srange_t *sp) {
	//TRACE(9, "reuse last node\n");
	pthread_spin_lock(&sp->snplock);
	/*
	 * The monitoring data reaches all the slots, the last slot will be reused.
	 */
	ism_mondata_t * node = sp->tail;
	sp->tail = node->prev;
	(sp->tail)->next = NULL;   //new tail


	node->enabled = 0;
	node->errcode = 0;

	node->connect_active    = 0;    /**< Currently active connections */
	node->connect_count     = 0;    /**< Connection count since reset */
	node->bad_connect_count = 0;    /**< Count of connections which have failed to connect since reset */
	node->lost_msg_count    = 0;    /**< Count of messages lost for size or queue depth */
	node->warn_msg_count    = 0;    /**< Count of messages lost for size or queue depth for some but not all destinations */
	node->read_msg_count    = 0;    /**< Message count since reset    */
	node->read_bytes_count  = 0;    /**< Bytes count since reset      */
	node->write_msg_count   = 0;    /**< Message count since reset    */
	node->write_bytes_count = 0;    /**< Bytes count since reset      */
	node->timestamp 		= 0;
	node->next = sp->monitoring_data;
	sp->monitoring_data->prev = node;
	node->prev = NULL;
	sp->monitoring_data = node;

	pthread_spin_unlock(&sp->snplock);
	return ISMRC_OK;
}
/*
 * A new(reused) ism_mondata_t node needs to be initialed as an idle node in case some endpoints can not be updated during the snapshot (disabled temporary etc.).
 *  -- clean all expired endpoints -- the endpoints in idle mode in all its nodes. no reason to keep it.
 *  -- depends on the max_count, decided if a new node should be added or reused the earliest node.
 */
static int initMonDataNode(ism_monitoring_t * slist, ism_monitoring_t * llist) {
	ism_srange_t *p;
	int rc;

	if (!slist) {
		TRACE(9, "Monitoring: initMonDataNode short list is NULL\n");
		return ISMRC_NotFound;
	}
	p = slist->ept_list;

	/*
	 * First clean all unused srange_t object.
	 */
    if (!p)   {
    	return ISMRC_NotFound;
    }
	rc = cleanExpiredEpt(p, slist->max_count);
	if (rc)
		return rc;


	/*
	 * Adding a new node to all the endpoints' mondata list.
	 */
	while(p) {
		if (p->node_count < slist->max_count) {
			rc = createNewNode(p);
		} else {
			rc = reuseLastNode(p);
		}
		if (rc)
			return rc;
		p = p->next;
	}

	//init 24H list
	if (llist) {
		p = NULL;
	    p = llist->ept_list;
	    if (!p)   {
	    	return ISMRC_NotFound;
	    }
	    cleanExpiredEpt(p, llist->max_count);
		if (rc)
			return rc;

		/*
		 * Adding a new node to all the endpoints' mondata list.
		 */
		while(p) {
			if (p->node_count < llist->max_count) {
				rc = createNewNode(p);
			} else {
				rc = reuseLastNode(p);
			}
			if (rc)
				return rc;
			p = p->next;
		}
	}
	return ISMRC_OK;
}

/*
 * The new node has already been allocated or resigned as the first in the list in initNewData
 */
static int storeEptMonData(ism_srange_t ** list, ism_listener_mon_t * mon, ism_time_t timestamp) {
	ism_srange_t *sp;

    if (!mon || !mon->name) {
    	TRACE(9, "Monitoring: in storeEptMonData, endpoint or endpoint's name is NULL\n");
    	return ISMRC_NullPointer;
    }

    // TRACE(9, "Monitoring: in storeEptMonData, endpoint name is: %s\n", mon->name);
	sp = *list;
	while(sp) {
		//TRACE(9, "Monitoring: in storeEptMonData, current endpoint name on the list is: %s\n", sp->name);
		if (sp->name && !strcmp(sp->name, mon->name)) {
			/*TODO
			 *
			 */
			pthread_spin_lock(&sp->snplock);
			ism_mondata_t * node = sp->monitoring_data;

			node->connect_active    = mon->connect_active;;    /**< Currently active connections */
			node->connect_count     = mon->connect_count;      /**< Connection count since reset */
			node->bad_connect_count = mon->bad_connect_count;  /**< Count of connections which have failed to connect since reset */
			node->lost_msg_count    = mon->lost_msg_count;     /**< Count of messages lost for size or queue depth */
			node->warn_msg_count    = mon->warn_msg_count;     /**< Count of messages lost for size or queue depth for some but not all destinations */
			node->read_msg_count    = mon->read_msg_count;     /**< Message count since reset    */
			node->read_bytes_count  = mon->read_bytes_count;   /**< Bytes count since reset      */
			node->write_msg_count   = mon->write_msg_count;    /**< Message count since reset    */
			node->write_bytes_count = mon->write_bytes_count;  /**< Bytes count since reset      */
			node->timestamp			= timestamp;
			if (sp->node_idle > 0)
                sp->node_idle--;
            pthread_spin_unlock(&sp->snplock);
			return ISMRC_OK;
		}
		sp = sp->next;
	}
	return ISMRC_NotFound;
}


/*
 * A new endpoint and associated mondata_t will be created and add to the list
 */
static int newEptMonData(ism_srange_t ** list, uint64_t max_count, ism_listener_mon_t * mon, ism_time_t timestamp) {
	ism_srange_t *ept = (ism_srange_t *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,47),sizeof(ism_srange_t));

	//TRACE(9, "Monitoring: in newEptMonData, name=%s, max_count=%llu\n", mon->name, (ULL)max_count);
	ept->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_monitoring_misc,1000),mon->name);              /**< Endpoint name                */
	if (mon->ipaddr)
	    ept->ipaddr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_monitoring_misc,1000),mon->ipaddr);            /**< IP address or null for any   */

    ept->monitoring_data = NULL;   /**< A double link list holding the monitroing data */
    ept->node_count = 0;           /**< Count of nodes in the mondata list             */
    ept->node_idle = 0;
    pthread_spin_init(&ept->snplock, 0);
    createNewNode(ept);
    ept->tail = ept->monitoring_data;              /**< A point to the last node of monitoring_data    */

    ept->next = *list;
    *list = ept;

    storeEptMonData(list, mon, timestamp);
    return ISMRC_OK;
}


static int recordSnapShot(ism_monitoring_t * slist, ism_monitoring_t * llist) {
	ism_listener_mon_t * monlist;
    int num, i, rc;

	if ( (num = ism_transport_getListenerMonitor(&monlist, NULL)) > 0 ) {
	    //TRACE(9, "Monitoring: get %d endpoints' monitoring data\n", num);
	    ism_listener_mon_t * mon = monlist;
	    ism_time_t timestamp = ism_common_currentTimeNanos();
	    for (i = 0; i< num; i++) {

	        rc = storeEptMonData(&(slist->ept_list), mon, timestamp);
	        /*
	         * Can not find the endpoint in current list, need to create a new one and add it to the ept_list
	         */
	        if (rc == ISMRC_NotFound) {
	        	newEptMonData(&(slist->ept_list), slist->max_count,  mon, timestamp);
	        }

	        if (llist) {
	        	rc = storeEptMonData(&(llist->ept_list), mon,timestamp);
		        /*
		         * Can not find the endpoint in current list, need to create a new one and add it to the ept_list
		         */
		        if (rc == ISMRC_NotFound) {
		        	newEptMonData(&(llist->ept_list), llist->max_count, mon, timestamp);
		        }
	        }
	        mon++;
	    }
	    ism_transport_freeEndpointMonitor(monlist);

	}
	return ISMRC_OK;
}

/*
 * Check if a snapshot is needed at the time.
 */
XAPI int ism_monitoring_checkAction(ism_snaptime_t curtime) {
	int rc = ISMRC_OK;


	ism_monitoring_t * slist = ism_monitoring_getListByInterval(DEFAULT_SNAPSHOTS_SHORTRANGE_INTERVAL);
	ism_monitoring_t * llist = ism_monitoring_getListByInterval(DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL);

    //TRACE(9, "Monitoring: checkAction, current time:%llu, last time:%llu\n", (ULL)curtime,  (ULL)llist->last_snap_shot);
	if (curtime - llist->last_snap_shot >= llist->snap_interval) {
		rc = initMonDataNode(slist, llist);
		if (rc && rc != ISMRC_NotFound)
			return rc;
		rc = recordSnapShot(slist, llist);
		if (rc)
			return rc;
	    slist->last_snap_shot = curtime;
	    llist->last_snap_shot = curtime;

	} else {
		rc = initMonDataNode(slist, NULL);
		if (rc && rc != ISMRC_NotFound)
			return rc;
		rc = recordSnapShot(slist, NULL);
		if (rc)
			return rc;
	    slist->last_snap_shot = curtime;
	}
	return rc;
}

static int getDataType(char *type) {
	if (!strcasecmp(type, "ActiveConnections"))
		return ismMon_CONNECT_ACTIVE;
	if (!strcasecmp(type, "Connections"))
		return ismMon_CONNECT_COUNT;
	if (!strcasecmp(type, "BadConnections"))
		return ismMon_BAD_CONNECT_COUNT;
	if (!strcasecmp(type, "LostMsgs"))
		return ismMon_LOST_MSG_COUNT;
	if (!strcasecmp(type, "WarnMsgs"))
		return ismMon_WARN_MSG_COUNT;
	if (!strcasecmp(type, "ReadMsgs"))
		return ismMon_READ_MSG_COUNT;
	if (!strcasecmp(type, "WriteMsgs"))
		return ismMon_WRITE_MSG_COUNT;
	if (!strcasecmp(type, "ReadBytes"))
	    return ismMon_READ_BYTES_COUNT;
	if (!strcasecmp(type, "WriteBytes"))
		return ismMon_WRITE_BYTES_COUNT;
	if (!strncasecmp(type, "enabled", 7))
		return ismMon_ENABLED;
	if (!strncasecmp(type, "errcode", 7))
		return ismMon_ERR_CODE;

	return ismMon_NONE;
}

static int32_t getDatafromList(ism_monitoring_t *list, const char * name, char * type, int duration, ism_snaptime_t interval, concat_alloc_t * output_buffer) {
	ism_srange_t *sp;
	int query_count = 0;
	int remain_count = 0;
	char buf[4096];
	int rc = ISMRC_OK;
	 const char * repl[3];
	char  msgID[12];
    char tmpbuf[2048];
    char  lbuf[2048];
    memset(lbuf, 0, sizeof(lbuf));
    int xlen;
    ism_time_t timestamp=0;

	memset(buf, 0, sizeof(buf));
	sp = list->ept_list;
	while(sp) {
		if (name && !strcmp(sp->name, name)) {
            break;
		}
		sp = sp->next;
	}
	if (!sp) {
		rc = ISMRC_NotFound;
		TRACE(5, "Monitoring: getDatafromList cannot find the Endpoint :%s\n", name?name:"");

		ism_monitoring_getMsgId(6514, msgID);

        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]=name?name:"";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "The Endpoint \"%s\" cannot be found.", name?name:"");
        }
        ism_monitoring_setReturnCodeAndStringJSON(output_buffer, rc, tmpbuf);


		return rc;
	}

    int montype=getDataType(type);

    char *p = buf;

    pthread_spin_lock(&sp->snplock);
    ism_mondata_t * mdata = sp->monitoring_data;
    if (!mdata) {
        pthread_spin_unlock(&sp->snplock);

    	rc = ISMRC_NullPointer;
    	TRACE(9, "Monitoring: getDatafromList cannot find the monitoring data for the endpoint %s\n", name);

    	ism_monitoring_getMsgId(6515, msgID);

        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]=name?name:"";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "Failed to get monitoring data for the Endpoint \"%s\".",name);
        }
        ism_monitoring_setReturnCodeAndStringJSON(output_buffer, rc, tmpbuf);

    	return rc;
    }
    p += sprintf(buf, "{ \"Endpoint\":\"%s\", \"Type\":\"%s\", \"Duration\":%d, \"Interval\":%llu, \"Data\":\"", name, type, duration, (ULL)interval);
    /*
     * calculate how many noded need to be returned.
     */
    query_count = duration/interval + 1;
    if (query_count > sp->node_count) {
    	remain_count = query_count - sp->node_count;
    	query_count = sp->node_count;
    }

    /*Get the timestamp of last node (or the first one on the mdata object*/
   	if(mdata->timestamp>0) timestamp  = mdata->timestamp;

    switch(montype) {
    case ismMon_CONNECT_ACTIVE:
      	for (int i= 0; i < query_count; i++) {
        	if (i > 0)
        		p += sprintf(p, ",");
        	p += sprintf(p, "%llu", (ULL)mdata->connect_active);
        	if( p - buf > (sizeof(buf)-512)) {
        		ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        		memset(buf, 0, sizeof(buf));
        		p = buf;
        	}
        	mdata = mdata->next;
        }
      	if (p != buf)
      	    ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        break;
    case ismMon_CONNECT_COUNT:
	    for (int i= 0; i < query_count; i++) {
  	        if (i > 0)
   		        p += sprintf(p, ",");
   	        p += sprintf(p, "%llu", (ULL)mdata->connect_count);
   	        if( p - buf > (sizeof(buf)-512)) {
   		        ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
   		        memset(buf, 0, sizeof(buf));
   		        p = buf;
   	        }
	        mdata = mdata->next;
        }
	    if (p != buf)
	        ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        break;
    case ismMon_BAD_CONNECT_COUNT:
        for (int i= 0; i < query_count; i++) {
        	if (i > 0)
        		p += sprintf(p, ",");
        	p += sprintf(p, "%llu", (ULL)mdata->bad_connect_count);
        	if( p - buf > (sizeof(buf)-512)) {
        		ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        		memset(buf, 0, sizeof(buf));
        		p = buf;
        	}
        	mdata = mdata->next;
        }
        if (p != buf)
            ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        break;
    case ismMon_LOST_MSG_COUNT:
        for (int i= 0; i < query_count; i++) {
        	if (i > 0)
        		p += sprintf(p, ",");
        	p += sprintf(p, "%llu", (ULL)mdata->lost_msg_count);
        	if( p - buf > (sizeof(buf)-512)) {
        		ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        		memset(buf, 0, sizeof(buf));
        		p = buf;
        	}
        	mdata = mdata->next;
        }
        if (p != buf)
            ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        break;
    case ismMon_WARN_MSG_COUNT:
            for (int i= 0; i < query_count; i++) {
            	if (i > 0)
            		p += sprintf(p, ",");
            	p += sprintf(p, "%llu", (ULL)mdata->warn_msg_count);
            	if( p - buf > (sizeof(buf)-512)) {
            		ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
            		memset(buf, 0, sizeof(buf));
            		p = buf;
            	}
            	mdata = mdata->next;
            }
            if (p != buf)
                ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
            break;
    case ismMon_READ_MSG_COUNT:
        for (int i= 0; i < query_count; i++) {
        	if (i > 0)
        		p += sprintf(p, ",");
        	p += sprintf(p, "%llu", (ULL)mdata->read_msg_count);
        	if( p - buf > (sizeof(buf)-512)) {
        		ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        		memset(buf, 0, sizeof(buf));
        		p = buf;
        	}
        	mdata = mdata->next;
        }
        if (p != buf)
            ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        break;
    case ismMon_WRITE_MSG_COUNT:
        for (int i= 0; i < query_count; i++) {
        	if (i > 0)
        		p += sprintf(p, ",");
        	p += sprintf(p, "%llu", (ULL)mdata->write_msg_count);
        	if( p - buf > (sizeof(buf)-512)) {
        		ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        		memset(buf, 0, sizeof(buf));
        		p = buf;
        	}
        	mdata = mdata->next;
        }
        if (p != buf)
            ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        break;
    case ismMon_READ_BYTES_COUNT:
        for (int i= 0; i < query_count; i++) {
        	if (i > 0)
        		p += sprintf(p, ",");
        	p += sprintf(p, "%llu", (ULL)mdata->read_bytes_count);
        	if( p - buf > (sizeof(buf)-512)) {
        		ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        		memset(buf, 0, sizeof(buf));
        		p = buf;
        	}
        	mdata = mdata->next;
        }
        if (p != buf)
            ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        break;
    case ismMon_WRITE_BYTES_COUNT:
        for (int i= 0; i < query_count; i++) {
        	if (i > 0)
        		p += sprintf(p, ",");
        	p += sprintf(p, "%llu", (ULL)mdata->write_bytes_count);
        	if( p - buf > (sizeof(buf)-512)) {
        		ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        		memset(buf, 0, sizeof(buf));
        		p = buf;
        	}
        	mdata = mdata->next;
        }
        if (p != buf)
            ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
        break;
    case ismMon_NONE:
    default:
    	TRACE(9, "Monitoring: getDatafromList does not support monitoring type %s\n", type);
    	remain_count = duration/interval;
        break;
    }

    if (remain_count > 0) {
        p = buf;
        if (remain_count == duration/interval) {
        	for (int i= 0; i < remain_count; i++) {
        		if (i > 0)
        		    p += sprintf(p, ",");
        	    p += sprintf(p, "0");
        	}
        } else {
        	for (int i= 0; i < remain_count; i++) {
        	    p += sprintf(p, ",0");
        	}
        }
    	ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
    }
    /*
     * Add the end of the json string
     * LastUpdateTimestamp in ISO8601
     */
    char tbuf[30];
    ism_ts_t *ts =  ism_common_openTimestamp(ISM_TZF_LOCAL);
    ism_common_setTimestamp(ts, timestamp);
    ism_common_formatTimestamp(ts, tbuf, 30, 0, ISM_TFF_ISO8601);
    sprintf(buf, "\",\"LastUpdateTimestamp\":\"%s\" }", tbuf);
    ism_common_closeTimestamp(ts);
    ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
    pthread_spin_unlock(&sp->snplock);
	return ISMRC_OK;
}

static int32_t getAllfromList(ism_monitoring_t *list, char * type, int duration, ism_snaptime_t interval, concat_alloc_t * output_buffer) {
	ism_srange_t *sp;
	int query_count = 0;
	char buf[4096];
	int rc = ISMRC_OK;
	int i = 0;
	int64_t *darray;
	ism_time_t timestamp=0;

	/*
	 * calculate how many noded need to be returned.
	 */
	int master_count = duration/interval + 1;
	query_count = master_count;
	darray = alloca(query_count*sizeof(int64_t));
	for (i = 0; i< master_count; i++)
		darray[i] = 0;

	int montype=getDataType(type);
	sp = list->ept_list;
	while(sp) {
		if (query_count > sp->node_count) {
	    	query_count = sp->node_count;
	    }
		pthread_spin_lock(&sp->snplock);
	    ism_mondata_t * mdata = sp->monitoring_data;
	    if (!mdata) {
	        pthread_spin_unlock(&sp->snplock);

	    	rc = ISMRC_NullPointer;
	    	TRACE(9, "Monitoring: getAllfromList cannot find the monitoring data for the endpoint %s\n", sp->name);
	    	snprintf(buf, sizeof(buf), "{ \"RC\":\"%d\", \"ErrorString\":\"Cannot find the monitoring data for the endpoint %s\" }", rc, sp->name);
	    	ism_common_allocBufferCopyLen(output_buffer,buf,strlen(buf));
	    	output_buffer->used = strlen(output_buffer->buf);
	    	return rc;
	    }
	    /*Get the timestamp of last node (or the first one on the mdata object*/
	    if(mdata->timestamp>0) timestamp  = mdata->timestamp;

	    switch(montype) {
        case ismMon_CONNECT_ACTIVE:
          	for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->connect_active;
            	mdata = mdata->next;
            }
            break;
        case ismMon_CONNECT_COUNT:
        	for (i= 0; i < query_count; i++) {
        	    darray[i] += mdata->connect_count;
        	    mdata = mdata->next;
        	}
        	break;
	    case ismMon_BAD_CONNECT_COUNT:
            for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->bad_connect_count;
            	mdata = mdata->next;
            }
            break;
        case ismMon_LOST_MSG_COUNT:
        	for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->lost_msg_count;
            	mdata = mdata->next;
            }
            break;
        case ismMon_WARN_MSG_COUNT:
        	for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->warn_msg_count;
            	mdata = mdata->next;
            }
            break;
        case ismMon_READ_MSG_COUNT:
            for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->read_msg_count;
            	mdata = mdata->next;
            }
            break;
        case ismMon_WRITE_MSG_COUNT:
        	for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->write_msg_count;
            	mdata = mdata->next;
            }
            break;
	    case ismMon_READ_BYTES_COUNT:
            for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->read_bytes_count;
            	mdata = mdata->next;
            }
            break;
        case ismMon_WRITE_BYTES_COUNT:
            for (i= 0; i < query_count; i++) {
            	darray[i] +=mdata->write_bytes_count;
            	mdata = mdata->next;
            }
            break;
        case ismMon_NONE:
       	default:
       		TRACE(9, "Monitoring: getDatafromList does not support monitoring type %s\n", type);
       		break;
        }

	    pthread_spin_unlock(&sp->snplock);
		sp = sp->next;
		query_count = master_count;
	}

	memset(buf, 0, sizeof(buf));
	/*
	 * LastUpdateTimestamp in ISO8601
	 */
    char tbuf[30];
    ism_ts_t *ts =  ism_common_openTimestamp(ISM_TZF_LOCAL);
    ism_common_setTimestamp(ts, timestamp);
    ism_common_formatTimestamp(ts, tbuf, 30, 0, ISM_TFF_ISO8601);
    sprintf(buf, "{ \"Endpoint\":\"All Endpoints\", \"Type\":\"%s\", \"Duration\":%d, \"Interval\":%llu, \"LastUpdateTimestamp\":\"%s\", \"Data\":\"", type, duration, (ULL)interval, tbuf);
    ism_common_closeTimestamp(ts);
    ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
    /*
     * print out the darray
     */
    memset(buf, 0, sizeof(buf));
    char *p = buf;
    for (i= 0; i < master_count; i++) {
    	if (i > 0)
    	    p += sprintf(p, ",");
    	p += sprintf(p, "%llu", (ULL)darray[i]);
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
    sprintf(buf, "\" }");
    ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));

	return ISMRC_OK;
}


XAPI int ism_monitoring_getEndpointMonData(
	    char               * action,
        ism_json_parse_t   * parseobj,
        concat_alloc_t     *output_buffer)
{
	ism_monitoring_t * list;
	int interval = 0;
	char * type = NULL;
	int duration = 0;
	int rc = ISMRC_OK;
    char *name = NULL;

	name = (char *)ism_json_getString(parseobj, "Name");

    if ( name && *name != '\0' && strcmp(name, "!MQConnectivityEndpoint") && ism_config_json_getComposite("Endpoint", name, 1) == NULL ) {
        rc = ISMRC_EndpointNotFound;
        return rc;
    }

	char *option = (char *)ism_json_getString(parseobj, "SubType");
	if (option && !strcmpi(option, "current")) {
		rc = ism_monitoring_getListenerMonitoringData(name, output_buffer);
		return rc;
	} else if (option && !strcmpi(option, "history")) {
		type = (char *)ism_json_getString(parseobj, "StatType");
		if (type == NULL)  {
			type="ActiveConnections";
		}
		char *dur = (char *)ism_json_getString(parseobj, "Duration");
		if (dur == NULL) {
			dur="1800";
		}
		duration = atoi(dur);
		if (duration > 0 && duration < 5) {
			duration = 5;
		}
	}


	if (duration <= 60*60) {
		//TRACE(9, "Monitoring: get data from short list\n");
		interval = DEFAULT_SNAPSHOTS_SHORTRANGE_INTERVAL;
	} else {
		//TRACE(9, "Monitoring: get data from long list\n");
		interval = DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL;
	}
	list = ism_monitoring_getListByInterval(interval);
	// defect 25913
	if (!name) {
 	    rc = getAllfromList(list, type, duration, interval, output_buffer);
	} else {
	    rc = getDatafromList(list, name, type, duration, interval, output_buffer);
	}
    return rc;
}
