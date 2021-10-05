/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
 * File: testHashMapCUnit.c
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>    /* System Time Routines for comparison */
#include <sys/time.h>
#include <limits.h>

#include <ismutil.h>
#include <ismuints.h>
#include "testTimeUtilsCUnit.h"

/*
 * Some time conversion macros
 */
#define LTStoJTS(x) ((x)/1000000)
#define JTStoLTS(x) ((x)*1000000)
#define UTStoLTS(x) (ism_time_t)((x)->tv_sec) * 100000000 + (x)->tv_nsec;           /* timespec */
#define UTVtoLTS(x) (ism_time_t)((x)->tv_sec) * 100000000 + ((x)->tv_usec * 1000);  /* timeval */
#define LTStoSEC(x) ((x)/1000000000)
#define LTStoNS(x)  ((x)%1000000000)

static int  TZ_off = 0;
/**
 * Perform initialization for hash map test suite.
 */
int initTimeUtilsCUnitSuite(void){
    struct timeval tv;
    struct tm  ltime;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &ltime);
    TZ_off = ltime.tm_gmtoff/60;
	return 0;
}

/**
 * Clean up after hash map test suite is executed.
 */
int cleanTimeUtilsCUnitSuite(void){
	return 0;
}

static void CUnit_ISM_getCurrentTime_test(void){
    ism_time_t current_time_in_nano, current_time_in_seconds;
    int64_t time_in_seconds;

    current_time_in_nano = ism_common_currentTimeNanos();

    time_in_seconds = (int64_t)time(NULL);
    /* convert current_time nano seconds to seconds */
    current_time_in_seconds = current_time_in_nano/1000000000;

    CU_ASSERT((time_in_seconds-current_time_in_seconds) < 1 );
}

static void CUnit_ISM_time2Jtime_test(void){
	int64_t current_time_in_jts;
    int64_t Jtime;
	int i;
	ism_time_t test_input[] = { 0, 0, LONG_MAX, LONG_MAX / 2 };

	/* Test1 input:  current time */
	test_input[1] = ism_common_currentTimeNanos();

	for (i = 0; i < 4; i++) {

		Jtime = ism_common_convertTimeToJTime(test_input[i]);
		current_time_in_jts = LTStoJTS(test_input[i]);
		CU_ASSERT(Jtime == current_time_in_jts);
	}
}

static void CUnit_ISM_Jtime2time_test(void){
	int64_t current_time_in_jts;
	ism_time_t ismTime, current_time_in_ism;
	int i;
	ism_time_t test_input[] = { 0, 0, LONG_MAX, LONG_MAX / 2 };

	/* Test1 input:  current time */
	test_input[1] = ism_common_currentTimeNanos();

	for (i = 0; i < 4; i++) {
		current_time_in_jts = LTStoJTS(test_input[i]);
		ismTime = ism_common_convertJTimeToTime(current_time_in_jts);
		current_time_in_ism = JTStoLTS(current_time_in_jts);
		CU_ASSERT(ismTime == current_time_in_ism);
	}
}

void CUnit_ISM_openTimestamp_test(void) {
    ism_ts_t * timestampObj;
    int i;

    int test_input[] = {ISM_TZF_UNDEF, ISM_TZF_UTC,ISM_TZF_LOCAL};

    for( i=0; i<3; i++) {
        timestampObj = ism_common_openTimestamp(test_input[i]);
        CU_ASSERT(timestampObj != NULL);
        ism_common_closeTimestamp(timestampObj);
    }

    timestampObj = ism_common_openTimestamp( ISM_TZF_LOCAL+10 );
    CU_ASSERT(timestampObj == NULL);
    ism_common_closeTimestamp(timestampObj);

    timestampObj = ism_common_openTimestamp( ISM_TZF_LOCAL+1 );
    CU_ASSERT(timestampObj == NULL);
    ism_common_closeTimestamp(timestampObj);
}

void CUnit_ISM_get_set_Timestamp_test(void) {
    ism_ts_t * timestampObj;
    ism_time_t ismTime;
    int i, j;

    ism_time_t test_input[] = {0, 0, INT_MAX, INT_MIN};

    for(i = 0; i < 3; i++){
	    test_input[0] = ism_common_currentTimeNanos();
	    timestampObj = ism_common_openTimestamp(i);
    	for(j = 0; j < 4; j++){
    		 ism_common_setTimestamp(timestampObj,test_input[j]);
    		 ismTime = ism_common_getTimestamp(timestampObj);
    		 CU_ASSERT(ismTime == test_input[j]);
    	}
    	ism_common_closeTimestamp(timestampObj);
    }
}

void CUnit_ISM_get_setTimestampValues(void){
    ism_ts_t * timestampObj;
    ism_timeval_t getResult;
    int rc;
    int i, j;

    ism_timeval_t test_input[15] = {
        { 1902, 1, 1, 0, 0, 0, 0, 3, 0 },
        { 2030, 12, 31, 0, 0, 0, 0, 2, 0 },
        { 2008, 1, 31, 0, 0, 0, 0, 4, 0 },
        { 2009, 12, 1, 0, 0, 0, 0, 2, 0 },
        { 1975, 5, 1, 0, 0, 0, 0, 4, 0 },
        { 1978, 8, 31, 0, 0, 0, 0, 4, 0 },
        { 1955, 6, 5, 1, 0, 0, 0, 0, 0 },
        { 1955, 6, 5, 23, 0, 0, 0, 0, 0 },
        { 1945, 3, 4, 9, 1, 0, 0, 0, 0 },
        { 1945, 3, 4, 15, 59, 0, 0, 0, 0 },
        { 2012, 11, 22, 0, 1, 1, 0, 4, 0 },
        { 2012, 11, 22, 0, 59, 59, 0, 4, 0 },
        { 2030, 10, 25, 0, 0, 0, 1, 5, 0 },
        { 2030, 10, 25, 0, 0, 0, 999999999, 5, 0 },
        { 1917, 9, 27, 0, 0, 0, 0, 4, 0 }
    };

    for( i=0; i<3; i++) {
	    timestampObj = ism_common_openTimestamp(i);

        for( j=0; j<15; j++) {
           ism_timeval_t *tv = &test_input[j];
           rc = ism_common_setTimestampValues(timestampObj, tv, 0);
           CU_ASSERT(rc == 0);
           CU_ASSERT(ism_common_getTimestampValues(timestampObj, &getResult) != NULL);
           CU_ASSERT(getResult.year == tv->year);
           CU_ASSERT(getResult.month == tv->month);
           CU_ASSERT(getResult.day == tv->day);
           CU_ASSERT(getResult.hour == tv->hour);
           CU_ASSERT(getResult.minute == tv->minute);
           CU_ASSERT(getResult.second == tv->second);
           CU_ASSERT(getResult.nanos == tv->nanos);
           CU_ASSERT(getResult.dayofweek == tv->dayofweek);
           if(i == ISM_TZF_LOCAL){
        	   CU_ASSERT(getResult.tzoffset == TZ_off);
           } else {
        	   CU_ASSERT(getResult.tzoffset == tv->tzoffset);
           }
        }

    	ism_common_closeTimestamp(timestampObj);
    }
}

void timestampByLocale_test(void) {
    uint64_t nanos = ism_common_currentTimeNanos();
    char buf[256];
    ism_common_formatTimestampByLocale("en", nanos, buf, sizeof buf);
    printf("Locale=%s=%s\n", "en", buf);
    ism_common_formatTimestampByLocale("en_US", nanos, buf, sizeof buf);
    printf("Locale=%s=%s\n", "en_US", buf);
    ism_common_formatTimestampByLocale("en_GB", nanos, buf, sizeof buf);
    printf("Locale=%s=%s\n", "en_GB", buf);
    ism_common_formatTimestampByLocale("ja", nanos, buf, sizeof buf);
    printf("Locale=%s=%s\n", "ja", buf);
    ism_common_formatTimestampByLocale("fr", nanos, buf, sizeof buf);
    printf("Locale=%s=%s\n", "fr", buf);
    ism_common_formatTimestampByLocale("fr_CA", nanos, buf, sizeof buf);
    printf("Locale=%s=%s\n", "fr_CA", buf);
}

extern const char * * ism_ifmaps;
extern int            ism_ifmap_count;
const char * mymap [] = {"fred0", "eth0", "sam", "eth1"};

/*
 * Interface mapping test
 */
void CUnit_ISM_ifmap(void) {
    ism_ifmaps = mymap;
    ism_ifmap_count = 2;

    CU_ASSERT(ism_common_ifmap_known("fred0", 0)==1);
    CU_ASSERT(ism_common_ifmap_known("SAM",   0)==1);
    CU_ASSERT(ism_common_ifmap_known("eth0",  1)==1);
    CU_ASSERT(ism_common_ifmap_known("Eth1",  1)==1);
    CU_ASSERT(ism_common_ifmap_known("fred0", 1)==0);
    CU_ASSERT(ism_common_ifmap_known("SAM",   1)==0);
    CU_ASSERT(ism_common_ifmap_known("eth0",  0)==0);
    CU_ASSERT(ism_common_ifmap_known("Eth1",  0)==0);

    CU_ASSERT(!strcmp(ism_common_ifmap("fred0", 0), "eth0"));
    CU_ASSERT(!strcmp(ism_common_ifmap("sam", 0),   "eth1"));
    CU_ASSERT(!strcmp(ism_common_ifmap("eth0", 1),  "fred0"));
    CU_ASSERT(!strcmp(ism_common_ifmap("eth1", 1),  "sam"));
    CU_ASSERT(!strcmp(ism_common_ifmap("fREd0", 0), "eth0"));
    CU_ASSERT(!strcmp(ism_common_ifmap("saM", 0),   "eth1"));
    CU_ASSERT(!strcmp(ism_common_ifmap("eTH0", 1),  "fred0"));
    CU_ASSERT(!strcmp(ism_common_ifmap("Eth1", 1),  "sam"));
}

void ism_common_initUUID();

/*
 * TODO: clean up test and do more self checking
 * TODO: add test for error conditions
 */
void CUnit_uuid(void) {
    char ubuf[40];
    char ubuf2[40];
    char ubin[16];
    char tbuf[40];
    int i;
    uint64_t start;
    uint64_t endt;

    ism_common_initUtil();
    ism_common_initUUID();
    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_UNDEF);

    CU_ASSERT(ism_common_newUUID(ubuf, 37, 1, 0) == (const char *)ubuf);
    printf("uuid1=%s\n", ubuf);
    uint64_t utime = ism_common_extractUUIDtime(ubuf);
    CU_ASSERT(utime != 0);
    ism_time_t ntime = ism_common_convertUTimeToTime(utime);
    CU_ASSERT(ntime != 0);
    ism_common_setTimestamp(ts, ntime);
    ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 6, 0);
    printf("date=%s\n", tbuf);

    /* Check with known timestamp */
    CU_ASSERT(ism_common_setTimestampString(ts, "2017-06-01T01") == 0);
    ism_common_newUUID(ubuf, 37, 1, ism_common_getTimestamp(ts));
    utime = ism_common_extractUUIDtime(ubuf);
    CU_ASSERT(utime != 0);
    ntime = ism_common_convertUTimeToTime(utime);
    CU_ASSERT(ntime != 0);
    ism_common_setTimestamp(ts, ntime);
    ism_common_formatTimestamp(ts, tbuf, sizeof tbuf, 6, 0);
    CU_ASSERT(!strcmp(tbuf, "2017-06-01T01:00:00"));

    ism_common_newUUID(ubin, 16, 17, 0);
    ism_common_newUUID(ubuf, 37, 17, 0);
    ism_common_newUUID(ubuf2, 37, 17, 0);
    printf("uuid17a=%s\n", ism_common_UUIDtoString(ubin, tbuf, 37));
    printf("uuid17b=%s\n", ubuf);
    printf("uuid17c=%s\n", ubuf2);
    CU_ASSERT(ism_common_UUIDtoBinary(ubuf, ubin)==ubin);
    CU_ASSERT(ism_common_UUIDtoString(ubin, ubuf2, 37)==ubuf2);
    CU_ASSERT(!strcmp(ubuf, ubuf2));

    /* Time for binary */
    int count = 1000000;
    start = ism_common_currentTimeNanos();
    for (i=0; i<count; i++) {
        ism_common_newUUID(ubin, 16, 17, 0);
    }
    endt = ism_common_currentTimeNanos();
    printf("Create time for binary UUID = %g ns\n", ((double)(endt-start))/count);

    /* Time for string form */
    start = ism_common_currentTimeNanos();
    for (i=0; i<count; i++) {
        ism_common_newUUID(ubuf, 37, 17, 0);
    }
    endt = ism_common_currentTimeNanos();
    printf("Create time for string UUID = %g ns\n", ((double)(endt-start))/count);

    /* Time for random */
    start = ism_common_currentTimeNanos();
    for (i=0; i<count; i++) {
        ism_common_newUUID(ubuf, 16, 4, 0);
    }
    endt = ism_common_currentTimeNanos();
    printf("Create time for random UUID = %g ns\n", ((double)(endt-start))/count);

    /* cleanup */
    ism_common_closeTimestamp(ts);
}

void testTimeZone(void) {
    int  offset;
    ism_timezone_t * tz1 = ism_common_getTimeZone("America/New_York");
    ism_timezone_t * tz2 = ism_common_getTimeZone("Europe/Rome");
    ism_timezone_t * tz3 = ism_common_getTimeZone("UTC");
    ism_timezone_t * tz4 = ism_common_getTimeZone("This_does_not_exist");
    ism_time_t timens = ism_common_currentTimeNanos();

    CU_ASSERT(tz1 != NULL);
    CU_ASSERT(tz2 != NULL);
    CU_ASSERT(tz3 != NULL);
    CU_ASSERT(tz4 != NULL);


    #define ADD_300DAYS  300*24*60*60*1000LL
    ism_time_t until;
    offset = ism_common_checkTimeZone(tz1, timens, &until);
    CU_ASSERT(offset == -300 || offset == -240);
    CU_ASSERT(until < timens + (ADD_300DAYS * 1000000L));
    offset = ism_common_checkTimeZone(tz2, timens, &until);
    CU_ASSERT(offset == 60 || offset == 120);
    CU_ASSERT(until < timens + (ADD_300DAYS * 1000000L));
    offset = ism_common_checkTimeZone(tz3, timens, &until);
    CU_ASSERT(offset == 0);
    CU_ASSERT(until > timens + (ADD_300DAYS * 1000000L));
    offset = ism_common_checkTimeZone(tz4, timens, &until);
    CU_ASSERT(offset == 0);
    CU_ASSERT(until > timens + (ADD_300DAYS * 1000000L));

}


/*
 * Array of time utils tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Util_CUnit_TimeUtils[] =  {
    { "getCurrentTimeTest", CUnit_ISM_getCurrentTime_test },
    { "convertTimeToJTime", CUnit_ISM_time2Jtime_test },
    { "convertJTimeToTime", CUnit_ISM_Jtime2time_test },
    { "openTimestamp", CUnit_ISM_openTimestamp_test },
    { "getTimestamp/setTimestamp", CUnit_ISM_get_set_Timestamp_test },
    { "getTimestampValues/setTimestampValues", CUnit_ISM_get_setTimestampValues },
    { "timestampByLocale", timestampByLocale_test },
    { "interface map", CUnit_ISM_ifmap },
    { "uuid test", CUnit_uuid },
    { "test time zone", testTimeZone },
    CU_TEST_INFO_NULL
};

