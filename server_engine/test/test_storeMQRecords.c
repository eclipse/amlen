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
/* Module Name: test_storeMQRecords.c                                */
/*                                                                   */
/* Description: CUnit tests of Engine MQ Record store functions      */
/*                                                                   */
/*********************************************************************/
#include "test_storeMQRecords.h"
#include "test_utils_assert.h"
#include "test_utils_sync.h"

CU_TestInfo ISM_Engine_CUnit_StoreMQRecords[] = {
    { "StoreMQRecordsTestQMgrRecord",    storeMQRecordsTestQMgrRecord   },
    { "StoreMQRecordsTestQMXidRecord",   storeMQRecordsTestQMXidRecord  },
    { "StoreMQRecordsTestTransactions",  storeMQRecordsTestTransactions },
    CU_TEST_INFO_NULL
};


static void qmgrRecordCountCallback(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QManagerRecordHandle_t    hQMgrRec,
    void *                              pContext);

static void qmgrRecordHandleCallback(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QManagerRecordHandle_t    hQMgrRec,
    void *                              pContext);

static void qmgrXidRecordCountCallback(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QMgrXidRecordHandle_t     hQMgrXidRec,
    void *                              pContext);

static void qmgrXidRecordHandleCallback(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QMgrXidRecordHandle_t     hQMgrXidRec,
    void *                              pContext);

/*
 * Test creation and deletion of queue-manager records
 */
void storeMQRecordsTestQMgrRecord(void)
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_QManagerRecordHandle_t hQMgrRec1;
    ismEngine_QManagerRecordHandle_t hQMgrRec2;
    ismEngine_QManagerRecordHandle_t hQMgrRec3;
    uint32_t qmrCount;
    int32_t rc = OK;

    rc = ism_engine_createClientState(__func__,
                                      PROTOCOL_ID_MQ,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createQManagerRecord(hSession,
                                         "QMRecord1",
                                         10,
                                         &hQMgrRec1,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createQManagerRecord(hSession,
                                         "QMRecord2",
                                         10,
                                         &hQMgrRec2,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createQManagerRecord(hSession,
                                         "QMRecord3",
                                         10,
                                         &hQMgrRec3,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    qmrCount = 0;
    rc = ism_engine_listQManagerRecords(hSession,
                                        &qmrCount,
                                        qmgrRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmrCount, 3);

    rc = ism_engine_destroyQManagerRecord(hSession,
                                          hQMgrRec2,
                                          NULL,
                                          0,
                                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyQManagerRecord(hSession,
                                          hQMgrRec1,
                                          NULL,
                                          0,
                                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    qmrCount = 0;
    rc = ism_engine_listQManagerRecords(hSession,
                                        &qmrCount,
                                        qmgrRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmrCount, 1);

    rc = ism_engine_destroySession(hSession,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_listQManagerRecords(hSession,
                                        &hQMgrRec3,
                                        qmgrRecordHandleCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyQManagerRecord(hSession,
                                          hQMgrRec3,
                                          NULL,
                                          0,
                                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    qmrCount = 0;
    rc = ism_engine_listQManagerRecords(hSession,
                                        &qmrCount,
                                        qmgrRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmrCount, 0);

    rc = ism_engine_destroySession(hSession,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Test creation and deletion of queue-manager XID records
 */
void storeMQRecordsTestQMXidRecord(void)
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_QManagerRecordHandle_t hQMgrRec1;
    ismEngine_QMgrXidRecordHandle_t hQMXidRec1a;
    ismEngine_QManagerRecordHandle_t hQMgrRec2;
    uint32_t qmxidCount;
    int32_t rc = OK;

    rc = ism_engine_createClientState(__func__,
                                      PROTOCOL_ID_MQ,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createQManagerRecord(hSession,
                                         "QMRecord1",
                                         10,
                                         &hQMgrRec1,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createQManagerRecord(hSession,
                                         "QMRecord2",
                                         10,
                                         &hQMgrRec2,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createQMgrXidRecord(hSession,
                                        hQMgrRec1,
                                        NULL,
                                        "XIDRec1a",
                                        9,
                                        &hQMXidRec1a,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec1,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 1);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec2,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 0);

    rc = ism_engine_destroyQManagerRecord(hSession,
                                          hQMgrRec2,
                                          NULL,
                                          0,
                                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyQManagerRecord(hSession,
                                          hQMgrRec1,
                                          NULL,
                                          0,
                                          NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_LockNotGranted);

    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec1,
                                       &hQMXidRec1a,
                                       qmgrXidRecordHandleCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyQMgrXidRecord(hSession,
                                         hQMXidRec1a,
                                         NULL,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec1,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 0);

    rc = ism_engine_destroyQManagerRecord(hSession,
                                          hQMgrRec1,
                                          NULL,
                                          0,
                                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSession,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Test creation and deletion of queue-manager XID records in transactions
 */
void storeMQRecordsTestTransactions(void)
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_TransactionHandle_t hTran;
    ismEngine_QManagerRecordHandle_t hQMgrRec1;
    ismEngine_QMgrXidRecordHandle_t hQMXidRec1a;
    ismEngine_QMgrXidRecordHandle_t hQMXidRec1b;
    ismEngine_QMgrXidRecordHandle_t hQMXidRec1c;
    ismEngine_QManagerRecordHandle_t hQMgrRec2;
    ismEngine_QMgrXidRecordHandle_t hQMXidRec2a;
    ismEngine_QMgrXidRecordHandle_t hQMXidRec2b;
    ismEngine_QMgrXidRecordHandle_t hQMXidRec2c;
    uint32_t qmxidCount;
    int32_t rc = OK;

    rc = ism_engine_createClientState(__func__,
                                      PROTOCOL_ID_MQ,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createQManagerRecord(hSession,
                                         "QMRecord1",
                                         10,
                                         &hQMgrRec1,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createQManagerRecord(hSession,
                                         "QMRecord2",
                                         10,
                                         &hQMgrRec2,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_createLocalTransaction(hSession,
                                                &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create XID 1a outside a transaction - 1a
    rc = ism_engine_createQMgrXidRecord(hSession,
                                        hQMgrRec1,
                                        NULL,
                                        "XIDRec1a",
                                        9,
                                        &hQMXidRec1a,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec1,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 1);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec2,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 0);

    // Create XID 1b in a transaction - 1a, 1b(+)
    rc = ism_engine_createQMgrXidRecord(hSession,
                                        hQMgrRec1,
                                        hTran,
                                        "XIDRec1b",
                                        9,
                                        &hQMXidRec1b,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec1,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 1);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec2,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 0);

    // And commit it - 1a, 1b
    rc = sync_ism_engine_commitTransaction(hSession,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec1,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 2);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec2,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 0);

    rc = sync_ism_engine_createLocalTransaction(hSession,
                                                &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create XID 1c in a transaction - 1a, 1b, 1c(+)
    rc = ism_engine_createQMgrXidRecord(hSession,
                                        hQMgrRec1,
                                        hTran,
                                        "XIDRec1c",
                                        9,
                                        &hQMXidRec1c,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Try to remove it
    rc = ism_engine_destroyQMgrXidRecord(hSession,
                                         hQMXidRec1c,
                                         hTran,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_LockNotGranted);


    // Rollback the transaction - 1a, 1b
    rc = ism_engine_rollbackTransaction(hSession,
                                        hTran,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec1,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 2);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec2,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 0);

    rc = sync_ism_engine_createLocalTransaction(hSession,
                                                &hTran);
    TEST_ASSERT_EQUAL(rc, OK);


    // Remove 1a - 1a(-), 1b
    rc = ism_engine_destroyQMgrXidRecord(hSession,
                                         hQMXidRec1a,
                                         hTran,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And rollback the transaction - 1a, 1b
    rc = ism_engine_rollbackTransaction(hSession,
                                        hTran,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec1,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 2);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec2,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 0);

    rc = sync_ism_engine_createLocalTransaction(hSession,
                                                &hTran);
    TEST_ASSERT_EQUAL(rc, OK);


    // Remove 1b - 1a, 1b(-)
    rc = ism_engine_destroyQMgrXidRecord(hSession,
                                         hQMXidRec1b,
                                         hTran,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // And commit the transaction - 1a
    rc = sync_ism_engine_commitTransaction(hSession,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec1,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 1);

    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec2,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 0);


    // Create 2a outside a transaction - 2a
    rc = ism_engine_createQMgrXidRecord(hSession,
                                        hQMgrRec2,
                                        NULL,
                                        "XIDRec2a",
                                        9,
                                        &hQMXidRec2a,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Destroy 2a and create 2b in the same transaction - 2a(-), 2b(+)
    rc = sync_ism_engine_createLocalTransaction(hSession,
                                                &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyQMgrXidRecord(hSession,
                                         hQMXidRec2a,
                                         hTran,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createQMgrXidRecord(hSession,
                                        hQMgrRec2,
                                        hTran,
                                        "XIDRec2b",
                                        9,
                                        &hQMXidRec2b,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Roll it back - 2a
    rc = ism_engine_rollbackTransaction(hSession,
                                        hTran,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Destroy 2a and create 2c in the same transaction - 2a(-), 2c(+)
    rc = sync_ism_engine_createLocalTransaction(hSession,
                                                &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyQMgrXidRecord(hSession,
                                         hQMXidRec2a,
                                         hTran,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createQMgrXidRecord(hSession,
                                        hQMgrRec2,
                                        hTran,
                                        "XIDRec2c",
                                        9,
                                        &hQMXidRec2c,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Commit it - 2c
    rc = sync_ism_engine_commitTransaction(hSession,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);


    // And destroy 2c - 2c(-)
    rc = sync_ism_engine_createLocalTransaction(hSession,
                                                &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyQMgrXidRecord(hSession,
                                         hQMXidRec2c,
                                         hTran,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Commit it
    rc = sync_ism_engine_commitTransaction(hSession,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);


    qmxidCount = 0;
    rc = ism_engine_listQMgrXidRecords(hSession,
                                       hQMgrRec2,
                                       &qmxidCount,
                                       qmgrXidRecordCountCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(qmxidCount, 0);

    rc = ism_engine_destroyQManagerRecord(hSession,
                                          hQMgrRec2,
                                          NULL,
                                          0,
                                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Can't destroy this because 1a still exists
    rc = ism_engine_destroyQManagerRecord(hSession,
                                          hQMgrRec1,
                                          NULL,
                                          0,
                                          NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_LockNotGranted);

    rc = ism_engine_destroyQMgrXidRecord(hSession,
                                         hQMXidRec1a,
                                         NULL,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyQManagerRecord(hSession,
                                          hQMgrRec1,
                                          NULL,
                                          0,
                                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSession,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}


static void qmgrRecordCountCallback(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QManagerRecordHandle_t    hQMgrRec,
    void *                              pContext)
{
    uint32_t *pQmrCount = (uint32_t *)pContext;
    (*pQmrCount)++;
}


static void qmgrRecordHandleCallback(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QManagerRecordHandle_t    hQMgrRec,
    void *                              pContext)
{
    ismEngine_QManagerRecordHandle_t *pHQMgrRec = (ismEngine_QManagerRecordHandle_t *)pContext;
    *pHQMgrRec = hQMgrRec;
}

static void qmgrXidRecordCountCallback(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QMgrXidRecordHandle_t     hQMgrXidRec,
    void *                              pContext)
{
    uint32_t *pQmxrCount = (uint32_t *)pContext;
    (*pQmxrCount)++;
}

static void qmgrXidRecordHandleCallback(
    void *                              pData,
    size_t                              dataLength,
    ismEngine_QMgrXidRecordHandle_t     hQMgrXidRec,
    void *                              pContext)
{
    ismEngine_QMgrXidRecordHandle_t *pHQMgrXidRec = (ismEngine_QMgrXidRecordHandle_t *)pContext;
    *pHQMgrXidRec = hQMgrXidRec;
}
