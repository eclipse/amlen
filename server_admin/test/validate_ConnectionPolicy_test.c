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
 * Add CUNIT tests for ConnectionPolicy validation
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval);

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int cnpoltestno = 0;

/*
 * Fake callback for transport
 */
int cnpCallback(
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
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING ConnectionPolicy Configuration Object   ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateAdminEndpoint() API */
static int32_t validate_ConnectionPolicy(
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
    int rc = ism_config_validate_ConnectionPolicy(post, mergedObj, "ConnectionPolicy", (char *)name, CREATE, props);
    cnpoltestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", cnpoltestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}




/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_configObject_ConnectionPolicy(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("AdminEndpoint INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_SECURITY, NULL,
        (ism_config_callback_t) cnpCallback, &handle);

    printf("ConnectionPolicy ism_config_register(): rc: %d\n", rc);

    /* Add config items required for ConnectionPolicy unit tests */
    json_error_t error = {0};
    json_t *value = NULL;


    /* Add ConnectionPolicy - name: testmp */
    value = json_pack("{s{ssss}}","testmp","Description","TestConnectionPolicy","ClientID","*");
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    printf("calling ism_config_json_setComposite\n");
    rc = ism_config_json_setComposite("ConnectionPolicy", "testmp", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Create ConnectionPolicy: rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":\"Test ConnectionPolicy 1\", \"ClientID\":\"*\" }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Create ConnectionPolicy: rc=ISMRC_OK */
    mbuf = "{ \"TestMp2\": { \"Description\":\"Test ConnectionPolicy 2\", \"ClientID\":\"*\" }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 3. Update ConnectionPolicy - default description i.e. empty string : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"*\" }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 4. Update ConnectionPolicy - name length 256 characters : rc=ISMRC_OK */
    mbuf = "{ \"a256_CharName111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999900000000001111111111222222222233333333334444444444123456\": { \"Description\":\"a256_CharName\", \"ClientID\":\"*\" }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 5. Update ConnectionPolicy - name length 257 characters : rc=ISMRC_NameLimitExceed */
    mbuf = "{ \"a257_CharName1111111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000111111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444441234567\": { \"Description\":\"a257_CharName\", \"ClientID\":\"*\" }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_NameLimitExceed) == ISMRC_NameLimitExceed );

    /* 6. Update ConnectionPolicy - invalid - no MinOneOpt given : rc=ISMRC_MinOneOptMissing */
    mbuf = "{ \"TestMp1\": { \"Description\":null }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_MinOneOptMissing) == ISMRC_MinOneOptMissing );

    /* 7. Update ConnectionPolicy - ClientID bad value = "ab*cd* : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"ab*cd*\" }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 8. Update ConnectionPolicy - invalid - ClientID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"**\" }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 9. Update ConnectionPolicy - invalid - UserID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"UserID\":\"**\" }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 10. Update ConnectionPolicy - invalid - GroupID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"GroupID\":\"**\" }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 11. Update ConnectionPolicy - invalid - CommonName (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"CommonNames\":\"**\" }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 12. Update ConnectionPolicy - invalid option - ExpectedMsgRate : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ExpectedMessageRate\":\"Medium\" }}";
    CU_ASSERT( validate_ConnectionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    return;
}

