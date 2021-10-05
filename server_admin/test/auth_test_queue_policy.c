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

int noqpoltestnum = 0;

static void test_queuePolicy_auth(char *poln, char *cid, char *uid, char *proto, ismSecurityAuthActionType_t actionType, char *queue, int exprc) {
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
    listener->qpolicies = poln;

    tport->listener = listener;

    noqpoltestnum += 1;
    rc = ism_security_validate_policy(secContext, ismSEC_AUTH_QUEUE, queue, actionType, ISM_CONFIG_COMP_SECURITY, NULL);
    printf("QueuePolicyTest-%d: RC:%d  ExpectedRC:%d\n", noqpoltestnum, rc, exprc);
    CU_ASSERT(rc == exprc);

    rc = ism_security_destroy_context(secContext);
    CU_ASSERT(rc == 0);

}

/* Cunit test for QueuePolicy authorization */
void test_authorize_queue_policy(void)
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

    /* Add/Update Queue Policy */
    ibuf = "{ \"QueuePolicy\": { \"testQueuePol1\": { \"Protocol\":\"JMS\", \"Queue\":\"*\", \"ActionList\":\"Send,Receive\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("TestQueuePol: create policy: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );

    ibuf = "{ \"QueuePolicy\": { \"testQueuePol2\": { \"Protocol\":\"JMS\", \"Queue\":\"Test*\", \"ActionList\":\"Send,Receive\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("TestQueuePol: create policy: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );

    ibuf = "{ \"QueuePolicy\": { \"testQueuePol3\": { \"Protocol\":\"JMS\", \"Queue\":\"Test/${UserID}/*\", \"ActionList\":\"Send,Receive,Browse\" }}}";
    rc = set_configFromJSONStr(ibuf);
    printf("TestQueuePol: create policy: rc=%d\n", rc);
    CU_ASSERT( rc == ISMRC_OK );

    test_queuePolicy_auth("testQueuePol1", "ClientA", "UserA", "JMS", ismSEC_AUTH_ACTION_SEND, "Test", ISMRC_OK);
    test_queuePolicy_auth("testQueuePol1", "ClientA", "UserA", "JMS", ismSEC_AUTH_ACTION_RECEIVE, "Test", ISMRC_OK);
    test_queuePolicy_auth("testQueuePol1", "ClientA", "UserA", "JMS", ismSEC_AUTH_ACTION_CONTROL, "Test", ISMRC_NotAuthorized);

    test_queuePolicy_auth("testQueuePol2", "ClientA", "UserA", "JMS", ismSEC_AUTH_ACTION_SEND, "Test", ISMRC_OK);
    test_queuePolicy_auth("testQueuePol2", "ClientA", "UserA", "JMS", ismSEC_AUTH_ACTION_RECEIVE, "Test/xyz", ISMRC_OK);
    test_queuePolicy_auth("testQueuePol2", "ClientA", "UserA", "JMS", ismSEC_AUTH_ACTION_PUBLISH, "XYZ", ISMRC_NotAuthorized);

    test_queuePolicy_auth("testQueuePol3", "ClientA", "UserA", "JMS", ismSEC_AUTH_ACTION_SEND, "Test/UserA/", ISMRC_OK);
    test_queuePolicy_auth("testQueuePol3", "ClientA", "UserA", "JMS", ismSEC_AUTH_ACTION_SEND, "Test/UserA/xyz", ISMRC_OK);
    test_queuePolicy_auth("testQueuePol3", "ClientA", "UserA", "JMS", ismSEC_AUTH_ACTION_SEND, "Test/xyz", ISMRC_NotAuthorized);
    test_queuePolicy_auth("testQueuePol3", "ClientA", "UserA", "JMS", ismSEC_AUTH_ACTION_BROWSE, "Test/UserA/xyz", ISMRC_OK);

    return;
}

