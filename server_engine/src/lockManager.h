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
/// @file  lockManager.h
/// @brief Engine component lock manager header file
//*********************************************************************
#ifndef __ISM_LOCKMANAGER_DEFINED
#define __ISM_LOCKMANAGER_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include "engineCommon.h"     /* Engine common internal header file  */
#include "engineInternal.h"   /* Engine internal header file         */


/*********************************************************************/
/*                                                                   */
/* TYPE DEFINITIONS                                                  */
/*                                                                   */
/*********************************************************************/

//*******************************************************************
/// @brief Lock name - unique for all lockable objects in the lock manager
//*******************************************************************
typedef union ielmLockName_t
{
    struct
    {
        uint32_t LockType;
    } Common;
    struct
    {
        uint32_t LockType;
        uint32_t QId;
        uint64_t NodeId;
    } Msg;
} ielmLockName_t;

#define ielmLOCK_TYPE_MESSAGE 0

#define ielmNUM_LOCK_TYPES 1                   ///< The number of different lock types


/*******************************************************************/
/* Locking modes                                                   */
/*******************************************************************/
#define ielmLOCK_MODE_NONE 0
#define ielmLOCK_MODE_IS   1
#define ielmLOCK_MODE_IX   2
#define ielmLOCK_MODE_S    3
#define ielmLOCK_MODE_SIX  4
#define ielmLOCK_MODE_X    5

/*******************************************************************/
/* The number of different lock modes                              */
/*******************************************************************/
#define ielmNUM_LOCK_MODES 6

/*******************************************************************/
/* Lock duration - may not need all of these in practice           */
/*******************************************************************/
#define ielmLOCK_DURATION_NONE          0x00000000
#define ielmLOCK_DURATION_REQUEST       0x00000001
#define ielmLOCK_DURATION_COMMIT        0x00000002
#define ielmLOCK_DURATION_MASK          0x00000003

/*******************************************************************/
/* The number of different lock types - object, page and message   */
/*******************************************************************/
#define ielmNUM_LOCK_TYPES 1

/*******************************************************************/
/* The number of different lock modes - and a special one which    */
/* signifies a conflict                                            */
/*******************************************************************/
#define ielmLOCK_MODE_CLASH ielmNUM_LOCK_MODES

/*******************************************************************/
/* The capabilities of a lock scope - can it do commit-duration?   */
/*******************************************************************/
#define ielmLOCK_SCOPE_NO_COMMIT        0x0000
#define ielmLOCK_SCOPE_COMMIT_CAPABLE   0x0001


typedef void (*ielm_InstantLockCallback_t) (void *context);


/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/

/*******************************************************************/
/* ielm_createLockManager - Creates the lock manager               */
/*******************************************************************/
int32_t ielm_createLockManager(ieutThreadData_t *pThreadData);

/*******************************************************************/
/* ielm_destroyLockManager - Destroys the lock manager             */
/*******************************************************************/
void ielm_destroyLockManager(ieutThreadData_t *pThreadData);

/*******************************************************************/
/* ielm_allocateLockScope - Allocates a lock scope                 */
/*******************************************************************/
int32_t ielm_allocateLockScope(ieutThreadData_t *pThreadData,
                               uint32_t ScopeOptions,
                               iempMemPoolHandle_t hMemPool,
                               ielmLockScopeHandle_t *phLockScope);

/*******************************************************************/
/* ielm_allocateCachedLockRequest - Allocates a lock request       */
/* structure. Can be used when later request will need to be made  */
/* but no mallocs are allowed (during commit)                      */
/*******************************************************************/
int32_t ielm_allocateCachedLockRequest(ieutThreadData_t *pThreadData,
                                       ielmLockRequestHandle_t *phLockRequest);

/*******************************************************************/
/* ielm_freeLockScope - Frees a lock scope                         */
/*******************************************************************/
void ielm_freeLockScope(ieutThreadData_t *pThreadData,
                        ielmLockScopeHandle_t *phLockScope);

/*******************************************************************/
/* ielm_freeLockRequest - Frees a lock request for a lock which    */
/* has already been released, but not freed.                       */
/*******************************************************************/
void ielm_freeLockRequest(ieutThreadData_t *pThreadData,
                          ielmLockRequestHandle_t hLockRequest);

/*******************************************************************/
/* ielm_takeLock - Takes a lock which is not contested             */
/*******************************************************************/
int32_t ielm_takeLock(ieutThreadData_t *pThreadData,
                      ielmLockScopeHandle_t hLockScope,
                      ielmLockRequestHandle_t hPreallocLockRequest,
                      ielmLockName_t *pLockName,
                      uint32_t LockMode,
                      uint32_t LockDuration,
                      ielmLockRequestHandle_t *phLockRequest);

/*******************************************************************/
/* ielm_instantLockWithPeek - Requests an exclusive lock for an    */
/* instant and optional peeks at a memory address as it does so.   */
/* This does not respect atomic release of commit-duration locks.  */
/*******************************************************************/
int32_t ielm_instantLockWithPeek(ielmLockName_t *pLockName,
                                 ismMessageState_t *pPeekData,
                                 ismMessageState_t *pPeekValueOut);

/*******************************************************************/
/* ielm_instantLockWithCallback - Requests an exclusive lock for   */
/* an instant calls a callback whilst the lock is held.            */
/* NB: If fPauseForCommit=False then this does not respect atomic  */
/* release of commit-duration locks.                               */
/*                                                                 */
/* NB: Callback should be /very/ short running. A chain of locks   */
/* (i.e. multiple locks) are paused whilst the callback is run     */
/*******************************************************************/
int32_t ielm_instantLockWithCallback( ieutThreadData_t *pThreadData
                                    , ielmLockName_t *pLockName
                                    , bool fPauseForCommit
                                    , ielm_InstantLockCallback_t pCallback
                                    , void *pCallbackContext);

/*******************************************************************/
/* ielm_requestLock - Requests a lock                              */
/*******************************************************************/
int32_t ielm_requestLock(ieutThreadData_t *pThreadData,
                         ielmLockScopeHandle_t hLockScope,
                         ielmLockName_t *pLockName,
                         uint32_t LockMode,
                         uint32_t LockDuration,
                         ielmLockRequestHandle_t *phLockRequest);

/*******************************************************************/
/* ielm_releaseLock - Releases a lock                              */
/* This function is only permitted for release of a                */
/* request-duration lock. To release a commit-duration lock, you   */
/* must use ielm_releaseAllLocksBegin/Complete.                    */
/*******************************************************************/
void ielm_releaseLock(ieutThreadData_t *pThreadData,
                      ielmLockScopeHandle_t hLockScope,
                      ielmLockRequestHandle_t hLockRequest);

/*******************************************************************/
/* ielm_releaseLockNoFree - Releases a lock but does not free it   */
/* This function is only permitted for release of a                */
/* request-duration lock. The memory for the lock request is not   */
/* freed and is no longer associated with the scope. The memory    */
/* must be used to take a lock using ielm_takeLock, or freed       */
/* explicitly using ielm_freeLockRequest.                          */
/*******************************************************************/
void ielm_releaseLockNoFree(ielmLockScopeHandle_t hLockScope,
                            ielmLockRequestHandle_t hLockRequest);

/*******************************************************************/
/* ielm_releaseAllLocksBegin                                       */
/*  - Begins release of all locks in scope                         */
/* This function begins the release of all locks in a scope. If    */
/* there are commit-duration locks, this causes future requests of */
/* these locks to be delayed until the release is completed,       */
/* rather than being bounced directly.                             */
/*******************************************************************/
void ielm_releaseAllLocksBegin(ielmLockScopeHandle_t hLockScope);

/*******************************************************************/
/* ielm_releaseAllLocksComplete                                    */
/*  - Completes release of all locks in scope                      */
/* This function completes release of all locks in a scope. Any    */
/* requests delayed until the completion of the release are allowed*/
/* to proceed at the end of this processing.                       */
/*******************************************************************/
void ielm_releaseAllLocksComplete(ieutThreadData_t *pThreadData,
                                  ielmLockScopeHandle_t hLockScope);

/*******************************************************************/
/* ielm_releaseManyLocksBegin                                      */
/*  - Begins the release of many locks in a scope                  */
/* This function begins the release of many locks in a scope.      */
/* It locks the scope in preparation for a sequence of calls to    */
/* ielm_releaseOneOfManyLocks followed by a final call to          */
/* ielm_releaseManyLocksComplete.                                  */
/*******************************************************************/
int32_t ielm_releaseManyLocksBegin(ielmLockScopeHandle_t hLockScope);

/*******************************************************************/
/* ielm_releaseOneOfManyLocks                                      */
/*  - Releases one lock in a sequence of many releases             */
/* This function is only permitted for release of a                */
/* request-duration lock. The function is only permitted between   */
/* calls to ielm_releaseManyLocksBegin/Complete.                   */
/*******************************************************************/
void ielm_releaseOneOfManyLocks(ieutThreadData_t *pThreadData,
                                ielmLockScopeHandle_t hLockScope,
                                ielmLockRequestHandle_t hLockRequest);

/*******************************************************************/
/* ielm_releaseManyLocksComplete                                   */
/*  - Completes release of many locks in a scope                   */
/* This function completes release of many locks in a scope. It    */
/* unlocks the scope at the end of a sequence of calls which       */
/* began with ielm_releaseManyLocksBegin.                          */
/*******************************************************************/
int32_t ielm_releaseManyLocksComplete(ielmLockScopeHandle_t hLockScope);

/*******************************************************************/
/* ielm_dumpWriteDescriptions                                      */
/*   - Write dump description for lock manager structures          */
/*******************************************************************/
void ielm_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl);

/*******************************************************************/
/* ielm_dumpLocks                                                  */
/*  - Dump information about a locks to a file                     */
/*******************************************************************/
int32_t ielm_dumpLocks(const char *lockName, iedmDumpHandle_t dumpHdl);

#endif /* __ISM_LOCKMANAGER_DEFINED */
