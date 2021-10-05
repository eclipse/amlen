/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
#include <stdint.h>
#include <stdbool.h>

#include "engineInternal.h"
#include "test_utils_assert.h"


#ifdef USEFAKE_ASYNC_COMMIT
#include "fakeAsyncStore.h"

void test_utils_pauseAsyncCompletions(void)
{
    pauseFakeCallbacks();
}
void test_utils_restartAsyncCompletions(void)
{
    unpauseFakeCallbacks();
}
#else
#define MAX_BLOCKED_CALLBACKS 4096
ismStore_CompletionCallback_t BlockedCallbacks[MAX_BLOCKED_CALLBACKS];
void *CallBackContexts[MAX_BLOCKED_CALLBACKS];
uint64_t numBlockedCBs = 0;
bool blockStoreCommitCallbacks = false;

XAPI int32_t ism_store_asyncCommit(ismStore_StreamHandle_t hStream,
                                   ismStore_CompletionCallback_t pCallback,
                                   void *pContext)
{
    int32_t rc;
    static int32_t (*real_ism_store_asyncCommit)(ismStore_StreamHandle_t hStream,
                                                 ismStore_CompletionCallback_t pCallback,
                                                 void *pContext) = NULL;

    if (real_ism_store_asyncCommit == NULL)
    {
        real_ism_store_asyncCommit  = dlsym(RTLD_NEXT, "ism_store_asyncCommit");
        TEST_ASSERT_PTR_NOT_NULL(real_ism_store_asyncCommit);
    }

    if (!blockStoreCommitCallbacks)
    {
        rc = real_ism_store_asyncCommit(hStream, pCallback, pContext);
    }
    else
    {
        if (numBlockedCBs < MAX_BLOCKED_CALLBACKS)
        {
            rc = ism_store_commit(hStream);
            TEST_ASSERT_EQUAL(rc, OK)
            BlockedCallbacks[numBlockedCBs] = pCallback;
            CallBackContexts[numBlockedCBs] = pContext;
            numBlockedCBs++;

            rc = ISMRC_AsyncCompletion;
        }
        else
        {
            printf("Too many callbacks quieued whilst paused\n");
            TEST_ASSERT_EQUAL(1, 0);
        }
    }
    return rc;
}
void test_utils_pauseAsyncCompletions(void)
{
    blockStoreCommitCallbacks = true;
}
void test_utils_restartAsyncCompletions(void)
{
    for (uint32_t i =0 ; i < numBlockedCBs; i++)
    {
        BlockedCallbacks[i](ISMRC_OK, CallBackContexts[i]);
    }
    blockStoreCommitCallbacks = false;
}
#endif

