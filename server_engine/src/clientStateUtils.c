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

//*********************************************************************
/// @file  clientStateUtils.c
/// @brief Utilities related to the management of client-states
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "engineDump.h"
#include "memHandler.h"
#include "clientState.h"
#include "clientStateInternal.h"
#include "exportClientState.h"
#include "topicTree.h"
#include "resourceSetStats.h"

/*
 * Increment active client state count -- including resourceSet specific counts.
 */
void iecs_incrementActiveClientStateCount(ieutThreadData_t *pThreadData,
                                          iecsDurability_t durability,
                                          bool updateExternalStats,
                                          iereResourceSetHandle_t resourceSet)
{
    iere_primeThreadCache(pThreadData, resourceSet);

    if (updateExternalStats)
    {
        if (durability == iecsDurable)
        {
            iere_updateInt64Stat(pThreadData, resourceSet,
                                 ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_ACTIVE_PERSISTENT_CLIENTS, 1);
        }
        else
        {
            iere_updateInt64Stat(pThreadData, resourceSet,
                                 ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_ACTIVE_NONPERSISTENT_CLIENTS, 1);
        }
    }

    __sync_fetch_and_add(&ismEngine_serverGlobal.totalActiveClientStatesCount, 1);
}

/*
 * Decrement active client state count -- including resourceSet specific counts.
 */
void iecs_decrementActiveClientStateCount(ieutThreadData_t *pThreadData,
                                          iecsDurability_t durability,
                                          bool updateExternalStats,
                                          iereResourceSetHandle_t resourceSet)
{
    iere_primeThreadCache(pThreadData, resourceSet);

    if (updateExternalStats)
    {
        if (durability == iecsDurable)
        {
            iere_updateInt64Stat(pThreadData, resourceSet,
                                 ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_ACTIVE_PERSISTENT_CLIENTS, -1);
        }
        else
        {
            iere_updateInt64Stat(pThreadData, resourceSet,
                                 ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_ACTIVE_NONPERSISTENT_CLIENTS, -1);
        }
    }

    DEBUG_ONLY uint64_t oldVal = __sync_fetch_and_sub(&ismEngine_serverGlobal.totalActiveClientStatesCount, 1);
    assert(oldVal != 0);
}

/*
 * Get the resourceSet to use for a given clientID and protocolId
 */
iereResourceSetHandle_t iecs_getResourceSet(ieutThreadData_t *pThreadData,
                                            const char *pClientId,
                                            uint32_t protocolId,
                                            int32_t operation)
{
    iereResourceSetHandle_t resourceSet;

    // Don't want the clients that exist to own globally shared subscriptions to belong to any resourceSet,
    // the subscriptions themselves belong to a resourceSet based on the topic they are on.
    if (protocolId == PROTOCOL_ID_SHARED)
    {
        resourceSet = iereNO_RESOURCE_SET;
    }
    else
    {
        resourceSet =  iere_getResourceSet(pThreadData, pClientId, NULL, operation);
    }

    return resourceSet;
}

/*
 * Create a new client-state object by allocating it and adding it to the table
 */
int32_t iecs_createClientState(ieutThreadData_t *pThreadData,
                               const char *pClientId,
                               uint32_t protocolId,
                               uint32_t options,
                               uint32_t internalOptions,
                               iereResourceSetHandle_t resourceSet,
                               void *pStealContext,
                               ismEngine_StealCallback_t pStealCallbackFn,
                               ismSecurity_t *pSecContext,
                               ismEngine_ClientStateHandle_t * phClient,
                               void *pContext,
                               size_t contextLength,
                               ismEngine_CompletionCallback_t pCallbackFn)
{
    int32_t rc;

    ieutTRACEL(pThreadData, protocolId, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "(protocolId=%u, internalOptions=0x%08x)\n", __func__,
               protocolId, internalOptions);

    iecsNewClientStateInfo_t clientInfo;

    clientInfo.pClientId = pClientId;
    clientInfo.protocolId = protocolId;
    clientInfo.pSecContext = pSecContext;
    clientInfo.pStealContext = pStealContext;
    clientInfo.pStealCallbackFn = pStealCallbackFn;
    clientInfo.resourceSet = resourceSet;

    assert((options & ~(ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                        ismENGINE_CREATE_CLIENT_OPTION_CHECK_USER_STEAL |
                        ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                        ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART |
                        ismENGINE_CREATE_CLIENT_OPTION_INHERIT_DURABILITY)) == 0);

    if (options & ismENGINE_CREATE_CLIENT_OPTION_INHERIT_DURABILITY)
    {
        clientInfo.durability = (options & ismENGINE_CREATE_CLIENT_OPTION_DURABLE) ?
                                 iecsInheritOrDurable : iecsInheritOrNonDurable;
    }
    else
    {
        clientInfo.durability = (options & ismENGINE_CREATE_CLIENT_OPTION_DURABLE) ?
                                 iecsDurable : iecsNonDurable;
    }

    clientInfo.fCleanStart = (options & ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART) ? true : false;

    ismEngine_ClientState_t *pClient = NULL;

    // Allocate a new client-state
    rc = iecs_newClientState(pThreadData, &clientInfo, &pClient);
    if (rc == OK)
    {
        bool countExternally = pClient->fCountExternally;

        // Attempt to add the new client-state, potentially starting the steal process
        rc = iecs_addClientState(pThreadData,
                                 pClient,
                                 options,
                                 internalOptions,
                                 pContext,
                                 contextLength,
                                 pCallbackFn);

        if (rc < ISMRC_Error)
        {
            // We consider this clientState to be active now, even if it completes asynchrnously.
            iecs_incrementActiveClientStateCount(pThreadData, clientInfo.durability, countExternally, resourceSet);

            if (rc == OK || rc == ISMRC_ResumedClientState)
            {
                *phClient = pClient;
            }
        }
        else
        {
            iecs_freeClientState(pThreadData, pClient, true);
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

/*
 * Find the clientstate (with the specified clientid) that
 * has the longest train of theives victomising it.
 *
 * @remark: MUST have the client state mutex held whilst this
 *          function is called
 */
ismEngine_ClientState_t *iecs_getVictimizedClient(ieutThreadData_t *pThreadData,
                                                  const char *pClientId,
                                                  uint32_t clientIdHash)
{
    ieutTRACEL(pThreadData, pClientId,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pClientId '%s'\n", __func__, pClientId);

    ismEngine_ClientState_t *pClient = NULL;

    iecsHashTable_t *pTable = NULL;

    iecsHashChain_t *pChain;
    uint32_t chain;

    pTable = ismEngine_serverGlobal.ClientTable;

    chain = clientIdHash & pTable->ChainMask;
    pChain = pTable->pChains + chain;

    // Scan looking for the right client-state
    iecsHashEntry_t *pEntry = pChain->pEntries;
    uint32_t remaining = pChain->Count;
    uint32_t highestThiefCount = 0;

    while (remaining > 0)
    {
        ismEngine_ClientState_t *pCurrent = pEntry->pValue;
        if (pCurrent != NULL)
        {
            // This client state matches
            if (pEntry->Hash == clientIdHash &&
                strcmp(pCurrent->pClientId, pClientId) == 0)
            {
                if (pCurrent->fLeaveResourcesAtRestart == false)
                {
                    uint32_t thiefCount = 0;
                    ismEngine_ClientState_t *pThief = pCurrent->pThief;

                    // Need to find the entry with the most thieves
                    while(pThief != NULL)
                    {
                        thiefCount += 1;

                        // If we found our highest so far, add the highest count on and stop
                        if (pThief == pClient)
                        {
                            thiefCount += highestThiefCount;
                            break; // leave the inner while loop
                        }

                        pThief = pThief->pThief;
                    }

                    // Found an entry with more thieves
                    if (thiefCount >= highestThiefCount)
                    {
                        pClient = pCurrent;
                        highestThiefCount = thiefCount;
                    }
                }
            }

            remaining--;
        }

        pEntry++;
    }

    ieutTRACEL(pThreadData, pClient,  ENGINE_FNC_TRACE, FUNCTION_EXIT "pClient=%p \n", __func__, pClient);

    return pClient;
}

/*
 * Finds a clientState and returns a pointer to it, if it not a zombie
 * This function increases the refcount of the client and a steal will not complete
 * until the caller reduces the refcount
 */
void iecs_findClientState( ieutThreadData_t *pThreadData
                         , const char *pClientId
                         , bool onlyIfConnected
                         , ismEngine_ClientState_t **ppConnectedClient)
{


    ieutTRACEL(pThreadData, pClientId,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pClientId '%s'\n", __func__, pClientId);

    ismEngine_ClientState_t *pClient = NULL;
    *ppConnectedClient = NULL;

    uint32_t hash = iecs_generateClientIdHash(pClientId);

    // Find and acquire a reference to the client state
    ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);

    pClient = iecs_getVictimizedClient(pThreadData, pClientId, hash);

    if (pClient != NULL && (onlyIfConnected == false || pClient->OpState != iecsOpStateZombie))
    {
        iecs_acquireClientStateReference(pClient);
        *ppConnectedClient = pClient;
    }

    ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

    ieutTRACEL(pThreadData, *ppConnectedClient,  ENGINE_FNC_TRACE, FUNCTION_EXIT "pClient=%p \n", __func__, *ppConnectedClient);

    return;
}

/*
 * Discard a zombie client-state
 */
int32_t iecs_discardZombieClientState(ieutThreadData_t *pThreadData,
                                      const char *pClientId,
                                      bool fFromImport,
                                      void *pContext,
                                      size_t contextLength,
                                      ismEngine_CompletionCallback_t pCallbackFn)
{
    iecsHashTable_t *pTable = NULL;
    uint32_t hash;
    ismEngine_ClientState_t *pClient = NULL;
    int32_t rc = OK;

    hash = iecs_generateClientIdHash(pClientId);

    ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);

    // Unless it is being requested by the import processing, don't allow this zombie
    // to be discarded
    if (fFromImport == false && ieie_isClientIdBeingImported(pThreadData, pClientId, hash))
    {
        rc = ISMRC_ClientIDInUse;
        ism_common_setErrorData(rc, "%s", pClientId);
    }
    else
    {
        uint32_t chain;
        iecsHashChain_t *pChain;

        pTable = ismEngine_serverGlobal.ClientTable;

        chain = hash & pTable->ChainMask;
        pChain = pTable->pChains + chain;

        // Scan looking for the right client-state
        iecsHashEntry_t *pEntry = pChain->pEntries;
        uint32_t remaining = pChain->Count;
        while (remaining > 0)
        {
            ismEngine_ClientState_t *pCurrent = pEntry->pValue;
            if (pCurrent != NULL)
            {
                if ((pEntry->Hash == hash) &&
                    (pCurrent->pThief == NULL) &&
                    (strcmp(pCurrent->pClientId, pClientId) == 0))
                {
                    pClient = pCurrent;
                    break;
                }

                remaining--;
            }

            pEntry++;
        }

        // If we found the client-state, it can only be discarded if it's a zombie
        if (pClient != NULL)
        {
            assert(rc == OK);

            void *pPendingDestroyContext = NULL;
            iereResourceSetHandle_t resourceSet = pClient->resourceSet;

            // Allocate memory for the callback in case we need to go asynchronous
            if (contextLength > 0)
            {
                iere_primeThreadCache(pThreadData, resourceSet);
                pPendingDestroyContext = iere_malloc(pThreadData,
                                                     resourceSet,
                                                     IEMEM_PROBE(iemem_callbackContext, 7), contextLength);
                if (pPendingDestroyContext == NULL)
                {
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                }
            }

            if (rc == OK)
            {
                pthread_spin_lock(&pClient->UseCountLock);
                if (pClient->OpState != iecsOpStateZombie)
                {
                    rc = ISMRC_ClientIDConnected;
                    ism_common_setError(rc);
                }
                else
                {
                    pClient->UseCount += 1;
                    pClient->OpState = iecsOpStateZombieRemoval;
                    if (pClient->ExpiryTime != 0)
                    {
                        pClient->ExpiryTime = 0;
                        pThreadData->stats.zombieSetToExpireCount -= 1;
                    }
                    assert(pClient->pStealCallbackFn == NULL);
                }
                pthread_spin_unlock(&pClient->UseCountLock);
            }

            if (rc != OK)
            {
                pClient = NULL;
            }

            if (pClient != NULL)
            {
                iecs_lockClientState(pClient);

                assert(pClient->pPendingDestroyContext == NULL);
                assert(pClient->pPendingDestroyCallbackFn == NULL);

                pClient->pPendingDestroyContext = pPendingDestroyContext;
                if (contextLength > 0)
                {
                    memcpy(pPendingDestroyContext, pContext, contextLength);
                }
                pClient->pPendingDestroyCallbackFn = pCallbackFn;

                iecs_unlockClientState(pClient);
            }
            else
            {
                iere_free(pThreadData, resourceSet, iemem_callbackContext, pPendingDestroyContext);
            }
        }
        else
        {
            rc = ISMRC_NotFound;
            if (!fFromImport) ism_common_setError(rc);
        }
    }

    ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

    if (pClient != NULL)
    {
        assert(rc == OK);

        bool didRelease = iecs_releaseClientStateReference(pThreadData, pClient, true, false);

        // If the caller didn't pass us a callback function, they also won't be expecting
        // us to go async (specifically, the admin component does this) - the destroy will
        // still happen asynchronously, the caller will just not be able to tell when it
        // finishes.
        if (didRelease == false && pCallbackFn != NULL)
        {
            rc = ISMRC_AsyncCompletion;
        }
    }

    return rc;
}

/*
 * Steal callback used in the middle of a forceDiscardClientState request.
 * (We ignore the steal request, the completion callback will perform the destroy).
 */
static void iecs_forceDiscardClientStateStealCallback(int32_t reason,
                                                     ismEngine_ClientStateHandle_t hClient,
                                                     uint32_t options,
                                                     void *pContext)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    iecsEngineClientContext_t *context = (iecsEngineClientContext_t *)pContext;

    assert(strcmp(hClient->pClientId, context->pClientId) == 0);

    ieutTRACEL(pThreadData, hClient, ENGINE_INTERESTING_TRACE,
               FUNCTION_IDENT "(reason=%d, hClient=%p, clientId='%s', options=0x%08x, reason=%d, options=0x%08x)\n",
               __func__, reason, hClient, hClient->pClientId, options, context->reason, context->options);
}

/*
 * Destroy completion routine for forceDiscardClientState request.
 */
static int32_t iecs_forceDiscardFinishDestroyClient(ieutThreadData_t *pThreadData,
                                                   int32_t retcode,
                                                   ismEngine_ClientState_t *pClient,
                                                   iecsEngineClientContext_t *context)
{
    ieutTRACEL(pThreadData, pClient, ENGINE_INTERESTING_TRACE,
               FUNCTION_IDENT "(retcode=%d, pClient=%p, clientId='%s', reason=%d, options=0x%08x)\n",
               __func__, retcode, pClient, context->pClientId, context->reason, context->options);

    iemem_free(pThreadData, iemem_callbackContext, context);

    return retcode;
}

/*
 * Async destroy completion callback for forceDiscardClientState request.
 */
static void iecs_forceDiscardClientDestroyCompletionCallback(int32_t retcode,
                                                            void *handle,
                                                            void *pContext)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)handle;
    iecsEngineClientContext_t *context = *(iecsEngineClientContext_t **)pContext;

    assert(retcode == OK);
    assert(handle == NULL);

    (void)iecs_forceDiscardFinishDestroyClient(pThreadData, retcode, pClient, context);
}

/*
 * Create completion routine for forceDiscardClientState request.
 */
static int32_t iecs_forceDiscardFinishCreateClient(ieutThreadData_t *pThreadData,
                                                  int32_t retcode,
                                                  ismEngine_ClientState_t *pClient,
                                                  iecsEngineClientContext_t *context)
{
    ieutTRACEL(pThreadData, pClient, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "(retcode=%d, pClient=%p, clientId='%s', reason=%d)\n",
               __func__, retcode, pClient, context->pClientId, context->reason);

    int32_t rc = ism_engine_destroyClientState(pClient,
                                               ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                               &context,
                                               sizeof(context),
                                               iecs_forceDiscardClientDestroyCompletionCallback);

    if (rc == OK)
    {
        rc = iecs_forceDiscardFinishDestroyClient(pThreadData,
                                                 rc,
                                                 pClient,
                                                 context);
    }
    else
    {
        assert(rc == ISMRC_AsyncCompletion);
    }

    ieutTRACEL(pThreadData, pClient, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

/*
 * Async create completion callback for forceDiscardClientState request.
 */
static void iecs_forceDiscardClientCreateCompletionCallback(int32_t retcode,
                                                            void *handle,
                                                            void *pContext)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)handle;
    iecsEngineClientContext_t *context = *(iecsEngineClientContext_t **)pContext;

    assert(retcode == OK || retcode == ISMRC_ResumedClientState);
    assert(context->hClient == NULL || context->hClient == pClient);
    assert(pClient != NULL);

    context->hClient = handle;

    (void)iecs_forceDiscardFinishCreateClient(pThreadData, retcode, pClient, context);
}

/*
 * Find the message delivery reference info for the specified clientId
 */
int32_t iecs_forceDiscardClientState(ieutThreadData_t *pThreadData,
                                     const char *pClientId,
                                     uint32_t options)
{
    int32_t rc = OK;

    // Sanity check
    assert(ismEngine_serverGlobal.runPhase >= EnginePhaseRunning);

    ieutTRACEL(pThreadData, options, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "(pClientId='%s', options=0x%08x)\n",
               __func__, pClientId, options);

    uint32_t internalCreateOptions = iecsCREATE_CLIENT_OPTION_NONE;
    int32_t reason = OK;

    // We actually remove the clientState by replacing it with one which then destroys
    // itself, we need a valid reason to do this.
    if ((options & iecsFORCE_DISCARD_CLIENT_OPTION_NON_ACKING_CLIENT) != 0)
    {
        reason = ISMRC_NonAckingClient;
        internalCreateOptions = iecsCREATE_CLIENT_OPTION_NONACKING_TAKEOVER;
    }

    if (reason == OK)
    {
        rc = ISMRC_InvalidParameter;
        ism_common_setError(rc);
        goto mod_exit;
    }

    char messageBuffer[256];

    // Write a log message (Note: for the resourceSet query, we assume PROTOCOL_ID_MQTT)
    iereResourceSetHandle_t resourceSet = iecs_getResourceSet(pThreadData, pClientId, PROTOCOL_ID_MQTT, iereOP_FIND);
    ism_common_log_context logContext = {0};
    logContext.clientId = pClientId;
    if (resourceSet != iereNO_RESOURCE_SET)
    {
        logContext.resourceSetId = resourceSet->stats.resourceSetIdentifier;
    }

    LOGCTX(&logContext, ERROR, Messaging, 3021, "%s%s%d", "The state for clientId {0} is being forcibly discarded for reason {1} ({2}).",
           pClientId,
           ism_common_getErrorStringByLocale(reason, ism_common_getLocale(), messageBuffer, 255),
           reason);

    size_t clientIdLength = strlen(pClientId)+1;
    iecsEngineClientContext_t *context;
    context = (iecsEngineClientContext_t *)iemem_malloc(pThreadData,
                                                        IEMEM_PROBE(iemem_callbackContext, 17),
                                                        sizeof(iecsEngineClientContext_t) + clientIdLength);

    if (context == NULL)
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    ismEngine_SetStructId(context->StrucId, iecsENGINE_CLIENT_CONTEXT_STRUCID);
    context->reason = reason;
    context->pClientId = (char *)(context+1);
    memcpy(context->pClientId, pClientId, clientIdLength);
    context->hClient = NULL;
    context->options = options;

    rc = iecs_createClientState(pThreadData,
                                pClientId,
                                PROTOCOL_ID_ENGINE,
                                ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART |
                                ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                internalCreateOptions,
                                iereNO_RESOURCE_SET,
                                context,
                                iecs_forceDiscardClientStateStealCallback,
                                NULL,
                                &context->hClient,
                                &context, sizeof(context),
                                iecs_forceDiscardClientCreateCompletionCallback);

    // Creation didn't go asynchronous, so we now need to call the completion function.
    if (rc == OK || rc == ISMRC_ResumedClientState)
    {
        assert(context->hClient != NULL);
        rc = iecs_forceDiscardFinishCreateClient(pThreadData, rc, context->hClient, context);
    }
    // Unless the request is going async, we need to clean up...
    else if (rc != ISMRC_AsyncCompletion)
    {
        iemem_free(pThreadData, iemem_callbackContext, context);
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

// Compare two ENGINE protocol client instances to determine if one is a duplicate of the
// other, returns 0 if the two clients should be considered duplicates of eachother, otherwise
// non-zero.
int32_t iecs_compareEngineClientStates(ieutThreadData_t *pThreadData,
                                       ismEngine_ClientStateHandle_t hClient1,
                                       ismEngine_ClientStateHandle_t hClient2)
{
    int32_t result;
    ismEngine_ClientState_t *pClient1 = (ismEngine_ClientState_t *)hClient1;
    ismEngine_ClientState_t *pClient2 = (ismEngine_ClientState_t *)hClient2;

    assert(pClient1 != NULL);
    assert(pClient2 != NULL);

    if (pClient1->protocolId != PROTOCOL_ID_ENGINE || pClient2->protocolId != PROTOCOL_ID_ENGINE)
    {
        result = 1;
        goto mod_exit;
    }

    iecsEngineClientContext_t *pContext1 = pClient1->pStealContext;
    iecsEngineClientContext_t *pContext2 = pClient2->pStealContext;

    if (pContext1 == NULL || pContext2 == NULL)
    {
        result = 2;
        goto mod_exit;
    }

    ismEngine_CheckStructId(pContext1->StrucId, iecsENGINE_CLIENT_CONTEXT_STRUCID, ieutPROBE_001);
    ismEngine_CheckStructId(pContext2->StrucId, iecsENGINE_CLIENT_CONTEXT_STRUCID, ieutPROBE_002);

    // Reason for creation should match
    if (pContext1->reason != pContext2->reason)
    {
        result = 3;
        goto mod_exit;
    }

    // Any specified options should match
    if (pContext1->options != pContext2->options)
    {
        result = 4;
        goto mod_exit;
    }

    // ClientIds must match
    if (strcmp(pContext1->pClientId, pContext2->pClientId) != 0)
    {
        result = 5;
        goto mod_exit;
    }

    // We got this far, it's a match
    result = 0;

mod_exit:

    return result;
}


// Takes one of the following combinations:
//      ClientId:     Disconnects this client
//      Qhdl:         Disconnects the client that owns this queue (should not be a shared q)
//      Qhdl & node:  Disconnects the client that owns this node (for shared queues)
//
// Returns true if client has been (or will be) removed
//         false otherwise
//
bool iecs_discardClientForUnreleasedMessageDeliveryReference( ieutThreadData_t *pThreadData
                                                            , ismQHandle_t Qhdl
                                                            , void *mdrNode
                                                            , const char *pClientId)
{
    ieutTRACEL(pThreadData, Qhdl,  ENGINE_INTERESTING_TRACE, FUNCTION_ENTRY "Q %p Node %p Client %s\n", __func__, Qhdl, mdrNode, pClientId);

    int32_t rc = OK;
    bool doingRemove = false;
    bool subUseCountIncreased = false;
    ismEngine_SubscriptionHandle_t subHdl = NULL;
    const char **clientIdArr = NULL;

    // Until the engine is running, we shouldn't be accessing the client or its subscriptions.
    if (ismEngine_serverGlobal.runPhase < EnginePhaseRunning)
    {
        ieutTRACEL(pThreadData, ismEngine_serverGlobal.runPhase, ENGINE_INTERESTING_TRACE, "Unable to discard client during runPhase 0x%08x\n",
                   ismEngine_serverGlobal.runPhase);
        goto mod_exit;
    }

    if (pClientId == NULL)
    {
        rc = iett_findQHandleSubscription ( pThreadData
                                          , Qhdl
                                          , &subHdl );

        if (rc != OK)
        {
            ieutTRACEL(pThreadData, Qhdl,  ENGINE_INTERESTING_TRACE,
                                 "Couldn't find sub for Qhdl %p\n", Qhdl);
            assert(subHdl == NULL);
            goto mod_exit;
        }

        assert(subHdl != NULL);
        subUseCountIncreased = true;

        rc = iett_findSubscriptionClientIds ( pThreadData
                                            , subHdl
                                            , &clientIdArr );

        if (rc != OK)
        {
            ieutTRACEL(pThreadData, subHdl,  ENGINE_INTERESTING_TRACE,
                                 "Couldn't find client for subHdl %p\n", subHdl);
            assert(clientIdArr == NULL);
            goto mod_exit;
        }
        assert(clientIdArr != NULL && clientIdArr[0] != NULL);

        if (mdrNode != NULL)
        {
           const char *nonAckingClientId = NULL;

           rc = iecs_identifyMessageDeliveryReferenceOwner(
                      pThreadData,
                      clientIdArr,
                      Qhdl,
                      mdrNode,
                      &nonAckingClientId);

           if (rc != OK)
           {
               ieutTRACEL(pThreadData, mdrNode,  ENGINE_INTERESTING_TRACE,
                                    "Couldn't find client for MDR\n");
               goto mod_exit;
           }

           pClientId = nonAckingClientId;
        }
        else
        {
            //Check only 1 client returned for this subscription
            assert(clientIdArr[1] == NULL);
            //that's the one we'll disconnect
            pClientId = clientIdArr[0];
        }

        (void)iett_releaseSubscription(pThreadData, subHdl, false);
        subUseCountIncreased = false;
    }

    if (pClientId != NULL)
    {
        //Right, we have our guy... let's shoot him.
        ieutTRACEL(pThreadData, pClientId,  ENGINE_INTERESTING_TRACE,
                             "Discarding state for Client %s\n", pClientId);

        rc = iecs_forceDiscardClientState( pThreadData
                                         , pClientId
                                         , iecsFORCE_DISCARD_CLIENT_OPTION_NON_ACKING_CLIENT);

        if (rc == OK || rc == ISMRC_AsyncCompletion || rc == ISMRC_RequestInProgress)
        {
            doingRemove = true;
        }
    }
    else
    {
        ieutTRACEL(pThreadData, Qhdl,  ENGINE_INTERESTING_TRACE, "Couldn't find client\n");
    }

mod_exit:
    if (clientIdArr != NULL)
    {
        iett_freeSubscriptionClientIds(pThreadData, clientIdArr);
    }

    if (subUseCountIncreased)
    {
        (void)iett_releaseSubscription(pThreadData, subHdl, false);
    }
    ieutTRACEL(pThreadData, rc,  ENGINE_INTERESTING_TRACE, FUNCTION_EXIT "\n", __func__);

    return doingRemove;
}

/*
 * Find the message delivery reference info for the specified clientId
 */
int32_t iecs_findClientMsgDelInfo(ieutThreadData_t *pThreadData,
                                  const char *pClientId,
                                  iecsMessageDeliveryInfoHandle_t *phMsgDelInfo)
{
    int32_t rc = ISMRC_NotFound;

    ieutTRACEL(pThreadData, pClientId,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pClientId '%s'\n", __func__, pClientId);

    ismEngine_ClientState_t *pClient = NULL;

    uint32_t hash = iecs_generateClientIdHash(pClientId);

    // Find and acquire a reference to the client state
    ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);

    pClient = iecs_getVictimizedClient(pThreadData, pClientId, hash);

    // We have a client, acquire a reference on it (which the caller must release)
    if (pClient != NULL && pClient->hMsgDeliveryInfo != NULL)
    {
        rc = iecs_acquireMessageDeliveryInfoReference(pThreadData, pClient, phMsgDelInfo);
    }

    ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d \n", __func__, rc);

    return rc;
}

/*
 * Add the specified client state to the dump
 */
int32_t iecs_dumpClientState(ieutThreadData_t *pThreadData,
                             const char *pClientId,
                             iedmDumpHandle_t dumpHdl)
{
    int32_t rc = OK;

    ismEngine_ClientState_t *pClient = NULL;

    uint32_t hash = iecs_generateClientIdHash(pClientId);

    // Find and acquire a reference to the client state
    ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);

    iecsHashTable_t *pTable = NULL;
    iecsHashChain_t *pChain;
    uint32_t chain;

    pTable = ismEngine_serverGlobal.ClientTable;

    chain = hash & pTable->ChainMask;
    pChain = pTable->pChains + chain;

    // Scan looking for the right client-state
    iecsHashEntry_t *pEntry = pChain->pEntries;
    uint32_t remaining = pChain->Count;
    while (remaining > 0)
    {
        ismEngine_ClientState_t *pCurrent = pEntry->pValue;
        if (pCurrent != NULL)
        {
            if ((pEntry->Hash == hash) &&
                (pCurrent->pThief == NULL) &&
                (strcmp(pCurrent->pClientId, pClientId) == 0))
            {
                pClient = pCurrent;
                iecs_acquireClientStateReference(pClient);
                break;
            }

            remaining--;
        }

        pEntry++;
    }

    ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

    // We found a client state for the clientId specified
    if (pClient != NULL)
    {
        iedmDump_t *dump = (iedmDump_t *)dumpHdl;

        if (iedm_dumpStartObject(dump, pClient))
        {
            iedm_dumpStartGroup(dump, "Client-State");

            // Dump this client-state
            iedm_dumpData(dump, "ismEngine_ClientState_t", pClient,
                          iere_usable_size(iemem_clientState, pClient));

            // Run through the sessions on this client-id
            if (dump->detailLevel >= iedmDUMP_DETAIL_LEVEL_2)
            {
                iecs_lockClientState(pClient);

                ismEngine_Session_t *pSession = pClient->pSessionHead;

                while (pSession != NULL)
                {
                    dumpSession(pThreadData, pSession, dumpHdl);
                    pSession = pSession->pNext;
                }

                iecs_unlockClientState(pClient);
            }

            iedm_dumpEndGroup(dump);
            iedm_dumpEndObject(dump, pClient);
        }

        iecs_releaseClientStateReference(pThreadData, pClient, false, false);
    }
    else
    {
        rc = ISMRC_NotFound;
    }

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of clientStateUtils.c                                         */
/*                                                                   */
/*********************************************************************/
