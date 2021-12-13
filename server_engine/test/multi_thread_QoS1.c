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

/*
 *                                                                      *
 * Module Name: multi_thread_QoS1.c                                     *
 *                                                                      *
 * Description: THIS IS AN UPDATED VERSION OF simple_thread_test.c      *
 *                 this can have multiple producers with multi msgs and *
 *                 multiple threads all messages under QoS1.            *
 *                 Could do different types of tests here, but only with*
 *                single     session.                                   *
 *                                                                      *
 ************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include <ismutil.h>
#include "engine.h"
#include "engineInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_task.h"

#define AJ_MSG "AJ:MSG"
#define AJ_RETCODE_WRONG_MSG 1313
#define AJ_RETCODE_WRONG_CONSUMER 1314

#define NOMESSAGES 100                            //Number of Messages to be passed.
#define POINTOFCHECKS 10                        //At every POINTOFCHECKS the message will be verified.
#define NOPUBSUB 10                                //Number of producers and consumers.
#define TOPICNAMELENGTH 256                        //Topic Name Length.
#define MESSAGEBUFFERLENGTH 512                    //Message Buffer Length.

typedef struct tag_testEngineProducerDetails_t                //Producer Details
{
    ismEngine_ProducerHandle_t hProducer;
    char                       topic[256];
} testEngineProducerDetails_t;

typedef struct tag_testEngineConsumerDetails_t                //Consumer Details
{
    ismEngine_SessionHandle_t  hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    char                       topic[256];
    int32_t                    expectedMessageNo1;
    int32_t                    expectedMessageNo2;
} testEngineConsumerDetails_t;

typedef struct tag_testEngineDetails_t
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    testEngineProducerDetails_t  *hProducer;
    testEngineConsumerDetails_t  *hConsumer;
    int32_t threadIdentifier;                         //To identify which Thread is using the function
} testEngineDetails_t;

typedef struct tag_inputs_t
{
    int32_t noMessages;
    int32_t pointOfChecks;
    int32_t noProdCons;
} inputs_t;

pthread_spinlock_t spin;
int checkLastMsg = 0;
int rc = OK;
int32_t testRetcode = OK;
inputs_t ips;


bool asyncMessageCallback(
        ismEngine_ConsumerHandle_t hConsumer,
        ismEngine_DeliveryHandle_t hDelivery,
        ismEngine_MessageHandle_t hMessage, uint32_t deliveryId,
        ismMessageState_t state,
        uint32_t destinationOptions,
        ismMessageHeader_t * pMsgDetails, uint8_t areaCount,
        ismMessageAreaType_t areaTypes[areaCount],
        size_t areaLengths[areaCount],
        void * pAreaData[areaCount],
        void * pConsumerContext,
        ismEngine_DelivererContext_t * _delivererContext);

void *create_put_messages(void *arg)
{
    char msgBuffer[MESSAGEBUFFERLENGTH];
    ismMessageAreaType_t areaType[2];
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    size_t areaLength[2];                 //+1 is for the null terminator character
    char *areaArray[2];
    int32_t i, j = 0, k;                 //Counters for creating messages and assigning them to producers and consumers.
    testEngineDetails_t *pEds = (testEngineDetails_t *) arg;
    ismEngine_MessageHandle_t hMessage;
    header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

    k = ips.noMessages / ips.noProdCons;

    ism_engine_threadInit(0);

    for (i = 0; i < ips.noMessages; i++)
    {
        int ref = sprintf(msgBuffer, "%s_%d_%d", AJ_MSG, pEds->threadIdentifier, (i+1));
        areaLength[0] = ref + 1;
        areaType[0] = ismMESSAGE_AREA_PAYLOAD;
        areaArray[0] = msgBuffer; //Remember its a hack, u r not suppose to know which pointer in the areaArray is the message, but to find it using iteration by checking the areaType

        areaLength[1] = strlen(pEds->hProducer[j].topic)+1;
        areaType[1] = ismMESSAGE_AREA_PROPERTIES;
        areaArray[1] = pEds->hProducer[j].topic;


        rc =  ism_engine_createMessage(&header,
                                        2,
                                        areaType,
                                        areaLength,
                                        (void **) areaArray,
                                        &hMessage);
        TEST_ASSERT(rc == OK, ("create msg rc %d", rc));

        rc =  ism_engine_putMessage(pEds->hSession, // Each message should be put and then released before going to next message
                                     pEds->hProducer[j].hProducer,
                                     NULL,
                                     hMessage,
                                     NULL, 0, NULL);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("put msg rc %d", rc));

        if ((i+1) % k == 0)
        {
            j++;
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    testEngineDetails_t eds1;
    testEngineDetails_t eds2;
    pthread_t t1PutMessages;
    void *ret1PutMessages = NULL;

    pthread_t t2PutMessages;
    void *ret2PutMessages = NULL;
    int trclvl = 0;

    rc = test_processInit(trclvl, NULL);
    if (rc != OK) return rc;

    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);


    ips.noMessages = NOMESSAGES;
    ips.pointOfChecks = POINTOFCHECKS;
    ips.noProdCons = NOPUBSUB;

    if (argc > 1) {
        ips.noMessages = atoi(argv[1]);
        ips.pointOfChecks = atoi(argv[2]);
        ips.noProdCons = atoi(argv[3]);
    }

    eds1.hProducer = malloc(ips.noProdCons * sizeof(testEngineProducerDetails_t));
    eds1.hConsumer = malloc(ips.noProdCons * sizeof(testEngineConsumerDetails_t));

    printf("\nThe test commenced for passing %d messages with checkpoint at every %d message.",
            ips.noMessages, ips.pointOfChecks);
    printf("\nWill be having %d producers with a topic each.\n\n", ips.noProdCons);

    rc = test_engineInit_DEFAULT;
    if (rc != OK){
        free(eds1.hConsumer);
        free(eds1.hProducer);
    	    return rc;
    }

    rc = ism_engine_createClientState("Ashlin-Client",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &eds1.hClient,
                                      NULL, 0, NULL);

    rc = ism_engine_createSession(eds1.hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &eds1.hSession,
                                  NULL, 0, NULL);

    rc = ism_engine_startMessageDelivery(eds1.hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);

    int32_t x, y =1; //For loop count for creating producers and consumers

    for (x = 0; x < ips.noProdCons; x++)
    {
        sprintf(eds1.hProducer[x].topic, "Topic/%d", x+1);

        eds1.hConsumer[x].hSession = eds1.hSession;
        strcpy(eds1.hConsumer[x].topic, eds1.hProducer[x].topic);
        eds1.hConsumer[x].expectedMessageNo1 = x*y + 1;
        eds1.hConsumer[x].expectedMessageNo2 = x*y + 1;

        rc =  ism_engine_createProducer(eds1.hSession,
                                         ismDESTINATION_TOPIC,
                                         eds1.hProducer[x].topic,
                                         &(eds1.hProducer[x].hProducer),
                                         NULL, 0, NULL);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("create prod rc %d", rc));

        testEngineConsumerDetails_t *pConsumerDetails = &(eds1.hConsumer[x]);
        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

        rc = ism_engine_createConsumer(eds1.hSession,
                                         ismDESTINATION_TOPIC,
                                         eds1.hConsumer[x].topic,
                                         &subAttrs,
                                         NULL, // Unused for TOPIC
                                         &pConsumerDetails,
                                         sizeof(pConsumerDetails),
                                         asyncMessageCallback,
                                         NULL,
                                         ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                         &(eds1.hConsumer[x].hConsumer),
                                         NULL, 0, NULL);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("create cons rc %d", rc));

        y = ips.noMessages/ips.noProdCons;

    }

    eds2 = eds1;
    eds1.threadIdentifier = 1;
    eds2.threadIdentifier = 2;
    rc = test_task_startThread(&t1PutMessages,create_put_messages, &eds1,"create_put_messages");
    TEST_ASSERT(rc == 0, ("pthread_create returned %d", rc));
    rc = test_task_startThread(&t2PutMessages,create_put_messages, &eds2,"create_put_messages");
    TEST_ASSERT(rc == 0, ("pthread_create returned %d", rc));

    pthread_join(t1PutMessages, &ret1PutMessages);
    pthread_join(t2PutMessages, &ret2PutMessages);

    for (x = 0; x < ips.noProdCons; x++)
    {

        rc = ism_engine_destroyProducer(eds1.hProducer[x].hProducer, NULL, 0, NULL);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("destroy prod rc %d", rc));

        rc = ism_engine_destroyConsumer(eds1.hConsumer[x].hConsumer, NULL, 0, NULL);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("destroy cons rc %d", rc));
    }

    rc = ism_engine_destroySession(eds1.hSession, NULL, 0, NULL);
    TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("destroy session rc %d", rc));

    rc = ism_engine_destroyClientState(eds1.hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("destroy client rc %d", rc));

    rc = test_engineTerm(true);
    if (rc != OK) return rc;

    rc = test_processTerm(testRetcode == OK);
    if (rc != OK) return rc;

    if (testRetcode == OK) {
        printf("Test passed.\n");
    } else {
        printf("Test failed.\n");
    }

    free(eds1.hConsumer);
    free(eds1.hProducer);

    return (int) testRetcode;

}

bool asyncMessageCallback(ismEngine_ConsumerHandle_t hConsumer,
                          ismEngine_DeliveryHandle_t hDelivery,
                          ismEngine_MessageHandle_t hMessage,
                          uint32_t deliveryId,
                          ismMessageState_t state,
                          uint32_t destinationOptions,
                          ismMessageHeader_t * pMsgDetails,
                          uint8_t areaCount,
                          ismMessageAreaType_t areaTypes[areaCount],
                          size_t areaLengths[areaCount],
                          void * pAreaData[areaCount],
                          void * pConsumerContext,
                          ismEngine_DelivererContext_t * _delivererContext)
{
    static int countIncomingMessage = 1;
    int32_t threadNo, msgNo;

    char *msgT1, *msgT2;
    char *reportMsg = NULL;

    testEngineConsumerDetails_t *pConsumerDetails = *(testEngineConsumerDetails_t **)pConsumerContext;

    if (countIncomingMessage % ips.pointOfChecks == 0)
    {
        printf("The message received under the %s was: %.*s\n", pConsumerDetails->topic,
                (int) areaLengths[0], (char *) pAreaData[0]);

    }

    if (rc == OK)
    {
        msgT1 = strchr(pAreaData[0], '_');
        threadNo = atoi((msgT1+1));

        msgT2 = strrchr(pAreaData[0], '_');
        msgNo = atoi((msgT2+1));

        if (threadNo == 1)
        {
            if (msgNo != pConsumerDetails->expectedMessageNo1)
            {
                if (testRetcode == OK) testRetcode = AJ_RETCODE_WRONG_MSG;
                printf("**Here it went wrong in T%d: Exp = %d and MsgNo = %d**\n", threadNo, pConsumerDetails->expectedMessageNo1, msgNo);
            }
            else
            {
                pConsumerDetails->expectedMessageNo1 = msgNo + 1;
            }
        }
        else
        {
            if (msgNo != pConsumerDetails->expectedMessageNo2)
            {
                if (testRetcode == OK) testRetcode = AJ_RETCODE_WRONG_MSG;
                printf("**Here it went wrong in T%d: Exp = %d and MsgNo = %d**\n", threadNo, pConsumerDetails->expectedMessageNo2, msgNo);
            }
            else
            {
                pConsumerDetails->expectedMessageNo2 = msgNo + 1;
            }
        }

        /* Check that the consumer we were called with was the one we expected */
        if (pConsumerDetails->hConsumer != hConsumer)
        {
            if (testRetcode == OK) testRetcode = AJ_RETCODE_WRONG_CONSUMER;
            printf("***Wrong consumer in T%d: Exp = %p and hConsumer = %p**\n", threadNo, pConsumerDetails->hConsumer, hConsumer);
        }

        reportMsg = alloca(areaLengths[0]+1);
        if (reportMsg != NULL)
        {
            memcpy(reportMsg, pAreaData[0], areaLengths[0]);
            reportMsg[areaLengths[0]] = '\0';
        }

        ism_engine_releaseMessage(hMessage);

        rc = ism_engine_confirmMessageDelivery(pConsumerDetails->hSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               NULL,
                                               0,
                                               NULL);
    }

    bool wantAnotherMessage = true;

    pthread_spin_lock(&spin);

    if (countIncomingMessage < (ips.noMessages * 2))
    {
        countIncomingMessage++;
    }
    else
    {
        printf("The Last Message is %s.\n", reportMsg ? reportMsg : "<NULLMSG>");

        //stops message delivery callback will unlock the mutex
        wantAnotherMessage = false;
    }

    pthread_spin_unlock(&spin);

    if ((rc != OK) && (testRetcode == OK)) {
        testRetcode = rc;
    }

    return wantAnotherMessage;
}
