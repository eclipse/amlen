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
/// @file dumpFormatCustomFormatters.c
/// @brief In-built custom formatting routines for engine dumps.
//****************************************************************************
#define TRACE_COMP Engine

#include <dumpFormat.h>
#include <engine.h>
#include <lockManager.h>
#include <simpQInternal.h>
#include <intermediateQInternal.h>
#include <multiConsumerQInternal.h>

// Define custom formatters
typedef struct tag_iefmCustomFormatter_t
{
    char *name;
    iefmStructureFormatter_t formatter;
} iefmCustomFormatter_t;

#define iefmENUM_CASE(_hdr, _val)\
    case _val:\
        iefm_printLine(_hdr, #_val);\
        break;

int32_t iefm_QName_Formatter(iefmHeader_t *dumpHeader,iefmStructureDescription_t *structure);
int32_t iefm_LockName_Formatter(iefmHeader_t *dumpHeader, iefmStructureDescription_t *structure);
int32_t iefm_pthread_mutex_Formatter(iefmHeader_t *dumpHeader, iefmStructureDescription_t *structure);
int32_t iefm_pthread_rwlock_Formatter(iefmHeader_t *dumpHeader, iefmStructureDescription_t *structure);
int32_t iefm_iewsWaiterStatus_Formatter(iefmHeader_t *dumpHeader, iefmStructureDescription_t *structure);
int32_t iefm_ConsumerCounts_Formatter(iefmHeader_t *dumpHeader, iefmStructureDescription_t *structure);
int32_t iefm_iesqQNodePage_t_Formatter(iefmHeader_t *dumpHeader, iefmStructureDescription_t *structure);
int32_t iefm_ieiqQNodePage_t_Formatter(iefmHeader_t *dumpHeader, iefmStructureDescription_t *structure);
int32_t iefm_iemqQNodePageAndLocks_Formatter(iefmHeader_t *dumpHeader, iefmStructureDescription_t *structure);
int32_t iefm_ism_xid_t_Formatter(iefmHeader_t *dumpHeader, iefmStructureDescription_t *structure);

/// @brief Array of custom formatting functions terminated by {NULL, NULL}
/// @remark For a function to be picked up, it must be added to this array with
///         the structure name with which it is expected to be called.
static iefmCustomFormatter_t customFormatters[] = { {"QName", iefm_QName_Formatter},
                                                    {"ielmLockName_t", iefm_LockName_Formatter},
                                                    {"iewsWaiterStatus_t", iefm_iewsWaiterStatus_Formatter},
                                                    {"ismEngine_ConsumerCounts_t", iefm_ConsumerCounts_Formatter },
                                                    {"pthread_mutex_t", iefm_pthread_mutex_Formatter},
                                                    {"pthread_rwlock_t", iefm_pthread_rwlock_Formatter},
                                                    {"iesqQNodePage_t", iefm_iesqQNodePage_t_Formatter},
                                                    {"ieiqQNodePage_t", iefm_ieiqQNodePage_t_Formatter},
                                                    {"iemqQNodePageAndLocks", iefm_iemqQNodePageAndLocks_Formatter},
                                                    {"ism_xid_t", iefm_ism_xid_t_Formatter},
                                                    {NULL, NULL}};

//****************************************************************************
/// @internal
///
/// @brief Map some types to other types
///
/// @param[in]     dumpHeader  Header information from the dump
/// @param[in,out] type        Type string to check, updated if mapped
/// @param[in]     hash        Type hash
///
/// @returns true if a mapping was made, or false
//****************************************************************************
bool iefm_mapTypes(iefmHeader_t *dumpHeader, char *type, uint32_t hash)
{
    bool mapped = true;

    switch(hash)
    {
        case iefmHASH_ismEngine_MessageHandle_t:
            strcpy(type, iefmKEY_ismEngine_Message_t);
            strcat(type, " *");
            break;
        case iefmHASH_ismQHandle_t:
            strcpy(type, iefmKEY_ismEngine_Queue_t);
            strcat(type, " *");
            break;
        case iefmHASH_iemem_memoryType:
        case iefmHASH_ieutHashTableLocking_t:
        case iefmHASH_ismQueueType_t:
        case iefmHASH_ieutReservationState_t:
            strcpy(type, iefmKEY_enum);
            break;
        case iefmHASH_pthread_spinlock_t:
            strcpy(type, iefmKEY_int32_t);
            break;
        case iefmHASH_ieqOptions_t:
        case iefmHASH_pthread_key_t:
            strcpy(type, iefmKEY_uint32_t);
            break;
        case iefmHASH_ismStore_Handle_t:
        case iefmHASH_ism_timer_t:
            strcpy(type, iefmKEY_uint64_t);
            break;
        case iefmHASH_size_t:
            if (dumpHeader->inputSizeTSize == 8)
                strcpy(type, iefmKEY_uint64_t);
            else
                strcpy(type, iefmKEY_uint32_t);
            break;
        default:
            if ((strncmp(type, "ie", 2) == 0 && strstr(type, "Handle") != NULL) || // Engine Handles
                strstr(type, "Callback_t") != NULL)                                // Engine Callbacks
            {
                strcpy(type, iefmKEY_void);
                strcat(type, " *");
            }
            else
            {
                mapped = false;
            }
            break;
    }

    return mapped;
}

//****************************************************************************
/// @internal
///
/// @brief Find a custom formatter for the specified structure
///
/// @param[in]     dumpHeader        The dump formatting header
/// @param[in]     structureName     The name of the structure
/// @param[in]     defaultFormatter  The default formatting function to use
///                                  if no other is found.
///
/// @remark This will find a function named iefm_<STRUCTURE>_Formatter in a
///         loaded library of formatting routines, failing that, it tries to
///         load one from the program, if all else fails, the default specified
///         is used.
///
/// @returns Pointer to either the special formatter, or a generic one.
//****************************************************************************
iefmStructureFormatter_t iefm_findCustomFormatter(iefmHeader_t *dumpHeader,
                                                  char *structureName,
                                                  iefmStructureFormatter_t defaultFormatter)
{
    iefmStructureFormatter_t formatter = NULL;

    char structureFormatterName[50+strlen(structureName)];

    sprintf(structureFormatterName, "iefm_%s_Formatter", structureName);

    // Look for a formatter in the user-provided library
    if (dumpHeader->libHandle != NULL)
    {
        formatter = dlsym(dumpHeader->libHandle, structureFormatterName);
        if (formatter != NULL) goto mod_exit;
    }

    // See if there is a custom formatter for this structure name
    iefmCustomFormatter_t *check = &customFormatters[0];

    while(check->name != NULL)
    {
        if (structureName[0] == check->name[0] &&
            strcmp(structureName, check->name) == 0)
        {
            formatter = check->formatter;
            break;
        }

        check++;
    }

    if (formatter != NULL) goto mod_exit;

    // Didn't find a local one, use the default provided
    formatter = defaultFormatter;

mod_exit:

    return formatter;
}

//****************************************************************************
/// @internal
/// @brief Custom formatter for data blocks labelled "QName"
//****************************************************************************
int32_t iefm_QName_Formatter(iefmHeader_t *dumpHeader,
                             iefmStructureDescription_t *structure)
{
    iefm_printLine(dumpHeader, "%p - %p QName \"%s\"",
                   structure->startAddress, structure->endAddress, structure->buffer);
    return OK;
}

//****************************************************************************
/// @internal
/// @brief Custom formatter for data blocks labelled "iewsWaiterStatus_t"
///
/// @remarks Note, this formatter uses the fact that although all three types
///          are distinct, they use the same values (at the moment).
//****************************************************************************
int32_t iefm_iewsWaiterStatus_Formatter(iefmHeader_t *dumpHeader,
                                        iefmStructureDescription_t *structure)
{
    uint64_t waiterStatus = iefm_getUint64(structure->buffer, dumpHeader);

    switch(waiterStatus & 0x000000000000000F)
    {
        case 0x0000000000000000:
            iefm_print(dumpHeader, "Disconnected");
            break;
        case 0x0000000000000001:
            iefm_print(dumpHeader, "Disabled");
            break;
        case 0x0000000000000002:
            iefm_print(dumpHeader, "Enabled");
            break;
        case 0x0000000000000004:
            iefm_print(dumpHeader, "Getting");
            break;
        case 0x0000000000000008:
            iefm_print(dumpHeader, "Delivering");
            break;
    }

    if (waiterStatus & 0x0000000000000010) iefm_print(dumpHeader, " | Disable_Pend");
    if (waiterStatus & 0x0000000000000020) iefm_print(dumpHeader, " | Enable_Pend");
    if (waiterStatus & 0x0000000000000040) iefm_print(dumpHeader, " | Disconnect_Pend");

    iefm_printLineFeed(dumpHeader);

    return OK;
}


//****************************************************************************
/// @internal
/// @brief Custom formatter for data blocks labelled "ielmLockName_t"
//****************************************************************************
int32_t iefm_LockName_Formatter(iefmHeader_t *dumpHeader,
                                iefmStructureDescription_t *structure)
{
    if (structure->length == sizeof(ielmLockName_t))
    {
        ielmLockName_t lockName;

        memcpy(&lockName, structure->buffer, sizeof(lockName));
        iefm_printLine(dumpHeader, "<LockType %u, QId %u, NodeId %lu>",
                       lockName.Msg.LockType, lockName.Msg.QId, lockName.Msg.NodeId);
    }
    else
    {
        iefm_dataFormatter(dumpHeader, structure);
    }

    return OK;
}

//****************************************************************************
/// @internal
/// @brief Custom formatter for data blocks labelled "ismEngine_ConsumerCounts_t"
//****************************************************************************
int32_t iefm_ConsumerCounts_Formatter(iefmHeader_t *dumpHeader,
                                      iefmStructureDescription_t *structure)
{
    if (structure->length == sizeof(ismEngine_ConsumerCounts_t))
    {
        ismEngine_ConsumerCounts_t counts;

        memcpy(&counts, structure->buffer, sizeof(counts));
        iefm_printLine(dumpHeader, "<whole %lu, useCount %u, pendingAckCount %u>",
                       counts.whole, counts.parts.useCount, counts.parts.pendingAckCount);
    }
    else
    {
        iefm_dataFormatter(dumpHeader, structure);
    }

    return OK;
}

//****************************************************************************
/// @internal
/// @brief Custom formatter for data blocks labelled "pthread_mutex_t"
//****************************************************************************
int32_t iefm_pthread_mutex_Formatter(iefmHeader_t *dumpHeader,
                                     iefmStructureDescription_t *structure)
{
    if (structure->length == sizeof(pthread_mutex_t))
    {
        pthread_mutex_t mutex;

        memcpy(&mutex, structure->buffer, sizeof(mutex));
        iefm_printLine(dumpHeader, "<kind %d, owner %d, nusers %u>",
                       mutex.__data.__kind, mutex.__data.__owner, mutex.__data.__nusers);
    }
    else
    {
        iefm_dataFormatter(dumpHeader, structure);
    }
    return OK;
}

//****************************************************************************
/// @internal
/// @brief Custom formatter for data blocks labelled "pthread_rwlock_t"
//****************************************************************************
int32_t iefm_pthread_rwlock_Formatter(iefmHeader_t *dumpHeader,
                                      iefmStructureDescription_t *structure)
{
#if __GLIBC_MINOR__ < 25
    if (structure->length == sizeof(pthread_rwlock_t))
    {
        pthread_rwlock_t rwlock;

        memcpy(&rwlock, structure->buffer, sizeof(rwlock));
        iefm_printLine(dumpHeader, "<writer %d, nr_writers_queued %u, nr_readers %u, nr_readers_queued %u>",
                       rwlock.__data.__writer, rwlock.__data.__nr_writers_queued,
                       rwlock.__data.__nr_readers, rwlock.__data.__nr_readers_queued);
    }
    else
#endif
    {
        iefm_dataFormatter(dumpHeader, structure);
    }

    return OK;
}

//****************************************************************************
/// @internal
/// @brief Convert an XID to it's string representation
/// @remark This is based on ism_common_xidToString - copied locally to
///         remove reliance on common functions
//****************************************************************************
const char * iefm_xidToString(ism_xid_t * xid, char * buf, int len)
{
    char   out[278];
    char * outp = out;
    uint8_t * in;
    int    i;
    int    shift = 28;
    static char myhex [18] = "0123456789ABCDEF";

    if (!xid || (uint32_t)xid->bqual_length>64 || (uint32_t)xid->gtrid_length>64) {
        if (len)
            *buf = 0;
        return NULL;
    }

    /* Put out the format ID in hex with no leading zeros */
    for (i=0; i<7; i++) {
        if ((xid->formatID>>shift)&0xf)
            break;
        shift -= 4;
    }
    for (; i<7; i++) {
        *outp++ = myhex[(xid->formatID>>shift)&0xf];
        shift -= 4;
    }
    *outp++ = myhex[xid->formatID&0xf];
    *outp++ = ':';

    /* Put out the branch qualifier */
    in = (uint8_t *)xid->data;
    for (i=0; i<xid->bqual_length; i++) {
        *outp++ = myhex[(*in>>4)&0xf];
        *outp++ = myhex[*in++&0xf];
    }
    *outp++ = ':';

    /* Put out the global transaction ID */
    for (i=0; i<xid->gtrid_length; i++) {
        *outp++ = myhex[(*in>>4)&0xf];
        *outp++ = myhex[*in++&0xf];
    }
    *outp = 0;

    /* Copy into the output buffer - this is a copy of ism_common_strlcpy */
    size_t outlen = strlen(out);
    if (outlen < len) {
        memcpy(buf, out, outlen+1);
    } else if (len > 0) {
        memcpy(buf, out, len-1);
        out[len-1] = 0;
    }

    return buf;
}

//****************************************************************************
/// @internal
/// @brief Custom formatter for data blocks labelled "ism_xid_t"
//****************************************************************************
int32_t iefm_ism_xid_t_Formatter(iefmHeader_t *dumpHeader,
                                 iefmStructureDescription_t *structure)
{
    if (structure->length == sizeof(ism_xid_t))
    {
        ism_xid_t xid;

        memcpy(&xid, structure->buffer, sizeof(ism_xid_t));

        char XIDBuffer[300];
        const char *XIDString;

        // Generate the string representation of the XID
        XIDString = iefm_xidToString(&xid, XIDBuffer, sizeof(XIDBuffer));

        iefm_printLine(dumpHeader, "<%s>", XIDString);
    }
    else
    {
        iefm_dataFormatter(dumpHeader, structure);
    }

    return OK;
}

//****************************************************************************
/// @internal
/// @brief Custom formatter for data blocks labelled "iesqQNodePage_t"
//****************************************************************************
int32_t iefm_iesqQNodePage_t_Formatter(iefmHeader_t *dumpHeader,
                                       iefmStructureDescription_t *structure)
{
    uint32_t index;

    if (structure->length >= sizeof(iesqQNodePage_t))
    {
        iesqQNodePage_t *nodePage = (iesqQNodePage_t *)structure->buffer;

        iefm_printLine(dumpHeader, "%p - %p iesqQNodePage_t", structure->startAddress, structure->endAddress);
        iefm_indent(dumpHeader);
        iefm_printLine(dumpHeader, "nextStatus:      %ld", nodePage->nextStatus);
        iefm_printLine(dumpHeader, "next:            %p", nodePage->next);
        iefm_printLine(dumpHeader, "nodesInPage:     %d", nodePage->nodesInPage);

        iefm_indent(dumpHeader);
        for (index = 0; index < nodePage->nodesInPage; index++)
        {
            iesqQNode_t *node=&(nodePage->nodes[index]);

            iefm_printLine(dumpHeader, "Msg: Node=%p, Msg=%p",
               ((char *)structure->startAddress) + offsetof(iesqQNodePage_t, nodes) + (sizeof(iesqQNode_t) * index),
               node->msg);
        }
        iefm_outdent(dumpHeader);
        iefm_outdent(dumpHeader);
    }
    else
    {
        iefm_dataFormatter(dumpHeader, structure);
    }

    return OK;
}

//****************************************************************************
/// @internal
/// @brief Custom formatter for data blocks labelled "ieiqQNodePage_t"
//****************************************************************************
int32_t iefm_ieiqQNodePage_t_Formatter(iefmHeader_t *dumpHeader,
                                       iefmStructureDescription_t *structure)
{
    uint32_t index;

    if (structure->length >= sizeof(ieiqQNodePage_t))
    {
        ieiqQNodePage_t *nodePage = (ieiqQNodePage_t *)structure->buffer;

        iefm_printLine(dumpHeader, "%p - %p ieiqQNodePage_t", structure->startAddress, structure->endAddress);
        iefm_indent(dumpHeader);
        iefm_printLine(dumpHeader, "SequenceNo:      %ld", nodePage->sequenceNo);
        iefm_printLine(dumpHeader, "nextStatus:      %ld", nodePage->nextStatus);
        iefm_printLine(dumpHeader, "next:            %p", nodePage->next);
        iefm_printLine(dumpHeader, "nodesInPage:     %d", nodePage->nodesInPage);

        iefm_indent(dumpHeader);
        for (index = 0; index < nodePage->nodesInPage; index++)
        {
            ieiqQNode_t *node=&(nodePage->nodes[index]);

            iefm_printLine(dumpHeader, "Msg: OId=%lu, State=%s, Msg=%p, %d, %hhd, %hhd, %s, %s, ref=%#X",
               node->orderId,
                ( (node->msgState == ismMESSAGE_STATE_AVAILABLE)? "avail":
                  ( (node->msgState == ismMESSAGE_STATE_DELIVERED) ? "dlvrd":
                    ( (node->msgState == ismMESSAGE_STATE_RECEIVED) ? "rcvd ":
                       ( (node->msgState == ismMESSAGE_STATE_CONSUMED) ? "consd": "!ERR!" )))),
                node->msg,
                node->deliveryId,
                node->deliveryCount,
                node->msgFlags,
                node->hasMDR?" mdr":"!mdr",
                node->inStore?" store":"!store",
                node->hMsgRef);
        }
        iefm_outdent(dumpHeader);
        iefm_outdent(dumpHeader);
    }
    else
    {
        iefm_dataFormatter(dumpHeader, structure);
    }

    return OK;
}

//****************************************************************************
/// @internal
/// @brief Custom formatter for data blocks labelled "iemqQNodePageAndLocks"
//****************************************************************************
int32_t iefm_iemqQNodePageAndLocks_Formatter(iefmHeader_t *dumpHeader,
                                             iefmStructureDescription_t *structure)
{
    uint32_t index;
    char ConsumerInfo[32];

    if (structure->length >= sizeof(iemqQNodePage_t))
    {
        iemqQNodePage_t *nodePage = (iemqQNodePage_t *)structure->buffer;
        bool *pnodeLocks = (bool *)(((char *)structure->buffer) +
                                    offsetof(iemqQNodePage_t, nodes) +
                                        (sizeof(iemqQNode_t) * nodePage->nodesInPage));
        iefm_printLine(dumpHeader, "%p - %p iemqQNodePage_t", structure->startAddress, structure->endAddress);
        iefm_indent(dumpHeader);
        iefm_printLine(dumpHeader, "nextStatus:      %ld", nodePage->nextStatus);
        iefm_printLine(dumpHeader, "next:            %p", nodePage->next);
        iefm_printLine(dumpHeader, "nodesInPage:     %d", nodePage->nodesInPage);

        iefm_indent(dumpHeader);
        for (index = 0; index < nodePage->nodesInPage; index++)
        {
            iemqQNode_t *node=&(nodePage->nodes[index]);

            if (node->ackData.pConsumer != NULL)
            {
               sprintf(ConsumerInfo, "Consumer=%p", node->ackData.pConsumer);
            }
            else
            {
                ConsumerInfo[0]='\0';
            }

            iefm_printLine(dumpHeader, "Msg: OId=%lu, State=%s, Msg=%p, %s, %s, ref=%#X, %s, 0x%x",
               node->orderId,
                ( (node->msgState == ismMESSAGE_STATE_AVAILABLE)? "avail":
                  ( (node->msgState == ismMESSAGE_STATE_DELIVERED) ? "dlvrd":
                    ( (node->msgState == ismMESSAGE_STATE_RECEIVED) ? "rcvd ":
                       ( (node->msgState == ismMESSAGE_STATE_CONSUMED) ? "consd": "!ERR!" )))),
                node->msg,
                pnodeLocks[index] ? " locked":"!locked",
                (node->inStore) ? " store": "!store",
                node->hMsgRef,
                ConsumerInfo,
                node->rehydratedState);
        }
        iefm_outdent(dumpHeader);
        iefm_outdent(dumpHeader);
    }
    else
    {
        iefm_dataFormatter(dumpHeader, structure);
    }

    return OK;
}

