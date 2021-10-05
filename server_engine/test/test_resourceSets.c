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
/* Module Name: test_resourceSets.c                                  */
/*                                                                   */
/* Description: Main source file for CUnit test of Resource Sets     */
/*                                                                   */
/*********************************************************************/
#include <math.h>
#include <stdbool.h>

#include "engine.h"
#include "engineInternal.h"
#include "memHandler.h"
#include "resourceSetStats.h"
#include "resourceSetStatsInternal.h"
#include "engineDiag.h"

#include "test_utils_phases.h"
#include "test_utils_initterm.h"
#include "test_utils_client.h"
#include "test_utils_message.h"
#include "test_utils_assert.h"
#include "test_utils_sync.h"
#include "test_utils_config.h"

bool fullTest = false;
bool trackUnmatched = false;
uint32_t testNumber = 0;

void test_MapStatTypeToMonitorType(void)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    ismEngineMonitorType_t monitorType,roundTripMonitorType;
    const char *statType;

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, "NOTASTATTYPE", false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_NONE);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, "None", false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_NONE);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEHOURLY, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_NONE);
    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEHOURLY, true);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_INTERNAL_FAKEHOURLY);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEHOURLY);
    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEDAILY, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_NONE);
    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEDAILY, true);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_INTERNAL_FAKEDAILY);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEDAILY);
    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEWEEKLY, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_NONE);
    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEWEEKLY, true);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_INTENAL_FAKEWEEKLY);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEWEEKLY);

    // Case insensitive
    monitorType = iere_mapStatTypeToMonitorType(pThreadData, "totalmemoryBYTEShighest", false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_TOTALMEMORYBYTES_HIGHEST);
    roundTripMonitorType = iere_mapStatTypeToMonitorType(pThreadData, statType, false);
    TEST_ASSERT_EQUAL(monitorType, roundTripMonitorType);

    // Numeric
    monitorType = iere_mapStatTypeToMonitorType(pThreadData, "44", false);
    TEST_ASSERT_EQUAL(monitorType, 44);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_TOTALMEMORYBYTES_HIGHEST);
    roundTripMonitorType = iere_mapStatTypeToMonitorType(pThreadData, statType, false);
    TEST_ASSERT_EQUAL(monitorType, roundTripMonitorType);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, "-2", false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_NONE);
    monitorType = iere_mapStatTypeToMonitorType(pThreadData, "9999", true);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_NONE);
    monitorType = iere_mapStatTypeToMonitorType(pThreadData, "9999", false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_NONE);

    // Others
    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_ALLUNSORTED, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_ALL_UNSORTED);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_ALLUNSORTED);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGBYTES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_BUFFEREDMSGBYTES_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PERSISTENTBUFFEREDMSGBYTES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PERSISTENTBUFFEREDMSGBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PERSISTENTBUFFEREDMSGBYTES_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTBUFFEREDMSGBYTES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_NONPERSISTENTBUFFEREDMSGBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTBUFFEREDMSGBYTES_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGBYTES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDMSGBYTES_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGBYTES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS0MSGBYTES_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGBYTES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS1MSGBYTES_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGBYTES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PUBLISHEDQOS2MSGBYTES_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_MAXPUBLISHRECIPIENTS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_MAXPUBLISHRECIPIENTS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_MAXPUBLISHRECIPIENTS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_SUBSCRIPTIONS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_SUBSCRIPTIONS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PERSISTENTNONSHAREDSUBSCRIPTIONS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_NONPERSISTENTNONSHAREDSUBSCRIPTIONS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTNONSHAREDSUBSCRIPTIONS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PERSISTENTSHAREDSUBSCRIPTIONS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_NONPERSISTENTSHAREDSUBSCRIPTIONS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTSHAREDSUBSCRIPTIONS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_DISCARDEDMSGS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_DISCARDEDMSGS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_REJECTEDMSGS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_REJECTEDMSGS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_RETAINEDMSGS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_RETAINEDMSGS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_RETAINEDMSGS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_RETAINEDMSGBYTES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_RETAINEDMSGBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_RETAINEDMSGBYTES_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_WILLMSGS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_WILLMSGS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_WILLMSGS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_WILLMSGBYTES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_WILLMSGBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_WILLMSGBYTES_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PERSISTENTWILLMSGBYTES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PERSISTENTWILLMSGBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PERSISTENTWILLMSGBYTES_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTWILLMSGBYTES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_NONPERSISTENTWILLMSGBYTES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_NONPERSISTENTWILLMSGBYTES_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_CONNECTIONS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_CONNECTIONS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_CONNECTIONS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_ACTIVECLIENTS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_ACTIVECLIENTS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_ACTIVEPERSISTENTCLIENTS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_ACTIVEPERSISTENTCLIENTS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_ACTIVEPERSISTENTCLIENTS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_ACTIVENONPERSISTENTCLIENTS_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_ACTIVENONPERSISTENTCLIENTS);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_ACTIVENONPERSISTENTCLIENTS_HIGHEST);

    monitorType = iere_mapStatTypeToMonitorType(pThreadData, ismENGINE_MONITOR_STATTYPE_PERSISTENTCLIENTSTATES_HIGHEST, false);
    TEST_ASSERT_EQUAL(monitorType, ismENGINE_MONITOR_HIGHEST_PERSISTENTCLIENTSTATES);
    statType = iere_mapMonitorTypeToStatType(pThreadData, monitorType);
    TEST_ASSERT_STRINGS_EQUAL(statType, ismENGINE_MONITOR_STATTYPE_PERSISTENTCLIENTSTATES_HIGHEST);
}

typedef struct tag_test_orgInfo_t
{
    char *orgName;
    uint32_t npClientCount;
    ismEngine_ClientStateHandle_t *npClients;
    ismEngine_SessionHandle_t *npSessions;
    uint32_t willMsgs;
} test_orgInfo_t;

#define ORGNAME_PATTERN    "org%03u"
#define CLIENTID_PATTERN   "t:%s:client%03u"
#define WILLTOPIC_PATTERN  "iot-2/%s/willmsg/%s"

void test_createOrg(test_orgInfo_t *orgInfo)
{
    int32_t rc;

    // NONPERSISTENT CLIENTS
    if (orgInfo->npClientCount != 0)
    {
        orgInfo->npClients = malloc(orgInfo->npClientCount*sizeof(ismEngine_ClientStateHandle_t));
        orgInfo->npSessions = malloc(orgInfo->npClientCount*sizeof(ismEngine_SessionHandle_t));

        for(uint32_t npClientNum=1; npClientNum <= orgInfo->npClientCount; npClientNum++)
        {
            char clientId[strlen(CLIENTID_PATTERN)+strlen(orgInfo->orgName)+1];

            sprintf(clientId, CLIENTID_PATTERN, orgInfo->orgName, npClientNum);

            rc = test_createClientAndSessionWithProtocol(clientId,
                                                         PROTOCOL_ID_MQTT,
                                                         NULL,
                                                         ismENGINE_CREATE_CLIENT_OPTION_NONE,
                                                         ismENGINE_CREATE_SESSION_OPTION_NONE,
                                                         &orgInfo->npClients[npClientNum-1],
                                                         &orgInfo->npSessions[npClientNum-1],
                                                         true);
            TEST_ASSERT_EQUAL(rc, OK);

            if (orgInfo->willMsgs != 0)
            {
                ismEngine_MessageHandle_t hMsg;
                size_t  size = 20;
                uint8_t persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
                uint8_t reliability = ismMESSAGE_RELIABILITY_AT_MOST_ONCE;
                uint8_t flags = ismMESSAGE_FLAGS_NONE;

                switch(orgInfo->willMsgs)
                {
                    case 1:
                        persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
                        reliability = ismMESSAGE_RELIABILITY_AT_MOST_ONCE;
                        flags = ismMESSAGE_FLAGS_NONE;
                        break;
                    case 2:
                        persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
                        reliability = ismMESSAGE_RELIABILITY_AT_MOST_ONCE;
                        flags = ismMESSAGE_FLAGS_NONE;
                        break;
                    case 3:
                        persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
                        reliability = ismMESSAGE_RELIABILITY_AT_LEAST_ONCE;
                        flags = ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;
                        break;
                    case 4:
                        persistence = ismMESSAGE_PERSISTENCE_NONPERSISTENT;
                        reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
                        flags = ismMESSAGE_FLAGS_NONE;
                        break;
                    default:
                        printf("ORGNAME: %s\n", orgInfo->orgName);
                        TEST_ASSERT_GREATER_THAN(4, orgInfo->willMsgs);
                        break;
                }

                char willtopic[strlen(WILLTOPIC_PATTERN)+strlen(orgInfo->orgName)+strlen(clientId)+1];
                sprintf(willtopic, WILLTOPIC_PATTERN, orgInfo->orgName, clientId);

                rc = test_createMessage(size,
                                        persistence,
                                        reliability,
                                        (flags & ~ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN),
                                        0,
                                        ismDESTINATION_TOPIC, willtopic,
                                        &hMsg, NULL);
                TEST_ASSERT_EQUAL(rc, OK);

                // Really want retained
                if (flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN)
                {
                    hMsg->Header.Flags |= ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN;
                }

                rc = ism_engine_setWillMessage(orgInfo->npClients[npClientNum-1],
                                               ismDESTINATION_TOPIC, willtopic,
                                               hMsg, 0, iecsWILLMSG_TIME_TO_LIVE_INFINITE,
                                               NULL, 0, NULL);
                TEST_ASSERT_EQUAL(rc, OK);
            }
        }
    }
}

void test_destroyOrg(test_orgInfo_t *orgInfo, bool destroyDurable, bool freeMembers)
{
    int32_t rc;

    // NONPERSISTENT CLIENTS
    for(uint32_t npClientNum=1; npClientNum <= orgInfo->npClientCount; npClientNum++)
    {
        ismEngine_ClientStateHandle_t hClient = orgInfo->npClients[npClientNum-1];

        if (hClient != NULL)
        {
            rc = sync_ism_engine_destroyClientState(hClient, ismENGINE_DESTROY_CLIENT_OPTION_NONE);
            TEST_ASSERT_EQUAL(rc, OK);
            orgInfo->npClients[npClientNum-1] = NULL;
        }

    }

    if (freeMembers)
    {
        free(orgInfo->npClients);
        free(orgInfo->orgName);
    }
}

// This phase just sets up the resource set descriptor for the next phases...
void test_SetMix1_Phase1(void)
{
    int32_t rc;

    rc = test_configProcessPost("{\"ResourceSetDescriptor\": {\"ClientID\": \"^[^:]+:([^:]+):\", \"Topic\": \"^iot[^/]+/([^/+#]+)/\"}}");

    TEST_ASSERT(rc == OK, ("ERROR: test_configProcessPost returned %d", rc));
}

#define SETMIX1_ORGCOUNT 15
#define WILLMSG_TYPES    5
void test_SetMix1_Phase2(void)
{
    int32_t rc;
    test_orgInfo_t orgInfo[SETMIX1_ORGCOUNT] = {{0}};

    // Must be a multiple of WILLMSG_TYPES for the following
    TEST_ASSERT_EQUAL(SETMIX1_ORGCOUNT%WILLMSG_TYPES, 0);

    test_orgInfo_t *expectHighestOrg[ismENGINE_MONITOR_MAX];

    // Most stats, the highest org will be the last one for others
    // we update as we set up the orgInfo structures
    for(uint32_t statNum=0; statNum<ismENGINE_MONITOR_MAX; statNum++)
    {
        expectHighestOrg[statNum] = &orgInfo[SETMIX1_ORGCOUNT-1];
    }

    // Call with no resources, expect only the default.
    ismEngine_ResourceSetMonitor_t *results = NULL;
    uint32_t resultCount = 0;

    rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_HIGHEST_CONNECTIONS, 10, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);
    TEST_ASSERT_STRINGS_EQUAL(results[0].resourceSetId, iereDEFAULT_RESOURCESET_ID);
    TEST_ASSERT_NOT_EQUAL(results[0].resetTime, 0);

    ism_engine_freeResourceSetMonitor(results);

    // Call with invalid type
    results = NULL;
    rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_HIGHEST_CONSUMERS, 10, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);
    TEST_ASSERT_PTR_NULL(results);

    // Call with invalid maxResults
    rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_HIGHEST_CONNECTIONS, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_InvalidParameter);
    TEST_ASSERT_PTR_NULL(results);

    for(uint32_t orgNum=0; orgNum<SETMIX1_ORGCOUNT; orgNum++)
    {
        test_orgInfo_t *thisOrgInfo = &orgInfo[orgNum];

        thisOrgInfo->orgName = malloc(strlen(ORGNAME_PATTERN)+1);
        sprintf(thisOrgInfo->orgName, ORGNAME_PATTERN, orgNum+1);
        thisOrgInfo->npClientCount = orgNum * 5;
        thisOrgInfo->willMsgs = (orgNum % WILLMSG_TYPES);
        switch(thisOrgInfo->willMsgs)
        {
            case 2:
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGS] = thisOrgInfo;
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGBYTES] = thisOrgInfo;
                break;
            case 3:
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_PERSISTENTWILLMSGBYTES] = thisOrgInfo;
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGS] = thisOrgInfo;
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGBYTES] = thisOrgInfo;
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_RETAINEDMSGS] = thisOrgInfo;
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_RETAINEDMSGBYTES] = thisOrgInfo;
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES] = thisOrgInfo;           // Retained count
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_PERSISTENTBUFFEREDMSGBYTES] = thisOrgInfo; // Retained count
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGBYTES] = thisOrgInfo;          // Retained msgs are bigger!!
                break;
            case 4:
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_NONPERSISTENTWILLMSGBYTES] = thisOrgInfo;
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGS] = thisOrgInfo;
                expectHighestOrg[ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGBYTES] = thisOrgInfo;
                break;
            default:
                break;
        }
        test_createOrg(&orgInfo[orgNum]);
    }

    // Test the diagnostics function
    char *outputString = NULL;

    rc =  ism_engine_diagnostics(ediaVALUE_MODE_DUMPRESOURCESETS,
                                 "testDUMPRESSETS",
                                 &outputString,
                                 NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(outputString);

    ism_engine_freeDiagnosticsOutput(outputString);

    ismEngineMonitorType_t *thisType;
    iereResourceSet_I64_StatType_t *thisStat;

    // Check a set of monitoring requests
    ismEngineMonitorType_t testTypesA[] = {ismENGINE_MONITOR_HIGHEST_CONNECTIONS,
                                           ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS,
                                           ismENGINE_MONITOR_HIGHEST_ACTIVENONPERSISTENTCLIENTS,
                                           ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES,
                                           ismENGINE_MONITOR_HIGHEST_WILLMSGS,
                                           ismENGINE_MONITOR_NONE};
    iereResourceSet_I64_StatType_t testStatsA[] = {ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_CONNECTIONS,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_ACTIVE_NONPERSISTENT_CLIENTS,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_MEMORY,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_WILLMSGS};

    for(thisType = testTypesA, thisStat = testStatsA; *thisType != ismENGINE_MONITOR_NONE; thisType++, thisStat++)
    {
        uint32_t maxResults = 1+(rand()%SETMIX1_ORGCOUNT);

        rc = ism_engine_getResourceSetMonitor(&results, &resultCount, *thisType, maxResults, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, maxResults);

        //printf("*thisType=%d *thisStat=%d\n", *thisType, *thisStat);
        TEST_ASSERT_STRINGS_EQUAL(results[0].resourceSetId, expectHighestOrg[*thisType]->orgName);

        // Make sure the stats are in order.
        if (*thisStat != ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS)
        {
            for(int32_t r=1; r<resultCount; r++)
            {
                TEST_ASSERT_GREATER_THAN_OR_EQUAL(((iereResourceSet_t *)results[r-1].resourceSet)->stats.int64Stats[*thisStat],
                                                  ((iereResourceSet_t *)results[r].resourceSet)->stats.int64Stats[*thisStat]);
                TEST_ASSERT_NOT_EQUAL(results[r].resetTime, 0);
            }
        }

        ism_engine_freeResourceSetMonitor(results);
    }

    // Check a set of monitoring requests where we cannot be sure which will be highest (cos memory) but can check order
    ismEngineMonitorType_t testTypesA2[] = {ismENGINE_MONITOR_HIGHEST_WILLMSGBYTES,
                                           ismENGINE_MONITOR_HIGHEST_PERSISTENTWILLMSGBYTES,
                                           ismENGINE_MONITOR_HIGHEST_NONPERSISTENTWILLMSGBYTES,
                                           ismENGINE_MONITOR_NONE};
    iereResourceSet_I64_StatType_t testStatsA2[] = {ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS,
                                                    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_WILLMSG_BYTES,
                                                    ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_WILLMSG_BYTES};

    for(thisType = testTypesA2, thisStat = testStatsA2; *thisType != ismENGINE_MONITOR_NONE; thisType++, thisStat++)
    {
        uint32_t maxResults = 1+(rand()%SETMIX1_ORGCOUNT);
        rc = ism_engine_getResourceSetMonitor(&results, &resultCount, *thisType, maxResults, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, maxResults);

        //printf("*thisType=%d *thisStat=%d\n", *thisType, *thisStat);

        // Make sure the stats are in order.
        if (*thisStat != ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS)
        {
            for(int32_t r=1; r<resultCount; r++)
            {
                TEST_ASSERT_GREATER_THAN_OR_EQUAL(((iereResourceSet_t *)results[r-1].resourceSet)->stats.int64Stats[*thisStat],
                                                  ((iereResourceSet_t *)results[r].resourceSet)->stats.int64Stats[*thisStat]);
                TEST_ASSERT_NOT_EQUAL(results[r].resetTime, 0);
            }
        }

        ism_engine_freeResourceSetMonitor(results);
    }

    // Things where all stats should be zero.
    ismEngineMonitorType_t testTypesB[] = {ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS,
                                           ismENGINE_MONITOR_HIGHEST_NONPERSISTENTNONSHAREDSUBSCRIPTIONS,
                                           ismENGINE_MONITOR_HIGHEST_NONPERSISTENTSHAREDSUBSCRIPTIONS,
                                           ismENGINE_MONITOR_HIGHEST_PERSISTENTNONSHAREDSUBSCRIPTIONS,
                                           ismENGINE_MONITOR_HIGHEST_PERSISTENTSHAREDSUBSCRIPTIONS,
                                           ismENGINE_MONITOR_HIGHEST_ACTIVEPERSISTENTCLIENTS,
                                           ismENGINE_MONITOR_HIGHEST_PERSISTENTCLIENTSTATES,
                                           ismENGINE_MONITOR_HIGHEST_MAXPUBLISHRECIPIENTS,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES,
                                           ismENGINE_MONITOR_HIGHEST_DISCARDEDMSGS,
                                           ismENGINE_MONITOR_HIGHEST_REJECTEDMSGS,
                                           ismENGINE_MONITOR_NONE};

    for(thisType = testTypesB; *thisType != ismENGINE_MONITOR_NONE; thisType++)
    {
        uint32_t maxResults = 1+(rand()%SETMIX1_ORGCOUNT);

        rc = ism_engine_getResourceSetMonitor(&results, &resultCount, *thisType, maxResults, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, maxResults);

        switch(*thisType)
        {
            case ismENGINE_MONITOR_HIGHEST_SUBSCRIPTIONS:
                TEST_ASSERT_EQUAL(results[0].stats.Subscriptions, 0);
                break;
            case ismENGINE_MONITOR_HIGHEST_PERSISTENTNONSHAREDSUBSCRIPTIONS:
                TEST_ASSERT_EQUAL(results[0].stats.PersistentNonSharedSubscriptions, 0);
                break;
            default:
                break;
        }

        ism_engine_freeResourceSetMonitor(results);
    }

    // Query subscriptions for non-existent resource set
    ismEngine_SubscriptionMonitor_t *subResults;
    ism_prop_t *filterProperties = ism_common_newProperties(10);
    ism_field_t f;

    f.type = VT_String;
    f.val.s = "NORESOURCESET";
    rc = ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_RESOURCESET_ID, &f);
    TEST_ASSERT_EQUAL(rc, OK);
    rc = ism_engine_getSubscriptionMonitor(&subResults,
                                           &resultCount,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                           50,
                                           filterProperties);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    ism_engine_freeSubscriptionMonitor(subResults);

    // Destroy / disconnect org (but don't free member variables)
    for(uint32_t orgNum=0; orgNum<SETMIX1_ORGCOUNT; orgNum++)
    {
        test_destroyOrg(&orgInfo[orgNum], false, false);
    }

    // Check stats that we expect to be there after disconnect
    ismEngineMonitorType_t testTypesC[] = {ismENGINE_MONITOR_HIGHEST_CONNECTIONS,
                                           ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                           ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGS,
                                           ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGS,
                                           ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGS,
                                           ismENGINE_MONITOR_HIGHEST_RETAINEDMSGS,
                                           ismENGINE_MONITOR_HIGHEST_RETAINEDMSGBYTES,
                                           ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES,
                                           ismENGINE_MONITOR_HIGHEST_PERSISTENTBUFFEREDMSGBYTES,
                                           ismENGINE_MONITOR_NONE};
    iereResourceSet_I64_StatType_t testStatsC[] = {ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_CONNECTIONS,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSGS_PUBLISHED,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSGS_PUBLISHED,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSGS_PUBLISHED,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSGS,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_RETAINEDMSG_BYTES,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS,
                                                   ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_BUFFEREDMSG_BYTES};

    for(thisType = testTypesC, thisStat = testStatsC; *thisType != ismENGINE_MONITOR_NONE; thisType++, thisStat++)
    {
        uint32_t maxResults = 100;

        rc = ism_engine_getResourceSetMonitor(&results, &resultCount, *thisType, maxResults, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, SETMIX1_ORGCOUNT); // 1st org does nothing, but also have __Default

        //printf("*thisType=%d *thisStat=%d\n", *thisType, *thisStat);

        TEST_ASSERT_STRINGS_EQUAL(results[0].resourceSetId, expectHighestOrg[*thisType]->orgName);

        // Make sure the stats are in order.
        if (*thisStat != ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS)
        {
            for(int32_t r=1; r<resultCount; r++)
            {
                TEST_ASSERT_GREATER_THAN_OR_EQUAL(((iereResourceSet_t *)results[r-1].resourceSet)->stats.int64Stats[*thisStat],
                                                  ((iereResourceSet_t *)results[r].resourceSet)->stats.int64Stats[*thisStat]);
            }
        }

        ism_engine_freeResourceSetMonitor(results);
    }

    // Check stats that we expect to be there after disconnect where we cannot be sure
    // which will be highest (cos memory)
    ismEngineMonitorType_t testTypesC2[] = {ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGBYTES,
                                            ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS0MSGBYTES,
                                            ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS1MSGBYTES,
                                            ismENGINE_MONITOR_HIGHEST_PUBLISHEDQOS2MSGBYTES,
                                            ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGBYTES,
                                            ismENGINE_MONITOR_NONE};
    iereResourceSet_I64_StatType_t testStatsC2[] = {ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS,
                                                    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS0_MSG_BYTES_PUBLISHED,
                                                    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS1_MSG_BYTES_PUBLISHED,
                                                    ISM_ENGINE_RESOURCESETSTATS_I64_COUNT_QOS2_MSG_BYTES_PUBLISHED,
                                                    ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS};

    for(thisType = testTypesC2, thisStat = testStatsC2; *thisType != ismENGINE_MONITOR_NONE; thisType++, thisStat++)
    {
        uint32_t maxResults = 5;

        rc = ism_engine_getResourceSetMonitor(&results, &resultCount, *thisType, maxResults, NULL);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_EQUAL(resultCount, maxResults);

        //printf("*thisType=%d *thisStat=%d\n", *thisType, *thisStat);

        // Make sure the stats are in order.
        if (*thisStat != ISM_ENGINE_RESOURCESETSTATS_I64_NUMSTATS)
        {
            for(int32_t r=1; r<resultCount; r++)
            {
                TEST_ASSERT_GREATER_THAN_OR_EQUAL(((iereResourceSet_t *)results[r-1].resourceSet)->stats.int64Stats[*thisStat],
                                                  ((iereResourceSet_t *)results[r].resourceSet)->stats.int64Stats[*thisStat]);
            }
        }

        ism_engine_freeResourceSetMonitor(results);
    }

    ism_common_clearProperties(filterProperties);

    // Things where all stats will *now* be zero.
    ismEngineMonitorType_t testTypesD[] = {ismENGINE_MONITOR_HIGHEST_WILLMSGS,
                                           ismENGINE_MONITOR_HIGHEST_WILLMSGBYTES,
                                           ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS,
                                           ismENGINE_MONITOR_HIGHEST_ACTIVENONPERSISTENTCLIENTS,
                                           ismENGINE_MONITOR_HIGHEST_NONPERSISTENTBUFFEREDMSGBYTES,
                                           ismENGINE_MONITOR_NONE};

    f.type = VT_String;
    f.val.s = "org*";
    ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_RESOURCESET_ID, &f);

    for(thisType = testTypesD; *thisType != ismENGINE_MONITOR_NONE; thisType++)

    {
        uint32_t maxResults = 1+(rand()%SETMIX1_ORGCOUNT);

        rc = ism_engine_getResourceSetMonitor(&results, &resultCount, *thisType, maxResults, filterProperties);
        TEST_ASSERT_EQUAL(rc, OK);
        TEST_ASSERT_GREATER_THAN_OR_EQUAL(maxResults, resultCount);

        switch(*thisType)
        {
            case ismENGINE_MONITOR_HIGHEST_WILLMSGS:
                TEST_ASSERT_EQUAL(results[0].stats.WillMsgs, 0);
                break;
            case ismENGINE_MONITOR_HIGHEST_WILLMSGBYTES:
                TEST_ASSERT_EQUAL(results[0].stats.WillMsgBytes, 0);
                break;
            case ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS:
                TEST_ASSERT_EQUAL(results[0].stats.ActiveClients, 0);
                break;
            case ismENGINE_MONITOR_HIGHEST_ACTIVENONPERSISTENTCLIENTS:
                TEST_ASSERT_EQUAL(results[0].stats.ActiveNonpersistentClients, 0);
                break;
            default:
                break;
        }

        ism_engine_freeResourceSetMonitor(results);
    }

    // Filter for a specific org that doesn't exist...
    f.type = VT_String;
    f.val.s = "NOTANORG";
    ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_RESOURCESET_ID, &f);

    rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES, 10, filterProperties);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 0);

    ism_engine_freeResourceSetMonitor(results);

    // Filter for the default resource set specifically...
    f.val.s = iereDEFAULT_RESOURCESET_ID;
    ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_RESOURCESET_ID, &f);

    rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES, 10, filterProperties);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);

    ism_engine_freeResourceSetMonitor(results);

    // Filter for a specific orgId that does exist...
    f.val.s = orgInfo[SETMIX1_ORGCOUNT-1].orgName;
    ism_common_setProperty(filterProperties, ismENGINE_MONITOR_FILTER_RESOURCESET_ID, &f);

    rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES, 10, filterProperties);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);
    TEST_ASSERT_STRINGS_EQUAL(results[0].resourceSetId, orgInfo[SETMIX1_ORGCOUNT-1].orgName);

    ism_time_t prevResetTime = results[0].resetTime;
    TEST_ASSERT_NOT_EQUAL(prevResetTime, 0);
    TEST_ASSERT_NOT_EQUAL(results[0].stats.PublishedMsgs, 0);

    ism_engine_freeResourceSetMonitor(results);

    // Test the reporting by faking the time etc
    iereResourceSetStatsControl_t *resourceSetStatsControl = ismEngine_serverGlobal.resourceSetStatsControl;
    iereReportingControl_t *reportingControl = &resourceSetStatsControl->reporting;
    int32_t prevCount;

    // Make sure the reporting thread is running
    while(reportingControl->threadHandle == 0) usleep(100);

    // Give the thread half a second to get into its waiting state...
    usleep(500000);

    char args[10];

    // Trick the code into thinking that a requested report is already in progress...
    TEST_ASSERT_EQUAL(reportingControl->requestedReportMonitorType, ismENGINE_MONITOR_NONE);
    reportingControl->requestedReportMonitorType = ismENGINE_MONITOR_HIGHEST_ACTIVECLIENTS;
    rc = ism_engine_diagnostics(ediaVALUE_MODE_RESOURCESETREPORT, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEDAILY, &outputString, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, ISMRC_ExistingKey);

    reportingControl->requestedReportMonitorType = ismENGINE_MONITOR_NONE;

    // Fake a daily report
    TEST_ASSERT_NOT_EQUAL(reportingControl->dailyReportMonitorType, ismENGINE_MONITOR_NONE);
    TEST_ASSERT_EQUAL(reportingControl->dailyReportResetStats, false);
    sprintf(args, "%u", ismENGINE_MONITOR_INTERNAL_FAKEDAILY);
    prevCount = reportingControl->reportingCyclesCompleted;

    // Use the basic diagnostic function to prompt the report
    outputString = NULL;

    rc =  ism_engine_diagnostics(ediaVALUE_MODE_RESOURCESETREPORT, args, &outputString, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(outputString);
    ism_engine_freeDiagnosticsOutput(outputString);

    // Wait for the the report to complete
    while(reportingControl->reportingCyclesCompleted == prevCount) usleep(100);
    prevCount = reportingControl->reportingCyclesCompleted;

    // Do it again, using string to request it...
    outputString = NULL;

    rc = ism_engine_diagnostics(ediaVALUE_MODE_RESOURCESETREPORT, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEDAILY, &outputString, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(outputString);
    ism_engine_freeDiagnosticsOutput(outputString);

    // Wait for the the report to complete
    while(reportingControl->reportingCyclesCompleted == prevCount) usleep(100);
    prevCount = reportingControl->reportingCyclesCompleted;

    // Fake an hourly report
    TEST_ASSERT_NOT_EQUAL(reportingControl->hourlyReportMonitorType, ismENGINE_MONITOR_NONE);
    TEST_ASSERT_EQUAL(reportingControl->hourlyReportResetStats, false);
    sprintf(args, "%u", ismENGINE_MONITOR_INTERNAL_FAKEHOURLY);

    // Use the basic diagnostic function to prompt the report
    outputString = NULL;

    rc =  ism_engine_diagnostics(ediaVALUE_MODE_RESOURCESETREPORT, args, &outputString, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(outputString);

    ism_engine_freeDiagnosticsOutput(outputString);

    // Wait for the report to complete
    while(reportingControl->reportingCyclesCompleted == prevCount) usleep(100);
    prevCount = reportingControl->reportingCyclesCompleted;

    // Do another hourly report that is configured to do nothing...
    ismEngineMonitorType_t prevType = reportingControl->hourlyReportMonitorType;
    reportingControl->hourlyReportMonitorType = ismENGINE_MONITOR_NONE;

    // Use the basic diagnostic function to prompt the report
    outputString = NULL;

    rc =  ism_engine_diagnostics(ediaVALUE_MODE_RESOURCESETREPORT, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEHOURLY, &outputString, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(outputString);

    ism_engine_freeDiagnosticsOutput(outputString);

    while(reportingControl->reportingCyclesCompleted == prevCount) usleep(100);
    prevCount = reportingControl->reportingCyclesCompleted;

    reportingControl->hourlyReportMonitorType = prevType;

    // Fake a weekly report (AND FORCE A RESET OF STATS)
    TEST_ASSERT_NOT_EQUAL(reportingControl->weeklyReportMonitorType, ismENGINE_MONITOR_NONE);
    TEST_ASSERT_EQUAL(reportingControl->weeklyReportResetStats, false);
    reportingControl->weeklyReportResetStats = true;

    // Use the basic diagnostic function to prompt the report
    outputString = NULL;

    rc =  ism_engine_diagnostics(ediaVALUE_MODE_RESOURCESETREPORT, ismENGINE_MONITOR_STATTYPE_INTERNAL_FAKEWEEKLY, &outputString, NULL, 0, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(outputString);

    ism_engine_freeDiagnosticsOutput(outputString);

    // Wait for the the report to complete
    while(reportingControl->reportingCyclesCompleted == prevCount) usleep(100);
    prevCount = reportingControl->reportingCyclesCompleted;

    // Check that the stats were reset by the weekly report
    rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_HIGHEST_TOTALMEMORYBYTES, 10, filterProperties);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_EQUAL(resultCount, 1);
    TEST_ASSERT_STRINGS_EQUAL(results[0].resourceSetId, orgInfo[SETMIX1_ORGCOUNT-1].orgName);
    TEST_ASSERT_NOT_EQUAL(results[0].resetTime, prevResetTime);
    TEST_ASSERT_EQUAL(results[0].stats.PublishedMsgs, 0);

    ism_engine_freeResourceSetMonitor(results);

    ism_common_freeProperties(filterProperties);

    // Clean up
    reportingControl->weeklyReportResetStats = false;

    for(uint32_t orgNum=0; orgNum<SETMIX1_ORGCOUNT; orgNum++)
    {
        test_destroyOrg(&orgInfo[orgNum], false, true);
    }

}

void test_SetMix1_Phase3(void)
{
    // TODO
}

CU_TestInfo ISM_ResourceSets_CUnit_test_SetMix1_Phase1[] =
{
    { "MapStatTypeToMonitorType", test_MapStatTypeToMonitorType },
    { "SetMix1", test_SetMix1_Phase1 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_ResourceSets_CUnit_test_SetMix1_Phase2[] =
{
    { "SetMix1", test_SetMix1_Phase2 },
    CU_TEST_INFO_NULL
};

CU_TestInfo ISM_ResourceSets_CUnit_test_SetMix1_Phase3[] =
{
    { "SetMix1", test_SetMix1_Phase3 },
    CU_TEST_INFO_NULL
};

// Test some other code paths not covered by other tests
void test_UpdateMessageResourceSet(void)
{
    int32_t rc;
    const char *clientId = "OtherCodeClient";

    ismEngine_MessageHandle_t hMessage = NULL;

    TEST_ASSERT_PTR_NOT_NULL(ismEngine_serverGlobal.resourceSetStatsControl);

    char topic[strlen(WILLTOPIC_PATTERN)+strlen(__func__)+strlen(clientId)+1];
    sprintf(topic, WILLTOPIC_PATTERN, __func__, clientId);
    rc = test_createMessage(TEST_SMALL_MESSAGE_SIZE,
                            ismMESSAGE_PERSISTENCE_NONPERSISTENT,
                            ismMESSAGE_RELIABILITY_AT_LEAST_ONCE,
                            ismMESSAGE_FLAGS_NONE,
                            0,
                            ismDESTINATION_TOPIC, topic,
                            &hMessage, NULL);
    TEST_ASSERT_EQUAL(rc, OK);
    TEST_ASSERT_PTR_NOT_NULL(hMessage);
    TEST_ASSERT_EQUAL(hMessage->resourceSet, iereNO_RESOURCE_SET);

    ieutThreadData_t *pThreadData = ieut_getThreadData();

    iereResourceSetHandle_t resourceSet = iere_getResourceSet(pThreadData, clientId, topic, iereOP_ADD);
    iere_updateMessageResourceSet(pThreadData, resourceSet, hMessage, true, false);
    TEST_ASSERT_EQUAL(hMessage->resourceSet, resourceSet);

    const char *resourceSetIdentifier = iere_getResourceSetIdentifier(resourceSet);
    TEST_ASSERT_STRINGS_EQUAL(resourceSetIdentifier, __func__);

    iereResourceSetHandle_t defaultResourceSet = iere_getDefaultResourceSet();
    TEST_ASSERT_NOT_EQUAL(defaultResourceSet, iereNO_RESOURCE_SET);

    // This should NOT change the resourceSet of the message...
    iere_updateMessageResourceSet(pThreadData, defaultResourceSet, hMessage, true, false);
    TEST_ASSERT_EQUAL(hMessage->resourceSet, resourceSet);

    // This should 'change' it to the same thing
    iere_updateMessageResourceSet(pThreadData, resourceSet, hMessage, false, true);
    TEST_ASSERT_EQUAL(hMessage->resourceSet, resourceSet);

    ism_engine_releaseMessage(hMessage);
}

CU_TestInfo ISM_ResourceSets_CUnit_test_OtherCodePaths[] =
{
    { "UpdateMessageResourceSet", test_UpdateMessageResourceSet },
    CU_TEST_INFO_NULL
};

/*********************************************************************/
/*                                                                   */
/* Function Name:  main                                              */
/*                                                                   */
/* Description:    Main entry point for the test.                    */
/*                                                                   */
/*********************************************************************/

int initResourceSets(void)
{
    return test_engineInit_COLD;
}

int initResourceSetsWarm(void)
{
    return test_engineInit_WARM;
}

int termResourceSets(void)
{
    return test_engineTerm(true);
}

int termResourceSetsWarm(void)
{
    return test_engineTerm(false);
}

CU_SuiteInfo ISM_ResourceSets_CUnit_phase1suites[] =
{
    IMA_TEST_SUITE("SetMix1", initResourceSets, termResourceSetsWarm, ISM_ResourceSets_CUnit_test_SetMix1_Phase1),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ResourceSets_CUnit_phase2suites[] =
{
    IMA_TEST_SUITE("SetMix1", initResourceSetsWarm, termResourceSetsWarm, ISM_ResourceSets_CUnit_test_SetMix1_Phase2),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo ISM_ResourceSets_CUnit_phase3suites[] =
{
    IMA_TEST_SUITE("SetMix1", initResourceSetsWarm, termResourceSetsWarm, ISM_ResourceSets_CUnit_test_SetMix1_Phase3),
    IMA_TEST_SUITE("OtherCodePaths", initResourceSetsWarm, termResourceSets, ISM_ResourceSets_CUnit_test_OtherCodePaths),
    CU_SUITE_INFO_NULL,
};

CU_SuiteInfo *PhaseSuites[] = { ISM_ResourceSets_CUnit_phase1suites,
                                ISM_ResourceSets_CUnit_phase2suites,
                                ISM_ResourceSets_CUnit_phase3suites
                              };

int main(int argc, char *argv[])
{
    // We want to gather resource set stats
    setenv("IMA_TEST_PROTOCOL_ALLOW_MQTT_PROXY", "true", false);
    trackUnmatched = ((rand()%100) >= 20);
    setenv("IMA_TEST_RESOURCESET_TRACK_UNMATCHED", trackUnmatched ? "true" : "false", true);

    time_t now_time = time(NULL);
    struct tm now_tm;
    struct tm *pNow_tm = localtime_r(&now_time, &now_tm);
    // Give ourselves ~59 minutes so we can prod the reporting thread and pretend it is different times...
    char minutes[20];
    if (pNow_tm != NULL)
    {
        if (now_tm.tm_min == 0)
            strcpy(minutes, "59");
        else
            sprintf(minutes, "%d", now_tm.tm_min-1);

        setenv("IMA_TEST_RESOURCESET_STATS_MINUTES_PAST_HOUR", minutes, true);
    }

    return test_utils_simplePhases(argc, argv, PhaseSuites);
}
