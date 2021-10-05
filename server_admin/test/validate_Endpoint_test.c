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
 * Add CUNIT tests for generic configuration data validation
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval);

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int eptestno = 0;

/*
 * Fake callback for transport
 */
int epEPTransportCallback(
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
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING Endpoint Configuration Object   ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateAdminEndpoint() API */
static int32_t validate_Endpoint(
    char *mbuf,
    int32_t exprc)
{
    json_error_t error = {0};
    json_t *post = json_loads(mbuf, 0, &error);
    if ( !post ) {
        printf("post: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( post != NULL );

    /* Loop thru post json object, validate object, invoke callback and persist data */
    json_t *mobj = json_loads(mbuf, 0, &error);
    if ( !mobj ) {
        printf("mobj: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( mobj != NULL );

    const char *name = NULL;
    json_t *mergedObj = NULL;

    void *objiter = json_object_iter(mobj);
    while (objiter) {
        name = json_object_iter_key(objiter);
        mergedObj = json_object_iter_value(objiter);
        break;
    }

    CU_ASSERT( name != NULL );
    CU_ASSERT( mergedObj != NULL );

    ism_prop_t *props = ism_common_newProperties(ISM_VALIDATE_CONFIG_ENTRIES);

    /* no difference in CREATE and UPDATE */
    int rc = ism_config_validate_Endpoint(post, mergedObj, "Endpoint", (char *)name, CREATE, props);
    eptestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", eptestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}




/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_configObject_Endpoint(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("AdminEndpoint INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_TRANSPORT, NULL,
        (ism_config_callback_t) epEPTransportCallback, &handle);

    printf("Endpoint ism_config_register(): rc: %d\n", rc);

    /* Add config items required for Endpoint unit tests */
    char *ibuf = NULL;
    json_error_t error = {0};
    json_t *value = NULL;

    /* Add SecurityProfile - name: secprof */
    ibuf = "{ \"secprof\": { \"TLSEnabled\": false, \"UsePasswordAuthentication\": false }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("SecurityProfile", "secprof", value);
    CU_ASSERT( rc == ISMRC_OK );

    /* Add ConnectionPolicy - name: conpol */
    ibuf = "{ \"conpol\": { \"ClientID\": \"*\" }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("ConnectionPolicy", "messpol", value);
    CU_ASSERT( rc == ISMRC_OK );

    /* Add TopicPolicy - name: messpol */
    ibuf = "{ \"messpol\": { \"ClientID\": \"*\", \"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("TopicPolicy", "messpol", value);
    CU_ASSERT( rc == ISMRC_OK );

    /* Add MessageHub - name: msghub */
    ibuf = "{ \"msghub\": { \"Description\": \"hub\" }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("MessageHub", "msghub", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Add Endpoint - name: testmp */
    ibuf = "{ \"testep\": { \"Description\":\"Test Endpoint\", \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    value = json_loads(ibuf, 0, &error);
//    value = json_pack("{s{ssssssssss}}","testmp","Description","TestEndpoint","ClientID","*","DestinationType","Topic","Destination","*","ActionList","Publish,Subscribe");
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    printf("calling ism_config_json_setComposite\n");
    rc = ism_config_json_setComposite("Endpoint", "testmp", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Create Endpoint: rc=ISMRC_OK */
    mbuf = "{ \"Testep1\": { \"Description\":\"Test Endpoint 1\", \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Create Endpoint: rc=ISMRC_OK */
    mbuf = "{ \"Testep2\": { \"Description\":\"Test Endpoint 2\", \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 3. Update Endpoint - default description i.e. empty string : rc=ISMRC_OK */
    mbuf = "{ \"Testep1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 4. Update Endpoint - name length 256 characters : rc=ISMRC_OK */
    mbuf = "{ \"a256_CharName111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999900000000001111111111222222222233333333334444444444123456\": { \"Description\":\"a256_CharName\", \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 5. Update Endpoint - name length 257 characters : rc=ISMRC_NameLimitExceed */
    mbuf = "{ \"a257_CharName1111111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000111111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444441234567\": { \"Description\":\"a257_CharName\", \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_NameLimitExceed) == ISMRC_NameLimitExceed );

    /* 6. Update Endpoint - invalid - missing Port : rc=ISMRC_PropertyRequired */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    /* 7. Update Endpoint - Bad property name : rc=ISMRC_BadPropertyName */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Portie\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_BadAdminPropName) == ISMRC_BadAdminPropName );

    /* 8. Update Endpoint - Bad property type, string for Port : rc=ISMRC_BadPropertyType */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":\"1234\", \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    /* 9. Update Endpoint - Invalid protocol specified : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Protocol\":\"unknown\", \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 10. Update Endpoint - Bad property type, string for Enabled : rc=ISMRC_BadPropertyType */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":\"false\", \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    /* 11. Update Endpoint - Bad property, unknown ConnectionPolicy : rc=ISMRC_BadPISMRC_ObjectNotFoundropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol2\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_ObjectNotFound) == ISMRC_ObjectNotFound );

    /* 12. Update Endpoint - Bad property, unknown TopicPolicy : rc=ISMRC_ObjectNotFound */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol2\", \"MessageHub\":\"msghub\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_ObjectNotFound) == ISMRC_ObjectNotFound );

    /* 13. Update Endpoint - Bad property, unknown MessageHub : rc=ISMRC_InvalidMessageHub */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub2\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_InvalidMessageHub) == ISMRC_InvalidMessageHub );

    /* 14. Update Endpoint - Bad property, unknown Protocol : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"Protocol\":\"Anything\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 15. Update Endpoint - Bad property, unknown Interface : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"Interface\":\"Anything\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 16 Update Endpoint - Known Interface : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"Interface\":\"127.0.0.1\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 17 Update Endpoint - MaxMessageSize=1024 : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"MaxMessageSize\":\"1024\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 18 Update Endpoint - MaxMessageSize=1024KB : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"MaxMessageSize\":\"1024KB\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 19 Update Endpoint - MaxMessageSize=100MB : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"MaxMessageSize\":\"100MB\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 20 Update Endpoint - MaxMessageSize=1023 : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"MaxMessageSize\":\"1023\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 21 Update Endpoint - MaxMessageSize=1000MB : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"MaxMessageSize\":\"1000MB\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 22 Update Endpoint - MaxSendSize=128 : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"MaxSendSize\":128 }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 23 Update Endpoint - MaxSendSize=16384 : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"MaxSendSize\":16384 }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 24 Update Endpoint - MaxSendSize=127 : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"MaxSendSize\":127 }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 25 Update Endpoint - MaxSendSize=65537 : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"MaxSendSize\":65537 }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 26 Update Endpoint - BatchMessages=true : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"BatchMessages\":true }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 27 Update Endpoint - BatchMessages="true" : rc=ISMRC_BadPropertyType */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Enabled\":false, \"Port\":1234, \"ConnectionPolicies\":\"conpol\", \"TopicPolicies\":\"messpol\", \"MessageHub\":\"msghub\", \"BatchMessages\":\"true\" }}";
    CU_ASSERT( validate_Endpoint(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    return;
}

