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

//*********************************************************************
/// @file  engineTimers.c
/// @brief Module for Engine timer tasks
//*********************************************************************
#define TRACE_COMP Engine

#include <stdio.h>
#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineTimers.h"
#include "engineInternal.h"
#include "engineStore.h"       // iest_ functions & constants
#include "topicTree.h"         // iett_ functions & constants
#include "remoteServers.h"     // iers_ functions & constants

/*********************************************************************/
/*                                                                   */
/* INTERNAL FUNCTION PROTOTYPES                                      */
/*                                                                   */
/*********************************************************************/
static int ietm_timerThreadLast(ism_timer_t key, ism_time_t timestamp, void * userdata);
static int ietm_timerThreadHighLast(ism_timer_t key, ism_time_t timestamp, void * userdata);


/*********************************************************************/
/*                                                                   */
/* FUNCTION DEFINITIONS                                              */
/*                                                                   */
/*********************************************************************/

/*
 * Set up the Engine's periodic timers
 */
int32_t ietm_setUpTimers(void)
{
    int32_t rc = OK;

    // The creation of the timer tasks counts one towards the use count of
    // Engine initialisation in the timer thread. The task itself actually
    // does the initialisation.
    ismEngine_serverGlobal.ActiveTimerUseCount = 1;

    // If the server timestamp is being updated regularly, we use a low-priority
    // timer task to do this.
    if (ismEngine_serverGlobal.ServerTimestampInterval != 0)
    {
        ismEngine_serverGlobal.ServerTimestampTimer =
                ism_common_setTimerRate(ISM_TIMER_LOW,
                                        ietm_updateServerTimestamp,
                                        NULL,
                                        1,
                                        ismEngine_serverGlobal.ServerTimestampInterval,
                                        TS_SECONDS);
    }

    // If the retained minimum active orderid is being updated create a timer task for it.
    if (ismEngine_serverGlobal.RetMinActiveOrderIdInterval != 0)
    {
        ismEngine_serverGlobal.RetMinActiveOrderIdTimer =
                ism_common_setTimerRate(ISM_TIMER_LOW,
                                        ietm_updateRetMinActiveOrderId,
                                        NULL,
                                        ismEngine_serverGlobal.RetMinActiveOrderIdInterval,
                                        ismEngine_serverGlobal.RetMinActiveOrderIdInterval,
                                        TS_SECONDS);
    }

    if (ismEngine_serverGlobal.ClusterRetainedSyncInterval != 0)
    {
        uint32_t initialSyncDelay = 10;

        if (initialSyncDelay > ismEngine_serverGlobal.ClusterRetainedSyncInterval)
        {
            initialSyncDelay = ismEngine_serverGlobal.ClusterRetainedSyncInterval;
        }

        ismEngine_serverGlobal.ClusterRetainedSyncTimer =
                ism_common_setTimerRate(ISM_TIMER_LOW,
                                        ietm_syncClusterRetained,
                                        NULL,
                                        initialSyncDelay,
                                        ismEngine_serverGlobal.ClusterRetainedSyncInterval,
                                        TS_SECONDS);
    }

    if ((ismEngine_serverGlobal.ServerTimestampTimer == NULL) &&
        (ismEngine_serverGlobal.RetMinActiveOrderIdTimer == NULL) &&
        (ismEngine_serverGlobal.ClusterRetainedSyncTimer == NULL))
    {
        ismEngine_serverGlobal.ActiveTimerUseCount = 0;
    }

    return rc;
}


/*
 * Clean up the Engine's periodic timers
 */
void ietm_cleanUpTimers(void)
{
    int PauseUs = 20000;   // Initial pause is 0.02s
    uint32_t Loop = 0;
    uint32_t cancelled = 0;

    TRACE(ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    // If the server timestamp timer was set, cancel it now
    ism_timer_t timerKey = __sync_lock_test_and_set(&ismEngine_serverGlobal.ServerTimestampTimer, NULL);
    if (timerKey != NULL)
    {
        ism_common_cancelTimer(timerKey);
        cancelled++;
    }

    // If the retained message min active orderid time was set, cancel it now
    timerKey = __sync_lock_test_and_set(&ismEngine_serverGlobal.RetMinActiveOrderIdTimer, NULL);
    if (timerKey != NULL)
    {
        ism_common_cancelTimer(timerKey);
        cancelled++;
    }

    // If cluster retained synchronization was set, cancel it now
    timerKey = __sync_lock_test_and_set(&ismEngine_serverGlobal.ClusterRetainedSyncTimer, NULL);
    if (timerKey != NULL)
    {
        ism_common_cancelTimer(timerKey);
        cancelled++;
    }

    // If there are any one-off timer requests in progress, wait for them to complete
    int EventsPauseUs = PauseUs;
    Loop = 0;
    uint64_t totalPauseUs = 0;

    while(ismEngine_serverGlobal.TimerEventsRequested > 0)
    {
        TRACE(ENGINE_NORMAL_TRACE, "%s: TimerEventsRequested is %lu\n",
              __func__, ismEngine_serverGlobal.TimerEventsRequested);

        // And pause for a short time to allow the timer to end
        ism_common_sleep(EventsPauseUs);
        totalPauseUs += EventsPauseUs;

        if (++Loop > 290) // After 2 minutes
        {
            EventsPauseUs = 5000000; // Upgrade pause to 5 seconds
        }
        else if (Loop > 50) // After 1 Seconds
        {
            EventsPauseUs = 500000; // Upgrade pause to .5 second
        }

        if (totalPauseUs >  ietmMAXIMUM_SHUTDOWN_TIMEOUT_SECONDS*1000000L)
        {
            //We've waited more that 5 mins.. we've hung - take a core for diagnostics
            ieutTRACE_FFDC( ieutPROBE_001, true
                  , "timers(TimerEventsRequested) did not finish within allowed timeout during shutdown.", ISMRC_Error
                  , NULL );
        }
    }

    // If we cancelled any timers we need to wait for them to finish
    if (cancelled > 0)
    {
        // Schedule a one-off task which can terminate the Engine in the timer thread
        // This will also decrement the ActiveTimerUseCount value
        ism_common_setTimerOnce(ISM_TIMER_LOW, ietm_timerThreadLast, NULL, 500);

        // If one of our timers is still running, then wait for it to end
        int TimersPauseUs = PauseUs;
        Loop = 0;
        totalPauseUs = 0;

        while ((volatile uint32_t)ismEngine_serverGlobal.ActiveTimerUseCount > 0)
        {
            TRACE(ENGINE_NORMAL_TRACE, "%s: ActiveTimerUseCount is %d\n",
                  __func__, ismEngine_serverGlobal.ActiveTimerUseCount);

            // And pause for a short time to allow the timer to end
            ism_common_sleep(TimersPauseUs);
            totalPauseUs += TimersPauseUs;

            if (++Loop > 290) // After 2 minutes
            {
                TimersPauseUs = 5000000; // Upgrade pause to 5 seconds
            }
            else if (Loop > 50) // After 1 Seconds
            {
                TimersPauseUs = 500000; // Upgrade pause to .5 second
            }

            if (totalPauseUs > ietmMAXIMUM_SHUTDOWN_TIMEOUT_SECONDS*1000000L)
            {
                //We've waited more that 5 mins.. we've hung - take a core for diagnostics
                ieutTRACE_FFDC( ieutPROBE_002, true
                        , "timers(ActiveTimerUseCount) did not finish within allowed timeout during shutdown.", ISMRC_Error
                        , NULL );
            }
        }
    }

    //Ensure the High Priority Timer thread is engine term'd...
    volatile int32_t termTimerTasksRunning = 1;
    ism_common_setTimerOnce(ISM_TIMER_HIGH, ietm_timerThreadHighLast, (void *)&termTimerTasksRunning, 20);

    int FinalHighTaskPauseUs = PauseUs;
    Loop = 0;
    totalPauseUs = 0;

    while (termTimerTasksRunning > 0)
    {
        TRACE(ENGINE_NORMAL_TRACE, "%s: termTimerTasksRunning is %d\n",
                __func__, termTimerTasksRunning);

        // And pause for a short time to allow the timer to end
        ism_common_sleep(FinalHighTaskPauseUs);
        totalPauseUs += FinalHighTaskPauseUs;

        if (++Loop > 290) // After 2 minutes
        {
            FinalHighTaskPauseUs = 5000000; // Upgrade pause to 5 seconds
        }
        else if (Loop > 50) // After 1 Seconds
        {
            FinalHighTaskPauseUs = 500000; // Upgrade pause to .5 second
        }

        if (totalPauseUs >  ietmMAXIMUM_SHUTDOWN_TIMEOUT_SECONDS*1000000L)
        {
            //We've waited more that 5 mins.. we've hung - take a core for diagnostics
            ieutTRACE_FFDC( ieutPROBE_003, true
                    , "timers(termTimerTasksRunning) did not finish within allowed timeout during shutdown.", ISMRC_Error
                    , NULL );
        }
    }

    TRACE(ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "totalPauseUs=%lu\n", __func__, totalPauseUs);
}

/*
 * Information required to update the server timestamp possibly asynchronously
 */
typedef struct tag_ietmUpdateServerTimestampInfo_t
{
    uint32_t  now;            ///< Timestamp which the store was updated with
    uint32_t  inProgress;     ///< Indication of whether another update is in progress
    uint64_t  asyncId;        ///< ACId used for async requests
}
ietmUpdateServerTimestampInfo_t;

/*
 * Actually update the server timestamp having potentially gone async
 */
void ietm_finishUpdateServerTimestamp(ieutThreadData_t *pThreadData,
                                      int retcode,
                                      ietmUpdateServerTimestampInfo_t *pUpdateInfo)
{
    if (retcode == OK)
    {
        assert(pUpdateInfo->now > ismEngine_serverGlobal.ServerTimestamp);
        ismEngine_serverGlobal.ServerTimestamp = pUpdateInfo->now;
    }

    if (__sync_bool_compare_and_swap(&pUpdateInfo->inProgress, 1, 0) == false)
    {
        ieutTRACEL(pThreadData, pUpdateInfo->inProgress, ENGINE_WORRYING_TRACE,
                   FUNCTION_IDENT "Unexpected inProgress value %u (retcode=%d)\n", __func__,
                   pUpdateInfo->inProgress, retcode);
    }
    else if (retcode != OK)
    {
        ieutTRACEL(pThreadData, retcode, ENGINE_WORRYING_TRACE,
                   FUNCTION_IDENT "Retcode %d\n", __func__, retcode);
    }
}

/*
 * Callback for when updating the server timestamp in the store goes async
 */
void ietm_asyncUpdateServerTimestamp(int retcode, void *pContext)
{
    ietmUpdateServerTimestampInfo_t *pUpdateInfo = (ietmUpdateServerTimestampInfo_t *)pContext;

    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    pThreadData->threadType = AsyncCallbackThreadType;

    ieutTRACEL(pThreadData, pUpdateInfo->asyncId, ENGINE_CEI_TRACE,
               FUNCTION_IDENT "ietmACId=0x%016lx, now=%u\n", __func__,
               pUpdateInfo->asyncId, pUpdateInfo->now);

    ietm_finishUpdateServerTimestamp(pThreadData, retcode, pUpdateInfo);

    ieut_leavingEngine(pThreadData);
}

/*
 * Periodic task to update the server timestamp in the Server Config Record in the Store.
 * This always runs in the low-priority timer thread.
 *
 * Periodically updates the Server Configuration Record in the Store
 * with the current date and time. The idea is that any client-states
 * which are marked as connected when the server restarts must have
 * been connected at the date and time recovered from the SCR.
 */
int ietm_updateServerTimestamp(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    int runagain = 1;
    static ietmUpdateServerTimestampInfo_t updateInfo = {0};

    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Register our use of the Engine's initialisation on this thread
    uint32_t usecount = __sync_fetch_and_add(&ismEngine_serverGlobal.ActiveTimerUseCount, 1);
    if (usecount >= 1)
    {
        // Make sure we're thread-initialised. This can be issued repeatedly.
        ism_engine_threadInit(0);

        // Get the current timestamp
        uint32_t now = ism_common_nowExpire();

        // If time has moved forwards, update the timestamp in the Store.
        // It might go backwards temporarily if someone messes around with
        // the system time, but we side-step this by ensuring the value in
        // the Store always increases.
        if (now > ismEngine_serverGlobal.ServerTimestamp)
        {
            // We are not acting for any specific client
            ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

            // Make sure that this is the only update in progress before making the update
            if (__sync_bool_compare_and_swap(&updateInfo.inProgress, 0, 1) == true)
            {
                // We are now behind the server timestamp value, so just release
                // the inProgress lock which we took.
                if (now <= ismEngine_serverGlobal.ServerTimestamp)
                {
                    DEBUG_ONLY bool swapped;
                    swapped = __sync_bool_compare_and_swap(&updateInfo.inProgress, 1, 0);
                    assert(swapped == true);
                }
                // We can now go ahead and update the timestamp
                else
                {
                    updateInfo.now = now;

                    uint64_t newState = (uint64_t)updateInfo.now << 32;

                    assert(ismEngine_serverGlobal.hStoreSCR != ismSTORE_NULL_HANDLE);

                    // Update the SCR's state to reflect the current time.
                    // We are NOT setting the attribute to be a null handle value.
                    uint32_t rc = ism_store_updateRecord(pThreadData->hStream,
                                                         ismEngine_serverGlobal.hStoreSCR,
                                                         ismSTORE_NULL_HANDLE,
                                                         newState,
                                                         ismSTORE_SET_STATE);
                    if (rc == OK)
                    {
                        updateInfo.asyncId = pThreadData->asyncCounter++;

                        ieutTRACEL(pThreadData, updateInfo.asyncId, ENGINE_CEI_TRACE,
                                   FUNCTION_IDENT "ietmACId=0x%016lx\n", __func__, updateInfo.asyncId);

                        rc = iest_store_asyncCommit(pThreadData,
                                                    false,
                                                    ietm_asyncUpdateServerTimestamp,
                                                    &updateInfo);
                    }
                    else
                    {
                        assert(false); // We really don't expect to fail this.

                        iest_store_rollback(pThreadData, false);
                    }

                    // If we're not finishing asynchronously, it's up to us to finish the update
                    if (rc != ISMRC_AsyncCompletion)
                    {
                        ietm_finishUpdateServerTimestamp(pThreadData, rc, &updateInfo);
                    }
                }
            }
            else
            {
                ieutTRACEL(pThreadData,
                           ismEngine_serverGlobal.ServerTimestamp,
                           ENGINE_WORRYING_TRACE, FUNCTION_IDENT "Server timestamp update already in progress\n", __func__);
            }

            ieut_leavingEngine(pThreadData);
        }

        // Last one out turns off the lights
        usecount = __sync_sub_and_fetch(&ismEngine_serverGlobal.ActiveTimerUseCount, 1);
        if (usecount == 0)
        {
            ism_engine_threadTerm(1);
            runagain = 0;
        }
    }
    else
    {
        // Just tidy up
        __sync_sub_and_fetch(&ismEngine_serverGlobal.ActiveTimerUseCount, 1);
        runagain = 0;
    }

    if (!runagain && __sync_bool_compare_and_swap(&ismEngine_serverGlobal.ServerTimestampTimer, key, NULL))
    {
        ism_common_cancelTimer(key);
    }

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "runagain=%d\n", __func__, runagain);
    return runagain;
}

/*
 * Periodic task to update the minimum active orderid for retained messages in
 * the engine topic tree.
 *
 * This always runs in the low-priority timer thread.
 */
int ietm_updateRetMinActiveOrderId(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    int runagain = 1;
    bool repositionInitiated = false;

    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Register our use of the Engine's initialisation on this thread
    uint32_t usecount = __sync_fetch_and_add(&ismEngine_serverGlobal.ActiveTimerUseCount, 1);
    if (usecount >= 1)
    {
        // Make sure we're thread-initialised. This can be issued repeatedly.
        ism_engine_threadInit(0);

        // We are not acting for any specific client
        ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

        repositionInitiated = iett_scanForRetMinActiveOrderId(pThreadData,
                                                              0,
                                                              ismEngine_serverGlobal.retainedRepositioningEnabled);

        ieut_leavingEngine(pThreadData);

        // Last one out turns off the lights
        usecount = __sync_sub_and_fetch(&ismEngine_serverGlobal.ActiveTimerUseCount, 1);
        if (usecount == 0)
        {
            ism_engine_threadTerm(1);
            runagain = 0;
        }
    }
    else
    {
        // Just tidy up
        __sync_sub_and_fetch(&ismEngine_serverGlobal.ActiveTimerUseCount, 1);
        runagain = 0;
    }

    if (!runagain  && __sync_bool_compare_and_swap(&ismEngine_serverGlobal.RetMinActiveOrderIdTimer, key, NULL))
    {
        ism_common_cancelTimer(key);
    }

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "repositionInitiated=%d runagain=%d\n", __func__,
          (int)repositionInitiated, runagain);
    return runagain;
}

/*
 * Periodic task to check synchronization of clustered retained messages
 *
 * This always runs in the low-priority timer thread.
 */
int ietm_syncClusterRetained(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    int runagain = 1;

    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Register our use of the Engine's initialisation on this thread
    uint32_t usecount = __sync_fetch_and_add(&ismEngine_serverGlobal.ActiveTimerUseCount, 1);
    if (usecount >= 1)
    {
        // Make sure we're thread-initialised. This can be issued repeatedly.
        ism_engine_threadInit(0);

        // We are not acting for any specific client
        ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

        (void)iers_syncClusterRetained(pThreadData);

        ieut_leavingEngine(pThreadData);

        // Last one out turns off the lights
        usecount = __sync_sub_and_fetch(&ismEngine_serverGlobal.ActiveTimerUseCount, 1);
        if (usecount == 0)
        {
            ism_engine_threadTerm(1);
            runagain = 0;
        }
    }
    else
    {
        // Just tidy up
        __sync_sub_and_fetch(&ismEngine_serverGlobal.ActiveTimerUseCount, 1);
        runagain = 0;
    }

    if (!runagain  && __sync_bool_compare_and_swap(&ismEngine_serverGlobal.ClusterRetainedSyncTimer, key, NULL))
    {
        ism_common_cancelTimer(key);
    }

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "runagain=%d\n", __func__, runagain);
    return runagain;
}

/*
 * One-off task which offers a chance to terminate the Engine in the timer thread.
 * This always runs in the low-priority timer thread.
 */
static int ietm_timerThreadLast(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    TRACE(ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Last one out turns off the lights
    uint32_t usecount = __sync_sub_and_fetch(&ismEngine_serverGlobal.ActiveTimerUseCount, 1);
    if (usecount == 0)
    {
        ism_engine_threadTerm(1);
    }
    ism_common_cancelTimer(key);

    TRACE(ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return 0;
}

static int ietm_timerThreadHighLast(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    TRACE(ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    volatile int32_t *pTermTimerTasksRunning = (volatile int32_t *)userdata;

    ism_engine_threadTerm(1);
    ism_common_cancelTimer(key);

    __sync_fetch_and_sub(pTermTimerTasksRunning, 1);

    TRACE(ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return 0;
}

/*********************************************************************/
/*                                                                   */
/* End of engineTimers.c                                             */
/*                                                                   */
/*********************************************************************/
