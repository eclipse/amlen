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
 * ----------   CUNIT TES FUNCTIONS FOR VALIDATING OAuthProfile Object   ----------------------
 ******************************************************************************************************/

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int oauthtestno = 0;

/*
 * Fake callback for Security
 */
int securityOauthCallback(
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
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING OAuthProfile Configuration Object   ---------------
 *
 ******************************************************************************************************/

static int32_t validate_OAuthProfile(
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
    int rc = ism_config_validate_OAuthProfile(post, mergedObj, "OAuthProfile", (char *)name, CREATE, props);
    oauthtestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", oauthtestno, mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}


void test_validate_configObject_OAuthProfile(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
//    ism_common_setTraceLevel(9);
    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("Admin INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);
    rc = ism_config_register(ISM_CONFIG_COMP_SECURITY, NULL, (ism_config_callback_t) securityOauthCallback, &handle);

    printf("OAuthProfile ism_config_register(): rc: %d\n", rc);

    /* Run tests for the following config items
     *        "ResourseURL":"string",
     *        "KeyFileName":"string",
     *        "AuthKey":"string",
     *        "UserInfoURL":"string",
     *        "UserInfoKey":"string",
     *        "GroupInfoKey":"string",
     *        "UsePost":"boolean"
     */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    mbuf = "{ \"MyOAuthProfile\": { }}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"http://192.23.45.23/test\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"http://192.23.45.23/test\",\"UserInfoURL\":\"https://192.23.45.23/test\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"KeyFileName\":\"TestKeyFile\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, 6185) == 6185 );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"TokenSendMethod\":\"URLParam\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"TokenSendMethod\":\"HTTPHeader\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"TokenSendMethod\":\"HTTPPost\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"TokenSendMethod\":\"HTTPPostHeader\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"GroupDelimiter\":\",\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"GroupDelimiter\":\" \"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"GroupDelimiter\":\"#\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"GroupDelimiter\":\"# \"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"UserName\":\"xxxx\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"UserPassword\":\"xxxxyyyy\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"UserName\":\"xxx\",\"UserPassword\":\"\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"UserName\":\"xxx\",\"UserPassword\":\"yyyyy\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_OK) == ISMRC_OK );

    mbuf = "{ \"MyOAuthProfile\": { \"ResourceURL\":\"https://192.23.45.23/test\",\"UserName\":\"\",\"UserPassword\":\"xxxx\"}}";
    CU_ASSERT( validate_OAuthProfile(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    return;
}

