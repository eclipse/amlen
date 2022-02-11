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
/* Module Name: test_queue4.c                                        */
/*                                                                   */
/* Description: Test program which validates the recovery of queues  */
/*              and messages from the store.                         */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <assert.h>
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
#include "test_utils_sync.h"
#include "test_utils_task.h"

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
static ismEngine_ClientStateHandle_t hClient = NULL;
static bool EndNow = false;

#define BATCH_SIZE 5

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
    uint32_t ConsumerNo;
    uint32_t Phase;
    uint64_t Value;
} Payload_t;

void putMessageCompleted(int rc, void *handle, void *context)
{
	uint64_t *pPutsCompleted = *(uint64_t **)context;
	__sync_add_and_fetch(pPutsCompleted, 1);
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
/*     ConsumerNo          - Consumer who put the message            */
/*     Phase               - Phase in which the message was put      */
/*********************************************************************/
static void Test_PutMsgParts( ismEngine_SessionHandle_t hSession
                            , uint32_t ThreadId
                            , char *QueueName
                            , ismEngine_TransactionHandle_t hTran
                            , uint32_t MsgCount
                            , uint64_t Total
                            , uint32_t Phase )
{ 
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismMessageHeader_t MsgHeader = { 0
                                   , ismMESSAGE_PERSISTENCE_PERSISTENT
                                   , ismMESSAGE_RELIABILITY_JMS_PERSISTENT
                                   , ismMESSAGE_PRIORITY_JMS_DEFAULT
                                   , 0 // RedeliveryCount
                                   , 0 // Expiry
                                   , ismMESSAGE_FLAGS_NONE
                                   , MTYPE_JMS };
    uint64_t Remaining = Total;
    ismEngine_MessageHandle_t hMessage;
    ismMessageAreaType_t AreaTypes[] = { ismMESSAGE_AREA_PAYLOAD };
    size_t AreaLengths[] = { sizeof(Payload_t) };
    Payload_t Payload;
    void *Areas[] = { &Payload };
    uint64_t putsCompleted = 0;
    uint64_t *pPutsCompleted = &putsCompleted;

    Payload.Phase = Phase;
    Payload.ConsumerNo = ThreadId;
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
        rc = ism_engine_createMessage( &MsgHeader
                                     , 1
                                     , AreaTypes
                                     , AreaLengths
                                     , Areas
                                     , &hMessage );
        TEST_ASSERT(rc == OK , ("ism_engine_createMessage failed. rc = %d"));

        test_log(testLOGLEVEL_VERBOSE,
                 "Putting message %d with value %ld", counter, Payload.Value);

        // And put the message
        rc = ism_engine_putMessageOnDestination( hSession
                                               , ismDESTINATION_QUEUE
                                               , QueueName
                                               , hTran
                                               , hMessage
                                               , &pPutsCompleted
                                               , sizeof(pPutsCompleted)
                                               , putMessageCompleted );

        if (rc != OK && rc != ISMRC_AsyncCompletion)
        {
            printf("DEBUG: View of queue after failed put\n");
            test_log_queueHandle(testLOGLEVEL_VERBOSE, ieqn_getQueueHandle(pThreadData, QueueName), 9, -1, "");
        }
        TEST_ASSERT( rc == OK || rc == ISMRC_AsyncCompletion, ("ism_engine_putMessageOnDestination failed. rc = %d", rc));

        if (rc == OK)
        {
        	putMessageCompleted(rc, NULL, &pPutsCompleted);
        }
    }

    while ( putsCompleted < MsgCount)
    {
    	usleep(50);
    }

    return;
}

typedef struct VerifyContext_t
{
    volatile uint64_t Total;
    volatile uint32_t MsgCount;
} VerifyContext_t;

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
    VerifyContext_t *pContext = *(VerifyContext_t **)pConsumerContext;

    TEST_ASSERT( areaCount == 1, 
                 ("Received %d areas. Expected 1!", areaCount) );
    TEST_ASSERT(areaLengths[0] == sizeof(Payload_t),
                ("Unsupported message length of %d", areaLengths[1]));

    Payload_t *pPayload=(Payload_t *)pAreaData[0];
    pContext->MsgCount++;
    pContext->Total += pPayload->Value;

    test_log(testLOGLEVEL_VERBOSE,
             "Received message. orderId(%lu) Phase(%u) Consumer(%u) Value(%lu) total-so-far %lu",
             pMsgDetails->OrderId, pPayload->Phase, pPayload->ConsumerNo, 
             pPayload->Value, pContext->Total);

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
/*     InitialValue        - Initial message value                   */
/*********************************************************************/
static void Test_VerifyQueue( char *QueueName
                            , uint64_t InitialValue )
{
    uint32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    VerifyContext_t VerifyContext = { 0 };
    VerifyContext_t *pVerifyContext = &VerifyContext;

    /*************************************************************/
    /* Create a session.                                         */
    /*************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    /*************************************************************/
    /* And initialise the consumer                               */
    /*************************************************************/
    VerifyContext.Total = 0;

    // printf("DEBUG: View of queue before verify\n");
    // test_log_queue(testLOGLEVEL_VERBOSE, QueueName, 9, -1, "");

    // initialise a consumer
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , QueueName
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

    test_waitForMessages64(&(VerifyContext.Total), NULL, InitialValue, 20);

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
    TEST_ASSERT(VerifyContext.Total == InitialValue,
               ("Total message value %ld for %d messages doesn't match initial value %ld",
                VerifyContext.Total, VerifyContext.MsgCount, InitialValue));

    test_log(testLOGLEVEL_CHECK, "Value of messages following restart verified as %ld", VerifyContext.Total);
 
    // And finally destroy the session, this will cause the messages we got
    // to be backed out.
    rc = sync_ism_engine_destroySession( hSession);
    TEST_ASSERT(rc==OK, ("Destroy session failed rc=%d", rc));

    return;
}

/*********************************************************************/
/* Name:        Worker Callback                                      */
/* Description: Callback function wheich does the main work in the   */
/*              test.                                                */
/*********************************************************************/

typedef struct WorkerContext_t
{
    pthread_t hThread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    uint32_t id;
    uint32_t Phase;
    char *QueueName;
    ismEngine_SessionHandle_t hSession;
    ismEngine_TransactionHandle_t hTran;
    uint32_t RollbackRatio;
    uint32_t BatchSize;
    uint32_t MsgCount;
    uint64_t Total;
    uint64_t acksCompleted;
    bool     transactionFinished;
} WorkerContext_t;



static void WorkerAckCompleted(int32_t rc, void *handle, void *context)

{
    WorkerContext_t *pWorkerContext = *(WorkerContext_t **)context;

    int osrc = pthread_mutex_lock(&pWorkerContext->mutex);
    TEST_ASSERT(osrc==0, ("Failed to lock worker mutex for thread %d. rc = %d",
                pWorkerContext->id, osrc));

    uint64_t batchAcksCompleted = __sync_add_and_fetch(&(pWorkerContext->acksCompleted), 1);

    if (batchAcksCompleted == pWorkerContext->BatchSize)
    {
        // Currently we put and get messages in fixed size batches as we have
        // no way of being told when the the queue is empty
        osrc = pthread_cond_signal(&pWorkerContext->cond);
        TEST_ASSERT(osrc==0, ("Failed to signal worker cond for thread %d. rc = %d",
        		pWorkerContext->id, osrc));
    }

    osrc = pthread_mutex_unlock(&pWorkerContext->mutex);
    TEST_ASSERT(osrc==0, ("Failed to unlock worker mutex for thread %d. rc = %d",
                pWorkerContext->id, osrc));
}

static bool WorkerCallback(ismEngine_ConsumerHandle_t  hConsumer,
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
    uint32_t rc=0;
    int osrc;
    WorkerContext_t *pWorkerContext = *(WorkerContext_t **)pConsumerContext;
    bool wantsMoreMessages = true;

    TEST_ASSERT( areaCount == 1, 
                 ("Received %d areas. Expected 1!", areaCount) );
    TEST_ASSERT(areaLengths[0] == sizeof(Payload_t),
                ("Unsupported message length of %d", areaLengths[1]));

    Payload_t *pPayload=(Payload_t *)pAreaData[0];

    // Lock the mutex for the consumer before updating the consumer context

    osrc = pthread_mutex_lock(&pWorkerContext->mutex);
    TEST_ASSERT(osrc==0, ("Failed to lock worker mutex for thread %d. rc = %d",
                pWorkerContext->id, osrc));

    // If this is the first message then create a transaction
    if (pWorkerContext->MsgCount == 0)
    {
    	ieutThreadData_t *pThreadData = ieut_getThreadData();
    	rc = ietr_createLocal(pThreadData, pWorkerContext->hSession, true, false, NULL, &pWorkerContext->hTran);

        TEST_ASSERT( rc == OK, 
                     ("createLocalTransaction failed. rc=%d", rc));

        pWorkerContext->transactionFinished = false;

        test_log(testLOGLEVEL_VERBOSE, "Consumer(%d) created transaction.", pWorkerContext->id);
    }

    test_log(testLOGLEVEL_VERBOSE,
             "Consumer(%u) received message. orderId(%lu) Phase(%u) Consumer(%u) Value(%lu) total-so-far %lu",
             pWorkerContext->id, pMsgDetails->OrderId, pPayload->Phase,
             pPayload->ConsumerNo, pPayload->Value, pWorkerContext->Total + pPayload->Value);

    pWorkerContext->Total += pPayload->Value;
    pWorkerContext->MsgCount++;

    if (pWorkerContext->MsgCount == pWorkerContext->BatchSize)
    {
        // stop delivery of messages for the consumer until acks arrive
        rc = ism_engine_stopMessageDelivery( pWorkerContext->hSession
                                           , NULL
                                           , 0
                                           , NULL );
        TEST_ASSERT( rc == OK,
                     ("stopMessageDelivery failed. rc=%d", rc));
        wantsMoreMessages = false;
    }
    TEST_ASSERT((pWorkerContext->MsgCount <= pWorkerContext->BatchSize),
                   ("msgcount was %u, batch size should be: %u\n",
                   pWorkerContext->MsgCount, pWorkerContext->BatchSize));

    // Acknowledge the message and put it in the transaction.
     rc = ism_engine_confirmMessageDelivery( pWorkerContext->hSession
                                           , pWorkerContext->hTran
                                           , hDelivery
                                           , ismENGINE_CONFIRM_OPTION_CONSUMED
                                           , &pWorkerContext, sizeof(pWorkerContext)
                                           , WorkerAckCompleted);
     TEST_ASSERT( rc == OK || rc == ISMRC_AsyncCompletion,
                  ("confirmMessageDelivery failed. rc=%d", rc));


    osrc = pthread_mutex_unlock(&pWorkerContext->mutex);
    TEST_ASSERT(osrc==0, ("Failed to unlock worker mutex for thread %d. rc = %d",
                pWorkerContext->id, osrc));

    if (rc == OK)
    {
        WorkerAckCompleted(rc, NULL, &pWorkerContext);
    }


    ism_engine_releaseMessage(hMessage);

    return wantsMoreMessages;
}
  
/*********************************************************************/
/* Name:        defineConsumer                                       */
/* Description: This function will setup a consumer which will get   */
/*              1 or messages and put 1 or more messages all within  */
/*              the same transaction.                                */
/* Parameters:                                                       */
/*     QueueName           - Name to give queue                      */
/*     InitialValue        - Initial message value                   */
/*********************************************************************/
static void defineConsumer( WorkerContext_t *pWorkerContext )
{
    uint32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    /*************************************************************/
    /* Initialise a session for this consumer                    */
    /*************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_TRANSACTIONAL
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    /*************************************************************/
    /* And initialise the consumer                               */
    /*************************************************************/
    pWorkerContext->hSession = hSession; 

    test_log(testLOGLEVEL_TESTPROGRESS, "Creating consumer(%d)", pWorkerContext->id);

    // initialise a consumer
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , pWorkerContext->QueueName
                                  , &subAttrs
                                  , NULL // Unused for QUEUE
                                  , &pWorkerContext
                                  , sizeof(pWorkerContext)
                                  , WorkerCallback
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

    /*************************************************************/
    /* And we are done                                           */
    /*************************************************************/
    return;
}

void workerTransactionCompleted(int rc, void *handle, void *context)
{
	WorkerContext_t *pWorkerContext = *(WorkerContext_t **)context;

	int osrc = pthread_mutex_lock(&pWorkerContext->mutex);
    TEST_ASSERT(osrc==0, ("Failed to lock worker mutex for thread %d. rc = %d",
                pWorkerContext->id, osrc));

    pWorkerContext->transactionFinished = true;

    osrc = pthread_cond_signal(&pWorkerContext->cond);
    TEST_ASSERT(osrc==0, ("Failed to signal worker cond for thread %d. rc = %d",
    		pWorkerContext->id, osrc));

    osrc = pthread_mutex_unlock(&pWorkerContext->mutex);
    TEST_ASSERT(osrc==0, ("Failed to unlock worker mutex for thread %d. rc = %d",
    		pWorkerContext->id, osrc));
}

/*********************************************************************/
/* Name:        workerThread                                         */
/* Description: This function waits to be posted that the set number */
/*              of messages have been received and should commit.    */
/*********************************************************************/
void *workerThread(void *arg)
{
    uint32_t rc;
    WorkerContext_t *pWorkerContext = (WorkerContext_t *)arg;
    char ThreadName[16];
    int osrc;

    sprintf(ThreadName, "Worker(%d)", pWorkerContext->id);
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&ThreadName);

    ism_engine_threadInit(0);

    osrc = pthread_mutex_lock(&pWorkerContext->mutex);
    TEST_ASSERT(osrc==0, ("Failed to lock worker mutex for thread %d. rc = %d",
                pWorkerContext->id, osrc));

    while (!EndNow) /* BEAM suppression: infinite loop */
    {
        // We have the mutex locked, check to see if the batch is ready to
        // be committed
        while (!EndNow && (pWorkerContext->acksCompleted < pWorkerContext->BatchSize))
        {
            osrc = pthread_cond_wait( &pWorkerContext->cond
                                    , &pWorkerContext->mutex );
            TEST_ASSERT(osrc==0,
                        ("Failed to wait on condition for thread %d. rc = %d",
                        pWorkerContext->id, osrc));
        }
        if (EndNow)break;

        TEST_ASSERT(pWorkerContext->acksCompleted == pWorkerContext->BatchSize,
        		    ("We continued to get messages after the end of the batch"));
        pWorkerContext->acksCompleted  = 0;

        // We have been posted, time to put the messages
        Test_PutMsgParts( pWorkerContext->hSession
                        , pWorkerContext->id
                        , pWorkerContext->QueueName
                        , pWorkerContext->hTran
                        , pWorkerContext->BatchSize
                        , pWorkerContext->Total
                        , pWorkerContext->Phase );

        // If I have put the sum of the messages received, then now I must
        // commit or rollback. (For now I only commit)

        if ((pWorkerContext->RollbackRatio) &&
            ((rand() % pWorkerContext->RollbackRatio) == 0))
        {
            test_log(testLOGLEVEL_VERBOSE, "Consumer(%d) rolling back transaction", pWorkerContext->id);

            rc = ism_engine_rollbackTransaction( pWorkerContext->hSession
                                               , pWorkerContext->hTran
                                               , NULL
                                               , 0
                                               , NULL);
            TEST_ASSERT( rc == OK, 
                         ("rollbackTransaction failed. rc=%d", rc));

            pWorkerContext->transactionFinished = true;
        }
        else
        {
            test_log(testLOGLEVEL_VERBOSE, "Consumer(%d) committing transaction", pWorkerContext->id);

            rc = ism_engine_commitTransaction( pWorkerContext->hSession
                                             , pWorkerContext->hTran
                                             , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT
                                             , &pWorkerContext
                                             , sizeof(pWorkerContext)
                                             , workerTransactionCompleted);

            TEST_ASSERT( rc == OK || rc == ISMRC_AsyncCompletion,
                         ("commitTransaction failed. rc=%d", rc));

            if (rc == OK)
            {
            	osrc = pthread_mutex_unlock(&pWorkerContext->mutex);
                TEST_ASSERT(osrc==0, ("Failed to unlock worker mutex for thread %d. rc = %d",
                            pWorkerContext->id, osrc));

            	workerTransactionCompleted(rc, NULL, &pWorkerContext);

            	osrc = pthread_mutex_lock(&pWorkerContext->mutex);
                TEST_ASSERT(osrc==0, ("Failed to lock worker mutex for thread %d. rc = %d",
                            pWorkerContext->id, osrc));
            }
        }

        while (pWorkerContext->transactionFinished == false)
		{
			osrc = pthread_cond_wait( &pWorkerContext->cond
					                , &pWorkerContext->mutex );
			TEST_ASSERT(osrc==0,
					("Failed to wait on condition for thread %d. rc = %d",
							pWorkerContext->id, osrc));
		}

        // printf("DEBUG: View of queue following commit\n");
        // test_log_queue(testLOGLEVEL_VERBOSE, pWorkerContext->QueueName, 9, -1, "");

        pWorkerContext->MsgCount = 0;
        pWorkerContext->Total = 0;

        osrc = pthread_mutex_unlock(&pWorkerContext->mutex);
        TEST_ASSERT(osrc==0, ("Failed to unlock worker mutex for thread %d. rc = %d",
                    pWorkerContext->id, osrc));

        // And resume delivery of messages for the consumer
        do {
            rc = ism_engine_startMessageDelivery( pWorkerContext->hSession
                                                  , 0 /* options */
                                                  , NULL
                                                  , 0
                                                  , NULL );
        } while (rc == ISMRC_RequestInProgress);

        TEST_ASSERT( rc == OK, 
                     ("ism_engine_startMessageDelivery failed. rc=%d", rc));

        osrc = pthread_mutex_lock(&pWorkerContext->mutex);
        TEST_ASSERT(osrc==0, ("Failed to lock worker mutex for thread %d. rc = %d",
                    pWorkerContext->id, osrc));

    }

    TEST_ASSERT(EndNow, ("EndNow"));

    osrc = pthread_mutex_unlock(&pWorkerContext->mutex);
    TEST_ASSERT(osrc==0, ("Failed to unlock worker mutex for thread %d. rc = %d",
                pWorkerContext->id, osrc));

    return NULL;
}

int32_t parseArgs( int argc
                 , char *argv[]
                 , uint32_t *pphase
                 , uint32_t *pfinalphase
                 , uint32_t *pconsumerCount
                 , uint32_t *prollbackRatio
                 , uint32_t *pbatchSize
                 , testLogLevel_t *pverboseLevel
                 , int *ptrclvl
                 , char **padminDir)
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "phase", 1, NULL, 'p' },
        { "final", 1, NULL, 'f' },
        { "verbose", 1, NULL, 'v' },
        { "consumerCount", 1, NULL, 'c' },
        { "rollbackRatio", 1, NULL, 'r' },
        { "batchSize", 1, NULL, 'b' },
        { "adminDir", 1, NULL, 'a' },
        { NULL, 1, NULL, 0 } };
    int long_index;

    *pphase = 0;
    *pfinalphase = 8;
    *pverboseLevel = testLOGLEVEL_CHECK;
    *pconsumerCount = 4;
    *prollbackRatio = 8; // Rollback 1 in 8 transactions
    *pbatchSize = 5; // 5 gets + 5 puts in a transaction
    *ptrclvl = 0;
    *padminDir = NULL;

    while ((opt = getopt_long(argc, argv, ":p:f:c:r:v:a:0123456789", long_options, &long_index)) != -1)
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
            case 'p':
               *pphase = (uint32_t)atoi(optarg);
               break;
            case 'f':
               *pfinalphase = (uint32_t)atoi(optarg);
               break;
            case 'c':
               *pconsumerCount = (uint32_t)atoi(optarg);
               break;
            case 'r':
               *prollbackRatio = (uint32_t)atoi(optarg);
               break;
            case 'b':
               *pbatchSize = (uint32_t)atoi(optarg);
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
    
    if (*pverboseLevel > testLOGLEVEL_VERBOSE) *pverboseLevel=testLOGLEVEL_VERBOSE; /* BEAM suppression: constant condition */

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-v 0-9] [-0...-9] [-p phase] [-f final-phase] [-c count] [-r ratio]\n", argv[0]);
        fprintf(stderr, "       -p (--phase)         - Set initial phase\n");
        fprintf(stderr, "       -f (--final)         - Set final phase\n");
        fprintf(stderr, "       -c (--consumerCount) - Number of consumers\n");
        fprintf(stderr, "       -r (--rollbackRatio) - Ratio of rollbacks to commit\n");
        fprintf(stderr, "       -b (--batchSize)     - size of transaction batches\n");
        fprintf(stderr, "       -v (--verbose)       - Test Case verbosity\n");
        fprintf(stderr, "       -a (--adminDir)      - Admin directory to use\n");
        fprintf(stderr, "       -0 .. -9             - ISM Trace level\n");
    }

   return usage;
}


/*********************************************************************/
/* NAME:        test_queue4                                          */
/* DESCRIPTION: This program tests the recover of queues and         */
/*              messages from the store.                             */
/*                                                                   */
/*              Each test runs in 2 phases                           */
/*              Phase 1. The queue(s) are created and loaded with    */
/*                       messages.                                   */
/*              Phase 2. At the end of Phase 1, the program re-execs */
/*                       itself and store recover is called to       */
/*                       reload the queues and messages. The queues  */
/*                       and message are checked to ensure they      */
/*                       match what was put in Phase 1.              */
/* USAGE:       test_queue4 [-r] -t <testno>                         */
/*                  -r : When -r is not specified the program runs   */
/*                       Phase 1 and defines and loads the queue(s). */
/*                       When -r is specified, the program runs      */
/*                       Phase 2 and recovers the queues and messages*/
/*                       from the store and verifies everything has  */
/*                       been loaded correctly.                      */
/*                  -t <no> : Testcase number                        */
/*                     1 - Define queue and load with persistent     */
/*                         messages.                                 */
/*                     2 - Define queue and load with non-persistent */
/*                         messages.                                 */
/*                     3 - Define queue and load with mixture of     */
/*                         persistent and non-persistent messages    */
/*                     4 - Define queue and load with transactional  */
/*                         messages, leaving last transaction        */
/*                         uncommited so messages should not be      */
/*                         recovered.                                */
/*********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 5;
    testLogLevel_t testLogLevel = testLOGLEVEL_TESTDESC;
    int rc = 0;
    char *newargv[15];
    int i;
    int osrc;
    uint32_t phase;
    uint32_t finalphase;
    uint32_t consumerCount;
    uint32_t rollbackRatio;
    uint32_t batchSize;
    char QUEUE_NAME[]="TEST_Queue4";
    uint32_t INITIAL_VALUE = 40000000;
    WorkerContext_t *Consumers;
    time_t curTime;
    struct tm tm = { 0 };
    char *adminDir=NULL;
    
    /*****************************************************************/
    /* Parse arguments                                               */
    /*****************************************************************/
    rc = parseArgs( argc
                  , argv
                  , &phase
                  , &finalphase
                  , &consumerCount
                  , &rollbackRatio
                  , &batchSize
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

    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    /*************************************************************/
    /* Prepare the restart command                               */
    /*************************************************************/
    bool phasefound=false, adminDirfound=false;
    char textphase[16];
    sprintf(textphase, "%d", phase+1);
    TEST_ASSERT(argc < 12, ("argc was %d", argc));

    for (i=0; i < argc; i++)
    {
        newargv[i]=argv[i];
        if (strcmp(newargv[i], "-p") == 0)
        {
          newargv[i+1]=textphase;
          i++;
          phasefound=true;
        }
        if (strcmp(newargv[i], "-a") == 0)
        {
          newargv[i+1]=adminDir;
          i++;
          adminDirfound=true;
        }
    }

    if (!phasefound)
    {
        newargv[i++]="-p";
        newargv[i++]=textphase;
    }

    if (!adminDirfound)
    {
        newargv[i++]="-a";
        newargv[i++]=adminDir;
    }

    newargv[i]=NULL;

    /*****************************************************************/
    /* Seed the random number generator and prepare the time for     */
    /* printing                                                      */
    /*****************************************************************/
    curTime = time(NULL);
    srand(curTime);

    (void)localtime_r(&curTime, &tm);
    
    /*****************************************************************/
    /* If this is the first run for this test then cold start the    */
    /* store.                                                        */
    /*****************************************************************/
    if (phase == 0)
    {
        ismEngine_SessionHandle_t hSession;

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

        rc = ism_engine_createClientState("Queue4 Test",
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL, NULL, NULL,
                                          &hClient,
                                          NULL,
                                          0,
                                          NULL);
        TEST_ASSERT(rc == OK, ("Failed to create client state. rc = %d", rc));

        /*************************************************************/
        /* Create the queue we will be using.                        */
        /*************************************************************/
        Test_CreateQ( multiConsumer, QUEUE_NAME);

        /*************************************************************/
        /* Load the queue with a single message.                     */
        /*************************************************************/
        rc=ism_engine_createSession( hClient
                                   , ismENGINE_CREATE_SESSION_OPTION_NONE
                                   , &hSession
                                   , NULL
                                   , 0
                                   , NULL);
        TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

        Test_PutMsgParts( hSession
                        , 0
                        , QUEUE_NAME
                        , NULL // Non transactional
                        , (consumerCount +1) * BATCH_SIZE
                        , INITIAL_VALUE
                        , 0 );

        // test_log_queue(testLOGLEVEL_VERBOSE, QUEUE_NAME, 9, -1, "");

        rc = ism_engine_destroySession( hSession
                                      , NULL
                                      , 0
                                      , NULL);
        TEST_ASSERT(rc == OK, ("Failed to destroy session, rc = %d", rc));
    }
    else
    {
        test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Running test phase %d warm-start",
                 tm.tm_hour, tm.tm_min, tm.tm_sec, phase);

        char *backupDir;

        if ((backupDir = getenv("QUEUE4_BACKUPDIR")) != NULL)
        {
            char *qualifyShared = getenv("QUALIFY_SHARED");

            // space for QUALIFY_SHARED value and com.ibm.ism::%u:store (and null terminator)
            char shmName[(qualifyShared?strlen(qualifyShared):0)+41];
            sprintf(shmName, "com.ibm.ism::%u:store%s", getuid(), qualifyShared ? qualifyShared : "");

            // space for /dev/shm (and null terminator)
            char sourceName[strlen(shmName)+10];
            sprintf(sourceName, "/dev/shm/%s", shmName);

            // space for / (and null terminator)
            char backupName[strlen(backupDir)+strlen(shmName)+2];
            sprintf(backupName, "%s/%s", backupDir, shmName);

            // Allow space for 50 bytes of command (and null terminator)
            char cmd[strlen(sourceName)+strlen(backupName)+51];
            struct stat statBuf;

            if (stat(backupDir, &statBuf) != 0)
            {
                (void)mkdir(backupDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            }

            if (stat(sourceName, &statBuf) == 0)
            {
                sprintf(cmd, "/bin/cp -f %s %s", sourceName, backupName);
                if (system(cmd) == 0)
                {
                    test_log(testLOGLEVEL_TESTPROGRESS,
                             "Copied store shm %s to %s\n", sourceName, backupName);
                }
            }
        }

        /*************************************************************/
        /* Start up the Engine - Warm Start                          */
        /*************************************************************/
        test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start");
        rc = test_engineInit_WARMAUTO;
        if (rc != 0)
        {
            goto mod_exit;
        }
        test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");

        // printf("DEBUG: View of queue before verify\n");
        // test_log_queue(testLOGLEVEL_VERBOSE, QUEUE_NAME, 9, -1, "");

        rc = ism_engine_createClientState("Queue4 Test",
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL, NULL, NULL,
                                          &hClient,
                                          NULL,
                                          0,
                                          NULL);
        TEST_ASSERT(rc == OK, ("Failed to create client state. rc = %d", rc));

        /*************************************************************/
        /* Verify the contents on the messages on the queue          */
        /*************************************************************/
        Test_VerifyQueue( QUEUE_NAME
                        , INITIAL_VALUE );
        
    }

    /*****************************************************************/
    /* Create the consumers.                                         */
    /* First we create a thread which the consumer will post after   */
    /* it receives 5 messages. Then we will define the consumer.     */
    /*****************************************************************/
    Consumers=(WorkerContext_t *)calloc(sizeof(WorkerContext_t), consumerCount);
    TEST_ASSERT(Consumers != NULL, ("Failed to allocate memory for %d consumers", consumerCount));

    for (uint32_t consumerNo=0; consumerNo < consumerCount; consumerNo++)
    {
        Consumers[consumerNo].id = consumerNo;
        Consumers[consumerNo].QueueName = QUEUE_NAME;
        Consumers[consumerNo].BatchSize = batchSize;
        Consumers[consumerNo].Phase = phase;
        Consumers[consumerNo].RollbackRatio = rollbackRatio;

        osrc=pthread_mutex_init(&Consumers[consumerNo].mutex, NULL);
        TEST_ASSERT(osrc == 0, ("Failed to create os mutex. osrc=%d\n", osrc));

        osrc=pthread_cond_init(&Consumers[consumerNo].cond, NULL);
        TEST_ASSERT(osrc == 0, ("Failed to create os cond. osrc=%d\n", osrc));

        osrc = test_task_startThread( &Consumers[consumerNo].hThread ,workerThread, &Consumers[consumerNo] ,"workerThread");
        TEST_ASSERT(osrc == 0, ("Failed to start os thread. osrc=%d\n", osrc));

        // And define the consumer
        defineConsumer( &Consumers[consumerNo] );
    }

    /*****************************************************************/
    /* Pause for a short period allowing the test to run.            */
    /*****************************************************************/
    sleep(2);

    /*****************************************************************/
    /* And restart the server.                                       */
    /*****************************************************************/
    if (phase < finalphase)
    {
        test_log(2, "Restarting...");
        rc = test_execv(newargv[0], newargv);
        TEST_ASSERT(false, ("'test_execv' failed. rc = %d", rc));
    }
    else
    {
        test_log(2, "Success. Test complete!\n");

        // Signal the threads to end
        EndNow = true;

        test_log(4, "Waiting for threads to end.");
        for (uint32_t consumerNo=0; consumerNo < consumerCount; consumerNo++)
        {
            osrc = pthread_mutex_lock(&(Consumers[consumerNo].mutex));
            TEST_ASSERT(osrc==0, ("Failed to lock worker mutex for thread %d. rc = %d",
                         consumerNo, osrc));
            osrc = pthread_cond_signal(&(Consumers[consumerNo].cond));
            TEST_ASSERT(osrc==0, ("Failed to signal worker cond for thread %d. rc = %d",
                         consumerNo, osrc));
            osrc = pthread_mutex_unlock(&(Consumers[consumerNo].mutex));
            TEST_ASSERT(osrc==0, ("Failed to unlock worker mutex for thread %d. rc = %d",
                         consumerNo, osrc));
        }
        for (uint32_t consumerNo=0; consumerNo < consumerCount; consumerNo++)
        {
            (void)pthread_join(Consumers[consumerNo].hThread, NULL);
        }
        test_log(5, "Threads ended.");

        test_engineTerm(true);
        test_processTerm(false);
        test_removeAdminDir(adminDir);
    }

mod_exit:
    return (0);
}
