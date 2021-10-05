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
/// @file  lockManagerInternal.h
/// @brief Engine component lock manager internal header file
//*********************************************************************
#ifndef __ISM_LOCKMANAGERINTERNAL_DEFINED
#define __ISM_LOCKMANAGERINTERNAL_DEFINED

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

#define ielmLOCK_TABLE_SIZE_DEFAULT (24593)    ///< The size of the lock tables (number of synonym chains)
#define ielmLOCK_MODE_CLASH ielmNUM_LOCK_MODES ///< A special lock mode which signifies a clash

#define ielmLOCK_HASH_CONSTANT_1 2147483587UL  ///< Constant used in hashing lock names
#define ielmLOCK_HASH_CONSTANT_2 2147483647UL  ///< Constant used in hashing lock names


//****************************************************************************
/// @brief Atomic release block
//****************************************************************************
typedef struct ielmAtomicRelease_t
{
    char                            StrucId[4];                ///< Eyecatcher "ELKA"
    uint32_t                        InterestCount;             ///< Number of other lock scopes with an interest in this block
    pthread_mutex_t                 Latch;                     ///< Latch associated with event
    pthread_cond_t                  Event;                     ///< Event for atomic release of commit-duration locks
    bool                            fEventFired;               ///< Has the atomic release occurred?
} ielmAtomicRelease_t;

#define ielmATOMICRELEASE_STRUCID "ELKA"


// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ielmAtomicRelease_t(__file)\
    iedm_descriptionStart(__file, ielmAtomicRelease_t, StrucId, ielmATOMICRELEASE_STRUCID);\
    iedm_describeMember(char [4],                     StrucId);\
    iedm_describeMember(uint32_t,                     InterestCount);\
    iedm_describeMember(pthread_mutex_t,              Latch);\
    iedm_describeMember(pthread_cond_t,               Event);\
    iedm_describeMember(bool,                         fEventFired);\
    iedm_descriptionEnd;


//****************************************************************************
/// @brief Lock Request Block
//****************************************************************************
typedef struct ielmLockRequest_t
{
    char                            StrucId[4];                ///< Eyecatcher "ELKR"
    uint32_t                        HashValue;                 ///< Hashed lock name for faster search
    ielmLockName_t                  LockName;                  ///< Lock name
    uint32_t                        LockMode;                  ///< Lock mode for this request
    uint32_t                        LockDuration;              ///< Lock duration for this request
    struct ielmLockRequest_t       *pChainNext;                ///< Ptr to next in hash chain
    struct ielmLockRequest_t       *pChainPrev;                ///< Ptr to prev in hash chain
    struct ielmLockRequest_t       *pScopeReqNext;             ///< Handle of next request for scope
    struct ielmLockRequest_t       *pScopeReqPrev;             ///< Handle of prev request for scope
    ielmAtomicRelease_t            *pAtomicRelease;            ///< Ptr to atomic release structure
    bool                            fBeingReleased;            ///< Whether lock being released
} ielmLockRequest_t;

#define ielmLOCKREQUEST_STRUCID "ELKR"


// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ielmLockRequest_t(__file)\
    iedm_descriptionStart(__file, ielmLockRequest_t, StrucId, ielmLOCKREQUEST_STRUCID);\
    iedm_describeMember(char [4],                     StrucId);\
    iedm_describeMember(uint32_t,                     HashValue);\
    iedm_describeMember(ielmLockName_t,               LockName);\
    iedm_describeMember(uint32_t,                     LockMode);\
    iedm_describeMember(uint32_t,                     LockDuration);\
    iedm_describeMember(struct ielmLockRequest_t *,   pChainNext);\
    iedm_describeMember(struct ielmLockRequest_t *,   pChainPrev);\
    iedm_describeMember(struct ielmLockRequest_t *,   pScopeReqNext);\
    iedm_describeMember(struct ielmLockRequest_t *,   pScopeReqPrev);\
    iedm_describeMember(ielmAtomicRelease_t *,        pAtomicRelease);\
    iedm_describeMember(bool,                         fBeingReleased);\
    iedm_descriptionEnd;


//****************************************************************************
/// @brief Lock Scope
//****************************************************************************
typedef struct ielmLockScope_t
{
    char                            StrucId[4];                ///< Eyecatcher "ELKS"
    uint32_t                        RequestCount;              ///< Number of scope's requests
    uint32_t                        CommitDurationCount;       ///< Number of commit-duration locks in this scope
    pthread_spinlock_t              Latch;                     ///< Latch for scope
    ielmLockRequest_t              *pScopeReqHead;             ///< Ptr to head of scope's requests
    ielmLockRequest_t              *pScopeReqTail;             ///< Ptr to tail of scope's requests
    ielmLockRequest_t              *pCacheRequest;             ///< Cached lock request
    ielmAtomicRelease_t            *pCacheAtomicRelease;       ///< Cached atomic release
    ielmAtomicRelease_t            *pCurrentAtomicRelease;     ///< Current atomic release
    bool                            fMemPool;                  ///< Is the memory for the scope part of a memory pool
} ielmLockScope_t;

#define ielmLOCKSCOPE_STRUCID "ELKS"


// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ielmLockScope_t(__file)\
    iedm_descriptionStart(__file, ielmLockScope_t, StrucId, ielmLOCKSCOPE_STRUCID);\
    iedm_describeMember(char [4],                     StrucId);\
    iedm_describeMember(uint32_t,                     RequestCount);\
    iedm_describeMember(uint32_t,                     CommitDurationCount);\
    iedm_describeMember(pthread_spinlock_t,           Latch);\
    iedm_describeMember(ielmLockRequest_t *,          pScopeReqHead);\
    iedm_describeMember(ielmLockRequest_t *,          pScopeReqTail);\
    iedm_describeMember(ielmLockRequest_t *,          pCacheRequest);\
    iedm_describeMember(ielmAtomicRelease_t *,        pCacheAtomicRelease);\
    iedm_describeMember(ielmAtomicRelease_t *,        pCurrentAtomicRelease);\
    iedm_descriptionEnd;


//****************************************************************************
/// @brief Lock Manager Hash Table
//****************************************************************************
typedef struct ielmLockHashChain_t
{
    char                            StrucId[4];                ///< Eyecatcher "ELKC"
    uint32_t                        HeaderCount;               ///< Number of headers in this chain
    pthread_mutex_t                 Latch;                     ///< Latch for hash chain
    ielmLockRequest_t              *pChainHead;                ///< Ptr to head of lock header list
    ielmLockRequest_t              *pChainTail;                ///< Ptr to tail of lock header list
} ielmLockHashChain_t;

#define ielmLOCKHASHCHAIN_STRUCID "ELKC"


// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ielmLockHashChain_t(__file)\
    iedm_descriptionStart(__file, ielmLockHashChain_t, StrucId, ielmLOCKHASHCHAIN_STRUCID);\
    iedm_describeMember(char [4],                     StrucId);\
    iedm_describeMember(uint32_t,                     HeaderCount);\
    iedm_describeMember(pthread_mutex_t,              Latch);\
    iedm_describeMember(ielmLockRequest_t *,          pChainHead);\
    iedm_describeMember(ielmLockRequest_t *,          pChainTail);\
    iedm_descriptionEnd;


//****************************************************************************
/// @brief Lock Manager Global Data
//****************************************************************************
typedef struct ielmLockManager_t
{
    char                            StrucId[4];                ///< Eyecatcher "ELKG"
    uint32_t                        LockTableSize;             ///< Number of synonym chains
    ielmLockHashChain_t            *pLockChains[ielmNUM_LOCK_TYPES]; ///< Memory blocks containing hash chains
} ielmLockManager_t;

#define ielmLOCKMANAGER_STRUCID     "ELKG"
#define ielmLOCKMANAGER_BAD_STRUCID "?LKG"


// Define the members to be described in a dump (can exclude & reorder members)
#define iedm_describe_ielmLockManager_t(__file)\
    iedm_descriptionStart(__file, ielmLockManager_t, StrucId, ielmLOCKMANAGER_STRUCID);\
    iedm_describeMember(char [4],                     StrucId);\
    iedm_describeMember(uint32_t,                     LockTableSize);\
    iedm_describeMember(ielmLockHashChain_t *,        pLockChains);\
    iedm_descriptionEnd;


#endif /* __ISM_LOCKMANAGERINTERNAL_DEFINED */
