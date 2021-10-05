/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
 * ActivityDBClient.h
 *
 *  Created on: Oct 29, 2017
 */

#ifndef ACTIVITYDBCLIENT_H_
#define ACTIVITYDBCLIENT_H_

#include <string>

#include <mongoc.h>

#include <time.h>

#include "ActivityUtils.h"

namespace ism_proxy
{

//=== ActivityDBClient ========================================================

/**
 * A client for reading and writing the activity DB.
 *
 * This is a single threaded client, it is not thread safe.
 * Typically the thread that created it will be the only one to use it,
 * and will be the one to close it.
 */
class ActivityDBClient
{
	friend class ActivityDB;
public:
	ActivityDBClient(const std::string& name, ActivityDBConfig_SPtr config, mongoc_client_pool_t* pool);
	virtual ~ActivityDBClient();

	//=== Proxy state ===============================================
	/**
	 * Insert ProxyStateRecord to proxy_state collection if it does not exist.
	 *
	 * The last_hb field is assigned $currentDate;
	 * The status to ACTIVE;
	 * The version to 1;
	 *
	 * @param proxy_state
	 * @return ISMRC_OK, ISMRC_ExistingKey, ISMRC_Error.
	 */
	int insertNewUID(const std::string& proxy_uid, int hbto_ms);

	/**
	 * Remove a ProxyStateRecord.
	 *
	 * @param proxy_uid
	 * @return ISMRC_OK, ISMRC_Error.
	 */
	int removeProxyUID(const std::string& proxy_uid);

	/**
	 * Remove all ProxyStateRecord.
	 *
	 * For testing.
	 *
	 * @return
	 */
	int removeAllProxyUID();

	/**
	 * Read a ProxyStateRecord from proxy_state collection.
	 * @param proxy_state
	 * @return
	 */
	int read(ProxyStateRecord* proxy_state);

	/**
	 * Update the last heartbeat time to $currentDate, and retrieve the new record.
	 *
	 * @param proxy_state with proxy_uid set
	 * @return
	 */
	int updateHeartbeat(ProxyStateRecord* proxy_state);

	/**
	 *
	 * @param proxy_uid
	 * @param status
	 * @return
	 */
	int updateStatus_CondVersion(const std::string& proxy_uid, ProxyStateRecord::PxStatus status, int64_t version);

	/**
	 * Get all proxies with a given state.
	 *
	 * Returns a linked list of all proxies with a given state.
	 *
	 * @param proxy_state head of the list
	 * @param list_size optional, can be NULL
	 * @return
	 */
	int getAllWithStatus(ProxyStateRecord::PxStatus status, ProxyStateRecord* proxy_state, int* list_size);

	int getAllWithStatusIn(const std::vector<ProxyStateRecord::PxStatus>& status_vec, ProxyStateRecord* proxy_state, int* list_size);

	//=== Client state ===============================================


	/**
	 * Read a single ClientStateRecord from client_state collection.
	 *
	 * @param client_state client_state.client_id must be set
	 *
	 * @return ISMRC_OK, ISMRC_NotFound, ISMRC_Error
	 */
	int read(ClientStateRecord* client_state);

	/**
	 * Insert if does not exist.
	 *
	 * @param client_update
	 * @param proxy_uid
	 * @return ISMRC_OK, ISMRC_ExistingKey, ISMRC_Error
	 */
	int insert(ClientUpdateRecord* client_update, const std::string& proxy_uid);

	/**
	 * Remove a ClientStateRecord.
	 *
	 * @param org
	 * @param client_id
	 * @return ISMRC_OK, ISMRC_Error
	 */
	int removeClientID(const std::string& org, const std::string& client_id);

	/**
	 * Remove all client IDs.
	 *
	 * For testing.
	 *
	 * @return
	 */
	int removeAllClientID();

	/**
	 * Update the client in the client_state collection.
	 *
	 * The dirty flags determine which fields are updated.
	 * The proxy_uid is updated only when DIRTY_FLAG_CONN_EVENT is set.
	 *
	 * @param client_update
	 * @param proxy_uid
	 * @return ISMRC_OK, ISMRC_NotFound, ISMRC_Error
	 */
	int updateOne(ClientUpdateRecord* client_update, const std::string& proxy_uid);

	/**
	 * Update the client in the client_state collection, conditional on the version being equal.
	 *
	 * The dirty flags determine which fields are updated.
	 * The proxy_uid is updated only when DIRTY_FLAG_CONN_EVENT is set.

	 * @param client_update
	 * @param proxy_uid
	 * @param version
	 * @return
	 */
	int updateOne_CondVersion(ClientUpdateRecord* client_update, const std::string& proxy_uid, int64_t version);

	/**
	 * Bulk Update the client in the client_state collection.
	 *
	 * Assumes activity & meta only - not connectivity.
	 *
	 * @param client_update_vec
	 * @return
	 */
	int updateBulk(ClientUpdateVector& client_update_vec, std::size_t numUpdates, const std::string& proxy_uid);


	/**
	 *
	 * @param proxy_uid
	 * @param selectConnectivity 0: all, 1: only connected, 2: only disconnected
	 * @param limit
	 * @param client_state
	 * @param list_size
	 * @return
	 */
	int findAllAssociatedToProxy(const std::string proxy_uid, int selectConnectivity, int limit, ClientStateRecord* client_state, int* list_size);

	/**
	 *
	 * @param proxy_uid
	 * @param selectConnectivity 0: all, 1: only connected, 2: only disconnected
	 * @param limit
	 * @param num_updated
	 * @return
	 */
    int disassociatedFromProxy(const std::string proxy_uid, int selectConnectivity, int limit, int* num_updated);

	int close();

private:
	const std::string name_;
	ActivityDBConfig_SPtr config_;
	mongoc_client_pool_t* pool_;
	bool closed_;
	mongoc_client_t* client_;
	mongoc_database_t* database_;
	mongoc_collection_t* client_state_coll_;
	mongoc_collection_t* proxy_state_coll_;

	int readInsert_UnmatchedBulkUpdate(ClientUpdateVector& client_update_vec, std::size_t numUpdates, const std::string& proxy_uid);
	int parseFindResults(mongoc_cursor_t* cursor, ProxyStateRecord* proxy_state, int* list_size);


};

using ActivityDBClient_SPtr = std::shared_ptr<ActivityDBClient>;

} /* namespace ism_proxy */

#endif /* ACTIVITYDBCLIENT_H_ */
