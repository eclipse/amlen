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

//*********************************************************************
/// @file  msgCommon.c
/// @brief Common message functions
//*********************************************************************
#define TRACE_COMP Engine

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "engineInternal.h"
#include "memHandler.h"
#include "msgCommon.h"
#include "engineDump.h"
#include "resourceSetStats.h"

//****************************************************************************
// @brief  Increment the usage count for the message by 1
//
// @param[in]     hMessage         Message handle
//
// @see ism_engine_releaseMessage
//****************************************************************************
void iem_recordMessageUsage(ismEngine_Message_t *msg)
{
    __sync_add_and_fetch(&(msg->usageCount), 1);
}

//****************************************************************************
// @brief  Increment the usage count for the message by a given amount
//
// @param[in]     hMessage         Message handle
// @param[in]     newUsers         Count of new users
//
// @see ism_engine_releaseMessage
//****************************************************************************
void iem_recordMessageMultipleUsage(ismEngine_Message_t *msg, uint32_t newUsers)
{
    assert(newUsers != 0);
    __sync_add_and_fetch(&(msg->usageCount), newUsers);
}

//****************************************************************************
// @brief  Reduce the use count for a message and release it if no longer used
//
// @param[in]     hMessage         Message handle
//
// @remark Notice that this function, unlike other engine does not perform
//         trace on entry / exit. This is because it will be called often
//         (for every reference to a message on any destination) and we
//         only need to know that we were called.
//
// @see ism_engine_releaseMessage
//****************************************************************************
void iem_releaseMessage(ieutThreadData_t *pThreadData, ismEngine_Message_t *pMessage)
{
    assert(pMessage->usageCount > 0);

    uint32_t newUsage = __sync_sub_and_fetch(&pMessage->usageCount, 1);

    ieutTRACEL(pThreadData, pMessage, ENGINE_HIGH_TRACE, "%s pMessage %p, newUsage %d.\n",
               __func__, pMessage, newUsage);

    // If the message is no longer in use, we can free the memory.
    if (newUsage == 0)
    {
        iereResourceSetHandle_t resourceSet = pMessage->resourceSet;

        if (pMessage->Flags & ismENGINE_MSGFLAGS_ALLOCTYPE_1)
        {
            // Msg data is separate from the header, so it needs to be freed
            // separately. The first pAreaData with non-NULL value is the
            // allocated area.
            for(uint8_t i=0; i<pMessage->AreaCount; i++)
            {
                if (pMessage->pAreaData[i] != NULL)
                {
                    iemem_free(pThreadData, iemem_messageBody, pMessage->pAreaData[i]);
                    break;
                }
                else
                {
                    assert(pMessage->AreaLengths[i] == 0);
                }
            }
        }

        iere_primeThreadCache(pThreadData, resourceSet);
        int64_t fullMemSize = pMessage->fullMemSize;
        if (pMessage->Header.Persistence == ismMESSAGE_PERSISTENCE_PERSISTENT)
        {
            iere_updateInt64Stat(pThreadData,
                                 resourceSet,
                                 ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_PERSISTENT_BUFFEREDMSG_BYTES, -fullMemSize);
        }
        else
        {
            iere_updateInt64Stat(pThreadData,
                                 resourceSet,
                                 ISM_ENGINE_RESOURCESETSTATS_I64_TOTAL_NONPERSISTENT_BUFFEREDMSG_BYTES, -fullMemSize);
        }
        iere_freeStruct(pThreadData, resourceSet, iemem_messageBody, pMessage, pMessage->StrucId);
    }

    return;

}

//****************************************************************************
/// @brief  Create a copy of the in-memory data for a message adding / updating
///         properties for example, those required for retained messages.
///
/// @param[in]   pMessage           Original message to be copied from
/// @param[in]   simpleCopy         Whether the copy routine should consider
///                                 modifying properties or just make a copy.
/// @param[in]   retainedTimestamp  A timestamp value to use if this is a
///                                 retained message and not doing simple copy.
/// @param[in]   retainedRealExpiry Real expiry time for a retained message.
/// @param[in]   ppNewMessage       New message created from original
///
/// @remark Note: The usage count on the created message is 1.
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @see ism_engine_releaseMessage
//****************************************************************************
int32_t iem_createMessageCopy(ieutThreadData_t *pThreadData,
                              ismEngine_Message_t *pMessage,
                              bool simpleCopy,
                              ism_time_t retainedTimestamp,
                              uint32_t retainedRealExpiry,
                              ismEngine_Message_t **ppNewMessage)
{
    int32_t rc = OK;
    ismEngine_Message_t *pNewMessage;
    size_t NewMessageLength = 0;
    int32_t propertiesIndex = pMessage->AreaCount;

    ieutTRACEL(pThreadData, pMessage, ENGINE_FNC_TRACE, FUNCTION_ENTRY "pMessage=%p, simpleCopy=%d\n",
               __func__, pMessage, (int)simpleCopy);

    concat_alloc_t properties = {0};

    for(int32_t i=0; i < pMessage->AreaCount; i++)
    {
        if (pMessage->AreaTypes[i] == ismMESSAGE_AREA_PROPERTIES)
        {
            properties.buf = pMessage->pAreaData[i];
            properties.used = properties.len = pMessage->AreaLengths[i];
            propertiesIndex = i;
        }
        else
        {
            NewMessageLength += pMessage->AreaLengths[i];
        }
    }

    assert(propertiesIndex != pMessage->AreaCount);

    // Allow an extra 200 bytes for what we may need to add
    char localBuffer[properties.len + 200];

    // Unless the caller asked for a simple copy of the message, for retained messages,
    // we may need to add / update some fields
    if (simpleCopy == false && (pMessage->Header.Flags & ismMESSAGE_FLAGS_PUBLISHED_FOR_RETAIN) != 0)
    {
        properties.buf = localBuffer;
        properties.len = sizeof(localBuffer);

        memcpy(properties.buf, pMessage->pAreaData[propertiesIndex], properties.used);

        ism_field_t field = {0};

        // The properties should contain a topic string
        assert(ism_common_findPropertyID(&properties, ID_Topic, &field) == 0);

        // We cannot update existing properties this way. At the moment we don't have to do
        // so since the only time this should occur is when this is a will message which is
        // being retained, and this currently doesn't set ID_ServerTime.
        assert((ism_common_findPropertyID(&properties, ID_ServerTime, &field) != 0) ||
               (field.val.l == (int64_t)retainedTimestamp) ||
               (retainedTimestamp == 0));

        // Ensure that if the message has no serverUID that we set it to *this* server
        if (ism_common_findPropertyID(&properties, ID_OriginServer, &field) != 0)
        {
            field.type = VT_String;
            field.val.s = (char *)ism_common_getServerUID();
            ism_common_putPropertyID(&properties, ID_OriginServer, &field);
        }

        // Ensure that the message has a server time stamp and it matches the requested one
        if (ism_common_findPropertyID(&properties, ID_ServerTime, &field) != 0)
        {
            assert(retainedTimestamp != 0);
            field.type = VT_Long;
            field.val.l = (int64_t)retainedTimestamp;
            ism_common_putPropertyID(&properties, ID_ServerTime, &field);
        }

        // Ensure that the message has a real expiry time specified
        if (ism_common_findPropertyID(&properties, ID_RealExpiry, &field) != 0)
        {
            field.type = VT_UInt;
            field.val.u = retainedRealExpiry;
            ism_common_putPropertyID(&properties, ID_RealExpiry, &field);
        }
    }

    // Include whatever properties we now have in the message length
    NewMessageLength += properties.used;

    // Allocate space for the message
    pNewMessage = iere_malloc(pThreadData,
                              iereNO_RESOURCE_SET,
                              IEMEM_PROBE(iemem_messageBody, 5),
                              NewMessageLength + sizeof(ismEngine_Message_t));

    if (pNewMessage != NULL)
    {
        char *bufPtr = (char *)(pNewMessage+1);

        ismEngine_SetStructId(pNewMessage->StrucId, ismENGINE_MESSAGE_STRUCID);
        pNewMessage->usageCount = 1;
        memcpy(&(pNewMessage->Header), &pMessage->Header, sizeof(ismMessageHeader_t));
        pNewMessage->AreaCount = pMessage->AreaCount;
        pNewMessage->Flags = pMessage->Flags & ~ismENGINE_MSGFLAGS_ALLOCTYPE_1;
        pNewMessage->MsgLength = NewMessageLength;
        pNewMessage->resourceSet = iereNO_RESOURCE_SET;
        pNewMessage->fullMemSize = (int64_t)iere_full_size(iemem_messageBody, pNewMessage);
        for (int32_t i = 0; i < pMessage->AreaCount; i++)
        {
            char *areaPtr;

            if (i == propertiesIndex)
            {
                pNewMessage->AreaLengths[i] = properties.used;
                areaPtr = properties.buf;
            }
            else
            {
                pNewMessage->AreaLengths[i] = pMessage->AreaLengths[i];
                areaPtr = pMessage->pAreaData[i];
            }

            pNewMessage->AreaTypes[i] = pMessage->AreaTypes[i];
            if (pNewMessage->AreaLengths[i] == 0)
            {
                pNewMessage->pAreaData[i] = NULL;
            }
            else
            {
                pNewMessage->pAreaData[i] = bufPtr;
                memcpy(bufPtr, areaPtr, pNewMessage->AreaLengths[i]);
                bufPtr += pNewMessage->AreaLengths[i];
            }
        }
        pNewMessage->StoreMsg.parts.hStoreMsg = ismSTORE_NULL_HANDLE;
        pNewMessage->StoreMsg.parts.RefCount = 0;

        *ppNewMessage = pNewMessage;
    }
    else
    {
        rc = ISMRC_AllocateError;
        ism_common_setError(rc);
    }

    if (properties.inheap) ism_common_freeAllocBuffer(&properties);

    ieutTRACEL(pThreadData, pNewMessage,  ENGINE_HIGH_TRACE, FUNCTION_EXIT "rc=%d, pNewMessage=%p\n", __func__,
               rc, pNewMessage);
    return rc;
}

//****************************************************************************
// @brief  Release Message
//
// Reduce the usage count of a message as previously incremented by
// iem_recordMessageUsage and if it is no longer in use, release the storage
// associated with the message.
//
// @param[in]     hMessage         Message handle
//
// @remark This function is an external entry point to call the internal
//         iem_releaseMessage function.
//
// @see iem_recordMessageUsage
//****************************************************************************
XAPI void ism_engine_releaseMessage(ismEngine_MessageHandle_t hMessage)
{
    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);
    iem_releaseMessage(pThreadData, hMessage);
    ieut_leavingEngine(pThreadData);
}

//****************************************************************************
// @brief  Locate the message properties in the specified message
//
// @param[in]     msg    Message
// @param[in,out] props  Properties structure to fill in
//****************************************************************************
void iem_locateMessageProperties(ismEngine_Message_t *msg, concat_alloc_t *props)
{
    assert(msg != NULL && msg->AreaCount != 0 && props != NULL);

    props->buf = NULL;

    for (uint32_t i = 0; i < msg->AreaCount; i++)
    {
        if (msg->AreaTypes[i] == ismMESSAGE_AREA_PROPERTIES)
        {
            props->len = props->used = (int)msg->AreaLengths[i];
            props->buf = (char *)msg->pAreaData[i];
            break;
        }
    }

    // We have properties
    assert(props->buf != NULL);
}

//****************************************************************************
// @brief  Add contents of a message to the dump
//
// @param[in]     msg      Message to dump
// @param[in]     dumpHdl  Dump into which to add the message
//
// @remarks This function will only dump up-to dump->userDataBytes from each
//          area of the message. The special value -1 means dump all of the
//          message.
//****************************************************************************
void iem_dumpMessage(ismEngine_Message_t *msg, iedmDumpHandle_t dumpHdl)
{
    char propsEyeCatcher[12] = {'P','r','o','p','e','r','t','i','e','s','\0','\0'};
    char *eyePropsCount = &propsEyeCatcher[10];
    char payloadEyeCatcher[9] = {'P','a','y','l','o','a','d','\0','\0'};
    char *eyePayloadCount = &payloadEyeCatcher[7];
    size_t length;
    iedmDump_t *dump = (iedmDump_t *)dumpHdl;
    int64_t userDataBytes = dump->userDataBytes;

    assert(userDataBytes != 0);

    iedm_dumpData(dump, "ismEngine_Message_t", msg, sizeof(ismEngine_Message_t));

    if (msg->AreaCount <= ismMESSAGE_AREA_COUNT)
    {
        *eyePropsCount = '1';
        *eyePayloadCount = '1';

        for(int32_t i=0; i<msg->AreaCount; i++)
        {
            char *eyeCatcher;
            char *eyeCount;

            if (msg->AreaTypes[i] == ismMESSAGE_AREA_PROPERTIES)
            {
                eyeCatcher = propsEyeCatcher;
                eyeCount = eyePropsCount;
            }
            else
            {
                eyeCatcher = payloadEyeCatcher;
                eyeCount = eyePayloadCount;
            }

            if ((length = msg->AreaLengths[i]) > 0)
            {
                // Limit by the userDataBytes value
                if (userDataBytes != -1 && length > userDataBytes)
                {
                    length = userDataBytes;
                }

                iedm_dumpData(dump, eyeCatcher, msg->pAreaData[i], length);
            }

            (*eyeCount)++;
        }
    }

    return;
}
