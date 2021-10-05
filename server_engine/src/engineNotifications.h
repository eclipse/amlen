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
/// @file  engineNotifications.h
/// @brief Data types for Engine notification processing
//****************************************************************************
#ifndef __ISM_ENGINENOTIFICATIONS_DEFINED
#define __ISM_ENGINENOTIFICATIONS_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include <engine.h>           /* Engine external header file         */

#define ienfDCN_TOPIC "$SYS/DisconnectedClientNotification"

/*********************************************************************/
/*                                                                   */
/* DATA TYPES                                                        */
/*                                                                   */
/*********************************************************************/
typedef struct tag_ienfClientStatesContext_t
{
    int32_t           rc;           // Return code recorded inside callback function
    uint32_t          resultCount;  // Number of results (not pairs)
    char            **results;      // Pairs of ClientId,UserId for matching client states
    uint32_t          startIndex;   // Start (and finish) index to use in the array
    uint32_t          resultSize;   // Size of results array (not pairs)
    bool              DCNClients;   // Interested in disconnected (zombie) clients with disconnected client notification subs
} ienfClientStatesContext_t;

typedef struct tag_ienfSubscription_t
{
    ismEngine_SubscriptionHandle_t subHandle;
    ismEngine_QueueStatistics_t    stats;
} ienfSubscription_t;

typedef struct tag_ienfBuildDCNMessageContext_t
{
    int32_t             rc;           // Return code recorded inside callback function
    char               *clientId;     // ClientId being examined
    char               *userId;       // UserId being examined
    concat_alloc_t      payload;      // Buffer used to build the payload for a message
    ienfSubscription_t *subsIncluded; // Subscriptions included in this message
    uint32_t            subsCount;    // Count of subscriptions included
    uint32_t            subsMax;      // Maximum subscriptions in subsIncluded array
} ienfBuildDCNMessageContext_t;

#endif /* __ISM_ENGINENOTIFICATIONS_DEFINED */

/*********************************************************************/
/* End of engineNotifications.h                                      */
/*********************************************************************/
