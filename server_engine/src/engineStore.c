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
/// @file  engineStore.c
/// @brief Engine functions for using the Store
//*********************************************************************
#define TRACE_COMP Engine

#include <stdio.h>
#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "engineStore.h"
#include "topicTree.h"
#include "transaction.h"
#include "memHandler.h"
#include "policyInfo.h"        // iepiPolicyInfo_t
#include "topicTreeInternal.h" // iettSubsNode_t
#include "remoteServers.h"     // iers functions & constants
#include "engineAsync.h"       // for dealing with async store commits
#include "engineTraceDump.h"   // for dumping in memory trace
#include "resourceSetStats.h"  // iere functions & constants

#ifdef USEFAKE_ASYNC_COMMIT
#include "fakeAsyncStore.h"
#endif


void iest_setupPersistedMsgData(
                        ieutThreadData_t *pThreadData
                      , ismEngine_Message_t *pMsg
                      , iestMessageRecord_t *pMsgRecord
                      , iestMessageHdrArea_t *pMsgHdrArea
                      , uint32_t *pTotalDataLength
                      , char *Frags[ismMESSAGE_AREA_COUNT + 2]
                      , uint32_t FragLengths[ismMESSAGE_AREA_COUNT + 2])

{
    iestMessageHdrArea_t DefaultMsgHdrArea = { iestMESSAGE_HEADER_AREA_STRUCID_ARRAY
                                             , iestMHA_CURRENT_VERSION };
    iestMessageRecord_t DefaultMsgRecord = { iestMESSAGE_RECORD_STRUCID_ARRAY
                                           , iestMR_CURRENT_VERSION
                                           , pMsg->AreaCount + 1 };

    *pMsgRecord  = DefaultMsgRecord;
    *pMsgHdrArea = DefaultMsgHdrArea;

    FragLengths[0] = sizeof(*pMsgRecord);
    Frags[0] = (char *)pMsgRecord;
    *pTotalDataLength = sizeof(*pMsgRecord);

    // Message header fields
    pMsgHdrArea->Persistence = pMsg->Header.Persistence;
    pMsgHdrArea->Reliability = pMsg->Header.Reliability;
    pMsgHdrArea->Priority = pMsg->Header.Priority;
    pMsgHdrArea->Flags = pMsg->Header.Flags;
    pMsgHdrArea->Expiry = pMsg->Header.Expiry;
    pMsgHdrArea->MessageType = pMsg->Header.MessageType;

    pMsgRecord->AreaTypes[0] = ismMESSAGE_AREA_INTERNAL_HEADER;
    pMsgRecord->AreaLen[0] =  sizeof(*pMsgHdrArea);
    FragLengths[1] = sizeof(*pMsgHdrArea);
    Frags[1] = (char *)pMsgHdrArea;
    *pTotalDataLength += sizeof(*pMsgHdrArea);

    // And copy in the user areas
    for (uint32_t i=0; i < pMsg->AreaCount; i++)
    {
        *pTotalDataLength += pMsg->AreaLengths[i];
        FragLengths[i+2] = pMsg->AreaLengths[i];
        Frags[i+2] = pMsg->pAreaData[i];

        pMsgRecord->AreaTypes[i+1] = pMsg->AreaTypes[i];
        pMsgRecord->AreaLen[i+1] = pMsg->AreaLengths[i];
    }
    assert(*pTotalDataLength == iest_MessageStoreDataLength(pMsg));

}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Puts a message in the store 
///  @remarks
///    This function is called to put a message in the store. If the message
///    is already in the store, then the reference count to the store 
///    message is incremented by the requested amount (normally 1).
///
///    In many cases, iest_storeMessage will only be being called at a time
///    when it is not possible for other threads to be calling iest_storeMessage
///    or iest_unstoreMessage for the same message - when this is so, we don't
///    want to use a memory barrier to alter the reference count value.
///
///    When it is possible that other threads will be either storing or unstoring
///    the same message (at time of writing, this is the case for a retained msg
///    being delivered to new subscribers) we need to make sure the count is
///    updated atomically.
///  @par
///    This function may call ism_storeCommit() or ism_storeRollback so 
///    should not be called with a pending store transaction.
///  @param[in] hStream             - Store stream
///  @param[in] pMsg                - Message
///  @param[in] refCountIncrement   - Amount to increment the refCount (normally 1)
///  @param[in] pTran               - Transaction
///  @param[in] options             - iestStoreMessageOptions_t values
///  @param[out] phStoreMsg         - Message Reference
///  @return                        - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iest_storeMessage( ieutThreadData_t *pThreadData
                         , ismEngine_Message_t *pMsg
                         , uint64_t refCountIncrement
                         , iestStoreMessageOptions_t options
                         , ismStore_Handle_t *phStoreMsg )
{
    int32_t rc = OK;
    ismStore_Handle_t hStoreMsg;
    ismStore_Record_t storeRecord;

    ieutTRACEL(pThreadData, pMsg, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(pMsg->Header.Persistence != ismMESSAGE_PERSISTENCE_NONPERSISTENT);
    assert((pMsg->Header.Flags & ismMESSAGE_FLAGS_RETAINED) == 0);

    uint64_t NewRefCount;

    if (pMsg->StoreMsg.parts.RefCount == 0)
    {
        // It's up to us to add the message to the store, so attempt to
        // add it to the store.
        char *Frags[ismMESSAGE_AREA_COUNT + 2];
        uint32_t FragLengths[ismMESSAGE_AREA_COUNT + 2];
        uint32_t StoreRecordLength;

        iestMessageHdrArea_t MsgHdrArea;
        iestMessageRecord_t MsgRecord;

        iest_setupPersistedMsgData( pThreadData
                                  , pMsg
                                  , &MsgRecord
                                  , &MsgHdrArea
                                  , &StoreRecordLength
                                  , Frags
                                  , FragLengths);

        // Add the message to the store
        // We can do this before we take the queue put lock, as it is only
        // the message reference which we must add to the store in the 
        // context of the put lock in order to preserve message order.
        storeRecord.Type = ISM_STORE_RECTYPE_MSG; 
        storeRecord.FragsCount = pMsg->AreaCount + 2;
        storeRecord.pFrags = Frags;
        storeRecord.pFragsLengths = FragLengths;
        storeRecord.DataLength = StoreRecordLength;
        storeRecord.Attribute = 0; // Unused
        storeRecord.State = 0; // Unused
        do
        {
            rc = ism_store_createRecord( pThreadData->hStream
                                       , &storeRecord
                                       , &hStoreMsg);
            if (rc == ISMRC_StoreGenerationFull)
            { 
                if ((options & iestStoreMessageOptions_EXISTING_TRANSACTION) == 0)
                {
                    iest_store_rollback(pThreadData, false);
                }
                else
                {
                    if (pThreadData->ReservationState == Active)
                    {
                        ieutTRACE_FFDC( ieutPROBE_001, true
                                      , "Failed to store message even though I had reserved space!", rc
                                      , NULL );
                    }

                    // There was an existing store transaction, so the caller must handle
                    // the ISMRC_StoreGenerationFull appropriately.
                    break;
                }
            }
 
        } while (rc == ISMRC_StoreGenerationFull);

        if (rc != OK)
        {
            ism_common_setError(rc);
            goto mod_exit;
        }

        assert(hStoreMsg != ismSTORE_NULL_HANDLE);

        // Set the Store handle and store refrence count in the message
        if ((options & iestStoreMessageOptions_ATOMIC_REFCOUNTING) == 0)
        {
            pMsg->StoreMsg.parts.RefCount = refCountIncrement;
        }
        else
        {
            (void)__sync_lock_test_and_set(&(pMsg->StoreMsg.parts.RefCount), refCountIncrement);
        }

        NewRefCount = refCountIncrement;

        pMsg->StoreMsg.parts.hStoreMsg = hStoreMsg;
        *phStoreMsg = hStoreMsg;

        if ((options & iestStoreMessageOptions_EXISTING_TRANSACTION) == 0)
        {
            iest_store_commit(pThreadData, false);
        }
    }
    else
    {
        if ((options & iestStoreMessageOptions_ATOMIC_REFCOUNTING) == 0)
        {
            pMsg->StoreMsg.parts.RefCount += refCountIncrement;
            NewRefCount = pMsg->StoreMsg.parts.RefCount;
        }
        else
        {
            NewRefCount = __sync_add_and_fetch(&(pMsg->StoreMsg.parts.RefCount), refCountIncrement);
        }

        *phStoreMsg = pMsg->StoreMsg.parts.hStoreMsg;
    }

mod_exit:
    if (rc == OK)
    {
        if (NewRefCount == refCountIncrement)
        {
            ieutTRACEL(pThreadData, *phStoreMsg, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "Created message 0x%0lx in store\n",
                       __func__, *phStoreMsg);
        }
        else
        {
            ieutTRACEL(pThreadData, NewRefCount, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "Incremented use count by %lu to %lu for message 0x%0lx\n",
                       __func__, refCountIncrement, NewRefCount, *phStoreMsg);
        }
    }
    else
    {
        ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "failed to increment by %lu with rc=%d\n",
                   __func__, refCountIncrement, rc);
    }
    return rc;
}

//If we've run out of space to remember messages to delete at some point, do it here
int32_t iest_checkLazyMessages( ieutThreadData_t *pThreadData
                              , ismEngine_AsyncData_t *asyncInfo)
{
    int32_t rc = OK;

    if (pThreadData->numLazyMsgs == ieutMAXLAZYMSGS)
    {
        //We've built up loads of messages that need to be removed.
        //Do it now
        rc = iead_store_asyncCommit(pThreadData, false, asyncInfo);
        assert (rc == ISMRC_OK || rc == ISMRC_AsyncCompletion);
    }

    return rc;
}

/// Delete a bunch of messages store handles returned from iest_unstoreMessage()
int32_t iest_finishUnstoreMessages( ieutThreadData_t *pThreadData
                                  , ismEngine_AsyncData_t *asyncInfo
                                  , size_t numStoreHandles
                                  , ismStore_Handle_t hHandleToUnstore[])
{
    int32_t rc = OK;

    for (size_t i=0; i < numStoreHandles; i++)
    {
        ismStore_Handle_t hStoreMsg = hHandleToUnstore[i];
        ieutTRACEL(pThreadData, hStoreMsg, ENGINE_FNC_TRACE, FUNCTION_EXIT "Removing message 0x%0lx from store\n",
                __func__, hStoreMsg);

        rc = ism_store_deleteRecord(pThreadData->hStream, hStoreMsg);
        if (rc != OK)
        {
            ieutTRACE_FFDC(ieutPROBE_001, true, "ism_store_deleteRecord failed! failed.", rc, NULL);
        }
    }

    rc = iead_store_asyncCommit(pThreadData, false, asyncInfo);
    assert (rc == ISMRC_OK || rc == ISMRC_AsyncCompletion);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Releases the reference to a message in the store. 
///  @remarks
///    This function is called to decrement the reference count to a
///    message in the store. If the reference count reaches zero the
///    message needs to be removed from the store.
///    @par
///    The message can be removed from the store in one of 3 ways:
///     1) If useLazyRemoval is true, it is added to a small per-thread list
///        of messages to remove next time we do a store commit... it is the callers
///        responsibility to call iest_checkLazyMessages() (or iest_store_(async)Commit() )
///        before next use of lazyRemoval to ensure the per-thread list does not fill up.
///     2) If useLazyRemoval=false & phHandleToUnstore != NULL,
///        *phHandleToUnstore will be set to the message to unstore. It is the callers
///        responsibility to call iest_finishUnstoreMessage() with all relevant msgs
///     3) Otherwise the delete is added to the current store transaction.
///        It is the responsibility of the caller to call ism_store_commit. If
///        caller instead calls ism_store_rollback, then there is a risk that
///        a message will be orphaned in the store.
///
///   NB: All 3 ways of removing messages require the caller to do something!
///
///  @param[in] pMsg               - Message
///  @param[in] storeSameTran      - Was the original store of this message
///                                  part of the current store transaction?
///  @param[in] useLazyRemoval     - Should we remember this message for later deletion
///                                  rather than put it in an store transaction immediately
///  @param[out] pStoreOpCount     - If non-NULL on entry,
///                                  It is increment for each store update
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iest_unstoreMessage( ieutThreadData_t *pThreadData
                           , ismEngine_Message_t *pMsg
                           , bool storeSameTran
                           , bool useLazyRemoval
                           , ismStore_Handle_t *phHandleToUnstore
                           , uint32_t *pStoreOpCount)
{
    int32_t rc = OK;
    uint64_t NewRefCount = 0;
    union ismEngine_StoreMsg_t StoreMsg;
    union ismEngine_StoreMsg_t NewStoreMsg;

    ieutTRACEL(pThreadData, pMsg, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

Retry:
    StoreMsg = pMsg->StoreMsg;
    assert(StoreMsg.parts.hStoreMsg != ismSTORE_NULL_HANDLE);

    if (StoreMsg.parts.RefCount == 1)
    {
        NewStoreMsg.parts.RefCount = 0;
        NewStoreMsg.parts.hStoreMsg = ismSTORE_NULL_HANDLE;

        if (!__sync_bool_compare_and_swap(&(pMsg->StoreMsg.whole), StoreMsg.whole, NewStoreMsg.whole))
        {
            // Someone has incremented the use count between us checking
            // and decrementing the value so retry:
            goto Retry;
        }

        // We have set the refcount to zero so remove the message from
        // the store....unless it was added in the same UOW...
        if(!storeSameTran)
        {
            if (useLazyRemoval)
            {
                if (pThreadData->numLazyMsgs < ieutMAXLAZYMSGS)
                {
                    pThreadData->hMsgForLazyRemoval[pThreadData->numLazyMsgs] = StoreMsg.parts.hStoreMsg;
                    pThreadData->numLazyMsgs++;
                }
                else
                {
                    ieutTRACE_FFDC(ieutPROBE_001, true, "tried to lazily unstore a message when slots were all full.", ISMRC_Error,
                                   "StoreMsg",  &StoreMsg, sizeof(StoreMsg),
                                   "pThreadData", pThreadData, sizeof(ieutThreadData_t),
                                   NULL);
                }
            }
            else if(phHandleToUnstore != NULL)
            {
                *phHandleToUnstore = StoreMsg.parts.hStoreMsg;
            }
            else
            {
                rc = ism_store_deleteRecord(pThreadData->hStream, StoreMsg.parts.hStoreMsg);
                if (rc != OK)
                {
                    ieutTRACE_FFDC(ieutPROBE_002, true, "ism_store_deleteRecord failed! failed.", rc, NULL);
                }

                //We've updated the store, need a store commit
                if (pStoreOpCount)
                {
                    (*pStoreOpCount)++;
                }
            }
        }
        else
        {
            ieutTRACEL(pThreadData, 0, ENGINE_NORMAL_TRACE, "Not unstoring message as it was created as part of this store transaction\n");
        }
    }
    else
    {
        NewRefCount = NewStoreMsg.parts.RefCount = StoreMsg.parts.RefCount-1;
        assert(NewRefCount != 0);
        NewStoreMsg.parts.hStoreMsg = StoreMsg.parts.hStoreMsg;
        if (!__sync_bool_compare_and_swap(&(pMsg->StoreMsg.whole), StoreMsg.whole, NewStoreMsg.whole))
        {
            // The refcount changed since we checked the value so we must
            // retry:
            goto Retry;
        }
    }

    if (rc == OK)
    {
        ismStore_Handle_t hStoreMsg = StoreMsg.parts.hStoreMsg;

        if (NewRefCount == 0)
        {
            if (useLazyRemoval)
            {
                ieutTRACEL(pThreadData, hStoreMsg, ENGINE_FNC_TRACE, FUNCTION_EXIT "Recorded message 0x%0lx for lazy removal\n",
                           __func__, hStoreMsg);
            }
            else if(phHandleToUnstore != NULL)
            {
                ieutTRACEL(pThreadData, hStoreMsg, ENGINE_FNC_TRACE, FUNCTION_EXIT "Returning message 0x%0lx for caller to remove\n",
                           __func__, hStoreMsg);
            }
            else
            {
                ieutTRACEL(pThreadData, hStoreMsg, ENGINE_FNC_TRACE, FUNCTION_EXIT "Removed message 0x%0lx from store\n",
                           __func__, hStoreMsg);
            }
        }
        else
        {
            ieutTRACEL(pThreadData, NewRefCount, ENGINE_FNC_TRACE, FUNCTION_EXIT "Decremented use count to %lu for message 0x%0lx\n",
                       __func__, NewRefCount, hStoreMsg);
        }
    }
    else
    {
        ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "failed with rc=%d. message handle = 0x%0lx\n", __func__, rc, StoreMsg.parts.hStoreMsg);
    }
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Releases the reference to a message in the store. 
///  @remarks
///    This function is called to decrement the reference count to a
///    message in the store. If the reference count reaches zero the
///    message will be removed from the store.
///    This function will call store_commit if the message is deleted
///    from the store or if StoreOpCount is non zero on entry.
///    @par
///    It is the responsibility of the caller to call ism_store_commit. If
///    caller instead calls ism_store_rollback, then there is a risk that 
///    a message will be orphaned in the store. If this is not possible 
///    then perhaps we should change this function to always call ism_storeCommit
///    and make a transactional version of this function for users who need
///    more sophistication.
///
///  @param[in] pMsg               - Message      
///  @param[in] StoreOpCount       - Number of pending store operations
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
void iest_unstoreMessageCommit( ieutThreadData_t *pThreadData
                              , ismEngine_Message_t *pMsg
                              , uint32_t StoreOpCount)
{
    int32_t rc = OK;
    uint64_t NewRefCount = 0;
    union ismEngine_StoreMsg_t StoreMsg;
    union ismEngine_StoreMsg_t NewStoreMsg;

    ieutTRACEL(pThreadData, pMsg, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

Retry:
    StoreMsg = pMsg->StoreMsg;
    assert(StoreMsg.parts.hStoreMsg != ismSTORE_NULL_HANDLE);

    if (StoreMsg.parts.RefCount == 1)
    {
        // We are removing the last reference to the message in the store
        // so it is our responsibility to unstore it.
        pMsg->StoreMsg.parts.RefCount = 0;
        pMsg->StoreMsg.parts.hStoreMsg = ismSTORE_NULL_HANDLE;

        // We have set the refcount to zero so remove the message from
        // the store.
        rc = ism_store_deleteRecord(pThreadData->hStream, StoreMsg.parts.hStoreMsg);
        if (rc != OK)
        {
            ieutTRACE_FFDC(ieutPROBE_001, true, "ism_store_deleteRecord failed! failed.", rc, NULL);
        }

        iest_store_commit( pThreadData, false);

        ieutTRACEL(pThreadData, StoreMsg.parts.hStoreMsg, ENGINE_FNC_TRACE, FUNCTION_EXIT "Removed message 0x%0lx from store\n",
                   __func__, StoreMsg.parts.hStoreMsg);
    }
    else
    {
        // We are not removing the last reference to the message so we will
        // just decrement the use count but in order to ensure our reference 
        // is deleted from the store before the message we must commit before
        // we do our decrement.
        if (StoreOpCount > 0)
        {
            iest_store_commit( pThreadData, false);
            StoreOpCount = 0;
        }

        NewRefCount = NewStoreMsg.parts.RefCount = StoreMsg.parts.RefCount-1;
        assert(NewRefCount != 0);
        NewStoreMsg.parts.hStoreMsg = StoreMsg.parts.hStoreMsg;
        if (!__sync_bool_compare_and_swap(&(pMsg->StoreMsg.whole), StoreMsg.whole, NewStoreMsg.whole))
        {
            // The refcount changed since we checked the value so we must
            // retry:
            goto Retry;
        }

        // We have succesfully decremented the use count so we are done.
        ieutTRACEL(pThreadData, NewRefCount, ENGINE_FNC_TRACE, FUNCTION_EXIT "Decremented use count to %lu for message 0x%0lx\n",
                   __func__, NewRefCount, StoreMsg.parts.hStoreMsg);
    }

    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Rebuilds a message from the store
///  @remarks
///    This function is called to rebuild a message from the store. 
///  @param[in] recHandle          - Store handle 
///  @param[in] record             - Store record 
///  @param[in] transData          - Transaction data (unused)
///  @param[out] outData           - Ouptut message
///  @param[in] pinMessage         - Original message header
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iest_rehydrateMessage( ieutThreadData_t *pThreadData
                             , ismStore_Handle_t recHandle
                             , ismStore_Record_t *record
                             , ismEngine_RestartTransactionData_t *transData
                             , void **outData
                             , void *pinMessage)
{
    int32_t rc = OK;
    char *pData = (char *)(record->pFrags[0]);
    iestMessageRecord_t *pMsgRecord;
    iestMessageHdrArea_t *pMsgHdrArea;
    ismEngine_Message_t *pMessage = NULL;
    char *pMsgData = NULL;
    size_t MsgLength = record->DataLength;
    uint8_t Flags;
    uint32_t i;

    ieutTRACEL(pThreadData, recHandle, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // First check that we have been given only 1 fragment
    assert(record->FragsCount == 1);

    pMsgRecord = (iestMessageRecord_t *)(pData);

    if (LIKELY(pMsgRecord->Version == iestMR_CURRENT_VERSION))
    {
        pData += sizeof(iestMessageRecord_t);

        assert(MsgLength >= sizeof(iestMessageRecord_t));
        MsgLength -= sizeof(iestMessageRecord_t);
    }
    else
    {
        rc = ISMRC_InvalidValue;
        ism_common_setErrorData(rc, "%u", pMsgRecord->Version);
        goto mod_exit;
    }

    pMsgHdrArea = (iestMessageHdrArea_t *)(pData);

    if (LIKELY(pMsgHdrArea->Version == iestMHA_CURRENT_VERSION))
    {
        pData += sizeof(iestMessageHdrArea_t);

        assert(MsgLength >= sizeof(iestMessageHdrArea_t));
        MsgLength -= sizeof(iestMessageHdrArea_t);
    }
    else
    {
        rc = ISMRC_InvalidValue;
        ism_common_setErrorData(rc, "%u", pMsgHdrArea->Version);
        goto mod_exit;
    }

    if (pinMessage == NULL)
    {
        pMessage = (ismEngine_Message_t *)iere_malloc(pThreadData,
                                                      iereNO_RESOURCE_SET,
                                                      IEMEM_PROBE(iemem_messageBody, 2), sizeof(ismEngine_Message_t) + MsgLength);
        pMsgData = (char *)(pMessage+1);
        Flags = ismENGINE_MSGFLAGS_NONE;
    }
    else
    {
        pMessage = (ismEngine_Message_t *)pinMessage;
        pMsgData = (void *)iemem_malloc(pThreadData, IEMEM_PROBE(iemem_messageBody, 4), MsgLength);
        Flags = pMessage->Flags;
        assert((Flags & ismENGINE_MSGFLAGS_ALLOCTYPE_1) == ismENGINE_MSGFLAGS_ALLOCTYPE_1);
    }

    if ((pMessage != NULL) && (pMsgData != NULL))
    {
        ismEngine_SetStructId(pMessage->StrucId, ismENGINE_MESSAGE_STRUCID);
        pMessage->MsgLength = MsgLength;
        pMessage->resourceSet = iereNO_RESOURCE_SET;
        pMessage->fullMemSize = (int64_t)iere_full_size(iemem_messageBody, pMessage);

        assert(record->DataLength == iest_MessageStoreDataLength(pMessage));

        if (pinMessage == NULL)
        {
            pMessage->usageCount = 0;
            pMessage->StoreMsg.parts.RefCount = 0;
            pMessage->StoreMsg.parts.hStoreMsg = recHandle;
        }
        pMessage->Flags = Flags;
        pMessage->AreaCount = pMsgRecord->AreaCount-1;
        pMessage->Header.Persistence = pMsgHdrArea->Persistence;
        pMessage->Header.Reliability = pMsgHdrArea->Reliability;
        pMessage->Header.Priority = pMsgHdrArea->Priority;
        pMessage->Header.RedeliveryCount = 0;
        pMessage->Header.Expiry = pMsgHdrArea->Expiry;
        pMessage->Header.Flags = pMsgHdrArea->Flags;
        pMessage->Header.MessageType = pMsgHdrArea->MessageType;

        memcpy(pMsgData, pData, MsgLength);

        for (i = 0; i < pMessage->AreaCount; i++)
        {
            pMessage->AreaTypes[i]   = pMsgRecord->AreaTypes[i+1];
            pMessage->AreaLengths[i] = pMsgRecord->AreaLen[i+1];
            if (pMsgRecord->AreaLen[i+1])
            {
                pMessage->pAreaData[i] = pMsgData;
            }
            else
            {
                pMessage->pAreaData[i] = NULL;
            }
            pMsgData += pMsgRecord->AreaLen[i+1];
        }

        *outData = (void *)pMessage;
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Rehydrate the storeMessage operation during restart 
///  @remarks
///    This function is called during restart processing of a message 
///    reference to update the StoreMsgRefCount correctly.
///  @param[in] pMsg               - Message      
///  @param[in] pTran              - Transaction  
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iest_rehydrateMessageRef( ieutThreadData_t *pThreadData, ismEngine_Message_t *pMsg )
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, pMsg, ENGINE_FNC_TRACE, FUNCTION_IDENT "\n", __func__);

    assert(pMsg->Header.Persistence != ismMESSAGE_PERSISTENCE_NONPERSISTENT);

    pMsg->StoreMsg.parts.RefCount++;

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Fill in a store record and SPR for the given subscription properties
///
/// @param[in]     pSPR                 Subscription properties record to update
/// @param[in]     pStoreRecord         Pointer to the store record to update
/// @param[in]     pPolicyInfo          Policy Info being used for this subscription
/// @param[in]     pSubscription        Pointer to the subscription
/// @param[in]     pTopicString         The topic string on which subscribed
/// @param[in]     clientIdLength       Length of the clientId (including null)
/// @param[in]     subNameLength        Length of the subName (including null)
/// @param[in]     topicStringLength    Length of topic string (including null)
/// @param[in]     subPropertiesLength  Length of subscription properties
/// @param[in,out] ppFragments          Fragments to pass to the store
/// @param[in,out] pFragmentLengths     Lengths to pass to the store
/// @param[in,out] pFragmentAllocated   Whether the fragment was allocated in this function
///
/// @remark ppFragments must be big enough to take up to 5 entries
///////////////////////////////////////////////////////////////////////////////
int32_t iest_prepareSPR(ieutThreadData_t *pThreadData,
                        iestSubscriptionPropertiesRecord_t *pSPR,
                        ismStore_Record_t *pStoreRecord,
                        iepiPolicyInfo_t *pPolicyInfo,
                        ismEngine_Subscription_t *pSubscription,
                        const char *pTopicString,
                        size_t clientIdLength,
                        size_t subNameLength,
                        size_t topicStringLength,
                        size_t subPropertiesLength,
                        char **ppFragments,
                        uint32_t *pFragmentLengths,
                        bool *pFragmentAllocated)
{
    int32_t rc = OK;

    assert((pSubscription->subOptions & ~ismENGINE_SUBSCRIPTION_OPTION_PERSISTENT_MASK) == 0);

    // First fragment is always the SPR itself
    ppFragments[0] = (char *)pSPR;
    pFragmentLengths[0] = sizeof(*pSPR);
    pFragmentAllocated[0] = false;

    // Fill in the store record
    pStoreRecord->Type = ISM_STORE_RECTYPE_SPROP;
    pStoreRecord->Attribute = 0;
    pStoreRecord->State = iestSPR_STATE_NONE;
    pStoreRecord->pFrags = ppFragments;
    pStoreRecord->pFragsLengths = pFragmentLengths;
    pStoreRecord->FragsCount = 1;
    pStoreRecord->DataLength = pFragmentLengths[0];

    // Build the subscription properties record from the various fragment sources
    memcpy(pSPR->StrucId, iestSUBSC_PROPS_RECORD_STRUCID, 4);
    pSPR->Version = iestSPR_CURRENT_VERSION;
    pSPR->SubOptions = pSubscription->subOptions;
    pSPR->SubId = pSubscription->subId;
    pSPR->InternalAttrs = (pSubscription->internalAttrs & iettSUBATTR_PERSISTENT_MASK);

    // Store the properties of the policy information for this subscription
    pSPR->MaxMessageCount = pPolicyInfo->maxMessageCount;
    pSPR->MaxMsgBehavior = (uint8_t)pPolicyInfo->maxMsgBehavior;
    pSPR->DCNEnabled = pPolicyInfo->DCNEnabled;

    pSPR->ClientIdLength = (uint32_t)clientIdLength;
    ppFragments[pStoreRecord->FragsCount] = pSubscription->clientId;
    pFragmentLengths[pStoreRecord->FragsCount] = pSPR->ClientIdLength;
    pFragmentAllocated[pStoreRecord->FragsCount] = false;
    pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
    pStoreRecord->FragsCount += 1;

    pSPR->SubNameLength = (uint32_t)subNameLength;
    ppFragments[pStoreRecord->FragsCount] = pSubscription->subName;
    pFragmentLengths[pStoreRecord->FragsCount] = pSPR->SubNameLength;
    pFragmentAllocated[pStoreRecord->FragsCount] = false;
    pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
    pStoreRecord->FragsCount += 1;

    // If the topic string is blank ("") then there is no fragment to store
    pSPR->TopicStringLength = (uint32_t)topicStringLength;
    if (pSPR->TopicStringLength != 0)
    {
        ppFragments[pStoreRecord->FragsCount] = (char *)pTopicString;
        pFragmentLengths[pStoreRecord->FragsCount] = pSPR->TopicStringLength;
        pFragmentAllocated[pStoreRecord->FragsCount] = false;
        pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
        pStoreRecord->FragsCount += 1;
    }

    pSPR->SubPropertiesLength = (uint32_t)subPropertiesLength;
    if (pSPR->SubPropertiesLength != 0)
    {
        ppFragments[pStoreRecord->FragsCount] = pSubscription->flatSubProperties;
        pFragmentLengths[pStoreRecord->FragsCount] = pSPR->SubPropertiesLength;
        pFragmentAllocated[pStoreRecord->FragsCount] = false;
        pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
        pStoreRecord->FragsCount += 1;
    }

    if (pPolicyInfo->name != NULL)
    {
        pSPR->PolicyNameLength = strlen(pPolicyInfo->name)+1;
        ppFragments[pStoreRecord->FragsCount] = pPolicyInfo->name;
        pFragmentLengths[pStoreRecord->FragsCount] = pSPR->PolicyNameLength;
        pFragmentAllocated[pStoreRecord->FragsCount] = false;
        pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
        pStoreRecord->FragsCount += 1;
    }
    else
    {
        pSPR->PolicyNameLength = 0;
    }

    pSPR->AnonymousSharers = iettNO_ANONYMOUS_SHARER;
    pSPR->SharingClientCount = 0;
    pSPR->SharingClientIdsLength = 0;

    // Shared subscriptions need to store additional information about sharers
    iettSharedSubData_t *sharedSubData = iett_getSharedSubData(pSubscription);

    if (sharedSubData != NULL)
    {
        int32_t sharingClient;
        size_t clientIdBufferSize = 0;
        uint32_t persistedSharingClients = 0;

        assert((pSubscription->subOptions & ismENGINE_SUBSCRIPTION_OPTION_SHARED) != 0);

        pSPR->AnonymousSharers = sharedSubData->anonymousSharers & iettANONYMOUS_SHARER_PERSISTENT_MASK;

        if (sharedSubData->sharingClientCount != 0)
        {
            for(sharingClient = 0; sharingClient < sharedSubData->sharingClientCount; sharingClient++)
            {
                iettSharingClient_t *thisSharingClient = &sharedSubData->sharingClients[sharingClient];

                if ((thisSharingClient->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) != 0)
                {
                    clientIdBufferSize += strlen(thisSharingClient->clientId) + 1;
                    persistedSharingClients += 1;
                }
            }
        }

        if (persistedSharingClients != 0)
        {
            assert(clientIdBufferSize != 0);

            size_t subOptionBufferSize = (size_t)persistedSharingClients * sizeof(uint32_t);

            // Allocate storage for an array of subOptions
            uint32_t *subOptionBuffer = (uint32_t *)iemem_malloc(pThreadData,
                                                                 IEMEM_PROBE(iemem_namedSubs, 1),
                                                                 subOptionBufferSize);

            if (subOptionBuffer == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            size_t subIdBufferSize = (size_t)persistedSharingClients * sizeof(ismEngine_SubId_t);

            // Allocate storage for an array of subIds
            ismEngine_SubId_t *subIdBuffer = (ismEngine_SubId_t *)iemem_malloc(pThreadData,
                                                                               IEMEM_PROBE(iemem_namedSubs, 3),
                                                                               subIdBufferSize);

            if (subIdBuffer == NULL)
            {
                iemem_free(pThreadData, iemem_namedSubs, subOptionBuffer);
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            // Allocate storage for the buffer containing clientIds
            char *clientIdBuffer = (char *)iemem_malloc(pThreadData,
                                                        IEMEM_PROBE(iemem_namedSubs, 2),
                                                        clientIdBufferSize);

            if (clientIdBuffer == NULL)
            {
                iemem_free(pThreadData, iemem_namedSubs, subIdBuffer);
                iemem_free(pThreadData, iemem_namedSubs, subOptionBuffer);
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            // Fill in the arrays
            char *tmpPtr = clientIdBuffer;
            uint32_t foundClientCount = 0;
            for(sharingClient = 0; sharingClient < sharedSubData->sharingClientCount; sharingClient++)
            {
                iettSharingClient_t *thisSharingClient = &sharedSubData->sharingClients[sharingClient];

                if ((thisSharingClient->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) != 0)
                {
                    size_t sharingClientIdLength = strlen(thisSharingClient->clientId) + 1;
                    subOptionBuffer[foundClientCount] = thisSharingClient->subOptions;
                    subIdBuffer[foundClientCount] = thisSharingClient->subId;
                    memcpy(tmpPtr, thisSharingClient->clientId, sharingClientIdLength);
                    tmpPtr += sharingClientIdLength;
                    foundClientCount++;
                }
            }
            assert(foundClientCount == persistedSharingClients);
            assert((tmpPtr - clientIdBuffer) == clientIdBufferSize);

            // Fill in the SPR and fragment arrays
            pSPR->SharingClientCount = persistedSharingClients;
            pSPR->SharingClientIdsLength = (uint64_t)clientIdBufferSize;

            ppFragments[pStoreRecord->FragsCount] = (char *)subOptionBuffer;
            pFragmentLengths[pStoreRecord->FragsCount] = subOptionBufferSize;
            pFragmentAllocated[pStoreRecord->FragsCount] = true;
            pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
            pStoreRecord->FragsCount += 1;

            ppFragments[pStoreRecord->FragsCount] = clientIdBuffer;
            pFragmentLengths[pStoreRecord->FragsCount] = clientIdBufferSize;
            pFragmentAllocated[pStoreRecord->FragsCount] = true;
            pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
            pStoreRecord->FragsCount += 1;

            ppFragments[pStoreRecord->FragsCount] = (char *)subIdBuffer;
            pFragmentLengths[pStoreRecord->FragsCount] = subIdBufferSize;
            pFragmentAllocated[pStoreRecord->FragsCount] = true;
            pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
            pStoreRecord->FragsCount += 1;
        }
    }

mod_exit:

    assert(pStoreRecord->FragsCount <= iestSUBSCRIPTION_MAX_FRAGMENTS);

    if (rc != OK)
    {
        // Free any fragments that were allocated by this function
        for(uint32_t fragment = 0; fragment < pStoreRecord->FragsCount; fragment++)
        {
            if (pFragmentAllocated[fragment] == true)
            {
                iemem_free(pThreadData, iemem_namedSubs, ppFragments[fragment]);
                pFragmentAllocated[fragment] = false;
            }
        }
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Get the expected size of an SPR for the specified subscription
///
/// @param[in]    pPolicyInfo          Policy associated with the subscription
/// @param[in]    pTopicString         The topic string on which the subscription
///                                    is (or is to be).
/// @param[in]    pSubscription        Subscription
///
/// @return OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
uint64_t iest_getSPRSize(ieutThreadData_t *pThreadData,
                         iepiPolicyInfo_t *pPolicyInfo,
                         char *pTopicString,
                         ismEngine_Subscription_t *pSubscription)
{
    size_t SPRSize = 0;

    assert(pTopicString != NULL);
    assert(pSubscription->clientId != NULL);
    assert(pSubscription->subName != NULL);

    size_t topicStringLength = strlen(pTopicString);
    size_t clientIdLength = strlen(pSubscription->clientId);
    size_t subNameLength = strlen(pSubscription->subName);

    SPRSize += sizeof(iestSubscriptionPropertiesRecord_t);
    SPRSize += clientIdLength != 0 ? clientIdLength + 1 : 0;
    SPRSize += subNameLength != 0 ? subNameLength + 1 : 0;
    SPRSize += topicStringLength != 0 ? topicStringLength + 1 : 0;
    SPRSize += pSubscription->flatSubPropertiesLength;
    SPRSize += pPolicyInfo->name != NULL ? strlen(pPolicyInfo->name) + 1 : 0;

    iettSharedSubData_t *sharedSubData = iett_getSharedSubData(pSubscription);

    if (sharedSubData != NULL)
    {
        for(int32_t sharingClient = 0; sharingClient < sharedSubData->sharingClientCount; sharingClient++)
        {
            iettSharingClient_t *thisSharingClient = &sharedSubData->sharingClients[sharingClient];

            if ((thisSharingClient->subOptions & ismENGINE_SUBSCRIPTION_OPTION_DURABLE) != 0)
            {
                SPRSize += strlen(thisSharingClient->clientId) + sizeof(uint32_t) + sizeof(ismEngine_SubId_t) + 1;
            }
        }
    }

    return (uint64_t)SPRSize;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Puts a subscription definition and properties records in the store
/// @remarks
///    This function is called to write a durable subscription into the store.
///    It is the responsibility of the caller to call ism_store_commit.
///
/// @param[in]    hStream              Store stream
/// @param[in]    topicString          Topic string for the subscription
/// @param[in]    topicStringLength    Length of topic string (not including null)
/// @param[in]    subscription         Subscription from which additional values
///                                    are pulled
/// @param[in]    clientIdLength       Length of the clientId (including null)
/// @param[in]    subNameLength        Length of the subName (including null)
/// @param[in]    subContextLength     Length of subscription properties
/// @param[in]    queueType            The type of queue to use for this sub
/// @param[in]    pPolicyInfo          The policy information associated with this
///                                    subscription queue.
/// @param[out]   phStoreSubscDefn     Subscription definition store handle.
/// @param[out]   phStoreSubscProps    Subscription properties store handle
///
/// @return OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iest_storeSubscription(ieutThreadData_t         *pThreadData,
                               const char               *topicString,
                               size_t                    topicStringLength,
                               ismEngine_Subscription_t *subscription,
                               size_t                    clientIdLength,
                               size_t                    subNameLength,
                               size_t                    subPropertiesLength,
                               ismQueueType_t            queueType,
                               iepiPolicyInfo_t         *pPolicyInfo,
                               ismStore_Handle_t        *phStoreSubscDefn,
                               ismStore_Handle_t        *phStoreSubscProps)
{
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestSubscriptionDefinitionRecord_t SDR;
    iestSubscriptionPropertiesRecord_t SPR;
    char *fragment[iestSUBSCRIPTION_MAX_FRAGMENTS];
    uint32_t fragmentLength[iestSUBSCRIPTION_MAX_FRAGMENTS];
    bool fragmentAllocated[iestSUBSCRIPTION_MAX_FRAGMENTS];

    ieutTRACEL(pThreadData, subscription, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(NULL != topicString);
    assert(NULL != subscription->clientId);
    assert(NULL != subscription->subName);
    assert(subPropertiesLength == 0 || NULL != subscription->flatSubProperties);

    // Include the null terminator
    if (topicStringLength != 0)
    {
        topicStringLength += 1;
    }

    memset(fragmentAllocated, 0, sizeof(fragmentAllocated));

retry_create_subscription:

    /*******************************************************************************/
    /* Subscription Properties record is written first, so we have a handle to put */
    /* in the subscription definition.                                             */
    /*******************************************************************************/
    rc = iest_prepareSPR(pThreadData,
                         &SPR,
                         &storeRecord,
                         pPolicyInfo,
                         subscription,
                         topicString,
                         clientIdLength,
                         subNameLength,
                         topicStringLength,
                         subPropertiesLength,
                         fragment,
                         fragmentLength,
                         fragmentAllocated);

    if (rc != OK) goto mod_exit;

    assert(NULL != phStoreSubscProps);
    assert(storeRecord.DataLength == iest_getSPRSize(pThreadData,
                                                     pPolicyInfo,
                                                     (char *)topicString,
                                                     subscription));

    // Add to the store
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                phStoreSubscProps);

    // Before checking the return code, free up any fragments allocated for this request
    for(int32_t i=0; i<storeRecord.FragsCount; i++)
    {
        if (fragmentAllocated[i] == true)
        {
            iemem_free(pThreadData, iemem_namedSubs, fragment[i]);
            fragmentAllocated[i] = false;
        }
    }

    if (rc != OK)
    {
        iest_store_rollback(pThreadData, false);

        if (rc == ISMRC_StoreGenerationFull) goto retry_create_subscription;

        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: ism_store_createRecord (SPR) failed! (rc=%d)\n", __func__, rc);

        goto mod_exit;
    }

    assert(*phStoreSubscProps != ismSTORE_NULL_HANDLE);

    assert(phStoreSubscDefn != NULL);

    // Build the subscription definition record from the various fragment sources
    memcpy(SDR.StrucId, iestSUBSC_DEFN_RECORD_STRUCID, 4);
    SDR.Version = iestSDR_CURRENT_VERSION;
    SDR.Type = queueType;

    fragment[0] = (char *)&SDR;
    fragmentLength[0] = sizeof(SDR);

    // Fill in the store record and contents of the subscription record
    storeRecord.Type = ISM_STORE_RECTYPE_SUBSC;
    storeRecord.Attribute = *phStoreSubscProps;
    storeRecord.State = iestSDR_STATE_CREATING;
    storeRecord.pFrags = fragment;
    storeRecord.pFragsLengths = fragmentLength;
    storeRecord.FragsCount = 1;
    storeRecord.DataLength = fragmentLength[0];

    // Add to the store
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                phStoreSubscDefn);

    if (rc != OK)
    {
        iest_store_rollback(pThreadData, false);

        if (rc == ISMRC_StoreGenerationFull) goto retry_create_subscription;

        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: ism_store_createRecord (SDR) failed! (rc=%d)\n", __func__, rc);

        goto mod_exit;
    }

    assert(*phStoreSubscDefn != ismSTORE_NULL_HANDLE);

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Updates a subscription properties record in the store
/// @remarks
///    This function is called to write a revised SPR into the store and
///    update the SDR to reference the new SPR.
///
/// @param[in]    pPolicyInfo       The policy information associated with this
///                                 subscription queue
/// @param[in]    pSubscription     Subscription from which additional values
///                                 are pulled
/// @param[in]    hStoreSubscriptionDefn
///                                 Subscription definition store handle.
/// @param[in]    hOldStoreSubscriptionProps
///                                 Subscription properties store handle for
///                                 the record to be deleted
/// @param[out]   phNewStoreSubscriptionProps
///                                 New subscription properties (SPR) store
///                                 handle
/// @param[in]    commitUpdate      Whether we should commit store updates
///
/// @return OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iest_updateSubscription(ieutThreadData_t *pThreadData,
                                iepiPolicyInfo_t *pPolicyInfo,
                                ismEngine_Subscription_t *pSubscription,
                                ismStore_Handle_t hStoreSubscriptionDefn,
                                ismStore_Handle_t hOldStoreSubscriptionProps,
                                ismStore_Handle_t *phNewStoreSubscriptionProps,
                                bool commitUpdate)
{
    int32_t rc = OK;

    ismStore_Record_t storeRecord;
    iestSubscriptionPropertiesRecord_t SPR;

    char *pTopicString = pSubscription->node->topicString;
    char *pClientId = pSubscription->clientId;
    char *pSubName = pSubscription->subName;

    size_t topicStringLength = strlen(pTopicString);
    size_t clientIdLength = strlen(pClientId);
    size_t subNameLength = strlen(pSubName);

    size_t subPropertiesLength = pSubscription->flatSubPropertiesLength;
    char *fragment[iestSUBSCRIPTION_MAX_FRAGMENTS];
    uint32_t fragmentLength[iestSUBSCRIPTION_MAX_FRAGMENTS];
    bool fragmentAllocated[iestSUBSCRIPTION_MAX_FRAGMENTS];

    ieutTRACEL(pThreadData, pSubscription, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (topicStringLength != 0)
    {
        topicStringLength += 1;
    }
    if (clientIdLength != 0)
    {
        clientIdLength += 1;
    }
    if (subNameLength != 0)
    {
        subNameLength += 1;
    }

    memset(fragmentAllocated, 0, sizeof(fragmentAllocated));

    /*******************************************************************************/
    /* Subscription Properties record is written first, so we have a handle to put */
    /* in the subscription definition.                                             */
    /*******************************************************************************/
    rc = iest_prepareSPR(pThreadData,
                         &SPR,
                         &storeRecord,
                         pPolicyInfo,
                         pSubscription,
                         pTopicString,
                         clientIdLength,
                         subNameLength,
                         topicStringLength,
                         subPropertiesLength,
                         fragment,
                         fragmentLength,
                         fragmentAllocated);

    if (rc != OK) goto mod_exit;

    assert(storeRecord.DataLength == iest_getSPRSize(pThreadData,
                                                     pPolicyInfo,
                                                     pSubscription->node->topicString,
                                                     pSubscription));

    /* Update the subscription properties record (SPR). */
    do
    {
        rc = ism_store_createRecord(pThreadData->hStream,
                                    &storeRecord,
                                    phNewStoreSubscriptionProps);

        // If there was an existing SPR then delete it
        if (rc == OK)
        {
            assert(hOldStoreSubscriptionProps != ismSTORE_NULL_HANDLE);

            rc = ism_store_deleteRecord(pThreadData->hStream,
                                        hOldStoreSubscriptionProps);
        }

        // If there is a subscription definition, update it to point to the
        // new properties record.
        if (rc == OK && hStoreSubscriptionDefn != ismSTORE_NULL_HANDLE)
        {
            rc = ism_store_updateRecord(pThreadData->hStream,
                                        hStoreSubscriptionDefn,
                                        *phNewStoreSubscriptionProps,
                                        iestSDR_STATE_NONE,
                                        ismSTORE_SET_ATTRIBUTE);
        }

        if (rc != OK)
        {
            if (commitUpdate)
            {
                iest_store_rollback(pThreadData, false);
            }
            else
            {
                break;
            }
        }
    }
    while (rc == ISMRC_StoreGenerationFull);

    // Before checking the return code, free up any fragments allocated for this request
    for(int32_t i=0; i<storeRecord.FragsCount; i++)
    {
        if (fragmentAllocated[i] == true)
        {
            iemem_free(pThreadData, iemem_namedSubs, fragment[i]);
            fragmentAllocated[i] = false;
        }
    }

    // Any kind of error, we will have rolled back, report and return.
    if (rc != OK)
    {
        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: failed to update SPR (rc=%d)\n", __func__, rc);
        goto mod_exit;
    }

    assert(*phNewStoreSubscriptionProps != ismSTORE_NULL_HANDLE);

    // Commit the update
    if (commitUpdate)
    {
        iest_store_commit(pThreadData, false);
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Fill in a store record and QPR for the given queue properties
///
/// @param[in]     pQPR                 Queue properties record to update
/// @param[in]     pStoreRecord         Pointer to the store record to update
/// @param[in]     pQueueName           The queue name
/// @param[in,out] ppFragments          Fragments to pass to the store
/// @param[in,out] pFragmentLengths     Lengths to pass to the store
///
/// @remark ppFragments must be big enough to take up to 5 entries
///////////////////////////////////////////////////////////////////////////////
void iest_prepareQPR(iestQueuePropertiesRecord_t *pQPR,
                     ismStore_Record_t *pStoreRecord,
                     const char *pQueueName,
                     char **ppFragments,
                     uint32_t *pFragmentLengths)
{
    // First fragment is always the QPR itself
    ppFragments[0] = (char *)pQPR;
    pFragmentLengths[0] = sizeof(*pQPR);

    // Fill in the store record
    pStoreRecord->Type = ISM_STORE_RECTYPE_QPROP;
    pStoreRecord->Attribute = 0;
    pStoreRecord->State = iestQPR_STATE_NONE;
    pStoreRecord->pFrags = ppFragments;
    pStoreRecord->pFragsLengths = pFragmentLengths;
    pStoreRecord->FragsCount = 1;
    pStoreRecord->DataLength = pFragmentLengths[0];

    // Build the subscription properties record from the various fragment sources
    memcpy(pQPR->StrucId, iestQUEUE_PROPS_RECORD_STRUCID, 4);
    pQPR->Version = iestQPR_CURRENT_VERSION;

    if (pQueueName != NULL)
    {
        pQPR->QueueNameLength = strlen(pQueueName)+1;
        ppFragments[pStoreRecord->FragsCount] = (char *)pQueueName;
        pFragmentLengths[pStoreRecord->FragsCount] = pQPR->QueueNameLength;
        pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
        pStoreRecord->FragsCount += 1;
    }
    else
    {
        pQPR->QueueNameLength = 0;
    }

    assert(pStoreRecord->FragsCount <= iestQUEUE_MAX_FRAGMENTS);
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Puts a queue definition and properties record in the store
/// @remarks
///    This function is called to write a permanent queue into the store.
///    It is the responsibility of the caller to call ism_store_commit.
///
/// @param[in]    pThreadData          Thread Data
/// @param[in]    type                 Type of queue
/// @param[in]    name                 Name of queue
/// @param[out]   phStoreQueueDefn     Queue definition (QDR) store handle
/// @param[out]   phStoreQueueProps    Queue properties (QPR) store handle
///
/// @return OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iest_storeQueue(ieutThreadData_t         *pThreadData,
                        ismQueueType_t            type,
                        const char               *name,
                        ismStore_Handle_t        *phStoreQueueDefn,
                        ismStore_Handle_t        *phStoreQueueProps)
{
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestQueueDefinitionRecord_t QDR;
    iestQueuePropertiesRecord_t QPR;
    char *fragments[iestQUEUE_MAX_FRAGMENTS];
    uint32_t fragmentLengths[iestQUEUE_MAX_FRAGMENTS];

    ieutTRACEL(pThreadData, type, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

retry_create_queue:

    iest_prepareQPR(&QPR, &storeRecord,
                    name,
                    fragments, fragmentLengths);

    rc = ism_store_createRecord( pThreadData->hStream
                               , &storeRecord
                               , phStoreQueueProps );

    if (rc != OK)
    {
        iest_store_rollback(pThreadData, false);

        if (rc == ISMRC_StoreGenerationFull) goto retry_create_queue;

        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: ism_store_createRecord (QPR) failed! (rc=%d)\n", __func__, rc);

        goto mod_exit;
    }

    assert(*phStoreQueueProps != ismSTORE_NULL_HANDLE);

    memcpy(QDR.StrucId, iestQUEUE_DEFN_RECORD_STRUCID, 4);
    QDR.Version = iestQDR_CURRENT_VERSION;
    QDR.Type = type;

    fragments[0] = (char *)&QDR;
    fragmentLengths[0] = sizeof(QDR);

    storeRecord.Type = ISM_STORE_RECTYPE_QUEUE;
    storeRecord.FragsCount = 1;
    storeRecord.pFrags = fragments;
    storeRecord.pFragsLengths = fragmentLengths;
    storeRecord.DataLength = fragmentLengths[0];
    storeRecord.Attribute = *phStoreQueueProps; // Reference the properties
    storeRecord.State = iestQDR_STATE_NONE;

    rc = ism_store_createRecord( pThreadData->hStream
                               , &storeRecord
                               , phStoreQueueDefn);
    if (rc != OK)
    {
        iest_store_rollback(pThreadData, false);

        if (rc == ISMRC_StoreGenerationFull) goto retry_create_queue;

        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: ism_store_createRecord (QDR) failed! (rc=%d)\n", __func__, rc);

        goto mod_exit;
    }

    assert(*phStoreQueueDefn != ismSTORE_NULL_HANDLE);

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Update the QPR associated with an existing QDR.
/// @remarks
///    This function is called to update the properties of a a permanent queue
///    in the store - this function will call iest_store_commit to commit the update.
///
/// @param[in]    hStoreQueueDefn        Store handle of the existing QDR
/// @param[in]    hStoreQueueProps       Store handle of the existing QPR
/// @param[in]    name                   Name of queue
/// @param[out]   phNewStoreQueueProps   New Queue properties (QPR) store handle
///
/// @return OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iest_updateQueue(ieutThreadData_t    *pThreadData,
                         ismStore_Handle_t    hStoreQueueDefn,
                         ismStore_Handle_t    hStoreQueueProps,
                         const char          *name,
                         ismStore_Handle_t   *phNewStoreQueueProps)
{
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestQueuePropertiesRecord_t QPR;
    char *fragments[iestQUEUE_MAX_FRAGMENTS];
    uint32_t fragmentLengths[iestQUEUE_MAX_FRAGMENTS];

    ieutTRACEL(pThreadData, name, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(pThreadData != NULL);
    assert(hStoreQueueProps != ismSTORE_NULL_HANDLE);

    iest_prepareQPR(&QPR, &storeRecord,
                    name,
                    fragments, fragmentLengths);

    // Update the queue properties record.
    do
    {
        rc = ism_store_createRecord(pThreadData->hStream,
                                    &storeRecord,
                                    phNewStoreQueueProps);

        // If there was an existing QPR delete it
        if (rc == OK)
        {
            assert(*phNewStoreQueueProps != ismSTORE_NULL_HANDLE);

            rc = ism_store_deleteRecord(pThreadData->hStream,
                                        hStoreQueueProps);
        }

        // If there is a queue definition, update it to point to the
        // new properties record.
        if (rc == OK && hStoreQueueDefn != ismSTORE_NULL_HANDLE)
        {
            rc = ism_store_updateRecord(pThreadData->hStream,
                                        hStoreQueueDefn,
                                        *phNewStoreQueueProps,
                                        iestQDR_STATE_NONE,
                                        ismSTORE_SET_ATTRIBUTE);
        }

        if (rc != OK) iest_store_rollback(pThreadData, false);
    }
    while(rc == ISMRC_StoreGenerationFull);

    // Any kind of error, we will have rolled back, report and return.
    if (rc != OK)
    {
        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: failed to update QPR (rc=%d)\n", __func__, rc);
        goto mod_exit;
    }

    assert(*phNewStoreQueueProps != ismSTORE_NULL_HANDLE);

    // Commit the update
    iest_store_commit(pThreadData, false);

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Fill in a store record and RPR for the given remote server properties
///
/// @param[in]     pRPR                 Remote server properties record to update
/// @param[in]     pStoreRecord         Pointer to the store record to update
/// @param[in]     pRemoteServer        Remote server being stored
/// @param[in]     pClusterData         Remote server data from cluster component
/// @param[in]     ClusterDataLength    Length of server data from cluster component
/// @param[in,out] ppFragments          Fragments to pass to the store
/// @param[in,out] pFragmentLengths     Lengths to pass to the store
///
/// @remark ppFragments must be big enough to take up to 4 entries
///////////////////////////////////////////////////////////////////////////////
void iest_prepareRPR(iestRemoteServerPropertiesRecord_t *pRPR,
                     ismStore_Record_t *pStoreRecord,
                     ismEngine_RemoteServer_t *pRemoteServer,
                     void *pClusterData,
                     size_t ClusterDataLength,
                     char **ppFragments,
                     uint32_t *pFragmentLengths)
{
    // First fragment is always the RPR itself
    ppFragments[0] = (char *)pRPR;
    pFragmentLengths[0] = sizeof(*pRPR);

    // Fill in the store record
    pStoreRecord->Type = ISM_STORE_RECTYPE_RPROP;
    pStoreRecord->Attribute = 0;
    pStoreRecord->State = iestRPR_STATE_NONE;
    pStoreRecord->pFrags = ppFragments;
    pStoreRecord->pFragsLengths = pFragmentLengths;
    pStoreRecord->FragsCount = 1;
    pStoreRecord->DataLength = pFragmentLengths[0];

    // Build the properties record from the various fragment sources
    memcpy(pRPR->StrucId, iestREMSRV_PROPS_RECORD_STRUCID, 4);
    pRPR->Version = iestRPR_CURRENT_VERSION;
    pRPR->InternalAttrs = (pRemoteServer->internalAttrs & iersREMSRVATTR_PERSISTENT_MASK);

    pRPR->UIDLength = (uint32_t)(strlen(pRemoteServer->serverUID)+1);
    ppFragments[pStoreRecord->FragsCount] = pRemoteServer->serverUID;
    pFragmentLengths[pStoreRecord->FragsCount] = pRPR->UIDLength;
    pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
    pStoreRecord->FragsCount += 1;

    pRPR->NameLength = (uint32_t)(strlen(pRemoteServer->serverName)+1);
    ppFragments[pStoreRecord->FragsCount] = pRemoteServer->serverName;
    pFragmentLengths[pStoreRecord->FragsCount] = pRPR->NameLength;
    pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
    pStoreRecord->FragsCount += 1;

    // If there is cluster data, add that to the store too
    pRPR->ClusterDataLength = (uint32_t)ClusterDataLength;
    if (pRPR->ClusterDataLength != 0)
    {
        ppFragments[pStoreRecord->FragsCount] = (char *)pClusterData;
        pFragmentLengths[pStoreRecord->FragsCount] = pRPR->ClusterDataLength;
        pStoreRecord->DataLength += pFragmentLengths[pStoreRecord->FragsCount];
        pStoreRecord->FragsCount += 1;
    }

    assert(pStoreRecord->FragsCount <= iestREMOTESERVER_MAX_FRAMGENTS);

    return;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Puts a remote server definition and properties records in the store
/// @remarks
///    This function is called to write a remote server into the store.
///    It is the responsibility of the caller to call ism_store_commit.
///
///    Note that the newly stored remote server has an initial state set to
///    iestRDR_STATE_CREATING. Until this has been updated to iestRDR_STATE_NONE
///    it will be deleted and not be reported to the cluster component at restart.
///
/// @param[in]    pThreadData              ThreadData to use
/// @param[in]    remoteServer             Remote server being stored
/// @param[out]   phStoreRemoteServerDefn  Remote server definition store handle.
/// @param[out]   phStoreRemoteServerProps Remote server properties store handle
///
/// @return OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iest_storeRemoteServer(ieutThreadData_t         *pThreadData,
                               ismEngine_RemoteServer_t *remoteServer,
                               ismStore_Handle_t        *phStoreRemoteServerDefn,
                               ismStore_Handle_t        *phStoreRemoteServerProps)
{
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestRemoteServerDefinitionRecord_t RDR;
    iestRemoteServerPropertiesRecord_t RPR;
    ismStore_Handle_t hStoreDefn = ismSTORE_NULL_HANDLE;
    ismStore_Handle_t hStoreProps = ismSTORE_NULL_HANDLE;
    char *fragment[iestREMOTESERVER_MAX_FRAMGENTS];
    uint32_t fragmentLength[iestREMOTESERVER_MAX_FRAMGENTS];

    assert(remoteServer->serverName != NULL);
    assert(remoteServer->serverUID != NULL);
    assert(phStoreRemoteServerProps != NULL);
    assert(phStoreRemoteServerDefn != NULL);

    ieutTRACEL(pThreadData, remoteServer, ENGINE_FNC_TRACE, FUNCTION_ENTRY "serverName='%s', serverUID='%s'\n",
               __func__, remoteServer->serverName, remoteServer->serverUID);

retry_create_remoteServer:

    /*******************************************************************************/
    /* The Properties record is written first, so we have a handle to put into the */
    /* definition record.                                                          */
    /*******************************************************************************/
    iest_prepareRPR(&RPR,
                    &storeRecord,
                    remoteServer,
                    NULL, 0, // No cluster data supplied at initial creation
                    fragment,
                    fragmentLength);

    // Add to the store
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreProps);

    if (rc != OK)
    {
        if (rc == ISMRC_StoreGenerationFull) goto retry_create_remoteServer;

        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: ism_store_createRecord (RPR) failed! (rc=%d)\n", __func__, rc);

        goto mod_exit;
    }

    // Build the definition record from the various fragment sources
    memcpy(RDR.StrucId, iestREMSRV_DEFN_RECORD_STRUCID, 4);
    RDR.Version = iestRDR_CURRENT_VERSION;
    RDR.Local = ((remoteServer->internalAttrs & iersREMSRVATTR_LOCAL) != 0);

    fragment[0] = (char *)&RDR;
    fragmentLength[0] = sizeof(RDR);

    // Fill in the store record and contents of the subscription record
    storeRecord.Type = ISM_STORE_RECTYPE_REMSRV;
    storeRecord.Attribute = hStoreProps;
    storeRecord.State = iestRDR_STATE_CREATING;
    storeRecord.pFrags = fragment;
    storeRecord.pFragsLengths = fragmentLength;
    storeRecord.FragsCount = 1;
    storeRecord.DataLength = fragmentLength[0];

    // Add to the store
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreDefn);

    if (rc != OK)
    {
        iest_store_rollback(pThreadData, false);

        if (rc == ISMRC_StoreGenerationFull) goto retry_create_remoteServer;

        ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: ism_store_createRecord (RDR) failed! (rc=%d)\n", __func__, rc);

        goto mod_exit;
    }

    *phStoreRemoteServerProps = hStoreProps;
    *phStoreRemoteServerDefn = hStoreDefn;

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Updates an array remote server properties record in the store
/// @remarks
///    This function is called to write revised RPRs into the store and
///    update the RDRs to reference the new RPRs.
///
///    It takes arrays of objects, as opposed to a single object to enable the
///    updates to be batched together to reduce the number of store commits.
///
/// @param[in]     pThreadData         Thread data to use
/// @param[in,out] remoteServers       Array of remote servers to be stored
/// @param[in]     remoteServerCount   Count of remote servers
///
/// @return OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iest_updateRemoteServers(ieutThreadData_t *pThreadData,
                                 ismEngine_RemoteServer_t **remoteServers,
                                 int32_t remoteServerCount)
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, remoteServers, ENGINE_FNC_TRACE, FUNCTION_ENTRY "remoteServers=%p, remoteServerCount=%u\n",
               __func__, remoteServers, remoteServerCount);

    if (remoteServerCount > 0)
    {
        ismStore_Record_t storeRecord;
        iestRemoteServerPropertiesRecord_t RPR;
        char *fragment[iestREMOTESERVER_MAX_FRAMGENTS];
        uint32_t fragmentLength[iestREMOTESERVER_MAX_FRAMGENTS];
        uint32_t curServer;
        uint64_t dataLength = 0;
        DEBUG_ONLY uint64_t checkDataLength = 0;
        ismStore_Handle_t hStoreProps[remoteServerCount];

        // Calculate the data length needed for the updates
        for(curServer = 0; curServer < remoteServerCount; curServer++)
        {
            assert(remoteServers[curServer]->serverName != NULL);
            assert(remoteServers[curServer]->serverUID != NULL);
            dataLength += iest_RemoteServerStorePropertiesDataLength(remoteServers[curServer]);
        }

        // Reserve store resources
        iest_store_reserve(pThreadData,
                           dataLength,        // Calculated data length
                           remoteServerCount, // Records being ism_store_created
                           0);                // No references

        // Loop through the remote servers
        for(curServer = 0; curServer < remoteServerCount; curServer++)
        {
            ismEngine_RemoteServer_t *remoteServer = remoteServers[curServer];

            ieutTRACEL(pThreadData, remoteServer, ENGINE_FNC_TRACE, "Updating serverName='%s' serverUID='%s'\n",
                       remoteServer->serverName, remoteServer->serverUID);

            iest_prepareRPR(&RPR,
                            &storeRecord,
                            remoteServer,
                            remoteServer->clusterData,
                            remoteServer->clusterDataLength,
                            fragment,
                            fragmentLength);

            // Sanity check the dataLength value calculated
            #ifndef NDEBUG
            checkDataLength += storeRecord.DataLength;
            if (curServer == remoteServerCount-1)
            {
                assert(dataLength == checkDataLength);
            }
            #endif

            hStoreProps[curServer] = ismSTORE_NULL_HANDLE;

            // Add to the store
            rc = ism_store_createRecord(pThreadData->hStream,
                                        &storeRecord,
                                        &hStoreProps[curServer]);

            // Delete the existing RPR
            if (rc == OK)
            {
                assert(remoteServer->hStoreProps != ismSTORE_NULL_HANDLE);
                rc = ism_store_deleteRecord(pThreadData->hStream,
                                            remoteServer->hStoreProps);
            }

            // Update the RDR to point to the new RPR
            if (rc == OK)
            {
                assert(remoteServer->hStoreDefn != ismSTORE_NULL_HANDLE);
                rc = ism_store_updateRecord(pThreadData->hStream,
                                            remoteServer->hStoreDefn,
                                            hStoreProps[curServer],
                                            iestRDR_STATE_NONE,
                                            ismSTORE_SET_ATTRIBUTE);
            }

            // Something went wrong - roll back and consider retrying
            if (rc != OK)
            {
                iest_store_rollback(pThreadData, true);

                // We reserved space... We should not get generation full
                if (rc == ISMRC_StoreGenerationFull)
                {
                    ieutTRACE_FFDC( ieutPROBE_001, true
                                  , "Failed to store remote server even though reserved space!", rc
                                  , NULL );
                }
                else
                {
                    ieutTRACEL(pThreadData, rc, ENGINE_SEVERE_ERROR_TRACE,
                               "%s: failed to update %d RPRs (rc=%d)\n", __func__, remoteServerCount, rc);
                    goto mod_exit;
                }
            }
        }

        // Commit the update
        iest_store_commit(pThreadData, true);

        // Update the servers' handles
        for(curServer = 0; curServer < remoteServerCount; curServer++)
        {
            remoteServers[curServer]->hStoreProps = hStoreProps[curServer];
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Reserves space in the store for the specified store operations
/// @remarks
///    This function reserves space in the store for the specified
///    store operations on this stream. Having made the reservation the
///    subsequent store calls on this stream will not fail with 
///    ISMRC_GenerationFull.
///
/// @param[in]    DataLength           Reservation Data Length
/// @param[in]    RecordsCount         Number of records created
/// @param[in]    RefsCount            Number of references created
///
///////////////////////////////////////////////////////////////////////////////
void iest_store_reserve( ieutThreadData_t *pThreadData
                       , uint64_t DataLength
                       , uint32_t RecordsCount
                       , uint32_t RefsCount )
{
    uint32_t rc;
    ismStore_Reservation_t Reservation;

    ieutTRACEL(pThreadData, RefsCount, ENGINE_FNC_TRACE, FUNCTION_ENTRY "DataLength=%ld RecordsCount=%d RefsCount=%d\n",
               __func__, DataLength, RecordsCount, RefsCount);

    assert(pThreadData->ReservationState != Active);

    Reservation.DataLength = DataLength;
    Reservation.RecordsCount = RecordsCount+ieutMAXLAZYMSGS; //+lazyMsgs in case we need to complete the lazy deletion of
                                                             //pThreadData->hMsgForLazyRemoval[blah]
    Reservation.RefsCount = RefsCount;

    rc = ism_store_reserveStreamResources( pThreadData->hStream
                                         , &Reservation );
    if (rc != OK)
    {
        // Failure to reserve space in the store is not expected. The
        // store will already have called ism_common_setError, so
        // nothing more for us to do except propograte the error.
        ieutTRACE_FFDC(ieutPROBE_001, true, "ism_store_reserveStreamResources failed! failed.", rc,
                       "Reservation", &Reservation, sizeof(Reservation),
                       "hStream", &pThreadData->hStream, sizeof(ismStore_StreamHandle_t),
                       NULL);
    }

    pThreadData->ReservationState = Active;

    ieutTRACEL(pThreadData, RecordsCount, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}


///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Completes the lazy removal of an unneeded message record
/// @remarks
///    Orphaned Message Records are not a big problem so rather than creating
///    a new store transaction just for their removal, we record in pThreadData
///    that they are no longer required and remove it next time we are completing
///    a store transaction
///////////////////////////////////////////////////////////////////////////////
static inline void iest_completeLazyRemoval(ieutThreadData_t *pThreadData)
{
    for (uint32_t i = 0; i < pThreadData->numLazyMsgs; i++)
    {
        ismStore_Handle_t hMsgForLazyRemoval = pThreadData->hMsgForLazyRemoval[i];
        assert(hMsgForLazyRemoval != ismSTORE_NULL_HANDLE);

        int32_t rc = ism_store_deleteRecord(pThreadData->hStream, hMsgForLazyRemoval);
        if (rc != OK)
        {
            ieutTRACE_FFDC(ieutPROBE_001, true, "ism_store_deleteRecord failed!", rc, NULL);
        }
        ieutTRACEL(pThreadData, hMsgForLazyRemoval, ENGINE_NORMAL_TRACE, "Removing message 0x%0lx because of lazy removal\n",
                   hMsgForLazyRemoval);
        pThreadData->hMsgForLazyRemoval[i] = ismSTORE_NULL_HANDLE;
    }
    pThreadData->numLazyMsgs = 0;
}

static inline void setupCommit( ieutThreadData_t *pThreadData
                              , bool commitReservation )
{
    // We expect commitReservation to be false only if ReservationState *was* Inactive
    assert(commitReservation == !(pThreadData->ReservationState == Inactive));

    pThreadData->ReservationState = Inactive;

    //If we have a message to lazily remove... add that message to this transaction
    iest_completeLazyRemoval(pThreadData);
}

#ifdef USEFAKE_ASYNC_COMMIT
XAPI int32_t fake_ism_store_asyncCommit(ismStore_StreamHandle_t hStream,
                                   ismStore_CompletionCallback_t pCallback,
                                   void *pContext)
{
//printf("In fake ism_store_asyncCommit\n");
    int32_t rc = ism_store_commit(hStream);

    callbackEntry_t entry = {pCallback, pContext, rc};

    fake_addCallback(&entry);

    return ISMRC_AsyncCompletion;
}

#define ism_store_asyncCommit(a, b, c) fake_ism_store_asyncCommit(a, b, c)
#endif

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Asyncly commits the current store transaction
/// @remarks
///    This function commits the updates in the current store transaction
///////////////////////////////////////////////////////////////////////////////
int32_t iest_store_asyncCommit( ieutThreadData_t *pThreadData
                              , bool commitReservation
                              , ismStore_CompletionCallback_t pCallback
                              , void *pContext )
{
    uint32_t rc;

    ieutTRACEL(pThreadData, commitReservation, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Start_AsyncStoreCommit = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_AsyncStoreCommit);
#endif

    setupCommit(pThreadData, commitReservation);

#ifdef HAINVESTIGATIONSTATS
    pThreadData->stats.asyncCommits++;
#endif

    rc = ism_store_asyncCommit(pThreadData->hStream, pCallback, pContext);
    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "ism_store_asyncCommit failed.", rc, NULL);
    }

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Stop_AsyncStoreCommit = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_AsyncStoreCommit);
#endif

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Commits the current store transaction
/// @remarks
///    This function commits the updates in the current store transaction
///////////////////////////////////////////////////////////////////////////////
void iest_store_commit( ieutThreadData_t *pThreadData
                      , bool commitReservation )
{
    uint32_t rc;

    ieutTRACEL(pThreadData, commitReservation, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Start_StoreCommit = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_StoreCommit);
#endif

#ifdef HAINVESTIGATIONSTATS
    pThreadData->stats.syncCommits++;
#endif

    setupCommit(pThreadData, commitReservation);

    rc = ism_store_commit(pThreadData->hStream);
    if (rc != OK)
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "ism_store_commit failed.", rc, NULL);
    }

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Stop_StoreCommit = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_StoreCommit);
#endif

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Backout the current store transaction
/// @remarks
///    This function commits the updates in the current store transaction
///////////////////////////////////////////////////////////////////////////////
void iest_store_rollback( ieutThreadData_t *pThreadData
                        , bool rollbackReservation )
{
    uint32_t rc;

    ieutTRACEL(pThreadData, rollbackReservation, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // We expect rollbackReservation to be false only if ReservationState *was* Inactive
    assert(rollbackReservation == !(pThreadData->ReservationState == Inactive));

    pThreadData->ReservationState = Inactive;

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Start_StoreRollback = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_StoreRollback);
#endif

#ifdef HAINVESTIGATIONSTATS
    pThreadData->stats.syncRollbacks++;
#endif

    rc = ism_store_rollback(pThreadData->hStream);
    if (rc != OK)
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "ism_store_rollback failed.", rc, NULL);
    }

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Stop_StoreRollback = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_StoreRollback);
#endif

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Cancel the current store reservation
/// @remarks
///    This function cancels the reservation we made with the store
///////////////////////////////////////////////////////////////////////////////
void iest_store_cancelReservation( ieutThreadData_t *pThreadData )
{
    uint32_t rc;

    ieutTRACEL(pThreadData, pThreadData->ReservationState, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(pThreadData->ReservationState == Active);

    pThreadData->ReservationState = Inactive;

    rc = ism_store_cancelResourceReservation(pThreadData->hStream);
    if (rc != OK)
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "ism_store_cancelResourceReservation failed.", rc, NULL);
    }

    ieutTRACEL(pThreadData, 0, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Wrapper around ism_store_startTransaction to handle errors etc
/// @returns
///    Boolean indicating whether a new store transaction was created (true) or
///    an existing one was already in place (false)
///////////////////////////////////////////////////////////////////////////////
bool iest_store_startTransaction( ieutThreadData_t *pThreadData )
{
    int32_t rc;
    int storeTranAlreadyActive;

    rc = ism_store_startTransaction(pThreadData->hStream, &storeTranAlreadyActive);
    if (rc != OK)
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "ism_store_startTransaction failed.", rc,
                       "hStream", pThreadData->hStream, sizeof(pThreadData->hStream),
                       "storeTranAlreadyActive", &storeTranAlreadyActive, sizeof(int),
                       NULL);
    }

    ieutTRACEL(pThreadData, storeTranAlreadyActive, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_IDENT "storeTranAlreadyActive=%d\n", __func__, storeTranAlreadyActive);

    return (storeTranAlreadyActive == 0);
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Thin wrapper around ism_store_cancelTransaction to handle errors etc
///////////////////////////////////////////////////////////////////////////////
void iest_store_cancelTransaction( ieutThreadData_t *pThreadData )
{
    int32_t rc;

    rc = ism_store_cancelTransaction(pThreadData->hStream);
    if (rc != OK)
    {
        ieutTRACE_FFDC(ieutPROBE_001, true, "ism_store_cancelTransaction failed.", rc,
                       "hStream", pThreadData->hStream, sizeof(pThreadData->hStream),
                       NULL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Thin wrapper around ism_store_deleteReferenceCommit
/// @remarks
///    This function finishes any lazy message removals as well as doing the
//     deleteReference & commit
///////////////////////////////////////////////////////////////////////////////

int32_t iest_store_deleteReferenceCommit(
                       ieutThreadData_t *pThreadData,
                       ismStore_StreamHandle_t hStream,
                       void *hRefCtxt,
                       ismStore_Handle_t handle,
                       uint64_t orderId,
                       uint64_t minimumActiveOrderId)
{
    //If we have a message to lazily remove... add that message to this transaction
    iest_completeLazyRemoval(pThreadData);

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Start_DeleteReferenceCommit = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_DeleteReferenceCommit);
#endif

#ifdef HAINVESTIGATIONSTATS
    pThreadData->stats.syncCommits++;
#endif

    int32_t rc = ism_store_deleteReferenceCommit(
                    hStream,
                    hRefCtxt,
                    handle,
                    orderId,
                    minimumActiveOrderId);

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Stop_DeleteReferenceCommit = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_DeleteReferenceCommit);
#endif

    return rc;
}


///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Thin wrapper around ism_store_deleteReferenceCommit
/// @remarks
///    This function finishes any lazy message removals as well as doing the
//     createReference & commit
///////////////////////////////////////////////////////////////////////////////
int32_t iest_store_updateReferenceCommit(ieutThreadData_t *pThreadData,
                                        ismStore_StreamHandle_t hStream,
                                        void *hRefCtxt,
                                        ismStore_Handle_t handle,
                                        uint64_t orderId,
                                        uint8_t state,
                                        uint64_t minimumActiveOrderId)
{
    //If we have a message to lazily remove... add that message to this transaction
    iest_completeLazyRemoval(pThreadData);

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Start_UpdateReferenceCommit = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_UpdateReferenceCommit);
#endif

#ifdef HAINVESTIGATIONSTATS
    pThreadData->stats.syncCommits++;
#endif

    int32_t rc = ism_store_updateReferenceCommit(
                        hStream,
                        hRefCtxt,
                        handle,
                        orderId,
                        state,
                        minimumActiveOrderId);

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Stop_UpdateReferenceCommit = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_UpdateReferenceCommit);
#endif

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
/// @brief
///    Thin wrapper around ism_store_createReferenceCommit
/// @remarks
///    This function finishes any lazy message removals as well as doing the
//     createReference & commit
///////////////////////////////////////////////////////////////////////////////
int32_t iest_store_createReferenceCommit(ieutThreadData_t *pThreadData,
                                        ismStore_StreamHandle_t hStream,
                                        void *hRefCtxt,
                                        const ismStore_Reference_t *pReference,
                                        uint64_t minimumActiveOrderId,
                                        ismStore_Handle_t *pHandle)
{
    //If we have a message to lazily remove... add that message to this transaction
    iest_completeLazyRemoval(pThreadData);

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Start_CreateReferenceCommit = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_CreateReferenceCommit);
#endif

#ifdef HAINVESTIGATIONSTATS
    pThreadData->stats.syncCommits++;
#endif

    int32_t rc = ism_store_createReferenceCommit(
                        hStream,
                        hRefCtxt,
                        pReference,
                        minimumActiveOrderId,
                        pHandle);

#if TRACETIMESTAMP_STORECOMMITS
    uint64_t TTS_Stop_CreateReferenceCommit = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_CreateReferenceCommit);
#endif

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Called when the store wish to inform the engine of an event
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iest_storeEventHandler(ismStore_EventType_t eventType, void *pContext)
{
    TRACE(ENGINE_FNC_TRACE, FUNCTION_IDENT "Event(%d)\n", __func__, eventType);

     bool dumpInMemoryTrace = false;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    switch(eventType)
    {
        case ISM_STORE_EVENT_MGMT0_ALERT_ON:
            ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0] = StatusWarning;
            break;
        case ISM_STORE_EVENT_MGMT0_ALERT_OFF:
            ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0] = StatusOk;
            break;
        case ISM_STORE_EVENT_MGMT1_ALERT_ON:
            ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_1] = StatusWarning;
            break;
        case ISM_STORE_EVENT_MGMT1_ALERT_OFF:
            ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_1] = StatusOk;
            break;
        case ISM_STORE_EVENT_DISK_ALERT_ON:
            ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_DISK] = StatusWarning;
            break;
        case ISM_STORE_EVENT_DISK_ALERT_OFF:
            ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_DISK] = StatusOk;
            break;
        case ISM_STORE_EVENT_CBQ_ALERT_ON:
            ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE] = StatusWarning;
            dumpInMemoryTrace = true;
            break;
        case ISM_STORE_EVENT_CBQ_ALERT_OFF:
            ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_ASYNC_CALLBACK_QUEUE] = StatusOk;
            break;
        default:
            assert(false);
            break;
    }

    if (dumpInMemoryTrace)
    {
        //try and dump in memory trace... if we can't keep going...
        (void)ietd_dumpInMemoryTrace( pThreadData
                                    , NULL, NULL, NULL);
    }
    ieut_leavingEngine(pThreadData);
    return OK;
}



/*********************************************************************/
/*                                                                   */
/* End of engineStore.c                                              */
/*                                                                   */
/*********************************************************************/
