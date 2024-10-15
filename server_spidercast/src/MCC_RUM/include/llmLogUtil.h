/*
 * Copyright (c) 2009-2021 Contributors to the Eclipse Foundation
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

#ifndef LLM_LOG_UTIL_H
#define LLM_LOG_UTIL_H

#include "llmUtils.h"
#include "llmCommonApi.h"

/* This will cause gcc to verify parameters for llmSimpleTraceInvoke and llmAddTraceData */
#define PRINTF_ATTRIB(x,y)  __attribute__ ((format (printf, x, y)));

/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

/* Logging configuration for a specific component */
typedef struct {
	int				componentId;		/* Unique component id */
	unsigned int	allowedLogLevel;    /* Allowed logging level */
} llmComponentLogCfg_t;

#define MAX_NUMBER_OF_KEYS			64
#define MAX_NUMBER_OF_COMPONENTS	64
#define MAX_COMPONENT_NAME_LENGTH	64

typedef char				COMPONENT_NAME[MAX_COMPONENT_NAME_LENGTH];

/**
 * Get time callback. The get_time callback will be used to get the time for each instance configuration
 */
typedef uint64_t (* llm_gettime_t)(void * userdata);

/* Logging configuration for the specific instance. Multiple configurations with different 
   filter ids are allowed. */
typedef struct {
  char*			        instanceName;							/* Instance name */
  int					filterID;								/* Filter id */
  llm_on_log_event_t	onLogEvent;								/* Callback function */
  void*					user;									/* user data provided to the application when callback method is called */
  unsigned int			allowedLogLevelDefault;					/* Default allowed log level. Used if nothing else is defined for a specific component and
																the message key does not appear in the exceptions lists */
  int					numOfAllowedKeys;						/* Number of message keys that are allowed independent of the log level */
  int					allowedKeys[MAX_NUMBER_OF_KEYS];		/* Array of message keys that are allowed independent of the log level */
  int					numOfRestrictedKeys;					/* Number of message keys that are NOT allowed independent of the log level */
  int					restrictedKeys[MAX_NUMBER_OF_KEYS];		/* Array of message keys that are NOT allowed independent of the log level */
  int					numOfComponentCfgs;						/* Number of component configurations */
  llmComponentLogCfg_t	componentCfgs[MAX_NUMBER_OF_COMPONENTS];/* Component configurations */ 
  llm_gettime_t		    get_time	;							/* Get time callback */
  void                  * get_time_param;				        /* Parameter for get_time callback*/			
} llmInstanceLogCfg_t;

	/*TraceComponent handle*/
typedef struct TraceComponent_t*	TCHandle;
	/*TraceBuffer handle*/
typedef struct TraceBuffer_t*	TBHandle;

   /* Logging utility errors */
typedef enum LOG_UTIL_ERRORS{
	INSTANCE_ALREADY_EXISTS = 1, 
	INSTANCE_UNKNOWN = 2, 
	INVALID_CONFIG_PARAM = 3,
	MAX_NUMBER_OF_COMPONENTS_EXEEDED = 4,
	COMPONENT_NOT_REGISTERED = 5,
	TOO_MANY_FILTERS_DEFINED = 6,
	NOT_ENOUGH_MEMORY = 7
} logUtilErrors_t;

/* APIs for logging configuration */

/* Sets the logging configuration 
*  config - configuration parameters
*  updateExisting - should it update an existing configuration or create a new one
*  errorCode - pointer to the error code
*  return 0 on success 
*/
LLM_API int llmSetInstanceLogConfig(const llmInstanceLogCfg_t *config, int updateExisting, int* errorCode);

/* Get the logging configuration 
*  config - configuration parameters (instance name and filter id must be set)
*  errorCode - pointer to the error code
*  return 0 on success 
*/
LLM_API int llmGetInstanceLogConfig(llmInstanceLogCfg_t *config, int* errorCode);

/* Changes the logging configuration for a component
*  instanceName - the instance name
*  filterID - the filter id
*  config - configuration parameters
*  errorCode - pointer to the error code
*  return 0 on success 
*/
LLM_API int llmChangeComponentLogConfig(const char* instanceName, int filterID, const llmComponentLogCfg_t *config, int* errorCode);

/* Remove logging configuration
*  instanceName - the instance name
*  filterID - the filter id
*  errorCode - pointer to the error code
*  return 0 on success 
*/
LLM_API int llmRemoveInstanceLogConfig(const char* instanceName, int filterID, int* errorCode);


/* Register trace component 
*  instanceName - the instance name
*  componentID - the component id
*  componentName - the component name
*  pHandle - pointer to TCHandle to be returned
*  errorCode - pointer to the error code
*  return 0 on success 
*/
LLM_API int	llmRegisterTraceComponent(const char* instanceName, int componentId, const char *componentName,
									  TCHandle* pHandle, int *errorCode);

/* Unregister trace component 
*  tcHandle - TCHandle to be released
*  errorCode - pointer to the error code
*  return 0 on success 
*/
LLM_API int llmUnregisterTraceComponent(TCHandle tcHandle, int *errorCode);

/* Checks if specific log entry is allowed 
*/
LLM_API int llmIsTraceAllowed(TCHandle tcHandle, unsigned int logLevel, int msgId);

/* Log single line */
LLM_API int llmSimpleTraceInvoke(TCHandle tcHandle, unsigned int logLevel, int msgId, 
								 const char* replacementDataTypes, const char* msgFormat, ...) PRINTF_ATTRIB(4,6) ;

/*
	Creates TraceBuffer for composite (e.g. multiline) log entries  
	Returns null if trace is not allowed
	NOTE: One of two methods (llmCompositeTraceInvoke or llmReleaseTraceBuffer) has to be called for
	the handle return by this call. Failure to do so will cause a memory leak 
*/
LLM_API TBHandle llmCreateTraceBuffer(TCHandle tcHandle, unsigned int logLevel, int msgId);

/* Add data to the trace buffer */
LLM_API int llmAddTraceData(TBHandle tbHandle, const char* replacementDataTypes, 
							const char* msgFormat, ...) PRINTF_ATTRIB(2,4) ;

/* Invoke the log entry (Will combine all data in Tracebuffer to one log entry) */
LLM_API int llmCompositeTraceInvoke(TBHandle tbHandle);

/*Discard the trace buffer*/
LLM_API int llmReleaseTraceBuffer(TBHandle tbHandle);

/*Get instance name of the trace component*/
LLM_API const char * llmGetTCInstanceName(const TCHandle tcHandle);


#ifdef __cplusplus
}
#endif

#endif /*LLM_LOG_UTIL_H*/

