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
/// @file  engineQueue.c
/// @brief Main module for Engine Queues
//*********************************************************************
#define TRACE_COMP Engine

#include <stdio.h>
#include <assert.h>

#include "engineInternal.h"
#include "transaction.h"
#include "queueCommon.h"
#include "simpQ.h"
#include "intermediateQ.h"
#include "multiConsumerQ.h"
#include "engineStore.h"
#include "engineRestore.h"
#include "queueNamespace.h"   // ieqn functions & constants
#include "exportQMessages.h"

const ieqInterface_t QInterface[] = 
{
    {   /* zero - not used */
        NULL                            /* create */
      , NULL                            /* deleteQ */
      , NULL                            /* drainQ */
      , NULL                            /* dumpQ */
      , NULL                            /* getConsumableMsgCount */
      , NULL                            /* getStats */
      , NULL                            /* setStats */
      , NULL                            /* put */
      , NULL                            /* rehydrate */
      , NULL                            /* rehydrateDeliveryId */
      , NULL                            /* initWaiter */
      , NULL                            /* termWaiter */
      , NULL                            /* checkWaiters */
      , NULL                            /* acknowledge */
      , NULL                            /* relinquish */
      , NULL                            /* prepareAck */
      , NULL                            /* processAck */
      , NULL                            /* completeAckBatch */
      , NULL                            /* getDefnHdl */
      , NULL                            /* getPropsHdl */
      , NULL                            /* setPropsHdl */
      , NULL                            /* completeRehydrate */
      , NULL                            /* markQDeleted */
      , NULL                            /* isDeleted */
      , NULL                            /* markMsgTranGot */
      , NULL                            /* reduceCount */
      , NULL                            /* forgetInflightMsgs */
      , NULL                            /* redeliverOnly */
      , NULL                            /* reclaimSpace */
      , NULL                            /* relinquishAll */
      , NULL                            /* removeIfUnneeded */
      , NULL                            /* reapExpiredMsgs */
      , NULL                            /* messageExpired */
      , NULL                            /* exportMessages */
      , NULL                            /* completeImport */
      , NULL                            /* consumerStats */
    }, 
    {   /* multi-consumer */
        iemq_createQ                    /* create */
      , iemq_deleteQ                    /* deleteQ */
      , iemq_drainQ                     /* drainQ */
      , iemq_dumpQ                      /* dumpQ */
      , iemq_getStats                   /* getStats */
      , iemq_setStats                   /* setStats */
      , iemq_checkAvailableMsgs         /* checkAvailableMsgs */
      , iemq_putMessage                 /* put */
      , iemq_rehydrateMsg               /* rehydrateMsg */
      , iemq_rehydrateDeliveryId        /* rehydrateDeliveryId */
      , iemq_initWaiter                 /* initWaiter */
      , iemq_termWaiter                 /* termWaiter */
      , iemq_checkWaiters               /* checkWaiters */
      , iemq_acknowledge                /* acknowledge */
      , iemq_relinquish                 /* relinquish */
      , iemq_prepareAck                 /* prepareAck */
      , iemq_processAck                 /* processAck */
      , iemq_completeAckBatch           /* completeAckBatch */
      , iemq_getDefnHdl                 /* getDefnHdl */
      , iemq_getPropsHdl                /* getPropsHdl */
      , iemq_setPropsHdl                /* setPropsHdl */
      , iemq_updateProperties           /* updateProperties */
      , iemq_completeRehydrate          /* completeRehydrate */
      , iemq_markQDeleted               /* markQDeleted */
      , iemq_isDeleted                  /* isDeleted */
      , iemq_markMessageGotInTran       /* markMsgTranGot */
      , iemq_reducePreDeleteCount       /* reduce count */
      , iemq_forgetInflightMsgs         /* forgetInflightMsgs */
      , iemq_redeliverEssentialOnly     /* redeliverOnly */
      , NULL                            /* reclaimSpace - called if pending flag on waiter */
                                        /*   and locking different in this queue so unused */
      , iemq_relinquishAllMsgsForClient /* relinquishAll */
      , iemq_removeIfUnneeded           /* removeIfUnneeded */
      , iemq_reapExpiredMsgs            /* reapExpiredMsgs */
      , iemq_messageExpired             /* messageExpired */
      , ieie_exportMultiConsumerQMessages  /* exportMessages */
      , iemq_completeImport             /* completeImport */
      , iemq_getConsumerStats           /* consumerStats */
    }, 
    {   /* simple */
        iesq_createQ                    /* create */
      , iesq_deleteQ                    /* deleteQ */
      , iesq_drainQ                     /* drainQ */
      , iesq_dumpQ                      /* dumpQ */
      , iesq_getStats                   /* getStats */
      , iesq_setStats                   /* setStats */
      , iesq_checkAvailableMsgs         /* checkAvailableMsgs */
      , iesq_putMessage                 /* put */
      , NULL                            /* rehydrate */
      , NULL                            /* rehydrateDeliveryId */
      , iesq_initWaiter                 /* initWaiter */
      , iesq_termWaiter                 /* termWaiter */
      , iesq_checkWaiters               /* checkWaiters */
      , NULL                            /* acknowledge */
      , NULL                            /* relinquish */
      , NULL                            /* prepareAck */
      , NULL                            /* processAck */
      , NULL                            /* completeAckBatch */
      , iesq_getDefnHdl                 /* getDefnHdl */
      , iesq_getPropsHdl                /* getPropsHdl */
      , iesq_setPropsHdl                /* setPropsHdl */
      , iesq_updateProperties           /* updateProperties */
      , iesq_completeRehydrate          /* completeRehydrate */
      , iesq_markQDeleted               /* markQDeleted */
      , iesq_isDeleted                  /* isDeleted */
      , NULL                            /* markMsgTranGot */
      , iesq_reducePreDeleteCount       /* reduce count */
      , iesq_forgetInflightMsgs         /* forgetInflightMsgs */
      , iesq_redeliverEssentialOnly     /* redeliverOnly */
      , iesq_reclaimSpace               /* reclaimSpace */
      , NULL                            /* relinquishAll */
      , iesq_removeIfUnneeded           /* removeIfUnneeded */
      , iesq_reapExpiredMsgs            /* reapExpiredMsgs */
      , iesq_messageExpired             /* messageExpired */
      , ieie_exportSimpQMessages        /* exportMessages */
      , iesq_completeImport             /* completeImport */
      , iesq_getConsumerStats           /* consumerStats */
    }, 
    {   /* intermediate */
        ieiq_createQ                    /* create */
      , ieiq_deleteQ                    /* deleteQ */
      , ieiq_drainQ                     /* drainQ */
      , ieiq_dumpQ                      /* dumpQ */
      , ieiq_getStats                   /* getStats */
      , ieiq_setStats                   /* setStats */
      , ieiq_checkAvailableMsgs         /* checkAvailableMsgs */
      , ieiq_putMessage                 /* put */
      , ieiq_rehydrateMsg               /* rehydrateMsg */
      , ieiq_rehydrateDeliveryId        /* rehydrateDeliveryId */
      , ieiq_initWaiter                 /* initWaiter */
      , ieiq_termWaiter                 /* termWaiter */
      , ieiq_checkWaiters               /* checkWaiters */
      , ieiq_acknowledge                /* acknowledge */
      , NULL                            /* relinquish */
      , NULL                            /* prepareAck */
      , NULL                            /* processAck */
      , NULL                            /* completeAckBatch */
      , ieiq_getDefnHdl                 /* getDefnHdl */
      , ieiq_getPropsHdl                /* getPropsHdl */
      , ieiq_setPropsHdl                /* setPropsHdl */
      , ieiq_updateProperties           /* updateProperties */
      , ieiq_completeRehydrate          /* completeRehydrate */
      , ieiq_markQDeleted               /* markQDeleted */
      , ieiq_isDeleted                  /* isDeleted */
      , NULL                            /* markMsgTranGot */
      , ieiq_reducePreDeleteCount       /* reduce count */
      , ieiq_forgetInflightMsgs         /* forgetInflightMsgs */
      , ieiq_redeliverEssentialOnly     /* redeliverOnly */
      , ieiq_reclaimSpace               /* reclaimSpace */
      , NULL                            /* relinquishAll */
      , ieiq_removeIfUnneeded           /* removeIfUnneeded */
      , ieiq_reapExpiredMsgs            /* reapExpiredMsgs */
      , ieiq_messageExpired             /* messageExpired */
      , ieie_exportInterQMessages       /* exportMessages */
      , ieiq_completeImport             /* completeImport */
      , ieiq_getConsumerStats           /* consumerStats */
    }
};

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Rehydrates an intermediate/multiConsumer Q
///  @remarks
///    This function is used to recreate an intermediate or multiConsumerQ.
///
///  @param[in]  recHandle         - Store record handle     
///  @param[in]  record            - Store record
///  @param[in]  transData         - info about transaction (or NULL)
///  @param[out] outData           - Associated object
///  @param[in]  pContext          - Context 
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ieq_rehydrateQ(ieutThreadData_t *pThreadData,
                       ismStore_Handle_t recHandle,
                       ismStore_Record_t *record,
                       ismEngine_RestartTransactionData_t *transData,
                       void **outData,
                       void *pContext)
{
    int32_t rc;
    ismQHandle_t hQueue;

    // Verify this is a queue record
    assert(record->Type == ISM_STORE_RECTYPE_QUEUE);

    // Currently a queue definition contains only a single frag which
    // contains the QDR
    assert(record->FragsCount == 1);

    iestQueueDefinitionRecord_t *pQDR = (iestQueueDefinitionRecord_t *)(record->pFrags[0]);
    ismEngine_CheckStructId(pQDR->StrucId, iestQUEUE_DEFN_RECORD_STRUCID, ieutPROBE_001);

    ieutTRACEL(pThreadData, pQDR, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    ismQueueType_t queueType;

    if (LIKELY(pQDR->Version == iestQDR_CURRENT_VERSION))
    {
        queueType = pQDR->Type;
    }
    else
    {
        rc = ISMRC_InvalidValue;
        ism_common_setErrorData(rc, "%u", pQDR->Version);
        goto mod_exit;
    }

    assert(queueType != simple); // Should be intermediate or multiConsumer

    ieutTRACEL(pThreadData, queueType, ENGINE_HIFREQ_FNC_TRACE, "Found QDR for %d queue.\n", queueType);
    rc = ieq_createQ( pThreadData
                    , queueType
                    , NULL
                    , ieqOptions_DEFAULT | ieqOptions_IN_RECOVERY
                    , iepi_getDefaultPolicyInfo(true)
                    , recHandle
                    , ismSTORE_NULL_HANDLE
                    , iereNO_RESOURCE_SET
                    , &hQueue );

    *outData = hQueue;

    if (record->State & iestQDR_STATE_DELETED)
    {
       ieutTRACEL(pThreadData, hQueue, ENGINE_FNC_TRACE, "Deleted QDR found for queue %p\n", hQueue);

       assert(queueType == multiConsumer);

       rc = iemq_markQDeleted(pThreadData, hQueue, false);
    }

mod_exit:

    if (rc != OK)
    {
        ierr_recordBadStoreRecord( pThreadData
                                 , record->Type
                                 , recHandle
                                 , NULL
                                 , rc);
    }


    ieutTRACEL(pThreadData, rc, ENGINE_FNC_TRACE, FUNCTION_EXIT "\n", __func__);
    return rc;
}

//****************************************************************************
/// @brief  Rehydrate a queue properties record read from the store
///
/// Recreates a queue from the store.
///
/// @param[in]     recHandle         Store handle of the QPR
/// @param[in]     record            Pointer to the QPR
/// @param[in]     transData         info about transaction (or NULL)
/// @param[in]     requestingRecord  The QDR (queue) that requested this QPR
/// @param[out]    rehydratedRecord  Pointer to data created from this record if
///                                  the recovery code needs to track it (unused)
/// @param[in]     pContext          Context (unused)
///
/// @return OK on successful completion or an ISMRC_ value.
///
/// @remark if called with NULL requestingRecord, creates a simple queue otherwise
///         sets the properties for the queue specified.
///
/// @remark The record must be consistent upon return.
//****************************************************************************
int32_t ieq_rehydrateQueueProps(ieutThreadData_t *pThreadData,
                                ismStore_Handle_t recHandle,
                                ismStore_Record_t *record,
                                ismEngine_RestartTransactionData_t *transData,
                                void *requestingRecord,
                                void **rehydratedRecord,
                                void *pContext)
{
    int32_t rc = OK;
    iepiPolicyInfo_t *pPolicyInfo = NULL;

    // Verify this is a queue properties record
    assert(record->Type == ISM_STORE_RECTYPE_QPROP);
    assert(record->FragsCount == 1);

    ismQHandle_t hQueue = (ismQHandle_t)requestingRecord;

    assert(hQueue != NULL);

    iestQueuePropertiesRecord_t *pQPR = (iestQueuePropertiesRecord_t *)(record->pFrags[0]);
    ismEngine_CheckStructId(pQPR->StrucId, iestQUEUE_PROPS_RECORD_STRUCID, ieutPROBE_001);

    ieutTRACEL(pThreadData, pQPR, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    uint32_t queueNameLength;
    char *tmpPtr;

    if (pQPR->Version == iestQPR_CURRENT_VERSION)
    {
        queueNameLength = pQPR->QueueNameLength;

        tmpPtr = (char *)(pQPR+1);
    }
    else
    {
        rc = ISMRC_InvalidValue;
        ism_common_setErrorData(rc, "%u", pQPR->Version);
        goto mod_exit;
    }

    /*******************************************************************/
    /* Pull the constituent parts of the queue from the following data */
    /*******************************************************************/
    char   *queueName;
    if (queueNameLength != 0)
    {
        queueName = tmpPtr;
    }
    else
    {
        queueName = NULL;
    }
    tmpPtr += queueNameLength;

    ieutTRACEL(pThreadData, hQueue, ENGINE_HIFREQ_FNC_TRACE, "Rehydrating queueName '%s' (queue=%p).\n",
               queueName ? queueName : "", hQueue);

    // Reconciliation will configure the proper policy info, for now we use the default
    pPolicyInfo = iepi_getDefaultPolicyInfo(true);

    assert(pPolicyInfo != NULL);

    DEBUG_ONLY bool changedProps = ieq_setPolicyInfo(pThreadData, hQueue, pPolicyInfo);
    assert(changedProps == false);

    ieq_updateProperties(pThreadData,
                         hQueue,
                         queueName,
                         ieqOptions_DEFAULT | ieqOptions_IN_RECOVERY,
                         recHandle,
                         iereNO_RESOURCE_SET);

    // For a named queue, add to the hash of named queues.
    if (queueName != NULL)
    {
        // If the queue is deleted, don't add it to the namespace as it will
        // be deleted at the end of restart or whenever it is no longer in use
        if (ieq_isDeleted(hQueue))
        {
            ieutTRACEL(pThreadData, hQueue, ENGINE_FNC_TRACE, "QPR found for deleted QDR (queue %p), not adding to namespace\n",
                       hQueue);
        }
        else
        {
            rc = ieqn_addQueue(pThreadData,
                               queueName,
                               hQueue,
                               NULL);

            if (rc != OK) goto mod_exit;
        }
    }

mod_exit:

    if (rc != OK)
    {
        ierr_recordBadStoreRecord( pThreadData
                                 , record->Type
                                 , recHandle
                                 , NULL
                                 , rc);

        // If we decide to do partial store recovery, remove the problematic queue.
        // To do that, we also need to make sure that the queue knows what the QPR record handle is.
        ieq_setPropsHdl(hQueue, recHandle);
        ieq_markQDeleted(pThreadData, hQueue, false);
    }

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}


///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Rehydrates a queue-message reference 
///  @remarks
///    This function is used to recreate the link from a queue to a message
///    during restart
///
///  @param[in]  owner             - Pointer to Queue structure
///  @param[in]  child             - Pointer to Message structure
///  @param[in]  refHandle         - Message Reference Handle
///  @param[in]  transData         - info about transaction (or NULL)
///  @return                       - OK on success or an ISMRC error code
///////////////////////////////////////////////////////////////////////////////
int32_t ieq_rehydrateQueueMsgRef(ieutThreadData_t *pThreadData,
                                 void *owner,
                                 void *child,
                                 ismStore_Handle_t refHandle,
                                 ismStore_Reference_t *reference,
                                 ismEngine_RestartTransactionData_t *transData,
                                 void *pContext)
{
    int32_t rc;
    ismEngine_Queue_t *pQueue = (ismEngine_Queue_t *)owner;
    ismEngine_Message_t *pMsg = (ismEngine_Message_t *)child;

    ieutTRACEL(pThreadData, pQueue, ENGINE_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    assert(pQueue != NULL);
    assert(pMsg != NULL);

    // Verify this is a queue record
    ismEngine_CheckStructId(pQueue->StrucId, ismENGINE_QUEUE_STRUCID, ieutPROBE_001);

    // And verify we have been given a message reference
    ismEngine_CheckStructId(pMsg->StrucId, ismENGINE_MESSAGE_STRUCID, ieutPROBE_002);

    rc = ieq_rehydrateMsg( pThreadData
                         , pQueue
                         , pMsg
                         , transData
                         , refHandle
                         , reference );

    ieutTRACEL(pThreadData, rc,  ENGINE_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    Set the policy info for a queue
///  @remarks
///    The reference count (useCount) of the policy info being set is inherited
///    by the queue.
///
///  @param[in]  pThreadData       - Thread data
///  @param[in]  pQueue            - The queue being updated
///  @param[in]  pPolicyInfo       - The new policy info to use
///
///  @return  boolean indicating whether the policy actually changed
///////////////////////////////////////////////////////////////////////////////
bool ieq_setPolicyInfo(ieutThreadData_t *pThreadData,
                       ismEngine_Queue_t *pQueue,
                       iepiPolicyInfo_t *pPolicyInfo)
{
    bool changed;

    assert(pQueue != NULL);
    assert(pPolicyInfo != NULL);

    iepiPolicyInfoHandle_t oldPolicy = pQueue->PolicyInfo;

    if (pQueue->PolicyInfo != pPolicyInfo)
    {
        ieutTRACEL(pThreadData, pPolicyInfo, ENGINE_FNC_TRACE, FUNCTION_IDENT "from=%p, to=%p\n",
                   __func__, oldPolicy, pPolicyInfo);

        pQueue->PolicyInfo = pPolicyInfo;

        if (oldPolicy != NULL)
        {
            iepi_releasePolicyInfo(pThreadData, oldPolicy);
        }

        changed = true;
    }
    else
    {
        changed = false;
    }

    return changed;
}

//Remove any qnodes we rehydrated but which came back consumed...
int32_t ieq_removeRehydratedConsumedNodes(ieutThreadData_t *pThreadData)
{
    //At the moment the only queue type that has nodes on restart that
    //need to be deleted is multiconsumerQ.c....
    int32_t rc = iemq_removeRehydratedConsumedNodes(pThreadData);

    return rc;
}

static int ieq_scheduleCheckWaitersCB(ism_timer_t key, ism_time_t timestamp, void * userdata)
{
    ismEngine_Queue_t *Q= (ismEngine_Queue_t *)userdata;

    // Make sure we're thread-initialised. This can be issued repeatedly.
    ism_engine_threadInit(0);

    ieutThreadData_t *pThreadData = ieut_enteringEngine(NULL);

    ieutTRACEL(pThreadData, Q, ENGINE_CEI_TRACE, FUNCTION_IDENT "Q=%p\n"
                       , __func__, Q);

    ieq_checkWaiters(pThreadData, Q, NULL);

    //ReduceCount that CALLER of ieq_scheduleCheckWaiters increased
    ieq_reducePreDeleteCount(pThreadData, Q);

    ieut_leavingEngine(pThreadData);
    ism_common_cancelTimer(key);

    //We did engine threadInit...  term will happen when the engine terms and after the counter
    //of events reaches 0
    __sync_fetch_and_sub(&(ismEngine_serverGlobal.TimerEventsRequested), 1);
    return 0;
}

//NB: *MUST have increased the predeletecount before calling this function
void ieq_scheduleCheckWaiters(ieutThreadData_t *pThreadData,
                              ismEngine_Queue_t *Q)
{
    ieutTRACEL(pThreadData, Q, ENGINE_CEI_TRACE, FUNCTION_IDENT "Q=%p\n"
                       , __func__, Q);

    __sync_fetch_and_add(&(ismEngine_serverGlobal.TimerEventsRequested), 1);

   (void)ism_common_setTimerOnce(ISM_TIMER_LOW,
                                 ieq_scheduleCheckWaitersCB,
                                 Q, 20);
}
