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

int qpoltestno = 0;

/*
 * Fake callback for transport
 */
int qpEPSecurityCallback(
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
static int32_t validate_QueuePolicy(
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
    int rc = ism_config_validate_QueuePolicy(post, mergedObj, "QueuePolicy", (char *)name, CREATE, props);
    qpoltestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", qpoltestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}




/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_configObject_QueuePolicy(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("AdminEndpoint INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_SECURITY, NULL,
        (ism_config_callback_t) qpEPSecurityCallback, &handle);

    printf("QueuePolicy ism_config_register(): rc: %d\n", rc);

    /* Add config items required for QueuePolicy unit tests */
    char *ibuf = NULL;
    json_error_t error = {0};
    json_t *value = NULL;


    /* Add QueuePolicy - name: testmp */
    ibuf = "{ \"testmp\": { \"Description\":\"Test QueuePolicy\", \"ClientID\":\"*\", \"Queue\":\"*\", \"ActionList\":\"Send,Receive,Browse\" }}";
    value = json_loads(ibuf, 0, &error);
//    value = json_pack("{s{ssssssss}}","testmp","Description","TestQueuePolicy","ClientID","*","Queue","*","ActionList","Send,Receive,Browse");
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    printf("calling ism_config_json_setComposite\n");
    rc = ism_config_json_setComposite("QueuePolicy", "testmp", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Create QueuePolicy: rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":\"Test QueuePolicy 1\", \"ClientID\":\"*\", \"Queue\":\"*\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Create QueuePolicy: rc=ISMRC_OK */
    mbuf = "{ \"TestMp2\": { \"Description\":\"Test QueuePolicy 2\", \"ClientID\":\"*\", \"Queue\":\"*\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 3. Update QueuePolicy - default description i.e. empty string : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"*\", \"Queue\":\"*\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 4. Update QueuePolicy - name length 256 characters : rc=ISMRC_OK */
    mbuf = "{ \"a256_CharName111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999900000000001111111111222222222233333333334444444444123456\": { \"Description\":\"a256_CharName\", \"ClientID\":\"*\", \"Queue\":\"*\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 5. Update QueuePolicy - name length 257 characters : rc=ISMRC_NameLimitExceed */
    mbuf = "{ \"a257_CharName1111111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000111111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444441234567\": { \"Description\":\"a257_CharName\", \"ClientID\":\"*\", \"Queue\":\"*\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_NameLimitExceed) == ISMRC_NameLimitExceed );

    /* 6. Update QueuePolicy - invalid - no MinOneOpt given : rc=ISMRC_MinOneOptMissing */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_MinOneOptMissing) == ISMRC_MinOneOptMissing );

    /* 7. Update QueuePolicy - invalid - bad ActionList value : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"*\", \"ActionList\":\"Send,Receive,Subscribe\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 8. Update QueuePolicy - invalid - missing Queue : rc=ISMRC_PropertyRequired */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"*\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    /* 9. Update QueuePolicy - Queue and MaxMessages : rc=ISMRC_BadAdminPropName */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"*\", \"MaxMessages\":5000, \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadAdminPropName) == ISMRC_BadAdminPropName );

    /* 10. Update QueuePolicy - Queue and MaxMessagesBehavior : rc=ISMRC_BadAdminPropName */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"*\", \"MaxMessagesBehavior\":\"RejectNewMessages\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadAdminPropName) == ISMRC_BadAdminPropName );

    /* 11. Update QueuePolicy - ClientID bad value = "ab*cd* : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"ab*cd*\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 12. Update QueuePolicy - ClientID bad value = "ab*cd : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"ab*cd\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 13. Update QueuePolicy - MaxMessageTimeToLive="1" : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"1\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 14. Update QueuePolicy - MaxMessageTimeToLive="2147483647" : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"2147483647\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 15. Update QueuePolicy - MaxMessageTimeToLive="unlimited" : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"unlimited\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 16. Update QueuePolicy - MaxMessageTimeToLive="whatever" : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"whatever\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 17. Update QueuePolicy - MaxMessageTimeToLive="0" : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"0\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 18. Update QueuePolicy - MaxMessageTimeToLive="2147483648" : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"2147483648\", \"ActionList\":\"Send,Receive,Browse\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 19. Update QueuePolicy - invalid - ClientID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"ClientID\":\"**\", \"ActionList\":\"Send,Receive\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 20. Update QueuePolicy - invalid - UserID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"UserID\":\"**\", \"ActionList\":\"Send,Receive\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 21. Update QueuePolicy - invalid - GroupID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"GroupID\":\"**\", \"ActionList\":\"Send,Receive\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 22. Update QueuePolicy - invalid - CommonName (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"CommonNames\":\"**\", \"ActionList\":\"Send,Receive\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 23. Update QueuePolicy - invalid - Protocol: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Queue\":\"*\", \"Protocol\":\"MQTT\", \"ActionList\":\"Send,Receive\" }}";
    CU_ASSERT( validate_QueuePolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );


    return;
}

