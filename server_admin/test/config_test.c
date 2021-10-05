/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
 * File: config_test.c
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
#include <ismjson.h>
#include <config.h>

extern int32_t ism_config_set_dynamic(ism_json_parse_t *json);
void testconfigString(void);
void testconfigInt(void);
int transportCallback(char *object, char *name, ism_prop_t *props, ism_ConfigChangeType_t  flag );

/*
 * init
 */
void testconfigInit(void) {

    ism_config_t *hdl;

    /*
     * Initialize the utilities
     */
    setenv("ISMSSN", "SSN4BVT", 1);
    ism_common_initUtil();

    int rc  = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("ism_admin_init: Ignore rc=113 error for CUNIT test. rc=%d\n", rc);
    CU_ASSERT( rc == 0 );

    hdl=ism_config_getHandle(ISM_CONFIG_COMP_SERVER, NULL);
    CU_ASSERT_PTR_NOT_NULL(hdl);
    
    ism_field_t f;
    f.type = VT_String;
    f.val.s = "WARN";
    ism_common_setProperty( ism_common_getConfigProperties(), "LogLevel", &f);

    ism_prop_t *prop = ism_config_getProperties(hdl, NULL, "LogLevel");
    CU_ASSERT_PTR_NOT_NULL(prop);
    ism_field_t val;
    ism_common_getProperty(prop, "LogLevel", &val);
    printf("LogLevel: %s\n",val.val.s);
    CU_ASSERT_STRING_EQUAL(val.val.s, "WARN");

    char buf[1024];
    int buflen = 1024;
    concat_alloc_t outbuf = {0};
    outbuf.buf = buf;
    outbuf.len = buflen;
    memset(outbuf.buf, 0, buflen);

    ism_http_t *http;
    http = alloca(sizeof(ism_http_t));
    memset(http->version, 0, 16);
    memcpy(http->version, "1.3.1.Beta", 10);
    http->outbuf = outbuf;

    rc = ism_config_get_singletonObject("Server", "LogLevel", http);
    CU_ASSERT( rc == 0 );
    printf("RESTAPI OUTPUT: %s\n", http->outbuf.buf);
}

void testconfigSetGet(void) {
    char buf[512];
    ism_json_parse_t configobj;
    char svchar;
    int buflen = 0;
    ism_config_t *thandle;
    ism_config_t *shandle;
    ism_prop_t *props = NULL;

    ism_common_initUtil();
    // Generate a per-user UUID string if no ISMSSN file exists on the machine
    setenv("ISMSSN", "SSN4BVT", 1);

    int rc  = ism_admin_init(ISM_PROTYPE_SERVER);
    printf("ism_admin_init: Ignore rc=113 error for CUNIT test. rc=%d\n", rc);
    CU_ASSERT( rc == 0 );


    /*Create Message Hub*/
    sprintf(buf, "{ \"Action\":\"Set\",\"Component\":\"Transport\",\"User\":\"admin\",\"Type\":\"Composite\",\"Item\": \"MessageHub\",\"Name\":\"MH1\" }");

    memset(&configobj, 0, sizeof(configobj));
    buflen = strlen(buf);
    svchar = buf[buflen];
    ((char *)buf)[buflen] = 0;
    ((char *)buf)[buflen] = svchar;

    configobj.src_len = buflen;
    configobj.source = malloc(buflen+1);
    memcpy(configobj.source, buf, buflen);
    configobj.source[buflen] = 0;   /* null terminate for debug */
    ism_json_parse(&configobj);
    printf("ism_json_parse: rc=%d\n", configobj.rc);
    CU_ASSERT(configobj.rc == 0);

    int regresult= ism_config_register(ISM_CONFIG_COMP_TRANSPORT, NULL, transportCallback, &thandle);
    if ( regresult != 0 ) {
        thandle = ism_config_getHandle(ISM_CONFIG_COMP_TRANSPORT, NULL);
    }

    int setdynamic = ism_config_set_dynamic(&configobj);
    printf("Message Hub Set Dynamic Value: rc=%d\n", setdynamic);
    CU_ASSERT(setdynamic== 0);

    /*check UID*/
    props = ism_config_getProperties(thandle, "MessageHub", "MH1");
    CU_ASSERT_PTR_NOT_NULL(props);


    /*Create Connection Policy*/
    sprintf(buf, "{ \"Action\":\"Set\",\"Component\":\"Security\",\"User\":\"admin\",\"Type\":\"Composite\",\"Item\": \"ConnectionPolicy\",\"Name\":\"TestCP1\",\"Protocol\":\"JMS,MQTT\" }");

    memset(&configobj, 0, sizeof(configobj));
    buflen = strlen(buf);
    svchar = buf[buflen];
    ((char *)buf)[buflen] = 0;
    ((char *)buf)[buflen] = svchar;

    configobj.src_len = buflen;
    configobj.source = malloc(buflen+1);
    memcpy(configobj.source, buf, buflen);
    configobj.source[buflen] = 0;   /* null terminate for debug */
    ism_json_parse(&configobj);
    printf("ism_json_parse return %d\n", configobj.rc);
    CU_ASSERT(configobj.rc == 0);
    regresult= ism_config_register(ISM_CONFIG_COMP_SECURITY, "ConnectionPolicy", transportCallback, &shandle);
    if (regresult != 0 ) {
        shandle = ism_config_getHandle(ISM_CONFIG_COMP_SECURITY, NULL);
    }

    setdynamic = ism_config_set_dynamic(&configobj);
    printf("ConnectionPolicy:  Set Dynamic Value: %d\n", setdynamic);
    CU_ASSERT(setdynamic== 0);
    
    props = ism_config_getProperties(shandle, "ConnectionPolicy", "TestCP1");
    CU_ASSERT_PTR_NOT_NULL(props);

}

/*
 * Transport.Port.0.Enabled = true
 */
void testconfigRegistration(void) {

/*
    concat_alloc_t output_buffer = {buf, sizeof(buf), 0, 0 };
    ism_config_t *handle;

    output_buffer.used = 0;
    CU_ASSERT_PTR_NOT_NULL(ism_config_getHandle("Transport"));
    CU_ASSERT(ism_config_set(handle, ISM_CONFIG_DATA_BOOL, "Transport.Listener.0.Enabled", &trl, 0) == 0);
    CU_ASSERT(ism_config_getBoolean(ism_config_getHandle("Transport"), "Transport.Listener.0.Enabled", &retval) == 0);
    CU_ASSERT_EQUAL(retval, trl);*/
}

int transportCallback(char *object, char *name, ism_prop_t *props, ism_ConfigChangeType_t  flag ) {
    printf("the call back has been called\n");
    return 0;
}


/* Test for ism_config_getCompType() API */
void testConfigGetComponent(void) {

    CU_ASSERT(ism_config_getCompType("Server") == ISM_CONFIG_COMP_SERVER);
    CU_ASSERT(ism_config_getCompType("Transport") == ISM_CONFIG_COMP_TRANSPORT);
    CU_ASSERT(ism_config_getCompType("Protocol") == ISM_CONFIG_COMP_PROTOCOL);
    CU_ASSERT(ism_config_getCompType("Engine") == ISM_CONFIG_COMP_ENGINE);
    CU_ASSERT(ism_config_getCompType("Security") == ISM_CONFIG_COMP_SECURITY);
    CU_ASSERT(ism_config_getCompType("Admin") == ISM_CONFIG_COMP_ADMIN);
    CU_ASSERT(ism_config_getCompType("Monitoring") == ISM_CONFIG_COMP_MONITORING);
    CU_ASSERT(ism_config_getCompType("Store") == ISM_CONFIG_COMP_STORE);
    CU_ASSERT(ism_config_getCompType("MQConnectivity") == ISM_CONFIG_COMP_MQCONNECTIVITY);
    return;
}


/* Test ism_config_getComponentProps() */
void testgetComponentProps(void) {

    ism_config_t *hdl;

    setenv("ISMSSN", "SSN4BVT", 1);
    ism_common_initUtil();
    ism_admin_init(ISM_PROTYPE_SERVER);

    hdl=ism_config_getHandle(ISM_CONFIG_COMP_SERVER, NULL);
    CU_ASSERT_PTR_NOT_NULL(hdl);
    ism_field_t f;
    f.type = VT_String;
       f.val.s = "WARN";
    ism_common_setProperty( ism_common_getConfigProperties(), "LogLevel", &f);
    ism_prop_t *cfghdl = ism_config_getComponentProps(ISM_CONFIG_COMP_SERVER);
    CU_ASSERT_PTR_NOT_NULL(cfghdl);

    return;
}

/* Test ism_admin_setAdminMode() */
void testSetAdminMode(void) {
    ism_config_t *handle;
    ism_config_t *hdl;
    ism_prop_t *prop = NULL;

    setenv("ISMSSN", "SSN4BVT", 1);
    ism_common_initUtil();
    ism_admin_init(ISM_PROTYPE_SERVER);
    CU_ASSERT(ism_config_register(ISM_CONFIG_COMP_SERVER, NULL, NULL, &handle)==0);
    CU_ASSERT_PTR_NOT_NULL(handle);
    hdl=ism_config_getHandle(ISM_CONFIG_COMP_SERVER, NULL);
    CU_ASSERT_PTR_NOT_NULL(hdl);

    /* set Admin mode to production, mode = 0 */
    int rc = ism_admin_setAdminMode(0, 0);
    CU_ASSERT( rc == ISMRC_OK );
    prop = ism_config_getProperties(hdl, NULL, "AdminMode");
    CU_ASSERT_PTR_NOT_NULL(prop);
    // ism_common_getProperty(prop, "AdminMode", &val);
    // CU_ASSERT_STRING_EQUAL(val.val.i, 0);

    /* set Admin mode to maintenance, mode = 1 */
    rc = ism_admin_setAdminMode(1, 0);
    CU_ASSERT( rc == ISMRC_OK );
    prop = ism_config_getProperties(hdl, NULL, "AdminMode");
    CU_ASSERT_PTR_NOT_NULL(prop);
    // ism_common_getProperty(prop, "AdminMode", &val);
    // CU_ASSERT_STRING_EQUAL(val.val.i, 1);

    /* set Admin mode to maintenance, mode = 1 with error code 210 */
    rc = ism_admin_setAdminMode(1, 200);
    CU_ASSERT( rc == ISMRC_OK );
    prop = ism_config_getProperties(hdl, NULL, "AdminMode");
    CU_ASSERT_PTR_NOT_NULL(prop);
    // ism_common_getProperty(prop, "AdminMode", &val);
    // CU_ASSERT_STRING_EQUAL(val.val.i, 1);
    prop = ism_config_getProperties(hdl, NULL, "AdminErrorCode");
    CU_ASSERT_PTR_NOT_NULL(prop);
    // ism_common_getProperty(prop, "AdminErrorCode", &val);
    // CU_ASSERT_STRING_EQUAL(val.val.i, 200);


    return;
}

/* Test ism_admin_setAdminMode() */
void testupdateProtocolCapabilities(void) {

	int rc = ism_admin_updateProtocolCapabilities("mqtt", 0x01);
	printf ("updateProtocolCapabilities:0x01 RC: %d\n", rc);

	int cap = ism_admin_getProtocolCapabilities("mqtt");
	CU_ASSERT( rc == ISMRC_OK );
	printf ("getProtocolCapabilities:0x01 result: %d\n", cap);
	printf ("AND BITWISE:0x01  %d\n", cap & 0x01);
	CU_ASSERT( (cap & 0x01) > 0 );

	rc = ism_admin_updateProtocolCapabilities("mqtt", 0x08);
	CU_ASSERT( rc == ISMRC_OK );
	printf ("updateProtocolCapabilities:0x08 RC: %d\n", rc);

	cap = ism_admin_getProtocolCapabilities("mqtt");
	printf ("getProtocolCapabilities:0x08 result: %d\n", cap);
	printf ("AND BITWISE:0x08 %d\n", cap & 0x08);
	CU_ASSERT( (cap & 0x08) > 0 );

	rc = ism_admin_updateProtocolCapabilities("mqtt", 0x02);
	CU_ASSERT( rc == ISMRC_OK );
	printf ("updateProtocolCapabilities:0x02 RC: %d\n", rc);

	cap = ism_admin_getProtocolCapabilities("mqtt");
	printf ("getProtocolCapabilities:0x02 result: %d\n", cap);
	printf ("AND BITWISE:0x02 %d\n", cap & 0x02);
	printf ("AND BITWISE:0x04 %d\n", cap & 0x04);
	CU_ASSERT( (cap & 0x02) > 0 );
	CU_ASSERT( (cap & 0x04) == 0 );

    return;
}

