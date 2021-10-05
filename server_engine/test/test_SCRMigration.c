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
/* Module Name: test_SCRMigration.c                                  */
/*                                                                   */
/* Description: CUnit tests of Engine SCR Migration                  */
/*                                                                   */
/*********************************************************************/
#include "test_SCRMigration.h"
#include "test_utils_assert.h"
#include "test_utils_initterm.h"
#include "engineStore.h"
#include "engineTimers.h"

CU_TestInfo ISM_Engine_CUnit_SCRMigration[] = {
    { "SCRMigration", SCRMigration },
    CU_TEST_INFO_NULL
};

// Write a Server Configuration Record to the store
ismStore_Handle_t test_storeV1SCR(uint32_t useTimestamp)
{
    ieutThreadData_t *pThreadData = ieut_getThreadData();
    int32_t rc = OK;
    ismStore_Record_t storeRecord;
    iestServerConfigRecord_V1_t SCR_V1;
    ismStore_Handle_t hStoreSCR;

    char *fragments[1]; // 1 fragment for an SCR
    uint32_t fragmentLengths[1];

    // First fragment is always the SCR
    fragments[0] = (char *)&SCR_V1;
    fragmentLengths[0] = sizeof(SCR_V1);

    // Fill in the store record
    storeRecord.Type = ISM_STORE_RECTYPE_SERVER;
    storeRecord.Attribute = 0;
    storeRecord.pFrags = fragments;
    storeRecord.pFragsLengths = fragmentLengths;
    storeRecord.FragsCount = 1;
    storeRecord.DataLength = fragmentLengths[0];
    storeRecord.State = (uint64_t)useTimestamp << 32;

    SCR_V1.Version = iestSCR_VERSION_1;
    memcpy(SCR_V1.StrucId, iestSERVER_CONFIG_RECORD_STRUCID, 4);

    // Add to the store
    rc = ism_store_createRecord(pThreadData->hStream,
                                &storeRecord,
                                &hStoreSCR);
    TEST_ASSERT_EQUAL(rc, OK);

    rc = ism_store_commit(pThreadData->hStream);
    TEST_ASSERT_EQUAL(rc, OK);

    return hStoreSCR;
}

/*
 * Destroy an up-to-date SCR and add old ones to migrate from.
 */
void SCRMigration(void)
{
    int32_t rc = OK;

    int32_t oldSCRCounts[] = {10, 0, 120, 1};
    uint32_t prevLoopTime = 0;
    ismStore_Handle_t prevLoopStoreSCR = ismSTORE_NULL_HANDLE;

    // The following tests rely on 1st loop not adding 0 v1 SCRs
    TEST_ASSERT_NOT_EQUAL(oldSCRCounts[0], 0);

    for(int32_t loops=0; loops<(sizeof(oldSCRCounts)/sizeof(oldSCRCounts[0])); loops++)
    {
        ieutThreadData_t *pThreadData = ieut_getThreadData();

        int32_t oldSCRCount = oldSCRCounts[loops];

        TEST_ASSERT_NOT_EQUAL(ismEngine_serverGlobal.hStoreSCR, ismSTORE_NULL_HANDLE);
        TEST_ASSERT_NOT_EQUAL(ismEngine_serverGlobal.ServerTimestamp, 0);

        if (oldSCRCount != 0)
        {
            // Get rid of the existing (current) SCR.
            rc = ism_store_deleteRecord(pThreadData->hStream, ismEngine_serverGlobal.hStoreSCR);
            TEST_ASSERT_EQUAL(rc, OK);

            rc = ism_store_commit(pThreadData->hStream);
            TEST_ASSERT_EQUAL(rc, OK);

            ismEngine_serverGlobal.hStoreSCR = ismSTORE_NULL_HANDLE;
            ismEngine_serverGlobal.ServerTimestamp = 0;

            // Write some V1 SCRs
            ismStore_Handle_t oldSCR[oldSCRCount];
            uint32_t startTime = ism_common_nowExpire();

            for(int32_t i=0; i<oldSCRCount; i++)
            {
                oldSCR[i] = test_storeV1SCR(startTime+i);
            }

            // Bounce the engine.
            test_bounceEngine();

            for(int32_t i=0; i<oldSCRCount; i++)
            {
                TEST_ASSERT_NOT_EQUAL(ismEngine_serverGlobal.hStoreSCR, oldSCR[i]);
            }

            TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ServerTimestamp, startTime+(oldSCRCount-1));
            prevLoopTime = ismEngine_serverGlobal.ServerTimestamp;
            prevLoopStoreSCR = ismEngine_serverGlobal.hStoreSCR;
        }
        else
        {
            // Bounce the engine.
            test_bounceEngine();

            TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ServerTimestamp, prevLoopTime);
            TEST_ASSERT_EQUAL(ismEngine_serverGlobal.hStoreSCR, prevLoopStoreSCR);
        }
    }

    // Just drive the timer function once explicitly to prove that it updates the time stamp
    TEST_ASSERT_EQUAL(ismEngine_serverGlobal.ActiveTimerUseCount, 0);
    ismEngine_serverGlobal.ActiveTimerUseCount = 1;
    ismEngine_serverGlobal.ServerTimestamp = 0;
    int runagain = ietm_updateServerTimestamp(NULL, 0, NULL);
    ismEngine_serverGlobal.ActiveTimerUseCount = 0;
    TEST_ASSERT_EQUAL(runagain, 1);
    uint32_t allowSeconds=30;
    while(ismEngine_serverGlobal.ServerTimestamp == 0)
    {
        if (allowSeconds-- > 0)
        {
            sleep(1);
        }
        else
        {
            TEST_ASSERT(false, ("Server timestamp not updated\n"));
        }
    }

    // Try setting up the timers
    ismEngine_serverGlobal.ServerTimestampInterval = 120;
    rc = ietm_setUpTimers();
    TEST_ASSERT_EQUAL(rc, OK);
    ietm_cleanUpTimers();
}
