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
 * CUNIT tests for log location config item validation
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int lltestno = 0;

/*
 * Fake callback for server
 */
int testLLServerCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("Server call is invoked\n");
    return 0;
}

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING LogLocation Configuration Object   ---------------
 *
 ******************************************************************************************************/

static int32_t validate_LogLocation(
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
    int rc = ism_config_validate_LogLocation(post, mergedObj, "LogLocation", (char *)name, CREATE, props);
    lltestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", lltestno, mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}


void test_validate_configObject_LogLocation(void)
{
    int rc = ISMRC_OK;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("test_validate_configObject_LogLocation: Admin INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Create new log location: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestLogLocation1\": { \"LocationType\":\"file\", \"Destination\":\"testfile.log\" }}";
    CU_ASSERT( validate_LogLocation(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 2. Update AdminLog log location - type "file" : rc=ISMRC_OK */
    mbuf = "{ \"AdminLog\": { \"LocationType\":\"file\", \"Destination\":\"testfile.log\" }}";
    CU_ASSERT( validate_LogLocation(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 3. Update AdminLog log location - type "syslog", valid integer destination : rc=ISMRC_OK */
    mbuf = "{ \"AdminLog\": { \"LocationType\":\"syslog\", \"Destination\":\"11\" }}";
    CU_ASSERT( validate_LogLocation(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 4. Update AdminLog log location - type "syslog", invalid integer destination : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"AdminLog\": { \"LocationType\":\"syslog\", \"Destination\":\"24\" }}";
    CU_ASSERT( validate_LogLocation(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 5. Update AdminLog log location - type "syslog", invalid integer destination : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"AdminLog\": { \"LocationType\":\"syslog\", \"Destination\":\"-1\" }}";
    CU_ASSERT( validate_LogLocation(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 6. Update AdminLog log location - type "syslog", non-integer destination : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"AdminLog\": { \"LocationType\":\"syslog\", \"Destination\":\"testfile.log\" }}";
    CU_ASSERT( validate_LogLocation(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 7. Update AdminLog log location - type "file", file name length 256 characters : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"AdminLog\": { \"LocationType\":\"file\", \"Destination\":\"a256_CharName111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999900000000001111111111222222222233333333334444444444123456\" }}";
    CU_ASSERT( validate_LogLocation(mbuf, ISMRC_LenthLimitExceed) == ISMRC_LenthLimitExceed );

    /* 8. Update AdminLog log location - type "file", no destination : rc=ISMRC_PropertyRequired */
    mbuf = "{ \"AdminLog\": { \"LocationType\":\"file\" }}";
    CU_ASSERT( validate_LogLocation(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    /* 9. Update AdminLog log location - destination, no type: rc=ISMRC_PropertyRequired */
    mbuf = "{ \"AdminLog\": { \"Destination\":\"10\" }}";
    CU_ASSERT( validate_LogLocation(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    /* 10. Update AdminLog log location - type "test" : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"AdminLog\": { \"LocationType\":\"test\", \"Destination\":\"testfile.log\" }}";
    CU_ASSERT( validate_LogLocation(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 11. Update AdminLog log location - invalid property "Type" : rc=ISMRC_BadPropertyName */
    mbuf = "{ \"AdminLog\": { \"Type\":\"file\", \"Destination\":\"testfile.log\" }}";
    CU_ASSERT( validate_LogLocation(mbuf, ISMRC_BadPropertyName) == ISMRC_BadPropertyName );

    return;
}

