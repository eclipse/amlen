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
 * ProxyMembershipTest.cpp
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

BOOST_AUTO_TEST_SUITE( Membership_suite )

BOOST_AUTO_TEST_CASE( start_4 )
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
    config.hbIntervalMS = 500;
    config.hbTimeoutMS = 10000;
    config.numWorkerThreads = 2;

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

    string uid = tracker.getProxyUID();
    BOOST_CHECK(!uid.empty());
    BOOST_CHECK(uid.size() >= 8);

    cout << "uid: " << uid << endl;

    rc = tracker.close(true);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    db.closeClient(db_client);
    db.close();
}

BOOST_AUTO_TEST_SUITE_END()
