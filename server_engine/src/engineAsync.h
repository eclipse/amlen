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

//****************************************************************************
/// @file  engineAsync.h
/// @brief Types and functions for handling going async
//****************************************************************************
#ifndef __ISM_ENGINE_ENGINEASYNC_DEFINED
#define __ISM_ENGINE_ENGINEASYNC_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "engineCommon.h"

//How many microseconds the sleep in CBQ Alert pause processing is
#define IEAD_ASYNC_CBQ_ALERT_PAUSE_SLEEP_USEC 1000

//Normally an ism_engine_ level function creates an array on the the stack
//which is filled in as we get deeper.... this defines num entries in the array
#define IEAD_MAXARRAYENTRIES 8

typedef enum tag_ismAsyncDataType_t
{
    EngineCaller,
    EngineAcknowledge,
    EngineProcessBatchAcks1,
    EngineProcessBatchAcks2,
    EngineTranCommit, ///Part way through an engine tran commit, we used this mechanism to go async
    EngineTranForget,
    EnginePrepareGlobal,
    EngineRemoveUnreleasedDeliveryId,
    ieiqQueueConsume,
    ieiqQueueDeliver,
    ieiqQueueMsgReceived,
    ieiqQueuePostCommit,
    ieiqQueueCancelDeliver,
    ieiqQueueCompleteCheckWaiters,
    ieiqQueueDestroyMessageBatch1,
    ieiqQueueDestroyMessageBatch2,
    ieiqQueueCheckAndDiscard,
    iemqQueueAcknowledge,
    iemqQueueDeliver,
    iemqQueueMsgReceived,
    iemqQueuePostCommit,
    iemqQueueCancelDeliver,
    iemqQueueRestartSession,
    iemqQueueCompleteCheckWaiters,
    iemqQueueDestroyMessageBatch1,
    iemqQueueDestroyMessageBatch2,
    iemqQueueCheckAndDiscard,
    RemoveMDR,
    TranPrepare,
    TranCreateLocal,
    TranCreateGlobal,
    TranForget,
    TopicCreateSubscriptionPhaseInfo,
    TopicCreateSubscriptionTopicString,
    TopicCreateSubscriptionClientInfo,
    TopicLazyUnstoreCompletionInfo,
    ClientStateAdditionCompletionInfo,
    ClientStateFinishAdditionInfo,
    ClientStateDestroyCompletionInfo,
    ClientStateUnstoreUnreleasdMessageStateCompletionInfo,
    ImportResourcesContinue,
} ismAsyncDataType_t;

#ifndef ASYNCDATA_FORWARDDECLARATION
#define ASYNCDATA_FORWARDDECLARATION
typedef struct tag_ismEngine_AsyncDataEntry_t ismEngine_AsyncDataEntry_t; //forward declaration...lower in this file
typedef struct tag_ismEngine_AsyncData_t      ismEngine_AsyncData_t;//forward declaration...lower in this file
#endif

typedef int32_t (*ieadInternalCompletionCallback_t)(
    ieutThreadData_t               *pThreadData,
    int32_t                         retcode,
    ismEngine_AsyncData_t          *asyncInfo,
    ismEngine_AsyncDataEntry_t     *context);

typedef union tag_ieadCompletionCallback_t
{
    ismEngine_CompletionCallback_t    externalFn;
    ieadInternalCompletionCallback_t  internalFn;
} ieadCompletionCallback_t;

struct tag_ismEngine_AsyncDataEntry_t {
    char                              StrucId[4];
    ismAsyncDataType_t                Type;
    void                             *Data;                  //The data pointed to here is copied if this structure is copied
    size_t                            DataLen;
    void                             *Handle;                //The data pointed to here is NOT copied if this structure is copied
    ieadCompletionCallback_t          pCallbackFn;
};

#define ismENGINE_ASYNCDATAENTRY_STRUCID "EADE"


struct tag_ismEngine_AsyncData_t {
    char                              StrucId[4];
    ismEngine_ClientState_t           *pClient;
    uint32_t                          numEntriesAllocated;
    uint32_t                          numEntriesUsed;
    uint64_t                          asyncId;             //Used to tie up commit and callback
    bool                              fOnStack;
    uint64_t                          DataBufferAllocated; //Only used when fOnStack is false
    uint64_t                          DataBufferUsed;      //Only used when fOnStack is false
    ismEngine_AsyncDataEntry_t       *entries;
};

#define ismENGINE_ASYNCDATA_STRUCID "EADS"

void iead_setEngineCallerHandle( ismEngine_AsyncData_t *asyncData
                               , void *handle);

int32_t iead_store_asyncCommit( ieutThreadData_t *pThreadData
                              , bool commitReservation
                              , ismEngine_AsyncData_t *asyncData);

int32_t iead_checkAsyncCallbackQueue(ieutThreadData_t *pThreadData,
                                     ismEngine_Transaction_t *pTran,
                                     bool failIfAlerted);

void iead_pushAsyncCallback( ieutThreadData_t *pThreadData
                           , ismEngine_AsyncData_t *AsyncInfo
                           , ismEngine_AsyncDataEntry_t *newEntry);

//We always seem to have the dataLen of the last entry in our hand when we
//call this function so we pass it in as a param to save getting in
static inline void iead_popAsyncCallback( ismEngine_AsyncData_t *AsyncInfo
                                        , uint64_t dataLen)
{
    assert(AsyncInfo->numEntriesUsed > 0);

    if(AsyncInfo->fOnStack == false)
    {
        uint64_t roundedDataLen = RoundUp16(dataLen);

#ifndef NDEBUG
        //Check the data size for last entry we've been passed is correct
        ismEngine_AsyncDataEntry_t *lastEntry = &(AsyncInfo->entries[AsyncInfo->numEntriesUsed - 1]);
        assert (lastEntry->DataLen == dataLen);
#endif
        assert(AsyncInfo->DataBufferUsed >= roundedDataLen);
        AsyncInfo->DataBufferUsed -= roundedDataLen;
    }
    AsyncInfo->numEntriesUsed--;
}

ismEngine_AsyncData_t *iead_ensureAsyncDataOnHeap( ieutThreadData_t *pThreadData
                                                 , ismEngine_AsyncData_t *asyncData );

void iead_completeAsyncData(int rc, void *context);

void iead_freeAsyncData( ieutThreadData_t *pThreadData
                       , ismEngine_AsyncData_t *asyncData);

#ifdef __cplusplus
}
#endif

#endif
