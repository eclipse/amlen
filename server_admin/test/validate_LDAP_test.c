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
 * Add CUNIT tests for LDAP configuration data validation
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();
XAPI void ism_admin_setCunitEnv(void);
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval);

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int ldaptestno = 0;

/*
 * Fake callback for security
 */
int ldapSecCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("Security callback is invoked\n");
    return 0;
}

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING Endpoint Configuration Object   ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateAdminEndpoint() API */
static int32_t validate_LDAP(
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

    int rc = ism_config_validate_LDAP(post, mergedObj, (char *)object, "ldapconfig", CREATE, props);
    ldaptestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", ldaptestno, mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}




/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_configObject_LDAP(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("AdminEndpoint INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    /* set env for cunit test */
    ism_admin_setCunitEnv();

    rc = ism_config_register(ISM_CONFIG_COMP_SECURITY, NULL, (ism_config_callback_t) ldapSecCallback, &handle);

    printf("LDAP ism_config_register(): rc: %d\n", rc);

    /* Add config items required for TopicPolicy unit tests */
    char *ibuf = NULL;
    json_error_t error = {0};
    json_t *value = NULL;


    /* Add LDAP object */
    ibuf = "{\"ldapconfig\":{\"URL\":\"ldap://184.173.22.41:6636\",\"Timeout\":30,\"EnableCache\":true,\"CacheTimeout\":10,\"GroupCacheTimeout\":300,\"BaseDN\":\"ou=SVT,o=IBM,c=US\",\"BindDN\":\"cn=root,o=IBM,c=US\",\"BindPassword\":\"ima4test\",\"UserIdMap\":\"b\",\"UserSuffix\":\"\",\"NestedGroupSearch\":false,\"GroupMemberIdMap\":\"c\",\"GroupIdMap\":\"d\",\"GroupSuffix\":\"e\",\"IgnoreCase\":true,\"Verify\":false,\"Enabled\":true,\"Overwrite\":true}}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("LDAP", "ldapconfig", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Update LDAP for cert and verify set to true: rc=ISMRC_VerifyTestOK */
    mbuf = "{ \"LDAP\": { \"URL\":\"ldaps://184.173.22.41:6636\",\"Certificate\":\"ldap.pem\",\"Verify\":true,\"Timeout\":30,\"EnableCache\":true,\"CacheTimeout\":10,\"GroupCacheTimeout\":300,\"BaseDN\":\"ou=SVT,o=IBM,c=US\",\"BindDN\":\"cn=root,o=IBM,c=US\",\"BindPassword\":\"ima4test\",\"UserIdMap\":\"b\",\"UserSuffix\":\"\",\"NestedGroupSearch\":false,\"GroupMemberIdMap\":\"c\",\"GroupIdMap\":\"d\",\"GroupSuffix\":\"e\",\"IgnoreCase\":true,\"Enabled\":true}}";
    CU_ASSERT( validate_LDAP(mbuf, ISMRC_VerifyTestOK) == ISMRC_VerifyTestOK );

    return;
}

