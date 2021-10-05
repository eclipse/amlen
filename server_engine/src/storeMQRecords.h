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
/// @file  storeMQRecords.h
/// @brief Engine header for functions to store MQ Records
//****************************************************************************
#ifndef __ISM_STOREMQRECORDS_DEFINED
#define __ISM_STOREMQRECORDS_DEFINED

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

/*
 * Rehydrates a Bridge Queue-Manager Record from the Store during recovery
 */
int32_t iesm_rehydrateBridgeQMgrRecord(ieutThreadData_t *pThreadData,
                                       ismStore_Record_t *pStoreRecord,
                                       ismStore_Handle_t hStoreRecord,
                                       void **rehydratedRecord);

#endif /* __ISM_STOREMQRECORDS_DEFINED */

/*********************************************************************/
/* End of storeMQRecords.h                                           */
/*********************************************************************/
