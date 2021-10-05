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
/// @file  clientStateExpiry.h
/// @brief Engine component header file for tracking expiry and delayed
///        Will messages for clientStates
//*********************************************************************
#ifndef __ISM_CLIENTSTATEEXPIRY_INTERNAL_DEFINED
#define __ISM_CLIENTSTATEEXPIRY_INTERNAL_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include "engineCommon.h"     /* Engine common internal header file  */
#include "engineInternal.h"   /* Engine internal header file         */

#include "clientStateExpiry.h"

#define ieceNO_TIMED_SCAN_SCHEDULED        ((ism_time_t)UINT64_MAX)  ///> No time scheduled (or mid-scan)
#define ieceMAX_TABLE_CHAINS_TO_SCAN       1000                      ///> How many chains to scan before releasing the lock
#define ieceMAX_CLIENTSEXPIRY_BATCH_SIZE   100                       ///> Maximum number of clientStates to expire per scan loop

typedef struct tag_ieceFindDelayedActionClientStateContext_t
{
    ism_time_t now;
    ismEngine_ClientState_t *expiringClients[ieceMAX_CLIENTSEXPIRY_BATCH_SIZE];
    uint32_t expiringClientCount;
    ismEngine_ClientState_t *willMsgClients[ieceMAX_CLIENTSEXPIRY_BATCH_SIZE];
    uint32_t willMsgClientCount;
    ism_time_t lowestTimeSeen;  ///> The lowest time seen but not actioned
    uint32_t tableGeneration;
    uint32_t startIndex;
} ieceFindDelayedActionClientStateContext_t;

#endif /* __ISM_CLIENTSTATEEXPIRY_INTERNAL_DEFINED */
