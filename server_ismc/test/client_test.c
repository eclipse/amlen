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
 * File: client_test.c
 */

/* Programming Notes:
 *
 * Following steps provide the guidelines of adding new CUnit test function of ISM APIs to the program structure.
 * (searching for "programming" keyword to find all the labels).
 *
 * 1. Create an entry in "ISM_CUnit_tests" for passing the specific ISM APIs CUnit test function to CUnit framework.
 * 2. Create the prototype and the definition of the functions that need to be passed to CUnit framework.
 * 3. Define the number of the test iteration.
 * 4. Define the specific data structure for passing the parameters of the specific ISM API for the test.
 * 5. Define the prototype and the definition of the functions that are used by the function created in note 2.
 * 6. Create the test output message for the success ISM API tests
 * 7. Create the test output message for the failure ISM API tests
 * 8. Define the prototype and the definition of the parameter initialization functions of ISM API
 * 9. Define the prototype and the definition of the function, that calls the ISM API to carry out the test.
 *
 *
 */

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <ismutil.h>
#include <select_unit_test.h>
#include <message_unit_test.h>
#include <noack_unit_test.h>
#include <api_unit_test.h>
#include <tran_unit_test.h>
#include <transport.h>
#define DISPLAY_VERBOSE		0
#define DISPLAY_QUIET		1
void ism_engine_threadInit(uint8_t isStoreCrit)
{
}
int ism_process_monitoring_action(ism_transport_t *transport, const char *inpbuf, int buflen, concat_alloc_t *output_buffer, int *rc)
{
	return 0;
}
/*
 * Global Variables
 */
int display_mode = DISPLAY_VERBOSE;          /* initial display mode */
int default_display_mode = DISPLAY_VERBOSE;  /* default display mode */

/*
 * Array that carries the test suite and other functions to the CUnit framework
 */
CU_SuiteInfo ISM_client_suites[] = {
	IMA_TEST_SUITE("--- ISM selection compiler tests ---", NULL, NULL, ISM_select_tests),
	IMA_TEST_SUITE("--- ISM message tests            ---", NULL, NULL, ISM_message_tests),
	IMA_TEST_SUITE("--- ISM noack tests              ---", NULL, NULL, ISM_noack_tests),
	IMA_TEST_SUITE("--- ISM API tests                ---", NULL, NULL, ISM_api_tests),
	IMA_TEST_SUITE("--- ISM MQ bridge tran tests     ---", NULL, NULL, ISM_tran_tests),
	CU_SUITE_INFO_NULL,
};

/*
 * This is the main CUnit routine that starts the CUnit framework.
 * CU_basic_run_tests() Actually runs all the test routines.
 */
static void startup(void) {
	setvbuf(stdout, NULL, _IONBF, 0);
	if (CU_initialize_registry() == CUE_SUCCESS) {
		if (CU_register_suites(ISM_client_suites) == CUE_SUCCESS) {
			if (display_mode == DISPLAY_VERBOSE)
				CU_basic_set_mode(CU_BRM_VERBOSE);
			else
				CU_basic_set_mode(CU_BRM_SILENT);
			CU_basic_run_tests();
		}
	}
}


/*
 * This routine closes CUnit environment, and needs to be called only before the exiting of the program.
 */
void term(void) {
	CU_cleanup_registry();
}

/*
 * This routine displays the statistics for the test run in silent mode.
 * CUnit does not display any data for the test result logged as "success".
 */
static void summary(char * headline) {
	CU_RunSummary *pCU_pRunSummary;

	pCU_pRunSummary = CU_get_run_summary();
	if (display_mode != DISPLAY_VERBOSE) {
		printf("%s", headline);
		printf("\n--Run Summary: Type       Total     Ran  Passed  Failed\n");
		printf("               tests     %5d   %5d   %5d   %5d\n",
				pCU_pRunSummary->nTestsRun + 1, pCU_pRunSummary->nTestsRun + 1,
				pCU_pRunSummary->nTestsRun + 1 - pCU_pRunSummary->nTestsFailed,
				pCU_pRunSummary->nTestsFailed);
		printf("               asserts   %5d   %5d   %5d   %5d\n",
				pCU_pRunSummary->nAsserts, pCU_pRunSummary->nAsserts,
				pCU_pRunSummary->nAsserts - pCU_pRunSummary->nAssertsFailed,
				pCU_pRunSummary->nAssertsFailed);
	}
}

/*
 * This routine prints out the final test sun status. The final test run status will be scanned by the
 * build process to determine if the build is successful.
 * Please do not change the format of the output.
 */
void print_final_summary(void) {
	CU_RunSummary * CU_pRunSummary_Final;
	CU_pRunSummary_Final = CU_get_run_summary();
	printf("\n\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
	CU_pRunSummary_Final->nTestsRun,
	CU_pRunSummary_Final->nTestsFailed,
	CU_pRunSummary_Final->nAssertsFailed);
}

void ism_common_initUtil(void);

/*
 * Main entry point
 */
int main(int argc, char * * argv) {
	int trclvl = 0;
#ifdef _WIN32
	if (argc>2) {
		__debugbreak();
	}
#endif
	if (argc > 1 && *argv[1]=='v')
		display_mode = DISPLAY_VERBOSE;
	if (argc > 1 && *argv[1]=='q')
		display_mode = DISPLAY_QUIET;
	if (argc > 1 && *argv[1]=='t') {
		display_mode = DISPLAY_VERBOSE;
		trclvl = 7;
	}

    ism_common_initUtil();
	printf("%s\n", ism_common_getPlatformInfo());
	printf("%s\n\n", ism_common_getProcessorInfo());
	ism_common_setTraceLevel(trclvl);

	/* Run tests */
	startup();

	/* Print results */
	summary("\n\n Test: --- Testing ISM Client --- ...\n");
	print_final_summary();

	term();
	return 0;
}
