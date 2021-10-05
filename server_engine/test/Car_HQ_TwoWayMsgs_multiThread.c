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
 *                                                                  *
 * Module Name: Car_HQ_TwoWayMsgs_multiThread.c                     *
 *                                                                  *
 * Description: QoS1 Messages flowing in both directions. HQ        *
 *             disconnects at some point. 100k Cars, 1Million msgs  *
 *             and 1 HQ.                                            *
 *                                                                  *
 ********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include <ismutil.h>
#include "engine.h"
#include "engineInternal.h"
#include "policyInfo.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"
#include "test_utils_task.h"

#define AJ_MSG "AJ-MSG-0"
#define AJ_CAR "AJ_CAR"
#define AJ_RETCODE_WRONG_MSG 1313

#define NO_THREADS 10                   //Number of Threads to be created.
#define NO_CARS_PER_THREAD 10000        //Number of Cars per Thread.
#define NO_MSGS_PER_CAR 10              //Number of Messages Per Car.
#define CHECK_POINTS 100000             //Check point for messages. if '1', all message will be printed.
#define NO_SECONDS_TO_WAIT 5            //Number of seconds to wait before the HQ re-start.

int32_t testRetcode = OK;
static int countIncomingMessagesToHQ = 0;
static int countIncomingMessagesToCar = 0;

typedef struct tag_msgDetails
{
    int32_t prodNo;
    int32_t msgNo;

} msgDetails_t;

typedef struct tag_asyncDetails
{
    pthread_mutex_t *pMutex;
    ismEngine_ClientStateHandle_t *phClientHQNew;

} asyncDetails_t;

typedef struct tag_carDetails
{
    int32_t carsPerThread;
    int32_t firstCar;

} carDetails_t;
/*
typedef struct tag_carStatus
{
    int32_t lastMsg;

} carStatus_t;
*/
typedef struct tag_msgDeliverDetails
{
    int32_t carsLastMsg[(NO_CARS_PER_THREAD * NO_THREADS)+1];
    ismEngine_SessionHandle_t hSession;

} msgDeliveryDetails_t;

typedef struct durableSubsContext
{
    uint32_t subOptions;
    ismEngine_ConsumerHandle_t *phConsumer;     //Pointer needs to be passed as it has to be used throughout the rest of the program.
    msgDeliveryDetails_t mdd;

} durableSubsContext_t;

void asyncReCreateSessionCB(
        int32_t retcode,
        void* handle,
        void *pContext)
{
    asyncDetails_t **ppasyncDetails = (asyncDetails_t **) pContext;
    asyncDetails_t *pasyncDetails = *ppasyncDetails;

    TEST_ASSERT(retcode == OK, ("asyncReCreateSessionCB received retcode %d\n", retcode));

    pthread_mutex_unlock(pasyncDetails->pMutex);
}

void createClientAsyncCB(
        int32_t retcode,
        void* handle,
        void *pContext)
{
    asyncDetails_t **ppasyncDetails = (asyncDetails_t **)pContext;
    asyncDetails_t *pasyncDetails = *ppasyncDetails;

    TEST_ASSERT(retcode == OK || retcode == ISMRC_ResumedClientState, ("createClientAsyncCB received retcode %d\n", retcode));

    *(pasyncDetails->phClientHQNew) = handle;
    pthread_mutex_unlock(pasyncDetails->pMutex);
}

bool asyncHQ2CarMessageCallback(
        ismEngine_ConsumerHandle_t hConsumer,
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
        void * pConsumerContext)
{
    int32_t rc = OK;
    char *temp_msg = (char *)pAreaData[1];
    bool wantAnotherMessage = true;
    ismEngine_SessionHandle_t *phSession = (ismEngine_SessionHandle_t *) pConsumerContext;
    ismEngine_SessionHandle_t hSession =  *phSession;

/*
    msgDeliveryDetails_t **ppmddCB = (msgDeliveryDetails_t **) pConsumerContext;
    msgDeliveryDetails_t *pmddCB = *ppmddCB;
*/
    /*static carStatus_t carS[(NO_CARS_PER_THREAD * NO_THREADS)+1];*/


    if ((countIncomingMessagesToCar) % CHECK_POINTS == 0)
    {
        printf(
                "\nThe update message we received is %s",
                temp_msg);
    }

    __sync_fetch_and_add(&countIncomingMessagesToCar, 1);

    if (rc == OK)
    {
        rc = ism_engine_confirmMessageDelivery(
            hSession,
            NULL,
            hDelivery,
            ismENGINE_CONFIRM_OPTION_CONSUMED,
            NULL,
            0,
            NULL);
        TEST_ASSERT(rc == OK || rc ==  ISMRC_AsyncCompletion, ("ack returned %d", rc));

        rc = OK; // If it went async, that's not a failure in this test.
/*
        //AJ- Actual TEST: Compare the message with a message we expect and checks if each time the message is received.
        if (temp_msg->msgNo != 0)
        {
            if (pmddCB->carsLastMsg[temp_msg->prodNo] != (temp_msg->msgNo - 1))
            {
                //The message wasn't the message we expected
                printf(
                        "\nThe message no we expected under the car %d was: %d but the message no we received was %d\n",
                        temp_msg->prodNo,
                        pmddCB->carsLastMsg[temp_msg->prodNo],
                        temp_msg->msgNo);
                rc = AJ_RETCODE_WRONG_MSG;
            }
        }
        pmddCB->carsLastMsg[temp_msg->prodNo] = temp_msg->msgNo;
*/
    }

    if (countIncomingMessagesToCar == (NO_CARS_PER_THREAD * NO_THREADS))
    {
        printf(
                "\n\nITS DONE!! and the Last update Message is '%s'.\n\n",
                temp_msg);
        wantAnotherMessage = false;
    }

    if ((rc != OK) && (testRetcode == OK))
    {
        testRetcode = rc;
    }

    ism_engine_releaseMessage(hMessage);
    return wantAnotherMessage;
}

void clientStealCallback(int32_t reason,
                         ismEngine_ClientStateHandle_t hClient,
                         uint32_t options,
                         void *pContext)
{
    msgDeliveryDetails_t **ppMddCB = (msgDeliveryDetails_t **)pContext;

    TEST_ASSERT(reason == ISMRC_ResumedClientState,
                ("\nTest failed reason for stealCB unexpected %d", reason));
    TEST_ASSERT(options == ismENGINE_STEAL_CALLBACK_OPTION_NONE,
                ("\nTest failed options for stealCB unexpected 0x%08x", options));

    int32_t rc = OK;

    rc = ism_engine_destroyClientState(hClient,
            ismENGINE_DESTROY_CLIENT_OPTION_NONE,
            NULL,
            0,
            NULL);
    TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion,
                ("\nTest failed while destroying client in stealCB, RC = %d", rc));

    *ppMddCB = NULL;
}

void test_setClientHandle(int32_t retcode, void *handle, void *pContext)
{
    ismEngine_ClientStateHandle_t *pClientHandle = *(ismEngine_ClientStateHandle_t **)pContext;

    TEST_ASSERT(retcode == OK, ("\nUnexpected retcode in %s (%d)", __func__, retcode));

    *pClientHandle = (ismEngine_ClientStateHandle_t)handle;
}

void *carsThreads(void *arg)
{
    carDetails_t *pcd = (carDetails_t *) arg;

    int i, j;
    int32_t carNo;
    int32_t rc = OK;
    char topic[256];
    char temp_name[256];

    /*multiple producer details*/
    ismEngine_ClientStateHandle_t hClientCar[pcd->carsPerThread];
    ismEngine_SessionHandle_t hSession[pcd->carsPerThread];
    ismEngine_ProducerHandle_t hProducer[pcd->carsPerThread];
    ismEngine_ConsumerHandle_t hCarConsumer[pcd->carsPerThread];
    ismEngine_MessageHandle_t hMessage = NULL;

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaType = ismMESSAGE_AREA_PAYLOAD;

    header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;

    ism_engine_threadInit(0);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

    for (j = 0; j < (pcd->carsPerThread); j++)
    {
        carNo = (j + 1 + pcd->firstCar);
        sprintf(temp_name, "AJ_CAR:%d", carNo);
        sprintf(topic, "DIAGNOSTICS/%d", (j + 1));

        hClientCar[j] = NULL;
        ismEngine_ClientStateHandle_t *pHandle = &hClientCar[j];
        rc = ism_engine_createClientState(
                temp_name,
                testDEFAULT_PROTOCOL_ID,
                ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                NULL,
                NULL,
                NULL,
                &hClientCar[j],
                &pHandle, sizeof(pHandle), test_setClientHandle);

        if (rc != OK)
        {
            TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("\nTest failed, while creating car-client :%d and RC: %d.\n\n", carNo, rc));
        }
    }

    // Wait for all clients to be created
    for(j = 0; j < (pcd->carsPerThread); j++)
    {
        if (hClientCar[j] == NULL)
        {
            j = -1;
            sched_yield();
        }
    }

    uint32_t actionsRemaining = pcd->carsPerThread;
    uint32_t *pActionsRemaining= &actionsRemaining;

    for (j = 0; j < (pcd->carsPerThread); j++)
    {
        carNo = (j + 1 + pcd->firstCar);
        rc = ism_engine_createSession(
                hClientCar[j],
                ismENGINE_CREATE_SESSION_OPTION_NONE,
                &hSession[j],
                NULL,
                0,
                NULL);
        TEST_ASSERT(rc == OK, ("\nTest failed, while creating car-session :%d and RC: %d.\n\n", carNo, rc));

        rc = ism_engine_createProducer(
                hSession[j],
                ismDESTINATION_TOPIC,
                topic,
                &hProducer[j],
                NULL,
                0,
                NULL);
        TEST_ASSERT(rc == OK, ("\nTest failed, while creating car-producer :%d and RC: %d.\n\n", carNo, rc));

        rc = ism_engine_createSubscription(
                    hClientCar[j],
                    "Cars",
                    NULL,
                    ismDESTINATION_TOPIC,
                    "UPDATE/ALL/",
                    &subAttrs,
                    NULL,
                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("\nTest Failed, while Creating Durable Subscription for Car RC: %d!!\n\n",rc));
    }

    // Wait for all subscriptions (cars) to be created
    while(actionsRemaining > 0)
    {
        sched_yield();
    }

    for(j = 0; j < (pcd->carsPerThread); j++)
    {
        carNo = (j + 1 + pcd->firstCar);
        rc = ism_engine_createConsumer(
                    hSession[j],
                    ismDESTINATION_SUBSCRIPTION,
                    "Cars",
                    NULL,
                    NULL, // Owning client same as session client
                    &hSession[j],                            //Context for MsgDeliveryCB
                    sizeof(&hSession[j]),
                    asyncHQ2CarMessageCallback,
                    NULL,
                    test_getDefaultConsumerOptions(subAttrs.subOptions),
                    &hCarConsumer[j],
                    NULL,
                    0,
                    NULL);
        TEST_ASSERT(rc == OK, ("\nTest Failed, while Creating Consumer Car-0%d RC: %d!!\n\n",(j+1), rc));

        rc = ism_engine_startMessageDelivery(
                hSession[j],
                ismENGINE_START_DELIVERY_OPTION_NONE,
                NULL,
                0,
                NULL);
        TEST_ASSERT(rc == OK, ("\nTest Failed, while starting message delivery for HQ RC: %d!!\n\n",rc));
    }

    actionsRemaining = 0;
    for (j = 0; j < pcd->carsPerThread; j++)
    {
        carNo = (j + 1 + pcd->firstCar);
        sprintf(temp_name, "AJ_CAR:%d", carNo);
        for (i = 0; i < NO_MSGS_PER_CAR; i++)
        {
            msgDetails_t msgBuffer;
            size_t areaLength;
            void *areaArray[1];
            msgBuffer.prodNo = carNo;
            msgBuffer.msgNo = (i + 1);
            int ref = sizeof(msgDetails_t);
            areaLength = ref;
            areaArray[0] = (void *) &msgBuffer; //Remember its a hack,
            // u r not suppose to know which pointer in the array is the message, but to find it using iteration by checking the areaType
            rc = ism_engine_createMessage(
                    &header,
                    1,
                    &areaType,
                    &areaLength,
                    (void **) areaArray,
                    &hMessage);
            TEST_ASSERT(rc == OK, ("\nTest failed, while creating Message No: %d for car :%d and RC: %d.\n\n", i + 1, carNo, rc));

            test_incrementActionsRemaining(pActionsRemaining, 1);
            rc = ism_engine_putMessage(hSession[j],
                    hProducer[j],
                    NULL,
                    hMessage,
                    &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
            if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("\nTest failed, while putting Message No: %d for car :%d and RC: %d.\n\n", i + 1, carNo, rc));

            // Only allow 500 async publishes
            if (actionsRemaining > 500)
            {
                // Wait for all messages to be published
                while(actionsRemaining > 0)
                {
                    sched_yield();
                }
            }
        }
    }

    // Wait for all messages to be published
    while(actionsRemaining > 0)
    {
        sched_yield();
    }

    int sleeps = 1;

    while (countIncomingMessagesToCar != (NO_CARS_PER_THREAD * NO_MSGS_PER_CAR * NO_THREADS) && sleeps < 10)
    {
        sleep(1);
        sleeps++;
    }

    for (j = 0; j < pcd->carsPerThread; j++)
    {
        carNo = (j + 1 + pcd->firstCar);
        rc = ism_engine_destroyProducer(hProducer[j], NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("\nTest failed, while destroying car-producer :%d and RC: %d.\n\n", carNo, rc));

        rc = ism_engine_destroySession(hSession[j], NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("\nTest failed, while destroying car-session :%d and RC: %d.\n\n", carNo, rc));

        test_incrementActionsRemaining(pActionsRemaining, 1);
        rc = ism_engine_destroyClientState(
                hClientCar[j],
                ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("\nTest failed, while destroying car-client :%d and RC: %d.\n\n", carNo, rc));
    }

    // Wait for all clients to be destroyed
    while(actionsRemaining > 0)
    {
        sched_yield();
    }

    ism_engine_threadTerm(1);
    return NULL;
}

bool asyncMessageCallback(
        ismEngine_ConsumerHandle_t hConsumer,
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
        void * pConsumerContext)
{
    int32_t rc = OK;
    msgDetails_t *temp_msg = pAreaData[0];

    msgDeliveryDetails_t **ppmddCB = (msgDeliveryDetails_t **) pConsumerContext;
    msgDeliveryDetails_t *pmddCB = *ppmddCB;

    //static carStatus_t carS[(NO_CARS_PER_THREAD * NO_THREADS)+1];

    if ((countIncomingMessagesToHQ) % CHECK_POINTS == 0)
    {
        printf(
                "\nThe Diagnosis message we expected is %d and the message received under the car '%d' is %d",
                (pmddCB->carsLastMsg[temp_msg->prodNo] + 1),
                temp_msg->prodNo,
                temp_msg->msgNo);
    }

    __sync_fetch_and_add(&countIncomingMessagesToHQ, 1);

    if (rc == OK)
    {
        rc = ism_engine_confirmMessageDelivery(
                        pmddCB->hSession,
                        NULL,
                        hDelivery,
                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                        NULL, 0, NULL);
        TEST_ASSERT(rc == OK || rc ==  ISMRC_AsyncCompletion, ("ack returned %d", rc));

        rc = OK; //If the test went async that's not a failure

        //AJ- Actual TEST: Compare the message with a message we expect and checks if each time the message is received.
        if (temp_msg->msgNo != 0)
        {
            if (pmddCB->carsLastMsg[temp_msg->prodNo] != (temp_msg->msgNo - 1))
            {
                //The message wasn't the message we expected
                printf(
                        "\nThe message no we expected under the car %d was: %d but the message no we received was %d\n",
                        temp_msg->prodNo,
                        pmddCB->carsLastMsg[temp_msg->prodNo],
                        temp_msg->msgNo);
                rc = AJ_RETCODE_WRONG_MSG;
            }
        }
        pmddCB->carsLastMsg[temp_msg->prodNo] = temp_msg->msgNo;
    }

    bool wantAnotherMessage = true;

    if (countIncomingMessagesToHQ == (NO_CARS_PER_THREAD * NO_MSGS_PER_CAR * NO_THREADS))
    {
        printf(
                "\n\nITS DONE!! and the Last Message is '%d' under the Car : %d.\n\n",
                temp_msg->msgNo,
                temp_msg->prodNo);
        wantAnotherMessage = false;
    }


    if ((rc != OK) && (testRetcode == OK))
    {
        testRetcode = rc;
    }

    ism_engine_releaseMessage(hMessage);
    return wantAnotherMessage;
}

void durableHQSubsCB(
        ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName,
        const char * pTopicString,
        void * properties,
        size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttrs,
        uint32_t consumerCount,
        void * pContext)
{
    int32_t rc = OK;
    durableSubsContext_t **ppCallbackContext = (durableSubsContext_t **) pContext;
    durableSubsContext_t *pCallbackContext = *ppCallbackContext;

    msgDeliveryDetails_t *pMdd = &pCallbackContext->mdd;

    rc = ism_engine_createConsumer(
            pCallbackContext->mdd.hSession,
            ismDESTINATION_SUBSCRIPTION,
            pSubName,
            NULL,
            NULL, // Owning client same as session client
            &pMdd,
            sizeof(pMdd),
            asyncMessageCallback,
            NULL,
            test_getDefaultConsumerOptions(pCallbackContext->subOptions),
            pCallbackContext->phConsumer,
            NULL,
            0,
            NULL);
    TEST_ASSERT(rc == OK, ("\nTest failed, while creating HQ consumer Async RC: %d.\n\n",rc));
}



int main(int argc, char *argv[])
{
    int trclvl = 3;
    int32_t rc = OK;
    pthread_mutexattr_t mutexattrs;
    pthread_mutex_t mutex;

    asyncDetails_t asyncDetails;
    asyncDetails_t *pAsyncDetails = &asyncDetails;

    durableSubsContext_t durableSubContxt;
    durableSubsContext_t *pDurableSubContxt = &durableSubContxt;
    int z;
    for(z = 0; z <((NO_CARS_PER_THREAD * NO_THREADS)+1);z++)
    {
        durableSubContxt.mdd.carsLastMsg[z] = 0;
    }


    //memset(durableSubContxt.mdd.carsLastMsg, 0, ((NO_CARS_PER_THREAD * NO_THREADS)+1) );

    carDetails_t cd[NO_THREADS];
    pthread_t t_carsThread[NO_THREADS];

    void *retCarsThread[NO_THREADS];

    /*HQ client details*/
    ismEngine_ClientStateHandle_t hClientHQ = NULL;
    ismEngine_SessionHandle_t hSessionHQ = NULL;
    ismEngine_ConsumerHandle_t hConsumer = NULL;
    ismEngine_ClientStateHandle_t hClientHQNew = NULL;
    ismEngine_ProducerHandle_t hProducerHQ =NULL;

    pthread_mutexattr_init(&mutexattrs);
    pthread_mutexattr_settype(&mutexattrs, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&mutex, &mutexattrs);
    asyncDetails.pMutex = &mutex;
    asyncDetails.phClientHQNew = &hClientHQNew;


    printf("\n\nThe test commenced with %d threads running.",NO_THREADS);
    printf("\nTotal No. of Producers: %d\nTotal No. of Messages: %d\n\n",(NO_THREADS * NO_CARS_PER_THREAD), (NO_THREADS * NO_CARS_PER_THREAD * NO_MSGS_PER_CAR));

    rc = test_processInit(trclvl, NULL);
    TEST_ASSERT(rc == OK, ("Test failed, while test_processInit() RC: %d", rc));

    rc = test_engineInit(true, true,
                         ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                         false, /*recovery should complete ok*/
                         ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                         1024);
    TEST_ASSERT(rc == OK, ("Test failed, while test_engineInit() RC: %d", rc));

    // Set default maxMessageCount to 0 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;

    pthread_mutex_lock(&mutex);
    rc = ism_engine_createClientState(
            "HeadQuarters",
            testDEFAULT_PROTOCOL_ID,
            ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
            &hClientHQ,                                            //StealCB context
            clientStealCallback,                                //StealCB function
            NULL,                                               //Security Context
            &hClientHQ,                                            //Returned client handle
            NULL,
            0,
            NULL);
    if(rc == OK)
    {
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("\nTest Failed, while creating Client for HQ RC: %d!!\n\n",rc));
    }

    pthread_mutex_lock(&mutex);
    rc = ism_engine_createSession(
            hClientHQ,
            ismENGINE_CREATE_SESSION_OPTION_NONE,
            &hSessionHQ,
            NULL,
            0,
            NULL);
    if(rc == OK)
    {
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("\nTest Failed, while Creating Session HQ RC: %d!!\n\n",rc));
    }

    pthread_mutex_lock(&mutex);
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

    rc = sync_ism_engine_createSubscription(
            hClientHQ,
            "SubHQ",
            NULL,
            ismDESTINATION_TOPIC,
            "DIAGNOSTICS/#",
            &subAttrs,
            NULL); // Owning client same as requesting client
    TEST_ASSERT(rc == OK, ("\nTest Failed, while Creating Durable Subscription for HQ RC: %d!!\n\n",rc));

    //pthread_mutex_unlock(&mutex);
    //pthread_mutex_lock(&mutex);

    durableSubContxt.mdd.hSession = hSessionHQ;
    msgDeliveryDetails_t *pMdd = &durableSubContxt.mdd;

    rc = ism_engine_createConsumer(
            hSessionHQ,
            ismDESTINATION_SUBSCRIPTION,
            "SubHQ",
            NULL,
            NULL, // Owning client same as session client
            &pMdd,
            sizeof(pMdd),
            asyncMessageCallback,
            NULL,
            test_getDefaultConsumerOptions(subAttrs.subOptions),
            &hConsumer,
            NULL,
            0,
            NULL);
    TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("\nTest Failed, while Creating Consumer HQ RC: %d!!\n\n",rc));

    //pthread_mutex_unlock(&mutex);
    //pthread_mutex_lock(&mutex);

    rc = ism_engine_startMessageDelivery(
            hSessionHQ,
            ismENGINE_START_DELIVERY_OPTION_NONE,
            NULL,
            0,
            NULL);
    if(rc == OK)
    {
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("\nTest Failed, while starting message delivery for HQ RC: %d!!\n\n",rc));
    }

    int i;
    for (i = 0; i < NO_THREADS; i++)
    {
        cd[i].carsPerThread = NO_CARS_PER_THREAD;
        cd[i].firstCar = i * NO_CARS_PER_THREAD;
        rc = test_task_startThread(&(t_carsThread[i]),carsThreads, &cd[i],"carsThreads");
        TEST_ASSERT(rc == 0, ("pthread_create returned %d", rc));
    }

    char msgBufferHQ[256];

    ismEngine_MessageHandle_t hMessageHQ = NULL;

    rc = ism_engine_createProducer(
            hSessionHQ,
            ismDESTINATION_TOPIC,
            "UPDATE/ALL/",
            &hProducerHQ,
            NULL,
            0,
            NULL);
    TEST_ASSERT(rc == OK, ("\nTest failed, while creating an HQ-producer and RC: %d.\n\n", rc));

    sprintf(msgBufferHQ, "New Software V1.0");
    void *msgBufferPtr = msgBufferHQ;
    rc = test_createMessage(sizeof(msgBufferHQ),
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, "UPDATE/ALL/",
                            &hMessageHQ, &msgBufferPtr);
    TEST_ASSERT(rc == OK, ("\nTest failed, while creating Message: %s for HQ and RC: %d.\n\n", msgBufferHQ, rc));

    rc = sync_ism_engine_putMessage(hSessionHQ, // Each message should be put and then released before going to next message
                                    hProducerHQ,
                                    NULL,
                                    hMessageHQ);
    TEST_ASSERT(rc < ISMRC_Error, ("\nTest failed, while putting Message: %s for HQ and RC: %d.\n\n",  msgBufferHQ, rc));

    sleep(NO_SECONDS_TO_WAIT);

    pthread_mutex_lock(&mutex);
    rc = ism_engine_createClientState(
            "HeadQuarters",
            testDEFAULT_PROTOCOL_ID,
            ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
            &pMdd,
            clientStealCallback,
            NULL,
            &hClientHQNew,
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            createClientAsyncCB);
    if(rc == ISMRC_OK)
    {
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("\nTest Failed, while re-creating Client for HQ RC: %d!!\n\n",rc));
    }

    printf("\n\n****HeadQuarters has been Re-Started!!****\n");

    pthread_mutex_lock(&mutex);
    rc = ism_engine_createSession(
            hClientHQNew,
            ismENGINE_CREATE_SESSION_OPTION_NONE,
            &hSessionHQ,
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            asyncReCreateSessionCB);
    if(rc == OK)
    {
        pthread_mutex_unlock(&mutex);
    }
    else
    {
        TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("\nTest Failed, while re-creating Session for HQ RC: %d!!\n\n",rc));
    }

    pthread_mutex_lock(&mutex);

    durableSubContxt.subOptions = subAttrs.subOptions;
    durableSubContxt.phConsumer = &hConsumer;
    durableSubContxt.mdd.hSession = hSessionHQ;

    rc = ism_engine_listSubscriptions(
            hClientHQNew,
            NULL,
            &pDurableSubContxt,
            durableHQSubsCB);
    TEST_ASSERT(rc == OK , ("\nTest Failed, while listing durable subscription for HQ RC: %d!!\n\n",rc));

    rc = ism_engine_startMessageDelivery(
            hSessionHQ,
            ismENGINE_START_DELIVERY_OPTION_NONE,
            NULL,
            0,
            NULL);
    TEST_ASSERT(rc == OK, ("\nTest Failed, while re-starting Message delivery for HQ RC: %d!!\n\n",rc));

    for (i = 0; i < NO_THREADS; i++)
    {
        pthread_join(t_carsThread[i], &retCarsThread[i]);
    }

    int sleeps = 1;

    while (countIncomingMessagesToHQ != (NO_CARS_PER_THREAD * NO_MSGS_PER_CAR * NO_THREADS) && sleeps < 10)
    {
        sleep(1);
        sleeps++;
    }

    rc = sync_ism_engine_destroyConsumer(hConsumer);
    TEST_ASSERT(rc == OK, ("\nTest Failed, while re-destroying Consumer for HQ RC: %d!!\n\n",rc));

    rc = sync_ism_engine_destroySession(hSessionHQ);
    TEST_ASSERT(rc == OK, ("\nTest Failed, while re-destroying Session for HQ RC: %d!!\n\n",rc));

    //pthread_mutex_unlock(&mutex);
    //pthread_mutex_lock(&mutex);

    rc = sync_ism_engine_destroyClientState(
            hClientHQNew,
            ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT(rc == OK, ("\nTest Failed, while re-destroying Client for HQ RC: %d!!\n\n",rc));

    pthread_mutex_unlock(&mutex);

    rc = test_engineTerm(true);
    TEST_ASSERT(rc == OK, ("\nTest Failed, while engineTerm() RC: %d!!\n\n",rc));

    rc = test_processTerm(testRetcode == OK);
    TEST_ASSERT(rc == OK, ("\nTest Failed, while processTerm() RC: %d!!\n\n",rc));

    if (testRetcode == OK)
    {
        if(countIncomingMessagesToHQ == (NO_CARS_PER_THREAD * NO_MSGS_PER_CAR * NO_THREADS))
        {
            printf("Test passed.\n");
        }
        else
        {
            printf("Test failed, not all the messages were transfered.\n");
        }
    }
    else
    {
        printf("Test failed : %d.\n", rc);
    }

    return (int) testRetcode;
}

