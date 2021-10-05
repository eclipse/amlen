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

/*********************************************************************/
/*                                                                   */
/* Module Name: test_transactionRecovery.c                           */
/*                                                                   */
/* Description: Main source file for testing transaction recovery    */
/*                                                                   */
/*********************************************************************/
#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include <transaction.h>

#include "test_utils_phases.h"
#include "test_utils_file.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_log.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"

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

static volatile uint32_t acksStarted = 0;
static volatile uint32_t acksCompleted = 0;

static void finishAck(int rc, void *handle, void *context)
{
    __sync_fetch_and_add(&acksCompleted, 1);
}

bool DeletedTransactionRestartCallback(
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
    ismEngine_TransactionHandle_t hTran = **((ismEngine_TransactionHandle_t **)pConsumerContext);

    __sync_fetch_and_add(&acksStarted, 1);

    int32_t rc = ism_engine_confirmMessageDelivery( ((ismEngine_Consumer_t *)hConsumer)->pSession
                                                       , hTran
                                                       , hDelivery
                                                       , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                       , NULL, 0
                                                       , finishAck);
    TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                      ("rc was %d\n", rc));

    if (rc == OK)
    {
        finishAck(rc, NULL, NULL);
    }

    //Free our copy of the message
    ism_engine_releaseMessage(hMessage);

    return true;
}

// This test creates a transacted ack, deletes the subscription then restarts
void test_DeletedTransaction_Phase1(bool global)
{
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;
    char *subName="toBeDeleted";
    char *topicString="/test/DeletedTransactionRestart";
    int32_t rc = OK;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s (%s)...", __func__,
                                                        global ? "global":"local");

    rc = test_createClientAndSession("SubbingClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK)

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };

    rc = sync_ism_engine_createSubscription(
                hClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                NULL); // Owning client same as requesting client

    ismEngine_MessageHandle_t hMessage;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, topicString,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            topicString,
                                            NULL,
                                            hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    if (global)
    {
        memset(&XID, 0, sizeof(ism_xid_t));
        XID.formatID = 0x12C13311;
        XID.gtrid_length = sizeof(uint64_t);
        XID.bqual_length = sizeof(uint64_t);
        globalId.gtrid = 0x0123456789ABCDEF;
        globalId.bqual = 0xABCDEF0123456789;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        rc = sync_ism_engine_createGlobalTransaction( hSession
                                               , &XID
                                               , ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS
                                               , &hTran );
        TEST_ASSERT_EQUAL(rc , ISMRC_OK);
    }
    else
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();
        rc = ietr_createLocal(pThreadData, NULL, true, false, NULL, &hTran);
        TEST_ASSERT_EQUAL(rc, OK);
    }

     ismEngine_TransactionHandle_t *phTran = &hTran;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   subName,
                                   NULL,
                                   NULL,
                                   &phTran,
                                   sizeof(phTran),
                                   DeletedTransactionRestartCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //By the time create consumer returns it will have locked the consumer to give it the message..

    rc = sync_ism_engine_destroyConsumer(hConsumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySubscription(
                              hClient,
                              subName,
                              hClient,
                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    if (global)
    {
        // End and prepare transaction
        rc = ism_engine_endTransaction(hSession,
                                       hTran,
                                       ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        rc = sync_ism_engine_prepareGlobalTransaction(hSession, &XID);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

void test_DeletedTransaction_Phase2(bool global)
{
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    int32_t rc = OK;
    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s (%s)...", __func__,
                                                        global ? "global":"local");

    if (global)
    {
        memset(&XID, 0, sizeof(ism_xid_t));
        XID.formatID = 0x12C13311;
        XID.gtrid_length = sizeof(uint64_t);
        XID.bqual_length = sizeof(uint64_t);
        globalId.gtrid = 0x0123456789ABCDEF;
        globalId.bqual = 0xABCDEF0123456789;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        //Recreate client
        rc = test_createClientAndSession("SubbingClient",
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                         &hClient, &hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);

        // Find the transaction
        ism_xid_t XIDArray[5] = {{0}};

        uint32_t found = ism_engine_XARecover(hSession,
                                              XIDArray,
                                              sizeof(XIDArray)/sizeof(XIDArray[0]),
                                              0,
                                              ismENGINE_XARECOVER_OPTION_XA_TMNOFLAGS);
        TEST_ASSERT_EQUAL(found, 1);

        TEST_ASSERT_EQUAL(memcmp(&XIDArray[0], &XID, sizeof(ism_xid_t)), 0);

        // Check that the transaction can now be committed
        rc = sync_ism_engine_commitGlobalTransaction(hSession,
                                                &XIDArray[0],
                                                ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS);
        TEST_ASSERT_EQUAL(rc, ISMRC_OK);

        rc = test_destroyClientAndSession(hClient, hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

void test_DeletedLocalTransaction_Phase1(void)
{
    test_DeletedTransaction_Phase1(false);
}

void test_DeletedLocalTransaction_Phase2(void)
{
    test_DeletedTransaction_Phase2(false);
}

void test_DeletedGlobalTransaction_Phase1(void)
{
    test_DeletedTransaction_Phase1(true);
}

void test_DeletedGlobalTransaction_Phase2(void)
{
    test_DeletedTransaction_Phase2(true);
}

/*********************************************************************/
/* TestCase:       GlobalTran_Rehydration                            */
/* Description:    Test XA transactions are rehydrated successfully. */
/*********************************************************************/
void test_GlobalTranRehydration_Phase1( void )
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
    char subName[50];

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create a client and session
    rc = test_createClientAndSession("CLIENT_REHDRATION",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient1,
                                     &hSession1,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create some subscriptions
    for(uint32_t i=0; i<8; i++)
    {
        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE | ((i%4)+1) };

        sprintf(subName, "SUB%u", i);

        rc = sync_ism_engine_createSubscription(hClient1, subName,
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                "GLOBALTRAN/REHDRATION/TEST",
                                                &subAttrs,
                                                NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x12C13311;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x953122FD11DD43E1;
    globalId.bqual = 0xABCDEF0123456789;

    for(uint32_t i=0; i<10; i++)
    {
        globalId.bqual += i;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        // Create a global transaction
        rc = sync_ism_engine_createGlobalTransaction(hSession1,
                                                     &XID,
                                                     ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                     &hTran);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hTran);

        // Publish a persistent message
        ismEngine_MessageHandle_t hMessage;
        void *pPayload=NULL;
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, "GLOBALTRAN/REHDRATION/TEST",
                                &hMessage,&pPayload);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = sync_ism_engine_putMessageOnDestination(hSession1,
                                                ismDESTINATION_TOPIC,
                                                "GLOBALTRAN/REHDRATION/TEST",
                                                hTran,
                                                hMessage);
        TEST_ASSERT_EQUAL(rc, OK);

        free(pPayload);

        switch(i)
        {
            // Unended transaction
            case 0:
                break;
            // Suspended transaction
            case 1:
                rc = ism_engine_endTransaction(hSession1,
                                               hTran,
                                               ismENGINE_END_TRANSACTION_OPTION_XA_TMSUSPEND,
                                               NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);
                break;
            // Failed unprepared transaction
            case 2:
                rc = ism_engine_endTransaction(hSession1,
                                               hTran,
                                               ismENGINE_END_TRANSACTION_OPTION_XA_TMFAIL,
                                               NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);
                break;
            // Successful unprepared transaction
            case 3:
                rc = ism_engine_endTransaction(hSession1,
                                               hTran,
                                               ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                               NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);
                break;
            // destroy subscription, prepared transaction
            case 6: // simpQ
            case 7: // intermediateQ
            case 8: // intermediateQ
            case 9: // multiConsumerQ
                sprintf(subName, "SUB%u", i-6);
                rc = ism_engine_destroySubscription(hClient1, subName, hClient1, NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);
                /* BEAM suppression: fall through */
            // prepared transaction
                /* no break - fall through */
            case 4:
            case 5:
                rc = ism_engine_endTransaction(hSession1,
                                               hTran,
                                               ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                               NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);
                rc = sync_ism_engine_prepareGlobalTransaction(hSession1, &XID);
                TEST_ASSERT_EQUAL(rc, OK);
                break;

        }
    }

    test_dumpTransactions(NULL, false);
}

void test_GlobalTranRehydration_Phase2( void )
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    char subName[50];

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // Recreate client and session
    rc = test_createClientAndSession("CLIENT_REHDRATION",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient1,
                                     &hSession1,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Find the transaction
    ism_xid_t XIDArray[20] = {{0}};

    // None are yet prepared (using implicit start)
    uint32_t found = ism_engine_XARecover(hSession1,
                                          XIDArray,
                                          sizeof(XIDArray)/sizeof(XIDArray[0]),
                                          0,
                                          ismENGINE_XARECOVER_OPTION_XA_TMNOFLAGS);

    TEST_ASSERT_EQUAL(found, 6);

    // Check they all have the right values
    for(uint32_t i=0; i<found; i++)
    {
        memcpy(&globalId, XIDArray[i].data, sizeof(globalId));
        TEST_ASSERT_EQUAL(globalId.gtrid, 0x953122FD11DD43E1);
    }

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x12C13311;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x953122FD11DD43E1;
    globalId.bqual = 0xABCDEF0123456789;

    for(uint32_t i=0; i<10; i++)
    {
        globalId.bqual += i;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        rc = ietr_findGlobalTransaction(pThreadData, &XID, &hTran);

        switch(i)
        {
            // Not expecting to find a transaction
            case 0:
            case 1:
            case 2:
            case 3:
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
                break;
            // Find the transaction and commit it
            case 4:
            case 6:
            case 7:
            case 9:
                TEST_ASSERT_EQUAL(rc, OK);
                ietr_releaseTransaction(pThreadData, hTran);

                rc = sync_ism_engine_commitGlobalTransaction(hSession1,
                                                             &XID,
                                                             ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS);
                TEST_ASSERT_EQUAL(rc, OK);

                // Check that an appropriate return code is returned if we try and commit again -
                // depending on the timing, it will either be an invalid operation (because we are
                // trying to commit a transaction that is already committed) or it will be not found.
                do
                {
                    rc = ism_engine_commitGlobalTransaction(hSession1,
                                                            &XID,
                                                            ismENGINE_COMMIT_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                            NULL, 0, NULL);
                }
                while(rc == ISMRC_InvalidOperation);

                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
                break;
            // Find the transaction and roll it back
            case 5:
            case 8:
                TEST_ASSERT_EQUAL(rc, OK);
                ietr_releaseTransaction(pThreadData, hTran);

                rc = ism_engine_rollbackGlobalTransaction(hSession1,
                                                          &XID,
                                                          NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);

                // Check it's no longer in the transaction table
                do
                {
                    rc = ism_engine_rollbackGlobalTransaction(hSession1,
                                                              &XID,
                                                              NULL, 0, NULL);
                }
                while(rc == ISMRC_InvalidOperation);

                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
                break;
        }
    }

    // Should still have a valid session, destroy the remaining subscriptions
    for(uint32_t i=4; i<8; i++)
    {
        sprintf(subName, "SUB%u", i);

        rc = ism_engine_destroySubscription(hClient1, subName, hClient1, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroyClientAndSession(hClient1, hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* TestCase:       GlobalTran_Defect_44264                           */
/*********************************************************************/
typedef struct callbackContext
{
    volatile uint32_t               MessagesReceived;
    ismEngine_ClientStateHandle_t   hClient;
    ismEngine_SessionHandle_t       hSession;
    ismEngine_TransactionHandle_t   hTran;
    volatile uint32_t               ackBatchesStarted;
    volatile uint32_t               ackBatchesCompleted;
} callbackContext_t;

static void completedBatchAck(int rc, void *handle, void *context)
{
    callbackContext_t *pCallbackContext = *(callbackContext_t **)context;
    __sync_fetch_and_add(&(pCallbackContext->ackBatchesCompleted), 1);
}

/*
 * Message-delivery callbacks used by this unit test
 */
static bool deliveryCallback(
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
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t rc = OK;

    // We got a message!
    pCallbackContext->MessagesReceived++;
    pCallbackContext->ackBatchesStarted++;
    // Confirm it, as a batch in the transaction
    rc = ism_engine_confirmMessageDeliveryBatch(pCallbackContext->hSession,
                                                pCallbackContext->hTran,
                                                1,
                                                &hDelivery,
                                                ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                &(pCallbackContext), sizeof(pCallbackContext),
                                                completedBatchAck);
    TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                      ("rc was %d\n", rc));

    if (rc == OK)
    {
        completedBatchAck(rc, NULL, &pCallbackContext);
    }


    ism_engine_releaseMessage(hMessage);

    return true;
}

void test_GlobalTran_Defect_44264_Phase1( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Create a client and session
    rc = test_createClientAndSession("DEFECT_44264",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient1,
                                     &hSession1,
                                     true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a transaction capable subscription
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    rc = sync_ism_engine_createSubscription(hClient1, "DEFECT_44264_SUB",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "GLOBALTRAN/DEFECT_44264",
                                            &subAttrs,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a global transaction
    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x12C13311;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x0123456789ABCDEF;
    globalId.bqual = 0xABCDEF0123456789;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    // Create a global transaction
    rc = sync_ism_engine_createGlobalTransaction(hSession1,
                                            &XID,
                                            ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                            &hTran );
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran);

    // Publish a persistent message
    ismEngine_MessageHandle_t hMessage;
    void *pPayload=NULL;
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, "GLOBALTRAN/DEFECT_44264",
                            &hMessage,&pPayload);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish a message (non-transactionally)
    rc = sync_ism_engine_putMessageOnDestination(hSession1,
                                            ismDESTINATION_TOPIC,
                                            "GLOBALTRAN/DEFECT_44264",
                                            NULL,
                                            hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    free(pPayload);

    // Create a consumer
    callbackContext_t callbackContext = {0};
    callbackContext.hClient = hClient1;
    callbackContext.hSession = hSession1;
    callbackContext.hTran = hTran;
    callbackContext_t *pCallbackContext = &callbackContext;

    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "DEFECT_44264_SUB",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    test_waitForMessages(&(callbackContext.MessagesReceived), NULL, 1, 20);
    TEST_ASSERT_EQUAL(callbackContext.MessagesReceived, 1);

    //TODO: Should we have to do this??? end+prepare should wait
    test_waitForMessages(&(callbackContext.ackBatchesCompleted), NULL,
                                    callbackContext.ackBatchesStarted, 20);

    // End and prepare transaction
    rc = ism_engine_endTransaction(hSession1,
                                   hTran,
                                   ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = sync_ism_engine_prepareGlobalTransaction(hSession1, &XID);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_GlobalTran_Defect_44264_Phase2( void )
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_ConsumerHandle_t hConsumer;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...", __func__);

    // Recreate client and session
    rc = test_createClientAndSession("DEFECT_44264",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient1,
                                     &hSession1,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Find the transaction
    ism_xid_t XIDArray[5] = {{0}};

    uint32_t found = ism_engine_XARecover(hSession1,
                                          XIDArray,
                                          sizeof(XIDArray)/sizeof(XIDArray[0]),
                                          0,
                                          ismENGINE_XARECOVER_OPTION_XA_TMNOFLAGS);
    TEST_ASSERT_EQUAL(found, 1);

    // Roll back the transaction so we can see the message again
    rc = ism_engine_rollbackGlobalTransaction(hSession1,
                                              &XIDArray[0],
                                              NULL, 0, NULL);

    // Check that the transaction has now gone
    rc = ism_engine_rollbackGlobalTransaction(hSession1,
                                              &XIDArray[0],
                                              NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // Reconsume...
    callbackContext_t callbackContext = {0};
    callbackContext.hClient = hClient1;
    callbackContext.hSession = hSession1;
    callbackContext.hTran = NULL;
    callbackContext_t *pCallbackContext = &callbackContext;

    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "DEFECT_44264_SUB",
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Owning client same as session client
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(callbackContext.MessagesReceived, 0); // Message delivery not started

    rc = ism_engine_startMessageDelivery(hSession1, ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_waitForMessages(&(callbackContext.MessagesReceived), NULL, 1, 20);
    TEST_ASSERT_EQUAL(callbackContext.MessagesReceived, 1);

    rc = sync_ism_engine_destroyConsumer(hConsumer);
    TEST_ASSERT_EQUAL(rc, OK);

    // Destroy the subscription
    rc = ism_engine_destroySubscription(hClient1, "DEFECT_44264_SUB", hClient1, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Destroy the client & Session
    rc = test_destroyClientAndSession(hClient1, hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

CU_TestInfo ISM_TransactionRecovery_CUnit_test_Phase1[] =
{
    { "DeletedGlobalPhase1", test_DeletedGlobalTransaction_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TransactionRecovery_CUnit_test_Phase2[] =
{
    { "DeletedGlobalPhase2", test_DeletedGlobalTransaction_Phase2 },
    { "GlobalTranRehydrationPhase1", test_GlobalTranRehydration_Phase1 },
    { "DeletedLocalPhase1", test_DeletedLocalTransaction_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TransactionRecovery_CUnit_test_Phase3[] =
{
    { "GlobalTranRehydrationPhase2", test_GlobalTranRehydration_Phase2 },
    { "GlobalTranDefect44264Phase1", test_GlobalTran_Defect_44264_Phase1 },
    { "DeletedLocalPhase2", test_DeletedLocalTransaction_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_TransactionRecovery_CUnit_test_Phase4[] =
{
    { "GlobalTranDefect44264Phase2", test_GlobalTran_Defect_44264_Phase2 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initPhaseCold(void)
{
    return test_engineInit(true, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false,
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           1024);
}

int termPhaseCold(void)
{
    return test_engineTerm(true);
}

//For phases after phase 0
int initPhaseWarm(void)
{
    return test_engineInit(false, true,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false,
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           1024);
}
//For phases after phase 0
int termPhaseWarm(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_TransactionRecovery_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("Phase1", initPhaseCold, termPhaseWarm, ISM_TransactionRecovery_CUnit_test_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TransactionRecovery_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("Phase2", initPhaseWarm, termPhaseWarm, ISM_TransactionRecovery_CUnit_test_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TransactionRecovery_CUnit_phase3suites[] =
{
    IMA_TEST_SUITE("Phase3", initPhaseWarm, termPhaseWarm, ISM_TransactionRecovery_CUnit_test_Phase3),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_TransactionRecovery_CUnit_phase4suites[] =
{
    IMA_TEST_SUITE("Phase4", initPhaseWarm, termPhaseCold, ISM_TransactionRecovery_CUnit_test_Phase4),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_TransactionRecovery_CUnit_phase1suites
                              , ISM_TransactionRecovery_CUnit_phase2suites
                              , ISM_TransactionRecovery_CUnit_phase3suites
                              , ISM_TransactionRecovery_CUnit_phase4suites };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
