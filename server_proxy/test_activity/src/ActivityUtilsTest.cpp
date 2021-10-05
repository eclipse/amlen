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



#include <thread>
#include <memory>
#include <string>
#include <chrono>
#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>

#include "mongoc.h"
#include "pxactivity.h"
#include "ActivityUtils.h"


#include <mongoc.h>

#include "ServerProxyUnitTestInit.h"


int timer_refill_token_bucket(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
	if (userdata)
	{
		ism_proxy::TokenBucket* bucket = reinterpret_cast<	ism_proxy::TokenBucket*>(userdata);
		int rc = bucket->refill();
		if (rc == ISMRC_OK)
			return 1;
	}

    return 0;
}

BOOST_AUTO_TEST_SUITE( Util_suite )

BOOST_AUTO_TEST_SUITE( TB_suite )

/*
 * When the task refills with one or more tokens
 */
BOOST_AUTO_TEST_CASE( token_bucket_fast )
{
	using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    TokenBucket * bucket = new TokenBucket(1000, 2000);
    BOOST_CHECK(bucket);

    ism_timer_t timer2 = ism_common_setTimerRate(ISM_TIMER_LOW, timer_refill_token_bucket, (void*)bucket,
    		TokenBucket::REFILL_INTERVAL_MILLIS, TokenBucket::REFILL_INTERVAL_MILLIS, TS_MILLISECONDS);

    int rc = ISMRC_OK;
    BOOST_CHECK_EQUAL(bucket->getBucketSize(), 2000);
    BOOST_CHECK_EQUAL(bucket->getLimit(), 1000);

    rc = bucket->getTokens(500);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = bucket->getTokens(500);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = bucket->getTokens(5000);
    BOOST_CHECK_EQUAL(rc, ISMRC_Error);

    sleep(1);

    auto start = std::chrono::system_clock::now();
    int count = 10000;
    while (count > 0)
    {
    	rc = bucket->getTokens(1);
    	if (rc == ISMRC_OK)
    	{
    		count--;
    	}
    	else
    	{
    		BOOST_CHECK(false);
    		break;
    	}
    }

    auto finish = std::chrono::system_clock::now();
    double diff = std::chrono::duration<double>(finish-start).count();
    BOOST_CHECK(diff > ((10000.0 - 2000)/1000) * 0.9);
    std::cout << "TB time to get 10000 tokens @1000tps = " << diff << "S"<< std::endl;

    ism_common_cancelTimer(timer2);

    delete bucket;

}

/*
 * When the task refills with a fraction of a token
 */
BOOST_AUTO_TEST_CASE( token_bucket_slow )
{
	using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    TokenBucket * bucket = new TokenBucket(10, 20);
    BOOST_CHECK(bucket);

    ism_timer_t timer2 = ism_common_setTimerRate(ISM_TIMER_LOW, timer_refill_token_bucket, (void*)bucket,
    		TokenBucket::REFILL_INTERVAL_MILLIS, TokenBucket::REFILL_INTERVAL_MILLIS, TS_MILLISECONDS);

    int rc = ISMRC_OK;
    BOOST_CHECK_EQUAL(bucket->getBucketSize(), 20);
    BOOST_CHECK_EQUAL(bucket->getLimit(), 10);

    rc = bucket->getTokens(5);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);
    rc = bucket->getTokens(5);
    BOOST_CHECK_EQUAL(rc, ISMRC_OK);

    rc = bucket->getTokens(5000);
    BOOST_CHECK_EQUAL(rc, ISMRC_Error);

    sleep(1);

    auto start = std::chrono::system_clock::now();
    int count = 100;
    while (count > 0)
    {
    	rc = bucket->getTokens(1);
    	if (rc == ISMRC_OK)
    	{
    		count--;
    	}
    	else
    	{
    		BOOST_CHECK(false);
    		break;
    	}
    }

    auto finish = std::chrono::system_clock::now();
    double diff = std::chrono::duration<double>(finish-start).count();
    BOOST_CHECK(diff > ((100.0 - 20)/10) * 0.9);
    std::cout << "TB time to get 100 tokens @10tps= " << diff << "S"<< std::endl;

    ism_common_cancelTimer(timer2);

    delete bucket;

}


/*
 * When the task refills with a fraction of a token
 */
BOOST_AUTO_TEST_CASE( token_bucket_try)
{
	using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    TokenBucket * bucket = new TokenBucket(10, 20);
    BOOST_CHECK(bucket);

    ism_timer_t timer2 = ism_common_setTimerRate(ISM_TIMER_LOW, timer_refill_token_bucket, (void*)bucket,
    		TokenBucket::REFILL_INTERVAL_MILLIS, TokenBucket::REFILL_INTERVAL_MILLIS, TS_MILLISECONDS);

    int rc = ISMRC_OK;
    BOOST_CHECK_EQUAL(bucket->getBucketSize(), 20);
    BOOST_CHECK_EQUAL(bucket->getLimit(), 10);

    auto start = std::chrono::system_clock::now();
    int count = 50;
    int would_block = 0;
    while (count > 0)
    {
    	rc = bucket->tryGetTokens(1);
    	if (rc == ISMRC_OK)
    	{
    		count--;
    	}
    	else if (rc == ISMRC_WouldBlock)
    	{
    		would_block++;
    		usleep(1000);
    	}
    	else
    	{
    		BOOST_CHECK(false);
    		break;
    	}
    }

    BOOST_CHECK(would_block > 0);
    auto finish = std::chrono::system_clock::now();
    double diff = std::chrono::duration<double>(finish-start).count();
    BOOST_CHECK(diff > ((50.0 - 20)/10) * 0.9);
    std::cout << "TB time to get 100 tokens @10tps= " << diff << "S"<< std::endl;

    ism_common_cancelTimer(timer2);

    delete bucket;
}
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( URI_suite )


BOOST_AUTO_TEST_CASE( trim_uri )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    string uri1 = " aaaa ";
    string uri1_t = "aaaa";

    trim_inplace(uri1);
    BOOST_CHECK_EQUAL(uri1, uri1_t);

    string uri2 = "  ";
    string uri2_t = "";
    trim_inplace(uri2);
    BOOST_CHECK_EQUAL(uri2, uri2_t);

}

BOOST_AUTO_TEST_CASE( build_uri )
{
	using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    string user1 = "aaaa";
    string pwd1 = "xyz";

    string user2 = "bbbb";
    string pwd2 = "uvw";

    string user3 = "bb@b";
    string pwd3 = "u@w";

    string user4 = "bb%40b";
    string pwd4 = "u%3Aw";

    string pwd_mask = "********";

    string uri1 = "mongodb://server1.com:11111,server2.com:2222/mydb?replicaSet=rs0,ssl=true";
    string uri1c = "mongodb://" + user1 + ":" + pwd1 + "@server1.com:11111,server2.com:2222/mydb?replicaSet=rs0,ssl=true";

    //insert
    string uri = ActivityDBConfig::build_uri(uri1, user1, pwd1);
    BOOST_CHECK_EQUAL(uri, uri1c);
    //ignore
    uri = ActivityDBConfig::build_uri(uri1, "", pwd1);
    BOOST_CHECK_EQUAL(uri, uri1);

    string uri2 = "mongodb://" +  user2 + ":" + pwd2 + "@server3.com:11111,server4.com:2222/?replicaSet=rs1";
    string uri2c = "mongodb://" +  user1 + ":" + pwd1 + "@server3.com:11111,server4.com:2222/?replicaSet=rs1";

    //replace
    uri = ActivityDBConfig::build_uri(uri2, user1, pwd1);
    BOOST_CHECK_EQUAL(uri, uri2c);

    //user:password has @ in it
    string uri3 = "mongodb://" +  user3 + ":" + pwd3 + "@server3.com:11111,server4.com:2222/mydb?replicaSet=rs1";

    mongoc_uri_t* pUri = mongoc_uri_new(uri3.c_str());
    BOOST_CHECK(pUri == NULL);

    uri = ActivityDBConfig::build_uri(uri3, user1, pwd1);
    BOOST_CHECK(uri.empty());

    //user:password has @ : in it, percent encoding
    string uri4 = "mongodb://" +  user4 + ":" + pwd4 + "@server3.com:11111,server4.com:2222/mydb?replicaSet=rs1";
    string uri4c = "mongodb://" +  user1 + ":" + pwd1 + "@server3.com:11111,server4.com:2222/mydb?replicaSet=rs1";

    pUri = mongoc_uri_new(uri4.c_str());
    BOOST_CHECK(pUri != NULL);
    const char* user = mongoc_uri_get_username(pUri);
    const char* pwd = mongoc_uri_get_password(pUri);
    cout << "user=" << user << " pwd=" << pwd << endl;
    mongoc_uri_destroy(pUri);

    uri = ActivityDBConfig::build_uri(uri4, user1, pwd1);
    BOOST_CHECK_EQUAL(uri, uri4c);
}


BOOST_AUTO_TEST_CASE( assemble_uri )
{
    using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    string user1 = "aaaa";
    string pwd1 = "xyz";

    string endpoint = "[\"server1.com:11111\", \"server2.com:2222\"]";
    string db = "mydb";
    int useTLS = 1;
    string caFile = "/tmp/myca.pem";
    string options = "replicaSet=rs0";

    string uri1c = "mongodb://" + user1 + ":" + pwd1 + "@server1.com:11111,server2.com:2222/mydb?ssl=true&sslCertificateAuthorityFile=/tmp/myca.pem&replicaSet=rs0";

    //assemble
    string uri = ActivityDBConfig::assemble_uri(user1,pwd1,endpoint, db, useTLS, caFile, options);
    cout << uri << endl;
    BOOST_CHECK_EQUAL(uri, uri1c);

    db="";
    uri1c = "mongodb://" + user1 + ":" + pwd1 + "@server1.com:11111,server2.com:2222/?ssl=true&sslCertificateAuthorityFile=/tmp/myca.pem&replicaSet=rs0";
    uri = ActivityDBConfig::assemble_uri(user1,pwd1,endpoint, db, useTLS, caFile, options);
    cout << uri << endl;
    BOOST_CHECK_EQUAL(uri, uri1c);

    options = "";
    uri1c = "mongodb://" + user1 + ":" + pwd1 + "@server1.com:11111,server2.com:2222/?ssl=true&sslCertificateAuthorityFile=/tmp/myca.pem";
    uri = ActivityDBConfig::assemble_uri(user1,pwd1,endpoint, db, useTLS, caFile, options);
    cout << uri << endl;
    BOOST_CHECK_EQUAL(uri, uri1c);

    options = "replicaSet=rs1";
    useTLS=0;
    uri1c = "mongodb://" + user1 + ":" + pwd1 + "@server1.com:11111,server2.com:2222/?replicaSet=rs1";
    uri = ActivityDBConfig::assemble_uri(user1,pwd1,endpoint, db, useTLS, caFile, options);
    cout << uri << endl;
    BOOST_CHECK_EQUAL(uri, uri1c);

    options = "";
    uri1c = "mongodb://server1.com:11111,server2.com:2222/";
    uri = ActivityDBConfig::assemble_uri("","",endpoint, db, useTLS, caFile, options);
    cout << uri << endl;
    BOOST_CHECK_EQUAL(uri, uri1c);


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
    endpoint = oss.str();
    cout << "Too long: " << endpoint << endl;
    uri = ActivityDBConfig::assemble_uri("","",endpoint, db, useTLS, caFile, options);
    BOOST_CHECK(uri.empty());
}


BOOST_AUTO_TEST_CASE( mask_uri )
{
	using namespace std;
    using namespace ism_proxy;

    BOOST_TEST_MESSAGE("==>>");

    string user1 = "aaaa";
    string pwd1 = "xyz";

    string user2 = "bb@b";
    string pwd2 = "u@w";

    string user3 = "bb:b";
    string pwd3 = "u:w";

    string user4 = "bb%40b";
    string pwd4 = "u%3Aw";

    string pwd_mask = "********";

    string uri;

    string uri1 = "mongodb://" + user1 + ":" + pwd1 + "@server1.com:11111,server2.com:2222/mydb?replicaSet=rs0,ssl=true";
    string uri1m = "mongodb://" + user1 + ":" + pwd_mask + "@server1.com:11111,server2.com:2222/mydb?replicaSet=rs0,ssl=true";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, uri1m);

    uri1 = "mongodb://" + user1 + ":" + pwd1 + "@server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri1m = "mongodb://" + user1 + ":" + pwd_mask + "@server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, uri1m);

    uri1 = "mongodb://" + user1 + ":" + pwd1 + "@server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri1m = "mongodb://" + user1 + ":" + pwd_mask + "@server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, uri1m);

    uri1 = "mongodb://" + user1 + ":" + pwd1 + "@server1.com:11111,server2.com:2222";
    uri1m = "mongodb://" + user1 + ":" + pwd_mask + "@server1.com:11111,server2.com:2222";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, uri1m);

    uri1 = "mongodb://" + user1 + ":" + pwd1 + "@server1.com:11111,server2.com:2222/?@=@";
    uri1m = "mongodb://" + user1 + ":" + pwd_mask + "@server1.com:11111,server2.com:2222/?@=@";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, uri1m);

    uri1 = "mongodb://server1.com:11111,server2.com:2222/?@=@";
    uri1m = "mongodb://server1.com:11111,server2.com:2222/?@=@";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, uri1m);


    uri1 = "mongodb://" + user2 + ":" + pwd2 + "@server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, string(""));


    uri1 = "mongodb://" + user3 + ":" + pwd3 + "@server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, string(""));


    uri1 = "mongodb://" + user2 + ":" + pwd3 + "@server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, string(""));

    uri1 = "mongodb://" + user3 + ":" + pwd2 + "@server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, string(""));

    uri1 = "mongodb://" + user4 + ":" + pwd4 + "@server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri1m = "mongodb://" + user4 + ":" + pwd_mask + "@server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, uri1m);

    uri1 = "mongodb://server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri1m = "mongodb://server1.com:11111,server2.com:2222/?replicaSet=rs0,ssl=true";
    uri = ActivityDBConfig::mask_pwd_uri(uri1);
    BOOST_CHECK_EQUAL(uri, uri1m);


}

BOOST_AUTO_TEST_CASE( query )
{
    using namespace std;
    using namespace ism_proxy;
//query = {status: {$in: [1,2,3] }}
//bson_t* query = BCON_NEW(ProxyStateRecord::KEY_STATUS,BCON_INT32(status));

    ostringstream q;
    q << "{ \"status\" : { \"$in\" : [ " << 1 << ", " << 2 << " ] } }";
    string qs = q.str();
    std::cout << qs << std::endl;
    bson_error_t error;
    bson_t* query = bson_new_from_json((const uint8_t*)qs.c_str(), -1, &error);

    if (query)
    {
        std::cout << bson_as_json(query, NULL) << std::endl;
        bson_destroy(query);
    }
    else
    {
        std::cout << error.domain << " " << error.code << " " << error.message << std::endl;
        BOOST_CHECK(false);
    }



}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( QS_suite )

BOOST_AUTO_TEST_CASE( integers )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	for (size_t N=200; N<1001; N+=100)
	{
		for (size_t K = 1; K<11; ++K)
		{
//			cout << "k/N=" << K << "/" << N << endl;
			vector<int> array;
			array.reserve(N);

			for (unsigned i=0; i<N; ++i)
			{
				array.push_back(rand()%1000);
			}

			vector<int> array_sorted = array;
			std::sort(array_sorted.begin(), array_sorted.end());
			std::set<int> smallest;
			for (unsigned j=0; j<K; ++j)
			{
				smallest.insert(array_sorted[j]);
			}

			int k_th = quickselect<int>(array, K);

//			for (unsigned i=0; i<K; ++i )
//			{
//				cout << array[i] << " ";
//			}
//			cout << endl;
//
//			for (unsigned i=0; i<K; ++i )
//			{
//				cout << array_sorted[i] << " ";
//			}
//			cout << endl;

			BOOST_CHECK_EQUAL(k_th, array_sorted[K-1]);

			for (unsigned j=0; j<K; ++j)
			{
				BOOST_CHECK(smallest.count(array[j])>0);
			}
		}
	}
}

BOOST_AUTO_TEST_CASE( integers_deque )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	for (size_t N=200; N<1001; N+=100)
	{
		for (size_t K = 1; K<11; ++K)
		{
//			cout << "k/N=" << K << "/" << N << endl;
			deque<int> array;

			for (unsigned i=0; i<N; ++i)
			{
				array.push_back(rand()%1000);
			}

			deque<int> array_sorted = array;
			std::sort(array_sorted.begin(), array_sorted.end());
			std::set<int> smallest;
			for (unsigned j=0; j<K; ++j)
			{
				smallest.insert(array_sorted[j]);
			}

			int k_th = quickselect<int, deque<int> >(array, K);

//			for (unsigned i=0; i<K; ++i )
//			{
//				cout << array[i] << " ";
//			}
//			cout << endl;
//
//			for (unsigned i=0; i<K; ++i )
//			{
//				cout << array_sorted[i] << " ";
//			}
//			cout << endl;

			BOOST_CHECK_EQUAL(k_th, array_sorted[K-1]);

			for (unsigned j=0; j<K; ++j)
			{
				BOOST_CHECK(smallest.count(array[j])>0);
			}
		}
	}
}


BOOST_AUTO_TEST_CASE( integers_nth )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	for (size_t N=200; N<1001; N+=100)
	{
		for (size_t K = 1; K<11; ++K)
		{
			cout << "k/N=" << K << "/" << N << endl;
			vector<int> array;
			array.reserve(N);

			for (unsigned i=0; i<N; ++i)
			{
				array.push_back(rand()%1000);
			}

			vector<int> array_sorted = array;
			std::sort(array_sorted.begin(), array_sorted.end());
			std::set<int> smallest;
			for (unsigned j=0; j<K; ++j)
			{
				smallest.insert(array_sorted[j]);
			}

			vector<int>::iterator k_th = array.begin()+K-1;
			nth_element(array.begin(), k_th, array.end());

//			for (unsigned i=0; i<K; ++i )
//			{
//				cout << array[i] << " ";
//			}
//			cout << endl;
//
//			for (unsigned i=0; i<K; ++i )
//			{
//				cout << array_sorted[i] << " ";
//			}
//			cout << endl;

			BOOST_CHECK_EQUAL(*k_th, array_sorted[K-1]);

			for (unsigned j=0; j<K; ++j)
			{
				BOOST_CHECK(smallest.count(array[j])>0);
			}
			cout << "---------------------------------------------" << endl;
		}
	}
}

BOOST_AUTO_TEST_CASE( pairs )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	for (size_t N = 200; N < 1001; N += 100)
	{
		for (size_t K = 1; K <=N; ++K)
		{
			using DC_Pair = pair<double, const char*>;
			vector<DC_Pair> array;
			array.reserve(N);

			const char* odd = "odd";
			const char* even = "even";
			for (unsigned i = 0; i < N; ++i)
			{
				int r = rand() % 1000;
				array.push_back(make_pair(static_cast<double>(r) / 10, i % 2 ? odd : even));
			}

			vector<DC_Pair> array_sorted = array;
			std::sort(array_sorted.begin(), array_sorted.end());
			std::set<DC_Pair> smallest;
			for (unsigned j = 0; j < K; ++j)
			{
				smallest.insert(array_sorted[j]);
			}

			DC_Pair k_th = quickselect<DC_Pair>(array, K);

			BOOST_CHECK(k_th == array_sorted[K - 1]);

			for (unsigned j = 0; j < K; ++j)
			{
				BOOST_CHECK(smallest.count(array[j]) > 0);
			}
		}
	}
}

BOOST_AUTO_TEST_CASE( pairs_nth )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	for (size_t N = 200; N < 1001; N += 100)
	{
		for (size_t K = 1; K <=N; ++K)
		{
			using DC_Pair = pair<double, const char*>;
			deque<DC_Pair> array;

			const char* odd = "odd";
			const char* even = "even";
			for (unsigned i = 0; i < N; ++i)
			{
				int r = rand() % 1000;
				array.push_back(make_pair(static_cast<double>(r) / 10, i % 2 ? odd : even));
			}

			deque<DC_Pair> array_sorted = array;
			std::sort(array_sorted.begin(), array_sorted.end());
			std::set<DC_Pair> smallest;
			for (unsigned j = 0; j < K; ++j)
			{
				smallest.insert(array_sorted[j]);
			}

			deque<DC_Pair>::iterator k_th = array.begin()+K-1;
			nth_element(array.begin(), k_th, array.end());

			BOOST_CHECK(*k_th == array_sorted[K - 1]);

			for (unsigned j = 0; j < K; ++j)
			{
				BOOST_CHECK(smallest.count(array.front()) > 0);
				array.pop_front();
			}
		}
	}
}


BOOST_AUTO_TEST_CASE( updates_nth )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	for (size_t N = 200; N < 1001; N += 100)
	{
		for (size_t K = 1; K <=N; ++K)
		{
			using UpdateDeque = deque<ClientUpdateRecord_SPtr>;
			UpdateDeque array;

			const char* clean = "clean";
			const char* dirty = "dirty";
			for (unsigned i = 0; i < N; ++i)
			{
				string name((i%2 ? dirty : clean));
				name += to_string(i);
				ClientUpdateRecord_SPtr rec(new ClientUpdateRecord( name.c_str(), "org", "type", "devid", PXACT_CLIENT_TYPE_DEVICE, ""));
				rec->dirty_flags = (i%2 ? 1 : 0);
				rec->update_timestamp = static_cast<double>(rand() % 1000) / 100.0;
				array.push_back(rec);
			}


			UpdateDeque array_sorted = array;
			std::sort(array_sorted.begin(), array_sorted.end());
			std::set<ClientUpdateRecord_SPtr, Compare_TS_Less> smallest;

			for (unsigned j = 0; j < K; ++j)
			{
				smallest.insert(array_sorted[j]);
			}

			UpdateDeque::iterator k_th = array.begin()+K-1;
			nth_element(array.begin(), k_th, array.end());

			BOOST_CHECK(*k_th == array_sorted[K-1]);

			for (unsigned j = 0; j < K; ++j)
			{
				BOOST_CHECK(smallest.count(array.front()) > 0);
				array.pop_front();
			}
		}
	}
}


BOOST_AUTO_TEST_CASE( updates_deque )
{
	using namespace std;
	using namespace ism_proxy;

	BOOST_TEST_MESSAGE("==>>");

	for (size_t N = 200; N < 1001; N += 100)
	{
		for (size_t K = 1; K <=N; ++K)
		{
//	size_t N=20;
//	size_t K=5;

	using UpdateDeque = deque<ClientUpdateRecord_SPtr>;
	UpdateDeque array;

	const char* clean = "clean";
	const char* dirty = "dirty";
	for (unsigned i = 0; i < N; ++i)
	{
		string name((i%2 ? dirty : clean));
		name += to_string(i);
		ClientUpdateRecord_SPtr rec(new ClientUpdateRecord( name.c_str(), "org", "type", "devid", PXACT_CLIENT_TYPE_DEVICE, ""));
		rec->dirty_flags = (i%2 ? 1 : 0);
		rec->update_timestamp = static_cast<double>(rand() % 1000) / 100.0;
		array.push_back(rec);
	}


	UpdateDeque array_sorted = array;
	std::sort(array_sorted.begin(), array_sorted.end(), compare_ts_less);
	std::set<ClientUpdateRecord_SPtr, Compare_TS_Less> smallest;

	for (unsigned j = 0; j < K; ++j)
	{
		smallest.insert(array_sorted[j]);
	}

	auto k_th = quickselect<ClientUpdateRecord_SPtr, UpdateDeque, Compare_TS_Less>(array, K);

	for (unsigned j = 0; j < K; ++j)
	{
		BOOST_CHECK(smallest.count(array.front()) > 0);
		array.pop_front();
	}
		}
	}
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
