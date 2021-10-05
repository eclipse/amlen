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
#include <stdlib.h>

#include "ismutil.h"
#include "engineInternal.h"
#include "topicTree.h" //needed at the moment for releasemessage
#include "topicTreeInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_options.h"

iettTopicTree_t * mainTree;
#define NUM_SUBSCRIBERS 100
#define NUM_PUBLISHERS 1
#define NUM_TOPIC_NODES 100

int testStatus;

int        histSize = 10000;		/* can contain up to 10 millisecond latencies */
double     histUnits = 1e-6;		/* units are in microseconds */

typedef struct ourThreadInfo
{
    pthread_t pthread_id; //pthread identifier for thread
    uint32_t  our_id;  //in range 0-num threads of this type
    iettTopicTree_t * mainTree;
    char * topicString;
} ourThreadInfo;

ismEngine_Subscription_t **duplicateSubscriberList(ismEngine_Subscription_t **subscribers)
{
    ismEngine_Subscription_t **result = NULL;
    int i=0;
    while(subscribers[i])
    {
        i++;
    }
    result = calloc(i+1, sizeof(ismEngine_Subscription_t *));
    if (result)
    {
        i = 0;
        while(subscribers[i])
        {
            result[i] = subscribers[i];
            i++;
        }
    }
    return result;
}

void testSingleNode(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    /*Create subscribers*/
    int count=0,rc;
    char * topicString="Sport/Football/Giants";
    ismEngine_Subscription_t **returnSubscribers=NULL;
    ismEngine_Subscription_t **dupList = NULL;
    iettSubscriberList_t       returnSubList={0};

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };

    rc = ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    if (rc != OK)
    {
        testStatus = -3;
        return;
    }

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL,
                                  0,
                                  NULL);
    if (rc != OK)
    {
        testStatus = -3;
        return;
    }

    ismEngine_Consumer_t **consumers = malloc(NUM_SUBSCRIBERS * sizeof(ismEngine_Consumer_t *));
    int32_t               *messageCounts = malloc(NUM_SUBSCRIBERS * sizeof(int32_t));

    for(count=0; count<NUM_SUBSCRIBERS; count++)
    {
        int32_t *currentMessageCount = &(messageCounts[count]);

        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       topicString,
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &currentMessageCount,
                                       sizeof(int32_t *),
                                       NULL, /* No delivery callback */
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_NONE,
                                       &consumers[count],
                                       NULL,
                                       0,
                                       NULL);

        if (rc != OK)
        {
            testStatus=-1;
            break;
        }
    }
    returnSubList.topicString = topicString;
    rc = iett_getSubscriberList(pThreadData, &returnSubList);
    if(rc == OK)
    {
        dupList = duplicateSubscriberList(returnSubList.subscribers);
        iett_releaseSubscriberList(pThreadData, &returnSubList);
        returnSubscribers = dupList;

        count=0;
        ismEngine_Subscription_t * tmpsubscriber;
        tmpsubscriber = returnSubscribers[count];
        while(tmpsubscriber!=NULL)
        {
            count++;
            //printf("TopicString: %s.\n",tpmsubscriber->topicString);
            tmpsubscriber = returnSubscribers[count];
        }
    }
  
    printf("SingleNode-Total Subscribers: %d.\n", count);
    if(NUM_SUBSCRIBERS!=(count))
        testStatus=-2;

    for(count=0; count<NUM_SUBSCRIBERS; count++)
    {
        rc = ism_engine_destroyConsumer(consumers[count],
                                        NULL,
                                        0,
                                        NULL);
        if (rc != OK && rc != ISMRC_AsyncCompletion)
        {
            testStatus = -4;
            return;
        }
    }

    rc = ism_engine_destroySession(hSession,
                                   NULL,
                                   0,
                                   NULL);
    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        testStatus = -5;
        return;
    }

    rc = ism_engine_destroyClientState(hClient,
                                        ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                        NULL,
                                        0,
                                        NULL);
    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        testStatus = -6;
        return;
    }

    if (dupList)free(dupList);
    if (consumers) free(consumers);
    if (messageCounts) free(messageCounts);
}


void testMultiNodes(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    /*Create subscribers*/
    int count=0, node_count=0, rc;
    char * topicString;
    ismEngine_Subscription_t **returnSubscribers=NULL;
    int totalcount=0;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };

    rc = ism_engine_createClientState(__func__,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    if (rc != OK)
    {
        testStatus = -7;
        return;
    }

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL,
                                  0,
                                  NULL);

    if (rc != OK)
    {
        testStatus = -8;
        return;
    }

    ismEngine_Consumer_t **consumers = malloc(NUM_TOPIC_NODES * NUM_SUBSCRIBERS * sizeof(ismEngine_Consumer_t *));
    int32_t               *messageCounts = malloc(NUM_TOPIC_NODES * NUM_SUBSCRIBERS * sizeof(int32_t));

    topicString = (char *)malloc(1024);

    for(node_count=0; node_count<NUM_TOPIC_NODES; node_count++)
    {
        sprintf(topicString,"Sport/Football/Giants%d", node_count);
        for(count=0; count<NUM_SUBSCRIBERS; count++)
        {
            int entry = (node_count*NUM_SUBSCRIBERS)+count;
            int32_t *currentMessageCount = &(messageCounts[entry]);

            rc = ism_engine_createConsumer(hSession,
                                           ismDESTINATION_TOPIC,
                                           topicString,
                                           &subAttrs,
                                           NULL, // Unused for TOPIC
                                           &currentMessageCount,
                                           sizeof(int32_t *),
                                           NULL, /* No delivery callback */
                                           NULL,
                                           ismENGINE_CONSUMER_OPTION_NONE,
                                           &consumers[entry],
                                           NULL,
                                           0,
                                           NULL);

            if(rc != OK)
            {
                testStatus=-1;
                break;
            }
        }
    }

    for(node_count=0; node_count<NUM_TOPIC_NODES; node_count++)
    {
        ismEngine_Subscription_t **dupList=NULL;
        iettSubscriberList_t       returnSubList={0};

        sprintf(topicString,"Sport/Football/Giants%d", node_count);
        returnSubList.topicString = topicString;
        rc= iett_getSubscriberList(pThreadData, &returnSubList);
        count=0;
        if(rc==OK)
        {
            dupList = duplicateSubscriberList(returnSubList.subscribers);
            iett_releaseSubscriberList(pThreadData, &returnSubList);
            returnSubscribers = dupList;

            ismEngine_Subscription_t * tmpsubscriber;
            tmpsubscriber = returnSubscribers[count];
            while(tmpsubscriber!=NULL)
            {
                count++;

                //printf("TopicString: %s.\n",tmpsubscriber->topicString);

                tmpsubscriber = returnSubscribers[count];
            }
        }
        else
        {
            printf("Failed to find subscribers for topic: %s.\n",topicString );
        }
        totalcount+=count;
        if (dupList) free(dupList);
    }
    free(topicString);

    printf("MultiNode-Total Subscribers: %d.\n", totalcount);
    if(NUM_SUBSCRIBERS*NUM_TOPIC_NODES!=(totalcount))
        testStatus=-2;

    for(node_count=0; node_count<NUM_TOPIC_NODES; node_count++)
    {
        for(count=0; count<NUM_SUBSCRIBERS; count++)
        {
            int entry = (node_count*NUM_SUBSCRIBERS)+count;

            rc= ism_engine_destroyConsumer(consumers[entry],
                                             NULL,
                                             0,
                                             NULL);
            if (rc != OK && rc != ISMRC_AsyncCompletion)
            {
                testStatus = -11;
                return;
            }
        }
    }

    rc = ism_engine_destroySession(hSession,
                                   NULL,
                                   0,
                                   NULL);
    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        testStatus = -9;
        return;
    }

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    if (rc != OK && rc != ISMRC_AsyncCompletion)
    {
        testStatus = -10;
        return;
    }

    if (consumers) free(consumers);
    if (messageCounts) free(messageCounts);
}

int main(int argc, char **argv)
{
    ism_time_t seedVal = 0;
    int trclvl = 0;

    testStatus=0;

    testStatus = test_processInit(trclvl, NULL);
    if (testStatus != 0) goto mod_exit;

    testStatus = test_engineInit_DEFAULT;
    if (testStatus != 0) goto mod_exit;

    seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    testSingleNode();
    printf("Test with Multi Nodes and Subscribers.\n");
    testMultiNodes();
  
    testStatus = test_engineTerm(true);
    if (testStatus != 0) goto mod_exit;

    testStatus = test_processTerm(testStatus == 0);
    if (testStatus != 0) goto mod_exit;

mod_exit:

    if(testStatus==0)
    {
        printf("Test passed.\n");
    }
    else
    {
        printf("Test failed. RC=%d (Random Seed=%"PRId64")\n", testStatus, seedVal);
    }

    return testStatus;
}
