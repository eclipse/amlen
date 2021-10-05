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

/*********************************************************************/
/*                                                                   */
/* Module Name: test_startStop.c                                     */
/*                                                                   */
/* Description: Testing a scenario described at:                     */
/* https://github.ibm.com/wiotp-connect/wiotp-connect/issues/386 )   */
/*                                                                   */
/* MessageSight detected a "split brain" - both servers in an HA Pair*/
/* thought they were the primary.                                    */
/* One server decided it should be the secondary. It then shutdown   */
/* all the usual components (engine/store etc) and started new       */
/* threads to wipe the persistent state. When these threads finished */
/* their work and exited, they crashed inside the                    */
/* engine whilst flushing the stats tracking usage for different     */
/* orgs ("Resource Set stats").                                      */
/*                                                                   */
/*********************************************************************/

#include <stdbool.h>

#include "engine.h"
#include "engineInternal.h"
#include "memHandler.h"
#include "resourceSetStats.h"
#include "resourceSetStatsInternal.h"
#include "engineDiag.h"

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_config.h"
#include "test_utils_task.h"

void *thread_JustThreadInitTerm(void *);

// This phase just sets up the resource set descriptor for the next phases...
void test_Phase1_SetupResourceSetDescriptor(void)
{
    int32_t rc;

    rc = test_configProcessPost("{\"ResourceSetDescriptor\": {\"ClientID\": \"^[^:]+:([^:]+):\", \"Topic\": \"^iot[^/]+/([^/+#]+)/\"}}");

    TEST_ASSERT_EQUAL(rc, OK);
}


//Start&Stop the engine THEN threadinit/threadterm a new thread
void test_Phase2_StartStopThenThreadStartStop(void)
{
    test_engineInit_WARM;

    //Turn off Clean shutdown to do the fast mode that we do in production...
    // Set clean shutdown to true so that we can valgrind our unit tests
    ism_field_t f;
    f.type = VT_Boolean;
    f.val.i = 0;

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();
    int rc = ism_common_setProperty(staticConfigProps, ismENGINE_CFGPROP_CLEAN_SHUTDOWN, &f);
    TEST_ASSERT_EQUAL(rc, OK);


    test_engineTerm(false);

    pthread_t threadId;
    int os_rc = test_task_startThread( &threadId,thread_JustThreadInitTerm, NULL,"thread_JustThreadInitTerm");
    TEST_ASSERT_EQUAL(os_rc, 0);

    os_rc = pthread_join(threadId, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
}

void *thread_JustThreadInitTerm(void *args)
{
    ism_engine_threadInit(0);

    //ieut_breakIntoDebugger();

    ism_engine_threadTerm(0);

    return NULL;
}

CU_TestInfo ISM_test_Phase1_Setup[] =
{
    { "SetupResourceDescriptor", test_Phase1_SetupResourceSetDescriptor },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_test_Phase2_StartStopThreadAfterEngineTerm[] =
{
    { "test_StartStopThenThreadStartStop", test_Phase2_StartStopThenThreadStartStop },
    CU_TEST_INFO_NULL
};



/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/

int initEngineCold(void)
{
    return test_engineInit_COLD;
}

int initEngineWarm(void)
{
    return test_engineInit_WARM;
}

int termEngineCold(void)
{
    return test_engineTerm(true);
}

int termEngineWarm(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_ResourceSets_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("PreRestartSetup", initEngineCold, termEngineWarm, ISM_test_Phase1_Setup),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ResourceSets_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("StartStopThenThreadStartStop", NULL, NULL, ISM_test_Phase2_StartStopThreadAfterEngineTerm),
    CU_SUITE_INFO_NULL,
};


CU_SuiteInfo *PhaseSuites[] = { ISM_ResourceSets_CUnit_phase1suites,
                                ISM_ResourceSets_CUnit_phase2suites,
                              };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
