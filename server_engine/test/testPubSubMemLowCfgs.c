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
/* Module Name: testPubSubMemLowCfgs                                 */
/*                                                                   */
/* Description: UnitTests that pub/sub survives low memory scenarios */
/* A separate file to testPubSubCfgs as we don't want threads to be  */
/* successfully allocating memory from blocks they reserved in       */
/* earlier tests                                                     */
/*                                                                   */
/*********************************************************************/
#include <stdio.h>
#include <math.h>

#include "ismutil.h"
#include "engineInternal.h"
#include "engine.h"
#include "testPubSubThreads.h"
#include "policyInfo.h"
#include "resourceSetStats.h"
#include "clientState.h"

#include "test_utils_initterm.h"

#define TOPIC_FORMAT "test/"TPT_RESOURCE_SET_ID"/testMemoryDisable/QoS=%d/Txn=%d"

int testMemoryDisable(int QoS, transactionCapableQoS1 txnCapable) {
    printf("Starting testMemoryDisable...\n");

    int32_t rc = OK;
    char TopicString[256];
    int msgsperpubber = 100000;

    if (txnCapable != TxnCapable_False)
    {
        msgsperpubber = 10000;
    }
    else if (QoS > 0)
    {
        msgsperpubber = 40000;
    }

    sprintf(TopicString, TOPIC_FORMAT, QoS, txnCapable);
    rc = doPubs(TopicString, 3, msgsperpubber, 10, true, false, true, txnCapable, QoS);
    if (rc != OK) {
        printf("...test failed (doPubs(QoS=%d) returned %d\n)", QoS, rc);
        return rc;
    }

    printf("test passed.\n");
    return OK;
}


int main(int argc, char *argv[])
{
    int rc = OK;
    int failures=0;
    int trclvl = 0;
    bool resourceSetStatsEnabled = false;
    bool resourceSetStatsTrackUnmatched;

    ism_time_t seedVal = ism_common_currentTimeNanos();
    srand(seedVal);

    // 70% of the time, turn on resourceSetStats
    if ((rand()%100) >= 30)
    {
        setenv("IMA_TEST_RESOURCESET_CLIENTID_REGEX", "^[^:]+:([^:]+):", false);
        setenv("IMA_TEST_RESOURCESET_TOPICSTRING_REGEX", "^[^/]+/([^/]+)", false);
        setenv("IMA_TEST_PROTOCOL_ALLOW_MQTT_PROXY", "true", false);

        resourceSetStatsEnabled = true;
        resourceSetStatsTrackUnmatched = ((rand()%100) >= 20); // 80% of the time track unmatched
        setenv("IMA_TEST_RESOURCESET_TRACK_UNMATCHED", resourceSetStatsTrackUnmatched ? "true" : "false", false);
    }

    // We don't want threads to reserve large blocks of memory in advance, so force the chunk
    // size to 0 for the entirety of this test...
    iemem_setMemChunkSize(0);

    rc = test_processInit(trclvl, NULL);
    if (rc != OK) goto mod_exit;

    rc = test_engineInit_DEFAULT;
    if (rc != OK) goto mod_exit;

    ieutThreadData_t *pThreadData = ieut_getThreadData();
    iereResourceSetHandle_t resourceSet = iecs_getResourceSet(pThreadData, "X:"TPT_RESOURCE_SET_ID":BLAH", PROTOCOL_ID_MQTT, iereOP_ADD);

    // Set default maxMessageCount to 0 for the duration of the test
    iepi_getDefaultPolicyInfo(false)->maxMessageCount = 0;

    // failures += testMemoryDisable(0);
    // We cannot test QoS0 at the moment, because we cannot predict
    // how many messages the subscribers will each receive - it will
    // differ depending on when the memory is disabled
    failures += testMemoryDisable(1, TxnCapable_False);
    iere_traceResourceSet(pThreadData, 0, resourceSet);
    failures += testMemoryDisable(2, TxnCapable_False);
    iere_traceResourceSet(pThreadData, 0, resourceSet);

    //Now test multiconsumer queue....
    failures += testMemoryDisable(1, TxnCapable_True);
    iere_traceResourceSet(pThreadData, 0, resourceSet);

    ismEngine_ResourceSetMonitor_t *results = NULL;
    uint32_t resultCount = 0;

    rc = ism_engine_getResourceSetMonitor(&results, &resultCount, ismENGINE_MONITOR_ALL_UNSORTED, 10, NULL);
    if (rc != OK)
    {
        printf("ERROR: ism_engine_getResourceSerMonitor returned %d\n", rc);
        goto mod_exit;
    }

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
            resourceSet = (iereResourceSet_t *)(results[r].resourceSet);
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

    rc = test_processTerm(true);
    if (rc != OK) goto mod_exit;

mod_exit:

    if (rc != OK && failures == 0) failures = 1;

    if (failures == 0) {
        printf("SUCCESS: testPubSubMemLowCfgs: tests passed\n");
    } else {
        printf("FAILURE: testPubSubMemLowCfgs: %d tests failed\n", failures);
        if (rc == OK) rc = 10;
    }
    return rc;
}
