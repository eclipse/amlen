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
/// @file  msgCommon.h
/// @brief Common message functions header file
//*********************************************************************
#ifndef __ISM_MSGCOMMON_DEFINED
#define __ISM_MSGCOMMON_DEFINED

#include "engine.h"
#include "engineCommon.h"

void iem_recordMessageUsage(ismEngine_Message_t *msg);
void iem_recordMessageMultipleUsage(ismEngine_Message_t *msg, uint32_t newUsers);
void iem_releaseMessage(ieutThreadData_t *pThreadData, ismEngine_Message_t *pMessage);
int32_t iem_createMessageCopy(ieutThreadData_t *pThreadData,
                              ismEngine_Message_t *pMessage,
                              bool simpleCopy,
                              ism_time_t retainedTimestamp,
                              uint32_t retainedRealExpiry,
                              ismEngine_Message_t **ppNewMessage);
void iem_locateMessageProperties(ismEngine_Message_t *msg, concat_alloc_t *props);
void iem_dumpMessage(ismEngine_Message_t *msg, iedmDumpHandle_t dumpHdl);

#endif /* __ISM_MSGCOMMON_DEFINED */

/*********************************************************************/
/* End of msgCommon.h                                                */
/*********************************************************************/
