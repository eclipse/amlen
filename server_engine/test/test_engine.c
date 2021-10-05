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
/* Module Name: test_engine.c                                        */
/*                                                                   */
/* Description: Main source file for CUnit test of Engine interface  */
/*                                                                   */
/*********************************************************************/
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "test_clientState.h"
#include "test_storeMQRecords.h"
#include "test_qos0.h"
#include "test_qos1.h"
#include "test_qos2.h"
#include "test_willMessage.h"
#include "test_batchAcks.h"
#include "test_SCRMigration.h"
#include "test_lockManager.h"
#include "test_engineTimers.h"

#include "test_utils_initterm.h"
#include "test_utils_security.h"
#include "test_utils_assert.h"

void test_ismEngine_Message_t(void)
{
    ismEngine_Message_t msg;
    test_log(testLOGLEVEL_TESTNAME, "Starting %s...\n", __func__);

    size_t pos = 0;
    size_t size = sizeof(msg);

    // We expect this structure to be this size (from past versions) -- if it isn't, that's okay but if
    // a store is recovered, containing messages, those messages will be using a different total amount
    // of memory... consider whether that's okay...
    TEST_ASSERT_EQUAL(size, 128);

    FILE *dbgFile;

    #if 0
    dbgFile = stdout;
    #else
    dbgFile = fopen("/dev/null", "w");
    #endif

    fprintf(dbgFile, "ismEngine_Message_t(%lu) 0-%lu\n", size, size);
    pos = offsetof(ismEngine_Message_t, StrucId);
    size = sizeof(msg.StrucId);
    fprintf(dbgFile, "StrucId(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, usageCount);
    size = sizeof(msg.usageCount);
    fprintf(dbgFile, "usageCount(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, Header);
    size = sizeof(msg.Header);
    fprintf(dbgFile, "Header(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, AreaCount);
    size = sizeof(msg.AreaCount);
    fprintf(dbgFile, "AreaCount(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, Flags);
    size = sizeof(msg.Flags);
    fprintf(dbgFile, "Flags(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, futureField1);
    size = sizeof(msg.futureField1);
    fprintf(dbgFile, "futureField1(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, futureField2);
    size = sizeof(msg.futureField2);
    fprintf(dbgFile, "futureField2(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, futureField3);
    size = sizeof(msg.futureField3);
    fprintf(dbgFile, "futureField3(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, AreaTypes);
    size = sizeof(msg.AreaTypes);
    fprintf(dbgFile, "AreaTypes(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, AreaLengths);
    size = sizeof(msg.AreaLengths);
    fprintf(dbgFile, "AreaLengths(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, pAreaData);
    size = sizeof(msg.pAreaData);
    fprintf(dbgFile, "pAreaData(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, MsgLength);
    size = sizeof(msg.MsgLength);
    fprintf(dbgFile, "MsgLength(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, recovNext);
    size = sizeof(msg.recovNext);
    fprintf(dbgFile, "recovNext(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, resourceSet);
    size = sizeof(msg.resourceSet);
    fprintf(dbgFile, "resourceSet(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, fullMemSize);
    size = sizeof(msg.fullMemSize);
    fprintf(dbgFile, "fullMemSize(%lu) %lu-%lu\n", size, pos, pos+size);
    pos = offsetof(ismEngine_Message_t, StoreMsg);
    size = sizeof(msg.StoreMsg);
    fprintf(dbgFile, "StoreMsg(%lu) %lu-%lu\n", size, pos, pos+size);

    if (dbgFile != stdout) fclose(dbgFile);
}

CU_TestInfo ISM_Engine_CUnit_StructureStatusQuo[] = {
    { "ismEngine_Message_t",                     test_ismEngine_Message_t },
    CU_TEST_INFO_NULL
};

int initEngine(void)
{
    return test_engineInit_DEFAULT;
}

int initEngineLowIDRange(void)
{
    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();

    // Increase the number of messages in flight per sub so we can easily run out of delivery ids
    // for a client without creating thousands of subscriptions.
    ism_field_t f;
    f.type = VT_Integer;
    f.val.i = 10;
    int rc = ism_common_setProperty(staticConfigProps,
                                    ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES,
                                    &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d returned %d\n", ismENGINE_CFGPROP_MAX_MQTT_UNACKED_MESSAGES, f.val.i, rc);
        return 10;
    }

    return test_engineInit_DEFAULT;
}

int initEngineSecurity(void)
{
    return test_engineInit(true, false,
                           ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,
                           false, /*recovery should complete ok*/
                           ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY,
                           testDEFAULT_STORE_SIZE);
}
int termEngine(void)
{
    return test_engineTerm(true);
}

/*
 * Array that carries the test suite and other functions to the CUnit framework
 */
CU_SuiteInfo ISM_Engine_CUnit_basicsuites[] = {
    IMA_TEST_SUITE("EngineStructureStatusQuo",     initEngine,           termEngine, ISM_Engine_CUnit_StructureStatusQuo    ),
    IMA_TEST_SUITE("EngineClientStateBasic",       initEngine,           termEngine, ISM_Engine_CUnit_ClientStateBasic      ),
    IMA_TEST_SUITE("EngineClientStateLowIDRange",  initEngineLowIDRange, termEngine, ISM_Engine_CUnit_ClientStateLowIDRange ),
    IMA_TEST_SUITE("EngineClientStateHardToReach", initEngine,           termEngine, ISM_Engine_CUnit_ClientStateHardToReach),
    IMA_TEST_SUITE("EngineClientStateSecurity",    initEngineSecurity,   termEngine, ISM_Engine_CUnit_ClientStateSecurity   ),
    IMA_TEST_SUITE("EngineQoS0Basic",              initEngine,           termEngine, ISM_Engine_CUnit_QoS0Basic             ),
    IMA_TEST_SUITE("EngineQoS0ExplicitSuspend",    initEngine,           termEngine, ISM_Engine_CUnit_QoS0ExplicitSuspend   ),
    IMA_TEST_SUITE("EngineQoS1Basic",              initEngine,           termEngine, ISM_Engine_CUnit_QoS1Basic             ),
    IMA_TEST_SUITE("EngineQoS2Basic",              initEngine,           termEngine, ISM_Engine_CUnit_QoS2Basic             ),
    IMA_TEST_SUITE("EngineQoS2LowIDRange",         initEngineLowIDRange, termEngine, ISM_Engine_CUnit_QoS2LowIDRange        ),
    IMA_TEST_SUITE("EngineWillMessageBasic",       initEngine,           termEngine, ISM_Engine_CUnit_WillMessageBasic      ),
    IMA_TEST_SUITE("EngineWillMessageSecurity",    initEngineSecurity,   termEngine, ISM_Engine_CUnit_WillMessageSecurity   ),
    IMA_TEST_SUITE("EngineWillMessageDelay",       initEngine,           termEngine, ISM_Engine_CUnit_WillDelay             ),
    IMA_TEST_SUITE("EngineStoreMQRecords",         initEngine,           termEngine, ISM_Engine_CUnit_StoreMQRecords        ),
    IMA_TEST_SUITE("EngineBatchAcks",              initEngine,           termEngine, ISM_Engine_CUnit_BatchAcks             ),
    IMA_TEST_SUITE("EngineSCRMigration",           initEngine,           termEngine, ISM_Engine_CUnit_SCRMigration          ),
    IMA_TEST_SUITE("EngineLockManager",            initEngine,           termEngine, ISM_Engine_CUnit_LockManager           ),
    IMA_TEST_SUITE("EngineTimers",                 initEngine,           termEngine, ISM_Engine_CUnit_Timers                ),
    CU_SUITE_INFO_NULL,
};

int main(int argc, char *argv[])
{
    int trclvl = 0;
    int retval = 0;

    retval = test_processInit(trclvl, NULL);
    if (retval != OK) goto mod_exit;

    CU_initialize_registry();
    CU_register_suites(ISM_Engine_CUnit_basicsuites);

    CU_basic_run_tests();

    CU_RunSummary * CU_pRunSummary_Final;
    CU_pRunSummary_Final = CU_get_run_summary();
    printf("\n\n[cunit] Tests run: %d, Failures: %d, Errors: %d\n\n",
            CU_pRunSummary_Final->nTestsRun,
            CU_pRunSummary_Final->nTestsFailed,
            CU_pRunSummary_Final->nAssertsFailed);
    if ((CU_pRunSummary_Final->nTestsFailed > 0) ||
        (CU_pRunSummary_Final->nAssertsFailed > 0))
    {
        retval = 1;
    }

    CU_cleanup_registry();

    int32_t rc = test_processTerm(retval == 0);
    if (retval == 0) retval = rc;

mod_exit:

    return retval;
}
