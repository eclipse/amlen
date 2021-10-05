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

#include <ismc_p.h>
#include <select_unit_test.h>
int verbose = 0;

XAPI void ism_common_setSelectorDebug(int val);

/*
 Array that carries all selector unit tests for APIs to CUnit framework.
 */
CU_TestInfo ISM_select_tests[] = {
    { "--- Testing constants                  ---", test_const },
    { "--- Testing expressions                ---", test_expression },
    { "--- Testing compile errors             ---", test_bad_compile },
    { "--- Testing selection                  ---", test_selection },
    CU_TEST_INFO_NULL
};

#define  COMPILE(s) f_compile(buf, (int)sizeof(buf), (s));

/*
 * Compile a selection rule
 */
static ismRule_t * saverule = NULL;

static int f_compile(char * buf, int bufsize, const char * ruledef) {
	ismRule_t * rule;
	int       rc;
//	concat_alloc_t zbuf = {0};
//	char xbuf[2048];
//	printf("compile: %s\n", ruledef);

	if (saverule) {
		ism_common_freeSelectRule(saverule);
		saverule = NULL;
	}
	rc = ism_common_compileSelectRule(&rule, NULL, ruledef);
	if (rc == 0) {
		saverule = rule;
		rc = ism_common_dumpSelectRule(rule, buf, bufsize);
		if (rc >= 0) 
			rc = 0;
		if (verbose) {
		    printf("rule = [%s]\n", ruledef);
		//    zbuf.buf = xbuf;
		//    zbuf.len = sizeof xbuf;
		//    ism_common_toStringSelectRule(rule, &zbuf);
		//    printf("out  = [%s]\n\n", zbuf.buf);
		}
	} else {
		*buf = 0;
	}
	return rc;
}


/*
 * Test constants
 */
void test_const(void) {
	char buf[8192];
	int  rc;

	 /* Check boolean constants */
	rc = COMPILE("true");
	CU_ASSERT(rc == 0);
	CU_ASSERT(strstr(buf, "Boolean True\n") != NULL);

	rc = COMPILE("TRUE");
    CU_ASSERT(rc == 0);
	CU_ASSERT(strstr(buf, "Boolean True\n") != NULL);

	rc = COMPILE("  false");
    CU_ASSERT(rc == 0);
	CU_ASSERT(strstr(buf, "Boolean False\n") != NULL);

	rc = COMPILE("\tFALSE");
    CU_ASSERT(rc == 0);
	CU_ASSERT(strstr(buf, "Boolean False\n") != NULL);

	rc = COMPILE("faLse");
    CU_ASSERT(rc == 0);
	CU_ASSERT(strstr(buf, "Boolean False\n") != NULL);

	rc = COMPILE("faLse");
    CU_ASSERT(rc == 0);
	CU_ASSERT(strstr(buf, "Boolean False\n") != NULL);

	rc = COMPILE("faLse");
    CU_ASSERT(rc == 0);
	CU_ASSERT(strstr(buf, "Boolean False\n") != NULL);

	rc = COMPILE("tRuE");
    CU_ASSERT(rc == 0);
	CU_ASSERT(strstr(buf, "Boolean True\n") != NULL);

	rc = COMPILE("\t1 > 0\n");
    CU_ASSERT(rc == 0);
	CU_ASSERT(strstr(buf, "Int 1\n") != NULL);


	rc = COMPILE("\t\t1.0 > 0x22");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Double 1.0\n") != NULL);
    CU_ASSERT(strstr(buf, "Int 34\n") != NULL);

	rc = COMPILE("\r\n1L > 0.0f");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Long 1\n") != NULL);
    CU_ASSERT(strstr(buf, "Float 0.0\n") != NULL);

	rc = COMPILE("1234567890123456 > 0L");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Long 1234567890123456\n") != NULL);

	rc = COMPILE("0 < 314.0E-2");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Double 3.14\n") != NULL);

	rc = COMPILE("555E60 > -55E+60");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Double 5.55e") != NULL);
    CU_ASSERT(strstr(buf, "Double -5.5e") != NULL);

	rc = COMPILE("'abc' = def");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "String abc\n") != NULL);
    CU_ASSERT(strstr(buf, "Var def\n") != NULL);

	rc = COMPILE("'abc''def' = def");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "String abc'def\n") != NULL);
}

/*
 * Test expressions
 */
void test_expression(void) {
	char buf[8192];
	int  rc;

    rc = COMPILE("abc");    /* A variable by itself is a valid expression since it might be a boolean */
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Var abc\n") >= 0);

    rc = COMPILE("abc\f>\t5");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Compare GT") >= 0);

    rc = COMPILE("abc\n<>5");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Compare NE") >= 0);

    rc = COMPILE("abc=\r5");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Compare EQ") >= 0);

    rc = COMPILE("abc\t< 5");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Compare LT") >= 0);

    rc = COMPILE("abc > 5");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Compare GT") >= 0);

    rc = COMPILE("abc >=\t5\t");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Compare GE") >= 0);

    rc = COMPILE("abc <=5");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Compare LE") >= 0);

    rc = COMPILE("abc in ('def', 'ghi') Or def LiKE 'a%b'");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "In def ghi\n") >= 0);
    CU_ASSERT(strstr(buf, "Like a%b") >= 0);
    CU_ASSERT(strstr(buf, "Or\n") >= 0);

    rc = COMPILE("abc not in ('def', 'ghi') Or def LiKE 'a%b'");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "In def ghi\n") >= 0);
    CU_ASSERT(strstr(buf, "Like a%b") >= 0);
    CU_ASSERT(strstr(buf, "Not\n") >= 0);

    rc = COMPILE("def between 3 and 5");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Between") >= 0);

    rc = COMPILE("abc Between (3+5.0) and 47E+0");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Between") >= 0);
    CU_ASSERT(strstr(buf, "Calc add") >= 0);

    rc = COMPILE("abc NOT beTween 1L and (3.0*5f)");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Between") >= 0);
    CU_ASSERT(strstr(buf, "Calc multiply") >= 0);
    CU_ASSERT(strstr(buf, "Not") >= 0);

    rc = COMPILE("abc + 5 between def*3 and def*4");

    CU_ASSERT(rc == 0);
    rc = COMPILE("abc noT LIKE 'abc' esCape '$'");
    CU_ASSERT(strstr(buf, "Like abc") >= 0);

    rc = COMPILE("not (abc not Like 'abc') aNd abc > ((003-4+005-6)/02.0+(14))");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Not\nNot") >= 0);
    CU_ASSERT(strstr(buf, "Int 3\nInt 4\nCalc subtract\nInt 5\n") >= 0);
    CU_ASSERT(strstr(buf, "Calc divide\nInt 14\nCalc add\n") >= 0);

    rc = COMPILE("(((((true)))))");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Boolean True\n") >= 0);

    rc = COMPILE("true or (false and false)");
    CU_ASSERT(rc == 0);
    CU_ASSERT(strstr(buf, "Boolean True\n") >= 0);

}

/*
 * Test bad compiles
 */
void test_bad_compile(void) {
	int   rc;
	char buf[8192];

	rc = COMPILE("3");
	CU_ASSERT(rc == ISMRC_NotBoolean);

	rc = COMPILE("3+5");
	CU_ASSERT(rc == ISMRC_NotBoolean);

	rc = COMPILE("(true");
	CU_ASSERT(rc == ISMRC_TooManyLeftParen);

	rc = COMPILE("true)");
	CU_ASSERT(rc == ISMRC_TooManyRightParen);

	rc = COMPILE("((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((");
	CU_ASSERT(rc == ISMRC_TooComplex);

	rc = COMPILE("abc is fred");
	CU_ASSERT(rc == ISMRC_IsNotValid);

	rc = COMPILE("abc is not fred");
	CU_ASSERT(rc == ISMRC_IsNotValid);

	rc = COMPILE("abc in 'abc'");
	CU_ASSERT(rc == ISMRC_InRequiresGroup);

	rc = COMPILE("abc in ()");
	CU_ASSERT(rc == ISMRC_InGroupNotValid);

	rc = COMPILE("abc in ('def' 'ghi')");
	CU_ASSERT(rc == ISMRC_InSeparator);

	rc = COMPILE("'abc' like 'a*c'");
	CU_ASSERT(rc == ISMRC_OpRequiresID);

	rc = COMPILE("abc like def");
	CU_ASSERT(rc == ISMRC_LikeSyntax);

	rc = COMPILE("abc like 'a%c' escape '$j$'");
	CU_ASSERT(rc == ISMRC_EscapeNotValid);

	rc = COMPILE("abc like 'a%c' escape true");
	CU_ASSERT(rc == ISMRC_EscapeNotValid);

	rc = COMPILE("abc like 'a%c' escape");
	CU_ASSERT(rc == ISMRC_EscapeNotValid);

	rc = COMPILE("abc +");
	CU_ASSERT(rc == ISMRC_OperandMissing);

	rc = COMPILE("abc or");
	CU_ASSERT(rc == ISMRC_OperandMissing);

	rc = COMPILE("3 + true");
	CU_ASSERT(rc == ISMRC_NotBoolean);

	rc = COMPILE("true + 3");
	CU_ASSERT(rc == ISMRC_OpNotBoolean);

	rc = COMPILE("abc = 3ab");
	CU_ASSERT(rc == ISMRC_OperandNotValid);

	rc = COMPILE("3 or true");
	CU_ASSERT(rc == ISMRC_OpNotNumeric);

	rc = COMPILE("true or 3");
	CU_ASSERT(rc == ISMRC_NotBoolean);

	rc = COMPILE("abc > true");;
	CU_ASSERT(rc == ISMRC_OpNoString);

	rc = COMPILE("true > abc");
	CU_ASSERT(rc == ISMRC_OpNoString);

	rc = COMPILE("'abc' > abc");
	CU_ASSERT(rc == ISMRC_OpNoString);

	rc = COMPILE("abc > 'abc'");
	CU_ASSERT(rc == ISMRC_OpNoString);
}


/*
 * Test bad compiles
 */
void test_selection(void) {
	int   rc;
	ism_field_t field;
	char buf[8192];
	ismc_message_t * msg;
    msg = ismc_createMessage(NULL, MTYPE_Message);
    ismc_setStringProperty(msg, "abc", "ABC");
    field.type = VT_Boolean;
    field.val.i = 1;
    ismc_setProperty(msg, "btrue", &field);
    ismc_setStringProperty(msg, "myProp0", "foo");
    ismc_setIntProperty(msg, "myProp1", 1, VT_Integer);
    field.type = VT_Float;
    field.val.f = 2.0f;
    ismc_setProperty(msg, "myProp2", &field);
    field.type = VT_Integer;
    field.val.i = 3;
    ismc_setProperty(msg, "myProp3", &field);
    field.type = VT_Double;
    field.val.i = 4.0;
    ismc_setProperty(msg, "myProp4", &field);
    ismc_setIntProperty(msg, "myProp5", 5, VT_Integer);
    ismc_setIntProperty(msg, "myProp6", 6, VT_Integer);
    field.type = VT_Float;
    field.val.f = 0.0f;
    ismc_setProperty(msg, "myProp7", &field);
    ismc_setIntProperty(msg, "ival", 5, VT_Integer);

    rc = COMPILE("");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("true");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("btrue");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("false");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_FALSE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("junk");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_UNKNOWN == ismc_filterMessage(msg, saverule));

    rc = COMPILE("abc = 'ABC'");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("abc <> 'ABC'");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_FALSE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("abc = junk");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_UNKNOWN == ismc_filterMessage(msg, saverule));

    rc = COMPILE("abc in ('ABC','DEF')");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("abc like 'A%C'");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("abc not like 'A%C'");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_FALSE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("abc is not null");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("not abc is null");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("junk is null");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("ival between 5 and 9");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("ival between 9 and 11");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_FALSE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("true or (false and false)");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("(true or false) and false");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_FALSE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("(true or true) and true");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("true or (false and false)");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    rc = COMPILE("(false and false) or true");
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));

    ism_common_setSelectorDebug(1);
    rc = COMPILE("myProp0 = 'foo' AND myProp1 > 0 and myProp2 >= 2 AnD myProp3 >= 2 and myProp4 < 5 AND myProp5 <= 6 AND myProp6 <= 6 AND myProp7 <> 7");
 //   printf("xxx %s\n", buf);
    CU_ASSERT(rc == 0);
    CU_ASSERT(SELECT_TRUE == ismc_filterMessage(msg, saverule));
    ism_common_setSelectorDebug(0);
}
