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
/// @file  messageDelivery.c
/// @brief Management of message delivery
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "messageDelivery.h"
#include "memHandler.h"
#include "queueCommon.h"
#include "waiterStatus.h"
#include "clientState.h"
#include "resourceSetStats.h"

#include "ismutil.h"

static void scheduleRestartMessageDelivery( ieutThreadData_t *pThreadData
                                          , ismEngine_Session_t *pSession);

// Callback from destination when a message is being delivered
bool ism_engine_deliverMessage(ieutThreadData_t *pThreadData,
                               ismEngine_Consumer_t *pConsumer,
                               void *pDelivery,
                               ismEngine_Message_t *pMessage,
                               ismMessageHeader_t *pMsgHdr,
                               ismMessageState_t messageState,
                               uint32_t deliveryId)
{
    bool reenableWaiter = true;
    ismEngine_DeliveryHandle_t hDeliveryHandle;

    ieutTRACEL(pThreadData, pDelivery, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hConsumer %p, hDelivery %p, hMessage %p, Reliability %d, messageState %d, deliveryId %u, Length=%ld)\n",
               __func__, pConsumer, pDelivery, pMessage, pMessage->Header.Reliability, messageState, deliveryId, pMessage->MsgLength);

#if 0
    for (int i=0; i < pMessage->AreaCount; i++)
    {
        ieutTRACEL(pThreadData, pMessage->AreaLengths[i], ENGINE_HIGH_TRACE,
                   "MSGDETAILS(deliver): Area(%d) Type(%d) Length(%ld) Data(%.*s)\n",
                   i,
                   pMessage->AreaTypes[i],
                   pMessage->AreaLengths[i],
                   (pMessage->AreaLengths[i] > 100)?100:pMessage->AreaLengths[i],
                   (pMessage->AreaTypes[i] == ismMESSAGE_AREA_PAYLOAD)?pMessage->pAreaData[i]:"");
    }
#endif

    //Do a basic expiry check before we call the protocol layer. We only expire messages that have never
    //been delivered as some protocols (e.g. MQTT) are forward only and once we have sent it, it should
    //not be expired before the client says so. For other protocols (JMS) where we can expire messages
    //that have previously been delivered we need to do the expiry in the protocol
    bool expired =  (    (pMsgHdr->Expiry != 0)
                      && (pMsgHdr->RedeliveryCount == 0)
                      && (pMsgHdr->Expiry < ism_common_nowExpire()));
    if (expired)
    {
        if (pDelivery)
        {
            int32_t rc = ieq_acknowledge( pThreadData
                                        , pConsumer->queueHandle
                                        , pConsumer->pSession
                                        , NULL
                                        , pDelivery
                                        , ismENGINE_CONFIRM_OPTION_EXPIRED
                                        , NULL);

            if (UNLIKELY(rc != OK))
            {
                //We are in the process of delivering this message...
                //...it can't have gone anywhere - what's going on???
                ieutTRACE_FFDC( ieutPROBE_001, true
                              , "Can't mark node expired.", ISMRC_Error
                              , NULL );
            }
        }
        else
        {
            //Record a message expired without referring to an individual node
            //(as that has already been consumed)
            ieq_messageExpired( pThreadData
                              , pConsumer->queueHandle);
        }
        ism_engine_releaseMessage((ismEngine_MessageHandle_t)pMessage);
        return true;
    }
    else
    {
        if (pDelivery)
        {
            ismEngine_DeliveryInternal_t hTempHandle = { .Parts = { pConsumer->queueHandle, pDelivery } };

            hDeliveryHandle = hTempHandle.Full;
        }
        else
        {
            hDeliveryHandle = ismENGINE_NULL_DELIVERY_HANDLE;
        }

        reenableWaiter = pConsumer->pMsgCallbackFn(pConsumer,
                                                   hDeliveryHandle,
                                                   (ismEngine_MessageHandle_t)pMessage,
                                                   deliveryId,
                                                   messageState,
                                                   pConsumer->DestinationOptions,
                                                   pMsgHdr,
                                                   pMessage->AreaCount,
                                                   pMessage->AreaTypes,
                                                   pMessage->AreaLengths,
                                                   pMessage->pAreaData,
                                                   pConsumer->pMsgCallbackContext);
    }
    ieutTRACEL(pThreadData, reenableWaiter, ENGINE_CEI_TRACE,
               FUNCTION_EXIT "reenableWaiter='%s'\n", __func__,
               reenableWaiter ? "true" : "false");
    return reenableWaiter;
}



// Callback from destination when delivery status changes, such as
// an empty destination or disabling a waiter
// fir waiterdisabled/waiterterminated, goneasync says whether we are inline or have gone async
void ism_engine_deliverStatus( ieutThreadData_t *pThreadData
                             , ismEngine_Consumer_t *pConsumer
                             , int32_t retcode)
{
    ismEngine_Session_t *pSession = pConsumer->pSession;
    iereResourceSetHandle_t resourceSet = pSession->pClient->resourceSet;

    ieutTRACEL(pThreadData, pSession, ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hConsumer %p, retcode %d)\n",
               __func__, pConsumer, retcode);

    if (retcode == ISMRC_WaiterDisabled)
    {
        uint32_t oldCBcount = __sync_fetch_and_sub(&pSession->ActiveCallbacks, 1);
        assert(oldCBcount > 0);

        if (oldCBcount == 1)
        {
            //Reduced the last count...
            if (pSession->fIsDeliveryStopping)
            {
                bool doSessionRelease = true;
                __sync_lock_release(&pSession->fIsDeliveryStopping);

                if (pSession->pClient->hMsgDeliveryInfo != NULL)
                {
                    bool doRestart = iecs_canRestartDelivery(pThreadData,
                                                             pSession->pClient->hMsgDeliveryInfo);

                    if (doRestart)
                    {
                        //Start delivery if session is still engine controlled
                        //This is getting incestuous, we're in the process of stopping and we want to
                        //start again... in order to prevent winding up  a nest thread stack of
                        //starting->stopping->starting->stopping->....., we'll schedule the restart to
                        //happen on a new thread...
                        scheduleRestartMessageDelivery(pThreadData, pSession);

                        doSessionRelease = false; //schedule call inherited usecount we were about to
                                                  //decrement
                    }
                }
                if (doSessionRelease)
                {
                    releaseSessionReference(pThreadData, pSession, false);
                }
            }
        }
    }
    else if (retcode == ISMRC_WaiterRemoved)
    {
        if (pConsumer->hMsgDelInfo != NULL)
        {
            //If we have been told to relinquish messages when we go away, do that now..
            if (pConsumer->relinquishOnTerm != ismEngine_RelinquishType_NONE)
            {
                iecsRelinquishType_t iecsRelinqType;

                // Relinquish messages that are on this queue now
                if (pConsumer->relinquishOnTerm == ismEngine_RelinquishType_ACK_HIGHRELIABLITY)
                {
                    iecsRelinqType = iecsRELINQUISH_ACK_HIGHRELIABILITY_ON_QUEUE;
                }
                else
                {
                    iecsRelinqType = iecsRELINQUISH_NACK_ALL_ON_QUEUE;
                    assert(pConsumer->relinquishOnTerm == ismEngine_RelinquishType_NACK_ALL);
                }
                iecs_relinquishAllMsgs(pThreadData,
                                       pConsumer->hMsgDelInfo,
                                       &pConsumer->queueHandle,
                                       1,
                                       iecsRelinqType);
            }

            iecs_releaseMessageDeliveryInfoReference(pThreadData, pConsumer->hMsgDelInfo);
            pConsumer->hMsgDelInfo = NULL;
        }

        //all we need to do is free ourselves (pConsumer->pDestination was free'd earlier)
        iere_primeThreadCache(pThreadData, resourceSet);
        iere_freeStruct(pThreadData, resourceSet, iemem_externalObjs, pConsumer, pConsumer->StrucId);

        //and finally allow the session to go away...
        assert(pSession != NULL);
        releaseSessionReference(pThreadData, pSession, false);
    }

    ieutTRACEL(pThreadData, pConsumer, ENGINE_CEI_TRACE, FUNCTION_EXIT"\n", __func__);
    return;
}

static int restartMessageDelivery(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    ismEngine_Session_t *pSession = (ismEngine_Session_t *)userdata;

    // Make sure we're thread-initialised. This can be issued repeatedly.
    ism_engine_threadInit(0);

    ieutThreadData_t *pThreadData = ieut_enteringEngine(pSession->pClient);

    ieutTRACEL(pThreadData, pSession, ENGINE_CEI_TRACE, FUNCTION_IDENT "pSession=%p\n"
                       , __func__, pSession);

    int32_t rc = ism_engine_startMessageDelivery(pSession,
                                                 ismENGINE_START_DELIVERY_OPTION_ENGINE_START,
                                                 NULL, 0, NULL);

    if (  (rc == OK)
        ||(rc == ISMRC_NotEngineControlled)
        ||(rc == ISMRC_Destroyed))
    {
        //This is the session reference scheduleRestartMessageDelivery()
        //inherited (the startMessageDelivery call above will have created
        //a reference if it needs to
        releaseSessionReference(pThreadData, pSession, false);
    }
    else
    {
        //Couldn't restart (hopefully just couldn't yet... schedule to try again:
        assert (rc == ISMRC_RequestInProgress);
        if (rc != ISMRC_RequestInProgress)
        {
            ieutTRACE_FFDC( ieutPROBE_001, false
                          , "Failed to restart session delivery.", rc
                          , NULL );
        }
        scheduleRestartMessageDelivery(pThreadData, pSession);
    }

    ieut_leavingEngine(pThreadData);
    ism_common_cancelTimer(key);

    //We did engine threadInit...  term will happen when the engine terms and after the counter
    //of events reaches 0
    __sync_fetch_and_sub(&(ismEngine_serverGlobal.TimerEventsRequested), 1);
    return 0;
}

//inherits a session usecount from the caller (so the caller may need
//to increase it if it still needs its own).
static void scheduleRestartMessageDelivery( ieutThreadData_t *pThreadData
                                          , ismEngine_Session_t *pSession)
{
     ieutTRACEL(pThreadData, pSession, ENGINE_CEI_TRACE, FUNCTION_IDENT "pSession=%p\n"
                        , __func__, pSession);

     __sync_fetch_and_add(&(ismEngine_serverGlobal.TimerEventsRequested), 1);

    (void)ism_common_setTimerOnce(ISM_TIMER_HIGH,
                                  restartMessageDelivery,
                                  pSession, 20);
}

/*********************************************************************/
/*                                                                   */
/* End of messageDelivery.c                                          */
/*                                                                   */
/*********************************************************************/
