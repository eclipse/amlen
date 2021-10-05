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
 * ActivityUtils.h
 *  Created on: Oct 29, 2017
 */

#ifndef ACTIVITYUTILS_H_
#define ACTIVITYUTILS_H_

#include <set>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <sstream>
#include <openssl/rand.h>

#include <ismutil.h>
#include "pxactivity.h"
#include "MurmurHash3.h"

namespace ism_proxy
{

//===========================================================================

using LockGuardRecur = std::lock_guard<std::recursive_mutex>;
using LockGuard = std::lock_guard<std::mutex>;

//=== UID ===================================================================

static const char* const PROXY_UID_NULL = "?";

static const char base64url[] = {
		'A','B','C','D','E','F','G','H','I','J','K','L','M',
		'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
		'a','b','c','d','e','f','g','h','i','j','k','l','m',
		'n','o','p','q','r','s','t','u','v','w','x','y','z',
		'0','1','2','3','4','5','6','7','8','9',
		'-','_'
   };

/**
 * K random characters, from base64url.
 *
 * Standard 'base64url' with URL and Filename Safe Alphabet
 * (RFC 4648 §5 Table 2: The "URL and Filename safe" Base 64 Alphabet).
 *
 * (i.e. 'A-Z', 'a-z', '0-9', '-', '_').
 *
 * @return
 */
std::string generateProxyUID(const int k);

//=== Trim strings ==========================================================

std::string& trim_right_inplace(
        std::string& s, const std::string& delimiters = " \f\n\r\t\v");

std::string& trim_left_inplace(
        std::string& s, const std::string& delimiters = " \f\n\r\t\v");

std::string& trim_inplace(
        std::string& s, const std::string& delimiters = " \f\n\r\t\v");


using StringSet = std::set<std::string>;
std::string randomSelect(const StringSet& sset);


//=== ActivityDBConfig ========================================================

class ActivityDBConfig
{
public:
    static constexpr int NUM_WORKER_THREADS_DEF = 8;
    static constexpr int NUM_BANKS_PER_WORKER_DEF = 4;
    static constexpr int BULK_SIZE_DEF = 100;
    static constexpr int TERM_TO_MS_DEF = 10000;
    static constexpr int REM_PROXY_CLEANUP_TO_SEC_DEF = 3*24*3600;

	ActivityDBConfig();
	explicit ActivityDBConfig(const ismPXActivity_ConfigActivityDB_t*);
	virtual ~ActivityDBConfig();

    /**
     * Construct a URI with user password, if they exist.
     *
     * If the user argument is not empty, insert (or replace) the user:pwd into the uri.
     * If user is empty, the uri is used unchanged.
     *
     * @param uri original uri
     * @param user user-name, or empty string
     * @param pwd password, or empty string
     * @return new uri, with user:pwd inserted or replaced, or original uri.
     */
    static std::string build_uri(const std::string& uri, const std::string& user, const std::string& pwd);

    /**
     * Assemble the URI from fragments.
     *
     * e.g.
     * mongodb://user:password@server1.ibm.com:27017,server2.ibm.com:27017/authDB?options
     *
     * @param user
     * @param password
     * @param endpoints a JSON array containing server endpoints. e.g. ["server1.ibm.com:27017", "server2.ibm.com:27017"]
     * @param authDB the name of the authentication BD
     * @param options the options
     * @return
     */
    static std::string assemble_uri(const std::string& user, const std::string& password,
            const std::string&  endpoints, const std::string&  authDB, int useTLS, const std::string& ca_file, const std::string& options);

    /**
     * Analyze the changes between current and update.
     *
     * @param update the updated config
     * @return first: is different? second: need db-client restart?
     */
    std::pair<bool,int> compare(const ActivityDBConfig& update) const;

    /**
     * Substitute the password (if exists) with "********".
     *
     * @param uri
     * @return
     */
    static std::string mask_pwd_uri(const std::string& uri);

    /**
     * host name length from endoint
     * @param endpoint
     * @return
     */
    static int hostLength(const char* endpoint);

    //static int parse_uri(const std::string& uri, std::string& scheme, std::string& authority, std::string& database, std::string& options);

	std::string toString() const;

	/**
	 * DB Type, "MongoDB".
	 */
	std::string	   	dbType;

	/**
	 * MongoDB connection URI.
	 * e.g. mongodb://server1.ibm.com:27017,server2.ibm.com:27017
	 */
	std::string   	mongoURI;

	/**
	 * MongoDB connection URI: endpoints.
	 *
	 * The endpoints part of the URI, in a JSON array.
	 *
	 * Use EITHER the full URI (pMongoURI) OR in parts: pMongoEndpoints (this one) + [pMongoAuthDB] + [pMongoOptions].
	 *
	 * e.g. ["server1.ibm.com:27017", "server2.ibm.com:27017", "1.2.3.4:27017"]
	 *
	 */
	std::string     mongoEndpoints;

	/**
	 * MongoDB connection URI: authentication DB (optional).
	 *
	 * The authentication DB part of the URI. If missing, mongodb uses 'admin' by default.
	 *
	 * e.g. "myDB"
	 *
	 */
	std::string     mongoAuthDB;

	/**
	 *
	 */
	int useTLS;

	/**
	 *
	 */
	std::string     mongoCAFile;

	/**
	 * MongoDB connection URI: options (optional).
	 *
	 * The options part of the URI. Must be URI compliant.
	 *
	 * e.g. "replicaSet=rs0,ssl=true"
	 *
	 */
	std::string     mongoOptions;

    /**
     * MongoDB database name (optional).
     * Default: @see PXACT_MONGO_DB_NAME
     */
	std::string    	mongoDBName;

	/**
	 * MongoDB client-state collection name (optional).
	 * Default: @see PXACT_MONGO_CLIENT_STATE_COLL_NAME
	 */
	std::string    	mongoClientStateColl;

	/**
	 * MongoDB proxy-state collection name (optional).
	 * Default: @see PXACT_MONGO_PROXY_STATE_COLL_NAME
	 */
	std::string    	mongoProxyStateColl;

	/**
	 * MongoDB user name for the database specified in pMongoDBName (optional).
	 * The user must have at least readWrite role.
	 * If NULL, connects to a database without access control.
	 */
	std::string		mongoUser;

	/**
	 * MongoDB password for user specified in pMongoUser (optional).
	 * If user is NULL, value is ignored.
	 */
	std::string 	mongoPassword;

    /**
     * MongoDB write concern (optional).
     * 0: Default (DEFAULT = one); (k>0): k, (-1) majority;
     */
	int 			mongoWconcern;

	/**
	 * MongoDB write concern journal (ack policy), (optional).
	 * 0: false, 1: true.
	 */
	int 			mongoWjournal;

	/**
	 * MongoDB read concern (optional).
	 * 0: Default (Local); (1): Local, (2) Majority;
	 */
	int 			mongoRconcern;

	/**
	 * MongoDB read preference (optional).
	 * 0: Default (PRIMARY), 1: PRIMARY, 2: SECONDARY,
	 * 3: PRIMARY_PREFERRED, 4: SECONDARY_PREFERRED, 5: MONGOC_READ_NEAREST;
	 */
	int 			mongoRpref;

	/**
	 * The number of worker threads in the component.
	 * 0: Default (8 threads).
	 */
    int            	numWorkerThreads;

    /**
     * The number banks per worker thread in the component.
     * 0: Default (4 threads).
     */
    int            	numBanksPerWorkerThread;

    /**
     * Bulk write size.
     * 0: Default (=100).
     */
    int            	bulkWriteSize;

    /**
     * Heart-beat timeout of proxy membership, in milliseconds.
     * 0=Default: 20000ms;
     */
    int 		   	hbTimeoutMS;

    /**
     * Heartbeat interval of proxy membership, in milliseconds.
     * 0=Default: (Heart-beat-timeout/10)=2000ms;
     */
    int            	hbIntervalMS;

    /**
     * Database I/O per second rate limit.
     * Value: >=1: limited, 0: unlimited
     * Default: 0 (unlimited).
     */
    int 			dbIOPSRateLimit;

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
    float			memoryLimitPercentOfTotal;

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
    int removedProxyCleanupIntervalSec;

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

};

using ActivityDBConfig_SPtr = std::shared_ptr<ActivityDBConfig>;

//=== Activity Stats ==========================================================

class ActivityStats
{
public:
	ActivityStats();
	virtual ~ActivityStats();

	std::string toString() const;

	void norm(int numBanks, int numWorkers);

	void copyTo(ismPXACT_Stats_t* pStats);

	friend ActivityStats operator-(const ActivityStats& x, const ActivityStats& y);
	friend ActivityStats operator+(const ActivityStats& x, const ActivityStats& y);

	int64_t num_db_insert;              ///< cumulative
	int64_t num_db_update;              ///< cumulative
	int64_t num_db_bulk_update;         ///< cumulative
	int64_t num_db_bulk;                ///< cumulative
	int64_t num_db_delete;              ///< cumulative
	int64_t num_db_read;                ///< cumulative
	int64_t num_db_error;               ///< cumulative
	int64_t avg_db_read_latency_ms;            ///< gauge
	int64_t avg_db_update_latency_ms;          ///< gauge
	int64_t avg_db_bulk_update_latency_ms;     ///< gauge

	int64_t num_activity;               ///< cumulative
	int64_t num_connectivity;           ///< cumulative

	int64_t num_clients;                ///< gauge
	int64_t num_evict_clients;          ///< cumulative
	int64_t num_new_clients;            ///< cumulative

	int64_t memory_bytes;               ///< gauge

	int64_t avg_conflation_delay_ms;    ///< gauge
	int64_t avg_conflation_factor;      ///< gauge

	double duration_sec;                ///< from beginning
};
//=== C-String utilities for std::map and std::unordered_map with c-string keys ===

struct CStringLess
{
	bool operator()(char const* p1, char const* p2) const
	{
		return strcmp(p1, p2) < 0;
	}
};


struct CStringGreater
{
	bool operator()(char const* p1, char const* p2) const
	{
		return strcmp(p1, p2) > 0;
	}
};

struct CStringEquals
{
	bool operator()(char const* p1, char const* p2) const
	{
		return strcmp(p1, p2) == 0;
	}
};

/**
 * This is based on K&R version 2 (pg. 144); public domain.
 */
struct CStringKR2Hash
{
	std::size_t operator()(char const* s) const
	{
		size_t hashval;

		for (hashval = 0; *s != '\0'; s++)

			hashval = *s + 31*hashval;
		return hashval;

	}
};

/**
 * This is based on DJB2; public domain.
 */
struct CStringDJB2Hash
{
	std::size_t operator()(char const* str) const
	{
		size_t hash = 5381;
		int c;

		while ((c = *str++))
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

		return hash;
	}
};

/**
 * FNV-1a (Fowler/Noll/Vo) fast and well dispersed hash functions
 * Reference: http://www.isthe.com/chongo/tech/comp/fnv/index.html
 */
struct CStringFNV1aHash
{
	std::size_t operator()(char const* str) const
	{
		int len = strlen(str);

		uint32_t hval = 0x811C9DC5;
		unsigned char *bp = (unsigned char *)str;  /* start of buffer */
		unsigned char *be = bp + len;              /* beyond end of buffer */

		/*
		 * FNV-1a hash each octet in the buffer
		 */
		while (bp < be)
		{
			/* xor the bottom with the current octet */
			hval ^= (uint32_t)*bp++;
			hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
		}

		return hval;
	}
};


/**
 * This based on MurmurHash3; public domain.
 * Assumes unaligned 32 bit access is fine.
 *
 * This is also the implementation of std:hash<std::string> in GCC.
 *
 * Faster then KR2, DJB2, FNV1a by x2-x3 on long strings (128 chars),
 * 20-50% faster on short strings (32 chars).
 */
struct CStringMurmur3Hash
{
	std::size_t operator()(char const* s) const
	{
		uint32_t h = 0;
		if (s)
			MurmurHash3_x86_32(s, strlen(s), 17,&h);

		return h;
	}
};

//=== Template method for quick-select on a vector ===========================

/**
 * Partitions the array[left:right] around the pivot value, and returns the new pivot index.
 *
 * Type T must implement: operator<(), default constructor, copy constructor, assignment operator.
 * @param array
 * @param left start index, inclusive
 * @param right end index, inclusive
 * @param pivotIndex initial index of the pivot, within [left,right], inclusive
 * @return the final index of the pivot, within [left,right], inclusive
 */
template<typename T, typename A = std::vector<T>, typename U = std::less<T> >
std::size_t quickselect_partition(A& array,
		const std::size_t left,	const std::size_t right, const std::size_t pivotIndex)
{
	U less_then;
	//move pivot to end
	const T pivot = array[pivotIndex];
	array[pivotIndex] = array[right];
	array[right] = pivot;

	auto storeIndex = left;
	T tmp;
	for (auto i = left; i < right; ++i)
	{
		if (less_then(array[i],pivot))
		{
			//swap array[i] and array[storeIndex], increment storeIndex
			tmp = array[i];
			array[i] = array[storeIndex];
			array[storeIndex++] = tmp;
		}
	}
	//restore pivot to its final location
	tmp = array[right];
	array[right] = array[storeIndex];
	array[storeIndex] = tmp;

	return storeIndex;
}

/**
 * Returns the k-th smallest element in array, and leaves array partially sorted,
 * such that all elements [0,k-1) are smaller (or equal) to array[k-1].
 *
 * For k=1 returns the smallest element, for k=array-size returns the largest element.
 *
 * Complexity: O(array-size).
 *
 * @param array
 * @param k
 * @return the k-th smallest element
 */
template<typename T, typename A = std::vector<T>, typename U = std::less<T> >
T quickselect(A& array, const std::size_t k)
{
	if (k > array.size())
		throw std::out_of_range("arguments must hold: k <= array.size()");

	std::size_t left = 0;
	std::size_t right = array.size() - 1;

	while (true)
	{
		if (left == right)
		{
			return array[left];
		}

		std::size_t pivotIndex = left + rand() % (right - left + 1);
		pivotIndex = quickselect_partition<T,A,U>(array, left, right, pivotIndex);
		if ((k - 1) == pivotIndex)
		{
			return array[pivotIndex];
		}
		else if ((k - 1) < pivotIndex)
		{
			right = pivotIndex - 1;
		}
		else
		{
			left = pivotIndex + 1;
		}
	}
}

//=== ActivityNotify ==========================================================

/**
 * Notification interface for ActivityWorker class
 * @see ActivityWorker
 */
class ActivityNotify
{
public:
	ActivityNotify();
	virtual ~ActivityNotify();
	virtual void notifyWork() = 0;
};

//=== FailureNotify ==========================================================

/**
 * Failure notification interface for ActivityWorker class
 * @see ActivityWorker
 */
class FailureNotify
{
public:
    FailureNotify();
    virtual ~FailureNotify();
    virtual void notifyFailure(int err_code, const std::string& err_msg) = 0;
};


//=== Client Update record ====================================================

using StringSPtr = std::shared_ptr<std::string>;

/**
 * The value type in the ClientUpdateMap.
 *
 * Connection-event-time & Activity-event-time are written by the DB server $currentDate,
 * and are therefore omitted from this structure.
 *
 * enum-types & flags are represented by smaller integer sizes (int16_t & int8_t) to save space.
 */
struct ClientUpdateRecord
{
	//--- MongoDB fields ---
	StringSPtr		client_id; 		///< Client ID, globally unique, Org name embedded as prefix

	StringSPtr		org;			///< Org name

	StringSPtr      device_type;	///< the device type

	StringSPtr      device_id;      ///< The device id

	StringSPtr		gateway_id;		///< The gateway client ID for devices behind a gateway

	int16_t 		client_type;	///< Client type

	int16_t			protocol;		///< Protocol type: PXACT_CONN_PROTOCOL_TYPE_t

	int16_t 		conn_ev_type;   ///< Connection event type: PXACT_ACTIVITY_TYPE_t; can only be Connect  or None.

	int16_t 		disconn_ev_type;   ///< Connection event type: PXACT_ACTIVITY_TYPE_t; can only be Disconnect or None.

	int16_t 		last_act_type;	///< last activity type: PXACT_ACTIVITY_TYPE_t

	int32_t			expiry_sec; 	///< MQTT expiry interval seconds

	int8_t			clean_s; 		///< MQTT clean session/start, 0: false, 1: true, -1: N/A

	uint8_t			reason_code; 	///< MQTT disconnect reason code

	uint8_t			target_server; 	///< 0: N/A, 1: MS, 2: Kafka, 3: MS+Kafka

	//--- Auxiliary fields ---

	static const uint8_t DIRTY_FLAG_CLEAN	 			= 0;
	static const uint8_t DIRTY_FLAG_ACTITITY_EVENT 		= 1<<0;	///< Activity event (including connectivity)
	static const uint8_t DIRTY_FLAG_CONN_EVENT 			= 1<<1;	///< Connectivity event
	static const uint8_t DIRTY_FLAG_METADATA		 	= 1<<2;	///< Any field other then Activity or Connectivity
	static const uint8_t DIRTY_FLAG_NEW_RECORD	 		= 1<<3; ///< New record in the map

	uint8_t			dirty_flags;

	double			update_timestamp;	///< Time of last update, lazy time-stamp, seconds.
	double			queuing_timestamp;	///< Time inserted into queue, lazy time-stamp, seconds.
	int             num_updates;        ///< The number of updates from clean to db, i.e. conflation factor

	int 			conn_refcount;	///< local connections reference count, for keeping track of local connection stealing

	uint64_t 		connect_time;		//< connect time from proxy
	uint64_t 		disconnect_time;	//< disconnect time

	ClientUpdateRecord();

	/**
	 *
	 * @param pClientID
	 * @param pOrgName
	 * @param clientType
	 * @param pGateWayID optional, can be NULL
	 */
	ClientUpdateRecord(
			const char*                 pClientID,
			const char*                 pOrgName,
			const char*                 pDeviceType,
			const char*                 pDeviceID,
			PXACT_CLIENT_TYPE_t         clientType,
			const char*					pGateWayID);

	/**
	 *  Destroy this node and the list hanging off next.
	 */
	virtual ~ClientUpdateRecord();

	void reset();

	void update(ClientUpdateRecord& record);

	std::size_t getSizeBytes() const;

	std::string toString() const;
};

using ClientUpdateRecord_SPtr = std::shared_ptr<ClientUpdateRecord>;
using ClientUpdateVector = std::vector<ClientUpdateRecord_SPtr>;

/**
 * Compare ClientUpdateRecord_SPtr by their time stamp, for nth_element()
 */
bool compare_ts_less(const ClientUpdateRecord_SPtr& p1, const ClientUpdateRecord_SPtr& p2);

/**
 * Compare ClientUpdateRecord_SPtr by their time stamp, for nth_element()
 */
struct Compare_TS_Less
{
	bool operator()(const ClientUpdateRecord_SPtr& p1, const ClientUpdateRecord_SPtr& p2) const;
};

//=== Client State ============================================================

struct ClientStateRecord
{
	//--- MongoDB field names ---

	static constexpr const char* KEY_CLIENT_ID 		= PXACT_CLIENT_STATE_KEY_CLIENT_ID;
	static constexpr const char* KEY_ORG 			= PXACT_CLIENT_STATE_KEY_ORG;
	static constexpr const char* KEY_DEVICE_TYPE 	= PXACT_CLIENT_STATE_KEY_DEVICE_TYPE;
	static constexpr const char* KEY_DEVICE_ID 		= PXACT_CLIENT_STATE_KEY_DEVICE_ID;
	static constexpr const char* KEY_CLIENT_TYPE 	= PXACT_CLIENT_STATE_KEY_CLIENT_TYPE;
	static constexpr const char* KEY_GATEWAY_ID 	= PXACT_CLIENT_STATE_KEY_GATEWAY_ID;

	static constexpr const char* KEY_CONN_EV_TYPE 	= PXACT_CLIENT_STATE_KEY_CONN_EV_TYPE;
	static constexpr const char* KEY_CONN_EV_TIME 	= PXACT_CLIENT_STATE_KEY_CONN_EV_TIME;

    static constexpr const char* KEY_LAST_CONNECT_TIME      = PXACT_CLIENT_STATE_KEY_LAST_CONNECT_TIME;
    static constexpr const char* KEY_LAST_DISCONNECT_TIME   = PXACT_CLIENT_STATE_KEY_LAST_DISCONNECT_TIME;

	static constexpr const char* KEY_LAST_ACT_TYPE 	= PXACT_CLIENT_STATE_KEY_LAST_ACT_TYPE;
	static constexpr const char* KEY_LAST_ACT_TIME 	= PXACT_CLIENT_STATE_KEY_LAST_ACT_TIME;

	static constexpr const char* KEY_LAST_PUB_TIME 	= PXACT_CLIENT_STATE_KEY_LAST_PUB_TIME;

	static constexpr const char* KEY_PROTOCOL 		= PXACT_CLIENT_STATE_KEY_PROTOCOL;
	static constexpr const char* KEY_CLEAN_S 		= PXACT_CLIENT_STATE_KEY_CLEAN_S;
	static constexpr const char* KEY_REASON_CODE 	= PXACT_CLIENT_STATE_KEY_REASON_CODE;
	static constexpr const char* KEY_EXPIRY_SEC 	= PXACT_CLIENT_STATE_KEY_EXPIRY_SEC;

	static constexpr const char* KEY_TARGET_SERVER 	= PXACT_CLIENT_STATE_KEY_TARGET_SERVER;
	static constexpr const char* KEY_PROXY_UID 		= PXACT_CLIENT_STATE_KEY_PROXY_UID;

	static constexpr const char* KEY_VERSION 		= PXACT_CLIENT_STATE_KEY_VERSION;

	//--- MongoDB fields ---

	std::string 				client_id; 		///< Client ID, globally unique, Org name embedded as prefix
	std::string 				org;			///< Org name
	std::string 				device_type;	///< Device type
	std::string 				device_id;		///< Device ID
	PXACT_CLIENT_TYPE_t 		client_type;	///< Client type
	int64_t 					conn_ev_time;	///< connection event time (millisec since Unix epoch)
	PXACT_ACTIVITY_TYPE_t 		conn_ev_type;   ///< Connection event type; can only be Connect, Disconnect, or None.
	int64_t                     last_connect_time;      ///< last connect time
	int64_t                     last_disconnect_time;   ///< last disconnect time
	int64_t 					last_act_time;  ///< last activity time
	int64_t 					last_pub_time;  ///< last pub time
	PXACT_ACTIVITY_TYPE_t 		last_act_type;	///< last activity type
	PXACT_CONN_PROTOCOL_TYPE_t 	protocol;       ///< MQTT protocol
	int32_t						clean_s; 		///< MQTT clean session/start, 0: false, 1: true, -1: N/A
	int32_t						expiry_sec;		///< MQTT expiry interval seconds
	int32_t						reason_code; 	///< MQTT disconnect reason code
	std::string					proxy_uid;		///< the UID of the proxy handling the event
	std::string					gateway_id;		///< the gateway client ID for devices behind a gateway
	PXACT_TRG_SERVER_TYPE_t		target_server; 	///< 0: N/A, 1: MS, 2: Kafka, 3: MS+Kafka
	int64_t						version;		///< incremented on every write, for light-weight transactions

	//--- Auxiliary fields ---
	int 					return_code;
	ClientStateRecord*		next;

	ClientStateRecord();

	/**
	 *  Destroy this node and the list hanging off next.
	 */
	virtual ~ClientStateRecord();

	/**
	 * Destroy the list hanging off next, set next to NULL.
	 */
	void destroyNext();

	std::string toString() const;

	static bool isConnectType(const PXACT_ACTIVITY_TYPE_t type);
	static bool isDisconnectType(const PXACT_ACTIVITY_TYPE_t type);
	static bool isPubType(const PXACT_ACTIVITY_TYPE_t type);
	static bool isMqttActivityType(const PXACT_ACTIVITY_TYPE_t type);
	static bool isHttpActivityType(const PXACT_ACTIVITY_TYPE_t type);
};

using ClientStateRecord_SPtr = std::shared_ptr<ClientStateRecord>;
using ClientStateRecord_UPtr = std::unique_ptr<ClientStateRecord>;
//=== Proxy State =============================================================

struct ProxyStateRecord
{
	//--- MongoDB field names ---

	static constexpr const char* KEY_PROXY_UID = PXACT_PROXY_STATE_KEY_PROXY_UID;
	static constexpr const char* KEY_STATUS = PXACT_PROXY_STATE_KEY_STATUS;
	static constexpr const char* KEY_LAST_HB = PXACT_PROXY_STATE_KEY_LAST_HB;
	static constexpr const char* KEY_HBTO_MS = PXACT_PROXY_STATE_KEY_HBTO_MS;
	static constexpr const char* KEY_VERSION = PXACT_PROXY_STATE_KEY_VERSION;

	enum PxStatus
	{
		PX_STATUS_BOGUS = -1,
		PX_STATUS_ACTIVE = 0,
		PX_STATUS_SUSPECT = 1,
		PX_STATUS_LEAVE = 2,
		PX_STATUS_REMOVED = 3
	};

	std::string proxy_uid;        	///< Proxy UID
	int hbto_ms;          			///< Heartbeat timeout millis
	int64_t last_hb;          		///< Last heartbeat, in millis since epoch
	PxStatus status;           		///< Proxy status
	int64_t version; 				///< version for optimistic transactions; valid >=0, init: (-1).

	ProxyStateRecord* next;		  	///< When returning a linked list of records.

	ProxyStateRecord();
	ProxyStateRecord(const std::string& uid, int hbto_ms, int64_t last_hb, PxStatus status, int64_t version);

	/**
	 *  Destroy this node and the list hanging off next.
	 */
	virtual ~ProxyStateRecord();

	/**
	 * Destroy the list hanging off next, set next to NULL.
	 */
	void destroyNext();

	std::string toString() const;
};


using ProxyStateRecord_SPtr = std::shared_ptr<ProxyStateRecord>;


//=== TokenBucket =============================================================

/**
 * A token bucket for rate limiting I/O rate to the DB.
 * Uses ism_common_readTSC() to measure time between refills.
 * An external thread must call refill() periodically.
 *
 * A rate limit of zero (0) means unlimited-rate.
 */
class TokenBucket
{
public:
	static const int REFILL_INTERVAL_MILLIS = 10;

	/**
	 *
	 * @param rate_iops Valid rate >=1, 0=unlimited rate
	 * @param bucket_size Valid size >= 1.
	 */
	TokenBucket(uint32_t rate_iops, uint32_t bucket_size);
	virtual ~TokenBucket();

	/**
	 * Get tokens, wait if not enough are available.
	 *
	 * @param num The number of requested tokens; Must be <= BucketSize.
	 * @return ISMRC_OK, ISMRC_Closed, ISMRC_Error.
	 */
	int getTokens(uint32_t num);

	/**
	 * Try to get tokens, return immediately if not enough is available.
	 * If there
	 * @param num The number of requested tokens; Must be <= BucketSize.
	 * @return ISMRC_OK, ISMRC_WouldBlock: in case there are not enough tokens available, ISMRC_Closed, ISMRC_Error.
	 */
	int tryGetTokens(uint32_t num);

	/**
	 * Set new limit - sustained rate and bucket-size (burst size).
	 *
	 * @param rate_iops Valid rate >=1, 0=unlimited rate
	 * @param bucket_size Valid size >= minBucketSize; If rate_iops=0, ignored (internally set to 0).
	 * @return ISMRC_OK, ISMRC_Closed, ISMRC_Error.
	 */
	int setLimit(uint32_t rate_iops);

	uint32_t getBucketSize() const;
	uint32_t getLimit() const;

	/**
	 * Sould be called periodically by an external thread.
	 * @return ISMRC_OK, ISMRC_Closed, ISMRC_Error.
	 */
	int refill();

	/**
	 * Close and release any thread waiting for tokens.
	 * @return ISMRC_OK, ISMRC_Error.
	 */
	int close();

	int restart();

private:
	mutable std::mutex mutex_;
	std::condition_variable condVar_;
	bool closed_;
	uint32_t bucket_Size_;
	double bucket_;
	uint32_t rate_IOpS_;
	double lastRefillSec_;
};

//=== toString of API structures ==============================================

std::string toString(const ismPXACT_Stats_t* pStats);

/**
 * String representation of the configuration structure.
 * @param config
 * @return
 */
std::string toString(const ismPXActivity_ConfigActivityDB_t* config);

/**
 * String representation of the client structure.
 * @param pClient
 * @return
 */
std::string toString(const ismPXACT_Client_t* pClient);

std::string toString(const ismPXACT_ConnectivityInfo_t* pConnInfo);

} /* namespace ism_proxy */

#endif /* ACTIVITYUTILS_H_ */
