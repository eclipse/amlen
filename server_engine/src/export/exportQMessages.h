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
/// @file exportQMessages.h
/// @brief Functions for exporting messages from queues
///
//*********************************************************************

#ifndef EXPORTQMESSAGES_H_DEFINED
#define EXPORTQMESSAGES_H_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "engineInternal.h"
#include "exportResources.h"

int32_t ieie_exportSimpQMessages(ieutThreadData_t *pThreadData,
                                 ismQHandle_t Qhdl,
                                 ieieExportResourceControl_t *control);

int32_t ieie_exportInterQMessages(ieutThreadData_t *pThreadData,
                                  ismQHandle_t Qhdl,
                                  ieieExportResourceControl_t *control);

int32_t ieie_exportMultiConsumerQMessages(ieutThreadData_t *pThreadData,
                                          ismQHandle_t Qhdl,
                                          ieieExportResourceControl_t *control);

int32_t ieie_exportInflightMessages(ieutThreadData_t *pThreadData,
                                    ieieExportResourceControl_t *control);

int32_t ieie_importSimpQNode(ieutThreadData_t            *pThreadData,
                             ieieImportResourceControl_t *pControl,
                             uint64_t                     dataId,
                             void                        *data,
                             size_t                       dataLen);

int32_t ieie_importInterQNode(ieutThreadData_t            *pThreadData,
                              ieieImportResourceControl_t *pControl,
                              uint64_t                     dataId,
                              void                        *data,
                              size_t                       dataLen);

int32_t ieie_importMultiConsumerQNode(ieutThreadData_t            *pThreadData,
                                      ieieImportResourceControl_t *pControl,
                                      ieieDataType_t               dataType,
                                      uint64_t                     dataId,
                                      void                        *data,
                                      size_t                       dataLen);

//Structure shared between exportMultiConsumer.c and exportInflightMsgs.c
typedef struct tag_ieieExportInflightMultiConsumerQNode_t {
    uint64_t                        queueDataId;       ///< Queue this message belongs to
    uint64_t                        clientDataId;      ///< Which client is this in flight to
    uint64_t                        messageId;         ///< Msged id in exported file
    ismMessageState_t               msgState;          ///< State of message
    uint32_t                        deliveryId;        ///< Delivery Id if set
    uint8_t                         deliveryCount;     ///< Delivery counter
    uint8_t                         msgFlags;          ///< Message flags
    bool                            hasMDR;            ///< Has MDR in store
    bool                            inStore;           ///< Persisted in store
} ieieExportInflightMultiConsumerQNode_t;
#ifdef __cplusplus
}
#endif

#endif
