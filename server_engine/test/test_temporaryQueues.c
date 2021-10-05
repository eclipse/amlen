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
/* Module Name: test_temporaryQueues.c                               */
/*                                                                   */
/* Description: Main source file for CUnit test of temporary queues  */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "engineInternal.h"
#include "engine.h"
#include "queueNamespace.h"          // access to the ieqn functions & constants
#include "engineStore.h"             // To satisfy dependency in multiConsumerQInternal.h
#include "multiConsumerQInternal.h"  // iemqQueue_t

#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"
#include "test_utils_config.h"

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
                           void *                          pContext)
{
    queueMessagesCbContext_t *context = *((queueMessagesCbContext_t **)pContext);

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
/// @brief Test basic operation of temporary queues
//****************************************************************************
#define TEMP_QUEUE1 "_TQ/MadeUpClientId/1234560001"
#define TEMP_QUEUE2 "_TQ/MadeUpOtherClient/987650001"
#define TEMP_QUEUE3 "_TQ/BlahBlah/1111110010"

void test_capability_TemporaryQueueCreation(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient1=NULL, hClient2=NULL;
    ismEngine_SessionHandle_t hSession1=NULL, hSession2=NULL;

    printf("Starting %s...\n", __func__);

    /* Create two clients */
    rc = test_createClientAndSession("CLIENTONE",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient1);
    TEST_ASSERT_PTR_NOT_NULL(hSession1);

    rc = test_createClientAndSession("CLIENTTWO",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient2);
    TEST_ASSERT_PTR_NOT_NULL(hSession2);

    printf("  ...create\n");

    // Create a temporary queue on CLIENTONE
    rc = ism_engine_createTemporaryDestination(hClient1,
                                               ismDESTINATION_QUEUE,
                                               TEMP_QUEUE1,
                                               NULL,
                                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismQHandle_t pNamedQHandle = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE1);
    TEST_ASSERT_PTR_NOT_NULL(pNamedQHandle);
    TEST_ASSERT_EQUAL(((ismEngine_Queue_t *)pNamedQHandle)->QType, multiConsumer);
    TEST_ASSERT_EQUAL(((ismEngine_Queue_t *)pNamedQHandle)->PolicyInfo->maxMessageCount, ismMAXIMUM_MESSAGES_DEFAULT);
    // Nothing in the store...
    TEST_ASSERT_EQUAL(((iemqQueue_t *)pNamedQHandle)->hStoreObj, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_EQUAL(((iemqQueue_t *)pNamedQHandle)->hStoreProps, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_EQUAL_FORMAT(((iemqQueue_t *)pNamedQHandle)->QueueRefContext, NULL, "%p");

    // Verify that this is a temporary queue
    ismEngine_ClientStateHandle_t hCreator = NULL;
    bool bIsTempQueue;
    rc = ieqn_isTemporaryQueue(TEMP_QUEUE1, &bIsTempQueue, &hCreator);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(bIsTempQueue, true);
    TEST_ASSERT_EQUAL_FORMAT(hCreator, hClient1, "%p");

    // Modify the queue
    ism_prop_t *pProperties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(pProperties);
    ism_field_t f;
    f.type = VT_Integer;
    f.val.i = 50;
    rc = ism_common_setProperty(pProperties, ismENGINE_ADMIN_PROPERTY_MAXMESSAGES, &f);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createTemporaryDestination(hClient1,
                                               ismDESTINATION_QUEUE,
                                               TEMP_QUEUE1,
                                               pProperties,
                                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(((ismEngine_Queue_t *)pNamedQHandle)->PolicyInfo->maxMessageCount, 50);

    // Try and create the same name on a different client
    rc = ism_engine_createTemporaryDestination(hClient2,
                                               ismDESTINATION_QUEUE,
                                               TEMP_QUEUE1,
                                               pProperties,
                                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);

    // Try and create the same name via admin
    rc = test_configProcessPost("{\"Queue\":{\""TEMP_QUEUE1"\":{\"Name\":\""TEMP_QUEUE1"\"}}}");
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid); // (ISMRC_ConfigError)

    // Create a second temporary queue on CLIENTONE, specifying properties
    f.type = VT_Boolean;
    f.val.i = 0;
    rc = ism_common_setProperty(pProperties, ismENGINE_ADMIN_PROPERTY_ALLOWSEND, &f);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createTemporaryDestination(hClient1,
                                               ismDESTINATION_QUEUE,
                                               TEMP_QUEUE2,
                                               pProperties,
                                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismQHandle_t pNamedQHandle2 = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE2);
    TEST_ASSERT_PTR_NOT_NULL(pNamedQHandle2);
    TEST_ASSERT_EQUAL(((ismEngine_Queue_t *)pNamedQHandle2)->QType, multiConsumer);
    TEST_ASSERT_EQUAL(((ismEngine_Queue_t *)pNamedQHandle2)->PolicyInfo->maxMessageCount, 50);
    TEST_ASSERT_EQUAL(((ismEngine_Queue_t *)pNamedQHandle2)->PolicyInfo->allowSend, false);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_destroyClientAndSession(hClient2, hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Can we still see a temporary queue?
    pNamedQHandle = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE1);
    TEST_ASSERT_PTR_NULL(pNamedQHandle);
    pNamedQHandle2 = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE2);
    TEST_ASSERT_PTR_NULL(pNamedQHandle2);

    ism_common_freeProperties(pProperties);
}

void test_capability_TemporaryQueueUsage(void)
{
    uint32_t rc;

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;
    ismEngine_ClientStateHandle_t hClient1=NULL, hClient2=NULL;
    ismEngine_SessionHandle_t hSession1=NULL, hSession2=NULL;
    ismEngine_ProducerHandle_t hProducer1=NULL, hProducer2=NULL;
    ismEngine_ConsumerHandle_t hConsumer1=NULL, hConsumer2=NULL;
    queueMessagesCbContext_t context1={0}, context2={0};
    queueMessagesCbContext_t *cb1=&context1, *cb2=&context2;

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "CLIENTTWO";

    /* Create two clients */
    rc = test_createClientAndSession("CLIENTONE",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient1);
    TEST_ASSERT_PTR_NOT_NULL(hSession1);

    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient2);
    TEST_ASSERT_PTR_NOT_NULL(hSession2);

    printf("  ...create\n");

    // Create a temporary queue on CLIENTONE
    rc = ism_engine_createTemporaryDestination(hClient1,
                                               ismDESTINATION_QUEUE,
                                               TEMP_QUEUE1,
                                               NULL,
                                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a producer on CLIENTTWO
    rc = ism_engine_createProducer(hSession2,
                                   ismDESTINATION_QUEUE,
                                   TEMP_QUEUE1,
                                   &hProducer1,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer1);

    // Create a consumer on CLIENTONE
    context1.hSession = hSession1;
    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_QUEUE,
                                   TEMP_QUEUE1,
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

    // Try and create a consumer on CLIENTTWO
    context2.hSession = hSession2;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_QUEUE,
                                   TEMP_QUEUE1,
                                   0, // UNUSED
                                   NULL, // Unused for QUEUE
                                   &cb2,
                                   sizeof(queueMessagesCbContext_t *),
                                   queueMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);
    TEST_ASSERT_PTR_NULL(hConsumer2);

    // Create a persistent high QoS message to play with
    ismEngine_MessageHandle_t hMessage1;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, TEMP_QUEUE1,
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage1);

    rc = ism_engine_putMessage(hSession2,
                               hProducer1,
                               NULL,
                               hMessage1,
                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 1);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, TEMP_QUEUE1,
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage1);

    rc = ism_engine_putMessageOnDestination(hSession1,
                                            ismDESTINATION_QUEUE,
                                            TEMP_QUEUE1,
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 2);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, TEMP_QUEUE1,
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage1);

#ifdef NDEBUG // Test non-standard use of ism_engine_putMessageInternalOnDestination
    rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_QUEUE,
                                                    TEMP_QUEUE1,
                                                    hMessage1,
                                                    NULL, 0, NULL);
#else
    rc = ism_engine_putMessageOnDestination(hSession1,
                                            ismDESTINATION_QUEUE,
                                            TEMP_QUEUE1,
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL);
#endif
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 3);

    printf("  ...disconnect\n");

    // Destroy the creating client
    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, TEMP_QUEUE1,
                            &hMessage1, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage1);

    // Still able to put messages with existing producer - but none
    // should be received by consumer
    rc = ism_engine_putMessage(hSession2,
                               hProducer1,
                               NULL,
                               hMessage1,
                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context1.received, 3);

    // Try and create a new producer
    rc = ism_engine_createProducer(hSession2,
                                   ismDESTINATION_QUEUE,
                                   TEMP_QUEUE1,
                                   &hProducer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);

    // Destroy the session owning the producer
    rc = test_destroyClientAndSession(hClient2, hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_TemporaryQueueDeletion(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;

    printf("Starting %s...\n", __func__);

    // Create a client
    rc = test_createClientAndSession("SomeClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Try deleting a queue that does not yet exist
    rc = ism_engine_destroyTemporaryDestination(hClient,
                                                ismDESTINATION_QUEUE,
                                                TEMP_QUEUE1,
                                                NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // Create a temporary queue
    rc = ism_engine_createTemporaryDestination(hClient,
                                               ismDESTINATION_QUEUE,
                                               TEMP_QUEUE1,
                                               NULL,
                                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    ismQHandle_t pNamedQHandle = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE1);
    TEST_ASSERT_PTR_NOT_NULL(pNamedQHandle);

    // Emulate an administrative deletion of the temporary queue - this should fail
    rc = ieqn_destroyQueue(pThreadData, TEMP_QUEUE1, ieqnDiscardMessages, true);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestTypeNotValid);

    // Explicitly delete the temporary queue that now does exist
    rc = ism_engine_destroyTemporaryDestination(hClient,
                                                ismDESTINATION_QUEUE,
                                                TEMP_QUEUE1,
                                                NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    pNamedQHandle = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE1);
    TEST_ASSERT_PTR_NULL(pNamedQHandle);

    // Destroy the client (which should now have no temporary queues)
    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int outerLoop=0; outerLoop<10; outerLoop++)
    {
        // Recreate our client
        rc = test_createClientAndSession("SomeClient",
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient, &hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hClient);
        TEST_ASSERT_PTR_NOT_NULL(hSession);

        // Create three temporary queues
        rc = ism_engine_createTemporaryDestination(hClient,
                                                   ismDESTINATION_QUEUE,
                                                   TEMP_QUEUE1,
                                                   NULL,
                                                   NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        pNamedQHandle = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE1);
        TEST_ASSERT_PTR_NOT_NULL(pNamedQHandle);

        rc = ism_engine_createTemporaryDestination(hClient,
                                                   ismDESTINATION_QUEUE,
                                                   TEMP_QUEUE2,
                                                   NULL,
                                                   NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        pNamedQHandle = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE2);
        TEST_ASSERT_PTR_NOT_NULL(pNamedQHandle);

        rc = ism_engine_createTemporaryDestination(hClient,
                                                   ismDESTINATION_QUEUE,
                                                   TEMP_QUEUE3,
                                                   NULL,
                                                   NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        pNamedQHandle = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE3);
        TEST_ASSERT_PTR_NOT_NULL(pNamedQHandle);

        // Destroy two of the queues in one of a different set of orders
        char *deleteQueue[][6] = {{TEMP_QUEUE2, TEMP_QUEUE3},
                                  {TEMP_QUEUE3, TEMP_QUEUE1},
                                  {TEMP_QUEUE1, TEMP_QUEUE2},
                                  {TEMP_QUEUE2, TEMP_QUEUE1},
                                  {TEMP_QUEUE1, TEMP_QUEUE3},
                                  {TEMP_QUEUE3, TEMP_QUEUE2}};

        int deleteOrder = rand()%6;

        for(int innerLoop=0; innerLoop<2; innerLoop++)
        {
            rc = ism_engine_destroyTemporaryDestination(hClient,
                                                        ismDESTINATION_QUEUE,
                                                        deleteQueue[deleteOrder][innerLoop],
                                                        NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            pNamedQHandle = ieqn_getQueueHandle(pThreadData, deleteQueue[deleteOrder][innerLoop]);
            TEST_ASSERT_PTR_NULL(pNamedQHandle);
        }

        // Destroy the client (which should now have no temporary queues)
        rc = test_destroyClientAndSession(hClient, hSession, false);
        TEST_ASSERT_EQUAL(rc, OK);

        // Make sure all the queues have been deleted
        pNamedQHandle = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE1);
        TEST_ASSERT_PTR_NULL(pNamedQHandle);
        pNamedQHandle = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE2);
        TEST_ASSERT_PTR_NULL(pNamedQHandle);
        pNamedQHandle = ieqn_getQueueHandle(pThreadData, TEMP_QUEUE3);
        TEST_ASSERT_PTR_NULL(pNamedQHandle);
    }
}

CU_TestInfo ISM_TemporaryQueues_CUnit_test_capability_TemporaryQueuesBasic[] =
{
    { "TemporaryQueueCreation", test_capability_TemporaryQueueCreation },
    { "TemporaryQueueUsage", test_capability_TemporaryQueueUsage },
    { "TemporaryQueueDeletion", test_capability_TemporaryQueueDeletion },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initTemporaryQueues(void)
{
    return test_engineInit(true, false,
                           true, // Disable Auto creation of named queues
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termTemporaryQueues(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_TemporaryQueues_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("TemporaryQueuesBasic", initTemporaryQueues, termTemporaryQueues, ISM_TemporaryQueues_CUnit_test_capability_TemporaryQueuesBasic),
    CU_SUITE_INFO_NULL
};

int setup_CU_registry(int argc, char **argv, CU_SuiteInfo *allSuites)
{
    CU_SuiteInfo *tempSuites = NULL;

    int retval = 0;

    if (argc > 1)
    {
        int suites = 0;

        for(int i=1; i<argc; i++)
        {
            if (!strcasecmp(argv[i], "FULL"))
            {
                if (i != 1)
                {
                    retval = 97;
                    break;
                }
                // Driven from 'make fulltest' ignore this.
            }
            else if (!strcasecmp(argv[i], "verbose"))
            {
                CU_basic_set_mode(CU_BRM_VERBOSE);
            }
            else if (!strcasecmp(argv[i], "silent"))
            {
                CU_basic_set_mode(CU_BRM_SILENT);
            }
            else
            {
                bool suitefound = false;
                int index = atoi(argv[i]);
                int totalSuites = 0;

                CU_SuiteInfo *curSuite = allSuites;

                while(curSuite->pTests)
                {
                    if (!strcasecmp(curSuite->pName, argv[i]))
                    {
                        suitefound = true;
                        break;
                    }

                    totalSuites++;
                    curSuite++;
                }

                if (!suitefound)
                {
                    if (index > 0 && index <= totalSuites)
                    {
                        curSuite = &allSuites[index-1];
                        suitefound = true;
                    }
                }

                if (suitefound)
                {
                    tempSuites = realloc(tempSuites, sizeof(CU_SuiteInfo) * (suites+2));
                    memcpy(&tempSuites[suites++], curSuite, sizeof(CU_SuiteInfo));
                    memset(&tempSuites[suites], 0, sizeof(CU_SuiteInfo));
                }
                else
                {
                    printf("Invalid test suite '%s' specified, the following are valid:\n\n", argv[i]);

                    index=1;

                    curSuite = allSuites;

                    while(curSuite->pTests)
                    {
                        printf(" %2d : %s\n", index++, curSuite->pName);
                        curSuite++;
                    }

                    printf("\n");

                    retval = 99;
                    break;
                }
            }
        }
    }

    if (retval == 0)
    {
        if (tempSuites)
        {
            CU_register_suites(tempSuites);
        }
        else
        {
            CU_register_suites(allSuites);
        }
    }

    if (tempSuites) free(tempSuites);

    return retval;
}

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_TemporaryQueues_CUnit_allsuites);

    if (retval == 0)
    {
        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();
        printf("Random Seed =     %"PRId64"\n", seedVal);
        printf("\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
               CU_pRunSummary_Final->nTestsRun,
               CU_pRunSummary_Final->nTestsFailed,
               CU_pRunSummary_Final->nAssertsFailed);
        if ((CU_pRunSummary_Final->nTestsFailed > 0) ||
            (CU_pRunSummary_Final->nAssertsFailed > 0))
        {
            retval = 1;
        }
    }

    CU_cleanup_registry();

    test_processTerm(retval == 0);

mod_exit:

    return retval;
}
