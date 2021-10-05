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

//****************************************************************************
/// @file  exportRetained.h
/// @brief Functions to export subscription information
//****************************************************************************
#ifndef __ISM_EXPORTRETAINED_DEFINED
#define __ISM_EXPORTRETAINED_DEFINED

#include "engineInternal.h"
#include "exportResources.h"
#include "exportCrypto.h"

//****************************************************************************
/// @brief Information exported about a retained message
///
/// @remark The dataId of this record is the dataId of the message record that
/// is being retained.
///
/// @remark The message itself has an embedded topic string, so the only
/// information included here is the Id of the message.
//****************************************************************************
typedef struct tag_ieieRetainedMsgInfo_t
{
    uint32_t                    Version;                       ///< The version of retained import/export information
} ieieRetainedMsgInfo_t;

#define ieieRETAINEDMSG_VERSION_1          1
#define ieieRETAINEDMSG_CURRENT_VERSION    ieieRETAINEDMSG_VERSION_1

//****************************************************************************
/// @internal
///
/// @brief  Export the information for retained messages whose topic string
///         matches topic string specified in the control client.
///
/// @param[in]     control        ieieExportResourceControl_t for this export
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_exportRetainedMsgs(ieutThreadData_t *pThreadData,
                                ieieExportResourceControl_t *control);

//****************************************************************************
/// @internal
///
/// @brief  Import (publish) a retained message.
///
/// @param[in]     control        ieieImportResourceControl_t for this import
/// @param[in]     dataId         dataId (for retained msgs, the dataId of the
///                               message itself)
/// @param[in]     data           ieieRetainedMsgInfo_t
/// @param[in]     dataLen        sizeof(ieieRetainedMsgInfo_t)
///
/// @return OK on successful completion or an ISMRC_ value if there is a problem.
//****************************************************************************
int32_t ieie_importRetainedMsg(ieutThreadData_t *pThreadData,
                               ieieImportResourceControl_t *control,
                               uint64_t dataId,
                               uint8_t *data,
                               size_t dataLen);

#endif
