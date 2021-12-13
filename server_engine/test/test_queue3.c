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
/* Module Name: test_queue3.c                                        */
/*                                                                   */
/* Description: Test program which validates the recovery of queues  */
/*              and messages from the store.                         */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <getopt.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "transaction.h"
#include "queueNamespace.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_assert.h"
#include "test_utils_file.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
static ismEngine_ClientStateHandle_t hClient = NULL;

/*********************************************************************/
/* Name: tiqMsg_t                                                    */
/* Description: Structure used to hold testcase message. Normally    */
/*              used to build an array of messages for a given test. */
/*********************************************************************/
typedef struct
{
    uint32_t MsgId;
    uint32_t MsgLength;
    uint16_t CheckSum;
    ismEngine_Message_t *pMessage;
    bool Received;
    bool Ackd;
    bool Committed;
    uint8_t MsgFlags;
    uint32_t tranNumber;
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
                    , uint8_t MsgFlags
                    , uint64_t msgId
                    , uint64_t orderId
                    , uint32_t length
                    , ismEngine_Message_t **ppMsg
                    , uint16_t *pcheckSum)
{
    char *buf = NULL;
    int i;
    int32_t rc;
    uint32_t areaNo;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2];
    size_t areaLengths[2];
    void *areas[2];


    header.OrderId = orderId;
    header.Persistence = Persistence;
    header.Reliability = Reliability;
    header.Flags = MsgFlags;

    if (length)
    {
        buf=(char *)malloc(length);
        TEST_ASSERT( buf != NULL
                   , ("Failed to allocate buffer of %d bytes.",
                      sizeof(length)) );

        snprintf(buf, length, "Message:%ld ", msgId);

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

    areaNo = 0;
    areaTypes[areaNo] = ismMESSAGE_AREA_PROPERTIES;
    areaLengths[areaNo] = sizeof(msgId);
    areas[areaNo] = &msgId;
    areaNo++;
    areaTypes[areaNo] = ismMESSAGE_AREA_PAYLOAD;
    areaLengths[areaNo] = length;
    areas[areaNo] = buf;
    areaNo++;

    rc = ism_engine_createMessage( &header
                                 , areaNo
                                 , areaTypes
                                 , areaLengths
                                 , areas
                                 , ppMsg);
    TEST_ASSERT( (rc == OK)
               , ("Failed to put message. rc = %d", rc) );

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
                            , uint8_t MsgFlags
                            , uint32_t PutFlags
                            , uint32_t msgCount
                            , uint32_t minMsgSize
                            , uint32_t maxMsgSize
                            , uint32_t tranBatchSize
                            , uint32_t noCommitRatio
                            , uint32_t *pcommittedMsgCount
                            , tiqMsg_t **ppMsgs )
{
    uint64_t counter;
    int64_t OrderId;
    tiqMsg_t *pMsgArray;
    uint32_t variation;
    uint32_t tranNumber = 0;
    uint32_t committedMsgCount = 0;

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
    TEST_ASSERT( pMsgArray != 0
               , ("Failed to allocate buffer of %d bytes.",
                  msgCount * sizeof(tiqMsg_t)) );

    for (counter=0; counter < msgCount; counter++)
    {
        if ((tranBatchSize) &&
            ((counter % tranBatchSize) == 0))
        {
          // We need to start a new transaction
          tranNumber++;
        }
        pMsgArray[counter].tranNumber = tranNumber;

        if ((noCommitRatio != 0) &&
            ((tranNumber % noCommitRatio) == 0))
        {
            pMsgArray[counter].Committed = false;
        }
        else
        {
            pMsgArray[counter].Committed = true;
            committedMsgCount++;
        }

        pMsgArray[counter].MsgId = counter+1;
        pMsgArray[counter].MsgLength = minMsgSize;
        pMsgArray[counter].Received = false;
        pMsgArray[counter].Ackd = false;
        pMsgArray[counter].MsgFlags = MsgFlags;
        pMsgArray[counter].hDelivery =  ismENGINE_NULL_DELIVERY_HANDLE;
        if (variation)
        {
            pMsgArray[counter].MsgLength += (variation * counter) / msgCount;
        }

        if (PutFlags & ieqPutOptions_SET_ORDERID)
        {
            OrderId = (((counter+1) & 0x00000000000f) << 4) |
                      (((counter+1) & 0x0000000000f0) >> 4) |
                      (((counter+1) & 0x000000000f00) << 4) |
                      (((counter+1) & 0x00000000f000) >> 4) |
                      (((counter+1) & 0x0000000f0000) << 4) |
                      (((counter+1) & 0x000000f00000) >> 4) |
                      (((counter+1) & 0x00000f000000) << 4) |
                      (((counter+1) & 0x0000f0000000) >> 4) |
                      (((counter+1) & 0x000f00000000) << 4) |
                      (((counter+1) & 0x00f000000000) >> 4) |
                      (((counter+1) & 0x0f0000000000) << 4) |
                      (((counter+1) & 0xf00000000000) >> 4);
        }
        else
        {
            OrderId = 0;
        }

        allocMsg( hSession
                , Persistence
                , Reliability
                , MsgFlags
                , counter +1
                , OrderId
                , pMsgArray[counter].MsgLength
                , &pMsgArray[counter].pMessage
                , &pMsgArray[counter].CheckSum);
    }

    test_log(testLOGLEVEL_TESTPROGRESS, "Generated message array containing %d messages", msgCount);

    *ppMsgs = pMsgArray;
    *pcommittedMsgCount = committedMsgCount;

    return;
}

/*********************************************************************/
/* Name:        freeMsgArray                                         */
/* Description: Utility function used to free the array of msgs      */
/* Parameters:                                                       */
/*     hSession            - Session                                 */
/*     msgCount            - Number of messages in array             */
/*     pMsgs               - Array of messages                       */
/*********************************************************************/
static void freeMsgArray( ismEngine_SessionHandle_t hSession
                        , uint32_t msgCount
                        , tiqMsg_t *pMsgs
                        , bool releaseMsgs)
{
    if (releaseMsgs)
    {
        for(uint32_t i=0; i<msgCount; i++)
        {
            ism_engine_releaseMessage((ismEngine_MessageHandle_t)pMsgs[i].pMessage);
        }
    }

    free(pMsgs);
    return;
}

/*********************************************************************/
/* Name:        putMsgArray                                          */
/* Description: Utility function used to put the array of messages   */
/*              to the queue.                                        */
/* Parameters:                                                       */
/*     QueueName           - Queue name                              */
/*     msgCount            - Number of messages to put               */
/*     tranBatchSize       - Put msgs transactions in batches of 'N' */
/*     pMsgs               - Array of messages to put                */
/*********************************************************************/
static void putMsgArray( ismEngine_SessionHandle_t hSession
                       , char *QueueName
                       , uint32_t msgCount
                       , uint32_t tranBatchSize
                       , uint32_t noCommitRatio
                       , ieqPutOptions_t putFlags
                       , tiqMsg_t *pMsgs )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int counter;
    uint32_t tranNumber = 0;
    ismEngine_Transaction_t *pTransaction = NULL;
    ismQHandle_t hQueue =  ieqn_getQueueHandle(pThreadData, QueueName);

    TEST_ASSERT(hQueue != NULL, ("Failed to lookup handle for queue %s", QueueName));
 
    for (counter=0; counter < msgCount; counter++)
    {
        if (pMsgs[counter].tranNumber != tranNumber)
        {
            if ((counter != 0) &&
                (pMsgs[counter-1].tranNumber) &&
                (pMsgs[counter-1].Committed))
            {
                test_log(testLOGLEVEL_VERBOSE,
                        "Committing transaction T%d",
                        pMsgs[counter-1].Committed);

                rc = ietr_commit(pThreadData, pTransaction, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
                TEST_ASSERT(rc == 0,
                            ("Failed to commit transaction T%d - rc = %d",
                            pMsgs[counter-1].Committed,  rc));
            }

            tranNumber++;
            rc = ietr_createLocal( pThreadData, NULL, true, false, NULL, &pTransaction );
            TEST_ASSERT(rc == 0, ("Failed to create transaction T%d - rc = %d",
                        tranNumber, rc));

            test_log(5, "Created transaction T%d", tranNumber);
        }

        rc = ieq_put( pThreadData
                    , hQueue
                    , putFlags
                    , pTransaction
                    , pMsgs[counter].pMessage
                    , IEQ_MSGTYPE_REFCOUNT
                    , NULL );
        TEST_ASSERT(rc == 0, ("Failed to put message %d - rc = %d",
                    counter,  rc));

        test_log(testLOGLEVEL_VERBOSE, "Put message %d (tranNum=%d commit=%d)",
                counter, pMsgs[counter].tranNumber, pMsgs[counter].Committed);
    }

    if ((pMsgs[counter-1].tranNumber) &&
        (pMsgs[counter-1].Committed))
    {
        test_log(testLOGLEVEL_TESTPROGRESS,
                 "Committing transaction T%d", pMsgs[counter-1].Committed);

        rc = ietr_commit(pThreadData, pTransaction, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
        TEST_ASSERT(rc == 0,
                    ("Failed to commit transaction T%d - rc = %d",
                     pMsgs[counter-1].tranNumber,  rc));
    }

    // Alter testLOGLEVEL_ to get output at lower verbosity, alter detailLevel to get more detail
    test_log_queueHandle(testLOGLEVEL_VERBOSE, hQueue, 1, 0, "");

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
    bool transactional;
    uint32_t nackRatio;
    uint32_t nackLimit;
    uint32_t deliveryCount;
    volatile uint32_t msgsReceived;
    uint32_t invocationCount;
    bool enabled;
    uint64_t lastOrderId;
    tiqMsg_t *pMsgs;
} tiqConsumerContext_t;

/*********************************************************************/
/* Name:        verifyMessage                                        */
/* Description: verify a message                                     */
/*********************************************************************/
static void verifyMessage( uint32_t msgNum
                         , ismMessageHeader_t *pMsgDetails
                         , uint64_t msgId
                         , char *buffer
                         , uint32_t msgLen
                         , tiqMsg_t *pmsg )
{
    TEST_ASSERT(pmsg->Committed,
                ("Message %d should not have been committed. Transaction(%d)",
                 msgNum, pmsg->tranNumber));

    TEST_ASSERT(pmsg->MsgLength == msgLen, 
                ("Message %d length(%d) doesn't match expected length(%d)",
                 msgNum, msgLen, pmsg->MsgLength));

    // If retain was specified on input the output flags will not necessarily
    // match the input flags - other tests do verify that the correct flags
    // are set for retained messages.
    if (pmsg->MsgFlags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN)
    {
        TEST_ASSERT(pMsgDetails->Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                    ("Message %d MsgFlags(0x%08x) don't include expected bit (0x%08x)",
                     msgNum, pMsgDetails->Flags, ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN));
    }
    else
    {
        TEST_ASSERT(pMsgDetails->Flags == pmsg->MsgFlags,
                    ("Message %d MsgFlags(%d) don't match expected MsgFlags(%d)",
                     msgNum, pMsgDetails->Flags, pmsg->MsgFlags));
    }

    uint16_t checkSum = calcCheckSum(buffer, msgLen);
    TEST_ASSERT(checkSum == pmsg->CheckSum,
                ("Message %d length(%d) doesn't match expected lenth(%d)",
                 msgNum, checkSum, pmsg->CheckSum));

    test_log(testLOGLEVEL_VERBOSE, "Verified message %d - %s", msgNum, buffer);
}

typedef struct tag_inflightAckInfo_t {
    bool transactional;
    bool ack;
    bool commitAck;
    ismEngine_SessionHandle_t hSession;
    ismEngine_TransactionHandle_t hTran;
    uint32_t msgNum;
    volatile uint32_t *pMsgsReceived;
} inflightAckInfo_t;

static void finishCommit(int rc, void *handle, void *context)
{
	inflightAckInfo_t *ackInfo = (inflightAckInfo_t *)context;
    __sync_fetch_and_add(ackInfo->pMsgsReceived, 1);
}

static void finishAck(int rc, void *handle, void *context)
{
    inflightAckInfo_t *ackInfo = (inflightAckInfo_t *)context;

    if (rc != OK)
    {
        TEST_ASSERT( rc == OK,
                     ("Failed to confirm message delivery(%s). rc = %d",
                     ackInfo->ack?"ACK":"NACK", rc) );
    }
    if (ackInfo->transactional)
    {
        if (ackInfo->commitAck)
        {
            // Commit the transaction
            rc = ism_engine_commitTransaction( ackInfo->hSession
                                             , ackInfo->hTran
                                             , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT
                                             , ackInfo
                                             , sizeof(*ackInfo)
                                             , finishCommit );
            TEST_ASSERT( rc == OK || rc == ISMRC_AsyncCompletion,
                         ("Failed to commit transaction. rc = %d", rc));

            if (rc == OK)
            {
            	finishCommit(rc, NULL, ackInfo);
            }

        }
        else
        {
            test_log(testLOGLEVEL_VERBOSE,
                     "Not committing transaction for message %d",
                     ackInfo->msgNum);

            __sync_fetch_and_add(ackInfo->pMsgsReceived, 1);
        }
    }
    else
    {
        if (ackInfo->ack)
        {
            __sync_fetch_and_add(ackInfo->pMsgsReceived, 1);
        }
    }
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
    tiqConsumerContext_t *pContext = *(tiqConsumerContext_t **)pConsumerContext;
    uint32_t msgNum;
    bool ack = true;
    int32_t rc;
    bool WaiterEnabled = true;
    ismEngine_TransactionHandle_t hTran = NULL;

    test_log(testLOGLEVEL_VERBOSE, "Received message %s", pAreaData[1]);

    TEST_ASSERT( areaCount == 2, 
                 ("Received %d areas. Expected 2!", areaCount) );
    TEST_ASSERT(areaLengths[1] >= 32,
                ("Unsupported message length of %d", areaLengths[1]));


    // Check the message contents
    msgNum = -1;
    sscanf(pAreaData[1], "Message:%d", &msgNum);
    TEST_ASSERT(msgNum != -1,
                ("Unable to extract msgNum for Message %.32s", pAreaData[1]));

    ismEngine_Message_t *pMessage = (ismEngine_Message_t *)hMessage;

    test_log(testLOGLEVEL_VERBOSE, "received msg %d (%p) - useCount=%d storeUseCount=%d deliveryCount=%d orderId=%ld",
            msgNum, hMessage,
            pMessage->usageCount,
            pMessage->StoreMsg.parts.RefCount,
            pMsgDetails->RedeliveryCount,
            pMsgDetails->OrderId );

    // Having got the message number compare it to what we expect
    verifyMessage( msgNum
                 , pMsgDetails
                 , *(uint64_t *)pAreaData[0]
                 , pAreaData[1]
                 , areaLengths[1]
                 , &(pContext->pMsgs[msgNum-1]));

    pContext->invocationCount++;

    if (pContext->deliveryCount != 0)
    {
        TEST_ASSERT(pContext->deliveryCount == pMsgDetails->RedeliveryCount,
               ("Message %d deliveryCount(%d) doesn't match expected deliveryCount(%d)",
                msgNum, pMsgDetails->RedeliveryCount, pContext->deliveryCount));
    }
    
    if (pMsgDetails->RedeliveryCount == 0)
    {
        // Verify the message are retrieved in order
        TEST_ASSERT(pMsgDetails->OrderId > pContext->lastOrderId,
                    ("Current orderId %ld <= Last orderId %ld",
                     pMsgDetails->OrderId, pContext->lastOrderId));
        pContext->lastOrderId = pMsgDetails->OrderId;
    }

    if (state == ismMESSAGE_STATE_DELIVERED)
    {
        bool commitAck = false;

        if (pContext->transactional)
        {
            // Create a transaction, for the message - do it using sync commit for now (ugly!)
        	ieutThreadData_t *pThreadData = ieut_getThreadData();
            rc = ietr_createLocal(pThreadData, pContext->hSession, true, false, NULL, &hTran);
            TEST_ASSERT( rc == OK, 
                         ("Failed to create transaction. rc = %d", rc));

            if (pContext->invocationCount % pContext->nackRatio)
            {
                commitAck = true;
            }
            else
            {
                commitAck = false;

                test_log(testLOGLEVEL_VERBOSE,
                         "Deciding we won't be committing transaction for message %d (%p)",
                         msgNum, hMessage);
            }
        }
        else
        {
            if ((pContext->nackLimit) && 
                (pMsgDetails->RedeliveryCount >= pContext->nackLimit))
            {
                ack = false;
                pContext->enabled = false;
                WaiterEnabled = false;
            }
 
            if (pContext->nackRatio)
            {
                if ((pContext->invocationCount % pContext->nackRatio) == 0)
                {
                    ack = false;
                    test_log(testLOGLEVEL_VERBOSE, "Negatively ack'ing message");
                }
            }
        }

        inflightAckInfo_t ackInfo = { pContext->transactional
                                    , ack
                                    , commitAck
                                    , pContext->hSession
                                    , hTran
                                    , msgNum
                                    , &(pContext->msgsReceived)};

        rc = ism_engine_confirmMessageDelivery( pContext->hSession
                                              , hTran
                                              , hDelivery
                                              , ack ? ismENGINE_CONFIRM_OPTION_CONSUMED
                                                    : ismENGINE_CONFIRM_OPTION_NOT_RECEIVED
                                              , &ackInfo, sizeof(ackInfo)
                                              , finishAck);

        TEST_ASSERT( rc == OK || rc == ISMRC_AsyncCompletion,
                     ("Failed to confirm message delivery(%s). rc = %d",
                     ack?"ACK":"NACK", rc) );

        if (rc == OK)
        {
            finishAck(rc, NULL, &ackInfo);
        }


    }
    else
    {
        __sync_fetch_and_add(&(pContext->msgsReceived), 1);
    }

    ism_engine_releaseMessage(hMessage);

    return WaiterEnabled;
}
  

/*********************************************************************/
/* Name:        Queue3_VerifyMsgs                                    */
/* Description: Utility function used to get and verify the messages */
/*              from a queue.                                        */
/* Parameters:                                                       */
/*     messageCount        - The number of messages to put           */
/*     minMsgSize          - The size of the messages put will be    */
/*     maxMsgSize          - randomly picked between these 2 values  */
/*     Persistence         - Message persistance                     */
/*     MsgReliability      - Qualilty of Service for message 0-2     */
/*     tranBatchSize       - Put msgs transactions in batches of 'N' */
/*     noCommitRatio       - Don't commit 1 in 'N' transactions      */
/*     deliveryCount       - the delivery count of the msgs received */
/*********************************************************************/
static void Queue3_VerifyMsgs( ismQueueType_t type
		                     , uint32_t messageCount
                             , uint32_t minMsgSize
                             , uint32_t maxMsgSize
                             , uint8_t Persistence
                             , uint8_t MsgReliability
                             , uint8_t MsgFlags
                             , uint32_t transactionalGetRatio
                             , uint32_t tranBatchSize
                             , uint32_t noCommitRatio
                             , uint32_t deliveryCount )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismEngine_Consumer_t *pConsumer;
    ismEngine_QueueStatistics_t stats;
    tiqMsg_t *msgArray;
    tiqConsumerContext_t CallbackContext;
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    char QueueName[]="test-queue3";
    uint32_t committedMsgCount;

    // Alter testLOGLEVEL_ to get output at lower verbosity, alter detailLevel to get more detail
    test_log_queue(testLOGLEVEL_VERBOSE, QueueName, 1, 0, "");

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    // Generate array of messages to be put
    generateMsgArray( hSession
                    , Persistence
                    , MsgReliability
                    , MsgFlags
                    , 0
                    , messageCount
                    , minMsgSize
                    , maxMsgSize
                    , tranBatchSize
                    , noCommitRatio
                    , &committedMsgCount
                    , &msgArray );

    if (transactionalGetRatio)
    {
        committedMsgCount = messageCount / transactionalGetRatio;
    }

    CallbackContext.transactional = false;
    CallbackContext.nackRatio = 0;
    CallbackContext.nackLimit = 0;
    CallbackContext.deliveryCount = deliveryCount;
    CallbackContext.enabled = true;
    CallbackContext.pMsgs = msgArray;
    CallbackContext.msgsReceived = 0;
    CallbackContext.invocationCount = 0;
    CallbackContext.hSession = hSession;
    CallbackContext.lastOrderId = 0;

    // initialise a consumer
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , QueueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pCallbackContext
                                  , sizeof(pCallbackContext)
                                  , MessageCallback
                                  , NULL
                                  , test_getDefaultConsumerOptionsFromQType(type)
                                  , &pConsumer
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

    test_waitForMessages(&(CallbackContext.msgsReceived), NULL, committedMsgCount, 20);

    (void)ieqn_getQueueStats(pThreadData, QueueName, &stats);
    (void)ieq_setStats(ieqn_getQueueHandle(pThreadData, QueueName), NULL, ieqSetStats_RESET_ALL);
    
    test_log(testLOGLEVEL_CHECK, "Verified %d messages", CallbackContext.msgsReceived);

    // verify that the correct number of messages have been received
    TEST_ASSERT(stats.BufferedMsgs == 0, ("Queue not empty(%d)", stats.BufferedMsgs));

    TEST_ASSERT( CallbackContext.msgsReceived==committedMsgCount
               , ("Incorrect number of message received(%d). Expected(%d)",
                  CallbackContext.msgsReceived, committedMsgCount));

    freeMsgArray( hSession, messageCount, msgArray, true);
   
    // Cleaup session
    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );

    return;
}

/*********************************************************************/
/* Name:        Queue3_PutMsgs                                       */
/* Description: This test function defines a queue and then puts     */
/*              msgCount messages to the queue.                      */
/* Parameters:                                                       */
/*     type - Type of queue to use                                   */
/*     messageCount        - The number of messages to put           */
/*     minMsgSize          - The size of the messages put will be    */
/*     maxMsgSize          - randomly picked between these 2 values  */
/*     Persistence         - Message persistance                     */
/*     MsgReliability      - Qualilty of Service for message 0-2     */
/*     tranBatchSize       - Put msgs transactions in batches of 'N' */
/*     noCommitRatio       - Don't commit 1 in 'N' transactions      */
/*********************************************************************/
static void Queue3_PutMsgs( ismQueueType_t type
                          , ieqPutOptions_t putFlags
                          , uint32_t messageCount
                          , uint32_t minMsgSize
                          , uint32_t maxMsgSize
                          , uint8_t Persistence
                          , uint8_t MsgReliability
                          , uint8_t MsgFlags
                          , uint32_t tranBatchSize
                          , uint32_t noCommitRatio )
{ 
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    tiqMsg_t *msgArray;
    ismEngine_QueueStatistics_t stats;
    uint32_t committedMsgCount;
    char QueueName[]="test-queue3";
    char QueueTypeBuf[16];

    test_log(testLOGLEVEL_TESTDESC, "Define a %s queue and load with %d %s messages",
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)),
            messageCount,
            Persistence?"Persistent":"Non-Persistent");

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    // Generate array of messages to be put
    generateMsgArray( hSession
                    , Persistence
                    , MsgReliability
                    , MsgFlags
                    , putFlags
                    , messageCount
                    , minMsgSize
                    , maxMsgSize
                    , tranBatchSize
                    , noCommitRatio
                    , &committedMsgCount
                    , &msgArray );

   
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

    // Put array of messages
    putMsgArray(hSession, QueueName, messageCount, tranBatchSize, noCommitRatio, putFlags, msgArray);
    test_log(testLOGLEVEL_TESTPROGRESS, "Put %d messages", messageCount);

    // Check Q depth 
    (void)ieqn_getQueueStats(pThreadData, QueueName, &stats);
    TEST_ASSERT( (stats.BufferedMsgs == messageCount)
               , ("Available messages(%d) != messageCount(%d)",
                  stats.BufferedMsgs, messageCount) );
    TEST_ASSERT( (stats.BufferedMsgs == messageCount)
               , ("Total messages(%d) != messageCount(%d)",
                  stats.BufferedMsgs, messageCount) );
    (void)ieq_setStats(ieqn_getQueueHandle(pThreadData, QueueName), NULL, ieqSetStats_RESET_ALL);

    if (rc == 0)
    {
        test_log(testLOGLEVEL_CHECK, "Checked %d messages exist on queue", messageCount);
    }

    // Cleaup session
    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );

    return;
}

/*********************************************************************/
/* Name:        Queue3_PutGetMsgs                                    */
/* Description: This test function defines a queue and then puts     */
/*              msgCount messages to the queue and then get's them.  */
/*              During this process the number of messages put/got   */
/*              are checked as well as the message contents.         */
/* Parameters:                                                       */
/*     type - Type of queue to use simple / intermediate             */
/*     messageCount        - The number of messages to put           */
/*     minMsgSize          - The size of the messages put will be    */
/*     maxMsgSize          - randomly picked between these 2 values  */
/*     Persistence         - Message persistance                     */
/*     MsgReliability      - Qualilty of Service for message 0-2     */
/*     nackRatio           - negative acknowledge 1 in 'N' messages  */
/*     nackLimit           - Stop after 'N' nacks                    */
/*********************************************************************/
static void Queue3_PutGetMsgs( ismQueueType_t type
                             , ieqPutOptions_t putFlags
                             , uint32_t messageCount
                             , uint32_t minMsgSize
                             , uint32_t maxMsgSize
                             , uint8_t Persistence
                             , uint8_t MsgReliability
                             , uint8_t MsgFlags
                             , bool transactional
                             , uint32_t nackRatio
                             , uint32_t nackLimit )
{ 
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    int32_t retry = 0;
    ismEngine_SessionHandle_t hSession;
    tiqMsg_t *msgArray;
    ismEngine_QueueStatistics_t stats;
    uint32_t committedMsgCount;
    ismEngine_Consumer_t *pConsumer;
    tiqConsumerContext_t CallbackContext;
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    char QueueName[]="test-queue3";
    char QueueTypeBuf[16];

    test_log(testLOGLEVEL_TESTDESC,
            "Define a %s queue and load with %d %s messages",
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)),
            messageCount,
            Persistence?"Persistent":"Non-Persistent");

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , (transactional) ?  ismENGINE_CREATE_SESSION_TRANSACTIONAL:
                                                    ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT(rc == OK, ("Failed to create session, rc = %d", rc));

    // Generate array of messages to be put
    generateMsgArray( hSession
                    , Persistence
                    , MsgReliability
                    , MsgFlags
                    , putFlags
                    , messageCount
                    , minMsgSize
                    , maxMsgSize
                    , 1
                    , 0
                    , &committedMsgCount
                    , &msgArray );

   
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
    
    // Put array of messages
    putMsgArray(hSession, QueueName, messageCount, 1, 0, putFlags, msgArray);
    test_log(testLOGLEVEL_TESTPROGRESS, "Put %d messages", messageCount);

    // Check Q depth 
    (void)ieqn_getQueueStats(pThreadData, QueueName, &stats);
    TEST_ASSERT( (stats.BufferedMsgs == messageCount)
               , ("Available messages(%d) != messageCount(%d)",
                  stats.BufferedMsgs, messageCount) );
    TEST_ASSERT( (stats.BufferedMsgs == messageCount)
               , ("Total messages(%d) != messageCount(%d)",
                  stats.BufferedMsgs, messageCount) );
    (void)ieq_setStats(ieqn_getQueueHandle(pThreadData, QueueName), NULL, ieqSetStats_RESET_ALL);

    if (rc == 0)
    {
        test_log(testLOGLEVEL_CHECK, "Checked %d messages exist on queue", messageCount);
    }

    // Now setup the consumer 
    CallbackContext.transactional = transactional;
    CallbackContext.nackRatio = nackRatio;
    CallbackContext.nackLimit = nackLimit;
    CallbackContext.deliveryCount = 0;
    CallbackContext.enabled = true;
    CallbackContext.pMsgs = msgArray;
    CallbackContext.msgsReceived = 0;
    CallbackContext.invocationCount = 0;
    CallbackContext.hSession = hSession;
    CallbackContext.lastOrderId = 0;

    // initialise a consumer
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , QueueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pCallbackContext
                                  , sizeof(pCallbackContext)
                                  , MessageCallback
                                  , NULL
                                  , test_getDefaultConsumerOptionsFromQType(type)
                                  , &pConsumer
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

    test_log(4, "Waiting for %d of %d messages to be delivered",
            committedMsgCount, messageCount);
    // wait for the consumer to be signalled that there are no more messages
    while ((CallbackContext.msgsReceived < committedMsgCount) &&
             (CallbackContext.enabled))
    {
        uint32_t lastmsgsReceived = CallbackContext.msgsReceived;
        retry = 0;
        while(  (CallbackContext.msgsReceived == lastmsgsReceived)
              &&(CallbackContext.msgsReceived < committedMsgCount)
              &&(CallbackContext.enabled))
        {
            usleep(200*1000); //200ms = 0.2s
            retry++;
            if (retry > 5*10) //10s without a message...
            {
                test_log(1, "No msgs received for 10s.... aborting.");
                abort();
            }
        }
    }
    ieqn_getQueueStats(pThreadData, QueueName, &stats);
    (void)ieq_setStats(ieqn_getQueueHandle(pThreadData, QueueName), NULL, ieqSetStats_RESET_ALL);

    test_log(testLOGLEVEL_CHECK, "Received %d messages", CallbackContext.msgsReceived);
    test_log(testLOGLEVEL_CHECK, "Invoked %d times", CallbackContext.invocationCount);
    test_log(testLOGLEVEL_CHECK, "Queue contains %d messages", stats.BufferedMsgs);

    freeMsgArray( hSession, messageCount, msgArray, false);

    // Cleaup session
    rc = ism_engine_destroySession( hSession
                                  , NULL
                                  , 0
                                  , NULL );

    return;
}

int32_t parseArgs( int argc
                 , char *argv[]
                 , uint32_t *ptestNo
                 , uint32_t *pphase
                 , bool *prestart
                 , bool *pruninline
                 , bool *prunall
                 , testLogLevel_t *pverboseLevel
                 , int *ptrclvl
                 , char **padminDir )
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "testCase", 1, NULL, 't' },
        { "recover", 0, NULL, 'r' },
        { "noRestart", 0, NULL, 'n' },
        { "inline", 0, NULL, 'i' },
        { "verbose", 1, NULL, 'v' },
        { "adminDir", 1, NULL, 'a' },
        { NULL, 1, NULL, 0 } };
    int long_index;

    *ptestNo = 1; 
    *pverboseLevel = 3;
    *pphase = 1;
    *prestart = true;
    *pruninline = false;
    *ptrclvl = 0;
    *prunall = (argc == 1)?true:false;
    *padminDir = NULL;

    while ((opt = getopt_long(argc, argv, ":rint:c:v:a:0123456789", long_options, &long_index)) != -1)
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
            case 'r':
                *pphase = 2;
                break;
            case 'c':
               *ptestNo = atoi(optarg);
               *prunall = true;
                break;
            case 't':
               *ptestNo = atoi(optarg);
               break;
            case 'n':
               *prestart = false;
               break;
            case 'i':
               *pruninline = true;
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
    
    if (*pverboseLevel >= testLOGLEVEL_VERBOSE) *pverboseLevel=testLOGLEVEL_VERBOSE;

    if (*ptestNo == 0) usage = 1;

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-r] [-n] [-i] [-v 0-9] [-0...-9] -t [1-4]\n", argv[0]);
        fprintf(stderr, "       -r (--recover)   - Run only recovery phase\n");               
        fprintf(stderr, "       -n (--noRestart) - Run only create phase\n");
        fprintf(stderr, "       -i (--inline)    - Perform recovery without restart\n");
        fprintf(stderr, "       -t (--testCase)  - Test Case number\n");
        fprintf(stderr, "          1 - phase-1   - Create-q and load with 100 msgs\n");
        fprintf(stderr, "            - phase-2   - verify that 100 messages exist on queue\n");
        fprintf(stderr, "          2 - phase-1   - Load queue with 100 msgs. Fail to commit 20\n");
        fprintf(stderr, "            - phase-2   - verify that 80 messages exist on queue\n");
        fprintf(stderr, "          3 - phase-1   - Load queue with 1 msg with retain flag set\n");
        fprintf(stderr, "            - phase-2   - Verify that message has retain flag set\n");
        fprintf(stderr, "          4 - phase-1   - Load queue with 1 msg and N-Ack it 7 times\n");
        fprintf(stderr, "            - phase-2   - Verify that message delivery count is 7\n");
        fprintf(stderr, "       -v (--verbose)   - Test Case verbosity\n");
        fprintf(stderr, "       -a (--adminDir)  - Admin directory to use\n");
        fprintf(stderr, "       -0 .. -9         - ISM Trace level\n");
    }

   return usage;
}


/*********************************************************************/
/* NAME:        test_queue3                                          */
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
/* USAGE:       test_queue3 [-r] -t <testno>                         */
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
    char *newargv[20];
    uint32_t testNo = 1;
    uint32_t phase = 1;
    bool restart = true;
    bool runinline = false;
    bool runall = true;
    char testNumber[10];
    char *adminDir = NULL;
    
    if (argc != 1)
    {
        /*************************************************************/
        /* Parse arguments                                           */
        /*************************************************************/
        rc = parseArgs( argc
                      , argv
                      , &testNo
                      , &phase
                      , &restart
                      , &runinline
                      , &runall
                      , &testLogLevel
                      , &trclvl
                      , &adminDir );
        if (rc != 0)
        {
            goto mod_exit;
        }
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

    if (phase == 1)
    {
        /*************************************************************/
        /* Prepare the restart command                               */
        /*************************************************************/
        TEST_ASSERT(argc < 8, ("argc was %d", argc));

        newargv[0]=argv[0];
        newargv[1]="-a";
        newargv[2]=adminDir;
        if (runall)
        {
            sprintf(testNumber, "%d", testNo);
            newargv[3]="-c";
            newargv[4]=testNumber;
            newargv[5]="-r";
            newargv[6]=NULL;
        }
        else
        {
          sprintf(testNumber, "%d", testNo);
          newargv[3]="-t";
          newargv[4]=testNumber;
          newargv[5]="-r";
          newargv[6]=NULL;
        }

        /*************************************************************/
        /* Start up the Engine - Cold Start                          */
        /*************************************************************/
        test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - cold start");
        rc = test_engineInit_DEFAULT;
        if (rc != 0)
        {
            goto mod_exit;
        }
        test_log(testLOGLEVEL_TESTPROGRESS, "Engine cold started");

        rc = ism_engine_createClientState("Queue3 Test",
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL, NULL, NULL,
                                          &hClient,
                                          NULL,
                                          0,
                                          NULL);
        TEST_ASSERT(rc == OK, ("Failed to create client state. rc = %d", rc));

        test_log(testLOGLEVEL_TESTDESC, "Running test %d phase cold-start", testNo);

 
        /*************************************************************/
        /* And perform the setup for te required testcase            */
        /*************************************************************/
        switch (testNo)
        { 
            case 1:
                    Queue3_PutMsgs( intermediate
                                  , ieqPutOptions_NONE
                                  | ieqPutOptions_SET_ORDERID
                                  , 2000
                                  , 32
                                  , 102400
                                  , ismMESSAGE_PERSISTENCE_PERSISTENT
                                  , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                  , ismMESSAGE_FLAGS_NONE
                                  , 10 
                                  , 0 );
                    break;

            case 2:
                    Queue3_PutMsgs( intermediate
                                  , ieqPutOptions_NONE
                                  | ieqPutOptions_SET_ORDERID
                                  , 2000
                                  , 32
                                  , 102400
                                  , ismMESSAGE_PERSISTENCE_PERSISTENT
                                  , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                  , ismMESSAGE_FLAGS_NONE
                                  , 10
                                  , 4 );
                    break;

            case 3:
                    Queue3_PutMsgs( intermediate
                                  , ieqPutOptions_RETAINED
                                  | ieqPutOptions_SET_ORDERID
                                  , 1
                                  , 156
                                  , 156
                                  , ismMESSAGE_PERSISTENCE_PERSISTENT
                                  , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                  , ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN
                                  , 10
                                  , 4 );
                    break;

            case 4:
                    Queue3_PutGetMsgs( intermediate
                                     , ieqPutOptions_NONE
                                     | ieqPutOptions_SET_ORDERID
                                     , 1
                                     , 156
                                     , 156
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_NONE
                                     , false
                                     , 1    // reject Ratio
                                     , 5 ); // reject Count
                    break;

            case 5:
                    Queue3_PutMsgs( multiConsumer
                                  , ieqPutOptions_NONE
                                  | ieqPutOptions_SET_ORDERID
                                  , 2000
                                  , 32
                                  , 102400
                                  , ismMESSAGE_PERSISTENCE_PERSISTENT
                                  , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                  , ismMESSAGE_FLAGS_NONE
                                  , 10 
                                  , 0 );
                    break;

            case 6:
                    Queue3_PutMsgs( multiConsumer
                                  , ieqPutOptions_NONE
                                  | ieqPutOptions_SET_ORDERID
                                  , 2000
                                  , 32
                                  , 102400
                                  , ismMESSAGE_PERSISTENCE_PERSISTENT
                                  , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                  , ismMESSAGE_FLAGS_NONE
                                  , 10
                                  , 4 );
                    break;

            case 7:
                    Queue3_PutMsgs( multiConsumer
                                  , ieqPutOptions_RETAINED
                                  | ieqPutOptions_SET_ORDERID
                                  , 1
                                  , 156
                                  , 156
                                  , ismMESSAGE_PERSISTENCE_PERSISTENT
                                  , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                  , ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN
                                  , 10
                                  , 4 );
                    break;

            case 8:
                    // Note we cannot use ieqPutOptions_SET_ORDERID on
                    // multi consumer queue if we are going to get any
                    // messages.
                    Queue3_PutGetMsgs( multiConsumer
                                     , ieqPutOptions_NONE
                                     , 1
                                     , 156
                                     , 156
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_NONE
                                     , false
                                     , 1    // reject Ratio
                                     , 5 ); // reject Count
                    break;

            case 9:
                    Queue3_PutGetMsgs( multiConsumer
                                     , ieqPutOptions_NONE
                                     , 1000
                                     , 156
                                     , 156
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_NONE
                                     , true
                                     , 10   // ratio of transactions to not commit
                                     , 0 ); // backout ratio
                    break;

            default:
                    TEST_ASSERT(0, ("unexpected testno: %d", testNo));
                    break;
        }

        /*************************************************************/
        /* Having initialised the queue, now re-exec ourself in      */
        /* restart mode.                                             */
        /*************************************************************/
        if (restart)
        {
            if (runinline)
            {
                test_log(2, "Ignoring restart, entering phase 2...");
                phase = 2;

                rc = ism_engine_destroyClientState(hClient,
                                                   ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                                   NULL,
                                                   0,
                                                   NULL);
                TEST_ASSERT(rc == OK, ("Destroy Client failed. rc = %d", rc));
            }
            else
            {
                test_log(2, "Restarting...");
                rc = test_execv(newargv[0], newargv);
                TEST_ASSERT(false, ("'test_execv' failed. rc = %d", rc));
            }
        }
    }

    if (phase == 2)
    {
        /*************************************************************/
        /* Start up the Engine - Warm Start enabling auto queue      */
        /* creation to avoid reconciliation deleting our queues.    */
        /*************************************************************/
        if (!runinline)
        {
            test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start");
            rc = test_engineInit(false,  true,
                                 false, // Enable auto queue creation
                                 false, /*recovery should complete ok*/
                                 ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                                 testDEFAULT_STORE_SIZE);
            TEST_ASSERT(rc == OK, ("'test_engineInit' failed. rc = %d", rc));
            test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");
        }

        test_log(testLOGLEVEL_TESTDESC, "Running test %d phase warm-start", testNo);

        rc = ism_engine_createClientState("Queue3 Test",
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL, NULL, NULL,
                                          &hClient,
                                          NULL,
                                          0,
                                          NULL);
        TEST_ASSERT(rc == OK, ("Failed to create client state. rc = %d", rc));

        switch (testNo)
        { 
            case 1: 
                    Queue3_VerifyMsgs( intermediate
                    		         , 2000
                                     , 32
                                     , 102400
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_NONE
                                     , 0
                                     , 10 
                                     , 0 
                                     , 0); 

                    break;

            case 2: 
                    Queue3_VerifyMsgs( intermediate
                    		         , 2000
                                     , 32
                                     , 102400
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_NONE
                                     , 0
                                     , 10
                                     , 4
                                     , 0 ); 

                    break;

            case 3: 
                    Queue3_VerifyMsgs( intermediate
                    		         , 1
                                     , 156
                                     , 156
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN
                                     , 0
                                     , 10
                                     , 4 
                                     , 0 ); 

                    break;

            case 4: 
                    Queue3_VerifyMsgs( intermediate
                    		         , 1
                                     , 156
                                     , 156
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_NONE
                                     , 0
                                     , 10
                                     , 4 
                                     , 5 +1 ); 
                    break;

            case 5: 
                    Queue3_VerifyMsgs( multiConsumer
                    		         , 2000
                                     , 32
                                     , 102400
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_NONE
                                     , 0
                                     , 10 
                                     , 0 
                                     , 0); 

                    break;

            case 6: 
                    Queue3_VerifyMsgs( multiConsumer
           		                     , 2000
                                     , 32
                                     , 102400
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_NONE
                                     , 0
                                     , 10
                                     , 4
                                     , 0 ); 

                    break;

            case 7: 
                    Queue3_VerifyMsgs( multiConsumer
  		                             , 1
                                     , 156
                                     , 156
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN
                                     , 0
                                     , 10
                                     , 4 
                                     , 0 ); 

                    break;

            case 8: 
                    Queue3_VerifyMsgs( multiConsumer
	                                 , 1
                                     , 156
                                     , 156
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_NONE
                                     , 0
                                     , 10
                                     , 4 
                                     , 5 +1 ); 
                    break;

            case 9: 
                    Queue3_VerifyMsgs( multiConsumer
                                     , 1000 // Expect only to see 100 msgs
                                     , 156
                                     , 156
                                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                                     , ismMESSAGE_FLAGS_NONE
                                     , 10
                                     , 1
                                     , 0
                                     , 0 ); 

                    runall = false; // No more tests
                    break;

            default:
                    break;
        }

        test_engineTerm(true);
        test_processTerm(false);

        if (runall)
        {
            newargv[0]=argv[0];
            newargv[1]="-a";
            newargv[2]=adminDir;
            newargv[3]="-c";
            sprintf(testNumber, "%d", testNo+1);
            newargv[4]=testNumber;
            newargv[5]=NULL;

            test_log(2, "Starting test %d...", testNo+1);
            rc = test_execv(newargv[0], newargv);
            TEST_ASSERT(false, ("'test_execv' failed. rc = %d", rc));
        }
        else
        {
            test_removeAdminDir(adminDir);
        }
    }

mod_exit:

    return rc?1:0;
}
