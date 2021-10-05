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
/* Module Name: test_awkwardQDelete.c                                */
/*                                                                   */
/* Description: Main source file for CUnit test of deleting queues.  */
/* Intention is to have a mix of deterministic and more random       */
/* multi-threaded tests                                              */
/*                                                                   */
/*********************************************************************/
#include <sys/prctl.h>

#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_client.h"
#include "test_utils_initterm.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_preload.h"
#include "test_utils_task.h"

#include "engine.h"
#include "engineInternal.h"
#include "queueNamespace.h"

#include "intermediateQInternal.h" //So we can dig around with usecounts...
#include "multiConsumerQInternal.h" //...in the queues
#include "topicTree.h"              //and subscriptions
#include "engineMonitoring.h"

/*******************************************************************/
/* First some "Deterministic" tests                                */
/*******************************************************************/

void test_PutDeleteCommit(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ProducerHandle_t hProducer=NULL;
    ismEngine_TransactionHandle_t hTran=NULL;


    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    /* Create our clients and sessions */
    rc = test_createClientAndSession("SomeClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Create the queue
    char *qName = "test_PutDeleteCommit";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server, NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   qName,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer);


    ismEngine_MessageHandle_t hMessage;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_QUEUE, qName,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    rc = sync_ism_engine_createLocalTransaction(hSession, &hTran);
    TEST_ASSERT_EQUAL(rc, OK);


    rc = sync_ism_engine_putMessage(hSession,
                                    hProducer,
                                    hTran,
                                    hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    //Destroy producer, note we haven't committed the transaction yet
    rc = ism_engine_destroyProducer(hProducer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //The producer is gone so we should be able to delete the queue, discarding messages...
    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    //Now try and commit the put and check it's ok...
    rc = sync_ism_engine_commitTransaction(hSession,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}


static uint32_t consumeDeleteRollbackMsgsRcvd = 0;

bool ConsumeDeleteRollbackCB(
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
    ismEngine_TransactionHandle_t hTran= *(ismEngine_TransactionHandle_t *)pConsumerContext;

    int32_t rc = ism_engine_confirmMessageDelivery( ((ismEngine_Consumer_t *)hConsumer)->pSession
                                                  , hTran, hDelivery, ismENGINE_CONFIRM_OPTION_CONSUMED, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_engine_releaseMessage(hMessage);
    consumeDeleteRollbackMsgsRcvd++;

    return true;
}

void test_ConsumeDeleteRollback(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;
    ismEngine_TransactionHandle_t hTran=NULL;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    /* Create our clients and sessions */
    rc = test_createClientAndSession("ConsumeClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    // Create the queue
    char *qName = "test_ConsumeDeleteCommit";
    rc = ieqn_createQueue(pThreadData,
                          qName,
                          multiConsumer,
                          ismQueueScope_Server, NULL,
                          NULL,
                          NULL,
                          NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    QPreloadMessages(qName, 1);


    rc = sync_ism_engine_createLocalTransaction(hSession, &hTran);
    TEST_ASSERT_EQUAL(rc, OK);


    rc = ism_engine_createConsumer( hSession
                                  , ismDESTINATION_QUEUE
                                  , qName
                                  , ismENGINE_SUBSCRIPTION_OPTION_NONE
                                  , NULL // Unused for QUEUE
                                  , &hTran
                                  , sizeof(hTran)
                                  , ConsumeDeleteRollbackCB
                                  , NULL
                                  , ismENGINE_CONSUMER_OPTION_ACK
                                  , &hConsumer
                                  , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(consumeDeleteRollbackMsgsRcvd, 1);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //The producer is gone so we should be able to delete the queue, discarding messages...
    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);

    //Now try and commit the put and check it's ok...
    rc = ism_engine_rollbackTransaction(hSession, hTran, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}


typedef struct tag_stashedAckData_t {
	ismEngine_DeliveryHandle_t      hDelivery;
} stashedAckData_t;

bool StashHandleForLaterAckCB(
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
	stashedAckData_t *pStashedAckData = *(stashedAckData_t **)pConsumerContext;

	pStashedAckData->hDelivery = hDelivery;

	ism_engine_releaseMessage(hMessage);

	return true;
}

void adminDeleteClientId(char *clientId)
{
    int32_t rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_MQTTCLIENT,
                                           clientId,
                                           NULL,
                                           ISM_CONFIG_CHANGE_DELETE);
    TEST_ASSERT_EQUAL(rc, OK);
}

void adminDeleteSub(char *clientId, char *subName)
{
    ism_prop_t *subProps = ism_common_newProperties(2);
    char *objectidentifier = "BOB"; //Unused by engine for delete sub
    char subNamePropName[strlen(ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_SUBSCRIPTIONNAME)+strlen(objectidentifier)+1];
    char clientIdPropName[strlen(ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_CLIENTID)+strlen(objectidentifier)+1];
    ism_field_t f;

    sprintf(subNamePropName,"%s%s", ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_SUBSCRIPTIONNAME,
                                    objectidentifier);
    f.type = VT_String;
    f.val.s = subName;
    ism_common_setProperty(subProps, subNamePropName, &f);

    sprintf(clientIdPropName,"%s%s", ismENGINE_ADMIN_PREFIX_SUBSCRIPTION_CLIENTID,
                                     objectidentifier);
    f.type = VT_String;
    f.val.s = clientId;
    ism_common_setProperty(subProps, clientIdPropName, &f);

    int32_t rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_SUBSCRIPTION,
                                           objectidentifier,
                                           subProps,
                                           ISM_CONFIG_CHANGE_DELETE);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_common_freeProperties(subProps);
}

//Check usecount on queue is correct when deleting/disconnecting with acks in flight
//If everAck is false, the ack never arrives....
void test_LateMQTTAck( bool durable
                     , bool shared
                     , bool destroyConsumer
                     , bool adminDelSubConnected
                     , bool doUnsub
                     , bool everAck
                     , bool adminDelSubDisconnected
                     , bool adminDelClient
                     , bool doRehydrate
                     )
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ClientStateHandle_t hSharedClient=NULL;
    ismEngine_SessionHandle_t hSharedSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;
    stashedAckData_t stashedAckData = {0};
    stashedAckData_t *pStashedAckData = &stashedAckData;
#ifdef ENGINE_FORCE_INTERMEDIATE_TO_MULTI
    iemqQueue_t *q = NULL;
#else
    ieiqQueue_t *q = NULL;
#endif
    iemqQueue_t *sharedq = NULL;
    char *ConsumeClientId="ConsumeClient";
    char *SharedClientId="TEST_SHARED";



    char *topicString = "/topic/test_LateMQTTAck";
    char *subName     = "test_LateMQTTAck_Sub";
    test_log(testLOGLEVEL_TESTNAME, "Starting %s... %s, %s, %s, %s, %s, %s, %s, %s, %s\n", __func__,
               durable              ? "Durable"     : "Non-durable",
               shared               ? "Shared"      : "Non-shared",
               destroyConsumer      ? ""            : "NoDestroyConsumer",
               adminDelSubConnected ? "admin delete sub whilst connected" : "",
               doUnsub              ? "Unsub"       : "NoUnsub",
               everAck              ? "Late-Ack"    : "Never-Ack",
               adminDelSubDisconnected ? "admin delete sub whilst not connected" : "",
               adminDelClient          ? "admin delete of client" : "",
               doRehydrate             ? "rehydrate" : "");


    /* Create our clients and sessions */
    rc = test_createClientAndSession(ConsumeClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    if (shared)
    {
        rc = test_createClientAndSession(SharedClientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hSharedClient, &hSharedSession, true);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hSharedClient);
        TEST_ASSERT_PTR_NOT_NULL(hSharedSession);
    }

    //Create the subscription if we need to do it explicitly
    ismEngine_SubscriptionAttributes_t subAttrs = {ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE};
    if (durable||shared)
    {
        if (durable)
        {
            subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
        }

        if (shared)
        {
            subAttrs.subOptions |= (ismENGINE_SUBSCRIPTION_OPTION_SHARED|
                                    ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT);
        }
        rc = sync_ism_engine_createSubscription(
                hClient,
                subName,
                NULL,
                ismDESTINATION_TOPIC,
                topicString,
                &subAttrs,
                (shared) ? hSharedClient : NULL); // If not shared Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Create the consumer
    rc = ism_engine_createConsumer( hSession
            , ((durable||shared)  ? ismDESTINATION_SUBSCRIPTION : ismDESTINATION_TOPIC)
            , ((durable||shared) ? subName : topicString)
            , ((durable||shared) ? NULL : &subAttrs)
            , (shared) ? hSharedClient : NULL
            , &pStashedAckData
            , sizeof(pStashedAckData)
            , StashHandleForLaterAckCB
            , NULL
            , test_getDefaultConsumerOptions(subAttrs.subOptions)
            , &hConsumer
            , NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    if (doRehydrate)
    {
        ismEngine_Subscription_t *pSub = ((ismEngine_Subscription_t *)((ismEngine_Consumer_t *)hConsumer)->engineObject);

        //This will stop the sub being removed from the store (it will get marked deleted though)
        iett_acquireSubscriptionReference(pSub);
    }
    else
    {
        //Get a handle on the queue and increase its use-count so we can look at
        //it when it should be deleted
        if (shared)
        {
            sharedq = (iemqQueue_t *)((ismEngine_Consumer_t *)hConsumer)->queueHandle;
            __sync_fetch_and_add(&(sharedq->preDeleteCount), 1);
        }
        else
        {
#ifdef ENGINE_FORCE_INTERMEDIATE_TO_MULTI
            q = (iemqQueue_t *)((ismEngine_Consumer_t *)hConsumer)->queueHandle;
#else
            q = (ieiqQueue_t *)((ismEngine_Consumer_t *)hConsumer)->queueHandle;
#endif
            __sync_fetch_and_add(&(q->preDeleteCount), 1);
        }
    }

    //Put a message
    ismEngine_MessageHandle_t hMessage;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            (doRehydrate) ? ismMESSAGE_PERSISTENCE_PERSISTENT
                                    : ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, topicString,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
    		ismDESTINATION_TOPIC,
            topicString,
            NULL,
            hMessage);

    TEST_ASSERT_EQUAL(rc, OK);

    if (destroyConsumer)
    {
        //Delete Consumer
        rc = sync_ism_engine_destroyConsumer(hConsumer);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    if (adminDelSubConnected)
    {
        adminDeleteSub(shared ? SharedClientId : ConsumeClientId, subName);
    }

    if (doUnsub)
    {
        //Delete sub if it exists
        if (durable||shared)
        {
            rc = ism_engine_destroySubscription(hClient, subName, shared ? hSharedClient : hClient,
                                                NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    if (everAck)
    {
        //Ack the message
        rc = ism_engine_confirmMessageDelivery( hSession
                                              , NULL
                                              , stashedAckData.hDelivery
                                              , ismENGINE_CONFIRM_OPTION_CONSUMED
                                              , NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    if (adminDelSubDisconnected)
    {
        adminDeleteSub(shared ? SharedClientId : ConsumeClientId, subName);
    }
    if (adminDelClient)
    {
        adminDeleteClientId(ConsumeClientId);
    }

    if (shared)
    {
        rc = test_destroyClientAndSession(hSharedClient, hSharedSession, false);
        TEST_ASSERT_EQUAL(rc, OK);
    }




    if (doRehydrate)
    {
        test_bounceEngine();
        iemnMessagingStatistics_t overallStats;

        ieutThreadData_t *pThreadData = ieut_getThreadData();

        iemn_getMessagingStatistics(pThreadData, &overallStats);
        TEST_ASSERT_EQUAL(overallStats.externalStats.BufferedMessages, 0);

    }
    else
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();
        //Because we artificially increase the usecount on the queue we can take a look...:
        if (shared)
        {
            TEST_ASSERT_EQUAL(sharedq->preDeleteCount, 1);
            ieq_reducePreDeleteCount(pThreadData, (ismQHandle_t)sharedq);

        }
        else
        {
#ifndef ENGINE_FORCE_INTERMEDIATE_TO_MULTI
            //For multiconsumer queue we don't update inflightDeqs once the queue is deleted...
            TEST_ASSERT_EQUAL(q->inflightDeqs, 0); //forgotten when disconnecting after sub deletion
#endif
            TEST_ASSERT_EQUAL(q->preDeleteCount, 1);
            ieq_reducePreDeleteCount(pThreadData, (ismQHandle_t)q);
        }
    }
#ifdef ENGINE_FORCE_INTERMEDIATE_TO_MULTI
    //Adminly deleting sub when the subqueue is a multiconsumer queue does not get rid of MDRs for it
    //(as multiconsumerq is not a full replacement for intermediateq in this repect so we get rid of all mdrs manually
    if (adminDelSubConnected || adminDelSubDisconnected)
    {
        rc = test_createClientAndSession(ConsumeClientId,
                                         NULL,
                                         ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                         &hClient, &hSession, true);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hClient);

        rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
        TEST_ASSERT_EQUAL(rc, OK);
    }
#endif

}

void test_LateMQTTAckNonDurable(void)
{
	test_LateMQTTAck( false /*durable*/
                    , false /*shared*/
                    , true  /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , true  /*doUnsub*/
                    , true  /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/);
}
void test_LateMQTTAckNonDurableShared(void)
{
    test_LateMQTTAck( false /*durable*/
                    , true /*shared*/
                    , true  /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , true  /*doUnsub*/
                    , true  /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/);
}
void test_LateMQTTAckDurable(void)
{
	test_LateMQTTAck( true  /*durable*/
                    , false /*shared*/
                    , true  /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , true  /*doUnsub*/
                    , true  /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/);
}
void test_LateMQTTAckDurableShared(void)
{
    test_LateMQTTAck( true  /*durable*/
                    , true  /*shared*/
                    , true  /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , true  /*doUnsub*/
                    , true  /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/);
}
void test_NoMQTTAckNonDurable(void)
{
    test_LateMQTTAck( false /*durable*/
                    , false /*shared*/
                    , true  /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , true  /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/);
}

void test_NoMQTTAckNonDurableShared(void)
{
    test_LateMQTTAck( false /*durable*/
                    , true  /*shared*/
                    , true  /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , true  /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/);
}

void test_NoMQTTAckDurable(void)
{
    test_LateMQTTAck( true  /*durable*/
                    , false /*shared*/
                    , true  /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , true  /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/ );
}

void test_NoMQTTAckDurableShared(void)
{
    test_LateMQTTAck( true  /*durable*/
                    , true  /*shared*/
                    , true  /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , true  /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/ );
}

void test_DisconnectNonDurableInflight(void)
{
    //If we disconnect with inflight messages to
    //a non-durable sub that we haven't unsubbed from
    //the queue should be deleted
    test_LateMQTTAck( false /*durable*/
                    , false /*shared*/
                    , false /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , false /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/ );
}

void test_DisconnectNonDurableInflightShared(void)
{
    //If we disconnect with inflight messages to
    //a non-durable sub that we haven't unsubbed from
    //the queue should be deleted
    test_LateMQTTAck( false /*durable*/
                    , true /*shared*/
                    , false /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , false /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/ );
}
void test_AdminDeleteMQTTClientWithInflight(void)
{
    //Adminly delete client with inflight messages
    test_LateMQTTAck( true  /*durable*/
                    , false /*shared*/
                    , false /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , false /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , true  /*adminDelClient*/
                    , false /*dorehydrate*/ );
}

void test_AdminDeleteMQTTClientWithInflightShared(void)
{
    //Adminly delete client with inflight messages
    test_LateMQTTAck( true  /*durable*/
                    , true  /*shared*/
                    , false /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , false /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , true  /*adminDelClient*/
                    , false /*dorehydrate*/ );
}

void test_AdminDeleteMQTTSubWithInflight(void)
{
    //Adminly delete client with inflight messages
    test_LateMQTTAck( true  /*durable*/
                    , false /*shared*/
                    , false /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , false /*doUnsub*/
                    , false /*everAck*/
                    , true  /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/ );
}

void test_AdminDeleteMQTTSubWithInflightShrd(void)
{
    //Adminly delete client with inflight messages
    test_LateMQTTAck( true  /*durable*/
                    , true  /*shared*/
                    , false /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , false /*doUnsub*/
                    , false /*everAck*/
                    , true  /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/);
}

void test_AdminDeleteMQTTSubConnectedWithInflight(void)
{
    //Adminly delete client with inflight messages who is
    //connected but has no consumer on the sub
    test_LateMQTTAck( true  /*durable*/
                    , false /*shared*/
                    , true  /*destroyConsumer*/
                    , true  /*adminDelSubConnected*/
                    , false /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/ );
}

void test_AdminDeleteMQTTSubConnectedWithInflightShrd(void)
{
    //Adminly delete client with inflight messages who is
    //connected but has no consumer on the sub
    test_LateMQTTAck( true  /*durable*/
                    , true  /*shared*/
                    , true  /*destroyConsumer*/
                    , true  /*adminDelSubConnected*/
                    , false /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , false /*dorehydrate*/ );
}

void test_RehydrateInflightDeleted(void)
{
    //Mark a subscription with inflight acks deleted in the store and check it's gone away
    //after a restart
    test_LateMQTTAck( true  /*durable*/
                    , false /*shared*/
                    , true  /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , true  /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , true  /*dorehydrate*/ );
}

void test_RehydrateInflightDeletedShrd(void)
{
    //Mark a subscription with inflight acks deleted in the store and check it's gone away
    //after a restart
    test_LateMQTTAck( true  /*durable*/
                    , true /*shared*/
                    , true  /*destroyConsumer*/
                    , false /*adminDelSubConnected*/
                    , true  /*doUnsub*/
                    , false /*everAck*/
                    , false /*adminDelSubDisconnected*/
                    , false /*adminDelClient*/
                    , true  /*dorehydrate*/ );
}
CU_TestInfo ISM_NamedQueues_CUnit_test_LateMQTTAck[] =
{
    { "LateMQTTAckNonDurable", test_LateMQTTAckNonDurable },
    { "LateMQTTAckDurable",    test_LateMQTTAckDurable    },
    { "NoMQTTAckNonDurable",   test_NoMQTTAckNonDurable   },
    { "NoMQTTAckDurable",      test_NoMQTTAckDurable      },
    { "DisconnectNonDurableInflight", test_DisconnectNonDurableInflight },
    { "AdminDeleteMQTTClientInflight", test_AdminDeleteMQTTClientWithInflight },
    { "AdminDeleteMQTTSubInflight", test_AdminDeleteMQTTSubWithInflight },
    { "AdminDeleteMQTTSubConnectedWithInflight", test_AdminDeleteMQTTSubConnectedWithInflight },
    { "LateMQTTAckNonDurableShrd", test_LateMQTTAckNonDurableShared },
    { "LateMQTTAckDurableShrd",    test_LateMQTTAckDurableShared    },
    { "NoMQTTAckNonDurableShrd",   test_NoMQTTAckNonDurableShared   },
    { "NoMQTTAckDurableShrd",      test_NoMQTTAckDurableShared      },
    { "DisconnectNonDurableInflightShrd", test_DisconnectNonDurableInflightShared },
    { "AdminDeleteMQTTClientInflightShrd", test_AdminDeleteMQTTClientWithInflightShared },
    { "AdminDeleteMQTTSubWithInflightShrd", test_AdminDeleteMQTTSubWithInflight },
    { "AdminDeleteMQTTSubConnectedWithInflightShrd", test_AdminDeleteMQTTSubConnectedWithInflightShrd },
    { "RehydrateInflightDeleted", test_RehydrateInflightDeleted },
    { "RehydrateInflightDeleted", test_RehydrateInflightDeletedShrd },
    CU_TEST_INFO_NULL
};

//If a JMS sub is adminly deleted after the JMS client has gone, can we reconnect as MQTT (i.e.
//do we still remember the JMS client after there is no reason to)
void test_rememberClientAfterAdminDeleteSub( void )
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;


    char *TestClientId="RememberClient";
    char *topicString = "/topic/test_doWeRemember/theclient";
    char *subName     = "RememberClient_Sub";
    test_log(testLOGLEVEL_TESTNAME, "Starting %s... \n", __func__);


    /* Create our clients and sessions...nondurable client as we are mimicing JMS */
    rc = test_createClientAndSessionWithProtocol(TestClientId,
                                                 PROTOCOL_ID_JMS,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    //Create a durable subscription
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_DURABLE };

    rc = sync_ism_engine_createSubscription(
            hClient,
            subName,
            NULL,
            ismDESTINATION_TOPIC,
            topicString,
            &subAttrs,
            NULL);// Not shared Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    //Now the client has disconnected, administrative delete the subscription
    adminDeleteSub(TestClientId, subName);

    //Check we can connect with a different protocol id...
    rc = test_createClientAndSessionWithProtocol(TestClientId,
                                                 PROTOCOL_ID_MQTT,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_NONE |
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                                 ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                 &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK); //Woo - all traces of the old client are gone (good!)
}

CU_TestInfo ISM_NamedQueues_CUnit_test_Deterministic[] =
{
    { "PutDeleteCommit",                    test_PutDeleteCommit },
    { "ConsumeDeleteRollback",              test_ConsumeDeleteRollback },
    { "rememberClientAfterAdminDeleteSub",  test_rememberClientAfterAdminDeleteSub},
    CU_TEST_INFO_NULL
};

/*******************************************************************/
/* Next a "random" test:                                           */
/*            Create a queue                                       */
/*            Kick off some threads that do puts, gets etc...      */
/*            Repeatedly try and delete the queue until it goes    */
/*            Check everything is handled gracefully.              */
/*            Rinse and repeat                                     */
/*******************************************************************/
uint64_t ThreadId = 0;

typedef enum tag_actionTypes_t {
    actionNone,
    actionPutThenCommit,
    actionPut,
    actionGetThenAck,
    actionGetThenTranAck,
//    actionGetThenTranAckRollback,
    actionNumTypes  //<Leave as last action type
} actionTypes_t;

typedef struct tag_actionThreadContext_t {
    pthread_t threadId;
    char *queueName;
    actionTypes_t type;
    int *threadSetupCompletedCounter;   ///< If this is not null, we should should decrement to say we are ready for delete
    pthread_cond_t *setup_cond;         ///< used to broadcast a change to threadSetupCompletedCounter
    pthread_mutex_t *setup_mutex;       ///< used to protect setup_cond,threadSetupCompletedCounter
    uint32_t numMsgs;                   ///< Number of messages to put or get
    bool canEndGetBeforeAck;            ///< For gets, can we destroy consumer when the msgs have been received or do we wait until they are acked.
    uint32_t extraConsumeOptions;       ///< Extra options when creating a consumer
} actionThreadContext_t;

typedef struct tag_rcvdMsgDetails_t {
    struct tag_rcvdMsgDetails_t *next;
    ismEngine_DeliveryHandle_t hDelivery; ///< Set by the callback so the processing thread can ack it
    ismEngine_MessageHandle_t  hMessage; ///<Set by the callback so the processing thread can release it
    ismMessageState_t  msgState;  ///< Set by the callback so the processing thread knows whether to ack it
} rcvdMsgDetails_t;

typedef struct tag_receiveMsgContext_t {
    ismEngine_SessionHandle_t hSession; ///< Used to ack messages by processor thread
    rcvdMsgDetails_t *firstMsg;   ///< Head pointer for list of received messages
    rcvdMsgDetails_t *lastMsg;   ///< Tail pointer for list of received messages
    pthread_mutex_t msgListMutex; ///< Used to protect the received message list
    pthread_cond_t msgListCond;  ///< This used to broadcast when there are messages in the list to process
    uint32_t msgsToReceive; ///< How many more messages do we want to be given from the queue
    uint32_t msgsToProcess; ///< How many messages does our processing thread need to act on before exiting...
} receiveMsgContext_t;

void initialiseActionsContexts( int numActionsPerDelete
                              , actionThreadContext_t threadContexts[numActionsPerDelete]
                              , char *qName
                              , pthread_mutex_t *setup_mutex
                              , pthread_cond_t *setup_cond)
{
    int os_rc = pthread_cond_init(setup_cond, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);

    os_rc = pthread_mutex_init(setup_mutex, NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);

    for (int i = 0; i < numActionsPerDelete; i++)
    {
        threadContexts[i].queueName = qName;
        threadContexts[i].setup_cond  = setup_cond;
        threadContexts[i].setup_mutex = setup_mutex;
    }
}

void chooseActions( char *queueName
                  , int numActionsPerDelete
                  , actionThreadContext_t threadContexts[numActionsPerDelete]
                  , int *pThreadsDoingSetup )
{
    *pThreadsDoingSetup = 0;
    int preLoadedMessages = 0;

    for (int i = 0; i < numActionsPerDelete; i++)
    {

        threadContexts[i].type = (actionTypes_t)(rand() % actionNumTypes);
        threadContexts[i].threadSetupCompletedCounter = NULL;
        threadContexts[i].numMsgs = 1;
        threadContexts[i].canEndGetBeforeAck = true;
        threadContexts[i].extraConsumeOptions = 0;

        //Only some actions have specified work they can do before the deleting begins (that ought to block it)
        //If we have any such type, then we make the first one do work before deletion and randomly decide for the others

        if (   (threadContexts[i].type != actionNone)
            && ((*pThreadsDoingSetup == 0)||(rand() % 2 == 0)))
        {
            threadContexts[i].threadSetupCompletedCounter = pThreadsDoingSetup;
            (*pThreadsDoingSetup)++;
        }

        //Some actions require a message preloaded on the queue...
        if (   (threadContexts[i].type == actionGetThenAck)
            || (threadContexts[i].type == actionGetThenTranAck))
        {
            preLoadedMessages++;
        }
    }

    if (preLoadedMessages > 0)
    {
        QPreloadMessages(queueName, preLoadedMessages);
    }
}


void threadSetupCompleted( actionThreadContext_t *pThreadContext)
{
    int os_rc =0;

    if(pThreadContext->threadSetupCompletedCounter != NULL)
    {
        os_rc = pthread_mutex_lock(pThreadContext->setup_mutex);
        TEST_ASSERT_EQUAL(os_rc, 0);

        *(pThreadContext->threadSetupCompletedCounter) -= 1;

        os_rc = pthread_cond_broadcast(pThreadContext->setup_cond);
        TEST_ASSERT_EQUAL(os_rc, 0);
    }

    os_rc = pthread_mutex_unlock(pThreadContext->setup_mutex);
    TEST_ASSERT_EQUAL(os_rc, 0);
}

void waitForActionsSetup ( int *pThreadsDoingSetup
                         , pthread_mutex_t *setup_mutex
                         , pthread_cond_t *setup_cond)
{
    int os_rc = pthread_mutex_lock(setup_mutex);
    TEST_ASSERT_EQUAL(os_rc, 0);

    while (*pThreadsDoingSetup > 0)
    {
        os_rc = pthread_cond_wait(setup_cond, setup_mutex);
        TEST_ASSERT_EQUAL(os_rc, 0);
    }

   os_rc = pthread_mutex_unlock(setup_mutex);
   TEST_ASSERT_EQUAL(os_rc, 0);
}

void joinActionThreads( int numActionsPerDelete
                       , actionThreadContext_t threadContexts[numActionsPerDelete])
{
    for (int i = 0; i < numActionsPerDelete; i++)
    {
        if (threadContexts[i].type != actionNone)
        {
            int os_rc = pthread_join(threadContexts[i].threadId, NULL);
            TEST_ASSERT_EQUAL(os_rc, 0);
        }
    }
}

void *putCommitThread(void *arg)
{
    char tname[20];
    sprintf(tname, "PCT-%lu", __sync_add_and_fetch(&ThreadId, 1));
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    actionThreadContext_t *pThreadContext = (actionThreadContext_t *)arg;

    int32_t rc = OK;

    ism_engine_threadInit(0);

    /* Create our clients and sessions */
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ProducerHandle_t hProducer=NULL;
    ismEngine_TransactionHandle_t hTran=NULL;

    rc = test_createClientAndSession(tname,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    rc = sync_ism_engine_createLocalTransaction(hSession, &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   pThreadContext->queueName,
                                   &hProducer,
                                   NULL, 0, NULL);

    if (pThreadContext->threadSetupCompletedCounter != NULL)
    {
        //This is setup, and the controlling thread is waiting for our setup to complete so it should work
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hProducer);
    }
    else
    {
        if (rc == ISMRC_DestNotValid)
        {
            //The queue was deleted before we created our producer... not much we can do
            test_log(testLOGLEVEL_VERBOSE, "queue deleted before a (nonprotected) setup completed in %s", __func__);
            hProducer = NULL;
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hProducer);
        }
    }

    //If we were a thread with a protected setup, say we have completed setup
    if (pThreadContext->threadSetupCompletedCounter != NULL)
    {
        threadSetupCompleted(pThreadContext);
    }

    if (hProducer != NULL)
    {
        uint32_t actionsRemaining = 0;
        uint32_t *pActionsRemaining = &actionsRemaining;

        test_incrementActionsRemaining(pActionsRemaining, pThreadContext->numMsgs);
        for (uint32_t i = 0; i < pThreadContext->numMsgs; i++)
        {
            ismEngine_MessageHandle_t hMessage;

            rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                    ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                    ismMESSAGE_FLAGS_NONE,
                                    0,
                                    ismDESTINATION_QUEUE, pThreadContext->queueName,
                                    &hMessage, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_putMessage(hSession,
                                       hProducer,
                                       hTran,
                                       hMessage,
                                       &pActionsRemaining,
                                       sizeof(pActionsRemaining),
                                       test_decrementActionsRemaining);
            if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }

        test_waitForRemainingActions(pActionsRemaining);

        //Destroy producer, note we haven't committed the transaction yet
        rc = ism_engine_destroyProducer(hProducer, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Now try and commit the put and check it's ok...
    rc = sync_ism_engine_commitTransaction(hSession,
                                      hTran,
                                      ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_engine_threadTerm(1);

    return NULL;
}

void *putThread(void *arg)
{
    actionThreadContext_t *pThreadContext = (actionThreadContext_t *)arg;

    char tname[20];
    sprintf(tname, "P-%lu", __sync_add_and_fetch(&ThreadId, 1));
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    int32_t rc = OK;

    ism_engine_threadInit(0);

    /* Create our clients and sessions */
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ProducerHandle_t hProducer=NULL;

    rc = test_createClientAndSession(tname,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   pThreadContext->queueName,
                                   &hProducer,
                                   NULL, 0, NULL);

    if (pThreadContext->threadSetupCompletedCounter != NULL)
    {
        //This is setup, and the controlling thread is waiting for our setup to complete so it should work
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hProducer);
    }
    else
    {
        if (rc == ISMRC_DestNotValid)
        {
            //The queue was deleted before we created our producer... not much we can do
            test_log(testLOGLEVEL_VERBOSE, "queue deleted before a (nonprotected) setup completed in %s", __func__);
            hProducer = NULL;
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hProducer);
        }
    }

    //If we were a thread with a protected setup, say we have completed setup
    if (pThreadContext->threadSetupCompletedCounter != NULL)
    {
        threadSetupCompleted(pThreadContext);
    }

    if (hProducer != NULL)
    {
        uint32_t actionsRemaining = 0;
        uint32_t *pActionsRemaining = &actionsRemaining;

        for (uint32_t i = 0; i < pThreadContext->numMsgs; i++)
        {
            ismEngine_MessageHandle_t hMessage;

            test_incrementActionsRemaining(pActionsRemaining, pThreadContext->numMsgs);
            rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                    ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                    ismMESSAGE_FLAGS_NONE,
                                    0,
                                    ismDESTINATION_QUEUE, pThreadContext->queueName,
                                    &hMessage, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_putMessage(hSession,
                                       hProducer,
                                       NULL,
                                       hMessage,
                                       &pActionsRemaining,
                                       sizeof(pActionsRemaining),
                                       test_decrementActionsRemaining);
            if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
            else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }

        test_waitForRemainingActions(pActionsRemaining);

        //Destroy producer, note we haven't committed the transaction yet
        rc = ism_engine_destroyProducer(hProducer, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_engine_threadTerm(1);

    return NULL;
}



bool receivedMsgCallback(
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
    bool moreMsgsWanted = false;

    receiveMsgContext_t *context = *(receiveMsgContext_t **)pConsumerContext;

    TEST_ASSERT_EQUAL(state, ismMESSAGE_STATE_DELIVERED);


    //Copy the details we want from the message into some memory....
    rcvdMsgDetails_t *msgDetails = malloc(sizeof(rcvdMsgDetails_t));
    TEST_ASSERT_PTR_NOT_NULL(msgDetails);

    msgDetails->hDelivery = hDelivery;
    msgDetails->hMessage  = hMessage;
    msgDetails->msgState  = state;
    msgDetails->next      = NULL;

    //Add it to the list of messages....
    int32_t os_rc = pthread_mutex_lock(&(context->msgListMutex));
    TEST_ASSERT_EQUAL(os_rc, 0);

    if(context->firstMsg == NULL)
    {
        context->firstMsg = msgDetails;
        TEST_ASSERT_EQUAL(context->lastMsg, NULL);
        context->lastMsg = msgDetails;
    }
    else
    {
        TEST_ASSERT_PTR_NOT_NULL(context->lastMsg);
        context->lastMsg->next = msgDetails;
        context->lastMsg       = msgDetails;
    }

    //Broadcast that we've added some messages to the list
    os_rc = pthread_cond_broadcast(&(context->msgListCond));
    TEST_ASSERT_EQUAL(os_rc, 0);

    //Let the processing thread access the list of messages...
    os_rc = pthread_mutex_unlock(&(context->msgListMutex));
    TEST_ASSERT_EQUAL(os_rc, 0);

    context->msgsToReceive--;

    if(context->msgsToReceive > 0)
    {
        moreMsgsWanted = true;
    }

    return moreMsgsWanted;
}

//Acks/releases messages
void *getProcessorThread(void *arg)
{
    char tname[20];
    sprintf(tname, "GP-%lu", __sync_add_and_fetch(&ThreadId, 1));
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    ism_engine_threadInit(0);

    receiveMsgContext_t *context = (receiveMsgContext_t *)arg;

    while(context->msgsToProcess > 0)
    {
        int os_rc = pthread_mutex_lock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        while (context->firstMsg == NULL)
        {
            os_rc = pthread_cond_wait(&(context->msgListCond), &(context->msgListMutex));
            TEST_ASSERT_EQUAL(os_rc, 0);
        }

        TEST_ASSERT_PTR_NOT_NULL(context->firstMsg);
        rcvdMsgDetails_t *msg = context->firstMsg;

        context->firstMsg = msg->next;
        if(context->lastMsg == msg)
        {
            TEST_ASSERT_EQUAL(msg->next, NULL);
            context->lastMsg = NULL;
        }

        os_rc = pthread_mutex_unlock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        ism_engine_releaseMessage(msg->hMessage);

        if (msg->msgState != ismMESSAGE_STATE_CONSUMED)
        {

            int rc = ism_engine_confirmMessageDelivery(context->hSession,
                                              NULL,
                                              msg->hDelivery,
                                              ismENGINE_CONFIRM_OPTION_CONSUMED,
                                              NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        free(msg);

        context->msgsToProcess--;
    }

    ism_engine_threadTerm(1);

    return NULL;
}

//releases messages and acks them in a transaction
void *getThenTranAckProcessorThread(void *arg)
{
    char tname[20];
    sprintf(tname, "GTAP-%lu", __sync_add_and_fetch(&ThreadId, 1));
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    int32_t rc = OK;

    ism_engine_threadInit(0);

    receiveMsgContext_t *context = (receiveMsgContext_t *)arg;

    while(context->msgsToProcess > 0)
    {
        int os_rc = pthread_mutex_lock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        while (context->firstMsg == NULL)
        {
            os_rc = pthread_cond_wait(&(context->msgListCond), &(context->msgListMutex));
            TEST_ASSERT_EQUAL(os_rc, 0);
        }

        TEST_ASSERT_PTR_NOT_NULL(context->firstMsg);
        rcvdMsgDetails_t *msg = context->firstMsg;

        context->firstMsg = msg->next;
        if(context->lastMsg == msg)
        {
            TEST_ASSERT_EQUAL(msg->next, NULL);
            context->lastMsg = NULL;
        }

        os_rc = pthread_mutex_unlock(&(context->msgListMutex));
        TEST_ASSERT_EQUAL(os_rc, 0);

        ism_engine_releaseMessage(msg->hMessage);

        if (msg->msgState != ismMESSAGE_STATE_CONSUMED)
        {
            ismEngine_TransactionHandle_t hTran = NULL;

            rc = sync_ism_engine_createLocalTransaction(NULL, &hTran);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                   hTran,
                                                   msg->hDelivery,
                                                   ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                   NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = sync_ism_engine_commitTransaction(NULL, hTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        free(msg);

        context->msgsToProcess--;
    }

    ism_engine_threadTerm(1);

    return NULL;
}

void startGetProcessingThread( actionThreadContext_t *pThreadContext
		                     , receiveMsgContext_t *pRMC
		                     , pthread_t *pProcessingThreadId)
{
    /* Create a thread to process messages (acks etc...) */
	int os_rc;

    switch (pThreadContext->type)
    {
        case actionGetThenAck:
            os_rc = test_task_startThread(pProcessingThreadId,getProcessorThread, pRMC,"getProcessorThread");
            TEST_ASSERT_EQUAL(os_rc, 0);
            break;

        case actionGetThenTranAck:
            os_rc = test_task_startThread(pProcessingThreadId,getThenTranAckProcessorThread, pRMC,"getThenTranAckProcessorThread");
            TEST_ASSERT_EQUAL(os_rc, 0);
            break;
        default:
            TEST_ASSERT(0, ("pThreadContext->type is %d", pThreadContext->type));
    }
}

void UnlockMutex(int32_t                         retcode,
	             void *                          handle,
	             void *                          pContext)
{
	pthread_mutex_t *pMutex = *(pthread_mutex_t **)pContext;
	int os_rc = pthread_mutex_unlock(pMutex);
	TEST_ASSERT_EQUAL(os_rc, 0);
}

void *getControllerThread(void *arg)
{
    char tname[20];
    sprintf(tname, "GCT-%lu", __sync_add_and_fetch(&ThreadId, 1));
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&tname);

    actionThreadContext_t *pThreadContext = (actionThreadContext_t *)arg;

    int32_t rc = OK;

    ism_engine_threadInit(0);

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ConsumerHandle_t hConsumer=NULL;
    receiveMsgContext_t rMC = {0};
    receiveMsgContext_t *pRMC = &rMC;
    pthread_t processingThreadId;
    int os_rc;

    pthread_mutexattr_t mutexattrs;
    pthread_mutex_t delayMutex;
    pthread_mutex_t *pDelayMutex = &delayMutex;
    os_rc = pthread_mutexattr_init(&mutexattrs);
    TEST_ASSERT_EQUAL(os_rc, 0);
    os_rc = pthread_mutexattr_settype(&mutexattrs, PTHREAD_MUTEX_NORMAL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    os_rc = pthread_mutex_init(pDelayMutex, &mutexattrs);
    TEST_ASSERT_EQUAL(os_rc, 0);


    /* Create our clients and sessions */
    rc = test_createClientAndSession(tname,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);


    /* Init the consumer context */
    os_rc = pthread_mutex_init(&(rMC.msgListMutex), NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);
    os_rc = pthread_cond_init(&(rMC.msgListCond), NULL);
    TEST_ASSERT_EQUAL(os_rc, 0);

    rMC.msgsToReceive = pThreadContext->numMsgs;
    rMC.msgsToProcess = rMC.msgsToReceive;
    rMC.hSession  = hSession;
    rMC.firstMsg  = NULL;
    rMC.lastMsg   = NULL;

    /* And then the consumer */
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_QUEUE,
                                   pThreadContext->queueName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pRMC,
                                   sizeof(pRMC),
                                   receivedMsgCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK|pThreadContext->extraConsumeOptions,
                                   &hConsumer,
                                   NULL, 0, NULL);

    if (pThreadContext->threadSetupCompletedCounter != NULL)
    {
        //This is setup, and the controlling thread is waiting for our setup to complete so it should work
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer);
    }
    else
    {
        if (rc == ISMRC_DestNotValid)
        {
            //The queue was deleted before we created our producer... not much we can do
            test_log(testLOGLEVEL_VERBOSE, "queue deleted before a (nonprotected) setup completed in %s", __func__);
            hConsumer = NULL;
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hConsumer);
        }
    }

    //If we were a thread with a protected setup, say we have completed setup
    if (pThreadContext->threadSetupCompletedCounter != NULL)
    {
        threadSetupCompleted(pThreadContext);
    }

    if (hConsumer != NULL)
    {
    	startGetProcessingThread(pThreadContext, pRMC, &processingThreadId);

        rc = ism_engine_startMessageDelivery(hSession,0, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        if (pThreadContext->canEndGetBeforeAck)
        {
            //We need to wait until the messages have been received but not necessarily acked...
            while (rMC.msgsToReceive > 0)
            {
                usleep(5000);
            }
        }
        else
        {
            //When the ack thread finishes all the acks are complete
            os_rc = pthread_join(processingThreadId, NULL);
            TEST_ASSERT_EQUAL(os_rc, 0);
        }

        TEST_ASSERT_EQUAL(rMC.msgsToReceive, 0);

        //This can validly return asynchronous as the last message callback can still
        //be active even though it has reduced the number of remaining messages...if so just wait...
        os_rc = pthread_mutex_lock(pDelayMutex);
        TEST_ASSERT_EQUAL(os_rc, 0);

        rc = ism_engine_destroyConsumer(hConsumer, &pDelayMutex, sizeof(pDelayMutex), UnlockMutex);
        if (rc == OK)
        {
        	os_rc = pthread_mutex_unlock(pDelayMutex);
        	TEST_ASSERT_EQUAL(os_rc, 0);
        }
        else if (rc == ISMRC_AsyncCompletion)
        {
        	//Lock won't complete until aync callback called
        	os_rc = pthread_mutex_lock(pDelayMutex);
        	TEST_ASSERT_EQUAL(os_rc, 0);
        	os_rc = pthread_mutex_unlock(pDelayMutex);
        	TEST_ASSERT_EQUAL(os_rc, 0);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }

		if (pThreadContext->canEndGetBeforeAck)
        {
		    //Finally wait for the acking thread to finish
		    os_rc = pthread_join(processingThreadId, NULL);
		    TEST_ASSERT_EQUAL(os_rc, 0);
        }
    }


    os_rc = pthread_mutex_lock(pDelayMutex);
    TEST_ASSERT_EQUAL(os_rc, 0);

    rc = ism_engine_destroySession(hSession,
                                   &pDelayMutex, sizeof(pDelayMutex), UnlockMutex);
    if (rc == OK)
    {
        os_rc = pthread_mutex_unlock(pDelayMutex);
        TEST_ASSERT_EQUAL(os_rc, 0);
    }
    else if (rc == ISMRC_AsyncCompletion)
    {
        //Lock won't complete until aync callback called
        os_rc = pthread_mutex_lock(pDelayMutex);
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_mutex_unlock(pDelayMutex);
        TEST_ASSERT_EQUAL(os_rc, 0);
    }
    else
    {
        TEST_ASSERT_EQUAL(rc, OK);
    }

    os_rc = pthread_mutex_lock(pDelayMutex);
    TEST_ASSERT_EQUAL(os_rc, 0);

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pDelayMutex, sizeof(pDelayMutex), UnlockMutex);

    if (rc == OK)
    {
        os_rc = pthread_mutex_unlock(pDelayMutex);
        TEST_ASSERT_EQUAL(os_rc, 0);
    }
    else if (rc == ISMRC_AsyncCompletion)
    {
        //Lock won't complete until aync callback called
        os_rc = pthread_mutex_lock(pDelayMutex);
        TEST_ASSERT_EQUAL(os_rc, 0);
        os_rc = pthread_mutex_unlock(pDelayMutex);
        TEST_ASSERT_EQUAL(os_rc, 0);
    }
    else
    {
        TEST_ASSERT_EQUAL(rc, OK);
    }

    os_rc = pthread_mutex_destroy(pDelayMutex);
    TEST_ASSERT_EQUAL(os_rc, 0);

    ism_engine_threadTerm(1);

    return NULL;
}

void startActionThreads( int numActionsPerDelete
                       , actionThreadContext_t threadContexts[numActionsPerDelete])
{
    int os_rc = 0;

    for (int i = 0; i < numActionsPerDelete; i++)
    {
        switch (threadContexts[i].type)
        {
        case actionNone:
            //No action required
            break;

        case actionPut:
            os_rc = test_task_startThread(&(threadContexts[i].threadId),putThread, &(threadContexts[i]),"putThread");
            TEST_ASSERT_EQUAL(os_rc, 0);
            break;

        case actionPutThenCommit:
            os_rc = test_task_startThread(&(threadContexts[i].threadId),putCommitThread, &(threadContexts[i]),"putCommitThread");
            TEST_ASSERT_EQUAL(os_rc, 0);
            break;

        //All the get function use the same getcontroller function:
        case actionGetThenAck:
        case actionGetThenTranAck:
            os_rc = test_task_startThread(&(threadContexts[i].threadId),getControllerThread, &(threadContexts[i]),"getControllerThread");
            TEST_ASSERT_EQUAL(os_rc, 0);
            break;

        default:
            TEST_ASSERT(0, ("threadContexts[i].type is %d", threadContexts[i].type));
            break;
        }
    }
}


void test_MultiThreadActions(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int numDeletes=800;
    int numActionsPerDelete=4;
    int rc = OK;
    int threadsDoingSetup = 0;
    int os_rc = 0;
    char *qName= "test_MultiThreadActions";

    pthread_cond_t setup_cond;
    pthread_mutex_t setup_mutex;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    actionThreadContext_t threadContexts[numActionsPerDelete];
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    /* Create a client+session for this (main) thread */
    rc = test_createClientAndSession("MainThreadClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    initialiseActionsContexts(numActionsPerDelete, threadContexts,
            qName,  &setup_mutex, &setup_cond);

    for (int i = 0; i < numDeletes; i++)
    {
        rc = ieqn_createQueue(pThreadData, qName, multiConsumer, ismQueueScope_Server, NULL, NULL, NULL, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        chooseActions(qName, numActionsPerDelete, threadContexts, &threadsDoingSetup);

		//Log with a lower level periodically so it doesn't look like we've hung...
        test_log((i%128 == 0) ? 2: testLOGLEVEL_TESTPROGRESS
                , "Iteration %u has %u threads doing work before delete\n", i, threadsDoingSetup);

        startActionThreads(numActionsPerDelete, threadContexts);

        //wait until any action threads that want to get going before we start deleting
        waitForActionsSetup(&threadsDoingSetup, &setup_mutex, &setup_cond);

        do
        {
            rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
        }
        while (rc == ISMRC_DestinationInUse);

        TEST_ASSERT_EQUAL(rc, OK);

        joinActionThreads(numActionsPerDelete, threadContexts);
    }
    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    os_rc = pthread_cond_destroy(&setup_cond);
    TEST_ASSERT_EQUAL(os_rc, 0);

    os_rc = pthread_mutex_destroy(&setup_mutex);
    TEST_ASSERT_EQUAL(os_rc, 0);
}


//Not a delete test... just check that we can stream messages from a queue when we are
//acking async-ly so may hit the limit of messages in-flight
//Only a single consumer as this is for intermediateQ...
void test_flowFromQ(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int numQueueLoops=3;
    int rc = OK;
    int threadsDoingSetup = 0;
    int os_rc = 0;
    char *qName= "test_flowfromQ";

    pthread_cond_t setup_cond;
    pthread_mutex_t setup_mutex;
    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    actionThreadContext_t threadContext;
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    // Set default maxMessageCount to 0 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;

    /* Create a client+session for this (main) thread */
    rc = test_createClientAndSession("MainThreadClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    initialiseActionsContexts(1, &threadContext, qName,  &setup_mutex, &setup_cond);

    for (int i = 0; i < numQueueLoops; i++)
    {
        threadsDoingSetup = 0;

        rc = ieqn_createQueue(pThreadData, qName, intermediate, ismQueueScope_Server, NULL, NULL, NULL, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        threadContext.type = actionGetThenAck;
        threadsDoingSetup = 1;
        threadContext.threadSetupCompletedCounter = &threadsDoingSetup;
        threadContext.numMsgs = 100000;
        threadContext.canEndGetBeforeAck = true;
        threadContext.extraConsumeOptions = ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID;

        QPreloadMessages(qName, threadContext.numMsgs);

        startActionThreads(1, &threadContext);

        //wait until any action threads...as this test doesn't delete the queue until all threads completed, this is not necessary.
        waitForActionsSetup(&threadsDoingSetup, &setup_mutex, &setup_cond);

        joinActionThreads(1, &threadContext);

        rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    os_rc = pthread_cond_destroy(&setup_cond);
    TEST_ASSERT_EQUAL(os_rc, 0);

    os_rc = pthread_mutex_destroy(&setup_mutex);
    TEST_ASSERT_EQUAL(os_rc, 0);
}

/***********************************************************************************************************
 * Start of functions types etc. for test_destroyConsumer
 */

static bool consumerDestroyCompleted = false;

typedef enum tag_TestDestroyConsumer_State_t {
    testDestroyConsumer_setup, testDestroyConsumer_running
} TestDestroyConsumer_State_t;

typedef struct tag_TestDisable_ThreadInfo_t {
    uint32_t putterNum;
    pthread_t thread_id;
    uint32_t *MsgsReceived;
    uint32_t numIterations;
    uint32_t *currentIteration;
    uint32_t *puttersInProgress;
    pthread_cond_t *startCond;
    pthread_mutex_t *startMutex;
    pthread_cond_t *stopCond;
    pthread_mutex_t *stopMutex;
    TestDestroyConsumer_State_t *state;
    bool *stopPutting;
    char *topicString;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t **pphConsumer;
} TestDestroyConsumer_ThreadInfo_t;

static uint32_t illegalMessagesReceived = 0;


//Putting thread for testDestroyConsumer
static void *TestDestroyConsumerPutter(void *threadarg)
{
    TestDestroyConsumer_ThreadInfo_t *putterInfo = (TestDestroyConsumer_ThreadInfo_t *)threadarg;
    int32_t rc;

    int iteration;
    uint32_t msgsPerBlock = 50; //We put this many messages and see if the waiter
                                //has been disabled, if not we try again

    char ThreadName[16];
    sprintf(ThreadName, "DP(%d)", putterInfo->putterNum);
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&ThreadName);

    ism_engine_threadInit(0);

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ProducerHandle_t hProducer;

    rc = test_createClientAndSession(ThreadName,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_TOPIC,
                                   putterInfo->topicString,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    for (iteration = 0; iteration < putterInfo->numIterations; iteration++)
    {
        //Wait to be told to start
        rc = pthread_mutex_lock(putterInfo->startMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        while (  ( *(putterInfo->currentIteration) < iteration)
               ||( *(putterInfo->state) == testDestroyConsumer_setup) )
        {
            rc = pthread_cond_wait(putterInfo->startCond, putterInfo->startMutex);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        rc = pthread_mutex_unlock(putterInfo->startMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        //OK...let's fire bunches of messages until the waiter is disabled
        do
        {
            int i;
            for (i=0; i < msgsPerBlock; i++)
            {
                ismEngine_MessageHandle_t hMsg;

                rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                        ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                        ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                        ismMESSAGE_FLAGS_NONE,
                                        0,
                                        ismDESTINATION_TOPIC, putterInfo->topicString,
                                        &hMsg, NULL);
                TEST_ASSERT_EQUAL(rc, OK);

                rc = ism_engine_putMessage(hSession,
                                           hProducer,
                                           NULL,
                                           hMsg,
                                           NULL, 0, NULL);
                if (rc != OK) TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
            }
        }
        while(!(*putterInfo->stopPutting)); /* BEAM suppression: infinite loop */

        uint32_t puttersRemaining = __sync_sub_and_fetch( putterInfo->puttersInProgress, 1);

        if (puttersRemaining == 0)
        {
            //If we were the last putter in the iteration, declare the iteration done
            //broadcast start publishing to the threads
            rc = pthread_mutex_lock(putterInfo->stopMutex);
            TEST_ASSERT_EQUAL(rc, OK);
            *(putterInfo->state) = testDestroyConsumer_setup;
            rc = pthread_cond_broadcast(putterInfo->stopCond);
            TEST_ASSERT_EQUAL(rc, OK);
            rc = pthread_mutex_unlock(putterInfo->stopMutex);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    return NULL;
}


static bool MsgArrivedTestDestroyConsumer(
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
    TestDestroyConsumer_ThreadInfo_t *pDestroyerInfo = *(TestDestroyConsumer_ThreadInfo_t **)pConsumerContext;
    uint32_t *msgsReceived = pDestroyerInfo->MsgsReceived;
    int32_t rc = OK;

    TEST_ASSERT_EQUAL( consumerDestroyCompleted, false );

    if (state == ismMESSAGE_STATE_DELIVERED)
    {
        rc = ism_engine_confirmMessageDelivery(pDestroyerInfo->hSession,
                                               NULL,
                                               hDelivery,
                                               ismENGINE_CONFIRM_OPTION_CONSUMED,
                                               NULL,
                                               0,
                                               NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    if (consumerDestroyCompleted)
    {
        //We don't know about the threadsafety of CUNIT ASSERTS
        //So increase a count so the main thread can assert too
        __sync_fetch_and_add(&illegalMessagesReceived, 1);
    }
    test_log(testLOGLEVEL_VERBOSE, "Msg %u arrived (%p)\n", *msgsReceived, msgsReceived);

    (void)__sync_fetch_and_add(msgsReceived, 1);
    ism_engine_releaseMessage(hMessage);

    return true;
}

void destroyAsyncCompletion(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext)
{
    test_log(testLOGLEVEL_VERBOSE, "consumer destroyed async...\n");
    TEST_ASSERT_EQUAL(retcode, OK);
    char *threadName = (char *)pContext;

    //Context is thread name of thread which initiated the destroy which is TT(<num>)
    TEST_ASSERT_EQUAL(threadName[0], 'T');
    TEST_ASSERT_EQUAL(threadName[1], 'T');

    consumerDestroyCompleted = true;
}

static void *TestDestroyConsumerDestroyer(void *threadarg)
{
    TestDestroyConsumer_ThreadInfo_t *putterInfo = (TestDestroyConsumer_ThreadInfo_t *)threadarg;
    int32_t rc;

    int iteration;

    int ThreadNameLength=16;
    char ThreadName[ThreadNameLength];
    sprintf(ThreadName, "TT(%d)", putterInfo->putterNum);
    prctl (PR_SET_NAME, (unsigned long)(uintptr_t)&ThreadName);

    ism_engine_threadInit(0);

    for (iteration = 0; iteration < putterInfo->numIterations; iteration++)
    {
        //Wait to be told to start
        rc = pthread_mutex_lock(putterInfo->startMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        while (  ( *(putterInfo->currentIteration) < iteration)
               ||( *(putterInfo->state) == testDestroyConsumer_setup) )
        {
            rc = pthread_cond_wait(putterInfo->startCond, putterInfo->startMutex);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        rc = pthread_mutex_unlock(putterInfo->startMutex);
        TEST_ASSERT_EQUAL(rc, OK);

        //wait till the putters are in progress
        while(*(putterInfo->MsgsReceived) == 0)
        {
            sched_yield();
        }

        test_log(testLOGLEVEL_VERBOSE, "Destroying consumer...\n");
        rc = ism_engine_destroyConsumer(**(putterInfo->pphConsumer), ThreadName, ThreadNameLength, destroyAsyncCompletion);
        TEST_ASSERT_CUNIT((rc == OK || rc == ISMRC_AsyncCompletion ), ("rc was %d, rc"));

        if (rc == OK)
        {
            test_log(testLOGLEVEL_VERBOSE,"consumer destroyed inline...\n");
            consumerDestroyCompleted = true;
        }
    }

    return NULL;
}

//Check we can destroy a consumer whilst messages are flowing

static void testDestroyConsumer(ismQueueType_t qtype)
{
    int32_t rc = OK;

    char TestName[64];
    char qtypeText[16];

    uint32_t numPutters = 3;
    uint32_t numIterations = 200;
    uint32_t puttersInProgress = 0;
    uint32_t messagesReceived = 0;
    uint32_t currentIteration = 0;

    TestDestroyConsumer_ThreadInfo_t putterInfo[numPutters];
    TestDestroyConsumer_ThreadInfo_t destroyerInfo;
    TestDestroyConsumer_ThreadInfo_t *pDestroyerInfo = &destroyerInfo;
    pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t stop_cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t stop_mutex = PTHREAD_MUTEX_INITIALIZER;
    TestDestroyConsumer_State_t state = testDestroyConsumer_setup;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    ismEngine_ConsumerHandle_t *phConsumer;
    test_log(1, "");
    sprintf(TestName, "testDestroyConsumer(%s)",  ieut_queueTypeText(qtype, qtypeText, sizeof(qtypeText)));
    test_log(1, "Starting %s...", TestName);


    // Set default maxMessageCount to 0 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;

    rc = test_createClientAndSession("testDestroyConsumer",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    int i;
    for (i=0; i<numPutters; i++)
    {
        //Set up the info for the putter
        putterInfo[i].putterNum = i;
        putterInfo[i].topicString = TestName;
        putterInfo[i].MsgsReceived = &messagesReceived;
        putterInfo[i].numIterations = numIterations;
        putterInfo[i].hSession = hSession;
        putterInfo[i].pphConsumer = &phConsumer;
        putterInfo[i].currentIteration = &currentIteration;
        putterInfo[i].startCond = &start_cond;
        putterInfo[i].startMutex = &start_mutex;
        putterInfo[i].puttersInProgress = &puttersInProgress;
        putterInfo[i].stopCond = &stop_cond;
        putterInfo[i].stopMutex = &stop_mutex;
        putterInfo[i].state = &state;
        putterInfo[i].stopPutting = &(consumerDestroyCompleted);

        //Start the thread
        rc = test_task_startThread(&(putterInfo[i].thread_id),TestDestroyConsumerPutter, (void *)&(putterInfo[i]),"TestDestroyConsumerPutter");
        TEST_ASSERT_EQUAL(rc, OK);
    }

    //Set up the disabler thread
    destroyerInfo.putterNum = 0;
    destroyerInfo.MsgsReceived = &messagesReceived;
    destroyerInfo.numIterations = numIterations;
    destroyerInfo.currentIteration = &currentIteration;
    destroyerInfo.topicString = TestName;
    destroyerInfo.startCond = &start_cond;
    destroyerInfo.startMutex = &start_mutex;
    destroyerInfo.puttersInProgress = &puttersInProgress;
    destroyerInfo.stopCond = &stop_cond;
    destroyerInfo.stopMutex = &stop_mutex;
    destroyerInfo.state = &state;
    destroyerInfo.pphConsumer = &phConsumer;
    destroyerInfo.hSession = hSession;

    //Start the thread
    rc = test_task_startThread(&(destroyerInfo.thread_id),TestDestroyConsumerDestroyer, (void *)&(destroyerInfo),"TestDestroyConsumerDestroyer");
    TEST_ASSERT_EQUAL(rc, OK);

    for (currentIteration=0; currentIteration < numIterations; currentIteration++)
    {
        test_log(testLOGLEVEL_VERBOSE, "Starting iteration %d\n", currentIteration);

        ismEngine_SubscriptionAttributes_t subAttrs = { (qtype == simple)
                                                         ? ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE
                                                         : ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       TestName,
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &pDestroyerInfo,
                                       sizeof(pDestroyerInfo),
                                       MsgArrivedTestDestroyConsumer,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hConsumer,
                                       NULL, 0, NULL);
        //Reset the count of putters...
        puttersInProgress = numPutters;
        consumerDestroyCompleted = false;
        //Clear the number of messages received
        messagesReceived = 0;
        phConsumer = &hConsumer;

        //broadcast start publishing to the threads
        rc = pthread_mutex_lock(&(start_mutex));
        TEST_ASSERT_EQUAL(rc, OK);
        state = testDestroyConsumer_running;
        rc = pthread_cond_broadcast(&(start_cond));
        TEST_ASSERT_EQUAL(rc, OK);
        rc = pthread_mutex_unlock(&(start_mutex));
        TEST_ASSERT_EQUAL(rc, OK);

        //wait for all the putting threads to finish
        rc = pthread_mutex_lock(&(stop_mutex));
        TEST_ASSERT_EQUAL(rc, OK);

        while (state == testDestroyConsumer_running)
        {
            rc = pthread_cond_wait(&(stop_cond), &(stop_mutex));
            TEST_ASSERT_EQUAL(rc, OK);
        }
        rc = pthread_mutex_unlock(&(stop_mutex));
        TEST_ASSERT_EQUAL(rc, OK);


        //Check the consumer has been successfully destroyed...
        TEST_ASSERT_EQUAL(consumerDestroyCompleted, true);
        TEST_ASSERT_EQUAL(illegalMessagesReceived, 0);
    }

    //Join all the threads
    for (i=0; i<numPutters; i++)
    {
        rc = pthread_join(putterInfo[i].thread_id, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    rc = pthread_join(destroyerInfo.thread_id, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    test_log(3, "Completed %s", TestName);
}

void test_DestroySimpleConsumer(void)
{
    testDestroyConsumer(simple);
}

void test_DestroyIntermediateConsumer(void)
{
    testDestroyConsumer(intermediate);
}


CU_TestInfo ISM_NamedQueues_CUnit_test_Random[] =
{
    { "MultiThreadActions", test_MultiThreadActions },
    { "TestFlowFromQ", test_flowFromQ},
    { "TestDestroySimpleConsumer", test_DestroySimpleConsumer},
    { "TestDestroyIntermediateConsumer", test_DestroyIntermediateConsumer},
    CU_TEST_INFO_NULL
};

int initAwkwardQueues(void)
{
    return test_engineInit(true, true,
                           true, // Disable Auto creation of named queues
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termAwkwardQueues(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_AwkwardQDelete_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Deterministic", initAwkwardQueues, termAwkwardQueues, ISM_NamedQueues_CUnit_test_Deterministic),
    IMA_TEST_SUITE("LateMQTTAck", initAwkwardQueues, termAwkwardQueues, ISM_NamedQueues_CUnit_test_LateMQTTAck),
    IMA_TEST_SUITE("Random", initAwkwardQueues, termAwkwardQueues, ISM_NamedQueues_CUnit_test_Random),
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
    int testLogLevel = 2;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_AwkwardQDelete_CUnit_allsuites);

    if (retval == 0)
    {
        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();

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
