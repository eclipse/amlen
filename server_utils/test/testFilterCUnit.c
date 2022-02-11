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

/*
 * File: testFilterCUnit.c
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ismutil.h>
#include <ismuints.h>
#include "testFilterCUnit.h"

//defined in ismutil.c
void ism_common_initUtil2(int type);

/**
 * Perform initialization for filter test suite.
 */
int initFilterCUnitSuite(void){
    return 0;
}

/**
 * Clean up after filter test suite is executed.
 */
int cleanFilterCUnitSuite(void){
    return 0;
}

#define NULL_PTR ((void**)0)
static void CUnit_acl_readlock_test(void) {
    ism_common_initUtil2(2);
    ism_protocol_lockACLList(false,NULL);
    int wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked != 0);
    int rlocked = ism_common_tryReadLockACLList( );
    CU_ASSERT(rlocked == 0);
    ism_common_unlockACLList(); 
    ism_common_unlockACLList(); 
}

static void CUnit_acl_writelock_test(void) {
    ism_common_initUtil2(2);
    ism_protocol_lockACLList(true,NULL);
    int wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked != 0);
    int rlocked = ism_common_tryReadLockACLList( );
    CU_ASSERT(rlocked != 0);
    ism_common_unlockACLList(); 
}

static void CUint_acl_notPersist_test(void) {
    ism_common_initUtil2(2);
    ismMessageSelectionLockStrategy_t locks;
    locks.rlac = LS_DONT_PERSIST_LOCK;
    ism_protocol_lockACLList(false,&locks);
    CU_ASSERT(locks.rlac == LS_DONT_PERSIST_LOCK);
    int wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked != 0);
    ism_protocol_unlockACLList(&locks);
    CU_ASSERT(locks.rlac == LS_DONT_PERSIST_LOCK);
    wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked == 0);
    ism_common_unlockACLList(); 
}

static void CUint_acl_persistRead_test(void) {
    ism_common_initUtil2(2);
    ismMessageSelectionLockStrategy_t locks;
    locks.rlac = LS_NO_LOCK_HELD;
    ism_protocol_lockACLList(false,&locks);
    CU_ASSERT(locks.rlac == LS_READ_LOCK_HELD);
    int wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked != 0);
    int rlocked = ism_common_tryReadLockACLList( );
    CU_ASSERT(rlocked == 0);
    ism_common_unlockACLList();
    ism_protocol_unlockACLList(&locks);
    CU_ASSERT(locks.rlac == LS_READ_LOCK_HELD);
    wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked != 0);
    rlocked = ism_common_tryReadLockACLList( );
    CU_ASSERT(rlocked == 0);
    ism_common_unlockACLList();
    ism_common_unlockACLList(); 
}

static void CUint_acl_dontPersistWrite_test(void) {
    ism_common_initUtil2(2);
    ismMessageSelectionLockStrategy_t locks;
    locks.rlac = LS_NO_LOCK_HELD;
    ism_protocol_lockACLList(true,&locks);
    CU_ASSERT(locks.rlac == LS_WRITE_LOCK_HELD);
    int wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked != 0);
    int rlocked = ism_common_tryReadLockACLList( );
    CU_ASSERT(rlocked != 0);

    ism_protocol_unlockACLList(&locks);
    CU_ASSERT(locks.rlac == LS_NO_LOCK_HELD);
    wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked == 0);
    ism_common_unlockACLList(); 
}

static void CUint_acl_upgradeReadLock_test(void) {
    ism_common_initUtil2(2);

    //Take read lock and persist
    ismMessageSelectionLockStrategy_t locks;
    locks.rlac = LS_NO_LOCK_HELD;
    ism_protocol_lockACLList(false,&locks);
    CU_ASSERT(locks.rlac == LS_READ_LOCK_HELD);
    int wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked != 0);
    int rlocked = ism_common_tryReadLockACLList( );
    CU_ASSERT(rlocked == 0);
    ism_common_unlockACLList(); 

    //check it persists
    CU_ASSERT(locks.rlac == LS_READ_LOCK_HELD);
    wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked != 0);

    //upgrade to write lock
    ism_protocol_lockACLList(true,&locks);
    CU_ASSERT(locks.rlac == LS_WRITE_LOCK_HELD);
    wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked != 0);
    rlocked = ism_common_tryReadLockACLList( );
    CU_ASSERT(rlocked != 0);

    //release lock
    ism_protocol_unlockACLList(&locks);
    CU_ASSERT(locks.rlac == LS_NO_LOCK_HELD);
    wlocked = ism_common_tryWriteLockACLList( );
    CU_ASSERT(wlocked == 0);
    ism_common_unlockACLList(); 
}

/*
 * Array of hash map tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Util_CUnit_Filter[] =
  {
        { "ACLReadLockTest",     CUnit_acl_readlock_test },
        { "ACLWriteLockTest",    CUnit_acl_writelock_test },
        { "ACLNotPersist",       CUint_acl_notPersist_test },
        { "ACLPersistRead",      CUint_acl_persistRead_test },
        { "ACLDontPersistWrite", CUint_acl_dontPersistWrite_test },
        { "ACLUpgradeRead",      CUint_acl_upgradeReadLock_test },
       CU_TEST_INFO_NULL
  };

