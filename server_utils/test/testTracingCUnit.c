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
#define TRACE_COMP Util
#include <ismutil.h>
#include <log.h>
#include <testTracingCUnit.h>
#include <ismrc.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

//#define LOGFILE  "/tmp/ismservertest.log"
#define TRACE_FILE  "ismservertrace.log"
#define TRACE_PREV_FILE  "ismservertrace_prev.log"
static FILE * trcfile = NULL;
extern void ism_common_initTrace();

/*   (programming note 1)
 * (programming note 11)
 * Array of timer tests for server_utils APIs to CUnit framework.
 */
CU_TestInfo ISM_Util_CUnit_tracing[] =
  {
     	{ "Trace Macros Test", CUnit_test_ISM_Logging_TraceMacro },
     	{ "Trace Macros Test with Replacements", CUnit_test_ISM_Logging_TraceMacroWithReplacements },
     	{ "Trace Macros Test with Component Levels", CUnit_test_ISM_Logging_TraceMacroWithComponentLevels },
        CU_TEST_INFO_NULL
   };

int initTracingTests(void) {
   	int rc = 0;
    return rc;
}

/* The timer tests cleanup function.
 * Removes any files created and terminates the timer thread.
 * Returns zero on success, non-zero otherwise.
 */
int cleanTracingTests(void) {
    int rc = 0;
    return rc;
}

extern int g_traceInited;

void CUnit_test_ISM_Logging_TraceMacro(void) {
	int traceLevel;
    const char * traceFile;
    ism_field_t f;
    const char * traceLine="This is a test from Cunit.";
    const int timestampLength = 0;
    int traceLineLen = strlen(traceLine) + timestampLength;
    int trcSize=0;
    char outputStr[1026];

    ism_common_initUtil();

    f.type = VT_String;
    f.val.s = TRACE_FILE;

    ism_common_setProperty( ism_common_getConfigProperties(), "TraceFile", &f);

    f.type = VT_String;
   	f.val.s = "1";
    ism_common_setProperty( ism_common_getConfigProperties(), "TraceLevel", &f);

    f.type = VT_String;
    f.val.s = "";
    ism_common_setProperty(ism_common_getConfigProperties(), "TraceOptions", &f);

    traceFile = ism_common_getStringConfig("TraceFile");
    remove(traceFile);

    CU_ASSERT(!strcmp(traceFile, TRACE_FILE));

    traceLevel = ism_common_getIntConfig("TraceLevel", 0);
	CU_ASSERT(traceLevel == 1);


	/*Init Trace*/
	g_traceInited = 0;
	ism_common_initTrace();

	/*Insert: With Level lower than the current trace level*/
	TRACE(5, traceLine);

	trcfile = fopen(traceFile, "r");
    if (trcfile) {
        fseek(trcfile, 0, SEEK_END);
        trcSize = ftell(trcfile);
    }
    CU_ASSERT(trcfile != NULL);
    if (trcfile)
        fclose(trcfile);

    CU_ASSERT(trcSize==0);


    /* Insert: With Level equal than the current trace level */
	TRACE(1, traceLine);

	trcfile = fopen(traceFile, "r");
    if (trcfile) {
        fseek(trcfile, 0, SEEK_END);
        trcSize = ftell(trcfile);
    }
    CU_ASSERT(trcfile != NULL);
    if (trcfile)
        fclose(trcfile);
    remove(traceFile);
    CU_ASSERT(trcSize == traceLineLen);

	/* Init Trace*/
    g_traceInited = 0;
	ism_common_initTrace();

    /* Insert: With Level higher than the current trace level */
	TRACE(0, traceLine);

	trcfile = fopen(traceFile, "r");
	memset(&outputStr, 0, 1026);
	trcSize = 0;
    if (trcfile) {
        char *fgets_ret = NULL;

        fgets_ret = fgets((char *)&outputStr, 1026, trcfile);
        CU_ASSERT(fgets_ret != NULL);

        fseek(trcfile, 0, SEEK_END);
        trcSize = ftell(trcfile);

    }
    CU_ASSERT(trcfile!=NULL);

    if (trcfile)
        fclose(trcfile);
    remove(traceFile);
    CU_ASSERT(trcSize == traceLineLen);
    CU_ASSERT(!strcmp(outputStr + timestampLength, traceLine));
}

void CUnit_test_ISM_Logging_TraceMacroWithReplacements(void) {
	int traceLevel;
    const char * traceFile;
    ism_field_t f;
    int rvalue1=1;
    const char * rvalue2="testing";
    const char * traceLine="This is a test from Cunit. value1=%d. value=%s";
    const char * rtraceLine="This is a test from Cunit. value1=1. value=testing";
    const int timestampLength = 0;
    int traceLineLen = strlen(rtraceLine) + timestampLength;
    int trcSize=0;
    char outputStr[1026];

    ism_common_initUtil();

    f.type = VT_String;
    f.val.s = TRACE_FILE;

    ism_common_setProperty( ism_common_getConfigProperties(), "TraceFile", &f);

    f.type = VT_Integer;
   	f.val.i = 1;
    ism_common_setProperty( ism_common_getConfigProperties(), "TraceLevel", &f);

    traceFile = ism_common_getStringConfig("TraceFile");
	remove(traceFile);
    CU_ASSERT(!strcmp(traceFile, TRACE_FILE));

    traceLevel = ism_common_getIntConfig("TraceLevel", 0);
	CU_ASSERT(traceLevel==1);


	/*Init Trace*/
    g_traceInited = 0;
	ism_common_initTrace();

	/*Insert: With Level lower than the current trace level*/
	TRACE(5,traceLine);

	trcfile = fopen(traceFile, "r");
    if (trcfile) {
            fseek(trcfile, 0, SEEK_END);
            trcSize = ftell(trcfile);
   }
   CU_ASSERT(trcfile!=NULL);
   if (trcfile) fclose(trcfile);

   CU_ASSERT(trcSize==0);


   /*Insert: With Level equal than the current trace level*/
    TRACE(1,traceLine, rvalue1, rvalue2);

    trcfile = fopen(traceFile, "r");
    if (trcfile) {
            char *fgets_ret = fgets((char *)&outputStr, 1026, trcfile);
            CU_ASSERT(fgets_ret != NULL);
            fseek(trcfile, 0, SEEK_END);
            trcSize = ftell(trcfile);
    }
    CU_ASSERT(trcfile!=NULL);
    CU_ASSERT(trcSize==traceLineLen);
    CU_ASSERT(trcfile && !strcmp(outputStr+timestampLength,rtraceLine));
    if (trcfile) fclose(trcfile);
    remove(traceFile);

    /*Init Trace*/
    g_traceInited = 0;
    ism_common_initTrace();

    /*Insert: With Level higher than the current trace level*/
    TRACE(0,traceLine, rvalue1, rvalue2);

    trcfile = fopen(traceFile, "r");
    memset(&outputStr, 0, 1026);
    trcSize=0;
    if (trcfile) {
            char * fgets_ret = fgets((char *)&outputStr, 1026, trcfile);
            CU_ASSERT(fgets_ret != NULL);
            fseek(trcfile, 0, SEEK_END);
            trcSize = ftell(trcfile);

   }
   CU_ASSERT(trcfile!=NULL);

   if (trcfile) fclose(trcfile);
   remove(traceFile);
   CU_ASSERT(trcSize==traceLineLen);
   CU_ASSERT(!strcmp(outputStr+timestampLength,rtraceLine));


}


void CUnit_test_ISM_Logging_TraceMacroWithComponentLevels(void) {
	int traceLevel;
    const char * traceFile;
    ism_field_t f;
    int rvalue1=1;
    const char * rvalue2="testing";
    const char * traceLine="This is a test from Cunit. value1=%d. value=%s";
    const char * rtraceLine="This is a test from Cunit. value1=1. value=testing";
    const int timestampLength = 0;
    int traceLineLen = strlen(rtraceLine) + timestampLength;
    int trcSize=0;
    char outputStr[1026];

    ism_common_initUtil();

    f.type = VT_String;
    f.val.s = TRACE_FILE;

    ism_common_setProperty( ism_common_getConfigProperties(), "TraceFile", &f);

    f.type = VT_Integer;
   	f.val.i = 1;
    ism_common_setProperty( ism_common_getConfigProperties(), "TraceLevel", &f);

    traceFile = ism_common_getStringConfig("TraceFile");
	remove(traceFile);
    CU_ASSERT(!strcmp(traceFile, TRACE_FILE));

    traceLevel = ism_common_getIntConfig("TraceLevel", 0);
	CU_ASSERT(traceLevel==1);


	/*Init Trace*/
    g_traceInited = 0;
	ism_common_initTrace();

	/*Insert: With Level lower than the current trace level*/
	TRACE(5,traceLine);

	trcfile = fopen(traceFile, "r");
    if (trcfile) {
            fseek(trcfile, 0, SEEK_END);
            trcSize = ftell(trcfile);
   }
   CU_ASSERT(trcfile!=NULL);
   if (trcfile) fclose(trcfile);

   CU_ASSERT(trcSize==0);


   /*Insert: With Level equal than the current trace level*/
	TRACE(1,traceLine, rvalue1, rvalue2);

	trcfile = fopen(traceFile, "r");
    if (trcfile) {
            char *fgets_ret = fgets((char *)&outputStr, 1026, trcfile);
            CU_ASSERT(fgets_ret != NULL);

            fseek(trcfile, 0, SEEK_END);
            trcSize = ftell(trcfile);
    }
    CU_ASSERT(trcfile!=NULL);
    CU_ASSERT(trcSize==traceLineLen);
    CU_ASSERT(trcfile && !strcmp(outputStr+timestampLength,rtraceLine));
    if (trcfile) fclose(trcfile);
    remove(traceFile);

    /*Init Trace*/
    g_traceInited = 0;
    ism_common_initTrace();

    ism_defaultTrace->trcComponentLevels[TRACECOMP_Util] = 2;

    /*Insert: With Level higher than the current trace level*/
    TRACE(3,traceLine, rvalue1, rvalue2);
    trcSize=0;
    trcfile = fopen(traceFile, "r");
    if (trcfile) {
            char *fgets_ret = fgets((char *)&outputStr, 1026, trcfile);
            //May be empty read - may not - silly asserts to prevent compiler warning
            if (fgets_ret != NULL) {
                CU_ASSERT(fgets_ret != NULL);
            } else {
                CU_ASSERT(fgets_ret == NULL);
            }

            fseek(trcfile, 0, SEEK_END);
            trcSize = ftell(trcfile);

   }
   CU_ASSERT(trcfile!=NULL);

   if (trcfile) fclose(trcfile);
   remove(traceFile);

   CU_ASSERT(trcSize==0);



   /*Init Trace*/
   g_traceInited = 0;
	ism_common_initTrace();

    ism_defaultTrace->trcComponentLevels[TRACECOMP_Util] = 2;

   /*Insert: With Level higher than the current trace level*/
	TRACE(1,traceLine, rvalue1, rvalue2);
	trcSize=0;
	trcfile = fopen(traceFile, "r");
	memset(&outputStr, 0, 1026);
    if (trcfile) {
            char *fgets_ret = fgets((char *)&outputStr, 1026, trcfile);
            //May be empty read - may not - silly asserts to prevent compiler warning
            if (fgets_ret != NULL) {
                CU_ASSERT(fgets_ret != NULL);
            } else {
                CU_ASSERT(fgets_ret == NULL);
            }

            fseek(trcfile, 0, SEEK_END);
            trcSize = ftell(trcfile);
   }
   CU_ASSERT(trcfile!=NULL);

   CU_ASSERT(trcSize==traceLineLen);
   CU_ASSERT(trcfile && !strcmp(outputStr+timestampLength,rtraceLine));
   if (trcfile) fclose(trcfile);
   remove(traceFile);
}
