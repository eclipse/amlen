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
 * CUNIT tests for plugin config item validation
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval);

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int ptestno = 0;

/*
 * Fake callback for protocol
 */
int testProtocolCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("Protocol call is invoked\n");
    return 0;
}

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING Plugin Configuration Object   ---------------
 *
 ******************************************************************************************************/

static int32_t validate_Plugin(
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
    int rc = ism_config_validate_Plugin(post, mergedObj, "Plugin", (char *)name, CREATE, props);
    ptestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", ptestno, mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}


void test_validate_configObject_Plugin(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("test_validate_configObject_Plugin: Admin INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_PROTOCOL, NULL, (ism_config_callback_t) testProtocolCallback, &handle);

    printf("test_validate_configObject_Plugin: ism_config_register(): rc: %d\n", rc);

    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Create plugin: rc=ISMRC_OK */
    mbuf = "{ \"TestPlugin1\": { \"File\":\"TestFile1\", \"PropertiesFile\":\"PropFile1\" }}";
    CU_ASSERT( validate_Plugin(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Update plugin - name length 256 characters : rc=ISMRC_OK */
    mbuf = "{ \"a256_CharName111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999900000000001111111111222222222233333333334444444444123456\": { \"File\":\"TestFile1\", \"PropertiesFile\":\"PropFile1\" }}";
    CU_ASSERT( validate_Plugin(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 3. Update plugin - name length 257 characters : rc=ISMRC_NameLimitExceed */
    mbuf = "{ \"a257_CharName1111111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000111111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444441234567\": { \"File\":\"TestFile1\", \"PropertiesFile\":\"PropFile1\" }}";
    CU_ASSERT( validate_Plugin(mbuf, ISMRC_NameLimitExceed) == ISMRC_NameLimitExceed );

    /* 4. Update plugin: rc=ISMRC_OK */
    mbuf = "{ \"TestPlugin1\": { \"File\":\"TestFile1\" }}";
    CU_ASSERT( validate_Plugin(mbuf, ISMRC_OK) == ISMRC_OK );

    return;
}

