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
/* Module Name: test_failedAllocs.c                                  */
/*                                                                   */
/* Description: Cunit tests of targetted alloc failures.             */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "topicTree.h"
#include "topicTreeInternal.h"
#include "clientState.h"
#include "clientStateInternal.h"
#include "clientStateExpiry.h"
#include "policyInfo.h"
#include "queueNamespace.h"     // ieqn functions & constants
#include "messageExpiry.h"
#include "remoteServers.h"
#include "multiConsumerQInternal.h"
#include "engineStore.h"
#include "resourceSetStats.h"

#include "test_utils_initterm.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_memory.h"
#include "test_utils_client.h"
#include "test_utils_security.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_file.h"

uint32_t analysisType = TEST_ANALYSE_IEMEM_NONE;

#if defined(NO_MALLOC_WRAPPER)
#define IEMEM_PROBE(a, b) 0
#define IEMEM_GET_MEMORY_PROBEID(a) 0
#endif

// Swallow FDCs if we're forcing failures
uint32_t swallowed_ffdc_count = 0;
bool swallow_ffdcs = false;
bool swallow_expect_core = false;
void ieut_ffdc( const char *function
              , uint32_t seqId
              , bool core
              , const char *filename
              , uint32_t lineNo
              , char *label
              , uint32_t retcode
              , ... )
{
    static void (*real_ieut_ffdc)(const char *, uint32_t, bool, const char *, uint32_t, char *, uint32_t, ...) = NULL;

    if (real_ieut_ffdc == NULL)
    {
        real_ieut_ffdc = dlsym(RTLD_NEXT, "ieut_ffdc");
    }

    if (swallow_ffdcs == true)
    {
        TEST_ASSERT_EQUAL(swallow_expect_core, core);
        __sync_add_and_fetch(&swallowed_ffdc_count, 1);
        return;
    }

    TEST_ASSERT(0, ("Unexpected FFDC from %s:%u", filename, lineNo));
}

// Override ism_common_startThread so it can be forced to fail
const char *override_startThreadFunc = NULL;
int override_startThreadRc = 0;
int ism_common_startThread(ism_threadh_t * handle,
                           ism_thread_func_t addr,
                           void * data,
                           void * context,
                           int value,
                           enum thread_usage_e usage,
                           int flags,
                           const char * name,
                           const char * description)
{
    int rc;
    static int32_t (*real_ism_common_startThread)(ism_threadh_t *,
                                                  ism_thread_func_t,
                                                  void *,
                                                  void *,
                                                  int,
                                                  enum thread_usage_e,
                                                  int,
                                                  const char *,
                                                  const char *) = NULL;

    if (real_ism_common_startThread == NULL)
    {
        real_ism_common_startThread = dlsym(RTLD_NEXT, "ism_common_startThread");
    }

    if (override_startThreadFunc != NULL && name != NULL && strcmp(name, override_startThreadFunc) == 0)
    {
        rc = override_startThreadRc;
    }
    else
    {
        rc = real_ism_common_startThread(handle, addr, data, context, value, usage, flags, name, description);
    }

    return rc;
}

//****************************************************************************
/// @brief Test the capabilities of the topic tree hash tables when allocs fail.
//****************************************************************************
void test_capability_HashTableErrors(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int32_t expectedRC = OK;
    ieutHashTable_t *table = NULL;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    uint32_t failSequence1[][3] = {// Fail in ieut_createHashTable creating hash table
                                   {IEMEM_PROBE(iemem_subsTree, 60000), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable creating chains
                                   {IEMEM_PROBE(iemem_subsTree, 60001), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        rc = ieut_createHashTable(pThreadData, 100, iemem_subsTree, &table);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
        TEST_ASSERT_PTR_NULL(table);
    }

    // Actually create the table!
    failProbes[0] = 0; probeCounts[0] = 0;

    if (analysisType == TEST_ANALYSE_IEMEM_NONE)
    {
        rc = ieut_createHashTable(pThreadData, 100, iemem_subsTree, &table);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(table);
    }

    char *key = "TESTKEY";
    uint32_t keyHash = iett_generateTopicStringHash(key);
    char *value = "TESTVALUE";
    size_t valueLength = strlen(value)+1;

    uint32_t failSequence2[][3] = {// Fail in ieut_putHashEntry duplicating value
                                   {IEMEM_PROBE(iemem_subsTree, 60003), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry allocating chain
                                   {IEMEM_PROBE(iemem_subsTree, 60004), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry duplicating key string
                                   {IEMEM_PROBE(iemem_subsTree, 60002), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<sizeof(failSequence2)/sizeof(failSequence2[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence2[loop][0];
            probeCounts[0] = failSequence2[loop][1];
            expectedRC = failSequence2[loop][2];
        }

        // Fail duplicating key
        rc = ieut_putHashEntry(pThreadData,
                               table,
                               ieutFLAG_DUPLICATE_KEY_STRING | ieutFLAG_DUPLICATE_VALUE,
                               key, keyHash, value, valueLength);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // Fail resizing
    failProbes[0] = IEMEM_PROBE(iemem_subsTree, 60005);
    probeCounts[0] = 0;

    rc = ieut_resizeHashTable(pThreadData, table, 200);
    TEST_ASSERT_EQUAL(rc, ISMRC_AllocateError);

    // Check that the table is usable after failures
    failProbes[0] = 0; probeCounts[0] = 0;
    rc = ieut_putHashEntry(pThreadData,
                           table,
                           ieutFLAG_DUPLICATE_KEY_STRING | ieutFLAG_DUPLICATE_VALUE | ieutFLAG_REPLACE_EXISTING,
                           key, keyHash, value, valueLength);
    TEST_ASSERT_EQUAL(rc, OK);

    ieut_destroyHashTable(pThreadData, table);

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);
}

//****************************************************************************
/// @brief Test the capabilities of the engine hash sets when allocs fail.
//****************************************************************************
void test_capability_HashSetErrors(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int32_t expectedRC = OK;
    ieutHashSet_t *set = NULL;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    uint32_t failSequence1[][3] = {// Fail in ieut_createHashSet creating hash set
                                   {IEMEM_PROBE(iemem_subsQuery, 60100), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashSet creating chains
                                   {IEMEM_PROBE(iemem_subsQuery, 60101), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        rc = ieut_createHashSet(pThreadData, iemem_subsQuery, &set);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
        TEST_ASSERT_PTR_NULL(set);
    }

    // Actually create the set!
    failProbes[0] = 0; probeCounts[0] = 0;

    if (analysisType == TEST_ANALYSE_IEMEM_NONE)
    {
        rc = ieut_createHashSet(pThreadData, iemem_subsQuery, &set);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(set);
    }

    uint64_t value = 1;

    uint32_t failSequence2[][3] = {// Fail in ieut_addValueToHashSet allocating the chain
                                   {IEMEM_PROBE(iemem_subsQuery, 60103), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<sizeof(failSequence2)/sizeof(failSequence2[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence2[loop][0];
            probeCounts[0] = failSequence2[loop][1];
            expectedRC = failSequence2[loop][2];
        }

        rc = ieut_addValueToHashSet(pThreadData, set, value);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    // While we're here let's test some things.

    // Test numbers on the same chain...
    uint64_t values[] = {0, 193, 386, 579, 772, 965, 1158};
    for(loop=0; loop<sizeof(values)/sizeof(values[0]); loop++)
    {
        rc = ieut_addValueToHashSet(pThreadData, set, values[loop]);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    TEST_ASSERT_EQUAL(set->totalCount, 7);
    rc = ieut_findValueInHashSet(set, values[1]);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ieut_findValueInHashSet(set, values[5]);
    TEST_ASSERT_EQUAL(rc, OK);
    ieut_removeValueFromHashSet(set, values[2]);
    TEST_ASSERT_EQUAL(set->totalCount, 6);
    ieut_removeValueFromHashSet(set, values[2]);
    TEST_ASSERT_EQUAL(set->totalCount, 6);
    ieut_removeValueFromHashSet(set, values[6]);
    TEST_ASSERT_EQUAL(set->totalCount, 5);

    ieut_destroyHashSet(pThreadData, set);
}

//****************************************************************************
/// @brief Test the capabilities of split lists when allocs fail
//****************************************************************************
void test_capability_SplitListErrors(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int32_t expectedRC = OK;
    ieutSplitList_t *list = NULL;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    uint32_t failSequence1[][3] = {// Fail in ieut_createSplitList creating split list structure
                                   {IEMEM_PROBE(iemem_messageExpiryData, 60200), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createSplitList creating chains
                                   {IEMEM_PROBE(iemem_messageExpiryData, 60201), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        rc = ieut_createSplitList(pThreadData, 0, iemem_messageExpiryData, &list);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
        TEST_ASSERT_PTR_NULL(list);
    }

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);
}

//****************************************************************************
/// @brief Test the capabilities of deferred free lists when allocs fail
//****************************************************************************
void test_capability_DeferredFreeErrors(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    ieutDeferredFreeList_t list;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;
    void *test_area;

    printf("Starting %s...\n", __func__);

    // Need to pretend this is a thread already in the engine
    ieut_enteringEngine(NULL);

    // Need a deferred free list to work with.
    rc = ieut_initDeferredFreeList(pThreadData, &list);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    uint32_t failSequence1[][3] = {// Fail in ieut_addDeferredFree creating new areas
                                   {IEMEM_PROBE(iemem_deferredFreeLists, 2), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        test_area = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_policyInfo, 99), 20);
        TEST_ASSERT_PTR_NOT_NULL(test_area);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
        }

        ieut_addDeferredFree(pThreadData, &list, test_area, NULL, iemem_policyInfo, iereNO_RESOURCE_SET);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        iemem_free(pThreadData, iemem_policyInfo, test_area);
    }

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    swallow_ffdcs = true;
    swallowed_ffdc_count = 0;

    #ifdef NDEBUG
    swallow_expect_core = false;
    #else
    swallow_expect_core = true;
    #endif

    // Force force the FFDC about adding the number of unfreed deferred frees growing
    for(loop=0; loop<ieutDEFERREDFREE_AREA_MAX_INCREMENT; loop++)
    {
        test_area = iemem_malloc(pThreadData, IEMEM_PROBE(iemem_policyInfo, 99), 10);
        ieut_addDeferredFree(pThreadData, &list, test_area, NULL, iemem_policyInfo, iereNO_RESOURCE_SET);
    }
    TEST_ASSERT_EQUAL(swallowed_ffdc_count, 1);

    ieut_destroyDeferredFreeList(pThreadData, &list);

    ieut_leavingEngine(pThreadData);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_UtilityDataTypeErrors[] =
{
    { "HashTableErrors", test_capability_HashTableErrors },
    { "HashSetErrors", test_capability_HashSetErrors },
    { "SplitListErrors", test_capability_SplitListErrors },
    { "DeferredFreeErrors", test_capability_DeferredFreeErrors },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the capabilities of subscriptions when allocs fail.
//****************************************************************************
void test_capability_SubscriptionErrors(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int32_t expectedRC = OK;
    ismEngine_Consumer_t  *hConsumer = NULL;
    int32_t messageCount;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t loop;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};

    rc = test_createClientAndSession("FailedAllocsClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    // *** AT_MOST_ONCE NON-DURABLE ***
    uint32_t failSequence1[][3] = {// Fail in ism_engine_newDestination
                                   {IEMEM_PROBE(iemem_externalObjs, 1), 0, ISMRC_AllocateError},
                                   // Fail in ism_engine_createConsumer
                                   {IEMEM_PROBE(iemem_externalObjs, 4), 0, ISMRC_AllocateError},
                                   // Fail in iett_allocateSubscription
                                   {IEMEM_PROBE(iemem_subsTree, 6), 0, ISMRC_AllocateError},
                                   // Fail in iett_allocateSubQueueName allocating with clientId.
                                   {IEMEM_PROBE(iemem_subsTree, 8), 0, ISMRC_AllocateError},
                                   // Fail in iesq_createQ creating the queue structure
                                   {IEMEM_PROBE(iemem_simpleQ, 1), 0, ISMRC_AllocateError},
                                   // Fail in iesq_createQ allocating the queue name
                                   {IEMEM_PROBE(iemem_simpleQ, 2), 0, ISMRC_AllocateError},
                                   // Fail in iesq_createNewPage allocating the first page
                                   {IEMEM_PROBE(iemem_simpleQPage, 1), 0, ISMRC_AllocateError},
                                   // Fail in iett_analyseTopicString
                                   {IEMEM_PROBE(iemem_topicAnalysis, 3), 0, ISMRC_AllocateError},
                                   // Fail in iett_insertOrFindSubsNode allocating new node 1
                                   {IEMEM_PROBE(iemem_subsTree, 1), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable creating hash table
                                   {IEMEM_PROBE(iemem_subsTree, 60000), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable creating chains
                                   {IEMEM_PROBE(iemem_subsTree, 60001), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry allocating chain
                                   {IEMEM_PROBE(iemem_subsTree, 60004), 0, ISMRC_AllocateError},
                                   // Fail in iett_insertOrFindSubsNode allocating new node 2
                                   {IEMEM_PROBE(iemem_subsTree, 1), 0, ISMRC_AllocateError},
                                   // Fail in iett_insertOrFindSubsNode allocating new node 3
                                   {IEMEM_PROBE(iemem_subsTree, 1), 1, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry allocating chain
                                   {IEMEM_PROBE(iemem_subsTree, 2), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<(sizeof(failSequence1)/sizeof(failSequence1[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
        rc = ism_engine_createConsumer(hSession, ismDESTINATION_TOPIC, "TEST.TOPIC.1/+/#",
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &messageCount, sizeof(int32_t),
                                       NULL, /* No delivery callback */
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_NONE,
                                       &hConsumer,
                                       NULL, 0, NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
        TEST_ASSERT_PTR_NULL(hConsumer);
    }

    // *** AT_LEAST_ONCE NON-DURABLE ***
    uint32_t failSequence2[][3] = {// Fail in ism_engine_newDestination
                                   {IEMEM_PROBE(iemem_externalObjs, 1), 0, ISMRC_AllocateError},
                                   // Fail in ism_engine_createConsumer
                                   {IEMEM_PROBE(iemem_externalObjs, 4), 0, ISMRC_AllocateError},
                                   // Fail in iett_allocateSubscription
                                   {IEMEM_PROBE(iemem_subsTree, 6), 0, ISMRC_AllocateError},
                                   // Fail in iett_allocateSubQueueName allocating with clientId.
                                   {IEMEM_PROBE(iemem_subsTree, 8), 0, ISMRC_AllocateError},
                               #ifdef ENGINE_FORCE_INTERMEDIATE_TO_MULTI
                                   // Fail in iemq_createQ creating the queue structure
                                   {IEMEM_PROBE(iemem_multiConsumerQ, 1), 0, ISMRC_AllocateError},
                                   // Fail in iemq_createQ allocating the queue name
                                   {IEMEM_PROBE(iemem_multiConsumerQ, 2), 0, ISMRC_AllocateError},
                                   // Fail in iemq_createNewPage allocating the first page
                                   {IEMEM_PROBE(iemem_multiConsumerQPage, 1), 0, ISMRC_AllocateError},
                               #else
                                   // Fail in ieiq_createQ creating the queue structure
                                   {IEMEM_PROBE(iemem_intermediateQ, 1), 0, ISMRC_AllocateError},
                                   // Fail in ieiq_createQ allocating the queue name
                                   {IEMEM_PROBE(iemem_intermediateQ, 2), 0, ISMRC_AllocateError},
                                   // Fail in ieiq_createNewPage allocating the first page
                                   {IEMEM_PROBE(iemem_intermediateQPage, 1), 0, ISMRC_AllocateError},
                               #endif
                                   // Fail in iett_analyseTopicString
                                   {IEMEM_PROBE(iemem_topicAnalysis, 3), 0, ISMRC_AllocateError},
                                   // Fail in iett_insertOrFindSubsNode allocating a new node
                                   {IEMEM_PROBE(iemem_subsTree, 1), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry allocating chain
                                   {IEMEM_PROBE(iemem_subsTree, 60004), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry allocating new subscriber list
                                   {IEMEM_PROBE(iemem_subsTree, 2), 0, ISMRC_AllocateError},
                               };

    for(loop=0; loop<sizeof(failSequence2)/sizeof(failSequence2[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence2[loop][0];
            probeCounts[0] = failSequence2[loop][1];
            expectedRC = failSequence2[loop][2];
        }

        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
        rc = ism_engine_createConsumer(hSession, ismDESTINATION_TOPIC, "TEST.TOPIC.2",
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &messageCount, sizeof(int32_t),
                                       NULL, /* No delivery callback */
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                       &hConsumer,
                                       NULL, 0, NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
        TEST_ASSERT_PTR_NULL(hConsumer);
    }

    // *** TRANSACTION_CAPABLE (JMS) NON-DURABLE ***
    uint32_t failSequence3[][3] = {// Fail in ism_engine_newDestination
                                   {IEMEM_PROBE(iemem_externalObjs, 1), 0, ISMRC_AllocateError},
                                   // Fail in ism_engine_createConsumer
                                   {IEMEM_PROBE(iemem_externalObjs, 4), 0, ISMRC_AllocateError},
                                   // Fail in iett_allocateSubscription
                                   {IEMEM_PROBE(iemem_subsTree, 6), 0, ISMRC_AllocateError},
                                   // Fail in iett_allocateSubQueueName allocating with clientId.
                                   {IEMEM_PROBE(iemem_subsTree, 8), 0, ISMRC_AllocateError},
                                   // Fail in iemq_createQ creating the queue structure
                                   {IEMEM_PROBE(iemem_multiConsumerQ, 1), 0, ISMRC_AllocateError},
                                   // Fail in iemq_createQ allocating the queue name
                                   {IEMEM_PROBE(iemem_multiConsumerQ, 2), 0, ISMRC_AllocateError},
                                   // Fail in iemq_createNewPage allocating the first page
                                   {IEMEM_PROBE(iemem_multiConsumerQPage, 1), 0, ISMRC_AllocateError},
                                   // Fail in iett_analyseTopicString
                                   {IEMEM_PROBE(iemem_topicAnalysis, 3), 0, ISMRC_AllocateError},
                                   // Fail in iett_insertOrFindSubsNode allocating a new node
                                   {IEMEM_PROBE(iemem_subsTree, 1), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry allocating new subscriber list
                                   {IEMEM_PROBE(iemem_subsTree, 2), 0, ISMRC_AllocateError},
                               };

    for(loop=0; loop<sizeof(failSequence3)/sizeof(failSequence3[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence3[loop][0];
            probeCounts[0] = failSequence3[loop][1];
            expectedRC = failSequence3[loop][2];
        }

        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
        rc = ism_engine_createConsumer(hSession, ismDESTINATION_TOPIC, "TEST.TOPIC.3",
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &messageCount, sizeof(int32_t),
                                       NULL, /* No delivery callback */
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_ACK,
                                       &hConsumer,
                                       NULL, 0, NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
        TEST_ASSERT_PTR_NULL(hConsumer);
    }

    // *** TRANSACTION_CAPABLE (JMS) DURABLE ***
    uint32_t failSequence4[][3] = {// Fail in iett_allocateSubscription allocating subscription structure
                                   {IEMEM_PROBE(iemem_subsTree, 6), 0, ISMRC_AllocateError},
                                   // Fail in iett_allocateSubQueueName allocating with subName.
                                   {IEMEM_PROBE(iemem_subsTree, 7), 0, ISMRC_AllocateError},
                                   // Fail in iemq_createQ creating the queue structure
                                   {IEMEM_PROBE(iemem_multiConsumerQ, 1), 0, ISMRC_AllocateError},
                                   // Fail in iett_addSubToEngineTopicTree allocating clientDurables
                                   {IEMEM_PROBE(iemem_subsTree, 3), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry resizing chain
                                   {IEMEM_PROBE(iemem_namedSubs, 60004), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry duplicating key string (client Id)
                                   {IEMEM_PROBE(iemem_namedSubs, 60002), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK}
                               };

    // Allow the faking of requirement to use incremental transaction
    ietrTransactionControl_t *pControl = (ietrTransactionControl_t *)ismEngine_serverGlobal.TranControl;
    uint32_t realStoreTranRsrvOps = pControl->StoreTranRsrvOps;

    for(loop=0; loop<sizeof(failSequence4)/sizeof(failSequence4[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence4[loop][0];
            probeCounts[0] = failSequence4[loop][1];
            expectedRC = failSequence4[loop][2];
        }

        // Debug builds assert that the number of store operations will fit.
        #if defined(NDEBUG)
        if (loop == 4) pControl->StoreTranRsrvOps = 1;
        #endif

        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                              ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
        rc = sync_ism_engine_createSubscription(hClient,
                                                "DURABLESUB1",
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                "TEST.TOPIC.4",
                                                &subAttrs,
                                                NULL); // Owning client same as requesting client

        if (loop == 4) pControl->StoreTranRsrvOps = realStoreTranRsrvOps;

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    ismEngine_SubscriptionHandle_t subscription = NULL;
    rc = iett_findClientSubscription(pThreadData,
                                     "FailedAllocsClient",
                                     iett_generateClientIdHash("FailedAllocsClient"),
                                     "DURABLESUB1",
                                     iett_generateSubNameHash("DURABLESUB1"),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscription);

    uint32_t failSequence5[][3] = {// Fail in iett_findSubscriptionClientIds allocating clientId array
                                   {IEMEM_PROBE(iemem_callbackContext, 14), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<(sizeof(failSequence5)/sizeof(failSequence5[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence5[loop][0];
            probeCounts[0] = failSequence5[loop][1];
            expectedRC = failSequence5[loop][2];
        }

        const char **foundClients = NULL;
        rc = iett_findSubscriptionClientIds(pThreadData, subscription, &foundClients);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    iett_releaseSubscription(pThreadData, subscription, false);

    rc = ism_engine_destroySubscription(hClient, "DURABLESUB1", hClient, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    subscription = NULL;

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create a client
    rc = test_createClientAndSession(__func__,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Bounce the server and check that the durable doesn't come back!
    rc = test_bounceEngine();
    TEST_ASSERT_EQUAL(rc, OK);

    pThreadData = ieut_getThreadData();

    rc = test_createClientAndSession("FailedAllocsClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(pThreadData,
                                     ((ismEngine_ClientState_t *)hClient)->pClientId,
                                     iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient)->pClientId),
                                     "DURABLESUB1",
                                     iett_generateSubNameHash("DURABLESUB1"),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(subscription);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

void test_capability_SharedSubsErrors(void)
{
    int32_t rc;
    int32_t expectedRC = OK;
    ismEngine_ClientStateHandle_t hClient, hOwningClient;
    ismEngine_SessionHandle_t hSession, hOwningSession;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t loop;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};

    rc = test_createClientAndSession("FailedAllocsClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession("__SharedOwner",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hOwningClient, &hOwningSession, false);

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    // Creation failures
    uint32_t failSequence1[][3] = {// Fail in iett_createSubscription allocating the sharing clients array
                                   {IEMEM_PROBE(iemem_externalObjs, 5), 0, ISMRC_AllocateError},
                                   // Fail in iett_createSubscription allocating the clientId
                                   {IEMEM_PROBE(iemem_externalObjs, 6), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence1)/sizeof(failSequence1[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                              ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT;
        rc = sync_ism_engine_createSubscription(hClient,
                                                "FailingSharedSub",
                                                NULL,
                                                ismDESTINATION_TOPIC, "/SHAREDSUB/FAILURES",
                                                &subAttrs,
                                                hOwningClient);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);

    }

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngine_SubscriptionHandle_t subscription = NULL;
    rc = iett_findClientSubscription(pThreadData,
                                     "FailedAllocsClient",
                                     iett_generateClientIdHash("FailedAllocsClient"),
                                     "FailingSharedSub",
                                     iett_generateSubNameHash("FailingSharedSub"),
                                     &subscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(subscription);

    // Creation failures
    uint32_t failSequence2[][3] = {// Fail in iett_findSubscriptionClientIds allocating the array for sharing clients array
                                   {IEMEM_PROBE(iemem_callbackContext, 15), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<(sizeof(failSequence2)/sizeof(failSequence2[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence2[loop][0];
            probeCounts[0] = failSequence2[loop][1];
            expectedRC = failSequence2[loop][2];
        }

        const char **foundClients = NULL;
        rc = iett_findSubscriptionClientIds(pThreadData, subscription, &foundClients);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    iett_releaseSubscription(pThreadData, subscription, false);

    ismEngine_ClientStateHandle_t hAdditionalClient[iettSHARED_SUBCLIENT_INCREMENT];

    // Set things up to be about to add the last sharing client
    for(int32_t i=0; i<iettSHARED_SUBCLIENT_INCREMENT; i++)
    {
        char clientId[50];

        sprintf(clientId, "AdditionalClient%d", i+1);

        rc = sync_ism_engine_createClientState(clientId,
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                               NULL,
                                               NULL,
                                               NULL,
                                               &hAdditionalClient[i]);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hAdditionalClient[i]);

        if (i<iettSHARED_SUBCLIENT_INCREMENT-1)
        {
            subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                                  ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT;
            rc = ism_engine_reuseSubscription(hAdditionalClient[i],
                                              "FailingSharedSub",
                                              &subAttrs,
                                              hOwningClient);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    // Reusing failures
    uint32_t failSequence3[][3] = {// Fail in iett_shareSubscription re-allocating the sharing clients array
                                   {IEMEM_PROBE(iemem_externalObjs, 7), 0, ISMRC_AllocateError},
                                   // Fail in iett_shareSubscription allocating the new clientId
                                   {IEMEM_PROBE(iemem_externalObjs, 8), 0, ISMRC_AllocateError},
                                   // Fail having faked that this subscription is already marked deleted
                                   {0, 0, ISMRC_NotFound},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence3)/sizeof(failSequence3[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence3[loop][0];
            probeCounts[0] = failSequence3[loop][1];
            expectedRC = failSequence3[loop][2];
        }

        subscription = NULL;

        // Fake up the subscription being marked logically deleted
        if (expectedRC == ISMRC_NotFound)
        {
            rc = iett_findClientSubscription(ieut_getThreadData(),
                                             hClient->pClientId,
                                             iett_generateClientIdHash(hClient->pClientId),
                                             "FailingSharedSub",
                                             iett_generateSubNameHash("FailingSharedSub"),
                                             &subscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(subscription);

            subscription->internalAttrs |= iettSUBATTR_DELETED;
        }

        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_SHARED |
                              ismENGINE_SUBSCRIPTION_OPTION_ADD_CLIENT;
        rc = ism_engine_reuseSubscription(hAdditionalClient[iettSHARED_SUBCLIENT_INCREMENT-1],
                                          "FailingSharedSub",
                                          &subAttrs,
                                          hOwningClient);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);

        if (subscription != NULL)
        {
            subscription->internalAttrs &= ~iettSUBATTR_DELETED;
            iett_releaseSubscription(ieut_getThreadData(), subscription, true);
        }
    }

    // List subscriptions failures
    uint32_t failSequence4[][3] = {// Fail in iett_listSubscriptions allocating the array to hold subscriptions
                                   {IEMEM_PROBE(iemem_callbackContext, 8), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<(sizeof(failSequence4)/sizeof(failSequence4[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence4[loop][0];
            probeCounts[0] = failSequence4[loop][1];
            expectedRC = failSequence4[loop][2];
        }

        rc = ism_engine_listSubscriptions(hClient, NULL, NULL, NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    for(int32_t i=0; i<iettSHARED_SUBCLIENT_INCREMENT; i++)
    {
        rc = sync_ism_engine_destroyClientState(hAdditionalClient[i], ismENGINE_DESTROY_CLIENT_OPTION_DISCARD);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hOwningClient, hOwningSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_SubscriptionErrors[] =
{
    { "SubscriptionErrors", test_capability_SubscriptionErrors },
    { "SharedSubsErrors", test_capability_SharedSubsErrors },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the capabilities of the topic tree when allocs fail.
//****************************************************************************
void test_capability_TopicTreeErrors(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int32_t expectedRC = OK;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    // Remove the topic tree created by the infrastructure
    iett_destroyEngineTopicTree(pThreadData);

    uint32_t failSequence1[][3] = {// Fail in iett_createTopicTree allocating tree structure
                                   {IEMEM_PROBE(iemem_subsTree, 4), 0, ISMRC_AllocateError},
                                   // Fail in iett_createTopicTree allocating head of subscription tree
                                   {IEMEM_PROBE(iemem_subsTree, 5), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable allocating the named subscription hash table
                                   {IEMEM_PROBE(iemem_namedSubs, 60000), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable allocating the named subscription chains
                                   {IEMEM_PROBE(iemem_namedSubs, 60001), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable allocating head of the topics tree
                                   {IEMEM_PROBE(iemem_topicsTree, 2), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable allocating head of the remote servers tree
                                   {IEMEM_PROBE(iemem_remoteServers, 8), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        rc = iett_initEngineTopicTree(pThreadData);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // Cache creation failure is tolerated - i.e. will result in a valid tree, so force
    // those errors individually now.
    failProbes[0] = IEMEM_PROBE(iemem_subscriberCache, 60000); // Fail in ieut_createHashTable allocating sublist cache table
    probeCounts[0] = 0;
    rc = iett_initEngineTopicTree(pThreadData);
    TEST_ASSERT_EQUAL(rc, OK);
    iett_destroyEngineTopicTree(pThreadData);

    failProbes[0] = IEMEM_PROBE(iemem_subscriberCache, 60001); // Fail in ieut_createHashTable allocating sublist cache chains
    probeCounts[0] = 0;
    rc = iett_initEngineTopicTree(pThreadData);
    TEST_ASSERT_EQUAL(rc, OK);
    iett_destroyEngineTopicTree(pThreadData);

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    // Make sure there is a topic tree for the infrastructure to remove!
    rc = iett_initEngineTopicTree(pThreadData);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_TopicTreeErrors[] =
{
    { "TopicTreeErrors", test_capability_TopicTreeErrors },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the capabilities of publication when allocs fail.
//****************************************************************************
void test_capability_PublicationErrors(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int32_t expectedRC = OK;
    ismEngine_Consumer_t  *hConsumer[3] = {0};
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t messageCount[3];
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    int32_t loop;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};

    rc = test_createClientAndSession("FailedAllocsClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Create non-durable subscriptions
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession, ismDESTINATION_TOPIC, "PUBLISH/ERRORS/TOPIC",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &messageCount[0], sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer[0],
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer[0]);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
    rc = ism_engine_createConsumer(hSession, ismDESTINATION_TOPIC, "PUBLISH/ERRORS/TOPIC",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &messageCount[1], sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK | ismENGINE_CONSUMER_OPTION_RECORD_SHORT_DELIVERYID,
                                   &hConsumer[1],
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer[1]);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
    rc = ism_engine_createConsumer(hSession, ismDESTINATION_TOPIC, "PUBLISH/ERRORS/TOPIC",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &messageCount[2], sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer[2],
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer[2]);

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    char *payload="PAYLOAD";
    ismEngine_MessageHandle_t hMessage;

    // *** Create a message
    uint32_t failSequence1[][3] = {// Fail in ism_engine_createMessage allocating message body
                                   {IEMEM_PROBE(iemem_messageBody, 1), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence1)/sizeof(failSequence1[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        void *payloadPtr = payload;
        rc = test_createMessage(strlen(payload)+1,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, "PUBLISH/ERRORS/TOPIC",
                                &hMessage, &payloadPtr);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    ism_engine_releaseMessage(hMessage);

    // Create a subscriber list with which to check various aspects of the topic tree after
    // each procedure.
    iettSubscriberList_t checkList = {0};

    checkList.topicString = "PUBLISH/ERRORS/TOPIC";

    rc = iett_getSubscriberList(pThreadData, &checkList);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(checkList.subscriberNodeCount, 1);
    TEST_ASSERT_PTR_NOT_NULL(checkList.subscriberNodes[0]);
    TEST_ASSERT_EQUAL(checkList.subscriberNodes[0]->useCount, 1);

    // Start from a clean cache again!
    // ieut_clearHashTable(iett_getEngineTopicTree()->cache);

    // *** Non transactional, non-persistent, QoS 2, retained ***
    uint32_t failSequence2[][3] = {// Fail in iett_analyseTopicString allocating topic string copy
                                   {IEMEM_PROBE(iemem_topicAnalysis, 3), 0, ISMRC_AllocateError},
                                   // Fail in iett_insertOrFindTopicsNode allocating node 'ERRS'
                                   {IEMEM_PROBE(iemem_topicsTree, 1), 1, ISMRC_AllocateError},
                                   // Fail in iett_insertOrFindTopicsNode allocating node 'TOPIC'
                                   {IEMEM_PROBE(iemem_topicsTree, 1), 1, ISMRC_AllocateError},
                                   // Fail in iett_insertOrFindOriginServer allocating origin server record
                                   {IEMEM_PROBE(iemem_remoteServers, 17), 0, ISMRC_AllocateError},
                                   // Fail adding origin server record to the origin server table
                                   {IEMEM_PROBE(iemem_remoteServers, 60004), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable creating topic node hash header
                                   {IEMEM_PROBE(iemem_topicsTree, 60000), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable creating topic node hash chain
                                   {IEMEM_PROBE(iemem_topicsTree, 60001), 0, ISMRC_AllocateError},
                                   // Fail in ielm_takeLock locking the message
                                   {IEMEM_PROBE(iemem_lockManager, 4), 0, ISMRC_AllocateError},
                                   // Fail in ielm_takeLock allocating release block
                                   {IEMEM_PROBE(iemem_lockManager, 5), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable allocating the pThreadData->sublistCache
                                   {IEMEM_PROBE(iemem_subscriberCache, 60000), 0, OK},
                                   // Fail in iett_getSubscriberList under iett_releaseSubscriberList allocating SubsNodes
                                   {IEMEM_PROBE(iemem_subsQuery, 3), 0, OK},
                                   // Fail in iett_getSubscriberList under iett_releaseSubscriberList allocating Subscribers
                                   {IEMEM_PROBE(iemem_subsQuery, 4), 0, OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence2)/sizeof(failSequence2[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence2[loop][0];
            probeCounts[0] = failSequence2[loop][1];
            expectedRC = failSequence2[loop][2];
        }

        void *payloadPtr = payload;
        rc = test_createMessage(strlen(payload)+1,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, "PUBLISH/ERRORS/TOPIC",
                                &hMessage, &payloadPtr);
        TEST_ASSERT_EQUAL(rc, OK);

        if (loop == 8)
        {
            swallow_ffdcs = true;
            swallow_expect_core = false;
        }

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                "PUBLISH/ERRORS/TOPIC",
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
        TEST_ASSERT_EQUAL(checkList.subscriberNodes[0]->useCount, 1);

        swallow_ffdcs = false;
    }

    iett_releaseSubscriberList(pThreadData, &checkList);

    // MORE HERE...

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_PublicationErrors[] =
{
    { "PublicationErrors", test_capability_PublicationErrors },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the capabilities of delivery when allocs fail.
//****************************************************************************
static uint32_t numDeliveryHandlerCalls = 0;

void deliveryFailureHandler(
        int32_t                       rc,
        ismEngine_ClientStateHandle_t hClient,
        ismEngine_ConsumerHandle_t    hConsumer,
        void                         *consumerContext)
{
    __sync_add_and_fetch(&numDeliveryHandlerCalls, 1);

    //Allow all memory to be alloc'd again...
    test_memoryFailMemoryProbes(NULL, NULL);
}

bool deliveryErrorsMsgArrivedCB(
        ismEngine_ConsumerHandle_t      hConsumer,
        ismEngine_DeliveryHandle_t      hDelivery,
        ismEngine_MessageHandle_t       hMessage,
        uint32_t                        deliveryId,
        ismMessageState_t               state,
        uint32_t                        destinationOptions,
        ismMessageHeader_t *            pMsgDetails,
        uint8_t                         areaCount,
        ismMessageAreaType_t            areaTypes[areaCount],
        size_t                          areaLengths[areaCount],
        void *                          pAreaData[areaCount],
        void *                          pConsumerContext)
{
    uint32_t *pMsgCount = *(uint32_t **)pConsumerContext;
    __sync_add_and_fetch(pMsgCount, 1);

    ismEngine_TransactionHandle_t hTran=NULL;
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = ietr_createLocal(pThreadData, NULL, true, false, NULL, &hTran);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_Session_t *pSession = ((ismEngine_Consumer_t *)hConsumer)->pSession;

#if !defined(NO_MALLOC_WRAPPER)
    //disable all malloc...
    iemem_setMallocState(iememDisableAll);
#endif

    rc = ism_engine_confirmMessageDelivery(pSession,
                                      hTran,
                                      hDelivery,
                                      ismENGINE_CONFIRM_OPTION_CONSUMED,
                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_commitTransaction(NULL, hTran, ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
    TEST_ASSERT_EQUAL(rc, OK);

#if !defined(NO_MALLOC_WRAPPER)
    //reenable mallocs (apart from ones explicit prevented by this test)
    iemem_setMallocState(iememPlentifulMemory);
#endif
    ism_engine_releaseMessage(hMessage);
    return true;
}
void test_capability_DeliveryErrors(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    ismEngine_Consumer_t  *hConsumer[2] = {0};
    uint32_t messageCount[2] = {0};
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    char *qName = "deliveryErrorQ";
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};

    printf("Starting %s...\n", __func__);


    ism_engine_registerDeliveryFailureCallback(deliveryFailureHandler);

    rc = test_createClientAndSession("DeliveryErrorsClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Set default maxMessageCount to 0 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;

    // Create  a queue with two consumers
    rc = ieqn_createQueue(pThreadData, qName, multiConsumer, ismQueueScope_Server, NULL, NULL, NULL, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t *pMsgCount = &messageCount[0];
    rc = ism_engine_createConsumer(hSession, ismDESTINATION_QUEUE, qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pMsgCount, sizeof(int32_t *),
                                   deliveryErrorsMsgArrivedCB,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer[0],
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer[0]);

    pMsgCount = &messageCount[1];
    rc = ism_engine_createConsumer(hSession, ismDESTINATION_QUEUE, qName,
                                   ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                   NULL, // Unused for QUEUE
                                   &pMsgCount, sizeof(int32_t *),
                                   deliveryErrorsMsgArrivedCB,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_ACK,
                                   &hConsumer[1],
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer[1]);


    failProbes[0]  = IEMEM_PROBE(iemem_lockManager, 7);
    probeCounts[0] = 0; //Allow zero before failures
    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    char *payload="PAYLOAD";
    ismEngine_MessageHandle_t hMessage;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
    size_t areaLengths[2] = {0, strlen(payload)+1};
    void *areaData[2] = {NULL, payload};

    header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
    header.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;

    rc = ism_engine_createMessage(&header,
                                  2,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_QUEUE,
                                            qName,
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Check that the delivery failure callback happened and that a message was delivered
    TEST_ASSERT_EQUAL(numDeliveryHandlerCalls, 1);
    TEST_ASSERT_EQUAL(messageCount[0] + messageCount[1], 1);

    //and that the other consumer was disabled
    if (messageCount[0] == 0)
    {
        //consumer 0 was the consumer who delivery failed for...
        ismMessageDeliveryStatus_t status;
        ism_engine_getConsumerMessageDeliveryStatus( hConsumer[0], &status);
        TEST_ASSERT_EQUAL(status, ismMESSAGE_DELIVERY_STATUS_STOPPED);
    }
    else
    {
        //The message was given to consumer 0...
        //...consumer 1 should have received no message
        TEST_ASSERT_EQUAL(messageCount[1], 0);
        //...and should be disabled (or in the process of disabling)
        ismMessageDeliveryStatus_t status;
        uint32_t waits = 0;
        
        do
        {
            ism_engine_getConsumerMessageDeliveryStatus( hConsumer[1], &status);
            TEST_ASSERT_CUNIT((  (status == ismMESSAGE_DELIVERY_STATUS_STOPPED)
                               ||(status == ismMESSAGE_DELIVERY_STATUS_STOPPING)),
                              ("status was %d", status));
            
            if (status == ismMESSAGE_DELIVERY_STATUS_STOPPING)
            {
                //wait 1ms to see if we stop
                usleep(1000);
                waits++;
                
                if (waits > 20 *1000)
                {
                    //We've waited for 20 seconds... it's never gone to stop
                    printf("Consumer should have been disabled due to delivery failure.\n");
                    abort();
                }
            }
        }
        while(status != ismMESSAGE_DELIVERY_STATUS_STOPPED);
    }

    // Ensure we're back to normal...even if this test went awry...
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ieqn_destroyQueue(pThreadData, qName, ieqnDiscardMessages, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_DeliveryErrors[] =
{
    { "DeliveryErrors", test_capability_DeliveryErrors },
    CU_TEST_INFO_NULL
};


//****************************************************************************
/// @brief Test the capabilities of client / session creation when allocs fail.
//****************************************************************************
void test_operationComplete( int32_t retcode
                           , void *handle
                           , void *pcontext )
{
}

static void test_stealCallback( int32_t reason,
                                ismEngine_ClientStateHandle_t hClient,
                                uint32_t options,
                                void *pContext)
{
    ismEngine_ClientStateHandle_t *pHandle = (ismEngine_ClientStateHandle_t *)pContext;
    int32_t rc = OK;

    TEST_ASSERT_EQUAL(reason, ISMRC_ResumedClientState);
    TEST_ASSERT_EQUAL(options, ismENGINE_STEAL_CALLBACK_OPTION_NONE);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // We have a tame victim - let's check that it's us, just to drive the code that follows thieves!
    iecsMessageDeliveryInfoHandle_t hVictimMsgDelInfo = NULL;

    rc = iecs_findClientMsgDelInfo(pThreadData,
                                   ((ismEngine_ClientState_t *)hClient)->pClientId,
                                   &hVictimMsgDelInfo);
    TEST_ASSERT_EQUAL(hVictimMsgDelInfo, ((ismEngine_ClientState_t *)hClient)->hMsgDeliveryInfo);

    if (rc == OK)
    {
        TEST_ASSERT_PTR_NOT_NULL(hVictimMsgDelInfo);
        iecs_releaseMessageDeliveryInfoReference(pThreadData, hVictimMsgDelInfo);
    }
    else
    {
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    }

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE, NULL, 0, NULL);
    if (rc != OK) TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    *pHandle = NULL;
}

void test_capability_ClientAndSessionErrors(void)
{
    int32_t rc;
    int32_t expectedRC = OK;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    int32_t loop;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->userid = "FailedAllocUser";

    //Initialise OpenSSL - in the product - the equivalent is ism_ssl_init();
    //TODO: Do we need to run that in the engine thread that does this or is it done?
    sslInit();

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    // Need to stop the clientStateExpiry reaper while we do this test.
    iece_stopClientStateExpiry(pThreadData);

    // Start out destroying the engine client state... the first test is of creating it
    iecs_destroyClientStateTable(pThreadData);

    uint32_t failSequence1[][3] = {// Fail in iecs_createClientStateTable allocating the hash table
                                   {IEMEM_PROBE(iemem_clientState, 1), 0, ISMRC_AllocateError},
                                   // Fail in iecs_createClientStateTable allocating the chains
                                   {IEMEM_PROBE(iemem_clientState, 2), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence1)/sizeof(failSequence1[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        rc = iecs_createClientStateTable(pThreadData);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // Resize the client state table
    uint32_t failSequence2[][3] = {// Fail in iecs_resizeClientStateTable allocating the new hash table
                                   {IEMEM_PROBE(iemem_clientState, 3), 0, ISMRC_AllocateError},
                                   // Fail in iecs_resizeClientStateTable allocating the chains
                                   {IEMEM_PROBE(iemem_clientState, 4), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    iecsHashTableHandle_t newTable = NULL;
    for(loop=0; loop<(sizeof(failSequence2)/sizeof(failSequence2[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence2[loop][0];
            probeCounts[0] = failSequence2[loop][1];
            expectedRC = failSequence2[loop][2];
        }

        rc = iecs_resizeClientStateTable(pThreadData, ismEngine_serverGlobal.ClientTable, &newTable);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    TEST_ASSERT_PTR_NOT_NULL(newTable);
    iecs_freeClientStateTable(pThreadData, newTable, true);

    // *** Create Client State ***
    uint32_t failSequence3[][3] = {// Fail in iecs_newClientState allocating a client state
                                   {IEMEM_PROBE(iemem_clientState, 6), 0, ISMRC_AllocateError},
                                   // Fail in iecs_newClientState allocating the userid
                                   {IEMEM_PROBE(iemem_clientState, 7), 0, ISMRC_AllocateError},
                                   // Fail in iecs_addClientState adding client state to table
                                   {IEMEM_PROBE(iemem_clientState, 9), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence3)/sizeof(failSequence3[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence3[loop][0];
            probeCounts[0] = failSequence3[loop][1];
            expectedRC = failSequence3[loop][2];
        }

        rc = sync_ism_engine_createClientState("FailedAllocsClient",
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_DURABLE | ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                               &hClient, test_stealCallback,
                                               mockContext,
                                               &hClient);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // *** Create Session ***
    uint32_t failSequence4[][3] = {// Fail in ism_engine_createSession allocating a ismEngine_Session_t
                                   {IEMEM_PROBE(iemem_externalObjs, 2), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence4)/sizeof(failSequence4[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence4[loop][0];
            probeCounts[0] = failSequence4[loop][1];
            expectedRC = failSequence4[loop][2];
        }

        rc = ism_engine_createSession(hClient,
                                      ismENGINE_CREATE_SESSION_OPTION_NONE,
                                      &hSession,
                                      NULL, 0, NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // *** Set Will Message ***
    uint32_t failSequence5[][3] = {// Fail in iecs_setWillMessage allocating will topic name
                                   {IEMEM_PROBE(iemem_clientState, 15), 0, ISMRC_AllocateError},
                                  };

    char *payload="PAYLOAD";
    ismEngine_MessageHandle_t hMessage;

    for(loop=0; loop<(sizeof(failSequence5)/sizeof(failSequence5[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence5[loop][0];
            probeCounts[0] = failSequence5[loop][1];
            expectedRC = failSequence5[loop][2];
        }

        void *payloadPtr = payload;
        rc = test_createMessage(strlen(payload)+1,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, "WILLMSG/ERRORS/TOPIC",
                                &hMessage, &payloadPtr);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_setWillMessage(hClient,
                                       ismDESTINATION_TOPIC,
                                       "WILLMSG/ERRORS/TOPIC",
                                       hMessage,
                                       0,
                                       20,
                                       NULL, 0, NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }


    // *** Destroy Session / Start Message Delivery ***
    uint32_t failSequence6[][3] = {// Fail in ism_engine_destroySession allocating pending destroy context
                                   {IEMEM_PROBE(iemem_callbackContext, 3), 0, ISMRC_AllocateError},
                                   // Fail in ism_engine_startMessageDelivery because we make pSession->fIsDestroyed true
                                   {0, 0, ISMRC_Destroyed},
                                   // Fail in ism_engine_destroySession because we make pSession->fIsDestroyed true
                                   {0, 0, ISMRC_Destroyed},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence6)/sizeof(failSequence6[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence6[loop][0];
            probeCounts[0] = failSequence6[loop][1];
            expectedRC = failSequence6[loop][2];
        }

        uint32_t context = 111;
        uint32_t *pContext = &context;

        switch(loop)
        {
            case 1:
                hSession->fIsDestroyed = true;
                rc = ism_engine_startMessageDelivery(hSession,
                                                     ismENGINE_START_DELIVERY_OPTION_NONE,
                                                     &pContext,
                                                     sizeof(uint32_t *),
                                                     test_operationComplete);
                hSession->fIsDestroyed = false;
                break;
            case 2:
                hSession->fIsDestroyed = true;
                rc = ism_engine_destroySession(hSession,
                                               &pContext,
                                               sizeof(uint32_t *),
                                               test_operationComplete);
                hSession->fIsDestroyed = false;
                break;
            default:
                rc = ism_engine_destroySession(hSession,
                                               &pContext,
                                               sizeof(uint32_t *),
                                               test_operationComplete);
                break;
        }

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
    rc = sync_ism_engine_createSubscription(hClient,
                                            "FailedAllocsSubscription",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            "/test/failedAllocs/Topic",
                                            &subAttrs,
                                            NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // *** Destroy ClientState ***
    uint32_t failSequence7[][3] = {// Fail in ism_engine_destroyClientState allocating pending destroy context
                                   {IEMEM_PROBE(iemem_callbackContext, 2), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence7)/sizeof(failSequence7[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence7[loop][0];
            probeCounts[0] = failSequence7[loop][1];
            expectedRC = failSequence7[loop][2];
        }

        rc = sync_ism_engine_destroyClientState(hClient,
                                                ismENGINE_DESTROY_CLIENT_OPTION_NONE);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // *** Zombie takeover / Steal ***
    uint32_t failSequence8[][3] = {// Fail in iecs_addClientState allocating callback context during Zombie takeover
                                   {IEMEM_PROBE(iemem_callbackContext, 4), 0, ISMRC_AllocateError},
                                   // Fail allocating when creating a list to reauthorize subscriptions
                                   {IEMEM_PROBE(iemem_callbackContext, 8), 0, ISMRC_OK /* YES - IT WORKS */},
                                   // Fail in iecs_addClientState allocating callback context during steal
                                   {IEMEM_PROBE(iemem_callbackContext, 1), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, ISMRC_OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence8)/sizeof(failSequence8[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence8[loop][0];
            probeCounts[0] = failSequence8[loop][1];
            expectedRC = failSequence8[loop][2];
        }

        if (loop == 1)
        {
            swallow_ffdcs = true;
            swallow_expect_core = false;
        }

        rc = sync_ism_engine_createClientState("FailedAllocsClient",
                                               testDEFAULT_PROTOCOL_ID,
                                               ismENGINE_CREATE_CLIENT_OPTION_NONE | ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                               &hClient, test_stealCallback,
                                               mockContext,
                                               &hClient);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);

        swallow_ffdcs = false;
    }

    // *** Explicit test of iecs_newClientStateRecovery ***
    uint32_t failSequence9[][3] = {// Fail in iecs_newClientStateRecovery allocating clientState structure
                                   {IEMEM_PROBE(iemem_clientState, 8), 0, ISMRC_AllocateError},
                                   // Fail in iecs_newClientStateRecovery initializing the client mutex
                                   {TEST_IEMEM_PROBE_MUTEX_INIT, 0, ISMRC_AllocateError},
                                   // Fail in iecs_newClientStateRecovery initializing the useCountLock
                                   {TEST_IEMEM_PROBE_SPIN_INIT, 0, ISMRC_AllocateError},
                                   // Fail in iecs_newClientStateRecovery initializing the unreleased mutex
                                   {TEST_IEMEM_PROBE_MUTEX_INIT, 1, ISMRC_AllocateError},
                                  };

    iecsNewClientStateInfo_t clientInfo;
    clientInfo.pClientId = "FailedAllocClient";
    clientInfo.protocolId = PROTOCOL_ID_MQTT;
    clientInfo.durability = iecsDurable;
    clientInfo.resourceSet = iecs_getResourceSet(pThreadData, clientInfo.pClientId, clientInfo.protocolId, iereOP_ADD);

    for(loop=0; loop<(sizeof(failSequence9)/sizeof(failSequence9[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence9[loop][0];
            probeCounts[0] = failSequence9[loop][1];
            expectedRC = failSequence9[loop][2];
        }

        rc = iecs_newClientStateRecovery(pThreadData,
                                         &clientInfo,
                                         &hClient);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // *** Explicit test of iecs_newClientState ***
    uint32_t failSequence10[][3] = {// Fail in iecs_newClientState allocating clientState structure
                                    {IEMEM_PROBE(iemem_clientState, 6), 0, ISMRC_AllocateError},
                                    // Fail in iecs_newClientState initializing the client mutex
                                    {TEST_IEMEM_PROBE_MUTEX_INIT, 0, ISMRC_AllocateError},
                                    // Fail in iecs_newClientState initializing the useCountLock
                                    {TEST_IEMEM_PROBE_SPIN_INIT, 0, ISMRC_AllocateError},
                                    // Fail in iecs_newClientState initializing the unreleased mutex
                                    {TEST_IEMEM_PROBE_MUTEX_INIT, 1, ISMRC_AllocateError},
                                  };

    clientInfo.pClientId = "FailedAllocClient";
    clientInfo.protocolId = PROTOCOL_ID_MQTT;
    clientInfo.pSecContext = NULL;
    clientInfo.durability = iecsDurable;
    clientInfo.takeover = iecsNoTakeover;
    clientInfo.fCleanStart = false;
    clientInfo.pStealContext = NULL;
    clientInfo.pStealCallbackFn = NULL;
    clientInfo.resourceSet = iecs_getResourceSet(pThreadData, clientInfo.pClientId, clientInfo.protocolId, iereOP_ADD);

    for(loop=0; loop<(sizeof(failSequence10)/sizeof(failSequence10[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence10[loop][0];
            probeCounts[0] = failSequence10[loop][1];
            expectedRC = failSequence10[loop][2];
        }

        rc = iecs_newClientState(pThreadData,
                                 &clientInfo,
                                 &hClient);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // *** Explicit test of iecs_newClientStateImport ***
    uint32_t failSequence11[][3] = {// Fail in iecs_newClientStateImport allocating clientState structure
                                   {IEMEM_PROBE(iemem_clientState, 21), 0, ISMRC_AllocateError},
                                   // Fail in iecs_newClientStateImport allocating UserId
                                   {IEMEM_PROBE(iemem_clientState, 22), 0, ISMRC_AllocateError},
                                   // Fail in iecs_newClientState initializing the client mutex
                                   {TEST_IEMEM_PROBE_MUTEX_INIT, 0, ISMRC_AllocateError},
                                   // Fail in iecs_newClientState initializing the useCountLock
                                   {TEST_IEMEM_PROBE_SPIN_INIT, 0, ISMRC_AllocateError},
                                   // Fail in iecs_newClientState initializing the unreleased mutex
                                   {TEST_IEMEM_PROBE_MUTEX_INIT, 1, ISMRC_AllocateError},
                                  };

    clientInfo.pClientId = "FailedAllocClient";
    clientInfo.protocolId = PROTOCOL_ID_MQTT;
    clientInfo.durability = iecsDurable;
    clientInfo.pUserId = "FailedAllocUserId";
    clientInfo.lastConnectedTime = 123;
    clientInfo.expiryInterval = iecsEXPIRY_INTERVAL_INFINITE;
    clientInfo.resourceSet = iecs_getResourceSet(pThreadData, clientInfo.pClientId, clientInfo.protocolId, iereOP_ADD);

    for(loop=0; loop<(sizeof(failSequence11)/sizeof(failSequence11[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence11[loop][0];
            probeCounts[0] = failSequence11[loop][1];
            expectedRC = failSequence11[loop][2];
        }

        rc = iecs_newClientStateImport(pThreadData,
                                       &clientInfo,
                                       &hClient);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // *** Explicit test of iecs_forceDiscardClientState ***
    uint32_t failSequence12[][3] = {// Fail in iecs_forceDiscardClientState allocating context
                                   {IEMEM_PROBE(iemem_callbackContext, 17), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<(sizeof(failSequence12)/sizeof(failSequence12[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence12[loop][0];
            probeCounts[0] = failSequence12[loop][1];
            expectedRC = failSequence12[loop][2];
        }

        rc = iecs_forceDiscardClientState(pThreadData,
                                          "FailedAllocClient",
                                          iecsFORCE_DISCARD_CLIENT_OPTION_NON_ACKING_CLIENT);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    // Tidy up
    rc = sync_ism_engine_destroyClientState(hClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_NONE);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    sslTerm();

    // Restart the expiry reaper.
    iece_startClientStateExpiry(pThreadData);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_ClientAndSessionErrors[] =
{
    { "ClientAndSessionErrors", test_capability_ClientAndSessionErrors },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the capabilities of queue namespace when allocs fail.
//****************************************************************************
void test_capability_QueueNamespaceErrors(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int32_t expectedRC = OK;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    // Remove queues created by the infrastructure
    ieqn_destroyEngineQueueNamespace(pThreadData);

    uint32_t failSequence1[][3] = {// Fail in ieqn_initEngineQueueNamespace allocating header
                                   {IEMEM_PROBE(iemem_queueNamespace, 1), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable allocating the queue namespace hash table
                                   {IEMEM_PROBE(iemem_queueNamespace, 60000), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable allocating the queue namespace chains
                                   {IEMEM_PROBE(iemem_queueNamespace, 60001), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        rc = ieqn_initEngineQueueNamespace(pThreadData);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // *** FAIL CREATING A NAMED QUEUE ***
    uint32_t failSequence2[][3] = {// Fail in ieqn_createQueue allocating property name format string
                                   {IEMEM_PROBE(iemem_queueNamespace, 5), 0, ISMRC_AllocateError},
                                   // Fail in iemq_createQ allocating the queue structure
                                   {IEMEM_PROBE(iemem_multiConsumerQ, 1), 0, ISMRC_AllocateError},
                                   // Fail in ieqn_addQueue allocating the named queue
                                   {IEMEM_PROBE(iemem_queueNamespace, 4), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry resizing the chain
                                   {IEMEM_PROBE(iemem_queueNamespace, 60004), 0, ISMRC_AllocateError},
                                   // Fail in iepi_createPolicyInfoFromProperties allocating the policy info
                                   {IEMEM_PROBE(iemem_policyInfo, 2), 0, ISMRC_AllocateError},
                                   // Fail in iepi_updatePolicyInfoFromProperties allocating local variable for property name
                                   {IEMEM_PROBE(iemem_policyInfo, 1), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    ism_prop_t *pProperties = ism_common_newProperties(1);

    for(loop=0; loop<sizeof(failSequence2)/sizeof(failSequence2[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence2[loop][0];
            probeCounts[0] = failSequence2[loop][1];
            expectedRC = failSequence2[loop][2];
        }

        rc = ieqn_createQueue(pThreadData,
                              "FAIL.NAMED.QUEUE",
                              multiConsumer,
                              ismQueueScope_Server, NULL,
                              pProperties,
                              NULL,
                              NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    ism_common_freeProperties(pProperties);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_QueueNamespaceErrors[] =
{
    { "QueueNamespaceErrors", test_capability_QueueNamespaceErrors },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the capabilities of policy infos when allocs fail.
//****************************************************************************
void test_capability_PolicyInfoErrors(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int32_t expectedRC = OK;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    iepiPolicyInfo_t *pPolicyInfo = NULL;

    uint32_t failSequence1[][3] = {// Fail in iepi_createPolicyInfo allocating policy info
                                   {IEMEM_PROBE(iemem_policyInfo, 3), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        iepiPolicyInfo_t templatePolicyInfo;

        templatePolicyInfo.maxMessageCount = 1234;
        templatePolicyInfo.maxMsgBehavior = DiscardOldMessages;
        templatePolicyInfo.DCNEnabled = false;

        if (failSequence1[loop][2] != OK)
        {
            rc = iepi_createPolicyInfo(pThreadData, "testName", ismSEC_POLICY_TOPIC, true, &templatePolicyInfo, &pPolicyInfo);
        }
        else
        {
            rc = iepi_createPolicyInfo(pThreadData, NULL, ismSEC_POLICY_LAST, false, &templatePolicyInfo, &pPolicyInfo);
        }

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    iepiPolicyInfo_t *pCopiedPolicyInfo = NULL;
    iepiPolicyInfo_t *pCopiedPolicyInfo2 = NULL;

    // Test copy policy failures
    uint32_t failSequence2[][3] = {// Fail in iepi_copyPolicyInfo allocating policy info
                                   {IEMEM_PROBE(iemem_policyInfo, 8), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<sizeof(failSequence2)/sizeof(failSequence2[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence2[loop][0];
            probeCounts[0] = failSequence2[loop][1];
            expectedRC = failSequence2[loop][2];
        }

        if (loop == 2)
        {
            rc = iepi_copyPolicyInfo(pThreadData, pPolicyInfo, NULL, &pCopiedPolicyInfo2);
        }
        else
        {
            rc = iepi_copyPolicyInfo(pThreadData, pPolicyInfo, "NEWNAME", &pCopiedPolicyInfo);
        }

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    ism_field_t f;
    ism_prop_t *pProperties = ism_common_newProperties(10);
    TEST_ASSERT_PTR_NOT_NULL(pProperties);

    f.type = VT_String;
    f.val.s = "UPDATEDNAME";
    rc = ism_common_setProperty(pProperties,
                                ismENGINE_ADMIN_PROPERTY_NAME,
                                &f);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_common_freeProperties(pProperties);

    iepi_releasePolicyInfo(pThreadData, pCopiedPolicyInfo);
    iepi_releasePolicyInfo(pThreadData, pCopiedPolicyInfo2);
    iepi_releasePolicyInfo(pThreadData, pPolicyInfo);

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_PolicyInfoErrors[] =
{
    { "PolicyInfoErrors", test_capability_PolicyInfoErrors },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test that being unable to allocate the per-quueue expiry data
/// doesn't cause us to access a deleted queue from the expiry reaper
//****************************************************************************
void test_capability_ExpiryQDataError(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    ismEngine_Consumer_t  *hConsumer = NULL;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_SubscriptionAttributes_t subAttrs = {0};
    char *topicString = "/test/expiryfailedalloc";
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    uint64_t scansNeeded = 0;

    rc = test_createClientAndSession("FailedAllocsExpiryClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("Starting %s...\n", __func__);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession, ismDESTINATION_TOPIC, topicString,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   NULL, 0, NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, ISMRC_OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    failProbes[0] = IEMEM_PROBE(iemem_messageExpiryData, 2);
    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    //Publish a message with expiry so that the queue should be added to the
    //list but we can't create the per-queue data as the calloc fails:
    ismEngine_MessageHandle_t hMessage;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            ism_common_nowExpire()+10000,
                            ismDESTINATION_TOPIC,
                            topicString,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                    topicString,
                                                    hMessage,
                                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Ensure an expiry scan occurs which will try and fail the alloc again
    iemeExpiryControl_t *expiryControl = (iemeExpiryControl_t *)ismEngine_serverGlobal.msgExpiryControl;
    scansNeeded = expiryControl->scansStarted + 1;
    ieme_wakeMessageExpiryReaper(pThreadData);
    while(scansNeeded > expiryControl->scansEnded) { usleep(5000); }

    //Destroy the consumer...
    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    //Wake up the reaper when the queue should no longer be in the list (ensure we don't segfault)
    scansNeeded = expiryControl->scansStarted + 1;
    ieme_wakeMessageExpiryReaper(pThreadData);
    while(scansNeeded > expiryControl->scansEnded) { usleep(5000); }

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

//****************************************************************************
/// @brief Test the allocations etc related to creating the clientState expiry
/// infrastructure
//****************************************************************************
void test_capability_ClientExpiryInfrastructure(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int32_t expectedRC = OK;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;

    // Destroy the current infrastructure
    iece_stopClientStateExpiry(pThreadData);
    iece_destroyClientStateExpiry(pThreadData);

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    // FAILURES in the initialization routines
    uint32_t failSequence1[][3] = {// Fail in iece_initClientStateExpiry allocating control structure
                                   {IEMEM_PROBE(iemem_messageExpiryData, 3), 0, ISMRC_AllocateError},
                                   // Fail to initialize the condition attrs
                                   {TEST_IEMEM_PROBE_CONDATTR_INIT, 0, OK},
                                   // Fail to set the clock in the condition attrs
                                   {TEST_IEMEM_PROBE_CONDATTR_SETCLOCK, 0, OK},
                                   // Fail to initialize the condition attrs
                                   // {TEST_IEMEM_PROBE_COND_INIT, 0, OK},
                                   // Fail to destroy the condition attrs
                                   {TEST_IEMEM_PROBE_CONDATTR_DESTROY, 0, OK},
                                   // Fail to initialize the condition variable mutex
                                   {TEST_IEMEM_PROBE_MUTEX_INIT, 0, OK},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        swallowed_ffdc_count = 0;

        if (failProbes[0] == TEST_IEMEM_PROBE_MUTEX_INIT ||
            failProbes[0] == TEST_IEMEM_PROBE_CONDATTR_INIT ||
            failProbes[0] == TEST_IEMEM_PROBE_CONDATTR_SETCLOCK ||
            failProbes[0] == TEST_IEMEM_PROBE_CONDATTR_DESTROY)
        {
            swallow_ffdcs = true;
            swallow_expect_core = true;
        }
        else
        {
            swallow_ffdcs = false;
            swallow_expect_core = false;
        }

        rc = iece_initClientStateExpiry(pThreadData);

        TEST_ASSERT_EQUAL(rc, expectedRC);

        if (swallow_ffdcs)
        {
            TEST_ASSERT_EQUAL(swallowed_ffdc_count, 1);
            iece_stopClientStateExpiry(pThreadData);
            iece_destroyClientStateExpiry(pThreadData);
        }
    }

    TEST_ASSERT_EQUAL(swallow_ffdcs, false);

    ieceExpiryControl_t *expiryControl = ismEngine_serverGlobal.clientStateExpiryControl;

    // *FAIL* to restart clientState expiry by artificially making ism_common_startThread fail
    expiryControl->reaperEndRequested = false;
    override_startThreadFunc = "clientReaper";
    override_startThreadRc = 5;
    rc = iece_startClientStateExpiry(pThreadData);
    TEST_ASSERT_EQUAL(rc, ISMRC_Error);
    override_startThreadFunc = NULL;
    override_startThreadRc = 0;

    rc = iece_startClientStateExpiry(pThreadData);
    TEST_ASSERT_EQUAL(rc, OK);

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_ExpiryErrors[] =
{
    { "ExpiryQDataError", test_capability_ExpiryQDataError},
    { "ClientExpiryInfrastructure", test_capability_ClientExpiryInfrastructure },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the capabilities of remote servers when allocs fail.
//****************************************************************************
void test_capability_RemoteServersErrors(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int32_t expectedRC = OK;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("FailedAllocsClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Pretend that the cluster is enabled
    ismEngine_serverGlobal.clusterEnabled = true;

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    // ENGINE_RS_CREATE tests
    uint32_t failSequence1[][3] = {// Fail in iers_createRemoteServer allocating structure
                                   {IEMEM_PROBE(iemem_remoteServers, 2), 0, ISMRC_AllocateError},
                                   // Fail in iers_createRemoteServer allocating structure
                                   {IEMEM_PROBE(iemem_remoteServers, 2), 0, ISMRC_AllocateError},
                                   // Fail in iers_createRemoteServer allocating structure
                                   {IEMEM_PROBE(iemem_remoteServers, 12), 0, ISMRC_AllocateError},
                                   // Fail in iemq_createQ when allocating the queue name for low QoS
                                   {IEMEM_PROBE(iemem_multiConsumerQ, 2), 0, ISMRC_AllocateError},
                                   // Fail in iemq_createQ when allocating the queue name for high QoS
                                   {IEMEM_PROBE(iemem_multiConsumerQ, 2), 1, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    ismEngine_RemoteServerHandle_t remoteServerHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        // Try creating a remote server (on one loop, we create a remote server for the local server)
        // to ensure that an allocation failure there does work correctly
        rc = iers_clusterEventCallback(loop == 1 ? ENGINE_RS_CREATE_LOCAL : ENGINE_RS_CREATE,
                                       ismENGINE_NULL_REMOTESERVER_HANDLE,
                                       (ismCluster_RemoteServerHandle_t)1,
                                       "FailedAllocs",
                                       "FAUID",
                                       NULL, 0,
                                       NULL, 0,
                                       false, false, NULL, NULL,
                                       NULL,
                                       &remoteServerHandle);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    TEST_ASSERT_PTR_NOT_NULL(remoteServerHandle);

    // Publish tests
    uint32_t failSequence2[][3] = {// Fail to allocate response buffer for retained
                                   {IEMEM_PROBE(iemem_subsQuery, 10), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, ISMRC_NoMatchingLocalDestinations},
                                  };
    char *payload="PAYLOAD";
    ismEngine_MessageHandle_t hMessage;

    for(loop=0; loop<sizeof(failSequence2)/sizeof(failSequence2[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence2[loop][0];
            probeCounts[0] = failSequence2[loop][1];
            expectedRC = failSequence2[loop][2];
        }

        void *payloadPtr = payload;
        rc = test_createMessage(strlen(payload)+1,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                0,
                                ismDESTINATION_TOPIC, "REMOTESERVER/ERRORS/TOPIC",
                                &hMessage, &payloadPtr);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                "REMOTESERVER/ERRORS/TOPIC",
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // ENGINE_RS_UPDATE tests
    uint32_t failSequence3[][3] = {// Fail in iers_updateRemoteServer allocating a pending update structure
                                   {IEMEM_PROBE(iemem_remoteServers, 3), 0, ISMRC_AllocateError},
                                   // Fail in iers_updateRemoteServer re-allocating the remote server list
                                   {IEMEM_PROBE(iemem_remoteServers, 4), 0, ISMRC_AllocateError},
                                   // Fail in iers_updateRemoteServer allocating a copy of the cluster data
                                   {IEMEM_PROBE(iemem_remoteServers, 5), 0, ISMRC_AllocateError},
                                   // Fail in iers_updateRemoteServer allocating new server name
                                   {IEMEM_PROBE(iemem_remoteServers, 13), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    ismEngine_RemoteServer_PendingUpdateHandle_t pendingUpdate = 0;

    for(int32_t outerLoop=0; outerLoop<2; outerLoop++)
    {
        if (outerLoop == 0)
        {
            loop=0;
        }
        else
        {
            loop=2;
        }

        for(; loop<sizeof(failSequence3)/sizeof(failSequence3[0]); loop++)
        {
            char *remoteServerName = "NORMAL_NAME";
            char clusterData[20];

            if (analysisType != TEST_ANALYSE_IEMEM_NONE)
            {
                printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
                failProbes[0] = analysisType;
                probeCounts[0] = 0;
            }
            else
            {
                failProbes[0] = failSequence3[loop][0];
                probeCounts[0] = failSequence3[loop][1];
                expectedRC = failSequence3[loop][2];
            }

            if (loop == 3) remoteServerName = "CHANGED_NAME";

            sprintf(clusterData, "DATA%02u%02u", outerLoop, loop);

            // Try creating a remote server (on one loop, we create a remote server for the local server)
            // to ensure that an allocation failure there does work correctly
            rc = iers_clusterEventCallback(ENGINE_RS_UPDATE,
                                           remoteServerHandle,
                                           0, remoteServerName, NULL,
                                           clusterData, strlen(clusterData)+1,
                                           NULL, 0,
                                           false, false, &pendingUpdate, NULL,
                                           NULL,
                                           NULL);

            if (analysisType != TEST_ANALYSE_IEMEM_NONE)
            {
                printf("Analysis complete\n");
                break;
            }

            TEST_ASSERT_EQUAL(rc, expectedRC);

            if (outerLoop == 0 && loop < sizeof(failSequence3)/sizeof(failSequence3[0])-1)
            {
                TEST_ASSERT_PTR_NULL(pendingUpdate);
            }
            else
            {
                TEST_ASSERT_PTR_NOT_NULL(pendingUpdate);
            }
        }
    }

    TEST_ASSERT_EQUAL(strcmp(remoteServerHandle->clusterData, "DATA0104"), 0); // Prove updated data
    TEST_ASSERT_EQUAL(remoteServerHandle->useCount, 2);

    // Commit the updates
    rc = iers_clusterEventCallback(ENGINE_RS_UPDATE,
                                   NULL, 0, NULL, NULL, NULL, 0, NULL, 0, false, true, &pendingUpdate, NULL,
                                   NULL, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(remoteServerHandle->useCount, 1);

    // Terminate the cluster (it's going to fail)
    uint32_t failSequence4[][3] = {// Fail in iers_terminateCluster allocating a list of servers
                                   {IEMEM_PROBE(iemem_remoteServers, 16), 0, ISMRC_AllocateError},
                                  };

    for(loop=0; loop<sizeof(failSequence4)/sizeof(failSequence4[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence4[loop][0];
            probeCounts[0] = failSequence4[loop][1];
            expectedRC = failSequence4[loop][2];
        }

        rc = iers_clusterEventCallback(ENGINE_RS_TERM,
                                       NULL,
                                       0, NULL, NULL,
                                       NULL, 0,
                                       NULL, 0,
                                       false, false, NULL, NULL,
                                       NULL,
                                       NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // Should have set clusterEnabled to false
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.clusterEnabled, false);
    ismEngine_serverGlobal.clusterEnabled = true;

    // ENGINE_RS_ADD_SUB / ENGINE_RS_DEL_SUB tests
    uint32_t failSequence5[][3] = {// Fail in iett_insertOrFindRemSrvNode allocating a node
                                   {IEMEM_PROBE(iemem_remoteServers, 9), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable creating hash table
                                   {IEMEM_PROBE(iemem_remoteServers, 60000), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry allocating chain
                                   {IEMEM_PROBE(iemem_remoteServers, 60004), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    char *topicString = "/TEST/ADD/SUB/#";

    for(loop=0; loop<sizeof(failSequence4)/sizeof(failSequence4[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence5[loop][0];
            probeCounts[0] = failSequence5[loop][1];
            expectedRC = failSequence5[loop][2];
        }

        // Add the remote server to a topic
        rc = iers_clusterEventCallback(ENGINE_RS_ADD_SUB,
                                       remoteServerHandle,
                                       0, NULL, NULL,
                                       NULL, 0,
                                       &topicString, 1,
                                       false, false, NULL, NULL,
                                       NULL,
                                       NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    rc = iers_clusterEventCallback(ENGINE_RS_DEL_SUB,
                                   remoteServerHandle,
                                   0, NULL, NULL,
                                   NULL, 0,
                                   &topicString, 1,
                                   false, false, NULL, NULL,
                                   NULL,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Test ism_engine_createRemoteServerConsumer
    uint32_t failSequence6[][3] = {// Fail in ism_engine_createRemoteServerConsumer allocating consumer
                                   {IEMEM_PROBE(iemem_externalObjs, 9), 0, ISMRC_AllocateError},
                                   // FAIL when initializing consumer
                                   {0, 0, ISMRC_QueueDeleted},
                                  };

    rc = test_createClientAndSessionWithProtocol("TESTCLIENT",
                                                 PROTOCOL_ID_FWD,
                                                 NULL,
                                                 ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL,
                                                 ismENGINE_CREATE_SESSION_TRANSACTIONAL |
                                                 ismENGINE_CREATE_SESSION_EXPLICIT_SUSPEND,
                                                 &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hClient);
    TEST_ASSERT_PTR_NOT_NULL(hSession);

    for(loop=0; loop<sizeof(failSequence6)/sizeof(failSequence6[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence6[loop][0];
            probeCounts[0] = failSequence6[loop][1];
            expectedRC = failSequence6[loop][2];
        }

        // Add the remote server to a topic
        ismEngine_Consumer_t *hConsumer;
        rc = ism_engine_createRemoteServerConsumer(hSession,
                                                   remoteServerHandle,
                                                   NULL, 0, NULL,
                                                   ismENGINE_CONSUMER_OPTION_LOW_QOS,
                                                   &hConsumer,
                                                   NULL, 0, NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        // YUCK (YUCK, YUCK) but forces another failure case
        if (loop==0)
        {
            ieq_markQDeleted(pThreadData, remoteServerHandle->lowQoSQueueHandle, false);
        }
        else if (loop == 1)
        {
            iemqQueue_t *q = (iemqQueue_t *)remoteServerHandle->lowQoSQueueHandle;
            q->isDeleted = false;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
        TEST_ASSERT_EQUAL(remoteServerHandle->useCount, 1);
        TEST_ASSERT_EQUAL(remoteServerHandle->consumerCount, 0);
    }

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Test basic creation failure
    uint32_t failSequence7[][3] = {// Fail in iers_initEngineRemoteServers allocating the structure
                                   {IEMEM_PROBE(iemem_remoteServers, 1), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<sizeof(failSequence7)/sizeof(failSequence7[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence7[loop][0];
            probeCounts[0] = failSequence7[loop][1];
            expectedRC = failSequence7[loop][2];
        }

        // Try (re)initializing the engine remote servers code
        rc = iers_initEngineRemoteServers(pThreadData);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    ismEngine_serverGlobal.clusterEnabled = false;
}

//****************************************************************************
/// @brief Test the capabilities of remote server recovery when allocs fail
///        by faking calls to recovery routines
//****************************************************************************
void test_capability_RemoteServerRecovery(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc;
    int32_t expectedRC = OK;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    // Recover a local server definition record
    uint32_t failSequence1[][3] = {// Fail in iers_rehydrateServerDefn allocating structure
                                   {IEMEM_PROBE(iemem_remoteServers, 6), 0, ISMRC_AllocateError},
                                   // Fail attempting to allocate the page map structure for the queue
                                   {IEMEM_PROBE(iemem_multiConsumerQ, 3), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK}
                                  };

    ismStore_Handle_t fakeStoreHandle = 99;
    ismStore_Record_t fakeStoreRecord = {0};
    iestRemoteServerDefinitionRecord_t remsrvDefinitionRecord;
    char *Fragments[1];
    uint32_t FragmentLengths[1];

    memcpy(&remsrvDefinitionRecord.StrucId, iestREMSRV_DEFN_RECORD_STRUCID, 4);
    remsrvDefinitionRecord.Local = true;
    remsrvDefinitionRecord.Version = iestRDR_CURRENT_VERSION;

    Fragments[0] = (char *)&remsrvDefinitionRecord;
    FragmentLengths[0] = sizeof(remsrvDefinitionRecord);
    fakeStoreRecord.DataLength = FragmentLengths[0];

    fakeStoreRecord.Type = ISM_STORE_RECTYPE_REMSRV;
    fakeStoreRecord.Attribute = 0;
    fakeStoreRecord.State = iestRDR_STATE_NONE;
    fakeStoreRecord.pFrags = Fragments;
    fakeStoreRecord.pFragsLengths = FragmentLengths;
    fakeStoreRecord.FragsCount = 1;

    void *rehydratedRecord = NULL;

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        // Actually try and create a queue this time
        if (loop == 1) remsrvDefinitionRecord.Local = false;

        // Call the rehydration function with our fake data
        rc = iers_rehydrateServerDefn(pThreadData,
                                            fakeStoreHandle,
                                            &fakeStoreRecord,
                                            NULL,
                                            &rehydratedRecord,
                                            NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    TEST_ASSERT_PTR_NOT_NULL(rehydratedRecord);

    // Recover a remote server properties record
    uint32_t failSequence2[][3] = {// Fail in iers_rehydrateServerProps allocating cluster data
                                   {IEMEM_PROBE(iemem_remoteServers, 15), 0, ISMRC_AllocateError},
                                   // Fail in iers_rehydrateServerProps allocating structure
                                   {IEMEM_PROBE(iemem_remoteServers, 7), 0, ISMRC_AllocateError},
                                   // Fail in iers_rehydrateServerProps allocating name
                                   {IEMEM_PROBE(iemem_remoteServers, 14), 0, ISMRC_AllocateError},
                                  };

    void *requestingRecord = rehydratedRecord; // Prepare for properties record
    rehydratedRecord = NULL;

    char fragmentBuffer[sizeof(iestRemoteServerPropertiesRecord_t)+100];
    iestRemoteServerPropertiesRecord_t *remsrvPropertiesRecord = (iestRemoteServerPropertiesRecord_t *)fragmentBuffer;
    char *tempPtr = fragmentBuffer+sizeof(iestRemoteServerPropertiesRecord_t);

    TEST_ASSERT_EQUAL(iestRPR_CURRENT_VERSION, iestRPR_VERSION_1);

    memcpy(&remsrvPropertiesRecord->StrucId, iestREMSRV_PROPS_RECORD_STRUCID, 4);
    remsrvPropertiesRecord->Version = iestRPR_CURRENT_VERSION;
    remsrvPropertiesRecord->InternalAttrs = iersREMSRVATTR_LOCAL;

    strcpy(tempPtr, "FAKEUID");
    remsrvPropertiesRecord->UIDLength = strlen(tempPtr)+1;
    tempPtr += remsrvPropertiesRecord->UIDLength;

    strcpy(tempPtr, "FAKENAME");
    remsrvPropertiesRecord->NameLength = strlen(tempPtr)+1;
    tempPtr += remsrvPropertiesRecord->NameLength;

    strcpy(tempPtr, "FAKECLUSTERDATA");
    remsrvPropertiesRecord->ClusterDataLength = strlen(tempPtr)+1;
    tempPtr += remsrvPropertiesRecord->ClusterDataLength;

    Fragments[0] = fragmentBuffer;
    FragmentLengths[0] = tempPtr - fragmentBuffer;
    fakeStoreRecord.DataLength = FragmentLengths[0];

    fakeStoreRecord.Type = ISM_STORE_RECTYPE_RPROP;
    fakeStoreRecord.Attribute = 99;
    fakeStoreRecord.State = iestRPR_STATE_NONE;
    fakeStoreRecord.pFrags = Fragments;
    fakeStoreRecord.pFragsLengths = FragmentLengths;
    fakeStoreRecord.FragsCount = 1;

    for(loop=0; loop<sizeof(failSequence2)/sizeof(failSequence2[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence2[loop][0];
            probeCounts[0] = failSequence2[loop][1];
            expectedRC = failSequence2[loop][2];
        }

        // Call the rehydration function with our fake data
        rc = iers_rehydrateServerProps(pThreadData,
                                             fakeStoreHandle,
                                             &fakeStoreRecord,
                                             NULL,
                                             requestingRecord,
                                             &rehydratedRecord,
                                             NULL);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);
}

//****************************************************************************
/// @brief Test problems allocating memory when seeding a remote server
//****************************************************************************
void test_capability_SeedingErrors(void)
{
    int32_t rc;
    int32_t expectedRC = OK;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    printf("Starting %s...\n", __func__);

    // Get a retained message in place
    rc = test_createClientAndSession("FailedAllocsClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    void *payload1=NULL;
    ismEngine_MessageHandle_t hMessage1;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, "FAILED/ALLOCS/TOPIC",
                            &hMessage1, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            "FAILED/ALLOCS/TOPIC",
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL );
    TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Pretend that the cluster is enabled
    ismEngine_serverGlobal.clusterEnabled = true;

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    // ENGINE_RS_CREATE tests
    uint32_t failSequence1[][3] = {// Fail in iett_findOriginServerRetainedMessages allocating array
                                   {IEMEM_PROBE(iemem_topicsTree, 9), 0, ISMRC_AllocateError},
                                   // Fail in iett_listLocalOriginServers allocating originServer array
                                   {IEMEM_PROBE(iemem_topicsTree, 11), 0, ISMRC_AllocateError},
                                  };

    ismEngine_RemoteServerHandle_t remoteServerHandle = ismENGINE_NULL_REMOTESERVER_HANDLE;

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        // Try creating a remote server (on one loop, we create a remote server for the local server)
        // to ensure that an allocation failure there does work correctly
        rc = iers_clusterEventCallback(ENGINE_RS_CREATE,
                                       ismENGINE_NULL_REMOTESERVER_HANDLE,
                                       (ismCluster_RemoteServerHandle_t)1,
                                       "FailedAllocs",
                                       "FAUID",
                                       NULL, 0,
                                       NULL, 0,
                                       false, false, NULL, NULL,
                                       NULL,
                                       &remoteServerHandle);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    TEST_ASSERT_PTR_NULL(remoteServerHandle);

    // TODO: More tests?

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    ismEngine_serverGlobal.clusterEnabled = false;

    // Unset any retained messages left behind
    rc = test_createClientAndSession("FailedAllocsClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_unsetRetainedMessageOnDestination(hSession,
                                                      ismDESTINATION_REGEX_TOPIC,
                                                      ".*",
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_RemoteServers[] =
{
    { "RemoteServersErrors", test_capability_RemoteServersErrors},
    { "RemoteServerRecovery", test_capability_RemoteServerRecovery},
    { "SeedingErrors", test_capability_SeedingErrors},
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the capabilities of transactions when allocs fail.
//****************************************************************************
void test_capability_TransactionErrors(void)
{
    int32_t rc;
    int32_t expectedRC = OK;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};
    int32_t loop;
    ismEngine_ClientStateHandle_t hClient1;
    ismEngine_SessionHandle_t hSession1;
    ism_xid_t XID;
    ismEngine_TransactionHandle_t hTran;
    struct
    {
        uint64_t gtrid;
        uint64_t bqual;
    } globalId;

    printf("Starting %s...\n", __func__);

    // Create an initial client and session
    rc = test_createClientAndSession("CLIENT_FAIL",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_TRANSACTIONAL,
                                     &hClient1,
                                     &hSession1,
                                     false);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    uint32_t failSequence1[][3] = {// Fail in ietr_createGlobal allocating the transaction structure
                                   {IEMEM_PROBE(iemem_globalTransactions, 1), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry duplicating key string
                                   {IEMEM_PROBE(iemem_globalTransactions, 60002), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    for(loop=0; loop<sizeof(failSequence1)/sizeof(failSequence1[0]); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        // Try creating a remote server (on one loop, we create a remote server for the local server)
        memset(&XID, 0, sizeof(ism_xid_t));
        XID.formatID = 0xC1D1E1F1;
        XID.gtrid_length = sizeof(uint64_t);
        XID.bqual_length = sizeof(uint64_t);
        globalId.gtrid = 0xFEDCBA9876543210;
        globalId.bqual = 0x0123456789ABCDEF;
        memcpy(&XID.data, &globalId, sizeof(globalId));

        // Create a transaction
        rc = sync_ism_engine_createGlobalTransaction(hSession1,
                                                &XID,
                                                ismENGINE_CREATE_TRANSACTION_OPTION_XA_TMNOFLAGS,
                                                &hTran);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);

        if (failSequence1[loop][2] != OK)
        {
            ismEngine_Transaction_t *pTran = NULL;
            rc = ietr_findGlobalTransaction(pThreadData, &XID, &pTran);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
            TEST_ASSERT_EQUAL(pTran, NULL);
        }
    }

    TEST_ASSERT_PTR_NOT_NULL(hTran);

    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_TransactionErrors[] =
{
    { "TransactionErrors", test_capability_TransactionErrors},
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the capabilities of exporting / importing when allocs fail.
//****************************************************************************
void test_capability_ExportImportErrors(void)
{
    int32_t rc;
    int32_t expectedRC = OK;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    int32_t loop;
    uint32_t failProbes[10] = {0};
    uint32_t probeCounts[10] = {0};

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->userid = "FailedAllocsUser";

    //Initialise OpenSSL - in the product - the equivalent is ism_ssl_init();
    //TODO: Do we need to run that in the engine thread that does this or is it done?
    sslInit();

    // Use a really long clientID that will require some reallocation during export
    uint32_t clientIdLength = 1024;
    char *clientId = malloc(clientIdLength);

    memset(clientId, 'F', clientIdLength-1);
    clientId[clientIdLength-1] = '\0';

    const char *topicString = "FAILED/EXPORT_IMPORT/TOPIC";

    // Create the artifacts to export
    rc = sync_ism_engine_createClientState(clientId,
                                           testDEFAULT_PROTOCOL_ID,
                                           ismENGINE_CREATE_CLIENT_OPTION_NONE | ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                           NULL, NULL,
                                           mockContext,
                                           &hClient);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    // Add an unreleased delivery Ids
    ismEngine_UnreleasedHandle_t unreleasedDeliveryIdHandle;
    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_addUnreleasedDeliveryId(hSession,
                                            NULL,
                                            (rand()%65535)+1,
                                            &unreleasedDeliveryIdHandle,
                                            &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT_EQUAL(rc, ISMRC_AsyncCompletion);

    // PUBLISH A RETAINED MSG
    char *payload="PAYLOAD";
    ismEngine_MessageHandle_t hMessage;
    void *payloadPtr = payload;

    rc = test_createMessage(strlen(payload)+1,
                            ismMESSAGE_PERSISTENCE_PERSISTENT,
                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, topicString,
                            &hMessage, &payloadPtr);
    TEST_ASSERT_EQUAL(rc, OK);

    test_incrementActionsRemaining(pActionsRemaining, 1);
    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            topicString,
                                            NULL,
                                            hMessage,
                                            &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc != ISMRC_AsyncCompletion)
    {
        TEST_ASSERT_GREATER_THAN(ISMRC_Error, rc); // Informational or OK
        test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    }

    test_waitForRemainingActions(pActionsRemaining);

    rc = sync_ism_engine_destroySession(hSession);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = sync_ism_engine_destroyClientState(hClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_NONE);

    TEST_ASSERT_EQUAL(rc, OK);

    printf("Starting %s...\n", __func__);

    uint32_t originalThreadId = test_memoryFailMemoryProbes(failProbes, probeCounts);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    char importExportFile[255];
    char *exportFilePath = NULL;
    char *importFilePath = NULL;

    strcpy(importExportFile, __func__);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, importExportFile, false, &exportFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exportFilePath);

    rc = ieie_fullyQualifyResourceFilename(pThreadData, importExportFile, true, &importFilePath);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(importFilePath);

    // *** Exporting ClientStates ***
    uint32_t failSequence1[][3] = {// Fail in ism_engine_exportResources allocating the control structure
                                   {IEMEM_PROBE(iemem_exportResources, 9), 0, ISMRC_AllocateError},
                                   // Fail in ieie_fullyQualifyResourceFilename allocating the localFilePath
                                   {IEMEM_PROBE(iemem_exportResources, 1), 0, ISMRC_AllocateError},
                                   // Fail in ieie_allocateRequestId allocating the statusFilePath
                                   {IEMEM_PROBE(iemem_exportResources, 16), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashSet when trying to allocate exported message hashset
                                   {IEMEM_PROBE(iemem_exportResources, 60100), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable when trying to allocate clientId hash table
                                   {IEMEM_PROBE(iemem_exportResources, 60000), 0, ISMRC_AllocateError},
                                   // Fail in ieie_writeResourceFileHeader allocating the header structure
                                   {IEMEM_PROBE(iemem_exportResources, 3), 0, ISMRC_AllocateError},
                                   // Fail in ieie_exportClientStates allocating the CSI buffer
                                   {IEMEM_PROBE(iemem_exportResources, 7), 0, ISMRC_AllocateError},
                                   // Fail in ieie_exportClientState re-allocating the CSI buffer
                                   {IEMEM_PROBE(iemem_exportResources, 6), 0, ISMRC_AllocateError},
                                   // Fail in ieie_writeResourceFileFooter allocating the footer structure
                                   {IEMEM_PROBE(iemem_exportResources, 8), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence1)/sizeof(failSequence1[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence1[loop][0];
            probeCounts[0] = failSequence1[loop][1];
            expectedRC = failSequence1[loop][2];
        }

        uint64_t requestID = 0;
        rc = sync_ism_engine_exportResources(clientId,
                                             topicString,
                                             importExportFile,
                                             "PASSWORD",
                                             ismENGINE_EXPORT_RESOURCES_OPTION_OVERWRITE,
                                             &requestID);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    rc = test_copyFile(exportFilePath, importFilePath);
    TEST_ASSERT_EQUAL(rc, 0);

    // *** Importing ClientStates ***
    uint32_t failSequence2[][3] = {// Fail in ism_engine_importResources allocating the control structure
                                   {IEMEM_PROBE(iemem_exportResources, 10), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable creating the validated ClientIds hash table
                                   {IEMEM_PROBE(iemem_exportResources, 60000), 0, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable creating the imported Messages hash table
                                   {IEMEM_PROBE(iemem_exportResources, 60000), 1, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable creating the imported clientStates hash table
                                   {IEMEM_PROBE(iemem_exportResources, 60000), 2, ISMRC_AllocateError},
                                   // Fail in ieut_createHashTable creating the imported subscriptions hash table
                                   {IEMEM_PROBE(iemem_exportResources, 60000), 3, ISMRC_AllocateError},
                                   // Fail in ieie_OpenEncryptedFile allocating the file handle
                                   {IEMEM_PROBE(iemem_exportResources, 60502), 0, ISMRC_FileCorrupt},
                                   // Fail in ieieValidateClientStateImport allocating the clientId
                                   {IEMEM_PROBE(iemem_exportResources, 2), 0, ISMRC_AllocateError},
                                   // Fail in ieut_putHashEntry when allocating the chain in validatedClients table
                                   {IEMEM_PROBE(iemem_exportResources, 60004), 0, ISMRC_AllocateError},
                                   // Fail in ieie_importClientState allocating the callback context
                                   {IEMEM_PROBE(iemem_exportResources, 5), 0, ISMRC_AllocateError},
                                   // Fail in ieie_importClientState allocating the UMS array
                                   {IEMEM_PROBE(iemem_exportResources, 13), 0, ISMRC_AllocateError},
                                   // Don't fail
                                   {0, 0, OK},
                                  };

    for(loop=0; loop<(sizeof(failSequence2)/sizeof(failSequence2[0])); loop++)
    {
        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis type %d %s line %d\n", IEMEM_GET_MEMORY_PROBEID(analysisType), __func__, __LINE__);
            failProbes[0] = analysisType;
            probeCounts[0] = 0;
        }
        else
        {
            failProbes[0] = failSequence2[loop][0];
            probeCounts[0] = failSequence2[loop][1];
            expectedRC = failSequence2[loop][2];
        }

        uint64_t requestID = 0;
        rc = sync_ism_engine_importResources(importExportFile,
                                             "PASSWORD",
                                             ismENGINE_IMPORT_RESOURCES_OPTION_NONE,
                                             &requestID);

        if (analysisType != TEST_ANALYSE_IEMEM_NONE)
        {
            printf("Analysis complete\n");
            break;
        }

        TEST_ASSERT_EQUAL(rc, expectedRC);
    }

    // Back to normal
    test_memoryFailMemoryProbes(NULL, &originalThreadId);

    // Clean up retained message
    rc = ism_engine_unsetRetainedMessageOnDestination(NULL,
                                                      ismDESTINATION_REGEX_TOPIC, topicString,
                                                      ismENGINE_UNSET_RETAINED_OPTION_NONE,
                                                      ismENGINE_UNSET_RETAINED_DEFAULT_SERVER_TIME,
                                                      NULL,
                                                      NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Get rid of the client
    rc = ism_engine_destroyDisconnectedClientState(clientId, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    sslTerm();

    free(clientId);
    iemem_free(pThreadData, iemem_exportResources, exportFilePath);
    iemem_free(pThreadData, iemem_exportResources, importFilePath);
}

CU_TestInfo ISM_FailedAllocs_CUnit_test_capability_ExportImportErrors[] =
{
    { "ExportImportErrors", test_capability_ExportImportErrors},
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initFailedAllocs(void)
{
    return test_engineInit_DEFAULT;
}

int termFailedAllocs(void)
{
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_FailedAllocs_CUnit_allsuites[] =
{
    IMA_TEST_SUITE("UtilityDataTypeErrors", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_UtilityDataTypeErrors),
    IMA_TEST_SUITE("SubscriptionErrors", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_SubscriptionErrors),
    IMA_TEST_SUITE("TopicTreeErrors", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_TopicTreeErrors),
    IMA_TEST_SUITE("PublicationErrors", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_PublicationErrors),
    IMA_TEST_SUITE("ClientAndSessionErrors", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_ClientAndSessionErrors),
    IMA_TEST_SUITE("QueueNamespaceErrors", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_QueueNamespaceErrors),
    IMA_TEST_SUITE("PolicyInfoErrors", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_PolicyInfoErrors),
    IMA_TEST_SUITE("DeliveryErrors", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_DeliveryErrors),
    IMA_TEST_SUITE("ExpiryErrors", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_ExpiryErrors),
    IMA_TEST_SUITE("RemoteServers", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_RemoteServers),
    IMA_TEST_SUITE("TransactionErrors", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_TransactionErrors),
    IMA_TEST_SUITE("ExportImportErrors", initFailedAllocs, termFailedAllocs, ISM_FailedAllocs_CUnit_test_capability_ExportImportErrors),
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
            if (!strcasecmp(argv[i], "ANALYSE"))
            {
                analysisType = TEST_ANALYSE_IEMEM_PROBE;
            }
            else if (!strcasecmp(argv[i], "ANALYSE_STACK"))
            {
                analysisType = TEST_ANALYSE_IEMEM_PROBE_STACK;
            }
            else if (!strcasecmp(argv[i], "FULL"))
            {
                if (i != 1)
                {
                    retval = 99;
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

#if defined(NO_MALLOC_WRAPPER)
    printf("Malloc wrapper disabled\n");
#else
    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    ism_time_t seedVal = ism_common_currentTimeNanos();

    srand(seedVal);

    CU_initialize_registry();

    retval = setup_CU_registry(argc, argv, ISM_FailedAllocs_CUnit_allsuites);

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
#endif

mod_exit:

    return retval;
}
