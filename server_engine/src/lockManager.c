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
/// @file  lockManager.c
/// @brief Engine component lock manager
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "lockManager.h"
#include "lockManagerInternal.h"
#include "memHandler.h"
#include "mempool.h"
#include "engineDump.h"           // iedm functions & constants

/*********************************************************************/
/*                                                                   */
/*    Locking matrices                                               */
/*                                                                   */
/*********************************************************************/

#ifdef LOCKMANAGER_INCLUDE_COMPATABILITY_INFO
/*
   * Compatibility matrix
   *  - if ielm_lockCompat[existing][new] == NONE,
   *    the lock modes are not compatible
   */
  static const uint8_t ielm_lockCompat[ielmNUM_LOCK_MODES][ielmNUM_LOCK_MODES] =
  { { ielmLOCK_MODE_NONE , ielmLOCK_MODE_IS   , ielmLOCK_MODE_IX   , ielmLOCK_MODE_S    , ielmLOCK_MODE_SIX  , ielmLOCK_MODE_X    },
    { ielmLOCK_MODE_IS   , ielmLOCK_MODE_IS   , ielmLOCK_MODE_IX   , ielmLOCK_MODE_S    , ielmLOCK_MODE_SIX  , ielmLOCK_MODE_NONE },
    { ielmLOCK_MODE_IX   , ielmLOCK_MODE_IX   , ielmLOCK_MODE_IX   , ielmLOCK_MODE_NONE , ielmLOCK_MODE_NONE , ielmLOCK_MODE_NONE },
    { ielmLOCK_MODE_S    , ielmLOCK_MODE_S    , ielmLOCK_MODE_NONE , ielmLOCK_MODE_S    , ielmLOCK_MODE_NONE , ielmLOCK_MODE_NONE },
    { ielmLOCK_MODE_SIX  , ielmLOCK_MODE_SIX  , ielmLOCK_MODE_NONE , ielmLOCK_MODE_NONE , ielmLOCK_MODE_NONE , ielmLOCK_MODE_NONE },
    { ielmLOCK_MODE_X    , ielmLOCK_MODE_NONE , ielmLOCK_MODE_NONE , ielmLOCK_MODE_NONE , ielmLOCK_MODE_NONE , ielmLOCK_MODE_NONE } };

  /*
   * Effective mode matrix
   *  - if two locks are compatible, the effective lock mode is
   *  ielm_lockModeEffective[existing][new]
   */
  static const uint8_t ielm_lockModeEffective[ielmNUM_LOCK_MODES][ielmNUM_LOCK_MODES] =
  { { ielmLOCK_MODE_NONE , ielmLOCK_MODE_IS   , ielmLOCK_MODE_IX   , ielmLOCK_MODE_S    , ielmLOCK_MODE_SIX  , ielmLOCK_MODE_X    },
    { ielmLOCK_MODE_IS   , ielmLOCK_MODE_IS   , ielmLOCK_MODE_IX   , ielmLOCK_MODE_S    , ielmLOCK_MODE_SIX  , ielmLOCK_MODE_CLASH},
    { ielmLOCK_MODE_IX   , ielmLOCK_MODE_IX   , ielmLOCK_MODE_IX   , ielmLOCK_MODE_CLASH, ielmLOCK_MODE_CLASH, ielmLOCK_MODE_CLASH},
    { ielmLOCK_MODE_S    , ielmLOCK_MODE_S    , ielmLOCK_MODE_CLASH, ielmLOCK_MODE_S    , ielmLOCK_MODE_CLASH, ielmLOCK_MODE_CLASH},
    { ielmLOCK_MODE_SIX  , ielmLOCK_MODE_SIX  , ielmLOCK_MODE_CLASH, ielmLOCK_MODE_CLASH, ielmLOCK_MODE_CLASH, ielmLOCK_MODE_CLASH},
    { ielmLOCK_MODE_X    , ielmLOCK_MODE_CLASH, ielmLOCK_MODE_CLASH, ielmLOCK_MODE_CLASH, ielmLOCK_MODE_CLASH, ielmLOCK_MODE_CLASH} };
#endif

//Define the following to get a dump of hash table depths in the trace
//(at configurable intervals based on LOCKMANAGER_SHOW_HASH_SPREAD_EVERY requests to lock)
//#define LOCKMANAGER_SHOW_HASH_SPREAD 1
#ifdef LOCKMANAGER_SHOW_HASH_SPREAD
  uint32_t locksSinceOutput = 0;
  #define LOCKMANAGER_SHOW_HASH_SPREAD_EVERY 100000
  void ielm_showHashSpread(void);
#endif

/*********************************************************************/
/*                                                                   */
/*    Local function prototypes                                      */
/*                                                                   */
/*********************************************************************/
static uint32_t _local_hashLockName(ielmLockName_t *pLockName);

static bool _local_areLockNamesEqual(ielmLockName_t *pLN1,
                                     ielmLockName_t *pLN2);

static int32_t _local_takeLockInternal(ielmLockManager_t *pLM,
                                       ielmLockScope_t *pScope,
                                       ielmLockRequest_t *pLRB,
                                       ielmLockName_t *pLockName,
                                       uint32_t Hash,
                                       uint32_t LockMode,
                                       uint32_t LockDuration,
                                       ielmLockRequest_t **ppLockRequest);

static int32_t _local_requestLockInternal(ielmLockManager_t *pLM,
                                          ielmLockScope_t *pScope,
                                          ielmLockName_t *pLockName,
                                          uint32_t Hash,
                                          uint32_t LockMode,
                                          uint32_t LockDuration,
                                          ielmLockRequest_t **ppLockRequest,
                                          ielmAtomicRelease_t **ppAR);

static int32_t _local_instantLockInternal(ielmLockManager_t *pLM,
                                          ielmLockName_t *pLockName,
                                          uint32_t Hash,
                                          ielmAtomicRelease_t **ppAR,
                                          ismMessageState_t *pPeekData,
                                          ismMessageState_t *pPeekValueOut,
                                          ielm_InstantLockCallback_t pCallback,
                                          void *pCallbackContext);

static void _local_releaseLockInternal(ielmLockManager_t *pLM,
                                       ielmLockRequest_t *pLRB);

static void _local_releaseAllBeginInternal(ielmLockManager_t *pLM,
                                           ielmLockScope_t *pScope);

static void _local_releaseAllCompleteInternal(ieutThreadData_t *pThreadData,
                                              ielmLockManager_t *pLM,
                                              ielmLockScope_t *pScope);


/*********************************************************************/
/*                                                                   */
/*    Function Definitions                                           */
/*                                                                   */
/*********************************************************************/

/*
 * ielm_createLockManager
 *  - Creates the lock manager
 */
int32_t ielm_createLockManager(ieutThreadData_t *pThreadData)
{
    ielmLockManager_t *pLM = NULL;
    ielmLockHashChain_t *pHash;
    uint32_t t;
    uint32_t i;
    int osrc;
    int32_t rc = OK;

    pLM = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_lockManager, 1), 1, sizeof(ielmLockManager_t));
    if (pLM != NULL)
    {
        memcpy(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4);
        pLM->LockTableSize = ielmLOCK_TABLE_SIZE_DEFAULT;

        for (t = 0; (t < ielmNUM_LOCK_TYPES) && (rc == OK); t++)  /* BEAM suppression: loop doesn't iterate */
        {
            pHash = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_lockManager, 2), ielmLOCK_TABLE_SIZE_DEFAULT, sizeof(ielmLockHashChain_t));
            if (pHash != NULL)
            {
                pLM->pLockChains[t] = pHash;

                for (i = 0; i < ielmLOCK_TABLE_SIZE_DEFAULT; i++, pHash++)
                {
                    memcpy(pHash->StrucId, ielmLOCKHASHCHAIN_STRUCID, 4);
                }

                pHash = pLM->pLockChains[t];
                for (i = 0; i < ielmLOCK_TABLE_SIZE_DEFAULT; i++, pHash++)
                {
                    osrc = pthread_mutex_init(&pHash->Latch, NULL);
                    if (UNLIKELY(osrc != 0))
                    {
                        ieutTRACEL(pThreadData, osrc,  ENGINE_SEVERE_ERROR_TRACE, "%s: pthread_mutex_init failed (rc=%d)\n", __func__, osrc);
                        rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                    }
                }
            }
            else
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
            }
        }
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    if (rc == OK)
    {
        ismEngine_serverGlobal.LockManager = pLM;
    }
    else if (pLM != NULL)
    {
        for (t = 0; t < ielmNUM_LOCK_TYPES; t++)  /* BEAM suppression: loop doesn't iterate */
        {
            ielmLockHashChain_t *pThisLockChain = pLM->pLockChains[t];

            if (pThisLockChain != NULL)
            {
                pHash = pThisLockChain;

                for (i = 0; i < ielmLOCK_TABLE_SIZE_DEFAULT; i++, pHash++)
                {
                    (void)pthread_mutex_destroy(&pHash->Latch);
                }

                iemem_freeStruct(pThreadData, iemem_lockManager, pThisLockChain, pThisLockChain->StrucId);
            }
        }

        iemem_freeStruct(pThreadData, iemem_lockManager, pLM, pLM->StrucId);
    }

    return rc;
}


/*
 * ielm_destroyLockManager
 *  - Destroys the lock manager
 */
void ielm_destroyLockManager(ieutThreadData_t *pThreadData)
{
    ielmLockManager_t *pLM = ismEngine_serverGlobal.LockManager;
    uint32_t t;
    uint32_t i;

    ieutTRACEL(pThreadData, pLM, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_ENTRY "pLM=%p\n", __func__, pLM);

    if (pLM != NULL)
    {
        ismEngine_serverGlobal.LockManager = NULL;

        assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

        for (t = 0; t < ielmNUM_LOCK_TYPES; t++)  /* BEAM suppression: loop doesn't iterate */
        {
            ielmLockHashChain_t *pThisLockChain = pLM->pLockChains[t];

            if (pThisLockChain != NULL)
            {
                ielmLockHashChain_t *pHash = pThisLockChain;

                for (i = 0; i < ielmLOCK_TABLE_SIZE_DEFAULT; i++, pHash++)
                {
                    (void)pthread_mutex_destroy(&pHash->Latch);
                }

                iemem_freeStruct(pThreadData, iemem_lockManager, pThisLockChain, pThisLockChain->StrucId);
            }
        }

        iemem_freeStruct(pThreadData, iemem_lockManager, pLM, pLM->StrucId);
    }

    ieutTRACEL(pThreadData, pLM, ENGINE_SHUTDOWN_DIAG_TRACE, FUNCTION_EXIT "\n", __func__);
}


/*
 * ielm_allocateLockScope
 *  - Allocates a lock scope
 */
int32_t ielm_allocateLockScope(ieutThreadData_t *pThreadData,
                               uint32_t ScopeOptions,
                               iempMemPoolHandle_t hMemPool,
                               ielmLockScope_t **ppLockScope)
{
    ielmLockScope_t *pScope = NULL;
    ielmAtomicRelease_t *pAR = NULL;
    int osrc;
    int32_t rc = OK;

    if (hMemPool != NULL)
    {
        rc = iemp_allocate(pThreadData, hMemPool, sizeof(ielmLockScope_t), (void **)&pScope);

        if (rc != OK)
        {
            pScope = NULL;
            goto exit;
        }
        memset(pScope, 0, sizeof(ielmLockScope_t));
        pScope->fMemPool = true;
    }
    else
    {
        pScope = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_lockManager, 3), 1, sizeof(ielmLockScope_t));
    }

    if (pScope != NULL)
    {
        memcpy(pScope->StrucId, ielmLOCKSCOPE_STRUCID, 4);
        pthread_spin_init(&pScope->Latch, PTHREAD_SCOPE_PROCESS);

        if (ScopeOptions & ielmLOCK_SCOPE_COMMIT_CAPABLE)
        {
            //Can't use a mempool for the AR as it can only be freed when its interest count reaches 0
            pAR = iemem_calloc(pThreadData, IEMEM_PROBE(iemem_lockManager, 4), 1, sizeof(ielmAtomicRelease_t));

            if (pAR != NULL)
            {
                memcpy(pAR->StrucId, ielmATOMICRELEASE_STRUCID, 4);
                pAR->InterestCount = 1; //Not destroyed yet
                osrc = pthread_mutex_init(&pAR->Latch, NULL);
                if (osrc == 0)
                {
                    osrc = pthread_cond_init(&pAR->Event, NULL);
                    if (UNLIKELY(osrc != 0))
                    {
                        ieutTRACEL(pThreadData, osrc,  ENGINE_SEVERE_ERROR_TRACE, "%s: pthread_cond_init failed (rc=%d)\n", __func__, osrc);
                        rc = ISMRC_AllocateError;
                        ism_common_setError(rc);
                        (void)pthread_mutex_destroy(&pAR->Latch);

                        iemem_freeStruct(pThreadData, iemem_lockManager, pAR, pAR->StrucId);
                        goto exit;
                    }
                }
                else
                {
                    ieutTRACEL(pThreadData, osrc,  ENGINE_SEVERE_ERROR_TRACE, "%s: pthread_mutex_init failed (rc=%d)\n", __func__, osrc);
                    rc = ISMRC_AllocateError;
                    ism_common_setError(rc);

                    iemem_freeStruct(pThreadData, iemem_lockManager, pAR, pAR->StrucId);
                    goto exit;
                }
            }
            else
            {
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto exit;
            }

            pScope->pCacheAtomicRelease = pAR;
        }
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

exit:
    if (rc == OK)
    {
        *ppLockScope = pScope;
    }
    else
    {
        if (pScope != NULL)
        {
            pthread_spin_destroy(&pScope->Latch);
            if(!pScope->fMemPool)
            {
                iemem_freeStruct(pThreadData, iemem_lockManager, pScope, pScope->StrucId);
            }
        }
    }

    return rc;
}

static void ielm_reduceAtomicReleaseInterestCount( ieutThreadData_t *pThreadData
                                                 , ielmAtomicRelease_t *pAR)
{
    if (__sync_sub_and_fetch(&pAR->InterestCount, 1) == 0)
    {
        (void)pthread_cond_destroy(&pAR->Event);
        (void)pthread_mutex_destroy(&pAR->Latch);

        iemem_freeStruct(pThreadData, iemem_lockManager, pAR, pAR->StrucId);
    }
}

/*
 * ielm_freeLockScope
 *  - Frees a lock scope
 */
void ielm_freeLockScope(ieutThreadData_t *pThreadData, ielmLockScope_t **ppLockScope)
{
    ielmLockScope_t *pScope = NULL;
    ielmAtomicRelease_t *pAR;

    if (*ppLockScope != NULL)
    {
        pScope = *ppLockScope;
        assert(pScope->RequestCount == 0);

        *ppLockScope = NULL;

        if (pScope->pCacheRequest != NULL)
        {
            iemem_freeStruct(pThreadData, iemem_lockManager, pScope->pCacheRequest, pScope->pCacheRequest->StrucId);
        }

        if (pScope->pCacheAtomicRelease != NULL)
        {
            pAR = pScope->pCacheAtomicRelease;

            //Remove the interest count that implies it is not destroyed
            ielm_reduceAtomicReleaseInterestCount(pThreadData, pAR);
        }

        pthread_spin_destroy(&pScope->Latch);

        if (!(pScope->fMemPool))
        {
            iemem_freeStruct(pThreadData, iemem_lockManager, pScope, pScope->StrucId);
        }
    }

    return;
}


/*******************************************************************/
/* ielm_allocateCachedLockRequest - Allocates a lock request       */
/* structure. Can be used when later request will need to be made  */
/* but no mallocs are allowed (during commit)                      */
/*******************************************************************/
int32_t ielm_allocateCachedLockRequest(ieutThreadData_t *pThreadData, ielmLockRequest_t **ppLockRequest)
{
    int32_t rc = OK;

    *ppLockRequest = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_lockManager, 7), sizeof(ielmLockRequest_t));

    if (*ppLockRequest != NULL)
    {
        memcpy((*ppLockRequest)->StrucId, ielmLOCKREQUEST_STRUCID, 4);
        (*ppLockRequest)->LockMode = ielmLOCK_MODE_NONE;
        (*ppLockRequest)->LockDuration = ielmLOCK_DURATION_NONE;
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    return rc;
}

/*
 * ielm_freeLockRequest
 *  - Frees a lock request for a lock which has already been released, but not freed.
 */
void ielm_freeLockRequest(ieutThreadData_t *pThreadData, ielmLockRequest_t *pLRB)
{
    assert(pLRB->LockMode == ielmLOCK_MODE_NONE);
    assert(pLRB->LockDuration == ielmLOCK_DURATION_NONE);

    iemem_freeStruct(pThreadData, iemem_lockManager, pLRB, pLRB->StrucId);
}


/*
 * ielm_takeLock
 *  - Takes a lock which is not contested
 */
int32_t ielm_takeLock(ieutThreadData_t *pThreadData,
                      ielmLockScope_t *pScope,
                      ielmLockRequest_t *pLockRequestIn,
                      ielmLockName_t *pLockName,
                      uint32_t LockMode,
                      uint32_t LockDuration,
                      ielmLockRequest_t **ppLockRequestOut)
{
    ielmLockManager_t *pLM;
    uint32_t hash;
    ielmLockRequest_t *pLockRequest = NULL;
    bool fUsingCachedLRB = true;
    int32_t rc = OK;

    pLM = ismEngine_serverGlobal.LockManager;
    assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

    // For atomic release of commit-duration locks, it is assumed
    // that commit-duration locks are exclusive only, and thus that
    // there's at most one lock request chained off the lock header.
    assert((LockDuration != ielmLOCK_DURATION_COMMIT) || (LockMode == ielmLOCK_MODE_X));
    assert((LockDuration & ielmLOCK_DURATION_MASK) != ielmLOCK_DURATION_NONE);

    // Hash the lock name
    assert(pLockName->Common.LockType < ielmNUM_LOCK_TYPES);
    hash = _local_hashLockName(pLockName);

    // We start off by making sure that we have cached any structures
    // we may need to use during the request.
    (void)pthread_spin_lock(&pScope->Latch);

    if (pLockRequestIn != NULL)
    {
        fUsingCachedLRB = false;
        pLockRequest = pLockRequestIn;
    }
    else
    {
        if (pScope->pCacheRequest == NULL)
        {
            pScope->pCacheRequest = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_lockManager, 5), sizeof(ielmLockRequest_t));
            if (pScope->pCacheRequest == NULL)
            {
                (void)pthread_spin_unlock(&pScope->Latch);
                rc = ISMRC_AllocateError;
                ism_common_setError(rc);
                goto exit;
            }
        }

        pLockRequest = pScope->pCacheRequest;
    }

    if ((LockDuration & ielmLOCK_DURATION_COMMIT) &&
        (pScope->CommitDurationCount == 0) &&
        (pScope->pCacheAtomicRelease == NULL))
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(9, true,
                       "Atomic release not allocated", rc,
                       NULL);
    }

    if (rc == OK)
    {
        rc = _local_takeLockInternal(pLM, pScope, pLockRequest, pLockName, hash, LockMode, LockDuration, &pLockRequest);
    }

    (void)pthread_spin_unlock(&pScope->Latch);

exit:
    if ((rc == OK) && fUsingCachedLRB)
    {
        pScope->pCacheRequest = NULL;
    }

    if (ppLockRequestOut != NULL)
    {
        *ppLockRequestOut = pLockRequest;
    }

#ifdef LOCKMANAGER_SHOW_HASH_SPREAD
    uint32_t locksSince = __sync_add_and_fetch(&locksSinceOutput, 1);

    if (locksSince > LOCKMANAGER_SHOW_HASH_SPREAD_EVERY) {
        ielm_showHashSpread();
        locksSinceOutput=0;
    }
#endif

    return rc;
}


/*
 * ielm_instantLockWithPeek
 *  - Requests an exclusive lock for an instant and optionally peeks at
 *    a memory address as it does so. This does not respect atomic
 *    release of commit-duration locks.
 */
int32_t ielm_instantLockWithPeek(ielmLockName_t *pLockName,
                                 ismMessageState_t *pPeekData,
                                 ismMessageState_t *pPeekValueOut)
{
    ielmLockManager_t *pLM;
    uint32_t hash;
    int32_t rc = OK;

    pLM = ismEngine_serverGlobal.LockManager;
    assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

    assert(pLockName->Common.LockType < ielmNUM_LOCK_TYPES);
    hash = _local_hashLockName(pLockName);

    rc = _local_instantLockInternal(pLM,
                                    pLockName,
                                    hash,
                                    NULL,
                                    pPeekData,
                                    pPeekValueOut,
                                    NULL,
                                    NULL);

    return rc;
}

/*
 * ielm_instantLockWithCallback
 *  - Requests an exclusive lock for an instant calls a callback whilst the lock is held.
 *    NB: If fPauseForCommit=False then this does not respect atomic release of commit-duration locks.
 *
 *    NB: Callback should be /very/ short running.  A chain of locks
 * (i.e. multiple locks) are paused whilst the callback is run
 */
int32_t ielm_instantLockWithCallback( ieutThreadData_t *pThreadData
                                    , ielmLockName_t *pLockName
                                    , bool fPauseForCommit
                                    , ielm_InstantLockCallback_t pCallback
                                    , void *pCallbackContext)
{
    ielmLockManager_t *pLM;
    uint32_t hash;
    ielmAtomicRelease_t *pAR = NULL;
    int32_t rc = OK;
    bool tryAgain;

    pLM = ismEngine_serverGlobal.LockManager;
    assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

    assert(pLockName->Common.LockType < ielmNUM_LOCK_TYPES);
    hash = _local_hashLockName(pLockName);

    do
    {
        tryAgain = false;

        rc = _local_instantLockInternal(pLM,
                                         pLockName,
                                         hash,
                                         (fPauseForCommit ? &pAR : NULL),
                                         NULL,
                                         NULL,
                                         pCallback,
                                         pCallbackContext);

        if ((rc == ISMRC_LockNotGranted) && (pAR != NULL))
        {
            // If the lock we are requesting is being released as
            // part of a transaction commit, we wait for the release
            // and try again
            tryAgain = true;

            int osrc = pthread_mutex_lock(&pAR->Latch);
            if (UNLIKELY(osrc != 0))
            {
                rc = ISMRC_Error;
                ism_common_setError(rc);
                ieutTRACE_FFDC(1, true,
                               "pthread_mutex_lock failed", rc,
                               "osrc", &osrc, sizeof(osrc),
                               NULL);
            }

            while (!(pAR->fEventFired))
            {
                osrc = pthread_cond_wait(&pAR->Event, &pAR->Latch);
                if (UNLIKELY(osrc != 0))
                {
                    rc = ISMRC_Error;
                    ism_common_setError(rc);
                    ieutTRACE_FFDC(2, true,
                                   "pthread_cond_wait failed", rc,
                                   "osrc", &osrc, sizeof(osrc),
                                   NULL);
                }
            }

            (void)pthread_mutex_unlock(&pAR->Latch);

            ielm_reduceAtomicReleaseInterestCount(pThreadData, pAR);

            pAR = NULL;
        }
    }
    while (tryAgain);

    return rc;
}


/*
 * ielm_requestLock
 *  - Requests a lock
 */
int32_t ielm_requestLock(ieutThreadData_t *pThreadData,
                         ielmLockScope_t *pScope,
                         ielmLockName_t *pLockName,
                         uint32_t LockMode,
                         uint32_t LockDuration,
                         ielmLockRequest_t **ppLockRequestOut)
{
    ielmLockManager_t *pLM;
    uint32_t hash;
    ielmAtomicRelease_t *pAR = NULL;
    ielmLockRequest_t *pLockRequest = NULL;
    bool fScopeLocked = false;
    int osrc;
    int32_t rc = OK;

    pLM = ismEngine_serverGlobal.LockManager;
    assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

    // For atomic release of commit-duration locks, it is assumed
    // that commit-duration locks are exclusive only, and thus that
    // there's at most one lock request chained off the lock header.
    assert((LockDuration != ielmLOCK_DURATION_COMMIT) || (LockMode == ielmLOCK_MODE_X));
    assert((LockDuration & ielmLOCK_DURATION_MASK) != ielmLOCK_DURATION_NONE);

    // Hash the lock name
    assert(pLockName->Common.LockType < ielmNUM_LOCK_TYPES);
    hash = _local_hashLockName(pLockName);

    // We start off by making sure that we have cached any structures
    // we may need to use during the request.
    (void)pthread_spin_lock(&pScope->Latch);
    fScopeLocked = true;

    if (pScope->pCacheRequest == NULL)
    {
        pScope->pCacheRequest = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_lockManager, 6), sizeof(ielmLockRequest_t));
        if (pScope->pCacheRequest == NULL)
        {
            rc = ISMRC_AllocateError;
            ism_common_setError(rc);
            goto exit;
        }
    }

    if ((LockDuration & ielmLOCK_DURATION_COMMIT) &&
        (pScope->CommitDurationCount == 0) &&
        (pScope->pCacheAtomicRelease == NULL))
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(10, true,
                       "Atomic release not allocated", rc,
                       NULL);
    }

    while (rc == OK)
    {
        rc = _local_requestLockInternal(pLM, pScope, pLockName, hash, LockMode, LockDuration, &pLockRequest, &pAR);

        if ((rc == ISMRC_LockNotGranted) && (pAR != NULL))
        {
            // If the lock we are requesting is being released as
            // part of a transaction commit, we wait for the release
            rc = OK;

            osrc = pthread_mutex_lock(&pAR->Latch);
            if (UNLIKELY(osrc != 0))
            {
                rc = ISMRC_Error;
                ism_common_setError(rc);
                ieutTRACE_FFDC(1, true,
                               "pthread_mutex_lock failed", rc,
                               "osrc", &osrc, sizeof(osrc),
                               NULL);
                goto exit;
            }

            while (!(pAR->fEventFired))
            {
                osrc = pthread_cond_wait(&pAR->Event, &pAR->Latch);
                if (UNLIKELY(osrc != 0))
                {
                    rc = ISMRC_Error;
                    ism_common_setError(rc);
                    ieutTRACE_FFDC(2, true,
                                   "pthread_cond_wait failed", rc,
                                   "osrc", &osrc, sizeof(osrc),
                                   NULL);
                }
            }

            (void)pthread_mutex_unlock(&pAR->Latch);
            if (rc != OK)
            {
                goto exit;
            }

            ielm_reduceAtomicReleaseInterestCount(pThreadData, pAR);

            pAR = NULL;
        }
        else
        {
            break;
        }
    }

exit:
    if (fScopeLocked)
    {
        (void)pthread_spin_unlock(&pScope->Latch);
    }

    if (ppLockRequestOut != NULL)
    {
        *ppLockRequestOut = pLockRequest;
    }

#ifdef LOCKMANAGER_SHOW_HASH_SPREAD
    uint32_t locksSince = __sync_add_and_fetch(&locksSinceOutput, 1);

    if (locksSince > LOCKMANAGER_SHOW_HASH_SPREAD_EVERY) {
        ielm_showHashSpread();
        locksSinceOutput=0;
    }
#endif

    return rc;
}


/*
 * ielm_releaseLock
 *  - Releases a lock
 *
 * This function is only permitted for release of a
 * request-duration lock. To release a commit-duration lock, you
 * must use ielm_releaseAllLocksBegin/Complete.
 */
void ielm_releaseLock(ieutThreadData_t *pThreadData,
                      ielmLockScope_t *pScope,
                      ielmLockRequest_t *pLRB)
{
    ielmLockManager_t *pLM;
    ielmLockRequest_t *pOtherLRB;

    pLM = ismEngine_serverGlobal.LockManager;
    assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

    // It's not permitted to release a commit-duration lock like
    // this. All commit-duration locks in a scope must be released
    // atomically to ensure message ordering

    // First, remove the LRB from the scope
    (void)pthread_spin_lock(&pScope->Latch);
    if (pLRB->pScopeReqPrev == NULL)
    {
        pScope->pScopeReqHead = pLRB->pScopeReqNext;
    }
    else
    {
        pOtherLRB = pLRB->pScopeReqPrev;
        pOtherLRB->pScopeReqNext = pLRB->pScopeReqNext;
    }

    if (pLRB->pScopeReqNext == NULL)
    {
        pScope->pScopeReqTail = pLRB->pScopeReqPrev;
    }
    else
    {
        pOtherLRB = pLRB->pScopeReqNext;
        pOtherLRB->pScopeReqPrev = pLRB->pScopeReqPrev;
    }

    pLRB->pScopeReqNext = NULL;
    pLRB->pScopeReqPrev = NULL;
    pScope->RequestCount--;

    // Then, release the lock
    _local_releaseLockInternal(pLM, pLRB);

    if (pScope->pCacheRequest == NULL)
    {
        pScope->pCacheRequest = pLRB;
    }
    else
    {
        iemem_freeStruct(pThreadData, iemem_lockManager, pLRB, pLRB->StrucId);
    }

    (void)pthread_spin_unlock(&pScope->Latch);

    return;
}


/*******************************************************************/
/* ielm_releaseLockNoFree - Releases a lock but does not free it   */
/* This function is only permitted for release of a                */
/* request-duration lock. The memory for the lock request is not   */
/* freed and is no longer associated with the scope. The memory    */
/* must be used to take a lock using ielm_takeLock, or freed       */
/* explicitly using ielm_freeLockRequest.                          */
/*******************************************************************/
void ielm_releaseLockNoFree(ielmLockScope_t *pScope,
                            ielmLockRequest_t *pLRB)
{
    ielmLockManager_t *pLM;
    ielmLockRequest_t *pOtherLRB;

    pLM = ismEngine_serverGlobal.LockManager;
    assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

    // It's not permitted to release a commit-duration lock like
    // this. All commit-duration locks in a scope must be released
    // atomically to ensure message ordering

    // First, remove the LRB from the scope
    (void)pthread_spin_lock(&pScope->Latch);
    if (pLRB->pScopeReqPrev == NULL)
    {
        pScope->pScopeReqHead = pLRB->pScopeReqNext;
    }
    else
    {
        pOtherLRB = pLRB->pScopeReqPrev;
        pOtherLRB->pScopeReqNext = pLRB->pScopeReqNext;
    }

    if (pLRB->pScopeReqNext == NULL)
    {
        pScope->pScopeReqTail = pLRB->pScopeReqPrev;
    }
    else
    {
        pOtherLRB = pLRB->pScopeReqNext;
        pOtherLRB->pScopeReqPrev = pLRB->pScopeReqPrev;
    }

    pLRB->pScopeReqNext = NULL;
    pLRB->pScopeReqPrev = NULL;
    pScope->RequestCount--;

    // Then, release the lock
    _local_releaseLockInternal(pLM, pLRB);

    // Do not free the lock. It's no longer within a scope, but the storage can be
    // used for ielm_requestLock. Reinitialise it so no one gets confused.
    memset(pLRB, 0, sizeof(ielmLockRequest_t));
    memcpy(pLRB->StrucId, ielmLOCKREQUEST_STRUCID, 4);

    (void)pthread_spin_unlock(&pScope->Latch);

    return;
}


/*
 * ielm_releaseAllLocksBegin
 *  - Begins release of all locks in scope
 *
 * This function begins the release of all locks in a scope. If
 * there are commit-duration locks, this causes future requests of
 * these locks to be delayed until the release is completed,
 * rather than being bounced directly.
 */
void ielm_releaseAllLocksBegin(ielmLockScope_t *pScope)
{
    ielmLockManager_t *pLM;

    pLM = ismEngine_serverGlobal.LockManager;
    assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

    // If this scope contains any commit-duration locks, mark them
    // all as being released and delay any requests for these locks
    // until they've ALL been released
    (void)pthread_spin_lock(&pScope->Latch);

    if (pScope->CommitDurationCount > 0)
    {
        _local_releaseAllBeginInternal(pLM, pScope);
    }

    (void)pthread_spin_unlock(&pScope->Latch);

    return;
}


/*
 * ielm_releaseAllLocksComplete
 *  - Completes release of all locks in scope
 *
 * This function completes release of all locks in a scope. Any
 * requests delayed until the completion of the release are allowed
 * to proceed at the end of this processing.
 */
void ielm_releaseAllLocksComplete(ieutThreadData_t *pThreadData, ielmLockScope_t *pScope)
{
    ielmLockManager_t *pLM;
    ielmAtomicRelease_t *pAR;
    int osrc;
    int32_t rc = OK;

    pLM = ismEngine_serverGlobal.LockManager;
    assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

    // Iterate through the LRBs in the scope and release them all.
    // We iterate through them BACKWARDS just in case there's a
    // locking hierarchy on top of us which assumes this
    (void)pthread_spin_lock(&pScope->Latch);

    _local_releaseAllCompleteInternal(pThreadData, pLM, pScope);

    // If this scope contained any commit-duration locks, now
    // release the atomic release latch to enable delayed requests
    // to proceed
    if (pScope->pCurrentAtomicRelease != NULL)
    {
        pAR = pScope->pCurrentAtomicRelease;

        pScope->CommitDurationCount = 0;
        pScope->pCurrentAtomicRelease = NULL;

        if (__sync_sub_and_fetch(&pAR->InterestCount, 1) > 0)
        {
            osrc = pthread_mutex_lock(&pAR->Latch);
            if (UNLIKELY(osrc != 0))
            {
                rc = ISMRC_Error;
                ism_common_setError(rc);
                ieutTRACE_FFDC(3, true,
                               "pthread_mutex_lock failed", rc,
                               "osrc", &osrc, sizeof(osrc),
                               NULL);
            }

            pAR->fEventFired = true;

            osrc = pthread_cond_broadcast(&pAR->Event);
            if (UNLIKELY(osrc != 0))
            {
                rc = ISMRC_Error;
                ism_common_setError(rc);
                ieutTRACE_FFDC(4, true,
                               "pthread_cond_broadcast failed", rc,
                               "osrc", &osrc, sizeof(osrc),
                               NULL);
            }

            (void)pthread_mutex_unlock(&pAR->Latch);
        }
    }

    (void)pthread_spin_unlock(&pScope->Latch);

    return;
}


/*
 * ielm_releaseManyLocksBegin
 *  - Begins the release of many locks in a scope
 *
 * This function begins the release of many locks in a scope.
 * It locks the scope in preparation for a sequence of calls to
 * ielm_releaseOneOfManyLocks followed by a final call to
 * ielm_releaseManyLocksComplete.
 */
int32_t ielm_releaseManyLocksBegin(ielmLockScope_t *pScope)
{
    DEBUG_ONLY ielmLockManager_t *pLM;
    int32_t rc = OK;

    pLM = ismEngine_serverGlobal.LockManager;
    assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

    (void)pthread_spin_lock(&pScope->Latch);
    return rc;
}


/*
 * ielm_releaseOneOfManyLocks
 *  - Releases one lock in a sequence of many releases
 *
 * This function is only permitted for release of a
 * request-duration lock. The function is only permitted between
 * calls to ielm_releaseManyLocksBegin/Complete.
 */
void ielm_releaseOneOfManyLocks(ieutThreadData_t *pThreadData,
                                ielmLockScope_t *pScope,
                                ielmLockRequest_t *pLRB)
{
    ielmLockManager_t *pLM;
    ielmLockRequest_t *pOtherLRB;

    pLM = ismEngine_serverGlobal.LockManager;
    assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

    // It's not permitted to release a commit-duration lock like
    // this. All commit-duration locks in a scope must be released
    // atomically to ensure message ordering

    // First, remove the LRB from the scope
    if (pLRB->pScopeReqPrev == NULL)
    {
        pScope->pScopeReqHead = pLRB->pScopeReqNext;
    }
    else
    {
        pOtherLRB = pLRB->pScopeReqPrev;
        pOtherLRB->pScopeReqNext = pLRB->pScopeReqNext;
    }

    if (pLRB->pScopeReqNext == NULL)
    {
        pScope->pScopeReqTail = pLRB->pScopeReqPrev;
    }
    else
    {
        pOtherLRB = pLRB->pScopeReqNext;
        pOtherLRB->pScopeReqPrev = pLRB->pScopeReqPrev;
    }

    pLRB->pScopeReqNext = NULL;
    pLRB->pScopeReqPrev = NULL;
    pScope->RequestCount--;

    // Then, release the lock
    _local_releaseLockInternal(pLM, pLRB);

    if (pScope->pCacheRequest == NULL)
    {
        pScope->pCacheRequest = pLRB;
    }
    else
    {
        iemem_freeStruct(pThreadData, iemem_lockManager, pLRB, pLRB->StrucId);
    }

    return;
}


/*
 * ielm_releaseManyLocksComplete
 *  - Completes release of many locks in a scope
 *
 * This function completes release of many locks in a scope. It
 * unlocks the scope at the end of a sequence of calls which
 * began with ielm_releaseManyLocksBegin.
 */
int32_t ielm_releaseManyLocksComplete(ielmLockScope_t *pScope)
{
    DEBUG_ONLY ielmLockManager_t *pLM;
    int32_t rc = OK;

    pLM = ismEngine_serverGlobal.LockManager;
    assert(memcmp(pLM->StrucId, ielmLOCKMANAGER_STRUCID, 4) == 0);

    (void)pthread_spin_unlock(&pScope->Latch);
    return rc;
}


/*
 * _local_hashLockName
 *  - Computes hash value for a lock name
 */
static uint32_t _local_hashLockName(ielmLockName_t *pLockName)
{
    uint32_t result = 0;

    switch (pLockName->Common.LockType)
    {
        case ielmLOCK_TYPE_MESSAGE:
            result = (uint32_t)( ((pLockName->Msg.QId << 24)^(pLockName->Msg.NodeId))
                                % ielmLOCK_TABLE_SIZE_DEFAULT);
            break;
    }

    return result;
}


/*
 * _local_areLockNamesEqual
 *  - Compares two lock names for equality
 */
static inline bool _local_areLockNamesEqual(ielmLockName_t *pLN1, ielmLockName_t *pLN2)
{
    if (pLN1->Common.LockType != pLN2->Common.LockType)
    {
        return false;
    }

    switch (pLN1->Common.LockType)
    {
        case ielmLOCK_TYPE_MESSAGE:
            return ((pLN1->Msg.NodeId == pLN2->Msg.NodeId) && (pLN1->Msg.QId == pLN2->Msg.QId)) ? true : false;
            break;
    }

    return false;
}


/*
 * _local_takeLockInternal
 *  - Takes a lock which is not contested
 */
static int32_t _local_takeLockInternal(ielmLockManager_t *pLM,
                                       ielmLockScope_t *pScope,
                                       ielmLockRequest_t *pLRB,
                                       ielmLockName_t *pLockName,
                                       uint32_t Hash,
                                       uint32_t LockMode,
                                       uint32_t LockDuration,
                                       ielmLockRequest_t **ppLockRequest)
{
    ielmLockHashChain_t *pHashChain;
    ielmLockRequest_t *pOtherLRB;
    bool fChainLatched = false;
    int osrc;
    int32_t rc = OK;

    // Calculate the hash chain
    pHashChain = pLM->pLockChains[pLockName->Common.LockType];
    pHashChain += (Hash % pLM->LockTableSize);

    // We appear to have relatively high contention on the hash chain
    // latches in some scenarios. Try to minimise the pathlength when
    // we have the latch by preparing the structures which we may link
    // into the hash chain if the lock cannot be found
    memcpy(pLRB->StrucId, ielmLOCKREQUEST_STRUCID, 4);
    memcpy(&pLRB->LockName, pLockName, sizeof(pLRB->LockName));
    pLRB->HashValue = Hash;
    pLRB->pScopeReqNext = NULL;
    pLRB->pScopeReqPrev = NULL;
    pLRB->LockMode = ielmLOCK_MODE_NONE;
    pLRB->LockDuration = ielmLOCK_DURATION_NONE;
    pLRB->fBeingReleased = false;
    pLRB->pAtomicRelease = NULL;

    // Latch the hash chain and add the lock
    osrc = pthread_mutex_lock(&pHashChain->Latch);
    if (UNLIKELY(osrc != 0))
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(5, true,
                       "pthread_mutex_lock failed", rc,
                       "osrc", &osrc, sizeof(osrc),
                       NULL);
    }
    
    if (rc == OK)
    {
        fChainLatched = true;

        if (LockDuration == ielmLOCK_DURATION_COMMIT)
        {
            if (pScope->pCurrentAtomicRelease == NULL)
            {
                pScope->pCurrentAtomicRelease = pScope->pCacheAtomicRelease;
                pScope->pCurrentAtomicRelease->InterestCount = 2; //+1 as not being destroyed, +1 as we are interested
            }

            pScope->CommitDurationCount++;

            pLRB->pAtomicRelease = pScope->pCurrentAtomicRelease;
        }

        pLRB->LockMode = LockMode;
        pLRB->LockDuration = LockDuration;

        if (pHashChain->pChainHead == NULL)
        {
            assert(pHashChain->pChainTail == NULL);
            pLRB->pChainPrev = NULL;
            pLRB->pChainNext = NULL;
            pHashChain->pChainHead = pLRB;
            pHashChain->pChainTail = pLRB;
        }
        else
        {
            pOtherLRB = pHashChain->pChainTail;
            pLRB->pChainPrev = pHashChain->pChainTail;
            pLRB->pChainNext = NULL;
            pOtherLRB->pChainNext = pLRB;
            pHashChain->pChainTail = pLRB;
        }

        pHashChain->HeaderCount++;

        (void)pthread_mutex_unlock(&pHashChain->Latch);
        fChainLatched = false;

        // Finally, chain the LRB onto the scope
        if (pScope->pScopeReqHead == NULL)
        {
            assert(pScope->pScopeReqTail == NULL);
            pLRB->pScopeReqPrev = NULL;
            pLRB->pScopeReqNext = NULL;
            pScope->pScopeReqHead = pLRB;
            pScope->pScopeReqTail = pLRB;
        }
        else
        {
            pOtherLRB = pScope->pScopeReqTail;
            pLRB->pScopeReqNext = NULL;
            pLRB->pScopeReqPrev = pScope->pScopeReqTail;
            pOtherLRB->pScopeReqNext = pLRB;
            pScope->pScopeReqTail = pLRB;
        }

        pScope->RequestCount++;

        *ppLockRequest = pLRB;
    }

    if (fChainLatched)
    {
        (void)pthread_mutex_unlock(&pHashChain->Latch);
    }

    return rc;
}


/*
 * _local_requestLockInternal
 *  - Requests a lock
 */
static int32_t _local_requestLockInternal(ielmLockManager_t *pLM,
                                          ielmLockScope_t *pScope,
                                          ielmLockName_t *pLockName,
                                          uint32_t Hash,
                                          uint32_t LockMode,
                                          uint32_t LockDuration,
                                          ielmLockRequest_t **ppLockRequest,
                                          ielmAtomicRelease_t **ppAR)
{
    ielmLockHashChain_t *pHashChain;
    ielmLockRequest_t *pLRB;
    ielmLockRequest_t *pOtherLRB;
    bool fChainLatched = false;
    bool fLockFound = false;
    int osrc;
    int32_t rc = OK;

    *ppAR = NULL;

    // Calculate the hash chain
    pHashChain = pLM->pLockChains[pLockName->Common.LockType];
    pHashChain += (Hash % pLM->LockTableSize);

    // We appear to have relatively high contention on the hash chain
    // latches in some scenarios. Try to minimise the pathlength when
    // we have the latch by preparing the structures which we may link
    // into the hash chain if the lock cannot be found
    pLRB = pScope->pCacheRequest;
    memcpy(pLRB->StrucId, ielmLOCKREQUEST_STRUCID, 4);
    memcpy(&pLRB->LockName, pLockName, sizeof(pLRB->LockName));
    pLRB->HashValue = Hash;
    pLRB->pScopeReqNext = NULL;
    pLRB->pScopeReqPrev = NULL;
    pLRB->LockMode = LockMode;
    pLRB->LockDuration = LockDuration;
    pLRB->fBeingReleased = false;
    pLRB->pAtomicRelease = NULL;

    // Latch the hash chain and go looking for the lock
    osrc = pthread_mutex_lock(&pHashChain->Latch);
    if (UNLIKELY(osrc != 0))
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(6, true,
                       "pthread_mutex_lock failed", rc,
                       "osrc", &osrc, sizeof(osrc),
                       NULL);
    }

    if (rc == OK)
    {
        fChainLatched = true;

        // While we have not looked at all of the entries...
        pLRB = pHashChain->pChainHead;
        while (pLRB != NULL)
        {
            // If the hash values match, it's possibly our lock
            if (pLRB->HashValue == Hash)
            {
                if (_local_areLockNamesEqual(pLockName, &pLRB->LockName))
                {
                    fLockFound = true;
                    break;
                }
            }

            // Continue from where we left off
            pLRB = pLRB->pChainNext;
        }
    }

    // If we haven't found the lock header, we consume the cached one
    // and grant the lock request
    if ((rc == OK) && !fLockFound)
    {
        if (LockDuration == ielmLOCK_DURATION_COMMIT)
        {
            if (pScope->pCurrentAtomicRelease == NULL)
            {
                pScope->pCurrentAtomicRelease = pScope->pCacheAtomicRelease;
                pScope->pCurrentAtomicRelease->InterestCount = 2; //+1 as not being destroyed, +1 as we are interested
            }

            pScope->CommitDurationCount++;

            pLRB = pScope->pCacheRequest;
            pLRB->pAtomicRelease = pScope->pCurrentAtomicRelease;
        }

        pLRB = pScope->pCacheRequest;
        pScope->pCacheRequest = NULL;

        if (pHashChain->pChainHead == NULL)
        {
            assert(pHashChain->pChainTail == NULL);
            pLRB->pChainPrev = NULL;
            pLRB->pChainNext = NULL;
            pHashChain->pChainHead = pLRB;
            pHashChain->pChainTail = pLRB;
        }
        else
        {
            pOtherLRB = pHashChain->pChainTail;
            pLRB->pChainPrev = pHashChain->pChainTail;
            pLRB->pChainNext = NULL;
            pOtherLRB->pChainNext = pLRB;
            pHashChain->pChainTail = pLRB;
        }

        pHashChain->HeaderCount++;

        (void)pthread_mutex_unlock(&pHashChain->Latch);
        fChainLatched = false;

        // Finally, chain the LRB onto the scope
        if (pScope->pScopeReqHead == NULL)
        {
            assert(pScope->pScopeReqTail == NULL);
            pLRB->pScopeReqPrev = NULL;
            pLRB->pScopeReqNext = NULL;
            pScope->pScopeReqHead = pLRB;
            pScope->pScopeReqTail = pLRB;
        }
        else
        {
            pOtherLRB = pScope->pScopeReqTail;
            pLRB->pScopeReqNext = NULL;
            pLRB->pScopeReqPrev = pScope->pScopeReqTail;
            pOtherLRB->pScopeReqNext = pLRB;
            pScope->pScopeReqTail = pLRB;
        }

        pScope->RequestCount++;

        *ppLockRequest = pLRB;
    }

    // If we have found the lock header, check if we can satisfy the request
    if ((rc == OK) && fLockFound)
    {
        // If this lock is being released as an atomic operation with
        // the other commit-duration locks in its scope, we register
        // our interest in the lock and prepare to wait for its release
        if (pLRB->fBeingReleased)
        {
            ielmAtomicRelease_t *pOwnerAR = pLRB->pAtomicRelease;
            __sync_fetch_and_add(&pOwnerAR->InterestCount, 1);
            *ppAR = pOwnerAR;
        }
        else
        {
            *ppAR = NULL;
        }

        *ppLockRequest = NULL;
        rc = ISMRC_LockNotGranted;
    }

    if (fChainLatched)
    {
        (void)pthread_mutex_unlock(&pHashChain->Latch);
    }

    return rc;
}


/*
 * _local_instantLockInternal
 *  - Requests a lock with instant duration
 */
static int32_t _local_instantLockInternal(ielmLockManager_t *pLM,
                                          ielmLockName_t *pLockName,
                                          uint32_t Hash,
                                          ielmAtomicRelease_t **ppAR,
                                          ismMessageState_t *pPeekData,
                                          ismMessageState_t *pPeekValueOut,
                                          ielm_InstantLockCallback_t pCallback,
                                          void *pCallbackContext)
{
    ielmLockHashChain_t *pHashChain;
    ielmLockRequest_t *pLRB=NULL;  // Initialise the pointer with NULL
    bool fChainLatched = false;
    int osrc;
    int32_t rc = OK;

    // Calculate the hash chain
    pHashChain = pLM->pLockChains[pLockName->Common.LockType];
    pHashChain += (Hash % pLM->LockTableSize);

    // Latch the hash chain and go looking for the lock
    osrc = pthread_mutex_lock(&pHashChain->Latch);
    if (UNLIKELY(osrc != 0))
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(7, true,
                       "pthread_mutex_lock failed", rc,
                       "osrc", &osrc, sizeof(osrc),
                       NULL);
    }

    if (rc == OK)
    {
        fChainLatched = true;

        // While we have not looked at all of the entries...
        pLRB = pHashChain->pChainHead;
        while (pLRB != NULL)
        {
            // If the hash values match, it's possibly our lock
            if (pLRB->HashValue == Hash)
            {
                if (_local_areLockNamesEqual(pLockName, &pLRB->LockName))
                {
                    rc = ISMRC_LockNotGranted;
                    break;
                }
            }

            // Continue from where we left off
            pLRB = pLRB->pChainNext;
        }
    }

    // If the lock request could be granted, we essentially have
    // the lock until we drop the hash chain latch.
    if (rc == OK)
    {
        //If the caller wants us to peek at some data safe in the
        //knowledge that there's no conflicting lock, do it now
        if (pPeekData != NULL)
        {
            *pPeekValueOut = *pPeekData;
        }
        else if (pCallback != NULL)
        {
            pCallback(pCallbackContext);
        }
    }
    else if (    (rc == ISMRC_LockNotGranted)
              && (ppAR != NULL))
    {
        // If this lock is being released as an atomic operation with
        // the other commit-duration locks in its scope, we register
        // our interest in the lock and prepare to wait for its release
        assert(NULL != pLRB);
        if (pLRB->fBeingReleased)
        {
            ielmAtomicRelease_t *pOwnerAR = pLRB->pAtomicRelease;
            __sync_fetch_and_add(&pOwnerAR->InterestCount, 1);
            *ppAR = pOwnerAR;
        }
        else
        {
            *ppAR = NULL;
        }
    }

    if (fChainLatched)
    {
        (void)pthread_mutex_unlock(&pHashChain->Latch);
    }

    return rc;
}


/*
 * _local_releaseLockInternal
 *  - Releases a lock
 */
void _local_releaseLockInternal(ielmLockManager_t *pLM,
                                ielmLockRequest_t *pLRB)
{
    ielmLockHashChain_t *pHashChain;
    ielmLockRequest_t *pLRBPrev;
    ielmLockRequest_t *pLRBNext;
    int osrc;
    int32_t rc = OK;

    // Extract the hash value and calculate the hash chain
    pHashChain = pLM->pLockChains[pLRB->LockName.Common.LockType];
    pHashChain += (pLRB->HashValue % pLM->LockTableSize);

    // Latch the hash chain and find the lock
    osrc = pthread_mutex_lock(&pHashChain->Latch);
    if (UNLIKELY(osrc != 0))
    {
        rc = ISMRC_Error;
        ism_common_setError(rc);
        ieutTRACE_FFDC(8, true,
                       "pthread_mutex_lock failed", rc,
                       "osrc", &osrc, sizeof(osrc),
                       NULL);
    }

    pLRBPrev = pLRB->pChainPrev;
    pLRBNext = pLRB->pChainNext;

    if (pLRBPrev == NULL)
    {
        pHashChain->pChainHead = pLRBNext;
    }
    else
    {
        pLRBPrev->pChainNext = pLRBNext;
    }

    if (pLRBNext == NULL)
    {
        pHashChain->pChainTail = pLRBPrev;
    }
    else
    {
        pLRBNext->pChainPrev = pLRBPrev;
    }

    pHashChain->HeaderCount--;

    (void)pthread_mutex_unlock(&pHashChain->Latch);

    pLRB->pChainNext = NULL;
    pLRB->pChainPrev = NULL;

    return;
}


/*
 * _local_releaseAllBeginInternal
 *  - Begins release of all locks in a scope
 */
static void _local_releaseAllBeginInternal(ielmLockManager_t *pLM,
                                           ielmLockScope_t *pScope)
{
    ielmLockRequest_t *pLRB;

    // Iterate through the LRBs in the scope and mark the
    // commit-duration ones as being released
    pLRB = pScope->pScopeReqHead;
    while (pLRB != NULL)
    {
        if (pLRB->LockDuration == ielmLOCK_DURATION_COMMIT)
        {
            (void)__sync_lock_test_and_set(&pLRB->fBeingReleased, 1);
        };

        pLRB = pLRB->pScopeReqNext;
    }

    return;
}


/*
 * _local_releaseAllCompleteInternal
 *  - Completes release of all locks in a scope
 */
static void _local_releaseAllCompleteInternal(ieutThreadData_t *pThreadData,
                                              ielmLockManager_t *pLM,
                                              ielmLockScope_t *pScope)
{
    ielmLockRequest_t *pLRB;
    ielmLockRequest_t *pPrevLRB;

    // Iterate through the LRBs in the scope and release them all.
    // We iterate through them BACKWARDS just in case there's a
    // locking hierarchy on top of us which assumes this
    pLRB = pScope->pScopeReqTail;
    while (pLRB != NULL)
    {
        pPrevLRB = pLRB->pScopeReqPrev;
        pLRB->pScopeReqNext = NULL;
        pLRB->pScopeReqPrev = NULL;

        _local_releaseLockInternal(pLM, pLRB);

        if (pScope->pCacheRequest == NULL)
        {
            pScope->pCacheRequest = pLRB;
        }
        else
        {
            iemem_freeStruct(pThreadData, iemem_lockManager, pLRB, pLRB->StrucId);
        }

        pLRB = pPrevLRB;
    }

    pScope->pScopeReqHead = NULL;
    pScope->pScopeReqTail = NULL;
    pScope->RequestCount = 0;

    return;
}

#ifdef LOCKMANAGER_SHOW_HASH_SPREAD
XAPI void ielm_showHashSpread(void)
{
    ielmLockManager_t *pLM;
    ielmLockHashChain_t *pHash;
    ielmLockRequest_t *pLRB;
    uint32_t t;
    uint32_t i;

    pLM = ismEngine_serverGlobal.LockManager;
    if (pLM != NULL)
    {
        for (t = 0; t < ielmNUM_LOCK_TYPES; t++)  /* BEAM suppression: loop doesn't iterate */
        {
            pHash = pLM->pLockChains[t];
            uint32_t ChainLength;

            for (i = 0; i < ielmLOCK_TABLE_SIZE_DEFAULT; i++, pHash++)
            {
                ChainLength = 0;
                (void)pthread_mutex_lock(&pHash->Latch);

                pLRB = pHash->pChainHead;
                while (pLRB != NULL)
                {
                    pLRB = pLRB->pChainNext;
                    ChainLength++;
                }

                (void)pthread_mutex_unlock(&pHash->Latch);

                TRACE(4, "ChainLength[%d][%d] = %d\n", t, i, ChainLength);
            }
        }
    }
}
#endif /* end ifdef LOCKMANAGER_SHOW_HASH_SPREAD */


/*
 * ielm_dumpWriteDescriptions
 *   - Write dump description for lock manager structures
 */
void ielm_dumpWriteDescriptions(iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;

    iedm_describe_ielmAtomicRelease_t(dump->fp);
    iedm_describe_ielmLockRequest_t(dump->fp);
    iedm_describe_ielmLockScope_t(dump->fp);
    iedm_describe_ielmLockHashChain_t(dump->fp);
    iedm_describe_ielmLockManager_t(dump->fp);
}


/*
 * ielm_dumpLocks
 *   - Dump information about a locks to a file
 */
int32_t ielm_dumpLocks(const char *lockName, iedmDumpHandle_t dumpHdl)
{
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;
    ielmLockManager_t *pLM;
    ielmLockHashChain_t *pHash;
    ielmLockRequest_t *pLRB;
    uint32_t t;
    uint32_t i;
    ism_time_t startTime, endTime, diffTime;
    double diffTimeSecs;
    int32_t rc = OK;

    TRACE(ENGINE_CEI_TRACE, FUNCTION_ENTRY "lockName='%s'\n", __func__, lockName);

    // We measure how long these dumps take for information
    startTime = ism_common_currentTimeNanos();

    pLM = ismEngine_serverGlobal.LockManager;
    if (pLM != NULL)
    {
        iedm_dumpStartGroup(dump, "LockManager");

        iedm_dumpData(dump, "ielmLockManager_t", pLM, iemem_usable_size(iemem_lockManager, pLM));

        for (t = 0; t < ielmNUM_LOCK_TYPES; t++)  /* BEAM suppression: loop doesn't iterate */
        {
            pHash = pLM->pLockChains[t];

            for (i = 0; i < ielmLOCK_TABLE_SIZE_DEFAULT; i++, pHash++)
            {
                bool fStartedGroup = false;
                bool fStartedLocksGroup = false;

                (void)pthread_mutex_lock(&pHash->Latch);

                // For dump detail greater than 7, we dump out empty hash chains too
                if ((dump->detailLevel >= iedmDUMP_DETAIL_LEVEL_7) ||
                    (pHash->HeaderCount > 0))
                {
                    char groupName[50];
                    sprintf(groupName, "HashChain[%u]", i);
                    iedm_dumpStartGroup(dump, groupName);
                    fStartedGroup = true;
                    iedm_dumpData(dump, "ielmLockHashChain_t", pHash, sizeof(ielmLockHashChain_t));
                }

                pLRB = pHash->pChainHead;
                while (pLRB != NULL)
                {
                    if (!fStartedLocksGroup)
                    {
                        iedm_dumpStartGroup(dump, "Locks");
                        fStartedLocksGroup = true;
                    }
                    iedm_dumpData(dump, "ielmLockRequest_t", pLRB, iemem_usable_size(iemem_lockManager, pLRB));

                    pLRB = pLRB->pChainNext;
                }

                if (fStartedLocksGroup)
                {
                    iedm_dumpEndGroup(dump);
                }
                if (fStartedGroup)
                {
                    iedm_dumpEndGroup(dump);
                }

                (void)pthread_mutex_unlock(&pHash->Latch);
            }
        }

        iedm_dumpEndGroup(dump);
    }

    endTime = ism_common_currentTimeNanos();

    diffTime = endTime - startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;

    TRACE(ENGINE_HIGH_TRACE, "Dumping locks took %.2f secs (%ldns)\n", diffTimeSecs, diffTime);

    TRACE(ENGINE_CEI_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of lockManager.c                                              */
/*                                                                   */
/*********************************************************************/
