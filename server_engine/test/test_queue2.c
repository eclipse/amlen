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
/* Module Name: test_queue2.c                                        */
/*                                                                   */
/* Description: CUnit tests of Engine Intermedaiate Q functions      */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "transaction.h"
#include "policyInfo.h"
#include "queueNamespace.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_task.h"

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
    uint32_t MsgLength;
    uint16_t CheckSum;
    ismEngine_Message_t *pMessage;
    bool Received;
    bool Ackd;
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
        buf=(char *)malloc(length + 4);  // + 4 saves warning from valgrind 
                                         // when executing strlen 
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
        pMsgArray[counter].MsgLength = minMsgSize;
        pMsgArray[counter].Received = false;
        pMsgArray[counter].Ackd = false;
        pMsgArray[counter].hDelivery = ismENGINE_NULL_DELIVERY_HANDLE;
        if (variation)
        {
            pMsgArray[counter].MsgLength += random() % variation;
        }
   
        allocMsg( hSession
                , Persistence
                , Reliability
                , counter
                , pMsgArray[counter].MsgLength
                , &pMsgArray[counter].pMessage
                , &pMsgArray[counter].CheckSum);
    }

    verbose(4, "Generated message array containing %d messages", msgCount);

    *ppMsgs = pMsgArray;

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
                        , tiqMsg_t *pMsgs )
{
    free(pMsgs);
    return;
}

/*********************************************************************/
/* Name:        putMsgArray                                          */
/* Description: Utility function used to put the array of messages   */
/*              to the queue.                                        */
/* Parameters:                                                       */
/*     hQueue              - Queue                                   */
/*     pTran               - Transaction (if transactional)          */
/*     msgCount            - Number of messages to put               */
/*     pMsgs               - Array of messages to put                */
/*********************************************************************/
static void putMsgArray( ismQHandle_t hQueue
                       , ismEngine_Transaction_t *pTran
                       , uint32_t msgCount
                       , tiqMsg_t *pMsgs )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int counter;
    int32_t rc;

    for (counter=0; counter < msgCount; counter++)
    {
        pMsgs[counter].Received = false;
        pMsgs[counter].Ackd = false;
        pMsgs[counter].hDelivery = ismENGINE_NULL_DELIVERY_HANDLE;

        rc = ieq_put( pThreadData
                    , hQueue
                    , ieqPutOptions_NONE
                    , pTran
                    , pMsgs[counter].pMessage
                    , IEQ_MSGTYPE_REFCOUNT 
                    , NULL );
        TEST_ASSERT_EQUAL(rc, OK);

        verbose(5, "Put message %d", counter);
    }

    return;
}

/*********************************************************************/
/* Name:        tiqConsumerContext_t                                 */
/* Description: Context informaton used to pass information to       */
/*              MessageCallback function.                            */
/*********************************************************************/
typedef enum
{
    Auto,
    Manual,
    Record,
    Never
} AckMode_t;

typedef struct
{
    pthread_mutex_t lock;
    uint32_t DeliveryHdlSize;
    uint32_t DeliveryHdlCount;
    ismEngine_DeliveryHandle_t *pDeliveryHdls;
} tiqAckList;

typedef struct
{
    ismEngine_SessionHandle_t hSession;
    AckMode_t AckMode;
    uint8_t negAckRatio;
    uint8_t Reliability;
    volatile uint32_t msgsReceived;
    volatile uint32_t msgsAckStarted;
    volatile uint32_t msgsAckCompleted;
    volatile uint32_t msgsNegAckd;
    uint32_t expectedDupCount;
    uint32_t dupMsgs;
    tiqMsg_t *pMsgs;
    ismQueueType_t Qtype;
    tiqAckList *pAckList;
} tiqConsumerContext_t;

typedef struct
{
    ismEngine_SessionHandle_t hSession;
    ismEngine_DeliveryHandle_t  hDelivery;
    volatile uint32_t *pMsgsAcked;
} tiqInflightAckInfo_t;



static void completedAckIncrement(int rc, void *handle, void *context)
{
    if (context != NULL)
    {
        volatile uint32_t *pMsgsAcked = *(volatile uint32_t **)context;

        __sync_fetch_and_add(pMsgsAcked, 1);
    }
}

static void completedReceiveAck(int rc, void *handle, void *context)
{
    TEST_ASSERT_EQUAL(rc, OK);
    tiqInflightAckInfo_t *ackInfo = (tiqInflightAckInfo_t *)context;

    rc= ism_engine_confirmMessageDelivery( ackInfo->hSession
                                        , NULL
                                        , ackInfo->hDelivery
                                        , ismENGINE_CONFIRM_OPTION_CONSUMED
                                        , &(ackInfo->pMsgsAcked)
                                        , sizeof(ackInfo->pMsgsAcked)
                                        , completedAckIncrement);
    TEST_ASSERT_CUNIT((rc == OK || rc == ISMRC_AsyncCompletion),
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

    verbose(5, "Received message %s", pAreaData[0]);
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
            TEST_ASSERT((pAreaData[0] != NULL && strlen(pAreaData[0]) < 32), ("Unexpected message content"));
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
                         (pContext->Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)), ("Unexpected Reliabilty"));

            pContext->msgsReceived++;

            ism_engine_releaseMessage(hMessage);
            break;

        case ismMESSAGE_STATE_DELIVERED:
            TEST_ASSERT_NOT_EQUAL(msgNum, -1);

            // When message is delivered as DELIVERED check that both the
            // msg reliability and the consumer reliability is non-zero.
            TEST_ASSERT_NOT_EQUAL(pMsgDetails->Reliability, ismMESSAGE_RELIABILITY_AT_MOST_ONCE);

            // Check the delivery Id is zero (only if negAckRatio is zero)
//            if (pContext->negAckRatio == 0)
//            {
//                TEST_ASSERT_EQUAL(deliveryId, 0);
//            }

            TEST_ASSERT_NOT_EQUAL(hDelivery, ismENGINE_NULL_DELIVERY_HANDLE);
            pContext->pMsgs[msgNum].hDelivery = hDelivery;

//            if (pContext->Qtype != multiConsumer)
//            {
//                verbose(5, "Setting delivery id to %d", msgNum);
//                rc = ism_engine_setMessageDeliveryId( hConsumer
//                                                    , hDelivery
//                                                    , msgNum
//                                                    , NULL
//                                                    , 0
//                                                    , NULL );
//                TEST_ASSERT_EQUAL(rc, OK)
//            }

            if (pMsgDetails->RedeliveryCount)
            {
                verbose(5, "Message number %d received as a duplicate", msgNum);
                pContext->dupMsgs++;

                verbose(5, "DupCount is %d ExpectedDupCount is %d", pContext->dupMsgs, pContext->expectedDupCount);
            }

            if (pContext->AckMode == Auto)
            {
                if ((pContext->negAckRatio) && ((random() % pContext->negAckRatio) == 0))
                {
                    uint32_t confirmOption;

                    if (random() % 2)
                    {
                        confirmOption = ismENGINE_CONFIRM_OPTION_NOT_DELIVERED;

                        verbose(5, "Negatively acknowledging undelivered msg with delivery id to %d", msgNum);

                        // If this messase was already a duplicate it will still be
                        // marked as a duplicate when it is redelivered
                        if (pMsgDetails->RedeliveryCount)
                        {
                            pContext->expectedDupCount++;
                            pContext->msgsReceived++;
                        }
                    }
                    else
                    {
                        confirmOption = ismENGINE_CONFIRM_OPTION_NOT_RECEIVED;
                        pContext->expectedDupCount++;
                        pContext->msgsReceived++;

                        verbose(5, "Negatively acknowledging unreceived msg with delivery id to %d", msgNum);
                    }

                    pContext->pMsgs[msgNum].Received = false;
                    pContext->msgsNegAckd++;

                    rc = ism_engine_confirmMessageDelivery( pContext->hSession
                                                          , NULL
                                                          , hDelivery
                                                          , confirmOption
                                                          , NULL
                                                          , 0
                                                          , NULL );
                    TEST_ASSERT_EQUAL(rc, OK)
                }
                else
                {
                    pContext->msgsAckStarted++;

                    // If the consumer reliability is QoS 2,
                    if (   (pContext->Reliability==ismMESSAGE_RELIABILITY_EXACTLY_ONCE)
                        && (pContext->Qtype != multiConsumer))
                    {
                        tiqInflightAckInfo_t ackInfo = {pContext->hSession, hDelivery, &(pContext->msgsAckCompleted)};

                        rc = ism_engine_confirmMessageDelivery( pContext->hSession
                                                              , NULL
                                                              , hDelivery
                                                              , ismENGINE_CONFIRM_OPTION_RECEIVED
                                                              , &ackInfo
                                                              , sizeof(ackInfo)
                                                              , completedReceiveAck );
                        TEST_ASSERT_CUNIT((rc == OK || rc == ISMRC_AsyncCompletion),
                                          ("rc was %d\n", rc));

                        if (rc == OK)
                        {
                            completedReceiveAck(rc, NULL, &ackInfo);
                        }
                    }
                    else
                    {
                        volatile uint32_t *pMsgsAckd = &(pContext->msgsAckCompleted);

                        rc= ism_engine_confirmMessageDelivery( pContext->hSession
                                                            , NULL
                                                            , hDelivery
                                                            , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                            , &pMsgsAckd, sizeof(pMsgsAckd)
                                                            , completedAckIncrement);
                        TEST_ASSERT_CUNIT((rc == OK || rc == ISMRC_AsyncCompletion),
                                ("rc was %d\n", rc));

                        if (rc == OK)
                        {
                            completedAckIncrement(rc, NULL, &pMsgsAckd);
                        }
                    }
                    pContext->msgsReceived++;
                }
            }
            else if (pContext->AckMode == Record)
            {
                tiqAckList *pAckList = pContext->pAckList;
                TEST_ASSERT_NOT_EQUAL(pAckList, NULL);

                pthread_mutex_lock(&(pAckList->lock));
                if (pAckList->DeliveryHdlCount == pAckList->DeliveryHdlSize)
                {
                    // Need to increase the size of the array
                    pAckList->DeliveryHdlSize += 200;
                    pAckList->pDeliveryHdls=realloc(pAckList->pDeliveryHdls, sizeof(ismEngine_DeliveryHandle_t) * (pAckList->DeliveryHdlSize));
                    TEST_ASSERT(pAckList->pDeliveryHdls != NULL, ("Failed to alloc memory for %d delivery handles", pAckList->DeliveryHdlSize));
                }
                pAckList->pDeliveryHdls[pAckList->DeliveryHdlCount++] = hDelivery;
                pthread_mutex_unlock(&(pAckList->lock));
                pContext->msgsReceived++;
            }
            else
            {
                pContext->msgsReceived++;
            }
            ism_engine_releaseMessage(hMessage);

            break;
        default:
            TEST_ASSERT(false, ("Invalid state %d", state));
            break;
    }

    return true;
}
  
/*********************************************************************/
/* Name:        MessageCallback2                                     */
/* Description: This message callback routine does no validation on  */
/*              the messages with which it is invoked but simply    */
/*              keeps a counter on the messages received.           */
/*********************************************************************/
static bool MessageCallback2(ismEngine_ConsumerHandle_t  hConsumer,
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
    uint32_t msgNum  = 0;

    verbose(5, "Received message %s", pAreaData[0]);
    TEST_ASSERT_EQUAL(areaCount, 1);

    switch(state)
    {
        case ismMESSAGE_STATE_CONSUMED:
            // When message is delivered as CONSUMED check that either
            // the consumer reliability or the msg reliability is zero
            TEST_ASSERT(((pMsgDetails->Reliability==ismMESSAGE_RELIABILITY_AT_MOST_ONCE) ||
                         (pContext->Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)), ("Unexpected Reliabilty"));

            pContext->msgsReceived++;

            ism_engine_releaseMessage(hMessage);
            break;

        case ismMESSAGE_STATE_DELIVERED:
            // When message is delivered as DELIVERED check that both the
            // msg reliability and the consumer reliability is non-zero.
            TEST_ASSERT_NOT_EQUAL(pMsgDetails->Reliability, ismMESSAGE_RELIABILITY_AT_MOST_ONCE);
            TEST_ASSERT_NOT_EQUAL(pContext->Reliability, ismMESSAGE_RELIABILITY_AT_MOST_ONCE);

//            TEST_ASSERT_EQUAL(deliveryId, 0);

            TEST_ASSERT_NOT_EQUAL(hDelivery, ismENGINE_NULL_DELIVERY_HANDLE);

            msgNum = ++pContext->msgsReceived;
            verbose(5, "Setting delivery id to %d", msgNum);

//            if(pContext->Qtype != multiConsumer)
//            {
//                rc = ism_engine_setMessageDeliveryId( hConsumer
//                                                    , hDelivery
//                                                    , msgNum
//                                                    , NULL
//                                                    , 0
//                                                    , NULL );
//                TEST_ASSERT_EQUAL(rc, OK);
//            }

            // If the consumer reliability is QoS 2,
            if (    (pContext->Reliability==ismMESSAGE_RELIABILITY_EXACTLY_ONCE)
                 && (pContext->Qtype != multiConsumer))
            {
                tiqInflightAckInfo_t ackInfo = {pContext->hSession, hDelivery, &(pContext->msgsAckCompleted)};
                pContext->msgsAckStarted++;
                rc = ism_engine_confirmMessageDelivery( pContext->hSession
                                                      , NULL
                                                      , hDelivery
                                                      , ismENGINE_CONFIRM_OPTION_RECEIVED
                                                      , NULL
                                                      , 0
                                                      , NULL );
                TEST_ASSERT_CUNIT((rc == OK || rc == ISMRC_AsyncCompletion),
                                  ("rc was %d\n", rc));

                if (rc == OK)
                {
                    completedReceiveAck(rc, NULL, &ackInfo);
                }
            }
            else
            {
                volatile uint32_t *pMsgsAckCompleted = &(pContext->msgsAckCompleted);
                pContext->msgsAckStarted++;
                rc = ism_engine_confirmMessageDelivery( pContext->hSession
                                                      , NULL
                                                      , hDelivery
                                                      , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                      , &pMsgsAckCompleted, sizeof(pMsgsAckCompleted)
                                                      , completedAckIncrement);
                TEST_ASSERT_CUNIT((rc == OK || rc == ISMRC_AsyncCompletion),
                                  ("rc was %d\n", rc));

                if (rc == OK)
                {
                    completedAckIncrement(rc, NULL, &pMsgsAckCompleted);
                }

            }
            ism_engine_releaseMessage(hMessage);

            break;
        default:
            TEST_ASSERT(false, ("Invalid state %d", state));
            break;
    }

    return true;
}

/*********************************************************************/
/* Name:        tiqPutterContext_t                                   */
/* Description: Context informaton used to pass information to       */
/*              Putter thread          n.                            */
/*********************************************************************/
typedef struct tiqPutterContext_t
{
    pthread_t handle;
    ismQHandle_t hQueue;
    bool *pEnd;
    uint8_t MsgReliability;
    uint32_t minMsgSize;
    uint32_t maxMsgSize;
    uint64_t msgPutCount;
} tiqPutterContext_t;
  
/*********************************************************************/
/* Name:        putterThread                                         */
/* Description: Repeatedly loops putting messages to a queue until   */
/*              requested to end.                                    */
/* Parameters:                                                       */
/*     arg                 - tiqPutterContext_t structure            */
/*********************************************************************/
static void *putterThread( void *arg )
{
    DEBUG_ONLY uint32_t rc;
    tiqPutterContext_t *pContext=(tiqPutterContext_t *)arg;
    uint32_t msgId = 0;
    uint32_t msgLength;
    uint32_t variation;
    uint16_t CheckSum;
    ismEngine_Message_t *pMessage;
    ismEngine_SessionHandle_t hSession;

    ism_engine_threadInit(0);
    
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // Create Session for this test
    rc = ism_engine_createSession( hClient
                                 , ismENGINE_CREATE_SESSION_OPTION_NONE
                                 , &hSession
                                 , NULL
                                 , 0
                                 , NULL);
    TEST_ASSERT(rc == OK, ("createSession returned %rc",rc));


    assert (pContext->maxMsgSize >= pContext->minMsgSize);
    variation = pContext->maxMsgSize-pContext->minMsgSize;

    while (*(pContext->pEnd) == false) /* BEAM suppression: infinite loop */
    {
        msgLength = pContext->minMsgSize;
        if (variation)
        {
            msgLength += random() % variation;
        }

        allocMsg( hSession
                , false
                , pContext->MsgReliability
                , ++msgId
                , msgLength
                , &pMessage
                , &CheckSum);

        rc = ieq_put( pThreadData
                    , pContext->hQueue
                    , ieqPutOptions_NONE
                    , NULL
                    , pMessage
                    , IEQ_MSGTYPE_REFCOUNT
                    , NULL );
        TEST_ASSERT(rc == OK, ("ieq_put returned %rc",rc));


        pContext->msgPutCount++;

        ism_engine_releaseMessage(pMessage);
    }

    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT(rc == OK, ("ism_engine_destroySession returned %rc",rc));

    ism_engine_threadTerm(1);

    return NULL;
}

/*********************************************************************/
/* Name:        acknowledgeAllMsgs                                   */
/* Description: Utility function used to acknowledge the delivery    */
/*              of all received messages.                            */
/* Parameters:                                                       */
/*     hSession            - Session                                 */
/*     hConsumer           - Consumer                                */
/*     Reliability         - Message reliability                     */
/*     msgCount            - Number of messages expected             */
/*     pMsgs               - Array of messages including delivery    */
/*                           handles.                                */
/*     type                - type of queue                           */
/*     doubleAckRcvFreq    - double ack(rcv) every doubleAckFreq'th  */
/*                           msg (test for PMR 90255,000,858)        */
/*********************************************************************/
static void acknowledgeAllMsgs( ismEngine_SessionHandle_t hSession
                                , ismEngine_ConsumerHandle_t hConsumer
                                , uint8_t Reliability
                                , uint32_t msgCount
                                , tiqMsg_t *pMsgs
                                , ismQueueType_t type
                                , uint32_t doubleAckRcvFreq) {
    uint32_t counter;
    uint32_t ackCount=0;
    int32_t rc;
    uint8_t doubleAckRcv = doubleAckRcvFreq == 1 ? 1 : 0;

    verbose(4, "Acknowledging messages,");

    for (counter=0; counter < msgCount; counter++)
    {
        if (doubleAckRcvFreq > 1 ) {
            doubleAckRcv = (counter % doubleAckRcvFreq) == 0 ? 1 : 0;
        }

        verbose(5, "Acknowledging message %d", counter);
        TEST_ASSERT_EQUAL(pMsgs[counter].Received, true);
        TEST_ASSERT_EQUAL(pMsgs[counter].Ackd, false);

        if (   (Reliability==ismMESSAGE_RELIABILITY_EXACTLY_ONCE)
                &&(type != multiConsumer) )
        {
            rc = sync_ism_engine_confirmMessageDelivery( hSession
                    , NULL
                    , pMsgs[counter].hDelivery
                    , ismENGINE_CONFIRM_OPTION_RECEIVED);

            TEST_ASSERT_EQUAL(rc, OK)

            if (doubleAckRcv == 1) {
                verbose(5, "Double Acknowledging message receipt %d", counter);
                rc = sync_ism_engine_confirmMessageDelivery( hSession
                                    , NULL
                                    , pMsgs[counter].hDelivery
                                    , ismENGINE_CONFIRM_OPTION_RECEIVED);
                TEST_ASSERT_EQUAL(rc, OK)
            }
        }

        rc=sync_ism_engine_confirmMessageDelivery( hSession
                , NULL
                , pMsgs[counter].hDelivery
                , ismENGINE_CONFIRM_OPTION_CONSUMED);
        TEST_ASSERT_EQUAL(rc, OK)

        pMsgs[counter].Ackd = true;
        ackCount++;
    }

    verbose(4, "Acknowledged %d messages.", ackCount);
}

/*********************************************************************/
/* Name:        checkMsgArray                                        */
/* Description: Utility function used to get and verify the messages */
/*              from a queue.                                        */
/* Parameters:                                                       */
/*     hSession            - Session                                 */
/*     hQueue              - Queue                                   */
/*     MsgReliability      - Qualilty of Service for message 0-2     */
/*     msgCount            - Number of messages expected             */
/*     AckMode             - Auto-acknowledge messages               */
/*     negAckRatio         - NegativelyAck 1 in 'n' messages         */
/*     pMsgs               - Array of messages for comparison        */
/*     doubleAckRcvFreq    - double ack(rcv) every                   */
/*                           doubleAckRcvFreq'th msg (test for PMR   */
/*                           90255,000,858)                          */
/*********************************************************************/
static void checkMsgArray( ismEngine_SessionHandle_t hSession
                         , ismEngine_ConsumerHandle_t hConsumer
                         , tiqConsumerContext_t *pCallbackContext
                         , ismQHandle_t hQueue
                         , uint8_t MessageReliability
                         , uint32_t msgCount
                         , AckMode_t AckMode
                         , tiqMsg_t *pMsgs
                         , ismQueueType_t type
                         , uint32_t doubleAckRcvFreq)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_QueueStatistics_t stats;

    verbose(5, "Waiting for messages to be delivered\n");
    // wait for the consumer to be signalled that there are no more messages
    test_waitForMessages(&(pCallbackContext->msgsReceived), NULL, msgCount, 20);
    test_waitForMessages(&(pCallbackContext->msgsAckCompleted), NULL, pCallbackContext->msgsAckStarted, 20);

    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

    verbose(3, "Number of messages Received was  %d", pCallbackContext->msgsReceived);
    verbose(3, "Number of messages Ackd was %d", pCallbackContext->msgsAckCompleted);
    verbose(3, "Number of messages Negatively-Ackd was %d", pCallbackContext->msgsNegAckd);
    verbose(3, "Number of duplicate messages was %d (expected %d)", pCallbackContext->dupMsgs,
            pCallbackContext->expectedDupCount);

    // verify that the correct number of messages have been received
    TEST_ASSERT_EQUAL(pCallbackContext->msgsReceived, msgCount + pCallbackContext->expectedDupCount);
    TEST_ASSERT_EQUAL(pCallbackContext->dupMsgs, pCallbackContext->expectedDupCount);

    if (AckMode == Manual)
    {
        if ((type != simple) &&
            (MessageReliability != ismMESSAGE_RELIABILITY_AT_MOST_ONCE))
        {
            verbose(3, "Before acknowledge expecting %d messages. Received %d. %d Remaining",
                msgCount, pCallbackContext->msgsReceived, stats.BufferedMsgs);
            // If the messages put are non QoS 0 and the consumer is not
            // QoS 0 then the messages should still be on the queue
            TEST_ASSERT_EQUAL(stats.BufferedMsgs, msgCount);

            // Now acknowledge each message
            acknowledgeAllMsgs( hSession
                              , hConsumer
                              , MessageReliability
                              , msgCount
                              , pMsgs
                              , type
                              , doubleAckRcvFreq);
        }
    }

    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    verbose(3, "Expecting %d messages. Received %d. %d Remaining",
            msgCount, pCallbackContext->msgsReceived, stats.BufferedMsgs);
    if (AckMode == Never)
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, msgCount);
    }
    else
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
    }

    return;
}

/*********************************************************************/
/* Name:        receiveMsgs                                          */
/* Description: This function is used to create a consumer on a      */
/*              queue and receive the supplied number of messages.   */
/* Parameters:                                                       */
/*     hSession    - Session under which to create consumer          */
/*     hQueue      - Handle to queue                                 */
/*     numMsgs     - Number of messages to receive                   */
/*     confirmType - How to receive the message                      */
/*********************************************************************/
typedef struct receiveContext_t 
{
    ismEngine_SessionHandle_t hSession;
    uint32_t msgsToReceive;
    uint32_t confirmOption;
    ismEngine_Transaction_t *pTransaction;
// State fields
    uint32_t msgsReceived;
} receiveContext_t;

static bool receiveCallback( ismEngine_ConsumerHandle_t  hConsumer
                           , ismEngine_DeliveryHandle_t  hDelivery
                           , ismEngine_MessageHandle_t   hMessage
                           , uint32_t                    deliveryId
                           , ismMessageState_t           state
                           , uint32_t                    destinationOptions
                           , ismMessageHeader_t *        pMsgDetails
                           , uint8_t                     areaCount
                           , ismMessageAreaType_t        areaTypes[areaCount]
                           , size_t                      areaLengths[areaCount]
                           , void *                      pAreaData[areaCount]
                           , void *                      pConsumerContext
                           , ismEngine_DelivererContext_t * _delivererContext)
{
    uint32_t rc;
    bool cont = true;
    receiveContext_t *pContext = *(receiveContext_t **)pConsumerContext;

    pContext->msgsReceived++;

    TEST_ASSERT_CUNIT(pContext->msgsReceived <= pContext->msgsToReceive,
                      ("Number of message received %d is greater than limit %d\n",
                       pContext->msgsReceived, pContext->msgsToReceive));

    if (state != ismMESSAGE_STATE_CONSUMED)
    {
        rc = ism_engine_confirmMessageDelivery( pContext->hSession
                                              , pContext->pTransaction
                                              , hDelivery
                                              , pContext->confirmOption
                                              , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    ism_engine_releaseMessage(hMessage);

    if (pContext->msgsReceived >= pContext->msgsToReceive)
    {
        verbose(5, "Final message %d received", pContext->msgsReceived);
        cont = false;
    }
    else
    {
        verbose(5, "Received message %d", pContext->msgsReceived);
    }

    return cont;
}

void receiveMsgs( ismEngine_SessionHandle_t hSession
                , char *queueName
                , ismQueueType_t type
                , uint32_t numMsgs
                , uint32_t confirmType
                , ismEngine_Transaction_t *pTransaction)
{
    uint32_t rc;
    receiveContext_t CallbackContext = { hSession, numMsgs, confirmType, pTransaction, 0 };
    receiveContext_t *pCallbackContext = &CallbackContext;
    ismEngine_ConsumerHandle_t hConsumer;
    
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pCallbackContext
                                  , sizeof(pCallbackContext)
                                  , receiveCallback
                                  , NULL
                                  , test_getDefaultConsumerOptionsFromQType(type)
                                  , &hConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    // enable the waiter
    rc = ism_engine_startMessageDelivery( hSession
                                        , ismENGINE_START_DELIVERY_OPTION_NONE
                                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // All of the messages should be delivered by now.
    TEST_ASSERT_EQUAL(CallbackContext.msgsReceived, numMsgs);

    // enable the waiter
    rc = ism_engine_stopMessageDelivery( hSession
                                       , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer( hConsumer, NULL, 0 , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    return;
}

/*********************************************************************/
/* Name:        Queue2_PutGet                                        */
/* Description: This test function defines a queue and then puts     */
/*              msgCount messages to the queue and then get's them.  */
/*              During this process the number of messages put/got   */
/*              are checked as well as the message contents.         */
/* Parameters:                                                       */
/*     type - Type of queue to use simple / intermediate             */
/*     extraQOptions       - Additional ieqOption values to set.     */
/*     messageCount        - The number of messages to put           */
/*     minMsgSize          - The size of the messages put will be    */
/*     maxMsgSize          - randomly picked between these 2 values  */
/*     Persistence         - Message persistance                     */
/*     MsgReliability      - Qualilty of Service for message 0-2     */
/*     ConsumerReliability - Qualilty of Service for consumer 0-2    */
/*     doubleAckRcvFreq    - double ack(rcv) every doubleAckFreq'th  */
/*                           msg (test for PMR 90255,000,858)        */
/*********************************************************************/
static void Queue2_PutGet( ismQueueType_t type
                         , uint32_t extraQOptions
                         , uint32_t messageCount
                         , uint32_t minMsgSize
                         , uint32_t maxMsgSize
                         , uint8_t Persistence
                         , uint8_t MsgReliability
                         , uint8_t ConsumerReliability
                         , uint32_t doubleAckRcvFreq)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    tiqMsg_t *msgArray;
    ismEngine_QueueStatistics_t stats;
    char QueueTypeBuf[16];

    verbose(2, "Put %d %s messages to a %s queue with Msg QoS(%d) Consumer QoS(%d) and verify that their contents are delivered correctly.",
            messageCount,
            (Persistence)?"Persistent":"Non-Persistent",
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)),
            MsgReliability,
            ConsumerReliability);

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate array of messages to be put
    generateMsgArray( hSession
                    , Persistence
                    , MsgReliability
                    , messageCount
                    , minMsgSize
                    , maxMsgSize
                    , &msgArray );


    // Create the queue
    rc = ieq_createQ( pThreadData
                    , type
                    , NULL
                    , ieqOptions_DEFAULT | extraQOptions
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &hQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");

    // Add the queue to the queue namespace so we can consume from it.
    char queueName[32];
    strcpy(queueName, __func__);
    rc = ieqn_addQueue(pThreadData,
                       queueName,
                       hQueue,
                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that, for a temporary queue, no store object was created.
    if (extraQOptions & ieqOptions_TEMPORARY_QUEUE)
    {
        TEST_ASSERT_EQUAL(ieq_getDefnHdl(hQueue), 0);
    }

    // Put array of messages
    putMsgArray(hQueue, NULL, messageCount, msgArray);

    verbose(4, "Put %d messages", messageCount);

    // Check Q depth
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, messageCount);

    // TODO - This test in only applicable to the intermediate queue
    // currently but I think this statistic should be kepy for the
    // simple queue also in due course.
    if ( type != simple )
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, messageCount);
    }

    verbose(3, "Checked %d messages exist on queue", messageCount);

    // Check messages

    // initialise the consumer
    tiqConsumerContext_t CallbackContext = { hSession, Manual, 0, ConsumerReliability, 0, 0, 0, 0, 0, 0, msgArray, type };
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    ismEngine_ConsumerHandle_t hConsumer;

    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pCallbackContext
                                  , sizeof(pCallbackContext)
                                  , MessageCallback
                                  , NULL
                                  , test_getDefaultConsumerOptionsFromQType(type)
                                  , &hConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    // enable the waiter
    rc = ism_engine_startMessageDelivery( hSession
                                        , ismENGINE_START_DELIVERY_OPTION_NONE
                                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    checkMsgArray( hSession
                 , hConsumer
                 , &CallbackContext
                 , hQueue
                 , MsgReliability
                 , messageCount
                 , Manual
                 , msgArray
                 , type
                 , doubleAckRcvFreq);

    freeMsgArray( hSession, messageCount, msgArray);

    // Cleaup session (will destrouy the consumer)
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete Q
    rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Deleted queue");

    return;
}


/*********************************************************************/
/* Name:        Queue2_EnableDisable                                 */
/* Description: This test function defines a queue and then starts   */
/*              a number of threads to put messages to the queue.    */
/*              While messages are being put to the queue, the main  */
/*              thread thread randomly enables/disables the waiter   */
/*              a number of times.                                   */
/*              This test checks:                                    */
/*              - That all of the messages put are received          */
/*                                                                   */
/* Parameters:                                                       */
/*     type                - Type of queue to use simple/intermediate*/
/*     extraQOptions       - Additional ieqOptions values to set.    */
/*     putterCount         - The number of putter threads            */
/*     cycleCount          - The number of enable/disable calls      */
/*     minMsgSize          - The size of the messages put will be    */
/*     maxMsgSize          - randomly picked between these 2 values  */
/*********************************************************************/
static void Queue2_EnableDisable( ismQueueType_t type
                                , uint32_t extraQOptions
                                , uint32_t putterCount
                                , uint32_t cycleCount
                                , uint32_t minMsgSize
                                , uint32_t maxMsgSize )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    bool End=false;
    uint64_t totalMsgsPut = 0;
    uint8_t MsgReliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    uint8_t ConsumerReliability = (type == simple)?ismMESSAGE_RELIABILITY_AT_MOST_ONCE:ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    tiqConsumerContext_t CallbackContext = { NULL, true, 0, ConsumerReliability, 0, 0, 0, 0, 0, 0, NULL, type };
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    tiqPutterContext_t *PutContexts;
    ismEngine_Consumer_t *pConsumer;
    uint32_t counter;
    char QueueTypeBuf[16];

    verbose(2, "Enable/Disable waiter %d times while putting messages to %s queue",
            cycleCount,
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)));

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    CallbackContext.hSession = hSession;

    // Create the queue
    rc = ieq_createQ( pThreadData
                    , type
                    , NULL
                    , ieqOptions_DEFAULT | extraQOptions
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &hQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");
    
    // Add the queue to the queue namespace so we can consume from it.
    char queueName[32];
    strcpy(queueName, __func__);
    rc = ieqn_addQueue(pThreadData,
                       queueName,
                       hQueue,
                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that, for a temporary queue, no store object was created.
    if (extraQOptions & ieqOptions_TEMPORARY_QUEUE)
    {
        TEST_ASSERT_EQUAL(ieq_getDefnHdl(hQueue), 0);
    }

    // Initialise a consumer
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pCallbackContext
                                  , sizeof(pCallbackContext)
                                  , MessageCallback2
                                  , NULL
                                  , test_getDefaultConsumerOptionsFromQType(type)
                                  , &pConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    //Start Message Delivery on the consumer
    rc = ism_engine_startMessageDelivery( hSession,
                                          ismENGINE_START_DELIVERY_OPTION_NONE,
                                          NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the putter threads
    PutContexts=(tiqPutterContext_t *)calloc(putterCount, sizeof(tiqPutterContext_t));
    for (counter=0; counter < putterCount; counter++)
    {
        PutContexts[counter].hQueue = hQueue;
        PutContexts[counter].minMsgSize = minMsgSize;
        PutContexts[counter].maxMsgSize = maxMsgSize;
        PutContexts[counter].msgPutCount = 0;
        PutContexts[counter].MsgReliability = MsgReliability;
        PutContexts[counter].pEnd = &End;

        rc = test_task_startThread( &(PutContexts[counter].handle) ,putterThread, (void *)&(PutContexts[counter]),"putterThread");

        TEST_ASSERT(rc == 0, ("pthread_create returned %rc",rc));

    }
    verbose(4, "Started %d threads", putterCount);

    // Ensure the putters get a chance to run
    sleep(1);

    // And finally the nitty-gritty, loop round either enabling or disabling
    // the queue.
    for (counter=1; counter <= cycleCount; counter++)
    {
        if (((rand() % 2) == 0) || (counter == cycleCount))
        {
            verbose(5, "Starting msg delivery");

            //Start Message Delivery on the consumer
            uint32_t loopCount=0;
            do
            {
                rc = ism_engine_startMessageDelivery( hSession,
                                                      ismENGINE_START_DELIVERY_OPTION_NONE,
                                                      NULL, 0, NULL);
                if (rc != OK)
                {
                    verbose(3, "rc(%d) from ism_engine_startMessageDelivery", rc);
                    if(rc == ISMRC_RequestInProgress)
                    {
                        if (counter < cycleCount)
                        {
                            //Just ignore it
                            rc = OK;
                        }
                        else
                        {
                            //This is the last loop... we NEED to enable it otherwise
                            //the messages will be trapped on the queue
                            loopCount++;
                            if (loopCount > 30000) //It's had 30 seconds!!!...
                            {
                                printf("Failing to start message delivery!!!\n");
                                TEST_ASSERT(0,("failing to start message delivery"));

                            }
                            else
                            {
                                usleep(1000); //sleep for a millisecond
                            }
                        }
                    }
                }
            }
            while(rc == ISMRC_RequestInProgress);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            // Stop message delivery
            // ieq_disableWaiter(hQueue, pConsumer);
            verbose(5, "Stopping msg delivery");

            //Start Message Delivery on the consumer
            rc = ism_engine_stopMessageDelivery( hSession,
                                                 NULL, 0, NULL);
            if (rc != OK)
            {
                verbose(3, "rc(%d) from ism_engine_stopMessageDelivery", rc);
                if (rc == ISMRC_RequestInProgress) rc = OK;
            }
            TEST_ASSERT_EQUAL(rc, OK);
        }

        sched_yield();
    }

    // Having reached here ask the putters to stop
    End = true;

    // And wait for the putters to end
    totalMsgsPut = 0;
    for (counter=0; counter < putterCount; counter++)
    {
        rc = pthread_join(PutContexts[counter].handle, NULL);
        TEST_ASSERT_EQUAL(rc, 0);

        verbose(4, "Putter %d received %d messages", counter, PutContexts[counter].msgPutCount);
        totalMsgsPut += PutContexts[counter].msgPutCount;
    }

    verbose(3, "Put %d messages. Received %d messages", totalMsgsPut, CallbackContext.msgsReceived);

    // Having reached here, check that the number of messages put 
    // matches the
    TEST_ASSERT_EQUAL(totalMsgsPut, CallbackContext.msgsReceived);
    
    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete Q
    rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Deleted queue");

    free(PutContexts);

    return;
}



/*********************************************************************/
/* Name:        Queue2_Put                                           */
/* Description: This test function defines a queue and then puts     */
/*              msgCount messages to the queue. During this process  */
/*              the number of messages put is verfified.             */
/*              This test is useful to verify we do not leak memory  */
/*              while deleting a queue with pending msgs             */
/* Parameters:                                                       */
/*     type - Type of queue to use simple / intermediate             */
/*     extraQOptions       - Additional ieqOptions value to set      */
/*     messageCount        - The number of messages to put           */
/*     minMsgSize          - The size of the messages put will be    */
/*     maxMsgSize          - randomly picked between these 2 values  */
/*     Persistence         - Message persistance                     */
/*     MsgReliability      - Qualilty of Service for message 0-2     */
/*********************************************************************/
static void Queue2_Put( ismQueueType_t type
                      , uint32_t extraQOptions
                      , uint32_t messageCount
                      , uint32_t minMsgSize
                      , uint32_t maxMsgSize
                      , uint8_t Persistence
                      , uint8_t MsgReliability )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    tiqMsg_t *msgArray;
    ismEngine_QueueStatistics_t stats;
    char QueueTypeBuf[16];

    verbose(2, "Put %d %s messages to a %s queue with Msg QoS(%d) and verify that their contents are delivered correctly.",
            messageCount,
            (Persistence)?"Persistent":"Non-Persistent",
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)),
            MsgReliability);

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate array of messages to be put
    generateMsgArray( hSession
                    , Persistence
                    , MsgReliability
                    , messageCount
                    , minMsgSize
                    , maxMsgSize
                    , &msgArray );

   
    // Create the queue
    rc = ieq_createQ( pThreadData
                    , type
                    , NULL
                    , ieqOptions_DEFAULT | extraQOptions
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &hQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");
    
    // Check that, for a temporary queue, no store object was created.
    if (extraQOptions & ieqOptions_TEMPORARY_QUEUE)
    {
        TEST_ASSERT_EQUAL(ieq_getDefnHdl(hQueue), 0);
    }

    // Put array of messages
    putMsgArray(hQueue, NULL, messageCount, msgArray);

    verbose(4, "Put %d messages", messageCount);

    // Check Q depth 
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, messageCount);

    // TODO - This test in only applicable to the intermediate queue
    // currently but I think this statistic should be kepy for the
    // simple queue also in due course.
    if ( type != simple )
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, messageCount);
    }

    verbose(3, "Checked %d messages exist on queue", messageCount);
   
    freeMsgArray( hSession, messageCount, msgArray);

    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete Q
    rc = ieq_delete(pThreadData, &hQueue, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Deleted queue");


    return;
}


/*********************************************************************/
/* Name:        Queue2_Commit                                        */
/* Description: This test function defines a queue and then puts     */
/*              a number of messages to the queue before calling     */
/*              Commit.                                              */
/* Parameters:                                                       */
/*     type - Type of queue to use simple / intermediate             */
/*     extraQOptions       - Additional ieqOptions values to set     */
/*     messageCount        - The number of messages to put           */
/*     Persistence         - Message persistance                     */
/*     MsgReliability      - Qualilty of Service for message 0-2     */
/*********************************************************************/
static void Queue2_Commit( ismQueueType_t type
                         , uint32_t extraQOptions
                         , uint32_t messageCount
                         , uint8_t Persistence
                         , uint8_t MsgReliability )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    tiqMsg_t *msgArray;
    ismEngine_QueueStatistics_t stats;
    ismEngine_Transaction_t *pTransaction;
    ismEngine_Consumer_t *pConsumer;
    uint8_t ConsumerReliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    tiqConsumerContext_t CallbackContext = { NULL, Auto, 0, ConsumerReliability, 0, 0, 0, 0, 0, 0, NULL, type };
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    char QueueTypeBuf[16];

    verbose(2, "Put %d messages to a %s queue inside a transaction and verify they are not delivered until commit is called.",
            messageCount,
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)));

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    CallbackContext.hSession = hSession;

    // Generate array of messages to be put
    generateMsgArray( hSession
                    , Persistence
                    , MsgReliability
                    , messageCount
                    , 32
                    , 16384
                    , &msgArray );

    CallbackContext.pMsgs = msgArray;
   
    // Create the queue
    rc = ieq_createQ( pThreadData
                    , type
                    , NULL
                    , ieqOptions_DEFAULT | extraQOptions
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &hQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");

    // Check that, for a temporary queue, no store object was created.
    if (extraQOptions & ieqOptions_TEMPORARY_QUEUE)
    {
        TEST_ASSERT_EQUAL(ieq_getDefnHdl(hQueue), 0);
    }

    // Add the queue to the queue namespace so we can consume from it.
    char queueName[32];
    strcpy(queueName, __func__);
    rc = ieqn_addQueue(pThreadData,
                       queueName,
                       hQueue,
                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create the transaction
    rc = ietr_createLocal( pThreadData,
                           NULL,
                           (Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)?true:false,
                           false,
						   NULL,
                           &pTransaction );
    TEST_ASSERT_EQUAL(rc, OK);

    // Put array of messages
    putMsgArray(hQueue, pTransaction, messageCount, msgArray);

    verbose(4, "Put %d messages", messageCount);

    // Check Q depth 
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    if (type != simple)
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, messageCount);
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, messageCount);

        verbose(3, "Checked %d messages exist on queue", messageCount);
    }
    else
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);

        verbose(3, "Checked no messages yet exist on queue");
    }

    // initialise a consumer
    verbose(4, "Create consumer");
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
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
    TEST_ASSERT_EQUAL(rc, OK);

    // enable the waiter
    rc = ism_engine_startMessageDelivery( hSession
                                        , ismENGINE_START_DELIVERY_OPTION_NONE
                                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Wait for 1 second
    sleep(1);

    // Verify that no messages have been delivered

    verbose(2, "Verify that no messages have been delivered to the consumer.");
    TEST_ASSERT_EQUAL(CallbackContext.msgsReceived, 0);

    // Commit the transaction
    verbose(4, "Call commit.");
    rc = ietr_commit(pThreadData, pTransaction, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now attempt to get the messages, from the queue. 
    do
    {
        ieq_getStats(pThreadData, hQueue, &stats);
        ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
        uint64_t lastbufferedmsgs = stats.BufferedMsgs;
        int32_t retry = 0;

        while(  (stats.BufferedMsgs == lastbufferedmsgs)
              &&(stats.BufferedMsgs > 0))
        {
            usleep(200*1000); //200ms = 0.2s
            retry++;
            if (retry > 5*10) //10s without a message...
            {
                test_log(1, "No msgs delivered for 10s.... aborting.");
                abort();
            }
            ieq_getStats(pThreadData, hQueue, &stats);
            ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
        }
    }
    while (stats.BufferedMsgs > 0);
    
    // verify that the correct number of messages have been received
    verbose(3, "Verify that all messages are receieved (%d of %d)", 
            CallbackContext.msgsReceived, messageCount);
    TEST_ASSERT_EQUAL(CallbackContext.msgsReceived, messageCount);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
    
    freeMsgArray( hSession, messageCount, msgArray);

    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);


    // Delete Q
    rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Deleted queue");


    return;
}

/*********************************************************************/
/* Name:        Queue2_Rollback                                      */
/* Description: This test function defines a queue and then puts     */
/*              a number of messages to the queue before calling     */
/*              Rollback.                                            */
/* Parameters:                                                       */
/*     type - Type of queue to use simple / intermediate             */
/*     extraQOptions       - Additional ieqOptions values to set.    */
/*     messageCount        - The number of messages to put           */
/*     Persistence         - Message persistance                     */
/*     MsgReliability      - Qualilty of Service for message 0-2     */
/*********************************************************************/
static void Queue2_Rollback( ismQueueType_t type
                           , uint32_t extraQOptions
                           , uint32_t messageCount
                           , uint8_t Persistence
                           , uint8_t MsgReliability )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    tiqMsg_t *msgArray;
    ismEngine_QueueStatistics_t stats;
    ismEngine_Transaction_t *pTransaction;
    char QueueTypeBuf[16];

    verbose(2, "Put %d messages to a %s queue inside a transaction and verify they are not delivered and removed when rollback is called.",
            messageCount,
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)));

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate array of messages to be put
    generateMsgArray( hSession
                    , Persistence
                    , MsgReliability
                    , messageCount
                    , 32
                    , 16384
                    , &msgArray );

    // Create the queue
    rc = ieq_createQ( pThreadData
                    , type
                    , NULL
                    , ieqOptions_DEFAULT | extraQOptions
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &hQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");

    // Check that, for a temporary queue, no store object was created.
    if (extraQOptions & ieqOptions_TEMPORARY_QUEUE)
    {
        TEST_ASSERT_EQUAL(ieq_getDefnHdl(hQueue), 0);
    }

    // Create the transaction
    rc = ietr_createLocal( pThreadData,
                           NULL,
                           (Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)?true:false,
                           false, NULL, &pTransaction );
    TEST_ASSERT_EQUAL(rc, OK);

    // Put array of messages
    putMsgArray(hQueue, pTransaction, messageCount, msgArray);

    verbose(4, "Put %d messages", messageCount);

    freeMsgArray( hSession, messageCount, msgArray);

    // Check Q depth 
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    if (type == simple)
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);

        verbose(3, "Checked no messages yet exist on queue");
    }
    else
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, messageCount);
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, messageCount);

        verbose(3, "Checked %d messages exist on queue", messageCount);
    }

    // Rollback the transaction
    verbose(4, "Call rollback.");
    rc = ietr_rollback(pThreadData, pTransaction, NULL, IETR_ROLLBACK_OPTIONS_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now verify that no messages exist on the queue
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

    verbose(4, "Verify no messages remain on queue.");
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);

    // TODO - This test in only applicable to the intermediate queue
    // currently but I think this statistic should be kepy for the
    // simple queue also in due course.
    if ( type != simple )
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, messageCount);
    }

    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete Q
    rc = ieq_delete(pThreadData, &hQueue, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Deleted queue");

    return;
}

/*********************************************************************/
/* Name:        Queue2_DrainQ                                        */
/* Description: This test function defines a queue and then puts     */
/*              a number of messages to the queue. It may optionally */
/*              start a consumer on the queue before calling DrainQ. */
/* Parameters:                                                       */
/*     type - Type of queue to use simple / intermediate             */
/*     extraQOptions       - Additional ieqOptions values to set.    */
/*     Persistence         - Message persistance                     */
/*     MsgReliability      - Qualilty of Service for message 0-2     */
/*********************************************************************/
static void Queue2_DrainQ( ismQueueType_t type
                         , uint32_t extraQOptions
                         , uint8_t Persistence
                         , uint8_t MsgReliability )
{
#ifdef ENGINE_FORCE_INTERMEDIATE_TO_MULTI
	if (type == intermediate )
	{
	   verbose(1, "Multiconsumer queue doesn't support drain at the moment...");
	   return;
	}
#endif

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    tiqMsg_t *msgArray;
    uint32_t msgCount = 200;
    ismEngine_QueueStatistics_t stats;
    uint8_t ConsumerReliability = MsgReliability;
    char QueueTypeBuf[16];

    verbose(2, "Test the basic operation of  DrainQ for a %s queue", 
        ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)));

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate array of messages to be put
    generateMsgArray( hSession
                    , Persistence
                    , MsgReliability
                    , msgCount
                    , 32
                    , 16384
                    , &msgArray );

    // Create the queue
    rc = ieq_createQ( pThreadData
                    , type
                    , NULL
                    , ieqOptions_DEFAULT | extraQOptions
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &hQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");

    // Add the queue to the queue namespace so we can consume from it.
    char queueName[32];
    strcpy(queueName, __func__);
    rc = ieqn_addQueue(pThreadData,
                       queueName,
                       hQueue,
                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    putMsgArray(hQueue, NULL, msgCount, msgArray);

    verbose(4, "Put %d messages", msgCount);

    // verify that there are messages on the queue
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, msgCount);
    if (type != simple)
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, msgCount);
    }

    verbose(3, "Checked %d messages exist on queue", msgCount);

    // Now drain the queue of all messages
    rc = ieq_drain(pThreadData, hQueue);
    TEST_ASSERT_EQUAL(rc, OK);

    // Re-put the messages
    putMsgArray(hQueue, NULL, msgCount, msgArray);

    // initialise the consumer
    tiqConsumerContext_t CallbackContext = { hSession, Auto, 0, ConsumerReliability, 0, 0, 0, 0, 0, 0, msgArray, type };
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    ismEngine_ConsumerHandle_t hConsumer;

    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pCallbackContext
                                  , sizeof(pCallbackContext)
                                  , MessageCallback
                                  , NULL
                                  , test_getDefaultConsumerOptionsFromQType(type)
                                  , &hConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    // enable the waiter
    rc = ism_engine_startMessageDelivery( hSession
                                        , ismENGINE_START_DELIVERY_OPTION_NONE
                                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    checkMsgArray( hSession
                 , hConsumer
                 , &CallbackContext
                 , hQueue
                 , MsgReliability
                 , msgCount
                 , Auto
                 , msgArray
                 , type
                 , 0);

    freeMsgArray( hSession, msgCount, msgArray);

    // verify that there no messages on the queue
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
    if (type != simple)
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, 0);
    }
    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete Q
    rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Deleted queue");

    return;
}

/*********************************************************************/
/* Name:        Queue2_DrainQAdv                                     */
/* Description: This test function peforms advanced testing of the   */
/*              DrainQ functionality.                                */
/* Parameters:                                                       */
/*     type - Type of queue to use simple / intermediate             */
/*     extraQOptions       - Additional ieqOptions values to set.    */
/*     getCount            - The number of messages to get           */
/*     Persistence         - Message persistance                     */
/*     MsgReliability      - Qualilty of Service for message 0-2     */
/*********************************************************************/
static void Queue2_DrainQAdv( ismQueueType_t type
                            , uint32_t extraQOptions
                            , uint8_t Persistence
                            , uint8_t MsgReliability )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    tiqMsg_t *msgArray;
    uint32_t msgCount = 200;
    ismEngine_QueueStatistics_t stats;
    uint8_t ConsumerReliability = MsgReliability;
    char QueueTypeBuf[16];

#ifdef ENGINE_FORCE_INTERMEDIATE_TO_MULTI
	if (type == intermediate )
	{
	   verbose(1, "Multiconsumer queue doesn't support drain at the moment...");
	   return;
	}
#endif

    verbose(2, "Test the failure to call DrainQ for a %s queue becuase there are unacknowledged messages", 
        ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)));

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate array of messages to be put
    generateMsgArray( hSession
                    , Persistence
                    , MsgReliability
                    , msgCount
                    , 32
                    , 16384
                    , &msgArray );

    // Create the queue
    rc = ieq_createQ( pThreadData
                    , type
                    , NULL
                    , ieqOptions_DEFAULT | extraQOptions
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &hQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");

    // Add the queue to the queue namespace so we can consume from it.
    char queueName[32];
    strcpy(queueName, __func__);
    rc = ieqn_addQueue(pThreadData,
                       queueName,
                       hQueue,
                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // First try draining the empty queue
    rc = ieq_drain(pThreadData, hQueue);
    TEST_ASSERT_EQUAL(rc, OK);

    putMsgArray(hQueue, NULL, msgCount, msgArray);

    verbose(4, "Put %d messages", msgCount);

    // verify that there are messages on the queue
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, msgCount);
    if (type != simple)
    {
        TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, msgCount);
    }

    verbose(3, "Checked %d messages exist on queue", msgCount);

    // initialise the consumer
    tiqConsumerContext_t CallbackContext = { hSession, Manual, 0, ConsumerReliability, 0, 0, 0, 0, 0, 0, msgArray, type };
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    ismEngine_ConsumerHandle_t hConsumer;

    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pCallbackContext
                                  , sizeof(pCallbackContext)
                                  , MessageCallback
                                  , NULL
                                  , test_getDefaultConsumerOptionsFromQType(type)
                                  , &hConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    // enable the waiter
    rc = ism_engine_startMessageDelivery( hSession
                                        , ismENGINE_START_DELIVERY_OPTION_NONE
                                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // receive but don't acknowledge the messages
    checkMsgArray( hSession
                 , hConsumer
                 , &CallbackContext
                 , hQueue
                 , MsgReliability
                 , msgCount
                 , Never
                 , msgArray
                 , type
                 , 0);

    // Now drain the queue of all messages
    rc = ieq_drain(pThreadData, hQueue);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationInUse);

    // verify that all of the messages are still on the queue
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, msgCount);
    TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, msgCount);

    acknowledgeAllMsgs( hSession
                      , NULL
                      , MsgReliability
                      , msgCount
                      , msgArray
                      , type
                      , 0);

    // And then Re-put the messages
    CallbackContext.AckMode = Auto;
    CallbackContext.msgsReceived = 0;
    CallbackContext.msgsAckStarted = 0;
    CallbackContext.msgsAckCompleted = 0;
    CallbackContext.msgsNegAckd = 0;
    CallbackContext.expectedDupCount = 0;
    CallbackContext.dupMsgs = 0;

    putMsgArray(hQueue, NULL, msgCount, msgArray);

    checkMsgArray( hSession
                 , hConsumer
                 , &CallbackContext
                 , hQueue
                 , MsgReliability
                 , msgCount
                 , Auto
                 , msgArray
                 , type
                 , 0);

    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    freeMsgArray( hSession, msgCount, msgArray);

    // Delete Q
    rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Deleted queue");

    return;
}

/*********************************************************************/
/* Name:        Queue2_Nack                                          */
/* Description: This test function defines a queue and then puts     */
/*              a number of messages to the queue. A consumer is     */
/*              then registered which negatively-acks every 4th msg  */
/*              and checks that it is re-delivered.                  */
/* Parameters:                                                       */
/*     type - Type of queue to use simple / intermediate             */
/*     extraQOptions       - Additional ieqOptions values to set     */
/*     messageCount        - The number of messages to put           */
/*     Persistence         - Message persistance                     */
/*     ConsumerReliability - Qualilty of Service for message 1-2     */
/*********************************************************************/
static void Queue2_Nack( ismQueueType_t type
                       , uint32_t extraQOptions
                       , uint32_t messageCount
                       , uint8_t Persistence
                       , uint8_t ConsumerReliability )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    tiqMsg_t *msgArray;
    ismEngine_QueueStatistics_t stats;
    uint8_t MsgReliability = ConsumerReliability;
    uint32_t ratio = 5;
    char QueueTypeBuf[16];

    verbose(2, "Put %d messages to a %s queue and negtively acknowlege 1 in %d messages are negatively acknowledged causing redelivery.",
            messageCount,
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)),
            ratio);

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate array of messages to be put
    generateMsgArray( hSession
                    , Persistence
                    , MsgReliability
                    , messageCount
                    , 32
                    , 16384
                    , &msgArray );

    // Create the queue
    rc = ieq_createQ( pThreadData
                    , type
                    , NULL
                    , ieqOptions_DEFAULT | extraQOptions
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &hQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");
    
    // Check that, for a temporary queue, no store object was created.
    if (extraQOptions & ieqOptions_TEMPORARY_QUEUE)
    {
        TEST_ASSERT_EQUAL(ieq_getDefnHdl(hQueue), 0);
    }

    // Add the queue to the queue namespace so we can consume from it.
    char queueName[32];
    strcpy(queueName, __func__);
    rc = ieqn_addQueue(pThreadData,
                       queueName,
                       hQueue,
                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Put array of messages
    putMsgArray(hQueue, NULL, messageCount, msgArray);

    verbose(4, "Put %d messages", messageCount);

    // Check Q depth 
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, messageCount);
    TEST_ASSERT_EQUAL(stats.BufferedMsgsHWM, messageCount);

    verbose(3, "Checked %d messages exist on queue", messageCount);

    // initialise the consumer
    tiqConsumerContext_t CallbackContext = { hSession, Manual, ratio, ConsumerReliability, 0, 0, 0, 0, 0, 0, msgArray, type };
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    ismEngine_ConsumerHandle_t hConsumer;

    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pCallbackContext
                                  , sizeof(pCallbackContext)
                                  , MessageCallback
                                  , NULL
                                  , test_getDefaultConsumerOptionsFromQType(type)
                                  , &hConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    // enable the waiter
    rc = ism_engine_startMessageDelivery( hSession
                                        , ismENGINE_START_DELIVERY_OPTION_NONE
                                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
   
    // Check messages
    checkMsgArray( hSession
                 , hConsumer
                 , &CallbackContext
                 , hQueue
                 , MsgReliability
                 , messageCount
                 , Manual
                 , msgArray
                 , type
                 , 0);

    freeMsgArray( hSession, messageCount, msgArray);

    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete Q
    rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Deleted queue");

    return;
}

/*********************************************************************/
/* Name:        Queue2_NackActive                                    */
/* Description: This test function defines a queue and starts a      */
/*              consumer. Then a a number of messages are put and    */
/*              the consumer negatively-acks every 5th msg and       */
/*              checks that it is re-delivered.                      */
/* Parameters:                                                       */
/*     type - Type of queue to use simple / intermediate             */
/*     extraQOptions       - Additional ieqOptions values to set     */
/*     messageCount        - The number of messages to put           */
/*     Persistence         - Message persistance                     */
/*     ConsumerReliability - Qualilty of Service for message 1-2     */
/*********************************************************************/
static void Queue2_NackActive( ismQueueType_t type
                             , uint32_t extraQOptions
                             , uint32_t messageCount
                             , uint8_t Persistence
                             , uint8_t MsgReliability )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    tiqMsg_t *msgArray;
    ismEngine_QueueStatistics_t stats;
    ismEngine_Consumer_t *pConsumer;
    uint8_t ConsumerReliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    uint32_t ratio = 5;
    tiqConsumerContext_t CallbackContext = { NULL, Auto, ratio, ConsumerReliability, 0, 0, 0, 0, 0, 0, NULL, type };
    tiqConsumerContext_t *pCallbackContext = &CallbackContext;
    char queueName[32];
    char QueueTypeBuf[16];

    verbose(2, "Put %d messages to a %s queue with an active consumer and negtively acknowlege 1 in %d messages are negatively acknowledged causing redelivery.",
            messageCount,
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)),
            ratio);

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    CallbackContext.hSession = hSession;

    // Generate array of messages to be put
    generateMsgArray( hSession
                    , Persistence
                    , MsgReliability
                    , messageCount
                    , 32
                    , 16384
                    , &msgArray );

    CallbackContext.pMsgs = msgArray;
   
    // Create the queue
    rc = ieq_createQ( pThreadData
                    , type
                    , NULL
                    , ieqOptions_DEFAULT | extraQOptions
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &hQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");
    
    // Check that, for a temporary queue, no store object was created.
    if (extraQOptions & ieqOptions_TEMPORARY_QUEUE)
    {
        TEST_ASSERT_EQUAL(ieq_getDefnHdl(hQueue), 0);
    }

    // Add the queue to the queue namespace so we can consume from it.
    strcpy(queueName, __func__);
    rc = ieqn_addQueue(pThreadData,
                       queueName,
                       hQueue,
                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // initialise a consumer
    verbose(4, "Create consumer");
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
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
    TEST_ASSERT_EQUAL(rc, OK);

    // enable the waiter
    rc = ism_engine_startMessageDelivery( hSession
                                        , ismENGINE_START_DELIVERY_OPTION_NONE
                                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Put array of messages, we expect that as we put the messages will
    // be put to the consumer
    putMsgArray(hQueue, NULL, messageCount, msgArray);
    verbose(4, "Put %d messages", messageCount);

    do
    {
        ieq_getStats(pThreadData, hQueue, &stats);
        ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

        uint32_t lastBuffered = stats.BufferedMsgs;

        int retry = 0;
        while(   (stats.BufferedMsgs != 0)
              && (stats.BufferedMsgs == lastBuffered) )
        {
            usleep(200*1000); //200ms = 0.2s

            ieq_getStats(pThreadData, hQueue, &stats);
            ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

            retry++;
            if (retry > 5*10) //10s without a message...
            {
                test_log(1, "No msgs received for 10s.... aborting.");
                abort();
            }
        }
    }
    while (stats.BufferedMsgs != 0);

    verbose(4, "After waiting: msgsReceived %d, messageCount %d, expectedDupCount %d\n",CallbackContext.msgsReceived, messageCount,  CallbackContext.expectedDupCount);

    // verify that the correct number of messages have been received
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);

    verbose(3, "Number of messages Received was  %d", CallbackContext.msgsReceived);
    verbose(3, "Number of messages Ackd was %d", CallbackContext.msgsAckCompleted);
    verbose(3, "Number of messages Negatively-Ackd was %d", CallbackContext.msgsNegAckd);
    verbose(3, "Number of duplicate messages was %d (expected %d)", CallbackContext.dupMsgs,
            CallbackContext.expectedDupCount);
    verbose(3, "Verify that all messages are receieved (%d of %d)", 
            CallbackContext.msgsReceived, messageCount);
    TEST_ASSERT_EQUAL(CallbackContext.msgsReceived, messageCount + CallbackContext.expectedDupCount);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);

    freeMsgArray( hSession, messageCount, msgArray);

    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete Q
    rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Deleted queue");

    return;
}

/*********************************************************************/
/* Name:        Queue2_BatchNack                                     */
/* Description: This test function defines 2 queues. It creates a    */
/*              consumer on each queue (part of the same session)    */
/*              and then puts 20 messages to each queue.             */
/*              The consumer does not ack the messages seen but adds */
/*              them to the list of messages to be ack'd. Once all   */
/*              messages have been seen the messages are ack'd       */
/*              using the batch ack api.                             */
/* Parameters:                                                       */
/*     type - Type of queue to use simple / intermediate             */
/*     extraQOptions       - Additional ieqOptions values to set     */
/*     numConsumers        - The number of queues                    */
/*     messageCount        - The number of messages to put           */
/*     Persistence         - Message persistance                     */
/*********************************************************************/
static void Queue2_BatchNack( ismQueueType_t type
                            , uint32_t extraQOptions
                            , uint32_t numConsumers
                            , bool queuePerConsumer
                            , uint32_t messageCount
                            , uint8_t Persistence )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hPubSession;
    ismEngine_SessionHandle_t hConsumerSession;
    ismQHandle_t hQueue[numConsumers];
    ismEngine_ConsumerHandle_t hConsumer[numConsumers];
    char queueName[32];
    char QueueTypeBuf[16];
    tiqConsumerContext_t CallbackContext[numConsumers];
    tiqAckList *pAckList;
    uint32_t numQueues=1;
    uint32_t loop;
    uint32_t msgsReceived;
    tiqMsg_t *msgArray;
    ismEngine_QueueStatistics_t stats;

    if (queuePerConsumer) numQueues=numConsumers;

    verbose(2, "Put %d messages to %d %s queues drained by %d consumers and then negatively acknowledge them in a batch",
            messageCount,
            numQueues,
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)),
            numConsumers);

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hPubSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hConsumerSession
                               , NULL
                               , 0
                               , NULL);

    // Generate the array of messages we will put
    generateMsgArray( hPubSession
                    , Persistence
                    , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                    , messageCount
                    , 32
                    , 16384
                    , &msgArray );

    pAckList = (tiqAckList *)malloc(sizeof(tiqAckList));
    pAckList->DeliveryHdlCount = 0;
    pAckList->DeliveryHdlSize = 0;
    pAckList->pDeliveryHdls = NULL;
    if (pthread_mutex_init(&(pAckList->lock), NULL) != 0)
    {
        abort();
    }

    // Create the queues and consumers
    for (loop=0; loop < numConsumers; loop++)
    {
        if (queuePerConsumer || (loop == 0))
        {
            rc = ieq_createQ( pThreadData
                            , type
                            , NULL
                            , ieqOptions_DEFAULT | extraQOptions
                            , iepi_getDefaultPolicyInfo(true)
                            , ismSTORE_NULL_HANDLE
                            , ismSTORE_NULL_HANDLE
                            , iereNO_RESOURCE_SET
                            , &hQueue[loop] );
            TEST_ASSERT_EQUAL(rc, OK);

            verbose(4, "Created queue %d", loop);

            // Add the queue to the queue namespace so we can consume from it.
            sprintf(queueName, "%s.%u", __func__, loop);
            rc = ieqn_addQueue(pThreadData,
                               queueName,
                               hQueue[loop],
                               NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            hQueue[loop] = hQueue[loop-1];
        }

        // And create a consumer on the queue
        tiqConsumerContext_t *pCallbackContext = &CallbackContext[loop];
        pCallbackContext->hSession = hConsumerSession;
        pCallbackContext->AckMode = Record;
        pCallbackContext->negAckRatio = 0;
        pCallbackContext->Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
        pCallbackContext->msgsReceived = 0;
        pCallbackContext->msgsAckStarted = 0;
        pCallbackContext->msgsAckCompleted = 0;
        pCallbackContext->msgsNegAckd = 0;
        pCallbackContext->expectedDupCount = 0;
        pCallbackContext->dupMsgs = 0;
        pCallbackContext->pMsgs = msgArray;
        pCallbackContext->Qtype = type;
        pCallbackContext->pAckList = pAckList;

        rc = ism_engine_createConsumer( hConsumerSession
                                      , ismDESTINATION_QUEUE
                                      , queueName
                                      , 0
                                      , NULL // Unused for QUEUE
                                      , &pCallbackContext
                                      , sizeof(pCallbackContext)
                                      , MessageCallback
                                      , NULL
                                      , test_getDefaultConsumerOptionsFromQType(type)
                                      , &hConsumer[loop]
                                      , NULL
                                      , 0
                                      , NULL );
        TEST_ASSERT_EQUAL(rc, OK);

        // Check that, for a temporary queue, no store object was created.
        if (extraQOptions & ieqOptions_TEMPORARY_QUEUE)
        {
            TEST_ASSERT_EQUAL(ieq_getDefnHdl(hQueue[loop]), 0);
        }
    }

    // enable the consumers
    rc = ism_engine_startMessageDelivery( hConsumerSession
                                        , ismENGINE_START_DELIVERY_OPTION_NONE
                                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    
    // Now put the messages
    for (loop=0; loop < messageCount; loop++)
    {
        msgArray[loop].Received = false;
        msgArray[loop].Ackd = false;
        msgArray[loop].hDelivery = ismENGINE_NULL_DELIVERY_HANDLE;

        uint32_t numQueue = rand() % numQueues;
        rc = ieq_put( pThreadData
                    , hQueue[numQueue]
                    , ieqPutOptions_NONE
                    , NULL
                    , msgArray[loop].pMessage
                    , IEQ_MSGTYPE_REFCOUNT
                    , NULL );
        TEST_ASSERT_EQUAL(rc, OK);

        verbose(5, "Put message %d", loop);
    }

    // Check that all of the messages have arrived and we've built up a list of things we can
    //subsequently nack
    msgsReceived = 0;
    int retries = 0;

    do
    {
        int lastmsgsReceived = msgsReceived;
        int lasthdlcount     = pAckList->DeliveryHdlCount;

        msgsReceived = 0;
        for (loop=0; loop < numConsumers; loop++)
        {
            msgsReceived += CallbackContext[loop].msgsReceived;
        }

        if ((msgsReceived < messageCount)||(pAckList->DeliveryHdlCount < messageCount))
        {
            usleep(200 * 1000); //0.2s

            if (   (msgsReceived == lastmsgsReceived)
                && (pAckList->DeliveryHdlCount == lasthdlcount))
            {
                retries++;
            }
            else
            {
                //We've made progress
                retries=0;
            }

            if (retries > 100) //100 *0.2s = 20s
            {
                printf("No messages received for a long time. Expected %d (hdlcount %d) received: %d\n", messageCount, pAckList->DeliveryHdlCount, msgsReceived);
            }
        }
    }
    while (   (msgsReceived < messageCount)
           || (pAckList->DeliveryHdlCount < messageCount));


    TEST_ASSERT_EQUAL(msgsReceived, messageCount);
    TEST_ASSERT_EQUAL(pAckList->DeliveryHdlCount, messageCount);

    // disable the consumers
    rc = ism_engine_stopMessageDelivery( hConsumerSession
                                       , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the queue depths to verify that they still contain the messages
    
    uint32_t bufferedMessages = 0;
    for (loop=0; loop < numQueues; loop++)
    {
        ieq_getStats(pThreadData,hQueue[loop], &stats);
        ieq_setStats(hQueue[loop], NULL, ieqSetStats_RESET_ALL);
        bufferedMessages += stats.BufferedMsgs;
    }
    TEST_ASSERT_EQUAL(bufferedMessages, messageCount);


    // NAck the messages using the batch api
    rc = ism_engine_confirmMessageDeliveryBatch( hConsumerSession
                                               , NULL
                                               , pAckList->DeliveryHdlCount
                                               , pAckList->pDeliveryHdls
                                               , ismENGINE_CONFIRM_OPTION_NOT_DELIVERED
                                               , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
  
    // Reset the stats and start message delivery again.
    for (loop=0; loop < numConsumers; loop++)
    {
        tiqConsumerContext_t *pCallbackContext = &CallbackContext[loop];
        pCallbackContext->msgsReceived = 0;
    }
    pAckList->DeliveryHdlCount = 0;

    // enable the consumers
    rc = ism_engine_startMessageDelivery( hConsumerSession
                                        , ismENGINE_START_DELIVERY_OPTION_NONE
                                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that all of the messages have arrived
    msgsReceived = 0;
    retries = 0;

    do
    {
        int lastmsgsReceived = msgsReceived;
        int lasthdlcount     = pAckList->DeliveryHdlCount;

        msgsReceived = 0;
        for (loop=0; loop < numConsumers; loop++)
        {
            msgsReceived += CallbackContext[loop].msgsReceived;
        }

        if ((msgsReceived < messageCount)||(pAckList->DeliveryHdlCount < messageCount))
        {
            if (   (msgsReceived == lastmsgsReceived)
                && (pAckList->DeliveryHdlCount == lasthdlcount))
            {
                retries++;
            }
            else
            {
                //We've made progress...not hung
                retries = 0;
            }

            usleep(200 * 1000); //0.2s

            if (retries > 100) //100 *0.2s = 20s
            {
                printf("No messages received for a long time. Expected %d (hdlcount %d) received: %d\n", messageCount, pAckList->DeliveryHdlCount, msgsReceived);
            }
        }
    }
    while (    (msgsReceived < messageCount)
            || (pAckList->DeliveryHdlCount < messageCount));

    TEST_ASSERT_EQUAL(msgsReceived, messageCount);
    TEST_ASSERT_EQUAL(pAckList->DeliveryHdlCount, messageCount);

    // And this time ack all of the messages
    rc = sync_ism_engine_confirmMessageDeliveryBatch( hConsumerSession
                                               , NULL
                                               , pAckList->DeliveryHdlCount
                                               , pAckList->pDeliveryHdls
                                               , ismENGINE_CONFIRM_OPTION_CONSUMED);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the queue depths to verify that they are zero.
    for (loop=0; loop < numQueues; loop++)
    {
        ieq_getStats(pThreadData,hQueue[loop], &stats);
        ieq_setStats(hQueue[loop], NULL, ieqSetStats_RESET_ALL);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
    }
   
    // Free the delivery handles
    free(pAckList->pDeliveryHdls);

    // Free the messages we allocated
    freeMsgArray( hPubSession, messageCount, msgArray);

    // Cleaup session
    rc = sync_ism_engine_destroySession( hPubSession );
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroySession( hConsumerSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete the queues
    for (loop=0; loop < numQueues; loop++)
    {
        sprintf(queueName, "%s.%u", __func__, loop);
        rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
        TEST_ASSERT_EQUAL(rc, OK);

        verbose(4, "Deleted queue %d", loop);
    }

    return;
}

/*********************************************************************/
/* Name:        Queue2_BatchTranAck                                  */
/* Description: This test function defines N queues. It creates a    */
/*              consumer on each queue (part of the same session)    */
/*              and then puts M messages to each queue.              */
/*              The consumer does not ack the messages seen but adds */
/*              them to the list of messages to be ack'd. Once all   */
/*              messages have been seen the messages are ack'd       */
/*              using the batch ack api.                             */
/* Parameters:                                                       */
/*     type - Type of queue to use simple / intermediate             */
/*     extraQOptions       - Additional ieqOptions values to set     */
/*     numConsumers        - The number of queues                    */
/*     messageCount        - The number of messages to put           */
/*     Persistence         - Message persistance                     */
/*********************************************************************/
static void Queue2_BatchTranAck( ismQueueType_t type
                               , uint32_t extraQOptions
                               , uint32_t numConsumers
                               , bool queuePerConsumer
                               , uint32_t messageCount
                               , uint8_t Persistence )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hPubSession;
    ismEngine_SessionHandle_t hConsumerSession;
    ismQHandle_t hQueue[numConsumers];
    ismEngine_ConsumerHandle_t hConsumer[numConsumers];
    ismEngine_TransactionHandle_t hTran;
    char queueName[32];
    char QueueTypeBuf[16];
    tiqConsumerContext_t CallbackContext[numConsumers];
    tiqAckList *pAckList;
    uint32_t numQueues=1;
    uint32_t loop;
    uint32_t msgsReceived;
    uint32_t bufferedMessages;
    tiqMsg_t *msgArray;
    ismEngine_QueueStatistics_t stats;

    if (queuePerConsumer) numQueues=numConsumers;

    verbose(2, "Put %d messages to %d %s queues drained by %d consumers and then negatively acknowledge them in a batch",
            messageCount,
            numQueues,
            ieut_queueTypeText(type, QueueTypeBuf, sizeof(QueueTypeBuf)),
            numConsumers);

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hPubSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_TRANSACTIONAL
                               , &hConsumerSession
                               , NULL
                               , 0
                               , NULL);

    // Generate the array of messages we will put
    generateMsgArray( hPubSession
                    , Persistence
                    , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                    , messageCount
                    , 32
                    , 16384
                    , &msgArray );

    pAckList = (tiqAckList *)malloc(sizeof(tiqAckList));
    pAckList->DeliveryHdlCount = 0;
    pAckList->DeliveryHdlSize = 0;
    pAckList->pDeliveryHdls = NULL;
    pthread_mutex_init(&(pAckList->lock), NULL);

    // Create the queues and consumers
    for (loop=0; loop < numConsumers; loop++)
    {
        if (queuePerConsumer || (loop == 0))
        {
            rc = ieq_createQ( pThreadData
                            , type
                            , NULL
                            , ieqOptions_DEFAULT | extraQOptions
                            , iepi_getDefaultPolicyInfo(true)
                            , ismSTORE_NULL_HANDLE
                            , ismSTORE_NULL_HANDLE
                            , iereNO_RESOURCE_SET
                            , &hQueue[loop] );
            TEST_ASSERT_EQUAL(rc, OK);

            verbose(4, "Created queue %d", loop);

            // Add the queue to the queue namespace so we can consume from it.
            sprintf(queueName, "%s.%u", __func__, loop);
            rc = ieqn_addQueue(pThreadData,
                               queueName,
                               hQueue[loop],
                               NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            hQueue[loop] = hQueue[loop-1];
        }

        // And create a consumer on the queue
        tiqConsumerContext_t *pCallbackContext = &CallbackContext[loop];
        pCallbackContext->hSession = hConsumerSession;
        pCallbackContext->AckMode = Record;
        pCallbackContext->negAckRatio = 0;
        pCallbackContext->Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
        pCallbackContext->msgsReceived = 0;
        pCallbackContext->msgsAckStarted = 0;
        pCallbackContext->msgsAckCompleted = 0;
        pCallbackContext->msgsNegAckd = 0;
        pCallbackContext->expectedDupCount = 0;
        pCallbackContext->dupMsgs = 0;
        pCallbackContext->pMsgs = msgArray;
        pCallbackContext->Qtype = type;
        pCallbackContext->pAckList = pAckList;

        rc = ism_engine_createConsumer( hConsumerSession
                                      , ismDESTINATION_QUEUE
                                      , queueName
                                      , 0
                                      , NULL // Unused for QUEUE
                                      , &pCallbackContext
                                      , sizeof(pCallbackContext)
                                      , MessageCallback
                                      , NULL
                                      , test_getDefaultConsumerOptionsFromQType(type)
                                      , &hConsumer[loop]
                                      , NULL
                                      , 0
                                      , NULL );
        TEST_ASSERT_EQUAL(rc, OK);

        // Check that, for a temporary queue, no store object was created.
        if (extraQOptions & ieqOptions_TEMPORARY_QUEUE)
        {
            TEST_ASSERT_EQUAL(ieq_getDefnHdl(hQueue[loop]), 0);
        }
    }

    // enable the consumers
    rc = ism_engine_startMessageDelivery( hConsumerSession
                                        , ismENGINE_START_DELIVERY_OPTION_NONE
                                        , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    
    // Now put the messages
    for (loop=0; loop < messageCount; loop++)
    {
        msgArray[loop].Received = false;
        msgArray[loop].Ackd = false;
        msgArray[loop].hDelivery = ismENGINE_NULL_DELIVERY_HANDLE;

        uint32_t numQueue = rand() % numQueues;
        rc = ieq_put( pThreadData
                    , hQueue[numQueue]
                    , ieqPutOptions_NONE
                    , NULL
                    , msgArray[loop].pMessage
                    , IEQ_MSGTYPE_REFCOUNT
                    , NULL );
        TEST_ASSERT_EQUAL(rc, OK);

        verbose(5, "Put message %d", loop);
    }

    //Wait for the messages to arrive
    msgsReceived = 0;
    int retries = 0;

    do
    {
        int lastmsgsReceived = msgsReceived;

        msgsReceived = 0;
        for (loop=0; loop < numConsumers; loop++)
        {
            msgsReceived += CallbackContext[loop].msgsReceived;
        }

        if (msgsReceived < messageCount)
        {
            usleep(200 * 1000); //0.2s

            if (msgsReceived == lastmsgsReceived)
            {
                retries++;
            }
            else
            {
                //We've made progress
                retries=0;
            }

            if (retries > 100) //100 *0.2s = 20s
            {
                printf("No messages received for a long time. Expected %d received: %d\n", messageCount, msgsReceived);
            }
        }
    }
    while (msgsReceived < messageCount);

    TEST_ASSERT_EQUAL(msgsReceived, messageCount);

    // disable the consumers
    rc = ism_engine_stopMessageDelivery( hConsumerSession
                                       , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that all of the messages have arrived
    msgsReceived = 0;
    for (loop=0; loop < numConsumers; loop++)
    {
        msgsReceived += CallbackContext[loop].msgsReceived;
    }
    TEST_ASSERT_EQUAL(msgsReceived, messageCount);
    TEST_ASSERT_EQUAL(pAckList->DeliveryHdlCount, messageCount);

    // Check the queue depths to verify that they still contain the messages
    bufferedMessages = 0;
    for (loop=0; loop < numQueues; loop++)
    {
        ieq_getStats(pThreadData, hQueue[loop], &stats);
        ieq_setStats(hQueue[loop], NULL, ieqSetStats_RESET_ALL);
        bufferedMessages += stats.BufferedMsgs;
    }
    TEST_ASSERT_EQUAL(bufferedMessages, messageCount);

    // Create a transaction to which we are going to add all of the 
    // messages
    rc = sync_ism_engine_createLocalTransaction( hConsumerSession
                                               , &hTran );
    TEST_ASSERT_EQUAL(rc, OK);

    // Ack the messages using the batch api as part of a transaction
    rc = sync_ism_engine_confirmMessageDeliveryBatch( hConsumerSession
                                                   , hTran
                                                   , pAckList->DeliveryHdlCount
                                                   , pAckList->pDeliveryHdls
                                                   , ismENGINE_CONFIRM_OPTION_CONSUMED);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check the queue depths to verify that they still contain the messages
    bufferedMessages = 0;
    for (loop=0; loop < numQueues; loop++)
    {
        ieq_getStats(pThreadData, hQueue[loop], &stats);
        ieq_setStats(hQueue[loop], NULL, ieqSetStats_RESET_ALL);
        bufferedMessages += stats.BufferedMsgs;
    }
    TEST_ASSERT_EQUAL(bufferedMessages, messageCount);

    // Commit the transaction
    rc = sync_ism_engine_commitTransaction( hConsumerSession
                                     , hTran
                                     , ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);
  
    // Check the queue depths to verify that they are zero.
    for (loop=0; loop < numQueues; loop++)
    {
        ieq_getStats(pThreadData, hQueue[loop], &stats);
        ieq_setStats(hQueue[loop], NULL, ieqSetStats_RESET_ALL);
        TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);
    }
   
    // Free the messages we allocated
    freeMsgArray( hPubSession, messageCount, msgArray);

    // Cleaup session
    rc = sync_ism_engine_destroySession( hPubSession);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroySession( hConsumerSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete the queues
    for (loop=0; loop < numQueues; loop++)
    {
        // Add the queue to the queue namespace so we can consume from it.
        sprintf(queueName, "%s.%u", __func__, loop);
        rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
        TEST_ASSERT_EQUAL(rc, OK);

        verbose(4, "Deleted queue %d", loop);
    }

    return;
}


typedef struct tag_tiqRedeliverContext_t {
    ismEngine_SessionHandle_t hSession;
    uint32_t numMsgRcvdExpected;   //< Num msgs callback should expect to receive in received state
    volatile uint32_t numMsgRcvdArrived;    //< Num of above that have arrived so far
    uint32_t numMsgRcvdToConsume;  //< Num of "received" messages we should mark consumed
    volatile uint32_t numMsgRcvdConsumed;   //< Num of above that have arrived so far
    uint32_t numMsgDelivExpected;  //< Num msgs callback should expect to receive in delivered state
    volatile uint32_t numMsgDelivArrived;   //< Num of above that have arrived so far
    uint32_t numMsgDelivToConsume; //< Num of "delivered" messages we should mark consumed
    volatile uint32_t numMsgDelivConsumed;  //< Num of above we've done so far
    uint32_t numMsgDelivToMarkReceived; //< Num of "delivered" messages we should mark received
    volatile uint32_t numMsgDelivMarkedReceived; //< Num of above we've done
    volatile uint32_t numAcksStarted;   //Number of acks (of any type) we have started
    volatile uint32_t numAcksCompleted; //Number of acks (of any type) that have completed
} tiqRedeliverContext_t;

static bool RedeliverReopenCB(ismEngine_ConsumerHandle_t  hConsumer,
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
                              ismEngine_DelivererContext_t * _delivererContext )
{
    tiqRedeliverContext_t *pContext = *(tiqRedeliverContext_t **)pConsumerContext;

    verbose(5, "Received message %s state %u", pAreaData[0], (uint64_t)state);

    //We totally ignore the message body... and assume other tests check that
    //We just want to check we get the correct numbers of received+delivered msgs
    switch(state)
    {
        case ismMESSAGE_STATE_CONSUMED:
            //This test is all about QoS 2...we shouldn't be receiving Qos 0 messages
            TEST_ASSERT(false, ("State is consumed"));
            break;

        case ismMESSAGE_STATE_DELIVERED:
            pContext->numMsgDelivArrived++;
            TEST_ASSERT(pContext->numMsgDelivArrived <= pContext->numMsgDelivExpected, ("numMsgDeliv wrong"));

            //Do we need to consume this message?
            if (pContext->numMsgDelivConsumed < pContext->numMsgDelivToConsume)
            {
                verbose(5, "Marking Message Consumed");

                pContext->numAcksStarted++;
                volatile uint32_t *pAcksCompleted = &(pContext->numAcksCompleted);

                int32_t rc = ism_engine_confirmMessageDelivery(pContext->hSession,
                                                               NULL,
                                                               hDelivery,
                                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                               &pAcksCompleted, sizeof(pAcksCompleted),
                                                               completedAckIncrement);
                TEST_ASSERT_CUNIT((rc == OK) || (rc == ISMRC_AsyncCompletion),
                                  ("rc was %d\n", rc));
                if (rc == OK)
                {
                    completedAckIncrement(rc, NULL, &pAcksCompleted);
                }

                pContext->numMsgDelivConsumed++;
            }
            //Otherwise do we need to mark this message received?
            else if (pContext->numMsgDelivMarkedReceived < pContext->numMsgDelivToMarkReceived)
            {
                verbose(5, "Marking Message Received");

                pContext->numAcksStarted++;
                volatile uint32_t *pAcksCompleted = &(pContext->numAcksCompleted);

                int32_t rc = ism_engine_confirmMessageDelivery(pContext->hSession,
                                                               NULL,
                                                               hDelivery,
                                                               ismENGINE_CONFIRM_OPTION_RECEIVED,
                                                               &pAcksCompleted, sizeof(pAcksCompleted),
                                                               completedAckIncrement);
                TEST_ASSERT_CUNIT((rc == OK) || (rc == ISMRC_AsyncCompletion),
                                                  ("rc was %d\n", rc));
                if (rc == OK)
                {
                    completedAckIncrement(rc, NULL, &pAcksCompleted);
                }

                pContext->numMsgDelivMarkedReceived++;
            }
            break;

        case ismMESSAGE_STATE_RECEIVED:
            pContext->numMsgRcvdArrived++;
            TEST_ASSERT(pContext->numMsgRcvdArrived <= pContext->numMsgRcvdExpected, ("numMsgRcvd wrong"));

            //Do we need to consume this message?
            if (pContext->numMsgRcvdConsumed < pContext->numMsgRcvdToConsume)
            {
                verbose(5, "Marking Message Consumed");
                pContext->numAcksStarted++;
                volatile uint32_t *pAcksCompleted = &(pContext->numAcksCompleted);

                int32_t rc = ism_engine_confirmMessageDelivery(pContext->hSession,
                                                               NULL,
                                                               hDelivery,
                                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                               &pAcksCompleted, sizeof(pAcksCompleted),
                                                               completedAckIncrement);

                TEST_ASSERT_CUNIT((rc == OK) || (rc == ISMRC_AsyncCompletion),
                                                                  ("rc was %d\n", rc));
                if (rc == OK)
                {
                    completedAckIncrement(rc, NULL, &pAcksCompleted);
                }
                pContext->numMsgRcvdConsumed++;
            }
            break;

        default:
            //We don't expect messages in any other state
            TEST_ASSERT(false, ("Unexpected state %d", state));
            break;
    }

    ism_engine_releaseMessage(hMessage);
    return true;
}

/*********************************************************************/
/* Name:        Queue2_RedeliverReopen                               */
/* Description: When a client disconnects and reconnects we need to  */
/* For Qos2 messages:                                                */
/*    (re-)send publish for messages for messages not marked         */
/*    received (== msgs we've not had a pub-rec)                     */
/*    (re-)send any pubrels for messages with have not been consumed */
/*     (==msgs we've not had  a pubcomp for)                         */
/* For Qos1 messages:                                                */
/*    (re-)send any publish for messages for messages not consumed   */
/* Parameters:                                                       */
/*     type                - Type of queue to create                 */
/*     extraQOptions       - Additional ieqOptions values to set     */
/*     Persistence         - Message persistance                     */
/*     MsgReliability      - QOs 1 or Qos 2 msgs                     */
/*********************************************************************/
static void Queue2_RedeliverReopen( ismQueueType_t type
                                  , uint32_t extraQOptions
                                  , uint8_t Persistence
                                  , uint8_t MsgReliability)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismQHandle_t hQueue;
    ismEngine_QueueStatistics_t stats;
    ismEngine_Consumer_t *pConsumer;
    char queueName[32];
    tiqRedeliverContext_t origContext = { 0 };  //Context for the original consumer
    tiqRedeliverContext_t *pOrigContext = &origContext;
    tiqMsg_t *msgArray;

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    origContext.hSession = hSession;

    // Create the queue
    rc = ieq_createQ( pThreadData
                    , type
                    , NULL
                    , ieqOptions_DEFAULT | extraQOptions
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &hQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");

    // Check that, for a temporary queue, no store object was created.
    if (extraQOptions & ieqOptions_TEMPORARY_QUEUE)
    {
        TEST_ASSERT_EQUAL(ieq_getDefnHdl(hQueue), 0);
    }

    // Add the queue to the queue namespace so we can consume from it.
    strcpy(queueName, __func__);
    rc = ieqn_addQueue(pThreadData,
                       queueName,
                       hQueue,
                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // initialise a consumer..they should receive 4 more than the maximum it is allowed to receive
    // (all in delivered state), it should consume 4 and (if Qos 2)mark 4 received )
    origContext.numMsgDelivExpected       = ismEngine_serverGlobal.mqttMsgIdRange + 4;
    origContext.numMsgDelivToConsume      = 4;
    if (MsgReliability == ismMESSAGE_RELIABILITY_EXACTLY_ONCE)
    {
        origContext.numMsgDelivToMarkReceived = 4;
    }

    verbose(4, "Create consumer");
    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pOrigContext
                                  , sizeof(pOrigContext)
                                  , RedeliverReopenCB
                                  , NULL
                                  , test_getDefaultConsumerOptionsFromQType(type)
                                  , &pConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    //Start Message Delivery on the consumer
    rc = ism_engine_startMessageDelivery( hSession,
                                          ismENGINE_START_DELIVERY_OPTION_NONE,
                                          NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Generate the messages to send... we'll send a few more messages than will be allowed
    //to be sent to the current client (as he won't ack many messages)
    uint32_t excessMsgsForOldClient = 5;

    generateMsgArray( hSession
            , Persistence
            , MsgReliability
            , pOrigContext->numMsgDelivExpected + excessMsgsForOldClient
            , 32
            , 16384
            , &msgArray );

    //...and send them, we expect that as we put the messages will
    // be put to the consumer who will mark some consumed then some available
    putMsgArray(hQueue, NULL, pOrigContext->numMsgDelivExpected + excessMsgsForOldClient, msgArray);
    verbose(4, "Put %d messages", pOrigContext->numMsgDelivExpected + excessMsgsForOldClient);

    //Check the messages arrived at the consumer safely
    test_waitForMessages(&(pOrigContext->numMsgDelivArrived), NULL, pOrigContext->numMsgDelivExpected, 20);
    test_waitForMessages(&(pOrigContext->numMsgRcvdArrived), NULL, pOrigContext->numMsgRcvdExpected, 20);
    test_waitForMessages(&(pOrigContext->numAcksCompleted), NULL, pOrigContext->numAcksStarted, 20);

    TEST_ASSERT_EQUAL(pOrigContext->numMsgDelivExpected, pOrigContext->numMsgDelivArrived);
    TEST_ASSERT_EQUAL(pOrigContext->numMsgRcvdExpected,  pOrigContext->numMsgRcvdArrived);

    //Delete the consumer, restart delivery
    rc = ism_engine_destroyConsumer( pConsumer, NULL, 0 , NULL);
    TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
            ("rc was %d\n", rc));

    //Destroy & recreate session so that we get inflight messages redelivered...
    //(even on a multiconsumer queue that has zombie consumers)
    rc = sync_ism_engine_destroySession( hSession);
    TEST_ASSERT(rc == OK, ("ism_engine_destroySession returned %rc",rc));

    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK)

    //Work out what a recreated consumer should receive based on what the original
    //consumer was asked to do.
    tiqRedeliverContext_t newContext = {0};
    newContext.hSession = hSession;
    newContext.numMsgRcvdExpected  =   origContext.numMsgDelivMarkedReceived
                                     + origContext.numMsgRcvdExpected
                                     - origContext.numMsgRcvdToConsume;
    newContext.numMsgRcvdToConsume = newContext.numMsgRcvdExpected;

    newContext.numMsgDelivExpected =   excessMsgsForOldClient
                                     + origContext.numMsgDelivExpected
                                     - origContext.numMsgDelivToConsume
                                     - origContext.numMsgDelivToMarkReceived;
    newContext.numMsgDelivToConsume = newContext.numMsgDelivExpected;

    tiqRedeliverContext_t *pNewContext = &newContext;

    //start message delivery... clear ours any engine session-wide flow control.
    rc = ism_engine_startMessageDelivery( hSession,
                                          ismENGINE_START_DELIVERY_OPTION_NONE,
                                          NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    //Recreate consumer... as message delivery is started on the session, messages will be delivered immediately!
    verbose(4, "Re-create consumer");

    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , queueName
                                  , 0
                                  , NULL // Unused for QUEUE
                                  , &pNewContext
                                  , sizeof(pNewContext)
                                  , RedeliverReopenCB
                                  , NULL
                                  , test_getDefaultConsumerOptionsFromQType(type)
                                  , &pConsumer
                                  , NULL
                                  , 0
                                  , NULL );
    TEST_ASSERT_EQUAL(rc, OK);

    //Check the messages arrived at the consumer safely
    test_waitForMessages(&(pNewContext->numMsgDelivArrived), NULL, pNewContext->numMsgDelivExpected, 20);
    test_waitForMessages(&(pNewContext->numMsgRcvdArrived), NULL, pNewContext->numMsgRcvdExpected, 20);
    test_waitForMessages(&(pNewContext->numAcksCompleted), NULL, pNewContext->numAcksStarted, 20);
    TEST_ASSERT_EQUAL(pNewContext->numMsgDelivExpected, pNewContext->numMsgDelivArrived);
    TEST_ASSERT_EQUAL(pNewContext->numMsgRcvdExpected,  pNewContext->numMsgRcvdArrived);

    rc = ism_engine_destroyConsumer( pConsumer, NULL, 0 , NULL);
    TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                          ("rc was %d\n", rc));

    //Check that the queue is now empty
    ieq_getStats(pThreadData, hQueue, &stats);
    ieq_setStats(hQueue, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(stats.BufferedMsgs, 0);


    //free the messages we've used
    freeMsgArray( hSession, origContext.numMsgDelivExpected, msgArray);

    // Cleaup session
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete Q
    rc = ieqn_destroyQueue(pThreadData, queueName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Deleted queue");

    return;
}


/*********************************************************************/
/*********************************************************************/
/* Testcase definitions                                              */
/*********************************************************************/
/*********************************************************************/





/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_SimpQ_Basic                   */
/* Description:    Basic tests for the simple queue                  */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_SimpQ_M0_C0_NP_PutGet                     */
/* Description:     This testcase tests basic queue operation to     */
/*                  the simple queue.                                */
/* Queue:           Simple                                           */
/* Msg Persistence: Non Persistent                                   */
/* Msg Reliability: QoS 0                                            */
/* Get Reliability: QoS 0                                            */
/* Msg Size:      : 32 - 100K (random)                               */
/*********************************************************************/
void Queue2_SimpQ_M0_C0_NP_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_SimpQ_M0_C0_NP_PutGet...");

    Queue2_PutGet( simple
                 , ieqOptions_NONE
                 , 3072
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_NONPERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE 
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_SimpQ_M0_C0_NP_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_SimpQ_M1_C0_P_PutGet                      */
/* Description:     This testcase tests basic queue operation to     */
/*                  the simple queue.                                */
/* Queue:           Simple                                           */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Get Reliability: QoS 0                                            */
/* Msg Size:      : 32 - 100K (random)                               */
/*********************************************************************/
void Queue2_SimpQ_M1_C0_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_SimpQ_M1_C0_P_PutGet...");

    Queue2_PutGet( simple
                 , ieqOptions_NONE
                 , 3072
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE 
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_SimpQ_M1_C0_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_SimpQ_M2_C0_P_PutGet                      */
/* Description:     This testcase tests basic queue operation to     */
/*                  the simple queue.                                */
/* Queue:           Simple                                           */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Get Reliability: QoS 0                                            */
/* Msg Size:      : 32 - 100K (random)                               */
/*********************************************************************/
void Queue2_SimpQ_M2_C0_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_SimpQ_M2_C0_P_PutGet...");

    Queue2_PutGet( simple
                 , ieqOptions_NONE
                 , 3072
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_NONPERSISTENT
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE 
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_SimpQ_M2_C0_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_SimpQ_M1_P_Put                            */
/* Description:     This testcase loads a queue with messages and    */
/*                  deletes the queue with messages still loaded.    */
/*                  This test is useful to verify we do not leak     */
/*                  memory while deleting a queue with pending msgs  */
/* Queue:           Simple                                           */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_SimpQ_M1_P_Put( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_SimpQ_M1_P_Put...");

    Queue2_Put( simple
              , ieqOptions_NONE
              , 100
              , 32 // Minimum msgsize
              , 4096 // Maximum msgsize
              , ismMESSAGE_PERSISTENCE_PERSISTENT
              , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_SimpQ_M0_P_Put");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_SimpQ_Statistics                          */
/* Description:     This testcase verifies that queue statistics are */
/*                  collected correctly.                             */
/* Queue:           Simple                                           */
/*********************************************************************/
static void Queue2_SimpQ_Statistics( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismEngine_QueueStatistics_t stats;
    ieqnQueue_t *pNamedQueue;
    ismEngine_Transaction_t *pTransaction;
    ismEngine_Transaction_t *pTransactionBack;
    tiqMsg_t *msgArray;

    char queueName[]="Queue2_Simp_Statistics";

    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_SimpQ_Statistics...");

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate an array of messages we will use to send to the queue
    generateMsgArray( hSession
                    , ismMESSAGE_PERSISTENCE_NONPERSISTENT
                    , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                    , 5
                    , 32
                    , 128
                    , &msgArray );

    // Create the queue
    rc = ieqn_createQueue( pThreadData
                         , queueName
                         , simple
                         , ismQueueScope_Server
                         , NULL
                         , NULL
                         , NULL
                         , &pNamedQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");


    // Query the statistics for the queue
    verbose(3, "Verify statistics are zero to start");
    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(0, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(0, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 1 - Put 5 message to the queue and check statistics
    verbose(3, "Put 5 messages and verify BufferedMsgs/HWM and EnqueueCount");
    putMsgArray(pNamedQueue->queueHandle, NULL, 5, msgArray);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(5, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);
    
    // TEST 2 - Put 5 messages within a transaction
    verbose(3, "Put 5 messages within transaction and verify InflightEnqs");

    // Create the transaction
    rc = ietr_createLocal( pThreadData,
                           NULL,
                           false,
                           false,
						   NULL,
                           &pTransactionBack );
    TEST_ASSERT_EQUAL(rc, OK);
    putMsgArray(pNamedQueue->queueHandle, pTransactionBack, 5, msgArray);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 3 - Rollback the 2nd transaction
    verbose(3, "Rollback the 2nd transcation and verify BufferedMsgs");
    rc = ietr_rollback(pThreadData, pTransactionBack, NULL, IETR_ROLLBACK_OPTIONS_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 4 - Put 5 messages within a transaction
    verbose(3, "Put 5 messages within transaction and verify InflightEnqs");

    // Create the transaction
    rc = ietr_createLocal( pThreadData,
                           NULL,
                           false,
                           false,
						   NULL,
                           &pTransaction );
    TEST_ASSERT_EQUAL(rc, OK);
    putMsgArray(pNamedQueue->queueHandle, pTransaction, 5, msgArray);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 5 - Completely consume 3 messages from the queue
    verbose(3, "Consume 3 messages from the queue and verify BufferedMsgs, DequeueCount");
    receiveMsgs(hSession, queueName, simple, 3, ismENGINE_CONFIRM_OPTION_CONSUMED, NULL);
    
    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(2, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(3, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 6 - Commit the transaction
    verbose(3, "Commit the transaction and verify BufferedMsgs/HWM and EnqueueCount");
    rc = ietr_commit(pThreadData, pTransaction, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(7, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(7, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(5, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);

    // TEST 7 - Completely consume the remaining messages from the queue
    verbose(3, "Consume all messages and verify BufferedMsgs/HWM and DequeueCount");
    receiveMsgs(hSession, queueName, simple, 7, ismENGINE_CONFIRM_OPTION_CONSUMED, NULL);
    
    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(0, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(7, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(7, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    verbose(4, "Completed Queue2_SimpQ_Statistics");

    return;
}


CU_TestInfo ISM_Engine_CUnit_Q2_SimpQ_Basic[] = {
    { "Queue2_SimpQ_M0_C0_NP_PutGet",     Queue2_SimpQ_M0_C0_NP_PutGet },
    { "Queue2_SimpQ_M1_C0_P_PutGet",      Queue2_SimpQ_M1_C0_P_PutGet },
    { "Queue2_SimpQ_M2_C0_P_PutGet",      Queue2_SimpQ_M2_C0_P_PutGet },
    { "Queue2_SimpQ_M1_P_Put",            Queue2_SimpQ_M1_P_Put },
    { "Queue2_SimpQ_Statistics",          Queue2_SimpQ_Statistics },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_SimpQ_Advanced                */
/* Description:    Advanced tests for the simple queue               */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_SimpQ_P_0_Commit                          */
/* Description:     This testcase tests commit operation for QoS 0   */
/*                  producer on the simple queue.                    */
/* Queue:           Simple                                           */
/* Msg Persistence: Persistent                                       */
/*********************************************************************/
void Queue2_SimpQ_P_0_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_SimpQ_P_0_Commit...");

    Queue2_Commit( simple
                 , ieqOptions_NONE
                 , 5
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE );

    verbose(4, "Completed Queue2_SimpQ_P_0_Commit");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_SimpQ_P_0_Rollback                        */
/* Description:     This testcase tests rollback operation for QoS 0 */
/*                  producer on the simple queue.                    */
/* Queue:           Simple                                           */
/* Msg Persistence: Persistent                                       */
/*********************************************************************/
void Queue2_SimpQ_P_0_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_SimpQ_P_0_Rollback...");

    Queue2_Rollback( simple
                   , ieqOptions_NONE
                   , 5
                   , ismMESSAGE_PERSISTENCE_PERSISTENT
                   , ismMESSAGE_RELIABILITY_AT_MOST_ONCE );

    verbose(4, "Completed Queue2_SimpQ_P_0_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_SimpQ_EnableDisable                       */
/* Description:     This testcase tests enable/disable operation to  */
/*                  the simple queue.                                */
/* Queue:           Simple                                           */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_SimpQ_EnableDisable( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_SimpQ_EnableDisable...");

    Queue2_EnableDisable( simple
                        , ieqOptions_NONE
                        , 5     // Number of putting threads
                        , 10000  // Number of enable/disable operations
                        , 0 // Minimum msgsize
                        , 102400 ); // Maximum msgsize

    verbose(4, "Completed Queue2_SimpQ_EnableDisable");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_SimpQ_DrainQ                              */
/* Description:     This testcase tests loads a queue with messages  */
/*                  before draining the messages from the queue.     */
/* Queue:           Simple                                           */
/* Msg Persistence: Persistent                                       */
/*********************************************************************/
void Queue2_SimpQ_DrainQ( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_SimpQ_DrainQ...");

    Queue2_DrainQ( simple
                 , ieqOptions_NONE
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE );

    verbose(4, "Completed Queue2_SimpQ_DrainQ");

    return;
}

CU_TestInfo ISM_Engine_CUnit_Q2_SimpQ_Advanced[] = {
    { "Queue2_SimpQ_P_0_Commit",          Queue2_SimpQ_P_0_Commit },
    { "Queue2_SimpQ_P_0_Rollback",        Queue2_SimpQ_P_0_Rollback },
    { "Queue2_SimpQ_EnableDisable",       Queue2_SimpQ_EnableDisable },
    { "Queue2_SimpQ_DrainQ",              Queue2_SimpQ_DrainQ },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_InterQ_Basic                  */
/* Description:    Basic tests for the intermediate queue            */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_InterQ_M0_C1_P_PutGet                     */
/* Description:     This testcase tests basic queue operation to     */
/*                  the intermediate queue.                          */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 0                                            */
/* Get Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_M0_C1_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_M0_C1_P_PutGet...");

    Queue2_PutGet( intermediate
                 , ieqOptions_NONE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_InterQ_M0_C1_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_M0_C1_P_PutGetDA                   */
/* Description:     This testcase tests queue operation to           */
/*                  the intermediate queue, double acking every msg  */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Get Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/* Double Ack:    : every message                                    */
/*********************************************************************/
void Queue2_InterQ_M2_C2_P_PutGetDA( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_InterQ_M2_C2_P_PutGetDA...");

    Queue2_PutGet( intermediate
                 , ieqOptions_NONE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                 , 1);

    verbose(4, "Completed Queue2_InterQ_M2_C2_P_PutGetDA");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_M0_C2_P_PutGet                     */
/* Description:     This testcase tests basic queue operation to     */
/*                  the intermediate queue.                          */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 0                                            */
/* Get Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_M0_C2_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_M0_C2_P_PutGet...");

    Queue2_PutGet( intermediate
                 , ieqOptions_NONE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                 , 0);

    verbose(4, "Completed Queue2_InterQ_M0_C2_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_M1_C1_P_PutGet                     */
/* Description:     This testcase tests basic queue operation to     */
/*                  the intermediate queue.                          */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Get Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_M1_C1_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_M1_C1_P_PutGet...");

    Queue2_PutGet( intermediate
                 , ieqOptions_NONE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_InterQ_M1_C1_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_M1_C2_P_PutGet                     */
/* Description:     This testcase tests basic queue operation to     */
/*                  the intermediate queue.                          */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Get Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_M1_C2_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_M1_C2_P_PutGet...");

    Queue2_PutGet( intermediate
                 , ieqOptions_NONE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                 , 0);

    verbose(4, "Completed Queue2_InterQ_M1_C2_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_M2_C1_P_PutGet                     */
/* Description:     This testcase tests basic queue operation to     */
/*                  the intermediate queue.                          */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Get Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_M2_C1_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_M2_C1_P_PutGet...");

    Queue2_PutGet( intermediate
                 , ieqOptions_NONE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_InterQ_M2_C1_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_M2_C2_P_PutGet                     */
/* Description:     This testcase tests basic queue operation to     */
/*                  the intermediate queue.                          */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Get Reliability: QoS 0                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_M2_C2_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_M2_C2_P_PutGet...");

    Queue2_PutGet( intermediate
                 , ieqOptions_NONE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                 , 0);

    verbose(4, "Completed Queue2_InterQ_M2_C2_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_M1_P_Put                           */
/* Description:     This testcase loads a queue with messages and    */
/*                  deletes the queue with messages still loaded.    */
/*                  This test is useful to verify we do not leak     */
/*                  memory while deleting a queue with pending msgs  */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_M1_P_Put( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_M0_P_Put...");

    Queue2_Put( intermediate
              , ieqOptions_NONE
              , 100
              , 32 // Minimum msgsize
              , 4096 // Maximum msgsize
              , ismMESSAGE_PERSISTENCE_PERSISTENT
              , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_InterQ_M1_P_Put");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_Statistics                         */
/* Description:     This testcase verifies that queue statistics are */
/*                  collected correctly.                             */
/* Queue:           Intermediate                                     */
/*********************************************************************/
static void Queue2_InterQ_Statistics( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismEngine_QueueStatistics_t stats;
    ieqnQueue_t *pNamedQueue;
    ismEngine_Transaction_t *pTransaction;
    ismEngine_Transaction_t *pTransactionBack;
    tiqMsg_t *msgArray;

    char queueName[]="Queue2_InterQ_Statistics";

    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_Statistics...");

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate an array of messages we will use to send to the queue
    generateMsgArray( hSession
                    , ismMESSAGE_PERSISTENCE_NONPERSISTENT
                    , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                    , 5
                    , 32
                    , 128
                    , &msgArray );

    // Create the queue
    rc = ieqn_createQueue( pThreadData
                         , queueName
                         , intermediate
                         , ismQueueScope_Server
                         , NULL
                         , NULL
                         , NULL
                         , &pNamedQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");

    // Query the statistics for the queue
    verbose(3, "Verify statistics are zero to start");
    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(0, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(0, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 1 - Put 5 message to the queue and check statistics
    verbose(3, "Put 5 messages and verify BufferedMsgs/HWM and EnqueueCount");
    putMsgArray(pNamedQueue->queueHandle, NULL, 5, msgArray);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(5, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 2 - Put 5 messages within a transaction
    verbose(3, "Put 5 messages within transaction and verify InflightEnqs");

    // Create the transaction
    rc = ietr_createLocal( pThreadData,
                           NULL,
                           false,
                           false,
						   NULL,
                           &pTransactionBack );
    TEST_ASSERT_EQUAL(rc, OK);
    putMsgArray(pNamedQueue->queueHandle, pTransactionBack, 5, msgArray);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(5, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 3 - Rollback the 2nd transaction
    verbose(3, "Rollback the 2nd transcation and verify BufferedMsgs");
    rc = ietr_rollback(pThreadData, pTransactionBack, NULL, IETR_ROLLBACK_OPTIONS_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);
    
    // TEST 4 - Put 5 messages within a transaction
    verbose(3, "Put 5 messages within transaction and verify InflightEnqs");

    // Create the transaction
    rc = ietr_createLocal( pThreadData,
                           NULL,
                           false,
                           false,
						   NULL,
                           &pTransaction );
    TEST_ASSERT_EQUAL(rc, OK);
    putMsgArray(pNamedQueue->queueHandle, pTransaction, 5, msgArray);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(5, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 5 - Receive 3 messages from the queue
    verbose(3, "Consume 3 messages from the queue and verify BufferedMsgs and InflightDeqs");
    receiveMsgs(hSession, queueName, intermediate, 3, ismENGINE_CONFIRM_OPTION_RECEIVED, NULL);
    
    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(5, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(3, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    //Destroy & recreate session so that we get inflight messages redelivered...
    //(even on a multiconsumer queue that has zombie consumers)
    rc = sync_ism_engine_destroySession( hSession );
    TEST_ASSERT(rc == OK, ("ism_engine_destroySession returned %rc",rc));

    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK)

    // TEST 6 - Completely consume the remaining messages from the queue
    verbose(3, "Commit the transaction and verify BufferedMsgs/HWM and EnqueueCount");
    rc = ietr_commit(pThreadData, pTransaction, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    receiveMsgs(hSession, queueName, intermediate, 7, ismENGINE_CONFIRM_OPTION_CONSUMED, NULL);
    
    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(3, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(5, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(7, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    verbose(4, "Completed Queue2_InterQ_Statistics");

    return;
}


CU_TestInfo ISM_Engine_CUnit_Q2_InterQ_Basic[] = {
    { "Queue2_InterQ_M0_C1_P_PutGet",     Queue2_InterQ_M0_C1_P_PutGet },
    { "Queue2_InterQ_M2_C2_P_PutGetDA",    Queue2_InterQ_M2_C2_P_PutGetDA },
    { "Queue2_InterQ_M0_C2_P_PutGet",     Queue2_InterQ_M0_C2_P_PutGet },
    { "Queue2_InterQ_M1_C1_P_PutGet",     Queue2_InterQ_M1_C1_P_PutGet },
    { "Queue2_InterQ_M1_C2_P_PutGet",     Queue2_InterQ_M1_C2_P_PutGet },
    { "Queue2_InterQ_M2_C1_P_PutGet",     Queue2_InterQ_M2_C1_P_PutGet },
    { "Queue2_InterQ_M2_C2_P_PutGet",     Queue2_InterQ_M2_C2_P_PutGet },
    { "Queue2_InterQ_M1_P_Put",           Queue2_InterQ_M1_P_Put },
    { "Queue2_InterQ_Statistics",         Queue2_InterQ_Statistics },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_InterQ_Advanced               */
/* Description:    Advanced tests for the intermediate queue         */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_0_Commit                         */
/* Description:     This testcase tests commit operation for QoS 0   */
/*                  producer on the intermediate queue.              */
/* Queue:           Intermediate                                     */
/*********************************************************************/
void Queue2_InterQ_P_0_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_P_0_Commit...");

    Queue2_Commit( intermediate
                 , ieqOptions_NONE
                 , 5
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_0_Commit");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_1_Commit                         */
/* Description:     This testcase tests commit operation for QoS 1   */
/*                  producer on the intermediate queue.              */
/* Queue:           Intermediate                                     */
/*********************************************************************/
void Queue2_InterQ_P_1_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_P_1_Commit...");

    Queue2_Commit( intermediate
                 , ieqOptions_NONE
                 , 5
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_1_Commit");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_2_Commit                         */
/* Description:     This testcase tests commit operation for QoS 2   */
/*                  producer on the intermediate queue.              */
/* Queue:           Intermediate                                     */
/*********************************************************************/
void Queue2_InterQ_P_2_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_P_2_Commit...");

    Queue2_Commit( intermediate
                 , ieqOptions_NONE
                 , 5
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_2_Commit");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_0_Rollback                       */
/* Description:     This testcase tests rollback operation for QoS 0 */
/*                  producer on the intermediate queue.              */
/* Queue:           Intermediate                                     */
/*********************************************************************/
void Queue2_InterQ_P_0_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_P_0_Rollback...");

    Queue2_Rollback( intermediate
                   , ieqOptions_NONE
                   , 5
                   , ismMESSAGE_PERSISTENCE_PERSISTENT
                   , ismMESSAGE_RELIABILITY_AT_MOST_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_0_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_1_Rollback                       */
/* Description:     This testcase tests rollback operation for QoS 1 */
/*                  producer on the intermediate queue.              */
/* Queue:           Intermediate                                     */
/*********************************************************************/
void Queue2_InterQ_P_1_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_P_1_Rollback...");

    Queue2_Rollback( intermediate
                   , ieqOptions_NONE
                   , 5
                   , ismMESSAGE_PERSISTENCE_PERSISTENT
                   , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_1_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_2_Rollback                       */
/* Description:     This testcase tests rollback operation for QoS 2 */
/*                  producer on the intermediate queue.              */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/*********************************************************************/
void Queue2_InterQ_P_2_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_P_2_Rollback...");

    Queue2_Rollback( intermediate
                   , ieqOptions_NONE
                   , 5
                   , ismMESSAGE_PERSISTENCE_PERSISTENT
                   , ismMESSAGE_RELIABILITY_EXACTLY_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_2_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_EnableDisable                      */
/* Description:     This testcase tests enable/disable operation to  */
/*                  the intermediate queue.                          */
/* Queue:           Intermediate                                     */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_EnableDisable( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_InterQ_EnableDisable...");

    Queue2_EnableDisable( intermediate
                        , ieqOptions_NONE
                        , 5     // Number of putting threads
                        , 10000  // Number of enable/disable operations
                        , 0 // Minimum msgsize
                        , 102400 ); // Maximum msgsize

    verbose(4, "Completed Queue2_InterQ_EnableDisable");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_DrainQ                             */
/* Description:     This testcase tests loads a queue with messages  */
/*                  before draining the messages from the queue.     */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/*********************************************************************/
void Queue2_InterQ_DrainQ( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_DrainQ...");

    Queue2_DrainQ( intermediate
                 , ieqOptions_NONE
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_InterQ_DrainQ");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_DrainQFail                         */
/* Description:     This testcase tests loads a queue with messages  */
/*                  before attempting to drain messages from the     */
/*                  queue. The drain should fail as their are        */
/*                  unacknowledged messages.                         */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/*********************************************************************/
void Queue2_InterQ_DrainQAdv( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_DrainQAdv...");

    Queue2_DrainQAdv( intermediate
                    , ieqOptions_NONE
                    , ismMESSAGE_PERSISTENCE_PERSISTENT
                    , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_InterQ_DrainQAdv");

    return;
}

CU_TestInfo ISM_Engine_CUnit_Q2_InterQ_Advanced[] = {
    { "Queue2_InterQ_P_0_Commit",         Queue2_InterQ_P_0_Commit },
    { "Queue2_InterQ_P_1_Commit",         Queue2_InterQ_P_1_Commit },
    { "Queue2_InterQ_P_2_Commit",         Queue2_InterQ_P_2_Commit },
    { "Queue2_InterQ_P_0_Rollback",       Queue2_InterQ_P_0_Rollback },
    { "Queue2_InterQ_P_1_Rollback",       Queue2_InterQ_P_1_Rollback },
    { "Queue2_InterQ_P_2_Rollback",       Queue2_InterQ_P_2_Rollback },
    { "Queue2_InterQ_EnableDisable",      Queue2_InterQ_EnableDisable },
    { "Queue2_InterQ_DrainQ",             Queue2_InterQ_DrainQ },
    { "Queue2_InterQ_DrainQAdv",          Queue2_InterQ_DrainQAdv },
    CU_TEST_INFO_NULL
};


/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_InterQ_Confirm                */
/* Description:    Confirm tests for the intermediate queue          */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_1_Nack                           */
/* Description:     This testcase tests the redelilvery of messages  */
/*                  when were NACK'd                                 */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_P_1_Nack( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_P_1_Nack...");

    Queue2_Nack( intermediate
               , ieqOptions_NONE
               , 512
               , ismMESSAGE_PERSISTENCE_PERSISTENT
               , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_1_Nack");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_2_Nack                           */
/* Description:     This testcase tests the redelilvery of messages  */
/*                  when were NACK'd                                 */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_P_2_Nack( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_P_2_Nack...");

    Queue2_Nack( intermediate
               , ieqOptions_NONE
               , 512
               , ismMESSAGE_PERSISTENCE_PERSISTENT
               , ismMESSAGE_RELIABILITY_EXACTLY_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_2_Nack");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_1_NackActive                     */
/* Description:     This testcase tests the redelilvery of messages  */
/*                  when were NACK'd.Msgs are put while consumer     */
/*                  is active.                                       */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_P_1_NackActive( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_P_1_NackActive...");

    Queue2_NackActive( intermediate 
                     , ieqOptions_NONE
                     , 512
                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                     , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_1_NackActive");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_2_Nack                           */
/* Description:     This testcase tests the redelilvery of messages  */
/*                  when were NACK'd                                 */
/*                  when were NACK'd.Msgs are put while consumer     */
/*                  is active.                                       */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_P_2_NackActive( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_InterQ_P_2_NackActive...");

    Queue2_NackActive( intermediate
                     , ieqOptions_NONE
                     , 512
                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_2_NackActive");

    return;
}

CU_TestInfo ISM_Engine_CUnit_Q2_InterQ_Confirm[] = {
    { "Queue2_InterQ_P_1_Nack",           Queue2_InterQ_P_1_Nack },
    { "Queue2_InterQ_P_2_Nack",           Queue2_InterQ_P_2_Nack },
    { "Queue2_InterQ_P_1_NackActive",     Queue2_InterQ_P_1_NackActive },
    { "Queue2_InterQ_P_2_NackActive",     Queue2_InterQ_P_2_NackActive },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_InterQ_Reopen                 */
/* Description:    Reopen and Redelivery tests for the intermediate  */
/*                 queue                                             */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_1_RedeliverReopen                */
/* Description:     This testcase tests the redelivery of messages   */
/*                  when the queue is reopened                       */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_P_1_RedeliverReopen( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_InterQ_P_1_RedeliverReopen...");

    Queue2_RedeliverReopen( intermediate
                          , ieqOptions_NONE
                          , ismMESSAGE_PERSISTENCE_PERSISTENT
                          , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_1_RedeliverReopen");

    return;
}
/*********************************************************************/
/* TestCase:        Queue2_InterQ_P_2_RedeliverReopen                */
/* Description:     This testcase tests the redelivery of messages   */
/*                  when the queue is reopened                       */
/* Queue:           Intermediate                                     */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_InterQ_P_2_RedeliverReopen( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_InterQ_P_2_RedeliverReopen...");

    Queue2_RedeliverReopen( intermediate
                          , ieqOptions_NONE
                          , ismMESSAGE_PERSISTENCE_PERSISTENT
                          , ismMESSAGE_RELIABILITY_EXACTLY_ONCE );

    verbose(4, "Completed Queue2_InterQ_P_2_RedeliverReopen");

    return;
}

CU_TestInfo ISM_Engine_CUnit_Q2_InterQ_Reopen[] = {
    { "Queue2_InterQ_P_1_RedeliverReopen",  Queue2_InterQ_P_1_RedeliverReopen },
    { "Queue2_InterQ_P_2_RedeliverReopen",  Queue2_InterQ_P_2_RedeliverReopen },
    CU_TEST_INFO_NULL
};


/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_MultiCQ_Basic                 */
/* Description:    Basic tests for the MultiConsumer queue           */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_M0_P_PutGet                       */
/* Description:     This testcase tests basic queue operation to     */
/*                  the multiConsumer queue.                         */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 0                                            */
/* Get Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_MultiCQ_M0_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_M0_C1_P_PutGet...");

    Queue2_PutGet( multiConsumer
                 , ieqOptions_NONE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_MultiCQ_M0_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_M1_P_PutGet                       */
/* Description:     This testcase tests basic queue operation to     */
/*                  the multiConsumer queue.                         */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Get Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_MultiCQ_M1_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_M1_C1_P_PutGet...");

    Queue2_PutGet( multiConsumer
                 , ieqOptions_NONE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_MultiCQ_M1_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_M2_P_PutGet                       */
/* Description:     This testcase tests basic queue operation to     */
/*                  the multiConsumer queue.                         */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Get Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_MultiCQ_M2_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_M2_C1_P_PutGet...");

    Queue2_PutGet( multiConsumer
                 , ieqOptions_NONE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_MultiCQ_M2_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_M1_P_Put                          */
/* Description:     This testcase loads a queue with messages and    */
/*                  deletes the queue with messages still loaded.    */
/*                  This test is useful to verify we do not leak     */
/*                  memory while deleting a queue with pending msgs  */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_MultiCQ_M1_P_Put( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_M0_P_Put...");

    Queue2_Put( multiConsumer
              , ieqOptions_NONE
              , 100
              , 32 // Minimum msgsize
              , 4096 // Maximum msgsize
              , ismMESSAGE_PERSISTENCE_PERSISTENT
              , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_M1_P_Put");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_Statistics                        */
/* Description:     This testcase verifies that queue statistics are */
/*                  collected correctly.                             */
/* Queue:           MultiConsumer                                    */
/*********************************************************************/
static void Queue2_MultiCQ_Statistics( void )
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismEngine_SessionHandle_t hSession;
    ismEngine_QueueStatistics_t stats;
    ieqnQueue_t *pNamedQueue;
    ismEngine_Transaction_t *pPutTransaction;
    ismEngine_Transaction_t *pPutTransactionBack;
    ismEngine_Transaction_t *pGetTransaction;
    tiqMsg_t *msgArray;

    char queueName[]="Queue2_MultiCQ_Statistics";

    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_Statistics...");

    // Create Session for this test
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_TRANSACTIONAL
                               , &hSession
                               , NULL
                               , 0
                               , NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Generate an array of messages we will use to send to the queue
    generateMsgArray( hSession
                    , ismMESSAGE_PERSISTENCE_NONPERSISTENT
                    , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                    , 5
                    , 32
                    , 128
                    , &msgArray );

    // Create the queue
    rc = ieqn_createQueue( pThreadData
                         , queueName
                         , multiConsumer
                         , ismQueueScope_Server
                         , NULL
                         , NULL
                         , NULL
                         , &pNamedQueue );
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(4, "Created queue");

    // Query the statistics for the queue
    verbose(3, "Verify statistics are zero to start");
    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(0, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(0, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 1 - Put 5 message to the queue and check statistics
    verbose(3, "Put 5 messages and verify BufferedMsgs/HWM and EnqueueCount");
    putMsgArray(pNamedQueue->queueHandle, NULL, 5, msgArray);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(5, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 2 - Put 5 messages within a transaction
    verbose(3, "Put 5 messages within transaction and verify InflightEnqs");

    // Create the transaction
    rc = ietr_createLocal( pThreadData,
                           NULL,
                           false,
                           false,
						   NULL,
                           &pPutTransactionBack );
    TEST_ASSERT_EQUAL(rc, OK);
    putMsgArray(pNamedQueue->queueHandle, pPutTransactionBack, 5, msgArray);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(5, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 3 - Rollback the 2nd transaction
    verbose(3, "Rollback the 2nd transcation and verify BufferedMsgs");
    rc = ietr_rollback(pThreadData, pPutTransactionBack, NULL, IETR_ROLLBACK_OPTIONS_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(5, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);
    
    
    // TEST 2 - Put 5 messages within a transaction
    verbose(3, "Put 5 messages within transaction and verify InflightEnqs");

    // Create the transaction
    rc = ietr_createLocal( pThreadData,
                           NULL,
                           false,
                           false,
						   NULL,
                           &pPutTransaction );
    TEST_ASSERT_EQUAL(rc, OK);
    putMsgArray(pNamedQueue->queueHandle, pPutTransaction, 5, msgArray);
    verbose(4, "Put %d messages", 5);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(5, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 3 - Wiithin a transaction consume 3 messages from the queue
    verbose(3, "Consume 3 messages from the queue and verify BufferedMsgs and InflightDeqs");

    // Create the transaction
    rc = ietr_createLocal( pThreadData,
                           NULL,
                           false,
                           false,
						   NULL,
                           &pGetTransaction );
    TEST_ASSERT_EQUAL(rc, OK);

    receiveMsgs(hSession, queueName, multiConsumer, 3, ismENGINE_CONFIRM_OPTION_CONSUMED, pGetTransaction);
    
    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(5, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(3, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 4 - Commit the consuming transcation
    verbose(3, "Commit the consuming transaction and verify BufferedMsgs/HWM and EnqueueCount");
    rc = ietr_commit(pThreadData, pGetTransaction, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(7, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(10, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(5, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(3, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);


    // TEST 5 - Commit the putting transaction
    verbose(3, "Commit the put-transaction and verify BufferedMsgs/HWM and EnqueueCount");
    rc = ietr_commit(pThreadData, pPutTransaction, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    
    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(7, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(7, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(5, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 6 - Wiithin a transaction consume 4 messages from the queue
    verbose(3, "Consume 4 messages from the queue and verify BufferedMsgs and InflightDeqs");

    // Create the transaction
    rc = ietr_createLocal( pThreadData,
                           NULL,
                           false,
                           false,
						   NULL,
                           &pGetTransaction );
    TEST_ASSERT_EQUAL(rc, OK);

    receiveMsgs(hSession, queueName, multiConsumer, 4, ismENGINE_CONFIRM_OPTION_CONSUMED, pGetTransaction);
    
    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(7, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(7, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(4, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 7 - Rollback the consuming transcation
    verbose(3, "Rollback the consuming transaction and verify BufferedMsgs/HWM and EnqueueCount");
    rc = ietr_rollback(pThreadData, pGetTransaction, NULL, IETR_ROLLBACK_OPTIONS_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(7, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(7, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(0, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    // TEST 8 - Completely consume the remaining messages from the queue
    verbose(3, "Verify BufferedMsgs/HWM and EnqueueCount");
    receiveMsgs(hSession, queueName, multiConsumer, 7, ismENGINE_CONFIRM_OPTION_CONSUMED, NULL);
    
    ieq_getStats(pThreadData, pNamedQueue->queueHandle, &stats);
    ieq_setStats(pNamedQueue->queueHandle, NULL, ieqSetStats_RESET_ALL);
    TEST_ASSERT_EQUAL(0, stats.BufferedMsgs);
    TEST_ASSERT_EQUAL(7, stats.BufferedMsgsHWM);
    TEST_ASSERT_EQUAL(0, stats.RejectedMsgs);
    TEST_ASSERT_EQUAL(0, stats.InflightEnqs);
    TEST_ASSERT_EQUAL(0, stats.InflightDeqs);
    TEST_ASSERT_EQUAL(0, stats.EnqueueCount);
    TEST_ASSERT_EQUAL(7, stats.DequeueCount);
    TEST_ASSERT_EQUAL(0, stats.QAvoidCount);
    TEST_ASSERT_EQUAL(5000, stats.MaxMessages);

    verbose(4, "Completed Queue2_MultiCQ_Statistics");

    return;
}

CU_TestInfo ISM_Engine_CUnit_Q2_MultiCQ_Basic[] = {
    { "Queue2_MultiCQ_M0_P_PutGet",       Queue2_MultiCQ_M0_P_PutGet },
    { "Queue2_MultiCQ_M1_P_PutGet",       Queue2_MultiCQ_M1_P_PutGet },
    { "Queue2_MultiCQ_M2_P_PutGet",       Queue2_MultiCQ_M2_P_PutGet },
    { "Queue2_MultiCQ_M1_P_Put",          Queue2_MultiCQ_M1_P_Put },
    { "Queue2_MultiCQ_Statistics",        Queue2_MultiCQ_Statistics },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_MultiCQ_Advanced              */
/* Description:    Advanced tests for the multiConsumer queue        */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_0_Commit                        */
/* Description:     This testcase tests commit operation for QoS 0   */
/*                  producer on the multiConsumer queue.             */
/* Queue:           MultiConsumer                                    */
/*********************************************************************/
void Queue2_MultiCQ_P_0_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_P_0_Commit...");

    Queue2_Commit( multiConsumer
                 , ieqOptions_NONE
                 , 5
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_0_Commit");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_1_Commit                        */
/* Description:     This testcase tests commit operation for QoS 1   */
/*                  producer on the multiConsumer queue.             */
/* Queue:           MultiConsumer                                    */
/*********************************************************************/
void Queue2_MultiCQ_P_1_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_P_1_Commit...");

    Queue2_Commit( multiConsumer
                 , ieqOptions_NONE
                 , 5
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_1_Commit");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_2_Commit                        */
/* Description:     This testcase tests commit operation for QoS 2   */
/*                  producer on the multiConsumer queue.             */
/* Queue:           MultiConsumer                                    */
/*********************************************************************/
void Queue2_MultiCQ_P_2_Commit( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_P_2_Commit...");

    Queue2_Commit( multiConsumer
                 , ieqOptions_NONE
                 , 5
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_2_Commit");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_0_Rollback                      */
/* Description:     This testcase tests rollback operation for QoS 0 */
/*                  producer on the multiConsumer queue.             */
/* Queue:           MultiConsumer                                    */
/*********************************************************************/
void Queue2_MultiCQ_P_0_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_P_0_Rollback...");

    Queue2_Rollback( multiConsumer
                   , ieqOptions_NONE
                   , 5
                   , ismMESSAGE_PERSISTENCE_PERSISTENT
                   , ismMESSAGE_RELIABILITY_AT_MOST_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_0_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_1_Rollback                      */
/* Description:     This testcase tests rollback operation for QoS 1 */
/*                  producer on the multiConsumer queue.             */
/* Queue:           MultiConsumer                                    */
/*********************************************************************/
void Queue2_MultiCQ_P_1_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_P_1_Rollback...");

    Queue2_Rollback( multiConsumer
                   , ieqOptions_NONE
                   , 5
                   , ismMESSAGE_PERSISTENCE_PERSISTENT
                   , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_1_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_2_Rollback                      */
/* Description:     This testcase tests rollback operation for QoS 2 */
/*                  producer on the multiConsumer queue.             */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/*********************************************************************/
void Queue2_MultiCQ_P_2_Rollback( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_P_2_Rollback...");

    Queue2_Rollback( multiConsumer
                   , ieqOptions_NONE
                   , 5
                   , ismMESSAGE_PERSISTENCE_PERSISTENT
                   , ismMESSAGE_RELIABILITY_EXACTLY_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_2_Rollback");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_EnableDisable                     */
/* Description:     This testcase tests enable/disable operation to  */
/*                  the multiConsumer queue.                         */
/* Queue:           MultiConsumer                                    */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_MultiCQ_EnableDisable( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_MultiCQ_EnableDisable...");

    Queue2_EnableDisable( multiConsumer
                        , ieqOptions_NONE
                        , 5     // Number of putting threads
                        , 10000  // Number of enable/disable operations
                        , 0 // Minimum msgsize
                        , 102400 ); // Maximum msgsize

    verbose(4, "Completed Queue2_MultiCQ_EnableDisable");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_DrainQ                            */
/* Description:     This testcase tests loads a queue with messages  */
/*                  before draining the messages from the queue.     */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/*********************************************************************/
void Queue2_MultiCQ_DrainQ( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_DrainQ...");

    Queue2_DrainQ( multiConsumer
                 , ieqOptions_NONE
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_DrainQ");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_DrainQFail                        */
/* Description:     This testcase tests loads a queue with messages  */
/*                  before attempting to drain messages from the     */
/*                  queue. The drain should fail as their are        */
/*                  unacknowledged messages.                         */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/*********************************************************************/
void Queue2_MultiCQ_DrainQAdv( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_DrainQAdv...");

    Queue2_DrainQAdv( multiConsumer
                    , ieqOptions_NONE
                    , ismMESSAGE_PERSISTENCE_PERSISTENT
                    , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_DrainQAdv");

    return;
}


CU_TestInfo ISM_Engine_CUnit_Q2_MultiCQ_Advanced[] = {
    { "Queue2_MultiCQ_P_0_Commit",        Queue2_MultiCQ_P_0_Commit },
    { "Queue2_MultiCQ_P_1_Commit",        Queue2_MultiCQ_P_1_Commit },
    { "Queue2_MultiCQ_P_2_Commit",        Queue2_MultiCQ_P_2_Commit },
    { "Queue2_MultiCQ_P_0_Rollback",      Queue2_MultiCQ_P_0_Rollback },
    { "Queue2_MultiCQ_P_1_Rollback",      Queue2_MultiCQ_P_1_Rollback },
    { "Queue2_MultiCQ_P_2_Rollback",      Queue2_MultiCQ_P_2_Rollback },
    { "Queue2_MultiCQ_EnableDisable",     Queue2_MultiCQ_EnableDisable },
//  { "Queue2_MultiCQ_DrainQ",            Queue2_MultiCQ_DrainQ },
//  { "Queue2_MultiCQ_DrainQAdv",         Queue2_MultiCQ_DrainQAdv },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_MultiCQ_Confirm               */
/* Description:    Confirm tests for the multiConsumer queue         */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_1_Nack                          */
/* Description:     This testcase tests the redelilvery of messages  */
/*                  when were NACK'd                                 */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_MultiCQ_P_1_Nack( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_P_1_Nack...");

    Queue2_Nack( multiConsumer
               , ieqOptions_NONE
               , 512
               , ismMESSAGE_PERSISTENCE_PERSISTENT
               , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_1_Nack");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_2_Nack                          */
/* Description:     This testcase tests the redelilvery of messages  */
/*                  when were NACK'd                                 */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_MultiCQ_P_2_Nack( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_P_2_Nack...");

    Queue2_Nack( multiConsumer
               , ieqOptions_NONE
               , 512
               , ismMESSAGE_PERSISTENCE_PERSISTENT
               , ismMESSAGE_RELIABILITY_EXACTLY_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_2_Nack");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_1_NackActive                    */
/* Description:     This testcase tests the redelilvery of messages  */
/*                  when were NACK'd.Msgs are put while consumer     */
/*                  is active.                                       */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_MultiCQ_P_1_NackActive( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_P_1_NackActive...");

    Queue2_NackActive( multiConsumer
                     , ieqOptions_NONE
                     , 512
                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                     , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_1_NackActive");

    return;
}

/*********************************************************************/
/* TestCase:         Queue2_MultiCQ_Q1_C1_BatchNack                  */
/* Description:      This testcase tests the ability to ack a set    */
/*                   of messages in a batch.                         */
/* Queue:            MultiConsumer                                   */
/* Number of queues:    1                                            */
/* Number of consumers: 1                                            */
/* Number of messages:  1000                                         */
/*********************************************************************/
void Queue2_MultiCQ_Q1_C1_BatchNack( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_Q1_C1_BatchNack...");

    Queue2_BatchNack( multiConsumer 
                    , ieqOptions_NONE
                    , 1
                    , false
                    , 1000
                    , ismMESSAGE_PERSISTENCE_PERSISTENT );

    verbose(4, "Completed Queue2_MultiCQ_Q1_C1_BatchNack");

    return;
}

/*********************************************************************/
/* TestCase:         Queue2_MultiCQ_Q1_C5_BatchNack                  */
/* Description:      This testcase tests the ability to ack a set    */
/*                   of messages in a batch.                         */
/* Queue:            MultiConsumer                                   */
/* Number of queues:    1                                            */
/* Number of consumers: 5                                            */
/* Number of messages:  5000                                         */
/*********************************************************************/
void Queue2_MultiCQ_Q1_C5_BatchNack( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_Q1_C5_BatchNack...");

    Queue2_BatchNack( multiConsumer 
                    , ieqOptions_NONE
                    , 5
                    , false
                    , 5000
                    , ismMESSAGE_PERSISTENCE_PERSISTENT );

    verbose(4, "Completed Queue2_MultiCQ_Q1_C5_BatchNack");

    return;
}

/*********************************************************************/
/* TestCase:         Queue2_MultiCQ_Q5_C5_BatchNack                  */
/* Description:      This testcase tests the ability to ack a set    */
/*                   of messages in a batch.                         */
/* Queue:            MultiConsumer                                   */
/* Number of queues:    1                                            */
/* Number of consumers: 1                                            */
/* Number of messages:  5000                                         */
/*********************************************************************/
void Queue2_MultiCQ_Q5_C5_BatchNack( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_Q5_C5_BatchNack...");

    Queue2_BatchNack( multiConsumer 
                    , ieqOptions_NONE
                    , 5
                    , true
                    , 5000
                    , ismMESSAGE_PERSISTENCE_PERSISTENT );

    verbose(4, "Completed Queue2_MultiCQ_Q5_C5_BatchNack");

    return;
}

/*********************************************************************/
/* TestCase:         Queue2_MultiCQ_Q1_C1_BatchTranAck               */
/* Description:      This testcase tests the ability to ack a set    */
/*                   of messages in a batch.                         */
/* Queue:            MultiConsumer                                   */
/* Number of queues:    1                                            */
/* Number of consumers: 1                                            */
/* Number of messages:  1000                                         */
/*********************************************************************/
void Queue2_MultiCQ_Q1_C1_BatchTranAck( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_Q1_C1_BatchTranAck...");

    Queue2_BatchTranAck( multiConsumer 
                       , ieqOptions_NONE
                       , 1
                       , false
                       , 1000
                       , ismMESSAGE_PERSISTENCE_PERSISTENT );

    verbose(4, "Completed Queue2_MultiCQ_Q1_C1_BatchTranAck");

    return;
}

/*********************************************************************/
/* TestCase:         Queue2_MultiCQ_Q1_C5_BatchTranAck               */
/* Description:      This testcase tests the ability to ack a set    */
/*                   of messages in a batch.                         */
/* Queue:            MultiConsumer                                   */
/* Number of queues:    1                                            */
/* Number of consumers: 5                                            */
/* Number of messages:  5000                                         */
/*********************************************************************/
void Queue2_MultiCQ_Q1_C5_BatchTranAck( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_Q1_C5_BatchTranAck...");

    Queue2_BatchTranAck( multiConsumer 
                       , ieqOptions_NONE
                       , 5
                       , false
                       , 5000
                       , ismMESSAGE_PERSISTENCE_PERSISTENT );

    verbose(4, "Completed Queue2_MultiCQ_Q1_C5_BatchTranAck");

    return;
}

/*********************************************************************/
/* TestCase:         Queue2_MultiCQ_Q5_C5_BatchTranAck               */
/* Description:      This testcase tests the ability to ack a set    */
/*                   of messages in a batch.                         */
/* Queue:            MultiConsumer                                   */
/* Number of queues:    1                                            */
/* Number of consumers: 1                                            */
/* Number of messages:  5000                                         */
/*********************************************************************/
void Queue2_MultiCQ_Q5_C5_BatchTranAck( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_Q5_C5_BatchTranAck...");

    Queue2_BatchTranAck( multiConsumer 
                       , ieqOptions_NONE
                       , 5
                       , true
                       , 5000
                       , ismMESSAGE_PERSISTENCE_PERSISTENT );

    verbose(4, "Completed Queue2_MultiCQ_Q5_C5_BatchTranAck");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_2_Nack                          */
/* Description:     This testcase tests the redelilvery of messages  */
/*                  when were NACK'd                                 */
/*                  when were NACK'd.Msgs are put while consumer     */
/*                  is active.                                       */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_MultiCQ_P_2_NackActive( void )
{
    verbose(1, ""); // Ensure we start on a new line 
    verbose(1, "Starting Queue2_MultiCQ_P_2_NackActive...");

    Queue2_NackActive( multiConsumer
                     , ieqOptions_NONE
                     , 512
                     , ismMESSAGE_PERSISTENCE_PERSISTENT
                     , ismMESSAGE_RELIABILITY_EXACTLY_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_2_NackActive");

    return;
}

CU_TestInfo ISM_Engine_CUnit_Q2_MultiCQ_Confirm[] = {
    { "Queue2_MultiCQ_P_1_Nack",           Queue2_MultiCQ_P_1_Nack },
    { "Queue2_MultiCQ_P_2_Nack",           Queue2_MultiCQ_P_2_Nack },
    { "Queue2_MultiCQ_P_1_NackActive",     Queue2_MultiCQ_P_1_NackActive },
    { "Queue2_MultiCQ_P_2_NackActive",     Queue2_MultiCQ_P_2_NackActive },
    { "Queue2_MultiCQ_Q1_C1_BatchNack",    Queue2_MultiCQ_Q1_C1_BatchNack },
    { "Queue2_MultiCQ_Q1_C5_BatchNack",    Queue2_MultiCQ_Q1_C5_BatchNack },
    { "Queue2_MultiCQ_Q5_C5_BatchNack",    Queue2_MultiCQ_Q5_C5_BatchNack },
    { "Queue2_MultiCQ_Q1_C1_BatchTranAck", Queue2_MultiCQ_Q1_C1_BatchTranAck },
    { "Queue2_MultiCQ_Q1_C5_BatchTranAck", Queue2_MultiCQ_Q1_C5_BatchTranAck },
    { "Queue2_MultiCQ_Q5_C5_BatchTranAck", Queue2_MultiCQ_Q5_C5_BatchTranAck },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_MultiCQ_Reopen                */
/* Description:    Reopen and Redelivery tests for the multiConsumer */
/*                 queue                                             */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_1_RedeliverReopen               */
/* Description:     This testcase tests the redelivery of messages   */
/*                  when the queue is reopened                       */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_MultiCQ_P_1_RedeliverReopen( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_MultiCQ_P_1_RedeliverReopen...");

    Queue2_RedeliverReopen( multiConsumer
                          , ieqOptions_NONE
                          , ismMESSAGE_PERSISTENCE_PERSISTENT
                          , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_1_RedeliverReopen");

    return;
}
/*********************************************************************/
/* TestCase:        Queue2_MultiCQ_P_2_RedeliverReopen               */
/* Description:     This testcase tests the redelivery of messages   */
/*                  when the queue is reopened                       */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_MultiCQ_P_2_RedeliverReopen( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_MultiCQ_P_2_RedeliverReopen...");

    Queue2_RedeliverReopen( multiConsumer
                          , ieqOptions_NONE
                          , ismMESSAGE_PERSISTENCE_PERSISTENT
                          , ismMESSAGE_RELIABILITY_EXACTLY_ONCE );

    verbose(4, "Completed Queue2_MultiCQ_P_2_RedeliverReopen");

    return;
}

CU_TestInfo ISM_Engine_CUnit_Q2_MultiCQ_Reopen[] = {
    { "Queue2_MultiCQ_P_1_RedeliverReopen",  Queue2_MultiCQ_P_1_RedeliverReopen },
    { "Queue2_MultiCQ_P_2_RedeliverReopen",  Queue2_MultiCQ_P_2_RedeliverReopen },
    CU_TEST_INFO_NULL
};


/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_TempMultiCQ_Basic             */
/* Description:    Basic tests for temporary multiConsumer queue     */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_TempMultiCQ_M0_P_PutGet                   */
/* Description:     This testcase tests basic queue operation to     */
/*                  the multiConsumer queue when created with a      */
/*                  flag indicating it is to be temporary.           */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 0                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_TempMultiCQ_M0_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_TempMultiCQ_M0_C2_P_PutGet...");

    Queue2_PutGet( multiConsumer
                 , ieqOptions_TEMPORARY_QUEUE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_TempMultiCQ_M0_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_TempMultiCQ_M1_P_PutGet                   */
/* Description:     This testcase tests basic queue operation to     */
/*                  the multiConsumer queue when created with a      */
/*                  flag indicating it is to be temporary.           */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_TempMultiCQ_M1_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_TempMultiCQ_M1_C1_P_PutGet...");

    Queue2_PutGet( multiConsumer
                 , ieqOptions_TEMPORARY_QUEUE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_TempMultiCQ_M1_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_TempMultiCQ_M2_P_PutGet                   */
/* Description:     This testcase tests basic queue operation to     */
/*                  the multiConsumer queue when created with a      */
/*                  flag indicating it is to be temporary.           */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_TempMultiCQ_M2_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_TempMultiCQ_M2_P_PutGet...");

    Queue2_PutGet( multiConsumer
                 , ieqOptions_TEMPORARY_QUEUE
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_TempMultiCQ_M2_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_TempMultiCQ_M1_P_Put                      */
/* Description:     This testcase loads a queue with messages and    */
/*                  deletes the queue with messages still loaded.    */
/*                  This test is useful to verify we do not leak     */
/*                  memory while deleting a queue with pending msgs  */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_TempMultiCQ_M1_P_Put( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_TempMultiCQ_M0_P_Put...");

    Queue2_Put( multiConsumer
              , ieqOptions_TEMPORARY_QUEUE
              , 100
              , 32 // Minimum msgsize
              , 4096 // Maximum msgsize
              , ismMESSAGE_PERSISTENCE_PERSISTENT
              , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_TempMultiCQ_M1_P_Put");

    return;
}


CU_TestInfo ISM_Engine_CUnit_Q2_TempMultiCQ_Basic[] = {
    { "Queue2_TempMultiCQ_M0_P_PutGet",       Queue2_TempMultiCQ_M0_P_PutGet },
    { "Queue2_TempMultiCQ_M1_P_PutGet",       Queue2_TempMultiCQ_M1_P_PutGet },
    { "Queue2_TempMultiCQ_M2_P_PutGet",       Queue2_TempMultiCQ_M2_P_PutGet },
    { "Queue2_TempMultiCQ_M1_P_Put",          Queue2_TempMultiCQ_M1_P_Put },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestSuite:      ISM_Engine_CUnit_Q2_TempSingleCMultiCQ_Basic      */
/* Description:    Basic tests for multiConsumer queue with 'Single  */
/*                 Consumer Only' specified.                         */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        Queue2_TempSingleCMultiCQ_M0_NP_PutGet           */
/* Description:     This testcase tests basic queue operation to     */
/*                  the multiConsumer queue when created with a      */
/*                  flag indicating it has a single consumer.        */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Non Persistent                                   */
/* Msg Reliability: QoS 0                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_TempSingleCMultiCQ_M0_NP_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_TempSingleCMultiCQ_M0_NP_PutGet...");

    Queue2_PutGet( multiConsumer
                 , ieqOptions_TEMPORARY_QUEUE | ieqOptions_SINGLE_CONSUMER_ONLY
                 , 3072
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_NONPERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_TempSingleCMultiCQ_M0_NP_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_TempSingleCMultiCQ_M0_P_PutGet            */
/* Description:     This testcase tests basic queue operation to     */
/*                  the multiConsumer queue when created with a      */
/*                  flag indicating it has a single consumer.        */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 0                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_TempSingleCMultiCQ_M0_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_TempSingleCMultiCQ_M0_P_PutGet...");

    Queue2_PutGet( multiConsumer
                 , ieqOptions_TEMPORARY_QUEUE | ieqOptions_SINGLE_CONSUMER_ONLY
                 , 3072
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_MOST_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_TempSingleCMultiCQ_M0_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_TempSingleCMultiCQ_M1_P_PutGet            */
/* Description:     This testcase tests basic queue operation to     */
/*                  the multiConsumer queue when created with a      */
/*                  flag indicating it has a single consumer.        */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_TempSingleCMultiCQ_M1_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_TempSingleCMultiCQ_M1_P_PutGet...");

    Queue2_PutGet( multiConsumer
                 , ieqOptions_TEMPORARY_QUEUE | ieqOptions_SINGLE_CONSUMER_ONLY
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_TempSingleCMultiCQ_M1_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_TempSingleCMultiCQ_M2_P_PutGet            */
/* Description:     This testcase tests basic queue operation to     */
/*                  the multiConsumer queue when created with a      */
/*                  flag indicating it has a single consumer.        */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 2                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_TempSingleCMultiCQ_M2_P_PutGet( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_TempSingleCMultiCQ_M2_P_PutGet...");

    Queue2_PutGet( multiConsumer
                 , ieqOptions_TEMPORARY_QUEUE | ieqOptions_SINGLE_CONSUMER_ONLY
                 , 100
                 , 32 // Minimum msgsize
                 , 102400 // Maximum msgsize
                 , ismMESSAGE_PERSISTENCE_PERSISTENT
                 , ismMESSAGE_RELIABILITY_EXACTLY_ONCE
                 , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE
                 , 0);

    verbose(4, "Completed Queue2_TempSingleCMultiCQ_M2_P_PutGet");

    return;
}

/*********************************************************************/
/* TestCase:        Queue2_TempSingleCMultiCQ_M1_P_Put               */
/* Description:     This testcase loads a queue with messages and    */
/*                  deletes the queue with messages still loaded.    */
/*                  This test is useful to verify we do not leak     */
/*                  memory while deleting a queue with pending msgs  */
/* Queue:           MultiConsumer                                    */
/* Msg Persistence: Persistent                                       */
/* Msg Reliability: QoS 1                                            */
/* Msg Size:      : 0 - 100K (random)                                */
/*********************************************************************/
void Queue2_TempSingleCMultiCQ_M1_P_Put( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting Queue2_TempSingleCMultiCQ_M0_P_Put...");

    Queue2_Put( multiConsumer
              , ieqOptions_TEMPORARY_QUEUE | ieqOptions_SINGLE_CONSUMER_ONLY
              , 100
              , 32 // Minimum msgsize
              , 4096 // Maximum msgsize
              , ismMESSAGE_PERSISTENCE_PERSISTENT
              , ismMESSAGE_RELIABILITY_AT_LEAST_ONCE );

    verbose(4, "Completed Queue2_TempSingleCMultiCQ_M1_P_Put");

    return;
}


CU_TestInfo ISM_Engine_CUnit_Q2_TempSingleCMultiCQ_Basic[] = {
    { "Queue2_TempSingleCMultiCQ_M0_NP_PutGet",   Queue2_TempSingleCMultiCQ_M0_NP_PutGet },
    { "Queue2_TempSingleCMultiCQ_M0_P_PutGet",    Queue2_TempSingleCMultiCQ_M0_P_PutGet },
    { "Queue2_TempSingleCMultiCQ_M1_P_PutGet",    Queue2_TempSingleCMultiCQ_M1_P_PutGet },
    { "Queue2_TempSingleCMultiCQ_M2_P_PutGet",    Queue2_TempSingleCMultiCQ_M2_P_PutGet },
    { "Queue2_TempSingleCMultiCQ_M1_P_Put",       Queue2_TempSingleCMultiCQ_M1_P_Put },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/* TestCase:        Queue2_InvalidQueueType                          */
/* Description:     Call ie* functions with an invalid queue type    */
/* Queue:           N/A                                              */
/*********************************************************************/
void Queue2_InvalidQueueType( void )
{
    verbose(1, ""); // Ensure we start on a new line
    verbose(1, "Starting %s...", __func__);

    char buffer[8];

    char *typeText = ieut_queueTypeText(0, buffer, sizeof(buffer));
    TEST_ASSERT_PTR_NOT_NULL(typeText);
    TEST_ASSERT_STRINGS_EQUAL(typeText, "UNKNOWN");

    return;
}


CU_TestInfo ISM_Engine_CUnit_Q2_OtherThings[] = {
    { "InvalidQueueType",   Queue2_InvalidQueueType },
    CU_TEST_INFO_NULL
};

int initQueue2(void)
{
    ism_field_t f;
    int32_t rc;

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();

    //Allow MQTT clients to have 100 unacked msgs for some of the acking/nacking tests
    f.type = VT_Integer;
    f.val.i = 1000;
    rc = ism_common_setProperty( staticConfigProps
                               , ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES
                               , &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s returned %d\n", ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES, rc);
        goto mod_exit;
    }

    rc = test_engineInit_DEFAULT;
    if (rc != OK) goto mod_exit;

    rc = ism_engine_createClientState("Consumer Test",
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

int termQueue2(void)
{
    return test_engineTerm(true);
}

/*********************************************************************/
/* TestSuite definition                                              */
/*********************************************************************/
CU_SuiteInfo ISM_Engine_CUnit_Q2_Suites[] = {
    IMA_TEST_SUITE("SimpQBasic", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_SimpQ_Basic ),
    IMA_TEST_SUITE("SimpQAdvanced", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_SimpQ_Advanced ),
    IMA_TEST_SUITE("InterQBasic", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_InterQ_Basic ),
    IMA_TEST_SUITE("InterQAdvanced", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_InterQ_Advanced ),
    IMA_TEST_SUITE("InterQConfirm", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_InterQ_Confirm ),
    IMA_TEST_SUITE("InterQReopen", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_InterQ_Reopen ),
    IMA_TEST_SUITE("MultiCQBasic", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_MultiCQ_Basic ),
    IMA_TEST_SUITE("MultiCQAdvanced", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_MultiCQ_Advanced ),
    IMA_TEST_SUITE("MultiCQConfirm", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_MultiCQ_Confirm ),
//    IMA_TEST_SUITE("MultiCQReopen", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_MultiCQ_Reopen ),
    IMA_TEST_SUITE("TempMultiCQBasic", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_TempMultiCQ_Basic ),
    IMA_TEST_SUITE("TempSingleCMultiCQBasic", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_TempSingleCMultiCQ_Basic ),
    IMA_TEST_SUITE("OtherThings", initQueue2, termQueue2, ISM_Engine_CUnit_Q2_OtherThings ),
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
    CU_register_suites(ISM_Engine_CUnit_Q2_Suites);

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
