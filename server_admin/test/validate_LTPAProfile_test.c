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

/******************************************************************************************************
 * ----------   CUNIT TES FUNCTIONS FOR VALIDATING LTPAProfile Object   ----------------------
 ******************************************************************************************************/

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int ltpatestno = 0;

/*
 * Fake callback for Security
 */
int securityCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("Security call is invoked\n");
    return 0;
}

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING LTPAProfile Configuration Object   ---------------
 *
 ******************************************************************************************************/

static int32_t validate_LTPAProfile(
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
    int rc = ism_config_validate_LTPAProfile(post, mergedObj, "LTPAProfile", (char *)name, CREATE, props);
    ltpatestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", ltpatestno, mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}


void test_validate_configObject_LTPAProfile(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
//    ism_common_setTraceLevel(9);
    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("Admin INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);
    rc = ism_config_register(ISM_CONFIG_COMP_SECURITY, NULL, (ism_config_callback_t) securityCallback, &handle);

    printf("LTPAProfile ism_config_register(): rc: %d\n", rc);

    /* Run tests for the following config items
     * "Name":"string",
     * "KeyFileName": "string",
     * "Password": "string",
     * "Overwrite": "Boolean"
     */

    /* Set static configuration LTPAKeyStore */
    ism_field_t var = {0};
    var.type = VT_String;
    var.val.s = "test";
    rc = ism_common_setProperty(ism_common_getConfigProperties(), "LTPAKeyStore", &var);
    CU_ASSERT(rc == ISMRC_OK);

    /* Get status configuration LTPAKeyStore */
    const char *ltpaKeyStore = ism_common_getStringProperty(ism_common_getConfigProperties(), "LTPAKeyStore");
    printf("LTPAProfile test: LTPAKeyStore: %s\n", ltpaKeyStore?ltpaKeyStore:"NULL");
    CU_ASSERT_PTR_NOT_NULL(ltpaKeyStore);
    int strCmpRC = ISMRC_Error;
    if ( ltpaKeyStore && strcmp(ltpaKeyStore, "test") == 0 ) strCmpRC = ISMRC_OK;
    CU_ASSERT(strCmpRC == ISMRC_OK);


    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    mbuf = "{ \"MyLTPAProfile\": { \"KeyFileName\":\"TestLTPAKey\",\"Password\":\"password\" }}";
    CU_ASSERT( validate_LTPAProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyLTPAProfile\": { \"KeyFileName\":\"TestLTPAKey\",\"Password\":\"password\",\"Overwrite\":true }}";
    CU_ASSERT( validate_LTPAProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyLTPAProfile\": { \"KeyFileName\":\"TestLTPAKey\",\"Password\":\"xpassword\" }}";
    CU_ASSERT( validate_LTPAProfile(mbuf, ISMRC_LTPAInvalidKeyFile) == ISMRC_LTPAInvalidKeyFile );

    mbuf = "{ \"MyLTPAProfile\": { \"KeyFileName\":\"TestLTPAKey\",\"Password\":\"XXXXXX\" }}";
    CU_ASSERT( validate_LTPAProfile(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );


    return;
}

