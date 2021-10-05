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
/// @file  exportSimpQ.c
/// @brief Export / Import functions for simpQ
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "simpQInternal.h"
#include "simpQ.h"
#include "waiterStatus.h"
#include "exportResources.h"
#include "exportMessage.h"
#include "exportSubscription.h"  //for find q handle
#include "resourceSetStats.h"

/// @brief Data exported for a single node in a queue
typedef struct tag_ieieExportedSimpQNode_t {
    uint64_t queueId;
    uint64_t messageId;
    uint8_t  msgFlags;
} ieieExportedSimpQNode_t;

int32_t ieie_exportSimpQNode( ieutThreadData_t *pThreadData
                            , ieieExportResourceControl_t *control
                            , iesqQueue_t *Q
                            , uint64_t    orderId
                            , iesqQNode_t *pNode)
{
    int32_t rc = OK;

    assert(pNode->msg != NULL);

    rc = ieie_exportMessage( pThreadData
                           , pNode->msg
                           , control
                           , false);

    if (OK == rc)
    {
         // Initialize to zero to ensure unused fields are obvious (and usable in the future)
        ieieExportedSimpQNode_t expData = {0};

        expData.queueId = (uint64_t)Q;
        expData.messageId = (uint64_t)(pNode->msg);
        expData.msgFlags = pNode->msgFlags;

        rc = ieie_writeExportRecord( pThreadData
                                   , control
                                   , ieieDATATYPE_EXPORTEDQNODE_SIMPLE
                                   , orderId
                                   , (uint32_t)sizeof(expData)
                                   , (void *)&expData);

    }

    return rc;
}


//****************************************************************************
/// @internal
///
/// @brief  Export all messages on a simple queue
///
/// @param[in]     message              The message being exported
/// @param[in]     control              Export control structure
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_exportSimpQMessages(ieutThreadData_t *pThreadData,
                                 ismQHandle_t Qhdl,
                                 ieieExportResourceControl_t *control)
{
    assert(ieq_getQType(Qhdl) == simple);
    iesqQueue_t *Q = (iesqQueue_t *)Qhdl;
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
        iesqQNode_t *currNode = Q->head;

        uint64_t orderId = 0;


        while(rc == OK && currNode != NULL)
        {
            ismEngine_Message_t *msg = currNode->msg;

            if (msg != MESSAGE_STATUS_ENDPAGE)
            {
                if (msg == NULL)
                {
                    //We're at the end of the messages, we're done
                    break;
                }
                else if (msg != MESSAGE_STATUS_REMOVED)
                {

                    rc = ieie_exportSimpQNode(pThreadData, control, Q, ++orderId, currNode);
                }
                currNode = currNode + 1;
            }
            else
            {
                currNode = iesq_getNextNodeFromPageEnd(
                                     pThreadData,
                                     Q,
                                     currNode);
            }
        }

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

int32_t ieie_importSimpQNode(ieutThreadData_t            *pThreadData,
                             ieieImportResourceControl_t *pControl,
                             uint64_t                     dataId,
                             void                        *data,
                             size_t                       dataLen)
{
    int32_t rc = OK;
    ieieExportedSimpQNode_t *impData = (ieieExportedSimpQNode_t *)data;

    ismQHandle_t qhdl = NULL;
    ismEngine_Message_t *message = NULL;
    bool releaseMsg = false;

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
        //Check that the queue is a simple queue
        if (qhdl != NULL && (ieq_getQType(qhdl) == simple))
        {
            DEBUG_ONLY iesqQueue_t *Q = (iesqQueue_t *)qhdl;

            //Because SimpQ is a simple FIFO we can check the messages are in the
            //order we expect...
            assert(((Q->enqueueCount + Q->rejectedMsgs) + 1) == dataId);

            rc = iesq_importMessage( pThreadData
                                   , qhdl
                                   , message );

            if (rc == OK)
            {
                // Our refcount from findImportedMessage was used for import...
                releaseMsg = false;
            }
        }
        else if(qhdl == NULL)
        {
            //We chose not to import the queue... ignore messages on it
            ieutTRACEL(pThreadData, impData->queueId, ENGINE_HIGH_TRACE,
                       "Ignoring simple queue node: %lu, qDataId %lu\n",
                       dataId, impData->queueId);
        }
        else
        {
            rc = ISMRC_FileCorrupt;
        }
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
