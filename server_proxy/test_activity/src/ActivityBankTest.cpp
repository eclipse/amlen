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
 * ActivityBankTest.cpp
 *  Created on: Nov 9, 2017
 */



#include <pthread.h>
#include <memory>
#include <string>
#include <chrono>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include "pxactivity.h"
#include "ActivityBank.h"
#include "ActivityTracker.h"

#include "ServerProxyUnitTestInit.h"


BOOST_AUTO_TEST_SUITE( Bank_suite )

BOOST_AUTO_TEST_CASE( lru )
{
	using namespace std;
	using namespace ism_proxy;

	using LastActive = std::pair<double,const char*>;
	using LRU_Heap = std::priority_queue<LastActive>;

	LRU_Heap heap;

	heap.push(make_pair(1.0, "a"));
	heap.push(make_pair(2.0, "b1"));
	heap.push(make_pair(2.0, "b2"));
	heap.push(make_pair(3.0, "c"));
	heap.push(make_pair(4.0, "d"));
	heap.push(make_pair(5.0, "e"));
	heap.push(make_pair(6.0, "f"));


	while (!heap.empty())
	{
		LRU_Heap::const_reference top = heap.top();
		cout << top.first << " " << top.second << endl;
		heap.pop();
	}

}

BOOST_AUTO_TEST_CASE( create )
{
	using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityBank* bank = new ActivityBank(0);
    BOOST_CHECK(bank);
    BOOST_CHECK(bank->getNumClients() == 0);
    BOOST_CHECK(bank->getSizeBytes() == 0);
    BOOST_CHECK(bank->getUpdateQueueSize() == 0);

    delete bank;

}

BOOST_AUTO_TEST_CASE( one_client_connect )
{
	using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityBank* bank = new ActivityBank(0);
    BOOST_CHECK(bank);
    BOOST_CHECK(bank->getNumClients() == 0);
    BOOST_CHECK(bank->getSizeBytes() == 0);
    BOOST_CHECK(bank->getUpdateQueueSize() == 0);

    ClientUpdateVector updateVec;
    for (int i=0; i<10; ++i)
    {
    	updateVec.push_back(ClientUpdateRecord_SPtr(new ClientUpdateRecord));
    }


    ismPXACT_Client_t* pClient = (ismPXACT_Client_t*)calloc(1,sizeof(ismPXACT_Client_t));
    pClient->pClientID = "d:org1:type1:dev1";
    pClient->pOrgName = "org1";
    pClient->pDeviceType = "type1";
    pClient->pDeviceID = "dev1";

    pClient->clientType = PXACT_CLIENT_TYPE_DEVICE;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    int rc = ISMRC_OK;
    pClient->activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    rc = bank->clientConnectivity(pClient, &connInfo);
    BOOST_CHECK(rc == ISMRC_OK);


    BOOST_CHECK(bank->getNumClients() == 1);
    BOOST_CHECK(bank->getUpdateQueueSize() == 1);

    uint32_t numUpdates = 0;
    rc = bank->getUpdateBatch(updateVec, numUpdates);
    BOOST_CHECK(rc == ISMRC_OK);
    BOOST_CHECK(numUpdates == 1);
    std::cout << "update: " << updateVec[0]->toString() << std::endl;

    BOOST_CHECK(strcmp(updateVec[0]->client_id->c_str(), pClient->pClientID)==0);
    BOOST_CHECK(strcmp(updateVec[0]->org->c_str(), pClient->pOrgName)==0);
    BOOST_CHECK(strcmp(updateVec[0]->device_type->c_str(), pClient->pDeviceType)==0);
    BOOST_CHECK(strcmp(updateVec[0]->device_id->c_str(), pClient->pDeviceID)==0);
    BOOST_CHECK(updateVec[0]->client_type == pClient->clientType);
    BOOST_CHECK(updateVec[0]->target_server == connInfo.targetServerType);
    BOOST_CHECK(updateVec[0]->conn_ev_type == PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(updateVec[0]->gateway_id->empty());
    BOOST_CHECK(updateVec[0]->last_act_type == PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(updateVec[0]->dirty_flags == (ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD));

    BOOST_CHECK(updateVec[0]->queuing_timestamp > 0);
    BOOST_CHECK(updateVec[0]->num_updates > 0);

    BOOST_CHECK(bank->getNumClients() == 1);
    BOOST_CHECK(bank->getUpdateQueueSize() == 0);

    free(pClient);
    delete bank;
}


BOOST_AUTO_TEST_CASE( one_client_connect2 )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	ActivityBank* bank = new ActivityBank(0);
	BOOST_CHECK(bank);
	BOOST_CHECK(bank->getNumClients() == 0);
	BOOST_CHECK(bank->getSizeBytes() == 0);
	BOOST_CHECK(bank->getUpdateQueueSize() == 0);

	ClientUpdateVector updateVec;
	for (int i = 0; i < 10; ++i) {
		updateVec.push_back(ClientUpdateRecord_SPtr(new ClientUpdateRecord));
	}

	ismPXACT_Client_t* pClient = (ismPXACT_Client_t*) calloc(1,
			sizeof(ismPXACT_Client_t));
	pClient->pClientID = "d:org1:client1";
	pClient->pOrgName = "org1";
	pClient->clientType = PXACT_CLIENT_TYPE_DEVICE;

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

	int rc = ISMRC_OK;
	pClient->activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
	rc = bank->clientConnectivity(pClient, &connInfo);
	BOOST_CHECK(rc == ISMRC_OK);

	pClient->activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
	rc = bank->clientActivity(pClient);
	BOOST_CHECK(rc == ISMRC_OK);


    BOOST_CHECK(bank->getNumClients() == 1);
    BOOST_CHECK(bank->getUpdateQueueSize() == 1);

    uint32_t numUpdates = 0;
    rc = bank->getUpdateBatch(updateVec, numUpdates);
    BOOST_CHECK(rc == ISMRC_OK);
    BOOST_CHECK(numUpdates == 1);
    std::cout << "update: " << updateVec[0]->toString() << std::endl;

    BOOST_CHECK(
            strcmp(updateVec[0]->client_id->c_str(), pClient->pClientID) == 0);
    BOOST_CHECK(strcmp(updateVec[0]->org->c_str(), pClient->pOrgName) == 0);
    BOOST_CHECK(strcmp(updateVec[0]->device_type->c_str(), "")==0);
    BOOST_CHECK(strcmp(updateVec[0]->device_id->c_str(), "")==0);

    BOOST_CHECK(updateVec[0]->client_type == pClient->clientType);
    BOOST_CHECK(updateVec[0]->target_server == connInfo.targetServerType);
    BOOST_CHECK(updateVec[0]->conn_ev_type == PXACT_ACTIVITY_TYPE_MQTT_CONN);
    BOOST_CHECK(updateVec[0]->gateway_id);
    BOOST_CHECK(
            updateVec[0]->last_act_type == PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
    BOOST_CHECK(
            updateVec[0]->dirty_flags
                    == (ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT
                            | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT
                            | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD));

    BOOST_CHECK(updateVec[0]->queuing_timestamp > 0);
    BOOST_CHECK(updateVec[0]->queuing_timestamp < updateVec[0]->update_timestamp);
    BOOST_CHECK(updateVec[0]->num_updates == 2);

    BOOST_CHECK(bank->getNumClients() == 1);
    BOOST_CHECK(bank->getUpdateQueueSize() == 0);


	pClient->activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT;
	rc = bank->clientConnectivity(pClient, &connInfo);
	BOOST_CHECK(rc == ISMRC_OK);

	BOOST_CHECK(bank->getNumClients() == 1);
	BOOST_CHECK(bank->getUpdateQueueSize() == 1);

	rc = bank->getUpdateBatch(updateVec, numUpdates);
	BOOST_CHECK(rc == ISMRC_OK);
	BOOST_CHECK(numUpdates == 1);
	std::cout << "update: " << updateVec[0]->toString() << std::endl;

	BOOST_CHECK(
			strcmp(updateVec[0]->client_id->c_str(), pClient->pClientID) == 0);
	BOOST_CHECK(strcmp(updateVec[0]->org->c_str(), pClient->pOrgName) == 0);
    BOOST_CHECK(strcmp(updateVec[0]->device_type->c_str(), "")==0);
    BOOST_CHECK(strcmp(updateVec[0]->device_id->c_str(), "")==0);

	BOOST_CHECK(updateVec[0]->client_type == pClient->clientType);
	BOOST_CHECK(updateVec[0]->target_server == connInfo.targetServerType);
	BOOST_CHECK(updateVec[0]->conn_ev_type == PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
	BOOST_CHECK(updateVec[0]->gateway_id);
	BOOST_CHECK(
			updateVec[0]->last_act_type == PXACT_ACTIVITY_TYPE_MQTT_DISCONN_CLIENT);
	BOOST_CHECK(
			updateVec[0]->dirty_flags
					== (ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT
							| ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT));

    BOOST_CHECK(updateVec[0]->queuing_timestamp > 0);
    BOOST_CHECK(updateVec[0]->queuing_timestamp <= updateVec[0]->update_timestamp);
    BOOST_CHECK(updateVec[0]->num_updates == 1);

	BOOST_CHECK(bank->getNumClients() == 1);
	BOOST_CHECK(bank->getUpdateQueueSize() == 0);

	//===
	pClient->activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
	rc = bank->clientConnectivity(pClient, &connInfo);
	BOOST_CHECK(rc == ISMRC_OK);

	pClient->activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
	rc = bank->clientActivity(pClient);
	BOOST_CHECK(rc == ISMRC_OK);

	pClient->activityType = PXACT_ACTIVITY_TYPE_MQTT_SUB;
	rc = bank->clientActivity(pClient);
	BOOST_CHECK(rc == ISMRC_OK);

	pClient->activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_NETWORK;
	rc = bank->clientConnectivity(pClient, &connInfo);
	BOOST_CHECK(rc == ISMRC_OK);

	rc = bank->getUpdateBatch(updateVec, numUpdates);
	BOOST_CHECK(rc == ISMRC_OK);
	BOOST_CHECK(numUpdates == 1);
	std::cout << "update: " << updateVec[0]->toString() << std::endl;

	BOOST_CHECK(
			strcmp(updateVec[0]->client_id->c_str(), pClient->pClientID) == 0);
	BOOST_CHECK(strcmp(updateVec[0]->org->c_str(), pClient->pOrgName) == 0);
    BOOST_CHECK(strcmp(updateVec[0]->device_type->c_str(), "")==0);
    BOOST_CHECK(strcmp(updateVec[0]->device_id->c_str(), "")==0);

	BOOST_CHECK(updateVec[0]->client_type == pClient->clientType);
	BOOST_CHECK(updateVec[0]->target_server == connInfo.targetServerType);
	BOOST_CHECK(updateVec[0]->conn_ev_type == PXACT_ACTIVITY_TYPE_MQTT_DISCONN_NETWORK);
	BOOST_CHECK(updateVec[0]->gateway_id->empty());
	BOOST_CHECK(updateVec[0]->last_act_type == PXACT_ACTIVITY_TYPE_MQTT_SUB);
	BOOST_CHECK(
			updateVec[0]->dirty_flags
					== (ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT
							| ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT));

	//===
	pClient->pGateWayID = "g:org1:gw1";
	pClient->activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
	rc = bank->clientConnectivity(pClient, &connInfo);
	BOOST_CHECK(rc == ISMRC_OK);

	rc = bank->getUpdateBatch(updateVec, numUpdates);
	BOOST_CHECK(rc == ISMRC_OK);
	BOOST_CHECK(numUpdates == 1);
	std::cout << "update: " << updateVec[0]->toString() << std::endl;

	BOOST_CHECK(
			strcmp(updateVec[0]->client_id->c_str(), pClient->pClientID) == 0);
	BOOST_CHECK(strcmp(updateVec[0]->org->c_str(), pClient->pOrgName) == 0);
    BOOST_CHECK(strcmp(updateVec[0]->device_type->c_str(), "")==0);
    BOOST_CHECK(strcmp(updateVec[0]->device_id->c_str(), "")==0);

	BOOST_CHECK(updateVec[0]->client_type == pClient->clientType);
	BOOST_CHECK(updateVec[0]->target_server == connInfo.targetServerType);
	BOOST_CHECK(updateVec[0]->conn_ev_type == PXACT_ACTIVITY_TYPE_MQTT_CONN);
	BOOST_CHECK(
			strcmp(updateVec[0]->gateway_id->c_str(), pClient->pGateWayID)
					== 0);
	BOOST_CHECK(updateVec[0]->last_act_type == PXACT_ACTIVITY_TYPE_MQTT_CONN);
	BOOST_CHECK(
			updateVec[0]->dirty_flags
					== (ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT
							| ClientUpdateRecord::DIRTY_FLAG_METADATA));

	cout << "Stats: " << bank->getActivityStats().toString() << endl;
	free(pClient);
	delete bank;
}

BOOST_AUTO_TEST_CASE( multi_client_connect )
{
	using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityBank* bank = new ActivityBank(0);
    BOOST_CHECK(bank);
    BOOST_CHECK(bank->getNumClients() == 0);
    BOOST_CHECK(bank->getSizeBytes() == 0);
    BOOST_CHECK(bank->getUpdateQueueSize() == 0);

    ClientUpdateVector updateVec;
    unsigned VEC_SZ = 100;
    for (unsigned i=0; i<VEC_SZ; ++i)
    {
    	updateVec.push_back(ClientUpdateRecord_SPtr(new ClientUpdateRecord));
    }


    std::string types[4];
    for (auto i=0; i<4; ++i)
    {
    	types[i] = "type";
    	types[i].append(to_string(i));
    }
    unsigned NUM = 1000;
    std::string cids[NUM];


    ismPXACT_Client_t* pClientArray = (ismPXACT_Client_t*)calloc(NUM,sizeof(ismPXACT_Client_t));
    for (unsigned i = 0; i< NUM; ++i)
    {
    	ostringstream oss;
    	oss << "d:org1:" << types[(i%4)] << ":dev" << i;
    	cids[i] = oss.str();
    	pClientArray[i].pClientID = cids[i].c_str();
    	pClientArray[i].pOrgName = "org123";
    	pClientArray[i].pDeviceType = types[i%4].c_str();
    	pClientArray[i].pDeviceID = cids[i].c_str();
    	int j = cids[i].find_last_of(":");
    	pClientArray[i].pDeviceID += j;
    	pClientArray[i].clientType = PXACT_CLIENT_TYPE_DEVICE;
    }

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    int rc = ISMRC_OK;
    for (unsigned i=0; i<NUM; ++i)
    {
        pClientArray[i].activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
    	rc = bank->clientConnectivity(&pClientArray[i], &connInfo);
    	BOOST_CHECK(rc == ISMRC_OK);
    }

    BOOST_CHECK(bank->getNumClients() == NUM);
    BOOST_CHECK(bank->getUpdateQueueSize() == NUM);
    usleep(100000);

    for (unsigned i=0; i<NUM; ++i)
    {
    	uint32_t numUpdates = 0;
    	rc = bank->getUpdateBatch(updateVec, numUpdates);
    	BOOST_CHECK(rc == ISMRC_OK);
    	BOOST_CHECK(numUpdates == 1);
    	//std::cout << "update: " << updateVec[0]->toString() << std::endl;

    	BOOST_CHECK(strcmp(updateVec[0]->client_id->c_str(), pClientArray[i].pClientID)==0);
    	BOOST_CHECK(strcmp(updateVec[0]->org->c_str(), pClientArray[i].pOrgName)==0);
        BOOST_CHECK(strcmp(updateVec[0]->device_type->c_str(), pClientArray[i].pDeviceType)==0);
        BOOST_CHECK(strcmp(updateVec[0]->device_id->c_str(), pClientArray[i].pDeviceID)==0);

    	BOOST_CHECK(updateVec[0]->client_type == pClientArray[i].clientType);
    	BOOST_CHECK(updateVec[0]->target_server == connInfo.targetServerType);
    	BOOST_CHECK(updateVec[0]->conn_ev_type == PXACT_ACTIVITY_TYPE_MQTT_CONN);
    	BOOST_CHECK(updateVec[0]->gateway_id->empty());
    	BOOST_CHECK(updateVec[0]->last_act_type == PXACT_ACTIVITY_TYPE_MQTT_CONN);
    	BOOST_CHECK(updateVec[0]->dirty_flags == (ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT));

    	BOOST_CHECK(bank->getNumClients() == NUM);
    	BOOST_CHECK(bank->getUpdateQueueSize() == NUM-1-i);
    }

    for (unsigned i=0; i<100*NUM; ++i)
    {
        pClientArray[i%NUM].activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
    	rc = bank->clientActivity(&pClientArray[i%NUM]);
    	BOOST_CHECK(rc == ISMRC_OK);
    }

    BOOST_CHECK(bank->getNumClients() == NUM);
    BOOST_CHECK(bank->getUpdateQueueSize() == NUM);

    usleep(100000);

    while (bank->getUpdateQueueSize()>0)
    {
    	uint32_t numUpdates = 0;
    	rc = bank->getUpdateBatch(updateVec, numUpdates);
    	BOOST_CHECK(rc == ISMRC_OK);
    	BOOST_CHECK(numUpdates == VEC_SZ);
    	for (unsigned j=0; j<VEC_SZ; ++j)
    	{
    		BOOST_CHECK(strcmp(updateVec[j]->org->c_str(), pClientArray[0].pOrgName)==0);
            BOOST_CHECK(!updateVec[j]->device_type->empty());
            BOOST_CHECK(!updateVec[j]->device_id->empty());

    		BOOST_CHECK(updateVec[j]->client_type == pClientArray[0].clientType);
    		BOOST_CHECK(updateVec[j]->target_server == connInfo.targetServerType);
    		BOOST_CHECK(updateVec[j]->conn_ev_type == PXACT_ACTIVITY_TYPE_MQTT_CONN);
    		BOOST_CHECK(updateVec[j]->gateway_id->empty());
    		BOOST_CHECK(updateVec[j]->last_act_type == PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
    		BOOST_CHECK(updateVec[j]->dirty_flags == (ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT));
    	}
    }

    std::cout << "size bytes: " << bank->getSizeBytes() << std::endl;
    ActivityStats stats = bank->getActivityStats();
    cout << "Stats: " << stats.toString() << endl;
    BOOST_CHECK(stats.avg_conflation_factor >= 50);
    BOOST_CHECK(stats.avg_conflation_delay_ms > 100);

    free(pClientArray);
    delete bank;
}


BOOST_AUTO_TEST_CASE( multi_client_mix )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    ActivityBank* bank = new ActivityBank(0);
    BOOST_CHECK(bank);
    BOOST_CHECK(bank->getNumClients() == 0);
    BOOST_CHECK(bank->getSizeBytes() == 0);
    BOOST_CHECK(bank->getUpdateQueueSize() == 0);

    ClientUpdateVector updateVec;
    unsigned VEC_SZ = 100;
    for (unsigned i=0; i<VEC_SZ; ++i)
    {
        updateVec.push_back(ClientUpdateRecord_SPtr(new ClientUpdateRecord));
    }


    std::string types[4];
    for (auto i=0; i<4; ++i)
    {
        types[i] = "type";
        types[i].append(to_string(i));
    }
    unsigned NUM = 1000;
    std::string cids[NUM];


    ismPXACT_Client_t* pClientArray = (ismPXACT_Client_t*)calloc(NUM,sizeof(ismPXACT_Client_t));
    for (unsigned i = 0; i< NUM; ++i)
    {
        ostringstream oss;
        oss << "d:org1:" << types[(i%4)] << ":dev" << i;
        cids[i] = oss.str();
        pClientArray[i].pClientID = cids[i].c_str();
        pClientArray[i].pOrgName = "org123";
        pClientArray[i].pDeviceType = types[i%4].c_str();
        pClientArray[i].pDeviceID = cids[i].c_str();
        int j = cids[i].find_last_of(":");
        pClientArray[i].pDeviceID += j;
        pClientArray[i].clientType = PXACT_CLIENT_TYPE_DEVICE;
    }

    ismPXACT_ConnectivityInfo_t connInfo;
    connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1;
    connInfo.cleanS = 1;
    connInfo.expiryIntervalSec = 0;
    connInfo.reasonCode = 0;
    connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

    int rc = ISMRC_OK;
    for (unsigned i=0; i<NUM; ++i)
    {
        pClientArray[i].activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
        rc = bank->clientConnectivity(&pClientArray[i], &connInfo);
        BOOST_CHECK(rc == ISMRC_OK);
    }

    BOOST_CHECK(bank->getNumClients() == NUM);
    BOOST_CHECK(bank->getUpdateQueueSize() == NUM);
    usleep(100000);

    for (unsigned i=0; i<NUM; ++i)
    {
        uint32_t numUpdates = 0;
        rc = bank->getUpdateBatch(updateVec, numUpdates);
        BOOST_CHECK(rc == ISMRC_OK);
        BOOST_CHECK(numUpdates == 1);
        //std::cout << "update: " << updateVec[0]->toString() << std::endl;

        BOOST_CHECK(strcmp(updateVec[0]->client_id->c_str(), pClientArray[i].pClientID)==0);
        BOOST_CHECK(strcmp(updateVec[0]->org->c_str(), pClientArray[i].pOrgName)==0);
        BOOST_CHECK(strcmp(updateVec[0]->device_type->c_str(), pClientArray[i].pDeviceType)==0);
        BOOST_CHECK(strcmp(updateVec[0]->device_id->c_str(), pClientArray[i].pDeviceID)==0);

        BOOST_CHECK(updateVec[0]->client_type == pClientArray[i].clientType);
        BOOST_CHECK(updateVec[0]->target_server == connInfo.targetServerType);
        BOOST_CHECK(updateVec[0]->conn_ev_type == PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(updateVec[0]->gateway_id->empty());
        BOOST_CHECK(updateVec[0]->last_act_type == PXACT_ACTIVITY_TYPE_MQTT_CONN);
        BOOST_CHECK(updateVec[0]->dirty_flags == (ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT | ClientUpdateRecord::DIRTY_FLAG_NEW_RECORD | ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT));

        BOOST_CHECK(bank->getNumClients() == NUM);
        BOOST_CHECK(bank->getUpdateQueueSize() == NUM-1-i);
    }

    for (unsigned i=0; i<100*NUM; ++i)
    {
        unsigned c = i%NUM;
        if (c%10 == 0)
        {
            pClientArray[c].activityType = PXACT_ACTIVITY_TYPE_MQTT_DISCONN_NETWORK;
            rc = bank->clientConnectivity(&pClientArray[c], &connInfo);
            BOOST_CHECK(rc == ISMRC_OK);
            pClientArray[c].activityType = PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT;
            rc = bank->clientActivity(&pClientArray[c]);
            BOOST_CHECK(rc == ISMRC_OK);
        }
        else
        {
            pClientArray[c].activityType = PXACT_ACTIVITY_TYPE_MQTT_PUBLISH;
            rc = bank->clientActivity(&pClientArray[c]);
            BOOST_CHECK(rc == ISMRC_OK);
        }
    }

    BOOST_CHECK(bank->getNumClients() == NUM);
    BOOST_CHECK(bank->getUpdateQueueSize() == NUM);

    usleep(100000);

    unsigned int num_bulk = 0;
    while (bank->getUpdateQueueSize()>0)
    {
        uint32_t numUpdates = 0;
        rc = bank->getUpdateBatch(updateVec, numUpdates);
        BOOST_CHECK(rc == ISMRC_OK);
        if (numUpdates > 1)
        {
            num_bulk++;
            BOOST_CHECK(numUpdates == VEC_SZ);
            for (unsigned j=0; j<numUpdates; ++j)
            {
                BOOST_CHECK(strcmp(updateVec[j]->org->c_str(), pClientArray[0].pOrgName)==0);
                BOOST_CHECK(!updateVec[j]->device_type->empty());
                BOOST_CHECK(!updateVec[j]->device_id->empty());

                BOOST_CHECK(updateVec[j]->client_type == pClientArray[0].clientType);
                BOOST_CHECK(updateVec[j]->target_server == connInfo.targetServerType);
                BOOST_CHECK(updateVec[j]->conn_ev_type == PXACT_ACTIVITY_TYPE_MQTT_CONN);
                BOOST_CHECK(updateVec[j]->gateway_id->empty());
                BOOST_CHECK(updateVec[j]->last_act_type == PXACT_ACTIVITY_TYPE_MQTT_PUBLISH);
                BOOST_CHECK(updateVec[j]->dirty_flags == (ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT));
            }
        }
        else
        {
            BOOST_CHECK(numUpdates == 1);
            for (unsigned j=0; j<numUpdates; ++j)
            {
                BOOST_CHECK(strcmp(updateVec[j]->org->c_str(), pClientArray[0].pOrgName)==0);
                BOOST_CHECK(!updateVec[j]->device_type->empty());
                BOOST_CHECK(!updateVec[j]->device_id->empty());

                BOOST_CHECK(updateVec[j]->client_type == pClientArray[0].clientType);
                BOOST_CHECK(updateVec[j]->target_server == connInfo.targetServerType);
                BOOST_CHECK(updateVec[j]->conn_ev_type == PXACT_ACTIVITY_TYPE_MQTT_DISCONN_NETWORK);
                BOOST_CHECK(updateVec[j]->gateway_id->empty());
                BOOST_CHECK(updateVec[j]->last_act_type == PXACT_ACTIVITY_TYPE_HTTP_POST_EVENT);
                BOOST_CHECK(updateVec[j]->dirty_flags == (ClientUpdateRecord::DIRTY_FLAG_ACTITITY_EVENT | ClientUpdateRecord::DIRTY_FLAG_CONN_EVENT));
            }
        }
    }

    BOOST_CHECK(num_bulk == (9*NUM/10) / VEC_SZ);

    std::cout << "size bytes: " << bank->getSizeBytes() << std::endl;
    ActivityStats stats = bank->getActivityStats();
    cout << "Stats: " << stats.toString() << endl;
    BOOST_CHECK(stats.avg_conflation_factor >= 50);
    BOOST_CHECK(stats.avg_conflation_delay_ms > 100);

    free(pClientArray);
    delete bank;
}

BOOST_AUTO_TEST_CASE( evict )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	for (auto algo=0; algo <= 2; algo++)
	{

		ActivityBank* bank = new ActivityBank(0,100*1024*1024,algo);
		ClientUpdateVector updateVec;
		unsigned VEC_SZ = 100;
		for (unsigned i=0; i<VEC_SZ; ++i)
		{
			updateVec.push_back(ClientUpdateRecord_SPtr(new ClientUpdateRecord));
		}

		std::string types[4];
		for (auto i=0; i<4; ++i)
		{
			types[i] = "type";
			types[i].append(to_string(i));
		}
		unsigned NUM = 1000;
		std::string cids[NUM];

		ismPXACT_Client_t* pClientArray = (ismPXACT_Client_t*)calloc(NUM,sizeof(ismPXACT_Client_t));
		for (unsigned i = 0; i< NUM; ++i)
		{
			ostringstream oss;
			oss << "d:org1:" << types[(i%4)] << ":dev" << i;
			cids[i] = oss.str();
			pClientArray[i].pClientID = cids[i].c_str();
			pClientArray[i].pOrgName = "org123";
			pClientArray[i].pDeviceType = types[i%4].c_str();
			pClientArray[i].pDeviceID = cids[i].c_str();
			int j = cids[i].find_last_of(":");
			pClientArray[i].pDeviceID += j;
			pClientArray[i].clientType = PXACT_CLIENT_TYPE_DEVICE;
		}

		ismPXACT_ConnectivityInfo_t connInfo;
		connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1;
		connInfo.cleanS = 1;
		connInfo.expiryIntervalSec = 0;
		connInfo.reasonCode = 0;
		connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

		//=== start activity ===

		int rc = ISMRC_OK;
		for (unsigned i=0; i<NUM/2; ++i)
		{
		    pClientArray[i].activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
			rc = bank->clientConnectivity(&pClientArray[i], &connInfo);
			BOOST_CHECK(rc == ISMRC_OK);
		}

		BOOST_CHECK(bank->getNumClients() == NUM/2);
		BOOST_CHECK(bank->getUpdateQueueSize() == NUM/2);

		for (unsigned i=0; i<NUM/2; ++i)
		{
			uint32_t numUpdates = 0;
			rc = bank->getUpdateBatch(updateVec, numUpdates);
			BOOST_CHECK(rc == ISMRC_OK);
			BOOST_CHECK(numUpdates == 1);
		}

		//=== now we have NUM/2 clean clients

		for (unsigned i=NUM/2; i<NUM; ++i)
		{
		    pClientArray[i].activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
			rc = bank->clientConnectivity(&pClientArray[i], &connInfo);
			BOOST_CHECK(rc == ISMRC_OK);
		}

		BOOST_CHECK(bank->getNumClients() == NUM);
		BOOST_CHECK(bank->getUpdateQueueSize() == NUM/2);

		//=== now we have NUM/2 clean clients & NUM/2 dirty clients
		std::cout << "1> size bytes: " << bank->getSizeBytes() << " #clients: " << bank->getNumClients() << std::endl;
		std::size_t total_mem1 = bank->getSizeBytes();

		std::size_t evicted = bank->evictClients(NUM/10);
		BOOST_CHECK(evicted == NUM/10);
		BOOST_CHECK(bank->getNumClients() == 9*NUM/10);

		std::cout << "2> size bytes: " << bank->getSizeBytes() << " #clients: " << bank->getNumClients() << std::endl;

		evicted = bank->evictClients(NUM/10);
		BOOST_CHECK(evicted == NUM/10);
		BOOST_CHECK(bank->getNumClients() == 8*NUM/10);

		std::cout << "3> size bytes: " << bank->getSizeBytes() << " #clients: " << bank->getNumClients() << std::endl;

		evicted = bank->evictClients(NUM/10);
		BOOST_CHECK(evicted == NUM/10);
		BOOST_CHECK(bank->getNumClients() == 7*NUM/10);

		std::cout << "4> size bytes: " << bank->getSizeBytes() << " #clients: " << bank->getNumClients() << std::endl;

		evicted = bank->evictClients(NUM/10);
		BOOST_CHECK(evicted == NUM/10);
		BOOST_CHECK(bank->getNumClients() == 6*NUM/10);

		std::cout << "5> size bytes: " << bank->getSizeBytes() << " #clients: " << bank->getNumClients() << std::endl;

		evicted = bank->evictClients(NUM/10);
		BOOST_CHECK(evicted == NUM/10);
		BOOST_CHECK(bank->getNumClients() == 5*NUM/10);

		std::cout << "6> size bytes: " << bank->getSizeBytes() << " #clients: " << bank->getNumClients() << std::endl;

		evicted = bank->evictClients(NUM/10);
		BOOST_CHECK(evicted == 0);
		BOOST_CHECK(bank->getNumClients() == 5*NUM/10);

		std::cout << "7> size bytes: " << bank->getSizeBytes() << " #clients: " << bank->getNumClients() << std::endl;

		std::size_t total_mem2 = bank->getSizeBytes();

		BOOST_CHECK(total_mem2 * 2.0 < total_mem1 * 1.1);

		free(pClientArray);
		delete bank;
	} //algo
}





BOOST_AUTO_TEST_CASE( evict_big )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	for (auto algo=0; algo <= 3; algo++)
	{

		ActivityBank* bank = new ActivityBank(0,100*1024*1024,algo);
		ClientUpdateVector updateVec;
		unsigned VEC_SZ = 100;
		for (unsigned i=0; i<VEC_SZ; ++i)
		{
			updateVec.push_back(ClientUpdateRecord_SPtr(new ClientUpdateRecord));
		}

		std::string types[4];
		for (auto i=0; i<4; ++i)
		{
			types[i] = "type";
			types[i].append(to_string(i));
		}
		unsigned NUM = 100000;
		vector<string> cids;
		cids.reserve(NUM);

		ismPXACT_Client_t* pClientArray = (ismPXACT_Client_t*)calloc(NUM,sizeof(ismPXACT_Client_t));
		for (unsigned i = 0; i< NUM; ++i)
		{
			ostringstream oss;
			oss << "d:org1:" << types[(i%4)] << ":dev" << i;
			cids.push_back(oss.str());
			pClientArray[i].pClientID = cids[i].c_str();
			pClientArray[i].pOrgName = "org123";
			pClientArray[i].pDeviceType = types[i%4].c_str();
			pClientArray[i].pDeviceID = cids[i].c_str();
			int j = cids[i].find_last_of(":");
			pClientArray[i].pDeviceID += j;
			pClientArray[i].clientType = PXACT_CLIENT_TYPE_DEVICE;
		}

		ismPXACT_ConnectivityInfo_t connInfo;
		connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1;
		connInfo.cleanS = 1;
		connInfo.expiryIntervalSec = 0;
		connInfo.reasonCode = 0;
		connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

		//=== start activity ===

		auto start = std::chrono::system_clock::now();

		int rc = ISMRC_OK;
		for (unsigned i=0; i<NUM; ++i)
		{
		    pClientArray[i].activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
			rc = bank->clientConnectivity(&pClientArray[i], &connInfo);
			BOOST_CHECK(rc == ISMRC_OK);
		}

		BOOST_CHECK(bank->getNumClients() == NUM);
		BOOST_CHECK(bank->getUpdateQueueSize() == NUM);

		for (unsigned i=0; i<NUM/2; ++i)
		{
			uint32_t numUpdates = 0;
			rc = bank->getUpdateBatch(updateVec, numUpdates);
			BOOST_CHECK(rc == ISMRC_OK);
			BOOST_CHECK(numUpdates == 1);
		}

		auto fill = std::chrono::system_clock::now();

		//=== now we have NUM/2 clean clients

		std::cout << "Algo:" << algo << "> size bytes: " << bank->getSizeBytes() << " #clients: " << bank->getNumClients()
				<< "; Fill-Dur=" << chrono::duration<double>(fill-start).count() << std::endl;

		std::size_t evicted = bank->evictClients(NUM/20);
		BOOST_CHECK(evicted == NUM/20);

		auto evict_time = std::chrono::system_clock::now();
		std::cout << "Algo:" << algo << "> size bytes: " << bank->getSizeBytes() << " #clients: " << bank->getNumClients()
				<< "; Evict-Dur=" << chrono::duration<double>(evict_time-fill).count() << std::endl;

		free(pClientArray);
		delete bank;
	} //algo
}




BOOST_AUTO_TEST_CASE( manage_memory )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");
	std::size_t limit = 16*1024;
	ActivityBank* bank = new ActivityBank(0, limit);
	ClientUpdateVector updateVec;
	unsigned VEC_SZ = 100;
	for (unsigned i=0; i<VEC_SZ; ++i)
	{
		updateVec.push_back(ClientUpdateRecord_SPtr(new ClientUpdateRecord));
	}

	std::string types[4];
	for (auto i=0; i<4; ++i)
	{
		types[i] = "type";
		types[i].append(to_string(i));
	}
	unsigned NUM = 1000;
	std::string cids[NUM];

	ismPXACT_Client_t* pClientArray = (ismPXACT_Client_t*)calloc(NUM,sizeof(ismPXACT_Client_t));
	for (unsigned i = 0; i< NUM; ++i)
	{
		ostringstream oss;
		oss << "d:org1:" << types[(i%4)] << ":dev" << i;
		cids[i] = oss.str();
		pClientArray[i].pClientID = cids[i].c_str();
		pClientArray[i].pOrgName = "org123";
		pClientArray[i].pDeviceType = types[i%4].c_str();
		pClientArray[i].pDeviceID = cids[i].c_str();
		int j = cids[i].find_last_of(":");
		pClientArray[i].pDeviceID += j;
		pClientArray[i].clientType = PXACT_CLIENT_TYPE_DEVICE;
	}

	ismPXACT_ConnectivityInfo_t connInfo;
	connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1;
	connInfo.cleanS = 1;
	connInfo.expiryIntervalSec = 0;
	connInfo.reasonCode = 0;
	connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

	//=== start activity ===

	int rc = ISMRC_OK;
	for (unsigned i=0; i<NUM; ++i)
	{
	    pClientArray[i].activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
		rc = bank->clientConnectivity(&pClientArray[i], &connInfo);
		BOOST_CHECK(rc == ISMRC_OK);

		if (i%10 == 0)
		{
			while (bank->getUpdateQueueSize() > 0)
			{
				uint32_t numUpdates = 0;
				rc = bank->getUpdateBatch(updateVec, numUpdates);
				BOOST_CHECK(rc == ISMRC_OK);
				BOOST_CHECK(numUpdates == 1);
				rc = bank->manageMemory();
				BOOST_CHECK(rc == ISMRC_OK);
			}

			BOOST_CHECK(bank->getSizeBytes() <= limit);
			cout << "#clients: " << bank->getNumClients() << " size: " << bank->getSizeBytes() << " limit: " << limit << endl;
		}
	}

	while (bank->getUpdateQueueSize() > 0)
	{
		uint32_t numUpdates = 0;
		rc = bank->getUpdateBatch(updateVec, numUpdates);
		BOOST_CHECK(rc == ISMRC_OK);
		BOOST_CHECK(numUpdates == 1);
		rc = bank->manageMemory();
		BOOST_CHECK(rc == ISMRC_OK);
	}

	BOOST_CHECK(bank->getSizeBytes() <= limit);
	cout << "#clients: " << bank->getNumClients() << " size: " << bank->getSizeBytes() << " limit: " << limit << endl;

	free(pClientArray);
	delete bank;
}



BOOST_AUTO_TEST_CASE( manage_memory_big_quickselect )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	unsigned NUM = 100000;
	std::size_t limit = 19*1024*1024;
	ActivityBank* bank = new ActivityBank(0, limit, 0);
	ClientUpdateVector updateVec;
	unsigned VEC_SZ = 100;
	for (unsigned i=0; i<VEC_SZ; ++i)
	{
		updateVec.push_back(ClientUpdateRecord_SPtr(new ClientUpdateRecord));
	}

	std::string types[4];
	for (auto i=0; i<4; ++i)
	{
		types[i] = "type";
		types[i].append(to_string(i));
	}

	std::vector<std::string> cids;

	ismPXACT_Client_t* pClientArray = (ismPXACT_Client_t*)calloc(NUM,sizeof(ismPXACT_Client_t));
	for (unsigned i = 0; i< NUM; ++i)
	{
		ostringstream oss;
		oss << "d:org1:" << types[(i%4)] << ":dev" << i;
		cids.push_back(oss.str());
		pClientArray[i].pClientID = cids[i].c_str();
		pClientArray[i].pOrgName = "org123";
		pClientArray[i].pDeviceType = types[i%4].c_str();
		pClientArray[i].pDeviceID = cids[i].c_str();
		int j = cids[i].find_last_of(":");
		pClientArray[i].pDeviceID += j;
		pClientArray[i].clientType = PXACT_CLIENT_TYPE_DEVICE;
	}

	ismPXACT_ConnectivityInfo_t connInfo;
	connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1;
	connInfo.cleanS = 1;
	connInfo.expiryIntervalSec = 0;
	connInfo.reasonCode = 0;
	connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

	//=== start activity ===

	auto start = std::chrono::system_clock::now();

	int rc = ISMRC_OK;
	for (unsigned i=0; i<NUM; ++i)
	{
	    pClientArray[i].activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
		rc = bank->clientConnectivity(&pClientArray[i], &connInfo);
		BOOST_CHECK(rc == ISMRC_OK);

		if (i%10 == 0)
		{
			while (bank->getUpdateQueueSize() > 0)
			{
				uint32_t numUpdates = 0;
				rc = bank->getUpdateBatch(updateVec, numUpdates);
				BOOST_CHECK(rc == ISMRC_OK);
				BOOST_CHECK(numUpdates == 1);
			}
		}
	}

	cout << "#clients: " << bank->getNumClients() << " size: " << bank->getSizeBytes() << " limit: " << limit << endl;
	auto mid = std::chrono::system_clock::now();

	while (bank->getSizeBytes() > limit)
	{
		rc = bank->manageMemory();
		BOOST_CHECK(rc == ISMRC_OK);
	}

	//BOOST_CHECK(bank->getSizeBytes() <= limit);
	cout << "#clients: " << bank->getNumClients() << " size: " << bank->getSizeBytes() << " limit: " << limit << endl;

	auto stop = std::chrono::system_clock::now();

	cout << "fill: " << std::chrono::duration<double>(mid-start).count() << " manage: " << std::chrono::duration<double>(stop-mid).count() << endl;

	free(pClientArray);
	delete bank;
}



BOOST_AUTO_TEST_CASE( manage_memory_big_heap )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");
	unsigned NUM = 100000;
	std::size_t limit = 19*1024*1024;
	ActivityBank* bank = new ActivityBank(0, limit, 1);
	ClientUpdateVector updateVec;
	unsigned VEC_SZ = 100;
	for (unsigned i=0; i<VEC_SZ; ++i)
	{
		updateVec.push_back(ClientUpdateRecord_SPtr(new ClientUpdateRecord));
	}

	std::string types[4];
	for (auto i=0; i<4; ++i)
	{
		types[i] = "type";
		types[i].append(to_string(i));
	}

	std::vector<std::string> cids;

	ismPXACT_Client_t* pClientArray = (ismPXACT_Client_t*)calloc(NUM,sizeof(ismPXACT_Client_t));
	for (unsigned i = 0; i< NUM; ++i)
	{
		ostringstream oss;
		oss << "d:org1:" << types[(i%4)] << ":dev" << i;
		cids.push_back(oss.str());
		pClientArray[i].pClientID = cids[i].c_str();
		pClientArray[i].pOrgName = "org123";
		pClientArray[i].pDeviceType = types[i%4].c_str();
		pClientArray[i].pDeviceID = cids[i].c_str();
		int j = cids[i].find_last_of(":");
		pClientArray[i].pDeviceID += j;
		pClientArray[i].clientType = PXACT_CLIENT_TYPE_DEVICE;
	}

	ismPXACT_ConnectivityInfo_t connInfo;
	connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1;
	connInfo.cleanS = 1;
	connInfo.expiryIntervalSec = 0;
	connInfo.reasonCode = 0;
	connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

	//=== start activity ===

	auto start = std::chrono::system_clock::now();

	int rc = ISMRC_OK;
	for (unsigned i=0; i<NUM; ++i)
	{
	    pClientArray[i].activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
		rc = bank->clientConnectivity(&pClientArray[i], &connInfo);
		BOOST_CHECK(rc == ISMRC_OK);

		if (i%10 == 0)
		{
			while (bank->getUpdateQueueSize() > 0)
			{
				uint32_t numUpdates = 0;
				rc = bank->getUpdateBatch(updateVec, numUpdates);
				BOOST_CHECK(rc == ISMRC_OK);
				BOOST_CHECK(numUpdates == 1);
			}
		}
	}

	cout << "#clients: " << bank->getNumClients() << " size: " << bank->getSizeBytes() << " limit: " << limit << endl;
	auto mid = std::chrono::system_clock::now();

	while (bank->getSizeBytes() > limit)
	{
		rc = bank->manageMemory();
		BOOST_CHECK(rc == ISMRC_OK);
	}

	//BOOST_CHECK(bank->getSizeBytes() <= limit);
	cout << "#clients: " << bank->getNumClients() << " size: " << bank->getSizeBytes() << " limit: " << limit << endl;

	auto stop = std::chrono::system_clock::now();

	cout << "fill: " << std::chrono::duration<double>(mid-start).count() << " manage: " << std::chrono::duration<double>(stop-mid).count() << endl;

	free(pClientArray);
	delete bank;
}


BOOST_AUTO_TEST_CASE( manage_memory_big_nth_element_deque )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	unsigned NUM = 100000;
	std::size_t limit = 19*1024*1024;
	ActivityBank* bank = new ActivityBank(0, limit, 2);
	ClientUpdateVector updateVec;
	unsigned VEC_SZ = 100;
	for (unsigned i=0; i<VEC_SZ; ++i)
	{
		updateVec.push_back(ClientUpdateRecord_SPtr(new ClientUpdateRecord));
	}

	std::string types[4];
	for (auto i=0; i<4; ++i)
	{
		types[i] = "type";
		types[i].append(to_string(i));
	}

	std::vector<std::string> cids;

	ismPXACT_Client_t* pClientArray = (ismPXACT_Client_t*)calloc(NUM,sizeof(ismPXACT_Client_t));
	for (unsigned i = 0; i< NUM; ++i)
	{
		ostringstream oss;
		oss << "d:org1:" << types[(i%4)] << ":dev" << i;
		cids.push_back(oss.str());
		pClientArray[i].pClientID = cids[i].c_str();
		pClientArray[i].pOrgName = "org123";
		pClientArray[i].pDeviceType = types[i%4].c_str();
		pClientArray[i].pDeviceID = cids[i].c_str();
		int j = cids[i].find_last_of(":");
		pClientArray[i].pDeviceID += j;
		pClientArray[i].clientType = PXACT_CLIENT_TYPE_DEVICE;
	}

	ismPXACT_ConnectivityInfo_t connInfo;
	connInfo.protocol = PXACT_CONN_PROTO_TYPE_MQTT_v3_1_1;
	connInfo.cleanS = 1;
	connInfo.expiryIntervalSec = 0;
	connInfo.reasonCode = 0;
	connInfo.targetServerType = PXACT_TRG_SERVER_TYPE_MS;

	//=== start activity ===

	auto start = std::chrono::system_clock::now();

	int rc = ISMRC_OK;
	for (unsigned i=0; i<NUM; ++i)
	{
	    pClientArray[i].activityType = PXACT_ACTIVITY_TYPE_MQTT_CONN;
		rc = bank->clientConnectivity(&pClientArray[i], &connInfo);
		BOOST_CHECK(rc == ISMRC_OK);

		if (i%10 == 0)
		{
			while (bank->getUpdateQueueSize() > 0)
			{
				uint32_t numUpdates = 0;
				rc = bank->getUpdateBatch(updateVec, numUpdates);
				BOOST_CHECK(rc == ISMRC_OK);
				BOOST_CHECK(numUpdates == 1);
			}
		}
	}

	cout << "#clients: " << bank->getNumClients() << " size: " << bank->getSizeBytes() << " limit: " << limit << endl;
	auto mid = std::chrono::system_clock::now();

	while (bank->getSizeBytes() > limit)
	{
		rc = bank->manageMemory();
		BOOST_CHECK(rc == ISMRC_OK);
	}

	//BOOST_CHECK(bank->getSizeBytes() <= limit);
	cout << "#clients: " << bank->getNumClients() << " size: " << bank->getSizeBytes() << " limit: " << limit << endl;

	auto stop = std::chrono::system_clock::now();

	cout << "fill: " << std::chrono::duration<double>(mid-start).count() << " manage: " << std::chrono::duration<double>(stop-mid).count() << endl;

	cout << "Stats: " << bank->getActivityStats().toString() << endl;

	free(pClientArray);
	delete bank;
}


BOOST_AUTO_TEST_SUITE_END()
