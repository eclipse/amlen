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

int dmrtestno = 0;

/*
 * Fake callback for transport
 */
int qpDMRSecurityCallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("Transport call is invoked\n");
    return 0;
}

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING QueuePolicy Configuration Object   ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateAdminEndpoint() API */
static int32_t validate_DestinationMappingRule(
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
    int rc = ism_config_validate_DestinationMappingRule(post, mergedObj, "DestinationMappingRule", (char *)name, CREATE, props);
    dmrtestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", dmrtestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}




/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_configObject_DestinationMappingRule(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("AdminEndpoint INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_SECURITY, NULL,
        (ism_config_callback_t) qpDMRSecurityCallback, &handle);

    printf("DestinationMappingRule ism_config_register(): rc: %d\n", rc);

    /* Add config items required for DestinationMappingRule unit tests */
    char *ibuf = NULL;
    json_error_t error = {0};
    json_t *value = NULL;

    /* Add QueueManagerConnection - name: testqmc1 */
    ibuf = "{ \"testqmc1\": { \"QueueManagerName\":\"QMN\", \"ConnectionName\":\"1.2.3.4(5)\", \"ChannelName\":\"CN\", \"SSLCipherSpec\":null }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("QueueManagerConnection", "testqmc1", value);
    CU_ASSERT( rc == ISMRC_OK );

    /* Add QueueManagerConnection - name: testqmc2 */
    ibuf = "{ \"testqmc2\": { \"QueueManagerName\":\"QMN\", \"ConnectionName\":\"1.2.3.4(5)\", \"ChannelName\":\"CN\", \"SSLCipherSpec\":null }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("QueueManagerConnection", "testqmc2", value);
    CU_ASSERT( rc == ISMRC_OK );

    /* Add QueueManagerConnection - name: testqmc3 */
    ibuf = "{ \"testqmc3\": { \"QueueManagerName\":\"QMN\", \"ConnectionName\":\"1.2.3.4(5)\", \"ChannelName\":\"CN\", \"SSLCipherSpec\":null }}";
    value = json_loads(ibuf, 0, &error);
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    rc = ism_config_json_setComposite("QueueManagerConnection", "testqmc3", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Add DestinationMappingRule - name: testqmc */
    ibuf = "{ \"testdmr\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"QueueManagerConnection\":\"testqmc1\" }}";
    value = json_loads(ibuf, 0, &error);
//    value = json_pack("{s{ssssssss}}","testmp","Description","TestQueuePolicy","ClientID","*","Queue","*","ActionList","Send,Receive,Browse");
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    printf("calling ism_config_json_setComposite\n");
    rc = ism_config_json_setComposite("DestinationMappingRule", "testdmr", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Create DestinationMappingRule: rc=ISMRC_OK */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"QueueManagerConnection\":\"testqmc1\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Create DestinationMappingRule: rc=ISMRC_OK */
    mbuf = "{ \"testdmr2\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"QueueManagerConnection\":\"testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 3. Create DestinationMappingRule: bad QueueManagerConnection name, rc=ISMRC_ObjectNotFound */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"QueueManagerConnection\":\"testqmc5\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_ObjectNotFound) == ISMRC_ObjectNotFound );

    /* 4. Create DestinationMappingRule: rc=ISMRC_RC */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 5. Create DestinationMappingRule: invalid QM destination, rc=6210 */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"abcd\", \"Destination\":\"?abcd\", \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, 6210) == 6210 );

    /* 6. Create DestinationMappingRule: rc=ISMRC_OK */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"?abcd\", \"Destination\":\"abcd\", \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 7. Create DestinationMappingRule: invalid ruletype, rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":17, \"Source\":\"?abcd\", \"Destination\":\"abcd\", \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 8. Create DestinationMappingRule: invalid ruletype, rc=ISMRC_BadPropertyType */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":\"5\", \"Source\":\"?abcd\", \"Destination\":\"abcd\", \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType );

    /* 9. Create DestinationMappingRule: invalid # in topic, rc=6210 */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"?abcd/#\", \"Destination\":\"abcd\", \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, 6210) == 6210 );

    /* 10. Create DestinationMappingRule: invalid + in topic, rc=6210 */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"?abcd/+/efg\", \"Destination\":\"abcd\", \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, 6210) == 6210 );

    /* 11. Create DestinationMappingRule: MaxMessages=1, rc=ISMRC_RC */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"MaxMessages\":1, \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 12. Create DestinationMappingRule: MaxMessages=20000000, rc=ISMRC_RC */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"MaxMessages\":20000000, \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 13. Create DestinationMappingRule: MaxMessages=0, rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"MaxMessages\":0, \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 14. Create DestinationMappingRule: MaxMessages=20000001, rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"MaxMessages\":20000001, \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 15. Create DestinationMappingRule: RetainedMessages=All, rc=ISMRC_RC */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":2, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"RetainedMessages\":\"All\", \"QueueManagerConnection\":\"testqmc1\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 15. Create DestinationMappingRule: RetainedMessages=All, rc=6212 */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":2, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"RetainedMessages\":\"All\", \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, 6212) == 6212 );

    /* 16. Create DestinationMappingRule: RetainedMessages=All, rc=6211 */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":5, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"RetainedMessages\":\"All\", \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, 6211) == 6211 );

    /* 16. Create DestinationMappingRule: RetainedMessages=Any, rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"testdmr1\": { \"Enabled\":false, \"RuleType\":2, \"Source\":\"abcd\", \"Destination\":\"abcd\", \"RetainedMessages\":\"Any\", \"QueueManagerConnection\":\"testqmc1,testqmc2\" }}";
    CU_ASSERT( validate_DestinationMappingRule(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    return;
}

