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


#include "ServerProxyUnitTestInit.h"
#include "ismutil.h"

std::string ServerProxyUnitTestInit::mongodbURI = URI_DEF;
std::string ServerProxyUnitTestInit::mongodbUSER = "";
std::string ServerProxyUnitTestInit::mongodbPWD = "";
std::string ServerProxyUnitTestInit::mongodbEP = "";
std::string ServerProxyUnitTestInit::mongodbCAFile = "";
std::string ServerProxyUnitTestInit::mongodbOptions = "";
std::string ServerProxyUnitTestInit::mongodbStartCommand = "";
std::string ServerProxyUnitTestInit::mongodbStopCommand = "";

int ServerProxyUnitTestInit::mongodbUseTLS = 0;

ServerProxyUnitTestInit::ServerProxyUnitTestInit()
{
	using namespace std;

	cout << "ServerProxy UnitTest Init:" << endl;

	{
		char * mongo_uri = getenv("SERVERPROXY_UNITTEST_MONGODB_URI");
		if (mongo_uri)
		{
			cout << "value of SERVERPROXY_UNITTEST_MONGODB_URI is: " << mongo_uri << " len=" <<strlen(mongo_uri)<< endl;
			mongodbURI = mongo_uri;

			cout << "mongodbURI=" << mongodbURI << endl;
		}
		else
		{
			cout << "variable SERVERPROXY_UNITTEST_MONGODB_URI not defined." << endl;
			cout << "default: mongodbURI=" << mongodbURI << endl;
		}
	}

	{
	char * mongo_user = getenv("SERVERPROXY_UNITTEST_MONGODB_USER");
		if (mongo_user)
		{
			cout << "value of SERVERPROXY_UNITTEST_MONGODB_USER is: " << mongo_user << endl;
			mongodbUSER = mongo_user;

			cout << "mongodbUSER=" << mongodbUSER << endl;
		}
		else
		{
			cout << "variable SERVERPROXY_UNITTEST_MONGODB_USER not defined." << endl;
			cout << "default: mongodbUSER=" << mongodbUSER << endl;
		}
	}

	{
		char * mongo_pwd = getenv("SERVERPROXY_UNITTEST_MONGODB_PWD");
			if (mongo_pwd)
			{
				cout << "value of SERVERPROXY_UNITTEST_MONGODB_PWD is: " << mongo_pwd << endl;
				mongodbPWD = mongo_pwd;

				cout << "mongodbPWD=" << mongodbPWD << endl;
			}
			else
			{
				cout << "variable SERVERPROXY_UNITTEST_MONGODB_PWD not defined." << endl;
				cout << "default: mongodbPWD=" << mongodbPWD << endl;
			}
		}

	{
        char * mongo_address = getenv("SERVERPROXY_UNITTEST_MONGODB_EP");
        if (mongo_address)
        {
            cout << "value of SERVERPROXY_UNITTEST_MONGODB_EP is: " << mongo_address << endl;
            mongodbEP = mongo_address;

            cout << "mongodbEP=" << mongodbEP << endl;
        }
        else
        {
            cout << "variable SERVERPROXY_UNITTEST_MONGODB_EP not defined." << endl;
            cout << "default: mongodbEP=" << mongodbEP << endl;
        }
    }

    {
        char * mongo_ca_file = getenv("SERVERPROXY_UNITTEST_MONGODB_CA_FILE");
        if (mongo_ca_file)
        {
            cout << "value of SERVERPROXY_UNITTEST_MONGODB_CA_FILE is: " << mongo_ca_file << endl;
            mongodbCAFile = mongo_ca_file;

            cout << "mongodbCAFile=" << mongodbCAFile << endl;
        }
        else
        {
            cout << "variable SERVERPROXY_UNITTEST_MONGODB_CA_FILE not defined." << endl;
            cout << "default: mongodbCAFile=" << mongodbCAFile << endl;
        }
    }

    {
        char * mongo_opt = getenv("SERVERPROXY_UNITTEST_MONGODB_OPTIONS");
        if (mongo_opt)
        {
            cout << "value of SERVERPROXY_UNITTEST_MONGODB_OPTIONS is: " << mongo_opt << endl;
            mongodbOptions = mongo_opt;

            cout << "mongodbOptions=" << mongodbOptions << endl;
        }
        else
        {
            cout << "variable SERVERPROXY_UNITTEST_MONGODB_OPTIONS not defined." << endl;
            cout << "default: mongodbOptions=" << mongodbOptions << endl;
        }
    }

    {
        char * mongo_start = getenv("SERVERPROXY_UNITTEST_MONGODB_START_COMMAND");
        if (mongo_start)
        {
            cout << "value of SERVERPROXY_UNITTEST_MONGODB_START_COMMAND is: " << mongo_start << endl;
            mongodbStartCommand = mongo_start;

            cout << "mongodbStartCommand=" << mongodbStartCommand << endl;
        }
        else
        {
            cout << "variable SERVERPROXY_UNITTEST_MONGODB_START_COMMAND not defined." << endl;
            cout << "default: mongodbStartCommand=" << mongodbStartCommand << endl;
        }
    }

    {
        char * mongo_stop = getenv("SERVERPROXY_UNITTEST_MONGODB_STOP_COMMAND");
        if (mongo_stop)
        {
            cout << "value of SERVERPROXY_UNITTEST_MONGODB_STOP_COMMAND is: " << mongo_stop << endl;
            mongodbStopCommand = mongo_stop;

            cout << "mongodbStopCommand=" << mongodbStopCommand << endl;
        }
        else
        {
            cout << "variable SERVERPROXY_UNITTEST_MONGODB_STOP_COMMAND not defined." << endl;
            cout << "default: mongodbStopCommand=" << mongodbStopCommand << endl;
        }
    }

    {
          char * mongo_use_tls = getenv("SERVERPROXY_UNITTEST_MONGODB_USE_TLS");
          if (mongo_use_tls)
          {
              cout << "value of SERVERPROXY_UNITTEST_MONGODB_USE_TLS is: " << mongo_use_tls << endl;

              mongodbUseTLS = atoi(mongo_use_tls);

              cout << "mongodbUseTLS=" << mongodbUseTLS << endl;
          }
          else
          {
              cout << "variable SERVERPROXY_UNITTEST_MONGODB_USE_TLS not defined." << endl;
              cout << "default: mongodbUseTLS=" << mongodbUseTLS << endl;
          }
      }

	ism_common_initUtil();
	ism_common_initTimers();

	cout << endl;
}

ServerProxyUnitTestInit::~ServerProxyUnitTestInit()
{
	ism_common_stopTimers();
}
