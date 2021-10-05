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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <assert.h>
#include <engine.h>
#include <engineCommon.h>

#include "test_utils_task.h"


/*************************************************************/
/* Data declarations                                         */
/*************************************************************/
#define TASK_NAME_LEN 20

typedef struct test_task_defn_t
{
    char name[TASK_NAME_LEN+1];
    bool running;
    test_task_fnptr pfn;
    void *context;
    uint32_t Rate;
    uint64_t uDelay;
    pthread_t hthread;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    uint64_t TriggerCount;
    uint64_t MissCount;
    struct test_task_defn_t *pNext;
} test_task_defn_t;

typedef struct test_task_thread_t
{
    ism_simple_thread_func_t function;
} test_task_thread_t;

void *run_test_thread(void *arg, void *context, int _value)
{
    test_task_thread_t * param = (test_task_thread_t*)arg;
    return param->function(context);
}

XAPI int test_task_startThread(ism_threadh_t * handle, ism_simple_thread_func_t addr , void * data, const char * name)
{
    int rc;
    test_task_thread_t * parm;

    parm = calloc(1, sizeof(struct test_task_thread_t));
    parm->function = addr;
    rc = ism_common_startThread(handle, run_test_thread, parm, data, 0,
            0, 0, name, name);
    if (rc) {
        *handle = 0;
        //this is all done before the memusage is set up for the thread so
        // need to free the raw parms
        ism_common_free_raw(ism_memory_engine_misc, parm);
        TRACE(3, "Failed to create thread %s: error=%d\n", name, rc);
        return rc;
    }

    TRACE(5, "Create simple thread %s handle=%p data=%p\n", name,
            (void * )*handle, data);

    return rc;
}

/*************************************************************/
/* Global data declarations                                  */
/*************************************************************/
static pthread_mutex_t TaskListLock = PTHREAD_MUTEX_INITIALIZER;
static test_task_defn_t *TaskList = NULL;

/*************************************************************/
/* Worker thread                                             */
/*************************************************************/
static void *task_worker(void *arg)
{
    test_task_defn_t *ptask = (test_task_defn_t *)arg;
    struct timespec wakeTime;
    struct timespec curTime;
    bool cont=true;
    int osrc;
    uint64_t diffsec, diffnsec;

    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&ptask->name);

    ism_engine_threadInit(0);

    osrc = pthread_mutex_lock(&ptask->mutex);
    assert(osrc == 0);

    clock_gettime(CLOCK_REALTIME, &wakeTime);
    wakeTime.tv_nsec += ptask->uDelay;
    if (wakeTime.tv_nsec >= 1000000000)
    {
      wakeTime.tv_nsec-= 1000000000;
      wakeTime.tv_sec++;
    }

    test_log(testLOGLEVEL_TESTPROGRESS, "Task '%s' started. rate %ld",
             ptask->name, ptask->Rate);

    do
    {
        if (ptask->running)
        {
            clock_gettime(CLOCK_REALTIME, &wakeTime);
            wakeTime.tv_nsec = wakeTime.tv_nsec + ptask->uDelay;
            if (wakeTime.tv_nsec >= 1000000000)
            {
              wakeTime.tv_nsec-= 1000000000;
              wakeTime.tv_sec++;
            }

            ptask->TriggerCount++;
            ptask->pfn(ptask->context);

            clock_gettime(CLOCK_REALTIME, &curTime);
            if ((wakeTime.tv_sec < curTime.tv_sec) ||
                ((wakeTime.tv_sec == curTime.tv_sec) && (wakeTime.tv_nsec < curTime.tv_nsec)))
            {
                if (ptask->MissCount == 0)
                {
                    test_log(testLOGLEVEL_ERROR, "Task '%s' unable to match rate.", ptask->name);
                }
                diffsec = curTime.tv_sec-wakeTime.tv_sec;
                if (curTime.tv_nsec >= wakeTime.tv_nsec)
                {
                    diffnsec = curTime.tv_nsec-wakeTime.tv_nsec;
                }
                else
                {
                    diffnsec = 1000000000 - (wakeTime.tv_nsec-curTime.tv_nsec);
                    diffsec--;
                }
                diffnsec += (1000000000 * diffsec);
                ptask->MissCount += ((diffnsec / ptask->uDelay) + 1);

                continue;
            }
        }
        else
        {
            cont = false;
        }

        if (cont)
        {
            osrc = pthread_cond_timedwait(&ptask->cond,
                                          &ptask->mutex,
                                          &wakeTime);
            if (osrc != ETIMEDOUT)
            {
                cont = false;
            }
        }
    }
    while (cont);

    osrc = pthread_mutex_unlock(&ptask->mutex);
    assert(osrc == 0);

    // Stop call frees the memory
    return NULL;
}

void test_task_create( char *name
                     , test_task_fnptr pfn
                     , void *context
                     , uint32_t Rate
                     , void **phandle)
{
    DEBUG_ONLY int osrc;
    test_task_defn_t *ptask = calloc(sizeof(test_task_defn_t), 1);
    assert(ptask != NULL);

    ptask->running = pfn;
    ptask->pfn = pfn;
    ptask->context = context;
    ptask->Rate = Rate;
    ptask->uDelay = (uint64_t)1000000000 / Rate;
    strncpy(ptask->name, name, TASK_NAME_LEN);
    ptask->name[TASK_NAME_LEN]='\0';

    osrc = pthread_mutex_init(&ptask->mutex, NULL);
    assert(osrc == 0);
    osrc = pthread_cond_init(&ptask->cond, NULL);
    assert(osrc == 0);

    osrc = pthread_mutex_lock(&TaskListLock);
    assert(osrc == 0);

    ptask->pNext = TaskList;
    TaskList = ptask;

    osrc = pthread_mutex_unlock(&TaskListLock);
    assert(osrc == 0);

    pthread_attr_t attr;

    osrc = pthread_attr_init(&attr);
    assert(osrc == 0);

    // And start the thread
    osrc = test_task_startThread(&(ptask->hthread),task_worker, ptask,"task_worker");
    assert(osrc == 0);

    osrc = pthread_attr_destroy(&attr);
    assert(osrc == 0);

    *phandle = (void *)ptask;

    // Done
    return;
}

void test_task_stop_internal( void *handle
                            , testLogLevel_t level
                            , uint64_t *pTriggerCount
                            , uint64_t *pMissCount
                            , bool taskListLockHeld )
{
    DEBUG_ONLY int osrc;
    test_task_defn_t *ptask = (test_task_defn_t *)handle;

    while (ptask->running)
    {
        osrc = pthread_mutex_lock(&ptask->mutex);
        assert(osrc == 0);

        ptask->running = false;

        osrc = pthread_cond_signal(&ptask->cond);
        assert(osrc == 0);

        osrc = pthread_mutex_unlock(&ptask->mutex);
        assert(osrc == 0);
    }

    // Wait for the thread to actually end
    osrc = pthread_join(ptask->hthread, NULL);
    assert(osrc == 0);

    *pTriggerCount=ptask->TriggerCount;
    *pMissCount=ptask->MissCount;

    test_log(level, "Task '%s' rate-per-sec(%d) triggerCount(%ld) missCount(%ld)",
             ptask->name, ptask->Rate, ptask->TriggerCount, ptask->MissCount);

    if (taskListLockHeld == false)
    {
        osrc = pthread_mutex_lock(&TaskListLock);
        assert(osrc == 0);
    }

    if (TaskList == ptask)
    {
        TaskList = TaskList->pNext;
    }
    else
    {
        test_task_defn_t *pPrevTask = TaskList;
        while ((pPrevTask->pNext != NULL) && (pPrevTask->pNext != ptask))
            pPrevTask= pPrevTask->pNext;

        if (pPrevTask != NULL && pPrevTask->pNext != NULL)
        {
            pPrevTask->pNext = pPrevTask->pNext->pNext;
        }
    }

    if (taskListLockHeld == false)
    {
        osrc = pthread_mutex_unlock(&TaskListLock);
        assert(osrc == 0);
    }

    free(ptask);

    return;
}

void test_task_stop( void *handle
                   , testLogLevel_t level
                   , uint64_t *pTriggerCount
                   , uint64_t *pMissCount )
{
    test_task_stop_internal(handle, level, pTriggerCount, pMissCount, false);
}

void test_task_stopall( testLogLevel_t level )
{
    uint64_t dummy1, dummy2;

    DEBUG_ONLY int osrc = pthread_mutex_lock(&TaskListLock);
    assert(osrc == 0);

    while (TaskList !=  NULL)
    {
        test_task_stop_internal(TaskList, level, &dummy1, &dummy2, true);
    }

    osrc = pthread_mutex_unlock(&TaskListLock);
    assert(osrc == 0);

    return;
}
