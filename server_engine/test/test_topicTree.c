
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
/* Module Name: test_topicTree.c                                     */
/*                                                                   */
/* Description: Main source file for CUnit test of Topic Tree        */
/*                                                                   */
/*********************************************************************/
#include <math.h>

#include "topicTree.h"
#include "topicTreeInternal.h"
#include "engineHashTable.h"
#include "memHandler.h"
#include "queueCommon.h"
#include "selector.h"
#include "engineStore.h"
#include "policyInfo.h"
#include "engineDump.h"
#include "dumpFormat.h"
#include "engineUtils.h"
#include "resourceSetStats.h"
#include "clientState.h"

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_file.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"
#include "test_utils_security.h"

typedef struct tag_genericMsgCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
    int32_t expected;
    int32_t reliable;
    int32_t unreliable;
} genericMsgCbContext_t;

bool genericMessageCallback(ismEngine_ConsumerHandle_t hConsumer,
                            ismEngine_DeliveryHandle_t hDelivery,
                            ismEngine_MessageHandle_t hMessage,
                            uint32_t deliveryId,
                            ismMessageState_t state,
                            uint32_t destinationOptions,
                            ismMessageHeader_t *pMsgDetails,
                            uint8_t areaCount,
                            ismMessageAreaType_t areaTypes[areaCount],
                            size_t areaLengths[areaCount],
                            void *pAreaData[areaCount],
                            void *pContext,
                            ismEngine_DelivererContext_t * _delivererContext)
{
    genericMsgCbContext_t *context = *((genericMsgCbContext_t **)pContext);

    // char *topic = test_extractMsgPropertyID(ID_Topic, (ismEngine_Message_t *)hMessage);

    __sync_add_and_fetch(&context->received, 1);

    if (pMsgDetails->Reliability == ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
    {
        __sync_add_and_fetch(&context->unreliable, 1);
    }
    else
    {
        __sync_add_and_fetch(&context->reliable, 1);
    }

    ism_engine_releaseMessage(hMessage);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        NULL,
                                                        0,
                                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    return true; // more messages, please.
}

int32_t iett_findMatchingSubsNodes(ieutThreadData_t *pThreadData,
                                   const iettSubsNode_t *parent,
                                   const iettTopic_t    *topic,
                                   const uint32_t        curIndex,
                                   const bool            usingCachedArray,
                                   uint32_t             *maxNodes,
                                   uint32_t             *nodeCount,
                                   iettSubsNode_t     ***result);

#define TOPIC_STRING_CHARS            "\" abcdefghijklmnopqrstuvwxyz' ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789*!:\\"
#define TOPIC_STRING_WILDCHARS        "+#"

uint32_t SUB_DELIVERY_OPTIONS[] = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
                                    ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE,
                                    ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE,
                                    ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE };
#define SUB_DELIVERY_NUMOPTIONS (sizeof(SUB_DELIVERY_OPTIONS)/sizeof(SUB_DELIVERY_OPTIONS[0]))
uint32_t SUB_OTHER_OPTIONS[] = { ismENGINE_SUBSCRIPTION_OPTION_NONE,
                                 ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL };
#define SUB_OTHER_NUMOPTIONS (sizeof(SUB_OTHER_OPTIONS)/sizeof(SUB_OTHER_OPTIONS[0]))

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_debugAndDumpTopic                            */
/*                                                                   */
/* Description:    Test ism_engine_debugTopic and dumpTopic.         */
/*                                                                   */
/*********************************************************************/
void test_debugAndDumpTopic(const char *topicString, bool show)
{
    int origStdout = -1;

    if (!show)
    {
        origStdout = test_redirectStdout("/dev/null");
        TEST_ASSERT_NOT_EQUAL(origStdout, -1);
    }

    char tempDirectory[500] = {0};

    test_utils_getTempPath(tempDirectory);
    TEST_ASSERT_NOT_EQUAL(tempDirectory[0], 0);

    strcat(tempDirectory, "/test_topicTree_debug_XXXXXX");

    if (mkdtemp(tempDirectory) == NULL)
    {
        TEST_ASSERT(false, ("mkdtemp() failed with %d\n", errno));
    }

    // Test the topic debugging function with 10 subscribers
    char cwdbuf[500];
    char *cwd = getcwd(cwdbuf, sizeof(cwdbuf));
    TEST_ASSERT_PTR_NOT_NULL(cwd);

    void *libHandle = NULL;
    iefmReadAndFormatFile_t iefm_readAndFormatFileFn;

    if (!show)
    {
        libHandle = dlopen(iefmDUMP_FORMAT_LIB, RTLD_LAZY | RTLD_GLOBAL);
        TEST_ASSERT_PTR_NOT_NULL(libHandle);
        iefm_readAndFormatFileFn = dlsym(libHandle, iefmDUMP_FORMAT_FUNC);
        TEST_ASSERT_PTR_NOT_NULL(iefm_readAndFormatFileFn);
    }
    else
    {
        iefm_readAndFormatFileFn = NULL;
    }

    if (chdir(tempDirectory) == 0)
    {
        char tempFilename[strlen(tempDirectory)+strlen(".partial")+20];
        int32_t rc;

        sprintf(tempFilename, "%s/dumpTopic_1", tempDirectory);
        rc = ism_engine_dumpTopic(topicString, 1, 0, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopic_1.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpTopic_9_0", tempDirectory);
        rc = ism_engine_dumpTopic(topicString, 9, 0, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopic_9_0.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpTopic_9_50", tempDirectory);
        rc = ism_engine_dumpTopic(topicString, 9, 50, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopic_9_50.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        if (iefm_readAndFormatFileFn != NULL)
        {
            iefmHeader_t dumpHeader = {0};

            dumpHeader.inputFilePath = tempFilename;
            dumpHeader.outputFile = NULL;
            dumpHeader.outputPath = tempDirectory;
            dumpHeader.showProgress = true;

            (void)iefm_readAndFormatFileFn(&dumpHeader);
        }

        sprintf(tempFilename, "%s/dumpTopic_9_All", tempDirectory);
        rc = ism_engine_dumpTopic(topicString, 9, -1, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopic_9_All.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        if (iefm_readAndFormatFileFn != NULL)
        {
            iefmHeader_t dumpHeader = {0};

            dumpHeader.inputFilePath = tempFilename;
            dumpHeader.outputFile = stdout;
            dumpHeader.showProgress = true;

            (void)iefm_readAndFormatFileFn(&dumpHeader);
        }

        if(chdir(cwd) != 0)
        {
            TEST_ASSERT(false, ("chdir() back to old cwd failed with errno %d\n", errno));
        }
    }

    if (libHandle != NULL)
    {
        dlclose(libHandle);
    }

    test_removeDirectory(tempDirectory);

    if (origStdout != -1)
    {
        int32_t rc = test_reinstateStdout(origStdout);
        TEST_ASSERT_NOT_EQUAL(rc, -1);
    }
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_dumpTopicTree                                */
/*                                                                   */
/* Description:    Test ism_engine_dumpTopicTree.                    */
/*                                                                   */
/*********************************************************************/
void test_dumpTopicTree(const char *rootTopicString)
{
    char tempDirectory[500] = {0};

    test_utils_getTempPath(tempDirectory);
    TEST_ASSERT_NOT_EQUAL(tempDirectory[0], 0);

    strcat(tempDirectory, "/test_topicTree_debug_XXXXXX");

    if (mkdtemp(tempDirectory) == NULL)
    {
        TEST_ASSERT(false, ("mkdtemp() failed with %d\n", errno));
    }

    // Test the topic debugging function with 10 subscribers
    char cwdbuf[500];
    char *cwd = getcwd(cwdbuf, sizeof(cwdbuf));
    TEST_ASSERT_PTR_NOT_NULL(cwd);

    if (chdir(tempDirectory) == 0)
    {
        char tempFilename[strlen(tempDirectory)+strlen(".partial")+20];
        int32_t rc;

        sprintf(tempFilename, "%s/dumpTopicTree_1", tempDirectory);
        rc = ism_engine_dumpTopicTree(rootTopicString, 1, 0, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopicTree_1.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpTopicTree_7", tempDirectory);
        rc = ism_engine_dumpTopicTree(rootTopicString, 7, 0, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopicTree_7.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpTopicTree_9_0", tempDirectory);
        rc = ism_engine_dumpTopicTree(rootTopicString, 9, 0, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopicTree_9_0.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpTopicTree_9_50", tempDirectory);
        rc = ism_engine_dumpTopicTree(rootTopicString, 9, 50, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopicTree_9_50.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(tempFilename, "%s/dumpTopicTree_9_All", tempDirectory);
        rc = ism_engine_dumpTopicTree(rootTopicString, 9, -1, tempFilename);
        TEST_ASSERT_PTR_NOT_NULL(strstr(tempFilename, "dumpTopicTree_9_All.dat"));
        TEST_ASSERT_EQUAL(rc, OK);

        if(chdir(cwd) != 0)
        {
            TEST_ASSERT(false, ("chdir() back to old cwd failed with errno %d\n", errno));
        }
    }

    test_removeDirectory(tempDirectory);
}


void test_publishMessages(const char *topicString,
                          uint32_t msgCount,
                          uint8_t reliability,
                          uint8_t persistence)
{
    ismEngine_MessageHandle_t hMessage;
    void *payload=NULL;

    for(uint32_t i=0; i<msgCount; i++)
    {
        payload = NULL;

        int32_t rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                        persistence,
                                        reliability,
                                        ismMESSAGE_FLAGS_NONE,
                                        0,
                                        ismDESTINATION_TOPIC, topicString,
                                        &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                        topicString,
                                                        hMessage,
                                                        NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        if (payload) free(payload);
    }
}

void test_checkConsistentTopicAnalysis(ieutThreadData_t *pThreadData, iettTopic_t *topic)
{
    const char  *substrings[10];
    uint32_t     substringHashes[10];
    const char  *wildcards[10];
    const char  *multicards[10];

    iettTopic_t checkTopic = {0};

    checkTopic.destinationType = topic->destinationType;
    checkTopic.topicString = topic->topicString;
    checkTopic.substrings = substrings;
    checkTopic.substringHashes = substringHashes;
    checkTopic.wildcards = wildcards;
    checkTopic.multicards = multicards;
    checkTopic.initialArraySize = 10;

    int32_t rc = iett_analyseTopicString(pThreadData, &checkTopic);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(checkTopic.substringCount, topic->substringCount);

    // Check that the substrings and hashes are the same
    for(int32_t i=0; i<topic->substringCount; i++)
    {
        TEST_ASSERT_STRINGS_EQUAL(topic->substrings[i], checkTopic.substrings[i]);
        TEST_ASSERT_EQUAL(topic->substringHashes[i], checkTopic.substringHashes[i]);
    }

    // Check wildcards
    for(int32_t i=0; i<topic->wildcardCount; i++)
    {
        TEST_ASSERT_STRINGS_EQUAL(topic->wildcards[i], checkTopic.wildcards[i]);
    }

    // Check multicards
    for(int32_t i=0; i<topic->multicardCount; i++)
    {
        TEST_ASSERT_STRINGS_EQUAL(topic->multicards[i], checkTopic.multicards[i]);
    }

    if (checkTopic.topicStringCopy != NULL)
    {
        iemem_free(pThreadData, iemem_topicAnalysis, checkTopic.topicStringCopy);

        if (checkTopic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, checkTopic.substrings);
        if (checkTopic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, checkTopic.substringHashes);
        if (checkTopic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, checkTopic.wildcards);
        if (checkTopic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, checkTopic.multicards);
    }
}

ismEngine_MessageSelectionCallback_t OriginalSelectionCallback = NULL;

typedef struct tag_test_selectionCallback_t
{
    const char *expectTopic;
} test_selectionCallback_t;

test_selectionCallback_t *pSelectionExpected = NULL;

int32_t test_selectionCallback(ismMessageHeader_t * pMsgDetails,
                               uint8_t              areaCount,
                               ismMessageAreaType_t areaTypes[areaCount],
                               size_t               areaLengths[areaCount],
                               void *               pareaData[areaCount],
                               const char *         topic,
                               const void *         pselectorRule,
                               size_t               selectorRuleLen,
                               ismMessageSelectionLockStrategy_t * lockStrategy )
{
    TEST_ASSERT_PTR_NOT_NULL(OriginalSelectionCallback);

    if (pSelectionExpected)
    {
        ismEngine_Message_t fakeMsg;

        memset(&fakeMsg, 0, sizeof(fakeMsg));
        ismEngine_SetStructId(fakeMsg.StrucId, ismENGINE_MESSAGE_STRUCID);
        fakeMsg.usageCount = 1;
        memcpy(&fakeMsg.Header, pMsgDetails, sizeof(ismMessageHeader_t));
        fakeMsg.AreaCount = areaCount;
        memcpy(&fakeMsg.AreaTypes, areaTypes, areaCount * sizeof(ismMessageAreaType_t));
        memcpy(&fakeMsg.AreaLengths, areaLengths, areaCount * sizeof(size_t));
        memcpy(&fakeMsg.pAreaData, pareaData, areaCount * sizeof(void *));
        // fakeMsg.MsgLength ?

        char *extractedTopic = test_extractMsgPropertyID(ID_Topic, &fakeMsg);

        if (pSelectionExpected->expectTopic)
        {
            // Does it match the topic we're being called with.
            TEST_ASSERT_STRINGS_EQUAL(topic, pSelectionExpected->expectTopic);
            // Does it match the topic in the message?
            if (extractedTopic != NULL)
            {
                TEST_ASSERT_STRINGS_EQUAL(topic, extractedTopic);
            }
        }
    }

    return OriginalSelectionCallback(pMsgDetails, areaCount, areaTypes, areaLengths, pareaData, topic, pselectorRule, selectorRuleLen, lockStrategy);
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_func_iett_analyseTopicString                 */
/*                                                                   */
/* Description:    Test the iett_analyseTopicString function.        */
/*                                                                   */
/*********************************************************************/
#define REPORT_ON_BUCKETS      0 // Set to >0 to get hash information
#define TOTAL_ITERATIONS       3
#define MAX_SUBSTRINGS         1000
#define MAX_SUBSTRING_LEN      20
#define FIXED_SUBSTRING        "TestSubstring"

void test_func_iett_analyseTopicString(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t   rc = OK;

    char        *topicString;
    uint32_t     stringHash;
    iettTopic_t  topic;
    const char  *substrings[10];
    uint32_t     substringHashes[10];
    const char  *wildcards[10];
    const char  *multicards[10];

    char       thissubstring[MAX_SUBSTRING_LEN+1];

    int        i,j,k,m,w;

    uint32_t   bucket10[10] = {0};
    uint32_t   bucket32[32] = {0};
    uint32_t   bucket100[100] = {0};
    uint32_t   bucket1000[1000] = {0};
    uint32_t   bucket10000[10000] = {0};

    uint32_t   iteration = 0;

    topicString = malloc((MAX_SUBSTRING_LEN+1)*MAX_SUBSTRINGS);

    TEST_ASSERT_PTR_NOT_NULL(topicString);

    topicString[0] = '\0';

    printf("Starting %s...\n", __func__);

    // Test recognition of a $SYS* topic string
    memset(&topic, 0, sizeof(topic));
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 10;

    char *sysTopicTests[] = {ismENGINE_DOLLARSYS_PREFIX,
                             ismENGINE_DOLLARSYS_PREFIX "/Test/#",
                             ismENGINE_DOLLARSYS_PREFIX "TEMATIC/+",
                             "TEST" ismENGINE_DOLLARSYS_PREFIX "/A",
                             "TEST/" ismENGINE_DOLLARSYS_PREFIX,
                             "$Systolic/Blood/Pressure",
                             "$sys/Test/#",
                             NULL};
    int sysTopicResults[] = { 1, 1, 1, 0, 0, 1, 1};

    for(i=0; sysTopicTests[i] != NULL; i++)
    {
        topic.destinationType = ismDESTINATION_TOPIC;
        topic.topicString = sysTopicTests[i];
        rc = iett_analyseTopicString(pThreadData, &topic);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(topic.sysTopicEndIndex, sysTopicResults[i]);

        test_checkConsistentTopicAnalysis(pThreadData, &topic);

        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);
        topic.topicStringCopy = NULL;
    }

    // Test for consistent HASH generation
    for(i=0;i<20;i++)
    {
        memset(&topic, 0, sizeof(topic));
        topic.substrings = substrings;
        topic.substringHashes = substringHashes;
        topic.wildcards = wildcards;
        topic.multicards = multicards;
        topic.initialArraySize = 10;

        strcat(topicString, FIXED_SUBSTRING);

        stringHash = iett_generateTopicStringHash(topicString);

        topic.destinationType = ismDESTINATION_TOPIC;
        topic.topicString = topicString;

        rc = iett_analyseTopicString(pThreadData, &topic);

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(topic.topicStringCopy);
        TEST_ASSERT_EQUAL(topic.topicStringLength, strlen(topicString));
        TEST_ASSERT_EQUAL(topic.substringCount,i+1);
        TEST_ASSERT_EQUAL(topic.substringHashes[0], topic.substringHashes[i]);
        TEST_ASSERT_PTR_NULL(topic.substrings[i+1])
        TEST_ASSERT_EQUAL(topic.sysTopicEndIndex, 0);
        // Check that iett_generateSubstringHash comes out with the same
        // value as iett_analyseTopicString
        TEST_ASSERT_EQUAL(topic.substringHashes[i], iett_generateSubstringHash(FIXED_SUBSTRING));
        TEST_ASSERT_EQUAL(topic.substringHashes[0], iett_generateSubstringHash(topic.topicStringCopy));

#if REPORT_ON_BUCKETS
        for(j=0;j<=i;j++)
        {
            printf("%08X ", substringHashes[j]);
        }
        printf("%08X ", substringHashes[j]);
        printf("(%08X = %d)\n", stringHash, stringHash%100);
#endif

        test_checkConsistentTopicAnalysis(pThreadData, &topic);

        iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);


        strcat(topicString, "/");

        if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
        if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
        if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
        if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
    }

    // Test for variation & long strings
    do
    {
        topicString[0] = '\0';
        m = 0;
        w = 0;

        for(i=0;i<MAX_SUBSTRINGS;i++)
        {
            memset(&topic, 0, sizeof(topic));
            topic.substrings = substrings;
            topic.substringHashes = substringHashes;
            topic.wildcards = wildcards;
            topic.multicards = multicards;
            topic.initialArraySize = 10;

            j = 0;

            // Wildcards 5% of the time, and 5% of those become strings with wildcards
            // that are not wild (e.g. ##. #+ etc)
            if (rand()%20 == 0)
            {
                thissubstring[j++] = TOPIC_STRING_WILDCHARS[rand()%strlen(TOPIC_STRING_WILDCHARS)];
                if (rand()%20 == 0)
                {
                    thissubstring[j++] = TOPIC_STRING_WILDCHARS[rand()%strlen(TOPIC_STRING_WILDCHARS)];
                }
                else
                {
                    if (thissubstring[0] == '+') w++;
                    else if (thissubstring[0] == '#') m++;
                }
            }
            else
            {
                int addlen=rand()%MAX_SUBSTRING_LEN;

                for(j=0;j<addlen;j++)
                {
                    thissubstring[j] = TOPIC_STRING_CHARS[rand()%strlen(TOPIC_STRING_CHARS)];
                }
            }

            thissubstring[j] = '\0';

            strcat(topicString, thissubstring);

            // For some topic strings, check the validity functions for both subscribe
            // and publish. Note: i+1 is the number of substrings in this topic.
            if ((i+1)<(iettMAX_TOPIC_DEPTH*2))
            {
                bool validForSubscribe = iett_validateTopicString(pThreadData, topicString, iettVALIDATE_FOR_SUBSCRIBE);
                bool validForPublish = iett_validateTopicString(pThreadData, topicString, iettVALIDATE_FOR_PUBLISH);

                if ((i+1)>iettMAX_TOPIC_DEPTH)
                {
                    TEST_ASSERT_EQUAL(validForSubscribe, false);
                    TEST_ASSERT_EQUAL(validForPublish, false);
                }
                else if (w > 0 || m > 0)
                {
                    TEST_ASSERT_EQUAL(validForSubscribe, true);
                    TEST_ASSERT_EQUAL(validForPublish, false);
                }
                else
                {
                    TEST_ASSERT_EQUAL(validForSubscribe, true);
                    TEST_ASSERT_EQUAL(validForPublish, true);
                }
            }

            stringHash = iett_generateTopicStringHash(topicString);

            topic.destinationType = ismDESTINATION_TOPIC;
            topic.topicString = topicString;

            rc = iett_analyseTopicString(pThreadData, &topic);

            if (i>=iettMAX_TOPIC_DEPTH)
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);
            }
            else
            {
                test_checkConsistentTopicAnalysis(pThreadData, &topic);
                TEST_ASSERT_EQUAL(rc, OK);
                TEST_ASSERT_EQUAL(topic.substringHashes[i], iett_generateSubstringHash(thissubstring));
            }

            TEST_ASSERT_EQUAL(topic.topicStringLength, strlen(topicString));
            TEST_ASSERT_PTR_NOT_NULL(topic.topicStringCopy);
            TEST_ASSERT_EQUAL(topic.substringCount, i+1);
            TEST_ASSERT_EQUAL(strlen(topic.substrings[i]), strlen(thissubstring));
            TEST_ASSERT_EQUAL(strncmp(topic.substrings[i], thissubstring, strlen(thissubstring)), 0);
            TEST_ASSERT_EQUAL(topic.wildcardCount, w);
            TEST_ASSERT_EQUAL(topic.multicardCount, m);
            TEST_ASSERT_PTR_NULL(topic.substrings[topic.substringCount]);
            TEST_ASSERT_EQUAL(topic.sysTopicEndIndex, 0); // $SYS cannot appear at start ($ not in set!)

            if (w != 0)
            {
                TEST_ASSERT_PTR_NOT_NULL(topic.wildcards);
                for(k=0;k<w;k++)
                {
                    TEST_ASSERT_EQUAL(*(topic.wildcards[k]), '+');
                }
            }
            else
            {
                TEST_ASSERT_EQUAL_FORMAT(topic.wildcards, wildcards, "%p");
            }

            if (m != 0)
            {
                TEST_ASSERT_PTR_NOT_NULL(topic.multicards);
                for(k=0;k<m;k++)
                {
                    TEST_ASSERT_EQUAL(*(topic.multicards[k]), '#');
                }
            }
            else
            {
                TEST_ASSERT_EQUAL_FORMAT(topic.multicards, multicards, "%p");
            }
            TEST_ASSERT_PTR_NULL(topic.substrings[i+1]);

            bucket10[stringHash%10] += 1;
            bucket32[stringHash%32] += 1;
            bucket100[stringHash%100] += 1;
            bucket1000[stringHash%1000] += 1;
            bucket10000[stringHash%10000] += 1;

            iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

            if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
            if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
            if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
            if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);

            strcat(topicString,"/");
        }
    }
    while(++iteration < TOTAL_ITERATIONS);

#if REPORT_ON_BUCKETS
    uint32_t low10 = 0xffffffff;
    uint32_t high10 = 0;
    uint32_t low32 = 0xffffffff;
    uint32_t high32 = 0;
    uint32_t low100 = 0xffffffff;
    uint32_t high100 = 0;
    uint32_t low1000 = 0xffffffff;
    uint32_t high1000 = 0;
    uint32_t low10000 = 0xffffffff;
    uint32_t high10000 = 0;

    printf("Over %d iterations of %d\n", TOTAL_ITERATIONS, MAX_SUBSTRINGS);
    for (i=0;i<10000;i++)
    {
        if (i<10)
        {
            if (i<REPORT_ON_BUCKETS) printf("BUCKET10 %03d: %5d ", i, bucket10[i]);
            if (bucket10[i] < low10)
                low10=bucket10[i];
            if (bucket10[i] > high10)
                high10=bucket10[i];
        }
        else if (i<REPORT_ON_BUCKETS) printf("                    ");

        if (i<32)
        {
            if (i<REPORT_ON_BUCKETS) printf("BUCKET32 %03d: %5d ", i, bucket32[i]);
            if (bucket32[i] < low32)
                low32=bucket32[i];
            if (bucket32[i] > high32)
                high32=bucket32[i];
        }
        else if (i<REPORT_ON_BUCKETS) printf("                    ");

        if (i<100)
        {
            if (i<REPORT_ON_BUCKETS) printf("BUCKET100 %03d: %4d ", i, bucket100[i]);
            if (bucket100[i] < low100)
                low100=bucket100[i];
            if (bucket100[i] > high100)
                high100=bucket100[i];
        }
        else if (i<REPORT_ON_BUCKETS) printf("                    ");

        if (i<1000)
        {
            if (i<REPORT_ON_BUCKETS) printf("BUCKET1000 %03d: %2d ", i, bucket1000[i]);
            if (bucket1000[i] < low1000)
                low1000=bucket1000[i];
            if (bucket1000[i] > high1000)
                high1000=bucket1000[i];
        }
        else if (i<REPORT_ON_BUCKETS) printf("                     ");

        if (i<10000)
        {
            if (i<REPORT_ON_BUCKETS) printf("BUCKET10000 %03d: %2d ", i, bucket10000[i]);
            if (bucket10000[i] < low10000)
                low10000=bucket10000[i];
            if (bucket10000[i] > high10000)
                high10000=bucket10000[i];
        }
        else if (i<REPORT_ON_BUCKETS) printf("                      ");

        if (i<=REPORT_ON_BUCKETS) printf("\n");
    }

    printf("Bucket10:    %u<-XX->%u [P:%u]\n",
           low10, high10,
           (uint32_t)(TOTAL_ITERATIONS*MAX_SUBSTRINGS/(sizeof(bucket10)/sizeof(int))));
    printf("Bucket32:    %u<-XX->%u [P:%u]\n",
           low32, high32,
           (uint32_t)(TOTAL_ITERATIONS*MAX_SUBSTRINGS/(sizeof(bucket32)/sizeof(int))));
    printf("Bucket100:   %u<-XX->%u [P:%u]\n",
           low100, high100,
           (uint32_t)(TOTAL_ITERATIONS*MAX_SUBSTRINGS/(sizeof(bucket100)/sizeof(int))));
    printf("Bucket1000:  %u<-XX->%u [P:%u]\n",
           low1000, high1000,
           (uint32_t)(TOTAL_ITERATIONS*MAX_SUBSTRINGS/(sizeof(bucket1000)/sizeof(int))));
    printf("Bucket10000: %u<-XX->%u [P:%u]\n",
           low10000, high10000,
           (uint32_t)(TOTAL_ITERATIONS*MAX_SUBSTRINGS/(sizeof(bucket10000)/sizeof(int))));
#endif

    free(topicString);
}

void test_defect_52608(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    bool valid;

    static char *LONGSTRING = "TOOMANYLEVELS/HCT_66a77addZ2a5bZ4f27Z/HCT_8f180603Za893Z4b1eZ/"
                              "HCT_243cd43eZb890Z45b5Z/HCT_d3b1c041Z9f42Z4057Z/HCT_bea5bff4Z438fZ4c98Z/"
                              "HCT_3d2b8201Z163fZ4048Z/HCT_43cc164eZ962bZ4630Z/HCT_777ffc9dZcc38Z429eZ/"
                              "HCT_a7b4fce6Z14a9Z43e1Z/HCT_47b1eb9fZac92Z43b0Z/HCT_c19f811dZ1c31Z46baZ/"
                              "HCT_dfc52abfZ30cfZ4062Z/HCT_9c7af0c9Z4ec1Z40b0Z/HCT_77046ff8Z0944Z43c1Z/"
                              "HCT_49df89f8Zcc37Z47dcZ/HCT_0ad7aee6Z2ff5Z40deZ/HCT_230ce674Zfa6aZ48a1Z/"
                              "HCT_705115b5Zf97dZ4854Z/HCT_f7504e46Z1589Z45d3Z/HCT_6680a589Zdd26Z488cZ/"
                              "HCT_0f540a03Z3910Z47a3Z/HCT_3a26aeb6ZedaaZ40ccZ/HCT_fab145c2Z4d74Z4d7fZ/"
                              "HCT_7e6e678bZb737Z40f0Z/HCT_121cf6c3Z9916Z4411Z/HCT_f0bab42fZ4b17Z4aedZ/"
                              "HCT_26e8b87dZ0714Z4487Z/HCT_6e4b2642Z7df3Z4257Z/HCT_b54d1beeZac37Z4a34Z/"
                              "HCT_bb06de94Z7bb5Z41ccZ/HCT_dd45a785Z8b2eZ4223Z/HCT_8a0e8109Z469cZ4800Z/"
                              "HCT_47d9fd47Z6c3cZ4ad7Z/HCT_7e4973c1Za254Z446bZ/HCT_6bf03962Ze0e0Z4ba7Z/"
                              "HCT_21b01e02Zd0c1Z43c2Z/HCT_6fa68105Zd0dcZ401dZ/HCT_a8c13bd2Z8120Z44baZ/"
                              "HCT_e4dba0d3Ze33cZ482aZ/HCT_6746122dZf966Z4e55Z/HCT_9df1ae44Zb135Z4cd0Z/"
                              "HCT_d4d9c76eZ3415Z4906Z/HCT_f22d3ed4Z1dcdZ4801Z/HCT_5eaa9023Z9ab8Z4c78Z/"
                              "HCT_41c02431Zd617Z41f1Z/HCT_a369be5dZ798fZ4e86Z/HCT_a09e9cccZ2529Z4454Z/"
                              "HCT_94079659Za87bZ4fd5Z/HCT_cb750820Z7d1bZ4be2Z/HCT_1755bcf5Z5a5dZ4731Z/"
                              "HCT_700dcff6Zb57aZ48ffZ/HCT_95c52997Zfb3cZ45f9Z/HCT_7bf6c90aZb738Z4714Z/"
                              "HCT_5389c253Z884aZ47fbZ";

    // Try a simple validateTopicString call
    valid = iett_validateTopicString(pThreadData, LONGSTRING, iettVALIDATE_FOR_PUBLISH);
    TEST_ASSERT_EQUAL(valid, false);

    // Try the actual call that failed
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    int32_t rc = test_createClientAndSession(__func__,
                                             NULL,
                                             ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                             ismENGINE_CREATE_SESSION_OPTION_NONE,
                                             &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_MessageHandle_t hMessage;
    void *payload=NULL;

    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, LONGSTRING,
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);

    if (payload) free(payload);

    ismEngine_UnreleasedHandle_t hUnrel;

    rc = ism_engine_putMessageWithDeliveryIdOnDestination( hSession
                                                         , ismDESTINATION_TOPIC
                                                         , LONGSTRING
                                                         , NULL
                                                         , hMessage
                                                         , 1
                                                         , &hUnrel
                                                         , NULL
                                                         , 0
                                                         , NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_TopicTree_CUnit_test_func_iett_analyseTopicString[] =
{
    { "iett_analyseTopicString", test_func_iett_analyseTopicString },
    { "Defect_52608", test_defect_52608 },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_func_iett_insertOrFindSubsNode               */
/*                                                                   */
/* Description:    Test the iett_insertOrFindSubsNode function       */
/*                                                                   */
/*********************************************************************/
#define SUBSNODE_ACCESS_TEST_TOPIC "Subs Node/+/Access/Test/+//Topic/#"

void test_func_iett_insertOrFindSubsNode(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t   rc = OK;

    iettTopic_t  topic;
    const char  *substrings[10];
    uint32_t     substringHashes[10];
    const char  *wildcards[10];
    const char  *multicards[10];

    printf("Starting %s...\n", __func__);

    iettTopicTree_t *tree;

    iettSubsNode_t *node;

    /*****************************************************************/
    /* Create a topic tree                                           */
    /*****************************************************************/
    tree = iett_createTopicTree(pThreadData);

    TEST_ASSERT_PTR_NOT_NULL(tree);
    TEST_ASSERT_EQUAL(tree->subs->nodeFlags, iettNODE_FLAG_TREE_ROOT);

    memset(&topic, 0, sizeof(topic));
    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = SUBSNODE_ACCESS_TEST_TOPIC;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 10;

    /*****************************************************************/
    /* Need to get an analysis of the topic string to use when       */
    /* calling functions using a topic string on the topic tree.     */
    /*****************************************************************/
    rc = iett_analyseTopicString(pThreadData, &topic);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(topic.topicStringCopy);
    TEST_ASSERT_EQUAL(topic.substringCount, 8);
    TEST_ASSERT_EQUAL(topic.wildcardCount, 2);
    TEST_ASSERT_EQUAL(topic.multicardCount, 1);
    TEST_ASSERT_EQUAL(topic.topicStringLength, strlen(SUBSNODE_ACCESS_TEST_TOPIC));
    TEST_ASSERT_EQUAL_FORMAT(topic.substrings, substrings, "%p");
    TEST_ASSERT_EQUAL_FORMAT(topic.substringHashes, substringHashes, "%p");
    TEST_ASSERT_EQUAL_FORMAT(topic.wildcards, wildcards, "%p");
    TEST_ASSERT_EQUAL_FORMAT(topic.multicards, multicards, "%p");

    /*****************************************************************/
    /* Try and find the node - it should not exist yet.              */
    /*****************************************************************/
    node = NULL;
    rc = iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_FIND, &node);

    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(node);

    /*****************************************************************/
    /* Add the node into the tree                                    */
    /*****************************************************************/
    rc = iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_ADD, &node);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(node);
    TEST_ASSERT_EQUAL(node->nodeFlags & iettNODE_FLAG_TYPE_MASK, iettNODE_FLAG_MULTICARD);

    /*****************************************************************/
    /* Find the node that should now exist.                          */
    /*****************************************************************/
    rc = iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_FIND, &node);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(node);
    TEST_ASSERT_EQUAL(node->nodeFlags & iettNODE_FLAG_TYPE_MASK, iettNODE_FLAG_MULTICARD);

    /*****************************************************************/
    /* Find a node in the middle of the tree.                        */
    /*****************************************************************/
    iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

    memset(&topic, 0, sizeof(topic));
    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = "Subs Node/+/Access";
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 10;

    rc = iett_analyseTopicString(pThreadData, &topic);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(topic.topicStringCopy);
    TEST_ASSERT_EQUAL(topic.substringCount, 3);
    TEST_ASSERT_EQUAL(topic.wildcardCount, 1);
    TEST_ASSERT_EQUAL(topic.multicardCount, 0);
    TEST_ASSERT_EQUAL_FORMAT(topic.substrings, substrings, "%p");
    TEST_ASSERT_EQUAL_FORMAT(topic.substringHashes, substringHashes, "%p");
    TEST_ASSERT_EQUAL_FORMAT(topic.wildcards, wildcards, "%p");
    TEST_ASSERT_EQUAL_FORMAT(topic.multicards, multicards, "%p");

    rc = iett_insertOrFindSubsNode(pThreadData, tree->subs, &topic, iettOP_FIND, &node);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(node);
    TEST_ASSERT_EQUAL(node->nodeFlags & iettNODE_FLAG_TYPE_MASK, iettNODE_FLAG_NORMAL);

    /*****************************************************************/
    /* Destroy the topic tree                                        */
    /*****************************************************************/
    iett_destroyTopicTree(pThreadData, tree);

    iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);
}

CU_TestInfo ISM_TopicTree_CUnit_test_func_iett_insertOrFindSubsNode[] =
{
    { "iett_insertOrFindSubsNode", test_func_iett_insertOrFindSubsNode },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  test_capability_SubscriptionMatching              */
/*                                                                   */
/* Description:    Test the correctness of subscription matching     */
/*                                                                   */
/*********************************************************************/
void testSubscriptionMatching(char  **subscribeTopics,
                              char  **publishTopics,
                              int    *pubSubFactors,
                              int32_t deliveryOption,
                              bool    verbose)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t                rc;

    ismEngine_Consumer_t **consumers = NULL;
    int32_t               *messageCounts = NULL;

    char                 **currentTopicString;
    ismEngine_Consumer_t **currentConsumer;
    int32_t               *currentMessageCount;
    int                   *currentPubSubFactor;

    iettSubscriberList_t   returnSubList = {0};

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    consumers = calloc(1, sizeof(ismEngine_Consumer_t*) * 50); /* Max 50 Topics */
    messageCounts = calloc(1, sizeof(int32_t)*50);

    TEST_ASSERT_PTR_NOT_NULL(consumers);
    TEST_ASSERT_PTR_NOT_NULL(messageCounts);

    rc = test_createClientAndSession(__func__,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    /* Register Subscribers */
    currentTopicString = subscribeTopics;
    currentConsumer = consumers;
    currentMessageCount = messageCounts;
    while(*currentTopicString)
    {
        ismEngine_SubscriptionAttributes_t subAttrs = {0};

        if (deliveryOption == -1) subAttrs.subOptions = SUB_DELIVERY_OPTIONS[rand()%SUB_DELIVERY_NUMOPTIONS];
        else subAttrs.subOptions = SUB_DELIVERY_OPTIONS[deliveryOption];

        // This will result in an ism_engine_registerSubscriber call
        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       *currentTopicString,
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       NULL,
                                       0,
                                       NULL, /* No delivery callback */
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       currentConsumer,
                                       NULL,
                                       0,
                                       NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_Subscription_t *underlyingSub = NULL;
        ismQHandle_t queueHandle = (*currentConsumer)->queueHandle;

        rc = iett_findQHandleSubscription(pThreadData, queueHandle, &underlyingSub);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(underlyingSub);
        TEST_ASSERT_EQUAL(underlyingSub->queueHandle, queueHandle);
        iett_releaseSubscription(pThreadData, underlyingSub, false);

        rc = iett_findQHandleSubscription(pThreadData, queueHandle, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        // Should be no messages available
        rc = ism_engine_checkAvailableMessages(*currentConsumer);
        TEST_ASSERT_EQUAL(rc, ISMRC_NoMsgAvail);

        currentTopicString += 1;
        currentConsumer += 1;
        currentMessageCount += 1;
    }
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.totalSubsCount, currentTopicString-subscribeTopics);
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.maintree->subs->totalSubsCount, ismEngine_serverGlobal.totalSubsCount);

    test_dumpTopicTree(*subscribeTopics);

    /* Test Topics Twice (second time to use cache if there is one) */
    for(int32_t loop=0; loop<2; loop++)
    {
        currentTopicString = publishTopics;
        currentPubSubFactor = pubSubFactors;
        while(*currentTopicString)
        {
            int subscriberCount = 0;

            memset(&returnSubList, 0, sizeof(returnSubList));
            returnSubList.topicString = *currentTopicString;

            rc = iett_getSubscriberList(pThreadData, &returnSubList);

            if (*currentPubSubFactor == 0)
            {
                TEST_ASSERT((rc == ISMRC_NotFound), ("rc (%d) != ISMRC_NotFound Topic: '%s'\n",
                                                     rc, *currentTopicString));
            }
            else
            {
                TEST_ASSERT((rc == OK), ("rc (%d) != OK Topic: '%s' Expected: %d hits\n",
                                         rc, *currentTopicString, *currentPubSubFactor));
            }

            if (rc == OK)
            {
                if (returnSubList.subscriberCount != 0)
                {
                    ismEngine_Subscription_t *tmpSubscriber;

                    while((tmpSubscriber = returnSubList.subscribers[subscriberCount]))
                    {
                        subscriberCount++;
                        if (verbose)
                        {
                            printf("Topic: '%s' Subscriber %d: '%s'\n",
                                   *currentTopicString, subscriberCount,
                                   tmpSubscriber->node->topicString);
                        }
                    }
                }

                if (subscriberCount != *currentPubSubFactor)
                {
                    printf("%d != %d (%s) [%d]\n",
                           subscriberCount, *currentPubSubFactor, *currentTopicString, returnSubList.subscriberCount);
                }

                iett_releaseSubscriberList(pThreadData, &returnSubList);
            }

            TEST_ASSERT_EQUAL(subscriberCount, *currentPubSubFactor);

            currentTopicString += 1;
            currentPubSubFactor += 1;
        }
    }

    test_dumpTopicTree(NULL);

    test_publishMessages(*publishTopics, 1, 0, 0);

    test_dumpTopicTree(NULL);

    currentConsumer = consumers;
    while(*currentConsumer)
    {
        rc = ism_engine_checkAvailableMessages(*currentConsumer); // Just drive this code
        TEST_ASSERT_CUNIT(rc == OK || rc == ISMRC_NoMsgAvail,
                                         ("rc was %d\n", rc));

        rc = ism_engine_destroyConsumer(*currentConsumer,
                                        NULL,
                                        0,
                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        currentConsumer += 1;
    }
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.totalSubsCount, 0);
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.maintree->subs->totalSubsCount, 0);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    free(consumers);
    free(messageCounts);
}

void test_capability_SubscriptionMatching(void)
{
    printf("Starting %s...\n", __func__);

    char *subTop1[] = {"+/+", "/+", "+", "finance", "/finance/+", NULL};
    char *pubTop1[] = {"/finance", "a/b/c", "/finance/", NULL};
    int   pubSubFactors1[] = {2, 0, 1, 0};

    testSubscriptionMatching(subTop1, pubTop1, pubSubFactors1, -1, false);

    char *subTop2[] = {"///", "//", "/", "#", "/#", NULL};
    char *pubTop2[] = {"/finance", "/", "//", "///", NULL};
    int   pubSubFactors2[] = {2, 3, 3, 3 ,0};

    testSubscriptionMatching(subTop2, pubTop2, pubSubFactors2, -1, false);

    char *subTop3[] = {"#/Giants", "+/+/Giants", "Sports/Football/Giants",
                       "/#/Giants", "#/Football/#", "/test/testMultiTopic/#/level3/#", NULL};
    char *pubTop3[] = {"/Sports/Football/Giants", "/Sports/Football/Football/Giants",
                       "Sports/Football/Giants", "/test/testMultiTopic/level1/level2/level3/level4/level5",
                       NULL};
    int   pubSubFactors3[] = {3, 3, 4, 1, 0};

    testSubscriptionMatching(subTop3, pubTop3, pubSubFactors3, 3, false);

    char *subTop4[] = {"/#/#", "/#", "#", "/A/#", "#/#", NULL};
    char *pubTop4[] = {"A", "/A", NULL};
    int   pubSubFactors4[] = {2, 5, 0};

    testSubscriptionMatching(subTop4, pubTop4, pubSubFactors4, 3, false);

    char *subTop5[] = {"/A/#/#", "Z/#/K/#", NULL};
    char *pubTop5[] = {"/A/B/C/D", "Z/K", "Z/A/K", "Z/A/K/U", NULL};
    int   pubSubFactors5[] = {1, 1, 1, 1, 0};

    testSubscriptionMatching(subTop5, pubTop5, pubSubFactors5, 3, false);

    char *subTop6[] = {"/A/+/#", "/A/#/+", "/A/+/#/D", "/A/#/+/D", "/A/+/+/D", "/A/+/+/D/#",
                       "/#/C/#", NULL};
    char *pubTop6[] = {"/A", "/A/B", "/A/B/C", "/A/B/C/D", NULL};
    int   pubSubFactors6[] = {0, 2, 3, 7, 0};

    testSubscriptionMatching(subTop6, pubTop6, pubSubFactors6, 3, false);

    char *subTop7[] = {"/TOPIC/#/WHATEVER", NULL};
    char *pubTop7[] = {"/NOMATCH/WHATEVER", "/TOPIC/WHATEVER", "TOPIC/WHATEVER", "/TOPIC/A/WHATEVER",
                       "/TOPIC/A/B/C/WHATEVER", "/TOPIC/WHATEVER/BLAH", "/TOPIC/WHATEVER/BLAH/WHATEVER", NULL};
    int   pubSubFactors7[] = {0, 1, 0, 1, 1, 0, 1, 0};

    testSubscriptionMatching(subTop7, pubTop7, pubSubFactors7, 3, false);

    // Check $SYS topic matching (PUB on $SYS*/ should not match outside $SYS tree)
    char *subTop8[] = {"$SYS/<NodeName>/#", "$SYS/#", "$SYS/<NodeName>/<Connection>/#", "#",
                       "+/<NodeName>/#", "+/#", "$SYSOTHER/#", "$SYSOTHER/ANOTHER/+",
                       "+/ANOTHER", "#/$SYS/#", "$SIS", "$sys/<NodeName>", "$syStOlIc/bLOOd/Pre55ure", NULL};
    char *pubTop8[] = {"$SYS", "$SYS/<NodeName>", "$SYS/<NodeName>/<Connection>/CONN",
                       "CUSTOMER/<NodeName>", "$SYSNOT/<NodeName>", "$SYSOTHER/ANOTHER",
                       "A/$SYS/B", "TEST/$SYSTEM", "$sys/<NodeName>", "$SYSTOLIC/BLOOD/PRESSURE", NULL};
    int   pubSubFactors8[] = {1, 2, 3, 3, 0, 1, 3, 2, 1, 0, 0};

    testSubscriptionMatching(subTop8, pubTop8, pubSubFactors8, 3, false);

    // Verify OASIS Issues Tracker issue MQTT-108 variations
    char *subTop9[] = {"sport/+", "sport/#", "+/", NULL};
    char *pubTop9[] = {"sport", "sport/", NULL};
    int   pubSubFactors9[] = {1, 3, 0};

    testSubscriptionMatching(subTop9, pubTop9, pubSubFactors9, -1, false);

    // Go through the code which clears up the arrays when low on memory
    iettSubscriberList_t returnSubList = {0};
    ieutThreadData_t *pThreadData = ieut_getThreadData();

    returnSubList.topicString = "sport";

    iett_getSubscriberList(pThreadData, &returnSubList);

    // Don't do the releasing...
    pThreadData->publishDepth = 2;
    iett_updateCachedArrays(pThreadData, &returnSubList, ISMRC_AllocateError);

    // Do the releasing!
    pThreadData->publishDepth = 1;
    iett_updateCachedArrays(pThreadData, &returnSubList, ISMRC_AllocateError);

    // Stop cheating!
    pThreadData->publishDepth = 0;

    iett_releaseSubscriberList(pThreadData, &returnSubList);
}

void test_forWildcards(ismEngine_SessionHandle_t hSession,
                       char *string,
                       int32_t expectMulticard,
                       int32_t expectWildcard,
                       uint32_t subOptions,
                       genericMsgCbContext_t *controlContext)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    iettTopic_t  topic = {0};
    const char  *substrings[10];
    uint32_t     substringHashes[10];
    const char  *wildcards[10];
    const char  *multicards[10];

    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = string;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 10;

    if (iett_validateTopicString(pThreadData, string, iettVALIDATE_FOR_PUBLISH))
    {
        TEST_ASSERT_EQUAL(expectMulticard+expectWildcard, 0);
    }
    else
    {
        TEST_ASSERT_NOT_EQUAL(expectMulticard+expectWildcard, 0);
    }

    rc = iett_analyseTopicString(pThreadData, &topic);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(topic.multicardCount, expectMulticard);
    TEST_ASSERT_EQUAL(topic.wildcardCount, expectWildcard);

    ismEngine_MessageHandle_t hMessage;
    void *payload=NULL;
    ismEngine_SubscriptionAttributes_t subAttrs = { subOptions };

    if (hSession != NULL)
    {
        ismEngine_Consumer_t *hConsumer = NULL;
        ismEngine_Producer_t *hProducer = NULL;
        int32_t              messageCount;

        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       string,
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &messageCount,
                                       sizeof(int32_t),
                                       NULL, /* No delivery callback */
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hConsumer,
                                       NULL,
                                       0,
                                       NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer);

        rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, string,
                                &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                string,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);

        if (payload) free(payload);

        // Publish does not allow wildcards in the topic string
        if (expectMulticard+expectWildcard > 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);

            __sync_fetch_and_add(&controlContext->expected, 1);
        }

        rc = ism_engine_createProducer(hSession,
                                       ismDESTINATION_TOPIC,
                                       string,
                                       &hProducer,
                                       NULL, 0, NULL);

        // Producers do not allow wildcards in the topic string
        if (expectMulticard+expectWildcard > 0)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);
            TEST_ASSERT_PTR_NULL(hProducer);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hProducer);

            payload = NULL;

            rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                    ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                    ismMESSAGE_FLAGS_NONE,
                                    0,
                                    ismDESTINATION_TOPIC, string,
                                    &hMessage, &payload);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_putMessage(hSession,
                                       hProducer,
                                       NULL,
                                       hMessage,
                                       NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            if (payload) free(payload);

            __sync_fetch_and_add(&controlContext->expected, 1);

            rc = ism_engine_destroyProducer(hProducer, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

    if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
    if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
    if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
    if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);
}

#define DEFECT_X_TOPICSTRING "a991111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a991111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44bba991111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44bba991111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44bba991111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44a1111111111a2222222222a3333333333a4444444444a5555555555a6666666666a7777777777a8888888888a9999999999a0000000000b1111111111b2222222222b3333333333b4444444444b5555555555b6666666666b7777777777b8888888888b9999999999b0000000000c1111111111c2222222222c3333333333c44bb/#"

void test_capability_Wildcards(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession(__func__,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    genericMsgCbContext_t controlContext = {0};
    genericMsgCbContext_t *controlCb = &controlContext;
    ismEngine_ConsumerHandle_t hControlConsumer = NULL;

    controlContext.hSession = hSession;

    // Create a control consumer to pick up any messages that are published
    // whilst testing for wildcards - we will check that the right number were
    // received at the end of our wildcard tests
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &controlCb,
                                   sizeof(genericMsgCbContext_t *),
                                   genericMessageCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hControlConsumer,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hControlConsumer);

    test_forWildcards(hSession, "#",             1, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, &controlContext);
    test_forWildcards(hSession, "+",             0, 1, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, &controlContext);
    test_forWildcards(hSession, "/#",            1, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, &controlContext);
    test_forWildcards(hSession, "/+",            0, 1, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, &controlContext);
    test_forWildcards(hSession, "##",            0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, &controlContext);
    test_forWildcards(hSession, "++",            0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, &controlContext);
    test_forWildcards(hSession, "#/#",           2, 0, ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE, &controlContext);
    test_forWildcards(hSession, "+/+",           0, 2, ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE, &controlContext);
    test_forWildcards(hSession, "topic#",        0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, &controlContext);
    test_forWildcards(hSession, "topic+",        0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, &controlContext);
    test_forWildcards(hSession, "topic/#",       1, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, &controlContext);
    test_forWildcards(hSession, "topic/+",       0, 1, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, &controlContext);
    test_forWildcards(hSession, "topic##",       0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, &controlContext);
    test_forWildcards(hSession, "topic++",       0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, &controlContext);
    test_forWildcards(hSession, "topic/##",      0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, &controlContext);
    test_forWildcards(hSession, "topic/++",      0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, &controlContext);
    test_forWildcards(hSession, "topic#/#",      1, 0, ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE, &controlContext);
    test_forWildcards(hSession, "topic+/+",      0, 1, ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE, &controlContext);
    test_forWildcards(hSession, "topic/#/#",     2, 0, ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE, &controlContext);
    test_forWildcards(hSession, "topic/+/+",     0, 2, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, &controlContext);
    test_forWildcards(hSession, "#topic",        0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, &controlContext);
    test_forWildcards(hSession, "+topic",        0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, &controlContext);
    test_forWildcards(hSession, "#/topic",       1, 0, ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE, &controlContext);
    test_forWildcards(hSession, "+/topic",       0, 1, ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, &controlContext);
    test_forWildcards(hSession, "/#topic",       0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, &controlContext);
    test_forWildcards(hSession, "/+topic",       0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, &controlContext);
    test_forWildcards(hSession, "/",             0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE, &controlContext);
    test_forWildcards(hSession, "topic/#/topic", 1, 0, ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE, &controlContext);
    test_forWildcards(hSession, "top+ic",        0, 0, ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE, &controlContext);

    // Check that the right number were received
    TEST_ASSERT_EQUAL(controlContext.received, controlContext.expected);

    rc = ism_engine_destroyConsumer(hControlConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check a few topic strings for topic monitors
    bool bResult = iett_validateTopicString(pThreadData, "topic/+", iettVALIDATE_FOR_TOPICMONITOR);
    TEST_ASSERT_EQUAL(bResult, false);
    bResult = iett_validateTopicString(pThreadData, "", iettVALIDATE_FOR_TOPICMONITOR);
    TEST_ASSERT_EQUAL(bResult, false);
    bResult = iett_validateTopicString(pThreadData, "#", iettVALIDATE_FOR_TOPICMONITOR);
    TEST_ASSERT_EQUAL(bResult, true);
    bResult = iett_validateTopicString(pThreadData, "A/#", iettVALIDATE_FOR_TOPICMONITOR);
    TEST_ASSERT_EQUAL(bResult, true);
    bResult = iett_validateTopicString(pThreadData, "a/+/b/#", iettVALIDATE_FOR_TOPICMONITOR);
    TEST_ASSERT_EQUAL(bResult, false);
    bResult = iett_validateTopicString(pThreadData, "a/#/b/#", iettVALIDATE_FOR_TOPICMONITOR);
    TEST_ASSERT_EQUAL(bResult, false);
    bResult = iett_validateTopicString(pThreadData, "a/b/c/d/e/f/g/#", iettVALIDATE_FOR_TOPICMONITOR);
    TEST_ASSERT_EQUAL(bResult, true);
    bResult = iett_validateTopicString(pThreadData, "0/1/2/3/4/5/6/7/8/9/0/1/2/3/4/5/6/7/8/9/0/1/2/3/4/5/6/7/8/9/1/2/#",
                                       iettVALIDATE_FOR_TOPICMONITOR);
    TEST_ASSERT_EQUAL(bResult, false);
    bResult = iett_validateTopicString(pThreadData, "/", iettVALIDATE_FOR_TOPICMONITOR);
    TEST_ASSERT_EQUAL(bResult, false);
    bResult = iett_validateTopicString(pThreadData, "A/B", iettVALIDATE_FOR_TOPICMONITOR);
    TEST_ASSERT_EQUAL(bResult, false);
    bResult = iett_validateTopicString(pThreadData, DEFECT_X_TOPICSTRING, iettVALIDATE_FOR_TOPICMONITOR);
    TEST_ASSERT_EQUAL(bResult, true);
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_TopicMatching[] =
{
    { "SubscriptionMatching", test_capability_SubscriptionMatching },
    { "Wildcards", test_capability_Wildcards },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the capabilities of the topic tree hash tables.
//****************************************************************************
typedef struct tag_traversalCbContext_t
{
    int32_t called;
} traversalCbContext_t;

void test_hashTableTraversal(ieutThreadData_t *pThreadData,
                             char *key,
                             uint32_t keyHash,
                             void *value,
                             void *pContext)
{
    traversalCbContext_t *context = (traversalCbContext_t *)pContext;
    TEST_ASSERT_PTR_NOT_NULL(pThreadData);
    (context->called)++;
    return;
}

#define MAX_KEYS 100000
void test_capability_HashTables(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc,i;

    ieutHashTable_t *table;

    printf("Starting %s...\n", __func__);

    printf("  ...basics\n");

    rc = ieut_createHashTable(pThreadData, MAX_KEYS/1000, iemem_subsTree, &table);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(table);
    TEST_ASSERT_EQUAL(table->capacity, MAX_KEYS/1000);
    TEST_ASSERT_PTR_NOT_NULL(table->chains);
    TEST_ASSERT_EQUAL(table->totalCount, 0);
    for(i=0;i<table->capacity;i++)
    {
        TEST_ASSERT_PTR_NULL(table->chains[i].entries);
    }

    ieut_traverseHashTable(pThreadData, table, &test_hashTableTraversal, NULL);
    ieut_clearHashTable(pThreadData, table);

    TEST_ASSERT_EQUAL(table->capacity, MAX_KEYS/1000);
    TEST_ASSERT_PTR_NOT_NULL(table->chains);
    TEST_ASSERT_EQUAL(table->totalCount, 0);
    for(i=0;i<table->capacity;i++)
    {
        TEST_ASSERT_PTR_NULL(table->chains[i].entries);
    }

    TEST_ASSERT_EQUAL(table->chains[1].size, 0);

    rc = ieut_putHashEntry(pThreadData, table, ieutFLAG_DUPLICATE_ALL, "test", 1, "value", strlen("value")+1);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(table->totalCount, 1);
    TEST_ASSERT_EQUAL(table->chains[1].count, 1);
    TEST_ASSERT_NOT_EQUAL(table->chains[1].size, 0);

    ieut_removeHashEntry(pThreadData, table, "test", 1);

    TEST_ASSERT_EQUAL(table->totalCount, 0);
    TEST_ASSERT_EQUAL(table->chains[1].count, 0);
    TEST_ASSERT_NOT_EQUAL(table->chains[1].size, 0);

    rc = ieut_putHashEntry(pThreadData, table, ieutFLAG_DUPLICATE_ALL, "query", 5, "value", strlen("value")+1);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(table->totalCount, 1);
    TEST_ASSERT_EQUAL(table->chains[5].count, 1);
    TEST_ASSERT_PTR_NOT_NULL(table->chains[5].entries);
    TEST_ASSERT_NOT_EQUAL(table->chains[5].size, 0);

    ieut_clearHashTable(pThreadData, table);

    TEST_ASSERT_EQUAL(table->totalCount, 0);
    TEST_ASSERT_EQUAL(table->chains[5].count, 0);
    TEST_ASSERT_PTR_NULL(table->chains[5].entries);
    TEST_ASSERT_EQUAL(table->chains[5].size, 0);

    char     **keys = malloc(MAX_KEYS*sizeof(char*));
    uint32_t  *keyHashes = malloc(MAX_KEYS*sizeof(uint32_t));
    uint32_t   minKeyHash = 0xFFFFFFFF;
    uint32_t   maxKeyHash = 0;

    TEST_ASSERT_PTR_NOT_NULL(keys);
    TEST_ASSERT_PTR_NOT_NULL(keyHashes);

    char      *value;

    printf("  ...addition\n");
    for(i=0; i<MAX_KEYS; i++)
    {
        uint32_t length = rand()%20;

        keys[i] = calloc(1, length+8+1);

        TEST_ASSERT_PTR_NOT_NULL(keys[i]);

        uint32_t j;

        /* Ensure each key is unique - not interested in replace entries */
        sprintf(keys[i], "%08d", i);

        for (j=8; j<length; j++)
        {
            keys[i][j] = TOPIC_STRING_CHARS[rand()%strlen(TOPIC_STRING_CHARS)];
        }

        keyHashes[i] = iett_generateTopicStringHash(keys[i]);

        if (keyHashes[i] > maxKeyHash) maxKeyHash = keyHashes[i];
        if (keyHashes[i] < minKeyHash) minKeyHash = keyHashes[i];

        rc = ieut_putHashEntry(pThreadData, table, ieutFLAG_DUPLICATE_NONE, keys[i], keyHashes[i], keys[i], 0);

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(table->totalCount, i+1);
    }

    // Try getting & removing an entry below the valid hash range
    if (minKeyHash > 0)
    {
        value = (char *)0xdeadbeef900df00d;

        rc = ieut_getHashEntry(table, "BLAH", minKeyHash-1, (void **)&value);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        TEST_ASSERT_EQUAL(value, (char *)0xdeadbeef900df00d);

        ieut_removeHashEntry(pThreadData, table, "BLAH", minKeyHash-1);
        TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS);
    }

    // Try getting & removing an entry above the valid hash range
    if (maxKeyHash < 0xffffffff)
    {
        value = (char *)0xdeadbeef900df00d;

        rc = ieut_getHashEntry(table, "BLAH", maxKeyHash+1, (void **)&value);
        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        TEST_ASSERT_EQUAL(value, (char *)0xdeadbeef900df00d);

        ieut_removeHashEntry(pThreadData, table, "BLAH", maxKeyHash+1);
        TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS);
    }

    // Test traversal of the hash table
    traversalCbContext_t traversalContext = {0};
    ieut_traverseHashTable(pThreadData, table, &test_hashTableTraversal, &traversalContext);
    TEST_ASSERT_EQUAL(traversalContext.called, MAX_KEYS);

    printf("  ...access & resizing\n");
    int32_t iterations = 10;
    int32_t newCapacity;
    do
    {
        for(i=0; i<1000; i++)
        {
            uint32_t  key = rand()%MAX_KEYS;

            value = (char *)0xdeadbeef900df00d;

            rc = ieut_getHashEntry(table, keys[key], keyHashes[key], (void **)&value);

            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_EQUAL(value, keys[key]);
            TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS);
        }

        newCapacity = 20 + (rand()%MAX_KEYS);

        rc = ieut_resizeHashTable(pThreadData, table, newCapacity);

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS);
        TEST_ASSERT_EQUAL(table->capacity, newCapacity);
    }
    while(iterations--);

    printf("  ...removal\n");
    for(i=MAX_KEYS-1; i>MAX_KEYS/2; i--)
    {
        value = (char *)0xdeadbeef900df00d;

        ieut_removeHashEntry(pThreadData, table, keys[i], keyHashes[i]);

        TEST_ASSERT_EQUAL(table->totalCount, i);
        TEST_ASSERT_EQUAL(table->capacity, newCapacity);

        rc = ieut_getHashEntry(table, keys[i], keyHashes[i], (void **)&value);

        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        TEST_ASSERT_EQUAL_FORMAT(value, (void*)0xdeadbeef900df00d, "%p");
    }

    // Drive removal again, for a key we know should not exist
    ieut_removeHashEntry(pThreadData, table, keys[MAX_KEYS-2], keyHashes[MAX_KEYS-2]);

    printf("  ...downsizing\n");
    iterations = 3;
    do
    {
        newCapacity = (table->capacity)/2;
        TEST_ASSERT_NOT_EQUAL(newCapacity, 0);

        rc = ieut_resizeHashTable(pThreadData, table, newCapacity);

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(table->capacity, newCapacity);

        for(i=0; i<1000; i++)
        {
            uint32_t  key = rand()%MAX_KEYS;

            value = (char*)0xdeadbeef900df00d;

            rc = ieut_getHashEntry(table, keys[key], keyHashes[key], (void **)&value);

            if (key > MAX_KEYS/2)
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
                TEST_ASSERT_EQUAL_FORMAT(value, (void*)0xdeadbeef900df00d, "%p");
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, OK);
                TEST_ASSERT_EQUAL(value, keys[key]);
            }
        }
    }
    while(iterations--);

    printf("  ...cleanup\n");
    ieut_clearHashTable(pThreadData, table);

    TEST_ASSERT_EQUAL(table->totalCount, 0);

    for(i=0; i<5000; i++)
    {
        uint32_t  key = rand()%MAX_KEYS;

        value = (char*)0xdeadbeef900df00d;

        rc = ieut_getHashEntry(table, keys[key], keyHashes[key], (void **)&value);

        TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        TEST_ASSERT_EQUAL_FORMAT(value, (void*)0xdeadbeef900df00d, "%p");
    }

    printf("  ...repopulation (duplicating keys & values)\n");
    for(i=MAX_KEYS/4; i<(3*(MAX_KEYS/4)); i++)
    {
        value = (char*)0xdeadbeef900df00d;

        rc = ieut_putHashEntry(pThreadData, table, ieutFLAG_DUPLICATE_ALL, keys[i], keyHashes[i], keys[i], strlen(keys[i])+1);

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(table->totalCount, (i-(MAX_KEYS/4))+1);

        rc = ieut_getHashEntry(table, keys[i], keyHashes[i], (void**)&value);

        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_NOT_EQUAL_FORMAT(value, keys[i], "%p");
        TEST_ASSERT_STRINGS_EQUAL(keys[i], value);
    }

    rc = ieut_putHashEntry(pThreadData,
                           table,
                           ieutFLAG_DUPLICATE_KEY_STRING,
                           keys[MAX_KEYS/2],
                           keyHashes[MAX_KEYS/2],
                           keys[MAX_KEYS/2],
                           strlen(keys[MAX_KEYS/2])+1);

    TEST_ASSERT_EQUAL(rc, ISMRC_ExistingKey);

    value = (char*)0xdeadbeef900df00d;

    rc = ieut_getHashEntry(table, keys[MAX_KEYS/2], keyHashes[MAX_KEYS/2], (void**)&value);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_NOT_EQUAL_FORMAT(value, keys[MAX_KEYS/2],"%p");
    TEST_ASSERT_STRINGS_EQUAL(keys[MAX_KEYS/2], value);

    rc = ieut_putHashEntry(pThreadData,
                           table,
                           ieutFLAG_DUPLICATE_KEY_STRING | ieutFLAG_REPLACE_EXISTING,
                           keys[MAX_KEYS/2],
                           keyHashes[MAX_KEYS/2],
                           keys[MAX_KEYS/2],
                           strlen(keys[MAX_KEYS/2])+1);

    TEST_ASSERT_EQUAL(rc, OK);

    value = (char*)0xdeadbeef900df00d;

    rc = ieut_getHashEntry(table, keys[MAX_KEYS/2], keyHashes[MAX_KEYS/2], (void**)&value);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL_FORMAT(value, keys[MAX_KEYS/2], "%p");

    printf("  ...destruction\n");

    ieut_destroyHashTable(pThreadData, table);

    for(i=0; i<MAX_KEYS; i++)
    {
        free(keys[i]);
    }

    free(keys);
    free(keyHashes);
}

void test_capability_EmptyEntryRemoval(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc,i;

    ieutHashTable_t *table;

    printf("Starting %s...\n", __func__);

    rc = ieut_createHashTable(pThreadData, MAX_KEYS/20, iemem_subsTree, &table);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(table);
    TEST_ASSERT_EQUAL(table->capacity, MAX_KEYS/20);
    TEST_ASSERT_PTR_NOT_NULL(table->chains);
    TEST_ASSERT_EQUAL(table->totalCount, 0);

    ieut_removeEmptyHashEntries(pThreadData, table);
    TEST_ASSERT_EQUAL(table->totalCount, 0);

    char     **keys = malloc(MAX_KEYS*sizeof(char*));
    uint32_t  *keyHashes = malloc(MAX_KEYS*sizeof(uint32_t));

    for(i=0;i<MAX_KEYS;i++)
    {
        uint32_t length = rand()%20;

        keys[i] = calloc(1, length+8+1);

        TEST_ASSERT_PTR_NOT_NULL(keys[i]);

        uint32_t j;

        /* Ensure each key is unique - not interested in replace entries */
        sprintf(keys[i], "%08d", i);

        for (j=8; j<length; j++)
        {
            keys[i][j] = TOPIC_STRING_CHARS[rand()%strlen(TOPIC_STRING_CHARS)];
        }

        keyHashes[i] = iett_generateTopicStringHash(keys[i]);
    }

    // Add some non-empty entries
    for(i=0;i<MAX_KEYS/4;i++)
    {
        rc = ieut_putHashEntry(pThreadData, table, ieutFLAG_DUPLICATE_ALL, keys[i], keyHashes[i], keys[i], strlen(keys[i])+1);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS/4);

    ieut_removeEmptyHashEntries(pThreadData, table);
    TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS/4);

    // Put some empty entries
    for(i=MAX_KEYS/4;i<MAX_KEYS/2;i++)
    {
        rc = ieut_putHashEntry(pThreadData, table, ieutFLAG_DUPLICATE_ALL, keys[i], keyHashes[i], NULL, 0);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS/2);

    traversalCbContext_t traversalContext = {0};
    ieut_traverseHashTable(pThreadData, table, &test_hashTableTraversal, &traversalContext);
    TEST_ASSERT_EQUAL(traversalContext.called, MAX_KEYS/2);

    ieut_removeEmptyHashEntries(pThreadData, table);
    TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS/4);

    memset(&traversalContext, 0, sizeof(traversalCbContext_t));
    ieut_traverseHashTable(pThreadData, table, &test_hashTableTraversal, &traversalContext);
    TEST_ASSERT_EQUAL(traversalContext.called, MAX_KEYS/4);

    ieut_removeEmptyHashEntries(pThreadData, table);
    TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS/4);

    // Put some more non-empty entries
    for(i=MAX_KEYS/2;i<3*(MAX_KEYS/4);i++)
    {
        rc = ieut_putHashEntry(pThreadData, table, ieutFLAG_DUPLICATE_ALL, keys[i], keyHashes[i], keys[i], strlen(keys[i])+1);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS/2);

    ieut_removeEmptyHashEntries(pThreadData, table);
    TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS/2);

    // Put some more empty entries
    for(i=3*(MAX_KEYS/4);i<MAX_KEYS;i++)
    {
        rc = ieut_putHashEntry(pThreadData, table, ieutFLAG_DUPLICATE_ALL, keys[i], keyHashes[i], NULL, 0);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    TEST_ASSERT_EQUAL(table->totalCount, 3*(MAX_KEYS/4));

    memset(&traversalContext, 0, sizeof(traversalCbContext_t));
    ieut_traverseHashTable(pThreadData, table, &test_hashTableTraversal, &traversalContext);
    TEST_ASSERT_EQUAL(traversalContext.called, 3*(MAX_KEYS/4));

    ieut_removeEmptyHashEntries(pThreadData, table);
    TEST_ASSERT_EQUAL(table->totalCount, MAX_KEYS/2);

    memset(&traversalContext, 0, sizeof(traversalCbContext_t));
    ieut_traverseHashTable(pThreadData, table, &test_hashTableTraversal, &traversalContext);
    TEST_ASSERT_EQUAL(traversalContext.called, MAX_KEYS/2);

    ieut_clearHashTable(pThreadData, table);

    memset(&traversalContext, 0, sizeof(traversalCbContext_t));
    ieut_traverseHashTable(pThreadData, table, &test_hashTableTraversal, &traversalContext);
    TEST_ASSERT_EQUAL(traversalContext.called, 0);

    ieut_destroyHashTable(pThreadData, table);

    for(i=0; i<MAX_KEYS; i++)
    {
        free(keys[i]);
    }

    free(keys);
    free(keyHashes);
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_HashTables[] =
{
    { "HashTables", test_capability_HashTables },
    { "EmptyEntryRemoval", test_capability_EmptyEntryRemoval },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test the adding and removal of subscribers
//****************************************************************************
#define MAX_SUBS 50000

#define ADDREMOVE_SUBSCRIBERS_TEST_TOPIC       "TEST/TOPIC"
#define ADDREMOVE_SUBSCRIBERS_TEST_TOPICCHILD  "TEST/TOPIC/CHILD"
#define ADDREMOVE_SUBSCRIBERS_TEST_TOPICWILD   "TEST/TOPIC/+"
#define ADDREMOVE_SUBSCRIBERS_TEST_TOPICMULTI  "TEST/TOPIC/#"

/*******************************************************************************/
/* Create consumer on A/B/C/D, then create/destroy/create on A/B/C -> segFault */
/*******************************************************************************/
void testBug1(ismEngine_SessionHandle_t hSession,
              int32_t *                 messageCounts,
              ismEngine_Consumer_t **   consumers)
{
    uint32_t rc;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "A/B/C/D",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &messageCounts[0],
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &consumers[0],
                                   NULL,
                                   0,
                                   NULL);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(consumers[0]);

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "A/B/C",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &messageCounts[1],
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &consumers[1],
                                   NULL,
                                   0,
                                   NULL);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(consumers[1]);

    rc = ism_engine_destroyConsumer(consumers[1], NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "A/B/C",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &messageCounts[1],
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &consumers[1],
                                   NULL,
                                   0,
                                   NULL);

    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(consumers[1], NULL, 0, NULL);
    consumers[1] = NULL;
    rc = ism_engine_destroyConsumer(consumers[0], NULL, 0, NULL);
    consumers[0] = NULL;
}

/*******************************************************************************/
/* Create consumer on A/B/C/#, get subscribers for A/B/C, create consumer on   */
/* A/B/C, get subscribers for A/B/C fails to include the subscription on A/B/C */
/* because the cache is not being invalidated correctly.                       */
/*******************************************************************************/
void testBug2(ismEngine_SessionHandle_t hSession,
              int32_t *                 messageCounts,
              ismEngine_Consumer_t **   consumers)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "A/B/C/#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &messageCounts[0],
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &consumers[0],
                                   NULL,
                                   0,
                                   NULL);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(consumers[0]);

    iettSubscriberList_t list = {0};
    list.topicString = "A/B/C";

    rc = iett_getSubscriberList(pThreadData, &list);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 1);
    TEST_ASSERT_EQUAL(list.subscriberNodeCount, 1);

    iett_releaseSubscriberList(pThreadData, &list);

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "A/B/C",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &messageCounts[1],
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &consumers[1],
                                   NULL,
                                   0,
                                   NULL);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(consumers[1]);

    memset(&list, 0, sizeof(iettSubscriberList_t));
    list.topicString = "A/B/C";

    rc = iett_getSubscriberList(pThreadData, &list);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 2);
    TEST_ASSERT_EQUAL(list.subscriberNodeCount, 2);

    iett_releaseSubscriberList(pThreadData, &list);

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "A/B/+",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &messageCounts[2],
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &consumers[2],
                                   NULL,
                                   0,
                                   NULL);

    TEST_ASSERT_EQUAL(rc, OK);

    memset(&list, 0, sizeof(iettSubscriberList_t));
    list.topicString = "A/B/C";

    rc = iett_getSubscriberList(pThreadData, &list);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(list.subscriberCount, 3);
    TEST_ASSERT_EQUAL(list.subscriberNodeCount, 3);

    iett_releaseSubscriberList(pThreadData, &list);

    rc = ism_engine_destroyConsumer(consumers[2], NULL, 0, NULL);
    consumers[2] = NULL;
    rc = ism_engine_destroyConsumer(consumers[1], NULL, 0, NULL);
    consumers[1] = NULL;
    rc = ism_engine_destroyConsumer(consumers[0], NULL, 0, NULL);
    consumers[0] = NULL;
}

void test_capability_AddRemoveSubscribers(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t i;
    uint32_t rc;

    iettTopic_t  topic = {0};
    const char  *substrings[10];
    uint32_t     substringHashes[10];
    const char  *wildcards[10];
    const char  *multicards[10];

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession("TestClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_Consumer_t **consumers = malloc(MAX_SUBS * sizeof(ismEngine_Consumer_t *));
    int32_t               *messageCounts = malloc(MAX_SUBS * sizeof(int32_t));
    char                  *topicString = malloc(iettMAX_TOPIC_DEPTH*10);
    iettSubscriberList_t   list;

    TEST_ASSERT_PTR_NOT_NULL(consumers);
    TEST_ASSERT_PTR_NOT_NULL(messageCounts);
    TEST_ASSERT_PTR_NOT_NULL(topicString);

    printf("  ...limits\n");

    topicString[0] = '\0';
    for(i=0;i<iettMAX_TOPIC_DEPTH;i++)
    {
        char thisSubstring[2];

        sprintf(thisSubstring, "%c", TOPIC_STRING_CHARS[rand()%strlen(TOPIC_STRING_CHARS)]);

        strcat(topicString, thisSubstring);

        if (i<iettMAX_TOPIC_DEPTH-1)
        {
            strcat(topicString, "/");
        }
    }

    // Use random options to get random queue types / behaviour
    ismEngine_SubscriptionAttributes_t subAttrs = { SUB_DELIVERY_OPTIONS[rand()%SUB_DELIVERY_NUMOPTIONS] |
                                                    SUB_OTHER_OPTIONS[rand()%SUB_OTHER_NUMOPTIONS] };

    // Create a non-durable subscription
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   topicString,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &messageCounts[0],
                                   sizeof(int32_t),
                                   NULL, /* No delivery callback */
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &consumers[0],
                                   NULL, 0, NULL);

    TEST_ASSERT_EQUAL(rc, OK);

    memset(&list, 0, sizeof(iettSubscriberList_t));
    list.topicString = topicString;

    rc = iett_getSubscriberList(pThreadData, &list);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL_FORMAT(list.subscribers[0], consumers[0]->engineObject, "%p");

    iett_releaseSubscriberList(pThreadData, &list);

    // Force an odd case in iett_findMatchingSubsNodes where we need to reallocate
    // because there is exactly the right number of results to fit the array passed.
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 10;
    topic.topicString = topicString;
    topic.destinationType = ismDESTINATION_TOPIC;

    rc = iett_analyseTopicString(pThreadData, &topic);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(topic.topicStringCopy);
    TEST_ASSERT_NOT_EQUAL(topic.substrings, substrings);

    uint32_t maxNodes = 1;
    uint32_t nodeCount = 0;
    iettSubsNodeHandle_t *result = iemem_malloc(pThreadData, iemem_subsQuery, sizeof(iettSubsNodeHandle_t));

    TEST_ASSERT_PTR_NOT_NULL(result);

    rc = iett_findMatchingSubsNodes(pThreadData,
                                    ismEngine_serverGlobal.maintree->subs,
                                    &topic,
                                    0,
                                    false,
                                    &maxNodes,
                                    &nodeCount,
                                    &result);
    TEST_ASSERT_EQUAL(rc, OK);

    iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);
    iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
    iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
    if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
    if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);

    iemem_free(pThreadData, iemem_subsQuery, result);

    // Create a durable subscription
    subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
    rc = sync_ism_engine_createSubscription(hClient,
                                            "TEST_SUBSCRIPTION 1",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            topicString,
                                            &subAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyConsumer(consumers[0],
                                    NULL,
                                    0,
                                    NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroySubscription(hClient,
                                        "TEST_SUBSCRIPTION 1",
                                        hClient,
                                        NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(pThreadData,
                                     ((ismEngine_ClientState_t *)hClient)->pClientId,
                                     iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient)->pClientId),
                                     "TEST_SUBSCRIPTION 1",
                                     iett_generateSubNameHash("TEST_SUBSCRIPTION 1"),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // Try lots of additional levels on the topicString
    for (i=0; i<iettMAX_TOPIC_DEPTH*2; i++)
    {
        // Use random options to get random queue types / behaviour
        subAttrs.subOptions = SUB_DELIVERY_OPTIONS[rand()%SUB_DELIVERY_NUMOPTIONS] |
                              SUB_OTHER_OPTIONS[rand()%SUB_OTHER_NUMOPTIONS];

        strcat(topicString,"/A");

        // Create a non-durable subscription
        uint32_t consumerOptions = ((subAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK) == ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE)
                                       ? ismENGINE_CONSUMER_OPTION_NONE
                                       : ismENGINE_CONSUMER_OPTION_ACK;

        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       topicString,
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &messageCounts[0],
                                       sizeof(int32_t),
                                       NULL, /* No delivery callback */
                                       NULL,
                                       consumerOptions,
                                       &consumers[0],
                                       NULL,
                                       0,
                                       NULL);
        TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);

        // Create a durable subscription
        subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_DURABLE;
        rc = sync_ism_engine_createSubscription(hClient,
                                                "TEST_SUBSCRIPTION 1",
                                                NULL,
                                                ismDESTINATION_TOPIC,
                                                topicString,
                                                &subAttrs,
                                                NULL); // Owning client same as requesting client
        TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);

        memset(&list, 0, sizeof(iettSubscriberList_t));
        list.topicString = topicString;

        rc = iett_getSubscriberList(pThreadData, &list);

        TEST_ASSERT_EQUAL(rc, ISMRC_DestNotValid);
    }

    printf("  ...bugs!\n");

    testBug1(hSession, messageCounts, consumers);
    testBug2(hSession, messageCounts, consumers);

    printf("  ...basics\n");

    for(i=0;i<40;i++)
    {
        char *subscriptionTopic;

        // Make subscribers of differing types - after that (and for the rest of the test) we
        // just make QoS0 subscribers (ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE).
        switch(i%10)
        {
            case 0:
                subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
                break;
            case 1:
                subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
                break;
            case 2:
                subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL;
                break;
            case 3:
                subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_TRANSACTION_CAPABLE;
                break;
            default:
                subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
                break;

        }

        // Make subscribers with differing topic strings
        switch(i/10)
        {
            case 0:
                subscriptionTopic = ADDREMOVE_SUBSCRIBERS_TEST_TOPIC;
                break;
            case 1:
                subscriptionTopic = ADDREMOVE_SUBSCRIBERS_TEST_TOPICCHILD;
                break;
            case 2:
                subscriptionTopic = ADDREMOVE_SUBSCRIBERS_TEST_TOPICWILD;
                break;
            case 3:
                subscriptionTopic = ADDREMOVE_SUBSCRIBERS_TEST_TOPICMULTI;
                break;
            default:
                TEST_ASSERT(false, ("Unexpected value for i/10 = %d", i/10));
                break;
        }

        // This will result in an ism_engine_registerSubscriber call
        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       subscriptionTopic,
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &messageCounts[i],
                                       sizeof(int32_t),
                                       NULL, /* No delivery callback */
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &consumers[i],
                                       NULL,
                                       0,
                                       NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(consumers[i]);
        TEST_ASSERT_PTR_NOT_NULL(consumers[i]->engineObject);
        TEST_ASSERT_PTR_NOT_NULL(consumers[i]->queueHandle);
        TEST_ASSERT_EQUAL_FORMAT(consumers[i]->queueHandle, ((ismEngine_Subscription_t *)consumers[i]->engineObject)->queueHandle, "%p");
    }

    // Run Various dump routines not showing the output
    test_debugAndDumpTopic(ADDREMOVE_SUBSCRIBERS_TEST_TOPIC, false);

    // Remove the child consumers for the remainder of the test
    for(i=10; i<20; i++)
    {
        rc = ism_engine_destroyConsumer(consumers[i], NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }
    printf("  ...addition\n");
    for(i=10;i<MAX_SUBS;i++)
    {
        // This will result in an ism_engine_registerSubscriber call
        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       ADDREMOVE_SUBSCRIBERS_TEST_TOPIC,
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &messageCounts[i],
                                       sizeof(int32_t),
                                       NULL, /* No delivery callback */
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_NONE,
                                       &consumers[i],
                                       NULL,
                                       0,
                                       NULL);

        TEST_ASSERT_EQUAL(rc, OK);
    }

    printf("  ...removal\n");
    for(i=MAX_SUBS/4;i<3*(MAX_SUBS/4);i++)
    {
        rc = ism_engine_destroyConsumer(consumers[i],
                                        NULL,
                                        0,
                                        NULL);

        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Get a subscriber list causing the following to go into the delete pending list
    iettSubscriberList_t subsList = {0};
    subsList.topicString = ADDREMOVE_SUBSCRIBERS_TEST_TOPIC;
    rc = iett_getSubscriberList(pThreadData, &subsList);
    TEST_ASSERT_EQUAL(rc, OK);

    for(i=0; i<MAX_SUBS/4; i++)
    {
        rc = ism_engine_destroyConsumer(consumers[i],
                                        NULL,
                                        0,
                                        NULL);

        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Run dump routines
    test_debugAndDumpTopic(ADDREMOVE_SUBSCRIBERS_TEST_TOPIC, false);

    iett_releaseSubscriberList(pThreadData, &subsList);

    for(i=MAX_SUBS-1; i>=(3*(MAX_SUBS/4))+1; i--)
    {
        rc = ism_engine_destroyConsumer(consumers[i],
                                        NULL,
                                        0,
                                        NULL);

        TEST_ASSERT_EQUAL(rc, OK);
    }

    // Remove the last remaining consumer separately (to enable edge-case debugging!)
    rc = ism_engine_destroyConsumer(consumers[3*(MAX_SUBS/4)],
                                    NULL,
                                    0,
                                    NULL);

    TEST_ASSERT_EQUAL(rc, OK);

    /************************************************************************/
    /* Try and find a subscription node - it should not exist with no subs  */
    /************************************************************************/
    memset(&topic, 0, sizeof(topic));
    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = SUBSNODE_ACCESS_TEST_TOPIC;

    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 10;

    rc = iett_analyseTopicString(pThreadData, &topic);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(topic.topicStringCopy);
    TEST_ASSERT_EQUAL(topic.topicStringLength, strlen(SUBSNODE_ACCESS_TEST_TOPIC));

    iettSubsNode_t *node = NULL;

    rc = iett_insertOrFindSubsNode(pThreadData,
                                   iett_getEngineTopicTree(pThreadData)->subs,
                                   &topic,
                                   iettOP_FIND,
                                   &node);

    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
    TEST_ASSERT_PTR_NULL(node);

    printf("  ...repopulation\n");
    for(i=MAX_SUBS/4;i<3*(MAX_SUBS/4);i++)
    {
        // This will result in an ism_engine_registerSubscriber call
        subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       ADDREMOVE_SUBSCRIBERS_TEST_TOPIC,
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &messageCounts[i],
                                       sizeof(int32_t),
                                       NULL, /* No delivery callback */
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_NONE,
                                       &consumers[i],
                                       NULL,
                                       0,
                                       NULL);

        TEST_ASSERT_EQUAL(rc, OK);
    }

    printf("  ...re-removal\n");
    // Leave 10 for destroySession to clear up
    for(i=(MAX_SUBS/4)+10;i<(3*(MAX_SUBS/4)-10);i++)
    {
        rc = ism_engine_destroyConsumer(consumers[i],
                                        NULL,
                                        0,
                                        NULL);

        TEST_ASSERT_EQUAL(rc, OK);
    }

    memset(&list, 0, sizeof(iettSubscriberList_t));
    list.topicString = ADDREMOVE_SUBSCRIBERS_TEST_TOPIC;

    rc = iett_getSubscriberList(pThreadData, &list);

    TEST_ASSERT_EQUAL(rc, OK);

    for(i=MAX_SUBS/4; i<(MAX_SUBS/4)+10; i++)
    {
        rc = ism_engine_destroyConsumer(consumers[i],
                                        NULL,
                                        0,
                                        NULL);

        TEST_ASSERT_EQUAL(rc, OK);
    }

    iett_releaseSubscriberList(pThreadData, &list);

    printf("  ...destroy\n");

    rc = ism_engine_destroySession(hSession,
                                   NULL,
                                   0,
                                   NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);
    free(consumers);
    free(messageCounts);
    free(topicString);
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_AddRemoveSubscribers[] =
{
    { "AddRemoveSubscribers", test_capability_AddRemoveSubscribers },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test a single node with large number of child nodes
//****************************************************************************
#define CHILD_NODES               75000
#define WIDENODE_TOPIC_PARENT     "TEST"
#define WIDENODE_TOPIC_SUBSTRING  "TOPIC%08x"
#define WIDENODE_TOPIC_ROOT       WIDENODE_TOPIC_PARENT"/"WIDENODE_TOPIC_SUBSTRING

void test_capability_WideNode(void)
{
    int32_t i;
    uint32_t rc;
    int32_t startPos = CHILD_NODES/4;
    int32_t endNode = CHILD_NODES+(startPos*2);

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession(__func__,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    char                 **topics = malloc(CHILD_NODES * sizeof(char *));
    ismEngine_Consumer_t **consumers = malloc(CHILD_NODES * sizeof(ismEngine_Consumer_t *));
    int32_t               *messageCounts = malloc(CHILD_NODES * sizeof(int32_t));

    TEST_ASSERT_PTR_NOT_NULL(topics);
    TEST_ASSERT_PTR_NOT_NULL(consumers);
    TEST_ASSERT_PTR_NOT_NULL(messageCounts);

    printf("  ...addition\n");
    for(i=startPos; i<startPos+CHILD_NODES; i++)
    {
        topics[i-startPos] = malloc(sizeof(WIDENODE_TOPIC_ROOT)+8);

        TEST_ASSERT_PTR_NOT_NULL(topics[i-startPos]);

        sprintf(topics[i-startPos], WIDENODE_TOPIC_ROOT, i);

        // This will result in an ism_engine_registerSubscriber call
        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       topics[i-startPos],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &messageCounts[i-startPos],
                                       sizeof(int32_t),
                                       NULL, /* No delivery callback */
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_NONE,
                                       &consumers[i-startPos],
                                       NULL,
                                       0,
                                       NULL);

        TEST_ASSERT_EQUAL(rc, OK);
    }

    printf("  ...access\n");

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    TEST_ASSERT_PTR_NOT_NULL(pThreadData);

    iettTopic_t  topic;
    const char  *substrings[10];
    uint32_t     substringHashes[10];
    const char  *wildcards[10];
    const char  *multicards[10];

    memset(&topic, 0, sizeof(topic));
    topic.destinationType = ismDESTINATION_TOPIC;
    topic.topicString = WIDENODE_TOPIC_PARENT;
    topic.substrings = substrings;
    topic.substringHashes = substringHashes;
    topic.wildcards = wildcards;
    topic.multicards = multicards;
    topic.initialArraySize = 10;

    /*****************************************************************/
    /* Need to get an analysis of the topic string to use when       */
    /* calling functions using a topic string on the topic tree.     */
    /*****************************************************************/
    rc = iett_analyseTopicString(pThreadData, &topic);
    TEST_ASSERT_EQUAL(rc, OK);

    /*****************************************************************/
    /* Try and find the node - it should not exist yet.              */
    /*****************************************************************/
    iettSubsNode_t *node = NULL;
    rc = iett_insertOrFindSubsNode(pThreadData,
                                   ismEngine_serverGlobal.maintree->subs,
                                   &topic, iettOP_FIND, &node);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(node);

    // Make sure hashes within a chain increase
    for(i=0; i<node->children->capacity; i++)
    {
        if (node->children->chains[i].count > 5)
        {
            uint32_t baseHash = 0;

            for(int32_t x=0; x<node->children->chains[i].count; x++)
            {
                uint32_t thisHash = node->children->chains[i].entries[x].keyHash;

                if (x==0)
                {
                    baseHash = thisHash;
                }
                else
                {
                    TEST_ASSERT_GREATER_THAN_OR_EQUAL(thisHash, baseHash);
                }
            }
        }
    }

    iemem_free(pThreadData, iemem_topicAnalysis, topic.topicStringCopy);

    if (topic.substrings != substrings) iemem_free(pThreadData, iemem_topicAnalysis, topic.substrings);
    if (topic.substringHashes != substringHashes) iemem_free(pThreadData, iemem_topicAnalysis, topic.substringHashes);
    if (topic.wildcards != wildcards) iemem_free(pThreadData, iemem_topicAnalysis, topic.wildcards);
    if (topic.multicards != multicards) iemem_free(pThreadData, iemem_topicAnalysis, topic.multicards);

    ism_time_t startTime, endTime, diffTime;
    double diffTimeSecs;

    int32_t hit=0;
    int32_t miss=0;
    int32_t accessTestCount = 10000000; // 10 Million requests
    char accessTopic[sizeof(WIDENODE_TOPIC_SUBSTRING)+8];

    startTime = ism_common_currentTimeNanos();

    for(i=0; i<accessTestCount; i++)
    {
        int32_t testVal = rand()%endNode;
        sprintf(accessTopic, WIDENODE_TOPIC_SUBSTRING, testVal);

        void *entry = NULL;
        rc = ieut_getHashEntry(node->children,
                               accessTopic,
                               iett_generateSubstringHash(accessTopic),
                               &entry);

        if (testVal >= startPos && testVal < startPos + CHILD_NODES)
        {
            hit++;
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(entry);
        }
        else
        {
            miss++;
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
            TEST_ASSERT_PTR_NULL(entry);
        }
    }

    endTime = ism_common_currentTimeNanos();

    diffTime = endTime-startTime;
    diffTimeSecs = ((double)diffTime) / 1000000000;
    printf("  ...time to make %d topic searches (%d hits, %d misses) from node with %d children is %.2f secs. (%ldns)\n",
           accessTestCount, hit, miss, CHILD_NODES, diffTimeSecs, diffTime);

    printf("  ...removal\n");
    for(i=0;i<CHILD_NODES;i++)
    {
        rc = ism_engine_destroyConsumer(consumers[i],
                                        NULL,
                                        0,
                                        NULL);

        TEST_ASSERT_EQUAL(rc, OK);

        free(topics[i]);
    }

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    free(topics);
    free(consumers);
    free(messageCounts);
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_WideNode[] =
{
    { "WideNode", test_capability_WideNode },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test NoLocal flag support
//****************************************************************************
char *NOLOCAL_TOPICS[] = {"TEST/NOLOCAL", "TEST/+", "TEST/NOLOCAL/#", "TEST/#"};
#define NOLOCAL_NUMTOPICS (sizeof(NOLOCAL_TOPICS)/sizeof(NOLOCAL_TOPICS[0]))

typedef struct tag_nolocalMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
} nolocalMessagesCbContext_t;

bool nolocalMessagesCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
                             void *                          pContext,
                             ismEngine_DelivererContext_t *  _delivererContext)
{
    nolocalMessagesCbContext_t *context = *((nolocalMessagesCbContext_t **)pContext);

    __sync_fetch_and_add(&context->received, 1);

    ism_engine_releaseMessage(hMessage);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        NULL,
                                                        0,
                                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    return true; // more messages, please.
}

void test_capability_NoLocal(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient3, hClient, hClient2;
    ismEngine_SessionHandle_t hSession3, hSession, hSession2;
    ismEngine_ProducerHandle_t hProducer2;
    ismEngine_ConsumerHandle_t nolocalConsumer,
                               qos0Consumer,
                               durnolocalConsumer,
                               durConsumer;
    nolocalMessagesCbContext_t nolocalContext={0},
                               qos0Context={0},
                               durContext={0};
    nolocalMessagesCbContext_t *nolocalCb=&nolocalContext,
                               *qos0Cb=&qos0Context,
                               *durCb=&durContext;

    printf("Starting %s...\n", __func__);

    /* Create our clients and sessions */
    rc = test_createClientAndSession("NoLocalClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession("AnotherClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession(__func__,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient3, &hSession3, true);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    nolocalContext.hSession = hSession;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                          ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL;

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   NOLOCAL_TOPICS[rand()%NOLOCAL_NUMTOPICS],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &nolocalCb,
                                   sizeof(nolocalMessagesCbContext_t *),
                                   nolocalMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &nolocalConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    qos0Context.hSession = hSession3;
    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
    rc = ism_engine_createConsumer(hSession3,
                                   ismDESTINATION_TOPIC,
                                   NOLOCAL_TOPICS[rand()%NOLOCAL_NUMTOPICS],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &qos0Cb,
                                   sizeof(nolocalMessagesCbContext_t *),
                                   nolocalMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &qos0Consumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_createProducer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   NOLOCAL_TOPICS[0],
                                   &hProducer2,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t dnlSubAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                       ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                                                       ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL };

    rc = sync_ism_engine_createSubscription(hClient,
                                            "DURABLENOLOCAL",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            NOLOCAL_TOPICS[rand()%NOLOCAL_NUMTOPICS],
                                            &dnlSubAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t dSubAttrs = { ismENGINE_SUBSCRIPTION_OPTION_DURABLE |
                                                     ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };

    rc = sync_ism_engine_createSubscription(hClient2,
                                            "DURABLE",
                                            NULL,
                                            ismDESTINATION_TOPIC,
                                            NOLOCAL_TOPICS[rand()%NOLOCAL_NUMTOPICS],
                                            &dSubAttrs,
                                            NULL); // Owning client same as requesting client
    TEST_ASSERT_EQUAL(rc, OK);

    /* Create messages to play with */
    void *payload1="NoId Message", *payload2="Message", *payload3="Other Message", *payload4="Internal Message";
    ismEngine_MessageHandle_t hMessage1, hMessage2, hMessage3, hMessage4;

    printf("  ...publish\n");

    rc = test_createMessage(strlen(payload1),
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, NOLOCAL_TOPICS[0],
                            &hMessage1, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    // Should add 1 to ALL counts since being sent from session with no client Id
    rc = ism_engine_putMessageOnDestination(hSession3,
                                            ismDESTINATION_TOPIC,
                                            NOLOCAL_TOPICS[0],
                                            NULL,
                                            hMessage1,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(strlen(payload2),
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, NOLOCAL_TOPICS[0],
                            &hMessage2, &payload2);
    TEST_ASSERT_EQUAL(rc, OK);

    // Should add 1 to the non-nolocal consumers, 0 to the nolocal ones.
    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            NOLOCAL_TOPICS[0],
                                            NULL,
                                            hMessage2,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(strlen(payload3),
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, NOLOCAL_TOPICS[0],
                            &hMessage3, &payload3);
    TEST_ASSERT_EQUAL(rc, OK);

    // Should add 1 to ALL counts since being sent from session with a different client id
    rc = ism_engine_putMessage(hSession2,
                               hProducer2,
                               NULL,
                               hMessage3,
                               NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(strlen(payload4),
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, NOLOCAL_TOPICS[0],
                            &hMessage4, &payload4);
    TEST_ASSERT_EQUAL(rc, OK);

    // Should add 1 to ALL counts since being sent from session with no client Id
    rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                    NOLOCAL_TOPICS[0],
                                                    hMessage4,
                                                    NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // ism_store_dump("stdout");

    // Check we got the expected messages
    TEST_ASSERT_EQUAL(qos0Context.received, 4);

    // Create consumers for the durables
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "DURABLENOLOCAL",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &nolocalCb,
                                   sizeof(nolocalMessagesCbContext_t *),
                                   nolocalMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(dnlSubAttrs.subOptions),
                                   &durnolocalConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    durContext.hSession = hSession2;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_SUBSCRIPTION,
                                   "DURABLE",
                                   NULL,
                                   NULL, // Owning client same as session client
                                   &durCb,
                                   sizeof(nolocalMessagesCbContext_t *),
                                   nolocalMessagesCallback,
                                   NULL,
                                   ((dSubAttrs.subOptions & ismENGINE_SUBSCRIPTION_OPTION_DELIVERY_MASK) == ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE)
                                       ? ismENGINE_CONSUMER_OPTION_NONE
                                       : ismENGINE_CONSUMER_OPTION_ACK,
                                   &durConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Check we got the expected messages
    TEST_ASSERT_EQUAL(nolocalContext.received, 6);
    TEST_ASSERT_EQUAL(durContext.received, 4);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient3, hSession3, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient2, hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);
}

char *EMPTY_NOLOCAL_TOPICS[] = {"EMPTY_NOLOCAL/AT_MOST_ONCE", "EMPTY_NOLOCAL/AT_LEAST_ONCE", "EMPTY_NOLOCAL/EXACTLY_ONCE"};

void test_capability_EmptyNoLocal(void)
{
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer[4];
    nolocalMessagesCbContext_t Context[4];
    nolocalMessagesCbContext_t *Cb[4] = { &Context[0], &Context[0], &Context[0], &Context[0]};

    printf("Starting %s...\n", __func__);

    /* Create our clients and sessions */
    rc = test_createClientAndSession("NoLocalClient",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    void *payload="Message";

    for(int32_t consumer=0; consumer<3; consumer++)
    {
        ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_NO_LOCAL };

        memset(&Context[consumer], 0, sizeof(nolocalMessagesCbContext_t));

        switch(consumer)
        {
            case 0:
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;
                break;
            case 1:
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
                break;
            case 2:
                subAttrs.subOptions |= ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
                break;
            default:
                //TEST_ASSERT_CUNIT(false, "Invalid loop value");
                break;
        }

        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       EMPTY_NOLOCAL_TOPICS[consumer],
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &Cb[consumer],
                                       sizeof(nolocalMessagesCbContext_t *),
                                       nolocalMessagesCallback,
                                       NULL,
                                       test_getDefaultConsumerOptions(subAttrs.subOptions),
                                       &hConsumer[consumer],
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        for(int32_t i=0; i<6; i++)
        {
            uint8_t msgPersistence = (i<3 ? ismMESSAGE_PERSISTENCE_NONPERSISTENT : ismMESSAGE_PERSISTENCE_PERSISTENT);
            uint8_t msgReliability = 0;

            switch(i)
            {
                case 0:
                case 3:
                    msgReliability = ismMESSAGE_RELIABILITY_AT_MOST_ONCE;
                    break;
                case 1:
                case 4:
                    msgReliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
                    break;
                case 2:
                case 5:
                    msgReliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
                    break;

            }

            ismEngine_MessageHandle_t hMessage = NULL;

            rc = test_createMessage(strlen(payload),
                                    msgPersistence,
                                    msgReliability,
                                    ismMESSAGE_FLAGS_NONE,
                                    0,
                                    ismDESTINATION_TOPIC, EMPTY_NOLOCAL_TOPICS[consumer],
                                    &hMessage, &payload);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hMessage);

            rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_TOPIC,
                                                    EMPTY_NOLOCAL_TOPICS[consumer],
                                                    NULL,
                                                    hMessage);
            TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
        }

        rc = ism_engine_destroyConsumer(hConsumer[consumer], NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_NoLocal[] =
{
    { "NoLocal", test_capability_NoLocal },
    { "EmptyNolocal", test_capability_EmptyNoLocal },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test Subscription with a selection string
//****************************************************************************
#define SELECT_ALL_TOPIC "TEST/SELECT/ALL"

//****************************************************************************
// This test verifies selection using either a defaul selection rule on a
// policy or a subscription with a selection string which matches all messages
// published actually receives all of the messages
void test_capability_Selects_All(bool usePolicy)
{
    uint32_t rc;
    const uint32_t messageCount = 6;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    genericMsgCbContext_t ConsumerContext, *pConsumerContext = &ConsumerContext;
    char payload[128];
    
    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    test_selectionCallback_t expectSelection = {SELECT_ALL_TOPIC};

    pSelectionExpected = &expectSelection;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "TestClient";

    /* Create our clients and sessions */
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ConsumerContext.hSession = hSession;
    ConsumerContext.received = 0;

    printf("  ...create\n");

    // Initiliase the properties which contain the selection string
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    uint32_t subOptions;

    if (usePolicy)
    {
        subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;

        // Add a topic policy that allows publish / subscribe on 'TEST/SELECTION'
        // For clients whose ID is 'TestClient' and sets a default selection rule
        // which should select all messages.
        rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                    "{\"UID\":\"UID.SELECTS_ALL_POLICY\","
                                    "\"Name\":\"SELECTS_ALL_POLICY\","
                                    "\"ClientID\":\"TestClient\","
                                    "\"Topic\":\""SELECT_ALL_TOPIC"\","
                                    "\"DefaultSelectionRule\":\"Colour='BLUE'\","
                                    "\"ActionList\":\"subscribe,publish\"}");
    }
    else
    {
        char *selectorString = "Colour='BLUE'";
        ism_field_t SelectorField = {VT_String, 0, {.s = selectorString }};
        rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
        TEST_ASSERT_EQUAL(rc, OK);
        subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                     ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION;

        // Add a topic policy that allows publish / subscribe on 'TEST/SELECTION'
        // For clients whose ID is 'TestClient' and sets a default selection rule
        // which should not be referred to (and so doesn't match any messages).
        rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                    "{\"UID\":\"UID.SELECTS_ALL_POLICY\","
                                    "\"Name\":\"SELECTS_ALL_POLICY\","
                                    "\"ClientID\":\"TestClient\","
                                    "\"Topic\":\""SELECT_ALL_TOPIC"\","
                                    "\"DefaultSelectionRule\":\"Colour='ORANGE'\","
                                    "\"ActionList\":\"subscribe,publish\"}");
    }

    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, "SELECTS_ALL_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the subscription
    ismEngine_SubscriptionAttributes_t subAttrs = { subOptions };

    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   SELECT_ALL_TOPIC,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pConsumerContext,
                                   sizeof(genericMsgCbContext_t *),
                                   genericMessageCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    // Now publish 6 messages all which match the selection string
    properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    char *ColourText="BLUE";
    ism_field_t ColourField = {VT_String, 0, {.s = ColourText }};
    rc = ism_common_setProperty(properties, "Colour", &ColourField);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i<messageCount; i++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        rc = ism_common_serializeProperties(properties, &FlatProperties);
        TEST_ASSERT_EQUAL(rc, OK);

        sprintf(payload, "Message - %d. Property is '%s'", i, ColourText);

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                SELECT_ALL_TOPIC,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    // And verify that all of the messages have been received
    TEST_ASSERT_EQUAL(ConsumerContext.received, messageCount);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_deleteSecurityPolicy("UID.SELECTS_ALL_POLICY", "SELECTS_ALL_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    pSelectionExpected = NULL;
}

//****************************************************************************
// Call the Selects_All test requesting per-subscription selection
void test_capability_Selects_All_Subscriber(void)
{
    printf("Starting %s...\n", __func__);

    test_capability_Selects_All(false);
}

//****************************************************************************
// Call the Selects_All test requesting policy selection
void test_capability_Selects_All_Policy(void)
{
    printf("Starting %s...\n", __func__);

    test_capability_Selects_All(true);
}

#define SELECT_NONE_TOPIC "TEST/SELECT/NONE"

//****************************************************************************
// This test verifies that a subscription with a selection string which
// matches no messages published receives actually receives none
void test_capability_Selects_None(bool usePolicy)
{
    uint32_t rc;
    const uint32_t messageCount = 6;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    genericMsgCbContext_t ConsumerContext, *pConsumerContext = &ConsumerContext;
    char payload[128];
    
    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    test_selectionCallback_t expectSelection = {SELECT_NONE_TOPIC};

    pSelectionExpected = &expectSelection;

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "TestClient";

    /* Create our clients and sessions */
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ConsumerContext.hSession = hSession;
    ConsumerContext.received = 0;

    printf("  ...create [** EXPECT FFDCs FROM iett_allocateSubscription **]\n");

    uint32_t subOptions;

    // Initiliase the properties which contain the selection string
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    // Add a padding field so that the VT_ByteArray which gets specified in one of the tests
    // is not aligned on a 4-byte boundary.
    ism_field_t PaddingField = {VT_Byte, 1, {.i = 3}};
    rc = ism_common_setProperty(properties, "PADDING", &PaddingField);
    TEST_ASSERT_EQUAL(rc, OK);

    char *selectorString = "Colour='BLUE'";
    ism_field_t SelectorField = {VT_String, 0, {.s = NULL }};

    if (usePolicy)
    {
        subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;

        // Add a topic policy that allows publish / subscribe on 'TEST/SELECTION'
        // For clients whose ID is 'TestClient' and sets a default selection rule
        // which should select no messages.
        rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                    "{\"UID\":\"UID.SELECTS_NONE_POLICY\","
                                    "\"Name\":\"SELECTS_NONE_POLICY\","
                                    "\"ClientID\":\"TestClient\","
                                    "\"Topic\":\""SELECT_NONE_TOPIC"\","
                                    "\"DefaultSelectionRule\":\"Colour='ORANGE'\","
                                    "\"ActionList\":\"subscribe,publish\"}");
    }
    else
    {
        SelectorField.val.s = selectorString;
        rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
        TEST_ASSERT_EQUAL(rc, OK);
        subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                     ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION;

        // Add a topic policy that allows publish / subscribe on 'TEST/SELECTION'
        // For clients whose ID is 'TestClient' and sets a default selection rule
        // which should not be referred to (and so matches all messages).
        rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                    "{\"UID\":\"UID.SELECTS_NONE_POLICY\","
                                    "\"Name\":\"SELECTS_NONE_POLICY\","
                                    "\"ClientID\":\"TestClient\","
                                    "\"Topic\":\""SELECT_NONE_TOPIC"\","
                                    "\"DefaultSelectionRule\":\"Colour='RED'\","
                                    "\"ActionList\":\"subscribe,publish\"}");
    }

    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, "SELECTS_NONE_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    // If not using a policy, We loop multiple times:
    //
    // 1) Specifying valid properties
    // 2) Specifying invalid properties containing no selector property
    // 3) Specifying invalid properties containing a malformed selector property
    // 4) Specifying compiled selection rule
    // 5) Specifying no properties
    //
    // In each case we expect either a selection string that matches everything we publish
    // or no selection string at all
    int32_t endLoop = usePolicy ? 1 : 5;
    for(int32_t loop=0; loop<endLoop; loop++)
    {
        if (loop == 2)
        {
            SelectorField.val.s = "Colour='RED"; // Note: invalid, no trailing quote
            rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else if (loop == 3)
        {
            uint32_t selectionRuleLen;
            struct ismRule_t *selectionRule;
            rc = ism_common_compileSelectRule(&selectionRule,
                                              (int *)&selectionRuleLen,
                                              selectorString);

            TEST_ASSERT_EQUAL(rc, OK);

            SelectorField.type = VT_ByteArray;
            SelectorField.len = selectionRuleLen;
            SelectorField.val.s = (char *)selectionRule;

            rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
            TEST_ASSERT_EQUAL(rc, OK);

            // Free the compiled selection rule
            ism_common_freeSelectRule(selectionRule);
        }

        // Now create the subscription
        ismEngine_SubscriptionAttributes_t subAttrs = { subOptions };
        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       SELECT_NONE_TOPIC,
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &pConsumerContext,
                                       sizeof(genericMsgCbContext_t *),
                                       genericMessageCallback,
                                       (loop < 4) ? properties : NULL,
                                       ismENGINE_CONSUMER_OPTION_NONE,
                                       &hConsumer,
                                       NULL, 0, NULL);
        if (loop == 0)
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else if (loop == 1)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);
        }
        else if (loop == 2)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);
        }
        else if (loop == 3)
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else if (loop == 4)
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);
        }

        // And free the properties we allocated
        ism_common_freeProperties(properties);

        // Now publish 6 messages all which match the selection string
        properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        char *ColourText="RED";
        ism_field_t ColourField = {VT_String, 0, {.s = ColourText }};
        rc = ism_common_setProperty(properties, "Colour", &ColourField);
        TEST_ASSERT_EQUAL(rc, OK);

        for(int32_t i=0; i<messageCount; i++)
        {
            ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
            ismMessageAreaType_t areaTypes[2];
            size_t areaLengths[2];
            void *areas[2];
            concat_alloc_t FlatProperties = { NULL };
            char localPropBuffer[1024];

            FlatProperties.buf = localPropBuffer;
            FlatProperties.len = 1024;

            rc = ism_common_serializeProperties(properties, &FlatProperties);
            TEST_ASSERT_EQUAL(rc, OK);

            sprintf(payload, "Message - %d. Property is '%s'", i, ColourText);

            areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
            areaLengths[0] = FlatProperties.used;
            areas[0] = FlatProperties.buf;
            areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
            areaLengths[1] = strlen(payload) +1;
            areas[1] = (void *)payload;

            header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
            header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

            ismEngine_MessageHandle_t hMessage = NULL;
            rc = ism_engine_createMessage(&header,
                                          2,
                                          areaTypes,
                                          areaLengths,
                                          areas,
                                          &hMessage);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hMessage);

            rc = ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_TOPIC,
                                                    SELECT_NONE_TOPIC,
                                                    NULL,
                                                    hMessage,
                                                    NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
        }

        if (loop == 0 || loop == 3)
        {
            rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    // And verify that none of the messages have been received
    TEST_ASSERT_EQUAL(ConsumerContext.received, 0);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_deleteSecurityPolicy("UID.SELECTS_NONE_POLICY", "SELECTS_NONE_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    pSelectionExpected = NULL;
}

//****************************************************************************
// Call the Selects_None test requesting per-subscription selection
void test_capability_Selects_None_Subscriber(void)
{
    printf("Starting %s...\n", __func__);

    test_capability_Selects_None(false);
}

//****************************************************************************
// Call the Selects_All test requesting policy selection
void test_capability_Selects_None_Policy(void)
{
    printf("Starting %s...\n", __func__);

    test_capability_Selects_None(true);
}

//****************************************************************************
// This test verifies that a subscription with a selection string which
// matches no messages published receives actually receives none
void test_capability_Selects_ChangingPolicy(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    genericMsgCbContext_t ConsumerContext, *pConsumerContext = &ConsumerContext;

    ism_listener_t *mockListener=NULL;
    ism_transport_t *mockTransport=NULL;
    ismSecurity_t *mockContext;

    test_selectionCallback_t expectSelection = {0};

    pSelectionExpected = &expectSelection;

    printf("Starting %s...\n", __func__);

    rc = test_createSecurityContext(&mockListener,
                                    &mockTransport,
                                    &mockContext);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(mockListener);
    TEST_ASSERT_PTR_NOT_NULL(mockTransport);
    TEST_ASSERT_PTR_NOT_NULL(mockContext);

    mockTransport->clientID = "TestClient";

    /* Create our clients and sessions */
    rc = test_createClientAndSession(mockTransport->clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ConsumerContext.hSession = hSession;
    ConsumerContext.received = 0;

    printf("  ...create\n");

    uint32_t subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE;

    char policyStringFormat[] = "{\"UID\":\"UID.SELECTS_CHANGING_POLICY\","
                                "\"Name\":\"SELECTS_CHANGING_POLICY\","
                                "\"ClientID\":\"TestClient\","
                                "\"Topic\":\"*\","
                                "\"DefaultSelectionRule\":\"%s\","
                                "\"ActionList\":\"subscribe,publish\","
                                "\"Update\":true}";

    char policyString[strlen(policyStringFormat)+1024];

    sprintf(policyString, policyStringFormat, "");

    // Add a topic policy that allows publish / subscribe on any topic, and
    // has no selection rule.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY, policyString);
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, "SELECTS_CHANGING_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the subscription
    ismEngine_SubscriptionAttributes_t subAttrs = { subOptions };
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "#",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pConsumerContext,
                                   sizeof(genericMsgCbContext_t *),
                                   genericMessageCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    iepiPolicyInfo_t *origPolicy = ieq_getPolicyInfo(hConsumer->queueHandle);

    // Change the policy a bunch of ways and publish messages to see which ones arrive
    void   *payload=NULL;
    ismEngine_MessageHandle_t hMessage;
    uint32_t loop = 0;
    char *updatePolicy;
    const char *topic[] = {"iot-2/myorg1/topic", "iot-2/myorg2/topic", "iot-2/myorg3/topic"};
    uint32_t topicCount = (uint32_t)(sizeof(topic)/sizeof(topic[0]));
    uint32_t publishCount = 10;
    uint32_t expectReceived = 0; // We expect to receive the 1st msg
    bool expectMsgs[topicCount];

    memset(expectMsgs, true, sizeof(expectMsgs));

    while(1)
    {
        uint32_t published[topicCount];
        memset(published, 0, sizeof(published));

        for(uint32_t i=0; i<publishCount; i++)
        {
            int32_t topicNum = rand()%topicCount;

            expectSelection.expectTopic = topic[topicNum];

            rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                                    ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                    ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                    ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                                    0,
                                    ismDESTINATION_TOPIC, topic[topicNum],
                                    &hMessage, &payload);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_TOPIC,
                                                    topic[topicNum],
                                                    NULL,
                                                    hMessage,
                                                    NULL, 0, NULL);
            if (rc != OK) TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

            published[topicNum]++;
        }

        for(uint32_t i=0; i<topicCount; i++)
        {
            if (expectMsgs[i]) expectReceived += published[i];
        }

        TEST_ASSERT_EQUAL(ConsumerContext.received, expectReceived);

        // Confirm that the policy itself hasn't changed (just the selection info)
        TEST_ASSERT_EQUAL(origPolicy, ieq_getPolicyInfo(hConsumer->queueHandle));

        updatePolicy = NULL;
        switch(++loop)
        {
            case 1:
                expectMsgs[0] = false;
                updatePolicy = "JMS_Topic not like 'iot-2/myorg1/\%'";
                break;
            case 2:
                expectMsgs[0] = expectMsgs[1] = false;
                updatePolicy = "JMS_Topic not like 'iot-2/myorg1/\%' and JMS_Topic not like 'iot-2/myorg2/\%'";
                break;
            case 3:
                memset(expectMsgs, true, sizeof(expectMsgs));
                updatePolicy = "";
                break;
            case 4:
                memset(expectMsgs, false, sizeof(expectMsgs));
                updatePolicy = "Colour = 'Red'";
                break;
            case 5:
                memset(expectMsgs, true, sizeof(expectMsgs));
                updatePolicy = "Colour = 'Red' or JMS_Topic = 'iot-2/myorg1/topic' or JMS_Topic like 'iot-2/myorg2/\%' or JMS_Topic = 'iot-2/myorg3/topic'";
                break;
            case 6:
                memset(expectMsgs, false, sizeof(expectMsgs));
                updatePolicy = "JMS_Topic like 'other\%'";
                break;
            default:
                goto LEAVE_LOOP;
        }

        if (updatePolicy)
        {
            TEST_ASSERT_GREATER_THAN(sizeof(policyString), strlen(policyStringFormat)+strlen(updatePolicy));
            sprintf(policyString, policyStringFormat, updatePolicy);
            rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY, policyString);
            TEST_ASSERT_EQUAL(rc, OK);
        }
    }

LEAVE_LOOP:

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_deleteSecurityPolicy("UID.SELECTS_CHANGING_POLICY", "SELECTS_CHANGING_POLICY");
    TEST_ASSERT_EQUAL(rc, OK);

    pSelectionExpected = NULL;
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Selection_SubAndPolicy[] =
{
    { "SingleSubscriberSelectsAll", test_capability_Selects_All_Subscriber },
    { "DefaultSelectionSelectsAll", test_capability_Selects_All_Policy },
    { "SingleSubscriberSelectsNone", test_capability_Selects_None_Subscriber },
    { "DefaultSelectionSelectsNone", test_capability_Selects_None_Policy },
    { "DefaultSelectionChanges", test_capability_Selects_ChangingPolicy },
    CU_TEST_INFO_NULL
};

#define SELECT_SOME_TOPIC "TEST/SELECT/SOME"

//****************************************************************************
// This test verifies that a subscription with a selection string which
// matches some of the messages published receives actually receives the 
// correct number of messages
void test_capability_Sub_Selects_Some(void)
{
    uint32_t rc;
    const uint32_t messageCount = 18;
    const uint32_t HitCount = 10;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    genericMsgCbContext_t ConsumerContext, *pConsumerContext = &ConsumerContext;
    char payload[128];
    
    test_selectionCallback_t expectSelection = {SELECT_SOME_TOPIC};

    pSelectionExpected = &expectSelection;

    printf("Starting %s...\n", __func__);

    /* Create our clients and sessions */
    rc = test_createClientAndSession(__func__,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ConsumerContext.hSession = hSession;
    ConsumerContext.received = 0;

    printf("  ...create\n");

    // Initiliase the properties which contain the selection string
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    char *selectorString = "Colour='BLUE'";
    ism_field_t SelectorField = {VT_String, 0, {.s = selectorString }};
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the subscription
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION };
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   SELECT_SOME_TOPIC,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pConsumerContext,
                                   sizeof(genericMsgCbContext_t *),
                                   genericMessageCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    // Now publish 18 messages 10 match the selection string and 8 do not

    ism_prop_t *HitProperties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(HitProperties);
    char *HitColourText="BLUE";
    ism_field_t HitColourField = {VT_String, 0, {.s = HitColourText }}; 
    rc = ism_common_setProperty(HitProperties, "Colour", &HitColourField);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_prop_t *MissProperties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(MissProperties);
    char *MissColourText="RED";
    ism_field_t MissColourField = {VT_String, 0, {.s = MissColourText }}; 
    rc = ism_common_setProperty(MissProperties, "Colour", &MissColourField);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i<messageCount; i++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        if (i < HitCount)
        {
            rc = ism_common_serializeProperties(HitProperties, &FlatProperties);
            TEST_ASSERT_EQUAL(rc, OK);

            sprintf(payload, "Message - %d. Property is '%s'", i, HitColourText);
        }
        else
        {
            rc = ism_common_serializeProperties(MissProperties, &FlatProperties);
            TEST_ASSERT_EQUAL(rc, OK);

            sprintf(payload, "Message - %d. Property is '%s'", i, MissColourText);
        }

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                SELECT_SOME_TOPIC,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        if (i < HitCount)
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
        }
    }

    // And free the properties we allocated
    ism_common_freeProperties(HitProperties);
    ism_common_freeProperties(MissProperties);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And verify that none of the messages have been received
    TEST_ASSERT_EQUAL(ConsumerContext.received, 10);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    pSelectionExpected = NULL;
}

//****************************************************************************
// This test verifies that a subscription with a selection string which
// matches some of the messages published receives actually receives the 
// correct number of messages. This test passes the selection strings as a
// compiled selection string.
void test_capability_Sub_Selects_SomeCompiled(void)
{
    uint32_t rc;
    const uint32_t messageCount = 18;
    const uint32_t HitCount = 10;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    genericMsgCbContext_t ConsumerContext, *pConsumerContext = &ConsumerContext;
    char payload[128];
    
    test_selectionCallback_t expectSelection = {SELECT_SOME_TOPIC};

    pSelectionExpected = &expectSelection;

    printf("Starting %s...\n", __func__);

    /* Create our clients and sessions */
    rc = test_createClientAndSession(__func__,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ConsumerContext.hSession = hSession;
    ConsumerContext.received = 0;

    printf("  ...create\n");

    // Initiliase the properties which contain the selection string
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    char *selectorString = "Colour='BLUE'";
    uint32_t selectionRuleLen = 0;
    struct ismRule_t *selectionRule = NULL;
    rc = ism_common_compileSelectRule(&selectionRule,
                                      (int *)&selectionRuleLen,
                                      selectorString);

    ism_field_t SelectorField = {VT_ByteArray, selectionRuleLen, {.s = (char *)selectionRule }};
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
    TEST_ASSERT_EQUAL(rc, OK);

    // Free the compiled selection rule
    ism_common_freeSelectRule(selectionRule);

    // Now create the subscription
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION };
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   SELECT_SOME_TOPIC,
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &pConsumerContext,
                                   sizeof(genericMsgCbContext_t *),
                                   genericMessageCallback,
                                   properties,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    // Now publish 18 messages 10 match the selection string and 8 do not
    ism_prop_t *HitProperties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(HitProperties);
    char *HitColourText="BLUE";
    ism_field_t HitColourField = {VT_String, 0, {.s = HitColourText }}; 
    rc = ism_common_setProperty(HitProperties, "Colour", &HitColourField);
    TEST_ASSERT_EQUAL(rc, OK);

    ism_prop_t *MissProperties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(MissProperties);
    char *MissColourText="RED";
    ism_field_t MissColourField = {VT_String, 0, {.s = MissColourText }}; 
    rc = ism_common_setProperty(MissProperties, "Colour", &MissColourField);
    TEST_ASSERT_EQUAL(rc, OK);

    for(int32_t i=0; i<messageCount; i++)
    {
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2];
        size_t areaLengths[2];
        void *areas[2];
        concat_alloc_t FlatProperties = { NULL };
        char localPropBuffer[1024];

        FlatProperties.buf = localPropBuffer;
        FlatProperties.len = 1024;

        if (i < HitCount)
        {
            rc = ism_common_serializeProperties(HitProperties, &FlatProperties);
            TEST_ASSERT_EQUAL(rc, OK);

            sprintf(payload, "Message - %d. Property is '%s'", i, HitColourText);
        }
        else
        {
            rc = ism_common_serializeProperties(MissProperties, &FlatProperties);
            TEST_ASSERT_EQUAL(rc, OK);

            sprintf(payload, "Message - %d. Property is '%s'", i, MissColourText);
        }

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = FlatProperties.used;
        areas[0] = FlatProperties.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = strlen(payload) +1;
        areas[1] = (void *)payload;

        header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

        ismEngine_MessageHandle_t hMessage = NULL;
        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areas,
                                      &hMessage);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                SELECT_SOME_TOPIC,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        if (i < HitCount)
        {
            TEST_ASSERT_EQUAL(rc, OK);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
        }
    }

    // And free the properties we allocated
    ism_common_freeProperties(HitProperties);
    ism_common_freeProperties(MissProperties);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And verify that none of the messages have been received
    TEST_ASSERT_EQUAL(ConsumerContext.received, 10);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    pSelectionExpected = NULL;
}

//****************************************************************************
// This test verifies that a subscription with a selection string which
// matches some of the messages published receives actually receives the
// correct number of messages
#define SELECT_RELIABILITY_TOPIC "TEST/SELECT/RELIABILITY"

void test_capability_Sub_Selects_Reliability(void)
{
    uint32_t rc;
    const uint32_t messageCount = 100;
    uint32_t unreliableCount = 0;
    uint32_t reliableCount = 0;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    uint32_t subSelectOpts[] = {ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE,
            ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE | ismENGINE_SUBSCRIPTION_OPTION_RELIABLE_MSGS_ONLY,
            ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE | ismENGINE_SUBSCRIPTION_OPTION_UNRELIABLE_MSGS_ONLY};
    uint32_t subSelectOptCount = sizeof(subSelectOpts)/sizeof(subSelectOpts[0]);

    ismEngine_ConsumerHandle_t hConsumer[subSelectOptCount];
    genericMsgCbContext_t ConsumerContext[subSelectOptCount];
    genericMsgCbContext_t *pConsumerContext[subSelectOptCount];

    test_selectionCallback_t expectSelection = {"DONT_EXPECT_A_SELECT_CALL!!"};

    pSelectionExpected = &expectSelection;

    char payload[128];
    void *pPayload = (void *)payload;

    printf("Starting %s...\n", __func__);

    /* Create our clients and sessions */
    rc = test_createClientAndSession(__func__,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    // Create a set of subscriptions which will receive a subset of the msgs
    ismEngine_SubscriptionAttributes_t subAttrs = {0};

    for(uint32_t i=0; i<subSelectOptCount; i++)
    {
        subAttrs.subOptions = subSelectOpts[i];

        hConsumer[i] = NULL;
        memset(&ConsumerContext[i], 0, sizeof(ConsumerContext[i]));
        ConsumerContext[i].hSession = hSession;
        pConsumerContext[i] = &ConsumerContext[i];

        rc = ism_engine_createConsumer(hSession,
                                       ismDESTINATION_TOPIC,
                                       SELECT_RELIABILITY_TOPIC,
                                       &subAttrs,
                                       NULL, // Unused for TOPIC
                                       &pConsumerContext[i],
                                       sizeof(genericMsgCbContext_t *),
                                       genericMessageCallback,
                                       NULL,
                                       ismENGINE_CONSUMER_OPTION_NONE,
                                       &hConsumer[i],
                                       NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hConsumer[i]);
    }

    // Now publish a mix if QoS messages
    for(int32_t i=0; i<messageCount; i++)
    {
        uint8_t reliability;

        if (rand()%100 > 49)
        {
            reliability = ismMESSAGE_RELIABILITY_AT_MOST_ONCE;
            unreliableCount++;
        }
        else if (rand()%100 > 49)
        {
            reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
            reliableCount++;
        }
        else
        {
            reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
            reliableCount++;
        }

        sprintf(payload, "Message - %d. Rel=%u.", i, reliability);

        ismEngine_MessageHandle_t hMessage = NULL;
        test_createMessage(100,
                           ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                           reliability,
                           ismMESSAGE_FLAGS_NONE,
                           0,
                           ismDESTINATION_TOPIC,
                           SELECT_RELIABILITY_TOPIC,
                           &hMessage,
                           &pPayload);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_PTR_NOT_NULL(hMessage);

        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_TOPIC,
                                                SELECT_RELIABILITY_TOPIC,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    TEST_ASSERT_EQUAL(ConsumerContext[0].received, messageCount);
    TEST_ASSERT_EQUAL(ConsumerContext[0].reliable, reliableCount);
    TEST_ASSERT_EQUAL(ConsumerContext[0].unreliable, unreliableCount);

    TEST_ASSERT_EQUAL(ConsumerContext[1].received, reliableCount);
    TEST_ASSERT_EQUAL(ConsumerContext[1].reliable, reliableCount);
    TEST_ASSERT_EQUAL(ConsumerContext[1].unreliable, 0);

    TEST_ASSERT_EQUAL(ConsumerContext[2].received, unreliableCount);
    TEST_ASSERT_EQUAL(ConsumerContext[2].reliable, 0);
    TEST_ASSERT_EQUAL(ConsumerContext[2].unreliable, unreliableCount);

    // MORE...

    for(uint32_t i=0; i<subSelectOptCount; i++)
    {
        rc = ism_engine_destroyConsumer(hConsumer[i], NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    pSelectionExpected = NULL;
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_Selection_SubOnly[] =
{
    { "SingleSubscriberSelectsSome", test_capability_Sub_Selects_Some },
    { "SingleSubscriberSelectsSomeCompiled", test_capability_Sub_Selects_SomeCompiled },
    { "SelectOnReliability", test_capability_Sub_Selects_Reliability },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test Operation of publish with subscribers hitting max messages
//****************************************************************************
char *MAXMESSAGES_TOPICS[] = {"TEST/MAXMESSAGES/TOPIC", "TEST/MAXMESSAGES/+", "TEST/MAXMESSAGES/#"};
#define MAXMESSAGES_NUMTOPICS (sizeof(MAXMESSAGES_TOPICS)/sizeof(MAXMESSAGES_TOPICS[0]))

typedef struct tag_maxMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
} maxMessagesCbContext_t;

bool maxMessagesCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
                         void *                          pContext,
                         ismEngine_DelivererContext_t *  _delivererContext)
{
    maxMessagesCbContext_t *context = *((maxMessagesCbContext_t **)pContext);

    __sync_fetch_and_add(&context->received, 1);

    ism_engine_releaseMessage(hMessage);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        NULL,
                                                        0,
                                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    return true; // more messages, please.
}

void test_capability_MaxMessages(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient1, hClient2;
    ismEngine_SessionHandle_t hSession1, hSession2;
    ismEngine_ProducerHandle_t hProducer;
    ismEngine_ConsumerHandle_t atMostOnceConsumer, atLeastOnceConsumer, exactlyOnceConsumer;
    maxMessagesCbContext_t atMostOnceContext={0}, atLeastOnceContext={0}, exactlyOnceContext={0};
    maxMessagesCbContext_t *cb1=&atMostOnceContext, *cb2=&atLeastOnceContext, *cb3=&exactlyOnceContext;

    printf("Starting %s...\n", __func__);

    /* Create our clients and sessions */
    rc = test_createClientAndSession("CLIENTONE",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient1, &hSession1, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession("CLIENTTWO",
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient2, &hSession2, true);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };

    atMostOnceContext.hSession = hSession1;
    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_TOPIC,
                                   MAXMESSAGES_TOPICS[rand()%MAXMESSAGES_NUMTOPICS],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb1,
                                   sizeof(maxMessagesCbContext_t *),
                                   maxMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &atMostOnceConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(atMostOnceConsumer);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE;
    atLeastOnceContext.hSession = hSession1;
    rc = ism_engine_createConsumer(hSession1,
                                   ismDESTINATION_TOPIC,
                                   MAXMESSAGES_TOPICS[rand()%MAXMESSAGES_NUMTOPICS],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb2,
                                   sizeof(maxMessagesCbContext_t *),
                                   maxMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &atLeastOnceConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(atLeastOnceConsumer);

    subAttrs.subOptions = ismENGINE_SUBSCRIPTION_OPTION_EXACTLY_ONCE;
    exactlyOnceContext.hSession = hSession2;
    rc = ism_engine_createConsumer(hSession2,
                                   ismDESTINATION_TOPIC,
                                   MAXMESSAGES_TOPICS[rand()%MAXMESSAGES_NUMTOPICS],
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb3,
                                   sizeof(maxMessagesCbContext_t *),
                                   maxMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &exactlyOnceConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(exactlyOnceConsumer);

    rc = ism_engine_createProducer(hSession1,
                                   ismDESTINATION_TOPIC,
                                   MAXMESSAGES_TOPICS[0],
                                   &hProducer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hProducer);

    /* Create messages to play with */
    void *payload1="AtMostOnce Message";
    ismEngine_MessageHandle_t atMostOnceMessage;
    void *payload2="AtLeastOnce Message";
    ismEngine_MessageHandle_t atLeastOnceMessage;
    void *payload3="ExactlyOnce Message";
    ismEngine_MessageHandle_t exactlyOnceMessage;

    printf("  ...publish\n");

    int32_t atMostOnceExpect=0, atLeastOnceExpect=0, exactlyOnceExpect=0;

    // Should add 1 to ALL counts
    rc = test_createMessage(strlen((char *)payload1)+1,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, MAXMESSAGES_TOPICS[0],
                            &atMostOnceMessage, &payload1);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSession1, hProducer, NULL, atMostOnceMessage, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    atMostOnceExpect++;
    TEST_ASSERT_EQUAL(atMostOnceContext.received, atMostOnceExpect);
    atLeastOnceExpect++;
    TEST_ASSERT_EQUAL(atLeastOnceContext.received, atLeastOnceExpect);
    exactlyOnceExpect++;
    TEST_ASSERT_EQUAL(exactlyOnceContext.received, exactlyOnceExpect);

    rc = test_createMessage(strlen((char *)payload2)+1,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, MAXMESSAGES_TOPICS[0],
                            &atLeastOnceMessage, &payload2);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSession1, hProducer, NULL, atLeastOnceMessage, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    atMostOnceExpect++;
    TEST_ASSERT_EQUAL(atMostOnceContext.received, atMostOnceExpect);
    atLeastOnceExpect++;
    TEST_ASSERT_EQUAL(atLeastOnceContext.received, atLeastOnceExpect);
    exactlyOnceExpect++;
    TEST_ASSERT_EQUAL(exactlyOnceContext.received, exactlyOnceExpect);

    rc = test_createMessage(strlen((char *)payload2)+1,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, MAXMESSAGES_TOPICS[0],
                            &atLeastOnceMessage, &payload2);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_engine_putMessage(hSession1, hProducer, NULL, atLeastOnceMessage, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    atMostOnceExpect++;
    TEST_ASSERT_EQUAL(atMostOnceContext.received, atMostOnceExpect);
    atLeastOnceExpect++;
    TEST_ASSERT_EQUAL(atLeastOnceContext.received, atLeastOnceExpect);
    exactlyOnceExpect++;
    TEST_ASSERT_EQUAL(exactlyOnceContext.received, exactlyOnceExpect);

    iepiPolicyInfo_t templatePolicyInfo;

    templatePolicyInfo.maxMessageCount = 2;
    templatePolicyInfo.maxMsgBehavior = RejectNewMessages;
    templatePolicyInfo.DCNEnabled = false;

    iepiPolicyInfo_t *fakePolicyInfo = NULL;
    rc = iepi_createPolicyInfo(pThreadData, NULL, ismSEC_POLICY_LAST, false, &templatePolicyInfo, &fakePolicyInfo);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(fakePolicyInfo);
    TEST_ASSERT_EQUAL(fakePolicyInfo->creationState, CreatedByEngine);
    TEST_ASSERT_EQUAL(fakePolicyInfo->maxMessageCount, 2);
    TEST_ASSERT_EQUAL(fakePolicyInfo->maxMessageBytes, 0);

    iepiPolicyInfo_t *defaultPolicyInfo = iepi_getDefaultPolicyInfo(false);

    int32_t consumer;
    for(consumer=0; consumer<6; consumer++)
    {
        switch(consumer%3)
        {
        case 0:
            iepi_acquirePolicyInfoReference(fakePolicyInfo);
            ieq_setPolicyInfo(pThreadData, atMostOnceConsumer->queueHandle, fakePolicyInfo);
            iepi_acquirePolicyInfoReference(defaultPolicyInfo);
            ieq_setPolicyInfo(pThreadData, atLeastOnceConsumer->queueHandle, defaultPolicyInfo);
            iepi_acquirePolicyInfoReference(defaultPolicyInfo);
            ieq_setPolicyInfo(pThreadData, exactlyOnceConsumer->queueHandle, defaultPolicyInfo);
            break;
        case 1:
            iepi_acquirePolicyInfoReference(defaultPolicyInfo);
            ieq_setPolicyInfo(pThreadData, atMostOnceConsumer->queueHandle, defaultPolicyInfo);
            iepi_acquirePolicyInfoReference(fakePolicyInfo);
            ieq_setPolicyInfo(pThreadData, atLeastOnceConsumer->queueHandle, fakePolicyInfo);
            iepi_acquirePolicyInfoReference(defaultPolicyInfo);
            ieq_setPolicyInfo(pThreadData, exactlyOnceConsumer->queueHandle, defaultPolicyInfo);
            break;
        case 2:
            iepi_acquirePolicyInfoReference(defaultPolicyInfo);
            ieq_setPolicyInfo(pThreadData, atMostOnceConsumer->queueHandle, defaultPolicyInfo);
            iepi_acquirePolicyInfoReference(defaultPolicyInfo);
            ieq_setPolicyInfo(pThreadData, atLeastOnceConsumer->queueHandle, defaultPolicyInfo);
            iepi_acquirePolicyInfoReference(fakePolicyInfo);
            ieq_setPolicyInfo(pThreadData, exactlyOnceConsumer->queueHandle, fakePolicyInfo);
            break;
        }

        int32_t msgNo;
        for(msgNo=0; msgNo<3; msgNo++)
        {
            ismEngine_TransactionHandle_t hTran = NULL;
            uint32_t expectedFinalPutRC = OK;
            uint32_t expectedCommitRC = OK;
            ismEngine_MessageHandle_t useMessage;

            if (consumer >= 3)
            {
                rc = sync_ism_engine_createLocalTransaction(hSession1, &hTran);
                TEST_ASSERT_EQUAL(rc, OK);
                TEST_ASSERT_PTR_NOT_NULL(hTran);
            }

            if (consumer%3 == 0) // AT_MOST_ONCE consumer is limited, publish loop continues
            {
                atMostOnceExpect += 2;
                atLeastOnceExpect += 3;
                exactlyOnceExpect += 3;
            }
            else if (msgNo == 0) // AT_MOST_ONCE message, publish loop continues
            {
                atMostOnceExpect += 3;
                atLeastOnceExpect += (consumer%3 == 1 ? 2 : 3);
                exactlyOnceExpect += (consumer%3 == 2 ? 2 : 3);

            }
            else if (consumer >= 4) // AT_LEAST_ONCE/EXACTLY_ONCE consumer limited, transaction rollback only
            {
                expectedFinalPutRC = ISMRC_DestinationFull;
                expectedCommitRC = ISMRC_RolledBack;
            }
            else // AT_LEAST_ONCE / AT_MOST_ONCE consumer limited, no transaction.
            {
                atMostOnceExpect += 2;
                atLeastOnceExpect += 2;
                exactlyOnceExpect += 2;
                expectedFinalPutRC = ISMRC_DestinationFull;
                expectedCommitRC = ISMRC_RolledBack;
            }

            rc = ism_engine_stopMessageDelivery(hSession1, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            rc = ism_engine_stopMessageDelivery(hSession2, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            for(uint32_t i=0; i<3u; i++)
            {
                switch(msgNo)
                {
                case 0:
                    rc = test_createMessage(strlen((char *)payload1)+1,
                                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                            ismMESSAGE_FLAGS_NONE,
                                            0,
                                            ismDESTINATION_TOPIC, MAXMESSAGES_TOPICS[0],
                                            &atMostOnceMessage, &payload1);
                    TEST_ASSERT_EQUAL(rc, OK);

                    useMessage = atMostOnceMessage;
                    break;
                case 1:
                    rc = test_createMessage(strlen((char *)payload2)+1,
                                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                            ismMESSAGE_FLAGS_NONE,
                                            0,
                                            ismDESTINATION_TOPIC, MAXMESSAGES_TOPICS[0],
                                            &atLeastOnceMessage, &payload2);
                    TEST_ASSERT_EQUAL(rc, OK);

                    useMessage = atLeastOnceMessage;
                    break;
                case 2:
                    rc = test_createMessage(strlen((char *)payload3)+1,
                                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                            ismMESSAGE_RELIABILITY_EXACTLY_ONCE,
                                            ismMESSAGE_FLAGS_NONE,
                                            0,
                                            ismDESTINATION_TOPIC, MAXMESSAGES_TOPICS[0],
                                            &exactlyOnceMessage, &payload3);
                    TEST_ASSERT_EQUAL(rc, OK);

                    useMessage = exactlyOnceMessage;
                    break;
                }

                //printf("%d useMessage=%p Reliability=%d\n", msgNo, useMessage, (ismEngine_Message_t *)(useMessage)->Header.Reliability);

                rc = ism_engine_putMessage(hSession1, hProducer, hTran, useMessage, NULL, 0, NULL);
                if (i<2)
                {
                    TEST_ASSERT_EQUAL(rc, OK);
                }
                else
                {
                    if (expectedFinalPutRC == OK)
                    {
                        TEST_ASSERT(rc == OK || rc == ISMRC_SomeDestinationsFull, ("Unexpected rc=%d", rc));
                    }
                    else
                    {
                        TEST_ASSERT_EQUAL(rc, expectedFinalPutRC);
                    }
                }
            }

            if (hTran != NULL)
            {
                rc = sync_ism_engine_commitTransaction(hSession1,
                                                       hTran,
                                                       ismENGINE_COMMIT_TRANSACTION_OPTION_DEFAULT);
                TEST_ASSERT_EQUAL(rc, expectedCommitRC);
            }

            rc = ism_engine_startMessageDelivery(hSession1, ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);
            rc = ism_engine_startMessageDelivery(hSession2, ismENGINE_START_DELIVERY_OPTION_NONE, NULL, 0, NULL);
            TEST_ASSERT_EQUAL(rc, OK);

            //printf("atMostOnce %d %d msgNo=%d consumer=%d\n", atMostOnceContext.received, atMostOnceExpect, msgNo, consumer);
            TEST_ASSERT_EQUAL(atMostOnceContext.received, atMostOnceExpect);
            //printf("atLeastOnce %d %d msgNo=%d consumer=%d\n", atLeastOnceContext.received, atLeastOnceExpect, msgNo, consumer);
            TEST_ASSERT_EQUAL(atLeastOnceContext.received, atLeastOnceExpect);
            //printf("exactlyOnce %d %d msgNo=%d consumer=%d\n", exactlyOnceContext.received, exactlyOnceExpect, msgNo, consumer);
            TEST_ASSERT_EQUAL(exactlyOnceContext.received, exactlyOnceExpect);
        }
    }

    iepi_releasePolicyInfo(pThreadData, fakePolicyInfo);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient2, hSession2, false);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient1, hSession1, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

void resetDroppedMsgs(ieutThreadData_t *pThreadData, void *context)
{
    ismEngine_serverGlobal.endedThreadStats.droppedMsgCount = 0;
    pThreadData->stats.droppedMsgCount = 0;
}

void test_capability_DroppedCount(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    maxMessagesCbContext_t Context={0};
    maxMessagesCbContext_t *cb=&Context;

    printf("Starting %s...\n", __func__);

    // Dodgy reset of dropped message count
    ieut_enumerateThreadData(resetDroppedMsgs, NULL);

    rc = test_createClientAndSession(__func__,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "TEST/DROPPEDCOUNT",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   &cb,
                                   sizeof(maxMessagesCbContext_t *),
                                   maxMessagesCallback,
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    iepiPolicyInfo_t templatePolicyInfo;

    templatePolicyInfo.maxMessageCount = 1;
    templatePolicyInfo.maxMsgBehavior = RejectNewMessages;
    templatePolicyInfo.DCNEnabled = false;

    iepiPolicyInfo_t *fakePolicyInfo = NULL;
    rc = iepi_createPolicyInfo(pThreadData, "fakePolicy", ismSEC_POLICY_QUEUE, false, &templatePolicyInfo, &fakePolicyInfo);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(fakePolicyInfo);
    TEST_ASSERT_EQUAL(fakePolicyInfo->creationState, CreatedByEngine);

    ieq_setPolicyInfo(pThreadData, hConsumer->queueHandle, fakePolicyInfo);

    printf("  ...publish\n");

    for(int32_t i=0; i<10; i++)
    {
        void *payload="TestMessage";
        ismEngine_MessageHandle_t hMessage;

        rc = test_createMessage(strlen((char *)payload)+1,
                                ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                ismMESSAGE_FLAGS_NONE,
                                0,
                                ismDESTINATION_TOPIC, "TEST/DROPPEDCOUNT",
                                &hMessage, &payload);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = ism_engine_putMessageInternalOnDestination(ismDESTINATION_TOPIC,
                                                        "TEST/DROPPEDCOUNT",
                                                        hMessage,
                                                        NULL, 0, NULL);
        TEST_ASSERT_EQUAL(rc, OK);

        ismEngine_MessagingStatistics_t msgStats;
        ism_engine_getMessagingStatistics(&msgStats);
        TEST_ASSERT_NOT_EQUAL(msgStats.ServerShutdownTime, 0);
        TEST_ASSERT_EQUAL(msgStats.DroppedMessages, i);
    }

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_TopicTree_CUnit_test_capability_MaxMessages[] =
{
    { "MaxMessages", test_capability_MaxMessages },
    { "DroppedCount", test_capability_DroppedCount },
    CU_TEST_INFO_NULL
};

void test_defect_24507(void)
{
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;

    printf("Starting %s...\n", __func__);

    uint32_t rc;

    /* Create our clients and sessions */
    rc = test_createClientAndSession(__func__,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_MOST_ONCE };
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_TOPIC,
                                   "A/B",
                                   &subAttrs,
                                   NULL, // Unused for TOPIC
                                   NULL,
                                   0,
                                   NULL, /* No delivery callback */
                                   NULL,
                                   ismENGINE_CONSUMER_OPTION_NONE,
                                   &hConsumer,
                                   NULL,
                                   0,
                                   NULL);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    /* Create messages to play with */
    void *payload="MESSAGE";
    ismEngine_MessageHandle_t hMessage;

    rc = test_createMessage(strlen((char *)payload)+1,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, "A/B",
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);

    // Fill up the sublist cache capacity
    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            "A/B",
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createMessage(strlen((char *)payload)+1,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, "X/Y",
                            &hMessage, &payload);
    TEST_ASSERT_EQUAL(rc, OK);

    // Try and publish to 'no-one' - this defect meant it would fail with a seg fault.
    rc = ism_engine_putMessageOnDestination(hSession,
                                            ismDESTINATION_TOPIC,
                                            "X/Y",
                                            NULL,
                                            hMessage,
                                            NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroyClientAndSession(hClient, hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);
}

CU_TestInfo ISM_TopicTree_CUnit_capability_SmallCache[] =
{
    { "Defect24507", test_defect_24507 },
    { "Wildcards", test_capability_Wildcards },
    { "NoLocal", test_capability_NoLocal },
    { "EmptyNolocal", test_capability_EmptyNoLocal },
    CU_TEST_INFO_NULL
};

typedef struct tag_NDSubsMessagesCbContext_t
{
    ismEngine_SessionHandle_t hSession;
    int32_t received;
} NDSubsMessagesCbContext_t;

bool NDSubsMessagesCallback(ismEngine_ConsumerHandle_t      hConsumer,
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
                            void *                          pContext,
                            ismEngine_DelivererContext_t *  _delivererContext)
{
    NDSubsMessagesCbContext_t *context = *((NDSubsMessagesCbContext_t **)pContext);

    __sync_fetch_and_add(&context->received, 1);

    ism_engine_releaseMessage(hMessage);

    if (ismENGINE_NULL_DELIVERY_HANDLE != hDelivery)
    {
        uint32_t rc = ism_engine_confirmMessageDelivery(context->hSession,
                                                        NULL,
                                                        hDelivery,
                                                        ismENGINE_CONFIRM_OPTION_CONSUMED,
                                                        NULL,
                                                        0,
                                                        NULL);
        TEST_ASSERT_EQUAL(rc, OK);
    }

    return true; // more messages, please.
}

//****************************************************************************
/// @brief Test the use of non-durable subscriptions
//****************************************************************************
#define NONDURABLE_SUBSCRIPTIONS_TEST_TOPIC   "NonDurable/Topic"
#define NONDURABLE_SUBSCRIPTIONS_TEST_SUBNAME "NonDurable Subscription 1"
#define NONDURABLE_TOPICSECURITY_POLICYNAME   "NonDurableTopicSecurityPolicy"

typedef struct tag_listSubsCbContext_t
{
    int32_t  foundSubs;
    char    *expectedTopic; // Optional topic string to check
} listSubsCbContext_t;

void listSubsCallback(ismEngine_SubscriptionHandle_t subHandle,
                      const char * subName,
                      const char * pTopicString,
                      void * subProperties,
                      size_t   subPropertiesLength,
                      const ismEngine_SubscriptionAttributes_t *pSubAttrs,
                      uint32_t consumerCount,
                      void *   pContext)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    listSubsCbContext_t *context = (listSubsCbContext_t *)pContext;
    ismEngine_Subscription_t *subscription = (ismEngine_Subscription_t *)subHandle;

    TEST_ASSERT_PTR_NOT_NULL(subscription);
    TEST_ASSERT_NOT_EQUAL(subscription->useCount, 0);

    if (context->expectedTopic != NULL)
    {
        TEST_ASSERT_STRINGS_EQUAL(pTopicString, context->expectedTopic);
        TEST_ASSERT_STRINGS_EQUAL(subscription->node->topicString, context->expectedTopic);
    }

    // Get and release this subscription, checking use counts are updated as expected
    uint32_t oldUseCount = subscription->useCount;
    ismEngine_Subscription_t *gotSubscription;

    int32_t rc = iett_findClientSubscription(pThreadData,
                                             subscription->clientId,
                                             subscription->clientIdHash,
                                             subName,
                                             iett_generateSubNameHash(subName),
                                             &gotSubscription);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(subscription, gotSubscription);
    TEST_ASSERT_EQUAL(oldUseCount+1, gotSubscription->useCount);

    iett_acquireSubscriptionReference(gotSubscription);

    TEST_ASSERT_EQUAL(oldUseCount+2, gotSubscription->useCount);

    iett_releaseSubscription(pThreadData, gotSubscription, false);

    TEST_ASSERT_EQUAL(oldUseCount+1, gotSubscription->useCount);

    iett_releaseSubscription(pThreadData, gotSubscription, false);

    TEST_ASSERT_EQUAL(subscription->useCount, oldUseCount);

    context->foundSubs++;

#if 0
    printf("SUBHANDLE: %p\n", subHandle);
    printf("SUBNAME: '%s'\n", subName);
    printf("TOPICSTRING: '%s'\n", pTopicString);
    printf("SUBPROPERTIES: %p\n", subProperties);
    printf("SUBPROPERTIESLENGTH: %ld\n", subPropertiesLength);
    printf("SUBOPTIONS: 0x%08x\n", pSubAttrs->subOptions);
    printf("CONSUMERCOUNT: %u\n", consumerCount);
    printf("\n");
#endif
}

void test_capability_CreateSubscription(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    uint32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;
    ismEngine_ConsumerHandle_t    hConsumer;

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

    char *clientID = "TestClient";

    mockTransport->clientID = clientID;

    printf("Starting %s...\n", __func__);

    rc = test_createClientAndSession(clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    // Look for a specific subscription, that should not exist yet
    listSubsCbContext_t context = {0};
    context.foundSubs = 0;
    rc = ism_engine_listSubscriptions(hClient,
                                      "Nondurable Subscription 1",
                                      &context, listSubsCallback);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(context.foundSubs, 0);

    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

    // Check not authorized.
    rc = ism_engine_createSubscription(hClient,
                                       NONDURABLE_SUBSCRIPTIONS_TEST_SUBNAME,
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       NONDURABLE_SUBSCRIPTIONS_TEST_TOPIC,
                                       &subAttrs,
                                       NULL,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotAuthorized);

    // Add a messaging policy that allows publish / subscribe on 'NonDurable/*'
    // For clients whose ID is 'TestClient'.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."NONDURABLE_TOPICSECURITY_POLICYNAME"\","
                                "\"Name\":\""NONDURABLE_TOPICSECURITY_POLICYNAME"\","
                                "\"ClientID\":\"TestClient\","
                                "\"Topic\":\"NonDurable/*\","
                                "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, NONDURABLE_TOPICSECURITY_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    // Should now be authorized
    rc = ism_engine_createSubscription(hClient,
                                       NONDURABLE_SUBSCRIPTIONS_TEST_SUBNAME,
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       NONDURABLE_SUBSCRIPTIONS_TEST_TOPIC,
                                       &subAttrs,
                                       NULL,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ismEngine_Subscription_t *tmpSubscription = NULL;
    rc = iett_findClientSubscription(pThreadData,
                                     ((ismEngine_ClientState_t *)hClient)->pClientId,
                                     iett_generateClientIdHash(((ismEngine_ClientState_t *)hClient)->pClientId),
                                     NONDURABLE_SUBSCRIPTIONS_TEST_SUBNAME,
                                     iett_generateSubNameHash(NONDURABLE_SUBSCRIPTIONS_TEST_SUBNAME),
                                     &tmpSubscription);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(tmpSubscription);
    TEST_ASSERT_STRINGS_EQUAL(tmpSubscription->subName, NONDURABLE_SUBSCRIPTIONS_TEST_SUBNAME);
    TEST_ASSERT_STRINGS_EQUAL(tmpSubscription->clientId, clientID);
    TEST_ASSERT_NOT_EQUAL(ieq_getQType(tmpSubscription->queueHandle), multiConsumer);

    const char **foundClients = NULL;
    rc = iett_findSubscriptionClientIds(pThreadData, tmpSubscription, &foundClients);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(foundClients);
    TEST_ASSERT_STRINGS_EQUAL(foundClients[0], clientID);
    TEST_ASSERT_PTR_NULL(foundClients[1]);
    iett_freeSubscriptionClientIds(pThreadData, foundClients);

    rc = iett_releaseSubscription(pThreadData, tmpSubscription, false);
    TEST_ASSERT_EQUAL(rc, OK);

    NDSubsMessagesCbContext_t ConsumerContext={0};
    NDSubsMessagesCbContext_t *cb1=&ConsumerContext;

    ConsumerContext.hSession = hSession;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   NONDURABLE_SUBSCRIPTIONS_TEST_SUBNAME,
                                   NULL,  /* NOT_USED */
                                   NULL,
                                   &cb1,
                                   sizeof(NDSubsMessagesCbContext_t *),
                                   NDSubsMessagesCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    // Publish some messages to see they get received
    test_publishMessages(NONDURABLE_SUBSCRIPTIONS_TEST_TOPIC, 10, 0, 0);
    TEST_ASSERT_EQUAL(ConsumerContext.received, 10);

    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL,
                                       0,
                                       NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // Publish more messages - shouldn't be received (consumer should have gone along with sub)
    test_publishMessages(NONDURABLE_SUBSCRIPTIONS_TEST_TOPIC, 10, 0, 0);
    TEST_ASSERT_EQUAL(ConsumerContext.received, 10);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(pThreadData,
                                     clientID,
                                     iett_generateClientIdHash(clientID),
                                     "Nondurable Subscription 1",
                                     iett_generateSubNameHash("Nondurable Subscription 1"),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
}

//****************************************************************************
/// @brief Test non-durable sub with selection works correctly
//****************************************************************************
#define NONDURABLE_SUB_SELECTION_TEST_TOPIC   "NonDurable/Topic/Selection"
#define NONDURABLE_SUB_SELECTION_POLICYNAME   "NonDurableSubSelTopicPolicy"
#define NONDURABLE_SUB_SELECTION_TEST_SUBNAME "NonDurable_Sub_COLOUR_Selection"

void test_capability_NondurableSubSelection(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;
    ismEngine_ConsumerHandle_t hConsumer;
    genericMsgCbContext_t ConsumerContext, *pConsumerContext = &ConsumerContext;
    char payload[128];

    printf("Starting %s...\n", __func__);

    /* Create our clients and sessions */
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

    char *clientID = "TestClientSelection";

    mockTransport->clientID = clientID;

    // Add a messaging policy that allows publish / subscribe on 'NonDurable/*'
    // For clients whose ID is 'TestClient'.
    rc = test_addSecurityPolicy(ismENGINE_ADMIN_VALUE_TOPICPOLICY,
                                "{\"UID\":\"UID."NONDURABLE_SUB_SELECTION_POLICYNAME"\","
                                "\"Name\":\""NONDURABLE_SUB_SELECTION_POLICYNAME"\","
                                "\"ClientID\":\"TestClient*\","
                                "\"Topic\":\"NonDurable/*\","
                                "\"ActionList\":\"subscribe,publish\"}");
    TEST_ASSERT_EQUAL(rc, OK);

    // Associate the policy with security contexts
    rc = test_addPolicyToSecContext(mockContext, NONDURABLE_SUB_SELECTION_POLICYNAME);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_createClientAndSession(clientID,
                                     mockContext,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    ConsumerContext.hSession = hSession;
    ConsumerContext.received = 0;

    printf("  ...create\n");

    // Initiliase the properties which contain the selection string
    ism_prop_t *properties = ism_common_newProperties(1);
    TEST_ASSERT_PTR_NOT_NULL(properties);

    char *selectorString = "Colour='BLUE'";
    ism_field_t SelectorField = {VT_String, 0, {.s = selectorString }};
    rc = ism_common_setProperty(properties, ismENGINE_PROPERTY_SELECTOR, &SelectorField);
    TEST_ASSERT_EQUAL(rc, OK);

    // Now create the subscription
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE |
                                                    ismENGINE_SUBSCRIPTION_OPTION_MESSAGE_SELECTION };

    rc = ism_engine_createSubscription(hClient,
                                       NONDURABLE_SUB_SELECTION_TEST_SUBNAME,
                                       properties,
                                       ismDESTINATION_TOPIC,
                                       NONDURABLE_SUB_SELECTION_TEST_TOPIC,
                                       &subAttrs,
                                       NULL,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And free the properties we allocated
    ism_common_freeProperties(properties);

    ConsumerContext.hSession = hSession;
    rc = ism_engine_createConsumer(hSession,
                                   ismDESTINATION_SUBSCRIPTION,
                                   NONDURABLE_SUB_SELECTION_TEST_SUBNAME,
                                   NULL,  /* NOT_USED */
                                   NULL,
                                   &pConsumerContext,
                                   sizeof(genericMsgCbContext_t *),
                                   genericMessageCallback,
                                   NULL,
                                   test_getDefaultConsumerOptions(subAttrs.subOptions),
                                   &hConsumer,
                                   NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hConsumer);

    // Now publish 10 matching and 10 non-matching messages
    for(int32_t j=0; j<2; j++)
    {
        properties = ism_common_newProperties(1);
        TEST_ASSERT_PTR_NOT_NULL(properties);

        char *ColourText= j == 0 ? "BLUE" : "RED";
        ism_field_t ColourField = {VT_String, 0, {.s = ColourText }};
        rc = ism_common_setProperty(properties, "Colour", &ColourField);
        TEST_ASSERT_EQUAL(rc, OK);

        for(int32_t i=0; i<10; i++)
        {
            ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
            ismMessageAreaType_t areaTypes[2];
            size_t areaLengths[2];
            void *areas[2];
            concat_alloc_t FlatProperties = { NULL };
            char localPropBuffer[1024];

            FlatProperties.buf = localPropBuffer;
            FlatProperties.len = 1024;

            rc = ism_common_serializeProperties(properties, &FlatProperties);
            TEST_ASSERT_EQUAL(rc, OK);

            sprintf(payload, "Message - %d. Property is '%s'", i, ColourText);

            areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
            areaLengths[0] = FlatProperties.used;
            areas[0] = FlatProperties.buf;
            areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
            areaLengths[1] = strlen(payload) +1;
            areas[1] = (void *)payload;

            header.Persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
            header.Reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;

            ismEngine_MessageHandle_t hMessage = NULL;
            rc = ism_engine_createMessage(&header,
                                          2,
                                          areaTypes,
                                          areaLengths,
                                          areas,
                                          &hMessage);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_PTR_NOT_NULL(hMessage);

            rc = ism_engine_putMessageOnDestination(hSession,
                                                    ismDESTINATION_TOPIC,
                                                    NONDURABLE_SUB_SELECTION_TEST_TOPIC,
                                                    NULL,
                                                    hMessage,
                                                    NULL, 0, NULL);
            if (j == 0)
            {
                TEST_ASSERT_EQUAL(rc, OK);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_NoMatchingDestinations);
            }
        }

        // And free the properties we allocated
        ism_common_freeProperties(properties);
    }

    rc = ism_engine_destroyConsumer(hConsumer, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    // And verify that all of the messages have been received
    TEST_ASSERT_EQUAL(ConsumerContext.received, 10);

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = test_destroySecurityContext(mockListener,
                                     mockTransport,
                                     mockContext);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(ieut_getThreadData(),
                                     clientID,
                                     iett_generateClientIdHash(clientID),
                                     NONDURABLE_SUB_SELECTION_TEST_SUBNAME,
                                     iett_generateSubNameHash(NONDURABLE_SUB_SELECTION_TEST_SUBNAME),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
}

CU_TestInfo ISM_TopicTree_CUnit_capability_NonDurableSubs[] =
{
    { "CreateSubscription", test_capability_CreateSubscription },
    { "NondurableSubSelection", test_capability_NondurableSubSelection },
    CU_TEST_INFO_NULL
};

//****************************************************************************
/// @brief Test behaviour of client subscription lists
//****************************************************************************
#define testSUBNUMBER_TO_SUBNAMEHASH(_startHash, _subNumber) (_startHash) + (((_subNumber)%5)*10);
void test_capability_ClientSubList(void)
{
    uint32_t rc;
    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t hSession;

    printf("Starting %s...\n", __func__);

    char *clientId = "CSL_CLIENT";
    uint32_t clientIdHash = iett_generateClientIdHash(clientId);

    /* Create our clients and sessions */
    rc = test_createClientAndSession(clientId,
                                     NULL,
                                     ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                     ismENGINE_CREATE_SESSION_OPTION_NONE,
                                     &hClient, &hSession, false);
    TEST_ASSERT_EQUAL(rc, OK);

    printf("  ...create\n");

    // Now create a subscription legitimately
    ismEngine_SubscriptionAttributes_t subAttrs = { ismENGINE_SUBSCRIPTION_OPTION_AT_LEAST_ONCE };

    rc = ism_engine_createSubscription(hClient,
                                       "RealSubscription",
                                       NULL,
                                       ismDESTINATION_TOPIC,
                                       "/TEST/CLIENTSUBLIST",
                                       &subAttrs,
                                       NULL,
                                       NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    ismEngine_Subscription_t *tmpSubscription = NULL;
    rc = iett_findClientSubscription(pThreadData,
                                     clientId,
                                     clientIdHash,
                                     "RealSubscription",
                                     iett_generateSubNameHash("RealSubscription"),
                                     &tmpSubscription);
    TEST_ASSERT_EQUAL(rc, OK);

    uint32_t startHash = tmpSubscription->subNameHash;
    if (startHash > 10) startHash -= 10;

    iett_releaseSubscription(pThreadData, tmpSubscription, true);

    char subName[100];
    uint32_t subNameHash;

    // Check this the list has the expected number of entries in it etc
    iettTopicTree_t *tree = ismEngine_serverGlobal.maintree;

    iettClientSubscriptionList_t *clientNamedSubs = NULL;

    // Get the existing list of named subscriptions for this client.
    rc = ieut_getHashEntry(tree->namedSubs,
                           clientId,
                           clientIdHash,
                           (void **)&clientNamedSubs);

    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(clientNamedSubs->count, 1);

    // Check that the subscriptions are ordered as expected
    uint32_t prevSubNameHash = 0;

    iereResourceSetHandle_t resourceSet = iecs_getResourceSet(pThreadData, clientId, PROTOCOL_ID_MQTT, iereOP_ADD);

    // Add some butchered subscriptions to this node...
    for(int32_t i=0; i<50; i++)
    {
        ismEngine_Subscription_t *newSub;
        size_t clientIdLength;
        size_t subNameLength;
        size_t propertiesLength;

        sprintf(subName, "SUB_%05u", i);

        rc = iett_allocateSubscription(pThreadData,
                                       clientId,
                                       &clientIdLength,
                                       subName,
                                       &subNameLength,
                                       NULL,
                                       &propertiesLength,
                                       &subAttrs,
                                       iettSUBATTR_NONE,
                                       resourceSet,
                                       &newSub);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(newSub->subNameHash, iett_generateSubNameHash(subName));
        TEST_ASSERT_EQUAL(newSub->clientIdHash, clientIdHash);

        // Set a subNameHash to force adding to different parts of the array
        subNameHash = newSub->subNameHash = testSUBNUMBER_TO_SUBNAMEHASH(startHash, i);

        rc = iett_addSubscription(pThreadData,
                                  newSub,
                                  clientId,
                                  clientIdHash);
        TEST_ASSERT_EQUAL(rc, OK);

        rc = iett_findClientSubscription(pThreadData,
                                         clientId,
                                         clientIdHash,
                                         newSub->subName,
                                         newSub->subNameHash,
                                         &tmpSubscription);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(tmpSubscription, newSub);

        iett_releaseSubscription(pThreadData, tmpSubscription, true);

        // Check the ordering..
        prevSubNameHash = 0;
        for(uint32_t j=0; j<clientNamedSubs->count; j++)
        {
            // Output the list at the end of the loop
            if (i == 49)
            {
                //printf("%u: HASH=%u SUBNAME=%s\n", j,
                //       clientNamedSubs->list[j].subNameHash,
                //       clientNamedSubs->list[j].subscription->subName);
            }

            TEST_ASSERT_GREATER_THAN_OR_EQUAL(clientNamedSubs->list[j].subNameHash, prevSubNameHash);
            prevSubNameHash = clientNamedSubs->list[j].subNameHash;
        }
    }

    TEST_ASSERT_EQUAL(clientNamedSubs->count, 51);

    // Force looking all the way to the end...
    rc = iett_findClientSubscription(pThreadData,
                                     clientId,
                                     clientIdHash,
                                     "NOT_THERE",
                                     subNameHash,
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    // And beginning of the list
    subNameHash = startHash;
    rc = iett_findClientSubscription(pThreadData,
                                     clientId,
                                     clientIdHash,
                                     "NOT_THERE",
                                     subNameHash,
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);

    int32_t subsToRemove[] = {60, 49, 49, 48, 0, 44, 21, 16, 14};
    int32_t subsToCheck[] = {47, 1, 63, 15, 43};

    for(int32_t i=0; i<sizeof(subsToRemove)/sizeof(subsToRemove[0]); i++)
    {
        sprintf(subName, "SUB_%05u", subsToRemove[i]);
        subNameHash = testSUBNUMBER_TO_SUBNAMEHASH(startHash, subsToRemove[i]);

        rc = iett_findClientSubscription(pThreadData,
                                         clientId,
                                         clientIdHash,
                                         subName,
                                         subNameHash,
                                         &tmpSubscription);

        if (subsToRemove[i] < 50 && i != 2)
        {
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_STRINGS_EQUAL(subName, tmpSubscription->subName);

            iett_releaseSubscription(pThreadData, tmpSubscription, true);

            iett_removeSubscription(pThreadData,
                                    tmpSubscription,
                                    clientId,
                                    clientIdHash);
        }
        else
        {
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        }

        for(int32_t j=0; j<sizeof(subsToCheck)/sizeof(subsToCheck[0]); j++)
        {
            sprintf(subName, "SUB_%05u", subsToCheck[j]);
            subNameHash = testSUBNUMBER_TO_SUBNAMEHASH(startHash, subsToCheck[j]);

            rc = iett_findClientSubscription(pThreadData,
                                             clientId,
                                             clientIdHash,
                                             subName,
                                             subNameHash,
                                             &tmpSubscription);
            if (subsToCheck[j] < 50)
            {
                TEST_ASSERT_EQUAL(rc, OK);
                TEST_ASSERT_STRINGS_EQUAL(subName, tmpSubscription->subName);

                iett_releaseSubscription(pThreadData, tmpSubscription, true);
            }
            else
            {
                TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
            }
        }

        // Recheck the ordering..
        prevSubNameHash = 0;
        for(uint32_t j=0; j<clientNamedSubs->count; j++)
        {
            TEST_ASSERT_GREATER_THAN_OR_EQUAL(clientNamedSubs->list[j].subNameHash, prevSubNameHash);
            prevSubNameHash = clientNamedSubs->list[j].subNameHash;
        }

        TEST_ASSERT_EQUAL(clientNamedSubs->list[clientNamedSubs->count].subscription, NULL);
    }

    for(int32_t i=0; i<50; i++)
    {
        bool doRemove = true;

        for(int32_t j=0; j<sizeof(subsToRemove)/sizeof(subsToRemove[0]); j++)
        {
            if (i == subsToRemove[j])
            {
                doRemove = false;
                break;
            }
        }

        if (doRemove)
        {
            sprintf(subName, "SUB_%05u", i);
            subNameHash = testSUBNUMBER_TO_SUBNAMEHASH(startHash, i);

            rc = iett_findClientSubscription(pThreadData,
                                             clientId,
                                             clientIdHash,
                                             subName,
                                             subNameHash,
                                             &tmpSubscription);
            TEST_ASSERT_EQUAL(rc, OK);
            TEST_ASSERT_STRINGS_EQUAL(subName, tmpSubscription->subName);

            iett_releaseSubscription(pThreadData, tmpSubscription, true);

            iett_removeSubscription(pThreadData,
                                    tmpSubscription,
                                    clientId,
                                    clientIdHash);

            rc = iett_findClientSubscription(pThreadData,
                                             clientId,
                                             clientIdHash,
                                             subName,
                                             subNameHash,
                                             NULL);
            TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
        }

        TEST_ASSERT_EQUAL(clientNamedSubs->list[clientNamedSubs->count].subscription, NULL);
    }

    printf("  ...disconnect\n");

    rc = test_destroyClientAndSession(hClient, hSession, true);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = iett_findClientSubscription(ieut_getThreadData(),
                                     clientId,
                                     clientIdHash,
                                     "RealSubscription",
                                     iett_generateSubNameHash("RealSubscription"),
                                     NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_NotFound);
}

CU_TestInfo ISM_TopicTree_CUnit_capability_ClientSubList[] =
{
    { "ClientSubList", test_capability_ClientSubList },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/
int initTopicTree(void)
{
    return test_engineInit_DEFAULT;
}

int initTopicTreeSmallCache(void)
{
    return test_engineInit(true, true, true, false /*recovery should complete ok*/, 1, testDEFAULT_STORE_SIZE);
}

int initTopicTreeNoCache(void)
{
    return test_engineInit(true, true, true, false /*recovery should complete ok*/, 0, testDEFAULT_STORE_SIZE);
}

int initTopicTreeSecurity(void)
{
    return test_engineInit(true, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termTopicTree(void)
{
    return test_engineTerm(true);
}

void initSelection(void)
{
    OriginalSelectionCallback = ismEngine_serverGlobal.selectionFn;
    ismEngine_serverGlobal.selectionFn = test_selectionCallback;
}

int initTopicTreeSelection(void)
{
    initSelection();
    return test_engineInit_DEFAULT;
}

int initTopicTreeSecuritySelection(void)
{
    initSelection();
    return test_engineInit(true, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}

int termTopicTreeSelection(void)
{
    ismEngine_serverGlobal.selectionFn = OriginalSelectionCallback;
    return test_engineTerm(true);
}

CU_SuiteInfo ISM_TopicTree_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("iett_insertOrFindSubsNode", initTopicTree, termTopicTree, ISM_TopicTree_CUnit_test_func_iett_insertOrFindSubsNode),
    IMA_TEST_SUITE("iett_analyseTopicString", initTopicTree, termTopicTree, ISM_TopicTree_CUnit_test_func_iett_analyseTopicString),
    IMA_TEST_SUITE("TopicMatching", initTopicTree, termTopicTree, ISM_TopicTree_CUnit_test_capability_TopicMatching),
    IMA_TEST_SUITE("HashTables", initTopicTree, termTopicTree, ISM_TopicTree_CUnit_test_capability_HashTables),
    IMA_TEST_SUITE("AddRemoveSubscribers", initTopicTree, termTopicTree, ISM_TopicTree_CUnit_test_capability_AddRemoveSubscribers),
    IMA_TEST_SUITE("WideNode", initTopicTree, termTopicTree, ISM_TopicTree_CUnit_test_capability_WideNode),
    IMA_TEST_SUITE("NoLocal", initTopicTree, termTopicTree, ISM_TopicTree_CUnit_test_capability_NoLocal),
    IMA_TEST_SUITE("MaxMessages", initTopicTree, termTopicTree, ISM_TopicTree_CUnit_test_capability_MaxMessages),
    IMA_TEST_SUITE("Selection (Sub and Policy)", initTopicTreeSecuritySelection, termTopicTreeSelection, ISM_TopicTree_CUnit_test_capability_Selection_SubAndPolicy),
    IMA_TEST_SUITE("Selection (Sub only)", initTopicTreeSelection, termTopicTreeSelection, ISM_TopicTree_CUnit_test_capability_Selection_SubOnly),
    IMA_TEST_SUITE("SmallCache", initTopicTreeSmallCache, termTopicTree, ISM_TopicTree_CUnit_capability_SmallCache),
    IMA_TEST_SUITE("NoCache",  initTopicTreeNoCache, termTopicTree, ISM_TopicTree_CUnit_capability_SmallCache),
    IMA_TEST_SUITE("NonDurableSubscription", initTopicTreeSecurity, termTopicTree, ISM_TopicTree_CUnit_capability_NonDurableSubs),
    IMA_TEST_SUITE("ClientSubscriptionList", initTopicTree, termTopicTree, ISM_TopicTree_CUnit_capability_ClientSubList),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_TopicTree_CUnit_phase1suites };

int main(int argc, char *argv[])
{
    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
