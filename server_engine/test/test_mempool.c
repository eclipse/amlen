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
#include "test_utils_assert.h"
#include "test_utils_initterm.h"
#include "test_utils_log.h"

#include "mempool.h"
#include "mempoolInternal.h"

#define ROUNDED_MEMAMOUNT(_memAmount) ((_memAmount) + (8 - ((_memAmount) & 0x07)))

void test_MemPoolBasic1(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    test_log(testLOGLEVEL_TESTNAME, "Starting test_MemPoolBasic1");
    iempMemPoolHandle_t memPoolHdl;
    size_t initialPageSize = 4192;
    size_t expectReservedMemRemaining = 256;

    int32_t rc = iemp_createMemPool( pThreadData
                                   , IEMEM_PROBE(iemem_localTransactions, 1024)
                                   , 256
                                   , initialPageSize
                                   , 10240
                                   , &memPoolHdl);
    TEST_ASSERT_EQUAL(rc, OK);
    void *rsvdMem1 = NULL;
    size_t memAmount, requestedAmount;

    iempMemPoolOverallHeader_t *poolHdr = (iempMemPoolOverallHeader_t *)(memPoolHdl + 1);
    TEST_ASSERT_EQUAL(poolHdr->reservedMemRemaining, expectReservedMemRemaining);

    memAmount = requestedAmount = 100;
    rc = iemp_useReservedMem( pThreadData
                            , memPoolHdl
                            , &memAmount
                            , &rsvdMem1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_GREATER_THAN((void *)rsvdMem1, (void *)memPoolHdl);
    TEST_ASSERT_GREATER_THAN(((void *)memPoolHdl)+initialPageSize, (void *)rsvdMem1);
    TEST_ASSERT_GREATER_THAN_OR_EQUAL(memAmount, requestedAmount);
    expectReservedMemRemaining -= memAmount;
    TEST_ASSERT_EQUAL(poolHdr->reservedMemRemaining, expectReservedMemRemaining);

    memset(rsvdMem1, 'a', 100);

    void *rsvdMem2 = NULL;
    memAmount = requestedAmount = 100;
    rc = iemp_useReservedMem( pThreadData
                            , memPoolHdl
                            , &memAmount
                            , &rsvdMem2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_GREATER_THAN((void *)rsvdMem2, (void *)memPoolHdl);
    TEST_ASSERT_GREATER_THAN(((void *)memPoolHdl)+initialPageSize, (void *)rsvdMem2);
    TEST_ASSERT_GREATER_THAN_OR_EQUAL(memAmount, requestedAmount);
    expectReservedMemRemaining -= memAmount;
    TEST_ASSERT_EQUAL(poolHdr->reservedMemRemaining, expectReservedMemRemaining);

    memset(rsvdMem1, 'b', 100);

    void *normalMem = NULL;

    rc = iemp_allocate( pThreadData
                      , memPoolHdl
                      , 500
                      , &normalMem);
    memset(normalMem, 'c', 500);

    //Check this all fits on the initial page...
    TEST_ASSERT_EQUAL(memPoolHdl->nextPage, NULL);
    TEST_ASSERT_EQUAL(poolHdr->reservedMemRemaining, expectReservedMemRemaining);

    for (uint32_t i=0; i < 100; i++)
    {
        void *loopMem = NULL;

        rc = iemp_allocate( pThreadData
                          , memPoolHdl
                          , 500
                          , &loopMem);
        memset(normalMem, i, 500);
    }

    //Check we are now using multiple pages
    TEST_ASSERT_NOT_EQUAL(memPoolHdl->nextPage, NULL);
    TEST_ASSERT_EQUAL(poolHdr->reservedMemRemaining, expectReservedMemRemaining);

    // Clear keeping reserved
    iemp_clearMemPool( pThreadData, memPoolHdl, true);

    //Check we are now only one page is left and the reserved is untouched
    TEST_ASSERT_EQUAL(memPoolHdl->nextPage, NULL);
    TEST_ASSERT_EQUAL(poolHdr->reservedMemRemaining, expectReservedMemRemaining);

    // Clear releasing reserved
    iemp_clearMemPool(pThreadData, memPoolHdl, false);
    expectReservedMemRemaining = poolHdr->reservedMemInitial;
    TEST_ASSERT_EQUAL(poolHdr->reservedMemRemaining, expectReservedMemRemaining);

    iemp_destroyMemPool( pThreadData
                       , &memPoolHdl);
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

CU_TestInfo ISM_MemPool_CUnit_test_Basic[] =
{
    { "test_MemPoolBasic1", test_MemPoolBasic1 },
    CU_TEST_INFO_NULL
};

CU_SuiteInfo ISM_MemPool_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("BasicSuite", initSuite, termSuite, ISM_MemPool_CUnit_test_Basic),
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
