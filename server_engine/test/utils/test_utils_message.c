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
/// @file  test_utils_initterm.c
/// @brief Common initialization and termination routines for use by tests
//****************************************************************************
#include "engine.h"
#include "engineInternal.h"
#include "topicTree.h"
#include "topicTreeInternal.h"
#include "msgCommon.h"

#include "test_utils_message.h"

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
                                 void **ppPayload)
{
    int32_t rc = OK;
    void *local_payload;
    bool setPayloadOnOutput = true;
    char xbuf[1024];
    concat_alloc_t props = {xbuf, sizeof(xbuf)};

    if (payloadSize != 0)
    {
        // Payload has been provided, use it.
        if (ppPayload != NULL && *ppPayload != NULL)
        {
            local_payload = *ppPayload;
            setPayloadOnOutput = false;
        }
        else
        {
            local_payload = malloc(payloadSize);

            if (local_payload == NULL)
            {
                printf("ERROR: malloc() failed (size=%ld)", payloadSize);
                rc = ISMRC_AllocateError;
                goto mod_exit;
            }

            // Fill with random text
            for(size_t payloadByte=0; payloadByte<payloadSize; payloadByte++)
            {
                ((char*)local_payload)[payloadByte] = 'A' + rand()%25;
            }
        }
    }
    else
    {
        local_payload = NULL;
    }

    ismEngine_MessageHandle_t hMessage = NULL;
    ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
    ismMessageAreaType_t areaTypes[3];
    size_t areaLengths[3];
    void *areaData[3];
    uint8_t areaCount;

    // Just payload
    if ((destinationType == ismDESTINATION_NONE) || (destinationName == NULL))
    {
        areaTypes[0] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[0] = payloadSize;
        areaData[0] = local_payload;
        areaCount = 1;
    }
    // Properties and payload
    else
    {
        ism_field_t field;
        field.type = VT_String;
        field.val.s = (char *)destinationName;
        if (destinationType == ismDESTINATION_QUEUE)
            ism_common_putPropertyID(&props, ID_Queue, &field);
        else
            ism_common_putPropertyID(&props, ID_Topic, &field);

        if ((flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) != 0)
        {
            // Add the serverUID to the properties
            field.type = VT_String;
            field.val.s = (char *)serverUID;
            ism_common_putPropertyID(&props, ID_OriginServer, &field);

            // Add the serverTime to the properties
            field.type = VT_Long;
            field.val.l = timestamp;
            ism_common_putPropertyID(&props, ID_ServerTime, &field);

            // Make any retained that is meant to come from another server have
            // a real expiry property
            if (strcmp(serverUID, ism_common_getServerUID()) != 0)
            {
                field.type = VT_UInt;
                field.val.u = 0;
                ism_common_putPropertyID(&props, ID_RealExpiry, &field);
            }
        }

        areaTypes[0] = ismMESSAGE_AREA_PROPERTIES;
        areaLengths[0] = props.used;
        areaData[0] = props.buf;
        areaTypes[1] = ismMESSAGE_AREA_PAYLOAD;
        areaLengths[1] = payloadSize;
        areaData[1] = local_payload;

        areaCount = 2;
    }

    header.Persistence = persistence;
    header.Reliability = reliability;
    header.MessageType = messageType;
    header.Flags = flags;
    header.Expiry = expiry;

    rc = ism_engine_createMessage(&header,
                                  areaCount,
                                  areaTypes,
                                  areaLengths,
                                  areaData,
                                  &hMessage);

    if (rc != OK) goto mod_exit;

    if (rc == OK)
    {
        *phMessage = hMessage;

        if (ppPayload != NULL && setPayloadOnOutput)
        {
            *ppPayload = local_payload;
        }
    }

    //If the caller doesn't want a separate copy of the payload, free it
    if ((ppPayload == NULL) && (local_payload != NULL))
    {
        free(local_payload);
    }

    // Free any properties we allocated
    if (props.inheap)
    {
        ism_common_freeAllocBuffer(&props);
    }

mod_exit:

    return rc;
}

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
                           void **ppPayload)
{
    return test_createOriginMessage(payloadSize,
                                    persistence,
                                    reliability,
                                    flags,
                                    expiry,
                                    destinationType,
                                    destinationName,
                                    (char *)ism_common_getServerUID(),
                                    ism_engine_retainedServerTime(),
                                    MTYPE_JMS, // Normal default
                                    phMessage,
                                    ppPayload);
}

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
                              char *initialString)
{
    if ((rand()%10) > 7) /* 20% of the time, use empty payload */
    {
        *payloadSize = 0;
        *payload = NULL;
    }
    else
    {
        *payloadSize =  (size_t)(rand()%10000)+strlen(initialString);
        *payload = malloc(*payloadSize);
        memset(*payload, 'X', *payloadSize);
        memcpy(*payload, initialString, strlen(initialString));
    }
}

//****************************************************************************
/// @brief  Extract a property from a set of message areas
///
/// @param[in]     propertyId         The ID of the property (e.g. ID_Topic)
/// @param[in]     message            The message to use
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
char *test_extractMsgPropertyID(int propertyId, ismEngine_Message_t *message)
{
    concat_alloc_t  props = {0};
    // Get values out of the message properties
    iem_locateMessageProperties(message, &props);

    ism_field_t field = {0};

    // The properties contain an originating serverUID
    ism_common_findPropertyID(&props, propertyId, &field);

    return field.val.s;
}
