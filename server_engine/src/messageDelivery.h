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
/// @file  messageDelivery.h
/// @brief Management of message delivery header file
//*********************************************************************
#ifndef __ISM_MESSAGEDELIVERY_DEFINED
#define __ISM_MESSAGEDELIVERY_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include "engineCommon.h"     /* Engine common internal header file  */
#include "engineInternal.h"   /* Engine internal header file         */


/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/

// Callback from destination when a message is being delivered
bool ism_engine_deliverMessage(ieutThreadData_t *pThreadData,
                               ismEngine_Consumer_t *pConsumer,
                               void *pDelivery,
                               ismEngine_Message_t *pMessage,
                               ismMessageHeader_t *pMsgHdr,
                               ismMessageState_t messageState,
                               uint32_t deliveryId);

// Callback from destination when delivery status changes, such as
// an empty destination or disabling a waiter
void ism_engine_deliverStatus(ieutThreadData_t *pThreadData,
                              ismEngine_Consumer_t *pConsumer,
                              int32_t retcode);

#endif /* __ISM_MESSAGEDELIVERY_DEFINED */

/*********************************************************************/
/* End of messageDelivery.h                                          */
/*********************************************************************/
