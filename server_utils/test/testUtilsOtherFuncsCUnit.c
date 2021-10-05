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
 * File: testTimerCUnit.c
 * Component: server
 * SubComponent: server_utils
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */

#include <ismutil.h>
#include <imacontent.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "testUtilsOtherFuncsCUnit.h"
void CUnit_test_ISM_findPropertyNameIndex(void);
void CUnit_test_countTokens(void);

extern int g_verbose;

/*   (programming note 1)
 * (programming note 11)
 * Array of timer tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Util_CUnit_otherFunctions[] =
  {
        { "fromToBase64Test", CUnit_test_ISM_Tracing_Base64 },
        { "content",          CUnit_test_content },
        { "test_ism_common_ifmapip", CUnit_test_ISM_ifmapip },
        { "test_ism_common_ifmapname", CUnit_test_ISM_ifmapname },
        { "test_ism_common_ifmapnameIPv6", CUnit_test_ISM_ifmapnameIPv6 },
        { "test_ism_common_ifmapipIPv6", CUnit_test_ISM_ifmapipIPv6 },
        { "test_ism_common_countTokens", CUnit_test_countTokens },
        { "test_ism_findPropertyNameIndex", CUnit_test_ISM_findPropertyNameIndex },
        CU_TEST_INFO_NULL };

/* The timer tests initialization function.
 * Creates a timer thread.
 * Returns zero on success, non-zero otherwise.
 */
int initOtherFunctionsTests(void) {

    return 0;
}

/* The timer test suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int initOtherFunctionsUtilSuite(void) {
    return 0;
}


/* The timer test suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int cleanOtherFunctionsUtilSuite(void) {

    return 0;
}

void CUnit_test_content(void) {
    char xbuf [1024];
    concat_alloc_t buf = {xbuf, sizeof xbuf};
    ism_field_t field;
    ism_field_t fout;

    field.type = VT_UInt;
    field.val.u = 99;
    ism_protocol_putObjectValue(&buf, &field);
    ism_protocol_getObjectValue(&buf, &fout);
    CU_ASSERT(fout.type == VT_UInt);
    CU_ASSERT(fout.val.u == 99);
}


/*
 * Test Base64 encoder and decoder functions (toBase64 and fromBase64)
 */
void CUnit_test_ISM_Tracing_Base64(void) {
    char inputData1[] = { 0x01, 0x02, 0x03, 0x54, 0x67, 0xBA, 0x7D };
    char inputData2[] = { 0xFF, 0xAF, 0xCC, 0x01, 0x12, 0xBA, 0x30, 0xCC,
            0x01, 0x12, 0xBA, 0x30, 0xCC, 0x01, 0x12, 0xBA,
            0x01, 0x12, 0xBA, 0x30, 0xCC, 0x01, 0x12, 0xBA,
            0x30, 0xCC, 0x01, 0x12, 0xBA, 0x30,
            0x01, 0x12, 0xBA, 0x30, 0xCC, 0x01, 0x12, 0xBA,
            0x30, 0xCC, 0x01, 0x12, 0xBA, 0x30,
            0x30, 0xCC, 0x01, 0x12, 0xBA, 0x30 };
    char buffer1[1024], buffer2[1024];
    char *out = buffer1;

    memset(buffer1, 0x00, sizeof(buffer1));
    memset(buffer2, 0x00, sizeof(buffer2));

    CU_ASSERT(ism_common_toBase64(inputData1, out, sizeof(inputData1)) == (sizeof(inputData1) + 2) / 3 * 4);
    CU_ASSERT(ism_common_fromBase64(out, buffer2, (int)strlen(out)) == sizeof(inputData1));
    CU_ASSERT(memcmp(buffer2, inputData1, sizeof(inputData1)) == 0);

    memset(buffer1, 0x00, sizeof(buffer1));
    memset(buffer2, 0x00, sizeof(buffer2));

    CU_ASSERT(ism_common_toBase64(inputData2, out, sizeof(inputData2)) == (sizeof(inputData2) + 2) / 3 * 4);
    CU_ASSERT(ism_common_fromBase64(out, buffer2, (int)strlen(out)) == sizeof(inputData2));
    CU_ASSERT(memcmp(buffer2, inputData2, sizeof(inputData2)) == 0);
}


#define IF_CONFIG_DIR "ethernet-interface/"

void CUnit_test_ISM_ifmapip(void) {
	char * ip= ism_common_ifmapip(NULL);

	CU_ASSERT(ip == NULL);

	ip= ism_common_ifmapip("eth6_0");
	if (g_verbose)
	    printf("\nIP_0: %s\n", ip);
	CU_ASSERT(!strcmp("10.10.1.125", ip));

	ip= ism_common_ifmapip("eth6_1");
	if (g_verbose)
	    printf("IP_1: %s\n", ip);
	CU_ASSERT(!strcmp("10.10.17.125", ip));

	ip= ism_common_ifmapip("eth6_2");
	if (g_verbose)
	    printf("IP_2: %s\n", ip);
	CU_ASSERT(!strcmp("10.10.19.125", ip));

	ip= ism_common_ifmapip("eth6_3");
	if (g_verbose)
	    printf("IP_3: %s\n", ip);
	CU_ASSERT(!strcmp("10.10.21.125", ip));

	/*No Index*/
	ip= ism_common_ifmapip("eth6");
    if (g_verbose)
	    printf("IP with -1: %s\n", ip);
	CU_ASSERT(!strcmp("10.10.1.125,10.10.17.125,10.10.19.125,10.10.21.125", ip));


	ip= ism_common_ifmapip("eth7_0");
    if (g_verbose)
	    printf("IP_0: %s\n", ip);
	CU_ASSERT(!strcmp("10.10.7.1", ip));

	ip= ism_common_ifmapip("eth7_1");
    if (g_verbose)
	    printf("IP_1 eth7: %s\n", ip);
	CU_ASSERT(!strcmp("10.10.7.2", ip));

	ip= ism_common_ifmapip("eth7_2");
    if (g_verbose)
	    printf("IP_2 eth7: %s\n", ip);
	CU_ASSERT(!strcmp("10.10.7.3", ip));

	ip= ism_common_ifmapip("eth7_3");
    if (g_verbose)
	    printf("IP_3 eth7: %s\n", ip);
	CU_ASSERT(!strcmp("10.10.7.4", ip));

	ip= ism_common_ifmapip("eth7_4");
    if (g_verbose)
	    printf("IP_4 eth7: %s\n", ip);
	CU_ASSERT(!strcmp("10.10.7.5", ip));

	ip= ism_common_ifmapip("eth7_5");
    if (g_verbose)
	    printf("IP_5 eth7: %s\n", ip);
	CU_ASSERT(!strcmp("10.10.7.6", ip));
}

void CUnit_test_ISM_ifmapname(void)
{
	int admin_state;
	char * ifname =  ism_common_ifmapname(NULL, NULL);
	CU_ASSERT(ifname == NULL);

	/*Not exist ip*/
	ifname =  ism_common_ifmapname("10.10.1.10", NULL);
	if (g_verbose)
	    printf("\nIFName_NotExist: %s\n", ifname);
	CU_ASSERT(ifname==NULL);

	ifname =  ism_common_ifmapname("10.10.1.1", &admin_state);
    if (g_verbose)
	    printf("\nIFName_1: %s\n", ifname);
	CU_ASSERT(!strcmp("eth1_0", ifname));
	CU_ASSERT(admin_state == 0);

	ifname =  ism_common_ifmapname("10.10.1.2", &admin_state);
    if (g_verbose)
	    printf("IFName_2: %s, state: %d\n", ifname, admin_state);
	CU_ASSERT(!strcmp("eth2_0", ifname));
	CU_ASSERT(admin_state == 1);

	ifname =  ism_common_ifmapname("10.10.1.3", &admin_state);
    if (g_verbose)
	    printf("IFName_2: %s\n", ifname);
	CU_ASSERT(!strcmp("eth3_0", ifname));
	CU_ASSERT(admin_state == 1);

	ifname =  ism_common_ifmapname("10.10.1.4", &admin_state);
    if (g_verbose)
	    printf("IFName_2: %s\n", ifname);
	CU_ASSERT(!strcmp("eth4_0", ifname));

	ifname =  ism_common_ifmapname("10.10.1.5", &admin_state);
    if (g_verbose)
	    printf("IFName_2: %s\n", ifname);
	CU_ASSERT(!strcmp("eth5_0", ifname));
	CU_ASSERT(admin_state == 1);

	ifname =  ism_common_ifmapname("10.10.7.1", &admin_state);
    if (g_verbose)
	    printf("IFName_2: %s\n", ifname);
	CU_ASSERT(!strcmp("eth7_0", ifname));
	CU_ASSERT(admin_state == 1);

	ifname =  ism_common_ifmapname("10.10.7.2", &admin_state);
    if (g_verbose)
	    printf("IFName_2: %s\n", ifname);
	CU_ASSERT(!strcmp("eth7_1", ifname));
	CU_ASSERT(admin_state == 1);

	ifname =  ism_common_ifmapname("10.10.7.3", &admin_state);
    if (g_verbose)
	    printf("IFName_2: %s\n", ifname);
	CU_ASSERT(!strcmp("eth7_2", ifname));
	CU_ASSERT(admin_state == 1);

	ifname =  ism_common_ifmapname("10.10.7.4", &admin_state);
    if (g_verbose)
	    printf("IFName_2: %s\n", ifname);
	CU_ASSERT(!strcmp("eth7_3", ifname));
	CU_ASSERT(admin_state == 1);

	ifname =  ism_common_ifmapname("10.10.7.5", &admin_state);
    if (g_verbose)
	    printf("IFName_2: %s\n", ifname);
	CU_ASSERT(!strcmp("eth7_4", ifname));
	CU_ASSERT(admin_state == 1);

	ifname =  ism_common_ifmapname("10.10.7.6", &admin_state);
    if (g_verbose)
	    printf("IFName_2: %s\n", ifname);
	CU_ASSERT(!strcmp("eth7_5", ifname));
	CU_ASSERT(admin_state == 1);


}

void CUnit_test_ISM_ifmapnameIPv6(void)
{
	int admin_state;
	char * ifname =  ism_common_ifmapname(NULL, NULL);
	CU_ASSERT(ifname == NULL);


	ifname =  ism_common_ifmapname("fc00::10:10:1:125", NULL);
	if (g_verbose)
	    printf("\nIFNameIPv6_1: %s\n", ifname);
	CU_ASSERT(!strcmp("eth6IPv6_1", ifname));

	ifname =  ism_common_ifmapname("fc00::10:10:17:125", NULL);
    if (g_verbose)
	    printf("\nIFNameIPv6_1: %s\n", ifname);
	CU_ASSERT(!strcmp("eth6IPv6_3", ifname));

	ifname =  ism_common_ifmapname("fc00::10:10:19:125", &admin_state);
    if (g_verbose)
	    printf("\nIFNameIPv6_1: %s\n", ifname);
	CU_ASSERT(!strcmp("eth6IPv6_5", ifname));
	CU_ASSERT(admin_state == 1);

	ifname =  ism_common_ifmapname("fc00::10:10:21:125", NULL);
    if (g_verbose)
	    printf("\nIFNameIPv6_1: %s\n", ifname);
	CU_ASSERT(!strcmp("eth6IPv6_7", ifname));

	/*With Brackets*/
	ifname =  ism_common_ifmapname("[fc00::10:10:1:125]", NULL);
    if (g_verbose)
	    printf("\nIFNameIPv6_1 With Brackets: %s\n", ifname);
	CU_ASSERT(!strcmp("eth6IPv6_1", ifname));

	ifname =  ism_common_ifmapname("[fc00::10:10:17:125]", NULL);
    if (g_verbose)
	    printf("\nIFNameIPv6_1 With Brackets: %s\n", ifname);
	CU_ASSERT(!strcmp("eth6IPv6_3", ifname));

	ifname =  ism_common_ifmapname("[fc00::10:10:19:125]", NULL);
    if (g_verbose)
	    printf("\nIFNameIPv6_1 With Brackets: %s\n", ifname);
	CU_ASSERT(!strcmp("eth6IPv6_5", ifname));

	ifname =  ism_common_ifmapname("[fc00::10:10:21:125]", NULL);
    if (g_verbose)
	    printf("\nIFNameIPv6_1 With Brackets: %s\n", ifname);
	CU_ASSERT(!strcmp("eth6IPv6_7", ifname));
}

void CUnit_test_ISM_ifmapipIPv6(void) {

	char * ip= ism_common_ifmapip(NULL);

	CU_ASSERT(ip == NULL);

	ip= ism_common_ifmapip("eth6IPv6_1");
    if (g_verbose)
	    printf("\nIPv6_1: %s\n", ip);
	CU_ASSERT(!strcmp("[fc00::10:10:1:125]", ip));

	ip= ism_common_ifmapip("eth6IPv6_3");
    if (g_verbose)
	    printf("\nIPv6_3: %s\n", ip);
	CU_ASSERT(!strcmp("[fc00::10:10:17:125]", ip));

	ip= ism_common_ifmapip("eth6IPv6_5");
    if (g_verbose)
	    printf("\nIPv6_5: %s\n", ip);
	CU_ASSERT(!strcmp("[fc00::10:10:19:125]", ip));

	ip= ism_common_ifmapip("eth6IPv6_7");
    if (g_verbose)
	    printf("\nIPv6_7: %s\n", ip);
	CU_ASSERT(!strcmp("[fc00::10:10:21:125]", ip));

}

void CUnit_test_ISM_findPropertyNameIndex(void) {
    const char * topic = "ABCDEFGHIJKLMNOPRSTUVWXYZ0123456789/#?!,.@${";
    const char * uid = "ASDFGHJK";
    char buf[4096];
    ism_actionbuf_t props = {buf, sizeof(buf)};
    ism_field_t f = {0};
    uint64_t currTime = ism_common_currentTimeNanos();
    ism_protocol_putNameIndex(&props, ID_Topic);
    ism_protocol_putStringLenValue(&props, topic, strlen(topic));
    ism_protocol_putNameIndex(&props, ID_ServerTime);
    ism_protocol_putLongValue(&props, currTime);
    ism_protocol_putNameIndex(&props, ID_OriginServer);
    ism_protocol_putStringValue(&props, "ASDFGHJK");
    props.pos = 0;
    ism_findPropertyNameIndex(&props, ID_ServerTime, &f);
    CU_ASSERT(f.type == VT_Long);
    CU_ASSERT(f.val.l == currTime);
    f.type = VT_Null;
    ism_findPropertyNameIndex(&props, ID_OriginServer, &f);
    CU_ASSERT(f.type == VT_String);
    if(f.type == VT_String)
        CU_ASSERT(!strcmp(uid, f.val.s));
    f.type = VT_Null;
    ism_findPropertyNameIndex(&props, ID_Topic, &f);
    CU_ASSERT(f.type == VT_String);
    if(f.type == VT_String)
        CU_ASSERT(!strcmp(topic, f.val.s));
    return;
}

//#pragma GCC diagnostic ignored "-Wformat-overflow="

static int  countTokens(int expected, const char * str, const char * delim) {
    int rc = ism_common_countTokens(str, delim);
    if (rc != expected)
        printf("countTokens \"%s\" \"%s\" = %d (%d)\n", 
                  (str ? str : "NULL"), 
                  (delim ? delim : "NULL"), rc, expected);
    return rc;
}



void CUnit_test_countTokens(void) {
    CU_ASSERT(countTokens(0, NULL, " ") == 0);
    CU_ASSERT(countTokens(0, "", "/") == 0);
    CU_ASSERT(countTokens(1, "  abc   ", " ") == 1);
    CU_ASSERT(countTokens(1, "---abc-,/-", "-,/") == 1);
    CU_ASSERT(countTokens(1, "abc",  "d") == 1);
    CU_ASSERT(countTokens(1, "   abc", " ") == 1);
    CU_ASSERT(countTokens(1, "abc. .", " .") == 1);
    CU_ASSERT(countTokens(2, "  abc  def  ", " ") == 2);
    CU_ASSERT(countTokens(2, "abc-def", "-") == 2);
    CU_ASSERT(countTokens(2, "   abc    def", " ") == 2);
    CU_ASSERT(countTokens(2, "abc          def      ", " ") == 2);
}
