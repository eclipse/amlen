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

/*********************************************************************/
/*                                                                   */
/* Module Name: test_dumpFormatting.c                                */
/*                                                                   */
/* Description: Unit tests of the dump formatting routines           */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "engine.h"
#include "dumpFormat.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"

// test Utility function iefm_convertToHumanNumber
void test_iefm_convertToHumanNumber(void)
{
    typedef struct test_t
    {
        int64_t number;
        char *result;
    } test_t;

    test_t testConversion[] = {{0, "0"}, {1, "1"}, {-1, "-1"},
                               {607, "607"}, {9000, "9,000"}, {40030, "40,030"},
                               {303004, "303,004"},
                               {-2000001, "-2,000,001"},
                               {12345678901, "12,345,678,901"},
                               {0, NULL}};

    for(int32_t i = 0; testConversion[i].result != NULL; i++)
    {
        char humanNumber[50];

        iefm_convertToHumanNumber(testConversion[i].number, humanNumber);

        TEST_ASSERT_STRINGS_EQUAL(humanNumber, testConversion[i].result);
    }
}

// test Utility function iefm_convertToHumanFileSize
void test_iefm_convertToHumanFileSize(void)
{
    typedef struct test_t
    {
        size_t bytes;
        char *result;
    } test_t;

    test_t testConversion[] = {{0, "0B"}, {1, "1B"}, {607, "607B"},
                               {(size_t)(8.8 * 1024), "8.8K"},
                               {(size_t)(1.0 * 1024*1024), "1.0M"},
                               {(size_t)(3.3 * 1024*1024), "3.3M"},
                               {(size_t)(9.0 * 1024*1024*1024), "9.0G"},
                               {(size_t)(1.0 * 1024*1024*1024*1024), "1.0T"},
                               {0, NULL}};

    for(int32_t i = 0; testConversion[i].result != NULL; i++)
    {
        char humanFileSize[50];

        iefm_convertToHumanFileSize(testConversion[i].bytes, humanFileSize);

        TEST_ASSERT(strcmp(humanFileSize, testConversion[i].result) == 0,
                    ("expected '%s' got '%s'", testConversion[i].result, humanFileSize));
    }
}

// test Utility functions iefm_get* extraction from buffers
void test_iefm_get(void)
{
    iefmHeader_t dumpHeader = {0};
    dumpHeader.outputFile = stdout;

    char buffer[1024];
    int32_t i;

    for(int32_t x=0; x<2; x++)
    {
        dumpHeader.byteSwap = !!x;
        int32_t val_int32[] = {-1, INT32_MAX, INT32_MIN, 50, 100000, 0};
        i=-1;
        do
        {
            i++;
            int32_t pos=rand()%376; // vary the position of the data in the buffer
            if (dumpHeader.byteSwap)
            {
                int32_t value = __builtin_bswap32(val_int32[i]);
                memcpy(&(buffer[pos]), &value, sizeof(value));
            }
            else
            {
                memcpy(&(buffer[pos]), &val_int32[i], sizeof(val_int32[i]));
            }
            int32_t retval = iefm_getInt32(&buffer[pos], &dumpHeader);
            TEST_ASSERT_EQUAL(retval, val_int32[i]);
        }
        while(val_int32[i] != 0);

        uint32_t val_uint32[] = {20, UINT32_MAX, 17, 24531, 0};
        i = -1;
        do
        {
            i++;
            int32_t pos=rand()%376; // vary the position of the data in the buffer
            if (dumpHeader.byteSwap)
            {
                int32_t value = __builtin_bswap32(val_uint32[i]);
                memcpy(&(buffer[pos]), &value, sizeof(value));
            }
            else
            {
                memcpy(&(buffer[pos]), &val_uint32[i], sizeof(val_uint32[i]));
            }
            uint32_t retval = iefm_getUint32(&buffer[pos], &dumpHeader);
            TEST_ASSERT_EQUAL(retval, val_uint32[i]);
        }
        while(val_uint32[i] != 0);

        uint64_t val_uint64[] = {UINT64_MAX, 50, 100000, 3333333333, 0xDEADBEEF, 0};
        i=-1;
        do
        {
            i++;
            int32_t pos=rand()%376; // vary the position of the data in the buffer
            if (dumpHeader.byteSwap)
            {
                uint64_t value = __builtin_bswap64(val_uint64[i]);
                memcpy(&(buffer[pos]), &value, sizeof(value));
            }
            else
            {
                memcpy(&(buffer[pos]), &val_uint64[i], sizeof(val_uint64[i]));
            }
            uint64_t retval = iefm_getUint64(&buffer[pos], &dumpHeader);
            TEST_ASSERT_EQUAL(retval, val_uint64[i]);
            void *retptr = iefm_getPointer(&buffer[pos], &dumpHeader);
            TEST_ASSERT_EQUAL(retptr, (void *)val_uint64[i]);
        }
        while(val_uint64[i] != 0);
    }
}

// test Utility function iefm_convertToHumanFileSize
void test_iefm_checkStrucId(void)
{
    iefmHeader_t dumpHeader = {0};
    iefmStructureDescription_t structure = {0};
    iefmMemberDescription_t members[] = { { "strucId", 50, 4, "char[4]", "char", 0, false, 4, 1 }, {0} };
    char buffer[1024];

    structure.name = "testStructure";
    structure.strucIdValue = "TEST";
    structure.member = members;
    structure.memberCount = 1;
    structure.buffer = buffer;
    structure.length = 100;

    dumpHeader.outputFile = stdout;
    memcpy(&buffer[50], "TEST", 4);

    // No strucIdMember defined - should just ignore
    iefm_checkStrucId(&dumpHeader, &structure);

    structure.strucIdMember = "strucId";
    // Should work
    iefm_checkStrucId(&dumpHeader, &structure);

    printf("The next tests are testing error cases, and will print 'ERROR:...'\n");
    // Should not match
    buffer[50] = 'Z';
    iefm_checkStrucId(&dumpHeader, &structure);
    // Eyecatcher not in buffer
    structure.length = 20;
    iefm_checkStrucId(&dumpHeader, &structure);
}

CU_TestInfo ISM_DumpFormatting_Cunit_UtilityFunctions[] =
{
    { "iefm_convertToHumanNumber", test_iefm_convertToHumanNumber },
    { "iefm_convertToHumanFileSize", test_iefm_convertToHumanFileSize },
    { "iefm_get", test_iefm_get },
    { "iefm_checkStrucId", test_iefm_checkStrucId },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initDumpFormatting(void)
{
    return OK;
}

int termDumpFormatting(void)
{
    return OK;
}

CU_SuiteInfo ISM_DumpFormatting_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("UtilityFunctions", initDumpFormatting, termDumpFormatting, ISM_DumpFormatting_Cunit_UtilityFunctions),
    CU_SUITE_INFO_NULL,
};

int setup_CU_registry(int argc, char **argv, CU_SuiteInfo *allSuites)
{
    CU_SuiteInfo *tempSuites = NULL;

    int retval = 0;

    if (argc > 1)
    {
        int suites = 0;

        for(int i=1; i<argc; i++)
        {
            if (!strcasecmp(argv[i], "FULL"))
            {
                if (i != 1)
                {
                    retval = 97;
                    break;
                }
                // Driven from 'make fulltest' ignore this.
            }
            else if (!strcasecmp(argv[i], "verbose"))
            {
                CU_basic_set_mode(CU_BRM_VERBOSE);
            }
            else if (!strcasecmp(argv[i], "silent"))
            {
                CU_basic_set_mode(CU_BRM_SILENT);
            }
            else
            {
                bool suitefound = false;
                int index = atoi(argv[i]);
                int totalSuites = 0;

                CU_SuiteInfo *curSuite = allSuites;

                while(curSuite->pTests)
                {
                    if (!strcasecmp(curSuite->pName, argv[i]))
                    {
                        suitefound = true;
                        break;
                    }

                    totalSuites++;
                    curSuite++;
                }

                if (!suitefound)
                {
                    if (index > 0 && index <= totalSuites)
                    {
                        curSuite = &allSuites[index-1];
                        suitefound = true;
                    }
                }

                if (suitefound)
                {
                    tempSuites = realloc(tempSuites, sizeof(CU_SuiteInfo) * (suites+2));
                    memcpy(&tempSuites[suites++], curSuite, sizeof(CU_SuiteInfo));
                    memset(&tempSuites[suites], 0, sizeof(CU_SuiteInfo));
                }
                else
                {
                    printf("Invalid test suite '%s' specified, the following are valid:\n\n", argv[i]);

                    index=1;

                    curSuite = allSuites;

                    while(curSuite->pTests)
                    {
                        printf(" %2d : %s\n", index++, curSuite->pName);
                        curSuite++;
                    }

                    printf("\n");

                    retval = 99;
                    break;
                }
            }
        }
    }

    if (retval == 0)
    {
        if (tempSuites)
        {
            CU_register_suites(tempSuites);
        }
        else
        {
            CU_register_suites(allSuites);
        }
    }

    if (tempSuites) free(tempSuites);

    return retval;
}

int main(int argc, char *argv[])
{
    int trclvl = 4;
    int retval = 0;

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_DumpFormatting_CUnit_allsuites);

    if (retval == 0)
    {
        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();
        printf("Random Seed =     %"PRId64"\n", seedVal);
        printf("\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
               CU_pRunSummary_Final->nTestsRun,
               CU_pRunSummary_Final->nTestsFailed,
               CU_pRunSummary_Final->nAssertsFailed);
        if ((CU_pRunSummary_Final->nTestsFailed > 0) ||
            (CU_pRunSummary_Final->nAssertsFailed > 0))
        {
            retval = 1;
        }
    }

    CU_cleanup_registry();

    test_processTerm(retval == 0);

mod_exit:

    return retval;
}
