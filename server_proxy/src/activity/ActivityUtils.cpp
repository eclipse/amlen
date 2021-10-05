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
 * ActivityUtils.cpp
 *  Created on: Oct 29, 2017
 */

#include "ActivityUtils.h"
#include "mongoc.h"
#include "bson.h"

#define SoN(x) ((x)?(x):"nil")

namespace ism_proxy
{

//=== UID ===================================================================


std::string generateProxyUID(const int k)
{
	std::string uid;
	unsigned char buf[k];

    RAND_bytes(buf, k);

	for (int i=0; i<k; ++i)
	{
		char c = base64url[static_cast<int>(buf[i]) & 0x3F];
		uid.push_back(c);
	}

	TRACE(3, "%s Generate new ProxyUID=%s\n", __FUNCTION__, uid.c_str());

	return uid;
}

//=== Trim strings ==========================================================

std::string& trim_right_inplace(std::string& s, const std::string& delimiters)
{
    return s.erase(s.find_last_not_of(delimiters) + 1);
}

std::string& trim_left_inplace(std::string& s, const std::string& delimiters)
{
    return s.erase(0, s.find_first_not_of(delimiters));
}

std::string& trim_inplace(std::string& s, const std::string& delimiters)
{
    return trim_left_inplace(trim_right_inplace(s, delimiters), delimiters);
}

std::string randomSelect(const StringSet& sset)
{
    using namespace std;
    string target_proxy;
    if (!sset.empty())
    {
        int p = std::rand() % sset.size();
        for (StringSet::const_iterator it = sset.begin(); it != sset.end(); ++it, --p)
        {
            if (p==0)
            {
                target_proxy = *it;
                break;
            }
        }
    }
    return target_proxy;
}

//=== Config ==================================================================

static constexpr char const* PREFIX = "mongodb://";
static constexpr unsigned PREFIX_SZ = 10;
static constexpr char const* USER_PWD_SEP = ":";
static constexpr char const* CRED_SEP = "@";
static constexpr char const* AUTH_SEP = "/?";


ActivityDBConfig::ActivityDBConfig() :
        useTLS(0),
		mongoWconcern(0),
		mongoWjournal(0),
		mongoRconcern(0),
		mongoRpref(0),
       	numWorkerThreads(NUM_WORKER_THREADS_DEF),
		numBanksPerWorkerThread(NUM_BANKS_PER_WORKER_DEF),
		bulkWriteSize(BULK_SIZE_DEF),
		hbTimeoutMS(0),
		hbIntervalMS(0),
		dbIOPSRateLimit(0),
		dbRateLimitGlobalEnabled(0),
		memoryLimitPercentOfTotal(0),
		terminationTimeoutMS(TERM_TO_MS_DEF),
		removedProxyCleanupIntervalSec(REM_PROXY_CLEANUP_TO_SEC_DEF),
		monitoringPolicyDeviceClientClass(0),
		monitoringPolicyGatewayClientClass(0),
		monitoringPolicyConnectorClientClass(0),
		monitoringPolicyAppClientClass(0),
		monitoringPolicyAppScaleClientClass(0)
{
}

ActivityDBConfig::~ActivityDBConfig()
{
}


std::string ActivityDBConfig::build_uri(const std::string& uri, const std::string& user, const std::string& pwd)
{
	using namespace std;

	mongoc_uri_t* pUri = mongoc_uri_new(uri.c_str()); //ensure it is a legal URI
	if (!pUri)
	{
		return "";
	}

	if (user.empty())
	{
		mongoc_uri_destroy(pUri);
		return uri;
	}
	else
	{
		ostringstream oss;
		oss << string(PREFIX) << user << USER_PWD_SEP << pwd << CRED_SEP;

		const char* old_user = mongoc_uri_get_username(pUri);
		if (old_user)
		{
			//search @ in scheme://authority part
			size_t auth_end = uri.find_first_of(AUTH_SEP,PREFIX_SZ);
			string pref_authority = uri.substr(0,auth_end);
			size_t cred_end = pref_authority.find_first_of(CRED_SEP);

			if (cred_end != string::npos)
			{
				oss << uri.substr(cred_end+1);
			}
			else //can't happen
			{
				mongoc_uri_destroy(pUri);
				return "";
			}
		}
		else //no user:pwd in uri
		{
			oss << uri.substr(PREFIX_SZ);
		}
		mongoc_uri_destroy(pUri);
		return oss.str();
	}
}

std::string ActivityDBConfig::assemble_uri(const std::string& user, const std::string& password,
            const std::string&  endpoints, const std::string&  authDB, int useTLS, const std::string& ca_file, const std::string& options)
{
    using namespace std;
    ostringstream oss;
    oss << string(PREFIX);
    if (!user.empty())
    {
        oss << user << USER_PWD_SEP << password << CRED_SEP;
    }
    bson_t* doc = bson_new_from_json(reinterpret_cast<const uint8_t*>(endpoints.c_str()), -1, NULL);
    if (doc == NULL)
    {
        TRACE(1,"Error: %s, cannot create bson_new_from_json, endpoints: %s \n", __FUNCTION__, endpoints.c_str());
        return "";
    }
    bson_iter_t iter;
    if (bson_iter_init(&iter, doc))
    {
        bool first = true;
        while (bson_iter_next(&iter))
        {
            const char* ep = bson_iter_utf8(&iter, NULL);
            int host_len = hostLength(ep);
            if (host_len > BSON_HOST_NAME_MAX)
            {
                TRACE(1,"Error: %s, host name too long, must be <= BSON_HOST_NAME_MAX = %d, length=%d, EP: %s\n",
                        __FUNCTION__, BSON_HOST_NAME_MAX, host_len, ep);
                return "";
            }
            else if (host_len < 0)
            {
                TRACE(1,"Error: %s, computing host name length, EP: %s\n",
                        __FUNCTION__, ep);
                return "";
            }

            oss << (first ? "" : ",") << ep;
            first = false;
        }
        oss << "/" << authDB;
        if (useTLS || !options.empty())
        {
            oss << "?";
            if (useTLS)
            {
                oss << "ssl=true";

                if (!ca_file.empty())
                    oss << "&sslCertificateAuthorityFile=" << ca_file;
            }

            if (!options.empty())
            {
                oss << (useTLS ? "&" : "") << options;
            }
        }
        return oss.str();
    }
    else
    {
        return "";
    }
}

int ActivityDBConfig::hostLength(const char* ep)
{
    using namespace std;
    if (ep)
    {
       string ep_str(ep);
       size_t p = ep_str.find_last_of("]");
       if (p == string::npos) //IPv4 IP or domain name
       {
           size_t q = ep_str.find_last_of(":");
           if (q != string::npos)
               return q;
           else
               return ep_str.size();
       }
       else //IPv6 IP
       {
           return p+1;
       }
    }
    else
        return -1;

}



std::string ActivityDBConfig::mask_pwd_uri(const std::string& uri)
{
	using namespace std;

	if (uri.empty())
	    return "";

	mongoc_uri_t* pUri = mongoc_uri_new(uri.c_str());
	if (!pUri)
	{
		return "";
	}

	const char* user = mongoc_uri_get_username(pUri);
	if (user)
	{
		//search @ in scheme://authority part
		size_t auth_end = uri.find_first_of(AUTH_SEP,PREFIX_SZ);
		string auth = uri.substr(0,auth_end);
		size_t cred_end = auth.find_first_of(CRED_SEP);
		if (cred_end != auth.find_last_of(CRED_SEP))
		{
			mongoc_uri_destroy(pUri);
			return "";
		}

		if (cred_end != string::npos)
		{
			string cred = auth.substr(PREFIX_SZ,cred_end-PREFIX_SZ);

			if (cred.find_first_of(USER_PWD_SEP) != cred.find_last_of(USER_PWD_SEP))
			{
				mongoc_uri_destroy(pUri);
				return "";
			}

			size_t pwd_start = auth.find_first_of(USER_PWD_SEP,PREFIX_SZ);
			ostringstream oss;
			oss << uri.substr(0,pwd_start+1) << "********" << uri.substr(cred_end, string::npos);
			mongoc_uri_destroy(pUri);
			return oss.str();
		}
		else
		{
			mongoc_uri_destroy(pUri);
			return uri;
		}
	}
	else
	{
		mongoc_uri_destroy(pUri);
		return uri;
	}
}


/**
 * Analyze the changes between current and update.
 *
 * @param update the updated config
 * @return first: is different? second: 1: need db-client restart; 0: does not need db-client restart; (-1) static only.
 */
std::pair<bool,int> ActivityDBConfig::compare(const ActivityDBConfig& u) const
{
    std::pair<bool,bool> ret(false,0);

    if (dbType != u.dbType ||
        mongoURI != u.mongoURI ||
        mongoEndpoints != u.mongoEndpoints ||
        mongoAuthDB != u.mongoAuthDB ||
        useTLS != u.useTLS ||
        mongoCAFile != u.mongoCAFile ||
        mongoOptions != u.mongoOptions ||
        mongoDBName != u.mongoDBName ||
        mongoClientStateColl != u.mongoClientStateColl ||
        mongoProxyStateColl != u.mongoProxyStateColl ||
        mongoUser != u.mongoUser ||
        mongoPassword != u.mongoPassword ||
        mongoWconcern != u.mongoWconcern ||
        mongoWjournal != u.mongoWjournal ||
        mongoRconcern != u.mongoRconcern ||
        mongoRpref != u.mongoRpref)
    {
        ret.first = true;
        ret.second = 1;
        return ret;
    }

    if (dbIOPSRateLimit!= u.dbIOPSRateLimit ||
        dbRateLimitGlobalEnabled!= u.dbRateLimitGlobalEnabled ||
        memoryLimitPercentOfTotal!= u.memoryLimitPercentOfTotal ||
        monitoringPolicyDeviceClientClass!= u.monitoringPolicyDeviceClientClass ||
        monitoringPolicyGatewayClientClass!= u.monitoringPolicyGatewayClientClass ||
        monitoringPolicyConnectorClientClass!= u.monitoringPolicyConnectorClientClass ||
        monitoringPolicyAppClientClass!= u.monitoringPolicyAppClientClass ||
        monitoringPolicyAppScaleClientClass!= u.monitoringPolicyAppScaleClientClass)
    {
        ret.first = true;
        ret.second = 0;
        return ret;
    }


    if (numWorkerThreads != u.numWorkerThreads ||
        numBanksPerWorkerThread != u.numBanksPerWorkerThread ||
        bulkWriteSize!= u.bulkWriteSize ||
        hbTimeoutMS!= u.hbTimeoutMS ||
        hbIntervalMS!= u.hbIntervalMS ||
        terminationTimeoutMS != u.terminationTimeoutMS ||
        removedProxyCleanupIntervalSec != u.removedProxyCleanupIntervalSec)
    {
        ret.first = true;
        ret.second = -1;
        return ret;
    }

    return ret;
}

std::string ActivityDBConfig::toString() const
{
	std::ostringstream oss;
		oss << "Type=" << dbType
				<< " URI=" << mask_pwd_uri(mongoURI)
				<< " EP=" << mongoEndpoints
				<< " AuthDB=" << mongoAuthDB
				<< " UseTLS=" << (useTLS ? "T" : "F")
				<< " CAFile=" << mongoCAFile
				<< " Options=" << mongoOptions
				<< " DBName=" << mongoDBName
				<< " Coll-client=" << mongoClientStateColl
				<< " Coll-proxy=" << mongoProxyStateColl
				<< " User=" << mongoUser
				<< " PWD=" << (mongoPassword.empty() ? "" : "********")
				<< " W_concern=" << mongoWconcern
				<< " W_journal=" << mongoWjournal
				<< " R_concern=" << mongoRconcern
				<< " R_pref=" << mongoRpref
				<< " NumWorkerThreads=" << numWorkerThreads
				<< " numBanksPerWorkerThread=" << numBanksPerWorkerThread
				<< " BulkWriteSize=" << bulkWriteSize
				<< " hbTimeoutMS=" << hbTimeoutMS
				<< " hbIntervalMS=" << hbIntervalMS
				<< " dbIOPSRateLimit=" << dbIOPSRateLimit
				<< " dbRateLimitGlobalEnabled=" << dbRateLimitGlobalEnabled
				<< " memoryLimitPercentOfTotal=" << memoryLimitPercentOfTotal
				<< " terminationTimeoutMS=" << terminationTimeoutMS
				<< " removedProxyCleanupIntervalSec=" << removedProxyCleanupIntervalSec
		        << " monitoringPolicyDeviceClientClass=" << monitoringPolicyDeviceClientClass
		        << " monitoringPolicyGatewayClientClass=" << monitoringPolicyGatewayClientClass
		        << " monitoringPolicyConnectorClientClass=" << monitoringPolicyConnectorClientClass
		        << " monitoringPolicyAppClientClass=" << monitoringPolicyAppClientClass
		        << " monitoringPolicyAppScaleClientClass=" << monitoringPolicyAppScaleClientClass;

	return oss.str();
}

ActivityDBConfig::ActivityDBConfig(const ismPXActivity_ConfigActivityDB_t* config) :
        dbType(config->pDBType ? config->pDBType : ""),
        //mongoURI(config->pMongoURI ? config->pMongoURI : ""),
        mongoEndpoints(config->pMongoEndpoints ? config->pMongoEndpoints : ""),
        mongoAuthDB(config->pMongoAuthDB ? config->pMongoAuthDB : ""),
        useTLS(config->useTLS),
        mongoCAFile(config->pTrustStore ? config->pTrustStore : ""),
        mongoOptions(config->pMongoOptions ? config->pMongoOptions : ""),
        mongoDBName(config->pMongoDBName ? config->pMongoDBName : ""),
        mongoClientStateColl(config->pMongoClientStateColl ? config->pMongoClientStateColl : ""),
        mongoProxyStateColl(config->pMongoProxyStateColl ? config->pMongoProxyStateColl : ""),
        mongoUser(config->pMongoUser ? config->pMongoUser : ""),
        mongoPassword(config->pMongoPassword ? config->pMongoPassword : ""),
		mongoWconcern(config->mongoWconcern),
		mongoWjournal(config->mongoWjournal),
		mongoRconcern(config->mongoRconcern),
		mongoRpref(config->mongoRpref),
       	numWorkerThreads(config->numWorkerThreads > 0 ? config->numWorkerThreads : NUM_WORKER_THREADS_DEF),
		numBanksPerWorkerThread(config->numBanksPerWorkerThread>0 ? config->numBanksPerWorkerThread : NUM_BANKS_PER_WORKER_DEF),
		bulkWriteSize(config->bulkWriteSize>0 ? config->bulkWriteSize : BULK_SIZE_DEF),
		hbTimeoutMS(config->hbTimeoutMS),
		hbIntervalMS(config->hbIntervalMS),
		dbIOPSRateLimit(config->dbIOPSRateLimit),
		dbRateLimitGlobalEnabled(config->dbRateLimitGlobalEnabled),
		memoryLimitPercentOfTotal(config->memoryLimitPercentOfTotal),
		terminationTimeoutMS(config->terminationTimeoutMS !=0 ? config->terminationTimeoutMS : TERM_TO_MS_DEF),
		removedProxyCleanupIntervalSec(config->removedProxyCleanupIntervalSec > 0 ? config->removedProxyCleanupIntervalSec : REM_PROXY_CLEANUP_TO_SEC_DEF),
		monitoringPolicyDeviceClientClass(config->monitoringPolicyDeviceClientClass),
		monitoringPolicyGatewayClientClass(config->monitoringPolicyGatewayClientClass),
		monitoringPolicyConnectorClientClass(config->monitoringPolicyConnectorClientClass),
		monitoringPolicyAppClientClass(config->monitoringPolicyAppClientClass),
		monitoringPolicyAppScaleClientClass(config->monitoringPolicyAppScaleClientClass)
{
}

//=== ActivityStats ===========================================================
ActivityStats::ActivityStats() :
		num_db_insert(0),
		num_db_update(0),
		num_db_bulk_update(0),
		num_db_bulk(0),
		num_db_delete(0),
		num_db_read(0),
		num_db_error(0),
		avg_db_read_latency_ms(0),
		avg_db_update_latency_ms(0),
		avg_db_bulk_update_latency_ms(0),
		num_activity(0),
		num_connectivity(0),
		num_clients(0),
		num_evict_clients(0),
		num_new_clients(0),
		memory_bytes(0),
	    avg_conflation_delay_ms(0),
		avg_conflation_factor(0),
		duration_sec(0)
{
}

ActivityStats::~ActivityStats()
{
}

std::string ActivityStats::toString() const
{
	std::ostringstream oss;
	oss << "db-Ins=" << num_db_insert
			<< " db-Upd=" << num_db_update
			<< " db-BkUpd=" << 	num_db_bulk_update
			<< " db-Bk=" << num_db_bulk
			<< " db-Del=" << num_db_delete
			<< " db-Read=" << num_db_read
			<< " db-Error=" << num_db_error
			<< " db-ReadLatency=" << avg_db_read_latency_ms
			<< " db-UpdLatency=" << avg_db_update_latency_ms
			<< " db-BkUpdLatency=" << avg_db_bulk_update_latency_ms
			<< " act=" << num_activity
			<< " conn=" << num_connectivity
			<< " clients=" << num_clients
			<< " evict-cl=" << num_evict_clients
			<< " new-cl=" << num_new_clients
			<< " mem-bytes=" << memory_bytes
			<< " conf-delay=" << avg_conflation_delay_ms
			<< " conf-factor=" << avg_conflation_factor
			<< " dur_sec=" << duration_sec;

	return oss.str();
}

ActivityStats operator-(const ActivityStats& x, const ActivityStats& y)
{
	ActivityStats z;
	z.num_db_insert = x.num_db_insert - y.num_db_insert;
	z.num_db_update = x.num_db_update - y.num_db_update;
	z.num_db_bulk_update = x.num_db_bulk_update - y.num_db_bulk_update;
	z.num_db_bulk = x.num_db_bulk - y.num_db_bulk;
	z.num_db_delete = x.num_db_delete - y.num_db_delete;
	z.num_db_read = x.num_db_read - y.num_db_read;
	z.num_db_error = x.num_db_error - y.num_db_error;

	z.avg_db_read_latency_ms = x.avg_db_read_latency_ms; //(g)
	z.avg_db_update_latency_ms = x.avg_db_update_latency_ms; //(g)
	z.avg_db_bulk_update_latency_ms = x.avg_db_bulk_update_latency_ms; //(g)

	z.num_activity = x.num_activity - y.num_activity;
	z.num_connectivity = x.num_connectivity - y.num_connectivity;
	z.num_clients = x.num_clients;  //(g)
	z.num_evict_clients = x.num_evict_clients - y.num_evict_clients;
	z.num_new_clients = x.num_new_clients - y.num_new_clients;
	z.memory_bytes = x.memory_bytes;  //(g)

	z.avg_conflation_delay_ms = x.avg_conflation_delay_ms;  //(g)
	z.avg_conflation_factor = x.avg_conflation_factor; //(g)

	z.duration_sec = x.duration_sec - y.duration_sec;
	return z;
}

ActivityStats operator+(const ActivityStats& x, const ActivityStats& y)
{
    ActivityStats z;
    z.num_db_insert = x.num_db_insert + y.num_db_insert;
    z.num_db_update = x.num_db_update + y.num_db_update;
    z.num_db_bulk_update = x.num_db_bulk_update + y.num_db_bulk_update;
    z.num_db_bulk = x.num_db_bulk + y.num_db_bulk;
    z.num_db_delete = x.num_db_delete + y.num_db_delete;
    z.num_db_read = x.num_db_read + y.num_db_read;
    z.num_db_error = x.num_db_error + y.num_db_error;

    z.avg_db_read_latency_ms = x.avg_db_read_latency_ms + y.avg_db_read_latency_ms;
    z.avg_db_update_latency_ms = x.avg_db_update_latency_ms + y.avg_db_update_latency_ms;
    z.avg_db_bulk_update_latency_ms = x.avg_db_bulk_update_latency_ms + y.avg_db_bulk_update_latency_ms;

    z.num_activity = x.num_activity + y.num_activity;
    z.num_connectivity = x.num_connectivity + y.num_connectivity;
    z.num_clients = x.num_clients + y.num_clients;
    z.num_evict_clients = x.num_evict_clients + y.num_evict_clients;
    z.num_new_clients = x.num_new_clients + y.num_new_clients;

    z.memory_bytes = x.memory_bytes + y.memory_bytes;
    z.avg_conflation_delay_ms = x.avg_conflation_delay_ms + y.avg_conflation_delay_ms;
    z.avg_conflation_factor = x.avg_conflation_factor + y.avg_conflation_factor;

    return z;
}

void ActivityStats::norm(int numBanks, int numWorkers)
{
    if (numBanks > 0)
    {
        avg_conflation_delay_ms = avg_conflation_delay_ms / numBanks;
        avg_conflation_factor = avg_conflation_factor / numBanks;
    }

    if (numWorkers > 0)
    {
        avg_db_read_latency_ms = avg_db_read_latency_ms / numWorkers;
        avg_db_update_latency_ms = avg_db_update_latency_ms / numWorkers;
        avg_db_bulk_update_latency_ms = avg_db_bulk_update_latency_ms / numWorkers;
    }
}

void ActivityStats::copyTo(ismPXACT_Stats_t* pStats)
{
    pStats->num_db_insert = num_db_insert;
    pStats->num_db_update = num_db_update;
    pStats->num_db_bulk_update = num_db_bulk_update;
    pStats->num_db_bulk = num_db_bulk;
    pStats->num_db_read = num_db_read;
    pStats->num_db_delete = num_db_delete;
    pStats->num_db_error = num_db_error;
    pStats->avg_db_read_latency_ms = avg_db_read_latency_ms;
    pStats->avg_db_update_latency_ms = avg_db_update_latency_ms;
    pStats->avg_db_bulk_update_latency_ms = avg_db_bulk_update_latency_ms;

    pStats->num_activity = num_activity;
    pStats->num_connectivity = num_connectivity;
    pStats->num_clients = num_clients;
    pStats->num_evict_clients = num_evict_clients;
    pStats->num_new_clients = num_new_clients;

    pStats->memory_bytes = memory_bytes;
    pStats->avg_conflation_delay_ms = avg_conflation_delay_ms;
    pStats->avg_conflation_factor = avg_conflation_factor;
}

//=== toString ================================================================


std::string toString(const ismPXACT_Stats_t* pStats)
{
    std::ostringstream oss;
    if (pStats)
    {
        oss << "state=" << pStats->activity_tracking_state
            << " db-Ins=" << pStats->num_db_insert
            << " db-Upd=" << pStats->num_db_update
            << " db-BkUpd=" << pStats-> num_db_bulk_update
            << " db-Bk=" << pStats->num_db_bulk
            << " db-Del=" << pStats->num_db_delete
            << " db-Read=" << pStats->num_db_read
            << " db-Error=" << pStats->num_db_error
            << " db-Latency=" << pStats->avg_db_read_latency_ms
            << " act=" << pStats->num_activity
            << " conn=" << pStats->num_connectivity
            << " clients=" << pStats->num_clients
            << " evict-cl=" << pStats->num_evict_clients
            << " new-cl=" << pStats->num_new_clients
            << " mem-bytes=" << pStats->memory_bytes
            << " conf-delay=" << pStats->avg_conflation_delay_ms
            << " conf-factor=" << pStats->avg_conflation_factor;
    }
    else
    {
        oss << "NULL";
    }

    return oss.str();
}


std::string toString(const ismPXActivity_ConfigActivityDB_t* config)
{
	if (config)
	{
		ActivityDBConfig c(config);
		return c.toString();
	}
	else
	{
		return "NULL";
	}
}

std::string toString(const ismPXACT_Client_t* pClient)
{
	if (pClient)
	{
		std::ostringstream oss;
		oss << "ClientID=" << SoN(pClient->pClientID)
			<< " OrgName=" << SoN(pClient->pOrgName)
			<< " ClientType=" << pClient->clientType
			<< " GwID=" << SoN(pClient->pGateWayID)
			<< " ActivityType=" << pClient->activityType;
		return oss.str();
	}
	else
	{
		return "NULL";
	}
}


std::string toString(const ismPXACT_ConnectivityInfo_t* pConnInfo)
{
	if (pConnInfo)
	{
		std::ostringstream oss;
		oss << "protocol=" << pConnInfo->protocol
			<< " cleanS=" << static_cast<int>(pConnInfo->cleanS)
			<< " expiryIntSec=" << pConnInfo->expiryIntervalSec
			<< " reasonCode=" << static_cast<int>(pConnInfo->reasonCode)
			<< " TrgSrv=" << pConnInfo->targetServerType;
		return oss.str();
	}
	else
	{
		return "NULL";
	}
}

//=== ActivityNotify ==========================================================
ActivityNotify::ActivityNotify()
{
}
ActivityNotify::~ActivityNotify()
{
}

//=== FailureNotify ===========================================================
FailureNotify::FailureNotify()
{
}
FailureNotify::~FailureNotify()
{
}
//=== ClientUpdateRecord ======================================================

ClientUpdateRecord::ClientUpdateRecord() :
	client_type(PXACT_CLIENT_TYPE_NONE),
	protocol(PXACT_CONN_PROTO_TYPE_NONE),
	conn_ev_type(PXACT_ACTIVITY_TYPE_NONE),
	disconn_ev_type(PXACT_ACTIVITY_TYPE_NONE),
	last_act_type(PXACT_ACTIVITY_TYPE_NONE),
	expiry_sec(0),
	clean_s(-1),
	reason_code(0),
	target_server(PXACT_TRG_SERVER_TYPE_NONE),
	dirty_flags(DIRTY_FLAG_CLEAN),
	update_timestamp(0),
	queuing_timestamp(0),
	num_updates(0),
	conn_refcount(0),
	connect_time(0),
	disconnect_time(0)
{
}

ClientUpdateRecord::ClientUpdateRecord(
		const char* pClientID,
		const char* pOrgName,
		const char* pDeviceType,
		const char* pDeviceID,
		PXACT_CLIENT_TYPE_t clientType,
		const char* pGateWayID) :
				client_id(new std::string(pClientID)),
				org(new std::string(pOrgName)),
				device_type(new std::string(pDeviceType ? pDeviceType : "")),
				device_id(new std::string(pDeviceID ? pDeviceID : "")),
				gateway_id(new std::string(pGateWayID ? pGateWayID : "")),
				client_type(clientType),
				protocol(PXACT_CONN_PROTO_TYPE_NONE),
				conn_ev_type(PXACT_ACTIVITY_TYPE_NONE),
				disconn_ev_type(PXACT_ACTIVITY_TYPE_NONE),
				last_act_type(PXACT_ACTIVITY_TYPE_NONE),
				expiry_sec(0),
				clean_s(-1),
				reason_code(0),
				target_server(PXACT_TRG_SERVER_TYPE_NONE),
				dirty_flags(DIRTY_FLAG_CLEAN),
				update_timestamp(0),
				queuing_timestamp(0),
				num_updates(0),
				conn_refcount(0),
				connect_time(0),
				disconnect_time(0)
{
}

ClientUpdateRecord::~ClientUpdateRecord()
{
}

void ClientUpdateRecord::reset()
{
	client_id.reset();
	org.reset();
	device_type.reset();
	device_id.reset();
	gateway_id.reset();
	client_type = PXACT_CLIENT_TYPE_NONE;
	conn_ev_type = PXACT_ACTIVITY_TYPE_NONE;
	disconn_ev_type = PXACT_ACTIVITY_TYPE_NONE;
	last_act_type = PXACT_ACTIVITY_TYPE_NONE;
	protocol = PXACT_CONN_PROTO_TYPE_NONE;
	clean_s = (-1);
	reason_code = 0;
	expiry_sec = 0;
	target_server = (0);
	update_timestamp = (0);
	dirty_flags = (DIRTY_FLAG_CLEAN);
	connect_time = (0);
	disconnect_time = (0);
}

std::string ClientUpdateRecord::toString() const
{
	std::ostringstream oss;
	oss << "CID=" << (client_id ? *client_id : "")
		<< " Org=" << (org ? *org : "")
		<< " DevType= " << (device_type ? *device_type : "")
		<< " DevID= " << (device_id ? *device_id : "")
		<< " GwID=" << (gateway_id ? *gateway_id : "")
		<< " CType=" << client_type
		<< " proto=" << protocol
		<< " connEv=" << conn_ev_type
		<< " disconnEv=" << disconn_ev_type
		<< " connect_time=" << connect_time
		<< " disconnect_time=" << disconnect_time
		<< " actEv=" << last_act_type
		<< " cleanS=" << static_cast<int>(clean_s)
		<< " expirySec=" << expiry_sec
		<< " reasonCode=" << static_cast<int>(reason_code)
		<< " TrgSrv=" << static_cast<int>(target_server)
		<< " dirty=";
	for (int i=7; i>=0; --i)
		oss << ((((int)dirty_flags)>>i) & 0x01);

	return oss.str();
}

std::size_t ClientUpdateRecord::getSizeBytes() const
{
	using namespace std;

	std::size_t sz = 17 + 5*(sizeof(StringSPtr) + sizeof(string))
			+ (client_id ? client_id->size() : 0)
			+ (org ? org->size() : 0)
			+ (device_type ? device_type->size() : 0)
			+ (device_id ? device_id->size() : 0)
			+ (gateway_id ? gateway_id->size() : 0);

	return sz;
}

bool compare_ts_less(const ClientUpdateRecord_SPtr& p1, const ClientUpdateRecord_SPtr& p2)
{
	if (p1->dirty_flags == 0 && p2->dirty_flags > 0)
	{
		return true;
	}
	else if (p1->dirty_flags > 0 && p2->dirty_flags == 0)
	{
		return false;
	}
	else //dirty_flags are either both ==0 or both >0
	{
		return p1->update_timestamp < p2->update_timestamp;
	}
}


bool Compare_TS_Less::operator()(const ClientUpdateRecord_SPtr& p1, const ClientUpdateRecord_SPtr& p2) const
{
	if (p1->dirty_flags == 0 && p2->dirty_flags > 0)
	{
		return true;
	}
	else if (p1->dirty_flags > 0 && p2->dirty_flags == 0)
	{
		return false;
	}
	else //dirty_flags are either both ==0 or both >0
	{
		return p1->update_timestamp < p2->update_timestamp;
	}
}

//=== ClientStateRecord =======================================================
ClientStateRecord::ClientStateRecord() :
		client_type(PXACT_CLIENT_TYPE_NONE),
		conn_ev_time(0),
		conn_ev_type(PXACT_ACTIVITY_TYPE_NONE),
		last_connect_time(0),
		last_disconnect_time(0),
		last_act_time(0),
		last_pub_time(0),
		last_act_type(PXACT_ACTIVITY_TYPE_NONE),
		protocol(PXACT_CONN_PROTO_TYPE_NONE),
		clean_s(-1),
		expiry_sec(0),
		reason_code(0),
		target_server(PXACT_TRG_SERVER_TYPE_NONE),
		version(0),
		return_code(0),
		next(NULL)
{
}

ClientStateRecord::~ClientStateRecord()
{
	destroyNext();
}


void ClientStateRecord::destroyNext()
{
	ClientStateRecord* p1 = next;
	while (p1)
	{
		next = p1->next;
		p1->next = NULL;
		delete p1;
		p1 = next;
	}
}

std::string ClientStateRecord::toString() const
{
	std::ostringstream oss;
	oss << "CID=" << client_id
			<< " Org=" << org
			<< " DevType=" << device_type
			<< " DevID=" << device_id
			<< " GwID=" << gateway_id
			<< " CType=" << client_type
			<< " connEvT=" << conn_ev_time
			<< " connEv=" << conn_ev_type
			<< " lastConnT=" << last_connect_time
			<< " lastDisconnT=" << last_disconnect_time
			<< " actEvT=" << last_act_time
			<< " actEv=" << last_act_type
			<< " protocol=" << protocol
			<< " cleanS=" << (int)clean_s
			<< " expirySec=" << expiry_sec
			<< " reasonCode=" << reason_code
			<< " TrgSrv=" << (int)target_server
			<< " V=" << version
			<< "; Nx=" << next << ";";

	return oss.str();

}

bool ClientStateRecord::isConnectType(const PXACT_ACTIVITY_TYPE_t type)
{
	return (type >= PXACT_ACTIVITY_TYPE_CONN
			&& type <= PXACT_ACTIVITY_TYPE_CONN_END);
}

bool ClientStateRecord::isDisconnectType(const PXACT_ACTIVITY_TYPE_t type)
{
	return (type >= PXACT_ACTIVITY_TYPE_DISCONN
			&& type <= PXACT_ACTIVITY_TYPE_DISCONN_END);
}

bool ClientStateRecord::isPubType(const PXACT_ACTIVITY_TYPE_t type)
{
	return ((type == PXACT_ACTIVITY_TYPE_MQTT_PUBLISH) || (type == PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT) || (type == PXACT_ACTIVITY_TYPE_HTTP_POST_COMMAND));
}

bool ClientStateRecord::isMqttActivityType(const PXACT_ACTIVITY_TYPE_t type)
{
	return (type >= PXACT_ACTIVITY_TYPE_MQTT
			&& type <= PXACT_ACTIVITY_TYPE_MQTT_END);
}

bool ClientStateRecord::isHttpActivityType(const PXACT_ACTIVITY_TYPE_t type)
{
	return (type >= PXACT_ACTIVITY_TYPE_HTTP
			&& type <= PXACT_ACTIVITY_TYPE_HTTP_END);
}
//=== ProxyStateRecord ========================================================
ProxyStateRecord::ProxyStateRecord() :
		hbto_ms(-1),
		last_hb(-1),
		status(PX_STATUS_BOGUS),
		version(-1),
		next(NULL)
{
}

ProxyStateRecord::ProxyStateRecord(const std::string& uid, int hbto_ms, int64_t last_hb, PxStatus status, int64_t version) :
		proxy_uid(uid), hbto_ms(hbto_ms), last_hb(last_hb), status(status), version(version),
		next(NULL)
{
}

ProxyStateRecord::~ProxyStateRecord()
{
	destroyNext();
}

void ProxyStateRecord::destroyNext()
{
	ProxyStateRecord* p1 = next;
	while (p1)
	{
		next = p1->next;
		p1->next = NULL;
		delete p1;
		p1 = next;
	}
}

std::string ProxyStateRecord::toString() const
{
	std::ostringstream oss;
	oss << "UID=" << proxy_uid << " HBTO=" << hbto_ms << " LHB=" << last_hb << " S=" << status
			<< " V=" << version << "; Nx=" << next;
	return oss.str();
}

//=== TokenBucket =============================================================
TokenBucket::TokenBucket(uint32_t rate_iops, uint32_t bucket_size) :
		closed_(false),
		bucket_Size_(std::max(bucket_size, static_cast<uint32_t>(1))),
		bucket_(bucket_Size_),
		rate_IOpS_(rate_iops),
		lastRefillSec_(ism_common_readTSC())
{
	TRACE(7,"%s rate=%d, size=%d\n", __FUNCTION__, rate_iops, bucket_size);
}

TokenBucket::~TokenBucket()
{
	close();
}

int TokenBucket::getTokens(uint32_t num)
{
	TRACE(7,"%s Entry, num=%d\n", __FUNCTION__, num);

	std::unique_lock<std::mutex> ulock(mutex_);
	if (closed_)
	{
		TRACE(7,"%s Exit, rc=%d\n", __FUNCTION__, ISMRC_Closed);
		return ISMRC_Closed;
	}

	if (rate_IOpS_ == 0)
	{
		TRACE(7,"%s Exit, rc=%d\n", __FUNCTION__, ISMRC_OK);
		return ISMRC_OK; //unlimited
	}

	if (num > bucket_Size_)
	{
		TRACE(1,"%s Exit, rc=%d\n", __FUNCTION__, ISMRC_Error);
		return ISMRC_Error;
	}

	while (!closed_ && rate_IOpS_ > 0 && num > bucket_)
	{
		condVar_.wait(ulock);
	}

	if (closed_)
	{
		return ISMRC_Closed;
	}
	else
	{
		bucket_ -= num;
		TRACE(7,"%s Exit, rc=%d\n", __FUNCTION__, ISMRC_OK);
		return ISMRC_OK;
	}
}

int TokenBucket::tryGetTokens(uint32_t num)
{
	LockGuard lock(mutex_);
	if (closed_)
	{
		return ISMRC_Closed;
	}

	if (rate_IOpS_ == 0)
	{
		return ISMRC_OK; //unlimited
	}

	if (num > bucket_Size_)
	{
		return ISMRC_Error;
	}

	if (num <= bucket_)
	{
		bucket_ -= num;
		return ISMRC_OK;
	}
	else
	{
		return ISMRC_WouldBlock;
	}
}

int TokenBucket::setLimit(uint32_t rate_iops)
{
	{
		LockGuard lock(mutex_);
		rate_IOpS_ = rate_iops;
	}
	condVar_.notify_all();
	return ISMRC_OK;
}

uint32_t TokenBucket::getBucketSize() const
{
	LockGuard lock(mutex_);
	return bucket_Size_;
}
uint32_t TokenBucket::getLimit() const
{
	LockGuard lock(mutex_);
	return rate_IOpS_;
}

int TokenBucket::refill()
{
	double currentTimeSec = ism_common_readTSC();
	double bucket1 = 0;
	double bucket2 = 0;

	{
		LockGuard lock(mutex_);
		if (closed_)
		{
			TRACE(7,"%s Exit, rc=%d\n", __FUNCTION__, ISMRC_Closed);
			return ISMRC_Closed;
		}

		double interval = currentTimeSec - lastRefillSec_;
		if (interval <= 0)
		{
			TRACE(1,"%s Error: rc=%d\n", __FUNCTION__, ISMRC_Error);
			return ISMRC_Error;
		}
		bucket1 = bucket_;
		bucket_ = std::min((double)bucket_Size_, bucket_ + interval * rate_IOpS_);
		bucket2 = bucket_;
		lastRefillSec_ = currentTimeSec;
	}
	condVar_.notify_all();
	TRACE(7,"%s bucket-before=%f, bucket-after=%f, rc=%d\n", __FUNCTION__, bucket1, bucket2, ISMRC_OK);
	return ISMRC_OK;
}

int TokenBucket::close()
{
	{
		LockGuard lock(mutex_);
		closed_ = true;
	}
	condVar_.notify_all();
	return ISMRC_OK;
}


int TokenBucket::restart()
{
    {
        LockGuard lock(mutex_);
        closed_ = false;
    }
    return ISMRC_OK;
}

} /* namespace ism_proxy */
