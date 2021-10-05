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

#ifndef __MONIT_DEFINED
#define __MONIT_DEFINED


/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#include <ismutil.h>
#include <transport.h>
#include <engine.h>
#include <store.h>
#include <ha.h>
#include <ismjson.h>

/*
 * Monitoring action types
 */
typedef enum {
	ismMON_STAT_None          = 0,
	ismMON_STAT_Store         = 1,
	ismMON_STAT_Memory        = 2,
	ismMON_STAT_HA            = 3,
	ismMON_STAT_Endpoint      = 4,
	ismMON_STAT_Connection    = 5,
	ismMON_STAT_Subscription  = 6,
	ismMON_STAT_Topic         = 7,
	ismMON_STAT_Queue         = 8,
	ismMON_STAT_MQTTClient    = 9,
	ismMON_STAT_AdvEnginePD   = 10,
	ismMON_STAT_MemoryDetail  = 11,
	ismMON_STAT_DestinationMappingRule   = 12,
	ismMON_STAT_Security      = 13,
	ismMON_STAT_Transaction   = 14,
	ismMON_STAT_Cluster       = 15,
	ismMON_STAT_Forwarder     = 16,
	ismMON_STAT_ResourceSet   = 17
} ismMonitoringStatType_t;

XAPI int ism_monitoring_getSecurityStats(char * action, ism_json_parse_t * inputJSONObj, concat_alloc_t * outputBuffer);

/**
 * @brief  Process Monitoring action
 *
 * Called by protocol to process monitoring action (e.g. endpoint, topic statistics).
 *
 * @param[in]     transport        Pointer to transport object
 * @param[in]     inpbuf           Input buffer containing object name,
 *                                 action and any required arguments
 * @param[in]     buflen           Input buffer length
 * @param[out]    output_buffer    Returns results
 * @param[out]    rc               Return code
 *
 * Returns ISMRC_OK on success or ISMRC_*
 */
XAPI int ism_process_monitoring_action(ism_transport_t *transport, const char *inpbuf, int buflen, concat_alloc_t *output_buffer, int *rc);

/**
 * Initialize monitoring
 */
XAPI int32_t ism_monitoring_init(void);

/**
 * Terminate monitoring
 */
XAPI int32_t ism_monitoring_term(void);

/**
 * Start processing engine events in monitoring - called after engine and messaging start
 */
XAPI void ism_monitoring_startEngineEventFlag(void);

/**
 * Stop processing engine events in monitoring - called after engine and messaging start
 */
XAPI void ism_monitoring_stopEngineEventFlag(void);


/********************************NOTIFICATION PROTOTYPES*****************/

/*
 * Monitoring Object Types for the External monitorings
 */
typedef enum {
	ismMonObjectType_Server	      			 	= 0, /*Server Type*/
	ismMonObjectType_Endpoint		     		= 1, /*Endpoint Type*/
	ismMonObjectType_Connection	     			= 2, /*Connection Type*/
	ismMonObjectType_Topic		     			= 3, /*Topic Type*/
	ismMonObjectType_Queue		     			= 4, /*Queue Type*/
	ismMonObjectType_DestinationMappingRule    	= 5, /*DestinationMappingRule type*/
	ismMonObjectType_Memory				    	= 6, /*Memory Type*/
	ismMonObjectType_Store					   	= 7, /*Store Type*/
	ismMonObjectType_Plugin                     = 8  /*Plugin Type*/
} ismMonitoringObjectType_t;

/*
 * Monitoring Publishing Type
 */
typedef enum {
	ismPublishType_ASYNC	      			 	= 0, /*Asynchronous Publishing Type*/
	ismPublishType_SYNC		     				= 1, /*Synchronous Publishing Type*/
} ismMonitoringPublishType_t;

/**
 * This is a public API for other components to use to submit the external monitoring
 * data or submit the alerts.
 * @param objectType 		Monitoring object type
 * @param objectName 		The object name or ID
 * @param objectNameLen		The size of object name
 * @param messageData		The monitoring data/message
 * @param messageDataLen	The monitoring data length
 * @param publishType		Publishing type (Asyn or Sync type)
 * @return ISMRC_OK if the submission of event is succeeded. Otherwise, an error will be returned.
 */
XAPI int32_t  ism_monitoring_submitMonitoringEvent( ismMonitoringObjectType_t objectType,
							   						const char * objectName,
							   						int objectNameLen ,
							   						const char * messageData,
							   						int messageDataLen,
							   						ismMonitoringPublishType_t publishType);


/**
 * This function starts the thread for DisconnectedClientNotification feature.
 */
XAPI int32_t ism_monitoring_startDisconnectedClientNotificationThread(void) ;
/********************************NOTIFICATION END*****************/

/********************************Snapshot Data*******************/
#define DEFAULT_SNAPSHOTS_SHORTRANGE_INTERVAL  5          //in second
#define DEFAULT_SNAPSHOTS_SHORTRANGE6_INTERVAL  6          //in second
#define DEFAULT_SNAPSHOTS_LONGRANGE_INTERVAL   60         //in second
#define NUM_MONITORING_LIST   2

typedef uint64_t ism_snaptime_t;         /**< Time in second of snap shot */

/*
 * The snap shot data object
 *
 *
 */
typedef struct ism_snapshot_data_node_t {
    void *      		  data;   	 /**< A link list holding the monitoring_data */
    struct ism_snapshot_data_node_t  *  prev;
    struct ism_snapshot_data_node_t  *  next;
}ism_snapshot_data_node_t;

/*
 * The snap shot range object
 * This object will be created when the time is more than
 * the snapshot interval.
 *
 */
typedef struct ism_snapshot_range_t {
    ism_snapshot_data_node_t *      		  data_nodes;   	 /**< A link list holding the monitoring_data */
    ism_snapshot_data_node_t * 			      tail;              /**< A point to the last node of monitoring_data    */
    uint64_t              node_count;        /**< Count of nodes in the monitoring data list             */
    uint64_t              node_idle;         /**< Count of nodes do not updated. If it is exceed max_count, it will be removed */
    void *      		  userdata;   		 /**< User data which can identify the uniqueness of this node */
    pthread_spinlock_t    snplock;          /**< Lock over snapshots                            */
    void			      (*data_destroy)(void *data); //the function which will destroy or free the data structure in the ism_snapshot_data_node_t
    struct ism_snapshot_range_t * next;          		 /**< next snaprange object                           */
}ism_snapshot_range_t;


/*
 * monitoring snap object.
 * It will contains all aggregate data for an stat object. It is up to the GUI/CLI to define
 * the intervals for each snapshot.
 * In case of Memory SnapShot: range_list should be only 1.
 */
typedef struct ism_monitoring_snap_t {
	ism_snaptime_t   	last_snap_shot;      /**< Time in second of last snapshot    */
	ism_snaptime_t   	snap_interval;       /**< Time in second of update interval  */
    uint64_t         	max_count;           /**< Maximum count of nodes in the mondata list     */
    ism_snapshot_range_t 	*range_list;         /**< List of the stat object in the monitoring obj    */
}ism_monitoring_snap_t;

/*
 * The main monitoring snap List that hold all the monitoring aggregation data
 */
typedef struct ism_monitoring_snap_list_t {
	ism_monitoring_snap_t   **snapList;
	int               		 numOfSnapList;

}ism_monitoring_snap_list_t;

/**
 * Initialize the Monitoring Snapshot List
 * @param snapList 			snapshot list
 * @param short_interval 	short interval value for short interval list
 * @param long_interval 	long interval value for log interval list
 * @param return code
 */
XAPI int ism_monitoring_initMonitoringSnapList(ism_monitoring_snap_list_t **snapList, int short_interval, int long_interval );

/**
 * Get Snapshot List By Interval
 * @param snap_interval snapshot interval
 * @param snapList		snapshot list
 * @return snapshot object
 */
XAPI ism_monitoring_snap_t * ism_monitoring_getSnapshotListByInterval(ism_snaptime_t snap_interval, ism_monitoring_snap_list_t * snapList  );

/**
 * Create new instance of the snapshot range object
 * @param list 			snapshot range object
 * @param statType		statistics type
 * @param data_destroy	the destroyer of the data.
 * @retur a return code
 */
XAPI int ism_monitoring_newSnapshotRange(ism_snapshot_range_t ** list, int statType, void	(*data_destroy)(void *data));

/**
 * Update the Snapshot
 * @param curtime 	current time
 * @param stat		the input stat object. If stat is NULL, the function make the call to component to get the statistics.
 * @param statType	statistics type
 * @param snapList 	snapshot list
 */
XAPI int  ism_monitoring_updateSnapshot(ism_snaptime_t curtime, void * stat, int statType, ism_monitoring_snap_list_t * snapList);

/********************************Snapshot Data End*******************/

/**
 * Removes an inactive cluster member
 */
XAPI int ism_monitoring_removeInactiveClusterMember(ism_http_t *http);

typedef void (*ismMonitoring_CompletionCallback_t)(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext);

//****************************************************************************
/// @brief  Request diagnostics from the monitoring
///
/// Requests miscellaneous diagnostic information from the engine.
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_monitoring_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @remark If the return code is OK then diagnosticsOutput will be a string that
/// should be returned to the user and then freed using ism_monitoring_freeDiagnosticsOutput().
/// If the return code is ISMRC_AsyncCompletion then the callback function will be called
/// and if the retcode is OK, the handle will point to the string to show and then free with
/// ism_monitoring_freeDiagnosticsOutput()
///
/// @returns OK on successful completion
///          ISMRC_AsyncCompletion if gone asynchronous
///          or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_monitoring_diagnostics(
    const char *                    mode,
    const char *                    args,
    char **                         diagnosticsOutput,
    void *                          pContext,
    size_t                          contextLength,
    ismMonitoring_CompletionCallback_t  pCallbackFn);


//****************************************************************************
/// @brief  Free diagnostic info supplied by monitoring
///
/// @param[in]    diagnosticsOutput   string from ism_monitoring_diagnostics() or
///                                   returned as the handle to the callback function
///                                   from it
//****************************************************************************
XAPI void ism_monitoring_freeDiagnosticsOutput(
    char *                         diagnosticsOutput);

#ifdef __cplusplus
}
#endif

#endif

