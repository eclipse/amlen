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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "engineInternal.h"
#include "engineRestoreTable.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_assert.h"
#include "test_utils_file.h"

typedef struct tag_tableData
{
   uint64_t recNum;
   char buffer[32];
} tableData;

#ifndef OK
#define OK 0
#endif
#define NANOS_PER_SECOND 1000000000

struct timespec timespec_diff( struct timespec start
                             , struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

void loadTable(ieutThreadData_t *pThreadData,
               iertTable_t **testTable,
               uint64_t numRecords,
               uint64_t numSamplesWanted,
               uint64_t sampleKeys[numSamplesWanted])
{
    int32_t rc = OK;
    uint64_t recNum = 0;
    struct timespec startTime, finishTime;

    uint64_t nextSampleRecNum = (numSamplesWanted != 0) ? 1: 0; //When recNum == this value we record the Key as a sample key
    uint64_t numSamplesTaken = 0;
    uint64_t sampleRecNum_increment = (numSamplesWanted != 0) ? numRecords/numSamplesWanted: 0;

    rc = clock_gettime(CLOCK_MONOTONIC, &startTime);
    TEST_ASSERT_EQUAL(rc, 0);

    while (recNum < numRecords)
    {
        recNum++;

        tableData *entry = (tableData *)malloc(sizeof(tableData));
        TEST_ASSERT_PTR_NOT_NULL(entry);

        entry->recNum = recNum;

        rc = iert_addTableEntry(pThreadData, testTable, (uint64_t)entry, entry);
        TEST_ASSERT_EQUAL(rc, OK);

        if(recNum == nextSampleRecNum)
        {
            sampleKeys[numSamplesTaken] = (uint64_t)entry;
            numSamplesTaken++;
            nextSampleRecNum += sampleRecNum_increment;
        }
    }

    rc = clock_gettime(CLOCK_MONOTONIC, &finishTime);
    TEST_ASSERT_EQUAL(rc, 0);

    TEST_ASSERT_EQUAL(numSamplesTaken, numSamplesWanted);

    struct timespec diff = timespec_diff(startTime, finishTime);

    test_log(testLOGLEVEL_CHECK, "Successfully added %lu records in %f seconds (%lu nanos)\n", recNum,
            (diff.tv_sec+((diff.tv_nsec*1.0)/NANOS_PER_SECOND)),
            (diff.tv_nsec+((uint64_t)NANOS_PER_SECOND)*(diff.tv_sec)) );
}

void searchTable(iertTable_t *testTable,
                 uint64_t numSampleKeys,
                 uint64_t sampleKeys[numSampleKeys])
{
    int32_t rc = OK;

    struct timespec startTime, finishTime;

    rc = clock_gettime(CLOCK_MONOTONIC, &startTime);
    TEST_ASSERT_EQUAL(rc, 0);

    for (uint64_t i = 0; i < numSampleKeys; i++)
    {
        void *outvalue = iert_getTableEntry(testTable, sampleKeys[i]);
#define CHECK_VALUE
#ifdef CHECK_VALUE
        TEST_ASSERT_EQUAL(sampleKeys[i],(uint64_t)outvalue);
#endif
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = clock_gettime(CLOCK_MONOTONIC, &finishTime);
    TEST_ASSERT_EQUAL(rc, 0);

    struct timespec diff = timespec_diff(startTime, finishTime);

    test_log(testLOGLEVEL_CHECK, "Found %lu records in %f seconds\n", numSampleKeys, (diff.tv_sec+((diff.tv_nsec*1.0)/NANOS_PER_SECOND)));
}


static uint64_t found_recNum;
static uint64_t needle=1204; //Actual number doesn't matter
static uint64_t curr_recNum;

int32_t compareWithNeedle(
        ieutThreadData_t *pThreadData,
        uint64_t key,
        void     *value,
        void *context)
{
    curr_recNum++;

    TEST_ASSERT_PTR_NOT_NULL(pThreadData);
    if (((tableData *)value)->recNum == needle)
    {
        TEST_ASSERT_EQUAL(found_recNum, 0); //recNum should be unique
        found_recNum = curr_recNum;
    }
    return OK;
}

void iterateTable(iertTable_t *testTable)
{
    found_recNum = 0;
    curr_recNum = 0;
    struct timespec startTime, finishTime;
    int32_t rc = OK;

    rc = clock_gettime(CLOCK_MONOTONIC, &startTime);
    TEST_ASSERT_EQUAL(rc, 0);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    //Check there is a single record with recNum == needle
    rc = iert_iterateOverTable(pThreadData,
                               testTable,
                               compareWithNeedle,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = clock_gettime(CLOCK_MONOTONIC, &finishTime);
    TEST_ASSERT_EQUAL(rc, 0);

    struct timespec diff = timespec_diff(startTime, finishTime);

    TEST_ASSERT_NOT_EQUAL(found_recNum, 0); //Should have found the record

    test_log(testLOGLEVEL_CHECK, "Successfully found a unique record with the required recNum in %f seconds (%lu nanos)\n",
            (diff.tv_sec+((diff.tv_nsec*1.0)/NANOS_PER_SECOND)),
            (diff.tv_nsec+((uint64_t)NANOS_PER_SECOND)*(diff.tv_sec)) );
}

void debugTable(iertTable_t *testTable)
{
    // Print chain length distributions, we don't actually want this to
    // come to the screen so we redirect stdout to /dev/null for this portion
    // of the test
    int origStdout = test_redirectStdout("/dev/null");
    TEST_ASSERT_NOT_EQUAL(origStdout, -1);

    iert_getChainLengthDistribution(testTable, NULL);

    int32_t rc = test_reinstateStdout(origStdout);
    TEST_ASSERT_NOT_EQUAL(rc, -1);
}
void removeTableEntries(ieutThreadData_t *pThreadData,
                        iertTable_t *testTable,
                        uint64_t numSampleKeys,
                        uint64_t sampleKeys[numSampleKeys])
{
    int32_t rc = OK;

    struct timespec startTime, finishTime;

    rc = clock_gettime(CLOCK_MONOTONIC, &startTime);
    TEST_ASSERT_EQUAL(rc, 0);

    //Remove some of the samples from the end of the lists
    uint64_t halfWayPoint = numSampleKeys/2;

    for (uint64_t i = numSampleKeys-1; i >= halfWayPoint; i--)
    {
        rc = iert_removeTableEntry(pThreadData, testTable, sampleKeys[i]);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Remove the rest from the front of the lists
    for (uint64_t i = 0; i < halfWayPoint; i++)
    {
        rc = iert_removeTableEntry(pThreadData, testTable, sampleKeys[i]);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = clock_gettime(CLOCK_MONOTONIC, &finishTime);
    TEST_ASSERT_EQUAL(rc, 0);

    struct timespec diff = timespec_diff(startTime, finishTime);

    test_log(testLOGLEVEL_CHECK, "Removed %lu records in %f seconds\n", numSampleKeys, (diff.tv_sec+((diff.tv_nsec*1.0)/NANOS_PER_SECOND)));
}



//Check we can flow a large number of messages through the queue from multiple putters
static void testRestoreTableBasic(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting testRestoreTableBasic....");

    iertTable_t *testTable;
    int32_t rc = OK;
    uint64_t numRecords = 1000000; //We'll add and find this make entries
    iertTableCapacities_t initialSize = iertHundredsOfItems; //small so adding causes resizing
    uint64_t *sampleKeys;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    //create an array to store the keys we're going to look up in
    sampleKeys = malloc(numRecords * sizeof(uint64_t));

    rc = iert_createTable( pThreadData
                         , &testTable
                         , initialSize
                         , false
                         , false
                         , 0
                         , 0);

    TEST_ASSERT_EQUAL(rc, OK);

    loadTable(pThreadData, &testTable, numRecords, numRecords, sampleKeys);
    searchTable(testTable, numRecords, sampleKeys);
    iterateTable(testTable);
    debugTable(testTable);
    removeTableEntries(pThreadData, testTable, numRecords, sampleKeys);

    free(sampleKeys);

    test_log(testLOGLEVEL_TESTNAME, "Completed testRestoreTableBasic");
}

CU_TestInfo ISM_Engine_CUnit_RestoreTable_Basic[] = {
    { "testRestoreTableBasic", testRestoreTableBasic },
    CU_TEST_INFO_NULL
};


CU_SuiteInfo ISM_Engine_CUnit_RestoreTable_Suites[] = {
    IMA_TEST_SUITE("RestoreTableBasic", NULL, NULL, ISM_Engine_CUnit_RestoreTable_Basic),
    CU_SUITE_INFO_NULL,
};

/*********************************************************************/
/* Main                                                              */
/*********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;

    // Level of output
    if (argc >= 2)
    {
        int32_t level=atoi(argv[1]);
        if ((level >=0) && (level <= 9))
        {
            test_setLogLevel(level);
        }
    }

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    retval = test_engineInit_DEFAULT;
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();
    CU_register_suites(ISM_Engine_CUnit_RestoreTable_Suites);

    CU_basic_run_tests();

    CU_RunSummary * CU_pRunSummary_Final;
    CU_pRunSummary_Final = CU_get_run_summary();
    printf("\n\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
            CU_pRunSummary_Final->nTestsRun,
            CU_pRunSummary_Final->nTestsFailed,
            CU_pRunSummary_Final->nAssertsFailed);
    if ((CU_pRunSummary_Final->nTestsFailed > 0) ||
        (CU_pRunSummary_Final->nAssertsFailed > 0))
    {
        retval = 1;
    }

    CU_cleanup_registry();

    int32_t rc = test_engineTerm(true);
    if (retval == 0) retval = rc;

    rc = test_processTerm(retval == 0);
    if (retval == 0) retval = rc;

mod_exit:

    return retval;
}
