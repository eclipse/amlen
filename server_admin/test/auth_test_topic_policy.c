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

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

extern int set_configFromJSONStr(const char *JSONStr);

ism_domain_t  ismDefaultDomain = {"DOM", 0, "Default" };
ism_trclevel_t * ismDefaultTrace;
int notpoltestnum = 0;

static void test_topicPolicy_auth(char *poln, char *cid, char *uid, char *proto, ismSecurityAuthActionType_t actionType, char *topic, int exprc) {
    int rc = ISMRC_OK;

    ismSecurity_t *secContext = NULL;
    char trans[1024] = {0};
    ism_transport_t *tport = (ism_transport_t *)trans;

    tport->clientID = cid;
    tport->userid = uid;
    tport->cert_name = "CertA";
    tport->client_addr = "127.0.0.1";
    tport->protocol = proto;
    tport->protocol_family = proto;
    tport->listener = NULL;
    tport->trclevel = ism_defaultTrace;

    rc = ism_security_create_context(ismSEC_POLICY_CONNECTION, tport, &secContext);
    CU_ASSERT(rc == 0);

    char listenerx[2048] = {0};
    ism_endpoint_t *listener = (ism_endpoint_t *)listenerx;
    listener->name = "EndpointA";
    listener->topicpolicies = poln;

    tport->listener = listener;

    notpoltestnum += 1;
    rc = ism_security_validate_policy(secContext, ismSEC_AUTH_TOPIC, topic, actionType, ISM_CONFIG_COMP_SECURITY, NULL);
    printf("TopicPolicyTest-%d: RC:%d  ExpectedRC:%d\n", notpoltestnum, rc, exprc);
    CU_ASSERT(rc == exprc);

    rc = ism_security_destroy_context(secContext);
    CU_ASSERT(rc == 0);

}

/* Cunit test for TopicPolicy authorization */
void test_authorize_topic_policy(void)
{
    int rc = ISMRC_OK;
    char *ibuf = NULL;

    ism_common_initUtil();
    ism_common_setTraceLevel(0);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("Init admin: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    ismDefaultTrace = &ismDefaultDomain.trace;

    rc = ism_security_init();
    CU_ASSERT(rc == 0 || rc == 113);

    if ( !statCount )
        statCount = (security_stat_t *)alloca(sizeof(security_stat_t));

    /* Add/Update Topic Policy */
    ibuf = "{ \"TopicPolicy\": { \"testTopicPol1\": { \"Protocol\":\"MQTT,JMS\", \"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("TestTopicPol: create policy: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );

    ibuf = "{ \"TopicPolicy\": { \"testTopicPol2\": { \"Protocol\":\"MQTT,JMS\", \"Topic\":\"Test*\", \"ActionList\":\"Publish,Subscribe\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("TestTopicPol: create policy: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );

    ibuf = "{ \"TopicPolicy\": { \"testTopic Pol3\": { \"Protocol\":\"MQTT,JMS\", \"Topic\":\"Test/${UserID}/*\", \"ActionList\":\"Publish,Subscribe\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("TestTopicPol: create policy: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );

    test_topicPolicy_auth("testTopicPol1", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "Test", ISMRC_OK);
    test_topicPolicy_auth("testTopicPol1", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_SUBSCRIBE, "Test", ISMRC_OK);
    test_topicPolicy_auth("testTopicPol1", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_CONTROL, "Test", ISMRC_NotAuthorized);

    test_topicPolicy_auth("testTopicPol2", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "Test", ISMRC_OK);
    test_topicPolicy_auth("testTopicPol2", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_SUBSCRIBE, "Test/xyz", ISMRC_OK);
    test_topicPolicy_auth("testTopicPol2", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "XYZ", ISMRC_NotAuthorized);

    test_topicPolicy_auth("  testTopic Pol3  ", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "Test/UserA/", ISMRC_OK);
    test_topicPolicy_auth("testTopic Pol3,  ", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "Test/UserA/xyz", ISMRC_OK);
    test_topicPolicy_auth(",,,,testTopic Pol3", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "Test/xyz", ISMRC_NotAuthorized);

    /* UID Tests */
    /* Add/Update Topic Policy */
    ibuf = "{ \"TopicPolicy\": { \"testTopicPol4\": { \"Protocol\":\"MQTT,JMS\",\"UserID\":\"TestUser\",\"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("TestTopicPol: create policy: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );
    test_topicPolicy_auth("testTopicPol4", "ClientA", "TestUser", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "Test", ISMRC_OK);
    test_topicPolicy_auth("testTopicPol4", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "Test", ISMRC_NotAuthorized);

    /* ClientAddress Tests */
    /* Add/Update Topic Policy */
    ibuf = "{ \"TopicPolicy\": { \"testTopicPol5\": { \"Protocol\":\"MQTT,JMS\",\"ClientAddress\":\"198.167.23.65\",\"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("TestTopicPol: create policy: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );
    test_topicPolicy_auth("testTopicPol5", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "Test", ISMRC_NotAuthorized);
    ibuf = "{ \"TopicPolicy\": { \"testTopicPol5\": { \"Protocol\":\"MQTT,JMS\",\"ClientAddress\":\"127.0.0.1\",\"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("TestTopicPol: create policy: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );
    test_topicPolicy_auth("testTopicPol5", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "Test", ISMRC_OK);

    /* CommonName Tests */
    /* Add/Update Topic Policy */
    ibuf = "{ \"TopicPolicy\": { \"testTopicPol6\": { \"Protocol\":\"MQTT,JMS\",\"CommonNames\":\"CertA\",\"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("TestTopicPol: create policy: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );
    test_topicPolicy_auth("testTopicPol6", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "Test", ISMRC_OK);
    ibuf = "{ \"TopicPolicy\": { \"testTopicPol6\": { \"Protocol\":\"MQTT,JMS\",\"CommonNames\":\"CertB\",\"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("TestTopicPol: create policy: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );
    test_topicPolicy_auth("testTopicPol6", "ClientA", "UserA", "JMS,MQTT", ismSEC_AUTH_ACTION_PUBLISH, "Test", ISMRC_NotAuthorized);

    return;
}

