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

/*********************************************************************/
/*                                                                   */
/* Module Name: test_lockManager.c                                   */
/*                                                                   */
/* Description: Test a subset of lockManager that is not included in */
/*              other unit tests.                                    */
/*                                                                   */
/*********************************************************************/
#include "test_lockManager.h"
#include "test_utils_assert.h"
#include "test_utils_initterm.h"
#include "lockManager.h"

CU_TestInfo ISM_Engine_CUnit_LockManager[] = {
    { "ManyLocks", ManyLocks },
    CU_TEST_INFO_NULL
};

/*
 * Test the *ManyLocks* functions not tested elsewhere
 */
void ManyLocks(void)
{
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    ielmLockName_t lockName1;
    ielmLockScopeHandle_t hLockScope = NULL;
    ielmLockRequestHandle_t hLockRequest[1000];

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = ielm_allocateLockScope(pThreadData, ielmLOCK_SCOPE_COMMIT_CAPABLE,
                                                     NULL, &hLockScope);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hLockScope);

    const uint32_t lockCount = sizeof(hLockRequest)/sizeof(hLockRequest[0]);

    /* Now take the lock with commit-duration */
    uint32_t i;
    for(i=0; i<lockCount; i++)
    {
        lockName1.Msg.LockType = ielmLOCK_TYPE_MESSAGE;
        lockName1.Msg.QId = i/10;
        lockName1.Msg.NodeId = i;

        rc = ielm_takeLock(pThreadData,
                           hLockScope,
                           NULL,
                           &lockName1,
                           ielmLOCK_MODE_X,
                           ielmLOCK_DURATION_REQUEST,
                           &hLockRequest[i]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hLockRequest[i]);
    }

    rc = ielm_releaseManyLocksBegin(hLockScope);
    TEST_ASSERT_EQUAL(rc, OK);

    for(i=0; i<50; i++)
    {
        uint32_t lockNo = (uint32_t)(rand()%lockCount);

        if (hLockRequest[lockNo] != NULL)
        {
            ielm_releaseOneOfManyLocks(pThreadData, hLockScope, hLockRequest[lockNo]);
            hLockRequest[lockNo] = NULL;
        }
    }

    // Release 1st and last explicitly
    if (hLockRequest[0] != NULL) ielm_releaseOneOfManyLocks(pThreadData, hLockScope, hLockRequest[0]);
    if (hLockRequest[lockCount-1] != NULL) ielm_releaseOneOfManyLocks(pThreadData, hLockScope, hLockRequest[lockCount-1]);

    ielm_releaseManyLocksComplete(hLockScope);

    // Release all the rest
    rc = ielm_releaseManyLocksBegin(hLockScope);
    TEST_ASSERT_EQUAL(rc, OK);

    for(i=1; i<lockCount-1; i++)
    {
        if (hLockRequest[i] != NULL)
        {
            ielm_releaseOneOfManyLocks(pThreadData, hLockScope, hLockRequest[i]);
        }
    }

    ielm_releaseManyLocksComplete(hLockScope);

    ielm_freeLockScope(pThreadData, &hLockScope);
    TEST_ASSERT_PTR_NULL(hLockScope);
}
