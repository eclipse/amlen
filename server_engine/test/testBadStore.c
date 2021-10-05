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
/* Module Name: test_badStore                                        */
/*                                                                   */
/* Description: Test program which deliberately writes bad data to   */
/* the store and ensures that we can recover.                        */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <getopt.h>
#include <sys/prctl.h>
#include <sys/stat.h>

#include "engineInternal.h"
#include "engineStore.h"
#include "clientState.h"
#include "topicTree.h"
#include "queueNamespace.h"

#include "test_utils_initterm.h"
#include "test_utils_log.h"
#include "test_utils_assert.h"
#include "test_utils_file.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_config.h"
#include "test_utils_client.h"

/*********************************************************************/
/* Global data                                                       */
/*********************************************************************/
const char *goodClientId = "GoodClient";
const char *duplicateClientId = "DuplicateClient";
const char *futureClientId1 = "FutureClient1";
const char *futureClientId2 = "FutureClient2";
const char *duplicateSubName =  "DuplicateSub";
const char *futureSubName1 = "FutureSub1";
const char *futureSubName2 = "FutureSub2";
const char *futureQueue1 = "FutureQueue1";
const char *futureQueue2 = "FutureQueue2";

uint64_t nonFatalFFDCs = 0;
uint64_t fatalFFDCs = 0;

uint64_t currentlyExpectedFatalFFDCs    = 0;
uint64_t currentlyExpectedNonFatalFFDCs = 0;

void ieut_ffdc( const char *function
              , uint32_t seqId
              , bool core
              , const char *filename
              , uint32_t lineNo
              , char *label
              , uint32_t retcode
              , ... )
{
    if (core)
    {
        uint64_t oldExpectedVal = __sync_fetch_and_sub(&currentlyExpectedFatalFFDCs, 1);

        if (oldExpectedVal < 1)
        {
            //fatal FFDC when we don't expect one.
            abort();
        }

        __sync_add_and_fetch(&fatalFFDCs, 1);
    }
    else
    {
        uint64_t oldExpectedVal = __sync_fetch_and_sub(&currentlyExpectedNonFatalFFDCs, 1);

        if (oldExpectedVal < 1)
        {
            //non fatal FFDC when we don't expect one.
            abort();
        }

        __sync_add_and_fetch(&nonFatalFFDCs, 1);
    }
}

void writeClient(const char *clientId, uint32_t CSRVersion, uint32_t CPRVersion)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismStore_Record_t CSRStoreRecord;
    ismStore_Record_t CPRStoreRecord;
    iestClientStateRecord_t clientStateRec;
    iestClientPropertiesRecord_t clientPropsRec;
    uint32_t dataLength;
    uint32_t fragsCount;
    char *pFrags[2];
    uint32_t fragsLength[2];
    int32_t rc = OK;

    // First fragment is always the CPR itself
    pFrags[0] = (char *)&clientPropsRec;
    fragsLength[0] = sizeof(clientPropsRec);

    // Fill in the store record
    CPRStoreRecord.Type = ISM_STORE_RECTYPE_CPROP;
    CPRStoreRecord.Attribute = ismSTORE_NULL_HANDLE;
    CPRStoreRecord.State = iestCPR_STATE_NONE;
    CPRStoreRecord.pFrags = &pFrags[0];
    CPRStoreRecord.pFragsLengths = &fragsLength[0];
    CPRStoreRecord.FragsCount = 1;
    CPRStoreRecord.DataLength = fragsLength[0];

    TEST_ASSERT(iestCPR_CURRENT_VERSION == iestCPR_VERSION_5, ("CPR version has changed, update required"));

    // Build the subscription properties record from the various fragment sources
    memcpy(clientPropsRec.StrucId, iestCLIENT_PROPS_RECORD_STRUCID, 4);
    clientPropsRec.Version = CPRVersion;
    clientPropsRec.Flags = iestCPR_FLAGS_DURABLE;
    clientPropsRec.WillTopicNameLength = 0;
    clientPropsRec.WillMsgTimeToLive = 0;
    clientPropsRec.UserIdLength = 0;
    clientPropsRec.ExpiryInterval = iecsEXPIRY_INTERVAL_INFINITE;
    clientPropsRec.WillDelay = 0;

    ismStore_Handle_t hStoreCPR = ismSTORE_NULL_HANDLE;

    rc = ism_store_createRecord(pThreadData->hStream,
                                &CPRStoreRecord,
                                &hStoreCPR);

    TEST_ASSERT(rc == OK, ("creating CPR failed. rc = %d", rc));

    // Now create a CSR
    fragsCount = 1;
    pFrags[0] = (char *)&clientStateRec;
    fragsLength[0] = sizeof(clientStateRec);
    dataLength = sizeof(clientStateRec);


    fragsCount++;
    pFrags[1] = (char *)clientId;
    fragsLength[1] = strlen(clientId) + 1;
    dataLength += fragsLength[1];

    memcpy(clientStateRec.StrucId, iestCLIENT_STATE_RECORD_STRUCID, 4);
    clientStateRec.Version = CSRVersion;
    clientStateRec.Flags = iestCSR_FLAGS_DURABLE;
    clientStateRec.ClientIdLength = fragsLength[1];
    clientStateRec.protocolId = testDEFAULT_PROTOCOL_ID;

    CSRStoreRecord.Type = ISM_STORE_RECTYPE_CLIENT;
    CSRStoreRecord.FragsCount = fragsCount;
    CSRStoreRecord.pFrags = pFrags;
    CSRStoreRecord.pFragsLengths = fragsLength;
    CSRStoreRecord.DataLength = dataLength;
    CSRStoreRecord.Attribute = hStoreCPR;
    CSRStoreRecord.State = iestCSR_STATE_NONE;

    ismStore_Handle_t hStoreCSR = ismSTORE_NULL_HANDLE;

    rc = ism_store_createRecord(pThreadData->hStream,
                                &CSRStoreRecord,
                                &hStoreCSR);

    TEST_ASSERT(rc == OK, ("creating CSR failed. rc = %d", rc));

    iest_store_commit(pThreadData, false);

    test_log(testLOGLEVEL_VERBOSE,"For clientid %s got store CSR: 0x%lx\n", clientId, hStoreCSR);
}

typedef struct tag_searchInfo_t {
    const char *StringToLookFor;
    bool foundIt;
} searchInfo_t;

bool isMatchingClient(ieutThreadData_t *pThreadData,
                      ismEngine_ClientState_t *pClient,
                      void *pContext)
{
    searchInfo_t *pCtxt = (searchInfo_t *)pContext;
    bool keepLooking = true;

    TEST_ASSERT(pClient->pClientId != NULL, ("ClientId is NULL for client %p", pClient));

    if (strcmp(pCtxt->StringToLookFor, pClient->pClientId) == 0)
    {
        pCtxt->foundIt = true;
        keepLooking = false;
    }

    return keepLooking;
}

//Currently looks at each client - could be more efficient!
void checkClientExists(const char *clientId)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    searchInfo_t ctxt = {clientId, false};

    iecs_traverseClientStateTable(pThreadData,
                                  NULL, 0, 0, NULL,
                                  isMatchingClient,
                                  &ctxt);

    TEST_ASSERT(ctxt.foundIt == true, ("client %s not found", clientId));
}

//Currently looks at each client - could be more efficient!
void checkClientMissing(const char *clientId)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    searchInfo_t ctxt = {clientId, false};

    iecs_traverseClientStateTable(pThreadData,
                                  NULL, 0, 0, NULL,
                                  isMatchingClient,
                                  &ctxt);

    TEST_ASSERT(ctxt.foundIt == false, ("client %s found", clientId));
}

void isMatchingSub(
    ieutThreadData_t                         *pThreadData,
    ismEngine_SubscriptionHandle_t            subHandle,
    const char *                              pSubName,
    const char *                              pTopicString,
    void *                                    properties,
    size_t                                    propertiesLength,
    const ismEngine_SubscriptionAttributes_t *pSubAttrs,
    uint32_t                                  consumerCount,
    void *                                    pContext)
{
    searchInfo_t *pCtxt = (searchInfo_t *)pContext;

    if (   pSubName != NULL
        && strcmp(pCtxt->StringToLookFor, pSubName) == 0)
    {
        pCtxt->foundIt = true;
    }
}

void checkSubExists(const char *clientId, const char *subName)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    searchInfo_t ctxt = {subName, false};

    // Call the internal routine to perform the listing
    int32_t rc = iett_listSubscriptions(pThreadData,
                                        clientId,
                                        iettFLAG_LISTSUBS_MATCH_SUBNAME,
                                        subName,
                                        &ctxt,
                                        isMatchingSub);

    TEST_ASSERT(rc == OK, ("rc was %d", rc));
    TEST_ASSERT(ctxt.foundIt == true, ("subName %s not found for client %s", subName, clientId));
}

void checkSubMissing(const char *clientId, const char *subName)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    searchInfo_t ctxt = {subName, false};

    // Call the internal routine to perform the listing
    int32_t rc = iett_listSubscriptions(pThreadData,
                                        clientId,
                                        iettFLAG_LISTSUBS_MATCH_SUBNAME,
                                        subName,
                                        &ctxt,
                                        isMatchingSub);

    TEST_ASSERT(rc == OK, ("rc was %d", rc));
    TEST_ASSERT(ctxt.foundIt == false, ("subName %s found for client %s", subName, clientId));
}

void checkQueueExists(const char *queueName)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ismQHandle_t queueHandle = ieqn_getQueueHandle(pThreadData, queueName);

    TEST_ASSERT(queueHandle != NULL, ("queue '%s' not found", queueName));
}

uint32_t override_QPR_VERSION = 0;
uint32_t override_QDR_VERSION = 0;
uint32_t override_SPR_VERSION = 0;
uint32_t override_SDR_VERSION = 0;
uint32_t override_SCR_VERSION = 0;
uint32_t override_TR_VERSION = 0;
int32_t ism_store_createRecord(ismStore_StreamHandle_t hStream,
                               const ismStore_Record_t *pRecord,
                               ismStore_Handle_t *pHandle)
{
    static int32_t (*real_ism_store_createRecord)(ismStore_StreamHandle_t,
                                                  const ismStore_Record_t *,
                                                  ismStore_Handle_t *) = NULL;

    if (real_ism_store_createRecord == NULL)
    {
        real_ism_store_createRecord = dlsym(RTLD_NEXT, "ism_store_createRecord");
    }

    if (override_SCR_VERSION != 0 && pRecord->Type == ISM_STORE_RECTYPE_SERVER)
    {
        iestServerConfigRecord_t *pSCR = (iestServerConfigRecord_t *)pRecord->pFrags[0];
        pSCR->Version = override_SCR_VERSION;
    }
    else if (override_QPR_VERSION != 0 && pRecord->Type == ISM_STORE_RECTYPE_QPROP)
    {
        iestQueuePropertiesRecord_t *pQPR = (iestQueuePropertiesRecord_t *)pRecord->pFrags[0];
        pQPR->Version = override_QPR_VERSION;
    }
    else if (override_QDR_VERSION != 0 && pRecord->Type == ISM_STORE_RECTYPE_QUEUE)
    {
        iestQueueDefinitionRecord_t *pQDR = (iestQueueDefinitionRecord_t *)pRecord->pFrags[0];
        pQDR->Version = override_QDR_VERSION;
    }
    else if (override_SPR_VERSION != 0 && pRecord->Type == ISM_STORE_RECTYPE_SPROP)
    {
        iestSubscriptionPropertiesRecord_t *pSPR = (iestSubscriptionPropertiesRecord_t *)pRecord->pFrags[0];
        pSPR->Version = override_SPR_VERSION;
    }
    else if (override_SDR_VERSION != 0 && pRecord->Type == ISM_STORE_RECTYPE_SUBSC)
    {
        iestSubscriptionDefinitionRecord_t *pSDR = (iestSubscriptionDefinitionRecord_t *)pRecord->pFrags[0];
        pSDR->Version = override_SDR_VERSION;
    }
    else if (override_TR_VERSION != 0 && pRecord->Type == ISM_STORE_RECTYPE_TRANS)
    {
        iestTransactionRecord_t *pTR = (iestTransactionRecord_t *)pRecord->pFrags[0];
        pTR->Version = override_TR_VERSION;
    }

    return real_ism_store_createRecord(hStream, pRecord, pHandle);
}

bool tolerateValue = false;
int set_tolerateRecoveryInconsistencies(void)
{
    int32_t rc;

    if (tolerateValue)
        rc = test_configProcessPost("{\"TolerateRecoveryInconsistencies\": true}");
    else
        rc = test_configProcessPost("{\"TolerateRecoveryInconsistencies\": false}");

    TEST_ASSERT(rc == OK, ("ERROR: test_configProcessPost (%d) returned %d",
                           (int)tolerateValue, rc));

    return rc;
}

void phase0(void)
{
    /*************************************************************/
    /* Start up the Engine - Cold Start                          */
    /*************************************************************/
    int32_t rc = test_engineInit_COLD;
    TEST_ASSERT(rc == OK, ("cold start failed. rc = %d", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine cold started");

    //Write Some genuinely good data to the store
    writeClient(goodClientId, iestCSR_CURRENT_VERSION, iestCPR_CURRENT_VERSION);

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));
}

void phase1(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start");
    int32_t rc = test_engineInit_WARMAUTO;
    TEST_ASSERT(rc == OK, ("warm start failed. rc = %d", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");

    //Check that the data from phase 0 still exists
    checkClientExists(goodClientId);

    //Now write deliberate duplicate clients to the store
    writeClient(duplicateClientId, iestCSR_CURRENT_VERSION, iestCPR_CURRENT_VERSION);
    writeClient(duplicateClientId, iestCSR_CURRENT_VERSION, iestCPR_CURRENT_VERSION);

    //Write a future client to the store
    TEST_ASSERT(iestCPR_CURRENT_VERSION < 9999, ("CPR Version is >= 9999 -- need to update test."));
    writeClient(futureClientId1, iestCSR_CURRENT_VERSION, 9999);
    TEST_ASSERT(iestCSR_CURRENT_VERSION < 9999, ("CSR Version is >= 9999 -- need to update test."));
    writeClient(futureClientId1, 9999, iestCPR_CURRENT_VERSION);

    ismEngine_ClientState_t *hClient;
    ismEngine_Session_t *hSession;
    ismEngine_Transaction_t *pTran;

    rc = test_createClientAndSession(goodClientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient,
                                     &hSession,
                                     false);
    TEST_ASSERT(rc == OK, ("rc == %d", rc));

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    override_TR_VERSION = 9999;
    TEST_ASSERT(iestTR_CURRENT_VERSION < override_TR_VERSION,
                ("TR Version is >= %u -- need to update test.", override_TR_VERSION));

    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 1234;
    globalId.bqual = 5678;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    rc = ietr_createGlobal(pThreadData,
                           hSession,
                           &XID,
                           ismENGINE_CREATE_TRANSACTION_OPTION_DEFAULT,
                           NULL,
                           &pTran);
    TEST_ASSERT(rc == OK, ("rc == %d", rc));
    override_TR_VERSION = 0;

    rc = ietr_prepare(pThreadData, pTran, hSession, NULL);
    TEST_ASSERT(rc == OK, ("rc == %d", rc));

    rc = ietr_findGlobalTransaction(pThreadData, &XID, &pTran);
    TEST_ASSERT(rc == OK, ("rc == %d", rc));

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));
}

void phase2(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start - expecting failure");
    currentlyExpectedFatalFFDCs = 1; //Recovery FDCs by default if it finds inconsistencies in the store

    int32_t rc = test_engineInit(false, true,
                          false,
                          true, /*recovery should fail*/
                          ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                          testDEFAULT_STORE_SIZE);
    TEST_ASSERT(rc != OK, ("warm start rc unexpected. rc = %d (expected != OK)", rc));

    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm start failed as expected");

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));

    //Failure to start results in an FFDC which would would normally bring the server down
    TEST_ASSERT(fatalFFDCs == 1,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));

    /* Despite failure set Admin mode to production, mode = 0 so next phase tries to read store again */
    rc = ism_admin_setAdminMode(0, 0);
    TEST_ASSERT(rc == OK, ("resetting mode to production failed. rc = %d", rc));
}

void phase3(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start (with Tolerate Inconsistencies)");

    // Request the setting of the tolerateRecoveryInconsistencies property to true
    tolerateValue = true;
    test_engineInit_betweenInitAndStartFunc = set_tolerateRecoveryInconsistencies;

    int32_t rc = test_engineInit_WARMAUTO;
    TEST_ASSERT(rc == OK, ("warm start failed. rc = %d", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");

    //Check that the data from phase 0 still exists
    checkClientExists(goodClientId);
    //Check that one of the duplicate clients still exists
    checkClientExists(duplicateClientId);
    //Check that neither of the future clients exist
    checkClientMissing(futureClientId1);
    checkClientMissing(futureClientId2);
    //Check that the transaction is not found
    ism_xid_t XID;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    memset(&XID, 0, sizeof(ism_xid_t));
    XID.formatID = 0;
    XID.gtrid_length = sizeof(uint64_t);
    XID.bqual_length = sizeof(uint64_t);
    globalId.gtrid = 1234;
    globalId.bqual = 5678;
    memcpy(&XID.data, &globalId, sizeof(globalId));

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_Transaction_t *pTran;

    rc = ietr_findGlobalTransaction(pThreadData, &XID, &pTran);
    TEST_ASSERT(rc == ISMRC_NotFound, ("rc == %d", rc));

    // Write a future client back, but as though it had come back in this releas
    writeClient(futureClientId1, iestCSR_CURRENT_VERSION, iestCPR_CURRENT_VERSION);

    // Make sure we unset the tolerateRecoveryInconsistencies property
    test_engineInit_betweenInitAndStartFunc = NULL;
    tolerateValue = false;
    set_tolerateRecoveryInconsistencies();

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));
}

void phase4(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start");
    int32_t rc = test_engineInit_WARMAUTO;
    TEST_ASSERT(rc == OK, ("warm start failed. rc = %d", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");

    //Check that the data from phase 0 still exists
    checkClientExists(goodClientId);
    //Check that one of the duplicate clients still exists
    checkClientExists(duplicateClientId);
    //Check that the reconnected future client exists
    checkClientExists(futureClientId1);
    //Check that the originally removed future client is still missing
    checkClientMissing(futureClientId2);

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));

    //Write duplicate subscriptions out
    ismEngine_Subscription_t subscription = {{0},0};
    iepiPolicyInfo_t         policyInfo = {{0},0};
    memset(&subscription, 0, sizeof(subscription));
    memset(&policyInfo, 0, sizeof(policyInfo));

    subscription.clientId = (char *)duplicateClientId;
    subscription.subName  = (char *)duplicateSubName;
    subscription.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
    subscription.internalAttrs = iettSUBATTR_PERSISTENT;
    policyInfo.name = "fakePolicy";
    policyInfo.maxMessageCount = 100;
    policyInfo.maxMsgBehavior  = DiscardOldMessages;

    char *topicString = "dupSubTopicStr";

    ismStore_Handle_t hStoreSubDefn = ismSTORE_NULL_HANDLE;
    ismStore_Handle_t hStoreSubProps = ismSTORE_NULL_HANDLE;

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // Add duplicate subs and subs for future client which have a future SDR or SPR
    for(int32_t i=0; i<4; i++)
    {
        switch(i)
        {
            case 0:
            case 1:
                break;
            case 2:
                TEST_ASSERT(iestSPR_CURRENT_VERSION < 9999, ("SPR Version is >= 9999 -- need to update test."));
                override_SPR_VERSION = 9999;
                subscription.clientId = (char *)futureClientId1;
                subscription.subName = (char *)futureSubName1;
                break;
            case 3:
                TEST_ASSERT(iestSDR_CURRENT_VERSION < 9999, ("SDR Version is >= 9999 -- need to update test."));
                override_SDR_VERSION = 9999;
                subscription.clientId = (char *)futureClientId1;
                subscription.subName = (char *)futureSubName2;
                break;
        }

        rc = iest_storeSubscription(pThreadData,
                                    topicString,
                                    strlen(topicString),
                                    &subscription,
                                    strlen(subscription.clientId)+1,
                                    strlen(subscription.subName)+1,
                                    0,
                                    multiConsumer,
                                    &policyInfo,
                                    &hStoreSubDefn,
                                    &hStoreSubProps);

        override_SPR_VERSION = override_SDR_VERSION = 0;

        TEST_ASSERT(rc == OK, ("rc was %d", rc));
        test_log(testLOGLEVEL_VERBOSE,"For sub1 with: %s,%s got store SDR: 0x%lx\n",
                 subscription.clientId, subscription.subName, hStoreSubDefn);

        iest_store_commit(pThreadData, false);

        rc = ism_store_updateRecord(pThreadData->hStream,
                                    hStoreSubDefn,
                                    0,
                                    iestSDR_STATE_NONE,
                                    ismSTORE_SET_STATE);

        TEST_ASSERT(rc == OK, ("rc was %d", rc));

        iest_store_commit(pThreadData, false);
    }

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));
}

void phase5(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start - expecting failure");
    currentlyExpectedFatalFFDCs = 1; //Recovery FDCs by default if it finds inconsistencies in the store

    int32_t rc = test_engineInit(false, true,
                          false,
                          true, /*recovery should fail*/
                          ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                          testDEFAULT_STORE_SIZE);
    TEST_ASSERT(rc != OK, ("warm start rc unexpected. rc = %d (expected OK)", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm start failed as expected");

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));

    //Failure to start results in an FFDC which would would normally bring the server down
    TEST_ASSERT(fatalFFDCs == 1,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));

    /* Despite failure set Admin mode to production, mode = 0 so next phase tries to read store again */
    rc = ism_admin_setAdminMode(0, 0);
    TEST_ASSERT(rc == OK, ("resetting mode to production failed. rc = %d", rc));
}

void phase6(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start (with Tolerate Inconsistencies)");
    // Request the setting of the tolerateRecoveryInconsistencies property to true
    tolerateValue = true;
    test_engineInit_betweenInitAndStartFunc = set_tolerateRecoveryInconsistencies;
    int32_t rc = test_engineInit_WARMAUTO;
    TEST_ASSERT(rc == OK, ("warm start failed. rc = %d", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");

    //Check that the data from phase 0 still exists
    checkClientExists(goodClientId);
    //Check that one of the duplicate clients still exists
    checkClientExists(duplicateClientId);
    //Check that the expected future client still exists
    checkClientExists(futureClientId1);
    //Check that duplicateSubName exists...
    checkSubExists(duplicateClientId, duplicateSubName);
    //Check that futureSubName1 doesn't exist
    checkSubMissing(futureClientId1, futureSubName1);
    //Check that futureSubName2 doesn't exist
    checkSubMissing(futureClientId1, futureSubName2);

    // Make sure we unset the tolerateRecoveryInconsistencies property
    test_engineInit_betweenInitAndStartFunc = NULL;
    tolerateValue = false;
    set_tolerateRecoveryInconsistencies();

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));
}

void phase7(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start");
    int32_t rc = test_engineInit_WARMAUTO;
    TEST_ASSERT(rc == OK, ("warm start failed. rc = %d", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");

    //Check that the data from phase 0 still exists
    checkClientExists(goodClientId);
    //Check that one of the duplicate clients still exists
    checkClientExists(duplicateClientId);
    //Check that the expected future client still exists
    checkClientExists(futureClientId1);
    //One copy of the sub should still exist.
    checkSubExists(duplicateClientId, duplicateSubName);
    //Check that futureSubName1 doesn't exist
    checkSubMissing(futureClientId1, futureSubName1);
    //Check that futureSubName2 doesn't exist
    checkSubMissing(futureClientId1, futureSubName2);

    //Write a some future queues
    override_QPR_VERSION = 9999;
    TEST_ASSERT(iestQPR_CURRENT_VERSION < override_QPR_VERSION,
                ("QPR Version is >= %u -- need to update test.", override_QPR_VERSION));
    rc = test_configProcessPost("{\"Queue\":{\"FutureQueue1\":{\"Description\":\"Future Queue\"}}}");
    TEST_ASSERT(rc == OK, ("Failed to create %s rc = %d", futureQueue1, rc));
    override_QPR_VERSION = 0;

    override_QDR_VERSION = 9999;
    TEST_ASSERT(iestQDR_CURRENT_VERSION < override_QDR_VERSION,
                ("QDR Version is >= %u -- need to update test.", override_QDR_VERSION));
    rc = test_configProcessPost("{\"Queue\":{\"FutureQueue2\":{\"Description\":\"Future Queue\"}}}");
    TEST_ASSERT(rc == OK, ("Failed to create %s rc = %d", futureQueue2, rc));
    override_QDR_VERSION = 0;

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));
}

void phase8(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start (with Tolerate Inconsistencies)");
    // Request the setting of the tolerateRecoveryInconsistencies property to true
    tolerateValue = true;
    test_engineInit_betweenInitAndStartFunc = set_tolerateRecoveryInconsistencies;
    int32_t rc = test_engineInit_WARMAUTO;
    TEST_ASSERT(rc == OK, ("warm start failed. rc = %d", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");

    //Check that the data from phase 0 still exists
    checkClientExists(goodClientId);
    //Check that one of the duplicate clients still exists
    checkClientExists(duplicateClientId);
    //Check that the expected future client still exists
    checkClientExists(futureClientId1);
    //Check that duplicateSubName exists...
    checkSubExists(duplicateClientId, duplicateSubName);
    //Check that futureSubName1 doesn't exist
    checkSubMissing(futureClientId1, futureSubName1);
    //Check that futureSubName2 doesn't exist
    checkSubMissing(futureClientId1, futureSubName2);
    //Check that the future queues exist (they will have been recreated because there is still an admin definition)
    checkQueueExists(futureQueue1);
    checkQueueExists(futureQueue2);

    // Make sure we unset the tolerateRecoveryInconsistencies property
    test_engineInit_betweenInitAndStartFunc = NULL;
    tolerateValue = false;
    set_tolerateRecoveryInconsistencies();

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));
}

void phase9(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start");
    int32_t rc = test_engineInit_WARMAUTO;
    TEST_ASSERT(rc == OK, ("warm start failed. rc = %d", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");

    //Check that the data from phase 0 still exists
    checkClientExists(goodClientId);
    //Check that one of the duplicate clients still exists
    checkClientExists(duplicateClientId);
    //Check that the expected future client still exists
    checkClientExists(futureClientId1);
    //One copy of the sub should still exist.
    checkSubExists(duplicateClientId, duplicateSubName);
    //Check that futureSubName1 doesn't exist
    checkSubMissing(futureClientId1, futureSubName1);
    //Check that futureSubName2 doesn't exist
    checkSubMissing(futureClientId1, futureSubName2);
    //Check that the future queues still exist
    checkQueueExists(futureQueue1);
    checkQueueExists(futureQueue2);

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));
}

void phase10(void)
{
    override_SCR_VERSION = 9999;
    TEST_ASSERT(iestSCR_CURRENT_VERSION < override_SCR_VERSION,
                ("SCR Version is >= %u -- need to update test.", override_SCR_VERSION));

    /*************************************************************/
    /* Start up the Engine - Cold Start                          */
    /*************************************************************/
    int32_t rc = test_engineInit_COLD;
    TEST_ASSERT(rc == OK, ("cold start failed. rc = %d", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine cold started");

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));

    //Check that the data from pre-cold phases have gone
    checkClientMissing(goodClientId);
    checkClientMissing(duplicateClientId);
    checkClientMissing(futureClientId1);
    checkClientMissing(futureClientId2);
    // Queues will have been recreated because of admin entry
    checkQueueExists(futureQueue1);
    checkQueueExists(futureQueue2);

    // Check we have an SCR
    TEST_ASSERT(ismEngine_serverGlobal.hStoreSCR != ismSTORE_NULL_HANDLE, ("NULL hStoreSCR"));
    override_SCR_VERSION = 0;
}

void phase11(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start - expecting failure");
    currentlyExpectedFatalFFDCs = 1; //Recovery FDCs by default if it finds inconsistencies in the store

    int32_t rc = test_engineInit(false, true,
                          false,
                          true, /*recovery should fail*/
                          ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                          testDEFAULT_STORE_SIZE);
    TEST_ASSERT(rc != OK, ("warm start rc unexpected. rc = %d (expected != OK)", rc));

    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm start failed as expected");

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));

    //Failure to start results in an FFDC which would would normally bring the server down
    TEST_ASSERT(fatalFFDCs == 1,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));

    /* Despite failure set Admin mode to production, mode = 0 so next phase tries to read store again */
    rc = ism_admin_setAdminMode(0, 0);
    TEST_ASSERT(rc == OK, ("resetting mode to production failed. rc = %d", rc));
}

void phase12(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start (with Tolerate Inconsistencies)");

    // Request the setting of the tolerateRecoveryInconsistencies property to true
    tolerateValue = true;
    test_engineInit_betweenInitAndStartFunc = set_tolerateRecoveryInconsistencies;

    int32_t rc = test_engineInit_WARMAUTO;
    TEST_ASSERT(rc == OK, ("warm start failed. rc = %d", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");

    // Make sure we unset the tolerateRecoveryInconsistencies property
    test_engineInit_betweenInitAndStartFunc = NULL;
    tolerateValue = false;
    set_tolerateRecoveryInconsistencies();

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));

    // Check we have an SCR
    TEST_ASSERT(ismEngine_serverGlobal.hStoreSCR != ismSTORE_NULL_HANDLE, ("NULL hStoreSCR"));
}

void phase13(void)
{
    test_log(testLOGLEVEL_TESTPROGRESS, "Starting engine - warm start");
    int32_t rc = test_engineInit_WARMAUTO;
    TEST_ASSERT(rc == OK, ("warm start failed. rc = %d", rc));
    test_log(testLOGLEVEL_TESTPROGRESS, "Engine warm started");

    // Check we have an SCR
    TEST_ASSERT(ismEngine_serverGlobal.hStoreSCR != ismSTORE_NULL_HANDLE, ("NULL hStoreSCR"));

    TEST_ASSERT(nonFatalFFDCs == 0, ("Unexpected nonFatalFFDCs %lu", nonFatalFFDCs));
    TEST_ASSERT(fatalFFDCs == 0,    ("Unexpected fatalFFDCs    %lu", fatalFFDCs));
}

void runPhase(uint32_t phaseNumber)
{
    switch(phaseNumber)
    {
        //Write good data to the store
        case 0: phase0(); break;
        //Check we can start and write duplicate clients to the store / bad version clients
        case 1: phase1(); break;
        //Check normal start fails
        case 2: phase2(); break;
        //Check Partial Store Recovery works
        case 3: phase3(); break;
        //Check normal start works now and write duplicate subs / bad version subs
        case 4: phase4(); break;
        //Check normal start fails
        case 5: phase5(); break;
        //Check Partial Store Recovery works
        case 6: phase6(); break;
        //Check normal start works after the Partial Store Recovery and add bad version queue
        case 7: phase7(); break;
        //Check normal start fails
        case 8: phase8(); break;
        //Check Partial Store Recovery works
        case 9: phase9(); break;
        //Cold start but ensuring that the SCR gets a future version number
        case 10: phase10(); break;
        //Check normal start fails
        case 11: phase11(); break;
        //Check Partial Store Recovery works
        case 12: phase12(); break;
        //Check normal start works
        case 13: phase13(); break;

        default:
            TEST_ASSERT(false, ("unexpected phase = %u", phaseNumber));
            break;
    }
}



int32_t parseArgs( int argc
                 , char *argv[]
                 , uint32_t *pphase
                 , uint32_t *pfinalphase
                 , testLogLevel_t *pverboseLevel
                 , int *ptrclvl
                 , char **padminDir )
{
    int usage = 0;
    char opt;
    struct option long_options[] = {
        { "phase", 1, NULL, 'p' },
        { "final", 1, NULL, 'f' },
        { "verbose", 1, NULL, 'v' },
        { "adminDir", 1, NULL, 'a' },
        { NULL, 1, NULL, 0 } };
    int long_index;

    *pphase = 0;
    *pfinalphase = 13;
    *pverboseLevel = 3;
    *ptrclvl = 0;
    *padminDir = NULL;

    while ((opt = getopt_long(argc, argv, ":p:f:c:r:v:a:0123456789", long_options, &long_index)) != -1)
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
            case 'p':
               *pphase = (uint32_t)atoi(optarg);
               break;
            case 'f':
               *pfinalphase = (uint32_t)atoi(optarg);
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
    
    if (*pverboseLevel > testLOGLEVEL_VERBOSE) /* BEAM suppression: constant condition */
        *pverboseLevel=testLOGLEVEL_VERBOSE;

    if (usage)
    {
        fprintf(stderr, "Usage: %s [-v 0-9] [-0...-9] [-p phase] [-f final-phase] [-c count] [-r ratio]\n", argv[0]);
        fprintf(stderr, "       -p (--phase)         - Set initial phase\n");
        fprintf(stderr, "       -f (--final)         - Set final phase\n");
        fprintf(stderr, "       -v (--verbose)       - Test Case verbosity\n");
        fprintf(stderr, "       -a (--adminDir)      - Admin directory to use\n");
        fprintf(stderr, "       -0 .. -9             - ISM Trace level\n");
    }

   return usage;
}

int main(int argc, char *argv[])
{
    int trclvl = 5;
    testLogLevel_t testLogLevel = testLOGLEVEL_TESTDESC;
    int rc = 0;
    char *newargv[15];
    int i;
    uint32_t phase;
    uint32_t finalphase;
    time_t curTime;
    struct tm tm = { 0 };
    char *adminDir = NULL;

    /*****************************************************************/
    /* Parse arguments                                               */
    /*****************************************************************/
    rc = parseArgs( argc
                  , argv
                  , &phase
                  , &finalphase
                  , &testLogLevel
                  , &trclvl
                  , &adminDir );
    if (rc != 0)
    {
        goto mod_exit;
    }

    //Hack in increased diagnostics
    setenv("IMA_TEST_TRACE_LEVEL", "9", 1);
    testLogLevel = 9;

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

    /*************************************************************/
    /* Prepare the restart command                               */
    /*************************************************************/
    bool phasefound=false, adminDirfound=false;
    char textphase[16];
    sprintf(textphase, "%d", phase+1);
    TEST_ASSERT(argc < 12, ("argc was %d", argc));

    for (i=0; i < argc; i++)
    {
        newargv[i]=argv[i];
        if (strcmp(newargv[i], "-p") == 0)
        {
          newargv[i+1]=textphase;
          i++;
          phasefound=true;
        }
        if (strcmp(newargv[i], "-a") == 0)
        {
            newargv[i+1]=adminDir;
            i++;
            adminDirfound=true;
        }
    }

    if (!phasefound)
    {
        newargv[i++]="-p";
        newargv[i++]=textphase;
    }

    if (!adminDirfound)
    {
        newargv[i++]="-a";
        newargv[i++]=adminDir;
    }

    newargv[i]=NULL;

    /*****************************************************************/
    /* Seed the random number generator and prepare the time for     */
    /* printing                                                      */
    /*****************************************************************/
    curTime = time(NULL);
    srand(curTime);

    (void)localtime_r(&curTime, &tm);
    
    /*****************************************************************/
    /* If this is the first run for this test then cold start the    */
    /* store.                                                        */
    /*****************************************************************/
    if (phase == 0)
    {
        test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Running test phase 0",
                 tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    else
    {
        test_log(testLOGLEVEL_TESTDESC, "%d:%02d:%02d -- Running test phase %d",
                 tm.tm_hour, tm.tm_min, tm.tm_sec, phase);

        char *backupDir;

        if ((backupDir = getenv("BADSTORE_BACKUPDIR")) != NULL)
        {
            char *qualifyShared = getenv("QUALIFY_SHARED");

            // space for QUALIFY_SHARED value and com.ibm.ism::%u:store (and null terminator)
            char shmName[(qualifyShared?strlen(qualifyShared):0)+41];
            sprintf(shmName, "com.ibm.ism::%u:store%s", getuid(), qualifyShared ? qualifyShared : "");

            // space for /dev/shm (and null terminator)
            char sourceName[strlen(shmName)+10];
            sprintf(sourceName, "/dev/shm/%s", shmName);

            // space for / (and null terminator)
            char backupName[strlen(backupDir)+strlen(shmName)+2];
            sprintf(backupName, "%s/%s", backupDir, shmName);

            // Allow space for 50 bytes of command (and null terminator)
            char cmd[strlen(sourceName)+strlen(backupName)+51];
            struct stat statBuf;

            if (stat(backupDir, &statBuf) != 0)
            {
                (void)mkdir(backupDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            }

            if (stat(sourceName, &statBuf) == 0)
            {
                sprintf(cmd, "/bin/cp -f %s %s", sourceName, backupName);
                if (system(cmd) == 0)
                {
                    test_log(testLOGLEVEL_TESTPROGRESS,
                             "Copied store shm %s to %s\n", sourceName, backupName);
                }
            }
        }
    }

    runPhase(phase);


    /*****************************************************************/
    /* And restart the server.                                       */
    /*****************************************************************/
    if (phase < finalphase)
    {
        test_log(2, "Restarting...");
        rc = test_execv(newargv[0], newargv);
        TEST_ASSERT(false, ("'test_execv' failed. rc = %d", rc));
    }
    else
    {
        test_log(2, "Success. Test complete!\n");

        test_log(2, "Ending engine.");
        test_engineTerm(true);
        test_processTerm(false);
        test_removeAdminDir(adminDir);
    }

mod_exit:
    return (0);
}
