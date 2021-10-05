/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int asubtestno = 0;

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING AdminSub Configuration Object  ------------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateAdminSubscription() API */
static int32_t validate_AdminSubscription(
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
    int rc = ism_config_validate_AdminSubscription(post, mergedObj, "AdminSubscription", (char *)name, CREATE, props);
    asubtestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", asubtestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}




/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_configObject_AdminSubscription(void)
{
    int rc = ISMRC_OK;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("%s INIT: rc: %d\n", __func__, rc);
    CU_ASSERT(rc == ISMRC_OK);

    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *testBuffer = malloc(10240);
    char *longString = malloc(2048);

    memset(longString, 'X', 2047); longString[2047] = '\0';

    /* 1. Empty description rc=ISMRC_OK */
    sprintf(testBuffer, "{\"/AdminSub1/TopicFilter1\": { \"Description\":\"%s\", \"SubscriptionPolicy\":\"%s\" }}", "", "TestPolicy");
    CU_ASSERT( validate_AdminSubscription(testBuffer, ISMRC_OK) == ISMRC_OK );

    /* 2. Long description */
    sprintf(testBuffer, "{\"/AdminSub1/TopicFilter1\": { \"Description\":\"%s\", \"SubscriptionPolicy\":\"%s\" }}", longString, "TestPolicy");
    CU_ASSERT( validate_AdminSubscription(testBuffer, ISMRC_LenthLimitExceed) == ISMRC_LenthLimitExceed );

    /* 3. Empty policy name */
    sprintf(testBuffer, "{\"/AdminSub1/TopicFilter1\": { \"Description\":\"%s\", \"SubscriptionPolicy\":\"%s\" }}", "", "");
    CU_ASSERT( validate_AdminSubscription(testBuffer, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 4. NULL policy name */
    sprintf(testBuffer, "{\"/AdminSub1/TopicFilter1\": { \"Description\":\"%s\", \"SubscriptionPolicy\":null }}", "");
    CU_ASSERT( validate_AdminSubscription(testBuffer, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 5. Long policy name */
    sprintf(testBuffer, "{\"/AdminSub1/TopicFilter1\": { \"Description\":\"%s\", \"SubscriptionPolicy\":\"%s\" }}", "", longString);
    CU_ASSERT( validate_AdminSubscription(testBuffer, ISMRC_LenthLimitExceed) == ISMRC_LenthLimitExceed );

    /* 6. No leading slash on name */
    sprintf(testBuffer, "{\"AdminSub1/TopicFilter1\": { \"Description\":\"%s\", \"SubscriptionPolicy\":\"%s\" }}", "", longString);
    CU_ASSERT( validate_AdminSubscription(testBuffer, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 7. No topic filter */
    sprintf(testBuffer, "{\"/AdminSub1\": { \"Description\":\"%s\", \"SubscriptionPolicy\":\"%s\" }}", "", "TestPolicy");
    CU_ASSERT( validate_AdminSubscription(testBuffer, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 8. Empty share name filter */
    sprintf(testBuffer, "{\"//TopicFilter1\": { \"Description\":\"%s\", \"SubscriptionPolicy\":\"%s\" }}", "", "TestPolicy");
    CU_ASSERT( validate_AdminSubscription(testBuffer, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 9. Empty topic filter (VALID) */
    sprintf(testBuffer, "{\"/AdminSub1/\": { \"Description\":\"%s\", \"SubscriptionPolicy\":\"%s\" }}", "", "TestPolicy");
    CU_ASSERT( validate_AdminSubscription(testBuffer, ISMRC_OK) == ISMRC_OK );

    /* TODO: MORE VALIDATION... */

    free(longString);
    free(testBuffer);

    return;
}

