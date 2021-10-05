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

//****************************************************************************
/// @file  jobQueue.c
/// @brief Code for queues of work being queued between threads
//****************************************************************************
#define TRACE_COMP Engine

#include "engineInternal.h"
#include "memHandler.h"
#include "jobQueue.h"

typedef struct tag_iejqJob_t {
    void *func;
    void *args;
} iejqJob_t;

void *iejqNullJob =  NULL;
void *iejqJobSeperator = (void *)0x1; //0x01 = illegal value for func ptr

typedef struct tag_iejqJobQueue_t {
#ifdef IEJQ_JOBQUEUE_PUTLOCK
    pthread_spinlock_t putLock;
#endif
    uint64_t  putPos;
    volatile iejqJob_t jobArr[IEJQ_JOB_MAX];
    pthread_spinlock_t getLock; //Separate in memory from putlock to avoud cacheline contention
    uint64_t  getPos;
    bool ownerBlocked; //Has the self declared owner complained they couldn't get the get lock
} iejqJobQueue_t;

#ifdef IEJQ_JOBQUEUE_PUTLOCK
static inline void iejq_takePutLockInternal(ieutThreadData_t *pThreadData, iejqJobQueue_t *jq)
{
    int os_rc = pthread_spin_lock(&(jq->putLock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                , "failed taking put lock.", os_rc
                , "JQ", jq, sizeof(iejqJobQueue_t)
                , NULL );
    }
}
static inline void iejq_releasePutLockInternal(ieutThreadData_t *pThreadData, iejqJobQueue_t *jq)
{
    int os_rc = pthread_spin_unlock(&(jq->putLock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                , "failed releasing put lock.", os_rc
                , "JQ", jq, sizeof(iejqJobQueue_t)
                , NULL );
    }
}
void iejq_takePutLock(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh)
{
    iejqJobQueue_t *jq = (iejqJobQueue_t *)jqh;
    return iejq_takePutLockInternal(pThreadData, jq);
}
void iejq_releasePutLock(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh)
{
    iejqJobQueue_t *jq = (iejqJobQueue_t *)jqh;
    return iejq_releasePutLockInternal(pThreadData, jq);
}
#endif

int32_t iejq_addJob( ieutThreadData_t *pThreadData
                   , iejqJobQueueHandle_t jqh
                   , void *func
                   , void *args
#ifdef IEJQ_JOBQUEUE_PUTLOCK
                   , bool takeLock)
#else
                   )
#endif
{
    int32_t rc = ISMRC_OK;
    iejqJobQueue_t *jq = (iejqJobQueue_t *)jqh;

#ifdef IEJQ_JOBQUEUE_PUTLOCK
    if (takeLock)
    {
        iejq_takePutLock(pThreadData, jq);
    }
#endif
    uint64_t curPos = jq->putPos;

    //printf("put - curPos: %lu, func: %lu, args %lu, nulljob %lu\n",
    //          curPos, (uint64_t)(jq->jobArr[curPos].func),
    //                  (uint64_t)(jq->jobArr[curPos].args),
    //                  (uint64_t)iejqNullJob);

    if(jq->jobArr[curPos].func == iejqNullJob)
    {
       jq->jobArr[curPos].args = args; //Do args before func,  have write barrier between then on non x86-64
       ismEngine_WriteMemoryBarrier();
       jq->jobArr[curPos].func       = func;

       if (jq->putPos < (IEJQ_JOB_MAX-1))
       {
           jq->putPos++;
       }
       else
       {
           jq->putPos = 0;
       }
    }
    else
    {
        rc = ISMRC_DestinationFull;
    }

#ifdef IEJQ_JOBQUEUE_PUTLOCK
    if(takeLock)
    {
        iejq_releasePutLock(pThreadData, jq);
    }
#endif

    return rc;
}
static inline void iejq_takeGetLockInternal(ieutThreadData_t *pThreadData, iejqJobQueue_t *jq)
{
    int os_rc = pthread_spin_lock(&(jq->getLock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                , "failed taking get lock.", os_rc
                , "JQ", jq, sizeof(iejqJobQueue_t)
                , NULL );
    }
}

static inline void iejq_releaseGetLockInternal(ieutThreadData_t *pThreadData, iejqJobQueue_t *jq)
{
    int os_rc = pthread_spin_unlock(&(jq->getLock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                , "failed releasing get lock.", os_rc
                , "JQ", jq, sizeof(iejqJobQueue_t)
                , NULL );
    }
}

bool iejq_tryTakeGetLock(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh)
{
    bool gotLock = true;
    iejqJobQueue_t *jq = (iejqJobQueue_t *)jqh;

    int os_rc = pthread_spin_trylock(&(jq->getLock));

    if (os_rc == EBUSY)
    {
        //Not an error - someone has the lock
        gotLock = false;
    }
    else if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                , "failed trying to get get lock.", os_rc
                , "JQ", jq, sizeof(iejqJobQueue_t)
                , NULL );
    }

    return gotLock;
}

void iejq_takeGetLock(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh)
{
    iejqJobQueue_t *jq = (iejqJobQueue_t *)jqh;
    return iejq_takeGetLockInternal(pThreadData, jq);
}

void iejq_releaseGetLock(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh)
{
    iejqJobQueue_t *jq = (iejqJobQueue_t *)jqh;
    return iejq_releaseGetLockInternal(pThreadData, jq);
}

int32_t iejq_getJob( ieutThreadData_t *pThreadData
                   , iejqJobQueueHandle_t jqh
                   , void **pFunc
                   , void **pArgs
                   , bool takeLock)
{
    int32_t rc = ISMRC_NoMsgAvail;
    iejqJobQueue_t *jq = (iejqJobQueue_t *)jqh;

    if (takeLock)
    {
        iejq_takeGetLock(pThreadData, jq);
    }

    uint64_t curPos = jq->getPos;

    //printf("get - curPos: %lu, func: %lu, args %lu, nulljob %lu\n",
    //         curPos, (uint64_t)(jq->jobArr[curPos].func),
    //                 (uint64_t)(jq->jobArr[curPos].args),
    //                (uint64_t)iejqNullJob);

    void *func = jq->jobArr[curPos].func;

    if(func != iejqNullJob) {
       *pFunc = func;
       ismEngine_ReadMemoryBarrier();
       *pArgs = jq->jobArr[curPos].args;
       rc = ISMRC_OK;

       //Work out current separator pos and update get pos;
       uint64_t curSeparator;

       if (curPos == 0)
       {
           curSeparator = IEJQ_JOB_MAX - 1;
           jq->getPos   = 1;
       }
       else if (curPos == (IEJQ_JOB_MAX-1))
       {
           curSeparator = curPos - 1;
           jq->getPos   = 0;
       }
       else
       {
           curSeparator = curPos - 1;
           jq->getPos +=1;
       }
       //Set the slot we got from to the separator and clear the existing sepator
       jq->jobArr[curPos].func       = iejqJobSeperator;
       jq->jobArr[curSeparator].func = iejqNullJob;
    }

    if(takeLock)
    {
        iejq_releaseGetLock(pThreadData, jq);
    }

    return rc;
}



int32_t iejq_createJobQueue(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t *pJQH)
{
    int32_t rc = OK;
    iejqJobQueue_t *jq = (iejqJobQueue_t *)iemem_calloc( pThreadData
                                                       , IEMEM_PROBE(iemem_jobQueues, 1)
                                                       , 1, sizeof(iejqJobQueue_t));

    if (jq != NULL)
    {
        #ifdef IEJQ_JOBQUEUE_PUTLOCK
        int os_rc = pthread_spin_init(&(jq->putLock), PTHREAD_PROCESS_PRIVATE);

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_001, true
                    , "failed creating put lock.", os_rc
                    , "JQ", jq, sizeof(iejqJobQueue_t)
                    , NULL );
        }
        #else
        int os_rc;
        #endif

        os_rc = pthread_spin_init(&(jq->getLock), PTHREAD_PROCESS_PRIVATE);

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_001, true
                    , "failed get put lock.", os_rc
                    , "JQ", jq, sizeof(iejqJobQueue_t)
                    , NULL );
        }

        //Set the 0th position in the array to a "separator" that stops the putter
        //looping around and overwriting entries that haven't been got
        jq->jobArr[0].func = iejqJobSeperator;

        //Positions for both put and get set to 1
        jq->putPos = 1;
        jq->getPos = 1;
        jq->ownerBlocked = false;

        *pJQH = jq;
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }
    return rc;
}

void iejq_freeJobQueue(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh)
{
    iejqJobQueue_t *jq = (iejqJobQueue_t *)jqh;

    #ifdef IEJQ_JOBQUEUE_PUTLOCK
    int32_t os_rc = pthread_spin_destroy(&(jq->putLock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                , "failed destroying put lock.", os_rc
                , "JQ", jq, sizeof(iejqJobQueue_t)
                , NULL );
    }
    #else
    int32_t os_rc;
    #endif

    os_rc = pthread_spin_destroy(&(jq->getLock));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                , "failed destroying put lock.", os_rc
                , "JQ", jq, sizeof(iejqJobQueue_t)
                , NULL );
    }

    iemem_free(pThreadData, iemem_jobQueues, jq);
}

bool iejq_ownerBlocked(iejqJobQueueHandle_t jqh, bool resetBlockedFlag)
{
    iejqJobQueue_t *jq = (iejqJobQueue_t *)jqh;

    bool wasOwnerBlocked = jq->ownerBlocked;

    if(wasOwnerBlocked && resetBlockedFlag)
    {
        jq->ownerBlocked = false;
    }

    return wasOwnerBlocked;
}

void iejq_recordOwnerBlocked(ieutThreadData_t *pThreadData, iejqJobQueueHandle_t jqh)
{
    iejqJobQueue_t *jq = (iejqJobQueue_t *)jqh;

    jq->ownerBlocked = true;
}

