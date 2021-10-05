Unit test for activity component
================================

1. Build
========

The test suite uses the Boost Test library (currently version 1.55): https://www.boost.org/doc/libs/1_55_0/libs/test/doc/html/index.html
Dynamically link to this library (use DBOOST_TEST_DYN_LINK).

The test suite uses the mongodb C-client libmongoc-1.0 and libbson-1.0.
The test suite includes from server_utils/include.
The test suite includes from server_proxy/src and server_proxy/src/activity.

For example:

g++ -std=c++11 -DBOOST_TEST_DYN_LINK -I/usr/include/libbson-1.0 -I/usr/include/libmongoc-1.0 -I"$BUILD_DIR/server_utils/include" 
	-I"$BUILD_DIR/server_proxy/src" -I"$BUILD_DIR/server_proxy/src/activity" -O1 -g3 -Wall -c -fmessage-length=0
	
Link against:

imaproxy
ismutil
mongoc-1.0 
bson-1.0
boost_unit_test_framework
boost_thread
boost_date_time
boost_system
boost_regex
icuuc
icudata
icui18n
pthread
	
2. Deployment & config
======================

The tests require a MongoDB instance to work against. MongoDB should include:
A database with name = "activity_track_unittest",
Client state collection = "client_state",
Proxy state collection = "proxy_state",
Access control should allow CRUD operations or ReadWrite role.

The following environment variables control how the test suite behaves w.r.t. MongoDB:  
SERVERPROXY_UNITTEST_MONGODB_USER
SERVERPROXY_UNITTEST_MONGODB_PWD
SERVERPROXY_UNITTEST_MONGODB_EP
SERVERPROXY_UNITTEST_MONGODB_CA_FILE
SERVERPROXY_UNITTEST_MONGODB_OPTIONS
SERVERPROXY_UNITTEST_MONGODB_START_COMMAND
SERVERPROXY_UNITTEST_MONGODB_STOP_COMMAND
SERVERPROXY_UNITTEST_MONGODB_USE_TLS

For example:

value of SERVERPROXY_UNITTEST_MONGODB_USER is: tester
mongodbUSER=tester

value of SERVERPROXY_UNITTEST_MONGODB_PWD is: xyz123
mongodbPWD=xyz123

value of SERVERPROXY_UNITTEST_MONGODB_EP is: ["lnx-wiotp3.openstack.haifa.ibm.com:27017"]
mongodbEP=["lnx-wiotp3.openstack.haifa.ibm.com:27017"]

value of SERVERPROXY_UNITTEST_MONGODB_CA_FILE is: /home/tock/workspace_act/Server_Proxy_Activity_UnitTest/mongodb-cert.pem
mongodbCAFile=/home/tock/workspace_act/Server_Proxy_Activity_UnitTest/mongodb-cert.pem

value of SERVERPROXY_UNITTEST_MONGODB_OPTIONS is: sslAllowInvalidCertificates=true
mongodbOptions=sslAllowInvalidCertificates=true

value of SERVERPROXY_UNITTEST_MONGODB_START_COMMAND is: /home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/start_MongoDB.sh
mongodbStartCommand=/home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/start_MongoDB.sh

value of SERVERPROXY_UNITTEST_MONGODB_STOP_COMMAND is: /home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/stop_MongoDB.sh
mongodbStopCommand=/home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/stop_MongoDB.sh

value of SERVERPROXY_UNITTEST_MONGODB_USE_TLS is: 1
mongodbUseTLS=1

To adjust trace use the following environment variables:
PXA_UNITTEST_TRACE_DIR
PXA_UNITTEST_TRACE_LEVEL

For example:

value of PXA_UNITTEST_TRACE_DIR is: ./log
value of PXA_UNITTEST_TRACE_LEVEL is: 9
Trace File: ./log/trace_20180418T140150_P1.log is open

The scripts start_MongoDB.sh and stop_MongoDB.sh should contain commands that will start and stop the MongoDB instance the test
is running against. This is used to check the auto-restart on MongoDB failure during run-time.
 
3. Run
======

If the executable is calls 'Server_Proxy_Activity_UnitTest.exe'

Server_Proxy_Activity_UnitTest.exe --log_level=test_suite  &> test.log &

This will run all (87) test cases and will run for ~5 minutes.

The test cases are organized in several test suits, for example to run just the conflation banks tests (which do not require access to mongodb), use:

Server_Proxy_Activity_UnitTest.exe --log_level=test_suite --run_test=Bank_suite &> test.log &

4. Output
=========

The output of a successful run will look like:

--- start ---------------------------------------------------------------------
ServerProxy UnitTest Init:
value of SERVERPROXY_UNITTEST_MONGODB_URI is:  len=0
mongodbURI=
value of SERVERPROXY_UNITTEST_MONGODB_USER is: tester
mongodbUSER=tester
value of SERVERPROXY_UNITTEST_MONGODB_PWD is: xyz123
mongodbPWD=xyz123
value of SERVERPROXY_UNITTEST_MONGODB_EP is: ["lnx-wiotp3.openstack.haifa.ibm.com:27017"]
mongodbEP=["lnx-wiotp3.openstack.haifa.ibm.com:27017"]
value of SERVERPROXY_UNITTEST_MONGODB_CA_FILE is: /home/tock/workspace_act/Server_Proxy_Activity_UnitTest/mongodb-cert.pem
mongodbCAFile=/home/tock/workspace_act/Server_Proxy_Activity_UnitTest/mongodb-cert.pem
value of SERVERPROXY_UNITTEST_MONGODB_OPTIONS is: sslAllowInvalidCertificates=true
mongodbOptions=sslAllowInvalidCertificates=true
value of SERVERPROXY_UNITTEST_MONGODB_START_COMMAND is: /home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/start_MongoDB.sh
mongodbStartCommand=/home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/start_MongoDB.sh
value of SERVERPROXY_UNITTEST_MONGODB_STOP_COMMAND is: /home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/stop_MongoDB.sh
mongodbStopCommand=/home/tock/workspace_act/Server_Proxy_Activity_UnitTest/scripts/stop_MongoDB.sh
value of SERVERPROXY_UNITTEST_MONGODB_USE_TLS is: 1
mongodbUseTLS=1

PXActivity UnitTest Trace Init:
value of PXA_UNITTEST_TRACE_DIR is: ./log
value of PXA_UNITTEST_TRACE_LEVEL is: 9
Trace File: ./log/trace_20180418T142724_P1.log is open

Running 87 test cases...
... omitted ...

*** No errors detected
--- end -----------------------------------------------------------------------


Whereas a failed run will end with, e.g.
--- start ---------------------------------------------------------------------
... omitted ...
*** 8 failures detected in test suite "Server_Proxy_UnitTestMain"
--- end ----------------------------------------------------------------------- 

The return code from the executable will also reflect whether the test passed or failed.
