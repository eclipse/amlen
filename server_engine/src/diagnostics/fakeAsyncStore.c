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

//****************************************************************************
/// @brief Utility functions to create a fake async store
//****************************************************************************
#include <assert.h>
#include <stdbool.h>

#include "store.h"

//****************************************************************************
/// @brief  Fake ism_store_asyncCommit function.
//****************************************************************************
int32_t ism_store_asyncCommit(ismStore_StreamHandle_t hStream,
                              ismStore_CompletionCallback_t pCallback,
                              void *pContext)
{
    static int32_t (*real_ism_store_commit)(ismStore_StreamHandle_t) = NULL;

    if (real_ism_store_commit == NULL)
    {
        real_ism_store_commit = dlsym(RTLD_NEXT, "ism_store_commit");
        assert(real_ism_store_commit != NULL);
    }

    int32_t rc = ism_store_commit(hStream);

    pCallback(rc, pContext);

    return ISMRC_AsyncCompletion;
}
