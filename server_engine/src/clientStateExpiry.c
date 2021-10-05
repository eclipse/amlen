/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
/// @file  clientStateExpiry.c
/// @brief Functions for tracking and removal of clientStates and publishing
///        of delayed will messages.
//****************************************************************************
#define TRACE_COMP Engine

#include <sys/time.h>

#include "engineInternal.h"
#include "engineUtils.h"
#include "clientState.h"            // iecs_ functions and constants
#include "clientStateInternal.h"
#include "clientStateExpiry.h"
#include "clientStateExpiryInternal.h"
#include "memHandler.h"

void *iece_reaperThread(void *arg, void * context, int value);

//****************************************************************************
/// @brief Setup the locks/conds for waking up the clientState expiry reaper
///        thread(s)
///
/// @param[in]     pThreadData    Thread data to use
/// @param[in]     expiryControl  Expiry Control information
///
/// @return void
//****************************************************************************
static inline void iece_initExpiryReaperWakeupMechanism( ieutThreadData_t *pThreadData
                                                       , ieceExpiryControl_t *expiryControl)
{
    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);
    assert (expiryControl != NULL);

    pthread_condattr_t attr;

    int os_rc = pthread_condattr_init(&attr);

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "pthread_condattr_init failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_002, true, "pthread_condattr_setclock failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = pthread_cond_init(&(expiryControl->cond_wakeup), &attr);

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_003, true, "pthread_cond_init failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = pthread_condattr_destroy(&attr);


    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_004, true, "pthread_condattr_destroy failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = pthread_mutex_init(&(expiryControl->mutex_wakeup), NULL);

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_005, true, "pthread_mutex_init failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    // logic relies on this being the largest time possible
    assert(ieceNO_TIMED_SCAN_SCHEDULED == UINT64_MAX);

    expiryControl->nextScheduledScan = ieceNO_TIMED_SCAN_SCHEDULED;

    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

static inline void iece_lockExpiryWakeupMutex(ieceExpiryControl_t *expiryControl)
{
    ismEngine_lockMutex(&(expiryControl->mutex_wakeup));
}

static inline void iece_unlockExpiryWakeupMutex(ieceExpiryControl_t *expiryControl)
{
    ismEngine_unlockMutex(&(expiryControl->mutex_wakeup));
}

//****************************************************************************
/// @brief Put the clientState Expiry reaper to sleep using the lowestTimeSeen
///        value and when the next scheduled scan is set for.
///
/// @param[in]     pThreadData       Thread data to use
/// @param[in]     lowestTimeSeen    The lowest time we know about
/// @param[inout]  numWakeups        On input: number of wakeups last time this
///                                            function was called
///                                  On output: current number of wakeups
///
/// @return void
//****************************************************************************
void iece_expiryReaperSleep( ieutThreadData_t *pThreadData
                           , ism_time_t lowestTimeSeen
                           , uint64_t *numWakeups)
{
    ieutTRACEL(pThreadData, lowestTimeSeen, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "lowestTimeSeen: %lu [%s] wakeups: %lu\n",
               __func__, lowestTimeSeen, (lowestTimeSeen == ieceNO_TIMED_SCAN_SCHEDULED) ? "NoTimedScan" : "Scan", *numWakeups);

    ieceExpiryControl_t *expiryControl = ismEngine_serverGlobal.clientStateExpiryControl;
    assert (expiryControl != NULL);

    iece_lockExpiryWakeupMutex(expiryControl);

    if (lowestTimeSeen < expiryControl->nextScheduledScan)
    {
        expiryControl->nextScheduledScan = lowestTimeSeen;
    }

    ism_time_t nowExpiryTime = ism_common_convertExpireToTime(ism_common_nowExpire());

    while((expiryControl->nextScheduledScan > nowExpiryTime) &&
          (*numWakeups == expiryControl->numWakeups) &&
          (expiryControl->reaperEndRequested == false))
    {
        int os_rc;

        if (expiryControl->nextScheduledScan == ieceNO_TIMED_SCAN_SCHEDULED)
        {
            os_rc = pthread_cond_wait(&expiryControl->cond_wakeup,
                                      &expiryControl->mutex_wakeup);
        }
        else
        {
            uint64_t deltaSecs = (expiryControl->nextScheduledScan - nowExpiryTime) / 1000000000UL;

            if (deltaSecs == 0) break;

            struct timespec waituntil;
            clock_gettime(CLOCK_MONOTONIC, &waituntil);

            waituntil.tv_sec += (__time_t)deltaSecs;

            os_rc = pthread_cond_timedwait(&(expiryControl->cond_wakeup)
                                          , &(expiryControl->mutex_wakeup)
                                          , &waituntil);

            if (os_rc == ETIMEDOUT) break;
        }

        assert(os_rc == OK);
        nowExpiryTime = ism_common_convertExpireToTime(ism_common_nowExpire());
    }

    expiryControl->nextScheduledScan = ieceNO_TIMED_SCAN_SCHEDULED;

    *numWakeups = expiryControl->numWakeups;

    iece_unlockExpiryWakeupMutex(expiryControl);

    ieutTRACEL(pThreadData, expiryControl->numWakeups, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

//****************************************************************************
/// @brief Cause the expiry reaper to start a scan of clientStates
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return void
//****************************************************************************
void iece_wakeClientStateExpiryReaper(ieutThreadData_t *pThreadData)
{
    ieceExpiryControl_t *expiryControl = ismEngine_serverGlobal.clientStateExpiryControl;
    assert (expiryControl != NULL);

    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    //Broadcast that the reaper should wake up
    iece_lockExpiryWakeupMutex(expiryControl);

    expiryControl->numWakeups++;

    int os_rc = pthread_cond_broadcast(&(expiryControl->cond_wakeup));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "broadcast failed!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    iece_unlockExpiryWakeupMutex(expiryControl);

    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}


//****************************************************************************
/// @brief Check whether the time specified is earlier than the next
///        scheduled scan and if so, wake up to schedule a scan.
///
/// @param[in]     pThreadData   Thread data to use
/// @param[in]     checkTime     The time to check against
///
/// @return void
//****************************************************************************
void iece_checkTimeWithScheduledScan(ieutThreadData_t *pThreadData, ism_time_t checkTime)
{
    ieceExpiryControl_t *expiryControl = ismEngine_serverGlobal.clientStateExpiryControl;
    assert (expiryControl != NULL);

    iece_lockExpiryWakeupMutex(expiryControl);

    if (expiryControl->scansStarted != 0 && checkTime < expiryControl->nextScheduledScan)
    {
        expiryControl->nextScheduledScan = checkTime;

        // NOTE: This doesn't count as a wake-up, we just want to re-assess the nextScheduledScan.
        int os_rc = pthread_cond_broadcast(&(expiryControl->cond_wakeup));

        if (UNLIKELY(os_rc != 0))
        {
            ieutTRACE_FFDC( ieutPROBE_001, true, "broadcast failed!", ISMRC_Error
                          , "expiryControl", expiryControl, sizeof(*expiryControl)
                          , "os_rc", &os_rc, sizeof(os_rc)
                          , NULL);
        }
    }

    iece_unlockExpiryWakeupMutex(expiryControl);
}

//****************************************************************************
/// @brief destroy wake-up mechanism for clientState expiry reaper
///
/// @param[in]     pThreadData      Thread data to use
/// @param[in]     expiryControl    Expiry Control structure
///
/// @return void
//****************************************************************************
static inline void iece_destroyExpiryReaperWakeupMechanism( ieutThreadData_t *pThreadData
                                                          , ieceExpiryControl_t *expiryControl)
{
    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);
    assert(expiryControl != NULL);

    int os_rc = pthread_cond_destroy(&(expiryControl->cond_wakeup));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "cond_destroy!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }

    os_rc = pthread_mutex_destroy(&(expiryControl->mutex_wakeup));

    if (UNLIKELY(os_rc != 0))
    {
        ieutTRACE_FFDC( ieutPROBE_002, true, "mutex_destroy!", ISMRC_Error
                      , "expiryControl", expiryControl, sizeof(*expiryControl)
                      , "os_rc", &os_rc, sizeof(os_rc)
                      , NULL);
    }
    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}


//****************************************************************************
/// @brief Create and initialize the structures needed to perform clientState
///        expiry and delayed will message publishing.
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iece_initClientStateExpiry( ieutThreadData_t *pThreadData )
{
    int32_t rc = OK;
    ieceExpiryControl_t *expiryControl = ismEngine_serverGlobal.clientStateExpiryControl;

    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (expiryControl != NULL) goto mod_exit;

    expiryControl = iemem_calloc(pThreadData,
                                 IEMEM_PROBE(iemem_messageExpiryData, 3),
                                 1, sizeof(ieceExpiryControl_t));

    if (expiryControl == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    //Allow us to be woken-up
    iece_initExpiryReaperWakeupMechanism(pThreadData, expiryControl);

mod_exit:

    ismEngine_serverGlobal.clientStateExpiryControl = expiryControl;

    if (rc != OK)
    {
        iece_destroyClientStateExpiry(pThreadData);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Start the thread(s) to perform clientState expiry
///
/// @param[in]     pThreadData      Thread data to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t iece_startClientStateExpiry( ieutThreadData_t *pThreadData )
{
    int32_t rc = OK;

    ieceExpiryControl_t *expiryControl = ismEngine_serverGlobal.clientStateExpiryControl;

    ieutTRACEL(pThreadData, expiryControl, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(expiryControl != NULL);
    assert(expiryControl->reaperThreadHandle == 0);

    int startRc = ism_common_startThread(&expiryControl->reaperThreadHandle,
                                         iece_reaperThread,
                                         NULL, expiryControl, 0, // Pass expiryControl as context
                                         ISM_TUSAGE_NORMAL,
                                         0,
                                         "clientReaper",
                                         "Remove_Expired_ClientStates");

    if (startRc != 0)
    {
        ieutTRACEL(pThreadData, startRc, ENGINE_ERROR_TRACE, "ism_common_startThread for clientReaper failed with %d\n", startRc);
        rc = ISMRC_Error;
        ism_common_setError(rc);
    }
    else
    {
        assert(expiryControl->reaperThreadHandle != 0);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

//****************************************************************************
/// @brief Stop the threads performing clientState expiry and will msg delivery
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void iece_stopClientStateExpiry( ieutThreadData_t *pThreadData )
{
    ieceExpiryControl_t *expiryControl = ismEngine_serverGlobal.clientStateExpiryControl;

    ieutTRACEL(pThreadData, expiryControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (expiryControl != NULL && expiryControl->reaperThreadHandle != 0)
    {
        void *retVal = NULL;

        // Request the reaper thread to end, and wait for it to do so
        expiryControl->reaperEndRequested = true;

        iece_wakeClientStateExpiryReaper(pThreadData);

        // Wait for the thread to actually end
        ieut_waitForThread(pThreadData,
                           expiryControl->reaperThreadHandle,
                           &retVal,
                           ieceMAXIMUM_SHUTDOWN_TIMEOUT_SECONDS);

        // The reaper thread doesn't return anything but if it starts to
        // we ought to do something with it!
        assert(retVal == NULL);

        expiryControl->reaperThreadHandle = 0;
    }

    ieutTRACEL(pThreadData, expiryControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @brief Clean up the clientState expiry control.
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void iece_destroyClientStateExpiry( ieutThreadData_t *pThreadData )
{
    ieceExpiryControl_t *expiryControl = ismEngine_serverGlobal.clientStateExpiryControl;

    ieutTRACEL(pThreadData, expiryControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (expiryControl != NULL)
    {
        assert(expiryControl->reaperThreadHandle == 0);

        iece_destroyExpiryReaperWakeupMechanism(pThreadData, expiryControl);

        iemem_free(pThreadData, iemem_messageExpiryData, expiryControl);

        ismEngine_serverGlobal.clientStateExpiryControl = NULL;
    }

    ieutTRACEL(pThreadData, expiryControl, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
/// @internal
///
/// @brief  Add the client to arrays of ones which have a delayed action that
/// should now be performed, like expiry or require a will message to be published.
///
/// @param[in]     pClient           The client state to match against
/// @param[in]     context           ieceFindExpiredClientStateContext_t
///
/// @return true to continue searching or false
//****************************************************************************
static bool iece_findDelayedActionClientState(ieutThreadData_t *pThreadData,
                                              ismEngine_ClientState_t *pClient,
                                              void *context)
{
    ieceFindDelayedActionClientStateContext_t *pContext = (ieceFindDelayedActionClientStateContext_t *)context;
    ism_time_t expiryTime = pClient->ExpiryTime;
    ism_time_t willDelayExpiryTime = pClient->WillDelayExpiryTime;

    // Take a peak at the clientState, only taking it's lock if it looks possible it has expired or
    // has a will message in need of publishing
    if (expiryTime != 0 || willDelayExpiryTime != 0)
    {
        bool bPublishWillMsg = false;
        bool bExpired = false;

        pthread_spin_lock(&pClient->UseCountLock);
        if (pClient->OpState == iecsOpStateZombie && pClient->fSuspendExpiry == false)
        {
            if (willDelayExpiryTime != 0 && willDelayExpiryTime <= pContext->now)
            {
                assert(pClient->hWillMessage != NULL);

                pClient->UseCount += 1;
                // If it's a non-durable client with no durable objects, so we should remove it.
                if (pClient->Durability == iecsNonDurable && pClient->durableObjects == 0)
                {
                    pClient->OpState = iecsOpStateZombieRemoval;
                    expiryTime = pClient->ExpiryTime = 0;
                }
                // Otherwise, if it's expiring at the same time, we can do that.
                else if (willDelayExpiryTime == expiryTime)
                {
                    pClient->OpState = iecsOpStateZombieExpiry;
                    expiryTime = pClient->ExpiryTime = 0; // Don't consider this in subsequent loops
                    pThreadData->stats.expiredClientStates += 1;
                    pThreadData->stats.zombieSetToExpireCount -= 1;
                }
                willDelayExpiryTime = pClient->WillDelayExpiryTime = 0; // Don't consider this in subsequent loops
                bPublishWillMsg = true;
            }
            else if (expiryTime != 0 && expiryTime <= pContext->now)
            {
                pClient->UseCount += 1;
                pClient->OpState = iecsOpStateZombieExpiry;
                expiryTime = pClient->ExpiryTime = 0; // Don't consider this in subsequent loops
                pThreadData->stats.expiredClientStates += 1;
                pThreadData->stats.zombieSetToExpireCount -= 1;
                bExpired = true;
            }
        }
        pthread_spin_unlock(&pClient->UseCountLock);

        // We have set this client state on the path to expiry, add it to the list so we
        // can finish the expiry later (when we've released the lock).
        if (bExpired)
        {
            pContext->expiringClients[pContext->expiringClientCount++] = pClient;
            assert(expiryTime == 0);
        }
        // We have a will message to publish (and might also have started the expiry process),
        // add it to the list so we can finish the publish later (when we've released the lock).
        else if (bPublishWillMsg)
        {
            pContext->willMsgClients[pContext->willMsgClientCount++] = pClient;
            assert(willDelayExpiryTime == 0);
        }

        // If we still have any 'live' expiry times, see if either of them is lower than the
        // lowest one we've seen so far.
        if (willDelayExpiryTime != 0)
        {
            assert(expiryTime == 0 || expiryTime >= willDelayExpiryTime);

            if (willDelayExpiryTime < pContext->lowestTimeSeen)
            {
                pContext->lowestTimeSeen = willDelayExpiryTime;
            }
        }
        else if (expiryTime != 0 && expiryTime < pContext->lowestTimeSeen)
        {
            pContext->lowestTimeSeen = expiryTime;
        }
    }

    return ((pContext->expiringClientCount != ieceMAX_CLIENTSEXPIRY_BATCH_SIZE) &&
            (pContext->willMsgClientCount != ieceMAX_CLIENTSEXPIRY_BATCH_SIZE));
}

//****************************************************************************
/// @brief The actual reaper thread
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
void *iece_reaperThread(void *arg, void *context, int value)
{
    char threadName[16];
    ism_common_getThreadName(threadName, sizeof(threadName));

    ieceExpiryControl_t *expiryControl = (ieceExpiryControl_t *)context;

    // Make sure we're thread-initialised.
    ism_engine_threadInit(0);

    // Not working on behalf of a particular client
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    ieutTRACEL(pThreadData, expiryControl, ENGINE_CEI_TRACE, FUNCTION_ENTRY "Started thread %s with control %p\n",
               __func__, threadName, expiryControl);

    uint64_t numWakeups = 0;

    while(expiryControl->reaperEndRequested == false)
    {
        expiryControl->scansStarted += 1;

        ieceFindDelayedActionClientStateContext_t scanContext = {0, {NULL}, 0, {NULL}, 0, ieceNO_TIMED_SCAN_SCHEDULED, 0, 0};
        uint32_t totalWillMsgsPublished = 0;
        uint32_t totalExpired = 0;
        int32_t rc;

        ieutTRACEL(pThreadData, expiryControl->scansStarted, ENGINE_NORMAL_TRACE,
                   "Starting scan %lu.\n", expiryControl->scansStarted);

        do
        {
            scanContext.now = ism_common_convertExpireToTime(ism_common_nowExpire());

            rc = iecs_traverseClientStateTable(pThreadData,
                                               &scanContext.tableGeneration,
                                               scanContext.startIndex,
                                               ieceMAX_TABLE_CHAINS_TO_SCAN,
                                               &scanContext.startIndex,
                                               iece_findDelayedActionClientState,
                                               &scanContext);

            // If the clientState table is a new generation, start over.
            if (rc == ISMRC_ClientTableGenMismatch)
            {
                assert(scanContext.expiringClientCount == 0);
                assert(scanContext.willMsgClientCount == 0);

                scanContext.startIndex = 0;
                scanContext.tableGeneration = 0;
                scanContext.lowestTimeSeen = ieceNO_TIMED_SCAN_SCHEDULED;

                rc = ISMRC_MoreChainsAvailable;
            }
            else
            {
                assert(rc == OK || rc == ISMRC_MoreChainsAvailable);

                // We have some clients for whom the will message needs to be published (and which
                // may also have expired).
                if (scanContext.willMsgClientCount != 0)
                {
                    totalWillMsgsPublished += scanContext.willMsgClientCount;

                    ieutTRACEL(pThreadData, scanContext.willMsgClientCount, ENGINE_HIGH_TRACE,
                               "Publishing Will messages for %u clients (totalWillMsgsPublished %u)\n",
                               scanContext.willMsgClientCount, totalWillMsgsPublished);

                    for (uint32_t i=0; i<scanContext.willMsgClientCount; i++)
                    {
                        ismEngine_ClientState_t *pClient = scanContext.willMsgClients[i];

                        // iecs_cleanupRemainingResources will release our reference once it has
                        // published the will message.
                        (void)iecs_cleanupRemainingResources(pThreadData,
                                                             pClient,
                                                             iecsCleanup_PublishWillMsg,
                                                             false, false);

                        // NOTE: The array entry is left set to the pClient purely for debugging
                        //       purposes -- it should not be referred to again.
                    }

                    scanContext.willMsgClientCount = 0;
                }

                // We have some clients which have expired (independently of a will message publish)
                if (scanContext.expiringClientCount != 0)
                {
                    totalExpired += scanContext.expiringClientCount;

                    ieutTRACEL(pThreadData, scanContext.expiringClientCount, ENGINE_HIGH_TRACE,
                               "Expiring %u clients (totalExpired %u)\n",
                               scanContext.expiringClientCount, totalExpired);

                    // Release the ones we found
                    for(uint32_t i=0; i<scanContext.expiringClientCount; i++)
                    {
                        ismEngine_ClientState_t *pClient = scanContext.expiringClients[i];

                        iecs_releaseClientStateReference(pThreadData, pClient, false, false);

                        // NOTE: The array entry is left set to the pClient purely for debugging
                        //       purposes -- it should not be referred to again.
                    }

                    scanContext.expiringClientCount = 0;
                }
            }
        }
        while(rc == ISMRC_MoreChainsAvailable && expiryControl->reaperEndRequested == false);

        expiryControl->scansEnded += 1;

        ieutTRACEL(pThreadData, scanContext.lowestTimeSeen, ENGINE_NORMAL_TRACE,
                   "Finished scan %lu. totalExpired=%u totalWillMsgsPublished=%u lowestTimeSeen=%lu.\n",
                   expiryControl->scansEnded, totalExpired, totalWillMsgsPublished, scanContext.lowestTimeSeen);

        // We've not been asked to stop -- So carry on.
        if (expiryControl->reaperEndRequested == false)
        {
            // Ensure that anything waiting for this thread to free memory doesn't wait now
            ieut_leavingEngine(pThreadData);
            iece_expiryReaperSleep( pThreadData, scanContext.lowestTimeSeen, &numWakeups );
            ieut_enteringEngine(NULL);
        }
    }

    ieutTRACEL(pThreadData, expiryControl, ENGINE_CEI_TRACE, FUNCTION_EXIT "Ending thread %s with control %p\n",
               __func__, threadName, expiryControl);
    ieut_leavingEngine(pThreadData);

    // No longer need the thread to be initialized
    ism_engine_threadTerm(1);

    return NULL;
}
