/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
 * pxactivity.h
 * Device activity and connectivity tracking.
 */

#ifndef PXACTIVITY_H_
#define PXACTIVITY_H_

#include "stdint.h"
#include "ismutil.h"

#ifdef __cplusplus
extern "C" {
#endif

//=== Life-cycle ==============================================================
/*
 * Start order:
 *   init()
 *   ConfigActivityDB_Set(...) : must be called on main thread before any other thread is started.
 *   start() : starts internal threads.
 */

/**
 * PXAct.EnableActivityTracking
 * Enable or disable activity tracking.
 *
 * A value of 0 means disabled.
 * A value of 1 means enabled.
 *
 * The type of the parameter is uint8_t.
 * The default value is 0
 *
 * This value is initialized with the command line (-a) option, and then may
 * be updated by configuration: config file, java connector, rest API.
 */
#define PXACT_CFG_ENABLE_ACTIVITY_TRACKING      "ActivityMonitoringEnable"

typedef enum
{
    PXACT_STATE_TYPE_NONE                      = 0,
    PXACT_STATE_TYPE_INIT                      = 1,
    PXACT_STATE_TYPE_CONFIG                    = 2,
    PXACT_STATE_TYPE_START                     = 3,
    PXACT_STATE_TYPE_STOP                      = 4,
    PXACT_STATE_TYPE_TERM                      = 5,
    PXACT_STATE_TYPE_ERROR                     = 6,
} PXACT_STATE_TYPE_t;

/**
 * Initialize data structures.
 * Must be called once by the main thread before any other call to the component.
 * @return ISMRC_OK, ISMRC_Error
 */
XAPI int ism_pxactivity_init();

/**
 * Get the state of the activity tracking component.
 * Valid only if called after ism_pxactivity_init().
 * Uses a single lock to guard the state, use only for non-frequent life cycle events.
 *
 * Viable state transitions:
 * NONE                     initial state
 * NONE    => INIT          call _init()
 * INIT    => CONFIG        call _configure() that returns OK
 * INIT    => INIT          call _configure() that returns not-OK
 * CONFIG  => START         call _start()
 * CONFIG  => INIT          call _configure() that returns not-OK
 * START   => STOP          call _stop()
 * START   => ERROR         internal error
 * START   => CLOSE         call _term()
 * STOP    => START         call _start()
 * STOP    => CLOSE         call _term()
 * ERROR   => STOP          call _stop() or _configure()
 * ERROR   => START         call _start()
 * ERROR   => CLOSE         call _term()
 * CLOSE   => CLOSE         end state
 *
 *@return
 */
XAPI PXACT_STATE_TYPE_t ism_pxactivity_get_state();

/**
 * Is started? lock free.
 * Equivalent to (PXACT_STATE_TYPE_START == ism_pxactivity_get_state()), but does not take a lock.
 * The visibility and ordering if the value returned by this method may be less accurate then getState().
 * Valid only if called after ism_pxactivity_init().
 *
 * @return 0: false, 1 true
 */
XAPI uint8_t ism_pxactivity_is_started();


//=== Component and Database configuration ==============================

/**
 * MongoDB default activity-tracking database name
 */
#define PXACT_MONGO_DB_NAME                     "activity_track"

/**
 * MongoDB default client-state collection name
 */
#define PXACT_MONGO_CLIENT_STATE_COLL_NAME      "client_state"

/**
 * MongoDB default proxy-state collection name
 */
#define PXACT_MONGO_PROXY_STATE_COLL_NAME       "proxy_state"

/**
 * Proxy heartbeat timeout, milliseconds.
 */
#define PXACT_PROXY_HEATBEAT_TIMEOUT_MS         60000

/**
 * Configures the database connection, as well as component tuning parameters.
 */
typedef struct ismPXACT_ConfigActivityDB_t
{
    /**
     * DB Type, "MongoDB".
     */
    const char*     pDBType;

    /**
     * MongoDB connection URI: endpoints.
     *
     * The endpoints part of the URI, in a JSON array.
     *
     * e.g. ["server1.ibm.com:27017", "server2.ibm.com:27017", "1.2.3.4:27017"]
     *
     */
    const char *      pMongoEndpoints;

    /**
     * Whether to use SSL/TLS to the MongoDB server (optional).
     *
     * Values: 0: false (no TLS), 1: true (TLS). Other values are reserved for future use.
     * Default: 0
     *
     * If true, adds "ssl=true" to the MongoDB URI options.
     * Used only if pMongoURI=NULL.
     */
    int        useTLS; //FIXME

    /**
     * MongoDB connection URI: authentication DB (optional).
     *
     * The authentication DB part of the URI. If missing, mongodb uses 'admin' by default.
     *
     * e.g. "myDB"
     * Used only if pMongoURI=NULL.
     */
    const char*     pMongoAuthDB;

    /**
     * The trust store to use to validate the server certificate.
     *
     * The certificate authority PEM file used to verify the server certificate (optional).
     * This must be a PEM file and is used to create the mongo options.
     *
     * Adds "sslCertificateAuthorityFile=<pem-file>" to the MongoDB URI options.
     *
     * Default: NULL.
     * Used only if pMongoURI=NULL.
     */
    const char*     pTrustStore;

    /**
     * MongoDB connection URI: additional options (optional).
     *
     * The options part of the MongoDB URI. Must be URI compliant.
     * Key-Value pairs separated by '&'.
     *
     * e.g. "replicaSet=rs0"
     * e.g. "replicaSet=rs0&sslAllowInvalidCertificates=true"
     *
     * Default: NULL.
     * Used only if pMongoURI=NULL.
     */
    const char*     pMongoOptions;

    /**
     * MongoDB user name for the database specified in pMongoDBName (optional).
     * This string must be URI compliant, without @ or :, if needed, use percent encoding.
     * The user must have at least readWrite role.
     * If NULL, connects to a database without access control.
     */
    const char*     pMongoUser;

    /**
     * MongoDB password for user specified in pMongoUser (optional).
     * This string must be URI compliant, without @ or :, if needed, use percent encoding.
     * If user is NULL, value is ignored.
     */
    const char*     pMongoPassword;

    /**
     * MongoDB database name (optional).
     * Note that the authentication database is taken from the URI,
     * not from this property (they may be the same though).
     *
     * Default: @see PXACT_MONGO_DB_NAME
     */
    const char*     pMongoDBName;

    /**
     * MongoDB client-state collection name (optional).
     * Default: @see PXACT_MONGO_CLIENT_STATE_COLL_NAME
     */
    const char*     pMongoClientStateColl;

    /**
     * MongoDB proxy-state collection name (optional).
     * Default: @see PXACT_MONGO_PROXY_STATE_COLL_NAME
     */
    const char*     pMongoProxyStateColl;

    /**
     * MongoDB write concern (optional).
     * 0: Default (DEFAULT = one); (k>0): k, (-1) majority;
     */
    int             mongoWconcern;

    /**
     * MongoDB write concern journal (ack policy), (optional).
     * 0: false, 1: true.
     */
    int             mongoWjournal;

    /**
     * MongoDB read concern (optional).
     * 0: Default (Local); (1): Local, (2) Majority;
     */
    int             mongoRconcern;

    /**
     * MongoDB read preference (optional).
     * 0: Default (PRIMARY), 1: PRIMARY, 2: SECONDARY,
     * 3: PRIMARY_PREFERRED, 4: SECONDARY_PREFERRED, 5: MONGOC_READ_NEAREST;
     */
    int             mongoRpref;

    /**
     * The number of worker threads in the component.
     * 0: Default (8 threads).
     */
    int             numWorkerThreads;

    /**
     * The number of banks per worker thread in the component.
     * 0: Default (4 banks).
     */
    int             numBanksPerWorkerThread;

    /**
     * Bulk write size.
     * 0: Default (=100).
     */
    int             bulkWriteSize;

    /**
     * Heart-beat timeout of proxy membership, in milliseconds.
     * 0=Default;
     */
    int             hbTimeoutMS;

    /**
     * Heartbeat interval of proxy membership, in milliseconds.
     * 0=Default: (Heart-beat-timeout/10);
     */
    int             hbIntervalMS;

    /**
     * Database I/O per second rate limit.
     * Value: >=1: limited, 0: unlimited
     * Default: 0 (unlimited).
     */
    int             dbIOPSRateLimit;

    /**
     * Whether the rate limit is global or local.
     * Enabled (Global): 1, Disabled (Local): 0.
     * Default: disabled.
     */
    int             dbRateLimitGlobalEnabled;

    /**
     * The memory limit of the client state cache, as percent of total memory.
     *
     * Default value: (0): disabled, no limit enforced
     * Value: >0: percemt of total memory
     */
    float           memoryLimitPercentOfTotal;

    /**
     * Termination timeout, in milliseconds.
     * The thread calling ism_pxact_term() will block until the conflation
     * buffers are empty or until this timeout.
     *
     * 0=Default: 10000ms.
     * (-1): no wait.
     */
    int             terminationTimeoutMS;

    /**
     * The interval, in seconds, after which a proxy record in status REMOVED
     * is deleted from the DB.
     *
     * 0=Default: 259200 (3 days).
     */
    int             removedProxyCleanupIntervalSec;

    /**
     * The monitoring and cleanup policy of device (d) client class.
     * A integer that defines whether this client class is monitored, and if yes,
     * what is the expiration time of records in the database.
     *
     * value:
     * (-1):  Monitoring disabled
     * 0:     No expiration
     * >0:    Expiration time, in seconds
     *
     * A value of (-1) disables monitoring for a client type.
     * A value >=0 only affects how the cache is being managed,
     * but not the actual expiration in the database, which handled by a different component.
     *
     * Default: 0.
     */
    int     monitoringPolicyDeviceClientClass;

    /**
     * The monitoring and cleanup policy of gateway (g) client class.
     * @see monitoringPolicyDeviceClientClass
     */
    int     monitoringPolicyGatewayClientClass;

    /**
     * The monitoring and cleanup policy of connector (c) client class.
     * @see monitoringPolicyDeviceClientClass
     */
    int     monitoringPolicyConnectorClientClass;

    /**
     * The monitoring and cleanup policy of the application (a) client class.
     * @see monitoringPolicyDeviceClientClass
     */
    int     monitoringPolicyAppClientClass;

    /**
     * The monitoring and cleanup policy of scalable application (A) client class.
     * @see monitoringPolicyDeviceClientClass
     */
    int     monitoringPolicyAppScaleClientClass;

} ismPXActivity_ConfigActivityDB_t;

/**
 * Initialize the configuration structure with the defaults.
 *
 * @param pConfigAffinityDB
 */
XAPI void ism_pxactivity_ConfigActivityDB_Init(ismPXActivity_ConfigActivityDB_t *pConfigAffinityDB);

/**
 * Configure the activity tracking component.
 *
 * @param pConfigAffinityDB
 * @return
 */
XAPI int ism_pxactivity_ConfigActivityDB_Set(ismPXActivity_ConfigActivityDB_t *pConfigAffinityDB);

/**
 * Signal to the activity module before the server is about to start messaging.
 * @return
 */
XAPI int ism_pxactivity_startMessaging();

/**
 * Is messaging started? lock free.
 *
 * @return 0: false, 1 true
 */
XAPI uint8_t ism_pxactivity_is_messaging_started();

/**
 * Start activity tracking.
 * @return
 */
XAPI int ism_pxactivity_start();

/**
 * Stop activity tracking.
 *
 * @return
 */
XAPI int ism_pxactivity_stop();

/**
 * Terminate.
 * Should be called once only, before the process exits.
 * @return
 */
XAPI int ism_pxactivity_term();

typedef void (*ismPXActivity_ShutdownHandler_t)(int core);

//=== MongoDB client_state collection data model ==========

#define PXACT_CLIENT_STATE_KEY_CLIENT_ID                "client_id"
#define PXACT_CLIENT_STATE_KEY_ORG                      "org"
#define PXACT_CLIENT_STATE_KEY_DEVICE_TYPE              "device_type"
#define PXACT_CLIENT_STATE_KEY_DEVICE_ID                "device_id"
#define PXACT_CLIENT_STATE_KEY_CLIENT_TYPE              "client_type"
#define PXACT_CLIENT_STATE_KEY_GATEWAY_ID               "gateway_id"

#define PXACT_CLIENT_STATE_KEY_CONN_EV_TYPE             "conn_ev_type"
#define PXACT_CLIENT_STATE_KEY_CONN_EV_TIME             "conn_ev_time"

#define PXACT_CLIENT_STATE_KEY_LAST_CONNECT_TIME        "last_connect_time"
#define PXACT_CLIENT_STATE_KEY_LAST_DISCONNECT_TIME     "last_disconnect_time"

#define PXACT_CLIENT_STATE_KEY_LAST_ACT_TYPE            "last_act_type"
#define PXACT_CLIENT_STATE_KEY_LAST_ACT_TIME            "last_act_time"

#define PXACT_CLIENT_STATE_KEY_LAST_PUB_TIME            "last_pub_time"


#define PXACT_CLIENT_STATE_KEY_PROTOCOL                 "protocol"
#define PXACT_CLIENT_STATE_KEY_CLEAN_S                  "clean_s"
#define PXACT_CLIENT_STATE_KEY_REASON_CODE              "reason_code"
#define PXACT_CLIENT_STATE_KEY_EXPIRY_SEC               "expiry_sec"

#define PXACT_CLIENT_STATE_KEY_TARGET_SERVER            "target_server"
#define PXACT_CLIENT_STATE_KEY_PROXY_UID                "proxy_uid"

#define PXACT_CLIENT_STATE_KEY_VERSION                  "version"

//=== MongoDB proxy_state collection data model ==========

#define PXACT_PROXY_STATE_KEY_PROXY_UID         "proxy_uid"
#define PXACT_PROXY_STATE_KEY_STATUS            "status"
#define PXACT_PROXY_STATE_KEY_LAST_HB           "last_hb"
#define PXACT_PROXY_STATE_KEY_HBTO_MS           "hbto_ms"
#define PXACT_PROXY_STATE_KEY_VERSION           "version"


//=== Tracking ============================================

/**
 * The client type, used in activity and connectivity events.
 *
 * Note: values must fit into int16_t.
 */
typedef enum
{
	PXACT_CLIENT_TYPE_NONE 						= 0,   //!< PXACT_CLIENT_TYPE_NONE
	PXACT_CLIENT_TYPE_DEVICE 					= 1,   //!< PXACT_CLIENT_TYPE_DEVICE Client class 'd'
	PXACT_CLIENT_TYPE_GATEWAY 					= 2,   //!< PXACT_CLIENT_TYPE_GATEWAY Client class 'g'
	PXACT_CLIENT_TYPE_APP 						= 3,   //!< PXACT_CLIENT_TYPE_APP Client class: "Small-a" application
	PXACT_CLIENT_TYPE_APP_SCALE 				= 4,   //!< PXACT_CLIENT_TYPE_APP_SCALE Client class: "Big-A" application
	PXACT_CLIENT_TYPE_DEVICE_BEHIND_GATEWAY 	= 5,   //!< PXACT_CLIENT_TYPE_DEVICE_BEHIND_GATEWAY (Not yet defined)
	PXACT_CLIENT_TYPE_CONNECTOR 				= 6,   //!< PXACT_CLIENT_TYPE_CONNECTOR (Not yet defined)
	PXACT_CLIENT_TYPE_OTHER 					= 100  //!< PXACT_CLIENT_TYPE_OTHER
} PXACT_CLIENT_TYPE_t;

/**
 * Target server type.
 *
 * The target server type defines whether a session from this device
 * is established with MessageSight, Kafka, or both. This determines
 * how we deal with client stealing.
 *
 * Note: values must fit into int8_t.
 */
typedef enum
{
	PXACT_TRG_SERVER_TYPE_NONE = 0,    		//!< PXACT_TRG_SERVER_TYPE_NONE
	PXACT_TRG_SERVER_TYPE_MS = 1,    		//!< PXACT_TRG_SERVER_TYPE_MS
	PXACT_TRG_SERVER_TYPE_KAFKA = 2,    	//!< PXACT_TRG_SERVER_TYPE_KAFKA
	PXACT_TRG_SERVER_TYPE_MS_KAFKA = 3,    	//!< PXACT_TRG_SERVER_TYPE_MS_KAFKA
} PXACT_TRG_SERVER_TYPE_t;


/**
 * Client activity types.
 *
 * Note: values must fit into int16_t.
 */
typedef enum
{
	PXACT_ACTIVITY_TYPE_NONE  					= 0,

	//Connectivity [100-300)
	//Connectivity - connect
	PXACT_ACTIVITY_TYPE_CONN					= 100, ///< Connect Range guard, begin
	PXACT_ACTIVITY_TYPE_MQTT_CONN,
	PXACT_ACTIVITY_TYPE_DBGw_CON,

	PXACT_ACTIVITY_TYPE_CONN_END				= 199, ///< Connect Range guard, end

	//Connectivity - disconnect
	PXACT_ACTIVITY_TYPE_DISCONN 				= 200, ///< Disconnect Range guard, begin
	PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT,
	PXACT_ACTIVITY_TYPE_MQTT_DISCONN_SERVER,
	PXACT_ACTIVITY_TYPE_MQTT_DISCONN_NETWORK,
	PXACT_ACTIVITY_TYPE_DBGw_DISCONN,

	PXACT_ACTIVITY_TYPE_DISCONN_END				= 299, ///< Disconnect Range guard, end


	//Activity [300,500)
	//MQTT activities
	PXACT_ACTIVITY_TYPE_MQTT					= 300, ///< MQTT activities Range guard, begin
	PXACT_ACTIVITY_TYPE_MQTT_SUB,
	PXACT_ACTIVITY_TYPE_MQTT_UNSUB,
	PXACT_ACTIVITY_TYPE_MQTT_PUBLISH,

	PXACT_ACTIVITY_TYPE_MQTT_END				= 399, ///< MQTT activities Range guard, end

	// HTTP messaging activities
	// TODO elaborate
	PXACT_ACTIVITY_TYPE_HTTP					= 400, ///< HTTP Range guard, begin
	PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT,
	PXACT_ACTIVITY_TYPE_HTTP_POST_COMMAND,
	PXACT_ACTIVITY_TYPE_HTTP_POST_COMMAND_REQ,
	PXACT_ACTIVITY_TYPE_HTTP_GET,
	PXACT_ACTIVITY_TYPE_HTTP_PUT,
	PXACT_ACTIVITY_TYPE_HTTP_DEL,

	PXACT_ACTIVITY_TYPE_HTTP_END				= 999, ///< HTTP Range guard, end

} PXACT_ACTIVITY_TYPE_t;

/**
 * Client connectivity protocol.
 *
 * Note: values must fit into int16_t.
 */
typedef enum
{
	PXACT_CONN_PROTO_TYPE_NONE 					= 0,
	PXACT_CONN_PROTO_TYPE_MQTT_v3_1				= 310,
	PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1			= 311,
	PXACT_CONN_PROTO_TYPE_MQTT_v5				= 500
} PXACT_CONN_PROTOCOL_TYPE_t;

/**
 * Private client activity error codes
 */
typedef enum
{
	PXACT_ERROR_START                           = 10000,
	PXACT_ERROR_DIRTY_FLAG_INVALID              = 10001,
	PXACT_ERROR_RETRY                           = 10002,
	PXACT_ERROR_END                             = 10003
} PXACT_ERROR_t;

/**
 * Client info.
 *
 *
 */
typedef struct ismPXACT_Client_t
{
	/**
	 * Client ID, mandatory in client activity reports.
	 */
	const char* pClientID;

	/**
	 * Org name,  mandatory in client activity reports.
	 */
	const char* pOrgName;

	/**
	 * The device type.
	 *
	 * For clients of type "device" (d), this is the deviceType;
	 * For clients of type "gateway" (g), this is the typeId;
	 * For clients of type "App" (a), this is not defined;
	 * For clients of type "App_Scale" (A), this is the instanceId; //TODO
	 */
	const char* pDeviceType;

	/**
	 * The device ID.
	 *
	 * For clients of type "device" (d), this is the deviceId;
	 * For clients of type "gateway" (g), this is the deviceId;
	 * For clients of type "App" (a), this is the appId;
	 * For clients of type "App_Scale" (A), this is the appId;
	 *
	 */
	const char* pDeviceID;

	/**
	 * Gateway ID, for devices behind gateway.
	 */
	const char* pGateWayID;

	/**
	 * Client Type (class).
	 */
	PXACT_CLIENT_TYPE_t clientType;

	/**
	 * Activity type.
	 *
	 * This field changes from invocation to invocation of
	 * clientConnectivity() / clientActivity(), to identify the
	 * activity that is reported.
	 */
	PXACT_ACTIVITY_TYPE_t activityType;

} ismPXACT_Client_t;

/**
 * Connectivity info.
 * Used to provide more information on MQTT connect and disconnect events.
 */
typedef struct ismPXACT_ConnectivityInfo_t
{
	/**
	 * A unique identifier for the connection.
	 * A value of 0 means "unknown".
	 */
	uint64_t connectionID;

	/**
	 * Connectivity protocol.
	 */
	PXACT_CONN_PROTOCOL_TYPE_t protocol;

	/**
	 * Session expiry interval, in seconds.
	 */
	uint32_t expiryIntervalSec;

	/**
	 * Target server type.
	 */
	PXACT_TRG_SERVER_TYPE_t targetServerType;

	/**
	 * Clean Session (v3), or Clean Start (v5).
	 *
	 * 1=true, 0=false.
	 */
	uint8_t cleanS;

	/**
	 * Disconnect reason code.
	 */
	uint8_t reasonCode;

	/**
	 * connect time
	 */
	uint64_t connectTime;

} ismPXACT_ConnectivityInfo_t;



/**
 * Client activity tracking.
 * For activities other than connectivity  events.
 *
 * @param pClient
 * @return
 */
XAPI int ism_pxactivity_Client_Activity(
		const ismPXACT_Client_t* pClient);

/**
 * Connectivity events.
 *
 * @param pClient
 * @param pConnInfo
 * @return
 */
XAPI int ism_pxactivity_Client_Connectivity(
		const ismPXACT_Client_t* pClient,
		const ismPXACT_ConnectivityInfo_t* pConnInfo);

//=== Policy dynamic changes ==============================

/**
 * Set the monitoring policy of a client type.
 * A value of (-1) disables monitoring for a client type.
 * A value >=0 only affects how the cache is being managed, but not the expiration in the database.
 * @param client_type
 * @param policy (-1): disable, 0: no expiration, >0 expiration in seconds.
 * @return
 */
XAPI int ism_pxactivity_MonitoringPolicy_set(PXACT_CLIENT_TYPE_t client_type, int policy);


//=== Stats ===============================================

typedef struct ismPXACT_Stats_t
{
    uint64_t activity_tracking_state;       ///< (g) activity tracking state; @see PXACT_STATE_TYPE_t

    //=== Database activity ===
    uint64_t num_db_insert;                 ///< (c) the number of MongoDB inserts (not in bulk)
    uint64_t num_db_update;                 ///< (c) the number of MongoDB updates (not in bulk)
    uint64_t num_db_bulk_update;            ///< (c) the number of MongoDB updates included in bulk writes
    uint64_t num_db_bulk;                   ///< (c) the number of MongoDB bulk writes
    uint64_t num_db_read;                   ///< (c) the number of MongoDB reads
    uint64_t num_db_delete;                 ///< (c) the number of MongoDB delete
    uint64_t num_db_error;                  ///< (c) the number of MongoDB errors
    uint64_t avg_db_read_latency_ms;        ///< (g) average read latency from MongoDB
    uint64_t avg_db_update_latency_ms;      ///< (g) average update latency to MongoDB
    uint64_t avg_db_bulk_update_latency_ms; ///< (g) average bulk update latency to MongoDB

    //=== Cache activity ===
    uint64_t num_activity;                  ///< (c) the number of clientActivity() calls
    uint64_t num_connectivity;              ///< (c) the number of clientConnectivity() calls
    uint64_t num_clients;                   ///< (g) the number of clients in cache
    uint64_t num_evict_clients;             ///< (c) the number of clients evicted from cache
    uint64_t num_new_clients;               ///< (c) the number of clients inserted to cache
    uint64_t memory_bytes;                  ///< (g) memory usage, bytes

    //=== Conflation ===
    uint64_t avg_conflation_delay_ms;       ///< (g) average time a record waiting in queue
    uint64_t avg_conflation_factor;         ///< (g) average number of events conflated in a record
} ismPXACT_Stats_t;

/**
 * The differential/absolute statistics between two calls.
 * The difference on (c) / absolute an (g).
 *
 * @param pStats
 * @return
 */
XAPI int ism_pxactivity_Stats_get(ismPXACT_Stats_t* pStats);


#ifdef __cplusplus
}
#endif

#endif /* PXACTIVITY_H_ */
