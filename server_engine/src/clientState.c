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
/// @file  clientState.c
/// @brief Management of client-state
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "engineStore.h"
#include "clientState.h"
#include "clientStateInternal.h"
#include "clientStateExpiry.h"
#include "destination.h"
#include "memHandler.h"
#include "msgCommon.h"
#include "engineRestore.h"
#include "queueNamespace.h"
#include "topicTree.h"
#include "engineAsync.h"
#include "exportClientState.h"
#include "resourceSetStats.h"

void iecs_SLEReplayAddUnrelMsg(ietrReplayPhase_t phase,
                               ieutThreadData_t *pThreadData,
                               ismEngine_Transaction_t *pTran,
                               void *pEntry,
                               ietrReplayRecord_t *pRecord,
                               ismEngine_DelivererContext_t *delivererContext);

void iecs_SLEReplayRmvUnrelMsg(ietrReplayPhase_t phase,
                               ieutThreadData_t *pThreadData,
                               ismEngine_Transaction_t *pTran,
                               void *pEntry,
                               ietrReplayRecord_t *pRecord,
                               ismEngine_DelivererContext_t * delivererContext);

/*
 * Calculate a hash value for a key string.
 * Key doesn't need to be 64-bit aligned.
 * Based on IBM Research Report RJ3483.
 */
#define HMULT  0x7FFFFFFF7FFFFFC3ul
#define XCONST 0x55CC55CC55CC55CCul

//If there are lots of subs we don't want to cache lots of memory for MDRs each sub. As the per-sub cache is
//large we set the threshold on the number of subscribers fairly low that disables the caching...
#define MAX_SUBS_MDR_MEMCACHE 15000

//If we are above MAX_SUBS_MDR_MEMCACHE then still cache chunks if we expect to send *this* sub lots of messages
//So if maxInflight messages for a client is above this... we always cache chunks
#define MDR_MEMCACHE_ALWAYS_INFLIGHTMSGS 250

//Leave one id reserved for private use
#define MQTT_MAX_DELIVERYID 65534

//data needed when removing an MDR with an asynchronos store commit:
typedef struct tag_iecsUnstoreMDRAsyncData_t  {
    char                             StructId[4]; //IECS_UNSTOREMDR_ASYNCDATA_STRUCID
    iecsMessageDeliveryInfo_t       *pMsgDelInfo;
    iecsMessageDeliveryChunk_t      *pMsgDelChunk;
    iecsMessageDeliveryReference_t  *pMsgDelRef;
    uint32_t deliveryId;
} iecsUnstoreMDRAsyncData_t;

#define IECS_UNSTOREMDR_ASYNCDATA_STRUCID "CSMR"


static uint64_t calculateHash(const char *pKey)
{
    uint64_t hash = 0;
    size_t length = strlen(pKey);

    // Deal with multiples of 8 bytes
    for (; length >= 8; pKey += 8, length -= 8)
    {
        uint64_t u1 =   ((uint64_t)pKey[0] << 56)
                     +  ((uint64_t)pKey[1] << 48)
                     +  ((uint64_t)pKey[2] << 40)
                     +  ((uint64_t)pKey[3] << 32)
                     +  ((uint64_t)pKey[4] << 24)
                     +  ((uint64_t)pKey[5] << 16)
                     +  ((uint64_t)pKey[6] <<  8)
                     +  ((uint64_t)pKey[7] <<  0);

        // Mix
        u1 = u1 ^ XCONST;
        u1 = (u1 << 27) | (u1 >> 37);

        // Multiply by large prime
        hash = (u1 ^ hash) * HMULT;
    }

    // Deal with remaining tail bytes
    if (length > 0)
    {
        uint64_t u2 = 0;
        switch(length)
        {
            // Rearrange bytes so less than 8 are handled better
            case 7:
                u2 |= ((uint64_t)pKey[6] << 32);
                /* no break - fall thru */
            case 6:
                u2 |= ((uint64_t)pKey[5] << 48);
                /* no break - fall thru */
            case 5:
                u2 |= ((uint64_t)pKey[4] << 24);
                /* no break - fall thru */
            case 4:
                u2 |= ((uint64_t)pKey[3] << 16);
                /* no break - fall thru */
            case 3:
                u2 |= ((uint64_t)pKey[2] << 40);
                /* no break - fall thru */
            case 2:
                u2 |= ((uint64_t)pKey[1] <<  0);
                /* no break - fall thru */
            case 1:
                u2 |= ((uint64_t)pKey[0] << 56);
                u2 |= ((uint64_t)pKey[0] <<  8);

                u2 = u2^XCONST;
                u2 = (u2 << 13) | (u2 >> 51);
                hash = (u2 ^ hash) * HMULT;
                break;
        }
    }

    // Do some final mixing
    int i;
    for (i = 0; i < 8; i++)
    {
        uint64_t u3 = hash ^ XCONST;
        hash = (((u3 << 13) | (u3 >> 51)) ^ hash) * HMULT;
    }

    // This is what the C standard claims should happen - check that it does
    assert(((uint32_t)hash) == (hash % 0x100000000));

    return hash;
}

/*
 * Expose the clientId hash calculation to external callers
 */
uint32_t iecs_generateClientIdHash(const char *pKey)
{
    return (uint32_t)(calculateHash(pKey));
}

bool iecs_completeCleanupRemainingResources(ieutThreadData_t *pThreadData,
                                            ismEngine_ClientState_t *pClient,
                                            bool fInline,
                                            bool fInlineThief)
{
    bool fDidRelease = false;
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;
    ismEngine_CompletionCallback_t pPendingDestroyCallbackFn = NULL;
    void *pPendingDestroyContext = NULL;

    bool makeZombie = (pClient->Durability == iecsDurable ||
                       pClient->durableObjects != 0 ||
                       pClient->hWillMessage != NULL) && !pClient->fDiscardDurable;
    if (makeZombie)
    {
        ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);
        if (pClient->pThief != NULL)
        {
            makeZombie = false;
        }
        else
        {
            assert(pClient->hWillMessage == NULL || pClient->WillDelayExpiryTime != 0); // Should have published by now
            pthread_spin_lock(&pClient->UseCountLock);
            if (pClient->OpState != iecsOpStateNonDurableCleanup)
            {
                makeZombie = false;
            }
            else
            {
                pClient->UseCount += 1;
                pClient->OpState = iecsOpStateZombie;
                if (pClient->ExpiryTime != 0) pThreadData->stats.zombieSetToExpireCount += 1;
                pClient->pStealCallbackFn = NULL;
                pClient->pStealContext = NULL;
            }
            pthread_spin_unlock(&pClient->UseCountLock);
        }

        // When the OpState is iecsOpStateZombie, it can be destroyed by a call to
        // ism_engine_destroyDisconnectedClientState which needs the ability to go async.
        //
        // So before we release our lock on the clientState table remember the existing callback,
        // so we can honour the one set on ism_engine_destroyClientState, and clear it ready for
        // reuse.
        if (makeZombie)
        {
            iecs_lockClientState(pClient);

            pPendingDestroyCallbackFn = pClient->pPendingDestroyCallbackFn;
            pPendingDestroyContext = pClient->pPendingDestroyContext;

            pClient->pPendingDestroyCallbackFn = NULL;
            pClient->pPendingDestroyContext = NULL;

            iecs_unlockClientState(pClient);
        }

        ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);
    }

    if (!makeZombie)
    {
        fDidRelease = iecs_releaseClientStateReference(pThreadData, pClient, fInline, fInlineThief);
    }
    else
    {
        fDidRelease = true;

        // If we're not running in-line, we're completing this asynchronously and
        // may have been given a completion callback for the destroy operation
        if (pPendingDestroyCallbackFn != NULL)
        {
            ieutTRACEL(pThreadData, pPendingDestroyCallbackFn, ENGINE_HIGH_TRACE,
                       FUNCTION_IDENT "pPendingDestroyCallbackFn=%p calling=%d\n",
                       __func__, pPendingDestroyCallbackFn, (int)(!fInline));

            if (!fInline)
            {
                pPendingDestroyCallbackFn(OK, NULL, pPendingDestroyContext);
            }
        }

        if (pPendingDestroyContext != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_free(pThreadData, resourceSet, iemem_callbackContext, pPendingDestroyContext);
        }

        iecs_releaseClientStateReference(pThreadData, pClient, fInline, fInlineThief);
    }

    return fDidRelease; 
}

/// @brief Context passed when listing subscriptions to be considered for cleanup
typedef struct tag_iecsCleanupSubContext_t
{
    ismEngine_ClientState_t *pClient; ///< The clientState issuing the request
    uint32_t cleanupCount;            ///< Cleanup requests still in progress
    bool includeDurable;              ///< Whether durable subscriptions should be destroyed
} iecsCleanupSubContext_t;

/*
 * Callback for the asynchronous completion of ism_engine_destroySubscription as
 * part of the cleanup.
 */
static void cleanupSubCompleteCallback(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext)
{
    iecsCleanupSubContext_t *context = *(iecsCleanupSubContext_t **)pContext;

    uint32_t cleanupCount = __sync_sub_and_fetch(&context->cleanupCount, 1);

    if (cleanupCount == 0)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();
        ismEngine_ClientState_t *pClient = context->pClient;

        ism_common_free(ism_memory_engine_misc,context);

        iecs_completeCleanupRemainingResources(pThreadData, pClient, false, false);
    }
}

/*
 * Locally-defined callback function to delete non-durable subscriptions for a client-state
 * which is being destroyed
 */
static void cleanupSubscriptionFn(
        ieutThreadData_t *pThreadData,
        ismEngine_SubscriptionHandle_t subHandle,
        const char *pSubName,
        const char *pTopicString,
        void *properties,
        size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes,
        uint32_t consumerCount,
        void *pContext)
{
    iecsCleanupSubContext_t *context = (iecsCleanupSubContext_t *)pContext;
    ismEngine_ClientState_t *pClient = context->pClient;
    uint32_t subOptions = pSubAttributes->subOptions;

    int32_t rc;

    __sync_fetch_and_add(&context->cleanupCount, 1);

    if (context->includeDurable == true ||
       (subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) == 0)
    {
        rc = ism_engine_destroySubscription(pClient, pSubName, pClient, &context, sizeof(context), cleanupSubCompleteCallback);
    }
    else
    {
        rc = OK;
    }

    // We didn't go asynchronous so call the callback function directly to complete operation
    if (rc != ISMRC_AsyncCompletion)
    {
        cleanupSubCompleteCallback(rc, NULL, &context);
    }
}

/*
 * Check authority for the specified subscription
 */
typedef struct tag_checkSubAuthContext_t
{
    ismEngine_ClientState_t *pClient;
} checkSubAuthContext_t;

static void checkSubAuthFn(
        ieutThreadData_t *pThreadData,
        ismEngine_SubscriptionHandle_t subHandle,
        const char *pSubName,
        const char *pTopicString,
        void *properties,
        size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes,
        uint32_t consumerCount,
        void *pContext)
{
    int32_t rc = OK;
    ismEngine_Subscription_t *pSubscription = (ismEngine_Subscription_t *)subHandle;
    checkSubAuthContext_t *context = (checkSubAuthContext_t *)pContext;
    uint32_t subOptions = pSubAttributes->subOptions;

    // Don't do revalidation for global shared subscriptions
    if (   ((subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) == 0)
        || (strcmp(context->pClient->pClientId, pSubscription->clientId) == 0))
    {
        iepiPolicyInfo_t *pValidatedPolicyInfo = NULL;

        // Check the authority for this subscription topic
        rc = ismEngine_security_validate_policy_func(context->pClient->pSecContext,
                                                     ismSEC_AUTH_TOPIC,
                                                     pTopicString,
                                                     ismSEC_AUTH_ACTION_SUBSCRIBE,
                                                     ISM_CONFIG_COMP_ENGINE,
                                                     (void **)&pValidatedPolicyInfo);

        // Take the policy info we just got and update the subscription (which might be a no-op)
        if (rc == OK)
        {
            rc = iett_setSubscriptionPolicyInfo(pThreadData,
                                                subHandle,
                                                pValidatedPolicyInfo);

            assert(rc == OK);
        }
        else
        {
            ieutTRACEL(pThreadData, rc, ENGINE_INTERESTING_TRACE,
                       "Policy revalidation for sub %p name '%s' failed, rc=%d\n",
                       subHandle, ((ismEngine_Subscription_t *)subHandle)->subName, rc);
        }
    }
}

/*
 * Performs revalidation of all subscriptions currently owned by a client
 */
void iecs_revalidateSubscriptions(ieutThreadData_t *pThreadData,
                                  ismEngine_ClientState_t *pClient)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pClient, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pClient=%p\n", __func__, pClient);

    if (pClient->protocolId != PROTOCOL_ID_SHARED)
    {
        checkSubAuthContext_t context;

        context.pClient = pClient;

        rc = iett_listSubscriptions(pThreadData,
                                    pClient->pClientId,
                                    iettFLAG_LISTSUBS_NONE,
                                    NULL,
                                    &context,
                                    checkSubAuthFn);
        if (rc != OK)
        {
            ieutTRACE_FFDC(ieutPROBE_001, false,
                           "Checking authority on subscriptions for client failed", rc,
                           "pClient", pClient, sizeof(ismEngine_ClientState_t),
                           NULL);
            ism_common_setError(rc);
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
}

/*
 * Locks the message-delivery information for a client-state
 */
static void iecs_lockMessageDeliveryInfo(iecsMessageDeliveryInfo_t *pMsgDelInfo)
{
    ismEngine_lockMutex(&pMsgDelInfo->Mutex);
}


/*
 * Unlocks the message-delivery information for a client-state
 */
static void iecs_unlockMessageDeliveryInfo(iecsMessageDeliveryInfo_t *pMsgDelInfo)
{
    ismEngine_unlockMutex(&pMsgDelInfo->Mutex);
}


/*
 * Assign the next available delivery id for a given client
 */
static inline int32_t iecs_assignDeliveryId_internal(ieutThreadData_t *pThreadData,
                                                     iecsMessageDeliveryInfo_t *pMsgDelInfo,
                                                     ismEngine_Session_t *pSession,
                                                     ismStore_Handle_t hStoreRecord,
                                                     ismQHandle_t hQueue,
                                                     void *hNode,
                                                     bool fHandleIsPointer,
                                                     uint32_t *pDeliveryId,
                                                     iecsMessageDeliveryChunk_t **ppMsgDelChunk,
                                                     iecsMessageDeliveryReference_t **ppMsgDelRef)
{
    iecsMessageDeliveryChunk_t *pMsgDelChunk = NULL;
    iecsMessageDeliveryReference_t *pMsgDelRef = NULL;
    uint32_t deliveryId = 0;
    int32_t rc = ISMRC_OK;

    // Check the existing chunks for the supplied delivery ID. If we already have this ID, it's an
    // error since we don't expect delivery IDs for a message to be updated
    uint32_t idsChecked;

    if (pMsgDelInfo->NumDeliveryIds >= pMsgDelInfo->MaxInflightMsgs)
    {
        rc = ISMRC_MaxDeliveryIds;

        //Mark the whole session as now under engine control
        ies_MarkSessionEngineControl(pSession);
        pMsgDelInfo->fIdsExhausted = true;

        goto mod_exit;
    }

    //At most, we should only need to look at iecsMaxInflightMsgs+1 slot to find an empty slot....
    uint32_t idsToCheck = pMsgDelInfo->MaxInflightMsgs+1;

    uint32_t MdrChunkSize = pMsgDelInfo->MdrChunkSize;

    for (idsChecked = 0, deliveryId = pMsgDelInfo->NextDeliveryId; idsChecked < idsToCheck; idsChecked++, deliveryId++)
    {
        if (deliveryId > pMsgDelInfo->MaxDeliveryId)
        {
            deliveryId = pMsgDelInfo->BaseDeliveryId;
        }

        pMsgDelChunk = pMsgDelInfo->pChunk[deliveryId / MdrChunkSize];
        if (pMsgDelChunk != NULL)
        {
            pMsgDelRef = pMsgDelChunk->Slot + (deliveryId % MdrChunkSize);
            if (!pMsgDelRef->fSlotInUse)
            {
                pMsgDelRef->fSlotInUse = true;
                pMsgDelRef->DeliveryId = deliveryId;
                pMsgDelRef->fSlotPending = true;
                pMsgDelChunk->slotsInUse++;
                pMsgDelInfo->NumDeliveryIds++;
                break;
            }
            else
            {
                pMsgDelRef = NULL;
            }
        }
        else
        {
            break;
        }
    }

    if (idsChecked == idsToCheck)
    {
        //We shouldn't get here... we've found more full slots than there are allowed to e messages in flight... FFDC but continue
        ieutTRACE_FFDC(ieutPROBE_001, false,
                       "Couldn't find slot for MDR despite not hitting limit of messages in flight", rc,
                       "MsgDelInfo", pMsgDelInfo, sizeof(*pMsgDelInfo),
                       NULL);


        rc = ISMRC_MaxDeliveryIds;
        ism_common_setError(rc);

        //Mark the whole session as now under engine control
        ies_MarkSessionEngineControl(pSession);
        pMsgDelInfo->fIdsExhausted = true;

        goto mod_exit;
    }

    // If we don't yet have storage allocated for this MDR, allocate it now
    if (pMsgDelRef == NULL)
    {
        if (pMsgDelInfo->pFreeChunk1 != NULL)
        {
            pMsgDelChunk = pMsgDelInfo->pFreeChunk1;
            pMsgDelInfo->pFreeChunk1 = NULL;
        }
        else if (pMsgDelInfo->pFreeChunk2 != NULL)
        {
            pMsgDelChunk = pMsgDelInfo->pFreeChunk2;
            pMsgDelInfo->pFreeChunk2 = NULL;
        }
        else
        {
            iereResourceSetHandle_t resourceSet = pMsgDelInfo->resourceSet;

            iere_primeThreadCache(pThreadData, resourceSet);
            pMsgDelChunk = iere_calloc(pThreadData,
                                       resourceSet,
                                       IEMEM_PROBE(iemem_clientState, 18), 1,
                                       sizeof(iecsMessageDeliveryChunk_t) + (MdrChunkSize)*sizeof(iecsMessageDeliveryReference_t));

            if (pMsgDelChunk == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }
        }

        pMsgDelInfo->pChunk[deliveryId / MdrChunkSize] = pMsgDelChunk;
        pMsgDelInfo->ChunksInUse++;

        pMsgDelRef = pMsgDelChunk->Slot + (deliveryId % MdrChunkSize);

        pMsgDelRef->fSlotInUse = true;
        pMsgDelRef->DeliveryId = deliveryId;
        pMsgDelRef->fSlotPending = true;
        pMsgDelChunk->slotsInUse = 1;
        pMsgDelInfo->NumDeliveryIds++;
    }

    if (pMsgDelInfo->NextDeliveryId == pMsgDelInfo->MaxDeliveryId)
    {
        pMsgDelInfo->NextDeliveryId = pMsgDelInfo->BaseDeliveryId;
    }
    else
    {
        pMsgDelInfo->NextDeliveryId++;
    }

    pMsgDelRef->fHandleIsPointer = fHandleIsPointer;
    pMsgDelRef->hStoreMsgDeliveryRef1 = ismSTORE_NULL_HANDLE;
    pMsgDelRef->MsgDeliveryRef1OrderId = 0;
    pMsgDelRef->hStoreMsgDeliveryRef2 = ismSTORE_NULL_HANDLE;
    pMsgDelRef->MsgDeliveryRef2OrderId = 0;
    pMsgDelRef->hStoreRecord = hStoreRecord;
    pMsgDelRef->hQueue = hQueue;
    pMsgDelRef->hNode = hNode;
    pMsgDelRef->hStoreMessage = ismSTORE_NULL_HANDLE;

    *pDeliveryId = deliveryId;
    if (ppMsgDelChunk != NULL)
    {
        *ppMsgDelChunk = pMsgDelChunk;
    }
    if (ppMsgDelRef   != NULL)
    {
        *ppMsgDelRef = pMsgDelRef;
    }
    else
    {
        // They must have have no further need to initialise the slot as they haven't asked for it back
        pMsgDelRef->fSlotPending = false;
    }

mod_exit:
    return rc;
}

/*
 * Remove a deliveryId for a client.
 */
static inline int32_t iecs_releaseDeliveryId_internal(ieutThreadData_t *pThreadData,
                                                      iecsMessageDeliveryInfo_t *pMsgDelInfo,
                                                      uint32_t deliveryId,
                                                      iecsMessageDeliveryChunk_t *pMsgDelChunk,
                                                      iecsMessageDeliveryReference_t *pMsgDelRef)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, deliveryId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "(pMsgDelInfo %p, deliveryId %u)\n", __func__, pMsgDelInfo, deliveryId);

    if (pMsgDelChunk == NULL)
    {
        // Check the existing chunks for the supplied delivery ID
        pMsgDelChunk = pMsgDelInfo->pChunk[deliveryId / pMsgDelInfo->MdrChunkSize];
        if (pMsgDelChunk != NULL)
        {
            pMsgDelRef = pMsgDelChunk->Slot + (deliveryId % pMsgDelInfo->MdrChunkSize);

            if (pMsgDelRef->fSlotInUse && !pMsgDelRef->fSlotPending)
            {
                assert(pMsgDelRef->DeliveryId == deliveryId);
            }
            else
            {
                pMsgDelRef = NULL;
            }
        }
        else
        {
            pMsgDelRef = NULL;
        }

        if (pMsgDelRef == NULL)
        {
            rc = ISMRC_NotFound;
            ism_common_setError(rc);
            ieutTRACE_FFDC(ieutPROBE_002, false,
                           "Releasing unknown deliveryid", rc,
                           "Delivery ID", &deliveryId, sizeof(deliveryId),
                           NULL);
            goto mod_exit;
        }
    }

    assert((pMsgDelInfo != NULL) && (pMsgDelRef != NULL) && (pMsgDelChunk != NULL));

    pMsgDelRef->fSlotInUse = false;
    pMsgDelRef->DeliveryId = 0;
    assert(pMsgDelChunk->slotsInUse > 0);
    pMsgDelChunk->slotsInUse--;
    assert(pMsgDelInfo->NumDeliveryIds > 0);
    pMsgDelInfo->NumDeliveryIds--;

    if (pMsgDelChunk->slotsInUse == 0)
    {
        pMsgDelInfo->pChunk[deliveryId / pMsgDelInfo->MdrChunkSize] = NULL;
        pMsgDelInfo->ChunksInUse--;


        //Cache this memory if there aren't that many subs or if this subs expected large numbers of messages...
        //....if there are lots of subs we waste too much in caching for ever sub
        bool cachedThisMem = false;
        if (   (ismEngine_serverGlobal.totalSubsCount < MAX_SUBS_MDR_MEMCACHE)
            || (pMsgDelInfo->MaxInflightMsgs > MDR_MEMCACHE_ALWAYS_INFLIGHTMSGS) )
        {
            if (pMsgDelInfo->pFreeChunk1 == NULL)
            {
                pMsgDelInfo->pFreeChunk1 = pMsgDelChunk;
                cachedThisMem = true;
            }
            else if (pMsgDelInfo->pFreeChunk2 == NULL)
            {
                pMsgDelInfo->pFreeChunk2 = pMsgDelChunk;
                cachedThisMem = true;
            }
        }
        if(!cachedThisMem)
        {
            iereResourceSetHandle_t resourceSet = pMsgDelInfo->resourceSet;

            iere_primeThreadCache(pThreadData, resourceSet);
            iere_free(pThreadData, resourceSet, iemem_clientState, pMsgDelChunk);
        }
    }

    if(pMsgDelInfo->fIdsExhausted && pMsgDelInfo->NumDeliveryIds <= pMsgDelInfo->InflightReenable)
    {
        rc = ISMRC_DeliveryIdAvailable; //We can now start sending messages again
        pMsgDelInfo->fIdsExhausted = false;
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

/*
 * Prepare the store record structures for a CPR
 */
void iecs_prepareCPR(iestClientPropertiesRecord_t *pCPR,
                     ismStore_Record_t *pStoreRecord,
                     ismEngine_ClientState_t *pClient,
                     char *willTopic,
                     ismStore_Handle_t willMsgStoreHdl,
                     uint32_t willMsgTimeToLive,
                     uint32_t willDelay,
                     char **ppFragments,
                     uint32_t *pFragmentLengths)
{
    // First fragment is always the CPR itself
    ppFragments[0] = (char *)pCPR;
    pFragmentLengths[0] = sizeof(*pCPR);

    // Fill in the store record
    pStoreRecord->Type = ISM_STORE_RECTYPE_CPROP;
    pStoreRecord->Attribute = willMsgStoreHdl;
    pStoreRecord->State = iestCPR_STATE_NONE;
    pStoreRecord->pFrags = ppFragments;
    pStoreRecord->pFragsLengths = pFragmentLengths;
    pStoreRecord->FragsCount = 1;
    pStoreRecord->DataLength = pFragmentLengths[0];

    assert(pClient->Durability == iecsDurable || pClient->Durability == iecsNonDurable);

    // Build the client properties record from the various fragment sources
    memcpy(pCPR->StrucId, iestCLIENT_PROPS_RECORD_STRUCID, 4);
    pCPR->Version = iestCPR_CURRENT_VERSION;
    pCPR->Flags = (pClient->Durability == iecsDurable) ? iestCPR_FLAGS_DURABLE : iestCPR_FLAGS_NONE;

    // Include the will message topic string, time to live and delay if there is a will message
    if (willMsgStoreHdl != ismSTORE_NULL_HANDLE)
    {
        pCPR->WillTopicNameLength = strlen(willTopic) + 1;
        ppFragments[pStoreRecord->FragsCount] = willTopic;
        pFragmentLengths[pStoreRecord->FragsCount] = pCPR->WillTopicNameLength;
        pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
        pStoreRecord->FragsCount += 1;
        pCPR->WillMsgTimeToLive = willMsgTimeToLive;
        pCPR->WillDelay = willDelay;
    }
    else
    {
        pCPR->WillTopicNameLength = 0;
        pCPR->WillMsgTimeToLive = 0;
        pCPR->WillDelay = 0;
    }

    // Include the userId if there is one
    if (pClient->pUserId != NULL)
    {
        pCPR->UserIdLength = strlen(pClient->pUserId) + 1;
        ppFragments[pStoreRecord->FragsCount] = pClient->pUserId;
        pFragmentLengths[pStoreRecord->FragsCount] = pCPR->UserIdLength;
        pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
        pStoreRecord->FragsCount += 1;
    }
    else
    {
        pCPR->UserIdLength = 0;
    }

    pCPR->ExpiryInterval = pClient->ExpiryInterval;
}

/*
 * Prepare the store record structures for a CSR
 */
void iecs_prepareCSR(iestClientStateRecord_t *pCSR,
                     ismStore_Record_t *pStoreRecord,
                     ismEngine_ClientState_t *pClient,
                     ismStore_Handle_t hStoreCPR,
                     bool fFromImport,
                     char **ppFragments,
                     uint32_t *pFragmentLengths)
{
    // First fragment is always the CSR itself
    ppFragments[0] = (char *)pCSR;
    pFragmentLengths[0] = sizeof(*pCSR);

    // Fill in the store record
    pStoreRecord->Type = ISM_STORE_RECTYPE_CLIENT;
    pStoreRecord->Attribute = hStoreCPR;
    if (fFromImport)
    {
        uint64_t expireLastConnectedTime = (uint64_t)ism_common_getExpire(pClient->LastConnectedTime);
        pStoreRecord->State = (expireLastConnectedTime << 32) | iestCSR_STATE_DISCONNECTED;
    }
    else
    {
        pStoreRecord->State = iestCSR_STATE_NONE;
    }
    pStoreRecord->pFrags = ppFragments;
    pStoreRecord->pFragsLengths = pFragmentLengths;
    pStoreRecord->FragsCount = 1;
    pStoreRecord->DataLength = pFragmentLengths[0];

    assert(pClient->Durability == iecsDurable || pClient->Durability == iecsNonDurable);

    // Build the client state record from the various fragment sources
    memcpy(pCSR->StrucId, iestCLIENT_STATE_RECORD_STRUCID, 4);
    pCSR->Version = iestCSR_CURRENT_VERSION;
    pCSR->Flags = (pClient->Durability == iecsDurable) ? iestCSR_FLAGS_DURABLE : iestCSR_FLAGS_NONE;
    pCSR->protocolId = pClient->protocolId;

    pCSR->ClientIdLength = strlen(pClient->pClientId) + 1;
    ppFragments[pStoreRecord->FragsCount] = pClient->pClientId;
    pFragmentLengths[pStoreRecord->FragsCount] = pCSR->ClientIdLength;
    pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
    pStoreRecord->FragsCount += 1;

    assert(pStoreRecord->FragsCount == 2);
}

/*
 * Update the CPR in the store and update the CSR to point to it
 */
int32_t iecs_updateClientPropsRecord(ieutThreadData_t *pThreadData,
                                     ismEngine_ClientState_t *pClient,
                                     char *willTopicName,
                                     ismStore_Handle_t willMsgStoreHdl,
                                     uint32_t willMsgTimeToLive,
                                     uint32_t willDelay)
{
    uint32_t storeUpdates;
    int32_t rc = OK;

retry_update:

    storeUpdates = 0;

    // Only ever expect to be called with a valid CSR
    assert(pClient->hStoreCSR != ismSTORE_NULL_HANDLE);

    // Start out by deleting any existing CPR (may not have one)
    if (pClient->hStoreCPR != ismSTORE_NULL_HANDLE)
    {
        rc = ism_store_deleteRecord(pThreadData->hStream, pClient->hStoreCPR);

        if (rc != OK)
        {
            assert(rc != ISMRC_StoreGenerationFull);

            ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                       "%s: ism_store_deleteRecord (CPR) failed! (rc=%d)\n", __func__, rc);

            goto mod_exit;
        }

        storeUpdates++;
    }

    ismStore_Handle_t hStoreCPR = ismSTORE_NULL_HANDLE;
    ismStore_Record_t storeRecord;
    iestClientPropertiesRecord_t clientPropsRec;
    char *pFrags[3];
    uint32_t fragsLength[3];

    iecs_prepareCPR(&clientPropsRec,
                    &storeRecord,
                    pClient,
                    willTopicName,
                    willMsgStoreHdl,
                    willMsgTimeToLive,
                    willDelay,
                    pFrags, fragsLength);

    // Create the CPR
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreCPR);

    if (rc != OK)
    {
        if (storeUpdates != 0) iest_store_rollback(pThreadData, false);

        if (rc == ISMRC_StoreGenerationFull) goto retry_update;

        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: ism_store_createRecord (CPR) failed! (rc=%d)\n", __func__, rc);

        goto mod_exit;
    }

    storeUpdates++;

    // Update the CSR
    rc = ism_store_updateRecord(pThreadData->hStream,
                                pClient->hStoreCSR,
                                hStoreCPR,
                                iestCSR_STATE_NONE,
                                ismSTORE_SET_ATTRIBUTE);

    if (rc != OK)
    {
        assert(rc != ISMRC_StoreGenerationFull);
        assert(storeUpdates != 0);

        iest_store_rollback(pThreadData, false);

        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: ism_store_updateRecord (CSR) failed! (rc=%d)\n", __func__, rc);

        goto mod_exit;
    }

    // Commit any store updates
    if (storeUpdates != 0) iest_store_commit(pThreadData, false);

    // Update the client structure with the new CPR
    pClient->hStoreCPR = hStoreCPR;

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief  Last action on an asynchronous addition of a clientState
///
/// @param[in]     rc                     Return code from the previous phase
/// @param[in]     asyncInfo              AsyncData for this call
/// @param[in]     context                AsyncData entry for this call.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t iecs_asyncFinishClientStateAddition(ieutThreadData_t *pThreadData,
                                                   int32_t rc,
                                                   ismEngine_AsyncData_t *asyncInfo,
                                                   ismEngine_AsyncDataEntry_t *context)
{
    assert(context->Type == ClientStateFinishAdditionInfo);

    iead_popAsyncCallback(asyncInfo, context->DataLen);

    if (context->Handle != NULL)
    {
        assert(context->Handle == asyncInfo->pClient);
        iecs_releaseClientStateReference(pThreadData, asyncInfo->pClient, false, false);
    }
    return rc;
}

//****************************************************************************
/// @brief  Finish the actual addition of a clientState
///
/// @param[in]     additionInfo           Information about the addition request
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
typedef struct tag_iecsClientStateAdditionInfo_t
{
    ismEngine_ClientState_t *pClient;            ///> Client state being added
    ismEngine_ClientState_t *pVictim;            ///> Victim of a steal or Zombie takeover request
    ismEngine_StealCallback_t pStealCallbackFn;  ///> Steal callback function to use
    void *pStealContext;                         ///> Steal context to use with steal function
} iecsClientStateAdditionInfo_t;

static int32_t iecs_finishClientStateAddition(ieutThreadData_t *pThreadData,
                                              iecsClientStateAdditionInfo_t *additionInfo,
                                              bool fInline,
                                              bool fInlineThief)
{
    int32_t rc = OK;
    ismEngine_ClientState_t *pClient = additionInfo->pClient;
    ismEngine_ClientState_t *pVictim = additionInfo->pVictim;

    ieutTRACEL(pThreadData, pClient, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "(pClient %p, pVictim %p, pStealCallbackFn=%p)\n",
               __func__, pClient, pVictim, additionInfo->pStealCallbackFn);

    // We've got a victim - so we need to potentially invoke a steal callback and release the useCount we took
    if (pVictim != NULL)
    {
        ieutTRACEL(pThreadData, pVictim, ENGINE_HIGH_TRACE, "pVictim->UseCount=%u, pVictim->OpState=%d\n",
                   pVictim->UseCount, (int)pVictim->OpState);

        if (additionInfo->pStealCallbackFn != NULL)
        {
            int32_t reason;

            if (pClient->Takeover == iecsNonAckingClient)
            {
                 reason = ISMRC_NonAckingClient;
            }
            else
            {
                reason = ISMRC_ResumedClientState;
            }

            additionInfo->pStealCallbackFn(reason,
                                           pVictim,
                                           ismENGINE_STEAL_CALLBACK_OPTION_NONE,
                                           additionInfo->pStealContext);
        }

        bool fReleasedClientState = iecs_releaseClientStateReference(pThreadData, pVictim, fInline, fInlineThief);

        if (!fReleasedClientState)
        {
            rc = ISMRC_AsyncCompletion;
        }
        else if (pClient->Takeover == iecsNoTakeover)
        {
            rc = ISMRC_OK;
        }
        else
        {
            rc = ISMRC_ResumedClientState;
        }
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "(pClient %p) rc=%d\n", __func__,
               additionInfo->pClient, rc);

    return rc;
}

//****************************************************************************
/// @brief  Async callback to finish the process of adding a client state.
///
/// @param[in]     rc                     Return code from the previous phase
/// @param[in]     asyncInfo              AsyncData for this call
/// @param[in]     context                AsyncData entry for this call.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
static int32_t iecs_asyncAddClientStateCompletion(ieutThreadData_t *pThreadData,
                                                  int32_t rc,
                                                  ismEngine_AsyncData_t *asyncInfo,
                                                  ismEngine_AsyncDataEntry_t *context)
{
    assert(context->Type == ClientStateAdditionCompletionInfo);

    iecsClientStateAdditionInfo_t *pInfo = (iecsClientStateAdditionInfo_t *)(context->Data);

    assert(pInfo->pClient == asyncInfo->pClient);

    iead_popAsyncCallback(asyncInfo, context->DataLen);

    rc = iecs_finishClientStateAddition(pThreadData, pInfo, false, false);

    if (rc == OK)
    {
        // This is not a steal, or a zombie takeover because there was no victim, that being
        // so, we need to set the handle so that the async callback will pass it to the caller.
        if (pInfo->pVictim == NULL)
        {
            assert(asyncInfo->numEntriesUsed == 2);
            assert(asyncInfo->entries[asyncInfo->numEntriesUsed-1].Type == EngineCaller);

            iead_setEngineCallerHandle(asyncInfo, pInfo->pClient);
        }
        else
        {
            assert(asyncInfo->numEntriesUsed == 1);
            assert(asyncInfo->entries[asyncInfo->numEntriesUsed-1].Type == ClientStateFinishAdditionInfo);
            rc = ISMRC_ResumedClientState;
        }
    }
    // If the finish call ended with ISMRC_AsyncCompletion, then the only thing
    // left for us to do is to release the clientState - which will happen next
    // if we return anything other than ISMRC_AsyncCompletion to our caller (and
    // ISMRC_ResumedClientState is appropriate)
    else
    {
        assert(rc == ISMRC_AsyncCompletion || rc == ISMRC_ResumedClientState);
        assert(asyncInfo->numEntriesUsed == 1);
        assert(asyncInfo->entries[asyncInfo->numEntriesUsed-1].Type == ClientStateFinishAdditionInfo);
        rc = ISMRC_ResumedClientState;
    }

    return rc;
}

/*
 * Set the in-memory copy of the last connected, expiry and will delay expiry times.
 */
void iecs_setLCTandExpiry(ieutThreadData_t *pThreadData,
                          ismEngine_ClientState_t *pClient,
                          ism_time_t now,
                          ism_time_t *pCheckScheduleTime)
{
    ism_time_t checkScheduleTime;
    pClient->LastConnectedTime = now;
    if (now == 0 || pClient->ExpiryInterval == iecsEXPIRY_INTERVAL_INFINITE)
    {
        pClient->ExpiryTime = 0;
    }
    else
    {
        pClient->ExpiryTime = pClient->LastConnectedTime + ((ism_time_t)pClient->ExpiryInterval * 1000000000);
    }

    // checkScheduleTime is whatever the ExpiryTime is set to at this point
    checkScheduleTime = pClient->ExpiryTime;

    // If there is a delayed will message, we might need to adjust the time we schedule an
    // expiry scan for (to publish that delayed will message).
    if (now != 0 && pClient->WillDelay != 0 && pClient->hWillMessage)
    {
        pClient->WillDelayExpiryTime = pClient->LastConnectedTime + ((ism_time_t)pClient->WillDelay * 1000000000);

        // If no ExpiryTime is set use WillDelayExpiryTime as the schedule check time, and
        // if this is a nondurable client with no durable objects, report WillDelayExpiryTime
        // as its ExpiryTime too (so that monitoring looks sensible).
        if (checkScheduleTime == 0)
        {
            checkScheduleTime = pClient->WillDelayExpiryTime;

            if (pClient->Durability == iecsNonDurable && pClient->durableObjects == 0)
            {
                pClient->ExpiryTime = pClient->WillDelayExpiryTime;
            }
        }
        // If the the WillDelayExpiryTime is before ExpiryTime (as expected) use
        // WillDelayExpiryTime as the schedule check time.
        if (pClient->WillDelayExpiryTime < checkScheduleTime)
        {
            checkScheduleTime = pClient->WillDelayExpiryTime;
        }
        // If the WillDelayExpiryTime is *later* than the ExpiryTime, we override the WillDelayExpiryTime to be
        // ExpiryTime (will message will be sent when the clientState is expired)
        else if (pClient->WillDelayExpiryTime > checkScheduleTime)
        {
            assert(pClient->ExpiryTime != 0);
            assert(checkScheduleTime == pClient->ExpiryTime);

            ieutTRACEL(pThreadData, pClient->WillDelay, ENGINE_HIFREQ_FNC_TRACE,
                       FUNCTION_IDENT "pClient=%p WillDelay (%u) higher than ExpiryInterval (%u) - setting both to ExpiryTime.\n",
                       __func__, pClient, pClient->WillDelay, pClient->ExpiryInterval);

            pClient->WillDelayExpiryTime = pClient->ExpiryTime;
        }
    }

    if (pCheckScheduleTime != NULL)
    {
        *pCheckScheduleTime = checkScheduleTime;
    }
    else if (checkScheduleTime != 0)
    {
        iece_checkTimeWithScheduledScan(pThreadData, checkScheduleTime);
    }
}

/*
 * Update the CSR in the Store with a new last-connected time
 */
int32_t iecs_updateLastConnectedTime(ieutThreadData_t *pThreadData,
                                     ismEngine_ClientState_t *pClient,
                                     bool fIsConnected,
                                     ismEngine_AsyncData_t *pAsyncData)
{
    uint64_t newState;
    int32_t rc = OK;

    if (fIsConnected)
    {
        // Connected. Clear out the CSR's last-connected time.
        iecs_setLCTandExpiry(pThreadData, pClient, 0, NULL);
        newState = iestCSR_STATE_NONE;
    }
    else
    {
        // Not connected. Get the current time and write it into the upper 32 bits of the CSR's state.
        uint32_t now = ism_common_nowExpire();
        iecs_setLCTandExpiry(pThreadData, pClient, ism_common_convertExpireToTime(now), NULL);
        newState = ((uint64_t)now << 32) | iestCSR_STATE_DISCONNECTED;
    }

    if (pClient->hStoreCSR != ismSTORE_NULL_HANDLE)
    {
        // If we might go async, add a trace line as an indication that this is why
        if (pAsyncData != NULL)
        {
            ieutTRACEL(pThreadData, pClient, ENGINE_HIFREQ_FNC_TRACE,
                       FUNCTION_IDENT "pClient=%p hStoreCSR=0x%016lx newState=0x%016lx\n",
                       __func__, pClient, pClient->hStoreCSR, newState);
        }

        // Update the state to reflect the last-connected time.
        // We are NOT setting the attribute to be a null handle value.
        rc = ism_store_updateRecord(pThreadData->hStream,
                                    pClient->hStoreCSR,
                                    ismSTORE_NULL_HANDLE,
                                    newState,
                                    ismSTORE_SET_STATE);
        if (rc == OK)
        {
            rc = iead_store_asyncCommit(pThreadData, false, pAsyncData);
        }
        else
        {
            assert(rc != ISMRC_StoreGenerationFull);

            iest_store_rollback(pThreadData, false);

            ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                       "%s: ism_store_updateRecord (CSR) failed! (rc=%d)\n", __func__, rc);
        }
    }

    return rc;
}

/*
 * Set the expiry times from the existing LastConnectedTime (and expiryInterval and willDelay)
 */
void iecs_setExpiryFromLastConnectedTime(ieutThreadData_t *pThreadData,
                                         ismEngine_ClientState_t *pClient,
                                         ism_time_t *pCheckScheduleTime)
{
    iecs_setLCTandExpiry(pThreadData, pClient, pClient->LastConnectedTime, pCheckScheduleTime);
}

/*
 * Create the client-state table
 */
int32_t iecs_createClientStateTable(ieutThreadData_t *pThreadData)
{
    iecsHashTable_t *pTable;
    int32_t rc = OK;

    pTable = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_clientState, 1), sizeof(iecsHashTable_t));
    if (pTable != NULL)
    {
        memcpy(pTable->StrucId, iecsHASH_TABLE_STRUCID, 4);
        pTable->Generation = 1;
        pTable->TotalEntryCount = 0;
        pTable->ChainCount = 1 << iecsHASH_TABLE_SHIFT_INITIAL;
        pTable->ChainMask = pTable->ChainCount - 1;
        pTable->ChainCountMax = 1 << iecsHASH_TABLE_SHIFT_MAX;
        pTable->fCanResize = true;
        pTable->pChains = NULL;

        iecsHashChain_t *pChain;
        pChain = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_clientState, 2), pTable->ChainCount, sizeof(iecsHashChain_t));
        if (pChain != NULL)
        {
            pTable->pChains = pChain;

            ieutTRACEL(pThreadData, pTable->ChainCount, ENGINE_HIGH_TRACE,
                       "Initial client-state table size is %u.\n", pTable->ChainCount);
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    if (rc == OK)
    {
        ismEngine_serverGlobal.ClientTable = pTable;
    }
    else
    {
        iecs_freeClientStateTable(pThreadData, pTable, false);
    }

    return rc;
}

/*
 * Destroy the client-state table, and any remaining client-states
 */
void iecs_destroyClientStateTable(ieutThreadData_t *pThreadData)
{
    iecsHashTable_t *pTable = ismEngine_serverGlobal.ClientTable;

    ieutTRACEL(pThreadData, pTable, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "pTable=%p\n", __func__, pTable);

    if (pTable != NULL)
    {
        iecs_freeClientStateTable(pThreadData, pTable, true);
        ismEngine_serverGlobal.ClientTable = NULL;
    }

    ieutTRACEL(pThreadData, pTable, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);
}


/*
 * Traverse the client-state table
 */
int32_t iecs_traverseClientStateTable(ieutThreadData_t *pThreadData,
                                      uint32_t *tableGeneration,
                                      uint32_t startChain,
                                      uint32_t maxChains,
                                      uint32_t *lastChain,
                                      iecsTraverseCallback_t callback,
                                      void *context)
{
    int32_t rc = OK;

    ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);

    iecsHashTable_t *pTable = ismEngine_serverGlobal.ClientTable;

    if (pTable != NULL)
    {
        if (tableGeneration != NULL)
        {
            if (*tableGeneration != 0 && *tableGeneration != pTable->Generation)
            {
                // Not an error - the table has changed.
                rc = ISMRC_ClientTableGenMismatch;
            }
            else
            {
                *tableGeneration = pTable->Generation;
            }
        }

        if (rc == OK && pTable->pChains != NULL)
        {
            uint32_t endChain;

            // Work out which chain to end at.
            if (maxChains == 0)
            {
                endChain = pTable->ChainCount;
            }
            else
            {
                endChain = startChain + maxChains;

                if (endChain > pTable->ChainCount)
                {
                    endChain = pTable->ChainCount;
                }
            }

            iecsHashChain_t *pChain = &pTable->pChains[startChain];
            bool fContinue = true;
            uint32_t i;
            for (i = startChain; i < endChain; i++, pChain++)
            {
                iecsHashEntry_t *pEntry = pChain->pEntries;
                if (pEntry != NULL)
                {
                    uint32_t j;
                    for (j = 0; j < pChain->Limit; j++, pEntry++)
                    {
                        if (pEntry->pValue != NULL)
                        {
                            fContinue = callback(pThreadData, pEntry->pValue, context);

                            // Leave with i set to the current chain
                            if (!fContinue) break;
                        }
                    }

                    // Leave with i set to current chain
                    if (!fContinue) break;
                }
            }

            // If we didn't get to the end of the chains, tell the caller
            if (i < pTable->ChainCount)
            {
                // Not an error - there are further chains available
                rc = ISMRC_MoreChainsAvailable;

                if (lastChain != NULL) *lastChain = i;
            }
        }
    }

    ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

    ieutTRACEL(pThreadData, rc, ENGINE_HIGH_TRACE, FUNCTION_IDENT "rc=%d\n", __func__, rc);
    return rc;
}


/*
 * Free the supplied client-state table
 */
void iecs_freeClientStateTable(ieutThreadData_t *pThreadData, iecsHashTable_t *pTable, bool fFreeClientStates)
{
    if (pTable != NULL)
    {
        iecsHashChain_t *pChain = pTable->pChains;
        if (pChain != NULL)
        {
            uint32_t i;
            for (i = 0; i < pTable->ChainCount; i++, pChain++)
            {
                iecsHashEntry_t *pEntry = pChain->pEntries;
                if (pEntry != NULL)
                {
                    if (fFreeClientStates)
                    {
                        uint32_t j;
                        for (j = 0; j < pChain->Limit; j++, pEntry++)
                        {
                            if (pEntry->pValue != NULL)
                            {
                                ismEngine_ClientState_t *pClient = pEntry->pValue;
                                pEntry->pValue = NULL;
                                pClient->pHashEntry = NULL;
                                iecs_freeClientState(pThreadData, pClient, false);
                            }
                        }
                    }

                    iemem_free(pThreadData, iemem_clientState, pChain->pEntries);
                }
            }

            iemem_free(pThreadData, iemem_clientState, pTable->pChains);
        }

        iemem_freeStruct(pThreadData, iemem_clientState, pTable, pTable->StrucId);
    }
}


/*
 * Allocates a new, larger client-state table moving the entries across
 */
int32_t iecs_resizeClientStateTable(ieutThreadData_t *pThreadData,
                                    iecsHashTable_t *pOldTable,
                                    iecsHashTable_t **ppNewTable)
{
    iecsHashTable_t *pNewTable;
    int32_t rc = OK;

    // First, allocate an empty table of the new size
    pNewTable = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_clientState, 3), sizeof(iecsHashTable_t));
    if (pNewTable != NULL)
    {
        memcpy(pNewTable->StrucId, iecsHASH_TABLE_STRUCID, 4);
        pNewTable->Generation = pOldTable->Generation + 1;
        pNewTable->TotalEntryCount = pOldTable->TotalEntryCount;
        pNewTable->ChainCount = pOldTable->ChainCount << iecsHASH_TABLE_SHIFT_FACTOR;
        pNewTable->ChainMask = pNewTable->ChainCount - 1;
        pNewTable->ChainCountMax = pOldTable->ChainCountMax;
        pNewTable->fCanResize = (pNewTable->ChainCount < pNewTable->ChainCountMax) ? true : false;
        pNewTable->pChains = NULL;

        ieutTRACEL(pThreadData, pNewTable->ChainCount, ENGINE_HIGH_TRACE,
                   "Resizing client-state table size to %u.\n", pNewTable->ChainCount);

        pNewTable->pChains = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_clientState, 4), pNewTable->ChainCount, sizeof(iecsHashChain_t));
        if (pNewTable->pChains == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }


    // Then iterate over the old table adding the pointers to the new table, but
    // do not make the pointers from the client-states back to the table entries
    if (rc == OK)
    {
        iecsHashChain_t *pOldChain = pOldTable->pChains;
        uint32_t i;
        for (i = 0; (i < pOldTable->ChainCount) && (rc == OK); i++, pOldChain++)
        {
            iecsHashEntry_t *pOldEntry = pOldChain->pEntries;
            if (pOldEntry != NULL)
            {
                uint32_t j;
                for (j = 0; (j < pOldChain->Limit) && (rc == OK); j++, pOldEntry++)
                {
                    if (pOldEntry->pValue != NULL)
                    {
                        uint32_t newChain = pOldEntry->Hash & pNewTable->ChainMask;
                        iecsHashChain_t *pNewChain = pNewTable->pChains + newChain;
                        if (pNewChain->Count == pNewChain->Limit)
                        {
                            // Resize the existing chain (which may be zero length currently)
                            iecsHashEntry_t *pNewEntries;
                            pNewEntries = iemem_calloc(pThreadData,
                                                       IEMEM_PROBE(iemem_clientState, 5),
                                                       pNewChain->Limit + iecsHASH_TABLE_CHAIN_INCREMENT,
                                                       sizeof(iecsHashEntry_t));
                            if (pNewEntries != NULL)
                            {
                                if (pNewChain->pEntries != NULL)
                                {
                                    memcpy(pNewEntries, pNewChain->pEntries, sizeof(iecsHashEntry_t) * pNewChain->Limit);
                                    iemem_free(pThreadData, iemem_clientState, pNewChain->pEntries);
                                }
                                pNewChain->Limit += iecsHASH_TABLE_CHAIN_INCREMENT;
                                pNewChain->pEntries = pNewEntries;
                            }
                            else
                            {
                                // The new entry has not been added, but the table is intact
                                rc = ISMRC_AllocateError;
                                ism_common_setError(rc);
                            }
                        }

                        if (rc == OK)
                        {
                            iecsHashEntry_t *pNewEntry = pNewChain->pEntries + pNewChain->Count;
                            pNewEntry->pValue = pOldEntry->pValue;
                            pNewEntry->Hash = pOldEntry->Hash;
                            pNewChain->Count++;
                        }
                    }
                }
            }
        }
    }


    // Finally, iterate over the new table making the pointers from the client-states
    // back to the table entries
    if (rc == OK)
    {
        iecsHashChain_t *pNewChain = pNewTable->pChains;
        uint32_t i;
        for (i = 0; i < pNewTable->ChainCount; i++, pNewChain++)
        {
            iecsHashEntry_t *pNewEntry = pNewChain->pEntries;
            if (pNewEntry != NULL)
            {
                uint32_t j;
                for (j = 0; j < pNewChain->Count; j++, pNewEntry++)
                {
                    pNewEntry->pValue->pHashEntry = pNewEntry;
                }
            }
        }
    }


    // And here's the shiny new table, darlings
    if (rc == OK)
    {
        *ppNewTable = pNewTable;
    }
    else
    {
        if (pNewTable != NULL)
        {
            iecs_freeClientStateTable(pThreadData, pNewTable, false);
        }
    }

    return rc;
}


void iecs_setClientMsgRateLimits( ieutThreadData_t *pThreadData
                                , ismEngine_ClientState_t *pClient
                                , ismSecurity_t *pSecContext)
{
    if (pSecContext != NULL)
    {
        pClient->expectedMsgRate =  ism_security_context_getExpectedMsgRate(pSecContext);
        //Convert the rate into a limit on inflight messages
        uint32_t limitFromRate;

        switch (pClient->expectedMsgRate)
        {
            case EXPECTEDMSGRATE_LOW:
                limitFromRate = 10;
                break;

            case EXPECTEDMSGRATE_HIGH:
                limitFromRate = 2048;
                break;

            case EXPECTEDMSGRATE_MAX:
                limitFromRate = MQTT_MAX_DELIVERYID - 100; //not exactly max so its easier to find a free slot when assigning ids
                break;

            case EXPECTEDMSGRATE_UNDEFINED:
            case EXPECTEDMSGRATE_DEFAULT:
                limitFromRate = ismEngine_serverGlobal.mqttMsgIdRange;
                break;

            default:
                ieutTRACE_FFDC(ieutPROBE_001, false,
                        "Illegal expectedMsgRate", pClient->expectedMsgRate, NULL);
                limitFromRate = ismEngine_serverGlobal.mqttMsgIdRange;
                break;

        }

        if (limitFromRate == 0)
        {
            //That doesn't sound right - override it
            ieutTRACEL(pThreadData, pClient->maxInflightLimit, ENGINE_ERROR_TRACE,
                                "Overriding msg limit from policy, was 0\n");
            limitFromRate = 128;
        }

        uint32_t explicitLimit = ism_security_context_getInflightMsgLimit(pSecContext);

        if (explicitLimit == 0)
        {
            //This is the number we get if it's not set - maybe we should make it all FFs
            explicitLimit = limitFromRate;
        }

        if (explicitLimit < limitFromRate)
        {
            pClient->maxInflightLimit = explicitLimit;
        }
        else
        {
            pClient->maxInflightLimit = limitFromRate;
        }
    }
    else
    {
        pClient->expectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
        pClient->maxInflightLimit = ismEngine_serverGlobal.mqttMsgIdRange;

        if (pClient->maxInflightLimit == 0)
        {
            pClient->maxInflightLimit = 128;
        }
    }
    ieutTRACEL(pThreadData, pClient->maxInflightLimit, ENGINE_FNC_TRACE,
                    FUNCTION_IDENT "maxmsgs %u, exprate %d\n", __func__,
                    pClient->maxInflightLimit, pClient->expectedMsgRate);
}

/*
 * Allocate a new client-state object
 */
int32_t iecs_newClientState(ieutThreadData_t *pThreadData,
                            iecsNewClientStateInfo_t *pInfo,
                            ismEngine_ClientState_t **ppClient)
{
    ismEngine_ClientState_t *pClient;
    int osrc;
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = pInfo->resourceSet;

    iere_primeThreadCache(pThreadData, resourceSet);
    pClient = iere_calloc(pThreadData,
                          resourceSet,
                          IEMEM_PROBE(iemem_clientState, 6), 1,
                          sizeof(ismEngine_ClientState_t) + strlen(pInfo->pClientId) + 1);
    if (pClient != NULL)
    {
        // Get the UserID from the security context (NULL if no context supplied)
        const char *pUserId = ism_security_context_getUserID(pInfo->pSecContext);

        // Make a copy of the user id that will persist with the client state
        if (pUserId != NULL)
        {
            size_t userIdLen = strlen(pUserId) + 1;

            pClient->pUserId = iere_malloc(pThreadData, resourceSet, IEMEM_PROBE(iemem_clientState, 7), userIdLen);

            if (pClient->pUserId == NULL)
            {
                iere_freeStruct(pThreadData, resourceSet, iemem_clientState, pClient, pClient->StrucId);
                pClient = NULL;
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            memcpy(pClient->pUserId, pUserId, userIdLen);
        }

        memcpy(pClient->StrucId, ismENGINE_CLIENT_STATE_STRUCID, 4);
        pClient->UseCount = 1;
        pClient->OpState = iecsOpStateActive;
        pClient->Durability = pInfo->durability;
        pClient->fCleanStart = pInfo->fCleanStart;
        pClient->Takeover = iecsNoTakeover;
        pClient->durableObjects = 0;
        pClient->resourceSet = resourceSet;
        assert(pClient->fSuspendExpiry == false);
        osrc = pthread_mutex_init(&pClient->Mutex, NULL);
        if (osrc == 0)
        {
            osrc = pthread_spin_init(&pClient->UseCountLock, false);
            if (osrc == 0)
            {
                osrc = pthread_mutex_init(&pClient->UnreleasedMutex, NULL);
                if (osrc != 0)
                {
                    pthread_spin_destroy(&pClient->UseCountLock);
                    pthread_mutex_destroy(&pClient->Mutex);
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                }
            }
            else
            {
                pthread_mutex_destroy(&pClient->Mutex);
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }

        if (LIKELY(rc == OK))
        {
            pClient->pClientId = (char *)(pClient + 1);
            strcpy(pClient->pClientId, pInfo->pClientId);
            pClient->protocolId = pInfo->protocolId;
            pClient->pSecContext = pInfo->pSecContext;
            pClient->pStealContext = pInfo->pStealContext;
            pClient->pStealCallbackFn = pInfo->pStealCallbackFn;
            iecs_setClientMsgRateLimits(pThreadData, pClient, pClient->pSecContext);
            // Only externally originating protocols should count...
            pClient->fCountExternally = (pInfo->protocolId == PROTOCOL_ID_MQTT)  ||
                                        (pInfo->protocolId == PROTOCOL_ID_JMS)   ||
                                        (pInfo->protocolId == PROTOCOL_ID_HTTP)  ||
                                        (pInfo->protocolId == PROTOCOL_ID_PLUGIN);

            // Return value to caller
            *ppClient = pClient;
        }
        else
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_free(pThreadData, resourceSet, iemem_clientState, pClient->pUserId);
            iere_freeStruct(pThreadData, resourceSet, iemem_clientState, pClient, pClient->StrucId);
            pClient = NULL;
        }
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

mod_exit:

    return rc;
}

/*
 * Allocate a new client-state object during recovery
 */
int32_t iecs_newClientStateRecovery(ieutThreadData_t *pThreadData,
                                    iecsNewClientStateInfo_t *pInfo,
                                    ismEngine_ClientState_t **ppClient)
{
    ismEngine_ClientState_t *pClient;
    size_t allocsize = sizeof(ismEngine_ClientState_t) + strlen(pInfo->pClientId) + 1;
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = pInfo->resourceSet;

    assert(pInfo->durability == iecsDurable || pInfo->durability == iecsNonDurable);

    iere_primeThreadCache(pThreadData, resourceSet);
    pClient = iere_calloc(pThreadData, resourceSet, IEMEM_PROBE(iemem_clientState, 8), 1, allocsize);
    if (pClient != NULL)
    {
        int osrc;
        memcpy(pClient->StrucId, ismENGINE_CLIENT_STATE_STRUCID, 4);
        pClient->UseCount = 1;
        pClient->OpState = iecsOpStateZombie;
        pClient->Durability = pInfo->durability;
        pClient->fCleanStart = false;
        pClient->Takeover = iecsNoTakeover;
        pClient->durableObjects = 0;
        // ExpiryInterval and Time will be updated when CPR is read in
        pClient->ExpiryInterval = iecsEXPIRY_INTERVAL_INFINITE;
        pClient->ExpiryTime = 0;
        pClient->resourceSet = resourceSet;
        assert(pClient->fSuspendExpiry == false);
        osrc = pthread_mutex_init(&pClient->Mutex, NULL);
        if (osrc == 0)
        {
            osrc = pthread_spin_init(&pClient->UseCountLock, false);
            if (osrc == 0)
            {
                osrc = pthread_mutex_init(&pClient->UnreleasedMutex, NULL);
                if (osrc != 0)
                {
                    pthread_spin_destroy(&pClient->UseCountLock);
                    pthread_mutex_destroy(&pClient->Mutex);
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                }
            }
            else
            {
                pthread_mutex_destroy(&pClient->Mutex);
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }

        if (LIKELY(rc == OK))
        {
            pClient->pClientId = (char *)(pClient + 1);
            strcpy(pClient->pClientId, pInfo->pClientId);
            pClient->protocolId = pInfo->protocolId;
            iecs_setClientMsgRateLimits(pThreadData, pClient, NULL);
            // This is filled in from the info in the CPR
            assert(pClient->pUserId == NULL);
            // Only externally originating protocols should count...
            pClient->fCountExternally = (pInfo->protocolId == PROTOCOL_ID_MQTT)  ||
                                        (pInfo->protocolId == PROTOCOL_ID_JMS)   ||
                                        (pInfo->protocolId == PROTOCOL_ID_HTTP)  ||
                                        (pInfo->protocolId == PROTOCOL_ID_PLUGIN);

            // Return value to caller
            *ppClient = pClient;
        }
        else
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_freeStruct(pThreadData, resourceSet, iemem_clientState, pClient, pClient->StrucId);
            pClient = NULL;
        }
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    return rc;
}

/*
 * Allocate a new client-state object as part of an import
 */
int32_t iecs_newClientStateImport(ieutThreadData_t *pThreadData,
                                  iecsNewClientStateInfo_t *pInfo,
                                  ismEngine_ClientState_t **ppClient)
{
    ismEngine_ClientState_t *pClient;
    size_t allocsize;
    int osrc;
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = pInfo->resourceSet;

    allocsize = sizeof(ismEngine_ClientState_t) + strlen(pInfo->pClientId) + 1;

    iere_primeThreadCache(pThreadData, resourceSet);
    pClient = iere_calloc(pThreadData, resourceSet, IEMEM_PROBE(iemem_clientState, 21), 1, allocsize);
    if (pClient != NULL)
    {
        memcpy(pClient->StrucId, ismENGINE_CLIENT_STATE_STRUCID, 4);
        pClient->UseCount = 2; // +1 for "importing"
        pClient->OpState = iecsOpStateZombie;
        pClient->Durability = pInfo->durability;
        pClient->protocolId = pInfo->protocolId;
        pClient->fCleanStart = false;
        pClient->Takeover = iecsNoTakeover;
        pClient->durableObjects = 0;
        pClient->ExpiryInterval = pInfo->expiryInterval;
        pClient->LastConnectedTime = pInfo->lastConnectedTime;
        pClient->resourceSet = resourceSet;
        pClient->fSuspendExpiry = true; // We only want expiry to begin once this has finished importing
        // Make a copy of the user id
        if (pInfo->pUserId != NULL)
        {
            allocsize = strlen(pInfo->pUserId) + 1;

            iere_primeThreadCache(pThreadData, resourceSet);
            pClient->pUserId = iere_malloc(pThreadData, resourceSet, IEMEM_PROBE(iemem_clientState, 22), allocsize);

            if (pClient->pUserId == NULL)
            {
                iere_freeStruct(pThreadData, resourceSet, iemem_clientState, pClient, pClient->StrucId);
                pClient = NULL;
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            memcpy(pClient->pUserId, pInfo->pUserId, allocsize);
        }

        osrc = pthread_mutex_init(&pClient->Mutex, NULL);
        if (osrc == 0)
        {
            osrc = pthread_spin_init(&pClient->UseCountLock, false);
            if (osrc == 0)
            {
                osrc = pthread_mutex_init(&pClient->UnreleasedMutex, NULL);
                if (osrc != 0)
                {
                    pthread_spin_destroy(&pClient->UseCountLock);
                    pthread_mutex_destroy(&pClient->Mutex);
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                }
            }
            else
            {
                pthread_mutex_destroy(&pClient->Mutex);
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }

        if (LIKELY(rc == OK))
        {
            pClient->pClientId = (char *)(pClient + 1);
            strcpy(pClient->pClientId, pInfo->pClientId);
            iecs_setClientMsgRateLimits(pThreadData, pClient, NULL);
            // Only externally originating protocols should count...
            pClient->fCountExternally = (pInfo->protocolId == PROTOCOL_ID_MQTT)  ||
                                        (pInfo->protocolId == PROTOCOL_ID_JMS)   ||
                                        (pInfo->protocolId == PROTOCOL_ID_HTTP)  ||
                                        (pInfo->protocolId == PROTOCOL_ID_PLUGIN);

            // Return value to caller
            *ppClient = pClient;
        }
        else
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_free(pThreadData, resourceSet, iemem_clientState, pClient->pUserId);
            iere_freeStruct(pThreadData, resourceSet, iemem_clientState, pClient, pClient->StrucId);
            pClient = NULL;
        }
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

mod_exit:

    return rc;
}


// Acquires a reference to a client-state to prevent its deallocation.
int32_t iecs_acquireClientStateReference(ismEngine_ClientState_t *pClient)
{
    int32_t rc = OK;

    assert(pClient != NULL);
    ismEngine_CheckStructId(pClient->StrucId, ismENGINE_CLIENT_STATE_STRUCID, ieutPROBE_004);

    pthread_spin_lock(&pClient->UseCountLock);
    pClient->UseCount += 1;
    pthread_spin_unlock(&pClient->UseCountLock);

    return rc;
}


static inline void iecs_getMDRChunkSizeAndCount( uint32_t inflightLimit
                                               , uint32_t *pMdrChunkSize
                                               , uint32_t *pMdrChunkCount)
{
    // We want to set the MDR chunk size to match the max in-flight msgs, so
    // that each allocated chunk contains only the MDRs we need, however,
    // with very low in-flight windows, this causes the chunk count to be very
    // high, which means each client state will allocate a lot of memory for
    // the chunk array, which it potentially will never use.
    //
    // As a result, we set a minimum chunk size which is an attempt to
    // balance the memory allocated up front with that not needed in a chunk.
    if (inflightLimit < iecsMINIMUM_MDR_CHUNK_SIZE)
    {
        *pMdrChunkSize = iecsMINIMUM_MDR_CHUNK_SIZE;
    }
    else
    {
        *pMdrChunkSize = inflightLimit;
    }
    *pMdrChunkCount = (65535/(*pMdrChunkSize))+1;
}

static inline void iecs_setupInflightLimitBasedParams( ieutThreadData_t *pThreadData
                                                     , iecsMessageDeliveryInfo_t *pMsgDelInfo
                                                     , uint32_t inflightLimit)
{
    pMsgDelInfo->MaxInflightMsgs = inflightLimit;
    pMsgDelInfo->InflightReenable = inflightLimit * 0.70; //Turn the taps back on when we're down to 70%

    //We can't change the chunk size/count once there are actually chunks in use
    //(we'd need to resize existing chunks and move the mdrs in them around)
    //we don't currently change the size once the pMsgDelInfo has been allocated as we'd have to realloc that as the chunk
    //array is part of it

    if (pMsgDelInfo->MdrChunkCount == 0 || pMsgDelInfo->MdrChunkSize == 0)
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "Partially setup pMsgDelInfo", pMsgDelInfo->MdrChunkSize, NULL);
    }

    ieutTRACEL(pThreadData, inflightLimit, ENGINE_PERFDIAG_TRACE,
                   "MDRChunkSize %u MDRChunkCount %u maxInflightMsgs %u\n",
                   pMsgDelInfo->MdrChunkSize, pMsgDelInfo->MdrChunkCount, inflightLimit);
}



// Release a reference to a client-state, deallocating the client-state when
// the use-count drops to zero.
// If fInline is true, the reference is being released by the operation
// to destroy the client-state and is thus "inline". When it's done inline,
// the destroy completion callback does not need to be called because the caller
// is synchronously completing the destroy.
// If fInlineThief is true, the reference is being released by the operation
// to create the thief client-state (which is stealing the victim's client ID).
// When the reference to the victim is released inline during the creation
// of the thief client-state, there's no need to call the thief's create completion
// callback because the caller is synchronously completing the creation.
bool iecs_releaseClientStateReference(ieutThreadData_t *pThreadData,
                                      ismEngine_ClientState_t *pClient,
                                      bool fInline,
                                      bool fInlineThief)
{
    bool fDidRelease = false;
    iecsCleanupOptions_t doCleanup = iecsCleanup_None;
    bool doZombieCleanup = false;
    bool doLateDecrement = false;

    assert(!(fInline && fInlineThief));

    pthread_spin_lock(&pClient->UseCountLock);
    ismEngine_ClientState_t *pThief = pClient->pThief;

    // We are being asked to reduce the UseCount from 1 to zero (the test here is on the UseCount BEFORE decrement)
    if (pClient->UseCount == 1)
    {
        // NOTE: This previously was iecsOpStateNonDurableCleanup || iecsOpStateZombieRemoval,
        //       but analysis shows we don't come here for iecsOpStateZombieRemoval.
        if (pClient->OpState == iecsOpStateNonDurableCleanup)
        {
            if ( pClient->hStoreCSR != ismSTORE_NULL_HANDLE &&    // a
                (pThief == NULL || pThief->fCleanStart ||         // b
                 pClient->Durability != pThief->Durability) &&
                 pClient->fLeaveResourcesAtRestart == false)      // c
            {
                pClient->fDiscardDurable = true;
                pClient->OpState = iecsOpStateZombieCleanup;
                doZombieCleanup = true;
            }
            else
            {
                doLateDecrement = true;
            }
        }
        else
        {
            // This could be the release from an iecs_findClientState which could be a
            // client in any state. In this case (so anything else) we just need to do
            // the decrement.
            doLateDecrement = true;
        }
    }
    else
    {
        assert(pClient->UseCount != 0);

        if (--(pClient->UseCount) == 1)
        {
            if (pClient->OpState == iecsOpStateDisconnecting)
            {
                pClient->OpState = iecsOpStateNonDurableCleanup;
                // Can no longer refer to the security context for this client, it will
                // be freed at some point during the process of cleaning up.
                pClient->pSecContext = NULL;
                doCleanup = iecsCleanup_Subs;
                if (pClient->WillDelayExpiryTime == 0)
                {
                    doCleanup |= iecsCleanup_PublishWillMsg;
                }
            }
            else if (pClient->OpState == iecsOpStateZombieRemoval ||
                     pClient->OpState == iecsOpStateZombieExpiry)
            {
                // We need to clean up the durable state if:
                //
                // (a) This clientState has a CSR in the store AND
                // (b) There is no thief OR the thief requested CleanStart OR
                //     this client and the thief have different durability AND
                // (c) The fLeaveResourcesAtRestart flag is not set (meaning that this is happening
                //     at restart and another instance of this clientState has picked up the resources)
                //
                if ( pClient->hStoreCSR != ismSTORE_NULL_HANDLE &&  // a
                    (pThief == NULL || pThief->fCleanStart ||       // b
                     pClient->Durability != pThief->Durability) &&
                     pClient->fLeaveResourcesAtRestart == false)    // c
                {
                    pClient->fDiscardDurable = true;
                    pClient->OpState = iecsOpStateZombieCleanup;
                    doZombieCleanup = true;
                }
                else
                {
                    doLateDecrement = true;
                }
            }
        }
    }
    pthread_spin_unlock(&pClient->UseCountLock);

    // We are moving into the CleaningUp state
    if (doCleanup != iecsCleanup_None)
    {
        assert(doLateDecrement == false);
        fDidRelease = iecs_cleanupRemainingResources(pThreadData,
                                                     pClient,
                                                     doCleanup,
                                                     fInline, fInlineThief);
    }
    else if (doZombieCleanup)
    {
        assert(doLateDecrement == false);
        assert(pClient->hStoreCSR != ismSTORE_NULL_HANDLE);

        ieutTRACEL(pThreadData, pClient->hStoreCSR, ENGINE_HIGH_TRACE,
                   "Marking pClient %p (store handle 0x%lx) deleted (pThief=%p)\n",
                   pClient, pClient->hStoreCSR, pThief);

        // We're removing the durable resources associated with this client state,
        // in case the server ends before we finish deleting subscriptions, mark the
        // clientState as DELETED so that restart can complete the job.
        int32_t rc = ism_store_updateRecord(pThreadData->hStream,
                                            pClient->hStoreCSR,
                                            ismSTORE_NULL_HANDLE,
                                            iestCSR_STATE_DELETED,
                                            ismSTORE_SET_STATE);
        if (rc == OK)
        {
            iest_store_commit(pThreadData, false);
        }
        else
        {
            ieutTRACE_FFDC(ieutPROBE_007, true,
                           "update record failed", rc,
                           "rc", &rc, sizeof(rc),
                           "CSR", &(pClient->hStoreCSR),  sizeof(pClient->hStoreCSR),
                           NULL);
        }

        fDidRelease = iecs_cleanupRemainingResources(pThreadData,
                                                     pClient,
                                                     iecsCleanup_Subs | iecsCleanup_Include_DurableSubs,
                                                     fInline, fInlineThief);
    }
    else if (doLateDecrement)
    {
        bool removeFromTable = false;
        bool updateThiefInStore = false;
        bool completeRelease = false;

        iecsOpState_t prevState;

        ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);

        // Do the late decrement, which might result in needing to remove & release
        pthread_spin_lock(&pClient->UseCountLock);
        assert(pClient->UseCount >= 1);
        prevState = pClient->OpState;
        assert(prevState != iecsOpStateFreeing);
        if (--(pClient->UseCount) == 0)
        {
            pThief = pClient->pThief;
            pClient->OpState = iecsOpStateFreeing;
            removeFromTable = pClient->fTableRemovalRequired;
            pClient->fTableRemovalRequired = false;
            completeRelease = true;
        }
        pthread_spin_unlock(&pClient->UseCountLock);

        ieutTRACEL(pThreadData, pClient, ENGINE_FNC_TRACE,
                   "Releasing reference to client-state %p, removeFromTable=%d, completeRelease=%d, fInline = %d\n",
                   pClient, removeFromTable, completeRelease, fInline);

        // We are responsible for removing the client state from the table
        if (removeFromTable)
        {
            // Remove from the table
            iecsHashEntry_t *pEntry = pClient->pHashEntry;

            if (pEntry != NULL)
            {
                uint32_t hash = pEntry->Hash;

                pClient->pHashEntry = NULL;
                pEntry->pValue = NULL;
                pEntry->Hash = 0;

                iecsHashTable_t *pTable = ismEngine_serverGlobal.ClientTable;
                iecsHashChain_t *pChain = pTable->pChains + (hash & pTable->ChainMask);
                pChain->Count--;
                pTable->TotalEntryCount--;

                if (pClient->fCountExternally)
                {
                    ismEngine_serverGlobal.totalClientStatesCount--;

                    if (pClient->Durability == iecsDurable)
                    {
                        iereResourceSetHandle_t resourceSet = pClient->resourceSet;
                        iere_primeThreadCache(pThreadData, resourceSet);
                        iere_updateInt64Stat(pThreadData, resourceSet,
                                             ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_CLIENT_STATES, -1);
                    }
                }
            }

            // We might need to hand our state over to a thief...
            if (prevState != iecsOpStateZombieCleanup && pThief != NULL)
            {
                bool durabilityChanged = (pClient->Durability != pThief->Durability);

                if (!durabilityChanged)
                {
                    assert(pThief->fCleanStart == false || pClient->hStoreCSR == ismSTORE_NULL_HANDLE);

                    pThief->hStoreCSR = pClient->hStoreCSR;
                    pThief->hStoreCPR = pClient->hStoreCPR;
                    pThief->pUnreleasedHead = pClient->pUnreleasedHead;
                    pThief->hUnreleasedStateContext = pClient->hUnreleasedStateContext;
                    pThief->hMsgDeliveryInfo = pClient->hMsgDeliveryInfo;
                    if (pThief->hMsgDeliveryInfo != NULL)
                    {
                        assert(pThief->hMsgDeliveryInfo->resourceSet == pThief->resourceSet);
                        pThief->hMsgDeliveryInfo->diagnosticOwner = pThief;
                        iecs_setupInflightLimitBasedParams( pThreadData
                                                          , pThief->hMsgDeliveryInfo
                                                          , pThief->maxInflightLimit);
                    }
                    pThief->inflightDestinationHead = pClient->inflightDestinationHead;
                    pThief->durableObjects = pClient->durableObjects;

                    assert(pThief->LastConnectedTime == 0);

                    pClient->hStoreCSR = ismSTORE_NULL_HANDLE;
                    pClient->hStoreCPR = ismSTORE_NULL_HANDLE;
                    pClient->pUnreleasedHead = NULL;
                    pClient->hUnreleasedStateContext = 0;
                    pClient->hMsgDeliveryInfo = NULL;
                    pClient->inflightDestinationHead = NULL;
                    pClient->durableObjects = 0;

                    // If the thief has a client state record (CSR) in the store and the
                    // a) ExpiryInterval has changed, b) the client has a will message
                    // still attached, or c) UserID has changed, we need to create a new CPR
                    // and update the CSR to point to it.
                    if ((pThief->hStoreCSR != ismSTORE_NULL_HANDLE) &&
                        ((pThief->ExpiryInterval != pClient->ExpiryInterval) ||    // a)
                         (pClient->hWillMessage != NULL) ||                        // b
                         ((pThief->pUserId == NULL && pClient->pUserId != NULL) || // c)
                          (pThief->pUserId != NULL && pClient->pUserId == NULL) ||
                          (pThief->pUserId != NULL && pClient->pUserId != NULL
                           && strcmp(pThief->pUserId, pClient->pUserId) != 0))))
                    {
                        assert(pThief->hWillMessage == NULL); // Thief shouldn't have a will message at this stage!
                        updateThiefInStore = true;
                    }
                }
            }
        }

        ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

        // The thief is different from the victim, so we need to update the store.
        if (updateThiefInStore)
        {
            assert(pThief != NULL);
            (void)iecs_updateClientPropsRecord(pThreadData, pThief, NULL, ismSTORE_NULL_HANDLE, 0, 0);
        }

        // We are responsible for completing the release of the ClientState
        if (completeRelease)
        {
            fDidRelease = true;
            iecs_completeReleaseClientState(pThreadData, pClient, fInline, fInlineThief);
        }
    }

    return fDidRelease;
}


void iecs_completeReleaseClientState(ieutThreadData_t *pThreadData,
                                     ismEngine_ClientState_t *pClient,
                                     bool fInline,
                                     bool fInlineThief)
{
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    assert(pClient->OpState == iecsOpStateFreeing);
    assert(pClient->UseCount == 0);

    ieutTRACEL(pThreadData, pClient, ENGINE_HIGH_TRACE, FUNCTION_IDENT
               "clientState %p, fInline = %d, CSR=0x%016lx\n", __func__, pClient, fInline, pClient->hStoreCSR);

    // If the client still has a store handle, we need to delete it now
    if (pClient->hStoreCSR != ismSTORE_NULL_HANDLE)
    {
        int32_t rc = OK;

        if (pClient->hMsgDeliveryInfo != NULL)
        {
            iecs_cleanupMessageDeliveryInfo(pThreadData, pClient->hMsgDeliveryInfo);
        }

        if (pClient->hUnreleasedStateContext != ismSTORE_NULL_HANDLE)
        {
            (void)ism_store_closeStateContext(pClient->hUnreleasedStateContext);
            pClient->hUnreleasedStateContext = ismSTORE_NULL_HANDLE;
        }

        rc = ism_store_deleteRecord(pThreadData->hStream,
                                    pClient->hStoreCSR);
        if (rc == OK)
        {
            iest_store_commit(pThreadData, false);
        }
        else
        {
            assert(rc != ISMRC_StoreGenerationFull);
            iest_store_rollback(pThreadData, false);
        }

        pClient->hStoreCSR = ismSTORE_NULL_HANDLE;
    }

    ismEngine_CompletionCallback_t pPendingDestroyCallbackFn = pClient->pPendingDestroyCallbackFn;
    void *pPendingDestroyContext = pClient->pPendingDestroyContext;

    // If this client-state was stolen by a new client-state, remember the thief
    ismEngine_ClientState_t *pThief = pClient->pThief;

    // Deallocate the client-state itself unless it's a zombie, durable
    // client-state and we're using it as a way of holding on to the durable state
    pClient->pPendingDestroyCallbackFn = NULL;
    pClient->pPendingDestroyContext = NULL;

    // If we're not running in-line, we're completing this asynchronously and
    // may have been given a completion callback for the destroy operation
    if (pPendingDestroyCallbackFn != NULL)
    {
        ieutTRACEL(pThreadData, pPendingDestroyCallbackFn, ENGINE_HIGH_TRACE,
                   FUNCTION_IDENT "pPendingDestroyCallbackFn=%p calling=%d\n",
                   __func__, pPendingDestroyCallbackFn, (int)(!fInline));

        if (!fInline)
        {
            pPendingDestroyCallbackFn(OK, NULL, pPendingDestroyContext);
        }
    }

    if (pPendingDestroyContext != NULL)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_free(pThreadData, resourceSet, iemem_callbackContext, pPendingDestroyContext);
    }

    // We are now in a position to complete the theft. The stolen client-state
    // will be freed, so the thief's creation can be completed. The fInlineThief flag
    // is true if we've entered this function in-line in the creation of the thief
    // which means that we are synchronously completing the operation and we don't
    // want to call the creation callback.
    if (pThief != NULL)
    {
        // It looks like our thief is durable, but didn't create a store record because
        // it wanted a clean start or we were in the process of being deleted... so
        // it is up to us to create the store records for this (new) clientState.
        if (pThief->Durability == iecsDurable && pThief->hStoreCSR == ismSTORE_NULL_HANDLE)
        {
            int32_t rc = iecs_storeClientState(pThreadData,
                                               pThief,
                                               false,
                                               NULL); // Make it go non-async for now

            if (rc != OK)
            {
                ieutTRACE_FFDC(ieutPROBE_001, true,
                               "Couldn't write the thief durable client state", rc,
                               "pThief", pThief, sizeof(*pThief),
                               "pClient", pClient, sizeof(*pClient),
                               NULL);
            }

            assert(pThief->OpState == iecsOpStateActive);
            assert(pThief->hStoreCSR != ismSTORE_NULL_HANDLE);
            assert(pThief->hStoreCPR != ismSTORE_NULL_HANDLE);

            // This shouldn't happen often, it means that this particular client state
            // took over while we were in the process of destroying a client state.
            // It is only being traced as an 'UNUSUAL' event because it's useful to see
            // that it does happen sometimes.
            ieutTRACEL(pThreadData, pThief, ENGINE_UNUSUAL_TRACE,
                       "Completing client-ID '%s' theft of clientState %p by clientState %p, new CSR=0x%016lx\n",
                       pThief->pClientId, pClient, pThief, pThief->hStoreCSR);
        }
        else
        {
            ieutTRACEL(pThreadData, pThief, ENGINE_HIGH_TRACE,
                       "Completing client-ID '%s' theft of clientState %p by clientState %p, inheriting CSR=0x%016lx\n",
                       pThief->pClientId, pClient, pThief, pThief->hStoreCSR);
        }

        ismEngine_CompletionCallback_t pPendingCreateCallbackFn;
        void *pPendingCreateContext;
        ismEngine_StealCallback_t pStealCallbackFn = NULL;
        void *pStealContext = NULL;

        // Revalidate any subscriptions owned by the stolen client-state using the thief
        iecs_revalidateSubscriptions(pThreadData, pThief);

        pthread_spin_lock(&pThief->UseCountLock);
        pPendingCreateCallbackFn = pThief->pPendingCreateCallbackFn;
        pPendingCreateContext = pThief->pPendingCreateContext;

        if (pPendingCreateCallbackFn != NULL)
        {
            if (pThief->pThief != NULL)
            {
                pStealCallbackFn = pThief->pStealCallbackFn;
                pThief->pStealCallbackFn = NULL;
                pStealContext = pThief->pStealContext;
                pThief->pStealContext = NULL;
            }

            if (fInlineThief)
            {
                pPendingCreateCallbackFn = NULL;
            }

            pThief->pPendingCreateCallbackFn = NULL;
            pThief->pPendingCreateContext = NULL;
        }
        else
        {
            assert(pThief->pThief == NULL || pThief->pStealCallbackFn == NULL);
        }

        if (pThief->pThief != NULL)
        {
            assert(pThief->pStealCallbackFn == NULL);
            assert(pThief->pStealContext == NULL);
        }
        pthread_spin_unlock(&pThief->UseCountLock);

        if (pPendingCreateCallbackFn != NULL)
        {
            int32_t rc;

            if (pThief->Takeover == iecsNoTakeover)
            {
                rc = ISMRC_OK;
            }
            else
            {
                rc = ISMRC_ResumedClientState;
            }

            pPendingCreateCallbackFn(rc, pThief, pPendingCreateContext);
        }

        if (pPendingCreateContext != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_free(pThreadData, resourceSet, iemem_callbackContext, pPendingCreateContext);
        }

        if (pStealCallbackFn != NULL)
        {
            int32_t reason;

            assert(pThief->pThief != NULL);

            if (pThief->pThief->Takeover == iecsNonAckingClient)
            {
                reason = ISMRC_NonAckingClient;
            }
            else
            {
                reason = ISMRC_ResumedClientState;
            }

            pStealCallbackFn(reason,
                             pThief,
                             ismENGINE_STEAL_CALLBACK_OPTION_NONE,
                             pStealContext);
        }

        iecs_releaseClientStateReference(pThreadData, pThief, false, false);
    }

    // Actually free the client state
    iecs_freeClientState(pThreadData, pClient, true);
}

/*
 * Update the resourceSet related statistics for a will message
 */
void iecs_updateWillMsgStats(ieutThreadData_t *pThreadData,
                             iereResourceSetHandle_t resourceSet,
                             ismEngine_Message_t *pMessage,
                             int64_t multiplier)
{
    int64_t msgBytes = pMessage->fullMemSize * multiplier;

    if (pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
    {
        iere_updateInt64Stat(pThreadData, resourceSet,
                             ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_WILLMSG_BYTES, msgBytes);
    }
    else
    {
        iere_updateInt64Stat(pThreadData, resourceSet,
                             ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_WILLMSG_BYTES, msgBytes);
    }

    iere_updateInt64Stat(pThreadData, resourceSet,
                         ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_WILLMSGS, multiplier);

    iere_updateTotalMemStat(pThreadData, resourceSet, IEMEM_PROBE(iemem_messageBody, 6), pMessage, msgBytes);
}

/*
 * Deallocate a client-state object
 */
void iecs_freeClientState(ieutThreadData_t *pThreadData,
                          ismEngine_ClientState_t *pClient,
                          bool unstore)
{
    ismEngine_UnreleasedState_t *pUnrelChunk;
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    assert(memcmp(pClient->StrucId, ismENGINE_CLIENT_STATE_STRUCID, 4) == 0);
    assert(pClient->pHashEntry == NULL);

    // If there is still a will message attached, destroy it now.
    if (pClient->hWillMessage != NULL)
    {
        ismEngine_Message_t *pMessage = (ismEngine_Message_t *)pClient->hWillMessage;

        // We want to do the unstore if it was stored when it was set, so we need to check
        // the persistence of the message.
        if (unstore == true && pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
        {
            iest_unstoreMessageCommit(pThreadData, pMessage, 0);
        }

        // Update the per-resourceSet stats.
        iere_primeThreadCache(pThreadData, resourceSet);
        iecs_updateWillMsgStats(pThreadData, resourceSet, pMessage, -1);
        iem_releaseMessage(pThreadData, pMessage);
    }

    if (pClient->pWillTopicName != NULL)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_free(pThreadData, resourceSet, iemem_clientState, pClient->pWillTopicName);
        pClient->pWillTopicName = NULL;
    }

    //If we still have inflight messages, get rid of them
    iecs_forgetInflightMsgs(pThreadData, pClient);

    if (pClient->hUnreleasedStateContext != ismSTORE_NULL_HANDLE)
    {
        (void)ism_store_closeStateContext(pClient->hUnreleasedStateContext);
        pClient->hUnreleasedStateContext = ismSTORE_NULL_HANDLE;
    }

    iecs_lockUnreleasedDeliveryState(pClient);
    pUnrelChunk = pClient->pUnreleasedHead;
    while (pUnrelChunk != NULL)
    {
        ismEngine_UnreleasedState_t *pNext = pUnrelChunk->pNext;
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_freeStruct(pThreadData,
                        resourceSet,
                        iemem_clientState, pUnrelChunk, pUnrelChunk->StrucId);
        pUnrelChunk = pNext;
    }
    pClient->pUnreleasedHead = NULL;
    iecs_unlockUnreleasedDeliveryState(pClient);

    if (pClient->hMsgDeliveryInfo != NULL)
    {
        iecs_releaseMessageDeliveryInfoReference(pThreadData, pClient->hMsgDeliveryInfo);
        pClient->hMsgDeliveryInfo = NULL;
    }

    // Destroy the temporary queues associated with this client
    if (pClient->pTemporaryQueues != NULL)
    {
        ieqn_destroyQueueGroup(pThreadData, pClient->pTemporaryQueues, ieqnDiscardMessages);
        pClient->pTemporaryQueues = NULL;
    }

    // Free the userid if it is allocated
    if (pClient->pUserId != NULL)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_free(pThreadData, resourceSet, iemem_clientState, pClient->pUserId);
    }

    pthread_mutex_destroy(&pClient->UnreleasedMutex);
    pthread_spin_destroy(&pClient->UseCountLock);
    pthread_mutex_destroy(&pClient->Mutex);

    iere_primeThreadCache(pThreadData, resourceSet);
    iere_freeStruct(pThreadData, resourceSet, iemem_clientState, pClient, pClient->StrucId);
}


/*
 * Adds a client-state to the client-state table, potentially beginning the steal process
 */
int32_t iecs_addClientState(ieutThreadData_t *pThreadData,
                            ismEngine_ClientState_t *pClient,
                            uint32_t options,
                            uint32_t internalOptions,
                            void *pContext,
                            size_t contextLength,
                            ismEngine_CompletionCallback_t pCallbackFn)
{
    int32_t rc = OK;
    iecsHashTable_t *pTable = NULL;
    char *pClientId = pClient->pClientId;
    ismEngineComponentStatus_t storeStatus;
    bool fStoreNeeded = false;
    bool fUpdateLastConnectedTime = false;
    ismEngine_ClientState_t *pVictim = NULL;
    ismEngine_StealCallback_t pStealCallbackFn = NULL;
    void *pStealContext = NULL;
    bool fFromImport = (options & ismENGINE_CREATE_CLIENT_INTERNAL_OPTION_IMPORTING) ? true : false;
    uint32_t hash;
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    ieutTRACEL(pThreadData, pClient, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "(pClient %p, options %u)\n", __func__,
               pClient, options);

    assert(pClientId != NULL);

    bool fSteal = (options & ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL) ? true : false;

    hash = calculateHash(pClientId);

    // If the new clientState is durable, we may need to write to the store, decide now
    // whether we are going to allow that if needed
    if (pClient->Durability == iecsDurable || pClient->Durability == iecsInheritOrDurable)
    {
        storeStatus = ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0];
    }
    else
    {
        storeStatus = StatusOk;
    }

    ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);

    iecsHashChain_t *pChain=NULL; // Initialise the point to NULL

    if (!fFromImport && ieie_isClientIdBeingImported(pThreadData, pClientId, hash))
    {
        rc = ISMRC_ClientIDInUse;
        ism_common_setErrorData(rc, "%s", pClientId);
    }
    else
    {
        pTable = ismEngine_serverGlobal.ClientTable;

        // First see if the table has reached its loading limit and needs resizing
        if (pTable->fCanResize &&
            (pTable->TotalEntryCount >= pTable->ChainCount * iecsHASH_TABLE_LOADING_LIMIT))
        {
            iecsHashTable_t *pNewTable = NULL;

            rc = iecs_resizeClientStateTable(pThreadData, pTable, &pNewTable);
            if (rc == OK)
            {
                iecs_freeClientStateTable(pThreadData, pTable, false);
                ismEngine_serverGlobal.ClientTable = pNewTable;
                pTable = pNewTable;
            }
            else if (rc == ISMRC_AllocateError)
            {
                // OK, so we couldn't resize the table, but we may still be able to
                // insert this entry into the existing table. The efficiency of a
                // larger table is not so much greater that we can't cope.
                pTable->fCanResize = false;
                rc = OK;
            }
        }

        // See if the chosen chain is already full - do it eagerly in the anticipation that
        // clashes are rare and we want the logic simple
        if (rc == OK)
        {
            uint32_t chain = hash & pTable->ChainMask;

            pChain = pTable->pChains + chain;

            if (pChain->Count == pChain->Limit)
            {
                iecsHashEntry_t *pNewEntries = iemem_calloc(pThreadData,
                                                            IEMEM_PROBE(iemem_clientState, 9),
                                                            pChain->Limit + iecsHASH_TABLE_CHAIN_INCREMENT,
                                                            sizeof(iecsHashEntry_t));
                if (pNewEntries != NULL)
                {
                    // Copy the entries from the old bucket to the new one
                    if (pChain->pEntries != NULL)
                    {
                        iecsHashEntry_t *pEntry = pChain->pEntries;
                        iecsHashEntry_t *pNewEntry = pNewEntries;
                        uint32_t remaining = pChain->Count;
                        while (remaining > 0)
                        {
                            if (pEntry->pValue != NULL)
                            {
                                pNewEntry->pValue = pEntry->pValue;
                                pNewEntry->Hash = pEntry->Hash;
                                pNewEntry->pValue->pHashEntry = pNewEntry;
                                pNewEntry++;

                                remaining--;
                            }

                            pEntry++;
                        }

                        iemem_free(pThreadData, iemem_clientState, pChain->pEntries);
                    }

                    pChain->Limit += iecsHASH_TABLE_CHAIN_INCREMENT;
                    pChain->pEntries = pNewEntries;
                }
                else
                {
                    // The new entry has not been added, but the table is intact
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);
                }
            }
        }
    }

    // If we're OK, there's space for this entry
    if (rc == OK)
    {
        assert(pChain != NULL);

        // Scan looking for a key clash, also keeping track of the first empty slot we see
        iecsHashEntry_t *pEmptySlot = NULL;
        iecsHashEntry_t *pEntry = pChain->pEntries;
        iecsHashEntry_t *pClash = NULL;
        uint32_t remaining = pChain->Count;
        while (remaining > 0)
        {
            if (pEntry->pValue == NULL)
            {
                if (pEmptySlot == NULL)
                {
                    pEmptySlot = pEntry;
                }
            }
            else
            {
                ismEngine_ClientState_t *pCurrent = pEntry->pValue;

                if ((pEntry->Hash == hash) &&
                    (pCurrent->pThief == NULL) &&
                    (strcmp(pCurrent->pClientId, pClientId) == 0))
                {
                    pClash = pEntry;
                    break;
                }

                remaining--;
            }

            pEntry++;
        }

        // If we had a clash, we have to decide what to do - take over or bounce
        if (pClash != NULL)
        {
            ismEngine_ClientState_t *pPotentialVictim = pClash->pValue;

            uint32_t clientProtocolId = pClient->protocolId;
            uint32_t clashProtocolId = pPotentialVictim->protocolId;

            assert(fFromImport == false); // Don't expect to get a clash on import

            bool protocolMatch = (clientProtocolId == clashProtocolId);

            // If the protocolIds don't match - check for allowable protocol 'swaps'
            // and potentially force the steal of a clientState by a non-stealing
            // client.
            if (protocolMatch == false)
            {
                // Allow an internal engine clientState to take over from any other
                if (clientProtocolId == PROTOCOL_ID_ENGINE)
                {
                    protocolMatch = true;
                }
                // Allow any protocol to take over from an internal engine clientState.
                // Additionally, if the internal engine has a steal callback we are going
                // to allow someone who didn't specify ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL
                // to perform a steal.
                else if (clashProtocolId == PROTOCOL_ID_ENGINE)
                {
                    protocolMatch = true;

                    if (!fSteal && pPotentialVictim->pStealCallbackFn != NULL)
                    {
                        ieutTRACEL(pThreadData, pClient, ENGINE_INTERESTING_TRACE,
                                   "Forcing steal for protocol %d client '%s' (%p) (pClash=%p)",
                                   clientProtocolId, pClient->pClientId, pClient, pClash);

                        fSteal = true;
                    }
                }
                // Allow MQTT & HTTP to take over from one another.
                else
                {
                    protocolMatch = ((clashProtocolId == PROTOCOL_ID_MQTT ||
                                      clashProtocolId == PROTOCOL_ID_HTTP) &&
                                     (clientProtocolId == PROTOCOL_ID_MQTT ||
                                      clientProtocolId == PROTOCOL_ID_HTTP));
                }

                // The protocol must match in order to proceed
                if (protocolMatch == false)
                {
                    rc = ISMRC_ProtocolMismatch;
                    ism_common_setErrorData(rc, "%s%u", pClientId, pPotentialVictim->protocolId);
                }
            }

            // If the check user steal option is specified, then only allow the
            // steal if the userIDs of the old and new client states match.
            // A null value matches anything.
            if (fSteal && (options & ismENGINE_CREATE_CLIENT_OPTION_CHECK_USER_STEAL))
            {
                if (pClient->pUserId && pPotentialVictim->pUserId &&
                    strcmp(pClient->pUserId, pPotentialVictim->pUserId))
                {
                    rc = ISMRC_ClientIDInUse;
                    ism_common_setErrorData(rc, "%s%s", pClientId, pPotentialVictim->pUserId);
                }

            }

            // If the protocols do match, and both are 'engine' ones, let's see if this is a
            // repeat request and if so, return that the request is in progress.
            if (rc == OK && clientProtocolId == PROTOCOL_ID_ENGINE)
            {
                if (iecs_compareEngineClientStates(pThreadData, pClient, pPotentialVictim) == 0)
                {
                    rc = ISMRC_RequestInProgress;
                }
            }

            if (rc == OK)
            {
                assert(resourceSet == pPotentialVictim->resourceSet);

                pthread_spin_lock(&pPotentialVictim->UseCountLock);

                // Need to consider whether to inherit durability
                if (pClient->Durability == iecsInheritOrDurable ||
                    pClient->Durability == iecsInheritOrNonDurable)
                {
                    // If we're requesting clean start, or we'll get a clean start because the victim
                    // is being cleaned up, we can pick the requested durability, otherwise we inherit
                    // the victim's durability.
                    if (pClient->fCleanStart || pPotentialVictim->OpState == iecsOpStateZombieCleanup)
                    {
                        if (pClient->Durability == iecsInheritOrDurable)
                        {
                            pClient->Durability = iecsDurable;
                        }
                        else
                        {
                            pClient->Durability = iecsNonDurable;
                        }
                    }
                    else
                    {
                        pClient->Durability = pPotentialVictim->Durability;
                    }
                }

                assert(pClient->Durability == iecsDurable ||
                       pClient->Durability == iecsNonDurable);

                // If we're clashing with a zombie state, we can take the state
                if (pPotentialVictim->OpState == iecsOpStateZombie ||
                    pPotentialVictim->OpState == iecsOpStateZombieRemoval ||
                    pPotentialVictim->OpState == iecsOpStateZombieExpiry ||
                    pPotentialVictim->OpState == iecsOpStateZombieCleanup)
                {
                    if (contextLength > 0)
                    {
                        iere_primeThreadCache(pThreadData, resourceSet);
                        pClient->pPendingCreateContext = iere_malloc(pThreadData,
                                                                     resourceSet,
                                                                     IEMEM_PROBE(iemem_callbackContext, 4), contextLength);
                        if (pClient->pPendingCreateContext != NULL)
                        {
                            memcpy(pClient->pPendingCreateContext, pContext, contextLength);
                            pClient->pPendingCreateCallbackFn = pCallbackFn;
                        }
                        else
                        {
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);
                        }
                    }

                    bool bExpiredAtTakeover = false;

                    if (rc == OK)
                    {
                        // If we are durable, we will either need to commit our state to the store
                        // or will need to update the victim's client state.
                        if (pClient->Durability == iecsDurable)
                        {
                            // We haven't requested CleanStart but our victim is a non-durable zombie. We
                            // don't want to inherit anything from it (for example, a delayed will message)
                            // so let's force CleanStart.
                            if (pClient->fCleanStart == false && pPotentialVictim->Durability != iecsDurable)
                            {
                                ieutTRACEL(pThreadData, pPotentialVictim->Durability, ENGINE_HIGH_TRACE,
                                           "Forcing CleanStart by clientState %p of nondurable %p (clientId='%s')\n",
                                           pClient, pPotentialVictim, pClient->pClientId);
                                pClient->fCleanStart = true;
                            }

                            // We haven't already requested CleanStart, and our durable victim is not
                            // being explictly destroyed and has an expiry time. If it has passed, then
                            // we can actively destroy the old state by changing this client to request
                            // CleanStart.
                            if (pClient->fCleanStart == false &&
                                pPotentialVictim->Durability == iecsDurable &&
                                pPotentialVictim->fDiscardDurable == false &&
                                pPotentialVictim->ExpiryTime != 0)
                            {
                                ism_time_t now = ism_common_currentTimeNanos();

                                if (now >= pPotentialVictim->ExpiryTime)
                                {
                                    ieutTRACEL(pThreadData, pPotentialVictim->ExpiryTime, ENGINE_HIGH_TRACE,
                                               "Forcing CleanStart by clientState %p of expired %p (clientId='%s')\n",
                                               pClient, pPotentialVictim, pClient->pClientId);

                                    pClient->fCleanStart = true;
                                    bExpiredAtTakeover = true;
                                }
                            }

                            if (pClient->fCleanStart ||
                                pPotentialVictim->fDiscardDurable ||
                                pPotentialVictim->Durability != iecsDurable ||
                                pPotentialVictim->OpState == iecsOpStateZombieCleanup)
                            {
                                if (storeStatus == StatusOk)
                                {
                                    // If we are requesting a clean start, or the victim is being explicitly
                                    // discarded or is in ZombieCleanup, *they* will write the new store record
                                    // when the steal finally happens, we don't want to write one here.
                                    fStoreNeeded = !pClient->fCleanStart &&
                                                   !pPotentialVictim->fDiscardDurable &&
                                                   (pPotentialVictim->OpState != iecsOpStateZombieCleanup);

                                    assert(fUpdateLastConnectedTime == false);

                                    if (!fStoreNeeded)
                                    {
                                        ieutTRACEL(pThreadData, pClient, ENGINE_UNUSUAL_TRACE,
                                                   "Initiating client-ID '%s' theft of zombie clientState %p by clientState %p, needing new CSR (old CSR=0x%016lx).\n",
                                                   pPotentialVictim->pClientId, pPotentialVictim, pClient, pPotentialVictim->hStoreCSR);
                                    }
                                }
                                else
                                {
                                    rc = ISMRC_ServerCapacity;
                                    ism_common_setError(rc);

                                    if (contextLength > 0)
                                    {
                                        iere_primeThreadCache(pThreadData, resourceSet);
                                        iere_free(pThreadData, resourceSet, iemem_callbackContext, pClient->pPendingCreateContext);

                                        pClient->pPendingCreateContext = NULL;
                                        pClient->pPendingCreateCallbackFn = NULL;
                                    }

                                    ieutTRACEL(pThreadData, storeStatus, ENGINE_INTERESTING_TRACE,
                                               "Rejecting creation of ClientState for '%s'. rc=%d, storeStatus=%d\n",
                                               pClientId, rc, (int32_t)storeStatus);
                                }
                            }
                            else
                            {
                                if (pClient->Takeover == iecsNoTakeover)
                                {
                                    pClient->Takeover = iecsZombieTakeover;
                                }
                                fUpdateLastConnectedTime = true;
                            }
                        }
                    }

                    if (rc == OK)
                    {
                        pVictim = pPotentialVictim;

                        assert(pVictim->pThief == NULL);
                        assert(pVictim->UseCount >= 1);
                        assert(pVictim->pPendingCreateCallbackFn == NULL);

                        // Don't need to lock this client since no-one else can see it (yet)
                        pClient->UseCount += 1;

                        // We identify the new client-state as the zombie victim's "thief"
                        pVictim->pThief = pClient;
                        pVictim->UseCount += 1;
                        if (pVictim->OpState == iecsOpStateZombie)
                        {
                            if (bExpiredAtTakeover)
                            {
                                pVictim->OpState = iecsOpStateZombieExpiry;
                                pThreadData->stats.expiredClientStates += 1;
                            }
                            else
                            {
                                pVictim->OpState = iecsOpStateZombieRemoval;
                            }

                            if (pVictim->ExpiryTime != 0)
                            {
                                pVictim->ExpiryTime = 0;
                                pThreadData->stats.zombieSetToExpireCount -= 1;
                            }
                        }
                    }
                }
                // Not a zombie, is this a valid steal request?
                else if (fSteal && (pPotentialVictim->pStealCallbackFn != NULL))
                {
                    // We have a victim
                    if (contextLength > 0)
                    {
                        iere_primeThreadCache(pThreadData, resourceSet);
                        pClient->pPendingCreateContext = iere_malloc(pThreadData,
                                                                     resourceSet,
                                                                     IEMEM_PROBE(iemem_callbackContext, 1), contextLength);
                        if (pClient->pPendingCreateContext != NULL)
                        {
                            memcpy(pClient->pPendingCreateContext, pContext, contextLength);
                            pClient->pPendingCreateCallbackFn = pCallbackFn;
                        }
                        else
                        {
                            rc = ISMRC_AllocateError;
                            ism_common_setError(rc);
                        }
                    }

                    if (rc == OK)
                    {
                        // If the thief (pClient) is durable and either requested a clean start or
                        // the existing client (pPotentialVictim) is being explicitly discarded or
                        // was not durable then we check if the store status is okay, and if it is,
                        // defer writing the store record until the steal happens.
                        if (pClient->Durability == iecsDurable)
                        {
                            if (pClient->fCleanStart ||
                                pPotentialVictim->fDiscardDurable ||
                                pPotentialVictim->Durability != iecsDurable)
                            {
                                if (storeStatus == StatusOk)
                                {
                                    // We wait until the steal happens to write the store record.
                                    assert(fStoreNeeded == false);
                                    assert(fUpdateLastConnectedTime == false);

                                    ieutTRACEL(pThreadData, pClient, ENGINE_NORMAL_TRACE,
                                               "Initiating client-ID '%s' theft of clientState %p by clientState %p, needing new CSR (old CSR=0x%016lx).\n",
                                               pPotentialVictim->pClientId, pPotentialVictim, pClient, pPotentialVictim->hStoreCSR);
                                }
                                else
                                {
                                    rc = ISMRC_ServerCapacity;
                                    ism_common_setError(rc);

                                    if (contextLength > 0)
                                    {
                                        iere_primeThreadCache(pThreadData, resourceSet);
                                        iere_free(pThreadData, resourceSet, iemem_callbackContext, pClient->pPendingCreateContext);

                                        pClient->pPendingCreateContext = NULL;
                                        pClient->pPendingCreateCallbackFn = NULL;
                                    }

                                    ieutTRACEL(pThreadData, storeStatus, ENGINE_INTERESTING_TRACE,
                                               "Rejecting creation of ClientState for '%s'. rc=%d, storeStatus=%d\n",
                                               pClientId, rc, (int32_t)storeStatus);
                                }
                            }
                            else
                            {
                                // The store record will be given to us
                                assert(fStoreNeeded == false);
                                pClient->Takeover = iecsStealTakeover;
                            }
                        }

                        // Want to remember that this steal is happening for some special reason...
                        if ((internalOptions & iecsCREATE_CLIENT_OPTION_NONACKING_TAKEOVER) != 0)
                        {
                            assert(pClient->protocolId == PROTOCOL_ID_ENGINE);
                            pClient->Takeover = iecsNonAckingClient;
                        }
                    }

                    if (rc == OK)
                    {
                        pVictim = pPotentialVictim;

                        assert(pVictim->pThief == NULL);
                        assert(pVictim->UseCount >= 1);

                        // Don't need to lock this client since no-one else can see it (yet)
                        pClient->UseCount += 1;

                        // We identify the new client-state as the victim's "thief"
                        // and each contributes one to the other's use count
                        pVictim->pThief = pClient;
                        pVictim->UseCount += 1;
                        if (pVictim->pPendingCreateCallbackFn == NULL)
                        {
                            pStealCallbackFn = pVictim->pStealCallbackFn;
                            pStealContext = pVictim->pStealContext;
                            pVictim->pStealCallbackFn = NULL;
                            pVictim->pStealContext = NULL;
                        }
                    }
                }
                // The client id is still in use
                else
                {
                    rc = ISMRC_ClientIDInUse;
                    ism_common_setErrorData(rc, "%s", pClientId);
                }

                pthread_spin_unlock(&pPotentialVictim->UseCountLock);
            }
        }
        else
        {
            if ((internalOptions & iecsCREATE_CLIENT_OPTION_NONACKING_TAKEOVER) != 0)
            {
                rc = ISMRC_NotFound;
            }
            else
            {
                assert(pStealCallbackFn == NULL);

                // For a durable client, we need to create a Client State Record
                // in the Store
                if (pClient->Durability == iecsDurable || pClient->Durability == iecsInheritOrDurable)
                {
                    pClient->Durability = iecsDurable; // Fix the Inherit case.

                    if (storeStatus == StatusOk)
                    {
                        fStoreNeeded = true;
                    }
                    else
                    {
                        rc = ISMRC_ServerCapacity;
                        ism_common_setError(rc);

                        ieutTRACEL(pThreadData, storeStatus, ENGINE_INTERESTING_TRACE,
                                   "Rejecting creation of ClientState for '%s'. rc=%d storeStatus=%d\n",
                                   pClientId, rc, (int32_t)storeStatus);
                    }
                }
                else
                {
                    pClient->Durability = iecsNonDurable; // Fix the Inherit case
                }
            }
        }

        // We now need to add the new client-state to its chain
        if (rc == OK)
        {
            // An imported clientState will already have had an expiryInterval set from the import
            if (!fFromImport)
            {
                if (pClient->Durability == iecsDurable && pClient->pSecContext != NULL)
                {
                    pClient->ExpiryInterval = ism_security_context_getClientStateExpiry(pClient->pSecContext);
                }
                else
                {
                    // For durables with no security context, and even for non-durable client states
                    // we set their expiry to INFINITE.
                    // The rationale behind doing so for non-durable is that for non-durable clients
                    // which create durable objects (JMS) we want them to stay until the objects have
                    // all been destroyed, and then we will discard them immediately anyway.
                    pClient->ExpiryInterval = iecsEXPIRY_INTERVAL_INFINITE;
                }
            }

            if (pEmptySlot == NULL)
            {
                assert(pEntry != NULL);

                while (true)
                {
                    if (pEntry->pValue == NULL)
                    {
                        pEmptySlot = pEntry;
                        break;
                    }

                    pEntry++;
                }

                assert(pEmptySlot != NULL);
            }

            pEmptySlot->pValue = pClient;
            pEmptySlot->Hash = hash;
            pClient->pHashEntry = pEmptySlot;
            pClient->fTableRemovalRequired = true;
            pChain->Count++;
            pTable->TotalEntryCount++;

            if (pClient->fCountExternally)
            {
                ismEngine_serverGlobal.totalClientStatesCount++;
                if (pClient->Durability == iecsDurable)
                {
                    iere_primeThreadCache(pThreadData, resourceSet);
                    iere_updateInt64Stat(pThreadData, resourceSet,
                                         ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_CLIENT_STATES, 1);
                }
            }
        }
    }

    ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

    bool stealing = (pVictim != NULL);

    // Set up for the store commits to go async
    iecsClientStateAdditionInfo_t additionInfo = {pClient, pVictim, pStealCallbackFn, pStealContext};

    ismEngine_AsyncDataEntry_t asyncArray[IEAD_MAXARRAYENTRIES] = {
        {ismENGINE_ASYNCDATAENTRY_STRUCID, ClientStateFinishAdditionInfo,
            NULL, 0, fFromImport ? NULL : pClient, {.internalFn = &iecs_asyncFinishClientStateAddition}},
        {ismENGINE_ASYNCDATAENTRY_STRUCID, EngineCaller,
            pContext, contextLength, NULL,
            {.externalFn = pCallbackFn }},
        {ismENGINE_ASYNCDATAENTRY_STRUCID, ClientStateAdditionCompletionInfo,
            &additionInfo, sizeof(additionInfo), NULL,
            {.internalFn = &iecs_asyncAddClientStateCompletion}}};

    ismEngine_AsyncData_t asyncData = {ismENGINE_ASYNCDATA_STRUCID,
                                       pClient,
                                       IEAD_MAXARRAYENTRIES, 3, 0, true,  0, 0, asyncArray};

    // If this is a steal (either a Zombie takeover, or an Active steal) then we expect
    // the end of the steal process to call the engine caller, so we remove that entry
    // from the asyncData.
    if (stealing)
    {
        uint32_t engineCallerIndex = 1;

        assert(asyncData.entries[engineCallerIndex].Type == EngineCaller);

        memmove(&asyncData.entries[engineCallerIndex],
                &asyncData.entries[engineCallerIndex+1],
                sizeof(ismEngine_AsyncDataEntry_t) * (asyncData.numEntriesUsed-engineCallerIndex));

        asyncData.numEntriesUsed -= 1;
    }

    if (!fFromImport)
    {
        iecs_acquireClientStateReference(pClient);
    }

    if (fStoreNeeded)
    {
        assert(rc == OK);
        assert(fUpdateLastConnectedTime == false);
        assert(pClient->Durability == iecsDurable);
        assert(pVictim == NULL || pVictim->Durability != iecsDurable);

        rc = iecs_storeClientState(pThreadData,
                                   pClient,
                                   fFromImport,
                                   &asyncData);

        assert(rc == OK || rc == ISMRC_AsyncCompletion);
    }
    else if (fUpdateLastConnectedTime)
    {
        assert(rc == OK);
        assert(pVictim != NULL);

        rc = iecs_updateLastConnectedTime(pThreadData, pVictim, true, &asyncData);

        assert(rc == OK || rc == ISMRC_AsyncCompletion);
    }

    if (!fFromImport && rc != ISMRC_AsyncCompletion)
    {
        iecs_releaseClientStateReference(pThreadData,
                                         pClient,
                                         false, false);
    }

    if (rc == OK)
    {
        rc = iecs_finishClientStateAddition(pThreadData, &additionInfo, !stealing, stealing);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "(pClient %p) rc=%d\n", __func__,
               pClient, rc);

    return rc;
}


/*
 * Adds a client-state to the client-state table, simplified for recovery only
 */
int32_t iecs_addClientStateRecovery(ieutThreadData_t *pThreadData,
                                    ismEngine_ClientState_t *pClient)
{
    iecsHashTable_t *pTable;
    char *pClientId = pClient->pClientId;
    uint32_t hash;
    iecsHashChain_t *pChain=NULL;  // Initialise the pointer to NULL
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    hash = calculateHash(pClientId);

    pTable = ismEngine_serverGlobal.ClientTable;

    // First see if the table has reached its loading limit and needs resizing
    if (pTable->fCanResize &&
        (pTable->TotalEntryCount >= pTable->ChainCount * iecsHASH_TABLE_LOADING_LIMIT))
    {
        iecsHashTable_t *pNewTable = NULL;

        rc = iecs_resizeClientStateTable(pThreadData, pTable, &pNewTable);
        if (rc == OK)
        {
            iecs_freeClientStateTable(pThreadData, pTable, false);
            ismEngine_serverGlobal.ClientTable = pNewTable;
            pTable = pNewTable;
        }
        else if (rc == ISMRC_AllocateError)
        {
            // OK, so we couldn't resize the table, but we may still be able to
            // insert this entry into the existing table. The efficiency of a
            // larger table is not so much greater that we can't cope.
            pTable->fCanResize = false;
            rc = OK;
        }
    }

    // See if the chosen chain is already full - do it eagerly in the anticipation that
    // clashes are rare and we want the logic simple
    if (rc == OK)
    {
        uint32_t chain = hash & pTable->ChainMask;
        pChain = pTable->pChains + chain;

        if (pChain->Count == pChain->Limit)
        {
            iecsHashEntry_t *pNewEntries = iemem_calloc(pThreadData,
                                                        IEMEM_PROBE(iemem_clientState, 10),
                                                        pChain->Limit + iecsHASH_TABLE_CHAIN_INCREMENT,
                                                        sizeof(iecsHashEntry_t));
            if (pNewEntries != NULL)
            {
                // Copy the entries from the old bucket to the new one
                if (pChain->pEntries != NULL)
                {
                    iecsHashEntry_t *pEntry = pChain->pEntries;
                    iecsHashEntry_t *pNewEntry = pNewEntries;
                    uint32_t remaining = pChain->Count;
                    while (remaining > 0)
                    {
                        if (pEntry->pValue != NULL)
                        {
                            pNewEntry->pValue = pEntry->pValue;
                            pNewEntry->Hash = pEntry->Hash;
                            pNewEntry->pValue->pHashEntry = pNewEntry;
                            assert(pNewEntry->pValue->fTableRemovalRequired == true);
                            pNewEntry++;

                            remaining--;
                        }

                        pEntry++;
                    }

                    iemem_free(pThreadData, iemem_clientState, pChain->pEntries);
                }

                pChain->Limit += iecsHASH_TABLE_CHAIN_INCREMENT;
                pChain->pEntries = pNewEntries;
            }
            else
            {
                // The new entry has not been added, but the table is intact
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }
    }

    // If we're OK, there's space for this entry
    if (rc == OK)
    {
        // Scan looking for a key clash, also keeping track of the first empty slot we see
        iecsHashEntry_t *pEmptySlot = NULL;
        iecsHashEntry_t *pEntry = pChain->pEntries;
        iecsHashEntry_t *pClash = NULL;
        uint32_t remaining = pChain->Count;
        while (remaining > 0)
        {
            if (pEntry->pValue == NULL)
            {
                if (pEmptySlot == NULL)
                {
                    pEmptySlot = pEntry;
                }
            }
            else
            {
                ismEngine_ClientState_t *pCurrent = pEntry->pValue;

                if ((pEntry->Hash == hash) &&
                    (strcmp(pCurrent->pClientId, pClientId) == 0))
                {
                    pClash = pEntry;
                    if (pClient->fDiscardDurable || !pClash->pValue->fDiscardDurable)
                    {
                        break;
                    }
                }

                remaining--;
            }

            pEntry++;
        }

        // Clashes are not allowed unless one is a discardable client
        if (pClash != NULL)
        {
            ieutTRACEL(pThreadData, pClash, ENGINE_SEVERE_ERROR_TRACE,
                       "State for client %s already exists\n", pClientId);

            // Keep the clash
            if (pClient->fDiscardDurable)
            {
                pClient->fLeaveResourcesAtRestart = true;
            }
            // Keep this new client (inherit from the clash)
            else if (pClash->pValue->fDiscardDurable)
            {
                // TODO: What can / should this client inherit from the clash?
                pClient->durableObjects = pClash->pValue->durableObjects;
                pClash->pValue->durableObjects = 0;

                pClash->pValue->fLeaveResourcesAtRestart = true;
            }
            else
            {
                rc = ISMRC_ClientIDInUse;
                ism_common_setErrorData(rc, "%s", pClientId);
            }
        }

        // We now need to add the new client-state to its chain
        if (rc == OK)
        {
            if (pEmptySlot == NULL)
            {
                assert(pEntry != NULL);

                while (true)
                {
                    if (pEntry->pValue == NULL)
                    {
                        pEmptySlot = pEntry;
                        break;
                    }

                    pEntry++;
                }

                assert(pEmptySlot != NULL);
            }

            pEmptySlot->pValue = pClient;
            pEmptySlot->Hash = hash;
            pClient->pHashEntry = pEmptySlot;
            pthread_spin_lock(&pClient->UseCountLock);
            pClient->fTableRemovalRequired = true;
            pthread_spin_unlock(&pClient->UseCountLock);
            pChain->Count++;
            pTable->TotalEntryCount++;

            if (pClient->fCountExternally)
            {
                ismEngine_serverGlobal.totalClientStatesCount++;
                if (pClient->Durability == iecsDurable)
                {
                    iere_primeThreadCache(pThreadData, resourceSet);
                    iere_updateInt64Stat(pThreadData, resourceSet,
                                         ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_CLIENT_STATES, 1);
                }
            }
        }
    }

    return rc;
}

/*
 * Add an unreleased delivery
 */
int32_t iecs_addUnreleasedDelivery(ieutThreadData_t *pThreadData,
                                   ismEngine_ClientState_t *pClient,
                                   ismEngine_Transaction_t *pTran,
                                   uint32_t unrelDeliveryId)
{
    ismEngine_UnreleasedState_t *pUnrelChunk;
    int16_t slotNumber;
    bool fSlotFound = false;
    bool fLocked;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, unrelDeliveryId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "(pClient %p, pTran %p, deliveryId %u)\n", __func__, pClient, pTran, unrelDeliveryId);

    iecs_lockUnreleasedDeliveryState(pClient);
    fLocked = true;

    // Check the existing chunks for the supplied delivery ID. If we
    // already have this ID, just carry on unless it's uncommitted
    pUnrelChunk = pClient->pUnreleasedHead;
    while (pUnrelChunk != NULL)
    {
        for (slotNumber = 0; slotNumber < pUnrelChunk->Limit; slotNumber++)
        {
            if (pUnrelChunk->Slot[slotNumber].fSlotInUse)
            {
                if (pUnrelChunk->Slot[slotNumber].UnrelDeliveryId == unrelDeliveryId)
                {
                    fSlotFound = true;

                    if (pUnrelChunk->Slot[slotNumber].fUncommitted)
                    {
                        rc = ISMRC_LockNotGranted;
                        ism_common_setError(rc);
                    }

                    break;
                }
            }
        }

        if (fSlotFound)
        {
            break;
        }

        pUnrelChunk = pUnrelChunk->pNext;
    }

    // If we haven't found the delivery ID already, we need to create a new one
    if (!fSlotFound)
    {
        // Check the existing chunks for a gap
        pUnrelChunk = pClient->pUnreleasedHead;
        while (pUnrelChunk != NULL)
        {
            for (slotNumber = 0; slotNumber < pUnrelChunk->Capacity; slotNumber++)
            {
                if (!pUnrelChunk->Slot[slotNumber].fSlotInUse)
                {
                    pUnrelChunk->Slot[slotNumber].fSlotInUse = true;
                    pUnrelChunk->Slot[slotNumber].fUncommitted = (pTran != NULL) ? true : false;
                    pUnrelChunk->Slot[slotNumber].UnrelDeliveryId = unrelDeliveryId;
                    pUnrelChunk->Slot[slotNumber].hStoreUnrelStateObject = ismSTORE_NULL_HANDLE;

                    if (slotNumber >= pUnrelChunk->Limit)
                    {
                        pUnrelChunk->Limit = slotNumber + 1;
                    }
                    pUnrelChunk->InUse++;

                    fSlotFound = true;
                    break;
                }
            }

            if (fSlotFound)
            {
                break;
            }

            pUnrelChunk = pUnrelChunk->pNext;
        }

        // New slots are added at the head
        if (!fSlotFound)
        {
            iereResourceSetHandle_t resourceSet = pClient->resourceSet;

            iere_primeThreadCache(pThreadData, resourceSet);
            pUnrelChunk = iere_calloc(pThreadData,
                                      resourceSet,
                                      IEMEM_PROBE(iemem_clientState, 11), 1, sizeof(ismEngine_UnreleasedState_t));
            if (pUnrelChunk != NULL)
            {
                memcpy(pUnrelChunk->StrucId, ismENGINE_UNRELEASED_STATE_STRUCID, 4);
                pUnrelChunk->InUse = 1;
                pUnrelChunk->Limit = 1;
                pUnrelChunk->Capacity = ismENGINE_UNRELEASED_CHUNK_SIZE;
                pUnrelChunk->Slot[0].fSlotInUse = true;
                pUnrelChunk->Slot[0].fUncommitted = (pTran != NULL) ? true : false;
                pUnrelChunk->Slot[0].UnrelDeliveryId = unrelDeliveryId;
                pUnrelChunk->Slot[0].hStoreUnrelStateObject = ismSTORE_NULL_HANDLE;
                pUnrelChunk->pNext = pClient->pUnreleasedHead;
                pClient->pUnreleasedHead = pUnrelChunk;
                slotNumber = 0;
            }
            else
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }

        // For a durable client-state, we need to create a UMS in the Store.
        // If it's in a transaction, we need an SLE.
        if ((rc == OK) && (pClient->Durability == iecsDurable || (pTran != NULL)))
        {
            // Now because we are going to start a store-transaction, we release the client-state
            // lock. We might be forced to wait for a new generation and this could take seconds.
            pUnrelChunk->Slot[slotNumber].fUncommitted = true;
            fLocked = false;
            iecs_unlockUnreleasedDeliveryState(pClient);

            ismStore_Handle_t hStoreUMS = ismSTORE_NULL_HANDLE;

            rc = iecs_storeUnreleasedMessageState(pThreadData,
                                                  pClient,
                                                  pTran,
                                                  unrelDeliveryId,
                                                  pUnrelChunk,
                                                  slotNumber,
                                                  &hStoreUMS);

            iecs_lockUnreleasedDeliveryState(pClient);
            fLocked = true;

            if (rc == OK)
            {
                pUnrelChunk->Slot[slotNumber].hStoreUnrelStateObject = hStoreUMS;
                pUnrelChunk->Slot[slotNumber].fUncommitted = (pTran != NULL) ? true : false;
            }
            else
            {
                // If the store operations failed, backout the slot allocation
                pUnrelChunk->Slot[slotNumber].fSlotInUse = false;
                pUnrelChunk->Slot[slotNumber].fUncommitted = false;
                pUnrelChunk->Slot[slotNumber].UnrelDeliveryId = 0;
                pUnrelChunk->Slot[slotNumber].hStoreUnrelStateObject = ismSTORE_NULL_HANDLE;
                pUnrelChunk->InUse--;
            }
        }
    }

    if (fLocked)
    {
        iecs_unlockUnreleasedDeliveryState(pClient);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

/*
 * Add an unreleased delivery
 * TODO: Should this import specific function go into exportClientState.c?
 */
int32_t iecs_importUnreleasedDeliveryIds(ieutThreadData_t *pThreadData,
                                         ismEngine_ClientState_t *pClient,
                                         uint32_t *unrelDeliveryIds,
                                         uint32_t unrelDeliveryIdCount,
                                         ismStore_CompletionCallback_t pCallback,
                                         void *pContext)
{
    ismEngine_UnreleasedState_t *pUnrelChunk;
    bool fLocked = false;
    int32_t rc = OK;
    uint32_t storeOperations = 0;
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    ieutTRACEL(pThreadData, unrelDeliveryIdCount, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "(pClient %p, deliveryIdCount %u)\n", __func__, pClient, unrelDeliveryIdCount);

    if (unrelDeliveryIdCount == 0) goto mod_exit;

    iecs_lockUnreleasedDeliveryState(pClient);
    fLocked = true;

    assert(pClient->pUnreleasedHead == NULL);

    // For a durable client-state, we will need to create a UMS in the Store.
    if (pClient->Durability == iecsDurable)
    {
        assert(pClient->hStoreCSR != ismSTORE_NULL_HANDLE);
        assert(pClient->hUnreleasedStateContext == NULL);

        rc = ism_store_openStateContext(pClient->hStoreCSR,
                                        &pClient->hUnreleasedStateContext);
        if (rc != OK)
        {
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    // Need some number of chunks for this set
    uint32_t chunkNumber = (unrelDeliveryIdCount / ismENGINE_UNRELEASED_CHUNK_SIZE) +
                           ((unrelDeliveryIdCount % ismENGINE_UNRELEASED_CHUNK_SIZE) ? 1 : 0);

    uint32_t *thisUnrelDeliveryId = unrelDeliveryIds;
    uint32_t remainingUnrelDeliveryIds = unrelDeliveryIdCount;
    while(chunkNumber > 0)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        pUnrelChunk = iere_calloc(pThreadData,
                                  resourceSet,
                                  IEMEM_PROBE(iemem_clientState, 23), 1, sizeof(ismEngine_UnreleasedState_t));

        if (pUnrelChunk == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        memcpy(pUnrelChunk->StrucId, ismENGINE_UNRELEASED_STATE_STRUCID, 4);
        pUnrelChunk->Capacity = ismENGINE_UNRELEASED_CHUNK_SIZE;
        pUnrelChunk->InUse = (chunkNumber == 1) ? (uint8_t)remainingUnrelDeliveryIds :
                                                  (uint8_t)ismENGINE_UNRELEASED_CHUNK_SIZE;

        for(uint8_t slot=0; slot<pUnrelChunk->InUse; slot++)
        {
            pUnrelChunk->Slot[slot].fSlotInUse = true;
            pUnrelChunk->Slot[slot].fUncommitted = false;
            pUnrelChunk->Slot[slot].UnrelDeliveryId = *thisUnrelDeliveryId++;
            remainingUnrelDeliveryIds--;

            pUnrelChunk->Slot[slot].hStoreUnrelStateObject = ismSTORE_NULL_HANDLE;

            if (pClient->hUnreleasedStateContext != NULL)
            {
                ismStore_StateObject_t storeStateObj = { pUnrelChunk->Slot[slot].UnrelDeliveryId };

                rc = ism_store_createState(pThreadData->hStream,
                                           pClient->hUnreleasedStateContext,
                                           &storeStateObj,
                                           &pUnrelChunk->Slot[slot].hStoreUnrelStateObject);

                if (rc != OK)
                {
                    ism_common_setError(rc);
                    goto mod_exit;
                }

                storeOperations += 1;
            }
        }

        pUnrelChunk->Limit = pUnrelChunk->InUse;

        pUnrelChunk->pNext = pClient->pUnreleasedHead;
        pClient->pUnreleasedHead = pUnrelChunk;

        chunkNumber--;
    }

    assert(chunkNumber == 0);
    assert(fLocked == true);

    // Unlock to do the commit
    iecs_unlockUnreleasedDeliveryState(pClient);
    fLocked = false;

    if (storeOperations != 0)
    {
        rc = iest_store_asyncCommit(pThreadData, false, pCallback, pContext);
    }
    else
    {
        assert(rc == OK);
    }

mod_exit:

    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        pUnrelChunk = pClient->pUnreleasedHead;
        while(pUnrelChunk)
        {
            ismEngine_UnreleasedState_t *pNextChunk = pUnrelChunk->pNext;
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_free(pThreadData, resourceSet, iemem_clientState, pUnrelChunk);
            pUnrelChunk = pNextChunk;
        }
        pClient->pUnreleasedHead = NULL;
        if (pClient->hUnreleasedStateContext != NULL)
        {
            if (storeOperations != 0)
            {
                iest_store_rollback(pThreadData, false);
            }
            ism_store_closeStateContext(pClient->hUnreleasedStateContext);
            pClient->hUnreleasedStateContext = NULL;
        }
    }

    if (fLocked) iecs_unlockUnreleasedDeliveryState(pClient);

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

typedef struct tag_iecsRemoveUnreleasedDeliveryInfo_t
{
    ismEngine_UnreleasedState_t *pUnrelChunk;
    ismEngine_Transaction_t *pTran;
    uint32_t unrelDeliveryId;
    int16_t slotNumber;
} iecsRemoveUnreleasedDeliveryInfo_t;

void iecs_finishRemoveUnreleasedDelivery(ieutThreadData_t *pThreadData,
                                         ismEngine_ClientState_t *pClient,
                                         bool fAlreadyLocked,
                                         ismEngine_UnreleasedState_t *pUnrelChunk,
                                         int16_t slotNumber,
                                         ismEngine_Transaction_t *pTran,
                                         uint32_t unrelDeliveryId)
{
    if (!fAlreadyLocked) iecs_lockUnreleasedDeliveryState(pClient);

    if (pTran != NULL)
    {
        assert(pUnrelChunk->Slot[slotNumber].fUncommitted == true);
    }
    else
    {
        assert(pUnrelChunk->Slot[slotNumber].UnrelDeliveryId == unrelDeliveryId);

        pUnrelChunk->Slot[slotNumber].UnrelDeliveryId = 0;
        pUnrelChunk->Slot[slotNumber].hStoreUnrelStateObject = ismSTORE_NULL_HANDLE;
        pUnrelChunk->Slot[slotNumber].fUncommitted = false;
        pUnrelChunk->Slot[slotNumber].fSlotInUse = false;

        pUnrelChunk->InUse--;

        // If the current chunk is empty we remove it if it is not the head chunk
        if (pUnrelChunk->InUse == 0)
        {
            ismEngine_UnreleasedState_t *pCurrChunk = pClient->pUnreleasedHead;
            ismEngine_UnreleasedState_t *pPrevChunk = NULL;

            assert(pCurrChunk != NULL); // It must at least contain this chunk!

            while (pCurrChunk != pUnrelChunk)
            {
                pPrevChunk = pCurrChunk;
                pCurrChunk = pCurrChunk->pNext;
            }

            if (pPrevChunk != NULL)
            {
                iereResourceSetHandle_t resourceSet = pClient->resourceSet;
                pPrevChunk->pNext = pUnrelChunk->pNext;
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_freeStruct(pThreadData, resourceSet, iemem_clientState, pUnrelChunk, pUnrelChunk->StrucId);
            }
        }
    }

    if (!fAlreadyLocked) iecs_unlockUnreleasedDeliveryState(pClient);
}

static int32_t iecs_asyncUnstoreUnreleasedMessageState(ieutThreadData_t *pThreadData,
                                                      int32_t rc,
                                                      ismEngine_AsyncData_t *asyncInfo,
                                                      ismEngine_AsyncDataEntry_t *context)
{
    assert(context->Type == ClientStateUnstoreUnreleasdMessageStateCompletionInfo);
    assert(rc == OK); //Store commit shouldn't fail (fatal fdc here?)

    iecsRemoveUnreleasedDeliveryInfo_t *pInfo = (iecsRemoveUnreleasedDeliveryInfo_t *)(context->Data);

    ismEngine_UnreleasedState_t *pUnrelChunk = pInfo->pUnrelChunk;
    int16_t slotNumber = pInfo->slotNumber;
    ismEngine_Transaction_t *pTran = pInfo->pTran;
    uint32_t unrelDeliveryId = pInfo->unrelDeliveryId;

    iead_popAsyncCallback(asyncInfo, context->DataLen);

    iecs_finishRemoveUnreleasedDelivery(pThreadData,
                                             asyncInfo->pClient,
                                             false, // Not already locked
                                             pUnrelChunk,
                                             slotNumber,
                                             pTran,
                                             unrelDeliveryId);

    return rc;
}

/*
 * Remove an unreleased delivery
 */
int32_t iecs_removeUnreleasedDelivery(ieutThreadData_t *pThreadData,
                                      ismEngine_ClientState_t *pClient,
                                      ismEngine_Transaction_t *pTran,
                                      uint32_t unrelDeliveryId,
                                      ismEngine_AsyncData_t *pAsyncData)
{
    ismEngine_UnreleasedState_t *pUnrelChunk;
    int16_t i;
    bool fLocked;
    int32_t rc = OK;

    iecs_lockUnreleasedDeliveryState(pClient);
    fLocked = true;

    // Check the existing chunks for the supplied delivery ID. If we already have this ID, just carry on
    pUnrelChunk = pClient->pUnreleasedHead;
    while (pUnrelChunk != NULL)
    {
        for (i = 0; i < pUnrelChunk->Limit; i++)
        {
            if (pUnrelChunk->Slot[i].fSlotInUse)
            {
                if (pUnrelChunk->Slot[i].UnrelDeliveryId == unrelDeliveryId)
                {
                    if (pUnrelChunk->Slot[i].fUncommitted)
                    {
                        rc = ISMRC_LockNotGranted;
                        ism_common_setError(rc);
                        break;
                    }

                    if ((pTran != NULL) ||
                        (pUnrelChunk->Slot[i].hStoreUnrelStateObject != ismSTORE_NULL_HANDLE))
                    {
                        iecsRemoveUnreleasedDeliveryInfo_t info = {pUnrelChunk, pTran, unrelDeliveryId, i};

                        // Now because we are going to potentially go async with store operations we release
                        // the client delivery state lock - marking this entry as uncommitted and in the process
                        // of removal.
                        pUnrelChunk->Slot[i].fUncommitted = true;
                        fLocked = false;
                        iecs_unlockUnreleasedDeliveryState(pClient);

                        // Add the work required when async commit finishes.
                        ismEngine_AsyncDataEntry_t newEntry =
                                    { ismENGINE_ASYNCDATAENTRY_STRUCID
                                    , ClientStateUnstoreUnreleasdMessageStateCompletionInfo
                                    , &info, sizeof(info)
                                    , NULL
                                    , {.internalFn = &iecs_asyncUnstoreUnreleasedMessageState}};

                        iead_pushAsyncCallback(pThreadData, pAsyncData, &newEntry);

                        rc = iecs_unstoreUnreleasedMessageState(pThreadData,
                                                                pClient,
                                                                pTran,
                                                                pUnrelChunk,
                                                                i,
                                                                pUnrelChunk->Slot[i].hStoreUnrelStateObject,
                                                                pAsyncData);

                        if (rc == ISMRC_AsyncCompletion) goto mod_exit;

                        iecs_lockUnreleasedDeliveryState(pClient);
                        fLocked = true;

                        if (rc != OK)
                        {
                            pUnrelChunk->Slot[i].fUncommitted = false;
                            goto mod_exit;
                        }
                    }

                    iecs_finishRemoveUnreleasedDelivery(pThreadData, pClient, fLocked, pUnrelChunk, i, pTran, unrelDeliveryId);
                    goto mod_exit;
                }
            }
        }

        pUnrelChunk = pUnrelChunk->pNext;
    }

mod_exit:
    if (fLocked)
    {
        iecs_unlockUnreleasedDeliveryState(pClient);
    }

    return rc;
}


/*
 * List unreleased deliveries
 */
int32_t iecs_listUnreleasedDeliveries(ismEngine_ClientState_t *pClient,
                                      void *pContext,
                                      ismEngine_UnreleasedCallback_t  pUnrelCallbackFunction)
{
    ismEngine_UnreleasedState_t *pUnrelChunk;
    int16_t i;
    int32_t rc = OK;

    iecs_lockUnreleasedDeliveryState(pClient);

    // Just loop through the chunks calling the callback (under the client-state lock)
    pUnrelChunk = pClient->pUnreleasedHead;
    while (pUnrelChunk != NULL)
    {
        for (i = 0; i < pUnrelChunk->Limit; i++)
        {
            if (pUnrelChunk->Slot[i].fSlotInUse && !pUnrelChunk->Slot[i].fUncommitted)
            {
                (*pUnrelCallbackFunction)(pUnrelChunk->Slot[i].UnrelDeliveryId,
                                          (ismEngine_UnreleasedHandle_t)(uintptr_t)(pUnrelChunk->Slot[i].UnrelDeliveryId),
                                          pContext);
            }
        }

        pUnrelChunk = pUnrelChunk->pNext;
    }

    iecs_unlockUnreleasedDeliveryState(pClient);

    return rc;
}


/*
 * Add an Unreleased Message State to the Store and/or transaction
 *
 * Non-transactional: Create UMS
 * Transactional:     Create UMS and TOR
 * Commit:            No-op
 * Rollback:          Delete UMS
 */
int32_t iecs_storeUnreleasedMessageState(ieutThreadData_t *pThreadData,
                                         ismEngine_ClientState_t *pClient,
                                         ismEngine_Transaction_t *pTran,
                                         uint32_t unrelDeliveryId,
                                         ismEngine_UnreleasedState_t *pUnrelChunk,
                                         int16_t slotNumber,
                                         ismStore_Handle_t *pHStoreUnrel)
{
    ismStore_StateObject_t storeStateObj;
    ismStore_Handle_t hStoreUnrel = ismSTORE_NULL_HANDLE;
    ietrSLE_Header_t *pSLE = NULL;
    bool fSLEAdded = false;
    int32_t rc = OK;

    // Make sure that we have a state context for the operation - this is created lazily
    if (pClient->Durability == iecsDurable)
    {
        assert(pClient->hStoreCSR != ismSTORE_NULL_HANDLE);

        if (pClient->hUnreleasedStateContext == NULL)
        {
            rc = ism_store_openStateContext(pClient->hStoreCSR,
                                            &pClient->hUnreleasedStateContext);
            if (rc != OK)
            {
                goto mod_exit;
            }
        }
    }

    // Create the Unreleased Message State, and an associated Transaction Operation Reference
    // if this is a transactional update, as part of the same store-transaction
    storeStateObj.Value = unrelDeliveryId;

    // We might need to retry our store-transaction if the current generation is full
    while (true)
    {
        if (pClient->Durability == iecsDurable)
        {
            rc = ism_store_createState(pThreadData->hStream,
                                       pClient->hUnreleasedStateContext,
                                       &storeStateObj,
                                       &hStoreUnrel);
        }

        if (rc == OK)
        {
            // If this is transactional, we need to create a Transaction Operation Reference in the same
            // Store transaction
            if (pTran)
            {
                iestSLEAddUMS_t SLEData;

                if (pClient->Durability == iecsDurable)
                {
                    rc = ietr_createTranRef(pThreadData,
                                            pTran,
                                            hStoreUnrel,
                                            iestTOR_VALUE_ADD_UMS,
                                            iestTOR_STATE_NONE,
                                            &SLEData.TranRef);
                }

                if (rc == OK)
                {
                    // We need a soft-log entry to make it part of the transaction
                    SLEData.pClient = pClient;
                    SLEData.pUnrelChunk = pUnrelChunk;
                    SLEData.SlotNumber = slotNumber;
                    SLEData.hStoreUMS = hStoreUnrel;
                    rc = ietr_softLogAdd(pThreadData,
                                         pTran,
                                         ietrSLE_CS_ADDUNRELMSG,
                                         iecs_SLEReplayAddUnrelMsg,
                                         NULL,
                                         Commit | Rollback,
                                         &SLEData,
                                         sizeof(SLEData),
                                         0, 1,
                                         &pSLE);
                    if (rc == OK)
                    {
                        fSLEAdded = true;
                    }
                }
            }
        }

        if ((pClient->Durability == iecsDurable) &&
            ((pTran == NULL) || (!(pTran->fAsStoreTran) && !(pTran->fStoreTranPublish))))
        {
            if (rc == OK)
            {
                iest_store_commit(pThreadData, false);
                break;
            }
            else
            {
                if (rc == ISMRC_StoreGenerationFull)
                {
                    iest_store_rollback(pThreadData, false);
                    // Round we go again
                }
                else
                {
                    iest_store_rollback(pThreadData, false);
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }

mod_exit:
    if (rc == OK)
    {
        *pHStoreUnrel = hStoreUnrel;
    }
    else
    {
        if (fSLEAdded)
        {
            ietr_softLogRemove(pThreadData, pTran, pSLE);
        }
    }

    return rc;
}


/*
 * Remove an Unreleased Message State from the Store and/or transaction
 *
 * Non-transactional: Delete UMS
 * Transactional:     Create TOR
 * Commit:            Delete UMS
 * Rollback:          No-op
 */
int32_t iecs_unstoreUnreleasedMessageState(ieutThreadData_t *pThreadData,
                                           ismEngine_ClientState_t *pClient,
                                           ismEngine_Transaction_t *pTran,
                                           ismEngine_UnreleasedState_t *pUnrelChunk,
                                           int16_t slotNumber,
                                           ismStore_Handle_t hStoreUnrel,
                                           ismEngine_AsyncData_t *pAsyncData)
{
    ietrSLE_Header_t *pSLE = NULL;
    bool fSLEAdded = false;
    int32_t rc = OK;

    // Make sure that we have a state context for the operation - this is created lazily
    if (pClient->Durability == iecsDurable)
    {
        if (pClient->hUnreleasedStateContext == NULL)
        {
            assert(pClient->hStoreCSR != ismSTORE_NULL_HANDLE);
            rc = ism_store_openStateContext(pClient->hStoreCSR,
                                            &pClient->hUnreleasedStateContext);
            if (rc != OK)
            {
                goto mod_exit;
            }
        }
    }

    // We might need to retry our store-transaction is the current generation is full
    while (true)
    {
        // If this is transactional, we need to create a Transaction Operation Reference
        // and actually delete the UMS during commit
        if (pTran)
        {
            iestSLERemoveUMS_t SLEData;

            if (pClient->Durability == iecsDurable)
            {
                rc = ietr_createTranRef(pThreadData,
                                        pTran,
                                        hStoreUnrel,
                                        iestTOR_VALUE_REMOVE_UMS,
                                        iestTOR_STATE_NONE,
                                        &SLEData.TranRef);
            }

            if (rc == OK)
            {
                // We need a soft-log entry to make it part of the transaction
                SLEData.pClient = pClient;
                SLEData.pUnrelChunk = pUnrelChunk;
                SLEData.SlotNumber = slotNumber;
                SLEData.hStoreUMS = hStoreUnrel;
                rc = ietr_softLogAdd(pThreadData,
                                     pTran,
                                     ietrSLE_CS_RMVUNRELMSG,
                                     iecs_SLEReplayRmvUnrelMsg,
                                     NULL,
                                     Commit | Rollback,
                                     &SLEData,
                                     sizeof(SLEData),
                                     1, 0,
                                     &pSLE);
                if (rc == OK)
                {
                    fSLEAdded = true;
                }
            }
        }
        else
        {
            // Outside a transaction, we delete the UMS immediately.
            assert(hStoreUnrel != ismSTORE_NULL_HANDLE);
            rc = ism_store_deleteState(pThreadData->hStream,
                                       pClient->hUnreleasedStateContext,
                                       hStoreUnrel);
        }

        if (pClient->Durability == iecsDurable)
        {
            if (rc == OK)
            {
                rc = iead_store_asyncCommit(pThreadData, false, pAsyncData);
                break;
            }
            else
            {
                if (rc == ISMRC_StoreGenerationFull)
                {
                    iest_store_rollback(pThreadData, false);
                    // Round we go again
                }
                else
                {
                    iest_store_rollback(pThreadData, false);
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }

mod_exit:
    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        if (fSLEAdded)
        {
            ietr_softLogRemove(pThreadData, pTran, pSLE);
        }
    }

    return rc;
}


/*
 * Rehydrate an Unreleased Message State from the Store during recovery
 *
 * Non-transactional: Just remember the unreleased message state
 * Transactional:     Make an unreleased message state and an SLE
 */
int32_t iecs_rehydrateUnreleasedMessageState(ieutThreadData_t *pThreadData,
                                             ismEngine_ClientState_t *pClient,
                                             ismEngine_Transaction_t *pTran,
                                             ietrTranEntryType_t type,
                                             uint32_t unrelDeliveryId,
                                             ismStore_Handle_t hStoreUnrel,
                                             ismEngine_RestartTransactionData_t *pTranData)
{
    ismEngine_UnreleasedState_t *pUnrelChunk = NULL;
    int16_t slotNumber;
    bool fSlotFound = false;
    int32_t rc = OK;

    assert(pClient->Durability == iecsDurable);
    assert(pClient->hStoreCSR != ismSTORE_NULL_HANDLE);

    if (pClient->hUnreleasedStateContext == NULL)
    {
        rc = ism_store_openStateContext(pClient->hStoreCSR,
                                        &pClient->hUnreleasedStateContext);
        if (rc != OK)
        {
            goto mod_exit;
        }
    }

    // Check the existing chunks for the supplied delivery ID. If we
    // already have this ID, just carry on
    pUnrelChunk = pClient->pUnreleasedHead;
    while (pUnrelChunk != NULL)
    {
        for (slotNumber = 0; slotNumber < pUnrelChunk->Limit; slotNumber++)
        {
            if (pUnrelChunk->Slot[slotNumber].fSlotInUse)
            {
                if (pUnrelChunk->Slot[slotNumber].UnrelDeliveryId == unrelDeliveryId)
                {
                    fSlotFound = true;
                    break;
                }
            }
        }

        if (fSlotFound)
        {
            break;
        }

        pUnrelChunk = pUnrelChunk->pNext;
    }

    // If we haven't found the delivery ID already, we need to create a new one
    if (!fSlotFound)
    {
        // Check the existing chunks for a gap
        pUnrelChunk = pClient->pUnreleasedHead;
        while (pUnrelChunk != NULL)
        {
            for (slotNumber = 0; slotNumber < pUnrelChunk->Capacity; slotNumber++)
            {
                if (!pUnrelChunk->Slot[slotNumber].fSlotInUse)
                {
                    pUnrelChunk->Slot[slotNumber].fSlotInUse = true;
                    pUnrelChunk->Slot[slotNumber].fUncommitted = (pTran != NULL) ? true : false;
                    pUnrelChunk->Slot[slotNumber].UnrelDeliveryId = unrelDeliveryId;
                    pUnrelChunk->Slot[slotNumber].hStoreUnrelStateObject = hStoreUnrel;

                    if (slotNumber >= pUnrelChunk->Limit)
                    {
                        pUnrelChunk->Limit = slotNumber + 1;
                    }
                    pUnrelChunk->InUse++;

                    fSlotFound = true;
                    break;
                }
            }

            if (fSlotFound)
            {
                break;
            }

            pUnrelChunk = pUnrelChunk->pNext;
        }

        // New slots are added at the head
        if (!fSlotFound)
        {
            iereResourceSetHandle_t resourceSet = pClient->resourceSet;
            iere_primeThreadCache(pThreadData, resourceSet);
            pUnrelChunk = iere_calloc(pThreadData,
                                      resourceSet,
                                      IEMEM_PROBE(iemem_clientState, 12), 1, sizeof(ismEngine_UnreleasedState_t));
            if (pUnrelChunk != NULL)
            {
                memcpy(pUnrelChunk->StrucId, ismENGINE_UNRELEASED_STATE_STRUCID, 4);
                pUnrelChunk->InUse = 1;
                pUnrelChunk->Limit = 1;
                pUnrelChunk->Capacity = ismENGINE_UNRELEASED_CHUNK_SIZE;
                pUnrelChunk->Slot[0].fSlotInUse = true;
                pUnrelChunk->Slot[0].fUncommitted = (pTran != NULL) ? true : false;
                pUnrelChunk->Slot[0].UnrelDeliveryId = unrelDeliveryId;
                pUnrelChunk->Slot[0].hStoreUnrelStateObject = hStoreUnrel;
                pUnrelChunk->pNext = pClient->pUnreleasedHead;
                pClient->pUnreleasedHead = pUnrelChunk;
                slotNumber = 0;
            }
            else
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }
    }

    if ((rc == OK) && (pTran != NULL))
    {
        switch (type)
        {
            case iestTOR_VALUE_ADD_UMS:
                {
                    // We need a soft-log entry to make it part of the transaction
                    iestSLEAddUMS_t SLEData;

                    SLEData.pClient = pClient;
                    SLEData.pUnrelChunk = pUnrelChunk;
                    SLEData.SlotNumber = slotNumber;
                    SLEData.hStoreUMS = hStoreUnrel;

                    // Ensure we have details of the link to the transaction
                    SLEData.TranRef.hTranRef = pTranData->operationRefHandle;
                    SLEData.TranRef.orderId = pTranData->operationReference.OrderId;

                    rc = ietr_softLogRehydrate(pThreadData,
                                               pTranData,
                                               ietrSLE_CS_ADDUNRELMSG,
                                               iecs_SLEReplayAddUnrelMsg,
                                               NULL,
                                               Commit | Rollback,
                                               &SLEData,
                                               sizeof(SLEData),
                                               0, 1,
                                               NULL);
                }
                break;
            case iestTOR_VALUE_REMOVE_UMS:
                {
                    // We need a soft-log entry to make it part of the transaction
                    iestSLERemoveUMS_t SLEData;

                    SLEData.pClient = pClient;
                    SLEData.pUnrelChunk = pUnrelChunk;
                    SLEData.SlotNumber = slotNumber;
                    SLEData.hStoreUMS = hStoreUnrel;

                    // Ensure we have details of the link to the transaction
                    SLEData.TranRef.hTranRef = pTranData->operationRefHandle;
                    SLEData.TranRef.orderId = pTranData->operationReference.OrderId;
                    rc = ietr_softLogRehydrate(pThreadData,
                                               pTranData,
                                               ietrSLE_CS_RMVUNRELMSG,
                                               iecs_SLEReplayRmvUnrelMsg,
                                               NULL,
                                               Commit | Rollback,
                                               &SLEData,
                                               sizeof(SLEData),
                                               1, 0,
                                               NULL);
                }
                break;
            default:
                assert(false);
                break;
        }
    }

mod_exit:
    if (rc != OK)
    {
        if (pUnrelChunk != NULL)
        {
            pUnrelChunk->Slot[slotNumber].fSlotInUse = false;
            pUnrelChunk->Slot[slotNumber].fUncommitted = false;
            pUnrelChunk->Slot[slotNumber].UnrelDeliveryId = 0;
            pUnrelChunk->Slot[slotNumber].hStoreUnrelStateObject = ismSTORE_NULL_HANDLE;
            pUnrelChunk->InUse--;
        }
    }

    return rc;
}


/*
 * Replays a soft-log entry - this is the commit/rollback for a single operation.
 * This runs within the auspices of a store-transaction, but it must not perform
 * operations which can run out of space, nor must it commit or roll back the
 * store-transaction.
 */
void iecs_SLEReplayAddUnrelMsg(ietrReplayPhase_t phase,
                               ieutThreadData_t *pThreadData,
                               ismEngine_Transaction_t *pTran,
                               void *pEntry,
                               ietrReplayRecord_t *pRecord,
                               ismEngine_DelivererContext_t *delivererContext)
{
    iestSLEAddUMS_t *pSLEAdd = (iestSLEAddUMS_t *)pEntry;
    ismEngine_ClientState_t *pClient = pSLEAdd->pClient;
    ismEngine_UnreleasedState_t *pUnrelChunk = pSLEAdd->pUnrelChunk;
    int16_t slot = pSLEAdd->SlotNumber;

    assert(phase == Commit || phase == Rollback);

    switch (phase)
    {
        case Commit:
            {
                iecs_lockUnreleasedDeliveryState(pClient);

                pUnrelChunk->Slot[slot].fUncommitted = false;

                iecs_unlockUnreleasedDeliveryState(pClient);
            }
            break;
        case Rollback:
            {
                if (pClient->Durability == iecsDurable && !pTran->fAsStoreTran)
                {
                    uint32_t rc;
                    rc = ism_store_deleteState(pThreadData->hStream,
                                               pClient->hUnreleasedStateContext,
                                               pSLEAdd->hStoreUMS);

                    if (rc != OK)
                    {
                        ieutTRACE_FFDC(ieutPROBE_008, false,
                                       "ism_store_deleteState failed", rc,
                                       "Owner handle", &pClient->hStoreCSR, sizeof(pClient->hStoreCSR),
                                       "State handle", &pSLEAdd->hStoreUMS, sizeof(pSLEAdd->hStoreUMS),
                                       NULL);
                    }

                    pRecord->StoreOpCount++;
                }

                iecs_lockUnreleasedDeliveryState(pClient);

                pUnrelChunk->Slot[slot].UnrelDeliveryId = 0;
                pUnrelChunk->Slot[slot].hStoreUnrelStateObject = ismSTORE_NULL_HANDLE;
                pUnrelChunk->Slot[slot].fUncommitted = false;
                pUnrelChunk->Slot[slot].fSlotInUse = false;
                pUnrelChunk->InUse--;

                iecs_unlockUnreleasedDeliveryState(pClient);
            }
            break;
        default:
            break;
    }
}

/*
 * Replays a soft-log entry - this is the commit/rollback for a single operation.
 * This runs within the auspices of a store-transaction, but it must not perform
 * operations which can run out of space, nor must it commit or roll back the
 * store-transaction.
 */
void iecs_SLEReplayRmvUnrelMsg(ietrReplayPhase_t phase,
                               ieutThreadData_t *pThreadData,
                               ismEngine_Transaction_t *pTran,
                               void *pEntry,
                               ietrReplayRecord_t *pRecord,
                               ismEngine_DelivererContext_t * t)
{
    iestSLERemoveUMS_t *pSLERmv = (iestSLERemoveUMS_t *)pEntry;
    ismEngine_ClientState_t *pClient = pSLERmv->pClient;
    ismEngine_UnreleasedState_t *pUnrelChunk = pSLERmv->pUnrelChunk;
    int16_t slot = pSLERmv->SlotNumber;

    assert(phase == Commit || phase == Rollback);

    switch (phase)
    {
        case Commit:
            {
                if (pClient->Durability == iecsDurable)
                {
                    uint32_t rc;
                    rc = ism_store_deleteState(pThreadData->hStream,
                                               pClient->hUnreleasedStateContext,
                                               pSLERmv->hStoreUMS);

                    if (rc != OK)
                    {
                        ieutTRACE_FFDC(ieutPROBE_009, false,
                                       "ism_store_deleteState failed", rc,
                                       "Owner handle", &pClient->hStoreCSR, sizeof(pClient->hStoreCSR),
                                       "State handle", &pSLERmv->hStoreUMS, sizeof(pSLERmv->hStoreUMS),
                                       NULL);
                    }

                    pRecord->StoreOpCount++;
                }

                iecs_lockUnreleasedDeliveryState(pClient);

                pUnrelChunk->Slot[slot].UnrelDeliveryId = 0;
                pUnrelChunk->Slot[slot].hStoreUnrelStateObject = ismSTORE_NULL_HANDLE;
                pUnrelChunk->Slot[slot].fUncommitted = false;
                pUnrelChunk->Slot[slot].fSlotInUse = false;
                pUnrelChunk->InUse--;

                iecs_unlockUnreleasedDeliveryState(pClient);
            }
            break;
        case Rollback:
            {
                iecs_lockUnreleasedDeliveryState(pClient);

                pUnrelChunk->Slot[slot].fUncommitted = false;

                iecs_unlockUnreleasedDeliveryState(pClient);
            }
            break;
        default:
            break;
    }
}


/*
 * Add client-state to Store and commit it
 */
int32_t iecs_storeClientState(ieutThreadData_t *pThreadData,
                              ismEngine_ClientState_t *pClient,
                              bool fFromImport,
                              ismEngine_AsyncData_t *pAsyncData)
{
    ismStore_Record_t storeRecord;
    iestClientStateRecord_t clientStateRec;
    iestClientPropertiesRecord_t clientPropsRec;
    char *pFrags[2];
    uint32_t fragsLength[2];
    int32_t rc;

#ifndef NDEBUG
    uint32_t storeOpsCount = 0;
    int32_t storeRC = ism_store_getStreamOpsCount(pThreadData->hStream, &storeOpsCount);
    assert(storeRC == OK && storeOpsCount == 0);
#endif

    assert(pClient->hStoreCSR == ismSTORE_NULL_HANDLE);
    assert(pClient->hStoreCPR == ismSTORE_NULL_HANDLE);
    assert(pClient->pClientId != NULL);

    // Prepare the CPR which will also give us the variable part of the dataLength
    // to reserve
    iecs_prepareCPR(&clientPropsRec,
                    &storeRecord,
                    pClient,
                    NULL,                 // No will message
                    ismSTORE_NULL_HANDLE, // No will message
                    0,                    // No will message
                    0,                    // No will message
                    pFrags, fragsLength);

    // CSR has one variable length property
    size_t clientIdLength = strlen(pClient->pClientId)+1;

    // Reserve the space required, so we can expect the createRecords to work
    iest_store_reserve(pThreadData,
                       storeRecord.DataLength + sizeof(clientStateRec) + clientIdLength,
                       2, 0);

    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &pClient->hStoreCPR);

    if (rc != OK)
    {
        ieutTRACE_FFDC(ieutPROBE_001, true,
                       "Couldn't create record even with reservation", rc,
                       "Type", &storeRecord.Type, sizeof(storeRecord.Type),
                       "DataLength", &storeRecord.DataLength, sizeof(storeRecord.DataLength),
                       NULL);
    }

    assert(pClient->hStoreCPR != ismSTORE_NULL_HANDLE);

    // Prepare the CSR
    iecs_prepareCSR(&clientStateRec,
                    &storeRecord,
                    pClient,
                    pClient->hStoreCPR,
                    fFromImport,
                    pFrags, fragsLength);

    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &pClient->hStoreCSR);

    if (rc != OK)
    {
        ieutTRACE_FFDC(ieutPROBE_002, true,
                       "ism_store_createRecord failed despite reservation", rc,
                       "Type", &storeRecord.Type, sizeof(storeRecord.Type),
                       "DataLength", &storeRecord.DataLength, sizeof(storeRecord.DataLength),
                       NULL);
    }

    // Now commit the changes (which may return that it's gone async)
    rc = iead_store_asyncCommit(pThreadData, true, pAsyncData);

    return rc;
}

/*
 * Remove subscriptions associated with a client then release the client state
 */
bool iecs_cleanupRemainingResources(ieutThreadData_t *pThreadData,
                                    ismEngine_ClientState_t *pClient,
                                    iecsCleanupOptions_t cleanupOptions,
                                    bool fInline,
                                    bool fInlineThief)
{
    int32_t rc = OK;
    bool fDidRelease = false;
    uint32_t cleanupCount = 0;
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    // If not called to publish a will message, there ought not be a will message left, or it ought
    // to be set up for delayed publish...
    assert((cleanupOptions & iecsCleanup_PublishWillMsg) != 0 ||
           (pClient->hWillMessage == NULL || pClient->WillDelayExpiryTime != 0));

    // If this client-state has a will message to publish, publish it now
    if ((cleanupOptions & iecsCleanup_PublishWillMsg) != 0 && pClient->hWillMessage != NULL)
    {
        ismEngine_Message_t *pOriginalMsg = (ismEngine_Message_t *)pClient->hWillMessage;
        ismEngine_Message_t *pMessage = NULL;

        // Override the message's expiry based on the will message expiry time
        if (pClient->WillMessageTimeToLive == iecsWILLMSG_TIME_TO_LIVE_INFINITE)
        {
            pOriginalMsg->Header.Expiry = 0;
        }
        else
        {
            uint32_t newExpiry = ism_common_getExpire(((ism_time_t)pClient->WillMessageTimeToLive * 1000000000) + ism_common_currentTimeNanos());
            ieutTRACEL(pThreadData, newExpiry, ENGINE_NORMAL_TRACE,
                       "Overriding will message expiry from %u to %u\n", pOriginalMsg->Header.Expiry, newExpiry);
            pOriginalMsg->Header.Expiry = newExpiry;
        }

        // Make a copy of the will message setting the ServerTime to the current time
        rc = iem_createMessageCopy(pThreadData,
                                   pOriginalMsg,
                                   false,
                                   ism_engine_retainedServerTime(),
                                   pOriginalMsg->Header.Expiry,
                                   &pMessage);

        if (rc == OK)
        {
            assert(pMessage != NULL);

            ismEngine_Transaction_t *pTran = NULL;

            // For retained messages, we don't create a transaction because we know the publish we are
            // about to do will use one, which will be more efficient (and also works around for 171313).
            if (pMessage->Header.Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN)
            {
                // If the willmessage is not already marked as a null retained message but is...
                // to be retained and has a 0 length body, change the message type to MTYPE_NullRetained.
                // We can do this because the pMessage we are dealing with is a copy of the original one
                // that is not yet stored.
                if (    (pMessage->Header.MessageType != MTYPE_NullRetained)
                     && (pMessage->AreaCount == 2)
                     && (pMessage->AreaTypes[1] == ismMESSAGE_AREA_PAYLOAD)
                     && (pMessage->AreaLengths[1] == 0))
                {
                    assert(pMessage->StoreMsg.whole == 0);
                    pMessage->Header.MessageType = MTYPE_NullRetained;
                }
            }
            else
            {
                rc = ietr_createLocal(pThreadData,
                                      NULL,
                                      pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT,
                                      false,
                                      NULL,
                                      &pTran);
            }

            if (rc == OK)
            {
                rc = ieds_publish(pThreadData,
                                  pClient,
                                  pClient->pWillTopicName,
                                  iedsPUBLISH_OPTION_NONE,
                                  pTran,
                                  pMessage,
                                  0,
                                  NULL,
                                  0,
                                  NULL);

                if (pTran != NULL)
                {
                    if (rc == OK)
                    {
                        rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
                    }
                    else
                    {
                        assert(rc != ISMRC_SomeDestinationsFull);
                        (void)ietr_rollback(pThreadData, pTran, NULL, IETR_ROLLBACK_OPTIONS_NONE);
                    }
                }
            }

            iem_releaseMessage(pThreadData, pMessage);
        }

        if (rc != OK)
        {
            ism_common_log_context logContext = {0};
            logContext.clientId = pClient->pClientId;
            if (pClient->resourceSet != iereNO_RESOURCE_SET)
            {
                logContext.resourceSetId = pClient->resourceSet->stats.resourceSetIdentifier;
            }
            logContext.topicFilter = pClient->pWillTopicName;

            // The failure to put a will message is logged with an error message, but we carry on
            char messageBuffer[256];

            LOGCTX(&logContext, ERROR, Messaging, 3000, "%-s%s%d", "The server is unable to publish the Will message to topic {0}: Error={1} RC={2}.",
                   pClient->pWillTopicName,
                   ism_common_getErrorStringByLocale(rc, ism_common_getLocale(), messageBuffer, 255),
                   rc);

            rc = OK;
        }
        assert(pOriginalMsg == pClient->hWillMessage);

        iecs_unstoreWillMessage(pThreadData, pClient);
        iere_primeThreadCache(pThreadData, resourceSet);
        iecs_updateWillMsgStats(pThreadData, resourceSet, pOriginalMsg, -1);
        iere_free(pThreadData, resourceSet, iemem_clientState, pClient->pWillTopicName);
        iem_releaseMessage(pThreadData, pOriginalMsg);
        pClient->hWillMessage = NULL;
        pClient->pWillTopicName = NULL;
        pClient->WillMessageTimeToLive = iecsWILLMSG_TIME_TO_LIVE_INFINITE;
    }

    assert(pClient->pClientId != NULL);

    // Cannot specify including durable subs, without requesting cleanup of subs
    assert((cleanupOptions & iecsCleanup_Include_DurableSubs) == 0 ||
           (cleanupOptions & iecsCleanup_Subs) != 0);

    iecsCleanupSubContext_t *pContext = NULL;

    if ((cleanupOptions & iecsCleanup_Subs) != 0)
    {
        pContext = ism_common_malloc(ISM_MEM_PROBE(ism_memory_engine_misc,74),sizeof(iecsCleanupSubContext_t));
        assert(pContext != NULL);

        pContext->pClient = pClient;
        pContext->cleanupCount = 1;
        pContext->includeDurable = ((cleanupOptions & iecsCleanup_Include_DurableSubs) != 0);

        // Go through remaining subscriptions for this client
        rc = iett_listSubscriptions(pThreadData,
                                    pClient->pClientId,
                                    iettFLAG_LISTSUBS_NONE,
                                    NULL,
                                    pContext,
                                    cleanupSubscriptionFn);
        if (rc != OK)
        {
            ieutTRACE_FFDC(ieutPROBE_023, false,
                           "Cleaning up subscriptions failed", rc,
                           "pContext", pContext, sizeof(*pContext),
                           NULL);
        }

        cleanupCount = __sync_sub_and_fetch(&pContext->cleanupCount, 1);
    }

    if (cleanupCount == 0)
    {
        if (pContext != NULL) ism_common_free(ism_memory_engine_misc,pContext);

        fDidRelease = iecs_completeCleanupRemainingResources(pThreadData, pClient, fInline, fInlineThief);
    }

    return fDidRelease;
}

/*
 * Rehydrate a Client-State Record from the Store during recovery
 */
int32_t iecs_rehydrateClientStateRecord(ieutThreadData_t *pThreadData,
                                        ismStore_Record_t *pStoreRecord,
                                        ismStore_Handle_t hStoreCSR,
                                        ismEngine_ClientState_t **ppClient)
{
    iestClientStateRecord_t *pCSR;
    ismEngine_ClientState_t *pClient = NULL;
    uint32_t clientIdLength = 0;
    iecsNewClientStateInfo_t clientInfo = {0};
    int32_t rc = OK;

    assert(pStoreRecord->Type == ISM_STORE_RECTYPE_CLIENT);
    assert(pStoreRecord->FragsCount == 1);
    assert(pStoreRecord->pFragsLengths[0] >= sizeof(iestClientStateRecord_t));

    pCSR = (iestClientStateRecord_t *)(pStoreRecord->pFrags[0]);

    char *tmpPtr;

    if (LIKELY(pCSR->Version == iestCSR_CURRENT_VERSION))
    {
        clientInfo.durability = (pCSR->Flags & iestCSR_FLAGS_DURABLE) ? iecsDurable : iecsNonDurable;
        clientIdLength = pCSR->ClientIdLength;
        clientInfo.protocolId = pCSR->protocolId;
        tmpPtr = (char *)(pCSR + 1);
    }
    else
    {
        if (pCSR->Version == iestCSR_VERSION_1)
        {
            iestClientStateRecord_V1_t *pCSR_V1 = (iestClientStateRecord_V1_t *)pCSR;

            clientInfo.durability = (pCSR_V1->Flags & iestCSR_FLAGS_DURABLE) ? iecsDurable : iecsNonDurable;
            clientIdLength = pCSR_V1->ClientIdLength;
            // Only MQTT protocol clients would write a V1 client state to the store.
            clientInfo.protocolId = PROTOCOL_ID_MQTT;
            tmpPtr = (char *)(pCSR_V1 + 1);
        }
        else
        {
            rc = ISMRC_InvalidValue;
            ism_common_setErrorData(rc, "%u", pCSR->Version);
        }
    }

    if (rc == OK)
    {
        // Never expect to have an empty clientId (clientIdLength includes null terminator)
        assert(clientIdLength > 1);

        clientInfo.pClientId = tmpPtr;

        ieutTRACEL(pThreadData, pStoreRecord->State, ENGINE_NORMAL_TRACE, "Found Client state for client(%s) state(0x%lx)\n",
                   clientInfo.pClientId, pStoreRecord->State);

        tmpPtr += clientIdLength;

        clientInfo.resourceSet = iecs_getResourceSet(pThreadData,
                                                     clientInfo.pClientId,
                                                     clientInfo.protocolId,
                                                     iereOP_ADD);
        rc = iecs_newClientStateRecovery(pThreadData, &clientInfo, &pClient);
        if (rc == OK)
        {
            assert(pClient->pSecContext == NULL);

            pClient->hStoreCSR = hStoreCSR;

            if (pStoreRecord->State & iestCSR_STATE_DELETED)
            {
                // This client-state will be discarded at the end of recovery but we
                // add it to the client-state table during recovery so that any other
                // surviving resources (like subscriptions) can find it.
                pClient->fDiscardDurable = true;
                rc = iecs_addClientStateRecovery(pThreadData, pClient);
            }
            else if (pStoreRecord->State & iestCSR_STATE_DISCONNECTED)
            {
                // It's a zombie - it only exists as a memory of a durable client-state.
                // The client-state was disconnected at the time the server stopped.
                // The last-connected time is the upper 32 bits of the CSR's state.
                pClient->LastConnectedTime = ism_common_convertExpireToTime((uint32_t)(pStoreRecord->State >> 32));
                rc = iecs_addClientStateRecovery(pThreadData, pClient);
            }
            else
            {
                // It's a zombie - it only exists as a memory of a durable client-state.
                // The client-state was connected at the time the server stopped, so we need
                // to work out the last-connected time based on when the server last ran.
                pClient->LastConnectedTime = 0;     // Left as zero until we complete recovery
                rc = iecs_addClientStateRecovery(pThreadData, pClient);
            }
        }
    }

    if (rc == OK)
    {
        *ppClient = pClient;
    }
    else
    {
        if (pClient != NULL)
        {
            iecs_freeClientState(pThreadData, pClient, false);
        }

        ierr_recordBadStoreRecord( pThreadData
                                 , ISM_STORE_RECTYPE_CLIENT
                                 , hStoreCSR
                                 , (char *)clientInfo.pClientId
                                 , rc);
    }

    return rc;
}


/*
 * Rehydrate a Client Properties Record from the Store during recovery
 */
int32_t iecs_rehydrateClientPropertiesRecord(ieutThreadData_t *pThreadData,
                                             ismStore_Record_t *pStoreRecord,
                                             ismStore_Handle_t hStoreCPR,
                                             ismEngine_ClientState_t *pOwningClient,
                                             ismEngine_MessageHandle_t hWillMessage,
                                             ismEngine_ClientState_t **ppClient)
{
    iestClientPropertiesRecord_t *pCPR;
    uint32_t willTopicLength;
    uint32_t willMsgTimeToLive;
    uint32_t expiryInterval;
    uint32_t willDelay;
    char *pWillTopicCopy = NULL;
    uint32_t userIdLength = 0;
    char *pUserIdCopy = NULL;
    ismEngine_ClientState_t *pClient = pOwningClient;
    int32_t rc = OK;
    assert(pClient != NULL);
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    assert(pStoreRecord->Type == ISM_STORE_RECTYPE_CPROP);
    assert(pStoreRecord->FragsCount == 1);

    pClient->hStoreCPR = hStoreCPR;

    // The CPR is associated with an existing CSR, and this gives us access to the
    // client-state's will-message and associated userid
    pCPR = (iestClientPropertiesRecord_t *)(pStoreRecord->pFrags[0]);

    char *tmpPtr;

    if (LIKELY(pCPR->Version == iestCPR_CURRENT_VERSION))
    {
        assert(pStoreRecord->pFragsLengths[0] >= sizeof(iestClientPropertiesRecord_t));

        willTopicLength = pCPR->WillTopicNameLength;
        userIdLength = pCPR->UserIdLength;
        willMsgTimeToLive = pCPR->WillMsgTimeToLive;
        expiryInterval = pCPR->ExpiryInterval;
        willDelay = pCPR->WillDelay;
        tmpPtr = (char *)(pCPR + 1);
    }
    else
    {
        // Previous versions we understand...
        if (pCPR->Version == iestCPR_VERSION_4)
        {
            iestClientPropertiesRecord_V4_t *pCPR_V4 = (iestClientPropertiesRecord_V4_t *)pCPR;

            willTopicLength = pCPR_V4->WillTopicNameLength;
            userIdLength = pCPR_V4->UserIdLength;
            willMsgTimeToLive = pCPR_V4->WillMsgTimeToLive;
            if (willMsgTimeToLive == 0) // In this version, 0 meant infinite
            {
                willMsgTimeToLive = iecsWILLMSG_TIME_TO_LIVE_INFINITE;
            }
            expiryInterval = pCPR_V4->ExpiryInterval;
            willDelay = 0;
            tmpPtr = (char *)(pCPR_V4 + 1);
        }
        else if (pCPR->Version == iestCPR_VERSION_3)
        {
            iestClientPropertiesRecord_V3_t *pCPR_V3 = (iestClientPropertiesRecord_V3_t *)pCPR;
            willTopicLength = pCPR_V3->WillTopicNameLength;
            userIdLength = pCPR_V3->UserIdLength;
            willMsgTimeToLive = pCPR_V3->WillMsgTimeToLive;
            if (willMsgTimeToLive == 0) // In this version, 0 meant infinite
            {
                willMsgTimeToLive = iecsWILLMSG_TIME_TO_LIVE_INFINITE;
            }
            expiryInterval = iecsEXPIRY_INTERVAL_INFINITE;
            willDelay = 0;
            tmpPtr = (char *)(pCPR_V3 + 1);
        }
        else if (pCPR->Version == iestCPR_VERSION_2)
        {
            iestClientPropertiesRecord_V2_t *pCPR_V2 = (iestClientPropertiesRecord_V2_t *)pCPR;
            willTopicLength = pCPR_V2->WillTopicNameLength;
            userIdLength = pCPR_V2->UserIdLength;
            willMsgTimeToLive = iecsWILLMSG_TIME_TO_LIVE_INFINITE;
            expiryInterval = iecsEXPIRY_INTERVAL_INFINITE;
            willDelay = 0;
            tmpPtr = (char *)(pCPR_V2 + 1);
        }
        else if (pCPR->Version == iestCPR_VERSION_1)
        {
            iestClientPropertiesRecord_V1_t *pCPR_V1 = (iestClientPropertiesRecord_V1_t *)pCPR;
            willTopicLength = pCPR_V1->WillTopicNameLength;
            userIdLength = 0;
            willMsgTimeToLive = iecsWILLMSG_TIME_TO_LIVE_INFINITE;
            expiryInterval = iecsEXPIRY_INTERVAL_INFINITE;
            willDelay = 0;
            tmpPtr = (char *)(pCPR_V1 + 1);
        }
        else
        {
            rc = ISMRC_InvalidValue;
            ism_common_setErrorData(rc, "%u", pCPR->Version);
            goto mod_exit;
        }
    }

    // The logic to publish the will-message at the end of recovery can handle the case
    // where there's a CPR but no will-message and it tidies it up
    if (hWillMessage != NULL)
    {
        if (willTopicLength != 0)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            pWillTopicCopy = iere_malloc(pThreadData,
                                         resourceSet,
                                         IEMEM_PROBE(iemem_clientState, 13), willTopicLength);
            if (pWillTopicCopy != NULL)
            {
                memcpy(pWillTopicCopy, tmpPtr, willTopicLength);
            }
            else
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            pClient->pWillTopicName = pWillTopicCopy;
        }

        pClient->hWillMessage = hWillMessage;
        pClient->WillMessageTimeToLive = willMsgTimeToLive;
        pClient->WillDelay = willDelay;
        iere_primeThreadCache(pThreadData, resourceSet);
        iecs_updateWillMsgStats(pThreadData, resourceSet, pClient->hWillMessage, 1);
        rc = iest_rehydrateMessageRef(pThreadData, hWillMessage);
        if (rc == OK)
        {
            iem_recordMessageUsage(hWillMessage);
        }
    }
    tmpPtr += willTopicLength;

    if (userIdLength != 0)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        pUserIdCopy = iere_malloc(pThreadData,
                                  resourceSet,
                                  IEMEM_PROBE(iemem_clientState, 14), userIdLength);
        if (pUserIdCopy != NULL)
        {
            memcpy(pUserIdCopy, tmpPtr, userIdLength);
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }

        pClient->pUserId = pUserIdCopy;
    }
    else
    {
        pClient->pUserId = NULL;
    }
    tmpPtr += userIdLength;

    pClient->ExpiryInterval = expiryInterval;

    // Need to set the ExpiryTime (and WillDelayExpiryTime) for this client
    iecs_setLCTandExpiry(pThreadData, pClient, pClient->LastConnectedTime, NULL);

mod_exit:

    if (rc == OK)
    {
        *ppClient = pClient;
    }
    else
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_free(pThreadData, resourceSet, iemem_clientState, pWillTopicCopy);
        iere_free(pThreadData, resourceSet, iemem_clientState, pUserIdCopy);
    }

    return rc;
}


/*
 * Add or replace a will message
 */
int32_t iecs_setWillMessage(ieutThreadData_t *pThreadData,
                            ismEngine_ClientState_t *pClient,
                            const char *pWillTopicName,
                            ismEngine_Message_t *pMessage,
                            uint32_t timeToLive,
                            uint32_t willDelay,
                            void *pContext,
                            size_t contextLength,
                            ismEngine_CompletionCallback_t  pCallbackFn)
{
    char *pWillTopicCopy;
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    iere_primeThreadCache(pThreadData, resourceSet);
    pWillTopicCopy = iere_malloc(pThreadData,
                                 resourceSet,
                                 IEMEM_PROBE(iemem_clientState, 15), strlen(pWillTopicName) + 1);
    if (pWillTopicCopy != NULL)
    {
        strcpy(pWillTopicCopy, pWillTopicName);
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
        goto mod_exit;
    }

    // We can't store the will message under the client-state mutex because we might
    // have to wait for the Store to write a generation to disk. Consequently, we
    // permit only a single will-message operation at a time and mark the client-state
    // as in the middle of such an operation while we're doing the work in the Store
    iecs_lockClientState(pClient);
    if (pClient->fWillMessageUpdating)
    {
        rc = ISMRC_RequestInProgress;
        iecs_unlockClientState(pClient);
        goto mod_exit;
    }

    pClient->fWillMessageUpdating = true;

    pthread_spin_lock(&pClient->UseCountLock);
    pClient->UseCount += 1;
    pthread_spin_unlock(&pClient->UseCountLock);

    bool bStoreWillMessage;
    ismEngine_Message_t *pPrevWillMessage = (ismEngine_Message_t *)pClient->hWillMessage;

    if ((pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT) ||
        (pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_NONPERSISTENT &&
         (pPrevWillMessage != NULL &&
          pPrevWillMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)))
    {
        bStoreWillMessage = true;
    }
    else
    {
        bStoreWillMessage = false;
    }

    assert(rc == OK);

    if (bStoreWillMessage == true)
    {
        // If we're going to write something to the store, drop the client lock while we do so,
        // otherwise we just stay locked.
        iecs_unlockClientState(pClient);
        rc = iecs_storeWillMessage(pThreadData, pClient, pWillTopicCopy, pMessage, timeToLive, willDelay);
        iecs_lockClientState(pClient);
        pPrevWillMessage = (ismEngine_Message_t *)pClient->hWillMessage;
    }

    if (rc == OK)
    {
        iem_recordMessageUsage(pMessage);

        if (pClient->pWillTopicName != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_free(pThreadData, resourceSet, iemem_clientState, pClient->pWillTopicName);
        }

        pClient->pWillTopicName = pWillTopicCopy;
        pClient->hWillMessage = (ismEngine_MessageHandle_t)pMessage;
        pClient->WillMessageTimeToLive = timeToLive;
        pClient->WillDelay = willDelay;

        pClient->fWillMessageUpdating = false;
        iecs_unlockClientState(pClient);

        iere_primeThreadCache(pThreadData, resourceSet);
        iecs_updateWillMsgStats(pThreadData, resourceSet, pMessage, 1);

        // If the previous will message was stored by us, unstore it
        if (pPrevWillMessage != NULL)
        {
            if (pPrevWillMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
            {
                iest_unstoreMessageCommit(pThreadData, pPrevWillMessage, 0);
            }

            iecs_updateWillMsgStats(pThreadData, resourceSet, pPrevWillMessage, -1);
            iem_releaseMessage(pThreadData, pPrevWillMessage);
        }
    }
    else
    {
        pClient->fWillMessageUpdating = false;
        iecs_unlockClientState(pClient);
    }

    iecs_releaseClientStateReference(pThreadData, pClient, false, false);

    if (rc != OK)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_free(pThreadData, resourceSet, iemem_clientState, pWillTopicCopy);
    }

mod_exit:
    return rc;
}


/*
 * Remove a will message
 */
int32_t iecs_unsetWillMessage(ieutThreadData_t *pThreadData,
                              ismEngine_ClientState_t *pClient)
{
    int32_t rc = OK;
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    iecs_lockClientState(pClient);
    if (pClient->fWillMessageUpdating)
    {
        rc = ISMRC_RequestInProgress;
        iecs_unlockClientState(pClient);
        goto mod_exit;
    }

    pClient->fWillMessageUpdating = true;

    pthread_spin_lock(&pClient->UseCountLock);
    pClient->UseCount += 1;
    pthread_spin_unlock(&pClient->UseCountLock);

    bool bUnstoreWillMessage;
    ismEngine_Message_t *pWillMessage = (ismEngine_Message_t *)pClient->hWillMessage;

    // Did *we* store the message when it was set? If so, we need to unstore it.
    if (pWillMessage != NULL && pWillMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
    {
        bUnstoreWillMessage = true;
    }
    else
    {
        bUnstoreWillMessage = false;
    }

    assert(rc == OK);

    if (bUnstoreWillMessage == true)
    {
        // If we're going to write something to the store, drop the client lock while we do so,
        // otherwise we just stay locked.
        iecs_unlockClientState(pClient);
        rc = iecs_unstoreWillMessage(pThreadData, pClient);
        iecs_lockClientState(pClient);
        pWillMessage = (ismEngine_Message_t *)pClient->hWillMessage;
    }

    if (rc == OK)
    {
        if (pClient->pWillTopicName != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_free(pThreadData, resourceSet, iemem_clientState, pClient->pWillTopicName);
        }

        if (pWillMessage != NULL)
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iecs_updateWillMsgStats(pThreadData, resourceSet, pWillMessage, -1);
            iem_releaseMessage(pThreadData, pWillMessage);
        }

        pClient->pWillTopicName = NULL;
        pClient->hWillMessage = NULL;
        pClient->WillMessageTimeToLive = iecsWILLMSG_TIME_TO_LIVE_INFINITE;
        pClient->WillDelay = 0;

        pClient->fWillMessageUpdating = false;
        iecs_unlockClientState(pClient);
    }
    else
    {
        pClient->fWillMessageUpdating = false;
        iecs_unlockClientState(pClient);
    }

    iecs_releaseClientStateReference(pThreadData, pClient, false, false);

mod_exit:
    return rc;
}


/*
 * Add or replace a will message in the Store
 */
int32_t iecs_storeWillMessage(ieutThreadData_t *pThreadData,
                              ismEngine_ClientState_t *pClient,
                              char *pWillTopicName,
                              ismEngine_Message_t *pMessage,
                              uint32_t timeToLive,
                              uint32_t willDelay)
{
    ismStore_Handle_t hStoreMsg = ismSTORE_NULL_HANDLE;
    ismStore_Record_t storeRecordCSR;
    iestClientStateRecord_t clientStateRec;
    ismStore_Handle_t hStoreCSR;
    ismStore_Record_t storeRecordCPR;
    iestClientPropertiesRecord_t clientPropsRec;
    ismStore_Handle_t hStoreCPR;
    char *pFrags[3];
    uint32_t fragsLength[3];
    int32_t rc = OK;

    // First store the message - this may get orphaned if there's a failure
    // because it commits in its own store-transaction
    if (pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
    {
        rc = iest_storeMessage(pThreadData,
                               pMessage,
                               1,
                               iestStoreMessageOptions_NONE,
                               &hStoreMsg);
    }

    bool bDurableClient = (pClient->Durability == iecsDurable);

    // If the new will message is persistent or there is a CPR already in the Store,we need
    // to make some updates.
    if ((rc == OK) &&
        ((hStoreMsg != ismSTORE_NULL_HANDLE) || (pClient->hStoreCPR != ismSTORE_NULL_HANDLE)))
    {
        // The operations must be in the same store-transaction.
        while (true)
        {
            // Make a new CPR if the new will-message is persistent or the clientState is durable
            if (hStoreMsg != ismSTORE_NULL_HANDLE || bDurableClient == true)
            {
                // We use a CPR to record the information about the will-message.
                iecs_prepareCPR(&clientPropsRec,
                                &storeRecordCPR,
                                pClient,
                                pWillTopicName,
                                hStoreMsg,
                                timeToLive,
                                willDelay,
                                pFrags, fragsLength);

                rc = ism_store_createRecord(pThreadData->hStream,
                                            &storeRecordCPR,
                                            &hStoreCPR);
            }
            else
            {
                hStoreCPR = ismSTORE_NULL_HANDLE;
            }

            // If there's an existing CPR, delete it
            if ((rc == OK) && (pClient->hStoreCPR != ismSTORE_NULL_HANDLE))
            {
                rc = ism_store_deleteRecord(pThreadData->hStream,
                                            pClient->hStoreCPR);
            }

            // The CSR needs to point to the CPR, or NULL if there isn't a CPR now.
            // If it's a durable client-state, there's always a CSR, so we need to update it.
            if (rc == OK)
            {
                if (bDurableClient == true)
                {
                    assert(hStoreCPR != ismSTORE_NULL_HANDLE);

                    hStoreCSR = pClient->hStoreCSR;
                    rc = ism_store_updateRecord(pThreadData->hStream,
                                                pClient->hStoreCSR,
                                                hStoreCPR,
                                                iestCPR_STATE_NONE,
                                                ismSTORE_SET_ATTRIBUTE);
                }
                else
                {
                    // If it's a non-durable client-state, it may or may not already have a CSR.
                    if (pClient->hStoreCSR != ismSTORE_NULL_HANDLE)
                    {
                        if (hStoreCPR != ismSTORE_NULL_HANDLE)
                        {
                            // If there is already a CSR and we have a new CPR, update the CSR.
                            hStoreCSR = pClient->hStoreCSR;
                            rc = ism_store_updateRecord(pThreadData->hStream,
                                                        pClient->hStoreCSR,
                                                        hStoreCPR,
                                                        iestCPR_STATE_NONE,
                                                        ismSTORE_SET_ATTRIBUTE);
                        }
                        else
                        {
                            // If there's already a CSR and we do not have a new CPR, delete the CSR.
                            hStoreCSR = ismSTORE_NULL_HANDLE;
                            rc = ism_store_deleteRecord(pThreadData->hStream,
                                                        pClient->hStoreCSR);
                        }
                    }
                    else
                    {
                        if (hStoreCPR != ismSTORE_NULL_HANDLE)
                        {
                            uint32_t dataLength;
                            uint32_t fragsCount = 2; // CSR and ClientId

                            // If we do not have a CSR but we have a new CPR, we need to create
                            // a CSR just so that we can hang the CPR off it.
                            assert(pClient->Durability == iecsNonDurable);
                            assert(pClient->pClientId != NULL);

                            pFrags[0] = (char *)&clientStateRec;
                            fragsLength[0] = sizeof(clientStateRec);
                            dataLength = sizeof(clientStateRec);

                            pFrags[1] = pClient->pClientId;
                            fragsLength[1] = strlen(pClient->pClientId) + 1;
                            dataLength += fragsLength[1];

                            memcpy(clientStateRec.StrucId, iestCLIENT_STATE_RECORD_STRUCID, 4);
                            clientStateRec.Version = iestCSR_CURRENT_VERSION;
                            clientStateRec.Flags = iestCSR_FLAGS_NONE;
                            clientStateRec.ClientIdLength = fragsLength[1];
                            clientStateRec.protocolId = pClient->protocolId;

                            storeRecordCSR.Type = ISM_STORE_RECTYPE_CLIENT;
                            storeRecordCSR.FragsCount = fragsCount;
                            storeRecordCSR.pFrags = pFrags;
                            storeRecordCSR.pFragsLengths = fragsLength;
                            storeRecordCSR.DataLength = dataLength;
                            storeRecordCSR.Attribute = hStoreCPR;
                            storeRecordCSR.State = iestCSR_STATE_NONE;

                            rc = ism_store_createRecord(pThreadData->hStream,
                                                        &storeRecordCSR,
                                                        &hStoreCSR);
                        }
                        else
                        {
                            hStoreCSR = ismSTORE_NULL_HANDLE;
                        }
                    }
                }
            }

            if (rc == OK)
            {
                iest_store_commit(pThreadData, false);
                pClient->hStoreCSR = hStoreCSR;
                pClient->hStoreCPR = hStoreCPR;
                break;
            }
            else
            {
                if (rc == ISMRC_StoreGenerationFull)
                {
                    iest_store_rollback(pThreadData, false);
                    // Round we go again
                }
                else
                {
                    iest_store_rollback(pThreadData, false);
                    break;
                }
            }
        }
    }

    return rc;
}


/*
 * Remove a will message from the Store
 */
int32_t iecs_unstoreWillMessage(ieutThreadData_t *pThreadData,
                                ismEngine_ClientState_t *pClient)
{
    ismStore_Handle_t hStoreCSR = pClient->hStoreCSR;
    ismStore_Handle_t hStoreCPR = ismSTORE_NULL_HANDLE;
    int32_t rc = OK;

    // If the clientState has a CPR then we will need to do some work in the store
    // to unstore the will message.
    if (pClient->hStoreCPR != ismSTORE_NULL_HANDLE)
    {
        uint32_t storeOperations = 0;

        // Now we delete the CPR and update or delete the CSR.
        // These operations must be in the same store-transaction.
        while (true)
        {
            // Delete the CPR
            rc = ism_store_deleteRecord(pThreadData->hStream,
                                        pClient->hStoreCPR);

            if (rc == OK)
            {
                storeOperations++;

                // For a durable client, need to create a new CPR and update
                // the CSR to point to it
                if (pClient->Durability == iecsDurable || pClient->durableObjects != 0)
                {
                    ismStore_Record_t storeRecord;
                    iestClientPropertiesRecord_t clientPropsRec;
                    char *pFrags[3];
                    uint32_t fragsLength[3];

                    iecs_prepareCPR(&clientPropsRec,
                                    &storeRecord,
                                    pClient,
                                    NULL,                  // No will message
                                    ismSTORE_NULL_HANDLE,  // No will message
                                    0,                     // No will message
                                    0,                     // No will message
                                    pFrags, fragsLength);

                    rc = ism_store_createRecord(pThreadData->hStream,
                                                &storeRecord,
                                                &hStoreCPR);

                    if (rc == OK)
                    {
                        storeOperations++;

                        // Update the CSR with the new CPR
                        rc = ism_store_updateRecord(pThreadData->hStream,
                                                    pClient->hStoreCSR,
                                                    hStoreCPR,
                                                    iestCPR_STATE_NONE,
                                                    ismSTORE_SET_ATTRIBUTE);
                    }
                }
                else if (pClient->hStoreCSR != ismSTORE_NULL_HANDLE)
                {
                    // If the client-state is not durable, delete the CSR too.
                    hStoreCSR = ismSTORE_NULL_HANDLE;
                    rc = ism_store_deleteRecord(pThreadData->hStream,
                                                pClient->hStoreCSR);
                }
            }

            if (rc == OK)
            {
                storeOperations++;
                break;
            }
            else
            {
                storeOperations = 0;

                if (rc == ISMRC_StoreGenerationFull)
                {
                    iest_store_rollback(pThreadData, false);
                    // Round we go again
                }
                else
                {
                    iest_store_rollback(pThreadData, false);
                    break;
                }
            }
        }

        // Now unstore the message / commit the changes made to CPR & CSR
        if (rc == OK)
        {
            ismEngine_Message_t *pMessage = (ismEngine_Message_t *)pClient->hWillMessage;

            if (pMessage != NULL && pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
            {
                iest_unstoreMessageCommit(pThreadData, pMessage, storeOperations);
            }
            else
            {
                assert(storeOperations != 0);
                iest_store_commit(pThreadData, false);
            }

            pClient->hStoreCSR = hStoreCSR;
            pClient->hStoreCPR = hStoreCPR;
        }
    }

    return rc;
}


/*
 * Allocate a new message-delivery information structure
 */
int32_t iecs_newMessageDeliveryInfo(ieutThreadData_t *pThreadData,
                                    ismEngine_ClientState_t *pClient,
                                    iecsMessageDeliveryInfo_t **ppMsgDelInfo,
                                    bool rehydrating)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo;
    int32_t rc = OK;
    uint32_t mdrChunkSize = 0;
    uint32_t mdrChunkCount = 0;
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    iecs_getMDRChunkSizeAndCount(  pClient->maxInflightLimit
                                ,  &mdrChunkSize
                                ,  &mdrChunkCount);

    iere_primeThreadCache(pThreadData, resourceSet);
    pMsgDelInfo = iere_calloc(pThreadData,
                              resourceSet,
                              IEMEM_PROBE(iemem_clientState, 16), 1,
                              sizeof(iecsMessageDeliveryInfo_t)+ (mdrChunkCount*sizeof(iecsMessageDeliveryChunk_t *)));
    if (pMsgDelInfo != NULL)
    {
        int osrc;

        //Ensure OS gives us memory for the chunk array so we don't find when we come to use it, we can't get it
        iemem_touch(pMsgDelInfo->pChunk,  (mdrChunkCount*sizeof(iecsMessageDeliveryChunk_t *)));

        osrc = pthread_mutex_init(&pMsgDelInfo->Mutex, NULL);
        if (osrc == 0)
        {
            memcpy(pMsgDelInfo->StrucId, iecsMESSAGE_DELIVERY_INFO_STRUCID, 4);
            pMsgDelInfo->UseCount = 1;
            pMsgDelInfo->BaseDeliveryId = 1;
            pMsgDelInfo->NextDeliveryId = pMsgDelInfo->BaseDeliveryId;
            pMsgDelInfo->MaxDeliveryId = MQTT_MAX_DELIVERYID; // assumes MQTT for now

            pMsgDelInfo->NumDeliveryIds = 0;  //No ids assigned yet
            pMsgDelInfo->diagnosticOwner = pClient;
            pMsgDelInfo->resourceSet = resourceSet;
            pMsgDelInfo->fIdsExhausted = false;
            pMsgDelInfo->MdrChunkCount = mdrChunkCount;
            pMsgDelInfo->MdrChunkSize  = mdrChunkSize;

            iecs_setupInflightLimitBasedParams( pThreadData
                                              , pMsgDelInfo
                                              , pClient->maxInflightLimit);

            pClient->hMsgDeliveryInfo = pMsgDelInfo;

            if (pClient->Durability == iecsDurable)
            {
                assert(pClient->hStoreCSR != ismSTORE_NULL_HANDLE);

                pMsgDelInfo->hStoreCSR = pClient->hStoreCSR;

                // During recovery we delay opening the reference context, it happens
                // during completion
                if (!rehydrating)
                {
                    ismStore_ReferenceStatistics_t stats = {0};

                    rc = ism_store_openReferenceContext(pClient->hStoreCSR,
                                                        &stats,
                                                        &pMsgDelInfo->hMsgDeliveryRefContext);
                    if (rc == OK)
                    {
                        ieutTRACEL(pThreadData, stats.HighestOrderId, ENGINE_HIFREQ_FNC_TRACE,
                                   "Highest order id %lu\n", stats.HighestOrderId);

                        pMsgDelInfo->NextOrderId = stats.HighestOrderId + 1;
                    }
                    else
                    {
                        pthread_mutex_destroy(&pMsgDelInfo->Mutex);
                        iere_primeThreadCache(pThreadData, resourceSet);
                        iere_freeStruct(pThreadData,
                                        resourceSet,
                                        iemem_clientState, pMsgDelInfo, pMsgDelInfo->StrucId);
                        ism_common_setError(rc);
                    }
                }
            }
        }
        else
        {
            iere_primeThreadCache(pThreadData, resourceSet);
            iere_freeStruct(pThreadData,
                            resourceSet,
                            iemem_clientState, pMsgDelInfo, pMsgDelInfo->StrucId);
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
        }
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    if (rc == OK)
    {
        *ppMsgDelInfo = pMsgDelInfo;
    }

    return rc;
}


/*
 * De-allocate a message-delivery information structure
 */
void iecs_freeMessageDeliveryInfo(ieutThreadData_t *pThreadData,
                                  iecsMessageDeliveryInfo_t *pMsgDelInfo)
{
    if (pMsgDelInfo != NULL)
    {
        iereResourceSetHandle_t resourceSet = pMsgDelInfo->resourceSet;

        // We expect the reference context to have been closed already, so this
        // is just being defensive to ensure we don't leak them in some odd circumstance
        if (pMsgDelInfo->hMsgDeliveryRefContext != ismSTORE_NULL_HANDLE)
        {
            (void)ism_store_closeReferenceContext(pMsgDelInfo->hMsgDeliveryRefContext);
        }

        uint32_t mdrChunkCount = pMsgDelInfo->MdrChunkCount;

        iere_primeThreadCache(pThreadData, resourceSet);
        for (uint32_t i = 0; (pMsgDelInfo->ChunksInUse > 0) && (i < mdrChunkCount); i++)
        {
            if (pMsgDelInfo->pChunk[i] != NULL)
            {
                assert((pMsgDelInfo->pChunk[i] != pMsgDelInfo->pFreeChunk1) &&
                       (pMsgDelInfo->pChunk[i] != pMsgDelInfo->pFreeChunk2));

                iere_free(pThreadData, resourceSet, iemem_clientState, pMsgDelInfo->pChunk[i]);
                pMsgDelInfo->ChunksInUse--;
            }
        }

        if (pMsgDelInfo->pFreeChunk1 != NULL)
        {
            iere_free(pThreadData, resourceSet, iemem_clientState, pMsgDelInfo->pFreeChunk1);
        }

        if (pMsgDelInfo->pFreeChunk2 != NULL)
        {
            iere_free(pThreadData, resourceSet, iemem_clientState, pMsgDelInfo->pFreeChunk2);
        }

        pthread_mutex_destroy(&pMsgDelInfo->Mutex);

        iere_freeStruct(pThreadData, resourceSet, iemem_clientState, pMsgDelInfo, pMsgDelInfo->StrucId);
    }
}


/*
 * Acquire a reference to the message-delivery information for a client-state
 */
int32_t iecs_acquireMessageDeliveryInfoReference(ieutThreadData_t *pThreadData,
                                                 ismEngine_ClientState_t *pClient,
                                                 iecsMessageDeliveryInfoHandle_t *phMsgDelInfo)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = NULL;
    bool fLocked = false;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pClient,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "(pClient %p)\n", __func__, pClient);

    if (pClient != NULL)
    {
        iecs_lockClientState(pClient);
        fLocked = true;

        pMsgDelInfo = pClient->hMsgDeliveryInfo;
        if (pMsgDelInfo == NULL)
        {
            rc = iecs_newMessageDeliveryInfo(pThreadData, pClient, &pMsgDelInfo, false);
            if (rc == OK)
            {
                // One reference for the client-state and this one for the caller
                __sync_fetch_and_add(&pMsgDelInfo->UseCount, 1);

                *phMsgDelInfo = pMsgDelInfo;
            }
        }
        else
        {
            __sync_fetch_and_add(&pMsgDelInfo->UseCount, 1);

            *phMsgDelInfo = pMsgDelInfo;
        }
    }
    else
    {
        // In the case where no pClient was provided, we know the address of the message-delivery
        // info but we need to register our interest - this happens during restart
        pMsgDelInfo = *phMsgDelInfo;

        __sync_fetch_and_add(&pMsgDelInfo->UseCount, 1);
    }

    if (fLocked)
    {
        iecs_unlockClientState(pClient);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


/*
 * Release a reference to the message-delivery information for a client-state
 */
void iecs_releaseMessageDeliveryInfoReference(ieutThreadData_t *pThreadData,
                                              iecsMessageDeliveryInfoHandle_t hMsgDelInfo)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = hMsgDelInfo;

    uint32_t usecount = __sync_sub_and_fetch(&pMsgDelInfo->UseCount, 1);
    if (usecount == 0)
    {
        ieutTRACEL(pThreadData, pMsgDelInfo, ENGINE_HIGH_TRACE,
                   "Releasing last reference to message-delivery info %p\n", pMsgDelInfo);
        iecs_freeMessageDeliveryInfo(pThreadData, pMsgDelInfo);
    }
}

/*
 * Identify which of an array of ClientIds has the Message Delivery Reference
 * represented by the hQueue and hNode pair in its list.
 *
 * This assumes the the suspected clientIds are contained in an array with a NULL
 * pointer acting as a sentinel at the end.
 */
typedef struct tag_iecsIdentifyMDROwnerCallbackContext_t
{
    ismQHandle_t hQueue;
    void *pNode;
} iecsIdentifyMDROwnerCallbackContext_t;

static int32_t iecs_identifyMDROwnerCallback(ieutThreadData_t *pThreadData,
                                             ismQHandle_t hQueue,
                                             void *pNode,
                                             void *pContext)
{
    iecsIdentifyMDROwnerCallbackContext_t *context = (iecsIdentifyMDROwnerCallbackContext_t *)pContext;

    // If this entry matches the one we're looking for, return ISMRC_ExistingKey which causes
    // the iterator to end and pass this return code to the caller. The caller interprets this
    // return code as a positive result (i.e. this is the owner of the MDR).
    return (context->pNode == pNode && context->hQueue == hQueue) ? ISMRC_ExistingKey : OK;
}

int32_t iecs_identifyMessageDeliveryReferenceOwner(ieutThreadData_t *pThreadData,
                                                   const char **pSuspectClientIds,
                                                   ismQHandle_t hQueue,
                                                   void *hNode,
                                                   const char **pIdentifiedClientId)
{
    int32_t rc = ISMRC_NotFound;
    const char **pThisClientId = pSuspectClientIds;
    const char *thisSuspect = NULL;

    ieutTRACEL(pThreadData, hQueue,  ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "(pSuspectedClientIds=%p, hQueue=%p, hNode=%p)\n",
               __func__, pSuspectClientIds, hQueue, hNode);

    iecsIdentifyMDROwnerCallbackContext_t identifyContext = {hQueue, hNode};

    while((thisSuspect = *pThisClientId) != NULL)
    {
        iecsMessageDeliveryInfoHandle_t hMsgDelInfo = NULL;

        rc = iecs_findClientMsgDelInfo(pThreadData,
                                       thisSuspect,
                                       &hMsgDelInfo);

        // Iterate over the information checking for a match of the requested node and queue.
        if (rc == OK)
        {
            assert(hMsgDelInfo != NULL);

            rc = iecs_iterateMessageDeliveryInfo(pThreadData,
                                                 hMsgDelInfo,
                                                 iecs_identifyMDROwnerCallback,
                                                 &identifyContext);

            iecs_releaseMessageDeliveryInfoReference(pThreadData, hMsgDelInfo);

            // The callback function returns ISMRC_ExistingKey when it has identified
            // that this node & queue match one in this MsgDelInfo (it does this to
            // stop the search as soon as it has been found).
            if (rc == ISMRC_ExistingKey)
            {
                ieutTRACEL(pThreadData, hMsgDelInfo, ENGINE_INTERESTING_TRACE,
                           FUNCTION_IDENT "Identified '%s' as owner of MDR with hQueue=%p hNode=%p\n",
                           __func__, thisSuspect, hQueue, hNode);
                break;
            }
        }

        pThisClientId++;
    }

    if (thisSuspect != NULL)
    {
        assert(rc == ISMRC_ExistingKey);
        *pIdentifiedClientId = thisSuspect;
        rc = OK;
    }
    else
    {
        rc = ISMRC_NotFound;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

/*
 * Performs any cleanup required for the message delivery info structure.
 *
 * Specifically:
 *  - Mark the message-delivery information as unstored. The CSR which owns the
 *    MDR references has been marked as deleted in the Store and is about to be
 *    deleted. The remaining MDR references are of no further interest and do
 *    not need to be individually unstored.
 */
void iecs_cleanupMessageDeliveryInfo(ieutThreadData_t *pThreadData,
                                     iecsMessageDeliveryInfo_t *pMsgDelInfo)
{
    iecs_lockMessageDeliveryInfo(pMsgDelInfo);

    if (pMsgDelInfo->hMsgDeliveryRefContext != ismSTORE_NULL_HANDLE)
    {
        void *refctxt = pMsgDelInfo->hMsgDeliveryRefContext;
        pMsgDelInfo->hMsgDeliveryRefContext = ismSTORE_NULL_HANDLE;

        iecs_unlockMessageDeliveryInfo(pMsgDelInfo);

        (void)ism_store_closeReferenceContext(refctxt);
    }
    else
    {
        iecs_unlockMessageDeliveryInfo(pMsgDelInfo);
    }
}

/*
 * Relinquish any messages whose MDR specifies particular queues
 */
void iecs_relinquishAllMsgs(ieutThreadData_t *pThreadData,
                            iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                            ismQHandle_t *hQueues,
                            uint32_t queueCount,
                            iecsRelinquishType_t relinquishType)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = (iecsMessageDeliveryInfo_t *)hMsgDelInfo;

    ieutTRACEL(pThreadData, pMsgDelInfo, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "(hMsgDeliveryInfo %p, hQueue %p [%p], queueCount %u, relinquishType %d)\n", __func__,
               hMsgDelInfo, hQueues, queueCount > 0 ? hQueues[0] : NULL, queueCount, relinquishType);

    // No MDRs - nothing to do.
    if (pMsgDelInfo == NULL)
    {
        goto mod_exit;
    }

    //Convert the iecsRelinquishType_t into a ismEngine_RelinquishType_t
    ismEngine_RelinquishType_t engineRelinquishType;

    if (   (relinquishType == iecsRELINQUISH_ACK_HIGHRELIABILITY_ON_QUEUE)
         ||(relinquishType == iecsRELINQUISH_ACK_HIGHRELIABILITY_NOT_ON_QUEUE))
    {
        engineRelinquishType = ismEngine_RelinquishType_ACK_HIGHRELIABLITY;
    }
    else
    {
        assert (  (relinquishType == iecsRELINQUISH_NACK_ALL_ON_QUEUE)
                ||(relinquishType == iecsRELINQUISH_NACK_ALL_NOT_ON_QUEUE));
        engineRelinquishType = ismEngine_RelinquishType_NACK_ALL;
    }


    // Lock the message delivery info structure
    iecs_lockMessageDeliveryInfo(pMsgDelInfo);

    // Look for any MDRs that mention this queue
    for (uint32_t i = 0; i < pMsgDelInfo->MdrChunkCount; i++)
    {
        iecsMessageDeliveryChunk_t *pMsgDelChunk = pMsgDelInfo->pChunk[i];
        if ((pMsgDelChunk == NULL) || (pMsgDelChunk->slotsInUse == 0))
        {
            continue;
        }

        iecsMessageDeliveryReference_t *pMsgDelRef = pMsgDelChunk->Slot;
        for (uint32_t j = 0; j < pMsgDelInfo->MdrChunkSize; j++, pMsgDelRef++)
        {
            if (pMsgDelRef->fSlotInUse)
            {
                // Can only relinquish messages which have a queue and node handle set (i.e. are multiConsumer)
                if (pMsgDelRef->hNode == NULL || pMsgDelRef->hQueue == NULL) continue;

                if (pMsgDelRef->fSlotPending)
                {
                    // Interesting -- we're going to miss a relinquish because fSlotPending is set...
                    ieutTRACEL(pThreadData, pMsgDelRef, ENGINE_WORRYING_TRACE,
                               "Skipping relinquish for pMsgDelRef %p (hQueue=%p, hNode=%p, relinquishType=%d\n",
                               pMsgDelRef, pMsgDelRef->hQueue, pMsgDelRef->hNode, relinquishType);

                    continue;
                }

                // Decide whether we want to relinquish this message or not
                if (relinquishType != iecsRELINQUISH_ALL)
                {
                    bool foundQueue = false;

                    for(uint32_t k = 0; k < queueCount; k++)
                    {
                        if (pMsgDelRef->hQueue == hQueues[k])
                        {
                            foundQueue = true;
                            break;
                        }
                    }

                    if (   (relinquishType == iecsRELINQUISH_ACK_HIGHRELIABILITY_ON_QUEUE)
                        || (relinquishType == iecsRELINQUISH_NACK_ALL_ON_QUEUE))
                    {
                        if (foundQueue == false) continue;
                    }
                    else
                    {
                        assert(  (relinquishType == iecsRELINQUISH_ACK_HIGHRELIABILITY_NOT_ON_QUEUE)
                               ||(relinquishType == iecsRELINQUISH_NACK_ALL_NOT_ON_QUEUE));

                        if (foundQueue == true) continue;
                    }
                }

                // Actually perform the relinquish logic - this better be a multiConsumerQ
                assert(ieq_getQType(pMsgDelRef->hQueue) == multiConsumer);

                int32_t rc;
                uint32_t storeOpCount = 0;

                if (pMsgDelRef->hStoreMsgDeliveryRef1 != ismSTORE_NULL_HANDLE)
                {
                    assert(pMsgDelInfo->hMsgDeliveryRefContext != ismSTORE_NULL_HANDLE);
                    assert(pMsgDelRef->hStoreMsgDeliveryRef2 != ismSTORE_NULL_HANDLE);

                    rc = ism_store_deleteReference(pThreadData->hStream,
                                                   pMsgDelInfo->hMsgDeliveryRefContext,
                                                   pMsgDelRef->hStoreMsgDeliveryRef1,
                                                   pMsgDelRef->MsgDeliveryRef1OrderId,
                                                   0);

                    if (rc == OK)
                    {
                        storeOpCount++;
                    }
                    else
                    {
                        ieutTRACE_FFDC(ieutPROBE_001, true, "Deleting MDR ref 1", rc, NULL);
                    }

                    rc = ism_store_deleteReference(pThreadData->hStream,
                                                   pMsgDelInfo->hMsgDeliveryRefContext,
                                                   pMsgDelRef->hStoreMsgDeliveryRef2,
                                                   pMsgDelRef->MsgDeliveryRef2OrderId,
                                                   0);
                    if (rc == OK)
                    {
                        storeOpCount++;
                    }
                    else
                    {
                        ieutTRACE_FFDC(ieutPROBE_002, true, "Deleting MDR ref 2", rc, NULL);
                    }
                }

                // Call the queue to relinquish our hold on this message.
                rc = ieq_relinquish( pThreadData
                                   , pMsgDelRef->hQueue
                                   , pMsgDelRef->hNode
                                   , engineRelinquishType
                                   , &storeOpCount);

                // Anything more to do here?
                if (rc == OK)
                {
                    if (storeOpCount != 0) iest_store_commit(pThreadData, false);

                    pMsgDelRef->MsgDeliveryRefState = 0;
                    pMsgDelRef->hStoreMsgDeliveryRef1 = ismSTORE_NULL_HANDLE;
                    pMsgDelRef->MsgDeliveryRef1OrderId = 0;
                    pMsgDelRef->hStoreMsgDeliveryRef2 = ismSTORE_NULL_HANDLE;
                    pMsgDelRef->MsgDeliveryRef2OrderId = 0;
                    pMsgDelRef->hStoreRecord = ismSTORE_NULL_HANDLE;
                    pMsgDelRef->hQueue = NULL;
                    pMsgDelRef->hNode = NULL;
                    pMsgDelRef->hStoreMessage = ismSTORE_NULL_HANDLE;

                    rc = iecs_releaseDeliveryId_internal(pThreadData,
                                                         pMsgDelInfo,
                                                         pMsgDelRef->DeliveryId,
                                                         pMsgDelChunk,
                                                         pMsgDelRef);

                    // It's possible that we have now got some delivery ids back
                    if (rc == ISMRC_DeliveryIdAvailable)
                    {
                        //This should only be occurring when clients are disconnecting/disconnected
                        //so we don't need to restart delivery for them... (as zombie consumers will
                        //have prevented relinquishing inflight messages from individually deleted
                        //subs)
                        ieutTRACEL(pThreadData, i, ENGINE_NORMAL_TRACE, "Freed up deliveryids in iecs_relinquishAllMsgs\n");
                        rc = OK;
                    }

                    // Move on if we've tidied up.
                    if (pMsgDelInfo->pChunk[i] == NULL || pMsgDelChunk->slotsInUse == 0)
                    {
                        break;
                    }
                }
                else
                {
                    iest_store_rollback(pThreadData, false);
                }

                if (rc != OK)
                {
                    ieutTRACE_FFDC(ieutPROBE_003, true, "Failure relinquishing messages", rc, NULL);
                }
            }
        }
    }

    iecs_unlockMessageDeliveryInfo(pMsgDelInfo);

mod_exit:

    ieutTRACEL(pThreadData, pMsgDelInfo, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}


/*
 * Writes the state for a previously assigned MDR to the store
 * Note: Must have MessageDeliveryInfo locked!!!!
 */
static inline int32_t iecs_writeMessageDeliveryReference( ieutThreadData_t *pThreadData
                                                        , iecsMessageDeliveryInfo_t *pMsgDelInfo
                                                        , iecsMessageDeliveryReference_t *pMsgDelRef
                                                        , ismStore_Handle_t hStoreRecord
                                                        , ismStore_Handle_t hStoreMessage
                                                        , uint32_t deliveryId
                                                        , bool *pfLocked
                                                        , bool *pfStoredMDR)
{
    assert(*pfLocked == true);
    int32_t rc = OK;

    if (pMsgDelInfo->hMsgDeliveryRefContext != NULL)
    {
        ismStore_Handle_t hStoreMDR1 = ismSTORE_NULL_HANDLE;
        ismStore_Handle_t hStoreMDR2 = ismSTORE_NULL_HANDLE;
        uint64_t minimumActiveOrderId = 0;

        uint32_t MdrsBelowTargetDelta = 0;
        uint32_t MdrsAboveTargetDelta = 0;

        // If there are no MDRs using order ids below the target, we can update the MinimumActiveOrderId.
        if ((pMsgDelInfo->MdrsBelowTarget == 0) &&
            (pMsgDelInfo->NextOrderId > pMsgDelInfo->TargetMinimumActiveOrderId + iecsMDR_ORDER_ID_TARGET_GAP))
        {
            // We're going to give the minimum to the Store
            minimumActiveOrderId = pMsgDelInfo->TargetMinimumActiveOrderId;

            // Set the target to the next order id which is greater than the target by at least the gap amount
            pMsgDelInfo->TargetMinimumActiveOrderId = pMsgDelInfo->NextOrderId;

            ieutTRACEL(pThreadData, minimumActiveOrderId, ENGINE_HIFREQ_FNC_TRACE, "Setting minimumActiveOrderId %lu, target %lu\n",
                       minimumActiveOrderId, pMsgDelInfo->TargetMinimumActiveOrderId);

            // So, all existing MDRs above the target are now below the target
            pMsgDelInfo->MdrsBelowTarget = pMsgDelInfo->MdrsAboveTarget;
            pMsgDelInfo->MdrsAboveTarget = 0;
        }

        // The MDRs are allocated as a pair of references because a single reference doesn't have
        // enough flexibility to do what we need
        ismStore_Reference_t mdr1 = {0};
        ismStore_Reference_t mdr2 = {0};

        mdr1.OrderId = pMsgDelInfo->NextOrderId++;
        mdr1.hRefHandle = hStoreRecord;
        mdr1.Value = deliveryId;
        mdr1.State = iestMDR_STATE_OWNER_IS_SUBSC | iestMDR_STATE_HANDLE_IS_RECORD;

        mdr2.OrderId = pMsgDelInfo->NextOrderId++;
        mdr2.hRefHandle = hStoreMessage;
        mdr2.Value = deliveryId;
        mdr2.State = iestMDR_STATE_OWNER_IS_SUBSC | iestMDR_STATE_HANDLE_IS_MESSAGE;

        // Keep track of how many MDRs there are above or below the target order id
        if (mdr1.OrderId < pMsgDelInfo->TargetMinimumActiveOrderId)
        {
            MdrsBelowTargetDelta++;
        }
        else
        {
            MdrsAboveTargetDelta++;
        }

        if (mdr2.OrderId < pMsgDelInfo->TargetMinimumActiveOrderId)
        {
            MdrsBelowTargetDelta++;
        }
        else
        {
            MdrsAboveTargetDelta++;
        }

        pMsgDelInfo->MdrsBelowTarget += MdrsBelowTargetDelta;
        pMsgDelInfo->MdrsAboveTarget += MdrsAboveTargetDelta;

        // Now because we are going to start a store-transaction, we release the client-state
        // lock. We might be forced to wait for a new generation and this could take seconds.
        *pfLocked = false;
        iecs_unlockMessageDeliveryInfo(pMsgDelInfo);

        // We might need to retry our store-transaction if the current generation is full
        //iest_AssertStoreCommitAllowed(pThreadData);

        while (true)
        {
            rc = ism_store_createReference(pThreadData->hStream,
                                           pMsgDelInfo->hMsgDeliveryRefContext,
                                           &mdr1,
                                           minimumActiveOrderId,
                                           &hStoreMDR1);
            if (rc == OK)
            {
                rc = ism_store_createReference(pThreadData->hStream,
                                               pMsgDelInfo->hMsgDeliveryRefContext,
                                               &mdr2,
                                               0,
                                               &hStoreMDR2);
            }

            if (rc == OK)
            {
                iecs_lockMessageDeliveryInfo(pMsgDelInfo);
                *pfLocked = true;

                pMsgDelRef->MsgDeliveryRefState = mdr1.State | mdr2.State;
                pMsgDelRef->hStoreMsgDeliveryRef1 = hStoreMDR1;
                pMsgDelRef->MsgDeliveryRef1OrderId = mdr1.OrderId;
                pMsgDelRef->hStoreMsgDeliveryRef2 = hStoreMDR2;
                pMsgDelRef->MsgDeliveryRef2OrderId = mdr2.OrderId;
                pMsgDelRef->hStoreRecord = hStoreRecord;
                pMsgDelRef->hStoreMessage = hStoreMessage;
                pMsgDelRef->fSlotPending = false;
                *pfStoredMDR = true;
                break;
            }
            else
            {
                if (rc == ISMRC_StoreGenerationFull)
                {
                    iest_store_rollback(pThreadData, false);
                    // Round we go again
                }
                else
                {
                    iest_store_rollback(pThreadData, false);

                    // Undo the increments we made to our target counters
                    iecs_lockMessageDeliveryInfo(pMsgDelInfo);
                    *pfLocked = true;

                    pMsgDelInfo->MdrsBelowTarget -= MdrsBelowTargetDelta;
                    pMsgDelInfo->MdrsAboveTarget -= MdrsAboveTargetDelta;
                    break;
                }
            }
        }
    }
    else
    {
        // Client is not durable so we don't actually need an MDR in the store
        pMsgDelRef->hStoreMsgDeliveryRef1 = ismSTORE_NULL_HANDLE;
        pMsgDelRef->MsgDeliveryRef1OrderId = 0;
        pMsgDelRef->hStoreMsgDeliveryRef2 = ismSTORE_NULL_HANDLE;
        pMsgDelRef->MsgDeliveryRef2OrderId = 0;
        pMsgDelRef->hStoreRecord = ismSTORE_NULL_HANDLE;
        pMsgDelRef->hStoreMessage = ismSTORE_NULL_HANDLE;
        pMsgDelRef->fSlotPending = false;
        *pfStoredMDR = true;
    }

    return rc;
}


/*
 * Assigns a Message Delivery Reference and then writes it to the Store
 *
 * @remarks This function potentially call store_rollback but, if it's successful (i.e. rc == OK)
 * then it might have left some store operations in an uncommitted transaction and expects the caller to
 * commit them
 */
int32_t iecs_storeMessageDeliveryReference(ieutThreadData_t *pThreadData,
                                           iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                                           ismEngine_Session_t *pSession,
                                           ismStore_Handle_t hStoreRecord,
                                           ismQHandle_t hQueue,
                                           void *hNode,
                                           ismStore_Handle_t hStoreMessage,
                                           uint32_t *pDeliveryId,
                                           bool *pfStoredMDR)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = hMsgDelInfo;
    iecsMessageDeliveryChunk_t *pMsgDelChunk = NULL;
    iecsMessageDeliveryReference_t *pMsgDelRef = NULL;
    uint32_t deliveryId = 1;
    bool fStoredMDR = false;
    bool fLocked = false;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pMsgDelInfo, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "(hMsgDelInfo %p, deliveryId %u)\n", __func__, hMsgDelInfo, deliveryId);

    iecs_lockMessageDeliveryInfo(pMsgDelInfo);
    fLocked = true;

    // Only want to store the queue handle for shared queues
    if (hQueue == NULL || ieq_getQType(hQueue) != multiConsumer)
    {
        hQueue = NULL;
        hNode = NULL;
    }

    rc = iecs_assignDeliveryId_internal(pThreadData, pMsgDelInfo, pSession, hStoreRecord, hQueue, hNode, false, &deliveryId, &pMsgDelChunk, &pMsgDelRef);
    if (rc != OK)
    {
        goto mod_exit;
    }

    assert(deliveryId != 0);

    rc = iecs_writeMessageDeliveryReference( pThreadData
                                           , pMsgDelInfo
                                           , pMsgDelRef
                                           , hStoreRecord
                                           , hStoreMessage
                                           , deliveryId
                                           , &fLocked
                                           , &fStoredMDR);

    if (rc != OK)
    {
        goto mod_exit;
    }

mod_exit:
    if (rc == OK)
    {
        *pDeliveryId = deliveryId;
    }

    *pfStoredMDR = fStoredMDR;

    if (rc != OK)
    {
        if (!fLocked)
        {
            iecs_lockMessageDeliveryInfo(pMsgDelInfo);
            fLocked = true;
        }

        if (pMsgDelRef != NULL)
        {
            memset(pMsgDelRef, '\0', sizeof(*pMsgDelRef));
        }
    }

    if (fLocked)
    {
        iecs_unlockMessageDeliveryInfo(pMsgDelInfo);
    }

    ieutTRACEL(pThreadData, rc, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_EXIT "rc=%d fStoredMDR=%d\n", __func__, rc, (int)fStoredMDR);
    return rc;
}


//Once the store commit of an MDR has occurred, this function completes the job
//by updating the in memory state
static int32_t iecs_completeRemovalofStoredMDR ( ieutThreadData_t *pThreadData
                                                , iecsMessageDeliveryInfo_t  *pMsgDelInfo
                                                , iecsMessageDeliveryChunk_t *pMsgDelChunk
                                                , iecsMessageDeliveryReference_t *pMsgDelRef
                                                , uint32_t deliveryId
                                                , bool MsgDelInfoLocked)
{
    if (!MsgDelInfoLocked)
    {
        iecs_lockMessageDeliveryInfo(pMsgDelInfo);
    }

    // Keep track of how many MDRs there are above or below the target order id
    if (pMsgDelRef->MsgDeliveryRef1OrderId < pMsgDelInfo->TargetMinimumActiveOrderId)
    {
        pMsgDelInfo->MdrsBelowTarget--;
    }
    else
    {
        pMsgDelInfo->MdrsAboveTarget--;
    }
    if (pMsgDelRef->MsgDeliveryRef2OrderId < pMsgDelInfo->TargetMinimumActiveOrderId)
    {
        pMsgDelInfo->MdrsBelowTarget--;
    }
    else
    {
        pMsgDelInfo->MdrsAboveTarget--;
    }

    pMsgDelRef->MsgDeliveryRefState = 0;
    pMsgDelRef->hStoreMsgDeliveryRef1 = ismSTORE_NULL_HANDLE;
    pMsgDelRef->MsgDeliveryRef1OrderId = 0;
    pMsgDelRef->hStoreMsgDeliveryRef2 = ismSTORE_NULL_HANDLE;
    pMsgDelRef->MsgDeliveryRef2OrderId = 0;
    pMsgDelRef->hStoreRecord = ismSTORE_NULL_HANDLE;
    pMsgDelRef->hQueue = NULL;
    pMsgDelRef->hNode = NULL;
    pMsgDelRef->hStoreMessage = ismSTORE_NULL_HANDLE;

    int32_t rc = iecs_releaseDeliveryId_internal(pThreadData, pMsgDelInfo, deliveryId, pMsgDelChunk, pMsgDelRef);

    if (!MsgDelInfoLocked)
    {
        iecs_unlockMessageDeliveryInfo(pMsgDelInfo);
    }
    return rc;
}

static void iecs_startRemovalofStoredMDR ( ieutThreadData_t *pThreadData
                                         , iecsMessageDeliveryInfo_t  *pMsgDelInfo
                                         , iecsMessageDeliveryReference_t *pMsgDelRef
                                         , uint32_t deliveryId
                                         , uint32_t *pStoreOpCount)
{
    uint64_t minimumActiveOrderId = 0;
    int32_t  rc = OK;
    // If there are no MDRs using order ids below the target, we can update the MinimumActiveOrderId.
    if ((pMsgDelInfo->MdrsBelowTarget == 0) &&
        (pMsgDelInfo->NextOrderId > pMsgDelInfo->TargetMinimumActiveOrderId + iecsMDR_ORDER_ID_TARGET_GAP))
    {
        // We're going to give the minimum to the Store
        minimumActiveOrderId = pMsgDelInfo->TargetMinimumActiveOrderId;

        // Set the target to the next order id which is greater than the target by at least the gap amount
        pMsgDelInfo->TargetMinimumActiveOrderId = pMsgDelInfo->NextOrderId;

        ieutTRACEL(pThreadData, minimumActiveOrderId, ENGINE_HIFREQ_FNC_TRACE,
                   "Setting minimumActiveOrderId %lu, target %lu\n", minimumActiveOrderId, pMsgDelInfo->TargetMinimumActiveOrderId);

        // So, all existing MDRs above the target are now below the target
        pMsgDelInfo->MdrsBelowTarget = pMsgDelInfo->MdrsAboveTarget;
        pMsgDelInfo->MdrsAboveTarget = 0;
    }

    //We could unlock MessageDeliveryInfo here and change model so caller calls us with it locked and we release it

    rc = ism_store_deleteReference(pThreadData->hStream,
                                   pMsgDelInfo->hMsgDeliveryRefContext,
                                   pMsgDelRef->hStoreMsgDeliveryRef1,
                                   pMsgDelRef->MsgDeliveryRef1OrderId,
                                   minimumActiveOrderId);
    if (rc != OK)
    {
        ieutTRACE_FFDC(ieutPROBE_013, false,
                       "Deleting MDR ref 1", rc,
                       "hStoreCSR", &pMsgDelInfo->hStoreCSR, sizeof(pMsgDelInfo->hStoreCSR),
                       "hRef 1", &pMsgDelRef->hStoreMsgDeliveryRef1, sizeof(pMsgDelRef->hStoreMsgDeliveryRef1),
                       "orderId 1", &pMsgDelRef->MsgDeliveryRef1OrderId, sizeof(pMsgDelRef->MsgDeliveryRef1OrderId),
                       "hRef 2", &pMsgDelRef->hStoreMsgDeliveryRef2, sizeof(pMsgDelRef->hStoreMsgDeliveryRef2),
                       "orderId 2", &pMsgDelRef->MsgDeliveryRef2OrderId, sizeof(pMsgDelRef->MsgDeliveryRef2OrderId),
                       NULL);
        rc = OK; // Carry on regardless
    }
    (*pStoreOpCount)++;

    rc = ism_store_deleteReference(pThreadData->hStream,
                                   pMsgDelInfo->hMsgDeliveryRefContext,
                                   pMsgDelRef->hStoreMsgDeliveryRef2,
                                   pMsgDelRef->MsgDeliveryRef2OrderId,
                                   0);
    if (rc != OK)
    {
        ieutTRACE_FFDC(ieutPROBE_014, false,
                       "Deleting MDR ref 2", rc,
                       "hStoreCSR", &pMsgDelInfo->hStoreCSR, sizeof(pMsgDelInfo->hStoreCSR),
                       "hRef 1", &pMsgDelRef->hStoreMsgDeliveryRef1, sizeof(pMsgDelRef->hStoreMsgDeliveryRef1),
                       "orderId 1", &pMsgDelRef->MsgDeliveryRef1OrderId, sizeof(pMsgDelRef->MsgDeliveryRef1OrderId),
                       "hRef 2", &pMsgDelRef->hStoreMsgDeliveryRef2, sizeof(pMsgDelRef->hStoreMsgDeliveryRef2),
                       "orderId 2", &pMsgDelRef->MsgDeliveryRef2OrderId, sizeof(pMsgDelRef->MsgDeliveryRef2OrderId),
                       NULL);
        rc = OK; // Carry on regardless
    }
    (*pStoreOpCount)++;
}

int32_t iecs_unstoreMDRCommitted(
        ieutThreadData_t               *pThreadData,
        int32_t                         retcode,
        ismEngine_AsyncData_t          *asyncInfo,
        ismEngine_AsyncDataEntry_t     *context)
{
    iecsUnstoreMDRAsyncData_t *asyncData = (iecsUnstoreMDRAsyncData_t *)(context->Data);

    int32_t rc = iecs_completeRemovalofStoredMDR ( pThreadData
                                                 , asyncData->pMsgDelInfo
                                                 , asyncData->pMsgDelChunk
                                                 , asyncData->pMsgDelRef
                                                 , asyncData->deliveryId
                                                 , false);

    //NB: this might have told us that delivery ids are available which will need to be
    //dealt with by another ismEngine_AsyncDataEntry_t
    //...but we don't need to call this one again.
    iead_popAsyncCallback(asyncInfo, context->DataLen);
    return rc;
}

/*
 * Remove a Message Delivery Reference from the Store
 * NB: Adds to existing transaction and commits it
 * (Does NOT remove message record which needs to be dealt with separately.
 *
 */
int32_t iecs_unstoreMessageDeliveryReference(ieutThreadData_t *pThreadData,
                                             ismEngine_Message_t *pMsg,
                                             iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                                             uint32_t deliveryId,
                                             uint32_t *pStoreOpCount,
                                             bool *pDeliveryIdsAvailable,
                                             ismEngine_AsyncData_t *asyncInfo)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = hMsgDelInfo;
    iecsMessageDeliveryChunk_t *pMsgDelChunk = NULL;
    iecsMessageDeliveryReference_t *pMsgDelRef = NULL;

    bool fLocked = false;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pMsgDelInfo, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "(hMsgDelInfo %p, deliveryId %u)\n", __func__, hMsgDelInfo, deliveryId);

    assert(deliveryId < pMsgDelInfo->MdrChunkSize * pMsgDelInfo->MdrChunkCount);

    iecs_lockMessageDeliveryInfo(pMsgDelInfo);
    fLocked = true;

    if (pMsgDelInfo->hMsgDeliveryRefContext != NULL)
    {
        // Check the existing chunks for the supplied delivery ID
        pMsgDelChunk = pMsgDelInfo->pChunk[deliveryId / pMsgDelInfo->MdrChunkSize];
        if (pMsgDelChunk != NULL)
        {
            pMsgDelRef = pMsgDelChunk->Slot + (deliveryId % pMsgDelInfo->MdrChunkSize);

            if (pMsgDelRef->fSlotInUse && !pMsgDelRef->fSlotPending)
            {
                assert(pMsgDelRef->DeliveryId == deliveryId);
            }
            else
            {
                pMsgDelRef = NULL;
            }
        }

        if (pMsgDelRef == NULL)
        {
            rc = ISMRC_NotFound;
            ism_common_setError(rc);
            ieutTRACE_FFDC(ieutPROBE_012, false,
                           "Unstoring unknown MDR", rc,
                           "Delivery ID", &deliveryId, sizeof(deliveryId),
                           NULL);

            //Do the store commit this function is expected to, if we need to
            //Note that we lose the 'not found' rc code in this case but there
            //Isn't much our caller can do about it anyway!!!
            if (*pStoreOpCount != 0)
            {
                rc = iead_store_asyncCommit( pThreadData, false, asyncInfo);
                *pStoreOpCount = 0;
            }
            goto mod_exit;
        }
        else
        {
            // Use the caller's store transaction - we know that we aren't going to fill it by
            // deleting references
            iecs_startRemovalofStoredMDR ( pThreadData
                                         , pMsgDelInfo
                                         , pMsgDelRef
                                         , deliveryId
                                         , pStoreOpCount);

            // Now because we are going to start a store-transaction, we release the client-state
            // lock. We might be forced to wait for a new generation and this could take seconds.
            fLocked = false;
            iecs_unlockMessageDeliveryInfo(pMsgDelInfo);


            // We have deleted the MDR references so we must
            // commit these deletes before updating the pMsgDelInfo... let's set up to do it asyncly
            if (asyncInfo != NULL)
            {
                assert(asyncInfo->numEntriesUsed < asyncInfo->numEntriesAllocated);
                assert(asyncInfo->fOnStack); //We're going to point at things on the stack, check it's going to get copied
                iecsUnstoreMDRAsyncData_t consumeAsyncData = {IECS_UNSTOREMDR_ASYNCDATA_STRUCID, pMsgDelInfo, pMsgDelChunk, pMsgDelRef, deliveryId};
                ismEngine_AsyncDataEntry_t newEntry =
                     {ismENGINE_ASYNCDATAENTRY_STRUCID, RemoveMDR, &consumeAsyncData, sizeof(consumeAsyncData), NULL, {.internalFn = &iecs_unstoreMDRCommitted}};
                asyncInfo->entries[asyncInfo->numEntriesUsed] = newEntry;
                asyncInfo->numEntriesUsed++;

                rc = iead_store_asyncCommit( pThreadData, false, asyncInfo);
            }
            else
            {
                iest_store_commit( pThreadData, false);
            }


            if (rc == OK)
            {
                *pStoreOpCount = 0;

                rc = iecs_completeRemovalofStoredMDR ( pThreadData
                                                     , pMsgDelInfo
                                                     , pMsgDelChunk
                                                     , pMsgDelRef
                                                     , deliveryId
                                                     , false);

                if (rc == ISMRC_DeliveryIdAvailable)
                {
                    *pDeliveryIdsAvailable = true;
                    rc = OK;
                }
            }
        }
    }
    else
    {
        rc = iecs_releaseDeliveryId_internal(pThreadData, pMsgDelInfo, deliveryId, NULL, NULL);

        if (rc == ISMRC_DeliveryIdAvailable)
        {
            *pDeliveryIdsAvailable = true;
            rc = OK;
        }

        // Although we had no store references to delete we must still
        // complete the transaction
        if ((*pStoreOpCount) > 0)
        {
            rc = iead_store_asyncCommit( pThreadData, false, asyncInfo);
            *pStoreOpCount = 0;
        }
    }

mod_exit:
    if (fLocked)
    {
        iecs_unlockMessageDeliveryInfo(pMsgDelInfo);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//Either call iecs_unstoreMessageDeliveryReference (above) or call this function (commit if necessary)
//then call completeUnstoreMessageDeliveryReference
int32_t iecs_startUnstoreMessageDeliveryReference(ieutThreadData_t *pThreadData,
        ismEngine_Message_t *pMsg,
        iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
        uint32_t deliveryId,
        uint32_t *pStoreOpCount)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = hMsgDelInfo;
    iecsMessageDeliveryChunk_t *pMsgDelChunk = NULL;
    iecsMessageDeliveryReference_t *pMsgDelRef = NULL;
    int32_t rc = OK;

    iecs_lockMessageDeliveryInfo(pMsgDelInfo);
    bool fLocked = true;

    if (pMsgDelInfo->hMsgDeliveryRefContext != NULL)
    {
        // Check the existing chunks for the supplied delivery ID
        pMsgDelChunk = pMsgDelInfo->pChunk[deliveryId /pMsgDelInfo->MdrChunkSize];
        if (pMsgDelChunk != NULL)
        {
            pMsgDelRef = pMsgDelChunk->Slot + (deliveryId % pMsgDelInfo->MdrChunkSize);

            if (pMsgDelRef->fSlotInUse && !pMsgDelRef->fSlotPending)
            {
                assert(pMsgDelRef->DeliveryId == deliveryId);
            }
            else
            {
                pMsgDelRef = NULL;
            }
        }

        if (pMsgDelRef == NULL)
        {
            rc = ISMRC_NotFound;
            ism_common_setError(rc);
            ieutTRACE_FFDC(ieutPROBE_012, false,
                    "Unstoring unknown MDR", rc,
                    "Delivery ID", &deliveryId, sizeof(deliveryId),
                    NULL);

            goto mod_exit;
        }
        iecs_startRemovalofStoredMDR ( pThreadData
                                     , pMsgDelInfo
                                     , pMsgDelRef
                                     , deliveryId
                                     , pStoreOpCount);
    }


    mod_exit:
    if (fLocked)
    {
        iecs_unlockMessageDeliveryInfo(pMsgDelInfo);
    }

    return rc;
}

int32_t iecs_completeUnstoreMessageDeliveryReference(ieutThreadData_t *pThreadData,
                                                     ismEngine_Message_t *pMsg,
                                                     iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                                                     uint32_t deliveryId)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = hMsgDelInfo;
    iecsMessageDeliveryChunk_t *pMsgDelChunk = NULL;
    iecsMessageDeliveryReference_t *pMsgDelRef = NULL;
    int32_t rc = OK;

    iecs_lockMessageDeliveryInfo(pMsgDelInfo);
    bool fLocked = true;

    if (pMsgDelInfo->hMsgDeliveryRefContext != NULL)
    {
        // Check the existing chunks for the supplied delivery ID
        pMsgDelChunk = pMsgDelInfo->pChunk[deliveryId / pMsgDelInfo->MdrChunkSize];
        if (pMsgDelChunk != NULL)
        {
            pMsgDelRef = pMsgDelChunk->Slot + (deliveryId % pMsgDelInfo->MdrChunkSize);

            if (pMsgDelRef->fSlotInUse && !pMsgDelRef->fSlotPending)
            {
                assert(pMsgDelRef->DeliveryId == deliveryId);
            }
            else
            {
                pMsgDelRef = NULL;
            }
        }

        if (pMsgDelRef == NULL)
        {
            rc = ISMRC_NotFound;
            ism_common_setError(rc);
            ieutTRACE_FFDC(ieutPROBE_012, false,
                    "Unstoring unknown MDR", rc,
                    "Delivery ID", &deliveryId, sizeof(deliveryId),
                    NULL);

            goto mod_exit;
        }

        rc = iecs_completeRemovalofStoredMDR ( pThreadData
                                             , pMsgDelInfo
                                             , pMsgDelChunk
                                             , pMsgDelRef
                                             , deliveryId
                                             , true);
    }
    else
    {
        rc = iecs_releaseDeliveryId_internal(pThreadData, pMsgDelInfo, deliveryId, NULL, NULL);
    }

mod_exit:
    if (fLocked)
    {
        iecs_unlockMessageDeliveryInfo(pMsgDelInfo);
    }

    return rc;
}

/*
 * Used during rehydrate/import etc.. to reassign a particular deliveryId. Assumes caller
 * has taken relevant locks.
 */
static inline int32_t iecs_restoreInMemMessageDeliveryReference(ieutThreadData_t *pThreadData,
                                                                ismEngine_ClientState_t *pClient,
                                                                uint32_t deliveryId,
                                                                uint8_t mdrState,
                                                                bool allowExisting,
                                                                bool rehydrating,
                                                                iecsMessageDeliveryReference_t **ppMsgDelRef)
{
    int32_t rc = OK;
    iecsMessageDeliveryInfo_t *pMsgDelInfo;
    iecsMessageDeliveryChunk_t *pMsgDelChunk = NULL;
    iecsMessageDeliveryReference_t *pMsgDelRef = NULL;

    // Check the existing chunks for the supplied delivery ID. We need two references
    // from the Store to make up a complete MDR
    pMsgDelInfo = pClient->hMsgDeliveryInfo;
    if (pMsgDelInfo != NULL)
    {
        pMsgDelChunk = pMsgDelInfo->pChunk[deliveryId / pMsgDelInfo->MdrChunkSize];
        if (pMsgDelChunk != NULL)
        {
            pMsgDelRef = pMsgDelChunk->Slot + (deliveryId % pMsgDelInfo->MdrChunkSize);

            if (pMsgDelRef->fSlotInUse)
            {
                assert(pMsgDelRef->DeliveryId == deliveryId);

                if (!allowExisting)
                {
                    ieutTRACEL(pThreadData, pMsgDelRef->DeliveryId, ENGINE_ERROR_TRACE,
                            "Unexpected duplicate MDR, exiting deliveryid %d, new %d\n",
                            pMsgDelRef->DeliveryId, deliveryId);

                    rc = ISMRC_Error;
                    ism_common_setError(rc);
                    goto mod_exit;
                }
            }
            else
            {
                pMsgDelRef->fSlotInUse = true;
                pMsgDelRef->MsgDeliveryRefState = mdrState;
                pMsgDelRef->DeliveryId = deliveryId;
                pMsgDelChunk->slotsInUse++;
                pMsgDelInfo->NumDeliveryIds++;
            }
        }
    }

    // If we don't yet have storage allocated for this MDR, allocate it now
    if (pMsgDelRef == NULL)
    {
        assert(pMsgDelChunk == NULL);
        if (pMsgDelInfo == NULL)
        {
            rc = iecs_newMessageDeliveryInfo(pThreadData, pClient, &pMsgDelInfo, rehydrating);
            if (rc != OK)
            {
                goto mod_exit;
            }
        }

        assert(pMsgDelInfo->resourceSet == pClient->resourceSet);
        iereResourceSetHandle_t resourceSet = pMsgDelInfo->resourceSet;

        iere_primeThreadCache(pThreadData, resourceSet);
        pMsgDelChunk = iere_calloc(pThreadData,
                                   resourceSet,
                                   IEMEM_PROBE(iemem_clientState, 17), 1,
                                   sizeof(iecsMessageDeliveryChunk_t) +
                                   (pMsgDelInfo->MdrChunkSize * sizeof(iecsMessageDeliveryReference_t)));

        if (pMsgDelChunk != NULL)
        {
            pMsgDelInfo->pChunk[deliveryId / pMsgDelInfo->MdrChunkSize] = pMsgDelChunk;
            pMsgDelInfo->ChunksInUse++;

            pMsgDelRef = pMsgDelChunk->Slot + (deliveryId % pMsgDelInfo->MdrChunkSize);

            pMsgDelRef->fSlotInUse = true;
            pMsgDelRef->MsgDeliveryRefState = mdrState;
            pMsgDelRef->DeliveryId = deliveryId;
            pMsgDelChunk->slotsInUse = 1;
            pMsgDelInfo->NumDeliveryIds++;
        }
        else
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }

    if (deliveryId >= pMsgDelInfo->NextDeliveryId)
    {
        pMsgDelInfo->NextDeliveryId = deliveryId + 1;
    }


mod_exit:
    *ppMsgDelRef = pMsgDelRef;
    return rc;
}

/*
 * Rehydrate a Message Delivery Reference from the Store during recovery
 */
int32_t iecs_rehydrateMessageDeliveryReference(ieutThreadData_t *pThreadData,
                                               ismEngine_ClientState_t *pClient,
                                               ismStore_Handle_t hStoreMDR,
                                               ismStore_Reference_t *pMDR)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = NULL;
    iecsMessageDeliveryReference_t *pMsgDelRef = NULL;
    uint32_t deliveryId;
    uint8_t mdrState;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, hStoreMDR, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "(pClient %p, orderId %lu, deliveryId %u)\n",
               __func__, pClient, pMDR->OrderId, pMDR->Value);

    assert(pClient->Durability == iecsDurable);
    assert(pClient->hStoreCSR != ismSTORE_NULL_HANDLE);

    // If the client-state has been marked as being destroyed in the Store, we certainly
    // don't want to bother with rehydrating MDRs only to discard all of the message IDs
    if (pClient->fDiscardDurable)
    {
        ieutTRACEL(pThreadData, pClient, ENGINE_HIFREQ_FNC_TRACE, "Client-state marked for discard\n");
        goto mod_exit;
    }

    // The delivery ID is encoded in the reference's value
    deliveryId = pMDR->Value;
    mdrState = pMDR->State & (iestMDR_STATE_OWNER_IS_QUEUE | iestMDR_STATE_OWNER_IS_SUBSC);

    rc = iecs_restoreInMemMessageDeliveryReference( pThreadData
                                                  , pClient
                                                  , deliveryId
                                                  , mdrState
                                                  , true
                                                  , true
                                                  , &pMsgDelRef);
    if (UNLIKELY(rc != OK))
    {
        ieutTRACEL(pThreadData, mdrState, ENGINE_ERROR_TRACE,
                "Unable to restore message-delivery reference (handle 0x%lx) %d\n",
                hStoreMDR, rc);
        goto mod_exit;
    }

    assert(pMsgDelRef != NULL);
    pMsgDelInfo = pClient->hMsgDeliveryInfo;
    assert(pMsgDelInfo != NULL);

    // Ensure that the MDR consistently refers to a subscription or a queue
    if ((pMsgDelRef->MsgDeliveryRefState &
            (iestMDR_STATE_OWNER_IS_QUEUE | iestMDR_STATE_OWNER_IS_SUBSC)) != mdrState)
    {
        ieutTRACEL(pThreadData, mdrState, ENGINE_ERROR_TRACE,
                "Inconsistent destination type for message-delivery reference (handle 0x%lx) expected %d, actual %d\n",
                   hStoreMDR,  pMsgDelRef->MsgDeliveryRefState, mdrState);

        rc = ISMRC_Error;
        ism_common_setError(rc);
        goto mod_exit;
    }

    if (pMDR->State & iestMDR_STATE_HANDLE_IS_RECORD)
    {
        pMsgDelRef->fHandleIsPointer = false;
        pMsgDelRef->hStoreMsgDeliveryRef1 = hStoreMDR;
        pMsgDelRef->MsgDeliveryRef1OrderId = pMDR->OrderId;
        pMsgDelRef->hStoreRecord = pMDR->hRefHandle;
        pMsgDelRef->MsgDeliveryRefState |= iestMDR_STATE_HANDLE_IS_RECORD;
    }
    else
    {
        assert(pMDR->State & iestMDR_STATE_HANDLE_IS_MESSAGE);
        pMsgDelRef->hStoreMsgDeliveryRef2 = hStoreMDR;
        pMsgDelRef->MsgDeliveryRef2OrderId = pMDR->OrderId;
        pMsgDelRef->hStoreMessage = pMDR->hRefHandle;
        pMsgDelRef->MsgDeliveryRefState |= iestMDR_STATE_HANDLE_IS_MESSAGE;
    }

    // Make sure that the next order id we assign is greater than any we find.
    // For restart, we set the target minimum to the lowest order id we see
    // and all MDRs are above the target. We re-establish the maintenance of
    // the MinimumActiveOrderId once we're in normal running.
    if (pMDR->OrderId < pMsgDelInfo->TargetMinimumActiveOrderId)
    {
        pMsgDelInfo->TargetMinimumActiveOrderId = pMDR->OrderId;
    }
    pMsgDelInfo->MdrsAboveTarget++;

    // If we have both halves of the MDR, we can set the delivery ID on the message node
    if ((pMsgDelRef->MsgDeliveryRefState & iestMDR_STATE_HANDLE_IS_RECORD) &&
        (pMsgDelRef->MsgDeliveryRefState & iestMDR_STATE_HANDLE_IS_MESSAGE))
    {
        if (pMsgDelRef->MsgDeliveryRefState & iestMDR_STATE_OWNER_IS_SUBSC)
        {
            rc = ierr_setMessageDeliveryIdFromMDR(pThreadData,
                                                  pMsgDelInfo,
                                                  pMsgDelRef->hStoreRecord,
                                                  &pMsgDelRef->hQueue,
                                                  &pMsgDelRef->hNode,
                                                  ISM_STORE_RECTYPE_SUBSC,
                                                  pMsgDelRef->hStoreMessage,
                                                  pMsgDelRef->DeliveryId);

            if (rc != OK)
            {
                ierr_recordBadStoreRecord( pThreadData
                                         , ISM_STORE_RECTYPE_CLIENT
                                         , pClient->hStoreCSR
                                         , pClient->pClientId
                                         , rc);

                // If partial recovery is allowed, we want to delete this client
                pClient->fDiscardDurable = true;
            }
        }
        else
        {
            rc = ISMRC_NotImplemented;
            ism_common_setError(rc);
            ieutTRACE_FFDC(ieutPROBE_015, false,
                           "Rehydrating MDR for a queue", rc,
                           NULL);
        }
    }

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

/*
 * Unstore all MDRs and release all delivery ids for a given owning record
 */
int32_t iecs_releaseAllMessageDeliveryReferences(ieutThreadData_t *pThreadData,
                                                 iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                                                 ismStore_Handle_t hStoreOwner,
                                                 bool fHandleIsPointer)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = hMsgDelInfo;
    iecsMessageDeliveryChunk_t *pMsgDelChunk = NULL;
    iecsMessageDeliveryReference_t *pMsgDelRef = NULL;
    uint64_t minimumActiveOrderId = 0;
    int32_t i, j;
    bool fLocked = false;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pMsgDelInfo, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "(hMsgDelInfo %p, hStoreOwner %lx, fHandleIsPointer %s)\n",
               __func__, hMsgDelInfo, hStoreOwner, fHandleIsPointer ? "true" : "false");

    if (hMsgDelInfo == NULL)
    {
        goto mod_exit;
    }

    iecs_lockMessageDeliveryInfo(pMsgDelInfo);
    fLocked = true;

    if (pMsgDelInfo->hMsgDeliveryRefContext != NULL)
    {
        for (i = 0; i < pMsgDelInfo->MdrChunkCount; i++)
        {
            //Each time around the for loop, the chunk could have been freed if we deleted the last id
            for ( j = 0
                ; j < pMsgDelInfo->MdrChunkSize  && pMsgDelInfo->pChunk[i] != NULL && pMsgDelInfo->pChunk[i]->slotsInUse > 0
                ; j++)
            {
                pMsgDelChunk = pMsgDelInfo->pChunk[i];
                pMsgDelRef   = &(pMsgDelChunk->Slot[j]);

                if ((pMsgDelRef->fSlotInUse) && (!pMsgDelRef->fSlotPending) &&
                    (pMsgDelRef->hStoreRecord == hStoreOwner) && (pMsgDelRef->fHandleIsPointer == fHandleIsPointer))
                {
                    // If there are no MDRs using order ids below the target, we can update the MinimumActiveOrderId.
                    if ((pMsgDelInfo->MdrsBelowTarget == 0) &&
                        (pMsgDelInfo->NextOrderId > pMsgDelInfo->TargetMinimumActiveOrderId + iecsMDR_ORDER_ID_TARGET_GAP))
                    {
                        // We're going to give the minimum to the Store
                        minimumActiveOrderId = pMsgDelInfo->TargetMinimumActiveOrderId;

                        // Set the target to the next order id which is greater than the target by at least the gap amount
                        pMsgDelInfo->TargetMinimumActiveOrderId = pMsgDelInfo->NextOrderId;

                        ieutTRACEL(pThreadData, minimumActiveOrderId, ENGINE_HIFREQ_FNC_TRACE,
                                   "Setting minimumActiveOrderId %lu, target %lu\n",
                                   minimumActiveOrderId, pMsgDelInfo->TargetMinimumActiveOrderId);

                        // So, all existing MDRs above the target are now below the target
                        pMsgDelInfo->MdrsBelowTarget = pMsgDelInfo->MdrsAboveTarget;
                        pMsgDelInfo->MdrsAboveTarget = 0;
                    }

                    // Now because we are going to start a store-transaction, we release the client-state
                    // lock. We might be forced to wait for a new generation and this could take seconds.
                    fLocked = false;
                    iecs_unlockMessageDeliveryInfo(pMsgDelInfo);

                    iest_AssertStoreCommitAllowed(pThreadData);

                    rc = ism_store_deleteReference(pThreadData->hStream,
                                                   pMsgDelInfo->hMsgDeliveryRefContext,
                                                   pMsgDelRef->hStoreMsgDeliveryRef1,
                                                   pMsgDelRef->MsgDeliveryRef1OrderId,
                                                   minimumActiveOrderId);
                    if (rc == OK)
                    {
                        rc = iest_store_deleteReferenceCommit(pThreadData,
                                                              pThreadData->hStream,
                                                              pMsgDelInfo->hMsgDeliveryRefContext,
                                                              pMsgDelRef->hStoreMsgDeliveryRef2,
                                                              pMsgDelRef->MsgDeliveryRef2OrderId,
                                                              0);
                        if (rc == OK)
                        {
                            iecs_lockMessageDeliveryInfo(pMsgDelInfo);
                            fLocked = true;

                            // Keep track of how many MDRs there are above or below the target order id
                            if (pMsgDelRef->MsgDeliveryRef1OrderId < pMsgDelInfo->TargetMinimumActiveOrderId)
                            {
                                pMsgDelInfo->MdrsBelowTarget--;
                            }
                            else
                            {
                                pMsgDelInfo->MdrsAboveTarget--;
                            }
                            if (pMsgDelRef->MsgDeliveryRef2OrderId < pMsgDelInfo->TargetMinimumActiveOrderId)
                            {
                                pMsgDelInfo->MdrsBelowTarget--;
                            }
                            else
                            {
                                pMsgDelInfo->MdrsAboveTarget--;
                            }
                        }
                        else
                        {
                            ieutTRACE_FFDC(ieutPROBE_016, false,
                                           "Deleting MDR ref 2", rc,
                                           "hStoreCSR", &pMsgDelInfo->hStoreCSR, sizeof(pMsgDelInfo->hStoreCSR),
                                           "hRef 1", &pMsgDelRef->hStoreMsgDeliveryRef1, sizeof(pMsgDelRef->hStoreMsgDeliveryRef1),
                                           "orderId 1", &pMsgDelRef->MsgDeliveryRef1OrderId, sizeof(pMsgDelRef->MsgDeliveryRef1OrderId),
                                           "hRef 2", &pMsgDelRef->hStoreMsgDeliveryRef2, sizeof(pMsgDelRef->hStoreMsgDeliveryRef2),
                                           "orderId 2", &pMsgDelRef->MsgDeliveryRef2OrderId, sizeof(pMsgDelRef->MsgDeliveryRef2OrderId),
                                           NULL);
                        }
                    }
                    else
                    {
                        ieutTRACE_FFDC(ieutPROBE_017, false,
                                       "Deleting MDR ref 1", rc,
                                       "hStoreCSR", &pMsgDelInfo->hStoreCSR, sizeof(pMsgDelInfo->hStoreCSR),
                                       "hRef 1", &pMsgDelRef->hStoreMsgDeliveryRef1, sizeof(pMsgDelRef->hStoreMsgDeliveryRef1),
                                       "orderId 1", &pMsgDelRef->MsgDeliveryRef1OrderId, sizeof(pMsgDelRef->MsgDeliveryRef1OrderId),
                                       "hRef 2", &pMsgDelRef->hStoreMsgDeliveryRef2, sizeof(pMsgDelRef->hStoreMsgDeliveryRef2),
                                       "orderId 2", &pMsgDelRef->MsgDeliveryRef2OrderId, sizeof(pMsgDelRef->MsgDeliveryRef2OrderId),
                                       NULL);
                    }

                    if (!fLocked)
                    {
                        iecs_lockMessageDeliveryInfo(pMsgDelInfo);
                        fLocked = true;
                    }

                    pMsgDelRef->MsgDeliveryRefState = 0;
                    pMsgDelRef->hStoreMsgDeliveryRef1 = ismSTORE_NULL_HANDLE;
                    pMsgDelRef->MsgDeliveryRef1OrderId = 0;
                    pMsgDelRef->hStoreMsgDeliveryRef2 = ismSTORE_NULL_HANDLE;
                    pMsgDelRef->MsgDeliveryRef2OrderId = 0;
                    pMsgDelRef->hStoreRecord = ismSTORE_NULL_HANDLE;
                    pMsgDelRef->hQueue = NULL;
                    pMsgDelRef->hNode = NULL;
                    pMsgDelRef->hStoreMessage = ismSTORE_NULL_HANDLE;

                    (void)iecs_releaseDeliveryId_internal(pThreadData, pMsgDelInfo, pMsgDelRef->DeliveryId, pMsgDelChunk, pMsgDelRef);
                }
            }
        }
    }
    else
    {
        uint32_t mdrChunkCount = pMsgDelInfo->MdrChunkCount;
        uint32_t mdrChunkSize  = pMsgDelInfo->MdrChunkSize;

        for (i = 0; i < mdrChunkCount; i++)
        {
            for ( j = 0
                ; j < mdrChunkSize && pMsgDelInfo->pChunk[i] != NULL &&  pMsgDelInfo->pChunk[i]->slotsInUse > 0
                ; j++)
            {
                pMsgDelChunk = pMsgDelInfo->pChunk[i];
                pMsgDelRef   = &(pMsgDelChunk->Slot[j]);

                if ((pMsgDelRef->fSlotInUse) && (!pMsgDelRef->fSlotPending) &&
                    (pMsgDelRef->hStoreRecord == hStoreOwner) && (pMsgDelRef->fHandleIsPointer == fHandleIsPointer))
                {
                    (void)iecs_releaseDeliveryId_internal(pThreadData, pMsgDelInfo, pMsgDelRef->DeliveryId, pMsgDelChunk, pMsgDelRef);
                }
            }
        }
    }

mod_exit:
    if (fLocked)
    {
        iecs_unlockMessageDeliveryInfo(pMsgDelInfo);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

/*
 * Locks a client-state
 */
void iecs_lockClientState(ismEngine_ClientState_t *pClient)
{
    int osrc = pthread_mutex_lock(&pClient->Mutex);
    if (UNLIKELY(osrc != 0))
    {
        int32_t rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(ieutPROBE_018, true,
                       "pthread_mutex_lock failed", rc,
                       "osrc", &osrc, sizeof(osrc),
                       NULL);
    }
}

/*
 * Unlocks a client-state
 */
void iecs_unlockClientState(ismEngine_ClientState_t *pClient)
{
    (void)pthread_mutex_unlock(&pClient->Mutex);
}

/*
 * Locks the unreleased delivery state for a client-state
 */
void iecs_lockUnreleasedDeliveryState(ismEngine_ClientState_t *pClient)
{
    int osrc = pthread_mutex_lock(&pClient->UnreleasedMutex);
    if (UNLIKELY(osrc != 0))
    {
        int32_t rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(ieutPROBE_019, true,
                       "pthread_mutex_lock failed", rc,
                       "osrc", &osrc, sizeof(osrc),
                       NULL);
    }
}

/*
 * Unlocks the unreleased delivery state for a client-state
 */
void iecs_unlockUnreleasedDeliveryState(ismEngine_ClientState_t *pClient)
{
    (void)pthread_mutex_unlock(&pClient->UnreleasedMutex);
}

/*
 * Link a transaction to the client state. This is done when a global
 * transaction is not associated with a session. When a client state
 * is destroyed any transactions in the list will be rolled back.
 */
void iecs_linkTransaction(ieutThreadData_t *pThreadData,
                          ismEngine_ClientState_t *pClient,
                          ismEngine_Transaction_t *pTran)
{
    ieutTRACEL(pThreadData, pClient,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "(pClient %p pTran %p)\n", __func__, pClient, pTran);

    int osrc = pthread_mutex_lock(&pClient->Mutex);
    if (UNLIKELY(osrc != 0))
    {
        uint32_t rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(ieutPROBE_020, true,
                       "pthread_mutex_lock failed", rc,
                       "osrc", &osrc, sizeof(osrc),
                       NULL);
    }

    pTran->pNext = pClient->pGlobalTransactions;
    pClient->pGlobalTransactions = pTran;

    pTran->pClient = pClient;

    (void)pthread_mutex_unlock(&pClient->Mutex);

    ieutTRACEL(pThreadData, pTran, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return;
}

/*
 * Unlink a transaction from the client state. 
 */
void iecs_unlinkTransaction( ismEngine_ClientState_t *pClient,
                             ismEngine_Transaction_t *pTran)
{
    int osrc = pthread_mutex_lock(&pClient->Mutex);
    if (UNLIKELY(osrc != 0))
    {
        uint32_t rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(ieutPROBE_021, true,
                       "pthread_mutex_lock failed", rc,
                       "osrc", &osrc, sizeof(osrc),
                       NULL);
    }

    if (pClient->pGlobalTransactions ==  pTran)
    {
        pClient->pGlobalTransactions = pTran->pNext;
        pTran->pNext = NULL;

        pTran->pClient = NULL;
    }
    else
    {
        ismEngine_Transaction_t *pPrevTran = pClient->pGlobalTransactions;
        while ((pPrevTran->pNext != pTran) && (pPrevTran->pNext != NULL))
        {
            pPrevTran = pPrevTran->pNext;
        }

        if (pPrevTran->pNext == NULL)
        {
            ieutTRACE_FFDC(ieutPROBE_022, false,
                           "Unassociated transaction not found in client state",
                           ISMRC_NotFound,
                           "Client State", pClient, sizeof(ismEngine_ClientState_t),
                           "Transaction", pTran, sizeof(ismEngine_Transaction_t),
                           NULL);
        }
        else
        {
            pPrevTran->pNext = pPrevTran->pNext->pNext;
            pTran->pNext = NULL;

            pTran->pClient = NULL;
        }
    }

    (void)pthread_mutex_unlock(&pClient->Mutex);

    return;
}

/*
* Assign the next available message delivery id for a given client
*/
int32_t iecs_assignDeliveryId(ieutThreadData_t *pThreadData,
                              iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                              ismEngine_Session_t *pSession,
                              ismStore_Handle_t hStoreRecord,
                              ismQHandle_t hQueue,
                              void *hNode,
                              bool fHandleIsPointer,
                              uint32_t *pDeliveryId)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = hMsgDelInfo;
    int rc = OK;

    // Only want to store the queue handle for shared queues
    if (hQueue == NULL || ieq_getQType(hQueue) != multiConsumer)
    {
        hQueue = NULL;
        hNode = NULL;
    }

    iecs_lockMessageDeliveryInfo(pMsgDelInfo);

    rc = iecs_assignDeliveryId_internal(pThreadData, pMsgDelInfo, pSession, hStoreRecord, hQueue, hNode, fHandleIsPointer, pDeliveryId, NULL, NULL);

    iecs_unlockMessageDeliveryInfo(pMsgDelInfo);

    return rc;
}

/*
* Remove a deliveryId for a client.
*/
int32_t iecs_releaseDeliveryId(ieutThreadData_t *pThreadData,
                               iecsMessageDeliveryInfoHandle_t hMsgDelInfo,
                               uint32_t deliveryId)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = hMsgDelInfo;
    int rc;

    iecs_lockMessageDeliveryInfo(pMsgDelInfo);

    rc = iecs_releaseDeliveryId_internal(pThreadData, pMsgDelInfo, deliveryId, NULL, NULL);

    iecs_unlockMessageDeliveryInfo(pMsgDelInfo);

    return rc;
}

/*
 * Configuration callback to handle deletion of client-state objects
 */
int iecs_clientStateConfigCallback(ieutThreadData_t *pThreadData,
                                   char *objectIdentifier,
                                   ism_prop_t *changedProps,
                                   ism_ConfigChangeType_t changeType)
{
    int rc = OK;

    ieutTRACEL(pThreadData, changeType, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Get the client id
    char *clientId = objectIdentifier;

    ieutTRACEL(pThreadData, 0, ENGINE_NORMAL_TRACE, "ClientId='%s'\n", clientId);

    // The action taken varies depending on the requested changeType.
    switch(changeType)
    {
        case ISM_CONFIG_CHANGE_DELETE:
            rc = iecs_discardZombieClientState(pThreadData, clientId, false, NULL, 0, NULL);
            assert(rc != ISMRC_AsyncCompletion);
            break;

        default:
            rc = ISMRC_InvalidOperation;
            break;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;

}

//Record that we have run out of deliveryids so future acks try and restart the session...
//returns number of acks in flight when we did it
uint32_t iecs_markDeliveryIdsExhausted( ieutThreadData_t *pThreadData
                                      , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                                      , ismEngine_Session_t *pSession)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = hMsgDelInfo;
    ieutTRACEL(pThreadData, pSession, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "hMsgDelInfo %p pSession %p\n", __func__, hMsgDelInfo, pSession);
    iecs_lockMessageDeliveryInfo(pMsgDelInfo);

    pMsgDelInfo->fIdsExhausted = true;
    uint32_t numAcksInflight = pMsgDelInfo->NumDeliveryIds;

    //We don't set/inquire the engine flow control flag in the session... if it has been removed
    //because the protocol/transport has taken control we'll not act on the flag we've set..

    iecs_unlockMessageDeliveryInfo(pMsgDelInfo);
    ieutTRACEL(pThreadData, pMsgDelInfo, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return numAcksInflight;
}

//If we stopped delivery as we ran out of ids and now can turn it on again... this will tell us
bool iecs_canRestartDelivery( ieutThreadData_t *pThreadData
                            , iecsMessageDeliveryInfoHandle_t hMsgDelInfo)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = hMsgDelInfo;

    bool doRestart = false;

    ieutTRACEL(pThreadData, hMsgDelInfo, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "hMsgDelInfo %p\n", __func__, hMsgDelInfo);

    iecs_lockMessageDeliveryInfo(pMsgDelInfo);

    if (pMsgDelInfo->fIdsExhausted && pMsgDelInfo->NumDeliveryIds <= pMsgDelInfo->InflightReenable)
    {
        doRestart = true;
        pMsgDelInfo->fIdsExhausted = false;
    }

    iecs_unlockMessageDeliveryInfo(pMsgDelInfo);

    ieutTRACEL(pThreadData, doRestart, ENGINE_FNC_TRACE, FUNCTION_EXIT "doRestart=%d\n", __func__, doRestart);
    return doRestart;
}

//If we stopped delivery as we ran out of ids and now can turn it on again... this will tell us
void iecs_getDeliveryStats( ieutThreadData_t *pThreadData
                           , iecsMessageDeliveryInfoHandle_t hMsgDelInfo
                           , bool *pfIdsExhausted
                           , uint32_t *pInflightMessages
                           , uint32_t *pInflightMax
                           , uint32_t *pInflightReenable)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = hMsgDelInfo;

    ieutTRACEL(pThreadData, hMsgDelInfo, ENGINE_FNC_TRACE,
               FUNCTION_ENTRY "hMsgDelInfo %p\n", __func__, hMsgDelInfo);

    iecs_lockMessageDeliveryInfo(pMsgDelInfo);

    *pfIdsExhausted    = pMsgDelInfo->fIdsExhausted;
    *pInflightMessages = pMsgDelInfo->NumDeliveryIds;
    *pInflightMax      = pMsgDelInfo->MaxInflightMsgs;
    *pInflightReenable = pMsgDelInfo->InflightReenable;

    iecs_unlockMessageDeliveryInfo(pMsgDelInfo);

    ieutTRACEL(pThreadData, *pInflightMessages, ENGINE_FNC_TRACE, FUNCTION_EXIT "inflight=%u\n", __func__, *pInflightMessages);
    return;
}

//Some protocols (MQTT) need to have message delivery complete for a message once it is started
//This function is called when the client has unsubbed so that the client can keep track of the queue
//now that the topic tree will no longer do so
void iecs_trackInflightMsgs(ieutThreadData_t *pThreadData,
                            ismEngine_ClientState_t *pClient,
                            ismQHandle_t queue)
{
    ieutTRACEL(pThreadData, pClient, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pClient %p queue %p\n",
               __func__, pClient, queue);

    iecs_lockClientState(pClient);

    if (ieq_redeliverEssentialOnly(pThreadData, queue))
    {
        iecsInflightDestination_t *pDest = pClient->inflightDestinationHead;

        while(pDest != NULL)
        {
            if (pDest->inflightContainer == queue)
            {
                break;
            }

            pDest = pDest->next;
        }

        // Didn't already find this destination in the inflight list, so need to add it.
        if (pDest == NULL)
        {
            iereResourceSetHandle_t resourceSet = pClient->resourceSet;

            iere_primeThreadCache(pThreadData, resourceSet);
            pDest = iere_calloc(pThreadData,
                                resourceSet,
                                IEMEM_PROBE(iemem_clientState, 19), 1,
                                sizeof(iecsInflightDestination_t));

            if (pDest == NULL)
            {
                iecs_unlockClientState(pClient);

                //It's not disasterous. We'll have a memory leak in the case that
                //The client state data is destroyed with acks outstanding.
                //FFDC so we know we are having the problem and continue

                ieutTRACE_FFDC(ieutPROBE_001, false,
                               "Not enough memory to track inflight messages", ISMRC_AllocateError,
                               "Client Id", pClient->pClientId, strlen(pClient->pClientId),
                               NULL);

                goto mod_exit;
            }
            else
            {
                // Add it to the head.
                pDest->inflightContainer = queue;
                pDest->next = pClient->inflightDestinationHead;
                pClient->inflightDestinationHead = pDest;
            }
        }
    }

    iecs_unlockClientState(pClient);

mod_exit:

    ieutTRACEL(pThreadData, queue, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}

// NOTE: The clientState lock should be held before calling this function
void iecs_forgetInflightMsgs( ieutThreadData_t *pThreadData
                            , ismEngine_ClientState_t *pClient )
{
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    ieutTRACEL(pThreadData, pClient,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pClient %p\n",  __func__, pClient);

    while (pClient->inflightDestinationHead != NULL)
    {
        iecsInflightDestination_t *pTmp = pClient->inflightDestinationHead;
        pClient->inflightDestinationHead = pClient->inflightDestinationHead->next;

        ieq_forgetInflightMsgs(pThreadData, pTmp->inflightContainer);
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_freeStruct(pThreadData, resourceSet, iemem_clientState, pTmp, pTmp->StrucId);
    }

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}

void  iecs_completedInflightMsgs(ieutThreadData_t *pThreadData,
                                 ismEngine_ClientState_t *pClient,
                                 ismQHandle_t queue)
{
    iereResourceSetHandle_t resourceSet = pClient->resourceSet;

    iecs_lockClientState(pClient);

    iecsInflightDestination_t *pDest = pClient->inflightDestinationHead;
    iecsInflightDestination_t *pPrev = NULL;

    while (pDest != NULL)
    {
        if (pDest->inflightContainer == queue)
        {
            break;
        }
        pPrev = pDest;
        pDest = pDest->next;
    }

    //Did we find the right dest
    if (pDest != NULL)
    {
        //Yep..we found it.. was it first in the list
        if (pClient->inflightDestinationHead == pDest)
        {
            //It was first... set the head pointer to subsequent one
            pClient->inflightDestinationHead = pDest->next;
        }
        else
        {
            //Not the first in the list so pPrev should be set...
            assert(pPrev != NULL);

            //Take the destination we found out of the list
            pPrev->next = pDest->next;
        }
    }

    iecs_unlockClientState(pClient);

    if (pDest != NULL)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_freeStruct(pThreadData, resourceSet, iemem_clientState, pDest, pDest->StrucId);
    }
    return;
}

bool iecs_msgInFlightForClient(ieutThreadData_t *pThreadData,
                               iecsMessageDeliveryInfoHandle_t  hMsgDelInfo,
                               uint32_t deliveryId,
                               void *hnode)
{
    iecsMessageDeliveryInfo_t       *pMsgDelInfo = hMsgDelInfo;
    iecsMessageDeliveryChunk_t      *pMsgDelChunk;

    iecs_lockMessageDeliveryInfo(pMsgDelInfo);

    bool msgInFlight = false;

    pMsgDelChunk = pMsgDelInfo->pChunk[deliveryId / pMsgDelInfo->MdrChunkSize];

    if (pMsgDelChunk != NULL)
    {
        iecsMessageDeliveryReference_t  *pMsgDelRef;

        pMsgDelRef = pMsgDelChunk->Slot + (deliveryId % pMsgDelInfo->MdrChunkSize);

        if (pMsgDelRef->fSlotInUse)
        {
            if (pMsgDelRef->hNode == hnode)
            {
                msgInFlight = true;
            }
        }
    }
    iecs_unlockMessageDeliveryInfo(pMsgDelInfo);

    ieutTRACEL(pThreadData, msgInFlight, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "hMsgDelInfo %p dId %u result %d\n",
               __func__, hMsgDelInfo, deliveryId, msgInFlight);

    return msgInFlight;
}

void iecs_sessionCleanupCompleted(
   int32_t                         retcode,
   void *                          handle,
   void *                          pContext)
{
    ismEngine_Session_t *pSession = *(ismEngine_Session_t **)pContext;
    ieutThreadData_t *pThreadData = ismEngine_threadData;

    ieutTRACEL(pThreadData, pSession,  ENGINE_FNC_TRACE, FUNCTION_IDENT "pSession %p\n",  __func__, pSession);
    assert(retcode == OK);
    releaseSessionReference(pThreadData, pSession, false);
}

/// @return OK Or AsyncCompletion - If async will reduce session usecount on completion
int32_t iecs_sessionCleanupFromDeliveryInfo( ieutThreadData_t *pThreadData
                                           , ismEngine_Session_t *pSession
                                           , iecsMessageDeliveryInfoHandle_t hMsgDeliveryInfo)
{
    iecsMessageDeliveryInfo_t *pMsgDelInfo = (iecsMessageDeliveryInfo_t *)hMsgDeliveryInfo;

    ismEngine_DeliveryInternal_t DeliveryArray[pMsgDelInfo->MaxInflightMsgs];
    uint32_t arraypos = 0;

    // Lock the message delivery info structure
    iecs_lockMessageDeliveryInfo(pMsgDelInfo);

    if (pMsgDelInfo->NumDeliveryIds > 0)
    {
        // Build an array of all the MDRs
        uint32_t mdrChunkCount   = pMsgDelInfo->MdrChunkCount;
        uint32_t mdrChunkSize    = pMsgDelInfo->MdrChunkSize;
        uint32_t maxInflightMsgs = pMsgDelInfo->MaxInflightMsgs;

        for (uint32_t i = 0; i < mdrChunkCount; i++)
        {
            iecsMessageDeliveryChunk_t *pMsgDelChunk = pMsgDelInfo->pChunk[i];
            if ((pMsgDelChunk == NULL) || (pMsgDelChunk->slotsInUse == 0))
            {
                continue;
            }

            iecsMessageDeliveryReference_t *pMsgDelRef = pMsgDelChunk->Slot;
            for (uint32_t j = 0; j < mdrChunkSize; j++, pMsgDelRef++)
            {
                if ((pMsgDelRef->fSlotInUse) && (!pMsgDelRef->fSlotPending))
                {
                    // Can only cleanup messages which have a queue and node handle set (i.e. are multiConsumer)
                    if (pMsgDelRef->hNode == NULL || pMsgDelRef->hQueue == NULL) continue;

                    if (arraypos >= maxInflightMsgs)
                    {
                        //More messages in flight than should be allowed
                        ieutTRACE_FFDC(ieutPROBE_001, true,
                                       "too many messages in flight", ISMRC_Error,
                                       "DeliveryArray", DeliveryArray, sizeof(DeliveryArray),
                                       NULL);
                    }
                    DeliveryArray[arraypos].Parts.Q = pMsgDelRef->hQueue;       /* BEAM suppression: accessing beyond memory */
                    DeliveryArray[arraypos].Parts.Node = pMsgDelRef->hNode;     /* BEAM suppression: accessing beyond memory */

                    arraypos++;
                }
            }
        }
    }
    iecs_unlockMessageDeliveryInfo(pMsgDelInfo);

    int32_t rc = ism_engine_confirmMessageDeliveryBatch( pSession
                                                       , NULL
                                                       , arraypos
                                                       , (ismEngine_DeliveryHandle_t *)DeliveryArray
                                                       , ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP
                                                       , &pSession, sizeof(pSession)
                                                       , iecs_sessionCleanupCompleted );

    // TODO: Need to handle the possibility that this function will now go async
    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "ism_engine_confirmMessageDeliveryBatch failed", rc,
                       "pSession",  pSession,  sizeof(pSession),
                       NULL );
    }
    return rc;
}

int32_t iecs_incrementDurableObjectCount(ieutThreadData_t *pThreadData,
                                         ismEngine_ClientState_t *pClient)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pClient,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pClient %p\n",  __func__, pClient);

    iecs_lockClientState(pClient);

    if (pClient->hStoreCSR == ismSTORE_NULL_HANDLE)
    {
        // Don't attempt to write anything to the store during recovery or for shared client states
        if ((ismEngine_serverGlobal.runPhase == EnginePhaseRecovery) ||
            (pClient->protocolId == PROTOCOL_ID_SHARED))
        {
            // This is only interesting, if it is not a shared client state (which are never written to the store)
            ieutTRACEL(pThreadData, pClient, (pClient->protocolId == PROTOCOL_ID_SHARED) ? ENGINE_HIGH_TRACE : ENGINE_INTERESTING_TRACE,
                       FUNCTION_IDENT "Client State for client %s not stored [phase %d]\n",
                       __func__, pClient->pClientId, (int)ismEngine_serverGlobal.runPhase);
        }
        else
        {
            rc = iecs_storeClientState(pThreadData,
                                       pClient,
                                       false,
                                       NULL); // non-async for now
            assert(rc == OK);
            assert(pClient->hStoreCSR != ismSTORE_NULL_HANDLE);
        }
    }

    pClient->durableObjects++;

    iecs_unlockClientState(pClient);

    ieutTRACEL(pThreadData, pClient->durableObjects,  ENGINE_FNC_TRACE, FUNCTION_EXIT "count=%lu rc=%d\n", __func__, pClient->durableObjects, rc);

    return rc;
}

void iecs_decrementDurableObjectCount(ieutThreadData_t *pThreadData,
                                      ismEngine_ClientState_t *pClient)
{
    ieutTRACEL(pThreadData, pClient,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pClient %p\n",  __func__, pClient);

    iecs_lockClientState(pClient);

    assert(pClient->durableObjects > 0);

    pClient->durableObjects--;

    iecs_unlockClientState(pClient);

    ieutTRACEL(pThreadData, pClient->durableObjects,  ENGINE_FNC_TRACE, FUNCTION_EXIT "count=%lu\n", __func__, pClient->durableObjects);
}

int32_t iecs_updateDurableObjectCountForClientId(ieutThreadData_t *pThreadData,
                                                 const char *pClientId,
                                                 uint32_t creationProtocolID,
                                                 bool increment)
{
    int32_t rc = OK;
    ismEngine_ClientState_t *pClient = NULL;
    ismEngine_ClientState_t *pClientToRelease = NULL;
    uint32_t hash = calculateHash(pClientId);
    bool bRecovery = (ismEngine_serverGlobal.runPhase == EnginePhaseRecovery);

    ieutTRACEL(pThreadData, pClientId,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pClientId %s\n",  __func__, pClientId);

    // Find the client state
    if (!bRecovery) ismEngine_lockMutex(&ismEngine_serverGlobal.Mutex);

    pClient = iecs_getVictimizedClient(pThreadData,
                                       pClientId,
                                       hash);

    // No client found, add a (nondurable) client State if we're in recovery using
    // the creationProtocolID requested.
    if (pClient == NULL)
    {
        if (bRecovery)
        {
            iecsNewClientStateInfo_t clientInfo;

            clientInfo.pClientId = pClientId;
            clientInfo.protocolId = creationProtocolID;
            clientInfo.durability = iecsNonDurable;
            clientInfo.resourceSet = iecs_getResourceSet(pThreadData,
                                                         clientInfo.pClientId,
                                                         clientInfo.protocolId,
                                                         iereOP_ADD);

            rc = iecs_newClientStateRecovery(pThreadData,
                                             &clientInfo,
                                             &pClient);

            if (rc == OK)
            {
                assert(pClient->OpState == iecsOpStateZombie);

                // Treat the client state like a zombie that was connected at last shutdown
                rc = iecs_addClientStateRecovery(pThreadData, pClient);

                // Setting its last connected time to the server's last timestamp (which will
                // always be set to either the recovered timestamp, or the time at which
                // ism_engine_init was called during startup, so should never be 0).
                if (rc == OK)
                {
                    uint32_t useTimestamp = ismEngine_serverGlobal.ServerShutdownTimestamp;

                    assert(useTimestamp != 0);

                    iecs_setLCTandExpiry(pThreadData, pClient, ism_common_convertExpireToTime(useTimestamp), NULL);

                    assert(pClient->ExpiryInterval == iecsEXPIRY_INTERVAL_INFINITE);
                    assert(pClient->ExpiryTime == 0);
                }
            }
        }
        else
        {
            rc = ISMRC_NotFound;
        }
    }

    // All OK, we can now perform the update required
    if (rc == OK)
    {
        assert(pClient != NULL);

        if (increment)
        {
            rc = iecs_incrementDurableObjectCount(pThreadData, pClient);
        }
        else
        {
            iecs_decrementDurableObjectCount(pThreadData, pClient);

            //If this was a nondurable zombie and durable count has hit zero
            //there is no need to remember this client any more unless it is
            //one of the shared namespace clients (which have protocol
            // PROTOCOL_ID_SHARED)
            //Remove from table and remove from store and free it
            pthread_spin_lock(&pClient->UseCountLock);
            if( pClient->OpState == iecsOpStateZombie &&
                pClient->Durability == iecsNonDurable &&
                pClient->pThief == NULL &&
                pClient->durableObjects == 0 &&
                pClient->protocolId != PROTOCOL_ID_SHARED)
            {
                //Can't alter state in the store during recovery
                assert(!bRecovery);

                pClient->OpState = iecsOpStateZombieRemoval;
                if (pClient->ExpiryTime != 0)
                {
                    pClient->ExpiryTime = 0;
                    pThreadData->stats.zombieSetToExpireCount -= 1;
                }

                pClient->UseCount += 1;

                pClientToRelease = pClient;
            }
            pthread_spin_unlock(&pClient->UseCountLock);
        }
    }

    if (!bRecovery) ismEngine_unlockMutex(&ismEngine_serverGlobal.Mutex);

    if (pClientToRelease != NULL)
    {
        iecs_releaseClientStateReference(pThreadData, pClientToRelease, false, false);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}


int32_t iecs_iterateMessageDeliveryInfo( ieutThreadData_t * pThreadData
                                       , iecsMessageDeliveryInfoHandle_t  hMsgDelInfo
                                       , iecs_iterateDlvyInfoCB_t dvlyInfoCB
                                       , void *context)
{
    int32_t rc = OK;

    iecsMessageDeliveryInfo_t *pMsgDelInfo = (iecsMessageDeliveryInfo_t *)hMsgDelInfo;


    // Lock the message delivery info structure
    iecs_lockMessageDeliveryInfo(pMsgDelInfo);

    if (pMsgDelInfo->NumDeliveryIds > 0)
    {
        uint32_t mdrChunkCount   = pMsgDelInfo->MdrChunkCount;
        uint32_t mdrChunkSize    = pMsgDelInfo->MdrChunkSize;

        // Build an array of all the MDRs
        for (uint32_t i = 0; i < mdrChunkCount && rc == OK; i++)
        {
            iecsMessageDeliveryChunk_t *pMsgDelChunk = pMsgDelInfo->pChunk[i];
            if ((pMsgDelChunk == NULL) || (pMsgDelChunk->slotsInUse == 0))
            {
                continue;
            }

            iecsMessageDeliveryReference_t *pMsgDelRef = pMsgDelChunk->Slot;
            for (uint32_t j = 0; j < mdrChunkSize && rc == OK; j++, pMsgDelRef++)
            {
                if ((pMsgDelRef->fSlotInUse) && (!pMsgDelRef->fSlotPending))
                {
                    rc = dvlyInfoCB( pThreadData
                                   , pMsgDelRef->hQueue
                                   , pMsgDelRef->hNode
                                   , context);
                }
            }
        }
    }
    iecs_unlockMessageDeliveryInfo(pMsgDelInfo);

    return rc;
}

int32_t iecs_importMessageDeliveryReference( ieutThreadData_t * pThreadData
                                           , ismEngine_ClientState_t *pClient
                                           , uint64_t dataId
                                           , uint32_t deliveryId
                                           , ismStore_Handle_t hStoreRecord
                                           , ismStore_Handle_t hStoreMessage
                                           , ismQHandle_t hQueue
                                           , void *hNode)
{
    int32_t rc = OK;

    iecsMessageDeliveryInfo_t *pMsgDelInfo;
    iecsMessageDeliveryReference_t *pMsgDelRef = NULL;

    uint8_t mdrState;
    bool fStoredMDR = false;
    bool fLocked = false;
    bool fAcquiredMsgDlvyInfo = false;

    ieutTRACEL(pThreadData, deliveryId, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "(pClient %p, deliveryId %u Record 0x%lx Msg 0x%lx)\n",
               __func__, pClient, deliveryId, hStoreRecord, hStoreMessage);


    rc = iecs_acquireMessageDeliveryInfoReference( pThreadData
                                                 , pClient
                                                 , &pMsgDelInfo);

    if (UNLIKELY(rc != OK))
    {
        goto mod_exit;
    }
    fAcquiredMsgDlvyInfo = true;

    iecs_lockMessageDeliveryInfo(pMsgDelInfo);
    fLocked = true;


    mdrState = iestMDR_STATE_OWNER_IS_SUBSC; //All MDRs are on subs st the mo

    rc = iecs_restoreInMemMessageDeliveryReference( pThreadData
                                                  , pClient
                                                  , deliveryId
                                                  , mdrState
                                                  , false
                                                  , false
                                                  , &pMsgDelRef);
    if (UNLIKELY(rc != OK))
    {
        ieutTRACEL(pThreadData, mdrState, ENGINE_ERROR_TRACE,
                "Unable to import message-delivery reference (import id: %lu) %d\n",
                dataId, rc);
        goto mod_exit;
    }

    // Only want to store the queue handle for shared queues in the mdr table
    if (hQueue == NULL || ieq_getQType(hQueue) != multiConsumer)
    {
        hQueue = NULL;
        hNode = NULL;
    }

    assert(pMsgDelRef != NULL);

    pMsgDelRef->hQueue = hQueue;
    pMsgDelRef->hNode  = hNode;

    assert(deliveryId != 0);

    rc = iecs_writeMessageDeliveryReference( pThreadData
                                           , pMsgDelInfo
                                           , pMsgDelRef
                                           , hStoreRecord
                                           , hStoreMessage
                                           , deliveryId
                                           , &fLocked
                                           , &fStoredMDR);

    if (rc != OK)
    {
        goto mod_exit;
    }

mod_exit:
    if (fLocked)
    {
        iecs_unlockMessageDeliveryInfo(pMsgDelInfo);
    }

    if (fAcquiredMsgDlvyInfo)
    {
        iecs_releaseMessageDeliveryInfoReference(pThreadData, pMsgDelInfo);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}


/*********************************************************************/
/*                                                                   */
/* End of clientState.c                                              */
/*                                                                   */
/*********************************************************************/
