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

#include "ismutil.h"
#include "engine.h"
#include "engineInternal.h"
#include "memHandler.h"
#include "engineUtils.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_log.h"

#ifndef NO_MALLOC_WRAPPER

/* We don't want to use up all the memory in the system in order to test the reduce */
/* memory functions so we override the querySystemMemory with a function that uses  */
/* return fake values                                                               */
iemem_systemMemInfo_t fakeMemInfo = {   2 * IEMEM_GIBIBYTE,    //fake effective memory
                                      1.5 * IEMEM_GIBIBYTE,    //fake free memory inc. buffers+swap
                                      (1.5/2)*100,             //fake free percentage
                                      false,                   //fake 'from Cgroup'
                                        2 * IEMEM_GIBIBYTE,    //fake total memory
                                      1.1 * IEMEM_GIBIBYTE,    //fake free memory
                                                               //... Other values uninitialized
};

bool fail_iemem_querySystemMemory = false;

int32_t iemem_querySystemMemory(iemem_systemMemInfo_t *sysmeminfo)
{
    if (fail_iemem_querySystemMemory)
    {
        return ISMRC_Error;
    }
    else
    {
        *sysmeminfo = fakeMemInfo;
        return OK;
    }
}


bool expectReduceCallbackToFire = false;
int32_t reduceCBFiredCount = 0;
bool delayInCB = false;
bool delayed = false;

void reduceMsgBodiesMemory(iememMemoryLevel_t currState,
                           iememMemoryLevel_t prevState,
                           iemem_memoryType type,
                           uint64_t currentLevel,
                           iemem_systemMemInfo_t *memInfo)
{
    //Check we are being called for the memory type we expected
    TEST_ASSERT_EQUAL(type, iemem_messageBody);

    //Check we are only called if we expect to be.
    TEST_ASSERT_EQUAL(expectReduceCallbackToFire, true);

    //Increase the count of times this callback has been called
    __sync_add_and_fetch(&reduceCBFiredCount, 1);

    if (delayInCB)
    {
        (void)__sync_lock_test_and_set(&delayed, true);
        sleep(1);
        (void)__sync_lock_test_and_set(&delayed, false);
    }
}

//This test checks we can allocate memory, then when  memory allocation
//is disabled, it checks we can no longer allocate memory
void testSimpleDisable(void)
{
    void *memptrs[3] = {0};
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    test_log(testLOGLEVEL_TESTNAME, "testSimpleDisable...\n");

    //We want to be able to see the effect of disabling mallocs instantly
    //So don't let threads buffer any memory
    iemem_setMemChunkSize(0);

    uint32_t chunkSize = iemem_getMemChunkSize();
    TEST_ASSERT_EQUAL(chunkSize, 0);

    /* Check that we can malloc+calloc+realloc memory */
    memptrs[0] = iemem_malloc(pThreadData, iemem_messageBody, 1 * IEMEM_KIBIBYTE);
    TEST_ASSERT_NOT_EQUAL(memptrs[0], NULL);

    memptrs[1] = iemem_calloc(pThreadData, iemem_simpleQPage, 1, 1 * IEMEM_KIBIBYTE);
    TEST_ASSERT_NOT_EQUAL(memptrs[1], NULL);

    memptrs[1] = iemem_realloc(pThreadData, iemem_simpleQPage, memptrs[1], 2 * IEMEM_KIBIBYTE);
    TEST_ASSERT_NOT_EQUAL(memptrs[1], NULL);

    /* Check that disabling mallocs for one type of memory does that */
    iemem_setMallocStateForType(iemem_simpleQPage, false);

    void *shouldfail = iemem_malloc(pThreadData, iemem_simpleQPage, 1 * IEMEM_KIBIBYTE);
    TEST_ASSERT_EQUAL(shouldfail, NULL);

    /* ...but we can still alloc other types */
    memptrs[2] = iemem_malloc(pThreadData, iemem_messageBody, 1 * IEMEM_KIBIBYTE);
    TEST_ASSERT_NOT_EQUAL(memptrs[2], NULL);

    /* and that reallocing "disabled" types smaller is allowed */
    memptrs[1] = iemem_realloc(pThreadData, iemem_simpleQPage, memptrs[1], 1 * IEMEM_KIBIBYTE);
    TEST_ASSERT_NOT_EQUAL(memptrs[1], NULL);

    /* free all the successful allocs */
    iemem_free(pThreadData, iemem_messageBody, memptrs[0]);
    iemem_free(pThreadData, iemem_simpleQPage, memptrs[1]);
    iemem_free(pThreadData, iemem_messageBody, memptrs[2]);

    /* check we have arrived back at 0 */
    size_t mem_levels[iemem_numtypes];

    iemem_queryControlledMemory(mem_levels);

    int i;
    for (i = 0; i < iemem_numtypes; i++)
    {
        TEST_ASSERT_EQUAL(mem_levels[i], 0);
    }

    test_log(testLOGLEVEL_TESTNAME, "...OK");
}


//Test that by altering the levels of memory the memory watcher thinks are available
//we can cause:
// a) a callback to be called to reduce the level of memory in use
// b) memory allocations to be disabled

void testReduceDisable(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    test_log(testLOGLEVEL_TESTNAME, "testReduceDisable...");

    //We want to be able to see the effect of disabling mallocs instantly
    //So don't let threads buffer any memory
    iemem_setMemChunkSize(0);

    //Allocate some message body memory.. as our fake memory stats claim there is
    //plenty of free memory, this should NOT cause the callback we registered to
    //be called to try and reduce the amount
    void *somemem = iemem_malloc(pThreadData, iemem_messageBody, 10 * IEMEM_KIBIBYTE);
    TEST_ASSERT_NOT_EQUAL(somemem, NULL);

    //wait a while to check the callback isn't called
    sleep(1);

    // Prove that iemem_querySystemMemory failing doesn't prove fatal
    fail_iemem_querySystemMemory = true;

    //wait a while to ensure a check has been made
    sleep(1);

    fail_iemem_querySystemMemory = false;

    TEST_ASSERT_EQUAL(reduceCBFiredCount, 0);

    iemem_systemMemInfo_t lowMemInfo = {   2 * IEMEM_GIBIBYTE,  //fake effective memory
                                         0.5 * IEMEM_GIBIBYTE,  //fake free memory inc. buffers+swap
                                         (0.5/2)*100,           //fake free percentage
                                         false,                 //fake 'from Cgroup'
                                           2 * IEMEM_GIBIBYTE,  //fake total memory
                                         0.4 * IEMEM_GIBIBYTE,  //fake free memory
                                                                //... Other values uninitialized
    };

    //We're going to fake low mem so tell the "reduce memory" handler that we expect it to be called
    expectReduceCallbackToFire = true;

    //fake low mem
    fakeMemInfo = lowMemInfo;

    //wait until the callback is fired
    int sleeps = 0;
    while((reduceCBFiredCount == 0)&&(sleeps < 6))
    {
        usleep(200000);
        sleeps++;
        __sync_synchronize();
    }

    TEST_ASSERT(reduceCBFiredCount > 0, ("reduceCBFiredCount > 0"));

    //Now reduce the level of memory further and check that the ability to
    //allocate memory is turned off.
    iemem_systemMemInfo_t reallyLowMemInfo = {    2 * IEMEM_GIBIBYTE,  //fake effective memory
                                               0.03 * IEMEM_GIBIBYTE,  //fake free memory inc. buffers+swap
                                               (0.03/2)*100,           //fake free percentage
                                               false,                  //fake 'from Cgroup'
                                                  2 * IEMEM_GIBIBYTE,  //fake total memory
                                               0.01 * IEMEM_GIBIBYTE,  //fake free memory
                                                                       //... Other values uninitialized
    };
    fakeMemInfo = reallyLowMemInfo;

    //the memwatcher may not have noticed yet, wait for a bit and see if it gets turned off
    sleeps = 0;
    while(1)
    {
        void *moremem = iemem_malloc(pThreadData, iemem_subsTree, 10 * IEMEM_KIBIBYTE);

        if (moremem != NULL)
        {
            iemem_free(pThreadData, iemem_subsTree, moremem);
            usleep(200000);
            sleeps++;
        }
        else
        {
            break;
        }

        TEST_ASSERT(sleeps < 6, ("sleeps < 6"));
    }

    //Now increase the amount of memory so that we can allocate some but not message payloads
    iemem_systemMemInfo_t quiteLowMemInfo = {    2 * IEMEM_GIBIBYTE,  //fake effective memory
                                              0.22 * IEMEM_GIBIBYTE,  //fake free memory inc. buffers+swap
                                              (0.22/2)*100,           //fake free percentage
                                              false,                  //fake 'from Cgroup'
                                                 2 * IEMEM_GIBIBYTE,  //fake total memory
                                              0.01 * IEMEM_GIBIBYTE,  //fake free memory
                                                                      //... Other values uninitialized
    };
    fakeMemInfo = quiteLowMemInfo;

    //the memwatcher may not have noticed yet, wait for a bit and see if it notices...
    sleeps = 0;
    while(1)
    {
        void *moremem = iemem_malloc(pThreadData, iemem_subsTree, 10 * IEMEM_KIBIBYTE);

        if (moremem == NULL)
        {
            usleep(200000);
        }
        else
        {
            iemem_free(pThreadData, iemem_subsTree, moremem);
            usleep(200000);
            sleeps++;
            break;
        }

        TEST_ASSERT(sleeps < 6, ("sleeps < 6"));
    }
    //Now we can allocate iemem_subsTree again...but we are still low enough on memory that we shouldn't be allowed
    //to do iemem_messagebody
    DEBUG_ONLY void *shouldfailmem = iemem_malloc(pThreadData, iemem_messageBody, 10 * IEMEM_KIBIBYTE);
    assert(shouldfailmem == NULL);

    //Now check malloc gets fully turned back on when we claim memory is fine
    iemem_systemMemInfo_t plentyOfMemInfo = {    2 * IEMEM_GIBIBYTE,  //fake effective memory
                                               1.7 * IEMEM_GIBIBYTE,  //fake free memory inc. buffers+swap
                                              (1.7/2)*100,            //fake free percentage
                                              false,                  //fake 'from Cgroup'
                                                 2 * IEMEM_GIBIBYTE,  //fake total memory
                                               1.5 * IEMEM_GIBIBYTE,  //fake free memory
                                                                      //... Other values uninitialized
    };
    fakeMemInfo = plentyOfMemInfo;

    //the memwatcher may not have noticed yet, wait for a bit and see if it gets turned off
    sleeps = 0;
    while(1)
    {
        void *moremem = iemem_malloc(pThreadData, iemem_messageBody, 10 * IEMEM_KIBIBYTE);

        if (moremem != NULL)
        {
            iemem_free(pThreadData, iemem_messageBody, moremem);
            break;
        }
        else
        {
            usleep(200000);
            sleeps++;
        }

        TEST_ASSERT(sleeps < 6, ("sleeps < 6"));
    }

    // Make the reduceMemory callback delay and try terminating - to check
    // the mechanism for delaying termination until callbacks have finished
    fakeMemInfo = lowMemInfo;
    (void)__sync_lock_test_and_set(&delayInCB, true);

    while((volatile bool)delayed == false)
    {
        usleep(10000);
    }

    iemem_stopMemoryMonitorTask(pThreadData);
    iemem_termMemHandler(pThreadData);

    (void)__sync_lock_test_and_set(&delayInCB, false);

    iemem_startMemoryMonitorTask(pThreadData);

    //OK... let's tidy up and check everything smells right!
    fakeMemInfo = plentyOfMemInfo;

    iemem_free(pThreadData, iemem_messageBody, somemem);
    /* check we have arrived back at 0 */
    size_t mem_levels[iemem_numtypes];

    iemem_queryControlledMemory(mem_levels);

    int i;
    for (i = 0; i < iemem_numtypes; i++)
    {
        TEST_ASSERT_EQUAL(mem_levels[i], 0);
    }
    test_log(testLOGLEVEL_TESTNAME, "...OK\n");
}

void testAdminDisplayMem(void)
{
    ismEngine_MemoryStatistics_t stats = {0};

    int32_t rc = ism_engine_getMemoryStatistics(&stats);
    TEST_ASSERT_EQUAL(rc, OK);
}

void testUtilityFuncs(void)
{
    const char *memTypeName;

    // Just confirm a couple of mem types report the expected string
    memTypeName = iemem_getTypeName(iemem_topicAnalysis);
    TEST_ASSERT_STRINGS_EQUAL(memTypeName, "iemem_topicAnalysis");
    memTypeName = iemem_getTypeName(iemem_lockManager);
    TEST_ASSERT_STRINGS_EQUAL(memTypeName, "iemem_lockManager");
}

CU_TestInfo ISM_Engine_CUnit_memHandler[] = {
    { "testSimpleDisable", testSimpleDisable },
    { "testReduceDisable", testReduceDisable },
    { "testAdminDisplayMem", testAdminDisplayMem },
    { "testUtilityFuncs", testUtilityFuncs },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Engine_CUnit_debugging_memHandler[] = {
    { "testSimpleDisable (DEBUG)", testSimpleDisable },
    CU_TEST_INFO_NULL
};

int initMemHandler(void)
{
    ieut_setEngineRunPhase(EnginePhaseRunning);

    ieut_createBasicThreadData();

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    iemem_startMemoryMonitorTask(pThreadData);
    iemem_setMemoryReduceCallback(iemem_messageBody, reduceMsgBodiesMemory);
    return 0;
}

int termMemHandler(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    iemem_stopMemoryMonitorTask(pThreadData);
    iemem_termMemHandler(pThreadData);
    ism_engine_threadTerm(1);
    return 0;
}

int initMemHandlerDebug(void)
{
    setenv("ISM_DEBUG_MEMORY", "YES", true);
    return initMemHandler();}

int termMemHandlerDebug(void)
{
    int rc = termMemHandler();
    unsetenv("ISM_DEBUG_MEMORY");
    return rc;
}

CU_SuiteInfo ISM_Engine_CUnit_memHandler_Suites[] = {
    IMA_TEST_SUITE("memHandler", initMemHandler, termMemHandler, ISM_Engine_CUnit_memHandler),
    IMA_TEST_SUITE("memHandler (DEBUG)", initMemHandlerDebug, termMemHandlerDebug, ISM_Engine_CUnit_debugging_memHandler),
    CU_SUITE_INFO_NULL,
};

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int testlogllvl = testLOGLEVEL_TESTNAME;
    int retval = 0;

    test_setLogLevel(testlogllvl);

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();
    CU_register_suites(ISM_Engine_CUnit_memHandler_Suites);

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

    int32_t rc = test_processTerm(retval == 0);
    if (retval == 0) retval = rc;

mod_exit:

    return retval;
}
#else
int main(int argc, char *argv[])
{
    printf("Malloc wrapper disabled, ending test\n");
    return 0;
}
#endif
