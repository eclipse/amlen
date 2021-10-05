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

#include <stdio.h>
#include <assert.h>
#include <sys/prctl.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "msgCommon.h"
#include "policyInfo.h"
#include "waiterStatus.h"
#include "simpQInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_client.h"

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/

ismEngine_ClientStateHandle_t hClient;
ismEngine_SessionHandle_t     hSession;


void testEnableDisable(void)
{
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };

    int32_t rc = ism_engine_createConsumer( hSession
                        , ismDESTINATION_TOPIC
                        , "testEnableDisableEnable"
                        , &subAttrs
                        , NULL
                        , NULL
                        , 0
                        , NULL
                        , NULL
                        , test_getDefaultConsumerOptions(subAttrs.subOptions)
                        , &hConsumer
                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    ismEngine_Consumer_t *pConsumer = (ismEngine_Consumer_t *)hConsumer;

    //We've got a consumer (usecount = 1), and it's enabled so +another 1
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 2);

    iesqQueue_t *queue = (iesqQueue_t *)pConsumer->queueHandle;
    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_ENABLED);

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    rc = ieq_disableWaiter(pThreadData, pConsumer->queueHandle, pConsumer);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_DISABLED);
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 1)

    acquireConsumerReference(pConsumer); //An caller of enabled waiter increases count on consumer...
    __sync_fetch_and_add(&pSession->ActiveCallbacks, 1);

    rc = ieq_enableWaiter(pThreadData, pConsumer->queueHandle, pConsumer, IEQ_ENABLE_OPTION_DELIVER_LATER);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_ENABLED);

    rc = sync_ism_engine_destroyConsumer(hConsumer);
    TEST_ASSERT_EQUAL(rc, OK);
}


void testLockedEnableDisable(void)
{
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };

    int32_t rc = ism_engine_createConsumer( hSession
                        , ismDESTINATION_TOPIC
                        , "testEnableDisableEnable"
                        , &subAttrs
                        , NULL
                        , NULL
                        , 0
                        , NULL
                        , NULL
                        , test_getDefaultConsumerOptions(subAttrs.subOptions)
                        , &hConsumer
                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    ismEngine_Consumer_t *pConsumer = (ismEngine_Consumer_t *)hConsumer;

    //We've got a consumer (usecount = 1), and it's enabled so +another 1
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 2);

    iesqQueue_t *queue = (iesqQueue_t *)pConsumer->queueHandle;
    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_ENABLED);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    //Lock the consumer
    bool lockedOK = iews_bool_tryLockToState( &(queue->waiterStatus)
                                            , IEWS_WAITERSTATUS_ENABLED
                                            , IEWS_WAITERSTATUS_GETTING);
    TEST_ASSERT_EQUAL(lockedOK, true);

    //Try and enable it again...
    acquireConsumerReference(pConsumer); //A caller of enabled waiter increases count on consumer...
    __sync_fetch_and_add(&pSession->ActiveCallbacks, 1);
    rc = ieq_enableWaiter(pThreadData, pConsumer->queueHandle, pConsumer, IEQ_ENABLE_OPTION_DELIVER_LATER);
    TEST_ASSERT_EQUAL(rc, ISMRC_WaiterEnabled);
    releaseConsumerReference(pThreadData, pConsumer, false); //Cancel our speculative increase
    __sync_fetch_and_sub(&pSession->ActiveCallbacks, 1);
    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_GETTING);
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 2);

    //And now disable (whilst still locked)
    rc = ieq_disableWaiter(pThreadData, pConsumer->queueHandle, pConsumer);
    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    //Although disable "worked"... it just added a flag... our usecount is still high
    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_DISABLE_PEND);
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 2);

    //Unlock....
    ieq_completeWaiterActions( pThreadData
                             , pConsumer->queueHandle
                             , pConsumer
                             , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                             , true);

    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_DISABLED);
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 1);

    //Enable
    acquireConsumerReference(pConsumer); //An caller of enabled waiter increases count on consumer...
    __sync_fetch_and_add(&pSession->ActiveCallbacks, 1);
    rc = ieq_enableWaiter(pThreadData, pConsumer->queueHandle, pConsumer, IEQ_ENABLE_OPTION_DELIVER_LATER);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_ENABLED);
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 2);

    //Lock again
    lockedOK = iews_bool_tryLockToState( &(queue->waiterStatus)
                                       , IEWS_WAITERSTATUS_ENABLED
                                       , IEWS_WAITERSTATUS_GETTING);
    TEST_ASSERT_EQUAL(lockedOK, true);

    //Add disable flag and cancel it and complete the actions (i.e. unlock)
    rc = ieq_disableWaiter(pThreadData, pConsumer->queueHandle, pConsumer);
    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    acquireConsumerReference(pConsumer); //A caller of enabled waiter increases count on consumer...
    __sync_fetch_and_add(&pSession->ActiveCallbacks, 1);
    rc = ieq_enableWaiter(pThreadData, pConsumer->queueHandle, pConsumer, IEQ_ENABLE_OPTION_DELIVER_LATER);
    TEST_ASSERT_EQUAL(rc, ISMRC_DisableWaiterCancel);
    //speculative usecount increases needed as disable callback will fire reducing it and we want it to stay high

    TEST_ASSERT_EQUAL(queue->waiterStatus, (IEWS_WAITERSTATUS_DISABLE_PEND|IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND));
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 3);
    //Unlock....
    ieq_completeWaiterActions( pThreadData
                             , pConsumer->queueHandle
                             , pConsumer
                             , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                             , true);

    //disable was cancelled...after unlocking should be enabled
    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_ENABLED);
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 2);
    TEST_ASSERT_EQUAL(pSession->ActiveCallbacks, 2); // 2 as +1 if session is not destroyed

    //Lock again.
    lockedOK = iews_bool_tryLockToState( &(queue->waiterStatus)
                                       , IEWS_WAITERSTATUS_ENABLED
                                       , IEWS_WAITERSTATUS_GETTING);
    TEST_ASSERT_EQUAL(lockedOK, true);

    //We're going to: add disable flag, cancel it and add it again

    //first disable
    rc = ieq_disableWaiter(pThreadData, pConsumer->queueHandle, pConsumer);
    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_DISABLE_PEND);
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 2);
    //...cancel disable
    acquireConsumerReference(pConsumer); //A caller of enabled waiter increases count on consumer...
    __sync_fetch_and_add(&pSession->ActiveCallbacks, 1);
    rc = ieq_enableWaiter(pThreadData, pConsumer->queueHandle, pConsumer, IEQ_ENABLE_OPTION_DELIVER_LATER);
    TEST_ASSERT_EQUAL(rc, ISMRC_DisableWaiterCancel);
    //pre-enable usecount increases needed as disable callback will fire reducing it and we want it to stay high
    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_DISABLE_PEND|IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND);
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 3);

    //Add the disable flag again
    rc = ieq_disableWaiter(pThreadData, pConsumer->queueHandle, pConsumer);
    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_DISABLE_PEND);
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 2);
    //Unlock....
    ieq_completeWaiterActions( pThreadData
                             , pConsumer->queueHandle
                             , pConsumer
                             , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                             , true);

    TEST_ASSERT_EQUAL(queue->waiterStatus, IEWS_WAITERSTATUS_DISABLED);
    TEST_ASSERT_EQUAL(pConsumer->counts.parts.useCount, 1);

    rc = sync_ism_engine_destroyConsumer(hConsumer);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*********************************************************************/
/* Test suite definitions                                            */
/*********************************************************************/


int initSuite(void)
{
    int rc = test_createClientAndSession("test_waiterstatus",
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient, &hSession, true);
    return rc;
}

int termSuite(void)
{
    int rc = test_destroyClientAndSession(hClient, hSession, false);

    return rc;
}

CU_TestInfo ISM_Engine_CUnit_WaiterStatus_Basic[] = {
    { "testEnableDisable",         testEnableDisable},
    { "testLockedEnableDisable",   testLockedEnableDisable},
    CU_TEST_INFO_NULL
};

CU_SuiteInfo ISM_Engine_CUnit_simpQ_Suites[] = {
    IMA_TEST_SUITE("Basic", initSuite, termSuite, ISM_Engine_CUnit_WaiterStatus_Basic),
    CU_SUITE_INFO_NULL,
};


/*********************************************************************/
/* Main                                                              */
/*********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;


    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    retval = test_engineInit_DEFAULT;
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();
    CU_register_suites(ISM_Engine_CUnit_simpQ_Suites);

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
