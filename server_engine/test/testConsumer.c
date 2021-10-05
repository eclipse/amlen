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
/* Module Name: testConsumer                                        */
/*                                                                  */
/* Description: This test starts creates a consumer and then starts */
/*              'n' producing threads. At the end of the test the   */
/*              number of messages produced/received are reported.  */
/*                                                                  */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <getopt.h>

#include <ismutil.h>
#include "engine.h"
#include "engineInternal.h"
#include "policyInfo.h"
#include "store.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_file.h"
#include "test_utils_sync.h"
#include "test_utils_task.h"

#ifdef DEBUG
#define debugPrintf(_x) \
  printf _x
#else
#define debugPrintf(_x)
#endif


/********************************************************************/
/* Local types                                                      */
/********************************************************************/
typedef struct tag_Producer 
{
    int32_t producerId;
    uint32_t DestType;
    char *cpuArray;
    int cpuCount;
    int32_t QoS;
    pthread_t hThread;
    uint32_t msgsToSend;
    uint32_t msgsSent;
} Producer_t;

typedef struct tag_Consumer
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    int32_t QoS;
    int32_t consumerSeqNum;
    uint32_t FailReceiveRatio;  // Fail delivery of 1 in 'n' messages
    uint32_t FailReceiveCounter;
    uint32_t MsgsConsumed;
    uint32_t MsgsRejected;
    uint32_t MsgsRedelivered;
} Consumer_t;

/********************************************************************/
/* Global variables                                                 */
/********************************************************************/

/********************************************************************/
/* Function prototypes                                              */
/********************************************************************/
bool ConsumerCallback( ismEngine_ConsumerHandle_t hConsumer
                     , ismEngine_DeliveryHandle_t hDelivery
                     , ismEngine_MessageHandle_t  hMessage
                     , uint32_t                   deliveryId
                     , ismMessageState_t          state
                     , uint32_t                   destinationOptions
                     , ismMessageHeader_t *       pMsgDetails
                     , uint8_t                    areaCount
                     , ismMessageAreaType_t       areaTypes[areaCount]
                     , size_t                     areaLengths[areaCount]
                     , void *                     pAreaData[areaCount]
                     , void *                     pConsumerContext);
void *ProducerThread(void *arg);
int ProcessArgs( int argc
               , char **argv
               , bool *pDurable
               , uint32_t *pDestType
               , uint32_t *pMReliability
               , uint32_t *pCReliability
               , uint32_t *pnumProducers
               , uint32_t *pnumConsumers
               , uint32_t *pnumMsgs );

/********************************************************************/
/* MAIN                                                             */
/********************************************************************/
int main(int argc, char *argv[])
{
    int i;
    int rc;
    int lrc;
    void *dummy;
    uint32_t msgsSent=0;
    Producer_t *Producers = NULL;
    ismEngine_ClientStateHandle_t hClient = NULL;
    uint32_t prevMsgsConsumed=0;
    uint32_t curMsgsConsumed=0;
    uint32_t numProducers = 8;   /* The default */
    uint32_t numConsumers = 1;   /* The default */
    uint32_t numMsgs = 100000;   /* The default */
    uint32_t MReliability = 2;           /* The default */
    uint32_t CReliability = 2;           /* The default */
    double Delivered = 0;
    bool Durable = 0;
    int num_cpus;
    Consumer_t *Consumers = NULL;
    Consumer_t *pConsumer;
    int32_t trclvl = 4;
    uint32_t DestType = ismDESTINATION_TOPIC;
    uint32_t TotalMsgsReceived = 0;

    /************************************************************************/
    /* Parse the command line arguments                                     */
    /************************************************************************/
    rc = ProcessArgs( argc
                    , argv
                    , &Durable
                    , &DestType
                    , &MReliability
                    , &CReliability
                    , &numProducers
                    , &numConsumers
                    , &numMsgs );
    TEST_ASSERT(rc == OK, ("ERROR: ProcessArgs returned rc=%d\n", rc));

    /************************************************************************/
    /* Calculate out scheduling                                             */
    /************************************************************************/
    num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if ((num_cpus == 0) || (num_cpus < numProducers+2))
    {
      printf("WARNING: Disabling cpu-pinning. Not enough cpus\n");
      num_cpus = 0;
    }

    rc = test_processInit(trclvl, NULL);
    TEST_ASSERT(rc == OK, ("ERROR: test_processInit returned rc=%d\n", rc));

    printf("testConsumer: %s NumProducers(%d) MsgsPerProd(%d) NumConsumers(%d) Consumer-QoS(%d) Msg-QoS(%d)\n",
           (DestType == ismDESTINATION_QUEUE)?"QUEUE(Queue1)":"TOPIC(/Topic/A)",
           numProducers, numMsgs, numConsumers, CReliability, MReliability);

    rc = test_engineInit_DEFAULT;
    TEST_ASSERT(rc == OK, ("ERROR: test_engineInit_DEFAULT returned rc=%d\n", rc));

    rc = ism_engine_createClientState("Consumer Test",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createClientState() returned %d\n", rc));

#if 0
    /************************************************************************/
    /* Sample properties code                                               */
    /************************************************************************/
    char *selector = "Colour='blue'";
    bool nolocal = false;
    ism_prop_t *properties = ism_common_newProperties(2);
    if (properties != NULL) {
        printf("Allocated properities\n");
        if (nolocal) {
            ism_field_t NoLocalField = {VT_Boolean, 0, {.i = true }};
            rc = ism_common_setProperty(properties, "NoLocal", &NoLocalField);

            printf("Created nolocal properties\n");
        }
        if ((selector) && (rc == OK)) {
            ism_field_t SelectorField = {VT_String, 0, {.s = selector }};
            rc = ism_common_setProperty(properties, "Selector", &SelectorField);

            printf("Created selector properties\n");
        }
    }
    else {
        rc = ISMRC_AllocateError;
    }

    if (properties)
    {
        printf("NoLocal value is %s\n", 
          ism_common_getBooleanProperty(properties, "NoLocal", false)?"true":"false");
        printf("Selector string is %s\n", 
          ism_common_getStringProperty(properties, "Selector"));
    }
#endif

    /************************************************************************/
    /* Set the default policy info to allow deep queues                     */
    /************************************************************************/
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 1000000;

    /************************************************************************/
    /* Create the consumer(s)                                               */
    /************************************************************************/
    Consumers = malloc(sizeof(Consumer_t) * numConsumers);
    memset(Consumers, 0, sizeof(Consumer_t) * numConsumers);

    for (i=0; i < numConsumers; i++)
    {
        /********************************************************************/
        /* Create a session for each consumer                               */
        /********************************************************************/
        Consumers[i].QoS = CReliability;
        Consumers[i].consumerSeqNum = 0;
        Consumers[i].FailReceiveRatio = 0;
        Consumers[i].FailReceiveCounter = 0;
        Consumers[i].MsgsConsumed = 0;
        Consumers[i].MsgsRejected = 0;
        Consumers[i].MsgsRedelivered = 0;

        pConsumer= &Consumers[i];

        char clientId[64];
        sprintf(clientId, "Consumer%d", i);
        rc = ism_engine_createClientState(clientId,
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL, NULL, NULL,
                                          &(Consumers[i].hClient),
                                          NULL,
                                          0,
                                          NULL);
        TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createClientState(consumer) returned %d\n", rc));

        rc=ism_engine_createSession( Consumers[i].hClient
                                   , ismENGINE_CREATE_SESSION_OPTION_NONE
                                   , &Consumers[i].hSession
                                   , NULL
                                   , 0 
                                   , NULL);
        TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createSession(consumer) returned %d\n", rc));

        ismEngine_SubscriptionAttributes_t subAttrs = {0};

        if (Durable)
        {
            TEST_ASSERT(DestType == ismDESTINATION_TOPIC, ("DestType was %d", DestType));

            char subName[64];
            sprintf(subName, "testConsumer.%d", i);

            subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                  (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + CReliability);

            rc=ism_engine_createSubscription( hClient
                                            , subName
                                            , NULL
                                            , ismDESTINATION_TOPIC
                                            , "/Topic/A"
                                            , &subAttrs
                                            , NULL // Owning client same as requesting client
                                            , NULL
                                            , 0
                                            , NULL );
            TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createSubscription() returned %d\n", rc));

            rc=ism_engine_createConsumer( pConsumer->hSession
                                        , ismDESTINATION_SUBSCRIPTION
                                        , subName
                                        , NULL
                                        , NULL // Owning client same as session client
                                        , &pConsumer
                                        , sizeof(pConsumer)
                                        , ConsumerCallback
                                        , NULL
                                        , test_getDefaultConsumerOptions(subAttrs.subOptions)
                                        , &pConsumer->hConsumer
                                        , NULL
                                        , 0
                                        , NULL);
            TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createConsumer() returned %d\n", rc));
        }
        else if (DestType == ismDESTINATION_TOPIC)
        {
            subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + CReliability;

            rc=ism_engine_createConsumer( pConsumer->hSession
                                        , ismDESTINATION_TOPIC
                                        , "/Topic/A"
                                        , &subAttrs
                                        , NULL // Unused for TOPIC
                                        , &pConsumer
                                        , sizeof(pConsumer)
                                        , ConsumerCallback
                                        , NULL
                                        , test_getDefaultConsumerOptions(subAttrs.subOptions)
                                        , &pConsumer->hConsumer
                                        , NULL
                                        , 0
                                        , NULL);
            TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createConsumer() returned %d\n", rc));
        }
        else if (DestType == ismDESTINATION_QUEUE)
        {
            subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE + CReliability;
            rc=ism_engine_createConsumer( pConsumer->hSession
                                        , ismDESTINATION_QUEUE
                                        , "Queue1"
                                        , &subAttrs
                                        , NULL // Unused for QUEUE
                                        , &pConsumer
                                        , sizeof(pConsumer)
                                        , ConsumerCallback
                                        , NULL
                                        , (CReliability == 0)
                                        ? ismENGINE_CONSUMER_OPTION_NONE
                                        : ismENGINE_CONSUMER_OPTION_ACK
                                        , &pConsumer->hConsumer
                                        , NULL
                                        , 0
                                        , NULL);
            TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_createConsumer() returned %d\n", rc));
        }

        rc=ism_engine_startMessageDelivery( pConsumer->hSession
                                          , 0
                                          , NULL
                                          , 0
                                          , NULL);
        TEST_ASSERT((rc == OK) || (rc == ISMRC_AsyncCompletion), ("ERROR:  ism_engine_startMessageDeliver() returned %d\n", rc));
    }


    /************************************************************************/
    /* Create the producers                                                 */
    /************************************************************************/
    Producers = malloc(sizeof(Producer_t) * numProducers);
    memset(Producers, 0, sizeof(Producer_t) * numProducers);

    for (i=0; i < numProducers; i++)
    { 
        if (num_cpus > 0)
        {
          Producers[i].cpuArray = (char *)malloc(num_cpus);
          memset(Producers[i].cpuArray, 0, num_cpus);
          Producers[i].cpuArray[i + 2] = 0xff;
          Producers[i].cpuCount = num_cpus;
        }
        else
        {
          Producers[i].cpuCount = 0;
        }
        Producers[i].producerId = i;
        Producers[i].msgsToSend = numMsgs;
        Producers[i].msgsSent = 0;
        Producers[i].DestType = DestType;
        if (MReliability == -1)
        {
            Producers[i].QoS = i % 2;   // toggle between QoS 0 & 1
        }
        else
        {
            Producers[i].QoS = MReliability;
        }

        lrc = test_task_startThread(&Producers[i].hThread,ProducerThread, &(Producers[i]),"ProducerThread");
        TEST_ASSERT(lrc == 0, ("ERROR:  pthread_create(thread=%d) returned %d\n", i, lrc));
    }

    /************************************************************************/
    /* Wait for the producers to end                                        */
    /************************************************************************/
    for (i=0; i < numProducers; i++)
    {
        lrc = pthread_join(Producers[i].hThread, &dummy);
        TEST_ASSERT(lrc == 0, ("ERROR:  pthread_join(thread=%d) returned %d\n", i, lrc));
        msgsSent += Producers[i].msgsSent;
    }

    /************************************************************************/
    /* The producers have ended, wait for the consumer(s) to stop receiving */
    /* messages.                                                            */
    /************************************************************************/
    do
    {
      prevMsgsConsumed = curMsgsConsumed;
      for (i=0, curMsgsConsumed=0; i < numConsumers; i++)
      {
          curMsgsConsumed += Consumers[i].MsgsConsumed;
      }
      sleep(2);
    } while (prevMsgsConsumed != curMsgsConsumed);

    /************************************************************************/
    /* Stop message delivery                                                */
    /************************************************************************/
    TotalMsgsReceived = 0;
    for (i=0; i < numConsumers; i++)
    {
        rc=ism_engine_stopMessageDelivery( Consumers[i].hSession
                                         , NULL
                                         , 0
                                         , NULL);
        TEST_ASSERT((rc == OK) || (rc == ISMRC_AsyncCompletion), ("ERROR:  ism_engine_stopMessageDeliver() returned %d\n", rc));

        /********************************************************************/
        /* Print out how many messages were received by this consumer.      */
        /********************************************************************/
        printf("Consumer[%d]:\n", i);
        printf("    Messages received     = %d\n", Consumers[i].MsgsConsumed);
        printf("    Messages rejected     = %d\n", Consumers[i].MsgsRejected);
        printf("    Messages re-delivered = %d\n", Consumers[i].MsgsRedelivered);

        TotalMsgsReceived += Consumers[i].MsgsConsumed;
    }

    /************************************************************************/
    /* Print out how many messages were put and how many messages were      */
    /* received.                                                            */
    /************************************************************************/
    Delivered = (((double)TotalMsgsReceived) / msgsSent) * 100;
    printf("Consumer:\n");
    printf("    Messages sent         = %d\n", msgsSent);
    printf("    Messages received     = %d\n", TotalMsgsReceived);
    printf("    Delivery success      = %.4f%%\n", Delivered);

    // Drive the client state dumping routine (which includes consumers) - we only
    // want the output to be displayed with verbose logging, but we want the function
    // to be driven in all cases.
    int origStdout = -1;
    if (test_getLogLevel() < testLOGLEVEL_VERBOSE) origStdout = test_redirectStdout("/dev/null");
    rc = ism_engine_dumpClientState(((ismEngine_ClientState_t *)hClient)->pClientId, 9, -1, "");
    TEST_ASSERT(rc == OK, ("Failed to dump client. rc = %d", rc));
    if (origStdout != -1) test_reinstateStdout(origStdout);

    // (void)ism_store_dump( "stdout" );

    /************************************************************************/
    /* cleanup                                                              */
    /************************************************************************/
    for (i=0; i < numConsumers; i++)
    {
        rc=ism_engine_destroyConsumer( Consumers[i].hConsumer
                                     , NULL
                                     , 0
                                     , NULL);
        TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_destroyConsumer() returned %d\n", rc));

        if (Durable)
        {
            char subName[64];
            sprintf(subName, "testConsumer.%d", i);

            rc=ism_engine_destroySubscription( hClient
                                             , subName
                                             , hClient
                                             , NULL
                                             , 0
                                             , NULL );
            TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_destroySubscription() returned %d\n", rc));
        }

        rc=ism_engine_destroySession( Consumers[i].hSession
                                    , NULL
                                    , 0
                                    , NULL);
        TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_destroySession(Consumer[%d]) returned %d\n", i, rc));

        rc=ism_engine_destroyClientState( Consumers[i].hClient
                                        , ismENGINE_DESTROY_CLIENT_OPTION_NONE
                                        , NULL
                                        , 0
                                        , NULL);
        TEST_ASSERT(rc == OK, ("ERROR:  ism_engine_destroyClientState() returned %d\n", rc));
    }

    rc = test_engineTerm(true);
    TEST_ASSERT(rc == OK, ("ERROR:  test_engineTerm returned %d\n", rc));

    if (Producers)
    {
        for (i=0; i < numProducers; i++)
        {
            if (Producers[i].cpuArray != NULL)
            {
                free(Producers[i].cpuArray);
            }
        }
        free(Producers);
    }

    test_processTerm(true);

    if (Consumers)
    {
        free(Consumers);
    }

    return rc;
}

/****************************************************************************/
/* ConsumerCallback                                                         */
/****************************************************************************/
bool ConsumerCallback( ismEngine_ConsumerHandle_t hConsumer
                     , ismEngine_DeliveryHandle_t hDelivery
                     , ismEngine_MessageHandle_t  hMessage
                     , uint32_t                   deliveryId
                     , ismMessageState_t          state
                     , uint32_t                   destinationOptions
                     , ismMessageHeader_t *       pMsgDetails
                     , uint8_t                    areaCount
                     , ismMessageAreaType_t       areaTypes[areaCount]
                     , size_t                     areaLengths[areaCount]
                     , void *                     pAreaData[areaCount]
                     , void *                     pConsumerContext)
{
    uint32_t rc;
    Consumer_t *pConsumer = *(Consumer_t **)pConsumerContext;
    uint32_t __attribute__((unused)) msgCount = pConsumer->MsgsConsumed+1;

    // debugPrintf(("Received Message(%d) - %s\n", msgCount, (char *)pAreaData[0]));

    if (pMsgDetails->RedeliveryCount)
    {
        pConsumer->MsgsRedelivered++;
    }

    // After we release this message we cannot access either the pMsgDetails
    // or the hMessage!
    ism_engine_releaseMessage(hMessage);

    switch (state)
    {
        case ismMESSAGE_STATE_DELIVERED:
            // If this is the first time a message has been delivered, we expect the 
            // delivery id to be initialised to zero and we must set an appropriate
            // delivery Id.
            if (pMsgDetails->RedeliveryCount == 0)
            {
                // Currently we are just incrementing the pConsumer->consumerSeqNum
                // with no concern about re-use.
                pConsumer->consumerSeqNum++;

//                rc = ism_engine_setMessageDeliveryId( pConsumer->hConsumer
//                                                    , hDelivery
//                                                    , pConsumer->consumerSeqNum
//                                                    , NULL
//                                                    , 0
//                                                    , NULL );
//
//                switch (rc)
//                {
//                    case OK:
//                        break;
//                    case ISMRC_AsyncCompletion:
//                        fprintf(stderr, "Unable to support async completion of delivery!\n");
//                        assert(false);
//                        break;
//                    default:
//                        fprintf(stderr, "Unknown return code %d from ism_engine_confirmMessageDelivery()\n",
//                                rc);
//                        assert(false);
//                        break;
//                }
            }

            if ((pConsumer->FailReceiveRatio) &&
                (++pConsumer->FailReceiveCounter == pConsumer->FailReceiveRatio))
            {
                pConsumer->FailReceiveCounter = 0;
#if 1
                rc = ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                      , NULL
                                                      , hDelivery
                                                      , ismENGINE_CONFIRM_OPTION_NOT_DELIVERED
                                                      , NULL
                                                      , 0
                                                      , NULL );

                switch (rc)
                {
                    case OK: 
                        pConsumer->MsgsRejected++;
                        break;
                    case ISMRC_AsyncCompletion:
                        fprintf(stderr, "Unable to support async completion of delivery!\n");
                        TEST_ASSERT(false, ("Unable to support async completion of delivery!"));
                        break;
                    default:
                        fprintf(stderr, "Unknown return code %d from ism_engine_confirmMessageDelivery()\n",
                                rc);
                        TEST_ASSERT(false,("Unknown return code %d from ism_engine_confirmMessageDelivery()", rc));
                        break;
                }
#endif
            }
            else
            {
                if (pConsumer->QoS == 2)
                {
                    // For Exactly-Once we must furst acknowledge that the message
                    // has been received, before we identify that the message has 
                    // been consumed.
                    rc = ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                          , NULL
                                                          , hDelivery
                                                          , ismENGINE_CONFIRM_OPTION_RECEIVED
                                                          , NULL
                                                          , 0
                                                          , NULL );

                    switch (rc)
                    {
                        case OK: 
                            break;
                        case ISMRC_AsyncCompletion:
                            fprintf(stderr, "Unable to support async completion of delivery!\n");
                            TEST_ASSERT(false,("Unable to support async completion of delivery!"));
                            break;
                        default:
                            fprintf(stderr, "Unknown return code %d from ism_engine_confirmMessageDelivery()\n",
                                    rc);
                            TEST_ASSERT(false,("Unknown return code %d from ism_engine_confirmMessageDelivery", rc));
                            break;
                    }
                }

                rc = ism_engine_confirmMessageDelivery( pConsumer->hSession
                                                      , NULL
                                                      , hDelivery
                                                      , ismENGINE_CONFIRM_OPTION_CONSUMED
                                                      , NULL
                                                      , 0
                                                      , NULL );

                switch (rc)
                {
                    case OK: 
                        pConsumer->MsgsConsumed++;
                        break;
                    case ISMRC_AsyncCompletion:
                        fprintf(stderr, "Unable to support async completion of delivery!\n");
                        TEST_ASSERT(false, ("Unable to support async completion of delivery!"));
                        break;
                    default:
                        fprintf(stderr, "Unknown return code %d from ism_engine_confirmMessageDelivery()\n",
                                rc);
                        TEST_ASSERT(false, ("Unknown return code %d from ism_engine_confirmMessageDelivery",rc));
                        break;
                }
            }
            break;
        case ismMESSAGE_STATE_CONSUMED:
            pConsumer->MsgsConsumed++;
            break;
        default:
            fprintf(stderr, "Unknown message state %d\n", state);
            TEST_ASSERT(false,("Unknown message state %d", state));
            break;
    }
                                               
    return true;
}

/****************************************************************************/
/* ProducerThread                                                           */
/****************************************************************************/
static void putComplete(int32_t rc, void *handle, void *pContext)
{
   if(rc != OK)
   {
       printf("rc was %d\n", rc);
       abort();
   }

   Producer_t *Producer = *(Producer_t **)pContext;

   ASYNCPUT_CB_ADD_AND_FETCH(&(Producer->msgsSent), 1);
}

void *ProducerThread(void *arg)
{
    int rc = 0;
    uint32_t msgCount=0;
    Producer_t *Producer = (Producer_t *)arg;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismEngine_ClientStateHandle_t hClient = NULL;
    ismEngine_SessionHandle_t hSession = NULL;
    ismEngine_ProducerHandle_t hProducer;
    ismEngine_MessageHandle_t  hMessage;
    header.Reliability = Producer->QoS ;
    header.Persistence = (Producer->QoS == 0) ? ismMESSAGE_PERSISTENCE_NONPERSISTENT
                                              : ismMESSAGE_PERSISTENCE_PERSISTENT;

    debugPrintf(("Starting Producer %d\n", Producer->producerId));

    ism_engine_threadInit(0);

    (void)ism_common_setAffinity(pthread_self(), Producer->cpuArray, Producer->cpuCount);

    char clientId[64];
    sprintf(clientId, "producer%d", Producer->producerId);

    rc = ism_engine_createClientState(clientId,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);

    if (rc != OK)
    {
        printf("ERROR:  ism_engine_createClientState(producer(%d)) returned %d\n", Producer->producerId, rc);
        goto mod_exit;
    }

    /************************************************************************/
    /* Create a session for the producer                                    */
    /************************************************************************/
    rc=ism_engine_createSession( hClient
                               , ismENGINE_CREATE_SESSION_OPTION_NONE
                               , &hSession
                               , NULL
                               , 0 
                               , NULL);
    if (rc != OK)
    {
        printf("ERROR:  ism_engine_createSession(producer(%d)) returned %d\n", Producer->producerId, rc);
        goto mod_exit;
    }

    rc=ism_engine_createProducer( hSession
                                , Producer->DestType
                                , (Producer->DestType == ismDESTINATION_TOPIC)?
                                  "/Topic/A" : "Queue1"
                                , &hProducer
                                , NULL
                                , 0
                                , NULL);
    if (rc != OK)
    {
        printf("ERROR:  ism_engine_createProducer(producer(%d)) returned %d\n", Producer->producerId, rc);
        goto mod_exit;
    }

    /************************************************************************/
    /* Deliver the messages                                                 */
    /************************************************************************/
    for(; msgCount < Producer->msgsToSend; msgCount++)
    {
        ismMessageAreaType_t areaTypes[1];
        size_t areaLengths[1];
        void *areas[1];
        char Buffer[1024];

        sprintf(Buffer, "Consumer_test - Producer(%d) Message(%d)\n", Producer->producerId, msgCount);
        areaTypes[0] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[0] = strlen(Buffer) +1;
        areas[0] = (void *)Buffer;

        rc=ism_engine_createMessage( &header
                                   , 1
                                   , areaTypes
                                   , areaLengths
                                   , areas
                                   , &hMessage);
        if (rc != OK)
        {
            printf("ERROR:  ism_engine_createMessage(producer(%d), message(%d)) returned %d\n", Producer->producerId, msgCount, rc);
            goto mod_exit;
        }
  
        rc=ism_engine_putMessage( hSession
                                , hProducer
                                , NULL
                                , hMessage
                                , &Producer
                                , sizeof(Producer_t *)
                                , putComplete);

        if(rc != OK && rc != ISMRC_AsyncCompletion)
        {
            printf("rc was %d\n",  rc);
            abort();
        }

        if (rc == OK)
        {
            ASYNCPUT_CB_ADD_AND_FETCH(&(Producer->msgsSent), 1);
        }
    }

    rc=ism_engine_destroyProducer( hProducer
                                 , NULL
                                 , 0
                                 , NULL);
    if (rc != OK)
    {
        printf("ERROR:  ism_engine_destroyProducer(producer(%d)) returned %d\n", Producer->producerId, rc);
        goto mod_exit;
    }

    rc=ism_engine_destroySession( hSession
                                , NULL
                                , 0
                                , NULL);
    if (rc != OK)
    {
        printf("ERROR:  ism_engine_destroySession(producer(%d)) returned %d\n", Producer->producerId, rc);
        goto mod_exit;
    }

    rc=ism_engine_destroyClientState( hClient
                                    , ismENGINE_DESTROY_CLIENT_OPTION_NONE
                                    , NULL
                                    , 0
                                    , NULL);
    if (rc != OK)
    {
        printf("ERROR:  ism_engine_destroyClientState(producer(%d)) returned %d\n", Producer->producerId, rc);
        goto mod_exit;
    }

    while((volatile int32_t)Producer->msgsSent < Producer->msgsToSend)
    {
        sched_yield();
    }


mod_exit:
    ism_engine_threadTerm(1);

    debugPrintf(("Producer PUT %d messages\n", msgCount));

    return NULL;
}

/****************************************************************************/
/* ProcessArgs                                                              */
/****************************************************************************/
int ProcessArgs( int argc
               , char **argv
               , bool *pDurable
               , uint32_t *pDestType
               , uint32_t *pMReliability
               , uint32_t *pCReliability
               , uint32_t *pnumProducers
               , uint32_t *pnumConsumers
               , uint32_t *pnumMsgs )
{
    int usage = 0;
    bool Topic=false, Queue=false;
    char opt;
    struct option long_options[] = {
        { "producers", 1, NULL, 'p' }, 
        { "consumers", 1, NULL, 'c' }, 
        { "numMsgs", 1, NULL, 'n' }, 
        { "topic", 0, NULL, 't' }, 
        { "queue", 0, NULL, 'q' }, 
        { "durable", 0, NULL, 'd' }, 
        { "mrel", 1, NULL, 0 }, 
        { "crel", 1, NULL, 0 }, 
        { NULL, 1, NULL, 0 } };
    int long_index;

    *pDestType = ismDESTINATION_TOPIC;

    while ((opt = getopt_long(argc, argv, ":dp:c:n:qt", long_options, &long_index)) != (char)-1)
    {
        switch (opt)
        {
            case 0:
               if (long_index == 6)
               {
                 *pMReliability = atoi(optarg);
               }
               else if (long_index == 7)
               {
                 *pCReliability = atoi(optarg);
               }
               break;
            case 't':
               if (Queue)
                   usage=1;
               else
                   Topic=true;
               break;
            case 'q':
               if (Topic)
                   usage=1;
               else
                   Queue=true;
               break;
            case 'd':
               *pDurable=true;
               break;
            case 'p':
               *pnumProducers = atoi(optarg);
               break;
            case 'c':
               *pnumConsumers = atoi(optarg);
               break;
            case 'n':
               *pnumMsgs = atoi(optarg);
               break;
            case '?':
               usage=1;
               break;
            default: 
               usage=1;
               break;
        }
    }

    if ((optind != argc) || (*pnumProducers == 0) || (*pnumMsgs == 0) || (*pCReliability > 2) || ((*pMReliability >2) && (*pMReliability != -1)))
    {
        usage=1;
    }

    if (Queue)
       *pDestType = ismDESTINATION_QUEUE;

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-t | -q] [--mrel 0-2 ] [--crel 0-2] [-p num-producers] [-c num-consumers] [-n msg-count]\n", argv[0]);
        fprintf(stderr, "       -t - TOPIC's\n");
        fprintf(stderr, "       -q - QUEUE's\n");
        fprintf(stderr, "       --mrel - Message Reliability 0 = At-most-once\n");
        fprintf(stderr, "                                    1 = At-least-once\n");
        fprintf(stderr, "                                    2 = Exactly-once\n");
        fprintf(stderr, "       --crel - Consumer Reliability 0 = At-most-once\n");
        fprintf(stderr, "                                     1 = At-least-once\n");
        fprintf(stderr, "                                 a   2 = Exactly-once\n");
        fprintf(stderr, "       -p - Number of producers\n");
        fprintf(stderr, "       -c - Number of consumers\n");
        fprintf(stderr, "       -n - Number of messages per producer\n");
    }

    return usage;
}
