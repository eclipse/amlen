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

int qmctestno = 0;

/*
 * Fake callback for transport
 */
int qpQMCSecurityCallback(
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
static int32_t validate_QueueManagerConnection(
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
    int rc = ism_config_validate_QueueManagerConnection(post, mergedObj, "QueueManagerConnection", (char *)name, CREATE, props);
    qmctestno += 1;
    if (exprc != rc)
    {
        printf("Test %2d Failed: buf=\"%s\" exprc=%d rc=%d\n", qmctestno,
            mbuf ? mbuf : "NULL", exprc, rc);
    }

    if ( props ) ism_common_freeProperties(props);
    if ( post ) json_decref(post);
    if ( mobj ) json_decref(mobj);

    return rc;
}




/* Cunit test for validating data type number using ism_config_validateDataType_number() API */
void test_validate_configObject_QueueManagerConnection(void)
{
    int rc = ISMRC_OK;
    ism_config_t *handle;

    ism_common_initUtil();
    // ism_common_setTraceLevel(9);

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("AdminEndpoint INIT: rc: %d\n", rc);
    CU_ASSERT(rc == ISMRC_OK);

    rc = ism_config_register(ISM_CONFIG_COMP_SECURITY, NULL,
        (ism_config_callback_t) qpQMCSecurityCallback, &handle);

    printf("QueuePManagerPolicy ism_config_register(): rc: %d\n", rc);

    /* Add config items required for QueuePolicy unit tests */
    char *ibuf = NULL;
    json_error_t error = {0};
    json_t *value = NULL;


    /* Add QueueManagerConnection - name: testqmc */
    ibuf = "{ \"testqmc\": { \"QueueManagerName\":\"QMN\", \"ConnectionName\":\"1.2.3.4(5)\", \"ChannelName\":\"CN\", \"SSLCipherSpec\":null }}";
    value = json_loads(ibuf, 0, &error);
//    value = json_pack("{s{ssssssss}}","testmp","Description","TestQueuePolicy","ClientID","*","Queue","*","ActionList","Send,Receive,Browse");
    if ( !value ) {
        printf("value: JSON parse error: %s%d\n", error.text, error.line);
    }
    CU_ASSERT( value != NULL );
    printf("calling ism_config_json_setComposite\n");
    rc = ism_config_json_setComposite("QueueManagerConnection", "testqmc", value);
    CU_ASSERT( rc == ISMRC_OK );


    /* Run tests */

    /* For cunit tests, create merged object. Also use merged object as current post object */
    char *mbuf = NULL;

    /* 1. Create QueueManagerConnection: rc=ISMRC_OK */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"QMN\", \"ChannelName\":\"CN\", \"ConnectionName\":\"1.2.3.4(5)\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 2. Create QueueManagerConnection: number as first char of QueueManagerName: rc=ISMRC_OK */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"123svtbridge/4567%890_managerz\", \"ChannelName\":\"CN\", \"ConnectionName\":\"1.2.3.4(5)\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 3. Create QueueManagerConnection: valid + char in QueueManagerName: rc=ISMRC_OK */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"QM+N\", \"ChannelName\":\"CN\", \"ConnectionName\":\"1.2.3.4(5)\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 4. Create QueueManagerConnection: number as first char of ChannelName: rc=ISMRC_OK */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"QMN\", \"ChannelName\":\"5CN\", \"ConnectionName\":\"1.2.3.4(5)\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 5. Create QueueManagerConnection: invalid char in ChannelName: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"QMN\", \"ChannelName\":\"C@N\", \"ConnectionName\":\"1.2.3.4(5)\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 6. Create QueueManagerConnection: invalid IP(PORT) in ConnectionName: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"QMN\", \"ChannelName\":\"CN\", \"ConnectionName\":\"1.2.3.4:5\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 7. Create QueueManagerConnection: invalid IP(PORT) in ConnectionName: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"QMN\", \"ChannelName\":\"CN\", \"ConnectionName\":\"1.2.3.4.5(5)\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 8. Create QueueManagerConnection: invalid IP(PORT) in ConnectionName: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"QMN\", \"ChannelName\":\"CN\", \"ConnectionName\":\"[1.2.3.4]\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 9. Create QueueManagerConnection: invalid IP(PORT) in ConnectionName: rc=ISMRC_BadPropertyValue */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"QMN\", \"ChannelName\":\"CN\", \"ConnectionName\":\"[1.2.3.4]:5\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_BadPropertyValue) == ISMRC_BadPropertyValue );

    /* 10. Create QueueManagerConnection: valid IP(PORT) in ConnectionName: rc=ISMRC_OK */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"QMN\", \"ChannelName\":\"CN\", \"ConnectionName\":\"[fe80::a00:27ff:fec3:36f0](5)\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 11. Create QueueManagerConnection: valid IP(PORT) in ConnectionName: rc=ISMRC_OK */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"QMN\", \"ChannelName\":\"CN\", \"ConnectionName\":\"[fe80::a00:27ff:fec3:36f0]\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_OK) == ISMRC_OK );

    /* 12. Create QueueManagerConnection: valid IP(PORT) in ConnectionName: rc=ISMRC_OK */
    mbuf = "{ \"TestQMC1\": { \"QueueManagerName\":\"QMN\", \"ChannelName\":\"CN\", \"ConnectionName\":\"11.22.33.44\" }}";
    CU_ASSERT( validate_QueueManagerConnection(mbuf, ISMRC_OK) == ISMRC_OK );

    return;
}

