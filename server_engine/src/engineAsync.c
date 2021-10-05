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

///////////////////////////////////////////////////////////////////////////////
///
///  @file  engineAsync.c
///
///  @brief
///   Keeps track of data and functions need to be called after an async action
///
///
///////////////////////////////////////////////////////////////////////////////
#define TRACE_COMP Engine

#include <stdint.h>
#include <stdbool.h>

#include "engineInternal.h"
#include "engineAsync.h"
#include "memHandler.h"
#include "engineStore.h"

// Whether someone is in the process of shutting down because we hit the max CBQ alert pause.
static bool asyncCBQAlertShutdownInProgress = false;

static void iead_copyAsyncData( ieutThreadData_t *pThreadData
                              , ismEngine_AsyncData_t *in
                              , ismEngine_AsyncData_t **out)
{
    // We make an assumption here that there will be no more than MAXARRAYENTRIES ever in
    // the asyncdata, even if this goes async again - we should at least confirm there are
    // no more than that now!
    assert(in->numEntriesUsed <= IEAD_MAXARRAYENTRIES);
    assert(in->fOnStack == true);

    uint64_t memBytes = 0;
    uint32_t numEntriesUsed = in->numEntriesUsed;
    uint32_t numEntriesToAllocate = IEAD_MAXARRAYENTRIES;

    //Work out how much memory we'll need
    for (uint32_t i = 0; i < numEntriesUsed; i++)
    {
        memBytes += RoundUp16(in->entries[i].DataLen);
    }

    // Add space for the chosen number of array entries
    memBytes  += numEntriesToAllocate * sizeof(ismEngine_AsyncDataEntry_t);

    //Eventually if we embrace async commit, we need to avoid this malloc by having cached
    //the memory at some point before we come to use it.....
    ismEngine_AsyncData_t *dest = iemem_malloc( pThreadData
                                              , IEMEM_PROBE(iemem_commitData, 1)
                                              , sizeof(ismEngine_AsyncData_t) + memBytes);

    if (dest == NULL)
    {
        //We shouldn't really be allocating memory during a commit, we can't really
        //fail... we're using a category of memory that shouldn't fail... if it does
        //go down in flames
        ieutTRACE_FFDC( ieutPROBE_001, true, "Out of memory during commit", ISMRC_Error
                      , NULL);
    }
    else
    {
        ieutTRACEL(pThreadData, dest, ENGINE_HIFREQ_FNC_TRACE,
            FUNCTION_IDENT "Copied in (%s) =%p to out=%p\n", __func__,
                 in->fOnStack ? "stack" : "heap", in,  dest);

        char *curPos = (char *)dest;

        memcpy(curPos, in, sizeof(ismEngine_AsyncData_t));
        curPos += sizeof(ismEngine_AsyncData_t);

        uint64_t arraySize = numEntriesToAllocate * sizeof(ismEngine_AsyncDataEntry_t);
        memcpy(curPos, in->entries, arraySize);
        ismEngine_AsyncDataEntry_t *destEntries = (ismEngine_AsyncDataEntry_t *)curPos;
        dest->entries = destEntries;
        curPos += arraySize;

        for (uint32_t i = 0; i < numEntriesUsed; i++)
        {
            if (destEntries[i].DataLen > 0)
            {
                memcpy(curPos, destEntries[i].Data, destEntries[i].DataLen);
                destEntries[i].Data = curPos;
                curPos += RoundUp16(destEntries[i].DataLen);
            }
            else
            {
                destEntries[i].Data = NULL;
            }
        }
        dest->fOnStack = false; //We've just constructed it on the heap
        dest->numEntriesAllocated = numEntriesToAllocate;
        dest->DataBufferAllocated = memBytes;
        dest->DataBufferUsed      = memBytes;
    }

    *out = dest;
}

void iead_pushAsyncCallback( ieutThreadData_t *pThreadData
                           , ismEngine_AsyncData_t *AsyncInfo
                           , ismEngine_AsyncDataEntry_t *newEntry)
{
    assert(AsyncInfo->numEntriesUsed < AsyncInfo->numEntriesAllocated);

    if (!AsyncInfo->fOnStack)
    {
        uint64_t roundedDataLen = RoundUp16(newEntry->DataLen);

        if (roundedDataLen != 0)
        {
            uint64_t dataBufferNeeded = AsyncInfo->DataBufferUsed+roundedDataLen;

            if (dataBufferNeeded > AsyncInfo->DataBufferAllocated)
            {
                ismEngine_AsyncDataEntry_t *newEntries;

                // This is either the 1st or a subsequent time we've had to do this.
                if ((void *)(AsyncInfo->entries) == (void *)(&AsyncInfo->entries+1))
                {
                    newEntries = iemem_malloc( pThreadData
                                             , IEMEM_PROBE(iemem_commitData, 2)
                                             , dataBufferNeeded );

                    assert(newEntries != NULL);

                    memcpy(newEntries, AsyncInfo->entries, AsyncInfo->DataBufferUsed);
                }
                else
                {
                    newEntries = iemem_realloc( pThreadData
                                              , IEMEM_PROBE(iemem_commitData, 3)
                                              , AsyncInfo->entries
                                              , dataBufferNeeded );

                    assert(newEntries != NULL);
                }

                // Move entries that are now in different places
                if (AsyncInfo->entries != newEntries)
                {
                    uint8_t *origEntries = (uint8_t *)(AsyncInfo->entries);

                    for(int32_t entry=0; entry < AsyncInfo->numEntriesUsed; entry++)
                    {
                        if (newEntries[entry].DataLen != 0)
                        {
                            ptrdiff_t diff = (uint8_t *)(newEntries[entry].Data) - origEntries;
                            newEntries[entry].Data = (uint8_t *)newEntries + diff;
                        }
                        else
                        {
                            assert(newEntries[entry].Data == NULL);
                        }
                    }

                    AsyncInfo->entries = newEntries;
                }

                AsyncInfo->DataBufferAllocated = dataBufferNeeded;
            }

            assert((AsyncInfo->DataBufferUsed + roundedDataLen) <= AsyncInfo->DataBufferAllocated);
            void *dataDest = ((char *)AsyncInfo->entries)+AsyncInfo->DataBufferUsed;
            memcpy( dataDest
                  , newEntry->Data
                  , newEntry->DataLen);
            AsyncInfo->DataBufferUsed += roundedDataLen;
            newEntry->Data = dataDest;
        }
        else
        {
            assert(newEntry->Data == NULL);
            assert(newEntry->DataLen == 0);
        }
    }

    AsyncInfo->entries[AsyncInfo->numEntriesUsed] = *newEntry;
    AsyncInfo->numEntriesUsed++;
}

ismEngine_AsyncData_t *iead_ensureAsyncDataOnHeap( ieutThreadData_t *pThreadData
                                                 , ismEngine_AsyncData_t *asyncData )
{
    ismEngine_AsyncData_t *newAsyncData;

    if (asyncData->fOnStack == true)
    {
        iead_copyAsyncData( pThreadData
                          , asyncData
                          , &newAsyncData );
    }
    else
    {
        newAsyncData = asyncData;
    }

    return newAsyncData;
}

void iead_freeAsyncData( ieutThreadData_t *pThreadData
                       , ismEngine_AsyncData_t *asyncData)
{

    if (asyncData->fOnStack)
    {
        ieutTRACEL(pThreadData, asyncData, ENGINE_HIFREQ_FNC_TRACE,
            FUNCTION_IDENT "Finished with stack: asyncData=%p\n", __func__, asyncData);
    }
    else
    {
        ieutTRACEL(pThreadData, asyncData, ENGINE_HIFREQ_FNC_TRACE,
            FUNCTION_IDENT "freeing asyncData=%p\n", __func__, asyncData);

        // If we had to re-allocate the entries, free the additional area
        if ((void *)(asyncData->entries) != (void *)(&asyncData->entries+1))
        {
            iemem_freeStruct(pThreadData, iemem_commitData, asyncData->entries, asyncData->entries->StrucId);
        }

        iemem_freeStruct(pThreadData, iemem_commitData, asyncData, asyncData->StrucId);
    }
}

void iead_completeAsyncData(int rc, void *context)
{
    ismEngine_AsyncData_t *pAsyncData = (ismEngine_AsyncData_t *)context;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pAsyncData->pClient);

    pThreadData->threadType = AsyncCallbackThreadType;

    ieutTRACEL(pThreadData, pAsyncData->asyncId, ENGINE_CEI_TRACE,
               FUNCTION_ENTRY "pAsyncData=%p, ieadACId=0x%016lx rc=%d\n",
               __func__, pAsyncData, pAsyncData->asyncId, rc);

    //NB: We're counting down to 0 with an unsigned variable...careful!
    //http://stackoverflow.com/questions/804777/counting-down-in-for-loops/804893#804893

    while( pAsyncData->numEntriesUsed > 0 )
    {
        ismEngine_AsyncDataEntry_t *entry = &(pAsyncData->entries[(pAsyncData->numEntriesUsed)-1]);

        if(entry->Type == EngineCaller)
        {
            //Callers to the engine don't have to provide a callback function
            ismEngine_CompletionCallback_t externalFn = entry->pCallbackFn.externalFn;
            if (externalFn != NULL)
            {
                externalFn(rc, entry->Handle, entry->Data);
            }
            //Don't call the external function again
            iead_popAsyncCallback( pAsyncData, entry->DataLen);
        }
        else
        {
            rc = entry->pCallbackFn.internalFn(pThreadData, rc, pAsyncData, entry);

            if (rc == ISMRC_AsyncCompletion)
            {
                //aargh, gone async again... get out of here
                //NB: Won't free pAsyncData on assumption we're reusing it for
                //async-ness
                goto mod_exit;
            }

        }
    }

    iead_freeAsyncData(pThreadData, pAsyncData);

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
}


void iead_setEngineCallerHandle( ismEngine_AsyncData_t *asyncData
                               , void *handle)
{
    bool engineCallerFound=false;

    for( uint32_t i = asyncData->numEntriesUsed; i-- > 0; )
    {
        if (asyncData->entries[i].Type == EngineCaller)
        {
            asyncData->entries[i].Handle = handle;
            engineCallerFound=true;
            break;
        }
    }

    if (!engineCallerFound)
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "No engine caller", ISMRC_Error
                      , NULL);
    }
}

int32_t iead_store_asyncCommit( ieutThreadData_t *pThreadData
                              , bool commitReservation
                              , ismEngine_AsyncData_t *asyncData)
{
    ismEngine_AsyncData_t *callbackData = NULL;


    int32_t rc = OK;

    if (asyncData != NULL)
    {
        if (asyncData->fOnStack)
        {
            iead_copyAsyncData(pThreadData, asyncData, &callbackData);
        }
        else
        {
            callbackData = asyncData;
        }
    }

    ieutTRACEL(pThreadData, callbackData, ENGINE_CEI_TRACE,
                         FUNCTION_IDENT "callbackData=%p\n", __func__, callbackData);

    if (callbackData != NULL)
    {
        //Setup AsyncId and trace it so we can tie up commit with callback:
        callbackData->asyncId = pThreadData->asyncCounter++;
        ieutTRACEL(pThreadData, callbackData->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "ieadACId=0x%016lx\n",
                              __func__, callbackData->asyncId);

        rc = iest_store_asyncCommit(pThreadData, commitReservation, iead_completeAsyncData, callbackData);

        if (rc == OK)
        {
            //We didn't go async, we don't need any of the async data
            iead_freeAsyncData(pThreadData, callbackData);
        }
    }
    else
    {
        //No async data... must be from code not ready for async commit
        iest_store_commit(pThreadData, commitReservation);
    }

    assert( rc == OK || rc == ISMRC_AsyncCompletion);

    return rc;
}

int32_t iead_checkAsyncCallbackQueue(ieutThreadData_t *pThreadData,
                                     ismEngine_Transaction_t *pTran,
                                     bool failIfAlerted)
{
    int32_t rc = OK;

    //Really crude pausing mechanism if store has too many callbacks queued.
    //Ought to cond_wait...
    if (ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE] != StatusOk)
    {
        // If the caller would rather fail if the Async callback queue status is not OK, fail now.
        if (failIfAlerted == true)
        {
            rc = ISMRC_AsyncCBQAlerted;
            ism_common_setError(rc);
        }
        else
        {
            // We don't want to pause any threads that have been identified as async callback threads
            // since that will impact our ability to reduce the async callback queue.
            if (pThreadData->threadType != AsyncCallbackThreadType)
            {
                uint64_t numPauses = 0;
                uint64_t maxPauses = (((uint64_t)ismEngine_serverGlobal.AsyncCBQAlertMaxPause)*1000000UL)/(uint64_t)IEAD_ASYNC_CBQ_ALERT_PAUSE_SLEEP_USEC;

                ieutTRACEL(pThreadData, pTran, ENGINE_INTERESTING_TRACE, FUNCTION_ENTRY
                           "Pausing for Async Callback Queue to reduce (Status: %d) pTran=%p (maxPauses: %lu)\n",
                           __func__, (int)ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE],
                           pTran, maxPauses);

                ism_common_backHome();
                while (ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE] != StatusOk)
                {
                    iememMemoryLevel_t currMallocState = iemem_queryCurrentMallocState();

                    // Memory is running low - so give up.
                    if (currMallocState >= iememDisableEarly)
                    {
                        rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                        break;
                    }
                    else
                    {
                        usleep(IEAD_ASYNC_CBQ_ALERT_PAUSE_SLEEP_USEC);
                        numPauses++;

                        // We set a maximum number of pauses, and we've got to the end of it.
                        if (maxPauses != 0 && numPauses >= maxPauses)
                        {
                            // We don't want all the threads which hit this situation to try to shut down,
                            // so we only do it if we're the first one to hit it.
                            if (__sync_bool_compare_and_swap(&asyncCBQAlertShutdownInProgress, false, true) == true)
                            {
                                ieutTRACE_FFDC( ieutPROBE_001, true, "Paused for too long waiting for AsyncCBQ status to change", ISMRC_TimeOut
                                              , "asyncCBQ status", &ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE],
                                                                   sizeof(ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE])
                                              , "currMallocState", &currMallocState, sizeof(currMallocState)
                                              , "maxPauses", &maxPauses, sizeof(maxPauses)
                                              , NULL);
                            }
                        }
                    }
                }
                ism_common_going2work();
                ieutTRACEL(pThreadData, rc, ENGINE_INTERESTING_TRACE, FUNCTION_EXIT
                           "Finished pausing (Status: %d) numPauses=%lu (maxPauses=%lu) rc=%d\n",
                           __func__, ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE],
                           numPauses, maxPauses, rc);
            }
        }
    }

    return rc;
}
