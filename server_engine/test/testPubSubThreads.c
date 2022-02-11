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
/* Module Name: testPubSubThreads                                    */
/*                                                                   */
/* Description: UnitTest functions for testing pub/sub..see header   */
/* file testPubSubThreads.h                                          */
/*                                                                   */
/*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <sys/prctl.h>
#include <sys/time.h>

#include "engine.h"
#include "engineInternal.h"
#include "ismutil.h"
#include "memHandler.h"
#include "testPubSubThreads.h"

#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"
#include "test_utils_task.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

enum OPTYPE {
    sub,
    pub,
};

uint32_t ThreadId = 0;

typedef struct callbackContext {
    enum OPTYPE optype;
    pthread_mutex_t *torelease;
    int msgs;
    int msgsSinceGroupUpdate;
    psgroupinfo *ginfo;
    ismEngine_ClientState_t *hClient;
    ismEngine_Session_t *hSession;
    ismEngine_Consumer_t *hConsumer;
    ismEngine_Producer_t *hProducer;
    int32_t retcode;
} callbackContext;

typedef struct ourThreadInfo {
    pthread_t pthread_id; //pthread identifier for thread
    uint32_t  our_id;  //in range 0-num threads of this type
    psgroupinfo *ginfo;
} ourThreadInfo;

bool memoryTestInProgress = false;
pthread_t memoryTestThreadId;
pthread_t consumerReenablerTheadId;
bool memoryTestThreadsShouldEnd = false;
uint32_t globalThreadId = 0;

void createClientCallback(int32_t retcode, void *handle, void *pContext);
void destroyClientCallback(int32_t retcode, void *handle, void *pContext);
void createSessionCallback(int32_t retcode, void *handle, void *pContext);
void destroySessionCallback(int32_t retcode, void *handle, void *pContext);

void PubberCreateProducerCallback(int32_t retcode, void *handle, void *pContext);
void PubberDestroyProducerCallback(int32_t retcode, void *handle, void *pContext);
void PubberPutMessageCallback(int32_t retcode, void *handle, void *pContext);

void SubberCreateSubscriptionCallback(int32_t retcode, void *handle, void *pContext);
void SubberCreateConsumerCallback(int32_t retcode, void *handle, void *pContext);
void SubberDestroyConsumerCallback(int32_t retcode, void *handle, void *pContext);

bool SubberGotMessageCallback(
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
        void *                          pConsumerContext,
        ismEngine_DelivererContext_t *  _delivererContext);

bool BgSubberGotMessageCallback(
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
        void *                          pConsumerContext,
        ismEngine_DelivererContext_t *  _delivererContext);

/*
 * Get the number of CPU threads available
 */
static int getHardwareNumCPUs(void)
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}

/*
 * This function pins the current thread to the next CPU,
 * distributing threads around the CPUs.
 *
 * We start by pinning to the 'last' CPU so it is less likely
 * we will conflict with worker threads (if we ever have worker
 * threads again), so, in a 4 CPU system, we choose the 4th (CPU 3)
 * first, then CPU 2, and so on. The 5th call will choose the 4th
 * CPU again (CPU 3).
 */
static void pinToNextCPU(void)
{
    char CPUs[128] = {0};

    static int hardwareNumCPUs = -1;
    static int nextCPU = 0;

    if (hardwareNumCPUs == -1) {
        hardwareNumCPUs = getHardwareNumCPUs();

        if (hardwareNumCPUs == -1) {
            printf("ERROR: getHardwareNumCPUs returned %d\n", hardwareNumCPUs);
            return;
        }
    }

    int myCPU = (hardwareNumCPUs - (__sync_fetch_and_add(&nextCPU, 1) % hardwareNumCPUs)) - 1;

    if (myCPU < sizeof(CPUs)) {
        CPUs[myCPU] = 1;
        ism_common_setAffinity(ism_common_threadSelf(), CPUs, sizeof(CPUs));
    }
    else {
        printf("ERROR: Unable to pin to CPU %d\n", myCPU);
    }
}

int threadWaitingForGroupSetup(psgroupinfo *ginfo)
{
    int rc;
    //Record that this thread is ready to do setup when asked
    rc = pthread_mutex_lock(&(ginfo->controller->mutex_setup));

    if (OK == rc) {
        ginfo->controller->threadsReadySetup++;

        if(   ginfo->controller->threadsReadySetup
           == ginfo->controller->threadsControlled) {
            rc = pthread_cond_broadcast(&(ginfo->controller->cond_setup));
        }

        int rc2 = pthread_mutex_unlock(&(ginfo->controller->mutex_setup));
        if ((rc2 != OK)&&(rc == OK)) {
            rc = rc2;
        }
    }
    //wait for the signal to do setup
    if (rc == OK) {
        rc = pthread_mutex_lock(&(ginfo->controller->mutex_setup));

        while ((rc == OK) &&
                (  ginfo->controller->phase == initing)) {
            rc = pthread_cond_wait(&(ginfo->controller->cond_setup)
                                  ,&(ginfo->controller->mutex_setup));
        }

        int rc2 = pthread_mutex_unlock(&(ginfo->controller->mutex_setup));

        if ((rc2 != OK)&&(rc == OK)) {
            rc = rc2;
        }
    }
    return rc;
}

int threadWaitingForGroupStart(psgroupinfo *ginfo)
{
    int rc;
    //Record that this thread is ready to start
    rc = pthread_mutex_lock(&(ginfo->controller->mutex_start));

    if (OK == rc) {
        ginfo->controller->threadsReadyStart++;

        if(   ginfo->controller->threadsReadyStart
           == ginfo->controller->threadsControlled) {
            rc = pthread_cond_broadcast(&(ginfo->controller->cond_start));
        }

        int rc2 = pthread_mutex_unlock(&(ginfo->controller->mutex_start));
        if ((rc2 != OK)&&(rc == OK)) {
            rc = rc2;
        }
    }
    //wait for the signal to go
    if (rc == OK) {
        rc = pthread_mutex_lock(&(ginfo->controller->mutex_start));

        while ((rc == OK) &&
                (  ginfo->controller->phase == settingup)) {
            rc = pthread_cond_wait(&(ginfo->controller->cond_start)
                                  ,&(ginfo->controller->mutex_start));
        }

        int rc2 = pthread_mutex_unlock(&(ginfo->controller->mutex_start));

        if ((rc2 != OK)&&(rc == OK)) {
            rc = rc2;
        }
    }
    return rc;
}

int threadfinished(psgroupinfo *ginfo) {
    int rc;

    //Broadcast that this thread has finished
    rc = pthread_mutex_lock(&(ginfo->controller->mutex_finish));

    if (OK == rc) {
        ginfo->controller->threadsFinished++;

        if(   ginfo->controller->threadsFinished
           == ginfo->controller->threadsControlled) {
            rc = pthread_cond_broadcast(&(ginfo->controller->cond_finish));
        }

        int rc2 = pthread_mutex_unlock(&(ginfo->controller->mutex_finish));
        if ((rc2 != OK)&&(rc == OK)) {
            rc = rc2;
        }
    }
    return rc;
}

int TriggerSetupThreadGroups(psgroupcontroller *gcontroller,
                             ism_time_t *setupTime,
                             bool memTest) {
    int rc;

    //wait for the threads to be ready
    rc = pthread_mutex_lock(&(gcontroller->mutex_setup));

    while ((rc == OK) &&
            (  gcontroller->threadsReadySetup
             < gcontroller->threadsControlled)) {
        rc = pthread_cond_wait(&(gcontroller->cond_setup)
                              ,&(gcontroller->mutex_setup));
    }

    int rc2 = pthread_mutex_unlock(&(gcontroller->mutex_setup));

    if ((rc2 != OK)&&(rc == OK)) {
        rc = rc2;
    }

    if (memTest) {
        //Now that the threads have inited themselves it's time to
        //start failing some allocs.
        startMemoryTest();
    }

    //Broadcast that it's time to start
    if (rc == OK) {
        rc = pthread_mutex_lock(&(gcontroller->mutex_setup));

        if (OK == rc) {

            *setupTime = ism_common_currentTimeNanos();

            gcontroller->phase = settingup;

            rc = pthread_cond_broadcast(&(gcontroller->cond_setup));

            rc2 = pthread_mutex_unlock(&(gcontroller->mutex_setup));
            if ((rc2 != OK)&&(rc == OK)) {
                rc = rc2;
            }
        }
    }

    return rc;
}
int StartThreadGroups(psgroupcontroller *gcontroller,
                      ism_time_t *startTime) {
    int rc;

    //wait for the threads to be ready
    rc = pthread_mutex_lock(&(gcontroller->mutex_start));

    while ((rc == OK) &&
            (  gcontroller->threadsReadyStart
             < gcontroller->threadsControlled)) {
        rc = pthread_cond_wait(&(gcontroller->cond_start)
                              ,&(gcontroller->mutex_start));
    }

    int rc2 = pthread_mutex_unlock(&(gcontroller->mutex_start));

    if ((rc2 != OK)&&(rc == OK)) {
        rc = rc2;
    }

    //Broadcast that it's time to start
    if (rc == OK) {
        rc = pthread_mutex_lock(&(gcontroller->mutex_start));

        if (OK == rc) {

            *startTime = ism_common_currentTimeNanos();

            gcontroller->phase = running;

            rc = pthread_cond_broadcast(&(gcontroller->cond_start));

            rc2 = pthread_mutex_unlock(&(gcontroller->mutex_start));
            if ((rc2 != OK)&&(rc == OK)) {
                rc = rc2;
            }
        }
    }

    return rc;
}
void *pubberController(void * args) {
    ourThreadInfo *tinfo = (ourThreadInfo *)args;
    pthread_mutex_t senderMutex = PTHREAD_MUTEX_INITIALIZER;
    int32_t rc = OK;
    char tname[20];

    pthread_mutexattr_t mutexattrs;
    callbackContext ctxt;
    callbackContext *pCtxt = &ctxt;

    sprintf(tname, "PSCFG-%d", __sync_add_and_fetch(&ThreadId, 1));
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    ism_engine_threadInit(0);

    ctxt.optype = pub;
    ctxt.torelease = &senderMutex;
    ctxt.ginfo = tinfo->ginfo;
    ctxt.hClient = NULL; //we'll create it in a sec...
    ctxt.hSession = NULL; //We'll create it in a sec...
    ctxt.hProducer = NULL; //We'll create it in a sec..
    ctxt.hConsumer = NULL; //unused
    ctxt.msgs =0;
    ctxt.msgsSinceGroupUpdate = 0;

    if (ctxt.ginfo->cpuPin) {
        pinToNextCPU();
    }

    //Finished copying data of threadinfo structure now...
    free(tinfo);

    threadWaitingForGroupSetup(ctxt.ginfo);

    /* Ensure that we have defined behaviour in the case of relocking a mutex */
    pthread_mutexattr_init(&mutexattrs);
    pthread_mutexattr_settype(&mutexattrs, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&senderMutex, &mutexattrs);

    //Create a client
    pthread_mutex_lock(&senderMutex);
    char clientid[64];
    sprintf(clientid, PUTTER_CLIENTID_FORMAT, tname);

    do {
        rc = ism_engine_createClientState(clientid,
                                          testDEFAULT_PROTOCOL_ID,
                                          (ctxt.ginfo->QoS == 0) ? ismENGINE_CREATE_CLIENT_OPTION_NONE:
                                                                   ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          NULL, NULL, NULL,
                                          &(ctxt.hClient),
                                          &pCtxt,
                                          sizeof(callbackContext *),
                                          createClientCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&senderMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_createClientState rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    //Create a session
    do {

        rc = ism_engine_createSession( ctxt.hClient
                                       , ismENGINE_CREATE_CLIENT_OPTION_NONE
                                       , &(ctxt.hSession)
                                       , &pCtxt
                                       , sizeof(callbackContext *)
                                       , createSessionCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&senderMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_createSession rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    /* Create a message to put */
    ismEngine_MessageHandle_t hMessage = NULL;

    char *msgPayload = "Message body";
    uint8_t msgFlags = ctxt.ginfo->pubMsgFlags;
    uint8_t msgPersistence = ctxt.ginfo->pubMsgPersistence;
    uint8_t msgReliability = ismMESSAGE_RELIABILITY_AT_MOST_ONCE;

    if (ctxt.ginfo->QoS > 0) {
        if(ctxt.ginfo->QoS == 1) {
            msgReliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
        } else {
            msgReliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
        }
    }

    // If this publisher is spraying messages, do that
    if (ctxt.ginfo->pubSprayWidth > 0) {
        char *topicString = malloc(strlen(ctxt.ginfo->topicString)+10);

        pthread_mutex_unlock(&senderMutex);

        threadWaitingForGroupStart(ctxt.ginfo);

        pthread_mutex_lock(&senderMutex);
        int j = 0;
        while(j < ctxt.ginfo->msgsPerPubber) {
            int topicNumber = 0;
            while(topicNumber < ctxt.ginfo->pubSprayWidth) {
                sprintf(topicString, "%s/%05d", ctxt.ginfo->topicString, topicNumber);

                hMessage = ismENGINE_NULL_MESSAGE_HANDLE;
                do {
                    void *payloadPtr = msgPayload;
                    rc = test_createMessage(strlen(msgPayload)+1,
                                            msgPersistence,
                                            msgReliability,
                                            msgFlags,
                                            0,
                                            ismDESTINATION_TOPIC, topicString,
                                            &hMessage, &payloadPtr);
                } while(memoryTestInProgress && (rc == ISMRC_AllocateError));

                TEST_ASSERT(rc == OK, ("Message creation failed rc=%d\n", rc));

                rc = ism_engine_putMessageOnDestination(ctxt.hSession,
                                                        ismDESTINATION_TOPIC,
                                                        topicString,
                                                        NULL,
                                                        hMessage,
                                                        &pCtxt,
                                                        sizeof(callbackContext *),
                                                        PubberPutMessageCallback);

                if (memoryTestInProgress && (rc == ISMRC_AllocateError)) {
                    //We weren't able to put the message, loop around again
                } else {
                    if (rc != ISMRC_AsyncCompletion) {
                        PubberPutMessageCallback(rc, NULL, &pCtxt);
                    }
                    j++;
                    topicNumber++;
                }
            }
        }

        free(topicString);

        //Wait for all the callbacks to complete...the mutex will be unlocked when it is

        pthread_mutex_lock(&senderMutex);
    }
    else
    {
        do {
            rc = ism_engine_createProducer(ctxt.hSession,
                                           ismDESTINATION_TOPIC,
                                           ctxt.ginfo->topicString,
                                           &(ctxt.hProducer),
                                           &pCtxt,
                                           sizeof(callbackContext *),
                                           PubberCreateProducerCallback);

            if (rc == ISMRC_AsyncCompletion) {
                //The callback will unlock the mutex when done
                pthread_mutex_lock(&senderMutex);
            } else {
                //Happened inline, set the return code
                ctxt.retcode = rc;
            }
        } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

        if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
        {
            fprintf(stderr, "%d: ism_engine_createProducer rc=%d\n", __LINE__, ctxt.retcode);
            ctxt.ginfo->groupRetCode = ctxt.retcode;
        }

        pthread_mutex_unlock(&senderMutex);

        threadWaitingForGroupStart(ctxt.ginfo);

        pthread_mutex_lock(&senderMutex);
        int j = 0;
        while(j < ctxt.ginfo->msgsPerPubber) {
            hMessage = ismENGINE_NULL_MESSAGE_HANDLE;
            do {
                void *payloadPtr = msgPayload;
                rc = test_createMessage(strlen(msgPayload)+1,
                                        msgPersistence,
                                        msgReliability,
                                        msgFlags,
                                        0,
                                        ismDESTINATION_TOPIC, ctxt.ginfo->topicString,
                                        &hMessage, &payloadPtr);
            } while(memoryTestInProgress && (rc == ISMRC_AllocateError));

            TEST_ASSERT(rc == OK, ("Return code from message creation %d\n", rc));

            rc = ism_engine_putMessage(ctxt.hSession,
                                       ctxt.hProducer,
                                       NULL,
                                       hMessage,
                                       &pCtxt,
                                       sizeof(callbackContext *),
                                       PubberPutMessageCallback);

            if(memoryTestInProgress && (rc == ISMRC_AllocateError)) {
                //We weren't able to put the message, loop around and retry
            } else {
                if( rc != ISMRC_AsyncCompletion) {
                    PubberPutMessageCallback(rc, ctxt.hProducer, &pCtxt);
                }
                j++;
            }
        }

        //Wait for all the callbacks to complete...the mutex will be unlocked when it is
        pthread_mutex_lock(&senderMutex);

        do {
            rc = ism_engine_destroyProducer(ctxt.hProducer,
                                            &pCtxt,
                                            sizeof(callbackContext *),
                                            PubberDestroyProducerCallback);

            if (rc == ISMRC_AsyncCompletion) {
                //The callback will unlock the mutex when done
                pthread_mutex_lock(&senderMutex);
            } else {
                //Happened inline, set the return code
                ctxt.retcode = rc;
            }
        } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

        if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
        {
            fprintf(stderr, "%d: ism_engine_destroyProducer rc=%d\n", __LINE__, ctxt.retcode);
            ctxt.ginfo->groupRetCode = ctxt.retcode;
        }
    }

    do {
        rc = ism_engine_destroySession(ctxt.hSession,
                                       &pCtxt,
                                       sizeof(callbackContext *),
                                       destroySessionCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&senderMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_destroySession rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    do {
        rc = ism_engine_destroyClientState(ctxt.hClient,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           &pCtxt,
                                           sizeof(callbackContext *),
                                           destroyClientCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&senderMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_destroyClient rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    pthread_mutex_unlock(&senderMutex);

    threadfinished(ctxt.ginfo);

    ism_engine_threadTerm(1);
    ism_common_freeTLS();

    return NULL;
}

static void * subberController(void * args) {
    ourThreadInfo *tinfo = (ourThreadInfo *)args;
    pthread_mutex_t receiverMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutexattr_t mutexattrs;
    callbackContext ctxt;
    callbackContext *pCtxt = &ctxt;
    int32_t rc;
    char tname[20];
    uint32_t ourThreadId = tinfo->our_id;
    ismEngine_SubscriptionAttributes_t subAttributes = { ismENGINE_SUBSCRIPTION_OPTION_NONE };

    sprintf(tname, "PSCFG-%d", __sync_add_and_fetch(&ThreadId, 1));

    ism_engine_threadInit(0);

    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    ctxt.optype = sub;
    ctxt.torelease = &receiverMutex;
    ctxt.ginfo = tinfo->ginfo;
    ctxt.hClient = NULL;   //We'll create it in a sec...
    ctxt.hSession = NULL; //We'll create it in a sec...
    ctxt.hProducer = NULL; //unused
    ctxt.hConsumer = NULL; //We'll create it in a sec..
    ctxt.msgs = 0;
    ctxt.msgsSinceGroupUpdate = 0;

    //Finished copying data of threadinfo structure now...
    free(tinfo);

    threadWaitingForGroupSetup(ctxt.ginfo);

    /* Ensure that we have defined behaviour in the case of relocking a mutex */
    pthread_mutexattr_init(&mutexattrs);
    pthread_mutexattr_settype(&mutexattrs, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&receiverMutex, &mutexattrs);


    //Create a client
    pthread_mutex_lock(&receiverMutex);
    char clientid[64];
    sprintf(clientid, SUBBER_CLIENTID_FORMAT, ourThreadId);

    do {
        rc = ism_engine_createClientState(clientid,
                                          testDEFAULT_PROTOCOL_ID,
                                          (ctxt.ginfo->QoS == 0) ? ismENGINE_CREATE_CLIENT_OPTION_NONE:
                                                                   ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          NULL, NULL, NULL,
                                          &(ctxt.hClient),
                                          &pCtxt,
                                          sizeof(callbackContext *),
                                          createClientCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&receiverMutex);
        } else {
            //Happened inline set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_createClientState rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    do {
        rc = ism_engine_createSession( ctxt.hClient
                                       , ismENGINE_CREATE_SESSION_OPTION_NONE
                                       , &(ctxt.hSession)
                                       , &pCtxt
                                       , sizeof(callbackContext *)
                                       , createSessionCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&receiverMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_createSession rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    /***********************************************************************/
    /* Decide on the subscription options to use based on QoS and request. */
    /***********************************************************************/
    switch(ctxt.ginfo->QoS)
    {
        case 0:
            subAttributes.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
            break;
        case 1:
            subAttributes.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;

            if (  (ctxt.ginfo->txnCapable == TxnCapable_True)
                ||((ctxt.ginfo->txnCapable == TxnCapable_Random) && (rand()%10) > 5))
            {
                subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
            }
            break;
        case 2:
            subAttributes.subOptions = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
            break;
        default:
            TEST_ASSERT(false, ("ctxt.ginfo->QoS (%d) unexpected", ctxt.ginfo->QoS));
            break;
    }

    if(ctxt.ginfo->QoS > 0) {
        //Non-zero QoS - Create a durable subscription and open a consumer on it

        //We need to make a durable sub... and give it a subname
        char subname[64];
        sprintf(subname, "sub%d", ourThreadId);

        subAttributes.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;

        do {
            rc = ism_engine_createSubscription(
                    ctxt.hClient,
                    subname,
                    NULL,
                    ismDESTINATION_TOPIC,
                    ctxt.ginfo->topicString,
                    &subAttributes,
                    NULL, // Owning client same as requesting client
                    &pCtxt,
                    sizeof(callbackContext *),
                    SubberCreateSubscriptionCallback);

            if (rc == ISMRC_AsyncCompletion) {
                //The callback will unlock the mutex when done
                pthread_mutex_lock(&receiverMutex);
            } else {
                //Happened inline set the return code
                ctxt.retcode = rc;
            }
        } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

        if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
        {
            fprintf(stderr, "%d: ism_engine_createSubscription rc=%d\n", __LINE__, ctxt.retcode);
            ctxt.ginfo->groupRetCode = ctxt.retcode;
        }

        callbackContext *CBptr = &ctxt;

        do {
            rc = ism_engine_createConsumer(ctxt.hSession,
                                           ismDESTINATION_SUBSCRIPTION,
                                           subname,
                                           NULL,
                                           NULL, // Owning client same as session client
                                           &CBptr,
                                           sizeof(callbackContext *),
                                           SubberGotMessageCallback,
                                           NULL,
                                           test_getDefaultConsumerOptions(subAttributes.subOptions),
                                           &(ctxt.hConsumer),
                                           &pCtxt,
                                           sizeof(callbackContext *),
                                           SubberCreateConsumerCallback);

            if (rc == ISMRC_AsyncCompletion) {
                //The callback will unlock the mutex when done
                pthread_mutex_lock(&receiverMutex);
            } else {
                //Happened inline set the return code
                ctxt.retcode = rc;
            }
        } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

        if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
        {
            fprintf(stderr, "%d: ism_engine_createConsumer rc=%d\n", __LINE__, ctxt.retcode);
            ctxt.ginfo->groupRetCode = ctxt.retcode;
        }
    } else {
        //Create consumer (which under the covers will create a non-durable sub)

        callbackContext *CBptr = &ctxt;

        do {
            uint32_t consumerOptions = ((subAttributes.subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK) == ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE)
                                                   ? ismENGINE_CONSUMER_OPTION_NONE
                                                   : ismENGINE_CONSUMER_OPTION_ACK;

            rc = ism_engine_createConsumer(ctxt.hSession,
                                           ismDESTINATION_TOPIC,
                                           ctxt.ginfo->topicString,
                                           &subAttributes,
                                           NULL, // Owning client same as session client
                                           &CBptr,
                                           sizeof(callbackContext *),
                                           SubberGotMessageCallback,
                                           NULL,
                                           consumerOptions,
                                           &(ctxt.hConsumer),
                                           &pCtxt,
                                           sizeof(callbackContext *),
                                           SubberCreateConsumerCallback);

            if (rc == ISMRC_AsyncCompletion) {
                //The callback will unlock the mutex when done
                pthread_mutex_lock(&receiverMutex);
            } else {
                //Happened inline set the return code
                ctxt.retcode = rc;
            }
        } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

        if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
        {
            fprintf(stderr, "%d: ism_engine_createConsumer rc=%d\n", __LINE__, rc);
            ctxt.ginfo->groupRetCode = ctxt.retcode;
        }
    }

    do {
        rc = ism_engine_startMessageDelivery(ctxt.hSession, ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
    } while(memoryTestInProgress && (rc == ISMRC_AllocateError));

    threadWaitingForGroupStart(ctxt.ginfo);

    // If this subscriber group is expecting messages - wait for them
    if( ctxt.ginfo->msgsPerSubber > 0) {
        //Mutex will be unlocked by the rcvdMessageCB when we have rcvd
        //the correct number of messages
        pthread_mutex_lock(&receiverMutex);

        if (ctxt.ginfo->subsWait) {
            usleep(1000*20); //pause for 0.2s to check that no extra msgs
                             //appear which would cause the test to be a failure
        }
    } else {
        usleep(50*(rand()%1000)); // pause for up to 0.5 seconds to cause cleanup
                                  // of non-receiving subscribers to vary
    }

    do {
        rc = ism_engine_destroyConsumer(ctxt.hConsumer,
                                        &pCtxt,
                                        sizeof(callbackContext *),
                                        SubberDestroyConsumerCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&receiverMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_destroyConsumer rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    do {
        rc = ism_engine_destroySession(ctxt.hSession,
                                       &pCtxt,
                                       sizeof(callbackContext *),
                                       destroySessionCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&receiverMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_destroySession rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    do {
        rc = ism_engine_destroyClientState(ctxt.hClient,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           &pCtxt,
                                           sizeof(callbackContext *),
                                           destroyClientCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&receiverMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_destroyClient rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    pthread_mutex_unlock(&receiverMutex);

    threadfinished(ctxt.ginfo);

    ism_engine_threadTerm(1);
    ism_common_freeTLS();

    return NULL;
}

void *bgSubberController(void * args) {
    ourThreadInfo *tinfo = (ourThreadInfo *)args;
    pthread_mutex_t receiverMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutexattr_t mutexattrs;
    callbackContext ctxt;
    callbackContext *pCtxt = &ctxt;
    int32_t rc;
    char tname[20];

    sprintf(tname, "PSCFG-%d", __sync_add_and_fetch(&ThreadId, 1));
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    ism_engine_threadInit(0);
    uint32_t ourThreadId = tinfo->our_id;
    ctxt.optype = sub;
    ctxt.torelease = &receiverMutex;
    ctxt.ginfo = tinfo->ginfo;
    ctxt.hClient = NULL;  //We'll create it in a sec,,,,
    ctxt.hSession = NULL; //We'll create it in a sec...
    ctxt.hProducer = NULL; //unused
    ctxt.hConsumer = NULL; //We'll create it in a sec..
    ctxt.msgs = 0;
    ctxt.msgsSinceGroupUpdate = 0;

    //Finished copying data of threadinfo structure now...
    free(tinfo);

    threadWaitingForGroupSetup(ctxt.ginfo);

    /* Ensure that we have defined behaviour in the case of relocking a mutex */
    pthread_mutexattr_init(&mutexattrs);
    pthread_mutexattr_settype(&mutexattrs, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&receiverMutex, &mutexattrs);


    //Create a client
    pthread_mutex_lock(&receiverMutex);
    char clientid[64];
    sprintf(clientid, BGSUBBER_CLIENTID_FORMAT, ourThreadId);

    do {
        rc = ism_engine_createClientState(clientid,
                                          testDEFAULT_PROTOCOL_ID,
                                          (ctxt.ginfo->QoS == 0) ? ismENGINE_CREATE_CLIENT_OPTION_NONE:
                                                                   ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          NULL, NULL, NULL,
                                          &(ctxt.hClient),
                                          &pCtxt,
                                          sizeof(callbackContext *),
                                          createClientCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&receiverMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_createClientState rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    do {
        rc = ism_engine_createSession( ctxt.hClient
                                       , ismENGINE_CREATE_CLIENT_OPTION_NONE
                                       , &(ctxt.hSession)
                                       , &pCtxt
                                       , sizeof(callbackContext *)
                                       , createSessionCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&receiverMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_createSession rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    threadWaitingForGroupStart(ctxt.ginfo);

    int32_t numBgSubbers = 0;

    pthread_mutex_unlock(&receiverMutex);

    // Keep working until all pubber & subber threads have finished
    while (ctxt.ginfo->controller->threadsFinished <
           (ctxt.ginfo->controller->threadsControlled-ctxt.ginfo->controller->bgThreads))
    {
        usleep(ctxt.ginfo->bgSubberDelay);

        pthread_mutex_lock(&receiverMutex);
        callbackContext *CBptr = &ctxt;

        do {
            ismEngine_SubscriptionAttributes_t subAttributes = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
            rc = ism_engine_createConsumer(ctxt.hSession,
                                           ismDESTINATION_TOPIC,
                                           ctxt.ginfo->topicString,
                                           &subAttributes,
                                           NULL, // Owning client same as session client
                                           &CBptr,
                                           sizeof(callbackContext *),
                                           BgSubberGotMessageCallback,
                                           NULL,
                                           ismENGINE_CONSUMER_OPTION_NONE,
                                           &(ctxt.hConsumer),
                                           &pCtxt,
                                           sizeof(callbackContext *),
                                           SubberCreateConsumerCallback);

            if (rc == ISMRC_AsyncCompletion) {
                //The callback will unlock the mutex when done
                pthread_mutex_lock(&receiverMutex);
            } else {
                //Happened inline
                ctxt.retcode = rc;
            }
        } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

        if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK)) {
            fprintf(stderr, "%d: ism_engine_createConsumer rc=%d\n", __LINE__, ctxt.retcode);
                ctxt.ginfo->groupRetCode = ctxt.retcode;
        }

        numBgSubbers++;

        do {
            rc = ism_engine_startMessageDelivery(ctxt.hSession, ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
        } while(memoryTestInProgress && (rc == ISMRC_AllocateError));

        if (rc != OK)
        {
            ctxt.retcode = rc;
        }

        usleep(ctxt.ginfo->bgSubberDelay);

        if (ctxt.hConsumer != NULL) {
            do {
                rc =  ism_engine_destroyConsumer(ctxt.hConsumer,
                                                 &pCtxt,
                                                 sizeof(callbackContext *),
                                                 SubberDestroyConsumerCallback);

                if (rc == ISMRC_AsyncCompletion) {
                    //The callback will unlock the mutex when done
                    pthread_mutex_lock(&receiverMutex);
                } else {
                    //Happened inline, set the return code
                    ctxt.retcode = rc;
                }
            } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

            if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK)) {
                fprintf(stderr, "%d: ism_engine_destroyConsumer rc=%d\n", __LINE__, ctxt.retcode);
                ctxt.ginfo->groupRetCode = ctxt.retcode;
            }
        }

        pthread_mutex_unlock(&receiverMutex);
    }

    if (ctxt.msgsSinceGroupUpdate)
    {
        //Update the number of messages for the group
        __sync_fetch_and_add(&(ctxt.ginfo->groupBgMsgsRcvd),
                             ctxt.msgsSinceGroupUpdate);
    }

    __sync_fetch_and_add(&(ctxt.ginfo->groupBgSubbersCreated),
                         numBgSubbers);

    pthread_mutex_lock(&receiverMutex);

    do {
        rc = ism_engine_destroySession(ctxt.hSession,
                                       &pCtxt,
                                       sizeof(callbackContext *),
                                       destroySessionCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&receiverMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_destroySession rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    do {
        rc = ism_engine_destroyClientState(ctxt.hClient,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           &pCtxt,
                                           sizeof(callbackContext *),
                                           destroyClientCallback);

        if (rc == ISMRC_AsyncCompletion) {
            //The callback will unlock the mutex when done
            pthread_mutex_lock(&receiverMutex);
        } else {
            //Happened inline, set the return code
            ctxt.retcode = rc;
        }
    } while(memoryTestInProgress && (ctxt.retcode == ISMRC_AllocateError));

    if ((ctxt.retcode != OK) && (ctxt.ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: ism_engine_destroyClient rc=%d\n", __LINE__, ctxt.retcode);
        ctxt.ginfo->groupRetCode = ctxt.retcode;
    }

    pthread_mutex_unlock(&receiverMutex);

    threadfinished(ctxt.ginfo);

    ism_engine_threadTerm(1);
    ism_common_freeTLS();

    return NULL;
}

int InitPubSubThreadGroup(psgroupinfo *psginfo)
{
    int rc = OK;
    int i;
    pthread_attr_t threadattr;

    /*Say we aren't going to join to the threads... maybe we ought to?*/
    pthread_attr_init  (&threadattr);
    pthread_attr_setdetachstate  (&threadattr,  PTHREAD_CREATE_DETACHED);

    if (psginfo->numSubbers != 0)
    {
        ourThreadInfo *getthreadinfo[psginfo->numSubbers];

        for(i=0; i < psginfo->numSubbers; i++) {
            getthreadinfo[i] = malloc(sizeof(ourThreadInfo));

            TEST_ASSERT(getthreadinfo[i] != NULL, ("getthreadinfo[%d] == NULL", i));

            getthreadinfo[i]->our_id = __sync_add_and_fetch(&globalThreadId, 1);
            getthreadinfo[i]->ginfo = psginfo;

            rc = test_task_startThread(&(getthreadinfo[i]->pthread_id),subberController, (void *)(getthreadinfo[i]),"subberController");

            if (rc != OK) {
                printf("ERROR:  Starting Getter thread returned %d\n", rc);
                return rc;
            }
        }
    }

    if (psginfo->numPubbers != 0)
    {
        ourThreadInfo *putthreadinfo[psginfo->numPubbers];
        for(i=0; i <psginfo->numPubbers; i++) {
            putthreadinfo[i] = malloc(sizeof(ourThreadInfo));

            TEST_ASSERT(putthreadinfo[i] != NULL, ("putthreadinfo[%d] == NULL", i));

            putthreadinfo[i]->our_id = __sync_add_and_fetch(&globalThreadId, 1);
            putthreadinfo[i]->ginfo = psginfo;

            rc = test_task_startThread(&(putthreadinfo[i]->pthread_id),pubberController, (void *)(putthreadinfo[i]),"pubberController");

            if (rc != OK) {
                printf("ERROR:  Starting Putter thread returned %d\n", rc);
                return rc;
            }
        }
    }

    if (psginfo->numBgSubThreads != 0)
    {
        ourThreadInfo *bgsubthreadinfo[psginfo->numBgSubThreads];

        for(i=0; i<psginfo->numBgSubThreads; i++) {
            if (psginfo->bgSubberDelay > -1) {
                bgsubthreadinfo[i] = malloc(sizeof(ourThreadInfo));

                TEST_ASSERT(bgsubthreadinfo[i] != NULL, ("bgsubthreadinfo[%d] == NULL", i));

                bgsubthreadinfo[i]->our_id = __sync_add_and_fetch(&globalThreadId, 1);
                bgsubthreadinfo[i]->ginfo = psginfo;

                rc = test_task_startThread(&(bgsubthreadinfo[i]->pthread_id),bgSubberController, (void *)(bgsubthreadinfo[i]),"bgSubberController");

                if (rc != OK) {
                    printf("ERROR: Starting Background Subscription thread returned %d\n", rc);
                    return rc;
                }

                psginfo->controller->bgThreads += 1;
            }
        }
    }

    pthread_attr_destroy  (&threadattr);
    return rc;
}

int WaitForGroupsFinish(psgroupcontroller *gcontroller,
                        ism_time_t *finishTime,
                        int numGroups,
                        psgroupinfo *ginfo[])
{
    int rc;
    bool firstloop = true;
    int i;

    //wait for the threads to be ready
    rc = pthread_mutex_lock(&(gcontroller->mutex_finish));

    while ((rc == OK) &&
            (  gcontroller->threadsFinished
             < gcontroller->threadsControlled)) {
        struct timespec   now;
        struct timespec   waituntil;

        // printf("threads finished: %d threads controlled: %d\n", gcontroller->threadsFinished, gcontroller->threadsControlled);

        if(firstloop) {
            //We don't print status on first loop to give time for pubs to flow
            firstloop = false;
        } else {
            __sync_synchronize();

            for(i = 0; i < numGroups; i++) {
                if (((ginfo[i]->numPubbers * ginfo[i]->msgsPerPubber) != ginfo[i]->groupMsgsSent) ||
                    ((ginfo[i]->numSubbers * ginfo[i]->msgsPerSubber) != ginfo[i]->groupMsgsRcvd) ||
                    (ginfo[i]->numBgSubThreads != 0)) {
                    printf("topic %s (%d P %d S %d B): rc: %d sent: %ld rvcd: %ld brcv: %ld\n"
                            , ginfo[i]->topicString
                            , ginfo[i]->numPubbers
                            , ginfo[i]->numSubbers
                            , ginfo[i]->numBgSubThreads
                            , ginfo[i]->groupRetCode
                            , ginfo[i]->groupMsgsSent
                            , ginfo[i]->groupMsgsRcvd
                            , ginfo[i]->groupBgMsgsRcvd);
                }
            }
        }

        rc = clock_gettime(PSGROUPCONTROLLER_CONDCLOCK, &now);
        TEST_ASSERT(rc == OK, ("clock_gettime rc (%d) != OK", rc));

        waituntil = now;
        waituntil.tv_sec +=1;

        rc = pthread_cond_timedwait(&(gcontroller->cond_finish)
                                   ,&(gcontroller->mutex_finish)
                                   ,&waituntil);

        if (rc == ETIMEDOUT) {
            if (gcontroller->monitorTopic != NULL) {
                ismEngine_TopicMonitor_t *topicMonitorResults = NULL;
                uint32_t resultCount = 0;

                rc = ism_engine_getTopicMonitor(&topicMonitorResults,
                                                &resultCount,
                                                ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS,
                                                50,
                                                NULL);
                printf("monitorStats     rc: %d results: %d\n", rc, resultCount);

                if (rc == OK) {
                    for(i=0; i<resultCount; i++) {
                        printf(" - Topic: \"%s\" Published: %-6lu RejectedMsgs: %-6lu FailedPublishes: %-6lu Subscriptions: %-6lu\n",
                               topicMonitorResults[i].topicString,
                               topicMonitorResults[i].stats.PublishedMsgs,
                               topicMonitorResults[i].stats.RejectedMsgs,
                               topicMonitorResults[i].stats.FailedPublishes,
                               topicMonitorResults[i].subscriptions);
                    }
                }

                ism_engine_freeTopicMonitor(topicMonitorResults);

                ismEngine_SubscriptionMonitor_t *subscriptionMonitorResults = NULL;

                rc = ism_engine_getSubscriptionMonitor(&subscriptionMonitorResults,
                                                       &resultCount,
                                                       ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                                       50,
                                                       NULL);

                printf("subsciptionStats rc: %d results: %d (showing highest one)\n", rc, resultCount);

                if (rc == OK && resultCount > 0) {
                    printf(" - SubName: \"%s\" Topic: \"%s\" CId: \"%s\" [%s]"
                               "ProdMsgs: %-6lu ConsMsgs: %-6lu Buff: %-6lu BuffHWM: %-6lu Rej: %-6lu Dis: %-6lu Exp: %-6lu BuffPct: %3.2f%%\n",
                           subscriptionMonitorResults[0].subName ? subscriptionMonitorResults[0].subName : "<NONE>",
                           subscriptionMonitorResults[0].topicString,
                           subscriptionMonitorResults[0].clientId ? subscriptionMonitorResults[0].clientId : "<NONE>",
                           subscriptionMonitorResults[0].durable ? "Durable" : "Nondurable",
                           subscriptionMonitorResults[0].stats.ProducedMsgs,
                           subscriptionMonitorResults[0].stats.ConsumedMsgs,
                           subscriptionMonitorResults[0].stats.BufferedMsgs,
                           subscriptionMonitorResults[0].stats.BufferedMsgsHWM,
                           subscriptionMonitorResults[0].stats.RejectedMsgs,
                           subscriptionMonitorResults[0].stats.DiscardedMsgs,
                           subscriptionMonitorResults[0].stats.ExpiredMsgs,
                           subscriptionMonitorResults[0].stats.BufferedPercent);
                }

                ism_engine_freeSubscriptionMonitor(subscriptionMonitorResults);
            }

            rc = OK; //timing out is fine...
        }
        TEST_ASSERT(rc == OK, ("Timedwait rc (%d) != OK", rc));
    }

    __sync_synchronize();

    if (finishTime) {
        *finishTime = ism_common_currentTimeNanos();
    }

    for(i = 0; i < numGroups; i++) {
        printf("topic %s (%d P %d S %d B): rc: %d sent: %ld rvcd: %ld brcv: %ld\n"
                , ginfo[i]->topicString
                , ginfo[i]->numPubbers
                , ginfo[i]->numSubbers
                , ginfo[i]->numBgSubThreads
                , ginfo[i]->groupRetCode
                , ginfo[i]->groupMsgsSent
                , ginfo[i]->groupMsgsRcvd
                , ginfo[i]->groupBgMsgsRcvd);
    }

    int rc2 = pthread_mutex_unlock(&(gcontroller->mutex_finish));

    if ((rc2 != OK)&&(rc == OK)) {
        printf("Eeek! pthread_mutex_unlock rc=%d\n", rc2);
        rc = rc2;
    }

    if (rc == OK) {
        for(i = 0; (rc == OK) && (i < numGroups); i++) {
            rc = ginfo[i]->groupRetCode;

            if (rc != 0) printf("Eeek! ThreadGroup %d of %d  rc=%d\n", i, numGroups, rc);
        }
    }

    return rc;
}

void FreeGroups(psgroupinfo *groups, int groupCount)
{
    for(int i=0; i<groupCount; i++)
    {
        if (groups[i].topicString != NULL) free(groups[i].topicString);
    }

    free(groups);
}

void initControllerConds(psgroupcontroller *pGController)
{
    pthread_condattr_t attr;
    DEBUG_ONLY int os_rc = pthread_condattr_init(&attr);
    assert(os_rc == 0);
    os_rc = pthread_condattr_setclock(&attr, PSGROUPCONTROLLER_CONDCLOCK);
    assert(os_rc == 0);
    os_rc = pthread_cond_init(&(pGController->cond_setup), &attr);
    assert(os_rc == 0);
    os_rc = pthread_cond_init(&(pGController->cond_start), &attr);
    assert(os_rc == 0);
    os_rc = pthread_cond_init(&(pGController->cond_finish), &attr);
    assert(os_rc == 0);
    os_rc = pthread_condattr_destroy(&attr);
    assert(os_rc == 0);
}

int32_t doPubs(char *topicstring,
               int numpublishers,
               int msgsperpubber,
               int numsubscribers,
               bool subsWait,
               bool cpupin,
               bool memtest,
               transactionCapableQoS1 transactionCapable,
               int qos)
{
    int32_t rc = OK;
    ism_time_t setupTime;
    ism_time_t startTime;
    ism_time_t finishTime;
    bool MemTestStarted = false;

    psgroupcontroller gcontroller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo ginfo = PSGROUPDEFAULT;

    initControllerConds(&gcontroller);

    ginfo.topicString   = topicstring;
    ginfo.numPubbers    = numpublishers;
    ginfo.msgsPerPubber = msgsperpubber;
    ginfo.numSubbers    = numsubscribers;
    ginfo.msgsPerSubber = numpublishers * msgsperpubber;
    ginfo.controller    = &gcontroller;
    ginfo.subsWait      = subsWait;
    ginfo.cpuPin        = cpupin;
    ginfo.QoS           = qos;
    ginfo.txnCapable    = transactionCapable;
    if (qos > 0)
    {
        ginfo.pubMsgPersistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    }


    gcontroller.threadsControlled = numpublishers + numsubscribers;


    rc = InitPubSubThreadGroup(&ginfo);

    if ( rc == OK ) {
            rc = TriggerSetupThreadGroups(&gcontroller,
                                          &setupTime,
                                          memtest);
            if (memtest) {
                MemTestStarted = true;
            }
    }
    if ( rc == OK ) {
        rc = StartThreadGroups(&gcontroller,
                               &startTime);
    }

    if ( rc == OK ) {
        psgroupinfo *groupArray[1];

        groupArray[0] = &ginfo;
        rc = WaitForGroupsFinish(&gcontroller,
                                 &finishTime,
                                 1, groupArray);
//        rc = WaitForGroupsFinish(&gcontroller,
//                                 &finishTime, 0, NULL);
    }

    if (rc != 0) printf("WaitForGroupsFinish rc=%d\n", rc);

    if (MemTestStarted) {
        stopMemoryTest();
    }

    return rc;
}

void createClientCallback(int32_t retcode, void *handle, void *pContext)
{
    callbackContext *ctxt = *(callbackContext **)pContext;
    ctxt->retcode = retcode;
    if (retcode == OK) {
        ctxt->hClient = handle;
    }
    pthread_mutex_unlock(ctxt->torelease);
}

void destroyClientCallback(int32_t retcode, void *handle, void *pContext)
{
    callbackContext *ctxt = *(callbackContext **)pContext;
    ctxt->retcode = retcode;
    if (retcode == OK) {
        ctxt->hClient = handle;
    }
    pthread_mutex_unlock(ctxt->torelease);
}

void createSessionCallback(int32_t retcode, void *handle, void *pContext)
{
    callbackContext *ctxt = *(callbackContext **)pContext;
    ctxt->retcode = retcode;
    if (retcode == OK) {
        ctxt->hSession = handle;
    }
    pthread_mutex_unlock(ctxt->torelease);
}

void destroySessionCallback(int32_t retcode, void *handle, void *pContext)
{
    callbackContext *ctxt = *(callbackContext **)pContext;
    ctxt->retcode = retcode;
    if (retcode == OK) {
        ctxt->hSession = handle;
    }
    pthread_mutex_unlock(ctxt->torelease);
}


void PubberCreateProducerCallback(int32_t retcode, void *handle, void *pContext)
{
    callbackContext *ctxt = *(callbackContext **)pContext;
    ctxt->retcode = retcode;

    TEST_ASSERT(ctxt->optype == pub, ("ctxt->optype (%d) != pub (%d)", ctxt->optype, pub));

    if (retcode == OK) {
        ctxt->hProducer = handle;
    }
    pthread_mutex_unlock(ctxt->torelease);
}

void PubberDestroyProducerCallback(int32_t retcode, void *handle, void *pContext)
{
    callbackContext *ctxt = *(callbackContext **)pContext;
    ctxt->retcode = retcode;

    TEST_ASSERT(ctxt->optype == pub, ("ctxt->optype (%d) != pub (%d)", ctxt->optype, pub));

    if (retcode == OK) {
        ctxt->hProducer = handle;
    }
    pthread_mutex_unlock(ctxt->torelease);
}

void PubberPutMessageCallback(int32_t retcode, void *handle, void *pContext)
{
    callbackContext *ctxt = *(callbackContext **)pContext;
    ctxt->retcode = retcode;

    TEST_ASSERT(ctxt->optype == pub, ("ctxt->optype (%d) != pub (%d)", ctxt->optype, pub));

    // Informational retcodes -- just treat as 'OK'.
    if (retcode == ISMRC_NoMatchingDestinations || retcode == ISMRC_NoMatchingLocalDestinations)
    {
        retcode = OK;
    }

    if (retcode != OK)
    {
        fprintf(stderr, "%d: PubberPutMessageCallback rc=%d\n", __LINE__, retcode);
        ctxt->ginfo->groupRetCode = retcode;
        TEST_ASSERT(false, ("retcode (%d) != 0", retcode));
    }
    if(retcode == OK) {
        //Update the per-thread number of messages
        int msgs = ASYNCPUT_CB_ADD_AND_FETCH(&(ctxt->msgs), 1);
        int msgsSinceGroupUpdate = ASYNCPUT_CB_ADD_AND_FETCH(&(ctxt->msgsSinceGroupUpdate), 1);

        if(msgs == ctxt->ginfo->msgsPerPubber) {
            //Update the number of messages for the group
            __sync_fetch_and_add(&(ctxt->ginfo->groupMsgsSent),
                                 msgsSinceGroupUpdate);
            ASYNCPUT_CB_SUB_AND_FETCH(&(ctxt->msgsSinceGroupUpdate),msgsSinceGroupUpdate);

            //unlock the mutex to tell the pubber thread to finish
            pthread_mutex_unlock(ctxt->torelease);
        } else if(ctxt->msgsSinceGroupUpdate > 10000) {
            //Update the number of messages for the group
            __sync_fetch_and_add(&(ctxt->ginfo->groupMsgsSent),
                                 msgsSinceGroupUpdate);
            ASYNCPUT_CB_SUB_AND_FETCH(&(ctxt->msgsSinceGroupUpdate),msgsSinceGroupUpdate);
        }
    }
    if ((retcode != OK) && (ctxt->ginfo->groupRetCode == OK))
    {
        fprintf(stderr, "%d: PubberPutMessageCallback rc=%d\n", __LINE__, retcode);
        ctxt->ginfo->groupRetCode = retcode;
    }
}

void SubberCreateSubscriptionCallback(int32_t retcode, void *handle, void *pContext)
{
    callbackContext *ctxt = *(callbackContext **)pContext;
    ctxt->retcode = retcode;

    TEST_ASSERT(ctxt->optype == sub, ("ctxt->optype (%d) != sub (%d)", ctxt->optype, sub));

    pthread_mutex_unlock(ctxt->torelease);
}

void SubberCreateConsumerCallback(int32_t retcode, void *handle, void *pContext)
{
    callbackContext *ctxt = *(callbackContext **)pContext;
    ctxt->retcode = retcode;

    TEST_ASSERT(ctxt->optype == sub, ("ctxt->optype (%d) != sub (%d)", ctxt->optype, sub));

    if (retcode == OK) {
        ctxt->hConsumer = handle;
    }
    pthread_mutex_unlock(ctxt->torelease);
}

void SubberDestroyConsumerCallback(int32_t retcode, void *handle, void *pContext)
{
    callbackContext *ctxt = *(callbackContext **)pContext;
    ctxt->retcode = retcode;

    TEST_ASSERT(ctxt->optype == sub, ("ctxt->optype (%d) != sub (%d)", ctxt->optype, sub));

    if (retcode == OK) {
        ctxt->hConsumer = handle;
    }
    pthread_mutex_unlock(ctxt->torelease);
}

bool SubberGotMessageCallback(
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
        void *                          pConsumerContext,
        ismEngine_DelivererContext_t *  _delivererContext)
{
    callbackContext *ctxt = *((callbackContext **)pConsumerContext);

    TEST_ASSERT(ctxt->optype == sub, ("ctxt->optype (%d) != sub (%d)", ctxt->optype, sub));

    ism_engine_releaseMessage(hMessage);
    if (state != ismMESSAGE_STATE_CONSUMED) {
        int32_t rc = ism_engine_confirmMessageDelivery(ctxt->hSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               NULL, 0, NULL);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion,
                            ("rc was(%d)", rc));
    }

    //Update the per-thread number of messages
    ctxt->msgs++;
    ctxt->msgsSinceGroupUpdate++;

    if(ctxt->msgs == ctxt->ginfo->msgsPerSubber) {
        //Update the number of messages for the group
        __sync_fetch_and_add(&(ctxt->ginfo->groupMsgsRcvd),
                             ctxt->msgsSinceGroupUpdate);
        ctxt->msgsSinceGroupUpdate = 0;

        //unlock the mutex to tell the subber thread to finish
        pthread_mutex_unlock(ctxt->torelease);
    } else if(ctxt->msgsSinceGroupUpdate > 10000) {
        //Update the number of messages for the group
        __sync_fetch_and_add(&(ctxt->ginfo->groupMsgsRcvd),
                             ctxt->msgsSinceGroupUpdate);
        ctxt->msgsSinceGroupUpdate = 0;
    }

    if(ctxt->msgs > ctxt->ginfo->msgsPerSubber) {
        printf("Received %d messages at topic %s (expected: %ld)\n",
               ctxt->msgs,
               ctxt->ginfo->topicString,
               ctxt->ginfo->msgsPerSubber );
        fflush(stdout);
        TEST_ASSERT(false,
                    ("ctxt->msgs (%d) > ctxt->ginfo->msgsPerSubber (%d)", ctxt->msgs, ctxt->ginfo->msgsPerSubber));
    }

    return true; //We'd like more messages
}

bool BgSubberGotMessageCallback(
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
        void *                          pConsumerContext,
        ismEngine_DelivererContext_t *  _delivererContext )
{
    callbackContext *ctxt = *((callbackContext **)pConsumerContext);

    TEST_ASSERT(ctxt->optype == sub, ("ctxt->optype (%d) != sub (%d)", ctxt->optype, sub));

    ism_engine_releaseMessage(hMessage);

    //Update the per-thread number of messages
    ctxt->msgs++;
    ctxt->msgsSinceGroupUpdate++;

    if (ctxt->msgsSinceGroupUpdate > 1000) {
        //Update the number of messages for the group
        __sync_fetch_and_add(&(ctxt->ginfo->groupBgMsgsRcvd),
                             ctxt->msgsSinceGroupUpdate);
        ctxt->msgsSinceGroupUpdate = 0;
    }

    return true; //We'd like more messages
}

#ifndef NO_MALLOC_WRAPPER

//Returns a new memory state - 40% chance (states: Plentiful & reduce) that all mallocs will succeed
//                             20% chance that just message bodies will fail (state = disable early)
//                             20% chance that most mallocs will fail
//                             20% chance that all mallocs will fail
iememMemoryLevel_t nextMemoryState(iememMemoryLevel_t currState)
{
    iememMemoryLevel_t newState;

    int randnum = rand() % 5;

    newState = (iememMemoryLevel_t) randnum;

    return newState;
}
void *memoryTestThread(void *args)
{
    char tname[20];
    static iememMemoryLevel_t currState = iememPlentifulMemory;

    sprintf(tname, "PSCFG-%d", __sync_add_and_fetch(&ThreadId, 1));
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    // This thread just happens to not require store resources, so let's
    // initialize it that way just to go through that code (ordinarily only
    // special store threads should do this).
    ism_engine_threadInit(ISM_ENGINE_NO_STORE_STREAM);

    // Make sure we didn't get a store stream
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    if (pThreadData->hStream != ismSTORE_NULL_HANDLE)
    {
        printf("Store stream opened unexpectedly 0x%016x\n", pThreadData->hStream);
        exit(1);
    }

    while(!memoryTestThreadsShouldEnd)
    {
        currState = nextMemoryState(currState);
        iemem_setMallocState(currState);

        usleep(10*1000); //sleep for 10 milliseconds = 0.01s
    }

    //When we exit, make sure mallocs work...
    iemem_setMallocState(iememPlentifulMemory);

    ism_engine_threadTerm(1);
    ism_common_freeTLS();

    return NULL;
}


typedef struct tag_consumerListNode_t {
    struct tag_consumerListNode_t *next;
    ismEngine_ConsumerHandle_t hConsumer;
} consumerListNode_t;

typedef struct tag_brokenConsumerList_t {
    consumerListNode_t *firstConsumer;   ///< Head pointer for list of received messages
    consumerListNode_t *lastConsumer;   ///< Tail pointer for list of received messages
    pthread_mutex_t listMutex; ///< Used to protect the received message list
    pthread_cond_t listCond;  ///< This used to broadcast when there are messages in the list to process
} brokenConsumerList_t;

static brokenConsumerList_t brokenConsumers = {0};

//don't want to start the memoryTest until the consumerRe-enabler thread has done a threadinit
bool consumerReenablerReady = false;

void *consumerReenablerThread(void *arg)
{
    char tname[20];
    sprintf(tname, "cRT-%u", __sync_add_and_fetch(&ThreadId, 1));
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    int32_t rc = OK;

    ism_engine_threadInit(0);
    
    consumerReenablerReady = true;

    while(!memoryTestThreadsShouldEnd)
    {
        ismEngine_ConsumerHandle_t hConsumerToReenable = NULL;

        //Wait to be told there's something to do...
        int os_rc = pthread_mutex_lock(&(brokenConsumers.listMutex));
        TEST_ASSERT(os_rc == 0 , ("os_rc (%d) != 0", os_rc));

        if (brokenConsumers.firstConsumer == NULL)
        {
            os_rc = pthread_cond_wait(&(brokenConsumers.listCond), &(brokenConsumers.listMutex));
            TEST_ASSERT(os_rc == 0 , ("os_rc (%d) != 0", os_rc));
        }

        if(brokenConsumers.firstConsumer != NULL)
        {
            hConsumerToReenable = brokenConsumers.firstConsumer->hConsumer;

            if(brokenConsumers.lastConsumer == brokenConsumers.firstConsumer)
            {
                TEST_ASSERT(brokenConsumers.firstConsumer->next == NULL, ("brokenConsumers.firstConsumer->next was %p", brokenConsumers.firstConsumer->next));
                brokenConsumers.lastConsumer = NULL;
            }
            consumerListNode_t *oldNode = brokenConsumers.firstConsumer;
            brokenConsumers.firstConsumer = brokenConsumers.firstConsumer->next;
            free(oldNode);
        }

        os_rc = pthread_mutex_unlock(&(brokenConsumers.listMutex));
        TEST_ASSERT(os_rc == 0 , ("os_rc (%d) != 0", os_rc));

        if (hConsumerToReenable != NULL)
        {
            //Wait for our selected consumer to be disabled
            ismMessageDeliveryStatus_t waiterStatus;

            do
            {
                ism_engine_getConsumerMessageDeliveryStatus(hConsumerToReenable, &waiterStatus);
            }
            while (waiterStatus != ismMESSAGE_DELIVERY_STATUS_STOPPED);

            //reenable it - allocation errors are from checkwaiters and should be ignored
            rc = ism_engine_resumeMessageDelivery(
                        hConsumerToReenable,
                        ismENGINE_RESUME_DELIVERY_OPTION_NONE,
                        NULL, 0, NULL);


            TEST_ASSERT(((rc == OK)||(rc == ISMRC_AllocateError)), ("rc (%d) != OK or alloc", rc));
        }
    }

    ism_engine_threadTerm(1);
    ism_common_freeTLS();

    return NULL;
}

void deliveryFailureCallback(
        int32_t                       rc,
        ismEngine_ClientStateHandle_t hClient,
        ismEngine_ConsumerHandle_t    hConsumer,
        void                         *consumerContext)
{
    TEST_ASSERT(rc == ISMRC_AllocateError , ("rc (%d) != ismRC_AllocateError", rc));
    TEST_ASSERT(memoryTestInProgress == true, ("memoryTestInProgress (%d) != true", memoryTestInProgress));

    callbackContext *ctxt = *((callbackContext **)consumerContext);

    TEST_ASSERT(ctxt->optype == sub, ("ctxt->optype (%d) != sub (%d)", ctxt->optype, sub));

    if(ctxt->msgs < ctxt->ginfo->msgsPerSubber)
    {
        //Ask the consumer reenabler thread to reenable this consumer......
        int32_t os_rc = pthread_mutex_lock(&(brokenConsumers.listMutex));
        TEST_ASSERT(os_rc == 0 , ("os_rc (%d) != 0", os_rc));

        consumerListNode_t *newNode = malloc(sizeof(consumerListNode_t));
        TEST_ASSERT(newNode != NULL , ("newNode was null"));

        newNode->next = NULL;
        newNode->hConsumer = hConsumer;

        if(brokenConsumers.firstConsumer == NULL)
        {
            brokenConsumers.firstConsumer = newNode;
            TEST_ASSERT(brokenConsumers.lastConsumer == NULL, ("last consumer was non NULL: %p", brokenConsumers.lastConsumer));
            brokenConsumers.lastConsumer = newNode;
        }
        else
        {
            TEST_ASSERT(brokenConsumers.lastConsumer != NULL, ("last consumer was NULL"));
            brokenConsumers.lastConsumer->next = newNode;
            brokenConsumers.lastConsumer       = newNode;
        }

        //Broadcast that we've added some messages to the list
        os_rc = pthread_cond_broadcast(&(brokenConsumers.listCond));
        TEST_ASSERT(os_rc == 0 , ("os_rc (%d) != 0", os_rc));


        //Let the processing thread access the list of messages...
        os_rc = pthread_mutex_unlock(&(brokenConsumers.listMutex));
        TEST_ASSERT(os_rc == 0 , ("os_rc (%d) != 0", os_rc));
    }
}

void startMemoryTest(void)
{
    TEST_ASSERT(memoryTestInProgress == false, ("memoryTestInProgress (%d) != false", memoryTestInProgress));

    memoryTestThreadsShouldEnd = false;
    consumerReenablerReady = false;

    DEBUG_ONLY int rc;
    rc = test_task_startThread(&consumerReenablerTheadId,consumerReenablerThread, NULL,"consumerReenablerThread");
    TEST_ASSERT(rc == 0, ("rc (%d) != 0", rc));

    while(!consumerReenablerReady)
    {
        usleep(100);
        ismEngine_FullMemoryBarrier();
    }

    rc = test_task_startThread(&memoryTestThreadId,memoryTestThread, NULL,"memoryTestThread");
    TEST_ASSERT(rc == 0, ("rc (%d) != 0", rc));



    ism_engine_registerDeliveryFailureCallback(deliveryFailureCallback);

    memoryTestInProgress = true;
}

void stopMemoryTest(void)
{
    TEST_ASSERT(memoryTestInProgress == true, ("memoryTestInProgress (%d) != true", memoryTestInProgress));

    memoryTestThreadsShouldEnd = true;

    //signal the consumerReenabler thread so it notices that it should end
    int32_t os_rc = pthread_mutex_lock(&(brokenConsumers.listMutex));
    TEST_ASSERT(os_rc == 0 , ("os_rc (%d) != 0", os_rc));
    os_rc = pthread_cond_broadcast(&(brokenConsumers.listCond));
    TEST_ASSERT(os_rc == 0 , ("os_rc (%d) != 0", os_rc));
    os_rc = pthread_mutex_unlock(&(brokenConsumers.listMutex));
    TEST_ASSERT(os_rc == 0 , ("os_rc (%d) != 0", os_rc));

    //Wait for both threads to end
    pthread_join(memoryTestThreadId, NULL);
    pthread_join(consumerReenablerTheadId, NULL);

    memoryTestInProgress = false;
}
#else
void startMemoryTest(void)
{
    printf("Malloc wrapper disabled, unable to start memory test\n");
}

void stopMemoryTest(void)
{
    printf("Malloc wrapper disabled, unable to stop memory test\n");
}
#endif
