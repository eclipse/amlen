/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
/* Module Name: test_utils_phases.c                                  */
/*                                                                   */
/* Description: Framework to enable simple test phases               */
/*                                                                   */
/*********************************************************************/
#include <math.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_log.h"

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>
#include <ismutil.h>

#define MAX_ARGS 15

static int32_t test_utils_phaseParseArgs( int argc
                                        , char **argv
                                        , uint32_t *pPhase
                                        , uint32_t *pFinalPhase
                                        , uint32_t *pTestLogLevel
                                        , int32_t *pTraceLevel
                                        , char **recoveryMethod
                                        , ism_time_t *pSeedVal
                                        , char **adminDir
                                        , bool *isTempAdminDir
                                        , CU_SuiteInfo **phaseSuites
                                        , uint32_t phaseCount)
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
            else if (argv[i][0] == '-')
            {
                //All these args are of the form -<flag> <Value> so check we have a value:
                if ((i+1) >= argc)
                {
                    printf("Found a flag: %s but there was no following arg!\n", argv[i]);
                    retval = 96;
                    break;
                }

                if (!strcasecmp(argv[i], "-a"))
                {
                    //Admin dir...
                    i++;
                    *adminDir = argv[i];
                    *isTempAdminDir = false;
                }
                else if (!strcasecmp(argv[i], "-at"))
                {
                    // Temp admin dir
                    i++;
                    *adminDir = argv[i];
                    *isTempAdminDir = true;
                }
                else if (!strcasecmp(argv[i], "-p"))
                {
                    //start phase...
                    i++;
                    if (!strcasecmp(argv[i], "f"))
                    {
                        *pPhase = phaseCount;
                    }
                    else
                    {
                        *pPhase = (uint32_t)atoi(argv[i]);

                        if (*pPhase == 0) *pPhase = 1;
                        else if (*pPhase > phaseCount)
                        {
                            printf("Invalid phase %s (max: %u)\n", argv[i], phaseCount);
                            retval = 95;
                        }

                    }
                }
                else if (!strcasecmp(argv[i], "-f"))
                {
                    //final phase...
                    i++;
                    *pFinalPhase = (uint32_t)atoi(argv[i]);

                    if (*pFinalPhase > phaseCount)
                    {
                        printf("Invalid final phase %s (max: %u)\n", argv[i], phaseCount);
                        retval = 94;
                    }
                }
                else if (!strcasecmp(argv[i], "-v"))
                {
                    //verbosity...
                    i++;
                    *pTestLogLevel= (uint32_t)atoi(argv[i]);
                }
                else if (!strcasecmp(argv[i], "-t"))
                {
                    //trace level
                    i++;
                    *pTraceLevel= (uint32_t)atoi(argv[i]);
                }
                else if (!strcasecmp(argv[i], "-s"))
                {
                    // Seed value
                    i++;
                    *pSeedVal = strtoul(argv[i], NULL, 10);
                }
                else if (!strcasecmp(argv[i], "-r"))
                {
                    // Recovery method
                    i++;
                    *recoveryMethod = argv[i];
                }
                else
                {
                    printf("Unknown flag arg: %s\n", argv[i]);
                    retval = 93;
                    break;
                }
            }
            else
            {
                bool suitefound = false;
                int index = atoi(argv[i]);
                int totalSuites = 0;

                CU_SuiteInfo *allSuites = phaseSuites[(*pPhase)-1];
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

                    for(int32_t showPhase=1; showPhase <= phaseCount; showPhase++)
                    {
                        index=1;

                        allSuites = phaseSuites[showPhase-1];

                        curSuite = allSuites;

                        if (phaseCount > 1) printf("Phase %d:\n", showPhase);

                        while(curSuite->pTests)
                        {
                            printf(" %2d : %s\n", index++, curSuite->pName);
                            curSuite++;
                        }
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
            CU_register_suites(phaseSuites[(*pPhase)-1]);
        }
    }

    if (tempSuites) free(tempSuites);

    return retval;
}

static void setupArgsForNextPhase(int argc,
                                  char *argv[],
                                  char *nextPhaseText,
                                  char *adminDir,
                                  bool isTempAdminDir,
                                  char *seedValText,
                                  char *recoveryMethod,
                                  char *NextPhaseArgv[])
{
    int phaseArg = 0, adminArg = 0, seedArg = 0, recoveryMethodArg = 0;
    int i;

    if (argc > MAX_ARGS-4)
    {
        printf("Too many incoming args (%d) max=%d\n", argc, MAX_ARGS-4);
        exit(100);
    }

    // Find the position of any arguments we'll add if not present
    for (i=1; i < argc; i++)
    {
        if (strcasecmp(argv[i], "-p") == 0)
        {
            phaseArg = i;
        }
        else if (strcasecmp(argv[i], "-a") == 0)
        {
            adminArg = i;

            if (isTempAdminDir == true) exit(101);
        }
        else if (strcasecmp(argv[i], "-at") == 0)
        {
            adminArg = i;

            if (isTempAdminDir == false) exit(102);
        }
        else if (strcasecmp(argv[i], "-s") == 0)
        {
            seedArg = i;
        }
        else if (strcasecmp(argv[i], "-r") == 0)
        {
            recoveryMethodArg = i;
        }

        if (phaseArg != 0 && adminArg != 0 && seedArg != 0 && recoveryMethodArg != 0) break;
    }

    int newArg = 0;

    // Deal with program name
    if (strchr(argv[0], '/') == NULL)
    {
        int readLength;
        int length = 1024;
        char *fullpath = malloc(length);

        while((readLength = readlink("/proc/self/exe", fullpath, length)) < 0)
        {
            if (errno == ENAMETOOLONG)
            {
                length *= 2;
                char * tempFullpath = realloc(fullpath, length);
                if(tempFullpath == NULL){
                    free(fullpath);
                    abort();
                }
                fullpath=tempFullpath;
            }
            else
            {
                free(fullpath);
                fullpath = argv[0];
            }
        }

        if (fullpath != argv[0])
        {
            fullpath[readLength] = '\0';
        }

        NextPhaseArgv[newArg++] = fullpath;
    }
    else
    {
        NextPhaseArgv[newArg++] = argv[0];
    }

    // Add missing arguments in first
    if (phaseArg == 0)
    {
        NextPhaseArgv[newArg++] = "-p";
        NextPhaseArgv[newArg++] = nextPhaseText;
    }

    if (adminArg == 0)
    {
        if (isTempAdminDir)
        {
            NextPhaseArgv[newArg++] = "-at";
        }
        else
        {
            NextPhaseArgv[newArg++] = "-a";
        }

        NextPhaseArgv[newArg++] = adminDir;
    }

    if (seedArg == 0)
    {
        NextPhaseArgv[newArg++] = "-s";
        NextPhaseArgv[newArg++] = seedValText;
    }

    if (recoveryMethodArg == 0 && recoveryMethod != NULL)
    {
        NextPhaseArgv[newArg++] = "-r";
        NextPhaseArgv[newArg++] = recoveryMethod;
    }

    // Replace any old arguments, and pick up other arguments
    for(i=1; i<argc; i++)
    {
        NextPhaseArgv[newArg++] = argv[i];

        if (i == phaseArg)
        {
            NextPhaseArgv[newArg++] = nextPhaseText;
            i++;
        }
        else if (i == adminArg)
        {
            NextPhaseArgv[newArg++] = adminDir;
            i++;
        }
        else if (i == seedArg)
        {
            NextPhaseArgv[newArg++] = seedValText;
            i++;
        }
        else if (i == recoveryMethodArg)
        {

            NextPhaseArgv[newArg++] = recoveryMethod;
            i++;
        }
    }

#if 0
    for(i=0; i<newArg; i++)
    {
        printf("%s ", NextPhaseArgv[i]);
    }
    printf("\n");
#endif

    NextPhaseArgv[newArg]=NULL;
}

int test_utils_phaseMain(int argc, char *argv[], CU_SuiteInfo **phaseSuites, uint32_t availablePhases)
{
    int32_t trclvl = 0;
    uint32_t testLogLevel = testLOGLEVEL_TESTDESC;
    int retval = 0;
    char *adminDir=NULL;
    bool isTempAdminDir = false;
    uint32_t phase=1;
    uint32_t finalPhase = availablePhases;
    ism_time_t seedVal = ism_common_currentTimeNanos();
    char *recoveryMethod = NULL;
    bool freeRM = false;

    CU_initialize_registry();

    retval = test_utils_phaseParseArgs( argc
                       , argv
                       , &phase
                       , &finalPhase
                       , &testLogLevel
                       , &trclvl
                       , &recoveryMethod
                       , &seedVal
                       , &adminDir
                       , &isTempAdminDir
                       , phaseSuites
                       , availablePhases );

    if (retval != OK) goto mod_exit;

    srand(seedVal);

    // Choose what recovery method to use for restart phases -- once a method is chosen all
    // subsequent phases use the same method.
    if (recoveryMethod == NULL && phase != 1)
    {
        recoveryMethod = malloc(5);
        sprintf(recoveryMethod, "%d", (int32_t)(rand()%(ismENGINE_VALUE_USE_FULL_NEW_RECOVERY+2)));
        freeRM = true;
    }

    if (recoveryMethod != NULL)
    {
        setenv("IMA_TEST_USE_RECOVERY_METHOD", recoveryMethod, false);
    }

    test_setLogLevel(testLogLevel);
    retval = test_processInit(trclvl, adminDir);

    if (retval == 0)
    {
        char localAdminDir[1024];
        if (adminDir == NULL && (test_getGlobalAdminDir(localAdminDir, sizeof(localAdminDir))) == true)
        {
            adminDir = localAdminDir;
            if (phase == 1) isTempAdminDir = true;
        }
        else if (adminDir != NULL && isTempAdminDir)
        {
            test_process_setGlobalTempAdminDir(adminDir);
        }

        char resultsFilePrefix[strlen(adminDir)+30];

        sprintf(resultsFilePrefix, "%s/CU_Results", adminDir);

        CU_set_output_filename(resultsFilePrefix);

        if (availablePhases > 1) printf("Running phase %u, final phase %u.\n", phase, finalPhase);

        CU_automated_run_tests();

        CU_pRunSummary CU_ThisSummary = CU_get_run_summary();
        CU_pTestRegistry CU_ThisRegistry = CU_get_registry();

        FILE *fp;

        strcat(resultsFilePrefix, "-PrevSummary.dat");

        CU_RunSummary CU_PrevSummary = {0};
        CU_TestRegistry CU_PrevRegistry = {0};

        if (phase > 1)
        {
            if ((fp = fopen(resultsFilePrefix, "rb")) != NULL)
            {
                size_t read_items = fread(&CU_PrevSummary, sizeof(CU_RunSummary), 1, fp);

                if (read_items != 1)
                {
                    printf("Failed to read summary of previous phase. rc = %d, errno = %d", retval, errno);
                }
                read_items = fread(&CU_PrevRegistry, sizeof(CU_TestRegistry), 1, fp);

                if (read_items != 1)
                {
                    printf("Failed to read test registry of previous phase. rc = %d, errno = %d", retval, errno);
                }
                CU_ThisSummary->nSuitesRun += CU_PrevSummary.nTestsRun;
                CU_ThisSummary->nSuitesFailed += CU_PrevSummary.nSuitesFailed;
                CU_ThisSummary->nSuitesInactive += CU_PrevSummary.nSuitesInactive;
                CU_ThisSummary->nTestsRun += CU_PrevSummary.nTestsRun;
                CU_ThisSummary->nTestsFailed += CU_PrevSummary.nTestsFailed;
                CU_ThisSummary->nTestsInactive += CU_PrevSummary.nTestsInactive;
                CU_ThisSummary->nAsserts += CU_PrevSummary.nAsserts;
                CU_ThisSummary->nAssertsFailed += CU_PrevSummary.nAssertsFailed;
                CU_ThisSummary->ElapsedTime += CU_PrevSummary.ElapsedTime;

                CU_ThisRegistry->uiNumberOfSuites += CU_PrevRegistry.uiNumberOfSuites;
                CU_ThisRegistry->uiNumberOfTests += CU_PrevRegistry.uiNumberOfTests;

                fclose(fp);
            }
        }

        if ((fp = fopen(resultsFilePrefix, "wb")) != NULL)
        {
            fwrite(CU_ThisSummary, sizeof(CU_RunSummary), 1, fp);
            fwrite(CU_ThisRegistry, sizeof(CU_TestRegistry), 1, fp);
            fclose(fp);
        }

        if ((CU_ThisSummary->nTestsFailed > 0) ||
            (CU_ThisSummary->nAssertsFailed > 0))
        {
            retval = 1;
        }

        if (phase < finalPhase)
        {
            char *nextPhaseArgv[MAX_ARGS] = {0};
            char nextPhaseText[16];
            sprintf(nextPhaseText, "%d", phase+1);
            char seedValText[32];
            sprintf(seedValText, "%lu", seedVal);
            setupArgsForNextPhase(argc, argv, nextPhaseText, adminDir, isTempAdminDir, seedValText, recoveryMethod, nextPhaseArgv);
            retval = test_execv(nextPhaseArgv[0], nextPhaseArgv);
            printf("'test_execv' failed. rc = %d", retval);
            exit(99);
        }
        else
        {
            printf("\nRandom Seed = %"PRId64"\n", seedVal);

            char *resultString = CU_get_run_results_string();

            if (resultString != NULL)
            {
                printf("%s\n", resultString);
                free(resultString);
            }
            else
            {
                printf("[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
                       CU_ThisSummary->nTestsRun,
                       CU_ThisSummary->nTestsFailed,
                       CU_ThisSummary->nAssertsFailed);
            }

            test_processTerm(retval == 0);
        }

        CU_ThisRegistry->uiNumberOfSuites -= CU_PrevRegistry.uiNumberOfSuites;
        CU_ThisRegistry->uiNumberOfTests -= CU_PrevRegistry.uiNumberOfTests;
    }

    if (freeRM) free(recoveryMethod);

mod_exit:

    CU_cleanup_registry();

    return retval;
}
