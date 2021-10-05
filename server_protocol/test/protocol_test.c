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

/*
 * File: protocol_test.c
 */

/* Programming Notes:
 *
 * Following steps provide the guidelines of adding new CUnit test function of ISM APIs to the program structure.
 * (searching for "programming" keyword to find all the labels).
 *
 * 1. Create an entry in "ISM_CUnit_tests" for passing the specific ISM APIs CUnit test function to CUnit framework.
 * 2. Create the prototype and the definition of the functions that need to be passed to CUnit framework.
 * 3. Define the number of the test iteration.
 * 4. Define the specific data structure for passing the parameters of the specific ISM API for the test.
 * 5. Define the prototype and the definition of the functions that are used by the function created in note 2.
 * 6. Create the test output message for the success ISM API tests
 * 7. Create the test output message for the failure ISM API tests
 * 8. Define the prototype and the definition of the parameter initialization functions of ISM API
 * 9. Define the prototype and the definition of the function, that calls the ISM API to carry out the test.
 *
 *
 */

#include <protocol_test.h>
#include <engine.h>
#include <admin.h>
#include <ismutil.h>

genericEngineAPI_CB_t g_genericEngineAPI_CB = NULL;
ism_engine_putMessageOnDestinationAPI_CB_t g_ism_engine_putMessageOnDestinationAPI_CB = NULL;
ism_engine_createMessageAPI_CB_t g_ism_engine_createMessageAPI_CB = NULL;
ism_engine_getRetainedMessageAPI_CB_t g_ism_engine_getRetainedMessageAPI_CB = NULL;

#define DISPLAY_VERBOSE		0
#define DISPLAY_QUIET		1

int g_verbose = 0;
int RC = 0;
int g_fake_max_expire = 0;

/*
 * Global Variables
 */
int display_mode = DISPLAY_VERBOSE;          /* initial display mode */
int default_display_mode = DISPLAY_VERBOSE;  /* default display mode */

int          g_entered_send_cb;
int          g_entered_close_cb;
int          g_entered_closed_cb;
int          g_entered_createSession;
int          g_entered_createConsumer;
int          g_entered_assocMsg_cb;
int          g_entered_createMsg;
int          g_entered_putMsg;
int          g_entered_releaseMsg;
int          g_entered_destroyConsumer;
int          g_entered_createClient;
int          g_entered_destroyClient;
int          g_entered_confirmDelivery;
int          g_entered_authenticate_user;
int          g_entered_authorize_user;
const char * g_cmd;
int          g_cmd_result;
char         g_retbuf [2048];
int          g_retlen;
int          g_ret_cmd;
int          g_qos;
uint8_t      g_authentication_fails;
uint8_t      g_authorization_fails;
int          g_entered_cancel_timer;
int          g_entered_startMessageDelivery;
int          g_entered_stopMessageDelivery;
int          g_entered_destroySession;
int          g_entered_createLocalTransaction;
int          g_entered_destroyProducer;
int          g_entered_putMessageWithDeliveryId;

int          g_allowdurable;
int          g_allowpersistentmessages;

/**
 * The initialization function for timer tests.
 * Assures all global variables used are set to or NULL.
 */
int init_fakeengine(void) {
    g_entered_send_cb = 0;
    g_entered_close_cb = 0;
    g_entered_closed_cb = 0;
    g_entered_createSession = 0;
    g_entered_startMessageDelivery = 0;
    g_entered_stopMessageDelivery = 0;
    g_entered_createConsumer = 0;
    g_entered_assocMsg_cb = 0;
    g_entered_putMsg = 0;
    g_entered_releaseMsg = 0;
    g_entered_destroyConsumer = 0;
    g_entered_createClient = 0;
    g_entered_destroyClient = 0;
    g_authentication_fails = 0;
    g_authorization_fails = 0;
    g_cmd = NULL;

    g_allowdurable = 1;
    g_allowpersistentmessages = 1;

    serverState = ISM_SERVER_RUNNING;

    return 0;
}

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
        if (!strcmpi(argv[1],"B") || !strcmpi(argv[1],"BASIC"))
           test_mode = BASIC_TEST_MODE;
        else if (!strcmpi(argv[1],"F") || !strcmpi(argv[1],"FULL"))
           test_mode = FULL_TEST_MODE;
        else
           test_mode = BYNAME_TEST_MODE;
    }
    if (argc > 2 && strlen(argv[2])==1) {
        if (*argv[2] == 'v')
            g_verbose = 1;
        if (*argv[2] == 'V')
            g_verbose = 2;
        if (*argv[2] == 'D')
            g_verbose = 3;
    }

    printf("Test mode is %s\n",test_mode == 0 ? "BASIC" : test_mode == 1 ? "FULL" : "BYNAME");

    ism_common_initTimers();

    if (test_mode != FULL_TEST_MODE)
        runsuite = ISM_Protocol_CUnit_basicsuites;
    else
        runsuite = ISM_Protocol_CUnit_allsuites;

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
                    int tlen;
                    testname = argv[i];
                    tlen = strlen(testname);
                    if (tlen < 3)
                        break;
                    printf("looking for %s\n",testname);
                    testsuite = CU_get_suite_by_name(testname,testregistry);
                    if (NULL != testsuite) {
                        CU_basic_run_suite(testsuite);
                        testsrun++;
                    } else {
                        int j;
                        testsuite = testregistry->pSuite;
                        for (j = 0; j < testregistry->uiNumberOfSuites; j++) {
                            testcase = CU_get_test_by_name(testname, testsuite);
                            if (testcase) {
                                CU_basic_run_test(testsuite, testcase);
                                break;
                            } else if (tlen<20) {
                                char nbuf[20];
                                memset(nbuf, ' ', 19);
                                nbuf[19] = 0;
                                memcpy(nbuf, testname, tlen);
                                testcase = CU_get_test_by_name(nbuf, testsuite);
                                if (testcase) {
                                    CU_basic_run_test(testsuite, testcase);
                                    break;
                                }
                            }
                            testsuite = testsuite->pNext;
                        }
                    }
                }
            }
        }
    }

    ism_common_stopTimers();
    usleep(50000);
}


/*
 * This routine closes CUnit environment, and needs to be called only before the exiting of the program.
 */
void term(void) {
	CU_cleanup_registry();
}

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

void ism_common_initUtil(void);

/*
 * Main entry point
 */
int main(int argc, char * * argv) {
	int trclvl = 0;
/* TODO: We probably want to add better command line parsing so that
 *       verbose display and similar options can be specified on the
 *       command line.
#ifdef _WIN32
	if (argc>2) {
		__debugbreak();
	}
#endif
	if (argc > 1 && *argv[1]=='v')
		display_mode = DISPLAY_VERBOSE;
	if (argc > 1 && *argv[1]=='q')
		display_mode = DISPLAY_QUIET;
	if (argc > 1 && *argv[1]=='t') {
		display_mode = DISPLAY_VERBOSE;
		trclvl = 7;
	}
*/
	setenv("CUNIT", "42", 1);
    ism_common_initUtil();
    ism_common_initTimers();
    ism_security_init();
    ism_protocol_init();
    ism_admin_init(ISM_PROTYPE_SERVER);

	printf("%s\n", ism_common_getPlatformInfo());
	printf("%s\n\n", ism_common_getProcessorInfo());
	ism_common_setTraceLevel(trclvl);

	/* Run tests */
	startup(argc, argv);

	/* Print results */
	summary("\n\n Test: --- Testing ISM Protocols --- ...\n");
	print_final_summary();

	term();
	return RC;
}

/**
 * Engine function stubs.
 */
XAPI int ism_transport_registerProtocol(ism_transport_onStartMessaging_t onStart, ism_transport_onConnection_t onConnection) {
    return 0;
}

static int32_t randomInformationalRC(int32_t rc)
{
    if (rc == OK) {
        int rval = rand()%100;
        if (rval > 95) {
            rc = ISMRC_SomeDestinationsFull;
        } else if (rval >= 90) {
            rc = ISMRC_NoMatchingDestinations;
        } else if (rval >= 85) {
            rc = ISMRC_NoMatchingLocalDestinations;
        }
    }
    return rc;
}

XAPI int32_t ism_engine_createClientState(
    const char *                    pClientId,
    uint32_t                        protocolId,
    uint32_t                        options,
    void *                          pStealContext,
    ismEngine_StealCallback_t       pStealCallbackFn,
    ismSecurity_t *                 pSecContext,
    ismEngine_ClientStateHandle_t * phClient,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc=0;
    g_entered_createClient = 1;
    if (g_verbose > 2)
        printf("engine create client: %s\n", pClientId);
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)pClientId, (void *)&options, (void **)phClient);
    } else {
        // Default action...
        *phClient = (void *)1;
    }
    return rc;
}

XAPI int32_t ism_engine_putMessageOnDestination(ismEngine_SessionHandle_t hSession,
                                                ismDestinationType_t destinationType,
                                                const char *pDestinationName,
                                                ismEngine_TransactionHandle_t hTran,
                                                ismEngine_MessageHandle_t pMessage,
                                                void * pContext,
                                                size_t contextLength,
                                                ismEngine_CompletionCallback_t cb) {

    int32_t rc=0;
    g_entered_putMsg = 1;
    if (g_verbose > 2)
        printf("engine put message: %s\n", pDestinationName);
    if (g_ism_engine_putMessageOnDestinationAPI_CB) {
        rc = g_ism_engine_putMessageOnDestinationAPI_CB(hSession,
                                                        destinationType,
                                                        pDestinationName,
                                                        hTran,
                                                        pMessage,
                                                        pContext,
                                                        contextLength,
                                                        cb);
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, hSession, pMessage, NULL);
    } else {
        // Default action...
    }
    return randomInformationalRC(rc);
}

XAPI int32_t ism_engine_putMessageWithDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_ProducerHandle_t      hProducer,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_MessageHandle_t       hMessage,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn) {
    int32_t rc=0;
    g_entered_putMessageWithDeliveryId = 1;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hSession, (void *)hProducer, (void **)phUnrel);
    } else {
        // Default action...
    }
    return randomInformationalRC(rc);
}

XAPI int32_t ism_engine_putMessageWithDeliveryIdOnDestination(ismEngine_SessionHandle_t hSession, ismDestinationType_t destinationType,
        const char *pDestinationName, ismEngine_TransactionHandle_t hTran, ismEngine_MessageHandle_t pMessage,
        uint32_t unrelDeliveryId, ismEngine_UnreleasedHandle_t *  phUnrel, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb) {
    int32_t rc=0;
    g_entered_putMsg = 1;
    if (g_verbose > 2)
        printf("engine put message on dest: %s\n", pDestinationName);
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hSession, (void *)pDestinationName, (void **)phUnrel);
    } else {
        // Default action...
        int slashcnt = 0;
        const char * topic = pDestinationName;
        while (*topic) {
            if (*topic++ == '/') {
                if (++slashcnt>31) {
                    rc = ISMRC_DestNotValid;
                    break;
                }
            }
        }
    }
    return randomInformationalRC(rc);
}

XAPI int32_t ism_engine_createConsumer(ismEngine_SessionHandle_t hSession, ismDestinationType_t destinationType,
        const char *pDestinationName, const ismEngine_SubscriptionAttributes_t *pSubAttributes, ismEngine_ClientStateHandle_t hOwningClient,
        void * pMessageContext, size_t messageContextLength, ismEngine_MessageCallback_t pMessageCallbackFn,
        const ism_prop_t * pConsumerProperties, uint32_t consumerOptions, ismEngine_ConsumerHandle_t * phConsumer,
        void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb) {
    int32_t rc=0;
    g_entered_createConsumer++;
    if (g_verbose > 2)
        printf("engine create consumer: %s\n", pDestinationName);
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hSession, (void *)pDestinationName, (void **)phConsumer);
    } else {
        // Default action...
        *phConsumer = (ismEngine_ConsumerHandle_t)-1;
    }
    return rc;
}

XAPI int32_t ism_engine_destroyConsumer(ismEngine_ConsumerHandle_t hConsumer, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb) {
    int32_t rc=0;
    g_entered_destroyConsumer = 1;
    if (g_verbose > 2)
        printf("engine destroy consumer\n");
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hConsumer, NULL, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_createMessage(ismMessageHeader_t *pHeader,
                                      uint8_t areaCount,
                                      ismMessageAreaType_t areaTypes[areaCount],
                                      size_t areaLengths[areaCount],
                                      void * pAreaData[areaCount],
                                      ismEngine_MessageHandle_t * phMessage) {

    int32_t rc=0;
    g_entered_createMsg = 1;

    if (g_ism_engine_createMessageAPI_CB) {
        rc = g_ism_engine_createMessageAPI_CB(pHeader,
                                              areaCount,
                                              areaTypes,
                                              areaLengths,
                                              pAreaData,
                                              phMessage);
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, NULL, 0, pHeader, pAreaData, (void **)phMessage);
    } else {
        // Default action...
        if(phMessage)
            *phMessage = (void*)-1;
    }
    return rc;
}

XAPI int32_t ism_engine_startMessageDelivery(ismEngine_SessionHandle_t hSession, uint32_t options, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb) {
    int32_t rc=0;
    g_entered_startMessageDelivery += 1;
    if(g_verbose > 2)
        printf("start messages\n");
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, hSession, &options, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_createSession(ismEngine_ClientStateHandle_t hClient, uint32_t options, ismEngine_SessionHandle_t * phSession, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb) {
    int32_t rc=0;
    g_entered_createSession = 1;
    if(g_verbose > 2)
        printf("create session\n");
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hClient, (void *)&options, (void **)phSession);
    } else {
        // Default action...
        *phSession = (void *)1;
    }
    return rc;
}

XAPI int32_t ism_engine_destroySession(ismEngine_SessionHandle_t hSession, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb) {
    int32_t rc=0;
    g_entered_destroySession = 1;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hSession, NULL, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI void ism_engine_releaseMessage(ismEngine_MessageHandle_t pMessage) {
    g_entered_releaseMsg = 1;
    if (g_verbose > 2)
        printf("release message\n");
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        (void)g_genericEngineAPI_CB(__func__, NULL, 0, pMessage, NULL, NULL);
    } else {
        // Default action...
    }
    return;
}

XAPI int32_t ism_engine_destroyClientState(ismEngine_ClientStateHandle_t hClient, uint32_t options, void * pContext, size_t contextLength, ismEngine_CompletionCallback_t cb) {
    int32_t rc=0;
    g_entered_destroyClient = 1;
    if (g_verbose > 2)
        printf("engine close client\n");
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, hClient, &options, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_setWillMessage(
        ismEngine_ClientStateHandle_t   hClient,
        ismDestinationType_t            destinationType,
        const char *                    pDestinationName,
        ismEngine_MessageHandle_t       hMessage,
        uint32_t                        delayInterval,
        uint32_t                        timeToLive,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, hClient, hMessage, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_unsetWillMessage(
        ismEngine_ClientStateHandle_t   hClient,
        void *                          pContext,
        size_t                          contextLength,
        ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, hClient, NULL, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_addUnreleasedDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    uint32_t                        unrelDeliveryId,
    ismEngine_UnreleasedHandle_t *  phUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hSession, (void *)&unrelDeliveryId, (void **)phUnrel);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_removeUnreleasedDeliveryId(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_UnreleasedHandle_t    hUnrel,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, hSession, hUnrel, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_createSubscription(
    ismEngine_ClientStateHandle_t             hClient,
    const char *                              pSubName,
    const ism_prop_t *                        pSubProperties,
    uint8_t                                   destinationType,
    const char *                              pDestinationName,
    const ismEngine_SubscriptionAttributes_t *pSubAttributes,
    ismEngine_ClientStateHandle_t             hOwningClient,
    void *                                    pContext,
    size_t                                    contextLength,
    ismEngine_CompletionCallback_t            pCallbackFn) {

    int32_t rc = 0;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hClient, (void *)pDestinationName, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_destroySubscription(
    ismEngine_ClientStateHandle_t    hClient,
    const char *                     pSubName,
    ismEngine_ClientStateHandle_t    hOwningClient,
    void *                           pContext,
    size_t                           contextLength,
    ismEngine_CompletionCallback_t   pCallbackFn) {

    int32_t rc = 0;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hClient, (void *)pSubName, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_listSubscriptions(
    ismEngine_ClientStateHandle_t    hClient,
    const char *                     pSubName,
    void *                           pContext,
    ismEngine_SubscriptionCallback_t pCallbackFn) {

    int32_t rc = 0;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, 0, (void *)hClient, (void *)pSubName, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_confirmMessageDelivery(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t   hTran,
    ismEngine_DeliveryHandle_t      hDelivery,
    uint32_t                        options,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    g_entered_confirmDelivery = 1;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hSession, (void *)&hDelivery, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_stopMessageDelivery(
    ismEngine_SessionHandle_t       hSession,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    g_entered_stopMessageDelivery = 1;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hSession, NULL, NULL);
    } else {
        // Default action...
    }
	return rc;
}

XAPI int32_t ism_engine_setMessageDeliveryId(
    ismEngine_ConsumerHandle_t      hConsumer,
    ismEngine_DeliveryHandle_t      hDelivery,
    uint32_t                        deliveryId,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hConsumer, (void *)&deliveryId, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI void ism_security_context_setMaxSessionExpiry(ismSecurity_t *sContext, uint32_t maxExpiry);

XAPI void ism_security_authenticate_user_async(ismSecurity_t * sContext, const char * username, int username_len,
												const char *password, int password_len,
												void *pContext, int pContext_size,
                                                ismSecurity_AuthenticationCallback_t pCallbackFn, int authnRequired, ism_time_t delay){
    void * cbParam = malloc(pContext_size);
    memcpy(cbParam, pContext, pContext_size);
	g_entered_authenticate_user = 1;
	if (!g_authentication_fails) {
		g_cmd_result= ISMRC_OK;
	} else {
		g_cmd_result=ISMRC_NotAuthenticated;
		free(cbParam);
		return;
	}
	if (g_fake_max_expire) {
	    ism_security_context_setMaxSessionExpiry(sContext, g_fake_max_expire);
	}

	pCallbackFn(g_cmd_result, cbParam);
	free(cbParam);
    return;

}


XAPI int32_t ism_security_validate_policy(ismSecurity_t *secContext,
		ismSecurityAuthObjectType_t objectType,
		const char * objectName,
		ismSecurityAuthActionType_t actionType,
		ism_ConfigComponentType_t compType,
		void ** context) {
	g_entered_authorize_user = 1;
	if (!g_authorization_fails)
		return  ISMRC_OK;
	else
		return ISMRC_NotAuthorized;
}

XAPI int   ism_common_cancelTimerInt(ism_timer_t timer, const char * file, int line) {
	g_entered_cancel_timer = 1;

	return 0;
}

XAPI int32_t ism_engine_createLocalTransaction(
    ismEngine_SessionHandle_t       hSession,
    ismEngine_TransactionHandle_t * phTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    g_entered_createLocalTransaction++;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hSession, NULL, NULL);
    } else {
        // Default action...
        if (phTran) {
            *phTran = (ismEngine_TransactionHandle_t)1;
        }
    }
    return rc;
}

XAPI int32_t ism_engine_listUnreleasedDeliveryIds(
    ismEngine_ClientStateHandle_t   hClient,
    void *                          pContext,
    ismEngine_UnreleasedCallback_t  pUnrelCallbackFunction) {

    int32_t rc = 0;
    g_entered_createLocalTransaction++;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, 0, (void *)hClient, NULL, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_createProducer(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    ismEngine_ProducerHandle_t *    phProducer,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hSession, (void *)pDestinationName, (void **)phProducer);
    } else {
        // Default action...
        *phProducer = (void *)1;
    }
    return rc;
}

XAPI int32_t ism_engine_destroyProducer(
    ismEngine_ProducerHandle_t      hProducer,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    g_entered_destroyProducer = 1;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hProducer, NULL, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_unsetRetainedMessageOnDestination(
    ismEngine_SessionHandle_t       hSession,
    ismDestinationType_t            destinationType,
    const char *                    pDestinationName,
    uint32_t                        options,
    uint64_t                        serverTime,
    ismEngine_TransactionHandle_t   hTran,
    void *                          pContext,
    size_t                          contextLength,
    ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hSession, (void *)pDestinationName, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI int32_t ism_engine_getRetainedMessage(
           ismEngine_SessionHandle_t       hSession,
           const char *                    topicString,
           void *                          pMessageContext,
           size_t                          messageContextLength,
           ismEngine_MessageCallback_t     pMessageCallbackFn,
           void *                          pContext,
           size_t                          contextLength,
           ismEngine_CompletionCallback_t  pCallbackFn) {

    int32_t rc = 0;
    if (g_ism_engine_getRetainedMessageAPI_CB) {
        rc = g_ism_engine_getRetainedMessageAPI_CB(hSession,
                                                   topicString,
                                                   pMessageContext,
                                                   messageContextLength,
                                                   pMessageCallbackFn,
                                                   pContext,
                                                   contextLength,
                                                   pCallbackFn);
    } else if (g_genericEngineAPI_CB) {
        rc = g_genericEngineAPI_CB(__func__, pContext, contextLength, (void *)hSession, (void *)topicString, NULL);
    } else {
        // Default action...
    }
    return rc;
}

XAPI void ism_engine_getMessagingStatistics(ismEngine_MessagingStatistics_t *pStatistics) {
    if (0) {
        //TODO: Consider implementing a specific API callback for this API
    } else if (g_genericEngineAPI_CB) {
        g_genericEngineAPI_CB(__func__, NULL, 0, NULL, NULL, (void *)pStatistics);
    } else {
        // Default action...
    }
    return;
}

XAPI void ism_transport_checkClientLiveness(const char * clientID, uint32_t excludeConnection) {
    return;
}


XAPI int ism_security_context_getAllowDurable(ismSecurity_t *sContext) {
    return g_allowdurable;
}

XAPI int ism_security_context_getAllowPersistentMessages(ismSecurity_t *sContext) {
    return g_allowpersistentmessages;
}
