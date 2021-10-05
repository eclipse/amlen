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

int spoltestno = 0;

/*
 * Fake callback for transport
 */
int spEPSecurityCallback(
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
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING SubscriptionPolicy Configuration Object   ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateAdminEndpoint() API */
static int32_t validate_SubscriptionPolicy(
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
    int rc = ism_config_validate_SubscriptionPolicy(post, mergedObj, "SubscriptionPolicy", (char *)name, CREATE, props);
    spoltestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", spoltestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}




/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_configObject_SubscriptionPolicy(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("AdminEndpoint INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_SECURITY, NULL,
        (ism_config_callback_t) spEPSecurityCallback, &handle);

    printf("SubscriptionPolicy ism_config_register(): rc: %d\n", rc);

    /* Add config items required for SubscriptionPolicy unit tests */
    char *ibuf = NULL;
    json_error_t error = {0};
    json_t *value = NULL;


    /* Add SubscriptionPolicy - name: testmp */
    ibuf = "{ \"testmp\": { \"Description\":\"Test SubscriptionPolicy\", \"ClientID\":\"*\", \"Subscription\":\"*\", \"ActionList\":\"Receive,Control\" }}";
    value = json_loads(ibuf, 0, &error);
//    value = json_pack("{s{ssssssss}}","testmp","Description","TestSubscriptionPolicy","ClientID","*","Subscription","*","ActionList","Receive,Control");
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    printf("calling ism_config_json_setComposite\n");
    rc = ism_config_json_setComposite("SubscriptionPolicy", "testmp", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Create SubscriptionPolicy: rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":\"Test SubscriptionPolicy 1\", \"ClientID\":\"*\", \"Subscription\":\"*\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Create SubscriptionPolicy: rc=ISMRC_OK */
    mbuf = "{ \"TestMp2\": { \"Description\":\"Test SubscriptionPolicy 2\", \"ClientID\":\"*\", \"Subscription\":\"*\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 3. Update SubscriptionPolicy - default description i.e. empty string : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"*\", \"Subscription\":\"*\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 4. Update SubscriptionPolicy - name length 256 characters : rc=ISMRC_OK */
    mbuf = "{ \"a256_CharName111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999900000000001111111111222222222233333333334444444444123456\": { \"Description\":\"a256_CharName\", \"ClientID\":\"*\", \"Subscription\":\"*\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 5. Update SubscriptionPolicy - name length 257 characters : rc=ISMRC_NameLimitExceed */
    mbuf = "{ \"a257_CharName1111111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000111111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444441234567\": { \"Description\":\"a257_CharName\", \"ClientID\":\"*\", \"Subscription\":\"*\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_NameLimitExceed) == ISMRC_NameLimitExceed );

    /* 6. Update SubscriptionPolicy - invalid - no MinOneOpt given : rc=ISMRC_MinOneOptMissing */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_MinOneOptMissing) == ISMRC_MinOneOptMissing );

    /* 7. Update SubscriptionPolicy - invalid - bad ActionList value : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"ClientID\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 8. Update SubscriptionPolicy - invalid - missing Subscription : rc=ISMRC_PropertyRequired */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"*\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    /* 09. Update SubscriptionPolicy - MaxMessagesBehavior bad value : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"*\", \"MaxMessagesBehavior\":\"Bad\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 10. Update SubscriptionPolicy - ClientID bad value = "ab*cd* : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"ab*cd*\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 11. Update SubscriptionPolicy - ClientID bad value = "ab*cd : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"ab*cd\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 12. Update SubscriptionPolicy - MaxMessageTimeToLive="1" : rc=ISMRC_BadAdminPropName */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"1\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadAdminPropName) == ISMRC_BadAdminPropName );

    /* 13. Update SubscriptionPolicy - MaxMessageTimeToLive="2147483647" : rc=ISMRC_BadAdminPropName */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"2147483647\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadAdminPropName) == ISMRC_BadAdminPropName );

    /* 14. Update SubscriptionPolicy - MaxMessageTimeToLive="unlimited" : rc=ISMRC_BadAdminPropName */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"unlimited\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadAdminPropName) == ISMRC_BadAdminPropName );

    /* 15. Update SubscriptionPolicy - MaxMessageTimeToLive="whatever" : rc=ISMRC_BadAdminPropName */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"whatever\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadAdminPropName) == ISMRC_BadAdminPropName );

    /* 16. Update SubscriptionPolicy - MaxMessageTimeToLive="0" : rc=ISMRC_BadAdminPropName */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"0\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadAdminPropName) == ISMRC_BadAdminPropName );

    /* 17. Update SubscriptionPolicy - MaxMessageTimeToLive="2147483648" : rc=ISMRC_BadAdminPropName */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"2147483648\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadAdminPropName) == ISMRC_BadAdminPropName );

    /* 18. Update SubscriptionPolicy - invalid - ClientID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"ClientID\":\"**\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 19. Update SubscriptionPolicy - invalid - UserID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"UserID\":\"**\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 20. Update SubscriptionPolicy - invalid - GroupID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"GroupID\":\"**\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 21. Update SubscriptionPolicy - invalid - CommonName (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Subscription\":\"*\", \"CommonNames\":\"**\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 22. Update SubscriptionPolicy - DefaultSelectionRule="JMS_Topic == 'TestTopic'" : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"DefaultSelectionRule\":\"JMS_Topic = 'TestTopic'\", \"Subscription\":\"*\", \"ClientID\":\"*\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 22. Update SubscriptionPolicy - DefaultSelectionRule="JMS_Topic not like NotAString" : rc=ISMRC_LikeSyntax */
    mbuf = "{ \"TestMp1\": { \"DefaultSelectionRule\":\"JMS_Topic not like NotAString\", \"Subscription\":\"*\", \"ClientID\":\"*\", \"ActionList\":\"Receive,Control\" }}";
    CU_ASSERT( validate_SubscriptionPolicy(mbuf, ISMRC_LikeSyntax) == ISMRC_LikeSyntax );

    return;
}

