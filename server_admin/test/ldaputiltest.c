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
#include <ldaputil.h>

extern int ism_admin_ldapExtraLen(char * str, int len, int isFilterStr);
extern void ism_admin_ldapEscape(char ** new, char * from, int len, int isFilterStr);
extern int ism_security_getLDAPIdPrefix(char * idMap, char *idPrefix);
extern int ism_admin_ldapHexExtraLen(char * str, int len);
extern int ism_admin_ldapHexEscape(char ** new, char * from, int len);

/*
 * init
 */
void testLDAPUtilInit(void) {


}

void test_ism_admin_ldapHexExtraLen(void)
{
    int rc = 0;
    char *ldapStr = NULL;
    char *tstStr = NULL;
    int len = 0;
    int extra = 0;

    /* escape and match */
    ldapStr = "cn=iot,ou=groups,ou=messaging,dc=swg,dc=usma,dc=ibm,dc=com";
    tstStr = "cn=iot\\2Cou=groups\\2Cou=messaging\\2Cdc=swg\\2Cdc=usma\\2Cdc=ibm\\2Cdc=com";
    len = strlen(ldapStr);
    extra = ism_admin_ldapHexExtraLen(ldapStr, len);
    printf("\nStr: %s Extra: %d\n", ldapStr, extra);
    CU_ASSERT(extra==12);
    char *hexStr = (char *)calloc(1, (len + extra + 1));
    char *hexStrPtr = hexStr;
    ism_admin_ldapHexEscape(&hexStrPtr, ldapStr, len);
    rc = strcmp(hexStrPtr, tstStr);
    printf("HexStr: %s  len=%d\n", hexStrPtr, (int)strlen(hexStrPtr));
    printf("tstStr: %s  len=%d\n", tstStr, (int)strlen(tstStr));
    printf("rc=%d\n", rc);
    free(hexStr);
    CU_ASSERT(rc == 0);
}


void test_ism_admin_ldapExtraLen(void)
{
	char * ldapStr = "test,user";
	int extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	CU_ASSERT(extra==1);
	
	ldapStr = "test,\"user";
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	printf("\nStr: %s. Extra: %d\n", ldapStr, extra);
	CU_ASSERT(extra==2);
	
	ldapStr = "test,\"user\\";
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0);
	printf("\nStr: %s. Extra: %d\n", ldapStr, extra);
	CU_ASSERT(extra==3);
	
	ldapStr = "test,\"user\\#";
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0);
	printf("\nStr: %s. Extra: %d\n", ldapStr, extra);
	CU_ASSERT(extra==4);
	
	ldapStr = "test,\"user\\#+";
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0);
	printf("\nStr: %s. Extra: %d\n", ldapStr, extra);
	CU_ASSERT(extra==5);
	
	ldapStr = "test,\"user\\#+<";
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0);
	printf("\nStr: %s. Extra: %d\n", ldapStr, extra);
	CU_ASSERT(extra==6);
	
	ldapStr = "test,\"user\\#+<>";
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0);
	printf("\nStr: %s. Extra: %d\n", ldapStr, extra);
	CU_ASSERT(extra==7);
	
	ldapStr = "test,\"user\\#+<>;";
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0);
	printf("\nStr: %s. Extra: %d\n", ldapStr, extra);
	CU_ASSERT(extra==8);
	
	ldapStr = "test,\"user\\#+<>; ";
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0);
	printf("\nStr: %s. Extra: %d\n", ldapStr, extra);
	CU_ASSERT(extra==9);
}

void test_ism_admin_ldapExtra(void)
{
	char * ldapStr = "test,user";
	char * ldapStrExpected = "test\\,user";
	char * ldapStrOut = NULL;
	int extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	int newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==1);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	
	ldapStr = "test,\"user";
	ldapStrExpected = "test\\,\\\"user";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==2);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	
	ldapStr = "test,\"user\\";
	ldapStrExpected = "test\\,\\\"user\\\\";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==3);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	ldapStr = "test,\"user\\#";
	ldapStrExpected = "test\\,\\\"user\\\\\\#";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==4);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	
	ldapStr = "test,\"user\\#+";
	ldapStrExpected = "test\\,\\\"user\\\\\\#\\+";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==5);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	ldapStr = "test,\"user\\#+<";
	ldapStrExpected = "test\\,\\\"user\\\\\\#\\+\\<";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==6);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	ldapStr = "test,\"user\\#+<>";
	ldapStrExpected = "test\\,\\\"user\\\\\\#\\+\\<\\>";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==7);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	ldapStr = "test,\"user\\#+<>;";
	ldapStrExpected = "test\\,\\\"user\\\\\\#\\+\\<\\>\\;";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==8);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	ldapStr = "test,\"user\\#+<>; ";
	ldapStrExpected = "test\\,\\\"user\\\\\\#\\+\\<\\>\\;\\ ";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==9);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
}

void test_ism_admin_ldapExtraFilterOption(void)
{
	char * ldapStr = "test,user";
	char * ldapStrExpected = "test\\,user";
	char * ldapStrOut = NULL;
	int extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	int newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==1);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	/*ldapEscape with filter option*/
	int extraFilter = ism_admin_ldapExtraLen(ldapStrOut, strlen(ldapStrOut),1 );
	
	printf("ExtraFilter: %d\n", extraFilter);
	CU_ASSERT(extraFilter==1);
	
	int newsizeFilter = extraFilter + strlen(ldapStrOut) + 1;
	char * ldapStrFilterOut = (char *) alloca( newsizeFilter);
	char * ldapStrFilterExpected = "test\\\\,user";
	ism_admin_ldapEscape(&ldapStrFilterOut,ldapStrOut, strlen(ldapStrOut),1);
	ldapStrFilterOut[newsizeFilter-1]=0;
	CU_ASSERT(!strcmp(ldapStrFilterExpected, ldapStrFilterOut));
	
	
	ldapStr = "test,\"user";
	ldapStrExpected = "test\\,\\\"user";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==2);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	/*ldapEscape with filter option*/
	extraFilter = ism_admin_ldapExtraLen(ldapStrOut, strlen(ldapStrOut),1 );
	printf("ExtraFilter: %d\n", extraFilter);
	CU_ASSERT(extraFilter==2);
	
	newsizeFilter = extraFilter + strlen(ldapStrOut) + 1;
	ldapStrFilterOut = (char *) alloca( newsizeFilter);
	ldapStrFilterExpected =  "test\\\\,\\\\\"user";
	ism_admin_ldapEscape(&ldapStrFilterOut,ldapStrOut, strlen(ldapStrOut),1);
	ldapStrFilterOut[newsizeFilter-1]=0;
	CU_ASSERT(!strcmp(ldapStrFilterExpected, ldapStrFilterOut));
	
	
	
	
	ldapStr = "test,\"user\\";
	ldapStrExpected = "test\\,\\\"user\\\\";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==3);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	/*ldapEscape with filter option*/
	extraFilter = ism_admin_ldapExtraLen(ldapStrOut, strlen(ldapStrOut),1 );
	printf("ExtraFilter: %d\n", extraFilter);
	CU_ASSERT(extraFilter==4);
	
	newsizeFilter = extraFilter + strlen(ldapStrOut) + 1;
	ldapStrFilterOut = (char *) alloca( newsizeFilter);
	ldapStrFilterExpected =  "test\\\\,\\\\\"user\\\\\\\\";
	ism_admin_ldapEscape(&ldapStrFilterOut,ldapStrOut, strlen(ldapStrOut),1);
	ldapStrFilterOut[newsizeFilter-1]=0;
	printf("ldapStrFilterExpected: %s\n",ldapStrFilterOut );
	CU_ASSERT(!strcmp(ldapStrFilterExpected, ldapStrFilterOut));
	
	
	ldapStr = "test,\"user\\#";
	ldapStrExpected = "test\\,\\\"user\\\\\\#";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==4);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	/*ldapEscape with filter option*/
	extraFilter = ism_admin_ldapExtraLen(ldapStrOut, strlen(ldapStrOut),1 );
	printf("ExtraFilter: %d\n", extraFilter);
	CU_ASSERT(extraFilter==5);
	
	newsizeFilter = extraFilter + strlen(ldapStrOut) + 1;
	ldapStrFilterOut = (char *) alloca( newsizeFilter);
	ldapStrFilterExpected =  "test\\\\,\\\\\"user\\\\\\\\\\\\#";
	ism_admin_ldapEscape(&ldapStrFilterOut,ldapStrOut, strlen(ldapStrOut),1);
	ldapStrFilterOut[newsizeFilter-1]=0;
	printf("ldapStrFilterExpected: %s\n",ldapStrFilterOut );
	CU_ASSERT(!strcmp(ldapStrFilterExpected, ldapStrFilterOut));
	
	
	
	ldapStr = "test,\"user\\#+";
	ldapStrExpected = "test\\,\\\"user\\\\\\#\\+";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==5);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	/*ldapEscape with filter option*/
	extraFilter = ism_admin_ldapExtraLen(ldapStrOut, strlen(ldapStrOut),1 );
	printf("ExtraFilter: %d\n", extraFilter);
	CU_ASSERT(extraFilter==6);
	
	newsizeFilter = extraFilter + strlen(ldapStrOut) + 1;
	ldapStrFilterOut = (char *) alloca( newsizeFilter);
	ldapStrFilterExpected =  "test\\\\,\\\\\"user\\\\\\\\\\\\#\\\\+";
	ism_admin_ldapEscape(&ldapStrFilterOut,ldapStrOut, strlen(ldapStrOut),1);
	ldapStrFilterOut[newsizeFilter-1]=0;
	printf("ldapStrFilterExpected: %s\n",ldapStrFilterOut );
	CU_ASSERT(!strcmp(ldapStrFilterExpected, ldapStrFilterOut));
	
	
	ldapStr = "test,\"user\\#+<";
	ldapStrExpected = "test\\,\\\"user\\\\\\#\\+\\<";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==6);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	/*ldapEscape with filter option*/
	extraFilter = ism_admin_ldapExtraLen(ldapStrOut, strlen(ldapStrOut),1 );
	printf("ExtraFilter: %d\n", extraFilter);
	CU_ASSERT(extraFilter==7);
	
	newsizeFilter = extraFilter + strlen(ldapStrOut) + 1;
	ldapStrFilterOut = (char *) alloca( newsizeFilter);
	ldapStrFilterExpected =  "test\\\\,\\\\\"user\\\\\\\\\\\\#\\\\+\\\\<";
	ism_admin_ldapEscape(&ldapStrFilterOut,ldapStrOut, strlen(ldapStrOut),1);
	ldapStrFilterOut[newsizeFilter-1]=0;
	printf("ldapStrFilterExpected: %s\n",ldapStrFilterOut );
	CU_ASSERT(!strcmp(ldapStrFilterExpected, ldapStrFilterOut));
	
	
	ldapStr = "test,\"user\\#+<>";
	ldapStrExpected = "test\\,\\\"user\\\\\\#\\+\\<\\>";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==7);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	/*ldapEscape with filter option*/
	extraFilter = ism_admin_ldapExtraLen(ldapStrOut, strlen(ldapStrOut),1 );
	printf("ExtraFilter: %d\n", extraFilter);
	CU_ASSERT(extraFilter==8);
	
	newsizeFilter = extraFilter + strlen(ldapStrOut) + 1;
	ldapStrFilterOut = (char *) alloca( newsizeFilter);
	ldapStrFilterExpected =  "test\\\\,\\\\\"user\\\\\\\\\\\\#\\\\+\\\\<\\\\>";
	ism_admin_ldapEscape(&ldapStrFilterOut,ldapStrOut, strlen(ldapStrOut),1);
	ldapStrFilterOut[newsizeFilter-1]=0;
	printf("ldapStrFilterExpected: %s\n",ldapStrFilterOut );
	CU_ASSERT(!strcmp(ldapStrFilterExpected, ldapStrFilterOut));
	
	
	ldapStr = "test,\"user\\#+<>;";
	ldapStrExpected = "test\\,\\\"user\\\\\\#\\+\\<\\>\\;";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==8);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	/*ldapEscape with filter option*/
	extraFilter = ism_admin_ldapExtraLen(ldapStrOut, strlen(ldapStrOut),1 );
	printf("ExtraFilter: %d\n", extraFilter);
	CU_ASSERT(extraFilter==9);
	
	newsizeFilter = extraFilter + strlen(ldapStrOut) + 1;
	ldapStrFilterOut = (char *) alloca( newsizeFilter);
	ldapStrFilterExpected =  "test\\\\,\\\\\"user\\\\\\\\\\\\#\\\\+\\\\<\\\\>\\\\;";
	ism_admin_ldapEscape(&ldapStrFilterOut,ldapStrOut, strlen(ldapStrOut),1);
	ldapStrFilterOut[newsizeFilter-1]=0;
	printf("ldapStrFilterExpected: %s\n",ldapStrFilterOut );
	CU_ASSERT(!strcmp(ldapStrFilterExpected, ldapStrFilterOut));
	
	
	ldapStr = "test,\"user\\#+<>; ";
	ldapStrExpected = "test\\,\\\"user\\\\\\#\\+\\<\\>\\;\\ ";
	ldapStrOut = NULL;
	extra = ism_admin_ldapExtraLen(ldapStr, strlen(ldapStr),0 );
	newsize = strlen(ldapStr)+ extra+1;
	ldapStrOut = (char *)alloca(newsize);
	ism_admin_ldapEscape(&ldapStrOut,ldapStr, strlen(ldapStr),0);
	ldapStrOut[newsize-1]=0;
	printf("\nEscaped: %s\n", ldapStrOut); 
	CU_ASSERT(extra==9);
	CU_ASSERT(!strcmp(ldapStrExpected, ldapStrOut));
	
	/*ldapEscape with filter option*/
	extraFilter = ism_admin_ldapExtraLen(ldapStrOut, strlen(ldapStrOut),1 );
	printf("ExtraFilter: %d\n", extraFilter);
	CU_ASSERT(extraFilter==10);
	
	newsizeFilter = extraFilter + strlen(ldapStrOut) + 1;
	ldapStrFilterOut = (char *) alloca( newsizeFilter);
	ldapStrFilterExpected =  "test\\\\,\\\\\"user\\\\\\\\\\\\#\\\\+\\\\<\\\\>\\\\;\\\\ ";
	ism_admin_ldapEscape(&ldapStrFilterOut,ldapStrOut, strlen(ldapStrOut),1);
	ldapStrFilterOut[newsizeFilter-1]=0;
	printf("ldapStrFilterExpected: %s\n",ldapStrFilterOut );
	CU_ASSERT(!strcmp(ldapStrFilterExpected, ldapStrFilterOut));
	
}

void test_ism_security_getLDAPIdPrefix(void)
{
	char * idMap = "*:uid";
	char * idExpected = "uid";
	char * idResult = alloca(1024);
	ism_security_getLDAPIdPrefix(idMap, idResult);
	CU_ASSERT(!strcmp(idExpected, idResult));
	
	idMap = "uid";
	idExpected = "uid";
	idResult = alloca(1024);
	ism_security_getLDAPIdPrefix(idMap, idResult);
	CU_ASSERT(!strcmp(idExpected, idResult));
	
}
