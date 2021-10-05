/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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


#ifndef ServerProxy_UNITTESTINIT_H_
#define ServerProxy_UNITTESTINIT_H_

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/bind.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/tss.hpp>
#include <boost/thread/once.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time.hpp>

/**
 * Will:
 *
 * 1) extract environment variable SERVERPROXY_UNITTEST_MONGODB_URI into static field mongodbURI.
 * 2) extract environment variable SERVERPROXY_UNITTEST_MONGODB_USER into static field mongodbUSER.
 * 3) extract environment variable SERVERPROXY_UNITTEST_MONGODB_PWD into static field mongodbPWD.
 */
class ServerProxyUnitTestInit
{

public:
	static constexpr char const * URI_DEF = "mongodb://lnx-wiotp2.openstack.haifa.ibm.com:27017";
	static std::string mongodbURI;
	static std::string mongodbUSER;
	static std::string mongodbPWD;
	static std::string mongodbEP;
	static std::string mongodbCAFile;
	static std::string mongodbOptions;

	/**
	 * A script that starts MongoDB
	 * e.g. "/home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/start_MongoDB.sh"
	 */
	static std::string mongodbStartCommand;

	/**
	 * A script that stops MongoDB
	 * e.g. "/home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/stop_MongoDB.sh"
	 */
	static std::string mongodbStopCommand;
	static int mongodbUseTLS;

	ServerProxyUnitTestInit();

	virtual ~ServerProxyUnitTestInit();
};

#endif /* ServerProxy_UNITTESTINIT_H_ */
