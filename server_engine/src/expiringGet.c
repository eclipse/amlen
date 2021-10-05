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
/// @file  expiringGet.c
/// @brief Module for code that allows a get with a time out
//*********************************************************************


#define TRACE_COMP Engine


#include <stdint.h>
#include <stdbool.h>
#include "engineInternal.h"
#include "expiringGetInternal.h"
#include "memHandler.h"

static ism_priority_class_e ExpiringGetTimerThead = ISM_TIMER_LOW;

static inline void iegiLockExpGetInfo(iegiExpiringGetInfo_t *expGetInfo)
{
    int os_rc = pthread_mutex_lock(&(expGetInfo->lock));

    if (os_rc != 0)
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                , "Taking expiring get lock.", os_rc
                , NULL );
    }
}
static inline void iegiUnlockExpGetInfo(iegiExpiringGetInfo_t *expGetInfo)
{
    int os_rc = pthread_mutex_unlock(&(expGetInfo->lock));

    if (os_rc != 0)
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                , "Releasing expiring get lock.", os_rc
                , NULL );
    }
}


//When we try to cancel the timer, we are told "OK" even if timer is running,
//hence we can't be sure when the timer thread is finished with ExpGetInfo.
//So this function runs on the timer thread to free the memory
static int iegiFinalReleaseExpGetInfo(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    iegiExpiringGetInfo_t *expGetInfo = (iegiExpiringGetInfo_t *)userdata;

    // Make sure we're thread-initialised. This can be issued repeatedly.
    ism_engine_threadInit(0);

    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ieutTRACEL(pThreadData, expGetInfo,  ENGINE_CEI_TRACE, FUNCTION_IDENT "(expGetInfo %p, key %p)\n", __func__,
                                                                                            expGetInfo, key);

    expGetInfo->eventCountState |= iegiEVENTCOUNT_FREE_TIMER_FIRED;

    //The memory needs to freed on the timer thread (so we know the timer osn't running and using it...
    if ((expGetInfo->pTimerFiredThread == NULL) || (pThreadData == expGetInfo->pTimerFiredThread))
    {
        ismEngine_CheckStructId(expGetInfo->StrucId, ismENGINE_EXPIRINGGETINFO_STRUCID, ieutPROBE_001);
        iemem_freeStruct(pThreadData, iemem_expiringGetData, expGetInfo, expGetInfo->StrucId);
    }
    else
    {
        //somehow we're on a different thread... can't free the memory
        assert(0);

        ieutTRACE_FFDC( ieutPROBE_001, false
                        , "Free mem thread wasn't run on timer thread that event fired on", ISMRC_Error
                        , NULL );
    }

    ieut_leavingEngine(pThreadData);

    ism_common_cancelTimer(key);

    //We did engine threadInit...  term will happen when the engine terms and after the counter
    //of events reaches 0
    __sync_fetch_and_sub(&(ismEngine_serverGlobal.TimerEventsRequested), 1);

    return 0;
}
//fInline is whetherthis was called in-line in ism_engine_getMessage()
//or whether we went async (and hence the callback need to be called
static inline void iegiFinishedWithExpGetInfo( ieutThreadData_t *pThreadData
                                        , iegiExpiringGetInfo_t *expGetInfo
                                        , bool fInline)
{
    ieutTRACEL(pThreadData, expGetInfo,  ENGINE_FNC_TRACE, FUNCTION_IDENT "expGetInfo %p\n",
                                        __func__, expGetInfo);

    iegiLockExpGetInfo(expGetInfo);

    if (!expGetInfo->doneCompletion)
    {
        if (       (!fInline)
                && (!(expGetInfo->completionCallbackFired))
                && (expGetInfo->pCompletionCallbackFn != NULL))
        {
            int32_t rc = OK;

            if (!expGetInfo->messageDelivered)
            {
                if (expGetInfo->recursivelyDestroyed)
                {
                    rc = ISMRC_Destroyed;
                }
                else
                {
                    rc = ISMRC_NoMsgAvail;
                }
            }
            ieutTRACEL(pThreadData, expGetInfo,  ENGINE_FNC_TRACE,"Calling completion\n");
            expGetInfo->pCompletionCallbackFn(rc, NULL, expGetInfo->pCompletionContext);
            expGetInfo->completionCallbackFired = true;
        }
        //Now we've fired our callback we can release the session, but we wanted our callback first...
        releaseSessionReference(pThreadData, expGetInfo->hSession, false);
        expGetInfo->doneCompletion = true;
    }
    ismEngine_CheckStructId(expGetInfo->StrucId, ismENGINE_EXPIRINGGETINFO_STRUCID, ieutPROBE_001);

    iegiUnlockExpGetInfo(expGetInfo);

    //Can't be sure when timer thread is finished with memory (cancel returns ok even if timer
    //running) so we free the memory on the timer thread...
    expGetInfo->eventCountState |= iegiEVENTCOUNT_FREE_TIMER_STARTED;
    __sync_fetch_and_add(&(ismEngine_serverGlobal.TimerEventsRequested), 1);
    ism_timer_t releaseMemTimer = ism_common_setTimerOnce(ExpiringGetTimerThead,
                                                          iegiFinalReleaseExpGetInfo,
                                                          expGetInfo,
                                                          100);

    if (releaseMemTimer == NULL)
    {
        //We're going to leak with memory as we couldn't free it on timer thread
        __sync_fetch_and_sub(&(ismEngine_serverGlobal.TimerEventsRequested), 1);
        ieutTRACE_FFDC( ieutPROBE_002, false
                        , "Couldn't schedule freeing on expiringGetMemory", ISMRC_Error
                        , NULL );
    }
}


//Called with expGetInfo Locked... if returns true: expGetInfo freed
//                                            false: expGetInfo still locked
static bool iegiConsumerDestroyed( ieutThreadData_t *pThreadData
                                 , iegiExpiringGetInfo_t *expGetInfo
                                 , bool fInline)
{
    ieutTRACEL(pThreadData, expGetInfo,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);
    bool everythingFinished = false;

    assert(expGetInfo->timerFinished); //We should have cancelled the timer before the destroy completed

    expGetInfo->consumerDestroyFinished = true;

    if (expGetInfo->constructionFinished)
    {
        //Both usecounts set...
        everythingFinished = true;
    }

    if (everythingFinished)
    {
        iegiUnlockExpGetInfo(expGetInfo);
        iegiFinishedWithExpGetInfo(pThreadData, expGetInfo, fInline);
    }
    ieutTRACEL(pThreadData, everythingFinished,  ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return everythingFinished;
}

//Takes a locked expGetInfo
static void iegiCancelTimer( ieutThreadData_t *pThreadData
                           , iegiExpiringGetInfo_t *expGetInfo )
{
    ism_timer_t timerKey = expGetInfo->timerKey;


    if (!expGetInfo->timerFinished)
    {
        if (timerKey != NULL)
        {
            __sync_fetch_and_sub(&(ismEngine_serverGlobal.TimerEventsRequested), 1);
            expGetInfo->eventCountState |= iegiEVENTCOUNT_DEC_CANCELLED;
            expGetInfo->timerKey = NULL;

            //Cancelling timer returns ok even if timer task is running so we can't
            //be actually sure timer thread is finished with it. So we'll free it on
            //the timer thread (must be SAME timer thread) so we know it's not currently
            //in use
            int32_t rc = ism_common_cancelTimer(timerKey);

            if (rc == OK)
            {
                //cancel returns true if timer already fired... so we may set this flag that
                //is already set... doesn't matter
                expGetInfo->timerFinished = true;
                expGetInfo->timerCancelState |= iegiCANCELSTATE_BYFUNC;
            }
            else
            {
                expGetInfo->timerCancelState |= iegiCANCELSTATE_CANCELFAILED;
                ieutTRACE_FFDC( ieutPROBE_001, true
                                , "Couldn't cancel timer.", rc
                                , NULL );
            }
        }
        else
        {
            expGetInfo->timerCancelState |= iegiCANCELSTATE_NOKEY;
        }
    }
    else
    {
        expGetInfo->timerCancelState |= iegiCANCELSTATE_ALREADYFINISHED;
    }
}


//Called with expGetInfo Unlocked
static void iegiConsumerDestroyedAsync(int32_t rc, void *handle, void *context)
{
    iegiExpiringGetInfo_t *expGetInfo = *(iegiExpiringGetInfo_t **)context;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    ieutTRACEL(pThreadData, expGetInfo,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(expInfo %p)\n", __func__,
                                                                                            expGetInfo );

    bool everythingFreed = false;

    iegiLockExpGetInfo(expGetInfo);

    expGetInfo->consumerState |= iegiCONSSTATE_DESTROYEDASYNC;

    if (!(expGetInfo->consumerDestroyStarted))
    {
        //We're being recursively destroyed by the session/client - we haven't asked for it!
        expGetInfo->recursivelyDestroyed = true;
        expGetInfo->consumerDestroyStarted = true;

        if (    expGetInfo->timerCreated
                && !expGetInfo->timerFinished)
        {
            if (expGetInfo->timerKey != NULL)
            {
                iegiCancelTimer(pThreadData, expGetInfo);
            }
        }
        else
        {
            expGetInfo->timerFinished = true;
        }
    }

    everythingFreed = iegiConsumerDestroyed(pThreadData, expGetInfo, false);

    if (!everythingFreed)
    {
        iegiUnlockExpGetInfo(expGetInfo);
    }

    ieutTRACEL(pThreadData, expGetInfo,  ENGINE_CEI_TRACE, FUNCTION_EXIT "\n", __func__);
    ieut_leavingEngine(pThreadData);
}

//NB: Currently called with the expGetInfo Locked... some debate if we
//needed to unlock before destroy (e.g. if destroy might try and lock expGetLock
//e.g. by calling async callback syncly).
//At the moment, safe to call destroyConsumer with expGetInfo locked. If need to
//unlock, add to consumer usecount before associating expGetInfo and reduce it
//when expGetInfo freed so destroySession can't call our recursive destroy after unlock
//but before we call destroyConsumer
//
// NB: We only set pMsgDlvd if destroyConsumer returns synchronously as we
// we only care about it in that case (it's for returning NoMSgAvail if *everything*
// happens synchronously
//
static bool iegiDestroyConsumer( ieutThreadData_t *pThreadData
                               , iegiExpiringGetInfo_t *expGetInfo
                               , bool fInline
                               , bool *pMsgDlvd)
{
    bool everythingFreed = false;

    ieutTRACEL(pThreadData, expGetInfo->hConsumer,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "pCons %p\n", __func__,
                               expGetInfo->hConsumer);

    int32_t rc = ism_engine_destroyConsumer(
                            expGetInfo->hConsumer,
                            NULL, 0, //For expiring get consumers we use the msgDlvry context to avoid duplication
                            iegiConsumerDestroyedAsync);

    if (rc == OK)
    {
        if (pMsgDlvd)*pMsgDlvd = expGetInfo->messageDelivered;
        everythingFreed = iegiConsumerDestroyed(pThreadData, expGetInfo, fInline);
    }
    else if (rc == ISMRC_Destroyed)
    {
        //Can happen if timer expires after a session has started recursive destroy, the
        //recursive destroy will cancel the timer before real harm can be done
        //(so we have to hold our nose and absorb this error)
        ieutTRACEL(pThreadData, ISMRC_Destroyed, ENGINE_UNUSUAL_TRACE,
                "Consumer was already destroyed (presumably by destruction of session)\n");
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
        ieutTRACE_FFDC( ieutPROBE_001, true
                , "destroying expiring get consumer", rc
                , NULL );
    }
    ieutTRACEL(pThreadData, expGetInfo,  ENGINE_FNC_TRACE, FUNCTION_EXIT "expGetInfo %p\n", __func__, expGetInfo);

    return everythingFreed;
}

static int iegiTimerExpired(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    iegiExpiringGetInfo_t *expGetInfo = (iegiExpiringGetInfo_t *)userdata;
    bool doDestroyConsumer = false;
    bool expGetInfoFreed = false;
    bool reduceTimerEventsRequestedCounter = false;

    // Make sure we're thread-initialised. This can be issued repeatedly.
    ism_engine_threadInit(0);

    ieutThreadData_t *pThreadData = NULL;

    iegiLockExpGetInfo(expGetInfo);

    if (!expGetInfo->timerFinished)
    {
        expGetInfo->timerFiredState |= iegiFIREDSTATE_FIRED;

        //We can rely on the session having not yet been freed
        ismEngine_ClientState_t *pClient = expGetInfo->hSession->pClient;

        pThreadData = ieut_enteringEngine(pClient);
        ieutTRACEL(pThreadData, pClient,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(key %p Client %p)\n", __func__,
                                                                                                 key, pClient);

        expGetInfo->timerFinished     = true;
        expGetInfo->pTimerFiredThread = pThreadData; //Record which thread timer fired on so we can
                                                     //check we free expGetInfo on same thread

        if (expGetInfo->timerKey != NULL)
        {
            reduceTimerEventsRequestedCounter = true;

            expGetInfo->timerCancelState |= iegiCANCELSTATE_WHENFIRED;
            expGetInfo->eventCountState  |= iegiEVENTCOUNT_DEC_FIRED;

            if (!expGetInfo->consumerDestroyStarted)
            {
                doDestroyConsumer = true;
                expGetInfo->consumerDestroyStarted = true;
                expGetInfo->consumerState |= iegiCONSSTATE_DESTROYTIMERFIRED;
            }
        }
        else
        {
            //Msg delivery callback has fired and is about to cancel us... just leave them to it
            expGetInfo->timerFiredState = iegiFIREDSTATE_NOKEY;
        }
    }
    else
    {
        //Timer cancelled but we were already started... do nothing - we can access expGetInfo
        //(as it is this thread that frees it AFTER a cancel... but can't do anything else
        pThreadData = ieut_enteringEngine(NULL);
        ieutTRACEL(pThreadData, expGetInfo,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(expGetInfo %p)\n", __func__,
                                                                                                     expGetInfo);
        expGetInfo->timerFiredState |= iegiFIREDSTATE_TOOLATE;
    }

    if (doDestroyConsumer)
    {
        expGetInfoFreed = iegiDestroyConsumer(pThreadData, expGetInfo, false, NULL);
    }

    if (!expGetInfoFreed)
    {
        iegiUnlockExpGetInfo(expGetInfo);
    }

    ieutTRACEL(pThreadData, expGetInfo,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(expGetInfo %p)\n", __func__, expGetInfo);
    ieut_leavingEngine(pThreadData);



    //We did engine threadInit...  term will happen when the engine terms and after the counter
    //of events reaches 0
    if (reduceTimerEventsRequestedCounter)
    {
        ism_common_cancelTimer(key);
        __sync_fetch_and_sub(&(ismEngine_serverGlobal.TimerEventsRequested), 1);

    }
    return 0;
}

static bool iegiMessageArrived(
        ismEngine_ConsumerHandle_t      hConsumer,
        ismEngine_DeliveryHandle_t      hDelivery,
        ismEngine_MessageHandle_t       hMessage,
        uint32_t                        deliveryId,
        ismMessageState_t               state,
        uint32_t                        destinationOptions,
        ismMessageHeader_t *            pMsgDetails,
        uint8_t                         areaCount,
        ismMessageAreaType_t            areaTypes[areaCount],
        size_t                          areaLengths[areaCount],
        void *                          pAreaData[areaCount],
        void *                          pConsumerContext)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(hConsumer->pSession->pClient);
    ieutTRACEL(pThreadData, hConsumer,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "(hCons %p)\n", __func__,hConsumer);

    iegiExpiringGetInfo_t *expGetInfo = *(iegiExpiringGetInfo_t **)pConsumerContext;
    bool doDestroyConsumer = false;
    bool expGetInfoFreed   = false;

    DEBUG_ONLY bool wantMoreMessages = expGetInfo->pMessageCallbackFn( hConsumer
                                         , hDelivery
                                         , hMessage
                                         , deliveryId
                                         , state
                                         , destinationOptions
                                         , pMsgDetails
                                         , areaCount
                                         , areaTypes
                                         , areaLengths
                                         , pAreaData
                                         , expGetInfo->pMessageContext);

    assert(!wantMoreMessages); //We could extend this to allow multiple messages and pass this back and only do following code when
                               //we know (somehow) that we want no more...

    iegiLockExpGetInfo(expGetInfo);

    expGetInfo->messageDelivered = true; //record that we did give a message to this getter

    if (  !(expGetInfo->consumerDestroyStarted) )
    {
        if (expGetInfo->hConsumer && expGetInfo->constructionFinished)
        {
            //If construction isn't complete then they'll destroy it when it is
            expGetInfo->consumerDestroyStarted = true;
            doDestroyConsumer = true;
        }

        if (    expGetInfo->timerCreated
            && !expGetInfo->timerFinished)
        {
            if (expGetInfo->timerKey != NULL)
            {
                iegiCancelTimer(pThreadData, expGetInfo);
            }
        }
        else
        {
            expGetInfo->timerFinished = true;
            expGetInfo->timerCancelState |= iegiCANCELSTATE_MSGARRIVDNOTIMER;
        }
    }

    if (doDestroyConsumer)
    {
        expGetInfoFreed = iegiDestroyConsumer(pThreadData, expGetInfo, false, NULL);
        assert(!expGetInfoFreed); //Consumer can't have finished destroy, we're in its callback!!!
    }

    if (!expGetInfoFreed)
    {
        iegiUnlockExpGetInfo(expGetInfo);
    }

    ieutTRACEL(pThreadData, expGetInfoFreed,  ENGINE_FNC_TRACE,
              FUNCTION_EXIT "(getInfoFreed %u)\n", __func__, expGetInfoFreed);
    ieut_leavingEngine(pThreadData);
    return false;
}

static int32_t iegiConsumerCreated( ieutThreadData_t *pThreadData
                                  , iegiExpiringGetInfo_t *expGetInfo
                                  , bool fInline)
{
    bool destroyConsumer        = false;
    bool expGetInfoFreed        = false;
    bool finishedWithExpGetInfo = false;

    ieutTRACEL(pThreadData, expGetInfo,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "(expGI %p, inline %d)\n", __func__,
                                                                                      expGetInfo, fInline);

    int32_t rc = OK;


    iegiLockExpGetInfo(expGetInfo);

    //Ensure our destroy callback gets called even during recursive destroys
    expGetInfo->hConsumer->fExpiringGet = true;
    expGetInfo->hConsumer->pPendingDestroyCallbackFn = iegiConsumerDestroyedAsync;

    if (expGetInfo->messageDelivered)
    {
        assert(expGetInfo->timerFinished); //The messageDelivered callback will have set it

        //Do we need to start the destroy of the consumer?
        if (!expGetInfo->consumerDestroyStarted)
        {
            expGetInfo->consumerState |= iegiCONSSTATE_DESTROYONCOMPLETE;
            expGetInfo->consumerDestroyStarted = true;
            destroyConsumer = true;
        }
    }
    else
    {
        if (expGetInfo->timeOutMillis > 0)
        {
            expGetInfo->eventCountState |= iegiEVENTCOUNT_INCREASED_CREATION;

            __sync_fetch_and_add(&(ismEngine_serverGlobal.TimerEventsRequested), 1);
            expGetInfo->timerKey = ism_common_setTimerOnce(ExpiringGetTimerThead,
                                                           iegiTimerExpired,
                                                           expGetInfo,
                                                           expGetInfo->timeOutMillis*1000000);

            if (expGetInfo->timerKey != NULL)
            {
                expGetInfo->timerCreated = true;
            }
            else
            {
                __sync_fetch_and_sub(&(ismEngine_serverGlobal.TimerEventsRequested), 1);
                expGetInfo->timerFinished = true;
                rc = ISMRC_Error;

                ieutTRACE_FFDC( ieutPROBE_001, false
                        , "Failed to create timer for expiring get.", rc
                        , NULL );
            }
        }
        else
        {
            //Don't want to wait... just start the destroy.
            expGetInfo->timerFinished = true;
            if (!expGetInfo->consumerDestroyStarted)
            {
                expGetInfo->consumerState = iegiCONSSTATE_DESTROYNOTIMER;
                expGetInfo->consumerDestroyStarted = true;
                destroyConsumer = true;
            }
        }
    }


    if (rc == OK)
    {
        assert(expGetInfo->constructionFinished == false); //we haven't got here yet
        expGetInfo->constructionFinished = true;
    }
    else
    {
        //Do we need to start the destroy of the consumer?
        if (!expGetInfo->consumerDestroyStarted)
        {
            expGetInfo->consumerState |= iegiCONSSTATE_DESTROYCREATEFAILED;
            expGetInfo->consumerDestroyStarted = true;
            destroyConsumer = true;
        }
    }

    //Is the other usecount flag already set?
    if(expGetInfo->consumerDestroyFinished)
    {
        assert(!destroyConsumer);
        finishedWithExpGetInfo = true;
    }

    bool messageDelivered=false;

    if (destroyConsumer)
    {
        expGetInfoFreed = iegiDestroyConsumer(pThreadData, expGetInfo, fInline, &messageDelivered);
    }

    if (!expGetInfoFreed)
    {
        iegiUnlockExpGetInfo(expGetInfo);

        if (finishedWithExpGetInfo)
        {
            iegiFinishedWithExpGetInfo(pThreadData, expGetInfo, fInline);
            expGetInfoFreed = true;
        }
    }

    if (rc == OK && !expGetInfoFreed)
    {
        //We're not done yet
        rc = ISMRC_AsyncCompletion;
    }
    else if (rc == OK && !messageDelivered)
    {
        //Everything has finished but we haven't delivered the message.
        rc = ISMRC_NoMsgAvail;
    }
    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "(rc %d)\n", __func__, rc);

    return rc;
}

static void iegiConsumerCreatedAsync(int32_t rc, void *handle, void *context)
{
    ismEngine_Consumer_t *pConsumer = (ismEngine_Consumer_t *)handle;
    iegiExpiringGetInfo_t *expGetInfo = *(iegiExpiringGetInfo_t **)context;

    ieutThreadData_t *pThreadData = ieut_enteringEngine(pConsumer->pSession->pClient);
    ieutTRACEL(pThreadData, expGetInfo,  ENGINE_FNC_TRACE, FUNCTION_ENTRY "(expGI %p)\n", __func__,
                                                                                      expGetInfo);

    if (rc == OK)
    {
        expGetInfo->hConsumer = (ismEngine_Consumer_t *)handle;

        rc = iegiConsumerCreated(pThreadData, expGetInfo, false);

        if (rc != OK)
        {
            //Want to destroy but consumer may already be delivering the message
            bool ableToCompleteDestroy = false;

            iegiLockExpGetInfo(expGetInfo);

            if (expGetInfo->pCompletionCallbackFn != NULL)
            {
                expGetInfo->pCompletionCallbackFn(rc, NULL, expGetInfo->pCompletionContext);
                expGetInfo->completionCallbackFired = true;
            }

            assert(!expGetInfo->constructionFinished); //It didn't finish successfully
            expGetInfo->constructionFinished = true; //as constructed as it going to get
            if (expGetInfo->consumerDestroyFinished)
            {
                ableToCompleteDestroy = true;
            }
            iegiUnlockExpGetInfo(expGetInfo);

            if (ableToCompleteDestroy)
            {
                iegiFinishedWithExpGetInfo(pThreadData, expGetInfo, false);
            }
        }

    }
    else
    {
        if (expGetInfo->pCompletionCallbackFn != NULL)
        {
            expGetInfo->pCompletionCallbackFn(rc, NULL, expGetInfo->pCompletionContext);
            expGetInfo->completionCallbackFired = true;
        }
        iegiFinishedWithExpGetInfo(pThreadData, expGetInfo ,false);
    }

    ieutTRACEL(pThreadData, pConsumer,  ENGINE_FNC_TRACE, FUNCTION_EXIT "(hCons  %p)\n",
                              __func__, pConsumer);
    ieut_leavingEngine(pThreadData);
}

XAPI int32_t WARN_CHECKRC ism_engine_getMessage(
    ismEngine_SessionHandle_t                  hSession,
    ismDestinationType_t                       destinationType,
    const char *                               pDestinationName,
    const ismEngine_SubscriptionAttributes_t * pSubAttributes,
    uint64_t                                   deliverTimeOutMillis,    //< timeout in milliseconds
    ismEngine_ClientStateHandle_t              hOwningClient,     ///< Which client owns the subscription
    void *                                     pMessageContext,
    size_t                                     messageContextLength,
    ismEngine_MessageCallback_t                pMessageCallbackFn, ///< Must return false to first message
    const ism_prop_t *                         pConsumerProperties,
    uint32_t                                   consumerOptions,
    void *                                     pContext,
    size_t                                     contextLength,
    ismEngine_CompletionCallback_t             pCallbackFn)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(hSession->pClient);
    ieutTRACEL(pThreadData, hSession,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hSession %p, deliverTimeOut %lu)\n", __func__,
                                                                                        hSession, deliverTimeOutMillis);
    int32_t rc = OK;


    iegiExpiringGetInfo_t *expGetInfo = iemem_calloc( pThreadData
                                                    , IEMEM_PROBE(iemem_expiringGetData, 1)
                                                    , 1
                                                    ,    sizeof(iegiExpiringGetInfo_t)
                                                       + RoundUp8(messageContextLength)
                                                       + contextLength);

    if (rc == OK)
    {
        //The Session should not go away until we have finished with the expGetInfo...
        //this ensures if we go async, our callback will fire before the client/session destroy callback
        __sync_fetch_and_add(&(hSession->UseCount), 1);

        ismEngine_SetStructId(expGetInfo->StrucId, ismENGINE_EXPIRINGGETINFO_STRUCID);

        int os_rc = pthread_mutex_init(&(expGetInfo->lock), NULL);

        if (os_rc != 0)
        {
            ieutTRACE_FFDC( ieutPROBE_001, true
                          , "Initial expiring get lock.", os_rc
                          , NULL );
        }

        expGetInfo->hSession = hSession;

        expGetInfo->timeOutMillis = deliverTimeOutMillis;
        expGetInfo->pMessageCallbackFn = pMessageCallbackFn;
        expGetInfo->pMessageContext = (void *)(expGetInfo+1);
        memcpy(expGetInfo->pMessageContext, pMessageContext, messageContextLength);

        expGetInfo->pCompletionCallbackFn = pCallbackFn;
        expGetInfo->pCompletionContext =  expGetInfo->pMessageContext + RoundUp8(messageContextLength);
        memcpy(expGetInfo->pCompletionContext, pContext, contextLength);

        rc = ism_engine_createConsumer(
                                hSession,
                                destinationType,
                                pDestinationName,
                                pSubAttributes,
                                hOwningClient,
                                &expGetInfo,
                                sizeof(iegiExpiringGetInfo_t *),
                                iegiMessageArrived,
                                pConsumerProperties,
                                consumerOptions,
                                &(expGetInfo->hConsumer),
                                &expGetInfo,
                                sizeof(iegiExpiringGetInfo_t *),
                                iegiConsumerCreatedAsync);

        if (rc == OK)
        {
            rc = iegiConsumerCreated(pThreadData, expGetInfo, true);
        }
        else
        {
            releaseSessionReference(pThreadData, hSession, false);
            iemem_freeStruct(pThreadData, iemem_expiringGetData, expGetInfo, expGetInfo->StrucId);
            assert(rc != ISMRC_AsyncCompletion); //We want an extra usec
        }
    }


    ieutTRACEL(pThreadData, rc,  ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    ieut_leavingEngine(pThreadData);
    return rc;
}
