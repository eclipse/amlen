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

//         for(int32_t r=0;r<resultCount;r++) { printf("RESULT %d: '%s'\n", r, results[r].queueName); };

/*********************************************************************/
/*                                                                   */
/* Module Name: test_namedQueues.c                                   */
/*                                                                   */
/* Description: Main source file for CUnit test of Queue namespace   */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "engineInternal.h"
#include "engineHashTable.h" // engine hash table functions & constants
#include "engine.h"
#include "store.h"
#include "queueCommon.h"
#include "multiConsumerQ.h"         // access to the iemq functions & constants
#include "multiConsumerQInternal.h" // access to iemqQueue_t
#include "queueNamespace.h"         // access to the ieqn functions & constants

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"
#define COUNT_LOGMESSAGES
#include "test_utils_ismlogs.h"
#include "test_utils_file.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_config.h"

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_dumpClientState                              */
/*                                                                   */
/* Description:    Test ism_engine_dumpClientState                   */
/*                                                                   */
/*********************************************************************/
void test_dumpClientState(const char *clientId)
{
    char tempDirectory[500] = {0};

    test_utils_getTempPath(tempDirectory);
    TEST_ASSERT_NOT_EQUAL(tempDirectory[0], 0);

    strcat(tempDirectory, "/test_clientState_dump_XXXXXX");

    if (mkdtemp(tempDirectory) == NULL)
    {
        TEST_ASSERT(false, ("mkdtemp() failed with %d\n", errno));
    }

    char cwdbuf[500];
    char *cwd = getcwd(cwdbuf, sizeof(cwdbuf));
    TEST_ASSERT_PTR_NOT_NULL(cwd);

    if (chdir(tempDirectory) == 0)
    {
        char tempFilename[strlen(tempDirectory)+30];

        sprintf(tempFilename, "%s/dumpClientState_1", tempDirectory);
        int32_t rc = ism_engine_dumpClientState(clientId, 1, 0, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpClientState_1.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpClientState_9_0", tempDirectory);
        rc = ism_engine_dumpClientState(clientId, 9, 0, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpClientState_9_0.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpClientState_9_50", tempDirectory);
        rc = ism_engine_dumpClientState(clientId, 9, 50, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpClientState_9_50.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpClientState_9_All", tempDirectory);
        rc = ism_engine_dumpClientState(clientId, 9, -1, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpClientState_9_All.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        if(chdir(cwd) != 0)
        {
            TEST_ASSERT(false, ("chdir() back to old cwd failed with errno %d\n", errno));
        }
    }

    test_removeDirectory(tempDirectory);
}

// Allow overriding the ism_store_createRecord function
ismStore_RecordType_t *createRecordIgnoreTypes = NULL;
int32_t ism_store_createRecord(ismStore_StreamHandle_t hStream,
                               const ismStore_Record_t *pRecord,
                               ismStore_Handle_t *pHandle)
{
    int32_t rc = OK;
    bool ignore = false;
    static int32_t (*real_ism_store_createRecord)(ismStore_StreamHandle_t, const ismStore_Record_t *, ismStore_Handle_t *) = NULL;

    if (real_ism_store_createRecord == NULL)
    {
        real_ism_store_createRecord = dlsym(RTLD_NEXT, "ism_store_createRecord");
    }

    if (createRecordIgnoreTypes != NULL)
    {
        ismStore_RecordType_t *ignoreType = createRecordIgnoreTypes;

        for(; *ignoreType; ignoreType++)
        {
            if (*ignoreType == pRecord->Type)
            {
                ignore = true;
                break;
            }
        }
    }

    if (!ignore)
    {
        rc = real_ism_store_createRecord(hStream, pRecord, pHandle);
    }
    else
    {
        // Pass back a 'non-null' handle, emulating a completed request
        *pHandle = (ismStore_Handle_t)(-1);
    }

    return rc;
}

int32_t ism_store_openReferenceContext(ismStore_Handle_t hOwnerHandle,
                                       ismStore_ReferenceStatistics_t *pRefStats,
                                       void **phRefCtxt)
{
    int32_t rc = OK;
    static int32_t (*real_ism_store_openReferenceContext)(ismStore_Handle_t, ismStore_ReferenceStatistics_t *, void **) = NULL;

    if (real_ism_store_openReferenceContext == NULL)
    {
        real_ism_store_openReferenceContext = dlsym(RTLD_NEXT, "ism_store_openReferenceContext");
    }

    if (createRecordIgnoreTypes != NULL && hOwnerHandle == (ismStore_Handle_t)-1)
    {
        *phRefCtxt = (void *)-1;
    }
    else
    {
        rc = real_ism_store_openReferenceContext(hOwnerHandle, pRefStats, phRefCtxt);
    }

    return rc;
}

int32_t ism_store_closeReferenceContext(void *hRefCtxt)
{
    int32_t rc = OK;
    static int32_t (*real_ism_store_closeReferenceContext)(void *) = NULL;

    if (real_ism_store_closeReferenceContext == NULL)
    {
        real_ism_store_closeReferenceContext = dlsym(RTLD_NEXT, "ism_store_closeReferenceContext");
    }

    if (hRefCtxt != (void *)-1)
    {
        rc = real_ism_store_closeReferenceContext(hRefCtxt);
    }

    return rc;
}
/*********************************************************************/
/*                                                                   */
/* Function Name:  test_dumpQueue                                    */
/*                                                                   */
/* Description:    Test ism_engine_dumpQueue.                        */
/*                                                                   */
/*********************************************************************/
void test_dumpQueue(const char *queueName)
{
    char tempDirectory[500] = {0};

    test_utils_getTempPath(tempDirectory);
    TEST_ASSERT_NOT_EQUAL(tempDirectory[0], 0);

    strcat(tempDirectory, "/test_namedQueues_debug_XXXXXX");

    if (mkdtemp(tempDirectory) == NULL)
    {
        TEST_ASSERT(false, ("mkdtemp() failed with %d\n", errno));
    }

    char cwdbuf[500];
    char *cwd = getcwd(cwdbuf, sizeof(cwdbuf));
    TEST_ASSERT_PTR_NOT_NULL(cwd);

    if (chdir(tempDirectory) == 0)
    {
        int32_t rc;
        char tempFilename[strlen(tempDirectory)+30];

        sprintf(tempFilename, "%s/dumpQueue_X", tempDirectory);
        rc = ism_engine_dumpQueue(NULL, 1, 0, tempFilename);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotImplemented);

        sprintf(tempFilename, "%s/dumpQueue_1", tempDirectory);
        rc = ism_engine_dumpQueue(queueName, 1, 0, tempFilename);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpQueue_1.dat"));

        sprintf(tempFilename, "%s/dumpQueue_9_0", tempDirectory);
        rc = ism_engine_dumpQueue(queueName, 9, 0, tempFilename);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpQueue_9_0.dat"));

        sprintf(tempFilename, "%s/dumpQueue_9_50", tempDirectory);
        rc = ism_engine_dumpQueue(queueName, 9, 50, tempFilename);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpQueue_9_50.dat"));

        sprintf(tempFilename, "%s/dumpQueue_9_All", tempDirectory);
        rc = ism_engine_dumpQueue(queueName, 9, -1, tempFilename);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpQueue_9_All.dat"));

        if(chdir(cwd) != 0)
        {
            TEST_ASSERT(false, ("chdir() back to old cwd failed with errno %d\n", errno));
        }
    }

    test_removeDirectory(tempDirectory);
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  queueMessagesCallback                             */
/*                                                                   */
/* Description:    Callback used to test delivery of messages to     */
/*                 named queues.                                     */
/*                                                                   */
/*********************************************************************/
typedef struct tag_queueMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
    int32_t expiring;
} queueMessagesCbContext_t;

bool queueMessagesCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
                           ismEngine_DelivererContext_t *  _delivererContext)
{
    queueMessagesCbContext_t *context = *((queueMessagesCbContext_t **)pContext);

    if (pMsgDetails->Expiry != 0) __sync_fetch_and_add(&context->expiring, 1);
    __sync_fetch_and_add(&context->received, 1);

    TEST_ASSERT_EQUAL(pMsgDetails->Flags & ismMESSAGE_FLAGS_RETAINED, 0);
    TEST_ASSERT_EQUAL(pMsgDetails->Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN, 0);
    TEST_ASSERT_EQUAL(pMsgDetails->Flags & ismMESSAGE_FLAGS_PROPAGATE_RETAINED, 0);

    ism_engine_releaseMessage(hMessage);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        NULL,
                                                        0,
                                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    return true; // more messages, please.
}

//****************************************************************************
/// @brief Test Creation of queue consumer(s) and put a message using
///        ism_engine_putMessageOnDestination
//****************************************************************************
#define NAMEDQUEUE_CONSUMER_QUEUE "TEST.NAMEDQUEUE.CONSUMER"
#define NAMEDQUEUE_CONSUMER_NUMQUEUES (sizeof(NAMEDQUEUE_CONSUMER_QUEUES)/sizeof(NAMEDQUEUE_CONSUMER_QUEUES[0]))
#define NAMEDQUEUE_CONSUMER_MSGS_TO_PUT 10

void test_capability_NamedQueueConsumerBasic(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient1=NULL, hClient2=NULL;
    ismEngine_SessionHandle_t hSession1=NULL, hSession2=NULL;
    ismEngine_ConsumerHandle_t hConsumer1=NULL, hConsumer2=NULL;
    queueMessagesCbContext_t context1={0}, context2={0};
    queueMessagesCbContext_t *cb1=&context1, *cb2=&context2;

    printf("Starting %s...\n", __func__);

    /* Create our clients and sessions */
    rc = test_createClientAndSession("OneClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient1);
    TEST_ASSERT_PTR_NOT_NULL(hSession1);

    rc = test_createClientAndSession("AnotherClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient2);
    TEST_ASSERT_PTR_NOT_NULL(hSession2);

    printf("  ...create\n");

    // Create the queue
    rc = test_configProcessPost("{\"Queue\":{\""NAMEDQUEUE_CONSUMER_QUEUE"\":{\"Name\":\""NAMEDQUEUE_CONSUMER_QUEUE"\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    context1.hSession = hSession1;
    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_QUEUE,
                                   NAMEDQUEUE_CONSUMER_QUEUE,
                                   0, // UNUSED
                                   NULL, // Unused for QUEUE
                                   &cb1,
                                   sizeof(queueMessagesCbContext_t *),
                                   queueMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer1);

    // Try accessing the queue handle of the newly created queue
    ismQHandle_t pNamedQHandle = ieqn_getQueueHandle(pThreadData, NAMEDQUEUE_CONSUMER_QUEUE);
    TEST_ASSERT_PTR_NOT_NULL(pNamedQHandle);
    TEST_ASSERT_EQUAL(((ismEngine_Queue_t *)pNamedQHandle)->QType, multiConsumer);
    // Store handles filled in?
    TEST_ASSERT_NOT_EQUAL(((iemqQueue_t *)pNamedQHandle)->hStoreObj, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL(((iemqQueue_t *)pNamedQHandle)->hStoreProps, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL_FORMAT(((iemqQueue_t *)pNamedQHandle)->QueueRefContext, NULL, "%p");

    // Confirm that this queue is not temporary
    ismEngine_ClientStateHandle_t hCreator = NULL;
    bool bIsTempQueue;
    rc = ieqn_isTemporaryQueue(NAMEDQUEUE_CONSUMER_QUEUE, &bIsTempQueue, &hCreator);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(bIsTempQueue, false);
    TEST_ASSERT_PTR_NULL(hCreator);

    // Ensure we cannot destroy a queue which has an active consumer
    rc = ieqn_destroyQueue(pThreadData, NAMEDQUEUE_CONSUMER_QUEUE, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationInUse);

    // multiConsumer queue (which this uses) can have multiple consumers
    context2.hSession = hSession2;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_QUEUE,
                                   NAMEDQUEUE_CONSUMER_QUEUE,
                                   0, // UNUSED
                                   NULL, // Unused for QUEUE
                                   &cb2,
                                   sizeof(queueMessagesCbContext_t *),
                                   queueMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer2,
                                   NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer2);

    TEST_ASSERT_EQUAL(context1.received, 0);

    for(int32_t msg=0; msg<NAMEDQUEUE_CONSUMER_MSGS_TO_PUT; msg++)
    {
        ismEngine_MessageHandle_t hMessage1=NULL;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_QUEUE, NAMEDQUEUE_CONSUMER_QUEUE,
                                &hMessage1, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage1);

        // Put a message to the queue by name
        rc = ism_engine_putMessageOnDestination(hSession2,
                                                ismDESTINATION_QUEUE,
                                                NAMEDQUEUE_CONSUMER_QUEUE,
                                                NULL,
                                                hMessage1,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    printf("  ...disconnect\n");

    // Check that all messages received once
    TEST_ASSERT_EQUAL(context1.received+context2.received, NAMEDQUEUE_CONSUMER_MSGS_TO_PUT);
    TEST_ASSERT_EQUAL(context1.expiring+context2.expiring, 0);

    // Destroy clients in different orders to cause removal of consumers from the queue
    // in different orders - check subsequent put to remaining consumer.
    if (rand()%2 == 0)
    {
        int32_t prevReceived = context1.received;
        rc = test_destroyClientAndSession(hClient2, hSession2, true);
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_MessageHandle_t hMessage1=NULL;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_QUEUE, NAMEDQUEUE_CONSUMER_QUEUE,
                                &hMessage1, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage1);

#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
        rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_QUEUE,
                                                        NAMEDQUEUE_CONSUMER_QUEUE,
                                                        hMessage1,
                                                        NULL, 0, NULL);
#else
        rc = ism_engine_putMessageOnDestination(hSession1,
                                                ismDESTINATION_QUEUE,
                                                NAMEDQUEUE_CONSUMER_QUEUE,
                                                NULL,
                                                hMessage1,
                                                NULL, 0, NULL);
#endif
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(context1.received, prevReceived+1);
        rc = test_destroyClientAndSession(hClient1, hSession1, false);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    else
    {
        int32_t prevReceived = context2.received;
        rc = test_destroyClientAndSession(hClient1, hSession1, false);
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_MessageHandle_t hMessage1=NULL;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_QUEUE, NAMEDQUEUE_CONSUMER_QUEUE,
                                &hMessage1, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage1);

#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
        rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_QUEUE,
                                                        NAMEDQUEUE_CONSUMER_QUEUE,
                                                        hMessage1,
                                                        NULL, 0, NULL);
#else
        rc = ism_engine_putMessageOnDestination(hSession2,
                                                ismDESTINATION_QUEUE,
                                                NAMEDQUEUE_CONSUMER_QUEUE,
                                                NULL,
                                                hMessage1,
                                                NULL, 0, NULL);
#endif
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(context2.received, prevReceived+1);
        rc = test_destroyClientAndSession(hClient2, hSession2, true);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // TODO: THIS IS A HORRIBLE workaround for DiscardMessages not working...
    #if 1
    ((iemqQueue_t *)ieqn_getQueueHandle(ieut_getThreadData(), NAMEDQUEUE_CONSUMER_QUEUE))->bufferedMsgs = 0;
    #endif

    rc = test_configProcessDelete("Queue", NAMEDQUEUE_CONSUMER_QUEUE, "DiscardMessages=true");
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test Creation of queue producers
//****************************************************************************
char *NAMEDQUEUE_PRODUCER_QUEUES[] = {"TEST.NAMEDQUEUE.PRODUCER"};
#define NAMEDQUEUE_PRODUCER_NUMQUEUES (sizeof(NAMEDQUEUE_PRODUCER_QUEUES)/sizeof(NAMEDQUEUE_PRODUCER_QUEUES[0]))

void test_capability_NamedQueueProducerBasic(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient1=NULL, hClient2=NULL;
    ismEngine_SessionHandle_t hSession1=NULL, hSession2=NULL;
    ismEngine_ProducerHandle_t hProducer1=NULL, hProducer2=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;
    queueMessagesCbContext_t context={0};
    queueMessagesCbContext_t *cb=&context;

    /* Create our clients and sessions */
    rc = test_createClientAndSession("OneClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient1);
    TEST_ASSERT_PTR_NOT_NULL(hSession1);

    rc = test_createClientAndSession("AnotherClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient2);
    TEST_ASSERT_PTR_NOT_NULL(hSession2);

    printf("Starting %s...\n", __func__);

    printf("  ...create\n");

    ieqnQueue_t *pCreatedQueue=NULL;
    // Create the queue
    rc = ieqn_createQueue(pThreadData,
                          NAMEDQUEUE_PRODUCER_QUEUES[0],
                          multiConsumer,
                          ismQueueScope_Server, NULL,
                          NULL,
                          NULL,
                          &pCreatedQueue);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pCreatedQueue);
    // Something in the store...
    iemqQueue_t *createdQHandle = (iemqQueue_t *)ieqn_getQueueHandle(pThreadData, NAMEDQUEUE_PRODUCER_QUEUES[0]);
    TEST_ASSERT_PTR_NOT_NULL(createdQHandle);
    TEST_ASSERT_NOT_EQUAL(createdQHandle->hStoreObj, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL(createdQHandle->hStoreProps, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL_FORMAT(createdQHandle->QueueRefContext, NULL, "%p");

    // Confirm that this queue is not temporary
    ismEngine_ClientStateHandle_t hCreator = NULL;
    bool bIsTempQueue;

    rc = ieqn_isTemporaryQueue(NAMEDQUEUE_CONSUMER_QUEUE, &bIsTempQueue, &hCreator);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(bIsTempQueue, false);
    TEST_ASSERT_PTR_NULL(hCreator);

    rc = ism_engine_createProducer(hSession1,
                                   ismDESTINATION_QUEUE,
                                   NAMEDQUEUE_PRODUCER_QUEUES[0],
                                   &hProducer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer1);

    // Ensure we cannot destroy a queue which has an active producer
    rc = ieqn_destroyQueue(pThreadData, NAMEDQUEUE_PRODUCER_QUEUES[0], ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationInUse);

    rc = ism_engine_createProducer(hSession2,
                                   ismDESTINATION_QUEUE,
                                   NAMEDQUEUE_PRODUCER_QUEUES[0],
                                   &hProducer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer2);

    // Create messages to play with
    void *payload1="Pre-Consumer Producer Message";
    void *payload2="Post-Consumer Producer Message";

    ismEngine_MessageHandle_t hMessage1=NULL, hMessage2=NULL;

    rc = test_createMessage(strlen((char *)payload1)+1,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, NAMEDQUEUE_PRODUCER_QUEUES[0],
                            &hMessage1, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage1);

    rc = ism_engine_putMessage(hSession1,
                               hProducer1,
                               NULL,
                               hMessage1,
                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_dumpClientState("OneClient");

    // Access the queue stats of the queue and check the message is on the queue
    ismEngine_QueueStatistics_t queueStats = {0};
    rc = ieqn_getQueueStats(pThreadData, NAMEDQUEUE_PRODUCER_QUEUES[0], &queueStats);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(queueStats.BufferedMsgs, 1);

    rc = ism_engine_startMessageDelivery(hSession2,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    context.hSession = hSession2;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_QUEUE,
                                   NAMEDQUEUE_PRODUCER_QUEUES[0],
                                   0, // UNUSED
                                   NULL, // Unused for QUEUE
                                   &cb,
                                   sizeof(queueMessagesCbContext_t *),
                                   queueMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);
    TEST_ASSERT_EQUAL(context.received, 1);
    TEST_ASSERT_EQUAL(context.expiring, 0);

    rc = test_createMessage(strlen((char *)payload1)+1,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, NAMEDQUEUE_PRODUCER_QUEUES[0],
                            &hMessage2, &payload2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage2);

    rc = ism_engine_putMessage(hSession2,
                               hProducer2,
                               NULL,
                               hMessage2,
                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.received, 2);
    TEST_ASSERT_EQUAL(context.expiring, 0);

    // Check we cannot put a retained message to a queue
    void *payload3="RETAINED MESSAGE";
    ismEngine_MessageHandle_t hMessage3=NULL;

    rc = test_createMessage(strlen((char *)payload3)+1,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_QUEUE, NAMEDQUEUE_PRODUCER_QUEUES[0],
                            &hMessage3, &payload3);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage3);

    rc = ism_engine_putMessage(hSession2,
                               hProducer2,
                               NULL,
                               hMessage3,
                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestTypeNotValid);
    TEST_ASSERT_EQUAL(context.received, 2);
    TEST_ASSERT_EQUAL(context.expiring, 0);

    printf("  ...disconnect\n");

    rc = ism_engine_destroyProducer(hProducer1,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient2, hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test Creation of queue producers
//****************************************************************************
char *NAMEDQUEUE_TRANSACTION_QUEUES[] = {"TEST.NAMEDQUEUE.TRANSACTION.1", "TEST.NAMEDQUEUE.TRANSACTION.2"};
#define NAMEDQUEUE_TRANSACTION_NUMQUEUES (sizeof(NAMEDQUEUE_TRANSACTION_QUEUES)/sizeof(NAMEDQUEUE_TRANSACTION_QUEUES[0]))

void test_capability_NamedQueueTransaction(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient1=NULL, hClient2=NULL;
    ismEngine_SessionHandle_t hSession1=NULL, hSession2=NULL;
    ismEngine_ConsumerHandle_t hConsumer1=NULL, hConsumer2=NULL;
    queueMessagesCbContext_t context1={0}, context2={0};
    queueMessagesCbContext_t *cb1=&context1, *cb2=&context2;

    printf("Starting %s...\n", __func__);

    /* Create our clients and sessions */
    rc = test_createClientAndSession("OneClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient1);
    TEST_ASSERT_PTR_NOT_NULL(hSession1);

    rc = test_createClientAndSession("AnotherClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient2);
    TEST_ASSERT_PTR_NOT_NULL(hSession2);

    printf("  ...create\n");

    // Create messages to play with
    ismEngine_MessageHandle_t hMessage1=NULL, hMessage2=NULL;

    printf("  ...use\n");

    ismEngine_TransactionHandle_t hTran1 = NULL;

    rc = sync_ism_engine_createLocalTransaction(hSession1,
                                                &hTran1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran1);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                        ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                        ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                        ismMESSAGE_FLAGS_NONE,
                                        0,
                                        ismDESTINATION_QUEUE, NAMEDQUEUE_TRANSACTION_QUEUES[0],
                                        &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage1);

    rc = ism_engine_putMessageOnDestination(hSession1,
                                            ismDESTINATION_QUEUE,
                                            NAMEDQUEUE_TRANSACTION_QUEUES[0],
                                            hTran1,
                                            hMessage1,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    context1.hSession = hSession1;
    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_QUEUE,
                                   NAMEDQUEUE_TRANSACTION_QUEUES[0],
                                   0, // UNUSED
                                   NULL, // Unused for QUEUE
                                   &cb1,
                                   sizeof(queueMessagesCbContext_t *),
                                   queueMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer1);
    TEST_ASSERT_EQUAL(context1.received, 0);

    ismEngine_TransactionHandle_t hTran2 = NULL;

    rc = sync_ism_engine_createLocalTransaction(hSession1, &hTran2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran2);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, NAMEDQUEUE_TRANSACTION_QUEUES[0],
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage1);

    rc = ism_engine_putMessageOnDestination(hSession1,
                                            ismDESTINATION_QUEUE,
                                            NAMEDQUEUE_TRANSACTION_QUEUES[0],
                                            hTran2,
                                            hMessage1,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 0);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, NAMEDQUEUE_TRANSACTION_QUEUES[0],
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage1);

    rc = ism_engine_putMessageOnDestination(hSession1,
                                            ismDESTINATION_QUEUE,
                                            NAMEDQUEUE_TRANSACTION_QUEUES[0],
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 1); // Non-transactional put should work
    TEST_ASSERT_EQUAL(context1.expiring, 0);

    rc = sync_ism_engine_commitTransaction(hSession1,
                                      hTran1,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 2); // Commit of one transaction
    TEST_ASSERT_EQUAL(context1.expiring, 0);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, NAMEDQUEUE_TRANSACTION_QUEUES[0],
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage1);

#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
    rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_QUEUE,
                                                    NAMEDQUEUE_TRANSACTION_QUEUES[0],
                                                    hMessage1,
                                                    NULL, 0, NULL);
#else
    rc = ism_engine_putMessageOnDestination(hSession1,
                                            ismDESTINATION_QUEUE,
                                            NAMEDQUEUE_TRANSACTION_QUEUES[0],
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL);
#endif
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 3); // Second non-transactional put

    rc = ism_engine_rollbackTransaction(hSession1,
                                        hTran2,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 3); // Should not get rolled back message

    // Single transaction spanning multiple queues
    context2.hSession = hSession2;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_QUEUE,
                                   NAMEDQUEUE_TRANSACTION_QUEUES[1],
                                   0, // UNUSED
                                   NULL, // Unused for QUEUE
                                   &cb2,
                                   sizeof(queueMessagesCbContext_t *),
                                   queueMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer2);
    TEST_ASSERT_EQUAL(context2.received, 0);

    ismEngine_TransactionHandle_t hTran3 = NULL;

    rc = sync_ism_engine_createLocalTransaction(hSession2,
                                                &hTran3);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran3);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, NAMEDQUEUE_TRANSACTION_QUEUES[0],
                            &hMessage2, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage2);

    rc = ism_engine_putMessageOnDestination(hSession2,
                                            ismDESTINATION_QUEUE,
                                            NAMEDQUEUE_TRANSACTION_QUEUES[0],
                                            hTran3,
                                            hMessage2,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 3);
    TEST_ASSERT_EQUAL(context2.received, 0);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, NAMEDQUEUE_TRANSACTION_QUEUES[1],
                            &hMessage2, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage2);

    rc = ism_engine_putMessageOnDestination(hSession2,
                                            ismDESTINATION_QUEUE,
                                            NAMEDQUEUE_TRANSACTION_QUEUES[1],
                                            hTran3,
                                            hMessage2,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 3);
    TEST_ASSERT_EQUAL(context2.received, 0);

    rc = sync_ism_engine_commitTransaction(hSession2,
                                      hTran3,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 4);
    TEST_ASSERT_EQUAL(context2.received, 1);

    hTran1 = NULL;
    rc = sync_ism_engine_createLocalTransaction(hSession2,
                                                &hTran1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran1);

    hTran2 = NULL;
    rc = sync_ism_engine_createLocalTransaction(hSession2,
                                                &hTran2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hTran2);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, NAMEDQUEUE_TRANSACTION_QUEUES[1],
                            &hMessage2, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage2);

    rc = ism_engine_putMessageOnDestination(hSession2,
                                            ismDESTINATION_QUEUE,
                                            NAMEDQUEUE_TRANSACTION_QUEUES[1],
                                            hTran1,
                                            hMessage2,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context2.received, 1);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, NAMEDQUEUE_TRANSACTION_QUEUES[1],
                            &hMessage2, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage2);

    rc = ism_engine_putMessageOnDestination(hSession2,
                                            ismDESTINATION_QUEUE,
                                            NAMEDQUEUE_TRANSACTION_QUEUES[1],
                                            hTran2,
                                            hMessage2,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context2.received, 1);

    rc = sync_ism_engine_commitTransaction(hSession2,
                                      hTran2,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context2.received, 2); // hTran2 committed

    rc = sync_ism_engine_commitTransaction(hSession2,
                                      hTran1,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context2.received, 3); // hTran1 committed
    TEST_ASSERT_EQUAL(context1.received, 4);

    printf("  ...disconnect\n");

    rc = ism_engine_destroyConsumer(hConsumer1,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(hConsumer2,
                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient2, hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_NamedQueues_CUnit_test_capability_NamedQueueConsumers[] =
{
    { "NamedQueueConsumers", test_capability_NamedQueueConsumerBasic },
    { "NamedQueueProducers", test_capability_NamedQueueProducerBasic },
    { "NamedQueueTransaction", test_capability_NamedQueueTransaction },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test Basic rehydration of named queues
//****************************************************************************
// This should be a local copy of ieqn_generateQueueNameHash
static uint32_t test_generateQueueNameHash(const char *key)
{
    uint32_t keyHash = 5381;
    char curChar;

    while((curChar = *key++))
    {
        keyHash = (keyHash * 33) ^ curChar;
    }

    return keyHash;
}


void test_capability_NamedQueueRehydration_Phase1(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;

    printf("Starting %s...\n", __func__);

    ieqnQueueNamespace_t *queues = ieqn_getEngineQueueNamespace();
    TEST_ASSERT_PTR_NOT_NULL(queues);

    /* Create our clients and sessions */
    rc = ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);

    printf("  ...create\n");

    char *pQueueName1 = "BASIC.REHDRATION.QUEUE";
    uint32_t queueNameHash1 = test_generateQueueNameHash(pQueueName1);
    char *pQueueName2 = "DELETE.AT.RESTART.QUEUE";
    uint32_t queueNameHash2 = test_generateQueueNameHash(pQueueName2);

    ieqnQueue_t *namedQueue = NULL;

    rc = ieut_getHashEntry(queues->names,
                           pQueueName1,
                           queueNameHash1,
                           (void **)&namedQueue);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(namedQueue);

    rc = ieut_getHashEntry(queues->names,
                           pQueueName2,
                           queueNameHash2,
                           (void **)&namedQueue);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(namedQueue);

    ieqnQueue_t *pCreatedQueue=NULL;
    rc = ieqn_createQueue(pThreadData,
                          pQueueName1,
                          multiConsumer,
                          ismQueueScope_Server, NULL,
                          NULL,
                          NULL,
                          &pCreatedQueue);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieut_getHashEntry(queues->names,
                           pQueueName1,
                           queueNameHash1,
                           (void**)&namedQueue);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(namedQueue);
    TEST_ASSERT_EQUAL_FORMAT(namedQueue, pCreatedQueue, "%p");

    rc = ieqn_createQueue(pThreadData,
                          pQueueName2,
                          multiConsumer,
                          ismQueueScope_Server, NULL,
                          NULL,
                          NULL,
                          &pCreatedQueue);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL_FORMAT(namedQueue, pCreatedQueue, "%p");

    rc = ieut_getHashEntry(queues->names,
                           pQueueName2,
                           queueNameHash2,
                           (void**)&namedQueue);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(namedQueue);
    TEST_ASSERT_EQUAL_FORMAT(namedQueue, pCreatedQueue, "%p");
    TEST_ASSERT_EQUAL(ieq_getQType(namedQueue->queueHandle), multiConsumer);

    // Mark the queue as deleted causing it to be deleted when the store is
    // reloaded - this is faking a queue that wasn't quite deleted when the
    // server ends.
    iemq_markQDeleted(pThreadData, namedQueue->queueHandle, true);
}

void test_capability_NamedQueueRehydration_Phase2(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    ieqnQueueNamespace_t *queues = ieqn_getEngineQueueNamespace();
    TEST_ASSERT_PTR_NOT_NULL(queues);

    char *pQueueName1 = "BASIC.REHDRATION.QUEUE";
    uint32_t queueNameHash1 = test_generateQueueNameHash(pQueueName1);
    char *pQueueName2 = "DELETE.AT.RESTART.QUEUE";
    uint32_t queueNameHash2 = test_generateQueueNameHash(pQueueName2);

    ieqnQueue_t *namedQueue = NULL;

    // Did restarting the engine reconstitute the queues?
    rc = ieut_getHashEntry(queues->names,
                           pQueueName1,
                           queueNameHash1,
                           (void**)&namedQueue);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(namedQueue);

    // This one should not have been recreated (store was flagged for deletion)
    namedQueue = NULL;
    rc = ieut_getHashEntry(queues->names,
                           pQueueName2,
                           queueNameHash2,
                           (void**)&namedQueue);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(namedQueue);

    printf("  ...delete\n");

    rc = ieqn_destroyQueue(pThreadData, pQueueName1, true, false);
    TEST_ASSERT_EQUAL(rc, OK);

    namedQueue = NULL;
    rc = ieut_getHashEntry(queues->names,
                           pQueueName1,
                           queueNameHash1,
                           (void**)&namedQueue);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(namedQueue);
}

void test_capability_NamedQueueRehydration_Phase3(void)
{
    uint32_t rc;

    printf("Starting %s...\n", __func__);

    ieqnQueueNamespace_t *queues = ieqn_getEngineQueueNamespace();
    TEST_ASSERT_PTR_NOT_NULL(queues);

    char *pQueueName1 = "BASIC.REHDRATION.QUEUE";
    uint32_t queueNameHash1 = test_generateQueueNameHash(pQueueName1);

    ieqnQueue_t *namedQueue = NULL;

    queues = ieqn_getEngineQueueNamespace();
    TEST_ASSERT_PTR_NOT_NULL(queues);

    // Did restarting the engine reconstitute the queue?
    namedQueue = NULL;
    rc = ieut_getHashEntry(queues->names,
                           pQueueName1,
                           queueNameHash1,
                           (void**)&namedQueue);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(namedQueue);
}

CU_TestInfo ISM_NamedQueues_CUnit_test_capability_Rehydration_Phase1[] =
{
    { "NamedQueueRehydration", test_capability_NamedQueueRehydration_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_NamedQueues_CUnit_test_capability_Rehydration_Phase2[] =
{
    { "NamedQueueRehydration", test_capability_NamedQueueRehydration_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_NamedQueues_CUnit_test_capability_Rehydration_Phase3[] =
{
    { "NamedQueueRehydration", test_capability_NamedQueueRehydration_Phase3 },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test operation of the DisableAutoQueueCreation config parameter
//****************************************************************************
#define NOAUTO_QUEUE_POLICYNAME "QUEUE.NoAuto.POLICY"
#define NOAUTO_QUEUE_NAME       "QUEUE.NoAuto"

void test_capability_NamedQueuesNoAuto(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;
    ismEngine_ProducerHandle_t hProducer=NULL;

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "SecureClient";

    /* Create our clients and sessions */
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieqn_createQueue(pThreadData,
                          "SHOULD.DISAPPEAR.INTERMEDIATE",
                          intermediate, ismQueueScope_Server,
                          hClient,
                          NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieqn_createQueue(pThreadData,
                          "SHOULD.DISAPPEAR.MULTICONSUMER",
                          multiConsumer,
                          ismQueueScope_Server,
                          hClient,
                          NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismQHandle_t hQueue = ieqn_getQueueHandle(pThreadData, "SHOULD.DISAPPEAR.INTERMEDIATE");
    TEST_ASSERT_PTR_NOT_NULL(hQueue);
    hQueue = ieqn_getQueueHandle(pThreadData, "SHOULD.DISAPPEAR.MULTICONSUMER");
    TEST_ASSERT_PTR_NOT_NULL(hQueue);

    // Try and create a consumer on a non-existent queue
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   NOAUTO_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);
    TEST_ASSERT_PTR_NULL(hConsumer);

    // Try and create a producer on the non-existent queue
    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   NOAUTO_QUEUE_NAME,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);
    TEST_ASSERT_PTR_NULL(hProducer);

    // Create the queue
    rc = test_configProcessPost("{\"Queue\":{\""NOAUTO_QUEUE_NAME"\":{\"Name\":\""NOAUTO_QUEUE_NAME"\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // We are still not authorized... can we create a consumer now?
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   NOAUTO_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);
    TEST_ASSERT_PTR_NULL(hConsumer);

    // Add a messaging policy that allows send/receive on all queues
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_QUEUEPOLICY,
                                "{\"UID\":\"UID."NOAUTO_QUEUE_POLICYNAME"\","
                                "\"Name\":\""NOAUTO_QUEUE_POLICYNAME"\","
                                "\"ClientID\":\"SecureClient\","
                                "\"Queue\":\"*\","
                                "\"ActionList\":\"send,receive\","
                                "\"MaxMessageTimeToLive\":500}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with this context
    rc = test_addPolicyToSecContext(mockContext, NOAUTO_QUEUE_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    // Finally can we create a consumer?
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   NOAUTO_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    // What about a consumer on a non-existent queue?
    ismEngine_ConsumerHandle_t hConsumerFail = NULL;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   "INVALID.QUEUENAME",
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumerFail,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);
    TEST_ASSERT_PTR_NULL(hConsumerFail);

    // Explicit creation again, should be ignored - the net result is a created queue.
    ieqnQueue_t *pCreatedQueue=NULL;
    rc = ieqn_createQueue(pThreadData,
                          NOAUTO_QUEUE_NAME,
                          multiConsumer,
                          ismQueueScope_Server, NULL,
                          NULL,
                          NULL,
                          &pCreatedQueue);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NULL(pCreatedQueue); // Will not get the updated queue

    // Destroy the session & client to ensure our resources are tidied up
    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_deleteSecurityPolicy("UID."NOAUTO_QUEUE_POLICYNAME, NOAUTO_QUEUE_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    // Bounce the server
    test_bounceEngine();

    pThreadData = ieut_getThreadData();

    // Check that we got 3008 messages for each of the queues deleted
    int32_t msg3008Count;
    test_findLogMessages(3008, &msg3008Count);
    TEST_ASSERT_EQUAL(msg3008Count, 2);

    hQueue = ieqn_getQueueHandle(pThreadData, "SHOULD.DISAPPEAR.INTERMEDIATE");
    TEST_ASSERT_PTR_NULL(hQueue);
    hQueue = ieqn_getQueueHandle(pThreadData, "SHOULD.DISAPPEAR.MULTICONSUMER");
    TEST_ASSERT_PTR_NULL(hQueue);

    hConsumer = NULL;

    // Recreate our client
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now can create a producer?
    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   NOAUTO_QUEUE_NAME,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer);
    // Check we picked up MaxMessageTimeToLive
    TEST_ASSERT_EQUAL(((ismEngine_Producer_t *)hProducer)->pPolicyInfo->maxMessageTimeToLive, 500);

    // What about a producer on a non-existent queue?
    ismEngine_ProducerHandle_t hProducerFail=NULL;
    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   "INVALID.QUEUENAME",
                                   &hProducerFail,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);
    TEST_ASSERT_PTR_NULL(hProducerFail);

    // Try and destroy the queue
    rc = test_configProcessDelete("Queue", NOAUTO_QUEUE_NAME, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationInUse); // (ISMRC_ConfigError)

    rc = ism_engine_destroyProducer(hProducer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Destroy the queue
    rc = test_configProcessDelete("Queue", NOAUTO_QUEUE_NAME, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now can we open it? Should not be able to (it was deleted)
    hProducer = NULL;
    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   NOAUTO_QUEUE_NAME,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);
    TEST_ASSERT_PTR_NULL(hProducer);

    // Destroy our client, releasing the producer
    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Bounce the server
    test_bounceEngine();

    pThreadData = ieut_getThreadData();

    // Recreate our client
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now can we open it? Should not be able to (it was deleted)
    hProducer = NULL;
    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   NOAUTO_QUEUE_NAME,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);
    TEST_ASSERT_PTR_NULL(hProducer);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo  ISM_NamedQueues_Cunit_test_capability_NamedQueuesNoAuto[] =
{
    { "NoAutoCreate", test_capability_NamedQueuesNoAuto },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test creation of queues through the admin interface
//****************************************************************************
#define ADMIN_QUEUE_NAME         "Admin.Queue"
#define ADMIN_QUEUE_DESCRIPTION  "Description for 'Admin Queue'"
#define INTERNALLY_CREATED_QUEUE "IntenalQueue"
#define SECOND_ADMIN_QUEUE       "Second.Admin.Queue"

void test_capability_AdminQueues(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;

    printf("Starting %s...\n", __func__);

    iepiPolicyInfo_t *defaultPolicyInfo = iepi_getDefaultPolicyInfo(false);

    ismQHandle_t hQueue = ieqn_getQueueHandle(pThreadData, ADMIN_QUEUE_NAME);
    TEST_ASSERT_PTR_NULL(hQueue);

    // Try updating a queue that doesn't exist... expect failure.
    rc = test_configProcessPost("{\"Queue\":{\""ADMIN_QUEUE_NAME"\":{\"Description\":\""ADMIN_QUEUE_DESCRIPTION"\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Something in the store...
    iemqQueue_t *createdQHandle = (iemqQueue_t *)ieqn_getQueueHandle(pThreadData, ADMIN_QUEUE_NAME);
    TEST_ASSERT_PTR_NOT_NULL(createdQHandle);
    TEST_ASSERT_NOT_EQUAL(createdQHandle->hStoreObj, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL(createdQHandle->hStoreProps, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL_FORMAT(createdQHandle->QueueRefContext, NULL, "%p");

    // Rename of queues is not supported - make sure we return an error if called
    rc = ism_engine_configCallback("Queue",
                                   "THIS SHOULD NOT WORK",
                                   NULL,
                                   ISM_CONFIG_CHANGE_NAME);
    TEST_ASSERT_NOT_EQUAL(rc, OK);

    hQueue = ieqn_getQueueHandle(pThreadData, ADMIN_QUEUE_NAME);
    TEST_ASSERT_PTR_NOT_NULL(hQueue);

    // Create a queue using the internal function, which should disappear after bouncing
    rc = ieqn_createQueue(pThreadData,
                          INTERNALLY_CREATED_QUEUE,
                          multiConsumer,
                          ismQueueScope_Server, NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    hQueue = ieqn_getQueueHandle(pThreadData, INTERNALLY_CREATED_QUEUE);
    TEST_ASSERT_PTR_NOT_NULL(hQueue);

    // Force recovery from the store
    test_bounceEngine();

    pThreadData = ieut_getThreadData();

    // Internally created queue deleted as unreconciled?
    hQueue = ieqn_getQueueHandle(pThreadData, INTERNALLY_CREATED_QUEUE);
    TEST_ASSERT_PTR_NULL(hQueue);

    // Create a second admin queue, with non-default values.
    rc = test_configProcessPost("{\"Queue\":{\""SECOND_ADMIN_QUEUE"\":{\"Description\":\""ADMIN_QUEUE_DESCRIPTION"\","
                                                                      "\"MaxMessages\":22222}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    hQueue = ieqn_getQueueHandle(pThreadData, SECOND_ADMIN_QUEUE);
    TEST_ASSERT_PTR_NOT_NULL(hQueue);

    iepiPolicyInfo_t *pPolicyInfo = ieq_getPolicyInfo(hQueue);
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo);
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, 22222);

    // Force multiple restarts, checking that things come back multiple times.
    for(int32_t i=0; i<5; i++)
    {
        test_bounceEngine();

        pThreadData = ieut_getThreadData();

        hQueue = ieqn_getQueueHandle(pThreadData, INTERNALLY_CREATED_QUEUE);
        TEST_ASSERT_PTR_NULL(hQueue);
        hQueue = ieqn_getQueueHandle(pThreadData, ADMIN_QUEUE_NAME);
        TEST_ASSERT_PTR_NOT_NULL(hQueue);
        pPolicyInfo = ieq_getPolicyInfo(hQueue);
        TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo);
        TEST_ASSERT_NOT_EQUAL_FORMAT(pPolicyInfo, defaultPolicyInfo, "%p");
        TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, ismMAXIMUM_MESSAGES_DEFAULT);
        hQueue = ieqn_getQueueHandle(pThreadData, SECOND_ADMIN_QUEUE);
        TEST_ASSERT_PTR_NOT_NULL(hQueue);
        pPolicyInfo = ieq_getPolicyInfo(hQueue);
        TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo);
        TEST_ASSERT_NOT_EQUAL_FORMAT(pPolicyInfo, defaultPolicyInfo, "%p");
        TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, 22222);
    }

    // Change the second queue's maxMessageCount (note this relies on pPolicyInfo from the
    // preceeding loop pointing to this queue's policy info).
    rc = test_configProcessPost("{\"Queue\":{\""SECOND_ADMIN_QUEUE"\":{\"MaxMessages\":333333}}}");
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, 333333);

    // One more bounce
    test_bounceEngine();

    pThreadData = ieut_getThreadData();

    hQueue = ieqn_getQueueHandle(pThreadData, SECOND_ADMIN_QUEUE);
    TEST_ASSERT_PTR_NOT_NULL(hQueue);
    pPolicyInfo = ieq_getPolicyInfo(hQueue);
    TEST_ASSERT_PTR_NOT_NULL(pPolicyInfo);
    TEST_ASSERT_NOT_EQUAL_FORMAT(pPolicyInfo, defaultPolicyInfo, "%p");
    TEST_ASSERT_EQUAL(pPolicyInfo->maxMessageCount, 333333);

    // Delete both queues
    rc = test_configProcessDelete("Queue", ADMIN_QUEUE_NAME, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_configProcessDelete("Queue", SECOND_ADMIN_QUEUE, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test creation of queues through the admin interface
//****************************************************************************
void test_capability_QueueMonitoring(void)
{
    int32_t rc = OK;

    printf("Starting %s...\n", __func__);

    uint32_t resultCount = 0;
    ismEngine_QueueMonitor_t *results = NULL;

    // Check a call with invalid maxResults
    rc = ism_engine_getQueueMonitor(&results,
                                    &resultCount,
                                    ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    // Create two queues
    rc = test_configProcessPost("{\"Queue\":{\"QMON1\":{\"Name\":\"QMON1\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_configProcessPost("{\"Queue\":{\"QMON2\":{\"Name\":\"QMON2\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Change the stats for QMON1, artificially to large numbers
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    iemqQueue_t *qMon1 = (iemqQueue_t *)ieqn_getQueueHandle(pThreadData, "QMON1");
    TEST_ASSERT_PTR_NOT_NULL(qMon1);

    qMon1->discardedMsgs = 3000000000UL;
    qMon1->bufferedMsgs = 3000000000UL;
    qMon1->enqueueCount = 3000000000UL;
    qMon1->dequeueCount = 3000000000UL;
    qMon1->expiredMsgs = 3000000000UL;

    // Now test valid types where we know the expected order...
    ismEngineMonitorType_t testTypesWithOrder[] = {ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                                   ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS,
                                                   ismENGINE_MONITOR_HIGHEST_PRODUCEDMSGS,
                                                   ismENGINE_MONITOR_LOWEST_PRODUCEDMSGS,
                                                   ismENGINE_MONITOR_HIGHEST_CONSUMEDMSGS,
                                                   ismENGINE_MONITOR_LOWEST_CONSUMEDMSGS,
                                                   ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS,
                                                   ismENGINE_MONITOR_LOWEST_EXPIREDMSGS,
                                                   ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS,
                                                   ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS};

    int32_t type;
    for(type=0; type < (sizeof(testTypesWithOrder)/sizeof(testTypesWithOrder[0])); type++)
    {
        rc = ism_engine_getQueueMonitor(&results,
                                        &resultCount,
                                        testTypesWithOrder[type],
                                        5,
                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 2);
        TEST_ASSERT_STRINGS_EQUAL(results[type%2].queueName, "QMON1");

        ism_engine_freeQueueMonitor(results);
    }

    qMon1->discardedMsgs = 0;
    qMon1->bufferedMsgs = 0;
    qMon1->enqueueCount = 0;
    qMon1->rejectedMsgs = 0;
    qMon1->expiredMsgs = 0;

    ismEngineMonitorType_t testTypes[] = {ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                          ismENGINE_MONITOR_LOWEST_BUFFEREDMSGS,
                                          ismENGINE_MONITOR_HIGHEST_BUFFEREDPERCENT,
                                          ismENGINE_MONITOR_LOWEST_BUFFEREDPERCENT,
                                          ismENGINE_MONITOR_HIGHEST_PRODUCERS,
                                          ismENGINE_MONITOR_LOWEST_PRODUCERS,
                                          ismENGINE_MONITOR_HIGHEST_CONSUMERS,
                                          ismENGINE_MONITOR_LOWEST_CONSUMERS,
                                          ismENGINE_MONITOR_HIGHEST_PRODUCEDMSGS,
                                          ismENGINE_MONITOR_LOWEST_PRODUCEDMSGS,
                                          ismENGINE_MONITOR_HIGHEST_CONSUMEDMSGS,
                                          ismENGINE_MONITOR_LOWEST_CONSUMEDMSGS,
                                          ismENGINE_MONITOR_HIGHEST_BUFFEREDHWMPERCENT,
                                          ismENGINE_MONITOR_LOWEST_BUFFEREDHWMPERCENT,
                                          ismENGINE_MONITOR_HIGHEST_EXPIREDMSGS,
                                          ismENGINE_MONITOR_LOWEST_EXPIREDMSGS,
                                          ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS,
                                          ismENGINE_MONITOR_LOWEST_DISCARDEDMSGS};

    for(type=0; type < (sizeof(testTypes)/sizeof(testTypes[0])); type++)
    {
        rc = ism_engine_getQueueMonitor(&results,
                                        &resultCount,
                                        testTypes[type],
                                        5,
                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 2);

        ism_engine_freeQueueMonitor(results);
    }

    // Try the same but with a filter for queue name & scope
    ism_field_t f;
    ism_prop_t *filterProps = ism_common_newProperties(3);

    f.type = VT_String;
    f.val.s = "*2";
    ism_common_setProperty(filterProps, ismENGINE_MONITOR_FILTER_QUEUENAME, &f);

    f.type = VT_String;
    f.val.s = ismENGINE_MONITOR_FILTER_SCOPE_SERVER;
    ism_common_setProperty(filterProps, ismENGINE_MONITOR_FILTER_SCOPE, &f);

    for(type=0; type < (sizeof(testTypes)/sizeof(testTypes[0])); type++)
    {
        rc = ism_engine_getQueueMonitor(&results,
                                        &resultCount,
                                        testTypes[type],
                                        5,
                                        filterProps);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 1);

        ism_engine_freeQueueMonitor(results);
    }

    // Change scope filter to client
    f.val.s = ismENGINE_MONITOR_FILTER_SCOPE_CLIENT;
    ism_common_setProperty(filterProps, ismENGINE_MONITOR_FILTER_SCOPE, &f);

    for(type=0; type < (sizeof(testTypes)/sizeof(testTypes[0])); type++)
    {
        rc = ism_engine_getQueueMonitor(&results,
                                        &resultCount,
                                        testTypes[type],
                                        5,
                                        filterProps);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, 0);

        ism_engine_freeQueueMonitor(results);
    }

    // Now delete the queues
    rc = test_configProcessDelete("Queue", "QMON1", "DiscardMessages=true");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_configProcessDelete("Queue", "QMON2", "DiscardMessages=true");
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo  ISM_NamedQueues_Cunit_test_capability_AdminQueues[] =
{
    { "AdminQueues", test_capability_AdminQueues },
    { "QueueMonitoring", test_capability_QueueMonitoring },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test deletion of queues and operation of DiscardMessages option
//****************************************************************************
#define DISCARDMSGS_QUEUE_POLICYNAME "DiscardMsgs.Queue.POLICY"
#define DISCARDMSGS_QUEUE_NAME       "DiscardMsgs.Queue"

void test_capability_DiscardMessages(void)
{
    int32_t rc = OK;

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext=NULL;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ProducerHandle_t hProducer=NULL;

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "SecureClient";

    // Create the queue
    rc = test_configProcessPost("{\"Queue\":{\""DISCARDMSGS_QUEUE_NAME"\":{\"Name\":\""DISCARDMSGS_QUEUE_NAME"\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a messaging policy that allows send/receive on queue
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_QUEUEPOLICY,
                                "{\"UID\":\"UID."DISCARDMSGS_QUEUE_POLICYNAME"\","
                                "\"Name\":\""DISCARDMSGS_QUEUE_POLICYNAME"\","
                                "\"ClientID\":\"SecureClient\","
                                "\"Queue\":\""DISCARDMSGS_QUEUE_NAME"*\","
                                "\"ActionList\":\"send,receive\","
                                "\"MaxMessageTimeToLive\":1200}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with this context
    rc = test_addPolicyToSecContext(mockContext, DISCARDMSGS_QUEUE_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    /* Create a client and session */
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   DISCARDMSGS_QUEUE_NAME,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer);
    TEST_ASSERT_EQUAL(((ismEngine_Producer_t *)hProducer)->pPolicyInfo->maxMessageTimeToLive, 1200);

    // Try deleting with a producer still in place
    rc = test_configProcessDelete("Queue", DISCARDMSGS_QUEUE_NAME, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationInUse); // (ISMRC_ConfigError)

    ismEngine_MessageHandle_t hMessage=NULL;

    for(int32_t i=0; i<3; i++)
    {
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_QUEUE, DISCARDMSGS_QUEUE_NAME,
                                &hMessage, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);
        TEST_ASSERT_EQUAL(((ismEngine_Message_t *)hMessage)->Header.Expiry, 0);

        ismEngine_UnreleasedHandle_t hUnrel = NULL;

        // Test put message
        if (i == 0)
        {
            rc = ism_engine_putMessage(hSession,
                                       hProducer,
                                       NULL,
                                       hMessage,
                                       NULL, 0, NULL);
        }
        // Test put message with delivery id
        else if (i == 1)
        {
            rc = ism_engine_putMessageWithDeliveryId(hSession,
                                                     hProducer,
                                                     NULL,
                                                     hMessage,
                                                     1,
                                                     &hUnrel,
                                                     NULL, 0, NULL);
        }
        // Test putMessageWithDeliveryIdOnDestination
        else if (i == 2)
        {
            rc = ism_engine_putMessageWithDeliveryIdOnDestination(hSession,
                                                                  ismDESTINATION_QUEUE,
                                                                  DISCARDMSGS_QUEUE_NAME,
                                                                  NULL,
                                                                  hMessage,
                                                                  2,
                                                                  &hUnrel,
                                                                  NULL, 0, NULL);
        }

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NULL(hUnrel);
        // Check MaxMessageTimeToLive got applied
        TEST_ASSERT_NOT_EQUAL(((ismEngine_Message_t *)hMessage)->Header.Expiry, 0);
    }

    test_dumpQueue(DISCARDMSGS_QUEUE_NAME);

    // Remove the producer from the equation
    rc = ism_engine_destroyProducer(hProducer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Try deleting with a message, without specifying DiscardMessages (test default)
    rc = test_configProcessDelete("Queue", DISCARDMSGS_QUEUE_NAME, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationNotEmpty); // (ISMRC_ConfigError)

    // Try deleting with a message, explicitly specifying DiscardMessages=false
    rc = test_configProcessDelete("Queue", DISCARDMSGS_QUEUE_NAME, "DiscardMessages=false");
    TEST_ASSERT_EQUAL(rc, ISMRC_DestinationNotEmpty); // (ISMRC_ConfigError)

    // TODO: THIS IS A HORRIBLE workaround for DiscardMessages not working...
    #if 1
    ((iemqQueue_t *)ieqn_getQueueHandle(ieut_getThreadData(), DISCARDMSGS_QUEUE_NAME))->bufferedMsgs = 0;
    #endif

    // Delete with a message, explicitly specifying DiscardMessages=true
    rc = test_configProcessDelete("Queue", DISCARDMSGS_QUEUE_NAME, "DiscardMessages=true");
    TEST_ASSERT_EQUAL(rc, OK);

    // Confirm the queue is gone by attempting to create a producer on it
    hProducer = NULL;
    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   DISCARDMSGS_QUEUE_NAME,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);
    TEST_ASSERT_PTR_NULL(hProducer);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo  ISM_NamedQueues_Cunit_test_capability_DiscardMessages[] =
{
    { "DiscardMessages", test_capability_DiscardMessages },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test use of the ConcurrentConsumers options
//****************************************************************************
#define CONCONS_QUEUE_POLICYNAME "ConCons Queue Policy"
#define CONCONS_QUEUE_NAME       "ConCons Queue"

void test_capability_ConcurrentConsumers(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext=NULL;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer1=NULL, hConsumer2=NULL, hConsumer3=NULL;

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "SecureClient";

    // Create the queue queue with concurrentConsumers set to default
    rc = test_configProcessPost("{\"Queue\":{\""CONCONS_QUEUE_NAME"\":{\"Name\":\""CONCONS_QUEUE_NAME"\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a messaging policy that allows send on queue
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_QUEUEPOLICY,
                                "{\"UID\":\"UID."CONCONS_QUEUE_POLICYNAME"\","
                                "\"Name\":\""CONCONS_QUEUE_POLICYNAME"\","
                                "\"ClientID\":\"SecureClient\","
                                "\"Queue\":\""CONCONS_QUEUE_NAME"\","
                                "\"ActionList\":\"send\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with this context
    rc = test_addPolicyToSecContext(mockContext, CONCONS_QUEUE_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    ismQHandle_t hQueue = ieqn_getQueueHandle(pThreadData, CONCONS_QUEUE_NAME);
    TEST_ASSERT_PTR_NOT_NULL(hQueue);
    TEST_ASSERT_PTR_NOT_NULL(hQueue->PolicyInfo);
    TEST_ASSERT_EQUAL(hQueue->PolicyInfo->concurrentConsumers, true);

    /* Create a client and session */
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Only SEND is authorized - this consumer should not get created
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   CONCONS_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);
    TEST_ASSERT_PTR_NULL(hConsumer1);

    // Change to allow only receive
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_QUEUEPOLICY,
                                "{\"UID\":\"UID."CONCONS_QUEUE_POLICYNAME"\","
                                "\"Name\":\""CONCONS_QUEUE_POLICYNAME"\","
                                "\"ClientID\":\"SecureClient\","
                                "\"Queue\":\""CONCONS_QUEUE_NAME"\","
                                "\"ActionList\":\"receive\","
                                "\"Update\":\"true\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Should now be authorized to create consumers
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   CONCONS_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer1);

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   CONCONS_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer2);

    // Try and explicitly disallow concurrent consumers
    rc = test_configProcessPost("{\"Queue\":{\""CONCONS_QUEUE_NAME"\":{\"ConcurrentConsumers\":false}}}");
    TEST_ASSERT_EQUAL(rc, ISMRC_TooManyConsumers); // (ISMRC_ConfigError)
    TEST_ASSERT_EQUAL(hQueue->PolicyInfo->concurrentConsumers, true);

    // Destroy an existing consumer
    rc = ism_engine_destroyConsumer(hConsumer2, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    hConsumer2 = NULL;

    // Explicitly disallow concurrent consumers
    rc = test_configProcessPost("{\"Queue\":{\""CONCONS_QUEUE_NAME"\":{\"ConcurrentConsumers\":false}}}");
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(hQueue->PolicyInfo->concurrentConsumers, false);

    // Try and create a consumer again
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   CONCONS_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_TooManyConsumers);
    TEST_ASSERT_PTR_NULL(hConsumer2);

    // Destroy the existing consumer
    rc = ism_engine_destroyConsumer(hConsumer1, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a consumer
    hConsumer1 = NULL;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   CONCONS_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer1);

    // Try and create a second
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   CONCONS_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_TooManyConsumers);
    TEST_ASSERT_PTR_NULL(hConsumer2);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Bounce the server - ensure the value is rehydrated correctly
    rc = test_bounceEngine();

    pThreadData = ieut_getThreadData();

    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a consumer
    hConsumer1 = NULL;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   CONCONS_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer1);

    // Try and create a second
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   CONCONS_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_TooManyConsumers);
    TEST_ASSERT_PTR_NULL(hConsumer2);

    // Allow concurrent consumers again
    rc = test_configProcessPost("{\"Queue\":{\""CONCONS_QUEUE_NAME"\":{\"ConcurrentConsumers\":true}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a second consumer
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   CONCONS_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer2);

    // Create a third consumer
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   CONCONS_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer3,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer3);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_configProcessDelete("Queue", CONCONS_QUEUE_NAME, "DiscardMessages=true");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo  ISM_NamedQueues_Cunit_test_capability_ConcurrentConsumers[] =
{
    { "ConcurrentConsumers", test_capability_ConcurrentConsumers },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the behaviour of MaxMessages option
//****************************************************************************
#define MAXMESSAGES_QUEUE_POLICYNAME "MaxMessage.QUEUE.PoLiCy"
#define MAXMESSAGES_QUEUE_NAME       "MaxMessages.QUEUE"

void test_capability_MaxMessages(void)
{
    int32_t rc = OK;

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext=NULL;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ProducerHandle_t hProducer=NULL;

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "TestClient";

    // Create the queue queue with concurrentConsumers set to default
    rc = test_configProcessPost("{\"Queue\":{\""MAXMESSAGES_QUEUE_NAME"\":{\"MaxMessages\":2}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a messaging policy that allows receive on queue
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_QUEUEPOLICY,
                                "{\"UID\":\"UID."MAXMESSAGES_QUEUE_POLICYNAME"\","
                                "\"Name\":\""MAXMESSAGES_QUEUE_POLICYNAME"\","
                                "\"ClientID\":\"TestClient\","
                                "\"Queue\":\""MAXMESSAGES_QUEUE_NAME"\","
                                "\"ActionList\":\"receive\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with this context
    rc = test_addPolicyToSecContext(mockContext, MAXMESSAGES_QUEUE_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a client and session
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Only receive (consumer) is authorized
    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   MAXMESSAGES_QUEUE_NAME,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);
    TEST_ASSERT_PTR_NULL(hProducer);

    // Update messaging policy to allow send and receive
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_QUEUEPOLICY,
                                "{\"UID\":\"UID."MAXMESSAGES_QUEUE_POLICYNAME"\","
                                "\"Name\":\""MAXMESSAGES_QUEUE_POLICYNAME"\","
                                "\"ClientID\":\"TestClient\","
                                "\"Queue\":\""MAXMESSAGES_QUEUE_NAME"\","
                                "\"ActionList\":\"send,receive\","
                                "\"Update\":\"true\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Create producer
    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   MAXMESSAGES_QUEUE_NAME,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer);

    ismEngine_MessageHandle_t hMessage1=NULL, hMessage2=NULL;

    // Put an EXACTLY_ONCE reliabiliry message 4 times (2 should work, 2 fail)
    for(int32_t i=0; i<4; i++)
    {
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_QUEUE, MAXMESSAGES_QUEUE_NAME,
                                &hMessage1, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage1);

        if (rand()%2 == 1)
        {
            rc = ism_engine_putMessage(hSession,
                                       hProducer,
                                       NULL,
                                       hMessage1,
                                       NULL, 0, NULL);
        }
        else
        {
#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
            if (i == 0)
            {
                rc = ism_engine_putMessageOnDestination(hSession,
                                                        ismDESTINATION_QUEUE,
                                                        MAXMESSAGES_QUEUE_NAME,
                                                        NULL,
                                                        hMessage1,
                                                        NULL, 0, NULL);
            }
            else
            {
                rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_QUEUE,
                                                                MAXMESSAGES_QUEUE_NAME,
                                                                hMessage1,
                                                                NULL, 0, NULL);
            }
#else
            rc = ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_QUEUE,
                                                    MAXMESSAGES_QUEUE_NAME,
                                                    NULL,
                                                    hMessage1,
                                                    NULL, 0, NULL);
#endif
        }

        if (i<2)
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_DestinationFull);
        }
    }

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, MAXMESSAGES_QUEUE_NAME,
                            &hMessage2, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage2);

    // Put an AT_MOST_ONCE message to a full queue
    rc = ism_engine_putMessage(hSession,
                               hProducer,
                               NULL,
                               hMessage2,
                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_SomeDestinationsFull);

    queueMessagesCbContext_t context={0};
    queueMessagesCbContext_t *cb=&context;
    ismEngine_ConsumerHandle_t hConsumer=NULL;

    // Create a consumer to drain the queue
    context.hSession = hSession;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   MAXMESSAGES_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &cb, sizeof(queueMessagesCbContext_t *), queueMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);
    TEST_ASSERT_EQUAL(context.received, 2);
    TEST_ASSERT_EQUAL(context.expiring, 0);

    rc = ism_engine_stopMessageDelivery(hSession, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, MAXMESSAGES_QUEUE_NAME,
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage1);

    // Put an EXACTLY_ONCE message now the queue is no longer full
    rc = ism_engine_putMessage(hSession,
                               hProducer,
                               NULL,
                               hMessage1,
                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.received, 3);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    // Delete the queue
    rc = test_configProcessDelete("Queue", MAXMESSAGES_QUEUE_NAME, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo  ISM_NamedQueues_Cunit_test_capability_MaxMessages[] =
{
    { "MaxMessages", test_capability_MaxMessages },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the behaviour of the AllowSend option
//****************************************************************************
#define ALLOWSEND_QUEUE_POLICYNAME "Policy For allowSend test"
#define ALLOWSEND_QUEUE_NAME       "AllowSend.QUEUE"

void test_capability_AllowSend(void)
{
    int32_t rc = OK;

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext=NULL;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ProducerHandle_t hProducer1=NULL, hProducer2=NULL;

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "SecureClient";

    // Create the queue queue with allowSend set to default
    rc = test_configProcessPost("{\"Queue\":{\""ALLOWSEND_QUEUE_NAME"\":{\"Name\":\""ALLOWSEND_QUEUE_NAME"\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a messaging policy that allows * send, receive & browse on * and enforces
    // a MaxMessageTimeToLive of 60 seconds
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_QUEUEPOLICY,
                                "{\"UID\":\"UID."ALLOWSEND_QUEUE_POLICYNAME"\","
                                "\"Name\":\""ALLOWSEND_QUEUE_POLICYNAME"\","
                                "\"ClientID\":\"*\","
                                "\"Queue\":\"*\","
                                "\"ActionList\":\"receive,send,browse\","
                                "\"MaxMessageTimeToLive\":60}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with this context
    rc = test_addPolicyToSecContext(mockContext, ALLOWSEND_QUEUE_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a client and session
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   ALLOWSEND_QUEUE_NAME,
                                   &hProducer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer1);

    ismEngine_MessageHandle_t hMessage1=NULL, hMessage2=NULL;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, ALLOWSEND_QUEUE_NAME,
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage1);

    int32_t expectMsgs=0;
    rc = ism_engine_putMessage(hSession,
                               hProducer1,
                               NULL,
                               hMessage1,
                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    expectMsgs++;

    // Set allowSend to false
    rc = test_configProcessPost("{\"Queue\":{\""ALLOWSEND_QUEUE_NAME"\":{\"AllowSend\":false}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i<2; i++)
    {
        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_QUEUE, ALLOWSEND_QUEUE_NAME,
                                &hMessage1, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage1);

        rc = ism_engine_putMessage(hSession,
                                   hProducer1,
                                   NULL,
                                   hMessage1,
                                   NULL, 0, NULL);
        if (i == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_SendNotAllowed);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            expectMsgs++;
        }

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_QUEUE, ALLOWSEND_QUEUE_NAME,
                                &hMessage1, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage1);
        TEST_ASSERT_EQUAL(((ismEngine_Message_t *)hMessage1)->Header.Expiry, 0);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_QUEUE,
                                                ALLOWSEND_QUEUE_NAME,
                                                NULL,
                                                hMessage1,
                                                NULL, 0, NULL);
        if (i == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_SendNotAllowed);

        }
        else
        {
            // Check MaxTTL got applied
            TEST_ASSERT_NOT_EQUAL(((ismEngine_Message_t *)hMessage1)->Header.Expiry, 0);
            TEST_ASSERT_EQUAL(rc, OK);
            expectMsgs++;
        }

        rc = ism_engine_createProducer(hSession,
                                       ismDESTINATION_QUEUE,
                                       ALLOWSEND_QUEUE_NAME,
                                       &hProducer2,
                                       NULL, 0, NULL);
        if (i == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_SendNotAllowed);
            TEST_ASSERT_PTR_NULL(hProducer2);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hProducer2);

            rc = ism_engine_destroyProducer(hProducer2, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_QUEUE, ALLOWSEND_QUEUE_NAME,
                                &hMessage2, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage2);

        // AT_MOST_ONCE messages ignore the failure to put the message.
        rc = ism_engine_putMessage(hSession,
                                   hProducer1,
                                   NULL,
                                   hMessage2,
                                   NULL, 0, NULL);

        // Only the 2nd will result in a message
        if (i == 1)
        {
            TEST_ASSERT_EQUAL(rc, OK);
            expectMsgs++;
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_SomeDestinationsFull);
        }

        // Set allowSend to true
        rc = test_configProcessPost("{\"Queue\":{\""ALLOWSEND_QUEUE_NAME"\":{\"AllowSend\":true}}}");
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Create a consumer to drain the queue
    queueMessagesCbContext_t context={0};
    queueMessagesCbContext_t *cb=&context;
    ismEngine_ConsumerHandle_t hConsumer=NULL;

    context.hSession = hSession;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   ALLOWSEND_QUEUE_NAME,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &cb, sizeof(queueMessagesCbContext_t *), queueMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);
    TEST_ASSERT_EQUAL(context.received, expectMsgs);
    TEST_ASSERT_EQUAL(context.expiring, expectMsgs); // Should ALL have an expiry

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_configProcessDelete("Queue", ALLOWSEND_QUEUE_NAME, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo  ISM_NamedQueues_Cunit_test_capability_AllowSend[] =
{
    { "AllowSend", test_capability_AllowSend },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test what happens if during restart, an QPR is seen without an QDR
//****************************************************************************
void test_capability_DiskGenerationQueue(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    iemqQueue_t *createdQHandle = NULL;
    ieqnQueue_t *pCreatedQueue=NULL;

    printf("Starting %s...\n", __func__);

    // Create one queue with a QDR & QPR.
    rc = test_configProcessPost("{\"Queue\":{\"Keep_DiskGenerationQueue\":{\"Name\":\"Keep_DiskGenerationQueue\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_configProcessPost("{\"Queue\":{\"Lose_DiskGenerationQueue\":{\"Name\":\"Lose_DiskGenerationQueue\"}}}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Check it's a multiConsumer queue
    createdQHandle = (iemqQueue_t *)ieqn_getQueueHandle(pThreadData, "Lose_DiskGenerationQueue");
    TEST_ASSERT_PTR_NOT_NULL(createdQHandle);
    TEST_ASSERT_EQUAL(ieq_getQType(createdQHandle), multiConsumer);

    // Destroy one of the queues (internally)
    rc = ieqn_destroyQueue(pThreadData, "Lose_DiskGenerationQueue", ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);
    createdQHandle = (iemqQueue_t *)ieqn_getQueueHandle(pThreadData, "Lose_DiskGenerationQueue");
    TEST_ASSERT_PTR_NULL(createdQHandle);

    // Make ism_store_createRecord ignore QDR creation.
    ismStore_RecordType_t ignoreQDR[] = {ISM_STORE_RECTYPE_QUEUE, 0};
    createRecordIgnoreTypes = ignoreQDR;

    // Recreate as a simple queue queue with no QDR (to simulate deletion when QPR is in disk generation)
    rc = ieqn_createQueue(pThreadData,
                          "Lose_DiskGenerationQueue",
                          intermediate,
                          ismQueueScope_Server, NULL,
                          NULL,
                          NULL,
                          &pCreatedQueue);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pCreatedQueue);

    // All back to normal
    createRecordIgnoreTypes = NULL;

    // Check that both queues exist, and are the right type
    createdQHandle = (iemqQueue_t *)ieqn_getQueueHandle(pThreadData, "Keep_DiskGenerationQueue");
    TEST_ASSERT_PTR_NOT_NULL(createdQHandle);
    TEST_ASSERT_EQUAL(ieq_getQType(createdQHandle), multiConsumer);
#ifndef ENGINE_FORCE_INTERMEDIATE_TO_MULTI
    createdQHandle = (iemqQueue_t *)ieqn_getQueueHandle(pThreadData, "Lose_DiskGenerationQueue");
    TEST_ASSERT_PTR_NOT_NULL(createdQHandle);
    TEST_ASSERT_EQUAL(ieq_getQType(createdQHandle), intermediate);
#endif
    // Bounce the server
    test_bounceEngine();

    pThreadData = ieut_getThreadData();

    // Check that the unmodified queue exists
    createdQHandle = (iemqQueue_t *)ieqn_getQueueHandle(pThreadData, "Keep_DiskGenerationQueue");
    TEST_ASSERT_PTR_NOT_NULL(createdQHandle);
    TEST_ASSERT_EQUAL(ieq_getQType(createdQHandle), multiConsumer);
    // Check that the modified queue does exist (because it will have been recreated
    // because an admin queue still exists) but that it is now multiConsumer again...
    // This means it's been through a cycle of being orphaned, and recreated - two
    // code paths we don't often hit.
    createdQHandle = (iemqQueue_t *)ieqn_getQueueHandle(pThreadData, "Lose_DiskGenerationQueue");
    TEST_ASSERT_PTR_NOT_NULL(createdQHandle);
    TEST_ASSERT_EQUAL(ieq_getQType(createdQHandle), multiConsumer);
}

CU_TestInfo ISM_NamedQueues_Cunit_test_capability_DiskGenerationQueue[] =
{
    { "DiskGenerationQueue", test_capability_DiskGenerationQueue },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initNamedQueues(void)
{
    // We don't want queues created in this test to fail reconciliation
    return test_engineInit_COLDAUTO;
}

int initNamedQueuesWarm(void)
{
    // We don't want queues created in this test to fail reconciliation
    return test_engineInit_WARMAUTO;
}

int initNamedQueues_NoAuto(void)
{
    return test_engineInit(true, false,
                           true, // Disable Auto creation of named queues
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termNamedQueues(void)
{
    return test_engineTerm(true);
}

int termNamedQueuesWarm(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_NamedQueues_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("NamedQueues", initNamedQueues, termNamedQueues, ISM_NamedQueues_CUnit_test_capability_NamedQueueConsumers),
    IMA_TEST_SUITE("NoAutoCreate", initNamedQueues_NoAuto, termNamedQueues, ISM_NamedQueues_Cunit_test_capability_NamedQueuesNoAuto),
    IMA_TEST_SUITE("AdminQueues", initNamedQueues_NoAuto, termNamedQueues, ISM_NamedQueues_Cunit_test_capability_AdminQueues),
    IMA_TEST_SUITE("DiscardMessages", initNamedQueues_NoAuto, termNamedQueues, ISM_NamedQueues_Cunit_test_capability_DiscardMessages),
    IMA_TEST_SUITE("ConcurrentConsumers", initNamedQueues_NoAuto, termNamedQueues, ISM_NamedQueues_Cunit_test_capability_ConcurrentConsumers),
    IMA_TEST_SUITE("MaxMessages", initNamedQueues_NoAuto, termNamedQueues, ISM_NamedQueues_Cunit_test_capability_MaxMessages),
    IMA_TEST_SUITE("AllowSend", initNamedQueues_NoAuto, termNamedQueues, ISM_NamedQueues_Cunit_test_capability_AllowSend),
    IMA_TEST_SUITE("DiskGenerationQueue", initNamedQueues_NoAuto, termNamedQueues, ISM_NamedQueues_Cunit_test_capability_DiskGenerationQueue),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_NamedQueues_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("Rehydration", initNamedQueues, termNamedQueuesWarm, ISM_NamedQueues_CUnit_test_capability_Rehydration_Phase1),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_NamedQueues_CUnit_phase3suites[] =
{
    IMA_TEST_SUITE("Rehydration", initNamedQueuesWarm, termNamedQueuesWarm, ISM_NamedQueues_CUnit_test_capability_Rehydration_Phase2),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo ISM_NamedQueues_CUnit_phase4suites[] =
{
    IMA_TEST_SUITE("Rehydration", initNamedQueuesWarm, termNamedQueues, ISM_NamedQueues_CUnit_test_capability_Rehydration_Phase3),
    CU_SUITE_INFO_NULL
};

CU_SuiteInfo *PhaseSuites[] = { ISM_NamedQueues_CUnit_phase1suites
                              , ISM_NamedQueues_CUnit_phase2suites
                              , ISM_NamedQueues_CUnit_phase3suites
                              , ISM_NamedQueues_CUnit_phase4suites };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
