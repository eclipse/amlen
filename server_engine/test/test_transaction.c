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
/* Description: CUnit tests of Engine Transactions                   */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "transaction.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_file.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
static ismEngine_ClientStateHandle_t hClient = NULL;
// 0   Silent
// 1   Test Name only
// 2   + Test Description
// 3   + Validation checks
// 4   + Test Progress
// 5   + Verbose Test Progress
static uint32_t verboseLevel = 1;   /* 0-9 */

/*********************************************************************/
/* Name:        verbose                                              */
/* Description: Debugging printf routine                             */
/* Parameters:                                                       */
/*     level               - Debug level of the text provided        */
/*     format              - Format string                           */
/*     ...                 - Arguments to format string              */
/*********************************************************************/
static void verbose(int level, char *format, ...)
{
    static char indent[]="--------------------";
    va_list ap;

    if (verboseLevel >= level)
    {
        va_start(ap, format);
        if (format[0] != '\0')
        {
            printf("%.*s ", level+1, indent);
            vprintf(format, ap);
        }
        printf("\n");
        va_end(ap);
    }
}

/*********************************************************************/
/* Name: tiqMsg_t                                                    */
/* Description: Structure used to hold testcase message. Normally    */
/*              used to build an array of messages for a given test. */
/*********************************************************************/
typedef struct
{
    uint32_t MsgId;
    uint8_t Persistence;
    uint8_t Reliability;
    uint32_t MsgLength;
    uint16_t CheckSum;
    bool Received;
    bool Consumed;
    ismEngine_DeliveryHandle_t hDelivery;
} tiqMsg_t;

/*********************************************************************/
/* Name:        calcCheckSum                                         */
/* Description: Simple string checkusm utility                       */
/* Parameters:                                                       */
/*     data                - string data                             */
/*     length              - Length of string                        */
/*********************************************************************/
uint16_t calcCheckSum( char* data, int length )
{
    uint16_t sum1 = 0;
    uint16_t sum2 = 0;
    int i;
    for( i=0; i < length; i++ )
    {
        sum1 = (sum1 + data[i]) % 255;
        sum2 = (sum2 + sum1) % 255;
    }
    return (sum2 << 8) | sum1;
}

/*********************************************************************/
/* Name:        allocMsg                                             */
/* Description: Utility function used to alloacte a single message   */
/* Parameters:                                                       */
/*     hSession            - Session                                 */
/*     Persistence         - Persistance of messages                 */
/*     Reliability         - Message reliability                     */
/*     msgId               - Number of messages to put               */
/*     length              - Length of message payload               */
/*     ppMsg               - Pointer to engine message               */
/*     pcheckSum           - Checksum value                          */
/*********************************************************************/
static void allocMsg( ismEngine_SessionHandle_t hSession
                    , uint8_t Persistence
                    , uint8_t Reliability
                    , uint32_t msgId
                    , uint32_t length
                    , ismEngine_Message_t **ppMsg
                    , uint16_t *pcheckSum)
{
    char *buf = NULL;
    int i;
    int32_t rc;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;

    header.Persistence = Persistence;
    header.Reliability = Reliability;

    if (length)
    {
        buf=(char *)malloc(length);
        TEST_ASSERT_PTR_NOT_NULL(buf);

        snprintf(buf, length, "Message:%d ", msgId);

        // After the NULL fill the buffer with data we can checksum
        for (i=strlen(buf)+1; i < (length-1); i++)
        {
            buf[i] = 'A' + (i % 26);
        }
        buf[length-1]='\0';
        *pcheckSum = calcCheckSum(buf, length);
    }
    else
    {
        *pcheckSum = 0;
    }

    ismMessageAreaType_t areaTypes[] = {ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengths[] = { length };
    void *areas[] = { buf };

    rc = ism_engine_createMessage( &header
                                 , 1
                                 , areaTypes
                                 , areaLengths
                                 , areas
                                 , ppMsg);
    TEST_ASSERT_EQUAL(rc, OK);

    if (buf)
    {
        free(buf);
    }

    return;
}

/*********************************************************************/
/* Name:        generateMsgArray                                     */
/* Description: Utility function used to generate an array of msgs   */
/* Parameters:                                                       */
/*     hSession            - Session                                 */
/*     Persistence         - Persistance of messages                 */
/*     Reliability         - Message reliability                     */
/*     msgCount            - Number of messages to put               */
/*     minMsgSize          - Minimum msg size                        */
/*     maxMsgSize          - Maximum msg size                        */
/*     pMsgs               - Allocated Array of messages             */
/*********************************************************************/
static void generateMsgArray( ismEngine_SessionHandle_t hSession
                            , uint8_t Persistence
                            , uint8_t Reliability
                            , uint32_t msgCount
                            , uint32_t minMsgSize
                            , uint32_t maxMsgSize
                            , tiqMsg_t **ppMsgs )
{
    uint32_t counter;
    tiqMsg_t *pMsgArray;
    uint32_t variation;

    if (Reliability > 0)
    {
        // The MessageConsumer callbacks insists that  QoS 1 or QoS 2
        // messages contain the msg id, so ensure the message size is
        // at least big enough to flow the msg id.
        assert (minMsgSize >= 32);
    }
    assert (maxMsgSize >= minMsgSize);
    variation = maxMsgSize-minMsgSize;

    // Allocate an array for the messages
    pMsgArray = (tiqMsg_t *)malloc(msgCount * sizeof(tiqMsg_t));
    TEST_ASSERT_PTR_NOT_NULL(pMsgArray);

    for (counter=0; counter < msgCount; counter++)
    {
        pMsgArray[counter].MsgId = counter;
        pMsgArray[counter].Persistence = Persistence;
        pMsgArray[counter].Reliability = Reliability;
        pMsgArray[counter].MsgLength = minMsgSize;
        pMsgArray[counter].Received = false;
        pMsgArray[counter].hDelivery = ismENGINE_NULL_DELIVERY_HANDLE;
        if (variation)
        {
            pMsgArray[counter].MsgLength += random() % variation;
        }
    }

    verbose(4, "Generated message array containing %d messages", msgCount);

    *ppMsgs = pMsgArray;

    return;
}

/*********************************************************************/
/* Name:        tiqConsumerContext_t                                 */
/* Description: Context informaton used to pass information to       */
/*              MessageCallback function.                            */
/*********************************************************************/
typedef struct
{
    ismEngine_SessionHandle_t hSession;
    uint8_t Reliability;
    volatile uint32_t msgsReceived;
    tiqMsg_t *pMsgs;
} tiqConsumerContext_t;


typedef struct
{
    ismEngine_SessionHandle_t hSession;
    ismEngine_DeliveryHandle_t hDelivery;
} tiqReceiveAck_t ;

static void completedReceiveAck(int rc, void *handle, void *context)
{
    tiqReceiveAck_t *ackInfo = (tiqReceiveAck_t *)context;

    rc=ism_engine_confirmMessageDelivery( ackInfo->hSession
                                        , NULL
                                        , ackInfo->hDelivery
                                        , ismENGINE_CONFIRM_OPTION_CONSUMED
                                        , NULL, 0, NULL);
    TEST_ASSERT_CUNIT( rc == OK || rc == ISMRC_AsyncCompletion,
                      ("rc was %d\n", rc));
}

/*********************************************************************/
/* Name:        MessageCallback                                      */
/* Description: Callback function invoked with messages              */
/*********************************************************************/
static bool MessageCallback(ismEngine_ConsumerHandle_t  hConsumer,
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
    int32_t rc;
    tiqConsumerContext_t *pContext = *(tiqConsumerContext_t **)pConsumerContext;
    uint16_t checkSum;
    uint32_t msgNum;
    uint32_t confirmOption;

    verbose(5, "Received message: %s", pAreaData[0]);
    TEST_ASSERT_EQUAL(areaCount, 1);

    // Check the message contents
    msgNum = -1;
    if (areaLengths[0] > 0)
    {
        if (pAreaData[0] != NULL)
        {
            sscanf(pAreaData[0], "Message:%d", &msgNum);
        }
        if (msgNum == -1) 
        {
            TEST_ASSERT((pAreaData[0] != NULL && strlen(pAreaData[0]) < 32), ("Unexpected message contents"));
        }
        else
        {
            checkSum = calcCheckSum(pAreaData[0], areaLengths[0]);
            verbose(5, "msgnum %d origchecksum(%d) calcchecksum(%d) ", msgNum,
                    pContext->pMsgs[msgNum].CheckSum, checkSum);

            TEST_ASSERT_EQUAL(checkSum, pContext->pMsgs[msgNum].CheckSum);

            pContext->pMsgs[msgNum].Received = true;
        }
    }

    switch(state)
    {
        case ismMESSAGE_STATE_CONSUMED:
            // When message is delivered as CONSUMED check that either
            // the consumer reliability or the msg reliability is zero
            TEST_ASSERT(((pMsgDetails->Reliability==ismMESSAGE_RELIABILITY_AT_MOST_ONCE) ||
                         (pContext->Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)), ("Unexpected reliability"));

            pContext->msgsReceived++;
            pContext->pMsgs[msgNum].Consumed = true;

            ism_engine_releaseMessage(hMessage);
            break;

        case ismMESSAGE_STATE_DELIVERED:
            TEST_ASSERT_NOT_EQUAL(msgNum, -1);

            // When message is delivered as DELIVERED check that both the
            // msg reliability and the consumer reliability is non-zero.
            TEST_ASSERT_NOT_EQUAL(pMsgDetails->Reliability, ismMESSAGE_RELIABILITY_AT_MOST_ONCE);
            TEST_ASSERT_NOT_EQUAL(pContext->Reliability, ismMESSAGE_RELIABILITY_AT_MOST_ONCE);

//            TEST_ASSERT_EQUAL(deliveryId, 0);

            TEST_ASSERT_NOT_EQUAL(hDelivery, ismENGINE_NULL_DELIVERY_HANDLE);
            pContext->pMsgs[msgNum].hDelivery = hDelivery;

//            verbose(5, "Setting delivery id to %d", msgNum);
//            rc = ism_engine_setMessageDeliveryId( hConsumer
//                                                , hDelivery
//                                                , msgNum
//                                                , NULL
//                                                , 0
//                                                , NULL );
//            TEST_ASSERT_EQUAL(rc, OK);

            confirmOption = ismENGINE_CONFIRM_OPTION_RECEIVED;

            // If the consumer reliability is QoS 2,
            if (pContext->Reliability==ismMESSAGE_RELIABILITY_EXACTLY_ONCE)
            {
                tiqReceiveAck_t ackInfo = {pContext->hSession, hDelivery};

                rc = ism_engine_confirmMessageDelivery( pContext->hSession
                                                      , NULL
                                                      , hDelivery
                                                      , confirmOption
                                                      , &ackInfo
                                                      , sizeof(ackInfo)
                                                      , completedReceiveAck );
                TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                                  ("rc was %d\n", rc));

                if (rc == OK)
                {
                    completedReceiveAck(rc, NULL, &ackInfo);
                }
            }

            confirmOption = ismENGINE_CONFIRM_OPTION_CONSUMED;
            rc=ism_engine_confirmMessageDelivery( pContext->hSession
                                                , NULL
                                                , hDelivery
                                                , confirmOption
                                                , NULL, 0, NULL);
            TEST_ASSERT_CUNIT( rc == OK || rc == ISMRC_AsyncCompletion,
                              ("rc was %d\n", rc));

            pContext->pMsgs[msgNum].Consumed = true;
            ism_engine_releaseMessage(hMessage);

            pContext->msgsReceived++;
            break;
        default:
            TEST_ASSERT(false, ("Unexpected state %d", state));
            break;
    }

    return true;
}
  

/*********************************************************************/
/* Name:            Transaction_Batches                              */
/* Description:     This testcase creates a session and then in a    */
/*                  loop creates a transaction and publishes a       */
/*                  message within the transaction before commiting  */
/*                  the transaction.                                 */
/* Parameters:                                                       */
/*     consumerReliability - Consumer reliability                    */
/*     msgCount            - The number of messages to put           */
/*     batchSize           - Number of messages in each transaction  */
/*     rollbackRatio       - Rollback 1 in N transactions            */
/*     Durable             - Create a durable subscription           */
/*********************************************************************/
void Transaction_Batches( uint32_t ConsumerReliability
                        , uint32_t msgCount
                        , uint32_t batchSize 
                        , uint32_t rollbackRatio 
                        , bool Durable )
{
    int32_t rc = OK;
    int32_t counter;
    uint32_t batchCount=0;
    uint32_t tranCount=0;
    int32_t startBatchCounter = 0;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_TransactionHandle_t hTran = NULL;
    const char *SubName= __func__;
    const char *TopicName= __func__;
    tiqMsg_t *pMsgs;
    tiqConsumerContext_t CallbackContext;
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    ismEngine_Message_t *pMessage;

    verbose(2, "Loop %d times publishing a message within a transaction on topic %s and call commit.",
            msgCount,
            TopicName );

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate the array of message to send
    generateMsgArray( hSession
                    , ismMESSAGE_PERSISTENCE_PERSISTENT
                    , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                    , msgCount
                    , 32
                    , 200
                    , &pMsgs );

    CallbackContext.hSession = hSession;
    CallbackContext.Reliability = ConsumerReliability;
    CallbackContext.msgsReceived = 0;
    CallbackContext.pMsgs = pMsgs;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + ConsumerReliability };

    // Create a subscription for the messages
    if (Durable)
    {
        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;

        rc=sync_ism_engine_createSubscription(
                                  hClient
                                , SubName
                                , NULL
                                , ismDESTINATION_TOPIC
                                , TopicName
                                , &subAttrs
                                , NULL ); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);

        rc=ism_engine_createConsumer( hSession
                                    , ismDESTINATION_SUBSCRIPTION
                                    , SubName
                                    , NULL
                                    , NULL // Unused for QUEUE
                                    , &pCallbackContext
                                    , sizeof(pCallbackContext)
                                    , MessageCallback
                                    , NULL
                                    , test_getDefaultConsumerOptions(subAttrs.subOptions)
                                    , &hConsumer
                                    , NULL
                                    , 0
                                    , NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    else
    {
        rc=ism_engine_createConsumer( hSession
                                    , ismDESTINATION_TOPIC
                                    , TopicName
                                    , &subAttrs
                                    , NULL // Unused for TOPIC
                                    , &pCallbackContext
                                    , sizeof(pCallbackContext)
                                    , MessageCallback
                                    , NULL
                                    , test_getDefaultConsumerOptions(subAttrs.subOptions)
                                    , &hConsumer
                                    , NULL
                                    , 0
                                    , NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Start message delivery
    rc=ism_engine_startMessageDelivery( hSession
                                      , 0
                                      , NULL
                                      , 0
                                      , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    for (counter=0; counter < msgCount; counter++)
    {
        if (hTran == NULL)
        {
            rc = sync_ism_engine_createLocalTransaction( hSession
                                                       , &hTran );
            TEST_ASSERT_EQUAL(rc, OK);
        }

        verbose(5, "Putting message: %d", counter);

        allocMsg( hSession
                , pMsgs[counter].Persistence
                , pMsgs[counter].Reliability
                , counter
                , pMsgs[counter].MsgLength
                , &pMessage
                , &pMsgs[counter].CheckSum);

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_putMessageOnDestination( hSession
                                               , ismDESTINATION_TOPIC
                                               , TopicName
                                               , hTran
                                               , (ismEngine_MessageHandle_t)pMessage
                                               , &pActionsRemaining
                                               , sizeof(pActionsRemaining)
                                               , test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

        batchCount++;

        if ( (batchCount == batchSize) ||
             (counter+1 == msgCount) )
        {
            test_waitForRemainingActions(pActionsRemaining);

            tranCount++;

            if ((rollbackRatio != 0) && ((tranCount % rollbackRatio) == 0))
            {
                verbose(5, "Calling rollback for batch=%d msgCount=%d", batchCount, counter+1);

                rc = ism_engine_rollbackTransaction( hSession
                                                   , hTran
                                                   , NULL
                                                   , 0
                                                   , NULL );
                TEST_ASSERT_EQUAL(rc, OK);
                batchCount = 0;
                hTran = NULL;

                // reset the message counter back to the start of the batch 
                counter = startBatchCounter-1;
            }
            else
            {
                verbose(5, "Calling commit for batch=%d msgCount=%d", batchCount, counter+1);

                rc = sync_ism_engine_commitTransaction( hSession
                                                 , hTran
                                                 , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
                TEST_ASSERT_EQUAL(rc, OK);
                batchCount = 0;
                hTran = NULL;
                startBatchCounter = counter+1;
            }
        }
    }

    test_waitForMessages(&(CallbackContext.msgsReceived), NULL, msgCount, 20);
    // All message should now have been delivered, so stop the consumer
    // and verify that it has received all of the expected messages
    verbose(4, "Put %d messages. %d messages received", msgCount, CallbackContext.msgsReceived);
    TEST_ASSERT_EQUAL(msgCount, CallbackContext.msgsReceived);

    // All done. cleanup
    free(pMsgs);

    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    if (Durable)
    { 
        rc = ism_engine_destroySubscription( hClient
                                           , SubName
                                           , hClient
                                           , NULL
                                           , 0
                                           , NULL );
        TEST_ASSERT_EQUAL(rc, OK);
    }

    return;
}

/*********************************************************************/
/* Name:            Transaction_AutoRollback                         */
/* Description:     This testcase creates a consumer session to      */
/*                  receive messages from a topic.                   */
/*                  The testcase then puts 'N' messages in 'M'       */
/*                  transactions to the consumers topic.             */
/*                  The producer session  is then destroyed and      */
/*                  the consumer verifies that no messages are       */
/*                  received.                                        */
/* Parameters:                                                       */
/*     consumerReliability - Consumer reliability                    */
/*     msgCount            - The number of message per producer      */
/*     batchCount          - The number of producers                 */
/*********************************************************************/
void Transaction_AutoRollback( uint32_t ConsumerReliability
                             , uint32_t msgCount 
                             , uint32_t batchCount )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    int32_t counter;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_SessionHandle_t hConsumerSession = NULL;
    uint32_t i;
    ismEngine_SessionHandle_t hProducerSession;
    ismEngine_TransactionHandle_t *hTranArray;
    ismEngine_QueueStatistics_t stats;
    const char *TopicName= __func__;
    tiqMsg_t *pMsgs;
    tiqConsumerContext_t CallbackContext;
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    ismEngine_Message_t *pMessage;

    verbose(2, "Publish %d messages in %d transactions on topic %s and destroy the session without calling commit.",
            msgCount,
            batchCount,
            TopicName );

    // Create Session for the Producer 
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hProducerSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate the array of message to send. 
    generateMsgArray( hConsumerSession
                    , ismMESSAGE_PERSISTENCE_PERSISTENT
                    , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                    , msgCount
                    , 32
                    , 200
                    , &pMsgs );

    // Setup the the consumer
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hConsumerSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    CallbackContext.hSession = hConsumerSession;
    CallbackContext.Reliability = ConsumerReliability;
    CallbackContext.msgsReceived = 0;
    CallbackContext.pMsgs = pMsgs;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + ConsumerReliability };
    rc=ism_engine_createConsumer( hConsumerSession
                                , ismDESTINATION_TOPIC
                                , TopicName
                                , &subAttrs
                                , NULL // Unused for TOPIC
                                , &pCallbackContext
                                , sizeof(pCallbackContext)
                                , MessageCallback
                                , NULL
                                , test_getDefaultConsumerOptions(subAttrs.subOptions)
                                , &hConsumer
                                , NULL
                                , 0
                                , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Start message delivery
    rc=ism_engine_startMessageDelivery( hConsumerSession
                                      , 0
                                      , NULL
                                      , 0
                                      , NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Setup the Producers
    hTranArray = (ismEngine_TransactionHandle_t *)malloc(sizeof(ismEngine_TransactionHandle_t) * batchCount);
    TEST_ASSERT_PTR_NOT_NULL(hTranArray);

    for (i=0; i < batchCount; i++)
    {
        // Create a transaction for the batch
        rc = sync_ism_engine_createLocalTransaction( hProducerSession
                                                   , &hTranArray[i] );
        TEST_ASSERT_EQUAL(rc, OK);

        for (counter=0; counter < msgCount; counter++)
        {
            verbose(5, "Putting message: %d", counter);

            allocMsg( hProducerSession
                    , pMsgs[counter].Persistence
                    , pMsgs[counter].Reliability
                    , counter
                    , pMsgs[counter].MsgLength
                    , &pMessage
                    , &pMsgs[counter].CheckSum);
    
            rc = ism_engine_putMessageOnDestination( hProducerSession
                                                   , ismDESTINATION_TOPIC
                                                   , TopicName
                                                   , hTranArray[i]
                                                   , (ismEngine_MessageHandle_t)pMessage
                                                   , NULL
                                                   , 0
                                                   , NULL );
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    verbose(3, "Put %d messages each in %d batches.", msgCount, batchCount);

    // Verify that the messages are available on the queue
    ieq_getStats(pThreadData, ((ismEngine_Consumer_t *)hConsumer)->queueHandle, &stats);
    ieq_setStats(((ismEngine_Consumer_t *)hConsumer)->queueHandle, NULL, ieqSetStats_RESET_ALL);

    if (ConsumerReliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
    {
        verbose(4, "Expected to find 0 msgs available on queue. found %d",
                0, stats.BufferedMsgs);

        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
    }
    else
    {
        verbose(4, "Expected to find %d msgs available on queue. found %d",
                batchCount * msgCount, stats.BufferedMsgs);

        TEST_ASSERT_EQUAL(stats.BufferedMsgs, (batchCount * msgCount));
    }

    // Now disconnect the producer
    rc = ism_engine_destroySession( hProducerSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    free(hTranArray);

    // And verify that no messages have been received by the consumer.
    verbose(3, "Put %d messages. %d messages received", (batchCount * msgCount), CallbackContext.msgsReceived);
    TEST_ASSERT_EQUAL(CallbackContext.msgsReceived, 0);

    // Verify that the the queue is now empty
    ieq_getStats(pThreadData, ((ismEngine_Consumer_t *)hConsumer)->queueHandle, &stats);
    ieq_setStats(((ismEngine_Consumer_t *)hConsumer)->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);


    // All done. cleanup
    free(pMsgs);

    // Cleaup session
    rc = ism_engine_destroySession( hConsumerSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
   
    return;
}

/*********************************************************************/
/* Name:            Transaction_MultiBatch                           */
/* Description:     This testcase creates a consumer session to      */
/*                  receive messages from a topic.                   */
/*                  The testcase then puts 'N' messages in 'M'       */
/*                  transactions to the consumers topic.             */
/*                  The producer then either commits/rollsback       */
/*                  each of the transactions and the number of msgs  */
/*                  received by the consumer is checked.             */
/* Parameters:                                                       */
/*     consumerReliability - Consumer reliability                    */
/*     msgCount            - Number of messages in each batch        */
/*     batchCount          - Number of batches                       */
/*     rollbackRatio       - Rollback ratio                          */
/*********************************************************************/
void Transaction_MultiBatch( uint32_t ConsumerReliability
                           , uint32_t msgCount 
                           , uint32_t batchCount
                           , uint32_t rollbackRatio )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    int32_t counter;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_SessionHandle_t hConsumerSession = NULL;
    uint32_t i;
    ismEngine_SessionHandle_t hProducerSession;
    ismEngine_TransactionHandle_t *hTranArray;
    ismEngine_QueueStatistics_t stats;
    const char *TopicName= __func__;
    tiqMsg_t *pMsgs;
    tiqConsumerContext_t CallbackContext;
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    uint32_t expectedQDepth = 0;
    uint32_t expectedMsgs = 0;
    ismEngine_Message_t *pMessage;

    verbose(2, "Publish %d messages in %d transactions on topic %s and destroy the session without calling commit.",
            msgCount,
            batchCount,
            TopicName );

    // Create Session for the Producer 
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hProducerSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate the array of message to send. 
    generateMsgArray( hConsumerSession
                    , ismMESSAGE_PERSISTENCE_PERSISTENT
                    , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                    , msgCount
                    , 32
                    , 200
                    , &pMsgs );

    // Setup the the consumer
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hConsumerSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    CallbackContext.hSession = hConsumerSession;
    CallbackContext.Reliability = ConsumerReliability;
    CallbackContext.msgsReceived = 0;
    CallbackContext.pMsgs = pMsgs;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + ConsumerReliability };
    rc=ism_engine_createConsumer( hConsumerSession
                                , ismDESTINATION_TOPIC
                                , TopicName
                                , &subAttrs
                                , NULL // Unused for TOPIC
                                , &pCallbackContext
                                , sizeof(pCallbackContext)
                                , MessageCallback
                                , NULL
                                , test_getDefaultConsumerOptions(subAttrs.subOptions)
                                , &hConsumer
                                , NULL
                                , 0
                                , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Start message delivery
    rc=ism_engine_startMessageDelivery( hConsumerSession
                                      , 0
                                      , NULL
                                      , 0
                                      , NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Setup the Producers
    hTranArray = (ismEngine_TransactionHandle_t *)malloc(sizeof(ismEngine_TransactionHandle_t) * batchCount);
    TEST_ASSERT_PTR_NOT_NULL(hTranArray);

    for (i=0; i < batchCount; i++)
    {
        // Create a transaction for the batch
        rc = sync_ism_engine_createLocalTransaction( hProducerSession
                                                   , &hTranArray[i]);
        TEST_ASSERT_EQUAL(rc, OK);

        for (counter=0; counter < msgCount; counter++)
        {
            verbose(5, "Putting message: %d", counter);
    
            allocMsg( hProducerSession
                    , pMsgs[counter].Persistence
                    , pMsgs[counter].Reliability
                    , counter
                    , pMsgs[counter].MsgLength
                    , &pMessage
                    , &pMsgs[counter].CheckSum);
    
            rc = ism_engine_putMessageOnDestination( hProducerSession
                                                   , ismDESTINATION_TOPIC
                                                   , TopicName
                                                   , hTranArray[i]
                                                   , (ismEngine_MessageHandle_t)pMessage
                                                   , NULL
                                                   , 0
                                                   , NULL );
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    verbose(3, "Put %d messages each in %d batches.", msgCount, batchCount);

    // Verify that the messages are available on the queue
    ieq_getStats(pThreadData, ((ismEngine_Consumer_t *)hConsumer)->queueHandle, &stats);
    ieq_setStats(((ismEngine_Consumer_t *)hConsumer)->queueHandle, NULL, ieqSetStats_RESET_ALL);
    if (ConsumerReliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
    {
        verbose(4, "Expected to find 0 msgs available on queue. found %d",
                0, stats.BufferedMsgs);

        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
    }
    else
    {
        verbose(4, "Expected to find %d msgs available on queue. found %d",
                batchCount * msgCount, stats.BufferedMsgs);

        TEST_ASSERT_EQUAL(stats.BufferedMsgs, (batchCount * msgCount));
    }

    // Now commit/rollback the transactions and verify the messages are
    // received.
    for (i=0; i < batchCount; i++)
    {
        if (rollbackRatio && (((i+1) % rollbackRatio) == 0))
        {
            verbose(4, "Calling rollback for batch=%d", i);

            rc = ism_engine_rollbackTransaction( hProducerSession
                                               , hTranArray[i]
                                               , NULL
                                               , 0
                                               , NULL );
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            verbose(4, "Calling commit for batch=%d", i);

            rc = sync_ism_engine_commitTransaction( hProducerSession
                                             , hTranArray[i]
                                             , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
            TEST_ASSERT_EQUAL(rc, OK);

            expectedMsgs += msgCount;
        }

        // Check that the expected number of messages has been received 
        // by the consumer
        verbose(5, "Expected consumer to have received %d messages. Actually received %d", expectedMsgs, CallbackContext.msgsReceived);
        TEST_ASSERT_EQUAL(expectedMsgs, CallbackContext.msgsReceived);

        // Check that the number of messages on the queue has been reduced
        if (ConsumerReliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
        {
            expectedQDepth = 0;
        }
        else
        {
            expectedQDepth = (batchCount - (i+1)) * msgCount;
        }
 
        ieq_getStats(pThreadData, ((ismEngine_Consumer_t *)hConsumer)->queueHandle, &stats);
        ieq_setStats(((ismEngine_Consumer_t *)hConsumer)->queueHandle, NULL, ieqSetStats_RESET_ALL);
        verbose(5, "Expected queue to contain %d messages. Actually contains %d", expectedQDepth, stats.BufferedMsgs);
        TEST_ASSERT_EQUAL(expectedQDepth, stats.BufferedMsgs);
    }


    // All done. cleanup
    free(pMsgs);

    // Now disconnect the producer
    rc = ism_engine_destroySession( hProducerSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
    free(hTranArray);

    // Cleaup session
    rc = ism_engine_destroySession( hConsumerSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);
   
    return;
}

/*********************************************************************/
/* Name:            Transaction_DestroySubscriber                    */
/* Description:     This testcase creates a consumer session to      */
/*                  receive messages from a topic.                   */
/*                  The testcase then puts 'N' messages in a         */
/*                  transactions to the consumers topic.             */
/*                  Before the producer calls commit, the subscriber */
/*                  unsubscribes.                                    */
/* Parameters:                                                       */
/*     consumerReliability - Consumer reliability                    */
/*     Durable             - Durable subscriber?                     */
/*     msgCount            - Number of messages in each batch        */
/*     Commit              - Commit or Rollback transaction          */
/*********************************************************************/
void durableDestroyedCB(int32_t retcode, void *handle, void *pContext)
{
    bool *bDestroyedBool = *(bool **)pContext;

    *bDestroyedBool = true;
}

void Transaction_DestroySubscriber( uint32_t ConsumerReliability
                                  , bool Durable 
                                  , uint32_t msgCount
                                  , bool fCommit )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_QueueStatistics_t stats;
    int32_t counter;
    uint32_t expectedQDepth=0;
    ismEngine_SessionHandle_t hProducerSession;
    ismEngine_SessionHandle_t hConsumerSession;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_TransactionHandle_t hTran = NULL;
    const char *SubName= __func__;
    const char *TopicName= __func__;
    tiqMsg_t *pMsgs;
    tiqConsumerContext_t CallbackContext;
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    bool bDestroyed = false;
    ismEngine_Message_t *pMessage;

    verbose(2, "Loop %d times publishing a message within a transaction on topic %s and call %s.",
            msgCount,
            TopicName,
            fCommit?"Commit":"Rollback" );

    // Create Session for the producer
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hProducerSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create Session for the consumer
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hConsumerSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate the array of message to send
    generateMsgArray( hProducerSession
                    , ismMESSAGE_PERSISTENCE_PERSISTENT
                    , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                    , msgCount
                    , 32
                    , 200
                    , &pMsgs );

    CallbackContext.hSession = hConsumerSession;
    CallbackContext.Reliability = ConsumerReliability;
    CallbackContext.msgsReceived = 0;
    CallbackContext.pMsgs = pMsgs;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + ConsumerReliability };

    // Create a subscription for the messages
    if (Durable)
    {
        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;

        rc=sync_ism_engine_createSubscription(
                                  hClient
                                , SubName
                                , NULL
                                , ismDESTINATION_TOPIC
                                , TopicName
                                , &subAttrs
                                , NULL ); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);

        rc=ism_engine_createConsumer( hConsumerSession
                                    , ismDESTINATION_SUBSCRIPTION
                                    , SubName
                                    , NULL
                                    , NULL // Owning client same as session client
                                    , &pCallbackContext
                                    , sizeof(pCallbackContext)
                                    , MessageCallback
                                    , NULL
                                    , test_getDefaultConsumerOptions(subAttrs.subOptions)
                                    , &hConsumer
                                    , NULL
                                    , 0
                                    , NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    else
    {
        rc=ism_engine_createConsumer( hConsumerSession
                                    , ismDESTINATION_TOPIC
                                    , TopicName
                                    , &subAttrs
                                    , NULL // Unused for TOPIC
                                    , &pCallbackContext
                                    , sizeof(pCallbackContext)
                                    , MessageCallback
                                    , NULL
                                    , test_getDefaultConsumerOptions(subAttrs.subOptions)
                                    , &hConsumer
                                    , NULL
                                    , 0
                                    , NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Start message delivery
    rc=ism_engine_startMessageDelivery( hConsumerSession
                                      , 0
                                      , NULL
                                      , 0
                                      , NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    rc = sync_ism_engine_createLocalTransaction( hProducerSession
                                               , &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    for (counter=0; counter < msgCount; counter++)
    {
        verbose(5, "Putting message: %d", counter);

        allocMsg( hProducerSession
                , pMsgs[counter].Persistence
                , pMsgs[counter].Reliability
                , counter
                , pMsgs[counter].MsgLength
                , &pMessage
                , &pMsgs[counter].CheckSum);
    
        rc = sync_ism_engine_putMessageOnDestination( hProducerSession
                                               , ismDESTINATION_TOPIC
                                               , TopicName
                                               , hTran
                                               , (ismEngine_MessageHandle_t)pMessage);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Verify that there are the correct number of messages pending on 
    // the queue.
    expectedQDepth = (ConsumerReliability==ismMESSAGE_RELIABILITY_AT_MOST_ONCE)?
                     0 : msgCount;
    ieq_getStats(pThreadData, ((ismEngine_Consumer_t *)hConsumer)->queueHandle, &stats);
    ieq_setStats(((ismEngine_Consumer_t *)hConsumer)->queueHandle, NULL, ieqSetStats_RESET_ALL);
    verbose(5, "Expected queue to contain %d messages. Actually contains %d", expectedQDepth, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(expectedQDepth, stats.BufferedMsgs);

    // Stop message delivery
    rc=ism_engine_stopMessageDelivery( hConsumerSession
                                     , NULL
                                     , 0
                                     , NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Now (deregister the subscription) and destroy the consumer session 
    if (Durable)
    { 
        bool *pbDestroyed = &bDestroyed;

        rc = ism_engine_destroySubscription( hClient
                                           , SubName
                                           , hClient
                                           , &pbDestroyed
                                           , sizeof(bool *)
                                           , durableDestroyedCB );
        // Cannot destroy subscription with an active consumer
        TEST_ASSERT_EQUAL(rc, ISMRC_DestinationInUse);

        // Destroy the consumer
        rc = ism_engine_destroyConsumer(hConsumer,
                                        NULL,
                                        0,
                                        NULL);

        // Retry the destroy which should now succeed (but because the transaction
        // which releases the subscriber list is still pending, internally this
        // will not happen until the commit)
        rc = ism_engine_destroySubscription( hClient
                                           , SubName
                                           , hClient
                                           , &pbDestroyed
                                           , sizeof(bool *)
                                           , durableDestroyedCB );
        TEST_ASSERT_EQUAL(rc, OK);
        bDestroyed = true;
    }
    else
    {
        bDestroyed = true;
    }

    // Cleaup session
    rc = ism_engine_destroySession( hConsumerSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    // Now commit (or rollback the session )
    if (fCommit)
    {
        verbose(5, "Calling commit.");

        rc = sync_ism_engine_commitTransaction( hProducerSession
                                         , hTran
                                         , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    else
    {
        verbose(5, "Calling rollback.");

        rc = ism_engine_rollbackTransaction( hProducerSession
                                           , hTran
                                           , NULL
                                           , 0
                                           , NULL );
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Check that the subscription actually got destroyed
    TEST_ASSERT(bDestroyed, ("bDestroyed"));

    // All done. cleanup
    free(pMsgs);

    // Cleaup session
    rc = sync_ism_engine_destroySession( hProducerSession );
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}


/*********************************************************************/
/*********************************************************************/
/* Testcase definitions                                              */
/*********************************************************************/
/*********************************************************************/

/*********************************************************************/
/* TestCase:       Transaction_SQ_ND_Commit                          */
/* Description:    Put 200 messages to a topic each in it's own      */
/*                 transaction, verify that all messages are         */
/*                 received.                                         */
/*********************************************************************/
void Transaction_SQ_ND_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_ND_Commit...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                       , 200
                       , 1
                       , 0
                       , false );

    verbose(4, "Completed Transaction_SQ_ND_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_D_Commit                           */
/* Description:    Put 200 messages to a topic each in it's own      */
/*                 transaction, verify that all messages are         */
/*                 received.                                         */
/*********************************************************************/
void Transaction_SQ_D_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_D_Commit...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                       , 200
                       , 1
                       , 0
                       , true );

    verbose(4, "Completed Transaction_SQ_D_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_ND_B7_Commit                       */
/* Description:    Put 200 messages to a topic each in batches of    */
/*                 7 messages and verify that all messages are       */
/*                 received.                                         */
/*********************************************************************/
void Transaction_SQ_ND_B7_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_ND_B7_Commit...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                       , 200
                       , 7
                       , 0
                       , false );

    verbose(4, "Completed Transaction_SQ_ND_B7_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_D_B7_Commit                        */
/* Description:    Put 200 messages to a topic each in batches of    */
/*                 7 messages and verify that all messages are       */
/*                 received.                                         */
/*********************************************************************/
void Transaction_SQ_D_B7_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_D_B7_Commit...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                       , 200
                       , 7
                       , 0
                       , true );

    verbose(4, "Completed Transaction_SQ_D_B7_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_AutoRollback                       */
/* Description:    Put 200 *5 messages to a topic and disconnect     */
/*                 session without committing. Check no msgs are     */
/*                 delivered.                                        */
/*********************************************************************/
void Transaction_SQ_AutoRollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_AutoRollback...");

    Transaction_AutoRollback( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                            , 200
                            , 5 );

    verbose(4, "Completed Transaction_SQ_AutoRollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_MultiBatch                         */
/* Description:    Create a producers which then put's 5 batches of  */
/*                 200 messages on a topic. Each batch has it's own  */
/*                 transaction.                                      */
/*                 Commit each transaction, except the last tran-    */
/*                 saction which is rolled back. Check that only the */
/*                 correct messages are deliver.                     */
/*********************************************************************/
void Transaction_SQ_MultiBatch( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_MultiBatch...");

    Transaction_MultiBatch( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                          , 20
                          , 40
                          , 5 );

    verbose(4, "Completed Transaction_SQ_MultiBatch");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_DestroySub_Commit                  */
/* Description:    Creates a subscriber on a topic and then publishes*/
/*                 200 messages on the topic, and then deletes the   */
/*                 subscription before commit is called.             */
/*********************************************************************/
void Transaction_SQ_DestroySub_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_DestroySub_Commit...");

    Transaction_DestroySubscriber( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                                 , false
                                 , 50
                                 , true );

    verbose(4, "Completed Transaction_SQ_DestroySub_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_DestroySub_Rollback                */
/* Description:    Creates a subscriber on a topic and then publishes*/
/*                 200 messages on the topic, and then deletes the   */
/*                 subscription before rollback is called.           */
/*********************************************************************/
void Transaction_SQ_DestroySub_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_DestroySub_Rollback...");

    Transaction_DestroySubscriber( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                                 , false
                                 , 50
                                 , false );

    verbose(4, "Completed Transaction_SQ_DestroySub_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_DestroyDurSub_Commit               */
/* Description:    Creates a subscriber on a topic and then publishes*/
/*                 200 messages on the topic, and then deletes the   */
/*                 subscription before commit is called.             */
/*********************************************************************/
void Transaction_SQ_DestroyDurSub_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_DestroyDurSub_Commit...");

    Transaction_DestroySubscriber( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                                 , true
                                 , 50
                                 , true );

    verbose(4, "Completed Transaction_SQ_DestroyDurSub_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_DestroyDurSub_Rollback             */
/* Description:    Creates a subscriber on a topic and then publishes*/
/*                 200 messages on the topic, and then deletes the   */
/*                 subscription before rollback is called.           */
/*********************************************************************/
void Transaction_SQ_DestroyDurSub_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_DestroyDurSub_Rollback...");

    Transaction_DestroySubscriber( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                                 , true
                                 , 50
                                 , false );

    verbose(4, "Completed Transaction_SQ_DestroyDurSub_Rollback");

    return;
}


/*********************************************************************/
/* TestCase:       Transaction_IQ_ND_Commit                          */
/* Description:    Put 200 messages to a topic each in it's own      */
/*                 transaction, verify that all messages are         */
/*                 received.                                         */
/*********************************************************************/
void Transaction_IQ_ND_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_ND_Commit...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                       , 200
                       , 1
                       , 0
                       , false );

    verbose(4, "Completed Transaction_IQ_ND_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_D_Commit                           */
/* Description:    Put 200 messages to a topic each in it's own      */
/*                 transaction, verify that all messages are         */
/*                 received.                                         */
/*********************************************************************/
void Transaction_IQ_D_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_D_Commit...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                       , 200
                       , 1
                       , 0
                       , true );

    verbose(4, "Completed Transaction_IQ_D_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_ND_B7_Commit                       */
/* Description:    Put 200 messages to a topic each in batches of    */
/*                 7 messages and verify that all messages are       */
/*                 received.                                         */
/*********************************************************************/
void Transaction_IQ_ND_B7_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_ND_B7_Commit...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                       , 200
                       , 7
                       , 0
                       , false );

    verbose(4, "Completed Transaction_IQ_ND_B7_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_D_B7_Commit                        */
/* Description:    Put 200 messages to a topic each in batches of    */
/*                 7 messages and verify that all messages are       */
/*                 received.                                         */
/*********************************************************************/
void Transaction_IQ_D_B7_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_D_B7_Commit...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                       , 200
                       , 7
                       , 0
                       , true );

    verbose(4, "Completed Transaction_IQ_D_B7_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_ND_Rollback                        */
/* Description:    Put 200 messages to a topic each in it's own      */
/*                 transaction, verify that all messages are         */
/*                 received.                                         */
/*********************************************************************/
void Transaction_SQ_ND_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_ND_Rollback...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                       , 200
                       , 1
                       , 3
                       , false );

    verbose(4, "Completed Transaction_SQ_ND_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_D_Rollback                         */
/* Description:    Put 200 messages to a topic each in it's own      */
/*                 transaction, verify that all messages are         */
/*                 received.                                         */
/*********************************************************************/
void Transaction_SQ_D_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_ND_Rollback...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                       , 200
                       , 1
                       , 3
                       , true );

    verbose(4, "Completed Transaction_SQ_ND_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_ND_B7_Rollback                     */
/* Description:    Put 200 messages to a topic each in batches of    */
/*                 7 messages and verify that all messages are       */
/*                 received.                                         */
/*********************************************************************/
void Transaction_SQ_ND_B7_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_ND_B7_Rollback...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                       , 200
                       , 7
                       , 3
                       , false );

    verbose(4, "Completed Transaction_SQ_ND_B7_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_SQ_D_B7_Rollback                      */
/* Description:    Put 200 messages to a topic each in batches of    */
/*                 7 messages and verify that all messages are       */
/*                 received.                                         */
/*********************************************************************/
void Transaction_SQ_D_B7_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_SQ_D_B7_Rollback...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                       , 200
                       , 7
                       , 3
                       , true );

    verbose(4, "Completed Transaction_SQ_D_B7_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_ND_Rollback                        */
/* Description:    Put 200 messages to a topic each in it's own      */
/*                 transaction, verify that all messages are         */
/*                 received.                                         */
/*********************************************************************/
void Transaction_IQ_ND_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_ND_Rollback...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                       , 200
                       , 1
                       , 3
                       , false );

    verbose(4, "Completed Transaction_IQ_ND_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_D_Rollback                         */
/* Description:    Put 200 messages to a topic each in it's own      */
/*                 transaction, verify that all messages are         */
/*                 received.                                         */
/*********************************************************************/
void Transaction_IQ_D_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_ND_Rollback...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                       , 200
                       , 1
                       , 3
                       , true );

    verbose(4, "Completed Transaction_IQ_ND_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_ND_B7_Rollback                     */
/* Description:    Put 200 messages to a topic each in batches of    */
/*                 7 messages and verify that all messages are       */
/*                 received.                                         */
/*********************************************************************/
void Transaction_IQ_ND_B7_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_ND_B7_Rollback...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                       , 200
                       , 7
                       , 3
                       , false );

    verbose(4, "Completed Transaction_IQ_ND_B7_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_D_B7_Rollback                      */
/* Description:    Put 200 messages to a topic each in batches of    */
/*                 7 messages and verify that all messages are       */
/*                 received.                                         */
/*********************************************************************/
void Transaction_IQ_D_B7_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_D_B7_Rollback...");

    Transaction_Batches( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                       , 200
                       , 7
                       , 3
                       , true );

    verbose(4, "Completed Transaction_IQ_D_B7_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_AutoRollback                       */
/* Description:    Put 200 * 5 messages to a topic and disconnect    */
/*                 session without committing. Check no msgs are     */
/*                 delivered.                                        */
/*********************************************************************/
void Transaction_IQ_AutoRollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_AutoRollback...");

    Transaction_AutoRollback( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                            , 200
                            , 5 );

    verbose(4, "Completed Transaction_IQ_AutoRollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_MultiBatch                         */
/* Description:    Create a producers which then put's 5 batches of  */
/*                 200 messages on a topic. Each batch has it's own  */
/*                 transaction.                                      */
/*                 Commit each transaction, except the last tran-    */
/*                 saction which is rolled back. Check that only the */
/*                 correct messages are deliver.                     */
/*********************************************************************/
void Transaction_IQ_MultiBatch( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_MultiBatch...");

    Transaction_MultiBatch( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                          , 20
                          , 40
                          , 5 );

    verbose(4, "Completed Transaction_IQ_MultiBatch");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_DestroySub_Commit                  */
/* Description:    Creates a subscriber on a topic and then publishes*/
/*                 200 messages on the topic, and then deletes the   */
/*                 subscription before commit is called.             */
/*********************************************************************/
void Transaction_IQ_DestroySub_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_DestroySub_Commit...");

    Transaction_DestroySubscriber( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                                 , false
                                 , 50
                                 , true );

    verbose(4, "Completed Transaction_IQ_DestroySub_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_DestroySub_Rollback                */
/* Description:    Creates a subscriber on a topic and then publishes*/
/*                 200 messages on the topic, and then deletes the   */
/*                 subscription before rollback is called.           */
/*********************************************************************/
void Transaction_IQ_DestroySub_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_DestroySub_Rollback...");

    Transaction_DestroySubscriber( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                                 , false
                                 , 50
                                 , false );

    verbose(4, "Completed Transaction_IQ_DestroySub_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_DestroyDurSub_Commit               */
/* Description:    Creates a subscriber on a topic and then publishes*/
/*                 200 messages on the topic, and then deletes the   */
/*                 subscription before commit is called.             */
/*********************************************************************/
void Transaction_IQ_DestroyDurSub_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_DestroyDurSub_Commit...");

    Transaction_DestroySubscriber( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                                 , true
                                 , 50
                                 , true );

    verbose(4, "Completed Transaction_IQ_DestroyDurSub_Commit");

    return;
}

/*********************************************************************/
/* TestCase:       Transaction_IQ_DestroyDurSub_Rollback             */
/* Description:    Creates a subscriber on a topic and then publishes*/
/*                 200 messages on the topic, and then deletes the   */
/*                 subscription before rollback is called.           */
/*********************************************************************/
void Transaction_IQ_DestroyDurSub_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_IQ_DestroyDurSub_Rollback...");

    Transaction_DestroySubscriber( ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                                 , true
                                 , 50
                                 , false );

    verbose(4, "Completed Transaction_IQ_DestroyDurSub_Rollback");

    return;
}

/*********************************************************************/
/* Name:            Transaction_Savepoints                           */
/* Description:     This testcase tries various things using         */
/*                  savepoints.                                      */
/*********************************************************************/
typedef struct testSPContext_t
{
    uint32_t callCount;
    uint32_t commitCount;
    uint32_t rollbackCount;
    uint32_t savepointRollbackCount;
    uint32_t otherCount;
    uint32_t SPACount;
} testSPContext_t;

void testSavepointFunc(ietrReplayPhase_t        Phase,
                       ieutThreadData_t        *pThreadData,
                       ismEngine_Transaction_t *pTran,
                       void                    *entry,
                       ietrReplayRecord_t      *pRecord,
                       ismEngine_DelivererContext_t * delivererContext)
{
    testSPContext_t *pContext = *(testSPContext_t **)entry;

    pContext->callCount += 1;

    switch(Phase)
    {
        case Commit:
            pContext->commitCount++;
            break;
        case Rollback:
            pContext->rollbackCount++;
            break;
        case SavepointRollback:
            pContext->savepointRollbackCount++;
            break;
        default:
            pContext->otherCount++;
            break;
    }

    if (pTran->pActiveSavepoint != NULL) pContext->SPACount += 1;
}

void Transaction_Savepoints( void )
{
    int32_t rc = OK;
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_SessionHandle_t hSession = NULL;
    ismEngine_Transaction_t *pTran;
    ietrSavepoint_t *pSavepoint;
    testSPContext_t context = {0};
    testSPContext_t *pContext = &context;

    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Transaction_Savepoints...");

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t expectCall = 0;
    uint32_t expectSPA = 0;
    uint32_t expectCommit = 0;
    uint32_t expectRollback = 0;
    uint32_t expectSavepointRollback = 0;

    for(int32_t i=0; i<6; i++)
    {
        rc = ietr_createLocal(pThreadData, hSession, false, false, NULL, &pTran);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ietr_beginSavepoint(pThreadData, pTran, &pSavepoint);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(pSavepoint);
        TEST_ASSERT_EQUAL(pTran->pActiveSavepoint, pSavepoint);

        // try and begin 2 savepoints at once
        rc = ietr_beginSavepoint(pThreadData, pTran, &pSavepoint);
        TEST_ASSERT_EQUAL(rc, ISMRC_SavepointActive);
        TEST_ASSERT_EQUAL(pTran->pActiveSavepoint, pSavepoint);

        // Prove that you can end the current savepoint and start another
        if (i<3)
        {
            ietrSavepoint_t *pOriginalSavepoint = pSavepoint;
            rc = ietr_endSavepoint(pThreadData, pTran, pSavepoint, None);
            TEST_ASSERT_EQUAL(rc, OK);
            rc = ietr_beginSavepoint(pThreadData, pTran, &pSavepoint);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(pSavepoint);
            TEST_ASSERT_EQUAL(pTran->pActiveSavepoint, pSavepoint);
            TEST_ASSERT_NOT_EQUAL(pSavepoint, pOriginalSavepoint);
        }
        // Add an SLE so we can test things
        rc = ietr_softLogAdd(pThreadData,
                             pTran,
                             ietrSLE_TT_OLD_STORE_SUBSC_DEFN,
                             testSavepointFunc,
                             NULL,
                             Commit|Rollback|SavepointRollback,
                             &pContext, sizeof(pContext), 0, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        switch(i)
        {
            // Commit and prove savepoint is gone.

            case 0: // Standard commit
                rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, hSession, NULL, NULL);
                expectCall++;
                expectCommit++;
                break;
            case 1: // Standard rollback
                rc = ietr_rollback(pThreadData, pTran, hSession, IETR_ROLLBACK_OPTIONS_NONE);
                expectCall++;
                expectRollback++;
                break;
            case 2: // Call SavepointRollback
                rc = ietr_endSavepoint(pThreadData, pTran, pSavepoint, SavepointRollback);
                TEST_ASSERT_EQUAL(rc, OK);
                expectCall++;
                expectSavepointRollback++;
                expectSPA++;
                rc = ietr_rollback(pThreadData, pTran, hSession, IETR_ROLLBACK_OPTIONS_NONE);
                expectCall++;
                expectRollback++;
                break;
            case 3: // Call with invalid savepoint
                rc = ietr_endSavepoint(pThreadData, pTran, (ietrSavepoint_t *)pTran, SavepointRollback);
                TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);
                rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, hSession, NULL, NULL);
                expectCall++;
                expectCommit++;
                break;
            case 4: // Mess (illegally) with the pre-resolve count
                pTran->preResolveCount++;
                rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, hSession, NULL, NULL);
                TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);
                pTran->preResolveCount--;
                rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, hSession, NULL, NULL);
                TEST_ASSERT_EQUAL(rc, OK);
                expectCall++;
                expectCommit++;
                break;
            case 5: // Mess (illegally) with the TranState
                pTran->TranState = ismTRANSACTION_STATE_NONE;
                rc = ietr_commit(pThreadData, pTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, hSession, NULL, NULL);
                TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);
                rc = ietr_rollback(pThreadData, pTran, hSession, IETR_ROLLBACK_OPTIONS_NONE);
                TEST_ASSERT_EQUAL(rc, ISMRC_InvalidOperation);
                pTran->TranState = ismTRANSACTION_STATE_IN_FLIGHT;
                rc = ietr_rollback(pThreadData, pTran, hSession, IETR_ROLLBACK_OPTIONS_NONE);
                expectCall++;
                expectRollback++;
                break;
        }

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(context.SPACount, expectSPA);
        TEST_ASSERT_EQUAL(context.callCount, expectCall);
        TEST_ASSERT_EQUAL(context.commitCount, expectCommit);
        TEST_ASSERT_EQUAL(context.rollbackCount, expectRollback);
        TEST_ASSERT_EQUAL(context.savepointRollbackCount, expectSavepointRollback);
        TEST_ASSERT_EQUAL(context.otherCount, 0);
    }

    // Cleaup session
    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);


    return;
}

/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Transactions                     */
/* Description:    Tests for transactions                            */
/*********************************************************************/
CU_TestInfo ISM_Engine_CUnit_IQ_Transactions[] = {
    { "Transaction_IQ_ND_Commit",            Transaction_IQ_ND_Commit },
    { "Transaction_IQ_D_Commit",             Transaction_IQ_D_Commit },
    { "Transaction_IQ_ND_B7_Commit",         Transaction_IQ_ND_B7_Commit },
    { "Transaction_IQ_D_B7_Commit",          Transaction_IQ_D_B7_Commit },
    { "Transaction_IQ_ND_Rollback",          Transaction_IQ_ND_Rollback },
    { "Transaction_IQ_D_Rollback",           Transaction_IQ_D_Rollback },
    { "Transaction_IQ_ND_B7_Rollback",       Transaction_IQ_ND_B7_Rollback },
    { "Transaction_IQ_D_B7_Rollback",        Transaction_IQ_D_B7_Rollback },
    { "Transaction_IQ_AutoRollback",         Transaction_IQ_AutoRollback },
    { "Transaction_IQ_MultiBatch",           Transaction_IQ_MultiBatch },
    { "Transaction_IQ_DestroySub_Commit",    Transaction_IQ_DestroySub_Commit },
    { "Transaction_IQ_DestroySub_Rollback",  Transaction_IQ_DestroySub_Rollback },
    { "Transaction_IQ_DestroyDurSub_Commit",    Transaction_IQ_DestroyDurSub_Commit },
    { "Transaction_IQ_DestroyDurSub_Rollback",  Transaction_IQ_DestroyDurSub_Rollback },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Engine_CUnit_SQ_Transactions[] = {
    { "Transaction_SQ_ND_Commit",            Transaction_SQ_ND_Commit },
    { "Transaction_SQ_D_Commit",             Transaction_SQ_D_Commit },
    { "Transaction_SQ_ND_B7_Commit",         Transaction_SQ_ND_B7_Commit },
    { "Transaction_SQ_D_B7_Commit",          Transaction_SQ_D_B7_Commit },
    { "Transaction_SQ_ND_Rollback",          Transaction_SQ_ND_Rollback },
    { "Transaction_SQ_D_Rollback",           Transaction_SQ_D_Rollback },
    { "Transaction_SQ_ND_B7_Rollback",       Transaction_SQ_ND_B7_Rollback },
    { "Transaction_SQ_D_B7_Rollback",        Transaction_SQ_D_B7_Rollback },
    { "Transaction_SQ_AutoRollback",         Transaction_SQ_AutoRollback },
    { "Transaction_SQ_MultiBatch",           Transaction_SQ_MultiBatch },
    { "Transaction_SQ_DestroySub_Commit",    Transaction_SQ_DestroySub_Commit },
    { "Transaction_SQ_DestroySub_Rollback",  Transaction_SQ_DestroySub_Rollback },
    { "Transaction_SQ_DestroyDurSub_Commit",    Transaction_SQ_DestroyDurSub_Commit },
    { "Transaction_SQ_DestroyDurSub_Rollback",  Transaction_SQ_DestroyDurSub_Rollback },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Engine_CUnit_Transaction_Other[] = {
    { "Transaction_Savepoints",            Transaction_Savepoints },
    CU_TEST_INFO_NULL
};


int initTransactions(void)
{
    int32_t rc;

    rc = test_engineInit_DEFAULT;
    if (rc != OK) goto mod_exit;

    rc = ism_engine_createClientState("Transaction Test",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
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

int termTransactions(void)
{
    return test_engineTerm(true);
}

/*********************************************************************/
/* TestSuite definition                                              */
/*********************************************************************/
CU_SuiteInfo ISM_Engine_CUnit_Transaction_Suites[] = {
    IMA_TEST_SUITE("Transactions_InterQ", initTransactions, termTransactions, ISM_Engine_CUnit_IQ_Transactions),
    IMA_TEST_SUITE("Transactions_SimpQ", initTransactions, termTransactions, ISM_Engine_CUnit_SQ_Transactions),
    IMA_TEST_SUITE("Transaction_Other", initTransactions, termTransactions, ISM_Engine_CUnit_Transaction_Other),
    CU_SUITE_INFO_NULL,
};

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;

    // Level of output
    if (argc >= 2)
    {
        int32_t level=atoi(argv[1]);
        if ((level >=0) && (level <= 9))
        {
            verboseLevel = level;
        }
    }

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

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
