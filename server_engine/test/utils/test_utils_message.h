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
/// @file  test_utils_message.h
/// @brief Utility functions to create / delete messages for tests.
//****************************************************************************
#ifndef __ISM_TEST_UTILS_MESSAGE_H_DEFINED
#define __ISM_TEST_UTILS_MESSAGE_H_DEFINED

#include <stdbool.h>
#include <stdint.h>
#include "engine.h"

#define TEST_SMALL_MESSAGE_SIZE 10

//****************************************************************************
/// @brief  Create a that appears to have come from a specified originating server
///
/// @param[in]     payloadSize        Size that the message payload should be
///                                   or size that it is if supplied.
/// @param[in]     persistence        Persistence to use
/// @param[in]     reliability        Reliability to use
/// @param[in]     flags              Flags to use
/// @param[in]     expiry             Time as stored in message header
/// @param[in]     destinationType    Type of destination for this message
/// @param[in]     destinationName    Name of destination for the message
/// @param[in]     serverUID          UID of the server to use as originator
/// @param[in]     timestamp          The server timestamp to use
/// @param[in]     messageType        The type to give to this message
/// @param[out]    phMessage          Pointer to receive the hMessage
/// @param[in,out] pPayload           Pointer to receive pointer to payload
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_createOriginMessage(size_t payloadSize,
                                 uint8_t persistence,
                                 uint8_t reliability,
                                 uint8_t flags,
                                 uint32_t expiry,
                                 ismDestinationType_t destinationType,
                                 const char *destinationName,
                                 const char *serverUID,
                                 ism_time_t timestamp,
                                 uint8_t messageType,
                                 ismEngine_MessageHandle_t *phMessage,
                                 void **ppPayload);

//****************************************************************************
/// @brief  Create a message.
///
/// @param[in]     payloadSize        Size that the message payload should be
///                                   or size that it is if supplied.
/// @param[in]     persistence        Persistence to use
/// @param[in]     reliability        Reliability to use
/// @param[in]     flags              Flags to use
/// @param[in]     expiry             Time as stored in message header
/// @param[in]     destinationType    Type of destination for this message
/// @param[in]     destinationName    Name of destination for the message
/// @param[out]    phMessage          Pointer to receive the hMessage
/// @param[in,out] pPayload           Pointer to receive pointer to payload
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_createMessage(size_t payloadSize,
                           uint8_t persistence,
                           uint8_t reliability,
                           uint8_t flags,
                           uint32_t expiry,
                           ismDestinationType_t destinationType,
                           const char *destinationName,
                           ismEngine_MessageHandle_t *phMessage,
                           void **ppPayload);

//****************************************************************************
/// @brief  Create a random message payload
///
/// @param[in]     payload            Pointer to receive allocated message
/// @param[in]     payloadSize        Pointer to receive the payload size
/// @param[in]     initialString      A string to use as the start of the msg
///
/// @remark The payload can in fact be 0 bytes, in which case the payload is
///         returned as NULL.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void test_createRandomPayload(void **payload,
                              size_t *payloadSize,
                              char *initialString);

//****************************************************************************
/// @brief  Extract a property from a set of message areas
///
/// @param[in]     propertyId         The ID of the property (e.g. ID_Topic)
/// @param[in]     message            The message to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
char *test_extractMsgPropertyID(int propertyId, ismEngine_Message_t *message);

#endif //end ifndef __ISM_TEST_UTILS_MESSAGE_H_DEFINED
