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

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval);

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int crltestno = 0;

/*
 * Fake callback for transport
 */
int CRLProfileTransportCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("Transport call is invoked\n");
    return 0;
}

/* wrapper to call ism_config_validateSecurityProfile() API */
static int32_t validate_CRLProfile(
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
    int rc = ism_config_validate_CRLProfile(post, mergedObj, "CRLProfile", (char *)name, CREATE, props);
    crltestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", crltestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}


/* Cunit test for CRLProfile configuration object validation */
void test_validate_configObject_CRLProfile(void) {
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("CRLProfile INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_TRANSPORT, NULL,
        (ism_config_callback_t) CRLProfileTransportCallback, &handle);

    printf("CRLProfile ism_config_register(): rc: %d\n", rc);

    /* Negative condition tests */
    char *mbuf = NULL;

    mbuf = "{ \"MyCRLProfile\": {\"CRLSource\":\"\" }}";
    CU_ASSERT( validate_CRLProfile(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    mbuf = "{ \"MyCRLProfile\": { \"CRLSource\": 6}}";
    CU_ASSERT( validate_CRLProfile(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"MyCRLProfile\": { \"UpdateInterval\":\"60 minutes\"}}";
    CU_ASSERT( validate_CRLProfile(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    mbuf = "{ \"MyCRLProfile\": { \"RevalidateConnection\":\"whatever\"}}";
    CU_ASSERT( validate_CRLProfile(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    return;
}


