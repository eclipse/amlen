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
/// @file  destination.h
/// @brief Engine component destination header file
//*********************************************************************
#ifndef __ISM_DESTINATION_DEFINED
#define __ISM_DESTINATION_DEFINED

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

// Allocate a destination
int32_t ieds_create_newDestination(ieutThreadData_t *pThreadData,
                                   iereResourceSetHandle_t resourceSet,
                                   uint8_t destinationType,
                                   const char *pDestinationName,
                                   ismEngine_Destination_t **ppDestination);

#define iedsPUBLISH_OPTION_NONE                      0x00000000  ///< ieds_publish should publish the message as normal
#define iedsPUBLISH_OPTION_ONLY_UPDATE_RETAINED      0x00000001  ///< ieds_publish should only perform retained message updating
                                                                 ///< i.e. not actually publish to any subscribers
#define iedsPUBLISH_OPTION_POTENTIAL_REPUBLISH       0x00000002  ///< ieds_publish is being called for a message that could be
                                                                 ///< being republished (a retained msg as part of an import for instance)
#define iedsPUBLISH_OPTION_REPOSITIONING_RETAINED    0x00000004  ///< ieds_publish is being called to reposition a retained message
#define iedsPUBLISH_OPTION_INFORMATIONAL_RETCODES    0x00000008  ///< ieds_publish should return information return codes, e.g. ISMRC_SomeDestinationsFull
#define iedsPUBLISH_OPTION_FAIL_IF_ASYNC_CBQ_ALERTED 0x01000000  ///< ieds_publish should fail rather than pause if the async callback queue is alerted

// Internal function to publish a message to a topic
int32_t ieds_publish(ieutThreadData_t *pThreadData,
                     ismEngine_ClientState_t *pClient,
                     const char *pTopicString,
                     uint32_t options,
                     ismEngine_Transaction_t *pTran,
                     ismEngine_Message_t *pMessage,
                     uint32_t unrelDeliveryId,
                     ismEngine_UnreleasedHandle_t *phUnrel,
                     size_t contextLength,
                     ietrAsyncTransactionDataHandle_t *pAsyncDataHandle);

int32_t ieds_putToQueueName(ieutThreadData_t *pThreadData,
                            ismEngine_ClientState_t *pClient,
                            const char *pQueueName,
                            ismEngine_Transaction_t *pTran,
                            ismEngine_Message_t *pMessage);

int32_t ieds_put(ieutThreadData_t *pThreadData,
                 ismEngine_ClientState_t *pClient,
                 ismEngine_Producer_t *pProducer,
                 ismEngine_Transaction_t *pTran,
                 ismEngine_Message_t *pMessage);


#endif /* __ISM_DESTINATION_DEFINED */

/*********************************************************************/
/* End of destination.h                                              */
/*********************************************************************/
