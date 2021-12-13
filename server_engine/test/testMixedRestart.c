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

#include <stdint.h>
#include <assert.h>
#include <getopt.h>

#include "engine.h"
#include "transaction.h"
#include "queueNamespace.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

static bool engine_commit_allowed=true;


typedef struct tag_test_MsgDescription {
    char *topicString;
    char *payload;
    int qos;
    int flags;
} test_MsgDescription;

#define TEST_MSGFLAGS_NONE          0x0
#define TEST_MSGFLAGS_WILLMESSAGE   0x1
#define TEST_MSGFLAGS_RETAIN        0x2

typedef struct tag_subsContext_t {
    const char *clientId;
    ismEngine_SessionHandle_t   hSession;
    ismEngine_ConsumerHandle_t  hConsumer;
    int numMsgsExpected;
    int numMsgsArrived;
    test_MsgDescription **expectedMsgs;
    pthread_mutex_t *pMutex;
} subsContext_t;

//Non-English characters taken from: http://www.columbia.edu/~fdc/utf8/
//If these are altered/changed test1_check() may need to be altered to change which messages it expects
#define NUM_MSGS_PUT 4
test_MsgDescription MsgsPut[4] = {
        {"/testMixedRestart/WillMessage", "This is a will message", 2, TEST_MSGFLAGS_WILLMESSAGE},
        {"/testMixedRestart/normal", "è‰²ã�¯åŒ‚ã�¸ã�© æ•£ã‚Šã�¬ã‚‹ã‚’\næˆ‘ã�Œä¸–èª°ã�ž å¸¸ã�ªã‚‰ã‚€\næœ‰ç‚ºã�®å¥¥å±± ä»Šæ—¥è¶Šã�ˆã�¦\næµ…ã��å¤¢è¦‹ã�˜ é…”ã�²ã‚‚ã�›ã�š", 2, TEST_MSGFLAGS_NONE},
        {"/testMixedRestart/normal", "ã�„ã‚�ã�¯ã�«ã�»ã�¸ã�©ã€€ã�¡ã‚Šã�¬ã‚‹ã‚’\nã‚�ã�Œã‚ˆã�Ÿã‚Œã�žã€€ã�¤ã�­ã�ªã‚‰ã‚€\nã�†ã‚�ã�®ã�Šã��ã‚„ã�¾ã€€ã�‘ã�µã�“ã�ˆã�¦\nã�‚ã�•ã��ã‚†ã‚�ã�¿ã�˜ã€€ã‚‘ã�²ã‚‚ã�›ã�š", 2, TEST_MSGFLAGS_RETAIN},
        {"/testMixedRestart/normal", "ÐœÐ¾Ð¶Ð°Ð¼ Ð´Ð° Ñ˜Ð°Ð´Ð°Ð¼ Ñ�Ñ‚Ð°ÐºÐ»Ð¾, Ð° Ð½Ðµ Ð¼Ðµ ÑˆÑ‚ÐµÑ‚Ð°", 2, TEST_MSGFLAGS_NONE},
};

int32_t ietr_commit( ieutThreadData_t *pThreadData
                   , ismEngine_Transaction_t *pTran
                   , uint32_t options
                   , ismEngine_Session_t *pSession
                   , ietrAsyncTransactionData_t *pAsyncData
                   , ietrCommitCompletionCallback_t  pCallbackFn)
{
    static int32_t (*real_commit)( ieutThreadData_t *
                                 , ismEngine_Transaction_t *
                                 , uint32_t
                                 , ismEngine_Session_t *
                                 , ietrAsyncTransactionData_t *
                                 , ietrCommitCompletionCallback_t) = NULL;
    int32_t rc=OK;

    if (!real_commit)
    {
        //Find the real version of ietr_commit
        real_commit = dlsym(RTLD_NEXT, "ietr_commit");
        TEST_ASSERT(real_commit != NULL,("Couldn't dlsym the real ietr_commit"));
    }

    if (engine_commit_allowed)
    {
        test_log(testLOGLEVEL_VERBOSE, "Calling real ietr_commit function ...\n");
        rc = real_commit(pThreadData, pTran, options, pSession, pAsyncData, pCallbackFn);
    }
    else
    {
        ieutTRACEL(pThreadData, 0xDD00DD00DD00DD00, 2, "*** NOT_ietr_commit %p\n", pTran);
        test_log(testLOGLEVEL_VERBOSE, "Not doing commit!\n");
    }

    return rc;
}

void ieut_ffdc( const char *function
              , uint32_t seqId
              , bool core
              , const char *filename
              , uint32_t lineNo
              , char *label
              , uint32_t retcode
              , ... )
{
    static void (*real_ieut_ffdc)(const char *, uint32_t, bool, const char *, uint32_t, char *, uint32_t, ...) = NULL;

    if (real_ieut_ffdc == NULL)
    {
        real_ieut_ffdc = dlsym(RTLD_NEXT, "ieut_ffdc");
    }

    if (engine_commit_allowed == false && strcmp(function, "ieut_leavingEngine") == 0)
    {
        test_log(testLOGLEVEL_VERBOSE, "Ignored ieut_ffdc from %s line:%u\n", function, filename, lineNo);
        return;
    }

    TEST_ASSERT(0, ("Unexpected FFDC from %s:%u", filename, lineNo));
}

ismEngine_MessageHandle_t createMessage(ismEngine_SessionHandle_t hSession,
                                        int qos,
                                        uint8_t flags,
                                        const char *topicString,
                                        const char *payload)
{
    ismEngine_MessageHandle_t hMessage = NULL;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    char xbuf[1024];
    concat_alloc_t props = {xbuf, sizeof(xbuf)};

    ism_field_t field;
    field.type = VT_String;
    field.val.s = (char *)topicString;
    ism_common_putPropertyID(&props, ID_Topic, &field);

    if ((flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) != 0)
    {
        // Add the serverUID to the properties
        field.type = VT_String;
        field.val.s = (char *)ism_common_getServerUID();
        ism_common_putPropertyID(&props, ID_OriginServer, &field);

        // Add the serverTime to the properties
        field.type = VT_Long;
        field.val.l = ism_engine_retainedServerTime();
        ism_common_putPropertyID(&props, ID_ServerTime, &field);
    }

    void *properties = props.buf;
    size_t propertiesSize = props.used;

    size_t payloadSize = strlen(payload)+1;
    size_t areaLengths[2] = {propertiesSize, payloadSize};
    void *areaData[2] = {properties, (char *)payload};


    header.Flags = flags;

    if (qos > 0) {
        header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;

        if(qos == 1) {
            header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
        } else {
            TEST_ASSERT(qos == 2,("qos was %d", qos));
            header.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
        }
    }
    else
    {
        header.Reliability = ismMESSAGE_RELIABILITY_AT_MOST_ONCE;
        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
    }

    DEBUG_ONLY int32_t rc;

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    // Free any properties we allocated
    if (props.inheap)
    {
        ism_common_freeAllocBuffer(&props);
    }

    return hMessage;
}

void setupSubber(bool AllowSubTrans, int qos, const char *clientid, const char *subName, const char *topicString)
{
    ismEngine_ClientStateHandle_t hClient;
    DEBUG_ONLY int32_t rc;
    ismEngine_SessionHandle_t hSession;

    rc = sync_ism_engine_createClientState(clientid,
                                           testDEFAULT_PROTOCOL_ID,
                                           (qos == 0) ? ismENGINE_CREATE_CLIENT_OPTION_NONE:
                                                        ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClient);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    rc = ism_engine_createSession( hClient
                                 , ismENGINE_CREATE_SESSION_OPTION_NONE
                                 , &hSession
                                 , NULL
                                 , 0
                                 , NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    // NOTE: If AllowSubTrans is false, we disallow engine_commit calls from this point on.
    //       So this had better be the last test we run in this phase of the test... We make
    //       sure of that by asserting engine_commit_allowed is true (so we haven't been called
    //       again).
    if (!AllowSubTrans)
    {
        engine_commit_allowed = false;
    }
    else
    {
        TEST_ASSERT(engine_commit_allowed == true, ("engine_commit_allowed set to false\n"));
    }

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ((qos == 1) ? ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE:
                                                     ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE) };

    rc = sync_ism_engine_createSubscription(
            hClient,
            subName,
            NULL,
            ismDESTINATION_TOPIC,
            topicString,
            &subAttrs,
            NULL); // Owning client same as requesting client
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    if (AllowSubTrans)
    {
        rc = sync_ism_engine_destroyClientState(hClient, 0);
        TEST_ASSERT(rc == OK, ("rc was %d", rc));
    }
}


void sendMessages(uint32_t numMsgs, test_MsgDescription MsgsToPut[numMsgs])
{
    DEBUG_ONLY int32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    rc = ism_engine_createClientState("publishingClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    rc = ism_engine_createSession( hClient
                                 , ismENGINE_CREATE_SESSION_OPTION_NONE
                                 , &hSession
                                 , NULL
                                 , 0
                                 , NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    for (int i=0; i<numMsgs; i++)
    {
        uint8_t msgFlags = 0;

        if (MsgsToPut[i].flags & TEST_MSGFLAGS_RETAIN)
        {
            msgFlags |= ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;
        }
        ismEngine_MessageHandle_t  message = createMessage(hSession,
                                                           MsgsToPut[i].qos,
                                                           msgFlags,
                                                           MsgsToPut[i].topicString,
                                                           MsgsToPut[i].payload);

        if (MsgsToPut[i].flags & TEST_MSGFLAGS_WILLMESSAGE)
        {
            test_log(testLOGLEVEL_VERBOSE, "Saving message %d as a will message on topicstring %s\n", i, MsgsToPut[i].topicString);
            rc = ism_engine_setWillMessage( hClient
                                          , ismDESTINATION_TOPIC
                                          , MsgsToPut[i].topicString
                                          , message
                                          , 0
                                          , iecsWILLMSG_TIME_TO_LIVE_INFINITE
                                          , NULL, 0, NULL);
            TEST_ASSERT(rc == OK, ("rc was %d", rc));
        }
        else
        {
            test_log(testLOGLEVEL_VERBOSE, "Publishing message %d on topicstring %s\n", i, MsgsToPut[i].topicString);


            rc = sync_ism_engine_putMessageOnDestination( hSession
                                                   , ismDESTINATION_TOPIC
                                                   , MsgsToPut[i].topicString
                                                   , NULL
                                                   , message);
            TEST_ASSERT(rc == OK, ("rc was %d", rc));
        }
    }

    rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));
}

//NB: Doesn't ack the message!
bool noAckCallback(
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
    ism_engine_releaseMessage(hMessage);
    return true; //We'd like more messages
}

//
//Setup functions are called before the restart...
//

//Creates a durable sub, creates qos=2 consumer on it which receives a message but doesn't ack it. Destroys consumer and sub...leaves client
//(ensures we don't leave a ghost MDR which causes us to choke during restart)
void test1_setup(void)
{
    int32_t rc = OK;
    ismEngine_ClientStateHandle_t hClient = NULL;
    ismEngine_SessionHandle_t hSession = NULL;
    ismEngine_ConsumerHandle_t hConsumer = NULL;
    char *testtopic = "/ghostmdr";
    char *subname = "ghostmdr_sub";

    rc = sync_ism_engine_createClientState("test_ghostmdr",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL,  NULL,
                                           &hClient);
    TEST_ASSERT(rc == OK,("ERROR: ism_engine_createClientState() returned %d\n", rc));

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT(rc == OK,("ERROR: ism_engine_createSession() returned %d\n", rc));

    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);

    TEST_ASSERT(rc == OK,("ERROR: ism_engine_startMessageDelivery() returned %d\n", rc));

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    rc = sync_ism_engine_createSubscription(
                hClient,
                subname,
                NULL,
                ismDESTINATION_TOPIC,
                testtopic,
                &subAttrs,
                NULL); // Owning client same as requesting client
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    rc = ism_engine_createConsumer(
                hSession,
                ismDESTINATION_SUBSCRIPTION,
                subname,
                NULL, // Don't need subAttrs when consuming from a sub
                NULL, // Owning client same as session client
                NULL,
                0,
                noAckCallback,
                NULL,
                test_getDefaultConsumerOptions(subAttrs.subOptions),
                &hConsumer,
                NULL,
                0,
                NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    test_MsgDescription PutMsg = {testtopic, "MsgPayload", 2, 0};
    sendMessages(1, &PutMsg);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    rc = ism_engine_destroySubscription(hClient, subname, hClient, NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));
}


//Creates a durable sub, creates qos=2 consumer on it which receives a message but doesn't ack it. Destroys consumer and sub...leaves client
//for a non-durable client-state (ensures we tidy up the delivery IDs as we remove the sub)
void test2_setup(void)
{
    int32_t rc = OK;
    ismEngine_ClientStateHandle_t hClient = NULL;
    ismEngine_SessionHandle_t hSession = NULL;
    ismEngine_ConsumerHandle_t hConsumer = NULL;
    char *testtopic = "/ghostmdr2";
    char *subname = "ghostmdr_sub";

    rc = ism_engine_createClientState("test_ghostmdr2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,  NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT(rc == OK,("ERROR: ism_engine_createClientState() returned %d\n", rc));

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT(rc == OK,("ERROR: ism_engine_createSession() returned %d\n", rc));

    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);

    TEST_ASSERT(rc == OK,("ERROR: ism_engine_startMessageDelivery() returned %d\n", rc));

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };

    rc = sync_ism_engine_createSubscription(
                hClient,
                subname,
                NULL,
                ismDESTINATION_TOPIC,
                testtopic,
                &subAttrs,
                NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    rc = ism_engine_createConsumer(
                hSession,
                ismDESTINATION_SUBSCRIPTION,
                subname,
                NULL,
                NULL, // Owning client same as session client
                NULL,
                0,
                noAckCallback,
                NULL,
                test_getDefaultConsumerOptions(subAttrs.subOptions),
                &hConsumer,
                NULL,
                0,
                NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    test_MsgDescription PutMsg = {testtopic, "MsgPayload", 2, 0};
    sendMessages(1, &PutMsg);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    rc = ism_engine_destroySubscription(hClient, subname, hClient, NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));
}


//Creates a non-durable sub, creates qos=2 consumer on it which receives a message but doesn't ack it. Destroys consumer and sub...leaves client
//for a non-durable client-state (ensures we tidy up the delivery IDs as we remove the sub)
void test3_setup(void)
{
    int32_t rc = OK;
    ismEngine_ClientStateHandle_t hClient = NULL;
    ismEngine_SessionHandle_t hSession = NULL;
    ismEngine_ConsumerHandle_t hConsumer = NULL;
    char *testtopic = "/ghostmdr3";

    rc = ism_engine_createClientState("test_ghostmdr3",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL,  NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT(rc == OK,("ERROR: ism_engine_createClientState() returned %d\n", rc));

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT(rc == OK,("ERROR: ism_engine_createSession() returned %d\n", rc));

    rc = ism_engine_startMessageDelivery(hSession,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL, 0, NULL);

    TEST_ASSERT(rc == OK,("ERROR: ism_engine_startMessageDelivery() returned %d\n", rc));

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE };
    rc = ism_engine_createConsumer(
                hSession,
                ismDESTINATION_TOPIC,
                testtopic,
                &subAttrs,
                NULL, // Unused for TOPIC
                NULL,
                0,
                noAckCallback,
                NULL,
                test_getDefaultConsumerOptions(subAttrs.subOptions),
                &hConsumer,
                NULL,
                0,
                NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    test_MsgDescription PutMsg = {testtopic, "MsgPayload", 2, 0};
    sendMessages(1, &PutMsg);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));
}

void createGenerations(uint32_t numGens)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    char *createGensQName = "creategenqueues";

	// Create the queue
	int32_t rc = ieqn_createQueue( pThreadData
	                             , createGensQName
			                     , multiConsumer
			                     , ismQueueScope_Server, NULL
			                     , NULL
		                       	 , NULL
			                     , NULL );
	TEST_ASSERT(rc == OK, ("Failed to create queue, rc = %d", rc));

	//Put lots of messages to the queue, specifying orderId (and skipping orderId 1)
	ismStore_GenId_t genofFirstMsg = 0;
	ismStore_GenId_t genofCurrentMsg = 0;
	ismQHandle_t hQueue =  ieqn_getQueueHandle(pThreadData, createGensQName);

	do
	{
		ismEngine_MessageHandle_t hBigMsg;

		rc = test_createMessage(1000000, // 1MB
				ismMESSAGE_PERSISTENCE_PERSISTENT,
				ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
				ismMESSAGE_FLAGS_NONE,
				0,
				ismDESTINATION_NONE, NULL,
				&hBigMsg, NULL);
		TEST_ASSERT(rc == OK, ("%d != %d", rc, OK));

		rc = ieq_put( pThreadData
                              , hQueue
                              , ieqPutOptions_NONE
                              , NULL
                              , hBigMsg
                              , IEQ_MSGTYPE_INHERIT
                              , NULL );

		//We're going to look at hBigMsg - in a real appliance it could have been got and freed
		//by now but we know there is no consumer
		if (0 == genofFirstMsg)
		{
			ism_store_getGenIdOfHandle((((ismEngine_Message_t *)hBigMsg)->StoreMsg.parts.hStoreMsg),
					&genofFirstMsg);
		}
		else
		{
			ism_store_getGenIdOfHandle((((ismEngine_Message_t *)hBigMsg)->StoreMsg.parts.hStoreMsg),
					&genofCurrentMsg);
		}


		TEST_ASSERT(rc == 0, ("Failed to put message rc = %d", rc));
	}
	while( (genofCurrentMsg == 0) || ((genofFirstMsg + numGens) >= genofCurrentMsg) );
}
//Needs to be final test before we restart...stops store commit working part way through
void test4_setup(void)
{
    setupSubber(true, 2,
                "successfulClient1",
                "successfulSub1",
                "/testMixedRestart/#");

    setupSubber(true, 2,
                "successfulClient2",
                "successfulSub2",
                "/testMixedRestart/#");

    sendMessages(NUM_MSGS_PUT, MsgsPut);

    createGenerations(3);

    setupSubber(false, 2,
                "badClient",
                "badSub",
                "/testMixedRestart/#");
}




void compareMsgs(const char *clientId,
                 test_MsgDescription *           expectedMsg,
                 ismMessageHeader_t *            pMsgDetails,
                 uint8_t                         areaCount,
                 ismMessageAreaType_t            areaTypes[areaCount],
                 size_t                          areaLengths[areaCount],
                 void *                          pAreaData[areaCount])
{
    //TODO: check whether the retain flag (etc..) is as expected...

    TEST_ASSERT(areaCount == 2,("areaCount was %d", areaCount));
    TEST_ASSERT(areaTypes[0] == ismMESSAGE_AREA_PROPERTIES,("areaTypes[0] was %d", areaTypes[0]));
    TEST_ASSERT(areaTypes[1] == ismMESSAGE_AREA_PAYLOAD,("areaTypes[1] was %d", areaTypes[1]));

    if (areaLengths[1] != 1+strlen(expectedMsg->payload))
    {
        printf("Client %s: Incorrect Message length\n",
                  clientId);
        printf("Message Expected (Length, inc null terminator: %d): %s\n",
                (int)(1+strlen(expectedMsg->payload)), expectedMsg->payload);
        printf("Message Arrived  (Length: %d): %s\n",
                (int)areaLengths[1], (char *)pAreaData[1]);
        TEST_ASSERT(false, ("Incorrect message length"));
    }

    if(strncmp(pAreaData[1], expectedMsg->payload, areaLengths[1]) != 0)
    {
        printf("Client %s: Incorrect Message\n",
               clientId);
        printf("Message Expected (Length, inc null terminator: %d): %s\n",
                (int)(1+strlen(expectedMsg->payload)), expectedMsg->payload);
        printf("Message Arrived  (Length: %d): %s\n",
                (int)(areaLengths[1]), (char *)pAreaData[1]);
        TEST_ASSERT(false, ("Incorrect message length"));
    }
    test_log(testLOGLEVEL_TESTPROGRESS, "Client %s, received message %s\n",
            clientId,  expectedMsg->payload);
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
        void *                          pConsumerContext,
        ismEngine_DelivererContext_t *  _delivererContext )
{
    subsContext_t *ctxt = *((subsContext_t **)pConsumerContext);

    if(ctxt->numMsgsArrived >= ctxt->numMsgsExpected)
    {
        printf("Client %s: Got more messages than expected\n", ctxt->clientId);
        TEST_ASSERT(false, ("Client %s: Got more messages than expected (%d msgs instead of %d expected)\n",
                ctxt->clientId, ctxt->numMsgsArrived, ctxt->numMsgsExpected));
    }
    test_MsgDescription *expectedMsg = ctxt->expectedMsgs[ctxt->numMsgsArrived];

    compareMsgs(ctxt->clientId,
                expectedMsg, pMsgDetails,
                             areaCount,
                             areaTypes,
                             areaLengths,
                             pAreaData);

    ctxt->numMsgsArrived++;

    if (state != ismMESSAGE_STATE_CONSUMED)
    {
        int32_t rc = ism_engine_confirmMessageDelivery(ctxt->hSession,
                                          NULL,
                                          hDelivery,
                                          ismENGINE_CONFIRM_OPTION_CONSUMED,
                                          NULL, 0, NULL);
        TEST_ASSERT(rc == OK || rc == ISMRC_AsyncCompletion, ("rc was %d", rc));
    }
    ism_engine_releaseMessage(hMessage);

    //If we have a mutex to unlock...unlock it
    if(ctxt->pMutex)
    {
        pthread_mutex_unlock(ctxt->pMutex);
    }

    return true; //We'd like more messages
}

void reopenSubsCallback(
        ismEngine_SubscriptionHandle_t subHandle,
        const char * pSubName,
        const char * pTopicString,
        void * properties,
        size_t propertiesLength,
        const ismEngine_SubscriptionAttributes_t *pSubAttributess,
        uint32_t consumerCount,
        void * pContext)
{
    DEBUG_ONLY int32_t rc;
    subsContext_t *pSubsContext = (subsContext_t *) pContext;

    rc = ism_engine_createConsumer(
            pSubsContext->hSession,
            ismDESTINATION_SUBSCRIPTION,
            pSubName,
            NULL,
            NULL, // Owning client same as session client
            &pSubsContext,
            sizeof(subsContext_t *),
            messageArrivedCallback,
            NULL,
            test_getDefaultConsumerOptions(pSubAttributess->subOptions),
            &(pSubsContext->hConsumer),
            NULL,
            0,
            NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

}
void verifyMessages( const char *clientId
                   , int numMessages
                   , test_MsgDescription *MsgsExpected[numMessages])
{
    ismEngine_ClientStateHandle_t hClient;
    DEBUG_ONLY int32_t rc;
    subsContext_t reopenSubsContxt = {0};

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutexattr_t mutexattrs;

    /* Ensure that we have defined behaviour in the case of relocking a mutex */
    pthread_mutexattr_init(&mutexattrs);
    pthread_mutexattr_settype(&mutexattrs, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&mutex, &mutexattrs);


    //Set up the context information used my the message received callback
    reopenSubsContxt.clientId = clientId;
    reopenSubsContxt.numMsgsExpected = numMessages;
    reopenSubsContxt.expectedMsgs = MsgsExpected;
    reopenSubsContxt.pMutex = &mutex;

    rc = sync_ism_engine_createClientState(clientId,
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL, NULL,
                                           &hClient);
    TEST_ASSERT(rc == ISMRC_ResumedClientState, ("rc was %d", rc));

    rc = ism_engine_createSession( hClient
                                   , ismENGINE_CREATE_SESSION_OPTION_NONE
                                   , &(reopenSubsContxt.hSession)
                                   , NULL
                                   , 0
                                   , NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    rc = ism_engine_listSubscriptions(
            hClient,
            NULL,
            &reopenSubsContxt,
            reopenSubsCallback);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    if (numMessages > 0)
    {
        //If we expect to receive messages, lock the mutex... it will be unlocked when the last expected message arrives
        pthread_mutex_lock(&mutex);
    }
    rc = ism_engine_startMessageDelivery(
            reopenSubsContxt.hSession,
            ismENGINE_START_DELIVERY_OPTION_NONE,
            NULL,
            0,
            NULL);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));

    //lock the mutex, this will only succeed when the expected messages have arrived...
    pthread_mutex_lock(&mutex);
    pthread_mutex_unlock(&mutex);

    //Wait 200ms =  to check for unexpected messages
    usleep(200*1000);

    rc = sync_ism_engine_destroyClientState(hClient, 0);
    TEST_ASSERT(rc == OK, ("rc was %d", rc));
}

void test4_check(void)
{
    test_MsgDescription *MsgsExpectedSuccessful[4] = { &(MsgsPut[1])
                                                     , &(MsgsPut[2])
                                                     , &(MsgsPut[3])
                                                     , &(MsgsPut[0])}; //Expect the will Message (msg 0) to come last

    verifyMessages("successfulClient1", 4, MsgsExpectedSuccessful);
    test_log(testLOGLEVEL_CHECK, "Checked messages for client successfulClient1");

    verifyMessages("successfulClient2", 4, MsgsExpectedSuccessful);
    test_log(testLOGLEVEL_CHECK, "Checked messages for client successfulClient2");

    verifyMessages("badClient", 0, NULL); //No messages as he won't even have a subscription
    test_log(testLOGLEVEL_CHECK, "Checked messages for client badClient");

    setupSubber(true, 2,
                "NewClient1",
                "NewSub1",
                "/testMixedRestart/#");

    test_MsgDescription *MsgsExpectedNew[1] = { &(MsgsPut[2]) }; //Only the retained message should be given to a new client
    verifyMessages("NewClient1", 1, MsgsExpectedNew);
    test_log(testLOGLEVEL_CHECK, "Checked messages for client newClient");
}


int32_t parseArgs( int argc
                 , char *argv[]
                 , uint32_t *pphase
                 , bool *prestart
                 , testLogLevel_t *pverboseLevel
                 , int *ptrclvl
                 , char **padminDir )
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "recover", 0, NULL, 'r' },
        { "noRestart", 0, NULL, 'n' },
        { "verbose", 1, NULL, 'v' },
        { "adminDir", 1, NULL, 'a' },
        { NULL, 1, NULL, 0 } };
    int long_index;

    *pverboseLevel = 3;
    *pphase = 1;
    *prestart = true;
    *ptrclvl = 0;
    *padminDir = NULL;

    while ((opt = getopt_long(argc, argv, ":rnv:a:0123456789", long_options, &long_index)) != -1)
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
            case 'r':
                *pphase = 2;
                break;
            case 'n':
               *prestart = false;
               break;
            case 'v':
               *pverboseLevel = (testLogLevel_t)atoi(optarg);
               break;
            case 'a':
               *padminDir = optarg;
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
        fprintf(stderr, "Usage: %s [-r] [-n] [-v 0-9] [-0...-9]\n", argv[0]);
        fprintf(stderr, "       -r (--recover)   - Run only recovery phase\n");
        fprintf(stderr, "       -n (--noRestart) - Run only create phase\n");
        fprintf(stderr, "       -v (--verbose)   - Test Case verbosity\n");
        fprintf(stderr, "       -a (--adminDir)  - Admin directory to use\n");
        fprintf(stderr, "       -0 .. -9         - ISM Trace level\n");
    }

   return usage;
}

/*********************************************************************/
/* NAME:        testMixedRestart                                     */
/* DESCRIPTION: This program is a rudimentary check that  store      */
/*              recovery works as expected                           */
/*                                                                   */
/*              Each test runs in 2 phases                           */
/*              Phase 1. The records are created in the store        */
/*              Phase 2. At the end of Phase 1, the program re-execs */
/*                       itself and store recover is called to       */
/*                       reload, then the state is checked           */
/*                                                                   */
/* USAGE:       test_queue3 [-r]                                     */
/*                  -r : When -r is not specified the program runs   */
/*                       Phase 1 and defines and loads the queue(s). */
/*                       When -r is specified, the program runs      */
/*                       Phase 2 and recovers the queues and messages*/
/*                       from the store and verifies everything has  */
/*                       been loaded correctly.                      */
/*********************************************************************/

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int rc = 0;
    char *newargv[20];
    int i;
    uint32_t phase = 1;
    bool restart = true;
    testLogLevel_t testLogLevel = testLOGLEVEL_TESTDESC;
    char *adminDir = NULL;

    if (argc != 1)
    {
        /*************************************************************/
        /* Parse arguments                                           */
        /*************************************************************/
        rc = parseArgs( argc
                      , argv
                      , &phase
                      , &restart
                      , &testLogLevel
                      , &trclvl
                      , &adminDir );
        if (rc != 0)
        {
            goto mod_exit;
        }
    }


    /*****************************************************************/
    /* Process Initialise                                            */
    /*****************************************************************/
    test_setLogLevel(testLogLevel);

    rc = test_processInit(trclvl, adminDir);
    if (rc != OK)
    {
        goto mod_exit;
    }

    // Pick up the admin directory used for this run
    char localAdminDir[1024];
    if (adminDir == NULL && test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir)) == true)
    {
        adminDir = localAdminDir;
    }

    if (phase == 1)
    {
        // Prepare the command-line that will do the recovery
        TEST_ASSERT(argc < 8, ("argc was %d", argc));

        newargv[0] = argv[0];
        newargv[1] = "-r";
        newargv[2] = "-a";
        newargv[3] = adminDir;
        for (i = 1; i <= argc; i++)
        {
            newargv[i + 3] = argv[i];
        }

        // Start up the Engine - Cold Start
        test_log(testLOGLEVEL_TESTDESC, "Starting engine - cold start");
        rc = test_engineInit(true, true, \
                ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION, \
                false, /*recovery should complete ok*/
                ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY, \
                50); //50mb store - tiny so we can change gens easily

        if (rc != 0)
        {
            goto mod_exit;
        }


        //Prepare the store records for recovery
        test1_setup();
        test2_setup();
        test3_setup();
        test4_setup();

        //Now re-exec ourselves to do the recovery
        if (restart)
        {
           test_log(testLOGLEVEL_TESTDESC, "Restarting...");
           rc = test_execv(newargv[0], newargv);
           TEST_ASSERT(false, ("'execv' failed. rc = %d", rc));
        }
    }

    if (phase == 2)
    {
        // Start up the Engine - Warm Start
        test_log(testLOGLEVEL_TESTDESC, "Starting engine - warm start");

        rc = test_engineInit(false,  true,
                             ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                             false, /*recovery should complete ok*/
                             ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                             50); //50mb store - tiny so we can change gens easily

        TEST_ASSERT(rc == OK, ("'test_engineInit' failed. rc = %d", rc));

        //Check that we can reconnect and find the messages we expect
        //test1_check(); <- if restart works then test1 passes...
        //test2_check(); <- if restart works then test2 passes...
        //test3_check(); <- if restart works then test3 passes...
        test4_check();

        rc = test_engineTerm(false);
        TEST_ASSERT( (rc == OK),("'test_engineTerm' failed. rc = %d", rc) );
        rc = test_processTerm(false);
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

