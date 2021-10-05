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
 * File: testTimerCUnit.c
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

#include <ismutil.h>

#include <testTimerCUnit.h>

ism_priority_class_e priClass = ISM_TIMER_HIGH;    /* Timer priority class */

const uint64_t DELAY = 10000000;            /* Default delay between scheduled events */

#define THOUSAND       1000
#define MILLION        1000000
#define BILLION        1000000000

struct StatusMap {
	volatile int ind[4];
};

static struct StatusMap *statusMap = NULL;

/*   (programming note 1)
 * (programming note 11)
 * Array of timer tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Util_CUnit_timer[] =
  {
        { "setTimerOnceTest", CUnit_test_ISM_timer_once },
        { "cancelTimerTest", CUnit_test_ISM_timer_once_cancel },
        { "setTimerRepeat1EventTest", CUnit_test_ISM_timer_repeat_single },
        { "setTimerRepeatMultiEventTest", CUnit_test_ISM_timer_repeat_multi },
        CU_TEST_INFO_NULL };

/* The timer tests initialization function.
 * Creates a timer thread.
 * Returns zero on success, non-zero otherwise.
 */
int initTimerTests(void) {
    ism_common_initTimers();

    statusMap = calloc(1, sizeof(struct StatusMap));
    sleep(1);
    return 0;
}

/* The timer test suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int initTimerUtilSuite(void) {
    initTimerTests();
    return 0;
}

/* The timer tests cleanup function.
 * Frees allocated memory and terminates the timer thread.
 * Returns zero on success, non-zero otherwise.
 */
int cleanTimerTests(void) {
	free(statusMap);

    ism_common_stopTimers();

    return 0;
}

/* The timer test suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int cleanTimerUtilSuite(void) {
    cleanTimerTests();
    return 0;
}

/*
 * Set an indicator.
 * The function's signature matches ism_attime_t.
 * @param   key       The timer event identifier.
 * @param   timestamp Time when the function was executed.
 * @param   userdata  The pointer to the string with indicator id.
 */
static int setIndicatorSingle(ism_timer_t key, ism_time_t timestamp, void * userdata) {
	int id = atoi((char *)userdata);
	statusMap->ind[id] = 1;
    ism_common_cancelTimer(key);
	return 0;
}

static int setIndicatorSingleNoCancel(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    int id = atoi((char *)userdata);
    statusMap->ind[id] = 1;
    return 0;
}

static int setIndicatorMulti(ism_timer_t key, ism_time_t timestamp, void * userdata) {
	int id = atoi((char *)userdata);
	statusMap->ind[id] = 1;
	return 1;
}

/**
 * Check if the indicator is set and reset it.
 * @param  id  The id of the indicator to check
 * @return A return code: 0=indicator is not set, 1 otherwise
 */
static int checkIndicator(int id) {
    int result = !!(statusMap->ind[id]);
    statusMap->ind[id] = 0;

    return result;
}

/*
 * Test ism_common_setTimerOnce function.
 * Create 4 one-off tasks to set indicators with DELAY
 * nanoseconds between each other.
 *
 * Wait corresponding amount of time + 1 mil and then
 * check if indicators were set.
 */
void CUnit_test_ISM_timer_once(void) {
    printf("\nCUnit_test_ISM_timer_once\n");

    ism_time_t when1 = DELAY * 1;
    ism_time_t when2 = DELAY * 4;
    ism_time_t when3 = DELAY * 8;
    ism_time_t when4 = DELAY * 16;

    ism_attime_t fun = setIndicatorSingle;

    ism_common_setTimerOnce(priClass, fun, "0", when4);
    ism_common_setTimerOnce(priClass, fun, "1", when1);
    ism_common_setTimerOnce(priClass, fun, "2", when3);
    ism_common_setTimerOnce(priClass, fun, "3", when2);

    usleep(DELAY / THOUSAND + THOUSAND);
    CU_ASSERT(checkIndicator(1) == 1);
    usleep(DELAY * (4 - 1) / THOUSAND + THOUSAND);
    CU_ASSERT(checkIndicator(3) == 1);
    usleep(DELAY * (8 - 4) / THOUSAND + THOUSAND);
    CU_ASSERT(checkIndicator(2) == 1);
    usleep(DELAY * (16 - 8) / THOUSAND + THOUSAND);
    CU_ASSERT(checkIndicator(0) == 1);
}

/*
 * Test ism_common_cancelTimer function with one-off events.
 *
 * Create 4 one-off tasks to set indicators with DELAY nanoseconds
 * between each other in the order 4, 1, 3, 2. Then cancel tasks
 * 2 (key3) and 4 (key0) before the scheduled event and tasks
 * 1 (key1) and 3 (key2) after the scheduled event.
 *
 * Wait corresponding amount of time + 1 mil and then
 * check if indicators were set.
 */
void CUnit_test_ISM_timer_once_cancel(void) {
    printf("\nCUnit_test_ISM_timer_once_cancel\n");

    ism_time_t when1 = DELAY * 1;
    ism_time_t when2 = DELAY * 4;
    ism_time_t when3 = DELAY * 8;
    ism_time_t when4 = DELAY * 16;

    ism_attime_t fun = setIndicatorSingleNoCancel;

    ism_timer_t key1 = ism_common_setTimerOnce(priClass, fun, "1", when1);
    ism_timer_t key3 = ism_common_setTimerOnce(priClass, fun, "3", when2);
    ism_timer_t key2 = ism_common_setTimerOnce(priClass, fun, "2", when3);
    ism_timer_t key0 = ism_common_setTimerOnce(priClass, fun, "0", when4);

    usleep(DELAY / THOUSAND + THOUSAND);
    ism_common_cancelTimer(key1);
    ism_common_cancelTimer(key3);
    CU_ASSERT(checkIndicator(1) == 1);
    usleep(DELAY * (4 - 1) / THOUSAND + THOUSAND);
    ism_common_cancelTimer(key0);
    CU_ASSERT(checkIndicator(3) == 0);
    usleep(DELAY * (8 - 4) / THOUSAND + THOUSAND);
    ism_common_cancelTimer(key2);
    CU_ASSERT(checkIndicator(2) == 1);
    usleep(DELAY  * (16 - 8)/ THOUSAND + THOUSAND);
    CU_ASSERT(checkIndicator(0) == 0);
}

/*
 * Test single event scheduled with ism_common_setTimerRate function.
 *
 * Create a repeated tasks to set indicators, delayed by 2xDELAY.
 * Ensure that the indicator is set in time, then reset it.
 * Repeat the test 2 times based on the period value of 3xDELAY.
 * Cancel the timer and ensure that the indicator was not set later.
 */
void CUnit_test_ISM_timer_repeat_single(void) {
    printf("\nCUnit_test_ISM_timer_repeat_single\n");

    int delay = 1;
    int period1 = 2;
    int period2 = 1;
    int period = period1 + period2;
    enum ism_timer_e units = TS_SECONDS;

    /*
     * Schedule a task using seconds
     */
    ism_timer_t key = ism_common_setTimerRate(priClass, setIndicatorMulti, "0", delay, period, units);
    CU_ASSERT(checkIndicator(0) == 0);
    usleep(delay * MILLION + THOUSAND);
    CU_ASSERT(checkIndicator(0) == 1);
    usleep(period1 * MILLION);
    CU_ASSERT(checkIndicator(0) == 0);		// Ensure event does not get triggered before its time
    usleep(period2 * MILLION + THOUSAND);
    CU_ASSERT(checkIndicator(0) == 1);
    usleep(period * MILLION + THOUSAND);
    CU_ASSERT(ism_common_cancelTimer(key) == 0);
    CU_ASSERT(checkIndicator(0) == 1);
    usleep(period * MILLION + THOUSAND);
    CU_ASSERT(checkIndicator(0) == 0);
}

/*
 * Test multiple simultaneous repeating events scheduled
 * with ism_common_setTimerRate function.
 *
 * Create 3 repeated tasks to set different indicators.
 * Delays for tasks would be DELAY, 2xDELAY and DELAY. Period would be
 * 3xDELAY, 3xDELAY and 2xDELAY.
 * Ensure that indicators are set in time, then reset them.
 * Repeat the test 3 times based on their period values.
 *
 *  x - check if an indicator was set
 *  0xDELAY           x  x  x
 *  1xDELAY           1     3
 *  2xDELAY              2
 *  3xDELAY                 3
 *  4xDELAY           1
 *  5xDELAY              2  3
 *  6xDELAY
 *  7xDELAY           1     3 - cancel 3
 *  8xDELAY              2
 *  9xDELAY                 x
 * 10xDELAY           1       - cancel 1
 * 11xDELAY              2    - cancel 2
 * 12xDELAY
 * 13xDELAY           x
 * 14xDELAY              x
 */
void CUnit_test_ISM_timer_repeat_multi(void) {
    printf("\nCUnit_test_ISM_timer_repeat_multi\n");

    int delay1 = DELAY;
    int delay2 = 2 * DELAY;
    int delay3 = DELAY;
    int period1 = 3 * DELAY;
    int period2 = 3 * DELAY;
    int period3 = 2 * DELAY;

    uint64_t	sleepTime;

    /*
     * Schedule tasks using different time units
     */
    ism_timer_t key0 = ism_common_setTimerRate(priClass, setIndicatorMulti, "0",
            delay1 / THOUSAND, period1 / THOUSAND, TS_MICROSECONDS);
    ism_timer_t key1 = ism_common_setTimerRate(priClass, setIndicatorMulti, "1",
            delay2 / MILLION, period2 / MILLION, TS_MILLISECONDS);
    ism_timer_t key2 = ism_common_setTimerRate(priClass, setIndicatorMulti, "2",
            delay3, period3, TS_NANOSECONDS);

    CU_ASSERT(checkIndicator(0) == 0);
    CU_ASSERT(checkIndicator(1) == 0);
    CU_ASSERT(checkIndicator(2) == 0);

    /* Tasks 1 and 3 sleep for the same time, check indicator */
    sleepTime = DELAY / THOUSAND + THOUSAND;
    usleep(sleepTime);

    /* 1xDELAY */
    CU_ASSERT(checkIndicator(0) == 1);
    CU_ASSERT(checkIndicator(2) == 1);

    /* Now wait for task 2 */
    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 2xDELAY */
    CU_ASSERT(checkIndicator(1) == 1);

    /* By now, first run is complete for all tasks */

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 3xDELAY */
    CU_ASSERT(checkIndicator(2) == 1);

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 4xDELAY */
    CU_ASSERT(checkIndicator(0) == 1);

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 5xDELAY */
    CU_ASSERT(checkIndicator(1) == 1);
    CU_ASSERT(checkIndicator(2) == 1);

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 6xDELAY */

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 7xDELAY */
    CU_ASSERT(checkIndicator(0) == 1);
    CU_ASSERT(ism_common_cancelTimer(key2) == 0);
    CU_ASSERT(checkIndicator(2) == 1);

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 8xDELAY */
    CU_ASSERT(checkIndicator(1) == 1);

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 9xDELAY */
    CU_ASSERT(checkIndicator(2) == 0);

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 10xDELAY */
    CU_ASSERT(ism_common_cancelTimer(key0) == 0);
    CU_ASSERT(checkIndicator(0) == 1);

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 11xDELAY */
    CU_ASSERT(ism_common_cancelTimer(key1) == 0);
    CU_ASSERT(checkIndicator(1) == 1);

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 12xDELAY */

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 13xDELAY */
    CU_ASSERT(checkIndicator(0) == 0);

    usleep(DELAY / THOUSAND + THOUSAND / 2);

    /* 14xDELAY */
    CU_ASSERT(checkIndicator(1) == 0);
}
