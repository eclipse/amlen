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
 * This module contains the protype and definitions for the external monitoring
 * or alert events. 
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
#include <monitoringutil.h>
#include <admin.h>

extern pthread_mutex_t monitConnLock;
extern pthread_cond_t  monitConnCond;

ism_ts_t *monEventTimeStampObj; 
pthread_spinlock_t    monEventTimeStampObjLock;  

/**
  * monitoring event information
  *
  * This structure contains information which is passed to the application for each
  * notification entry.
  */
typedef struct ism_monitoring_monitoringPublishEvent {
    struct ism_monitoring_monitoringPublishEvent * next;
    ism_time_t  		   	 	timestamp;              /* Timestamp when event was created   */
 	ismMonitoringObjectType_t	objectType;				/* Object Type */
    char *						messageData;			/* Data or payload to send out*/
    int							messageDataLen;			/* Size of the data*/ 
    char *						objectName;				/* Object Name*/
    int							objectNameLen;			/* Object Name len*/

} ism_monitoring_monitoringPublishEvent;

/*
 * Locks and event queue
 */
static pthread_mutex_t    notificationSyncLock;

static ism_monitoring_monitoringPublishEvent *    notificationHead;
static ism_monitoring_monitoringPublishEvent *    notificationTail;

/*
 * Create the properties with the topic when it is known to be small.
 *
 * @param topic   The topic name
 * @param outbuf  The output buffer.  This must be at least 5 bytes longer than the topic string.
 * @return lenghth of the topic property
 */
char * topicProperty(const char * topic, char * outbuf) {
	
    int  topiclen = (int)strlen(topic);

    if (topiclen > 248 || outbuf == NULL) {
       return NULL;
    }      
    outbuf[0] = 0x15;             /* S_ID+1  */
    outbuf[1] = 0x09;             /* ID_Topic */
    outbuf[2] = 0x29;             /* S_StrLen+1 */
    outbuf[3] = topiclen + 1;     /* Length of the topic string (including the null) */
    strcpy(outbuf+4, topic);      /* The topic string (including the null) */
    return outbuf;
}

/**
 * Create a message
 *
 * @param[out]    phMessage          Pointer to receive the hMessage
 * @param[in,out] pPayload           Pointer to receive pointer to payload
 * @param[in]     payloadSize        Size that the message payload should be
 *                                   or size that it is if supplied
 
 *
 * @return OK on successful completion or an ISMRC_ value.
 */
int32_t ism_monitoring_createEngineMessage(  ismEngine_MessageHandle_t *phMessage,
													const char * topicStr,
                           							void **ppPayload, 
                           							size_t payloadSize)
{
    int32_t rc = OK;
    void *local_payload;

    if (payloadSize == 0 || ppPayload==NULL || *ppPayload==NULL || topicStr==NULL || phMessage==NULL)
    {
    	return ISMRC_NullArgument;
    }
   
    local_payload = *ppPayload;
       
    ismEngine_MessageHandle_t hMessage = NULL;
    ismMessageHeader_t hdr;
    
    /*
     * Set up the header
     */
    memset(&hdr, 0, sizeof hdr);
    hdr.Persistence = 0;
    hdr.Reliability = 0;
    hdr.Priority = 4;
    hdr.RedeliveryCount = 0;
    hdr.Expiry = 0;
    hdr.Flags = ismMESSAGE_FLAGS_NONE;
    hdr.MessageType = MTYPE_MQTT_Text;
    
    

    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengths[2];
    void *areaData[2];
    
    /*Create the Topic Properties*/
    size_t tlen = strlen(topicStr)+5;
    char * topicProps  = alloca(tlen);
    topicProps = topicProperty(topicStr, topicProps);
    if(topicProps==NULL) tlen=0;
    
    /*Set the Properties into the message area */
    areaLengths[0] = tlen;
    areaData[0] = tlen ? topicProps : NULL;
    areaLengths[1] = payloadSize;
    areaData[1] = local_payload;
    
    /*Create then Engine message handle*/
    rc = ism_engine_createMessage(&hdr,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);

    if (rc != OK)
    {
       TRACE(6, "Failed to create message. Error code: %d\n", rc);
       goto mod_exit;
    }

    if (rc == OK)
    {
        *phMessage = hMessage;

    }


mod_exit:

    return rc;
}

/**
 *  Initialize the notification function
 */
void ism_monitoring_initNotification(void)
{
	 /************************************************************************/
    /* initialize mutex                                                     */
    /************************************************************************/
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&notificationSyncLock, &attr);
    
    
    
    notificationTail=NULL;
    notificationHead=NULL;
    
    /*Open TimeStamp Object*/
    monEventTimeStampObj =  ism_common_openTimestamp(ISM_TZF_UTC);
    pthread_spin_init(&monEventTimeStampObjLock, 0);
  
}

/**
 * Terminate the notifiation process
 */
void ism_monitoring_termNotification(void)
{
	
	pthread_mutex_destroy(&notificationSyncLock);
	
	/*Close TimeStamp Object*/
	ism_common_closeTimestamp(monEventTimeStampObj);
	pthread_spin_destroy(&monEventTimeStampObjLock);
	
}

/**
 * This function creates an engine Message handle and construct
 * the topic and publish the monitoring or alert message
 * @param notificationEvent the notification event object
 *.@return ISMRC_OK if the publish is OK. Otherwise, an error will be returned
 */
static int ism_monitoring_publishEvent(ism_monitoring_monitoringPublishEvent * notificationEvent) {
    
	
	uint32_t rc=ISMRC_OK;
	
    if(serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED){
        
        char topicStr[1024]={0};
	    ism_monitoring_getExtMonTopic(notificationEvent->objectType, (char *) &topicStr);
        
        ismEngine_MessageHandle_t message;
		
		rc = ism_monitoring_createEngineMessage( 	&message, 
													(const char *)&topicStr,
                                					(void *) &notificationEvent->messageData, 
                                					(size_t)notificationEvent->messageDataLen);
        
        
      	if(rc==ISMRC_OK){
			
			if(ism_common_getTraceLevel()==9){
		        /*Trace Monitoring Publish Event.*/
		       char *tMsgStr = alloca(notificationEvent->messageDataLen+1);
		       memcpy(tMsgStr, notificationEvent->messageData, notificationEvent->messageDataLen);
		       tMsgStr[notificationEvent->messageDataLen]='\0';
		        
		        TRACE(9, "Publish Monitoring Event: Message=%s\n",tMsgStr);
		    }
		    
	        rc = ism_engine_putMessageInternalOnDestination ( ismDESTINATION_TOPIC,
	                                                		  topicStr,
	                                                          message,
	                                                          NULL,
	                                                          0,
	                                                          NULL);
			if(rc!=ISMRC_OK){
				TRACE(5, "Failed to publish engine message: Error Code: %d\n", rc);
			}
       }else{
       		TRACE(5, "Failed to create engine message: Error Code: %d\n", rc);
       }
       
     
                                            
    }else{
    	TRACE(5, "Failed to publish monitoring event. The server state is not RUNNING.\n");
    }
    
    return rc;
    
    
}


/**
 * This function is invoked from the monitoring thread. It process
 * all the events which submitted. 
 */
void ism_monitoring_processNotifications(void)
{

	ism_monitoring_monitoringPublishEvent * notificationent;
	int rc=ISMRC_OK;
	
	if((serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED)){
		
		pthread_mutex_lock(&monitConnLock);
		
		while (notificationHead != NULL) {
	   	    /* Select the first item */
	   	    notificationent = notificationHead;
	        notificationHead = notificationent->next;
	        if (notificationHead == NULL)
	            notificationTail = NULL;
	        
	        /*Unlock before process the event*/
	        pthread_mutex_unlock(&monitConnLock);
	        
	        /*Process the notification.*/
	        rc=ism_monitoring_publishEvent(notificationent);
	        
	        if(rc!=ISMRC_OK){
	        	TRACE(6,"Failed to publish the event. Error code: %d.\n", rc);
	        }
	        
	        ism_common_free(ism_memory_monitoring_misc,notificationent);
	       
	        pthread_mutex_lock(&monitConnLock);
	        
	    }
	    
	    pthread_mutex_unlock(&monitConnLock);
	    
    }
    
} 
/**
 * This is a public API for other components to use to submit the external monitoring 
 * data or submit the alerts.
 */
XAPI int32_t  ism_monitoring_submitMonitoringEvent( ismMonitoringObjectType_t objectType,
							   						const char * objectName,
							   						int objectNameLen ,
							   						const char * messageData, 
							   						int messageDataLen,
							   						ismMonitoringPublishType_t publishType)
{
	int rc = ISMRC_OK;
	ism_monitoring_monitoringPublishEvent * notificationent;
	
	if(messageData==NULL || messageDataLen==0 ){
		TRACE(6,"Failed to submit the monitoring event. Message Data or Data Length is not valid.\n");
		return ISMRC_InvalidParameter;
	}
	
	/*Only submit if the server state is RUNNING or STARTED*/
	if((serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED)){
	
		/*Trace every event that is submitted.*/
		TRACE(9, "Submit the Monitoring Event: objectType=%d, objectName=%s, publishType=%d\n", 
				objectType, objectName!=NULL?objectName:"*UNKNOWN*", publishType);
				
		int size = sizeof(ism_monitoring_monitoringPublishEvent) +objectNameLen+messageDataLen ;
		notificationent = ism_common_calloc(ISM_MEM_PROBE(ism_memory_monitoring_misc,22),1, size);
	    
	    if (notificationent) {
	        notificationent->timestamp = ism_common_currentTimeNanos();
	
	        notificationent->objectType = objectType;
	        
	        
	        notificationent->objectName = (char *)(notificationent+1);
	        notificationent->objectNameLen = objectNameLen;
	        memcpy(notificationent->objectName, objectName, objectNameLen);

	     	notificationent->messageData = (char *)(notificationent->objectName+objectNameLen);
	        notificationent->messageDataLen = messageDataLen;
	        memcpy(notificationent->messageData, messageData, messageDataLen);
	        
		    if(publishType==ismPublishType_ASYNC){
		    
		    	/*Asynchronous Event*/
		    	
		    	/*
		     	* Insert the notification event item at the tail of the notification event queue
		     	*/
			    pthread_mutex_lock(&monitConnLock);
			    if (notificationTail!=NULL) {
			        notificationTail->next = notificationent;
			        notificationTail = notificationent;
			    } else {
			        notificationTail = notificationent;
			        notificationHead = notificationent;
			    }
			    pthread_cond_signal(&monitConnCond);
			    pthread_mutex_unlock(&monitConnLock);
	        }else{
	        	/*Synchronous event*/
	        	if((serverState == ISM_SERVER_RUNNING || serverState == ISM_MESSAGING_STARTED)){
	        	    rc = ism_monitoring_publishEvent(notificationent);
	        	}
	        	if(rc!=ISMRC_OK){
	        		TRACE(6,"Failed to publish the event. Error code: %d.\n", rc);
	        	}
	        	ism_common_free(ism_memory_monitoring_misc,notificationent);
	        }
	        
	    }else{
	    	rc = ISMRC_AllocateError;
	    }
	    
	    
	}else{
		TRACE(9, "Failed to submit the Monitoring Event: objectType=%d, objectName=%s, publishType=%d. The server is not in the RUNNING state.\n", 
				objectType, objectName!=NULL?objectName:"*UNKNOWN*", publishType);
		rc=ISMRC_Failure;
	}
	
	return rc;
}
