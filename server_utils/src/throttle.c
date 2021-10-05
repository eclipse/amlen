/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
#define TRACE_COMP Mqtt
#include <ismutil.h>
#include <assert.h>
#include <sys/types.h>
#include <throttle.h>


#define BILLION        1000000000L
#define MILLION        1000000L
static pthread_spinlock_t g_throttleTableLock;
static pthread_spinlock_t g_throttleConfigLock;
static ismHashMap * g_throttletable;

static int throttleTableCleanUpTime = 1; //in Minutes. Default 1 Minutes
static ism_time_t throttleTableCleanUpTimeInNano = 0;
static int throttleObjectTTLTime = 60; //in Minutes. Default 60 Minutes
static ism_time_t throttleObjectTTLTimeNano = 0;
static int throttleFrequency=0;  //in Seconds. Default is zero.
static ism_time_t throttleFrequencyInNano=0;
static int throttleInited=0;
static int throttleTableCleanUpTaskStarted =0;
ism_timer_t  throttleTableTimer = NULL;
static int throttleEnabled=0;

#define THROTTLE_DELAY_MAX	64
int throttleLimitCount=0;
int throttleConfigInited=0;



ismDelay * throttleDelay[THROTTLE_DELAY_MAX];
ismDelay * throttleConnCloseErrorDelay=NULL;

static int removeThrottleConfiguration(void);

/*
 *
 */
static int cancelDelayTableTimer(void) {
	ism_time_t prev_delayTableCleanUpTimeInNano = throttleTableCleanUpTimeInNano;
	if (throttleTableTimer && throttleTableCleanUpTaskStarted) {
		TRACE(5, "Canceling throttleTableCleanUpTimerTask. started=%d. Previous throttleTableCleanUpTimeInNano=%llu.\n", throttleTableCleanUpTaskStarted, (ULL)prev_delayTableCleanUpTimeInNano);
		ism_common_cancelTimer(throttleTableTimer);
		throttleTableTimer = NULL;
		throttleTableCleanUpTaskStarted=0;
	}
	return 0;
}

/*
 * Increment the authorization failed ocunt
 */
int ism_throttle_incrementAuthFailedCount(const char * clientID) {
	int rcount = 0;
    void * item = NULL;
    ismThrottleObj * throttleObj = NULL;

    if (!throttleInited)
        return 0;

    pthread_spin_lock(&g_throttleTableLock);

    item = ism_common_getHashMapElement(g_throttletable, clientID, 0);
    ism_time_t ctime= ism_common_currentTimeNanos();

    if (item == NULL){
    	throttleObj = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,53),1, sizeof(ismThrottleObj));
    } else {
    	throttleObj = (ismThrottleObj *) item;
    }

    throttleObj->timestamp = ctime;

    /* If more than frequency, reset the count  */
    if (throttleFrequencyInNano > 0 && (ctime > (throttleFrequencyInNano + throttleObj->authFailed_timestamp))){
    	throttleObj->authFailed_timestamp=ctime;
    	throttleObj->authFailedCount=0;
    }

    rcount = throttleObj->authFailedCount++;

    ism_common_putHashMapElement(g_throttletable, clientID, 0, throttleObj, NULL);

    pthread_spin_unlock(&g_throttleTableLock);

	return rcount;
}


/*
 * Increment the client ID steal count
 */
int ism_throttle_incrementClienIDStealCount(const char * clientID) {
	int rcount=0;
    void * item = NULL;
    ismThrottleObj * throttleObj=NULL;

    if (!throttleInited)
        return 0;

    pthread_spin_lock(&g_throttleTableLock);

    item = ism_common_getHashMapElement(g_throttletable, clientID, 0);
    ism_time_t ctime= ism_common_currentTimeNanos();

    if (item == NULL){
    	throttleObj = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,54),1, sizeof(ismThrottleObj));
    } else {
    	throttleObj = (ismThrottleObj *) item;
    }

    throttleObj->timestamp = ctime;

    if (throttleFrequencyInNano > 0 && (ctime > (throttleObj->clientIDSteal_timestamp + throttleFrequencyInNano))){
		throttleObj->clientIDSteal_timestamp=ctime;
		throttleObj->clientIDStealCount=0;
	}

    throttleObj->lastCloseRC= ISMRC_ClientIDReused;

    rcount = throttleObj->clientIDStealCount++;

    ism_common_putHashMapElement(g_throttletable, clientID, 0, throttleObj, NULL);

    pthread_spin_unlock(&g_throttleTableLock);

	return rcount;
}

/*
 * Increment the Connection Close Error
 */
int ism_throttle_incrementConnCloseError(const char * clientID, int rc) {
	int rcount=0;
    void * item = NULL;
    ismThrottleObj * throttleObj=NULL;

    if (!throttleInited)
        return 0;

    pthread_spin_lock(&g_throttleTableLock);

    item = ism_common_getHashMapElement(g_throttletable, clientID, 0);
    ism_time_t ctime= ism_common_currentTimeNanos();

    if (item == NULL){
    	throttleObj = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,55),1, sizeof(ismThrottleObj));
    } else {
    	throttleObj = (ismThrottleObj *) item;
    }

    throttleObj->timestamp = ctime;

    if (throttleFrequencyInNano > 0 && (ctime > (throttleObj->connCloseErrorCount_timestamp + throttleFrequencyInNano))){
		throttleObj->connCloseErrorCount_timestamp=ctime;
		throttleObj->connCloseErrorCount=0;
	}

    throttleObj->lastCloseRC= rc;

    rcount = throttleObj->connCloseErrorCount++;

    ism_common_putHashMapElement(g_throttletable, clientID, 0, throttleObj, NULL);

    pthread_spin_unlock(&g_throttleTableLock);

	return rcount;
}

/**
 * Call this init after logInit
 */
int ism_throttle_initThrottle(void)
{

	if (!throttleInited){
		/*Init Log Table*/

		pthread_spin_init(&g_throttleTableLock, 0);
		pthread_spin_init(&g_throttleConfigLock, 0);
		g_throttletable = ism_common_createHashMap(128,HASH_STRING);
		throttleInited=1;

		TRACE(5, "Throttle Table is initialized.\n");
	}
	return 0;
}

/*
 * Terminate throttle processing
 */
int ism_throttle_termThrottle(void) {
	if (throttleInited) {
		throttleInited=0;

		cancelDelayTableTimer();

		pthread_spin_lock(&g_throttleTableLock);

		ismHashMapEntry **array = ism_common_getHashMapEntriesArray(g_throttletable);
		int i=0;
		ismThrottleObj * clientObj=NULL;
		while (array[i] != ((void*)-1)) {
			clientObj = (ismThrottleObj *)array[i]->value;
			ism_common_free(ism_memory_utils_misc,clientObj);
			clientObj=NULL;
			i++;
		}
		ism_common_freeHashMapEntriesArray(array);

		ism_common_destroyHashMap(g_throttletable);
		g_throttletable=NULL;


		pthread_spin_unlock(&g_throttleTableLock);

		pthread_spin_destroy(&g_throttleTableLock);

		pthread_spin_lock(&g_throttleConfigLock);
		/*Remove proxy delay objects*/
		removeThrottleConfiguration();
		throttleConfigInited=0;

		pthread_spin_unlock(&g_throttleConfigLock);

		pthread_spin_destroy(&g_throttleConfigLock);

		TRACE(5, "Throttle is terminated.\n");

	}

	return 0;
}

/**
 * Clean Up Task for Client Table
 * The clean up time is based on the configuration clientTableCleanUpInterval
 */
static int delayTableCleanUpTimerTask(ism_timer_t key, ism_time_t timestamp, void * userdata) {
	ismThrottleObj * throttleObj = NULL;
	int removedCount = 0;

	if (!throttleInited) {
		ism_common_cancelTimer(key);
	    return 0;
	}

	pthread_spin_lock(&g_throttleTableLock);

	ismHashMapEntry **dataEntries =  ism_common_getHashMapEntriesArray(g_throttletable);
	int i=0;
	TRACE(5, "throttleTableCleanUpTimerTask: count:%d\n",ism_common_getHashMapNumElements(g_throttletable) );
	while (dataEntries[i] != ((void*)-1)) {
		throttleObj = (ismThrottleObj *)dataEntries[i]->value;
		ism_time_t client_timestamp = throttleObj->timestamp;

		if ( timestamp >= ((client_timestamp) + throttleObjectTTLTimeNano)) {
			//Remove from the table.
			ism_common_removeHashMapElement(g_throttletable, dataEntries[i]->key, dataEntries[i]->key_len);
			if(throttleObj)
				ism_common_free(ism_memory_utils_misc,throttleObj);
			removedCount++;
		}
		i++;
	}

	TRACE(5, "throttleTableCleanUpTimerTask removed: count:%d\n",removedCount );

	ism_common_freeHashMapEntriesArray(dataEntries);

	pthread_spin_unlock(&g_throttleTableLock);

    return 1;
}



/**
 * Call this SPI after timers are inited.
 * @param cleanup_time	time in secs
 */
int ism_throttle_startDelayTableCleanUpTimerTask(void) {
	// ism_time_t prev_delayTableCleanUpTimeInNano = throttleTableCleanUpTimeInNano;
	throttleTableCleanUpTime = ism_common_getIntConfig("DelayTableCleanUpInterval", 1);
	throttleTableCleanUpTimeInNano =  throttleTableCleanUpTime * 60 * BILLION; //Minutes to Nanoseconds

	cancelDelayTableTimer();

	TRACE(5, "Start throttleTableCleanUpTimerTask. started=%d. throttleTableCleanUpTimeInNano=%llu.\n", throttleTableCleanUpTaskStarted, (ULL)throttleTableCleanUpTimeInNano);
	if (throttleTableCleanUpTaskStarted==0){
		throttleTableCleanUpTaskStarted=1;
		throttleTableTimer = ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t)delayTableCleanUpTimerTask, NULL, throttleTableCleanUpTimeInNano, throttleTableCleanUpTimeInNano, TS_NANOSECONDS);
		TRACE(5, "Throttle Table Clean Up Timer Task is set. CleanUpInterval: %d\n",throttleTableCleanUpTime);
	}

	return 0;
}

/*
 *
 */
int ism_throttle_removeThrottleObj(const char * clientID) {
	int rcount=0;
	int stealcount=0;
	int conncloseerr=0;
	void * item = NULL;
	ismThrottleObj * throttleObj=NULL;
	if (!throttleInited) return 0;

	pthread_spin_lock(&g_throttleTableLock);

	item = ism_common_getHashMapElement(g_throttletable, clientID, 0);

	if (item==NULL){
		pthread_spin_unlock(&g_throttleTableLock);
		return 0;
	} else {
		throttleObj = (ismThrottleObj *) item;
		if (throttleObj->lastCloseRC == ISMRC_OK || throttleObj->lastCloseRC == ISMRC_ClosedByServer){
			throttleObj= ism_common_removeHashMapElement(g_throttletable, clientID, 0) ;
			if (throttleObj!=NULL){
				rcount= throttleObj->authFailedCount;
				stealcount= throttleObj->clientIDStealCount;
				conncloseerr= throttleObj->connCloseErrorCount;
				ism_common_free(ism_memory_utils_misc,throttleObj);
			}
			TRACE(6, "Remove Entry from Throttle Table: ClientID=%s. FailedAuthNum=%d. StealCount=%d. ConnCloseErrorCount=%d\n", clientID, rcount, stealcount, conncloseerr);
		}
	}

	pthread_spin_unlock(&g_throttleTableLock);

	return rcount;

}

/**
 * remove delay configuration.
 * Must do this under the config lock
 * @return number of config removed.
 */
static int removeThrottleConfiguration(void)
{
	int i=0;
	/*Remove proxy delay objects*/
	if (throttleLimitCount>0){
		for(i=0; i<throttleLimitCount; i++){
			ismDelay * delay =  throttleDelay[i];
			ism_common_free(ism_memory_utils_misc,delay);
		}
		throttleLimitCount=0;
	}

	return i;

}

/**
 * Parse Throttle Configuration from properties
 * Note: must call ism_throttle_initThrottle before calling
 * this function
 *
 * @return number of delay configuration is added.
 *  zero if there is no configuration or already parsed
 *  -1 if the initDelay function is not get called
 */
int ism_throttle_parseThrottleConfiguration(void)
{
	/*Parse Throttle Configurations*/
	int i=0;
	const char * propName;
	const char * indexStr;

	if (!throttleInited) return -1;

	pthread_spin_lock(&g_throttleConfigLock);

	if (throttleConfigInited) {
		pthread_spin_unlock(&g_throttleConfigLock);
		return 0;
	}

	/*Remove previous configurations*/
	removeThrottleConfiguration();


	throttleEnabled = ism_common_getBooleanConfig(THROTTLE_ENABLED_STR, 0);
	TRACE(5, "Throttle Configuration: Enabled=%d\n", throttleEnabled);

	throttleTableCleanUpTime = ism_common_getIntConfig("DelayTableCleanUpInterval", 1);
	throttleTableCleanUpTimeInNano =  throttleTableCleanUpTime * 60 * BILLION; //Minutes to Nanoseconds

	ism_throttle_setObjectTTL(ism_common_getIntConfig(THROTTLE_OBJECT_TTL_STR, 60));

	ism_throttle_setFrequency(ism_common_getIntConfig(THROTTLE_FREQUENCY_STR, 0));

	ism_prop_t * props = ism_common_getConfigProperties();
	if (throttleEnabled){

		for (i = 0; ism_common_getPropertyIndex(props, i, &propName) == 0; i++) {
			int proplen = strlen(propName);
			//printf("PropName: %s\n", propName);
			if (proplen > THROTTLE_LIMIT_STR_SIZE && strncasecmp(THROTTLE_LIMIT_STR,propName, THROTTLE_LIMIT_STR_SIZE)==0){
				indexStr =propName+THROTTLE_LIMIT_STR_SIZE;
				int limitvalue = ism_common_getIntProperty(props, propName,0);

				char * delayName = alloca(THROTTLE_DELAY_STR_SIZE + strlen(indexStr) +1 );
				sprintf(delayName,"%s%s",THROTTLE_DELAY_STR, indexStr );

				int delayValue = ism_common_getIntConfig(delayName, 0);
				if (delayValue>0){
					/*If no delay value. Skip the creation of the object*/
					ismDelay * proxyDelay = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,60),1, sizeof(ismDelay));
					proxyDelay->delay_time = delayValue;
					proxyDelay->delay_in_nanos = delayValue * MILLION;
					proxyDelay->limit = limitvalue;
					TRACE(5, "Throttle Configuration: index=%d. Limit=%d. Delay=%d. DelayNanos=%llu\n",throttleLimitCount, limitvalue, delayValue, (ULL)proxyDelay->delay_in_nanos );
					throttleDelay[throttleLimitCount++]=proxyDelay ;
					if (throttleLimitCount == THROTTLE_DELAY_MAX){
						TRACE(5, "The maximum number of throttle delay configuration is reached. The maximum delay configuration is %d.\n", THROTTLE_DELAY_MAX);
						break;
					}
				} else {
					TRACE(5, "Invalid throttle configuration: %s\n", propName);
				}

			}else if(proplen == THROTTLE_CONNCLOSEERROR_LIMIT_STR_SIZE && strncasecmp(THROTTLE_CONNCLOSEERROR_LIMIT_STR,propName, THROTTLE_CONNCLOSEERROR_LIMIT_STR_SIZE)==0 ){

				if(throttleConnCloseErrorDelay!=NULL)
					ism_common_free(ism_memory_utils_misc,throttleConnCloseErrorDelay);

				throttleConnCloseErrorDelay =  ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,62),1, sizeof(ismDelay));

				int limitvalue = ism_common_getIntProperty(props, propName,0);
				int delayValue = ism_common_getIntConfig(THROTTLE_CONNCLOSEERROR_DELAY_STR, 0);
				if(delayValue){
					throttleConnCloseErrorDelay->delay_time = delayValue;
					throttleConnCloseErrorDelay->delay_in_nanos = delayValue * MILLION;
					throttleConnCloseErrorDelay->limit = limitvalue;

				}

				TRACE(5, "Throttle ConnCloseError Configuration: Limit=%d. Delay=%d. DelayNanos=%llu\n",  limitvalue, delayValue, (ULL)throttleConnCloseErrorDelay->delay_in_nanos );
			}
		}
	} else {
		TRACE(5, "Throttle Feature is disabled.\n");
	}


	throttleConfigInited=1;

	pthread_spin_unlock(&g_throttleConfigLock);

	return throttleLimitCount;

}


static ismDelay * getDelayObject(int ilimit, ism_throttle_type_e type)
{
	int count=0;
	ismDelay * delayobj1=NULL;
	ismDelay * delayobj2=NULL;
	ismDelay * redelayobj=NULL;

	if(type==THROTTLET_CONNCLOSEERR){
		if(ilimit >= throttleConnCloseErrorDelay->limit)
			redelayobj = throttleConnCloseErrorDelay;
	}else{
		for(count=0; count<throttleLimitCount; count++){

			delayobj1 = throttleDelay[count];

			if (count+1 != throttleLimitCount){
				delayobj2 =  throttleDelay[count+1];
			} else {
				delayobj2=NULL;
			}

			if (ilimit >= delayobj1->limit){
				if (delayobj2!=NULL){
					if (ilimit < delayobj2->limit){
						redelayobj = delayobj1;
						break;

					}
				} else {
					redelayobj = delayobj1;
					break;
				}
			}

		}
	}

	return redelayobj;

}
/**
 * Get the delay time in milliseconds
 * @param ilimit	the input limit
 * @return the delay time in milliseconds
 */
int ism_throttle_getDelayTime(int ilimit)
{

	int retDelay=0;
	pthread_spin_lock(&g_throttleConfigLock);
	ismDelay * delayObj =  getDelayObject(ilimit, THROTTLET_AUTH_FAILED);
	if (delayObj){
		retDelay = delayObj->delay_time;
	}
	pthread_spin_unlock(&g_throttleConfigLock);

	return retDelay;

}

/**
 * Get the delay time in nanaseconds
 * @param ilimit	the input limit
 * @return the delay time in nanaseconds
 */
ism_time_t ism_throttle_getDelayTimeInNanos(int ilimit)
{

	ism_time_t retDelay=0;
	pthread_spin_lock(&g_throttleConfigLock);
	ismDelay * delayObj =  getDelayObject(ilimit, THROTTLET_AUTH_FAILED);
	if (delayObj){
		retDelay = delayObj->delay_in_nanos;
	}
	pthread_spin_unlock(&g_throttleConfigLock);

	return retDelay;

}

/**
 * Get the delay time in milliseconds
 * @param ilimit	the input limit
 * @param type 		the throttling type
 * @return the delay time in milliseconds
 */
int ism_throttle_getDelayTimeByType(int ilimit, ism_throttle_type_e type)
{

	int retDelay=0;
	pthread_spin_lock(&g_throttleConfigLock);
	ismDelay * delayObj =  getDelayObject(ilimit, type);
	if (delayObj){
		retDelay = delayObj->delay_time;
	}
	pthread_spin_unlock(&g_throttleConfigLock);

	return retDelay;

}

/**
 * Get the delay time in nanaseconds by type
 * @param ilimit	the input limit
 * @param type 		the throttling type
 * @return the delay time in nanaseconds
 */
ism_time_t ism_throttle_getDelayTimeInNanosByType(int ilimit,ism_throttle_type_e type )
{

	ism_time_t retDelay=0;
	pthread_spin_lock(&g_throttleConfigLock);
	ismDelay * delayObj =  getDelayObject(ilimit, type);
	if (delayObj){
		retDelay = delayObj->delay_in_nanos;
	}
	pthread_spin_unlock(&g_throttleConfigLock);

	return retDelay;

}



int ism_throttle_getThrottleLimit(const char * clientID, ism_throttle_type_e type)
{

	int rcount=0;
	void * item = NULL;
	ismThrottleObj * throttleObj=NULL;
	if (!throttleInited) return 0;

	pthread_spin_lock(&g_throttleTableLock);

	item = ism_common_getHashMapElement(g_throttletable, clientID, 0);

	if (item==NULL){
		pthread_spin_unlock(&g_throttleTableLock);
		return 0;
	} else {
		throttleObj = (ismThrottleObj *) item;
	}

	if (type==THROTTLET_AUTH_FAILED){
		rcount = throttleObj->authFailedCount;
	}else if (type==THROTTLET_CLIENTID_STEAL){
		rcount = throttleObj->clientIDStealCount;
	}else if (type==THROTTLET_HIGHEST_COUNT){
		rcount = ( throttleObj->authFailedCount >= throttleObj->clientIDStealCount)
													?throttleObj->authFailedCount:throttleObj->clientIDStealCount;
	}else if(type==THROTTLET_CONNCLOSEERR){
		rcount = throttleObj->connCloseErrorCount;
	}


	pthread_spin_unlock(&g_throttleTableLock);


	return rcount;

}

int ism_throttle_setLastCloseRC(const char * clientID, int rc)
{

	void * item = NULL;
	ismThrottleObj * throttleObj=NULL;
	if (!throttleInited) return 0;

	pthread_spin_lock(&g_throttleTableLock);

	item = ism_common_getHashMapElement(g_throttletable, clientID, 0);

	if (item==NULL){
		pthread_spin_unlock(&g_throttleTableLock);
		return 0;
	} else {
		throttleObj = (ismThrottleObj *) item;
	}
	throttleObj->lastCloseRC = rc;


	pthread_spin_unlock(&g_throttleTableLock);


	return 0;

}

/**
 * Determine if throttle feature is enabled.
 * @return 1 for true, 0 for false.
 */
int ism_throttle_isEnabled(void)
{
	return throttleEnabled;
}

/**
 * Set Frenquency
 */
int ism_throttle_setFrequency(int freq)
{
	throttleFrequency = freq;
	if (throttleFrequency>0){
		throttleFrequencyInNano = throttleFrequency * BILLION;
	} else {
		throttleFrequencyInNano = 0;
	}
	TRACE(5, "Set Throttle Configuration: Frequency=%d. FrequencyInNanos=%llu\n",throttleFrequency, (ULL)throttleFrequencyInNano);
	return 0;

}

/**
 * Set Throttle Object Time To Live (TTL)
 */
int ism_throttle_setObjectTTL(int ttl)
{
	if(ttl<1)
		ttl=1;

	throttleObjectTTLTime = ttl;
	throttleObjectTTLTimeNano = throttleObjectTTLTime * 60 * BILLION; //Minutes to Nano

	TRACE(5, "Set Throttle Configuration: ObjectTTL=%d. ObjectTTLInNanos=%llu\n",throttleObjectTTLTime, (ULL)throttleObjectTTLTimeNano);
	return 0;

}

/*
 * Set Enabled property
 */
int ism_throttle_setEnabled(int enabled) {
    if(enabled == throttleEnabled){
        //Same value. No need to set
        TRACE(5, "Throttling setting is %d.\n", throttleEnabled );
        return 0;
    }
	if (enabled==1) {
		throttleEnabled = 1;
		if (throttleInited) {
			ism_throttle_startDelayTableCleanUpTimerTask();
		}
		TRACE(5, "Throttling is enabled.\n");
	} else {
		if (throttleInited){
			cancelDelayTableTimer();
		}
		throttleEnabled = 0;

		//Throttling is disabled. To Clean the throttle table.
		pthread_spin_lock(&g_throttleTableLock);


        ismHashMapEntry **array = ism_common_getHashMapEntriesArray(g_throttletable);
        int i=0;
        ismThrottleObj * clientObj=NULL;
        while (array[i] != ((void*)-1)) {
            clientObj = (ismThrottleObj *)array[i]->value;
            //Remove from the table.
            ism_common_removeHashMapElement(g_throttletable, array[i]->key, array[i]->key_len);
            if(clientObj)
                ism_common_free(ism_memory_utils_misc,clientObj);
            clientObj=NULL;
            i++;
        }
        ism_common_freeHashMapEntriesArray(array);

        pthread_spin_unlock(&g_throttleTableLock);

        TRACE(5, "Throttling is disabled. Cleaned throttle objects. Count=%d\n",i);

	}
	return 0;
}

/**
 * Set whether the Connect is in progress
 */
int ism_throttle_setConnectReqInQ(const char *clientID, int isConnectReqInQ) {
	void * item = NULL;
	ismThrottleObj * throttleObj=NULL;
	int oldVal = 0;

	if (!throttleInited) return oldVal;

	pthread_spin_lock(&g_throttleTableLock);

	item = ism_common_getHashMapElement(g_throttletable, clientID, 0);

	if (item==NULL){
		pthread_spin_unlock(&g_throttleTableLock);
		return oldVal;
	} else {
		throttleObj = (ismThrottleObj *) item;
		oldVal = throttleObj->isConnectReqInQ;
		throttleObj->isConnectReqInQ=isConnectReqInQ;
	}

	pthread_spin_unlock(&g_throttleTableLock);

	return oldVal;
}

/**
 * Get whether the Connect is in progress
 */
int ism_throttle_getConnectReqInQ(const char *clientID) {
	void * item = NULL;
	ismThrottleObj * throttleObj=NULL;
	int retValue =0;

	if (!throttleInited) return retValue;

	pthread_spin_lock(&g_throttleTableLock);

	item = ism_common_getHashMapElement(g_throttletable, clientID, 0);

	if (item==NULL){
		retValue =0;
	} else {
		throttleObj = (ismThrottleObj *) item;
		retValue = throttleObj->isConnectReqInQ;
	}

	pthread_spin_unlock(&g_throttleTableLock);

	return retValue;
}


