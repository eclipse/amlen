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

/********************************************************************/
/*                                                                  */
/* Module Name: simple_pub_sub_new.c                               	*/
/*                                                                  */
/* Description: Simple point to point using pub sub on new engine	*/
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include <ismutil.h>
#include "engine.h"
#include "engineInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_options.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"
#include "test_utils_assert.h"

#define AJ_MSG "AJ-MSG-0"
#define AJ_CAR "AJ_CAR"
#define AJ_RETCODE_WRONG_MSG 1313

#define HOW_MANY 1
#define POINTOFCHECKS 1000000
#define NOPRODS 1000000

int checkLastMsg = 0;
int rc = OK;

int32_t testRetcode = OK;

bool asyncMessageCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
                          void *                          pConsumerContext);


int main(int argc, char *argv[])
{
    ismEngine_ClientStateHandle_t hClientHQ = NULL;
    ismEngine_SessionHandle_t hSessionHQ = NULL;
    ismEngine_ClientStateHandle_t *hClient;
    ismEngine_SessionHandle_t *hSession;
    ismEngine_ProducerHandle_t *hProducer;
    ismEngine_ConsumerHandle_t hConsumer = NULL;
    ismEngine_MessageHandle_t *hMessage;

    hClient = malloc(sizeof(ismEngine_ClientStateHandle_t) * NOPRODS);
    TEST_ASSERT(hClient != NULL, ("Failed to malloc hClient array\n"));
    hSession = malloc(sizeof(ismEngine_SessionHandle_t) * NOPRODS);
    TEST_ASSERT(hClient != NULL, ("Failed to malloc hSession array\n"));
    hProducer = malloc(sizeof(ismEngine_ProducerHandle_t) * NOPRODS);
    TEST_ASSERT(hClient != NULL, ("Failed to malloc hProducer array\n"));
    hMessage = malloc(sizeof(ismEngine_MessageHandle_t) * NOPRODS);
    TEST_ASSERT(hClient != NULL, ("Failed to malloc hMessage array\n"));

    int j;
    char msgBuffer[256], topic[256];
    int trclvl = 0;

    printf("\n\n\n\n\nThe test commenced will be creating %d clients with each of them trying to send and receive %d message with checkpoint at each %d message.\n\n", NOPRODS, HOW_MANY, POINTOFCHECKS);

    rc = test_processInit(trclvl, NULL);
    TEST_ASSERT(rc == OK, ("test_processInit returned %d\n", rc));

    rc = test_engineInit_DEFAULT;
    TEST_ASSERT(rc == OK, ("test_enginInit_DEFAULT returned %d\n", rc));

    rc = sync_ism_engine_createClientState("HeadQuarters",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClientHQ);
    TEST_ASSERT(rc == OK, ("sync_ism_engine_createClientState returned %d\n", rc));

    rc = ism_engine_createSession(hClientHQ,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionHQ,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT(rc == OK, ("ism_engine_createSession returned %d\n", rc));

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };

    rc = ism_engine_createConsumer(hSessionHQ,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   NULL,
                                   0,
                                   asyncMessageCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT(rc == OK, ("ism_engine_createConsumer returned %d\n", rc));

    for(j = 0; j < NOPRODS; j++)
    {
        char temp_name[256];
        sprintf(temp_name,"%s%d", AJ_CAR,j);
        sprintf(topic,"Topic/%d",j);

        rc = ism_engine_createClientState(temp_name,
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL, NULL, NULL,
                                          &hClient[j],
                                          NULL,
                                          0,
                                          NULL);

        rc = ism_engine_createSession(hClient[j],
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hSession[j],
                                      NULL,
                                      0,
                                      NULL);

        rc = ism_engine_createProducer(hSession[j],
                                       ismDESTINATION_TOPIC,
                                       topic,
                                       &hProducer[j],
                                       NULL,
                                       0,
                                       NULL);

        int ref = sprintf(msgBuffer,"%s%d",AJ_MSG, (j+1));
        void *payload = (void *)msgBuffer;
        rc = test_createMessage(ref+1,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, topic,
                                &hMessage[j], &payload);

        rc = ism_engine_putMessage(hSession[j],     // Each message should be put and then released before going to next message
                                   hProducer[j],
                                   NULL,
                                   hMessage[j],
                                   NULL,
                                   0,
                                   NULL);

        if (rc == ISMRC_SomeDestinationsFull) rc = OK;
    }

    rc = ism_engine_startMessageDelivery(hSessionHQ,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);

    for(j = 0; j < NOPRODS; j++)
    {
        rc = ism_engine_destroyProducer(hProducer[j],
                                        NULL,
                                        0,
                                        NULL);

        rc = ism_engine_destroySession(hSession[j],
                                       NULL,
                                       0,
                                       NULL);

        rc = ism_engine_destroyClientState(hClient[j],
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           NULL,
                                           0,
                                           NULL);
    }



    rc = ism_engine_destroyConsumer(hConsumer,
                                    NULL,
                                    0,
                                    NULL);

    rc = ism_engine_destroySession(hSessionHQ,
                                   NULL,
                                   0,
                                   NULL);

    rc = ism_engine_destroyClientState(hClientHQ,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);

    rc = test_engineTerm(true);
    if (rc != OK) return rc;

    rc = test_processTerm(testRetcode == OK);
    if (rc != OK) return rc;

    if (testRetcode == OK)
    {
        printf("Test passed.\n\n");
    }
    else
    {
        printf("Test failed.\n\n");
    }

    free(hClient);
    free(hSession);
    free(hProducer);
    free(hMessage);

    return (int)testRetcode;
}

bool asyncMessageCallback(
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
    static int countIncomingMessage = 1;
    char *reportMsg = NULL;

    if (rc == OK)
    {
        //AJ- Actual TEST: Compare the message payload with a message we expect and checks each time the message is received.
        char temp_msg[256];
        int len;
        static int i=1;
        len = sprintf(temp_msg,"%s%d", AJ_MSG,i++);

        if((i-1)%POINTOFCHECKS==0)
        {
            printf("The message we expected was: %s and the message received is %.*s\n",
                   temp_msg, (int)areaLengths[0], (char *)pAreaData[0]);
        }

        int32_t n;
        for(n=0; n<areaCount; n++)
        {
            if (areaTypes[n] == ismMESSAGE_AREA_PAYLOAD)
            {
                if((len+1)!= areaLengths[n] || strncmp(temp_msg, pAreaData[n], areaLengths[n])!=0)
                {
                    //The mesage wasn't the message we expected
                    printf("The message we expected was: %s but the message we received was %.*s\n",
                           temp_msg, (int)areaLengths[0], (char *)pAreaData[0]);
                    rc = AJ_RETCODE_WRONG_MSG;
                }
                else
                {
                    //The message matched the expected format
                    if((i-1) == NOPRODS)
                    {
                        checkLastMsg = 1;
                        printf("\n\nITS DONE and the last message has length %u and is %s!!\n",
                                (int)areaLengths[0], (char *)pAreaData[0]);
                    }
                }

                break;
            }
        }

        if (n == areaCount)
        {
            printf("The message we expected was: %s but no payload was found\n", temp_msg);
            rc = AJ_RETCODE_WRONG_MSG;
        }

        reportMsg = alloca(areaLengths[0]+1);
        if (reportMsg != NULL)
        {
            memcpy(reportMsg, pAreaData[0], areaLengths[0]);
            reportMsg[areaLengths[0]] = '\0';
        }

        ism_engine_releaseMessage(hMessage);
    }
    bool wantAnotherMessage = true;

    if (countIncomingMessage<=NOPRODS)
    {
        countIncomingMessage++;
    }
    else
    {
        printf("The Last Message is %s.\n", reportMsg ? reportMsg : "<NULLMSG>");

        //stops message delivery callback will unlock the mutex
        wantAnotherMessage = false;
    }

    if ((rc != OK) && (testRetcode == OK))
    {
        testRetcode = rc;
    }

    return wantAnotherMessage;
}
