/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
/// @file  messageExpiryInternal.h
/// @brief Engine component header file for tracking expiry of messages
//*********************************************************************
#ifndef __ISM_MESSAGEEXPIRY_INTERNAL_DEFINED
#define __ISM_MESSAGEEXPIRY_INTERNAL_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include "engineCommon.h"     /* Engine common internal header file  */
#include "engineInternal.h"   /* Engine internal header file         */

#include "messageExpiry.h"

/*********************************************************************/
/*                                                                   */
/* TYPE DEFINITIONS                                                  */
/*                                                                   */
/*********************************************************************/

typedef struct tag_iemeExpiryReaperQContext_t
{
    uint32_t       nowExpire;              ///< time that we should currently consider expired
    uint32_t       callbackCount;          ///< num times callback called
    uint32_t       statQNoWorkRequired;    ///< num queues we didn't need to reap
    uint32_t       statQReaped;            ///< num queues we successfully reaped
    uint32_t       statQNoLock;            ///< num queues we couldn't lock to reap
    uint32_t       statQNoMem;             ///< num queues we couldn't alloc mem to reap
    volatile bool *reaperEndRequested;     ///< Has the reaper been requested to end?
    uint32_t       earliestObservedExpiry; ///< The earliest (unexpired) expiry observed during a scan
} iemeExpiryReaperQContext_t;


#endif /* __ISM_MESSAGEEXPIRY_INTERNAL_DEFINED */
