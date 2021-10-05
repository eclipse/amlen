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
 * File: config_test.c
 * Component: server
 * SubComponent: server_admin
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <admin.h>
#include <ismjson.h>
#include <security.h>
#include <transport.h>

extern char * ism_security_encryptAdminUserPasswd(char * src);
extern char * ism_security_decryptAdminUserPasswd(char * src);

security_stat_t *statCount = NULL;

void ism_engine_threadInit(uint8_t isStoreCrit)
{
    return;
}

/*
 * init
 */
void testsecurityInit(void) {
    ism_config_t *hdl;

    ism_common_initUtil();

    int rc  = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("\nism_security_init: rc=%d\n", rc);
    CU_ASSERT(rc == 0 || rc == 113);

    hdl=ism_config_getHandle(ISM_CONFIG_COMP_SECURITY, NULL);
    CU_ASSERT_PTR_NOT_NULL(hdl);

}

void testsecurityCreatePolicy(void) {
    int rc;

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
       printf( "\nCWD: %s\n", cwd);
   
    rc = ism_security_addPolicy("test/testPolicy", NULL);
    printf("testsecurityCreatePolicy: Return RC: %d\n", rc);
    // CU_ASSERT(rc == 0);

}

void testsecurityValidateConnectionPolicy(void) {
    int rc;
    ism_domain_t  c_ism_defaultDomain = {"DOM", 0, "Default" };
    ism_trclevel_t * c_ism_defaultTrace = &c_ism_defaultDomain.trace;

    rc = ism_security_init();
    CU_ASSERT(rc == 0 || rc == 113);

    if ( !statCount )
        statCount = (security_stat_t *)calloc(1, sizeof(security_stat_t));

    ismSecurity_t *secContext = NULL;
    char trans[1024] = {0};
    ism_transport_t *tport = (ism_transport_t *)trans;

    tport->clientID = "ClientA";
    tport->userid = "UserA";
    tport->cert_name = "CertA";
    tport->client_addr = "127.0.0.1";
    tport->protocol = "JMS,MQTT";
    tport->listener = NULL;
    tport->trclevel = c_ism_defaultTrace;

    rc = ism_security_create_context(ismSEC_POLICY_CONNECTION, tport, &secContext);
    printf("\nconPolCreateContext: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);

    char listenerx[2048] = {0};
    ism_endpoint_t *listener = (ism_endpoint_t *)listenerx;
    listener->name = "EndpointA";
    listener->topicpolicies = "testTopicPolicy";
    listener->conpolicies = "testConnectionPolicy";

    tport->listener = listener;

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
       printf( "CWD: %s\n", cwd);
   
    rc = ism_security_addPolicy("test/testConnectionPolicy", NULL);
    printf("conPolAddPolicy: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);

    rc = ism_security_validate_policy(secContext, ismSEC_AUTH_USER, NULL, ismSEC_AUTH_ACTION_CONNECT, ISM_CONFIG_COMP_SECURITY, NULL);
    printf("conPolTest1: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);

    rc = ism_security_destroy_context(secContext);
    printf("conPolTest2: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);
}

/* Admin Password encrypt/decrypt test */
void testAdminPasswdEncryptDecrypt(void) {
    int rc;
    char * adminPassword = "testPassw0rd";
    
    /* encrypt password */
    printf("\nPlaintext=%s\n", adminPassword?adminPassword:"NULL");
    char *encryptedPasswd = ism_security_encryptAdminUserPasswd(adminPassword);
    printf("Encrypted=%s\n", encryptedPasswd?encryptedPasswd:"NULL");
    CU_ASSERT(encryptedPasswd != NULL);
    
    /* decrypt password */
    char *decryptedPasswd = ism_security_decryptAdminUserPasswd(encryptedPasswd);
    printf("Decrypted=%s\n", decryptedPasswd?decryptedPasswd:"NULL");
    CU_ASSERT(decryptedPasswd != NULL);
        
    /* compare password */
    rc = strcmp(decryptedPasswd, adminPassword);
    CU_ASSERT(rc == 0);
}


