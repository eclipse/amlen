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
/// @file  waiterStatus.h
/// @brief Defines functions related to starting stopping an individual waiter
//****************************************************************************
#ifndef __ISM_ENGINE_WAITERSTATUS_DEFINED
#define __ISM_ENGINE_WAITERSTATUS_DEFINED

#include "engineInternal.h"
#include "queueCommon.h"
#include "messageDelivery.h"

// The following states are defined as bits however they are all mutually
// exclusive (except for the *_CB states) in that a waiter can only be on
// one state. Each state is defined as a bit to allow fast comparison of
// a set of states.

//The iewsWaiterStatus_t type and the initial (disconnected) state are usually
//defined in engineInternal.h.... but optionally have them here so this can
//be used stand-alone
#ifndef IEWS_WAITERSTATUS_DISCONNECTED
typedef uint64_t iewsWaiterStatus_t;
//All the following states are mutually exclusive unless otherwise specified:
//First some states were the waiter is not considered locked:
#define IEWS_WAITERSTATUS_DISCONNECTED     0
#endif

#define IEWS_WAITERSTATUS_DISABLED                0x0000000000000001    // Waiter is disabled
#define IEWS_WAITERSTATUS_ENABLED                 0x0000000000000002    // Waiter is enabled
#define IEWS_WAITERSTATUS_GETTING                 0x0000000000000004    // Searching for msg for waiter (no promise to call checkWaiters)
#define IEWS_WAITERSTATUS_DELIVERING              0x0000000000000008    // Delivering msg to waiter (promise to call checkwaiters after this)

#define IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT     0x0000000001000000    //Want to prevent waiter changes whilst waiter is disabled/disconnected
                                                                        //caller should retry in a moment

//The following three states are NOT mutually exclusive and can co-exist with each other:
#define IEWS_WAITERSTATUS_DISABLE_PEND            0x0000000000000010    // Disable is pending
#define IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND     0x0000000000000020    // A pending disable has been cancelled before being completed
#define IEWS_WAITERSTATUS_DISCONNECT_PEND         0x0000000000000040    // Disconnect is pending
#define IEWS_WAITERSTATUS_RECLAIMSPACE_PEND       0x0000000000000080    // We have a pending need to discard old messages

// The following values are used for fast comparison of a number of states

// IEWS_WAITERSTATUSMASK_ACTIVE - not disabled or pending disable
// NB: Use this mask with care. It doesn't include more complicated cases like
// disable_pend+enable_pend (an inprogress disable has been cancelled)
// or
// reclaim_pend +disable_pend (unless you check that no bits outside the mask are set)
#define IEWS_WAITERSTATUSMASK_ACTIVE       ( IEWS_WAITERSTATUS_ENABLED    \
                                           | IEWS_WAITERSTATUS_GETTING    \
                                           | IEWS_WAITERSTATUS_DELIVERING \
                                           | IEWS_WAITERSTATUS_RECLAIMSPACE_PEND )  //Other _PEND states will cause disable pending

// IEWS_WAITERSTATUSMASK_PENDING - Pending states which may be concurrently set
#define IEWS_WAITERSTATUSMASK_PENDING      ( IEWS_WAITERSTATUS_DISABLE_PEND        \
                                           | IEWS_WAITERSTATUS_CANCEL_DISABLE_PEND \
                                           | IEWS_WAITERSTATUS_DISCONNECT_PEND     \
                                           | IEWS_WAITERSTATUS_RECLAIMSPACE_PEND   )

// Threads without the lock may (if the state doesn't end in WAIT) transistion to a state in the IEMQ_WAITERSTATUSMASK_PENDING
// If the state is a WAIT state, they need to loop around waiting for the state to change if they need the lock or to signal something
#define IEWS_WAITERSTATUSMASK_LOCKED       ( IEWS_WAITERSTATUS_GETTING              \
                                           | IEWS_WAITERSTATUS_DELIVERING           \
                                           | IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT  \
                                           | IEWS_WAITERSTATUSMASK_PENDING          )


int32_t ieq_enableWaiter( ieutThreadData_t *pThreadData,
                          ismQHandle_t Qhdl,
                          ismEngine_Consumer_t *pConsumer,
                          uint32_t enableOptions);

#define IEQ_ENABLE_OPTION_NONE            0x0
#define IEQ_ENABLE_OPTION_DELIVER_LATER   0x1 //Caller promises to call checkWaiters afterwards in order to
                                               //actually deliver messages to newly enabled waiter

int32_t ieq_disableWaiter( ieutThreadData_t *pThreadData,
                           ismQHandle_t Qhdl,
                           ismEngine_Consumer_t *pConsumer );

//Functions/macros that switch the waiterstatus between states

//Macros that check the waiterstatus is being locked/unlocked in expected operations
#define iews_checkLockOperation(fromState, toState)  assert(   ((fromState & IEWS_WAITERSTATUSMASK_LOCKED) == 0)       \
                                                            && ((toState   & IEWS_WAITERSTATUSMASK_LOCKED) != 0))

#define iews_checkUnlockOperation(fromState, toState)  assert(   ((fromState & IEWS_WAITERSTATUSMASK_LOCKED) != 0)     \
                                                              && ((toState   & IEWS_WAITERSTATUSMASK_LOCKED) == 0))

#define iews_checkLockinessUnchanged(fromState, toState)    assert(   (((fromState & IEWS_WAITERSTATUSMASK_LOCKED) != 0) && ((toState & IEWS_WAITERSTATUSMASK_LOCKED) != 0))      \
                                                                    ||(((fromState & IEWS_WAITERSTATUSMASK_LOCKED) == 0) && ((toState & IEWS_WAITERSTATUSMASK_LOCKED) == 0)))

#define iews_checkUnlockingWithNoPending(fromState, toState)  assert(   (   (fromState == IEWS_WAITERSTATUS_GETTING)              \
                                                                         || (fromState == IEWS_WAITERSTATUS_DELIVERING)           \
                                                                         || (fromState == IEWS_WAITERSTATUS_DISABLED_LOCKEDWAIT)) \
                                                                     && ((toState   & IEWS_WAITERSTATUSMASK_LOCKED) == 0))

//LOCK Functions and macros

//Three ways to lock the waiter. Normally we only want to lock the waiter in certain states and we use one of the first two
//(iews_val_tryLockToState or iews_bool_tryLockToState) depending on what you want returned
//(If you want it locked even if disabled/disconnected
#define iews_val_tryLockToState(PtrVariable, fromState, toState)       __sync_val_compare_and_swap(PtrVariable, fromState, toState);     \
                                                                       iews_checkLockOperation(fromState, toState);

#define iews_bool_tryLockToState(PtrVariable, fromState, toState)       __sync_bool_compare_and_swap(PtrVariable, fromState, toState);   \
                                                                        iews_checkLockOperation(fromState, toState)

//Lock specifically for reclaim (it leaves reclaim pending flag if can't get lock).
bool iews_tryLockForReclaimSpace( volatile iewsWaiterStatus_t *pWaiterStatus
                                , iewsWaiterStatus_t *pOldStatus
                                , iewsWaiterStatus_t *pNewStatus );

//Lock for expiry reaping
bool iews_tryLockForQOperation( volatile iewsWaiterStatus_t *pWaiterStatus
                              , iewsWaiterStatus_t *pOldStatus
                              , iewsWaiterStatus_t *pNewStatus
                              , bool allowMsgDelivery);


bool iews_getLockForQOperation( ieutThreadData_t *pThreadData
                              , volatile iewsWaiterStatus_t *pWaiterStatus
                              , uint64_t waitTimeNanos
                              , iewsWaiterStatus_t *pPreLockedState
                              , iewsWaiterStatus_t *pLockedState
                              , bool allowMsgDelivery);

//Change state from one locked state to other or from an unlocked state to another
#define iews_val_changeState(PtrVariable, fromState, toState)        __sync_bool_compare_and_swap(PtrVariable, fromState, toState);   \
                                                                     iews_checkLockinessUnchanged(fromState, toState);

#define iews_bool_changeState(PtrVariable, fromState, toState)       __sync_bool_compare_and_swap(PtrVariable, fromState, toState);   \
                                                                     iews_checkLockinessUnchanged(fromState, toState);

//Call to unlock in the simple case where no pending flags have been left (if it was locked in delivering the CALLER must still call checkWaiters after this)
#define iews_val_tryUnlockNoPending(PtrVariable, fromState, toState)        __sync_val_compare_and_swap(PtrVariable, fromState, toState);      \
                                                                            iews_checkUnlockingWithNoPending(fromState, toState);

#define iews_bool_tryUnlockNoPending(PtrVariable, fromState, toState)       __sync_bool_compare_and_swap(PtrVariable, fromState, toState);      \
                                                                            iews_checkUnlockingWithNoPending(fromState, toState);

//unlock specifically for reclaim space and expiry reaping
void iews_unlockAfterQOperation( ieutThreadData_t *pThreadData
                                   , ismQHandle_t Q
                                   , ismEngine_Consumer_t *pConsumer
                                   , volatile iewsWaiterStatus_t *pWaiterStatus
                                   , iewsWaiterStatus_t lockedState
                                   , iewsWaiterStatus_t preLockedState);

//Call to unlock when other threads have left requests for work
void ieq_completeWaiterActions(ieutThreadData_t *pThreadData,
                               ismEngine_Queue_t *Q,
                               ismEngine_Consumer_t *pConsumer,
                               uint32_t completeActionOptions,
                               bool waiterWasEnabled);

void iews_addPendFlagWhileLocked( volatile iewsWaiterStatus_t *pWaiterStatus
                                , iewsWaiterStatus_t pendFlag);

#define IEQ_COMPLETEWAITERACTION_OPTS_NONE       0x0
//If the no deliver option is set, the caller MUST call ieq_checkwaiters() (or some equivalent mechanism
//
#define IEQ_COMPLETEWAITERACTION_OPT_NODELIVER   0x1

#endif //__ISM_ENGINE_WAITERSTATUS_DEFINED
