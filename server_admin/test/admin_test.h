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
 * File: admin_test.h
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


#include "config_test.c"
#include "security_test.c"
#include "ldaputiltest.c"
#include "threadpooltest.c"
#include "ltpautiltest.c"
#include "validateAndSet_test.c"
#include "oauth_utils_test.c"
#include "validate_genericData_test.c"
#include "validate_MessageHub_test.c"
#include "validate_ConnectionPolicy_test.c"
#include "validate_QueuePolicy_test.c"
#include "validate_SubscriptionPolicy_test.c"
#include "validate_TopicPolicy_test.c"
#include "validate_Endpoint_test.c"
#include "config_json_test.c"
#include "validate_AdminEndpoint_test.c"
#include "validate_ConfigurationPolicy_test.c"
#include "validate_SecurityProfile_test.c"
#include "validate_HighAvailability_test.c"
#include "validate_TopicMonitor_test.c"
#include "validate_QueueMgrConnection_test.c"
#include "validate_DestinationMappingRule_test.c"
#include "jansson_config_test.c"
#include "auth_test_topic_policy.c"
#include "auth_test_queue_policy.c"
#include "auth_test_subscription_policy.c"
#include "validate_Plugin_test.c"
#include "validate_LogLocation_test.c"
#include "validate_Syslog_test.c"
#include "test_getProperties.c"
#include "validate_ClusterMembership_test.c"
#include "validate_Queue_test.c"
#include "validate_LTPAProfile_test.c"
#include "validate_OAuthProfile_test.c"
#include "test_MQCObjectConfig.c"
#include "closeConnection_test.c"
#include "validate_LDAP_test.c"
#include "validate_CRLProfile_test.c"
#include "test_ImportExport.c"
#include "validate_AdminSubs_test.c"
#include "policy_test_with_group.c"



#define BASIC_TEST_MODE       0
#define FULL_TEST_MODE        1
#define BYNAME_TEST_MODE      2

extern int g_verbose;

int test_mode = BASIC_TEST_MODE;
int default_test_mode = BASIC_TEST_MODE;


int init_adminconfig(void);

CU_TestInfo ISM_ImportExport_CUnit_ConfigBasic[] = {
        {"ImportExportValidate",    test_validate_ImportExport  },
        {"ImportExportRESTSetup",   test_RESTSetupImportExport  },
        {"ImportExportRESTGet",     test_RESTGetImportExport    },
        {"ImportExportRESTDelete",  test_RESTDeleteImportExport },
        {"ImportExportRESTCleanup", test_RESTCleanupImportExport },
        CU_TEST_INFO_NULL
};
CU_TestInfo ISM_Config_CUnit_ConfigBasic[] = {
    {"ConfigInit", testconfigInit },
    {"ConfigSetGet", testconfigSetGet },
    {"ConfigRegistation", testconfigRegistration },
    {"ConfigGetComponentType", testConfigGetComponent },
    {"ConfigGetComponentProps", testgetComponentProps },
    {"ConfigUpdateProtocolCapabilities", testupdateProtocolCapabilities },
    {"ConfigTestGetProperties", test_getProperties },
    {"test_MQC_configObject", test_MQC_configObject },
//    {"ConfigJSONTest", testConfigSetJSONStr },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_Security_CUnit_ConfigBasic[] = {
    {"SecurityInit", testsecurityInit },
    {"SecurityCreatePolicy", testsecurityCreatePolicy },
    {"SecurityValidateConnectionPolicy", testsecurityValidateConnectionPolicy },
    {"test_ism_security_ltpaReadKeyfile", test_ism_security_ltpaReadKeyfile },
    {"test_authorize_topic_policy", test_authorize_topic_policy },
    {"test_authorize_queue_policy", test_authorize_queue_policy },
    {"test_validate_LTPAProfile", test_validate_configObject_LTPAProfile },
    {"test_validate_OAuthProfile", test_validate_configObject_OAuthProfile },
    {"test_authorize_subscription_policy", test_authorize_subscription_policy },
    {"test_validate_configObject_LDAP", test_validate_configObject_LDAP },
    {"test_authorize_destWithGroup", test_authorize_destWithGroup },
    {"test_encrypt_decrypt_passwd", testAdminPasswdEncryptDecrypt },
    {"test_hash_passwd", testAdminPasswdHash },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_LDAPUtil_CUnit_ConfigBasic[] = {
    {"LDAPUtilInit", testLDAPUtilInit },
    {"LDAPExtraLenTest", test_ism_admin_ldapExtraLen },
    {"LDAPExtra", test_ism_admin_ldapExtra },
    {"LDAPExtraFilterOption", test_ism_admin_ldapExtraFilterOption },
    {"LDAPGetPrefix", test_ism_security_getLDAPIdPrefix },
    {"LDAPHexExtraLenTest", test_ism_admin_ldapHexExtraLen },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_ThreadPool_CUnit_ConfigBasic[] = {
    {"InitThreadPoolTest", test_ism_security_initThreadPool },
    {"StartThreadPoolTest", test_ism_security_startThreadPool },
    {"GetWorkerThreadPoolTest", test_ism_security_getWorker },
    
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_LTPA_CUnit_ConfigBasic[] = {
    {"InitLTPATest", testLTPAUtilInit },
    {"LTPAReadKeyFileTest", test_ism_security_ltpaReadKeyfile },
    {"LTPADecodeV2TokenTest", test_ism_security_ltpaV2DecodeToken },
    
    
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_ValidateAndSet_CUnit_ConfigBasic[] = {
    {"testValidateAndSetInit", testValidateAndSetInit },
    {"test_ism_config_validateClientAddress", test_ism_config_validateClientAddress },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_OAuth_validate[] = {
    {"oauth_utils_test_setup", oauth_utils_test_setup },
    {"test_oauth_validate_completeToken", test_oauth_validate_completeToken },
    {"test_oauth_validate_missingToken", test_oauth_validate_missingToken },
    {"test_oauth_validate_zeroLenToken", test_oauth_validate_zeroLenToken },
    {"test_oauth_validate_missingExpiry", test_oauth_validate_missingExpiry },
    {"test_oauth_userInfoURL_equal", test_oauth_userInfoURL_equal },
    {"test_oauth_userInfoURL_notEqual", test_oauth_userInfoURL_notEqual },
    {"test_oauth_userInfoURL_pollutedBody", test_oauth_userInfoURL_pollutedBody },
    {"test_oauth_long_token", test_oauth_long_token },
    {"oauth_utils_test_teardown", oauth_utils_test_teardown },
    CU_TEST_INFO_NULL
};


/* Tests to validate generic configuration data */
CU_TestInfo ISM_validate_CUnit_genericData[] = {
        {"test_validate_genericData_setup",   test_validate_genericData_setup },
        {"test_validate_genericData_number",  test_validate_genericData_number },
        {"test_validate_genericData_enum",    test_validate_genericData_enum },
        {"test_validate_genericData_boolean",    test_validate_genericData_boolean },
        {"test_validate_genericData_string", test_validate_genericData_string },
        {"test_validate_genericData_name",    test_validate_genericData_name },
        {"test_validate_genericData_IPAddress",    test_validate_genericData_IPAddress },
        {"test_validate_genericData_URL",    test_validate_genericData_URL },
        {"test_validate_genericData_Selector",  test_validate_genericData_Selector },
        {"test_validate_genericData_cleanup", test_validate_genericData_cleanup },
        CU_TEST_INFO_NULL
};

/* Tests to validate generic configuration data */
CU_TestInfo ISM_validate_CUnit_configObject[] = {
        {"test_validate_configObject_AdminEndpoint",  test_validate_configObject_AdminEndpoint },
        {"test_validate_configObject_MessageHub",  test_validate_configObject_MessageHub },
        {"test_validate_configObject_ConnectionPolicy",  test_validate_configObject_ConnectionPolicy },
        {"test_validate_configObject_QueuePolicy",  test_validate_configObject_QueuePolicy },
        {"test_validate_configObject_SubscriptionPolicy",  test_validate_configObject_SubscriptionPolicy },
        {"test_validate_configObject_TopicPolicy",  test_validate_configObject_TopicPolicy },
        {"test_validate_configObject_Endpoint",  test_validate_configObject_Endpoint },
        {"test_validate_configObject_ConfigurationPolicy",  test_validate_configObject_ConfigurationPolicy },
        {"test_validate_configObject_SecurityProfile", test_validate_configObject_SecurityProfile },
        {"test_validate_configObject_HighAvailability", test_validate_configObject_HighAvailability },
        {"test_validate_configObject_TopicMonitor", test_validate_configObject_TopicMonitor },
        {"test_validate_configObject_Plugin", test_validate_configObject_Plugin },
		{"test_validate_configObject_QueueManagerConnection", test_validate_configObject_QueueManagerConnection },
		{"test_validate_configObject_DestinationMappingRule", test_validate_configObject_DestinationMappingRule },
        {"test_validate_configObject_ClusterMembership", test_validate_configObject_ClusterMembership },
		{"test_validate_configObject_LogLocation", test_validate_configObject_LogLocation },
		{"test_validate_configObject_Syslog", test_validate_configObject_Syslog },
        {"test_validate_configObject_Queue", test_validate_configObject_Queue },
		{"test_validate_configObject_CRLProfile", test_validate_configObject_CRLProfile },
		{"test_validate_configObject_AdminSubscription", test_validate_configObject_AdminSubscription},
        CU_TEST_INFO_NULL
};

/* Tests to validate generic configuration data */
CU_TestInfo ISM_config_CUnit_JSONParse[] = {
        {"test_config_JSON_FileLoad",   test_config_JSON_FileLoad },
        CU_TEST_INFO_NULL
};

/* Tests for service domain options */
CU_TestInfo ISM_validate_CUnit_serviceDomainOptions[] = {
        {"test_closeConnection",   test_closeConnection },
        CU_TEST_INFO_NULL
};

/*
 * Array that carries the test suite and other functions to the CUnit framework
 */

CU_SuiteInfo ISM_AdminConfig_CUnit_basicsuites[] = {
	IMA_TEST_SUITE("GenericDataValidationTest", init_adminconfig, NULL, ISM_validate_CUnit_genericData),
    IMA_TEST_SUITE("ConfigBasicConfig", init_adminconfig, NULL, ISM_Config_CUnit_ConfigBasic),
    IMA_TEST_SUITE("ConfigObjectValidationTest", init_adminconfig, NULL, ISM_validate_CUnit_configObject),
    IMA_TEST_SUITE("SecurityBasicConfig", init_adminconfig, NULL, ISM_Security_CUnit_ConfigBasic),
    IMA_TEST_SUITE("LDAPUtilTest", init_adminconfig, NULL, ISM_LDAPUtil_CUnit_ConfigBasic),
    IMA_TEST_SUITE("ThreadPoolTest", init_adminconfig, NULL, ISM_ThreadPool_CUnit_ConfigBasic),
    IMA_TEST_SUITE("LTPATest", init_adminconfig, NULL, ISM_LTPA_CUnit_ConfigBasic),
    IMA_TEST_SUITE("OAuthValidate", init_adminconfig, NULL, ISM_OAuth_validate),
    IMA_TEST_SUITE("ValidateAndSetTest", init_adminconfig, NULL, ISM_ValidateAndSet_CUnit_ConfigBasic),
    IMA_TEST_SUITE("ConfigBasicJSONConfig", NULL, NULL, ISM_config_CUnit_JSONParse),
    IMA_TEST_SUITE("ServiceDomainOptions", NULL, NULL, ISM_validate_CUnit_serviceDomainOptions),
    IMA_TEST_SUITE("ImportExportTests", init_adminconfig, NULL, ISM_ImportExport_CUnit_ConfigBasic),
    CU_SUITE_INFO_NULL,
};


/*
 * Array that carries the test suite and other functions to the CUnit framework
 */

CU_SuiteInfo ISM_AdminConfig_CUnit_allsuites[] = {
    IMA_TEST_SUITE("GenericDataValidationTest", init_adminconfig, NULL, ISM_validate_CUnit_genericData),
    IMA_TEST_SUITE("ConfigObjectValidationTest", init_adminconfig, NULL, ISM_validate_CUnit_configObject),
    IMA_TEST_SUITE("ConfigBasicConfig", init_adminconfig, NULL, ISM_Config_CUnit_ConfigBasic),
    IMA_TEST_SUITE("SecurityBasicConfig", init_adminconfig, NULL, ISM_Security_CUnit_ConfigBasic),
    IMA_TEST_SUITE("LDAPUtilTest", init_adminconfig, NULL, ISM_LDAPUtil_CUnit_ConfigBasic),
    IMA_TEST_SUITE("ThreadPoolTest", init_adminconfig, NULL, ISM_ThreadPool_CUnit_ConfigBasic),
    IMA_TEST_SUITE("LTPATest", init_adminconfig, NULL, ISM_LTPA_CUnit_ConfigBasic),
    IMA_TEST_SUITE("OAuthValidate", init_adminconfig, NULL, ISM_OAuth_validate),
    IMA_TEST_SUITE("ValidateAndSetTest", init_adminconfig, NULL, ISM_ValidateAndSet_CUnit_ConfigBasic),
    IMA_TEST_SUITE("ConfigBasicJSONConfig", NULL, NULL, ISM_config_CUnit_JSONParse),
    CU_SUITE_INFO_NULL,
};

