/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
/// @file  exportMessage.c
/// @brief Export / Import functions for Messages
//*********************************************************************
#define TRACE_COMP Engine

#include <assert.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "engineInternal.h"
#include "engineHashSet.h"
#include "exportResources.h"
#include "engineStore.h"
#include "msgCommon.h"

//****************************************************************************
/// @internal
///
/// @brief  Export an individual message
///
/// @param[in]     message              The message being exported
/// @param[in]     control              Export control structure
/// @param[in]     usagePreIncremented  If the message has already had it's usage
///                                     incremented, this is set to indicate that
///
/// @remark If the message is added to the exportedMessages hashSet, it also
///         has its usage count increased - the exportedMessages hashSet must
///         therefore be enumerated and all messages found released at the end
///         of processing.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_exportMessage(ieutThreadData_t *pThreadData,
                           ismEngine_Message_t *message,
                           ieieExportResourceControl_t *control,
                           bool usagePreIncremented)
{
    int32_t rc = OK;
    bool releaseRequired = false;

    // If it's already been exported, it will be in the exportedMsgs hashSet
    rc = ieut_findValueInHashSet(control->exportedMsgs, (uint64_t)message);

    if (rc == OK)
    {
        if (usagePreIncremented) releaseRequired = true;
    }
    else
    {
        assert(rc == ISMRC_NotFound);

        rc = ieut_addValueToHashSet(pThreadData, control->exportedMsgs, (uint64_t)message);

        if (rc == OK)
        {
            if (!usagePreIncremented) iem_recordMessageUsage(message);

            ieutTRACEL(pThreadData, message, ENGINE_HIFREQ_FNC_TRACE, "Exporting message=%p'\n", message);

            char *Frags[ismMESSAGE_AREA_COUNT + 2];
            uint32_t FragLengths[ismMESSAGE_AREA_COUNT + 2];
            uint32_t TotalRecordLength;

            iestMessageHdrArea_t MsgHdrArea;
            iestMessageRecord_t MsgRecord;

            iest_setupPersistedMsgData( pThreadData
                                      , message
                                      , &MsgRecord
                                      , &MsgHdrArea
                                      , &TotalRecordLength
                                      , Frags
                                      , FragLengths);

            ieieFragmentedExportData_t FragsData = { message->AreaCount + 2
                                                   , (void **)&Frags
                                                   , FragLengths};

            rc = ieie_writeExportRecordFrags( pThreadData
                                            , control
                                            , ieieDATATYPE_EXPORTEDMESSAGERECORD
                                            , (uint64_t)message
                                            , &FragsData);
        }
        else
        {
            if (usagePreIncremented) releaseRequired = true;
        }
    }

    // Release the message usage.
    if (releaseRequired) iem_releaseMessage(pThreadData, message);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Import an individual message
///
/// @param[in]     control              Import control structure
/// @param[in]     dataId               Identifier given to the message on export
/// @param[in]     data                 Data of the message
/// @param[in]     dataLen              Length of the data
///
/// @remark If the message is not already in the importedMessages hashTable, it
///         also has its usage count increased - the importedMessages hashTable
///         must therefore be enumerated and all messages found released at the
///         end of processing.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_importMessage(ieutThreadData_t *pThreadData,
                           ieieImportResourceControl_t *control,
                           uint64_t dataId,
                           uint8_t *data,
                           size_t dataLen)
{
    int32_t rc = OK;
    ismEngine_Message_t *message;
    uint32_t dataIdHash = (uint32_t)(dataId>>4);

    ieutTRACEL(pThreadData, dataId, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_ENTRY "dataId=0x%0lx dateLen=%lx\n",
               __func__, dataId, dataLen);

    // If it's already been imported, it will be in the table
    ismEngine_getRWLockForRead(&control->importedTablesLock);
    rc = ieut_getHashEntry(control->importedMsgs,
                           (const char *)dataId,
                           dataIdHash,
                           (void **)&message);
    ismEngine_unlockRWLock(&control->importedTablesLock);

    if (rc == OK)
    {
        goto mod_exit;
    }
    else
    {
        assert(rc == ISMRC_NotFound);

        uint8_t *pData = data;

        iestMessageRecord_t *pMsgRecord;
        iestMessageHdrArea_t *pMsgHdrArea;
        ismMessageHeader_t msgHeader;

        assert(dataLen >= sizeof(iestMessageRecord_t) + sizeof(iestMessageHdrArea_t));

        // Should have a MessageRecord and Header
        pMsgRecord = (iestMessageRecord_t *)(pData);
        pData += sizeof(iestMessageRecord_t);

        pMsgHdrArea = (iestMessageHdrArea_t *)(pData);
        pData += sizeof(iestMessageHdrArea_t);

        uint8_t areaCount = pMsgRecord->AreaCount-1;

        // Only currently copes with version 1
        if (pMsgRecord->Version != iestMR_VERSION_1 || pMsgHdrArea->Version != iestMHA_VERSION_1)
        {
            rc = ISMRC_InvalidValue;
            ism_common_setError(rc);
            goto mod_exit;
        }

        msgHeader.Persistence = pMsgHdrArea->Persistence;
        msgHeader.Reliability = pMsgHdrArea->Reliability;
        msgHeader.Priority = pMsgHdrArea->Priority;
        msgHeader.Expiry = pMsgHdrArea->Expiry;
        msgHeader.Flags = pMsgHdrArea->Flags;
        msgHeader.MessageType = pMsgHdrArea->MessageType;

        ismMessageAreaType_t areaTypes[areaCount];
        size_t areaLengths[areaCount];
        void *pAreaData[areaCount];

        char xbuf[1024];
        concat_alloc_t alteredProps = {xbuf, sizeof(xbuf)};

        for(uint8_t i=0; i<areaCount; i++)
        {
            areaTypes[i] = pMsgRecord->AreaTypes[i+1];
            areaLengths[i] = pMsgRecord->AreaLen[i+1];
            pAreaData[i] = pData;
            pData += areaLengths[i];

            // The Message properties might need to be altered as part of the import
            //
            // Specifically, we alter the OriginServer on retained messages to the importing
            // server's serverUID so that the importing server can take ownership of the
            // messages if it joins a cluster in the future.
            if (areaTypes[i] == ismMESSAGE_AREA_PROPERTIES)
            {
                size_t proplen = areaLengths[i];
                char *propp = pAreaData[i];

                concat_alloc_t  props = {propp, proplen, proplen};
                ism_field_t field;

                // Look for an originating serverUID
                field.val.s = NULL;
                ism_common_findPropertyID(&props, ID_OriginServer, &field);

                // The properties contain an originating server, so we need to modify
                if (field.val.s != NULL)
                {
                    uint32_t propIndex = 0;
                    int32_t propNameID = 0;
                    char *propName = NULL;

                    props.pos = 0;

                    // The properties appear as 'Name/NameIndex' then 'Value' so to
                    // replace a particular one, we go through them and when we find
                    // NameIndex of ID_OriginServer use the local server UID instead.
                    while(ism_protocol_getObjectValue(&props, &field) == 0)
                    {
                        propIndex++;

                        if (field.type == VT_NameIndex)
                        {
                            assert((propIndex % 2) == 1);

                            propNameID = field.val.i;
                            propName = NULL;
                        }
                        else if (field.type == VT_Name)
                        {
                            assert((propIndex % 2) == 1);

                            propName = field.val.s;
                        }
                        else
                        {
                            assert((propIndex % 2) == 0);

                            if (propName)
                            {
                                ism_common_putProperty(&alteredProps, propName, &field);
                                propName = NULL;
                            }
                            else
                            {
                                // Replace the specified original OriginServer with this server.
                                if (propNameID == ID_OriginServer)
                                {
                                    field.val.s = (char *)ism_common_getServerUID();
                                }

                                ism_common_putPropertyID(&alteredProps, propNameID, &field);
                            }
                        }
                    }

                    areaLengths[i] = alteredProps.used;
                    pAreaData[i] = alteredProps.buf;
                }
            }
        }

        rc = ism_engine_createMessage(&msgHeader,
                                      areaCount,
                                      areaTypes,
                                      areaLengths,
                                      pAreaData,
                                      &message);

        ism_common_freeAllocBuffer(&alteredProps);

        if (rc != OK) goto mod_exit;

        ismEngine_getRWLockForWrite(&control->importedTablesLock);
        rc = ieut_putHashEntry(pThreadData,
                               control->importedMsgs,
                               ieutFLAG_NUMERIC_KEY,
                               (const char *)dataId,
                               dataIdHash,
                               message,
                               0);
        ismEngine_unlockRWLock(&control->importedTablesLock);

        if (rc != OK)
        {
            iem_releaseMessage(pThreadData, message);
            goto mod_exit;
        }
    }

mod_exit:

    ieutTRACEL(pThreadData, message, ENGINE_HIFREQ_FNC_TRACE, FUNCTION_EXIT "message=%p, rc=%d\n",
               __func__, message, rc);

    return rc;
}

//****************************************************************************
/// @internal
///
/// @brief  Find a message by dataId
///
/// @param[in]     control              Import control structure
/// @param[in]     dataId               Identifier given to the message on export
/// @param[out]    ppMessage            Message found
///
/// @remark If the message is found, it has it's useCount increased so if it is
///         not released by a call the caller makes subsequently, it must be
///         explicitly released by the caller.
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_findImportedMessage(ieutThreadData_t *pThreadData,
                                 ieieImportResourceControl_t *control,
                                 uint64_t dataId,
                                 ismEngine_Message_t **ppMessage)
{
    uint32_t dataIdHash = (uint32_t)(dataId>>4);

    ismEngine_getRWLockForRead(&control->importedTablesLock);
    int32_t rc = ieut_getHashEntry(control->importedMsgs,
                                   (const char *)dataId,
                                   dataIdHash,
                                   (void **)ppMessage);
    ismEngine_unlockRWLock(&control->importedTablesLock);

    if (rc == OK)
    {
        iem_recordMessageUsage(*ppMessage);
    }

    return rc;
}

/*********************************************************************/
/*                                                                   */
/* End of exportMessage.c                                            */
/*                                                                   */
/*********************************************************************/
