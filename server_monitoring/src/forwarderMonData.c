/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
struct ism_fwdmondata_t {
    uint32_t				 timestamp;			/**< Timestamp when the data is collected.       */
    uint32_t                 channel_count ;    /**< Currently active channel count              */
    uint32_t                 recv_msg_rate;     /**< Incoming Message rate of forwarder          */
    uint32_t                 send_msg_rate0;    /**< QoS0 Outgoing Message rate of forwarder     */
    uint32_t                 send_msg_rate1;    /**< QoS1 Outgoing Message rate of forwarder     */
    uint32_t                 send_msg_rate;     /**< Outgoing Message (QoS0 + QoS1) rate         */
    struct ism_fwdmondata_t *prev;
    struct ism_fwdmondata_t *next;
};

/*
 * The snap shot range object is used to monitoring forwarder object
 * Throughput will be calculated using bytes_count/interval
 * This data is for local server, so no need to have a link list.
 */
struct ism_fwdrange_t {
    const char *          name;              /**< Server name                                    */
    uint32_t              node_count;        /**< Count of nodes in the mondata list             */
    uint32_t              node_idle;         /**< Count of nodes do not updated. If it is exceed max_count, it will be removed */
    ism_fwdmondata_t *    monitoring_data;   /**< A double link list holding the monitroing data */
    ism_fwdmondata_t *    tail;              /**< A point to the last node of monitoring_data    */
    pthread_spinlock_t    snplock;           /**< Lock over snapshots                            */
    struct ism_fwdrange_t * next;
};

/*
 * monitoring object.
 * It will contains all aggregate data for forwarder. It is up to the GUI/CLI to define
 * the intervals for each snapshot.
 */
struct ism_fwdmonitoring_t {
	ism_snaptime_t   last_snap_shot;      /**< Time in second of last snapshot    */
	ism_snaptime_t   snap_interval;       /**< Time in second of update interval  */
    uint32_t         max_count;           /**< Maximum count of nodes in the mondata list     */
    ism_fwdrange_t * fwd_list;            /**< List of the endpoints in the monitoring obj    */
};


/*
 * The main monitoring List that hold all the monitoring aggregation data
 */
struct ism_fwdmonitoringList_t {
	ism_fwdmonitoring_t   **monlist;      /**< Time in second of last snapshot    */
	int                   numOfList;
};

/*
 * Code can handle multiple forwarder, for now only local forwarder is used
 */
static const char *fwdname = NULL;
static ism_fwdmonitoringList_t * monitoringList = NULL;
static int32_t getFwdDataFromList(ism_fwdmonitoring_t *list, const char * name, char * type, int duration, ism_snaptime_t interval, concat_alloc_t * output_buffer);

/* Cluster Monitoring Data */
static int getFwdMonitoringData(concat_alloc_t *output_buffer) {
    int rbufSize = 1024;
	char * rbuf = alloca(rbufSize);
	fwd_monstat_t mondata = {0};
	int rc = ISMRC_OK;
	int found = 0;
	const char *name = ism_common_getServerName();

	/* get forwarder monitoring data and add to output_buffer */
    TRACE(9, "Get forwarder stats for local server %s\n", name?name:"N/A");

	if ( (rc = ism_fwd_getMonitoringStats(&mondata, 0)) == ISMRC_OK ) {
        TRACE(9, "Received Forwarder stats\n");

		sprintf(rbuf,"[ ");
		ism_common_allocBufferCopyLen(output_buffer,rbuf,strlen(rbuf));

		char mon_time[30];
		ism_ts_t *ts =  ism_common_openTimestamp(ISM_TZF_LOCAL);
		ism_common_setTimestamp(ts, mondata.timestamp);
		ism_common_formatTimestamp(ts, mon_time, 30, 0, ISM_TFF_ISO8601);
		ism_common_closeTimestamp(ts);

		int l;
		l = snprintf( rbuf, rbufSize, "{ \"Name\":\"%s\",\"MonitoringTime\":\"%s\",\"ChannelCount\":%u,\"ReceiveRate\":%u,\"UnreliableSendRate\":%u,\"ReliableSendRate\":%u }\n",
				name?name:"", mon_time, mondata.channel_count, mondata.recvrate, mondata.sendrate0, mondata.sendrate1);

		if (l + 1 > rbufSize) {
			rbufSize = l + 1;
			rbuf = alloca(rbufSize);
			sprintf( rbuf, "{ \"Name\":\"%s\",\"MonitoringTime\":\"%s\",\"ChannelCount\":%u,\"ReceiveRate\":%u,\"UnreliableSendRate\":%u,\"ReliableSendRate\":%u }\n",
					name?name:"", mon_time, mondata.channel_count, mondata.recvrate, mondata.sendrate0, mondata.sendrate1);
		}

		ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));

		found++;
	}

	if (found == 0) {
		rc = ISMRC_ClusterNotAvailable;
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


static ism_fwdmonitoring_t * ism_monitoring_getListByInterval(ism_snaptime_t snap_interval)  {
	int i;
	ism_fwdmonitoringList_t * p = monitoringList;

	if (!p) {
	    TRACE(9, "Monitoring: FWD monitoring list has not been initialed\n");
		return NULL;
	}
	for (i = 0; i < p->numOfList; i++) {
	    if ((p->monlist[i])->snap_interval == snap_interval)
			   return p->monlist[i];
	}
	return NULL;
}

static int createNewMonList(ism_fwdmonitoring_t ** monlist, ism_snaptime_t snap_interval) {
	(*monlist) = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,31),sizeof(ism_fwdmonitoring_t));
	if (!(*monlist)) {
		TRACE(9, "Monitoring: FWD monitoring list initial failed on memory allocation\n");
	    return ISMRC_AllocateError;
	}
	(*monlist)->last_snap_shot = ism_monitoring_currentTimeSec();
	(*monlist)->snap_interval  = snap_interval;
	if (snap_interval == DEFAULT_SNAPSHOTS_SHORTRANGE_INTERVAL) {
	    (*monlist)->max_count      = 60*60/snap_interval +1;
	} else if (snap_interval == DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL) {
		(*monlist)->max_count      = 24*60*60/snap_interval + 1;
	}

	(*monlist)->fwd_list = NULL;
	return ISMRC_OK;
}

/*
 * MonitoringList will be hardcoded for now.
 * 2 sets of aggregate data list will be stored in the memory for monitoring thread:
 *   1.  The last 1 hour data for FWD (include a special overall one). The data will be colleceted every 5 seconds
 *   2.  The last 24 hour data for FWD (include a special overall one). The data will be collected every minute.
 * Once the list reach its max_count, no more memory will be allocated and the earliest data will be recycled.
 *
 *  It will provide a fine grain (at least 100 point/per show) for planned GUI.
 *  GUI will call:
 *  ism_monitoring_getData(const char * name, ism_snaptime_t interval, ism_snaptime_t start, ism_snaptime_t end, int points)
 *  to get an array of data with size of points.
 *  The finest grain will be 5 sec.
 */
XAPI int ism_monitoring_initFwdMonitoringList(void)  {
	int i;
    fwdname = ism_common_getServerName();
	monitoringList = (ism_fwdmonitoringList_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,32),sizeof(ism_fwdmonitoringList_t)*NUM_MONITORING_LIST);
	monitoringList->numOfList = NUM_MONITORING_LIST;
	monitoringList->monlist = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,33),sizeof(ism_fwdmonitoring_t *)*NUM_MONITORING_LIST);
	for (i = 0; i < NUM_MONITORING_LIST; i++) {
		if(i == 0)
		    createNewMonList(&(monitoringList->monlist[i]), DEFAULT_SNAPSHOTS_SHORTRANGE_INTERVAL);
		else if(i == 1)
			createNewMonList(&(monitoringList->monlist[i]), DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL);
		else {
			TRACE(9, "Monitoring: FWD monitoring list initial failed\n");
		    return ISMRC_NotImplemented;
		}
	}
	return ISMRC_OK;
}


static ism_fwdmondata_t * newMonDataNode(void) {
	ism_fwdmondata_t *nmd = (ism_fwdmondata_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,34),sizeof(ism_fwdmondata_t));
	if (!nmd) {
		TRACE(9, "Monitoring: FWD monitoring data memory allocation failed\n");
		return NULL;
	}

    nmd->channel_count    = 0;    /**< Channel count                 */
    nmd->recv_msg_rate    = 0;    /**< Receive message rate          */
    nmd->send_msg_rate0   = 0;    /**< QoS0 Send message rate        */
    nmd->send_msg_rate1   = 0;    /**< QoS1 Send message rate        */
    nmd->send_msg_rate    = 0;    /**< QoS0 + QoS1 send message rate */
    nmd->timestamp 		  = 0;
    nmd->prev = NULL;
    nmd->next = NULL;

    return nmd;
}


static int createNewNode(ism_fwdrange_t *sp) {
	ism_fwdmondata_t *md = newMonDataNode();
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
	return ISMRC_OK;
}

static int reuseLastNode(ism_fwdrange_t *sp) {
	//TRACE(9, "reuse last node\n");
	pthread_spin_lock(&sp->snplock);
	/*
	 * The monitoring data reaches all the slots, the last slot will be reused.
	 */
	ism_fwdmondata_t * node = sp->tail;
	sp->tail = node->prev;
	(sp->tail)->next = NULL;   //new tail

	node->channel_count  = 0;
	node->recv_msg_rate  = 0;
	node->send_msg_rate0 = 0;
	node->send_msg_rate1 = 0;
	node->send_msg_rate  = 0;
	node->timestamp      = 0;
	node->next = sp->monitoring_data;
	sp->monitoring_data->prev = node;
	node->prev = NULL;
	sp->monitoring_data = node;

	pthread_spin_unlock(&sp->snplock);
	return ISMRC_OK;
}

/*
 * A new(reused) ism_fwdmondata_t node needs to be initialed as an idle node in case some endpoints can not be updated during the snapshot (disabled temporary etc.).
 *  -- clean all expired endpoints -- the endpoints in idle mode in all its nodes. no reason to keep it.
 *  -- depends on the max_count, decided if a new node should be added or reused the earliest node.
 */
static int initMonDataNode(ism_fwdmonitoring_t * slist, ism_fwdmonitoring_t * llist) {
	ism_fwdrange_t *p;
	int rc;

	if (!slist) {
		TRACE(9, "Monitoring: FWD initMonDataNode short list is NULL\n");
		return ISMRC_NotFound;
	}
	p = slist->fwd_list;

	/*
	 * First clean all unused srange_t object.
	 */
    if (!p)   {
    	return ISMRC_NotFound;
    }

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
	    p = llist->fwd_list;
	    if (!p)   {
	    	return ISMRC_NotFound;
	    }

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
static int storeFwdMonData(ism_fwdrange_t ** list, fwd_monstat_t * mon, ism_time_t timestamp) {
	ism_fwdrange_t *sp;

    if (!mon) {
    	TRACE(9, "Monitoring: in storeFwdMonData, monitoring data is NULL\n");
    	return ISMRC_NullPointer;
    }

    // TRACE(9, "Monitoring: in storeEptMonData, endpoint name is: %s\n", mon->name);
	sp = *list;
	while(sp) {
		if (sp->name && !strcmp(sp->name, fwdname)) {
			pthread_spin_lock(&sp->snplock);
			ism_fwdmondata_t * node = sp->monitoring_data;

			node->channel_count    = mon->channel_count;
			node->recv_msg_rate    = mon->recvrate;
			node->send_msg_rate0   = mon->sendrate0;
			node->send_msg_rate1   = mon->sendrate1;
			node->send_msg_rate    = mon->sendrate0 + mon->sendrate1;
			node->timestamp		   = mon->timestamp;

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
static int newFwdMonData(ism_fwdrange_t ** list, uint32_t max_count, fwd_monstat_t * mon, ism_time_t timestamp) {
	ism_fwdrange_t *ept = (ism_fwdrange_t *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,35),sizeof(ism_fwdrange_t));

	ept->name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_monitoring_misc,1000),fwdname);
    ept->monitoring_data = NULL;   /**< A double link list holding the monitroing data */
    ept->node_count = 0;           /**< Count of nodes in the mondata list             */
    ept->node_idle = 0;
    pthread_spin_init(&ept->snplock, 0);
    createNewNode(ept);
    ept->tail = ept->monitoring_data;              /**< A point to the last node of monitoring_data    */

    ept->next = *list;
    *list = ept;

    storeFwdMonData(list, mon, mon->timestamp);
    return ISMRC_OK;
}


static int recordSnapShot(ism_fwdmonitoring_t * slist, ism_fwdmonitoring_t * llist) {
	fwd_monstat_t mon = {0};
    int rc;

	if ( ism_fwd_getMonitoringStats(&mon, 0) == ISMRC_OK ) {
	    ism_time_t timestamp = mon.timestamp;
	    if ( timestamp <= 0 ) timestamp = ism_common_currentTimeNanos();

	    rc = storeFwdMonData(&(slist->fwd_list), &mon, timestamp);
        if (rc == ISMRC_NotFound) {
        	newFwdMonData(&(slist->fwd_list), slist->max_count,  &mon, timestamp);
        }

	    if (llist) {
	       	rc = storeFwdMonData(&(llist->fwd_list), &mon, timestamp);
	        if (rc == ISMRC_NotFound) {
	        	newFwdMonData(&(llist->fwd_list), llist->max_count, &mon, timestamp);
	        }
        }
	}
	return ISMRC_OK;
}

static int getDataType(char *type) {
	if (!strcasecmp(type, "ChannelCount"))
		return ismMon_CHANNEL_COUNT;
	if (!strcasecmp(type, "ReceiveRate"))
		return ismMon_RECV_MSG_RATE;
	if (!strcasecmp(type, "UnreliableSendRate"))
	    return ismMon_SEND_MSG_RATE0;
	if (!strcasecmp(type, "ReliableSendRate"))
		return ismMon_SEND_MSG_RATE1;
	if (!strcasecmp(type, "SendRate"))
		return ismMon_SEND_MSG_RATE;
	return ismMon_NONE;
}

static int32_t getFwdDataFromList(ism_fwdmonitoring_t *list, const char * name, char * type, int duration, ism_snaptime_t interval, concat_alloc_t * output_buffer) {
	ism_fwdrange_t *sp;
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
    ism_time_t timestamp = 0;

	memset(buf, 0, sizeof(buf));
	sp = list->fwd_list;
	while(sp) {
		if (name && !strcmp(sp->name, name)) {
            break;
		}
		sp = sp->next;
	}
	if (!sp) {
		rc = ISMRC_ArgNotValid;
		TRACE(5, "Monitoring: getFwdDataFromList cannot find the Forwarder: %s\n", name?name:"");

		ism_monitoring_getMsgId(rc, msgID);

        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]=name?name:"";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "An argument is not valid: Name: %s.", name?name:"");
        }
        ism_monitoring_setReturnCodeAndStringJSON(output_buffer, rc, tmpbuf);


		return rc;
	}

    int montype = getDataType(type);

    char *p = buf;

    pthread_spin_lock(&sp->snplock);
    ism_fwdmondata_t * mdata = sp->monitoring_data;

    if (!mdata) {
        pthread_spin_unlock(&sp->snplock);

    	rc = ISMRC_ArgNotValid;
    	TRACE(9, "Monitoring: getFwdDataFromList cannot find the monitoring data for %s\n", name);

    	ism_monitoring_getMsgId(rc, msgID);

        if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
        	repl[0]=name?name:"";
            ism_common_formatMessage(tmpbuf, sizeof(tmpbuf), lbuf, repl, 1);
        } else {
        	sprintf(tmpbuf, "An argument is not valid: Name: %s.",name);
        }
        ism_monitoring_setReturnCodeAndStringJSON(output_buffer, rc, tmpbuf);

    	return rc;
    }
    p += sprintf(buf, "{ \"Forwarder\":\"%s\", \"Type\":\"%s\", \"Duration\":%d, \"Interval\":%llu, \"Data\":\"", name, type, duration, (ULL)interval);

    /*
     * calculate how many noded need to be returned.
     */
    query_count = duration/interval + 1;
    if (query_count > sp->node_count) {
    	remain_count = query_count - sp->node_count;
    	query_count = sp->node_count;
    }

    /*Get the timestamp of last node (or the first one on the mdata object*/
   	if(mdata->timestamp > 0) timestamp  = mdata->timestamp;

    switch(montype) {
    case ismMon_CHANNEL_COUNT:
	    for (int i= 0; i < query_count; i++) {
  	        if (i > 0)
   		        p += sprintf(p, ",");
   	        p += sprintf(p, "%u", mdata->channel_count);
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

    case ismMon_RECV_MSG_RATE:
        for (int i= 0; i < query_count; i++) {
        	if (i > 0)
        		p += sprintf(p, ",");
        	p += sprintf(p, "%u", mdata->recv_msg_rate);
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

    case ismMon_SEND_MSG_RATE0:
        for (int i= 0; i < query_count; i++) {
        	if (i > 0)
        		p += sprintf(p, ",");
        	p += sprintf(p, "%u", mdata->send_msg_rate0);
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

    case ismMon_SEND_MSG_RATE1:
        for (int i= 0; i < query_count; i++) {
        	if (i > 0)
        		p += sprintf(p, ",");
        	p += sprintf(p, "%u", mdata->send_msg_rate1);
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

    case ismMon_SEND_MSG_RATE:
        for (int i= 0; i < query_count; i++) {
        	if (i > 0)
        		p += sprintf(p, ",");
        	p += sprintf(p, "%u", mdata->send_msg_rate);
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
    	TRACE(9, "Monitoring: getFwdDataFromList does not support monitoring type %s\n", type);
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
    timestamp = ism_common_currentTimeNanos();
    ism_ts_t *ts =  ism_common_openTimestamp(ISM_TZF_LOCAL);
    ism_common_setTimestamp(ts, timestamp);
    ism_common_formatTimestamp(ts, tbuf, 30, 0, ISM_TFF_ISO8601);
    sprintf(buf, "\",\"LastUpdateTimestamp\":\"%s\" }", tbuf);
    ism_common_closeTimestamp(ts);
    ism_common_allocBufferCopyLen(output_buffer, buf,strlen(buf));
    pthread_spin_unlock(&sp->snplock);
	return ISMRC_OK;
}

static int32_t getAllfromList(ism_fwdmonitoring_t *list, char * type, int duration, ism_snaptime_t interval, concat_alloc_t * output_buffer) {
	ism_fwdrange_t *sp;
	int query_count = 0;
	char buf[4096];
	int rc = ISMRC_OK;
	int i = 0;
	int32_t *darray;
	ism_time_t timestamp = 0;

	/*
	 * calculate how many noded need to be returned.
	 */
	int master_count = duration/interval + 1;
	query_count = master_count;
	darray = alloca(query_count*sizeof(int64_t));
	for (i = 0; i< master_count; i++)
		darray[i] = 0;

	int montype = getDataType(type);
	sp = list->fwd_list;
	while(sp) {
		if (query_count > sp->node_count) {
	    	query_count = sp->node_count;
	    }
		pthread_spin_lock(&sp->snplock);
	    ism_fwdmondata_t * mdata = sp->monitoring_data;
	    if (!mdata) {
	        pthread_spin_unlock(&sp->snplock);

	    	rc = ISMRC_NullPointer;
	    	TRACE(9, "Monitoring: getAllfromList cannot find the forwarder monitoring data.\n");
	    	snprintf(buf, sizeof(buf), "{ \"RC\":\"%d\", \"ErrorString\":\"Cannot find the forwarder monitoring data.\" }", rc);
	    	ism_common_allocBufferCopyLen(output_buffer,buf,strlen(buf));
	    	output_buffer->used = strlen(output_buffer->buf);
	    	return rc;
	    }
	    /*Get the timestamp of last node (or the first one on the mdata object*/
	    if (mdata->timestamp > 0) timestamp  = mdata->timestamp;
	    if ( timestamp <= 0 ) timestamp = ism_common_currentTimeNanos();


	    switch(montype) {
        case ismMon_CHANNEL_COUNT:
          	for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->channel_count;
            	mdata = mdata->next;
            }
            break;
        case ismMon_RECV_MSG_RATE:
            for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->recv_msg_rate;
            	mdata = mdata->next;
            }
            break;
        case ismMon_SEND_MSG_RATE0:
        	for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->send_msg_rate0;
            	mdata = mdata->next;
            }
            break;
        case ismMon_SEND_MSG_RATE1:
        	for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->send_msg_rate1;
            	mdata = mdata->next;
            }
            break;
        case ismMon_SEND_MSG_RATE:
        	for (i= 0; i < query_count; i++) {
            	darray[i] += mdata->send_msg_rate;
            	mdata = mdata->next;
            }
            break;
        case ismMon_NONE:
       	default:
       		TRACE(9, "Monitoring: getFwdDataFromList does not support monitoring type %s\n", type);
       		break;
        }

	    pthread_spin_unlock(&sp->snplock);
		sp = sp->next;
		query_count = master_count;
	}

	memset(buf, 0, sizeof(buf));

	const char *spname = fwdname;

	/*
	 * LastUpdateTimestamp in ISO8601
	 */
    char tbuf[30];
    timestamp = ism_common_currentTimeNanos();
    ism_ts_t *ts =  ism_common_openTimestamp(ISM_TZF_LOCAL);
    ism_common_setTimestamp(ts, timestamp);
    ism_common_formatTimestamp(ts, tbuf, 30, 0, ISM_TFF_ISO8601);
    sprintf(buf, "{ \"Forwarder\":\"%s\", \"Type\":\"%s\", \"Duration\":%d, \"Interval\":%llu, \"LastUpdateTimestamp\":\"%s\", \"Data\":\"", spname, type, duration, (ULL)interval, tbuf);
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
    	p += sprintf(p, "%d", darray[i]);
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


/*
 * Check if a snapshot is needed at the time.
 */
XAPI int ism_fwdmonitoring_checkAction(ism_snaptime_t curtime) {
	int rc = ISMRC_OK;


	ism_fwdmonitoring_t * slist = ism_monitoring_getListByInterval(DEFAULT_SNAPSHOTS_SHORTRANGE_INTERVAL);
	ism_fwdmonitoring_t * llist = ism_monitoring_getListByInterval(DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL);

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

XAPI int ism_monitoring_getForwarderMonData(
	    char               * action,
        ism_json_parse_t   * parseobj,
        concat_alloc_t     * output_buffer)
{
	ism_fwdmonitoring_t * list;
	int interval = 0;
	char * type = NULL;
	int duration = 0;
	int rc = ISMRC_OK;
	char *name = NULL;

	char *option = (char *)ism_json_getString(parseobj, "SubType");
	if (option && !strcmpi(option, "current")) {
		rc = getFwdMonitoringData(output_buffer);
		return rc;
	} else if (option && !strcmpi(option, "history")) {
		type = (char *)ism_json_getString(parseobj, "StatType");
		if (type == NULL)  {
			type="ChannelCount";
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

	if (!name) {
 	    rc = getAllfromList(list, type, duration, interval, output_buffer);
	} else {
	    rc = getFwdDataFromList(list, fwdname, type, duration, interval, output_buffer);
	}
    return rc;
}
