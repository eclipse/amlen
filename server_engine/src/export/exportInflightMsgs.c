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
/// @file  exportInflightMsgs.c
/// @brief For a shared sub, the queue doesn't really track which
///        messages are inflight to which client, so we need this
//         file which exports inflight msgs on a per-client basis
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "multiConsumerQInternal.h"
#include "exportResources.h"
#include "exportMessage.h"
#include "lockManager.h"
#include "clientState.h"
#include "exportQMessages.h"

typedef struct tag_ieieExportInflightMultiConsumerQNodeContext_t {
    iemqQNode_t *pNode;
    ieieExportInflightMultiConsumerQNode_t nodeInfo;
    bool gotNodeDetails;
} ieieExportInflightMultiConsumerQNodeContext_t;

static void ieie_getInflightNodeDetailsForExport(void *context)
{
    ieieExportInflightMultiConsumerQNodeContext_t *exportContext = (ieieExportInflightMultiConsumerQNodeContext_t *)context;
    iemqQNode_t *pNode = exportContext->pNode;

    //Only do inflight messages
    if (   pNode->msg      != NULL
        && (   pNode->msgState == ismMESSAGE_STATE_DELIVERED
            || pNode->msgState == ismMESSAGE_STATE_RECEIVED))
    {
        exportContext->nodeInfo.messageId     = (uint64_t)pNode->msg;
        exportContext->nodeInfo.msgState      = pNode->msgState;
        exportContext->nodeInfo.deliveryId    = pNode->deliveryId;
        exportContext->nodeInfo.deliveryCount = pNode->deliveryCount;
        exportContext->nodeInfo.msgFlags      = pNode->msgFlags;
        exportContext->nodeInfo.hasMDR        = pNode->hasMDR;
        exportContext->nodeInfo.inStore       = pNode->inStore;

        //Stop the message going away until we've finished with it
        iem_recordMessageUsage(pNode->msg);

        exportContext->gotNodeDetails = true;
    }
}

static int32_t ieie_exportInflightQNode( ieutThreadData_t *pThreadData
                                , ieieExportResourceControl_t *control
                                , uint64_t ClientDataId
                                , iemqQueue_t *Q
                                , iemqQNode_t *pNode)
{
   int32_t rc = OK;

   ielmLockName_t LockName = { .Msg = { ielmLOCK_TYPE_MESSAGE, Q->qId,  pNode->orderId } };
   ieieExportInflightMultiConsumerQNodeContext_t context = {0};

   context.pNode = pNode;
   context.gotNodeDetails = false;

   rc = ielm_instantLockWithCallback( pThreadData
                                    , &LockName
                                    , true
                                    , ieie_getInflightNodeDetailsForExport
                                    , &context);

   ieutTRACEL(pThreadData, pNode->orderId, ENGINE_HIFREQ_FNC_TRACE,
              "EXPINMSG: Q %p, %u, OrderId %lu, rc %d gotNode %d\n",
                 Q, Q->qId, pNode->orderId, rc, context.gotNodeDetails);

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
       context.nodeInfo.queueDataId  = (uint64_t)Q;
       context.nodeInfo.clientDataId = (uint64_t)ClientDataId;

       rc = ieie_exportMessage( pThreadData
                              , pNode->msg
                              , control
                              , true);

       if (OK == rc)
       {
           rc = ieie_writeExportRecord( pThreadData
                                      , control
                                      , ieieDATATYPE_EXPORTEDQNODE_MULTI_INPROG
                                      , pNode->orderId
                                      , (uint32_t)sizeof(context.nodeInfo)
                                      , (void *)&(context.nodeInfo));
       }
   }

   return rc;
}

typedef struct tag_ieieInflightMessageContext_t {
    ieieExportResourceControl_t *control;
    uint64_t ClientDataId;
} ieieInflightMessageContext_t;

static int32_t ieie_exportInflightMessage( ieutThreadData_t *pThreadData
                                         , ismQHandle_t Qhdl
                                         , void *pNode
                                         , void *context)
{
    ieieInflightMessageContext_t *msgContext = (ieieInflightMessageContext_t *)context;
    int32_t rc = OK;

    // Can only cleanup messages which have a queue and node handle set (i.e. are multiConsumer)
    if (pNode == NULL || Qhdl == NULL)
    {
        //For single consumer queues, we  will already have exported the messages
        //as we know which client they are inflight to
        goto mod_exit;
    }

    if (ieq_getQType(Qhdl) != multiConsumer)
    {
        //For single consumer queues, we  will already have exported the messages
        //as we know which client they are inflight to
        goto mod_exit;
    }
    iemqQueue_t *Q = (iemqQueue_t *)Qhdl;
    iemqQNode_t *iemqNode = (iemqQNode_t *)pNode;

    iemq_takeReadHeadLock_ext(Q);

    rc = ieie_exportInflightQNode( pThreadData
                                 , msgContext->control
                                 , msgContext->ClientDataId
                                 , Q
                                 , iemqNode);

    iemq_releaseHeadLock_ext(Q);

mod_exit:
    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Export  inflight messages on shared subscriptions
/// On single consumer queues, it's "obvious" which client messages are in
/// flight to. From multi consumer subs, we need to follow links from the
/// client in order to find which messages are for which clients
///
/// @param[in]     message              The message being exported
/// @param[in]     control              Export control structure
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
static int32_t ieie_exportInflightQMessages(ieutThreadData_t *pThreadData,
                                            ieieExportResourceControl_t *control,
                                            uint64_t ClientDataId,
                                            iecsMessageDeliveryInfoHandle_t hMsgDelInfo)
{
    ieieInflightMessageContext_t msgContext = {control, ClientDataId};

    int32_t rc = iecs_iterateMessageDeliveryInfo( pThreadData
                                                , hMsgDelInfo
                                                , ieie_exportInflightMessage
                                                , &msgContext);


    return rc;
}

typedef struct tag_ieieExportInflightMsgsForClientContext_t
{
    int32_t rc;
    ieieExportResourceControl_t *control;
} ieieExportInflightMsgsForClientContext_t;

void ieie_exportInflightMsgsForClient(ieutThreadData_t *pThreadData,
                                      char *clientId,
                                      uint32_t keyHash,
                                      void *value,
                                      void *context)
{
    ieieExportInflightMsgsForClientContext_t *pContext = (ieieExportInflightMsgsForClientContext_t *)context;

    uint64_t ClientDataId = (uint64_t)value;

    ieutTRACEL(pThreadData, ClientDataId, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "clientId='%s' (0x%08x) dataId=0x%0lx\n",
               __func__, clientId, keyHash, ClientDataId);

    if (pContext->rc == OK)
    {
        iecsMessageDeliveryInfoHandle_t hMsgDelInfo;

        int32_t rc = iecs_findClientMsgDelInfo(pThreadData,
                                               clientId,
                                               &hMsgDelInfo);

        if (rc == OK)
        {
            rc = ieie_exportInflightQMessages( pThreadData
                                             , pContext->control
                                             , ClientDataId
                                             , hMsgDelInfo);

            if (rc != OK)
            {
                //whoever had the problem will have recorded more info
                pContext->rc = OK;
            }
        }
        else if (rc != ISMRC_NotFound)
        {
            //whoever had the problem will have recorded more info
            pContext->rc = OK;
        }
    }

    ieutTRACEL(pThreadData, pContext->rc, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "rc=%d\n",
               __func__, pContext->rc);
}

int32_t ieie_exportInflightMessages(ieutThreadData_t *pThreadData,
                                    ieieExportResourceControl_t *control)

{
    int32_t rc = OK;

    assert(control->file != NULL);

    ieutTRACEL(pThreadData, control->clientId, ENGINE_FNC_TRACE, FUNCTION_ENTRY "clientId='%s' outFile=%p\n", __func__,
               control->clientId, control->file);

    ieieExportInflightMsgsForClientContext_t context = {0};

    context.control = control;

    if (control->clientIdTable->totalCount != 0)
    {

        // Traverse the table of found clientIds exporting each one found.
        ieut_traverseHashTable(pThreadData, control->clientIdTable, ieie_exportInflightMsgsForClient, &context);

        rc = context.rc;
    }

    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of exportMessage.c                                            */
/*                                                                   */
/*********************************************************************/
