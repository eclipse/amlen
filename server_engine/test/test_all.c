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

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "engineInternal.h"

#include "test_utils_initterm.h"

typedef struct tag_SuiteSet
{
    char         *SetName;
    CU_SuiteInfo *Suite;
} SuiteSet;

extern CU_SuiteInfo ISM_Engine_CUnit_basicsuites[];
extern CU_SuiteInfo ISM_TopicTree_CUnit_allsuites[];
extern CU_SuiteInfo ISM_Engine_CUnit_Q2_Suites[];

//extern CU_SuiteInfo ISM_Engine_CUnit_memHandler_Suites[];
//extern CU_SuiteInfo ISM_Engine_CUnit_simpQ_Suites[];

SuiteSet ISM_Engine_CUnit_SuiteSets[] =
{
    { "Engine", ISM_Engine_CUnit_basicsuites },
    { "TopicTree", ISM_TopicTree_CUnit_allsuites },
    { "Queue2", ISM_Engine_CUnit_Q2_Suites },
    //{ "MemHandler", ISM_Engine_CUnit_memHandler_Suites },
    //{ "SimpQ", ISM_Engine_CUnit_simpQ_Suites },
    { NULL, NULL }
};

int trclvl = 0;
ism_time_t seedVal;

int ParseArgs(int argc, char **argv, SuiteSet *allSets)
{
    int totalCount = 0;
    CU_SuiteInfo *allSuites = NULL;
    CU_SuiteInfo *useSuites = NULL;

    int retval = 0;

    SuiteSet  *curSet = allSets;
    CU_SuiteInfo *curSuite = curSet->Suite;

    while(NULL != curSet->SetName)
    {
        curSuite = curSet->Suite;

        while(NULL != curSuite->pName)
        {
            allSuites = realloc(allSuites, sizeof(CU_SuiteInfo) * (totalCount+2));
            memcpy(&allSuites[totalCount++], curSuite, sizeof(CU_SuiteInfo));
            curSuite++;
        }

        curSet++;
    }

    if (totalCount > 0)
    {
        int tempCount = 0;

        memset(&allSuites[totalCount], 0, sizeof(CU_SuiteInfo));

        if (argc > 1)
        {
            for(int i=1; i<argc; i++)
            {
                int index = atoi(argv[i]);

                if (!strcasecmp(argv[i], "verbose"))
                {
                    CU_basic_set_mode(CU_BRM_VERBOSE);
                }
                else if (!strcasecmp(argv[i], "silent"))
                {
                    CU_basic_set_mode(CU_BRM_SILENT);
                }
                else if (!strcmp(argv[i], "-t"))
                {
                    trclvl = (int)strtoull(argv[++i],NULL,0);
                }
                else if (!strcmp(argv[i], "-r"))
                {
                    seedVal = strtoull(argv[++i],NULL,0);
                }
                else
                {
                    bool setfound = false;
                    bool suitefound = false;

                    curSet = allSets;
                    curSuite = NULL;

                    while(NULL != curSet->SetName)
                    {
                        if (!strcasecmp(argv[i], curSet->SetName))
                        {
                            setfound = true;
                            break;
                        }
                        else
                        {
                            curSuite = curSet->Suite;

                            while(curSuite->pTests)
                            {
                                if (!strcasecmp(curSuite->pName, argv[i]))
                                {
                                    suitefound = true;
                                    break;
                                }

                                curSuite++;
                            }

                            if (suitefound) break;
                        }

                        curSet++;
                    }

                    if (suitefound)
                    {
                        useSuites = realloc(useSuites, sizeof(CU_SuiteInfo) * (tempCount+2));
                        memcpy(&useSuites[tempCount++], curSuite, sizeof(CU_SuiteInfo));
                    }
                    else if (setfound)
                    {
                        curSuite = curSet->Suite;

                        while(NULL != curSuite->pName)
                        {
                            useSuites = realloc(useSuites, sizeof(CU_SuiteInfo) * (tempCount+2));
                            memcpy(&useSuites[tempCount++], curSuite, sizeof(CU_SuiteInfo));
                            curSuite++;
                        }
                    }
                    else if (index > 0 && index <= totalCount)
                    {
                        useSuites = realloc(useSuites, sizeof(CU_SuiteInfo)*(tempCount+2));
                        memcpy(&useSuites[tempCount++], &allSuites[index-1], sizeof(CU_SuiteInfo));
                    }
                    else
                    {
                        printf("Invalid test set / suite '%s' specified, the following are valid:\n\n", argv[i]);

                        curSet = allSets;

                        int thisSuite = 1;

                        while(NULL != curSet->SetName)
                        {
                            printf("%s:\n", curSet->SetName);

                            curSuite = curSet->Suite;

                            while(curSuite->pTests)
                            {
                                printf("  Suite %-3d  %s\n", thisSuite++, curSuite->pName);
                                curSuite++;
                            }

                            curSet++;
                        }

                        printf("\n");
                        retval = 99;
                        break;
                    }
                }
            }

            if (retval == 0 && tempCount != 0)
            {
                memset(&useSuites[tempCount], 0, sizeof(CU_SuiteInfo));
            }
        }

        if (tempCount == 0)
        {
            useSuites = allSuites;
        }
    }
    else
    {
        printf("No suites configured\n");
        retval = 98;
    }

    if (retval == 0)
    {
        CU_register_suites(useSuites);
    }

    if (NULL != useSuites) free(useSuites);
    if (NULL != allSuites && useSuites != allSuites) free(allSuites);

    return retval;
}

int main(int argc, char *argv[])
{
    int retval = 0;

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    seedVal = ism_common_currentTimeNanos();

    CU_initialize_registry();

    retval = ParseArgs(argc, argv, ISM_Engine_CUnit_SuiteSets);

    srand(seedVal);

    if (retval == 0)
    {
        ism_common_setTraceLevel(trclvl);

        CU_basic_run_tests();

        CU_RunSummary * CU_pRunSummary_Final;
        CU_pRunSummary_Final = CU_get_run_summary();
        printf("Random Seed =    %"PRId64"\n", seedVal);
        printf("\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
               CU_pRunSummary_Final->nTestsRun,
               CU_pRunSummary_Final->nTestsFailed,
               CU_pRunSummary_Final->nAssertsFailed);
        if ((CU_pRunSummary_Final->nTestsFailed > 0) ||
            (CU_pRunSummary_Final->nAssertsFailed > 0))
        {
            retval = CU_pRunSummary_Final->nTestsFailed + CU_pRunSummary_Final->nAssertsFailed;
        }
    }

    CU_cleanup_registry();

    int32_t rc = test_processTerm(retval == 0);
    if (retval == 0) retval = rc;

mod_exit:

    return retval;
}
