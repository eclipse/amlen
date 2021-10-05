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
/// @file  engineRestore.h
/// @brief Functions for restoring engine state from the store
//****************************************************************************
#ifndef __ISM_ENGINERESTORE_DEFINED
#define __ISM_ENGINERESTORE_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/**********************************************************************/
#include <engine.h>           /* Engine external header file         */
#include <engineCommon.h>     /* Engine common internal header file  */
#include <store.h>            /* Store external header file          */
#include <stdint.h>           /* Standard integer defns header file  */
#include <transaction.h>      /* Engine transaction header file      */

/// Function type used to rehydrate a record that has been persisted to the store
/// it is given a Pointer to a record, on output that record must be valid for
/// future reading from the store (but memory fragments allocated to it can have
/// been stolen by this function).
typedef int32_t (*ierr_RehydrateRecordFn_t)(
    ieutThreadData_t            *pThreadData,
    ismStore_Handle_t            recHandle,   ///< Handle to the record to be rehydrated
    ismStore_Record_t            *record,     ///< Rehydrated to be rehydrated
    ismEngine_RestartTransactionData_t  *transData, ///< Non-null if part of a transaction
    void                        **rehydratedRecord, ///< (Out) rehydrated version of the record
    void                         *pContext); ///< Function specific context info

///Type of function used to rehydrate a record requestd by another record
typedef int32_t (*ierr_PairCompletedFn_t)(
    ieutThreadData_t                   *pThreadData,
    ismStore_Handle_t                   recHandle,  ///<Handle to the (requested) record to be be rehydrated
    ismStore_Record_t                   *record,  ///<record to be rehydrated
    ismEngine_RestartTransactionData_t  *transData, ///< Non-null if record part of a transaction
    void                                *requestingRecord, ///< Rehydrated version of record that requested this one
    void                                **rehydratedRecord, ///< (out) rehydrated version of this record
    void                                *pContext); ///< Function specific context info


///Function type used to rehydrate a reference between two objects
///
///@remark Usually, when this function is called owner and child point to rehydrated objects
///In the special case of transactional operation references, the child/transData will not have been read
///in yet and is NULL
typedef int32_t (*ierr_RehydrateRefFn_t)(
    ieutThreadData_t             *pThreadData,
    void                         *owner,            ///< rehydrated owner record
    void                         *child,            ///< rehydrated child record (usually....see remarks for this function)
    ismStore_Handle_t            refHandle,         ///< handle of the reference to rehydrate
    ismStore_Reference_t         *reference,        ///< reference to rehydrate
    ismEngine_RestartTransactionData_t  *transData, ///< Non-NULL if reference is part of a transaction
    void                         *pContext);        ///< Function specific context data passed to it


///Type of function used to rehydrate a state record
typedef int32_t (*ierr_RehydrateStateFn_t)(
        ieutThreadData_t *pThreadData,
        void *rehydratedRecord,   ///<Rehydrated owning record
        ismStore_Handle_t stateHandle,
        ismStore_StateObject_t *pState,
        ismEngine_RestartTransactionData_t  *transData); ///<Non null if state record is part of a transaction


///@brief Main entry point for restart recovery logic
int32_t ierr_restartRecovery(ieutThreadData_t *pThreadData);

///@brief Entry point to complete final recovery actions required for starting messaging
int32_t ierr_startMessaging(ieutThreadData_t *pThreadData);

///@brief Sets message delivery ID from an MDR and updates MDR queue with queue handle
int32_t ierr_setMessageDeliveryIdFromMDR(
        ieutThreadData_t *pThreadData,
        iecsMessageDeliveryInfoHandle_t msgDelInfoHandle,
        ismStore_Handle_t ownerHandle,
        ismQHandle_t *queue,
        void **ppNode,
        ismStore_RecordType_t ownerType,
        ismStore_Handle_t messageHandle,
        uint32_t deliveryId);

///@brief Records that we encountered an error rehydrating a record from the store
void ierr_recordBadStoreRecord( ieutThreadData_t      *pThreadData
                              , ismStore_RecordType_t recType
                              , ismStore_Handle_t     recHandle
                              , char                  *identifier
                              , int32_t               rc);

#endif /* __ISM_ENGINESTORE_DEFINED */

/*********************************************************************/
/* End of engineRestore.h                                              */
/*********************************************************************/
