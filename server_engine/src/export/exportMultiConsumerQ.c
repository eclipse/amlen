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
/// @file  exportMultiConsumerQ.c
/// @brief Export / Import functions for MultiConsumerQ
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "multiConsumerQInternal.h"
#include "waiterStatus.h"
#include "exportResources.h"
#include "exportMessage.h"
#include "lockManager.h"
#include "exportQMessages.h"
#include "exportSubscription.h" //for find q handle
#include "exportClientState.h" //for find client
#include "engineStore.h"

typedef struct tag_ieieExportedMultiConsumerQNode_t {
    uint64_t                        queueDataId;       ///< Queue this message belongs to
    uint64_t                        messageId;         ///< Msged id in exported file
    ismMessageState_t               msgState;          ///< State of message
    uint8_t                         deliveryCount;     ///< Delivery counter
    uint8_t                         msgFlags;          ///< Message flags
    bool                            inStore;           ///< Persisted in store
} ieieExportedMultiConsumerQNode_t;


typedef struct tag_ieieExportMultiConsumerQNodeContext_t {
    iemqQNode_t *pNode;
    ieieExportedMultiConsumerQNode_t nodeInfo;
    bool gotNodeDetails;
} ieieExportMultiConsumerQNodeContext_t;

void ieie_getNodeDetailsForExport(void *context)
{
    ieieExportMultiConsumerQNodeContext_t *exportContext = (ieieExportMultiConsumerQNodeContext_t *)context;
    iemqQNode_t *pNode = exportContext->pNode;

    //Only do available messages for the moment
    if (   pNode->msg      != NULL
        && pNode->msgState == ismMESSAGE_STATE_AVAILABLE)
    {
        exportContext->nodeInfo.messageId     = (uint64_t)pNode->msg;
        exportContext->nodeInfo.msgState      = pNode->msgState;
        exportContext->nodeInfo.deliveryCount = pNode->deliveryCount;
        exportContext->nodeInfo.msgFlags      = pNode->msgFlags;
        exportContext->nodeInfo.inStore       = pNode->inStore;

        //Stop the message going away until we've finished with it
        iem_recordMessageUsage(pNode->msg);

        exportContext->gotNodeDetails = true;
    }
}

int32_t ieie_exportMultiConsumerQNode( ieutThreadData_t *pThreadData
                                     , ieieExportResourceControl_t *control
                                     , iemqQueue_t *Q
                                     , iemqQNode_t *pNode)
{
   int32_t rc = OK;

   ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE, Q->qId,  pNode->orderId } };
   ieieExportMultiConsumerQNodeContext_t context = {0};

   context.pNode = pNode;
   context.gotNodeDetails = false;

   rc = ielm_instantLockWithCallback( pThreadData
                                    , &LockName
                                    , true
                                    , ieie_getNodeDetailsForExport
                                    , &context);

   ieutTRACEL(pThreadData, pNode->orderId, ENGINE_HIFREQ_FNC_TRACE,
              "EXPMSG: Q %u, OrderId %lu, rc %d gotNode %d\n",
              Q->qId, pNode->orderId, rc, context.gotNodeDetails);

   if (rc == ISMRC_LockNotGranted)
   {
       //Someone had it locked... we can't have it
       ieutTRACE_HISTORYBUF(pThreadData, ISMRC_LockNotGranted);
       rc = OK;
   }
   else if (rc == OK && !context.gotNodeDetails)
   {
       //message wasn't available, we just don't export it
       ieutTRACE_HISTORYBUF(pThreadData, ISMRC_NoMsgAvail);
       rc = OK;
   }

   if(context.gotNodeDetails)
   {
       context.nodeInfo.queueDataId = (uint64_t)Q;

       rc = ieie_exportMessage( pThreadData
                              , pNode->msg
                              , control
                              , true);

       if (OK == rc)
       {
           rc = ieie_writeExportRecord( pThreadData
                                      , control
                                      , ieieDATATYPE_EXPORTEDQNODE_MULTI
                                      , pNode->orderId
                                      , (uint32_t)sizeof(context.nodeInfo)
                                      , (void *)&(context.nodeInfo) );
       }
   }

   return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Export all messages on a MultiConsumerle queue
///
/// @param[in]     message              The message being exported
/// @param[in]     control              Export control structure
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_exportMultiConsumerQMessages(ieutThreadData_t *pThreadData,
                                          ismQHandle_t Qhdl,
                                          ieieExportResourceControl_t *control)
{
    assert(ieq_getQType(Qhdl) == multiConsumer);
    iemqQueue_t *Q = (iemqQueue_t *)Qhdl;
    int32_t rc = OK;

    iemq_takeReadHeadLock_ext(Q);

    iemqQNode_t *currNode = &(Q->headPage->nodes[0]);

    while (1)
    {
        iemqQNode_t *nextNode = iemq_getSubsequentNode_ext(Q, currNode);

        if (nextNode == NULL)
        {
            //We don't destroy/get/look at etc a node until the subsequent node has
            //been alloced (in normal operation,so the cursor can point at that. In this case,
            //to be consistent with normal getting browsing etc.)
            break;
        }

        rc = ieie_exportMultiConsumerQNode(pThreadData, control, Q, currNode);

        currNode = nextNode;
    }

    iemq_releaseHeadLock_ext(Q);

    return rc;
}

typedef struct tag_ieieAsyncImportMultiQNode_t {
    uint64_t asyncId;
    ieieImportResourceControl_t *pControl;
    iemqQueue_t *Q;
    uint64_t orderId;
    uint64_t dataId;
    ieieDataType_t dataType;
} ieieAsyncImportMultiQNode_t;


void ieie_completeImportMultiQNode(int rc, void *context)
{
    ieieAsyncImportMultiQNode_t *pAsyncData = (ieieAsyncImportMultiQNode_t *)context;
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
                              , pAsyncData->dataType
                              , pAsyncData->asyncId
                              , humanIdentifier
                              , rc);
    }

    ieie_finishImportRecord(pThreadData, pAsyncData->pControl, pAsyncData->dataType);
    (void)ieie_importTaskFinish( pThreadData
                               , pAsyncData->pControl
                               , true
                               , NULL);

    iemem_free(pThreadData, iemem_exportResources, pAsyncData);
    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
}

int32_t ieie_importMultiConsumerQNode(ieutThreadData_t            *pThreadData,
                                      ieieImportResourceControl_t *pControl,
                                      ieieDataType_t               dataType,
                                      uint64_t                     dataId,
                                      void                        *data,
                                      size_t                       dataLen)
{
    int32_t rc = OK;
    ieieExportedMultiConsumerQNode_t *impStaticData = NULL;
    ieieExportInflightMultiConsumerQNode_t *impData = NULL;
    ieieExportInflightMultiConsumerQNode_t stackImpData = {0};

    ismQHandle_t qhdl = NULL;
    ismEngine_Message_t *message = NULL;
    ismEngine_ClientState_t *pClient = NULL;
    bool releaseMsg = false;
    bool needRollback = false;


    if (dataType == ieieDATATYPE_EXPORTEDQNODE_MULTI)
    {
        impStaticData = (ieieExportedMultiConsumerQNode_t *)data;
        impData       = &stackImpData;

        impData->deliveryCount = impStaticData->deliveryCount;
        impData->inStore       = impStaticData->inStore;
        impData->messageId     = impStaticData->messageId;
        impData->queueDataId   = impStaticData->queueDataId;
        impData->msgFlags      = impStaticData->msgFlags;
        impData->msgState      = impStaticData->msgState;
    }
    else if (dataType == ieieDATATYPE_EXPORTEDQNODE_MULTI_INPROG)
    {
        impData = (ieieExportInflightMultiConsumerQNode_t *)data;
    }
    else
    {
        ieutTRACE_FFDC( ieutPROBE_001, true, "Illegal data imported as multiconsumer q node", ISMRC_Error
                      , "dataType", &dataType, sizeof(dataType)
                      , NULL);
    }

    rc = ieie_findImportedMessage(pThreadData,
                                  pControl,
                                  impData->messageId,
                                  &message);

    if (rc == OK)
    {
        releaseMsg = true; // We've got a usecount on the message now

        rc = ieie_findImportedQueueHandle(pThreadData,
                                          pControl,
                                          impData->queueDataId,
                                          &qhdl);
    }

    if (rc == OK && impData->clientDataId != 0)
    {
         rc = ieie_findImportedClientState(pThreadData,
                                           pControl,
                                           impData->clientDataId,
                                           &pClient);
    }

    if (rc == OK)
    {
        //Check that the queue is a multiconsumer queue
        if (qhdl != NULL)
        {
            if (ieq_getQType(qhdl) == multiConsumer)
            {
                iemqQueue_t *Q =  (iemqQueue_t *)qhdl;
                iemqQNode_t *pnode = NULL;
                ieieAsyncImportMultiQNode_t *pContext = NULL;

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

                rc = iemq_importQNode( pThreadData
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

                if ((impData->hasMDR) && (dataType == ieieDATATYPE_EXPORTEDQNODE_MULTI_INPROG))
                {
                    assert(pClient != NULL);

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
                   pContext = (ieieAsyncImportMultiQNode_t *)iemem_calloc(pThreadData,
                                                                IEMEM_PROBE(iemem_exportResources, 15), 1,
                                                                sizeof(ieieAsyncImportMultiQNode_t));
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
                   pContext->dataType = dataType;
                   pContext->orderId  = pnode->orderId;

                   ieutTRACEL(pThreadData, pContext->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "ieieMultiACId=0x%016lx\n",
                                                                 __func__, pContext->asyncId);

                   needRollback = false; //We're committing - no need to rollback
                   rc = iest_store_asyncCommit(pThreadData, true, ieie_completeImportMultiQNode, pContext);

                   if (rc != ISMRC_AsyncCompletion)
                   {
                       ieutTRACEL(pThreadData, pContext->asyncId, ENGINE_CEI_TRACE, FUNCTION_IDENT "ieieMultiACId=0x%016lx Completed sync\n",
                                                                     __func__, pContext->asyncId);
                       iemem_free(pThreadData, iemem_exportResources, pContext);
                   }

               }
            }
            else
            {
                ieutTRACEL(pThreadData,ieq_getQType(qhdl), ENGINE_ERROR_TRACE,
                           "Importing multiconsumer queue node but queue type: %u\n",
                           ieq_getQType(qhdl));
                rc = ISMRC_FileCorrupt;
            }
        }
        else
        {
            //We chose not to import the queue... ignore messages on it
            ieutTRACEL(pThreadData, impData->queueDataId, ENGINE_HIGH_TRACE,
                       "Ignoring multiconsumer queue node: %lu, qDataId %lu\n",
                       dataId, impData->queueDataId);
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
                              , dataType
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
