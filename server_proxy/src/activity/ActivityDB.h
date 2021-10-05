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
 * ActivityDB.h
 *
 *  Created on: Oct 29, 2017
 */

#ifndef ACTIVITYDB_H_
#define ACTIVITYDB_H_

#include <memory>
#include <mutex>

#include <mongoc.h>

#include "pxactivity.h"
#include "ActivityUtils.h"
#include "ActivityDBClient.h"

namespace ism_proxy
{

//=== ActivityBD ==============================================================

/**
 * Synchronous API to MongoDB, reading & writing the client_state & proxy_state collections.
 * Using the MongoDB's C-driver mongoc.
 */
class ActivityDB
{
public:
	ActivityDB();
	virtual ~ActivityDB();

    int configure(const ismPXActivity_ConfigActivityDB_t* config);

    /**
     * Connect to MongoDB and perform sanity checks.
     *
     * Connect to MongoDB, create the client pool, ping the server,
     * get server descriptions.
     *
     * Validate DB & Collections exist.
     *
     * @return
     */
    int connect();

    int close();

    /**
     * Choose UID, insert document into proxy_state collection.
     *
     * @return
     */
    int register_uid();

    /**
     * Get the generated ProxyUID.
     * @return
     */
    std::string getProxyUID() const;

    int getHeartbeatIntervalMillis() const;

    int getHeartbeatTimeoutMillis() const;

    ActivityDBClient_SPtr getClient();

    int closeClient(ActivityDBClient_SPtr client);

private:

    /* the first object created calls mongoc_init() and creates the CleanUpLast unique_ptr */
    static std::once_flag once_flag_mongoc_init_;

    /* when the process exits it calls mongoc_cleanup() */
	struct CleanUpLast
	{
		CleanUpLast() {}
		virtual ~CleanUpLast() { mongoc_cleanup(); }
	};
	static std::unique_ptr<CleanUpLast> cleanup_last_UPtr_;

    mutable std::recursive_mutex mutex_;

    ActivityDBConfig_SPtr config_;

    bool configured_;
    bool connected_;
    bool closed_;

    std::string mongoURI_with_credentials_;
    mongoc_uri_t* mongoc_uri_;
    bool is_sslAllowInvalidCertificates_;
    bool is_sslAllowInvalidHostnames_;
    mongoc_ssl_opt_t ssl_opts_ = {0};
    mongoc_client_pool_t* mongoc_client_pool_;

    std::string proxy_uid_;

    static void init_once();

};


} /* namespace ism_proxy */

#endif /* ACTIVITYDB_H_ */
