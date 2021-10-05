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
/* Module Name: test_awkwardQDelete.c                                */
/*                                                                   */
/* Description: Main source file for CUnit test of deleting queues.  */
/* Intention is to have a mix of deterministic and more random       */
/* multi-threaded tests                                              */
/*                                                                   */
/*********************************************************************/
#include <sys/prctl.h>

#include "test_utils_assert.h"
#include "test_utils_message.h"
#include "test_utils_client.h"
#include "test_utils_initterm.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_preload.h"
#include "test_utils_security.h"

#include "engine.h"
#include "engineInternal.h"
#include "exportCrypto.h"

/*******************************************************************/
/* First some "Deterministic" tests                                */
/*******************************************************************/
void test_BasicWriteRead(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t filePath_BuffSize = 255;
    char filePath[filePath_BuffSize];
    char *suffix = "/test_BasicWriteRead.enc";
    if(!test_getGlobalAdminDir(filePath, filePath_BuffSize)) abort();

    if (strlen(filePath) + strlen(suffix) >= filePath_BuffSize)abort();

    strcat(filePath, suffix);


    //Initialise OpenSSL - in the product - the equivalent is ism_ssl_init();
    //TODO: Do we need to run that in the engine thread that does this or is it done?
    sslInit();

    ieieEncryptedFileHandle_t outfile = ieie_createEncryptedFile( pThreadData
                                                                , iemem_diagnostics // Arbitrary memtype choice
                                                                , filePath
                                                                , "testPassword");
    TEST_ASSERT_PTR_NOT_NULL(outfile);
    char *outText1 = "This is some test Text\n";
    size_t outDataLen = strlen(outText1)+1;
    uint64_t outDataId = (uint64_t)&outText1;
    ieieDataType_t outDataType = ieieDATATYPE_BYTES;
    int32_t rc = ieie_exportData( pThreadData
                                , outfile
                                , outDataType
                                , outDataId
                                , outDataLen
                                , outText1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieie_finishWritingEncryptedFile( pThreadData, outfile);
    TEST_ASSERT_EQUAL(rc, OK);

    ieieEncryptedFileHandle_t infile = ieie_OpenEncryptedFile( pThreadData
                                                             , iemem_topicsTree // Arbitrary type
                                                             , filePath
                                                             , "testPassword");
    TEST_ASSERT_PTR_NOT_NULL(infile);

    uint64_t inDataId = 0;
    size_t inDataLen =0;
    ieieDataType_t inDataType;
    void *inData = NULL;

    rc = ieie_importData(pThreadData, infile, &inDataType, &inDataId, &inDataLen, &inData);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(inDataId, outDataId);
    TEST_ASSERT_EQUAL(inDataLen, outDataLen);
    TEST_ASSERT_EQUAL(inDataType, outDataType);
    TEST_ASSERT_EQUAL(0, memcmp(inData, outText1, inDataLen));

    ieie_freeReadExportedData(pThreadData, iemem_topicsTree, inData);

    rc = ieie_finishReadingEncryptedFile(pThreadData, infile);
    TEST_ASSERT_EQUAL(rc, OK);

    sslTerm();

}

void test_LargerWriteRead(void)
{
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t filePath_BuffSize = 255;
    char filePath[filePath_BuffSize];
    char *suffix = "/test_LargerWriteRead.enc";
    if(!test_getGlobalAdminDir(filePath, filePath_BuffSize)) abort();

    if (strlen(filePath) + strlen(suffix) >= filePath_BuffSize)abort();

    strcat(filePath, suffix);

    int numRecords=5;
    size_t maxRecordLen=31000;
    size_t minRecordLen=5000;

    size_t   recordLen[numRecords];
    char     recordChar[numRecords];
    uint64_t recordId[numRecords];

    int32_t rc;

    //Initialise OpenSSL - in the product - the equivalent is ism_ssl_init();
    //TODO: Do we need to run that in the engine thread that does this or is it done?
    sslInit();

    ieieEncryptedFileHandle_t outfile = ieie_createEncryptedFile( pThreadData
                                                                , iemem_diagnostics // Arbitrary memtype
                                                                , filePath
                                                                , "testPassword");
    TEST_ASSERT_PTR_NOT_NULL(outfile);

    ieieDataType_t outDataType = ieieDATATYPE_BYTES;
    for (uint32_t i = 0; i < numRecords; i++)
    {
        recordLen[i] = minRecordLen + rand() %(maxRecordLen +1 - minRecordLen);
        recordChar[i] = 'a' + rand()%27;
        recordId[i]  = (uint64_t)&(recordId[i]);

        void *outBuf = malloc( recordLen[i]);
        TEST_ASSERT_PTR_NOT_NULL(outBuf);

        memset(outBuf, recordChar[i], recordLen[i]);

        rc = ieie_exportData( pThreadData
                            , outfile
                            , outDataType
                            , recordId[i]
                            , recordLen[i]
                            , outBuf);
        TEST_ASSERT_EQUAL(rc, OK);

        free(outBuf);
    }



    rc = ieie_finishWritingEncryptedFile( pThreadData, outfile);
    TEST_ASSERT_EQUAL(rc, OK);

    ieieEncryptedFileHandle_t infile = ieie_OpenEncryptedFile( pThreadData
                                                             , iemem_remoteServers // Arbitrary type
                                                             , filePath
                                                             , "testPassword");
    TEST_ASSERT_PTR_NOT_NULL(infile);

    size_t inDataBufSize = 0;
    char *inData = NULL;

    for (uint32_t i = 0; i < (numRecords+1); i++)
    {
        uint64_t inDataId = 0;
        size_t inDataLen = inDataBufSize;
        ieieDataType_t inDataType;

        test_log(testLOGLEVEL_TESTPROGRESS, "reading record %u\n", i);

        rc = ieie_importData(pThreadData, infile, &inDataType, &inDataId, &inDataLen, (void **)&inData);

        if (i < numRecords)
        {
            TEST_ASSERT_EQUAL(rc, OK);

            if (inDataLen > inDataBufSize)
            {
                inDataBufSize = inDataLen;
            }

            TEST_ASSERT_EQUAL(inDataId, recordId[i]);
            TEST_ASSERT_EQUAL(inDataLen, recordLen[i]);
            TEST_ASSERT_EQUAL(inDataType, outDataType);

            for (uint32_t j = 0; j < inDataLen; j++)
            {
                TEST_ASSERT_EQUAL(inData[j], recordChar[i]);
            }
        }
        else
        {
            //We've tried to read more records than there are in the file...
            TEST_ASSERT_EQUAL(rc, ISMRC_EndOfFile);
        }
    }
    ieie_freeReadExportedData(pThreadData, iemem_remoteServers, inData);


    rc = ieie_finishReadingEncryptedFile(pThreadData, infile);
    TEST_ASSERT_EQUAL(rc, OK);

    sslTerm();

}
CU_TestInfo ISM_ExportCrypto_CUnit_test_Deterministic[] =
{
    { "testBasicWriteRead", test_BasicWriteRead },
    { "testLargerWriteRead", test_LargerWriteRead },
    CU_TEST_INFO_NULL
};

int initExportCrypto(void)
{
    return test_engineInit(true, true,
                           true, // Disable Auto creation of named queues
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termExportCrypto(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_ExportCrypto_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("Deterministic", initExportCrypto, termExportCrypto, ISM_ExportCrypto_CUnit_test_Deterministic),
    CU_SUITE_INFO_NULL
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
    int trclvl = 0;
    int retval = 0;
    int testLogLevel = 2;

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_ExportCrypto_CUnit_allsuites);

    if (retval == 0)
    {
        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();

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
