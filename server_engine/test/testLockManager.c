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

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "engineInternal.h"
#include "lockManager.h"
#include "lockManagerInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_assert.h"
#include "test_utils_file.h"
#include "test_utils_task.h"

/*********************************************************************/
/*                                                                   */
/* Module Name: testLockManager.c                                    */
/*                                                                   */
/* Description: Engine component lock manager unit test              */
/*                                                                   */
/*********************************************************************/
typedef struct
{
    pthread_cond_t condAtoB;
    pthread_cond_t condBtoA;
    pthread_mutex_t batonMutex;
    bool fBatonOwnerIsA;
    bool fTestFailed;
    uint32_t qId;
    uint32_t nodeId;
    uint32_t qId2;
    uint32_t nodeId2;
} CONTEXT;

void *threadAMain(void *pArg)
{
    CONTEXT *pContext = (CONTEXT *)pArg;
    ielmLockName_t lockName1;
    ielmLockScopeHandle_t hLockScopeA;
    ielmLockRequestHandle_t hLockRequest1;
    int32_t rc = OK;

    ism_engine_threadInit(0);

    lockName1.Msg.LockType = ielmLOCK_TYPE_MESSAGE;
    lockName1.Msg.QId = pContext->qId;
    lockName1.Msg.NodeId = pContext->nodeId;

    printf("Thread A.\n");

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    /* Thread A starts with the baton */
    rc = ielm_allocateLockScope(pThreadData, ielmLOCK_SCOPE_COMMIT_CAPABLE, NULL, &hLockScopeA);
    if (rc == OK)
    {
        /* First request the lock with instant-duration */
        rc = ielm_instantLockWithPeek(&lockName1,
                                      NULL,
                                      NULL);
        if (rc == OK)
        {
            printf("Thread A got lock with instant-duration.\n");
        }
        else
        {
            printf("Thread A failed to get lock with instant-duration.\n");
            goto exit;
        }

        /* Now take the lock with commit-duration */
        rc = ielm_takeLock(pThreadData,
                           hLockScopeA,
                           NULL,
                           &lockName1,
                           ielmLOCK_MODE_X,
                           ielmLOCK_DURATION_COMMIT,
                           &hLockRequest1);
        if (rc == OK)
        {
            printf("Thread A took lock with commit-duration.\n");
        }
        else
        {
            printf("Thread A failed to get lock with commit-duration.\n");
            goto exit;
        }

        /* Now request the lock with instant-duration again, which should fail */
        rc = ielm_instantLockWithPeek(&lockName1,
                                      NULL,
                                      NULL);
        if (rc == OK)
        {
            printf("Thread A got lock with instant-duration - wrongly.\n");
            rc = ISMRC_LockNotGranted;
            goto exit;
        }
        else
        {
            printf("Thread A failed to get lock with instant-duration - correctly.\n");
            rc = OK;
        }

        /* Tell thread B to try to request it */
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fBatonOwnerIsA = false;
        pthread_mutex_unlock(&pContext->batonMutex);
        printf("Thread A signalling thread B to continue.\n");
        pthread_cond_signal(&pContext->condAtoB);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* And wait for thread B to signal us back */
        pthread_mutex_lock(&pContext->batonMutex);
        while (!pContext->fBatonOwnerIsA)
        {
            if (pContext->fTestFailed)
            {
                pthread_exit(NULL);
            }

            pthread_cond_wait(&pContext->condBtoA, &pContext->batonMutex);
        }

        pthread_mutex_unlock(&pContext->batonMutex);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* Now begin release of the lock */
        printf("Thread A beginning release.\n");
        ielm_releaseAllLocksBegin(hLockScopeA);

        /* Tell thread B to try to request it */
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fBatonOwnerIsA = false;
        pthread_mutex_unlock(&pContext->batonMutex);
        printf("Thread A signalling thread B to continue.\n");
        pthread_cond_signal(&pContext->condAtoB);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* Go to sleep for a while */
        sleep(2);

        /* Now complete release of the lock */
        printf("Thread A completing release.\n");
        ielm_releaseAllLocksComplete(pThreadData, hLockScopeA);

        /* And wait for thread B to signal us back */
        pthread_mutex_lock(&pContext->batonMutex);
        while (!pContext->fBatonOwnerIsA)
        {
            pthread_cond_wait(&pContext->condBtoA, &pContext->batonMutex);
            if (pContext->fTestFailed)
            {
                pthread_exit(NULL);
            }
        }

        pthread_mutex_unlock(&pContext->batonMutex);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        ielm_freeLockScope(pThreadData, &hLockScopeA);
    }

exit:
    if (rc != OK)
    {
        printf("Thread A failed %d.\n", rc);
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fTestFailed = true;
        pthread_mutex_unlock(&pContext->batonMutex);
        if (pContext->fBatonOwnerIsA)
        {
            pthread_cond_signal(&pContext->condAtoB);
        }
    }

    return NULL;
}

void *threadBMain(void *pArg)
{
    CONTEXT *pContext = (CONTEXT *)pArg;
    ielmLockName_t lockName1;
    ielmLockScopeHandle_t hLockScopeA;
    ielmLockRequestHandle_t hLockRequest1;
    int32_t rc = OK;

    ism_engine_threadInit(0);

    lockName1.Msg.LockType = ielmLOCK_TYPE_MESSAGE;
    lockName1.Msg.QId = pContext->qId;
    lockName1.Msg.NodeId = pContext->nodeId;

    printf("Thread B.\n");

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = ielm_allocateLockScope(pThreadData, ielmLOCK_SCOPE_COMMIT_CAPABLE, NULL, &hLockScopeA);
    if (rc == OK)
    {
        /* Wait for thread A to signal us */
        pthread_mutex_lock(&pContext->batonMutex);
        if (pContext->fBatonOwnerIsA)printf("Thread B is waiting for A to signal\n");
        while (pContext->fBatonOwnerIsA)
        {
            pthread_cond_wait(&pContext->condAtoB, &pContext->batonMutex);
        }
        pthread_mutex_unlock(&pContext->batonMutex);

        /* Request the lock with commit-duration */
        printf("Thread B requesting lock which is not supposed to get.\n");
        rc = ielm_requestLock(pThreadData,
                              hLockScopeA,
                              &lockName1,
                              ielmLOCK_MODE_X,
                              ielmLOCK_DURATION_COMMIT,
                              &hLockRequest1);
        if (rc == ISMRC_LockNotGranted)
        {
            printf("Thread B didn't get lock - excellent.\n");
            rc = OK;
        }
        else if (rc == OK)
        {
            printf("Thread B got the lock, but thread A should still hold it.\n");
            rc = ISMRC_Error;
            goto exit;
        }
        else
        {
            goto exit;
        }

        /* Tell thread A that we failed to get it */
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fBatonOwnerIsA = true;
        pthread_mutex_unlock(&pContext->batonMutex);
        printf("Thread B signalling thread A to continue.\n");
        pthread_cond_signal(&pContext->condBtoA);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* Wait for thread A to signal us that it's begun to release the lock */
        pthread_mutex_lock(&pContext->batonMutex);
        if (pContext->fBatonOwnerIsA)printf("Thread B is waiting for A to signal\n");
        while (pContext->fBatonOwnerIsA)
        {
            if (pContext->fTestFailed)
            {
                pthread_exit(NULL);
            }

            pthread_cond_wait(&pContext->condAtoB, &pContext->batonMutex);
        }

        pthread_mutex_unlock(&pContext->batonMutex);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* Request the lock with commit-duration */
        printf("Thread B is requesting the lock, but it must be delayed.\n");
        rc = ielm_requestLock(pThreadData,
                              hLockScopeA,
                              &lockName1,
                              ielmLOCK_MODE_X,
                              ielmLOCK_DURATION_COMMIT,
                              &hLockRequest1);
        if (rc == OK)
        {
            printf("Thread B got the lock, but should have been delayed for the commit.\n");
        }
        else
        {
            goto exit;
        }

        /* Tell thread A that we got it */
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fBatonOwnerIsA = true;
        pthread_mutex_unlock(&pContext->batonMutex);
        printf("Thread B signalling thread A to continue.\n");
        pthread_cond_signal(&pContext->condBtoA);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* Now begin release of the lock */
        ielm_releaseAllLocksBegin(hLockScopeA);

        /* Now complete release of the lock */
        ielm_releaseAllLocksComplete(pThreadData, hLockScopeA);

        ielm_freeLockScope(pThreadData, &hLockScopeA);
    }

exit:
    if (rc != OK)
    {
        printf("Thread B failed %d.\n", rc);
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fTestFailed = true;
        pthread_mutex_unlock(&pContext->batonMutex);
        if (!pContext->fBatonOwnerIsA)
        {
            pthread_cond_signal(&pContext->condBtoA);
        }
    }

    return NULL;
}

void *threadAReqMain(void *pArg)
{
    CONTEXT *pContext = (CONTEXT *)pArg;
    ielmLockName_t lockName1;
    ielmLockName_t lockName2;
    ielmLockScopeHandle_t hLockScopeA;
    ielmLockRequestHandle_t hLockRequest1;
    ielmLockRequestHandle_t hLockRequest2;
    int32_t rc = OK;

    ism_engine_threadInit(0);

    lockName1.Msg.LockType = ielmLOCK_TYPE_MESSAGE;
    lockName1.Msg.QId = pContext->qId;
    lockName1.Msg.NodeId = pContext->nodeId;

    lockName2.Msg.LockType = ielmLOCK_TYPE_MESSAGE;
    lockName2.Msg.QId = pContext->qId2;
    lockName2.Msg.NodeId = pContext->nodeId2;

    printf("Thread AReq.\n");

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    /* Thread A starts with the baton */
    rc = ielm_allocateLockScope(pThreadData, ielmLOCK_SCOPE_COMMIT_CAPABLE, NULL, &hLockScopeA);
    if (rc == OK)
    {
        /* First request the lock with request-duration */
        rc = ielm_requestLock(pThreadData,
                              hLockScopeA,
                              &lockName1,
                              ielmLOCK_MODE_X,
                              ielmLOCK_DURATION_REQUEST,
                              &hLockRequest1);
        if (rc == OK)
        {
            printf("Thread AReq took lock with request-duration.\n");
        }
        else
        {
            printf("Thread AReq failed to get lock with request-duration.\n");
            goto exit;
        }

        /* Now request the lock with instant-duration again, which should fail */
        rc = ielm_instantLockWithPeek(&lockName1,
                                      NULL,
                                      NULL);
        if (rc == OK)
        {
            printf("Thread AReq got lock with instant-duration - wrongly.\n");
            rc = ISMRC_LockNotGranted;
            goto exit;
        }
        else
        {
            printf("Thread AReq failed to get lock with instant-duration - correctly.\n");
        }

        /* Release the lock without freeing the storage - we're going to use it again */
        ielm_releaseLockNoFree(hLockScopeA,
                               hLockRequest1);
        printf("Thread AReq released the first lock singly.\n");

        /* Take the lock again with request-duration using the same storage */
        rc = ielm_takeLock(pThreadData,
                           hLockScopeA,
                           hLockRequest1,
                           &lockName1,
                           ielmLOCK_MODE_X,
                           ielmLOCK_DURATION_REQUEST,
                           &hLockRequest1);
        if (rc == OK)
        {
            printf("Thread AReq retook lock with request-duration.\n");
        }
        else
        {
            printf("Thread AReq failed to get lock with request-duration.\n");
            goto exit;
        }

        /* Now request the lock with instant-duration again, which should fail */
        rc = ielm_instantLockWithPeek(&lockName1,
                                      NULL,
                                      NULL);
        if (rc == OK)
        {
            printf("Thread AReq got lock with instant-duration - wrongly.\n");
            rc = ISMRC_LockNotGranted;
            goto exit;
        }
        else
        {
            printf("Thread AReq failed to get lock with instant-duration - correctly.\n");
        }

        /* Tell thread B to try to request it */
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fBatonOwnerIsA = false;
        pthread_mutex_unlock(&pContext->batonMutex);
        printf("Thread AReq signalling thread BReq to continue.\n");
        pthread_cond_signal(&pContext->condAtoB);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* And wait for thread B to signal us back */
        pthread_mutex_lock(&pContext->batonMutex);
        while (!pContext->fBatonOwnerIsA)
        {
            if (pContext->fTestFailed)
            {
                pthread_exit(NULL);
            }

            pthread_cond_wait(&pContext->condBtoA, &pContext->batonMutex);
        }
        pthread_mutex_unlock(&pContext->batonMutex);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* Request a second lock with commit-duration */
        rc = ielm_takeLock(pThreadData,
                           hLockScopeA,
                           NULL,
                           &lockName2,
                           ielmLOCK_MODE_X,
                           ielmLOCK_DURATION_COMMIT,
                           &hLockRequest2);
        if (rc == OK)
        {
            printf("Thread AReq took second lock with commit-duration.\n");
        }
        else
        {
            printf("Thread AReq failed to get lock with commit-duration.\n");
            goto exit;
        }

        /* And release the first lock by itself */
        ielm_releaseLock(pThreadData,
                         hLockScopeA,
                         hLockRequest1);
        printf("Thread AReq released the first lock singly.\n");

        /* Now begin release of the lock */
        printf("Thread AReq beginning release.\n");
        ielm_releaseAllLocksBegin(hLockScopeA);

        /* Tell thread B to try to request them */
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fBatonOwnerIsA = false;
        pthread_mutex_unlock(&pContext->batonMutex);
        printf("Thread AReq signalling thread BReq to continue.\n");
        pthread_cond_signal(&pContext->condAtoB);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* Go to sleep for a while */
        sleep(2);

        /* Now complete release of the lock */
        printf("Thread AReq completing release.\n");
        ielm_releaseAllLocksComplete(pThreadData, hLockScopeA);

        /* And wait for thread B to signal us back */
        pthread_mutex_lock(&pContext->batonMutex);
        while (!pContext->fBatonOwnerIsA)
        {
            pthread_cond_wait(&pContext->condBtoA, &pContext->batonMutex);
            if (pContext->fTestFailed)
            {
                pthread_exit(NULL);
            }
        }

        pthread_mutex_unlock(&pContext->batonMutex);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* Take the lock again with request-duration */
        rc = ielm_takeLock(pThreadData,
                           hLockScopeA,
                           NULL,
                           &lockName1,
                           ielmLOCK_MODE_X,
                           ielmLOCK_DURATION_REQUEST,
                           &hLockRequest1);
        if (rc == OK)
        {
            printf("Thread AReq retook lock with request-duration one last time.\n");
        }
        else
        {
            printf("Thread AReq failed to get lock with request-duration.\n");
            goto exit;
        }

        /* Release the lock without freeing the storage - we're going to use it again */
        ielm_releaseLockNoFree(hLockScopeA,
                               hLockRequest1);

        /* And finally free the storage */
        ielm_freeLockRequest(pThreadData, hLockRequest1);
        printf("Thread AReq released and freed the lock.\n");

        ielm_freeLockScope(pThreadData, &hLockScopeA);
    }

exit:
    if (rc != OK)
    {
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fTestFailed = true;
        pthread_mutex_unlock(&pContext->batonMutex);
        if (pContext->fBatonOwnerIsA)
        {
            pthread_cond_signal(&pContext->condAtoB);
        }
    }

    return NULL;
}

void *threadBReqMain(void *pArg)
{
    CONTEXT *pContext = (CONTEXT *)pArg;
    ielmLockName_t lockName1;
    ielmLockName_t lockName2;
    ielmLockScopeHandle_t hLockScopeA;
    ielmLockRequestHandle_t hLockRequest1;
    ielmLockRequestHandle_t hLockRequest2;
    int32_t rc = OK;

    ism_engine_threadInit(0);

    lockName1.Msg.LockType = ielmLOCK_TYPE_MESSAGE;
    lockName1.Msg.QId = pContext->qId;
    lockName1.Msg.NodeId = pContext->nodeId;

    lockName2.Msg.LockType = ielmLOCK_TYPE_MESSAGE;
    lockName2.Msg.QId = pContext->qId2;
    lockName2.Msg.NodeId = pContext->nodeId2;

    printf("Thread BReq.\n");

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = ielm_allocateLockScope(pThreadData, ielmLOCK_SCOPE_COMMIT_CAPABLE, NULL, &hLockScopeA);
    if (rc == OK)
    {
        /* Wait for thread A to signal us */
        pthread_mutex_lock(&pContext->batonMutex);
        if (pContext->fBatonOwnerIsA)printf("Thread BReq is waiting for AReq to signal\n");
        while (pContext->fBatonOwnerIsA)
        {
            pthread_cond_wait(&pContext->condAtoB, &pContext->batonMutex);
        }
        pthread_mutex_unlock(&pContext->batonMutex);

        /* Request the lock with commit-duration */
        printf("Thread BReq requesting lock which is not supposed to get.\n");
        rc = ielm_requestLock(pThreadData,
                              hLockScopeA,
                              &lockName1,
                              ielmLOCK_MODE_X,
                              ielmLOCK_DURATION_COMMIT,
                              &hLockRequest1);
        if (rc == ISMRC_LockNotGranted)
        {
            printf("Thread BReq didn't get lock - excellent.\n");
            rc = OK;
        }
        else if (rc == OK)
        {
            printf("Thread BReq got the lock, but thread AReq should still hold it.\n");
            rc = ISMRC_Error;
            goto exit;
        }
        else
        {
            goto exit;
        }

        /* Tell thread A that we failed to get it */
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fBatonOwnerIsA = true;
        pthread_mutex_unlock(&pContext->batonMutex);
        printf("Thread BReq signalling thread AReq to continue.\n");
        pthread_cond_signal(&pContext->condBtoA);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* Wait for thread A to signal us that it's begun to release the lock */
        pthread_mutex_lock(&pContext->batonMutex);
        if (pContext->fBatonOwnerIsA)printf("Thread BReq is waiting for AReq to signal\n");
        while (pContext->fBatonOwnerIsA)
        {
            if (pContext->fTestFailed)
            {
                pthread_exit(NULL);
            }

            pthread_cond_wait(&pContext->condAtoB, &pContext->batonMutex);
        }
        pthread_mutex_unlock(&pContext->batonMutex);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        /* Request the singly-released lock with commit-duration */
        printf("Thread BReq is requesting the singly-released lock.\n");
        rc = ielm_requestLock(pThreadData,
                              hLockScopeA,
                              &lockName1,
                              ielmLOCK_MODE_X,
                              ielmLOCK_DURATION_COMMIT,
                              &hLockRequest1);
        if (rc == OK)
        {
            printf("Thread BReq got the lock.\n");
        }
        else
        {
            goto exit;
        }

        /* Request the lock with commit-duration */
        printf("Thread BReq is requesting the lock, but it must be delayed.\n");
        rc = ielm_requestLock(pThreadData,
                              hLockScopeA,
                              &lockName2,
                              ielmLOCK_MODE_X,
                              ielmLOCK_DURATION_COMMIT,
                              &hLockRequest2);
        if (rc == OK)
        {
            printf("Thread BReq got the lock, but should have been delayed for the commit.\n");
        }
        else
        {
            goto exit;
        }

        /* Tell thread A that we got it */
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fBatonOwnerIsA = true;
        pthread_mutex_unlock(&pContext->batonMutex);
        printf("Thread BReq signalling thread AReq to continue.\n");
        pthread_cond_signal(&pContext->condBtoA);
        if (pContext->fTestFailed)
        {
            pthread_exit(NULL);
        }

        // Test debugging routines with some locks
        int origStdout = test_redirectStdout("/dev/null");
        if (origStdout != -1)
        {
            char debugFile[10] = "";
            rc = ism_engine_dumpLocks(NULL, 5, 0, debugFile);
            TEST_ASSERT(rc == OK, ("Failed to dump locks. rc = %d", rc));
            test_reinstateStdout(origStdout);
        }

        /* Now begin release of the lock */
        ielm_releaseAllLocksBegin(hLockScopeA);

        /* Now complete release of the lock */
        ielm_releaseAllLocksComplete(pThreadData, hLockScopeA);

        ielm_freeLockScope(pThreadData, &hLockScopeA);
    }

exit:
    if (rc != OK)
    {
        pthread_mutex_lock(&pContext->batonMutex);
        pContext->fTestFailed = true;
        pthread_mutex_unlock(&pContext->batonMutex);
        if (!pContext->fBatonOwnerIsA)
        {
            pthread_cond_signal(&pContext->condBtoA);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int trclvl = 0;
    testLogLevel_t testLogLevel = testLOGLEVEL_TESTDESC;
    CONTEXT ctxt = { PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER,
                     PTHREAD_MUTEX_INITIALIZER, true, false,
                     123, 456, 789, 321 };
    pthread_t threadA;
    pthread_t threadB;
    int32_t rc;

// lockManagerInternal.h
printf("ielmLockManager_t %lu (was 16)\n", sizeof(ielmLockManager_t));
printf("ielmLockHashChain_t %lu (was 72)\n", sizeof(ielmLockHashChain_t));
printf("ielmLockScope_t %lu (was 64)\n", sizeof(ielmLockScope_t));
printf("ielmLockRequest_t %lu (was 88)\n", sizeof(ielmLockRequest_t));
printf("ielmAtomicRelease_t %lu (was 120)\n", sizeof(ielmAtomicRelease_t));
    test_setLogLevel(testLogLevel);

    rc = test_processInit(trclvl, NULL);
    if (rc != 0)
    {
        goto mod_exit;
    }

    test_log(testLOGLEVEL_TESTDESC, "Starting engine - cold start");
    rc = test_engineInit_DEFAULT;
    if (rc != 0)
    {
        goto mod_exit;
    }

    if (test_task_startThread(&threadA,threadAMain, &ctxt,"threadAMain") != 0)
    {
        rc = ISMRC_Error;
        goto mod_exit;
    }

    if (test_task_startThread(&threadB,threadBMain, &ctxt,"threadBMain") != 0)
    {
        rc = ISMRC_Error;
        goto mod_exit;
    }

    pthread_join(threadA, NULL);

    pthread_join(threadB, NULL);

    if (!ctxt.fTestFailed)
    {
        if (test_task_startThread(&threadA,threadAReqMain, &ctxt,"threadAReqMain") != 0)
        {
            rc = ISMRC_Error;
            goto mod_exit;
        }

        if (test_task_startThread(&threadB,threadBReqMain, &ctxt,"threadBReqMain") != 0)
        {
            rc = ISMRC_Error;
            goto mod_exit;
        }

        pthread_join(threadA, NULL);

        pthread_join(threadB, NULL);
    }

    // Test debugging routines without any locks
    int origStdout = test_redirectStdout("/dev/null");
    if (origStdout != -1)
    {
        char debugFile[10] = "";
        rc = ism_engine_dumpLocks(NULL, 5, 0, debugFile);
        TEST_ASSERT(rc == OK, ("Failed to dump locks. rc = %d", rc));
        test_reinstateStdout(origStdout);
    }

    rc = test_engineTerm(true);
    TEST_ASSERT( (rc == OK),("'test_engineTerm' failed. rc = %d", rc) );

    rc = test_processTerm(true);
    if (rc != OK) goto mod_exit;

mod_exit:

    if ((rc == OK) && !ctxt.fTestFailed)
    {
        printf("Test passed.\n");
    }
    else
    {
        printf("Test failed.\n");
    }

    return rc;
}
