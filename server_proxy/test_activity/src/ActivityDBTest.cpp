/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
 * AffinityDBTest.cpp
 * Tests need single node (or more) MongoDB cluster, with DB and Collections predefined (e.g. in mongo shell):
 * USE activity_track_unittest
 *
 * db.createCollection("proxy_state")
 * db.proxy_state.createIndex({"proxy_uid" : 1}, {unique : true})
 *
 * db.createCollection("client_state")
 * db.client_state.createIndex({client_id: 1},{unique: true})
 *
 *
 * In addition, set environment variable SERVERPROXY_UNITTEST_MONGO_URI to a MongoDB server mongodb:\\IP:port
 *
 */

#include <pthread.h>
#include <memory>
#include <string>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include "pxactivity.h"
#include "ActivityDB.h"

#include "ServerProxyUnitTestInit.h"


BOOST_AUTO_TEST_SUITE( DB_suite )

BOOST_AUTO_TEST_SUITE( Client_suite )

BOOST_AUTO_TEST_CASE( connect )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();

    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->close();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    delete db;
}

BOOST_AUTO_TEST_CASE( connect_fragments )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = NULL;
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();

    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();

    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->close();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;

    sleep(1);
}

BOOST_AUTO_TEST_CASE( connect_bad )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    shared_ptr<ActivityDB> db = shared_ptr<ActivityDB>(new ActivityDB());
    BOOST_CHECK(db);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB-oops"; //<<
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();

    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config-1: " << toString(&config) << endl;
    int rc = db->configure(&config);
    BOOST_CHECK(rc != ISMRC_OK);

    db.reset(new ActivityDB());
    config.pDBType = "MongoDB";
    config.pMongoEndpoints = "[\"oops]"; //<<
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config-2: " << toString(&config) << endl;
    rc = db->configure(&config);
    BOOST_CHECK(rc != ISMRC_OK);


    trim_inplace(ServerProxyUnitTestInit::mongodbUSER);
    if (ServerProxyUnitTestInit::mongodbUSER.empty())
    {
        std::cout << "Test without access control... skipping rest of test" << std::endl;
        return;
    }

    db.reset(new ActivityDB());
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();

    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = NULL; //<<
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";
    cout << "config-3: " << toString(&config) << endl;
    rc = db->configure(&config);
    BOOST_CHECK(rc == ISMRC_OK);
    rc = db->connect();
    BOOST_CHECK(rc != ISMRC_OK);


    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);
    ClientStateRecord client_state;
    client_state.client_id = "this-is-not-a-client";
    rc = client->read(&client_state);
    BOOST_CHECK_EQUAL(rc, ISMRC_Error);
    db->closeClient(client);

    db.reset(new ActivityDB());
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();

    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest-oops"; //<<
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";
    cout << "config-4: " << toString(&config) << endl;
    rc = db->configure(&config);
    BOOST_CHECK(rc == ISMRC_OK);
    rc = db->connect();
    BOOST_CHECK(rc != ISMRC_OK);

    db.reset(new ActivityDB());
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();

    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state-oops"; //<<
    config.pMongoProxyStateColl = "proxy_state";
    cout << "config-5: " << toString(&config) << endl;
    rc = db->configure(&config);
    BOOST_CHECK(rc == ISMRC_OK);
    rc = db->connect();
    //The access control in WIoTP allow CRUD access to any collection... therefore this does not work.
    //BOOST_CHECK(rc != ISMRC_OK);

    db.reset(new ActivityDB());
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();

    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state-oops"; //<<
    cout << "config-7: " << toString(&config) << endl;
    rc = db->configure(&config);
    BOOST_CHECK(rc == ISMRC_OK);
    rc = db->connect();
    //TODO BOOST_CHECK(rc != ISMRC_OK);

}

BOOST_AUTO_TEST_CASE( long_host_bad )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    shared_ptr<ActivityDB> db = shared_ptr<ActivityDB>(new ActivityDB());
    BOOST_CHECK(db);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB"; //<<
    //config.pMongoURI = NULL;
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();

    string long_host1 = "server1";
    while (long_host1.size() < BSON_HOST_NAME_MAX+1)
    {
        long_host1.append(".com");
    }

    string long_host2 = "server2";
    while (long_host2.size() < BSON_HOST_NAME_MAX+1)
    {
        long_host2.append(".com");
    }

    ostringstream oss;
    oss << "[\"" << long_host1 << ":12345\",\"" << long_host2 << ":12345\"]";
    string endpoint = oss.str();
    config.pMongoEndpoints = endpoint.c_str();

    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config-1: " << toString(&config) << endl;
    int rc = db->configure(&config);
    BOOST_CHECK(rc != ISMRC_OK);

}


BOOST_AUTO_TEST_CASE( create_client )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);
    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( insert )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest"; //<<
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ClientUpdateRecord_SPtr update(new ClientUpdateRecord);
    update->client_id.reset(new string("d:org1:type1:client1"));
    update->org.reset(new string("org1"));
    update->device_type.reset(new string);
    update->device_id.reset(new string);

    update->device_type.reset(new string);
    update->device_id.reset(new string);
    update->gateway_id.reset(new string);
    update->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    update->last_act_type = PXACT_ACTIVITY_TYPE_NONE;
    update->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update->clean_s = 0;
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;
    string proxy_uid("ProxyTest1");

    std::cout << ">>insert: " << update->toString() << std::endl;
    rc = client->insert(update.get(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ClientStateRecord client_state;
    client_state.client_id = *(update->client_id);
    client_state.org = *(update->org);
    rc = client->read(&client_state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    BOOST_CHECK_EQUAL(client_state.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK(client_state.conn_ev_time > 0);
    BOOST_CHECK(client_state.last_connect_time > 0);
    BOOST_CHECK(client_state.last_disconnect_time == 0);
    BOOST_CHECK_EQUAL(client_state.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state.proxy_uid, proxy_uid);

    std::cout << ">>read: " << client_state.toString() << std::endl;

    rc = client->insert(update.get(), "ProxyTest2");
    BOOST_CHECK_EQUAL(rc, ISMRC_ExistingKey);

    rc = client->removeClientID(*(update->org), *(update->client_id));
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->read(&client_state);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( insert_2 )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ClientUpdateRecord_SPtr update(new ClientUpdateRecord);
    update->client_id.reset(new string("d:org1:devType1:dev1"));
    update->org.reset(new string("org1"));
    update->device_type.reset(new string("devType1"));
    update->device_id.reset(new string("dev1"));
    update->gateway_id.reset(new string);
    update->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_NETWORK;
    update->last_act_type = PXACT_ACTIVITY_TYPE_NONE;
    update->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update->clean_s = 0;
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;
    string proxy_uid("ProxyTest1");

    std::cout << ">>insert: " << update->toString() << std::endl;
    rc = client->insert(update.get(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ClientStateRecord client_state;
    client_state.client_id = *(update->client_id);
    client_state.org = *(update->org);
    rc = client->read(&client_state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    BOOST_CHECK_EQUAL(client_state.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state.device_id, *(update->device_id));
    BOOST_CHECK_EQUAL(client_state.device_type, *(update->device_type));
    BOOST_CHECK_EQUAL(client_state.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK(client_state.conn_ev_time > 0);
    BOOST_CHECK(client_state.last_connect_time == 0);
    BOOST_CHECK(client_state.last_disconnect_time > 0);
    BOOST_CHECK_EQUAL(client_state.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state.proxy_uid, proxy_uid);

    std::cout << ">>read: " << client_state.toString() << std::endl;

    rc = client->insert(update.get(), "ProxyTest2");
    BOOST_CHECK_EQUAL(rc, ISMRC_ExistingKey);

    rc = client->removeClientID(*(update->org), *(update->client_id));
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->read(&client_state);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( remove_all )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    ClientUpdateRecord_SPtr update(new ClientUpdateRecord);
    update->client_id.reset(new string("d:org1:client1"));
    update->org.reset(new string("org1"));
    update->device_type.reset(new string);
    update->device_id.reset(new string);
    update->gateway_id.reset(new string);
    update->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    update->last_act_type = PXACT_ACTIVITY_TYPE_NONE;
    update->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update->clean_s = 0;
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;
    string proxy_uid("ProxyTest1");

    std::cout << ">>insert: " << update->toString() << std::endl;
    rc = client->insert(update.get(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ClientStateRecord client_state;
    client_state.client_id = *(update->client_id);
    client_state.org = *(update->org);
    rc = client->read(&client_state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    BOOST_CHECK_EQUAL(client_state.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK_EQUAL(client_state.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state.proxy_uid, proxy_uid);

    std::cout << ">>read: " << client_state.toString() << std::endl;

    update->client_id.reset(new string("d:org1:client2"));
    std::cout << ">>insert: " << update->toString() << std::endl;
    rc = client->insert(update.get(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    client_state.client_id = *(update->client_id);
    client_state.org = *(update->org);
    rc = client->read(&client_state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    BOOST_CHECK_EQUAL(client_state.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK_EQUAL(client_state.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state.proxy_uid, proxy_uid);

    std::cout << ">>read: " << client_state.toString() << std::endl;
    //===

    rc = client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->read(&client_state);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    client_state.client_id = "d:org1:client1";
    rc = client->read(&client_state);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( update_one )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //=== insert

    ClientUpdateRecord_SPtr update(new ClientUpdateRecord);
    update->client_id.reset(new string("d:org1:client1"));
    update->org.reset(new string("org1"));
    update->device_type.reset(new string);
    update->device_id.reset(new string);

    update->gateway_id.reset(new string("gw1"));
    update->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    update->last_act_type = PXACT_ACTIVITY_TYPE_NONE;
    update->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update->clean_s = 0;
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;
    string proxy_uid("ProxyTest1");

    std::cout << ">>insert: " << update->toString() << std::endl;
    rc = client->insert(update.get(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);


    ClientStateRecord client_state1;
    client_state1.client_id = *(update->client_id);
    client_state1.org = *(update->org);
    rc = client->read(&client_state1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    BOOST_CHECK_EQUAL(client_state1.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state1.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state1.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state1.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK(client_state1.conn_ev_time > 0);
    BOOST_CHECK(client_state1.last_connect_time > 0);
    BOOST_CHECK(client_state1.last_disconnect_time == 0);
    BOOST_CHECK_EQUAL(client_state1.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state1.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state1.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state1.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state1.proxy_uid, proxy_uid);
    BOOST_CHECK_EQUAL(client_state1.version, 1L);

    std::cout << ">>read: " << client_state1.toString() << std::endl;
    usleep(5000);

    //=== update 1 - diconn
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT;
    update->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
    std::cout << ">>update: " << update->toString() << std::endl;
    rc = client->updateOne(update.get(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);


    ClientStateRecord client_state2;
    client_state2.client_id = *(update->client_id);
    client_state2.org = *(update->org);
    rc = client->read(&client_state2);
    BOOST_CHECK_EQUAL(client_state2.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state2.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state2.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state2.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK(client_state2.conn_ev_time > client_state1.conn_ev_time);
    BOOST_CHECK(client_state2.last_connect_time == client_state1.last_connect_time);
    BOOST_CHECK(client_state2.last_disconnect_time >  client_state1.last_connect_time);

    BOOST_CHECK_EQUAL(client_state2.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state2.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state2.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state2.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state2.proxy_uid, proxy_uid);
    BOOST_CHECK_EQUAL(client_state2.version, 2L);

    BOOST_CHECK(client_state2.conn_ev_time > client_state1.conn_ev_time);
    BOOST_CHECK(client_state2.last_act_time == 0);

    std::cout << ">>read2: " << client_state2.toString() << std::endl;

    //=== update 3 - act
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;
    update->last_act_type = PXACT_ACTIVITY_TYPE_HTTP_GET;
    std::cout << ">>update: " << update->toString() << std::endl;
    rc = client->updateOne(update.get(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = client->read(&client_state1);

    BOOST_CHECK_EQUAL(client_state1.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state1.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state1.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state1.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK_EQUAL(client_state1.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state1.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state1.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state1.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state1.proxy_uid, proxy_uid);
    BOOST_CHECK_EQUAL(client_state1.version, 3L);

    BOOST_CHECK(client_state2.conn_ev_time == client_state1.conn_ev_time);
    BOOST_CHECK(client_state2.last_act_time < client_state1.last_act_time);

    //=== update 4 - act with meta
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT | ClientUpdateRecord::DIRTY_FLAG_METADATA;
    update->last_act_type = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
    update->target_server = PXACT_TRG_SERVER_TYPE_MS_KAFKA;
    std::cout << ">>update: " << update->toString() << std::endl;
    rc = client->updateOne(update.get(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = client->read(&client_state2);

    BOOST_CHECK_EQUAL(client_state2.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state2.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state2.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state2.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK_EQUAL(client_state2.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state2.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state2.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state2.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state2.proxy_uid, proxy_uid);
    BOOST_CHECK_EQUAL(client_state2.version, 4L);

    BOOST_CHECK(client_state2.conn_ev_time == client_state1.conn_ev_time);
    BOOST_CHECK(client_state2.last_act_time > client_state1.last_act_time);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}

BOOST_AUTO_TEST_CASE( update_one_cond )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //=== insert

    ClientUpdateRecord_SPtr update(new ClientUpdateRecord);
    update->client_id.reset(new string("d:org1:client1"));
    update->org.reset(new string("org1"));
    update->device_type.reset(new string);
    update->device_id.reset(new string);
    update->gateway_id.reset(new string("gw1"));
    update->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    update->last_act_type = PXACT_ACTIVITY_TYPE_NONE;
    update->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update->clean_s = 0;
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;
    string proxy_uid("ProxyTest1");

    std::cout << ">>insert: " << update->toString() << std::endl;
    rc = client->insert(update.get(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);


    ClientStateRecord client_state1;
    client_state1.client_id = *(update->client_id);
    client_state1.org = *(update->org);
    rc = client->read(&client_state1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    BOOST_CHECK_EQUAL(client_state1.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state1.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state1.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state1.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK_EQUAL(client_state1.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state1.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state1.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state1.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state1.proxy_uid, proxy_uid);
    BOOST_CHECK_EQUAL(client_state1.version, 1L);

    std::cout << ">>read: " << client_state1.toString() << std::endl;


    //=== update 1 - diconn
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT;
    update->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
    std::cout << ">>update: " << update->toString() << std::endl;
    rc = client->updateOne_CondVersion(update.get(), proxy_uid, 100);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    rc = client->updateOne_CondVersion(update.get(), proxy_uid, 1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ClientStateRecord client_state2;
    client_state2.client_id = *(update->client_id);
    client_state2.org = *(update->org);
    rc = client->read(&client_state2);
    BOOST_CHECK_EQUAL(client_state2.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state2.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state2.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state2.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK_EQUAL(client_state2.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state2.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state2.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state2.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state2.proxy_uid, proxy_uid);
    BOOST_CHECK_EQUAL(client_state2.version, 2L);

    BOOST_CHECK(client_state2.conn_ev_time > client_state1.conn_ev_time);
    BOOST_CHECK(client_state2.last_act_time == 0);

    std::cout << ">>read2: " << client_state2.toString() << std::endl;

    //=== update 3 - act
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;
    update->last_act_type = PXACT_ACTIVITY_TYPE_HTTP_GET;
    std::cout << ">>update: " << update->toString() << std::endl;
    rc = client->updateOne_CondVersion(update.get(), proxy_uid, 1);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    rc = client->updateOne_CondVersion(update.get(), proxy_uid, 2);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = client->read(&client_state1);

    BOOST_CHECK_EQUAL(client_state1.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state1.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state1.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state1.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK_EQUAL(client_state1.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state1.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state1.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state1.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state1.proxy_uid, proxy_uid);
    BOOST_CHECK_EQUAL(client_state1.version, 3L);

    BOOST_CHECK(client_state2.conn_ev_time == client_state1.conn_ev_time);
    BOOST_CHECK(client_state2.last_act_time < client_state1.last_act_time);

    //=== update 4 - act with meta
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT | ClientUpdateRecord::DIRTY_FLAG_METADATA;
    update->last_act_type = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
    update->target_server = PXACT_TRG_SERVER_TYPE_MS_KAFKA;
    std::cout << ">>update: " << update->toString() << std::endl;

    rc = client->updateOne_CondVersion(update.get(), proxy_uid, 6);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);
    rc = client->updateOne_CondVersion(update.get(), proxy_uid, 3);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = client->read(&client_state2);

    BOOST_CHECK_EQUAL(client_state2.org, *(update->org));
    BOOST_CHECK_EQUAL(client_state2.gateway_id, *(update->gateway_id));
    BOOST_CHECK_EQUAL(client_state2.client_type, update->client_type);
    BOOST_CHECK_EQUAL(client_state2.conn_ev_type, update->conn_ev_type);
    BOOST_CHECK_EQUAL(client_state2.last_act_type, update->last_act_type);
    BOOST_CHECK_EQUAL(client_state2.target_server, update->target_server);
    BOOST_CHECK_EQUAL(client_state2.clean_s, update->clean_s);
    BOOST_CHECK_EQUAL(client_state2.protocol, update->protocol);
    BOOST_CHECK_EQUAL(client_state2.proxy_uid, proxy_uid);
    BOOST_CHECK_EQUAL(client_state2.version, 4L);

    BOOST_CHECK(client_state2.conn_ev_time == client_state1.conn_ev_time);
    BOOST_CHECK(client_state2.last_act_time > client_state1.last_act_time);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( update_one_not_found )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ClientUpdateRecord_SPtr update(new ClientUpdateRecord);
    update->client_id.reset(new string("d:org1:client1"));
    update->org.reset(new string("org1"));
    update->device_type.reset(new string);
    update->device_id.reset(new string);

    update->gateway_id.reset(new string("gw1"));
    update->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    update->last_act_type = PXACT_ACTIVITY_TYPE_NONE;
    update->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update->clean_s = 0;
    update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT;
    string proxy_uid("ProxyTest1");


    ClientStateRecord client_state1;
    client_state1.client_id = *(update->client_id);
    client_state1.org = *(update->org);
    rc = client->read(&client_state1);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    //=== update
    rc = client->updateOne(update.get(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( update_bulk )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //=== insert
    unsigned NUM =10;
    string proxy_uid("ProxyTest1");

    ClientUpdateVector updateVec;
    for (unsigned i=0; i<NUM; ++i)
    {
        ClientUpdateRecord_SPtr update(new ClientUpdateRecord);
        update->client_id.reset(new string("d:org1:client"));
        update->client_id->append(std::to_string(i));
        update->org.reset(new string("org1"));
        update->device_type.reset(new string);
        update->device_id.reset(new string);

        update->gateway_id.reset(new string("gw1"));
        update->client_type = PXACT_CLIENT_TYPE_DEVICE;
        update->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        update->last_act_type = PXACT_ACTIVITY_TYPE_NONE;
        update->target_server = PXACT_TRG_SERVER_TYPE_MS;
        update->clean_s = 0;
        update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;

        rc = client->insert(update.get(), proxy_uid);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);

        updateVec.push_back(update);
    }

    //=== read 1

    for (unsigned i=0; i<NUM; ++i)
    {

        ClientStateRecord client_state1;
        client_state1.client_id = *(updateVec[i]->client_id);
        client_state1.org = *(updateVec[i]->org);
        rc = client->read(&client_state1);


        BOOST_CHECK_EQUAL(client_state1.org, *(updateVec[i]->org));
        BOOST_CHECK_EQUAL(client_state1.gateway_id, *(updateVec[i]->gateway_id));
        BOOST_CHECK_EQUAL(client_state1.client_type, updateVec[i]->client_type);
        BOOST_CHECK_EQUAL(client_state1.conn_ev_type, updateVec[i]->conn_ev_type);
        BOOST_CHECK_EQUAL(client_state1.last_act_type, updateVec[i]->last_act_type);
        BOOST_CHECK_EQUAL(client_state1.target_server, updateVec[i]->target_server);
        BOOST_CHECK_EQUAL(client_state1.clean_s, updateVec[i]->clean_s);
        BOOST_CHECK_EQUAL(client_state1.protocol, updateVec[i]->protocol);
        BOOST_CHECK_EQUAL(client_state1.proxy_uid, proxy_uid);
        BOOST_CHECK_EQUAL(client_state1.version, 1L);

        BOOST_CHECK(client_state1.conn_ev_time > 0);
        BOOST_CHECK(client_state1.last_act_time == 0);

        updateVec[i]->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;
        updateVec[i]->last_act_type = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
    }

    usleep(10000);

    //=== bulk update
    rc = client->updateBulk(updateVec, updateVec.size(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //=== read 1

    for (unsigned i=0; i<NUM; ++i)
    {

        ClientStateRecord client_state1;
        client_state1.client_id = *(updateVec[i]->client_id);
        client_state1.org = *(updateVec[i]->org);
        rc = client->read(&client_state1);

        BOOST_CHECK_EQUAL(client_state1.org, *(updateVec[i]->org));
        BOOST_CHECK_EQUAL(client_state1.gateway_id, *(updateVec[i]->gateway_id));
        BOOST_CHECK_EQUAL(client_state1.client_type, updateVec[i]->client_type);
        BOOST_CHECK_EQUAL(client_state1.conn_ev_type, updateVec[i]->conn_ev_type);
        BOOST_CHECK_EQUAL(client_state1.last_act_type, updateVec[i]->last_act_type);
        BOOST_CHECK_EQUAL(client_state1.target_server, updateVec[i]->target_server);
        BOOST_CHECK_EQUAL(client_state1.clean_s, updateVec[i]->clean_s);
        BOOST_CHECK_EQUAL(client_state1.protocol, updateVec[i]->protocol);
        BOOST_CHECK_EQUAL(client_state1.proxy_uid, proxy_uid);
        BOOST_CHECK_EQUAL(client_state1.version, 2L);

        BOOST_CHECK(client_state1.conn_ev_time > 0);
        BOOST_CHECK(client_state1.last_act_time > 0);
        BOOST_CHECK(client_state1.conn_ev_time < client_state1.last_act_time);
    }

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( update_bulk_w_mismatch )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //=== insert
    unsigned NUM =10;
    string proxy_uid("ProxyTest1");

    ClientUpdateVector updateVec;
    for (unsigned i=0; i<NUM; ++i)
    {
        ClientUpdateRecord_SPtr update(new ClientUpdateRecord);
        update->client_id.reset(new string("d:org1:client"));
        update->client_id->append(std::to_string(i));
        update->org.reset(new string("org1"));
        update->device_type.reset(new string);
        update->device_id.reset(new string);

        update->gateway_id.reset(new string("gw1"));
        update->client_type = PXACT_CLIENT_TYPE_DEVICE;
        update->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        update->last_act_type = PXACT_ACTIVITY_TYPE_NONE;
        update->target_server = PXACT_TRG_SERVER_TYPE_MS;
        update->clean_s = 0;
        update->protocol = PXACT_CONN_PROTO_TYPE_MQTT_v5;
        update->expiry_sec = 99;
        update->reason_code = 1;
        update->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD;

        rc = client->insert(update.get(), proxy_uid);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);

        updateVec.push_back(update);
    }

    //=== read 1

    for (unsigned i=0; i<NUM; ++i)
    {

        ClientStateRecord client_state1;
        client_state1.client_id = *(updateVec[i]->client_id);
        client_state1.org = *(updateVec[i]->org);
        rc = client->read(&client_state1);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);

        BOOST_CHECK_EQUAL(client_state1.org, *(updateVec[i]->org));
        BOOST_CHECK_EQUAL(client_state1.gateway_id, *(updateVec[i]->gateway_id));
        BOOST_CHECK_EQUAL(client_state1.client_type, updateVec[i]->client_type);
        BOOST_CHECK_EQUAL(client_state1.conn_ev_type, updateVec[i]->conn_ev_type);
        BOOST_CHECK_EQUAL(client_state1.last_act_type, updateVec[i]->last_act_type);
        BOOST_CHECK_EQUAL(client_state1.target_server, updateVec[i]->target_server);
        BOOST_CHECK_EQUAL(client_state1.clean_s, updateVec[i]->clean_s);
        BOOST_CHECK_EQUAL(client_state1.protocol, updateVec[i]->protocol);
        BOOST_CHECK_EQUAL(client_state1.expiry_sec, updateVec[i]->expiry_sec);
        BOOST_CHECK_EQUAL(client_state1.reason_code, updateVec[i]->reason_code);
        BOOST_CHECK_EQUAL(client_state1.proxy_uid, proxy_uid);
        BOOST_CHECK_EQUAL(client_state1.version, 1L);

        BOOST_CHECK(client_state1.conn_ev_time > 0);
        BOOST_CHECK(client_state1.last_act_time == 0);
    }

    usleep(10000);

    //=== delete some - simulate expiry

    for (unsigned i=0; i<NUM; i+=2)
    {
        rc = client->removeClientID(*updateVec[i]->org, *updateVec[i]->client_id);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    for (unsigned i=0; i<NUM; ++i)
    {
        updateVec[i]->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;
        updateVec[i]->last_act_type = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
    }

    //=== bulk update
    rc = client->updateBulk(updateVec, updateVec.size(), proxy_uid);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //=== read 1

    for (unsigned i=0; i<NUM; ++i)
    {

        ClientStateRecord client_state1;
        client_state1.client_id = *(updateVec[i]->client_id);
        client_state1.org = *(updateVec[i]->org);
        rc = client->read(&client_state1);

        BOOST_CHECK_EQUAL(rc, ISMRC_OK);

        if (i%2 == 0)
        {
            BOOST_CHECK_EQUAL(client_state1.org, *(updateVec[i]->org));
            BOOST_CHECK_EQUAL(client_state1.gateway_id, *(updateVec[i]->gateway_id));
            BOOST_CHECK_EQUAL(client_state1.client_type, updateVec[i]->client_type);
            BOOST_CHECK_EQUAL(client_state1.conn_ev_type, PXACT_ACTIVITY_TYPE_NONE);
            BOOST_CHECK_EQUAL(client_state1.last_act_type, updateVec[i]->last_act_type);
            BOOST_CHECK_EQUAL(client_state1.target_server, updateVec[i]->target_server);
            BOOST_CHECK_EQUAL(client_state1.clean_s, -1);
            BOOST_CHECK_EQUAL(client_state1.protocol, PXACT_CONN_PROTO_TYPE_NONE);
            BOOST_CHECK_EQUAL(client_state1.expiry_sec, 0);
            BOOST_CHECK_EQUAL(client_state1.reason_code, 0);
            BOOST_CHECK_EQUAL(client_state1.proxy_uid, std::string(PROXY_UID_NULL));
            BOOST_CHECK_EQUAL(client_state1.version, 1L);

            BOOST_CHECK(client_state1.conn_ev_time == 0);
            BOOST_CHECK(client_state1.last_act_time > 0);
            BOOST_CHECK(client_state1.conn_ev_time < client_state1.last_act_time);
        }
        else
        {
            BOOST_CHECK_EQUAL(client_state1.org, *(updateVec[i]->org));
            BOOST_CHECK_EQUAL(client_state1.gateway_id, *(updateVec[i]->gateway_id));
            BOOST_CHECK_EQUAL(client_state1.client_type, updateVec[i]->client_type);
            BOOST_CHECK_EQUAL(client_state1.conn_ev_type, updateVec[i]->conn_ev_type);
            BOOST_CHECK_EQUAL(client_state1.last_act_type, updateVec[i]->last_act_type);
            BOOST_CHECK_EQUAL(client_state1.target_server, updateVec[i]->target_server);
            BOOST_CHECK_EQUAL(client_state1.clean_s, updateVec[i]->clean_s);
            BOOST_CHECK_EQUAL(client_state1.protocol, updateVec[i]->protocol);
            BOOST_CHECK_EQUAL(client_state1.expiry_sec, updateVec[i]->expiry_sec);
            BOOST_CHECK_EQUAL(client_state1.reason_code, updateVec[i]->reason_code);
            BOOST_CHECK_EQUAL(client_state1.proxy_uid, proxy_uid);
            BOOST_CHECK_EQUAL(client_state1.version, 2L);

            BOOST_CHECK(client_state1.conn_ev_time > 0);
            BOOST_CHECK(client_state1.last_act_time > 0);
            BOOST_CHECK(client_state1.conn_ev_time < client_state1.last_act_time);
        }




    }

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( cleanup_client_proxy )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ClientUpdateRecord_SPtr update1(new ClientUpdateRecord);
    update1->client_id.reset(new string("d:org1:devType1:dev1"));
    update1->org.reset(new string("org1"));
    update1->device_type.reset(new string("devType1"));
    update1->device_id.reset(new string("dev1"));
    update1->gateway_id.reset(new string);
    update1->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update1->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    update1->last_act_type = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
    update1->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update1->clean_s = 0;
    update1->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;

    ClientUpdateRecord_SPtr update2(new ClientUpdateRecord);
    update2->client_id.reset(new string("d:org2:devType2:dev2"));
    update2->org.reset(new string("org2"));
    update2->device_type.reset(new string("devType2"));
    update2->device_id.reset(new string("dev2"));
    update2->gateway_id.reset(new string);
    update2->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update2->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    update2->last_act_type = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
    update2->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update2->clean_s = 0;
    update2->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;


    ClientUpdateRecord_SPtr update3(new ClientUpdateRecord);
    update3->client_id.reset(new string("d:org3:devType3:dev3"));
    update3->org.reset(new string("org3"));
    update3->device_type.reset(new string("devType3"));
    update3->device_id.reset(new string("dev3"));
    update3->gateway_id.reset(new string);
    update3->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update3->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    update3->last_act_type = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
    update3->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update3->clean_s = 0;
    update3->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;


    ClientUpdateRecord_SPtr update4(new ClientUpdateRecord);
    update4->client_id.reset(new string("d:org4:devType4:dev4"));
    update4->org.reset(new string("org4"));
    update4->device_type.reset(new string("devType4"));
    update4->device_id.reset(new string("dev4"));
    update4->gateway_id.reset(new string);
    update4->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update4->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_SERVER;
    update4->last_act_type = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
    update4->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update4->clean_s = 0;
    update4->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;

    ClientUpdateRecord_SPtr update5(new ClientUpdateRecord);
    update5->client_id.reset(new string("d:org5:devType5:dev5"));
    update5->org.reset(new string("org5"));
    update5->device_type.reset(new string("devType5"));
    update5->device_id.reset(new string("dev5"));
    update5->gateway_id.reset(new string);
    update5->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update5->conn_ev_type = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_NETWORK;
    update5->last_act_type = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
    update5->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update5->clean_s = 0;
    update5->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;

    ClientUpdateRecord_SPtr update6(new ClientUpdateRecord);
    update6->client_id.reset(new string("d:org6:devType6:dev6"));
    update6->org.reset(new string("org6"));
    update6->device_type.reset(new string("devType6"));
    update6->device_id.reset(new string("dev6"));
    update6->gateway_id.reset(new string);
    update6->client_type = PXACT_CLIENT_TYPE_DEVICE;
    update6->conn_ev_type = PXACT_ACTIVITY_TYPE_NONE;
    update6->last_act_type = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
    update6->target_server = PXACT_TRG_SERVER_TYPE_MS;
    update6->clean_s = 0;
    update6->dirty_flags = ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT;

    string proxy_uid1("ProxyTest1");

    std::cout << ">>insert 1: " << update1->toString() << std::endl;
    rc = client->insert(update1.get(), proxy_uid1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    std::cout << ">>insert 2: " << update2->toString() << std::endl;
    rc = client->insert(update2.get(), proxy_uid1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    std::cout << ">>insert 3: " << update3->toString() << std::endl;
    rc = client->insert(update3.get(), proxy_uid1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    std::cout << ">>insert 4: " << update4->toString() << std::endl;
    rc = client->insert(update4.get(), proxy_uid1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    std::cout << ">>insert 5: " << update5->toString() << std::endl;
    rc = client->insert(update5.get(), proxy_uid1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    std::cout << ">>insert 6: " << update6->toString() << std::endl;
    rc = client->insert(update6.get(), proxy_uid1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ClientStateRecord client_state;
    int list_size = 0;
    int onlyConn = 1;
    rc = client->findAllAssociatedToProxy(proxy_uid1, onlyConn, 100, &client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 3);
    set<string> ids;
    for (ClientStateRecord* curr = &client_state; curr != NULL; curr = curr->next)
    {
        ids.insert(curr->client_id);
    }
    BOOST_CHECK_EQUAL(ids.size(), 3);
    BOOST_CHECK( ids.count(*(update1->client_id)) > 0 );
    BOOST_CHECK( ids.count(*(update2->client_id)) > 0 );
    BOOST_CHECK( ids.count(*(update3->client_id)) > 0 );
    client_state.destroyNext();

    onlyConn = 2;
    rc = client->findAllAssociatedToProxy(proxy_uid1, onlyConn, 100, &client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 2);
    ids.clear();
    for (ClientStateRecord* curr = &client_state; curr !=NULL; curr = curr->next)
    {
        ids.insert(curr->client_id);
    }
    BOOST_CHECK_EQUAL(ids.size(), 2);
    BOOST_CHECK( ids.count(*update4->client_id) > 0 );
    BOOST_CHECK( ids.count(*update5->client_id) > 0 );

    onlyConn = 0;
    rc = client->findAllAssociatedToProxy(proxy_uid1, onlyConn, 100, &client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 5);
    ids.clear();
    for (ClientStateRecord* curr = &client_state; curr !=NULL; curr = curr->next)
    {
        ids.insert(curr->client_id);
    }
    BOOST_CHECK_EQUAL(ids.size(), 5);
    BOOST_CHECK( ids.count(*(update1->client_id)) > 0 );
    BOOST_CHECK( ids.count(*(update2->client_id)) > 0 );
    BOOST_CHECK( ids.count(*(update3->client_id)) > 0 );
    BOOST_CHECK( ids.count(*update4->client_id) > 0 );
    BOOST_CHECK( ids.count(*update5->client_id) > 0 );
    BOOST_CHECK( ids.count(*update6->client_id) == 0 );

    rc = client->disassociatedFromProxy(proxy_uid1, 1, 100, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 3);

    rc = client->disassociatedFromProxy(proxy_uid1, 1, 100, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);
    BOOST_CHECK_EQUAL(list_size, 0);

    rc = client->disassociatedFromProxy(proxy_uid1,2, 100, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 2);

    rc = client->disassociatedFromProxy(proxy_uid1,2, 100, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);
    BOOST_CHECK_EQUAL(list_size, 0);

    rc = client->disassociatedFromProxy(proxy_uid1,0, 100, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);
    BOOST_CHECK_EQUAL(list_size, 0);

    //===
    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}



BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( Proxy_suite )

BOOST_AUTO_TEST_CASE( proxy_insert )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeProxyUID("proxy1");
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->insertNewUID("proxy1", 20000);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->insertNewUID("proxy1", 20000);
    BOOST_CHECK_EQUAL(rc, ISMRC_ExistingKey);

    ProxyStateRecord state;
    state.proxy_uid = "proxy1";
    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(state.hbto_ms,20000);
    BOOST_CHECK_EQUAL(state.version, 1);
    BOOST_CHECK(state.last_hb > 0);
    BOOST_CHECK_EQUAL(state.status, ProxyStateRecord::PX_STATUS_ACTIVE);

    rc = client->removeProxyUID("proxy1");
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( proxy_remove_all )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->insertNewUID("proxy-1", 20000);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->insertNewUID("proxy-2", 20000);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ProxyStateRecord state;
    state.proxy_uid = "proxy-1";
    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    state.proxy_uid = "proxy-2";
    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->removeAllProxyUID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    state.proxy_uid = "proxy-1";
    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    state.proxy_uid = "proxy-2";
    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}

BOOST_AUTO_TEST_CASE( proxy_heartbeat )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeProxyUID("proxy2");
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->insertNewUID("proxy2", 20000);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ProxyStateRecord state;
    state.proxy_uid = "proxy2";
    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(state.hbto_ms,20000);
    BOOST_CHECK_EQUAL(state.version, 1);
    int64_t last_hb = state.last_hb;
    BOOST_CHECK(last_hb > 0);
    BOOST_CHECK_EQUAL(state.status, ProxyStateRecord::PX_STATUS_ACTIVE);
    usleep(100);

    rc = client->updateHeartbeat(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK(state.last_hb > last_hb);
    BOOST_CHECK_EQUAL(state.version, 2);

    rc = client->removeProxyUID("proxy2");
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}

BOOST_AUTO_TEST_CASE( proxy_status )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeProxyUID("proxy3");
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->insertNewUID("proxy3", 20000);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ProxyStateRecord state;
    state.proxy_uid = "proxy3";
    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(state.hbto_ms,20000);
    BOOST_CHECK_EQUAL(state.version, 1);
    int64_t last_hb = state.last_hb;
    BOOST_CHECK(last_hb > 0);
    BOOST_CHECK_EQUAL(state.status, ProxyStateRecord::PX_STATUS_ACTIVE);
    usleep(100);

    rc = client->updateHeartbeat(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK(state.last_hb > last_hb);
    BOOST_CHECK_EQUAL(state.version, 2);

    rc = client->updateStatus_CondVersion("proxy3", ProxyStateRecord::PX_STATUS_SUSPECT, 1);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(state.hbto_ms,20000);
    BOOST_CHECK_EQUAL(state.version, 2);
    BOOST_CHECK(last_hb > 0);
    BOOST_CHECK_EQUAL(state.status, ProxyStateRecord::PX_STATUS_ACTIVE);

    rc = client->updateStatus_CondVersion("proxy3", ProxyStateRecord::PX_STATUS_SUSPECT, 2);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(state.hbto_ms,20000);
    BOOST_CHECK_EQUAL(state.version, 3);
    BOOST_CHECK(last_hb > 0);
    BOOST_CHECK_EQUAL(state.status, ProxyStateRecord::PX_STATUS_SUSPECT);


    rc = client->removeProxyUID("proxy3");
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( proxy_get_active )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeAllProxyUID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string proxyNames[] = {"proxy4","proxy5","proxy6","proxy7","proxy8","proxy9"};
    for (string s : proxyNames)
    {
        rc = client->insertNewUID(s, 20000);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    //-6
    ProxyStateRecord* state = new ProxyStateRecord;
    int list_size = 0;
    rc = client->getAllWithStatus(ProxyStateRecord::PX_STATUS_ACTIVE,state,&list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 6);
    std::set<std::string> uids;
    for (ProxyStateRecord* current = state; current; current = current->next)
    {
        uids.insert(current->proxy_uid);
        BOOST_CHECK_EQUAL(current->status,ProxyStateRecord::PX_STATUS_ACTIVE);
        BOOST_CHECK_EQUAL(current->version,1);
    }
    BOOST_CHECK_EQUAL(uids.size(), 6);
    for (string s : proxyNames)
    {
        BOOST_CHECK_EQUAL(uids.count(s), 1);
    }
    delete state;

    //-5
    rc = client->updateStatus_CondVersion(proxyNames[0], ProxyStateRecord::PX_STATUS_SUSPECT, 1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    state = new ProxyStateRecord;
    list_size = 0;
    rc = client->getAllWithStatus(ProxyStateRecord::PX_STATUS_ACTIVE,state,&list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 5);
    uids.clear();
    for (ProxyStateRecord* current = state; current; current = current->next)
    {
        uids.insert(current->proxy_uid);
        BOOST_CHECK_EQUAL(current->status,ProxyStateRecord::PX_STATUS_ACTIVE);
        BOOST_CHECK_EQUAL(current->version,1);
    }
    BOOST_CHECK_EQUAL(uids.size(), 5);
    for (unsigned i=1; i<6; i++)
    {
        BOOST_CHECK_EQUAL(uids.count(proxyNames[i]), 1);
    }
    delete state;


    //-4
    rc = client->updateStatus_CondVersion(proxyNames[1], ProxyStateRecord::PX_STATUS_SUSPECT, 1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    state = new ProxyStateRecord;
    list_size = 0;
    rc = client->getAllWithStatus(ProxyStateRecord::PX_STATUS_ACTIVE,state,&list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 4);
    uids.clear();
    for (ProxyStateRecord* current = state; current; current = current->next)
    {
        uids.insert(current->proxy_uid);
        BOOST_CHECK_EQUAL(current->status,ProxyStateRecord::PX_STATUS_ACTIVE);
        BOOST_CHECK_EQUAL(current->version,1);
    }
    BOOST_CHECK_EQUAL(uids.size(), 4);
    for (unsigned i=2; i<6; ++i)
    {
        BOOST_CHECK_EQUAL(uids.count(proxyNames[i]), 1);
    }
    state->destroyNext();
    delete state;

    rc = client->removeAllProxyUID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( proxy_get_status_in )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeAllProxyUID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string proxyNames[] = {"proxy4","proxy5","proxy6","proxy7","proxy8","proxy9"};
    for (string s : proxyNames)
    {
        rc = client->insertNewUID(s, 20000);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    //-6
    ProxyStateRecord* state = new ProxyStateRecord;
    int list_size = 0;
    vector<ProxyStateRecord::PxStatus> status_set_active;
    status_set_active.push_back(ProxyStateRecord::PX_STATUS_ACTIVE);

    vector<ProxyStateRecord::PxStatus> status_set_suspect_leave;
    status_set_suspect_leave.push_back(ProxyStateRecord::PX_STATUS_SUSPECT);
    status_set_suspect_leave.push_back(ProxyStateRecord::PX_STATUS_LEAVE);

    rc = client->getAllWithStatusIn(status_set_active,state,&list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 6);
    std::set<std::string> uids;
    for (ProxyStateRecord* current = state; current; current = current->next)
    {
        uids.insert(current->proxy_uid);
        BOOST_CHECK_EQUAL(current->status,ProxyStateRecord::PX_STATUS_ACTIVE);
        BOOST_CHECK_EQUAL(current->version,1);
    }
    BOOST_CHECK_EQUAL(uids.size(), 6);
    for (string s : proxyNames)
    {
        BOOST_CHECK_EQUAL(uids.count(s), 1);
    }
    delete state;

    state = new ProxyStateRecord;
    list_size = 0;
    rc = client->getAllWithStatusIn(status_set_suspect_leave,state,&list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);
    BOOST_CHECK_EQUAL(list_size, 0);
    delete state;

    //-5
    rc = client->updateStatus_CondVersion(proxyNames[0], ProxyStateRecord::PX_STATUS_SUSPECT, 1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    state = new ProxyStateRecord;
    list_size = 0;
    rc = client->getAllWithStatus(ProxyStateRecord::PX_STATUS_ACTIVE,state,&list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 5);
    uids.clear();
    for (ProxyStateRecord* current = state; current; current = current->next)
    {
        uids.insert(current->proxy_uid);
        BOOST_CHECK_EQUAL(current->status,ProxyStateRecord::PX_STATUS_ACTIVE);
        BOOST_CHECK_EQUAL(current->version,1);
    }
    BOOST_CHECK_EQUAL(uids.size(), 5);
    for (unsigned i=1; i<6; i++)
    {
        BOOST_CHECK_EQUAL(uids.count(proxyNames[i]), 1);
    }
    delete state;

    state = new ProxyStateRecord;
    list_size = 0;
    rc = client->getAllWithStatusIn(status_set_suspect_leave,state,&list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 1);
    uids.clear();
    for (ProxyStateRecord* current = state; current; current = current->next)
    {
        uids.insert(current->proxy_uid);
        BOOST_CHECK(current->status == ProxyStateRecord::PX_STATUS_SUSPECT || current->status == ProxyStateRecord::PX_STATUS_LEAVE);
    }
    BOOST_CHECK_EQUAL(uids.size(), 1);
    delete state;

    //-4
    rc = client->updateStatus_CondVersion(proxyNames[1], ProxyStateRecord::PX_STATUS_LEAVE, 1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    state = new ProxyStateRecord;
    list_size = 0;
    rc = client->getAllWithStatus(ProxyStateRecord::PX_STATUS_ACTIVE,state,&list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 4);
    uids.clear();
    for (ProxyStateRecord* current = state; current; current = current->next)
    {
        uids.insert(current->proxy_uid);
        BOOST_CHECK_EQUAL(current->status,ProxyStateRecord::PX_STATUS_ACTIVE);
        BOOST_CHECK_EQUAL(current->version,1);
    }
    BOOST_CHECK_EQUAL(uids.size(), 4);
    for (unsigned i=2; i<6; ++i)
    {
        BOOST_CHECK_EQUAL(uids.count(proxyNames[i]), 1);
    }
    state->destroyNext();
    delete state;

    state = new ProxyStateRecord;
    list_size = 0;
    rc = client->getAllWithStatusIn(status_set_suspect_leave,state,&list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 2);
    uids.clear();
    for (ProxyStateRecord* current = state; current; current = current->next)
    {
        uids.insert(current->proxy_uid);
        BOOST_CHECK(current->status == ProxyStateRecord::PX_STATUS_SUSPECT || current->status == ProxyStateRecord::PX_STATUS_LEAVE);
    }
    BOOST_CHECK_EQUAL(uids.size(), 2);
    delete state;

    rc = client->removeAllProxyUID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}


BOOST_AUTO_TEST_CASE( proxy_connect_close_reconnect )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityDB* db = new ActivityDB();
    BOOST_CHECK(db != NULL);

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    string uri;
    trim_inplace(uri);
    if (uri.empty())
    {
        config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    }
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";

    cout << "config: " << toString(&config) << endl;

    int rc = db->configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeProxyUID("proxy1");
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->insertNewUID("proxy1", 20000);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->insertNewUID("proxy1", 20000);
    BOOST_CHECK_EQUAL(rc, ISMRC_ExistingKey);

    ProxyStateRecord state;
    state.proxy_uid = "proxy1";
    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(state.hbto_ms,20000);
    BOOST_CHECK_EQUAL(state.version, 1);
    BOOST_CHECK(state.last_hb > 0);
    BOOST_CHECK_EQUAL(state.status, ProxyStateRecord::PX_STATUS_ACTIVE);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->close();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    client = db->getClient();
    BOOST_CHECK(client);

    rc = client->removeProxyUID("proxy1");
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->insertNewUID("proxy1", 20000);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->insertNewUID("proxy1", 20000);
    BOOST_CHECK_EQUAL(rc, ISMRC_ExistingKey);

    state.proxy_uid = "proxy1";
    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(state.hbto_ms,20000);
    BOOST_CHECK_EQUAL(state.version, 1);
    BOOST_CHECK(state.last_hb > 0);
    BOOST_CHECK_EQUAL(state.status, ProxyStateRecord::PX_STATUS_ACTIVE);

    rc = client->removeProxyUID("proxy1");
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = client->read(&state);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);

    rc = db->closeClient(client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db->close();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    delete db;
}



BOOST_AUTO_TEST_SUITE_END() //Proxy_suite

BOOST_AUTO_TEST_SUITE_END() //DB_suite



