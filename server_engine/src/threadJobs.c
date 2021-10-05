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
/// @file  threadJobs.c
/// @brief Functions for scavenging jobs on per-thread job queues
//****************************************************************************
#define TRACE_COMP Engine

#include <sys/time.h>

#include "engineInternal.h"
#include "engineUtils.h"
#include "memHandler.h"
#include "threadJobs.h"

void *ietj_scavengerThread(void *arg, void * context, int value);

//****************************************************************************
/// @brief Process the specified job queue
///
/// @param[in]     pThreadData      Thread data making the request
/// @param[in]     jobQueue         The queue of jobs to be processed (may not be
///                                 the queue for the requesting thread).
/// @param[in]     queueOwner       Is this being called by the owner of the queue?
///                                   (If the owner can't get the lock, we ask the scavenger not to check so often)
/// @param[in]     mustDo           Whether we must do the work
/// @param[in]     maxJobs          Max jobs we'll process before retr
///
/// @return Whether any jobs were found
//****************************************************************************
static bool ietj_processJobQueue_internal(ieutThreadData_t *pThreadData,
                                          iejqJobQueueHandle_t jobQueue,
                                          bool queueOwner,
                                          bool mustDo,
                                          uint64_t maxJobs)
{
    uint32_t callsMade = 0;


    if (queueOwner)
    {
        bool tookLock = iejq_tryTakeGetLock(pThreadData, jobQueue);

        if (!tookLock)
        {
            //Record someone else (Scavenger) had the lock on OUR queue so they give us more time to
            //do the work ourselves
            iejq_recordOwnerBlocked(pThreadData, jobQueue);

            if (mustDo)
            {
                iejq_takeGetLock(pThreadData, jobQueue);
            }
            else
            {
                goto mod_exit;
            }
        }
    }
    else
    {
        if (mustDo)
        {
            iejq_takeGetLock(pThreadData, jobQueue);
        }
        else
        {
            bool tookLock = iejq_tryTakeGetLock(pThreadData, jobQueue);

            if (!tookLock) goto mod_exit;
        }
    }

    ietjCallback_t callbackFunction;
    void *callbackArgs;

    uint8_t prevTrcLevel = pThreadData->componentTrcLevel;

    while(iejq_getJob(pThreadData,
                      jobQueue,
                      (void **)&callbackFunction,
                      (void **)&callbackArgs,
                      false) == OK)
    {
        callbackFunction(pThreadData, callbackArgs);
        callsMade++;

        if (    (maxJobs != 0)
             && (callsMade >= maxJobs))
        {
            break;
        }
    }

    pThreadData->componentTrcLevel = prevTrcLevel;

    iejq_releaseGetLock(pThreadData, jobQueue);

mod_exit:

    return (callsMade != 0);
}

//****************************************************************************
/// @brief Process the job queue for a given thread
///
/// @param[in]     pThreadData      Thread data whose job queue to request
///
/// @return Whether any jobs were found
//****************************************************************************
bool ietj_processJobQueue(ieutThreadData_t *pThreadData)
{
    return ietj_processJobQueue_internal( pThreadData
                                        , pThreadData->jobQueue
                                        , true //external callers only check their own job queue
                                        , true //...and they want to wait to get the lock
                                        , 0 );   //...and no limit on how many jobs to do
}

//****************************************************************************
/// @brief Create and initialize the structures needed to perform the scavenging
///        of unfinished jobs on per-thread job queues.
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ietj_initThreadJobs( ieutThreadData_t *pThreadData )
{
    int32_t rc = OK;
    ietjThreadJobControl_t *threadJobControl = ismEngine_serverGlobal.threadJobControl;

    ieutTRACEL(pThreadData, threadJobControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (threadJobControl != NULL) goto mod_exit;

    threadJobControl = iemem_calloc(pThreadData,
                                    IEMEM_PROBE(iemem_jobQueues, 3),
                                    1, sizeof(ietjThreadJobControl_t));

    if (threadJobControl == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    int os_rc = pthread_mutex_init(&(threadJobControl->scavengerListLock), NULL);

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "pthread_mutex_init failed!", ISMRC_Error
                      , "threadJobControl", threadJobControl, sizeof(*threadJobControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

mod_exit:

    ismEngine_serverGlobal.threadJobControl = threadJobControl;

    if (rc != OK)
    {
        ietj_destroyThreadJobs(pThreadData);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Start the per-thread job queue scavenger
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ietj_startThreadJobScavenger( ieutThreadData_t *pThreadData )
{
    int32_t rc = OK;

    ietjThreadJobControl_t *threadJobControl = ismEngine_serverGlobal.threadJobControl;

    ieutTRACEL(pThreadData, threadJobControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (threadJobControl != NULL)
    {
        assert(threadJobControl->scavengerThreadHandle == 0);

        int startRc = ism_common_startThread(&threadJobControl->scavengerThreadHandle,
                                             ietj_scavengerThread,
                                             NULL, threadJobControl, 0,
                                             ISM_TUSAGE_NORMAL,
                                             0,
                                             "jobScavenger",
                                             "Scavenge_Inactive_Thread_Jobs");

        if (startRc != 0)
        {
            ieutTRACEL(pThreadData, startRc, ENGINE_ERROR_TRACE, "ism_common_startThread for jobScavenger failed with %d\n", startRc);
            rc = ISMRC_Error;
            ism_common_setError(rc);
        }
        else
        {
            assert(threadJobControl->scavengerThreadHandle != 0);
        }
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Stop the per-thread job queue scavenger
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void ietj_stopThreadJobScavenger( ieutThreadData_t *pThreadData )
{
    ietjThreadJobControl_t *threadJobControl = ismEngine_serverGlobal.threadJobControl;

    ieutTRACEL(pThreadData, threadJobControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (threadJobControl != NULL && threadJobControl->scavengerThreadHandle != 0)
    {
        void *retVal = NULL;

        // Request the scavenger thread to end, and wait for it to do so
        threadJobControl->scavengerEndRequested = true;

        // Wait for the thread to actually end
        ieut_waitForThread(pThreadData,
                           threadJobControl->scavengerThreadHandle,
                           &retVal,
                           ietjMAXIMUM_SHUTDOWN_TIMEOUT_SECONDS);

        // Don't expect anything to be returned
        assert(retVal == NULL);

        threadJobControl->scavengerThreadHandle = 0;
    }

    ieutTRACEL(pThreadData, threadJobControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief End the per-thread job queue scavenger and clean up.
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void ietj_destroyThreadJobs( ieutThreadData_t *pThreadData )
{
    ietjThreadJobControl_t *threadJobControl = ismEngine_serverGlobal.threadJobControl;

    ieutTRACEL(pThreadData, threadJobControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (threadJobControl != NULL)
    {
        assert(threadJobControl->scavengerThreadHandle == 0);

        int os_rc = pthread_mutex_destroy(&(threadJobControl->scavengerListLock));

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_001, true, "mutex_destroy!", ISMRC_Error
                          , "threadJobControl", threadJobControl, sizeof(*threadJobControl)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }

        uint32_t i;
        for(i=0; i<threadJobControl->scavengerListCount; i++)
        {
            ietjScavengerEntry *listEntry = &threadJobControl->scavengerList[i];

            ieutTRACEL(pThreadData, listEntry->scavengedCount, ENGINE_SHUTDOWN_DIAG_TRACE,
                       "Destroying JobQueue for thread %p, scavengedCount=%lu, (last)ProcessedJobs=%lu.\n",
                       pThreadData, listEntry->scavengedCount, listEntry->lastProcessedJobs);

            // Don't expect anything to be left on the job queue???
            #if 0
            iejq_takeGetLock(pThreadData, listEntry->jobQueue);
            void *dbg_func, *dbg_args;
            assert (iejq_getJob(pThreadData, listEntry->jobQueue, &dbg_func, &dbg_args, false) != OK);
            iejq_releaseGetLock(pThreadData, listEntry->jobQueue);
            #endif

            iejq_freeJobQueue(pThreadData, listEntry->jobQueue);
        }

        iemem_free(pThreadData, iemem_jobQueues, threadJobControl->scavengerList);
        iemem_free(pThreadData, iemem_jobQueues, threadJobControl);

        ismEngine_serverGlobal.threadJobControl = NULL;
    }

    ieutTRACEL(pThreadData, threadJobControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Add a jobQueue to the thread, and add it to the scavenger list.
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
int32_t ietj_addThreadJobQueue( ieutThreadData_t *pThreadData )
{
    int32_t rc = OK;

    ietjThreadJobControl_t *threadJobControl = ismEngine_serverGlobal.threadJobControl;

    ieutTRACEL(pThreadData, pThreadData, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    iejqJobQueueHandle_t newJobQueue = NULL;

    if (threadJobControl != NULL && pThreadData->jobQueue == NULL)
    {
        rc = iejq_createJobQueue(pThreadData, &newJobQueue);

        if (rc == OK)
        {
            ismEngine_lockMutex(&(threadJobControl->scavengerListLock));

            if (threadJobControl->scavengerListCount == threadJobControl->scavengerListCapacity)
            {
                uint32_t newCapacity = threadJobControl->scavengerListCapacity + 100;
                ietjScavengerEntry *newScavengerList = iemem_realloc(pThreadData,
                                                                     IEMEM_PROBE(iemem_jobQueues, 2),
                                                                     threadJobControl->scavengerList,
                                                                     sizeof(ietjScavengerEntry) * newCapacity);

                if (newScavengerList == NULL)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                }
                else
                {
                    threadJobControl->scavengerList = newScavengerList;
                    threadJobControl->scavengerListCapacity = newCapacity;
                }
            }

            if (rc == OK)
            {
                ietjScavengerEntry *scavengerEntry = &threadJobControl->scavengerList[threadJobControl->scavengerListCount++];

                memset(scavengerEntry, 0, sizeof(*scavengerEntry));
                scavengerEntry->pThreadData = pThreadData;
                scavengerEntry->jobQueue = newJobQueue;
                scavengerEntry->scavengerWaitDelay = ietjSCAVENGER_THREAD_JOB_QUEUE_MIN_IDLE_NANOS;
                pThreadData->jobQueue = newJobQueue;
            }

            ismEngine_unlockMutex(&(threadJobControl->scavengerListLock));

            if (rc != OK && newJobQueue != NULL)
            {
                iejq_freeJobQueue(pThreadData, newJobQueue);
                newJobQueue = NULL;
            }
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_ENTRY "rc=%d newJobQueue=%p\n",
               __func__, rc, newJobQueue);

    return rc;
}

//****************************************************************************
/// @brief Request the removal of the thread's job queue, the actual removal
/// will happen when it can safely be removed.
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void ietj_removeThreadJobQueue( ieutThreadData_t *pThreadData )
{
    bool removalRequested = false;

    ietjThreadJobControl_t *threadJobControl = ismEngine_serverGlobal.threadJobControl;

    ieutTRACEL(pThreadData, pThreadData, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (threadJobControl != NULL)
    {
        ismEngine_lockMutex(&(threadJobControl->scavengerListLock));

        uint32_t i;
        for(i=0; i<threadJobControl->scavengerListCount; i++)
        {
            ietjScavengerEntry *listEntry = &threadJobControl->scavengerList[i];

            if (listEntry->pThreadData == pThreadData &&
                listEntry->removalRequested == false)
            {
                listEntry->removalRequested = true;
                ieutTRACEL(pThreadData, listEntry->scavengedCount, ENGINE_INTERESTING_TRACE,
                           "Removing thread %p. scavengedCount=%lu, processedJobs=%lu.\n",
                           pThreadData, listEntry->scavengedCount, listEntry->pThreadData->processedJobs);
                removalRequested = true;
                break;
            }
        }

        ismEngine_unlockMutex(&(threadJobControl->scavengerListLock));
    }

    ieutTRACEL(pThreadData, removalRequested, ENGINE_FNC_TRACE, FUNCTION_EXIT "removalRequested=%d\n",
               __func__, (int)removalRequested);

    return;
}

//****************************************************************************
/// @brief The actual scavenger thread
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void *ietj_scavengerThread(void *arg, void *context, int value)
{
    char threadName[16];
    ism_common_getThreadName(threadName, sizeof(threadName));

    ietjThreadJobControl_t *threadJobControl = (ietjThreadJobControl_t *)context;

    // Make sure we're thread-initialised
    ism_engine_threadInit(1);

    // Not working on behalf of a particular client
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    ieutTRACEL(pThreadData, threadJobControl, ENGINE_CEI_TRACE, FUNCTION_ENTRY "Started thread %s with control %p\n",
               __func__, threadName, threadJobControl);

    iejqJobQueueHandle_t scavengeQueue[100];
    bool scavengeRemoval[sizeof(scavengeQueue)/sizeof(scavengeQueue[0])];

    uint64_t scavengerDidJobsTime = 0;
    uint64_t engineCallsTime      = 0;

    while(threadJobControl->scavengerEndRequested == false)
    {
        int32_t scavengeCount = 0;
        uint64_t timeUntilNextScavangerActionNanos = ietjSCAVENGER_THREAD_JOB_QUEUE_MAX_IDLE_NANOS;
        uint64_t now = ism_common_currentTimeNanos();

        ismEngine_lockMutex(&(threadJobControl->scavengerListLock));

        for(int32_t i=0; i<(int32_t)threadJobControl->scavengerListCount &&
                         scavengeCount < sizeof(scavengeQueue)/sizeof(scavengeQueue[0]); i++)
        {
            ietjScavengerEntry *listEntry = &threadJobControl->scavengerList[i];
            ieutThreadData_t *pWatchedThreadData = listEntry->pThreadData;

            // Once removal has been requested, we cannot touch the thread data, so just
            // process anything left in the queue and free the queue.
            if (listEntry->removalRequested)
            {
                scavengeRemoval[scavengeCount] = true;
                scavengeQueue[scavengeCount++] = listEntry->jobQueue;

                // Move the last entry into this position
                threadJobControl->scavengerListCount--;
                *listEntry = threadJobControl->scavengerList[threadJobControl->scavengerListCount];
                i--;
            }
            else if (pWatchedThreadData != pThreadData)
            {
                // They've been in since the last time we looked at them, so will have processed
                // their own jobs (or possibly be in the midst of doing so).
                if (pWatchedThreadData->entryCount != listEntry->lastEntryCount)
                {
                    listEntry->lastEntryCount = pWatchedThreadData->entryCount;
                    listEntry->lastProcessedJobs = pWatchedThreadData->processedJobs;
                    listEntry->lastUpdated = now;

                    engineCallsTime = now;
                }
                // They haven't been in and we have waited long enough...
                else if (ismEngine_serverGlobal.ThreadJobAlgorithm == ismENGINE_THREAD_JOB_QUEUES_ALGORITHM_EXTRA
                          && (now-listEntry->lastUpdated > listEntry->scavengerWaitDelay))
                {
                    bool drainQueue=true;

                    //First let's consider whether the scavenger is jumping in too fast and getting in the way of the thread owner
                    //waiting too long (as no-one did the work even those the scavenger waited ages so it ought to do it quicker)
                    if (iejq_ownerBlocked(listEntry->jobQueue, true))
                    {
                        //The scavenger got in the way!
                        listEntry->lastOwnerBlockedTime = now;
                        listEntry->scavengerWaitDelay += IETJ_SCAVENGERWAITTIME_INCREMENT_NANOS;

                        if (listEntry->scavengerWaitDelay > ietjSCAVENGER_THREAD_JOB_QUEUE_MAX_IDLE_NANOS)
                        {
                            listEntry->scavengerWaitDelay = ietjSCAVENGER_THREAD_JOB_QUEUE_MAX_IDLE_NANOS;
                        }
                        else
                        {
                            //We've increased the time we should be waiting so don't do it yet
                            drainQueue = false;
                        }

                    }
                    else
                    {
                        //Thread owner didn't come back and collect the work and we're doing it.
                        //...maybe we should collect it quicker in future?
                        if ((now-listEntry->lastOwnerBlockedTime) > ietjSCAVENGER_THREAD_JOB_FORGET_OWNER_BLOCK_TIME)
                        {
                            listEntry->scavengerWaitDelay -= IETJ_SCAVENGERWAITTIME_INCREMENT_NANOS;

                            if (listEntry->scavengerWaitDelay < ietjSCAVENGER_THREAD_JOB_QUEUE_MIN_IDLE_NANOS)
                            {
                                listEntry->scavengerWaitDelay = ietjSCAVENGER_THREAD_JOB_QUEUE_MIN_IDLE_NANOS;
                            }

                            //Fake an owner blocked event so we don't immediately reduce it again.
                            listEntry->lastOwnerBlockedTime = now;
                        }
                    }

                    if (drainQueue)
                    {
                        scavengeRemoval[scavengeCount] = false;
                        scavengeQueue[scavengeCount++] = listEntry->jobQueue;

                        listEntry->lastUpdated = now;
                        listEntry->scavengedCount++;
                    }
                }
                else if (    ismEngine_serverGlobal.ThreadJobAlgorithm != ismENGINE_THREAD_JOB_QUEUES_ALGORITHM_EXTRA
                         && (now-listEntry->lastUpdated > ietjSCAVENGER_THREAD_JOB_QUEUE_IDLE_NANOS))
                {
                    scavengeRemoval[scavengeCount] = false;
                    scavengeQueue[scavengeCount++] = listEntry->jobQueue;

                    listEntry->lastUpdated = now;
                    listEntry->scavengedCount++;
                }

                //Work out how long until we need to check this job queue again
                uint64_t waitTilAction = 1 + listEntry->scavengerWaitDelay - (now - listEntry->lastUpdated);

                if (waitTilAction < timeUntilNextScavangerActionNanos)
                {
                    timeUntilNextScavangerActionNanos = waitTilAction;
                }
            }
        }

        ismEngine_unlockMutex(&(threadJobControl->scavengerListLock));

        // Process our own job queue first.
        if (pThreadData->jobQueue != NULL)
        {
            ietj_processJobQueue(pThreadData);
        }

        // Process any other job queues that we found
        for(int32_t i=0; i<scavengeCount; i++)
        {
            bool foundSomeWork;

            if (scavengeRemoval[i])
            {
                foundSomeWork = ietj_processJobQueue_internal( pThreadData
                                                             , scavengeQueue[i]
                                                             , false //Not the owner
                                                             , true  //wait on the lock
                                                             , 0 );  //do all the jobs
                iejq_freeJobQueue(pThreadData, scavengeQueue[i]);
            }
            else
            {
                uint64_t numJobs = 0;
                
                if (ismEngine_serverGlobal.ThreadJobAlgorithm == ismENGINE_THREAD_JOB_QUEUES_ALGORITHM_EXTRA)
                {
                   numJobs = ietjSCAVENGER_THREAD_JOB_BATCH_SIZE; //We only do a few jobs, we don't want to block the thread owner
                                                                  //If they are waiting
                }
                foundSomeWork = ietj_processJobQueue_internal( pThreadData
                                                             , scavengeQueue[i]
                                                             , false //Not the owner
                                                             , false //don't wait on the lock
                                                             , numJobs); //Do a few of the jobs
            }

            if (foundSomeWork)
            {
                scavengerDidJobsTime = now;
            }
        }

        // We know we are no longer referring to engine structures...
        ieut_virtuallyLeaveEngine(pThreadData);

        // If we didn't have a full list, sleep to allow threads to pick up their own work.
        if (scavengeCount < sizeof(scavengeQueue)/sizeof(scavengeQueue[0]))
        {
            if (ismEngine_serverGlobal.ThreadJobAlgorithm == ismENGINE_THREAD_JOB_QUEUES_ALGORITHM_EXTRA)
            {
                uint64_t inactivityThreshold = now - ietjSCAVENGER_THREAD_INACTIVITY_THRESHOLD_NANOS;

                if (scavengerDidJobsTime < inactivityThreshold && engineCallsTime < inactivityThreshold)
                {
                    ism_common_sleep(ietjSCAVENGER_THREAD_INACTIVITY_LONG_SLEEP/1000);
                }
                else
                {
                    ism_common_sleep(timeUntilNextScavangerActionNanos/1000);
                }
            }
            else
            {
                ism_common_sleep(ietjSCAVENGER_THREAD_LOOP_DELAY_NANOS/1000);
            }
        }

        // We might start referring to engine structures again...
        ieut_virtuallyEnterEngine(pThreadData);
    }

    ieutTRACEL(pThreadData, threadJobControl, ENGINE_CEI_TRACE, FUNCTION_EXIT "Ending thread %s with control %p\n",
               __func__, threadName, threadJobControl);
    ieut_leavingEngine(pThreadData);

    // No longer need the thread to be initialized
    ism_engine_threadTerm(1);

    return NULL;
}
