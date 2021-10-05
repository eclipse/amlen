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

int tpoltestno = 0;

/*
 * Fake callback for transport
 */
int tpEPSecurityCallback(
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
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING TopicPolicy Configuration Object   ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateAdminEndpoint() API */
static int32_t validate_TopicPolicy(
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
    int rc = ism_config_validate_TopicPolicy(post, mergedObj, "TopicPolicy", (char *)name, CREATE, props);
    tpoltestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", tpoltestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}




/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_configObject_TopicPolicy(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("AdminEndpoint INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_SECURITY, NULL,
        (ism_config_callback_t) tpEPSecurityCallback, &handle);

    printf("TopicPolicy ism_config_register(): rc: %d\n", rc);

    /* Add config items required for TopicPolicy unit tests */
    char *ibuf = NULL;
    json_error_t error = {0};
    json_t *value = NULL;


    /* Add TopicPolicy - name: testmp */
    ibuf = "{ \"testmp\": { \"Description\":\"Test TopicPolicy\", \"ClientID\":\"*\", \"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}";
    value = json_loads(ibuf, 0, &error);
//    value = json_pack("{s{ssssssss}}","testmp","Description","TestTopicPolicy","ClientID","*","Topic","*","ActionList","Publish,Subscribe");
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    printf("calling ism_config_json_setComposite\n");
    rc = ism_config_json_setComposite("TopicPolicy", "testmp", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Create TopicPolicy: rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":\"Test TopicPolicy 1\", \"ClientID\":\"*\", \"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Create TopicPolicy: rc=ISMRC_OK */
    mbuf = "{ \"TestMp2\": { \"Description\":\"Test TopicPolicy 2\", \"ClientID\":\"*\", \"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 3. Update TopicPolicy - default description i.e. empty string : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"*\", \"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 4. Update TopicPolicy - name length 256 characters : rc=ISMRC_OK */
    mbuf = "{ \"a256_CharName111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444445555555555666666666677777777778888888888999999999900000000001111111111222222222233333333334444444444123456\": { \"Description\":\"a256_CharName\", \"ClientID\":\"*\", \"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 5. Update TopicPolicy - name length 257 characters : rc=ISMRC_NameLimitExceed */
    mbuf = "{ \"a257_CharName1111111222222222233333333334444444444555555555566666666667777777777888888888899999999990000000000111111111122222222223333333333444444444455555555556666666666777777777788888888889999999999000000000011111111112222222222333333333344444444441234567\": { \"Description\":\"a257_CharName\", \"ClientID\":\"*\", \"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_NameLimitExceed) == ISMRC_NameLimitExceed );

    /* 6. Update TopicPolicy - invalid - no MinOneOpt given : rc=ISMRC_MinOneOptMissing */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_MinOneOptMissing) == ISMRC_MinOneOptMissing );

    /* 7. Update TopicPolicy - invalid - bad ActionList value : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ClientID\":\"*\", \"ActionList\":\"Browse,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 8. Update TopicPolicy - invalid - missing Topic : rc=ISMRC_PropertyRequired */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"ClientID\":\"*\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_PropertyRequired) == ISMRC_PropertyRequired );

    /* 09. Update TopicPolicy - MaxMessagesBehavior bad value : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ClientID\":\"*\", \"MaxMessagesBehavior\":\"Bad\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 10. Update TopicPolicy - ClientID bad value = "ab*cd* : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ClientID\":\"ab*cd*\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 11. Update TopicPolicy - ClientID bad value = "ab*cd : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ClientID\":\"ab*cd\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 12. Update TopicPolicy - MaxMessageTimeToLive="1" : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"1\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 13. Update TopicPolicy - MaxMessageTimeToLive="2147483647" : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"2147483647\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 14. Update TopicPolicy - MaxMessageTimeToLive="unlimited" : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"unlimited\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 15. Update TopicPolicy - MaxMessageTimeToLive="whatever" : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"whatever\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 16. Update TopicPolicy - MaxMessageTimeToLive="0" : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"0\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 17. Update TopicPolicy - MaxMessageTimeToLive="2147483648" : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ClientID\":\"ab*\", \"MaxMessageTimeToLive\":\"2147483648\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 18. Update TopicPolicy - invalid - ClientID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"ClientID\":\"**\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 19. Update TopicPolicy - invalid - UserID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"UserID\":\"**\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 20. Update TopicPolicy - invalid - GroupID (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"GroupID\":\"**\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 21. Update TopicPolicy - invalid - CommonName (with **) : rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestMp1\": { \"Description\":null, \"Topic\":\"*\", \"CommonNames\":\"**\", \"ActionList\":\"Publish,Subscribe\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 22. Valid protocol for disconnected client notif : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":\"Test\", \"Topic\":\"*\", \"Protocol\":\"mqtt\", \"ActionList\":\"Subscribe\",\"DisconnectedClientNotification\":true }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 23. Valid protocol for disconnected client notif : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":\"Test\", \"Topic\":\"*\", \"Protocol\":\"MQTT\", \"ActionList\":\"Subscribe\",\"DisconnectedClientNotification\":true }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 24. Invalid protocol for disconnected client notif : rc=ISMRC_DisconnNotifError */
    mbuf = "{ \"TestMp1\": { \"Description\":\"Test\", \"Topic\":\"*\", \"Protocol\":\"JMS\", \"ActionList\":\"Subscribe\",\"DisconnectedClientNotification\":true }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_DisNotifProtError) == ISMRC_DisNotifProtError );

    /* 25. Invalid protocol for disconnected client notif : rc=ISMRC_DisconnNotifError */
    mbuf = "{ \"TestMp1\": { \"Description\":\"Test\", \"Topic\":\"*\", \"Protocol\":\"MQTT\", \"ActionList\":\"Publish\",\"DisconnectedClientNotification\":true }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_DisNotifSubsError) == ISMRC_DisNotifSubsError );

    /* 26. Valid non-empty DefaultSelectionRule : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":\"Test\", \"Topic\":\"*\", \"Protocol\":\"jms,mqtt\", \"ActionList\":\"Subscribe\",\"DefaultSelectionRule\": \"JMS_Topic = 'yesTopic'\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 27. Valid NULL DefaultSelectionRule : rc=ISMRC_OK */
    mbuf = "{ \"TestMp1\": { \"Description\":\"Test\", \"Topic\":\"*\", \"Protocol\":\"jms,mqtt\", \"ActionList\":\"Subscribe\",\"DefaultSelectionRule\": null }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 28. Invalid DefaultSelectionRule (unfinished string) : rc=ISMRC_ObjectNotValid */
    mbuf = "{ \"TestMp1\": { \"Description\":\"Test\", \"Topic\":\"*\", \"Protocol\":\"jms,mqtt\", \"ActionList\":\"Subscribe\",\"DefaultSelectionRule\": \"JMS_Topic like '%/abc/%\" }}";
    CU_ASSERT( validate_TopicPolicy(mbuf, ISMRC_LikeSyntax) == ISMRC_LikeSyntax );

    return;
}

