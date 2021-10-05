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


extern int g_verbose;
int trclvl = 4;

#include <forwarder.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

void testfwdconnect(void);
void ism_common_setServerName(const char * name);
void ism_common_setServerUID(const char * name);

/**
 * Test array for API tests.
 */
extern CU_TestInfo ISM_Protocol_CUnit_FwdConnect[];

CU_TestInfo ISM_Protocol_CUnit_FwdConnect[] = {
    {"FwdConnect",     testfwdconnect },
    CU_TEST_INFO_NULL
};

/*
 * Array that carries the test suite and other functions to the CUnit framework
 */
CU_SuiteInfo ISM_Protocol_CUnit_basicsuites[] = {
    IMA_TEST_SUITE("ProtocolFwdConnect", NULL,            NULL, ISM_Protocol_CUnit_FwdConnect),
    CU_SUITE_INFO_NULL,
};

/*
 * Global Variables
 */
#define DISPLAY_VERBOSE     0
#define DISPLAY_QUIET       1
#define BASIC_TEST_MODE       0
#define FULL_TEST_MODE        1
#define BYNAME_TEST_MODE      2

int g_verbose = 0;
int RC = 0;

/*
 * Global Variables
 */
int display_mode = DISPLAY_VERBOSE;          /* initial display mode */
int default_display_mode = DISPLAY_VERBOSE;  /* default display mode */
int test_mode = BASIC_TEST_MODE;
int default_test_mode = BASIC_TEST_MODE;

ism_transport_t * z_transport;
ismEngine_ClientStateHandle_t z_client;
ismEngine_SessionHandle_t z_sessionh;
int z_count1 = 0;
int z_count2 = 0;
int z_countx = 0;

/*
 * This is the main CUnit routine that starts the CUnit framework.
 * CU_basic_run_tests() Actually runs all the test routines.
 */
static void startup(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    CU_SuiteInfo * runsuite;
    CU_pTestRegistry testregistry;
    CU_pSuite testsuite;
    CU_pTest testcase;
    int testsrun = 0;

    if (argc > 1) {
        if (!strcmp(argv[1],"F") || !strcmp(argv[1],"FULL"))
           test_mode = FULL_TEST_MODE;
        else
           test_mode = BYNAME_TEST_MODE;
    }

    printf("Test mode is %s\n",test_mode == 0 ? "BASIC" : test_mode == 1 ? "FULL" : "BYNAME");

    ism_common_initTimers();

    if (test_mode == BASIC_TEST_MODE)
        runsuite = ISM_Protocol_CUnit_basicsuites;
    else
        // Load all tests for both FULL and BYNAME.
        // This way BYNAME can search all suite and test names
        // to find the tests to run.
        runsuite = ISM_Protocol_CUnit_basicsuites;

    // display_mode &= default_display_mode;            // disable the CU_BRM_SILENT mode
    setvbuf(stdout, NULL, _IONBF, 0);
    if (CU_initialize_registry() == CUE_SUCCESS) {
        if (CU_register_suites(runsuite) == CUE_SUCCESS) {
            if (display_mode == DISPLAY_VERBOSE)
                CU_basic_set_mode(CU_BRM_VERBOSE);
            else
                CU_basic_set_mode(CU_BRM_SILENT);
            if (test_mode != BYNAME_TEST_MODE)
                CU_basic_run_tests();
            else {
                int i;
                char * testname = NULL;
                testregistry = CU_get_registry();
                for (i = 1; i < argc; i++) {
                    testname = argv[i];
                    //printf("looking for %s\n",testname);
                    testsuite = CU_get_suite_by_name(testname,testregistry);
                    if (NULL != testsuite) {
                        //printf("found suite %s\n",testname);
                        CU_basic_run_suite(testsuite);
                        testsrun++;
                    } else {
                        int j;
                        testsuite = testregistry->pSuite;
            //printf("looking for %s in %s\n",testname,testsuite->pName);
                        for (j = 0; j < testregistry->uiNumberOfSuites; j++) {
                            testcase = CU_get_test_by_name(testname,testsuite);
                            if (NULL != testcase) {
                //printf("found test %s\n",testname);
                                CU_basic_run_test(testsuite,testcase);
                                break;
                            }
                            testsuite = testsuite->pNext;
                            //printf("looking for %s in %s\n",testname,testsuite->pName);
                        }
                    }
                }
            }
        }
    }

    ism_common_stopTimers();
}


/*
 * This routine closes CUnit environment, and needs to be called only before the exiting of the program.
 */
void term(void) {
    CU_cleanup_registry();
}

void ism_common_initUtil(void);


/*
 * This routine displays the statistics for the test run in silent mode.
 * CUnit does not display any data for the test result logged as "success".
 */
static void summary(char * headline) {
    CU_RunSummary *pCU_pRunSummary;

    pCU_pRunSummary = CU_get_run_summary();
    if (display_mode != DISPLAY_VERBOSE) {
        printf("%s", headline);
        printf("\n--Run Summary: Type       Total     Ran  Passed  Failed\n");
        printf("               tests     %5d   %5d   %5d   %5d\n",
                pCU_pRunSummary->nTestsRun + 1, pCU_pRunSummary->nTestsRun + 1,
                pCU_pRunSummary->nTestsRun + 1 - pCU_pRunSummary->nTestsFailed,
                pCU_pRunSummary->nTestsFailed);
        printf("               asserts   %5d   %5d   %5d   %5d\n",
                pCU_pRunSummary->nAsserts, pCU_pRunSummary->nAsserts,
                pCU_pRunSummary->nAsserts - pCU_pRunSummary->nAssertsFailed,
                pCU_pRunSummary->nAssertsFailed);
    }
}

/*
 * This routine prints out the final test sun status. The final test run status will be scanned by the
 * build process to determine if the build is successful.
 * Please do not change the format of the output.
 */
void print_final_summary(void) {
    CU_RunSummary * CU_pRunSummary_Final;
    CU_pRunSummary_Final = CU_get_run_summary();
    printf("\n\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
    CU_pRunSummary_Final->nTestsRun,
    CU_pRunSummary_Final->nTestsFailed,
    CU_pRunSummary_Final->nAssertsFailed);
    RC = CU_pRunSummary_Final->nTestsFailed + CU_pRunSummary_Final->nAssertsFailed;
}

/*
 * Main entry point
 */
int main(int argc, char * * argv) {

    /* Run tests */
    startup(argc, argv);

    /* Print results */
    summary("\n\n Test: --- Testing ISM Forwarding --- ...\n");
    print_final_summary();

    term();
    return RC;
}


/*
 * We got a message
 */
static bool replyMessage(ismEngine_ConsumerHandle_t consumerh,
        ismEngine_DeliveryHandle_t deliveryh, ismEngine_MessageHandle_t msgh,
        uint32_t seqnum, ismMessageState_t state, uint32_t options,
        ismMessageHeader_t * hdr, uint8_t areas,
        ismMessageAreaType_t areatype[areas], size_t areasize[areas],
        void * areaptr[areas], void * vaction) {
    uint32_t zz = *(uint32_t *)(vaction);
    printf("got message: sub=%d msg=%s\n", zz, (char *)areaptr[1]);
    switch (zz) {
    case 1:  z_count1++; break;
    case 2:  z_count2++; break;
    default: z_countx++; break;
    }
    return true;
}

/*
 * Make a few engine objects
 */
int make_engine_objects(void) {
    int  rc = 0;
    int  options;
    uint32_t zz;
    ismEngine_ConsumerHandle_t consumerh = NULL;
    ismEngine_ConsumerHandle_t consumerh2 = NULL;

    z_transport = ism_transport_newTransport(ism_fwd_getOutEndpoint(), 0, 0);
    z_transport->protocol_family = "fwdtest";
    z_transport->protocol = "fwd";
    TRACE(5, "===== Make engine objects\n");
    rc = ism_security_create_context(ismSEC_POLICY_CONNECTION, z_transport, &z_transport->security_context);
    CU_ASSERT(rc == 0);
    if (rc)
        return rc;
    TRACE(5, "===== Make client state\n");
    rc = ism_engine_createClientState("fwdtest", PROTOCOL_ID_MQTT, 0,
            NULL, NULL, z_transport->security_context, &z_client,
            NULL, 0, NULL);

    CU_ASSERT(rc == 0);
    if (rc)
        return rc;
    options = 0;
    TRACE(5, "===== Make session\n");
    rc = ism_engine_createSession(z_client, options, &z_sessionh, NULL, 0, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(z_sessionh != NULL);
    if (rc)
        return rc;

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
    int consumerOpt = 0;
    zz = 1;
    rc = ism_engine_createConsumer(z_sessionh, ismDESTINATION_TOPIC, "#",
            &subAttrs, z_client, &zz, sizeof zz, replyMessage,
            NULL, consumerOpt, &consumerh, NULL, 0, NULL);
    CU_ASSERT(rc == 0);
    if (rc)
        return rc;
    zz = 2;
    rc = ism_engine_createConsumer(z_sessionh, ismDESTINATION_TOPIC,  "$SYS/#",
            &subAttrs, z_client, &zz, sizeof zz, replyMessage,
            NULL, consumerOpt, &consumerh2, NULL, 0, NULL);
    CU_ASSERT(rc == 0);
    if (rc)
        return rc;
    rc = ism_engine_startMessageDelivery(z_sessionh, 0, NULL, 0, NULL);
    CU_ASSERT(rc == 0);
    TRACE(5, "==== Create test engine objects complets\n");
    return rc;
}

static ismMessageAreaType_t MsgAreas[2] = {
    ismMESSAGE_AREA_PROPERTIES,
    ismMESSAGE_AREA_PAYLOAD
};

/*
 * Make a message
 *
 */
ismEngine_MessageHandle_t make_msg(int qos, const char * topic, const char * body) {
    int  rc = 0;
    ismEngine_MessageHandle_t msgh;
    ismMessageHeader_t hdr;
    char xbuf [1024];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    size_t areasize[2];
    void * areaptr[2];

    memset(&hdr, 0, sizeof hdr);
    hdr.Persistence = qos>0  ? ismMESSAGE_PERSISTENCE_PERSISTENT : ismMESSAGE_PERSISTENCE_NONPERSISTENT;
    hdr.Reliability = qos&3;
    hdr.Priority = 4;
    hdr.RedeliveryCount = 0;
    hdr.Expiry = 0;
    hdr.Flags = 0;
    hdr.MessageType = MTYPE_MQTT_Text;

    ism_protocol_putNameIndex(&buf, ID_Topic);
    ism_protocol_putStringLenValue(&buf, topic, strlen(topic));

    /* Set the properties and body area */
    areasize[0] = buf.used;
    areaptr[0] = buf.buf;
    areasize[1] = body ? strlen(body)+1 : 0;
    areaptr[1] = (char *)body;

    /*
     * Create the message
     */
    rc = ism_engine_createMessage(&hdr, 2, MsgAreas, areasize, areaptr, &msgh);
    CU_ASSERT(rc == 0);
    return msgh;
}

int ism_store_init(void);
int ism_store_start(void);

int ism_cluster_registerEngineEventCallback(ismEngine_RemoteServerCallback_t callback, void * pContext) {
    printf("Fake ism_cluster_registerEngineEventCallback\n");
    return 0;
}
int32_t ism_cluster_restoreRemoteServers(const ismCluster_RemoteServerData_t *pServersData, int numServers) {
    printf("Fake ism_cluster_restoreRemoteServers\n");
    return 0;
}
int32_t ism_cluster_recoveryCompleted(void) {
    printf("Fake ism_cluster_recoveryCompleted\n");
    return 0;
}
int32_t ism_cluster_addSubscriptions(const ismCluster_SubscriptionInfo_t *pSubInfo, int numSubs) {
    printf("Fake ism_cluster_addSubscriptions\n");
    return 0;
}
int32_t ism_cluster_routeLookup(ismCluster_LookupInfo_t *pLookupInfo) {
    printf("Fake ism_cluster_routeLookup\n");
    return 0;
}

/*
 * Make a forwarder connection and send a message
 */
void testfwdconnect(void) {
    ism_field_t f;
    int  rc;
    ismEngine_MessageHandle_t msgh;
    ismEngine_MessageHandle_t msgh2;


    /* Do some basic init */
    setenv("CUNIT", "42", 1);
    if (getenv("CUNIT"))
        return;

    ism_common_setTraceLevel(trclvl);
    ism_common_initUtil();

    /* Set up the config */
    f.type = VT_Integer;
    f.val.i = 1;
    ism_common_setProperty( ism_common_getConfigProperties(), "DisableAuthentication", &f);
    f.val.i = 1111;
    ism_common_setProperty( ism_common_getConfigProperties(), "ClusterDataLocalPort", &f);
    f.val.i = 1;
    ism_common_setProperty( ism_common_getConfigProperties(), "ClusterCUnit", &f);
    f.type = VT_String;
    f.val.s = "127.0.0.1";
    ism_common_setProperty( ism_common_getConfigProperties(), "ClusterDataLocalAddr", &f);
    f.val.s = "False";
    ism_common_setProperty( ism_common_getConfigProperties(), "Store.EnableDiskPersistence", &f);
    f.val.s = "fwd0,fwd012345,0,127.0.0.1,1111";
    ism_common_setProperty( ism_common_getConfigProperties(), "RemoteServer.0", &f);
    f.val.s = "fwd1,fwd123456,0,127.0.0.1,1111";
    ism_common_setProperty( ism_common_getConfigProperties(), "RemoteServer.1", &f);

    ism_common_setServerName("ThisServer");
    ism_common_setServerUID("TS_123456");

    /* Init components I need */
    rc = ism_config_init();
    CU_ASSERT(rc == 0);
    if (rc)
        return;

    ism_common_initTimers();
    ism_security_init();

    rc = ism_transport_init();
    CU_ASSERT(rc == 0);
    if (rc)
        return;
    rc = ism_store_init();
    CU_ASSERT(rc == 0);
    if (rc)
        return;
    rc = ism_engine_init();
    CU_ASSERT(rc == 0);
    if (rc)
        return;
    rc = ism_protocol_init();
    CU_ASSERT(rc == 0);
    if (rc)
        return;

    printf("%s\n", ism_common_getPlatformInfo());
    printf("%s\n\n", ism_common_getProcessorInfo());

    /* Start components I need */
    rc = ism_transport_start();
    rc = ism_store_start();
    CU_ASSERT(rc == 0);
    if (rc)
        return;
    rc = ism_engine_start();
    CU_ASSERT(rc == 0);
    if (rc)
        return;
    rc = ism_protocol_start();
    CU_ASSERT(rc == 0);
    if (rc)
        return;
    rc = ism_engine_startMessaging();
    CU_ASSERT(rc == 0);
    if (rc)
        return;
    rc = ism_transport_startMessaging();
    CU_ASSERT(rc == 0);
    if (rc)
        return;

    /* Let init complete */
    ism_common_sleep(100000);

    /* Set up a client session */
    TRACE(5, "===== Init complete\n");
    rc = make_engine_objects();
    CU_ASSERT(rc == 0);
    if (rc)
        return;


    /* Create a message */
     msgh  = make_msg(0, "my/topic", "Now is the time");
     msgh2 = make_msg(0, "my/topic", "QoS=1 message");
     CU_ASSERT(msgh != NULL);

    /* Send a message */
     TRACE(5, "===== Put msg1\n");
     rc = ism_engine_putMessageOnDestination(z_sessionh, ismDESTINATION_TOPIC, "my/topic",
         NULL, msgh, NULL, 0, NULL);
     CU_ASSERT(rc == 0);
     TRACE(5, "===== Put msg2\n");
     rc = ism_engine_putMessageOnDestination(z_sessionh, ismDESTINATION_TOPIC, "my/topic",
         NULL, msgh2, NULL, 0, NULL);
     CU_ASSERT(rc == 0);
     /* Wait a bit */
     ism_common_sleep(100000);
     CU_ASSERT(z_count1==6);
     /* Check received */
     printf("count1=%d count2=%d countx=%d\n", z_count1, z_count2, z_countx);

     char xbuf[16];
     concat_alloc_t buf = {xbuf, sizeof xbuf};
     CU_ASSERT(ism_fwd_getForwarderStats(&buf, 1) == 2);
     printf("%s\n", buf.buf);
}
