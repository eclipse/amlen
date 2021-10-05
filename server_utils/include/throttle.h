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

#ifndef THROTTLE_H
#define THROTTLE_H

#define THROTTLE_ENABLED_STR 						"Throttle.Enabled"
#define THROTTLE_ENABLED_STR_SIZE 					16
#define THROTTLE_LIMIT_STR 							"Throttle.Threshold.Limit."
#define THROTTLE_LIMIT_STR_SIZE 					25
#define THROTTLE_DELAY_STR 							"Throttle.Threshold.Delay."
#define THROTTLE_DELAY_STR_SIZE 					25
#define THROTTLE_FREQUENCY_STR 						"Throttle.Frequency"
#define THROTTLE_FREQUENCY_STR_SIZE 	18
#define THROTTLE_CONNCLOSEERROR_LIMIT_STR 			"Throttle.ConnCloseError.Limit"
#define THROTTLE_CONNCLOSEERROR_LIMIT_STR_SIZE 		29
#define THROTTLE_CONNCLOSEERROR_DELAY_STR 			"Throttle.ConnCloseError.Delay"
#define THROTTLE_CONNCLOSEERROR_DELAY_STR_SIZE 		29
#define THROTTLE_OBJECT_TTL_STR 					"ThrottleObjectTTL"
#define THROTTLE_OBJECT_TTL_STR_SIZE 				17

typedef enum ism_throttle_type_e {
    THROTTLET_AUTH_FAILED  			= 0,  /**< Authorization Failed    */
    THROTTLET_CLIENTID_STEAL 		= 1,  /**< Client ID Steal  */
    THROTTLET_HIGHEST_COUNT 		= 2,  /**< Highest between authorization failed or clientID steal  */
    THROTTLET_CONNCLOSEERR		 	= 3,  /**< Connection Close Error  */
} ism_throttle_type_e;

/*Throttle object*/
typedef struct ismThrottleObj_t {
		ism_time_t timestamp; //original or first timestamp
		int authFailedCount;
		ism_time_t authFailed_timestamp;
		int lastCloseRC;
		int clientIDStealCount;
		ism_time_t clientIDSteal_timestamp;
		int isConnectReqInQ;
		int connCloseErrorCount;
		ism_time_t connCloseErrorCount_timestamp;

} ismThrottleObj;

/*Proxy Client object*/
typedef struct ismDelay_t {
		int 		delay_time;		/*TIme in Milliseconds*/
		ism_time_t 	delay_in_nanos;	/*TIme in Nanos*/
		int 		limit;
} ismDelay;

/**
 * Initialize the Delay Process
 */
int ism_throttle_initThrottle(void);

/**
 * Terminate the Delay Process.
 */
int ism_throttle_termThrottle(void);

/**
 * Increase the count of failed authentication for the client
 * @param clientID	the client id
 * @return number of failures so far
 */
int ism_throttle_incrementAuthFailedCount(const char * clientID);

/**
 * Remove the client object from the throttle table.
 * @param clientID	the client id
 * @return number of authentication failed.
 */
int ism_throttle_removeThrottleObj(const char * clientID);

/**
 * Start the timer task to clean up the throttle table
 * @return 0 is OK. otherwise, it is an error.
 */
int ism_throttle_startDelayTableCleanUpTimerTask(void);

/**
 * Parse the configuration for throttle
 * @return number of configuration for throttle
 */
int ism_throttle_parseThrottleConfiguration(void);

/**
 * Get delay time in milliseconds
 * @param ilimit	the number failures
 * @return time in milliseconds
 */
int ism_throttle_getDelayTime(int ilimit);


/**
 * Get delay time in nanoseconds
 * @param ilimit	the number failures
 * @return time in nanoseconds
 */
ism_time_t ism_throttle_getDelayTimeInNanos(int ilimit);

/**
 * increment Client ID Steal count
 * @param clientID client id
 */
int ism_throttle_incrementClienIDStealCount(const char * clientID);

/**
 * Set the last close return code
 * @param clientID 	client id
 * @param rc the return code0
 */
int ism_throttle_setLastCloseRC(const char * clientID, int rc);


/**
 * Get throttle limit
 * @param clientID client id
 * @param type throttle type.
 */
int ism_throttle_getThrottleLimit(const char * clientID, ism_throttle_type_e type);

/**
 * Determine if throttle feature is enabled.
 * @return 1 for true, 0 for false.
 */
int ism_throttle_isEnabled(void);

/**
 * Set Throttle Frequency
 * @param freq the frequency in seconds
 * @return 0 for success. Otherwise, it is non-zero
 *
 */
int ism_throttle_setFrequency(int freq);

/**
 * Set Throttle Object Time to Live
 * @param freq the frequency in minutes. Default is 1.
 * @return 0 for success. Otherwise, it is non-zero
 *
 */
int ism_throttle_setObjectTTL(int ttl);

/**
 * Set Throttle Enabled
 * @param enabled 1 for enabled else disabled.
 *
 * @return 0 for success. Otherwise, it is non-zero
 *
 */
int ism_throttle_setEnabled(int enabled);

/**
 * Set whether the Connect is in progress
 * @param isConnectInProgress 	1 for connect in progress.
 * 								zero for not in progress.
 * @return current value. 1 for already in the timer queue. zero otherwise.
 */
int ism_throttle_setConnectReqInQ(const char *clientID, int isConnectInProgress) ;

/**
 * Get whether the Connect is in progress
 * @return	1 for connect in progress. zero for not in progress.
 */
int ism_throttle_getConnectReqInQ(const char *clientID) ;

/**
 * Increment the count for Connectino Close Error.
 * @param clientID the client ID
 * @param rc the closing rc
 * @return current value.
 */
int ism_throttle_incrementConnCloseError(const char * clientID, int rc) ;

/**
 * Get the delay time in milliseconds
 * @param ilimit	the input limit
 * @param type 		the throttling type
 * @return the delay time in milliseconds
 */
int ism_throttle_getDelayTimeByType(int ilimit, ism_throttle_type_e type);

/**
 * Get the delay time in nanaseconds by type
 * @param ilimit	the input limit
 * @param type 		the throttling type
 * @return the delay time in nanaseconds
 */
ism_time_t ism_throttle_getDelayTimeInNanosByType(int ilimit,ism_throttle_type_e type );

#endif
