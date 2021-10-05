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

int tmtestno = 0;


/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING MessageHub Configuration Object   ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateAdminEndpoint() API */
static int32_t validate_TopicMonitor(
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

    json_t *mergedObj = json_object_get(post, "TopicMonitor");

    CU_ASSERT( mergedObj != NULL );

    ism_prop_t *props = ism_common_newProperties(ISM_VALIDATE_CONFIG_ENTRIES);

    /* no difference in CREATE and UPDATE */
    int rc = ism_config_validate_TopicMonitor(post, mergedObj, "TopicMonitor", NULL, CREATE, props);
    tmtestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", tmtestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}




/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_configObject_TopicMonitor(void)
{

    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Create TopicMonitor: rc=ISMRC_OK */
    mbuf = "{ \"TopicMonitor\": [ \"/topic1/#\" ] }";
    CU_ASSERT( validate_TopicMonitor(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Create TopicMonitor: rc=ISMRC_OK */
    mbuf = "{ \"TopicMonitor\": [ \"/topic15/#\", \"/topic16/#\", \"/topic17/#\", \"/topic18/#\" ] }";
    CU_ASSERT( validate_TopicMonitor(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 3. Update TopicMonitor - default description i.e. empty string : rc=ISMRC_OK */
    mbuf = "{ \"TopicMonitor\": [ \"/topic1/#/\" ] }";
    CU_ASSERT( validate_TopicMonitor(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 4. Update TopicMonitor - default description i.e. empty string : rc=ISMRC_OK */
    mbuf = "{ \"TopicMonitor\": [ \"/topic1/#/#\" ] }";
    CU_ASSERT( validate_TopicMonitor(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 5. Update TopicMonitor - name length 256 characters : rc=ISMRC_OK */
    mbuf = "{ \"TopicMonitor\": [ \"/topic1/+/#\" ] }";
    CU_ASSERT( validate_TopicMonitor(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 6. Update TopicMonitor - name length 257 characters : rc=ISMRC_NameLimitExceed */
    mbuf = "{ \"TopicMonitor\": [ \"$SYS/#\" ] }";
    CU_ASSERT( validate_TopicMonitor(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    return;
}

