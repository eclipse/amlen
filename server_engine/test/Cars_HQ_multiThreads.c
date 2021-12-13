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
 * Module Name: Cars_HQ_multiThreads.c                              *
 *                                                                  *
 * Description: Test to create multiple threads with multiple       *
 *                 producers sending multiple msgs to one consumer. *
 *                 Trying to mock the Automotive TT use-case, where *
 *                 cars are communicating with the factory.         *
 *                                                                  *
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
static int countIncomingMessages = 0;

typedef struct tag_msgDetails
{
    int32_t prodNo;
    int32_t msgNo;

} msgDetails_t;

typedef struct tag_asyncDetails
{

    pthread_mutex_t *pMutex;
    ismEngine_ClientStateHandle_t *phClient;
    ismEngine_SessionHandle_t *phSession;
} asyncDetails_t;

typedef struct tag_carStatus
{
    int32_t lastMsg;

} carStatus_t;

typedef struct tag_carDetails
{
    int32_t carsPerThread;
    int32_t firstCar;
} carDetails_t;

typedef struct durableSubsContext
{
    ismEngine_ConsumerHandle_t *phConsumer; //Pointer needs to be passed as it has to be used throughout the rest of the program.
    ismEngine_SessionHandle_t hSession;

} durableSubsContext_t;

void createClientAsyncCB(
        int32_t retcode,
        void* handle,
        void *pContext)
{
    asyncDetails_t **ppasyncDetails = (asyncDetails_t **)pContext;
    asyncDetails_t *pasyncDetails = *ppasyncDetails;

    if(retcode != OK && retcode != ISMRC_ResumedClientState)
    {
        printf("\nTest Failed, completion of creating a Client RC: %d!!\n\n",retcode);
        exit(retcode);
    }
    else
    {
        *(pasyncDetails->phClient) = handle;
        pthread_mutex_unlock(pasyncDetails->pMutex);
    }
}

void createSessionAsyncCB(
        int32_t retcode,
        void* handle,
        void *pContext)
{
    asyncDetails_t **ppasyncDetails = (asyncDetails_t **) pContext;
    asyncDetails_t *pasyncDetails = *ppasyncDetails;
    if(retcode != OK)
    {
        printf("\nTest Failed, completion of creating a Session RC: %d!!\n\n",retcode);
        exit(retcode);
    }
    else
    {
        *(pasyncDetails->phSession) = handle;
        pthread_mutex_unlock(pasyncDetails->pMutex);
    }

}

void createSubscriptionAsyncCB(
        int32_t retcode,
        void* handle,
        void *pContext)
{
    asyncDetails_t **ppasyncDetails = (asyncDetails_t **)pContext;
    asyncDetails_t *pasyncDetails = *ppasyncDetails;

    if(retcode != OK)
    {
        printf("\nTest Failed, completion of creating a Subscription RC: %d!!\n\n",retcode);
        exit(retcode);
    }
    else
    {
        pthread_mutex_unlock(pasyncDetails->pMutex);
    }
}

void unlockAsyncMutexCB(
        int32_t retcode,
        void* handle,
        void *pContext)
{
    asyncDetails_t **ppasyncDetails = (asyncDetails_t **)pContext;
    asyncDetails_t *pasyncDetails = *ppasyncDetails;

    pthread_mutex_unlock(pasyncDetails->pMutex);
}


void clientStealCallback(int32_t reason,
                         ismEngine_ClientStateHandle_t hClient,
                         uint32_t options,
                         void *pContext)
{
    ismEngine_ClientStateHandle_t *pHandle = (ismEngine_ClientStateHandle_t *)pContext;
    int32_t rc = OK;

    if (reason != ISMRC_ResumedClientState)
    {
        printf("\nTest failed unexpected reason (%d) in stealCB", reason);
        exit(98);
    }

    if (options != ismENGINE_STEAL_CALLBACK_OPTION_NONE)
    {
        printf("\nTest failed unexpected options (0x%08x) in stealCB", options);
        exit(99);
    }

    rc = ism_engine_destroyClientState(hClient,
            ismENGINE_DESTROY_CLIENT_OPTION_NONE,
            NULL,
            0,
            NULL);
    if((rc != OK) && (rc != ISMRC_AsyncCompletion))
    {
        printf("\nTest failed while destroying client in stealCB, RC = %d", rc);
        exit(rc);

    }
    *pHandle = NULL;
}

void createClientStateAsync(int32_t retcode,
                            void *handle,
                            void *pContext)
{
    ismEngine_ClientStateHandle_t *phClient = *(ismEngine_ClientStateHandle_t **)pContext;
    if (retcode != OK) exit(retcode);
    *phClient = handle;
}

void *carsThreads(void *arg)
{
    carDetails_t *pcd = (carDetails_t *) arg;
    int32_t rc = OK;
    //multiple producer details
    ismEngine_ClientStateHandle_t hClientCar[pcd->carsPerThread];
    ismEngine_SessionHandle_t hSession[pcd->carsPerThread];
    ismEngine_ProducerHandle_t hProducer[pcd->carsPerThread];
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaType = ismMESSAGE_AREA_PAYLOAD;
    ismEngine_MessageHandle_t hMessage = NULL;
    header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;

    int i, j;
    int32_t carNo;
    char topic[256];
    char temp_name[256];
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    ism_engine_threadInit(0);

    // Make sure the array is initialized before we rely on it
    memset(hClientCar, 0, sizeof(hClientCar));

    for (j = 0; j < pcd->carsPerThread; j++)
    {

        carNo = (j + 1 + pcd->firstCar);
        sprintf(temp_name, "AJ_CAR:%d", carNo);
        sprintf(topic, "DIAGNOSTICS/%d", (j + 1));
        ismEngine_ClientStateHandle_t *phClientCar = &hClientCar[j];
        rc = ism_engine_createClientState(
                temp_name,
                testDEFAULT_PROTOCOL_ID,
                ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                NULL, NULL, NULL,
                &hClientCar[j],
                &phClientCar,
                sizeof(phClientCar),
                createClientStateAsync);
        if(rc != OK && rc != ISMRC_ResumedClientState && rc != ISMRC_AsyncCompletion)
        {
            printf("\nTest failed, while creating car-client :%d and RC: %d.\n\n", carNo, rc);
            exit(rc);
        }
    }

    memset(hSession, 0, sizeof(hSession));
    do
    {
        for(j = 0; j < pcd->carsPerThread; j++)
        {
            if (hClientCar[j] == NULL)
            {
                sched_yield();
                break;
            }
            else if (hSession[j] == NULL)
            {
                carNo = (j + 1 + pcd->firstCar);
                rc = ism_engine_createSession(
                        hClientCar[j],
                        ismENGINE_CREATE_SESSION_OPTION_NONE,
                        &hSession[j],
                        NULL,
                        0,
                        NULL);
                if(rc != OK)
                {
                    printf("\nTest failed, while creating car-session :%d and RC: %d.\n\n", carNo, rc);
                    exit(rc);
                }
                rc = ism_engine_createProducer(
                        hSession[j],
                        ismDESTINATION_TOPIC,
                        topic,
                        &hProducer[j],
                        NULL,
                        0,
                        NULL);
                if(rc != OK)
                {
                    printf("\nTest failed, while creating car-producer :%d and RC: %d.\n\n", carNo, rc);
                    exit(rc);
                }

                sprintf(temp_name, "AJ_CAR:%d", carNo);
                test_incrementActionsRemaining(pActionsRemaining, NO_MSGS_PER_CAR);
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
                    if(rc != OK)
                    {
                        printf("\nTest failed, while creating Message No: %d for car :%d and RC: %d.\n\n", i + 1, carNo, rc);
                        exit(rc);
                    }

                    rc = ism_engine_putMessage(hSession[j], // Each message should be put and then released before going to next message
                            hProducer[j],
                            NULL,
                            hMessage,
                            &pActionsRemaining,
                            sizeof(pActionsRemaining),
                            test_decrementActionsRemaining);
                    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
                    else if(rc != ISMRC_AsyncCompletion)
                    {
                        printf("\nTest failed, while putting Message No: %d for car :%d and RC: %d.\n\n", i + 1, carNo, rc);
                        exit(rc);
                    }
                }

                // Periodically wait for the remaining work to complete
                if (actionsRemaining > 1000) test_waitForRemainingActions(pActionsRemaining);
            }
        }
    }
    while(j < pcd->carsPerThread);

    // test_waitForRemainingActions(pActionsRemaining);

    test_incrementActionsRemaining(pActionsRemaining, pcd->carsPerThread*3);
    for (j = 0; j < pcd->carsPerThread; j++)
    {
        carNo = (j + 1 + pcd->firstCar);
        rc = ism_engine_destroyProducer(hProducer[j], &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else if (rc != ISMRC_AsyncCompletion)
        {
            printf("\nTest failed, while destroying car-producer :%d and RC: %d.\n\n", carNo, rc);
            exit(rc);
        }

        rc = ism_engine_destroySession(hSession[j], &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else if (rc != ISMRC_AsyncCompletion)
        {
            printf("\nTest failed, while destroying car-session :%d and RC: %d.\n\n", carNo, rc);
            exit(rc);
        }

        rc = ism_engine_destroyClientState(
                hClientCar[j],
                ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
        if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
        else if (rc != ISMRC_AsyncCompletion)
        {
            printf("\nTest failed, while destroying car-client :%d and RC: %d.\n\n", carNo, rc);
            exit(rc);
        }
    }
    test_waitForRemainingActions(pActionsRemaining);

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
        void * pConsumerContext,
        ismEngine_DelivererContext_t * _delivererContext);

void durableHQSubsCB(
        ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName,
        const char * pTopicString,
        void * properties,
        size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes,
        uint32_t consumerCount,
        void * pContext)
{
    int32_t rc = OK;
    durableSubsContext_t *pCallbackContext = (durableSubsContext_t *) pContext;

    rc = ism_engine_createConsumer(
            pCallbackContext->hSession,
            ismDESTINATION_SUBSCRIPTION,
            pSubName,
            NULL,
            NULL, // Owning client same as session client
            &(pCallbackContext->hSession),
            sizeof(&(pCallbackContext->hSession)),
            asyncMessageCallback,
            NULL,
            test_getDefaultConsumerOptions(pSubAttributes->subOptions),
            pCallbackContext->phConsumer,
            NULL,
            0,
            NULL);
    if(rc!=OK)
    {
        printf("\nTest failed, while creating HQ consumer Async RC: %d.\n\n",rc);
        exit(rc);
    }

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
    carDetails_t cd[NO_THREADS];
    pthread_t t_carsThread[NO_THREADS];

    void *retCarsThread[NO_THREADS];

    //HQ client details
    ismEngine_ClientStateHandle_t hClientHQ = NULL;
    ismEngine_SessionHandle_t hSessionHQ = NULL;
    ismEngine_ConsumerHandle_t hConsumer = NULL;
    ismEngine_ClientStateHandle_t hClientHQNew = NULL;

    pthread_mutexattr_init(&mutexattrs);
    pthread_mutexattr_settype(&mutexattrs, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&mutex, &mutexattrs);
    asyncDetails.pMutex = &mutex;


    printf("\n\nThe test commenced with %d threads running.",NO_THREADS);
    printf("\nTotal No. of Producers: %d\nTotal No. of Messages: %d\n\n",(NO_THREADS * NO_CARS_PER_THREAD), (NO_THREADS * NO_CARS_PER_THREAD * NO_MSGS_PER_CAR));

    rc = test_processInit(trclvl, NULL);
    if (rc != OK)
    {
        printf("Test failed, while processInit() RC: %d", rc);
        exit(rc);
    }

    rc = test_engineInit(true, true,
                         ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                         false, /*recovery should complete ok*/
                         ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                         512);
    if (rc != OK)
    {
        printf("Test failed, while engineInit() RC: %d", rc);
        exit(rc);
    }

    // Set default maxMessageCount to 0 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;

    pthread_mutex_lock(&mutex);
    asyncDetails.phClient = &hClientHQ;
    rc = ism_engine_createClientState(
            "HeadQuarters",
            testDEFAULT_PROTOCOL_ID,
            ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
            &hClientHQ,                                            //StealCB context
            clientStealCallback,                                //StealCB function
            NULL,                                               //Security Context
            &hClientHQ,                                            //Returned client handle
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            createClientAsyncCB);
    if(rc == OK || rc == ISMRC_ResumedClientState)
    {
        createClientAsyncCB(rc, hClientHQ, &pAsyncDetails);
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
        printf("\nTest Failed, while creating Client for HQ RC: %d!!\n\n",rc);
        exit(rc);
    }

    pthread_mutex_lock(&mutex);
    asyncDetails.phSession = &hSessionHQ;
    rc = ism_engine_createSession(
            hClientHQ,
            ismENGINE_CREATE_SESSION_OPTION_NONE,
            &hSessionHQ,
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            createSessionAsyncCB);
    if(rc == OK)
    {
        createSessionAsyncCB(rc, hSessionHQ, &pAsyncDetails);
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
            printf("\nTest Failed, while Creating Session HQ RC: %d!!\n\n",rc);
            exit(rc);
    }

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

    pthread_mutex_lock(&mutex);
    rc = ism_engine_createSubscription(
            hClientHQ,
            "SubHQ",
            NULL,
            ismDESTINATION_TOPIC,
            "DIAGNOSTICS/#",
            &subAttrs,
            NULL, // Owning client same as requesting client
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            createSubscriptionAsyncCB);
    if(rc == OK)
    {
        createSubscriptionAsyncCB(rc, NULL, &pAsyncDetails);
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
            printf("\nTest Failed, while Creating Durable Subscription for HQ RC: %d!!\n\n",rc);
            exit(rc);
    }

    pthread_mutex_lock(&mutex);
    rc = ism_engine_createConsumer(
            hSessionHQ,
            ismDESTINATION_SUBSCRIPTION,
            "SubHQ",
            NULL,
            NULL, // Owning client same as session client
            &hSessionHQ,
            sizeof(&(hSessionHQ)),
            asyncMessageCallback,
            NULL,
            test_getDefaultConsumerOptions(subAttrs.subOptions),
            &hConsumer,
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            unlockAsyncMutexCB);
    if(rc == OK)
    {
        unlockAsyncMutexCB(rc, NULL, &pAsyncDetails);
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
        printf("\nTest Failed, while Creating Consumer HQ RC: %d!!\n\n",rc);
        exit(rc);
    }

    pthread_mutex_lock(&mutex);
    rc = ism_engine_startMessageDelivery(
            hSessionHQ,
            ismENGINE_START_DELIVERY_OPTION_NONE,
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            unlockAsyncMutexCB);
    if(rc == OK)
    {
        unlockAsyncMutexCB(rc, NULL, &pAsyncDetails);
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
            printf("\nTest Failed, while starting message delivery for HQ RC: %d!!\n\n",rc);
            exit(rc);
    }

    int i;
    for (i = 0; i < NO_THREADS; i++)
    {
        cd[i].carsPerThread = NO_CARS_PER_THREAD;
        cd[i].firstCar = i * NO_CARS_PER_THREAD;
        rc = test_task_startThread(&(t_carsThread[i]),carsThreads, &cd[i],"carsThreads");
        TEST_ASSERT(rc == 0, ("pthread_create returned %d", rc));
    }

    sleep(NO_SECONDS_TO_WAIT);

    pthread_mutex_lock(&mutex);
    asyncDetails.phClient = &hClientHQNew;
    rc = ism_engine_createClientState(
            "HeadQuarters",
            testDEFAULT_PROTOCOL_ID,
            ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
            &hClientHQ,
            clientStealCallback,
            NULL,
            &hClientHQNew,
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            createClientAsyncCB);
    if(rc == OK || rc == ISMRC_ResumedClientState)
    {
        createClientAsyncCB(rc, hClientHQNew, &pAsyncDetails);
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
        printf("\nTest Failed, while re-creating Client for HQ RC: %d!!\n\n",rc);
        exit(rc);
    }

    printf("\n****HeadQuarters is DOWN!!****\n\n");

    pthread_mutex_lock(&mutex);
    asyncDetails.phSession = &hSessionHQ;
    rc = ism_engine_createSession(
            hClientHQNew,
            ismENGINE_CREATE_SESSION_OPTION_NONE,
            &hSessionHQ,
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            createSessionAsyncCB);
    if(rc == OK)
    {
        createSessionAsyncCB(rc, hSessionHQ, &pAsyncDetails);
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
        printf("\nTest Failed, while re-creating Session for HQ RC: %d!!\n\n",rc);
        exit(rc);
    }

    durableSubContxt.hSession = hSessionHQ;
    durableSubContxt.phConsumer = &hConsumer;

    pthread_mutex_lock(&mutex);
    rc = ism_engine_listSubscriptions(
            hClientHQNew,
            NULL,
            &durableSubContxt,
            durableHQSubsCB);
    if (rc != OK)
    {
        printf("\nTest Failed, while listing durable subscription for HQ RC: %d!!\n\n",rc);
        exit(rc);
    }
    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&mutex);
    rc = ism_engine_startMessageDelivery(
            hSessionHQ,
            ismENGINE_START_DELIVERY_OPTION_NONE,
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            unlockAsyncMutexCB);
    if (rc == OK)
    {
        unlockAsyncMutexCB(rc, NULL, &pAsyncDetails);
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
        printf("\nTest Failed, while re-starting Message delivery for HQ RC: %d!!\n\n",rc);
        exit(rc);
    }

    for (i = 0; i < NO_THREADS; i++)
    {
        pthread_join(t_carsThread[i], &retCarsThread[i]);
    }

    int sleeps = 1;

    while (countIncomingMessages != (NO_CARS_PER_THREAD * NO_MSGS_PER_CAR * NO_THREADS) && sleeps < 10)
    {
        sleep(1);
        sleeps++;
    }

    pthread_mutex_lock(&mutex);
    rc = ism_engine_destroyConsumer(hConsumer,
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            unlockAsyncMutexCB);
    if(rc == OK)
    {
        unlockAsyncMutexCB(rc, NULL, &pAsyncDetails);
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
        printf("\nTest Failed, while re-destroying Consumer for HQ RC: %d!!\n\n",rc);
        exit(rc);
    }

    pthread_mutex_lock(&mutex);
    rc = ism_engine_destroySession(hSessionHQ,
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            unlockAsyncMutexCB);
    if(rc == OK)
    {
        unlockAsyncMutexCB(rc, NULL, &pAsyncDetails);
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
        printf("\nTest Failed, while re-destroying Session for HQ RC: %d!!\n\n",rc);
        exit(rc);
    }

    pthread_mutex_lock(&mutex);
    rc = ism_engine_destroyClientState(
            hClientHQNew,
            ismENGINE_DESTROY_CLIENT_OPTION_NONE,
            &pAsyncDetails,
            sizeof(pAsyncDetails),
            unlockAsyncMutexCB);
    if(rc == OK)
    {
        unlockAsyncMutexCB(rc, NULL, &pAsyncDetails);
    }
    else if (rc != ISMRC_AsyncCompletion)
    {
        printf("\nTest Failed, while re-destroying Client for HQ RC: %d!!\n\n",rc);
        exit(rc);
    }

    pthread_mutex_lock(&mutex);

    rc = test_engineTerm(true);
    if (rc != OK)
    {
        printf("\nTest Failed, while engineTerm() RC: %d!!\n\n",rc);
        exit(rc);
    }

    rc = test_processTerm(testRetcode == OK);
    if (rc != OK)
    {
        printf("\nTest Failed, while processTerm() RC: %d!!\n\n",rc);
        exit(rc);
    }

    if (testRetcode == OK)
    {
        if(countIncomingMessages == (NO_CARS_PER_THREAD * NO_MSGS_PER_CAR * NO_THREADS))
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
        void * pConsumerContext,
        ismEngine_DelivererContext_t * _delivererContext)
{
    int32_t rc = OK;
    msgDetails_t *temp_msg = pAreaData[0];
    ismEngine_SessionHandle_t *hSession = (ismEngine_SessionHandle_t *) pConsumerContext;

    static carStatus_t carS[(NO_CARS_PER_THREAD * NO_THREADS)+1];

    if ((countIncomingMessages) % CHECK_POINTS == 0)
    {
        printf(
                "The message we expected is %d and the message received under the car '%d' is %d\n",
                (carS[temp_msg->prodNo].lastMsg + 1),
                temp_msg->prodNo,
                temp_msg->msgNo);
    }

    __sync_fetch_and_add(&countIncomingMessages, 1);

    if (rc == OK)
    {
        rc = ism_engine_confirmMessageDelivery(*hSession,
                                                  NULL,
                                                  hDelivery,
                                                  ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                  NULL, 0, NULL);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("ack %d", rc));

        rc = OK; //If the ack went async, this test ignores that.

        //AJ- Actual TEST: Compare the message with a message we expect and checks if each time the message is received.
        if (temp_msg->msgNo != 0)
        {
            if (carS[temp_msg->prodNo].lastMsg != (temp_msg->msgNo - 1))
            {
                //The mesage wasn't the message we expected
                printf(
                        "The message no we expected under the car %d was: %d but the message no we received was %d\n",
                        temp_msg->prodNo,
                        (carS[temp_msg->prodNo].lastMsg + 1),
                        temp_msg->msgNo);
                rc = AJ_RETCODE_WRONG_MSG;
            }
        }
        carS[temp_msg->prodNo].lastMsg = temp_msg->msgNo;
    }

    bool wantAnotherMessage = true;

    if (countIncomingMessages == (NO_CARS_PER_THREAD * NO_MSGS_PER_CAR * NO_THREADS))
    {
        printf(
                "\nITS DONE!! and the Last Message is '%d' under the Car : %d.\n",
                temp_msg->msgNo,
                temp_msg->prodNo);
        wantAnotherMessage = false;
    }


    if ((rc != OK) && (testRetcode == OK))
    {
        testRetcode = rc;
    }
    if (carS[temp_msg->prodNo].lastMsg == NO_MSGS_PER_CAR)
    {
        carS[temp_msg->prodNo].lastMsg = 0;
    }
    ism_engine_releaseMessage(hMessage);
    return wantAnotherMessage;
}
