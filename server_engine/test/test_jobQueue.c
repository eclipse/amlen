/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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

/*********************************************************************/
/*                                                                   */
/* Module Name: test_jobQueue.c                                      */
/*                                                                   */
/* Description: Main source file for CUnit test of inter thread work */
/* queues                                                            */
/*********************************************************************/

#include <sys/prctl.h>

#include "engine.h"
#include "engineInternal.h"
#include "test_utils_assert.h"
#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_task.h"

#include "jobQueue.h"

void test_JobQueueBasic1(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    iejqJobQueueHandle_t jqh = NULL;

    int32_t rc = iejq_createJobQueue(pThreadData, &jqh);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t maxPuttableJobs = IEJQ_JOB_MAX-1; //One for barrier
    //Fill and empty the queue a few times... interspersed with just putting a few messages
    for (uint64_t fill = 0; fill < 5; fill++)
    {
        uint64_t jobsToPut = maxPuttableJobs;
        uint64_t jobsAdded = 0;
        uint64_t jobsGot = 0;

        if (fill == 2)
        {
            jobsToPut = 5;
        }

        test_log(testLOGLEVEL_TESTPROGRESS, "Starting fill %lu...", fill);
#ifdef IEJQ_JOBQUEUE_PUTLOCK
        iejq_takePutLock(pThreadData, jqh);
#endif
        for (uint64_t i = 0; i < jobsToPut; i++)
        {
#ifdef IEJQ_JOBQUEUE_PUTLOCK
            rc = iejq_addJob(pThreadData, jqh, test_JobQueueBasic1, (void *)i, false);
#else
            rc = iejq_addJob(pThreadData, jqh, test_JobQueueBasic1, (void *)i);
#endif
            TEST_ASSERT_EQUAL(rc, OK);
            jobsAdded++;
        }

        //If it should be full now...check
        if (jobsAdded == maxPuttableJobs)
        {
#ifdef IEJQ_JOBQUEUE_PUTLOCK
            rc = iejq_addJob(pThreadData, jqh, test_JobQueueBasic1, (void *)1, false);
#else
            rc = iejq_addJob(pThreadData, jqh, test_JobQueueBasic1, (void *)1);
#endif
            TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);
        }

#ifdef IEJQ_JOBQUEUE_PUTLOCK
        iejq_releasePutLock(pThreadData, jqh);
#endif

        void *args;
        void *func;

        while (jobsGot < jobsAdded)
        {
            rc = iejq_getJob(pThreadData, jqh, &func, &args, true);
            TEST_ASSERT_EQUAL(rc, OK);

            TEST_ASSERT_EQUAL(func, test_JobQueueBasic1);
            TEST_ASSERT_EQUAL(args, (void *)jobsGot);

            jobsGot++;
        }

        //Should be empty now
        rc = iejq_getJob(pThreadData, jqh, &func, &args, true);
        TEST_ASSERT_EQUAL(rc, ISMRC_NoMsgAvail);
    }

    iejq_freeJobQueue(pThreadData, jqh);
}

typedef struct tag_testFlowPutterInfo_t {
    uint64_t numMessagesToPut;
    iejqJobQueueHandle_t jqh;
    uint64_t timesHitQueueFull;
} testFlowPutterInfo_t;

void *test_flowPutter(void * args)
{
    testFlowPutterInfo_t *pPutterInfo = (testFlowPutterInfo_t *)args;

    char tname[20];
    sprintf(tname, "flowPutter");
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    ism_engine_threadInit(0);
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint64_t numMessagesPut = 0;

    while (numMessagesPut < pPutterInfo->numMessagesToPut)
    {
#ifdef IEJQ_JOBQUEUE_PUTLOCK
        int32_t rc = iejq_addJob(pThreadData, pPutterInfo->jqh, test_flowPutter, (void *)numMessagesPut, true);
#else
        int32_t rc = iejq_addJob(pThreadData, pPutterInfo->jqh, test_flowPutter, (void *)numMessagesPut);
#endif

        if (rc == ISMRC_DestinationFull)
        {
            (pPutterInfo->timesHitQueueFull)++;
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            numMessagesPut++;
        }
    }

    ism_engine_threadTerm(1);

    return NULL;
}

void test_JobQueueFlow(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    iejqJobQueueHandle_t jqh = NULL;

    int32_t rc = iejq_createJobQueue(pThreadData, &jqh);
    TEST_ASSERT_EQUAL(rc, OK);


    pthread_t putterThreadId;

    uint64_t jobsToPut = ((IEJQ_JOB_MAX-1)*11)+17;


    testFlowPutterInfo_t putterInfo = {jobsToPut, jqh, 0};

    rc = test_task_startThread(&(putterThreadId),test_flowPutter, (void *)&putterInfo,"test_flowPutter");
    TEST_ASSERT_EQUAL(rc, OK);

    void *args;
    void *func;

    uint64_t jobsGot = 0;
    uint64_t timesHitEmptyQ = 0;

    //Get can request getlock
    bool takenGetLock = iejq_tryTakeGetLock(pThreadData, jqh);
    TEST_ASSERT_EQUAL(takenGetLock, true);
    iejq_releaseGetLock(pThreadData, jqh);

    iejq_takeGetLock(pThreadData, jqh);

    while (jobsGot < jobsToPut)
    {
        rc = iejq_getJob(pThreadData, jqh, &func, &args, false);

        if (rc == ISMRC_NoMsgAvail)
        {
            timesHitEmptyQ++;
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);

            TEST_ASSERT_EQUAL(func, test_flowPutter);
            TEST_ASSERT_EQUAL(args, (void *)jobsGot);

            jobsGot++;
        }
    }
    iejq_releaseGetLock(pThreadData, jqh);

    //Should be empty now
    rc = iejq_getJob(pThreadData, jqh, &func, &args, true);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMsgAvail);

    rc = pthread_join(putterThreadId, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    iejq_freeJobQueue(pThreadData, jqh);

    test_log(testLOGLEVEL_TESTPROGRESS, "Put and got %lu jobs...Hit Full queue %lu times and Empty queue %u times",
                       jobsToPut, putterInfo.timesHitQueueFull, timesHitEmptyQ);
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

CU_TestInfo ISM_JobQueue_CUnit_test_Basic[] =
{
    { "test_JobQueueBasic1", test_JobQueueBasic1 },
    { "test_JobQueueFlow",   test_JobQueueFlow },
    CU_TEST_INFO_NULL
};

CU_SuiteInfo ISM_MemPool_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("BasicSuite", initSuite, termSuite, ISM_JobQueue_CUnit_test_Basic),
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
