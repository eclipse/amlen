/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

/*********************************************************************/
/*                                                                   */
/* Module Name: storeHAadmin.c                                       */
/*                                                                   */
/* Description: High Availability admin interface                    */
/*                                                                   */
/*********************************************************************/

#define TRACE_COMP Store

#include <adminHA.h>
#include <ha.h>


/* Admin instructed to transfer state */
XAPI int ism_ha_admin_transfer_state(const char *generatedGroup) {
  int rc = ISMRC_OK;

    if ( adminHAInfo == NULL ) {
      TRACE(1, "HA Admin is not initialized yet.\n");
        return ISMRC_Error;
    }

    TRACE(5, "ism_ha_admin_transfer_state - send signal to AdminHA thread.\n");

    /* Queue transfer state action */
    ismHA_AdminMessage_t *pAdminMsg = (ismHA_AdminMessage_t *)alloca(sizeof(ismHA_AdminMessage_t));
    pAdminMsg->pData = generatedGroup;
    rc = ism_admin_ha_queueAdminAction(ISM_HA_ADMIN_NOTIFY_TRANSFER_STATE, 0, pAdminMsg);

    /* Signal AdminHA thread */
    pthread_mutex_lock(&adminHAInfo->threadMutex);
    pthread_cond_signal(&adminHAInfo->threadCond);
    pthread_mutex_unlock(&adminHAInfo->threadMutex);

    return rc;
}

/* View change notification to Admin */
XAPI int ism_ha_admin_viewChanged(ismHA_View_t *pView) {
  int rc = ISMRC_OK;

    if ( adminHAInfo == NULL )
        return ISMRC_Error;

    TRACE(5, "ism_ha_admin_viewChanged - send signal to AdminHA thread.\n");

    /* Queue action based on view data */
    if ( pView->OldRole == ISM_HA_ROLE_STANDBY && pView->NewRole == ISM_HA_ROLE_PRIMARY ) {
        /* Standby becomes Primary (OldRole=STANDBY NewRole=PRIMARY)
         * Action: Perform Standby to Primary procedure
         */
      rc = ism_admin_ha_queueAdminAction(ISM_HA_ADMIN_NOTIFY_VIEW_CHANGE, ISM_HA_ACTION_STANDBY_TO_PRIMARY, NULL);
    } else if ( pView->NewRole == ISM_HA_ROLE_ERROR ) {
      if ( pView->ReasonCode == ISM_HA_REASON_SPLIT_BRAIN_RESTART ) {
        /* Node is an inferior Primary in a SplitBrain situation (NewRole=ERROR)
         * Action: Restart the server (so that it will serve as SB)
         */
        rc = ism_admin_ha_queueAdminAction(ISM_HA_ADMIN_NOTIFY_VIEW_CHANGE, ISM_HA_ACTION_SPLITBRAIN_RESTART, NULL);
      } else {
        /* Node enters ERROR state (NewRole=ERROR)
         * Action: Terminate the Store (Store can no longer function)
         */
        rc = ism_admin_ha_queueAdminAction(ISM_HA_ADMIN_NOTIFY_VIEW_CHANGE, ISM_HA_ACTION_TERMINATE_STORE, NULL);
      }
    } else if ( pView->OldRole == ISM_HA_ROLE_PRIMARY && pView->NewRole == ISM_HA_ROLE_PRIMARY ) {
      if ( pView->ActiveNodesCount == 2 && pView->SyncNodesCount == 2 ) {
          /* Primary detects a new sync Standby (OldRole=NewRole=PRIMARY
             * ActiveNodesCount=2 SyncNodesCount=2)
             * Action: Start performing Admin replicating to the Standby
             */
          rc = ism_admin_ha_queueAdminAction(ISM_HA_ADMIN_NOTIFY_VIEW_CHANGE, ISM_HA_ACTION_STANDBY_SYNC_START, NULL);
      } else if ( pView->ActiveNodesCount == 1 && pView->SyncNodesCount == 1 ) {
        /* Primary detects Standby failed (oldRole=newRole=PRIMARY
           * ActiveNodesCount=1 SyncNodesCount=1)
           * Action: Stop performing Admin replicating to the Standby
           */
          rc = ism_admin_ha_queueAdminAction(ISM_HA_ADMIN_NOTIFY_VIEW_CHANGE, ISM_HA_ACTION_STANDBY_SYNC_STOP, NULL);
      }
    } else if ( pView->OldRole == ISM_HA_ROLE_STANDBY && pView->NewRole == ISM_HA_ROLE_TERM ) {
      /* Standby terminated by Primary (oldRole=STANDBY newRole=TERM)
       * Action: Terminate the ISM server (including the Store)
       */
      rc = ism_admin_ha_queueAdminAction(ISM_HA_ADMIN_NOTIFY_VIEW_CHANGE, ISM_HA_ACTION_TERMINATE_STORE, NULL);
    }

    /* Signal Admin HA thread */
    pthread_mutex_lock(&adminHAInfo->threadMutex);
    pthread_cond_signal(&adminHAInfo->threadCond);
    pthread_mutex_unlock(&adminHAInfo->threadMutex);

    return rc;
}

/* Process Admin message sent from Primary to Standby */
XAPI void ism_ha_admin_process_admin_msg(ismHA_AdminMessage_t *pAdminMsg) {
    if ( adminHAInfo == NULL ) {
      TRACE(1, "HA Admin is not initialized yet.\n");
        return;
    }

    TRACE(5, "ism_ha_admin_process_admin_msg: dataLength=%u.\n", pAdminMsg->DataLength);

    /* queue admin action */
    ism_admin_ha_queueAdminAction(ISM_HA_ADMIN_NOTIFY_ADMIN_MESSAGE, 0, pAdminMsg);

    /* Signal AdminHA thread */
    pthread_mutex_lock(&adminHAInfo->threadMutex);
    pthread_cond_signal(&adminHAInfo->threadCond);
    pthread_mutex_unlock(&adminHAInfo->threadMutex);
    return;
}

