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
 * File: testDelayCUNIT.C
 * Component: server
 * SubComponent: server_proxy
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#include <ismutil.h>
#include <testDelayCUnit.h>
#undef TRACE_COMP
#include "throttle.c"


#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>


extern ismDelay * throttleDelay[64];
extern int throttleLimitCount;
int testcount=30;
int isTestInited=0;
extern int g_verbose;

/*   (programming note 1)
 * (programming note 11)
 * Array of timer tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Throttle_CUnit_Delay[] =
  {
        { "Delay Parse Configuration Test", CUnit_test_ism_throttle_parseThrottleConfiguration },
        { "Get Delay Time Test", CUnit_test_ism_throttle_getDelayTime },
        { "Get Delay Time In Nanos", CUnit_test_ism_throttle_getDelayTimeNanos },
        { "Add Auth Failed Count Test", CUnit_test_ism_throttle_addAuthFailedClientEntryCount },
        { "Set & Get ConnectReqinQ Test", CUnit_test_ism_throttle_setConnectReqInQ },
        { "incrementClientIDStealCount Test", CUnit_test_ism_throttle_incrementClienIDStealCount },
        { "incrementConnCloseError Test", CUnit_test_ism_throttle_incrementConnCloseError },

        CU_TEST_INFO_NULL
   };

int initDelayTests(void) {
   	int rc = 0;
   	int i = 0;

   	if(isTestInited) return 0;

   	isTestInited=1;

   	ism_common_initUtil();

   	ism_field_t var = {0};
   	var.type = VT_Boolean;
	var.val.i = 1;
	ism_common_setProperty(ism_common_getConfigProperties(),THROTTLE_ENABLED_STR, &var);


	for (i=0 ; i< testcount; i++){

		char * limitPropName = alloca(128);
		char * delayPropName = alloca(128);
		sprintf(limitPropName, "%s%d",   THROTTLE_LIMIT_STR,i);
		sprintf(delayPropName, "%s%d",   THROTTLE_DELAY_STR,i);

		var.type = VT_Integer;
		var.val.i = 5 * (i+1);
		ism_common_setProperty(ism_common_getConfigProperties(),limitPropName, &var);

		// xUNUSED int throttleLimit= ism_common_getIntConfig(limitPropName, 0);
		//printf("%s=%d\n", limitPropName, throttleLimit);

		var.type = VT_Integer;
		var.val.i = 5000000 * (i+1);
		ism_common_setProperty(ism_common_getConfigProperties(),delayPropName, &var);

		// xUNUSED int delay = ism_common_getIntConfig(delayPropName, 0);



	}

    return rc;
}

/* The timer tests cleanup function.
 * Removes any files created and terminates the timer thread.
 * Returns zero on success, non-zero otherwise.
 */
int cleanDelayTests(void) {
    int rc = 0;
    return rc;
}

void CUnit_test_ism_throttle_parseThrottleConfiguration(void)
{
	int i=0;

	ism_throttle_initThrottle();

	int count = ism_throttle_parseThrottleConfiguration();
	CU_ASSERT(testcount == count);

	/*Already parsed. SHould receive zero*/
	count = ism_throttle_parseThrottleConfiguration();
	CU_ASSERT(0 == count);

	int enabled = ism_common_getBooleanConfig(THROTTLE_ENABLED_STR, 0);
	CU_ASSERT(enabled == 1);

	for(i=0; i<throttleLimitCount; i++){
		struct ismDelay_t * delay =  throttleDelay[i];

		//printf("Delay: Index=%d. Limit=%d. Delay:%llu\n", i, delay->limit, delay->delay_time);
		CU_ASSERT(delay->limit == (5 * (i+1)));
		CU_ASSERT(delay->delay_time == (5000000 * (i+1)));
	}

	CU_ASSERT(throttleLimitCount == testcount);

	ism_throttle_termThrottle();

}


void CUnit_test_ism_throttle_getDelayTime(void)
{
	int i = 0;
    int line = __LINE__;
	ism_throttle_initThrottle();

	int count = ism_throttle_parseThrottleConfiguration();
	if (g_verbose)
	    printf ("Line=%d Count: %d. TestCount: %d\n", line, count, testcount);
	CU_ASSERT(testcount == count);

	int enabled = ism_common_getBooleanConfig(THROTTLE_ENABLED_STR, 0);
	if (g_verbose>1)
	    printf("Line=%d Throttle.Enable=%d\n", line, throttleEnabled);
	CU_ASSERT(enabled == 1);

	for(i=0; i<throttleLimitCount; i++){
		struct ismDelay_t * delay =  throttleDelay[i];

		int delay_time = ism_throttle_getDelayTime(delay->limit+1);
        if (g_verbose > 1)
		  printf("Line=%d Delay: Index=%d. Limit=%d. Delay:%d. iLimit=%d. Returned Delay Time=%d\n",
		          line, i, delay->limit, delay->delay_time, delay->limit+1, delay_time);

		CU_ASSERT(delay->delay_time == delay_time);
	}

	CU_ASSERT(throttleLimitCount == testcount);
	if (g_verbose)
	printf("Line=%d Throttle Limit Count: %d\n", line, throttleLimitCount);

	ism_throttle_termThrottle();

}

void CUnit_test_ism_throttle_getDelayTimeNanos(void)
{
	int i = 0;
    int line = __LINE__;

	ism_throttle_initThrottle();

	int count = ism_throttle_parseThrottleConfiguration();
	if (g_verbose)
	    printf ("Line=%d Count: %d. TestCount: %d\n", line, count, testcount);
	CU_ASSERT(testcount == count);

	int enabled = ism_common_getBooleanConfig(THROTTLE_ENABLED_STR, 0);
	if (g_verbose)
	    printf("Line=%d Throttle.Enable=%d\n", line, throttleEnabled);
	CU_ASSERT(enabled == 1);

	for(i=0; i<throttleLimitCount; i++){
		struct ismDelay_t * delay =  throttleDelay[i];

		ism_time_t delay_time = ism_throttle_getDelayTimeInNanos(delay->limit+1);
        if (g_verbose > 1)
		    printf("Line=%d Delay: Index=%d. Limit=%d. Delay:%d. iLimit=%d. Returned Delay Time=%ld\n",
		            line, i, delay->limit, delay->delay_time, delay->limit+1, delay_time);

		CU_ASSERT(delay->delay_in_nanos == delay_time);
	}

	CU_ASSERT(throttleLimitCount == testcount);
	if (g_verbose)
	    printf("Line=%d Throttle Limit Count: %d\n", line, throttleLimitCount );

	ism_throttle_termThrottle();

}

void CUnit_test_ism_throttle_addAuthFailedClientEntryCount(void)
{
	const char * clientID = "ClientID1";
	int i=0;
	int prev_failed_count = 0;
	ism_throttle_initThrottle();

	for(i=0; i< testcount; i++){
		prev_failed_count = ism_throttle_incrementAuthFailedCount(clientID);
		CU_ASSERT(prev_failed_count == i);
	}

	prev_failed_count = ism_throttle_removeThrottleObj(clientID);
	CU_ASSERT(prev_failed_count == testcount);

	prev_failed_count = ism_throttle_removeThrottleObj(clientID);

	CU_ASSERT(prev_failed_count == 0);
}

void CUnit_test_ism_throttle_incrementClienIDStealCount(void)
{
	int maxcount = 10;
	int rcount;
	int line = __LINE__;

	ism_throttle_initThrottle();

	int count = ism_throttle_parseThrottleConfiguration();
	if (g_verbose)
	    printf ("Line=%d Count: %d. TestCount: %d\n", line, count, testcount);
	CU_ASSERT(testcount == count);

	const char * clientID = "a:quickstart:app1";

	for(count=0; count< maxcount; count++){
		rcount = ism_throttle_incrementClienIDStealCount(clientID);
		CU_ASSERT(rcount == count);
	}

	count = ism_throttle_getThrottleLimit(clientID, THROTTLET_CLIENTID_STEAL);
	CU_ASSERT(maxcount == count);

	/*Shouldn't remove because RC is not OK*/
	ism_throttle_removeThrottleObj(clientID);
	count = ism_throttle_getThrottleLimit(clientID, THROTTLET_CLIENTID_STEAL);
	CU_ASSERT(count == 10);

	/*Should remove because RC is OK*/
	ism_throttle_setLastCloseRC(clientID, ISMRC_OK);
	ism_throttle_removeThrottleObj(clientID);
	count = ism_throttle_getThrottleLimit(clientID, THROTTLET_CLIENTID_STEAL);
	CU_ASSERT(count == 0);


	ism_throttle_termThrottle();

}

void CUnit_test_ism_throttle_setConnectReqInQ(void)
{
	int line = __LINE__;
	ism_throttle_initThrottle();

	int count = ism_throttle_parseThrottleConfiguration();
	if (g_verbose)
	    printf("Line=%d, Count: %d. TestCount: %d\n", line, count, testcount);
	CU_ASSERT(testcount == count);

	const char * clientID = "a:quickstart:app1";

	ism_throttle_incrementClienIDStealCount(clientID);

	int oldvalue = ism_throttle_setConnectReqInQ(clientID , 1);
	int currvalue =  ism_throttle_getConnectReqInQ(clientID);

	CU_ASSERT(oldvalue == 0);
	CU_ASSERT(currvalue == 1);

	oldvalue = ism_throttle_setConnectReqInQ(clientID , 0);
	currvalue =  ism_throttle_getConnectReqInQ(clientID);

	CU_ASSERT(oldvalue == 1);
	CU_ASSERT(currvalue == 0);

	ism_throttle_termThrottle();

}


void CUnit_test_ism_throttle_incrementConnCloseError(void)
{
	int maxcount = 10;
	int rcount;
	int line = __LINE__;

	ism_throttle_initThrottle();

	int count = ism_throttle_parseThrottleConfiguration();
	if (g_verbose)
	    printf ("Line=%d Count: %d. TestCount: %d\n", line, count, testcount);
	CU_ASSERT(testcount == count);

	const char * clientID = "a:quickstart:app1";

	for(count=0; count< maxcount; count++){
		rcount = ism_throttle_incrementConnCloseError(clientID, 175);
		CU_ASSERT(rcount == count);
	}

	count = ism_throttle_getThrottleLimit(clientID, THROTTLET_CONNCLOSEERR);
	CU_ASSERT(maxcount == count);

	/*Shouldn't remove because RC is not OK*/
	ism_throttle_removeThrottleObj(clientID);
	count = ism_throttle_getThrottleLimit(clientID, THROTTLET_CONNCLOSEERR);
	CU_ASSERT(count == 10);

	/*Should remove because RC is OK*/
	ism_throttle_setLastCloseRC(clientID, ISMRC_OK);
	ism_throttle_removeThrottleObj(clientID);
	count = ism_throttle_getThrottleLimit(clientID, THROTTLET_CONNCLOSEERR);
	CU_ASSERT(count == 0);


	ism_throttle_termThrottle();

}
