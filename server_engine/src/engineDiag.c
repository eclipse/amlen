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
/// @file  engineDiag.c
/// @brief Function that support the diagnostics REST calls for debugging
//****************************************************************************

#define TRACE_COMP Engine

#include <ctype.h>
#include <sys/stat.h>

#include "engineDiag.h"
#include "engineUtils.h"
#include "clientState.h"
#include "topicTree.h"
#include "topicTreeInternal.h"
#include "queueCommon.h"
#include "engineTraceDump.h"
#include "mempool.h"
#include "engineMonitoring.h"
#include "resourceSetStats.h"
#include "ismjson.h"

// Execution modes
typedef enum tag_ediaExecMode_t
{
    execMode_NONE = 0,
    execMode_ECHO,
    execMode_MEMORYDETAILS,
    execMode_DUMPCLIENTSTATES,
    execMode_OWNERCOUNTS,
    execMode_DUMPTRACEHISTORY,
    execMode_SUBDETAILS,
    execMode_DUMPRESOURCESETS,
    execMode_RESOURCESETREPORT,
    execMode_MEMORYTRIM,
    execMode_ASYNCCBSTATS,
    execMode_LAST // Add new entries above this
} ediaExecMode_t;

#define ediaSUBDETAILS_MEMPOOL_RESERVEDMEMBYTES     0
#define ediaSUBDETAILS_MEMPOOL_INITIALPAGEBYTES     4096
#define ediaSUBDETAILS_MEMPOOL_SUBSEQUENTPAGEBYTES  4096

//****************************************************************************
/// @brief  Parse a simple whitespace separated argument list into an array of
///         strings.
///
/// @param[in]     args               Arguments to be parsed
/// @param[in]     minArgs            Minimum number of args required
/// @param[in]     maxArgs            Maximum number of args expected (0 means
///                                   unlimited).
/// @param[out]    parsedArgs         Array containing array of parsed args
///
/// @remarks parsedArgs must be freed with memType of iemem_diagnostics
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
static inline int32_t edia_parseSimpleArgs(ieutThreadData_t *pThreadData,
                                           const char *args,
                                           uint32_t minArgs,
                                           uint32_t maxArgs,
                                           char ***parsedArgs)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, args, ENGINE_FNC_TRACE, FUNCTION_ENTRY "args=%p, minArgs=%u, maxArgs=%u\n",
               __func__, args, minArgs, maxArgs);

    char **newArgs = NULL;
    uint32_t parsedArgIndex = 0;

    assert(args != NULL);

    size_t argsLen = strlen(args);

    // Must be enough characters for minArgs single character length values
    if (minArgs > 0 && argsLen < (minArgs*2)-1)
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // No maximum arguments specified, so we need to allow for the possible number we have
    if (maxArgs == 0) maxArgs = (argsLen/2)+1;

    newArgs = iemem_calloc(pThreadData,
                           iemem_diagnostics,
                           1,
                           (sizeof(char *)*(maxArgs+1))+argsLen+1);

    if (newArgs == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }


    const char *argStart = NULL;
    const char *argPos = args;
    char *parsedArgPos = (char *)(newArgs+maxArgs+1);
    while(1)
    {
        char thisChar = *argPos;

        if (isblank(thisChar) || thisChar == '\0')
        {
            if (argStart != NULL)
            {
                if (parsedArgIndex >= maxArgs)
                {
                    rc = ISMRC_InvalidParameter;
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                size_t argLen = (size_t)(argPos-argStart)+1;

                newArgs[parsedArgIndex++] = parsedArgPos;

                memcpy(parsedArgPos, argStart, argLen);
                parsedArgPos[argLen-1] = '\0';
                parsedArgPos += argLen;

                argStart = NULL;
            }

            if (thisChar == '\0') break;
        }
        else if (argStart == NULL)
        {
            argStart = argPos;
        }

        argPos++;
    }


    if (parsedArgIndex < minArgs)
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
        goto mod_exit;
    }

    assert(newArgs[parsedArgIndex] == NULL);

    *parsedArgs = newArgs;

mod_exit:

    if (rc != OK)
    {
        iemem_free(pThreadData, iemem_diagnostics, newArgs);
        newArgs = NULL;
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, newArgs=%p, parsedArgIndex=%u\n",
               __func__, rc, newArgs, parsedArgIndex);

    return rc;
}

//****************************************************************************
/// @brief  (Simulated) async diagnostic function used for echo mode
///
/// Echo request back to the caller
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @remark If the return code is OK then diagnosticsOutput will be a string that
/// should be returned to the user and then freed using ism_engine_freeDiagnosticsOutput().
/// If the return code is ISMRC_AsyncCompletion then the callback function will be called
/// and if the retcode is OK, the handle will point to the string to show and then free with
/// ism_engine_freeDiagnosticsOutput()
///
/// @returns OK on successful completion
///          ISMRC_AsyncCompletion if gone asynchronous
///          or an ISMRC_ value if there is a problem
//****************************************************************************
static int edia_completeEchoAsyncDiagnostics(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    // Make sure we're thread-initialised. This can be issued repeatedly.
    ism_engine_threadInit(0);

    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    ediaEchoAsyncData_t *diagInfo  = (ediaEchoAsyncData_t *)userdata;

    ieutTRACEL(pThreadData, diagInfo, ENGINE_CEI_TRACE, FUNCTION_IDENT "diagInfo=%p\n"
               , __func__, diagInfo);

    if (diagInfo->pCallbackFn)
    {
        diagInfo->pCallbackFn(OK, diagInfo->OutBuf, diagInfo->pContext);
    }

    iemem_freeStruct(pThreadData,iemem_callbackContext, diagInfo, diagInfo->StrucId);
    ieut_leavingEngine(pThreadData);
    ism_common_cancelTimer(key);

    //We did engine threadInit...  term will happen when the engine terms and after the counter
    //of events reaches 0
    __sync_fetch_and_sub(&(ismEngine_serverGlobal.TimerEventsRequested), 1);
    return 0;
}

//****************************************************************************
/// @brief  Echo
///
/// Echo request info back to the caller
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @remarks The function performs some basic processing on the args to show how
///          it would be split by edia_parseSimpleArgs.
///
/// @returns OK on successful completion
///          ISMRC_AsyncCompletion if gone asynchronous
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t edia_modeEcho(ieutThreadData_t *pThreadData,
                      const char *mode,
                      const char *args,
                      char **pDiagnosticsOutput,
                      void *pContext,
                      size_t contextLength,
                      ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;

    bool synchronous = (rand()%4 < 1); // Whether to return synchronously

    char xbuf[1024];
    ieutJSONBuffer_t buffer = {true, {xbuf, sizeof(xbuf)}};

    ieutTRACEL(pThreadData, contextLength, ENGINE_FNC_TRACE, FUNCTION_ENTRY "synchronous=%d contextLength=%lu\n",
               __func__, (int)synchronous, contextLength);

    ieut_jsonStartObject(&buffer, NULL);
    ieut_jsonAddString(&buffer, "Mode", (char *)mode);
    ieut_jsonAddString(&buffer, "Args", (char *)args);
    ieut_jsonStartArray(&buffer, "SimpleArgs");

    char **simpleArgs = NULL;

    rc = edia_parseSimpleArgs(pThreadData, args, 0, 0, &simpleArgs);

    uint32_t simpleArgCount = 0;
    if (rc == OK)
    {
        while(simpleArgs[simpleArgCount] != NULL)
        {
            ieut_jsonAddSimpleString(&buffer, simpleArgs[simpleArgCount]);
            simpleArgCount++;
        }

        iemem_free(pThreadData, iemem_diagnostics, simpleArgs);
    }

    ieut_jsonEndArray(&buffer);
    ieut_jsonAddBool(&buffer, "ContextPointer", (pContext != NULL));
    ieut_jsonAddUInt64(&buffer, "ContextLength", (uint64_t)contextLength);
    ieut_jsonAddBool(&buffer, "CallbackPointer", (pCallbackFn != NULL));
    ieut_jsonAddBool(&buffer, "Async", !synchronous);
    ieut_jsonEndObject(&buffer);

    char *outbuf = ieut_jsonGenerateOutputBuffer(pThreadData, &buffer, iemem_diagnostics);

    if (outbuf == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Return synchronously...
    if (synchronous)
    {
        //We'll return back synchronously
        *pDiagnosticsOutput = outbuf;
    }
    // Return asynchronously...
    else
    {
        ediaEchoAsyncData_t *diagInfo = iemem_malloc( pThreadData
                                                    , IEMEM_PROBE(iemem_callbackContext, 10)
                                                    , sizeof(ediaEchoAsyncData_t) + contextLength);

        if (diagInfo == NULL)
        {
            iemem_free(pThreadData, iemem_diagnostics, outbuf);
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        ieutTRACEL(pThreadData, diagInfo, ENGINE_FNC_TRACE, "diagInfo=%p\n", diagInfo);

        ismEngine_SetStructId(diagInfo->StrucId, ediaECHOASYNCDIAGNOSTICS_STRUCID);
        diagInfo->OutBuf = outbuf;
        diagInfo->contextLength = contextLength;
        diagInfo->pCallbackFn   = pCallbackFn;
        diagInfo->pContext = (void *)(diagInfo+1);
        memcpy((void *)(diagInfo->pContext), pContext, contextLength);

        //Simulate having do things with the store and return asyncly
        __sync_fetch_and_add(&(ismEngine_serverGlobal.TimerEventsRequested), 1);

        (void)ism_common_setTimerOnce(ISM_TIMER_HIGH,
                                      edia_completeEchoAsyncDiagnostics,
                                      diagInfo, 20);

        rc = ISMRC_AsyncCompletion;
    }

mod_exit:

    ieut_jsonReleaseJSONBuffer(&buffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief  Memory details
///
/// Return the memory detail information in a JSON format
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
/// @param[in]     name               Optional name to use in the json if embedding into other json object
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t edia_modeMemoryDetails(ieutThreadData_t *pThreadData,
                               const char *mode,
                               const char *args,
                               char **pDiagnosticsOutput,
                               void *pContext,
                               size_t contextLength,
                               ismEngine_CompletionCallback_t  pCallbackFn,
                               const char *name)
{
    int32_t rc = OK;
    char xbuf[2048];
    ieutJSONBuffer_t buffer = {true, {xbuf, sizeof(xbuf)}};

    ieutTRACEL(pThreadData, contextLength, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ismEngine_MemoryStatistics_t memoryStats;

    rc = ism_engine_getMemoryStatistics(&memoryStats);
    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    iemnMessagingStatistics_t internalStats;
    iemn_getMessagingStatistics(pThreadData, &internalStats);

    ieut_jsonStartObject(&buffer, name);

    // Overall / System information
    ieut_jsonAddBool(&buffer, "MemoryCGroupInUse", memoryStats.MemoryCGroupInUse);
    ieut_jsonAddUInt64(&buffer, "MemoryTotalBytes", memoryStats.MemoryTotalBytes);
    ieut_jsonAddUInt64(&buffer, "MemoryFreeBytes", memoryStats.MemoryFreeBytes);
    ieut_jsonAddDouble(&buffer, "MemoryFreePercent", memoryStats.MemoryFreePercent);
    ieut_jsonAddUInt64(&buffer, "ServerVirtualMemoryBytes", memoryStats.ServerVirtualMemoryBytes);
    ieut_jsonAddUInt64(&buffer, "ServerResidentSetBytes", memoryStats.ServerResidentSetBytes);
    ieut_jsonAddUInt64(&buffer, "ResourceSetMemoryBytes", internalStats.ResourceSetMemoryBytes);

    // Group breakdown
    ieut_jsonStartObject(&buffer, "Groups");
    ieut_jsonAddUInt64(&buffer, "GroupMessagePayloads", memoryStats.GroupMessagePayloads);
    ieut_jsonAddUInt64(&buffer, "GroupPublishSubscribe", memoryStats.GroupPublishSubscribe);
    ieut_jsonAddUInt64(&buffer, "GroupDestinations", memoryStats.GroupDestinations);
    ieut_jsonAddUInt64(&buffer, "GroupCurrentActivity", memoryStats.GroupCurrentActivity);
    ieut_jsonAddUInt64(&buffer, "GroupRecovery", memoryStats.GroupRecovery);
    ieut_jsonAddUInt64(&buffer, "GroupClientStates", memoryStats.GroupClientStates);
    ieut_jsonEndObject(&buffer);

    // Type breakdown
    ieut_jsonStartObject(&buffer, "Types");
    ieut_jsonAddUInt64(&buffer, "MessagePayloads", memoryStats.MessagePayloads);
    ieut_jsonAddUInt64(&buffer, "TopicAnalysis", memoryStats.TopicAnalysis);
    ieut_jsonAddUInt64(&buffer, "SubscriberTree", memoryStats.SubscriberTree);
    ieut_jsonAddUInt64(&buffer, "NamedSubscriptions", memoryStats.NamedSubscriptions);
    ieut_jsonAddUInt64(&buffer, "SubscriberCache", memoryStats.SubscriberCache);
    ieut_jsonAddUInt64(&buffer, "SubscriberQuery", memoryStats.SubscriberQuery);
    ieut_jsonAddUInt64(&buffer, "TopicTree", memoryStats.TopicTree);
    ieut_jsonAddUInt64(&buffer, "TopicQuery", memoryStats.TopicQuery);
    ieut_jsonAddUInt64(&buffer, "ClientState", memoryStats.ClientState);
    ieut_jsonAddUInt64(&buffer, "CallbackContext", memoryStats.CallbackContext);
    ieut_jsonAddUInt64(&buffer, "PolicyInfo", memoryStats.PolicyInfo);
    ieut_jsonAddUInt64(&buffer, "QueueNamespace", memoryStats.QueueNamespace);
    ieut_jsonAddUInt64(&buffer, "SimpleQueueDefns", memoryStats.SimpleQueueDefns);
    ieut_jsonAddUInt64(&buffer, "SimpleQueuePages", memoryStats.SimpleQueuePages);
    ieut_jsonAddUInt64(&buffer, "IntermediateQueueDefns", memoryStats.IntermediateQueueDefns);
    ieut_jsonAddUInt64(&buffer, "IntermediateQueuePages", memoryStats.IntermediateQueuePages);
    ieut_jsonAddUInt64(&buffer, "MulticonsumerQueueDefns", memoryStats.MulticonsumerQueueDefns);
    ieut_jsonAddUInt64(&buffer, "MulticonsumerQueuePages", memoryStats.MulticonsumerQueuePages);
    ieut_jsonAddUInt64(&buffer, "LockManager", memoryStats.LockManager);
    ieut_jsonAddUInt64(&buffer, "MQConnectivityRecords", memoryStats.MQConnectivityRecords);
    ieut_jsonAddUInt64(&buffer, "RecoveryTable", memoryStats.RecoveryTable);
    ieut_jsonAddUInt64(&buffer, "ExternalObjects", memoryStats.ExternalObjects);
    ieut_jsonAddUInt64(&buffer, "LocalTransactions", memoryStats.LocalTransactions);
    ieut_jsonAddUInt64(&buffer, "GlobalTransactions", memoryStats.GlobalTransactions);
    ieut_jsonAddUInt64(&buffer, "MonitoringData", memoryStats.MonitoringData);
    ieut_jsonAddUInt64(&buffer, "NotificationData", memoryStats.NotificationData);
    ieut_jsonAddUInt64(&buffer, "MessageExpiryData", memoryStats.MessageExpiryData);
    ieut_jsonAddUInt64(&buffer, "RemoteServerData", memoryStats.RemoteServerData);
    ieut_jsonAddUInt64(&buffer, "CommitData", memoryStats.CommitData);
    ieut_jsonAddUInt64(&buffer, "UnneededRetainedMsgs", memoryStats.UnneededRetainedMsgs);
    ieut_jsonAddUInt64(&buffer, "ExpiringGetData", memoryStats.ExpiringGetData);
    ieut_jsonAddUInt64(&buffer, "Diagnostics", memoryStats.Diagnostics);
    ieut_jsonAddUInt64(&buffer, "UnneededBufferedMsgs", memoryStats.UnneededBufferedMsgs);
    ieut_jsonAddUInt64(&buffer, "JobQueues", memoryStats.JobQueues);
    ieut_jsonAddUInt64(&buffer, "ResourceSetStats", memoryStats.ResourceSetStats);
    ieut_jsonAddUInt64(&buffer, "DeferredFreeLists", memoryStats.DeferredFreeLists);
    ieut_jsonEndObject(&buffer);

    ieut_jsonEndObject(&buffer);

    // Check no new values have been added to the structure
    assert(offsetof(ismEngine_MemoryStatistics_t, DeferredFreeLists) + sizeof(memoryStats.DeferredFreeLists) == sizeof(memoryStats));

    char *outbuf = ieut_jsonGenerateOutputBuffer(pThreadData, &buffer, iemem_diagnostics);

    if (outbuf == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    *pDiagnosticsOutput = outbuf;

mod_exit:

    ieut_jsonReleaseJSONBuffer(&buffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief  Memory Trim
///
/// Ask glibc to return some of the memory we have freed to the OS
///
/// @param[in]     mode               Type of diagnostics requested (currently ignored)
/// @param[in]     args               Arguments to the diagnostics collection (currently ignored)
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t edia_modeMemoryTrim(ieutThreadData_t *pThreadData,
                            const char *mode,
                            const char *args,
                            char **pDiagnosticsOutput,
                            void *pContext,
                            size_t contextLength,
                            ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;
    char xbuf[2048];
    ieutJSONBuffer_t buffer = {true, {xbuf, sizeof(xbuf)}};

    ieutTRACEL(pThreadData, contextLength, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ismEngine_MemoryStatistics_t beforeMemoryStats;

    rc = ism_engine_getMemoryStatistics(&beforeMemoryStats);
    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    //Leave 0.2 % of total memory for fast allocations
    malloc_trim((size_t)(beforeMemoryStats.MemoryTotalBytes*0.002));

    ismEngine_MemoryStatistics_t afterMemoryStats;

    rc = ism_engine_getMemoryStatistics(&afterMemoryStats);
    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    ieut_jsonStartObject(&buffer, NULL);

    // Overall / System information - Before the Trim
    ieut_jsonAddBool(&buffer,   "BeforeMemoryCGroupInUse", beforeMemoryStats.MemoryCGroupInUse);
    ieut_jsonAddUInt64(&buffer, "BeforeMemoryTotalBytes", beforeMemoryStats.MemoryTotalBytes);
    ieut_jsonAddUInt64(&buffer, "BeforeMemoryFreeBytes", beforeMemoryStats.MemoryFreeBytes);
    ieut_jsonAddDouble(&buffer, "BeforeMemoryFreePercent", beforeMemoryStats.MemoryFreePercent);
    ieut_jsonAddUInt64(&buffer, "BeforeServerVirtualMemoryBytes", beforeMemoryStats.ServerVirtualMemoryBytes);
    ieut_jsonAddUInt64(&buffer, "BeforeServerResidentSetBytes", beforeMemoryStats.ServerResidentSetBytes);

    // Overall / System information - After the Trim
    ieut_jsonAddBool(&buffer,   "AfterMemoryCGroupInUse", afterMemoryStats.MemoryCGroupInUse);
    ieut_jsonAddUInt64(&buffer, "AfterMemoryTotalBytes", afterMemoryStats.MemoryTotalBytes);
    ieut_jsonAddUInt64(&buffer, "AfterMemoryFreeBytes", afterMemoryStats.MemoryFreeBytes);
    ieut_jsonAddDouble(&buffer, "AfterMemoryFreePercent", afterMemoryStats.MemoryFreePercent);
    ieut_jsonAddUInt64(&buffer, "AfterServerVirtualMemoryBytes", afterMemoryStats.ServerVirtualMemoryBytes);
    ieut_jsonAddUInt64(&buffer, "AfterServerResidentSetBytes", afterMemoryStats.ServerResidentSetBytes);

    ieut_jsonEndObject(&buffer);

    char *outbuf = ieut_jsonGenerateOutputBuffer(pThreadData, &buffer, iemem_diagnostics);

    if (outbuf == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    *pDiagnosticsOutput = outbuf;

mod_exit:

    ieut_jsonReleaseJSONBuffer(&buffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief  edia_modeAsyncCBStats
///
/// Find out how many of the Callbacks that signal the end of an asynchronous
/// store commit are queueed
///
/// @param[in]     mode               Type of diagnostics requested (currently ignored)
/// @param[in]     args               Arguments to the diagnostics collection (currently ignored)
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t edia_modeAsyncCBStats(ieutThreadData_t *pThreadData,
                            const char *mode,
                            const char *args,
                            char **pDiagnosticsOutput,
                            void *pContext,
                            size_t contextLength,
                            ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;
    char xbuf[2048];
    ieutJSONBuffer_t buffer = {true, {xbuf, sizeof(xbuf)}};
    uint32_t numThreads = 20; //Initially guess there are no more than 20 threads

    ieutTRACEL(pThreadData, contextLength, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ismStore_AsyncThreadCBStats_t *pCBThreadStats = iemem_calloc(pThreadData,
                                                                 IEMEM_PROBE(iemem_diagnostics, 5),
                                                                 1,
                                                                 numThreads * sizeof(ismStore_AsyncThreadCBStats_t));


    if (pCBThreadStats == NULL)
    {
        rc = ISMRC_AllocateError;
        goto mod_exit;
    }

    uint32_t totalReadyCBs = 0;
    uint32_t totalWaitingCBs = 0;

    rc = ism_store_getAsyncCBStats(&totalReadyCBs, &totalWaitingCBs,
            &numThreads,
            pCBThreadStats);

    if ( rc == ISMRC_ArgNotValid)
    {
        ieutTRACEL(pThreadData, numThreads,  ENGINE_UNUSUAL_TRACE, "ism_store_getAsyncCBStats wants memory for %u threads\n",
                    numThreads);

        const uint32_t MaxBelivableNumAsyncCBThreads=1024;
        if (numThreads <= MaxBelivableNumAsyncCBThreads)
        {
            ismStore_AsyncThreadCBStats_t *resizedStats = iemem_realloc(pThreadData,
                                                                        IEMEM_PROBE(iemem_diagnostics, 6),
                                                                        pCBThreadStats,
                                                                        numThreads * sizeof(ismStore_AsyncThreadCBStats_t));

            if (resizedStats != NULL)
            {
                pCBThreadStats = resizedStats;
            }
            else
            {
                rc = ISMRC_AllocateError;
                goto mod_exit;
            }
            rc = ism_store_getAsyncCBStats(&totalReadyCBs, &totalWaitingCBs,
                        &numThreads,
                        pCBThreadStats);
        }
        else
        {
            ieutTRACEL(pThreadData, numThreads,  ENGINE_WORRYING_TRACE, "Refusing to allocate memory for %u threads\n",
                        numThreads);
            rc = ISMRC_AllocateError;
        }
    }

    if (rc == OK)
    {
        ieut_jsonStartObject(&buffer, NULL);

        // First "global" stats that are returned "live"
        ieut_jsonAddUInt32(&buffer, "TotalReadyCallbacks", totalReadyCBs);
        ieut_jsonAddUInt32(&buffer, "TotalWaitingCallbacks", totalWaitingCBs);

        //Go through each thread - if stats are enabled
        if (numThreads > 0)
        {
            ieut_jsonStartArray(&buffer, "AsyncCallbackThreads");
            ismStore_AsyncThreadCBStats_t *pCurrThreadStats = pCBThreadStats;

            ism_ts_t *ts = ism_common_openTimestamp(ISM_TZF_UTC);

            for (uint32_t i = 0; i < numThreads; i++)
            {
                ieutTRACEL(pThreadData, i,  ENGINE_WORRYING_TRACE, "Doing thread %u out of %u threads\n",
                    i, numThreads);

                ieut_jsonStartObject(&buffer, NULL);
                ieut_jsonAddUInt32(&buffer, "ThreadId", i);
                ieut_jsonAddUInt32(&buffer, "numProcessingCallbacks", pCurrThreadStats->numProcessingCallbacks);
                ieut_jsonAddUInt32(&buffer, "numReadyCallbacks", pCurrThreadStats->numReadyCallbacks);
                ieut_jsonAddUInt64(&buffer, "numWaitingCallbacks", pCurrThreadStats->numWaitingCallbacks);

                ieut_jsonStartObject(&buffer, "StatsPeriod");

                if (ts != NULL)
                {
                    ism_time_t endtime = ism_common_convertTSCToApproxTime(pCurrThreadStats->statsTime);

                    char timeString[80];
                    ism_common_setTimestamp(ts,  endtime);
                    ism_common_formatTimestamp(ts, timeString, 80, 6, ISM_TFF_ISO8601);
                    ieut_jsonAddString(&buffer, "EndTime", timeString);
                }

                ieut_jsonAddDouble(&buffer, "PeriodSeconds", pCurrThreadStats->periodLength);
                ieut_jsonAddUInt32(&buffer, "NumCallbacks", pCurrThreadStats->numCallbacks);
                ieut_jsonAddUInt32(&buffer, "NumLoops", pCurrThreadStats->numLoops);
                ieut_jsonEndObject(&buffer); //statsPeriod
                ieut_jsonEndObject(&buffer);

                pCurrThreadStats++;
            }
            ieut_jsonEndArray(&buffer);

            if (ts != NULL)
            {
                ism_common_closeTimestamp(ts);
            }
        }
       ieut_jsonEndObject(&buffer); //top level

       char *outbuf = ieut_jsonGenerateOutputBuffer(pThreadData, &buffer, iemem_diagnostics);

       if (outbuf == NULL)
       {
           rc = ISMRC_AllocateError;
           ism_common_setError(rc);
           goto mod_exit;
       }
       ieutTRACEL(pThreadData, outbuf,  ENGINE_WORRYING_TRACE, "outbuf is %p\n",
           outbuf);
       *pDiagnosticsOutput = outbuf;
    }

mod_exit:
    iemem_free(pThreadData, iemem_diagnostics, pCBThreadStats);
    ieut_jsonReleaseJSONBuffer(&buffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


/// @brief Context passed to the client state traversal callback for each clientState
typedef struct tag_ediaDumpClientStatesCallbackContext_t
{
    ieutJSONBuffer_t buffer;
    ism_time_t startTime;
    uint32_t tableGeneration;
    uint32_t emptyChainCount;
    uint32_t chainNumber;
    uint32_t totalClientCount;
    uint32_t startChain;
    uint32_t clientCount;
} ediaDumpClientStatesCallbackContext_t;

bool ediaClientStateTraversalCallback(ieutThreadData_t *pThreadData,
                                      ismEngine_ClientState_t *pClient,
                                      void *pContext)
{
    ediaDumpClientStatesCallbackContext_t *context = (ediaDumpClientStatesCallbackContext_t *)pContext;

    ieut_jsonStartObject(&context->buffer, NULL);
    ieut_jsonAddString(&context->buffer, "ClientId", pClient->pClientId);
    ieut_jsonAddUInt32(&context->buffer, "ProtocolId", pClient->protocolId);
    ieut_jsonAddBool(&context->buffer, "Zombie", pClient->OpState == iecsOpStateZombie);
    ieut_jsonAddBool(&context->buffer, "Durable", pClient->Durability == iecsDurable);
    ieut_jsonAddStoreHandle(&context->buffer, "StoreCSR", pClient->hStoreCSR);
    ieut_jsonAddStoreHandle(&context->buffer, "StoreCPR", pClient->hStoreCPR);
    ieut_jsonAddPointer(&context->buffer, "Address", pClient);
    if (pClient->pThief != NULL)
    {
        ieut_jsonAddPointer(&context->buffer, "Thief", pClient->pThief);
    }
    ieut_jsonAddUInt64(&context->buffer, "LastConnectedTime", pClient->LastConnectedTime);
    ieut_jsonAddUInt64(&context->buffer, "ExpiryTime", pClient->ExpiryTime);
    ieut_jsonAddString(&context->buffer, "ResourceSetID", (char *)iere_getResourceSetIdentifier(pClient->resourceSet));
    ieut_jsonEndObject(&context->buffer);

    context->clientCount += 1;

    return true;
}

//****************************************************************************
/// @brief  Return the diagnostic file path to use for a given component and
/// name
///
/// @param[in]     componentName      The component producing the file
/// @param[in]     fileName           The file name required
/// @param[out]    filePath           Where to return the created path.
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
static int32_t edia_createFilePath(ieutThreadData_t *pThreadData,
                                   char *componentName,
                                   char *fileName,
                                   char **filePath)
{
    int32_t rc = OK;

    if (strchr(fileName, '/') != NULL)
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
        goto mod_exit;
    }

    assert(componentName != NULL);
    assert(strchr(componentName, '/') == NULL);

    // Get the diagnostics directory
    const char *diagDir = ism_common_getStringConfig("DiagDir");

    if (diagDir == NULL)
    {
        ieutTRACEL(pThreadData, diagDir, ENGINE_INTERESTING_TRACE, "DiagDir not found\n");
        rc = ISMRC_Error;
        ism_common_setError(rc);
        goto mod_exit;
    }

    *filePath = iemem_malloc(pThreadData,
                             IEMEM_PROBE(iemem_diagnostics, 1),
                             strlen(diagDir)+strlen(componentName)+strlen(fileName)+3);

    if (*filePath == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Make sure the directory exists
    sprintf(*filePath, "%s/", diagDir);
    int osrc = mkdir(*filePath, S_IRWXU | S_IRGRP | S_IXGRP);

    // Create a componentName subdirectory
    if (osrc == 0 || errno == EEXIST)
    {
        strcat(*filePath, componentName);
        strcat(*filePath, "/");
        osrc = mkdir(*filePath, S_IRWXU | S_IRGRP | S_IXGRP);
    }

    if (osrc == -1 && errno != EEXIST)
    {
        ieutTRACEL(pThreadData, errno, ENGINE_INTERESTING_TRACE, "Failed to create directory '%s' errno=%d\n", *filePath, errno);
        iemem_free(pThreadData, iemem_diagnostics, *filePath);
        *filePath = NULL;
        rc = ISMRC_Error;
        ism_common_setError(rc);
        goto mod_exit;
    }

    strcat(*filePath, fileName);

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief  Create an encrypted Diagnostic file with the specified filename
///
/// @param[in,out] filePath           Full path specification - NULL means create
///                                   using fileName.
/// @param[in]     componentName      Component name specified (no path allowed)
/// @param[in]     fileName           Name specified (no path allowed)
/// @param[out]    password           Password to use for encryption
/// @param[in,out] diagFile           The created diagnostic file
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t edia_createEncryptedDiagnosticFile(ieutThreadData_t *pThreadData,
                                           char **filePath,
                                           char *componentName,
                                           char *fileName,
                                           char *password,
                                           ieieEncryptedFileHandle_t *diagFile)
{
    int32_t rc = OK;

    assert(filePath != NULL);
    assert(diagFile != NULL);

    char *localFilePath = *filePath;
    ieieEncryptedFileHandle_t localDiagFile = *diagFile;

    ieutTRACEL(pThreadData, localDiagFile, ENGINE_FNC_TRACE, FUNCTION_ENTRY "fileName='%s' *filePath=%p(%s) *diagFile=%p\n",
               __func__, fileName, localFilePath, localFilePath ? localFilePath : "NULL", localDiagFile);

    // Already have a diagFile - finish writing it
    if (localDiagFile != NULL)
    {
        assert(localFilePath != NULL);

        rc = ieie_finishWritingEncryptedFile(pThreadData, localDiagFile);

        if (rc != OK)
        {
            ism_common_setError(rc);
            goto mod_exit;
        }

        *diagFile = NULL;
    }
    // Don't have a filePath yet - need to create one
    else if (localFilePath == NULL)
    {
        rc = edia_createFilePath(pThreadData, componentName, fileName, &localFilePath);

        if (rc != OK) goto mod_exit;
    }

    assert(localFilePath != NULL);
    assert(*diagFile == NULL);

    localDiagFile = ieie_createEncryptedFile(pThreadData, iemem_diagnostics, localFilePath, password);

    if (localDiagFile == NULL)
    {
        iemem_free(pThreadData, iemem_diagnostics, localFilePath);
        *filePath = NULL;
        rc = ISMRC_FileCorrupt;
        ism_common_setError(rc);
        goto mod_exit;
    }

    *filePath = localFilePath;
    *diagFile = localDiagFile;

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d *filePath=%p(%s) *diagFile=%p\n",
               __func__, rc, *filePath, *filePath ? *filePath : "NULL", *diagFile);
    return rc;
}

//****************************************************************************
/// @brief  Create a Diagnostic file with the specified filename
///
/// @param[in,out] filePath           Full path specification - NULL means create
///                                   using fileName.
/// @param[in]     componentName      Component name specified (no path allowed)
/// @param[in]     fileName           Name specified (no path allowed)
/// @param[in,out] diagFile           The created diagnostic file
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
static int32_t edia_createDiagnosticFile(ieutThreadData_t *pThreadData,
                                         char **filePath,
                                         char *componentName,
                                         char *fileName,
                                         FILE **diagFile)
{
    int32_t rc = OK;

    assert(filePath != NULL);
    assert(diagFile != NULL);

    char *localFilePath = *filePath;
    FILE *localDiagFile = *diagFile;

    ieutTRACEL(pThreadData, localDiagFile, ENGINE_FNC_TRACE, FUNCTION_ENTRY "fileName='%s' *filePath=%p(%s) *diagFile=%p\n",
               __func__, fileName, localFilePath, localFilePath ? localFilePath : "NULL", localDiagFile);

    // Already have a diagFile - finish writing it
    if (localDiagFile != NULL)
    {
        rc = fclose(localDiagFile);

        if (rc != 0)
        {
            rc = ISMRC_Error;
            ism_common_setError(rc);
            goto mod_exit;
        }

        *diagFile = NULL;
    }
    // Don't have a filePath yet - need to create one
    else if (localFilePath == NULL)
    {
        rc = edia_createFilePath(pThreadData, componentName, fileName, &localFilePath);

        if (rc != OK) goto mod_exit;
    }

    assert(localFilePath != NULL);
    assert(*diagFile == NULL);

    localDiagFile = fopen(localFilePath, "w");

    if (localDiagFile == NULL)
    {
        iemem_free(pThreadData, iemem_diagnostics, localFilePath);
        *filePath = NULL;
        rc = ISMRC_FileCorrupt;
        ism_common_setError(rc);
        goto mod_exit;
    }

    *filePath = localFilePath;
    *diagFile = localDiagFile;

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d *filePath=%p(%s) *diagFile=%p\n",
               __func__, rc, *filePath, *filePath ? *filePath : "NULL", *diagFile);
    return rc;
}

//****************************************************************************
/// @brief  Dump client states
///
/// Dump the information about client states currently in the client state table
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @remarks args are very rudimentary, we expect to see 2 args a filename
///          and a password in that order (they cannot contain space).
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t edia_modeDumpClientStates(ieutThreadData_t *pThreadData,
                                  const char *mode,
                                  const char *args,
                                  char **pDiagnosticsOutput,
                                  void *pContext,
                                  size_t contextLength,
                                  ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;

    char xbuf[2048];
    ediaDumpClientStatesCallbackContext_t context = {{true, {xbuf, sizeof(xbuf)}}, 0};

    ieutTRACEL(pThreadData, contextLength, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    char *filePath = NULL;
    char **parsedArgs = NULL;
    ieieEncryptedFileHandle_t diagFile = NULL;

    // We expect exactly two positional arguments, filename and password.
    rc = edia_parseSimpleArgs(pThreadData, args, 2, 2, &parsedArgs);

    if (rc != OK) goto mod_exit;

    char *fileName = parsedArgs[0];
    char *password = parsedArgs[1];

    // Use this return code to initialize the context
    rc = ISMRC_ClientTableGenMismatch;

    do
    {
        // The generation has changed... we need to start over.
        if (rc == ISMRC_ClientTableGenMismatch)
        {
            context.startTime = ism_common_currentTimeNanos();
            context.tableGeneration = 0;
            context.chainNumber = 1; // 1st chain is chain number 1
            context.totalClientCount = 0;
            context.emptyChainCount = 0;
            context.startChain = 0;

            rc = edia_createEncryptedDiagnosticFile(pThreadData,
                                                    &filePath,
                                                    ediaCOMPONENTNAME_ENGINE,
                                                    fileName,
                                                    password,
                                                    &diagFile);

            if (rc != OK)
            {
                diagFile = NULL;
                ism_common_setError(rc);
                goto mod_exit;
            }

            assert(filePath != NULL);
            assert(diagFile != NULL);
            assert(context.buffer.buffer.used == 0);

            // Write a Header
            ieut_jsonStartObject(&context.buffer, NULL);
            ieut_jsonAddString(&context.buffer, "Mode", (char *)mode);
            ieut_jsonAddString(&context.buffer, "Filename", fileName);
            ieut_jsonAddUInt64(&context.buffer, "StartTime", context.startTime);
            ieut_jsonAddUInt32(&context.buffer, "TableGeneration", context.tableGeneration);
            ieut_jsonEndObject(&context.buffer);
            ieut_jsonNullTerminateJSONBuffer(&context.buffer);

            rc = ieie_exportData(pThreadData,
                                 diagFile,
                                 ieieDATATYPE_DIAGCLIENTDUMPHEADER,
                                 context.startTime,
                                 context.buffer.buffer.used,
                                 context.buffer.buffer.buf);

            if (rc != OK)
            {
                ism_common_setError(rc);
                goto mod_exit;
            }

            ieut_jsonResetJSONBuffer(&context.buffer);
        }
        else
        {
            context.chainNumber++;
            context.startChain++;
        }

        context.clientCount = 0;

        ieut_jsonStartObject(&context.buffer, NULL);
        ieut_jsonAddUInt32(&context.buffer, "ChainNumber", context.chainNumber);
        ieut_jsonStartArray(&context.buffer, "Entries");

        // Go to the next chain
        rc = iecs_traverseClientStateTable(pThreadData,
                                           &context.tableGeneration, context.startChain, 1, NULL,
                                           ediaClientStateTraversalCallback,
                                           &context);

        if (context.clientCount != 0)
        {
            context.totalClientCount += context.clientCount;

            ieut_jsonEndArray(&context.buffer);
            ieut_jsonAddUInt32(&context.buffer, "ClientCount", context.clientCount);
            ieut_jsonEndObject(&context.buffer);
            ieut_jsonNullTerminateJSONBuffer(&context.buffer);

            // Write this chain to the output file
            int32_t export_rc = ieie_exportData(pThreadData,
                                                diagFile,
                                                ieieDATATYPE_DIAGCLIENTDUMPCHAIN,
                                                context.chainNumber,
                                                context.buffer.buffer.used,
                                                context.buffer.buffer.buf);

            if (export_rc != OK)
            {
                rc = export_rc;
                ism_common_setError(rc);
                goto mod_exit;
            }
        }
        else
        {
            context.emptyChainCount++;
        }

        ieut_jsonResetJSONBuffer(&context.buffer);
    }
    while(rc == ISMRC_MoreChainsAvailable || rc == ISMRC_ClientTableGenMismatch);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    assert(diagFile != NULL);

    assert(context.buffer.buffer.used == 0);

    // Write a Footer
    ieut_jsonStartObject(&context.buffer, NULL);
    ieut_jsonAddString(&context.buffer, "Mode", (char *)mode);
    ieut_jsonAddUInt64(&context.buffer, "EndTime", ism_common_currentTimeNanos());
    ieut_jsonAddUInt32(&context.buffer, "ChainCount", context.chainNumber);
    ieut_jsonAddUInt32(&context.buffer, "EmptyChainCount", context.emptyChainCount);
    ieut_jsonAddUInt32(&context.buffer, "TotalClientCount", context.totalClientCount);
    ieut_jsonEndObject(&context.buffer);

    ieut_jsonNullTerminateJSONBuffer(&context.buffer);

    rc = ieie_exportData(pThreadData,
                         diagFile,
                         ieieDATATYPE_DIAGCLIENTDUMPFOOTER,
                         context.startTime,
                         context.buffer.buffer.used,
                         context.buffer.buffer.buf);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    ieut_jsonResetJSONBuffer(&context.buffer);

    ieut_jsonStartObject(&context.buffer, NULL);
    ieut_jsonAddString(&context.buffer, "FilePath", filePath);
    ieut_jsonAddUInt32(&context.buffer, "ChainCount", context.chainNumber);
    ieut_jsonAddUInt32(&context.buffer, "TotalClientCount", context.totalClientCount);
    ieut_jsonAddInt32(&context.buffer, "rc", rc);
    ieut_jsonEndObject(&context.buffer);

    char *outbuf = ieut_jsonGenerateOutputBuffer(pThreadData, &context.buffer, iemem_diagnostics);

    if (outbuf == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    *pDiagnosticsOutput = outbuf;

mod_exit:

    if (filePath != NULL) iemem_free(pThreadData, iemem_diagnostics, filePath);
    if (diagFile != NULL) (void)ieie_finishWritingEncryptedFile(pThreadData, diagFile);
    if (parsedArgs != NULL) iemem_free(pThreadData, iemem_diagnostics, parsedArgs);

    ieut_jsonReleaseJSONBuffer(&context.buffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

// Context used when traversing tables to locate owners
typedef struct tag_ediaOwnerCountsCallbackContext_t
{
    uint32_t tableGeneration;
    uint32_t chainNumber;
    uint32_t totalClientCount;
    uint32_t startChain;
    uint32_t clientOwnerCount;
    uint32_t subOwnerCount;
} ediaOwnerCountsCallbackContext_t;

bool edia_countClientOwnersTraversalCallback(ieutThreadData_t *pThreadData,
                                             ismEngine_ClientState_t *pClient,
                                             void *pContext)
{
    ediaOwnerCountsCallbackContext_t *context = (ediaOwnerCountsCallbackContext_t *)pContext;

    if (pClient->hStoreCSR != ismSTORE_NULL_HANDLE)
    {
        context->clientOwnerCount += 1;
    }

    context->totalClientCount += 1;

    return true;
}

bool edia_countSubscriptionOwnersTraversalCallback(ieutThreadData_t *pThreadData,
                                                   ismEngine_Subscription_t *pSubscription,
                                                   void *pContext)
{
    ediaOwnerCountsCallbackContext_t *context = (ediaOwnerCountsCallbackContext_t *)pContext;

    ismStore_Handle_t hStoreDefn = ieq_getDefnHdl(pSubscription->queueHandle);

    if (hStoreDefn != ismSTORE_NULL_HANDLE)
    {
        context->subOwnerCount += 1;
    }

    return true;
}

//****************************************************************************
/// @brief  Owner Counts
///
/// Return the counts of store Owners currently in-memory
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t edia_modeOwnerCounts(ieutThreadData_t *pThreadData,
                             const char *mode,
                             const char *args,
                             char **pDiagnosticsOutput,
                             void *pContext,
                             size_t contextLength,
                             ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;
    char xbuf[2048];
    ieutJSONBuffer_t buffer = {true, {xbuf, sizeof(xbuf)}};

    ediaOwnerCountsCallbackContext_t context = {0};

    ieutTRACEL(pThreadData, contextLength, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Clients...

    // Use this return code to initialize the context
    rc = ISMRC_ClientTableGenMismatch;

    do
    {
        // The generation has changed... we need to start over.
        if (rc == ISMRC_ClientTableGenMismatch)
        {
            context.tableGeneration = 0;
            context.chainNumber = 1; // 1st chain is chain number 1
            context.totalClientCount = 0;
            context.clientOwnerCount = 0;
            context.startChain = 0;
        }
        else
        {
            context.chainNumber++;
            context.startChain++;
        }

        // Go to the next chain
        rc = iecs_traverseClientStateTable(pThreadData,
                                           &context.tableGeneration, context.startChain, 1, NULL,
                                           edia_countClientOwnersTraversalCallback,
                                           &context);
    }
    while(rc == ISMRC_MoreChainsAvailable || rc == ISMRC_ClientTableGenMismatch);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    // Subscriptions...
    iett_traverseSubscriptions(pThreadData,
                               edia_countSubscriptionOwnersTraversalCallback,
                               &context);

    // TODO: Topics...
    // TODO: RemoteServers...
    // TODO: Queues...

    ieut_jsonStartObject(&buffer, NULL);
    ieut_jsonAddUInt32(&buffer, ediaVALUE_OUTPUT_CLIENTOWNERCOUNT, context.clientOwnerCount);
    ieut_jsonAddUInt32(&buffer, ediaVALUE_OUTPUT_SUBOWNERCOUNT, context.subOwnerCount);
    ieut_jsonEndObject(&buffer);

    char *outbuf = ieut_jsonGenerateOutputBuffer(pThreadData, &buffer, iemem_diagnostics);

    if (outbuf == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    *pDiagnosticsOutput = outbuf;

mod_exit:

    ieut_jsonReleaseJSONBuffer(&buffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief  Dump the trace history from all engine threads
///
/// Dump the engine trace history for all engine threads
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @remarks args are very rudimentary, we expect to see 2 args a filename
///          and a password in that order (they cannot contain space).
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t edia_modeDumpTraceHistory(ieutThreadData_t *pThreadData,
                                  const char *mode,
                                  const char *args,
                                  char **pDiagnosticsOutput,
                                  void *pContext,
                                  size_t contextLength,
                                  ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, contextLength, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    char *filePath = NULL;
    char **parsedArgs = NULL;

    char xbuf[2048];
    ieutJSONBuffer_t buffer = {true, {xbuf, sizeof(xbuf)}};

    // We expect exactly two positional arguments, filename and password.
    rc = edia_parseSimpleArgs(pThreadData, args, 2, 2, &parsedArgs);

    if (rc != OK) goto mod_exit;

    char *fileName = parsedArgs[0];
    char *password = parsedArgs[1];

    rc = ietd_dumpInMemoryTrace(pThreadData,
                                fileName,
                                password,
                                &filePath);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    ieut_jsonResetJSONBuffer(&buffer);

    ieut_jsonStartObject(&buffer, NULL);
    ieut_jsonAddString(&buffer, "FilePath", filePath);
    ieut_jsonEndObject(&buffer);

    char *outbuf = ieut_jsonGenerateOutputBuffer(pThreadData, &buffer, iemem_diagnostics);

    if (outbuf == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    *pDiagnosticsOutput = outbuf;

mod_exit:

    if (filePath != NULL) iemem_free(pThreadData, iemem_diagnostics, filePath);
    if (parsedArgs != NULL) iemem_free(pThreadData, iemem_diagnostics, parsedArgs);

    ieut_jsonReleaseJSONBuffer(&buffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief Callback used to perform dumping of one resourceSet
///
/// @param[in]     pThreadData      Thread data to use
//****************************************************************************
static void edia_dumpResourceSetStats(ieutThreadData_t *pThreadData,
                                      iereResourceSet_t *pResourceSet,
                                      ism_time_t resetTime,
                                      void *context)
{
    FILE *diagFile = (FILE *)context;

    fprintf(diagFile, "%s", pResourceSet->stats.resourceSetIdentifier);

    for(int32_t i=0; i<ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS; i++)
    {
        fprintf(diagFile, ",%ld", pResourceSet->stats.int64Stats[i]);
    }

    fprintf(diagFile, ",%lu\n", resetTime);
}

//****************************************************************************
/// @brief  Dump all resource sets
///
/// Dump the resource sets into a simple comma separated file
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @remarks args are very rudimentary, we expect to see 1 arg a filename.
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t edia_modeDumpResourceSets(ieutThreadData_t *pThreadData,
                                  const char *mode,
                                  const char *args,
                                  char **pDiagnosticsOutput,
                                  void *pContext,
                                  size_t contextLength,
                                  ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, contextLength, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    char *filePath = NULL;
    char **parsedArgs = NULL;

    char xbuf[2048];
    ieutJSONBuffer_t buffer = {true, {xbuf, sizeof(xbuf)}};

    // We expect exactly 1 positional arguments, filename.
    rc = edia_parseSimpleArgs(pThreadData, args, 1, 1, &parsedArgs);

    if (rc != OK) goto mod_exit;

    char *fileName = parsedArgs[0];
    FILE *diagFile = NULL;

    // NOTE: Passing a NULL password, so we'll actually get a
    rc = edia_createDiagnosticFile(pThreadData,
                                   &filePath,
                                   ediaCOMPONENTNAME_ENGINE,
                                   (char *)fileName,
                                   &diagFile);

    if (rc != OK)
    {
        ism_common_setError(rc);
        goto mod_exit;
    }

    iere_enumerateResourceSets(pThreadData, edia_dumpResourceSetStats, diagFile);

    fclose(diagFile);

    ieut_jsonResetJSONBuffer(&buffer);

    ieut_jsonStartObject(&buffer, NULL);
    ieut_jsonAddString(&buffer, "FilePath", filePath);
    ieut_jsonEndObject(&buffer);

    char *outbuf = ieut_jsonGenerateOutputBuffer(pThreadData, &buffer, iemem_diagnostics);

    if (outbuf == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    *pDiagnosticsOutput = outbuf;

mod_exit:

    if (filePath != NULL) iemem_free(pThreadData, iemem_diagnostics, filePath);
    if (parsedArgs != NULL) iemem_free(pThreadData, iemem_diagnostics, parsedArgs);

    ieut_jsonReleaseJSONBuffer(&buffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief  Request a resource set report
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                   to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t edia_modeResourceSetReport(ieutThreadData_t *pThreadData,
                                   const char *mode,
                                   const char *args,
                                   char **pDiagnosticsOutput,
                                   void *pContext,
                                   size_t contextLength,
                                   ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;

    char xbuf[1024];
    ieutJSONBuffer_t buffer = {true, {xbuf, sizeof(xbuf)}};

    ismEngineMonitorType_t monitorType = ismENGINE_MONITOR_ALL_UNSORTED;
    uint32_t maxResults = 10;
    bool resetStats = false;

    ieutTRACEL(pThreadData, contextLength, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    char **simpleArgs = NULL;

    // We expect up to three positional arguments, monitorType, maxResults and whether to reset stats
    rc = edia_parseSimpleArgs(pThreadData, args, 0, 3, &simpleArgs);

    if (rc == OK)
    {
        if (simpleArgs[0] != NULL) monitorType = iere_mapStatTypeToMonitorType(pThreadData, simpleArgs[0], true);
        if (simpleArgs[1] != NULL) maxResults = (uint32_t)strtod(simpleArgs[1], NULL);
        if (simpleArgs[2] != NULL) resetStats = (bool)strtod(simpleArgs[2], NULL);

        iemem_free(pThreadData, iemem_diagnostics, simpleArgs);
    }

    rc = iere_requestResourceSetReport(pThreadData, monitorType, maxResults, resetStats);

    ieut_jsonStartObject(&buffer, NULL);
    ieut_jsonAddString(&buffer, "Mode", (char *)mode);
    ieut_jsonAddString(&buffer, "Args", (char *)args);
    ieut_jsonAddInt32(&buffer, "rc", rc);
    ieut_jsonEndObject(&buffer);

    char *outbuf = ieut_jsonGenerateOutputBuffer(pThreadData, &buffer, iemem_diagnostics);

    if (outbuf == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    *pDiagnosticsOutput = outbuf;

mod_exit:

    ieut_jsonReleaseJSONBuffer(&buffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

// Context used when traversing tables to locate owners
typedef struct tag_ediaSubDetailsCallbackContext_t
{
    const char *matchSubName;
    const char *matchClientId;
    ismEngine_Subscription_t **matchingSubscriptions;
    uint32_t matchingSubCount;
    uint32_t matchingSubCapacity;
    int32_t rc;
} ediaSubDetailsCallbackContext_t;

bool edia_subDetailsTraversalCallback(ieutThreadData_t *pThreadData,
                                      ismEngine_Subscription_t *pSubscription,
                                      void *pContext)
{
    ediaSubDetailsCallbackContext_t *context = (ediaSubDetailsCallbackContext_t *)pContext;

    if ((!context->matchSubName ||
         (pSubscription->subName && ism_common_match(pSubscription->subName, context->matchSubName))) &&
        (!context->matchClientId ||
         (ism_common_match(pSubscription->clientId, context->matchClientId))))
    {
        if (context->matchingSubCount == context->matchingSubCapacity)
        {
            uint32_t newMatchingSubCapacity = context->matchingSubCapacity+100;

            ismEngine_Subscription_t **newMatchingSubscription = iemem_realloc(pThreadData,
                                                                               IEMEM_PROBE(iemem_diagnostics, 2),
                                                                               context->matchingSubscriptions,
                                                                               sizeof(ismEngine_Subscription_t *) * newMatchingSubCapacity);

            if (newMatchingSubscription == NULL)
            {
                context->rc = ISMRC_AllocateError;
                ism_common_setError(context->rc);
            }
            else
            {
                context->matchingSubscriptions = newMatchingSubscription;
                context->matchingSubCapacity = newMatchingSubCapacity;
            }
        }

        if (context->rc == OK)
        {
            iett_acquireSubscriptionReference(pSubscription);
            context->matchingSubscriptions[context->matchingSubCount++] = pSubscription;
        }
    }

    return (context->rc == OK);
}

//****************************************************************************
/// @brief  Return details of subscriptions with specified wildcard names
///
/// Return various useful details about the specified subscription
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                   to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @remarks args are very rudimentary, we expect to see at least 1 arg of the form
/// ClientID:XXX or SubName:XXX where XXX cannot contain spaces, but may include wildcards.
///
/// It is also valid to pass additional args, Filename:XXX and Password:XXX to write
/// the output to a file encrypted with specified password, without a file name
/// the data is sent back in the response to the request.
///
/// @returns OK on successful completion
///          or an ISMRC_ value if there is a problem
//****************************************************************************
int32_t edia_modeSubDetails(ieutThreadData_t *pThreadData,
                            const char *mode,
                            const char *args,
                            char **pDiagnosticsOutput,
                            void *pContext,
                            size_t contextLength,
                            ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;
    ediaSubDetailsCallbackContext_t context = {0};

    ieutTRACEL(pThreadData, contextLength, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    char **parsedArgs = NULL;

    char xbuf[2048];
    ieutJSONBuffer_t buffer = {true, {xbuf, sizeof(xbuf)}};

    // We expect exactly one positional argument a (wildcarded) subName to match
    rc = edia_parseSimpleArgs(pThreadData, args, 1, 0, &parsedArgs);

    if (rc != OK) goto mod_exit;

    const char *fileName = NULL;
    const char *password = "default";
    char *filePath = NULL;
    ieieEncryptedFileHandle_t diagFile = NULL;
    iempMemPoolHandle_t memPool = NULL;

    uint32_t parsedArgCount = 0;
    if (rc == OK)
    {
        while(parsedArgs[parsedArgCount] != NULL)
        {
            if (strcasestr(parsedArgs[parsedArgCount], ediaVALUE_FILTER_CLIENTID) == parsedArgs[parsedArgCount] &&
                parsedArgs[parsedArgCount][strlen(ediaVALUE_FILTER_CLIENTID)] == '=')
            {
                context.matchClientId = parsedArgs[parsedArgCount]+strlen(ediaVALUE_FILTER_CLIENTID)+1;
            }
            else if (strcasestr(parsedArgs[parsedArgCount], ediaVALUE_FILTER_SUBNAME) == parsedArgs[parsedArgCount] &&
                     parsedArgs[parsedArgCount][strlen(ediaVALUE_FILTER_SUBNAME)] == '=')
            {
                context.matchSubName = parsedArgs[parsedArgCount]+strlen(ediaVALUE_FILTER_SUBNAME)+1;
            }
            else if (strcasestr(parsedArgs[parsedArgCount], ediaVALUE_PARM_FILENAME) == parsedArgs[parsedArgCount] &&
                     parsedArgs[parsedArgCount][strlen(ediaVALUE_PARM_FILENAME)] == '=')
            {
                fileName = parsedArgs[parsedArgCount]+strlen(ediaVALUE_PARM_FILENAME)+1;
            }
            else if (strcasestr(parsedArgs[parsedArgCount], ediaVALUE_PARM_PASSWORD) == parsedArgs[parsedArgCount] &&
                    parsedArgs[parsedArgCount][strlen(ediaVALUE_PARM_PASSWORD)] == '=')
            {
               password = parsedArgs[parsedArgCount]+strlen(ediaVALUE_PARM_PASSWORD)+1;
            }

            parsedArgCount++;
        }

        // No search terms specified, complain -- we could return all but we want the caller to think
        // carefully about doing that, so we're going to make them specify an "*".
        if (password == NULL || password[0] == '\0' ||
            (context.matchSubName == NULL && context.matchClientId == NULL))
        {
            rc = ISMRC_InvalidParameter;
        }
        else
        {
            // Wildcard that matches all, don't even bother checking
            if (context.matchSubName && context.matchSubName[0] == '*' && context.matchSubName[1] == '\0')
            {
                context.matchSubName = NULL;
            }

            if (context.matchClientId && context.matchClientId[0] == '*' && context.matchClientId[1] == '\0')
            {
                context.matchClientId = NULL;
            }
        }
    }

    if (rc != OK) goto mod_exit;

    assert(context.rc == OK);

    // Find subscriptions that match the search criteria...
    iett_traverseSubscriptions(pThreadData,
                               edia_subDetailsTraversalCallback,
                               &context);

    rc = context.rc;

    // Create the diagnostic file if one was requested
    if (rc == OK && fileName != NULL)
    {
        rc = edia_createEncryptedDiagnosticFile(pThreadData,
                                                &filePath,
                                                ediaCOMPONENTNAME_ENGINE,
                                                (char *)fileName,
                                                (char *)password,
                                                &diagFile);

        if (rc == OK)
        {
            assert(diagFile != NULL);

            // Write a Header
            ieut_jsonStartObject(&buffer, NULL);
            ieut_jsonAddString(&buffer, "Mode", (char *)mode);
            ieut_jsonAddString(&buffer, "Filename", (char *)fileName);
            ieut_jsonEndObject(&buffer);
            ieut_jsonNullTerminateJSONBuffer(&buffer);

            rc = ieie_exportData(pThreadData,
                                 diagFile,
                                 ieieDATATYPE_DIAGSUBDETAILSHEADER,
                                 0,
                                 buffer.buffer.used,
                                 buffer.buffer.buf);
        }

        if (rc != OK)
        {
            ism_common_setError(rc);
        }

        ieut_jsonResetJSONBuffer(&buffer);
    }

    if (rc == OK)
    {
        rc = iemp_createMemPool(pThreadData,
                                IEMEM_PROBE(iemem_diagnostics, 3),
                                ediaSUBDETAILS_MEMPOOL_RESERVEDMEMBYTES,
                                ediaSUBDETAILS_MEMPOOL_INITIALPAGEBYTES,
                                ediaSUBDETAILS_MEMPOOL_SUBSEQUENTPAGEBYTES,
                                &memPool);

        if (rc != OK)
        {
            ism_common_setError(rc);
        }
        else
        {
            assert(memPool != NULL);
        }
    }

    // Produce the output...
    if (rc == OK)
    {
        const bool writingToFile = (diagFile != NULL);
        char *outbuf = NULL;

        if (!writingToFile) ieut_jsonStartArray(&buffer, NULL);

        ieqConsumerStats_t *consumerStats = NULL;
        size_t consumerCapacity = 0;

        for(uint32_t i=0; i<context.matchingSubCount; i++)
        {
            ismEngine_Subscription_t *curSubscription = context.matchingSubscriptions[i];

            ismQHandle_t queueHandle = curSubscription->queueHandle;
            size_t consumerCount = consumerCapacity;
            int32_t stats_rc;

            do
            {
                if (consumerCount > consumerCapacity)
                {
                    size_t newConsumerCapacity = consumerCapacity + 100;

                    ieqConsumerStats_t *newConsumerStats = iemem_realloc(pThreadData,
                                                                         IEMEM_PROBE(iemem_diagnostics, 4),
                                                                         consumerStats,
                                                                         sizeof(ieqConsumerStats_t) * newConsumerCapacity);

                    if (newConsumerStats == NULL)
                    {
                        stats_rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                        break; // Leave the while loop
                    }

                    consumerStats = newConsumerStats;
                    consumerCapacity = newConsumerCapacity;
                }

                stats_rc = ieq_getConsumerStats(pThreadData,
                                                queueHandle,
                                                memPool,
                                                &consumerCount,
                                                consumerStats);
            }
            while(stats_rc == ISMRC_TooManyConsumers);

            iepiPolicyInfo_t *policy = ieq_getPolicyInfo(queueHandle);

            ieut_jsonStartObject(&buffer, NULL);
            ieut_jsonAddString(&buffer, "SubName", curSubscription->subName);
            const char *configType = iett_getAdminSubscriptionType(curSubscription);
            if (configType != NULL)
            {
                ieut_jsonAddString(&buffer, "ConfigType", (char *)configType);
            }
            ieut_jsonAddString(&buffer, "TopicString", curSubscription->node->topicString);
            ieut_jsonAddString(&buffer, "Owner", curSubscription->clientId);
            ieut_jsonAddHexUInt32(&buffer, "SubOptions", curSubscription->subOptions);
            ieut_jsonAddUInt32(&buffer, "SubID", curSubscription->subId);
            ieut_jsonAddString(&buffer, "ResourceSetID", (char *)iere_getResourceSetIdentifier(curSubscription->resourceSet));
            ieut_jsonAddString(&buffer, "Policy", policy->name);
            ieut_jsonAddUInt32(&buffer, "QType", (uint32_t)ieq_getQType(queueHandle));

            // Queue Statistics
            ismEngine_QueueStatistics_t queueStats;
            ieq_getStats(pThreadData, queueHandle, &queueStats);

            ieut_jsonAddUInt64(&buffer, "BufferedMsgs", queueStats.BufferedMsgs);
            ieut_jsonAddUInt64(&buffer, "BufferedMsgsHWM", queueStats.BufferedMsgsHWM);
            ieut_jsonAddUInt64(&buffer, "RejectedMsgs", queueStats.RejectedMsgs);
            ieut_jsonAddUInt64(&buffer, "DiscardedMsgs", queueStats.DiscardedMsgs);
            ieut_jsonAddUInt64(&buffer, "ExpiredMsgs", queueStats.ExpiredMsgs);
            ieut_jsonAddUInt64(&buffer, "InflightEnqs", queueStats.InflightEnqs);
            ieut_jsonAddUInt64(&buffer, "InflightDeqs", queueStats.InflightDeqs);
            ieut_jsonAddUInt64(&buffer, "QAvoidCount", queueStats.QAvoidCount);
            ieut_jsonAddUInt64(&buffer, "MaxMessages", queueStats.MaxMessages);
            ieut_jsonAddUInt64(&buffer, "PutsAttempted", queueStats.PutsAttempted);
            ieut_jsonAddUInt64(&buffer, "BufferedMsgBytes", queueStats.BufferedMsgBytes);
            ieut_jsonAddUInt64(&buffer, "MaxMessageBytes", queueStats.MaxMessageBytes);

            if (stats_rc == OK)
            {
                ieut_jsonAddInt64(&buffer, "ConsumerCount", (int64_t)consumerCount);

                if (consumerCount != 0)
                {
                    // Consumers
                    ieut_jsonStartArray(&buffer, "Consumers");
                    for(size_t consumer=0; consumer<consumerCount; consumer++)
                    {
                        ieut_jsonStartObject(&buffer, NULL);

                        ieut_jsonAddString(&buffer, "ClientId", consumerStats[consumer].clientId);
                        ieut_jsonAddUInt64(&buffer, "ConsumerInflight", consumerStats[consumer].messagesInFlightToConsumer);
                        ieut_jsonAddUInt64(&buffer, "ClientInflight", consumerStats[consumer].messagesInFlightToClient);
                        ieut_jsonAddUInt64(&buffer, "MaxClientInflight", consumerStats[consumer].maxInflightToClient);
                        ieut_jsonAddUInt64(&buffer, "InflightReenable", consumerStats[consumer].inflightReenable);
                        ieut_jsonAddUInt32(&buffer, "ConsumerState", (uint32_t)consumerStats[consumer].consumerState);
                        ieut_jsonAddBool(&buffer, "DeliveryStarted", consumerStats[consumer].sessionDeliveryStarted);
                        ieut_jsonAddBool(&buffer, "DeliveryStopping", consumerStats[consumer].sessionDeliveryStopping);
                        ieut_jsonAddBool(&buffer, "SessionFlowPaused", consumerStats[consumer].sessionFlowControlPaused);
                        ieut_jsonAddBool(&buffer, "ConsumerFlowPaused", consumerStats[consumer].consumerFlowControlPaused);
                        ieut_jsonAddBool(&buffer, "MQTTIdsExhausted", consumerStats[consumer].mqttIdsExhausted);

                        ieut_jsonEndObject(&buffer);
                    }
                    ieut_jsonEndArray(&buffer);
                }
            }
            else
            {
                ieut_jsonAddInt32(&buffer, "ConsumerStatsRC", stats_rc);
            }

            // Shared subscription information
            iettSharedSubData_t *sharedSubData = iett_getSharedSubData(curSubscription);
            if (sharedSubData != NULL)
            {
                // Get the lock on the shared subscription
                DEBUG_ONLY int osrc = pthread_spin_lock(&sharedSubData->lock);
                assert(osrc == 0);

                ieut_jsonAddHexUInt32(&buffer, "AnonmyousSharers", (uint32_t)sharedSubData->anonymousSharers);

                ieut_jsonAddUInt32(&buffer, "SharingClientCount", sharedSubData->sharingClientCount);

                if (sharedSubData->sharingClientCount != 0)
                {
                    ieut_jsonStartArray(&buffer, "SharingClients");
                    for(uint32_t clientNo=0; clientNo<sharedSubData->sharingClientCount; clientNo++)
                    {
                        iettSharingClient_t *sharingClient = &sharedSubData->sharingClients[clientNo];

                        ieut_jsonStartObject(&buffer, NULL);
                        ieut_jsonAddString(&buffer, "ClientId", sharingClient->clientId);
                        ieut_jsonAddHexUInt32(&buffer, "SubOptions", sharingClient->subOptions);
                        ieut_jsonAddUInt32(&buffer, "SubId", sharingClient->subId);
                        ieut_jsonEndObject(&buffer);
                    }
                    ieut_jsonEndArray(&buffer);
                }

                osrc = pthread_spin_unlock(&sharedSubData->lock);
                assert(osrc == 0);
            }

            ieut_jsonEndObject(&buffer);

            if (writingToFile)
            {
                ieut_jsonNullTerminateJSONBuffer(&buffer);

                rc = ieie_exportData(pThreadData,
                                     diagFile,
                                     ieieDATATYPE_DIAGSUBDETAILSUBSCRIPTION,
                                     i+1,
                                     buffer.buffer.used,
                                     buffer.buffer.buf);

                if (rc != OK)
                {
                    ism_common_setError(rc);
                }

                ieut_jsonResetJSONBuffer(&buffer);
            }

            iemp_clearMemPool(pThreadData, memPool, false);
            iett_releaseSubscription(pThreadData, curSubscription, false);

            // Something went wrong -- release all the following subscriptions.
            if (rc != OK)
            {
                for(i++;i<context.matchingSubCount;i++)
                {
                    iett_releaseSubscription(pThreadData, context.matchingSubscriptions[i], false);
                }
            }
        }

        iemem_free(pThreadData, iemem_diagnostics, context.matchingSubscriptions);
        iemem_free(pThreadData, iemem_diagnostics, consumerStats);

        if (writingToFile)
        {
            if (rc == OK)
            {
                // Write a Footer
                ieut_jsonStartObject(&buffer, NULL);
                ieut_jsonAddString(&buffer, "Mode", (char *)mode);
                ieut_jsonAddUInt32(&buffer, "MatchingSubCount", context.matchingSubCount);
                ieut_jsonEndObject(&buffer);
                ieut_jsonNullTerminateJSONBuffer(&buffer);

                rc = ieie_exportData(pThreadData,
                                     diagFile,
                                     ieieDATATYPE_DIAGSUBDETAILSFOOTER,
                                     0,
                                     buffer.buffer.used,
                                     buffer.buffer.buf);

                if (rc != OK)
                {
                    ism_common_setError(rc);
                }
            }

            ieut_jsonResetJSONBuffer(&buffer);

            ieut_jsonStartObject(&buffer, NULL);
            ieut_jsonAddString(&buffer, "FilePath", filePath);
            ieut_jsonAddUInt32(&buffer, "MatchingSubCount", context.matchingSubCount);
            ieut_jsonAddInt32(&buffer, "rc", rc);
            ieut_jsonEndObject(&buffer);

            outbuf = ieut_jsonGenerateOutputBuffer(pThreadData, &buffer, iemem_diagnostics);
        }
        else
        {
            assert(rc == OK);

            ieut_jsonEndArray(&buffer);

            outbuf = ieut_jsonGenerateOutputBuffer(pThreadData, &buffer, iemem_diagnostics);
        }

        if (outbuf == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        *pDiagnosticsOutput = outbuf;
    }
    // Need to release any subscriptions...
    else
    {
        for(uint32_t i=0; i<context.matchingSubCount; i++)
        {
            iett_releaseSubscription(pThreadData, context.matchingSubscriptions[i], false);
        }

        iemem_free(pThreadData, iemem_diagnostics, context.matchingSubscriptions);
    }

mod_exit:

    if (filePath != NULL) iemem_free(pThreadData, iemem_diagnostics, filePath);
    if (diagFile != NULL) (void)ieie_finishWritingEncryptedFile(pThreadData, diagFile);
    if (parsedArgs != NULL) iemem_free(pThreadData, iemem_diagnostics, parsedArgs);
    if (memPool != NULL) iemp_destroyMemPool(pThreadData, &memPool);

    ieut_jsonReleaseJSONBuffer(&buffer);

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Request diagnostics from the engine
///
/// Requests miscellaneous diagnostic information from the engine.
///
/// @param[in]     mode               Type of diagnostics requested
/// @param[in]     args               Arguments to the diagnostics collection
/// @param[out]    diagnosticsOutput  If rc = OK, a diagnostic response string
///                                       to be freed with ism_engine_freeDiagnosticsOutput()
/// @param[in]     pContext           Optional context for completion callback
/// @param[in]     contextLength      Length of data pointed to by pContext
/// @param[in]     pCallbackFn        Operation-completion callback
///
/// @remark If the return code is OK then diagnosticsOutput will be a string that
/// should be returned to the user and then freed using ism_engine_freeDiagnosticsOutput().
/// If the return code is ISMRC_AsyncCompletion then the callback function will be called
/// and if the retcode is OK, the handle will point to the string to show and then free with
/// ism_engine_freeDiagnosticsOutput()
///
/// @returns OK on successful completion
///          ISMRC_AsyncCompletion if gone asynchronous
///          or an ISMRC_ value if there is a problem
//****************************************************************************
XAPI int32_t WARN_CHECKRC ism_engine_diagnostics(
    const char *                    mode,
    const char *                    args,
    char **                         pDiagnosticsOutput,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn)
{
    int32_t rc = OK;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ediaExecMode_t execMode = execMode_NONE;

    // Always pass an empty string for NULL args
    if (args == NULL) args = "";

    // We don't trace arguments for all calls (some contain sensitive information, like passwords)
    const char *traceArgs = args;

    // No mode specified or no pointer for output - trace entry but fail
    if (mode == NULL || pDiagnosticsOutput == NULL)
    {
        ieutTRACEL(pThreadData, mode, ENGINE_CEI_TRACE,
                   FUNCTION_ENTRY "mode=<NULL> execMode=%d traceArgs='%s' pDiagnosticsOutput=%p pContext=%p contextLength=%lu pCallbackFn=%p\n", __func__,
                   execMode, traceArgs, pDiagnosticsOutput, pContext, contextLength, pCallbackFn);
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
        goto mod_exit;
    }
    // Echo
    else if (mode[0] == ediaVALUE_MODE_ECHO[0] &&
             strcmp(mode, ediaVALUE_MODE_ECHO) == 0)
    {
        execMode = execMode_ECHO;
    }
    // MemoryDetails
    else if (mode[0] == ediaVALUE_MODE_MEMORYDETAILS[0] &&
             strcmp(mode, ediaVALUE_MODE_MEMORYDETAILS) == 0)
    {
        execMode = execMode_MEMORYDETAILS;
    }
    // DumpClientStates
    else if (mode[0] == ediaVALUE_MODE_DUMPCLIENTSTATES[0] &&
             strcmp(mode, ediaVALUE_MODE_DUMPCLIENTSTATES) == 0)
    {
        execMode = execMode_DUMPCLIENTSTATES;
        traceArgs = ediaVALUE_OUTPUT_UNTRACEDARGS;
    }
    // OwnerCounts
    else if (mode[0] == ediaVALUE_MODE_OWNERCOUNTS[0] &&
             strcmp(mode, ediaVALUE_MODE_OWNERCOUNTS) == 0)
    {
        execMode = execMode_OWNERCOUNTS;
    }
    // DumpTraceHistory
    else if (mode[0] == ediaVALUE_MODE_DUMPTRACEHISTORY[0] &&
             strcmp(mode, ediaVALUE_MODE_DUMPTRACEHISTORY) == 0)
    {
        execMode = execMode_DUMPTRACEHISTORY;
        traceArgs = ediaVALUE_OUTPUT_UNTRACEDARGS;
    }
    // SubscriptionDetails
    else if (mode[0] == ediaVALUE_MODE_SUBDETAILS[0] &&
             strcmp(mode, ediaVALUE_MODE_SUBDETAILS) == 0)
    {
        execMode = execMode_SUBDETAILS;

        if (strcasestr(args, ediaVALUE_PARM_PASSWORD) != NULL)
        {
            traceArgs = ediaVALUE_OUTPUT_UNTRACEDARGS;
        }
    }
    // DumpResourceSets
    else if (mode[0] == ediaVALUE_MODE_DUMPRESOURCESETS[0] &&
             strcmp(mode, ediaVALUE_MODE_DUMPRESOURCESETS) == 0)
    {
        execMode = execMode_DUMPRESOURCESETS;
    }
    // ResourceSetReport
    else if (mode[0] == ediaVALUE_MODE_RESOURCESETREPORT[0] &&
             strcmp(mode, ediaVALUE_MODE_RESOURCESETREPORT) == 0)
    {
        execMode = execMode_RESOURCESETREPORT;
    }
    else if (mode[0] == ediaVALUE_MODE_MEMORYTRIM[0] &&
            strcmp(mode, ediaVALUE_MODE_MEMORYTRIM) == 0)
    {
        execMode = execMode_MEMORYTRIM;
    }
    else if (mode[0] == ediaVALUE_MODE_ASYNCCBSTATS[0] &&
                strcmp(mode, ediaVALUE_MODE_ASYNCCBSTATS) == 0)
    {
        execMode = execMode_ASYNCCBSTATS;
    }
    // Invalid request type
    else
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
    }

    ieutTRACEL(pThreadData, mode[0], ENGINE_CEI_TRACE,
               FUNCTION_ENTRY "mode='%s' execMode=%d traceArgs='%s' pDiagnosticsOutput=%p pContext=%p contextLength=%lu pCallbackFn=%p\n", __func__,
               mode, execMode, traceArgs, pDiagnosticsOutput, pContext, contextLength, pCallbackFn);

    if (rc != OK) goto mod_exit;

    // Now actually execute the request
    switch(execMode)
    {
        case execMode_ECHO:
            rc = edia_modeEcho(pThreadData,
                               mode,
                               args,
                               pDiagnosticsOutput,
                               pContext, contextLength, pCallbackFn);
            break;
        case execMode_MEMORYDETAILS:
            rc = edia_modeMemoryDetails(pThreadData,
                                        mode,
                                        args,
                                        pDiagnosticsOutput,
                                        pContext, contextLength, pCallbackFn, NULL);
            break;
        case execMode_DUMPCLIENTSTATES:
            rc = edia_modeDumpClientStates(pThreadData,
                                           mode,
                                           args,
                                           pDiagnosticsOutput,
                                           pContext, contextLength, pCallbackFn);
            break;
        case execMode_OWNERCOUNTS:
            rc = edia_modeOwnerCounts(pThreadData,
                                      mode,
                                      args,
                                      pDiagnosticsOutput,
                                      pContext, contextLength, pCallbackFn);
            break;
        case execMode_DUMPTRACEHISTORY:
            rc = edia_modeDumpTraceHistory(pThreadData,
                                           mode,
                                           args,
                                           pDiagnosticsOutput,
                                           pContext, contextLength, pCallbackFn);
            break;
        case execMode_SUBDETAILS:
            rc = edia_modeSubDetails(pThreadData,
                                     mode,
                                     args,
                                     pDiagnosticsOutput,
                                     pContext, contextLength, pCallbackFn);
            break;
        case execMode_DUMPRESOURCESETS:
            rc = edia_modeDumpResourceSets(pThreadData,
                                           mode,
                                           args,
                                           pDiagnosticsOutput,
                                           pContext, contextLength, pCallbackFn);
            break;
        case execMode_RESOURCESETREPORT:
            rc = edia_modeResourceSetReport(pThreadData,
                                            mode,
                                            args,
                                            pDiagnosticsOutput,
                                            pContext, contextLength, pCallbackFn);
            break;
        case execMode_MEMORYTRIM:
            rc = edia_modeMemoryTrim(pThreadData,
                                     mode,
                                     args,
                                     pDiagnosticsOutput,
                                     pContext, contextLength, pCallbackFn);
            break;
        case execMode_ASYNCCBSTATS:
            rc = edia_modeAsyncCBStats(pThreadData,
                                       mode,
                                       args,
                                       pDiagnosticsOutput,
                                       pContext, contextLength, pCallbackFn);
            break;
        default:
            assert(false);
            rc = ISMRC_InvalidOperation;
            ism_common_setError(rc);
            break;
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Free diagnostic info supplied by engine
///
/// @param[in]    diagnosticsOutput   string from ism_engine_diagnostics() or
///                                   returned as the handle to the callback function
///                                   from it
//****************************************************************************
XAPI void ism_engine_freeDiagnosticsOutput(char * diagnosticsOutput)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    ieutTRACEL(pThreadData, diagnosticsOutput, ENGINE_CEI_TRACE, FUNCTION_IDENT "outbuf=%p\n"
                       , __func__, diagnosticsOutput);

    iemem_free( pThreadData, iemem_diagnostics, diagnosticsOutput );

    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
/// @internal
///
/// @brief  Add the engine diagnostics into a ism_json_t object
///
/// @param[in]    jobj    the ism_json_t to add the diagnostics into
/// @param[in]    name    what to call it
//****************************************************************************
XAPI void ism_engine_addDiagnostics(ism_json_t * jobj, const char * name) {
    char * engineStats;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    if (OK == edia_modeMemoryDetails(pThreadData, NULL, NULL, &engineStats, NULL, 0, NULL, name)) {
        ism_json_putIndent(jobj, 1, NULL);
        ism_common_allocBufferCopyLen(jobj->buf, engineStats, strlen(engineStats));
        ism_engine_freeDiagnosticsOutput(engineStats);
    }
    ieut_leavingEngine(pThreadData);
}

