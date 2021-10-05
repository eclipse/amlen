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
/// @file  ackList.c
/// @brief Management of message acknowledgement
//*********************************************************************
#define TRACE_COMP Engine

#include <pthread.h>

#include "engineInternal.h"
#include "ackList.h"
#include "multiConsumerQ.h"
#include "multiConsumerQInternal.h"
#include "memHandler.h"
#include "clientState.h"
#include "engineStore.h"

//NB: IF locking both put & get lock.... lock get lock first!

void ieal_addUnackedMessage( ieutThreadData_t *pThreadData
                           , ismEngine_Consumer_t *pConsumer
                           , iemqQNode_t *qnode )
{
    ismEngine_Session_t *pSession = pConsumer->pSession;

    ieutTRACEL(pThreadData, qnode->orderId, ENGINE_HIFREQ_FNC_TRACE,
               "Adding to Session %p  Consumer %p Q %u Node Oid %lu\n",
               pSession, pConsumer, ((iemqQueue_t *)(pConsumer->queueHandle))->qId, qnode->orderId);

    bool done_put = false;
    int os_rc;

    assert(qnode->ackData.pConsumer == NULL);
    qnode->ackData.pConsumer = pConsumer;
    qnode->ackData.pNext = NULL;

    if (pConsumer->fShortDeliveryIds)
    {
        //In this case we can use the list of delivery ids as our ack list
        increaseConsumerAckCount(pConsumer);
        return;
    }

    //dirty read tail
    if (pSession->lastAck != NULL)
    {
        os_rc = pthread_spin_lock(&(pSession->ackListPutLock));

        if (os_rc != 0)
        {
            ieutTRACE_FFDC( 1, true, "Failed to take the putlock", ISMRC_Error
                            , "pSession",  pSession,  sizeof(pSession)
                            , "pConsumer", pConsumer, sizeof(pConsumer)
                            , "qnode",     qnode,     sizeof(qnode)
                            , "os_rc",     os_rc,     sizeof(os_rc)
                            , NULL );
        }

        //Now can validly read tail
        if (pSession->lastAck != NULL)
        {
            //We have putlock and a non-empty list... we can validly add an item
            qnode->ackData.pPrev = pSession->lastAck;
            pSession->lastAck->ackData.pNext = qnode;
            pSession->lastAck = qnode;
            increaseConsumerAckCount(pConsumer);
            done_put = true;
        }

        (void)pthread_spin_unlock(&(pSession->ackListPutLock));
    }

    if (!done_put)
    {
        //Hmmm we may be adding to an empty list and thus may need to set head
        //We need the getlock and the putlock (in that order)
        os_rc = pthread_spin_lock(&(pSession->ackListGetLock));

        if (os_rc != 0)
        {
            ieutTRACE_FFDC( 2, true, "Failed to take the getlock", ISMRC_Error
                                        , "pSession",  pSession,  sizeof(pSession)
                                        , "pConsumer", pConsumer, sizeof(pConsumer)
                                        , "qnode",     qnode,     sizeof(qnode)
                                        , "os_rc",     os_rc,     sizeof(os_rc)
                                        , NULL );
        }

        os_rc = pthread_spin_lock(&(pSession->ackListPutLock));
        if (os_rc != 0)
        {
            ieutTRACE_FFDC( 3, true, "Failed to take the putlock (after getlock)", ISMRC_Error
                                        , "pSession",  pSession,  sizeof(pSession)
                                        , "pConsumer", pConsumer, sizeof(pConsumer)
                                        , "qnode",     qnode,     sizeof(qnode)
                                        , "os_rc",     os_rc,     sizeof(os_rc)
                                        , NULL );
        }

        if (pSession->lastAck != NULL) {
            //We have putlock and a non-empty list... we can validly add an item
            qnode->ackData.pPrev = pSession->lastAck;
            pSession->lastAck->ackData.pNext = qnode;

            assert(pSession->firstAck != NULL);
        } else {
            qnode->ackData.pPrev = NULL;
            assert(pSession->firstAck == NULL);
            pSession->firstAck = qnode;
        }
        pSession->lastAck = qnode;
        increaseConsumerAckCount(pConsumer);

        (void)pthread_spin_unlock(&(pSession->ackListPutLock));
        (void)pthread_spin_unlock(&(pSession->ackListGetLock));
    }
    return;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Remove a node from a Session's list of nodes that need acking
///
///  @param[in]     pSession        - Session that has the node in the ackList
///  @param[in]     qnode           - qnode to be removed
///  @param[in,out] ppConsumer      - If non-null, this is set to the (potentially ghost) consumer that
///                                  needs to have its ackcount reduced.
///                                  If null on input, this function reduces the ackcount, meaning the queue
///                                  may be freed before this function returns
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////

void ieal_removeUnackedMessage( ieutThreadData_t *pThreadData
                              , ismEngine_Session_t  *pSession
                              , iemqQNode_t          *qnode
                              , ismEngine_Consumer_t **ppConsumer)
{
    assert(qnode->ackData.pConsumer != NULL);
    ismEngine_Consumer_t *pConsumer = qnode->ackData.pConsumer;

    ieutTRACEL(pThreadData, pSession, ENGINE_HIFREQ_FNC_TRACE,
               "Removing from Session %p Consumer %p Q %u Node Oid %lu\n",
               pSession, pConsumer, ((iemqQueue_t *)(pConsumer->queueHandle))->qId, qnode->orderId);

    DEBUG_ONLY int os_rc;

    if ( !(pConsumer->fShortDeliveryIds) )
    {
        os_rc = pthread_spin_lock(&(pSession->ackListGetLock));
        assert(os_rc == 0);

        if (qnode->ackData.pNext == NULL) {
            //Hmmm appear to be getting last item...need both locks
            os_rc = pthread_spin_lock(&(pSession->ackListPutLock));
            assert(os_rc == 0);
            assert(pSession->lastAck !=  NULL);

            if (qnode->ackData.pPrev != NULL) {
                qnode->ackData.pPrev->ackData.pNext = qnode->ackData.pNext;
            } else {
                assert(pSession->firstAck == qnode);
                pSession->firstAck = qnode->ackData.pNext;
            }

            if (qnode->ackData.pNext != NULL) {
                qnode->ackData.pNext->ackData.pPrev = qnode->ackData.pPrev;
            } else {
                assert(pSession->lastAck == qnode);
                pSession->lastAck = qnode->ackData.pPrev;
            }
            qnode->ackData.pPrev = NULL;
            qnode->ackData.pNext = NULL;

            (void)pthread_spin_unlock(&(pSession->ackListPutLock));
        } else {
            if (qnode->ackData.pPrev != NULL) {
                qnode->ackData.pPrev->ackData.pNext = qnode->ackData.pNext;
            } else {
                assert(pSession->firstAck == qnode);
                pSession->firstAck = qnode->ackData.pNext;
            }
            //We know next is non-null else we'd be in other leg of if
            qnode->ackData.pNext->ackData.pPrev = qnode->ackData.pPrev;

            qnode->ackData.pPrev = NULL;
            qnode->ackData.pNext = NULL;
        }
        (void)pthread_spin_unlock(&(pSession->ackListGetLock));
    }

    qnode->ackData.pConsumer = NULL;
    
    if (ppConsumer == NULL)
    {
        //Caller is happy for us to decrease ackcount (potentially deleting the queue)
        decreaseConsumerAckCount(pThreadData, pConsumer, 1);
    }
    else
    {
        //Tell the caller to decrease the ack count
        *ppConsumer = pConsumer;
    }

    return;
}

/// @return OK Or AsyncCompletion - If async will reduce session usecount on completion
int32_t ieal_nackOutstandingMessages( ieutThreadData_t *pThreadData, ismEngine_Session_t *pSession )
{
    int32_t rc = OK;
    ieutTRACEL(pThreadData, pSession, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_ENTRY "Starting nack outstanding for %p\n", __func__, pSession);


    if (pSession->pClient->hMsgDeliveryInfo)
    {
        rc = iecs_sessionCleanupFromDeliveryInfo(pThreadData, pSession, pSession->pClient->hMsgDeliveryInfo);
    }
    else
    {
        //Check we can get both the get+put lock... no-one should be delivering messages
        //or processing acks for the session any more...
        int32_t os_rc = pthread_spin_trylock(&(pSession->ackListGetLock));
        if (os_rc != 0)
        {
            ieutTRACE_FFDC( 1, true, "Failed to take the getlock", ISMRC_Error
                          , "pSession",  pSession,  sizeof(pSession)
                          , "os_rc",     os_rc,     sizeof(os_rc),     NULL );
        }
        os_rc = pthread_spin_trylock(&(pSession->ackListPutLock));
        if (os_rc != 0)
        {
            ieutTRACE_FFDC( 2, true, "Failed to take the putlock", ISMRC_Error
                          , "pSession",  pSession,  sizeof(pSession)
                          , "os_rc",     os_rc,     sizeof(os_rc),  NULL );
        }

        iemqQNode_t *qnode = pSession->firstAck;

        uint32_t storeOps = 0;
        //TODO: store reserve!

        while ((rc == OK) && (qnode != NULL))
        {
            ismEngine_Consumer_t *pConsumer = qnode->ackData.pConsumer;
            iemqQNode_t *nextNode = qnode->ackData.pNext;

            // Nack any messages which have not been consumed. A consumed message
            // which is still in our Ack list is part of a transaction so will be
            // unack'd when we roll-back the transaction
            //After we do acknowledge we can no longer access the node

            iemq_prepareAck( pThreadData
                           , pConsumer->queueHandle
                           , pSession
                           , NULL
                           , qnode
                           , ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP
                           , &storeOps );

            //With the confirm option we're using, outside of a transaction it'll FDC and exit rather than return
            assert(rc == OK);

            qnode = nextNode;
        }

        if (storeOps > 0)
        {
            //Later make this go async
            iest_store_commit(pThreadData, false);
        }

        ismEngine_BatchAckState_t batchState = {0};
        qnode = pSession->firstAck;

        while ((rc == OK) && (qnode != NULL))
        {
            ismEngine_Consumer_t *pConsumer = qnode->ackData.pConsumer;
            iemqQNode_t *nextNode = qnode->ackData.pNext;
            bool triggerSessionRedelivery = false; // We're not going to restart this session it's ending
            ismStore_Handle_t hMsgToUnstore = ismSTORE_NULL_HANDLE;
            //After we finish acknowledge we can no longer access the node

            rc = iemq_processAck( pThreadData
                    , pConsumer->queueHandle
                    , pSession
                    , NULL
                    , qnode
                    , ismENGINE_CONFIRM_OPTION_SESSION_CLEANUP
                    , &hMsgToUnstore
                    , &triggerSessionRedelivery
                    , &batchState
                    , NULL);

            //With the confirm option we're using, outside of a transaction it'll FDC and exit rather than return
            assert(rc == OK);

            assert(hMsgToUnstore == ismSTORE_NULL_HANDLE); //Nacking messages shouldn't require anything to be removed from the store

            qnode = nextNode;
        }

        // And call checkWaiters for the last queue nack'd
        if (batchState.pConsumer != NULL)
        {
            ieq_completeAckBatch(pThreadData, batchState.Qhdl, pSession, &batchState );
        }

        os_rc = pthread_spin_unlock(&(pSession->ackListPutLock));

        if (os_rc != 0)
        {
            ieutTRACE_FFDC( 3, true, "Failed to release the putlock", ISMRC_Error
                          , "pSession",  pSession,  sizeof(pSession)
                          , "os_rc",     os_rc,     sizeof(os_rc), NULL );
        }

        os_rc = pthread_spin_unlock(&(pSession->ackListGetLock));

        if (os_rc != 0)
        {
            ieutTRACE_FFDC( 4, true, "Failed to release the getlock", ISMRC_Error
                          , "pSession",  pSession,  sizeof(pSession)
                          , "os_rc",     os_rc,     sizeof(os_rc), NULL );
        }
    }
    ieutTRACEL(pThreadData, pSession, ENGINE_HIFREQ_FNC_TRACE,
               FUNCTION_EXIT "pSession=%p\n", __func__, pSession);

    return rc;
}

void ieal_debugAckList(ieutThreadData_t *pThreadData, ismEngine_Session_t *pSession)
{
    int32_t os_rc = pthread_spin_lock(&(pSession->ackListGetLock));
    if (os_rc != 0)
    {
        ieutTRACE_FFDC( 1, true, "Failed to take the getlock", ISMRC_Error
                               , "pSession",  pSession,  sizeof(pSession)
                               , "os_rc",     os_rc,     sizeof(os_rc),     NULL );
    }

    ieutTRACEL(pThreadData, pSession, 2,"AckList for session %p!\n", pSession);

    iemqQNode_t *qnode = pSession->firstAck;

    while (qnode != NULL)
    {
        ieutTRACEL(pThreadData, qnode, 2, "pConsumer %p QId %u QNode %lu\n",
                   qnode->ackData.pConsumer,
                   ((qnode->ackData.pConsumer != NULL) && (qnode->ackData.pConsumer->queueHandle != NULL))?
                           ((iemqQueue_t *)(qnode->ackData.pConsumer->queueHandle))->qId : 0,
                   qnode->orderId );

        qnode = qnode->ackData.pNext;
    }

    os_rc = pthread_spin_unlock(&(pSession->ackListGetLock));

    if (os_rc != 0)
    {
        ieutTRACE_FFDC( 2, true, "Failed to release the getlock", ISMRC_Error
                                        , "pSession",  pSession,  sizeof(pSession)
                                        , "os_rc",     os_rc,     sizeof(os_rc), NULL );
    }

    return;
}
