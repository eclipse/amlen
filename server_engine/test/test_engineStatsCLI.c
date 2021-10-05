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
/* Module Name: test_namedQueues.c                                   */
/*                                                                   */
/* Description: Main source file for CUnit test of Queue namespace   */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "engineInternal.h"

#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_monitoring.h"
#include "test_utils_security.h"

//****************************************************************************
/// @brief Test the behaviour engine subscription stat function via CLI
//****************************************************************************
#define BASICSUBSCRIPTION_POLICYNAME   "BasicSubscriptionPolicy"
#define BASICSUBSCRIPTION_CLIENTID     "BasicSubscriptionClient"
#define BASICSUBSCRIPTION_SUBNAME      "BasicSubscription"
#define BASICSUBSCRIPTION_TOPICSTRING  "Basic/Subscription/Topic"

void test_capability_BasicSubscriptions(void)
{
    int32_t rc = OK;

    char tbuf[2048] = {0};
    char inputString[1024];
    concat_alloc_t output = {0};

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;
    ismEngine_ConsumerHandle_t hConsumer=NULL;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    const char *testString;
    char expectString[1024];

    ism_json_parse_t parseObj = { 0 };
    ism_json_entry_t ents[20];

    parseObj.ent = ents;
    parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));

    rc = ism_json_parse(&parseObj);
    // Initialize security context, session and client
    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = BASICSUBSCRIPTION_CLIENTID;

    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."BASICSUBSCRIPTION_POLICYNAME"\","
                                "\"Name\":\""BASICSUBSCRIPTION_POLICYNAME"\","
                                "\"ClientID\":\""BASICSUBSCRIPTION_CLIENTID"\","
                                "\"Topic\":\"*\","
                                "\"ActionList\":\"publish,subscribe\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_addPolicyToSecContext(mockContext, BASICSUBSCRIPTION_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("Starting %s...\n", __func__);

    output.buf = tbuf;
    output.len = (int)sizeof(tbuf);

    // Use an invalid stat type
    output.used = 0;
    rc = test_getEngineStats(TEST_ACTION_SUBSCRIPTION,
                             "{"TEST_NAME_STATTYPE":\"INVALID\","
                                TEST_DEFAULT_SUBSCRIPTION_FILTER"}",
                             &output);
    TEST_ASSERT_EQUAL(rc, ISMRC_ArgNotValid);

    // Test the valid stat types when there are ZERO subscriptions
    char *statTypes[] = {TEST_VALUE_PUBLISHEDMSGSHIGHEST,
                         TEST_VALUE_PUBLISHEDMSGSLOWEST,
                         TEST_VALUE_BUFFEREDMSGSHIGHEST,
                         TEST_VALUE_BUFFEREDMSGSLOWEST,
                         TEST_VALUE_BUFFEREDPERCENTHIGHEST,
                         TEST_VALUE_BUFFEREDPERCENTLOWEST,
                         TEST_VALUE_REJECTEDMSGSHIGHEST,
                         TEST_VALUE_REJECTEDMSGSLOWEST,
                         TEST_VALUE_PUBLISHEDMSGSHIGHEST_SHORT,
                         TEST_VALUE_PUBLISHEDMSGSLOWEST_SHORT,};

    for(int32_t i=0; i<sizeof(statTypes)/sizeof(statTypes[0]); i++)
    {
        strcpy(inputString, "{"TEST_NAME_STATTYPE":");
        strcat(inputString, statTypes[i]);
        strcat(inputString, ","TEST_NAME_RESULTCOUNT":10");
        strcat(inputString, ","TEST_DEFAULT_SUBSCRIPTION_FILTER"}");

        output.used = 0;
        rc = test_getEngineStats(TEST_ACTION_SUBSCRIPTION,
                                 inputString,
                                 &output);

        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    // Create a single durable subscription
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                          ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    rc = ism_engine_createSubscription(hClient,
                                       BASICSUBSCRIPTION_SUBNAME,
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       BASICSUBSCRIPTION_TOPICSTRING,
                                       &subAttrs,
                                       NULL, // Owning client same as requesting client
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Retry all the various stat types with a single unused subscription, filtering on subName
    for(int32_t i=0; i<sizeof(statTypes)/sizeof(statTypes[0]); i++)
    {
        strcpy(inputString, "{"TEST_NAME_STATTYPE":");
        strcat(inputString, statTypes[i]);
        strcat(inputString, ","TEST_NAME_RESULTCOUNT":10");
        strcat(inputString, ","TEST_NAME_SUBNAME":\""BASICSUBSCRIPTION_SUBNAME"\"");
        strcat(inputString, ","TEST_NAME_CLIENTID":"TEST_VALUE_ASTERISK);
        strcat(inputString, ","TEST_NAME_TOPICSTRING":"TEST_VALUE_ASTERISK);
        strcat(inputString, ","TEST_NAME_SUBTYPE":"TEST_VALUE_ALL);
        strcat(inputString, "}");

        output.used = 0;
        rc = test_getEngineStats(TEST_ACTION_SUBSCRIPTION,
                                 inputString,
                                 &output);

        TEST_ASSERT_EQUAL(rc, OK);

        // Parse the returned string, skipping the opening/closing square brackets
        memset(&parseObj, 0, sizeof(parseObj));
        parseObj.ent = ents;
        parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
        parseObj.source = strdup(&(output.buf[1]));
        parseObj.src_len = output.used-2;

        rc = ism_json_parse(&parseObj);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check a subset of the returned values
        testString = ism_json_getString(&parseObj, ismENGINE_MONITOR_FILTER_SUBNAME);
        TEST_ASSERT_PTR_NOT_NULL(testString);
        TEST_ASSERT(strcmp(testString, BASICSUBSCRIPTION_SUBNAME) == 0,
                    ("SubName '%s' expected '"BASICSUBSCRIPTION_SUBNAME"'", testString));
        testString = ism_json_getString(&parseObj, ismENGINE_MONITOR_FILTER_TOPICSTRING);
        TEST_ASSERT(strcmp(testString, BASICSUBSCRIPTION_TOPICSTRING) == 0,
                    ("TopicString '%s' expected '"BASICSUBSCRIPTION_TOPICSTRING"'", testString));
        sprintf(expectString, "%lu", (uint64_t)0);
        testString = ism_json_getString(&parseObj, "BufferedMsgs");
        TEST_ASSERT(strcmp(testString, expectString) == 0,
                    ("BufferedMsgs '%s' expected '%s'", testString, expectString));
        sprintf(expectString, "%lu", (uint64_t)5000);
        testString = ism_json_getString(&parseObj, "MaxMessages");
        TEST_ASSERT(strcmp(testString, expectString) == 0,
                    ("MaxMessages '%s' expected '%s'", testString, expectString));
        sprintf(expectString, "%.1f", (double)0);
        testString = ism_json_getString(&parseObj, "BufferedPercent");
        TEST_ASSERT(strcmp(testString, expectString) == 0,
                    ("BufferedPercent '%s' expected '%s'", testString, expectString));
        testString = ism_json_getString(&parseObj, "IsDurable");
        TEST_ASSERT(strcmp(testString, "true") == 0, ("IsDurable '%s' expected 'true'", testString));

        free(parseObj.source);
    }

    // replace with a non-durable subscription
    rc = ism_engine_destroySubscription(hClient, BASICSUBSCRIPTION_SUBNAME, hClient, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   BASICSUBSCRIPTION_TOPICSTRING,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   NULL, 0, NULL,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    // Retry all the various stat types with a single unused non-durable, this time filtering on topic
    for(int32_t i=0; i<sizeof(statTypes)/sizeof(statTypes[0]); i++)
    {
        strcpy(inputString, "{"TEST_NAME_STATTYPE":");
        strcat(inputString, statTypes[i]);
        strcat(inputString, ","TEST_NAME_RESULTCOUNT":10");
        strcat(inputString, ","TEST_NAME_SUBNAME":"TEST_VALUE_ASTERISK);
        strcat(inputString, ","TEST_NAME_CLIENTID":"TEST_VALUE_ASTERISK);
        strcat(inputString, ","TEST_NAME_TOPICSTRING":\""BASICSUBSCRIPTION_TOPICSTRING"\"");
        strcat(inputString, ","TEST_NAME_SUBTYPE":"TEST_VALUE_ALL);
        strcat(inputString, "}");

        output.used = 0;
        rc = test_getEngineStats(TEST_ACTION_SUBSCRIPTION,
                                 inputString,
                                 &output);
        TEST_ASSERT_EQUAL(rc, OK);

        // Parse the returned string, skipping the opening/closing square brackets
        memset(&parseObj, 0, sizeof(parseObj));
        parseObj.ent = ents;
        parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
        parseObj.source = strdup(&output.buf[1]);
        parseObj.src_len = output.used-2;

        rc = ism_json_parse(&parseObj);
        TEST_ASSERT_EQUAL(rc, OK);

        // Check a subset of the returned values
        testString = ism_json_getString(&parseObj, ismENGINE_MONITOR_FILTER_SUBNAME);
        TEST_ASSERT_PTR_NOT_NULL(testString);
        TEST_ASSERT(testString[0]=='\0', ("SubName '%s' expected ''", testString));
        testString = ism_json_getString(&parseObj, ismENGINE_MONITOR_FILTER_TOPICSTRING);
        TEST_ASSERT(strcmp(testString, BASICSUBSCRIPTION_TOPICSTRING) == 0,
                    ("TopicString '%s' expected '"BASICSUBSCRIPTION_TOPICSTRING"'", testString));
        sprintf(expectString, "%lu", (uint64_t)0);
        testString = ism_json_getString(&parseObj, "BufferedMsgs");
        TEST_ASSERT(strcmp(testString, expectString) == 0,
                    ("BufferedMsgs '%s' expected '%s'", testString, expectString));
        sprintf(expectString, "%lu", (uint64_t)5000);
        testString = ism_json_getString(&parseObj, "MaxMessages");
        TEST_ASSERT(strcmp(testString, expectString) == 0,
                    ("MaxMessages '%s' expected '%s'", testString, expectString));
        sprintf(expectString, "%.1f", (double)0);
        testString = ism_json_getString(&parseObj, "BufferedPercent");
        TEST_ASSERT(strcmp(testString, expectString) == 0,
                    ("BufferedPercent '%s' expected '%s'", testString, expectString));
        testString = ism_json_getString(&parseObj, "IsDurable");
        TEST_ASSERT(strcmp(testString, "false") == 0, ("IsDurable '%s' expected 'false'", testString));

        free(parseObj.source);
    }

    // cleanup
    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo  ISM_EngineStatsCLI_CUnit_test_capability_Subscription[] =
{
    { "BasicSubscriptions", test_capability_BasicSubscriptions },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the behaviour engine queue stat function via CLI
//****************************************************************************
void test_capability_NoQueues(void)
{
    int32_t rc = OK;

    char tbuf[2048];
    char inputString[1024];
    concat_alloc_t output = { tbuf, sizeof(tbuf), 0, 0 };

    printf("Starting %s...\n", __func__);

    // Use an invalid stat type
    output.used = 0;
    rc = test_getEngineStats(TEST_ACTION_QUEUE,
                             "{"TEST_NAME_STATTYPE":\"INVALID\","
                                TEST_NAME_RESULTCOUNT":10,"
                                TEST_DEFAULT_QUEUE_FILTER"}",
                             &output);
    TEST_ASSERT_EQUAL(rc, ISMRC_ArgNotValid);

    // Test the valid stat types
    char *statTypes[] = {TEST_VALUE_BUFFEREDMSGSHIGHEST,
                         TEST_VALUE_BUFFEREDMSGSLOWEST,
                         TEST_VALUE_BUFFEREDPERCENTHIGHEST,
                         TEST_VALUE_BUFFEREDPERCENTLOWEST,
                         TEST_VALUE_PRODUCEDMSGSHIGHEST,
                         TEST_VALUE_PRODUCEDMSGSLOWEST,
                         TEST_VALUE_CONSUMEDMSGSHIGHEST,
                         TEST_VALUE_CONSUMEDMSGSLOWEST,
                         TEST_VALUE_CONSUMERSHIGHEST,
                         TEST_VALUE_CONSUMERSLOWEST,
                         TEST_VALUE_PRODUCERSHIGHEST,
                         TEST_VALUE_PRODUCERSLOWEST,
                         TEST_VALUE_BUFFEREDMSGSHIGHEST_SHORT,
                         TEST_VALUE_BUFFEREDMSGSLOWEST_SHORT,};

    for(int32_t i=0; i<sizeof(statTypes)/sizeof(statTypes[0]); i++)
    {
        strcpy(inputString, "{"TEST_NAME_STATTYPE":");
        strcat(inputString, statTypes[i]);
        strcat(inputString, ","TEST_NAME_RESULTCOUNT":10");
        strcat(inputString, ","TEST_DEFAULT_QUEUE_FILTER"}");

        output.used = 0;
        rc = test_getEngineStats(TEST_ACTION_QUEUE,
                                 inputString,
                                 &output);

        if (rc != ISMRC_NotFound) printf("statType; %s\n", statTypes[i]);

        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }
}

CU_TestInfo  ISM_EngineStatsCLI_CUnit_test_capability_Queue[] =
{
    { "NoQueues", test_capability_NoQueues },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the behaviour engine topic stat function via CLI
//****************************************************************************
void test_capability_NoTopics(void)
{
    int32_t rc = OK;

    char tbuf[2048];
    char inputString[1024];
    concat_alloc_t output = { tbuf, sizeof(tbuf), 0, 0 };

    printf("Starting %s...\n", __func__);

    // Use an invalid stat type
    output.used = 0;
    rc = test_getEngineStats(TEST_ACTION_TOPIC,
                             "{"TEST_NAME_STATTYPE":\"INVALID\","
                                TEST_NAME_RESULTCOUNT":10,"
                                TEST_DEFAULT_TOPIC_FILTER"}",
                             &output);
    TEST_ASSERT_EQUAL(rc, ISMRC_ArgNotValid);

    // Test the valid stat types
    char *statTypes[] = {TEST_VALUE_PUBLISHEDMSGSHIGHEST,
                         TEST_VALUE_PUBLISHEDMSGSLOWEST,
                         TEST_VALUE_SUBSCRIPTIONSHIGHEST,
                         TEST_VALUE_SUBSCRIPTIONSLOWEST,
                         TEST_VALUE_REJECTEDMSGSHIGHEST,
                         TEST_VALUE_REJECTEDMSGSLOWEST,
                         TEST_VALUE_FAILEDPUBLISHESHIGHEST,
                         TEST_VALUE_FAILEDPUBLISHESLOWEST};

    for(int32_t i=0; i<sizeof(statTypes)/sizeof(statTypes[0]); i++)
    {
        strcpy(inputString, "{"TEST_NAME_STATTYPE":");
        strcat(inputString, statTypes[i]);
        strcat(inputString, ","TEST_NAME_RESULTCOUNT":10");
        strcat(inputString, ","TEST_DEFAULT_TOPIC_FILTER"}");

        output.used = 0;
        rc = test_getEngineStats(TEST_ACTION_TOPIC,
                                 inputString,
                                 &output);

        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }
}

CU_TestInfo  ISM_EngineStatsCLI_CUnit_test_capability_Topic[] =
{
    { "NoTopics", test_capability_NoTopics },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the behaviour engine client stat function via CLI
//****************************************************************************
void test_capability_NoMQTTClients(void)
{
    int32_t rc = OK;

    char tbuf[2048];
    char inputString[1024];
    concat_alloc_t output = { tbuf, sizeof(tbuf), 0, 0 };

    printf("Starting %s...\n", __func__);

    // Use an invalid stat type
    output.used = 0;
    rc = test_getEngineStats(TEST_ACTION_MQTTCLIENT,
                             "{"TEST_NAME_STATTYPE":\"INVALID\","
                                TEST_NAME_RESULTCOUNT":10,"
                                TEST_DEFAULT_MQTTCLIENT_FILTER"}",
                             &output);
    TEST_ASSERT_EQUAL(rc, ISMRC_ArgNotValid);

    // Test the valid stat types
    char *statTypes[] = {TEST_VALUE_LASTCONNECTEDTIMEOLDEST};

    for(int32_t i=0; i<sizeof(statTypes)/sizeof(statTypes[0]); i++) /* BEAM suppression: loop doesn't iterate */
    {
        strcpy(inputString, "{"TEST_NAME_STATTYPE":");
        strcat(inputString, statTypes[i]);
        strcat(inputString, ","TEST_NAME_RESULTCOUNT":10");
        strcat(inputString, ","TEST_DEFAULT_MQTTCLIENT_FILTER"}");

        output.used = 0;
        rc = test_getEngineStats(TEST_ACTION_MQTTCLIENT,
                                 inputString,
                                 &output);

        if (rc != ISMRC_NotFound) printf("statType; %s\n", statTypes[i]);

        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }
}

CU_TestInfo  ISM_EngineStatsCLI_CUnit_test_capability_MQTTClient[] =
{
    { "NoMQTTClients", test_capability_NoMQTTClients },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initEngineStatsCLI(void)
{
    return test_engineInit(true, false,
                           true, // Disable Auto creation of named queues
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termEngineStatsCLI(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_EngineStatsCLI_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Subscription", initEngineStatsCLI, termEngineStatsCLI, ISM_EngineStatsCLI_CUnit_test_capability_Subscription),
    IMA_TEST_SUITE("Queue", initEngineStatsCLI, termEngineStatsCLI, ISM_EngineStatsCLI_CUnit_test_capability_Queue),
    IMA_TEST_SUITE("Topic", initEngineStatsCLI, termEngineStatsCLI, ISM_EngineStatsCLI_CUnit_test_capability_Topic),
    IMA_TEST_SUITE("MQTTClient", initEngineStatsCLI, termEngineStatsCLI, ISM_EngineStatsCLI_CUnit_test_capability_MQTTClient),
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

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_EngineStatsCLI_CUnit_allsuites);

    if (retval == 0)
    {
        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();
        printf("Random Seed =     %"PRId64"\n", seedVal);
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
