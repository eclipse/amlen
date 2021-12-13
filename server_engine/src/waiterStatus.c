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
#define TRACE_COMP Engine

#include "engineInternal.h"
#include "waiterStatus.h"
#include "simpQInternal.h"
#include "intermediateQInternal.h"
#include "multiConsumerQInternal.h"
#include "messageDelivery.h"

//Outside this file (actually outside ieq_completeWaiterActions)
//Waiterstatus should be unlocked by iews_bool_UnlockNoPending or ieq_completeWaiterActions
//(depending on whether work has been left for you to do).
//
//Inside ieq_completeWaiterActions() we need to be able to get rid of the flags hence:
#define iews_bool_Unlock(PtrVariable, fromState, toState)       __sync_bool_compare_and_swap(PtrVariable, fromState, toState);   \
                                                                assert(   ((fromState & IEWS_WAITERSTATUSMASK_LOCKED) != 0 )     \
                                                                       && ((toState   & IEWS_WAITERSTATUSMASK_LOCKED) == 0));

//pConsumer can be NULL if and only if the queue is a single consumer queue
static inline volatile iewsWaiterStatus_t *iews_getWaiterStatusPtr( ismQHandle_t Qhdl
                                                                  , ismEngine_Consumer_t *pConsumer)
{
    volatile iewsWaiterStatus_t *pWaiterStatus = NULL;

    switch(ieq_getQType(Qhdl))
    {
        case multiConsumer:
            assert(pConsumer != NULL);
            pWaiterStatus = &(pConsumer->iemqWaiterStatus);
            break;

        case simple:
            assert(pConsumer == NULL || ((iesqQueue_t *)Qhdl)->pConsumer == pConsumer);
            pWaiterStatus = &(((iesqQueue_t *)Qhdl)->waiterStatus);
            break;

        case intermediate:
            assert(pConsumer == NULL || ((ieiqQueue_t *)Qhdl)->pConsumer == pConsumer);
            pWaiterStatus = &(((ieiqQueue_t *)Qhdl)->waiterStatus);
            break;

        default:
            ieutTRACE_FFDC( ieutPROBE_001, true
                            , "Unexpected queue type in iews_getWaiterStatusPtr.", ISMRC_Error
                            , "pConsumer", pConsumer,  sizeof(*pConsumer)
                            , "queue",     Qhdl,  sizeof(ismEngine_Queue_t)
                            , NULL );
            break;

    }
    return pWaiterStatus;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Enable delivery of messages to the consumer
///  @remarks
///    Start delivery of messages to the consumer. If any messages exist
///    on the queue upon which this consumer is registered then messages
///    will be delivered to the consumer as part of this function call.
///
///  @param[in] Qhdl               - Queue
///  @param[in] pConsumer          - Consumer to enable
///  @param[in] options            - IEQ_ENABLE_OPTIONS_*
///
///  @return                       - OK on success
///                                  ISMRC_WaiterEnabled if already enabled
///                                  ISMRC_ISMRC_DisableWaiterCancel if:
///                                    waiter was marked for disable and we cleared it
///                                  or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ieq_enableWaiter( ieutThreadData_t *pThreadData,
                          ismQHandle_t Qhdl,
                          ismEngine_Consumer_t *pConsumer,
                          uint32_t enableOptions)
{
    ismEngine_Queue_t *Q = (ismEngine_Queue_t *)Qhdl;
    ieutTRACEL(pThreadData, Qhdl,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " Qhdl=%p\n", __func__, Qhdl);

    iewsWaiterStatus_t oldState = IEWS_WAITERSTATUS_DISCONNECTED;
    int32_t rc = OK;

    if (Q->QType == multiConsumer && ((iemqQueue_t *)Q)->isDeleted)
    {
        rc = ISMRC_QueueDeleted;
        goto mod_exit;
    }

    volatile iewsWaiterStatus_t *pWaiterStatus = iews_getWaiterStatusPtr(Qhdl, pConsumer);

    Q->informOnEmpty = true;
    bool doneEnable = false;

    //Set the state to enabled and find out what we changed it from
    do
    {
        oldState = *pWaiterStatus;

        //If the waiter is currently flagged as needing the disabled callback to
        //fire then we have to wait for the getter to finish and do the callback
        //(as the callback isn't allowed to fire before the getter completes)

        if (oldState & IEWS_WAITERSTATUS_DISCONNECT_PEND)
        {
            // You can't enable the waiter if it's disconnecting
            rc = ISMRC_WaiterInvalid;
            goto mod_exit;
        }
        else if (oldState == IEWS_WAITERSTATUS_DISABLED)
        {
            doneEnable =  iews_bool_changeState( pWaiterStatus
                                               , IEWS_WAITERSTATUS_DISABLED
                                               , IEWS_WAITERSTATUS_ENABLED);
        }
        else if (   (oldState & IEWS_WAITERSTATUSMASK_ACTIVE)
                 && ((oldState & ~(IEWS_WAITERSTATUSMASK_ACTIVE)) == 0))
        {
            // We are already in the enabled state so nothing for us to do
            rc = ISMRC_WaiterEnabled;
            goto mod_exit;
        }
        else if (oldState & IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND)
        {
            //This flag implies an earlier disable was cancelled but flags have not been reset yet
            //The waiter will end up enabled already so this enable request can treat it like its already
            //enabled.
            rc = ISMRC_WaiterEnabled;
            goto mod_exit;
        }
        else if (oldState & IEWS_WAITERSTATUS_DISABLE_PEND)
        {
            // If disable is pending, then add IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND
            // this will cause us to fire the callback but change state
            // to ENABLED rather than DISABLED.
            doneEnable = iews_bool_changeState ( pWaiterStatus
                                               , oldState
                                               , (oldState | IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND));
            if (doneEnable)
            {
                rc = ISMRC_DisableWaiterCancel;
            }
        }
        else if (oldState & IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)
        {
            // Just loop around and wait
            ismEngine_FullMemoryBarrier();
        }
        else
        {
            rc = ISMRC_WaiterInvalid;
            ism_common_setError(rc);
            goto mod_exit;
        }
    }
    while(!doneEnable);


    if ( ((enableOptions & IEQ_ENABLE_OPTION_DELIVER_LATER) == 0)
         && ((oldState == IEWS_WAITERSTATUS_DISABLED) ||
             (oldState & IEWS_WAITERSTATUSMASK_ACTIVE)))
    {
        rc = ieq_checkWaiters(pThreadData, Qhdl, NULL, NULL);
    }
mod_exit:
    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d old=%u\n", __func__, rc, (uint32_t)oldState);

    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Disable delivery of messages to the consumer on the queue
///  @remarks
///    Suspend the delivery of messages to the consumer.  As part of this
///    function call the consumer will receive a callback indcating that
///    delivery of messages to the consumer has been suspended.
///
///  @param[in] Qhdl               - Queue
///  @return                       - OK on success
///                                - ISMRC_WaiterDisabled if already disabled
///                                  ISMRC_AsyncCompletion if waiter in use and we've marked it to be disabled
///                                  or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ieq_disableWaiter( ieutThreadData_t *pThreadData,
                           ismQHandle_t Qhdl,
                           ismEngine_Consumer_t *pConsumer )
{
    int32_t rc = OK;

    ieutTRACEL(pThreadData, Qhdl,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " Qhdl=%p\n", __func__, Qhdl);
    bool waiterInUse = false;
    bool doneDisable = false;
    iewsWaiterStatus_t oldState;
    iewsWaiterStatus_t newState = IEWS_WAITERSTATUS_DISCONNECTED;

    volatile iewsWaiterStatus_t *pWaiterStatus = iews_getWaiterStatusPtr(Qhdl, pConsumer);

    do
    {
        oldState = *pWaiterStatus;

        if ((oldState == IEWS_WAITERSTATUS_DISCONNECTED) ||
            ((oldState & IEWS_WAITERSTATUS_DISCONNECT_PEND) != 0))
        {
            rc = ISMRC_WaiterInvalid; //You can't disable the waiter if it's already disconnect(ing)
            goto mod_exit;
        }
        else if(  (oldState == IEWS_WAITERSTATUS_GETTING)
                ||(oldState == IEWS_WAITERSTATUS_DELIVERING))
        {
            newState = IEWS_WAITERSTATUS_DISABLE_PEND;
            waiterInUse = true; // Someone else owns the lock

            doneDisable = iews_bool_changeState( pWaiterStatus
                                               , oldState
                                               , newState);
        }
        else if (oldState == IEWS_WAITERSTATUS_DISABLED)
        {
            newState = IEWS_WAITERSTATUS_DISABLED; //we'll try and leave it unchanged to prove it's disabled
            waiterInUse = false; // We own the lock

            doneDisable = iews_bool_changeState( pWaiterStatus
                                               , oldState
                                               , newState);
        }
        else if (oldState & IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND)
        {
            //cancel_disable_pend implies an earlier disable was cancelled... this is getting very confusing. But we can't just
            //wait in case someone further up the stack of this thread owns this waiter... we'll remove the cancel and
            //fire the notification that the original disable "completed" (so the callback that the completeWaiterActions
            //does will correspond to *this* disable and we'll be balanced
            ieutTRACEL(pThreadData, rc,  ENGINE_UNUSUAL_TRACE,
                    "pConsumer=%p disabled,enabled&disabled again whilst locked\n", pConsumer);

            newState = oldState & (~IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND);
            waiterInUse = true; // Someone else owns the lock

            doneDisable = iews_bool_changeState( pWaiterStatus
                                               , oldState
                                               , newState);

            if (doneDisable)
            {
                ism_engine_deliverStatus( pThreadData
                                        , pConsumer
                                        , ISMRC_WaiterDisabled);
                releaseConsumerReference(pThreadData, pConsumer, false); //Cancel  the reference from the first enable,
                                                                         //the re-enable will have added another
            }

        }
        else if (oldState & IEWS_WAITERSTATUS_DISABLE_PEND)
        {
            newState = oldState; //we'll try and leave it unchanged to prove it's disabled/disabling
            waiterInUse = true; //The disable is in progress

            doneDisable = iews_bool_changeState( pWaiterStatus
                                               , oldState
                                               , newState);
        }
        else if (oldState & IEWS_WAITERSTATUSMASK_PENDING)
        {
            //There are pending flags (apart from ones we've tested in cases above)
            //We can just add the disable pending and let the current lock owner handle it...
            newState = oldState | IEWS_WAITERSTATUS_DISABLE_PEND;
            waiterInUse = true; //The pending flags mean that someone has it locked

            doneDisable = iews_bool_changeState( pWaiterStatus
                                               , oldState
                                               , newState);
        }
        else if (oldState & IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)
        {
            //In this case we can't tell whether it's disabled or disconnected etc... loop and see
            ismEngine_FullMemoryBarrier();
            continue;
        }
        else
        {
            assert((oldState & ( IEWS_WAITERSTATUS_GETTING |
                                 IEWS_WAITERSTATUS_DELIVERING |
                                 IEWS_WAITERSTATUS_DISABLE_PEND |
                                 IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND)) == 0);
            newState = IEWS_WAITERSTATUS_DISABLE_PEND;
            waiterInUse = false; // We own the lock

            doneDisable = iews_bool_tryLockToState( pWaiterStatus
                                                  , oldState
                                                  , newState);
        }
    } while (!doneDisable);

    // If the waiter was in-use, they'll call the callback when they notice
    // If they weren't, we can call the callback
    if (waiterInUse)
    {
        rc = ISMRC_AsyncCompletion;
    }
    else
    {
        if (oldState == IEWS_WAITERSTATUS_DISABLED)
        {
            //It was already disabled/disabling
            rc = ISMRC_WaiterDisabled;
        }
        else
        {
            ieq_completeWaiterActions( pThreadData
                                     , Qhdl
                                     , pConsumer
                                     , IEQ_COMPLETEWAITERACTION_OPTS_NONE
                                     , true);
        }
    }
mod_exit:
    //NB: The above callback may have destroyed the queue so
    //we cannot use it at this point
    ieutTRACEL(pThreadData,rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d old=%u new=%u\n", __func__,rc, (uint32_t)oldState, (uint32_t)newState);
    return rc;
}

//Whilst WE have got the waiter locked, add a pend flag
void iews_addPendFlagWhileLocked( volatile iewsWaiterStatus_t *pWaiterStatus
                                , iewsWaiterStatus_t pendFlag)
{
    iewsWaiterStatus_t oldState;
    iewsWaiterStatus_t newState = IEWS_WAITERSTATUS_DISCONNECTED;

    bool addedFlag = false;

    do
    {
        oldState = *pWaiterStatus;

        if(  (oldState == IEWS_WAITERSTATUS_GETTING)
           ||(oldState == IEWS_WAITERSTATUS_DELIVERING))
        {
            newState = pendFlag;
        }
        else if (oldState & IEWS_WAITERSTATUSMASK_PENDING)
        {
            //Check there are only pending flags...
            assert((oldState & (~(IEWS_WAITERSTATUSMASK_PENDING))) == 0);

            //Even if the flag is already set, we'll add it again to check its
            //state with a memory barrier
            newState = oldState | pendFlag;
        }
        else
        {
            //We have the waiter locked... there are no other valid states
            ieutTRACE_FFDC( ieutPROBE_001, true
                          , "Unexpected waiterStatus when adding flag.", ISMRC_Error
                          , "WaiterStatus", pWaiterStatus, sizeof(iewsWaiterStatus_t)
                          , NULL );
        }

        addedFlag = iews_bool_changeState( pWaiterStatus
                                         , oldState
                                         , newState);
    }
    while (!addedFlag);
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Lock a waiter, even if it's disabled/disconnected
///  @remarks
///    During message delivery we only want to lock a waiter if it's enabled.
///  This function is NOT the appropriate function (see iews_*_tryLockToState())
///  This function is used (for reclaiming space) when we want to lock even
///  waiters marked disabled/disconnected (or instead leave the reclaim pending
///  flag set.
///
///  @param[in] pWaiterStatus      - ptr to waiterstatus to lock
///  @param[out] pOldStatus        - state we locked it from
///  @param[out] pNewStatus        - state that we locked it into
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
bool iews_tryLockForReclaimSpace( volatile iewsWaiterStatus_t *pWaiterStatus
                                , iewsWaiterStatus_t *pOldStatus
                                , iewsWaiterStatus_t *pNewStatus )
{
    //try and take the lock or add a flag saying some else needs to do the clean
    //if added a flag we're done and goto mod_exit
    bool doneCAS = false;
    bool gotLock = false;

    iewsWaiterStatus_t oldStatus;
    iewsWaiterStatus_t newStatus;

    do
    {
        oldStatus = *pWaiterStatus;
        gotLock = false;

        if (oldStatus == IEWS_WAITERSTATUS_ENABLED)
        {
            //delivering rather than getting. We promise to call checkWaiters
            //so other threads don't wait for the lock
            newStatus = IEWS_WAITERSTATUS_DELIVERING;
            gotLock = true; //if the CAS works we'll have the lock
        }
        else if (    (oldStatus == IEWS_WAITERSTATUS_DISABLED)
                  || (oldStatus == IEWS_WAITERSTATUS_DISCONNECTED) )
        {
            newStatus = IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT;
            gotLock = true; //if the CAS works we'll have the lock
        }
        else if (  (oldStatus == IEWS_WAITERSTATUS_GETTING)
                 ||(oldStatus == IEWS_WAITERSTATUS_DELIVERING) )
        {
            newStatus = IEWS_WAITERSTATUS_RECLAIMSPACE_PEND;
            gotLock   = false; //if the CAS works, we'll have left a message to reclaim space
        }
        else if (oldStatus & IEWS_WAITERSTATUSMASK_PENDING )
        {
            //This includes the case that oldStatus = ReclaimSpace_Pend
            //That's fine, we leave it unchanged and do the CAS to ensure we don't have a stale value
            newStatus = oldStatus | IEWS_WAITERSTATUS_RECLAIMSPACE_PEND;
            gotLock   = false; //if the CAS works, we'll have left a message to reclaim space
        }
        else if (oldStatus == IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)
        {
            //try again.
            ismEngine_FullMemoryBarrier();
            continue;
        }
        else
        {
            ieutTRACE_FFDC( ieutPROBE_001, true
                          , "Unknown waiterStatus when reclaiming Q space.", ISMRC_Error
                          , "WaiterStatus", pWaiterStatus, sizeof(iewsWaiterStatus_t)
                          , NULL );

            // The compiler thinks we're using newStatus uninitialized (even though we know
            // that the FFDC will cause the server to end, so we won't get there).
            // To keep the compiler happy, initialize it to 'DISCONNECTED'.
            newStatus = IEWS_WAITERSTATUS_DISCONNECTED;
        }

        if (gotLock)
        {
            //try and take the lock
            doneCAS = iews_bool_tryLockToState( pWaiterStatus
                                              , oldStatus
                                              , newStatus );
        }
        else
        {
            //try and leave a flag
            doneCAS = iews_bool_changeState( pWaiterStatus
                                           , oldStatus
                                           , newStatus );
        }

    }
    while (!doneCAS);

    *pOldStatus = oldStatus;
    *pNewStatus = newStatus;

    TRACE(ENGINE_NORMAL_TRACE, "%s: %u -> %u\n", __func__,
               (unsigned int)oldStatus, (unsigned int)newStatus );

    return gotLock;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Attempt to lock a waiter even if it's disabled/disconnected
///  @remarks
///    During message delivery we only want to lock a waiter if it's enabled.
///  This function is NOT the appropriate function (see iews_*_tryLockToState())
///  This function is used (by expiry reaper) when we want to lock
///  waiters marked disabled/disconnected.
///
///  This function is NOT used when want to leave a pending flag if can't get lock.
///  (e.g. reclaim space... could common up by taking an optional pending flag
///   that this function can leave).
///
///  NB: If not allowing message delivery, **MUST** NEVER WAIT or lock
//   in a state that forces us to call checkwaiters
///  its called when we have the expiry list locked so checkwaiters might deadlock
///  if it tries to remove the queue from the list
///
///  @param[in] pWaiterStatus      - ptr to waiterstatus to lock
///  @param[out] pOldStatus        - state we locked it from
///  @param[out] pNewStatus        - state that we locked it into
///  @param[in]  allowMsgDelivery  - attempt to lock if waiter is enabled/delivering
///
///  @return                       - True if got lock, else false
///////////////////////////////////////////////////////////////////////////////
bool iews_tryLockForQOperation( volatile iewsWaiterStatus_t *pWaiterStatus
                              , iewsWaiterStatus_t *pOldStatus
                              , iewsWaiterStatus_t *pNewStatus
                              , bool allowMsgDelivery)
{
    //try and take the lock if we
    bool doneCAS = false;
    bool gotLock = false;

    iewsWaiterStatus_t oldStatus;
    iewsWaiterStatus_t newStatus = IEWS_WAITERSTATUS_DISCONNECTED;

    do
    {
        oldStatus = *pWaiterStatus;
        gotLock = false;

        if (allowMsgDelivery)
        {
            if (oldStatus == IEWS_WAITERSTATUS_ENABLED)
            {
                //delivering rather than getting. We promise to call checkWaiters
                //so other threads don't wait for the lock
                newStatus = IEWS_WAITERSTATUS_DELIVERING;
                gotLock = true; //if the CAS works we'll have the lock
            }
            else if (    (oldStatus == IEWS_WAITERSTATUS_DISABLED)
                      || (oldStatus == IEWS_WAITERSTATUS_DISCONNECTED) )
            {
                newStatus = IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT;
                gotLock = true; //if the CAS works we'll have the lock
            }
            else if (  (oldStatus == IEWS_WAITERSTATUS_GETTING)
                     ||(oldStatus == IEWS_WAITERSTATUS_DELIVERING)
                     ||(oldStatus & IEWS_WAITERSTATUSMASK_PENDING ) )
            {
                newStatus = oldStatus;
                gotLock   = false; //if the CAS works, we know waiter was in a state we couldn't lock
            }
            else if (oldStatus == IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)
            {
                //try again.
                ismEngine_FullMemoryBarrier();
                continue;
            }
            else
            {
                ieutTRACE_FFDC( ieutPROBE_001, true
                              , "Unknown waiterStatus when locking for expiry reap.", ISMRC_Error
                              , "WaiterStatus", pWaiterStatus, sizeof(iewsWaiterStatus_t)
                              , NULL );
            }
        }
        else
        {
            if (oldStatus & (   IEWS_WAITERSTATUSMASK_ACTIVE
                              | IEWS_WAITERSTATUSMASK_PENDING
                              | IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT))
            {
                //These are all states we can't have
                newStatus = oldStatus;
                gotLock   = false; //if the CAS works, we know the waiter really was unavailable

            }
            else if (    (oldStatus == IEWS_WAITERSTATUS_DISABLED)
                      || (oldStatus == IEWS_WAITERSTATUS_DISCONNECTED) )
            {
              newStatus = IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT;
              gotLock = true; //if the CAS works we'll have the lock
            }
            else
            {
                ieutTRACE_FFDC( ieutPROBE_001, true
                              , "Unknown waiterStatus when locking for expiry reap.", ISMRC_Error
                              , "WaiterStatus", pWaiterStatus, sizeof(iewsWaiterStatus_t)
                              , NULL );
            }
        }

        if (gotLock)
        {
            //try and take the lock
            doneCAS = iews_bool_tryLockToState( pWaiterStatus
                                              , oldStatus
                                              , newStatus );
        }
        else
        {
            //just check the state is as we expect by leaving it unchanged
            doneCAS = iews_bool_changeState( pWaiterStatus
                                           , oldStatus
                                           , newStatus );
        }

    }
    while (!doneCAS);

    *pOldStatus = oldStatus;
    *pNewStatus = newStatus;

    TRACE(ENGINE_NORMAL_TRACE, "%s: %u -> %u\n", __func__,
               (unsigned int)oldStatus, (unsigned int)newStatus );

    return gotLock;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Lock a waiter even if it's disabled/disconnected or we have to wait for the lock
///  @remarks
///   Wrapper around iews_tryLockForQOperation() which has a lot of caveats so read them if using
///   this function!
///
///  @param[in] pWaiterStatus      - ptr to waiterstatus to lock
///  @param[in] waitTimeNanos      - max nanoseconds to wait for the lock
///  @param[out] pOldStatus        - state we locked it from
///  @param[out] pNewStatus        - state that we locked it into
///  @param[in]  allowMsgDelivery  - attempt to lock if waiter is enabled/delivering
///
///  @return                       - True if got lock, else false
///////////////////////////////////////////////////////////////////////////////
bool iews_getLockForQOperation( ieutThreadData_t *pThreadData
                              , volatile iewsWaiterStatus_t *pWaiterStatus
                              , uint64_t waitTimeNanos
                              , iewsWaiterStatus_t *pPreLockedState
                              , iewsWaiterStatus_t *pLockedState
                              , bool allowMsgDelivery)
{
    //try to lock the consumer once before we worry about time stamps etc...
    *pPreLockedState = *pWaiterStatus;
    *pLockedState = IEWS_WAITERSTATUS_DISCONNECTED;

    bool lockedConsumer = false;

    if ((*pPreLockedState & IEWS_WAITERSTATUSMASK_LOCKED) == 0)
    {
        lockedConsumer = iews_tryLockForQOperation( pWaiterStatus
                                                  , pPreLockedState
                                                  , pLockedState
                                                  , allowMsgDelivery);
    }

    //Initial lock didn't work... loop around trying to lock for a while
    if (!lockedConsumer)
    {
        uint64_t starttime = ism_common_currentTimeNanos();
        uint64_t finishtime = 0;
        do
        {
            lockedConsumer = iews_tryLockForQOperation( pWaiterStatus
                                                      , pPreLockedState
                                                      , pLockedState
                                                      , allowMsgDelivery);
            finishtime = ism_common_currentTimeNanos();

            if (finishtime < starttime)
            {
                //clock changed, reset start time
                starttime = finishtime;
            }
        }
        while (   !lockedConsumer
               && ((finishtime - starttime) < waitTimeNanos));
    }

    return lockedConsumer;
}
///////////////////////////////////////////////////////////////////////////////
///  @brief
///    unlock a waiter locked for reclaim space, export or expiry
///  @remarks
///    Action any pending flags applied by other threads if we locked an enabled
///  waiter
///
///  @param[in] Qhdl               - Queue
///  @param[in] pConsumer          - pConsumer corresponding to waiterstatus
///  @param[in] pWaiterStatus      - ptr to waiterstatus to lock
///  @param[in] lockedState        - state that we previously locked it into
///  @param[in] preLockedState     - state we previously locked it from
///
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
void iews_unlockAfterQOperation( ieutThreadData_t *pThreadData
                               , ismQHandle_t Q
                               , ismEngine_Consumer_t *pConsumer
                               , volatile iewsWaiterStatus_t *pWaiterStatus
                               , iewsWaiterStatus_t lockedState
                               , iewsWaiterStatus_t preLockedState)
{
    if (lockedState == IEWS_WAITERSTATUS_DELIVERING)
    {
        //try and unlock...
        bool doneCAS = iews_bool_tryUnlockNoPending( pWaiterStatus
                                                   , IEWS_WAITERSTATUS_DELIVERING
                                                   , preLockedState );

        if (!doneCAS)
        {
            //We took a lock in a state that allowed other threads to give us work to
            //do and they have (otherwise our CAS would have work... do whatever they wanted
            ieq_completeWaiterActions( pThreadData
                                     , Q
                                     , pConsumer
                                     , IEQ_COMPLETEWAITERACTION_OPT_NODELIVER //Calling checkWaiters in a sec...
                                     , true);
        }

        //We locked it in delivering state and that promises to call checkWaiters
        //...honour the promise
        ieq_checkWaiters(pThreadData, Q, NULL, NULL);
    }
    else
    {
        assert(lockedState == IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT);

        //try and unlock...
        bool doneCAS = iews_bool_tryUnlockNoPending( pWaiterStatus
                                                   , IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT
                                                   , preLockedState );

        if (!doneCAS)
        {
            //No-one should be changing the state whilst we have it in this state
            ieutTRACE_FFDC( ieutPROBE_001, true
                          , "Unexpected waiterStatus change when reclaiming Q space.", ISMRC_Error
                          , "Queue", Q, sizeof(ismEngine_Queue_t)
                          , NULL );
        }
    }
}


void ieq_completeWaiterActions(ieutThreadData_t *pThreadData,
                               ismEngine_Queue_t *Q,
                               ismEngine_Consumer_t *pConsumer,
                               uint32_t completeActionOptions,
                               bool waiterWasEnabled)
{
    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_ENTRY " Q=%p pConsumer=%p\n", __func__, Q, pConsumer);
    bool stateChanged = false;

    volatile iewsWaiterStatus_t *pWaiterStatus = iews_getWaiterStatusPtr(Q, pConsumer);

    uint64_t oldStatus = *pWaiterStatus;
    uint64_t newStatus;

    bool doReleaseConsumerReference = false;
    bool loopAgain;
    bool allowDelivery = !(completeActionOptions & IEQ_COMPLETEWAITERACTION_OPT_NODELIVER);

    //First see if we need to reclaim space but not do anything more
    //"final" like disconnect/disable
    do
    {
        oldStatus = *pWaiterStatus;
        //Check there is pending work to do and no other unexpected flags
        assert(    ((oldStatus & IEWS_WAITERSTATUSMASK_PENDING) != 0)
                && ((oldStatus & ~(IEWS_WAITERSTATUSMASK_PENDING)) == 0));

        loopAgain = false;

        if (oldStatus == IEWS_WAITERSTATUS_RECLAIMSPACE_PEND)
        {
            //Remove the flag
            bool removedflag = iews_bool_changeState( pWaiterStatus
                                                    , IEWS_WAITERSTATUS_RECLAIMSPACE_PEND
                                                    , IEWS_WAITERSTATUS_DELIVERING);
            if (removedflag)
            {
                ieq_reclaimSpace(pThreadData, Q, false);

                stateChanged = iews_bool_tryUnlockNoPending( pWaiterStatus
                                                           , IEWS_WAITERSTATUS_DELIVERING
                                                           , IEWS_WAITERSTATUS_ENABLED );
                if (stateChanged)
                {
                    //We've re-enabled a waiter that was in delivering: deliver any messages
                    if (allowDelivery)
                    {
                        ieq_checkWaiters(pThreadData, Q, NULL, NULL);
                    }
                }
                else
                {
                    loopAgain=true;
                }
            }
            else
            {
                loopAgain=true;
            }
        }
    }
    while (loopAgain);

    if (!stateChanged)
    {
        //If we haven't managed to unlock the waiter we have disabling or disconnecting so say that delivery is stopping...
        if (oldStatus & (IEWS_WAITERSTATUS_DISABLE_PEND|IEWS_WAITERSTATUS_DISCONNECT_PEND))
        {
            if (waiterWasEnabled)
            {
                ism_engine_deliverStatus( pThreadData,
                                          pConsumer,
                                          ISMRC_WaiterDisabled);

                //Disabling the waiter reduces the refcount by 1...but we need to wait until we stop altering the waiterStatus.
                doReleaseConsumerReference = true;
            }

            if (oldStatus & IEWS_WAITERSTATUS_DISABLE_PEND)
            {
                bool doneDisable = false;

                do
                {
                    oldStatus = *pWaiterStatus;
                    assert((oldStatus & ~(IEWS_WAITERSTATUSMASK_PENDING)) == 0);

                    if (oldStatus & IEWS_WAITERSTATUS_RECLAIMSPACE_PEND)
                    {
                        //Remove the flag
                        bool removedflag = iews_bool_changeState( pWaiterStatus
                                                                , oldStatus
                                                                , oldStatus & (~IEWS_WAITERSTATUS_RECLAIMSPACE_PEND));
                        if (removedflag)
                        {
                            ieq_reclaimSpace(pThreadData, Q, false);
                        }

                        //Take another look at waiterStatus
                        continue;
                    }

                    if (oldStatus & IEWS_WAITERSTATUS_DISCONNECT_PEND)
                    {
                        newStatus = IEWS_WAITERSTATUS_DISCONNECT_PEND;
                        doneDisable = iews_bool_changeState( pWaiterStatus
                                                           , oldStatus
                                                           , newStatus)
                    }
                    else if (oldStatus & IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND)
                    {
                        newStatus = IEWS_WAITERSTATUS_ENABLED;

                        doneDisable = iews_bool_Unlock( pWaiterStatus
                                                      , oldStatus
                                                      , newStatus);

                        //We don't reset doReleaseConsumerReference to false as whoever cancelled the disabled
                        //will have increased the reference count and we are acting like the disable momentarily worked
                        //(e.g. we have fired the disable callback) so we shold analogously do the release a successful
                        //disable would have done

                        //We have enabled a waiter that was in *_PEND/DELIVERING
                        if (doneDisable && allowDelivery)
                        {
                           ieq_checkWaiters(pThreadData, Q, NULL, NULL);
                        }
                    }
                    else
                    {
                        newStatus = IEWS_WAITERSTATUS_DISABLED;
                        doneDisable = iews_bool_Unlock( pWaiterStatus
                                                      , oldStatus
                                                      , newStatus);
                    }
                }
                while (!doneDisable);

                oldStatus = newStatus;
            }
        }

        if (oldStatus & IEWS_WAITERSTATUS_DISCONNECT_PEND)
        {
            // Note: IEWS_WAITERSTATUS_DISCONNECTED is the only state where
            //       we release the lock before delivering the status.

            //The queue no longer has a right to refer to this consumer
            if (Q->QType == simple)
            {
                iesqQueue_t *simpQ = (iesqQueue_t *)Q;

                simpQ->putsAttempted = simpQ->qavoidCount  +
                                       simpQ->enqueueCount +
                                       simpQ->rejectedMsgs;
                simpQ->pConsumer = NULL;
            }
            else if (Q->QType == intermediate)
            {
                ieiqQueue_t *intermediateQ = (ieiqQueue_t *)Q;

                intermediateQ->putsAttempted = intermediateQ->qavoidCount  +
                                               intermediateQ->enqueueCount +
                                               intermediateQ->rejectedMsgs;
                intermediateQ->pConsumer = NULL;
            }

            bool finishedDisconnect = false;

            do
            {
                oldStatus = *pWaiterStatus;

                if (oldStatus & IEWS_WAITERSTATUS_RECLAIMSPACE_PEND)
                {
                    //Remove the flag
                    bool removedflag = iews_bool_changeState( pWaiterStatus
                                                            , oldStatus
                                                            , oldStatus & (~IEWS_WAITERSTATUS_RECLAIMSPACE_PEND));
                    if (removedflag)
                    {
                        ieq_reclaimSpace(pThreadData, Q, false);
                    }

                    //Take another look at waiterStatus
                    continue;
                }

                finishedDisconnect = iews_bool_Unlock( pWaiterStatus
                                                     , IEWS_WAITERSTATUS_DISCONNECT_PEND
                                                     , IEWS_WAITERSTATUS_DISCONNECTED );
            }
            while (!finishedDisconnect);

            ism_engine_deliverStatus( pThreadData,
                                      pConsumer,
                                      ISMRC_WaiterRemoved);

            //Delete the queue if this consumer was blocking it....
            ieq_reducePreDeleteCount(pThreadData, Q);
        }


        if (doReleaseConsumerReference)
        {
            releaseConsumerReference(pThreadData, pConsumer, false);
        }
    }

    ieutTRACEL(pThreadData, Q,  ENGINE_FNC_TRACE, FUNCTION_EXIT "Q=%p\n", __func__, Q);
}


void ism_engine_suspendMessageDelivery(ismEngine_Consumer_t *pConsumer,
                                       uint32_t options)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pConsumer->pSession->pClient);

    ieutTRACEL(pThreadData, pConsumer,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "hConsumer %p\n", __func__, pConsumer);

    volatile iewsWaiterStatus_t *pWaiterStatus = iews_getWaiterStatusPtr(pConsumer->queueHandle, pConsumer);

    iews_addPendFlagWhileLocked( pWaiterStatus
                               , IEWS_WAITERSTATUS_DISABLE_PEND);

    ieutTRACEL(pThreadData, pConsumer, ENGINE_CEI_TRACE, FUNCTION_EXIT "\n", __func__);
    ieut_leavingEngine(pThreadData);
    return;
}


//****************************************************************************
/// @brief  Get Consumer Message Delivery Status
///
/// Gets the message delivery status for an individual consumer.
///
/// @param[in]     hConsumer        Consumer handle
/// @param[out]    pStatus          Message delivery status for this consumer
///
/// @return Nothing
//****************************************************************************
XAPI void ism_engine_getConsumerMessageDeliveryStatus(
    ismEngine_ConsumerHandle_t      hConsumer,
    ismMessageDeliveryStatus_t *    pStatus)
{
    ismEngine_Consumer_t *pConsumer = (ismEngine_Consumer_t *)hConsumer;
    ieutThreadData_t *pThreadData = ieut_enteringEngine(pConsumer->pSession->pClient);

    ieutTRACEL(pThreadData, pConsumer,  ENGINE_CEI_TRACE, FUNCTION_ENTRY "(hConsumer %p)\n", __func__, pConsumer);

    assert(pConsumer != NULL);
    ismEngine_CheckStructId(pConsumer->StrucId, ismENGINE_CONSUMER_STRUCID, ieutPROBE_001);

    ismEngine_ReadMemoryBarrier();

    assert(pConsumer->queueHandle != NULL); //Should not be called when consumer is being inited or destroyed

    iewsWaiterStatus_t waiterStatus = *(iews_getWaiterStatusPtr(pConsumer->queueHandle, pConsumer));


    if (waiterStatus & ( IEWS_WAITERSTATUS_DISCONNECTED
                       | IEWS_WAITERSTATUS_DISABLED
                       | IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT))
    {
        *pStatus = ismMESSAGE_DELIVERY_STATUS_STOPPED;
    }
    else if (   (waiterStatus & (   IEWS_WAITERSTATUS_DISABLE_PEND
                                  | IEWS_WAITERSTATUS_DISCONNECT_PEND  ))
             && ((waiterStatus & IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND) == 0) )
    {
        *pStatus = ismMESSAGE_DELIVERY_STATUS_STOPPING;
    }
    else
    {
        *pStatus = ismMESSAGE_DELIVERY_STATUS_STARTED;
    }

    ieutTRACEL(pThreadData, *pStatus,  ENGINE_CEI_TRACE, FUNCTION_EXIT "status=%d\n", __func__, *pStatus);
    ieut_leavingEngine(pThreadData);
    return;
}
