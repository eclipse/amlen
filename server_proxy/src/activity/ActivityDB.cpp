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
 * ActivityDB.cpp
 *
 *  Created on: Oct 29, 2017
 */

#include "ActivityDB.h"

#define SoN(x) ((x)?(x):"nil")

namespace ism_proxy
{


//=== ActivityBD ==============================================================

std::once_flag ActivityDB::once_flag_mongoc_init_;
std::unique_ptr<ActivityDB::CleanUpLast> ActivityDB::cleanup_last_UPtr_;

ActivityDB::ActivityDB() :
			configured_(false),
		    connected_(false),
			closed_(false),
			mongoc_uri_(NULL),
			is_sslAllowInvalidCertificates_(false),
			is_sslAllowInvalidHostnames_(false),
			mongoc_client_pool_(NULL)
{
    TRACE(9,"Entry: %s\n",__FUNCTION__);
    std::call_once(once_flag_mongoc_init_,init_once);
}

ActivityDB::~ActivityDB()
{
	TRACE(5, "%s this=%p\n", __FUNCTION__, this);
    if (mongoc_client_pool_)
    {
        mongoc_client_pool_destroy(mongoc_client_pool_);
        mongoc_client_pool_ = NULL;
    }
    if (mongoc_uri_)
    {
        mongoc_uri_destroy(mongoc_uri_);
        mongoc_uri_ = NULL;
    }
}

void ActivityDB::init_once()
{
	mongoc_init();
	cleanup_last_UPtr_.reset(new CleanUpLast);
}

int ActivityDB::configure(const ismPXActivity_ConfigActivityDB_t* config)
{
    using namespace std;

    TRACE(9,"Entry: %s config=%s\n",__FUNCTION__, toString(config).c_str());

    LockGuardRecur lock(mutex_);

    if (!config)
    {
        TRACE(1,"Error: %s Null config, rc=%d\n",__FUNCTION__, ISMRC_NullArgument);
        return ISMRC_NullArgument;
    }

    if (connected_)
    {
        TRACE(1,"Error: %s Already connected, rc=%d\n",__FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }

    config_.reset(new ActivityDBConfig(config));

    configured_ = false;

    trim_inplace(config_->mongoURI);
    trim_inplace(config_->mongoEndpoints);
    trim_inplace(config_->mongoDBName);
    trim_inplace(config_->mongoClientStateColl);
    trim_inplace(config_->mongoProxyStateColl);
    trim_inplace(config_->mongoUser);
    trim_inplace(config_->mongoPassword);
    trim_inplace(config_->mongoOptions);

    if (config_->dbType != std::string("MongoDB"))
    {
        TRACE(1,"Error: %s Unknown DB type: %s, rc=%d\n",__FUNCTION__, config_->dbType.c_str(), ISMRC_Error);
        return ISMRC_Error;
    }

    if (config_->mongoURI.empty() && config_->mongoEndpoints.empty())
    {
        TRACE(1,"Error: %s Missing Mongo URI, or Endpoints, rc=%d\n",__FUNCTION__, ISMRC_NullPointer);
        return ISMRC_NullPointer;
    }

    if (!config_->mongoURI.empty() && !config_->mongoEndpoints.empty())
    {
        TRACE(1,"Error: %s Use either Mongo URI, or Endpoints, not both. rc=%d\n",__FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }

    if (config_->mongoDBName.empty())
    {
    	config_->mongoDBName = PXACT_MONGO_DB_NAME;
    }

    if (config_->mongoClientStateColl.empty())
    {
    	config_->mongoClientStateColl = PXACT_MONGO_CLIENT_STATE_COLL_NAME;
    }

    if (config_->mongoProxyStateColl.empty())
    {
    	config_->mongoProxyStateColl = PXACT_MONGO_PROXY_STATE_COLL_NAME;
    }

    if (config_->hbTimeoutMS <= 0 )
    {
    	config_->hbTimeoutMS = PXACT_PROXY_HEATBEAT_TIMEOUT_MS;
    }

    if (config_->hbIntervalMS <= 0)
    {
    	config_->hbIntervalMS = config_->hbTimeoutMS/10;
    }

    if (!((config_->hbIntervalMS) > 0 && (config_->hbTimeoutMS /5 > config_->hbIntervalMS)))
    {
    	TRACE(1,"Error: %s Illegal heartbeat config: timeout=%d, interval=%d, config=%s, rc=%d\n",
    			__FUNCTION__, config_->hbTimeoutMS , config_->hbIntervalMS, config_->toString().c_str(), ISMRC_Error);
    	return ISMRC_Error;
    }


    bool from_URI = false;
    if (!config_->mongoURI.empty())
    {
        from_URI = true;
        //override user, password, if they exist
        mongoURI_with_credentials_ = ActivityDBConfig::build_uri(
                config_->mongoURI, config_->mongoUser, config_->mongoPassword);
        if (mongoURI_with_credentials_.empty())
        {
            TRACE(1, "Error: %s Could not build URI with credentials: Illegal MongoDB URI: %s, rc=%d\n", __FUNCTION__,
                    ActivityDBConfig::mask_pwd_uri(config_->mongoURI).c_str(), ISMRC_Error);
            return ISMRC_Error;
        }
    }
    else if (!config_->mongoEndpoints.empty())
    {
        mongoURI_with_credentials_ = ActivityDBConfig::assemble_uri(
               config_->mongoUser, config_->mongoPassword, config_->mongoEndpoints, config_->mongoAuthDB,
               config_->useTLS, config_->mongoCAFile, config_->mongoOptions);
        if (mongoURI_with_credentials_.empty())
        {
            TRACE(1, "Error: %s Could not assemble URI with credentials: User=%s, Pwd=********, EP=%s, DB=%s, Options=%s, rc=%d\n",
                    __FUNCTION__, config_->mongoUser.c_str(), config_->mongoEndpoints.c_str(), config_->mongoAuthDB.c_str(), config_->mongoOptions.c_str(), ISMRC_Error);
            return ISMRC_Error;
        }
    }

    mongoc_uri_ = mongoc_uri_new(mongoURI_with_credentials_.c_str());
    if (!mongoc_uri_)
    {
    	TRACE(1,"Error: %s Illegal MongoDB URI: %s, rc=%d\n",
    	    			__FUNCTION__, ActivityDBConfig::mask_pwd_uri(mongoURI_with_credentials_).c_str(), ISMRC_Error);
    	return ISMRC_Error;
    }
    else
    {
        TRACE(4,"%s MongoDB URI: '%s'\n",
                                __FUNCTION__, ActivityDBConfig::mask_pwd_uri(mongoURI_with_credentials_).c_str());
    }

    //===
#if MONGOC_CHECK_VERSION(1,8,0)
    TRACE(4,"%s Using Mongo-C-Driver: Compiled version %s, Runtime version %s \n",
                __FUNCTION__, (MONGOC_VERSION_S), mongoc_get_version());
#else
    TRACE(4,"%s Using Mongo-C-Driver: Compiled version %s\n",
                    __FUNCTION__, (MONGOC_VERSION_S));
#endif

    //=== mongoc ver. 1.3.6 does not take the ssl options from the URI (only ssl=true); must set explicitly.
    //get ssl options
#if MONGOC_CHECK_VERSION(1,15,0)
    bool is_ssl = mongoc_uri_get_tls(mongoc_uri_);
#else
    bool is_ssl = mongoc_uri_get_ssl(mongoc_uri_);
#endif
    if (from_URI)
    {
        config_->useTLS = (is_ssl ? 1 : 0);
    }
    else if (is_ssl != (config_->useTLS > 0))
    {
        TRACE(1, "Error: %s Inconsistent SSL options: %s; vs. config: %s; rc=%d \n",
                __FUNCTION__, mongoURI_with_credentials_.c_str(), config_->toString().c_str(), ISMRC_Error);
        return ISMRC_Error;
    }

    const bson_t * bson_opt = mongoc_uri_get_options (mongoc_uri_);
    if (bson_opt)
    {
        char* str_opt = bson_as_json(bson_opt, NULL);
        TRACE(4, "%s URI options are (bson as json): %s \n", __FUNCTION__, str_opt);

        bson_iter_t iter;
        if (bson_iter_init(&iter, bson_opt))
        {
            while (bson_iter_next (&iter))
            {
                std::string key =  bson_iter_key (&iter);
                TRACE(9, "%s Found element key: %s \n", __FUNCTION__, key.c_str());
                std::transform(key.begin(), key.end(), key.begin(),
                        [](unsigned char c) { return std::tolower(c); } );
                TRACE(9, "%s Found element key (lower-case): %s \n", __FUNCTION__, key.c_str());
#if MONGOC_CHECK_VERSION(1,15,0)
                //Since 1.15.0 version ssl is deprecated for tls
                if (key == string("tlsallowinvalidcertificates"))
#else
                	 if (key == string("sslallowinvalidcertificates"))
#endif
                {
                    ssl_opts_.weak_cert_validation = false;
                    if (BSON_ITER_HOLDS_BOOL(&iter))
                    {
                        ssl_opts_.weak_cert_validation = bson_iter_bool(&iter);
                    }
                    else if (BSON_ITER_HOLDS_UTF8(&iter))
                    {
                        string val = bson_iter_utf8(&iter, NULL);
                        if (val == string("true"))
                        {
                            ssl_opts_.weak_cert_validation = true;
                        }
                    }

                    is_sslAllowInvalidCertificates_ = ssl_opts_.weak_cert_validation;
                    TRACE(4, "Config: %s, ssl_opts_.weak_cert_validation set to %s\n", __FUNCTION__, (ssl_opts_.weak_cert_validation ? "True" : "False"));
                }
#if MONGOC_CHECK_VERSION(1,15,0)
                if (key == string("tlscertificateauthorityfile")  || key == string("tlscafile"))
#else
                	if (key == string("sslcertificateauthorityfile"))
#endif
                {
                    if (from_URI)
                    {
                        if (BSON_ITER_HOLDS_UTF8(&iter))
                        {
                            config_->mongoCAFile = bson_iter_utf8(&iter,NULL);
                        }
                    }

                    ssl_opts_.ca_file = config_->mongoCAFile.c_str();
                    TRACE(4, "Config: %s, ssl_opts_.ca_file set to '%s'\n", __FUNCTION__, SoN(ssl_opts_.ca_file));
                }
#if MONGOC_CHECK_VERSION(1,15,0)
                if (key == string("tlsallowinvalidhostnames"))
#else
                	if (key == string("sslallowinvalidhostnames"))
#endif
                {

#if MONGOC_CHECK_VERSION(1,4,0)
                    ssl_opts_.allow_invalid_hostname = false;
                    if (BSON_ITER_HOLDS_BOOL(&iter))
                    {
                        ssl_opts_.allow_invalid_hostname = bson_iter_bool(&iter);
                    }
                    else if (BSON_ITER_HOLDS_UTF8(&iter))
                    {
                        string val = bson_iter_utf8(&iter, NULL);
                        if (val == string("true"))
                        {
                            ssl_opts_.allow_invalid_hostname = true;
                        }
                    }

                    is_sslAllowInvalidHostnames_ = ssl_opts_.allow_invalid_hostname;
                    TRACE(4, "Config: %s, ssl_opts_.allow_invalid_hostname set to %s\n",
                            __FUNCTION__, (ssl_opts_.allow_invalid_hostname ? "True" : "False"));
#else
                    TRACE(4,"%s Using Mongo-C-Driver: Compiled version %s; does not support sslAllowInvalidHostnames\n",
                            __FUNCTION__, (MONGOC_VERSION_S));
#endif

                }
            } //while

            if (ssl_opts_.weak_cert_validation)
            {
                TRACE(4, "Config: %s, ssl_opts_.ca_file reset to NULL because ssl_opts_.weak_cert_validation=true\n", __FUNCTION__);
                ssl_opts_.ca_file = NULL;
            }
        }

        bson_free (str_opt);
    }
    else
    {
        TRACE(4, "%s NO URI options.\n", __FUNCTION__);
    }


    configured_ = true;

    TRACE(4,"%s Configuration after setting defaults: %s\n",
    		__FUNCTION__, config_->toString().c_str());

    TRACE(9,"Exit: %s\n",__FUNCTION__);
    return ISMRC_OK;
}

int ActivityDB::connect()
{
	using namespace std;

    TRACE(9,"Entry: %s",__FUNCTION__);

    int rc = ISMRC_OK;

    LockGuardRecur lock(mutex_);

    if (configured_ && !connected_)
    {
		mongoc_client_pool_ = mongoc_client_pool_new(mongoc_uri_);
		if (!mongoc_client_pool_)
		{
			TRACE(1, "Error: %s Cannot create MongoDB client pool, URI=%s, rc=%d\n",
					__FUNCTION__, config_->mongoURI.c_str(), ISMRC_Error);
			return ISMRC_Error;
		}

#if MONGOC_CHECK_VERSION(1,5,0)
		if (!mongoc_client_pool_set_appname(mongoc_client_pool_, "ActivityDB"))
		{
			TRACE(1, "Error: %s Cannot set MongoDB appname: %s, rc=%d\n",
					__FUNCTION__, "ActivityDB", ISMRC_Error);
			return ISMRC_Error;
		}

		if (!mongoc_client_pool_set_error_api(mongoc_client_pool_,MONGOC_ERROR_API_VERSION_2))
		{
			TRACE(1, "Error: %s Cannot set MongoDB error api: %d, rc=%d\n",
					__FUNCTION__, MONGOC_ERROR_API_VERSION_2, ISMRC_Error);
			return ISMRC_Error;
		}
#endif

		TRACE(5,"%s Created connection pool to MongoDB cluster, rc=%d\n", __FUNCTION__, ISMRC_OK);

		if (config_->useTLS)
		{
		    mongoc_client_pool_set_ssl_opts(mongoc_client_pool_,&ssl_opts_);
		    TRACE(5,"%s Setting SSL options on connection pool\n", __FUNCTION__);
		    if (is_sslAllowInvalidCertificates_)
		    {
		        TRACE(2,"Warning: %s Using SSL with sslAllowInvalidCertificates=true\n", __FUNCTION__);
		    }
		}

		mongoc_client_t* pClient = mongoc_client_pool_pop(mongoc_client_pool_);
		if (pClient)
		{
			bson_t *ping = BCON_NEW ("ping", BCON_INT32 (1));
			bson_error_t error;
			bool r;

			/* ensure client has connected */
			r = mongoc_client_command_simple (pClient, config_->mongoDBName.c_str(), ping, NULL, NULL, &error);
			if (!r)
			{
				rc = ISMRC_UnableToConnect;
				TRACE(1, "Error: %s Could not connect to MongoDB: errro.domain=%d, error.code=%d, error.message=%s, rc=%d\n",
						__FUNCTION__, error.domain, error.code, error.message, rc);
				bson_destroy (ping);
				mongoc_client_pool_push(mongoc_client_pool_, pClient);
				goto connect_exit;
			}
			bson_destroy (ping);

			TRACE(5,"%s Ping to MongoDB OK, rc=%d\n", __FUNCTION__, ISMRC_OK);

			/* validate db & collections exist */

			bool found_coll_client = false;
			bool found_coll_proxy = false;
			mongoc_database_t * db = mongoc_client_get_database(pClient, config_->mongoDBName.c_str());
			if (db)
			{
			    /* verify collection access */
			    ActivityDBClient_SPtr client_sptr = getClient();
			    if (client_sptr)
			    {
			        {
			            ClientStateRecord client_state;
			            client_state.client_id = "this-is-not-a-client-and-will-never-be-one";
			            client_state.org = "this-is-not-an-org-and-will-never-be-one";
			            int rc1 = client_sptr->read(&client_state);
			            if (rc1 != ISMRC_NotFound)
			            {
			                TRACE(1, "Error: %s Could not get read access to MongoDB collection=%s from db=%s; %s\n",
			                        __FUNCTION__, config_->mongoClientStateColl.c_str(), config_->mongoDBName.c_str(),
			                        "check configuration and database schema.");
			            }
			            else
			            {
			                found_coll_client = true;
			            }
			        }

			        {
			            ProxyStateRecord proxy_state;
			            proxy_state.proxy_uid = "this-is-not-a-proxy-and-will-never-be-one";
			            int rc1 = client_sptr->read(&proxy_state);
			            if (rc1 != ISMRC_NotFound)
			            {
			                TRACE(1, "Error: %s Could not get read access to MongoDB collection=%s from db=%s; %s\n",
			                        __FUNCTION__, config_->mongoProxyStateColl.c_str(), config_->mongoDBName.c_str(),
			                        "check configuration and database schema.");
			            }
			            else
			            {
			                found_coll_proxy = true;
			            }
			        }
			        closeClient(client_sptr);
			    }

			    mongoc_database_destroy(db);
			}
			else
			{
				rc = ISMRC_Error;
				TRACE(1, "Error: %s Could not get database from MongoDB: db=%s, rc=%d\n",
						__FUNCTION__, config_->mongoDBName.c_str(), rc);
				mongoc_client_pool_push(mongoc_client_pool_, pClient);
				goto connect_exit;
			}

			if (!found_coll_client)
			{
				rc = ISMRC_Error;
				TRACE(1, "Error: %s Could not find MongoDB collection=%s in db=%s, check configuration and database schema, rc=%d\n",
						__FUNCTION__, config_->mongoClientStateColl.c_str(), config_->mongoDBName.c_str(), rc);
			}

			if (!found_coll_proxy)
			{
				rc = ISMRC_Error;
				TRACE(1, "Error: %s Could not find MongoDB collection=%s in db=%s, check configuration and database schema, rc=%d\n",
						__FUNCTION__, config_->mongoProxyStateColl.c_str(), config_->mongoDBName.c_str(), rc);
			}

			mongoc_client_pool_push(mongoc_client_pool_, pClient);
		}
		else
		{
			rc = ISMRC_UnableToConnect;
			TRACE(1, "Error: %s Could not create MongoDB client, rc=%d\n", __FUNCTION__, rc);
		}

		if (rc == ISMRC_OK)
		{
			connected_ = true;
			closed_ = false;
			TRACE(3,"%s ActivityDB successfully connected to MongoDB and validated DB & collections; rc=%d\n", __FUNCTION__, ISMRC_OK);
		}
    }
    else
    {
    	rc = ISMRC_Error;
        TRACE(1,"Error: %s Illegal state: configured=%d connected=%d, rc=%d\n",__FUNCTION__, (configured_?1:0), (connected_?1:0), rc);
    }

    connect_exit:
	if (rc != ISMRC_OK)
	{
	    TRACE(1,"Error: %s failure, rc=%d\n",__FUNCTION__, rc);
	}

    return rc;
}


int ActivityDB::close()
{
    LockGuardRecur lock(mutex_);
    TRACE(5, "%s Entry\n", __FUNCTION__);
    closed_ = true;
    connected_ = false;
    if (mongoc_client_pool_)
    {
        mongoc_client_pool_destroy(mongoc_client_pool_);
        mongoc_client_pool_ = NULL;
    }
    TRACE(5, "%s Exit\n", __FUNCTION__);
	return ISMRC_OK;
}

int ActivityDB::register_uid()
{

	ActivityDBClient_SPtr client = getClient();
	if (!client)
	{
		TRACE(1,"Error: %s failed to create client, rc=%d\n",__FUNCTION__, ISMRC_Error);
		return ISMRC_Error;
	}

	int rc = ISMRC_OK;
	int k = 3;

	while (k-- > 0)
	{
		{
			LockGuardRecur lock(mutex_);
			proxy_uid_ = generateProxyUID(12);
		}
		rc = client->insertNewUID(proxy_uid_, config_->hbTimeoutMS);
		if (rc == ISMRC_OK)
			break;
	}

	if (rc != ISMRC_OK)
	{
		TRACE(1,"Error: %s failed to insertNewUID, rc=%d\n",__FUNCTION__, rc);
	}
	else
	{
		TRACE(5,"%s registered new uid=%s, rc=%d\n",__FUNCTION__, proxy_uid_.c_str(), rc);
	}

	closeClient(client);

	return rc;
}

std::string ActivityDB::getProxyUID() const
{
	LockGuardRecur lock(mutex_);
	return proxy_uid_;
}

int ActivityDB::getHeartbeatIntervalMillis() const
{
	LockGuardRecur lock(mutex_);
	return config_->hbIntervalMS;
}

int ActivityDB::getHeartbeatTimeoutMillis() const
{
	LockGuardRecur lock(mutex_);
	return config_->hbTimeoutMS;
}

ActivityDBClient_SPtr ActivityDB::getClient()
{
	ActivityDBClient_SPtr client;

	LockGuardRecur lock(mutex_);

	try
	{
		client.reset(new ActivityDBClient("", config_, mongoc_client_pool_));
	}
	catch (std::runtime_error& e)
	{
		TRACE(1,"Error: %s failed, what: %s, returning NULL", __FUNCTION__, e.what());
	}

	return client;
}

int ActivityDB::closeClient(ActivityDBClient_SPtr client)
{
	LockGuardRecur lock(mutex_);
	TRACE(5, "%s Entry\n", __FUNCTION__);

	if (!closed_)
	{
		if (client)
			return client->close();
		else
			return ISMRC_OK;
	}
	else
	{
		return ISMRC_Closed;
	}
}

//=== Free functions ==========================================================


} /* namespace ism_proxy */
