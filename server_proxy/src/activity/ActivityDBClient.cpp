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
 * ActivityDBClient.cpp
 *
 *  Created on: Oct 29, 2017
 */

#include "ActivityDBClient.h"

/* ignore deprecated warnings as we are using an old version of the mongo C driver */
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// Selects between bson_as_json and bson_as_canonical_extended_json depending on the version of the bson library.
#if BSON_CHECK_VERSION(1,7,0)
#define PX_BSON_AS_JSON(b,n) bson_as_canonical_extended_json(b,n)
#else
#define PX_BSON_AS_JSON(b,n) bson_as_json(b,n)
#endif

#define PX_MONGOC_ERROR_DUPLICATE_KEY 11000

namespace ism_proxy
{

//=== ActivityDBClient ========================================================

ActivityDBClient::ActivityDBClient(const std::string& name, ActivityDBConfig_SPtr config, mongoc_client_pool_t* pool) :
		name_(name),
		config_(config),
		pool_(pool),
		closed_(false),
		client_(NULL),
		database_(NULL),
		client_state_coll_(NULL),
		proxy_state_coll_(NULL)
{
    TRACE(5, "%s Entry: name=%s this=%p\n", __FUNCTION__, name_.c_str(), this);

	if (NULL == (client_ = mongoc_client_pool_pop(pool_)))
	{
		throw std::runtime_error("NULL mongoc_client_t");
	}


	if (NULL == (database_ = mongoc_client_get_database(client_,config->mongoDBName.c_str())))
	{
		mongoc_client_pool_push(pool_,client_);
		throw std::runtime_error("NULL mongoc_database_t");
	}

	/* write concern */
	mongoc_write_concern_t* w_concern = mongoc_write_concern_new();
	if (config_->mongoWconcern == 0)
	{
		mongoc_write_concern_set_w(w_concern, MONGOC_WRITE_CONCERN_W_DEFAULT);
	}
	else if (config_->mongoWconcern > 0)
	{
		mongoc_write_concern_set_w(w_concern, config_->mongoWconcern);
	}
	else
	{
		mongoc_write_concern_set_wmajority(w_concern, 100);
	}
	mongoc_write_concern_set_journal(w_concern, (config->mongoWjournal > 0));
	mongoc_database_set_write_concern(database_,w_concern);
	mongoc_write_concern_destroy(w_concern);

	/* read concern & preference */
	mongoc_read_concern_t* r_concern = mongoc_read_concern_new();
	switch (config_->mongoRconcern)
	{
	case 1:
	{
		mongoc_read_concern_set_level(r_concern,MONGOC_READ_CONCERN_LEVEL_LOCAL);
		break;
	}
	case 2:
	{
		mongoc_read_concern_set_level(r_concern,MONGOC_READ_CONCERN_LEVEL_MAJORITY);
		break;
	}
	case 3:
	{
#if MONGOC_CHECK_VERSION(1,5,0)
		mongoc_read_concern_set_level(r_concern,MONGOC_READ_CONCERN_LEVEL_LINEARIZABLE);
#else
		mongoc_read_concern_set_level(r_concern,MONGOC_READ_CONCERN_LEVEL_MAJORITY);
#endif
		break;
	}

	default:
		mongoc_read_concern_set_level(r_concern,MONGOC_READ_CONCERN_LEVEL_LOCAL);
	}
	mongoc_database_set_read_concern(database_, r_concern);
	mongoc_read_concern_destroy(r_concern);

	mongoc_read_prefs_t* r_prefs = mongoc_read_prefs_new(MONGOC_READ_PRIMARY);
	switch (config_->mongoRpref)
	{
	case 1:
	{
		mongoc_read_prefs_set_mode(r_prefs, MONGOC_READ_PRIMARY);
		break;
	}
	case 2:
	{
		mongoc_read_prefs_set_mode(r_prefs, MONGOC_READ_SECONDARY);
		break;
	}
	case 3:
	{
		mongoc_read_prefs_set_mode(r_prefs, MONGOC_READ_PRIMARY_PREFERRED);
		break;
	}
	case 4:
	{
		mongoc_read_prefs_set_mode(r_prefs, MONGOC_READ_SECONDARY_PREFERRED);
		break;
	}
	case 5:
	{
		mongoc_read_prefs_set_mode(r_prefs, MONGOC_READ_NEAREST);
		break;
	}
	}
	mongoc_database_set_read_prefs(database_, r_prefs);
	mongoc_read_prefs_destroy(r_prefs);

	/* create collections */
	if (NULL == (client_state_coll_ = mongoc_database_get_collection(database_, config_->mongoClientStateColl.c_str())))
	{
	    TRACE(1, "Error: %s Could not get MongoDB collection=%s from db=%s, check configuration and database schema.\n",
	            __FUNCTION__, config_->mongoClientStateColl.c_str(), config_->mongoDBName.c_str());
	    mongoc_database_destroy(database_);
	    mongoc_client_pool_push(pool_,client_);
	    throw std::runtime_error("NULL mongoc_collection_t, client_state");
	}

	if (NULL == (proxy_state_coll_ = mongoc_database_get_collection(database_, config_->mongoProxyStateColl.c_str())))
	{
	    TRACE(1, "Error: %s Could not get MongoDB collection=%s from db=%s, check configuration and database schema.\n",
	            __FUNCTION__, config_->mongoProxyStateColl.c_str(), config_->mongoDBName.c_str());
	    mongoc_database_destroy(database_);
	    mongoc_client_pool_push(pool_,client_);
	    mongoc_collection_destroy(client_state_coll_);
	    throw std::runtime_error("NULL mongoc_collection_t, proxy_state");
	}
}

ActivityDBClient::~ActivityDBClient()
{
	close();
}

int ActivityDBClient::close()
{
	TRACE(5, "%s Entry\n", __FUNCTION__);
	if (!closed_)
	{
		if (client_)
		{
			mongoc_collection_destroy(proxy_state_coll_);
			mongoc_collection_destroy(client_state_coll_);
			mongoc_database_destroy(database_);
			mongoc_client_pool_push(pool_, client_);
			client_ = NULL;
			pool_ = NULL;

			config_.reset();
		}
		closed_ = true;
		TRACE(5, "%s closed_=%d\n", __FUNCTION__, closed_);
	}
	TRACE(5, "%s Exit\n", __FUNCTION__);
	return ISMRC_OK;
}

int ActivityDBClient::insertNewUID(const std::string& proxy_uid, int hbto_ms)
{
	TRACE(9, "%s Entry: uid=%s, hbto_ms=%d\n", __FUNCTION__, proxy_uid.c_str(),hbto_ms);
	int rc = ISMRC_OK;


	//db.proxy_state.update({proxy_uid: "---", status: -1},{$currentDate: {last_hb: true}, $set: {hbto_ms: ---, status: 0, version: 1}},{upsert: true})

	/* This query never finds a document, no record has status==PX_STATUS_BOGUS
	 * The upsert fails with duplicate key if proxy_uid exists.
	 */
	bson_t query = BSON_INITIALIZER;
	bson_append_utf8(&query, ProxyStateRecord::KEY_PROXY_UID, -1, proxy_uid.c_str(), -1);
	bson_append_int32(&query,ProxyStateRecord::KEY_STATUS,-1, ProxyStateRecord::PX_STATUS_BOGUS);

	bson_t document = BSON_INITIALIZER;
	bson_t curr_date = BSON_INITIALIZER;
	bson_t set = BSON_INITIALIZER;

	bson_append_document_begin(&document, "$currentDate",-1, &curr_date);
	bson_append_bool(&curr_date,ProxyStateRecord::KEY_LAST_HB,-1,true);
	bson_append_document_end(&document, &curr_date);

	bson_append_document_begin(&document, "$set",-1, &set);
	bson_append_int32(&set,ProxyStateRecord::KEY_HBTO_MS,-1, hbto_ms);
	bson_append_int32(&set,ProxyStateRecord::KEY_STATUS,-1, ProxyStateRecord::PX_STATUS_ACTIVE);
	bson_append_int64(&set,ProxyStateRecord::KEY_VERSION,-1, 1L);

	bson_append_document_end(&document, &set);

	bson_error_t error;
	bool ok = mongoc_collection_update(proxy_state_coll_, MONGOC_UPDATE_UPSERT, &query, &document, NULL, &error);


	//FIXME
	char* q_str = PX_BSON_AS_JSON(&query, NULL);
	char* d_str = PX_BSON_AS_JSON(&document, NULL);
	TRACE(9,"%s Upsert: Query=%s, Update=%s\n", __FUNCTION__,q_str,d_str);
	bson_free(q_str);
	bson_free(d_str);

		if (!ok)
		{
			//select between Error API ver1 (legacy) or ver2
#if MONGOC_CHECK_VERSION(1,5,0)
			if (error.domain == MONGOC_ERROR_SERVER && error.code == MONGOC_ERROR_DUPLICATE_KEY)
			{
				rc = ISMRC_ExistingKey;
			}
			else
			{
				rc = ISMRC_Error;
			}
#else
			if (error.code == PX_MONGOC_ERROR_DUPLICATE_KEY) // TODO feagure what is dup-key // && error.code == MONGOC_ERROR_DUPLICATE_KEY)
			{
				rc = ISMRC_ExistingKey;
			}
			else
			{
				rc = ISMRC_Error;
			}
#endif

			TRACE(5, "%s Upsert error domain=%d code=%d msg=%s; rc=%d\n",
					__FUNCTION__, error.domain, error.code, error.message, rc);
		}

	bson_destroy(&set);
	bson_destroy(&curr_date);
	bson_destroy(&document);
	bson_destroy(&query);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}

int ActivityDBClient::removeProxyUID(const std::string& proxy_uid)
{
	TRACE(9, "%s Entry: uid=%s\n", __FUNCTION__, proxy_uid.c_str());
	int rc = ISMRC_OK;

	bson_t query = BSON_INITIALIZER;
	bson_append_utf8(&query, ProxyStateRecord::KEY_PROXY_UID, -1, proxy_uid.c_str(), -1);

	bson_error_t error;
	bool ok = mongoc_collection_remove(proxy_state_coll_,MONGOC_REMOVE_SINGLE_REMOVE,&query,NULL,&error);
	if (!ok)
	{
		rc = ISMRC_Error;

		TRACE(5, "%s Remove error, proxy_uid=%s; domain=%d code=%d msg=%s; rc=%d\n",
				__FUNCTION__, proxy_uid.c_str(), error.domain, error.code, error.message, rc);
	}

	bson_destroy(&query);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}

int ActivityDBClient::removeAllProxyUID()
{

	TRACE(9, "%s Entry: \n", __FUNCTION__);
	int rc = ISMRC_OK;

	bson_t query = BSON_INITIALIZER;

	bson_error_t error;
	bool ok = mongoc_collection_remove(proxy_state_coll_,MONGOC_REMOVE_NONE,&query,NULL,&error);
	if (!ok)
	{
		rc = ISMRC_Error;

		TRACE(5, "%s Remove error, domain=%d code=%d msg=%s; rc=%d\n",
				__FUNCTION__, error.domain, error.code, error.message, rc);
	}

	bson_destroy(&query);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;

}

int ActivityDBClient::read(ProxyStateRecord* proxy_state)
{
	TRACE(9, "%s Entry: proxy_state=%s\n", __FUNCTION__, proxy_state->toString().c_str());
	int rc = ISMRC_OK;

	bson_t query = BSON_INITIALIZER;
	bson_append_utf8(&query, ProxyStateRecord::KEY_PROXY_UID, -1, proxy_state->proxy_uid.c_str(), -1);

	mongoc_cursor_t *cursor;
	bson_error_t error;
	const int limit = 1;
	bson_t opts = BSON_INITIALIZER;

#if MONGOC_CHECK_VERSION(1,5,0)
	bson_append_int32(&opts, "limit", 5, limit);
	cursor = mongoc_collection_find_with_opts(proxy_state_coll_, &query, &opts, NULL);
#else
	cursor = mongoc_collection_find(proxy_state_coll_, MONGOC_QUERY_NONE, 0, limit, 0, &query,NULL,NULL);
#endif

	const bson_t *doc;
	if (mongoc_cursor_next (cursor, &doc))
	{
		bson_iter_t iter;
		if (bson_iter_init (&iter, doc))
		{
			while (bson_iter_next (&iter))
			{
				const char* key = bson_iter_key(&iter);
				if (strcmp(key, ProxyStateRecord::KEY_PROXY_UID) == 0)
				{
					const char* val =  bson_iter_utf8(&iter,NULL);
					if (strcmp(val,proxy_state->proxy_uid.c_str()) != 0)
					{
						rc = ISMRC_Error;
						break;
					}
				}
				else if (strcmp(key, ProxyStateRecord::KEY_LAST_HB) == 0)
				{
					proxy_state->last_hb = bson_iter_date_time(&iter);
				}
				else if (strcmp(key, ProxyStateRecord::KEY_HBTO_MS) == 0)
				{
					proxy_state->hbto_ms =  bson_iter_int32(&iter);
				}
				else if (strcmp(key, ProxyStateRecord::KEY_STATUS) == 0)
				{
					proxy_state->status = static_cast<ProxyStateRecord::PxStatus>(bson_iter_int32(&iter));
				}
				else if (strcmp(key, ProxyStateRecord::KEY_VERSION) == 0)
				{
					proxy_state->version = bson_iter_int64(&iter);
				}
			}
		}
		else
		{
			rc = ISMRC_Error;
		}
	}
	else
	{
		rc = ISMRC_NotFound;
	}

	if (rc == ISMRC_OK)
	{
		if (mongoc_cursor_error (cursor, &error))
		{
			rc = ISMRC_Error;
			TRACE(5, "%s Read error, proxy_uid=%s; domain=%d code=%d msg=%s; rc=%d\n",
					__FUNCTION__, proxy_state->proxy_uid.c_str(), error.domain, error.code, error.message, rc);
		}
		else
		{
			TRACE(7, "%s Read OK, proxy_state=%s\n",__FUNCTION__, proxy_state->toString().c_str());
		}
	}

	mongoc_cursor_destroy(cursor);
	bson_destroy (&query);
	bson_destroy (&opts);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}

int ActivityDBClient::updateHeartbeat(ProxyStateRecord* proxy_state)
{
	if (proxy_state == NULL) {
		return ISMRC_NullPointer;
	}

	TRACE(9, "%s Entry: uid=%s\n", __FUNCTION__, proxy_state->proxy_uid.c_str());
	int rc = ISMRC_OK;

	bson_t* query = BCON_NEW(ProxyStateRecord::KEY_PROXY_UID,BCON_UTF8(proxy_state->proxy_uid.c_str()));
	bson_t* update = BCON_NEW ("$currentDate", "{", ProxyStateRecord::KEY_LAST_HB, BCON_BOOL(true), "}", "$inc", "{", ProxyStateRecord::KEY_VERSION, BCON_INT32(1), "}");
	bson_t reply;

	char* q_str = PX_BSON_AS_JSON(query, NULL);
	char* u_str = PX_BSON_AS_JSON(update, NULL);
	TRACE(9,"%s Find and modify: Query=%s, Update=%s\n", __FUNCTION__,
			q_str, u_str);
	bson_free(q_str);
	bson_free(u_str);

	bson_error_t error;
	bool ok = mongoc_collection_find_and_modify(proxy_state_coll_, query, NULL, update, NULL, false, false, true, &reply, &error);

	if (!ok)
	{
		rc = ISMRC_Error;
		TRACE(1, "Error: %s Find and modify: domain=%d code=%d msg=%s; rc=%d\n",
				__FUNCTION__, error.domain, error.code, error.message, rc);
	}
	else
	{
		char* r_str = PX_BSON_AS_JSON(&reply, NULL);
		TRACE(9, "%s reply record: %s", __FUNCTION__, r_str);
		bson_free(r_str);

		bson_iter_t iter;
		bson_iter_t iter_child;
		if (bson_iter_init_find(&iter, &reply,"value")
				&& BSON_ITER_HOLDS_DOCUMENT (&iter)
				&& bson_iter_recurse (&iter, &iter_child))
		{
			int num_found = 0;
			while (bson_iter_next (&iter_child))
			{
				num_found++;

				const char* key = bson_iter_key(&iter_child);
				if (strcmp(key, ProxyStateRecord::KEY_PROXY_UID) == 0)
				{
					const char* val =  bson_iter_utf8(&iter_child,NULL);
					if (strcmp(val,proxy_state->proxy_uid.c_str()) != 0)
					{
						rc = ISMRC_Error;
						break;
					}
				}
				else if (strcmp(key, ProxyStateRecord::KEY_LAST_HB) == 0)
				{
					proxy_state->last_hb = bson_iter_date_time(&iter_child);
				}
				else if (strcmp(key, ProxyStateRecord::KEY_HBTO_MS) == 0)
				{
					proxy_state->hbto_ms =  bson_iter_int32(&iter_child);
				}
				else if (strcmp(key, ProxyStateRecord::KEY_STATUS) == 0)
				{
					proxy_state->status = static_cast<ProxyStateRecord::PxStatus>(bson_iter_int32(&iter_child));
				}
				else if (strcmp(key, ProxyStateRecord::KEY_VERSION) == 0)
				{
					proxy_state->version = bson_iter_int64(&iter_child);
				}
				//skip _id
			}

			if (num_found != 6)
			{
				rc = ISMRC_Error;
			}
		}
		else
		{
			rc = ISMRC_Error;
		}
	}

	bson_destroy(update);
	bson_destroy(query);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}

int ActivityDBClient::updateStatus_CondVersion(const std::string& proxy_uid, ProxyStateRecord::PxStatus status, int64_t version)
{
	//TODO implement with update & get_last_error

	TRACE(9, "%s Entry: uid=%s, status=%d, version=%ld\n", __FUNCTION__, proxy_uid.c_str(), status, version);
	int rc = ISMRC_OK;

	bson_t* query = BCON_NEW(ProxyStateRecord::KEY_PROXY_UID,BCON_UTF8(proxy_uid.c_str()), ProxyStateRecord::KEY_VERSION, BCON_INT64(version));
	bson_t* update = BCON_NEW ("$set", "{", ProxyStateRecord::KEY_STATUS, BCON_INT32(status), "}", "$inc", "{", ProxyStateRecord::KEY_VERSION, BCON_INT32(1), "}");
	bson_t reply;

	char* q_str = PX_BSON_AS_JSON(query, NULL);
	char* u_str = PX_BSON_AS_JSON(update, NULL);
	TRACE(9,"%s Find and modify: Query=%s, Update=%s\n", __FUNCTION__,
			q_str, u_str);
	bson_free(q_str);
	bson_free(u_str);

	bson_error_t error;
	bool ok = mongoc_collection_find_and_modify(proxy_state_coll_, query, NULL, update, NULL, false, false, true, &reply, &error);
	if (!ok)
	{
		rc = ISMRC_Error;
		TRACE(5, "%s Find and modify failed: domain=%d code=%d msg=%s; rc=%d\n",
				__FUNCTION__, error.domain, error.code, error.message, rc);
	}
	else
	{
		char* r_str = PX_BSON_AS_JSON(&reply, NULL);
		TRACE(9, "%s reply record: %s", __FUNCTION__, r_str);
		bson_free(r_str);

		//=== not found:
		// {
		//	 "lastErrorObject" : { "updatedExisting" : false, "n" : { "$numberInt" : "0" } },
		//   "value" : null,
		//   "ok" : { "$numberDouble" : "1.0" }
		// }
		//=== found:
		// {
		//   "lastErrorObject" : { "updatedExisting" : true, "n" : { "$numberInt" : "1" } },
		//   "value" : { "_id" : { "$oid" : "5a0059698201b183e254ae3d" }, "proxy_uid" : "proxy3",
		//               "status" : { "$numberInt" : "0" }, "last_hb" : { "$date" : { "$numberLong" : "1509972329177" } },
		//		         "hbto_ms" : { "$numberInt" : "20000" }, "version" : { "$numberLong" : "2"} },
		//   "ok" : { "$numberDouble" : "1.0" }
		// }

		bson_iter_t iter;
		bson_iter_t iter_child;
		if (bson_iter_init_find(&iter, &reply,"lastErrorObject")
				&& BSON_ITER_HOLDS_DOCUMENT (&iter)
				&& bson_iter_recurse (&iter, &iter_child))
		{

			if (bson_iter_find(&iter_child, "updatedExisting"))
			{
				bool updatedExisting = bson_iter_bool(&iter_child);
				if (!updatedExisting)
				{
					rc = ISMRC_NotFound;
					TRACE(5, "%s condition failed, updateExisting=False; rc=%d\n", __FUNCTION__, rc);
				}
			}
			else
			{
				rc = ISMRC_Error;
				TRACE(1, "Error: %s cannot find updatedExisting field; rc=%d\n", __FUNCTION__, rc);
			}

		}
		else
		{
			rc = ISMRC_Error;
			TRACE(1, "Error: %s cannot find lastErrorObject field; rc=%d\n", __FUNCTION__, rc);
		}
	}

	bson_destroy(&reply);
	bson_destroy(update);
	bson_destroy(query);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}


int ActivityDBClient::getAllWithStatus(ProxyStateRecord::PxStatus status, ProxyStateRecord* proxy_state, int* list_size)
{
	TRACE(9, "%s Entry: status=%d \n", __FUNCTION__, status);
	int rc = ISMRC_OK;

	bson_t* query = BCON_NEW(ProxyStateRecord::KEY_STATUS,BCON_INT32(status));
	const int limit = 0;
	char* q_str = PX_BSON_AS_JSON(query, NULL);
	TRACE(9,"%s Find all with status: Query=%s, limit=%d \n", __FUNCTION__,
			q_str, limit);
	bson_free(q_str);

#if MONGOC_CHECK_VERSION(1,5,0)
	bson_t* opts = BCON_NEW("limit",BCON_INT64(limit));
	mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(proxy_state_coll_, query, opts, NULL);
#else
	mongoc_cursor_t* cursor = mongoc_collection_find(proxy_state_coll_, MONGOC_QUERY_NONE, 0, limit, 0, query, NULL, NULL);
#endif

	rc = parseFindResults(cursor, proxy_state, list_size);

	mongoc_cursor_destroy(cursor);
#if MONGOC_CHECK_VERSION(1,5,0)
	bson_destroy(opts);
#endif
	bson_destroy(query);
	if (rc != ISMRC_OK && proxy_state->next != NULL)
	{
		proxy_state->destroyNext();
	}

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}

int ActivityDBClient::getAllWithStatusIn(const std::vector<ProxyStateRecord::PxStatus>& status_vec, ProxyStateRecord* proxy_state, int* list_size)
{
    using namespace std;

    TRACE(9, "%s Entry: #status=%ld\n", __FUNCTION__, status_vec.size());
    int rc = ISMRC_OK;

    if (status_vec.size() == 0)
    {
        if (list_size)
        {
            *list_size = 0;
        }
        TRACE(9, "%s Exit: empty set, rc=%d\n", __FUNCTION__, rc);
        return rc;
    }
    else if (status_vec.size() == 1)
    {
        return getAllWithStatus(status_vec[0], proxy_state, list_size);
    }

    ostringstream q;
    q << "{ \"status\" : { \"$in\" : [";
    const unsigned N = status_vec.size();
    for (unsigned i=0; i < N-1; ++i)
    {
        q << status_vec[i] << ",";
    }
    q << status_vec[N-1] << "]}}";
    string qs = q.str();
    bson_error_t error;
    bson_t* query = bson_new_from_json(reinterpret_cast<const uint8_t*>(qs.c_str()), -1, &error);
    if (!query)
    {
        rc = ISMRC_Error;
        TRACE(1, "Error: %s, bson_new_from_json, error.domain=%d, error.code=%d, error.msg=%s; q=%s; rc=%d\n",
                __FUNCTION__, error.domain, error.code, error.message, qs.c_str(), rc);
        return rc;
    }

    const int limit = 0;
    TRACE(9,"%s Find all with status: Query=%s, limit=%d \n",
            __FUNCTION__, q.str().c_str(), limit);

#if MONGOC_CHECK_VERSION(1,5,0)
    bson_t* opts = BCON_NEW("limit",BCON_INT64(limit));
    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(proxy_state_coll_, query, opts, NULL);
#else
    mongoc_cursor_t* cursor = mongoc_collection_find(proxy_state_coll_, MONGOC_QUERY_NONE, 0, limit, 0, query, NULL, NULL);
#endif

    rc = parseFindResults(cursor, proxy_state, list_size);

    mongoc_cursor_destroy(cursor);
#if MONGOC_CHECK_VERSION(1,5,0)
    bson_destroy(opts);
#endif
    bson_destroy(query);
    if (rc != ISMRC_OK && proxy_state->next != NULL)
    {
        proxy_state->destroyNext();
    }

    TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
    return rc;
}

int ActivityDBClient::parseFindResults(mongoc_cursor_t* cursor, ProxyStateRecord* proxy_state, int* list_size)
{
    TRACE(9, "%s Entry:\n", __FUNCTION__);
    int rc = ISMRC_OK;
    ProxyStateRecord* current = NULL;
    const bson_t *doc;
    int num_found = 0;
    while (mongoc_cursor_next (cursor, &doc))
    {
        num_found++;
        if (current == NULL)
        {
            current = proxy_state;
        }
        else
        {
            current->next = new ProxyStateRecord;
            current = current->next;
        }

        bson_iter_t iter;
        if (bson_iter_init (&iter, doc))
        {
            while (bson_iter_next (&iter))
            {
                const char* key = bson_iter_key(&iter);
                if (strcmp(key, ProxyStateRecord::KEY_PROXY_UID) == 0)
                {
                    current->proxy_uid = bson_iter_utf8(&iter,NULL);
                }
                else if (strcmp(key, ProxyStateRecord::KEY_LAST_HB) == 0)
                {
                    current->last_hb = bson_iter_date_time(&iter);
                }
                else if (strcmp(key, ProxyStateRecord::KEY_HBTO_MS) == 0)
                {
                    current->hbto_ms =  bson_iter_int32(&iter);
                }
                else if (strcmp(key, ProxyStateRecord::KEY_STATUS) == 0)
                {
                    current->status = static_cast<ProxyStateRecord::PxStatus>(bson_iter_int32(&iter));
                }
                else if (strcmp(key, ProxyStateRecord::KEY_VERSION) == 0)
                {
                    current->version = bson_iter_int64(&iter);
                }
            }
        }
        else
        {
            rc = ISMRC_Error;
            break;
        }
    }


    if (list_size)
    {
        *list_size = num_found;
    }

    if (rc == ISMRC_OK && num_found == 0)
    {
        rc = ISMRC_NotFound;
    }

    TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
    return rc;
}

//=== Client methods ===

int ActivityDBClient::read(ClientStateRecord* client_state)
{
	TRACE(9, "%s Entry: client_state=%s\n", __FUNCTION__, client_state->toString().c_str());
	int rc = ISMRC_OK;

	bson_t query = BSON_INITIALIZER;
	bson_append_utf8(&query, ClientStateRecord::KEY_ORG, -1, client_state->org.c_str(), -1);
	bson_append_utf8(&query, ClientStateRecord::KEY_CLIENT_ID, -1, client_state->client_id.c_str(), -1);

	const int limit = 1;

	mongoc_cursor_t *cursor;
	bson_error_t error;
#if MONGOC_CHECK_VERSION(1,5,0)
	bson_t opts = BSON_INITIALIZER;
	bson_append_int32(&opts, "limit", 5, limit);
	cursor = mongoc_collection_find_with_opts(client_state_coll_, &query, &opts, NULL);
#else
	cursor = mongoc_collection_find(client_state_coll_, MONGOC_QUERY_NONE, 0, limit, 0, &query, NULL, NULL);
#endif

	const bson_t *doc;
	if (mongoc_cursor_next (cursor, &doc))
	{
		bson_iter_t iter;
		if (bson_iter_init (&iter, doc))
		{
			while (bson_iter_next (&iter))
			{
				const char* key = bson_iter_key(&iter);
				if (strcmp(key, ClientStateRecord::KEY_CLIENT_ID) == 0)
				{
					const char* val = bson_iter_utf8(&iter, NULL);
					if (strcmp(val, client_state->client_id.c_str()) != 0)
					{
						rc = ISMRC_Error;
						break;
					}
				}
				else if (strcmp(key, ClientStateRecord::KEY_ORG) == 0)
				{
					client_state->org = bson_iter_utf8(&iter, NULL);
				}
				else if (strcmp(key, ClientStateRecord::KEY_DEVICE_TYPE) == 0)
				{
					client_state->device_type = bson_iter_utf8(&iter, NULL);
				}
				else if (strcmp(key, ClientStateRecord::KEY_DEVICE_ID) == 0)
				{
					client_state->device_id = bson_iter_utf8(&iter, NULL);
				}
				else if (strcmp(key, ClientStateRecord::KEY_CLIENT_TYPE) == 0)
				{
					client_state->client_type = static_cast<PXACT_CLIENT_TYPE_t>(bson_iter_int32(&iter));
				}
				else if (strcmp(key, ClientStateRecord::KEY_CONN_EV_TIME) == 0)
				{
					client_state->conn_ev_time = bson_iter_date_time(&iter);
				}
				else if (strcmp(key, ClientStateRecord::KEY_CONN_EV_TYPE) == 0)
				{
					client_state->conn_ev_type = static_cast<PXACT_ACTIVITY_TYPE_t>(bson_iter_int32(&iter));
				}
                else if (strcmp(key, ClientStateRecord::KEY_LAST_CONNECT_TIME) == 0)
                {
                    client_state->last_connect_time = bson_iter_date_time(&iter);
                }
                else if (strcmp(key, ClientStateRecord::KEY_LAST_DISCONNECT_TIME) == 0)
                {
                    client_state->last_disconnect_time = bson_iter_date_time(&iter);
                }
				else if (strcmp(key, ClientStateRecord::KEY_LAST_ACT_TIME) == 0)
				{
					client_state->last_act_time = bson_iter_date_time(&iter);
				}
				else if (strcmp(key, ClientStateRecord::KEY_LAST_PUB_TIME) == 0)
				{
					client_state->last_pub_time = bson_iter_date_time(&iter);
				}
				else if (strcmp(key, ClientStateRecord::KEY_LAST_ACT_TYPE) == 0)
				{
					client_state->last_act_type = static_cast<PXACT_ACTIVITY_TYPE_t>(bson_iter_int32(&iter));
				}
				else if (strcmp(key, ClientStateRecord::KEY_PROTOCOL) == 0)
				{
					client_state->protocol = static_cast<PXACT_CONN_PROTOCOL_TYPE_t>(bson_iter_int32(&iter));
				}
				else if (strcmp(key, ClientStateRecord::KEY_CLEAN_S) == 0)
				{
					client_state->clean_s = bson_iter_int32(&iter);
				}
				else if (strcmp(key, ClientStateRecord::KEY_EXPIRY_SEC) == 0)
				{
					client_state->expiry_sec = bson_iter_int32(&iter);
				}
				else if (strcmp(key, ClientStateRecord::KEY_REASON_CODE) == 0)
				{
					client_state->reason_code = bson_iter_int32(&iter);
				}
				else if (strcmp(key, ClientStateRecord::KEY_PROXY_UID) == 0)
				{
					client_state->proxy_uid = bson_iter_utf8(&iter, NULL);
				}
				else if (strcmp(key, ClientStateRecord::KEY_GATEWAY_ID) == 0)
				{
					client_state->gateway_id = bson_iter_utf8(&iter, NULL);
				}
				else if (strcmp(key, ClientStateRecord::KEY_TARGET_SERVER) == 0)
				{
					client_state->target_server = static_cast<PXACT_TRG_SERVER_TYPE_t>(bson_iter_int32(&iter));
				}
				else if (strcmp(key, ClientStateRecord::KEY_VERSION) == 0)
				{
					client_state->version = bson_iter_int64(&iter);
				}
			}
		}
		else
		{
			rc = ISMRC_Error;
		}
	}
	else
	{
	    if (mongoc_cursor_error (cursor, &error))
	    {
	        rc = ISMRC_Error;
	        TRACE(5, "%s Read error, client_id=%s; domain=%d code=%d msg=%s; rc=%d\n",
	                __FUNCTION__, client_state->client_id.c_str(), error.domain, error.code, error.message, rc);
	    }
	    else
	    {
	        rc = ISMRC_NotFound;
	    }
	}

	if (rc == ISMRC_OK)
	{
		if (mongoc_cursor_error (cursor, &error))
		{
			rc = ISMRC_Error;
			TRACE(5, "%s Read error, client_id=%s; domain=%d code=%d msg=%s; rc=%d\n",
					__FUNCTION__, client_state->client_id.c_str(), error.domain, error.code, error.message, rc);
		}
		else
		{
			TRACE(7, "%s Read OK, client_state=%s\n",__FUNCTION__, client_state->toString().c_str());
		}
	}

	mongoc_cursor_destroy(cursor);
	bson_destroy (&query);
#if MONGOC_CHECK_VERSION(1,5,0)
	bson_destroy (&opts);
#endif

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}

int ActivityDBClient::insert(ClientUpdateRecord* client_update, const std::string& proxy_uid)
{
	TRACE(9, "%s Entry: client_state=%s\n", __FUNCTION__, client_update->toString().c_str());
	int rc = ISMRC_OK;

	if ((client_update->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD) == 0)
	{
		rc = PXACT_ERROR_t::PXACT_ERROR_DIRTY_FLAG_INVALID;
		TRACE(5, "Error: %s can insert only if ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD is set, state=%s; rc=%d\n",
				__FUNCTION__, client_update->toString().c_str(), rc);
		return rc;
	}

	if (client_update->dirty_flags == ClientUpdateRecord::DIRTY_FLAG_CLEAN)
	{
		rc = PXACT_ERROR_t::PXACT_ERROR_DIRTY_FLAG_INVALID;
		TRACE(5, "Error: %s cannot insert a clean record, state=%s; rc=%d\n",
				__FUNCTION__, client_update->toString().c_str(), rc);
		return rc;
	}

	//db.client_state.update(
	//  {proxy_uid: "---", version: -1},
	//  {$currentDate: {conn_ev_time: true, last_act_time: true}, $set: { ... }},
	//  {upsert: true})

	/* This query never finds a document, no record has version==-1.
	 * The upsert fails with duplicate key if client_id exists.
	 */
	bson_t query = BSON_INITIALIZER;
	bson_append_utf8(&query, ClientStateRecord::KEY_ORG, sizeof(PXACT_CLIENT_STATE_KEY_ORG)-1, client_update->org->c_str(), -1);
	bson_append_utf8(&query, ClientStateRecord::KEY_CLIENT_ID, sizeof(PXACT_CLIENT_STATE_KEY_CLIENT_ID)-1, client_update->client_id->c_str(), -1);
	bson_append_int64(&query,ClientStateRecord::KEY_VERSION, sizeof(PXACT_CLIENT_STATE_KEY_VERSION)-1, -1L);

	bson_t document = BSON_INITIALIZER;
	bson_t curr_date = BSON_INITIALIZER;
	bson_t set = BSON_INITIALIZER;

	bool act_time = (client_update->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT) >0;
	bool conn_time = (client_update->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT) >0;
	bool is_connect = ClientStateRecord::isConnectType(static_cast<PXACT_ACTIVITY_TYPE_t>(client_update->conn_ev_type));
	bool is_disconnect = ClientStateRecord::isDisconnectType(static_cast<PXACT_ACTIVITY_TYPE_t>(client_update->disconn_ev_type));

	if (act_time || conn_time)
	{
		bson_append_document_begin(&document, "$currentDate",-1, &curr_date);
		if (act_time)
		{
			bson_append_bool(&curr_date,ClientStateRecord::KEY_LAST_ACT_TIME,sizeof(PXACT_CLIENT_STATE_KEY_LAST_ACT_TIME)-1,true);
		}
		if (conn_time)
		{
			bson_append_bool(&curr_date,ClientStateRecord::KEY_CONN_EV_TIME,sizeof(PXACT_CLIENT_STATE_KEY_CONN_EV_TIME)-1,true);
		}
		bson_append_document_end(&document, &curr_date);
	}

	//	std::string 			client_id; 		///< Client ID, globally unique, Org name embedded as prefix
	//	std::string 			org;			///< Org name
	//	PXACT_CLIENT_TYPE_t 	client_type;	///< Client type
	//	int64_t 				conn_ev_time;	///< connection event time (millisec since Unix epoch)
	//	PXACT_ACTIVITY_TYPE_t 	conn_ev_type;   ///< Connection event type; can only be Connect, Disconnect, or None.
	//	int64_t 				last_act_time;  ///< last activity time
	//	PXACT_ACTIVITY_TYPE_t 	last_act_type;	///< last activity type
	//	int32_t					clean_s; 		///< clean_s MQTT session, 0: false, 1: true, -1: N/A
	//  int32_t					expiry_sec;
	//	std::string				proxy_uid;		///< the UID of the proxy handling the event
	//	std::string				gateway_id;		///< the gateway client ID for devices behind a gateway
	//	int32_t					target_server; 	///< 0: N/A, 1: MS, 2: Kafka, 3: MS+Kafka
	//	int64_t					version;		///< incremented on every write, for light-weight transactions

	bson_append_document_begin(&document, "$set",-1, &set);
	bson_append_utf8(&set,ClientStateRecord::KEY_ORG,sizeof(PXACT_CLIENT_STATE_KEY_ORG)-1,
			client_update->org->c_str(), client_update->org->size());
	bson_append_utf8(&set,ClientStateRecord::KEY_DEVICE_TYPE,sizeof(PXACT_CLIENT_STATE_KEY_DEVICE_TYPE)-1,
			client_update->device_type->c_str(), client_update->device_type->size());
	bson_append_utf8(&set,ClientStateRecord::KEY_DEVICE_ID,sizeof(PXACT_CLIENT_STATE_KEY_DEVICE_ID)-1,
			client_update->device_id->c_str(), client_update->device_id->size());
	bson_append_int32(&set, ClientStateRecord::KEY_CLIENT_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_CLIENT_TYPE)-1, (int32_t)client_update->client_type);
	if (!conn_time)
	{
		bson_append_date_time(&set, ClientStateRecord::KEY_CONN_EV_TIME, sizeof(PXACT_CLIENT_STATE_KEY_CONN_EV_TIME)-1, 0);
		bson_append_int32(&set, ClientStateRecord::KEY_CONN_EV_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_CONN_EV_TYPE)-1, PXACT_ACTIVITY_TYPE_NONE);
	    bson_append_date_time(&set, ClientStateRecord::KEY_LAST_CONNECT_TIME, sizeof(PXACT_CLIENT_STATE_KEY_LAST_CONNECT_TIME)-1, 0);
	    bson_append_date_time(&set, ClientStateRecord::KEY_LAST_DISCONNECT_TIME, sizeof(PXACT_CLIENT_STATE_KEY_LAST_DISCONNECT_TIME)-1, 0);
		bson_append_utf8(&set, ClientStateRecord::KEY_PROXY_UID, sizeof(PXACT_CLIENT_STATE_KEY_PROXY_UID)-1, PROXY_UID_NULL, -1);
	    bson_append_int32(&set, ClientStateRecord::KEY_PROTOCOL, sizeof(PXACT_CLIENT_STATE_KEY_PROTOCOL)-1, PXACT_CONN_PROTO_TYPE_NONE);
	    bson_append_int32(&set, ClientStateRecord::KEY_CLEAN_S, sizeof(PXACT_CLIENT_STATE_KEY_CLEAN_S)-1, -1);
	    bson_append_int32(&set, ClientStateRecord::KEY_REASON_CODE, sizeof(PXACT_CLIENT_STATE_KEY_REASON_CODE)-1, 0);
	    bson_append_int32(&set, ClientStateRecord::KEY_EXPIRY_SEC, sizeof(PXACT_CLIENT_STATE_KEY_EXPIRY_SEC)-1, 0);
	}
	else
	{
		if (is_connect)
		{
			bson_append_int32(&set, ClientStateRecord::KEY_CONN_EV_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_CONN_EV_TYPE)-1, (int32_t)client_update->conn_ev_type);
			bson_append_date_time(&set,ClientStateRecord::KEY_LAST_CONNECT_TIME,sizeof(PXACT_CLIENT_STATE_KEY_LAST_CONNECT_TIME)-1, (int64_t)client_update->connect_time);
			//bson_append_date_time(&set,ClientStateRecord::KEY_CONN_EV_TIME, sizeof(PXACT_CLIENT_STATE_KEY_CONN_EV_TIME)-1, (int64_t)client_update->connect_time);
		}
		if (is_disconnect)
		{
			bson_append_int32(&set, ClientStateRecord::KEY_CONN_EV_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_CONN_EV_TYPE)-1, (int32_t)client_update->disconn_ev_type);
			bson_append_date_time(&set,ClientStateRecord::KEY_LAST_DISCONNECT_TIME,sizeof(PXACT_CLIENT_STATE_KEY_LAST_DISCONNECT_TIME)-1,(int64_t)client_update->disconnect_time);
			//bson_append_date_time(&set,ClientStateRecord::KEY_CONN_EV_TIME, sizeof(PXACT_CLIENT_STATE_KEY_CONN_EV_TIME)-1, (int64_t)client_update->disconnect_time);
		}
		bson_append_utf8(&set, ClientStateRecord::KEY_PROXY_UID, sizeof(PXACT_CLIENT_STATE_KEY_PROXY_UID)-1, proxy_uid.c_str(), proxy_uid.size());
	    bson_append_int32(&set, ClientStateRecord::KEY_PROTOCOL, sizeof(PXACT_CLIENT_STATE_KEY_PROTOCOL)-1, (int32_t)client_update->protocol);
	    bson_append_int32(&set, ClientStateRecord::KEY_CLEAN_S, sizeof(PXACT_CLIENT_STATE_KEY_CLEAN_S)-1, (int32_t)client_update->clean_s);
	    bson_append_int32(&set, ClientStateRecord::KEY_REASON_CODE, sizeof(PXACT_CLIENT_STATE_KEY_REASON_CODE)-1, (int32_t)client_update->reason_code);
	    bson_append_int32(&set, ClientStateRecord::KEY_EXPIRY_SEC, sizeof(PXACT_CLIENT_STATE_KEY_EXPIRY_SEC)-1, client_update->expiry_sec);

	}

	if (!act_time)
	{
		bson_append_date_time(&set, ClientStateRecord::KEY_LAST_ACT_TIME, sizeof(PXACT_CLIENT_STATE_KEY_LAST_ACT_TIME)-1, 0);
		bson_append_int32(&set, ClientStateRecord::KEY_LAST_ACT_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_LAST_ACT_TYPE)-1, PXACT_ACTIVITY_TYPE_NONE);
	}
	else
	{
		bson_append_int32(&set, ClientStateRecord::KEY_LAST_ACT_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_LAST_ACT_TYPE)-1, (int32_t)client_update->last_act_type);
	}


	bson_append_utf8(&set, ClientStateRecord::KEY_GATEWAY_ID, sizeof(PXACT_CLIENT_STATE_KEY_GATEWAY_ID)-1,
			client_update->gateway_id->c_str(), client_update->gateway_id->size());
	bson_append_int32(&set, ClientStateRecord::KEY_TARGET_SERVER, sizeof(PXACT_CLIENT_STATE_KEY_TARGET_SERVER)-1, (int32_t)client_update->target_server);
	bson_append_int64(&set, ClientStateRecord::KEY_VERSION, sizeof(PXACT_CLIENT_STATE_KEY_VERSION)-1, 1L);

	bson_append_document_end(&document, &set);

	bson_error_t error;
	bool ok = mongoc_collection_update(client_state_coll_, MONGOC_UPDATE_UPSERT, &query, &document, NULL, &error);

	char* q_str = PX_BSON_AS_JSON(&query, NULL);
	char* d_str = PX_BSON_AS_JSON(&document, NULL);
	TRACE(9,"%s Upsert: Query=%s, Update=%s\n", __FUNCTION__,
			q_str, d_str);
	bson_free(q_str);
	bson_free(d_str);

	if (!ok)
	{
		//select between Error API ver1 (legacy) or ver2
#if MONGOC_CHECK_VERSION(1,5,0)
		if (error.domain == MONGOC_ERROR_SERVER && error.code == MONGOC_ERROR_DUPLICATE_KEY)
		{
			rc = ISMRC_ExistingKey;
		}
		else
		{
			rc = ISMRC_Error;
		}
#else
		if (error.code == PX_MONGOC_ERROR_DUPLICATE_KEY) // TODO feagure what is dup-key // && error.code == MONGOC_ERROR_DUPLICATE_KEY)
		{
			rc = ISMRC_ExistingKey;
		}
		else
		{
			rc = ISMRC_Error;
		}
#endif

		TRACE(5, "%s Upset error domain=%d code=%d msg=%s; rc=%d\n",
				__FUNCTION__, error.domain, error.code, error.message, rc);
	}

	bson_destroy(&set);
	bson_destroy(&curr_date);
	bson_destroy(&document);
	bson_destroy(&query);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}

int ActivityDBClient::removeClientID(const std::string& org, const std::string& client_id)
{
	TRACE(9, "%s Entry: client_id=%s\n", __FUNCTION__, client_id.c_str());
	int rc = ISMRC_OK;


	bson_t query = BSON_INITIALIZER;
	bson_append_utf8(&query, ClientStateRecord::KEY_ORG, -1, org.c_str(), -1);
	bson_append_utf8(&query, ClientStateRecord::KEY_CLIENT_ID, -1, client_id.c_str(), -1);

	bson_error_t error;
	bool ok = mongoc_collection_remove(client_state_coll_,MONGOC_REMOVE_SINGLE_REMOVE,&query,NULL,&error);
	if (!ok)
	{
		rc = ISMRC_Error;

		TRACE(5, "%s Remove error, client_id=%s; domain=%d code=%d msg=%s; rc=%d\n",
				__FUNCTION__, client_id.c_str(), error.domain, error.code, error.message, rc);
	}

	bson_destroy(&query);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}


int ActivityDBClient::removeAllClientID()
{
	TRACE(9, "%s Entry: \n", __FUNCTION__);
	int rc = ISMRC_OK;

	bson_t query = BSON_INITIALIZER;

	bson_error_t error;
	bool ok = mongoc_collection_remove(client_state_coll_,MONGOC_REMOVE_NONE,&query,NULL,&error);
	if (!ok)
	{
		rc = ISMRC_Error;

		TRACE(5, "%s Remove error, domain=%d code=%d msg=%s; rc=%d\n",
				__FUNCTION__, error.domain, error.code, error.message, rc);
	}

	bson_destroy(&query);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}


int ActivityDBClient::updateOne(ClientUpdateRecord* client_update, const std::string& proxy_uid)
{
	TRACE(9, "%s Entry: client_state=%s, proxy_uid=%s\n", __FUNCTION__,
			client_update->toString().c_str(), proxy_uid.c_str());

	int rc = updateOne_CondVersion(client_update, proxy_uid, -1);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}


int ActivityDBClient::updateOne_CondVersion(ClientUpdateRecord* client_update, const std::string& proxy_uid, int64_t version)
{
	TRACE(9, "%s Entry: client_state=%s, proxy_uid=%s, version=%ld\n", __FUNCTION__,
			client_update->toString().c_str(), proxy_uid.c_str(), version);
	int rc = ISMRC_OK;

	if ((client_update->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD) >0 )
	{
		rc = PXACT_ERROR_t::PXACT_ERROR_DIRTY_FLAG_INVALID;
		TRACE(1, "Error: %s can update only if ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD is not set, state=%s; rc=%d\n",
				__FUNCTION__, client_update->toString().c_str(), rc);
		return rc;
	}

	if (client_update->dirty_flags == ClientUpdateRecord::DIRTY_FLAG_CLEAN)
	{
		rc = PXACT_ERROR_t::PXACT_ERROR_DIRTY_FLAG_INVALID;
		TRACE(1, "Error: %s cannot update a clean record, state=%s; rc=%d\n",
				__FUNCTION__, client_update->toString().c_str(), rc);
		return rc;
	}

	bson_t* query;
	if (version >= 0)
		query = BCON_NEW(ClientStateRecord::KEY_ORG,client_update->org->c_str(),ClientStateRecord::KEY_CLIENT_ID,BCON_UTF8(client_update->client_id->c_str()),ClientStateRecord::KEY_VERSION,BCON_INT64(version));
	else
		query = BCON_NEW(ClientStateRecord::KEY_ORG,client_update->org->c_str(),ClientStateRecord::KEY_CLIENT_ID,BCON_UTF8(client_update->client_id->c_str()));

	bson_t document = BSON_INITIALIZER;
	bson_t curr_date = BSON_INITIALIZER;
	bson_t set = BSON_INITIALIZER;
	bson_t increment = BSON_INITIALIZER;

	bool act_time = (client_update->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT) >0;
	bool conn_time = (client_update->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT) >0;
	bool is_connect = ClientStateRecord::isConnectType(static_cast<PXACT_ACTIVITY_TYPE_t>(client_update->conn_ev_type));
	bool is_disconnect = ClientStateRecord::isDisconnectType(static_cast<PXACT_ACTIVITY_TYPE_t>(client_update->disconn_ev_type));
	bool is_pub = ClientStateRecord::isPubType(static_cast<PXACT_ACTIVITY_TYPE_t>(client_update->last_act_type));
	bool meta = (client_update->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_METADATA) >0;

	if (act_time || conn_time)
	{
		bson_append_document_begin(&document, "$currentDate",-1, &curr_date);
		if (act_time)
		{
			bson_append_bool(&curr_date,ClientStateRecord::KEY_LAST_ACT_TIME, sizeof(PXACT_CLIENT_STATE_KEY_LAST_ACT_TIME)-1, true);
		}
		if (conn_time)
		{
			bson_append_bool(&curr_date,ClientStateRecord::KEY_CONN_EV_TIME, sizeof(PXACT_CLIENT_STATE_KEY_CONN_EV_TIME)-1, true);
		}
		if (is_pub) {
			bson_append_bool(&curr_date,ClientStateRecord::KEY_LAST_PUB_TIME, sizeof(PXACT_CLIENT_STATE_KEY_LAST_PUB_TIME)-1, true);
		}
		bson_append_document_end(&document, &curr_date);
	}

	bson_append_document_begin(&document, "$set",-1, &set);
	if (meta)
	{
		bson_append_int32(&set, ClientStateRecord::KEY_CLIENT_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_CLIENT_TYPE)-1, (int32_t)client_update->client_type);
		bson_append_int32(&set, ClientStateRecord::KEY_PROTOCOL, sizeof(PXACT_CLIENT_STATE_KEY_PROTOCOL)-1, (int32_t)client_update->protocol);
		bson_append_int32(&set, ClientStateRecord::KEY_CLEAN_S, sizeof(PXACT_CLIENT_STATE_KEY_CLEAN_S)-1, (int32_t)client_update->clean_s);
		bson_append_int32(&set, ClientStateRecord::KEY_REASON_CODE, sizeof(PXACT_CLIENT_STATE_KEY_REASON_CODE)-1, (int32_t)client_update->reason_code);
		bson_append_int32(&set, ClientStateRecord::KEY_EXPIRY_SEC, sizeof(PXACT_CLIENT_STATE_KEY_EXPIRY_SEC)-1, client_update->expiry_sec);
		bson_append_utf8(&set,ClientStateRecord::KEY_GATEWAY_ID, sizeof(PXACT_CLIENT_STATE_KEY_GATEWAY_ID)-1, client_update->gateway_id->c_str(), client_update->gateway_id->size());
		bson_append_int32(&set, ClientStateRecord::KEY_TARGET_SERVER, sizeof(PXACT_CLIENT_STATE_KEY_TARGET_SERVER)-1, (int32_t)client_update->target_server);
	}
	if (conn_time)
	{
		bson_append_utf8(&set,ClientStateRecord::KEY_PROXY_UID, sizeof(PXACT_CLIENT_STATE_KEY_PROXY_UID)-1, proxy_uid.c_str(), proxy_uid.size());
		if (is_connect)
		{
			bson_append_int32(&set, ClientStateRecord::KEY_CONN_EV_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_CONN_EV_TYPE)-1, (int32_t)client_update->conn_ev_type);
			bson_append_date_time(&set,ClientStateRecord::KEY_LAST_CONNECT_TIME,sizeof(PXACT_CLIENT_STATE_KEY_LAST_CONNECT_TIME)-1, (int64_t)client_update->connect_time);
		}
		if (is_disconnect)
		{
			bson_append_int32(&set, ClientStateRecord::KEY_CONN_EV_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_CONN_EV_TYPE)-1, (int32_t)client_update->disconn_ev_type);
			bson_append_date_time(&set,ClientStateRecord::KEY_LAST_DISCONNECT_TIME,sizeof(PXACT_CLIENT_STATE_KEY_LAST_DISCONNECT_TIME)-1, (int64_t)client_update->disconnect_time);
		}
	}
	if (act_time)
	{
		bson_append_int32(&set, ClientStateRecord::KEY_LAST_ACT_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_LAST_ACT_TYPE)-1, (int32_t)client_update->last_act_type);
	}
	bson_append_document_end(&document, &set);

	bson_append_document_begin(&document, "$inc",-1, &increment);
	bson_append_int32(&increment, ClientStateRecord::KEY_VERSION, sizeof(PXACT_CLIENT_STATE_KEY_VERSION)-1, 1);
	bson_append_document_end(&document, &increment);

	char* q_str = PX_BSON_AS_JSON(query, NULL);
	char* d_str = PX_BSON_AS_JSON(&document, NULL);
	TRACE(9,"%s Update: Query=%s, Update=%s\n", __FUNCTION__,
				q_str, d_str);
	bson_free(q_str);
	bson_free(d_str);

	bson_error_t error;
	bool ok = mongoc_collection_update(client_state_coll_, MONGOC_UPDATE_NONE, query, &document, NULL, &error);

	if (!ok)
	{
		rc = ISMRC_Error;
		TRACE(5, "%s Upset error domain=%d code=%d msg=%s; rc=%d\n",
				__FUNCTION__, error.domain, error.code, error.message, rc);
	}
	else
	{
		const bson_t* reply = mongoc_collection_get_last_error(client_state_coll_);
		char* r_str = PX_BSON_AS_JSON(reply,NULL);
		TRACE(9,"%s reply: %s\n", __FUNCTION__, r_str);
		bson_free(r_str);

		bson_iter_t iter;
		if (bson_iter_init_find(&iter, reply, "nMatched"))
		{
			int nMatched = bson_iter_int32(&iter);
			if (nMatched == 0)
			{
				rc = ISMRC_NotFound;
			}
			else if (nMatched >1)
			{
				rc = ISMRC_Error;
				char* r_str = PX_BSON_AS_JSON(reply,NULL);
				TRACE(1, "Error: %s nMatched>1 in reply, reply=%s, state=%s; rc=%d\n",
						__FUNCTION__, r_str,
						client_update->toString().c_str(), rc);
				bson_free(r_str);
			}
			else
			{

				if (bson_iter_init_find(&iter, reply, "nModified"))
				{
					int nModified = bson_iter_int32(&iter);
					if (nModified != 1)
					{
						rc = ISMRC_Error;
						char* r_str = PX_BSON_AS_JSON(reply,NULL);
						TRACE(1, "Error: %s nMatched==1 but nModified!=0 in reply, reply=%s, state=%s; rc=%d\n",
								__FUNCTION__, r_str,
								client_update->toString().c_str(), rc);
						bson_free(r_str);
					}
				}
				else
				{
					rc = ISMRC_Error;
					char* r_str = PX_BSON_AS_JSON(reply,NULL);
					TRACE(1, "Error: %s cannot find nModified in reply, reply=%s, state=%s; rc=%d\n",
							__FUNCTION__, r_str,
							client_update->toString().c_str(), rc);
					bson_free(r_str);
				}
			}
		}
		else
		{
			rc = ISMRC_Error;
			char* r_str = PX_BSON_AS_JSON(reply,NULL);
			TRACE(1, "Error: %s cannot find nMatched in reply, reply=%s, state=%s; rc=%d\n",
					__FUNCTION__, r_str,
					client_update->toString().c_str(), rc);
			bson_free(r_str);
		}
	}

	bson_destroy(&set);
	bson_destroy(&curr_date);
	bson_destroy(&increment);
	bson_destroy(&document);
	bson_destroy(query);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}

int ActivityDBClient::updateBulk(ClientUpdateVector& update_vec, std::size_t numUpdates, const std::string& proxy_uid)
{
	TRACE(9, "%s Entry: size=%ld, numUpdates=%ld\n", __FUNCTION__,
			update_vec.size(), numUpdates);
	int rc = ISMRC_OK;

	std::size_t NUM = std::min(numUpdates,update_vec.size());

	bson_t query = BSON_INITIALIZER;
	bson_t document = BSON_INITIALIZER;
	bson_t curr_date = BSON_INITIALIZER;
	bson_t set = BSON_INITIALIZER;
	bson_t increment = BSON_INITIALIZER;
	bson_append_int32(&increment, ClientStateRecord::KEY_VERSION, -1, 1);
	bson_t opts = BSON_INITIALIZER;
	bson_append_bool(&opts,"upsert",-1,false);

	bson_error_t error_update[NUM];
	memset(error_update,0, NUM*sizeof(bson_error_t));

	mongoc_bulk_operation_t* pBulk = mongoc_collection_create_bulk_operation(client_state_coll_,false,NULL); //unorderd

	for (unsigned i = 0; i < NUM; ++i)
	{
		if ((update_vec[i]->dirty_flags & (ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD) ) >0)
		{
			rc = PXACT_ERROR_t::PXACT_ERROR_DIRTY_FLAG_INVALID;
			TRACE(5, "Error: %s can update only if ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT & ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD is not set, state=%s; rc=%d\n",
					__FUNCTION__, update_vec[i]->toString().c_str(), rc);
			return rc;
		}

		if ((update_vec[i]->dirty_flags == ClientUpdateRecord::DIRTY_FLAG_CLEAN))
		{
			rc = PXACT_ERROR_t::PXACT_ERROR_DIRTY_FLAG_INVALID;;
			TRACE(5, "Error: %s cannot update if ClientUpdateRecord::DIRTY_FLAG_CLEAN, state=%s; rc=%d\n",
					__FUNCTION__, update_vec[i]->toString().c_str(), rc);
			rc = ISMRC_OK;
			continue;
		}

		bson_reinit(&curr_date);
		bson_append_bool(&curr_date,ClientStateRecord::KEY_LAST_ACT_TIME,-1,true);

		bson_reinit(&query);
		bson_reinit(&document);
		bson_reinit(&set);
		bson_append_utf8(&query, ClientStateRecord::KEY_ORG, sizeof(PXACT_CLIENT_STATE_KEY_ORG)-1, update_vec[i]->org->c_str(), update_vec[i]->org->size());
		bson_append_utf8(&query, ClientStateRecord::KEY_CLIENT_ID, sizeof(PXACT_CLIENT_STATE_KEY_CLIENT_ID)-1,
				update_vec[i]->client_id->c_str(), update_vec[i]->client_id->size());
		bool act_time = (update_vec[i]->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT) > 0;
		bool is_pub = ClientStateRecord::isPubType(static_cast<PXACT_ACTIVITY_TYPE_t>(update_vec[i]->last_act_type));
		bool meta = (update_vec[i]->dirty_flags & ClientUpdateRecord::DIRTY_FLAG_METADATA) > 0;
		if (is_pub) {
			bson_append_bool(&curr_date,ClientStateRecord::KEY_LAST_PUB_TIME, sizeof(PXACT_CLIENT_STATE_KEY_LAST_PUB_TIME)-1, true);
		}
		if (act_time)
		{
			bson_append_document(&document, "$currentDate",-1, &curr_date);
		}

		bson_append_document_begin(&document, "$set",-1, &set);
		if (meta)
		{
			bson_append_int32(&set, ClientStateRecord::KEY_CLIENT_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_CLIENT_TYPE)-1, (int32_t)update_vec[i]->client_type);
			bson_append_int32(&set, ClientStateRecord::KEY_PROTOCOL, sizeof(PXACT_CLIENT_STATE_KEY_PROTOCOL)-1, (int32_t)update_vec[i]->protocol);
			bson_append_int32(&set, ClientStateRecord::KEY_CLEAN_S, sizeof(PXACT_CLIENT_STATE_KEY_CLEAN_S)-1, (int32_t)update_vec[i]->clean_s);
			bson_append_int32(&set, ClientStateRecord::KEY_REASON_CODE, sizeof(PXACT_CLIENT_STATE_KEY_CLEAN_S)-1, (int32_t)update_vec[i]->reason_code);
			bson_append_int32(&set, ClientStateRecord::KEY_EXPIRY_SEC, sizeof(PXACT_CLIENT_STATE_KEY_EXPIRY_SEC)-1, (int32_t)update_vec[i]->expiry_sec);
			bson_append_utf8(&set, ClientStateRecord::KEY_GATEWAY_ID, sizeof(PXACT_CLIENT_STATE_KEY_GATEWAY_ID)-1, update_vec[i]->gateway_id->c_str(), update_vec[i]->gateway_id->size());
			bson_append_int32(&set, ClientStateRecord::KEY_TARGET_SERVER, sizeof(PXACT_CLIENT_STATE_KEY_TARGET_SERVER)-1, (int32_t)update_vec[i]->target_server);
		}
		if (act_time)
		{
			bson_append_int32(&set, ClientStateRecord::KEY_LAST_ACT_TYPE, sizeof(PXACT_CLIENT_STATE_KEY_LAST_ACT_TYPE)-1, update_vec[i]->last_act_type);
		}
		bson_append_document_end(&document, &set);

		bson_append_document(&document, "$inc",-1, &increment);

		if ((TRACE_LEVEL) >= 9)
		{
			char* q_str = PX_BSON_AS_JSON(&query, NULL);
			char* d_str = PX_BSON_AS_JSON(&document, NULL);
			TRACE(9,"%s Bulk Update: i=%d Query=%s, Update=%s\n", __FUNCTION__, i, q_str, d_str);
			bson_free(q_str);
			bson_free(d_str);
		}

		mongoc_bulk_operation_update_one(pBulk,&query,&document,false);
	}

	bson_t reply;
	bson_error_t error;
	uint32_t ret = mongoc_bulk_operation_execute(pBulk,&reply,&error);
	if (ret == 0)
	{
		char* reply_str = PX_BSON_AS_JSON(&reply,NULL);
		TRACE(1, "Error: %s errors in bulk operation: domain=%d, code=%d, message=%s; Reply=%s\n",
				__FUNCTION__, error.domain, error.code, error.message, reply_str);
		bson_free(reply_str);
		rc = ISMRC_Error;
	}
	else
	{
		if ((TRACE_LEVEL) >= 9)
		{
			char* reply_str = PX_BSON_AS_JSON(&reply,NULL);
			TRACE(9, "%s Bulk update reply=%s\n",
					__FUNCTION__, reply_str);
			bson_free(reply_str);
		}

		unsigned nMatched = 0;
		unsigned nModified = 0;
		bson_iter_t iter;
		if (bson_iter_init_find(&iter, &reply, "nMatched"))
		{
			nMatched = bson_iter_int32(&iter);
		}
		if (bson_iter_init_find(&iter, &reply, "nModified"))
		{
			nModified = bson_iter_int32(&iter);
		}

		if (nMatched != NUM)
		{
		    TRACE(5, "%s some updates were not matched/modified: nMatched=%d, nModifine=%d, NUM-ops=%ld; %s\n",
		            __FUNCTION__, nMatched, nModified, NUM,
		            "This can happen if an entry in the cache was expired from the DB.");
		    rc = readInsert_UnmatchedBulkUpdate(update_vec, numUpdates, proxy_uid);
		}
	}

	TRACE(9,"%s Bulk Update: NUM-ops=%ld\n", __FUNCTION__, NUM);

	mongoc_bulk_operation_destroy(pBulk);
	bson_destroy(&reply);
	bson_destroy(&query);
	bson_destroy(&document);
	bson_destroy(&curr_date);
	bson_destroy(&set);
	bson_destroy(&increment);

	TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
	return rc;
}

int ActivityDBClient::findAllAssociatedToProxy(const std::string proxy_uid, int selectConnectivity, int limit, ClientStateRecord* client_state, int* list_size)
{
    using namespace std;

    TRACE(9, "%s Entry: proxy_uid=%s, selectConnectivity=%d, limit=%d\n",
            __FUNCTION__, proxy_uid.c_str(), selectConnectivity, limit);

    if (client_state == NULL)
    {
        return ISMRC_NullArgument;
    }

    int rc = ISMRC_OK;

    //example
    //db.client_state.find({$and: [{proxy_uid : "ProxyTest1"},  {conn_ev_type : {$gt: 100}}, {conn_ev_type: {$lt: 200}}] } ).limit(3)
    //db.client_state.find({proxy_uid : "ProxyTest1"}).limit(3)
    bson_t* query = NULL;
    bson_error_t error;
    switch (selectConnectivity)
    {
    case 0:
        query = BCON_NEW(ClientStateRecord::KEY_PROXY_UID, BCON_UTF8(proxy_uid.c_str()));
        break;

    case 1:
        query = BCON_NEW( "$and", "[",
                "{", ClientStateRecord::KEY_PROXY_UID, BCON_UTF8(proxy_uid.c_str()),"}",
                "{", ClientStateRecord::KEY_CONN_EV_TYPE, "{", "$gt", BCON_INT32(PXACT_ACTIVITY_TYPE_CONN), "}", "}",
                "{", ClientStateRecord::KEY_CONN_EV_TYPE, "{", "$lt", BCON_INT32(PXACT_ACTIVITY_TYPE_CONN_END), "}", "}",
                "]");
        break;

    case 2:
        query = BCON_NEW( "$and", "[",
                "{", ClientStateRecord::KEY_PROXY_UID, BCON_UTF8(proxy_uid.c_str()),"}",
                "{", ClientStateRecord::KEY_CONN_EV_TYPE, "{", "$gt", BCON_INT32(PXACT_ACTIVITY_TYPE_DISCONN), "}", "}",
                "{", ClientStateRecord::KEY_CONN_EV_TYPE, "{", "$lt", BCON_INT32(PXACT_ACTIVITY_TYPE_DISCONN_END), "}", "}",
                "]");
        break;

    default:
        TRACE(1, "Error: %s Illegal argument: selectConnectivity=%d; rc=%d\n", __FUNCTION__, selectConnectivity, ISMRC_Error);
        return ISMRC_Error;
    }

    if (query == NULL)
    {
        TRACE(1, "Error: %s Query creation error; rc=%d\n", __FUNCTION__, ISMRC_Error);
        return ISMRC_Error;
    }

    mongoc_cursor_t *cursor = NULL;

#if MONGOC_CHECK_VERSION(1,5,0)
    bson_t opts = BSON_INITIALIZER;
    bson_append_int32(&opts, "limit", -1, limit);
    cursor = mongoc_collection_find_with_opts(client_state_coll_, query, &opts, NULL);
#else
    cursor = mongoc_collection_find(client_state_coll_, MONGOC_QUERY_NONE, 0, limit, 0, query, NULL, NULL);
#endif

    ClientStateRecord* current = NULL;
    int num_found = 0;
    const bson_t *doc;
    while (mongoc_cursor_next (cursor, &doc))
    {
        num_found++;
        if (current == NULL)
        {
            current = client_state;
        }
        else
        {
            current->next = new ClientStateRecord;
            current = current->next;
        }

        {
            bson_iter_t iter;
            if (bson_iter_init (&iter, doc))
            {
                while (bson_iter_next (&iter))
                {
                    const char* key = bson_iter_key(&iter);

                    if (strcmp(key, ClientStateRecord::KEY_CLIENT_ID) == 0)
                    {
                        current->client_id = bson_iter_utf8(&iter, NULL);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_ORG) == 0)
                    {
                        current->org = bson_iter_utf8(&iter, NULL);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_DEVICE_TYPE) == 0)
                    {
                        current->device_type = bson_iter_utf8(&iter, NULL);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_DEVICE_ID) == 0)
                    {
                        current->device_id = bson_iter_utf8(&iter, NULL);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_CLIENT_TYPE) == 0)
                    {
                        current->client_type = static_cast<PXACT_CLIENT_TYPE_t>(bson_iter_int32(&iter));
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_CONN_EV_TIME) == 0)
                    {
                        current->conn_ev_time = bson_iter_date_time(&iter);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_CONN_EV_TYPE) == 0)
                    {
                        current->conn_ev_type = static_cast<PXACT_ACTIVITY_TYPE_t>(bson_iter_int32(&iter));
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_LAST_CONNECT_TIME) == 0)
                    {
                        client_state->last_connect_time = bson_iter_date_time(&iter);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_LAST_DISCONNECT_TIME) == 0)
                    {
                        client_state->last_disconnect_time = bson_iter_date_time(&iter);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_LAST_ACT_TIME) == 0)
                    {
                        current->last_act_time = bson_iter_date_time(&iter);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_LAST_ACT_TYPE) == 0)
                    {
                        current->last_act_type = static_cast<PXACT_ACTIVITY_TYPE_t>(bson_iter_int32(&iter));
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_PROTOCOL) == 0)
                    {
                        current->protocol = static_cast<PXACT_CONN_PROTOCOL_TYPE_t>(bson_iter_int32(&iter));
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_CLEAN_S) == 0)
                    {
                        current->clean_s = bson_iter_int32(&iter);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_EXPIRY_SEC) == 0)
                    {
                        current->expiry_sec = bson_iter_int32(&iter);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_REASON_CODE) == 0)
                    {
                        current->reason_code = bson_iter_int32(&iter);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_PROXY_UID) == 0)
                    {
                        current->proxy_uid = bson_iter_utf8(&iter, NULL);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_GATEWAY_ID) == 0)
                    {
                        current->gateway_id = bson_iter_utf8(&iter, NULL);
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_TARGET_SERVER) == 0)
                    {
                        current->target_server = static_cast<PXACT_TRG_SERVER_TYPE_t>(bson_iter_int32(&iter));
                    }
                    else if (strcmp(key, ClientStateRecord::KEY_VERSION) == 0)
                    {
                        current->version = bson_iter_int64(&iter);
                    }
                }
            }
            else
            {
                rc = ISMRC_Error;
            }

        }
    }//while

    if (num_found == 0)
    {
        if (mongoc_cursor_error (cursor, &error))
        {
            rc = ISMRC_Error;
            TRACE(5, "%s Read error, proxy_uid=%s; domain=%d code=%d msg=%s; rc=%d\n",
                    __FUNCTION__, proxy_uid.c_str(), error.domain, error.code, error.message, rc);
        }
        else
        {
            rc = ISMRC_NotFound;
        }
    }

    if (list_size)
    {
        *list_size = num_found;
    }

    mongoc_cursor_destroy(cursor);
    bson_destroy(query);
#if MONGOC_CHECK_VERSION(1,5,0)
	bson_destroy (&opts);
#endif

    TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
    return rc;
}


int ActivityDBClient::disassociatedFromProxy(const std::string proxy_uid, int selectConnectivity, int limit, int* num_updated)
{
    using namespace std;

    TRACE(9, "%s Entry: proxy_uid=%s, selectConnectivity=%d, limit=%d\n",
            __FUNCTION__, proxy_uid.c_str(), selectConnectivity, limit);

    ClientStateRecord_UPtr record(new ClientStateRecord);
    int list_size = 0;
    int rc = findAllAssociatedToProxy(proxy_uid,selectConnectivity, limit, record.get(), &list_size);

    if (rc == ISMRC_OK && list_size > 0)
    {
        bson_t query = BSON_INITIALIZER;
        bson_t document = BSON_INITIALIZER;
        bson_t curr_date = BSON_INITIALIZER;
        bson_t set = BSON_INITIALIZER;
        bson_t increment = BSON_INITIALIZER;
        bson_append_int32(&increment, ClientStateRecord::KEY_VERSION, -1, 1);
        bson_t opts = BSON_INITIALIZER;
        bson_append_bool(&opts,"upsert",-1,false);

        bson_error_t error_update[list_size];
        memset(error_update,0, list_size*sizeof(bson_error_t));

        mongoc_bulk_operation_t* pBulk = mongoc_collection_create_bulk_operation(client_state_coll_,false,NULL); //unorderd

        ClientStateRecord* curr = record.get();
        for (int i = 0; i < list_size; ++i)
        {
            bson_reinit(&query);
            bson_reinit(&document);
            bson_reinit(&curr_date);
            bson_reinit(&set);
            bool sel_conn_0 = (selectConnectivity ==0 && ClientStateRecord::isConnectType(curr->conn_ev_type));

            //conditional on version
            bson_append_utf8(&query, ClientStateRecord::KEY_ORG, -1,
                                curr->org.c_str(), curr->org.size());
            bson_append_utf8(&query, ClientStateRecord::KEY_CLIENT_ID, -1,
                    curr->client_id.c_str(), curr->client_id.size());
            bson_append_int64(&query, ClientStateRecord::KEY_VERSION, -1, curr->version);
            if (selectConnectivity == 1 || sel_conn_0)
            {
                bson_append_bool(&curr_date,ClientStateRecord::KEY_CONN_EV_TIME,-1,true);
                bson_append_bool(&curr_date,ClientStateRecord::KEY_LAST_DISCONNECT_TIME,-1,true);
                bson_append_document(&document, "$currentDate",-1, &curr_date);
            }

            bson_append_document_begin(&document, "$set",-1, &set);
            bson_append_utf8(&set, ClientStateRecord::KEY_PROXY_UID, -1, PROXY_UID_NULL, -1);
            if (selectConnectivity == 1 || sel_conn_0)
            {
                bson_append_int32(&set, ClientStateRecord::KEY_CONN_EV_TYPE, -1, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_NETWORK);
            }
            bson_append_document_end(&document, &set);

            bson_append_document(&document, "$inc",-1, &increment);

            if ((TRACE_LEVEL) >= 9)
            {
                char* q_str = PX_BSON_AS_JSON(&query, NULL);
                char* d_str = PX_BSON_AS_JSON(&document, NULL);
                TRACE(9,"%s Bulk Update: i=%d Query=%s, Update=%s\n", __FUNCTION__, i, q_str, d_str);
                bson_free(q_str);
                bson_free(d_str);
            }
            mongoc_bulk_operation_update_one(pBulk,&query,&document,false);
            curr = curr->next;
        }

        bson_t reply;
        bson_error_t error;
        uint32_t ret = mongoc_bulk_operation_execute(pBulk,&reply,&error);
        if (ret == 0)
        {
            char* reply_str = PX_BSON_AS_JSON(&reply,NULL);
            TRACE(1, "Error: %s errors in bulk operation: domain=%d, code=%d, message=%s; Reply=%s\n",
                    __FUNCTION__, error.domain, error.code, error.message, reply_str);
            bson_free(reply_str);
            rc = ISMRC_Error;
        }
        else
        {
            if ((TRACE_LEVEL) >= 9)
            {
                char* reply_str = PX_BSON_AS_JSON(&reply,NULL);
                TRACE(9, "%s Bulk update reply=%s\n",
                        __FUNCTION__, reply_str);
                bson_free(reply_str);
            }

            int nMatched = -1;
            int nModified = -1;
            bson_iter_t iter;
            if (bson_iter_init_find(&iter, &reply, "nMatched"))
            {
                nMatched = bson_iter_int32(&iter);
            }
            if (bson_iter_init_find(&iter, &reply, "nModified"))
            {
                nModified = bson_iter_int32(&iter);
            }

            if (nMatched != list_size || nModified != list_size)
            {
                TRACE(5, "%s some updates were not matched/modified: nMatched=%d, nModified=%d, #ops=%d; %s\n",
                        __FUNCTION__, nMatched, nModified, list_size,
                        "This can happen if an entry changed mean between read & write; ignored.");
            }
            else
            {
                TRACE(9,"%s Bulk Update: #ops=%d\n", __FUNCTION__, list_size);
            }

            if (num_updated)
            {
                *num_updated = nModified;
            }
        }

        mongoc_bulk_operation_destroy(pBulk);
        bson_destroy(&reply);
        bson_destroy(&query);
        bson_destroy(&document);
        bson_destroy(&curr_date);
        bson_destroy(&set);
        bson_destroy(&increment);
    }
    else
    {
        if (num_updated)
        {
            *num_updated = 0;
        }
        TRACE(9,"%s No records found, rc=%d\n", __FUNCTION__, rc);
    }

    TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
    return rc;
//
//    //example
//    // q = {$and: [{proxy_uid : "ProxyTest1"},  {conn_ev_type : {$gt: 100}}, {conn_ev_type: {$lt: 200}}] }
//    // u = {$set: {conn_ev_type: 203, proxy_uid: "?"}, $currentDate: {conn_ev_time : true}}
//    //db.client_state.updateMany( q,  ).limit(3)
//    //db.client_state.find({proxy_uid : "ProxyTest1"}).limit(3)
//    bson_t* query = NULL;
//    bson_t* update = NULL;
//
//    if (onlyConnected)
//    {
//        query = BCON_NEW( "$and", "[",
//                "{", ClientStateRecord::KEY_PROXY_UID, BCON_UTF8(proxy_uid.c_str()),"}",
//                "{", ClientStateRecord::KEY_CONN_EV_TYPE, "{", "$gt", BCON_INT32(PXACT_ACTIVITY_TYPE_CONN), "}", "}",
//                "{", ClientStateRecord::KEY_CONN_EV_TYPE, "{", "$lt", BCON_INT32(PXACT_ACTIVITY_TYPE_CONN_END), "}", "}",
//                "]");
//        update = BCON_NEW(
//                "$set", "{",
//                ClientStateRecord::KEY_PROXY_UID, BCON_UTF8(PROXY_UID_NULL),
//                ClientStateRecord::KEY_CONN_EV_TYPE, BCON_INT32(PXACT_ACTIVITY_TYPE_MQTT_DISCONN_NETWORK), "}",
//                "$currentDate", "{", ClientStateRecord::KEY_CONN_EV_TIME, BCON_BOOL(true), "}");
//    }
//    else
//    {
//        query = BCON_NEW( "$and", "[",
//                "{", ClientStateRecord::KEY_PROXY_UID, BCON_UTF8(proxy_uid.c_str()),"}",
//                "{", ClientStateRecord::KEY_CONN_EV_TYPE, "{", "$gt", BCON_INT32(PXACT_ACTIVITY_TYPE_DISCONN), "}", "}",
//                "{", ClientStateRecord::KEY_CONN_EV_TYPE, "{", "$lt", BCON_INT32(PXACT_ACTIVITY_TYPE_DISCONN_END), "}", "}",
//                "]");
//        update = BCON_NEW(
//                "$set", "{",
//                ClientStateRecord::KEY_PROXY_UID, BCON_UTF8(PROXY_UID_NULL), "}");
//    }
//
//    if (query == NULL)
//    {
//        TRACE(1, "Error: %s Query creation error; rc=%d\n", __FUNCTION__, ISMRC_Error);
//        return ISMRC_Error;
//    }
//
//    if (update == NULL)
//    {
//        TRACE(1, "Error: %s Update creation error; rc=%d\n", __FUNCTION__, ISMRC_Error);
//        return ISMRC_Error;
//    }
//
//    //TODO
//    bson_error_t error;
//    bool ok = mongoc_collection_update(client_state_coll_, MONGOC_UPDATE_MULTI_UPDATE, query, update, NULL, &error);
//
//    if (!ok)
//    {
//        rc = ISMRC_Error;
//        TRACE(5, "%s Upset error domain=%d code=%d msg=%s; rc=%d\n",
//                __FUNCTION__, error.domain, error.code, error.message, rc);
//    }
//    else
//    {
//        const bson_t* reply = mongoc_collection_get_last_error(client_state_coll_);
//        TRACE(9,"%s reply: %s\n", __FUNCTION__, PX_BSON_AS_JSON(reply,NULL));
//
//        int nMatched = -1;
//        int nModified = -1;
//        bson_iter_t iter;
//        if (bson_iter_init_find(&iter, reply, "nMatched"))
//        {
//            nMatched = bson_iter_int32(&iter);
//
//            if (bson_iter_init_find(&iter, reply, "nModified"))
//                    {
//                        nModified = bson_iter_int32(&iter);
//                    }
//                    else
//                    {
//                        rc = ISMRC_Error;
//                        TRACE(1, "Error: %s cannot find nModified in reply, reply=%s, proxy_uid=%s; rc=%d\n",
//                                __FUNCTION__, PX_BSON_AS_JSON(reply,NULL), proxy_uid.c_str(), rc);
//                    }
//        }
//        else
//        {
//            rc = ISMRC_Error;
//            TRACE(1, "Error: %s cannot find nMatched in reply, reply=%s, proxy_uid=%s; rc=%d\n",
//                    __FUNCTION__, PX_BSON_AS_JSON(reply,NULL), proxy_uid.c_str(), rc);
//        }
//
//        if (rc == ISMRC_OK && num_updated)
//        {
//            *num_updated = nModified;
//        }
//    }
//
//    bson_destroy(query);
//    bson_destroy(update);


}

int ActivityDBClient::readInsert_UnmatchedBulkUpdate(ClientUpdateVector& update_vec, std::size_t numUpdates, const std::string& proxy_uid)
{
    TRACE(9, "%s Entry: size=%ld, numUpdates=%ld\n", __FUNCTION__,
            update_vec.size(), numUpdates);
    int rc = ISMRC_OK;

    std::size_t NUM = std::min(numUpdates,update_vec.size());
    int num_inserted = 0;

    for (unsigned i = 0; i < NUM; ++i)
    {
        ClientStateRecord client_state;
        client_state.client_id = *update_vec[i]->client_id;
        client_state.org = *update_vec[i]->org;
        rc = read(&client_state);
        if (rc == ISMRC_NotFound)
        {
            update_vec[i]->dirty_flags |= ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;
            rc = insert(update_vec[i].get(), proxy_uid);
            if (rc == ISMRC_OK)
            {
                num_inserted++;
                TRACE(7, "%s Inserted record=%s \n", __FUNCTION__, update_vec[i]->toString().c_str());
            }
            else
            {
                TRACE(5, "%s NotFound, but insert failed, record=%s \n", __FUNCTION__, update_vec[i]->toString().c_str());
            }
        }
    }

    TRACE(7, "%s Inserted %d records \n", __FUNCTION__, num_inserted);
    TRACE(9, "%s Exit: rc=%d\n", __FUNCTION__, rc);
    return rc;
}

} /* namespace ism_proxy */
