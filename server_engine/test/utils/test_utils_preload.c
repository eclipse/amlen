/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
#include "engine.h"
#include "engineInternal.h"

#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"
#include "test_utils_client.h"

static void messagePreloaded(int32_t rc, void *handle, void *pContext)
{
   TEST_ASSERT(rc == OK, ("rc was %d", rc));
   volatile uint64_t *pNumMsgsSent = *(uint64_t **)pContext;

   ASYNCPUT_CB_ADD_AND_FETCH(pNumMsgsSent, 1);
}

void QPreloadMessages( char *queueName
                     , int numMessages)
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient=NULL;
    ismEngine_SessionHandle_t hSession=NULL;
    ismEngine_ProducerHandle_t hProducer=NULL;
    volatile uint64_t numMsgsSent=0;
    volatile uint64_t *pNumMsgsSent = &numMsgsSent;

    rc = test_createClientAndSession("PreloadClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    rc = ism_engine_createProducer(hSession,
                                   ismDESTINATION_QUEUE,
                                   queueName,
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer);


    for (int i = 0; i < numMessages; i++)
    {
        ismEngine_MessageHandle_t hMessage;

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_QUEUE, queueName,
                                &hMessage, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessage(hSession,
                                   hProducer,
                                   NULL,
                                   hMessage,
                                   &pNumMsgsSent,
                                   sizeof(pNumMsgsSent),
                                   messagePreloaded);
        TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_AsyncCompletion,
                                ("rc was %d\n", rc));

        if (rc == OK)
        {
            ASYNCPUT_CB_ADD_AND_FETCH(pNumMsgsSent, 1);
        }
    }

    //Destroy producer, note we haven't committed the transaction yet
    rc = ism_engine_destroyProducer(hProducer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    while((volatile uint64_t)numMsgsSent < numMessages)
    {
        sched_yield();
    }

}
