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
/* Module Name: testPubSubRestart                                    */
/*                                                                   */
/* Description: UnitTest functions for testing pub/sub restart       */
/*                                                                   */
/*********************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "engine.h"
#include "engineInternal.h"
#include "policyInfo.h"
#include "test_utils_log.h"
#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_task.h"

#include "testPubSubRestart.h"
#include "resourceSetStats.h"

//At the moment we only record how many messages to send etc. in the publishers
//thread... so we can't restart until all the threads have started up else we don't know what
//we trying to do....
static uint32_t PublishersPersisted = 0;

static uint64_t CallbacksInProgress = 0;

testMessage_t * parseReceivedMessage(ismMessageHeader_t    *pMsgDetails,
                                     uint8_t                areaCount,
                                     ismMessageAreaType_t   areaTypes[areaCount],
                                     size_t                 areaLengths[areaCount],
                                     void *                 pAreaData[areaCount])
{
    TEST_ASSERT(areaCount == 2, ("areaCount (%d) != 2", areaCount));
    TEST_ASSERT(areaTypes[0] == ismMESSAGE_AREA_PROPERTIES,
                ("areaTypes[0] (%d) != ismMESSAGE_AREA_PROPERTIES (%d)", areaTypes[0], ismMESSAGE_AREA_PROPERTIES));
    TEST_ASSERT(areaTypes[1] == ismMESSAGE_AREA_PAYLOAD,
                ("areaTypes[1] (%d) != ismMESSAGE_AREA_PAYLOAD (%d)", areaTypes[1], ismMESSAGE_AREA_PAYLOAD));
    TEST_ASSERT(areaLengths[1] == sizeof(testMessage_t),
                ("areaLengths[1] (%d) != sizeof(testMessage_t) (%d)", (int)areaLengths[1], (int)sizeof(testMessage_t)));

    testMessage_t *msgPayload = (testMessage_t *)pAreaData[1];

    ismEngine_CheckStructId(msgPayload->StructId, TESTMSG_STRUCTID, 1);

    return msgPayload;
}

recvdMsgStatus_t recordDeliveryId( subscriberStateComplete_t *subState
                                 , uint32_t deliveryId
                                 , testMessage_t *msgPayload)
{
    uint32_t emptySlot = TEST_MAXINFLIGHT;
    bool loopAgain = false;
    uint32_t loops = 0;

    //Look to see if that deliveryId is already in our records (and if there's an empty slot)
    do
    {
        loopAgain = false;

        for (uint32_t im = 0; im < TEST_MAXINFLIGHT; im++)
        {
            if (subState->persState->msgsInFlight[im].deliveryId == deliveryId)
            {
                if (   (subState->persState->msgsInFlight[im].msg.msgNum    == msgPayload->msgNum)
                     &&(subState->persState->msgsInFlight[im].msg.pubberNum == msgPayload->pubberNum))
                {
                    return rmsSEENBEFORE;
                }
                else
                {
                    printf("Sub %s has deliveryId %u illegally reused.\n", subState->persState->clientId,
                                                                           deliveryId);
                    printf("Message inflight record: Pubber %u, Msg %u.\n",
                            subState->persState->msgsInFlight[im].msg.pubberNum,
                            subState->persState->msgsInFlight[im].msg.msgNum);
                    printf("Just received representing: Pubber %u, Msg %u.\n",
                            msgPayload->pubberNum, msgPayload->msgNum);
                    TEST_ASSERT(false, ("DeliveryId illegally reused"));
                }
            }
            else if (subState->persState->msgsInFlight[im].deliveryId == 0)
            {
                if (emptySlot == TEST_MAXINFLIGHT)
                {
                    emptySlot = im;
                }
            }
        }

        if (emptySlot == TEST_MAXINFLIGHT)
        {
            //No empty slots... need to process some inflight messages before storing this
            //new one...
            //We'll just wait for messages already in flight to be processed...
            loops++;

            if (loops < 60000)
            {
                usleep(1000);
                loopAgain = true;
            }
        }
    }
    while(loopAgain);

    TEST_ASSERT(emptySlot < TEST_MAXINFLIGHT,
                ("emptySlot (%d) >= TEST_MAXINFLIGHT (%d)", emptySlot, TEST_MAXINFLIGHT));

    //Record the other details before storing deliveryId as that makes the record "live"
    subState->persState->msgsInFlight[emptySlot].msg = *msgPayload;
    ismEngine_WriteMemoryBarrier();
    subState->persState->msgsInFlight[emptySlot].deliveryId = deliveryId;

    return rmsFIRSTSEEN;
}


void removeDeliveryId(subscriberStateComplete_t *subState
                     , uint32_t deliveryId)
{
    for (uint32_t im = 0; im < TEST_MAXINFLIGHT; im++)
    {
        if (subState->persState->msgsInFlight[im].deliveryId == deliveryId)
        {
            subState->persState->msgsInFlight[im].deliveryId = 0;
            return;
        }
    }

    TEST_ASSERT(false,
                ("Unable to remove deliveryid %u for client %s...not found!",
                deliveryId, subState->persState->clientId));
}

///@returns true if we've received all expected messages from this publisher.
bool updateSubStateForPubber( ismEngine_ConsumerHandle_t hConsumer
                            , subscriberStateComplete_t *subState
                            , recvdMsgStatus_t firstseen
                            , testMessage_t * msgPayload)
{
    subInfoPerPublisher_t *pubInfo = &(subState->persState->pubState[msgPayload->pubberNum]);

    if((pubInfo->msgsArrived + 1) == msgPayload->msgNum)
    {
        //This is the next message we needed to process
        pubInfo->msgsArrived++;
    }
    else if (pubInfo->msgsArrived >= msgPayload->msgNum)
    {
        //We think we've already processed this message number...
        //..this is fine if the engine is duplicating an inflight message after
        //a restart as it didn't process the pubrec..
        if(firstseen == rmsFIRSTSEEN)
        {
            //We had no inflight record for this message...
            //Suggest: duplicate qos 2 messages arrived with different msgIds!?!
            printf("Client %s has processed %u messages from publisher %u but received a new message claiming to be number %u\n",
                   subState->persState->clientId,
                   pubInfo->msgsArrived,
                   msgPayload->pubberNum,
                   msgPayload->msgNum);
            TEST_ASSERT(false, ("Has the client been sent duplicate QoS 2 messages?"));
        }
    }
    else
    {
        printf("Client %s has processed %u messages from publisher %u but received a new message claiming to be number %u\n",
               subState->persState->clientId,
               pubInfo->msgsArrived,
               msgPayload->pubberNum,
               msgPayload->msgNum);

        //We're coming down anyway... we don't care about the rc from dump
        IGNORE_WARN_CHECKRC(ism_engine_dumpClientState(subState->persState->clientId, 9, -1, ""));

        TEST_ASSERT(false, ("Has the client been sent QoS 2 messages out of order?"));
    }

    //Have we received all the messages from this publisher?
    if(pubInfo->msgsArrived == pubInfo->msgExpected)
    {
        return true;
    }

    return false;
}

bool isDeliveryIdRecorded(subscriberStateComplete_t *subState
                         , uint32_t deliveryId)
{
    for (uint32_t im = 0; im <TEST_MAXINFLIGHT; im++)
    {
        if (subState->persState->msgsInFlight[im].deliveryId == deliveryId)
        {
            return true;
        }
    }
    return false;
}

typedef struct tag_pubRelInfo_t {
    char                          StructId[4];
    subscriberStateComplete_t    *subState;
    uint32_t                      deliveryId;
    uint32_t                      pubberNum;
    ismEngine_DeliveryHandle_t    hDelivery;
    bool                          publisherFinished;
    uint64_t                     *pCallbacksInProgress;
} pubRelInfo_t;

#define PUBRELINFO_STRUCTID "PRIS"

void callbackCompleted(int rc, void *handle, void *context)
{
    uint64_t *pCallbacksInProgress  = *(uint64_t **)context;

    __sync_fetch_and_sub(pCallbacksInProgress, 1);
}

void pubRelReceived(int rc, void *handle, void *context)
{
    pubRelInfo_t *info = (pubRelInfo_t *)context;

    ismEngine_CheckStructId(info->StructId, PUBRELINFO_STRUCTID, 1);

    //remove our inflight record for the message
    removeDeliveryId(info->subState, info->deliveryId);
    persistSubberState(info->subState);

    //tell the engine pubcomp
    //TODO: maybe we shouldn't immediately confirm...and have multiple message in doubt (unless this is the last message, where we'd need to tidy up)
    uint64_t *pCallbacksInProgress = info->pCallbacksInProgress;

    rc = ism_engine_confirmMessageDelivery(info->subState->hSession,
                                           NULL,
                                           info->hDelivery,
                                           ismENGINE_CONFIRM_OPTION_CONSUMED,
                                           &pCallbacksInProgress, sizeof(pCallbacksInProgress),
                                           callbackCompleted);

    TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("rc was (%d)", rc));

    if (rc == OK)
    {
        __sync_fetch_and_sub(pCallbacksInProgress, 1);
    }

    if (info->publisherFinished)
    {
        test_log(testLOGLEVEL_VERBOSE, "All messages from publisher %u arrived at client %s",
                                           info->pubberNum, info->subState->persState->clientId);
    }
}


bool messageArrivedCallback(
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
    subscriberStateComplete_t *subState = *(subscriberStateComplete_t **)pConsumerContext;
    uint64_t *pCallbacksInProgress = &(CallbacksInProgress);
    __sync_fetch_and_add(pCallbacksInProgress, 1);

    if (state == ismMESSAGE_STATE_RECEIVED)
    {
        //This is effectively a pubrel from the engine

        //Do we have an inflight record for this message?
        if (isDeliveryIdRecorded(subState, deliveryId))
        {
            //We do so remove it tell the engine pubcomp...
            removeDeliveryId(subState, deliveryId);
        }
        else
        {
            //We don't have a record of the publish for this pubrel...
            //we must have removed it and then terminated before issuing pubcomp...
            //we had better be still synchronising after restart
            if (subState->recoveryComplete)
            {
                printf("Client %s received a message (deliveryId %u) in state received (=pubrel).\n", subState->persState->clientId, deliveryId);
                TEST_ASSERT(false, ("We have no record of an inflight message for that delivery id and we are not in recovery after restart(!)"));
            }
        }
        persistSubberState(subState);

        //tell the engine pubcomp
        //TODO: maybe we shouldn't immediately confirm...and have multiple message in doubt (unless this is the last message, where we'd need to tidy up)
        DEBUG_ONLY int32_t rc;

        rc = ism_engine_confirmMessageDelivery(subState->hSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               &pCallbacksInProgress, sizeof(pCallbacksInProgress),
                                               callbackCompleted);

        TEST_ASSERT(rc == OK|| rc == ISMRC_AsyncCompletion, ("rc was (%d)", rc));

        if (rc == OK)
        {
            __sync_fetch_and_sub(pCallbacksInProgress, 1);
        }
    } else {
        //Check this is a publish from the engine
        TEST_ASSERT(state == ismMESSAGE_STATE_DELIVERED,
                    ("state (%d) != ismMESSAGE_STATE_DELIVERED", state));

        //parse the message
        testMessage_t *msgPayload = parseReceivedMessage(pMsgDetails,
                                                         areaCount, areaTypes, areaLengths,pAreaData);


        //Record that this deliveryId has been seen unless we've already seen it
        recvdMsgStatus_t firstseen = recordDeliveryId(subState, deliveryId, msgPayload);

        if ( (subState->recoveryComplete == false) && (firstseen == rmsFIRSTSEEN))
        {
            //We have started receiving new messages.... recovery is now complete
            subState->recoveryComplete= true;
        }


        //Check the message is the message we expect to see from this publisher and record
        //we got the message from the publisher
        bool publisherFinished = updateSubStateForPubber(hConsumer, subState, firstseen, msgPayload);

        persistSubberState(subState);

        //tell the engine pubrec
        //TODO: maybe we shouldn't immediately confirm...and have multiple message in doubt (unless this is the last message, where we'd need to tidy up)
        // Once the pubrec is completed (sync or not) we'll pretend a pubRel has been received...


        pubRelInfo_t info = { PUBRELINFO_STRUCTID
                            , subState
                            , deliveryId
                            , msgPayload->pubberNum
                            , hDelivery
                            , publisherFinished
                            , pCallbacksInProgress};

        DEBUG_ONLY int32_t rc;
        rc = ism_engine_confirmMessageDelivery(subState->hSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_RECEIVED,
                                               &info, sizeof(info),
                                               pubRelReceived);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("rc was (%d)", rc));

        if (rc == OK)
        {
            pubRelReceived(rc, NULL, &info);
        }
    }

    ism_engine_releaseMessage(hMessage);

    return true;
}

void setupSubscribers(uint32_t numSubbersToSetup,
                      persistableSubscriberState_t *persSubState,
                      uint32_t firstSubId,
                      char *clientIdPrefix,
                      char topicString[TEST_TOPICSTR_BUFSIZE],
                      uint32_t numPublishers,
                      uint32_t msgExpected[])
{
    uint32_t subId = firstSubId;

    for (uint32_t s = 0; s < numSubbersToSetup; s++)
    {
        subscriberStateComplete_t *subState = calloc(1, sizeof(subscriberStateComplete_t));
        TEST_ASSERT(subState != NULL, ("subState == NULL"));

        subState->persState = persSubState;
        subState->recoveryComplete = true; //This is the initial setup of the subscriber...not after restart
        DEBUG_ONLY int32_t rc;
        char clientId[TEST_CLIENTID_BUFSIZE];
        char subName[TEST_SUBNAME_BUFSIZE];

        DEBUG_ONLY int length;
        length = snprintf(clientId,TEST_CLIENTID_BUFSIZE, "%s%d", clientIdPrefix, s);
        TEST_ASSERT(length < TEST_CLIENTID_BUFSIZE,
                    ("length (%d) >= TEST_CLIENTID_BUFSIZE (%d)", length, TEST_CLIENTID_BUFSIZE));

        length = snprintf(subName,TEST_SUBNAME_BUFSIZE, "%s-%s", clientId, topicString);
        TEST_ASSERT(length < TEST_CLIENTID_BUFSIZE,
                    ("length (%d) >= TEST_CLIENTID_BUFSIZE (%d)", length, TEST_CLIENTID_BUFSIZE));

        rc = sync_ism_engine_createClientState(clientId,
                                               PROTOCOL_ID_MQTT,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL, NULL, NULL,
                                               &(subState->hClient));
        TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

        rc = ism_engine_createSession( subState->hClient
                                       , ismENGINE_CREATE_SESSION_OPTION_NONE
                                       , &(subState->hSession)
                                       , NULL
                                       , 0
                                       , NULL);
        TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                        ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

        rc = sync_ism_engine_createSubscription(
                subState->hClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                NULL); // Owning client same as requesting client
        TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

        rc = ism_engine_createConsumer(
                subState->hSession,
                ismDESTINATION_SUBSCRIPTION,
                subName,
                NULL,
                NULL, // Owning client same as session client
                &subState,
                sizeof(subscriberStateComplete_t *),
                messageArrivedCallback,
                NULL,
                test_getDefaultConsumerOptions(subAttrs.subOptions),
                &(subState->hConsumer),
                NULL,
                0,
                NULL);
        TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

        test_log(testLOGLEVEL_TESTPROGRESS, "Created consumer for subscription %s", subName);

        //Record the subscriber state in shared mem
        ismEngine_SetStructId(persSubState->StructId, PERSSUB_STRUCTID);
        strcpy(persSubState->clientId, clientId);
        strcpy(persSubState->subname, subName);
        persSubState->subId = subId++;
        strcpy(persSubState->topicstring, topicString);

        for (uint32_t p = 0; p < numPublishers; p++)
        {
            persSubState->pubState[p].msgExpected = msgExpected[p];
        }

        persistSubberState(subState);

        rc = ism_engine_startMessageDelivery(
                subState->hSession,
                ismENGINE_START_DELIVERY_OPTION_NONE,
                NULL,
                0,
                NULL);
        TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

        //Move to the shared mem for next subscriber
        persSubState = getNextSubberPersState(persSubState);
    }
}

void reopenSubsCallback(
        ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName,
        const char * pTopicString,
        void * properties,
        size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributes,
        uint32_t consumerCount,
        void * pContext)
{
    DEBUG_ONLY int32_t rc;
    subscriberStateComplete_t *subState = (subscriberStateComplete_t *) pContext;

    TEST_ASSERT(strcmp(pSubName, subState->persState->subname) == 0,
                ("'%s' != '%s'", pSubName, subState->persState->subname));

    rc = ism_engine_createConsumer(
            subState->hSession,
            ismDESTINATION_SUBSCRIPTION,
            pSubName,
            NULL,
            NULL, // Owning client same as session client
            &subState,
            sizeof(subscriberStateComplete_t *),
            messageArrivedCallback,
            NULL,
            test_getDefaultConsumerOptions(pSubAttributes->subOptions),
            &(subState->hConsumer),
            NULL,
            0,
            NULL);

    test_log(testLOGLEVEL_TESTPROGRESS, "Recreating consumer for subscription %s, TopicString %s", pSubName, pTopicString);
    TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));
}

void reconnectSubscribers(uint32_t numSubbersToSetup,
                          persistableSubscriberState_t *persSubState)
{
    for (uint32_t s = 0; s < numSubbersToSetup; s++)
    {
        ismEngine_CheckStructId(persSubState->StructId, PERSSUB_STRUCTID, 1);

        subscriberStateComplete_t *subState = calloc(1, sizeof(subscriberStateComplete_t));
        TEST_ASSERT(subState != NULL, ("subState == NULL"));

        subState->persState = persSubState;
        subState->recoveryComplete = false;

        DEBUG_ONLY int32_t rc;

        rc = sync_ism_engine_createClientState(persSubState->clientId,
                                               PROTOCOL_ID_MQTT,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL, NULL, NULL,
                                               &(subState->hClient));
        TEST_ASSERT(rc == ISMRC_ResumedClientState, ("rc (%d) != ISMRC_ResumedClientState", rc));

        rc = ism_engine_createSession( subState->hClient
                                     , ismENGINE_CREATE_SESSION_OPTION_NONE
                                     , &(subState->hSession)
                                     , NULL
                                     , 0
                                     , NULL);
        TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

        rc = ism_engine_listSubscriptions(
                subState->hClient,
                NULL,
                subState,
                reopenSubsCallback);
        TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

        rc = ism_engine_startMessageDelivery(
                subState->hSession,
                ismENGINE_START_DELIVERY_OPTION_NONE,
                NULL,
                0,
                NULL);
        TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

        //get the next persisted record for a subscriber
        persSubState = getNextSubberPersState(persSubState);
    }
}

pubMsgInFlight_t *setupInflightPublishMsg(persistablePublisherState_t *pubshm)
{
    //First look for existing messages in flight to find out the highest message
    //number
    uint32_t highestMessageNum = 0;
    uint32_t emptySlotNum = TEST_MAXINFLIGHT;
    pubMsgInFlight_t *msg = NULL;

    for (uint32_t m=0; m < TEST_MAXINFLIGHT; m++)
    {
        if (pubshm->msgsInFlight[m].msgNum > 0)
        {
            if(pubshm->msgsInFlight[m].msgNum > highestMessageNum)
            {
                highestMessageNum = pubshm->msgsInFlight[m].msgNum;
            }
        }
        else
        {
            if (emptySlotNum == TEST_MAXINFLIGHT)
            {
                emptySlotNum = m;
            }
        }
    }

    //Have we found a valid empty slot
    if(emptySlotNum < TEST_MAXINFLIGHT)
    {
        uint32_t newMsgNum;

        //Work out the number of the next message to send
        if (highestMessageNum > 0)
        {
            newMsgNum = highestMessageNum + 1;
        }
        else
        {
            //No messages in flight
            newMsgNum = pubshm->msgsCompleted + 1;
        }

        if (newMsgNum <= pubshm->msgsTotal)
        {
            //Found an empty slot and still need to send another message...
            //clear out the cruft before we mark it live..
            pubshm->msgsInFlight[emptySlotNum].hUnrel = NULL;
            pubshm->msgsInFlight[emptySlotNum].state = pMIFS_Publishing;

            ismEngine_WriteMemoryBarrier(); //Ensure the cruft is gone before we go live
            pubshm->msgsInFlight[emptySlotNum].msgNum = newMsgNum;
            msg = &(pubshm->msgsInFlight[emptySlotNum]);

            test_log( testLOGLEVEL_VERBOSE, "Publisher %u preparing to send message %u (slot %u)"
                    , pubshm->pubId, newMsgNum, emptySlotNum);
        }
    }

    return msg;
}

void pubRelease( ismEngine_SessionHandle_t hSession
               , pubMsgInFlight_t *msgInFlight)
{
    //We can call this multiple times for the same deliveryid, if it can't
    //find it (as we've called it already) it returns ok.
    DEBUG_ONLY int32_t rc;
    rc = sync_ism_engine_removeUnreleasedDeliveryId( hSession,
                                                     NULL,
                                                     msgInFlight->hUnrel);
    TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));
    //The completion of the above call acts like the receipt of the pubcomp
}


ismEngine_MessageHandle_t createMessage(ismEngine_SessionHandle_t hSession,
                                        uint8_t flags,
                                        uint32_t publisherId,
                                        pubMsgInFlight_t *msgInFlight )
{
    ismEngine_MessageHandle_t hMessage = NULL;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};

    testMessage_t msgPayload = { TESTMSG_STRUCTID_ARRAY,
                                 publisherId,
                                 msgInFlight->msgNum };
    size_t areaLengths[2] = {0,sizeof(testMessage_t)};
    void *areaData[2] = {NULL, (void *)&msgPayload};


    header.Flags = flags;
    header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;


    DEBUG_ONLY int32_t rc;
    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

    return hMessage;
}

void publishCompletedCallback(int32_t retcode, void *handle, void *pContext)
{
    volatile pubMsgInFlight_t *pMsgInFlight = *(pubMsgInFlight_t **)pContext;
    pMsgInFlight->state = pMIFS_Completing;
}

void publish( persistablePublisherState_t *pubshm
            , ismEngine_SessionHandle_t hSession
            , volatile pubMsgInFlight_t *pMsgInFlight)
{
        DEBUG_ONLY int32_t rc;

        ismEngine_MessageHandle_t hMessage;

        hMessage = createMessage( hSession
                        , ismMESSAGE_FLAGS_NONE
                        , pubshm->pubId
                        , (pubMsgInFlight_t *)pMsgInFlight);

        rc = ism_engine_putMessageWithDeliveryIdOnDestination( hSession
                        , ismDESTINATION_TOPIC
                        , pubshm->topicstring
                        , NULL
                        , hMessage
                        , pMsgInFlight->msgNum
                        , (ismEngine_UnreleasedHandle_t *)(&(pMsgInFlight->hUnrel))
                        , &pMsgInFlight
                        , sizeof(pubMsgInFlight_t *)
                        , publishCompletedCallback);

    TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("rc (%d) != OK/Async", rc));

    //Put completing successfully is the equivalent of "pubrec"... change the state to record this
    if (rc == OK)
    {
        pMsgInFlight->state = pMIFS_Completing;
    }
}

void processPubComp(persistablePublisherState_t *pubshm, pubMsgInFlight_t *msgInFlight)
{
    //We only process if we are the "next" message due to complete so on restart
    //we can tell whether the increment has happened
    if (msgInFlight->msgNum == (pubshm->msgsCompleted + 1))
    {
        pubshm->msgsCompleted++;

        //Now we can clear the msginflight... msgNum first as that is the field
        //                                    that says whether the others are valid
        msgInFlight->msgNum = 0;
        ismEngine_WriteMemoryBarrier();

        msgInFlight->hUnrel   = NULL;
        msgInFlight->state    = pMIFS_Publishing;
    }
}

void processEngineDeliveryId(
              uint32_t                        deliveryId,
              ismEngine_UnreleasedHandle_t    hUnrel,
              void *                          pContext)
{
    queryDeliveryId_t *context = (queryDeliveryId_t *)pContext;

    TEST_ASSERT(context->numIds < TEST_MAXINFLIGHT,
               ("context->numIds (%d) >= TEST_MAXINFLIGHT (%d)", context->numIds, TEST_MAXINFLIGHT));
    //We're going to use 0 in the deliveryId array to mean empty
    TEST_ASSERT(deliveryId != 0, ("deliveryId == 0"));
    context->hUnrel[context->numIds] = hUnrel;
    context->deliveryId[context->numIds] = deliveryId;


    test_log(testLOGLEVEL_TESTPROGRESS, "publisher %d received unlreased delivery-id %ld", 
             context->pubId, deliveryId);

    context->numIds++;
}

void resyncPublisherAfterRestart( ismEngine_ClientStateHandle_t hClient
                                        , ismEngine_SessionHandle_t hSession
                                        , persistablePublisherState_t *pubshm)
{
    uint32_t                        deliveryId[TEST_MAXINFLIGHT] = {0};
    ismEngine_UnreleasedHandle_t    hUnrel[TEST_MAXINFLIGHT] = {0};
    queryDeliveryId_t context = { pubshm->pubId,  0, deliveryId, hUnrel };

    DEBUG_ONLY int32_t rc;
    rc = ism_engine_listUnreleasedDeliveryIds(hClient,
                                              &context,
                                              processEngineDeliveryId);
    TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

    //We have records in state publish - engine has record
    //    => pubrel then process pubcomp
    //We have records in state publish - engine has no record
    //    => publish from scratch
    //We have records in state completing - engine has record
    //    => pubrel then process pubcomp
    //We have records in state completing - engine has no record
    //    => remove our record
    //We have no record and engine has record
    //    => ...doom!

    do
    {
        uint32_t lowest_id   = pubshm->msgsTotal+1;
        uint32_t lowest_slot = TEST_MAXINFLIGHT;

        //find the oldest delivery id we have
        for (uint32_t m = 0; m < TEST_MAXINFLIGHT; m++)
        {
                if ((pubshm->msgsInFlight[m].msgNum < lowest_id) && (pubshm->msgsInFlight[m].msgNum != 0))
                {
                        lowest_id   = pubshm->msgsInFlight[m].msgNum;
                        lowest_slot = m;
                }
        }

        if (lowest_slot == TEST_MAXINFLIGHT)
        {
                break;
        }

        test_log(testLOGLEVEL_TESTPROGRESS, "publisher %d processing message %d", 
                 pubshm->pubId, lowest_id);

        //OK... we have the lowest id we have yet to process.. did the engine know about it
        ismEngine_UnreleasedHandle_t engineHandle = NULL;
        for (uint32_t e = 0; e < TEST_MAXINFLIGHT; e++)
        {
                if (deliveryId[e] == lowest_id)
                {
                        engineHandle = hUnrel[e];
                        deliveryId[e] = 0; //record that we've dealt with this engine record
                        break;
                }
        }

        if (engineHandle != NULL)
        {
                //engine has the message...
                test_log(testLOGLEVEL_TESTPROGRESS, "After restart, client %s releasing message %u\n",
                                            pubshm->clientId, pubshm->msgsInFlight[lowest_slot].msgNum );
                pubshm->msgsInFlight[lowest_slot].hUnrel = engineHandle;
                pubRelease(hSession, &(pubshm->msgsInFlight[lowest_slot]));
                processPubComp(pubshm, &(pubshm->msgsInFlight[lowest_slot]));
        }
        else
        {
                //engine doesn't have the message...
                if (pubshm->msgsInFlight[lowest_slot].state == pMIFS_Publishing)
                {
                        test_log(testLOGLEVEL_TESTPROGRESS, "After restart, client %s republishing message %u\n",
                                            pubshm->clientId, pubshm->msgsInFlight[lowest_slot].msgNum );
                        publish(pubshm, hSession, &(pubshm->msgsInFlight[lowest_slot]));
                                pubRelease(hSession, &(pubshm->msgsInFlight[lowest_slot]));
                                processPubComp(pubshm, &(pubshm->msgsInFlight[lowest_slot]));
                }
                else
                {
                        TEST_ASSERT(pubshm->msgsInFlight[lowest_slot].state == pMIFS_Completing,
                                    ("pubshm->msgsInFlight[%d].state (%d) != pMIFS_Completing (%d)", lowest_slot,
                                     pubshm->msgsInFlight[lowest_slot].state, pMIFS_Completing));
                        test_log(testLOGLEVEL_TESTPROGRESS, "After restart, client %s removing left over state for message %u\n",
                                            pubshm->clientId, pubshm->msgsInFlight[lowest_slot].msgNum );
                        processPubComp(pubshm, &(pubshm->msgsInFlight[lowest_slot]));

                        //We may have been processing the pubcomp when we restarted...so the normal process function won't
                        //work (as it will think it is already processed.... so we do it ourselves here:
                        pubMsgInFlight_t *pMsgsInFlight = &(pubshm->msgsInFlight[lowest_slot]);
                        if (pMsgsInFlight->msgNum != 0)
                        {
                            //If we didn't process it in the normal pubcomp function, it must be because it was the one
                            //we were processing at restart... prove that:
                            TEST_ASSERT(pMsgsInFlight->msgNum == pubshm->msgsCompleted,
                                    ("Msg we are reprocessing %u and msgscompleted was %u",
                                            pMsgsInFlight->msgNum, pubshm->msgsCompleted ));

                            pMsgsInFlight->msgNum = 0;
                            ismEngine_WriteMemoryBarrier();

                            pMsgsInFlight->hUnrel   = NULL;
                            pMsgsInFlight->state    = pMIFS_Publishing;
                        }
                }
        }
    }
    while(1);

    //The engine should have not known about any delivery ids we didn't think were in flight... check
    DEBUG_ONLY bool allGood = true;

        for (uint32_t e = 0; e < TEST_MAXINFLIGHT; e++)
        {
                if (deliveryId[e] != 0)
                {
                        printf("After restart engine had an unreleased delivery id for publisher %s, message %u\n", pubshm->clientId, deliveryId[e]);
                        allGood = false;
                }
        }
        TEST_ASSERT(allGood, ("not all good!"));
}

void *publishThread(void *args)
{
    persistablePublisherState_t *pubshm = (persistablePublisherState_t *)args;
    ismEngine_ClientStateHandle_t hClient; ///< client handle so we can destroy cleanly
    ismEngine_SessionHandle_t hSession;
    DEBUG_ONLY int32_t rc;
    persistenceLayerPrivateData_t persLayerData = {0};

    char ThreadName[16];
    sprintf(ThreadName, "Pub(%d)", pubshm->pubId);
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&ThreadName);

    ism_engine_threadInit(0);

    rc = sync_ism_engine_createClientState(pubshm->clientId,
                                           PROTOCOL_ID_MQTT,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClient);
    TEST_ASSERT(rc == OK || rc == ISMRC_ResumedClientState, ("rc (%d) != OK", rc));

    rc = ism_engine_createSession( hClient
                                 , ismENGINE_CREATE_SESSION_OPTION_NONE
                                 , &hSession
                                 , NULL
                                 , 0
                                 , NULL);
    TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

    //Resend messages partly completed
    resyncPublisherAfterRestart(hClient, hSession, pubshm);

    persistPubberState(hSession, &persLayerData, pubshm);
    //We've recorded that this publisher exists, we can be restarted now and after restart, we'll still know about him
    __sync_fetch_and_add(&PublishersPersisted, 1);

    //Each publish has a number of steps
    // 1) we choose a deliveryid... (we choose this to be the msgNum from this client e.g. 7thMsg =id 7  and state=publishing)
    // 2) We do the recording of the id+publish in a transaction....rc of the commit =equiv the publisher receiving a pubrec
    // 3) After the "pubrec" We change state of the deliveryid to completing then later call engine remove the unreleased deliveryid return from this call= "pubcomp"
    // 4) After the "pubcomp", we wait until the msgNum = (msgsSent+1) (then it's the "next message" to complete and during restart we can work
    //                                                                    out whether we've already done the msgssent increment for /this/ msginflight) then:
    //                           a) increment the msgsSent
    //                           b) finally remove the inflight record
    while( pubshm->msgsCompleted < pubshm->msgsTotal )
    {
        volatile pubMsgInFlight_t *msgInFlight = setupInflightPublishMsg(pubshm);

        if (msgInFlight != NULL)
        {
            persistPubberState(hSession, &persLayerData, pubshm);
            publish(pubshm, hSession, msgInFlight);

            //Wait for the publish to complete
            //TODO: Could have multiple publishes in flight

            while (msgInFlight->state == pMIFS_Publishing)
            {
                ism_engine_heartbeat();
                if(msgInFlight->state == pMIFS_Publishing)sched_yield();
            }


            //
            //Record that we have received the pubrec...
            persistPubberState(hSession, &persLayerData, pubshm);


            //TODO: We shouldn't always send pubrel immediately..... there should be if statement saying if we feel like it (30% chance)
            {
                pubRelease(hSession,  (pubMsgInFlight_t *)msgInFlight);

                //TODO: Shouldn't always process pubcomp immediately...as above
                {
                    processPubComp(pubshm, (pubMsgInFlight_t *) msgInFlight);
                    //Record that we've processed the pubcomp
                    persistPubberState(hSession, &persLayerData, pubshm);
                }
            }
        }
        else
        {
            //Clear out some messages in flight...we'll clear them all out
            for (uint32_t m=0; m < TEST_MAXINFLIGHT; m++)
            {
                if (pubshm->msgsInFlight[m].msgNum > 0)
                {
                    //It's only in the publishing state in code-section above or
                    //during restart
                    TEST_ASSERT(pubshm->msgsInFlight[m].state == pMIFS_Completing,
                                ("pubshm->msgsInFlight[m].state (%d) != pMIFS_Completing (%d)", pubshm->msgsInFlight[m].state, pMIFS_Completing));

                    //We've not kept track of whether we've sent the pubrel... but
                    //we can do it multiple times with impunity :)
                    pubRelease(hSession, &(pubshm->msgsInFlight[m]));

                    processPubComp(pubshm, &(pubshm->msgsInFlight[m]));
                    //Record that we've processed the pubcomp
                    persistPubberState(hSession, &persLayerData, pubshm);
                }
            }
        }
    }

    test_log(testLOGLEVEL_TESTPROGRESS, "Publisher %u Finished! MsgsCompleted: %lu MsgTotal %lu",
                     pubshm->pubId, pubshm->msgsCompleted, pubshm->msgsTotal);

    return NULL;
}

void checkAllMsgsArrived( uint32_t numPublishers
                        , uint32_t numSubscribers)
{
    persistableSubscriberState_t *persSub;
    DEBUG_ONLY bool allGood = true;
    uint32_t loopsSinceChange        = 0;
    uint64_t lastMessagesMissing     = 0;
    uint64_t lastCallbacksinProgress = 0;

    do
    {
        allGood = true;
        loopsSinceChange++;
        persSub = getFirstSubberPersState();

        if (CallbacksInProgress > 0)
        {
            if(lastCallbacksinProgress !=  CallbacksInProgress)
            {
                lastCallbacksinProgress = CallbacksInProgress;
                loopsSinceChange = 0; //Callbacks are still happening - haven't hung
            }
            allGood = false;
        }

        uint64_t messagesMissing = 0;

        for (uint32_t s = 0 ; s < numSubscribers; s++)
        {
            for (uint32_t p = 0; p < numPublishers; p++)
            {
                if (persSub->pubState[p].msgExpected != persSub->pubState[p].msgsArrived)
                {
                    allGood = false;
                    messagesMissing += (   persSub->pubState[p].msgExpected
                                         - persSub->pubState[p].msgsArrived);
                }
            }
            persSub = getNextSubberPersState(persSub);
        }

        if (!allGood)
        {
            if (messagesMissing != lastMessagesMissing)
            {
                lastMessagesMissing = messagesMissing;
                loopsSinceChange = 0; //Messages are still arriving...haven't hung
            }

            usleep(1000); //wait a millisecond before we try again
        }
    }
    while(!allGood && loopsSinceChange < 60000); //If we've wait 60 seconds and see no change, we've hung

    if (!allGood)
    {
        for (uint32_t s = 0 ; s < numSubscribers; s++)
        {
            for (uint32_t p = 0; p < numPublishers; p++)
            {
                if (persSub->pubState[p].msgExpected != persSub->pubState[p].msgsArrived)
                {
                    printf("Client %s, expected %u messages from publisher %u but got %u\n",
                            persSub->clientId,persSub->pubState[p].msgExpected, p, persSub->pubState[p].msgsArrived );
                    allGood = false;
                    break;
                }
            }
            persSub = getNextSubberPersState(persSub);
        }
    }
    TEST_ASSERT(allGood, ("not all good"));
}

void startTestClean( bool waitForTestToComplete
                   , bool backupMem
                   , uint32_t numPublishers
                   , uint32_t numSubscribers
                   , uint32_t msgsPerPubber)
{
    initialiseTestState( !waitForTestToComplete || backupMem
                       , numPublishers
                       , numSubscribers );

    uint32_t msgsExpected[numPublishers];

    //at the moment expect every subscriber to get same number of messages from every pubber
    for (uint32_t p = 0; p < numPublishers; p++)
    {
        msgsExpected[p] = msgsPerPubber;
    }

    setupSubscribers(numSubscribers,
                     getFirstSubberPersState(),
                     0,
                     "test:"TPR_RESOURCE_SETID":subclient",
                     "test/"TPR_RESOURCE_SETID"/pubsubrestart",
                     numPublishers,
                     msgsExpected);

    pthread_t pub_thread_id[numPublishers];

    persistablePublisherState_t *persPubber = getFirstPubberPersState();

    for (uint32_t p = 0; p < numPublishers; p++)
    {
        ismEngine_SetStructId(persPubber->StructId, PERSPUB_STRUCTID);

        persPubber->msgsCompleted  = 0;
        persPubber->msgsTotal = msgsPerPubber;
        persPubber->pubId = p;
        snprintf(persPubber->clientId, TEST_CLIENTID_BUFSIZE,
                 "test:"TPR_RESOURCE_SETID":pubClient-%d", p);

        strcpy(persPubber->topicstring, "test/"TPR_RESOURCE_SETID"/pubsubrestart");

        int rc = test_task_startThread( &(pub_thread_id[p]) ,publishThread, (void *)(persPubber),"publishThread");

        if (rc != 0)
        {
            perror("Creating publisher Thread");
            exit(1);
        }
        persPubber = getNextPubberPersState(persPubber);
    }

    if(waitForTestToComplete)
    {
        //We're not restarting part-way through... wait for all the publishes to complete
        for (uint32_t p = 0; p < numPublishers; p++)
        {
            int rc = pthread_join( pub_thread_id[p], NULL);

            if (rc != 0)
            {
                perror("joining publisher Thread");
                exit(1);
            }
        }

        //Now check we got all the messages
        checkAllMsgsArrived(numPublishers, numSubscribers);
    }
}

void startTestReconnect( bool backupMem
                       , uint32_t numPublishers
                       , uint32_t numSubscribers)
{
    restoreTestState(backupMem, numPublishers, numSubscribers);

    reconnectSubscribers( numSubscribers
                        , getFirstSubberPersState() );

    pthread_t pub_thread_id[numPublishers];

    persistablePublisherState_t *persPubber = getFirstPubberPersState();

    for (uint32_t p = 0; p < numPublishers; p++)
    {
        ismEngine_CheckStructId(persPubber->StructId, PERSPUB_STRUCTID, 1);

        test_log(testLOGLEVEL_TESTPROGRESS
                , "Restarting Publisher %u with %u messages completed"
                , p, persPubber->msgsCompleted);

        int rc = test_task_startThread( &(pub_thread_id[p]) ,publishThread, (void *)(persPubber),"publishThread");

        if (rc != 0)
        {
            perror("Creating publisher Thread");
            exit(1);
        }

        persPubber = getNextPubberPersState(persPubber);
    }

    //Wait for all the publishes to complete
    for (uint32_t p = 0; p < numPublishers; p++)
    {
        int rc = pthread_join( pub_thread_id[p], NULL);

        if (rc != 0)
        {
            perror("joining publisher Thread");
            exit(1);
        }
    }

    //Now check we got all the messages
    checkAllMsgsArrived(numPublishers, numSubscribers);
}

int32_t parseArgs( int argc
                 , char *argv[]
                 , uint32_t *pphase
                 , bool *prestart
                 , bool *phaseRunsToCompletion
                 , bool *pbackupMem
                 , testLogLevel_t *pverboseLevel
                 , int *ptrclvl
                 , uint32_t *numPublishers
                 , uint32_t *numSubscribers
                 , uint32_t *msgsPerPubber
                 , char **padminDir
                 , bool *resourceSetStatsEnabled )
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "phase", 1, NULL, 'f' },
        { "noRestart", 0, NULL, 'n' },
        { "verbose", 1, NULL, 'v' },
        { "pubbers",1, NULL, 'p' },
        { "subbers",1, NULL, 's' },
        { "msgsperpubber",1,NULL, 'm' },
        { "backupMem",0,NULL, 'b' },
        { "adminDir", 1, NULL, 'a' },
        { "resourceSetStatsEnabled", 0, NULL, 'r' },
        { NULL, 0, NULL, 0 } };
    int long_index;

    while ((opt = getopt_long(argc, argv, "f:nrm:bp:s:v:a:0123456789", long_options, &long_index)) != -1)
    {
        switch (opt)
        {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                *ptrclvl = opt - '0';
                break;
            case 'f':
                *pphase = atoi(optarg);
                *prestart = false;
                break;
            case 'p':
                *numPublishers = atoi(optarg);
                break;
            case 's':
                *numSubscribers = atoi(optarg);
                break;
            case 'm':
                *msgsPerPubber = atoi(optarg);
                break;
            case 'n':
               *phaseRunsToCompletion = true;
               *prestart = false;
               break;
            case 'v':
                *pverboseLevel = (testLogLevel_t)atoi(optarg);
                break;
            case 'a':
                *padminDir = optarg;
                break;
            case 'b':
               *pbackupMem = true;
               break;
            case 'r':
               *resourceSetStatsEnabled = true;
               break;
            case '?':
            default:
               usage=1;
               break;
        }
    }

    if (*pverboseLevel >= testLOGLEVEL_VERBOSE) *pverboseLevel=testLOGLEVEL_VERBOSE;

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-f 1-2] [-x] [-n] [-v 0-9] [-0...-9] [-p <num>] [-s <num>] [-m <num>]\n", argv[0]);
        fprintf(stderr, "       -f (--phase)       - Run only specified phase (1= before restart, 2=after)\n");
        fprintf(stderr, "       -n (--noRestart)   - Don't restart...let the first phase run to completion\n");
        fprintf(stderr, "       -v (--verbose)     - Test Case verbosity\n");
        fprintf(stderr, "       -a (--adminDir)    - Admin directory to use\n");
        fprintf(stderr, "       -r (--ressetStats) - Enable resource set stats\n");
        fprintf(stderr, "       -0 .. -9           - ISM Trace level\n");
        fprintf(stderr, "       -p <num>           - Number of publishing threads\n");
        fprintf(stderr, "       -s <num>           - Number of subscribers\n");
        fprintf(stderr, "       -m <num>           - Msgs published per publisher\n");
    }

   return usage;
}

//Must be called after engine started as that is what sets up the config and we
//can find out whether disk persistence is being used
uint32_t getDefaultMsgsPerPublisher(void)
{
    if (usingDiskPersistence)
    {
        return 500;
    }
    else
    {
        return 10000;
    }
}

void checkAsyncCBStats(void)
{
    int32_t rc;
    char *pDiagnosticsOutput = NULL;

    rc = ism_engine_diagnostics(
            "AsyncCBStats", "",
            &pDiagnosticsOutput,
            NULL, 0, NULL);

    TEST_ASSERT(rc == OK, ("Getting AsyncCBStats failed, rc = %d", rc));
    TEST_ASSERT(pDiagnosticsOutput != NULL,
            ("Getting AsyncCBStats: Output was NULL"));

    test_log(testLOGLEVEL_VERBOSE, "AsyncCBStats: %s\n", pDiagnosticsOutput );

    //What can we check? - very little esp. if persistence is not enabled
    TEST_ASSERT(strstr(pDiagnosticsOutput,"TotalReadyCallbacks") != NULL,
             ("AsyncCBStats didn't include TotalReadyCallbacks"));

    TEST_ASSERT(strstr(pDiagnosticsOutput,"TotalWaitingCallbacks") != NULL,
             ("AsyncCBStats didn't include TotalWaitingCallbacks"));

    if(usingDiskPersistence)
    {
        //We should be reporting stats from at least one async CB thread

        TEST_ASSERT(strstr(pDiagnosticsOutput,"ThreadId") != NULL,
                 ("AsyncCBStats didn't include ThreadId"));
        TEST_ASSERT(strstr(pDiagnosticsOutput,"PeriodSeconds") != NULL,
                 ("AsyncCBStats didn't include PeriodSeconds"));
    }
    ism_engine_freeDiagnosticsOutput(pDiagnosticsOutput);
}

/*********************************************************************/
/* NAME:        testPubSubRestart                                    */
/* DESCRIPTION: This program is a check that a restart whilst msgs   */
/*              flowing doesn't cause dups/gaps                      */
/*                                                                   */
/*              Each test runs in 2 phases                           */
/*              Phase 1. Subbers+Pubs created from scratch, store    */
/*                       state in shared memory                      */
/*              Phase 2. At the end of Phase 1, the program re-execs */
/*                       itself and pubbing continues based on state */
/*                       in shared memory                            */
/*                                                                   */
/* USAGE:       see parseArgs()                                      */
/*********************************************************************/

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int rc = 0;
    char *newargv[20];
    int i;
    uint32_t phase = 1;
    bool restart = true;
    bool phaseRunsToCompletion = false;
    bool backupMem = false;
    testLogLevel_t testLogLevel = testLOGLEVEL_CHECK;
    uint32_t numPublishers=5;
    uint32_t numSubscribers=10;
    uint32_t msgsPerPubber=0;
    char *adminDir=NULL;
    bool resourceSetStatsEnabled=false;
    bool forcedRSE=false;
    bool resourceSetStatsTrackUnmatched=false;

    ism_time_t seedVal = ism_common_currentTimeNanos();
    srand(seedVal);

    if (argc != 1)
    {
        /*************************************************************/
        /* Parse arguments                                           */
        /*************************************************************/
        rc = parseArgs( argc
                      , argv
                      , &phase
                      , &restart
                      , &phaseRunsToCompletion
                      , &backupMem
                      , &testLogLevel
                      , &trclvl
                      , &numPublishers
                      , &numSubscribers
                      , &msgsPerPubber
                      , &adminDir
                      , &resourceSetStatsEnabled );
        if (rc != 0)
        {
            goto mod_exit;
        }
    }

    // 70% of the time, turn on resourceSetStats when not explicitly requested
    if (resourceSetStatsEnabled == false && phase == 1)
    {
        resourceSetStatsEnabled = ((rand()%100) >= 30);
        forcedRSE=resourceSetStatsEnabled;
    }

    // We want to gather resource set stats
    if (resourceSetStatsEnabled)
    {
        setenv("IMA_TEST_RESOURCESET_CLIENTID_REGEX", "^[^:]+:([^:]+):", false);
        setenv("IMA_TEST_RESOURCESET_TOPICSTRING_REGEX", "^[^/]+/([^/]+)", false);
        setenv("IMA_TEST_PROTOCOL_ALLOW_MQTT_PROXY", "true", false);

        resourceSetStatsEnabled = true;
        // 80% of the time track unmatched
        // NOTE: We OVERRIDE an existing setting (so 1st run might track, second might not)
        resourceSetStatsTrackUnmatched = ((rand()%100) >= 20);
        setenv("IMA_TEST_RESOURCESET_TRACK_UNMATCHED", resourceSetStatsTrackUnmatched ? "true" : "false", true);
    }

    /* Unless there is an explicit env var saying differently we want to */
    /* turn on async CB stats                                            */
    setenv("IMA_TEST_ASYNCCBSTATS_ENABLED","1", false);

    /*****************************************************************/
    /* Process Initialise                                            */
    /*****************************************************************/
    test_setLogLevel(testLogLevel);

    rc = test_processInit(trclvl, adminDir);
    if (rc != OK)
    {
        goto mod_exit;
    }

    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    /************************************************************************/
    /* Set the default policy info to allow deep(ish) queues                */
    /************************************************************************/
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 100000;

    if (phase == 1)
    {
        // Prepare the command-line that will do the recovery
        TEST_ASSERT(argc < 18, ("argc (%d) >= 18", argc));

        newargv[0]=argv[0];
        newargv[1]="-f";
        newargv[2]="2";
        newargv[3]="-a";
        newargv[4]=adminDir;
        int offset = 4;
        if (forcedRSE)
        {
            offset++;
            newargv[offset]="-r";
        }
        for (i=1; i <= argc; i++)
        {
            newargv[i+offset]=argv[i];
        }

        // Start up the Engine - Cold Start
        test_log(testLOGLEVEL_TESTDESC, "Starting engine - cold start");
        rc = test_engineInit_DEFAULT;
        if (rc != 0)
        {
            goto mod_exit;
        }

        //Now we've read the config, decide the default number of messages
        //based on whether we're using disk persistence
        if (msgsPerPubber == 0)
        {
            msgsPerPubber = getDefaultMsgsPerPublisher();
        }
        test_log(testLOGLEVEL_TESTDESC, "Using %u msgsPerPublisher", msgsPerPubber);

        //Prepare the store records for recovery
        startTestClean( phaseRunsToCompletion
                      , backupMem
                      , numPublishers
                      , numSubscribers
                      , msgsPerPubber );

        //Now re-exec ourselves to do the recovery
        if (restart || (!phaseRunsToCompletion))
        {
           //Wait until the first publisher gets roughly 33% through...
           uint32_t msgsToWaitFor = msgsPerPubber / 3;
           persistablePublisherState_t *firstPubber = getFirstPubberPersState();

           uint64_t numSleeps = 0;

           //Can't restart until all the publishers persisted - could wait for any fraction
           //of the messages to be completed
           while(   PublishersPersisted < numPublishers
                 || firstPubber->msgsCompleted < msgsToWaitFor)
           {
               usleep(10);

               //Look at async callback stats every few seconds
               if ((numSleeps & ((1<<17)-1)) == 0)
               {
                    checkAsyncCBStats();
               }

               numSleeps++;
           }

           test_log(testLOGLEVEL_TESTDESC, "Ready to restart");

           //Can break into a debugger here and generate a core to reproduce issues
           //ieut_breakIntoDebugger();

           ismEngine_ResourceSetMonitor_t *results = NULL;
           uint32_t resultCount = 0;

           rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 10, NULL);
           if (rc != OK) goto mod_exit;

           if (resourceSetStatsEnabled)
           {
               uint32_t expectResultCount = resourceSetStatsTrackUnmatched ? 2 : 1;
               if (resultCount != expectResultCount)
               {
                   printf("ERROR: line(%d) ResultCount %u should be %u\n", __LINE__, resultCount, expectResultCount);
                   abort();
                   rc = ISMRC_Error;
                   goto mod_exit;
               }

               for(int32_t r=0; r<resultCount; r++)
               {
                   if (strcmp(results[r].resourceSetId, iereDEFAULT_RESOURCESET_ID) == 0)
                   {
                       iereResourceSetHandle_t defaultResourceSet = iere_getDefaultResourceSet();
                       if (results[r].resourceSet != defaultResourceSet)
                       {
                           printf("ERROR: line(%d) Default resource set pointer %p doesn't match expected %p\n",
                                  __LINE__, results[r].resourceSet, defaultResourceSet);
                       }
                   }
                   else
                   {
                       if (strcmp(results[r].resourceSetId, TPR_RESOURCE_SETID) != 0)
                       {
                           printf("ERROR: line(%d) Unexpected resourceSetId '%s' should be '%s'\n",
                                  __LINE__, results[r].resourceSetId, TPR_RESOURCE_SETID);
                           rc = ISMRC_Error;
                           goto mod_exit;
                       }

                       if (results[r].stats.Connections == 0)
                       {
                           printf("ERROR: line(%d) Connection count %ld should be >0\n", __LINE__, results[r].stats.Connections);
                           rc = ISMRC_Error;
                           goto mod_exit;
                       }
                   }
               }
           }
           else if (resultCount != 0)
           {
               printf("ERROR: line(%d) ResultCount %u should be 0\n", __LINE__, resultCount);
               rc = ISMRC_Error;
               goto mod_exit;
           }

           ism_engine_freeResourceSetMonitor(results);

           if (restart)
           {
               test_log(testLOGLEVEL_TESTDESC, "Restarting...");
               rc = test_execv(newargv[0], newargv);
               TEST_ASSERT(false, ("'test_execv' failed. rc = %d", rc));
           }
        }
    }

    if (phase == 2)
    {
        if (backupMem)
        {
            backupPersistedState();
        }

        // Start up the Engine - Warm Start
        test_log(testLOGLEVEL_TESTDESC, "Starting engine - warm start");

        rc = test_engineInit(false,  true,
                             ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                             false, /*recovery should complete ok*/
                             ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                             testDEFAULT_STORE_SIZE);

        TEST_ASSERT(rc == OK, ("'test_engineInit' failed. rc = %d", rc));

        ismEngine_ResourceSetMonitor_t *results = NULL;
        uint32_t resultCount = 0;

        rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 10, NULL);
        if (rc != OK) goto mod_exit;

        if (resourceSetStatsEnabled)
        {
            uint32_t expectResultCount = resourceSetStatsTrackUnmatched ? 2 : 1;
            if (resultCount != expectResultCount)
            {
                printf("ERROR: line(%d) ResultCount %u should be %u\n", __LINE__, resultCount, expectResultCount);
                abort();
                rc = ISMRC_Error;
                goto mod_exit;
            }

            for(int32_t r=0; r<resultCount; r++)
            {
                if (strcmp(results[r].resourceSetId, iereDEFAULT_RESOURCESET_ID) == 0)
                {
                    iereResourceSetHandle_t defaultResourceSet = iere_getDefaultResourceSet();
                    if (results[r].resourceSet != defaultResourceSet)
                    {
                        printf("ERROR: line(%d) Default resource set pointer %p doesn't match expected %p\n",
                               __LINE__, results[r].resourceSet, defaultResourceSet);
                    }
                }
                else
                {
                    if (strcmp(results[r].resourceSetId, TPR_RESOURCE_SETID) != 0)
                    {
                        printf("ERROR: line(%d) Unexpected resourceSetId '%s' should be '%s'\n",
                               __LINE__, results[r].resourceSetId, TPR_RESOURCE_SETID);
                        rc = ISMRC_Error;
                        goto mod_exit;
                    }

                    if (results[r].stats.Connections != 0)
                    {
                        printf("ERROR: line(%d) Connection count %ld should be 0\n", __LINE__, results[r].stats.Connections);
                        rc = ISMRC_Error;
                        goto mod_exit;
                    }
                }
            }
        }
        else if (resultCount != 0)
        {
            printf("ERROR: line(%d) ResultCount %u should be 0\n", __LINE__, resultCount);
            rc = ISMRC_Error;
            goto mod_exit;
        }

        ism_engine_freeResourceSetMonitor(results);

        //Reconnect to the server and carry on from where we left off
        startTestReconnect( backupMem
                          , numPublishers
                          , numSubscribers );

        results = NULL;
        resultCount = 0;

        rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES, 10, NULL);
        if (rc != OK) goto mod_exit;

        if (resourceSetStatsEnabled)
        {
            uint32_t expectResultCount = resourceSetStatsTrackUnmatched ? 2 : 1;
            if (resultCount != expectResultCount)
            {
                printf("ERROR: line(%d) ResultCount %u should be %u\n", __LINE__, resultCount, expectResultCount);
                rc = ISMRC_Error;
                goto mod_exit;
            }

            uint64_t prevTotalMemory = UINT64_MAX;

            for(int32_t r=0; r<resultCount; r++)
            {
                // Check order of the results
                if (results[r].stats.TotalMemoryBytes > prevTotalMemory)
                {
                    printf("ERROR: line(%d) Results are not ordered as expected %lu > %lu\n",
                           __LINE__, results[r].stats.TotalMemoryBytes, prevTotalMemory);
                    rc = ISMRC_Error;
                    goto mod_exit;
                }

                prevTotalMemory = results[r].stats.TotalMemoryBytes;

                if (strcmp(results[r].resourceSetId, iereDEFAULT_RESOURCESET_ID) == 0)
                {
                    iereResourceSetHandle_t defaultResourceSet = iere_getDefaultResourceSet();
                    if (results[r].resourceSet != defaultResourceSet)
                    {
                        printf("ERROR: line(%d) Default resource set pointer %p doesn't match expected %p\n",
                               __LINE__, results[r].resourceSet, defaultResourceSet);
                        rc = ISMRC_Error;
                        goto mod_exit;
                    }
                }
                else
                {
                    if (strcmp(results[r].resourceSetId, TPR_RESOURCE_SETID) != 0)
                    {
                        printf("ERROR: line(%d) Unexpected resourceSetId '%s' should be '%s'\n",
                               __LINE__, results[r].resourceSetId, TPR_RESOURCE_SETID);
                        rc = ISMRC_Error;
                        goto mod_exit;
                    }

                    // The test doesn't disconnect, so we expect everything to be active
                    if (   results[r].stats.Connections != (numPublishers+numSubscribers)
                        || results[r].stats.ActivePersistentClients != (numPublishers+numSubscribers))

                    {
                        printf("ERROR: line(%d) Connection:%ld ActivePersistentClients:%ld should be 15\n",
                               __LINE__, results[r].stats.Connections, results[r].stats.ActivePersistentClients);
                        rc = ISMRC_Error;
                        goto mod_exit;
                    }
                }

    #if 1
                iereResourceSet_t *resourceSet = (iereResourceSet_t *)(results[r].resourceSet);
                printf("ResourceSetStats for setId '%s' {", resourceSet->stats.resourceSetIdentifier);
                for(int32_t ires=0; ires<ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS; ires++)
                {
                    printf("%ld%c", resourceSet->stats.int64Stats[ires], ires == ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS-1 ? '}':',');
                }
                printf("\n");
    #endif
            }
        }
        else if (resultCount != 0)
        {
            printf("ERROR: line(%d) ResultCount %u should be 0\n", __LINE__, resultCount);
            rc = ISMRC_Error;
            goto mod_exit;
        }

        ism_engine_freeResourceSetMonitor(results);

        rc = test_engineTerm(false);
        TEST_ASSERT( (rc == OK),("'test_engineTerm' failed. rc = %d", rc) );
        rc = test_processTerm(true);
        if (rc != OK) goto mod_exit;
        test_removeAdminDir(adminDir);
    }

 mod_exit:

    if (rc == OK) {
        printf("SUCCESS: testPubSubRestart: tests passed\n");
    } else {
        printf("FAILURE: testPubSubRestart: tests failed (rc=%d)\n", rc);
    }
    return rc;
}
