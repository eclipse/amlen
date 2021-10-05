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

//****************************************************************************
/// @file  simpQ.c
/// @brief Code for simple queue that doesn't support persistence/transactions
///
/// The queue is multi-producer, single consumer designed for MQTT QoS 0
/// subscriptions (it doesn't support acking). 
/// @par 
/// It also doens't really support transaction, but instead if ask to perform
/// a put within a transaction it merely creates an entry in the softlog and
/// will put the message /in the Post-Commit phase of Commit operation.
/// 
//****************************************************************************
#define TRACE_COMP Engine

#include <stddef.h>
#include <assert.h>
#include <stdint.h>

#include "simpQ.h"
#include "simpQInternal.h"
#include "queueCommonInternal.h"
#include "messageDelivery.h"
#include "memHandler.h"
#include "engineStore.h"
#include "engineDump.h"
#include "waiterStatus.h"
#include "messageExpiry.h"
#include "mempool.h"
#include "resourceSetStats.h"

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes - see definition for more info
///////////////////////////////////////////////////////////////////////////////
static int32_t iesq_putToWaitingGetter( ieutThreadData_t *pThreadData
                                      , iesqQueue_t *q
                                      , ismEngine_Message_t *msg 
                                      , uint8_t msgFlags );
static int32_t iesq_getMessage( ieutThreadData_t *pThreadData
                              , iesqQueue_t *Q
                              , ismEngine_Message_t **outmsg
                              , ismMessageHeader_t *pmsgHdr );
static inline iesqQNodePage_t *iesq_createNewPage( ieutThreadData_t *pThreadData
                                                 , iesqQueue_t *Q
                                                 , uint32_t nodesInPage );
static inline iesqQNodePage_t *iesq_getPageFromEnd( iesqQNode_t *node );
static inline iesqQNodePage_t *iesq_moveToNewPage( ieutThreadData_t *pThreadData
                                                 , iesqQueue_t *Q
                                                 , iesqQNode_t *endMsg );

static inline uint32_t iesq_choosePageSize( void );

void iesq_SLEReplayPut( ietrReplayPhase_t Phase
                      , ieutThreadData_t *pThreadData
                      , ismEngine_Transaction_t *pTran
                      , void *pEntry
                      , ietrReplayRecord_t *pRecord );

static inline void iesq_fullExpiryScan( ieutThreadData_t *pThreadData
                                      , iesqQueue_t *Q
                                      , uint32_t nowExpire);

static inline void iesq_expireNode( ieutThreadData_t *pThreadData
                                  , iesqQueue_t *Q
                                  , iesqQNode_t *qnode);

static void iesq_scanGetCursor( ieutThreadData_t *pThreadData
                              , iesqQueue_t *Q);

static inline void iesq_updateResourceSet( ieutThreadData_t *pThreadData
                                         , iesqQueue_t *Q
                                         , iereResourceSetHandle_t resourceSet);

//////////////////////////////////////////////////////////////////////////////
// Allowing pages to vary in size would allow a balance between the amount
// of memory used for an empty queue and performance. However currently we 
// do not have a sensible way of measuring a 'busy' queue so currently we 
// create the first page as small, and all further pages as the default if
// there are a small number of subscriptions in the system or the high
// capacity size if there are lots of subs in the system
/////////////////////////////////////////////////////////////////////////////
static uint32_t iesqPAGESIZE_SMALL = 8;
static uint32_t iesqPAGESIZE_DEFAULT = 32;
static uint32_t iesqPAGESIZE_HIGHCAPACITY = 8;

static inline int iesq_initPutLock(iesqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_init(&(Q->putlock), PTHREAD_PROCESS_PRIVATE);
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_init(&(Q->putlock.spinlock), PTHREAD_PROCESS_PRIVATE);
    }
    else
    {
        os_rc = pthread_mutex_init(&(Q->putlock.mutex), NULL);
    }
#endif

    return os_rc;
}

static inline int iesq_destroyPutLock(iesqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_destroy(&(Q->putlock));
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_destroy(&(Q->putlock.spinlock));
    }
    else
    {
        os_rc = pthread_mutex_destroy(&(Q->putlock.mutex));
    }
#endif

    return os_rc;
}

static inline void iesq_getPutLock(iesqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_lock(&(Q->putlock));
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_lock(&(Q->putlock.spinlock));
    }
    else
    {
        os_rc = pthread_mutex_lock(&(Q->putlock.mutex));
    }
#endif

    if (UNLIKELY(os_rc != 0))
    {
            ieutTRACE_FFDC( ieutPROBE_001, true
                          , "failed taking put lock.", ISMRC_Error
                          , "Queue", Q, sizeof(iesqQueue_t)
                          , NULL );
    }
}

static inline void iesq_releasePutLock(iesqQueue_t *Q)
{
    int os_rc;

#ifdef IMA_IGNORE_USESPINLOCKS_PROPERTY
    os_rc = pthread_spin_unlock(&(Q->putlock));
#else
    if (ismEngine_serverGlobal.useSpinLocks == true)
    {
        os_rc = pthread_spin_unlock(&(Q->putlock.spinlock));
    }
    else
    {
        os_rc = pthread_mutex_unlock(&(Q->putlock.mutex));
    }
#endif

    if (UNLIKELY(os_rc != 0))
    {
            ieutTRACE_FFDC( ieutPROBE_001, true
                          , "failed release put lock.", ISMRC_Error
                          , "Queue", Q, sizeof(iesqQueue_t)
                          , NULL );
    }
}

/// @{
/// @name External Functions


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Creates a simple Q
///  @remarks
///    This function is used to create a simple Q. The handle returned
///    can be used to put messages or create a consumer on the queue.
///
///  @param[in]  pQName            - Name associated with this queue (expect NULL)
///  @param[in]  QOptions          - Options
///  @param[in]  pPolicyInfo       - Initial policy information to use (optional)
///  @param[in]  hStoreObj         - Definition store handle
///  @param[in]  hStoreProps       - Properties store handle
///  @param[out] pQhdl             - Ptr to Q that has been created (if
///                                  returned ok)
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iesq_createQ( ieutThreadData_t *pThreadData
                    , const char *pQName
                    , ieqOptions_t QOptions
                    , iepiPolicyInfo_t *pPolicyInfo
                    , ismStore_Handle_t hStoreObj
                    , ismStore_Handle_t hStoreProps
                    , iereResourceSetHandle_t resourceSet
                    , ismQHandle_t *pQhdl)
{
    iesqQueue_t *Q;
    int32_t rc=0;
    int os_rc;
    iesqQNodePage_t *firstPage=NULL;

    ieutTRACEL(pThreadData, QOptions, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    // Simple queues are not expected to be forwarding queues
    assert((QOptions & ieqOptions_REMOTE_SERVER_QUEUE) == 0);

    iere_primeThreadCache(pThreadData, resourceSet);
    Q = (iesqQueue_t *)iere_malloc(pThreadData,
                                   resourceSet,
                                   IEMEM_PROBE(iemem_simpleQ, 1),
                                   sizeof(iesqQueue_t));

    if (Q == NULL)
    {
       rc = ISMRC_AllocateError;
       ism_common_setError(rc);
       goto mod_exit;
    }

    //Record that it's a q...
    ismEngine_SetStructId(Q->Common.StrucId, ismENGINE_QUEUE_STRUCID);
    //Record it's a simple Q... not transactions, no acking  - single getter..
    Q->Common.QType = simple;
    Q->Common.pInterface = &QInterface[simple];
    Q->Common.resourceSet = resourceSet;
    Q->Common.informOnEmpty = false;
    Q->Common.expiryLink.prev = Q->Common.expiryLink.next = NULL;
    Q->Common.QExpiryData = NULL;

    if (pQName != NULL)
    {
        Q->Common.QName = (char *)iere_malloc(pThreadData, resourceSet, IEMEM_PROBE(iemem_simpleQ, 2), strlen(pQName)+1);

        if (Q->Common.QName == NULL)
        {
            rc = ISMRC_AllocateError;

            ism_common_setError(rc);

            goto mod_exit;
        }

        strcpy(Q->Common.QName, pQName);
    }
    else
    {
        Q->Common.QName = NULL;
    }

    os_rc = iesq_initPutLock(Q);
    if (os_rc != OK)
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);

        ieutTRACEL(pThreadData, os_rc, ENGINE_SEVERE_ERROR_TRACE,
                   "%s: init(putlock) failed! (osrc=%d)\n", __func__,
                   os_rc);

        goto mod_exit;
    }

    assert(pPolicyInfo != NULL);

    Q->waiterStatus = IEWS_WAITERSTATUS_DISCONNECTED;
    Q->bufferedMsgs = 0;
    Q->bufferedMsgsHWM = 0;
    Q->rejectedMsgs = 0;
    Q->discardedMsgs = 0;
    Q->expiredMsgs = 0;
    Q->Common.PolicyInfo = pPolicyInfo;
    Q->ReportedQueueFull = false;
    Q->isDeleted = false;
    Q->QOptions = QOptions;
    Q->pConsumer = NULL;
    Q->hStoreObj = hStoreObj;
    Q->hStoreProps = hStoreProps;
    Q->enqueueCount = 0;
    Q->dequeueCount = 0;
    Q->qavoidCount = 0;
    Q->putsAttempted = 0;
    Q->preDeleteCount = 1; //=1 as not yet deleted

    //Allocate a page of queue nodes
    firstPage = iesq_createNewPage(pThreadData, Q, iesqPAGESIZE_SMALL);

    if (firstPage == NULL)
    {
       rc = ISMRC_AllocateError;
       ism_common_setError(rc);
       goto mod_exit;
    }
    Q->headPage = firstPage;
    Q->head     = &(firstPage->nodes[0]);
    Q->tail     = &(firstPage->nodes[0]);

    // Delay the creation of the second page until we actually need
    // it. This should help the memory footprint especially for queues/
    // subscriptions which only every have a few messages.
    firstPage->nextStatus = failed;

    *pQhdl = (ismQHandle_t)Q;

 mod_exit:

    if (rc != OK && Q != NULL)
    {
        if (Q->Common.QName != NULL) iere_free(pThreadData, resourceSet, iemem_simpleQ, Q->Common.QName);

        (void)iesq_destroyPutLock(Q);

        if (firstPage != NULL) iere_freeStruct(pThreadData, resourceSet, iemem_simpleQPage, firstPage, firstPage->StrucId);

        iere_freeStruct(pThreadData, resourceSet, iemem_simpleQ, Q, Q->Common.StrucId);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d, Q=%p\n", __func__, rc, Q);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Return whether the queue is marked as deleted or not
///  @param[in] Qhdl               - Queue to check
///////////////////////////////////////////////////////////////////////////////
bool iesq_isDeleted(ismQHandle_t Qhdl)
{
    return ((iesqQueue_t *)Qhdl)->isDeleted;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Update the in-memory queue
///
///  @param[in] Qhdl               - Queue to be marked as deleted
///  @param[in] updateStore        - Whether to change the store record
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iesq_markQDeleted(ieutThreadData_t *pThreadData,
                          ismQHandle_t Qhdl,
                          bool updateStore)
{

    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    int32_t rc = OK;

    uint64_t newState;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p updateStore=%d\n", __func__, Q, (int)updateStore);

    // Mark the in-memory queue as deleted
    Q->isDeleted = true;

    // If requested, mark the store definition record as deleted too.
    if (updateStore)
    {
        // If we have not store handle for the definition, there is
        // nothing to do here.
        if (Q->hStoreObj == ismSTORE_NULL_HANDLE)
        {
            goto mod_exit;
        }

        // Set the state of the definition record to deleted.
        if (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE)
        {
            newState = iestSDR_STATE_DELETED;
        }
        else
        {
            newState = iestQDR_STATE_DELETED;
        }

        // Mark the store definition record as deleted
        rc = ism_store_updateRecord(pThreadData->hStream,
                                    Q->hStoreObj,
                                    ismSTORE_NULL_HANDLE,
                                    newState,
                                    ismSTORE_SET_STATE);

        if (rc == OK)
        {
            iest_store_commit(pThreadData, false);
        }
        else
        {
            assert(rc != ISMRC_StoreGenerationFull);
            iest_store_rollback(pThreadData, false);
            ism_common_setError(rc);
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Completes the deletion a simple Q
///  @remarks
///    This function is used to release the storage for a simple Q.
///
///  @param[in] pQhdl              - Ptr to Q to be deleted
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static void iesq_completeDeletion(ieutThreadData_t *pThreadData, iesqQueue_t *Q )
{
    int32_t rc2;
    int os_rc;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " *Q=%p\n", __func__, Q);

    ieme_freeQExpiryData(pThreadData, (ismEngine_Queue_t *)Q);

    while (Q->headPage != NULL)
    {
        if (Q->head != NULL)
        {
            iesqQNodePage_t *pageToFree = NULL;
            iesqQNode_t *temp = Q->head;

            //Move the head to the next item..
            (Q->head)++;

            //Have we gone off the bottom of the page?
            if(Q->head->msg == MESSAGE_STATUS_ENDPAGE)
            {
                pageToFree = Q->headPage;
                iesqQNodePage_t *nextPage = pageToFree->next;

                if( nextPage != NULL)
                {
                    Q->headPage = nextPage;
                    Q->head = &(nextPage->nodes[0]);
                }
                else
                {
                    Q->headPage = NULL;
                    Q->head = NULL;
                }
            }

            if(temp->msg != NULL && temp->msg != MESSAGE_STATUS_REMOVED)
            {
                iem_releaseMessage(pThreadData, temp->msg);
            }

            if(pageToFree != NULL)
            {
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_freeStruct(pThreadData, resourceSet, iemem_simpleQPage, pageToFree, pageToFree->StrucId);
            }
        }
        else
        {
            //We've found an unwritten entry... no more items on this page
            iesqQNodePage_t *pageToFree = Q->headPage;

            Q->headPage = Q->headPage->next;

            if (NULL != Q->headPage)
            {
                Q->head = &(Q->headPage->nodes[0]);
            }

            iere_primeThreadCache(pThreadData, resourceSet);
            iere_freeStruct(pThreadData, resourceSet, iemem_simpleQPage, pageToFree, pageToFree->StrucId);
        }
    }

    // Delete the store definition record associated with this queue
    // (which will be an SDR) and it's associated properties record (which will
    // be for a subscription, an SPR).
    // Note that we never associated any message references with the store
    // definition for a simple queue, but it exists to identify undeleted subscriptions.
    if (Q->hStoreObj != ismSTORE_NULL_HANDLE)
    {
        if (Q->deletionRemovesStoreObjects)
        {
            rc2 = ism_store_deleteRecord(pThreadData->hStream, Q->hStoreObj);
            if (rc2 != OK)
            {
                ieutTRACEL(pThreadData, rc2, ENGINE_ERROR_TRACE,
                           "%s: ism_store_deleteRecord (%s) failed! (rc=%d)\n",
                           __func__, (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE) ? "SDR" : "QDR",
                           rc2);
            }
            rc2 = ism_store_deleteRecord(pThreadData->hStream, Q->hStoreProps);
            if (rc2 != OK)
            {
                ieutTRACEL(pThreadData, rc2, ENGINE_ERROR_TRACE,
                           "%s: ism_store_deleteRecord (%s) failed! (rc=%d)\n",
                           __func__, (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE) ? "SPR" : "QPR",
                           rc2);
            }
            iest_store_commit(pThreadData, false);
        }
    }

    os_rc = iesq_destroyPutLock(Q);

    if (os_rc != OK)
    {
        ieutTRACEL(pThreadData, os_rc, ENGINE_ERROR_TRACE, "%s: destroy putlock failed! (os_rc=%d)\n",
                   __func__, os_rc);
    }

    // Release our use of the messaging policy info
    iepi_releasePolicyInfo(pThreadData, Q->Common.PolicyInfo);

    //All the messages that were buffered on this queue need to be removed from the global
    //buffered messages count
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -(int64_t)(Q->bufferedMsgs));
    pThreadData->stats.bufferedMsgCount -= Q->bufferedMsgs;

    if (Q->Common.QName != NULL)
    {
        iere_free(pThreadData, resourceSet, iemem_simpleQ, Q->Common.QName);
    }

    iere_freeStruct(pThreadData, resourceSet, iemem_simpleQ, Q, Q->Common.StrucId);

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Deletes a simple Q
///  @remarks
///    This function is used to release the storage for a simple Q.
///    The queue will not actually be deleted until any consumer is also deleted
///
///  @param[in] pQhdl              - Ptr to Q to be deleted
///  @param[in] freeOnly           - Just free the memory or remove from store as well
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iesq_deleteQ( ieutThreadData_t *pThreadData
                    , ismQHandle_t *pQhdl
                    , bool freeOnly )
{
    int32_t rc = OK;
    iesqQueue_t *Q = (iesqQueue_t *)*pQhdl;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " *Q=%p\n", __func__, Q);

    Q->deletionRemovesStoreObjects = !freeOnly;

    // Mark the queue as deleted in the Store - either an SDR or a QDR
    rc = iesq_markQDeleted(pThreadData, *pQhdl, Q->deletionRemovesStoreObjects);
    if (rc != OK) goto mod_exit;

    // Complete the deletion if there is nothing blocking it
    iesq_reducePreDeleteCount(pThreadData, (ismQHandle_t)Q);


    //Caller has no right to access the queue any more
    *pQhdl = NULL;
mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


#if 0
void dumpExpiryData(iesqQueue_t *Q)
{
    iemeQueueExpiryData_t *pQExpiryData = (iemeQueueExpiryData_t *)Q->Common.QExpiryData;

    if(pQExpiryData != NULL)
    {
        printf("InArray %u, InTotal %lu\n",
                pQExpiryData->messagesInArray, pQExpiryData->messagesWithExpiry);
         for (uint32_t i=0; i < pQExpiryData->messagesInArray; i++)
         {
            printf("slot: %u node: %p msg: 0x%p expiry %c%u\n",
                    i, pQExpiryData->earliestExpiryMessages[i].qnode,
                    ((iesqQNode_t *)pQExpiryData->earliestExpiryMessages[i].qnode)->msg,
                    (i == 0) ? ' ': '+',
                    (i == 0)  ? pQExpiryData->earliestExpiryMessages[i].Expiry :
                                pQExpiryData->earliestExpiryMessages[i].Expiry
                                 - pQExpiryData->earliestExpiryMessages[0].Expiry);
        }
        if ( pQExpiryData->messagesInArray < NUM_EARLIEST_MESSAGES)
        {
            uint32_t i = pQExpiryData->messagesInArray;

            printf("nextslot: %u node: %p expiry  %u\n",
                                i, pQExpiryData->earliestExpiryMessages[i].qnode,
                                pQExpiryData->earliestExpiryMessages[i].Expiry);
        }
    }
    else
    {
        printf("ExpiryData is null\n");
    }
}
#endif

static void iesq_appendPage( ieutThreadData_t *pThreadData
                           , iesqQueue_t *Q
                           , iesqQNodePage_t *currPage)
{
    uint32_t nodesInPage =  iesq_choosePageSize();
    iesqQNodePage_t *newPage = iesq_createNewPage(pThreadData,
                                                  Q,
                                                  nodesInPage);

    if (newPage == NULL)
    {
        // Argh - we've run out of memory - record we failed to add a
        // page so no-one sits waiting for it to appear. This is not
        // a problem for us however we will confinue on our merry way.
        DEBUG_ONLY bool expectedState;
        expectedState=__sync_bool_compare_and_swap(&(currPage->nextStatus),
                                                   unfinished,
                                                   failed);
        assert(expectedState);

        ieutTRACEL(pThreadData, nodesInPage, ENGINE_ERROR_TRACE,
                   "iesq_createNewPage failed Q=%p nextPage=%p\n", Q, currPage);

    }
    else
    {
        currPage->next = newPage;
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Puts a message on a Simple Q
///  @remarks
///    This function is called to put a message on a simple queue.
///
///  @param[in] Qhdl               - Q to put to
///  @param[in] pTran              - Pointer to transaction
///  @param[in] inmsg              - message to put
///  @param[in] inputMsgTreatment  - IEQ_MSGTYPE_REFCOUNT (re-use & increment refcount)
///                                  or IEQ_MSGTYPE_INHERIT (re-use)
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iesq_putMessage( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ieqPutOptions_t putOptions
                       , ismEngine_Transaction_t *pTran
                       , ismEngine_Message_t *inmsg
                       , ieqMsgInputType_t inputMsgTreatment)
{
    int32_t rc = 0;
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    ismEngine_Message_t  *qmsg = NULL;
    iesqQNodePage_t *nextPage = NULL;
    iesqQNode_t *pNode;
    uint8_t msgFlags;
    int32_t putDepth;
    iereResourceSet_t *resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, inmsg, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p pTran=%p msg=%p Length=%ld\n",
               __func__, Q, pTran, inmsg, inmsg->MsgLength);

#if TRACETIMESTAMP_PUTMESSAGE
    uint64_t TTS_Start_PutMessage = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_PutMessage);
#endif

    iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;

    assert(pPolicyInfo->maxMessageBytes == 0); // No code to deal with maxMessageBytes

    if ((pPolicyInfo->maxMessageCount > 0) && (Q->bufferedMsgs >= pPolicyInfo->maxMessageCount))
    {
        if (!Q->ReportedQueueFull)
        {
            ism_common_log_context logContext = {0};
            if (resourceSet != iereNO_RESOURCE_SET)
            {
                logContext.resourceSetId = resourceSet->stats.resourceSetIdentifier;
            }

            // No need to separately trace, the log message will be shown in the trace

            Q->ReportedQueueFull = true;

            if ((Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE) == 0)
            {
                LOGCTX( &logContext, ERROR, Messaging, 3006, "%-s%lu",
                        "The limit of {1} messages buffered on queue {0} has been reached.",
                        Q->Common.QName ? Q->Common.QName : "", pPolicyInfo->maxMessageCount  );
            }
            else
            {
                LOGCTX( &logContext, ERROR, Messaging, 3007, "%-s%lu",
                        "The limit of {1} messages buffered for subscription {0} has been reached.",
                        Q->Common.QName ? Q->Common.QName : "", pPolicyInfo->maxMessageCount  );
            }
        }
        else
        {
            ieutTRACEL(pThreadData, Q->bufferedMsgs, ENGINE_HIGH_TRACE,
                          "%s: Queue full. bufferedMsgs=%lu maxMessageCount=%lu\n",
                          __func__, Q->bufferedMsgs, pPolicyInfo->maxMessageCount);
        }


        if (pPolicyInfo->maxMsgBehavior == RejectNewMessages)
        {
            // NOTE: SimpQ ignores the ieqPutOptions_IGNORE_REJECTNEWMSGS option
            //       because the publish will ignore the rejection, and this way
            // we don't build up a long list of messages - if we wanted to honour
            // the put option, we would need the code following...
            #if 0
            if ((putOptions & ieqPutOptions_IGNORE_REJECTNEWMSGS) != 0)
            {
                ieutTRACEL(pThreadData, putOptions, ENGINE_HIGH_TRACE,
                           "Ignoring RejectNewMessages on full queue. putOptions=0x%08x\n",
                           putOptions);
            }
            else
            #endif
            {
                rc = ISMRC_DestinationFull;
                ism_common_setError(rc);

                __sync_add_and_fetch(&(Q->rejectedMsgs), 1);
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_REJECTEDMSGS, 1);

                goto mod_exit;
            }
        }
        else if (Q->bufferedMsgs >= (100 + (3 * pPolicyInfo->maxMessageCount)))
        {
            ieutTRACEL(pThreadData, Q->bufferedMsgs, ENGINE_WORRYING_TRACE,
                          "%s: Queue %s Significantly OVERfull. bufferedMsgs=%lu maxMessageCount=%lu\n",
                          __func__, Q->Common.QName ? Q->Common.QName : "",
                          Q->bufferedMsgs, pPolicyInfo->maxMessageCount);
            usleep(50);
        }
    }

    // The sending (putting) of messages to this queue has been disallowed
    if (pPolicyInfo->allowSend == false)
    {
        rc = ISMRC_SendNotAllowed;
        goto mod_exit;
    }

    msgFlags = ismMESSAGE_FLAGS_NONE;
    if (putOptions & ieqPutOptions_RETAINED)
    {
        msgFlags |= ismMESSAGE_FLAGS_RETAINED;
    }

    qmsg = inmsg;

    if (inputMsgTreatment == IEQ_MSGTYPE_REFCOUNT)
    {
        iem_recordMessageUsage(qmsg);
    }
    else
    {
        assert(inputMsgTreatment == IEQ_MSGTYPE_INHERIT);
    }

    // If this put is being done as part of a transaction, then
    // we do not do the put now, instead add an entry to the softlog
    // for the message to be put at commit time.
    if (pTran)
    {
        iesqSLEPut_t SLE;
        SLE.pQueue = Q;
        SLE.pMsg = qmsg;
        SLE.putOptions = putOptions;
        SLE.bSavepointRolledback = false;

        // Add the message to the Soft log to be added to the queue when
        // commit is called
        rc = ietr_softLogAdd( pThreadData
                            , pTran
                            , ietrSLE_SQ_PUT
                            , iesq_SLEReplayPut
                            , NULL
                            , PostCommit | Rollback | SavepointRollback
                            , &SLE
                            , sizeof(SLE)
                            , 0, 0
                            , NULL);

        goto mod_exit;
    }


#if 1
    //If there's nothing on the queue that needs to be given first see if we can pass the message
    //directly to the consumer
    //This function is crucial to QoS-0 perofrmance howevever for comparison between the simpleQ
    //and intermediateQ it is useful to be able to switch it off.
    if (Q->bufferedMsgs == 0)
    {
        rc = iesq_putToWaitingGetter(pThreadData, Q, qmsg, msgFlags);
        if (rc == OK)
        {
            //We've given the message to someone..we're done
            goto mod_exit;
        }
        else /* (rc == ISMRC_NoAvailWaiter) */
        {
            assert(rc == ISMRC_NoAvailWaiter);
            // No-one wants it, enqueue it
            rc = OK;
        }
    }
#endif

#if TRACETIMESTAMP_PUTLOCK
    uint64_t TTS_Start_PutLock = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_PutLock);
#endif

    iesq_getPutLock(Q);

    pNode = Q->tail;

    // Have we run out of space on this page?
    if ((Q->tail+1)->msg == MESSAGE_STATUS_ENDPAGE)
    {
        //Page full...move to the next page if it has been added...
        //...if it hasn't been added someone will add it shortly
        iesqQNodePage_t *currpage=iesq_getPageFromEnd(Q->tail+1);
        nextPage=iesq_moveToNewPage(pThreadData, Q, Q->tail+1);

        // A non-null nextPage will be used to identify that a new
        // page must be created once we've released the put lock
        if (nextPage == NULL)
        {
            // We're going to fail the put through lack of memory
            if (inputMsgTreatment != IEQ_MSGTYPE_INHERIT)
            {
                iem_releaseMessage(pThreadData, qmsg);
            }

            iesq_releasePutLock(Q);

            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto mod_exit;
        }
        //Make the tail point to the first item in the new page
        Q->tail = &(nextPage->nodes[0]);

        //Record on the old page that a getter can free it
        currpage->nextStatus = completed;
    }
    else
    {
        // Move the tail to the next item in the page
        Q->tail++;
    }

    // Check that the node is empty. Perhpas this should be more than an
    // 'assert' when we have FDCs
    assert(pNode->msg == MESSAGE_STATUS_EMPTY);

    // Increase the count of enqueues
    Q->enqueueCount++;

    //Release the putting lock
    iesq_releasePutLock(Q);

#if TRACETIMESTAMP_PUTLOCK
    uint64_t TTS_Stop_PutLock = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_PutLock);
#endif

    //If we need to create a new page do it here...
    if (nextPage != NULL)
    {
        iesq_appendPage(pThreadData, Q, nextPage);
        nextPage = NULL;
    }

    // Increase the count of messages on the queue, this must be done
    // before we actually make the message visible
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, 1);
    pThreadData->stats.bufferedMsgCount++;

    //Store the flags for the message before we make it visible:
    pNode->msgFlags = msgFlags;

    putDepth = __sync_add_and_fetch(&(Q->bufferedMsgs), 1);

    if (qmsg->Header.Expiry != 0)
    {
        iemeBufferedMsgExpiryDetails_t msgExpiryData = { (uint64_t)pNode , pNode, qmsg->Header.Expiry };
        ieme_addMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  &msgExpiryData);
    }

    // Store the message. The intermediate queue needs a write barrier
    // here but the simple queue node contains only the msg pointer so
    // no need for the simple queue.
    pNode->msg = qmsg;

    // Update the HighWaterMark on the number of messages on the queue. 
    // this is recorded on a best-can-do basis 
    if (putDepth > Q->bufferedMsgsHWM)
    {
      Q->bufferedMsgsHWM = putDepth;
    }

    //And try and deliver any messages
    rc = iesq_checkWaiters(pThreadData, (ismQHandle_t)Q, NULL);

    //If we've hit maximum do a quick check for expired messages
    if (Q->bufferedMsgs >= pPolicyInfo->maxMessageCount)
    {
        ieme_reapQExpiredMessages(pThreadData, (ismEngine_Queue_t *)Q);

        //If we're actually over maximum, discard down if allowed
        if (   (pPolicyInfo->maxMsgBehavior == DiscardOldMessages)
                && (Q->bufferedMsgs > pPolicyInfo->maxMessageCount))
        {
            iesq_reclaimSpace(pThreadData, Qhdl, true);
        }
    }

mod_exit:
    assert(nextPage == NULL); //If we needed to add a page, we've done it

#if TRACETIMESTAMP_PUTMESSAGE
    uint64_t TTS_Stop_PutMessage = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_PutMessage);
#endif

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Puts a message onto a Simple Q as part of an import
///  @remarks
///    This function is called to import a message on a simple queue -- this is
///    really just a thin wrapper around iesq_putMessage with fixed putOptions,
///    pTran and inputMsgTreatment.
///
///  @param[in] Qhdl               - Q to put to
///  @param[in] pMessage            - message to import
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iesq_importMessage( ieutThreadData_t *pThreadData
                          , ismQHandle_t Qhdl
                          , ismEngine_Message_t *pMessage )
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;

    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    iere_updateMessageResourceSet(pThreadData, resourceSet, pMessage, true, false);

    int32_t rc = iesq_putMessage( pThreadData
                                , Qhdl
                                , ieqPutOptions_NONE
                                , NULL
                                , pMessage
                                , IEQ_MSGTYPE_INHERIT);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Complete the rehydration of a queue
///  @remarks
///    Perform any actions needed to complete rehydration for a simple queue.
///
///  @param[in] Qhdl               - Q to complete
///////////////////////////////////////////////////////////////////////////////
int32_t iesq_completeRehydrate( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl )
{

    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;

    if (Q->Common.resourceSet == iereNO_RESOURCE_SET)
    {
        // We expect subscriptions to be assigned to a resource set, if we don't have one now
        // assign it to the default (which may be iereNO_RESOURCE_SET in which case this is a
        // no-op)
        if (Q->QOptions & ieqOptions_SUBSCRIPTION_QUEUE)
        {
            iereResourceSetHandle_t defaultResourceSet = iere_getDefaultResourceSet();
            iesq_updateResourceSet(pThreadData, Q, defaultResourceSet);
        }
    }

    //Initialize the value of putsAttempted (should be 0 at this stage)
    Q->putsAttempted = Q->qavoidCount + Q->enqueueCount + Q->rejectedMsgs;

    //This queue is no longer in recovery
    Q->QOptions &= ~ieqOptions_IN_RECOVERY;

    return OK;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    After rehydration, delete this queue if it is no longer required
///  @remarks
///    This function is called during complete rehydration to ensure queues we
///    have left over but no longer need don't hang around for ever.
///
///  @param[in] Qhdl               - Q to put to
///////////////////////////////////////////////////////////////////////////////
void iesq_removeIfUnneeded( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl )
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    ieutTRACEL(pThreadData, Q,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "Q=%p\n", __func__, Q);

    if (Q->isDeleted)
    {
        assert(Q->bufferedMsgs == 0); // expecting no messages

        iesq_deleteQ(pThreadData, &Qhdl, false);
    }
    ieutTRACEL(pThreadData, Q, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Associate the Queue and the Consumer
///  @remarks
///    Associate the Consumer passed with the queue. When the consumer
///    is enabled messages on this queue will be delivered to the consumer.
///
///  @param[in] Qhdl               - Queue
///  @param[in] pConsumer          - Consumer
///  @return                       - OK
///////////////////////////////////////////////////////////////////////////////

int32_t iesq_initWaiter( ieutThreadData_t *pThreadData,
                         ismQHandle_t Qhdl,
                         ismEngine_Consumer_t *pConsumer)
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    int32_t rc=OK;
    iewsWaiterStatus_t oldStatus;

    bool doneCAS = false;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n", __func__, Q);

    //This queue can't do acks... check the consumer hasn't asked for them
    assert(!pConsumer->fAcking);

    do
    {
        oldStatus = Q->waiterStatus;
        iewsWaiterStatus_t newStatus;

        if (oldStatus == IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)
        {
            //loop and try again
            ismEngine_FullMemoryBarrier();
            continue;
        }
        else if (oldStatus == IEWS_WAITERSTATUS_DISCONNECTED)
        {
            newStatus = IEWS_WAITERSTATUS_DISABLED;
        }
        else
        {
            //If we CAS to prove we're not looking at a stale value and it's
            //not disconnected then someone is still connected...
            newStatus = oldStatus;
        }
        doneCAS = iews_bool_changeState( &(Q->waiterStatus)
                                       , oldStatus
                                       , newStatus);
    }
    while(!doneCAS);


    if (oldStatus == IEWS_WAITERSTATUS_DISCONNECTED)
    {
        __sync_fetch_and_add( &(Q->preDeleteCount), 1); //don't delete the queue whilst this consumer is connected
        Q->pConsumer = pConsumer;
    }
    else
    {
        rc = ISMRC_WaiterInUse;
        ism_common_setError(rc);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}
///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Remove the consumer from the queue.
///  @remarks
///    May happen asynchronously (if the waiter is locked)...
///  ism_engine_deliverStatus() when we're done
///
///  @param[in] Qhdl               - Queue
///  @return                       - OK
///////////////////////////////////////////////////////////////////////////////
int32_t iesq_termWaiter( ieutThreadData_t *pThreadData
                       , ismQHandle_t Qhdl
                       , ismEngine_Consumer_t *pConsumer)
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n", __func__, Q);

    //This type of queue only has one consumer... so they had better be destroying that one!
    assert(Q->pConsumer == pConsumer);

    bool doneDisconnect = false;
    bool waiterInUse = false;
    iewsWaiterStatus_t oldState;

    // First we need to move the waiter into DISCONNECT_PEND state. It may be
    // that DISABLE_PEND will also be on, but that is ok.
    do
    {
        oldState = Q->waiterStatus;
        iewsWaiterStatus_t newState;

        if ((oldState == IEWS_WAITERSTATUS_DISCONNECTED) ||
            ((oldState & IEWS_WAITERSTATUS_DISCONNECT_PEND) != 0))
        {
            // You can't disconnect the waiter if it's already disconnect(ing)
            rc = ISMRC_WaiterInvalid;
            goto mod_exit;
        }
        else if(  (oldState == IEWS_WAITERSTATUS_GETTING)
                ||(oldState == IEWS_WAITERSTATUS_DELIVERING))
        {
            newState = IEWS_WAITERSTATUS_DISCONNECT_PEND;
            waiterInUse = true;

            doneDisconnect = iews_bool_changeState( &(Q->waiterStatus)
                                                  , oldState
                                                  , newState);
        }
        else if (oldState & IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND)
        {
            //This flag implies an earlier disable was cancelled... this is getting very confusing. For the
            //moment lets just take a deep breath and wait for things to calm down
            ismEngine_FullMemoryBarrier();
            continue;
        }
        else if( oldState & IEWS_WAITERSTATUSMASK_PENDING)
        {
            //We know the enable flag is not set (special cased above)
            newState = oldState | IEWS_WAITERSTATUS_DISCONNECT_PEND;
            waiterInUse = true;
        }
        else if (oldState == IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)
        {
            //try again.
            ismEngine_FullMemoryBarrier();
            continue;
        }
        else
        {
            newState = IEWS_WAITERSTATUS_DISCONNECT_PEND;
            waiterInUse = false;

            doneDisconnect = iews_bool_tryLockToState( &(Q->waiterStatus)
                                                     , oldState
                                                     , newState);
        }
    } while (!doneDisconnect);

    if (waiterInUse)
    {
        // The waiter is (was) locked by someone else when we switched
        // on the IEWS_WAITERSTATUS_DISCONNECT_PEND state, so our work
        // is done.
        rc = ISMRC_AsyncCompletion;
    }
    else
    {
        bool waiterWasEnabled = (oldState != IEWS_WAITERSTATUS_DISABLED) ?  true : false;

        ieq_completeWaiterActions( pThreadData
                                 , Qhdl
                                 , Q->pConsumer
                                 , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                                 , waiterWasEnabled);
    }

mod_exit:
    //NB: The above callback may have destroyed the queue so
    //we cannot use it at this point
    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Adjust expiry counter as a message expired
///
///  @param[in] Qhdl               - Queue
///  @return                       - void
///////////////////////////////////////////////////////////////////////////////
void iesq_messageExpired( ieutThreadData_t *pThreadData
                        , ismQHandle_t Qhdl)
{
    iesqQueue_t *Q = (iesqQueue_t *) Qhdl;
    __sync_fetch_and_add(&(Q->expiredMsgs), 1);
    pThreadData->stats.expiredMsgCount++;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Remove messages from the front of the queue to make space for new messages
///
///  @param[in] Qhdl               - Queue
///  @param[in] takeLock             Should the waiterStatus lock be taken
///                                   (^ should be true unless already taken!!!)
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////

void iesq_reclaimSpace( ieutThreadData_t *pThreadData
                      , ismQHandle_t Qhdl
                      , bool takeLock )
{
    int32_t rc = OK;
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    iewsWaiterStatus_t oldStatus = Q->waiterStatus;
    iewsWaiterStatus_t newStatus = IEWS_WAITERSTATUS_DISCONNECTED;
    uint64_t removedMsgs=0;
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n", __func__, Q);

#if TRACETIMESTAMP_RECLAIM
    uint64_t TTS_Start_Reclaim = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_Reclaim);
#endif

    if (takeLock)
    {
        bool gotLock = iews_tryLockForReclaimSpace ( &(Q->waiterStatus)
                                                   , &oldStatus
                                                   , &newStatus );

        if (!gotLock)
        {
            //We didn't get the lock but we did leave a flag that asked someone else to do the clean
            //Good enough!
            goto mod_exit;
        }
    }

    //Work out what the target number of bufferedmsgs is
    iepiPolicyInfo_t *pPolicyInfo = Q->Common.PolicyInfo;

    assert(pPolicyInfo->maxMessageBytes == 0); // No code to deal with maxMessageBytes

    uint64_t targetBufferedMessages =  1 + (uint64_t)(pPolicyInfo->maxMessageCount * IEQ_RECLAIMSPACE_MSGS_SURVIVE_FRACTION);  //1+ causes target to round up not down

    //Now look for messages to remove and consume them
    while (Q->bufferedMsgs  > targetBufferedMessages)
    {
        ismEngine_Message_t *outmsg = NULL;
        rc = iesq_getMessage(pThreadData, Q,  &outmsg, NULL);

        if (rc == OK)
        {
            //We no longer need this message
            iem_releaseMessage(pThreadData, outmsg);
            removedMsgs++;
        }
        else
        {
            assert(rc == ISMRC_NoMsgAvail);
            break;
        }
    }

    if (removedMsgs > 0)
    {
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_DISCARDEDMSGS, (int64_t)removedMsgs);
        __sync_fetch_and_add(&(Q->discardedMsgs), removedMsgs);
    }

    if (takeLock)
    {
         iews_unlockAfterQOperation( pThreadData
                                       , Qhdl
                                       , Q->pConsumer
                                       , &(Q->waiterStatus)
                                       , newStatus
                                       , oldStatus);
    }

#if TRACETIMESTAMP_RECLAIM
    uint64_t TTS_Stop_Reclaim = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_Reclaim);
#endif

mod_exit:
    ieutTRACEL(pThreadData, removedMsgs,  ENGINE_FNC_TRACE, FUNCTION_EXIT "removed = %lu\n", __func__, removedMsgs);
    return;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Drain messages from a queue
///  @remark
///    This function is used to delete all messages from the queue.
///
///  @param[in] Qhdl               - Queue
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iesq_drainQ(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl)
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    ismEngine_Message_t *msg;
    ismMessageHeader_t msgHdr;
    int32_t rc = OK;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q %p\n", __func__, Q);

    while (rc == OK)
    {
        rc = iesq_getMessage( pThreadData
                            , Q
                            , &msg
                            , &msgHdr );

        if ( rc == OK)
        {
            Q->discardedMsgs++;
            iem_releaseMessage(pThreadData, msg);
        }
    }
    //If the queue is empty... that's good.
    if (rc == ISMRC_NoMsgAvail )
    {
        rc = OK;
    }
    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Query queue statistics from a queue
///  @remarks
///    Returns a structure ismEngine_QueueStatistics_t which contains information
///    about the current state of the queue.
///
///  @param[in]  Qhdl             Queue
///  @param[out] stats            Statistics data
///////////////////////////////////////////////////////////////////////////////
void iesq_getStats(ieutThreadData_t *pThreadData,
                   ismQHandle_t Qhdl,
                   ismEngine_QueueStatistics_t *stats)
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;

    stats->BufferedMsgs = Q->bufferedMsgs;
    stats->BufferedMsgsHWM = Q->bufferedMsgsHWM;
    //Once we've added ways of querying discarded separately from rejected, we should
    //Not do the following addition:
    stats->RejectedMsgs = Q->rejectedMsgs;
    stats->DiscardedMsgs = Q->discardedMsgs;
    stats->ExpiredMsgs = Q->expiredMsgs;
    stats->InflightEnqs = 0;
    stats->InflightDeqs = 0;
    stats->EnqueueCount = Q->enqueueCount;
    stats->DequeueCount = Q->dequeueCount;
    stats->QAvoidCount = Q->qavoidCount;
    stats->MaxMessages = Q->Common.PolicyInfo->maxMessageCount;
    stats->PutsAttempted = Q->putsAttempted;
    stats->MaxMessageBytes = Q->Common.PolicyInfo->maxMessageBytes;
    assert(stats->MaxMessageBytes == 0); // No code to deal with maxMessageBytes
    stats->BufferedMsgBytes = 0;
    // Calculate some additional statistics
    stats->ProducedMsgs = stats->EnqueueCount+stats->QAvoidCount;
    stats->ConsumedMsgs = stats->DequeueCount+stats->QAvoidCount;
    if (stats->MaxMessages == 0)
    {
        stats->BufferedPercent = 0;
        stats->BufferedHWMPercent = 0;
    }
    else
    {
        stats->BufferedPercent = ((double)stats->BufferedMsgs * 100.0)/(double)stats->MaxMessages;
        stats->BufferedHWMPercent = ((double)stats->BufferedMsgsHWM * 100.0)/(double)stats->MaxMessages;
    }

    stats->PutsAttemptedDelta = (Q->qavoidCount + Q->enqueueCount + Q->rejectedMsgs) - Q->putsAttempted;

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, "%s Q=%p msgs=%lu",__func__, Q, stats->BufferedMsgs);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Explicitly set the value of the putsAttempted statistic
///
///  @param[in] Qhdl              Queue
///  @param[in] newPutsAttempted  Value to reset the stat to
///  @param[in] setType           The type of stat setting required
///////////////////////////////////////////////////////////////////////////////
void iesq_setStats(ismQHandle_t Qhdl, ismEngine_QueueStatistics_t *stats, ieqSetStatsType_t setType)
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;

    if (setType == ieqSetStats_UPDATE_PUTSATTEMPTED)
    {
        // Only update the puts attempted stat if it is unchanged
        if (Q->putsAttempted == stats->PutsAttempted)
        {
            Q->putsAttempted += stats->PutsAttemptedDelta;
        }
    }
    else
    {
        assert(setType == ieqSetStats_RESET_ALL);

        Q->bufferedMsgsHWM = Q->bufferedMsgs;
        Q->rejectedMsgs = 0;
        Q->enqueueCount = 0;
        Q->dequeueCount = 0;
        Q->qavoidCount = 0;
        Q->putsAttempted = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Determine if there  are messages available to be delivered to /this/ consumer.
///  @param[in]   ismQHandle_t             Queue
///  @param[in]   ismEngine_Consumer_t     pConsumer
///
int32_t iesq_checkAvailableMsgs(ismQHandle_t Qhdl,
                                ismEngine_Consumer_t *pConsumer)
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;

    int32_t rc;

    //This type of queue only has one consumer... so they had better be asking about that one!
    assert(Q->pConsumer == pConsumer);

    if (Q->bufferedMsgs > 0)  //Doesn't include uncommitted puts on simple queue
    {
        rc = OK;
    }
    else
    {
        rc = ISMRC_NoMsgAvail;
    }

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief 
///     Replay Soft Log
///  @remark
///     This function executes a softlog records for Simple Queue 
///     for either Commit or Rollback.
///
///  @param[in]     Type             Softlog entry type
///  @param[in]     Phase            Commit/PostCommit/Rollback/PostRollback
///  @param[in]     hStream          Stream to perform operations on
///  @param[in]     pEntry           Softlog entry
///  @param[in]     pRecord          Transaction state record
///////////////////////////////////////////////////////////////////////////////
void iesq_SLEReplayPut( ietrReplayPhase_t Phase
                      , ieutThreadData_t *pThreadData
                      , ismEngine_Transaction_t *pTran
                      , void *pEntry 
                      , ietrReplayRecord_t *pRecord )
{
    iesqSLEPut_t *pSLE = (iesqSLEPut_t *)pEntry;

    ieutTRACEL(pThreadData, pSLE, ENGINE_FNC_TRACE, FUNCTION_ENTRY "Phase=%d pEntry=%p\n",
               __func__, Phase, pEntry);

    switch (Phase)
    {

        case PostCommit:
            if (! pSLE->bSavepointRolledback)
            {
                // Attempt to deliver the messages
                if (OK != iesq_putMessage( pThreadData
                                         , (ismQHandle_t)pSLE->pQueue
                                         , pSLE->putOptions
                                         , NULL
                                         , pSLE->pMsg
                                         , IEQ_MSGTYPE_INHERIT ))
                {
                    // If we failed, we need to decrement the message usage count which was
                    // incremented during the original put.
                    iem_releaseMessage(pThreadData, pSLE->pMsg);

                    // Also increment the count of messages which failed
                    pRecord->SkippedPutCount++;
                }
            }
            break;

        case Rollback:
            if (! pSLE->bSavepointRolledback)
            {
                iem_releaseMessage(pThreadData, pSLE->pMsg);
            }
            break;

        case SavepointRollback:
            pSLE->bSavepointRolledback = true;
            iem_releaseMessage(pThreadData, pSLE->pMsg);
            break;

        default:
            assert(false);
            break;
    }

    ieutTRACEL(pThreadData, Phase, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Write descriptions of iesq structures to the dump
///
///  @param[in]     dumpHdl  Dump file handle
///////////////////////////////////////////////////////////////////////////////
void iesq_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    iedm_describe_iesqQueue_t(dump->fp);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     During a dump we need to lock the consumer to ensure the pages we
/// are dumping aren't freed from under us.
///
/// @remark We lock the consumer in delivering state which means other threads
/// shouldn't wait for us but as a consequence we must call checkwaiters after
/// unlock
///
///  @param[in]     Q
///  @param[out]    State of the waiter when we locked it or gave up trying to
///////////////////////////////////////////////////////////////////////////////
bool iesq_lockConsumerForDump(iesqQueue_t *Q,
                              iewsWaiterStatus_t *pOldState)
{
    //try to lock the consumer once before we worry about time stamps etc...
    iewsWaiterStatus_t oldState = Q->waiterStatus;
    bool lockedConsumer = false;

    if ((oldState & IEWS_WAITERSTATUSMASK_LOCKED) == 0)
    {
        lockedConsumer = __sync_bool_compare_and_swap( &(Q->waiterStatus)
                                                     , oldState
                                                     , IEWS_WAITERSTATUS_DELIVERING); //Lock the consumer and delivering means we promise to call check waiters....
    }

    //Initial lock didn't work... loop around trying to lock for a while
    if (!lockedConsumer)
    {
        uint64_t starttime = ism_common_currentTimeNanos();
        uint64_t finishtime = 0;
        do
        {
            if ((oldState & IEWS_WAITERSTATUSMASK_LOCKED) == 0)
            {
                lockedConsumer = __sync_bool_compare_and_swap( &(Q->waiterStatus)
                                                               , oldState
                                                               , IEWS_WAITERSTATUS_DELIVERING); //Lock the consumer and delivering means we promise to call check waiters....
            }
            finishtime = ism_common_currentTimeNanos();
        }
        while (   !lockedConsumer
               && (finishtime >= starttime) //system time hasn't changed
               && ((finishtime - starttime) < (3 * 1000000000L))); //and we haven't been trying for 3 seconds
    }

    *pOldState = oldState;

    return lockedConsumer;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Queue Dump
///  @remark
///     This function dumps information about the supplied Queue. The
///     information is written using the supplied dumpHandle, dumpHdl.
///
///  @param[in]     Qhdl     Queue handle
///  @param[in]     dumpHdl  Dump file handle
///////////////////////////////////////////////////////////////////////////////
void iesq_dumpQ( ieutThreadData_t *pThreadData
               , ismQHandle_t  Qhdl
               , iedmDumpHandle_t dumpHdl)
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    if (iedm_dumpStartObject(dump, Q) == true)
    {
        iedm_dumpStartGroup(dump, "SimpleQ");

        // Dump the QName, PolicyInfo and Queue
        if (Q->Common.QName != NULL)
        {
            iedm_dumpData(dump, "QName", Q->Common.QName,
                          iere_usable_size(iemem_simpleQ, Q->Common.QName));
        }
        iepi_dumpPolicyInfo(Q->Common.PolicyInfo, dumpHdl);
        iedm_dumpData(dump, "iesqQueue_t", Q, iere_usable_size(iemem_simpleQ, Q));



        int32_t detailLevel = dump->detailLevel;

        if (detailLevel >= iedmDUMP_DETAIL_LEVEL_7)
        {
            //Lock the consumer so we can walk the pages safely
            iewsWaiterStatus_t previousState = IEWS_WAITERSTATUS_DISCONNECTED;

            bool lockedConsumer = iesq_lockConsumerForDump(Q, &previousState);


            if (lockedConsumer)
            {
                // Dump the consumer
                dumpConsumer(pThreadData, Q->pConsumer, dumpHdl);

                //Dump out the state of the consumer before we locked it
                iedm_dumpData(dump, "iewsWaiterStatus_t", &previousState,   sizeof(previousState));

                // Dump a subset of pages
                iesqQNodePage_t *pDisplayPageStart[2];
                int32_t maxPages, pageNum;

                // Detail level less than 9, we only dump a set of pages at the head and tail
                if (detailLevel < iedmDUMP_DETAIL_LEVEL_9)
                {
                    pageNum = 0;
                    maxPages = 3;

                    pDisplayPageStart[0] = Q->headPage;
                    pDisplayPageStart[1] = Q->headPage;

                    for (iesqQNodePage_t *pPage = pDisplayPageStart[0]; pPage != NULL; pPage = pPage->next)
                    {
                        if (++pageNum > maxPages) pDisplayPageStart[1] = pDisplayPageStart[1]->next;
                    }
                }
                else
                {
                    maxPages = 0;
                    pDisplayPageStart[0] = Q->headPage;
                    pDisplayPageStart[1] = NULL;
                }

                // Go through the lists of pages dumping maxPages of each
                for(int32_t i=0; i<2; i++)
                {
                    iesqQNodePage_t *pDumpPage = pDisplayPageStart[i];

                    pageNum = 0;

                    while(pDumpPage != NULL)
                    {
                        pageNum++;

                        // Actually dump the page
                        iedm_dumpData(dump, "iesqQNodePage_t", pDumpPage,
                                      iere_usable_size(iemem_simpleQPage, pDumpPage));

                        // If user data is requested, run through the nodes dumping it
                        if (Q->bufferedMsgs != 0 && dump->userDataBytes != 0)
                        {
                            iedm_dumpStartGroup(dump, "Available Msgs");

                            uint32_t nodesInPage = pDumpPage->nodesInPage;

                            for(uint32_t nodeNum = 0; nodeNum < nodesInPage; nodeNum++)
                            {
                                iesqQNode_t *node = &(pDumpPage->nodes[nodeNum]);

                                if (node->msg != NULL)
                                {
                                    iem_dumpMessage(node->msg, dumpHdl);
                                }
                            }

                            iedm_dumpEndGroup(dump);
                        }

                        // If the one we've just dumped for the head is the start of the tail,
                        // move the start of the tail on.
                        if (i == 0 && pDumpPage == pDisplayPageStart[1])
                        {
                            pDisplayPageStart[1] = pDisplayPageStart[1]->next;
                        }

                        if (pageNum == maxPages) break;

                        pDumpPage = pDumpPage->next;
                    }
                }

                //Finally unlock the consumer and then call checkwaiters (as we had the consumer locked in delivering)
                bool unlocked = __sync_bool_compare_and_swap( &(Q->waiterStatus)
                                                            , IEWS_WAITERSTATUS_DELIVERING
                                                            , previousState);

                //If we couldn't unlock it, it's because someone has flagged we need to something...
                if (!unlocked)
                {
                    //NoDeliver as we're about to call checkWaiters anyhow..
                    ieq_completeWaiterActions(pThreadData,
                                              Qhdl,
                                              Q->pConsumer,
                                              IEQ_COMPLETEWAITERACTION_OPT_NODELIVER,
                                              (previousState == IEWS_WAITERSTATUS_ENABLED));
                }
                (void)iesq_checkWaiters(pThreadData, Qhdl, NULL);
            }
            else
            {
                //Dump the state of the consumer that we couldn't lock....
                //Dump out the state of the consumer before we locked it
                iedm_dumpData(dump, "iewsWaiterStatus_t", &previousState,   sizeof(previousState));
            }
        }

        iedm_dumpEndGroup(dump);
        iedm_dumpEndObject(dump, Q);
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Return a handle to the store definition object, if there is one.
///
///  @return                       - Store record handle
///////////////////////////////////////////////////////////////////////////////
ismStore_Handle_t iesq_getDefnHdl( ismQHandle_t Qhdl )
{
    return ((iesqQueue_t *)Qhdl)->hStoreObj;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Return a handle to the store properties object, if there is one.
///
///  @return                       - Store record handle
///////////////////////////////////////////////////////////////////////////////
ismStore_Handle_t iesq_getPropsHdl( ismQHandle_t Qhdl )
{
    return ((iesqQueue_t *)Qhdl)->hStoreProps;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Update the QPR / SPR of a queue.
///  @remark
///     This function can be called to set a new queue properties record for
///     a queue.
///////////////////////////////////////////////////////////////////////////////
void iesq_setPropsHdl( ismQHandle_t Qhdl,
                       ismStore_Handle_t propsHdl )
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;

    if (propsHdl != ismSTORE_NULL_HANDLE)
    {
        Q->hStoreProps = propsHdl;
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Update the properties of a queue including update of the QPR if required.
///  @remark
///     This function can be called to set new properties of a queue, if a store
///     handle is specified (e.g. during recovery) or if an update requires a
///     change to the store, the queue's store handle is updated.
///
///  @return                       - OK
///////////////////////////////////////////////////////////////////////////////
int32_t iesq_updateProperties( ieutThreadData_t *pThreadData,
                               ismQHandle_t Qhdl,
                               const char *pQName,
                               ieqOptions_t QOptions,
                               ismStore_Handle_t propsHdl,
                               iereResourceSetHandle_t resourceSet )
{
    int32_t rc = OK;
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    bool storeChange = false;
    iereResourceSetHandle_t currentResourceSet = Q->Common.resourceSet;

    // Deal with any updates to Queue name
    if (pQName != NULL)
    {
        if (NULL == Q->Common.QName || strcmp(pQName, Q->Common.QName) != 0)
        {
            iere_primeThreadCache(pThreadData, currentResourceSet);
            char *newQName = iere_realloc(pThreadData,
                                          currentResourceSet,
                                          IEMEM_PROBE(iemem_simpleQ, 3),
                                          Q->Common.QName, strlen(pQName)+1);

            if (newQName == NULL)
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto mod_exit;
            }

            Q->Common.QName = newQName;

            strcpy(Q->Common.QName, pQName);

            // Can only make a store change for point-to-point queues
            storeChange = ((Q->QOptions & ieqOptions_PUBSUB_QUEUE_MASK) == 0);
        }
    }
    else
    {
        if (Q->Common.QName != NULL)
        {
            iere_primeThreadCache(pThreadData, currentResourceSet);
            iere_free(pThreadData, currentResourceSet, iemem_simpleQ, Q->Common.QName);
            Q->Common.QName = NULL;
            // Can only make a store change for point-to-point queues
            storeChange = ((Q->QOptions & ieqOptions_PUBSUB_QUEUE_MASK) == 0);
        }
    }

    // If we are still in recovery, we can update the queue options since they
    // are dealt with when recovery is completed - the queue options are not
    // stored.
    if (Q->QOptions & ieqOptions_IN_RECOVERY)
    {
        assert((QOptions & ieqOptions_IN_RECOVERY) != 0);
        Q->QOptions = QOptions;
    }

    // If the queue has changed, update the QPR in the store.
    if (storeChange && (Q->hStoreProps != ismSTORE_NULL_HANDLE))
    {
        // propsHdl should only be non-null during restart, and only the first
        // time this queue is discovered at restart.
        assert(propsHdl == ismSTORE_NULL_HANDLE);
        assert(pThreadData != NULL);

        rc = iest_updateQueue(pThreadData,
                              Q->hStoreObj,
                              Q->hStoreProps,
                              Q->Common.QName,
                              &propsHdl);

        if (rc != OK) goto mod_exit;

        assert(propsHdl != ismSTORE_NULL_HANDLE && propsHdl != Q->hStoreProps);
    }

    // Update the properties handle (this does nothing if it is ismSTORE_NULL_HANDLE)
    iesq_setPropsHdl(Qhdl, propsHdl);

    // Update the resourceSet for the queue
    iesq_updateResourceSet(pThreadData, Q, resourceSet);

mod_exit:

    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Update the resource set for a queue that currently has none.
///////////////////////////////////////////////////////////////////////////////
static inline void iesq_updateResourceSet( ieutThreadData_t *pThreadData
                                         , iesqQueue_t *Q
                                         , iereResourceSetHandle_t resourceSet)
{
    if (resourceSet != iereNO_RESOURCE_SET)
    {
        assert(Q->Common.resourceSet == iereNO_RESOURCE_SET);

        Q->Common.resourceSet = resourceSet;

        iere_primeThreadCache(pThreadData, resourceSet);

        // Update memory for the Queue
        int64_t fullMemSize = (int64_t)iere_full_size(iemem_simpleQ, Q);
        iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_simpleQ, 4), Q, fullMemSize);

        if (Q->Common.QName)
        {
            fullMemSize = (int64_t)iere_full_size(iemem_simpleQ, Q->Common.QName);
            iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_simpleQ, 5), Q->Common.QName, fullMemSize);
        }

        // Update memory for the head page.
        fullMemSize = (int64_t)iere_full_size(iemem_simpleQPage, Q->headPage);
        iere_updateMem(pThreadData, resourceSet, IEMEM_PROBE(iemem_simpleQPage, 2), Q->headPage, fullMemSize);

        // No messages on a simpleQ...
        assert(Q->bufferedMsgs == 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Scan the queue looking for expired msgs to reap
///
///  @param[in] Q                - Q to scan
///  @param[in] nowExpire        - current time for expiry purposes
///  @param[in] forcefullscan    - don't trust per queue expiry data, always full scan
///  @param[in] expiryListLocked - If set, must NOT lock the waiter in a state
///                                that causes us to call checkwaiters and drive
///                                message delivery as that may need to remove Q from list
///
///  @remark expiryListLocked is so that (by setting it) we can avoid a deadlock:
///            reaper calls this function with expiry list lock held but checkwaiters
///            may deliver all expired messages and try and remove Q from list
///
///  @return                       - OK or ISMRC... inc:
///                                      LockNotGranted: couldn't get expiry lock
///////////////////////////////////////////////////////////////////////////////
ieqExpiryReapRC_t  iesq_reapExpiredMsgs( ieutThreadData_t *pThreadData
                                       , ismQHandle_t Qhdl
                                       , uint32_t nowExpire
                                       , bool forcefullscan
                                       , bool expiryListLocked)
{
    bool reapComplete=false;
    ieqExpiryReapRC_t rc = ieqExpiryReapRC_OK;
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    iewsWaiterStatus_t oldStatus = IEWS_WAITERSTATUS_DISCONNECTED;
    iewsWaiterStatus_t newStatus = IEWS_WAITERSTATUS_DISCONNECTED;
    iemeQueueExpiryData_t *pQExpiryData = (iemeQueueExpiryData_t *)Q->Common.QExpiryData;

    ieutTRACEL(pThreadData,Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p \n", __func__,Q);

    bool gotWaiterLock = iews_tryLockForQOperation( &(Q->waiterStatus)
                                                  , &oldStatus
                                                  , &newStatus
                                                  , !expiryListLocked);

    if (!gotWaiterLock)
    {
        //We don't report lock not granted in this case, as someone else
        //was actively using the queue, we don't want to cause a new expiry scan
        //soon.
        //If we don't allow message delivery we could increase predeletecount of Q
        //then issue a return code asking to be redriven allowing it (and then remember
        //to decrease the predeletecount
        goto mod_exit;
    }

    bool gotExpiryLock = ieme_startReaperQExpiryScan( pThreadData, (ismEngine_Queue_t *)Q);

    if (!gotExpiryLock)
    {
        iews_unlockAfterQOperation( pThreadData
                                      , Qhdl
                                      , Q->pConsumer
                                      , &(Q->waiterStatus)
                                      , newStatus
                                      , oldStatus);

        rc = ieqExpiryReapRC_NoExpiryLock;
        goto mod_exit;
    }



    if (!forcefullscan)
    {
        //Can we remove just the select messages in the array
        for (uint32_t i = 0; i < pQExpiryData->messagesInArray; i++)
        {
            if (pQExpiryData->earliestExpiryMessages[i].Expiry > nowExpire)
            {
                //We've found a message not due to expire yet... we've done all messages we need to
                if (i > 0)
                {
                    pQExpiryData->messagesInArray -= i;

                    memmove( &(pQExpiryData->earliestExpiryMessages[0])
                           , &(pQExpiryData->earliestExpiryMessages[i])
                           , pQExpiryData->messagesInArray * sizeof(iemeBufferedMsgExpiryDetails_t));
                }

                reapComplete = true;
                break;
            }
            else
            {
                //We need to remove this message
                iesq_expireNode( pThreadData
                               , Q
                               , (iesqQNode_t *)pQExpiryData->earliestExpiryMessages[i].qnode);

                pQExpiryData->messagesWithExpiry--;
                pThreadData->stats.bufferedExpiryMsgCount--;

                if (pQExpiryData->messagesInArray == (i+1))
                {
                    //We've expired as many messages as are in the array
                    pQExpiryData->messagesInArray = 0;

                    if (pQExpiryData->messagesWithExpiry == 0)
                    {
                        reapComplete = true;
                    }
                    break;
                }
            }
        }
    }

    if (!reapComplete)
    {
        //do full scan of queue rebuilding expiry data
        iesq_fullExpiryScan(pThreadData, Q, nowExpire);
    }

    if (pQExpiryData->messagesWithExpiry == 0)
    {
        if (expiryListLocked)
        {
            rc = ieqExpiryReapRC_RemoveQ;
        }
        else
        {
            ieme_removeQueueFromExpiryReaperList( pThreadData
                                                , (ismEngine_Queue_t *)Q);
        }
    }

    ieme_endReaperQExpiryScan(pThreadData, (ismEngine_Queue_t *)Q);

    //See if we can move the get cursor past expired messages, freeing pages */
    iesq_scanGetCursor(pThreadData, Q);

    iews_unlockAfterQOperation( pThreadData
                                  , Qhdl
                                  , Q->pConsumer
                                  , &(Q->waiterStatus)
                                  , newStatus
                                  , oldStatus);

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///     Complete the importing of a queue
///
///  @remark Dummy function as this queue does not have a complete imorting step
///
///////////////////////////////////////////////////////////////////////////////
int32_t iesq_completeImport( ieutThreadData_t *pThreadData
                           , ismQHandle_t Qhdl)
{
    return OK;
}
/// @}

/// @{
/// @name Internal Functions

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Return pointer to nodePage structure from final Node in array
///  @remarks
///    The iesqQNodePage_t structure contains an array of iesqQNode_t
///    structures. This function accepts a pointer top the last
///    iesqQNode_t entry in the array and returns a pointer to the
///    iesqQNodePage_t structure.
///  @param[in] node               - points to the end of page marker
///  @return                       - pointer to the CURRENT page
///////////////////////////////////////////////////////////////////////////////
static inline iesqQNodePage_t *iesq_getPageFromEnd( iesqQNode_t *node )
{
    iesqQNodePage_t *page;
    uint32_t nodesInPage;

    //Check we've been given a pointer to the end of the page
    assert( node->msg == MESSAGE_STATUS_ENDPAGE );

    nodesInPage = *(uint32_t *)(node +1);

    page =   (iesqQNodePage_t *)(  (char *)node
                                  - offsetof(iesqQNodePage_t,nodes)
                                  - (nodesInPage * sizeof(iesqQNode_t)));
    ismEngine_CheckStructId(page->StrucId, IESQ_PAGE_STRUCID, ieutPROBE_001);
    return page;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Allocate and initialise a new queue page
///  @remarks
///    Allocate (malloc) the storage for a new iesqQNodePage_t structure
///    and initialise it as an 'empty' page. This function does not chain
///    the allocate page onto the queue.
///
/// @param[in] Q                   - Queue handle
/// @return                        - NULL if a failure occurred or a pointer
///                                  to newly created iesqQNodePage_t
///////////////////////////////////////////////////////////////////////////////

#define iesq_getNodesInPage2(_page) (uint32_t *)(((char *)((_page)+1)) + (sizeof(iesqQNode_t) * (_page)->nodesInPage))

static inline iesqQNodePage_t *iesq_createNewPage( ieutThreadData_t *pThreadData
                                                 , iesqQueue_t *Q
                                                 , uint32_t nodesInPage )
{
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    iere_primeThreadCache(pThreadData, resourceSet);
    size_t pageSize = offsetof(iesqQNodePage_t, nodes)
                    + (sizeof(iesqQNode_t) * (nodesInPage+1))
                    + sizeof(uint32_t);
    iesqQNodePage_t *page = (iesqQNodePage_t *)iere_calloc(pThreadData,
                                                           resourceSet,
                                                           IEMEM_PROBE(iemem_simpleQPage, 1),
                                                           1, pageSize);

    if (page != NULL)
    {
        //Add the end of page marker to the page
        ismEngine_SetStructId(page->StrucId, IESQ_PAGE_STRUCID);
        page->nodes[nodesInPage].msg = MESSAGE_STATUS_ENDPAGE;
        page->nodesInPage = nodesInPage;
        *iesq_getNodesInPage2(page) = nodesInPage;

        ieutTRACEL(pThreadData, pageSize, ENGINE_FNC_TRACE, FUNCTION_IDENT "Q %p, size %lu (nodes %u)\n",
                   __func__, Q, pageSize, nodesInPage);
    }
    else
    {
        ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_IDENT "Q %p, size %lu - no mem\n",__func__, Q, pageSize);
    }
    return page;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    return pointer to next iesqQNodePage_t structure in queue
///  @remarks
///    Given a pointer to an end of page marker, return a pointer to the next
///    page. If the next page has not yet been chained in, we wait for the
///    thread that should do it.
///    If the thread which should have created the pages failed to create
///    the page we try to create the page ourselves.
///
///  @param[in] Q                  - Queue handle
///  @param[in] endMsg             - Pointer to last message on a page
///  @return                       - pointer to the next page or NULL
///////////////////////////////////////////////////////////////////////////////
static inline iesqQNodePage_t *iesq_moveToNewPage(ieutThreadData_t *pThreadData,
                                                  iesqQueue_t *Q,
                                                  iesqQNode_t *endMsg)
{
    DEBUG_ONLY bool result;
    iesqQNodePage_t *currpage = iesq_getPageFromEnd(endMsg);

    if (currpage->next != NULL)
    {
        return currpage->next;
    }

#if TRACETIMESTAMP_MOVETONEWPAGE
    uint64_t TTS_Start_MoveToNewPage = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Start_MoveToNewPage);
#endif

    // Hmmm... no new page yet! Someone should be adding it, wait while they do
    while (currpage->next == NULL)
    {
        if (currpage->nextStatus == failed)
        {
            ieutTRACEL(pThreadData, currpage, ENGINE_NORMAL_TRACE,
                       "%s: noticed next page addition to %p has not occurred\n",
                       __func__, currpage);

            // The person who should add the page failed, are we the one to fix it?
            if (__sync_bool_compare_and_swap(&(currpage->nextStatus), failed, repairing))
            {
                // We have marked the status as repairing - we get to do it
                 uint32_t nodesInPage =  iesq_choosePageSize();
                iesqQNodePage_t *newpage = iesq_createNewPage( pThreadData
                                                             , Q
                                                             , nodesInPage );
                ieqNextPageStatus_t newStatus;

                if (newpage != NULL)
                {
                    currpage->next = newpage;
                    newStatus = unfinished;

                    ieutTRACEL(pThreadData, newpage, ENGINE_NORMAL_TRACE,
                               "%s: successful new page addition to Q %p currpage %p newPage %p\n",
                               __func__, Q, currpage, newpage);
                }
                else
                {
                    // We couldn't create a page either
                    ieutTRACEL(pThreadData, currpage, ENGINE_WORRYING_TRACE,
                               "%s: failed new page addition to Q %p currpage %p\n",
                               __func__, Q, currpage);

                    newStatus = failed;
                }
               
                result= __sync_bool_compare_and_swap(&(currpage->nextStatus), repairing, newStatus);
                // Assert that we can succesfully took this queue out of
                // repairing state.
                assert (result);

                break; // Leave the loop
            }
        }
        // We need the memory barrier here to ensure we see the change
        // made to 'currpage->next' by an other thread.
        ismEngine_ReadMemoryBarrier();
    }

#if TRACETIMESTAMP_MOVETONEWPAGE
    uint64_t TTS_Stop_MoveToNewPage = ism_engine_fastTimeUInt64();
    ieutTRACE_HISTORYBUF( pThreadData, TTS_Stop_MoveToNewPage);
#endif

    return currpage->next;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Moves head cursor passed messages that have been removed (e.g. expired
///    messages) freeing unneeded queue pages
///
///  @remark The waiterlock must be held by the caller of this function!
///
///  @param[in] Q                  - Queue to check head pointer on
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////

static void iesq_scanGetCursor( ieutThreadData_t *pThreadData
                              , iesqQueue_t *Q)
{
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    ieutTRACEL(pThreadData,Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n", __func__,Q);

    while(Q->head->msg == MESSAGE_STATUS_REMOVED)
    {
        //Find the next node
        iesqQNode_t *nextNode = Q->head+1;

        if(nextNode->msg == MESSAGE_STATUS_ENDPAGE)
        {
            //Yep...move to the next page if it has been added...
            //...if it hasn't been added someone will add it shortly
            iesqQNodePage_t *currpage = Q->headPage;

            if(currpage->nextStatus == completed)
            {
                //Another page has been added so the head can be moved to it
                Q->headPage = currpage->next;
                Q->head     = &(Q->headPage->nodes[0]);
                //free the current page
                iere_primeThreadCache(pThreadData, resourceSet);
                iere_freeStruct(pThreadData, resourceSet, iemem_simpleQPage, currpage, currpage->StrucId);
            }
            else
            {
                //Next page not yet ready, we need to leave the cursor on this one
                break;
            }
        }
        else
        {
            Q->head = nextNode;
        }
    }

    ieutTRACEL(pThreadData, Q, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Gets the first message from the head of the queue
///  @remarks
///    This function returns a pointer to the next message to be delivered.
///    @par
///    This function is only called when bracketed by CASing the waiter
///    state to 'getting'. This CASing acts like a lock&barriers so we can
///    use a simple increment on the cursor pointer.
///
///  @param[in] Q                  - Queue to get the message from
///  @param[out] outmsg            - On successful completion, the message
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static int32_t iesq_getMessage( ieutThreadData_t *pThreadData
                              , iesqQueue_t *Q
                              , ismEngine_Message_t **outmsg
                              , ismMessageHeader_t *pmsgHdr)
{
   int32_t rc = OK;
   uint64_t nodeExpiryOId;
   ismEngine_Message_t *msg;
   uint8_t nodeMsgFlags;
   iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

   ieutTRACEL(pThreadData,Q,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n", __func__,Q);

   //Find a message and update the head to point after it
   do
   {
       //Record a pointer to the first message
       msg   = Q->head->msg;
       nodeMsgFlags = Q->head->msgFlags;
       nodeExpiryOId = (uint64_t)Q->head;

       if (msg == NULL)
       {
           //We've come to the end of the queue...
           *outmsg = NULL;
           rc = ISMRC_NoMsgAvail;
       }
       else
       {
           if (msg == MESSAGE_STATUS_REMOVED)
           {
               msg = NULL; //Not a valid message, look at the next message
           }

           //Try and update head pointer to the next slot
           iesqQNode_t *newHead = Q->head+1;

           //Have we run out of space on this page?
           if(newHead->msg == MESSAGE_STATUS_ENDPAGE)
           {
               //Yep...move to the next page if it has been added...
               //...if it hasn't been added someone will add it shortly
               iesqQNodePage_t *currpage = Q->headPage;

               if(currpage->nextStatus == completed)
               {
                   //Another page has been added so the head can be moved to it
                   Q->headPage = currpage->next;
                   Q->head     = &(Q->headPage->nodes[0]);
                   //free the current page
                   iere_primeThreadCache(pThreadData, resourceSet);
                   iere_freeStruct(pThreadData, resourceSet, iemem_simpleQPage, currpage, currpage->StrucId);
               }
               else
               {
                   //Another page has not yet been added so the
                   //put of the last item on this page is not yet complete
                   //(as part of that put marks the page complete...)
                   *outmsg = NULL;
                   rc = ISMRC_NoMsgAvail;

               }
           }
           else
           {
               //The next item is another message, move the head to point to it
               Q->head = newHead;
           }
       }
   }
   while ( (rc == OK) && (msg == NULL));

   //If the queue isn't empty and we grabbed a message
   if (rc == OK)
   {
       if (msg->Header.Expiry != 0)
       {
           ieme_removeMessageExpiryData( pThreadData, (ismEngine_Queue_t *)Q,  nodeExpiryOId);
       }

       // Copy the message header, and update the msgFlags from the node
       if (pmsgHdr != NULL)
       {
           *pmsgHdr =  msg->Header;
           pmsgHdr->Flags |= nodeMsgFlags;
       }

       //Let the caller know the message they've been given
       *outmsg = msg;

       //Even though we are protected by the waiterStatus CAS lock,
       //We still need to atomically change the message count as that
       //is altered outside the waiterStatus CAS (during put)
       DEBUG_ONLY int32_t oldDepth;
       iere_primeThreadCache(pThreadData, resourceSet);
       iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
       pThreadData->stats.bufferedMsgCount--;
       oldDepth = __sync_fetch_and_sub(&(Q->bufferedMsgs), 1);
       assert(oldDepth > 0);
   }

   ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
   return rc;
}



///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Attempt to deliver messages to the consumer
///  @remarks
///    This function attempts to deliver message from the queue to the
///    consumer. In order to do this it must change the state of the
///    consumer from enabled to getting.
///    If the consumer is already in getting state then we loop round
///    until it's state changes.
///    If the consumer is in any other state then this function takes no
///    further action.
///
///  @param[in] q                  - Queue
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t iesq_checkWaiters( ieutThreadData_t *pThreadData
                         , ismQHandle_t Qhdl
                         , ismEngine_AsyncData_t *asyncInfo)
{
    int32_t rc = OK;
    bool loopAgain = true;
    iesqQueue_t *q = (iesqQueue_t *)Qhdl;
    
    ieutTRACEL(pThreadData,q,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY " Q=%p\n", __func__,q);

    if (q->bufferedMsgs == 0)
    {
        //There appear to be no messages on this queue, anyone who adds a message will
        //call checkWaiters themselves so we don't need to use a memory barrier to check this number or
        //anything... we're done.
        goto mod_exit;
    }

    // We loop giving messages until we want out of messages to give or
    // the waiter is no longer enabled (e.g. because someone else has it
    // whilst they are in the message received callback)
    do
    {
        loopAgain = false;

        iewsWaiterStatus_t oldStatus =  iews_val_tryLockToState( &(q->waiterStatus)
                                                               , IEWS_WAITERSTATUS_ENABLED
                                                               , IEWS_WAITERSTATUS_GETTING);

        if (oldStatus == IEWS_WAITERSTATUS_ENABLED)
        {
            // waiter is now in getting state for us to get
            ismEngine_Message_t *msg = NULL;
            ismMessageHeader_t msgHdr;

            // If someone else changes the waiter state after we moved
            // it out of enabled, we'll set this to true
            bool waiterStateAlteredElsewhere = false;

            rc = iesq_getMessage( pThreadData, q , &msg, &msgHdr );
            if (OK == rc)
            {
                //Increase the count of messages got
                q->dequeueCount++;

                bool reenableWaiter = false;

                //Does the user explicitly call suspend or do we disable if the return code is false
                bool fExplicitSuspends = q->pConsumer->pSession->fExplicitSuspends;

                // We've got a message for this waiter...change our state to
                // reflect that If someone else has disabled/enabled us...
                // continue anyway for the moment. We'll notice when we try
                // and enable/disable the waiter after the callback
                iews_bool_changeState( &(q->waiterStatus)
                                     , IEWS_WAITERSTATUS_GETTING
                                     , IEWS_WAITERSTATUS_DELIVERING);

                reenableWaiter = ism_engine_deliverMessage(
                                    pThreadData,
                                    q->pConsumer,
                                    NULL,
                                    msg,
                                    &msgHdr,
                                    ismMESSAGE_STATE_CONSUMED,
                                    0);

                if (reenableWaiter)
                {
                    oldStatus = iews_val_tryUnlockNoPending( &(q->waiterStatus)
                                                           , IEWS_WAITERSTATUS_DELIVERING
                                                           , IEWS_WAITERSTATUS_ENABLED);

                    if (oldStatus == IEWS_WAITERSTATUS_DELIVERING)
                    {
                        //We re-enabled it so it's our respnsibility to check for more messages
                        loopAgain = true;
                    }
                    else
                    {
                        waiterStateAlteredElsewhere = true; //Someone else has changed it
                    }
                }
                else
                {
                    if (!fExplicitSuspends)
                    {
                        iews_addPendFlagWhileLocked( &(q->waiterStatus)
                                                   , IEWS_WAITERSTATUS_DISABLE_PEND);
                    }
                    waiterStateAlteredElsewhere = true; //We've added a pend flag... need to action it
                }
            }
            else /* if (rc == ISMRC_NoMsgAvail) */
            {
                // Only possible return code is ISMRC_NoMsgAvail
                assert(rc == ISMRC_NoMsgAvail);

                // If we are supposed to be informing the engine when the
                // queue hits empty, do it now. Note we have no barriers or
                // atomic sets etc... so we /might/ make this call twice
                if(q->Common.informOnEmpty)
                {
                    q->Common.informOnEmpty = false;

                    ism_engine_deliverStatus( pThreadData
                                            , q->pConsumer
                                            , ISMRC_NoMsgAvail );
                }

                // If we reported queue full in the past, let's reset the flag
                // now that the queue is empty again.
                q->ReportedQueueFull = false;

                //No more messages...turn the waiter back on
                oldStatus = iews_val_tryUnlockNoPending( &(q->waiterStatus)
                                                       , IEWS_WAITERSTATUS_GETTING
                                                       , IEWS_WAITERSTATUS_ENABLED);

                if (oldStatus != IEWS_WAITERSTATUS_GETTING)
                {
                    waiterStateAlteredElsewhere = true; //Someone else has changed it
                }

                rc = OK;
                loopAgain = false;
            }

            if (waiterStateAlteredElsewhere)
            {
                //Someone requested a callback whilst we had the waiter in getting or got,
                //It's our responsibility to fire the callback(s)
                ieq_completeWaiterActions(pThreadData
                                         , (ismQHandle_t)q
                                         , q->pConsumer
                                         , IEQ_COMPLETEWAITERACTION_OPT_NODELIVER
                                         , true);

                //We did OPT_NODELIVER (to help to avoid recursively calling this function)
                //so we need to loop again...
                loopAgain = true;
            }
        }
        else if(oldStatus == IEWS_WAITERSTATUS_GETTING)
        {
            //We've found a valid waiter, his get might fail as it was fractionally too early whereas
            //ours would succeed so we'll loop around again and give him another chance
            loopAgain = true;
        }
    }
    while ((rc == OK) && loopAgain);

mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

static inline uint32_t iesq_choosePageSize( void )
{
    return ((ismEngine_serverGlobal.totalSubsCount > ieqNUMSUBS_HIGH_CAPACITY_THRESHOLD) ?
                               iesqPAGESIZE_HIGHCAPACITY : iesqPAGESIZE_DEFAULT);
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Attempt to deliver message directly to consumer
///  @remarks
///    This function is called directly from the iesq_putMessage function
///    to attempt to deliver a message directly to the conusmer. The
///    consumer must be in enabled state in order for the message to be
///    delivered.
///
///  @param[in] q                  - Queue
///  @param[in] msg                - Message
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
static int32_t iesq_putToWaitingGetter( ieutThreadData_t *pThreadData
                                      , iesqQueue_t *q
                                      , ismEngine_Message_t *msg
                                      , uint8_t msgFlags )
{
    int32_t rc = OK;
    bool deliveredMessage = false;
    bool loopAgain = true;

    ieutTRACEL(pThreadData,q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p \n", __func__,q);

    do
    {
        loopAgain = false;

        iewsWaiterStatus_t oldStatus =  iews_val_tryLockToState( &(q->waiterStatus)
                                                               , IEWS_WAITERSTATUS_ENABLED
                                                               , IEWS_WAITERSTATUS_DELIVERING);

        if(oldStatus == IEWS_WAITERSTATUS_ENABLED)
        {
            bool reenableWaiter = false;
            ismMessageHeader_t msgHdr = msg->Header;

            // Include any additional flags
            msgHdr.Flags |= msgFlags;

            // Update the record for the number of messages delivered directly
            // to the consumer
            q->qavoidCount++;

            //Does the user explicitly call suspend or do we disable if the return code is false
            bool fExplicitSuspends = q->pConsumer->pSession->fExplicitSuspends;

            reenableWaiter = ism_engine_deliverMessage(
                      pThreadData
                    , q->pConsumer
                    , NULL
                    , msg
                    , &msgHdr
                    , ismMESSAGE_STATE_CONSUMED
                    , 0 );

            if (reenableWaiter)
            {
                oldStatus = iews_val_tryUnlockNoPending( &(q->waiterStatus)
                                                       , IEWS_WAITERSTATUS_DELIVERING
                                                       , IEWS_WAITERSTATUS_ENABLED );

                // If someone requested a callback whilst we had
                // the waiter in state: got, It's our responsibility to fire
                // the callback
                if (oldStatus != IEWS_WAITERSTATUS_DELIVERING)
                {
                    ieq_completeWaiterActions( pThreadData
                                             , (ismQHandle_t)q
                                             , q->pConsumer
                                             , IEQ_COMPLETEWAITERACTION_OPT_NODELIVER //Going to call checkWaiters in a sec
                                             , true);
                }
            }
            else
            {
                if (!fExplicitSuspends)
                {
                    iews_addPendFlagWhileLocked( &(q->waiterStatus)
                                               , IEWS_WAITERSTATUS_DISABLE_PEND);
                }

                ieq_completeWaiterActions( pThreadData
                                         , (ismQHandle_t)q
                                         , q->pConsumer
                                         , IEQ_COMPLETEWAITERACTION_OPT_NODELIVER //Going to call checkWaiters in a sec
                                         , true);
            }

            // We've successfully delivered the message, we're done
            deliveredMessage = true;
            break;
        }
        else if (oldStatus == IEWS_WAITERSTATUS_GETTING)
        {
            // We've found a valid waiter, his get might fail as it was
            // fractionally too early whereas ours would succeed so we'll
            // loop around again and give him another chance
            loopAgain = true;
        }
    }
    while (loopAgain);

    // While we had the waiter in got state it may have missed puts so
    // check waiters
    if (deliveredMessage)
    {
        if (q->bufferedMsgs > 0)
        {
            // The only valid return codes from this function is OK
            // or ISMRC_NoAvailWaiter. If checkWaiters encounters a
            // problem we do not care.
            (void) iesq_checkWaiters(pThreadData, (ismQHandle_t)q, NULL);
        }
    }
    else
    {
        rc = ISMRC_NoAvailWaiter;
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d,Q=%p\n", __func__, rc,q);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Mark a node as expired
///
///  @param[in] q                  - Queue
///  @param[in] qnode              - Node
///  @return                       - void
///////////////////////////////////////////////////////////////////////////////
static inline void iesq_expireNode( ieutThreadData_t *pThreadData
                                  , iesqQueue_t *Q
                                  , iesqQNode_t *qnode)
{
    iereResourceSetHandle_t resourceSet = Q->Common.resourceSet;

    iem_releaseMessage(pThreadData, qnode->msg);
    qnode->msg = MESSAGE_STATUS_REMOVED;

    DEBUG_ONLY int32_t oldDepth;
    iere_primeThreadCache(pThreadData, resourceSet);
    iere_updateInt64Stat(pThreadData, resourceSet, ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_BUFFEREDMSGS, -1);
    pThreadData->stats.bufferedMsgCount--;
    oldDepth = __sync_fetch_and_sub(&(Q->bufferedMsgs), 1);
    assert(oldDepth > 0);

    Q->expiredMsgs++;
    pThreadData->stats.expiredMsgCount++;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Scan entire queue looking for messages with expiry set (and removing
///    expired messages.
///
///  @remarks
///    *MUST* already have expiry lock (i.e. after ieme_startReaperQExpiryScan)
///
///  @param[in] q                  - Queue
///  @param[in] nowExpire          - time that means means have expired
///
///  @return                       - void
///////////////////////////////////////////////////////////////////////////////
static inline void iesq_fullExpiryScan( ieutThreadData_t *pThreadData
                                      , iesqQueue_t *Q
                                      , uint32_t nowExpire)
{
    ieutTRACEL(pThreadData,Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "Q=%p \n", __func__,Q);

    //Reset expiry data
    ieme_clearQExpiryDataPreLocked( pThreadData
                               , (ismEngine_Queue_t *)Q);

    iesqQNode_t *currNode = Q->head;

    while (currNode->msg != NULL)
    {
        ismEngine_Message_t *msg = currNode->msg;

        if (msg != MESSAGE_STATUS_ENDPAGE)
        {
            if (msg != MESSAGE_STATUS_REMOVED)
            {
                uint32_t msgExpiry = msg->Header.Expiry;

                if (msgExpiry != 0)
                {
                    if (msgExpiry < nowExpire)
                    {
                        iesq_expireNode( pThreadData
                                       , Q
                                       , currNode);
                    }
                    else
                    {
                        iemeBufferedMsgExpiryDetails_t msgExpiryData = { (uint64_t)currNode , currNode, msgExpiry };

                        ieme_addMessageExpiryPreLocked( pThreadData
                                                      , (ismEngine_Queue_t *)Q
                                                      , &msgExpiryData
                                                      , true);
                    }
                }
            }
            currNode = currNode + 1;
        }
        else
        {
            iesqQNodePage_t *currpage=iesq_getPageFromEnd(currNode);

            if (currpage->nextStatus == completed)
            {
                iesqQNodePage_t *nextPage=iesq_moveToNewPage(pThreadData, Q, currNode);
                currNode = &(nextPage->nodes[0]);
            }
            else
            {
                //We've got to the end of a page and the addition of the next one is not complete,
                //We're done
                break;
            }
        }
    }
    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_EXIT "Q=%p\n", __func__, Q);
}

void iesq_reducePreDeleteCount(ieutThreadData_t *pThreadData, ismQHandle_t Qhdl)
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    uint32_t oldCount = __sync_fetch_and_sub(&(Q->preDeleteCount), 1);

    assert(oldCount > 0);

    if (oldCount == 1)
    {
        //We've just removed the last thing blocking deleting the queue, do it now....
        iesq_completeDeletion(pThreadData, Q);
    }
}

//forget about inflight_messages - no effect for this queue. The queue doesn't have a concept of
//inflight messages as there are no acks
void iesq_forgetInflightMsgs( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl )
{
}

//This queue has no messages that the delivery must complete
//for...there are no acks... delivery is never incomplete 
bool iesq_redeliverEssentialOnly( ieutThreadData_t *pThreadData, ismQHandle_t Qhdl )
{
    return false;
}

//Allow e.g. export code to chain along the queue
iesqQNode_t *iesq_getNextNodeFromPageEnd(
                ieutThreadData_t *pThreadData,
                iesqQueue_t *Q,
                iesqQNode_t *pPageEndNode)
{
    assert(pPageEndNode->msg == MESSAGE_STATUS_ENDPAGE);

    iesqQNode_t *nextNode = NULL;

    iesqQNodePage_t *currpage=iesq_getPageFromEnd(pPageEndNode);

    if (currpage->nextStatus == completed)
    {
        iesqQNodePage_t *nextPage=iesq_moveToNewPage(pThreadData, Q, pPageEndNode);
        nextNode = &(nextPage->nodes[0]);
    }

    return nextNode;
}

//Get for consumer connected to the queue (if any)
int32_t iesq_getConsumerStats( ieutThreadData_t *pThreadData
                             , ismQHandle_t Qhdl
                             , iempMemPoolHandle_t memPoolHdl
                             , size_t *pNumConsumers
                             , ieqConsumerStats_t consDataArray[])
{
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
    int32_t rc = OK;
    bool lockedConsumer = false;

    //Lock the consumer so we can walk the pages safely
    iewsWaiterStatus_t preLockedState = IEWS_WAITERSTATUS_DISCONNECTED;
    iewsWaiterStatus_t lockedState;

    if (Q->pConsumer == NULL)
    {
        *pNumConsumers = 0;
        goto mod_exit;
    }

    lockedConsumer = iews_getLockForQOperation( pThreadData
                                              , &(Q->waiterStatus)
                                              , (3 * 1000000000L) //3 seconds
                                              , &preLockedState
                                              , &lockedState
                                              , true);

    if (lockedConsumer)
    {
        if (preLockedState != IEWS_WAITERSTATUS_DISCONNECTED)
        {
            if (*pNumConsumers == 0)
            {
                //No space for stats
                *pNumConsumers = 1;
                rc = ISMRC_TooManyConsumers;
                goto mod_exit;
            }

            memset(&consDataArray[0], 0, sizeof(consDataArray[0]));

            char *pClientId = Q->pConsumer->pSession->pClient->pClientId;
            char *clientIdBuf = NULL;

            rc = iemp_allocate( pThreadData
                              , memPoolHdl
                              , strlen(pClientId)+1 //+1 for NULL
                              , (void **)&clientIdBuf);

            if (rc != OK)
            {
                goto mod_exit;
            }
            strcpy(clientIdBuf, pClientId);
            consDataArray[0].clientId  = clientIdBuf;

            consDataArray[0].sessionDeliveryStarted  = Q->pConsumer->pSession->fIsDeliveryStarted;
            consDataArray[0].sessionDeliveryStopping = Q->pConsumer->pSession->fIsDeliveryStopping;
            consDataArray[0].consumerState = Q->waiterStatus;

            *pNumConsumers = 1;
        }
        else
        {
            *pNumConsumers = 0;
        }
    }
    else
    {
        //Should have been able to lock consumer!
        rc = ISMRC_WaiterInUse;
    }


mod_exit:
   if (lockedConsumer)
   {
       iews_unlockAfterQOperation( pThreadData
               , Qhdl
               , Q->pConsumer
               , &(Q->waiterStatus)
               , lockedState
               , preLockedState);
   }
   return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of simpleQ.c                                                  */
/*                                                                   */
/*********************************************************************/
