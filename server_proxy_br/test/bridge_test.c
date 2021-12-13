/*
 * Copyright (c) 2018-2021 Contributors to the Eclipse Foundation
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
#include "bridge.c"
#include "topicMapping_test.c"

#define DISPLAY_VERBOSE       0
#define DISPLAY_QUIET         1

#define BASIC_TEST_MODE       0
#define FULL_TEST_MODE        1
#define BYNAME_TEST_MODE      2


void ism_http_free(ism_http_t * http);

/*
 * Global Variables
 */
int debug_mode = 0;
int display_mode = DISPLAY_VERBOSE; /* initial display mode */
int default_display_mode = DISPLAY_VERBOSE; /* default display mode */
int test_mode = BASIC_TEST_MODE;
int default_test_mode = BASIC_TEST_MODE;
int g_rc = 0;
int g_verbose = 0;

void selectorTest(void);
void propgenTest(void);
void testRestGET(void);
void testRestPOST(void);
void testRestPOSTunknown(void);
void testRestPOSTforwarder(void);
void testRestPOSTconnection(void);
void testRestPOSTendpoint(void);
void testRestPOSTuser(void);
void testRestDELETE(void);


/*
 * Array that carries all the transport tests for APIs to CUnit framework.
 */
CU_TestInfo bridge_tests[] = {
    { "--- Testing selector                  ---", selectorTest },
    { "--- Testing propgen                   ---", propgenTest },
    { "--- Testing REST GET interfaces       ---", testRestGET },
    { "--- Testing REST POST interfaces      ---", testRestPOST },
    { "--- Testing REST POST unknown         ---", testRestPOSTunknown },
    { "--- Testing REST POST forwarder err   ---", testRestPOSTforwarder },
    { "--- Testing REST POST connection err  ---", testRestPOSTconnection },
    { "--- Testing REST POST endpoint err    ---", testRestPOSTendpoint },
    { "--- Testing REST POST user err        ---", testRestPOSTuser },
    { "--- Testing REST DELETE interfaces    ---", testRestDELETE },
    { "--- Testing topic mapping             ---", topicMappingTest },
    CU_TEST_INFO_NULL
};

/*
 * Array that carries the basic test suite and other functions to the CUnit framework
 */
CU_SuiteInfo  ism_bridge_CUnit_test_basicsuites[] = {
    IMA_TEST_SUITE("--- Convert test ---", NULL, NULL,  (CU_TestInfo *)bridge_tests),
    CU_SUITE_INFO_NULL,
};

/*
 * This is the main CUnit routine that starts the CUnit framework.
 * CU_basic_run_tests() Actually runs all the test routines.
 */
void Startup_CUnit(int argc, char ** argv) {
    CU_SuiteInfo * runsuite;

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
        runsuite = ism_bridge_CUnit_test_basicsuites;
    else
        runsuite = ism_bridge_CUnit_test_basicsuites;

    // display_mode &= default_display_mode;            // disable the CU_BRM_SILENT mode
    setvbuf(stdout, NULL, _IONBF, 0);
    if (CU_initialize_registry() == CUE_SUCCESS) {
        if (CU_register_suites(runsuite) == CUE_SUCCESS) {
            CU_basic_set_mode(CU_BRM_VERBOSE);
            CU_basic_run_tests();
        }
    }
}

mqtt_prop_ctx_t * ism_proxy_getMqttContext(int version);
const char * ism_log_getCategoryName(int32_t id);
int ism_proxy_httpRestCall(ism_transport_t * transport, ism_http_t * http, uint64_t async);


/*
 * This routine closes CUnit environment, and needs to be called only before the exiting of the program.
 */
void Endup_CUnit(void) {
    CU_cleanup_registry();
}

int g_logstack  [32];
int g_logstack_depth = 0;

/*
 * Reimplement the log implementation for this test.
 * Just record the log number for now.
 *
 * TODO: record some of the data for select log values
 */
XAPI void ism_common_logInvoke(ism_common_log_context *context, const ISM_LOGLEV level, int msgnum, const char * msgID, int32_t category,
        ism_trclevel_t * trclvl, const char * func, const char * file, int line,
        const char * types, const char * fmts, ...) {
    if (g_logstack_depth < 32) {
        g_logstack[g_logstack_depth++] = msgnum;
        if (g_verbose) {
            printf("log msgnum=%u msgID=%s category=%s file=%s line=%u msg=%s\n",
                    msgnum, msgID, ism_log_getCategoryName(category), file, line, fmts);
        }
    }
}

/*
 * Clear the log entry stack
 */
void clearLogStack(void) {
    g_logstack_depth = 0;
}

/*
 * Get the last log entry and pop the stack
 */
int getLastLog(void) {
    if (g_logstack_depth == 0)
        return 0;
    return g_logstack[--g_logstack_depth];
}

/*
 * Test bridge propgen
 * This test properties for replacement values
 */
void propgenTest(void) {
    g_ctx5 = ism_proxy_getMqttContext(5);
    char xbuf [4096];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    ism_emsg_t  emsg = {0};
    ism_prop_t * props = ism_common_newProperties(100);
    ism_field_t f;
    int rc;

    ism_common_putMqttPropField(&buf, MPI_PayloadFormat, g_ctx5, 1);
    ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "prop1", -1, "val1", -1);
    ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "prop1", -1, "val2", -1);
    ism_common_putMqttPropField(&buf, MPI_SubID, g_ctx5, 77);
    emsg.otherprops = buf.buf;
    emsg.otherprop_len = buf.used;
    f.type = VT_Integer;

    rc = ism_mqtt_propgen(props, &emsg, "another", &f, NULL, NULL);
    CU_ASSERT(rc == 1);
    CU_ASSERT(f.type == VT_Null);

    rc = ism_mqtt_propgen(props, &emsg, "_ContentType", &f, NULL, NULL);
    CU_ASSERT(rc == 1);
    CU_ASSERT(f.type == VT_Null);

    rc = ism_mqtt_propgen(props, &emsg, "_ReplyTo", &f, NULL, NULL);
    CU_ASSERT(rc == 1);
    CU_ASSERT(f.type == VT_Null);

    rc = ism_mqtt_propgen(props, &emsg, "_Correlation", &f, NULL, NULL);
    CU_ASSERT(rc == 1);
    CU_ASSERT(f.type == VT_Null);

    ism_common_putMqttPropString(&buf, MPI_ContentType, g_ctx5, "fred", -1);
    ism_common_putMqttPropString(&buf, MPI_Correlation, g_ctx5, "abcd", 4);
    ism_common_putMqttPropString(&buf, MPI_ReplyTopic, g_ctx5, "output/topic", -1);
    ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "another", -1, "anotherval", -1);
    ism_common_putMqttPropField(&buf, MPI_TopicAlias, g_ctx5, 456);

    rc = ism_mqtt_propgen(props, &emsg, "another", &f, NULL, NULL);
    CU_ASSERT(rc == 1);
    CU_ASSERT(f.type == VT_Null);

    emsg.otherprop_len = buf.used;
    ism_common_clearProperties(props);
    rc = ism_mqtt_propgen(props, &emsg, "prop1", &f, NULL, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(f.type == VT_String);
    CU_ASSERT(f.val.s && !strcmp(f.val.s, "val2"));

    rc = ism_mqtt_propgen(props, &emsg, "_ContentType", &f, NULL, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(f.type == VT_String);
    CU_ASSERT(f.val.s && !strcmp(f.val.s, "fred"));

    rc = ism_mqtt_propgen(props, &emsg, "_Correlation", &f, NULL, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(f.type == VT_String);
    CU_ASSERT(f.val.s && !strcmp(f.val.s, "abcd"));

    rc = ism_mqtt_propgen(props, &emsg, "_ReplyTo", &f, NULL, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(f.type == VT_String);
    CU_ASSERT(f.val.s && !strcmp(f.val.s, "output/topic"));

    rc = ism_mqtt_propgen(props, &emsg, "another", &f, NULL, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(f.type == VT_String);
    CU_ASSERT(f.val.s && !strcmp(f.val.s, "anotherval"));

    buf.used = 0;
    ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "prop1", -1, "val2", -1);
    ism_common_putMqttPropString(&buf, MPI_Correlation, g_ctx5, "\0\0\0\4", 4);
    emsg.otherprop_len = buf.used;
    ism_common_clearProperties(props);

    rc = ism_mqtt_propgen(props, &emsg, "_Correlation", &f, NULL, NULL);
    CU_ASSERT(rc == 1);
    CU_ASSERT(f.type == VT_Null);

    rc = ism_mqtt_propgen(props, &emsg, "prop1", &f, NULL, NULL);
    CU_ASSERT(rc == 0);
    CU_ASSERT(f.type == VT_String);
    CU_ASSERT(f.val.s && !strcmp(f.val.s, "val2"));
}



/*
 * Test selector as used in bridge
 */
void selectorTest(void) {
    /* TODO */
    g_ctx5 = ism_proxy_getMqttContext(5);
    char xbuf [4096];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    ism_prop_t * props = ism_common_newProperties(100);
    ism_forwarder_t forwarder = {{0}};
    mqttbrMsg_t mmsg = {0};
    const char * rule;
    int rc;
    int rulelen;

    /* Fill in a few props */
    ism_common_putMqttPropField(&buf, MPI_PayloadFormat, g_ctx5, 1);
    ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "prop1", -1, "val1", -1);
    ism_common_putMqttPropField(&buf, MPI_SubID, g_ctx5, 77);
    ism_common_putMqttPropString(&buf, MPI_ContentType, g_ctx5, "fred", -1);
    ism_common_putMqttPropString(&buf, MPI_Correlation, g_ctx5, "abcd", 4);
    ism_common_putMqttPropString(&buf, MPI_ReplyTopic, g_ctx5, "output/topic", -1);
    ism_common_putMqttPropNamePair(&buf, MPI_UserProperty, g_ctx5, "another", -1, "anotherval", -1);
    ism_common_putMqttPropField(&buf, MPI_TopicAlias, g_ctx5, 456);

    pthread_spin_init(&forwarder.lock, 0);
    forwarder.props = props;
    mmsg.qos = 2;
    mmsg.props = buf.buf;
    mmsg.prop_len = buf.used;
    mmsg.topic = "this/is/a/topic";

    rule = "true";
    CU_ASSERT((rc = ism_common_compileSelectRuleOpt(&forwarder.selector, &rulelen, rule, SELOPT_Internal)) == 0);
    if (rc == 0)
        CU_ASSERT(selectMsg(&forwarder, &mmsg) == SELECT_TRUE);

    rule = "false";
    CU_ASSERT((rc = ism_common_compileSelectRuleOpt(&forwarder.selector, &rulelen, rule, SELOPT_Internal)) == 0);
    if (rc == 0)
        CU_ASSERT(selectMsg(&forwarder, &mmsg) == SELECT_FALSE);

    rule = "Topic1 = 'is' and Topic3 = 'topic'";
    CU_ASSERT((rc = ism_common_compileSelectRuleOpt(&forwarder.selector, &rulelen, rule, SELOPT_Internal)) == 0);
    if (rc == 0)
        CU_ASSERT(selectMsg(&forwarder, &mmsg) == SELECT_TRUE);

    rule = "Topic like 'this/%/topic'";
    CU_ASSERT((rc = ism_common_compileSelectRuleOpt(&forwarder.selector, &rulelen, rule, SELOPT_Internal)) == 0);
    if (rc == 0)
        CU_ASSERT(selectMsg(&forwarder, &mmsg) == SELECT_TRUE);

    rule = "Topic1 in ('this', 'that')";
    CU_ASSERT((rc = ism_common_compileSelectRuleOpt(&forwarder.selector, &rulelen, rule, SELOPT_Internal)) == 0);
    if (rc == 0)
        CU_ASSERT(selectMsg(&forwarder, &mmsg) == SELECT_FALSE);

    rule = "Topic0 in ('this', 'that')";
    CU_ASSERT((rc = ism_common_compileSelectRuleOpt(&forwarder.selector, &rulelen, rule, SELOPT_Internal)) == 0);
    if (rc == 0)
        CU_ASSERT(selectMsg(&forwarder, &mmsg) == SELECT_TRUE);

    rule = "QoS between 0 and 3";
    CU_ASSERT((rc = ism_common_compileSelectRuleOpt(&forwarder.selector, &rulelen, rule, SELOPT_Internal)) == 0);
    if (rc == 0)
        CU_ASSERT(selectMsg(&forwarder, &mmsg) == SELECT_TRUE);

    rule = "fred = 'abc' or Topic like '%topic%'";
    CU_ASSERT((rc = ism_common_compileSelectRuleOpt(&forwarder.selector, &rulelen, rule, SELOPT_Internal)) == 0);
    if (rc == 0)
        CU_ASSERT(selectMsg(&forwarder, &mmsg) == SELECT_TRUE);

    rule = "fred is not null";
    CU_ASSERT((rc = ism_common_compileSelectRuleOpt(&forwarder.selector, &rulelen, rule, SELOPT_Internal)) == 0);
    if (rc == 0)
        CU_ASSERT(selectMsg(&forwarder, &mmsg) == SELECT_FALSE);

}

/*
 * Change a string in place.
 * Normally the to and from strings are the same length, but if they differ one is found
 * and the other overlayed at that position.
 */
int changeStr(char * str, const char * from, const char * to) {
    int tolen = strlen(to);
    char * pos = strstr(str, from);
    if (!pos)
        return 1;
    if (strlen(pos) < tolen)
        return 1;
    memcpy(pos, to, tolen);
    return 0;
}


int g_entered_close_cb = 0;
int g_entered_send_cb = 0;

/**
 * The close method for the transport object used for tests in
 * this source file.
 */
int mclose(ism_transport_t * transport, int rc, int clean, const char * reason) {
    g_entered_close_cb++;
    if (g_verbose > 1 && g_entered_close_cb == 1)
        printf("close connect=%d rc=%d reason=%s\n", transport->serverport, rc, reason);
    if (transport->state == ISM_TRANST_Open) {
        transport->state = ISM_TRANST_Closing;
        transport->closing(transport, rc, clean, reason);
    }
    return 0;
}

/*
 * The closed method for the transport object used for tests in * this source file.
 */
int mclosed(ism_transport_t * trans) {
    return 0;
}

/*
 * The send method for the transport object used for tests in this source file.
 */
int msend(ism_transport_t * trans, char * buf, int len, int kind, int flags) {
    g_entered_send_cb++;
    return 0;
}

static ism_endstat_t testOutStat = { 0 };
/*
 * Dummy endpoint so we do not segfault
 */
static ism_endpoint_t testEndpoint = {
    .name        = "!BridgeTest",
    .ipaddr      = "",
    .transport_type = "TCP",
    .protomask   = PMASK_Internal,
    .thread_count = 1,
    .stats = &testOutStat,
};

/*
 * Make fake transport
 */
void setupTransport(ism_transport_t *transport, const char * protocol, struct ism_protobj_t * pobj, int line) {
    memset(transport, 0, sizeof(ism_transport_t));
    memset(pobj, 0, sizeof(ism_protobj_t));
    transport->protocol = protocol;
    transport->protocol_family = protocol;
    transport->close = mclose;
    transport->closed = mclosed;
    transport->send = msend;
    transport->pobj = pobj;
    transport->serverport = line;
    transport->state = ISM_TRANST_Open;
    transport->trclevel = ism_defaultTrace;
    if (pobj) {
        pobj->transport = transport;
        pobj->startState = 0;
    } else {
        transport->suballoc.size = 1024;
    }
    transport->endpoint = &testEndpoint;
}


/*
 * Make a call to the bridge REST API.
 *
 * This returns an http object which remains valid until the next call to callRest using the same transport
 * object.  The HTTP status code is returned in http->val1 and the ISMRC is returned in http->val2.  The
 * response content is available in the http->outbuf.
 *
 * This tests the bridge REST support, but not the transport or HTTP parser which this code bypasses.
 *
 * This method call the REST API it then asserts:
 * - That the HTTP status is as expected
 * - That the log entry produced is as expected
 * - The output buffer is not empty
 * - If the output buffer appears to be JSON that the JSON is validI
 *
 * This only works for REST APIs which return synchronously.
 */
ism_http_t * callRest(ism_transport_t * transport, const char * op, const char * path, const char * content,
        int expectrc, int expectlog, int line) {
    int content_len;
    ism_http_t * http;
    char xbuf[256];
    int http_op;
    int rc = 0;

    /* Map from the op name to the op value */
    if (!op) {
        http_op = 'G';
    } else {
        http_op = *op;
        if (http_op == 'P' && op[1]=='U')
            http_op = 'U';
    }

    if (g_verbose) {
        fflush(stderr); printf("test at line: %d\n", line); fflush(stdout);
    }

    /* Free up the previous http object */
    if (transport->http)
        ism_http_free(transport->http);
    clearLogStack();

    /* Make a new http object */
    content_len = content ? strlen(content) : 0;
    http = ism_http_newHttp(http_op, path, NULL, NULL, (char *)content, content_len, "text/plain", NULL, 0, 2048);
    http->user_path = http->path;    /* Tests do not put in the /admin "  */
    transport->serverport = line;
    http->transport = transport;
    transport->http = http;

    /* Make the call with a short-circuit return */
     http->norespond = 1;    /* Just return rather than sending it back over the wire */
    ism_proxy_httpRestCall(transport, http, 0);

    /* Check the HTTP status which is returned in http->val1 */
    sprintf(xbuf, "http status expected=%d actual=%d", expectrc, http->val1);
    CU_assertImplementation(http->val1 == expectrc, line, xbuf, __FILE__, "", CU_FALSE);

    /* Check the log entry */
    int logent = getLastLog();
    int logent2 = getLastLog();
    if (logent2) {
        sprintf(xbuf, "log entry expected=%d actual=%d,%d", expectlog, logent2, logent);
        CU_assertImplementation(logent == expectlog || logent2 == expectlog, line, xbuf, __FILE__, "", CU_FALSE);
    } else {
        sprintf(xbuf, "log entry expected=%d actual=%d", expectlog, logent);
        CU_assertImplementation(logent == expectlog, line, xbuf, __FILE__, "", CU_FALSE);
    }

    /* Check that we returned something */
    CU_assertImplementation(http->outbuf.used > 0, line, "REST API returned content", __FILE__, "", CU_FALSE);

    /* Parse the buffer to check for valid JSON if it appears to be JSON */
    if (http->outbuf.used > 0 && *http->outbuf.buf == '{') {
        ism_json_parse_t parseobj = { 0 };
        ism_json_entry_t ents[500];
        parseobj.ent_alloc = 500;
        parseobj.ent = ents;
        parseobj.source = alloca(http->outbuf.used + 1);
        memcpy(parseobj.source, http->outbuf.buf, http->outbuf.used);
        parseobj.source[http->outbuf.used] = 0;
        parseobj.src_len = http->outbuf.len;
        parseobj.options = JSON_OPTION_COMMENT;   /* Allow C style comments */
        rc = ism_json_parse(&parseobj);
        CU_assertImplementation(rc==0, line, "REST API output is valid JSON", __FILE__, "", CU_FALSE);
    }
    if (g_verbose && http->outbuf.used && (rc || http->val1 != 200)) {
        printf("content='%s'\n", http->outbuf.buf);
    }
    return http;
}

/*
 * Define some basic config files
 * These have some extra spaces so we can make modifications in place.
 */
char config_f1 [] =
    "{ \"Forwarder\": {\"fred\":    {\n"
    "    \"Source\": \"lisa\",     \"Destination\": \"dest\",    \"Enabled\": true,  \n"
    "    \"Topic\": [ \"topica\", \"topicb\", \"topicc\" ],\n"
    "    \"TopicMap\": \"xyz/${Topic2*}\",\n"
    "    \"Selector\": \"QoS > 0                  \",\n"
    "    \"SourceQoS\": 2   \n"
    "} } }";
char config_f2 [] =
    "{ \"Forwarder\": {\"evf1\":    {\n"
    "    \"Source\": \"lisa\",     \"Destination\": \"evst\",    \"Enabled\": true,  \n"
    "    \"Topic\": [ \"topica\", \"topicb\", \"topicc\" ],\n"
    "    \"KeyMap\": \"${JSON:event:Topic2},${JSON:time:TimeIso}\",\n"
    "    \"Selector\": \"QoS > 0                  \",\n"
    "    \"SourceQoS\": 2,    \n"
    "    \"KafkaAPIVersion\": 2,    \n"
    "    \"PartitionRule\": \"instance\",        \n"
    "    \"RoutingRule\": {\n"
    "       \"topic1\": \"Topic1 = 'event'\",   \n"
    "       \"topic2\": \"QoS > 0\",   \n"
    "} } } }";
char config_c1 [] =
    "{ \"Connection\": {\"lisa\":    {\n"
    "    \"MQTTServerList\": [ \"abc\", \"def\", \"ghi\" ],\n"
    "    \"ClientID\": \"thisclientid\",     \n"
    "    \"TLS\": \"None\"   ,   \n"
    "    \"Username\": \"tom\"   ,   \n"
    "    \"Password\": \"tompassword\"   ,   \n"
    "    \"SessionExpiry\": 0,     \n"
    "    \"Version\": \"5.0\"   \n"
    "} } }";
char config_c2 [] =
    "{ \"Connection\": {\"evst\":    {\n"
    "    \"EventStreamsBrokerList\": [ \"abc\", \"def\", \"ghi\" ],\n"
    "    \"Username\": \"tom\"   ,   \n"
    "    \"Password\": \"tompassword\"   ,   \n"
    "    \"TLS\": \"None\"      \n"
    "} } }";

char config_e1 [] =
    "{ \"Endpoint\": { \"admin1\": {\n"
    "    \"Port\": 9993, \n"
    "    \"Protocol\": \"Admin\", \n"
    "    \"Interface\": \"*\",              \n"
    "    \"Enabled\": true,  \n"
    "    \"Authentication\": \"basic\"   ,\n"
    "    \"Certificate\": \"mycert.pem\",   \n"
    "    \"Key\": \"mykey.pmd\",    \n"
    "    \"KeyPassword\": \"mykey_password\",   \n"
    "    \"Method\": \"TLSv1.2\",    \n"
    "    \"Secure\": true ,    \n"
    "    \"UseClientCipher\": false   \n"
    "} } }\n";

char config_u1 [] =
    "{ \"User\": {\"tom\":    { \"Password\": \"password\"    } } }";
char config_u2 [] =
    "{ \"User\": {\"many\":    {   } } }";

/*
 * Test REST GET methods.
 *
 * This uses a facility to not actually respond to the HTTP object.
 * This only works if the REST implementation is synchronous.
 *
 * The path used for this test does not include the /admin which is used for routing the HTTP
 * request to the admin REST.
 */

void testRestGET(void) {
    ism_transport_t xtransport;
    ism_protobj_t xpobj;
    ism_transport_t * transport = &xtransport;
    ism_http_t * http;

    /* Create a transport object */
    setupTransport(transport, "admin", &xpobj, __LINE__);

    http = callRest(transport, "GET", "/info", NULL, 200, 0, __LINE__);
    CU_ASSERT(strstr(http->outbuf.buf, "\"Container\": \"bridge_test\",") != NULL);
    // printf("content='%s'\n", http->outbuf.buf);

    http = callRest(transport, "GET", "/info/container", NULL, 200, 0, __LINE__);
    CU_ASSERT(strstr(http->outbuf.buf, "bridge_test") != NULL);

    http = callRest(transport, "GET", "/info/Container", NULL, 200, 0, __LINE__);
    CU_ASSERT(strstr(http->outbuf.buf, "bridge_test") != NULL);

    /* TODO: Lots more */
}

/*
 * Test good REST POST methods
 */
void testRestPOST(void) {
    ism_transport_t xtransport;
    ism_protobj_t xpobj;
    ism_transport_t * transport = &xtransport;
    ism_http_t * http;
    char content [2048];

    /* Create a transport object */
    setupTransport(transport, "admin", &xpobj, __LINE__);

    /* Make several objects */
    strcpy(content, config_f1);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make forwarder fred */
    CU_ASSERT(changeStr(content, "fred", "nora") == 0);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make forwarder nora */

    strcpy(content, config_f2);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make forwarder evf1 */
    CU_ASSERT(changeStr(content, "evf1", "evf2") == 0);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make forwarder evf2 */

    strcpy(content, config_c1);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make connection lisa */
    CU_ASSERT(changeStr(content, "lisa", "tony") == 0);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make connection tony */

    strcpy(content, config_c2);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make connection evst */
    CU_ASSERT(changeStr(content, "evst", "evs2") == 0);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make connection evs2 */

    strcpy(content, config_e1);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make endpoint admin1 */
    CU_ASSERT(changeStr(content, "admin1", "admin2") == 0);
    CU_ASSERT(changeStr(content, "9993", "9994") == 0);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make endpoint admin2 */
    CU_ASSERT(http->val1 == 200);

    strcpy(content, config_u1);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make user tom */
    CU_ASSERT(changeStr(content, "tom", "sam") == 0);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make user sam */

    /* TODO: Lots more */

    ism_http_free(http);
}
/*
 * Test REST POST forwarder errors
 */
void testRestPOSTunknown(void) {
    ism_transport_t xtransport;
    ism_protobj_t xpobj;
    ism_transport_t * transport = &xtransport;
    ism_http_t * http = NULL;
    xUNUSED char content [2048];
    setupTransport(transport, "admin", &xpobj, __LINE__);
    /* TODO */

    strcpy(content, config_c1);
    CU_ASSERT(changeStr(content, "Connection", "CoNNetcion") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 948, __LINE__);
    strcpy(content, config_c1);
    CU_ASSERT(changeStr(content, "MQTTServerList", "MQTTBrokerList") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 939, __LINE__);
    strcpy(content, config_f1);
    CU_ASSERT(changeStr(content, "Source", "SOURCE") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 938, __LINE__);
    strcpy(content, config_e1);
    CU_ASSERT(changeStr(content, "Interface", "Interafce") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 925, __LINE__);
    strcpy(content, config_u1);
    CU_ASSERT(changeStr(content, "Password", "Pawwsord") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 937, __LINE__);

    if (http)
        ism_http_free(http);
}

/*
 * Test REST POST forwarder errors
 */
void testRestPOSTforwarder(void) {
    ism_transport_t xtransport;
    ism_protobj_t xpobj;
    ism_transport_t * transport = &xtransport;
    ism_http_t * http = NULL;
    xUNUSED char content [2048];
    setupTransport(transport, "admin", &xpobj, __LINE__);

    strcpy(content, config_f1);   /* JSON error */
    CU_ASSERT(changeStr(content, "  {", " {{{") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 917, __LINE__);

    strcpy(content, config_f1);  /* Source is not string */
    CU_ASSERT(changeStr(content, "\"lisa\",", "3,     ") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 952, __LINE__);

    strcpy(content, config_f1);  /* Dest is not string */
    CU_ASSERT(changeStr(content, "\"dest\",", "true  ") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 952, __LINE__);

    strcpy(content, config_f1);  /* Topic element is not string */
    CU_ASSERT(changeStr(content, "\"topicb\",", "12345,   ") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 952, __LINE__);

    strcpy(content, config_f1);  /* Bad selector */
    CU_ASSERT(changeStr(content, "QoS > 0", "((     ") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 952, __LINE__);

    strcpy(content, config_f1);  /* Bad selector */
    CU_ASSERT(changeStr(content, "2  ", "\"a\"") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 952, __LINE__);

    /* TODO: a few more including kafka properties */

    if (http)
        ism_http_free(http);
}

/*
 * Test REST POST forwarder errors
 */
void testRestPOSTconnection(void) {
    ism_transport_t xtransport;
    ism_protobj_t xpobj;
    ism_transport_t * transport = &xtransport;
    ism_http_t * http = NULL;
    xUNUSED char content [2048];
    setupTransport(transport, "admin", &xpobj, __LINE__);

    strcpy(content, config_c1);  /* Bad element in list */
    CU_ASSERT(changeStr(content, "\"def\"", "3,    ") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 953, __LINE__);

    strcpy(content, config_c1);  /* Bad type */
    CU_ASSERT(changeStr(content, "\"thisclientid\",", "true,          ") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 953, __LINE__);

    strcpy(content, config_c1);  /* Bad enumerated value */
    CU_ASSERT(changeStr(content, "None", "Fred") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 953, __LINE__);

    strcpy(content, config_c1);  /* Bad numeric value */
    CU_ASSERT(changeStr(content, " 0,", "-1,") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 953, __LINE__);

    strcpy(content, config_c1);  /* Bad enumerated value */
    CU_ASSERT(changeStr(content, "5.0", "6.0") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 953, __LINE__);

    /* TODO some more cases including kafka items */
    if (http)
        ism_http_free(http);
}

/*
 * Test REST POST forwarder errors
 */
void testRestPOSTendpoint(void) {
    ism_transport_t xtransport;
    ism_protobj_t xpobj;
    ism_transport_t * transport = &xtransport;
    ism_http_t * http = NULL;
    xUNUSED char content [2048];
    setupTransport(transport, "admin", &xpobj, __LINE__);


    strcpy(content, config_e1);  /* Bad port value */
    CU_ASSERT(changeStr(content, "9993", "\"ab\"") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 955, __LINE__);

    strcpy(content, config_e1);  /* Bad enumerated value */
    CU_ASSERT(changeStr(content, "Admin\"", "MQTT\" ") == 0);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);

    strcpy(content, config_e1);  /* Bad string value */
    CU_ASSERT(changeStr(content, "\"*\"", "666") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 955, __LINE__);

    strcpy(content, config_e1);  /* Bad enumerated value */
    CU_ASSERT(changeStr(content, "Admin", "JMSXX") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 955, __LINE__);

    strcpy(content, config_e1);  /* Bad enumerated value */
    CU_ASSERT(changeStr(content, "true,", "3,   ") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 955, __LINE__);


    /* TODO try a few more */
    if (http)
        ism_http_free(http);
}

/*
 * Test REST POST forwarder errors
 */
void testRestPOSTuser(void) {
    ism_transport_t xtransport;
    ism_protobj_t xpobj;
    ism_transport_t * transport = &xtransport;
    ism_http_t * http = NULL;
    xUNUSED char content [2048];
    setupTransport(transport, "admin", &xpobj, __LINE__);


    strcpy(content, config_u1);
    CU_ASSERT(changeStr(content, "\"password\"", "3         ") == 0);
    http = callRest(transport, "POST", "/config", content, 400, 954, __LINE__);

    strcpy(content, config_u2);
    http = callRest(transport, "POST", "/config", content, 400, 954, __LINE__);

    /* TODO */
    if (http)
        ism_http_free(http);
}

/*
 * Test REST DELETE methods
 */
void testRestDELETE(void) {
    ism_transport_t xtransport;
    ism_protobj_t xpobj;
    ism_transport_t * transport = &xtransport;
    xUNUSED ism_http_t * http;
    char content [2048];

    /* Create a transport object */
    setupTransport(transport, "admin", &xpobj, __LINE__);

    /* Make several objects */
    strcpy(content, config_f1);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make forwarder fred */
    CU_ASSERT(changeStr(content, "fred", "nora") == 0);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make forwarder nora */

    strcpy(content, config_c1);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make connection lisa */
    CU_ASSERT(changeStr(content, "lisa", "tony") == 0);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make connection tony */

    strcpy(content, config_e1);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make endpoint admin1 */
    CU_ASSERT(changeStr(content, "admin1", "admin2") == 0);
    CU_ASSERT(changeStr(content, "9993", "9994") == 0);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make endpoint admin2 */

    strcpy(content, config_u1);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make user tom */
    CU_ASSERT(changeStr(content, "tom", "sam") == 0);
    http = callRest(transport, "POST", "/config", content, 200, 973, __LINE__);   /* make user sam */

    /*
     * Delete a forwarder
     */
    http = callRest(transport, "GET", "/config/forwarder/fred", NULL, 200, 0, __LINE__);
    http = callRest(transport, "DELETE", "/config/forwarder/fred", NULL, 200, 973, __LINE__);
    http = callRest(transport, "GET", "/config/forwarder/fred", NULL, 404, 0, __LINE__);
    http = callRest(transport, "DELETE", "/config/forwarder/fred", NULL, 404, 0, __LINE__);

    /*
     * Delete a connection
     */
    http = callRest(transport, "GET", "/config/connection/tony", NULL, 200, 0, __LINE__);
    http = callRest(transport, "DELETE", "/config/connection/tony", NULL, 200, 973, __LINE__);
    http = callRest(transport, "GET", "/config/connection/tony", NULL, 404, 0, __LINE__);
    http = callRest(transport, "DELETE", "/config/configuration/tony", NULL, 404, 0, __LINE__);

    /*
     * Delete an endpoint
     */
    http = callRest(transport, "GET", "/config/endpoint/admin1", NULL, 200, 0, __LINE__);
    http = callRest(transport, "DELETE", "/config/endpoint/admin1", NULL, 200, 973, __LINE__);
    http = callRest(transport, "GET", "/config/endpoint/admin1", NULL, 404, 0, __LINE__);
    http = callRest(transport, "DELETE", "/config/endpoint/admin1", NULL, 404, 0, __LINE__);

    /*
     * Delete a user
     */
    http = callRest(transport, "GET", "/config/user/tom", NULL, 200, 0, __LINE__);
    http = callRest(transport, "DELETE", "/config/user/tom", NULL, 200, 973, __LINE__);
    http = callRest(transport, "GET", "/config/user/tom", NULL, 404, 0, __LINE__);
    http = callRest(transport, "DELETE", "/config/user/tom", NULL, 404, 0, __LINE__);
    // http->outbuf.buf[http->outbuf.used] = 0;
    // printf("line=%d content='%s'\n", __LINE__, http->outbuf.buf);
    /* Probably done enough */
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
    g_rc = CU_pRunSummary_Final->nTestsFailed + CU_pRunSummary_Final->nAssertsFailed;
}

/*
 * Main entry point
 * Change to run in the test directory if there is one
 */
int main(int argc, char * * argv) {
    int trclvl = 3;
    char cwd[512];
    char newdir[512];

    ism_common_initUtil();
    printf("cwd=%s\n", getcwd(cwd, sizeof cwd));
    strcpy(newdir, cwd);
    strcat(newdir, "/test/cunit");
    if( 0 != chdir(newdir) ) {
        printf("chdir %s failed",newdir);
    }
    printf("newcwd=%s\n", getcwd(newdir, sizeof newdir));
    printf("%s %s\n", ism_common_getPlatformInfo(), ism_common_getKernelInfo());
    printf("%s\n\n", ism_common_getProcessorInfo());
    ism_common_setTraceLevel(trclvl);

    /* Run tests */
    Startup_CUnit(argc, argv);

    print_final_summary();
    Endup_CUnit();
    if( 0 != chdir(cwd) ) {
        printf("chdir %s failed",cwd);
    }
    return g_rc;
}
