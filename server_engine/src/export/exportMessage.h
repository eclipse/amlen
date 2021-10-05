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
/// @file  exportMessage.h
/// @brief Functions to export and import Message information
//****************************************************************************
#ifndef __ISM_EXPORTMESSAGE_DEFINED
#define __ISM_EXPORTMESSAGE_DEFINED

#include "engineInternal.h"

int32_t ieie_exportMessage(ieutThreadData_t *pThreadData,
                           ismEngine_Message_t *message,
                           ieieExportResourceControl_t *control,
                           bool usagePreIncremented);

int32_t ieie_importMessage(ieutThreadData_t *pThreadData,
                           ieieImportResourceControl_t *control,
                           uint64_t dataId,
                           uint8_t *data,
                           size_t dataLen);

int32_t ieie_findImportedMessage(ieutThreadData_t *pThreadData,
                                 ieieImportResourceControl_t *control,
                                 uint64_t dataId,
                                 ismEngine_Message_t **ppMessage);
#endif
