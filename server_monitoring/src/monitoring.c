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
#include <connectionMonData.h>
#include <engineMonData.h>
#include <engineAdvancedPD.h>
#include <monitoringutil.h>
#include <endpointMonData.h>
#include <storeMonData.h>
#include <monitoringutil.h>
#include <admin.h>
#include <config.h>
#include "monitoringSnmpTrap.h"


#define MONIT_INTERVAL        5000000      // 5 seconds
#define MONIT_2SEC_INTERVAL   2000000      // 2 seconds
#define MONIT_EVENT_SIZE      ( sizeof (struct inotify_event) )
#define MONIT_EVENT_BUF_LEN   ( 1024 * ( MONIT_EVENT_SIZE + 16 ) )

#define DEFAULT_SNAPSHOTS_SHORTRANGE_INTERVAL  5          //in second
#define DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL   60         //in second
#define NUM_MONITORING_LIST   2


#define THREAD_STATUS_INACTIVE 0
#define THREAD_STATUS_ACTIVE   1
#define THREAD_STATUS_STARTING 2
#define THREAD_STATUS_SHUTDOWN 3

#define LFCHAR "\n"

pthread_mutex_t monitLock;
pthread_cond_t  monitCond;

pthread_mutex_t monitConnLock;
pthread_cond_t  monitConnCond;

pthread_mutex_t monitDisconnectedClientNotificationLock;
pthread_cond_t  monitDisconnectedClientNotificationCond;
int DisconnectedClientNotificationStarted=0;
int SnmpEnabled;

static int initEngineInEndpointThread=0;
static int initEngineInConnThread=0;
static int initEngineInDisconnectedClientNotificationThread=0;

static int thread_status=THREAD_STATUS_INACTIVE;
static int conn_thread_status=THREAD_STATUS_INACTIVE;
static int DisconnectedClientNotification_thread_status=THREAD_STATUS_INACTIVE;

typedef int (*ism_snmpRunning_f) (void);
static ism_snmpRunning_f snmpRunning_f = NULL;

extern void ism_monitoring_initNotification(void);
extern void ism_monitoring_termNotification(void);
extern void ism_monitoring_processNotifications(void);
extern int ism_monitoring_getListenerMonitoringDataExternal(ism_endpoint_mon_t * mon , uint64_t currentTime,  concat_alloc_t *output_buffer) ;
extern void ism_monitoring_publishEngineTopicStatsExternal(ismMonitoringPublishType_t publishType,
													 ismEngineMonitorType_t engineMonType);
extern int ism_monitoring_initStoreHistMonitoringList(void);
extern int ism_monitoring_initEngineMonData(void);
char *  ism_admin_getStateStr(int type);

/*Request Locale Specific*/
pthread_key_t monitoring_localekey;
pthread_once_t monitoringlocalekey_once = PTHREAD_ONCE_INIT;

int startProcessingEngineEvents = 0;

static void make_key(void)
{
    (void) pthread_key_create(&monitoring_localekey, NULL);
}

/**
 * Function to process 60 seconds stats:
 * - Endpoint
 * - Topic
 * NOTE - to support time-graph in external monitoring application, time interval is reduced to 2 seconds
 */
static void ism_monitoring_process60SecondsStatsPublish(ismMonitoringPublishType_t publishType)
{
	/*Publish Endpoint Monitoring Data*/
	ism_endpoint_mon_t * monlis;
	int result_count=0, pcount=0;;

	TRACE(8, "Start process 60-second Statistics.\n");

	result_count = ism_transport_getEndpointMonitor(&monlis, NULL);

	uint64_t currentTime = ism_common_currentTimeNanos();

	TRACE(8, "Endpoint Statistics Publishing. Endpoint Count: %d\n", result_count);
	for(pcount=0; pcount< result_count; pcount++)
	{
		char output_buf[1024];
		concat_alloc_t outputBuffer =  { NULL, 0, 0, 0, 0 };
		outputBuffer.buf = (char *)&output_buf;
		outputBuffer.len=1024;

		ism_endpoint_mon_t * epmon = &monlis[pcount];
		ism_monitoring_getListenerMonitoringDataExternal(epmon,currentTime, &outputBuffer);

		ism_monitoring_submitMonitoringEvent(	ismMonObjectType_Endpoint,
						   						epmon->name,
						   						strlen(epmon->name),
						   						(const char *)outputBuffer.buf,
						   						outputBuffer.used,
						   						publishType);


		if(outputBuffer.inheap) ism_common_freeAllocBuffer(&outputBuffer);
	}
	if(monlis)
		ism_transport_freeEndpointMonitor(monlis);


	/* Get and Publish Topic Monitor Stats - get unsorted data for Topic stats */
	 TRACE(8, "Topic Statistics Publishing.\n");
	 ism_monitoring_publishEngineTopicStatsExternal(publishType, ismENGINE_MONITOR_ALL_UNSORTED);

    TRACE(8, "End process 60-second Statistics.\n");
}

/**
 * Function which process the 2 seconds stats:
 * - Memory
 * - Store
 */
static void ism_monitoring_process2SecondsStatsPublish(ismMonitoringPublishType_t publishType)
{
	char output_buf[1024];
	concat_alloc_t outputBuffer =  { NULL, 0, 0, 0, 0 };
	outputBuffer.buf = (char *)&output_buf;
	outputBuffer.len=1024;

#ifdef _SNMP_THREADED_
	char topicStr[1024]={0};
#endif

	TRACE(9, "Start process 2-second Statistics.\n");

	if ( startProcessingEngineEvents == 1 ) {
		/*Publish Memory Monitoring Data*/
		TRACE(8, "Memory Statistics Publishing.\n");
		ism_monitoring_getMemoryStats(NULL, NULL,&outputBuffer, 1);
		ism_monitoring_submitMonitoringEvent(	ismMonObjectType_Memory,
												NULL,
												0,
												(const char *)outputBuffer.buf,
												outputBuffer.used,
												publishType);
	}

	if (!snmpRunning_f)
		snmpRunning_f = (ism_snmpRunning_f)(uintptr_t)ism_common_getLongConfig("_ism_snmp_running_fnptr", 0L);
	/* process potential SNMP trap notifications, ignore errors*/
	if (snmpRunning_f())
	{
		ism_monitoring_getExtMonTopic(ismMonObjectType_Memory, topicStr);
		imaSnmp_messageArrived(topicStr, outputBuffer.buf, outputBuffer.used);
	}

	/*Publish Store Monitoring Data*/
	if(outputBuffer.inheap) ism_common_free(ism_memory_monitoring_misc,outputBuffer.buf);
	memset(&outputBuffer, 0 , sizeof(concat_alloc_t));

	if ( startProcessingEngineEvents == 1 ) {
		TRACE(8, "Store Statistics Publishing.\n");
		ism_monitoring_getStoreStats(NULL, NULL,&outputBuffer, 1);
		ism_monitoring_submitMonitoringEvent(	ismMonObjectType_Store,
												NULL,
												0,
												(const char *)outputBuffer.buf,
												outputBuffer.used,
												publishType);
	}


	/* process potential SNMP trap notifications ignore errors*/
	if (snmpRunning_f())
	{
		ism_monitoring_getExtMonTopic(ismMonObjectType_Store, topicStr);
		imaSnmp_messageArrived(topicStr, outputBuffer.buf, outputBuffer.used);
	}


	if(outputBuffer.inheap) ism_common_free(ism_memory_alloc_buffer,outputBuffer.buf);

	TRACE(8, "End process 2-second Statistics.\n");
}

/**
 * The monitoring thread which will process the 2-second
 * statistics. Also processing the short range endpoint data.
 */
void * ism_monitoring_thread(void * param, void * context, int value) {
	ism_time_t lastime=0, currenttime=0;
	pthread_mutex_lock(&monitLock);
	thread_status = THREAD_STATUS_ACTIVE;
	pthread_mutex_unlock(&monitLock);

	TRACE(5, "Monitoring Thread 1 is active.\n");

	struct timespec   ts={1,0};
  	int rc=0;
  	int count = 0;

    for (;;) {
    	pthread_mutex_lock(&monitLock);

    	rc = ism_common_cond_timedwait(&monitCond, &monitLock, &ts, 1);
    	count++;

          /*Check if the thread is still active*/
	   if(thread_status != THREAD_STATUS_ACTIVE){
			pthread_mutex_unlock(&monitLock);
			break;
	   }

       if (initEngineInEndpointThread==0 && (serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED) && startProcessingEngineEvents == 1){
       		ism_engine_threadInit(0);
			char threadName[16];
			ism_common_getThreadName(threadName, sizeof(threadName));

			initEngineInEndpointThread=1;
			TRACE(4, "Initialized engine in '%s' thread.\n", threadName);
       }

        pthread_mutex_unlock(&monitLock);

        if ( initEngineInEndpointThread == 0 ) {
        	continue;
        }

        if ( startProcessingEngineEvents == 1 && (serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED) && count > 1) {
            ism_monitoring_process2SecondsStatsPublish(ismPublishType_SYNC);
            count = 0;
        }

        /*Process Endpoint Data*/
        currenttime = (uint64_t) ism_common_readTSC() ;


        if ( currenttime >= ((DEFAULT_SNAPSHOTS_SHORTRANGE_INTERVAL) + lastime)) {
			TRACE(8, "Start process the short range snap shot for Endpoint and Forwarder data.\n");
            rc = ism_monitoring_checkAction(currenttime);
            if (rc) {
    		    TRACE(8, "Failed to  process the short range snap shot for Endpoint data.\n");
            }else{
            	TRACE(8, "End process the short range snap shot for Endpoint data.\n");
            }

            rc = ism_fwdmonitoring_checkAction(currenttime);
            if (rc) {
    		    TRACE(8, "Failed to  process the short range snap shot for Forwarder data.\n");
            }else{
            	TRACE(8, "End process the short range snap shot for Forwarder data.\n");
            }

            lastime = currenttime;

        }


    }

    pthread_mutex_lock(&monitLock);
	thread_status = THREAD_STATUS_INACTIVE;
	pthread_mutex_unlock(&monitLock);

    return NULL;
}

/**
 * This thread is processing the Connection Endpoint Data.
 * Also process 60-second statistics.
 */
void * ism_monitoring_threadEndpoint(void * param, void * context, int value) {
    ism_time_t concurrenttime=0, connlastime=0;
	ism_time_t ExternalLastTime60=0;

	pthread_mutex_lock(&monitConnLock);
	conn_thread_status = THREAD_STATUS_ACTIVE;
	pthread_mutex_unlock(&monitConnLock);

	TRACE(5,"Monitoring Thread 2 is active.\n");

	struct timespec   ts={2,0};

    for (;;) {

       //ism_common_sleep(MONIT_2SEC_INTERVAL);


        pthread_mutex_lock(&monitConnLock);

    	ism_common_cond_timedwait(&monitConnCond, &monitConnLock, &ts, 1);

        if (conn_thread_status != THREAD_STATUS_ACTIVE) {
			pthread_mutex_unlock(&monitConnLock);
			break;
		}

		if(initEngineInConnThread==0 && (serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED) && startProcessingEngineEvents == 1){
       		ism_engine_threadInit(0);
			char threadName[16];
			ism_common_getThreadName(threadName, sizeof(threadName));

			initEngineInConnThread=1;
			TRACE(4, "Initialized engine in '%s' thread.\n", threadName);
        }

		pthread_mutex_unlock(&monitConnLock);

        if ( initEngineInConnThread == 0 ) {
        	continue;
        }

		/*Process Monitoring Submitted Events*/
       	ism_monitoring_processNotifications();


		concurrenttime = (uint64_t) ism_common_readTSC() ;

		/* 60 second interval for endpoint and topic monitoring stats does significantly
		 * limit the dynamism of an external monitoring application. Reduce the time
		 * interval to 2 seconds
		 */
		if ( concurrenttime >= (2 + ExternalLastTime60))
		{
			if ( startProcessingEngineEvents == 1 && ( serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED ) ) {
				ism_monitoring_process60SecondsStatsPublish(ismPublishType_ASYNC);
			}
			//Submit Monitoring data for External Monitoring for the one for 60 seconds internval
			ExternalLastTime60 = concurrenttime;
		}


        concurrenttime = (uint64_t) ism_common_readTSC() ;

        if ( concurrenttime >= ((DEFAULT_CONN_DATA_UPDATE_INTERVAL) + connlastime))
        {
        	TRACE(8, "Start process the Connection Data.\n");
	    	/*Process connection data update*/
            ism_monitoring_connectionCacheUpdate();
            TRACE(8, "End process the Connection Data.\n");
            connlastime = concurrenttime;
        }
    }

    pthread_mutex_lock(&monitConnLock);
	conn_thread_status = THREAD_STATUS_INACTIVE;
	pthread_mutex_unlock(&monitConnLock);

    return NULL;
}

/*
 * Initialize monitoring
 */
int32_t ism_monitoring_init(void) {
    pthread_mutex_init(&monitLock, 0);
    pthread_mutex_init(&monitConnLock, 0);
    ism_common_cond_init_monotonic(&monitCond);
    ism_common_cond_init_monotonic(&monitConnCond);

	/*Init Monitoring List for Endpoint*/
    int rc = ism_monitoring_initMonitoringList();
    if (rc) {
		TRACE(1, "Monitoring: monitoring list for endpoint initial failed\n");
	    return rc;
    }

	/* Init Monitoring List for Forwarder */
    rc = ism_monitoring_initFwdMonitoringList();
    if (rc) {
		TRACE(1, "Monitoring: monitoring list for endpoint initial failed\n");
	    return rc;
    }

    /*Init Monitoring component for engine monitoring data*/
    rc = ism_monitoring_initEngineMonData();
    if (rc) {
		TRACE(1, "Monitoring: engine monitoring data initialization failed\n");
	    return rc;
    }

     /*Init Monitoring List for store historical objects*/
    rc = ism_monitoring_initStoreHistMonitoringList();
    if (rc) {
		TRACE(1, "Monitoring: monitoring list for store initial failed\n");
	    return rc;
    }

	ism_monitoring_initNotification();

	ism_monitoring_connectionMonDataInit();

	pthread_mutex_lock(&monitLock);
	thread_status = THREAD_STATUS_STARTING;
	pthread_mutex_unlock(&monitLock);

	pthread_mutex_lock(&monitConnLock);
	conn_thread_status = THREAD_STATUS_STARTING;
	pthread_mutex_unlock(&monitConnLock);

	/*Init Locale Thread Specific Key*/
	 (void) pthread_once(&monitoringlocalekey_once, make_key);

    /* start monitoring thread */
    /* TODO: the thread should be started in start */
    ism_threadh_t monitThread;
    ism_common_startThread(&monitThread, ism_monitoring_thread, NULL, NULL,
        0, ISM_TUSAGE_NORMAL, 0, "monitoring.1", "The monitoring thread");

    ism_threadh_t monitThread2;
    ism_common_startThread(&monitThread2, ism_monitoring_threadEndpoint, NULL, NULL,
        0, ISM_TUSAGE_NORMAL, 0, "monitoring.2", "The monitoring thread");

    return ISMRC_OK;
}

/*
 * Terminate monitoring
 */
int32_t ism_monitoring_term(void) {

	ism_monitoring_stopEngineEventFlag();

	pthread_mutex_lock(&monitLock);
	thread_status = THREAD_STATUS_SHUTDOWN;
	pthread_mutex_unlock(&monitLock);

	pthread_mutex_lock(&monitConnLock);
	conn_thread_status = THREAD_STATUS_SHUTDOWN;
	pthread_mutex_unlock(&monitConnLock);

	ism_common_sleep(2000);

	/*Give time for the monitoring threads to shutdown*/
	pthread_mutex_lock(&monitLock);
	pthread_cond_signal(&monitCond);
	while(thread_status != THREAD_STATUS_INACTIVE){
		pthread_mutex_unlock(&monitLock);
		ism_common_sleep(1000);
		pthread_mutex_lock(&monitLock);
	}
	pthread_mutex_unlock(&monitLock);

	pthread_mutex_destroy(&monitLock);
	pthread_cond_destroy(&monitCond);

	pthread_mutex_lock(&monitConnLock);
	pthread_cond_signal(&monitConnCond);
	while(conn_thread_status != THREAD_STATUS_INACTIVE){
		pthread_mutex_unlock(&monitConnLock);
		ism_common_sleep(1000);
		pthread_mutex_lock(&monitConnLock);
	}

	pthread_mutex_unlock(&monitConnLock);

	pthread_mutex_destroy(&monitConnLock);
	pthread_cond_destroy(&monitConnCond);

	/*Shutdown DisconnectedClientNotification Thread if it is started.*/
	if(DisconnectedClientNotificationStarted==1){

		pthread_mutex_lock(&monitDisconnectedClientNotificationLock);
		DisconnectedClientNotification_thread_status = THREAD_STATUS_SHUTDOWN;
		pthread_mutex_unlock(&monitDisconnectedClientNotificationLock);

		ism_common_sleep(1000);

		pthread_mutex_lock(&monitDisconnectedClientNotificationLock);
		pthread_cond_signal(&monitDisconnectedClientNotificationCond);
		while(DisconnectedClientNotification_thread_status != THREAD_STATUS_INACTIVE){
			pthread_mutex_unlock(&monitDisconnectedClientNotificationLock);
			ism_common_sleep(1000);
			pthread_mutex_lock(&monitDisconnectedClientNotificationLock);
		}

		pthread_mutex_unlock(&monitDisconnectedClientNotificationLock);

		pthread_mutex_destroy(&monitDisconnectedClientNotificationLock);
		pthread_cond_destroy(&monitDisconnectedClientNotificationCond);
	}

	ism_monitoring_connectionMonDataDestroy();
	ism_monitoring_termNotification();

    return ISMRC_OK;
}


/*
 * Interface for protocol - to send Monitoring request and get results back
 */
XAPI int ism_process_monitoring_action(ism_transport_t *transport, const char *inpbuf, int inpbuflen, concat_alloc_t *output_buffer, int *rc) {
    char *action = NULL;
    char *user = NULL;
    const char *locale;
    char  rbuf[1024];
    // int   buflen = 0;
    char  lbuf[1024];
    const char * repl[3];



    if ( inpbuf == NULL ) {
        *rc = EINVAL;
        return 1;
    }

    if ( *inpbuf != '{' ) {
        *rc = EINVAL;
        return 1;
    }



    char *input_buffer = ism_common_malloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,26),inpbuflen + 1);
    memcpy(input_buffer, inpbuf, inpbuflen);
    input_buffer[inpbuflen] = 0;

    TRACE(9, "Monitoring request buffer: \n%s\n", input_buffer);
    // LOG(INFO, Admin, 6014, "%-s", "Processing an administrative action {0}.", input_buffer);

    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[20];
    char *tmpbuf = input_buffer;

    parseobj.ent = ents;
    parseobj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseobj.source = (char *) input_buffer;
    parseobj.src_len = inpbuflen + 1;
    ism_json_parse(&parseobj);

    memset(rbuf, 0, sizeof(rbuf));
    memset(lbuf, 0, sizeof(lbuf));

	if (parseobj.rc) {
		LOG(ERROR, Monitoring, 6501, "%-s%d", "Failed to parse monitoring request {0}: RC={1}.",
			 tmpbuf, parseobj.rc );

		int xlen;
		char  msgID[12];
		ism_monitoring_getMsgId(6501, msgID);
		char mtmpbuf[1024];
		if ( ism_common_getMessageByLocale(msgID, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf), &xlen) != NULL ) {
			char abf[12];
			repl[0] = tmpbuf;
			repl[1] = ism_common_itoa(parseobj.rc, abf);
			ism_common_formatMessage(mtmpbuf, sizeof(mtmpbuf), lbuf, repl, 2);
		} else {
			sprintf(mtmpbuf, "Failed to parse monitoring request %s: %d.", tmpbuf,parseobj.rc);
		}
		ism_monitoring_setReturnCodeAndStringJSON(output_buffer, parseobj.rc, mtmpbuf);
        ism_common_free(ism_memory_monitoring_misc,input_buffer);
        *rc = ISMRC_ParseError;
		return ISMRC_ParseError;
	} else {
		action = (char *)ism_json_getString(&parseobj, "Action");
		user = (char *)ism_json_getString(&parseobj, "User");

		/*Get Request Locale*/
		locale = (const char *)ism_json_getString(&parseobj, "Locale");
		if(locale==NULL){
			/*Set to the server default locale*/
			locale = (const char *)ism_common_getLocale();
		}
		/*Set to Thread Specific*/
		ism_common_setRequestLocale(monitoring_localekey, locale);

	}

    if ( !action || !*action) {
        ism_common_getErrorStringByLocale(6502, ism_common_getRequestLocale(monitoring_localekey), lbuf, sizeof(lbuf));
        LOG(ERROR, Monitoring, 6502, "%-s", "The monitoring action {0} is not supported.", tmpbuf);
        repl[0] = tmpbuf;
        ism_common_formatMessage(rbuf, sizeof(rbuf), lbuf, repl, 1);
        ism_common_allocBufferCopyLen(output_buffer, rbuf, strlen(rbuf));
        ism_common_free(ism_memory_monitoring_misc,input_buffer);
        return 0;
    }

    // Change this to audit log */
    TRACE(7, "Monitoring: action=%s user=%s\n", action, user?user:"");
    int statType = ism_monitoring_getStatType(action);


	/*Before Processing the mon request, ensure that the Server is in RUNNING state.*/
    int state = ism_admin_get_server_state();
    if(state!=ISM_SERVER_RUNNING){

    	int monCheckFailed=1;
    	/*Only allow Connection Volume*/
    	if(statType==ismMON_STAT_Connection){
    		char *option = (char *)ism_json_getString(&parseobj, "Option");

			if( (option && (*option == 'v' || *option == 'V') ) ){
				monCheckFailed=0;
			}
    	}

    	if(monCheckFailed==1){
			char   errstr [1024];
			*rc = ISMRC_MonDataNotAvail;
            ism_common_setErrorData(ISMRC_MonDataNotAvail, "%s", ism_admin_getStateStr(state));
			ism_common_formatLastErrorByLocale(ism_common_getRequestLocale(monitoring_localekey), errstr, sizeof(errstr));
			sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":", ISMRC_MonDataNotAvail);
	        ism_common_allocBufferCopyLen(output_buffer,rbuf,strlen(rbuf));
	        ism_json_putString(output_buffer, errstr);
	        ism_common_allocBufferCopyLen(output_buffer,"}\n",2);
	        ism_common_free(ism_memory_monitoring_misc,input_buffer);
	        return ISMRC_MonDataNotAvail;
        }
    }


	/* Process Monitoring received in JSON format */
    switch(statType) {
        case ismMON_STAT_Store:
        {
            *rc = ism_monitoring_getStoreStats(action, &parseobj, output_buffer,0);
            break;
        }

        case ismMON_STAT_Memory:
        {
            *rc = ism_monitoring_getMemoryStats(action, &parseobj, output_buffer, 0);
            break;
        }

        case ismMON_STAT_HA:
        {
            *rc = ism_monitoring_getHAStats(action, &parseobj, output_buffer);
            break;
        }
        case ismMON_STAT_Endpoint:
        {
            *rc = ism_monitoring_getEndpointMonData(action, &parseobj, output_buffer);
            break;
        }
        case ismMON_STAT_Connection:
        {
            *rc = ism_monitoring_getConnectionMonData(action, &parseobj, output_buffer);
            break;
        }

        case ismMON_STAT_Subscription:
        case ismMON_STAT_Topic:
        case ismMON_STAT_Queue:
        case ismMON_STAT_MQTTClient:
        case ismMON_STAT_Transaction:
        case ismMON_STAT_ResourceSet:
        {
            *rc = ism_monitoring_getEngineStats(action, &parseobj, output_buffer);
            break;
        }

        case ismMON_STAT_AdvEnginePD:
        {
            *rc = ism_monitoring_getAdvancedEnginePDData(action, &parseobj, output_buffer);
            break;
        }

        case ismMON_STAT_MemoryDetail:
        {
            *rc = ism_monitoring_getDetailedMemoryStats(action, &parseobj, output_buffer);
            break;
        }

        case ismMON_STAT_Security:
        {
            *rc = ism_monitoring_getSecurityStats(action, &parseobj, output_buffer);
            break;
        }

        case ismMON_STAT_Cluster:
        {
            ism_fwd_getForwarderStats(output_buffer, 1);
            break;
        }

        case ismMON_STAT_Forwarder:
        {
        	    *rc = ism_monitoring_getForwarderMonData(action, &parseobj, output_buffer);
            break;
        }

        default:
        {
            *rc = ISMRC_NotFound;
            char buf[1024];
            char *errstr = NULL;
            errstr = (char *)ism_common_getErrorStringByLocale(*rc, ism_common_getRequestLocale(monitoring_localekey), buf, 256);
            if ( errstr )
                sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"%s\" }\n", *rc, errstr);
            else
                sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"Unknown\" }\n", *rc);

            ism_common_allocBufferCopyLen(output_buffer,rbuf,strlen(rbuf));
            output_buffer->used = strlen(output_buffer->buf);
            break;
        }
    }

    ism_common_free(ism_memory_monitoring_misc,input_buffer);

	if ( output_buffer->used > 0 ) {
    	if(ism_common_getTraceLevel()==9){
			char * tmpStr = (char *) alloca( output_buffer->used+1);
			memcpy(tmpStr, output_buffer->buf, output_buffer->used);
			tmpStr[output_buffer->used]='\0';
			TRACE(9, "Process Monitoring Action: Return Value=%s\n", tmpStr);
		}
        ism_common_allocBufferCopyLen(output_buffer, LFCHAR, strlen(LFCHAR));
        return 0;
    } else {
        *rc = ENODATA;
        return 1;
    }
}

/**
 * This thread is processing the Connection Endpoint Data.
 */
void * ism_monitoring_threadDisconnectedClientNotification(void * param, void * context, int value) {
    ism_time_t concurrenttime=0;
	ism_time_t ExternalLastTime60=0;

	pthread_mutex_lock(&monitDisconnectedClientNotificationLock);
	DisconnectedClientNotification_thread_status = THREAD_STATUS_ACTIVE;
	pthread_mutex_unlock(&monitDisconnectedClientNotificationLock);

	TRACE(5, "Monitoring Thread 3 is active.\n");

    for (;;) {
    	struct timespec   ts = {0,0};
    	int dissconClientNotifInterval = ism_config_getDisconnectedClientNotifInterval();

        pthread_mutex_lock(&monitDisconnectedClientNotificationLock);
        if ( dissconClientNotifInterval >= 15 ) {
            ts.tv_sec = dissconClientNotifInterval;
        } else {
        	ts.tv_sec = 60;
        }

    	ism_common_cond_timedwait(&monitDisconnectedClientNotificationCond, &monitDisconnectedClientNotificationLock, &ts, 1);

        if(DisconnectedClientNotification_thread_status != THREAD_STATUS_ACTIVE){
			pthread_mutex_unlock(&monitDisconnectedClientNotificationLock);
			break;
		}

		if(initEngineInDisconnectedClientNotificationThread==0 && (serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED) && startProcessingEngineEvents == 1 ){
       		ism_engine_threadInit(0);
			char threadName[16];
			ism_common_getThreadName(threadName, sizeof(threadName));

			initEngineInDisconnectedClientNotificationThread=1;
			TRACE(4, "Initialized engine in '%s' thread.\n", threadName);
        }

		pthread_mutex_unlock(&monitDisconnectedClientNotificationLock);

		if ( initEngineInDisconnectedClientNotificationThread == 0 ) {
			continue;
		}


		concurrenttime = (uint64_t) ism_common_readTSC() ;

		/* 60 second interval for endpoint and topic monitoring stats does significantly
		 * limit the dynamism of an external monitoring application. Reduce the time
		 * interval to 2 seconds
		 */
		if ( concurrenttime >= (60 + ExternalLastTime60))
		{
			if ( startProcessingEngineEvents == 1 && (serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED) ) {
				/*Call Engine API*/
				TRACE(8, "Monitoring: DisconnectedClientNotification: Invoke Engine API for Shoulder Tap.\n");
				ism_time_t starttime = (uint64_t) ism_common_currentTimeNanos();
				/* check return code to satisfy compiler warning */
				int rc1 = ism_engine_generateDisconnectedClientNotifications();
				ism_time_t endtime = (uint64_t) ism_common_currentTimeNanos();
				TRACE(6, "DisconnectedClientNotification: rc=%d Time taken to complete the cycle (in nanosec): %lu\n", rc1, (endtime - starttime) );
			}
			//Submit Monitoring data for External Monitoring for the one for 60 seconds internval
			ExternalLastTime60 = concurrenttime;
		}


    }

    pthread_mutex_lock(&monitDisconnectedClientNotificationLock);
	DisconnectedClientNotification_thread_status = THREAD_STATUS_INACTIVE;
	pthread_mutex_unlock(&monitDisconnectedClientNotificationLock);

    return NULL;
}

/**
 * Start the thread for should tap (DisconnectedClientNotification).
 * This function will be called when there is a MessagingPolicy
 * had the DisconnectedClientNotification configuration enabled.
 */
XAPI int32_t ism_monitoring_startDisconnectedClientNotificationThread(void) {
	if(DisconnectedClientNotificationStarted==1){
		return ISMRC_OK;
	}

	DisconnectedClientNotificationStarted=1;

    pthread_mutex_init(&monitDisconnectedClientNotificationLock, 0);
    ism_common_cond_init_monotonic(&monitDisconnectedClientNotificationCond);


	pthread_mutex_lock(&monitDisconnectedClientNotificationLock);
	DisconnectedClientNotification_thread_status = THREAD_STATUS_STARTING;
	pthread_mutex_unlock(&monitDisconnectedClientNotificationLock);

    ism_threadh_t monitThread3;
    ism_common_startThread(&monitThread3, ism_monitoring_threadDisconnectedClientNotification, NULL, NULL,
        0, ISM_TUSAGE_NORMAL, 0, "DncClntNft", "The notification thread for DisconnectedClientNotification feature.");

    return ISMRC_OK;
}

/**
 * Set flag for engine events
 */
XAPI void ism_monitoring_startEngineEventFlag(void) {
	startProcessingEngineEvents = 1;
	return;
}

/**
 * Unset flag for engine events
 */
XAPI void ism_monitoring_stopEngineEventFlag(void) {
	startProcessingEngineEvents = 0;
	return;
}
