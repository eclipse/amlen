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
 * File: store_test.c
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

#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <ismutil.h>
#include <store_unit_test.h>
#include <store.h>

#define DISPLAY_VERBOSE       0
#define DISPLAY_SILENT        1

#define BASIC_TEST_MODE       0
#define FULL_TEST_MODE        1
#define BYNAME_TEST_MODE      2

#define DIR_NAME              "/tmp/store_tests/TEST_STORE"
#define LARGE_FILENAME        "StoreLargeFile.dat"
#define SMALL_FILENAME        "StoreSmallFile.dat"
#define OUTPUT_FILENAME       "StoreOutput.dat"
#define ADMIN_FILENAME        "StoreAdminFile.dat"

#define LARGE_FILESIZE        1073741824
#define SMALL_FILESIZE        512000

#define STORE_MEMSIZE_MB      512
#define CHUNK_SIZE            65536

extern int ism_common_initServer(void);
extern int32_t ism_admin_init(int proctype);

/*
 * Global Variables
 */
int display_mode = DISPLAY_VERBOSE;         /* initial display mode    */
int test_mode = BASIC_TEST_MODE;            /* initial test mode       */
testParameters_t test_params;               /* test parameters         */
int RC = 0;

#include "stubFuncs.c"

/*
 * Array that carries the basic test suite and other functions to the CUnit framework
 */
CU_SuiteInfo
        ISM_store_CUnit_test_basicsuites[] = {
           IMA_TEST_SUITE("--- ISM Store tests ---", NULL, NULL, ISM_store_tests),
            CU_SUITE_INFO_NULL,
        };

/*
 * Array that carries the complete test suite and other functions to the CUnit framework
 */
CU_SuiteInfo
        ISM_store_CUnit_test_allsuites[] = {
           IMA_TEST_SUITE("--- ISM Store tests ---", NULL, NULL, ISM_store_tests),
           IMA_TEST_SUITE("--- ISM Store Performance tests ---", NULL, NULL, ISM_store_perf_tests),
           IMA_TEST_SUITE("--- ISM Store HA tests ---", NULL, NULL, ISM_storeHA_tests),
            CU_SUITE_INFO_NULL,
        };

/*
 * Prepare an input file for the test
 */
static int prepare_input_file(char *input_filename, size_t file_size)
{
   size_t bytes_written=0;
   void *pbuff;
   uint64_t val=0, *pvec;
   int fd, rc, nvecs, i;
   struct stat st;

   // Verify that the size of the input file is correct
   if (!stat(input_filename, &st))
   {
      if (st.st_size != file_size)
      {
         printf("The size %lu of the input file %s is not as requested %lu. The original file is being deleted.\n", st.st_size, input_filename, file_size);
         unlink(input_filename);
      }
   }

   // If the input data file does not exist, we have to create it
   if ( (fd = open(input_filename, O_RDONLY, 0444)) < 0 )
   {
      if ( (fd = open(input_filename, O_WRONLY|O_CREAT, 0644)) < 0 )
      {
         printf("Failed to create the input data file %s. Error: %s\n", input_filename, strerror(errno));
         return -1;
      }

      if ((rc = posix_memalign(&pbuff, getpagesize(), CHUNK_SIZE)))
      {
         printf("Failed to allocate memory for the temporary buffer. Error: %s\n", strerror(errno));
         close(fd);
         return -1;
      }

      printf("Creating input data file %s of size %lu ... ", input_filename, file_size);
      fflush(stdout);

      pvec = (uint64_t *)pbuff;
      nvecs = CHUNK_SIZE / 8;
      while (bytes_written < file_size)
      {
         for (i=0; i < nvecs ; i++) { pvec[i] = val++; }
         rc = write(fd, pbuff, CHUNK_SIZE);
         if (rc != CHUNK_SIZE)
         {
            printf("failed (%lu bytes written). %s\n", bytes_written, strerror(errno));
            close(fd);
            return -1;
         }
         bytes_written += rc;
      }
      rc = ftruncate(fd, file_size);
      printf("done\n");
   }
   close(fd);

   return 0;
}

/*
 * This is the main CUnit routine that starts the CUnit framework.
 */
static int Startup_CUnit(int argc, char **argv)
{
   CU_SuiteInfo *runsuite;
   CU_pTestRegistry testregistry;
   CU_pSuite testsuite;
   CU_pTest testcase;
   char *lfilename = LARGE_FILENAME, *sfilename = SMALL_FILENAME, *afilename = ADMIN_FILENAME;
   int testsrun = 0, trclvl = 0, haenabled = 0, ec, i;

   ism_common_initUtil();
   if ((ec = ism_common_initServer()) != ISMRC_OK)
   {
      printf("Failed to initialize the Server. error code %d\n", ec);
      return -1;
   }

   printf("%s\n", ism_common_getPlatformInfo());
   printf("%s\n\n", ism_common_getProcessorInfo());

   xUNUSED int rc;
   char cmdbuffer[256];
   sprintf(cmdbuffer,"rm -rf %s", DIR_NAME);
   printf("Cleaning up temp store directory %s\n", DIR_NAME);

   rc = system(cmdbuffer);
   sprintf(cmdbuffer,"mkdir -p %s", DIR_NAME);
   printf("Creating temp store directory %s\n", DIR_NAME);
   rc = system(cmdbuffer);

   memset(&test_params, 0, sizeof(test_params));
   strcpy(test_params.dir_name, DIR_NAME);
   test_params.store_memsize_mb = STORE_MEMSIZE_MB;

   // SHM access fails inside containers (changing mem type to non SHM for container builds)
   //test_params.mem_type = 2;
   //test_params.enable_persist = 1;

   // Parse command line arguments
   for (i=1; i < argc; i++)
   {
      if (!strcasecmp(argv[i],"f") || !strcasecmp(argv[i],"full"))
      {
         test_mode = FULL_TEST_MODE;
      }
      else if (!strcasecmp(argv[i],"byname"))
      {
         test_mode = BYNAME_TEST_MODE;
      }
      else if (!strcasecmp(argv[i],"silent"))
      {
         display_mode = DISPLAY_SILENT;
      }
      else if (!strcmp(argv[i],"-d"))
      {
         strcpy(test_params.dir_name, argv[++i]);
      }
      else if (!strcmp(argv[i],"-l"))
      {
         lfilename = argv[++i];
      }
      else if (!strcmp(argv[i],"-s"))
      {
         sfilename = argv[++i];
      }
      else if (!strcmp(argv[i],"-t"))
      {
         trclvl = atoi(argv[++i]);
      }
      else if (!strcmp(argv[i],"-m"))
      {
         test_params.store_memsize_mb = atoi(argv[++i]);
      }
      else if (!strcmp(argv[i],"-y"))
      {
         test_params.mem_type = atoi(argv[++i]);
      }
      else if (!strcmp(argv[i],"-oi"))
      {
         test_params.oid_interval = atoi(argv[++i]);
      }
      else if (!strcmp(argv[i],"-hp"))
      {
         test_params.ha_protocol = atoi(argv[++i]);
         haenabled = 1;
      }
      else if (!strcmp(argv[i],"-hs"))
      {
         strcpy(test_params.ha_startup, argv[++i]);
         haenabled = 1;
      }
      else if (!strcmp(argv[i],"-ht"))
      {
         test_params.ha_port = atoi(argv[++i]);
         haenabled = 1;
      }
      else if (!strcmp(argv[i],"-h0"))
      {
         strcpy(test_params.remote_disc_nic, argv[++i]);
         haenabled = 1;
      }
      else if (!strcmp(argv[i],"-h1"))
      {
         strcpy(test_params.local_disc_nic, argv[++i]);
         haenabled = 1;
      }
      else if (!strcmp(argv[i],"-h2"))
      {
         strcpy(test_params.local_rep_nic, argv[++i]);
         haenabled = 1;
      }
      else if (!strcmp(argv[i],"-hf"))
      {
         afilename = argv[++i];
      }
      else if (!strcmp(argv[i],"-p"))
      {
         test_params.enable_persist = atoi(argv[++i]);
      }
   }

   sprintf(test_params.large_filename, "%s/%s", test_params.dir_name, lfilename);
   sprintf(test_params.small_filename, "%s/%s", test_params.dir_name, sfilename);
   sprintf(test_params.output_filename, "%s/%s", test_params.dir_name, OUTPUT_FILENAME);

   printf("Test mode %s, MemType %d, MinActiveOidInterval %u\n",
         (test_mode == 0 ? "BASIC" : test_mode == 1 ? "FULL" : "BYNAME"), test_params.mem_type, test_params.oid_interval);
   printf("Data directory %s, Large file %s, Small file %s\n",
          test_params.dir_name, test_params.large_filename, test_params.small_filename);

   //ism_common_setTraceFile("store_tests.log", 0);
   //trclvl=5;
   ism_common_setTraceLevel(trclvl);
   ism_common_setTraceOptions("time,thread,where");

   if (haenabled)
   {
      if ((ec = ism_admin_init(0)) != ISMRC_OK)
      {
         printf("Failed to initialize the admin service. error code %d\n", ec);
         return -1;
      }
      sprintf(test_params.admin_filename, "%s", afilename);
      printf("High-Availability: Protocol %u, Startup mode %s, Port %d, Remote discovery NIC %s, Local discovery NIC %s, Local replication NIC %s, Admin file %s\n",
             test_params.ha_protocol, test_params.ha_startup, test_params.ha_port, test_params.remote_disc_nic, test_params.local_disc_nic, test_params.local_rep_nic, test_params.admin_filename);
   }

   // Make sure the process has read/write access to the data directory
   if (access(test_params.dir_name, R_OK | W_OK))
   {
      printf("Failed to access the data directory %s. Error: %s\n", test_params.dir_name, strerror(errno));
      return -1;
   }

   if (prepare_input_file(test_params.large_filename, LARGE_FILESIZE) || prepare_input_file(test_params.small_filename, SMALL_FILESIZE))
   {
      return -1;
   }

   if (test_mode == BASIC_TEST_MODE)
      runsuite = ISM_store_CUnit_test_basicsuites;
   else
      // Load all tests for both FULL and BYNAME.
      // This way BYNAME can search all suite and test names
      // to find the tests to run.
      runsuite = ISM_store_CUnit_test_allsuites;

   setvbuf(stdout, NULL, _IONBF, 0);
   if (CU_initialize_registry() == CUE_SUCCESS)
   {
      if (CU_register_suites(runsuite) == CUE_SUCCESS)
      {
         if (display_mode == DISPLAY_VERBOSE)
            CU_basic_set_mode(CU_BRM_VERBOSE);
         else
            CU_basic_set_mode(CU_BRM_SILENT);

         if (test_mode != BYNAME_TEST_MODE)
            CU_basic_run_tests();
         else
         {
            int k;
            char * testname = NULL;
            testregistry = CU_get_registry();
            for (k = 1; k < argc; k++)
            {
               testname = argv[k];
               testsuite = CU_get_suite_by_name(testname,testregistry);
               if (NULL != testsuite)
               {
                  CU_basic_run_suite(testsuite);
                  testsrun++;
               }
               else
               {
                  int j;
                  testsuite = testregistry->pSuite;
                  for (j = 0; j < testregistry->uiNumberOfSuites; j++)
                  {
                     testcase = CU_get_test_by_name(testname,testsuite);
                     if (NULL != testcase)
                     {
                        CU_basic_run_test(testsuite,testcase);
                        break;
                     }
                     testsuite = testsuite->pNext;
                  }
               }
            }
         }
      }
   }

   return 0;
}

/*
 * This routine closes CUnit environment, and needs to be called only before the exiting of the program.
 */
static void Endup_CUnit(void)
{
   char cmdbuffer[256];
   sprintf(cmdbuffer,"rm -rf %s", DIR_NAME);
   printf("Cleaning up temp store directory %s\n", DIR_NAME);
   xUNUSED int rc;
   rc = system(cmdbuffer);
   CU_cleanup_registry();
}

/*
 * This routine displays the statistics for the test run in silent mode.
 * CUnit does not display any data for the test result logged as "success".
 */
static void summary(char * headline)
{
   CU_RunSummary *pCU_pRunSummary;

   pCU_pRunSummary = CU_get_run_summary();
   if (display_mode != DISPLAY_VERBOSE)
   {
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
static void print_final_summary(void)
{
   CU_RunSummary * CU_pRunSummary_Final;
   CU_pRunSummary_Final = CU_get_run_summary();
   printf("\n\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
   CU_pRunSummary_Final->nTestsRun,
   CU_pRunSummary_Final->nTestsFailed,
   CU_pRunSummary_Final->nAssertsFailed);
   RC = CU_pRunSummary_Final->nTestsFailed + CU_pRunSummary_Final->nAssertsFailed;
}

/*
 * Main entry point
 */
int main(int argc, char **argv)
{
   /* Run tests */
   if (!Startup_CUnit(argc, argv))
   {
      /* Print results */
      summary("\n\n Test: --- Testing ISM Store --- ...\n");
      print_final_summary();
   }

   Endup_CUnit();

   return RC;
}
