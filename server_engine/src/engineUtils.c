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
/// @file  engineUtils.c
/// @brief Utility function for use within the engine component
//*********************************************************************
#define TRACE_COMP Engine

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <syscall.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "engineStore.h"
#include "memHandler.h"
#include "topicTree.h"         // iett_ functions
#include "engineUtils.h"
#include "threadJobs.h"        // ietj_ functions
#include "resourceSetStats.h"  // iere_ functions

//We don't use these vars - they are here so that we can find the values of the constants
//in gdb python scripts
#ifdef ieutTRACEHISTORY_BUFFERSIZE
xUNUSED const int g_ieutTRACEHISTORY_BUFFERSIZE = ieutTRACEHISTORY_BUFFERSIZE;
#endif
#ifdef ieutTRACEHISTORY_SAVEDTHREADS
xUNUSED const int g_ieutTRACEHISTORY_SAVEDTHREADS = ieutTRACEHISTORY_SAVEDTHREADS;
#endif

//****************************************************************************
// @brief  Acquire a reference to the specified thread data structure.
//
// @param[in]     pThreadData  Pointer to thread data structure to destory
//****************************************************************************
void ieut_acquireThreadDataReference(ieutThreadData_t *pThreadData)
{
    __sync_fetch_and_add(&pThreadData->useCount, 1);
}

//****************************************************************************
// @brief  Release the useCount on an existing thread data structure, freeing
//         resources if the useCount drops to 0.
//
// @param[in]     pThreadData  Pointer to thread data structure to destory
//****************************************************************************
void ieut_releaseThreadDataReference(ieutThreadData_t *pThreadData)
{
    assert(pThreadData != NULL);

    ismEngine_CheckStructId(pThreadData->StrucId, ismENGINE_THREADDATA_STRUCID, ieutPROBE_001);

    DEBUG_ONLY uint32_t oldUseCount = __sync_fetch_and_sub(&pThreadData->useCount, 1);

    assert(oldUseCount != 0); // Should not go negative

    // We were holding the last useCount on the thread data structure - so free it.
    if (oldUseCount == 1)
    {
        ietj_removeThreadJobQueue(pThreadData);

        // Close the store stream if required
        if (pThreadData->closeStream == true)
        {
            assert(pThreadData->hStream != ismSTORE_NULL_HANDLE);
            (void)ism_store_closeStream(pThreadData->hStream);
            pThreadData->hStream = ismSTORE_NULL_HANDLE;
        }

        iett_destroyThreadData(pThreadData);
        iere_destroyResourceSetThreadCache(pThreadData);

        iemem_destroyThreadMemUsage(pThreadData);

        ism_common_free_raw(ism_memory_engine_misc,pThreadData);
    }
}

//****************************************************************************
// @brief  Engine component thread initialization
//
// The Engine component requires some initialisation for each thread
// before it can make engine calls. This function performs this
// initialisation and MUST be called before a thread makes Engine
// calls. This function may be called repeatedly.
//
// @remark The call to ism_engine_threadInit can be made any time after
// ism_engine_init, however engine calls on the initialised thread can only
// be made after messaging has started - the engine does not fully initialise
// the per-thread data until the engine has been started (more specifically
// when the store is able to accept a request to start a store stream for the
// thread).
//
// @param[in]     isStoreCrit      Indicates whether Store performance is critical
//
// @remark The special value ISM_ENGINE_NO_STORE_STREAM can be specified for
// isStoreCrit to indicate that no store stream will be opened for this thread,
// this value should only be used by certain store threads that know it is
// safe to do so.
//****************************************************************************
XAPI void ism_engine_threadInit(uint8_t isStoreCrit)
{
    int32_t rc = OK;

    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (ismEngine_threadData == NULL)
    {
        bool fullyInitialize = false;

        DEBUG_ONLY int osrc;

        osrc = pthread_mutex_lock(&ismEngine_serverGlobal.threadDataMutex);
        assert(osrc == 0);

        rc = ieut_createBasicThreadData();

        if (rc == OK)
        {
            assert(ismEngine_threadData != NULL);
            assert(ismEngine_threadData->jobQueue == NULL);

            ismEngine_threadData->isStoreCritical = isStoreCrit;

            // We can only create full thread data after we've told the store
            // we have completed recovery and before we've begun shutting down.
            if (ismEngine_serverGlobal.runPhase >= EnginePhaseThreadsSelfInitialize &&
                ismEngine_serverGlobal.runPhase <  EnginePhaseEnding)
            {
                fullyInitialize = true;
            }
        }

        osrc = pthread_mutex_unlock(&ismEngine_serverGlobal.threadDataMutex);
        assert(osrc == 0);

        // If we should fully initialize this thread, do so now.
        if (fullyInitialize == true)
        {
            rc = ieut_createFullThreadData(ismEngine_threadData);
        }

        if (rc != OK)
        {
            ieutTRACE_FFDC(ieutPROBE_001, true,
                           "ism_engine_threadInit failed", rc,
                           NULL);
        }
    }

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "pThreadData=%p, rc=%d\n", __func__, ismEngine_threadData, rc);

    return;
}

//****************************************************************************
// @brief  Engine component thread termination
//
// Frees Engine thread-specific data. This function may be called repeatedly.
//
// @param[in]    closeStoreStream   Indicates whether the store stream associated
//                                  with this thread should be closed
//
// @remark Ordinarily (and for most threads) the closeStoreStream flag should be
// true. However for some threads (most notably store persistence threads) this
// will be false because the store has already terminated.
//
// @return Almost certainly.
//****************************************************************************
XAPI void ism_engine_threadTerm(uint8_t closeStoreStream)
{
    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (ismEngine_threadData != NULL)
    {
        // Flush any remaining information from the resourceSet thread cache.
        iere_flushResourceSetThreadCache(ismEngine_threadData);

        // Remove this thread data from the global list of thread data structures
        DEBUG_ONLY int osrc;
        osrc = pthread_mutex_lock(&ismEngine_serverGlobal.threadDataMutex);
        assert(osrc == 0);

        // When a thread data structure is destroyed we 'dump' the stats it had
        // collected into the ieutThreadData_t structure embedded in the server
        // global to preserve this historic information.
        ismEngine_serverGlobal.endedThreadStats.droppedMsgCount += ismEngine_threadData->stats.droppedMsgCount;
        ismEngine_serverGlobal.endedThreadStats.expiredMsgCount += ismEngine_threadData->stats.expiredMsgCount;
        ismEngine_serverGlobal.endedThreadStats.bufferedMsgCount += ismEngine_threadData->stats.bufferedMsgCount;
        ismEngine_serverGlobal.endedThreadStats.internalRetainedMsgCount += ismEngine_threadData->stats.internalRetainedMsgCount;
        ismEngine_serverGlobal.endedThreadStats.externalRetainedMsgCount += ismEngine_threadData->stats.externalRetainedMsgCount;
        ismEngine_serverGlobal.endedThreadStats.bufferedExpiryMsgCount += ismEngine_threadData->stats.bufferedExpiryMsgCount;
        ismEngine_serverGlobal.endedThreadStats.retainedExpiryMsgCount += ismEngine_threadData->stats.retainedExpiryMsgCount;
        ismEngine_serverGlobal.endedThreadStats.remoteServerBufferedMsgBytes += ismEngine_threadData->stats.remoteServerBufferedMsgBytes;
        ismEngine_serverGlobal.endedThreadStats.fromForwarderMsgCount += ismEngine_threadData->stats.fromForwarderMsgCount;
        ismEngine_serverGlobal.endedThreadStats.fromForwarderNoRecipientMsgCount += ismEngine_threadData->stats.fromForwarderNoRecipientMsgCount;
        ismEngine_serverGlobal.endedThreadStats.zombieSetToExpireCount += ismEngine_threadData->stats.zombieSetToExpireCount;
        ismEngine_serverGlobal.endedThreadStats.expiredClientStates += ismEngine_threadData->stats.expiredClientStates;
        ismEngine_serverGlobal.endedThreadStats.resourceSetMemBytes += ismEngine_threadData->stats.resourceSetMemBytes;

#ifdef HAINVESTIGATIONSTATS
        ismEngine_serverGlobal.endedThreadStats.syncCommits   += ismEngine_threadData->stats.syncCommits;
        ismEngine_serverGlobal.endedThreadStats.syncRollbacks += ismEngine_threadData->stats.syncRollbacks;
        ismEngine_serverGlobal.endedThreadStats.asyncCommits  += ismEngine_threadData->stats.asyncCommits;

        if(ismEngine_threadData->stats.minDeliveryBatchSize < ismEngine_serverGlobal.endedThreadStats.minDeliveryBatchSize)
        {
            ismEngine_serverGlobal.endedThreadStats.minDeliveryBatchSize = ismEngine_threadData->stats.minDeliveryBatchSize;
        }
        if(ismEngine_threadData->stats.maxDeliveryBatchSize > ismEngine_serverGlobal.endedThreadStats.maxDeliveryBatchSize)
        {
            ismEngine_serverGlobal.endedThreadStats.maxDeliveryBatchSize = ismEngine_threadData->stats.maxDeliveryBatchSize;
        }
        if ( (  ismEngine_serverGlobal.endedThreadStats.numDeliveryBatches
              + ismEngine_threadData->stats.numDeliveryBatches) > 0)
        {
            ismEngine_serverGlobal.endedThreadStats.avgDeliveryBatchSize = (  (  ismEngine_serverGlobal.endedThreadStats.avgDeliveryBatchSize
                                                                               * ismEngine_serverGlobal.endedThreadStats.numDeliveryBatches)
                                                                            + (  ismEngine_threadData->stats.avgDeliveryBatchSize
                                                                               * ismEngine_threadData->stats.numDeliveryBatches))
                                                                           / (  ismEngine_serverGlobal.endedThreadStats.numDeliveryBatches
                                                                              + ismEngine_threadData->stats.numDeliveryBatches);
        }
        ismEngine_serverGlobal.endedThreadStats.numDeliveryBatches += ismEngine_threadData->stats.numDeliveryBatches;
        ismEngine_serverGlobal.endedThreadStats.messagesDeliveryCancelled += ismEngine_threadData->stats.messagesDeliveryCancelled;

        ismEngine_serverGlobal.endedThreadStats.perConsumerFlowControlCount += ismEngine_threadData->stats.perConsumerFlowControlCount;
        ismEngine_serverGlobal.endedThreadStats.perSessionFlowControlCount  += ismEngine_threadData->stats.perSessionFlowControlCount;

        // Check we didn't miss any fields... we don't want to copy the data for the last 32 consumers locked.... so:
        assert(sizeof(ieutThreadStats_t) ==
               offsetof(ieutThreadStats_t, nextConsSlot) +
               sizeof(uint64_t)); //sizeof changed because of padding //sizeof(ismEngine_threadData->stats.nextConsSlot));
#else
        // Check we didn't miss any fields
        assert(sizeof(ieutThreadStats_t) ==
               offsetof(ieutThreadStats_t, resourceSetMemBytes) +
               sizeof(ismEngine_threadData->stats.resourceSetMemBytes));
#endif

#if (ieutTRACEHISTORY_SAVEDTHREADS > 0)
#if (ieutTRACEHISTORY_BUFFERSIZE > 0)
        //We also want to preserve the per-thread trace data for the thread
        uint32_t ourSlot = 0;
        bool foundSlot = false;

        do
        {
           ourSlot = ismEngine_serverGlobal.traceHistoryNextThread;
           uint32_t nextSlot = (ourSlot +1 ) % ieutTRACEHISTORY_SAVEDTHREADS;

           foundSlot = __sync_bool_compare_and_swap( &(ismEngine_serverGlobal.traceHistoryNextThread)
                                                   , ourSlot
                                                   , nextSlot);
        }
        while (!foundSlot);

        ismEngine_StoredThreadData_t *storedThreadData = &(ismEngine_serverGlobal.storedThreadData[ourSlot]);

        memcpy( storedThreadData->traceHistoryIdent
              , ismEngine_threadData->traceHistoryIdent
              , ieutTRACEHISTORY_BUFFERSIZE * sizeof(ismEngine_serverGlobal.storedThreadData[ourSlot].traceHistoryIdent[0]));
        memcpy( storedThreadData->traceHistoryValue
              , ismEngine_threadData->traceHistoryValue
              , ieutTRACEHISTORY_BUFFERSIZE * sizeof(ismEngine_serverGlobal.storedThreadData[ourSlot].traceHistoryValue[0]));

        storedThreadData->traceHistoryBufPos =  ismEngine_threadData->traceHistoryBufPos;
#endif
#endif
        // Remove the thread data from the chain
        if (ismEngine_serverGlobal.threadDataHead == ismEngine_threadData)
        {
            ismEngine_serverGlobal.threadDataHead = ismEngine_threadData->next;
        }
        else if (ismEngine_threadData->prev != NULL)
        {
            ismEngine_threadData->prev->next = ismEngine_threadData->next;
        }

        if (ismEngine_threadData->next != NULL)
        {
            ismEngine_threadData->next->prev = ismEngine_threadData->prev;
        }

        osrc = pthread_mutex_unlock(&ismEngine_serverGlobal.threadDataMutex);
        assert(osrc == 0);

        // We don't want to close the stream for this thread
        if (closeStoreStream == 0) ismEngine_threadData->closeStream = false;

        ieut_releaseThreadDataReference(ismEngine_threadData);

        ismEngine_threadData = NULL;
    }

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

//****************************************************************************
// @brief  Create a basic thread data structure with enough information to
//         enable tracing but without topic tree / store initialization
//
// @remark ismEngine_serverGlobal.threadDataMutex must be locked when this
//         function is called.
//
// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieut_createBasicThreadData(void)
{
    int32_t rc = OK;

    assert(ismEngine_threadData == NULL);

    ismEngine_threadData = (ieutThreadData_t *)calloc(1, sizeof(ieutThreadData_t));

    if (ismEngine_threadData == NULL)
    {
        TRACE(ENGINE_SEVERE_ERROR_TRACE, "Failed to alloc %ld bytes of memory\n", sizeof(ieutThreadData_t));
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ismEngine_SetStructId(ismEngine_threadData->StrucId, ismENGINE_THREADDATA_STRUCID);

    ismEngine_threadData->useCount = 1;

    // Initialize the thread with the default traceLvl
    ieut_initializeThreadTraceLvl(ismEngine_threadData);

    // Initialize the thread with memory size accounting structure
    rc = iemem_initializeThreadMemUsage(ismEngine_threadData);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    assert(sizeof(ismEngine_threadData->inConfigCallback) == sizeof(uint8_t));
    assert(ismEngine_threadData->inConfigCallback == NoConfigCallback);
#ifndef NDEBUG
    ismEngine_threadData->tid = syscall(SYS_gettid);
#endif
#if (ieutTRACEHISTORY_BUFFERSIZE > 0)
    ismEngine_threadData->traceHistoryBufPos = 0;
    memset(ismEngine_threadData->traceHistoryIdent, 0
          , ieutTRACEHISTORY_BUFFERSIZE * sizeof(ismEngine_threadData->traceHistoryIdent[0]));
    memset(ismEngine_threadData->traceHistoryValue, 0
          , ieutTRACEHISTORY_BUFFERSIZE * sizeof(ismEngine_threadData->traceHistoryValue[0]));
#endif

#ifdef HAINVESTIGATIONSTATS
    ismEngine_threadData->stats.minDeliveryBatchSize = 1000000; //Start with an unfeasibly big min
#endif

    // Add this to the global chain of thread data structures
    if (ismEngine_serverGlobal.threadDataHead != NULL)
    {
        (ismEngine_serverGlobal.threadDataHead->prev) = ismEngine_threadData;
        ismEngine_threadData->next = ismEngine_serverGlobal.threadDataHead;
    }

    ismEngine_serverGlobal.threadIdCounter++;
    ismEngine_threadData->engineThreadId = ismEngine_serverGlobal.threadIdCounter;
    ismEngine_threadData->asyncCounter   = ((uint64_t)ismEngine_threadData->engineThreadId) << 32;

    ismEngine_serverGlobal.threadDataHead = ismEngine_threadData;

mod_exit:

    if (rc != OK && ismEngine_threadData != NULL)
    {
        // The threaddata contains the memusage so is created outside of the policing so use a free raw
        ism_common_free_raw(ism_memory_engine_misc,ismEngine_threadData);
        ismEngine_threadData = NULL;
    }

    return rc;
}

//****************************************************************************
// @brief  Create full thread data structure including topic tree and store
//         initialization
//
// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t ieut_createFullThreadData(ieutThreadData_t *pThreadData)
{
    int32_t rc = OK;

    assert(pThreadData != NULL);

    // Initialise caches for resource set stats (if not already initialized)
    if (pThreadData->resourceSetCache == NULL)
    {
        iere_initResourceSetThreadCache(pThreadData);
    }

    // Take a non-NULL sublist as indication we've already done this
    if (pThreadData->sublist == NULL)
    {
        // Initialize the topic tree on this thread, do so.
        rc = iett_createThreadData(pThreadData);

        if (rc != OK) goto mod_exit;

        // If this thread hasn't indicated that it shouldn't have a store stream opened,
        // open it now.
        if (pThreadData->isStoreCritical != ISM_ENGINE_NO_STORE_STREAM)
        {
            assert(pThreadData->hStream == ismSTORE_NULL_HANDLE);

            // Open a stream for this thread if we're bey
            rc = ism_store_openStream(&pThreadData->hStream, pThreadData->isStoreCritical);

            if (rc != OK) goto mod_exit;

            assert(pThreadData->hStream != ismSTORE_NULL_HANDLE);

            pThreadData->closeStream = true;

            // If isStoreCritical is 1 or 0, we create a job queue for this thread so that
            // we can reschedule work onto it.
            if (pThreadData->isStoreCritical <= 1)
            {
                rc = ietj_addThreadJobQueue(pThreadData);

                if (rc != OK)
                {
                    assert(rc == ISMRC_AllocateError);
                    TRACE(ENGINE_WORRYING_TRACE, FUNCTION_IDENT "Unable to allocate job queue using NULL, pThreadData=%p, rc=%d\n", __func__, pThreadData, rc);
                    rc = OK;
                }
            }
            else
            {
                assert(pThreadData->jobQueue == NULL);
            }

#ifndef NDEBUG
            DEBUG_ONLY int osrc;
            osrc = pthread_mutex_lock(&ismEngine_serverGlobal.threadDataMutex);
            assert(osrc == 0);

            ieutThreadData_t *threadPtr = ismEngine_serverGlobal.threadDataHead;

            while (threadPtr != NULL)
            {
                if (   (threadPtr->hStream == pThreadData->hStream)
                     &&(threadPtr          != pThreadData))
                {
                    TRACE(ENGINE_FFDC, "multiple threads (EIds %u and %u) with hstream %u\n",
                          threadPtr->engineThreadId, pThreadData->engineThreadId,
                          pThreadData->hStream );
                    assert(0);
                }
                threadPtr = threadPtr->next;
            }

            osrc = pthread_mutex_unlock(&ismEngine_serverGlobal.threadDataMutex);
            assert(osrc == 0);
#endif
        }
        else
        {
            assert(pThreadData->hStream == ismSTORE_NULL_HANDLE);
            assert(pThreadData->closeStream == false);
            assert(pThreadData->jobQueue == NULL);
        }
    }

mod_exit:

    return rc;
}

//****************************************************************************
// @brief  Create full thread data structure for all existing thread data
//         structures that have been defined.
//
// @remark This is called once we've got to the point where the store is open
//         and we are about to start messaging - for any threads that have
//         initialised before now, this completes that initialization.
//
// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void ieut_createFullThreadDataForAllThreads(void)
{
    assert(ismEngine_serverGlobal.runPhase == EnginePhaseCompletingRecovery);

    // Iterate over all thread datas we know about
    DEBUG_ONLY int osrc;
    osrc = pthread_mutex_lock(&ismEngine_serverGlobal.threadDataMutex);
    assert(osrc == 0);

    // Create a list of the thread data structures that already existed - these need
    // to have full initialization performed on their behalf.
    int32_t index = 0;
    ieutThreadData_t **pAllThreadDatas = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,35),sizeof(ieutThreadData_t *) * ismEngine_serverGlobal.threadIdCounter);
    assert(pAllThreadDatas != NULL);

    ieutThreadData_t *pThreadData = ismEngine_serverGlobal.threadDataHead;

    while (pThreadData != NULL)
    {
        ieut_acquireThreadDataReference(pThreadData);
        pAllThreadDatas[index++] = pThreadData;
        pThreadData = pThreadData->next;
    }

    assert(index <= ismEngine_serverGlobal.threadIdCounter);

    index--;

    // Before releasing the lock, update the runPhase so that new threads will self-initialize
    ismEngine_serverGlobal.runPhase = EnginePhaseThreadsSelfInitialize;

    osrc = pthread_mutex_unlock(&ismEngine_serverGlobal.threadDataMutex);
    assert(osrc == 0);

    while(index >= 0)
    {
        pThreadData = pAllThreadDatas[index--];

        int32_t rc = ieut_createFullThreadData(pThreadData);

        if (rc != OK)
        {
            ieutTRACE_FFDC(ieutPROBE_001, true,
                           "ieut_createFullThreadDataForAllThreads failed", rc,
                           "pThreadData", pThreadData, sizeof(ieutThreadData_t),
                           NULL);
        }

        ieut_releaseThreadDataReference(pThreadData);
    }

    if (pAllThreadDatas != NULL)
    {
        ism_common_free(ism_memory_engine_misc,pAllThreadDatas);
    }
}

//****************************************************************************
/// @brief  Process this thread's job queue.
///
/// @remark Only expected to be called for this threadNote: The thread chain remains locked while callbacks are called.
//****************************************************************************
bool ieut_processJobQueue(ieutThreadData_t *pThreadData)
{
    assert(pThreadData == ieut_getThreadData());
    return ietj_processJobQueue(pThreadData);
}

//****************************************************************************
/// @brief  Enumerate all of the thread data structures in the global list
///         calling a callback for each.
///
/// @param[in]     callback  Function to call with each thread data structure
/// @param[in]     context   Context information used by the callback routine
///
/// @remark Note: The thread chain remains locked while callbacks are called.
//****************************************************************************
void ieut_enumerateThreadData(ieutThreadData_EnumCallback_t  callback,
                              void *context)
{
    // Run the chain of thread data structures
    DEBUG_ONLY int osrc;
    osrc = pthread_mutex_lock(&ismEngine_serverGlobal.threadDataMutex);
    assert(osrc == 0);

    ieutThreadData_t *pThreadData = ismEngine_serverGlobal.threadDataHead;

    while(pThreadData != NULL)
    {
        callback(pThreadData,
                 context);

        pThreadData = pThreadData->next;
    }

    osrc = pthread_mutex_unlock(&ismEngine_serverGlobal.threadDataMutex);
    assert(osrc == 0);
}

#if (ieutTRACEHISTORY_BUFFERSIZE > 0)
void ieut_traceHistoryBuf(ieutThreadData_t *pThreadData,
                          void *context)
{
    if ((context != (void *)pThreadData) && (pThreadData != NULL))
    {
        TRACE(ENGINE_FFDC, "NEWTHREAD: %p\n", pThreadData);

        uint32_t pos = pThreadData->traceHistoryBufPos;

        for(uint32_t i=0; i<ieutTRACEHISTORY_BUFFERSIZE; i++)
        {
            if (pThreadData->traceHistoryIdent[pos] != 0)
            {
                TRACE(ENGINE_FFDC, "TP[%p,%u]: %lu %lu\n",
                      pThreadData, pos,
                      pThreadData->traceHistoryIdent[pos],
                      pThreadData->traceHistoryValue[pos]);
            }

            pos = (pos+1)%ieutTRACEHISTORY_BUFFERSIZE;
        }

        TRACE(ENGINE_FFDC, "ENDTHREAD: %p\n", pThreadData);
    }
}

void ieut_writeHistoriesToTrace(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // Trace out our thread's history (or a message indicating we didn't have one)
    if (pThreadData != NULL)
    {
        ieut_traceHistoryBuf(pThreadData, NULL);
    }
    else
    {
        TRACE(ENGINE_FFDC, "No Thread data for failing thread\n");
    }

    // Enumerate all other threads (avoiding ours)
    ieut_enumerateThreadData(ieut_traceHistoryBuf, pThreadData);
}
#else
void ieut_writeHistoriesToTrace(void)
{
    TRACE(ENGINE_FFDC, "Trace histories not enabled\n");
}
#endif

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    This function is used to write a block of data about a failure to
///    the trace file.
///
///  @param[in] function        - Function name
///  @param[in] seqId           - Unique Id within function
///  @param[in] filename        - Filename
///  @param[in] lineNo          - line number
///  @param[in] label           - Text description of failure
///  @param[in] retcode         - Associated return code
///  @param[in] ...             - A variable number of arguments (grouped in
///                               sets of 3 terminated with a NULL)
///         1)  ptr (void *)    - Pointer to data to dump
///         2)  length (size_t) - Size of data
///         3)  label           - Text label of data
///
///////////////////////////////////////////////////////////////////////////////
void ieut_ffdc( const char *function
              , uint32_t seqId
              , bool core
              , const char *filename
              , uint32_t lineNo
              , char *label
              , uint32_t retcode
              , ... )
{
    va_list ap;
    void *ptr;
    size_t length;
    char *datalabel;
    char retcodeName[64];
    char retcodeText[128];

    TRACE(ENGINE_FFDC, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    TRACE(ENGINE_FFDC, "!! Probe:    %s:%d\n", function, seqId);
    TRACE(ENGINE_FFDC, "!! Location: %s:%d\n", filename, lineNo);
    TRACE(ENGINE_FFDC, "!! Label:    %s\n", label);
    if (retcode != OK)
    {
        ism_common_getErrorName(retcode, retcodeName, 64);
        ism_common_getErrorString(retcode, retcodeText, 128);
        TRACE(ENGINE_FFDC, "!! Retcode:  %s (%d) - %s\n", retcodeName, retcode, retcodeText);
    }

    if (ENGINE_FFDC <= TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)])
    {
        va_start(ap, retcode);

        do
        {
            datalabel=va_arg(ap, void *);
            if (datalabel != NULL)
            {
                ptr=va_arg(ap, void *);
                length=va_arg(ap, size_t);

                if (ptr == NULL) length = 0; // If a NULL pointer was provided
                traceDataFunction(datalabel, 0, filename, lineNo, ptr, length, length);
            }
        } while (datalabel != NULL);

        va_end(ap);
    }

    if (!core)
    {
        // Write threads' engine trace histories to product trace
        ieut_writeHistoriesToTrace();

        // Write a message to the log with details of the failure
        LOG( ERROR, Messaging, 3004, "%s%d",
             "A failure has occurred at location {0}:{1}. The failure recording routine has been called.",
             function, seqId );

        __sync_add_and_fetch(&ismEngine_serverGlobal.totalNonFatalFFDCs , 1);
    }
    else
    {
        #if 0
        // Mechanism to write a file when a core is being requested to allow a watching process to stop
        // looping -- see the change set that introduced this change for more information.
        FILE *fp = fopen("doneIt.failed", "w");
        fprintf(fp, "FAILED in %s probe %u\n", function, seqId);
        fclose(fp);
        #endif

        // If this error is terminal then bring down the server
        LOG( ERROR, Messaging, 3005, "%s%d",
             "An unrecoverable failure has occurred at location {0}:{1}. The failure recording routine has been called. The server will now stop and restart.",
             function, seqId );

        ism_common_shutdown(true);
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    This function is used to return a text representation of queue type
///////////////////////////////////////////////////////////////////////////////
char * ieut_queueTypeText( ismQueueType_t queueType
                         , char *buffer
                         , size_t bufferLen )
{
    switch (queueType)
    {
        case multiConsumer:
            strncpy(buffer, "MultiConsumer", bufferLen);
            break;
        case simple:
            strncpy(buffer, "Simple", bufferLen);
            break;
        case intermediate:
            strncpy(buffer, "Intermediate", bufferLen);
            break;
        default:
            snprintf(buffer, bufferLen, "UNKNOWN(%d)", queueType);
            break;
    }
    return buffer;
}

static inline void edia_jsonAddName(ieutJSONBuffer_t *outputJSON,
                                    const char *name)
{
    if (outputJSON->newObject == true)
    {
        outputJSON->newObject = false;
    }
    else
    {
        ism_common_allocBufferCopyLen(&outputJSON->buffer, ", ", 2);
    }

    ism_json_putString(&outputJSON->buffer, name);
    ism_common_allocBufferCopyLen(&outputJSON->buffer, ":", 1);
}

void ieut_jsonStartObject(ieutJSONBuffer_t *outputJSON,
                          const char *name)
{
    if (name != NULL)
    {
        edia_jsonAddName(outputJSON, name);
        ism_common_allocBufferCopyLen(&outputJSON->buffer, " {", 2);
        outputJSON->newObject = true;
    }
    else if (outputJSON->newObject)
    {
        ism_common_allocBufferCopyLen(&outputJSON->buffer, "{", 1);
    }
    else
    {
        ism_common_allocBufferCopyLen(&outputJSON->buffer, ", {", 3);
        outputJSON->newObject = true;
    }
}

void ieut_jsonEndObject(ieutJSONBuffer_t *outputJSON)
{
    ism_common_allocBufferCopyLen(&outputJSON->buffer, "}", 1);
    outputJSON->newObject = false;
}

void ieut_jsonStartArray(ieutJSONBuffer_t *outputJSON,
                         char *name)
{
    if (name != NULL)
    {
        edia_jsonAddName(outputJSON, name);
        ism_common_allocBufferCopyLen(&outputJSON->buffer, " [", 2);
        outputJSON->newObject = true;
    }
    else if (outputJSON->newObject)
    {
        ism_common_allocBufferCopyLen(&outputJSON->buffer, "[", 1);
    }
    else
    {
        ism_common_allocBufferCopyLen(&outputJSON->buffer, ", [", 3);
        outputJSON->newObject = true;
    }
}

void ieut_jsonEndArray(ieutJSONBuffer_t *outputJSON)
{
    ism_common_allocBufferCopyLen(&outputJSON->buffer, "]", 1);
    outputJSON->newObject = false;
}

void ieut_jsonAddString(ieutJSONBuffer_t *outputJSON,
                        char *name,
                        char *value)
{
    edia_jsonAddName(outputJSON, name);
    if (value != NULL)
    {
        ism_json_putString(&outputJSON->buffer, value);
    }
    else
    {
        ism_common_allocBufferCopyLen(&outputJSON->buffer, "null", 4);
    }
}

void ieut_jsonAddSimpleString(ieutJSONBuffer_t *outputJSON,
                              char *value)
{
    if (outputJSON->newObject == true)
    {
        outputJSON->newObject = false;
    }
    else
    {
        ism_common_allocBufferCopyLen(&outputJSON->buffer, ", ", 2);
    }

    ism_json_putString(&outputJSON->buffer, value);
}

void ieut_jsonAddPointer(ieutJSONBuffer_t *outputJSON,
                         char *name,
                         void *value)
{
    edia_jsonAddName(outputJSON, name);

    char string[21];
    int len = sprintf(string, "\"%p\"", value);
    // No escaping required, so do this:
    ism_common_allocBufferCopyLen(&outputJSON->buffer, string, len);
}

void ieut_jsonAddInt32(ieutJSONBuffer_t *outputJSON,
                       char *name,
                       int32_t value)
{
    edia_jsonAddName(outputJSON, name);

    char string[11];
    int len = sprintf(string, "%d", value);
    ism_common_allocBufferCopyLen(&outputJSON->buffer, string, len);
}

void ieut_jsonAddUInt32(ieutJSONBuffer_t *outputJSON,
                        char *name,
                        uint32_t value)
{
    edia_jsonAddName(outputJSON, name);

    char string[11];
    int len = sprintf(string, "%u", value);
    ism_common_allocBufferCopyLen(&outputJSON->buffer, string, len);
}

void ieut_jsonAddHexUInt32(ieutJSONBuffer_t *outputJSON,
                           char *name,
                           uint32_t value)
{
    edia_jsonAddName(outputJSON, name);

    char string[13];
    int len = sprintf(string, "\"0x%08x\"", value);
    ism_common_allocBufferCopyLen(&outputJSON->buffer, string, len);
}

void ieut_jsonAddInt64(ieutJSONBuffer_t *outputJSON,
                       char *name,
                       int64_t value)
{
    edia_jsonAddName(outputJSON, name);

    char string[21];
    int len = sprintf(string, "%ld", value);
    ism_common_allocBufferCopyLen(&outputJSON->buffer, string, len);
}

void ieut_jsonAddUInt64(ieutJSONBuffer_t *outputJSON,
                        char *name,
                        uint64_t value)
{
    edia_jsonAddName(outputJSON, name);

    char string[21];
    int len = sprintf(string, "%lu", value);
    ism_common_allocBufferCopyLen(&outputJSON->buffer, string, len);
}

void ieut_jsonAddDouble(ieutJSONBuffer_t *outputJSON,
                        char *name,
                        double value)
{
    edia_jsonAddName(outputJSON, name);

    char string[1024];
    int len = sprintf(string, "%.02f", value);
    ism_common_allocBufferCopyLen(&outputJSON->buffer, string, len);
}

void ieut_jsonAddBool(ieutJSONBuffer_t *outputJSON,
                      char *name,
                      bool value)
{
    edia_jsonAddName(outputJSON, name);
    if (value)
    {
        ism_common_allocBufferCopyLen(&outputJSON->buffer, "true", 4);
    }
    else
    {
        ism_common_allocBufferCopyLen(&outputJSON->buffer, "false", 5);
    }
}

void ieut_jsonAddStoreHandle(ieutJSONBuffer_t *outputJSON,
                             char *name,
                             ismStore_Handle_t value)
{
    edia_jsonAddName(outputJSON, name);

    char string[21];
    int len = sprintf(string, "\"0x%016lx\"", value);
    // No escaping required, so do this:
    ism_common_allocBufferCopyLen(&outputJSON->buffer, string, len);
}

char *ieut_jsonGenerateOutputBuffer(ieutThreadData_t *pThreadData,
                                    ieutJSONBuffer_t *outputJSON,
                                    iemem_memoryType memType)
{
    // Add a carriage return so that it's formatted nicely at the caller
    ism_common_allocBufferCopyLen(&outputJSON->buffer, "\n", 1);

    size_t length = (size_t)(outputJSON->buffer.used);

    char *outbuf = iemem_malloc( pThreadData
                               , IEMEM_PROBE(memType, 60400)
                               , length + 1);

    if (outbuf != NULL)
    {
        memcpy(outbuf, outputJSON->buffer.buf, length);
        outbuf[outputJSON->buffer.used] = '\0';
        outputJSON->buffer.used = 0;
    }

    return outbuf;
}

void ieut_jsonNullTerminateJSONBuffer(ieutJSONBuffer_t *outputJSON)
{
    ism_common_allocBufferCopyLen(&outputJSON->buffer, "", 1);
}

void ieut_jsonResetJSONBuffer(ieutJSONBuffer_t *outputJSON)
{
    outputJSON->buffer.used = 0;
    outputJSON->newObject = true;
}

void ieut_jsonReleaseJSONBuffer(ieutJSONBuffer_t *outputJSON)
{
    ism_common_freeAllocBuffer(&outputJSON->buffer);
    outputJSON->newObject = true;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Wait for up to specified minutes (reporting regularly) for a
///    'remainingActions' count to get down to 0.
///
///  @param[in] remainingActions - pointer to the uint32_t value to follow
///  @param[in] callingFunctoon  - function to report when waiting
///  @param[in] waitForMinutes   - Minutes to wait (cannot be 0)
///
///  @return OK if the value got to 0 or an error.
///////////////////////////////////////////////////////////////////////////////
int32_t ieut_waitForRemainingActions(ieutThreadData_t *pThreadData,
                                     volatile uint32_t *remainingActions,
                                     const char *callingFunction,
                                     uint32_t waitForMinutes)
{
    int32_t rc = OK;
    int pauseMs = 20000; // 0.02s
    uint32_t loop = 0;
    uint32_t reportAfter = waitForMinutes == 1 ? 500 : 3000;  // after 10 seconds or a minute
    uint32_t reportEvery = waitForMinutes == 1 ? 500 : 1500;  // every 10 seconds or 30 seconds
    uint32_t waitForLoops = waitForMinutes * 3000;

    if (waitForLoops == 0)
    {
        rc = ISMRC_InvalidValue;
        ism_common_setError(rc);
    }
    else
    {
        while(*remainingActions != 0)
        {
            ism_common_sleep(pauseMs);

            // After some time...
            if (++loop > reportAfter)
            {
                double elapsedSeconds = ((double)loop) * 0.02;

                // Report every so many seconds
                if ((loop % reportEvery) == 0)
                {
                    assert(pauseMs == 20000);
                    ieutTRACEL(pThreadData, *remainingActions, ENGINE_INTERESTING_TRACE,
                               FUNCTION_IDENT "Waited for %.2f seconds and still %u remaining actions for %s\n",
                               __func__, elapsedSeconds, *remainingActions, callingFunction);
                }

                // After required minutes, report a problem.
                if (loop >= waitForLoops)
                {
                    rc = ISMRC_Error;
                    ieutTRACEL(pThreadData, *remainingActions, ENGINE_ERROR_TRACE,
                                FUNCTION_IDENT "Giving up after waiting %.2f seconds for %s\n",
                                __func__, elapsedSeconds, callingFunction);
                    break;
                }
            }
        }
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Wait for up to specified seconds for a thread to end.
///
///  @param[in] pThreadData - Callers thread data structure
///  @param[in] thread  - The thread that will end.
///  @param[out] retvalptr   - Return Value pointer
///  @param[in] timeout   - Seconds to wait (cannot be 0)
///
///  @remark Will initiate server shutdown if the thread doesn't end
///          within the specified timeout
///////////////////////////////////////////////////////////////////////////////
void ieut_waitForThread(ieutThreadData_t *pThreadData, ism_threadh_t thread, void ** retvalptr, uint32_t timeout)
{
    ieutTRACEL(pThreadData, thread, ENGINE_SHUTDOWN_DIAG_TRACE,
               FUNCTION_ENTRY "thread=%lu timeout=%u\n", __func__, thread, timeout);

    ism_time_t timeoutNanos = (ism_time_t) timeout * 1000000000UL;

    int32_t rc = ism_common_joinThreadTimed(thread, retvalptr, timeoutNanos);
    if(rc !=0)
    {
        ieutTRACE_FFDC( ieutPROBE_001, true,
                        "Thread did not finish within allowed timeout.",
                        ISMRC_Error,
                        &timeout, sizeof(timeout), "Timeout",
                        &thread, sizeof(thread), "Thread",
                        NULL );
    }

    ieutTRACEL(pThreadData, thread, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);
}

/*********************************************************************/
/*                                                                   */
/* End of engineUtils.c                                              */
/*                                                                   */
/*********************************************************************/
