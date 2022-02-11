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
/********************************************************************/
/*                                                                  */
/* Module Name: test_moreLateThings                                 */
/*                                                                  */
/* Description: This test attempts to:                              */
/*                  * engineer a queue that has a message rehydrated*/
/*                    with an earlier orderId than the messages on  */
/*                    the queue.                                    */
/*                                                                  */
/*                  * failover with a prepared transaction with     */
/*                    with a get of a message from an earlier       */
/*                    generation.                                   */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <getopt.h>

#include <ismutil.h>
#include "engine.h"
#include "engineInternal.h"
#include "engineCommon.h"
#include "policyInfo.h"
#include "queueCommon.h"
#include "topicTree.h"
#include "queueNamespace.h"

#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_log.h"
#include "test_utils_task.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"

/********************************************************************/
/* Function prototypes                                              */
/********************************************************************/
int ProcessArgs( int argc
               , char **argv
               , int *phase
               , bool *doRestart
               , char **padminDir );

/********************************************************************/
/* Global data                                                      */
/********************************************************************/
static uint32_t logLevel = testLOGLEVEL_TESTPROGRESS;

#define createLateMsgPhase          0 // Creation phase
#define verifyLateMsgPhase          1 // Check rehydration worked ok
#define createPreparedLateGetPhase  2 // create a prepared tran with a get from an earlier gen
#define verifyPreparedLateGetPhase  3 // Check rehydration worked ok

//Some globals for the Late Message test
#define testLateMsgClientId "LateMessage Test"
char *LateMsgQueueName="RehydrateMsgLate_Queue";
char *lateMsgPayload = "This is a message with a low orderId that was put very late!";
uint64_t MsgsReceivedinLateMsgTest = 0;

bool lateMessageDeliveryCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
                                 void *                          pContext,
                                 ismEngine_DelivererContext_t *  _delivererContext )
{
    int64_t payLoadAreaNum = -1;
    bool wantMoreMsgs = true;

    for(int32_t i=0; i<areaCount; i++)
    {
        if (areaTypes[i] == ismMESSAGE_AREA_PAYLOAD)
        {
            payLoadAreaNum = i;
        }
    }
    TEST_ASSERT(payLoadAreaNum >=0 , ("%ld < 0", payLoadAreaNum));

    uint64_t oldMsgsRecvd = __sync_fetch_and_add(&MsgsReceivedinLateMsgTest, 1);
    if (oldMsgsRecvd == 0)
    {
        //Should be the first "late" message
        TEST_ASSERT(areaLengths[payLoadAreaNum] == (1+strlen(lateMsgPayload)), ("areaLengths[\"%ld\"] was %lu, expected %lu",
                                                    payLoadAreaNum, areaLengths[payLoadAreaNum], (1+strlen(lateMsgPayload))  ));
        TEST_ASSERT(strcmp(pAreaData[payLoadAreaNum], lateMsgPayload) == 0,("First message was %s", pAreaData[payLoadAreaNum]));
    }
    else
    {
        //Shouldn't be first "late" message... and the string for that has a '!' so won't have been randomly generated...
        TEST_ASSERT(strcmp(pAreaData[payLoadAreaNum], lateMsgPayload) != 0,("Received the payload for the first message as message %d", oldMsgsRecvd+1));
        wantMoreMsgs = false;
    }
    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(((ismEngine_Consumer_t *)hConsumer)->pSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        NULL,0,NULL);
        TEST_ASSERT((rc == OK||rc == ISMRC_AsyncCompletion), ("rc was %d", rc));
    }
    ism_engine_releaseMessage(hMessage);

    return wantMoreMsgs;
}


int32_t createLateMessage(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // Create the queue
    int32_t rc = ieqn_createQueue( pThreadData
                                 , LateMsgQueueName
                                 , intermediate //as we can specify order ids and put messages with oIds out of order
                                 , ismQueueScope_Server, NULL
                                 , NULL
                                 , NULL
                                 , NULL );
    TEST_ASSERT(rc == OK, ("Failed to create queue, rc = %d", rc));

    //Put lots of messages to the queue, specifying orderId (and skipping orderId 1)
    ismStore_GenId_t genofFirstMsg = 0;
    ismStore_GenId_t genofCurrentMsg = 0;
    ismQHandle_t hQueue =  ieqn_getQueueHandle(pThreadData, LateMsgQueueName);
    uint64_t currOrderId = 2;

    do
    {
        ismEngine_MessageHandle_t hBigMsg;

        rc = test_createMessage(1000000, // 1MB
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_NONE, NULL,
                                &hBigMsg, NULL);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

        //Override the orderId
        ((ismEngine_Message_t *)hBigMsg)->Header.OrderId = currOrderId;

        rc = ieq_put( pThreadData
                    , hQueue
                    , ieqPutOptions_SET_ORDERID
                    , NULL
                    , hBigMsg
                    , IEQ_MSGTYPE_INHERIT
                    , NULL );

        //We're going to look at hBigMsg - in a real appliance it could have been got and freed
        //by now but we know there is no consumer
        if (0 == genofFirstMsg)
        {
            ism_store_getGenIdOfHandle((((ismEngine_Message_t *)hBigMsg)->StoreMsg.parts.hStoreMsg),
                                       &genofFirstMsg);
        }
        else
        {
            ism_store_getGenIdOfHandle((((ismEngine_Message_t *)hBigMsg)->StoreMsg.parts.hStoreMsg),
                                       &genofCurrentMsg);
        }


        TEST_ASSERT(rc == 0, ("Failed to put message with orderId %d - rc = %d", currOrderId,  rc));

        currOrderId++;
    }
    while( (genofCurrentMsg == 0) || (genofFirstMsg == genofCurrentMsg) );

    test_log(testLOGLEVEL_TESTPROGRESS, "Put messages 2-%lu, now going to put the message with oId 1\n",(currOrderId-1) );

    //Now we're in a different generation, create the message with oId 1
    ismEngine_MessageHandle_t hLateMsg;
    rc = test_createMessage(1+strlen(lateMsgPayload),
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_NONE, NULL,
                            &hLateMsg, (void **)&lateMsgPayload);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    //Override the orderId
    ((ismEngine_Message_t *)hLateMsg)->Header.OrderId = 1;

    rc = ieq_put( pThreadData
                , hQueue
                , ieqPutOptions_SET_ORDERID
                , NULL
                , hLateMsg
                , IEQ_MSGTYPE_INHERIT 
                , NULL );

    //Now restart and verify we get the messages we expect

    rc = test_engineTerm(false);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
    test_processTerm(false);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    return rc;
}

int32_t verifyLateMessage(void)
{
    ismEngine_SessionHandle_t hSession;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_ConsumerHandle_t hConsumer;

    int32_t rc = test_createClientAndSession(testLateMsgClientId,
                                             NULL,
                                             ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                             &hClient,
                                             &hSession,
                                             true);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , LateMsgQueueName
                                  , ismENGINE_SUBSCRIPTION_OPTION_NONE
                                  , hClient
                                  , NULL
                                  , 0
                                  , lateMessageDeliveryCallback
                                  , NULL
                                  , ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID
                                  , &hConsumer
                                  , NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    while (MsgsReceivedinLateMsgTest < 2)
    {
        usleep(30);
    }

    rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    rc = test_engineTerm(true);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
    test_processTerm(true);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    return rc;
}

/*********************************************************************/
/* TestCase:      testPreparedLateGet                                */
/*********************************************************************/
typedef struct testPreparedLateGetCallbackContext_t
{
    volatile int32_t                MessagesReceived;
    volatile int32_t                MessagesAcked;
    ismEngine_ClientStateHandle_t   hClient;
    ismEngine_SessionHandle_t       hSession;
    ismEngine_TransactionHandle_t   hTran;
} testPreparedLateGetCallbackContext_t;

//Some globals for the Late Prepared Get test
#define testPreparedLateGetClientId "LatePreparedGet Test"
char *PreparedLateGetQueueName="RehydratePreparedGet_Queue";

static void ackCompleted(int rc, void *handle, void *context)
{
    testPreparedLateGetCallbackContext_t **ppCallbackContext = (testPreparedLateGetCallbackContext_t **)context;
    testPreparedLateGetCallbackContext_t *pCallbackContext = *ppCallbackContext;

    // We finished with the message!
    __sync_fetch_and_add(&(pCallbackContext->MessagesAcked), 1);
}

static bool PrepareLateCallback(
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
    void *                          pConsumerContext,
    ismEngine_DelivererContext_t *  _delivererContext )
{
    testPreparedLateGetCallbackContext_t **ppCallbackContext = (testPreparedLateGetCallbackContext_t **)pConsumerContext;
    testPreparedLateGetCallbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t rc = OK;
    bool wantMoreMessages = true;


    // We got a message!
    __sync_fetch_and_add(&(pCallbackContext->MessagesReceived), 1);

    //If it's the first one... just consume it... we want the message in the have to have orderId 2 (to make it different to
    //orderid of tran ref

    // Do different things depending on how many messages we have received
    switch(pCallbackContext->MessagesReceived)
    {
        // Confirm outside of a transaction
        case 1:
        case 7:
        case 8:
            test_log(testLOGLEVEL_TESTPROGRESS, "Consuming (not in tran) message with oId: %lu", pMsgDetails->OrderId );
            // Confirm it, NOT as part of a transaction
            rc = ism_engine_confirmMessageDeliveryBatch(pCallbackContext->hSession,
                                                        NULL,
                                                        1,
                                                        &hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        &pCallbackContext,
                                                        sizeof(testPreparedLateGetCallbackContext_t *),
                                                        ackCompleted);
            TEST_ASSERT((rc == OK || rc == ISMRC_AsyncCompletion), ("rc is %d", rc));
            break;
        // Confirm as part of the transaction
        case 2:
        case 3:
        case 4:
        case 9:
        case 10:
            test_log(testLOGLEVEL_TESTPROGRESS, "Message for prepared tran: %lu", pMsgDetails->OrderId );

            // Confirm it, as a batch in the transaction
            rc = ism_engine_confirmMessageDeliveryBatch(pCallbackContext->hSession,
                                                        pCallbackContext->hTran,
                                                        1,
                                                        &hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        &pCallbackContext,
                                                        sizeof(testPreparedLateGetCallbackContext_t *),
                                                        ackCompleted);
            TEST_ASSERT((rc == OK || rc == ISMRC_AsyncCompletion), ("rc is %d", rc));
            break;
        // DO NOTHING
        case 5:
        case 6:
            break;
        // Stop delivering more messages
        default:
            wantMoreMessages = false;
            break;
    }

    ism_engine_releaseMessage(hMessage);

    return wantMoreMessages;
}

void checkAsyncCBStats(void)
{
    int32_t rc;
    char *pDiagnosticsOutput = NULL;

    rc = ism_engine_diagnostics(
            "AsyncCBStats", "",
            &pDiagnosticsOutput,
            NULL, 0, NULL);

    TEST_ASSERT(rc == OK, ("Getting AsyncCBStats failed, rc = %d", rc));
    TEST_ASSERT(pDiagnosticsOutput != NULL,
            ("Getting AsyncCBStats: Output was NULL"));

    test_log(testLOGLEVEL_VERBOSE, "AsyncCBStats: %s\n", pDiagnosticsOutput );

    //What can we check? - very little esp. if persistence is not enabled
    TEST_ASSERT(strstr(pDiagnosticsOutput,"TotalReadyCallbacks") != NULL,
             ("AsyncCBStats didn't include TotalReadyCallbacks"));

    TEST_ASSERT(strstr(pDiagnosticsOutput,"TotalWaitingCallbacks") != NULL,
             ("AsyncCBStats didn't include TotalWaitingCallbacks"));

    if(usingDiskPersistence)
    {
        //We should be reporting stats from at least one async CB thread

        TEST_ASSERT(strstr(pDiagnosticsOutput,"ThreadId") != NULL,
                 ("AsyncCBStats didn't include ThreadId"));
        TEST_ASSERT(strstr(pDiagnosticsOutput,"PeriodSeconds") != NULL,
                 ("AsyncCBStats didn't include PeriodSeconds"));
    }
    ism_engine_freeDiagnosticsOutput(pDiagnosticsOutput);
}

int32_t createPreparedLateGet(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // Create the queue
    int32_t rc = ieqn_createQueue( pThreadData
                                 , PreparedLateGetQueueName
                                 , multiConsumer
                                 , ismQueueScope_Server, NULL
                                 , NULL
                                 , NULL
                                 , NULL );
    TEST_ASSERT(rc == OK, ("Failed to create queue, rc = %d", rc));

    //Put lots of messages to the queue until we have a number of generations in the store
    ismStore_GenId_t genofFirstMsg = 0;
    ismStore_GenId_t genofCurrentMsg = 0;
    ismQHandle_t hQueue =  ieqn_getQueueHandle(pThreadData, PreparedLateGetQueueName);

    do
    {
        ismEngine_MessageHandle_t hBigMsg;

        rc = test_createMessage(1000000, // 1MB
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_NONE, NULL,
                                &hBigMsg, NULL);
        TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

        rc = ieq_put( pThreadData
                    , hQueue
                    , ieqPutOptions_NONE
                    , NULL
                    , hBigMsg
                    , IEQ_MSGTYPE_INHERIT
                    , NULL );

        //We're going to look at hBigMsg - in a real appliance it could have been got and freed
        //by now but we know there is no consumer
        if (0 == genofFirstMsg)
        {
            ism_store_getGenIdOfHandle((((ismEngine_Message_t *)hBigMsg)->StoreMsg.parts.hStoreMsg),
                                       &genofFirstMsg);
        }
        else
        {
            ism_store_getGenIdOfHandle((((ismEngine_Message_t *)hBigMsg)->StoreMsg.parts.hStoreMsg),
                                       &genofCurrentMsg);
        }

        checkAsyncCBStats();

        TEST_ASSERT(rc == 0, ("Failed to put message rc = %d", rc));
    }
    while( (genofCurrentMsg == 0) || ((genofFirstMsg +3) >= genofCurrentMsg) );

    //Now we're in a different generation, create an XA transaction and get a message...
    ismEngine_SessionHandle_t hSession;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_ConsumerHandle_t hConsumer;

    rc = test_createClientAndSession(testLateMsgClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient,
                                     &hSession,
                                     true);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    // Create a global transaction
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0x12C13311;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 0x0123456789ABCDEF;
    globalId.bqual = 0xABCDEF0123456789;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    // Create a global transaction
    rc = sync_ism_engine_createGlobalTransaction(hSession,
                                                 &XID,
                                                 ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                 &hTran);
    TEST_ASSERT((rc == OK), ("rc is %d", rc));
    TEST_ASSERT((hTran != NULL), ("hTran was NULL"));

    testPreparedLateGetCallbackContext_t callbackContext = {0};
    callbackContext.hClient = hClient;
    callbackContext.hSession = hSession;
    callbackContext.hTran = hTran;
    testPreparedLateGetCallbackContext_t *pCallbackContext = &callbackContext;

    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , PreparedLateGetQueueName
                                  , ismENGINE_SUBSCRIPTION_OPTION_NONE
                                  , NULL // Owning client same as session client
                                  , &pCallbackContext
                                  , sizeof(testPreparedLateGetCallbackContext_t *)
                                  , PrepareLateCallback
                                  , NULL
                                  , ismENGINE_CONSUMER_OPTION_ACK
                                  , &hConsumer
                                  , NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    while(  (callbackContext.MessagesReceived < 11)
          &&(callbackContext.MessagesAcked    < 8))
    {
        usleep(200);
    }

    // End and prepare transaction
    rc = ism_engine_endTransaction(hSession,
                                   hTran,
                                   ismENGINE_END_TRANSACTION_OPTION_XA_TMSUCCESS,
                                   NULL, 0, NULL);
    TEST_ASSERT((rc == OK), ("rc is %d", rc));
    rc = sync_ism_engine_prepareGlobalTransaction(hSession, &XID);
    TEST_ASSERT((rc == OK), ("rc is %d", rc));

    rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    rc = test_engineTerm(false);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
    test_processTerm(false);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    return rc;
}

static bool CheckNotGotMsgCallback(
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
    void *                          pConsumerContext,
    ismEngine_DelivererContext_t *  _delivererContext )
{
    testPreparedLateGetCallbackContext_t **ppCallbackContext = (testPreparedLateGetCallbackContext_t **)pConsumerContext;
    testPreparedLateGetCallbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t rc = OK;

    // We got a message!
    pCallbackContext->MessagesReceived++;

    test_log(testLOGLEVEL_TESTPROGRESS, "Received message oId: %lu (should have oId that was not in the transaction)", pMsgDetails->OrderId );

    //We should NOT get the message that was in the prepared (then commited) transaction
    switch(pMsgDetails->OrderId)
    {
        case 5:
        case 6:
        case 11:
        case 12:
        case 13:
            break;
        default:
            TEST_ASSERT(false,
                        ("pMsgDetails->OrderId was %lu but that message should have been consumed (live or by XA transaction)" ,pMsgDetails->OrderId));
            break;
    }

    // Confirm it
    rc = ism_engine_confirmMessageDeliveryBatch(pCallbackContext->hSession,
                                                NULL,
                                                1,
                                                &hDelivery,
                                                ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                NULL, 0, NULL);
    TEST_ASSERT((rc == OK || rc == ISMRC_AsyncCompletion), ("rc is %d", rc));

    ism_engine_releaseMessage(hMessage);

    return (pCallbackContext->MessagesReceived < 5); //We only want five messages
}

int32_t verifyPreparedLateGet(void)
{
    ismEngine_SessionHandle_t hSession;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_ConsumerHandle_t hConsumer;

    int32_t rc = test_createClientAndSession(testLateMsgClientId,
                                             NULL,
                                             ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                             &hClient,
                                             &hSession,
                                             true);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    // Find the transaction
    ism_xid_t XIDArray[5] = {{0}};

    uint32_t found = ism_engine_XARecover(hSession,
                                          XIDArray,
                                          sizeof(XIDArray)/sizeof(XIDArray[0]),
                                          0,
                                          ismENGINE_XARECOVER_OPTION_XA_TMNOFLAGS);
    TEST_ASSERT((found == 1),("found XA transactions: %u", found));

    // Commit the prepared get - it should no longer be on the queue
    rc = sync_ism_engine_commitGlobalTransaction(hSession,
                    &XIDArray[0],
                    ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);

    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    testPreparedLateGetCallbackContext_t callbackContext = {0};
    callbackContext.hClient = hClient;
    callbackContext.hSession = hSession;
    callbackContext.hTran = NULL;
    testPreparedLateGetCallbackContext_t *pCallbackContext = &callbackContext;

    rc = ism_engine_createConsumer( hSession
                                    , ismDESTINATION_QUEUE
                                    , PreparedLateGetQueueName
                                    , ismENGINE_SUBSCRIPTION_OPTION_NONE
                                    , NULL // Owning client same as session client
                                    , &pCallbackContext
                                    , sizeof(testPreparedLateGetCallbackContext_t *)
                                    , CheckNotGotMsgCallback
                                    , NULL
                                    , ismENGINE_CONSUMER_OPTION_ACK
                                    , &hConsumer
                                    , NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    TEST_ASSERT((rc == OK), ("rc is %d", rc));
    test_waitForMessages((volatile uint32_t *)&(callbackContext.MessagesReceived),
                         NULL, 5, 20);
    TEST_ASSERT((callbackContext.MessagesReceived == 5),
                ("msgs received: %d", callbackContext.MessagesReceived));

    rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    rc = test_engineTerm(false);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));
    test_processTerm(false);
    TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

    return rc;
}

/********************************************************************/
/* MAIN                                                             */
/********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 0;
    int rc;
    int phase;
    bool doNextPhase = true;
    char *adminDir = NULL;

    /************************************************************************/
    /* Parse the command line arguments                                     */
    /************************************************************************/
    rc = ProcessArgs( argc
                    , argv
                    , &phase
                    , &doNextPhase
                    , &adminDir );

    if (rc != OK)
    {
        return rc;
    }

#ifdef ENGINE_FORCE_INTERMEDIATE_TO_MULTI
    //MulticonsumerQ doesn't support putting messages with orderIds out of order and the first
    //test (phases 0 and 1) rely on it so we have to skip them...
    if (phase < createPreparedLateGetPhase)
    {
        phase = createPreparedLateGetPhase;
    }
#endif

    test_setLogLevel(logLevel);

    rc = test_processInit(trclvl, adminDir);
    if (rc != OK) return rc;

    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    rc =  test_engineInit(  (phase == createLateMsgPhase)
                          ||(phase == createPreparedLateGetPhase),
                          true,
                          false,
                          false, /*recovery should complete ok*/
                          ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                          50);  // 50mb Store
    if (rc != OK) return rc;

    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 100000000;


    if (phase == createLateMsgPhase)
    {
        rc = createLateMessage();
    }
    else if (phase == verifyLateMsgPhase)
    {
        rc = verifyLateMessage();
    }
    else if (phase == createPreparedLateGetPhase)
    {
        rc = createPreparedLateGet();
    }
    else
    {
        TEST_ASSERT(phase == verifyPreparedLateGetPhase, ("Unexpected phase. Expected %d, got %d", verifyPreparedLateGet, phase));
        rc = verifyPreparedLateGet();
        doNextPhase = false;
    }

    if ((doNextPhase) && (rc == OK))
    {
        phase++;

        fprintf(stdout, "== Restarting for phase %d\n", phase);

        // Re-execute ourselves for the next phase
        int32_t newargc = 0;

        char *newargv[argc+6];

        newargv[newargc++] = argv[0];

        for(int32_t i=newargc; i<argc; i++)
        {
            if ((strcmp(argv[i], "-p") == 0) || (strcmp(argv[i], "-a") == 0))
            {
                i++;
            }
            else
            {
                newargv[newargc++] = argv[i];
            }
        }

        char phaseString[50];
        sprintf(phaseString, "%d", phase);

        newargv[newargc++] = "-p";
        newargv[newargc++] = phaseString;
        newargv[newargc++] = "-a";
        newargv[newargc++] = adminDir;
        newargv[newargc] = NULL;

        printf("== Command: ");
        for(int32_t i=0; i<newargc; i++)
        {
            printf("%s ", newargv[i]);
        }
        printf("\n");

        rc = test_fork_and_execvp(newargv[0], newargv);
    }

    //test_engineTerm(true);
    //test_processTerm(false);
    test_removeAdminDir(adminDir);

    return rc;
}

/****************************************************************************/
/* ProcessArgs                                                              */
/****************************************************************************/
int ProcessArgs( int argc
               , char **argv
               , int *phase
               , bool *doRestart
               , char **padminDir )
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { NULL, 1, NULL, 0 } };
    int long_index;

    *phase = 0;
    *padminDir = NULL;

    while ((opt = getopt_long(argc, argv, "nv:p:a:", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case 'p':
                *phase = atoi(optarg);
                break;
            case 'n':
                *doRestart = false;
                break;
            case 'v':
               logLevel = atoi(optarg);
               if (logLevel > testLOGLEVEL_VERBOSE)
                   logLevel = testLOGLEVEL_VERBOSE;
               break;
            case 'a':
                *padminDir = optarg;
                break;
            case '?':
                usage=1;
                break;
            default: 
                printf("%c\n", (char)opt);
                usage=1;
                break;
        }
    }

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-v verbose] [-p <phase>] [-n] [-a adminDir]\n", argv[0]);
        fprintf(stderr, "       -v - logLevel 0-5 [2]\n");
        fprintf(stderr, "       -n - no restart into next phase\n");
        fprintf(stderr, "       -a - Admin directory to use\n");
        fprintf(stderr, "\n");
    }

    return usage;
}
