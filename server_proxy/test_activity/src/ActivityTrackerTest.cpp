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
 * ActivityTrackerTest.cpp
 *  Created on: Nov 13, 2017
 */

#include <pthread.h>
#include <memory>
#include <string>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include "pxactivity.h"
#include "ActivityTracker.h"

#include "ServerProxyUnitTestInit.h"

BOOST_AUTO_TEST_SUITE( Tracker_suite )


BOOST_AUTO_TEST_SUITE( LifeCycle_suite)


BOOST_AUTO_TEST_CASE( start_close_soft )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 100;
    config.hbTimeoutMS = 10000;

    cout << "config: " << toString(&config) << endl;

    ActivityTracker tracker;
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_INIT);
    int rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_CONFIG);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_START);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;
    sleep(2);

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_TERM);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
}


BOOST_AUTO_TEST_CASE( start_stop_close )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 100;
    config.hbTimeoutMS = 10000;

    cout << "config: " << toString(&config) << endl;

    ActivityTracker tracker;
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_INIT);
    int rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_CONFIG);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_START);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;
    sleep(1);
    rc = tracker.stop();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_STOP);

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_TERM);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
}


BOOST_AUTO_TEST_CASE( start_stop_start_close )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 100;
    config.hbTimeoutMS = 10000;

    cout << "config: " << toString(&config) << endl;

    ActivityTracker tracker;
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_INIT);
    int rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_CONFIG);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_START);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;
    sleep(1);
    rc = tracker.stop();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_STOP);

    sleep(1);
    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_START);

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_TERM);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
}

BOOST_AUTO_TEST_CASE( start_close_soft_fragments )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = NULL;

    config.pMongoEndpoints = ServerProxyUnitTestInit::mongodbEP.c_str();
    config.pTrustStore = ServerProxyUnitTestInit::mongodbCAFile.c_str();
    config.useTLS = ServerProxyUnitTestInit::mongodbUseTLS;
    config.pMongoOptions = ServerProxyUnitTestInit::mongodbOptions.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();

    config.pMongoDBName = "activity_track_unittest";
    config.pMongoClientStateColl = "client_state";
    config.pMongoProxyStateColl = "proxy_state";
    config.hbIntervalMS = 100;
    config.hbTimeoutMS = 10000;

    cout << "config: " << toString(&config) << endl;

    ActivityTracker tracker;
    int rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;
    sleep(2);

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
}

BOOST_AUTO_TEST_CASE( start_close_hard )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 100;
    config.hbTimeoutMS = 10000;

    cout << "config: " << toString(&config) << endl;

    ActivityTracker tracker;
    int rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;
    sleep(2);

    rc = tracker.close(false);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
}

BOOST_AUTO_TEST_CASE( conn_disconn_1 )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 2000;
    config.hbTimeoutMS = 20000;
    config.numWorkerThreads = 1;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    rc = tracker.clientConnectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    sleep(1);
    ClientStateRecord record1;
    record1.client_id = client.pClientID;
    record1.org = client.pOrgName;
    rc = db_client->read(&record1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(record1.conn_ev_time > 0);
    BOOST_CHECK(record1.last_connect_time > 0);
    BOOST_CHECK(record1.last_disconnect_time == 0);
    BOOST_CHECK(record1.conn_ev_time == record1.last_connect_time);
    BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(record1.last_act_time > 0);
    BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
    BOOST_CHECK(record1.gateway_id.empty());
    BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);

    BOOST_CHECK_EQUAL(record1.expiry_sec, connInfo.expiryIntervalSec);
    BOOST_CHECK_EQUAL(record1.reason_code, connInfo.reasonCode);
    BOOST_CHECK_EQUAL(record1.version, 1L);

    //=== disconnect
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_SERVER;
    rc = tracker.clientConnectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    sleep(1);
    ClientStateRecord record2;
    record2.client_id = client.pClientID;
    record2.org = client.pOrgName;
    rc = db_client->read(&record2);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_SERVER);
    BOOST_CHECK(record2.conn_ev_time > 0);
    BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
    BOOST_CHECK(record2.last_connect_time > 0);
    BOOST_CHECK(record2.last_disconnect_time > record2.last_connect_time);
    BOOST_CHECK_EQUAL(record2.conn_ev_time, record2.last_disconnect_time);

    BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(record2.last_act_time > 0);
    BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
    BOOST_CHECK(record2.gateway_id.empty());
    BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
    BOOST_CHECK_EQUAL(record2.expiry_sec, connInfo.expiryIntervalSec);
    BOOST_CHECK_EQUAL(record2.reason_code, connInfo.reasonCode);
    BOOST_CHECK_EQUAL(record2.version, 2L);

    //connect and change meta
    client.pGateWayID = "g:org1:gw1";
    connInfo.expiryIntervalSec = 60;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v5;
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    rc = tracker.clientConnectivity(&client,  &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    sleep(1);
    rc = db_client->read(&record1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(record1.conn_ev_time > record2.conn_ev_time);

    BOOST_CHECK(record1.last_connect_time > record1.last_disconnect_time);
    BOOST_CHECK(record1.last_disconnect_time > 0);
    BOOST_CHECK(record1.conn_ev_time == record1.last_connect_time);

    BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(record1.last_act_time > 0);
    BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
    BOOST_CHECK(strcmp(record1.gateway_id.c_str(), client.pGateWayID)==0);
    BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
    BOOST_CHECK_EQUAL(record1.expiry_sec, connInfo.expiryIntervalSec);
    BOOST_CHECK_EQUAL(record1.reason_code, connInfo.reasonCode);
    BOOST_CHECK_EQUAL(record1.version, 3L);

    //disconnect
    connInfo.expiryIntervalSec = 5;
    connInfo.reasonCode = 10;
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
    rc = tracker.clientConnectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    sleep(1);
    rc = db_client->read(&record2);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
    BOOST_CHECK(record2.conn_ev_time > 0);
    BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);

    BOOST_CHECK(record2.last_connect_time < record2.last_disconnect_time);
    BOOST_CHECK(record2.last_disconnect_time > 0);
    BOOST_CHECK(record2.conn_ev_time == record2.last_disconnect_time);

    BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
    BOOST_CHECK(record2.last_act_time > 0);
    BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
    BOOST_CHECK(strcmp(record2.gateway_id.c_str(), client.pGateWayID)==0);
    BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
    BOOST_CHECK_EQUAL(record2.expiry_sec, connInfo.expiryIntervalSec);
    BOOST_CHECK_EQUAL(record2.reason_code, connInfo.reasonCode);
    BOOST_CHECK_EQUAL(record2.version, 4L);

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    db.closeClient(db_client);
    db.close();
}

BOOST_AUTO_TEST_CASE( act_1 )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 2000;
    config.hbTimeoutMS = 20000;
    config.numWorkerThreads = 1;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    //=== 1 activity
    client.activityType =  PXACT_ACTIVITY_TYPE_HTTP_GET;
    rc = tracker.clientActivity(&client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    sleep(1);
    ClientStateRecord record1;
    record1.client_id = client.pClientID;
    record1.org = client.pOrgName;
    rc = db_client->read(&record1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_NONE);
    BOOST_CHECK(record1.conn_ev_time == 0);
    BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
    BOOST_CHECK(record1.last_act_time > 0);
    BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
    BOOST_CHECK(record1.gateway_id.empty());
    BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record1.clean_s, -1);
    BOOST_CHECK_EQUAL(record1.protocol, PXACT_CONN_PROTO_TYPE_NONE);
    BOOST_CHECK_EQUAL(record1.version, 1L);

    //=== 2 activity
    client.activityType = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
    rc = tracker.clientActivity(&client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    sleep(1);
    ClientStateRecord record2;
    record2.client_id = client.pClientID;
    record2.org = client.pOrgName;
    rc = db_client->read(&record2);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_NONE);
    BOOST_CHECK(record2.conn_ev_time == 0);
    BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT);
    BOOST_CHECK(record2.last_act_time > 0);
    BOOST_CHECK(record2.last_act_time > record1.last_act_time );
    BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
    BOOST_CHECK(record2.gateway_id.empty());
    BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record2.clean_s, -1);
    BOOST_CHECK_EQUAL(record2.protocol, PXACT_CONN_PROTO_TYPE_NONE);
    BOOST_CHECK_EQUAL(record2.version, 2L);

    //3 activity and change meta
    client.pGateWayID = "g:org1:gw1";
    client.activityType = PXACT_ACTIVITY_TYPE_HTTP_GET;
    rc = tracker.clientActivity(&client);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    sleep(1);
    rc = db_client->read(&record1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_NONE);
    BOOST_CHECK(record1.conn_ev_time == 0);
    BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
    BOOST_CHECK(record1.last_act_time > record2.last_act_time);
    BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
    BOOST_CHECK(strcmp(record1.gateway_id.c_str(), client.pGateWayID)==0);
    BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record1.clean_s, -1);
    BOOST_CHECK_EQUAL(record1.protocol, PXACT_CONN_PROTO_TYPE_NONE);
    BOOST_CHECK_EQUAL(record1.version, 3L);

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    db.closeClient(db_client);
    db.close();
}


BOOST_AUTO_TEST_CASE( act_conn_disconn_1 )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 2000;
    config.hbTimeoutMS = 20000;
    config.numWorkerThreads = 1;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    rc = tracker.clientConnectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    for (int i=0; i<100; ++i)
    {
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(1);
    ClientStateRecord record1;
    record1.client_id = client.pClientID;
    record1.org = client.pOrgName;
    rc = db_client->read(&record1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(record1.conn_ev_time > 0);
    BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
    BOOST_CHECK(record1.last_act_time > 0);
    BOOST_CHECK(record1.last_act_time >= record1.conn_ev_time);
    BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
    BOOST_CHECK(record1.gateway_id.empty());
    BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
    BOOST_CHECK(record1.version >= 1L);
    std::cout << record1.toString() << std::endl;

    //=== disconnect
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
    rc = tracker.clientConnectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    client.pGateWayID = "g:org1:gw1";
    for (int i=0; i<100; ++i)
    {
        client.activityType = PXACT_ACTIVITY_TYPE_HTTP_GET;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(1);
    ClientStateRecord record2;
    record2.client_id = client.pClientID;
    record2.org = client.pOrgName;
    rc = db_client->read(&record2);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
    BOOST_CHECK(record2.conn_ev_time > 0);
    BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
    BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
    BOOST_CHECK(record2.last_act_time > 0);
    BOOST_CHECK(record2.last_act_time > record1.last_act_time);
    BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
    BOOST_CHECK(strcmp(record2.gateway_id.c_str(), client.pGateWayID)==0);
    BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
    BOOST_CHECK(record2.version > record1.version);

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    db.closeClient(db_client);
    db.close();
}


BOOST_AUTO_TEST_CASE( restart_act_conn_disconn_1 )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 2000;
    config.hbTimeoutMS = 20000;
    config.numWorkerThreads = 1;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(), PXACT_STATE_TYPE_CONFIG);

    ismPXActivity_ConfigActivityDB_t config_bad = config;
    //config_bad.pMongoURI = NULL;
    config_bad.pMongoEndpoints = NULL;

    rc = tracker.configure(&config_bad);
    BOOST_CHECK(rc != ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(), PXACT_STATE_TYPE_INIT);

    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(), PXACT_STATE_TYPE_CONFIG);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(), PXACT_STATE_TYPE_START);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    ismPXACT_Client_t client;
    memset(&client, 0, sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    rc = tracker.clientConnectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    for (int i = 0; i < 100; ++i)
    {
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(1);
    ClientStateRecord record1;
    record1.client_id = client.pClientID;
    record1.org = client.pOrgName;
    rc = db_client->read(&record1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(record1.conn_ev_time > 0);
    BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
    BOOST_CHECK(record1.last_act_time > 0);
    BOOST_CHECK(record1.last_act_time >= record1.conn_ev_time);
    BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
    BOOST_CHECK(record1.gateway_id.empty());
    BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
    BOOST_CHECK(record1.version >= 1L);
    std::cout << record1.toString() << std::endl;

    //=== disconnect
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
    rc = tracker.clientConnectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    client.pGateWayID = "g:org1:gw1";
    for (int i = 0; i < 100; ++i)
    {
        client.activityType =PXACT_ACTIVITY_TYPE_HTTP_GET;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(1);
    ClientStateRecord record2;
    record2.client_id = client.pClientID;
    record2.org = client.pOrgName;
    rc = db_client->read(&record2);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
    BOOST_CHECK(record2.conn_ev_time > 0);
    BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
    BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
    BOOST_CHECK(record2.last_act_time > 0);
    BOOST_CHECK(record2.last_act_time > record1.last_act_time);
    BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
    BOOST_CHECK(strcmp(record2.gateway_id.c_str(), client.pGateWayID) == 0);
    BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
    BOOST_CHECK(record2.version > record1.version);

    rc = tracker.stop();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(), PXACT_STATE_TYPE_STOP);

    //=== Delete DB
    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //=== restart ===
    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(), PXACT_STATE_TYPE_START);

    uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    memset(&client, 0, sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    memset(&connInfo, 0, sizeof(connInfo));
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    rc = tracker.clientConnectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    for (int i = 0; i < 100; ++i)
    {
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(2);
    record1.client_id = client.pClientID;
    record1.org = client.pOrgName;
    rc = db_client->read(&record1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(record1.conn_ev_time > 0);
    BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
    BOOST_CHECK(record1.last_act_time > 0);
    BOOST_CHECK(record1.last_act_time >= record1.conn_ev_time);
    BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
    BOOST_CHECK(record1.gateway_id.empty());
    BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
    BOOST_CHECK(record1.version >= 1L);
    std::cout << record1.toString() << std::endl;

    //=== disconnect
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
    rc = tracker.clientConnectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    client.pGateWayID = "g:org1:gw1";
    for (int i = 0; i < 100; ++i)
    {
        client.activityType =PXACT_ACTIVITY_TYPE_HTTP_GET;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(1);
    record2.client_id = client.pClientID;
    record2.org = client.pOrgName;
    rc = db_client->read(&record2);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
    BOOST_CHECK(record2.conn_ev_time > 0);
    BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
    BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
    BOOST_CHECK(record2.last_act_time > 0);
    BOOST_CHECK(record2.last_act_time > record1.last_act_time);
    BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
    BOOST_CHECK(strcmp(record2.gateway_id.c_str(), client.pGateWayID) == 0);
    BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
    BOOST_CHECK(record2.version > record1.version);


    rc = tracker.stop();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(), PXACT_STATE_TYPE_STOP);

    //=== Do not delete DB

    //=== restart ===
    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(), PXACT_STATE_TYPE_START);

    uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    memset(&client, 0, sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    memset(&connInfo, 0, sizeof(connInfo));
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    rc = tracker.clientConnectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    for (int i = 0; i < 100; ++i)
    {
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(2);
    record1.client_id = client.pClientID;
    record1.org = client.pOrgName;
    rc = db_client->read(&record1);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(record1.conn_ev_time > 0);
    BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
    BOOST_CHECK(record1.last_act_time > 0);
    BOOST_CHECK(record1.last_act_time >= record1.conn_ev_time);
    BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
    BOOST_CHECK(record1.gateway_id == record2.gateway_id);
    BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
    BOOST_CHECK(record1.version >= 1L);
    std::cout << record1.toString() << std::endl;

    //=== disconnect
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
    rc = tracker.clientConnectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    client.pGateWayID = "g:org1:gw2";
    for (int i = 0; i < 100; ++i)
    {
        client.activityType = PXACT_ACTIVITY_TYPE_HTTP_GET;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(1);
    record2.client_id = client.pClientID;
    record2.org = client.pOrgName;
    rc = db_client->read(&record2);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
    BOOST_CHECK(record2.conn_ev_time > 0);
    BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
    BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
    BOOST_CHECK(record2.last_act_time > 0);
    BOOST_CHECK(record2.last_act_time > record1.last_act_time);
    BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
    BOOST_CHECK(strcmp(record2.gateway_id.c_str(), client.pGateWayID) == 0);
    BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
    BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
    BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
    BOOST_CHECK(record2.version > record1.version);

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    db.closeClient(db_client);
    db.close();
}


BOOST_AUTO_TEST_CASE( conn_act_disconn_1K )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 2000;
    config.hbTimeoutMS = 20000;
    config.numWorkerThreads = 4;

    cout << "config: " << toString(&config) << endl;

    ismPXACT_Stats_t stats;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.getStats(&stats);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_INIT);
    BOOST_CHECK_EQUAL(stats.num_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_activity, 0);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 0);

    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = tracker.getStats(&stats);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_CONFIG);
    BOOST_CHECK_EQUAL(stats.num_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_activity, 0);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 0);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.getStats(&stats);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_START);
    BOOST_CHECK_EQUAL(stats.num_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_activity, 0);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 0);
    BOOST_CHECK_EQUAL(stats.num_new_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_evict_clients, 0);
    BOOST_CHECK(stats.num_db_insert >= 0); //the UID
    BOOST_CHECK(stats.num_db_read >= 0);
    BOOST_CHECK(stats.num_db_update >= 0);
    BOOST_CHECK(stats.num_db_bulk == 0);
    BOOST_CHECK(stats.num_db_bulk_update == 0);
    BOOST_CHECK(stats.num_db_delete == 0);
    BOOST_CHECK(stats.num_db_error == 0);
    BOOST_CHECK(stats.avg_db_read_latency_ms >= 0);
    BOOST_CHECK(stats.avg_conflation_delay_ms >= 0);
    BOOST_CHECK(stats.avg_conflation_factor >= 0);
    BOOST_CHECK(stats.memory_bytes == 0);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    string clientID = "d:org1:dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(5);

    rc = tracker.getStats(&stats);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_START);
    BOOST_CHECK_EQUAL(stats.num_clients, NUM);
    BOOST_CHECK_EQUAL(stats.num_activity, NUM);
    BOOST_CHECK_EQUAL(stats.num_connectivity, NUM);
    BOOST_CHECK_EQUAL(stats.num_new_clients, NUM);
    BOOST_CHECK_EQUAL(stats.num_evict_clients, 0);
    BOOST_CHECK(stats.num_db_insert >= NUM);
    BOOST_CHECK(stats.num_db_read >= NUM);
    BOOST_CHECK(stats.num_db_update >= 0);
    BOOST_CHECK(stats.num_db_bulk == 0);
    BOOST_CHECK(stats.num_db_bulk_update == 0);
    BOOST_CHECK(stats.num_db_delete == 0);
    BOOST_CHECK(stats.num_db_error == 0);
    BOOST_CHECK(stats.avg_db_read_latency_ms >= 0);
    BOOST_CHECK(stats.avg_conflation_delay_ms >= 0);
    BOOST_CHECK(stats.avg_conflation_factor >= 0);
    BOOST_CHECK(stats.memory_bytes > 0);

    //=== activity
    for (unsigned j=0; j<10; j++)
        for (unsigned i=0; i<NUM; ++i)
        {
            client.pClientID = client_id_vec[i].c_str();
            client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
            rc = tracker.clientActivity(&client);
            BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        }

    sleep(5);

    rc = tracker.getStats(&stats);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_START);
    BOOST_CHECK_EQUAL(stats.num_clients, NUM);
    BOOST_CHECK_EQUAL(stats.num_activity, NUM*10);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 0);
    BOOST_CHECK_EQUAL(stats.num_new_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_evict_clients, 0);
    BOOST_CHECK(stats.num_db_insert == 0);
    BOOST_CHECK(stats.num_db_read >= 0);
    BOOST_CHECK(stats.num_db_update > 0);
    BOOST_CHECK(stats.num_db_bulk > 0);
    BOOST_CHECK(stats.num_db_bulk_update > 0);
    BOOST_CHECK(stats.num_db_delete == 0);
    BOOST_CHECK(stats.num_db_error == 0);
    BOOST_CHECK(stats.avg_db_read_latency_ms >= 0);
    BOOST_CHECK(stats.avg_conflation_delay_ms >= 0);
    BOOST_CHECK(stats.avg_conflation_factor >= 0);
    BOOST_CHECK(stats.memory_bytes > 0);

    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << std::endl;


    ClientStateRecord record1;
    for (unsigned i=0; i<NUM; ++i)
    {
        record1.client_id = client_id_vec[i].c_str();
        record1.org = client.pOrgName;
        rc = db_client->read(&record1);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(record1.conn_ev_time > 0);
        BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
        BOOST_CHECK(record1.last_act_time > 0);
        BOOST_CHECK(record1.last_act_time >= record1.conn_ev_time);
        BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
        BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
        BOOST_CHECK(record1.version >= 1L);

    }

    std::cout << record1.toString() << std::endl;

    std::cout << "#clients=" << tracker.getNumClients() << " #Bytes=" << tracker.getSizeBytes() << std::endl;

    //=== disconnect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_HTTP_GET;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(10);
    ClientStateRecord record2;
    for (unsigned i=0; i<NUM; ++i)
    {
        record2.client_id = client_id_vec[i].c_str();
        record2.org = client.pOrgName;
        rc = db_client->read(&record2);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
        BOOST_CHECK(record2.conn_ev_time > 0);
        BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
        BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
        BOOST_CHECK(record2.last_act_time > 0);
        BOOST_CHECK(record2.last_act_time > record1.last_act_time);
        BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
        BOOST_CHECK(record2.version >= 2L);
    }

    std::cout << record2.toString() << std::endl;

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    db.closeClient(db_client);
    db.close();
}


BOOST_AUTO_TEST_CASE( conn_act_disconn_1K_stop_config_start )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 2000;
    config.hbTimeoutMS = 20000;
    config.numWorkerThreads = 4;

    //const char* pUri_tmp = config.pMongoURI;
    const char* pEP_tmp = config.pMongoEndpoints;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    string clientID = "d:org1:dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(5);

    //=== activity
    for (unsigned j=0; j<10; j++)
        for (unsigned i=0; i<NUM; ++i)
        {
            client.pClientID = client_id_vec[i].c_str();
            client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
            rc = tracker.clientActivity(&client);
            BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        }

    sleep(5);

    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << std::endl;


    ClientStateRecord record1;
    for (unsigned i=0; i<NUM; ++i)
    {
        record1.client_id = client_id_vec[i].c_str();
        record1.org = client.pOrgName;
        rc = db_client->read(&record1);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(record1.conn_ev_time > 0);
        BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
        BOOST_CHECK(record1.last_act_time > 0);
        BOOST_CHECK(record1.last_act_time >= record1.conn_ev_time);
        BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
        BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
        BOOST_CHECK(record1.version >= 1L);

    }

    std::cout << record1.toString() << std::endl;

    std::cout << "#clients=" << tracker.getNumClients() << " #Bytes=" << tracker.getSizeBytes() << std::endl;

    //=== disconnect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_HTTP_GET;
        rc = tracker.clientActivity(&client );
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(10);
    ClientStateRecord record2;
    for (unsigned i=0; i<NUM; ++i)
    {
        record2.client_id = client_id_vec[i].c_str();
        record2.org = client.pOrgName;
        rc = db_client->read(&record2);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
        BOOST_CHECK(record2.conn_ev_time > 0);
        BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
        BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
        BOOST_CHECK(record2.last_act_time > 0);
        BOOST_CHECK(record2.last_act_time > record1.last_act_time);
        BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
        BOOST_CHECK(record2.version >= 2L);
    }

    //=== stop
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_START);
    rc = tracker.stop();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_STOP);

    //=== second connect, verify DB did not change
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_Closed);
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_Closed);
    }
    sleep(5);
    for (unsigned i=0; i<NUM; ++i)
    {
        record2.client_id = client_id_vec[i].c_str();
        record2.org = client.pOrgName;
        rc = db_client->read(&record2);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
        BOOST_CHECK(record2.conn_ev_time > 0);
        BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
        BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
        BOOST_CHECK(record2.last_act_time > 0);
        BOOST_CHECK(record2.last_act_time > record1.last_act_time);
        BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
        BOOST_CHECK(record2.version >= 2L);
    }

    //=== reconfigure @ stop

    //config.pMongoURI = NULL;
    config.pMongoEndpoints = NULL;
    config.dbIOPSRateLimit = 10000;
    config.memoryLimitPercentOfTotal = 5;
    config.monitoringPolicyAppClientClass = -1;
    config.monitoringPolicyAppScaleClientClass = -1;

    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_NullPointer);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_STOP);

    //config.pMongoURI = pUri_tmp;
    config.pMongoEndpoints = pEP_tmp;

    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_STOP);

    //=== start
    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_START);

    sleep(1);

    //=== second connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(10);

    for (unsigned i=0; i<NUM; ++i)
    {
        record1.client_id = client_id_vec[i].c_str();
        record1.org = client.pOrgName;
        rc = db_client->read(&record1);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(record1.conn_ev_time > 0);
        BOOST_CHECK(record1.conn_ev_time > record2.conn_ev_time);
        BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
        BOOST_CHECK(record1.last_act_time > 0);
        BOOST_CHECK(record1.last_act_time > record2.last_act_time);
        BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
        BOOST_CHECK(record1.version >= 3L);
    }
    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_TERM);

    db.closeClient(db_client);
    db.close();
}


BOOST_AUTO_TEST_CASE( conn_act_disconn_1K_start_config )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 2000;
    config.hbTimeoutMS = 20000;
    config.numWorkerThreads = 4;

    //const char* pUri_tmp = config.pMongoURI;
    const char* pEP_tmp = config.pMongoEndpoints;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    string clientID = "d:org1:dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(5);

    //=== activity
    for (unsigned j=0; j<10; j++)
        for (unsigned i=0; i<NUM; ++i)
        {
            client.pClientID = client_id_vec[i].c_str();
            client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
            rc = tracker.clientActivity(&client);
            BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        }

    sleep(5);

    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << std::endl;


    ClientStateRecord record1;
    for (unsigned i=0; i<NUM; ++i)
    {
        record1.client_id = client_id_vec[i].c_str();
        record1.org = client.pOrgName;
        rc = db_client->read(&record1);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(record1.conn_ev_time > 0);
        BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
        BOOST_CHECK(record1.last_act_time > 0);
        BOOST_CHECK(record1.last_act_time >= record1.conn_ev_time);
        BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
        BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
        BOOST_CHECK(record1.version >= 1L);

    }

    std::cout << record1.toString() << std::endl;

    std::cout << "#clients=" << tracker.getNumClients() << " #Bytes=" << tracker.getSizeBytes() << std::endl;

    //=== disconnect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_HTTP_GET;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(10);
    ClientStateRecord record2;
    for (unsigned i=0; i<NUM; ++i)
    {
        record2.client_id = client_id_vec[i].c_str();
        record2.org = client.pOrgName;
        rc = db_client->read(&record2);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
        BOOST_CHECK(record2.conn_ev_time > 0);
        BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
        BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
        BOOST_CHECK(record2.last_act_time > 0);
        BOOST_CHECK(record2.last_act_time > record1.last_act_time);
        BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
        BOOST_CHECK(record2.version >= 2L);
    }

    //=== configure @ start, no restart

    config.dbIOPSRateLimit = 10000;
    config.memoryLimitPercentOfTotal = 5;
    config.monitoringPolicyDeviceClientClass = -1;
    config.monitoringPolicyGatewayClientClass = -1;

    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_START);

    std::cout << "configure @ start, no restart" << std::endl;

    //=== second connect, verify DB did not change
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK); //policy -1
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK); //policy -1
    }
    sleep(5);
    for (unsigned i=0; i<NUM; ++i)
    {
        record2.client_id = client_id_vec[i].c_str();
        record2.org = client.pOrgName;
        rc = db_client->read(&record2);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
        BOOST_CHECK(record2.conn_ev_time > 0);
        BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
        BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
        BOOST_CHECK(record2.last_act_time > 0);
        BOOST_CHECK(record2.last_act_time > record1.last_act_time);
        BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
        BOOST_CHECK(record2.gateway_id.empty());
        BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
        BOOST_CHECK(record2.version >= 2L);
    }

    //=== configure @ start, no restart

    config.monitoringPolicyDeviceClientClass = 10000;
    config.monitoringPolicyGatewayClientClass = 10000;

    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_START);

    std::cout << "configure @ start, no restart" << std::endl;

    //=== second connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }
    sleep(5);
    for (unsigned i=0; i<NUM; ++i)
    {
        record1.client_id = client_id_vec[i].c_str();
        record1.org = client.pOrgName;
        rc = db_client->read(&record1);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(record1.conn_ev_time > 0);
        BOOST_CHECK(record1.conn_ev_time > record2.conn_ev_time);
        BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_SUB);
        BOOST_CHECK(record1.last_act_time > 0);
        BOOST_CHECK(record1.last_act_time > record2.last_act_time);
        BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
        BOOST_CHECK(record1.version >= 3L);
    }

    //=== reconfigure @ start - restart required, will fail to start

    //config.pMongoURI = "mongodb://example.com:11111/myDB";
    config.pMongoEndpoints = NULL;

    rc = tracker.configure(&config);
    BOOST_CHECK(rc != ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_STOP);

    std::cout << "configure @ start - restart required, will fail to start" << std::endl;

    //config.pMongoURI = pUri_tmp;
    config.pMongoEndpoints = pEP_tmp;

    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_STOP);

    std::cout << "configure @ stop " << std::endl;

    //=== start
    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_START);

    sleep(1);

    std::cout << "restart " << std::endl;
    //=== third connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(10);

    for (unsigned i=0; i<NUM; ++i)
    {
        record1.client_id = client_id_vec[i].c_str();
        record1.org = client.pOrgName;
        rc = db_client->read(&record1);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(record1.conn_ev_time > 0);
        BOOST_CHECK(record1.conn_ev_time > record2.conn_ev_time);
        BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_SUB);
        BOOST_CHECK(record1.last_act_time > 0);
        BOOST_CHECK(record1.last_act_time > record2.last_act_time);
        BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
        BOOST_CHECK(record1.version >= 4L);
    }
    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(tracker.getState(),PXACT_STATE_TYPE_TERM);

    db.closeClient(db_client);
    db.close();
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_CASE( term_drain )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 2000;
    config.hbTimeoutMS = 20000;
    config.numWorkerThreads = 4;
    config.dbIOPSRateLimit = 100;
    config.terminationTimeoutMS = 10000;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    string clientID = "d:org1:dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    ClientStateRecord client_state;
    int list_size = 0;
    rc = db_client->findAllAssociatedToProxy(uid,0,2000,&client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK((unsigned)list_size < NUM );

    std::cout << "Not waiting for Q to drain: Q-size = " << tracker.getUpdateQueueSize() << " list-size=" << list_size << std::endl;

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    int list_size2 = 0;
    rc = db_client->findAllAssociatedToProxy(uid,0,2000,&client_state, &list_size2);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK(list_size2 > list_size );

    std::cout << "After: list-size=" << list_size2 << std::endl;

    db.closeClient(db_client);
    db.close();
}


BOOST_AUTO_TEST_CASE( client_expiry_in_db)
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 2000;
    config.hbTimeoutMS = 20000;
    config.numWorkerThreads = 4;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    string clientID = "d:org1:dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    std::string org(client.pOrgName);

    //=== first connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    //=== activity
    for (unsigned j = 0; j < 10; j++)
    {
        for (unsigned i = 0; i < NUM; ++i)
        {
            client.pClientID = client_id_vec[i].c_str();
            client.activityType =PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
            rc = tracker.clientActivity(&client);
            BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        }
    }

    std::cout << "waiting for Q to drain.";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done!" << std::endl;


    ClientStateRecord record1;
    for (unsigned i=0; i<NUM; ++i)
    {
        record1.client_id = client_id_vec[i].c_str();
        record1.org = client.pOrgName;
        rc = db_client->read(&record1);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(record1.conn_ev_time > 0);
        BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
        BOOST_CHECK(record1.last_act_time > 0);
        BOOST_CHECK(record1.last_act_time >= record1.conn_ev_time);
        if (record1.last_act_time < record1.conn_ev_time)
            std::cout << " oops! record1.last_act_time < record1.conn_ev_time : " << record1.last_act_time << " " << record1.conn_ev_time << std::endl;
        BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
        BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
        BOOST_CHECK(record1.version >= 1L);

    }

    //=== disconnect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType =PXACT_ACTIVITY_TYPE_HTTP_GET;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "waiting for Q to drain.";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done!" << std::endl;

    //=== delete
    std::cout << "> Delete some records..." << std::endl;

    for (unsigned i=0; i<NUM/2; ++i)
    {
        rc = db_client->removeClientID(org,  client_id_vec[i]);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(1);

    //=== connect
    for (unsigned i=0; i<NUM/4; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "waiting for Q to drain.";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done!" << std::endl;

    //=== activity
    for (unsigned i=NUM/4; i<NUM/4+10; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType =PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        std::cout << "waiting for Q to drain.";
        while (tracker.getUpdateQueueSize() > 0)
        {
            sleep(1);
            std::cout << ".";
        }
        std::cout << " done!" << std::endl;
    }

    for (unsigned i=NUM/4; i<NUM/2; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType =PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "waiting for Q to drain.";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done!" << std::endl;

    ClientStateRecord record2;
    for (unsigned i=0; i<NUM/4; ++i)
    {
        record2.client_id = client_id_vec[i].c_str();
        record2.org = client.pOrgName;
        rc = db_client->read(&record2);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(record2.conn_ev_time > 0);
        BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_SUB);
        BOOST_CHECK(record2.last_act_time > 0);
        BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
        BOOST_CHECK(record2.version >= 1L);
    }

    for (unsigned i=NUM/4; i<NUM/2; ++i)
    {
        record2.client_id = client_id_vec[i].c_str();
        record2.org = client.pOrgName;
        rc = db_client->read(&record2);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_NONE);
        BOOST_CHECK(record2.conn_ev_time == 0);
        BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT);
        BOOST_CHECK(record2.last_act_time > 0);
        BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record2.clean_s, -1);
        BOOST_CHECK_EQUAL(record2.protocol, PXACT_CONN_PROTO_TYPE_NONE);
        BOOST_CHECK(record2.version >= 1L);
    }

    for (unsigned i=NUM/2; i<NUM; ++i)
    {
        record2.client_id = client_id_vec[i].c_str();
        record2.org = client.pOrgName;
        rc = db_client->read(&record2);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
        BOOST_CHECK(record2.conn_ev_time > 0);
        BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
        BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_GET);
        BOOST_CHECK(record2.last_act_time > 0);
        BOOST_CHECK(record2.last_act_time > record1.last_act_time);
        BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
        BOOST_CHECK(record2.version >= 2L);
    }

    std::cout << record2.toString() << std::endl;

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    db.closeClient(db_client);
    db.close();
}



BOOST_AUTO_TEST_CASE( client_expiry_in_cache)
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 2000;
    config.hbTimeoutMS = 20000;
    config.numWorkerThreads = 4;

    config.monitoringPolicyDeviceClientClass = 1;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    string clientID = "d:org1:dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    std::string org(client.pOrgName);

    //=== first connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    //=== activity
    for (unsigned j = 0; j < 10; j++)
    {
        for (unsigned i = 0; i < NUM; ++i)
        {
            client.pClientID = client_id_vec[i].c_str();
            client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
            rc = tracker.clientActivity(&client);
            BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        }
    }

    std::cout << "waiting for Q to drain.";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done!" << std::endl;


    ClientStateRecord record1;
    for (unsigned i=0; i<NUM; ++i)
    {
        record1.client_id = client_id_vec[i].c_str();
        record1.org = client.pOrgName;
        rc = db_client->read(&record1);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(record1.conn_ev_time > 0);
        BOOST_CHECK_EQUAL(record1.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
        BOOST_CHECK(record1.last_act_time > 0);
        BOOST_CHECK(record1.last_act_time >= record1.conn_ev_time);
        if (record1.last_act_time < record1.conn_ev_time)
            std::cout << " oops! record1.last_act_time < record1.conn_ev_time : " << record1.last_act_time << " " << record1.conn_ev_time << std::endl;

        BOOST_CHECK(strcmp(client.pOrgName, record1.org.c_str()) == 0);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record1.client_type, client.clientType);
        BOOST_CHECK_EQUAL(record1.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record1.protocol, connInfo.protocol);
        BOOST_CHECK(record1.version >= 1L);

    }

    //=== disconnect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType =PXACT_ACTIVITY_TYPE_HTTP_GET;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "waiting for Q to drain.";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done!, now let the cache expire..." << std::endl;

    sleep(5);

    //=== delete
    std::cout << "> Delete 1/2 records..." << std::endl;

    for (unsigned i=0; i<NUM/2; ++i)
    {
        rc = db_client->removeClientID(org,  client_id_vec[i]);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(1);

    //=== connect
    for (unsigned i=0; i<NUM/4; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType =PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "connect 1st 1/4 records, waiting for Q to drain.";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done!" << std::endl;

    //=== activity
    for (unsigned i=NUM/4; i<NUM/4+10; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        std::cout << "waiting for Q to drain.";
        while (tracker.getUpdateQueueSize() > 0)
        {
            sleep(1);
            std::cout << ".";
        }
        std::cout << " done!" << std::endl;
    }

    for (unsigned i=NUM/4; i<NUM/2; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "activity on 2nd 1/4 records, waiting for Q to drain.";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done!" << std::endl;

    for (unsigned i=NUM/2; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "connect 2nd 1/2 records, waiting for Q to drain.";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done!" << std::endl;

    ClientStateRecord record2;
    for (unsigned i=0; i<NUM/4; ++i)
    {
        record2.client_id = client_id_vec[i].c_str();
        record2.org = client.pOrgName;
        rc = db_client->read(&record2);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(record2.conn_ev_time > 0);
        BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_SUB);
        BOOST_CHECK(record2.last_act_time > 0);
        BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
        BOOST_CHECK(record2.version >= 1L);
    }

    for (unsigned i=NUM/4; i<NUM/2; ++i)
    {
        record2.client_id = client_id_vec[i].c_str();
        record2.org = client.pOrgName;
        rc = db_client->read(&record2);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_NONE);
        BOOST_CHECK(record2.conn_ev_time == 0);
        BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT);
        BOOST_CHECK(record2.last_act_time > 0);
        BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record2.clean_s, -1);
        BOOST_CHECK_EQUAL(record2.protocol, PXACT_CONN_PROTO_TYPE_NONE);
        BOOST_CHECK(record2.version >= 1L);
    }

    for (unsigned i=NUM/2; i<NUM; ++i)
    {
        record2.client_id = client_id_vec[i].c_str();
        record2.org = client.pOrgName;
        rc = db_client->read(&record2);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(record2.conn_ev_time > 0);
        BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
        BOOST_CHECK(record2.last_act_time > 0);
        BOOST_CHECK(strcmp(client.pOrgName, record2.org.c_str()) == 0);
        BOOST_CHECK_EQUAL(record2.client_type, client.clientType);
        BOOST_CHECK(record1.gateway_id.empty());
        BOOST_CHECK_EQUAL(record2.clean_s, connInfo.cleanS);
        BOOST_CHECK_EQUAL(record2.protocol, connInfo.protocol);
        BOOST_CHECK(record2.version >= 3L);
    }

    std::cout << record2.toString() << std::endl;

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    db.closeClient(db_client);
    db.close();
}


BOOST_AUTO_TEST_CASE( client_cleanup)
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 100;
    config.hbTimeoutMS = 2000;
    config.numWorkerThreads = 4;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = db_client->removeAllProxyUID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid1 = tracker.getProxyUID();
    BOOST_CHECK(!uid1.empty());
    cout << "uid1: " << uid1 << endl;

    string clientID = "d:org1:type1:dev";
    string devID = "dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    std::vector<string> device_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
        device_id_vec.push_back(devID);
        device_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pDeviceType = "type1";
    client.pDeviceID = "dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    for (unsigned i=0; i<9*NUM/10; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = device_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    //=== some disconnect
    for (unsigned i=0; i<9*NUM/10; i+=2)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = device_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_SERVER;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    //=== some http
    for (unsigned i=9*NUM/10; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = device_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "waiting for Q to drain.";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done! Now start another proxy to cleanup " << std::endl;

    tracker.close(true);

    //===
    ClientStateRecord client_state;
    int list_size = 0;
    rc = db_client->findAllAssociatedToProxy(uid1,0,2000,&client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 9*NUM/10);

    rc = db_client->findAllAssociatedToProxy(string(PROXY_UID_NULL),0,2000,&client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, NUM/10);


    //===
    ActivityTracker tracker2;
    rc = tracker2.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker2.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid2 = tracker2.getProxyUID();
    BOOST_CHECK(!uid2.empty());
    cout << "uid2: " << uid1 << endl;

    sleep(10);

    rc = db_client->findAllAssociatedToProxy(uid1,0,2000,&client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);
    BOOST_CHECK_EQUAL(list_size, 0);

    rc = db_client->findAllAssociatedToProxy(string(PROXY_UID_NULL),0,2000,&client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, NUM);

    ProxyStateRecord proxy_state;
    proxy_state.proxy_uid = uid1;
    rc = db_client->read(&proxy_state);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(proxy_state.status, ProxyStateRecord::PX_STATUS_REMOVED);

    rc = tracker2.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    db.closeClient(db_client);
    db.close();
}


BOOST_AUTO_TEST_CASE( client_cleanup_slow)
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

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
    config.hbIntervalMS = 100;
    config.hbTimeoutMS = 2000;
    config.numWorkerThreads = 4;
    config.dbIOPSRateLimit = 400; //100 for cleanup
    config.removedProxyCleanupIntervalSec = 1;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = db_client->removeAllProxyUID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid1 = tracker.getProxyUID();
    BOOST_CHECK(!uid1.empty());
    cout << "uid1: " << uid1 << endl;

    string clientID = "d:org1:type1:dev";
    string devID = "dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    std::vector<string> device_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
        device_id_vec.push_back(devID);
        device_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pDeviceType = "type1";
    client.pDeviceID = "dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    for (unsigned i=0; i<9*NUM/10; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = device_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    //=== some disconnect
    for (unsigned i=0; i<9*NUM/10; i+=2)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = device_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_SERVER;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    //=== some http
    for (unsigned i=9*NUM/10; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = device_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "waiting for Q to drain.";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done! Now start another proxy to cleanup " << std::endl;

    tracker.close(true);

    //===
    ClientStateRecord client_state;
    int list_size = 0;
    rc = db_client->findAllAssociatedToProxy(uid1,0,2000,&client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, 9*NUM/10);

    rc = db_client->findAllAssociatedToProxy(string(PROXY_UID_NULL),0,2000,&client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, NUM/10);


    //===
    ActivityTracker tracker2;
    rc = tracker2.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker2.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid2 = tracker2.getProxyUID();
    BOOST_CHECK(!uid2.empty());
    cout << "uid2: " << uid1 << endl;


    for (int i=0; i<10; i++)
    {
        rc = db_client->findAllAssociatedToProxy(uid1,0,2000,&client_state, &list_size);
        if (i<5)
        {
            BOOST_CHECK_EQUAL(rc, ISMRC_OK);
            BOOST_CHECK(list_size>0);
        }
        std::cout << " cleaning... list_size=" << list_size << std::endl;
        sleep(1);

    }

    sleep(10);

    rc = db_client->findAllAssociatedToProxy(uid1,0,2000,&client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);
    BOOST_CHECK_EQUAL(list_size, 0);

    rc = db_client->findAllAssociatedToProxy(string(PROXY_UID_NULL),0,2000,&client_state, &list_size);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(list_size, NUM);

    ProxyStateRecord proxy_state;
    proxy_state.proxy_uid = uid1;
    rc = db_client->read(&proxy_state);
    BOOST_CHECK_EQUAL(rc, ISMRC_NotFound);


    rc = tracker2.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    db.closeClient(db_client);
    db.close();
}

BOOST_AUTO_TEST_CASE( memory_managment )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ismPXActivity_ConfigActivityDB_t config;
    ism_pxactivity_ConfigActivityDB_Init(&config);
    config.pDBType = "MongoDB";
    //config.pMongoURI = ServerProxyUnitTestInit::mongodbURI.c_str();
    config.pMongoUser = ServerProxyUnitTestInit::mongodbUSER.c_str();
    config.pMongoPassword = ServerProxyUnitTestInit::mongodbPWD.c_str();
    config.memoryLimitPercentOfTotal = 0.01;
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
    config.hbIntervalMS = 2000;
    config.hbTimeoutMS = 20000;
    config.numWorkerThreads = 4;
    config.numBanksPerWorkerThread = 4;

    cout << "config: " << toString(&config) << endl;

    //========================

    ActivityTracker tracker;
    int rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    string clientID = "d:org2:type2:dev";
    unsigned NUM = 100000;
    std::vector<string> client_id_vec;
    for (unsigned i = 0; i < NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(NUM+i));
    }

    ismPXACT_Client_t client;
    memset(&client, 0, sizeof(ismPXACT_Client_t));
    client.pClientID = NULL;
    client.pOrgName = "org2";
    client.pDeviceType = "type2";
    client.pDeviceID = NULL;
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    //=== activity ===
    std::size_t NUM1 = 256;
    for (unsigned i = 0; i < NUM1; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = client_id_vec[i].c_str();
        client.pDeviceID += 13;
        client.activityType = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(1);
    double r = static_cast<double>(tracker.getSizeBytes()) / tracker.getMemoryLimitBytes();
    cout << r << " = " << tracker.getSizeBytes() << " / " << tracker.getMemoryLimitBytes() << endl;
    ActivityStats stats = tracker.getStats();
    cout << stats.toString() << endl;
    BOOST_CHECK(stats.num_evict_clients == 0);

    std::size_t NUM2 = static_cast<size_t>(0.9 * tracker.getNumClients() / r);
    for (unsigned i = 0; i < NUM2; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = client_id_vec[i].c_str();
        client.pDeviceID += 13;
        client.activityType =PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(3);
    r = static_cast<double>(tracker.getSizeBytes()) / tracker.getMemoryLimitBytes();
    cout << "NUM2=" << NUM2 << " " << r << " = " << tracker.getSizeBytes() << " / " << tracker.getMemoryLimitBytes() << endl;
    stats = tracker.getStats();
    cout << stats.toString() << endl;
    BOOST_CHECK(stats.num_evict_clients == 0);
    BOOST_CHECK(stats.num_clients + stats.num_evict_clients == stats.num_new_clients);
    BOOST_CHECK((uint64_t)stats.num_new_clients == NUM2);

    NUM2 = static_cast<size_t>(1.1 * tracker.getNumClients() / r);
    for (unsigned i = 0; i < NUM2; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = client_id_vec[i].c_str();
        client.pDeviceID += 13;
        client.activityType = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    sleep(3);
    r = static_cast<double>(tracker.getSizeBytes()) / tracker.getMemoryLimitBytes();
    cout << "NUM2=" << NUM2 << " " << r << " = " << tracker.getSizeBytes() << " / " << tracker.getMemoryLimitBytes() << endl;
    stats = tracker.getStats();
    cout << stats.toString() << endl;
    BOOST_CHECK(stats.num_evict_clients > 0);
    BOOST_CHECK(stats.num_clients + stats.num_evict_clients == stats.num_new_clients);
    BOOST_CHECK((uint64_t)stats.num_new_clients == NUM2);

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

}


BOOST_AUTO_TEST_SUITE( DBFailure_suite)

BOOST_AUTO_TEST_CASE( db_failure)
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    if (ServerProxyUnitTestInit::mongodbStartCommand.empty() || ServerProxyUnitTestInit::mongodbStopCommand.empty())
    {
        std::cerr << "Warning: mongodbStartCommand OR mongodbStopCommand is empty, cannot run test case!" << std::endl;
        //e.g. "/home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/stop_MongoDB.sh"
        //e.g. "/home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/start_MongoDB.sh"
        BOOST_CHECK(!ServerProxyUnitTestInit::mongodbStartCommand.empty());
        BOOST_CHECK(!ServerProxyUnitTestInit::mongodbStopCommand.empty());
        return;
    }

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
    config.hbIntervalMS = 20000;
    config.hbTimeoutMS = 200000;
    config.numWorkerThreads = 4;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = db_client->removeAllProxyUID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid1 = tracker.getProxyUID();
    BOOST_CHECK(!uid1.empty());
    cout << "uid1: " << uid1 << endl;

    string clientID = "d:org1:type1:dev";
    string devID = "dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    std::vector<string> device_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
        device_id_vec.push_back(devID);
        device_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pDeviceType = "type1";
    client.pDeviceID = "dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = device_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    ismPXACT_Stats_t stats;
    tracker.getStats(&stats);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;
    std::cout << ">>> Please shutdown DB now... " << std::endl;

    system(ServerProxyUnitTestInit::mongodbStopCommand.c_str());

    while (rc == ISMRC_OK)
    {
        for (unsigned i=0; i<NUM; ++i)
        {
            client.pClientID = client_id_vec[i].c_str();
            client.pDeviceID = device_id_vec[i].c_str();
            client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
            rc = tracker.clientActivity(&client);
            if (rc != ISMRC_OK) break;
        }
        usleep(100);
    }

    std::cout << ">>> Done - error detected - rc=" << rc << std::endl;
    sleep(1);

    BOOST_CHECK(tracker.getState() == PXACT_STATE_TYPE_ERROR);
    std::cout << ">>> Done - state=" << tracker.getState() << std::endl;

    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error > 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;
    sleep(1);
    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error == 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;

    rc = tracker.close(true);
    std::cout << ">>> Done close - rc=" << rc << std::endl;

    db.closeClient(db_client);
    db.close();

    std::cout << ">>> Done close all " << std::endl;

    std::cout << ">>> Please start DB now... " << std::endl;

    system(ServerProxyUnitTestInit::mongodbStartCommand.c_str());
    sleep(1);
}


BOOST_AUTO_TEST_CASE( db_failure_hb)
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");
    if (ServerProxyUnitTestInit::mongodbStartCommand.empty() || ServerProxyUnitTestInit::mongodbStopCommand.empty())
    {
        std::cerr << "Warning: mongodbStartCommand OR mongodbStopCommand is empty, cannot run test case!" << std::endl;
        BOOST_CHECK(!ServerProxyUnitTestInit::mongodbStartCommand.empty());
        BOOST_CHECK(!ServerProxyUnitTestInit::mongodbStopCommand.empty());
        return;
    }

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
    config.hbIntervalMS = 100;
    config.hbTimeoutMS = 2000;
    config.numWorkerThreads = 4;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = db_client->removeAllProxyUID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid1 = tracker.getProxyUID();
    BOOST_CHECK(!uid1.empty());
    cout << "uid1: " << uid1 << endl;

    string clientID = "d:org1:type1:dev";
    string devID = "dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    std::vector<string> device_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
        device_id_vec.push_back(devID);
        device_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pDeviceType = "type1";
    client.pDeviceID = "dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = device_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "waiting for Q to drain... ";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done! " << std::endl;


    ismPXACT_Stats_t stats;
    tracker.getStats(&stats);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;
    std::cout << ">>> Please shutdown DB now... " << std::endl;

    system(ServerProxyUnitTestInit::mongodbStopCommand.c_str());

    while (tracker.getState() != PXACT_STATE_TYPE_ERROR)
    {
        usleep(1000);
    }

    BOOST_CHECK(tracker.getState() == PXACT_STATE_TYPE_ERROR);
    std::cout << ">>> Done - state=" << tracker.getState() << std::endl;

    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error > 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;
    sleep(1);
    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error == 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;

    rc = tracker.close(true);
    std::cout << ">>> Done close - rc=" << rc << std::endl;

    db.closeClient(db_client);
    db.close();

    std::cout << ">>> Done close all " << std::endl;

    std::cout << ">>> Please start DB now... " << std::endl;

    system(ServerProxyUnitTestInit::mongodbStartCommand.c_str());
    sleep(1);
}


BOOST_AUTO_TEST_CASE( db_failure_restart)
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");
    if (ServerProxyUnitTestInit::mongodbStartCommand.empty() || ServerProxyUnitTestInit::mongodbStopCommand.empty())
    {
        std::cerr << "Warning: mongodbStartCommand OR mongodbStopCommand is empty, cannot run test case!" << std::endl;
        BOOST_CHECK(!ServerProxyUnitTestInit::mongodbStartCommand.empty());
        BOOST_CHECK(!ServerProxyUnitTestInit::mongodbStopCommand.empty());
        return;
    }

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
    config.hbIntervalMS = 100;
    config.hbTimeoutMS = 2000;
    config.numWorkerThreads = 4;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = db_client->removeAllProxyUID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid1 = tracker.getProxyUID();
    BOOST_CHECK(!uid1.empty());
    cout << "uid1: " << uid1 << endl;

    string clientID = "d:org1:type1:dev";
    string devID = "dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    std::vector<string> device_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
        device_id_vec.push_back(devID);
        device_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pDeviceType = "type1";
    client.pDeviceID = "dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = device_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "waiting for Q to drain... ";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done! " << std::endl;

    //=== stop 1 ===
    ismPXACT_Stats_t stats;
    tracker.getStats(&stats);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;
    std::cout << ">>> Please shutdown DB now... " << std::endl;

    system(ServerProxyUnitTestInit::mongodbStopCommand.c_str());

    while (tracker.getState() != PXACT_STATE_TYPE_ERROR)
    {
        usleep(1000);
    }

    BOOST_CHECK(tracker.getState() == PXACT_STATE_TYPE_ERROR);
    std::cout << ">>> Done - state=" << tracker.getState() << std::endl;

    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error > 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;
    sleep(1);
    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error == 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;

    //=== start 2 ===
    std::cout << ">>> Please start DB now... " << std::endl;

    system(ServerProxyUnitTestInit::mongodbStartCommand.c_str());
    sleep(5);
    while (tracker.getState() != PXACT_STATE_TYPE_START)
    {
        usleep(1000);
    }
    sleep(5);
    BOOST_CHECK(tracker.getState() == PXACT_STATE_TYPE_START);
    std::cout << ">>> Done - state=" << tracker.getState() << std::endl;
    tracker.getStats(&stats);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;

    //=== stop 3 ===
    std::cout << ">>> Please shutdown DB now... " << std::endl;
    system(ServerProxyUnitTestInit::mongodbStopCommand.c_str());

    while (tracker.getState() != PXACT_STATE_TYPE_ERROR)
    {
        usleep(1000);
    }

    BOOST_CHECK(tracker.getState() == PXACT_STATE_TYPE_ERROR);
    std::cout << ">>> Done - state=" << tracker.getState() << std::endl;

    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error > 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;
    sleep(1);
    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error == 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;


    //=== start 4 ===
    std::cout << ">>> Please start DB now... " << std::endl;
    system(ServerProxyUnitTestInit::mongodbStartCommand.c_str());
    sleep(5);
    while (tracker.getState() != PXACT_STATE_TYPE_START)
    {
        usleep(1000);
    }
    sleep(5);
    BOOST_CHECK(tracker.getState() == PXACT_STATE_TYPE_START);
    std::cout << ">>> Done - state=" << tracker.getState() << std::endl;
    tracker.getStats(&stats);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;

    //=== stop 5 ===
    std::cout << ">>> Please shutdown DB now... " << std::endl;
    system(ServerProxyUnitTestInit::mongodbStopCommand.c_str());

    while (tracker.getState() != PXACT_STATE_TYPE_ERROR)
    {
        usleep(1000);
    }

    BOOST_CHECK(tracker.getState() == PXACT_STATE_TYPE_ERROR);
    std::cout << ">>> Done - state=" << tracker.getState() << std::endl;

    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error > 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;
    sleep(1);
    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error == 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;


    //====
    rc = tracker.close(true);
    std::cout << ">>> Done close - rc=" << rc << std::endl;

    db.closeClient(db_client);
    db.close();

    std::cout << ">>> Done close all " << std::endl;

    //=== start 6 ===
    std::cout << ">>> Please start DB now... " << std::endl;
    system(ServerProxyUnitTestInit::mongodbStartCommand.c_str());
    sleep(1);
}



BOOST_AUTO_TEST_CASE( db_failure_hb_stop_start)
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");
    if (ServerProxyUnitTestInit::mongodbStartCommand.empty() || ServerProxyUnitTestInit::mongodbStopCommand.empty())
    {
        std::cerr << "Warning: mongodbStartCommand OR mongodbStopCommand is empty, cannot run test case!" << std::endl;
        BOOST_CHECK(!ServerProxyUnitTestInit::mongodbStartCommand.empty());
        BOOST_CHECK(!ServerProxyUnitTestInit::mongodbStopCommand.empty());
        return;
    }

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
    config.hbIntervalMS = 100;
    config.hbTimeoutMS = 2000;
    config.numWorkerThreads = 4;

    cout << "config: " << toString(&config) << endl;

    //=== DB client for test ===
    ActivityDB db;
    int rc = db.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = db.connect();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    ActivityDBClient_SPtr db_client = db.getClient();
    BOOST_CHECK(db_client);

    rc = db_client->removeAllClientID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = db_client->removeAllProxyUID();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    //========================

    ActivityTracker tracker;
    rc = tracker.configure(&config);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = tracker.start();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    string uid1 = tracker.getProxyUID();
    BOOST_CHECK(!uid1.empty());
    cout << "uid1: " << uid1 << endl;

    string clientID = "d:org1:type1:dev";
    string devID = "dev";
    unsigned NUM = 1000;
    std::vector<string> client_id_vec;
    std::vector<string> device_id_vec;
    for (unsigned i=0; i< NUM; ++i)
    {
        client_id_vec.push_back(clientID);
        client_id_vec.back().append(to_string(i));
        device_id_vec.push_back(devID);
        device_id_vec.back().append(to_string(i));
    }

    ismPXACT_Client_t client;
    memset(&client,0,sizeof(ismPXACT_Client_t));
    client.pClientID = "d:org1:dev1";
    client.pDeviceType = "type1";
    client.pDeviceID = "dev1";
    client.pOrgName = "org1";
    client.clientType = PXACT_CLIENT_TYPE_DEVICE;
    client.pGateWayID = NULL;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    //=== first connect
    for (unsigned i=0; i<NUM; ++i)
    {
        client.pClientID = client_id_vec[i].c_str();
        client.pDeviceID = device_id_vec[i].c_str();
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = tracker.clientConnectivity(&client, &connInfo);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
        client.activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
        rc = tracker.clientActivity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    }

    std::cout << "waiting for Q to drain... ";
    while (tracker.getUpdateQueueSize() > 0)
    {
        sleep(1);
        std::cout << ".";
    }
    std::cout << " done! " << std::endl;


    ismPXACT_Stats_t stats;
    tracker.getStats(&stats);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;

    //=== stop 1 ===
    std::cout << ">>> Please shutdown DB now... " << std::endl;

    system(ServerProxyUnitTestInit::mongodbStopCommand.c_str());

    while (tracker.getState() != PXACT_STATE_TYPE_ERROR)
    {
        usleep(1000);
    }

    BOOST_CHECK(tracker.getState() == PXACT_STATE_TYPE_ERROR);
    std::cout << ">>> Done - state=" << tracker.getState() << std::endl;

    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error > 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;
    sleep(1);
    tracker.getStats(&stats);
    BOOST_CHECK(stats.num_db_error == 0);
    std::cout << ">>> Stats = " << toString(&stats) << std::endl;

    //=== stop tracker
    rc = tracker.start();
    BOOST_CHECK(rc != ISMRC_OK);
    BOOST_CHECK(tracker.getState() == PXACT_STATE_TYPE_ERROR);

    rc = tracker.stop();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK(tracker.getState() == PXACT_STATE_TYPE_STOP);

    std::cout << ">>> Please start DB now... " << std::endl;

    system(ServerProxyUnitTestInit::mongodbStartCommand.c_str());

    sleep(5);
    BOOST_CHECK(tracker.getState() == PXACT_STATE_TYPE_STOP);

    //===
    rc = tracker.close(true);
    std::cout << ">>> Done close - rc=" << rc << std::endl;

    db.closeClient(db_client);
    db.close();

    std::cout << ">>> Done close all " << std::endl;


}


BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
