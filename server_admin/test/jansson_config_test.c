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
 * File: jansson_config_test.c
 * Component: server
 * SubComponent: server_admin
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <admin.h>
#include <config.h>
#include <janssonConfig.h>
#include <validateConfigData.h>

extern int32_t ism_config_validate_Singleton(json_t *currPostObj, json_t *objval, char * item);
extern int ism_config_set_fromJSONStr(char *jsonStr, char *object, int validate);
extern ism_prop_t * ism_config_getProperties(ism_config_t * handle, const char * object, const char * name);
XAPI int ism_common_initServer();

int janssonConfigTestNo = 0;

/*
 * Fake callback for Server
 */
int janssonTestServerCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("Dummy Server callback is invoked\n");
    return 0;
}

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING ism_config_set_fromJSONStr() ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_set_fromJSONStr() API */
static int32_t validate_json_configStr(
    char *mbuf,
    char *object,
    int32_t exprc)
{
    int validate = 0;
    int rc = ism_config_set_fromJSONStr(mbuf, object, validate);
    janssonConfigTestNo += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", janssonConfigTestNo,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    return rc;
}

/* Cunit test for ism_config_set_fromJSONStr API */
void testConfigSetJSONStr(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("JANSSON Config Test: INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_SERVER, NULL, (ism_config_callback_t) janssonTestServerCallback, &handle);

    printf("JANSSON Config Test: ism_config_register(): rc: %d\n", rc);

    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;
    char *object = NULL;

    // Sample buffer:
    // mbuf = "{ \"ServerUID\": \"VklNcTRk\" }"

    /* 1. Add a valid value: rc=ISMRC_OK */
    mbuf = "{ \"ServerUID\": \"VklNcTRk\" }";
    object = "ServerUID";
    CU_ASSERT( validate_json_configStr(mbuf, object, ISMRC_OK) == ISMRC_OK );

    /* 2. NULL post string: rc=ISMRC_NullPointer */
    CU_ASSERT( validate_json_configStr(NULL, object, ISMRC_NullPointer) == ISMRC_NullPointer );

    /* 3. NULL object: rc=ISMRC_NullPointer */
    CU_ASSERT( validate_json_configStr(mbuf, NULL, ISMRC_NullPointer) == ISMRC_NullPointer );

    /* 4. Invalid JSON string: rc=ISMRC_BadRESTfulRequest */
    mbuf = "{ \"ServerUID\": \"VklNcTRk\" ";
    CU_ASSERT( validate_json_configStr(mbuf, object, ISMRC_BadRESTfulRequest) == ISMRC_BadRESTfulRequest );

    /* 5. Invalid Object: rc=ISMRC_InvalidCfgObject */
    mbuf = "{ \"ServerXUUID\": \"VklNcTRk\" }";
    CU_ASSERT( validate_json_configStr(mbuf, object, ISMRC_BadRESTfulRequest) == ISMRC_BadRESTfulRequest );

    /* 6. Set to correct value and get the value to validate */
    mbuf = "{ \"ServerUID\": \"VklNcTRk\" }";
    object = "ServerUID";
    CU_ASSERT( validate_json_configStr(mbuf, object, ISMRC_OK) == ISMRC_OK );



    /* Tests for TolerateRecoveryInconsistencies */
    ism_prop_t * aprops = NULL;
    ism_field_t val1;

    object = "TolerateRecoveryInconsistencies";

    /* 7. Set TolerateRecoveryInconsistencies to true */
    mbuf = "{ \"TolerateRecoveryInconsistencies\": true }";
    CU_ASSERT( validate_json_configStr(mbuf, object, ISMRC_OK) == ISMRC_OK );

    /* 8. Set TolerateRecoveryInconsistencies to false */
    mbuf = "{ \"TolerateRecoveryInconsistencies\": false }";
    CU_ASSERT( validate_json_configStr(mbuf, object, ISMRC_OK) == ISMRC_OK );

    /* 9. Set TolerateRecoveryInconsistencies to null */
    mbuf = "{ \"TolerateRecoveryInconsistencies\": null }";
    CU_ASSERT( validate_json_configStr(mbuf, object, ISMRC_OK) == ISMRC_OK );

    /* 10 set TolerateRecoveryInconsistencies to true, get the value and verify */
    mbuf = "{ \"TolerateRecoveryInconsistencies\": true }";
    CU_ASSERT( validate_json_configStr(mbuf, object, ISMRC_OK) == ISMRC_OK );
    aprops = ism_config_json_getObjectProperties("TolerateRecoveryInconsistencies", NULL, 1);
    CU_ASSERT( aprops != NULL );
    if (aprops) {
        ism_common_getProperty(aprops, "TolerateRecoveryInconsistencies", &val1);
        CU_ASSERT( val1.type == VT_String);
        if (val1.type==VT_String) {
            printf("TolerateRecoveryInconsistencies String : %s\n",val1.val.s);
            CU_ASSERT_STRING_EQUAL(val1.val.s, "true");
        }
        ism_common_freeProperties(aprops);
        aprops = NULL;
    }

    /* 11 set TolerateRecoveryInconsistencies to false, get the value and verify */
    mbuf = "{ \"TolerateRecoveryInconsistencies\": false }";
    CU_ASSERT( validate_json_configStr(mbuf, object, ISMRC_OK) == ISMRC_OK );
    aprops = ism_config_json_getObjectProperties("TolerateRecoveryInconsistencies", NULL, 1);
    CU_ASSERT( aprops != NULL );
    if ( aprops ) {
		ism_common_getProperty(aprops, "TolerateRecoveryInconsistencies", &val1);
		CU_ASSERT( val1.type == VT_String);
		if(val1.type==VT_String){
			printf("TolerateRecoveryInconsistencies String : %s\n",val1.val.s);
			CU_ASSERT_STRING_EQUAL(val1.val.s, "false");
		}
		ism_common_freeProperties(aprops);
		aprops = NULL;
    }

    return;

}
