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
/// @file  messageInternal.h
/// @brief Definition of a message. Needed for both engine and mediations
///        This file is temporary - a new home is needed for this definition
//****************************************************************************

#ifndef __ISM_MSGINTERNAL_DEFINED
#define __ISM_MSGINTERNAL_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "ismmessage.h"
#include "store.h"

typedef struct tag_iereResourceSet_t *iereResourceSetHandle_t;  // defined in resourceSetStats.h

/*************************************************************************/
/* ismMESSAGE_AREA_COUNT is a count of the number of area types defined. */
/*                                                                       */
/* This includes an internal type ismMESSAGE_AREA_INTERNAL_HEADER which  */
/* is only ever used by the engine, and only then when writing a message */
/* to the store (and reading it back from the store).                    */
/*                                                                       */
/* For the in-memory representation of a message, we know that we don't  */
/* need to allow for this to type to be specified with other types, so   */
/* we can use a smaller array to store the types that will be specified. */
/*************************************************************************/
#define ismENGINE_MSG_AREAS_MAX (ismMESSAGE_AREA_COUNT-1)

//*********************************************************************
/// @brief  Message
///
/// Internal representation of a message.
//*********************************************************************
typedef struct ismEngine_Message_t
{
    char                        StrucId[4];                           ///< Eyecatcher "EMSG"
    uint32_t                    usageCount;                           ///< Number of usage of this message - must not deallocate when > 0
    ismMessageHeader_t          Header;                               ///< Message header
    uint8_t                     AreaCount;                            ///< Number of message areas in use
    uint8_t                     Flags;                                ///< Engine Message flags
    uint8_t                     futureField1;                         ///< 1 byte available for future use
    uint8_t                     futureField2;                         ///< 1 byte available for future use
    uint32_t                    futureField3;                         ///< 4 bytes available for future use
    ismMessageAreaType_t        AreaTypes[ismENGINE_MSG_AREAS_MAX];   ///< Types of message areas (see remarks)
    size_t                      AreaLengths[ismENGINE_MSG_AREAS_MAX]; ///< Lengths of message areas (see remarks)
    void                       *pAreaData[ismENGINE_MSG_AREAS_MAX];   ///< Ptrs to message areas (see remarks)
    size_t                      MsgLength;                            ///< Sum of all Area lengths
    void                       *recovNext;                            ///< Next message in recovery chain (valid during recovery only)
    iereResourceSetHandle_t     resourceSet;                          ///< Resource set to which this message currently belongs
    int64_t                     fullMemSize;                          ///< The size of this message's memory area
    union ismEngine_StoreMsg_t
    {
        __uint128_t whole;
        struct
        {
            uint64_t            RefCount;                             ///< Number of store references to Store Msg
            ismStore_Handle_t   hStoreMsg;                            ///< Store handle for Message Record
        } parts;
    } StoreMsg;
} ismEngine_Message_t;

#define ismENGINE_MESSAGE_STRUCID "EMSG"

#define ismENGINE_MSGFLAGS_NONE         0
#define ismENGINE_MSGFLAGS_ALLOCTYPE_1  0x01    // Areas separate from header

#ifdef __cplusplus
}
#endif

#endif /* __ISM_MSGINTERNAL_DEFINED */


