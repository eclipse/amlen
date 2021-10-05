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
 * UnitTestMain.cpp
 */


#define BOOST_TEST_MODULE Server_Proxy_UnitTestMain

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <chrono>

#include "ServerProxyUnitTestInit.h"
#include "PXActivityUnitTestTraceInit.h"

#include "ismutil.h"

#include "ActivityUtils.h"
#include "MurmurHash3.h"

//Stub for proxy.c variable
int g_rc = -1;

BOOST_GLOBAL_FIXTURE(PXClusterUnitTestTraceInit);
BOOST_GLOBAL_FIXTURE(ServerProxyUnitTestInit);


int timer_action1(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    static int count = 0;
    std::cout << "Done: " << (char*)userdata << " count=" << ++count << std::endl;
    ism_common_cancelTimer(key);
    return 0;
}


int timer_action2(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    static int count = 0;
    std::cout << "Done: " << (char*)userdata << " count=" << ++count << std::endl;
    return 1;
}

BOOST_AUTO_TEST_SUITE( Util_suite )

/**
 * Just say hello
 */
BOOST_AUTO_TEST_CASE( HelloWorld )
{
	BOOST_TEST_MESSAGE("==>>");

	std::cout << "This is thread (main): " << boost::this_thread::get_id() << std::endl;
	std::string cs0( "Hello World!" );
	BOOST_CHECK_EQUAL( cs0.length(), (size_t)12 );

	std::cout << cs0 << std::endl;

	for (int i=9; i>=0; --i)
	{
	    TRACE(i,"Hello dear! trace level %d",i);
	}

}

BOOST_AUTO_TEST_CASE( test_linked_list )
{
    BOOST_TEST_MESSAGE("==>>");

    std::list<int> myList;
    myList.push_front(1);
    std::list<int>::iterator pos1 = myList.begin();

    myList.push_front(2);
    std::list<int>::iterator pos2 = myList.begin();
    BOOST_CHECK(*pos1 == 1);

    myList.push_front(3);
    BOOST_CHECK(*pos1 == 1);
    myList.erase(pos2);

    BOOST_CHECK(*pos1 == 1);
}

BOOST_AUTO_TEST_CASE( test_linked_list_spr )
{
    BOOST_TEST_MESSAGE("==>>");
    using namespace std;
    using ISPtr = shared_ptr<int>;

    list<ISPtr> myList;


    ISPtr i1(new int(1));
    ISPtr i2(new int(2));
    ISPtr i3(new int(3));
    ISPtr i4(new int(4));
    ISPtr i5(new int(5));

    myList.push_back(i1);
    myList.push_back(i2);
    myList.push_back(i3);
    myList.push_back(i4);
    myList.push_back(i5);

    for (auto i : myList)
    {
    	cout << (*i) << " ";
    }
    cout << endl;

	cout << "i1: " << (*i1) << " i2: " << (*i2) << " i3: " << (*i3) << " i4: " << (*i4) << " i5: " << (*i5) << endl;

    myList.reverse();

    for (auto i : myList)
    {
    	cout << (*i) << " ";
    }
    cout << endl;

	cout << "i1: " << (*i1) << " i2: " << (*i2) << " i3: " << (*i3) << " i4: " << (*i4) << " i5: " << (*i5) << endl;

    list<ISPtr>::iterator f = myList.begin();
    f++;
    list<ISPtr>::reverse_iterator b = myList.rbegin();
    b++;

    cout << "f: " << *(*f) << " b: " << *(*b) << endl;
    (*f).swap(*b);
    cout << "f: " << *(*f) << " b: " << *(*b) << endl;

    cout << "i1: " << (*i1) << " i2: " << (*i2) << " i3: " << (*i3) << " i4: " << (*i4) << " i5: " << (*i5) << endl;

    for (auto i : myList)
    {
    	cout << (*i) << " ";
    }
    cout << endl;

}


BOOST_AUTO_TEST_CASE( test_timer_task )
{
    BOOST_TEST_MESSAGE("==>>");

    std::string str("One time job! ");
    ism_time_t delay_nanos = 1;
    ism_common_setTimerOnce(ISM_TIMER_HIGH, timer_action1, (void*)str.c_str(), delay_nanos);

    std::string strm("Many time job! ");
    ism_timer_t timer2 = ism_common_setTimerRate(ISM_TIMER_LOW, timer_action2, (void*)strm.c_str(), 1, 200, TS_MILLISECONDS);

    sleep(2);

    ism_common_cancelTimer(timer2);

    std::cout << "readTSC=" << ism_common_readTSC()
    		<< " chrono::duration="
			<< std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() << std::endl;
}

BOOST_AUTO_TEST_CASE( openssl_prng )
{
	for (int i=8; i<=16; ++i)
		std::cout << ism_proxy::generateProxyUID(i) << std::endl;
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( String_suite )

BOOST_AUTO_TEST_CASE( cstring_hash_equals )
{
	using namespace std;
	using namespace ism_proxy;

	const char* s = "abcdfghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRST1234567890-_abcdfghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRST1234567890-_";
	char array1[256] = {'\0'};
	char array2[256] = {'\0'};
	strcpy(array1, s);
	strcpy(array2+1, s);
	const char* s_alligned = array1;
	const char* s_nonalligned = array2+1;

	cout << " addr=" <<(void*)(s_alligned)  << " 32bit alligned: " << (((uint64_t)(s_alligned))%4 == 0) <<  " " << s_alligned << endl
		 << " addr=" <<(void*)(s_nonalligned)  << " 32bit alligned: " << (((uint64_t)(s_nonalligned))%4 == 0) << " " << s_nonalligned << endl;

	BOOST_CHECK(((uint64_t)(s_alligned))%4 == 0);
	BOOST_CHECK(((uint64_t)(s_nonalligned))%4 != 0);
	CStringKR2Hash h;
	size_t h1 = h(s_alligned);
	size_t h2 = h(s_nonalligned);
	cout << h1 << " " << h2 << endl;
	BOOST_CHECK(h1 == h2);
	CStringEquals eq;
	BOOST_CHECK(eq(s_alligned,s_nonalligned));

	const char* s1 = "abcdefg";
	const char* s2 = "bcdefgh";
	CStringLess cless;
	BOOST_CHECK(strcmp(s1,s2) < 0);
	BOOST_CHECK(cless(s1,s2));
	BOOST_CHECK(!cless(s2,s1));

	CStringGreater cgreater;
	BOOST_CHECK(strcmp(s1,s2) < 0);
	BOOST_CHECK(!cgreater(s1,s2));
	BOOST_CHECK(cgreater(s2,s1));
}


BOOST_AUTO_TEST_CASE( murmur3_hash )
{
	using namespace std;
	using namespace ism_proxy;

	const char* s = "abcdfghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRST1234567890-_abcdfghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRST1234567890-_";
	char array1[256] = {'\0'};
	char array2[256] = {'\0'};
	strcpy(array1, s);
	strcpy(array2+1, s);
	const char* s_alligned = array1;
	const char* s_nonalligned = array2+1;

	cout << " addr=" <<(void*)(s_alligned)  << " 32bit alligned: " << (((uint64_t)(s_alligned))%4 == 0) <<  " " << s_alligned << endl
		 << " addr=" <<(void*)(s_nonalligned)  << " 32bit alligned: " << (((uint64_t)(s_nonalligned))%4 == 0) << " " << s_nonalligned << endl;

	BOOST_CHECK(((uint64_t)(s_alligned))%4 == 0);
	BOOST_CHECK(((uint64_t)(s_nonalligned))%4 != 0);



	uint32_t h1;
	MurmurHash3_x86_32(s_alligned, strlen(s_alligned), 17, &h1);
	uint32_t h2;
	MurmurHash3_x86_32(s_nonalligned, strlen(s_nonalligned), 17, &h2);

	cout << h1 << " " << h2 << endl;
	BOOST_CHECK(h1 == h2);


	CStringMurmur3Hash murmur3;
	uint32_t h1a = murmur3(s_alligned);
	uint32_t h2a = murmur3(s_nonalligned);
	cout << h1a << " " << h2a << endl;
	BOOST_CHECK(h1a == h2a);
	BOOST_CHECK(h1a == h1);
	BOOST_CHECK(h2a == h2);
}

BOOST_AUTO_TEST_CASE( Murmur3_vs_KR2_s128 )
{
	using namespace std;
	using namespace ism_proxy;

	const char* s = "abcdfghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-_abcdfghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-_";
	char array1[256] = {'\0'};
	char array2[256] = {'\0'};
	strcpy(array1, s);
	strcpy(array2+1, s);
	const char* s_alligned = array1;
	const char* s_nonalligned = array2+1;
	BOOST_CHECK(((uint64_t)(s_alligned))%4 == 0);
	BOOST_CHECK(((uint64_t)(s_nonalligned))%4 != 0);

	unsigned ROUNDS = 1000000;
	//========
	cout << "Aligned comparison:" << endl;
	CStringMurmur3Hash murmur3;
	size_t h = 0;
	auto start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += murmur3(s_alligned);
	}
	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> diff = end-start;

	cout << "hash-sum: " << h << " Murmur3 time: " << diff.count() << " s" << endl;

	CStringKR2Hash kr2;
	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += kr2(s_alligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " KR2 time: " << diff.count() << " s" << endl;

	CStringDJB2Hash djb2;
	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += djb2(s_alligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " DJB2 time: " << diff.count() << " s" << endl;

	CStringFNV1aHash fnv1;
	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += fnv1(s_alligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " FNV1a time: " << diff.count() << " s" << endl;

	//========
	cout << "Non-Aligned comparison:" << endl;
	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += murmur3(s_nonalligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " Murmur3 time: " << diff.count() << " s" << endl;


	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += kr2(s_nonalligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " KR2 time: " << diff.count() << " s" << endl;

	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += djb2(s_nonalligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " DJB2 time: " << diff.count() << " s" << endl;

	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += fnv1(s_nonalligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " FNV1a time: " << diff.count() << " s" << endl;


	h = 0;
	std::hash<string> stdh;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		std::string str(s_alligned);
		h += stdh(str);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " std::hash time: " << diff.count() << " s" << endl;

}

BOOST_AUTO_TEST_CASE( Murmur3_vs_KR2_s32 )
{
	using namespace std;
	using namespace ism_proxy;

	const char* s = "abcdefghijklmnopqrstuvwxyzABCDEF";
	char array1[256] = {'\0'};
	char array2[256] = {'\0'};
	strcpy(array1, s);
	strcpy(array2+1, s);
	const char* s_alligned = array1;
	const char* s_nonalligned = array2+1;
	BOOST_CHECK(((uint64_t)(s_alligned))%4 == 0);
	BOOST_CHECK(((uint64_t)(s_nonalligned))%4 != 0);

	unsigned ROUNDS = 1000000;
	//========
	cout << "Aligned comparison:" << endl;
	CStringMurmur3Hash murmur3;
	size_t h = 0;
	auto start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += murmur3(s_alligned);
	}
	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> diff = end-start;

	cout << "hash-sum: " << h << " Murmur3 time: " << diff.count() << " s" << endl;

	CStringKR2Hash kr2;
	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += kr2(s_alligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " KR2 time: " << diff.count() << " s" << endl;

	CStringDJB2Hash djb2;
	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += djb2(s_alligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " DJB2 time: " << diff.count() << " s" << endl;

	CStringFNV1aHash fnv1;
	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += fnv1(s_alligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " FNV1a time: " << diff.count() << " s" << endl;

	//========
	cout << "Non-Aligned comparison:" << endl;
	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += murmur3(s_nonalligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " Murmur3 time: " << diff.count() << " s" << endl;


	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += kr2(s_nonalligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " KR2 time: " << diff.count() << " s" << endl;

	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += djb2(s_nonalligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " DJB2 time: " << diff.count() << " s" << endl;

	h = 0;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		h += fnv1(s_nonalligned);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " FNV1a time: " << diff.count() << " s" << endl;


	h = 0;
	std::hash<string> stdh;
	start = std::chrono::system_clock::now();
	for (unsigned i = 0; i<ROUNDS; ++i)
	{
		std::string str(s_alligned);
		h += stdh(str);
	}
	end = std::chrono::system_clock::now();
	diff = end-start;

	cout << "hash-sum: " << h << " std::hash time: " << diff.count() << " s" << endl;

}



BOOST_AUTO_TEST_SUITE_END()


