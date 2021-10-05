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
/// @file  test_utils_client.c
/// @brief Utility functions to create / delete clients for tests.
//****************************************************************************
#include "engine.h"
#include "engineInternal.h"
#include "security.h"

#include "test_utils_security.h"
#include "test_utils_client.h"
#include "test_utils_options.h"
#include "test_utils_sync.h"

//****************************************************************************
/// @brief  Dummy client-state steal callback function
///
/// @param[in]     reason           Reason that the client was stolen
/// @param[in]     hClient          Client-state handle which has been stolen
/// @param[in]     options          ismENGINE_STEAL_CALLBACK_OPTION_*
/// @param[in]     pContext         Optional context supplied when client-state created
///
/// @remark The callback can call other Engine operations, bearing in mind
/// that the stack must be allowed to unwind.
//****************************************************************************
void test_dummyStealCallback(int32_t                         reason,
                             ismEngine_ClientStateHandle_t   hClient,
                             uint32_t                        options,
                             void *                          pContext)
{
    return;
}

//****************************************************************************
/// @brief  Create a client state and associated session, optionally starting
///         message delivery on the session with a specified protocol Id.
///
/// @param[in]     pClientId          Client Id to use (can be NULL for anonymous)
/// @param[in]     protocolId         protocol Id to use
/// @param[in]     pSecContext        Security Context to use
/// @param[in]     clientOptions      Options to pass to create client
/// @param[in]     sessionOptions     Options to pass to create session
/// @param[out]    phClient           Returned client handle
/// @param[out]    phSession          Returned session handle
/// @param[in]     bStartMsgDelivery  Whether to start message delivery for the session
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_createClientAndSessionWithProtocol(const char *pClientId,
                                                uint32_t protocolId,
                                                ismSecurity_t *pSecContext,
                                                uint32_t clientOptions,
                                                uint32_t sessionOptions,
                                                ismEngine_ClientStateHandle_t *phClient,
                                                ismEngine_SessionHandle_t *phSession,
                                                bool bStartMsgDelivery)
{
    int32_t rc;
    ismEngine_StealCallback_t stealCallback = NULL;

    // Provide a dummy steal callback
    if (clientOptions & ismENGINE_CREATE_CLIENT_OPTION_CLIENTID_STEAL)
    {
        stealCallback = test_dummyStealCallback;
    }

    rc = sync_ism_engine_createClientState(pClientId,
                                      protocolId,
                                      clientOptions,
                                      NULL, stealCallback,
                                      pSecContext,
                                      phClient);

    if (rc != OK && rc != ISMRC_ResumedClientState)
    {
        printf("ERROR: ism_engine_createClientState() returned %d\n", rc);
        goto mod_exit;
    }

    rc = ism_engine_createSession((*phClient),
                                  sessionOptions,
                                  phSession,
                                  NULL, 0, NULL);
    if (rc != OK)
    {
        printf("ERROR: ism_engine_createSession() returned %d\n", rc);
        goto mod_exit;
    }

    if (bStartMsgDelivery)
    {
        rc = ism_engine_startMessageDelivery((*phSession),
                                             ismENGINE_START_DELIVERY_OPTION_NONE,
                                             NULL, 0, NULL);

        if (rc != OK)
        {
            printf("ERROR: ism_engine_startMessageDelivery() returned %d\n", rc);
            goto mod_exit;
        }
    }

    // If we have bounced the server, the context is going to need rebuilding
    // this will only get used once a new client/session have been created, so
    // update the security context to require a revalidation now, to ensure
    // new values are picked up.
    if (pSecContext != NULL)
    {
        mock_ismSecurity_t *mockContext = (mock_ismSecurity_t *)pSecContext;
        mockContext->reValidatePolicy = 1;
    }

mod_exit:

   return rc;
}

//****************************************************************************
/// @brief  Create a client state and associated session with a default unit
///         testing protocol.
///
/// @see test_createClientAndSessionWithProtocol
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_createClientAndSession(const char *pClientId,
                                    ismSecurity_t *pSecContext,
                                    uint32_t clientOptions,
                                    uint32_t sessionOptions,
                                    ismEngine_ClientStateHandle_t *phClient,
                                    ismEngine_SessionHandle_t *phSession,
                                    bool bStartMsgDelivery)
{
    return test_createClientAndSessionWithProtocol(pClientId,
                                                   testDEFAULT_PROTOCOL_ID,
                                                   pSecContext,
                                                   clientOptions,
                                                   sessionOptions,
                                                   phClient,
                                                   phSession,
                                                   bStartMsgDelivery);
}

//****************************************************************************
/// @brief  Destroy a client state and session, optionally explicitly stopping
///         message delivery on the session first.
///
/// @param[in]     hClient            Client Handle
/// @param[in]     hSession           Session Handle
/// @param[in]     bStopMsgDelivery   Whether to stop message delivery
///                                   explicitly
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_destroyClientAndSession(ismEngine_ClientStateHandle_t hClient,
                                     ismEngine_SessionHandle_t hSession,
                                     bool bStopMsgDelivery)
{
    int32_t rc;

    if (bStopMsgDelivery)
    {
        rc = ism_engine_stopMessageDelivery(hSession,
                                            NULL, 0, NULL);

        if (rc != OK)
        {
            printf("ERROR: ism_engine_stopMessageDelivery() returned %d\n", rc);
            goto mod_exit;
        }
    }

    rc = sync_ism_engine_destroySession(hSession);
    if (rc != OK)
    {
        printf("ERROR: ism_engine_destroySession() returned %d\n", rc);
        goto mod_exit;
    }

    rc = sync_ism_engine_destroyClientState(hClient,
                                            ismENGINE_DESTROY_CLIENT_OPTION_NONE);
    if (rc != OK)
    {
        printf("ERROR: ism_engine_destroyClientState() returned %d\n", rc);
        goto mod_exit;
    }

mod_exit:

    return rc;
}
