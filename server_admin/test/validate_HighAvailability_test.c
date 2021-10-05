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
 * Add CUNIT tests for AdminEndpoint configuration data validation
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <validateConfigData.h>

XAPI int ism_common_initServer();
XAPI int ism_config_json_setComposite(const char *object, const char *name, json_t *objval);

#define CREATE 0
#define UPDATE 1
#define DELETE 2

int hatestno = 0;

/*
 * Fake callback for HA
 */
int HACallback(
    char *object,
    char *name,
    ism_prop_t *props,
    ism_ConfigChangeType_t flag)
{
    printf("HA call is invoked\n");
    return 0;
}

/******************************************************************************************************
 *
 * -------------   CUNIT TES FUNCTIONS FOR VALIDATING AdminEndpoint Configuration Object   ---------------
 *
 ******************************************************************************************************/

/* wrapper to call ism_config_validateAdminEndpoint() API */
static int32_t validate_HighAvailability(
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

    /* no difference in CREATE and UPDATE */
    int rc = ism_config_validate_HighAvailability(post, mergedObj, (char *)object, "haconfig", CREATE, props);
    hatestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", hatestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}

/* Cunit test for AdminEndpoint configuration object validation */
void test_validate_configObject_HighAvailability(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    ism_common_setTraceLevel(0);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("AdminEndpoint INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_STORE, NULL,
        (ism_config_callback_t) HACallback, &handle);

    printf("HighAvailability ism_config_register(): rc: %d\n", rc);

    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Update: HA is created by default: rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"EnableHA\":true, \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroupA\" }}";
    CU_ASSERT( validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Name is not settable: rc=ISMRC_NullPointer */
    mbuf = "{ \"TestHA\": { \"EnableHA\":false, \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroupA\" }}";
    CU_ASSERT( validate_HighAvailability(mbuf, ISMRC_NullPointer) == ISMRC_NullPointer);

    /* 3. Test EnableHA - false: rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 4. Test EnableHA - badValue: rc=ISMRC_NullPointer */
    mbuf = "{ \"HighAvailability\": { \"EnableHA\":badValue, \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_NullPointer) == ISMRC_NullPointer);

    /* 5. Test EnableHA - "badValue": rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"EnableHA\":\"badValue\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 6. Test PreferredPrimary - true: rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"PreferredPrimary\":true, \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 7. Test PreferredPrimary - false: rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"PreferredPrimary\":false, \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 8. Test PreferredPrimary - badValue: rc=ISMRC_NullPointer */
    mbuf = "{ \"HighAvailability\": { \"PreferredPrimary\":badValue, \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_NullPointer) == ISMRC_NullPointer);

    /* 9. Test PreferredPrimary - "badValue": rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"PreferredPrimary\":\"badValue\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 10. Test StartupMode - "AutoDetect": rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"StartupMode\":\"AutoDetect\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 11. Test StartupMode - "StandAlone": rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"StartupMode\":\"StandAlone\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 12. Test StartupMode - "badValue": rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"StartupMode\":\"badValue\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 13. Test RemoteDiscoveryNIC - "192.168.23.5": rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

#ifdef HA_NIC_HYPHENS_DISALLOWED
    /* 14. Test RemoteDiscoveryNIC - "192.168.23.5-192.168.35.5": rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"RemoteDiscoveryNIC\":\"192.168.23.5-192.168.35.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);
#endif

    /* 15. Test RemoteDiscoveryNIC - "192.168.23.5,192.168.35.5": rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"RemoteDiscoveryNIC\":\"192.168.23.5,192.168.35.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 16. Test RemoteDiscoveryNIC - 1923: rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"RemoteDiscoveryNIC\":1923, \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 17. Test RemoteDiscoveryNIC - true: rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"RemoteDiscoveryNIC\":true, \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 18. Test LocalReplicationNIC - "192.168.23.5": rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"LocalReplicationNIC\":\"192.168.23.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

#ifdef HA_NIC_HYPHENS_DISALLOWED
    /* 19. Test LocalReplicationNIC - "192.168.23.5-192.168.35.5": rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"LocalReplicationNIC\":\"192.168.23.5-192.168.35.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);
#endif

    /* 20. Test LocalReplicationNIC - "192.168.23.5,192.168.35.5": rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"LocalReplicationNIC\":\"192.168.23.5,192.168.35.5\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 21. Test LocalReplicationNIC - 1923: rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"LocalReplicationNIC\":1923, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 22. Test LocalReplicationNIC - true: rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"LocalReplicationNIC\":true, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 23. Test LocalDiscoveryNIC - "192.168.23.5": rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

#ifdef HA_NIC_HYPHENS_DISALLOWED
    /* 24. Test LocalDiscoveryNIC - "192.168.23.5-192.168.35.5": rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"LocalDiscoveryNIC\":\"192.168.23.5-192.168.35.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);
#endif

    /* 25. Test LocalDiscoveryNIC - "192.168.23.5,192.168.35.5": rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"LocalDiscoveryNIC\":\"192.168.23.5,192.168.35.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 26. Test LocalDiscoveryNIC - 1923: rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"LocalDiscoveryNIC\":1923, \"LocalReplicationNIC\":true, \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 27. Test LocalDiscoveryNIC - true: rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"LocalDiscoveryNIC\":true, \"LocalReplicationNIC\":true, \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 28. Test DiscoveryTimeout - 1923: rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"DiscoveryTimeout\":1923, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 29. Test DiscoveryTimeout - 10: rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"DiscoveryTimeout\":10, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 30. Test DiscoveryTimeout - 2147483647: rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"DiscoveryTimeout\":2147483647, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 31. Test DiscoveryTimeout - 3: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"DiscoveryTimeout\":3, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 32. Test DiscoveryTimeout - -33: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"DiscoveryTimeout\":-33, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 33. Test DiscoveryTimeout - 2147483648: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"DiscoveryTimeout\":2147483648, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 34. Test DiscoveryTimeout - "badValue": rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"DiscoveryTimeout\":\"badValue\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 35. Test DiscoveryTimeout - true: rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"DiscoveryTimeout\":true, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 36. Test HeartbeatTimeout - 1923: rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"HeartbeatTimeout\":1923, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 37. Test HeartbeatTimeout - 1: rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"HeartbeatTimeout\":10, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 38. Test HeartbeatTimeout - 2147483647: rc=ISMRC_OK */
    mbuf = "{ \"HighAvailability\": { \"HeartbeatTimeout\":2147483647, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    /* 39. Test HeartbeatTimeout - 0: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"HeartbeatTimeout\":0, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 40. Test HeartbeatTimeout - -3: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"HeartbeatTimeout\":-33, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\",  \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 41. Test HeartbeatTimeout - 2147483648: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"HighAvailability\": { \"HeartbeatTimeout\":2147483648, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    /* 42. Test HeartbeatTimeout - "badValue": rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"HeartbeatTimeout\":\"badValue\", \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 43. Test HeartbeatTimeout - true: rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"HeartbeatTimeout\":true, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 44. Test StartupMode - 2035: rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"StartupMode\":2345, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    /* 45. Test StartupMode - true: rc=ISMRC_BadPropertyType */
    mbuf = "{ \"HighAvailability\": { \"StartupMode\":true, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"EnableHA\":false,\"Group\":\"TestGroupA\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false,\"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":true, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":true,\"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":null }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false,\"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":null }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":true, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationNIC\":\"192.168.23.5\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationNIC\":true }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationNIC\":1234 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);


    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"RemoteDiscoveryPort\":0 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"RemoteDiscoveryPort\":9999 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"RemoteDiscoveryPort\":65535 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"RemoteDiscoveryPort\":1024 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"RemoteDiscoveryPort\":1023 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"RemoteDiscoveryPort\":65536 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"RemoteDiscoveryPort\":\"xxx\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"RemoteDiscoveryPort\":true }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);


    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ReplicationPort\":0 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ReplicationPort\":9999 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ReplicationPort\":65535 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ReplicationPort\":1024 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ReplicationPort\":1023 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ReplicationPort\":65536 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ReplicationPort\":\"xxx\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ReplicationPort\":true }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);


    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationPort\":0 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationPort\":9999 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationPort\":65535 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationPort\":1024 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationPort\":1023 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationPort\":65536 }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationPort\":\"xxx\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationPort\":true }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationPort\":1024, \"UseSecuredConnections\":false }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationPort\":1024, \"UseSecuredConnections\":true }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_OK) == ISMRC_OK);

    mbuf = "{ \"HighAvailability\": { \"EnableHA\":false, \"LocalDiscoveryNIC\":\"192.168.23.5\", \"LocalReplicationNIC\":\"192.168.23.5\", \"RemoteDiscoveryNIC\":\"192.168.23.5\", \"Group\":\"TestGroup\",\"ExternalReplicationPort\":1024, \"UseSecuredConnections\":\"true\" }}";
    CU_ASSERT(validate_HighAvailability(mbuf, ISMRC_BadPropertyType) == ISMRC_BadPropertyType);

    return;
}

