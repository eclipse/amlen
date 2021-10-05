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

/*********************************************************************/
/*                                                                   */
/* Module Name: test_transaction                                     */
/*                                                                   */
/* Description: CUnit tests of Global Transactions                   */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "queueNamespace.h"
#include "transaction.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_file.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
static ismEngine_ClientStateHandle_t hGlobalClient = NULL;

/*********************************************************************/
/* Test the debug routines                                           */
/*********************************************************************/
void test_dumpTransactions(ism_xid_t *pXID, bool show)
{
    // Run Various debug routines, hijacking stdout
    int origStdout = 0;

    if (!show)
    {
        origStdout = test_redirectStdout("/dev/null");
        TEST_ASSERT_NOT_EQUAL(origStdout, -1);
    }

    // Test debugging routines with some locks
    if (origStdout != -1)
    {
        char debugFile[10] = "";
        int32_t rc = ism_engine_dumpTransactions(NULL, 6, 0, debugFile);
        TEST_ASSERT_EQUAL(rc, OK);

        if (pXID != NULL)
        {
            char XIDBuffer[80];
            ism_common_xidToString(pXID, XIDBuffer, 80);
            rc = ism_engine_dumpTransactions(XIDBuffer, 9, 50, debugFile);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    if (!show)
    {
        origStdout= test_reinstateStdout(origStdout);
        TEST_ASSERT_NOT_EQUAL(origStdout, -1);
    }
}

/*********************************************************************/
/* Override ism_store_createRecord to allow forcing errors           */
/*********************************************************************/
static int32_t ism_store_createRecord_FORCE_ERROR = OK;
int32_t ism_store_createRecord(ismStore_StreamHandle_t hStream,
                               const ismStore_Record_t *pRecord,
                               ismStore_Handle_t *pHandle)
{
    int32_t rc = ism_store_createRecord_FORCE_ERROR;

    static int32_t (*real_ism_store_createRecord)(ismStore_StreamHandle_t, const ismStore_Record_t *, ismStore_Handle_t *) = NULL;

    if (real_ism_store_createRecord == NULL)
    {
        real_ism_store_createRecord = dlsym(RTLD_NEXT, "ism_store_createRecord");
    }

    if (rc == OK) rc = real_ism_store_createRecord(hStream, pRecord, pHandle);

    return rc;
}

/*********************************************************************/
/* Override ism_store_openReferenceContext to allow forcing errors   */
/*********************************************************************/
static int32_t ism_store_openReferenceContext_FORCE_ERROR = OK;
int32_t ism_store_openReferenceContext(ismStore_Handle_t hOwnerHandle,
                                       ismStore_ReferenceStatistics_t *pRefStats,
                                       void **phRefCtxt)
{
    int32_t rc = ism_store_openReferenceContext_FORCE_ERROR;
    static int32_t (*real_ism_store_openReferenceContext)(ismStore_Handle_t, ismStore_ReferenceStatistics_t *, void **) = NULL;

    if (real_ism_store_openReferenceContext == NULL)
    {
        real_ism_store_openReferenceContext = dlsym(RTLD_NEXT, "ism_store_openReferenceContext");
    }

    if (rc == OK) rc = real_ism_store_openReferenceContext(hOwnerHandle, pRefStats, phRefCtxt);

    return rc;
}

/*********************************************************************/
/* TestCase:       GlobalTran_SuspendResume                          */
/* Description:    Test the interaction of TMSUSPEND and TMRESUME on */
/*                 multiple different clients and sessions.          */
/*********************************************************************/
void GlobalTran_SuspendResume( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t hSession1A, hSession1B, hSession2;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    ismEngine_TransactionHandle_t hNoise[10] = {0};
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    ism_xid_t *XIDBuffer = (ism_xid_t *)calloc(sizeof(ism_xid_t), 2);
    TEST_ASSERT_PTR_NOT_NULL(XIDBuffer);

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting GlobalTran_SuspendResume...");

    // Create an initial client and session
    rc = test_createClientAndSession("CLIENT1",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient1,
                                     &hSession1A,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession("CLIENT2",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient2,
                                     &hSession2,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create additional session on client 1 for this test
    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                  &hSession1B,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc , OK);

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x57415344;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x8877665544332211;
    globalId.bqual = 0x11FFEEDDCCBBAA99;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    // Try resuming a non-existent transaction on Session1A
    rc = sync_ism_engine_createGlobalTransaction(hSession1A,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMRESUME,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    rc = sync_ism_engine_createGlobalTransaction(hSession1A,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran);

    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;

    TEST_ASSERT_EQUAL(pTran->TranFlags & ietrTRAN_FLAG_GLOBAL, ietrTRAN_FLAG_GLOBAL);

    // Try and create the transaction when it already exists
    rc = sync_ism_engine_createGlobalTransaction(hSession1A,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, ISMRC_TransactionInUse);

    // Add a random number of local and global transactions for 'noise'
    int32_t noiseTrans = rand()%((sizeof(hNoise)/sizeof(hNoise[0]))/2);

    if (noiseTrans)
    {
        for(int32_t i=0; i<noiseTrans; i++)
        {
            // Local
            rc = sync_ism_engine_createLocalTransaction(hSession1A, &hNoise[i]);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hNoise[i]);

            // Global
            globalId.bqual += 1;
            memcpy(&XID.data, &globalId, sizeof(globalId));
            rc = sync_ism_engine_createGlobalTransaction(hSession1A,
                                                    &XID,
                                                    ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                    &hNoise[((sizeof(hNoise)/sizeof(hNoise[0]))/2)+i]);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hNoise[((sizeof(hNoise)/sizeof(hNoise[0]))/2)+i]);
        }

        globalId.bqual -= noiseTrans;
        memcpy(&XID.data, &globalId, sizeof(globalId));
    }

    for(int32_t i=0; i<4; i++)
    {
        // Try resuming it while it is in use on another session using both
        // explicit resume option and default
        rc = sync_ism_engine_createGlobalTransaction(i<2 ? hSession1B : hSession2,
                                                &XID,
                                                (i%2 == 0) ? ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMRESUME :
                                                             ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT,
                                                &hTran);
        if (i%2 == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_TransactionInUse);
        }
    }

    // Suspend the transaction
    rc = ism_engine_endTransaction(hSession1A,
                                   hTran,
                                   ismENGINE_END_TRANSACTION_OPTION_XA_TMSUSPEND,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(pTran->TranFlags & ietrTRAN_FLAG_SUSPENDED, ietrTRAN_FLAG_SUSPENDED);

    test_dumpTransactions(&XID, false);

    // Try resuming it on different sessions
    for(int32_t i=0; i<2; i++)
    {
        rc = sync_ism_engine_createGlobalTransaction(i==0 ? hSession2 : hSession1B,
                                                &XID,
                                                ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMRESUME,
                                                &hTran);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        TEST_ASSERT_EQUAL(pTran->TranFlags & ietrTRAN_FLAG_SUSPENDED, ietrTRAN_FLAG_SUSPENDED);
    }

    // Try resuming it on the correct session
    rc = sync_ism_engine_createGlobalTransaction(hSession1A,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMRESUME,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(hTran, pTran); // Same transaction
    TEST_ASSERT_NOT_EQUAL(pTran->TranFlags & ietrTRAN_FLAG_SUSPENDED, ietrTRAN_FLAG_SUSPENDED);

    test_dumpTransactions(&XID, false);

    // Suspend the transaction
    rc = ism_engine_endTransaction(hSession1A,
                                   hTran,
                                   ismENGINE_END_TRANSACTION_OPTION_XA_TMSUSPEND,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(pTran->TranFlags & ietrTRAN_FLAG_SUSPENDED, ietrTRAN_FLAG_SUSPENDED);

    // Try resuming it on the correct session
    rc = sync_ism_engine_createGlobalTransaction(hSession1A,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMRESUME,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(hTran, pTran); // Same transaction
    TEST_ASSERT_NOT_EQUAL(pTran->TranFlags & ietrTRAN_FLAG_SUSPENDED, ietrTRAN_FLAG_SUSPENDED);

    rc = ism_engine_destroySession(hSession1A, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Transaction should have been rolled back, so try and create the same XID on Session2
    rc = sync_ism_engine_createGlobalTransaction(hSession2,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    // Try and move the transaction that's still bound to a different session
    rc = sync_ism_engine_createGlobalTransaction(hSession1B,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, ISMRC_TransactionInUse);

    // End the transaction
    rc = ism_engine_endTransaction(hSession2,
                                   hTran,
                                   ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
								   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Move the transaction to session 1B using OPTION_DEFAULT
    // which _allows_ moving of unbound transaction
    rc = sync_ism_engine_createGlobalTransaction(hSession1B,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient2, hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // prepare and commit on session 1B
    rc =sync_ism_engine_prepareGlobalTransaction(hSession1B, &XID);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_commitTransaction(hSession1B,
                                           hTran,
                                           ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the transaction is no longer in the transaction table
    do
    {
        rc = ism_engine_commitGlobalTransaction(hSession1B,
                                                &XID,
                                                ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                NULL, 0, NULL);
    }
    while(rc == ISMRC_InvalidOperation);

    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    rc = test_destroyClientAndSession(hClient1, hSession1B, false);
    TEST_ASSERT_EQUAL(rc, OK);

    free(XIDBuffer);

    return;
}

/*********************************************************************/
/* TestCase:       GlobalTran_Fail                                   */
/* Description:    Test the use of TMFAIL when ending a transaction. */
/*********************************************************************/
void GlobalTran_Fail( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    ism_xid_t *XIDBuffer = (ism_xid_t *)calloc(sizeof(ism_xid_t), 2);
    TEST_ASSERT_PTR_NOT_NULL(XIDBuffer);

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create an initial client and session
    rc = test_createClientAndSession("CLIENT_FAIL",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient1,
                                     &hSession1,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0xC1D1E1F1;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0xFEDCBA9876543210;
    globalId.bqual = 0x0123456789ABCDEF;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    // Create a transaction
    rc = sync_ism_engine_createGlobalTransaction(hSession1,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran);

    // End the transaction as failed
    rc = ism_engine_endTransaction(hSession1, hTran, ismENGINE_END_TRANSACTION_OPTION_XA_TMFAIL, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_dumpTransactions(&XID, false);

    // Prepare the transaction
    rc = sync_ism_engine_prepareGlobalTransaction(hSession1, &XID);
    TEST_ASSERT_EQUAL(rc, ISMRC_RolledBack);

    // Create a new transaction
    rc = sync_ism_engine_createGlobalTransaction(hSession1,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    // End the transaction as failed
    rc = ism_engine_endTransaction(hSession1, hTran, ismENGINE_END_TRANSACTION_OPTION_XA_TMFAIL, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Commit the transaction as a ONEPHASE
    rc = ism_engine_commitTransaction(hSession1,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMONEPHASE,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_RolledBack);

    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    free(XIDBuffer);

    return;
}

/*********************************************************************/
/* TestCase:       GlobalTran_OnePhase                               */
/* Description:    Test the use of TMONEPHASE when committing.       */
/*********************************************************************/
void GlobalTran_OnePhase( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    ism_xid_t *XIDBuffer = (ism_xid_t *)calloc(sizeof(ism_xid_t), 2);
    TEST_ASSERT_PTR_NOT_NULL(XIDBuffer);

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create an initial client and session
    rc = test_createClientAndSession("CLIENT_ONEPHASE",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient1,
                                     &hSession1,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x1CD22EE1;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x0001;
    globalId.bqual = 0x2000;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    // Create a transaction
    rc = sync_ism_engine_createGlobalTransaction(hSession1,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran);

    test_dumpTransactions(&XID, false);

    // Try and commit the transaction which is not ended or prepared
    rc = ism_engine_commitTransaction(hSession1,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

    // End the transaction
    rc = ism_engine_endTransaction(hSession1,
                                   hTran,
                                   ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Try and commit the transaction which is not prepared
    rc = ism_engine_commitTransaction(hSession1,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

    // Commit the unprepared transaction using ONEPHASE
    rc = sync_ism_engine_commitTransaction(hSession1,
                                           hTran,
                                           ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMONEPHASE);
    TEST_ASSERT_EQUAL(rc, OK);

    do
    {
        // Check that the transaction is no longer in the transaction table
        rc = ism_engine_commitGlobalTransaction(hSession1,
                                                &XID,
                                                ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMONEPHASE,
                                                NULL, 0, NULL);
    }
    while(rc == ISMRC_InvalidOperation);

    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    free(XIDBuffer);

    return;
}

/*********************************************************************/
/* TestCase:       GlobalTran_XARecover                              */
/* Description:    Test the ism_engine_XARecover function.           */
/*********************************************************************/
void GlobalTran_XARecover( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1, hSession2;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran[200];
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create an initial client and session
    rc = test_createClientAndSession("CLIENT_XARECOVER",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient1,
                                     &hSession1,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                  &hSession2,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 17;

    // Create global transactions
    for(int32_t i=0; i< (sizeof(hTran)/sizeof(hTran[0])); i++)
    {
        globalId.bqual = i;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        // Create a transaction
        rc = sync_ism_engine_createGlobalTransaction(hSession1,
                                                &XID,
                                                ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                &hTran[i]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hTran[i]);
    }

    test_dumpTransactions(&XID, false);

    ism_xid_t XIDArray[20] = {{0}};

    TEST_ASSERT_PTR_NULL(((ismEngine_Session_t *)hSession1)->pXARecoverIterator);

    // None are yet prepared (using implicit start)
    uint32_t found = ism_engine_XARecover(hSession1,
                                          XIDArray,
                                          sizeof(XIDArray)/sizeof(XIDArray[0]),
                                          0,
                                          ismENGINE_XARECOVER_OPTION_XA_TMNOFLAGS);
    TEST_ASSERT_EQUAL(found, 0);
    TEST_ASSERT_PTR_NULL(((ismEngine_Session_t *)hSession1)->pXARecoverIterator);

    // Explicit end rescan when one is not in progress
    found = ism_engine_XARecover(hSession1,
                                 XIDArray,
                                 sizeof(XIDArray)/sizeof(XIDArray[0]),
                                 0,
                                 ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN);
    TEST_ASSERT_EQUAL(found, 0);
    TEST_ASSERT_PTR_NULL(((ismEngine_Session_t *)hSession1)->pXARecoverIterator);

    // End and prepare half of the transactions
    for(int32_t i=0; i< (sizeof(hTran)/sizeof(hTran[0]))/2; i++)
    {
        // End the transaction
        rc = ism_engine_endTransaction(hSession1,
                                       hTran[i],
                                       ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Dreadful cheat to get the XID
        ism_xid_t *pXID = ((ismEngine_Transaction_t *)hTran[i])->pXID;

        rc = sync_ism_engine_prepareGlobalTransaction(hSession1, pXID);
        TEST_ASSERT_EQUAL(rc, OK);

        // Try double-preparing a subset
        if (i<10)
        {
            rc = ism_engine_prepareGlobalTransaction(hSession1, pXID, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);
        }
    }

    // Use an explicit start for Session1
    found = ism_engine_XARecover(hSession1,
                                 XIDArray,
                                 sizeof(XIDArray)/sizeof(XIDArray[0]),
                                 0,
                                 ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN);
    TEST_ASSERT_EQUAL(found, sizeof(XIDArray)/sizeof(XIDArray[0]));
    TEST_ASSERT_PTR_NOT_NULL(((ismEngine_Session_t *)hSession1)->pXARecoverIterator);

    // End and prepare 2nd half of the transactions
    for(int32_t i=0; i<(sizeof(hTran)/sizeof(hTran[0]))/2; i++)
    {
        int32_t index = ((sizeof(hTran)/sizeof(hTran[0]))/2)+i;

        // End the transaction
        rc = ism_engine_endTransaction(hSession1,
                                       hTran[index],
                                       ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Dreadful cheat to get the XID
        ism_xid_t *pXID = ((ismEngine_Transaction_t *)hTran[index])->pXID;

        // Note we prepare using Session2
        rc = sync_ism_engine_prepareGlobalTransaction(hSession2, pXID);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    ism_xid_t BigXIDArray[(sizeof(hTran)/sizeof(hTran[0]))+10] = {{0}};

    TEST_ASSERT_PTR_NULL(((ismEngine_Session_t *)hSession2)->pXARecoverIterator);

    // Get a list on session 2 (using implicit start)
    found = ism_engine_XARecover(hSession2,
                                 BigXIDArray,
                                 sizeof(BigXIDArray)/sizeof(BigXIDArray[0]),
                                 0,
                                 ismENGINE_XARECOVER_OPTION_XA_TMNOFLAGS);
    TEST_ASSERT_EQUAL(found, (sizeof(hTran)/sizeof(hTran[0])));
    TEST_ASSERT_PTR_NULL(((ismEngine_Session_t *)hSession2)->pXARecoverIterator);

    for(int32_t i=0; i<(sizeof(hTran)/sizeof(hTran[0])); i++)
    {
        rc = sync_ism_engine_commitTransaction(hSession1,
                                          hTran[i],
                                          ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Get the next set from session1 (these XIDs have all now been committed,
    // but the recovery scan should still be valid)
    found = ism_engine_XARecover(hSession1,
                                 XIDArray,
                                 sizeof(XIDArray)/sizeof(XIDArray[0]),
                                 0,
                                 ismENGINE_XARECOVER_OPTION_XA_TMNOFLAGS);
    TEST_ASSERT_EQUAL(found, sizeof(XIDArray)/sizeof(XIDArray[0]));
    TEST_ASSERT_PTR_NOT_NULL(((ismEngine_Session_t *)hSession1)->pXARecoverIterator);

    // Destroy session1, driving the code to destroy an active iterator (can't test for it)
    rc = ism_engine_destroySession(hSession1, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient1, hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestCase:       GlobalTran_Errors                                 */
/* Description:    Test Errors checked for using global txns         */
/*                 Note: This function does unnatural things to the  */
/*                       transaction record to force error paths.    */
/*********************************************************************/
void GlobalTran_Errors( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t hSession1, hSession2;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create a client and session
    rc = test_createClientAndSession("CLIENT_ERRORS",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient1,
                                     &hSession1,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = -1;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x953122DDDDDD43E1;
    globalId.bqual = 0xABCDEFABCD456789;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    // Create a global transaction with invalid formatId
    hTran = NULL;
    rc = sync_ism_engine_createGlobalTransaction(hSession1,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);
    TEST_ASSERT_PTR_NULL(hTran);

    // Invalid XID length (>XID_DATASIZE)
    XID.formatID = 0x12C13311;
    XID.gtrid_length = XID_DATASIZE;

    rc = sync_ism_engine_createGlobalTransaction(hSession1,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);
    TEST_ASSERT_PTR_NULL(hTran);

    XID.gtrid_length = sizeof(uint64_t);

    rc = sync_ism_engine_createGlobalTransaction(hSession1,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                            &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    // Put the transaction into an odd state and try committing it
    hTran->TranState = 99;
    rc = ism_engine_commitTransaction(hSession1,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

    // Try and commit with a 2nd client while it is still linked (invalidly) to the 1st
    rc = test_createClientAndSession("CLIENT_2_ERRORS",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient2,
                                     &hSession2,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    hTran->TranState = ismTRANSACTION_STATE_PREPARED;
    rc = ism_engine_commitTransaction(hSession2,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_TransactionInUse);

    rc = test_destroyClientAndSession(hClient2, hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);

    hTran->TranState = ismTRANSACTION_STATE_PREPARED;
    hTran->CompletionStage = ietrCOMPLETION_STARTED;

    rc = ism_engine_commitTransaction(hSession1,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

    hTran->TranState = ismTRANSACTION_STATE_IN_FLIGHT;

    rc = ism_engine_rollbackTransaction(hSession1,
                                        hTran,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

    hTran->TranState = ismTRANSACTION_STATE_HEURISTIC_COMMIT;
    hTran->CompletionStage = ietrCOMPLETION_ENDED;

    rc = sync_ism_engine_forgetGlobalTransaction(&XID);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient1, hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestSuite:      ISM_Engine_Global_XATransactions                  */
/* Description:    Tests for global transaction XA options           */
/*********************************************************************/
CU_TestInfo ISM_Engine_Global_XATransactions[] = {
    { "SuspendResume", GlobalTran_SuspendResume },
    { "Fail", GlobalTran_Fail },
    { "OnePhase", GlobalTran_OnePhase },
    { "XARecover", GlobalTran_XARecover },
    { "Errors", GlobalTran_Errors },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestCase:       GlobalTran_Simple1                                */
/* Description:    Simple global transaction test. Create a session  */
/*                 Create 50 global transactions, and call the debug */
/*                 routine to print out the debug info.              */
/*********************************************************************/
void GlobalTran_Simple1( void )
{
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    uint64_t startNumber = 0;
    uint64_t counter;
    const uint64_t transactionCount = 50;
    ismEngine_TransactionHandle_t hTrans[transactionCount];
    struct 
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    uint32_t XIDCount;
    ism_xid_t *XIDBuffer = (ism_xid_t *)calloc(sizeof(ism_xid_t), transactionCount +1);

    TEST_ASSERT(XIDBuffer != NULL, ("XIDBuffer was NULL"));


    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting GlobalTran_Simple1...");

    // Create Session for this test
    rc=ism_engine_createSession( hGlobalClient
                               , ismENGINE_CREATE_SESSION_TRANSACTIONAL
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc , OK);

    test_log(testLOGLEVEL_TESTPROGRESS, "Creating transactions");
    for (counter = 0; counter < transactionCount; counter++)
    {
        test_log(testLOGLEVEL_VERBOSE, "Creating transaction %d", counter);

        memset(&XID, 0, sizeof(ism_xid_t));
        XID.formatID = 0;
        XID.gtrid_length = sizeof(uint64_t);
        XID.bqual_length = sizeof(uint64_t);
        globalId.gtrid = counter;
        globalId.bqual = counter % 4;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        rc=sync_ism_engine_createGlobalTransaction( hSession
                                             , &XID
                                             , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                             , &hTrans[counter]);
        TEST_ASSERT_EQUAL(rc , OK);
    }

    // Call recover, we do not expect any in-flight transactions
    test_log(testLOGLEVEL_TESTPROGRESS, "Calling XARecover before prepare");
    XIDCount = ism_engine_XARecover( hSession
                                   , XIDBuffer
                                   , transactionCount *2
                                   , 1
                                   , ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN );
    TEST_ASSERT(XIDCount ==  0, ("Expected zero Prepared transactions"));
    test_log(testLOGLEVEL_VERBOSE, "Verified that there are zero prepared transactions");

    // ietr_debugXIDs(ismDEBUGOPT_STDOUT | ismDEBUGVERBOSITY(9), NULL);

    test_log(testLOGLEVEL_TESTPROGRESS, "Preparing transactions");

    // Prepare the transactions, note: We are doing this without ending to test one
    // route through the code.
    for (counter = 0; counter < transactionCount; counter++)
    {
        test_log(testLOGLEVEL_TESTPROGRESS, "Preparing transaction %d", counter);

        // Dreadful cheat to get the XID
        ism_xid_t *pXID = ((ismEngine_Transaction_t *)hTrans[counter])->pXID;

        rc=sync_ism_engine_prepareGlobalTransaction( hSession
                                                   , pXID);
        TEST_ASSERT_EQUAL(rc , OK);
    }
    // Call recover, we expect transactionCount prepared transactions
    test_log(testLOGLEVEL_TESTPROGRESS, "Calling XARecover after prepare");
    XIDCount = ism_engine_XARecover( hSession
                                   , XIDBuffer
                                   , transactionCount +1
                                   , 1
                                   , ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN |
                                     ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN);
    // ietr_debugXIDs(ismDEBUGOPT_STDOUT | ismDEBUGVERBOSITY(9), NULL);
    TEST_ASSERT(XIDCount == transactionCount, ("Expected %d prepared transactions got %d", transactionCount, XIDCount));
    test_log(testLOGLEVEL_VERBOSE, "Verified that there are %d prepared transactions", XIDCount);

    for (counter = 0; counter < transactionCount; counter++)
    {
        test_log(testLOGLEVEL_TESTPROGRESS, "Committing transaction %d", counter);
        rc=sync_ism_engine_commitTransaction( hSession
                                       , hTrans[counter]
                                       , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
        TEST_ASSERT_EQUAL(rc , OK);
    }

    // Call recover, we do not expect any in-flight transactions
    test_log(testLOGLEVEL_TESTPROGRESS, "Calling XARecover before prepare");
    XIDCount = ism_engine_XARecover( hSession
                                   , XIDBuffer
                                   , transactionCount +1
                                   , 1
                                   , ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN );
    TEST_ASSERT(XIDCount ==  0, ("Expected zero Prepared transactions"));
    test_log(testLOGLEVEL_VERBOSE, "Verified that there are zero prepared transactions");

    startNumber += transactionCount;

    test_log(testLOGLEVEL_TESTPROGRESS, "Creating transactions");
    for (counter = 0; counter < transactionCount; counter++)
    {
        test_log(testLOGLEVEL_VERBOSE, "Creating transaction %d", counter);

        memset(&XID, 0, sizeof(ism_xid_t));
        XID.formatID = 0;
        XID.gtrid_length = sizeof(uint64_t);
        XID.bqual_length = sizeof(uint64_t);
        globalId.gtrid = counter + startNumber;
        globalId.bqual = counter % 4;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        rc=sync_ism_engine_createGlobalTransaction( hSession
                                             , &XID
                                             , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                             , &hTrans[counter]);
        TEST_ASSERT_EQUAL(rc , OK);
    }

    // Call recover, we do not expect any in-flight transactions
    test_log(testLOGLEVEL_TESTPROGRESS, "Calling XARecover before prepare");
    XIDCount = ism_engine_XARecover( hSession
                                   , NULL
                                   , 0
                                   , 1
                                   , ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN );
    TEST_ASSERT(XIDCount ==  0, ("Expected zero Prepared transactions"));
    test_log(testLOGLEVEL_VERBOSE, "Verified that there are zero prepared transactions");

    test_log(testLOGLEVEL_TESTPROGRESS, "Rolling back transactions");
    for (counter = 0; counter < transactionCount; counter++)
    {
        test_log(testLOGLEVEL_VERBOSE, "Rolling back transaction %d.", counter);

        rc=ism_engine_rollbackTransaction( hSession
                                         , hTrans[counter]
                                         , NULL
                                         , 0
                                         , NULL );
        TEST_ASSERT_EQUAL(rc , OK);
    }

    test_dumpTransactions(NULL, false);

    // Call recover, we do not expect any in-flight transactions
    test_log(testLOGLEVEL_TESTPROGRESS, "Calling XARecover before prepare");
    XIDCount = ism_engine_XARecover( hSession
                                   , XIDBuffer
                                   , transactionCount +1
                                   , 1
                                   , ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN );
    TEST_ASSERT(XIDCount ==  0, ("Expected zero Prepared transactions"));
    test_log(testLOGLEVEL_VERBOSE, "Verified that there are zero prepared transactions");

    test_log(testLOGLEVEL_TESTPROGRESS, "Completed GlobalTran_Simple1");

    if (XIDBuffer != NULL)
    {
        free(XIDBuffer);
    }

    // Explicit creation of a global transaction with no Async
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    TEST_ASSERT_NOT_EQUAL(pThreadData, NULL);
    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 999;
    globalId.bqual = 999 % 4;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    ismEngine_TransactionHandle_t hSyncTran;
    rc = ietr_createGlobal(pThreadData,
                           hSession,
                           &XID,
                           ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT,
                           NULL,
                           &hSyncTran);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hSyncTran);

    rc = ism_engine_rollbackTransaction( hSession
                                       , hSyncTran
                                       , NULL
                                       , 0
                                       , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestCase:       GlobalTran_WrongSessionCommit                     */
/* Description:    This testcase verifies that an attempt to commit  */
/*                 or rollback a transaction which is currently      */
/*                 bound to a different session is rejected.         */
/*********************************************************************/
void GlobalTran_WrongSessionCommit( void )
{
    uint32_t rc;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_ClientStateHandle_t hClient2 = NULL;
    ismEngine_SessionHandle_t hSession2;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct 
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    uint32_t XIDCount;
    uint32_t XIDBufferCapacity = 6;
    ism_xid_t *XIDBuffer = (ism_xid_t *)calloc(sizeof(ism_xid_t), XIDBufferCapacity);

    TEST_ASSERT(XIDBuffer != NULL, ("XIDBuffer was NULL"));

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting GlobalTran_WrongSessionCommit...");

    // Create initial session for this test
    rc=ism_engine_createSession( hGlobalClient
                               , ismENGINE_CREATE_SESSION_TRANSACTIONAL
                               , &hSession1
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc , OK);

    test_log(testLOGLEVEL_TESTPROGRESS, "Creating transaction 1");

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 1;
    globalId.bqual = 1;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc=sync_ism_engine_createGlobalTransaction( hSession1
                                         , &XID
                                         , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                         , &hTran);
    TEST_ASSERT_EQUAL(rc , OK);

    // Call recover, we do not expect any in-flight transactions
    test_log(testLOGLEVEL_TESTPROGRESS, "Calling XARecover before prepare");
    XIDCount = ism_engine_XARecover( hSession1
                                   , XIDBuffer
                                   , XIDBufferCapacity
                                   , 1
                                   , ismENGINE_XARECOVER_OPTION_XA_TMENDRSCAN );
    TEST_ASSERT(XIDCount ==  0, ("Expected zero Prepared transactions"));
    test_log(testLOGLEVEL_VERBOSE, "Verified that there are zero prepared transactions");

    free(XIDBuffer);

    // ietr_debugXIDs(ismDEBUGOPT_STDOUT | ismDEBUGVERBOSITY(9), NULL);

    // Create a second session
    rc = ism_engine_createClientState("Global Transaction Test-2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc , OK);

    rc=ism_engine_createSession( hClient2
                               , ismENGINE_CREATE_SESSION_TRANSACTIONAL
                               , &hSession2
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc , OK);

    // TEST 1 - attempt to prepare the transaction which is bound to a 
    //          different session. This should fail
    rc=ism_engine_prepareGlobalTransaction( hSession2
                                          , &XID
                                          , NULL
                                          , 0
                                          , NULL );
    TEST_ASSERT_EQUAL(rc , ISMRC_TransactionInUse);   
    test_log(testLOGLEVEL_TESTPROGRESS, "Prepare transaction using incorrect session completed with rc=%d", rc);

    // TEST 2 - Dis-associate the transaction from the original session
    //          and then try and prepeare using new client/session

    // Disassociate the transaction from the original session
    rc=ism_engine_endTransaction( hSession1
                                , hTran
                                , ismENGINE_END_TRANSACTION_OPTION_DEFAULT
                                , NULL
                                , 0
                                , NULL );
    TEST_ASSERT_EQUAL(rc , OK);
    test_log(testLOGLEVEL_TESTPROGRESS, "Disassociated transaction from session completed with rc=%d", rc);

    rc=sync_ism_engine_prepareGlobalTransaction( hSession2
                                               , &XID);
    TEST_ASSERT_EQUAL(rc, OK);   
    test_log(testLOGLEVEL_TESTPROGRESS, "Prepare transaction from session2 completed with rc=%d", rc);

    rc=sync_ism_engine_createGlobalTransaction( hSession2
                                         , &XID
                                         , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                         , &hTran);
    TEST_ASSERT_EQUAL(rc , OK);
    test_log(testLOGLEVEL_TESTPROGRESS, "Associated the transaction with the session2 completed with  rc=%d", rc);


    rc=ism_engine_rollbackTransaction( hSession1
                                     , hTran
                                     , NULL
                                     , 0
                                     , NULL );
    TEST_ASSERT_EQUAL(rc , ISMRC_TransactionInUse);
    test_log(testLOGLEVEL_TESTPROGRESS, "Rollback transaction from session1 completed with rc=%d", rc);

    rc=ism_engine_rollbackTransaction( hSession2
                                     , hTran
                                     , NULL
                                     , 0
                                     , NULL );
    TEST_ASSERT_EQUAL(rc , OK);
    test_log(testLOGLEVEL_TESTPROGRESS, "Rollback transaction from session2 completed with rc=%d", rc);

    rc = ism_engine_destroySession( hSession1
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hSession2
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestCase:       GlobalTran_RollbackGet                            */
/* Description:    This testcase verifies that the rollback of get   */
/*                 releases to the messages to any eligible          */
/*                 consumers.                                        */
/*********************************************************************/
typedef struct RollbackGetContext_t
{
    ismEngine_SessionHandle_t hSession;
    char *qname;
    uint32_t messageCount;
    uint32_t messagesReceived;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    bool tranActive;
    uint64_t tranId;
    uint64_t branchId;
} RollbackGetContext_t;

bool RollbackGetConsumer( ismEngine_ConsumerHandle_t      hConsumer
                        , ismEngine_DeliveryHandle_t      hDelivery
                        , ismEngine_MessageHandle_t       hMessage
                        , uint32_t                        deliveryId
                        , ismMessageState_t               state
                        , uint32_t                        destinationOptions
                        , ismMessageHeader_t *            pMsgDetails
                        , uint8_t                         areaCount
                        , ismMessageAreaType_t            areaTypes[areaCount]
                        , size_t                          areaLengths[areaCount]
                        , void *                          pAreaData[areaCount]
                        , void *                          pConsumerContext)
{
    int32_t rc;
    RollbackGetContext_t *pContext = *(RollbackGetContext_t **)pConsumerContext;

    test_log(testLOGLEVEL_VERBOSE, "Received message: %s", pAreaData[0]);

    if (pContext->hTran == NULL)
    {
        struct 
        {
            uint64_t gtrid;
            uint64_t bqual;
        } globalId;

        // If this is the first message received then we need to create 
        // a transaction.

        memset(&pContext->XID, 0, sizeof(ism_xid_t));
        pContext->XID.formatID = 0;
        pContext->XID.gtrid_length = sizeof(uint64_t);
        pContext->XID.bqual_length = sizeof(uint64_t);
        globalId.gtrid = pContext->tranId;
        globalId.bqual = ++pContext->branchId;
        memcpy(&pContext->XID.data, &globalId, sizeof(globalId));
        
        rc=sync_ism_engine_createGlobalTransaction( pContext->hSession
                                             , &pContext->XID
                                             , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                             , &pContext->hTran );
        pContext->tranActive = true;

        test_log(testLOGLEVEL_TESTPROGRESS, "Created global tran %ld.%ld",
                 pContext->tranId, pContext->branchId);

        TEST_ASSERT_EQUAL(rc , OK);
    }

    TEST_ASSERT(pContext->messagesReceived < pContext->messageCount,
                ("Too many messages received. got %d expected %d",
                 pContext->messagesReceived+1, pContext->messageCount));

    pContext->messagesReceived++;

    if (hDelivery != 0)
    {
        rc = ism_engine_confirmMessageDelivery( pContext->hSession
                                              , pContext->hTran
                                              , hDelivery
                                              , ismENGINE_CONFIRM_OPTION_CONSUMED
                                              , NULL
                                              , 0
                                              , NULL );
        TEST_ASSERT_EQUAL(rc, OK);
    }

    ism_engine_releaseMessage(hMessage);

    if (pContext->messagesReceived == pContext->messageCount)
    {
        // We have received as many messages as we expect, so give up 
        // access to the transaction.
        rc = ism_engine_endTransaction( pContext->hSession
                                      , pContext->hTran
                                      , ismENGINE_END_TRANSACTION_OPTION_DEFAULT
                                      , NULL
                                      , 0
                                      , NULL );
        TEST_ASSERT_EQUAL(rc, OK);

        pContext->hTran = NULL;
        pContext->tranActive = false;
    }

    //We'd like more messages
    return true;
}

void GlobalTran_RollbackGet( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    uint32_t loop;
    uint32_t messageCount = 10;
    ismEngine_SessionHandle_t hMainSession;
    ismEngine_SessionHandle_t hConsumerSession;
    ismEngine_ConsumerHandle_t hConsumer;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    char payload[128];
    ismEngine_MessageHandle_t hMessage = NULL;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2];
    size_t areaLengths[2];
    void *areas[2];
    RollbackGetContext_t consumerContext, *pconsumerContext=&consumerContext;

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting GlobalTran_RollbackGet...");

    // Create main session for this test
    rc=ism_engine_createSession( hGlobalClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hMainSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc , OK);

    // Create consumer session for this test
    rc=ism_engine_createSession( hGlobalClient
                               , ismENGINE_CREATE_SESSION_TRANSACTIONAL
                               , &hConsumerSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc , OK);


    test_log(testLOGLEVEL_TESTPROGRESS, "Creating transaction");

    // Create a multi-consumer queue and load with messages
    char *qName = "GlobalTran_RollbackGet";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server,
                          NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

    for (loop=0; loop < messageCount; loop++)
    {
        sprintf(payload, "Message %d", loop);
        areaTypes[0] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[0] = strlen(payload) +1;
        areas[0] = (void *)payload;

        rc = ism_engine_createMessage(&header,
                                      1,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = sync_ism_engine_putMessageOnDestination(hMainSession,
                                                ismDESTINATION_QUEUE,
                                                qName,
                                                NULL,
                                                hMessage);

        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Create a consumer which will each attempt to consume all
    // of the messages
    consumerContext.messageCount = messageCount;
    consumerContext.hSession = hConsumerSession;
    consumerContext.qname = qName;
    consumerContext.messagesReceived = 0;
    memset(&consumerContext.XID, 0, sizeof(ism_xid_t));
    consumerContext.hTran = NULL;
    consumerContext.tranActive = false;
    consumerContext.tranId=4;
    consumerContext.branchId=0;

    rc = ism_engine_createConsumer(hConsumerSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pconsumerContext,
                                   sizeof(RollbackGetContext_t *),
                                   RollbackGetConsumer,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And now start message delivery. When this call completes all of the
    // messages should have been delivered to the consumer.
    rc = ism_engine_startMessageDelivery(hConsumerSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);

    // Validate that 30 messages were received.
    TEST_ASSERT_EQUAL(consumerContext.messagesReceived, messageCount);

    test_log(testLOGLEVEL_TESTPROGRESS, "Consumer succesfully received %d messages as part of global tran", consumerContext.messagesReceived);

    // Validate that the consumer has finsihed with the transaction
    // it created
    TEST_ASSERT_EQUAL(consumerContext.tranActive, false);

    // release the consumer to start a new transactions
    memcpy(&XID, &consumerContext.XID, sizeof(ism_xid_t));
    memset(&consumerContext.XID, 0, sizeof(ism_xid_t));
    consumerContext.messagesReceived = 0;


    // Now rollback the transaction. this should cause the messages to
    // be redelivered to the consumer.
    rc=sync_ism_engine_createGlobalTransaction( hMainSession
                                         , &XID
                                         , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                         , &hTran );
    TEST_ASSERT_EQUAL(rc , OK);

    rc=ism_engine_endTransaction(hMainSession,
                                 hTran,
                                 ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                 NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc=sync_ism_engine_prepareGlobalTransaction( hMainSession
                                          , &XID);
    TEST_ASSERT_EQUAL(rc, OK);   

    rc=ism_engine_rollbackTransaction( hMainSession
                                     , hTran
                                     , NULL
                                     , 0
                                     , NULL );
    TEST_ASSERT_EQUAL(rc , OK);

    test_log(testLOGLEVEL_TESTPROGRESS, "Rollback transaction from MainSession completed with rc=%d", rc);

    // And verify that the consumer has now re-received all of the messages
    // Validate that 30 messages were received.
    TEST_ASSERT_EQUAL(consumerContext.messagesReceived, messageCount);

    // Validate that the consumer has finsihed with the transaction
    // it created
    TEST_ASSERT_EQUAL(consumerContext.tranActive, false);

    // Call commit and we are done
    // Now rollback the transaction. this should cause the messages to
    // be redelivered to the consumer.
    rc=sync_ism_engine_createGlobalTransaction( hMainSession
                                         , &XID
                                         , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                         , &hTran);
    TEST_ASSERT_EQUAL(rc , OK);


    rc=sync_ism_engine_prepareGlobalTransaction( hMainSession
                                          , &XID);
    TEST_ASSERT_EQUAL(rc, OK);   

    rc=sync_ism_engine_commitTransaction( hMainSession
                                   , hTran
                                   , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc , OK);

    test_log(testLOGLEVEL_TESTPROGRESS, "Commit transaction from MainSession completed with rc=%d", rc);

    //Delete the consumer
    rc = ism_engine_destroyConsumer( hConsumer, NULL, 0 , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hConsumerSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession( hMainSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestCase:       GlobalTran_StoreFail                              */
/* Description:    Check Operation of transaction creation with      */
/*                 store errors.                                     */
/*********************************************************************/
void GlobalTran_StoreFail( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting %s... [** EXPECT FFDC FROM ietr_createGlobal **]", __func__);

    // Create Session for this test
    rc = test_createClientAndSession("STOREFAIL",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 1001;
    globalId.bqual = 2002;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    ism_store_createRecord_FORCE_ERROR = ISMRC_Error;
    ism_store_openReferenceContext_FORCE_ERROR = OK;

    rc = sync_ism_engine_createGlobalTransaction( hSession
                                           , &XID
                                           , ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS
                                           , &hTran);
    TEST_ASSERT_EQUAL(rc , ISMRC_Error);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    rc = ietr_findGlobalTransaction(pThreadData, &XID, &hTran);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    ism_store_createRecord_FORCE_ERROR = OK;
    ism_store_openReferenceContext_FORCE_ERROR = ISMRC_Error;

    rc = sync_ism_engine_createGlobalTransaction( hSession
                                           , &XID
                                           , ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS
                                           , &hTran);
    TEST_ASSERT_EQUAL(rc , ISMRC_Error);

    rc = ietr_findGlobalTransaction(pThreadData, &XID, &hTran);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    ism_store_createRecord_FORCE_ERROR = OK;
    ism_store_openReferenceContext_FORCE_ERROR = OK;

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestSuite:      ISM_Engine_Global_Transactions1                   */
/* Description:    Tests for global transactions                     */
/*********************************************************************/
CU_TestInfo ISM_Engine_Global_Transactions1[] = {
    { "GlobalTran_Simple1",             GlobalTran_Simple1 },
    { "GlobalTran_WrongSessionCommit",  GlobalTran_WrongSessionCommit },
    { "GlobalTran_RollbackGet",         GlobalTran_RollbackGet },
    { "GlobalTran_StoreFail",           GlobalTran_StoreFail },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestCase:       GlobalTran_Defect_38833                           */
/*********************************************************************/
void GlobalTran_Defect_38833( void )
{
    uint8_t globalId_1[] = {0x00,0x00,0x01,0x40,0xC1,0x64,0x8F,0xF1,0x00,0x00,0x00,0x01,0x05,0x8D,0xA4,0xA0,0xC7,0x3F,
                            0xE1,0xFD,0x27,0x0C,0xB6,0x9D,0xEC,0x77,0xDD,0x15,0x10,0x1E,0x7B,0x41,0x81,0xD4,0x5E,0xD6,
                            0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
                            0x00,0x00,0x01,0x40,0xC1,0x64,0x8F,0xF1,0x00,0x00,0x00,0x01,0x05,0x8D,0xA4,0xA0,0xC7,0x3F,
                            0xE1,0xFD,0x27,0x0C,0xB6,0x9D,0xEC,0x77,0xDD,0x15,0x10,0x1E,0x7B,0x41,0x81,0xD4,0x5E,0xD6};

    uint8_t globalId_2[] = {0x00,0x00,0x01,0x40,0xC1,0x64,0x8F,0xF9,0x00,0x00,0x00,0x01,0x05,0x8D,0xA4,0xA8,0xC7,0x3F,
                            0xE1,0xFD,0x27,0x0C,0xB6,0x9D,0xEC,0x77,0xDD,0x15,0x10,0x1E,0x7B,0x41,0x81,0xD4,0x5E,0xD6,
                            0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
                            0x00,0x00,0x01,0x40,0xC1,0x64,0x8F,0xF9,0x00,0x00,0x00,0x01,0x05,0x8D,0xA4,0xA8,0xC7,0x3F,
                            0xE1,0xFD,0x27,0x0C,0xB6,0x9D,0xEC,0x77,0xDD,0x15,0x10,0x1E,0x7B,0x41,0x81,0xD4,0x5E,0xD6};

    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran1, hTran2;

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    TEST_ASSERT_EQUAL(sizeof(globalId_1), sizeof(globalId_2));
    TEST_ASSERT_NOT_EQUAL(memcmp(globalId_1, globalId_2, sizeof(globalId_1)), 0);

    // Create Session for this test
    rc = test_createClientAndSession("DEFECT_38833",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    XID.formatID = 0x57415344;
    XID.gtrid_length = 54;
    XID.bqual_length = 36;
    memcpy(&XID.data, globalId_1, sizeof(globalId_1));
    TEST_ASSERT_EQUAL(sizeof(globalId_1), XID.gtrid_length+XID.bqual_length);

    rc = sync_ism_engine_createGlobalTransaction( hSession
                                           , &XID
                                           , ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS
                                           , &hTran1);
    TEST_ASSERT_EQUAL(rc, OK);

    memcpy(&XID.data, globalId_2, sizeof(globalId_2));
    TEST_ASSERT_EQUAL(sizeof(globalId_2), XID.gtrid_length+XID.bqual_length);

    rc = sync_ism_engine_createGlobalTransaction( hSession
                                           , &XID
                                           , ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS
                                           , &hTran2);
    TEST_ASSERT_EQUAL(rc , OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

void GlobalTran_Defect_116399( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create a client and session
    rc = test_createClientAndSession("DEFECT_116399",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient,
                                     &hSession,
                                     true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a global transaction
    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x12C13311;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x0123456789ABCDEF;
    globalId.bqual = 0x175FAB2BEBAD1715;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    // Create a global transaction
    rc = sync_ism_engine_createGlobalTransaction(hSession,
                                                 &XID,
                                                 ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                 &hTran );
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran);

    // Try invalidly commiting the transaction multiple times
    for(int32_t i=0; i<100; i++)
    {
        rc = ism_engine_commitTransaction(hSession,
                                          hTran,
                                          ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT,
                                          NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);
    }

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_prepareTransaction(hSession,
                                       hTran,
                                       &pActionsRemaining, sizeof(pActionsRemaining),
                                       test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_commitGlobalTransaction(hSession, &XID, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT,
                                            &pActionsRemaining, sizeof(pActionsRemaining),
                                            test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    // Destroy the client & Session
    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestSuite:      ISM_Engine_Global_Defects                         */
/* Description:    Defects in global transactions                    */
/*********************************************************************/
CU_TestInfo ISM_Engine_Global_Defects[] = {
    { "Defect_38833", GlobalTran_Defect_38833 },
    { "Defect_116399", GlobalTran_Defect_116399 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestCase:       GlobalTran_Heuristic                              */
/* Description:    Test basic heuristic completion cases.            */
/*********************************************************************/
void GlobalTran_Heuristic( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create a client and session
    rc = test_createClientAndSession("CLIENT_HEURISTIC",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient,
                                     &hSession,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x12C13311;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x953122FD11DD43E1;
    globalId.bqual = 0xABCDEFABCDEFABCD;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    for(int32_t i=0; i<4; i++)
    {
        // Create a global transaction
        rc = sync_ism_engine_createGlobalTransaction(hSession,
                                                &XID,
                                                ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                &hTran );
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hTran);

        // Try completing the transaction
        rc = sync_ism_engine_completeGlobalTransaction(&XID,
                                                  ismTRANSACTION_COMPLETION_TYPE_COMMIT);
        TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

        // Try forgetting it
        rc = sync_ism_engine_forgetGlobalTransaction(&XID);
        TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

        switch(i)
        {
            case 0:
            case 1:
                // End the transaction success
                rc = ism_engine_endTransaction(hSession,
                                               hTran,
                                               ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                               NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);

                rc = sync_ism_engine_completeGlobalTransaction(&XID,
                                                          ismTRANSACTION_COMPLETION_TYPE_ROLLBACK);
                TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);
                break;

            case 2:
                // End the transaction success
                rc = ism_engine_endTransaction(hSession,
                                               hTran,
                                               ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                               NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);

                rc = sync_ism_engine_completeGlobalTransaction(&XID,
                                                          ismTRANSACTION_COMPLETION_TYPE_COMMIT);
                TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);
                break;
            case 3:
                // End the transaction fail
                rc = ism_engine_endTransaction(hSession,
                                               hTran,
                                               ismENGINE_END_TRANSACTION_OPTION_XA_TMFAIL,
                                               NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);

                rc = sync_ism_engine_completeGlobalTransaction(&XID,
                                                          ismTRANSACTION_COMPLETION_TYPE_COMMIT);
                TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);
                break;
        }

        rc = sync_ism_engine_forgetGlobalTransaction(&XID);
        TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);

        rc = sync_ism_engine_prepareGlobalTransaction(hSession, &XID);
        if (i == 3)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_RolledBack); // Already rolled back
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }

        rc = sync_ism_engine_forgetGlobalTransaction(&XID);
        if (i == 3)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound); // Already rolled back
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);
        }

        switch(i)
        {
            case 0:
            case 1:
                rc = sync_ism_engine_completeGlobalTransaction(&XID,
                                                          ismTRANSACTION_COMPLETION_TYPE_COMMIT);
                TEST_ASSERT_EQUAL(rc, OK);
                rc = sync_ism_engine_completeGlobalTransaction(&XID,
                                                          ismTRANSACTION_COMPLETION_TYPE_COMMIT);
                TEST_ASSERT_EQUAL(rc, ISMRC_HeuristicallyCommitted);
                rc = sync_ism_engine_completeGlobalTransaction(&XID,
                                                          ismTRANSACTION_COMPLETION_TYPE_ROLLBACK);
                TEST_ASSERT_EQUAL(rc, ISMRC_HeuristicallyCommitted);
                break;
            case 2:
                rc = sync_ism_engine_completeGlobalTransaction(&XID,
                                                          ismTRANSACTION_COMPLETION_TYPE_ROLLBACK);
                TEST_ASSERT_EQUAL(rc, OK);
                rc = sync_ism_engine_completeGlobalTransaction(&XID,
                                                          ismTRANSACTION_COMPLETION_TYPE_ROLLBACK);
                TEST_ASSERT_EQUAL(rc, ISMRC_HeuristicallyRolledBack);
                rc = sync_ism_engine_completeGlobalTransaction(&XID,
                                                          ismTRANSACTION_COMPLETION_TYPE_COMMIT);
                TEST_ASSERT_EQUAL(rc, ISMRC_HeuristicallyRolledBack);
                break;
            case 3:
                rc = sync_ism_engine_completeGlobalTransaction(&XID,
                                                          ismTRANSACTION_COMPLETION_TYPE_ROLLBACK);
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
                break;
        }

        if (i != 3)
        {
            rc = ism_engine_commitGlobalTransaction(hSession,
                                                    &XID,
                                                    ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                    NULL, 0, NULL);
            if (i == 2)
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_HeuristicallyRolledBack);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_HeuristicallyCommitted);
            }

            rc = ism_engine_rollbackGlobalTransaction(hSession,
                                                      &XID,
                                                      NULL, 0, NULL);
            if (i == 2)
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_HeuristicallyRolledBack);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_HeuristicallyCommitted);
            }
        }

        rc = sync_ism_engine_forgetGlobalTransaction(&XID);
        if (i == 3)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }

        // Prove it's gone
        rc = sync_ism_engine_forgetGlobalTransaction(&XID);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestCase:       GlobalTran_Monitor                                */
/* Description:    Test global transaction monitors                  */
/*********************************************************************/
void GlobalTran_Monitor( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId, checkGlobalId;

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    ismEngine_TransactionMonitor_t *results = NULL;
    uint32_t resultCount = 0;

    // No transactions
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 10, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    ism_engine_freeTransactionMonitor(results);

    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 10, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    ism_engine_freeTransactionMonitor(results);

    // Create a client and session
    rc = test_createClientAndSession("CLIENT_HEURMONITOR",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient,
                                     &hSession,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x12C13311;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x95AA22FD11DD11E1;

    ismEngine_TransactionHandle_t hTran[10];

    // Create 10 global transactions
    for(uint64_t i=0; i<10; i++)
    {
        globalId.bqual = i;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        rc = sync_ism_engine_createGlobalTransaction(hSession,
                                                &XID,
                                                ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                &hTran[i]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hTran[i]);
    }

    // No prepared / completed transactions
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 10, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    ism_engine_freeTransactionMonitor(results);

    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 10, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    ism_engine_freeTransactionMonitor(results);

    // Try to specifying various filter requests
    ism_prop_t *props = ism_common_newProperties(2);
    ism_field_t f;
    f.type = VT_String;

    // Inflight transactions
    f.val.s = ismENGINE_MONITOR_FILTER_TRANSTATE_IN_FLIGHT;
    ism_common_setProperty(props, ismENGINE_MONITOR_FILTER_TRANSTATE, &f);
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 10, props);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 10);

    ism_engine_freeTransactionMonitor(results);

    // Prepared transactions
    f.val.s = ismENGINE_MONITOR_FILTER_TRANSTATE_PREPARED;
    ism_common_setProperty(props, ismENGINE_MONITOR_FILTER_TRANSTATE, &f);
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 10, props);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    // Empty properties
    ism_common_clearProperties(props);
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 10, props);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    // Any transaction any XID requested with an asterisk (but limiting to 6 results)
    f.val.s = "*";
    ism_common_setProperty(props, ismENGINE_MONITOR_FILTER_XID, &f);
    f.val.s = ismENGINE_MONITOR_FILTER_TRANSTATE_ANY;
    ism_common_setProperty(props, ismENGINE_MONITOR_FILTER_TRANSTATE, &f);
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 6, props);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 6);

    ism_engine_freeTransactionMonitor(results);

    // Get a specific XID (the last one)
    char XIDBuffer[300];
    const char *XIDString;
    XIDString = ism_common_xidToString(&XID, XIDBuffer, sizeof(XIDBuffer));
    f.val.s = (char *)XIDString;
    ism_common_setProperty(props, ismENGINE_MONITOR_FILTER_XID, &f);
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 6, props);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);
    TEST_ASSERT_EQUAL(memcmp(&XID, &results[0].XID, sizeof(ism_xid_t)), 0);

    ism_engine_freeTransactionMonitor(results);

    // Filter with a wildcard XID string which should return all.
    f.val.s = "12C13311*";
    ism_common_setProperty(props, ismENGINE_MONITOR_FILTER_XID, &f);
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 1, props);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 10);

    ism_engine_freeTransactionMonitor(results);

    // Invalid properties
    ism_common_clearProperties(props);
    f.val.s = "INVALID!";
    ism_common_setProperty(props, ismENGINE_MONITOR_FILTER_TRANSTATE, &f);
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 1, props);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);
    f.val.s = "ALONGERINVALIDPARAMETER";
    ism_common_setProperty(props, ismENGINE_MONITOR_FILTER_TRANSTATE, &f);
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 1, props);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    ism_common_freeProperties(props);

    globalId.bqual = 0;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc = sync_ism_engine_prepareTransaction(hSession, hTran[0]);
    TEST_ASSERT_EQUAL(rc, OK);

    // One prepared transaction
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);
    memcpy(&checkGlobalId, &(results[0].XID.data), sizeof(checkGlobalId));
    TEST_ASSERT_EQUAL(globalId.bqual, checkGlobalId.bqual);
    TEST_ASSERT_EQUAL(results[0].state, ismTRANSACTION_STATE_PREPARED);
    TEST_ASSERT_NOT_EQUAL(results[0].stateChangedTime, 0);

    ism_time_t prevStateChangedTime = results[0].stateChangedTime;

    ism_engine_freeTransactionMonitor(results);

    test_log(testLOGLEVEL_TESTNAME, "Sleeping for 1 second...");
    sleep(1);

    rc = sync_ism_engine_completeGlobalTransaction(&XID, ismTRANSACTION_COMPLETION_TYPE_COMMIT);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 10, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);
    TEST_ASSERT_EQUAL(globalId.bqual, checkGlobalId.bqual);
    TEST_ASSERT_EQUAL(results[0].state, ismTRANSACTION_STATE_HEURISTIC_COMMIT);
    TEST_ASSERT_NOT_EQUAL(results[0].stateChangedTime, prevStateChangedTime);

    ism_engine_freeTransactionMonitor(results);

    rc = sync_ism_engine_forgetGlobalTransaction(&XID);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 10, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    ism_engine_freeTransactionMonitor(results);

    for(uint64_t i=1; i<6; i++)
    {
        rc = ism_engine_endTransaction(hSession, hTran[i], ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 3, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    ism_engine_freeTransactionMonitor(results);

    for(uint64_t i=1; i<6; i++)
    {
        globalId.bqual = i;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        rc = sync_ism_engine_prepareGlobalTransaction(hSession, &XID);
        TEST_ASSERT_EQUAL(rc, OK);

        test_log(testLOGLEVEL_TESTNAME, "Sleeping for 1 second...");
        sleep(1);
    }

    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 3, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 3);

    prevStateChangedTime = 0;
    for(int32_t i=0; i<resultCount; i++)
    {
        TEST_ASSERT_EQUAL(results[i].state, ismTRANSACTION_STATE_PREPARED);
        TEST_ASSERT(results[i].stateChangedTime >= prevStateChangedTime,
                    ("Result %d time (%lu) < prev (%lu)", i, results[i].stateChangedTime, prevStateChangedTime));
        prevStateChangedTime = results[i].stateChangedTime;
    }

    ism_engine_freeTransactionMonitor(results);

    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 10, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 5);

    ism_engine_freeTransactionMonitor(results);

    // Check that we filter out MQConnectivity transactions
    ismEngine_TransactionHandle_t hTranMQC;
    XID.formatID = 0x494D514D;

    rc = sync_ism_engine_createGlobalTransaction(hSession,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                            &hTranMQC);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTranMQC);

    props = ism_common_newProperties(2);
    f.type = VT_String;
    f.val.s = "*514D:*";
    ism_common_setProperty(props, ismENGINE_MONITOR_FILTER_XID, &f);

    for(int32_t i=0; i<4; i++)
    {
        rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 10, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 5);

        ism_engine_freeTransactionMonitor(results);

        switch(i)
        {
            case 0:
                rc = ism_engine_endTransaction(hSession,
                                               hTranMQC,
                                               ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                               NULL, 0, NULL);
                break;
            case 1:
                rc = sync_ism_engine_prepareGlobalTransaction(hSession, &XID);
                break;
            case 2:
                rc = sync_ism_engine_completeGlobalTransaction(&XID, ismTRANSACTION_COMPLETION_TYPE_COMMIT);
                break;
            case 3:
                rc = sync_ism_engine_forgetGlobalTransaction(&XID);
                break;
        }

        TEST_ASSERT(rc == OK, ("MQConnectivity transaction failed at step %d, rc=%d", i, rc));

        // Check what happens when we query with an XID, we should *INCLUDE* MQConnectivity transactions
        rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 10, props);
        TEST_ASSERT_EQUAL(rc, OK);

        if (i == 0 || i == 3)
        {
            TEST_ASSERT_EQUAL(resultCount, 0);
        }
        else
        {
            TEST_ASSERT_EQUAL(resultCount, 1);
        }

        ism_engine_freeTransactionMonitor(results);
    }

    ism_common_freeProperties(props);

    // Back to non-MQConnectivity transactions
    XID.formatID = 0x12C13311;

    globalId.bqual = 1;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc = sync_ism_engine_completeGlobalTransaction(&XID, ismTRANSACTION_COMPLETION_TYPE_ROLLBACK);
    TEST_ASSERT_EQUAL(rc, OK);

    test_log(testLOGLEVEL_TESTNAME, "Sleeping for 1 second...");
    sleep(1);

    globalId.bqual = 2;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc = sync_ism_engine_completeGlobalTransaction(&XID, ismTRANSACTION_COMPLETION_TYPE_COMMIT);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t loop=0; loop<3; loop++)
    {
        rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 5, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        uint32_t prepared=0, heurcom=0, heurrb=0;
        for(int32_t i=0; i<resultCount; i++)
        {
            test_log(testLOGLEVEL_VERBOSE, "Transaction %d %lu %u", i, results[i].stateChangedTime, results[i].state);

            switch(results[i].state)
            {
                case ismTRANSACTION_STATE_HEURISTIC_COMMIT:
                    heurcom++;
                    break;
                case ismTRANSACTION_STATE_HEURISTIC_ROLLBACK:
                    heurrb++;
                    break;
                case ismTRANSACTION_STATE_PREPARED:
                    prepared++;
                    break;
                default:
                    TEST_ASSERT(false, ("Unexpected transaction state %u", (uint32_t)results[i].state));
                    break;
            }
        }

        if (loop == 0)
        {
            TEST_ASSERT_EQUAL(resultCount, 5);
            TEST_ASSERT_EQUAL(prepared, 3);
        }
        else
        {
            TEST_ASSERT_EQUAL(resultCount, 4);
            TEST_ASSERT_EQUAL(prepared, 2);
        }

        TEST_ASSERT_EQUAL(heurcom, 1);
        TEST_ASSERT_EQUAL(heurrb, 1);

        ism_engine_freeTransactionMonitor(results);

        if (loop != 2)
        {
            if (loop == 0)
            {
                globalId.bqual = 3;
                memcpy(&XID.data, &globalId, sizeof(globalId));

                // Commit 1 before restarting
                rc = sync_ism_engine_commitGlobalTransaction(hSession,
                                                        &XID,
                                                        ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS);
                TEST_ASSERT_EQUAL(rc, OK);

                rc = test_destroyClientAndSession(hClient, hSession, true);
                TEST_ASSERT_EQUAL(rc, OK);
            }
        }
    }

    globalId.bqual = 1;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc = sync_ism_engine_forgetGlobalTransaction(&XID);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 5, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 3);

    ism_engine_freeTransactionMonitor(results);

    globalId.bqual = 4;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc = sync_ism_engine_completeGlobalTransaction(&XID, ismTRANSACTION_COMPLETION_TYPE_ROLLBACK);
    TEST_ASSERT_EQUAL(rc, OK);

    globalId.bqual = 5;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc = sync_ism_engine_completeGlobalTransaction(&XID, ismTRANSACTION_COMPLETION_TYPE_COMMIT);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 3);

    // Forget the heuristically completed transactions (should be all of them)
    for(int32_t i=0; i<resultCount; i++)
    {
        TEST_ASSERT_NOT_EQUAL(results[i].state, ismTRANSACTION_STATE_PREPARED);
        rc = sync_ism_engine_forgetGlobalTransaction(&results[i].XID);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    ism_engine_freeTransactionMonitor(results);

    // Check they are gone
    rc = ism_engine_getTransactionMonitor(&results, &resultCount, ismENGINE_MONITOR_OLDEST_STATECHANGE, 5, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    ism_engine_freeTransactionMonitor(results);

    return;
}

/*********************************************************************/
/* TestCase:       GlobalTran_NULLCallback                           */
/* Description:    Test heuristic completion with NULL callback.     */
/*********************************************************************/
void GlobalTran_NULLCallback( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    test_log(testLOGLEVEL_TESTNAME, ""); // Ensure we start on a new line
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create a client and session
    rc = test_createClientAndSession("CLIENT_NULLCALLBACK",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient,
                                     &hSession,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x12C13311;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x953122FD11DD43E1;
    globalId.bqual = 0xFEDDEADBEEF2ABBA;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    // Create a global transaction
    rc = sync_ism_engine_createGlobalTransaction(hSession,
                                                 &XID,
                                                 ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                 &hTran );
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran);

    ismEngine_Transaction_t *pTran = (ismEngine_Transaction_t *)hTran;

    rc = sync_ism_engine_prepareGlobalTransaction(hSession, &XID);
    TEST_ASSERT_EQUAL(rc, OK);

    // Allow it to go async, but pass a NULL callback
    rc = ism_engine_completeGlobalTransaction(&XID,
                                              ismTRANSACTION_COMPLETION_TYPE_COMMIT, NULL, 0, NULL);
    if (rc != OK) TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    while(pTran->CompletionStage != ietrCOMPLETION_ENDED)
    {
        sched_yield();
    }

    rc = sync_ism_engine_forgetGlobalTransaction(&XID);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestSuite:      ISM_Engine_Global_HeuristicCompletion             */
/* Description:    Tests for global transaction Heuristic completion */
/*********************************************************************/
CU_TestInfo ISM_Engine_Global_HeuristicCompletion[] = {
    { "Heuristic", GlobalTran_Heuristic },
    { "TransactionMonitor", GlobalTran_Monitor },
    { "NULLCallback", GlobalTran_NULLCallback },
    CU_TEST_INFO_NULL
};

int initTransactionsWithClient(void)
{
    int32_t rc;

    rc = test_engineInit_DEFAULT;
    if (rc != OK) goto mod_exit;

    rc = ism_engine_createClientState("Global Transaction Test",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hGlobalClient,
                                      NULL,
                                      0,
                                      NULL);

    if (rc != OK)
    {
        printf("ERROR:  ism_engine_createClientState() returned %d\n", rc);
    }

mod_exit:

    return rc;
}

int termTransactionsWithClient(void)
{
  int32_t rc = ism_engine_destroyClientState(hGlobalClient,
                                      ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                      NULL,
                                      0,
                                      NULL);
  if (rc != OK && rc != ISMRC_AsyncCompletion)
  {
      abort();
  }

    return test_engineTerm(true);
}

int initTransactions(void)
{
    return test_engineInit_DEFAULT;
}

int termTransactions(void)
{
    return test_engineTerm(true);
}

/*********************************************************************/
/* TestSuite definition                                              */
/*********************************************************************/
CU_SuiteInfo ISM_Engine_CUnit_Transaction_Suites[] = {
    IMA_TEST_SUITE("ISM_Engine_Global_Transactions1", initTransactionsWithClient, termTransactionsWithClient, ISM_Engine_Global_Transactions1),
    IMA_TEST_SUITE("ISM_Engine_Global_XATransactions", initTransactions, termTransactions, ISM_Engine_Global_XATransactions),
    IMA_TEST_SUITE("ISM_Engine_Global_Defects", initTransactions, termTransactions, ISM_Engine_Global_Defects),
    IMA_TEST_SUITE("ISM_Engine_Global_HeuristicCompletion", initTransactions, termTransactions, ISM_Engine_Global_HeuristicCompletion),
    CU_SUITE_INFO_NULL
};

int main(int argc, char *argv[])
{
    int trclvl = 4;
    int retval = 0;

    // Level of output
    if (argc >= 2)
    {
        int32_t level=atoi(argv[1]);
        if ((level >=0) && (level <= 9))
        {
            test_setLogLevel(level);
        }
    }

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    srand(ism_common_currentTimeNanos());

    CU_initialize_registry();
    CU_register_suites(ISM_Engine_CUnit_Transaction_Suites);

    CU_basic_run_tests();

    CU_RunSummary * CU_pRunSummary_Final;
    CU_pRunSummary_Final = CU_get_run_summary();
    printf("\n\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
            CU_pRunSummary_Final->nTestsRun,
            CU_pRunSummary_Final->nTestsFailed,
            CU_pRunSummary_Final->nAssertsFailed);
    if ((CU_pRunSummary_Final->nTestsFailed > 0) ||
        (CU_pRunSummary_Final->nAssertsFailed > 0))
    {
        retval = 1;
    }

    CU_cleanup_registry();

    int32_t rc = test_processTerm(retval == 0);
    if (retval == 0) retval = rc;

mod_exit:

    return retval;
}
