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
 * CUNIT tests for syslog config item validation
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval);

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int sltestno = 0;

/*
 * Fake callback for server
 */
int testSServerCallback(
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
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING Syslog Configuration Object   ---------------
 *
 ******************************************************************************************************/

static int32_t validate_Syslog(
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
    int rc = ism_config_validate_Syslog(post, mergedObj, "Syslog", (char *)name, CREATE, props);
    sltestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", sltestno, mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}

void test_validate_configObject_Syslog(void)
{
    int rc = ISMRC_OK;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    CU_ASSERT(rc == ISMRC_OK);

    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Create new syslog object: rc=ISMRC_ArgNotValid */
    mbuf = "{ \"TestSyslog1\": { \"Host\":\"127.0.0.1\", \"Port\": 514 }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_ArgNotValid) == ISMRC_ArgNotValid );

    /* 2. Update Syslog port (port 515) : rc=ISMRC_OK */
    mbuf = "{ \"Syslog\": { \"Port\": 515 }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 3. Update Syslog port (port 70000) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"Syslog\": { \"Port\": 70000 }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 4. Update Syslog host : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"Syslog\": { \"Host\": \"test.ibm.com\" }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 5. Update Syslog host and port : rc=ISMRC_OK */
    mbuf = "{ \"Syslog\": { \"Host\": \"10.2.0.15\"}}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 6. Update Syslog host 256 characters : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"Syslog\": { \"Host\": \"a256_CharName111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999900000000001111111111222222222233333333334444444444123456\" }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 7. Update Syslog host and port : rc=ISMRC_OK */
    mbuf = "{ \"Syslog\": { \"Host\": \"127.0.0.1\", \"Port\" : 514 }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 8. Update Syslog protocol (tcp) : rc=ISMRC_OK */
    mbuf = "{ \"Syslog\": { \"Protocol\": \"tcp\" }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 9. Update Syslog protocol (udp) : rc=ISMRC_OK */
    mbuf = "{ \"Syslog\": { \"Protocol\": \"udp\" }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 10. Update Syslog protocol (local) : rc=ISMRC_BadPropertyValue*/
    mbuf = "{ \"Syslog\": { \"Protocol\": \"local\" }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 11. Update Syslog host, port and protocol : rc=ISMRC_OK */
    mbuf = "{ \"Syslog\": { \"Host\": \"127.0.0.1\", \"Port\" : 600, \"Protocol\" : \"tcp\" }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 12. Update Syslog enabled=true with host: rc=ISMRC_OK */
    mbuf = "{ \"Syslog\": { \"Enabled\": true, \"Host\": \"127.0.0.1\", \"Port\" : 600, \"Protocol\" : \"tcp\" }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 13. Update Syslog enabled=true without host: rc=ISMRC_PropertyRequired */
    mbuf = "{ \"Syslog\": { \"Enabled\": true, \"Port\" : 600, \"Protocol\" : \"tcp\" }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 14. Update Syslog enabled=false without host: rc=ISMRC_OK */
    mbuf = "{ \"Syslog\": { \"Enabled\": false, \"Port\" : 600, \"Protocol\" : \"tcp\" }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 15. Update Syslog enabled="somestring": rc=ISMRC_BadPropertyType */
    mbuf = "{ \"Syslog\": { \"Enabled\": \"somestring\", \"Host\": \"127.0.0.1\" }}";
    CU_ASSERT( validate_Syslog(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    return;
}

