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
/* Module Name: testTopicTreeScenarios                               */
/*                                                                   */
/* Description: UnitTest to emulate some real-customer scenarios     */
/*                                                                   */
/*********************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ismutil.h"
#include "engineInternal.h"
#include "engine.h"
#include "policyInfo.h"
#include "testPubSubThreads.h"
#include "topicTree.h"
#include "remoteServers.h"
#include "queueCommon.h"
#include "resourceSetStats.h"

#include "test_utils_initterm.h"
#include "test_utils_assert.h"
#include "test_utils_options.h"
#include "test_utils_message.h"
#include "test_utils_sync.h"

typedef int (*scenarioFunc)(int32_t);

bool     subsWait = true;
bool     cpuPin = false;
int32_t  bgSubberDelay = -1;
char    *workloads = NULL;
char    *monitorTopic = NULL;

int64_t  totalExpectedPubMsgs = 0;
int64_t  totalExpectedSubMsgs = 0;
int64_t  totalBackgroundSubMsgs = 0;

// Override ism_cluster_routeLookup for unit tests with invalid remote servers.
int32_t ism_cluster_routeLookup(ismCluster_LookupInfo_t *pLookupInfo)
{
    int32_t rc = OK;

    static int32_t (*real_ism_cluster_routeLookup)(ismCluster_LookupInfo_t *) = NULL;

    if (real_ism_cluster_routeLookup == NULL)
    {
        real_ism_cluster_routeLookup = dlsym(RTLD_NEXT, "ism_cluster_routeLookup");
    }

    bool invoke_cluster_component = true;

    // Check if we should really invoke the cluster component
    if (pLookupInfo != NULL)
    {
        for(int32_t i=0; i<pLookupInfo->numDests; i++)
        {
            // If this is not a real cluster handle, we don't want to call the cluster component
            if (pLookupInfo->phMatchedServers[i] == NULL)
            {
                invoke_cluster_component = false;
                break;
            }
        }
    }

    if (invoke_cluster_component && real_ism_cluster_routeLookup)
    {
        rc = real_ism_cluster_routeLookup(pLookupInfo);
    }

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Topics_match                                      */
/*                                                                   */
/* Description:    Whether two topics match or not                   */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: Check whether two topic strings match (one wildcarded)  */
/*                                                                   */
/* Input:    char *wildTopic - wildcarded topic string               */
/*           char *topic     - topic string                          */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Boolean true if the topics match                        */
/*                                                                   */
/* Exit Normal:       See above                                      */
/*                                                                   */
/* Exit Error:        See above                                      */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
bool Topics_match(char* wildTopic, char* topic)
{
    bool rc = false;
    bool freetopics = false;
    char *last1 = NULL, *last2 = NULL;
    char *pwild = NULL, *pmatch = NULL;

    /* Hash or empty string matches anything... */
    if (strcmp(wildTopic, "#") == 0 || strcmp(wildTopic, topic) == 0)
    {
        rc = true;
        goto exit;
    }

    /* Special case for /# matches anything starting with / */
    if (strcmp(wildTopic, "/#") == 0)
    {
        rc = (topic[0] == '/') ? true : false;
        goto exit;
    }

    /* because strtok will return bill when matching /bill/ or bill in a topic name for the first time,
     * we have to check whether the first character is / explicitly.
     */
    if ((wildTopic[0] == '/') && (topic[0] != '/'))
    {
        goto exit;
    }

    if ((wildTopic[0] == '+') && (topic[0] == '/'))
    {
        goto exit;
    }

    wildTopic = (char*) strdup(wildTopic);
    topic = (char*) strdup(topic);
    freetopics = true;

    pwild = strtok_r(wildTopic, "/", &last1);
    pmatch = strtok_r(topic, "/", &last2);

    /* Step through the subscription, level by level */
    while (pwild != NULL)
    {
        /* Have we got # - if so, it matches anything. */
        if (strcmp(pwild, "#") == 0)
        {
            rc = true;
            break;
        }

        /* Nope - check for matches... */
        if (pmatch != NULL)
        {
            if (strcmp(pwild, "+") != 0 && strcmp(pwild, pmatch) != 0)
            {
                /* The two levels simply don't match... */
                break;
            }
        }
        else
        {
            break; /* No more tokens to match against further tokens in the wildcard stream... */
        }

        pwild = strtok_r(NULL, "/", &last1);
        pmatch = strtok_r(NULL, "/", &last2);
    }

    /* All tokens up to here matched, and we didn't end in #. If there
     are any topic tokens remaining, the match is bad, otherwise it was
     a good match. */
    if (pmatch == NULL && pwild == NULL)
    {
        rc = true;
    }

exit:

    if (freetopics)
    {
        /* Now free the memory allocated in strdup() */
        free(wildTopic);
        free(topic);
    }

    return rc;
} /* end matches*/

/*********************************************************************/
/*                                                                   */
/* Function Name:  GetReadableNumber                                 */
/*                                                                   */
/* Description:    Format a number to include commas                 */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: Produce a string which can be displayed including       */
/*           commas to separate millions, thousands and hundreds.    */
/*                                                                   */
/* Input:    int64_t  number         - Number to be formatted        */
/*                                                                   */
/* Output:   char    *readableNumber - Buffer to receive the text.   */
/*                                                                   */
/* Returns:  Nothing.                                                */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
void GetReadableNumber(char *readableNumber, int64_t number)
{
    char buffer[50];
    char *p = &buffer[sizeof(buffer)-1];
    int32_t i = 0;
    bool negative = false;

    if (number < 0)
    {
        negative = true;
        number *= -1;
    }

    *p = '\0';
    do
    {
        if(i%3 == 0 && i != 0)
            *--p = ',';
        *--p = '0' + number % 10;
        number /= 10;
        i++;
    } while(number != 0);

    if (negative)
        *--p = '-';

    strcpy(readableNumber, p);
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  RunScenario                                       */
/*                                                                   */
/* Description:    Run and report on the current set of work         */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function:                                                         */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t RunScenario(psgroupcontroller *controller,
                    psgroupinfo       *groups,
                    int32_t            groupCount,
                    int32_t            verbosity)
{
    int32_t rc = OK;

    ism_time_t setupTime = 0, startTime = 0, endTime = 0;

    psgroupinfo **waitGroups = malloc(groupCount*sizeof(psgroupinfo*));

    psgroupinfo  *curGroup = groups;
    psgroupinfo **curWaitGroup = waitGroups;
    char          readableNumber[5][20];

    if (waitGroups == NULL)
    {
        rc = 1;
        goto mod_exit;
    }

    ism_field_t f;
    ism_prop_t *monitorProps = ism_common_newProperties(2);

    if (monitorTopic != NULL)
    {
        controller->monitorTopic = monitorTopic;

        // Test the invalid parameter parsing in the topic monitor callback
        rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                       "TEST",
                                       monitorProps,
                                       ISM_CONFIG_CHANGE_PROPS);

        if (rc != ISMRC_InvalidParameter)
        {
            printf("ERROR: ism_engine_startTopicMonitor failed with %d\n", rc);
            goto mod_exit;
        }

        f.type = VT_String;
        f.val.s = monitorTopic;
        ism_common_setProperty(monitorProps,
                               ismENGINE_ADMIN_PREFIX_TOPICMONITOR_TOPICSTRING "TEST",
                               &f);

        // Start a topic monitor by emulating a config callback from the admin
        // component (thereby testing more of the engine).
        rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                       "TEST",
                                       monitorProps,
                                       ISM_CONFIG_CHANGE_PROPS);
        if (rc != OK)
        {
            printf("ERROR: ism_engine_startTopicMonitor failed with %d\n", rc);
            goto mod_exit;
        }

        printf(" + Topic Monitoring enabled on topic \"%s\"\n", monitorTopic);
    }

    while (rc == OK && (curGroup != &groups[groupCount]))
    {
        *curWaitGroup = curGroup;

        if (verbosity > 1)
        {
            if (curGroup->numSubbers)
            {
                GetReadableNumber(readableNumber[0], curGroup->numSubbers);
                printf("    = %s QoS%d Subscriber(s) on topic \"%s\".\n",
                       readableNumber[0], curGroup->QoS, curGroup->topicString);
            }

            if (curGroup->numPubbers)
            {
                GetReadableNumber(readableNumber[0], curGroup->numPubbers);
                GetReadableNumber(readableNumber[1], curGroup->msgsPerPubber);

                printf("    = %s QoS%d Publisher(s) on topic \"%s\", %s msgs per publisher.\n",
                       readableNumber[0], curGroup->QoS, curGroup->topicString, readableNumber[1]);
            }

            if (curGroup->numBgSubThreads)
            {
                GetReadableNumber(readableNumber[0], curGroup->numBgSubThreads);
                GetReadableNumber(readableNumber[1], curGroup->bgSubberDelay);

                printf("    = %s Background Subscribe Threads on topic \"%s\", with %s us delays\n",
                       readableNumber[0], curGroup->topicString, readableNumber[1]);
            }
        }

        rc = InitPubSubThreadGroup(curGroup++);
        curWaitGroup++;
    }

    if ( rc == OK )
    {
        rc = TriggerSetupThreadGroups(controller,
                                      &setupTime,
                                      false);
    }
    if (rc == OK)
    {
        rc = StartThreadGroups(controller, &startTime);
    }

    if (rc == OK)
    {
        rc = WaitForGroupsFinish(controller, &endTime,
                                 (verbosity > 3) ? (curWaitGroup-waitGroups) : 0,
                                 waitGroups);
    }

    if (rc == OK)
    {
        int i;

        for(i=0; i<groupCount; i++)
        {
            if (groups[i].groupBgSubbersCreated)
            {
                if (verbosity > 1)
                {
                    GetReadableNumber(readableNumber[0], groups[i].groupBgSubbersCreated);
                    GetReadableNumber(readableNumber[1], groups[i].groupBgMsgsRcvd);

                    printf("%s Background subscribers created on topic \"%s\", received %s total msgs\n",
                           readableNumber[0], groups[i].topicString, readableNumber[1]);
                }

                totalBackgroundSubMsgs += groups[i].groupBgMsgsRcvd;
            }
        }
    }

    if (monitorTopic != NULL)
    {
        // Stop the topic monitor by emulating a config callback
        rc = ism_engine_configCallback(ismENGINE_ADMIN_VALUE_TOPICMONITOR,
                                       "TEST",
                                       monitorProps,
                                       ISM_CONFIG_CHANGE_DELETE);
        if (rc != OK)
        {
            printf("ERROR: ism_engine_stopTopicMonitor failed with %d\n", rc);
            goto mod_exit;
        }
    }

    ism_common_freeProperties(monitorProps);

mod_exit:

    if (rc == OK)
    {
        double elapsedSeconds = ((endTime - startTime)/1000000000.0);
        double totalMsgs = (double)(totalExpectedPubMsgs+totalExpectedSubMsgs+totalBackgroundSubMsgs);
        double msgsPerSecond = totalMsgs/elapsedSeconds;

        GetReadableNumber(readableNumber[0], (int64_t)totalMsgs);
        GetReadableNumber(readableNumber[1], (int64_t)msgsPerSecond);
        GetReadableNumber(readableNumber[2], (int64_t)totalExpectedPubMsgs);
        GetReadableNumber(readableNumber[3], (int64_t)totalExpectedSubMsgs);
        GetReadableNumber(readableNumber[4], (int64_t)totalBackgroundSubMsgs);

        printf("%s msgs processed, elapsed time %.3f secs (%s msgs/sec)\n",
               readableNumber[0], elapsedSeconds, readableNumber[1]);
        printf("%s sent, %s received", readableNumber[2], readableNumber[3]);
        if (totalBackgroundSubMsgs != 0)
            printf(", %s received in background.\n", readableNumber[4]);
        else
            printf(".\n");
    }
    else
    {
        printf("ReturnCode is %d.\n", rc);
    }

    if (waitGroups) free(waitGroups);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_FanOut1                               */
/*                                                                   */
/* Description:    Add workload for a 1-10 fanout on "A/B/C/D"       */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: Just perform a QoS0 1-10 fan-out, with a large number   */
/*           of publications.                                        */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_FO1_PUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/A/B/C/D"
#define WORKLOAD_FO1_SUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/A/B/C/D"
#define WORKLOAD_FO1_PUBLISHERS       1
#define WORKLOAD_FO1_MSGSPERPUBBER    10000000
#define WORKLOAD_FO1_SUBSCRIBERS      10
#define WORKLOAD_FO1_GROUPS           2

int32_t AddWorkload_FanOut1(uint32_t            QoS,
                            uint8_t             msgPersistence,
                            psgroupcontroller  *pController,
                            psgroupinfo       **pGroups,
                            int32_t            *pGroupCount)
{
    static int32_t FO1_Instances = 0;

    psgroupinfo *curInfo;

    int32_t baseCount;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_FO1_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_FO1_GROUPS*sizeof(psgroupinfo));

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    if (QoS==ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
    {
        printf(" + QoS0 1->10 fan-out %s msgs (fo1/1)\n",
               (msgPersistence == ismMESSAGE_PERSISTENCE_NONPERSISTENT) ? "non-persistent" : "persistent");
    }
    else
    {
        printf(" + QoS1 1->10 fan-out %s msgs (fo4/13)\n",
               (msgPersistence == ismMESSAGE_PERSISTENCE_NONPERSISTENT) ? "non-persistent" : "persistent");
    }

    curInfo->topicString = malloc(strlen(WORKLOAD_FO1_PUBTOPICSTRING)+12);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_FO1_PUBTOPICSTRING);

    if (FO1_Instances > 0)
    {
        sprintf(&curInfo->topicString[strlen(WORKLOAD_FO1_PUBTOPICSTRING)], "/%03d", FO1_Instances);
    }

    curInfo->numPubbers = WORKLOAD_FO1_PUBLISHERS;
    curInfo->msgsPerPubber = WORKLOAD_FO1_MSGSPERPUBBER;
    curInfo->pubMsgPersistence = msgPersistence;
    curInfo->numSubbers = 0;
    curInfo->msgsPerSubber = 0;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    curInfo->QoS = QoS;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    curInfo++;

    curInfo->topicString = malloc(strlen(WORKLOAD_FO1_SUBTOPICSTRING)+12);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_FO1_SUBTOPICSTRING);

    if (FO1_Instances > 0)
    {
        sprintf(&curInfo->topicString[strlen(WORKLOAD_FO1_SUBTOPICSTRING)], "/%03d", FO1_Instances);
    }

    curInfo->numPubbers = 0;
    curInfo->msgsPerPubber = 0;
    curInfo->pubMsgPersistence = msgPersistence;
    curInfo->numSubbers = WORKLOAD_FO1_SUBSCRIBERS;
    curInfo->msgsPerSubber = WORKLOAD_FO1_PUBLISHERS * WORKLOAD_FO1_MSGSPERPUBBER;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    curInfo->QoS = QoS;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    FO1_Instances++;

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_FanOut2                               */
/*                                                                   */
/* Description:    Add workload for a QoS1 1-10 fanout on "A/B/C/D"  */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: perform a QoS1 1-10 fan-out, with a large number of     */
/*           publications.                                           */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_FO2_PUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/A/B/C/D"
#define WORKLOAD_FO2_SUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/A/B/C/D"
#define WORKLOAD_FO2_PUBLISHERS       1
#define WORKLOAD_FO2_MSGSPERPUBBER    10000000
#define WORKLOAD_FO2_SUBSCRIBERS      10
#define WORKLOAD_FO2_GROUPS           2

int32_t AddWorkload_FanOut2(psgroupcontroller  *pController,
                            psgroupinfo       **pGroups,
                            int32_t            *pGroupCount)
{
    static int32_t FO2_Instances = 0;

    psgroupinfo *curInfo;

    int32_t baseCount;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_FO2_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_FO2_GROUPS*sizeof(psgroupinfo));

    printf(" + 1->10 fan-out (fo2/11)\n");

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    curInfo->topicString = malloc(strlen(WORKLOAD_FO2_PUBTOPICSTRING)+12);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_FO2_PUBTOPICSTRING);

    if (FO2_Instances > 0)
    {
        sprintf(&curInfo->topicString[strlen(WORKLOAD_FO2_PUBTOPICSTRING)], "/%03d", FO2_Instances);
    }

    curInfo->numPubbers = WORKLOAD_FO2_PUBLISHERS;
    curInfo->msgsPerPubber = WORKLOAD_FO2_MSGSPERPUBBER;
    curInfo->numSubbers = 0;
    curInfo->msgsPerSubber = 0;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    curInfo++;

    curInfo->topicString = malloc(strlen(WORKLOAD_FO2_SUBTOPICSTRING)+12);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_FO2_SUBTOPICSTRING);

    if (FO2_Instances > 0)
    {
        sprintf(&curInfo->topicString[strlen(WORKLOAD_FO2_SUBTOPICSTRING)], "/%03d", FO2_Instances);
    }

    curInfo->numPubbers = 0;
    curInfo->msgsPerPubber = 0;
    curInfo->numSubbers = WORKLOAD_FO2_SUBSCRIBERS;
    curInfo->msgsPerSubber = WORKLOAD_FO2_PUBLISHERS * WORKLOAD_FO2_MSGSPERPUBBER;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    curInfo->QoS = 1;
    curInfo->txnCapable = TxnCapable_Random;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    FO2_Instances++;

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_FanOut3                               */
/*                                                                   */
/* Description:    Add workload for a QoS0 1-50  fanout on "M"       */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: perform a QoS0 1-50  fan-out, with a large number of    */
/*           publications.                                           */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_FO3_PUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/M"
#define WORKLOAD_FO3_SUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/M"
#define WORKLOAD_FO3_PUBLISHERS       1
#define WORKLOAD_FO3_MSGSPERPUBBER    10000000
#define WORKLOAD_FO3_SUBSCRIBERS      50
#define WORKLOAD_FO3_GROUPS           2

int32_t AddWorkload_FanOut3(psgroupcontroller  *pController,
                            psgroupinfo       **pGroups,
                            int32_t            *pGroupCount)
{
    static int32_t FO3_Instances = 0;

    psgroupinfo *curInfo;

    int32_t baseCount;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_FO3_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_FO3_GROUPS*sizeof(psgroupinfo));

    printf(" + 1->50 fan-out (fo3/12)\n");

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    curInfo->topicString = malloc(strlen(WORKLOAD_FO3_PUBTOPICSTRING)+11);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_FO3_PUBTOPICSTRING);

    if (FO3_Instances > 0)
    {
        sprintf(&curInfo->topicString[strlen(WORKLOAD_FO3_PUBTOPICSTRING)], "%03d", FO3_Instances);
    }

    curInfo->numPubbers = WORKLOAD_FO3_PUBLISHERS;
    curInfo->msgsPerPubber = WORKLOAD_FO3_MSGSPERPUBBER;
    curInfo->numSubbers = 0;
    curInfo->msgsPerSubber = 0;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    curInfo++;

    curInfo->topicString = malloc(strlen(WORKLOAD_FO3_SUBTOPICSTRING)+11);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_FO3_SUBTOPICSTRING);

    if (FO3_Instances > 0)
    {
        sprintf(&curInfo->topicString[strlen(WORKLOAD_FO2_SUBTOPICSTRING)], "%03d", FO3_Instances);
    }

    curInfo->numPubbers = 0;
    curInfo->msgsPerPubber = 0;
    curInfo->numSubbers = WORKLOAD_FO3_SUBSCRIBERS;
    curInfo->msgsPerSubber = WORKLOAD_FO3_PUBLISHERS * WORKLOAD_FO3_MSGSPERPUBBER;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    FO3_Instances++;

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_FanOut5                               */
/*                                                                   */
/* Description:    Add workload for a N-10 fanout on "A/B/C/D"       */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: Just perform a QoS? 1-10 fan-out, with a large number   */
/*           of publications.                                        */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_FO4_PUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/FO4/A/B/C/D"
#define WORKLOAD_FO4_SUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/FO4/A/B/C/D"
#define WORKLOAD_FO4_MSGSPERPUBBER    10000000
#define WORKLOAD_FO4_SUBSCRIBERS      10
#define WORKLOAD_FO4_GROUPS           2

int32_t AddWorkload_FanOut5(uint32_t            QoS,
                            uint8_t             msgPersistence,
                            uint32_t            NumPublishers,
                            psgroupcontroller  *pController,
                            psgroupinfo       **pGroups,
                            int32_t            *pGroupCount)
{
    static int32_t FO4_Instances = 0;

    psgroupinfo *curInfo;

    int32_t baseCount;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_FO4_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_FO4_GROUPS*sizeof(psgroupinfo));

    printf(" + QoS%d %d->10 fan-out %s msgs (fo5/14)\n",
            QoS, NumPublishers,
            (msgPersistence == ismMESSAGE_PERSISTENCE_NONPERSISTENT) ? "non-persistent" : "persistent");

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    curInfo->topicString = malloc(strlen(WORKLOAD_FO4_PUBTOPICSTRING)+12);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_FO4_PUBTOPICSTRING);

    if (FO4_Instances > 0)
    {
        sprintf(&curInfo->topicString[strlen(WORKLOAD_FO4_PUBTOPICSTRING)], "/%03d", FO4_Instances);
    }

    curInfo->numPubbers = NumPublishers;
    curInfo->msgsPerPubber = WORKLOAD_FO4_MSGSPERPUBBER;
    curInfo->pubMsgPersistence = msgPersistence;
    curInfo->numSubbers = 0;
    curInfo->msgsPerSubber = 0;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    curInfo->QoS = QoS;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    curInfo++;

    curInfo->topicString = malloc(strlen(WORKLOAD_FO4_SUBTOPICSTRING)+12);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_FO4_SUBTOPICSTRING);

    if (FO4_Instances > 0)
    {
        sprintf(&curInfo->topicString[strlen(WORKLOAD_FO4_SUBTOPICSTRING)], "/%03d", FO4_Instances);
    }

    curInfo->numPubbers = 0;
    curInfo->msgsPerPubber = 0;
    curInfo->pubMsgPersistence = msgPersistence;
    curInfo->numSubbers = WORKLOAD_FO4_SUBSCRIBERS;
    curInfo->msgsPerSubber = NumPublishers * WORKLOAD_FO4_MSGSPERPUBBER;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    curInfo->QoS = QoS;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    FO4_Instances++;

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_FanIn1                                */
/*                                                                   */
/* Description:    Add workload for a 10-1 fan-in on "H/I/J/K"       */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: Just perform a QoS0 1-10 fan-out, with a large number   */
/*           of publications.                                        */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_FI1_TOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/H/I/J/K"
#define WORKLOAD_FI1_PUBLISHERS    10
#define WORKLOAD_FI1_MSGSPERPUBBER 5000000
#define WORKLOAD_FI1_SUBSCRIBERS   1
#define WORKLOAD_FI1_GROUPS        1

int32_t AddWorkload_FanIn1(psgroupcontroller  *pController,
                           psgroupinfo       **pGroups,
                           int32_t            *pGroupCount)
{
    static int32_t FI1_Instances = 0;
    psgroupinfo *curInfo;

    int32_t baseCount;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_FI1_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_FI1_GROUPS*sizeof(psgroupinfo));

    printf(" + 10->1 fan-in (fi1/2)\n");

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    curInfo->topicString = malloc(strlen(WORKLOAD_FI1_TOPICSTRING)+5);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_FI1_TOPICSTRING);

    if (FI1_Instances > 0)
    {
        sprintf(&curInfo->topicString[strlen(WORKLOAD_FI1_TOPICSTRING)], "/%03d", FI1_Instances);
    }

    curInfo->numPubbers = WORKLOAD_FI1_PUBLISHERS;
    curInfo->msgsPerPubber = WORKLOAD_FI1_MSGSPERPUBBER;
    curInfo->numSubbers = WORKLOAD_FI1_SUBSCRIBERS;
    curInfo->msgsPerSubber = WORKLOAD_FI1_PUBLISHERS * WORKLOAD_FI1_MSGSPERPUBBER;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    FI1_Instances++;

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_BackgroundSubscribers1                */
/*                                                                   */
/* Description:    Background subscribers on topic "W/X/Y/Z"         */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: 10 threads continually subscribing to and then          */
/*           unsubscribing from topic "W/X/Y/Z".                     */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_BG1_TOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/W/X/Y/Z"
#define WORKLOAD_BG1_BGSUBTHREADS  10
#define WORKLOAD_BG1_BGSUBBERDELAY 1000
#define WORKLOAD_BG1_GROUPS        1

int32_t AddWorkload_BackgroundSubscribers1(psgroupcontroller  *pController,
                                           psgroupinfo       **pGroups,
                                           int32_t            *pGroupCount)
{
    psgroupinfo *curInfo;

    int32_t baseCount;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_BG1_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_BG1_GROUPS*sizeof(psgroupinfo));

    printf(" + QoS0 background subscription (bg1/3).\n");

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    curInfo->topicString = strdup(WORKLOAD_BG1_TOPICSTRING);
    curInfo->numPubbers = 0;
    curInfo->msgsPerPubber = 0;
    curInfo->numSubbers = 0;
    curInfo->msgsPerSubber = 0;
    curInfo->numBgSubThreads = WORKLOAD_BG1_BGSUBTHREADS;
    if (bgSubberDelay == -1)
        curInfo->bgSubberDelay = WORKLOAD_BG1_BGSUBBERDELAY;
    else
        curInfo->bgSubberDelay = bgSubberDelay;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_ComplexPubSub1                        */
/*                                                                   */
/* Description:    A complicated publish/subscribe configuration.    */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: Based on a real customer configuration, multiple sub on */
/*           wildcarded topic strings, multiple pub on distinct      */
/*           strings.                                                */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_CPS1_PUBLISHERS    500
#define WORKLOAD_CPS1_MSGSPERPUBBER 100000
#define WORKLOAD_CPS1_SUBSCRIBERS   5000
#define WORKLOAD_CPS1_GROUPS        WORKLOAD_CPS1_PUBLISHERS + WORKLOAD_CPS1_SUBSCRIBERS

int32_t AddWorkload_ComplexPubSub1(psgroupcontroller  *pController,
                                   psgroupinfo       **pGroups,
                                   int32_t            *pGroupCount)
{
    char topicstr[100];

    int32_t baseCount,i,j,r,r100,r1000;

    psgroupinfo *curInfo;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_CPS1_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_CPS1_GROUPS*sizeof(psgroupinfo));

    printf(" + Complex wildcard subscriptions 1 (cps1/4)\n");

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    /************************************************************/
    /* Complex, single node wildcard subscribers                */
    /************************************************************/
    for (i = 0; i < WORKLOAD_CPS1_SUBSCRIBERS; i++)
    {
        r = rand();
        r100 = r % 100;

        if (r100 < 19)
        {
            /* Topic double-wildcard, 7 parts */
            sprintf(topicstr, "test/"TPT_RESOURCE_SET_ID"/IXPO/AU/%d/+/+", (i % 450) + 10000);
        }
        else if (r100 < 40)
        {
            /* Topic double-wildcard, 8 parts, numeric */
            sprintf(topicstr, "test/"TPT_RESOURCE_SET_ID"/IXPO/AU/%d/%d/+/+", (i % 450) + 10000, (i % 10000) + 10000);
        }
        else if (r100 < 88)
        {
            /* Topic double-wildcard, 8 parts, string */
            if (r100 < 75)
            {
                sprintf(topicstr, "test/"TPT_RESOURCE_SET_ID"/IXPO/AU/%d/IQWV/+/+", (i % 450) + 10000);
            }
            else
            {
                sprintf(topicstr, "test/"TPT_RESOURCE_SET_ID"/IXPO/AU/%d/IQKS/+/+", (i % 450) + 10000);
            }
        }
        else
        {
            /* Topic, split-wildcard */
            if (r100 < 94)
            {
                sprintf(topicstr, "test/"TPT_RESOURCE_SET_ID"/IXPO/AU/+/SVKM/%d/+", (i % 20000) + 10000);
            }
            else if (r100 < 98)
            {
                sprintf(topicstr, "test/"TPT_RESOURCE_SET_ID"/IXPO/AU/+/TERM/%d/+", (i % 20000) + 10000);
            }
            else
            {
                sprintf(topicstr, "test/"TPT_RESOURCE_SET_ID"/IXPO/AU/+/KGSX/%d/+", (i * 13) + 10000);
            }
        }

        curInfo->topicString = strdup(topicstr);
        curInfo->numPubbers = 0;
        curInfo->msgsPerPubber = 0;
        curInfo->numSubbers = 1;
        curInfo->msgsPerSubber = 0; /* Calculated later */
        curInfo->controller = pController;
        curInfo->subsWait = subsWait;
        curInfo->cpuPin = cpuPin;

        pController->threadsControlled += curInfo->numPubbers +
                                          curInfo->numSubbers +
                                          curInfo->numBgSubThreads;
        (*pGroupCount)+=1;

        curInfo++;
    }

    /************************************************************/
    /* publishers on random strings                             */
    /************************************************************/
    for (i = 0; i < WORKLOAD_CPS1_PUBLISHERS; i++)
    {
        char *part2;
        char *part4;

        r = rand();
        r1000 = r % 1000;

        if (r1000 < 650)
        {
            part2 = "SVKM";
        }
        else if (r1000 < 800)
        {
            part2 = "KGSX";
        }
        else if (r1000 < 900)
        {
            part2 = "IQWV";
        }
        else if (r1000 < 950)
        {
            part2 = "AUFG";
        }
        else if (r1000 < 990)
        {
            part2 = "IQKS";
        }
        else if (r1000 < 991)
        {
            part2 = "AUFT";
        }
        else if (r1000 < 992)
        {
            part2 = "LAUF";
        }
        else if (r1000 < 993)
        {
            part2 = "OPPO";
        }
        else if (r1000 < 994)
        {
            part2 = "KOBE";
        }
        else if (r1000 < 995)
        {
            part2 = "KOTB";
        }
        else if (r1000 < 996)
        {
            part2 = "LEAD";
        }
        else if (r1000 < 997)
        {
            part2 = "IVPV";
        }
        else if (r1000 < 998)
        {
            part2 = "SEBT";
        }
        else if (r1000 < 999)
        {
            part2 = "ANGE";
        }
        else
        {
            part2 = "KOVA";
        }

        if (r1000 < 350)
        {
            part4 = "CHANGEIAD";
        }
        else if (r1000 < 600)
        {
            part4 = "OBJECTRECEIVEDWITHDATA";
        }
        else if (r1000 < 850)
        {
            part4 = "OBJECTLOSTWITHDATA";
        }
        else if (r1000 < 950)
        {
            part4 = "CHANGEDATAWITHDATA";
        }
        else
        {
            part4 = "CHANGEVIEWDATAWITHDATA";
        }

        sprintf(topicstr, "test/"TPT_RESOURCE_SET_ID"/IXPO/AU/%d/%s/%d/%s", (i % 450) + 10000, part2, (i % 10) + 555555, part4);

        curInfo->topicString = strdup(topicstr);
        curInfo->numPubbers = 1;
        curInfo->msgsPerPubber = WORKLOAD_CPS1_MSGSPERPUBBER;
        curInfo->numSubbers = 0;
        curInfo->msgsPerSubber = 0;
        curInfo->controller = pController;
        curInfo->subsWait = subsWait;
        curInfo->cpuPin = cpuPin;

        /* Calculate which subscriber groups this publisher will publish to */
        for (j = 0; j < WORKLOAD_CPS1_SUBSCRIBERS; j++)
        {
            if (Topics_match((*pGroups)[baseCount+j].topicString, topicstr))
            {
                (*pGroups)[baseCount+j].msgsPerSubber += curInfo->msgsPerPubber;
            }
        }

        totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;

        pController->threadsControlled += curInfo->numPubbers +
                                          curInfo->numSubbers +
                                          curInfo->numBgSubThreads;
        (*pGroupCount)+=1;

        curInfo++;
    }

    for (j = 0; j < WORKLOAD_CPS1_SUBSCRIBERS; j++)
    {
        totalExpectedSubMsgs += (*pGroups)[baseCount+j].msgsPerSubber * (*pGroups)[baseCount+j].numSubbers;
    }

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_ComplexPubSub2                        */
/*                                                                   */
/* Description:    A complicated publish/subscribe configuration.    */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: Based on a real customer configuration, multiple sub on */
/*           wildcarded topic strings, multiple pub on distinct      */
/*           strings.                                                */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_CPS2_PUBLISHERSPERCPU 2
#define WORKLOAD_CPS2_MSGSPERPUBBER    1000000
#define WORKLOAD_CPS2_SUBSCRIBERS      200

int32_t AddWorkload_ComplexPubSub2(psgroupcontroller  *pController,
                                   psgroupinfo       **pGroups,
                                   int32_t            *pGroupCount)
{
    int  cpuCount;
    int  publisherCount,subscriberCount;
    char topicstr[500];

    int32_t baseCount,i,j,r;

    cpuCount = sysconf(_SC_NPROCESSORS_ONLN);
    publisherCount =  cpuCount * WORKLOAD_CPS2_PUBLISHERSPERCPU;
    subscriberCount = WORKLOAD_CPS2_SUBSCRIBERS;

    psgroupinfo *curInfo;

    *pGroups = realloc(*pGroups,
                       (*pGroupCount+publisherCount+subscriberCount)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, (publisherCount+subscriberCount)*sizeof(psgroupinfo));

    printf(" + Complex wildcard subscriptions 2 (cps2/5)\n");

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    /************************************************************/
    /* Complex, single node wildcard subscribers                */
    /************************************************************/
    for (i = 0; i < subscriberCount; i++)
    {
        sprintf(topicstr,
                "test/"TPT_RESOURCE_SET_ID
                "/ConStatus/C3Id/+/PayingCouId/+/OrigCouId/+/IntDomIn/+/LegConIn/+/DetailedCon/+/ErrorCon/+"
                "/OrigDepot/%08X/DestDepot/+/StatCdProcess/+/StatCdActivity/+/StatCdCondition/+/StatCdException/+/#",
                rand()%(cpuCount*2));

        curInfo->topicString = strdup(topicstr);
        curInfo->numPubbers = 0;
        curInfo->msgsPerPubber = 0;
        curInfo->numSubbers = 1;
        curInfo->msgsPerSubber = 0; /* Calculated later */
        curInfo->controller = pController;
        curInfo->subsWait = subsWait;
        curInfo->cpuPin = cpuPin;

        pController->threadsControlled += curInfo->numPubbers +
                                          curInfo->numSubbers +
                                          curInfo->numBgSubThreads;
        (*pGroupCount)+=1;

        curInfo++;
    }

    /************************************************************/
    /* publishers on random strings                             */
    /************************************************************/
    for (i = 0; i < publisherCount; i++)
    {
        sprintf(topicstr,
                "test/"TPT_RESOURCE_SET_ID
                "/ConStatus/C3Id/%08x/PayingCouId/%08x/OrigCouId/%08x/IntDomIn/ffffffff/LegConIn/ffffffff/DetailedCon/ffffffff/ErrorCon/ffffffff"
                "/OrigDepot/%08x/DestDepot/%08x/StatCdProcess/ffffffff/StatCdActivity/ffffffff/StatCdCondition/ffffffff/StatCdException/ffffffff",
                rand()%300, rand()%300, rand()%300, rand()%(cpuCount*2), rand()%(cpuCount*2));

        r = rand()%3;

        while(r-- > 0)
        {
            strcat(topicstr, "/AddLevel");
        }

        curInfo->topicString = strdup(topicstr);
        curInfo->numPubbers = 1;
        curInfo->msgsPerPubber = WORKLOAD_CPS2_MSGSPERPUBBER;
        curInfo->numSubbers = 0;
        curInfo->msgsPerSubber = 0;
        curInfo->controller = pController;
        curInfo->subsWait = subsWait;
        curInfo->cpuPin = cpuPin;

        /* Calculate which subscriber groups this publisher will publish to */
        for (j = 0; j < subscriberCount; j++)
        {
            if (Topics_match((*pGroups)[baseCount+j].topicString, topicstr))
            {
                (*pGroups)[baseCount+j].msgsPerSubber += curInfo->msgsPerPubber;
            }
        }

        totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;

        pController->threadsControlled += curInfo->numPubbers +
                                          curInfo->numSubbers +
                                          curInfo->numBgSubThreads;
        (*pGroupCount)+=1;

        curInfo++;
    }

    for (j = 0; j < subscriberCount; j++)
    {
        totalExpectedSubMsgs += (*pGroups)[baseCount+j].msgsPerSubber * (*pGroups)[baseCount+j].numSubbers;
    }

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_BackgroundSubscribers2                */
/*                                                                   */
/* Description:    Background subscribers on topic "W/X/+/#"         */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: 10 threads continually subscribing to and then          */
/*           unsubscribing from topic "W/X/+/#".                     */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_BG2_TOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/W/X/+/#"
#define WORKLOAD_BG2_BGSUBTHREADS  10
#define WORKLOAD_BG2_BGSUBBERDELAY 1000
#define WORKLOAD_BG2_GROUPS        1

int32_t AddWorkload_BackgroundSubscribers2(psgroupcontroller  *pController,
                                           psgroupinfo       **pGroups,
                                           int32_t            *pGroupCount)
{
    psgroupinfo *curInfo;

    int32_t baseCount;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_BG2_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_BG2_GROUPS*sizeof(psgroupinfo));

    printf(" + Complex background subscription (bg2/6).\n");

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    curInfo->topicString = strdup(WORKLOAD_BG2_TOPICSTRING);
    curInfo->numPubbers = 0;
    curInfo->msgsPerPubber = 0;
    curInfo->numSubbers = 0;
    curInfo->msgsPerSubber = 0;
    curInfo->numBgSubThreads = WORKLOAD_BG2_BGSUBTHREADS;
    if (bgSubberDelay == -1)
        curInfo->bgSubberDelay = WORKLOAD_BG2_BGSUBBERDELAY;
    else
        curInfo->bgSubberDelay = bgSubberDelay;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_RemoteServer                          */
/*                                                                   */
/* Description:    Add remote server with specified topics in topic  */
/*                 tree.                                             */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function:                                                         */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t AddWorkload_RemoteServer(char *remoteServerName,
                                 char *remoteServerUID,
                                 char **topicStrings,
                                 size_t topicStringCount,
                                 ismEngine_RemoteServerHandle_t *pHandle)
{
    int32_t rc;

    printf(" + RemoteServer named %s [UID:%s] on %lu topic%s",
           remoteServerName, remoteServerUID, topicStringCount,topicStringCount == 1 ? "":"s");
    if (topicStringCount > 0)
    {
        printf(" [");
        for(size_t x=0; x<topicStringCount; x++)
        {
            printf("'%s'", topicStrings[x]);
            if (x==topicStringCount-1) printf("]");
            else printf(",");
        }
    }

    printf("\n");

    rc = iers_clusterEventCallback(ENGINE_RS_CREATE,
                                   NULL, 0,
                                   remoteServerName,
                                   remoteServerUID,
                                   NULL, 0,
                                   NULL, 0,
                                   0, false, NULL, NULL,
                                   NULL,
                                   pHandle);

    if (rc == OK)
    {
        rc = iers_clusterEventCallback(ENGINE_RS_ADD_SUB,
                                       *pHandle, 0,
                                       NULL,
                                       NULL,
                                       NULL, 0,
                                       topicStrings, topicStringCount,
                                       0, false, NULL, NULL,
                                       NULL,
                                       NULL);
    }

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  DelWorkload_RemoteServer                          */
/*                                                                   */
/* Description:    Remove the remote server with specified handle    */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function:                                                         */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t DelWorkload_RemoteServer(ismEngine_RemoteServerHandle_t handle)
{
    if (handle != NULL)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();
        ismEngine_QueueStatistics_t lowstats, highstats;
        char          readableNumber[4][20];

        ieq_getStats(pThreadData, handle->lowQoSQueueHandle, &lowstats);
        ieq_getStats(pThreadData, handle->highQoSQueueHandle, &highstats);

        GetReadableNumber(readableNumber[0], (int64_t)lowstats.EnqueueCount);
        GetReadableNumber(readableNumber[1], (int64_t)highstats.EnqueueCount);

        printf("%s msgs enqueued to remote server '%s' [UID:%s] lowQoS queue, %s msgs to highQoS queue.\n",
               readableNumber[0], handle->serverName, handle->serverUID, readableNumber[1]);

        (void)iers_clusterEventCallback(ENGINE_RS_REMOVE,
                                        handle, 0,
                                        NULL,
                                        NULL,
                                        NULL, 0,
                                        NULL, 0,
                                        0, false, NULL, NULL,
                                        NULL,
                                        NULL);
    }

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_PutRetained                           */
/*                                                                   */
/* Description:    Populate a topic string with a retained message   */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function:                                                         */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_RET1_TOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/W/X/Y/Z"
#define WORKLOAD_RET2_TOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/W/X/I/J/K"
#define WORKLOAD_RET_NONPERS_TEXT   "Non-persistent retained msg on '%s'"
#define WORKLOAD_RET_PERS_TEXT      "Persistent retained msg on '%s'"

int32_t AddWorkload_PutRetained(char *clientId, char *topicString)
{
    int32_t rc;

    ismEngine_ClientStateHandle_t hClient;
    ismEngine_SessionHandle_t     hSession;

    ismEngine_MessageHandle_t hMessage = NULL;
    uint8_t persistence;
    size_t payloadLength;
    char *payload;
    uint32_t actionsRemaining = 0;
    uint32_t *pActionsRemaining = &actionsRemaining;

    if (!strcmp(topicString, WORKLOAD_RET1_TOPICSTRING))
    {
        printf(" + Publish a non-persistent retained message on '%s' (ret1/7)\n", topicString);
        persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
        payloadLength = strlen(topicString)+strlen(WORKLOAD_RET_NONPERS_TEXT)+1;
        payload = malloc(payloadLength);
        TEST_ASSERT(payload != NULL, ("payload (%p) == NULL", payload));
        sprintf(payload, WORKLOAD_RET_NONPERS_TEXT, topicString);
    }
    else if (!strcmp(topicString, WORKLOAD_RET2_TOPICSTRING))
    {
        printf(" + Publish a persistent retained message on '%s' (ret2/8)\n", topicString);
        persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
        payloadLength = strlen(topicString)+strlen(WORKLOAD_RET_PERS_TEXT)+1;
        payload = malloc(payloadLength);
        TEST_ASSERT(payload != NULL, ("payload (%p) == NULL", payload));
        sprintf(payload, WORKLOAD_RET_PERS_TEXT, topicString);
    }
    else
    {
        TEST_ASSERT(false, ("topicString '%s' unexpected", topicString));
    }

    char fullClientId[strlen(clientId)+strlen(PUTTER_CLIENTID_FORMAT)+1];
    sprintf(fullClientId, PUTTER_CLIENTID_FORMAT, clientId);

    rc = ism_engine_createClientState(fullClientId,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

    /* Create the message to put */
    void *payloadPtr = payload;
    rc = test_createMessage(payloadLength,
                            persistence,
                            ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                            ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN,
                            0,
                            ismDESTINATION_TOPIC, topicString,
                            &hMessage, &payloadPtr);
    TEST_ASSERT(rc == OK, ("rc (%d) != OK", rc));

    free(payload);

    rc = sync_ism_engine_putMessageOnDestination(hSession,
                                                 ismDESTINATION_TOPIC,
                                                 topicString,
                                                 NULL,
                                                 hMessage);
    TEST_ASSERT(rc < ISMRC_Error, ("rc (%d) >= ISMRC_Error", rc));

    test_incrementActionsRemaining(pActionsRemaining, 2);
    rc = ism_engine_destroySession(hSession, &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("rc (%d) != ISMRC_AsyncCompletion", rc));

    rc = ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE, &pActionsRemaining, sizeof(pActionsRemaining), test_decrementActionsRemaining);
    if (rc == OK) test_decrementActionsRemaining(rc, NULL, &pActionsRemaining);
    else TEST_ASSERT(rc == ISMRC_AsyncCompletion, ("rc (%d) != ISMRC_AsyncCompletion", rc));

    test_waitForRemainingActions(pActionsRemaining);

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_RetainedProducer1                     */
/*                                                                   */
/* Description:    Add workload for a retained publisher "W/X/Y/Z"   */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function:                                                         */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_RP1_PUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/W/X/Y/Z"
#define WORKLOAD_RP1_PUBLISHERS       2
#define WORKLOAD_RP1_MSGSPERPUBBER    100000
#define WORKLOAD_RP1_GROUPS           1

int32_t AddWorkload_RetainedProducer1(psgroupcontroller  *pController,
                                      psgroupinfo       **pGroups,
                                      int32_t            *pGroupCount)
{
    psgroupinfo *curInfo;

    int32_t baseCount;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_RP1_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_RP1_GROUPS*sizeof(psgroupinfo));

    printf(" + %d publisher(s) publishing retained messages on '%s' (rp1/9)\n",
           WORKLOAD_RP1_PUBLISHERS, WORKLOAD_RP1_PUBTOPICSTRING);

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    curInfo->topicString = strdup(WORKLOAD_RP1_PUBTOPICSTRING);
    curInfo->numPubbers = WORKLOAD_RP1_PUBLISHERS;
    curInfo->msgsPerPubber = WORKLOAD_RP1_MSGSPERPUBBER;
    curInfo->pubMsgFlags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;
    curInfo->numSubbers = 0;
    curInfo->msgsPerSubber = 0;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_RetainedProducer1                     */
/*                                                                   */
/* Description:    Add workload for a retained publisher "W/X/Y/Z"   */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function:                                                         */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_RP2_PUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/W/X/Y/A"
#define WORKLOAD_RP2_PUBLISHERS       2
#define WORKLOAD_RP2_MSGSPERPUBBER    100000
#define WORKLOAD_RP2_GROUPS           1

int32_t AddWorkload_RetainedProducer2(psgroupcontroller  *pController,
                                      psgroupinfo       **pGroups,
                                      int32_t            *pGroupCount)
{
    psgroupinfo *curInfo;

    int32_t baseCount;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_RP2_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_RP2_GROUPS*sizeof(psgroupinfo));

    printf(" + %d publisher(s) publishing persistent retained messages on '%s' (rp2/10)\n",
           WORKLOAD_RP2_PUBLISHERS, WORKLOAD_RP2_PUBTOPICSTRING);

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    curInfo->topicString = strdup(WORKLOAD_RP2_PUBTOPICSTRING);
    curInfo->numPubbers = WORKLOAD_RP2_PUBLISHERS;
    curInfo->msgsPerPubber = WORKLOAD_RP2_MSGSPERPUBBER;
    curInfo->pubMsgFlags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;
    curInfo->pubMsgPersistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    curInfo->numSubbers = 0;
    curInfo->msgsPerSubber = 0;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_Spray1                                */
/*                                                                   */
/* Description:    Add workload for a sprayer on "TEST/XXXXX"        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function:                                                         */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_SP1_PUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/TEST"
#define WORKLOAD_SP1_SUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/TEST/#"
#define WORKLOAD_SP1_PUBLISHERS       1
#define WORKLOAD_SP1_MSGSPERPUBBER    10000000
#define WORKLOAD_SP1_PUBSPRAYWIDTH    5000
#define WORKLOAD_SP1_SUBSCRIBERS      1
#define WORKLOAD_SP1_GROUPS           2

int32_t AddWorkload_Spray1(uint32_t            QoS,
                           uint8_t             msgPersistence,
                           psgroupcontroller  *pController,
                           psgroupinfo       **pGroups,
                           int32_t            *pGroupCount)
{
    psgroupinfo *curInfo;

    int32_t baseCount;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_SP1_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_SP1_GROUPS*sizeof(psgroupinfo));

    if (QoS==ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
    {
        printf(" + Qos0 1->1 spray %s msgs (sp1/15)\n",
               (msgPersistence == ismMESSAGE_PERSISTENCE_NONPERSISTENT) ? "non-persistent" : "persistent");
    }
    else
    {
        printf(" + QoS1 1->1 spray %s msgs (sp2/16)\n",
               (msgPersistence == ismMESSAGE_PERSISTENCE_NONPERSISTENT) ? "non-persistent" : "persistent");
    }

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    curInfo->topicString = malloc(strlen(WORKLOAD_SP1_PUBTOPICSTRING)+1);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_SP1_PUBTOPICSTRING);

    curInfo->numPubbers = WORKLOAD_SP1_PUBLISHERS;
    curInfo->msgsPerPubber = WORKLOAD_SP1_MSGSPERPUBBER;
    curInfo->pubSprayWidth = WORKLOAD_SP1_PUBSPRAYWIDTH;
    curInfo->pubMsgPersistence = msgPersistence;
    curInfo->numSubbers = 0;
    curInfo->msgsPerSubber = 0;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    curInfo->QoS = QoS;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    curInfo++;

    curInfo->topicString = malloc(strlen(WORKLOAD_SP1_SUBTOPICSTRING)+1);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_SP1_SUBTOPICSTRING);

    curInfo->numPubbers = 0;
    curInfo->msgsPerPubber = 0;
    curInfo->pubMsgPersistence = msgPersistence;
    curInfo->numSubbers = WORKLOAD_SP1_SUBSCRIBERS;
    curInfo->msgsPerSubber = WORKLOAD_SP1_PUBLISHERS * WORKLOAD_SP1_MSGSPERPUBBER;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    curInfo->QoS = QoS;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  AddWorkload_PtP1                                  */
/*                                                                   */
/* Description:    Add workload for a point-to-point pub-sub test    */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function:                                                         */
/*                                                                   */
/* Input:                                                            */
/*                                                                   */
/* Output:                                                           */
/*                                                                   */
/* Returns:                                                          */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
#define WORKLOAD_PP1_PUBTOPICSTRING   "test/"TPT_RESOURCE_SET_ID"/PtoP"
#define WORKLOAD_PP1_PUBLISHERS       1
#define WORKLOAD_PP1_MSGSPERPUBBER    1000000
#define WORKLOAD_PP1_SUBSCRIBERS      1
#define WORKLOAD_PP1_GROUPS           1

int32_t AddWorkload_PtoP1(uint32_t            QoS,
                          uint8_t             msgPersistence,
                          psgroupcontroller  *pController,
                          psgroupinfo       **pGroups,
                          int32_t            *pGroupCount)
{
    static int32_t PP1_Instances = 0;

    psgroupinfo *curInfo;

    int32_t baseCount;

    *pGroups = realloc(*pGroups, (*pGroupCount+WORKLOAD_PP1_GROUPS)*sizeof(psgroupinfo));

    if (*pGroups == NULL)
        return 1;

    memset(&(*pGroups)[*pGroupCount], 0, WORKLOAD_PP1_GROUPS*sizeof(psgroupinfo));

    if (QoS==ismMESSAGE_RELIABILITY_AT_MOST_ONCE)
    {
        printf(" + QoS0 1->1 %s msgs (pp1/17)\n",
               (msgPersistence == ismMESSAGE_PERSISTENCE_NONPERSISTENT) ? "non-persistent" : "persistent");
    }
    else
    {
        printf(" + QoS1 1->1 %s msgs (pp2/18)\n",
               (msgPersistence == ismMESSAGE_PERSISTENCE_NONPERSISTENT) ? "non-persistent" : "persistent");
    }

    baseCount = *pGroupCount;

    curInfo = &(*pGroups)[baseCount];

    curInfo->topicString = malloc(strlen(WORKLOAD_PP1_PUBTOPICSTRING)+5);

    if (curInfo->topicString == NULL)
        return 2;

    strcpy(curInfo->topicString, WORKLOAD_PP1_PUBTOPICSTRING);

    char tmpString[5];
    sprintf(tmpString, "/%03d", PP1_Instances);
    strcat(curInfo->topicString, tmpString);

    curInfo->numPubbers = WORKLOAD_PP1_PUBLISHERS;
    curInfo->msgsPerPubber = WORKLOAD_PP1_MSGSPERPUBBER;
    curInfo->pubMsgPersistence = msgPersistence;
    curInfo->numSubbers = WORKLOAD_PP1_SUBSCRIBERS;
    curInfo->msgsPerSubber = WORKLOAD_PP1_PUBLISHERS * WORKLOAD_PP1_MSGSPERPUBBER;
    curInfo->controller = pController;
    curInfo->subsWait = subsWait;
    curInfo->cpuPin = cpuPin;
    curInfo->QoS = QoS;
    totalExpectedPubMsgs += curInfo->numPubbers * curInfo->msgsPerPubber;
    totalExpectedSubMsgs += curInfo->numSubbers * curInfo->msgsPerSubber;

    pController->threadsControlled += curInfo->numPubbers +
                                      curInfo->numSubbers +
                                      curInfo->numBgSubThreads;
    (*pGroupCount)+=1;
    PP1_Instances++;

    return 0;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  WorkloadSelection (Scenario)                      */
/*                                                                   */
/* Description:    Add selected workloads then run scenario          */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: Allows command line selection of workload from the set  */
/*           provided.                                               */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t WorkloadSelection(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;
    char              *curWorkload;

    printf("Workload Selection Scenario\n");
    initControllerConds(&controller);

    curWorkload = strtok(workloads, ",");
    while(curWorkload)
    {
        int numeric = atoi(curWorkload);

        if ((strcmpi(curWorkload,"fo1") == 0) || numeric == 1)
            rc = AddWorkload_FanOut1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                     ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                                     &controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"fi1") == 0) || numeric == 2)
            rc = AddWorkload_FanIn1(&controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"bg1") == 0) || numeric == 3)
            rc = AddWorkload_BackgroundSubscribers1(&controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"cps1") == 0) || numeric == 4)
            rc = AddWorkload_ComplexPubSub1(&controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"cps2") == 0) || numeric == 5)
            rc = AddWorkload_ComplexPubSub2(&controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"bg2") == 0) || numeric == 6)
            rc = AddWorkload_BackgroundSubscribers2(&controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"ret1") == 0) || numeric == 7)
            rc = AddWorkload_PutRetained(curWorkload, WORKLOAD_RET1_TOPICSTRING);
        else if ((strcmpi(curWorkload,"ret2") == 0) || numeric == 8)
            rc = AddWorkload_PutRetained(curWorkload, WORKLOAD_RET2_TOPICSTRING);
        else if ((strcmpi(curWorkload,"rp1") == 0) || numeric == 9)
            rc = AddWorkload_RetainedProducer1(&controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"rp2") == 0) || numeric == 10)
            rc = AddWorkload_RetainedProducer2(&controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"fo2") == 0) || numeric == 11)
            rc = AddWorkload_FanOut2(&controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"fo3") == 0) || numeric == 12)
            rc = AddWorkload_FanOut3(&controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"fo4") == 0) || numeric == 13)
            rc = AddWorkload_FanOut1(ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                     ismMESSAGE_PERSISTENCE_PERSISTENT,
                                     &controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"fo5") == 0) || numeric == 14)
            rc = AddWorkload_FanOut5(ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                     ismMESSAGE_PERSISTENCE_PERSISTENT,
                                     4, &controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"sp1") == 0) || numeric == 15)
            rc = AddWorkload_Spray1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                                    &controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"sp2") == 0) || numeric == 16)
            rc = AddWorkload_Spray1(ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                    ismMESSAGE_PERSISTENCE_PERSISTENT,
                                    &controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"pp1") == 0) || numeric == 17)
            rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE,
                                   ismMESSAGE_PERSISTENCE_PERSISTENT,
                                   &controller, &groups, &groupCount);
        else if ((strcmpi(curWorkload,"pp2") == 0) || numeric == 18)
            rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                                   ismMESSAGE_PERSISTENCE_PERSISTENT,
                                   &controller, &groups, &groupCount);
        else
            printf(" ? Unexpected workload '%s' ignored.\n", curWorkload);

        curWorkload = strtok(NULL, ",");
    }

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario1                                         */
/*                                                                   */
/* Description:    QoS0 1-10 fan-out                                 */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario1(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;
    uint8_t            msgPersistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;

    printf("Scenario 1\n");
    initControllerConds(&controller);

    rc = AddWorkload_FanOut1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario2                                         */
/*                                                                   */
/* Description:    A complicated publish/subscribe configuration.    */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario2(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;

    printf("Scenario 2\n");

#if 1
    printf("Not currently working, ISMRC_StoreFull returned due to large thread count.\n");
    rc = ISMRC_NotImplemented;
    goto mod_exit;
#else
    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;

    initControllerConds(&controller);

    rc = AddWorkload_ComplexPubSub1(&controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);
#endif

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario3                                         */
/*                                                                   */
/* Description:    QoS0 1-10 fan-out with background subscribers     */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario3(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;
    uint8_t            msgPersistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;

    printf("Scenario 3\n");

    initControllerConds(&controller);

    rc = AddWorkload_FanOut1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc == OK)
        rc = AddWorkload_BackgroundSubscribers1(&controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario4                                         */
/*                                                                   */
/* Description:    QoS0 10-1 fan-in                                  */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario4(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;

    printf("Scenario 4\n");

    initControllerConds(&controller);

    rc = AddWorkload_FanIn1(&controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario5                                         */
/*                                                                   */
/* Description:    Complex pub/sub config with background subs       */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario5(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;

    printf("Scenario 5\n");

#if 1
    printf("Not currently working, ISMRC_StoreFull returned due to large thread count.\n");
    rc = ISMRC_NotImplemented;
    goto mod_exit;
#else
    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;

    initControllerConds(&controller);

    rc = AddWorkload_ComplexPubSub1(&controller, &groups, &groupCount);

    if (rc == OK)
        rc = AddWorkload_BackgroundSubscribers1(&controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;


    rc = RunScenario(&controller, groups, groupCount, verbosity);
#endif

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario6                                         */
/*                                                                   */
/* Description:    A complicated publish/subscribe configuration     */
/*                 involving the topic tree as a selection string.   */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario6(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;

    printf("Scenario 6\n");
    initControllerConds(&controller);

    rc = AddWorkload_ComplexPubSub2(&controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario7                                         */
/*                                                                   */
/* Description:    QoS0 1-10 fan-out with background subscribers     */
/*                 and there is a retained message on the background */
/*                 subscriber topic.                                 */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario7(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;
    uint8_t            msgPersistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;

    printf("Scenario 7\n");

    initControllerConds(&controller);

    rc = AddWorkload_PutRetained("Scenario7_RetClient1", WORKLOAD_RET1_TOPICSTRING);

    if (rc == OK)
        rc = AddWorkload_FanOut1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc == OK)
        rc = AddWorkload_BackgroundSubscribers1(&controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario8                                         */
/*                                                                   */
/* Description:    QoS0 1-10 fan-out with background subscribers     */
/*                 on a wildcarded topic and there is a retained     */
/*                 message on a matching topic.                      */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario8(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;
    uint8_t            msgPersistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;

    printf("Scenario 8\n");

    initControllerConds(&controller);

    rc = AddWorkload_PutRetained("Scenario8_RetClient1", WORKLOAD_RET1_TOPICSTRING);
    rc = AddWorkload_PutRetained("Scenario8_RetClient2", WORKLOAD_RET2_TOPICSTRING);

    if (rc == OK)
        rc = AddWorkload_FanOut1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc == OK)
        rc = AddWorkload_BackgroundSubscribers2(&controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario9                                         */
/*                                                                   */
/* Description:    QoS0 1-10 fan-out with background subscribers     */
/*                 on a wildcarded topic, with a retained message on */
/*                 one topic string, and continual retaineds being   */
/*                 published on another.                             */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario9(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;
    uint8_t            msgPersistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;

    printf("Scenario 9\n");

    initControllerConds(&controller);

    if (rc == OK)
        rc = AddWorkload_PutRetained("Scenario9_RetClient1", WORKLOAD_RET2_TOPICSTRING);
    if (rc == OK)
        rc = AddWorkload_RetainedProducer1(&controller, &groups, &groupCount);
    if (rc == OK)
        rc = AddWorkload_FanOut1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc == OK)
        rc = AddWorkload_BackgroundSubscribers2(&controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario10                                        */
/*                                                                   */
/* Description:    QoS0 1-10 fan-out with background subscribers     */
/*                 on a wildcarded topic, with a retained message on */
/*                 one topic string, and continual persistent        */
/*                 retaineds being published on another.             */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario10(int32_t verbosity)
{
    int32_t rc = OK;

    ismEngine_RemoteServerHandle_t handle1 = NULL;
    ismEngine_RemoteServerHandle_t handle2 = NULL;
    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo *groups = NULL;
    int32_t groupCount = 0;
    uint8_t msgPersistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;

    printf("Scenario 10\n");

    initControllerConds(&controller);

    if (rc == OK)
        rc = AddWorkload_PutRetained("Scenario10_RetClient1", WORKLOAD_RET2_TOPICSTRING);
    if (rc == OK)
        rc = AddWorkload_RetainedProducer1(&controller, &groups, &groupCount);
    if (rc == OK)
        rc = AddWorkload_RetainedProducer2(&controller, &groups, &groupCount);
    if (rc == OK)
        rc = AddWorkload_FanOut1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc == OK)
        rc = AddWorkload_BackgroundSubscribers2(&controller, &groups, &groupCount);
    if (rc == OK)
    {
        char *topicString[] = {"L/+/N/#", "A/X/C/+"};
        int32_t topicStringCount = (int32_t)(sizeof(topicString)/sizeof(topicString[0]));
        rc = AddWorkload_RemoteServer((char *)"RemoteServer1", "RSUID1", topicString, topicStringCount, &handle1);
    }
    if (rc == OK)
    {
        char *topicString[] = {"A/#"};
        int32_t topicStringCount = (int32_t)(sizeof(topicString)/sizeof(topicString[0]));
        rc = AddWorkload_RemoteServer((char *)"RemoteServer2", "RSUID2", topicString, topicStringCount, &handle2);
    }

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

    DelWorkload_RemoteServer(handle1);
    DelWorkload_RemoteServer(handle2);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario11                                        */
/*                                                                   */
/* Description:    QoS1 1-10 fan-out with background subscribers     */
/*                 on a wildcarded topic, with a retained message on */
/*                 one topic string, and continual persistent        */
/*                 retaineds being published on another.             */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario11(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;

    printf("Scenario 11\n");

    initControllerConds(&controller);

    if (rc == OK)
        rc = AddWorkload_PutRetained("Scenario11_RetClient1", WORKLOAD_RET2_TOPICSTRING);
    if (rc == OK)
        rc = AddWorkload_RetainedProducer1(&controller, &groups, &groupCount);
    if (rc == OK)
        rc = AddWorkload_RetainedProducer2(&controller, &groups, &groupCount);
    if (rc == OK)
        rc = AddWorkload_FanOut2(&controller, &groups, &groupCount);
    if (rc == OK)
        rc = AddWorkload_BackgroundSubscribers2(&controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario12                                        */
/*                                                                   */
/* Description:    QoS0 1-50  fan-out                                */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario12(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;

    printf("Scenario 12\n");

    initControllerConds(&controller);

    rc = AddWorkload_FanOut3(&controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario13                                        */
/*                                                                   */
/* Description:    QoS1 1-10 fan-out                                 */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario13(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;
    uint8_t            msgPersistence = ismMESSAGE_PERSISTENCE_PERSISTENT;

    printf("Scenario 13\n");

    initControllerConds(&controller);

    rc = AddWorkload_FanOut1(ismMESSAGE_RELIABILITY_AT_LEAST_ONCE, msgPersistence, &controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario14                                        */
/*                                                                   */
/* Description:    QoS1 4-10 fan-out                                 */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario14(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;
    uint8_t            msgPersistence = ismMESSAGE_PERSISTENCE_PERSISTENT;

    printf("Scenario 14\n");

    initControllerConds(&controller);

    rc = AddWorkload_FanOut5(ismMESSAGE_RELIABILITY_AT_LEAST_ONCE, msgPersistence, 4, &controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario15                                        */
/*                                                                   */
/* Description:    QoS0 1-1 spray                                    */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario15(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;
    uint8_t            msgPersistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;

    printf("Scenario 15\n");

    initControllerConds(&controller);

    rc = AddWorkload_Spray1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);

    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario16                                        */
/*                                                                   */
/* Description:    QoS0 5:5:5 (Persistent Msgs)                      */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario16(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;
    uint8_t            msgPersistence = ismMESSAGE_PERSISTENCE_PERSISTENT;

    printf("Scenario 16\n");

    initControllerConds(&controller);

    rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc != OK)
        goto mod_exit;
    rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc != OK)
        goto mod_exit;
    rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc != OK)
        goto mod_exit;
    rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc != OK)
        goto mod_exit;
    rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_MOST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  Scenario17                                        */
/*                                                                   */
/* Description:    QoS1 5:5:5 (Persistent Msgs)                      */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: See individual workload comments.                       */
/*                                                                   */
/* Input:    int32_t     verbosity - Verbosity of the output         */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int32_t Scenario17(int32_t verbosity)
{
    int32_t rc = OK;

    psgroupcontroller  controller = PSGROUPCONTROLLERDEFAULT;
    psgroupinfo       *groups = NULL;
    int32_t            groupCount = 0;
    uint8_t            msgPersistence = ismMESSAGE_PERSISTENCE_PERSISTENT;

    printf("Scenario 17\n");

    initControllerConds(&controller);

    rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_LEAST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc != OK)
        goto mod_exit;
    rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_LEAST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc != OK)
        goto mod_exit;
    rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_LEAST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc != OK)
        goto mod_exit;
    rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_LEAST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc != OK)
        goto mod_exit;
    rc = AddWorkload_PtoP1(ismMESSAGE_RELIABILITY_AT_LEAST_ONCE, msgPersistence, &controller, &groups, &groupCount);
    if (rc != OK)
        goto mod_exit;

    rc = RunScenario(&controller, groups, groupCount, verbosity);

mod_exit:

    FreeGroups(groups, groupCount);

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  ProcessArgs                                       */
/*                                                                   */
/* Description:    Process command line arguments                    */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: Return values specified on the command line             */
/*                                                                   */
/* Input:    int    argc                 - number of arguments       */
/*           char **argv                 - arguments                 */
/*                                                                   */
/* Output:   scenarioFunc **pScenario        - scenario function     */
/*           bool          *pSubsWait        - subs wait for msgs?   */
/*           bool          *pCpuPin          - pin to a cpu.         */
/*           int32_t       *pVerbosity       - verbosity of output.  */
/*           ism_time_t    *pSeedVal         - random seed value.    */
/*           uint32_t      *pCacheCapacity   - initial capacity of   */
/*                                             sublist cache.        */
/*                                                                   */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int ProcessArgs(int argc, char **argv,
                scenarioFunc *pScenario,
                bool *pSubsWait, bool *pCpuPin, int32_t *pVerbosity,
                ism_time_t *pSeedVal,
                uint32_t *pCacheCapacity,
                char **pMonitorTopic)
{
    int rc = OK;
    int validargs = 0;
    int invalidargs = 0;
    int i;

    for (i=1; i<argc; i++)
    {
        char *argp = argv[i];
        int oldinvalidargs = invalidargs;

        if (argp[0]=='-')
        {
            if (argp[1] == 's')
            {
                if (i+1 < argc)
                {
                    argp = argv[++i];

                    int numeric = atoi(argp);

                    if (strcmpi(argp, "FAN-OUT") == 0 || numeric == 1)
                    {
                        *pScenario = Scenario1;
                        validargs++;
                    }
                    else if (strcmpi(argp, "MULTI1") == 0 || numeric == 2)
                    {
                        *pScenario = Scenario2;
                        validargs++;
                    }
                    else if (strcmpi(argp, "FAN-OUT-BG") == 0 || numeric == 3)
                    {
                        *pScenario = Scenario3;
                        validargs++;
                    }
                    else if (strcmpi(argp, "FAN-IN") == 0 || numeric == 4)
                    {
                        *pScenario = Scenario4;
                        validargs++;
                    }
                    else if (strcmpi(argp, "MULTI1-BG") == 0 || numeric == 5)
                    {
                        *pScenario = Scenario5;
                        validargs++;
                    }
                    else if (strcmpi(argp, "MULTI2") == 0 || numeric == 6)
                    {
                        *pScenario = Scenario6;
                        validargs++;
                    }
                    else if (strcmpi(argp, "FAN-OUT-BG-R") == 0 || numeric == 7)
                    {
                        *pScenario = Scenario7;
                        validargs++;
                    }
                    else if (strcmpi(argp, "FAN-OUT-BG-R2") == 0 || numeric == 8)
                    {
                        *pScenario = Scenario8;
                        validargs++;
                    }
                    else if (strcmpi(argp, "FAN-OUT-BG-R3") == 0 || numeric == 9)
                    {
                        *pScenario = Scenario9;
                        validargs++;
                    }
                    else if (strcmpi(argp, "FAN-OUT-BG-R4") == 0 || numeric == 10)
                    {
                        *pScenario = Scenario10;
                        validargs++;
                    }
                    else if (strcmpi(argp, "FAN-OUT-BG-R4-QOS1") == 0 || numeric == 11)
                    {
                        *pScenario = Scenario11;
                        validargs++;
                    }
                    else if (strcmpi(argp, "BIG-FAN-OUT") == 0 || numeric == 12)
                    {
                        *pScenario = Scenario12;
                        validargs++;
                    }
                    else if (strcmpi(argp, "FAN-OUT-QOS1") == 0 || numeric == 13)
                    {
                        *pScenario = Scenario13;
                        validargs++;
                    }
                    else if (strcmpi(argp, "FAN-MIX-QOS1") == 0 || numeric == 14)
                    {
                        *pScenario = Scenario14;
                        validargs++;
                    }
                    else if (strcmpi(argp, "SPRAY-QOS0") == 0 || numeric == 15)
                    {
                        *pScenario = Scenario15;
                        validargs++;
                    }
                    else if (strcmpi(argp, "555-QOS0") == 0 || numeric == 16)
                    {
                        *pScenario = Scenario16;
                        validargs++;
                    }
                    else if (strcmpi(argp, "555-QOS1") == 0 || numeric == 17)
                    {
                        *pScenario = Scenario17;
                        validargs++;
                    }
                    else invalidargs++;
                }
                else invalidargs++;
            }
            else if (argp[1] == 'w')
            {
                if (i+1 < argc)
                {
                    argp = argv[++i];
                    *pScenario = WorkloadSelection;
                    workloads = strdup(argp);
                    validargs++;
                }
                else invalidargs++;
            }
            else if (argp[1] == 'r')
            {
                if (i+1 < argc)
                {
                    argp = argv[++i];

                    *pSeedVal = strtoull(argp,NULL,0);

                    validargs++;
                }
                else invalidargs++;
            }
            else if (argp[1] == 'v')
            {
                if (i+1 < argc)
                {
                    argp = argv[++i];

                    *pVerbosity = atoi(argp);

                    validargs++;
                }
                else invalidargs++;
            }
            else if (argp[1] == 'c')
            {
                if (i+1 < argc)
                {
                    argp = argv[++i];

                    *pCacheCapacity = (uint32_t)atoi(argp);

                    validargs++;
                }
                else invalidargs++;
            }
            else if (argp[1] == 'm')
            {
                if (i+1 < argc)
                {
                    argp = argv[++i];

                    *pMonitorTopic = strdup(argp);

                    validargs++;
                }
                else invalidargs++;
            }
            else if (strcmpi(argp,"-bgd") == 0)
            {
                if (i+1 < argc)
                {
                    argp = argv[++i];
                    bgSubberDelay = atoi(argp);
                    validargs++;
                }
                else invalidargs++;
            }
            else if (strcmpi(argp,"--nowait") == 0)
            {
                *pSubsWait = false;
                validargs++;
            }
            else if (strcmpi(argp,"--pin") == 0)
            {
                *pCpuPin = true;
                validargs++;
            }
            else invalidargs++;
        }
        else invalidargs++;

        if (invalidargs > oldinvalidargs)
        {
            printf("Invalid arg: %s\n", argp);
        }
    }

    if(validargs == 0 || invalidargs > 0)
    {
        printf("USAGE: %s [-s <SCENARIO> | -w <WORKLOADS>] [-r <RANDOM_SEED>] [-bgd <BG_SUB_DELAY>]\n",
               strrchr(argv[0],'/')?strrchr(argv[0],'/')+1:argv[0]);
        printf("     : [-v <VERBOSITY>] [-c <CACHE_CAPACITY>] [-m <MONITOR_TOPIC>] [--pin] [--nowait] [--emptylists]\n");
        printf("Where:\n");
        printf(" SCENARIO is one of:\n");
        printf("   1, FAN-OUT            - QoS0 1-10 fan-out of messages on a single topic.\n");
        printf("   2, MULTI1             - Complex multi-wildcard subscriptions.\n");
        printf("   3, FAN-OUT-BG         - QoS0 1-10 fan-out on a single topic string, background subs on non-conflicting topic.\n");
        printf("   4, FAN-IN             - QoS0 10-1 fan-in on a single topic.\n");
        printf("   5, MULTI1-BG          - Complex multi-wildcard subscriptions, background subs on non-conflicting topic\n");
        printf("   6, MULTI2             - Complex multi-wildcard subscriptions - many single node matches.\n");
        printf("   7, FAN-OUT-BG-R       - QoS0 1-10 fan-out on a single topic string, background subs on non-conflicting topic with retained.\n");
        printf("   8, FAN-OUT-BG-R2      - QoS0 1-10 fan-out on a single topic string, wildcard background subs on non-conflicting topic with multiple retaineds.\n");
        printf("   9, FAN-OUT-BG-R3      - QoS0 1-10 fan-out on a single topic string, wildcard background subs on non-conflicting topic with multiple retaineds, one being continually updated.\n");
        printf("  10, FAN-OUT-BG-R4      - QoS0 1-10 fan-out on a single topic string, wildcard background subs on non-conflicting topic with multiple retaineds, one being continually updated with persistent messages.\n");
        printf("  11, FAN-OUT-BG-R4-QOS1 - QoS0/1 1-10 fan-out on a single topic string, wildcard background subs on non-conflicting topic with multiple retaineds, one being continually updated with persistent messages.\n");
        printf("  12, BIG-FAN_OUT        - QoS0 1-50 fan-out of message on simple topic.\n");
        printf("  13, FAN-OUT-QOS1       - QoS1 1-10 fan-out of messages on a single topic.\n");
        printf("  14, FAN-MIX-QOS1       - QoS1 4-10 fan-in/out of messages on a single topic.\n");
        printf("  15, SPRAY-QOS0         - QoS0 Sprayer.\n");
        printf("  16, 555-QOS0           - QoS0 5:5:5 (5 Pubbers to 5 Subbbers, on 5 Topics) with persistent msgs.\n");
        printf("  17, 555-QOS1           - QoS1 5:5:5 (5 Pubbers to 5 Subbbers, on 5 Topics) with persistent msgs.\n");
        printf("  If not specified, the default is 10 'FAN-OUT-BG-R4'.\n");
        printf(" WORKLOADS is a list of comma separated workload numbers to add\n");
        printf(" RANDOM_SEED is an optional seed for the random number\n");
        printf(" BG_SUB_DELAY is the delay in milliseconds between background subscriber creation/deletion\n");
        printf(" VERBOSITY is the level of output from 1-5 [def: 1]\n");
        printf("\n");

        rc = 1;
    }

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main program entry point                          */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Function: Process args and call appropriate scenario              */
/*                                                                   */
/* Input:    int    argc - number of arguments                       */
/*           char **argv - string arguments                          */
/*                                                                   */
/* Output:   None                                                    */
/*                                                                   */
/* Returns:  Return code.                                            */
/*                                                                   */
/* Exit Normal:       OK.                                            */
/*                                                                   */
/* Exit Error:        non-OK.                                        */
/*                                                                   */
/*********************************************************************/
/*                                                                   */
/* Notes:                                                            */
/*                                                                   */
/*********************************************************************/
int main(int argc, char **argv)
{
    scenarioFunc pScenario = Scenario10;
    int rc = OK;

    ism_time_t seedVal;

    int32_t verbosity = 1;
    int trclvl = 0;
    uint32_t cacheCapacity = ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY;

    bool resourceSetStatsEnabled = false;
    bool resourceSetStatsTrackUnmatched = false;

    seedVal = ism_common_currentTimeNanos();
    srand(seedVal);

    // 90% of the time, turn on resourceSetStats
    if ((rand()%100) >= 10)
    {
        setenv("IMA_TEST_RESOURCESET_CLIENTID_REGEX", "^[^:]+:([^:]+):", false);
        setenv("IMA_TEST_RESOURCESET_TOPICSTRING_REGEX", "^[^/]+/([^/]+)", false);
        setenv("IMA_TEST_PROTOCOL_ALLOW_MQTT_PROXY", "true", false);

        resourceSetStatsEnabled = true;
        resourceSetStatsTrackUnmatched = ((rand()%100) >= 20); // 80% of the time track unmatched
        setenv("IMA_TEST_RESOURCESET_TRACK_UNMATCHED", resourceSetStatsTrackUnmatched ? "true" : "false", false);
    }

    rc = test_processInit(trclvl, NULL);
    if (rc != OK) goto mod_exit;

    if (argc > 1)
    {
        rc = ProcessArgs(argc, argv, &pScenario, &subsWait, &cpuPin,
                         &verbosity, &seedVal, &cacheCapacity, &monitorTopic);

        if (rc != OK)
        {
            printf("ERROR: process_args returned %d\n", rc);
            goto mod_exit;
        }
    }
    // Invoked with no arguments, include topic monitoring on the root of the tree
    else
    {
        monitorTopic = "#";
    }

    if (verbosity >= 5) ism_common_setTraceLevel(verbosity);

    // Use unlimited default queue maxMessageCount
    iepiPolicyInfo_t *defPolicy = iepi_getDefaultPolicyInfo(false);

    defPolicy->maxMessageCount = 0;

    rc = test_engineInit(true, true,
                         ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                         false, /*recovery should complete ok*/
                         cacheCapacity,
                         testDEFAULT_STORE_SIZE);
    if (rc != OK) goto mod_exit;

    printf("Random seed: %"PRId64", SubsWait: %s, CpuPin: %s\n",
            seedVal, subsWait?"true":"false", cpuPin?"true":"false");

    srand(seedVal);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    char *setDefSelRule = getenv("IMA_TEST_DEFAULT_SELECTION_RULE");

    if (setDefSelRule != NULL)
    {
        bool updated = false;
        rc = iepi_setDefaultSelectorRule(pThreadData, defPolicy, setDefSelRule, &updated);
        if (rc != OK || updated == false)
        {
            if (rc == OK) rc = ISMRC_Error;
            printf("ERROR: Couldn't set default selector rule on defpolicy rc %d\n", rc);
            goto mod_exit;
        }
    }

    rc = (*pScenario)(verbosity);
    if (rc != OK && rc != ISMRC_NotImplemented)
    {
        printf("ERROR: Scenario ended with rc %d\n", rc);
        goto mod_exit;
    }

    ismEngine_ResourceSetMonitor_t *results = NULL;
    uint32_t resultCount = 0;

    rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS, 10, NULL);
    if (rc != OK) goto mod_exit;

    if (resourceSetStatsEnabled)
    {
        uint32_t expectResultCount = resourceSetStatsTrackUnmatched ? 2 : 1;
        if (resultCount != expectResultCount)
        {
            printf("ERROR: ResultCount %u should be %u\n", resultCount, expectResultCount);
            rc = ISMRC_Error;
            goto mod_exit;
        }

        for(int32_t r=0; r<resultCount; r++)
        {
            if (strcmp(results[r].resourceSetId, iereDEFAULT_RESOURCESET_ID) == 0)
            {
                iereResourceSetHandle_t defaultResourceSet = iere_getDefaultResourceSet();
                if (results[r].resourceSet != defaultResourceSet)
                {
                    printf("ERROR: Default resource set pointer %p doesn't match expected %p\n",
                           results[r].resourceSet, defaultResourceSet);
                }

                if (results[r].stats.Connections != 0)
                {
                    printf("ERROR: Expected default resource %ld connection count, should be 0\n",
                           results[r].stats.Connections);
                    rc = ISMRC_Error;
                    goto mod_exit;
                }
            }
            else
            {
                if (strcmp(results[r].resourceSetId, TPT_RESOURCE_SET_ID) != 0)
                {
                    printf("ERROR: Unexpected resourceSetId '%s' should be '%s'\n",
                           results[r].resourceSetId, TPT_RESOURCE_SET_ID);
                    rc = ISMRC_Error;
                    goto mod_exit;
                }

                if (results[r].stats.Connections == 0)
                {
                    printf("ERROR: Connection count %ld should be >0\n", results[r].stats.Connections);
                    rc = ISMRC_Error;
                    goto mod_exit;
                }
            }

#if 1
            iereResourceSet_t *resourceSet = (iereResourceSet_t *)(results[r].resourceSet);
            printf("ResourceSetStats for setId '%s' {", resourceSet->stats.resourceSetIdentifier);
            for(int32_t ires=0; ires<ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS; ires++)
            {
                printf("%ld%c", resourceSet->stats.int64Stats[ires], ires == ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS-1 ? '}':',');
            }
            printf("\n");
#endif
        }
    }
    else if (resultCount != 0)
    {
        rc = ISMRC_Error;
        goto mod_exit;
    }

    ism_engine_freeResourceSetMonitor(results);

    rc = test_engineTerm(true);
    if (rc != OK) goto mod_exit;

    rc = test_processTerm(rc == OK);
    if (rc != OK) goto mod_exit;

mod_exit:

    return rc;
}
