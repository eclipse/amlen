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

extern "C"
{
    extern void ism_pxactivity_test_destroy();
}

BOOST_AUTO_TEST_SUITE( CAPI_suite )

BOOST_AUTO_TEST_CASE( start_close )
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

	ismPXACT_Stats_t stats;

	int rc = ism_pxactivity_init();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	BOOST_CHECK_EQUAL(ism_pxactivity_is_started(), 0);

	rc = ism_pxactivity_Stats_get(&stats);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_INIT);
	BOOST_CHECK_EQUAL(stats.num_clients, 0);
	BOOST_CHECK_EQUAL(stats.num_activity, 0);
	BOOST_CHECK_EQUAL(stats.num_connectivity, 0);

	rc = ism_pxactivity_init();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	BOOST_CHECK_EQUAL(ism_pxactivity_is_started(), 0);
	rc = ism_pxactivity_ConfigActivityDB_Set(&config);
	BOOST_CHECK_EQUAL(ism_pxactivity_is_started(), 0);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	rc = ism_pxactivity_start();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	BOOST_CHECK_EQUAL(ism_pxactivity_is_started(), 1);

	rc = ism_pxactivity_Stats_get(&stats);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_START);
    BOOST_CHECK_EQUAL(stats.num_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_activity, 0);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 0);

	sleep(2);
    BOOST_CHECK_EQUAL(ism_pxactivity_is_started(), 1);
	rc = ism_pxactivity_term();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);

	rc = ism_pxactivity_Stats_get(&stats);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_TERM);
    BOOST_CHECK_EQUAL(stats.num_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_activity, 0);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 0);

    BOOST_CHECK_EQUAL(ism_pxactivity_is_started(), 0);
    ism_pxactivity_test_destroy();
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

	rc = ism_pxactivity_init();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	rc = ism_pxactivity_ConfigActivityDB_Set(&config);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	rc = ism_pxactivity_start();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);

	ismPXACT_Client_t client;
	memset(&client,0,sizeof(ismPXACT_Client_t));
	client.pClientID = "d:org1:dev1";
	client.pOrgName = "org1";
	client.clientType = PXACT_CLIENT_TYPE_DEVICE;
	client.pGateWayID = NULL;
	client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;


	//=== first connect
	rc = ism_pxactivity_Client_Connectivity(&client, &connInfo);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);

	sleep(1);
	ClientStateRecord record1;
	record1.client_id = client.pClientID;
	record1.org = client.pOrgName;
	rc = db_client->read(&record1);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
	BOOST_CHECK(record1.conn_ev_time > 0);
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
	client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
	rc = ism_pxactivity_Client_Connectivity(&client, &connInfo);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);

	sleep(1);
	ClientStateRecord record2;
	record2.client_id = client.pClientID;
	record2.org = client.pOrgName;
	rc = db_client->read(&record2);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
	BOOST_CHECK(record2.conn_ev_time > 0);
	BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
	BOOST_CHECK_EQUAL(record2.last_act_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
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
	client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
	connInfo.expiryIntervalSec = 60;
	connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v5;

	rc = ism_pxactivity_Client_Connectivity(&client, &connInfo);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	sleep(1);
	rc = db_client->read(&record1);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	BOOST_CHECK_EQUAL(record1.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_CONN);
	BOOST_CHECK(record1.conn_ev_time > record2.conn_ev_time);
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
	client.activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
	connInfo.expiryIntervalSec = 5;
	connInfo.reasonCode = 10;
	rc = ism_pxactivity_Client_Connectivity(&client, &connInfo);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);

	sleep(1);
	rc = db_client->read(&record2);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	BOOST_CHECK_EQUAL(record2.conn_ev_type, PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
	BOOST_CHECK(record2.conn_ev_time > 0);
	BOOST_CHECK(record2.conn_ev_time > record1.conn_ev_time);
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

	rc = ism_pxactivity_term();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);

	db.closeClient(db_client);
	db.close();
    ism_pxactivity_test_destroy();
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

	rc = ism_pxactivity_init();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	rc = ism_pxactivity_ConfigActivityDB_Set(&config);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	rc = ism_pxactivity_start();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);


	ismPXACT_Client_t client;
	memset(&client,0,sizeof(ismPXACT_Client_t));
	client.pClientID = "d:org1:dev1";
	client.pOrgName = "org1";
	client.clientType = PXACT_CLIENT_TYPE_DEVICE;
	client.pGateWayID = NULL;
	client.activityType = PXACT_ACTIVITY_TYPE_HTTP_GET;

	//=== 1 activity
	rc = ism_pxactivity_Client_Activity(&client);
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
	rc = ism_pxactivity_Client_Activity(&client);
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
	client.activityType = PXACT_ACTIVITY_TYPE_HTTP_GET;
	client.pGateWayID = "g:org1:gw1";
	rc = ism_pxactivity_Client_Activity(&client);
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

	rc = ism_pxactivity_term();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);

	db.closeClient(db_client);
	db.close();
    ism_pxactivity_test_destroy();
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

	rc = ism_pxactivity_Stats_get(&stats);
	BOOST_CHECK_EQUAL(rc, ISMRC_NullPointer);

	 rc = ism_pxactivity_init();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = ism_pxactivity_Stats_get(&stats);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_INIT);
    BOOST_CHECK_EQUAL(stats.num_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_activity, 0);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 0);

	rc = ism_pxactivity_ConfigActivityDB_Set(&config);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = ism_pxactivity_Stats_get(&stats);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_CONFIG);
    BOOST_CHECK_EQUAL(stats.num_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_activity, 0);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 0);

	rc = ism_pxactivity_start();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = ism_pxactivity_Stats_get(&stats);
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

	ismPXACT_Client_t client;
	memset(&client,0,sizeof(ismPXACT_Client_t));
	client.pClientID = "d:org1:dev1";
	client.pOrgName = "org1";
	client.clientType = PXACT_CLIENT_TYPE_DEVICE;
	client.pGateWayID = NULL;
	client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

	//=== first connect
	rc = ism_pxactivity_Client_Connectivity(&client, &connInfo);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
	for (int i=0; i<100; ++i)
	{
		rc = ism_pxactivity_Client_Activity(&client);
		BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	}

	sleep(1);

    rc = ism_pxactivity_Stats_get(&stats);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_START);
    BOOST_CHECK_EQUAL(stats.num_clients, 1);
    BOOST_CHECK_EQUAL(stats.num_activity, 100);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 1);
    BOOST_CHECK_EQUAL(stats.num_new_clients, 1);
    BOOST_CHECK_EQUAL(stats.num_evict_clients, 0);
    BOOST_CHECK(stats.num_db_insert > 0); //the UID
    BOOST_CHECK(stats.num_db_read > 0);
    BOOST_CHECK(stats.num_db_update >= 0);
    BOOST_CHECK(stats.num_db_bulk == 0);
    BOOST_CHECK(stats.num_db_bulk_update == 0);
    BOOST_CHECK(stats.num_db_delete == 0);
    BOOST_CHECK(stats.num_db_error == 0);
    BOOST_CHECK(stats.avg_db_read_latency_ms >= 0);
    BOOST_CHECK(stats.avg_conflation_delay_ms >= 0);
    BOOST_CHECK(stats.avg_conflation_factor >= 0);
    BOOST_CHECK(stats.memory_bytes > 0);

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
	rc = ism_pxactivity_Client_Connectivity(&client, &connInfo);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	client.pGateWayID = "g:org1:gw1";
	client.activityType = PXACT_ACTIVITY_TYPE_HTTP_GET;
	for (int i=0; i<100; ++i)
	{
		rc = ism_pxactivity_Client_Activity(&client);
		BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	}

	sleep(1);

    rc = ism_pxactivity_Stats_get(&stats);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_START);
    BOOST_CHECK_EQUAL(stats.num_clients, 1);
    BOOST_CHECK_EQUAL(stats.num_activity, 100);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 1);
    BOOST_CHECK_EQUAL(stats.num_new_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_evict_clients, 0);
    BOOST_CHECK(stats.num_db_insert == 0);
    BOOST_CHECK(stats.num_db_read > 0);
    BOOST_CHECK(stats.num_db_update > 0);
    BOOST_CHECK(stats.num_db_bulk >= 0);
    BOOST_CHECK(stats.num_db_bulk_update >= 0);
    BOOST_CHECK(stats.num_db_delete == 0);
    BOOST_CHECK(stats.num_db_error == 0);
    BOOST_CHECK(stats.avg_db_read_latency_ms >= 0);
    BOOST_CHECK(stats.avg_conflation_delay_ms >= 0);
    BOOST_CHECK(stats.avg_conflation_factor >= 0);
    BOOST_CHECK(stats.memory_bytes > 0);

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


    rc = ism_pxactivity_stop();
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = ism_pxactivity_Stats_get(&stats);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_STOP);
    BOOST_CHECK_EQUAL(stats.num_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_activity, 0);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 0);
    BOOST_CHECK_EQUAL(stats.num_new_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_evict_clients, 0);
    BOOST_CHECK(stats.num_db_insert == 0);
    BOOST_CHECK(stats.num_db_read == 0);
    BOOST_CHECK(stats.num_db_update == 0);
    BOOST_CHECK(stats.num_db_bulk == 0);
    BOOST_CHECK(stats.num_db_bulk_update == 0);
    BOOST_CHECK(stats.num_db_delete == 0);
    BOOST_CHECK(stats.num_db_error == 0);
    BOOST_CHECK(stats.avg_db_read_latency_ms == 0);
    BOOST_CHECK(stats.avg_conflation_delay_ms == 0);
    BOOST_CHECK(stats.avg_conflation_factor == 0);
    BOOST_CHECK(stats.memory_bytes == 0);

    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    rc = ism_pxactivity_Client_Connectivity(&client, &connInfo);
    BOOST_CHECK_EQUAL(rc, ISMRC_Closed);
    client.activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
    for (int i=0; i<100; ++i)
    {
        rc = ism_pxactivity_Client_Activity(&client);
        BOOST_CHECK_EQUAL(rc, ISMRC_Closed);
    }

    rc = ism_pxactivity_Stats_get(&stats);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_STOP);
    BOOST_CHECK_EQUAL(stats.num_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_activity, 0);
    BOOST_CHECK_EQUAL(stats.num_connectivity, 0);
    BOOST_CHECK_EQUAL(stats.num_new_clients, 0);
    BOOST_CHECK_EQUAL(stats.num_evict_clients, 0);
    BOOST_CHECK(stats.num_db_insert == 0);
    BOOST_CHECK(stats.num_db_read == 0);
    BOOST_CHECK(stats.num_db_update == 0);
    BOOST_CHECK(stats.num_db_bulk == 0);
    BOOST_CHECK(stats.num_db_bulk_update == 0);
    BOOST_CHECK(stats.num_db_delete == 0);
    BOOST_CHECK(stats.num_db_error == 0);
    BOOST_CHECK(stats.avg_db_read_latency_ms == 0);
    BOOST_CHECK(stats.avg_conflation_delay_ms == 0);
    BOOST_CHECK(stats.avg_conflation_factor == 0);
    BOOST_CHECK(stats.memory_bytes == 0);

	rc = ism_pxactivity_term();
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);

	rc = ism_pxactivity_Stats_get(&stats);
	BOOST_CHECK_EQUAL(rc, ISMRC_OK);
	BOOST_CHECK_EQUAL(stats.activity_tracking_state, PXACT_STATE_TYPE_TERM);
	BOOST_CHECK_EQUAL(stats.num_clients, 0);
	BOOST_CHECK_EQUAL(stats.num_activity, 0);
	BOOST_CHECK_EQUAL(stats.num_connectivity, 0);
	BOOST_CHECK_EQUAL(stats.num_clients, 0);
	BOOST_CHECK_EQUAL(stats.num_activity, 0);
	BOOST_CHECK_EQUAL(stats.num_connectivity, 0);
	BOOST_CHECK_EQUAL(stats.num_new_clients, 0);
	BOOST_CHECK_EQUAL(stats.num_evict_clients, 0);
	BOOST_CHECK(stats.num_db_insert == 0);
	BOOST_CHECK(stats.num_db_read == 0);
	BOOST_CHECK(stats.num_db_update == 0);
	BOOST_CHECK(stats.num_db_bulk == 0);
	BOOST_CHECK(stats.num_db_bulk_update == 0);
	BOOST_CHECK(stats.num_db_delete == 0);
	BOOST_CHECK(stats.num_db_error == 0);
	BOOST_CHECK(stats.avg_db_read_latency_ms == 0);
	BOOST_CHECK(stats.avg_conflation_delay_ms == 0);
	BOOST_CHECK(stats.avg_conflation_factor == 0);
	BOOST_CHECK(stats.memory_bytes == 0);

	db.closeClient(db_client);
	db.close();
    ism_pxactivity_test_destroy();
}

BOOST_AUTO_TEST_SUITE_END()
