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
#include <assert.h>
#include <sys/prctl.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "engineInternal.h"
#include "queueCommon.h"
#include "msgCommon.h"
#include "policyInfo.h"
#include "waiterStatus.h"
#include "simpQInternal.h"
#include "intermediateQInternal.h"
#include "multiConsumerQInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_task.h"

/* Message payload for all tests */
typedef struct tag_testPayload_t {
    int32_t msgNum;
    int32_t pubberNum;
    char msgbuffer[20];
} testPayload_t;

typedef enum tag_MultiFlow_State_t {
    setup, running
} MultiFlow_State_t;

typedef struct tag_MultiFlow_PutterInfo_t {
    uint32_t putterNum;
    pthread_t thread_id;
    uint32_t MsgsToPut;
    ismQHandle_t Q;
    pthread_cond_t *cond;
    pthread_mutex_t *mutex;
    MultiFlow_State_t *state;
} MultiFlow_PutterInfo_t;

typedef enum tag_TestDisable_State_t {
    testDisable_setup, testDisable_running
} TestDisable_State_t;

typedef struct tag_TestDisable_ThreadInfo_t {
    uint32_t putterNum;
    pthread_t thread_id;
    uint32_t *MsgsReceived;
    uint32_t numIterations;
    uint32_t *currentIteration;
    uint32_t *puttersInProgress;
    ismQHandle_t Q;
    pthread_cond_t *startCond;
    pthread_mutex_t *startMutex;
    pthread_cond_t *stopCond;
    pthread_mutex_t *stopMutex;
    TestDisable_State_t *state;
    bool *stopPutting;
    ismEngine_Consumer_t *pConsumer;
} TestDisable_ThreadInfo_t;

typedef struct tag_NestedPut_ContextInfo_t {
    uint32_t expectedMsgNum;
    uint32_t remainingMessages;
    uint32_t totalMsgs;
    ismQHandle_t Q;
} NestedPut_ContextInfo_t;

typedef struct tag_SimplePut_ContextInfo_t {
    uint32_t msgsExpected;
    uint32_t expectedMsgNum;
} SimplePut_ContextInfo_t;

//How many putting threads should we have in our multi-threaded putting test?
#define MULTIFLOW_PUTTERS 5

typedef struct tag_MultiPut_ContextInfo_t {
    uint32_t msgsExpected;
    uint32_t msgNumFromPutter[MULTIFLOW_PUTTERS];
} MultiPut_ContextInfo_t;

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
// Used by testDisable and testTerminate:
static bool waiterDisableCompleted = false;
//Used by testTerminate:
static bool waiterTermCompleted = false;

static uint32_t verboseLevel = 2;   /* 0-9 */

//Used to decide whether we are running in discard old or reject newest mode
iepiMaxMsgBehavior_t maxMsgBehaviour = RejectNewMessages;


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

// Callback from destination to provide status updates
void ism_engine_deliverStatus(ieutThreadData_t *pThreadData,
                              ismEngine_Consumer_t *pConsumer,
                              int32_t retcode)
{
    TRACE(ENGINE_FNC_TRACE, ">>> ism_engine_deliverStatus(hConsumer %p, rc=%d)\n",
                                  pConsumer,  retcode);

    if ( retcode == ISMRC_WaiterDisabled )
    {
        waiterDisableCompleted = true;
        //We should never be told we're disabled after we have terminated:

        TEST_ASSERT_CUNIT(!waiterTermCompleted, ("waterTermCompleted was true"));
    }
    else if (retcode == ISMRC_WaiterRemoved )
    {
        waiterTermCompleted = true;
    }

    TRACE(ENGINE_FNC_TRACE, "<<< ism_engine_deliverStatus\n");
}

static ismEngine_MessageHandle_t createTestMessage(int pubberNum, int msgNum)
{
    testPayload_t payload;
    ismEngine_MessageHandle_t hMessage = NULL;

    payload.msgNum = msgNum;
    payload.pubberNum = pubberNum;
    memset(payload.msgbuffer, 'X', 20);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengths[2] = {0, sizeof(testPayload_t)};
    void *areaData[2] = {NULL, &payload};

    int32_t rc = ism_engine_createMessage(&header,
                                          2,
                                          areaTypes,
                                          areaLengths,
                                          areaData,
                                          &hMessage);

    TEST_ASSERT_EQUAL(rc, OK);

    return hMessage;
}

static void checkTotalMessages(ismQHandle_t Q, uint64_t expectedMessages)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ismEngine_QueueStatistics_t qstats;
    ieq_getStats(pThreadData, Q, &qstats);
    TEST_ASSERT_EQUAL(qstats.BufferedMsgs, expectedMessages);
}


static bool GenericTestCallback(uint32_t                   *msgsExpected,
                                uint32_t                   expectedPutterNum,
                                uint32_t                   *expectedMsgNum,
                                ismEngine_MessageHandle_t  hMessage,
                                uint8_t                    areaCount,
                                ismMessageAreaType_t       areaTypes[areaCount],
                                size_t                     areaLengths[areaCount],
                                void *                     pAreaData[areaCount])
{
    TEST_ASSERT(*msgsExpected > 0, ("*msgsExpected > 0"));

    testPayload_t *payload = (testPayload_t *)pAreaData[1];

    TEST_ASSERT_EQUAL(payload->pubberNum, expectedPutterNum);
    TEST_ASSERT_EQUAL(payload->msgNum, *expectedMsgNum);

    //Next time, the message number in the payload should have increased....
    (*expectedMsgNum)++;

    //Reduce the number of expected messages as one arrived
    (*msgsExpected)--;

    //free the callers copy of the message
    ism_engine_releaseMessage(hMessage);

    //Request another message if we want one...
    if(*msgsExpected > 0)
    {
        return true;
    }
    //time to stop
    return false;

}


static bool CallbackTestSimplePut(
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
    SimplePut_ContextInfo_t *pContext = (SimplePut_ContextInfo_t *)pConsumerContext;

    GenericTestCallback(&pContext->msgsExpected, 
                        0,
                        &pContext->expectedMsgNum,
                        hMessage, areaCount,  areaTypes, areaLengths, pAreaData);

    //Request another message if one exists
    return true;
}

static void testPut(ismQueueType_t qtype)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismQHandle_t Q;
    ismEngine_ClientState_t Client = {{0}};
    ismEngine_Session_t Session = {{0}};
    ismEngine_Consumer_t Consumer = {{0}};
    SimplePut_ContextInfo_t Context = { 0, 0 };
    char TestName[64];
    char qtypeText[16];

    verbose(1, "");
    sprintf(TestName, "%s(%s)...", __func__, ieut_queueTypeText(qtype, qtypeText, sizeof(qtypeText)));
    verbose(1, "Starting %s...", TestName);

    rc = ieq_createQ( pThreadData
                    , qtype
                    , NULL
                    , ieqOptions_DEFAULT
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &Q);
    TEST_ASSERT_EQUAL(rc, OK);

    waiterDisableCompleted = false;
    waiterTermCompleted    = false;

    //Setup the getter
    Session.pClient = &Client;
    Client.maxInflightLimit = 128;
    Client.expectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
    Consumer.pSession = &Session;
    Consumer.fDestructiveGet = true;
    Consumer.fAcking = (qtype != simple);
    Consumer.fShortDeliveryIds = (qtype == intermediate);
    Consumer.pMsgCallbackFn = CallbackTestSimplePut;
    Consumer.pMsgCallbackContext = &Context;
    Consumer.counts.parts.useCount = 1;
    rc = ieq_initWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);

    //Create a message, pubber 0, 0th msg
    ismEngine_MessageHandle_t hMsg1 = createTestMessage(0,0);

    //Put a message to the Queue without enabling waiter
    rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg1, IEQ_MSGTYPE_REFCOUNT);
    TEST_ASSERT_EQUAL(rc, OK);
    ism_engine_releaseMessage(hMsg1);

    checkTotalMessages(Q, 1);

    //Once we  enable the waiter we expect to receive the message
    Context.msgsExpected = 1;
    acquireConsumerReference(&Consumer); //An enabled waiter counts as a reference
    rc = ieq_enableWaiter(pThreadData, Q, &Consumer, IEQ_ENABLE_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    //Message should have been received by the time enable waiter returns...
    TEST_ASSERT_EQUAL(0, Context.msgsExpected);
    checkTotalMessages(Q, 0);

    //Now put another and check it is delivered
    ismEngine_MessageHandle_t hMsg2 = createTestMessage(0,1);
    Context.msgsExpected = 1;
    rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg2, IEQ_MSGTYPE_REFCOUNT);
    TEST_ASSERT_EQUAL(rc, OK);
    ism_engine_releaseMessage(hMsg2);
    TEST_ASSERT_EQUAL(0, Context.msgsExpected);
    checkTotalMessages(Q, 0);

    rc = ieq_termWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieq_delete(pThreadData, &Q, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(3, "Completed %s", TestName);
}


static bool CallbackTestPutSomeGetFew(
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
    SimplePut_ContextInfo_t *pContext = (SimplePut_ContextInfo_t *)pConsumerContext;

    return GenericTestCallback(&pContext->msgsExpected,
                               0,
                               &pContext->expectedMsgNum,
                               hMessage, areaCount,  areaTypes, areaLengths, pAreaData);
}


//Check that if a getter requests the flow of messages to stop, they stop
//even if there are more messages
static void testPutSomeGetFew(ismQueueType_t qtype)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismQHandle_t Q;
    ismEngine_ClientState_t Client = {{0}};
    ismEngine_Consumer_t Consumer = {{0}};
    ismEngine_Session_t Session = {{0}};
    uint32_t msgsToPut = 10;
    uint32_t msgsToGet = 3;
    SimplePut_ContextInfo_t Context = { 0, 0 };
    char TestName[64];
    char qtypeText[16];

    verbose(1, "");
    sprintf(TestName, "%s(%s)...", __func__, ieut_queueTypeText(qtype, qtypeText, sizeof(qtypeText)));
    verbose(1, "Starting %s...", TestName);

    rc = ieq_createQ( pThreadData
                    , qtype
                    , NULL
                    , ieqOptions_DEFAULT
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &Q);
    TEST_ASSERT_EQUAL(rc, OK);

    waiterDisableCompleted = false;
    waiterTermCompleted    = false;

    //Setup the getter
    Session.pClient = &Client;
    Client.maxInflightLimit = 128;
    Client.expectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
    Consumer.pSession = &Session;
    Consumer.fDestructiveGet = true;
    Consumer.fAcking = (qtype != simple);
    Consumer.fShortDeliveryIds = (qtype == intermediate);
    Consumer.pMsgCallbackFn = CallbackTestPutSomeGetFew;
    Consumer.pMsgCallbackContext = &Context;
    Consumer.counts.parts.useCount = 1;
    rc = ieq_initWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);

    //Put some messages...
    int i;
    for (i=0; i< msgsToPut; i++)
    {
        ismEngine_MessageHandle_t hMsg = createTestMessage(0,i);
        rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
        TEST_ASSERT_EQUAL(rc, OK);
        ism_engine_releaseMessage(hMsg);
    }
    checkTotalMessages(Q, msgsToPut);

    //Once we enable the waiter we expect to receive messages
    Context.msgsExpected = msgsToGet;
    acquireConsumerReference(&Consumer); //An enabled waiter counts as a reference
    rc = ieq_enableWaiter(pThreadData, Q, &Consumer, IEQ_ENABLE_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    //Message should have been received by the time enable waiter returns...
    TEST_ASSERT_EQUAL(0, Context.msgsExpected);
    checkTotalMessages(Q, msgsToPut - msgsToGet);

    rc = ieq_termWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieq_delete(pThreadData, &Q, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(3, "Completed %s", TestName);
}


static bool CallbackTestMaxMessages(
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
    SimplePut_ContextInfo_t *pContext = (SimplePut_ContextInfo_t *)pConsumerContext;

    return GenericTestCallback(&pContext->msgsExpected,
                               0,
                               &pContext->expectedMsgNum,
                               hMessage, areaCount,  areaTypes, areaLengths, pAreaData);
}

//Check puts fail gracefully if maxMessageCount is hit
static void testMaxMessages(ismQueueType_t qtype)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismQHandle_t Q;
    ismEngine_ClientState_t Client = {{0}};
    ismEngine_Session_t Session = {{0}};
    ismEngine_Consumer_t Consumer = {{0}};
    uint64_t initialMaxMessages = 5;
    uint32_t msgNum = 0;
    ismEngine_MessageHandle_t hMsg;
    SimplePut_ContextInfo_t Context = { 0, 0 };
    char TestName[64];
    char qtypeText[16];

    verbose(1, "");
    sprintf(TestName, "%s(%s)...", __func__, ieut_queueTypeText(qtype, qtypeText, sizeof(qtypeText)));
    verbose(1, "Starting %s...", TestName);

    rc = ieq_createQ( pThreadData
                    , qtype
                    , NULL
                    , ieqOptions_DEFAULT
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &Q);
    TEST_ASSERT_EQUAL(rc, OK);

    waiterDisableCompleted = false;
    waiterTermCompleted    = false;

    iepiPolicyInfo_t templatePolicyInfo;

    templatePolicyInfo.maxMessageCount = initialMaxMessages;
    templatePolicyInfo.maxMsgBehavior = RejectNewMessages;
    templatePolicyInfo.DCNEnabled = false;

    iepiPolicyInfo_t *fakePolicy1 = NULL;
    rc = iepi_createPolicyInfo(pThreadData, "fakePolicy1", ismSEC_POLICY_QUEUE, true, &templatePolicyInfo, &fakePolicy1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(fakePolicy1);
    TEST_ASSERT_EQUAL(fakePolicy1->creationState, CreatedByConfig);
    TEST_ASSERT_EQUAL(fakePolicy1->maxMessageCount, initialMaxMessages);
    TEST_ASSERT_EQUAL(fakePolicy1->maxMessageBytes, 0);

    ieq_setPolicyInfo(pThreadData, Q, fakePolicy1);

    //Setup the getter
    Session.pClient = &Client;
    Client.maxInflightLimit = 128;
    Client.expectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
    Consumer.pSession = &Session;
    Consumer.fDestructiveGet = true;
    Consumer.fAcking = (qtype != simple);
    Consumer.fShortDeliveryIds = (qtype == intermediate);
    Consumer.pMsgCallbackFn = CallbackTestMaxMessages;
    Consumer.pMsgCallbackContext = &Context;
    Consumer.counts.parts.useCount = 1;

    rc = ieq_initWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);

    //Put some messages that should fit in the queue...
    uint64_t i;
    for (i=0; i < initialMaxMessages; i++)
    {
        hMsg = createTestMessage(0,msgNum++);
        rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
        TEST_ASSERT_EQUAL(rc, OK);
        ism_engine_releaseMessage(hMsg);
    }
    checkTotalMessages(Q, initialMaxMessages);

    //And now a message that shouldn't
    hMsg = createTestMessage(0,msgNum);
    rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);
    ism_engine_releaseMessage(hMsg);

    //Enable the waiter to get a single message
    Context.msgsExpected = 1;
    acquireConsumerReference(&Consumer); //An enabled waiter counts as a reference
    rc = ieq_enableWaiter(pThreadData, Q, &Consumer, IEQ_ENABLE_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);
    checkTotalMessages(Q, initialMaxMessages-1);

    //Check we can put another message...
    hMsg = createTestMessage(0,msgNum++);
    rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
    TEST_ASSERT_EQUAL(rc, OK);
    ism_engine_releaseMessage(hMsg);

    //But the queue should now be full...
    hMsg = createTestMessage(0,msgNum);
    rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);
    ism_engine_releaseMessage(hMsg);

    //Increasing the max messages should allow us to put more...
    fakePolicy1->maxMessageCount = 2*initialMaxMessages;
    for (i=0; i < initialMaxMessages; i++)
    {
        hMsg = createTestMessage(0,msgNum++);
        rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
        TEST_ASSERT_EQUAL(rc, OK);
        ism_engine_releaseMessage(hMsg);
    }
    checkTotalMessages(Q, 2*initialMaxMessages);

    //But the queue should now be full...
    hMsg = createTestMessage(0,msgNum);
    rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);
    ism_engine_releaseMessage(hMsg);
    checkTotalMessages(Q, 2*initialMaxMessages);

    //Decrease the max messages and check still can't put
    fakePolicy1->maxMessageCount = initialMaxMessages;
    hMsg = createTestMessage(0,msgNum);
    rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);
    ism_engine_releaseMessage(hMsg);
    checkTotalMessages(Q, 2*initialMaxMessages);

    //Get enough messages that we should be able to put a single message
    Context.msgsExpected = initialMaxMessages+1;
    acquireConsumerReference(&Consumer); //An enabled waiter counts as a reference
    rc = ieq_enableWaiter(pThreadData, Q, &Consumer, IEQ_ENABLE_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);
    checkTotalMessages(Q, initialMaxMessages-1);

    //Check we can put a message
    hMsg = createTestMessage(0,msgNum++);
    rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
    TEST_ASSERT_EQUAL(rc, OK);
    ism_engine_releaseMessage(hMsg);
    checkTotalMessages(Q, initialMaxMessages);

    //But check it's now full
    hMsg = createTestMessage(0,msgNum);
    rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);
    ism_engine_releaseMessage(hMsg);
    checkTotalMessages(Q, initialMaxMessages);

    rc = ieq_termWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieq_delete(pThreadData, &Q, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(3, "Completed %s", TestName);
}


static bool CallbackTestFlow(
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
    SimplePut_ContextInfo_t *pContext = (SimplePut_ContextInfo_t *)pConsumerContext;

    return GenericTestCallback(&pContext->msgsExpected,
                               0,
                               &pContext->expectedMsgNum,
                               hMessage, areaCount,  areaTypes, areaLengths, pAreaData);
}

//Check we can flow a large(ish) number of messages through the queue
static void testFlow(ismQueueType_t qtype)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismQHandle_t Q;
    ismEngine_ClientState_t Client = {{0}};
    ismEngine_Session_t Session = {{0}};
    ismEngine_Consumer_t Consumer = {{0}};
    uint64_t msgsToFlow = 500000;
    SimplePut_ContextInfo_t Context = { 0, 0 };
    uint32_t msgNum = 0;
    char TestName[64];
    char qtypeText[16];

    verbose(1, "");
    sprintf(TestName, "%s(%s)...", __func__, ieut_queueTypeText(qtype, qtypeText, sizeof(qtypeText)));
    verbose(1, "Starting %s...", TestName);

    iepiPolicyInfo_t templatePolicyInfo;

    templatePolicyInfo.maxMessageCount = msgsToFlow;
    templatePolicyInfo.maxMsgBehavior = RejectNewMessages;
    templatePolicyInfo.DCNEnabled = false;

    iepiPolicyInfo_t *flowPolicyInfo = NULL;
    rc = iepi_createPolicyInfo(pThreadData, "flowPolicy", ismSEC_POLICY_QUEUE, false, &templatePolicyInfo, &flowPolicyInfo);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(flowPolicyInfo);
    TEST_ASSERT_EQUAL(flowPolicyInfo->creationState, CreatedByEngine);
    TEST_ASSERT_EQUAL(flowPolicyInfo->maxMessageCount, msgsToFlow);
    TEST_ASSERT_EQUAL(flowPolicyInfo->maxMessageBytes, 0);

    rc = ieq_createQ( pThreadData
                    , qtype
                    , NULL
                    , ieqOptions_DEFAULT
                    , flowPolicyInfo
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &Q);
    TEST_ASSERT_EQUAL(rc, OK);

    waiterDisableCompleted = false;
    waiterTermCompleted = false;

    //Setup the getter (but don't start it)
    Session.pClient = &Client;
    Client.maxInflightLimit = 128;
    Client.expectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
    Consumer.pSession = &Session;
    Consumer.fDestructiveGet = true;
    Consumer.fAcking = (qtype != simple);
    Consumer.fShortDeliveryIds = (qtype == intermediate);
    Consumer.pMsgCallbackFn = CallbackTestFlow;
    Consumer.pMsgCallbackContext = &Context;
    Consumer.counts.parts.useCount = 1;

    rc = ieq_initWaiter(pThreadData, Q, &Consumer); //TREVOR
    TEST_ASSERT_EQUAL(rc, OK);

    //Build up a deep queue
    uint32_t target_depth = msgsToFlow/2;
    int i;

    for (i=0; i < target_depth; i++)
    {
        ismEngine_MessageHandle_t hMsg= createTestMessage(0,msgNum++);
        rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
        TEST_ASSERT_EQUAL(rc, OK);
        ism_engine_releaseMessage(hMsg);
    }

    checkTotalMessages(Q, target_depth);

    //Once we  enable the waiter we expect to receive the message
    Context.msgsExpected = msgsToFlow;
    acquireConsumerReference(&Consumer); //An enabled waiter counts as a reference
    rc = ieq_enableWaiter(pThreadData, Q, &Consumer, IEQ_ENABLE_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    //Messages on queue should have been received by the time enable waiter returns...
    TEST_ASSERT_EQUAL(Context.msgsExpected, (msgsToFlow - target_depth));
    checkTotalMessages(Q, 0);

    //Now flow remaining messages with the waiter enabled...
    uint32_t msgs = msgsToFlow - target_depth;

    for (i=0; i < msgs; i++)
    {
        ismEngine_MessageHandle_t hMsg= createTestMessage(0,msgNum++);
        rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
        TEST_ASSERT_EQUAL(rc, OK);
        ism_engine_releaseMessage(hMsg);
    }

    checkTotalMessages(Q, 0);

    rc = ieq_termWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieq_delete(pThreadData, &Q, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(3, "Completed %s", TestName);
}

static void *MultiFlowPutter(void *threadarg)
{
    MultiFlow_PutterInfo_t *putterInfo = (MultiFlow_PutterInfo_t *)threadarg;
    int32_t rc;

    ism_engine_threadInit(0);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    //Wait to be told to start
    rc = pthread_mutex_lock(putterInfo->mutex);
    TEST_ASSERT_EQUAL(rc, OK);

    while (*(putterInfo->state) == setup)
    {
        rc = pthread_cond_wait(putterInfo->cond, putterInfo->mutex);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    rc = pthread_mutex_unlock(putterInfo->mutex);
    TEST_ASSERT_EQUAL(rc, OK);

    //OK...let's go
    int i;
    for (i=0; i < putterInfo->MsgsToPut; i++)
    {
        ismEngine_MessageHandle_t hMsg= createTestMessage( putterInfo->putterNum
                                                         , i);
        rc = ieq_put(pThreadData, putterInfo->Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
        TEST_ASSERT_EQUAL(rc, OK);
        ism_engine_releaseMessage(hMsg);
    }

    ism_engine_threadTerm(1);

    return NULL;
}

static bool CallbackTestMultiFlow(
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
    MultiPut_ContextInfo_t *pContext = (MultiPut_ContextInfo_t *)pConsumerContext;

    //Work out which putter this msg is from so we know which message number we received last
    //Assumes the last area is the payload
    uint32_t putterNum = ((testPayload_t *)pAreaData[areaCount-1])->pubberNum;

    TEST_ASSERT(putterNum < MULTIFLOW_PUTTERS, ("putterNum < MULTIFLOW_PUTTERS"));

    GenericTestCallback(&pContext->msgsExpected,
                        putterNum,
                        &(pContext->msgNumFromPutter[putterNum]),
                        hMessage, areaCount,  areaTypes, areaLengths, pAreaData);

    return true;
}
//Check we can flow a large number of messages through the queue from multiple putters
static void testMultiFlow(ismQueueType_t qtype)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismQHandle_t Q;
    ismEngine_ClientState_t Client = {{0}};
    ismEngine_Session_t Session = {{0}};
    ismEngine_Consumer_t Consumer = {{0}};
    uint64_t msgsPerPutter = 100000;
    MultiFlow_PutterInfo_t putterInfo[MULTIFLOW_PUTTERS];
    pthread_cond_t testMultiFlow_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t testMultiFlow_mutex = PTHREAD_MUTEX_INITIALIZER;
    MultiFlow_State_t state = setup;
    MultiPut_ContextInfo_t Context = { MULTIFLOW_PUTTERS * msgsPerPutter, {0} };
    char TestName[64];
    char qtypeText[16];

    verbose(1, "");
    sprintf(TestName, "%s(%s)...", __func__, ieut_queueTypeText(qtype, qtypeText, sizeof(qtypeText)));
    verbose(1, "Starting %s...", TestName);

    iepiPolicyInfo_t templatePolicyInfo;

    templatePolicyInfo.maxMessageCount = msgsPerPutter*MULTIFLOW_PUTTERS;
    templatePolicyInfo.maxMsgBehavior = RejectNewMessages;
    templatePolicyInfo.DCNEnabled = false;

    iepiPolicyInfo_t *multiFlowPolicyInfo = NULL;
    rc = iepi_createPolicyInfo(pThreadData, "multiFlowPolicy", ismSEC_POLICY_QUEUE, false, &templatePolicyInfo, &multiFlowPolicyInfo);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(multiFlowPolicyInfo);
    TEST_ASSERT_EQUAL(multiFlowPolicyInfo->creationState, CreatedByEngine);
    TEST_ASSERT_EQUAL(multiFlowPolicyInfo->maxMessageCount, msgsPerPutter*MULTIFLOW_PUTTERS);
    TEST_ASSERT_EQUAL(multiFlowPolicyInfo->maxMessageBytes, 0);

    rc = ieq_createQ( pThreadData
                    , qtype
                    , NULL
                    , ieqOptions_DEFAULT
                    , multiFlowPolicyInfo
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &Q);
    TEST_ASSERT_EQUAL(rc, OK);

    //Setup the getter (but don't start it)
    Session.pClient = &Client;
    Client.maxInflightLimit = 128;
    Client.expectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
    Consumer.pSession = &Session;
    Consumer.fDestructiveGet = true;
    Consumer.fAcking = (qtype != simple);
    Consumer.fShortDeliveryIds = (qtype == intermediate);
    Consumer.pMsgCallbackFn = CallbackTestMultiFlow;
    Consumer.pMsgCallbackContext = &Context;
    Consumer.counts.parts.useCount = 1;

    rc = ieq_initWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);
    acquireConsumerReference(&Consumer); //An enabled waiter counts as a reference
    rc = ieq_enableWaiter(pThreadData, Q, &Consumer, IEQ_ENABLE_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);


    int i;
    for (i=0; i<MULTIFLOW_PUTTERS; i++)
    {
        //Set up the info for the putter
        putterInfo[i].putterNum = i;
        putterInfo[i].MsgsToPut = msgsPerPutter;
        putterInfo[i].Q = Q;
        putterInfo[i].cond = &testMultiFlow_cond;
        putterInfo[i].mutex = &testMultiFlow_mutex;
        putterInfo[i].state = &state;


        //Start the thread
        rc = test_task_startThread(&(putterInfo[i].thread_id),MultiFlowPutter, (void *)&(putterInfo[i]),"MultiFlowPutter");
        TEST_ASSERT_EQUAL(rc, OK);
    }
    //broadcast start publishing to the threads
    rc = pthread_mutex_lock(&(testMultiFlow_mutex));
    TEST_ASSERT_EQUAL(rc, OK);
    state = running;
    rc = pthread_cond_broadcast(&(testMultiFlow_cond));
    TEST_ASSERT_EQUAL(rc, OK);
    rc = pthread_mutex_unlock(&(testMultiFlow_mutex));
    TEST_ASSERT_EQUAL(rc, OK);


    //Join all the threads
    for (i=0; i<MULTIFLOW_PUTTERS; i++)
    {
        rc = pthread_join(putterInfo[i].thread_id, NULL);
    }

    //Check there are no messages on the queue and the getter received as many messages as expected
    TEST_ASSERT_EQUAL(Context.msgsExpected, 0);
    checkTotalMessages(Q, 0);

    waiterDisableCompleted = false;
    waiterTermCompleted = false;

    rc = ieq_termWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieq_delete(pThreadData, &Q, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(3, "Completed %s", TestName);
}


static bool CallbackTestNestedPut(
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
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    NestedPut_ContextInfo_t *context = (NestedPut_ContextInfo_t *)pConsumerContext;

    GenericTestCallback(&(context->remainingMessages), 0,
                        &(context->expectedMsgNum),
                        hMessage, areaCount,  areaTypes, areaLengths, pAreaData);

    //If we expect more messages, put more
    if (context->remainingMessages)
    {
        //Create a message, pubber 0, 0th msg
        ismEngine_MessageHandle_t hMsg = createTestMessage(0,   context->totalMsgs
                                                             - context->remainingMessages);

        //Put a message to the Queue nested in this callback
        int32_t rc = ieq_put(pThreadData, context->Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
        TEST_ASSERT_EQUAL(rc, OK);
        ism_engine_releaseMessage(hMsg);
    }

    //Request another message if one exists
    return true;
}

static void testNestedPut(ismQueueType_t qtype)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismQHandle_t Q;
    ismEngine_ClientState_t Client = {{0}};
    ismEngine_Session_t Session = {{0}};
    ismEngine_Consumer_t Consumer = {{0}};
    uint32_t totalNestedMsgs = 100;
    NestedPut_ContextInfo_t consumerContext;
    char TestName[64];
    char qtypeText[16];

    verbose(1, "");
    sprintf(TestName, "%s(%s)...", __func__, ieut_queueTypeText(qtype, qtypeText, sizeof(qtypeText)));
    verbose(1, "Starting %s...", TestName);

    rc = ieq_createQ( pThreadData
                    , qtype
                    , NULL
                    , ieqOptions_DEFAULT
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &Q);
    TEST_ASSERT_EQUAL(rc, OK);

    //Setup context for getter
    consumerContext.Q = Q;
    consumerContext.expectedMsgNum = 0;
    consumerContext.totalMsgs = totalNestedMsgs;
    consumerContext.remainingMessages = totalNestedMsgs;


    //Setup the getter
    Session.pClient = &Client;
    Client.maxInflightLimit = 128;
    Client.expectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
    Consumer.pSession = &Session;
    Consumer.fDestructiveGet = true;
    Consumer.fAcking = (qtype != simple);
    Consumer.fShortDeliveryIds = (qtype == intermediate);
    Consumer.pMsgCallbackFn = CallbackTestNestedPut;
    Consumer.pMsgCallbackContext = &consumerContext;
    Consumer.counts.parts.useCount = 1;

    rc = ieq_initWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);
    acquireConsumerReference(&Consumer); //An enabled waiter counts as a reference
    rc = ieq_enableWaiter(pThreadData, Q, &Consumer, IEQ_ENABLE_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    //Create a message, pubber 0, 0th msg
    ismEngine_MessageHandle_t hMsg = createTestMessage(0,0);

    //Put a message to the Queue without enabling waiter
    rc = ieq_put(pThreadData, Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
    TEST_ASSERT_EQUAL(rc, OK);
    ism_engine_releaseMessage(hMsg);

    //By the time we get here all the messages should have been delivered
    checkTotalMessages(Q, 0);
    TEST_ASSERT_EQUAL(0, consumerContext.remainingMessages);

    waiterDisableCompleted = false;
    waiterTermCompleted = false;

    rc = ieq_termWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieq_delete(pThreadData, &Q, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(3, "Completed %s", TestName);
}

static void testScheduleCheckWaiters(ismQueueType_t qtype)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismQHandle_t Q;
    char TestName[64];
    char qtypeText[16];

    verbose(1, "");
    sprintf(TestName, "%s(%s)...", __func__, ieut_queueTypeText(qtype, qtypeText, sizeof(qtypeText)));
    verbose(1, "Starting %s...", TestName);

    rc = ieq_createQ( pThreadData
                    , qtype
                    , NULL
                    , ieqOptions_DEFAULT
                    , iepi_getDefaultPolicyInfo(true)
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &Q);
    TEST_ASSERT_EQUAL(rc, OK);

    uint64_t previousTER = __sync_add_and_fetch(&(ismEngine_serverGlobal.TimerEventsRequested), 1);

    // The scheduled check waiters will decrement the preDeleteCount, so we need to increase it
    if (qtype == simple)
    {
        iesqQueue_t *pQueue = (iesqQueue_t *)Q;
        __sync_add_and_fetch(&(pQueue->preDeleteCount), 1);
    }
    else if (qtype == intermediate)
    {
        ieiqQueue_t *pQueue = (ieiqQueue_t *)Q;
        __sync_add_and_fetch(&(pQueue->preDeleteCount), 1);
    }
    else
    {
        TEST_ASSERT_EQUAL(qtype, multiConsumer);
        iemqQueue_t *pQueue = (iemqQueue_t *)Q;
        __sync_add_and_fetch(&(pQueue->preDeleteCount), 1);
    }

    ieq_scheduleCheckWaiters(pThreadData, Q);

    // Wait for the call to actually run...
    while(__sync_bool_compare_and_swap(&(ismEngine_serverGlobal.TimerEventsRequested),
                                       previousTER,
                                       previousTER-1) == false)
    {
        sched_yield();
    }

    rc = ieq_delete(pThreadData, &Q, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(3, "Completed %s", TestName);
}

static uint32_t illegalMessagesReceived = 0;
//Used by testDisable and testTerminate
static bool CallbackTestDisable(
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
    uint32_t *msgsReceived = (uint32_t *)pConsumerContext;

    TEST_ASSERT_EQUAL( waiterDisableCompleted,  false );

    if (waiterDisableCompleted)
    {
        //We don't know about the threadsafety of CUNIT ASSERTS
        //So increase a count so the main thread can assert too
        __sync_fetch_and_add(&illegalMessagesReceived, 1);
    }

    (void)__sync_fetch_and_add(msgsReceived, 1);
    ism_engine_releaseMessage(hMessage);

    return true;
}

//Used by testDisable and testTerminate
static void *TestDisablePutter(void *threadarg)
{
    TestDisable_ThreadInfo_t *putterInfo = (TestDisable_ThreadInfo_t *)threadarg;
    int32_t rc;

    int iteration;
    uint32_t msgsPerBlock = 50; //We put this many messages and see if the waiter
                                 //has been disabled, if not we try again

    char ThreadName[16];
    sprintf(ThreadName, "DP(%d)", putterInfo->putterNum);
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&ThreadName);

    ism_engine_threadInit(0);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    for (iteration = 0; iteration < putterInfo->numIterations; iteration++)
    {
        //Wait to be told to start
        rc = pthread_mutex_lock(putterInfo->startMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        while (  ( *(putterInfo->currentIteration) < iteration)
               ||( *(putterInfo->state) == testDisable_setup) )
        {
            rc = pthread_cond_wait(putterInfo->startCond, putterInfo->startMutex);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        rc = pthread_mutex_unlock(putterInfo->startMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        //OK...let's fire bunches of messages until the waiter is disabled
        do
        {
            int i;
            for (i=0; i < msgsPerBlock; i++)
            {
                ismEngine_MessageHandle_t hMsg= createTestMessage( putterInfo->putterNum
                                                                 , i);
                rc = ieq_put(pThreadData, putterInfo->Q, ieqPutOptions_NONE, NULL, hMsg, IEQ_MSGTYPE_REFCOUNT);
                TEST_ASSERT_EQUAL(rc, OK);
                ism_engine_releaseMessage(hMsg);
            }
        }
        while(!(*(putterInfo->stopPutting))); /* BEAM suppression: infinite loop */

        uint32_t puttersRemaining = __sync_sub_and_fetch( putterInfo->puttersInProgress, 1);

        if (puttersRemaining == 0)
        {
            //If we were the last putter in the iteration, declare the iteration done
            //broadcast start publishing to the threads
            rc = pthread_mutex_lock(putterInfo->stopMutex);
            TEST_ASSERT_EQUAL(rc, OK);
            *(putterInfo->state) = testDisable_setup;
            rc = pthread_cond_broadcast(putterInfo->stopCond);
            TEST_ASSERT_EQUAL(rc, OK);
            rc = pthread_mutex_unlock(putterInfo->stopMutex);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    ism_engine_threadTerm(1);
    ism_common_freeTLS();

    return NULL;
}

static void *TestDisableDisabler(void *threadarg)
{
    TestDisable_ThreadInfo_t *putterInfo = (TestDisable_ThreadInfo_t *)threadarg;
    int32_t rc;

    int iteration;

    ism_engine_threadInit(0);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    for (iteration = 0; iteration < putterInfo->numIterations; iteration++)
    {
        //Wait to be told to start
        rc = pthread_mutex_lock(putterInfo->startMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        while (  ( *(putterInfo->currentIteration) < iteration)
               ||( *(putterInfo->state) == testDisable_setup) )
        {
            rc = pthread_cond_wait(putterInfo->startCond, putterInfo->startMutex);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        rc = pthread_mutex_unlock(putterInfo->startMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        //wait til the putters are in progress
        while(*(putterInfo->MsgsReceived) == 0)
        {
            sched_yield();
        }
        //OK... putters are flowing
        ieq_disableWaiter(pThreadData, putterInfo->Q, putterInfo->pConsumer);
    }

    ism_engine_threadTerm(1);

    return NULL;
}


//Check we can disable a queue whilst messages are flowing
static void testDisable(ismQueueType_t qtype)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismQHandle_t Q;
    ismEngine_ClientState_t Client = {{0}};
    ismEngine_Session_t Session = {{0}};
    ismEngine_Consumer_t Consumer = {{0}};
    char TestName[64];
    char qtypeText[16];

    uint32_t numPutters = 3;
    uint32_t numIterations = 2000;
    uint32_t puttersInProgress = 0;
    uint32_t messagesReceived = 0;
    uint32_t currentIteration = 0;

    TestDisable_ThreadInfo_t putterInfo[numPutters];
    TestDisable_ThreadInfo_t disablerInfo;
    pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t stop_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t stop_mutex = PTHREAD_MUTEX_INITIALIZER;
    TestDisable_State_t state = testDisable_setup;

    verbose(1, "");
    sprintf(TestName, "%s(%s)...", __func__, ieut_queueTypeText(qtype, qtypeText, sizeof(qtypeText)));
    verbose(1, "Starting %s...", TestName);

    waiterDisableCompleted = false;
    waiterTermCompleted = false;

    iepiPolicyInfo_t templatePolicyInfo;

    templatePolicyInfo.maxMessageCount = (maxMsgBehaviour == DiscardOldMessages) ? 10 : 0;
    templatePolicyInfo.maxMsgBehavior = maxMsgBehaviour;
    templatePolicyInfo.DCNEnabled = false;

    iepiPolicyInfo_t *disablePolicyInfo = NULL;
    rc = iepi_createPolicyInfo(pThreadData, "disablePolicy", ismSEC_POLICY_QUEUE, false, &templatePolicyInfo, &disablePolicyInfo);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(disablePolicyInfo);
    TEST_ASSERT_EQUAL(disablePolicyInfo->creationState, CreatedByEngine);
    TEST_ASSERT_EQUAL(disablePolicyInfo->maxMessageCount, templatePolicyInfo.maxMessageCount);
    TEST_ASSERT_EQUAL(disablePolicyInfo->maxMessageBytes, 0);

    rc = ieq_createQ( pThreadData
                    , qtype
                    , NULL
                    , ieqOptions_DEFAULT
                    , disablePolicyInfo
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &Q);
    TEST_ASSERT_EQUAL(rc, OK);

    //Setup the getter (but don't start it)
    Session.pClient = &Client;
    Client.maxInflightLimit = 128;
    Client.expectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
    Consumer.pSession = &Session;
    Consumer.fDestructiveGet = true;
    Consumer.fAcking = (qtype != simple);
    Consumer.fShortDeliveryIds = (qtype == intermediate);
    Consumer.pMsgCallbackFn = CallbackTestDisable;
    Consumer.pMsgCallbackContext = &messagesReceived;
    Consumer.counts.parts.useCount = 1;

    rc = ieq_initWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);

    int i;
    for (i=0; i<numPutters; i++)
    {
        //Set up the info for the putter
        putterInfo[i].putterNum = i;
        putterInfo[i].MsgsReceived = &messagesReceived;
        putterInfo[i].numIterations = numIterations;
        putterInfo[i].Q = Q;
        putterInfo[i].currentIteration = &currentIteration;
        putterInfo[i].startCond = &start_cond;
        putterInfo[i].startMutex = &start_mutex;
        putterInfo[i].puttersInProgress = &puttersInProgress;
        putterInfo[i].stopCond = &stop_cond;
        putterInfo[i].stopMutex = &stop_mutex;
        putterInfo[i].state = &state;
        putterInfo[i].stopPutting = &(waiterDisableCompleted);

        //Start the thread
        rc = test_task_startThread(&(putterInfo[i].thread_id),TestDisablePutter, (void *)&(putterInfo[i]),"TestDisablePutter");
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Set up the disabler thread
    disablerInfo.putterNum = 0;
    disablerInfo.MsgsReceived = &messagesReceived;
    disablerInfo.numIterations = numIterations;
    disablerInfo.currentIteration = &currentIteration;
    disablerInfo.Q = Q;
    disablerInfo.startCond = &start_cond;
    disablerInfo.startMutex = &start_mutex;
    disablerInfo.puttersInProgress = &puttersInProgress;
    disablerInfo.stopCond = &stop_cond;
    disablerInfo.stopMutex = &stop_mutex;
    disablerInfo.state = &state;
    disablerInfo.pConsumer = &Consumer;

    //Start the thread
    rc = test_task_startThread(&(disablerInfo.thread_id),TestDisableDisabler, (void *)&(disablerInfo),"TestDisableDisabler");
    TEST_ASSERT_EQUAL(rc, OK);

    for (currentIteration=0; currentIteration < numIterations; currentIteration++)
    {
        //Reset the count of putters...
        puttersInProgress = numPutters;

        //Record we haven't disabled the waiter yet
        waiterDisableCompleted = false;
        waiterTermCompleted = false;

        //(Re-)enable the waiter...
        acquireConsumerReference(&Consumer); //An enabled waiter counts as a reference
        rc = ieq_enableWaiter(pThreadData, Q, &Consumer, IEQ_ENABLE_OPTION_NONE);
        if (rc == ISMRC_DisableWaiterCancel)
        {
            //The enable cancelled the previous disable before it completed..not an error
            rc = OK;
        }
        TEST_ASSERT_EQUAL(rc, OK);

        //Clear the number of messages received
        messagesReceived = 0;

        //broadcast start publishing to the threads
        rc = pthread_mutex_lock(&(start_mutex));
        TEST_ASSERT_EQUAL(rc, OK);
        state = testDisable_running;
        rc = pthread_cond_broadcast(&(start_cond));
        TEST_ASSERT_EQUAL(rc, OK);
        rc = pthread_mutex_unlock(&(start_mutex));
        TEST_ASSERT_EQUAL(rc, OK);

        //wait for all the putting threads to finish
        rc = pthread_mutex_lock(&(stop_mutex));
        TEST_ASSERT_EQUAL(rc, OK);

        while (state == testDisable_running)
        {
            rc = pthread_cond_wait(&(stop_cond), &(stop_mutex));
            TEST_ASSERT_EQUAL(rc, OK);
        }
        rc = pthread_mutex_unlock(&(stop_mutex));
        TEST_ASSERT_EQUAL(rc, OK);

        //print out the queue depth and the number of messages received
        ismEngine_QueueStatistics_t stats;
        ieq_getStats(pThreadData, Q, &stats);
//        printf("Messages received: %d, qdepth %d\n",messagesReceived, stats.BufferedMsgs);
    }
    //Join all the threads
    for (i=0; i<numPutters; i++)
    {
        rc = pthread_join(putterInfo[i].thread_id, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    rc = pthread_join(disablerInfo.thread_id, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieq_termWaiter(pThreadData, Q, &Consumer);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieq_delete(pThreadData, &Q, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(3, "Completed %s", TestName);
}

static void *TestTerminateTerminator(void *threadarg)
{
    TestDisable_ThreadInfo_t *putterInfo = (TestDisable_ThreadInfo_t *)threadarg;
    int32_t rc;

    int iteration;

    char ThreadName[16];
    sprintf(ThreadName, "TT(%d)", putterInfo->putterNum);
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&ThreadName);

    ism_engine_threadInit(0);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    for (iteration = 0; iteration < putterInfo->numIterations; iteration++)
    {
        //Wait to be told to start
        rc = pthread_mutex_lock(putterInfo->startMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        while (  ( *(putterInfo->currentIteration) < iteration)
               ||( *(putterInfo->state) == testDisable_setup) )
        {
            rc = pthread_cond_wait(putterInfo->startCond, putterInfo->startMutex);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        rc = pthread_mutex_unlock(putterInfo->startMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        //wait til the putters are in progress
        while(*(putterInfo->MsgsReceived) == 0)
        {
            sched_yield();
        }
        //OK... putters are flowing
        if(iteration % 2 == 0)
        {
            //On even numbered iteration we disable before terminating...
            ieq_disableWaiter(pThreadData, putterInfo->Q, putterInfo->pConsumer);
        }
        ieq_termWaiter(pThreadData, putterInfo->Q, putterInfo->pConsumer);
    }

    ism_engine_threadTerm(1);
    ism_common_freeTLS();

    return NULL;
}

//Check we can terminate/remove a waiter whilst messages are flowing
static void testTerminate(ismQueueType_t qtype)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismQHandle_t Q;
    ismEngine_ClientState_t Client = {{0}};
    ismEngine_Session_t Session   = {{0}};
    ismEngine_Consumer_t Consumer = {{0}};
    char TestName[64];
    char qtypeText[16];

    uint32_t numPutters = 3;
    uint32_t numIterations = 2000;
    uint32_t puttersInProgress = 0;
    uint32_t messagesReceived = 0;
    uint32_t currentIteration = 0;

    TestDisable_ThreadInfo_t putterInfo[numPutters];
    TestDisable_ThreadInfo_t disablerInfo;
    pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t stop_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t stop_mutex = PTHREAD_MUTEX_INITIALIZER;
    TestDisable_State_t state = testDisable_setup;

    verbose(1, "");
    sprintf(TestName, "%s(%s)...", __func__, ieut_queueTypeText(qtype, qtypeText, sizeof(qtypeText)));
    verbose(1, "Starting %s...", TestName);

    waiterDisableCompleted = false;
    waiterTermCompleted = false;

    iepiPolicyInfo_t templatePolicyInfo;

    templatePolicyInfo.maxMessageCount = (maxMsgBehaviour == DiscardOldMessages) ? 10 : 0;
    templatePolicyInfo.maxMsgBehavior = maxMsgBehaviour;
    templatePolicyInfo.DCNEnabled = false;

    iepiPolicyInfo_t *terminatePolicyInfo = NULL;
    rc = iepi_createPolicyInfo(pThreadData, NULL, ismSEC_POLICY_LAST, false, &templatePolicyInfo, &terminatePolicyInfo);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(terminatePolicyInfo);
    TEST_ASSERT_EQUAL(terminatePolicyInfo->creationState, CreatedByEngine);
    TEST_ASSERT_EQUAL(terminatePolicyInfo->maxMessageCount, templatePolicyInfo.maxMessageCount);
    TEST_ASSERT_EQUAL(terminatePolicyInfo->maxMessageBytes, 0);

    rc = ieq_createQ( pThreadData
                    , qtype
                    , NULL
                    , ieqOptions_DEFAULT
                    , terminatePolicyInfo
                    , ismSTORE_NULL_HANDLE
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &Q);
    TEST_ASSERT_EQUAL(rc, OK);

    //Setup the getter (but don't start it)
    Session.pClient = &Client;
    Client.maxInflightLimit = 128;
    Client.expectedMsgRate = EXPECTEDMSGRATE_DEFAULT;
    Consumer.pSession = &Session;
    Session.PreNackAllCount = 1; //Session counts as +1
    Consumer.fDestructiveGet = true;
    Consumer.fAcking = (qtype != simple);
    Consumer.fShortDeliveryIds = (qtype == intermediate);
    Consumer.pMsgCallbackFn = CallbackTestDisable;
    Consumer.pMsgCallbackContext = &messagesReceived;
    Consumer.counts.parts.useCount = 1;

    int i;
    for (i=0; i<numPutters; i++)
    {
        //Set up the info for the putter
        putterInfo[i].putterNum = i;
        putterInfo[i].MsgsReceived = &messagesReceived;
        putterInfo[i].numIterations = numIterations;
        putterInfo[i].Q = Q;
        putterInfo[i].currentIteration = &currentIteration;
        putterInfo[i].startCond = &start_cond;
        putterInfo[i].startMutex = &start_mutex;
        putterInfo[i].puttersInProgress = &puttersInProgress;
        putterInfo[i].stopCond = &stop_cond;
        putterInfo[i].stopMutex = &stop_mutex;
        putterInfo[i].state = &state;
        putterInfo[i].stopPutting = &(waiterTermCompleted);

        //Start the thread
        rc = test_task_startThread(&(putterInfo[i].thread_id),TestDisablePutter, (void *)&(putterInfo[i]),"TestDisablePutter");
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Set up the disabler thread
    disablerInfo.putterNum = 0;
    disablerInfo.MsgsReceived = &messagesReceived;
    disablerInfo.numIterations = numIterations;
    disablerInfo.currentIteration = &currentIteration;
    disablerInfo.Q = Q;
    disablerInfo.startCond = &start_cond;
    disablerInfo.startMutex = &start_mutex;
    disablerInfo.puttersInProgress = &puttersInProgress;
    disablerInfo.stopCond = &stop_cond;
    disablerInfo.stopMutex = &stop_mutex;
    disablerInfo.state = &state;
    disablerInfo.pConsumer = &Consumer;

    //Start the thread
    rc = test_task_startThread(&(disablerInfo.thread_id),TestTerminateTerminator, (void *)&(disablerInfo),"TestTerminateTerminator");
    TEST_ASSERT_EQUAL(rc, OK);

    for (currentIteration=0; currentIteration < numIterations; currentIteration++)
    {
        //Reset the count of putters...
        puttersInProgress = numPutters;

        //Record we haven't terminated the waiter yet
        waiterDisableCompleted = false;
        waiterTermCompleted = false;

        //(Re-)Init & enable the waiter...
        rc = ieq_initWaiter(pThreadData, Q, &Consumer);
        TEST_ASSERT_EQUAL(rc, OK);
        acquireConsumerReference(&Consumer); //An enabled waiter counts as a reference
        rc = ieq_enableWaiter(pThreadData, Q, &Consumer, IEQ_ENABLE_OPTION_NONE);
        TEST_ASSERT_EQUAL(rc, OK);

        //Clear the number of messages received
        messagesReceived = 0;

        //broadcast start publishing to the threads
        rc = pthread_mutex_lock(&(start_mutex));
        TEST_ASSERT_EQUAL(rc, OK);
        state = testDisable_running;
        rc = pthread_cond_broadcast(&(start_cond));
        TEST_ASSERT_EQUAL(rc, OK);
        rc = pthread_mutex_unlock(&(start_mutex));
        TEST_ASSERT_EQUAL(rc, OK);

        //wait for all the putting threads to finish
        rc = pthread_mutex_lock(&(stop_mutex));
        TEST_ASSERT_EQUAL(rc, OK);

        while (state == testDisable_running)
        {
            rc = pthread_cond_wait(&(stop_cond), &(stop_mutex));
            TEST_ASSERT_EQUAL(rc, OK);
        }
        rc = pthread_mutex_unlock(&(stop_mutex));
        TEST_ASSERT_EQUAL(rc, OK);

        //On even numbered iteration we expect to have disabled then terminated
        if (currentIteration % 2 == 0)
        {
            TEST_ASSERT_EQUAL(waiterDisableCompleted, true);
        }
        TEST_ASSERT_EQUAL(waiterTermCompleted, true);

        //print out the queue depth and the number of messages received
        ismEngine_QueueStatistics_t stats;
        ieq_getStats(pThreadData, Q, &stats);
//        printf("Messages received: %d, qdepth %d\n",messagesReceived, stats.BufferedMsgs);
    }
    //Join all the threads
    for (i=0; i<numPutters; i++)
    {
        rc = pthread_join(putterInfo[i].thread_id, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    rc = pthread_join(disablerInfo.thread_id, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieq_delete(pThreadData, &Q, false);
    TEST_ASSERT_EQUAL(rc, OK);

    verbose(3, "Completed %s", TestName);
}
/*********************************************************************/
/*********************************************************************/
/* Testcase definitions                                              */
/*********************************************************************/
/*********************************************************************/

/*********************************************************************/
/* Simple queue tests                                                */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        testPutSimp                                      */
/* Description:     Test put operation                               */
/* Queue:           Simple                                           */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testPutSimp(void)
{
    testPut(simple);
}

/*********************************************************************/
/* TestCase:        testPutSomeGetFewSimp                            */
/* Description:     Test put & get operation                         */
/* Queue:           Simple                                           */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testPutSomeGetFewSimp(void)
{
    testPutSomeGetFew(simple);
}

/*********************************************************************/
/* TestCase:        testMaxMessagesSimp                              */
/* Description:     Max queue depth operation                        */
/* Queue:           Simple                                           */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testMaxMessagesSimp(void)
{
    testMaxMessages(simple);
}

/*********************************************************************/
/* TestCase:        testFlowSimp                                     */
/* Description:     Flow a large number of messages                  */
/* Queue:           Simple                                           */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testFlowSimp(void)
{
    testFlow(simple);
}

/*********************************************************************/
/* TestCase:        testMultiFlowSimp                                */
/* Description:                                                      */
/* Queue:           Simple                                           */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testMultiFlowSimp(void)
{
    testMultiFlow(simple);
}

/*********************************************************************/
/* TestCase:        testNestedPutSimp                                */
/* Description:     Nested put operation                             */
/* Queue:           Simple                                           */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testNestedPutSimp(void)
{
    testNestedPut(simple);
}

/*********************************************************************/
/* TestCase:        testDisableSimp                                  */
/* Description:     Disable gets                                     */
/* Queue:           Simple                                           */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testDisableSimp(void)
{
    testDisable(simple);
}
/*********************************************************************/
/* TestCase:        testTerminateSimp                                */
/* Description:     terminate a waiter                               */
/* Queue:           simple                                           */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testTerminateSimp(void)
{
    testTerminate(simple);
}

/*********************************************************************/
/* Intermediate queue tests                                          */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        testPutInter                                     */
/* Description:     Test put operation                               */
/* Queue:           Intermediate                                     */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testPutInter( void )
{
    testPut(intermediate);
}

/*********************************************************************/
/* TestCase:        testPutSomeGetFewInter                           */
/* Description:     Test put & get operation                         */
/* Queue:           Intermediate                                     */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testPutSomeGetFewInter(void)
{
    testPutSomeGetFew(intermediate);
}

/*********************************************************************/
/* TestCase:        testMaxMessagesInter                             */
/* Description:     Max queue depth operation                        */
/* Queue:           Intermediate                                     */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testMaxMessagesInter(void)
{
    testMaxMessages(intermediate);
}

/*********************************************************************/
/* TestCase:        testFlowInter                                    */
/* Description:     Flow a large number of messages                  */
/* Queue:           Intermediate                                     */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testFlowInter(void)
{
    testFlow(intermediate);
}

/*********************************************************************/
/* TestCase:        testMultiFlowInter                               */
/* Description:                                                      */
/* Queue:           Intermediate                                     */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testMultiFlowInter(void)
{
    testMultiFlow(intermediate);
}

/*********************************************************************/
/* TestCase:        testNestedPutInter                               */
/* Description:     Nested put operation                             */
/* Queue:           Intermediate                                     */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testNestedPutInter(void)
{
    testNestedPut(intermediate);
}

/*********************************************************************/
/* TestCase:        testDisableInter                                 */
/* Description:     Disable gets                                     */
/* Queue:           Intermediate                                     */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testDisableInter(void)
{
    testDisable(intermediate);
}

/*********************************************************************/
/* TestCase:        testTerminateInter                               */
/* Description:     terminate a waiter                               */
/* Queue:           Intermediate                                     */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testTerminateInter(void)
{
    testTerminate(intermediate);
}

/*********************************************************************/
/* MultiConsumer queue tests                                         */
/*********************************************************************/

/*********************************************************************/
/* TestCase:        testPutMultiConsumer                             */
/* Description:     Test put operation                               */
/* Queue:           MultiConsumer                                    */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testPutMultiConsumer( void )
{
    testPut(multiConsumer);
}

/*********************************************************************/
/* TestCase:        testPutSomeGetFewMultiConsumer                   */
/* Description:     Test put & get operation                         */
/* Queue:           MultiConsumer                                    */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testPutSomeGetFewMultiConsumer(void)
{
    testPutSomeGetFew(multiConsumer);
}

/*********************************************************************/
/* TestCase:        testMaxMessagesMultiConsumer                     */
/* Description:     Max queue depth operation                        */
/* Queue:           MultiConsumer                                    */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testMaxMessagesMultiConsumer(void)
{
    testMaxMessages(multiConsumer);
}

/*********************************************************************/
/* TestCase:        testFlowMultiConsumer                            */
/* Description:     Flow a large number of messages                  */
/* Queue:           MultiConsumer                                    */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testFlowMultiConsumer(void)
{
    testFlow(multiConsumer);
}

/*********************************************************************/
/* TestCase:        testMultiFlowMultiConsumer                       */
/* Description:                                                      */
/* Queue:           MultiConsumer                                    */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testMultiFlowMultiConsumer(void)
{
    testMultiFlow(multiConsumer);
}

/*********************************************************************/
/* TestCase:        testNestedPutMultiConsumer                       */
/* Description:     Nested put operation                             */
/* Queue:           MultiConsumer                                    */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testNestedPutMultiConsumer(void)
{
    testNestedPut(multiConsumer);
}

/*********************************************************************/
/* TestCase:        testScheduleCheckWaitersMultiConsumer            */
/* Description:     Schedule check waiters                           */
/* Queue:           MultiConsumer                                    */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testScheduleCheckWaitersMultiConsumer(void)
{
    testScheduleCheckWaiters(multiConsumer);
}

/*********************************************************************/
/* TestCase:        testDisableMultiConsumer                         */
/* Description:     Disable gets                                     */
/* Queue:           MultiConsumer                                    */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testDisableMultiConsumer(void)
{
    testDisable(multiConsumer);
}

/*********************************************************************/
/* TestCase:        testTerminateMultiConsumer                       */
/* Description:     terminate a waiter                               */
/* Queue:           MultiConsumer                                    */
/* Get Reliability: QoS 0                                            */
/*********************************************************************/
void testTerminateMultiConsumer(void)
{
    testTerminate(multiConsumer);
}

/*********************************************************************/
/* Test suite definitions                                            */
/*********************************************************************/

CU_TestInfo ISM_Engine_CUnit_simpQ_Basic[] = {
    { "testPut", testPutSimp},
    { "testPutSomeGetFew", testPutSomeGetFewSimp},
    { "testMaxMessages", testMaxMessagesSimp },
    { "testFlow", testFlowSimp },
    { "testMultiFlow", testMultiFlowSimp },
    { "testNestedPut", testNestedPutSimp },
    { "testDisable", testDisableSimp },
    { "testTerminate", testTerminateSimp },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Engine_CUnit_interQ_Basic[] = {
    { "testPut", testPutInter },
    { "testPutSomeGetFew", testPutSomeGetFewInter },
    { "testMaxMessages", testMaxMessagesInter },
    { "testFlow", testFlowInter },
    { "testMultiFlow", testMultiFlowInter },
    { "testNestedPut", testNestedPutInter },
    { "testDisable", testDisableInter },
    { "testTerminate", testTerminateInter },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Engine_CUnit_multiConsumerQ_Basic[] = {
    { "testPut", testPutMultiConsumer },
    { "testPutSomeGetFew", testPutSomeGetFewMultiConsumer },
    { "testMaxMessages", testMaxMessagesMultiConsumer },
    { "testFlow", testFlowMultiConsumer },
    { "testMultiFlow", testMultiFlowMultiConsumer },
    { "testNestedPut", testNestedPutMultiConsumer },
    { "testDisable", testDisableMultiConsumer },
    { "testTerminate", testTerminateMultiConsumer },
    { "testScheduleCheckWaiters", testScheduleCheckWaitersMultiConsumer },
    CU_TEST_INFO_NULL
};

CU_SuiteInfo ISM_Engine_CUnit_simpQ_Suites[] = {
    IMA_TEST_SUITE("SimpQBasic", NULL, NULL, ISM_Engine_CUnit_simpQ_Basic),
    IMA_TEST_SUITE("InterQBasic", NULL, NULL, ISM_Engine_CUnit_interQ_Basic),
    IMA_TEST_SUITE("MultiConsumerQBasic", NULL, NULL, ISM_Engine_CUnit_multiConsumerQ_Basic),
    CU_SUITE_INFO_NULL,
};


/*********************************************************************/
/* Main                                                              */
/*********************************************************************/
int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;


    //Decide whether we are going to run in discardOld or Reject newest mode...
    uint64_t seedVal = ism_common_currentTimeNanos();
    srand(seedVal);
    printf("Random Seed =    %"PRId64"\n", seedVal);
    if (rand()%2 == 0)
    {
        printf("Running in discardOldMode\n");
        maxMsgBehaviour = DiscardOldMessages;
    }
    else
    {
        printf("Running in rejectNewestMode\n");
        maxMsgBehaviour = RejectNewMessages;
    }

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    retval = test_engineInit_DEFAULT;
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();
    CU_register_suites(ISM_Engine_CUnit_simpQ_Suites);

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

    int32_t rc = test_engineTerm(true);
    if (retval == 0) retval = rc;

    rc = test_processTerm(retval == 0);
    if (retval == 0) retval = rc;

mod_exit:

    return retval;
}
