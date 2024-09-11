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
 * File: proxy_test.c
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

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#define CUNIT
#include "pxmqtt.c"
#include "pxtcp.c"
#include "auth.c"
#define NO_KAFKA_POBJ
#include "pxmhub.h"
#undef TRACE_COMP
#define allowSNI xallowSNI
#define enum_methods xenum_methods
#define getJsonValue xgetJsonValue
#include "tenant.c"
#undef allowSNI
#undef enum_methods
#undef getJsonValue
#include "wstcp.c"
#undef TRACE_COMP
#define mqttStats xmqttStats
#define tcpStats xtcpStats
#define mqttMsgSizeStats xmqttMsgSizeStats
#include "pxStatsdClient.c"


ism_topicrule_t * ism_proxy_getTopicRule(const char * name);

#define DISPLAY_VERBOSE       0
#define DISPLAY_QUIET         1

#define BASIC_TEST_MODE       0
#define FULL_TEST_MODE        1
#define BYNAME_TEST_MODE      2

/*
 * Global Variables
 */
int debug_mode = 0;
int display_mode = DISPLAY_VERBOSE; /* initial display mode */
int default_display_mode = DISPLAY_VERBOSE; /* default display mode */
int test_mode = BASIC_TEST_MODE;
int default_test_mode = BASIC_TEST_MODE;
int g_rc = 0;
int RC = 0;
int g_verbose = 0;

void iot2TestD(void);
void iot2TestGW(void);
void iot2TestA(void);
void quickstart2TestD(void);
void quickstart2TestA(void);
void clientClassTest(void);
void nullRuleTest(void);
void testConvertSharedSubTopic(void);
void testMakeSharedSubClientID(void);
void testParseTopicAuth(void);
void testFrameMqtt(void);
void testaddMqttFrame(void);
void testPoolHeadNull(void);
void testTenantAlias(void);
void testContentTypeMap(void);
void testNameMatch(void);
void testGenerateNameFromCert(void);
void testFairUse(void);
void testEventKey(void);
void testGetPayload(void);
void mqttv5_test(void);
void updateStatsTest(void);
void pxrouting_test(void);
void setJSON_test(void);
void stopCrlTest(void);
void test_kafkaConnection_parse(void);
void test_mhub_mapper(void);
void test_mhub_mapper_perf(void);
void tenantTest(void) ;
void iotrest_nonSecure(void);
ism_tenant_t* createTestTenant(void);
ism_tenant_t * createSecureTenant(void);
void TestUnlinkTenant(ism_tenant_t * tenant);
int doConfig(const char * json, int len);
void testRevalidateSaveData (void);
extern int ism_transport_initTransportBufferPool(void);


/*
 * Array that carries all the transport tests for APIs to CUnit framework.
 */
CU_TestInfo convert_tests[] = {
    { "--- Testing iot2 device               ---", iot2TestD },
    { "--- Testing iot2 GW                   ---", iot2TestGW },
    { "--- Testing iot2 app                  ---", iot2TestA },
    { "--- Testing quickstart2 device        ---", quickstart2TestD },
    { "--- Testing quickstart2 app           ---", quickstart2TestA },
    { "--- Testing null rule                 ---", nullRuleTest },
    { "--- Testing client class              ---", clientClassTest },
    { "--- Testing convert Shared Sub        ---", testConvertSharedSubTopic },
    { "--- Testing make SharedSub            ---", testMakeSharedSubClientID },
    { "--- Testing parseTopicAuth            ---", testParseTopicAuth },
    { "--- Testing addMqttFrame              ---", testaddMqttFrame },
	{ "--- Testing testPoolHead              ---", testPoolHeadNull },
    { "--- Testing frameMqtt                 ---", testFrameMqtt },
    { "--- Testing tenantAlias               ---", testTenantAlias },
    { "--- Testing contentTypeMap            ---", testContentTypeMap },
    { "--- Testing updateStatsMap            ---", updateStatsTest },
    { "--- Testing nameMatch                 ---", testNameMatch },
    { "--- Testing generatedName             ---", testGenerateNameFromCert },
    { "--- Testing parseFairUse              ---", testFairUse },
    { "--- Testing eventKey                  ---", testEventKey },
    { "--- Testing getPublishPayload         ---", testGetPayload },
    { "--- Testing MQTTv5                    ---", mqttv5_test },
	{ "--- Testing Proxy Routing             ---", pxrouting_test },
	{ "--- Testing Tenant                    ---", tenantTest },
	{ "--- Testing setJSON                   ---", setJSON_test },
	{ "--- Testing stopCRL                   ---", stopCrlTest },
	{ "--- Testing revalidateSaveData        ---", testRevalidateSaveData },
	{ "--- Testing mhub_kafkaConnection      ---", test_kafkaConnection_parse },
	{ "--- Testing mhub_mapper               ---", test_mhub_mapper },
	{ "--- Testing mhub_mapper_perf          ---", test_mhub_mapper_perf },
	CU_TEST_INFO_NULL
};

void iotrest_http_splitPath(void);
void iotrest_http_compare_ids(void);
void iotrest_receive_device(void);
void iotrest_receive_gateway(void);
void iotrest_device_post_event(void);
CU_TestInfo iotrest_tests[] = {
    { "--- Testing iotrest http split path   ---", iotrest_http_splitPath },
	{ "--- Testing iotrest http compare IDs  ---", iotrest_http_compare_ids },
	{ "--- Testing iotrest receive device    ---", iotrest_receive_device },
	{ "--- Testing iotrest receive gateway   ---", iotrest_receive_gateway },
	{ "--- Testing iotrest device post event ---", iotrest_device_post_event },
	{"--- Testing iotrest non secure connection ---",   iotrest_nonSecure },
    CU_TEST_INFO_NULL
};


/*
 * Array that carries the basic test suite and other functions to the CUnit framework
 */
CU_SuiteInfo
        ISM_proxy_CUnit_test_basicsuites[] = {
           IMA_TEST_SUITE("--- Convert test ---", NULL, NULL,  (CU_TestInfo *)convert_tests),
		   IMA_TEST_SUITE("--- HTTP tests   ---", NULL, NULL,  (CU_TestInfo *)iotrest_tests),
           CU_SUITE_INFO_NULL, };

/*
 * Array that carries the complete test suite and other functions to the CUnit framework
 */
CU_SuiteInfo
        ISM_proxy_CUnit_test_allsuites[] = {
            IMA_TEST_SUITE("--- Convert test ---", NULL, NULL,  (CU_TestInfo *)convert_tests),
			IMA_TEST_SUITE("--- HTTP tests   ---", NULL, NULL,  (CU_TestInfo *)iotrest_tests),
            CU_SUITE_INFO_NULL, };


/*
 * This is the main CUnit routine that starts the CUnit framework.
 * CU_basic_run_tests() Actually runs all the test routines.
 */
void Startup_CUnit(int argc, char ** argv) {
    CU_SuiteInfo * runsuite;
    CU_pTestRegistry testregistry;
    CU_pSuite testsuite;
    CU_pTest testcase;
    int testsrun = 0;

    if (argc > 1) {
        if (!strcmpi(argv[1],"F") || !strcmpi(argv[1],"FULL"))
           test_mode = FULL_TEST_MODE;
        else if (!strcmpi(argv[1],"B") || !strcmpi(argv[1],"BASIC"))
           test_mode = BASIC_TEST_MODE;
        else
           test_mode = BYNAME_TEST_MODE;
    }
    if (argc > 2) {
        if (*argv[2] == 'v')
            g_verbose = 1;
        if (*argv[2] == 'V') {
            g_verbose = 2;
            debug_mode = 1;
        }
    }

    printf("Test mode is %s\n",test_mode == 0 ? "BASIC" : test_mode == 1 ? "FULL" : "BYNAME");

    if (test_mode == BASIC_TEST_MODE)
        runsuite = ISM_proxy_CUnit_test_basicsuites;
    else
        // Load all tests for both FULL and BYNAME.
        // This way BYNAME can search all suite and test names
        // to find the tests to run.
        runsuite = ISM_proxy_CUnit_test_allsuites;

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
}

/*
 * This routine closes CUnit environment, and needs to be called only before the exiting of the program.
 */
void Endup_CUnit(void) {
    CU_cleanup_registry();
}

int matchClient(ism_clientclass_t * clientclass, ism_transport_t * transport) {
    ism_pxrule_t * rule = clientclass->classlist;
    int matched = 0;
    while (rule) {
    	if (matchClientClass(transport, rule)) {
		   transport->client_class = rule->class;
		   if (rule->after == 1) {
			   transport->alt_monitor = 1;
			   transport->use_userid = 1;
		   }
		   matched = 1;
		   break;
		}
		rule = rule->next;
	}
	if (!matched && clientclass->deflt) {
		if (matchClientClass(transport, rule)) {
		   transport->org = "";
		   matched = 1;
		}
    }
    return matched;
}

/*
 * client class test
 */
void clientClassTest(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_clientclass_t * clientclass = ism_proxy_getClientClass("iot2");
    int  match;

    transport->name = "d:myorg:mytype:mydev";
    transport->clientID = transport->name;
    match = matchClient(clientclass, transport);
    CU_ASSERT(match == 1);
    CU_ASSERT(transport->org && !strcmp(transport->org, "myorg"));
    CU_ASSERT(transport->typeID && !strcmp(transport->typeID, "mytype"));
    CU_ASSERT(transport->deviceID && !strcmp(transport->deviceID, "mydev"));
    CU_ASSERT(transport->client_class == 'd');
    CU_ASSERT(transport->alt_monitor == 0);
    CU_ASSERT(transport->use_userid == 0);


    memset(transport, 0, sizeof(ism_transport_t));
    transport->name = "a:myorg:myapp";
    transport->clientID = transport->name;
    match = matchClient(clientclass, transport);
    CU_ASSERT(match == 1);
    CU_ASSERT(transport->org && !strcmp(transport->org, "myorg"));
    CU_ASSERT(transport->typeID == NULL);
    CU_ASSERT(transport->deviceID && !strcmp(transport->deviceID, "myapp"));
    CU_ASSERT(transport->client_class == 'a');
    CU_ASSERT(transport->alt_monitor == 1);
    CU_ASSERT(transport->use_userid == 1);

    /*Test GW */
    transport->name = "g:8xtlax:TestBulkTypeMgd:a";
    transport->clientID = transport->name;
    match = matchClient(clientclass, transport);
    CU_ASSERT(match == 1);
    CU_ASSERT(transport->org && !strcmp(transport->org, "8xtlax"));
    CU_ASSERT(transport->typeID && !strcmp(transport->typeID, "TestBulkTypeMgd"));
    CU_ASSERT(transport->deviceID && !strcmp(transport->deviceID, "a"));
    CU_ASSERT(transport->client_class == 'g');

    /*Test with out the appid*/
    memset(transport, 0, sizeof(ism_transport_t));
	transport->name = "a:myorg";
	transport->clientID = transport->name;
	match = matchClient(clientclass, transport);
	CU_ASSERT(match == 0);

	 /*Test without the appid and with VARIABLE END (:). */
	memset(transport, 0, sizeof(ism_transport_t));
	transport->name = "a:myorg:";
	transport->clientID = transport->name;
	match = matchClient(clientclass, transport);
	CU_ASSERT(match == 0);

	 /*Test wit out the org*/
	memset(transport, 0, sizeof(ism_transport_t));
	transport->name = "a::myapp";
	transport->clientID = transport->name;
	match = matchClient(clientclass, transport);
	CU_ASSERT(match == 0);

	 /*Test wit out the org*/
	memset(transport, 0, sizeof(ism_transport_t));
	transport->name = "a:";
	transport->clientID = transport->name;
	match = matchClient(clientclass, transport);
	CU_ASSERT(match == 0);

	 /*Test wit out the org*/
	memset(transport, 0, sizeof(ism_transport_t));
	transport->name = "a::";
	transport->clientID = transport->name;
	match = matchClient(clientclass, transport);
	CU_ASSERT(match == 0);

	 /*Test wit out the org*/
	memset(transport, 0, sizeof(ism_transport_t));
	transport->name = "a:quickstart:";
	transport->clientID = transport->name;
	match = matchClient(clientclass, transport);
	CU_ASSERT(match == 0);

	 /*Test wit out the org*/
	memset(transport, 0, sizeof(ism_transport_t));
	transport->name = ":";
	transport->clientID = transport->name;
	match = matchClient(clientclass, transport);
	CU_ASSERT(match == 0);

	 /*Test wit out the org*/
	memset(transport, 0, sizeof(ism_transport_t));
	transport->name = "a";
	transport->clientID = transport->name;
	match = matchClient(clientclass, transport);
	CU_ASSERT(match == 0);

    memset(transport, 0, sizeof(ism_transport_t));
    transport->name = "d:quickstart:mytype";
    transport->clientID = transport->name;
    match = matchClient(clientclass, transport);
    CU_ASSERT(match == 0);

    memset(transport, 0, sizeof(ism_transport_t));
	transport->name = "A:myorgA:myappA";
	transport->clientID = transport->name;
	match = matchClient(clientclass, transport);
	CU_ASSERT(match == 1);

    memset(transport, 0, sizeof(ism_transport_t));
    transport->name = "A:myorgA:myappA:123";
    transport->clientID = transport->name;
    match = matchClient(clientclass, transport);
    CU_ASSERT(transport->typeID && !strcmp(transport->typeID, "123"));
    CU_ASSERT(match == 1);

}

void setJSON_test(void) {
    int rc = 0;
    char json[1000];
    const char * jsonx =
    "{\"ActivityMonitoring\":{\"Address\":[\"mongos-0.outlast.mgmt.mongo.test.internetofthings.ibmcloud.com:27017\","
    "\"mongos-1.outlast.mgmt.mongo.test.internetofthings.ibmcloud.com:27017\",\"mongos-2.outlast.mgmt.mongo.test.internetofthings.ibmcloud.com:27017\"],"
    "\"Type\":\"MongoDB\",\"UseTLS\":true,\"TrustStore\":\"/opt/ibm/msproxy/truststore/client/ca_public_cert.pem\","
    "\"User\":\"fra02_fra02_2_cs_activity\",\"Password\":\"fra02_fra02_2_cs_activity\",\"Name\":\"fra02-2-cs-activity\","
    "\"MemoryLimitPct\":2}}";
    strcpy(json, jsonx);
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[500];
    parseobj.ent_alloc = 500;
    parseobj.ent = ents;
    parseobj.source = (char *)json;
    parseobj.src_len = strlen(json);
    parseobj.options = JSON_OPTION_COMMENT;   /* Allow C style comments */
    rc = ism_json_parse(&parseobj);
    CU_ASSERT(rc == 0);
    if (rc == 0) {
        rc = ism_proxy_complexConfig(&parseobj, 2, 0, 0);
        CU_ASSERT(rc == 0);
    }
}

/*
 * call convertTopic
 */
int cvtTopic(ism_transport_t * transport, concat_alloc_t * buf, const char * topic, int subscribe, int good) {
    int  rc;
    buf->used = 0;
    buf->pos = 0;

    int len = strlen(topic);
    char * tp = alloca(len+1);
    strcpy(tp, topic);

    rc = convertTopic(transport, buf, tp, len, subscribe);
    if ((good && rc != 0) || (!good && rc == 0)) {
        if (debug_mode)
            printf("rc=%d  topic=%s\n", rc, topic);
    }
    if (rc == 0) {
        int olen;
        int blen = (((uint32_t)((uint8_t)buf->buf[0])) << 8) + ((uint8_t)buf->buf[1]);
        char xbuf [2048];
        concat_alloc_t obuf = {xbuf, sizeof xbuf};
        int rc2 = convertTopicOut(transport, &obuf, buf->buf+2, blen);
        olen = (((uint32_t)((uint8_t)obuf.buf[0])) << 8) + ((uint8_t)obuf.buf[1]);
        CU_ASSERT(rc2 == 0);
        CU_ASSERT(olen == len);
        CU_ASSERT(!memcmp(topic, obuf.buf+2, olen));
    }
    return rc;
}
/*
 * IoT2 device
 */
void nullRuleTest(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_tenant_t * tenant = calloc(1, sizeof(ism_tenant_t));
    char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};

    transport->name = "nullRuleTest";
    transport->tenant = tenant;
    transport->typeID = "type";
    transport->deviceID = "device";
    transport->client_class = 'd';
    transport->client_addr = "*";
    transport->endpoint_name = "*";
    tenant->name = "ios2";
    transport->org = "ios2";
    tenant->topicrule  = NULL;
    transport->trclevel = ism_defaultTrace;

    CU_ASSERT(cvtTopic(transport, &buf, "xyz/abc", 1, 1) == 0);    /* Valid publish */
}

/*
 * IoT2 device
 */
void iot2TestD(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_tenant_t * tenant = calloc(1, sizeof(ism_tenant_t));
    char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};

    transport->name = "iot2Test";
    transport->tenant = tenant;
    transport->typeID = "type";
    transport->deviceID = "device";
    transport->client_class = 'd';
    transport->client_addr = "*";
    transport->endpoint_name = "*";
    tenant->name = "ios2";
    transport->org = "ios2";
    tenant->topicrule  = ism_proxy_getTopicRule("iot2");
    transport->trclevel = ism_defaultTrace;

    /*
     * Device subscribe
     */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd/abc/fmt/json", 1, 1) == 0);    /* Valid publish */
    CU_ASSERT(cvtTopic(transport, &buf, "iotdm-1/#", 1, 1) == 0);
    CU_ASSERT(cvtTopic(transport, &buf, "iotdm-1/mgmt/initiate/+/+", 1, 1) == 0);
    CU_ASSERT(cvtTopic(transport, &buf, "iotdm-1/mgmt/+", 1, 1) == 0);
    CU_ASSERT(cvtTopic(transport, &buf, "iotdm-1/device/update/+/+", 1, 1) == 0);

    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd/abc/fmt/json/more", 1, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd//fmt/json", 1, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd/abc/fmt/", 1, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd/test/fmt/#", 1, 0) == ISMRC_BadTopic);

    /*
     * Device publish
     */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt/abc/fmt/json", 0, 1) == 0);    /* Valid publish */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt/abc/fmt/json/more", 0, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt/abc/fmt", 0, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt/abc/fmt/", 0, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt//fmt/json", 0, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd/abc/fmt/json", 0, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/frd//fmt/json", 0, 0) == ISMRC_BadTopic);
}

/*
 * IoT2 GW
 */
void iot2TestGW(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_tenant_t * tenant = calloc(1, sizeof(ism_tenant_t));
    char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};

    transport->name = "iot2Test";
    transport->tenant = tenant;
    transport->typeID = "type";
    transport->deviceID = "GW";
    transport->client_class = 'g';
    transport->client_addr = "*";
    transport->endpoint_name = "*";
    tenant->name = "ios2";
    transport->org = "ios2";
    tenant->topicrule  = ism_proxy_getTopicRule("iot2");
    transport->trclevel = ism_defaultTrace;

    /*
     * Device subscribe
     */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/t/id/d/cmd/abc/fmt/json", 1, 1) == 0);    /* Valid publish */
    //CU_ASSERT(cvtTopic(transport, &buf, "iotdm-1/#", 1, 1) == 0);
    CU_ASSERT(cvtTopic(transport, &buf, "iotdm-1/type/t/id/d/#", 1, 1) == 0);
    CU_ASSERT(cvtTopic(transport, &buf, "iotdm-1/mgmt/initiate/+/+", 1, 1) == 0);
    CU_ASSERT(cvtTopic(transport, &buf, "iotdm-1/mgmt/+", 1, 1) == 0);
    CU_ASSERT(cvtTopic(transport, &buf, "iotdm-1/device/update/+/+", 1, 1) == 0);

    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd/abc/fmt/json/more", 1, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd//fmt/json", 1, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd/abc/fmt/", 1, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd/test/fmt/#", 1, 0) == ISMRC_BadTopic);

    /*
     * Device publish
     */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/t/id/d/evt/abc/fmt/json", 0, 1) == 0);    /* Valid publish */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt/abc/fmt/json/more", 0, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt/abc/fmt", 0, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt/abc/fmt/", 0, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt//fmt/json", 0, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd/abc/fmt/json", 0, 0) == ISMRC_BadTopic);
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/frd//fmt/json", 0, 0) == ISMRC_BadTopic);

}

#define CHK_NOT_TOPIC 0
#define CHK_TOPIC     1
#define CHK_PUBTOPIC  2
#define CHK_SUBTOPIC  3

static int checkString(const char * s, int len, int topicchk) {
    const char * pos;
    int rc = ISMRC_OK;
    if (rc == 0) {
        /*
         * In V3.1.1 we do not allow Unicode non-characters or control characters
         * In V3.1 we only prohibit a 0 character
         */
        int check = UR_NoControl | UR_NoNonchar;
        if (topicchk == CHK_PUBTOPIC)
            check |= UR_NoWildcard;
        int count = ism_common_validUTF8Restrict(s, len, check);
        if (count < 0) {
            if (check & UR_NoWildcard) {
                check &= ~UR_NoWildcard;
                count = ism_common_validUTF8Restrict(s, len, check & ~UR_NoWildcard);
            }
            if (count < 0)
                rc = ISMRC_UnicodeNotValid;
            else
                rc = ISMRC_BadTopic;
        } else {
            if (topicchk) {
                if (count < 1) {
                    rc = ISMRC_BadTopic;
                } else if (topicchk == CHK_PUBTOPIC) {
                    if (count >= 20 && s[0]=='$' && !memcmp(s, "$SharedSubscription/", 20)) {
                        rc = ISMRC_BadTopic;
                    }
                } else if (topicchk == CHK_SUBTOPIC) {
                    int i;
                    if (count >= 20 && s[0]=='$' && !memcmp(s, "$SharedSubscription/", 20)) {
                        pos = memchr(s+20, '/', len-20);
                        if (!pos || pos==(s+20)) {
                            rc = ISMRC_BadTopic;
                        } else {
                            pos++;
                            len -= (pos-s);
                            s = pos;
                            count = len;        /* Just need it to be zero or not */
                            if (len == 0 || *pos == '$') {
                                rc = ISMRC_BadTopic;
                            }
                        }

                    } else if (s[0]=='$') {
                        if (len<5 || s[1]!='S' || s[2]!='Y' || s[3]!='S' || s[4]!='/') {
                            rc = ISMRC_BadSysTopic;
                        }
                    }
                    if (rc == 0) {
                        for (i = 0; i < len; i++) {
                            if (s[i] == '#') {
                                if ((i > 0 && s[i - 1] != '/')
                                        || (i + 1 != len)) {
                                    rc = ISMRC_BadTopic;
                                    break;
                                }
                            } else if (s[i] == '+') {
                                if ((i > 0 && s[i - 1] != '/')
                                        || (i + 1 != len && s[i + 1] != '/')) {
                                    rc = ISMRC_BadTopic;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return rc;
}


/*
 * IoT2 Application: Test Convert Shared Subscription Topic
 */
void testConvertSharedSubTopic(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_tenant_t * tenant = calloc(1, sizeof(ism_tenant_t));
    char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    int rc=0;

    transport->name = "iot2Test";
    transport->tenant = tenant;
    transport->typeID = "";
    transport->deviceID = "app1";
    transport->client_class = 'A';
    transport->client_addr = "*";
    transport->endpoint_name = "*";
    tenant->name = "ios2";
    transport->org = "ios2";
    transport->clientID="A:ios2:app1";
    tenant->topicrule  = ism_proxy_getTopicRule("iot2");
    transport->trclevel = ism_defaultTrace;

    char * result="$SharedSubscription/ios2:app1:iot-2:type:typ:id:device:evt:abc:fmt:json/iot-2/ios2/type/typ/id/device/evt/abc/fmt/json";
    char * otopic = "iot-2/type/typ/id/device/evt/abc/fmt/json";
    //CU_ASSERT(cvtTopic(transport, &buf, otopic, 1, 1) == 0);              /* good */
    rc = convertTopic(transport, &buf, otopic, strlen(otopic), 1);
    int blen = (((uint32_t)((uint8_t)buf.buf[0])) << 8) + ((uint8_t)buf.buf[1]);

    CU_ASSERT(rc==0);
    CU_ASSERT(blen==118);
    CU_ASSERT(strcmp(buf.buf+2,result)==0);
    //printf("Buf len: %d. Buf: %s\n", blen,buf.buf+2);
    transport->clientID =  "A:1kxm3b:AckSub02:_169.50.80.242_Hxe2Dgrn";
    transport->deviceID = "AckSub02";
    result = "$SharedSubscription/ios2:app1:iot-2:type:WP:id:&:evt:cc_SetKvpResult:fmt:binary/iot-2/ios2/type/WP/id/+/evt/cc_SetKvpResult/fmt/binary";
    otopic = "iot-2/type/WP/id/+/evt/cc_SetKvpResult/fmt/binary";
    buf.used = 0;
    buf.buf = xbuf;
    buf.len = sizeof(xbuf);
    rc = convertTopic(transport, &buf, otopic, strlen(otopic), 1);
    CU_ASSERT(rc == 0);
    blen = (((uint32_t)((uint8_t)buf.buf[0])) << 8) + ((uint8_t)buf.buf[1]);
    rc = checkString(buf.buf+2, blen, CHK_SUBTOPIC);
    CU_ASSERT(rc == 0);
    CU_ASSERT(blen == strlen(buf.buf+2));
    // fprintf(stderr, "\nconvertedTopic=%s  blen=%d slen=%d rc=%d \n", buf.buf+2, blen, (int)strlen(buf.buf+2), rc);
}

/*
 * IoT2 Application: Test Convert Shared Subscription Topic
 */
void testMakeSharedSubClientID(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_tenant_t * tenant = calloc(1, sizeof(ism_tenant_t));

    transport->name = "iot2Test";
    transport->tenant = tenant;
    transport->typeID = "";
    transport->deviceID = "app1";
    transport->client_class = 'A';
    transport->client_addr = "10.2.3.4";
    transport->endpoint_name = "*";
    tenant->name = "ios2";
    transport->org = "ios2";
    transport->clientID="A:ios2:app1";
    tenant->topicrule  = ism_proxy_getTopicRule("iot2");
    transport->trclevel = ism_defaultTrace;

    char cxbuf [2048];
    char * appName = "A:ios2:app1";
    int appLen = strlen(appName);
    int size = makeSharedSubClientID(transport, cxbuf, appName, appLen);
    if (debug_mode)
        printf("newclientid='%s'\n", cxbuf);
    CU_ASSERT(size> appLen);
    CU_ASSERT(memcmp(cxbuf, appName, appLen)==0);

}

int emptyReceive(ism_transport_t * transport, char * buf, int len, int protval) {
    return 0;
}


int emptySender(ism_transport_t * transport, char * buf, int len, int frame, int flags) {
    return 0;
}

int emptyClosed(ism_transport_t * transport) {
    return 0;
}

int emptyClose(ism_transport_t * transport, int rc, int clean, const char * reason) {
    if (debug_mode)
        printf("\nclose: rc=%u reason=%s   ", rc, reason);
    return 0;
}


void initTransport(ism_transport_t * transport) {
    transport->send     = emptySender;
    transport->receive  = emptyReceive;
    transport->close    = emptyClose;
    transport->state    = ISM_TRANST_Open;

    transport->server_addr = "127.0.0.1";
    transport->serverport  = 16102;
    transport->client_addr = "fromip";
    transport->clientport  = 5;
    transport->protocol = "unknown";

}

/*
 * IoT2 Application
 */
void iot2TestA(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_tenant_t * tenant = calloc(1, sizeof(ism_tenant_t));
    char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};

    transport->name = "iot2Test";
    transport->tenant = tenant;
    transport->typeID = "";
    transport->deviceID = "app";
    transport->client_class = 'a';
    transport->client_addr = "*";
    transport->endpoint_name = "*";
    tenant->name = "ios2";
    transport->org = "ios2";
    tenant->topicrule  = ism_proxy_getTopicRule("iot2");
    transport->trclevel = ism_defaultTrace;

    /*
     * Application subscribe
     */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/fmt/json", 1, 1) == 0);              /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/+/fmt/json", 1, 1) == 0);                /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/+/fmt/+", 1, 1) == 0);                   /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type//id/device/evt/+/fmt/+", 1, 0) == ISMRC_BadTopic);         /* empty type */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/cmd/+", 1, 0) == ISMRC_BadTopic);    /* cmd not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/fred/+", 1, 0) == ISMRC_BadTopic);   /* fred not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/+/fmt/#", 1, 0) == ISMRC_BadTopic);      /* # not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/fmt/#", 1, 0) == ISMRC_BadTopic);    /* # not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type//id/device/evt/abc/fmt/json", 1, 0) == ISMRC_BadTopic);    /* zero len type not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type//id/device/evt//fmt/json", 1, 0) == ISMRC_BadTopic);       /* zero len event not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/+/id/device/evt/abc/fmt/json", 1, 1) == 0);                /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id//evt/abc/fmt/json", 1, 0) == ISMRC_BadTopic);       /* zero len device not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/+/evt/abc/fmt/json", 1, 1) == 0);                  /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/+/evt/abc/fmt/json/more", 1, 0) == ISMRC_BadTopic); /* too long */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/+/evt/abc/fmt/", 1, 0) == ISMRC_BadTopic);          /* too short */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/+/evt/abc/fmt", 1, 0) == ISMRC_BadTopic);           /* too short */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/mon", 1, 1) == 0);                           /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/+/mon", 1, 1) == 0);                   /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/mon/more", 1, 0) == ISMRC_BadTopic);         /* too long */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/app/appid/mon", 1, 1) == 0);                                    /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/app/+/mon", 1, 1) == 0);                                        /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/app//mon", 1, 0) == ISMRC_BadTopic);                            /* zero len not allwoed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/app/appid/mon/more", 1, 0) == ISMRC_BadTopic);                  /* too long */

    /*
     * Application publish
     */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/fmt/json", 1, 1) == 0);               /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/fmt/fred", 1, 1) == 0);               /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type//id/device/evt/abc/fmt/json", 1, 0) == ISMRC_BadTopic);     /* empty type */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/fmt/", 1, 0) == ISMRC_BadTopic);      /* empty format */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt//fmt/json", 1, 0) == ISMRC_BadTopic);     /* empty event */
}

/**
 * Test whether MQTT frame is being added correctly for various message sizes
 */
void testaddMqttFrame(void) {
    const int SIZE1 = 50;
    const int SIZE2 = 9876;
    const int SIZE3 = 69001;
    const int SIZE4 = 6004231;
    const int SIZE5 = 268435456;
    int len;
    char command = 1;
    char buffer[512];
    ism_transport_t * transport;

    ism_endpoint_t * endpoint = calloc(1, sizeof(ism_endpoint_t) + sizeof(ism_endstat_t));
    endpoint->stats = (ism_endstat_t *)(endpoint+1);
    endpoint->port = 12345;
    endpoint->name = "test";
    transport = ism_transport_newTransport(endpoint, 64, 0);
    initTransport(transport);

    memset(buffer, 'Z', sizeof(buffer));

    len = SIZE1;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 2, len, command) == 2);
    CU_ASSERT(buffer[0] == command);
    CU_ASSERT((unsigned char)buffer[1] == SIZE1);

    len = SIZE2;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 3, len, command) == 3);
    CU_ASSERT(buffer[0] == command);
    CU_ASSERT((unsigned char)buffer[1] == ((SIZE2 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[2] == ((SIZE2 / 128 % 128)));

    len = SIZE3;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 4, len, command) == 4);
    CU_ASSERT(buffer[0] == command);
    CU_ASSERT((unsigned char)buffer[1] == ((SIZE3 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[2] == ((SIZE3 / 128 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[3] == ((SIZE3 / 128 / 128 % 128)));

    len = SIZE4;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 5, len, command) == 5);
    CU_ASSERT(buffer[0] == command);
    CU_ASSERT((unsigned char)buffer[1] == ((SIZE4 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[2] == ((SIZE4 / 128 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[3] == ((SIZE4 / 128 / 128 % 128) | 0x80));
    CU_ASSERT((unsigned char)buffer[4] == ((SIZE4 / 128 / 128 / 128 % 128)));

    len = SIZE5;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 5, len, command) == -1);
    ism_transport_freeTransport(transport);
}

/**
 * Test whether MQTT frame is being added correctly for various message sizes
 */
void testPoolHeadNull(void) {
    ism_transport_t * transport;

    ism_transport_initTransportBufferPool();

    ism_endpoint_t * endpoint = calloc(1, sizeof(ism_endpoint_t) + sizeof(ism_endstat_t));
    endpoint->stats = (ism_endstat_t *)(endpoint+1);
    endpoint->port = 12345;
    endpoint->name = "test";
    transport = ism_transport_newTransport(endpoint, 64, 1); //transport from pool
    ism_transport_newTransport(endpoint, 64, 1); //transport from pool
    ism_transport_newTransport(endpoint, 64, 1); //transport from pool
    ism_transport_newTransport(endpoint, 64, 1); //transport from pool
    ism_transport_newTransport(endpoint, 64, 1); //transport from pool
    ism_transport_freeTransport(transport);
    ism_transport_freeTransport(transport);
    ism_transport_newTransport(endpoint, 64, 1); //transport from pool
    ism_transport_newTransport(endpoint, 64, 1); //transport from pool
    ism_transport_newTransport(endpoint, 64, 1); //transport from pool
    ism_transport_newTransport(endpoint, 64, 1); //transport from pool
    ism_transport_newTransport(endpoint, 64, 1); //transport from pool
    ism_transport_freeTransport(transport);
    ism_transport_newTransport(endpoint, 64, 1); //transport from pool
	ism_transport_newTransport(endpoint, 64, 1); //transport from pool
	ism_transport_newTransport(endpoint, 64, 1); //transport from pool
	ism_transport_newTransport(endpoint, 64, 1); //transport from pool
	ism_transport_newTransport(endpoint, 64, 1); //transport from pool
	ism_transport_newTransport(endpoint, 64, 1); //transport from pool
	ism_transport_newTransport(endpoint, 64, 1); //transport from pool
	ism_transport_newTransport(endpoint, 64, 1); //transport from pool
}

/**
 * Test MQTT framer processing
 */
void testFrameMqtt(void) {
    ism_transport_t * transport;
    const int SIZE1 = 15;
    const int SIZE2 = 5813;
    const int SIZE3 = 89000;
    const int SIZE4 = 3050411;
    const int BUF_SIZE = 4194304;
    char *buffer = malloc(BUF_SIZE);
    int len, kind, used, save_msgcnt, save_len;

    ism_endpoint_t * endpoint = calloc(1, sizeof(ism_endpoint_t) + sizeof(ism_endstat_t));
    endpoint->stats = (ism_endstat_t *)(endpoint+1);
    endpoint->port = 12345;
    endpoint->name = "test";
    endpoint->maxMsgSize = 128*1024;

    transport = ism_transport_newTransport(endpoint, 64, 0);
    initTransport(transport);

    transport->maxMsgSize = endpoint->maxMsgSize;
    memset(buffer, 'Q', BUF_SIZE);

    // Add MQTT frame
    len = SIZE1;
    kind = 1;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 2, len, kind) == 2);

    // Try incomplete message
    save_msgcnt = transport->read_msg;
    save_len = transport->read_bytes;
    CU_ASSERT(ism_transport_frameMqtt(transport, buffer, 0, SIZE1 - 2, &used) == SIZE1 + 2);
    CU_ASSERT(save_len == transport->read_bytes);

    // Try complete message
    used = 0;
    save_msgcnt = transport->read_msg;
    save_len = transport->read_bytes;
    CU_ASSERT(ism_transport_frameMqtt(transport, buffer, 0, BUF_SIZE, &used) == 0);
    CU_ASSERT(used == (SIZE1 + 2));

    // Add MQTT frame
    memset(buffer, 'Q', SIZE2);
    len = SIZE2;
    kind = 1;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 3, len, kind) == 3);

    // Try incomplete message
    save_msgcnt = transport->read_msg;
    CU_ASSERT(ism_transport_frameMqtt(transport, buffer, 0, SIZE2 - 2, &used) == SIZE2 + 3);

    // Try complete message
    used = 0;
    save_msgcnt = transport->read_msg;
    CU_ASSERT(ism_transport_frameMqtt(transport, buffer, 0, BUF_SIZE, &used) == 0);
    CU_ASSERT(used == (SIZE2 + 3));

    // Add MQTT frame
    memset(buffer, 'Q', SIZE3);
    len = SIZE3;
    kind = 1;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 4, len, kind) == 4);

    // Try incomplete message
    save_msgcnt = transport->read_msg;
    CU_ASSERT(ism_transport_frameMqtt(transport, buffer, 0, SIZE3 - 2, &used) == SIZE3 + 4);
    CU_ASSERT(save_msgcnt == transport->read_msg);

    // Try complete message
    used = 0;
    save_msgcnt = transport->read_msg;
    CU_ASSERT(ism_transport_frameMqtt(transport, buffer, 0, BUF_SIZE, &used) == 0);
    CU_ASSERT(used == (SIZE3 + 4));

    // Add MQTT frame
    memset(buffer, 'Q', SIZE4);
    len = SIZE4;
    kind = 1;
    CU_ASSERT(ism_transport_addMqttFrame(transport, buffer + 5, len, kind) == 5);

    // Try incomplete message
    CU_ASSERT(ism_transport_frameMqtt(transport, buffer, 0, SIZE4 - 2, &used) == -1);

    /* Now with a large max msg size */
    endpoint->maxMsgSize = 0;
    transport->maxMsgSize = 0;
    /* Try incomplete message */
    save_msgcnt = transport->read_msg;
    CU_ASSERT(ism_transport_frameMqtt(transport, buffer, 0, SIZE4 - 2, &used) == SIZE4 + 5);
    CU_ASSERT(save_msgcnt == transport->read_msg);

    /* Try a complete message */
    used = 0;
    save_msgcnt = transport->read_msg;
    CU_ASSERT(ism_transport_frameMqtt(transport, buffer, 0, BUF_SIZE, &used) == 0);
    CU_ASSERT(used == (SIZE4 + 5));

    /* Bad length */
    used = 0;
    buffer[4] = 0x80;
    CU_ASSERT(ism_transport_frameMqtt(transport, buffer, 0, 10, &used) == -1);

    free(buffer);
    ism_transport_freeTransport(transport);
}


/*
 * Quickstart 2 device
 */
void quickstart2TestD(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_tenant_t * tenant = calloc(1, sizeof(ism_tenant_t));
    char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    transport->trclevel = ism_defaultTrace;

    transport->name = "iot2Test";
    transport->tenant = tenant;
    transport->typeID = "type";
    transport->deviceID = "device";
    transport->client_class = 'd';
    transport->client_addr = "*";
    transport->endpoint_name = "*";
    tenant->name = "quickstart";
    transport->org = "quickstart";
    tenant->topicrule  = ism_proxy_getTopicRule("quickstart2");

    /*
     * Device subscribe
     */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt/abc/fmt/json", 1, 0) == ISMRC_BadTopic);    /* Devices cannot subscribe */

    /*
     * Device publish
     */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt/abc/fmt/json", 0, 1) == 0);    /* Valid publish */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/evt/abc/fmt/json/more", 0, 0) == ISMRC_BadTopic);    /* Valid publish */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/cmd/abc/fmt/json", 0, 0) == ISMRC_BadTopic);    /* Valid publish */
}

/*
 * Qickstart 2 application
 */
void quickstart2TestA(void) {
    ism_transport_t * transport = calloc(1, sizeof(ism_transport_t));
    ism_tenant_t * tenant = calloc(1, sizeof(ism_tenant_t));
    char xbuf [2048];
    transport->trclevel = ism_defaultTrace;
    concat_alloc_t buf = {xbuf, sizeof xbuf};

    transport->name = "iot2Test";
    transport->tenant = tenant;
    transport->org = "quickstart";
    transport->typeID = "";
    transport->deviceID = "app";
    transport->client_class = 'a';
    transport->client_addr = "*";
    transport->endpoint_name = "*";
    tenant->name = "quickstart";
    tenant->topicrule = ism_proxy_getTopicRule("quickstart2");


    /*
     * Application subscribe
     */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/fmt/json", 1, 1) == 0);              /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/+/fmt/json", 1, 1) == 0);                /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/+/fmt/+", 1, 1) == 0);                   /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type//id/device/evt/+/fmt/+", 1, 0) == ISMRC_BadTopic);         /* empty type */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/cmd/+", 1, 0) == ISMRC_BadTopic);    /* cmd not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/fred/+", 1, 0) == ISMRC_BadTopic);   /* fred not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/+/fmt/#", 1, 0) == ISMRC_BadTopic);      /* # not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/fmt/#", 1, 0) == ISMRC_BadTopic);    /* # not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type//id/device/evt/abc/fmt/json", 1, 0) == ISMRC_BadTopic);    /* zero len type not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type//id/device/evt//fmt/json", 1, 0) == ISMRC_BadTopic);       /* zero len event not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/+/id/device/evt/abc/fmt/json", 1, 1) == 0);                /* wild type allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id//evt/abc/fmt/json", 1, 0) == ISMRC_BadTopic);       /* zero len device not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/+/evt/abc/fmt/json", 1, 0) == ISMRC_BadTopic);      /* wild len device not allowed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/+/evt/abc/fmt/json/more", 1, 0) == ISMRC_BadTopic); /* too long */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/+/evt/abc/fmt/", 1, 0) == ISMRC_BadTopic);          /* too short */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/+/evt/abc/fmt", 1, 0) == ISMRC_BadTopic);           /* too short */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/mon", 1, 1) == 0);                           /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/+/mon", 1, 0) == ISMRC_BadTopic);                   /* wild not allwoed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/mon/more", 1, 0) == ISMRC_BadTopic);         /* too long */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/app/appid/mon", 1, 1) == 0);                                    /* good */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/app/+/mon", 1, 0) == ISMRC_BadTopic);                           /* wild not allwoed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/app//mon", 1, 0) == ISMRC_BadTopic);                            /* zero len not allwoed */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/app/appid/mon/more", 1, 0) == ISMRC_BadTopic);                  /* too long */


    /*
     * Application publish
     */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/fmt/json", 0, 1) == 0);    /* Valid publish */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/evt/abc/fmt/json/more", 0, 0) == ISMRC_BadTopic);    /* Valid publish */
    CU_ASSERT(cvtTopic(transport, &buf, "iot-2/type/typ/id/device/cmd/abc/fmt/json", 0, 0) == ISMRC_BadTopic);    /* Valid publish */
}

/*
 * Test for parseTopicAuth
 */
void testParseTopicAuth(void) {
    char topic [128];
    const char * devtype;
    const char * devid;

    strcpy(topic, "iot-2/org/type/type/id/id/fred");
    CU_ASSERT(parseTopicAuth(topic, &devtype, &devid) == 0);
    CU_ASSERT(!strcmp(devtype, "type"));
    CU_ASSERT(!strcmp(devid, "id"));

    strcpy(topic, "iot-2/org/type/id/id/type/fred");
    CU_ASSERT(parseTopicAuth(topic, &devtype, &devid) == 0);
    CU_ASSERT(!strcmp(devtype, "id"));
    CU_ASSERT(!strcmp(devid, "type"));

    strcpy(topic, "iot-2/org/type/+/id/id/fred");
    CU_ASSERT(parseTopicAuth(topic, &devtype, &devid) == 2);

    strcpy(topic, "iot-2/org/type/type/id/+/fred");
    CU_ASSERT(parseTopicAuth(topic, &devtype, &devid) == 2);
}

const char * ism_proxy_contentTypeMap(const char * contentType);

void testContentTypeMap(void) {
    const char * res;

    res = ism_proxy_contentTypeMap("application/json");
    CU_ASSERT(res != NULL);
    if (res) {
        CU_ASSERT(strcmp(res, "json") == 0);
    }

    res = ism_proxy_contentTypeMap("application/xml ; charset=utf-8");
    CU_ASSERT(res != NULL);
    if (res) {
        CU_ASSERT(strcmp(res, "xml") == 0);
    }

    res = ism_proxy_contentTypeMap("application/octet-stream");
    CU_ASSERT(res != NULL);
    if (res) {
        CU_ASSERT(strcmp(res, "bin") == 0);
    }

    res = ism_proxy_contentTypeMap("text/plain; charset=utf-8");
    CU_ASSERT(res != NULL);
    if (res) {
        CU_ASSERT(strcmp(res, "text") == 0);
    }

    res = ism_proxy_contentTypeMap("text/xml; charset=utf-8");
    CU_ASSERT(res != NULL);
    if (res) {
        CU_ASSERT(strcmp(res, "text") == 0);
    }

    res = ism_proxy_contentTypeMap("image/jpeg");
    CU_ASSERT(res == NULL);
}

/*
 * Test name match.
 * There is CUNIT code in pxmqtt.c to fake the SubjectAltName during CUNIT
 */
void testNameMatch(void) {
    char tobj [1000] = {0};
    ism_transport_t transport = {0};
    transport.tobj = (void *)tobj;
    transport.tobj->ssl = (void *)tobj;    /* Just needs to not be null */

    transport.typeID = "mytype";
    transport.client_class = 'g';
    transport.deviceID = "device";
    transport.cert_name = "g:mytype:device";
    CU_ASSERT(ism_proxy_matchCertNames(&transport)==3);   /* Match on common name */

    transport.typeID = "typ";
    transport.client_class = 'd';
    transport.deviceID = "device";
    CU_ASSERT(ism_proxy_matchCertNames(&transport)==0);   /* CN does not match */

    transport.client_host = "d:typ:device\0a:\0g:type:\0";    /* Fake SubjectAltName */
    transport.clientport = 24;                                /* Fake SAN len */
    transport.serverport = 3;                                 /* Fake SAN count */
    transport.cert_name = "g:mytype:device";
    CU_ASSERT(ism_proxy_matchCertNames(&transport)==3);   /* Match on alt name */

    transport.client_class = 'a';
    transport.deviceID = "abcdefg";
    CU_ASSERT(ism_proxy_matchCertNames(&transport)==1);   /* Match on alt name a: */

    transport.client_class = 'g';
    transport.typeID = "type";
    CU_ASSERT(ism_proxy_matchCertNames(&transport)==1);   /* Match on alt name g:type */

    transport.client_class = 'g';
    transport.typeID = "mytype";
    transport.deviceID = "device";
    CU_ASSERT(matchCertName(&transport, "g:mytype:device")==3);

    transport.client_class = 'd';
    transport.typeID = "mytype";
    transport.deviceID = "device";
    CU_ASSERT(matchCertName(&transport, "g:mytype:device")==0);

    transport.client_class = 'd';
    transport.typeID = "mytype";
    transport.deviceID = "device";
    CU_ASSERT(matchCertName(&transport, "d:mytype:")==1);

    transport.client_class = 'x';
    transport.typeID = "mytype";
    transport.deviceID = "device";
    CU_ASSERT(matchCertName(&transport, "g:mytype:device")==0);

    transport.client_host = "d:typ:\0d:typ:device\0a:\0g:type:\0";    /* Fake SubjectAltName */
    transport.clientport = 30;                                /* Fake SAN len */
    transport.serverport = 4;                                 /* Fake SAN count */
    transport.client_class = 'd';
    transport.typeID = "typ";
    transport.deviceID = "device";
    CU_ASSERT(ism_proxy_matchCertNames(&transport)==3);
}


void testGenerateNameFromCert(void) {
    char client_class;
    const char * devtypeid;
    CU_ASSERT(parseCertName("a:abc:def", &client_class, &devtypeid)==1);
    CU_ASSERT(parseCertName("d:abc:",    &client_class, &devtypeid)==1);
    CU_ASSERT(parseCertName("g:abc:d:f", &client_class, &devtypeid)==1);
    CU_ASSERT(parseCertName("d:abc:def", &client_class, &devtypeid)==0);
    CU_ASSERT(client_class='d');
    CU_ASSERT(!strcmp(devtypeid, "abc:def"));
    CU_ASSERT(parseCertName("g:xyz:nop", &client_class, &devtypeid)==0);
    CU_ASSERT(client_class='g');
    CU_ASSERT(!strcmp(devtypeid, "xyz:nop"));


    char tobj [sizeof(ism_transport_t)+200] = {0};
    ism_transport_t * transport = alloca(sizeof(ism_transport_t)+200);
    memset(transport, 0, sizeof(ism_transport_t));
    transport->tobj = (void *)tobj;
    transport->tobj->ssl = (void *)tobj;    /* Just needs to not be null */
    transport->suballoc.size = 200;
    transport->suballoc.pos = 0;

    transport->client_host = "g:type:\0a:\0d:typ:device\0";    /* Fake SubjectAltName */
    transport->clientport = 24;                                /* Fake SAN len */
    transport->serverport = 3;                                 /* Fake SAN count */
    transport->cert_name = "d:mytype:mydev";
    transport->sniName = "kb";
    transport->name = 0;

    transport->suballoc.pos = 0;
    generateNameFromCert(transport);
    CU_ASSERT(transport->name && !strcmp(transport->name, "d:kb:mytype:mydev"));

    transport->name = NULL;
    transport->suballoc.pos = 0;
    transport->cert_name = "mycert";
    generateNameFromCert(transport);
    CU_ASSERT(transport->name && !strcmp(transport->name, "d:kb:typ:device"));


}

int ism_proxy_setTenantAlias(const char * str);
const char * ism_tenant_getTenantAlias(const char * org, const char * deviceID) ;

void testTenantAlias(void) {
    ism_common_setTraceLevel(2);
    CU_ASSERT(ism_proxy_setTenantAlias("abc=def") == 0);
    CU_ASSERT(ism_proxy_setTenantAlias("") == 0);
    CU_ASSERT(ism_proxy_setTenantAlias(NULL) == 0);
    CU_ASSERT(ism_proxy_setTenantAlias("def=ghi,  zab=baz") == 0);
    CU_ASSERT(ism_proxy_setTenantAlias("def(WTR)=ghi") == 0);
    CU_ASSERT(ism_proxy_setTenantAlias("def(WTR)=ghi, zab=baz") == 0);
    CU_ASSERT(ism_proxy_setTenantAlias("def(WTR)=ghi, zab(WTR2)=baz") == 0);
    CU_ASSERT(ism_proxy_setTenantAlias("def(WTR232)=ghi, zab(WTR22344)=baz") == 0);
    CU_ASSERT(ism_proxy_setTenantAlias("def=ghi,  zabx=baz") == ISMRC_ArgNotValid);
    CU_ASSERT(ism_proxy_setTenantAlias("def=ghi,  zabbaz") == ISMRC_ArgNotValid);
    CU_ASSERT(ism_proxy_setTenantAlias("def(WTR232)=ghi, zab(WTR22344)=bazedf") == ISMRC_ArgNotValid);
    CU_ASSERT(ism_proxy_setTenantAlias("=") == ISMRC_ArgNotValid);


    //Test getTenantAlias
    CU_ASSERT(ism_proxy_setTenantAlias("def(WTR)=ghi, zab=baz") == 0);

    CU_ASSERT(ism_tenant_getTenantAlias("def", "WTR123445") !=NULL);
    CU_ASSERT(strcmp(ism_tenant_getTenantAlias("def", "WTR123445"), "ghi" ) == 0);
    CU_ASSERT(ism_tenant_getTenantAlias("def", "W") == NULL);
    CU_ASSERT(ism_tenant_getTenantAlias("def", "WT") == NULL);
    CU_ASSERT(ism_tenant_getTenantAlias("notdef", "WTR") == NULL);
    CU_ASSERT(ism_tenant_getTenantAlias("notdef", NULL) == NULL);
    CU_ASSERT(ism_tenant_getTenantAlias("def", "WTR") != NULL);


    CU_ASSERT(ism_tenant_getTenantAlias("zab", "WTR123445") !=NULL);
	CU_ASSERT(strcmp(ism_tenant_getTenantAlias("zab", "WTR123445"), "baz" ) == 0);
	CU_ASSERT(ism_tenant_getTenantAlias("zab", "W") != NULL);
	CU_ASSERT(ism_tenant_getTenantAlias("zab", "WTR") != NULL);
	CU_ASSERT(ism_tenant_getTenantAlias("zab", NULL) != NULL);

	//Test Empty String for Alias
	CU_ASSERT(ism_proxy_setTenantAlias("") == 0);
	CU_ASSERT(ism_tenant_getTenantAlias("zab", "123") == NULL);


	//Test M2M Test Org
	CU_ASSERT(ism_proxy_setTenantAlias("4j1u32(WPR)=34bhp8") == 0);
	CU_ASSERT(ism_tenant_getTenantAlias("4j1u32", "123") == NULL);  //No alias will return
	const char * alias = ism_tenant_getTenantAlias("4j1u32", "WPR");
	CU_ASSERT(alias != NULL);  //Shouldn't be NULL
	CU_ASSERT(strcmp(alias, "34bhp8" ) == 0); //SHould be Equal to the alias
	CU_ASSERT(ism_tenant_getTenantAlias("34bhp8", "WPR") == NULL);

}



int ism_http_splitPath(char * path, char * * parts, int partmax);

void iotrest_http_splitPath(void) {
	char * parts[20];
	int partcount;
	char path[100];
	strcpy(path, "//api/v0002/device/types/type001/devices/dev001/events/status");

	partcount = ism_http_splitPath(path, parts, 20);
	CU_ASSERT(partcount == 9);
	CU_ASSERT(strcmp(parts[0], "api") == 0);
	CU_ASSERT(strcmp(parts[1], "v0002") == 0);
	CU_ASSERT(strcmp(parts[2], "device") == 0);
	CU_ASSERT(strcmp(parts[3], "types") == 0);
	CU_ASSERT(strcmp(parts[4], "type001") == 0);
	CU_ASSERT(strcmp(parts[5], "devices") == 0);
	CU_ASSERT(strcmp(parts[6], "dev001") == 0);
	CU_ASSERT(strcmp(parts[7], "events") == 0);
	CU_ASSERT(strcmp(parts[8], "status") == 0);

	strcpy(path, "/");
	partcount = ism_http_splitPath(path, parts, 1);
	CU_ASSERT(partcount == 0);

	strcpy(path, "part1//part2//");
	parts[1] = 0;
	partcount = ism_http_splitPath(path, parts, 1);
	CU_ASSERT(partcount == 2);
	CU_ASSERT(strcmp(parts[0], "part1") == 0);
	CU_ASSERT(parts[1] == 0);


	strcpy(path, "//");
	partcount = ism_http_splitPath(path, parts, 1);
	CU_ASSERT(partcount == 0);

	path[0] = '\0';
	partcount = ism_http_splitPath(path, parts, 1);
	CU_ASSERT(partcount == 0);
}

int ism_iotrest_ids_cmp(const char *s1, char delim1, int count, const char *s2, char delim2);

void iotrest_http_compare_ids(void) {
	char s1[100];
	char s2[100];
	int rc;
	strcpy(s1, "g:oooooo:t:d");
	strcpy(s2, "g-oooooo-t-d");
	rc = ism_iotrest_ids_cmp(s1, ':', 3, s2, '-');
	CU_ASSERT(rc == 0);

	rc = ism_iotrest_ids_cmp(s1, ':', 4, s2, '-');
	CU_ASSERT(rc != 0);

	rc = ism_iotrest_ids_cmp(s1, ':', 0, s2, '-');
	CU_ASSERT(rc != 0);

	strcpy(s2, "g-oooooo-tdd");
	rc = ism_iotrest_ids_cmp(s1, ':', 3, s2, '-');
	CU_ASSERT(rc != 0);

	strcpy(s2, "g-oooooo-t-dd");
	rc = ism_iotrest_ids_cmp(s1, ':', 3, s2, '-');
	CU_ASSERT(rc != 0);

	rc = ism_iotrest_ids_cmp("::::", ':', 3, s2, '-');
	CU_ASSERT(rc != 0);

}

extern int ism_proxy_parseFairUse(tenant_fairuse_t * fairuse, const char * fairUsePolicy, const char * tenant);

/*
 * Test parse of the fair use policy
 */
void testFairUse(void) {
    tenant_fairuse_t fairuse = {0};
    int rc;

    rc = ism_proxy_parseFairUse(&fairuse, "unit=1024", "mytenant");
    CU_ASSERT(rc == 0);
    CU_ASSERT(fairuse.mups_units == 1024);

    fairuse.mups_a = 99;
    rc = ism_proxy_parseFairUse(&fairuse, "", "thistenant");
    CU_ASSERT(rc == 0);
    CU_ASSERT(fairuse.mups_units == 4096);
    CU_ASSERT(fairuse.mups_a == 0.0);

    fairuse.mups_d = 99;
    rc = ism_proxy_parseFairUse(&fairuse, NULL, "nulltenant");
    CU_ASSERT(rc == 1);

    rc = ism_proxy_parseFairUse(&fairuse, "mups_d=10, mups_a=40.2,mups_A=100    mups_g=1000.1", "fulltenant");
    CU_ASSERT(fairuse.mups_units == 4096);
    CU_ASSERT(fairuse.mups_a > 40.199 && fairuse.mups_a < 40.201);
    CU_ASSERT(fairuse.mups_A == 100);
    CU_ASSERT(fairuse.mups_g > 1000.099 && fairuse.mups_g < 1000.101);
    CU_ASSERT(fairuse.mups_d == 10);

    rc = ism_proxy_parseFairUse(&fairuse, "mups_d=fred, unit=256", "myname");
    CU_ASSERT(rc == ISMRC_BadPropertyValue);

    rc = ism_proxy_parseFairUse(&fairuse, "fred=3", "badname");
    CU_ASSERT(rc == ISMRC_BadPropertyName);
}

void ism_kafka_makeEventKey(concat_alloc_t * buf, const char * org, const char * type, const char * id, const char * event,
        const char * fmt, const char * uuid_bin, concat_alloc_t *	 userProperties);

extern int putUserPropertyPair(concat_alloc_t * buf, const char * name, int namelen, const char * value, int len);

/*
 * Load a big endian 2 byte integer
 */
#define BIGINT16(p) (((int)(uint8_t)(p)[0]<<8) | (uint8_t)(p)[1])
/*
 * Put a string for the Kafka key
 */

/*
 * Test generating the event key
 */
void testEventKey(void) {
    char xbuf [512];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    char uuid[16];
    char uuidbuf [40];
    int i;

    char userPropertiesXBuf[512];
    concat_alloc_t userProperties = {userPropertiesXBuf, sizeof userPropertiesXBuf};

    putUserPropertyPair(&userProperties,"key0", -1, "value0", -1);
    putUserPropertyPair(&userProperties,"key1", -1, "value1", -1);
    putUserPropertyPair(&userProperties,"key2", -1, "value2", -1);

    const char * eventID1 = "This is a very long event key!  ";
    char eventID [256] = {0};
    strcat(eventID, eventID1);
    strcat(eventID, eventID1);
    strcat(eventID, eventID1);
    strcat(eventID, eventID1);
    strcat(eventID, eventID1);
    strcat(eventID, eventID1);


    ism_kafka_makeEventKey(&buf, "myorg", "mytype", "myid", eventID, "myfmt", NULL, (concat_alloc_t *)&userProperties);
    CU_ASSERT(buf.buf[14]>>4 == 1);
    memcpy(uuid, buf.buf+8, 8);
    memcpy(uuid+8, buf.buf, 8);
    ism_common_UUIDtoString(uuid, uuidbuf, sizeof uuidbuf);
    if (debug_mode) {
        printf("key uuid=%s\n", uuidbuf);
        for (i=0; i<16; i++) {
            printf("%2.2x", (uint8_t)uuid[i]);
            if (i%4 == 3)
                printf(" ");
        }
        printf("\n");
    }
    int totalBufLen = 16;
    int totalEventKeyLen=buf.used;
    int proplen = 0;
    char * org = NULL;
    char * propbuf = buf.buf+16; //Skip UUID
    //Check Org
    proplen = (uint16_t)BIGINT16(propbuf);
    propbuf+=2;
    totalBufLen+=2;
    org = alloca(proplen+1);
    memcpy(org, propbuf , proplen);
    org[proplen]=0;
    propbuf+=proplen;
    totalBufLen+=proplen;
    CU_ASSERT( strcmp("myorg", org)==0);
    if (g_verbose)
        printf("\nOrg: len=%d value=%s\n", proplen, org);
    //Check Type
    proplen = (uint16_t)BIGINT16(propbuf);
    propbuf+=2;
    totalBufLen+=2;
    char * type = alloca(proplen+1);
    type[proplen]=0;
    memcpy(type, propbuf , proplen);
    propbuf+=proplen;
    totalBufLen+=proplen;
    CU_ASSERT( strcmp("mytype", type)==0);
    if (g_verbose) {
        printf("Type: len=%d value=%s\n", proplen, type);
    }

	//Check ID
	proplen = (uint16_t)BIGINT16(propbuf);
	propbuf+=2;
	totalBufLen+=2;
	char * id = alloca(proplen+1);
	memcpy(id, propbuf , proplen);
	id[proplen]=0;
	propbuf+=proplen;
	totalBufLen+=proplen;
	CU_ASSERT( strcmp("myid", id)==0);
	if (g_verbose)
        printf("ID: len=%d value=%s\n", proplen, id);

	//Check Event
	proplen = (uint16_t)BIGINT16(propbuf);
	propbuf+=2;
	totalBufLen+=2;
	char * event = alloca(proplen+1);
	memcpy(event, propbuf , proplen);
	event[proplen]=0;
	propbuf+=proplen;
	totalBufLen+=proplen;
	CU_ASSERT( strcmp(eventID, event)==0);
	if (g_verbose)
        printf("Event: len=%d value=%s\n", proplen, event);

	//Check Format
	proplen = (uint16_t)BIGINT16(propbuf);
	propbuf+=2;
	totalBufLen+=2;
	char * fmt = alloca(proplen+1);
	memcpy(fmt, propbuf , proplen);
	fmt[proplen]=0;
	propbuf+=proplen;
	totalBufLen+=proplen;
	CU_ASSERT( strcmp("myfmt", fmt)==0);
	if (g_verbose)
	    printf("Format: len=%d value=%s\n", proplen, fmt);

	if (g_verbose)
        printf("UserProperties: Len: %d eventKeyLeng=%d\n", totalBufLen, totalEventKeyLen);
	int xcount=0;
	char * ckey=alloca(5);
	char * cvalue=alloca(7);

	while(totalBufLen<totalEventKeyLen){
		//Get Key
		sprintf(ckey,"key%d", xcount);
		if (g_verbose)
            printf("ckey %s\n", ckey);
		proplen = (uint16_t)BIGINT16(propbuf);
		propbuf+=2;
		totalBufLen+=2;
		char * key = alloca(proplen+1);
		memcpy(key, propbuf , proplen);
		key[proplen]=0;
		propbuf+=proplen;
		totalBufLen+=proplen;
		CU_ASSERT( strcmp(ckey, key)==0);
		if (g_verbose)
            printf("Key: len=%d value=%s\n", proplen, key);


		//Get Value
		sprintf(cvalue,"value%d", xcount);
		if (g_verbose)
            printf("cvalue %s\n", cvalue);
		proplen = (uint16_t)BIGINT16(propbuf);
		propbuf+=2;
		totalBufLen+=2;
		char * value = alloca(proplen+1);
		memcpy(value, propbuf , proplen);
		value[proplen]=0;
		propbuf+=proplen;
		totalBufLen+=proplen;
		CU_ASSERT( strcmp(cvalue, value)==0);
		if (g_verbose) {
		    printf("value: len=%d value=%s\n", proplen, value);
		    printf("totalBufLen=%d totalEventKeyLen=%d\n", totalBufLen, totalEventKeyLen);
		}
		xcount++;
	}


}

/*
 * Test ism_mqtt_getPublishPayload
 */
void testGetPayload(void) {
    char xbuf[2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    ism_transport_t transport = {0};
    mqttProtoObj_t pobj = {0};
    const char * topic;
    const char * payload;
    int topiclen;
    int payloadlen;

    int cmd;
    int rc;

    transport.pobj = &pobj;
    pobj.mqtt_version = 4;

    /* QoS = 0 MQTTv3.1.1 */
    topic = "this/is/a/topic";
    payload = "This is the payload";
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t  xpmsg1 = {buf.buf+16, buf.used-16, cmd};
    mqtt_pmsg_t * pmsg = &xpmsg1;
    rc = ism_mqtt_getPublishPayload(&transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload, payloadlen));


    /* QoS = 1 MQTTv3.1.1 */
    topic = "this/is/a/topic";
    payload = "This is the payload";
    cmd = MT_PUBLISH<<4 | 2;   /* QoS = 1 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    buf.used += 2;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t  xpmsg2 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg2;
    rc = ism_mqtt_getPublishPayload(&transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 0);
    CU_ASSERT(pmsg->props == NULL);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload, payloadlen));

    /* QoS=0 MQTTv5 proplen=0 */
    pobj.mqtt_version = 5;
    topic = "this/is/a/topic";
    payload = "This is the payload";
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    xbuf[buf.used++] = 0;    /* proplen */
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t  xpmsg3 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg3;
    rc = ism_mqtt_getPublishPayload(&transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 0);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));

    /* QoS=0 MQTTv5 proplen=147 */
    topic = "this/is/a/topic";
    payload = "This is the payload";
    cmd = MT_PUBLISH<<4;   /* QoS = 0 */
    payloadlen = strlen(payload);
    topiclen = strlen(topic);

    buf.used = 18;
    xbuf[16] = (uint8_t)topiclen>>8;
    xbuf[17] = (uint8_t)topiclen;
    ism_common_allocBufferCopyLen(&buf, topic, topiclen);
    ism_common_putMqttVarInt(&buf, 147);   /* proplen */
    buf.used += 147;
    ism_common_allocBufferCopyLen(&buf, payload, payloadlen);
    mqtt_pmsg_t xpmsg4 = {buf.buf+16, buf.used-16, cmd};
    pmsg = &xpmsg4;
    rc = ism_mqtt_getPublishPayload(&transport, pmsg);
    CU_ASSERT(rc == 0);
    CU_ASSERT(pmsg->prop_len == 147);
    CU_ASSERT(pmsg->payload_len == payloadlen);
    CU_ASSERT(pmsg->topic_len == topiclen);
    CU_ASSERT(pmsg->topic_len == topiclen && pmsg->topic != NULL && !memcmp(pmsg->topic, topic, topiclen));
    CU_ASSERT(pmsg->payload_len == payloadlen && pmsg->payload != NULL && !memcmp(pmsg->payload, payload,  payloadlen));
}


extern void test_setUnitTest(int test);
extern void ism_iotrest_setHTTPOutboundEnable(int enable);
extern int test_iotrestReceive(ism_transport_t * transport, char * data, int datalen, int kind);
extern int test_getLastHttpRC();
extern void test_setLastHttpRC(int rc);
extern int test_getInprogress(ism_transport_t * transport);
extern int test_iotrestConnection(ism_transport_t * transport);
extern int test_getLastWaitTimeSecs(void);
extern void test_setLastWaitTimeSecs(int waitTimeSecs);

static void * createValidJsonBuf(void *buf, int *bufLen, const char * propertyName, const char * value) {
	char * newbuf;
	if (!buf) {
		newbuf = malloc(1024);
	} else {
		newbuf = buf;
	}
	*bufLen = sprintf(newbuf, "{\"%s\":%s}", propertyName, value);
	return newbuf;
}

static void * createInvalidWaitBuf(void *buf, int *bufLen, const char * propertyName, const char * value) {
	char * newbuf;
	if (!buf) {
		newbuf = malloc(1024);
	} else {
		newbuf = buf;
	}
	*bufLen = sprintf(newbuf, "{\"%s\":%s,\"A\":3}", propertyName, value);
	return newbuf;
}

static void * createInvalidWaitBuf2(void *buf, int *bufLen, const char * propertyName, const char * value) {
	char * newbuf;
	if (!buf) {
		newbuf = malloc(1024);
	} else {
		newbuf = buf;
	}
	*bufLen = sprintf(newbuf, "{\"%s\":\"%s\"}", propertyName, value);
	return newbuf;
}

static void * createEmptyJsonBuf(void *buf, int *bufLen) {
	char * newbuf;
	if (!buf) {
		newbuf = malloc(1024);
	} else {
		newbuf = buf;
	}
	*bufLen = sprintf(newbuf, "{}");
	return newbuf;
}

static void * createInvalidJsonBuf(void *buf, int *bufLen) {
	char * newbuf;
	if (!buf) {
		newbuf = malloc(1024);
	} else {
		newbuf = buf;
	}
	*bufLen = sprintf(newbuf, "{;}");
	return newbuf;
}

static void * createHttpHeaders(void *buf, int *bufLen, const char * host, int contentLen) {
	char * newbuf;
	if (!buf) {
		newbuf = malloc(1024);
	} else {
		newbuf = buf;
	}

	*bufLen = sprintf(newbuf,
			"Host: %s\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Length: %d\r\n"
			"Accept: application/json\r\n"
			"Accept-Language: us-en\r\n"
			,
			host, contentLen);
    return newbuf;
}

static void * createHttpHeadersWithContentType(void *buf, int *bufLen, const char * host, const char *contentType, int contentLen) {
	char * newbuf;
	if (!buf) {
		newbuf = malloc(1024);
	} else {
		newbuf = buf;
	}

	*bufLen = sprintf(newbuf,
			"Host: %s\r\n"
			"Connection: Keep-Alive\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %d\r\n"
			"Accept: application/json\r\n"
			"Accept-Language: us-en\r\n"
			,
			host, contentType, contentLen);
    return newbuf;
}


static void * createHttpBuffer(void *buf, int *bufLen, const char *requestLine, const char * headers, const char * content) {
	int size = strlen(requestLine) + strlen(headers) + strlen(content) + 256;
	char * newbuf;
	if (!buf) {
		newbuf = malloc(size);
	} else {
		newbuf = realloc(buf, size);
	}
	if (!content) {
	    *bufLen = sprintf(newbuf, "%s%s"
	    		"\r\n",
				requestLine, headers);
	} else {
	    *bufLen = sprintf(newbuf, "%s%s"
	    		"\r\n"
	    		"%s\r\n",
				requestLine, headers, content);
	}

	return newbuf;
}

/**
 * Test iotrestReceive()
 *
 * Here are the path rules.
 * HTTP inbound: (part count is 9)
 *    Application:
 *      /api/v0002/application/types/{deviceType}/devices/{deviceId}/commands/{command}
 *      /api/v0002/device/types/{deviceType}/devices/{deviceId}/events/{event}
 *    Gateway and device:
 *      /api/v0002/device/types/{deviceType}/devices/{deviceId}/events/{event}
 * HTTP outbound: (part count is 10)
 *    Gateway and device:
 *      /api/v0002/device/types/{deviceType}/devices/{deviceId}/commands/{command}/request
 */
void iotrest_receive_device(void) {
	int rc;
	int bufLen;
    int contentLen;
    int headersLen;
    int used;
    int lastWaitTimeSecs;
    char *buffer = 0;
    char content[4096];
    char path[1024];
    char requestLine[2048];
    char *headers = 0;
    char *waitBuf = 0;
    ism_transport_t * transport;

    test_setUnitTest(1);
    ism_iotrest_setHTTPOutboundEnable(1);

    ism_endpoint_t * endpoint = calloc(1, sizeof(ism_endpoint_t) + sizeof(ism_endstat_t));
    endpoint->stats = (ism_endstat_t *)(endpoint+1);
    endpoint->port = 11111;
    endpoint->name = "iotresttest1";
    endpoint->enabled = 1;
    endpoint->maxMsgSize = 131072;
    transport = ism_transport_newTransport(endpoint, 64, 0);
    initTransport(transport);
    transport->protocol = "/api";

    rc = test_iotrestConnection(transport);
    CU_ASSERT(rc == 0);
    if (rc) {
    	ism_transport_freeTransport(transport);
    	free(endpoint);
    	return;
    }

	char * org = "aaaaaa";
	char * type = "type1";
	char * device = "device1";
	char * command = "command1";

    if (!transport->userid) {
		char * clientID = alloca(strlen(org) + strlen(type) + strlen(device) + 10);
		sprintf(clientID, "d:%s:%s:%s", org, type, device);
		transport->userid = clientID;
		if (debug_mode)
		    printf("iotrest_receive: userid=%s\n", transport->userid);
    }

    test_setLastWaitTimeSecs(-1);
    //create Tenant
    ism_tenant_t* tenant = createTestTenant();
    // Test device command (no wildcards)
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/commands/%s/request", type, device, command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request no wildcards path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    // Test device command wildcard
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/commands/+/request", type, device);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with command wildcard path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    // Test type wildcard
    sprintf(path, "/api/v0002/device/types/+/devices/%s/commands/%s/request", device, command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with type wildcard path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 404);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    // Test device wildcard
    sprintf(path, "/api/v0002/device/types/%s/devices/+/commands/%s/request", type, command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with device wildcard path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 404);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    // Test type and device wildcard
    sprintf(path, "/api/v0002/device/types/+/devices/+/commands/%s/request", command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with type and device wildcards path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 404);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    // Test all wildcards
    sprintf(path, "/api/v0002/device/types/+/devices/+/commands/+/request");
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    //printf("%s\n", buffer);
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with all wildcards path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 404);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);


    //Test waitTimeSecs = 0
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/commands/%s/request", type, device, command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "0");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with waitTimeSecs=0 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test waitTImeSecs = 3600
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "3600");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with waitTimeSecs=3600 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 3600);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test waitTImeSecs = -2
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "-2");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with waitTimeSecs=-2 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test waitTImeSecs = 3601
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "3601");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with waitTimeSecs=3601 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test waitTImeSecs = 2.1
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "2.1");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with waitTimeSecs=2.1 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 400);
    //CU_ASSERT(lastWaitTimeSecs == 2);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test {}
    waitBuf = createEmptyJsonBuf(waitBuf, &contentLen);
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with {} path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test invalid property name
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecss", "0");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with invalid property path=%s  rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test invalid waitTimeSecs content #1
    waitBuf = createInvalidWaitBuf(waitBuf, &contentLen, "waitTimeSecs", "0");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with invalid content #1 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test invalid waitTimeSecs content #2
    waitBuf = createInvalidWaitBuf2(waitBuf, &contentLen, "waitTimeSecs", "0");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with invalid content #2 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test invalid Json
    waitBuf = createInvalidJsonBuf(waitBuf, &contentLen);
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Device Command Request with invalid JSON object path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test invalid path
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/commands/%s/invalid/request", type, device, command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "3600");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
    printf("Device Command Request with invalid path path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 404);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test invalid GET
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/commands/%s/request", type, device, command);
    sprintf(requestLine, "GET %s HTTP/1.1\r\n", path);
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "0");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Invalid GET Command Request with waitTimeSecs=0 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 405);
    TestUnlinkTenant(tenant);
    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    ism_transport_freeTransport(transport);
    free(buffer);
    free(headers);
    free(waitBuf);
    free(endpoint);
}

void iotrest_receive_gateway(void) {
	int rc;
	int bufLen;
    int contentLen;
    int headersLen;
    int used;
    char *buffer = 0;
    char content[4096];
    char path[1024];
    char requestLine[2048];
    char *headers = 0;
    char *waitBuf = 0;
    int lastWaitTimeSecs = 0;
    ism_transport_t * transport;

    test_setUnitTest(1);
    ism_iotrest_setHTTPOutboundEnable(1);

    ism_endpoint_t * endpoint = calloc(1, sizeof(ism_endpoint_t) + sizeof(ism_endstat_t));
    endpoint->stats = (ism_endstat_t *)(endpoint+1);
    endpoint->port = 22222;
    endpoint->name = "iotresttest2";
    endpoint->enabled = 1;
    endpoint->maxMsgSize = 131072;
    transport = ism_transport_newTransport(endpoint, 64, 0);
    initTransport(transport);
    transport->protocol = "/api";

    rc = test_iotrestConnection(transport);
    CU_ASSERT(rc == 0);
    if (rc) {
    	ism_transport_freeTransport(transport);
    	free(endpoint);
    	return;
    }

	char * org = "bbbbbb";
	char * type = "type2";
	char * device = "device2";
	char * command = "command2";

    if (!transport->userid) {
		char * clientID = alloca(strlen(org) + strlen(type) + strlen(device) + 10);
		sprintf(clientID, "g:%s:%s:%s", org, type, device);
		transport->userid = clientID;
		if (debug_mode)
		    printf("iotrest_receive: userid=%s\n", transport->userid);
    }

    //create Tenant
    ism_tenant_t* tenant = createTestTenant();
    test_setLastWaitTimeSecs(-1);

    // Test gateway command (no wildcards)
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/commands/%s/request", type, device, command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request no wildcard path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    // Test gateway command wildcard
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/commands/+/request", type, device);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request with command wildcard path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    // Test gateway type wildcard
    sprintf(path, "/api/v0002/device/types/+/devices/%s/commands/%s/request", device, command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request with type wildcard path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    // Test gateway device wildcard
    sprintf(path, "/api/v0002/device/types/%s/devices/+/commands/%s/request", device, command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request with device wildcard path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    // Test gateway type and device wildcard
    sprintf(path, "/api/v0002/device/types/+/devices/+/commands/%s/request", command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request with type and device wildcards path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    // Test all wildcards
    sprintf(path, "/api/v0002/device/types/+/devices/+/commands/+/request");
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request all wildcards path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);


    //Test waitTImeSecs = 0
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/commands/%s/request", type, device, command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "0");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request with waitTimeSecs=0 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test waitTImeSecs = 3600
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "3600");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request with waitTimeSecs=3600 rc=%d, used=%d, last HTTP RC=%d\n", rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 3600);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test waitTImeSecs = -3
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "-3");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request with waitTimeSecs=-3 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test waitTImeSecs = 3601
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "3601");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request with waitTimeSecs=3601 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test waitTImeSecs = 3.1
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "3.1");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request with waitTimeSecs=3.1 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 400);
    //CU_ASSERT(lastWaitTimeSecs == 3);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test {}
    waitBuf = createEmptyJsonBuf(waitBuf, &contentLen);
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request with {} path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    lastWaitTimeSecs = test_getLastWaitTimeSecs();
    CU_ASSERT(rc == 200);
    CU_ASSERT(lastWaitTimeSecs == 0);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test invalid property name
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecss", "0");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request invalid property path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test invalid content #1
    waitBuf = createInvalidWaitBuf(waitBuf, &contentLen, "waitTimeSecs", "0");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request invalid content #1 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test invalid content #2
    waitBuf = createInvalidWaitBuf2(waitBuf, &contentLen, "waitTimeSecs", "0");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request invalid content #2 path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test invalid Json
    waitBuf = createInvalidJsonBuf(waitBuf, &contentLen);
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request invalid JSON object path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 400);

    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    //Test invalid path
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/commands/%s/invalid/request", type, device, command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    waitBuf = createValidJsonBuf(waitBuf, &contentLen, "waitTimeSecs", "3600");
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com", contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, waitBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Gateway Command Request invalid path path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 404);
    TestUnlinkTenant(tenant);
    test_setLastWaitTimeSecs(-1);
    test_setLastHttpRC(0);

    ism_transport_freeTransport(transport);
    free(buffer);
    free(headers);
    free(waitBuf);
    free(endpoint);
}

/**
 * HTTP inbound - Device post event
 * /api/v0002/device/types/{deviceType}/devices/{deviceId}/events/{event}
 */
void iotrest_device_post_event(void) {
	int rc;
	int bufLen;
    int contentLen;
    int headersLen;
    int used;
    char *buffer = 0;
    char path[1024];
    char requestLine[2048];
    char *headers = 0;
    char *eventBuf = 0;
    char *contentType = "application/json";
    ism_transport_t * transport;

    test_setUnitTest(1);
    ism_iotrest_setHTTPOutboundEnable(1);

    ism_endpoint_t * endpoint = calloc(1, sizeof(ism_endpoint_t) + sizeof(ism_endstat_t));
    endpoint->stats = (ism_endstat_t *)(endpoint+1);
    endpoint->port = 33333;
    endpoint->name = "iotresttest3";
    endpoint->enabled = 1;
    endpoint->maxMsgSize = 131072;
    transport = ism_transport_newTransport(endpoint, 64, 0);
    initTransport(transport);
    transport->protocol = "/api";

    rc = test_iotrestConnection(transport);
    CU_ASSERT(rc == 0);
    if (rc) {
    	ism_transport_freeTransport(transport);
    	free(endpoint);
    	return;
    }
    //create Tenant
    ism_tenant_t* tenant = createTestTenant();

	char * org = "cccccc";
	char * type = "type3";
	char * device = "device3";
	char * event = "event3";

    if (!transport->userid) {
		char * clientID = alloca(strlen(org) + strlen(type) + strlen(device) + 10);
		sprintf(clientID, "d:%s:%s:%s", org, type, device);
		transport->userid = clientID;
		if (debug_mode)
		    printf("iotrest_receive: userid=%s\n", transport->userid);
    }

    // Test device event (no wildcards)
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/events/%s", type, device, event);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    eventBuf = createValidJsonBuf(eventBuf, &contentLen, "MyEvent", "0");
    headers = createHttpHeadersWithContentType(headers, &headersLen, "quickstart.xyz.com", contentType, contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, eventBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Post device event no wildcard rc=%d used=%d, last HTTP RC=%d\n", rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 200);

    test_setLastHttpRC(0);

    // Test device event wildcard
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/events/+", type, device);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    eventBuf = createValidJsonBuf(eventBuf, &contentLen, "MyEvent", "0");
    headers = createHttpHeadersWithContentType(headers, &headersLen, "quickstart.xyz.com", contentType, contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, eventBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Post device event with event wildcard rc=%d used=%d, last HTTP RC=%d\n", rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 404);

    test_setLastHttpRC(0);

    // Test type wildcard
    sprintf(path, "/api/v0002/device/types/+/devices/%s/events/%s", device, event);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    eventBuf = createValidJsonBuf(eventBuf, &contentLen, "MyEvent", "0");
    headers = createHttpHeadersWithContentType(headers, &headersLen, "quickstart.xyz.com", contentType, contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, eventBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Post device event with type wildcard path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 404);

    test_setLastHttpRC(0);

    // Test device wildcard
    sprintf(path, "/api/v0002/device/types/%s/devices/+/events/%s", type, event);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    eventBuf = createValidJsonBuf(eventBuf, &contentLen, "MyEvent", "0");
    headers = createHttpHeadersWithContentType(headers, &headersLen, "quickstart.xyz.com", contentType, contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, eventBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Post device event with device wildcard path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 404);

    test_setLastHttpRC(0);


    // Test all wildcards
    sprintf(path, "/api/v0002/device/types/+/devices/+/events/+");
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    eventBuf = createValidJsonBuf(eventBuf, &contentLen, "MyEvent", "0");
    headers = createHttpHeadersWithContentType(headers, &headersLen, "quickstart.xyz.com", contentType, contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, eventBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Post device event with all wildcards path=%s rc=%d used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 404);

    test_setLastHttpRC(0);


    //Test one property valid json
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/events/%s", type, device, event);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    eventBuf = createValidJsonBuf(eventBuf, &contentLen, "MyEvent", "0");
    headers = createHttpHeadersWithContentType(headers, &headersLen, "quickstart.xyz.com", contentType, contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, eventBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Post device event valid Json path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 200);

    test_setLastHttpRC(0);


    //Test invalid path
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/events/%s/invalid", type, device, event);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    eventBuf = createValidJsonBuf(eventBuf, &contentLen, "MyEvent", "3600");
    headers = createHttpHeadersWithContentType(headers, &headersLen, "quickstart.xyz.com", contentType, contentLen + 2);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, eventBuf);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf("Post device event invalid path=%s rc=%d, used=%d, last HTTP RC=%d\n", path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    CU_ASSERT(rc == 404);

    test_setLastHttpRC(0);
    TestUnlinkTenant(tenant);
    ism_transport_freeTransport(transport);
    free(buffer);
    free(headers);
    free(eventBuf);
    free(endpoint);
}


/*
 * Test parse and output kafkaConnection object
 */
void test_kafkaConnection_parse(void) {

	    ism_tenant_t * tenant = calloc(1, sizeof(ism_tenant_t));
	    tenant->name = "testtenant";
	    tenant->topicrule = ism_proxy_getTopicRule("iot-2");


}

void ism_mhub_makeKeyMap(ism_mhub_t * mhub, concat_alloc_t * buf, ism_transport_t * transport, mqtt_pmsg_t * pmsg);
void ism_mhub_makeKey(ism_mhub_t * mhub, concat_alloc_t * buf, const char * org, const char * type, const char * id,
        const char * event, const char * fmt);
uint32_t ism_mhub_getPartition(const char * type, const char * id);
uint32_t ism_mhub_getPartitionMap(ism_mhub_t * mhub, concat_alloc_t * buf, ism_transport_t * transport, mqtt_pmsg_t * pmsg);


void oneMap(ism_mhub_t * mhub, mqtt_pmsg_t * pmsg, const char * topic, const char * tmap, const char * expected) {
    char xbuf [4096];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 0};
    if (topic) {
        pmsg->topic = topic;
        pmsg->topic_len = strlen(topic);
    }
    mhub->keymap = tmap;
    ism_mhub_makeKeyMap(mhub, &buf, NULL, pmsg);
    xbuf[buf.used] = 0;
    if (strcmp(xbuf, expected))
        printf("is:'%s' expected='%s'\n", xbuf, expected);
    CU_ASSERT_STRING_EQUAL(xbuf, expected);
}
void oneMapBin(ism_mhub_t * mhub, mqtt_pmsg_t * pmsg, const char * topic, const char * tmap, const char * expected, int len, int checklen) {
    char xbuf [4096];
    concat_alloc_t buf = {xbuf, sizeof xbuf, 0};
    if (topic) {
        pmsg->topic = topic;
        pmsg->topic_len = strlen(topic);
    }
    mhub->keymap = tmap;
    ism_mhub_makeKeyMap(mhub, &buf, NULL, pmsg);
    CU_ASSERT(buf.used = len);
    CU_ASSERT(!memcmp(buf.buf, expected, checklen));
}

/*
 * Test the mhub key and partition mapper
 */
void test_mhub_mapper(void) {
    ism_mhub_t xmhub = {{0}};
    ism_mhub_t * mhub = &xmhub;
    ism_transport_t xtransport = {0};
    ism_transport_t * transport = &xtransport;
    mqtt_pmsg_t xpmsg = {0};
    mqtt_pmsg_t * pmsg = &xpmsg;
        char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};

    transport->org = "myorg";
    pthread_spin_init(&mhub->lock, 0);
    mhub->keymap = "{${JSON:org:Org},${JSON:deviceType:Type},${JSON:deviceId:Id},${JSON:eventType:Event},${JSON:format:Format},${JSON:timestamp:TimeISO}}";
    pmsg->topic = "";
    ism_mhub_makeKeyMap(mhub, &buf, transport, pmsg);
    ism_common_allocBufferCopyLen(&buf, "", 1);
    if (g_verbose)
        printf("\nmapkey: %s\n", buf.buf);
    buf.used = 0;
    mhub->keymap = "{${JSON?:org:Org},${JSON?:deviceType:Type},${JSON?:deviceId:Id},${JSON?:eventType:Event},${JSON?:format:Format},${JSON:timestamp:TimeISO}}";
    ism_mhub_makeKeyMap(mhub, &buf, transport, pmsg);
    ism_common_allocBufferCopyLen(&buf, "", 1);
    if (g_verbose)
        printf("\nmapkey: %s\n", buf.buf);

    pmsg->type = "type";
    pmsg->id = "id";
    pmsg->event = "event";
    pmsg->format = "format";

    /* Topic mapping tests to match bridge */
    oneMap(mhub, pmsg, "wiotp/event/Topic2/Topic3/MyEvent/etc", "iot-2/type/${Topic2}/id/${Topic3}/evt/${Topic4}/fmt/json'", "iot-2/type/Topic2/id/Topic3/evt/MyEvent/fmt/json'");
    oneMap(mhub, pmsg, "wiotp/event/主题/Topic3/MyEvent/etc", "iot-2/type/${Topic2}/id/${Topic3}/evt/${Topic4}/fmt/json'", "iot-2/type/主题/id/Topic3/evt/MyEvent/fmt/json'");
    oneMap(mhub, pmsg, "wiotp/event/Topic2/Topic3/MyEvent/etc", "物联网-2/type/${Topic2}/id/${Topic3}/evt/${Topic4}/fmt/json'", "物联网-2/type/Topic2/id/Topic3/evt/MyEvent/fmt/json'");
    oneMap(mhub, pmsg, "0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "a${Topic12}e", "ace");
    oneMap(mhub, pmsg, "0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "a${Topic20}e", "ae");
    oneMap(mhub, pmsg, "0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "1${Topic10*}2", "1a/b/c/d/e/f2");
    oneMap(mhub, pmsg, "0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "${Topic2*}/1${Topic10*}2", "2/3/4/5/6/7/8/9/a/b/c/d/e/f/1a/b/c/d/e/f2");
    oneMap(mhub, pmsg, "0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "${abc}", "");
    oneMap(mhub, pmsg, "0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "${JSON:abc:Topic2}", "\"abc\":\"2\"");
    oneMap(mhub, pmsg, "0/1/2/3/4/5/6/7/8/9/a/b/c/d/e/f", "${JSON:abc:Topic12*}", "\"abc\":\"c/d/e/f\"");
    oneMap(mhub, pmsg, "0/1", "{${JSON?:f1:Topic5}, ${JSON?:f2:Topic3},${JSON?:f3:Topic2},${JSON?:f4:Topic0}}", "{ \"f4\":\"0\"}");
    oneMap(mhub, pmsg, "0/1", "[${JSON?::Topic5},${JSON?::Topic3},${JSON?::Topic2},${JSON?::Topic0}]", "[\"0\"]");
    oneMap(mhub, pmsg, "0/1/2/3/4/5/6", "[${JSON?::Topic5},${JSON?::Topic3},${JSON?::Topic2},${JSON?::Topic0}]", "[\"5\",\"3\",\"2\",\"0\"]");
    oneMap(mhub, pmsg, "wiotp////t4", "iot-2/${Topic1*}", "iot-2////t4");
    oneMapBin(mhub, pmsg, "g/h/i/j/k/l/m/n/o/p", "${USTR:Topic2}${USTR:Topic4}${USTR:Topic6}${USTR:Topic8}",
            "\0\1i\0\1k\0\1m\0\1o", 12, 12);
    oneMapBin(mhub, pmsg, "g/h/i/j/k", "${USTR?:Topic2}${USTR?:Topic4}${USTR?:Topic6}${USTR?:Topic8}",
            "\0\1i\0\1k", 6, 6);
    oneMapBin(mhub, pmsg, "g/h/i/j/k/l/m/n/o/p", "${USTR:Topic2}${USTR:Topic4}${USTR:Topic6}${USTR:Topic8}${TimeUUID}",
            "\0\1i\0\1k\0\1m\0\1o", 28, 12);
}

void tenantTest(void) {
    ism_tenant_t* tenant = createTestTenant();
    TestUnlinkTenant(tenant);
}

ism_tenant_t* createTestTenant(void) {
    int rc;
    const char * serverConfig = "{ \"Server\": { \"QuickstartServer\": { "
          "\"Address\": [ \"testserver\", \"10.0.0.66\" ],"
          "\"Port\": 1883"
          "} } }";
    const char * tenantConfig="{ "
    		"\"Tenant\": {	                           "
        " \"quickstart\": {                        "
        "    \"Enabled\": true,                   "
        "    \"Server\": \"QuickstartServer\",  	"
        "    \"AllowDurable\": false,       	    "
        "    \"AllowSysTopic\": false,           "
        "    \"AllowAnonymous\": true,    	    "
        "    \"AllowRetain\": false,			    "
        "    \"CheckUser\": true,                 "
        "    \"AllowShared\": false,             "
        "    \"MaxConnections\": 50000,		    "
        "    \"MaxQoS\": 0,                 	    "
        "    \"ChangeTopic\": \"quickstart2\",   "
        "    \"RequireSecure\": false,           "
        "    \"RequireCertificate\": false, 	    "
        "    \"CheckTopic\": \"quickstart2\",     "
        "   \"Port\":  8883                      "
        "   }                                      "
        "  }                                      "
        "}                                      ";

    ism_common_setTraceLevel(2);
    //Server Object is required for Tenant creation
    CU_ASSERT((rc = doConfig(serverConfig, -1)) == 0);
    CU_ASSERT((rc = doConfig(tenantConfig, -1)) == 0);
    ism_tenant_t * tenant = 0;
    if (rc == 0) {
        //Get Tenant
        tenant = ism_tenant_getTenant("quickstart");

        //Inspect Tenant
        CU_ASSERT(tenant != NULL);
        CU_ASSERT(tenant->max_qos == 0);
        CU_ASSERT(tenant->max_connects == 50000);
        CU_ASSERT(tenant->allow_durable == 0);
        CU_ASSERT(tenant->allow_anon == 1);
        CU_ASSERT(tenant->allow_systopic == 0);
        CU_ASSERT(tenant->allow_shared == 0);
    }
    return tenant;
}

ism_tenant_t * createSecureTenant(void) {
    int rc;
    const char * serverConfig = "{ \"Server\": { \"QuickstartServer\": { "
          "\"Address\": [ \"testserver\", \"10.0.0.66\" ],"
          "\"Port\": 1883"
          "} } }";
    const char * tenantConfig="{ "
            "\"Tenant\": {                             "
        " \"quickstart\": {                        "
        "    \"Enabled\": true,                   "
        "    \"Server\": \"QuickstartServer\",      "
        "    \"AllowDurable\": false,               "
        "    \"AllowSysTopic\": false,           "
        "    \"AllowAnonymous\": true,          "
        "    \"AllowRetain\": false,                "
        "    \"CheckUser\": true,                 "
        "    \"AllowShared\": false,             "
        "    \"MaxConnections\": 50000,         "
        "    \"MaxQoS\": 0,                         "
        "    \"ChangeTopic\": \"quickstart2\",   "
        "    \"RequireSecure\": true,           "
        "    \"RequireCertificate\": false,         "
        "    \"CheckTopic\": \"quickstart2\",     "
        "   \"Port\":  8883                      "
        "   }                                      "
        "  }                                      "
        "}                                      ";

    ism_common_setTraceLevel(2);
    //Server Object is required for Tenant creation
    CU_ASSERT((rc = doConfig(serverConfig, -1)) == 0);
    CU_ASSERT((rc = doConfig(tenantConfig, -1)) == 0);
    ism_tenant_t * tenant = 0;
    if (rc == 0) {
        //Get Tenant
        tenant = ism_tenant_getTenant("quickstart");

        //Inspect Tenant
        CU_ASSERT(tenant != NULL);
        CU_ASSERT(tenant->max_qos == 0);
        CU_ASSERT(tenant->max_connects == 50000);
        CU_ASSERT(tenant->allow_durable == 0);
        CU_ASSERT(tenant->allow_anon == 1);
        CU_ASSERT(tenant->allow_systopic == 0);
        CU_ASSERT(tenant->allow_shared == 0);
    }
    return tenant;
}

void TestUnlinkTenant(ism_tenant_t * tenant) {
    //Unlink Tenant
    //Note: Unlink TEnant will not destroy the tenant.
    //This is an acceptable risk of memory leaks
    unlinkTenant(tenant);

    //Tenant Should be NULL after unlink
    tenant = ism_tenant_getTenant("quickstart");
    CU_ASSERT(tenant == NULL);

    //Get not exist tenant
    tenant = ism_tenant_getTenant("quickstart_nonexist");
    CU_ASSERT(tenant == NULL);

    //Get Tenant with NULL Name
    tenant = ism_tenant_getTenant(0);
    CU_ASSERT(tenant == NULL);

    //Get Tenant with Empty Name
    tenant = ism_tenant_getTenant("");
    CU_ASSERT(tenant == NULL);
}

void iotrest_nonSecure(void) {
    int rc;
    int bufLen;
    int contentLen;
    int headersLen;
    int used;
    char *buffer = 0;
    char content[4096];
    char path[1024];
    char requestLine[2048];
    char *headers = 0;
    ism_transport_t * transport;

    test_setUnitTest(1);
    ism_iotrest_setHTTPOutboundEnable(1);

    ism_endpoint_t * endpoint = calloc(1,
            sizeof(ism_endpoint_t) + sizeof(ism_endstat_t));
    endpoint->stats = (ism_endstat_t *) (endpoint + 1);
    endpoint->port = 11111;
    endpoint->name = "iotresttest1";
    endpoint->enabled = 1;
    endpoint->maxMsgSize = 131072;
    transport = ism_transport_newTransport(endpoint, 64, 0);
    initTransport(transport);
    transport->protocol = "/api";

    rc = test_iotrestConnection(transport);
    CU_ASSERT(rc == 0);
    if (rc) {
        ism_transport_freeTransport(transport);
        free(endpoint);
        return;
    }

    char * org = "aaaaaa";
    char * type = "type1";
    char * device = "device1";
    char * command = "command1";

    if (!transport->userid) {
        char * clientID = alloca(
                strlen(org) + strlen(type) + strlen(device) + 10);
        sprintf(clientID, "d:%s:%s:%s", org, type, device);
        transport->userid = clientID;
        if (debug_mode)
            printf("iotrest_receive: userid=%s\n", transport->userid);
    }

    test_setLastWaitTimeSecs(-1);
    //create Tenant
    ism_tenant_t * tenant = createSecureTenant();
    // Test device command (no wildcards)
    sprintf(path, "/api/v0002/device/types/%s/devices/%s/commands/%s/request",
            type, device, command);
    sprintf(requestLine, "POST %s HTTP/1.1\r\n", path);
    contentLen = 0;
    content[0] = 0;
    headers = createHttpHeaders(headers, &headersLen, "quickstart.xyz.com",
            contentLen);
    buffer = createHttpBuffer(buffer, &bufLen, requestLine, headers, content);
    used = 0;
    rc = parseWSHandshake(transport, buffer, bufLen, &used);
    if (debug_mode)
        printf(
                "Device Command Request no wildcards path=%s rc=%d used=%d, last HTTP RC=%d\n",
                path, rc, used, test_getLastHttpRC());
    rc = test_getLastHttpRC();
    TestUnlinkTenant(tenant);
    CU_ASSERT(rc == 0);
}

/*
 * Test performance of builtin and mapper key generation and partition map
 */
void test_mhub_mapper_perf(void) {
    int count = 100000;
    int i;
    ism_mhub_t xmhub = {{0}};
    ism_mhub_t * mhub = &xmhub;
    ism_transport_t xtransport = {0};
    ism_transport_t * transport = &xtransport;
    mqtt_pmsg_t xpmsg = {0};
    mqtt_pmsg_t * pmsg = &xpmsg;
    char xbuf [2048];
    concat_alloc_t buf = {xbuf, sizeof xbuf};

    transport->org = "myorg";
    pthread_spin_init(&mhub->lock, 0);
    pthread_spin_init(&mhub->tslock, 0);
    mhub->ts = ism_common_openTimestamp(ISM_TZF_UTC);

    mhub->keymap = "{${JSON?:org:Org},${JSON?:deviceType:Type},${JSON?:deviceId:Id},${JSON?:eventType:Event},${JSON?:format:Format},${JSON:timestamp:TimeISO}}";
    pmsg->topic = "";
    pmsg->type = "mytype";
    pmsg->id = "mydeviceid";
    pmsg->event = "myevent";
    pmsg->format = "format";
    ism_mhub_makeKeyMap(mhub, &buf, transport, pmsg);
    ism_common_allocBufferCopyLen(&buf, "", 1);
    if (g_verbose)
        printf("\nmapkey: %s\n", buf.buf);
    int genoffset = buf.used;
    char * chkpos = strstr(buf.buf, ":\"20");
    int chklen = 0;
    if (chkpos)
        chklen = chkpos-buf.buf;
    ism_mhub_makeKey(mhub, &buf, transport->org, pmsg->type, pmsg->id, pmsg->event, pmsg->format);
    ism_common_allocBufferCopyLen(&buf, "", 1);
    if (g_verbose)
        printf("genkey: %s\n", buf.buf+genoffset);
    CU_ASSERT(chklen>0 && !memcmp(buf.buf, buf.buf+genoffset, chklen));

    ism_time_t start1 = ism_common_currentTimeNanos();
    for (i=0; i<count; i++) {
        buf.used = 0;
        ism_mhub_makeKeyMap(mhub, &buf, transport, pmsg);
    }
    ism_time_t end1 = ism_common_currentTimeNanos();
    for (i=0; i<count; i++) {
        buf.used = 0;
        ism_mhub_makeKey(mhub, &buf, transport->org, pmsg->type, pmsg->id, pmsg->event, pmsg->format);
    }
    ism_time_t end2 = ism_common_currentTimeNanos();


    double maptime = (double)(end1-start1)/1e9;
    double gentime = (double)(end2-end1)/1e9;
    printf("\ncount=%u  maptime=%0.03g sec gentime=%0.03g sec\n", count, maptime, gentime);
    CU_ASSERT(maptime < gentime*4);

    uint32_t  partmap;
    uint32_t  partgen;
    mhub->partitionMap = "${Type}${Id}";
    count *= 10;
    start1 = ism_common_currentTimeNanos();
    for (i=0; i<count; i++) {
        buf.used = 0;
        partmap = ism_mhub_getPartitionMap(mhub, &buf, transport, pmsg);
    }
    end1 = ism_common_currentTimeNanos();
    for (i=0; i<count; i++) {
        partgen = ism_mhub_getPartition(pmsg->type, pmsg->id);
    }
    end2 = ism_common_currentTimeNanos();

    CU_ASSERT(partmap == partgen);
    if (partmap != partgen) {
        printf("partgen=%08x partmap=%08x\n", partgen, partmap);
    }

    maptime = (double)(end1-start1)/1e9;
    gentime = (double)(end2-end1)/1e9;
    printf("count=%u partmap=%0.03g sec partgen=%0.03g sec\n", count, maptime, gentime);
    //CU_ASSERT(maptime < gentime*3); //These tests failed consistently on mar030.
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

int doConfig(const char * json, int len) {
    int rc;
    ism_json_parse_t parseobj = { 0 };
    ism_json_entry_t ents[500];
    parseobj.ent_alloc = 500;
    parseobj.ent = ents;
    if (len < 0)
        len = strlen(json);
    parseobj.source = alloca(len+1);
    memcpy(parseobj.source, json, len);
    parseobj.source[len] = 0;
    parseobj.src_len = len;
    parseobj.options = JSON_OPTION_COMMENT;   /* Allow C style comments */
    rc = ism_json_parse(&parseobj);
    if (rc == 0) {
        rc = ism_proxy_complexConfig(&parseobj, 2, 0, 0);
    }
    return rc;
}

char g_statsdResult[4096];
int statsd_send(statsd_link * link, const char * msg) {
    ism_common_strlcpy(g_statsdResult, msg, sizeof g_statsdResult);
    return 0;
}

void statsd_prepare(statsd_link * link, char * stat, size_t value, const char * type, float sample_rate, char * buf, size_t buflen, int lf) {
    snprintf(buf, buflen, "%s=%u\n", stat, (uint32_t)value);
}



void updateStatsTest(void) {
    int rc;
    ism_field_t f;
    const char * config = "{ \"Server\": { \"fred\": { "
        "\"Address\": [ \"kwb999\", \"10.0.0.66\" ],"
        "\"Port\": 1883"
        "} } }";
    ism_common_setTraceLevel(2);
    tlsInited = 1;
    f.type  = VT_Integer;
    f.val.i = 0;
    ism_common_setProperty(ism_common_getConfigProperties(), "MqttUseMux", &f);
    CU_ASSERT((rc = doConfig(config, -1)) == 0);
    ism_common_setTraceLevel(2);
    if (rc == 0) {
        updateServersStats();
        // CU_ASSERT(strstr(g_statsdResult, "Server.fred.primary.10_0_0_66_1883.useCount") != NULL);
        CU_ASSERT(strstr(g_statsdResult, "Server.fred.primary.kwb999_1883.useCount") == NULL);
        if (g_verbose)
            printf("statsd=\n%s\n", g_statsdResult);
    }
}

#include "pxmqtt_test.c"
#include "pxrouting_test.c"
/*
 * Main entry point
 */
int main(int argc, char * * argv) {
    int trclvl = 2;

    ism_common_initUtil();

    printf("%s\n", ism_common_getPlatformInfo());
    printf("%s\n\n", ism_common_getProcessorInfo());
    ism_common_setTraceLevel(trclvl);
    // ism_defaultTrace->logLevel[2] = 2;

    /* Run tests */
    Startup_CUnit(argc, argv);

    /* Print results */
    summary("\n\n Test: --- Testing ISM Transports --- ...\n");
    print_final_summary();

    Endup_CUnit();

    return RC;
}
