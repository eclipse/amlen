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
/* Module Name: test_clientState.c                                   */
/*                                                                   */
/* Description: CUnit tests of Engine client-state functions         */
/*                                                                   */
/*********************************************************************/
#include "test_clientState.h"
#include "test_utils_assert.h"
#include "test_utils_file.h"
#include "test_utils_options.h"
#include "test_utils_initterm.h"
#include "test_utils_sync.h"
#include "test_utils_security.h"

#include "engineStore.h"
#include "clientStateInternal.h"
#include "clientStateExpiry.h"
#include "resourceSetStats.h"

CU_TestInfo ISM_Engine_CUnit_ClientStateBasic[] = {
    { "ClientStateAnonymous",                     clientStateTestAnonymous },
    { "ClientStateDestroyContext",                clientStateDestroyContext },
    { "ClientStateSimpleBounce",                  clientStateTestSimpleBounce },
    { "ClientStateSimpleSteal",                   clientStateTestSimpleSteal },
    { "ClientStateStealDuringDelivery",           clientStateTestStealDuringDelivery },
    { "ClientStateDoubleStealLazy",               clientStateTestDoubleStealLazy },
    { "ClientStateDoubleStealEager",              clientStateTestDoubleStealEager },
    { "ClientStateZombieCleanStart",              clientStateTestZombieCleanStart },
    { "ClientStateBasicExpiry",                   clientStateTestBasicExpiry },
    { "ClientStateUnreleased",                    clientStateTestUnreleased },
    { "ClientStateSimpleDurable",                 clientStateTestSimpleDurable },
    { "ClientStateCleanSession0",                 clientStateTestCleanSession0 },
    { "ClientStateCleanSession1",                 clientStateTestCleanSession1 },
    { "ClientStateDurableToDurable",              clientStateTestDurableToDurable },
    { "ClientStateNondurableToDurable",           clientStateTestNondurableToDurable },
    { "ClientStateDurableToNondurable",           clientStateTestDurableToNondurable },
    { "ClientStateMassive",                       clientStateTestMassive },
    { "ClientStateDurableTran",                   clientStateTestDurableTran },
    { "ClientStateStealDurableSubs",              clientStateTestStealDurableSubs },
    { "ClientStateProtocolMismatch",              clientStateTestProtocolMismatch },
    { "ClientStateUnusualSteals",                 clientStateTestUnusualSteals },
    { "ClientStateForceDiscard",                  clientStateTestForceDiscard },
    { "ClientStateTestNonDurableZombieToDurable", clientStateTestNondurableZombieToDurable },
    { "ClientStateTableTraversal",                clientStateTestClientStateTableTraversal },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Engine_CUnit_ClientStateSecurity[] = {
    { "ClientStateStealRemoveZombie",             clientStateTestStealRemoveZombie },
    { "ClientStateStealCleanStart",               clientStateTestStealCleanStart },
    { "ClientStateInheritDurability",             clientStateTestInheritDurability },
    CU_TEST_INFO_NULL
};

// Run a subset of tests when MQTT id range has been set low.
CU_TestInfo ISM_Engine_CUnit_ClientStateLowIDRange[] = {
    { "ClientStateSimpleDurable",       clientStateTestSimpleDurable },
    { "ClientStateUnreleased",          clientStateTestUnreleased },
    { "ClientStateCleanSession1",       clientStateTestCleanSession1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Engine_CUnit_ClientStateHardToReach[] = {
    { "TableResizeDuringRecovery", clientStateTestTableResizeDuringRecovery },
    CU_TEST_INFO_NULL
};

typedef struct callbackContext
{
    int32_t                         MessageCount;
    ismEngine_ClientStateHandle_t * phClient;
} callbackContext_t;


#define UNREL_LIMIT 100

typedef struct unreleasedContext
{
    uint32_t                        Count;
    bool                            Found[UNREL_LIMIT];
    ismEngine_UnreleasedHandle_t    hUnrel[UNREL_LIMIT];
} unreleasedContext_t;

typedef struct durableSubsContext
{
    uint32_t                        Count;
    bool                            expectProperties;
    bool                            Found[UNREL_LIMIT];
} durableSubsContext_t;

static void callbackCreateClientStateComplete(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext);

static void callbackDestroyClientStateComplete(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext);

static bool deliveryCallbackStealClientState(
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

static void clientStateNoStealCallback(
    int32_t                         reason,
    ismEngine_ClientStateHandle_t   hClient,
    uint32_t                        options,
    void *                          pContext);

static void clientStateStealCallback(
    int32_t                         reason,
    ismEngine_ClientStateHandle_t   hClient,
    uint32_t                        options,
    void *                          pContext);

static void unreleasedIdsCallback(
    uint32_t                        deliveryId,
    ismEngine_UnreleasedHandle_t    hUnrel,
    void *                          pContext);

static void durableSubsCallback(
    ismEngine_SubscriptionHandle_t            subHandle,
    const char *                              pSubName,
    const char *                              pTopicString,
    void *                                    properties,
    size_t                                    propertiesLength,
    const ismEngine_SubscriptionAttributes_t *pSubAttributes,
    uint32_t                                  consumerCount,
    void *                                    pContext);

uint32_t override_clientStateExpiryInterval = UINT32_MAX;
int32_t iecs_addClientState(ieutThreadData_t *pThreadData,
                            ismEngine_ClientState_t *pClient,
                            uint32_t options,
                            uint32_t internalOptions,
                            void *pContext,
                            size_t contextLength,
                            ismEngine_CompletionCallback_t pCallbackFn)
{
    int32_t rc;
    static int32_t (*real_iecs_addClientState)(ieutThreadData_t *, ismEngine_ClientState_t *, uint32_t, uint32_t, void *, size_t, ismEngine_CompletionCallback_t) = NULL;

    if (real_iecs_addClientState == NULL)
    {
        real_iecs_addClientState = dlsym(RTLD_NEXT, "iecs_addClientState");
    }

    rc = real_iecs_addClientState(pThreadData, pClient, options, internalOptions, pContext, contextLength, pCallbackFn);

    // Horridly simplistic way to override the expiry interval (which works because
    // nothing currently takes any notice of the value until the clientState is destroyed).
    if (override_clientStateExpiryInterval != UINT32_MAX)
    {
        pClient->ExpiryInterval = override_clientStateExpiryInterval;
    }

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_dumpClientState                              */
/*                                                                   */
/* Description:    Test ism_engine_dumpClientState                   */
/*                                                                   */
/*********************************************************************/
void test_dumpClientState(const char *clientId)
{
    char tempDirectory[500] = {0};

    test_utils_getTempPath(tempDirectory);
    TEST_ASSERT_NOT_EQUAL(tempDirectory[0], 0);

    strcat(tempDirectory, "/test_clientState_dump_XXXXXX");

    if (mkdtemp(tempDirectory) == NULL)
    {
        TEST_ASSERT(false, ("mkdtemp() failed with %d\n", errno));
    }

    char cwdbuf[500];
    char *cwd = getcwd(cwdbuf, sizeof(cwdbuf));
    TEST_ASSERT_PTR_NOT_NULL(cwd);

    if (chdir(tempDirectory) == 0)
    {
        int32_t rc;
        char tempFilename[strlen(tempDirectory)+30];

        sprintf(tempFilename, "%s/dumpClientState_X", tempDirectory);
        rc = ism_engine_dumpClientState(NULL, 1, 0, tempFilename);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotImplemented);

        sprintf(tempFilename, "%s/dumpClientState_1", tempDirectory);
        rc = ism_engine_dumpClientState(clientId, 1, 0, tempFilename);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpClientState_1.dat"));

        sprintf(tempFilename, "%s/dumpClientState_9_0", tempDirectory);
        rc = ism_engine_dumpClientState(clientId, 9, 0, tempFilename);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpClientState_9_0.dat"));

        sprintf(tempFilename, "%s/dumpClientState_9_50", tempDirectory);
        rc = ism_engine_dumpClientState(clientId, 9, 50, tempFilename);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpClientState_9_50.dat"));

        sprintf(tempFilename, "%s/dumpClientState_9_All", tempDirectory);
        rc = ism_engine_dumpClientState(clientId, 9, -1, tempFilename);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpClientState_9_All.dat"));

        if (strcmp(clientId, "MissingClient") != 0)
        {
            sprintf(tempFilename, "%s/dumpClientState_X", tempDirectory);
            rc = ism_engine_dumpClientState("MissingClient", 1, 0, tempFilename);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        }

        if(chdir(cwd) != 0)
        {
            TEST_ASSERT(false, ("chdir() back to old cwd failed with errno %d\n", errno));
        }
    }

    test_removeDirectory(tempDirectory);
}

/*
 * Create anonymous client-states (which are now disallowed)
 */
void clientStateTestAnonymous(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ism_engine_createClientState(NULL,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDRequired);

    rc = ism_engine_createClientState("",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDRequired);
}

/*
 * Test different sized destroy contexts (plus some tests of destroyDisconnectedClientState)
 */
void clientStateDestroyContext(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    // Test use of different sized destroy contexts
    char ContextBuffer[1024] = {'X'};
    for(int32_t i=0; i<11; i++)
    {
        const char *clientId = "ContextBufferTest";

        rc = ism_engine_createClientState(clientId,
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &hClient1,
                                          NULL,
                                          0,
                                          NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Try and destroy it when it's not disconnected
        rc = ism_engine_destroyDisconnectedClientState(clientId, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDConnected);

        rc = ism_engine_destroyClientState(hClient1,
                                           ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                           ContextBuffer,
                                           1 << i,
                                           NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Try and destroy it when it's not there!
        rc = ism_engine_destroyDisconnectedClientState(clientId, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }
}


/*
 * Create client-states making sure that duplicate client ids are bounced
 */
void clientStateTestSimpleBounce(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ClientStateHandle_t hClient1b;
    ismEngine_ClientStateHandle_t hClient2;
    int32_t rc = OK;

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                      NULL, NULL, NULL,
                                      &hClient1b,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDInUse);

    rc = ism_engine_createClientState("Client2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("Client2",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient2,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1b,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDInUse);

    rc = ism_engine_destroyClientState(hClient2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Create client-states making sure that simple stealing of a client id works
 */
void clientStateTestSimpleSteal(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ClientStateHandle_t hClient1b;
    ismEngine_ClientStateHandle_t hClient1c;
    int32_t rc = OK;

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &hClient1,
                                      clientStateStealCallback,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                           &hClient1b,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1b);
    TEST_ASSERT_EQUAL(rc, OK);

    // The steal callback will have cleared this
    TEST_ASSERT_EQUAL(hClient1, NULL);

    // Durable steal from non-durable
    hClient1c = NULL;
    for(int32_t i=0; i<2; i++)
    {
        // Fake up low resources in the store so that one loop will fail to create durable
        ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0] = ((i == 0) ? StatusWarning : StatusOk);

        rc = sync_ism_engine_createClientState("Client1",
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE | ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                               &hClient1c,
                                               clientStateStealCallback,
                                               NULL,
                                               &hClient1c);
        if (i == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_ServerCapacity);
            TEST_ASSERT_PTR_NULL(hClient1c);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hClient1c);
        }
    }

    rc = sync_ism_engine_destroyClientState(hClient1c,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}

//Check that after a takeover +disconnection of a durable client, we can still remove client state
void clientStateTestStealRemoveZombie(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ClientStateHandle_t hClient1b;
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    const char *clientId = "Client1";

    ism_listener_t *mockListener=NULL, *mockListener2=NULL;
    ism_transport_t *mockTransport=NULL, *mockTransport2=NULL;
    ismSecurity_t *mockContext=NULL, *mockContext2=NULL;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = clientId;
    mockTransport->userid = "USER1";

    rc = test_createSecurityContext(&mockListener2,
                                    &mockTransport2,
                                    &mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport2->clientID = clientId;
    mockTransport2->userid = "USER2";

    rc = sync_ism_engine_createClientState(clientId,
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL
                                         | ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                          &hClient1,
                                           clientStateStealCallback,
                                           mockContext,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hClient1->hStoreCSR, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL(hClient1->hStoreCPR, ismSTORE_NULL_HANDLE);

    ismStore_Handle_t origCSR = hClient1->hStoreCSR;
    ismStore_Handle_t origCPR = hClient1->hStoreCPR;

    // NOTE: We're changine the UserID so we expect the CPR to change
    rc = sync_ism_engine_createClientState(clientId,
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL
                                         | ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           &hClient1b,
                                           clientStateStealCallback,
                                           mockContext2,
                                           &hClient1b);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);
    TEST_ASSERT_EQUAL(hClient1b->hStoreCSR, origCSR);
    TEST_ASSERT_NOT_EQUAL(hClient1b->hStoreCPR, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL(hClient1b->hStoreCPR, origCPR);

    // The steal callback will have cleared this
    TEST_ASSERT_EQUAL(hClient1, NULL);

    rc = sync_ism_engine_destroyClientState(hClient1b,
                                            ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    // The destroy should leave a zombie that we should be able to successfully clean up...
    rc = iecs_clientStateConfigCallback(pThreadData, "Client1", NULL, ISM_CONFIG_CHANGE_DELETE);
    TEST_ASSERT_EQUAL(rc, OK);

    test_destroySecurityContext(mockListener, mockTransport, mockContext);
    test_destroySecurityContext(mockListener2, mockTransport2, mockContext2);
}

/*
 * Create client-states making sure that taking a zombie of a client id works as expected with CLEANSTART
 */
void clientStateTestZombieCleanStart(void)
{
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    // Just go through the code which sets maxInflight from mqttMsgIdRange with no
    // security context, and rejects a value of 0...
    uint32_t realMsgIdRange = ismEngine_serverGlobal.mqttMsgIdRange;
    ismEngine_serverGlobal.mqttMsgIdRange = 0;

    // Create using CLEANSTART (just in case there is a client left behind)
    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                           ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                           NULL, NULL,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hClient1->hStoreCSR, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_NOT_EQUAL(hClient1->maxInflightLimit, 0);

    ismEngine_serverGlobal.mqttMsgIdRange = realMsgIdRange; // Back to sanity
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
    rc = sync_ism_engine_createSubscription(hClient1,
                                            "Client1SUB",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "Client1/Topic",
                                            &subAttrs,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the subscription exists
    durableSubsContext_t durableSubsContext = {0, false, {false}};

    rc = ism_engine_listSubscriptions(hClient1,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 1);

    rc = sync_ism_engine_destroyClientState(hClient1, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    // Steal with DURABLE & CLEANSTART
    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                           ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                           NULL, NULL,
                                           NULL,
                                           &hClient2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hClient2->hStoreCSR, ismSTORE_NULL_HANDLE);

    // Check the subscription no longer exists
    durableSubsContext.Count = 0;
    rc = ism_engine_listSubscriptions(hClient2,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 0);

    rc = sync_ism_engine_destroyClientState(hClient2, ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}

/*
 * Create client-states making sure that stealing of a client id works as expected with CLEANSTART
 */
void clientStateTestStealCleanStart(void)
{
    ismStore_Handle_t CSR1;
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);
    const char *clientId = "Client1";

    ism_listener_t *mockListener=NULL, *mockListener2=NULL;
    ism_transport_t *mockTransport=NULL, *mockTransport2=NULL;
    ismSecurity_t *mockContext=NULL, *mockContext2=NULL;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = clientId;

    rc = test_createSecurityContext(&mockListener2,
                                    &mockTransport2,
                                    &mockContext2);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport2->clientID = clientId;

    // Create using CLEANSTART (just in case there is a client left behind)
    ((mock_ismSecurity_t *)mockContext)->ExpectedMsgRate = EXPECTEDMSGRATE_HIGH; // Force high msg rate code path
    rc = sync_ism_engine_createClientState(clientId,
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                           ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                           &hClient1,
                                           clientStateStealCallback,
                                           mockContext,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hClient1->hStoreCSR, ismSTORE_NULL_HANDLE);
    CSR1 = hClient1->hStoreCSR;

    // Add a messaging policy that allows publish & subscribe on Client1/* to Client1
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID.Client1TopicPolicy\","
                                "\"Name\":\"Client1TopicPolicy\","
                                "\"ClientID\":\"Client1\","
                                "\"Topic\":\"Client1/*\","
                                "\"ActionList\":\"publish,subscribe\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with the contexts
    rc = test_addPolicyToSecContext(mockContext, "Client1TopicPolicy");
    TEST_ASSERT_EQUAL(rc, OK);
    rc = test_addPolicyToSecContext(mockContext2, "Client1TopicPolicy");
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
    rc = sync_ism_engine_createSubscription(hClient1,
                                            "Client1SUB",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "Client1/Topic",
                                            &subAttrs,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the subscription exists
    durableSubsContext_t durableSubsContext = {0, false, {false}};

    rc = ism_engine_listSubscriptions(hClient1,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 1);

    // Steal with DURABLE and without CLEANSTART
    ((mock_ismSecurity_t *)mockContext2)->ExpectedMsgRate = EXPECTEDMSGRATE_LOW; // Force low msg rate code path
    rc = sync_ism_engine_createClientState(clientId,
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           &hClient2,
                                           clientStateStealCallback,
                                           mockContext2,
                                           &hClient2);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);
    TEST_ASSERT_EQUAL(hClient2->hStoreCSR, CSR1);

    // The steal callback will have cleared this
    TEST_ASSERT_EQUAL(hClient1, NULL);

    // Check the subscription still exists
    durableSubsContext.Count = 0;
    rc = ism_engine_listSubscriptions(hClient2,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 1);

    // Steal with DURABLE & CLEANSTART
    ((mock_ismSecurity_t *)mockContext)->ExpectedMsgRate = EXPECTEDMSGRATE_MAX; // Force MAX msg rate code path
    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                           ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                           &hClient1,
                                           clientStateStealCallback,
                                           mockContext,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hClient1->hStoreCSR, ismSTORE_NULL_HANDLE);

    // Check the subscription no longer exists
    durableSubsContext.Count = 0;
    rc = ism_engine_listSubscriptions(hClient1,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 0);

    // Steal with CLEANSTART (non-durable)
    uint32_t realMsgIdRange = ismEngine_serverGlobal.mqttMsgIdRange; // Make the global limit 0 (which is unexpected)
    ismEngine_serverGlobal.mqttMsgIdRange = 0;
    ((mock_ismSecurity_t *)mockContext2)->ExpectedMsgRate = 999; // Force invalid msg rate code path...
    ((mock_ismSecurity_t *)mockContext2)->InFlightMsgLimit = 15; // ...and an explicit msg limit

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                           ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                           &hClient2,
                                           clientStateStealCallback,
                                           mockContext2,
                                           &hClient2);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(hClient2->hStoreCSR, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_EQUAL(hClient2->maxInflightLimit, 15);

    ismEngine_serverGlobal.mqttMsgIdRange = realMsgIdRange;

    rc = sync_ism_engine_destroyClientState(hClient2, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    test_destroySecurityContext(mockListener, mockTransport, mockContext);
    test_destroySecurityContext(mockListener2, mockTransport2, mockContext2);
}

/*
 * Create client-states which inherit their durability from the client they stole / took over from
 */
void clientStateTestInheritDurability(void)
{
    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    durableSubsContext_t durableSubsContext = {0, false, {false}};
    ismStore_Handle_t prevCSR;
    int32_t rc;
    uint32_t options;
    const char *clientId = "Client1";

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext=NULL;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    mockTransport->clientID = clientId;

    // Create a stealable durable client
    options = ismENGINE_CREATE_CLIENT_OPTION_INHERIT_DURABILITY |
              ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
              ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL;

    rc = sync_ism_engine_createClientState(clientId,
                                           testDEFAULT_PROTOCOL_ID,
                                           options,
                                           &hClient1,
                                           clientStateStealCallback,
                                           mockContext,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK); // Don't expect it to inherit, but create.
    TEST_ASSERT_NOT_EQUAL(hClient1->hStoreCSR, ismSTORE_NULL_HANDLE);
    TEST_ASSERT_EQUAL(hClient1->Durability, iecsDurable);
    prevCSR = hClient1->hStoreCSR;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
    rc = sync_ism_engine_createSubscription(hClient1,
                                            "Client1SUB",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "Client1/Topic",
                                            &subAttrs,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // DURABLE Loops:
    //
    // 0) No Steal (with INHERIT_DURABILITY without CLEANSTART
    // 1) Steal with INHERIT_DURABILITY without CLEANSTART.
    // 2) Zombie take-over with INHERIT_DURABILITY without CLEANSTART.
    // 3) Steal with INHERIT_DURABILITY and CLEANSTART
    // 4) Zombie take-over with INHERIT_DURABILITY and CLEANSTART
    //
    uint32_t expectedSubsFound = 1;
    for(int32_t i=0; i<5; i++)
    {
        options = ismENGINE_CREATE_CLIENT_OPTION_INHERIT_DURABILITY |
                  ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                  ((i >= 1) ? ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL : 0) |
                  ((i >= 3) ? ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART : 0);

        // Check that the subscription (still) exists
        durableSubsContext.Count = 0;
        rc = ism_engine_listSubscriptions(i < 2 ? hClient1 : hClient2,
                                          NULL,
                                          &durableSubsContext,
                                          durableSubsCallback);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(durableSubsContext.Count, expectedSubsFound);

        // Steal / Take over with INHERIT_DURABILITY (durable if not inherited) - should end up durable
        rc = sync_ism_engine_createClientState(clientId,
                                               testDEFAULT_PROTOCOL_ID,
                                               options,
                                               &hClient2,
                                               clientStateStealCallback,
                                               mockContext,
                                               &hClient2);

        if ((options & ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL) == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDInUse);
        }
        else if ((options & ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART) == 0)
        {
            TEST_ASSERT_EQUAL(rc,  ISMRC_ResumedClientState);
            TEST_ASSERT_EQUAL(hClient2->hStoreCSR, prevCSR);
            TEST_ASSERT_EQUAL(hClient2->Durability, iecsDurable);
        }
        else
        {
            expectedSubsFound = 0;
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_NOT_EQUAL(hClient2->hStoreCSR, ismSTORE_NULL_HANDLE);
            TEST_ASSERT_NOT_EQUAL(hClient2->hStoreCSR, prevCSR);
            TEST_ASSERT_EQUAL(hClient2->Durability, iecsDurable);
            prevCSR = hClient2->hStoreCSR;
        }

        if (i == 1 || i == 3)
        {
            rc = sync_ism_engine_destroyClientState(hClient2, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    // NONDURABLE Loops:
    //
    // 0) Zombie take-over with INHERIT_DURABILITY without CLEANSTART.
    // 1) Steal with INHERIT_DURABILITY and CLEANSTART
    // 2) No Steal (with INHERIT_DURABILITY and CLEANSTART)
    //
    for(int32_t i=0; i<3; i++)
    {
        options = ismENGINE_CREATE_CLIENT_OPTION_INHERIT_DURABILITY |
                  ((i < 2) ? ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL : 0) |
                  ((i >= 1) ? ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART : 0);

        // Steal / Take over with INHERIT_DURABILITY (non-durable if not inherited) - should end up non-durable
        rc = sync_ism_engine_createClientState(clientId,
                                               testDEFAULT_PROTOCOL_ID,
                                               options,
                                               &hClient2,
                                               clientStateStealCallback,
                                               mockContext,
                                               &hClient2);

        if ((options & ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL) == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDInUse);
        }
        else if ((options & ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART) == 0)
        {
            TEST_ASSERT_EQUAL(rc,  ISMRC_ResumedClientState);
            TEST_ASSERT_EQUAL(hClient2->hStoreCSR, prevCSR);
            TEST_ASSERT_EQUAL(hClient2->Durability, iecsDurable);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(hClient2->hStoreCSR, ismSTORE_NULL_HANDLE);
            TEST_ASSERT_EQUAL(hClient2->Durability, iecsNonDurable);
        }
    }

    // Get rid of the client
    rc = sync_ism_engine_destroyClientState(hClient2, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    test_destroySecurityContext(mockListener, mockTransport, mockContext);

    // Check the client is gone.
    uint32_t clientIdHash = iecs_generateClientIdHash(clientId);
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ismEngine_ClientState_t *pFoundClient = iecs_getVictimizedClient(pThreadData, clientId, clientIdHash);
    TEST_ASSERT_EQUAL(pFoundClient, NULL);
}

//Check that a non-durable zombie can be taken over by a durable (ND zombies *could* occur
//in an export/import scenario or if a delayed will message is set)
void clientStateTestNondurableZombieToDurable(void)
{
    ismEngine_ClientState_t *pClient1;
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    iecsNewClientStateInfo_t clientInfo;

    clientInfo.pClientId = "Client1";
    clientInfo.protocolId = testDEFAULT_PROTOCOL_ID;
    clientInfo.durability = iecsNonDurable;
    clientInfo.pUserId = NULL;
    clientInfo.lastConnectedTime = 0;
    clientInfo.expiryInterval = iecsEXPIRY_INTERVAL_INFINITE;
    clientInfo.resourceSet = iecs_getResourceSet(pThreadData, clientInfo.pClientId, clientInfo.protocolId, iereOP_ADD);

    // Cheat to get the ND Zombie, this is effectively what would happen during import
    rc = iecs_newClientStateImport(pThreadData, &clientInfo, &pClient1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(pClient1);

    rc = iecs_addClientState(pThreadData,
                             pClient1,
                             ismENGINE_CREATE_CLIENT_INTERNAL_OPTION_IMPORTING,
                             iecsCREATE_CLIENT_OPTION_NONE,
                             NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    iecs_setExpiryFromLastConnectedTime(pThreadData, pClient1, NULL);

    bool didRelease = iecs_releaseClientStateReference(pThreadData, pClient1, true, false);
    TEST_ASSERT_EQUAL(didRelease, false);

    for(int32_t i=0; i<2; i++)
    {
        // Fake up low resources in the store so that one loop will fail to create durable
        ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0] = ((i == 0) ? StatusWarning : StatusOk);

        rc = sync_ism_engine_createClientState("Client1",
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL
                                             | ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               &hClient1,
                                               clientStateStealCallback,
                                               NULL,
                                               &hClient1);
        if (i == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_ServerCapacity);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK); // Was ISMRC_ResumedClientState);

            subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                  ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
            rc = sync_ism_engine_createSubscription(hClient1,
                                                    "TESTSUB",
                                                    NULL,
                                                    ismDESTINATION_TOPIC,
                                                    "TOPICTEST",
                                                    &subAttrs,
                                                    hClient1);
            TEST_ASSERT_EQUAL(rc, OK);

            durableSubsContext_t durableSubsContext = {0, false, {false}};

            // Check durable subs
            rc = ism_engine_listSubscriptions(hClient1,
                                              NULL,
                                              &durableSubsContext,
                                              durableSubsCallback);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(durableSubsContext.Count, 1);

            // Everything must be destroyed at the end of the test
            rc = sync_ism_engine_destroyClientState(hClient1,
                                                    ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }
}

/*
 * Create client-states making sure that taking a zombie of a client id works as expected with CLEANSTART
 */
void clientStateTestBasicExpiry(void)
{
    ismEngine_ClientStateHandle_t hClient1, hClient2, hClient3, hClient4;
    int32_t rc = OK;
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    // Suspend normal expiry for the duration of this test
    iece_stopClientStateExpiry(pThreadData);

    override_clientStateExpiryInterval = 1; // Set expiry to 1 second

    // Create using CLEANSTART (just in case there is a client left behind)
    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                           ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                           NULL, NULL,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create using CLEANSTART (just in case there is a client left behind)
    rc = sync_ism_engine_createClientState("Client2",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                           ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                           NULL, NULL,
                                           NULL,
                                           &hClient2);
    TEST_ASSERT_EQUAL(rc, OK);

    // Non-durable using will delay
    rc = sync_ism_engine_createClientState("Client3",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                           NULL, NULL,
                                           NULL,
                                           &hClient3);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create using CLEANSTART (just in case there is a client left behind)
    rc = sync_ism_engine_createClientState("Client4",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE |
                                           ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                           NULL, NULL,
                                           NULL,
                                           &hClient4);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_MessageHandle_t hMessage;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengths[2] = {0, 13};
    void *areaData[2] = {NULL, "Message body"};

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClient3,
                                   ismDESTINATION_TOPIC,
                                   "/test/willMsg",
                                   hMessage,
                                   1,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_setWillMessage(hClient4,
                                   ismDESTINATION_TOPIC,
                                   "/test/willMsg",
                                   hMessage,
                                   1,
                                   iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyClientState(hClient1, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hClient1->ExpiryTime, 0);
    TEST_ASSERT_EQUAL(hClient1->WillDelayExpiryTime, 0);

    rc = sync_ism_engine_destroyClientState(hClient2, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hClient2->ExpiryTime, 0);
    TEST_ASSERT_EQUAL(hClient2->WillDelayExpiryTime, 0);

    rc = sync_ism_engine_destroyClientState(hClient3, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hClient3->ExpiryTime, 0);
    TEST_ASSERT_NOT_EQUAL(hClient3->WillDelayExpiryTime, 0);

    rc = sync_ism_engine_destroyClientState(hClient4, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hClient4->ExpiryTime, 0);
    TEST_ASSERT_NOT_EQUAL(hClient4->WillDelayExpiryTime, 0);

    // Wait 2 seconds (so that it is past the expiry time)
    sleep(2);

    override_clientStateExpiryInterval = UINT32_MAX; // No expiry

    // Take over (without explicitly requesting CLEANSTART -- but it ought to happen implicitly)
    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL(hClient1->hStoreCSR, ismSTORE_NULL_HANDLE);

    rc = sync_ism_engine_destroyClientState(hClient1, ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);

    ieceExpiryControl_t *expiryControl = ismEngine_serverGlobal.clientStateExpiryControl;

    uint64_t beforeRestart_scansStarted = expiryControl->scansStarted;

    // Restart clientState expiry
    expiryControl->reaperEndRequested = false;
    rc = iece_startClientStateExpiry(pThreadData);
    TEST_ASSERT_EQUAL(rc, OK);

    // Wait for the scan initiated by the call to iece_startClientStateExpiry to finish
    while(beforeRestart_scansStarted == expiryControl->scansEnded)
    {
        sleep(1);
    }

    ismEngine_ClientState_t *pFoundClient = iecs_getVictimizedClient(pThreadData, "Client2", iecs_generateClientIdHash("Client2"));
    TEST_ASSERT_EQUAL(pFoundClient, NULL);
    pFoundClient = iecs_getVictimizedClient(pThreadData, "Client3", iecs_generateClientIdHash("Client3"));
    TEST_ASSERT_EQUAL(pFoundClient, NULL);
    pFoundClient = iecs_getVictimizedClient(pThreadData, "Client4", iecs_generateClientIdHash("Client4"));
    TEST_ASSERT_EQUAL(pFoundClient, NULL);
}

// Test various ways of traversing the client state table
typedef struct tag_stateTableTraversalContext_t
{
    uint32_t tableGeneration;
    uint32_t callCount;
} stateTableTraversalContext_t;

bool stateTableTraversalCallback(ieutThreadData_t *pThreadData,
                                 ismEngine_ClientState_t *pClient,
                                 void *pContext)
{
    stateTableTraversalContext_t *context = (stateTableTraversalContext_t *)pContext;

    context->callCount += 1;

    return true;
}

void clientStateTestClientStateTableTraversal(void)
{
    ismEngine_ClientStateHandle_t hClient[50];
    int32_t rc = OK;
    int32_t fullClientCount = 3; // Already expect 3 for shared namespaces

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    TEST_ASSERT_PTR_NOT_NULL(pThreadData);

    int32_t createdClients = 0;
    for(; createdClients<(sizeof(hClient)/sizeof(hClient[0])); createdClients++)
    {
        char clientId[20];

        sprintf(clientId, "CSTT_Client_%d", createdClients+1);

        rc = ism_engine_createClientState(clientId,
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                          NULL, NULL, NULL,
                                          &hClient[createdClients],
                                          NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    fullClientCount += createdClients;

    // Iterate over all chains
    stateTableTraversalContext_t context = {0};
    rc = iecs_traverseClientStateTable(pThreadData,
                                       NULL, 0, 0, NULL,
                                       stateTableTraversalCallback,
                                       &context);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.callCount, fullClientCount);

    // Get the current generation, but iterate over all chains
    context.callCount = 0;
    rc = iecs_traverseClientStateTable(pThreadData,
                                       &context.tableGeneration, 0, 0, NULL,
                                       stateTableTraversalCallback,
                                       &context);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.callCount, fullClientCount);

    // Specify a non-matching generation
    context.tableGeneration += 1;
    context.callCount = 0;
    rc = iecs_traverseClientStateTable(pThreadData,
                                       &context.tableGeneration, 0, 0, NULL,
                                       stateTableTraversalCallback,
                                       &context);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientTableGenMismatch);
    TEST_ASSERT_EQUAL(context.callCount, 0);

    // Call a chain at a time...
    context.tableGeneration -= 1;
    context.callCount = 0;

    uint32_t startChain = 0;
    uint32_t chainCount = 0;
    do
    {
        chainCount++;

        rc = iecs_traverseClientStateTable(pThreadData,
                                           &context.tableGeneration, startChain, 1, &startChain,
                                           stateTableTraversalCallback,
                                           &context);
    }
    while(rc == ISMRC_MoreChainsAvailable);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.callCount, fullClientCount);

    // Specify a startChain > count
    context.callCount = 0;
    rc = iecs_traverseClientStateTable(pThreadData,
                                       &context.tableGeneration, chainCount*10, 10, NULL,
                                       stateTableTraversalCallback, &context);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.callCount, 0);

    // Specify chainCount > chain count
    context.callCount = 0;
    rc = iecs_traverseClientStateTable(pThreadData,
                                       &context.tableGeneration, 0, chainCount*10, NULL,
                                       stateTableTraversalCallback, &context);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.callCount, fullClientCount);

    for(int32_t i=0; i<createdClients; i++)
    {
        rc = sync_ism_engine_destroyClientState(hClient[i],
                                                ismENGINE_DESTROY_CLIENT_OPTION_NONE);
        TEST_ASSERT_EQUAL(rc, OK);
    }
}

/*
 * Create client-states making sure that stealing during delivery of a message works
 */
void clientStateTestStealDuringDelivery(void)
{
    ismEngine_ClientStateHandle_t hClientPub;
    ismEngine_SessionHandle_t hSessionPub;
    ismEngine_ProducerHandle_t hProducerPub;
    ismEngine_ClientStateHandle_t hClientSub1;
    ismEngine_SessionHandle_t hSessionSub1;
    ismEngine_ConsumerHandle_t hConsumerSub1;
    ismEngine_MessageHandle_t hMessage;
    ismEngine_ClientStateHandle_t hClientSub1Steal;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ism_engine_createClientState("PubClient",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClientPub,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientPub,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionPub,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &hClientSub1,
                                      clientStateStealCallback,
                                      NULL,
                                      &hClientSub1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClientSub1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSessionSub1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_startMessageDelivery(hSessionSub1,
                                         ismENGINE_START_DELIVERY_OPTION_NONE,
                                         NULL,
                                         0,
                                         NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    callbackContext_t callbackContext;
    callbackContext.phClient = &hClientSub1Steal;
    callbackContext.MessageCount = 0;
    callbackContext_t *pCallbackContext = &callbackContext;
    hClientSub1Steal = NULL;

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSessionSub1,
                                   ismDESTINATION_TOPIC,
                                   "Topic/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pCallbackContext,
                                   sizeof(callbackContext_t *),
                                   deliveryCallbackStealClientState,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumerSub1,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSessionPub,
                                   ismDESTINATION_TOPIC,
                                   "Topic/A",
                                   &hProducerPub,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 0);

    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengths[2] = {0, 13};
    void *areaData[2] = {NULL, "Message body"};

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSessionPub,
                               hProducerPub,
                               NULL,
                               hMessage,
                               NULL,
                               0,
                               NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    // Dump a client state that includes producers
    test_dumpClientState("PubClient");

    TEST_ASSERT_EQUAL(callbackContext.MessageCount, 1);

    TEST_ASSERT_CUNIT(hClientSub1Steal != NULL, ("hClientSub1Steal != NULL"));
    rc = ism_engine_destroyClientState(hClientSub1Steal,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyProducer(hProducerPub,
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySession(hSessionPub,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClientPub,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}

/*
 * Create client-states making sure that stealing a client id during the steal of the same client id works
 * but don't destroy the first client-state immediately as its steal callback is called
 */
void clientStateTestDoubleStealLazy(void)
{
    ismEngine_ClientStateHandle_t hClient1 = NULL;
    ismEngine_ClientStateHandle_t hClient1b = NULL;
    ismEngine_ClientStateHandle_t hClient1c = NULL;
    ismEngine_ClientStateHandle_t *pStealContext1b = &hClient1b;
    ismEngine_ClientStateHandle_t *pStealContext1c = &hClient1c;
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &hClient1,
                                      clientStateNoStealCallback,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &hClient1b,
                                      clientStateStealCallback,
                                      NULL,
                                      &hClient1b,
                                      &pStealContext1b,
                                      sizeof(pStealContext1b),
                                      callbackCreateClientStateComplete);
    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &hClient1c,
                                      clientStateStealCallback,
                                      NULL,
                                      &hClient1c,
                                      &pStealContext1c,
                                      sizeof(pStealContext1c),
                                      callbackCreateClientStateComplete);
    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    while(hClient1c == NULL) { sched_yield(); }

    rc = ism_engine_destroyClientState(hClient1c,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Create client-states making sure that stealing a client id during the steal of the same client id works
 * but aggressively destroy the client-state as its steal callback is called
 */
void clientStateTestDoubleStealEager(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ClientStateHandle_t hClient1b;
    ismEngine_ClientStateHandle_t hClient1c;
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &hClient1,
                                      clientStateStealCallback,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                           &hClient1b,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1b);
    TEST_ASSERT_EQUAL(rc, OK); // Was ISMRC_ResumedClientState

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                           &hClient1c,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1c);
    TEST_ASSERT_EQUAL(rc, OK); // Was ISMRC_ResumedClientState

    rc = ism_engine_destroyClientState(hClient1c,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Test handling of unreleased delivery IDs with a nondurable client
 */
void clientStateTestUnreleased(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_UnreleasedHandle_t ahUnrel[UNREL_LIMIT] = {NULL};
    ismEngine_UnreleasedHandle_t hUnrelA;
    ismEngine_UnreleasedHandle_t hUnrelB;
    uint32_t unreleasedDeliveryId;
    unreleasedContext_t unrelContext1 = {0, {false}, {NULL}};
    unreleasedContext_t unrelContext2 = {0, {false}, {NULL}};
    unreleasedContext_t unrelContext3 = {0, {false}, {NULL}};
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add a bunch of unreleased delivery IDs (99 ... 0)
    unreleasedDeliveryId = UNREL_LIMIT;
    while (unreleasedDeliveryId > 0)
    {
        unreleasedDeliveryId--;

        rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                                NULL,
                                                unreleasedDeliveryId,
                                                &(ahUnrel[unreleasedDeliveryId]),
                                                NULL,
                                                0,
                                                NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Add a couple of duplicates too (99 and 5)
    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            UNREL_LIMIT - 1,
                                            &hUnrelA,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            UNREL_LIMIT / 20,
                                            &hUnrelB,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_listUnreleasedDeliveryIds(hClient1,
                                              &unrelContext1,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the unreleased delivery IDs were as expected
    TEST_ASSERT_EQUAL(unrelContext1.Count, UNREL_LIMIT);
    for (unreleasedDeliveryId = 0; unreleasedDeliveryId < UNREL_LIMIT; unreleasedDeliveryId++)
    {
        TEST_ASSERT_EQUAL(unrelContext1.Found[0], true);
    }

    // Remove just one, which will be a duplicate removal shortly... (99)
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               hUnrelA,
                                               NULL,
                                               0,
                                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_dumpClientState("Client1");

    // Remove a bunch of unreleased delivery IDs (99 ... 3)
    unreleasedDeliveryId = UNREL_LIMIT;
    while (unreleasedDeliveryId > 3)
    {
        unreleasedDeliveryId--;

        rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                                   NULL,
                                                   ahUnrel[unreleasedDeliveryId],
                                                   NULL,
                                                   0,
                                                   NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Remove the last one in (0)
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               ahUnrel[0],
                                               NULL,
                                               0,
                                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And a duplicate removal again (5)
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               hUnrelB,
                                               NULL,
                                               0,
                                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // List out what we have left, should be just 1 and 2
    rc = ism_engine_listUnreleasedDeliveryIds(hClient1,
                                              &unrelContext2,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the unreleased delivery IDs were as expected
    TEST_ASSERT_EQUAL(unrelContext2.Count, 2);
    TEST_ASSERT_EQUAL(unrelContext2.Found[0], false);
    TEST_ASSERT_EQUAL(unrelContext2.Found[1], true);
    TEST_ASSERT_EQUAL(unrelContext2.Found[2], true);
    TEST_ASSERT_EQUAL(unrelContext2.Found[3], false);

    // Remove the remaining ones (1 and 2)
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               ahUnrel[1],
                                               NULL,
                                               0,
                                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Remove the last one in (0)
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               ahUnrel[2],
                                               NULL,
                                               0,
                                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // List out what we have left, should be nothing at all
    rc = ism_engine_listUnreleasedDeliveryIds(hClient1,
                                              &unrelContext3,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(unrelContext3.Count, 0);

    // Check that the unreleased delivery IDs were as expected
    TEST_ASSERT_EQUAL(unrelContext3.Count, 0);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);


    // Make a new client-state and loop through a sequence of 1 ... 10 repeatedly
    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t i;

    hUnrelB = 0;
    for (i = 0; i < 10; i++)
    {
        for (unreleasedDeliveryId = 1; unreleasedDeliveryId <= 10; unreleasedDeliveryId++)
        {
            rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                                    NULL,
                                                    unreleasedDeliveryId,
                                                    &hUnrelA,
                                                    NULL,
                                                    0,
                                                    NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            if (hUnrelB != 0)
            {
                rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                                           NULL,
                                                           hUnrelB,
                                                           NULL,
                                                           0,
                                                           NULL);
                TEST_ASSERT_EQUAL(rc, OK);
            }

            hUnrelB = hUnrelA;
        }
    }

    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               hUnrelB,
                                               NULL,
                                               0,
                                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // List out what we have left, should be nothing at all
    rc = ism_engine_listUnreleasedDeliveryIds(hClient1,
                                              &unrelContext3,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(unrelContext3.Count, 0);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Create a durable client-state
 */
void clientStateTestSimpleDurable(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ClientStateHandle_t hClient1b;
    ismEngine_ClientStateHandle_t hClient2;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_UnreleasedHandle_t hUnrel1;
    ismEngine_UnreleasedHandle_t hUnrel2;
    ismEngine_UnreleasedHandle_t hUnrel3;
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    iecsMessageDeliveryInfoHandle_t hMsgDelInfo = NULL;

    rc = iecs_findClientMsgDelInfo(pThreadData, "Client1", &hMsgDelInfo);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                      ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1b,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDInUse);

    rc = iecs_findClientMsgDelInfo(pThreadData, "Client1", &hMsgDelInfo);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_EQUAL(hMsgDelInfo, ((ismEngine_ClientState_t *)hClient1)->hMsgDeliveryInfo);

    rc = sync_ism_engine_createClientState("Client2",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient2);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    rc = sync_ism_engine_createClientState("Client2",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient2);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    // Do we still find a zombie's msg delivery info?
    rc = iecs_findClientMsgDelInfo(pThreadData, "Client1", &hMsgDelInfo);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_EQUAL(hMsgDelInfo, ((ismEngine_ClientState_t *)hClient1)->hMsgDeliveryInfo);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                      ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1b,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDInUse);

    rc = ism_engine_destroyClientState(hClient2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add some unreleased delivery IDs so we can ensure we carried them across
    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            1,
                                            &hUnrel1,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            3,
                                            &hUnrel3,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            2,
                                            &hUnrel2,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Do the release and destroy client state together...
    test_incrementActionsRemaining(pActionsRemaining, 2);
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               hUnrel3,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    // Check that the unreleased delivery IDs have been inherited
    unreleasedContext_t unrelContext = {0, {false}, {NULL}};

    rc = ism_engine_listUnreleasedDeliveryIds(hClient1,
                                              &unrelContext,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the unreleased delivery IDs were as expected
    TEST_ASSERT_EQUAL(unrelContext.Count, 2);
    TEST_ASSERT_EQUAL(unrelContext.Found[0], false);
    TEST_ASSERT_EQUAL(unrelContext.Found[1], true);
    TEST_ASSERT_EQUAL(unrelContext.Found[2], true);
    TEST_ASSERT_EQUAL(unrelContext.Found[3], false);

    rc = sync_ism_engine_destroyClientState(hClient1,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Create a durable client (like MQTT CleanSession=0), disconnect and reconnect
 * ensuring that the subscriptions and delivery state is moved across
 */
void clientStateTestCleanSession0(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_UnreleasedHandle_t hUnrel1;
    ismEngine_UnreleasedHandle_t hUnrel2;
    ismEngine_UnreleasedHandle_t hUnrel3;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add some unreleased delivery IDs so we can ensure we carried them across
    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a property
    ism_field_t PropertyField = {VT_Integer, 0, {.i = 1 }};
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);
    rc = ism_common_setProperty(properties, "ClientStateProperty", &PropertyField);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = sync_ism_engine_createSubscription(hClient1,
                                            "Sub1",
                                            properties,
                                            ismDESTINATION_TOPIC,
                                            "TOPIC/1",
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    ism_common_freeProperties(properties);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            1,
                                            &hUnrel1,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            3,
                                            &hUnrel3,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            2,
                                            &hUnrel2,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Do unrelease and destroy client state together
    test_incrementActionsRemaining(pActionsRemaining, 2);
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               hUnrel3,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    // Check that the unreleased delivery IDs have been inherited
    unreleasedContext_t unrelContext = {0, {false}, {NULL}};

    rc = ism_engine_listUnreleasedDeliveryIds(hClient1,
                                              &unrelContext,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the unreleased delivery IDs were as expected
    TEST_ASSERT_EQUAL(unrelContext.Count, 2);
    TEST_ASSERT_EQUAL(unrelContext.Found[0], false);
    TEST_ASSERT_EQUAL(unrelContext.Found[1], true);
    TEST_ASSERT_EQUAL(unrelContext.Found[2], true);
    TEST_ASSERT_EQUAL(unrelContext.Found[3], false);


    // Check that the durable subscriptions have been inherited
    durableSubsContext_t durableSubsContext = {0, true, {false}};

    rc = ism_engine_listSubscriptions(hClient1,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the subscription was remembered
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 1);
    TEST_ASSERT_EQUAL(durableSubsContext.Found[1], true);

    rc = ism_engine_destroySubscription(hClient1,
                                        "Sub1",
                                        hClient1,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyClientState(hClient1,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Create a nondurable client (like MQTT CleanSession=1), disconnect and reconnect
 * ensuring that the delivery state is not moved across
 */
void clientStateTestCleanSession1(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_UnreleasedHandle_t hUnrel1;
    ismEngine_UnreleasedHandle_t hUnrel2;
    ismEngine_UnreleasedHandle_t hUnrel3;
    int32_t rc = OK;

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add some unreleased delivery IDs so we can ensure we carried them across
    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            1,
                                            &hUnrel1,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            3,
                                            &hUnrel3,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            2,
                                            &hUnrel2,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               hUnrel3,
                                               NULL,
                                               0,
                                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLEANSTART,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that no unreleased delivery IDs have been inherited
    unreleasedContext_t unrelContext = {0, {false}, {NULL}};

    rc = ism_engine_listUnreleasedDeliveryIds(hClient1,
                                              &unrelContext,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(unrelContext.Count, 0);


    // Check that no durable subscriptions have been inherited
    durableSubsContext_t durableSubsContext = {0, true, {false}};

    rc = ism_engine_listSubscriptions(hClient1,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 0);

    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Create a durable client which is stolen by a durable client,
 * ensuring that the subscriptions and delivery state is moved across
 */
void clientStateTestDurableToDurable(void)
{
    ismEngine_ClientStateHandle_t hClient1 = NULL;
    ismEngine_ClientStateHandle_t hClient1b;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    ismEngine_SessionHandle_t hSession1;
    ismEngine_UnreleasedHandle_t hUnrel1;
    ismEngine_UnreleasedHandle_t hUnrel2;
    ismEngine_UnreleasedHandle_t hUnrel3;
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    // Do two loops, first time make it look like the store is running out of space
    for(int32_t i=0; i<2; i++)
    {
        ismEngine_serverGlobal.componentStatus[ismENGINE_STATUS_STORE_MEMORY_0] = ((i == 0) ? StatusWarning : StatusOk);

        rc = sync_ism_engine_createClientState("Client1",
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               &hClient1,
                                               clientStateStealCallback,
                                               NULL,
                                               &hClient1);
        if (i == 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_ServerCapacity);
            TEST_ASSERT_PTR_NULL(hClient1);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hClient1);
        }
    }

    // Add some unreleased delivery IDs so we can ensure we carried them across
    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a property
    ism_field_t PropertyField = {VT_Integer, 0, {.i = 1}};
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);
    rc = ism_common_setProperty(properties, "ClientStateProperty", &PropertyField);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_createSubscription(hClient1,
                                       "Sub1",
                                       properties,
                                       ismDESTINATION_TOPIC,
                                       "TOPIC/1",
                                       &subAttrs,
                                       NULL, // Owning client same as requesting client
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    ism_common_freeProperties(properties);

    test_waitForRemainingActions(pActionsRemaining);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            1,
                                            &hUnrel1,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            3,
                                            &hUnrel3,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            2,
                                            &hUnrel2,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               hUnrel3,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           &hClient1b,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1b);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    test_waitForRemainingActions(pActionsRemaining);

    // The steal callback will have cleared this
    TEST_ASSERT_EQUAL(hClient1, NULL);

    // Check that the unreleased delivery IDs have been inherited
    unreleasedContext_t unrelContext = {0, {false}, {NULL}};

    rc = ism_engine_listUnreleasedDeliveryIds(hClient1b,
                                              &unrelContext,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the unreleased delivery IDs were as expected
    TEST_ASSERT_EQUAL(unrelContext.Count, 2);
    TEST_ASSERT_EQUAL(unrelContext.Found[0], false);
    TEST_ASSERT_EQUAL(unrelContext.Found[1], true);
    TEST_ASSERT_EQUAL(unrelContext.Found[2], true);
    TEST_ASSERT_EQUAL(unrelContext.Found[3], false);


    // Check that the durable subscriptions have been inherited
    durableSubsContext_t durableSubsContext = {0, true, {false}};

    rc = ism_engine_listSubscriptions(hClient1b,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the subscription was remembered
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 1);
    TEST_ASSERT_EQUAL(durableSubsContext.Found[1], true);

    rc = ism_engine_destroySubscription(hClient1b,
                                        "Sub1",
                                        hClient1b,
                                        NULL,
                                        0,
                                        NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient1b,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);
}


/*
 * Create a non-durable client which is stolen by a durable client,
 * ensuring that the subscriptions and delivery state is not moved across
 */
void clientStateTestNondurableToDurable(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ClientStateHandle_t hClient1b;
    ismEngine_ClientStateHandle_t hClient1c;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_SessionHandle_t hSession1b;
    ismEngine_UnreleasedHandle_t hUnrel1;
    ismEngine_UnreleasedHandle_t hUnrel2;
    ismEngine_UnreleasedHandle_t hUnrel3;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                      &hClient1,
                                      clientStateStealCallback,
                                      NULL,
                                      &hClient1,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add some unreleased delivery IDs so we can ensure we did not carry them across
    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            1,
                                            &hUnrel1,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            3,
                                            &hUnrel3,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            2,
                                            &hUnrel2,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               hUnrel3,
                                               NULL,
                                               0,
                                               NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           &hClient1b,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1b);
    TEST_ASSERT_EQUAL(rc, OK); // Was ISMRC_ResumedClientState

    // The steal callback will have cleared this
    TEST_ASSERT_EQUAL(hClient1, NULL);

    // Check that the unreleased delivery IDs have not been inherited
    unreleasedContext_t unrelContext = {0, {false}, {NULL}};

    rc = ism_engine_listUnreleasedDeliveryIds(hClient1b,
                                              &unrelContext,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the unreleased delivery IDs were as expected
    TEST_ASSERT_EQUAL(unrelContext.Count, 0);

    // Check that there are no durable subscriptions inherited
    durableSubsContext_t durableSubsContext = {0, true, {false}};

    rc = ism_engine_listSubscriptions(hClient1b,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that there are no subscriptions
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 0);

    // Add some unreleased delivery IDs so we can ensure we carried them across
    rc = ism_engine_createSession(hClient1b,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1b,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;

    rc = sync_ism_engine_createSubscription(hClient1b,
                                            "Sub1",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "TOPIC/1",
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1b,
                                            NULL,
                                            1,
                                            &hUnrel1,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1b,
                                            NULL,
                                            3,
                                            &hUnrel3,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1b,
                                            NULL,
                                            2,
                                            &hUnrel2,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1b,
                                               NULL,
                                               hUnrel3,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient1b,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                           &hClient1c,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1c);
    TEST_ASSERT_EQUAL(rc, OK); // Was ISMRC_ResumedClientState);

    // Check that no unreleased delivery IDs have been inherited
    memset(&unrelContext, 0, sizeof(unrelContext));

    rc = ism_engine_listUnreleasedDeliveryIds(hClient1c,
                                              &unrelContext,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(unrelContext.Count, 0);


    // Check that no durable subscriptions have been inherited
    memset(&durableSubsContext, 0, sizeof(durableSubsContext));

    rc = ism_engine_listSubscriptions(hClient1c,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 0);

    rc = ism_engine_destroyClientState(hClient1c,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Create a durable client-state which is stolen by a non-durable client-state,
 * ensuring that the subscriptions and delivery state are not moved across.
 * Then steal the non-durable with another non-durable, and finish by
 * stealing with a durable client-state.
 */
void clientStateTestDurableToNondurable(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ClientStateHandle_t hClient1b;
    ismEngine_ClientStateHandle_t hClient1c;
    ismEngine_ClientStateHandle_t hClient1d;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_UnreleasedHandle_t hUnrel1;
    ismEngine_UnreleasedHandle_t hUnrel2;
    ismEngine_UnreleasedHandle_t hUnrel3;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           &hClient1,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add some unreleased delivery IDs so we can ensure we carried them across
    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a property
    ism_field_t PropertyField = {VT_Integer, 0, {.i = 1}};
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);
    rc = ism_common_setProperty(properties, "ClientStateProperty", &PropertyField);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;

    rc = sync_ism_engine_createSubscription(hClient1,
                                            "Sub1",
                                            properties,
                                            ismDESTINATION_TOPIC,
                                            "TOPIC/1",
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    ism_common_freeProperties(properties);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            1,
                                            &hUnrel1,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            3,
                                            &hUnrel3,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            NULL,
                                            2,
                                            &hUnrel2,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               NULL,
                                               hUnrel3,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                           &hClient1b,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1b);
    TEST_ASSERT_EQUAL(rc, OK); // Was ISMRC_ResumedClientState

    test_waitForRemainingActions(pActionsRemaining);

    // The steal callback will have cleared this
    TEST_ASSERT_EQUAL(hClient1, NULL);

    // Check that the unreleased delivery IDs have not been inherited
    unreleasedContext_t unrelContext = {0, {false}, {NULL}};

    rc = ism_engine_listUnreleasedDeliveryIds(hClient1b,
                                              &unrelContext,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the unreleased delivery IDs were as expected
    TEST_ASSERT_EQUAL(unrelContext.Count, 0);


    // Check that the durable subscriptions have not been inherited
    durableSubsContext_t durableSubsContext = {0, true, {false}};

    rc = ism_engine_listSubscriptions(hClient1b,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 0);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                           &hClient1c,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1c);
    TEST_ASSERT_EQUAL(rc, OK); // Was ISMRC_ResumedClientState

    // The steal callback will have cleared this
    TEST_ASSERT_EQUAL(hClient1b, NULL);

    // Check that the unreleased delivery IDs have not been inherited
    rc = ism_engine_listUnreleasedDeliveryIds(hClient1c,
                                              &unrelContext,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(unrelContext.Count, 0);


    // Check that the durable subscriptions have not been inherited
    rc = ism_engine_listSubscriptions(hClient1c,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 0);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           &hClient1d,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1d);
    TEST_ASSERT_EQUAL(rc, OK); // Was ISMRC_ResumedClientState

    // The steal callback will have cleared this
    TEST_ASSERT_EQUAL(hClient1c, NULL);

    // Check that the unreleased delivery IDs have not been inherited
    rc = ism_engine_listUnreleasedDeliveryIds(hClient1d,
                                              &unrelContext,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(unrelContext.Count, 0);


    // Check that the durable subscriptions have not been inherited
    rc = ism_engine_listSubscriptions(hClient1d,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 0);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient1d,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);
}


/*
 * Create and destroy a massive number of client-states to ensure that the data structures
 * grow properly
 */
void clientStateTestMassive(void)
{
#define MASSIVE 1000000

    ismEngine_ClientStateHandle_t *phClient;
    char clientId[25];
    uint32_t i;
    int32_t rc = OK;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    phClient = malloc(sizeof(ismEngine_ClientStateHandle_t) * MASSIVE);
    TEST_ASSERT(phClient != NULL, ("Out of memory"));

    for (i = 0; (i < MASSIVE) && (rc == OK); i++)
    {
        sprintf(clientId, "Client%u", i);

        rc = ism_engine_createClientState(clientId,
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &phClient[i],
                                          NULL,
                                          0,
                                          NULL);
    }

    for (i = 0; (i < MASSIVE) && (rc == OK); i++)
    {
        rc = ism_engine_destroyClientState(phClient[i],
                                           ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                           NULL,
                                           0,
                                           NULL);
    }

    free(phClient);

    TEST_ASSERT_EQUAL(rc, OK);
}


/*
 * Durable client-state with transactional operations
 */
void clientStateTestDurableTran(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ClientStateHandle_t hClient1b;
    ismEngine_ClientStateHandle_t hClient2;
    ismEngine_SessionHandle_t hSession1;
    ismEngine_TransactionHandle_t hTran1;
    ismEngine_UnreleasedHandle_t hUnrel1;
    ismEngine_UnreleasedHandle_t hUnrel2;
    ismEngine_UnreleasedHandle_t hUnrel3;
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                      ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1b,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDInUse);

    rc = sync_ism_engine_createClientState("Client2",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient2);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyClientState(hClient2,
                                            ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_createClientState("Client2",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient2);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                      ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &hClient1b,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDInUse);

    rc = ism_engine_destroyClientState(hClient2,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add some unreleased delivery IDs so we can ensure we carried them across
    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add two delivery IDs in a transaction and commit
    rc = sync_ism_engine_createLocalTransaction(hSession1,
                                                &hTran1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            hTran1,
                                            1,
                                            &hUnrel1,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            hTran1,
                                            2,
                                            &hUnrel2,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_commitTransaction(hSession1,
                                           hTran1,
                                           ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    // Add one and remove one delivery IDs in a transaction but roll it back by destroying the client-state
    rc = sync_ism_engine_createLocalTransaction(hSession1,
                                                &hTran1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_addUnreleasedDeliveryId(hSession1,
                                            hTran1,
                                            3,
                                            &hUnrel3,
                                            NULL,
                                            0,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 2);
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               hTran1,
                                               hUnrel2,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               hTran1,
                                               hUnrel3,
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == ISMRC_LockNotGranted) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining); // Fails because this one is in-flight
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, ISMRC_ResumedClientState);

    rc = ism_engine_createSession(hClient1,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession1,
                                  NULL,
                                  0,
                                  NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the unreleased delivery IDs have been inherited
    unreleasedContext_t unrelContext = {0, {false}, {NULL}};

    rc = ism_engine_listUnreleasedDeliveryIds(hClient1,
                                              &unrelContext,
                                              unreleasedIdsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the unreleased delivery IDs were as expected
    TEST_ASSERT_EQUAL(unrelContext.Count, 2);
    TEST_ASSERT_EQUAL(unrelContext.Found[0], false);
    TEST_ASSERT_EQUAL(unrelContext.Found[1], true);
    TEST_ASSERT_EQUAL(unrelContext.Found[2], true);
    TEST_ASSERT_EQUAL(unrelContext.Found[3], false);

    // Remove one of the remaining unreleased delivery Ids in a transaction, and commit it
    rc = sync_ism_engine_createLocalTransaction(hSession1,
                                                &hTran1);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_removeUnreleasedDeliveryId(hSession1,
                                               hTran1,
                                               unrelContext.hUnrel[1],
                                               &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    test_waitForRemainingActions(pActionsRemaining);

    rc = sync_ism_engine_commitTransaction(hSession1,
                                           hTran1,
                                           ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_destroyClientState(hClient1,
                                       ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                       &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    // Wait for everything to finish.
    test_waitForRemainingActions(pActionsRemaining);
}


/*
 * Create a durable client with durable subscriptions and steal the client ID
 */
void clientStateTestStealDurableSubs(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ClientStateHandle_t hClient1b;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t rc = OK;

    rc = sync_ism_engine_createClientState("Client1",
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           &hClient1,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;

    rc = sync_ism_engine_createSubscription(hClient1,
                                            "Sub1",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "TOPIC/1",
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    // We had a bug where the callback was being fired even though the operation completed synchronously
    rc = sync_ism_engine_createClientState("Client1",
                                          testDEFAULT_PROTOCOL_ID,
                                          ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &hClient1b);
    TEST_ASSERT_EQUAL(rc, OK); // Was ISMRC_ResumedClientState);


    // Check that the durable subscriptions have been inherited
    durableSubsContext_t durableSubsContext = {0, true, {false}};

    rc = ism_engine_listSubscriptions(hClient1b,
                                      NULL,
                                      &durableSubsContext,
                                      durableSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check that the subscription was not remembered
    TEST_ASSERT_EQUAL(durableSubsContext.Count, 0);

    rc = sync_ism_engine_destroyClientState(hClient1b,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}

/*
 * Create a durable client of one protocol and try and use another
 * to (a) steal it and (b) pick it up as a zombie
 */
void clientStateTestProtocolMismatch(void)
{
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_ClientStateHandle_t hClient1b;
    int32_t rc = OK;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    rc = sync_ism_engine_createClientState("Client1",
                                           PROTOCOL_ID_MQTT,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                           ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           &hClient1,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t useProtocolId = PROTOCOL_ID_JMS;
    int32_t expectedRC = ISMRC_ProtocolMismatch;
    for(int32_t i=0; i<4; i++)
    {
        rc = sync_ism_engine_createClientState("Client1",
                                               useProtocolId,
                                               ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &hClient1b);
        TEST_ASSERT_EQUAL(rc, expectedRC);

        switch(i)
        {
            // 1st loop: Make it a zombie
            case 0:
                test_incrementActionsRemaining(pActionsRemaining, 1);
                rc = ism_engine_destroyClientState(hClient1,
                                                   ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                                   &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
                if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
                else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

                test_waitForRemainingActions(pActionsRemaining);
                break;
            // 2nd loop: Use the right protocol Id
            case 1:
                useProtocolId = PROTOCOL_ID_MQTT;
                expectedRC = ISMRC_ResumedClientState;
                break;
            // 3rd loop: Use an acceptable alternative protocol Id (but client is not stealable)
            case 2:
                useProtocolId = PROTOCOL_ID_HTTP;
                expectedRC = ISMRC_ClientIDInUse;
                break;
            // 4th loop: Destroy the client state completely
            case 3:
                test_incrementActionsRemaining(pActionsRemaining, 1);
                rc = ism_engine_destroyClientState(hClient1b,
                                                   ismENGINE_DESTROY_CLIENT_OPTION_DISCARD,
                                                   &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
                if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
                else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

                test_waitForRemainingActions(pActionsRemaining);
                expectedRC = OK;
                break;
            default:
                break;
        }
    }
}

//Test duplicate client problem arising from interleaved cleanSession=true & cleanSession=false stealing connects
typedef struct tag_clientStateTestInterleavedStealProblem_ThreadInfo_t
{
    ismEngine_ClientStateHandle_t hClient;
    const char *clientId;
    pthread_t threadToDelay;
    bool threadWait;
    bool threadWaiting;
} clientStateTestInterleavedStealProblem_ThreadInfo_t;

void *clientStateTestInterleavedStealProblem_Thread(void *context)
{
    int32_t rc;
    clientStateTestInterleavedStealProblem_ThreadInfo_t *pThreadInfo = (clientStateTestInterleavedStealProblem_ThreadInfo_t *)context;

    ism_engine_threadInit(0);

    pThreadInfo->threadToDelay = pthread_self();
    pThreadInfo->threadWait = true;

    ismEngine_ClientStateHandle_t hClient;
    rc = ism_engine_createClientState(pThreadInfo->clientId,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &pThreadInfo->hClient,
                                      clientStateStealCallback,
                                      NULL,
                                      &hClient,
                                      NULL, 0, NULL);

    // Don't expect to get control back
    TEST_ASSERT(false, ("We don't expect to get control back here rc=%d", rc));

    return NULL;
}

void clientStateTestTableResizeDuringRecovery(void)
{
    int32_t rc;

    uint32_t clientCount = ((1 << iecsHASH_TABLE_SHIFT_INITIAL)*(iecsHASH_TABLE_LOADING_LIMIT))+10;
    ismEngine_ClientState_t **pClient = malloc(clientCount * sizeof(ismEngine_ClientState_t *));
    TEST_ASSERT_PTR_NOT_NULL(pClient);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // Purposefully only fill in the fields that are meant to be touched for RECOVERY, to
    // find any ASAN problems.
    iecsNewClientStateInfo_t clientInfo;
    char clientId[15];

    clientInfo.pClientId = clientId;
    clientInfo.protocolId = PROTOCOL_ID_MQTT;
    clientInfo.durability = iecsNonDurable;

    for(uint32_t i=0; i<clientCount; i++)
    {
        sprintf(clientId, "CLIENT_%u", i);
        clientInfo.resourceSet = iecs_getResourceSet(pThreadData, clientInfo.pClientId, clientInfo.protocolId, iereOP_ADD);

        rc = iecs_newClientStateRecovery(pThreadData, &clientInfo, &pClient[i]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(pClient[i]);

        // Need to lock the client table because the client expiry reaper could be running
        rc = pthread_mutex_lock(&ismEngine_serverGlobal.Mutex);
        TEST_ASSERT_EQUAL(rc, 0);

        rc = iecs_addClientStateRecovery(pThreadData, pClient[i]);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = pthread_mutex_unlock(&ismEngine_serverGlobal.Mutex);
        TEST_ASSERT_EQUAL(rc, 0);
    }

    for(uint32_t i=0; i<clientCount; i++)
    {
        // So we can call ism_engine_destroyClientState, we need to 'fiddle' (look away now)...
        iecs_incrementActiveClientStateCount(pThreadData, pClient[i]->Durability, pClient[i]->fCountExternally, pClient[i]->resourceSet);
        pClient[i]->OpState = iecsOpStateActive;
        // ... done fiddling, you can look again.
        rc = ism_engine_destroyClientState(pClient[i], ismENGINE_DESTROY_CLIENT_OPTION_DISCARD, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    free(pClient);
}

/*
 * Test steals involving unusual pairings (e.g. PROTOCOL_ID_ENGINE)
 */
void clientStateTestUnusualSteals(void)
{
    ismEngine_ClientStateHandle_t hClient1 = NULL;
    ismEngine_ClientStateHandle_t hClient1b = NULL;
    int32_t rc = OK;

    rc = ism_engine_createClientState("Client1",
                                      PROTOCOL_ID_MQTT,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &hClient1,
                                      clientStateStealCallback,
                                      NULL,
                                      &hClient1,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient1);

    // Try and steal with a different (non-engine) protocol
    rc = ism_engine_createClientState("Client1",
                                      PROTOCOL_ID_PLUGIN,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &hClient1b,
                                      clientStateStealCallback,
                                      NULL,
                                      &hClient1b,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ProtocolMismatch);
    TEST_ASSERT_PTR_NULL(hClient1b);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    for(int32_t i=0; i<2; i++)
    {
        // Try and steal with an engine protocol
        // (need to go in through internal interface, because external doesn't allow
        // PROTOCOL_ID_ENGINE).
        rc = sync_iecs_createClientState(pThreadData,
                                         "Client1",
                                         PROTOCOL_ID_ENGINE,
                                         ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                         iecsCREATE_CLIENT_OPTION_NONE,
                                         &hClient1b,
                                         clientStateStealCallback,
                                         NULL,
                                         &hClient1b);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hClient1b);
        TEST_ASSERT_PTR_NULL(hClient1); // Steal callback clears

        // Try and steal with a non-engine protocol (specifying STEAL 1st time, and BOUNCE 2nd)
        if (i == 0)
        {
            rc = sync_ism_engine_createClientState("Client1",
                                                   PROTOCOL_ID_PLUGIN,
                                                   ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                                   &hClient1,
                                                   clientStateStealCallback,
                                                   NULL,
                                                   &hClient1);
        }
        else
        {
            rc = sync_ism_engine_createClientState("Client1",
                                                   PROTOCOL_ID_PLUGIN,
                                                   ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                                   &hClient1,
                                                   clientStateStealCallback,
                                                   NULL,
                                                   &hClient1);
        }

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hClient1);
        TEST_ASSERT_PTR_NULL(hClient1b);  // Steal callback clears
    }

    // Expect clientID in use because we specified BOUNCE
    rc = sync_ism_engine_createClientState("Client1",
                                           PROTOCOL_ID_PLUGIN,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_BOUNCE,
                                           &hClient1b,
                                           clientStateStealCallback,
                                           NULL,
                                           &hClient1b);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDInUse);

    // Expect steal because we specified STEAL (even though we don't have a steal callback)
    rc = sync_ism_engine_createClientState("Client1",
                                           PROTOCOL_ID_PLUGIN,
                                           ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &hClient1b);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient1b);
    TEST_ASSERT_PTR_NULL(hClient1); // Steal callback clears

    // Try engine client state stealing from a client with no steal callback
    // Try and steal with an engine protocol
    rc = sync_iecs_createClientState(pThreadData,
                                     "Client1",
                                     PROTOCOL_ID_ENGINE,
                                     ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                     iecsCREATE_CLIENT_OPTION_NONE,
                                     &hClient1,
                                     clientStateStealCallback,
                                     NULL,
                                     &hClient1);
    TEST_ASSERT_EQUAL(rc, ISMRC_ClientIDInUse);
    TEST_ASSERT_PTR_NULL(hClient1);

    // Clean up
    rc = sync_ism_engine_destroyClientState(hClient1b,
                                            ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
    TEST_ASSERT_EQUAL(rc, OK);
}

/*
 * Tests of the forceDiscardClientState function
 */
void clientStateTestForceDiscard(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_ClientStateHandle_t hClient1 = NULL;
    int32_t rc;

    // Shouldn't be allowed to specify no options...
    rc = iecs_forceDiscardClientState(pThreadData,
                                      "Client1",
                                      0);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);

    rc = ism_engine_createClientState("Client1",
                                      PROTOCOL_ID_MQTT,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      &hClient1,
                                      clientStateStealCallback,
                                      NULL,
                                      &hClient1,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient1);

    // Discard that client forcibly.
    rc = iecs_forceDiscardClientState(pThreadData,
                                      "Client1",
                                      iecsFORCE_DISCARD_CLIENT_OPTION_NON_ACKING_CLIENT);
    TEST_ASSERT_EQUAL(rc, OK);

    // Shouldn't be there any more...
    rc = iecs_forceDiscardClientState(pThreadData,
                                      "Client1",
                                      iecsFORCE_DISCARD_CLIENT_OPTION_NON_ACKING_CLIENT);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // Do it again with a durable client (steal 1st time, then zombie, then a client which won't go away immediately)
    for(int32_t i=0; i<3; i++)
    {
        ismEngine_ClientState_t *pReleaseClient = NULL;

        rc = sync_ism_engine_createClientState("Client1",
                                               PROTOCOL_ID_MQTT,
                                               ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL |
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                               &hClient1,
                                               clientStateStealCallback,
                                               NULL,
                                               &hClient1);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hClient1);

        // Make it a zombie...
        if (i == 1)
        {
            rc = sync_ism_engine_destroyClientState(hClient1, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else if (i == 2)
        {
            pReleaseClient = (ismEngine_ClientState_t *)hClient1;

            // Force the forceDiscardClientState to go async...
            iecs_acquireClientStateReference(pReleaseClient);
        }

        rc = iecs_forceDiscardClientState(pThreadData,
                                          "Client1",
                                          iecsFORCE_DISCARD_CLIENT_OPTION_NON_ACKING_CLIENT);
        if (i == 2)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }
        else
        {
            if (rc != OK) TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);
        }

        // Wait for it to actually go away...
        int32_t waitCount = 0;
        do
        {
            usleep(100000);
            rc = iecs_forceDiscardClientState(pThreadData,
                                              "Client1",
                                              iecsFORCE_DISCARD_CLIENT_OPTION_NON_ACKING_CLIENT);
            if (++waitCount > 50) break;

            if (pReleaseClient != NULL)
            {
                // For the first 5 iterations, force a stack of iecs_forceDiscardClientStates
                // because the underlying clientState hasn't got to 0 useCount yet...
                if (waitCount < 5)
                {
                    if (rc != ISMRC_AsyncCompletion) TEST_ASSERT_EQUAL(rc, ISMRC_RequestInProgress);
                }
                // Now let the underlying clientState go and everything should unwind...
                else if (waitCount == 5)
                {
                    iecs_releaseClientStateReference(pThreadData, pReleaseClient, false, false);
                }
            }
        }
        while(rc != ISMRC_NotFound);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }
}

static void callbackCreateClientStateComplete(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext)
{
    ismEngine_ClientStateHandle_t **ppHandle = (ismEngine_ClientStateHandle_t **)pContext;
    ismEngine_ClientStateHandle_t *pHandle = *ppHandle;
    ismEngine_ClientState_t *pClient = (ismEngine_ClientState_t *)handle;
    *pHandle = handle;

    if (pClient->Durability == iecsDurable)
    {
        TEST_ASSERT_EQUAL(retcode, ISMRC_ResumedClientState);
    }
    else
    {
        TEST_ASSERT_EQUAL(retcode, OK);
    }

}


static void callbackDestroyClientStateComplete(
    int32_t                         retcode,
    void *                          handle,
    void *                          pContext)
{
    //This function sometimes is called after the end of the test. This
    //causes CUnit to be very unhappy and fail. We could add an actions inflight
    //count and cause the test to wait for this callback but instead I've removed the
    //cunit assert.
    assert(retcode == OK);
    if (retcode != OK)
    {
        printf("Expect retcode OK from destroying client but received : %d\n", retcode);
        abort();
    }
}

static bool deliveryCallbackStealClientState(
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
    callbackContext_t **ppCallbackContext = (callbackContext_t **)pConsumerContext;
    callbackContext_t *pCallbackContext = *ppCallbackContext;
    int32_t rc = OK;

    // We got a message!
    pCallbackContext->MessageCount++;

    ism_engine_releaseMessage(hMessage);

    rc = ism_engine_createClientState("Client1",
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                      pCallbackContext->phClient,
                                      clientStateStealCallback,
                                      NULL,
                                      pCallbackContext->phClient,
                                      &pCallbackContext->phClient,
                                      sizeof(pCallbackContext->phClient),
                                      callbackCreateClientStateComplete);
    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    // We shouldn't really get a second message since we've just stolen our own
    // client-state but we'll ask for one anyway
    return true;
}

static void clientStateNoStealCallback(
    int32_t                         reason,
    ismEngine_ClientStateHandle_t   hClient,
    uint32_t                        options,
    void *                          pContext)
{
    // No-op
}

static void clientStateStealCallback(
    int32_t                         reason,
    ismEngine_ClientStateHandle_t   hClient,
    uint32_t                        options,
    void *                          pContext)
{
    ismEngine_ClientStateHandle_t *pHandle = (ismEngine_ClientStateHandle_t *)pContext;
    int32_t rc = OK;

    if (reason != ISMRC_ResumedClientState) TEST_ASSERT_EQUAL(reason, ISMRC_NonAckingClient);
    TEST_ASSERT_EQUAL(options, ismENGINE_STEAL_CALLBACK_OPTION_NONE);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // We have a tame victim - let's check that it's us, just to drive the code that follows thieves!
    iecsMessageDeliveryInfoHandle_t hVictimMsgDelInfo = NULL;

    rc = iecs_findClientMsgDelInfo(pThreadData,
                                   ((ismEngine_ClientState_t *)hClient)->pClientId,
                                   &hVictimMsgDelInfo);
    TEST_ASSERT_EQUAL(hVictimMsgDelInfo, ((ismEngine_ClientState_t *)hClient)->hMsgDeliveryInfo);

    if (rc == OK)
    {
        TEST_ASSERT_PTR_NOT_NULL(hVictimMsgDelInfo);
        iecs_releaseMessageDeliveryInfoReference(pThreadData, hVictimMsgDelInfo);
    }
    else
    {
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       callbackDestroyClientStateComplete);
    TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    *pHandle = NULL;
}

static void unreleasedIdsCallback(
    uint32_t                        deliveryId,
    ismEngine_UnreleasedHandle_t    hUnrel,
    void *                          pContext)
{
    unreleasedContext_t *pCallbackContext = (unreleasedContext_t *)pContext;

    TEST_ASSERT_CUNIT(deliveryId >= 0, ("deliveryId >= 0"));
    TEST_ASSERT_CUNIT(deliveryId < UNREL_LIMIT, ("deliveryId < UNREL_LIMIT"));

    TEST_ASSERT_EQUAL(pCallbackContext->Found[deliveryId], false);
    pCallbackContext->Count++;
    pCallbackContext->Found[deliveryId] = true;
    pCallbackContext->hUnrel[deliveryId] = hUnrel;
}

static void durableSubsCallback(
    ismEngine_SubscriptionHandle_t            subHandle,
    const char *                              pSubName,
    const char *                              pTopicString,
    void *                                    properties,
    size_t                                    propertiesLength,
    const ismEngine_SubscriptionAttributes_t *pSubAttributes,
    uint32_t                                  consumerCount,
    void *                                    pContext)
{
    durableSubsContext_t *pCallbackContext = (durableSubsContext_t *)pContext;
    ismEngine_Subscription_t *subscription = (ismEngine_Subscription_t *)subHandle;

    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_NOT_EQUAL(subscription->useCount, 0);

    if (pCallbackContext->expectProperties)
    {
        TEST_ASSERT_PTR_NOT_NULL(properties);
        uint32_t SubIndex = ism_common_getIntProperty(properties, "ClientStateProperty", 0);
        TEST_ASSERT_NOT_EQUAL(SubIndex, 0);
        TEST_ASSERT_EQUAL(pCallbackContext->Found[SubIndex], false);
        pCallbackContext->Found[SubIndex] = true;
    }

    pCallbackContext->Count++;
}
