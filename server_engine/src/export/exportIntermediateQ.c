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

//*********************************************************************
/// @file  exportIntermediateQ.c
/// @brief Export / Import functions for InterQ
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "intermediateQInternal.h"
#include "intermediateQ.h"
#include "waiterStatus.h"
#include "exportResources.h"
#include "exportMessage.h"
#include "exportSubscription.h"  //for find q handle
#include "exportClientState.h"   //for find client state
#include "engineStore.h"
#include "clientState.h"

typedef struct tag_ieieExportedInterQNode_t {
    uint64_t                        queueId;           ///< Queue this message is on
    uint64_t                        messageId;         ///< Msged id in exported file
    ismMessageState_t               msgState;          ///< State of message
    uint32_t                        deliveryId;        ///< Delivery Id if set
    uint8_t                         deliveryCount;     ///< Delivery counter
    uint8_t                         msgFlags;          ///< Message flags
    bool                            hasMDR;            ///< Has MDR in store
    bool                            inStore;           ///< Persisted in store
} ieieExportedInterQNode_t;

int32_t ieie_exportInterQNode( ieutThreadData_t *pThreadData
                            , ieieExportResourceControl_t *control
                            , ieiqQueue_t *Q
                            , ieiqQNode_t *pNode)
{
    int32_t rc = OK;

    if(   pNode->msg      != NULL
       && pNode->msgState != ismMESSAGE_STATE_CONSUMED
       && pNode->msgState != ieqMESSAGE_STATE_DISCARDING)
    {
        rc = ieie_exportMessage( pThreadData
                               , pNode->msg
                               , control
                               , false);

        if (OK == rc)
        {
            // Initialize to zero to ensure unused fields are obvious (and usable in future)
            ieieExportedInterQNode_t expData = {0};

            expData.queueId = (uint64_t)Q;
            expData.messageId = (uint64_t)(pNode->msg);
            expData.msgState = pNode->msgState;
            expData.deliveryId = pNode->deliveryId;
            expData.deliveryCount = pNode->deliveryCount;
            expData.msgFlags = pNode->msgFlags;
            expData.hasMDR = pNode->hasMDR;
            expData.inStore = pNode->inStore;

            rc = ieie_writeExportRecord( pThreadData
                                       , control
                                       , ieieDATATYPE_EXPORTEDQNODE_INTER
                                       , pNode->orderId
                                       , (uint32_t)sizeof(expData)
                                       , (void *)&(expData) );

        }
    }

    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Export all messages on a Intermediate queue
///
/// @param[in]     message              The message being exported
/// @param[in]     control              Export control structure
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_exportInterQMessages(ieutThreadData_t *pThreadData,
                                  ismQHandle_t Qhdl,
                                  ieieExportResourceControl_t *control)
{
    assert(ieq_getQType(Qhdl) == intermediate);
    ieiqQueue_t *Q = (ieiqQueue_t *)Qhdl;
    int32_t rc = OK;

    //Lock the consumer so we can walk the pages safely
    iewsWaiterStatus_t preLockedState = IEWS_WAITERSTATUS_DISCONNECTED;
    iewsWaiterStatus_t lockedState;

    bool lockedConsumer = iews_getLockForQOperation( pThreadData
                                                   , &(Q->waiterStatus)
                                                   , (3 * 1000000000L) //3 seconds
                                                   , &preLockedState
                                                   , &lockedState
                                                   , true);


    if (lockedConsumer)
    {
        ieiq_getHeadLock_ext(Q);

        ieiqQNode_t *currNode = Q->head;

        while(rc == OK && currNode != NULL)
        {
            ismMessageState_t msgState = currNode->msgState;

            if (msgState != ieqMESSAGE_STATE_END_OF_PAGE)
            {
                rc = ieie_exportInterQNode(pThreadData, control, Q, currNode);

                currNode = currNode + 1;
            }
            else
            {
                currNode = ieiq_getNextNodeFromPageEnd(
                                       pThreadData,
                                       Q,
                                       currNode);
            }
        }

        ieiq_releaseHeadLock_ext(Q);

        iews_unlockAfterQOperation( pThreadData
                                  , Qhdl
                                  , Q->pConsumer
                                  , &(Q->waiterStatus)
                                  , lockedState
                                  , preLockedState);
    }
    else
    {
        //Should have been able to lock consumer!
        rc = ISMRC_WaiterInUse;
    }

    return rc;
}

typedef struct tag_ieieAsyncImportInterQNode_t {
    uint64_t asyncId;
    ieieImportResourceControl_t *pControl;
    ieiqQueue_t *Q;
    uint64_t orderId;
    uint64_t dataId;
} ieieAsyncImportInterQNode_t;


void ieie_completeImportInterQNode(int rc, void *context)
{
    ieieAsyncImportInterQNode_t *pAsyncData = (ieieAsyncImportInterQNode_t *)context;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    pThreadData->threadType = AsyncCallbackThreadType;

    ieutTRACEL(pThreadData, pAsyncData->asyncId, ENGINE_CEI_TRACE,
               FUNCTION_ENTRY "pAsyncData=%p, ieadACId=0x%016lx rc=%d\n",
               __func__, pAsyncData, pAsyncData->asyncId, rc);

    if (rc != OK)
    {
        char humanIdentifier[256];

        sprintf(humanIdentifier, "Message %lu on %.*s", pAsyncData->orderId
                , 128, pAsyncData->Q->Common.QName);

        ieie_recordImportError( pThreadData
                              , pAsyncData->pControl
                              , ieieDATATYPE_EXPORTEDQNODE_INTER
                              , pAsyncData->asyncId
                              , humanIdentifier
                              , rc);
    }

    ieie_finishImportRecord(pThreadData, pAsyncData->pControl, ieieDATATYPE_EXPORTEDQNODE_INTER);
    (void)ieie_importTaskFinish( pThreadData
                               , pAsyncData->pControl
                               , true
                               , NULL);

    iemem_free(pThreadData, iemem_exportResources, pAsyncData);
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
}


int32_t ieie_importInterQNode(ieutThreadData_t            *pThreadData,
                              ieieImportResourceControl_t *pControl,
                              uint64_t                     dataId,
                              void                        *data,
                              size_t                       dataLen)
{
    int32_t rc = OK;
    ieieExportedInterQNode_t *impData = (ieieExportedInterQNode_t *)data;

    ismQHandle_t qhdl = NULL;
    ismEngine_Message_t *message = NULL;
    bool releaseMsg = false;
    bool needRollback = false;

    rc = ieie_findImportedMessage(pThreadData,
                                  pControl,
                                  impData->messageId,
                                  &message);

    if (rc == OK)
    {
        releaseMsg = true; // We've got a usecount on the message now

        rc = ieie_findImportedQueueHandle(pThreadData,
                                          pControl,
                                          impData->queueId,
                                          &qhdl);
    }

    if (rc == OK)
    {
        //Check that the queue is an intermediate queue
        if (qhdl != NULL)
        {
            if (ieq_getQType(qhdl) == intermediate)
            {
                ieiqQueue_t *Q =  (ieiqQueue_t *)qhdl;
                ieiqQNode_t *pnode = NULL;
                ieieAsyncImportInterQNode_t *pContext = NULL;
                ismEngine_ClientState_t *pClient = NULL;

                if (impData->hasMDR)
                {
                    rc = ieie_findImportedClientStateByQueueDataId(pThreadData,
                                                                   pControl,
                                                                   impData->queueId,
                                                                   &pClient);
                    if (rc != OK)
                    {
                        goto mod_exit;
                    }
                    assert(pClient != NULL);
                }

                if (impData->inStore)
                {
                    //if it was in the store before... it will be again.. allow space for the
                    //message, 2 MDR references and the message reference
                    iest_store_reserve( pThreadData
                                      , iest_MessageStoreDataLength(message)
                                      , 1
                                      , 3 );
                    needRollback = true; //We'll turn this off when we commit
                }

                rc = ieiq_importQNode( pThreadData
                                     , Q
                                     , message
                                     , impData->msgState
                                     , dataId
                                     , impData->deliveryId
                                     , impData->deliveryCount
                                     , impData->msgFlags
                                     , impData->hasMDR
                                     , impData->inStore
                                     , &pnode);

                if (rc != OK)
                {
                    goto mod_exit;
                }

                if (impData->hasMDR)
                {
                    rc = iecs_importMessageDeliveryReference( pThreadData
                                                            , pClient
                                                            , dataId
                                                            , impData->deliveryId
                                                            , Q->hStoreObj
                                                            , pnode->hMsgRef
                                                            , qhdl
                                                            , pnode);

                    if (rc != OK)
                    {
                        goto mod_exit;
                    }
                }

               assert(pnode->inStore == impData->inStore);
               assert(pnode->hasMDR  == impData->hasMDR);

               if (impData->inStore)
               {
                   pContext = (ieieAsyncImportInterQNode_t *)iemem_calloc(pThreadData,
                                                                IEMEM_PROBE(iemem_exportResources, 14), 1,
                                                                sizeof(ieieAsyncImportInterQNode_t));
                   if (pContext == NULL)
                   {
                       rc = ISMRC_AllocateError;
                       goto mod_exit;
                   }
                   //Setup AsyncId and trace it so we can tie up commit with callback:
                   pContext->asyncId  = pThreadData->asyncCounter++;
                   pContext->pControl = pControl;
                   pContext->Q        = Q;
                   pContext->dataId   = dataId;
                   pContext->orderId  = pnode->orderId;

                   ieutTRACEL(pThreadData, pContext->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "ieieInterACId=0x%016lx\n",
                                                                 __func__, pContext->asyncId);

                   rc = iest_store_asyncCommit(pThreadData, true, ieie_completeImportInterQNode, pContext);
                   needRollback = false;

                   if (rc != ISMRC_AsyncCompletion)
                   {
                       ieutTRACEL(pThreadData, pContext->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "ieieInterACId=0x%016lx Completed sync\n",
                                                                     __func__, pContext->asyncId);
                       iemem_free(pThreadData, iemem_exportResources, pContext);
                   }

               }
            }
            else
            {
                ieutTRACEL(pThreadData,ieq_getQType(qhdl), ENGINE_ERROR_TRACE,
                           "Importing intermediate queue node but queue type: %u\n",
                           ieq_getQType(qhdl));
                rc = ISMRC_FileCorrupt;
            }
        }
        else
        {
            //We chose not to import the queue... ignore messages on it
            ieutTRACEL(pThreadData, impData->queueId, ENGINE_HIGH_TRACE,
                       "Ignoring intermediate queue node: %lu, qDataId %lu\n",
                       dataId, impData->queueId);
        }
    }

mod_exit:

    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        char humanIdentifier[256];

        sprintf(humanIdentifier, "Message %lu on %.*s", dataId
                , 128, (qhdl != NULL)? qhdl->QName : "???");

        ieie_recordImportError( pThreadData
                              , pControl
                              , ieieDATATYPE_EXPORTEDQNODE_INTER
                              , dataId
                              , humanIdentifier
                              , rc);
    }

    if (needRollback)
    {
        iest_store_rollback(pThreadData, true);
    }

    if (releaseMsg)
    {
        iem_releaseMessage(pThreadData, message);
    }

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of exportMessage.c                                            */
/*                                                                   */
/*********************************************************************/
