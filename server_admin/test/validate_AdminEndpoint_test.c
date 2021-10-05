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

/*
 * Add CUNIT tests for AdminEndpoint configuration data validation
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval);

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int admtestno = 0;

/*
 * Fake callback for transport
 */
int adminEPTransportCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("Transport call is invoked\n");
    return 0;
}

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING AdminEndpoint Configuration Object   ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateAdminEndpoint() API */
static int32_t validate_AdminEndpoint(
    char *mbuf,
    int32_t exprc)
{
    json_error_t error = {0};
    const char *object = NULL;
    json_t *mergedObj = NULL;
    json_t *mobj = NULL;
    json_t *post = NULL;

    post = json_loads(mbuf, 0, &error);
    if ( post ) {
        mobj = json_loads(mbuf, 0, &error);
        if ( mobj ) {
            void *objiter = json_object_iter(mobj);
            while (objiter) {
                object = json_object_iter_key(objiter);
                mergedObj = json_object_iter_value(objiter);
                break;
            }
        }
    }

    ism_prop_t *props = ism_common_newProperties(ISM_VALIDATE_CONFIG_ENTRIES);

    /* no difference in CREATE and UPDATE */
    int rc = ism_config_validate_AdminEndpoint(post, mergedObj, (char *)object, "AdminEndpoint", CREATE, props);
    admtestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", admtestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}

/* Cunit test for AdminEndpoint configuration object validation */
void test_validate_configObject_AdminEndpoint(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    setenv("ISMSSN", "SSN4BVT", 1);
    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("AdminEndpoint INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_TRANSPORT, NULL,
        (ism_config_callback_t) adminEPTransportCallback, &handle);

    printf("AdminEndpoint ism_config_register(): rc: %d\n", rc);

    /* Add config items required for AdminEndpoint unit tests */
    char *ibuf = NULL;
    json_error_t error = {0};
    json_t *value = NULL;

    /* Add ConfigurationPolicy - name: colpol */
    ibuf = "{ \"cp1\": { \"ClientID\":\"*\", \"ActionList\":\"Browse\" }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("ConfigurationPolicy", "cp1", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Add SecurityProfile - name: secprof */
    ibuf = "{ \"secpol\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("SecurityProfile", "secprof", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Add AdminEndpoint - name: AdminEndpoint */
    ibuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"All\", \"ConfigurationPolicies\":\"cp1\", \"SecurityProfile\":\"secprof\" }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("AdminEndpoint", "AdminEndpoint", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    // Sample buffer:
    // mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"All\",\"SecurityProfile\":\"AdminDefaultSecProfile\",\"AdminDefaultConfPolicy\":\"%s\" }}";

    /* 1. Create: AdminEndpoint is created by default: rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"All\" }}";
    CU_ASSERT( validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Name is not settable: rc=ISMRC_NullPointer */
    mbuf = "{ \"TestEndpoint\": { \"Port\":9089,\"Interface\":\"All\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_NullPointer) == ISMRC_NullPointer);

    /* 3. Delete:  Not allowed: rc=ISMRC_DeleteNotAllowed */
    mbuf = "{ \"AdminEndpoint\": null }";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_DeleteNotAllowed) == ISMRC_DeleteNotAllowed);

    /* 4. Update: Change Interface - good value: rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"*\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 5. Update: Change Interface - good value: rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"127.0.0.1\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 6. Interface - good value: rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"10.10.2.1\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 7. Interface - bad value: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"[00:00:00:00:00:00]\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 8. Interface - good value: rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"[fe80::4af:1dff:fe92:2ada]\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 9. Interface - bad value: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"2.3.1\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 10. Interface - bad value: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"xxx\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 11. Change port: rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9099,\"Interface\":\"All\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 12. Change port - set to MIN : rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":2,\"Interface\":\"All\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 13. Change port - set to MAX : rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":65535,\"Interface\":\"All\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 14. Change port - bad value - incorrect type : rc=ISMRC_BadPropertyType */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":\"65535\", \"Interface\":\"All\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 15. Change port - bad value : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":1,\"Interface\":\"All\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 15A. Change port - bad value : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":-1,\"Interface\":\"All\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 16. Change port - bad value : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":165535,\"Interface\":\"All\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 17. set invalid configuration item - ConfigurationPolices: rc=ISMRC_BadAdminPropName */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"All\", \"ConfigurationPolices\":\"UNKNOWN\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_BadAdminPropName) == ISMRC_BadAdminPropName);

    /* 18. set UNKNOWN ConfigurationPolicies: rc=ISMRC_ObjectNotFound */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"All\", \"ConfigurationPolicies\":\"UNKNOWN\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_ObjectNotFound) == ISMRC_ObjectNotFound);

    /* 19. set only ConfigurationPolicies: rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"All\", \"ConfigurationPolicies\":\"cp1\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 20. set only SecurityProfile: rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"All\", \"SecurityProfile\":\"secprof\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 21. set ConfigurationPolicies and SecurityProfile: rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"All\", \"ConfigurationPolicies\":\"cp1\", \"SecurityProfile\":\"secprof\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 22. Invalid argument:  Not allowed: rc=ISMRC_PropertiesNotValid */
    mbuf = "{ \"AdminEndpoint\":\"\" }";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_PropertiesNotValid) == ISMRC_PropertiesNotValid);

    /* 23. Invalid argument:  Not allowed: rc=ISMRC_PropertiesNotValid */
    mbuf = "{ \"AdminEndpoint\":\"*\" }";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_PropertiesNotValid) == ISMRC_PropertiesNotValid);

    /* 24. Invalid argument:  Not allowed: rc=ISMRC_PropertiesNotValid */
    mbuf = "{ \"AdminEndpoint\":1234 }";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_PropertiesNotValid) == ISMRC_PropertiesNotValid);

    /* 25. set only ConfigurationPolicies: rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9089,\"Interface\":\"All\", \"ConfigurationPolicies\":\"cp1\", \"SecurityProfile\":\"\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 26. set 100 configuration policies, rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9099,\"Interface\":\"All\",\"SecurityProfile\":\"secprof\",\"ConfigurationPolicies\":\"cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 27. set 101 configuration policies, rc=ISMRC_TooManyItems */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9099,\"Interface\":\"All\",\"SecurityProfile\":\"secprof\",\"ConfigurationPolicies\":\"cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1,cp1\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_TooManyItems) == ISMRC_TooManyItems);

    /* 28. set ConfigurationPolicies and SecurityProfile to empty string, rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9099,\"Interface\":\"All\",\"SecurityProfile\":\"\",\"ConfigurationPolicies\":\"\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 29. set ConfigurationPolicies to null and SecurityProfile to empty, rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9099,\"Interface\":\"All\",\"SecurityProfile\":null,\"ConfigurationPolicies\":\"\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 30. set ConfigurationPolicies to empty and SecurityProfile to null, rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9099,\"Interface\":\"All\",\"SecurityProfile\":\"\",\"ConfigurationPolicies\":null }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 31. set ConfigurationPolicies to a valid value and SecurityProfile to empty, rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9099,\"Interface\":\"All\",\"SecurityProfile\":\"\",\"ConfigurationPolicies\":\"cp1\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 32. set ConfigurationPolicies to empty and SecurityProfile to a valid value, rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9099,\"Interface\":\"All\",\"SecurityProfile\":\"secprof\",\"ConfigurationPolicies\":\"\" }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 33. set ConfigurationPolicies to null value and SecurityProfile to null, rc=ISMRC_OK */
    mbuf = "{ \"AdminEndpoint\": { \"Port\":9099,\"Interface\":\"All\",\"SecurityProfile\":null,\"ConfigurationPolicies\":null }}";
    CU_ASSERT(validate_AdminEndpoint(mbuf, ISMRC_OK) == ISMRC_OK);

    return;
}

