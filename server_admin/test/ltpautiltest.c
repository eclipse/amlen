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
#include <ltpautils.h>
#include <security.h>

extern void ism_ssl_init(int useFips, int useBufferPool);

/*
 * init
 */
void testLTPAUtilInit(void) {
   ism_ssl_init(0,0);
	
}

void test_ism_security_ltpaReadKeyfile(void) {
    int rc;

    ismLTPA_t * ltpakey=NULL;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
           printf( "\nCurrent working dir: %s\n", cwd);
    rc = ism_security_ltpaReadKeyfile("test/TestLTPAKey", "password", &ltpakey);
    if ( ltpakey ) ism_security_ltpaDeleteKey(ltpakey);
    printf("ltpaTest: Return RC: %d\n", rc);
    CU_ASSERT(rc == 0);
    
    /*Test with empty path*/
    
    rc = ism_security_ltpaReadKeyfile("", "password", &ltpakey);
    if ( ltpakey ) ism_security_ltpaDeleteKey(ltpakey);
    printf("ltpaTest: Return RC for empty path: %d\n", rc);
    CU_ASSERT(rc != 0);
    
    /*Test with NULL path*/
    rc = ism_security_ltpaReadKeyfile(NULL, "password", &ltpakey);
    if ( ltpakey ) ism_security_ltpaDeleteKey(ltpakey);
    printf("ltpaTest: Return RC for empty path: %d\n", rc);
    CU_ASSERT(rc != 0);
}

void test_ism_security_ltpaV2DecodeToken(void) {
    int rc;
    
    char *user=NULL;
    time_t expiration=0;
    char *realm = NULL;

    ismLTPA_t * ltpakey=NULL;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
           printf( "\nCurrent working dir: %s\n", cwd);
    rc = ism_security_ltpaReadKeyfile("test/TestLTPAKey", "password", &ltpakey);
    CU_ASSERT(ltpakey != 0);
    CU_ASSERT(rc == 0);
    
    
    FILE * tokenv2File;
    
    tokenv2File = fopen("test/TestLTPATokenV2","r"); // read mode
    CU_ASSERT_PTR_NOT_NULL_FATAL(tokenv2File);
     
    fseek(tokenv2File, 0, SEEK_END);
    long fsize = ftell(tokenv2File);
    rewind (tokenv2File);

    char * tokenBuffer = malloc(sizeof(char)*fsize);
    int tokenBufferSize = (int )fsize;
    long bread = fread(tokenBuffer, fsize, 1, tokenv2File);
    CU_ASSERT(bread == 1);

    rc = ism_security_ltpaV2DecodeToken(ltpakey, tokenBuffer, tokenBufferSize, &user, &realm, &expiration);
    printf("User: %s\n", user);
    printf("Real: %s\n", realm);
    printf("RC: %d\n", rc);
    CU_ASSERT(rc == 0);
    CU_ASSERT(!strcmp(user,"uid=u0000000,ou=users,ou=PERF,dc=ism.ibm,dc=com"));
    CU_ASSERT(!strcmp(realm,"9.3.179.80:5389"));
    
    free(tokenBuffer);
    fclose(tokenv2File);
    
     /*Test with NULL Token Buffer*/
    rc = ism_security_ltpaV1DecodeToken(ltpakey, NULL, tokenBufferSize, &user, &realm, &expiration);
    printf("RC for invalid Token: %d\n", rc);
    CU_ASSERT(rc != 0);
   
    /*Test with NULL Key*/
    rc = ism_security_ltpaV1DecodeToken(NULL, NULL, tokenBufferSize, &user, &realm, &expiration);
    printf("RC for invalid Key: %d\n", rc);
    CU_ASSERT(rc != 0);
    
    if ( ltpakey ) ism_security_ltpaDeleteKey(ltpakey);
    
}

void test_ism_security_ltpaV1DecodeToken(void) {
    int rc;
    
    char *user=NULL;
    time_t expiration=0;
    char *realm = NULL;

    ismLTPA_t * ltpakey=NULL;
    ism_ssl_init(0,0);
    
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
           printf( "\nCurrent working dir: %s\n", cwd);
    rc = ism_security_ltpaReadKeyfile("test/TestLTPAKey", "password", &ltpakey);
    CU_ASSERT(ltpakey != 0);
    CU_ASSERT(rc == 0);
    
    
    FILE * tokenv1File;
    
    tokenv1File = fopen("test/TestLTPATokenV1","r"); // read mode
    
    /*Test with Valid Key*/ 
    fseek(tokenv1File, 0, SEEK_END);
	long fsize = ftell(tokenv1File);
	rewind (tokenv1File);
	
	char * tokenBuffer = malloc(sizeof(char)*fsize);
	int tokenBufferSize = (int )fsize;
	long bread = fread(tokenBuffer, fsize, 1, tokenv1File);
	CU_ASSERT(bread == 1);
	
	rc = ism_security_ltpaV1DecodeToken(ltpakey, tokenBuffer, tokenBufferSize, &user, &realm, &expiration);
	printf("User: %s\n", user);
	printf("Real: %s\n", realm);
	printf("RC: %d\n", rc);
	CU_ASSERT(rc == 0);
	CU_ASSERT(!strcmp(user,"uid=u0000000,ou=users,ou=PERF,dc=ism.ibm,dc=com"));
	CU_ASSERT(!strcmp(realm,"9.3.179.80:5389"));
    
    free(tokenBuffer);
    fclose(tokenv1File);
    
    /*Test with NULL key*/
    rc = ism_security_ltpaV1DecodeToken(ltpakey, NULL, tokenBufferSize, &user, &realm, &expiration);
    printf("RC for invalid Key: %d\n", rc);
    CU_ASSERT(rc != 0);
    
    if ( ltpakey ) ism_security_ltpaDeleteKey(ltpakey);
    
}
