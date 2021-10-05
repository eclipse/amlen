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
/// @file  test_utils_client.h
/// @brief Utility functions to create / delete clients for tests.
//****************************************************************************
#ifndef __ISM_TEST_UTILS_CLIENT_H_DEFINED
#define __ISM_TEST_UTILS_CLIENT_H_DEFINED

#include <stdbool.h>
#include <stdint.h>
#include "engine.h"
#include "security.h"

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
                                                bool bStartMsgDelivery);

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
                                    bool bStartMsgDelivery);

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
                                     bool bStopMsgDelivery);

#endif //end ifndef __ISM_TEST_UTILS_CLIENT_H_DEFINED
