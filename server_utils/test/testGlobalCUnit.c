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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ismutil.h>

#include <testGlobalCUnit.h>

extern int g_verbose;

CU_TestInfo ISM_Util_CUnit_global[] = {
    { "Test Set/Get Request Locale", CUnit_test_setgetRequestLocale },
    { "Test CRC and CRC32", testCRC },
    CU_TEST_INFO_NULL
};

/*Request Locale Specific*/
pthread_key_t localekey;
pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void make_key(void)
{
    (void) pthread_key_create(&localekey, NULL);
}

/* The globalization tests initialization function.
 * Creates a timer thread.
 * Returns zero on success, non-zero otherwise.
 */
int initGlobalTests(void)
{

    /*Init Locale Thread Specific Key*/
	 (void) pthread_once(&key_once, make_key);
	 return 0;
}

/* The globalization test suite initialization function.
 * Returns zero on success, non-zero otherwise.
 */
int initGlobalUtilSuite(void)
{

    initGlobalTests();
    return 0;
}

/* The globalization tests cleanup function.
 * Frees allocated memory and terminates the timer thread.
 * Returns zero on success, non-zero otherwise.
 */
int cleanGlobalTests(void) {


    return 0;
}

/* The global test suite cleanup function.
 * Returns zero on success, non-zero otherwise.
 */
int cleanGlobalUtilSuite(void)
{
    cleanGlobalTests();
    return 0;
}

/*
 * Test Request Locale
 */
void CUnit_test_setgetRequestLocale(void) {
    if (g_verbose) {
        printf("\nCUnit_test_ism_common_setRequestLocale\n");
    }

	/*Test Set Validate Locale*/
	const char * locale = "EN_en";
	ism_common_setRequestLocale(localekey, locale);

   	const char * retLocale = ism_common_getRequestLocale(localekey);
    CU_ASSERT(!strcmp(locale,retLocale ));

    locale = "FR_fr";
	ism_common_setRequestLocale(localekey, locale);

   	retLocale = ism_common_getRequestLocale(localekey);
    CU_ASSERT(!strcmp(locale,retLocale ));

    /*Test Invalid Locales*/
    locale = "";
	ism_common_setRequestLocale(localekey, locale);

   	retLocale = ism_common_getRequestLocale(localekey);
    CU_ASSERT(!strcmp(retLocale,ism_common_getLocale() ));

    locale = NULL;
	ism_common_setRequestLocale(localekey, locale);

   	retLocale = ism_common_getRequestLocale(localekey);
    CU_ASSERT(!strcmp(retLocale,ism_common_getLocale() ));

}

void ism_common_crcinit(void);
void ism_common_crc32c_init(void);
/*
 * Test CRC32 and CRC32C
 */
void testCRC(void) {
   ism_common_crcinit();
   ism_common_crc32c_init();

   uint32_t crc32 = ism_common_crc(0, "Now is the tiem for all good men", 32);
   CU_ASSERT(crc32 == 0x60b5c4eb);
   uint32_t crc32c = ism_common_crc32c(0, "Now is the tiem for all good men", 32);
   CU_ASSERT(crc32c == 0xcdf08b05);
   // printf("crc=%08x  crc32=%08x\n", crc32, crc32c);
}
