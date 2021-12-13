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

#include <stdio.h>
#include <getopt.h>
#include <sys/prctl.h>
#include <sys/stat.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "transaction.h"
#include "queueNamespace.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"

#if !defined(MIN)
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
static ismEngine_ClientStateHandle_t hClient = NULL;
static char QUEUE_NAME[]="TEST_GLOBALTRAN/Queue 1";
static int trclvl = 5;
static testLogLevel_t testLogLevel = testLOGLEVEL_TESTDESC;
static bool single = false;
static char *programName = NULL;
static int TestcaseNumber = 0;

/*********************************************************************/
/* Name:        Test_CreateQ                                         */
/* Description: This test function defines a queue with the given    */
/*              name.                                                */
/* Parameters:                                                       */
/*     type                - Type of queue to create                 */
/*     QueueName           - Name to give queue                      */
/*********************************************************************/
static void Test_CreateQ( ismQueueType_t type
                        , char *QueueName )
{ 
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    char QueueTypeBuf[16];

    test_log(testLOGLEVEL_TESTDESC, "Define a %s queue with name '%s'",
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)),
            QueueName);

    // Create the queue
    rc = ieqn_createQueue( pThreadData
                         , QueueName
                         , type
                         , ismQueueScope_Server, NULL
                         , NULL
                         , NULL
                         , NULL );
    TEST_ASSERT(rc == OK, ("Failed to create queue, rc = %d", rc));

    test_log(testLOGLEVEL_TESTPROGRESS, "Created queue");

    return;
}

typedef struct _Payload_t
{
    uint32_t TestNo;
    uint32_t Phase;
    uint64_t Value;
} Payload_t;

/*********************************************************************/
/* Name:        Test_PutMsg                                          */
/* Description: This function puts a message to the queue.           */
/* Parameters:                                                       */
/*     hSession            - Session handle                          */
/*     QueueName           - Name of destination queue               */
/*     hTran               - Transaction handle if transactional     */
/*     Value               - Value to put in message                 */
/*********************************************************************/
static void Test_PutMsg( ismEngine_SessionHandle_t hSession
                       , char *QueueName
                       , ismEngine_TransactionHandle_t hTran
                       , uint64_t Value
                       , uint32_t TestNo 
                       , uint32_t Phase )
{ 
    uint32_t rc;
    ismEngine_MessageHandle_t hMessage;
    Payload_t Payload;

    Payload.Value = Value;
    Payload.TestNo = TestNo;
    Payload.Phase = Phase;

    // Create the message
    void *payloadPtr = &Payload;
    rc = test_createMessage(sizeof(Payload_t),
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_JMS_PERSISTENT,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, QueueName,
                            &hMessage, &payloadPtr);
    TEST_ASSERT( rc == OK , ("message creation failed. rc = %d"));


    test_log(testLOGLEVEL_VERBOSE, "Putting message with value %d", Value);

    // And put the message
    rc = ism_engine_putMessageOnDestination( hSession
                                           , ismDESTINATION_QUEUE
                                           , QueueName
                                           , hTran
                                           , hMessage
                                           , NULL
                                           , 0
                                           , NULL );
    TEST_ASSERT( rc == OK , ("ism_engine_putMessageOnDestination failed. rc = %d"));

    return;
}

/*********************************************************************/
/* Name:        Test_PutMsgParts                                     */
/* Description: This function will put 'N' messages the sum of the   */
/*              payload will be equal to the provided total.         */
/* Parameters:                                                       */
/*     hSession            - Session handle                          */
/*     QueueName           - Queue name                              */
/*     hTran               - Transaction handle                      */
/*     MsgCount            - Number of messages to put               */
/*     Total               - Total of messages to put                */
/*     TestNo              - Testcase number                         */
/*     Phase               - Phase in which the message was put      */
/*********************************************************************/
static void Test_PutMsgParts( ismEngine_SessionHandle_t hSession
                            , char *QueueName
                            , ismEngine_TransactionHandle_t hTran
                            , uint32_t MsgCount
                            , uint64_t Total
                            , uint32_t TestNo 
                            , uint32_t Phase )
{ 
    uint32_t rc;
    uint64_t Remaining = Total;
    ismEngine_MessageHandle_t hMessage;
    Payload_t Payload;

    Payload.TestNo = TestNo;
    Payload.Phase = Phase;
    for ( uint32_t counter=0; counter < MsgCount; counter++)
    {
        if (counter == (MsgCount-1))
        {
            Payload.Value = Remaining;
        }
        else if (Remaining > 0)
        {
            Payload.Value = rand() % Remaining;
            Remaining -= Payload.Value;
        }
        else
        {
            Payload.Value = 0;
        }

        // Create the message
        void *payloadPtr = &Payload;
        rc = test_createMessage(sizeof(Payload_t),
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_JMS_PERSISTENT,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_QUEUE, QueueName,
                                &hMessage, &payloadPtr);
        TEST_ASSERT(rc == OK , ("message creation failed. rc = %d"));

        test_log(testLOGLEVEL_VERBOSE,
                 "Putting message %d with value %ld", counter, Payload.Value);

        // And put the message
        rc = ism_engine_putMessageOnDestination( hSession
                                               , ismDESTINATION_QUEUE
                                               , QueueName
                                               , hTran
                                               , hMessage
                                               , NULL
                                               , 0
                                               , NULL );
        if (rc != OK)
        {
            printf("DEBUG: View of queue after failed put\n");
            test_log_queue(testLOGLEVEL_VERBOSE, QueueName, 9, -1, "");
        }
        TEST_ASSERT( rc == OK , ("ism_engine_putMessageOnDestination failed. rc = %d", rc));
    }

    return;
}

typedef struct VerifyContext_t
{
    volatile uint64_t Total;
    volatile uint32_t MsgCount;
    ismEngine_SessionHandle_t hSession;
    ismEngine_TransactionHandle_t hTran;
    volatile uint32_t acksStarted;
    volatile uint32_t acksCompleted;
} VerifyContext_t;

static void completedAck(int rc, void *handle, void *context)
{
    VerifyContext_t *pContext = *(VerifyContext_t **)context;
    __sync_fetch_and_add(&(pContext->acksCompleted), 1);
}

/*********************************************************************/
/* Name:        Verify Callback                                      */
/* Description: Callback function invoked with messages              */
/*********************************************************************/
static bool VerifyCallback(ismEngine_ConsumerHandle_t  hConsumer,
                           ismEngine_DeliveryHandle_t  hDelivery,
                           ismEngine_MessageHandle_t   hMessage,
                           uint32_t                    deliveryId,
                           ismMessageState_t           state,
                           uint32_t                    destinationOptions,
                           ismMessageHeader_t *        pMsgDetails,
                           uint8_t                     areaCount,
                           ismMessageAreaType_t        areaTypes[areaCount],
                           size_t                      areaLengths[areaCount],
                           void *                      pAreaData[areaCount],
                           void *                      pConsumerContext,
                           ismEngine_DelivererContext_t * _delivererContext)
{
    uint32_t rc;
    VerifyContext_t *pContext = *(VerifyContext_t **)pConsumerContext;

    TEST_ASSERT( areaCount == 2,
                 ("Received %d areas. Expected 2!", areaCount) );
    TEST_ASSERT(areaLengths[1] == sizeof(Payload_t),
                ("Unsupported message length of %d", areaLengths[1]));

    Payload_t payload;
    TEST_ASSERT(sizeof(payload) == areaLengths[1], ("Unexpected payload length %lu", areaLengths[1]));
    memcpy(&payload, pAreaData[1], sizeof(payload));

    pContext->MsgCount++;
    pContext->Total += payload.Value;

    test_log(testLOGLEVEL_VERBOSE,
             "Received message. orderId(%lu) Testcase(%u) Phase(%u) Value(%lu) total-so-far %lu",
             pMsgDetails->OrderId, payload.TestNo, payload.Phase,
             payload.Value, pContext->Total);

    __sync_fetch_and_add(&(pContext->acksStarted), 1);
    rc =  ism_engine_confirmMessageDelivery( pContext->hSession
                                           , pContext->hTran
                                           , hDelivery
                                           , ismENGINE_CONFIRM_OPTION_CONSUMED
                                           , &pContext, sizeof(pContext)
                                           , completedAck);
    TEST_ASSERT(rc==OK|| rc == ISMRC_AsyncCompletion,
            ("ism_engine_confirmMessageDelivery failed rc=%d", rc));

    if (rc == OK)
    {
        completedAck(rc, NULL, &pContext);
    }

    ism_engine_releaseMessage(hMessage);

    return true;
}
    
/*********************************************************************/
/* Name:        Test_VerifyQueue                                     */
/* Description: This function verifies the message on the queue by   */
/*              summing the contents of all messages on the queue.   */
/*              The sum should match the initial value.              */
/* Parameters:                                                       */
/*     QueueName           - Name to give queue                      */
/*     hTran               - Tran to consume messages under          */
/*     expectedMsgCount    - Number of messages expected             */
/*     expectedValue       - Value of messages expected              */
/*********************************************************************/
static void Test_VerifyQueue( ismEngine_SessionHandle_t hSession
                            , ismEngine_TransactionHandle_t hTran
                            , uint32_t expectedMsgCount
                            , uint64_t expectedValue )
{
    uint32_t rc = OK;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    VerifyContext_t VerifyContext = { 0, 0, hSession, hTran };
    VerifyContext_t *pVerifyContext = &VerifyContext;

    /*************************************************************/
    /* And initialise the consumer                               */
    /*************************************************************/
    // printf("DEBUG: View of queue before verify\n");
    test_log_queue(testLOGLEVEL_VERBOSE, QUEUE_NAME, 9, -1, "");

    // initialise a consumer
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , QUEUE_NAME
                                  , &subAttrs
                                  , NULL // Unused for QUEUE
                                  , &pVerifyContext
                                  , sizeof(pVerifyContext)
                                  , VerifyCallback
                                  , NULL
                                  , ismENGINE_CONSUMER_OPTION_ACK
                                  , &hConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT(rc==OK, ("CreateConsumer failed rc=%d", rc));

    // start the delivery of messages
    rc = ism_engine_startMessageDelivery( hSession
                                        , 0 /* options */
                                        , NULL
                                        , 0
                                        , NULL );
    TEST_ASSERT(rc==OK, ("Start Message Delivery failed rc=%d", rc));
                                        
    test_waitForMessages(&(VerifyContext.MsgCount), NULL, expectedMsgCount, 20);
    test_waitForMessages64(&(VerifyContext.Total), NULL, expectedValue, 20);
    test_waitForMessages(&(VerifyContext.acksCompleted), NULL, VerifyContext.acksStarted, 20);

    // And stop message delivery. This call can go asynchronous, but as
    // all messages should have been delivered there is no reason for it
    // to go async.
    rc = ism_engine_stopMessageDelivery( hSession
                                       , NULL
                                       , 0
                                       , NULL );
    TEST_ASSERT(rc==OK, ("Stop Message Delivery failed rc=%d", rc));

    rc = sync_ism_engine_destroyConsumer( hConsumer );
    TEST_ASSERT(rc==OK, ("Destroy consumer failed rc=%d", rc));

    // And finally verify the total of the messages arrived equals
    // the initial value
    TEST_ASSERT(VerifyContext.MsgCount == expectedMsgCount,
               ("message count %d doesn't match expected message count %d",
                VerifyContext.MsgCount, expectedMsgCount));

    TEST_ASSERT(VerifyContext.Total == expectedValue,
               ("Total message value %ld for %d messages doesn't match initial value %ld",
                VerifyContext.Total, VerifyContext.MsgCount, expectedValue));

    test_log(testLOGLEVEL_CHECK, "Value of messages following restart verified as %ld", VerifyContext.Total);
 
    return;
}

/*********************************************************************/
/* restart the test program                                          */
/*********************************************************************/
void Test_restart( uint32_t testNo
                 , uint32_t phase )
{
    int argc = 0;
    char *argv[14] = { NULL };
    char textTest[8];
    char textPhase[8];
    char textLogLevel[8];
    char textTrace[8];
    char localAdminDir[1024];
    if (test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == false)
    {
        localAdminDir[0] = (char)0;
    }

    if (single)
    {
        test_log(3, "Ending test after 'single' test/phase.");
        exit(0);
    }

    sprintf(textTest, "%d", testNo);
    sprintf(textPhase, "%d", phase);
    sprintf(textTrace, "-%d", trclvl);
    sprintf(textLogLevel, "%d", testLogLevel);

    argv[argc++] = programName;
    argv[argc++] = "-t";
    argv[argc++] = textTest;
    argv[argc++] = "-p";
    argv[argc++] = textPhase;
    argv[argc++] = "-v";
    argv[argc++] = textLogLevel;
    argv[argc++] = textTrace;
    if (single)
    {
        argv[argc++] = "-s";
    }
    if (localAdminDir[0] != 0)
    {
        argv[argc++] = "-a";
        argv[argc++] = localAdminDir;
    }

    fflush(stdout);
    fflush(stderr);

    test_log(3, "Restarting");

    (void)test_execv(argv[0], argv);
    return;
}

void printXID(uint32_t Counter, ism_xid_t *pXID)
{
    char Buffer[XID_DATASIZE * 2];
    uint32_t i;

    for (i=0; i < (pXID->gtrid_length + pXID->bqual_length); i++)
    {
      sprintf(&Buffer[i*2], "%02x", pXID->data[i]);
    }
    Buffer[i*2]='\0';

    test_log(5, "%d: formatId:%ld Id(%ld):%.*s Branch(%ld):%.*s\n",
           Counter, pXID->formatID,
           pXID->gtrid_length, (int)pXID->gtrid_length * 2, Buffer,
           pXID->bqual_length, (int)pXID->bqual_length * 2, &Buffer[pXID->gtrid_length * 2]);
    return;
}

/*********************************************************************/
/*********************************************************************/
/* test1        - This phase put's 5 messages within a global        */
/*                transaction and the restarts the server. The       */
/*                transaction is not prepared or committed so upon   */
/*                restart there should be nothing left of the        */
/*                transaction.                                       */
/*********************************************************************/
/*********************************************************************/
void test1_phase1( void )
{
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    ismEngine_TransactionHandle_t hTran;

    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 1, 1);

    /*****************************************************************/
    /* Initialise the session                                        */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    /*****************************************************************/
    /* Create the global transaction to use                          */
    /*****************************************************************/
    test_log(5, "Creating transaction");
    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 1; // test
    globalId.bqual = 1;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc=sync_ism_engine_createGlobalTransaction( hSession
                                         , &XID
                                         , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                         , &hTran);
    TEST_ASSERT(rc==OK, ("Create global transaction failed rc=%d", rc));

    /*****************************************************************/
    /* Put 5 messages on the queue                                   */
    /*****************************************************************/
    Test_PutMsgParts( hSession
                    , QUEUE_NAME
                    , hTran
                    , 5      // 5 messages
                    , 12000  // Total value
                    , 1      // testcase number 
                    , 1 );   // phase

    test_log(3, "Test1 - Phase 1 - Put 5 messages");
    
    /*****************************************************************/
    /* Now restart the server                                        */
    /*****************************************************************/
    Test_restart( 1, 2 );

    return;
}

#define XID_BUFSIZE5 5
#define XID_BUFSIZE20 20

void test1_phase2( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    uint32_t XIDCount;
    ism_xid_t *XIDBuffer = NULL;
    uint32_t counter;
    
    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 1, 2);

    /*****************************************************************/
    /* Initialise the session                                        */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    XIDBuffer = (ism_xid_t *)calloc(sizeof(ism_xid_t), XID_BUFSIZE5);
    TEST_ASSERT(XIDBuffer!=NULL, ("Failed to allocate XID Buffer"));

    /*****************************************************************/
    /* Call recover, we do not expect any in-flight transactions     */
    /*****************************************************************/
    test_log(4, "Calling XARecover");
    XIDCount = ism_engine_XARecover( hSession
                                   , XIDBuffer
                                   , XID_BUFSIZE5
                                   , 1
                                   , ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN );

    for (counter = 0; counter < XIDCount; counter++)
    {
        printXID(counter, &XIDBuffer[counter]);
    }

    TEST_ASSERT(XIDCount==0, ("Number of global transaction not equal to zero (%d)", XIDCount));

    /*****************************************************************/
    /* And check that the queue depth is zero                        */
    /*****************************************************************/
    ismEngine_QueueStatistics_t stats;
    ismQHandle_t hQueue = ieqn_getQueueHandle(pThreadData, QUEUE_NAME);

    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

    test_log(4, "Current queue depth is %d", stats.BufferedMsgs);

    TEST_ASSERT(stats.BufferedMsgs == 0, ("The number of messages on the queue not zero (%d)", stats.BufferedMsgs));

    if (XIDBuffer != NULL)
    {
        free(XIDBuffer);
    }

    /*****************************************************************/
    /* Now restart the server                                        */
    /*****************************************************************/
    Test_restart( 2, 1 );

    return;
}


/*********************************************************************/
/*********************************************************************/
/* test2                                                             */
/*   - phase1 - put 18 messages within a global transaction and      */
/*              and prepares the transaction.                        */
/*   - phase2 - the XARecover call should report that there is one   */
/*              XID in prepared state.                               */
/*   - phase3 - again the XARecover call should report that there is */
/*              one XID in prepared state this time the transaction  */
/*              is committed.                                        */
/*   - phase4 - A consumer is then created to verify the messages.   */
/*********************************************************************/
/*********************************************************************/
void test2_phase1( void )
{
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    ismEngine_TransactionHandle_t hTran;

    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 2, 1);

    /*****************************************************************/
    /* Initialise the session                                        */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    /*****************************************************************/
    /* Create the global transaction to use                          */
    /*****************************************************************/
    test_log(5, "Creating transaction");
    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 2; // test
    globalId.bqual = 1;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc=sync_ism_engine_createGlobalTransaction( hSession
                                         , &XID
                                         , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                         , &hTran);
    TEST_ASSERT(rc==OK, ("Create global transaction failed rc=%d", rc));

    /*****************************************************************/
    /* Put 5 messages on the queue                                   */
    /*****************************************************************/
    Test_PutMsgParts( hSession
                    , QUEUE_NAME
                    , hTran
                    , 18     // 18 messages
                    , 12000  // Total value
                    , 2      // testcase number 
                    , 1 );   // phase

    test_log(4, "Put 5 messages");

    /*****************************************************************/
    /* End the transaction                                           */
    /*****************************************************************/
    rc = ism_engine_endTransaction( hSession
                                  , hTran
                                  , ismENGINE_END_TRANSACTION_OPTION_DEFAULT
                                  , NULL, 0, NULL);
    TEST_ASSERT(rc==OK, ("End transaction failed rc=%d", rc));

    /*****************************************************************/
    /* Prepare the transaction                                       */
    /*****************************************************************/
    rc = sync_ism_engine_prepareGlobalTransaction( hSession
                                                 , &XID);
    TEST_ASSERT(rc==OK, ("Prepare transaction failed rc=%d", rc));
    test_log(5, "Prepared the transaction");
    
    /*****************************************************************/
    /* Now restart the server                                        */
    /*****************************************************************/
    Test_restart( 2, 2 );

    return;
}

void test2_phase2( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    uint32_t XIDCount;
    ism_xid_t *XIDBuffer;
    uint32_t counter;
    
    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 2, 2);

    /*****************************************************************/
    /* Initialise the session                                        */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    XIDBuffer = (ism_xid_t *)calloc(sizeof(ism_xid_t), XID_BUFSIZE5);

    /*****************************************************************/
    /* Call recover, we do not expect any in-flight transactions     */
    /*****************************************************************/
    test_log(4, "Calling XARecover");
    XIDCount = ism_engine_XARecover( hSession
                                   , XIDBuffer
                                   , XID_BUFSIZE5
                                   , 1
                                   , ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN );

    for (counter = 0; counter < XIDCount; counter++)
    {
        printXID(counter, &XIDBuffer[counter]);
    }

    TEST_ASSERT(XIDCount==1, ("Number of global transaction not equal to 1 (%d)", XIDCount));

    /*****************************************************************/
    /* And check that the queue depth is zero                        */
    /*****************************************************************/
    ismEngine_QueueStatistics_t stats;
    ismQHandle_t hQueue = ieqn_getQueueHandle(pThreadData, QUEUE_NAME);

    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

    test_log(4, "Current queue depth is %d", stats.BufferedMsgs);

    TEST_ASSERT(stats.BufferedMsgs == 18, ("The number of messages on the queue not equal to 18", stats.BufferedMsgs));

    /*****************************************************************/
    /* Now restart the server                                        */
    /*****************************************************************/
    Test_restart( 2, 3 );

    return;
}

void test2_phase3( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    uint32_t XIDCount;
    ism_xid_t *XIDBuffer;
    uint32_t counter;
    ismEngine_TransactionHandle_t hTran;
    
    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 2, 3);

    /*****************************************************************/
    /* Initialise the session                                        */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    XIDBuffer = (ism_xid_t *)calloc(sizeof(ism_xid_t), XID_BUFSIZE5);

    /*****************************************************************/
    /* Call recover, we do not expect any in-flight transactions     */
    /*****************************************************************/
    test_log(4, "Calling XARecover");
    XIDCount = ism_engine_XARecover( hSession
                                   , XIDBuffer
                                   , XID_BUFSIZE5
                                   , 1
                                   , ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN );

    for (counter = 0; counter < XIDCount; counter++)
    {
        printXID(counter, &XIDBuffer[counter]);
    }

    TEST_ASSERT(XIDCount==1, ("Number of global transaction not equal to 1 (%d)", XIDCount));

    /*****************************************************************/
    /* And check that the queue depth is non zero                    */
    /*****************************************************************/
    ismEngine_QueueStatistics_t stats;
    ismQHandle_t hQueue = ieqn_getQueueHandle(pThreadData, QUEUE_NAME);

    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

    test_log(4, "Current queue depth is %d", stats.BufferedMsgs);

    TEST_ASSERT(stats.BufferedMsgs == 18, ("The number of messages on the queue not equal to 18", stats.BufferedMsgs));

    /*****************************************************************/
    /* Commit the transaction                                        */
    /*****************************************************************/
    rc=sync_ism_engine_createGlobalTransaction( hSession
                                         , &XIDBuffer[0]
                                         , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                         , &hTran);
    TEST_ASSERT(rc==OK, ("Create global transaction failed rc=%d", rc));

    rc = sync_ism_engine_commitTransaction( hSession
                                     , hTran
                                     , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT(rc==OK, ("Commit global transaction failed rc=%d", rc));

    /*****************************************************************/
    /* Now restart the server                                        */
    /*****************************************************************/
    Test_restart( 2, 4 );

    return;
}

void test2_phase4( void )
{
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    uint32_t XIDCount;
    ism_xid_t *XIDBuffer = NULL;
    uint32_t counter;

    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 2, 4);

    /*****************************************************************/
    /* Initialise the session                                        */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    XIDBuffer = (ism_xid_t *)calloc(sizeof(ism_xid_t), XID_BUFSIZE5);
    TEST_ASSERT(XIDBuffer!=NULL, ("Failed to allocate XID Buffer"));

    /*****************************************************************/
    /* Call recover, we do not expect any in-flight transactions     */
    /*****************************************************************/
    test_log(4, "Calling XARecover");
    XIDCount = ism_engine_XARecover( hSession
                                   , XIDBuffer
                                   , XID_BUFSIZE5
                                   , 1
                                   , ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN );

    for (counter = 0; counter < XIDCount; counter++)
    {
        printXID(counter, &XIDBuffer[counter]);
    }

    TEST_ASSERT(XIDCount==0, ("Number of global transaction not equal to zero (%d)", XIDCount));

    /*****************************************************************/
    /* But verify the contents of the queue                          */
    /*****************************************************************/
    Test_VerifyQueue( hSession, NULL, 18, 12000 );

    if (XIDBuffer != NULL)
    {
        free(XIDBuffer);
    }

    Test_restart( 3, 1 );

    return;
}

/*********************************************************************/
/*********************************************************************/
/* test3                                                             */
/*   - phase1 - Loop 90 times creating an XA transaction containing  */
/*              a single put message. All but the first 5            */
/*              transactions are prepared.                           */
/*   - phase2 - the XARecover call should report that there are 85   */
/*              XIDs in prepared state. The buffer provided is only  */
/*              big enough to contain 20 XID's so repeated call's    */
/*              are required to retrieve all of the XIDs.            */
/*              5 more transactions are created.                     */
/*              The first 10 XIDs are rolled-back, the next 25 are   */
/*              committed.                                           */
/*   - phase3 - the XARecover call should report that there are 55   */
/*              XIDs in prepared state. The buffer provided is only  */
/*              big enough to contain 20 XID's so repeated call's    */
/*              are required to retrieve all of the XIDs.            */
/*              5 more transactions are created.                     */
/*              The first 10 XIDs are rolled-back, the next 25 are   */
/*              committed.                                           */
/*   - phase4 - the XARecover call should report that there are 25   */
/*              XIDs in prepared state. The buffer provided is only  */
/*              big enough to contain 20 XID's so repeated call's    */
/*              are required to retrieve all of the XIDs.            */
/*              5 more transactions are created.                     */
/*              The first 10 XIDs are rolled-back, the final 15 are  */
/*              committed.                                           */
/*   - phase5 - the XARecover call should report that there are no   */
/*              XIDs remaining.                                      */
/*********************************************************************/
/*********************************************************************/
void test3( uint32_t phase )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    uint32_t XIDCount = 0;
    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    ismEngine_TransactionHandle_t hTran;
    ism_xid_t *XIDBuffer = NULL;
    int counter;
    bool remaining = false;

    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 3, phase);

    /*****************************************************************/
    /* Initialise the session                                        */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    if (phase == 1)
    {
        for (counter=0; counter < 90; counter++)
        {
            /*********************************************************/
            /* Create the global transaction to use                  */
            /*********************************************************/
            test_log(5, "Creating transaction");
            memset(&XID, 0, sizeof(ism_xid_t));
            XID.formatID = 0;
            XID.gtrid_length = sizeof(uint64_t);
            XID.bqual_length = sizeof(uint64_t);
            globalId.gtrid = 3; // test
            globalId.bqual = counter;
            memcpy(&XID.data, &globalId, sizeof(globalId));

            rc=sync_ism_engine_createGlobalTransaction( hSession
                                                 , &XID
                                                 , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                                 , &hTran);
            TEST_ASSERT(rc==OK, ("Create global transaction failed rc=%d", rc));

            /*********************************************************/
            /* Put messages on the queue                             */
            /*********************************************************/
            Test_PutMsg( hSession
                       , QUEUE_NAME
                       , hTran
                       , 20      // value
                       , 2       // testcase number 
                       , 1 );    // phase

            if (counter >= 5)
            {
                /*****************************************************/
                /* Prepare the transaction.                          */
                /*****************************************************/
                rc = sync_ism_engine_prepareGlobalTransaction( hSession
                                                             , &XID );
                TEST_ASSERT(rc==OK, ("Prepare transaction failed rc=%d", rc));
                test_log(5, "Prepared the transaction");
            }
        }
        XIDCount = 90; // Ensure we move onto the next phase
        remaining = true;
    }
    else 
    {
        uint32_t commitCount = 0;
        uint32_t xidcounter = 0;
        XIDBuffer = (ism_xid_t *)calloc(sizeof(ism_xid_t), XID_BUFSIZE20);
        TEST_ASSERT(XIDBuffer!=NULL, ("Failed to allocate XID Buffer"));

        /*************************************************************/
        /* Print out the XIDs and commit the first 35.               */
        /*************************************************************/
        uint32_t recoverFlags = ismENGINE_XARECOVER_OPTION_XA_TMSTARTRSCAN;

        do
        {
            test_log(4, "Calling XARecover");
            XIDCount = ism_engine_XARecover( hSession
                                           , XIDBuffer
                                           , XID_BUFSIZE20
                                           , 1
                                           , recoverFlags );
            recoverFlags = ismENGINE_XARECOVER_OPTION_XA_TMNOFLAGS;
    
            test_log(5, "Returned %d of maximum %d XIDs", XIDCount, XID_BUFSIZE20);

            for (counter = 0; counter < XIDCount; counter++)
            {
                printXID(++xidcounter, &XIDBuffer[counter]);

                if (commitCount < 35)
                {
                    if (commitCount < 10)
                    {
                        rc = ism_engine_rollbackGlobalTransaction( hSession
                                                                 , &XIDBuffer[counter]
                                                                 , NULL
                                                                 , 0
                                                                 , NULL );
                        TEST_ASSERT(rc==OK, ("Rollback global transaction (%.16x:%.16x) failed rc=%d", XIDBuffer[counter].data, &(XIDBuffer[counter].data[8]), rc));
                        commitCount++;

                        test_log(5, "Rolled-back global transaction (%.16x:%.16x)",
                                 XIDBuffer[counter].data, &(XIDBuffer[counter].data[8]));
                    }
                    else
                    {
                        rc = sync_ism_engine_commitGlobalTransaction( hSession
                                                               , &XIDBuffer[counter]
                                                               , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
                        TEST_ASSERT(rc==OK, ("Commit global transaction (%.16x:%.16x) failed rc=%d", XIDBuffer[counter].data, &(XIDBuffer[counter].data[8]), rc));
                        commitCount++;

                        test_log(5, "Committed transaction (%.16x:%.16x)",
                                 XIDBuffer[counter].data, &(XIDBuffer[counter].data[8]));
                    }
                }
                else
                {
                    remaining = true;
                }
            }
        }  while (XIDCount == XID_BUFSIZE20);

        /*************************************************************/
        /* Now restart the server                                    */
        /*************************************************************/
        if (remaining)
        {
            for (counter=0; counter < 5; counter++)
            {
                /*********************************************************/
                /* Create the global transaction to use                  */
                /*********************************************************/
                test_log(5, "Creating transaction");
                memset(&XID, 0, sizeof(ism_xid_t));
                XID.formatID = 0;
                XID.gtrid_length = sizeof(uint64_t);
                XID.bqual_length = sizeof(uint64_t);
                globalId.gtrid = 3; // test
                globalId.bqual = counter | (phase << 16);
                memcpy(&XID.data, &globalId, sizeof(globalId));

                rc=sync_ism_engine_createGlobalTransaction( hSession
                                                     , &XID
                                                     , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                                     , &hTran );
                TEST_ASSERT(rc==OK, ("Create global transaction failed rc=%d", rc));

                Test_PutMsg( hSession
                           , QUEUE_NAME
                           , hTran
                           , 20      // value
                           , 2       // testcase number 
                           , 1 );    // phase

                /*****************************************************/
                /* Prepare the transaction.                          */
                /*****************************************************/
                rc = sync_ism_engine_prepareGlobalTransaction( hSession
                                                             , &XID );
                TEST_ASSERT(rc==OK, ("Prepare transaction (%016x:%016x) failed rc=%d", globalId.gtrid, globalId.bqual, rc));
                test_log(5, "Prepared the transaction");
            }
        }
    }

    if (XIDBuffer != NULL)
    {
        free(XIDBuffer);
    }

    // if (XIDCount > 0)
    if (remaining)
    {
        fflush(stdout);
        fflush(stderr);
        Test_restart( 3, phase+1 );
    }

    /*****************************************************************/
    /* Clear the queue of messages                                   */
    /*****************************************************************/
    ismEngine_QueueStatistics_t stats;
    ismQHandle_t hQueue = ieqn_getQueueHandle(pThreadData, QUEUE_NAME);
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

    test_log(4, "Current queue depth is %d", stats.BufferedMsgs);

    TEST_ASSERT(stats.BufferedMsgs == 65, ("The number of messages on the queue (%d) not equal to 65", stats.BufferedMsgs));

    /*****************************************************************/
    /* Consume the message on the queue within the transaction       */
    /*****************************************************************/
    Test_VerifyQueue( hSession
                    , NULL
                    , 65
                    , 1300 ); // Expected value

    Test_restart( 4, 1 );

    return;
}

/*********************************************************************/
/*********************************************************************/
/* test4 - This test is different from tests 1-3 as we do not        */
/*         restart the server for each phase.                        */
/*                                                                   */
/*   - phase1 - Load a queue with 20 messages                        */
/*   - phase2 - Verify the queue depth is 20                         */
/*              Create a global transaction and consume the 20       */
/*              messages.                                            */
/*              Close the consumer, without XA-ending. The messages  */
/*              should be rolled-back.                               */
/*   - phase3 - Verify the queue depth is 20                         */
/*              Create a global transaction and consume the 20       */
/*              messages.                                            */
/*              call XA_end to disassociate the transaction and end  */
/*              the consumer.                                        */
/*   - phase4 - verify the queue depth is 20                         */
/*              Create a consumer and verify that no messages are    */
/*              available.                                           */
/*   - phase5 - disconnect the client then                           */
/*              connect a new client                                 */
/*   - phase6 - Verify the queue depth is 20                         */
/*              Create a global transaction and consume the 20       */
/*              messages.                                            */
/*              call XA_end to disassociate the transaction and end  */
/*              the consumer.                                        */
/*              commit the global transaction.                       */
/*   - phase7 - verify the queue depth is 0                          */
/*              Create a consumer and verify that no messages are    */
/*              available.                                           */
/*                                                                   */
/*********************************************************************/
/*********************************************************************/
void test4_phase1( void )
{
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;

    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 4, 1);

    /*****************************************************************/
    /* Initialise the session                                        */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    /*****************************************************************/
    /* Put 20 messages on the queue                                  */
    /*****************************************************************/
    Test_PutMsgParts( hSession
                    , QUEUE_NAME
                    , NULL
                    , 20     // 20 messages
                    , 12000  // Total value
                    , 4      // testcase number 
                    , 1 );   // phase

    test_log(4, "Put 20 messages");

    return;
}

void test4_phase2( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    ismEngine_TransactionHandle_t hTran;

    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 4, 2);


    /*****************************************************************/
    /* Verify that there are 20 messages available on the queue      */
    /*****************************************************************/
    ismEngine_QueueStatistics_t stats;
    ismQHandle_t hQueue = ieqn_getQueueHandle(pThreadData, QUEUE_NAME);

    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

    test_log(4, "Current queue depth is %d", stats.BufferedMsgs);

    TEST_ASSERT(stats.BufferedMsgs == 20, ("The number of messages on the queue not equal to 20", stats.BufferedMsgs));

    /*****************************************************************/
    /* Create a session                                              */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_TRANSACTIONAL
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    /*****************************************************************/
    /* Create the global transaction to use                          */
    /*****************************************************************/
    test_log(5, "Creating transaction");
    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 4; // test
    globalId.bqual = 2;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc=sync_ism_engine_createGlobalTransaction( hSession
                                         , &XID
                                         , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                         , &hTran );
    TEST_ASSERT(rc==OK, ("Create global transaction failed rc=%d", rc));

    /*****************************************************************/
    /* Consume the message on the queue within the transaction       */
    /*****************************************************************/
    Test_VerifyQueue( hSession
                    , hTran
                    , 20 
                    , 12000 ); // Expected value

    test_log(4, "Destroying session with consumed messages in unprepared transaction.");

    /*****************************************************************/
    /* Destroy the session                                           */
    /*****************************************************************/
    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT(rc==OK, ("Destroy session failed rc=%d", rc));

    return;
}

void test4_phase3( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    ismEngine_TransactionHandle_t hTran;

    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 4, 3);

    /*****************************************************************/
    /* Initialise the session                                        */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_TRANSACTIONAL
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    /*****************************************************************/
    /* Verify that there are 20 messages available on the queue      */
    /*****************************************************************/
    ismEngine_QueueStatistics_t stats;
    ismQHandle_t hQueue = ieqn_getQueueHandle(pThreadData, QUEUE_NAME);

    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

    test_log(4, "Current queue depth is %d", stats.BufferedMsgs);

    TEST_ASSERT(stats.BufferedMsgs == 20, ("The number of messages on the queue not equal to 20", stats.BufferedMsgs));

    /*****************************************************************/
    /* Create the global transaction to use                          */
    /*****************************************************************/
    test_log(5, "Creating transaction");
    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 4; // test
    globalId.bqual = 3;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc=sync_ism_engine_createGlobalTransaction( hSession
                                         , &XID
                                         , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                         , &hTran );
    TEST_ASSERT(rc==OK, ("Create global transaction failed rc=%d", rc));

    /*****************************************************************/
    /* Consume the message on the queue within the transaction       */
    /*****************************************************************/
    Test_VerifyQueue( hSession
                    , hTran
                    , 20 
                    , 12000 ); // Expected value

    test_log(4, "Disassociating session from transaction");

    /*****************************************************************/
    /* End the association between the session and the transaction   */
    /*****************************************************************/
    rc = ism_engine_endTransaction( hSession
                                  , hTran
                                  , ismENGINE_END_TRANSACTION_OPTION_DEFAULT
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT(rc==OK, ("End global transaction failed rc=%d", rc));

    test_log(4, "Destroying session with consumed messages in unprepared transaction.");

    /*****************************************************************/
    /* Destroy the session                                           */
    /*****************************************************************/
    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT(rc==OK, ("Destroy session failed rc=%d", rc));

    return;
}

void test4_phase4( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;

    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 4, 4);

    /*****************************************************************/
    /* Initialise the session                                        */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    /*****************************************************************/
    /* Verify that there are 20 messages available on the queue      */
    /*****************************************************************/
    ismEngine_QueueStatistics_t stats;
    ismQHandle_t hQueue = ieqn_getQueueHandle(pThreadData, QUEUE_NAME);

    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

    test_log(4, "Current queue depth is %d", stats.BufferedMsgs);

    TEST_ASSERT(stats.BufferedMsgs == 20, ("The number of messages on the queue not equal to 20", stats.BufferedMsgs));

    /*****************************************************************/
    /* Verify that there are no messages available on the queue      */
    /*****************************************************************/
    Test_VerifyQueue( hSession
                    , NULL
                    , 0 
                    , 0 ); 

    /*****************************************************************/
    /* Destroy the session                                           */
    /*****************************************************************/
    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT(rc==OK, ("Destroy session failed rc=%d", rc));

    return;
}

void test4_phase5( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;
    ismEngine_TransactionHandle_t hTran;

    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 4, 5);

    /*****************************************************************/
    /* Verify that there are 20 messages available on the queue      */
    /*****************************************************************/
    ismEngine_QueueStatistics_t stats;
    ismQHandle_t hQueue = ieqn_getQueueHandle(pThreadData, QUEUE_NAME);

    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

    test_log(4, "Current queue depth is %d", stats.BufferedMsgs);

    /*****************************************************************/
    /* Destroy and then recreate the client, this should cause the   */
    /* unprepared transaction to be rolled back.                     */
    /*****************************************************************/
    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT(rc == OK, ("Destroy client state failed"));

    rc = ism_engine_createClientState("Test GlobalTran",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT(rc == OK, ("Failed to create client state. rc = %d", rc));

    test_log(5, "Client state is destroyed.");

    /*****************************************************************/
    /* Initialise the session                                        */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_TRANSACTIONAL
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    test_log(5, "Client state is recreated.");

    /*****************************************************************/
    /* Create the global transaction to use                          */
    /*****************************************************************/
    test_log(5, "Creating transaction");
    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 4; // test
    globalId.bqual = 5;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc=sync_ism_engine_createGlobalTransaction( hSession
                                         , &XID
                                         , ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT
                                         , &hTran );
    TEST_ASSERT(rc==OK, ("Create global transaction failed rc=%d", rc));

    /*****************************************************************/
    /* Consume the message on the queue within the transaction       */
    /*****************************************************************/
    Test_VerifyQueue( hSession
                    , hTran
                    , 20 
                    , 12000 ); // Expected value

    /*****************************************************************/
    /* Destroy the session                                           */
    /*****************************************************************/
    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT(rc==OK, ("Destroy session failed rc=%d", rc));


    return;
}

/*********************************************************************/
/*********************************************************************/
/* test5                                                             */
/*   - phase1 - Publish a retained message on 998 topics.            */
/*   - phase2 - Restart and verify subscription is correctly backed  */
/*              out.                                                 */
/* Note 1: The value of the topicCount varaible (998) is matched to  */
/*         the guaranteed store transaction size of 1000.            */
/* Note 2: This testcase has been automated in order that it is easy */
/*         to rerun, however the phases need to be run individually  */
/*         and trace examined to be sure everything is working       */
/*         correctly.                                                */
/*********************************************************************/
/*********************************************************************/
uint32_t iett_SLEReplayOldStoreSubscDefn( ietrReplayPhase_t        Phase,
                                          ieutThreadData_t        *pThreadData,
                                          ismEngine_Transaction_t *pTran,
                                          void                    *entry,
                                          ietrReplayRecord_t      *pRecord)
{
    static uint32_t (*real_SLEReplayOldStoreSubscDefn)(ietrReplayPhase_t, ieutThreadData_t *, ismEngine_Transaction_t *, void *, ietrReplayRecord_t *) = NULL;
    int32_t rc=OK;

    if (!real_SLEReplayOldStoreSubscDefn)
    {
        //Find the real version of ietr_commit
        real_SLEReplayOldStoreSubscDefn = dlsym(RTLD_NEXT, "iett_SLEReplayOldStoreSubscDefn");
        TEST_ASSERT(real_SLEReplayOldStoreSubscDefn != NULL,
                    ("real_SLEReplayStoreSubscDefn (%p) == NULL", real_SLEReplayOldStoreSubscDefn));
    }

    if (TestcaseNumber == 5)
    {
        test_log(testLOGLEVEL_TESTPROGRESS, "Restart during CreateSubscription ROLLBACK\n");
        Test_restart( 5, 2 );
    }
    else
    {
        rc = real_SLEReplayOldStoreSubscDefn( Phase
                                         , pThreadData
                                         , pTran
                                         , entry
                                         , pRecord );
    }

    return rc;
}

void test5_countPut(int32_t retcode, void *handle, void *pContext)
{
    uint32_t *pPutCount = *(uint32_t **)pContext;

    TEST_ASSERT(retcode == OK || retcode == ISMRC_NoMatchingDestinations,
                ("async put callback retcode=%d\n", retcode));

    __sync_fetch_and_sub(pPutCount, 1);
}

void test5_phase1( void )
{
#define TOPIC_PREFIX "/TranRestart/Retained1/"
    uint32_t rc;
    ismEngine_SessionHandle_t hSession;
    uint64_t loop = 0;
    char topicString[128];
    uint64_t topicCount = 998;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    char subName[]="Tran2_Sub_Retained1";

    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 5, 1);

    // Set the maximum queue depth to less than the number of retained messages we'll publish
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = topicCount-1;

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("ism_engine_createSession failed with rc = %d", rc));

    test_log(testLOGLEVEL_CHECK, "Publish %d retained messages under topic %s",
             topicCount, TOPIC_PREFIX);

    // Publish the retained messages
    uint32_t putCount = topicCount;
    uint32_t *pPutCount = &putCount;
    for (loop=0; loop < topicCount; loop++)
    {
        void *payloadPtr = &loop;
        sprintf(topicString, TOPIC_PREFIX "%ld", loop+1);
        rc = test_createMessage(sizeof(uint64_t),
                                ismMESSAGE_PERSISTENCE_PERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, topicString,
                                &hMessage, &payloadPtr);
        TEST_ASSERT(rc == OK, ("creating message failed with rc = %d", rc));

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                NULL,
                                                hMessage,
                                                &pPutCount, sizeof(pPutCount), test5_countPut);
        if (rc != ISMRC_AsyncCompletion)
        {
            TEST_ASSERT(rc < ISMRC_Error, ("ism_engine_putMessageOnDestination failed with rc = %d", rc));
            __sync_fetch_and_sub(pPutCount, 1);
        }
    }

    while(putCount > 0)
    {
        sched_yield();
    }

    // And create a subscription
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
    rc=sync_ism_engine_createSubscription( hClient
                                         , subName
                                         , NULL
                                         , ismDESTINATION_TOPIC
                                         , TOPIC_PREFIX "#"
                                         , &subAttrs
                                         , NULL ); // Owning client same as requesting client

    TEST_ASSERT(rc == ISMRC_DestinationFull, ("ism_engine_putMessageOnDestination should fail with ISMRC_DestinationFull, but got rc = %d", rc));

    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT(rc==OK, ("Destroy session failed rc=%d", rc));

    return;
}

void test5_phase2( void )
{
    struct tm tm = { 0 };
    time_t curTime = time(NULL);
    (void)localtime_r(&curTime, &tm);
    test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Test %d Phase %d",
             tm.tm_hour, tm.tm_min, tm.tm_sec, 5, 2);


    test_log(testLOGLEVEL_TESTPROGRESS, "Succesfully restarted\n");
}


int32_t parseArgs( int argc
                 , char *argv[]
                 , uint32_t *ptestcase
                 , uint32_t *pphase
                 , bool *psingle
                 , bool *pcoldStart
                 , testLogLevel_t *pverboseLevel
                 , int *ptrclvl
                 , char **padminDir )
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "testcase", 1, NULL, 't' },
        { "phase", 1, NULL, 'p' },
        { "single", 0, NULL, 's' },
        { "cold", 0, NULL, 'c' },
        { "verbose", 1, NULL, 'v' },
        { "adminDir", 1, NULL, 'a' },
        { NULL, 1, NULL, 0 } };
    int long_index;

    *ptestcase = 1;
    *pphase = 1;
    *pverboseLevel = 3;
    *ptrclvl = 4;
    *psingle = false;
    *pcoldStart = false;
    *padminDir = NULL;

    while ((opt = getopt_long(argc, argv, ":t:p:scv:a:0123456789", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                *ptrclvl = opt - '0';
                break;
            case 't':
               *ptestcase = (uint32_t)atoi(optarg);
               break;
            case 'p':
               *pphase = (uint32_t)atoi(optarg);
               break;
            case 's':
               *psingle= true;
               break;
            case 'c':
               *pcoldStart= true;
               break;
            case 'v':
               *pverboseLevel = (testLogLevel_t)atoi(optarg);
               break;
            case 'a':
                *padminDir = optarg;
                break;
            case '?':
            default:
               usage=1;
               break;
        }
    }
    
    if (*pverboseLevel > testLOGLEVEL_VERBOSE) /* BEAM suppression: constant condition */
        *pverboseLevel=testLOGLEVEL_VERBOSE;

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-v 0-9] [-0...-9] [-p phase] [-f final-phase] [-c count] [-r ratio]\n", argv[0]);
        fprintf(stderr, "       -t (--testcase)      - Set initial testcase\n");
        fprintf(stderr, "       -p (--phase)         - Set initial phase\n");
        fprintf(stderr, "       -s (--single)        - Run only single testcase/phase\n");
        fprintf(stderr, "       -v (--verbose)       - Test Case verbosity\n");
        fprintf(stderr, "       -a (--adminDir)      - Admin directory to use\n");
        fprintf(stderr, "       -0 .. -9             - ISM Trace level\n");
    }

   return usage;
}


/*********************************************************************/
/* NAME:        test_tranRestart                                     */
/* DESCRIPTION: This program tests the recovery of transactions      */
/*********************************************************************/
int main(int argc, char *argv[])
{
    int rc = 0;
    uint32_t testcase;
    uint32_t phase;
    time_t curTime;
    bool coldStart = false;
    struct tm tm = { 0 };
    ismEngine_SessionHandle_t hSession;
    char *adminDir = NULL;
    
    /*****************************************************************/
    /* Parse arguments                                               */
    /*****************************************************************/
    programName = argv[0];
    rc = parseArgs( argc
                  , argv
                  , &testcase
                  , &phase
                  , &single
                  , &coldStart
                  , &testLogLevel
                  , &trclvl
                  , &adminDir );
    if (rc != 0)
    {
        goto mod_exit;
    }


    /*****************************************************************/
    /* Process Initialise                                            */
    /*****************************************************************/
    test_setLogLevel(testLogLevel);

    rc = test_processInit(trclvl, adminDir);
    if (rc != OK)
    {
        goto mod_exit;
    }

    /*****************************************************************/
    /* Seed the random number generator and prepare the time for     */
    /* printing                                                      */
    /*****************************************************************/
    curTime = time(NULL);

    (void)localtime_r(&curTime, &tm);
    
    /*****************************************************************/
    /* If this is the first run for this test then cold start the    */
    /* store.                                                        */
    /*****************************************************************/
    if (((testcase == 1) && (phase == 1)) || coldStart)
    {
        test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Running test phase 0 cold-start",
                 tm.tm_hour, tm.tm_min, tm.tm_sec);

        /*************************************************************/
        /* Start up the Engine - Cold Start                          */
        /*************************************************************/
        rc = test_engineInit_COLD;
        if (rc != 0)
        {
            goto mod_exit;
        }
        test_log(testLOGLEVEL_TESTPROGRESS, "Engine cold started");

        /*************************************************************/
        /* Create the queue we will be using.                        */
        /*************************************************************/
        Test_CreateQ( multiConsumer, QUEUE_NAME);
    }
    else
    {
        /*************************************************************/
        /* Start up the Engine - Warm Start and auto queue creation  */
        /* enabled to avoid queue deletion while reconciling admin.  */
        /*************************************************************/
        rc = test_engineInit_WARMAUTO;
        if (rc != 0)
        {
            goto mod_exit;
        }
        test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");
    }

    rc = ism_engine_createClientState("Test GlobalTran",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT(rc == OK, ("Failed to create client state. rc = %d", rc));

    /*****************************************************************/
    /* And initialise the session                                    */
    /*****************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    switch(testcase)
    {
        case 1:
            TestcaseNumber = 1;
            switch(phase)
            {
                case 1: test1_phase1( ); break;
                case 2: test1_phase2( ); break;
                default: TEST_ASSERT(false, ("Invalid test phase.")); break;
            }
            break;
        case 2:
            TestcaseNumber = 2;
            switch(phase)
            {
                case 1: test2_phase1( ); break;
                case 2: test2_phase2( ); break;
                case 3: test2_phase3( ); break;
                case 4: test2_phase4( ); break;
                default: TEST_ASSERT(false, ("Invalid test phase.")); break;
            }
            break;
        case 3:
            TestcaseNumber = 3;
            test3(phase);
            break;
        case 4:
            TestcaseNumber = 4;
            switch (phase)
            {
                case 1: test4_phase1( ); if (single) break; /* BEAM suppression: fall through */ /* no break */
                case 2: test4_phase2( ); if (single) break; /* BEAM suppression: fall through */ /* no break */
                case 3: test4_phase3( ); if (single) break; /* BEAM suppression: fall through */ /* no break */
                case 4: test4_phase4( ); if (single) break; /* BEAM suppression: fall through */ /* no break */
                case 5: test4_phase5( ); if (single) break; /* BEAM suppression: fall through */ /* no break */
                        Test_restart( 5, 1 );
                        break;
            }
            break;
        case 5:
            TestcaseNumber = 5;
            switch (phase)
            {
                case 1: test5_phase1( ); if (single) break; /* BEAM suppression: fall through */ /* no break */
                case 2: test5_phase2( ); if (single) break; /* BEAM suppression: fall through */ /* no break */
            }
            break;
        default: TEST_ASSERT(false, ("Invalid test case.")); break;
    }

    test_engineTerm(true);
    test_processTerm(false);
    test_removeAdminDir(adminDir);

    test_log(2, "Success. Test complete!\n");

mod_exit:
    return (0);
}
