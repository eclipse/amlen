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

extern int32_t ism_config_validateClientAddress(char *addr);

void testValidateAndSetInit(void) 
{


	char buf[512];
	ism_json_parse_t configobj;
	char svchar;
	int buflen = 0;

	
	/*Create Message Hub*/
	sprintf(buf, "{ \"Action\":\"Set\",\"Component\":\"Transport\",\"User\":\"admin\",\"Type\":\"Composite\",\"Item\": \"MessageHub\",\"Name\":\"ValidateMH1\" }");

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
	int setdynamic = ism_config_set_dynamic(&configobj);
	printf("Message Hub Set Dynamic Value: %d\n", setdynamic);
	//CU_ASSERT(setdynamic== 0);
	


	/*Create Connection Policy*/
	sprintf(buf, "{ \"Action\":\"Set\",\"Component\":\"Security\",\"User\":\"admin\",\"Type\":\"Composite\",\"Item\": \"ConnectionPolicy\",\"Name\":\"ValidateTestCP1\",\"Protocol\":\"JMS,MQTT\" }");

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
	
	 setdynamic = ism_config_set_dynamic(&configobj);
	
	printf("ConnectionPolicy:  Set Dynamic Value: %d\n", setdynamic);
	
	
	
}


void test_ism_config_validateClientAddress(void)
{
	int32_t rc = ism_config_validateClientAddress("127.0.0.1");
	
	CU_ASSERT(rc==1);
	
	rc = ism_config_validateClientAddress("*");
	
	CU_ASSERT(rc==1);
	
	rc = ism_config_validateClientAddress("0.0.0.0");
	
	CU_ASSERT(rc==1);
	
	rc = ism_config_validateClientAddress("[::]");
	
	CU_ASSERT(rc==1);
	
	/*Invalid IP*/
	rc = ism_config_validateClientAddress("128.0.1.x");
	
	CU_ASSERT(rc==0);
	
	/*Multiple*/
	rc = ism_config_validateClientAddress("127.0.0.1,0.0.0.0,[::]");
	
	/*Multiple - Invalid*/
	rc = ism_config_validateClientAddress("127.0.0.1,0.0.0.x,[::]");
	
	CU_ASSERT(rc==0);
	
	/*Pair*/
	rc = ism_config_validateClientAddress("0.0.0.0-127.0.0.1");
	
	CU_ASSERT(rc==1);
	
	/*Invalid Pair*/
	rc = ism_config_validateClientAddress("127.0.0.1-0.0.0.0");
	
	CU_ASSERT(rc==0);
}

