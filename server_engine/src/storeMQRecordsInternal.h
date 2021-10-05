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
/// @file  storeMQRecordsInternal.h
/// @brief Internal Engine header for functions to store MQ Records
//****************************************************************************
#ifndef __ISM_STOREMQRECORDSINTERNAL_DEFINED
#define __ISM_STOREMQRECORDSINTERNAL_DEFINED

/*********************************************************************/
/*                                                                   */
/* INCLUDES                                                          */
/*                                                                   */
/*********************************************************************/
#include "engineCommon.h"     /* Engine common internal header file  */
#include "engineInternal.h"   /* Engine internal header file         */

/*********************************************************************/
/*                                                                   */
/* TYPE DEFINITIONS                                                  */
/*                                                                   */
/*********************************************************************/

//*********************************************************************
/// @brief Queue-manager record
///
/// Contains information for the MQ Bridge about a WebSphere MQ queue manager.
//*********************************************************************
typedef struct iesmQManagerRecord_t
{
    char                            StrucId[4];                ///< Eyecatcher "EMBQ"
    uint32_t                        UseCount;                  ///< Number of uses of this record
    struct iesmQManagerRecord_t *   pPrev;                     ///< Ptr to previous record
    struct iesmQManagerRecord_t *   pNext;                     ///< Ptr to next record
    struct iesmQMgrXidRecord_t *    pXidHead;                  ///< Ptr to head of XID record list
    struct iesmQMgrXidRecord_t *    pXidTail;                  ///< Ptr to tail of XID record list
    ismStore_Handle_t               hStoreBMR;                 ///< Store handle of Bridge Queue Manager Record (BMR)
    void *                          pData;                     ///< Ptr to record data
    size_t                          DataLength;                ///< Length of record data
} iesmQManagerRecord_t;

#define iesmQ_MANAGER_RECORD_STRUCID "EMBQ"


//*********************************************************************
/// @brief Queue-manager XID record
///
/// Contains information for the MQ Bridge about a transaction with
/// a WebSphere MQ queue manager.
//*********************************************************************
typedef struct iesmQMgrXidRecord_t
{
    char                            StrucId[4];                ///< Eyecatcher "EMBX"
    bool                            fUncommitted;              ///< Whether this record is uncommitted
    iesmQManagerRecord_t *          pQMgrRec;                  ///< Ptr to associated queue manager record
    struct iesmQMgrXidRecord_t *    pPrev;                     ///< Ptr to previous record
    struct iesmQMgrXidRecord_t *    pNext;                     ///< Ptr to next record
    void *                          pData;                     ///< Ptr to record data
    size_t                          DataLength;                ///< Length of record data
} iesmQMgrXidRecord_t;

#define iesmQ_MGR_XID_RECORD_STRUCID "EMBX"


//*********************************************************************
/// @brief  Softlog entry for Add Queue-Manager XID
/// @remark
///    This structure contains the softlog entry for an Add Queue-Manager
///    XID request.
//*********************************************************************
typedef struct iesmSLEAddQMgrXID_t
{
    ietrStoreTranRef_t              TranRef;
    iesmQMgrXidRecord_t *           pQMgrXidRec;
} iesmSLEAddQMgrXID_t;

//*********************************************************************
/// @brief  Softlog entry for Remove Queue-Manager XID
/// @remark
///    This structure contains the softlog entry for an Remove Queue-Manager
///    XID request.
//*********************************************************************
typedef struct iesmSLERemoveQMgrXID_t
{
    ietrStoreTranRef_t              TranRef;
    iesmQMgrXidRecord_t *           pQMgrXidRec;
} iesmSLERemoveQMgrXID_t;


/*********************************************************************/
/*                                                                   */
/* FUNCTION PROTOTYPES                                               */
/*                                                                   */
/*********************************************************************/

/*
 * Locks the MQ store state held by the Engine
 */
void iesm_lockMQStoreState(void);

/*
 * Unlocks the MQ store state held by the Engine
 */
void iesm_unlockMQStoreState(void);

/*
 * Allocate a new queue manager record
 */
int32_t iesm_newQManagerRecord(ieutThreadData_t *pThreadData,
                               void *pData,
                               size_t dataLength,
                               iesmQManagerRecord_t **ppQMgrRec);

/*
 * Frees a queue manager record
 */
void iesm_freeQManagerRecord(ieutThreadData_t *pThreadData,
                             iesmQManagerRecord_t *pQMgrRec);

/*
 * Adds a queue-manager record to the list
 */
void iesm_addQManagerRecord(iesmQManagerRecord_t *pQMgrRec);

/*
 * Removes a queue-manager record from the list
 */
void iesm_removeQManagerRecord(iesmQManagerRecord_t *pQMgrRec);

/*
 * Add a Bridge Queue-Manager Record to the Store
 */
int32_t iesm_storeBridgeQMgrRecord(ieutThreadData_t *pThreadData,
                                   iesmQManagerRecord_t *pQMgrRec);

/*
 * Remove a Bridge Queue-Manager Record from the Store
 */
int32_t iesm_unstoreBridgeQMgrRecord(ieutThreadData_t *pThreadData,
                                     iesmQManagerRecord_t *pQMgrRec);

/*
 * Allocate a new queue-manager XID record
 */
int32_t iesm_newQMgrXidRecord(ieutThreadData_t *pThreadData,
                              void *pData,
                              size_t dataLength,
                              iesmQMgrXidRecord_t **ppQMgrXidRec);

/*
 * Frees a queue-manager XID record
 */
void iesm_freeQMgrXidRecord(ieutThreadData_t *pThreadData,
                            iesmQMgrXidRecord_t *pQMgrXidRec);

/*
 * Adds a queue-manager XID record to the list
 */
void iesm_addQMgrXidRecord(iesmQManagerRecord_t *pQMgrRec, iesmQMgrXidRecord_t *pQMgrXidRec);

/*
 * Removes a queue-manager XID record from the list
 */
void iesm_removeQMgrXidRecord(iesmQMgrXidRecord_t *pQMgrXidRec);

/*
 * Add Bridge XID Record to the Store and optionally a transaction
 *
 * NOTE: BXRs are no longer written into the Store, but we support
 *       transactional updates to them in a volatile way.
 */
int32_t iesm_storeBridgeXidRecord(ieutThreadData_t *pThreadData,
                                  iesmQMgrXidRecord_t *pQMgrXidRec,
                                  ismEngine_Transaction_t *pTran);

/*
 * Remove Bridge XID Record from the Store and optionally a transaction
 *
 * NOTE: BXRs are no longer written into the Store, but we support
 *       transactional updates to them in a volatile way.
 */
int32_t iesm_unstoreBridgeXidRecord(ieutThreadData_t *pThreadData,
                                    iesmQMgrXidRecord_t *pQMgrXidRec,
                                    ismEngine_Transaction_t *pTran);

#endif /* __ISM_STOREMQRECORDSINTERNAL_DEFINED */

/*********************************************************************/
/* End of storeMQRecordsInternal.h                                   */
/*********************************************************************/
