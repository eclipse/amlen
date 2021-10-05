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
#include <sys/prctl.h>
#include <unistd.h>

#include "engine.h"
#include "engineInternal.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "engineAsync.h"

static int32_t test_asyncInternalCallback(ieutThreadData_t *pThreadData,
                                          int32_t rc,
                                          ismEngine_AsyncData_t *asyncInfo,
                                          ismEngine_AsyncDataEntry_t *context)
{
    TEST_ASSERT_PTR_NOT_NULL(context->Handle);

    // If this callback has data, check that it contains expected data
    if (context->DataLen > 10)
    {
        char *alphabet = (char *)context->Data;

        TEST_ASSERT_EQUAL(alphabet[0], 'A');
        TEST_ASSERT_EQUAL(alphabet[7], 'H');
    }
    else
    {
        TEST_ASSERT_PTR_NULL(context->Data);
    }

    iead_popAsyncCallback(asyncInfo, context->DataLen);

    uint32_t *callArray = (uint32_t *)context->Handle;
    callArray[context->Type] += 1;

    return OK;
}

static void test_asyncExternalCallback(int32_t rc,
                                       void *handle,
                                       void *context)
{
    uint32_t *callArray = *(uint32_t **)context;
    TEST_ASSERT_EQUAL(handle, callArray); // should have been set
    callArray[EngineCaller] += 1; // Always should be EngineCaller
}

// Test the operation of pushing and popping async callbacks in different scenarios
void test_PushAndPopCallbacks(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    uint32_t callArray[IEAD_MAXARRAYENTRIES] = {0};
    uint32_t **pCallArray = (uint32_t **)&callArray;

    TEST_ASSERT_EQUAL(EngineCaller, 0);

    uint8_t dataArea[1024];

    // Fill the area with recognizable characters
    for(int32_t i=0; i<sizeof(dataArea); i++) dataArea[i] = 'A'+(i%26);

    ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
       {ismENGINE_ASYNCDATAENTRY_STRUCID, 1, &dataArea, 50, &callArray, {.internalFn = test_asyncInternalCallback}},
       {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller, &pCallArray, sizeof(pCallArray), NULL, {.externalFn = test_asyncExternalCallback}},
       {ismENGINE_ASYNCDATAENTRY_STRUCID, 2, NULL, 0, &callArray, {.internalFn = test_asyncInternalCallback}}
    };

    ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID,
                                       NULL, IEAD_MAXARRAYENTRIES, 3, 0, true,  0, 0, asyncArray};

    // Try pushing an entry while it's on the stack
    ismEngine_AsyncDataEntry_t newEntry = {ismENGINE_ASYNCDATAENTRY_STRUCID, 3, &dataArea, 50, &callArray, {.internalFn = test_asyncInternalCallback}};

    // Set the caller handle...
    iead_setEngineCallerHandle(&asyncData, &callArray);

    iead_pushAsyncCallback(pThreadData, &asyncData, &newEntry);
    TEST_ASSERT_EQUAL(asyncData.numEntriesUsed, 4);

    ismEngine_AsyncData_t *heapAsyncData = iead_ensureAsyncDataOnHeap(pThreadData, &asyncData);
    TEST_ASSERT_PTR_NOT_NULL(heapAsyncData);
    TEST_ASSERT_EQUAL(heapAsyncData->numEntriesUsed, 4);

    // Check we get the same address back on 2nd ensure
    TEST_ASSERT_EQUAL(heapAsyncData, iead_ensureAsyncDataOnHeap(pThreadData, heapAsyncData));
    TEST_ASSERT_EQUAL(heapAsyncData->numEntriesAllocated, IEAD_MAXARRAYENTRIES);
    TEST_ASSERT_EQUAL(heapAsyncData->numEntriesUsed, asyncData.numEntriesUsed);

    // Pop the newly pushed entry back off the stack
    iead_popAsyncCallback(heapAsyncData, newEntry.DataLen);
    TEST_ASSERT_EQUAL(heapAsyncData->numEntriesUsed, 3);

    newEntry.Type = 3;
    newEntry.Data = &dataArea;

    for(int32_t i=0; i<2; i++)
    {
        newEntry.Data = &dataArea;
        newEntry.DataLen = 100;

        // Now push a new entry while it's on the heap, but with a longer length - should cause malloc
        iead_pushAsyncCallback(pThreadData, heapAsyncData, &newEntry);
        TEST_ASSERT_EQUAL(heapAsyncData->numEntriesUsed, 4);

        // Pop the new entry back off again, and add it back with longer length
        iead_popAsyncCallback(heapAsyncData, newEntry.DataLen);
        TEST_ASSERT_EQUAL(heapAsyncData->numEntriesUsed, 3);
    }

    // Add an even longer one to cause a re-alloc
    newEntry.Data = &dataArea;
    newEntry.DataLen = 200;
    iead_pushAsyncCallback(pThreadData, heapAsyncData, &newEntry);
    TEST_ASSERT_EQUAL(heapAsyncData->numEntriesUsed, 4);

    // Now push an additional entry (one more than was ever used before it was on the heap)
    newEntry.Type = 4;
    newEntry.Data = &dataArea;
    newEntry.DataLen = 100;

    iead_pushAsyncCallback(pThreadData, heapAsyncData, &newEntry);
    TEST_ASSERT_EQUAL(heapAsyncData->numEntriesUsed, 5);

    iead_completeAsyncData(OK, heapAsyncData);
    TEST_ASSERT_EQUAL(callArray[0], 1);
    TEST_ASSERT_EQUAL(callArray[1], 1);
    TEST_ASSERT_EQUAL(callArray[2], 1);
    TEST_ASSERT_EQUAL(callArray[3], 1);
    TEST_ASSERT_EQUAL(callArray[4], 1);

    // Pop everything off and push one larger data area
    heapAsyncData = iead_ensureAsyncDataOnHeap(pThreadData, &asyncData);
    TEST_ASSERT_PTR_NOT_NULL(heapAsyncData);

    iead_popAsyncCallback(heapAsyncData, asyncData.entries[3].DataLen);
    newEntry.DataLen += asyncData.entries[3].DataLen;
    iead_popAsyncCallback(heapAsyncData, asyncData.entries[2].DataLen);
    newEntry.DataLen += asyncData.entries[2].DataLen;
    iead_popAsyncCallback(heapAsyncData, asyncData.entries[1].DataLen);
    newEntry.DataLen += asyncData.entries[1].DataLen;
    iead_popAsyncCallback(heapAsyncData, asyncData.entries[0].DataLen);
    newEntry.DataLen += asyncData.entries[0].DataLen;
    TEST_ASSERT_EQUAL(heapAsyncData->numEntriesUsed, 0);

    newEntry.Data = &dataArea;
    iead_pushAsyncCallback(pThreadData, heapAsyncData, &newEntry);
    TEST_ASSERT_EQUAL(heapAsyncData->numEntriesUsed, 1);
    iead_completeAsyncData(OK, heapAsyncData);
    TEST_ASSERT_EQUAL(callArray[newEntry.Type], 2);

    iead_completeAsyncData(OK, &asyncData);
    TEST_ASSERT_EQUAL(callArray[0], 2);
    TEST_ASSERT_EQUAL(callArray[1], 2);
    TEST_ASSERT_EQUAL(callArray[2], 2);
}

int initSuite(void)
{
    int32_t rc = test_engineInit(true, true,
                                 true, // Disable Auto creation of named queues
                                 false, /*recovery should complete ok*/
                                 ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                                 testDEFAULT_STORE_SIZE);
    return rc;
}

int termSuite(void)
{
    return test_engineTerm(true);
}

CU_TestInfo ISM_MemPool_CUnit_test_AsyncData[] =
{
    { "test_PushAndPopCallbacks", test_PushAndPopCallbacks },
    CU_TEST_INFO_NULL
};

CU_SuiteInfo ISM_MemPool_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("AsyncData", initSuite, termSuite, ISM_MemPool_CUnit_test_AsyncData),
    CU_SUITE_INFO_NULL
};

int setup_CU_registry(int argc, char **argv, CU_SuiteInfo *allSuites)
{
    CU_SuiteInfo *tempSuites = NULL;

    int retval = 0;

    if (argc > 1)
    {
        int suites = 0;

        for(int i=1; i<argc; i++)
        {
            if (!strcasecmp(argv[i], "FULL"))
            {
                if (i != 1)
                {
                    retval = 97;
                    break;
                }
                // Driven from 'make fulltest' ignore this.
            }
            else if (!strcasecmp(argv[i], "verbose"))
            {
                CU_basic_set_mode(CU_BRM_VERBOSE);
            }
            else if (!strcasecmp(argv[i], "silent"))
            {
                CU_basic_set_mode(CU_BRM_SILENT);
            }
            else
            {
                bool suitefound = false;
                int index = atoi(argv[i]);
                int totalSuites = 0;

                CU_SuiteInfo *curSuite = allSuites;

                while(curSuite->pTests)
                {
                    if (!strcasecmp(curSuite->pName, argv[i]))
                    {
                        suitefound = true;
                        break;
                    }

                    totalSuites++;
                    curSuite++;
                }

                if (!suitefound)
                {
                    if (index > 0 && index <= totalSuites)
                    {
                        curSuite = &allSuites[index-1];
                        suitefound = true;
                    }
                }

                if (suitefound)
                {
                    tempSuites = realloc(tempSuites, sizeof(CU_SuiteInfo) * (suites+2));
                    memcpy(&tempSuites[suites++], curSuite, sizeof(CU_SuiteInfo));
                    memset(&tempSuites[suites], 0, sizeof(CU_SuiteInfo));
                }
                else
                {
                    printf("Invalid test suite '%s' specified, the following are valid:\n\n", argv[i]);

                    index=1;

                    curSuite = allSuites;

                    while(curSuite->pTests)
                    {
                        printf(" %2d : %s\n", index++, curSuite->pName);
                        curSuite++;
                    }

                    printf("\n");

                    retval = 99;
                    break;
                }
            }
        }
    }

    if (retval == 0)
    {
        if (tempSuites)
        {
            CU_register_suites(tempSuites);
        }
        else
        {
            CU_register_suites(allSuites);
        }
    }

    if (tempSuites) free(tempSuites);

    return retval;
}

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;
    int testLogLevel = testLOGLEVEL_TESTPROGRESS;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_MemPool_CUnit_allsuites);

    if (retval == 0)
    {
        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();

        printf("\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
               CU_pRunSummary_Final->nTestsRun,
               CU_pRunSummary_Final->nTestsFailed,
               CU_pRunSummary_Final->nAssertsFailed);
        if ((CU_pRunSummary_Final->nTestsFailed > 0) ||
            (CU_pRunSummary_Final->nAssertsFailed > 0))
        {
            retval = 1;
        }
    }

    CU_cleanup_registry();

    test_processTerm(retval == 0);

mod_exit:

    return retval;
}
