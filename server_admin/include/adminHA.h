/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

#ifndef __ADMIN_HA_DEFINED
#define __ADMIN_HA_DEFINED


/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#include <ismrc.h>
#include <ismutil.h>
#include <ha.h>
#include <store.h>

/* Notification types from HA to Admin */
typedef enum
{
    ISM_HA_ADMIN_NOTIFY_RESET          = 0, /* Reset notify flag                   */
    ISM_HA_ADMIN_NOTIFY_VIEW_CHANGE    = 1, /* View change notification            */
    ISM_HA_ADMIN_NOTIFY_TRANSFER_STATE = 2, /* Transfer state notification         */
    ISM_HA_ADMIN_NOTIFY_ADMIN_MESSAGE  = 3, /* Received admin message notification */
} ismHA_Admin_Notify_t;


/* Admin action on HA view change */
typedef enum
{
    ISM_HA_ACTION_UNKNOWN             = 0,  /* Unknown or no                            */
    ISM_HA_ACTION_STANDBY_TO_PRIMARY  = 1,  /* Change node role from standby to primary */
    ISM_HA_ACTION_TERMINATE_STORE     = 2,  /* Error condition - terminate store        */
    ISM_HA_ACTION_STANDBY_SYNC_START  = 3,  /* Start configuration sync on standby node */
    ISM_HA_ACTION_STANDBY_SYNC_STOP   = 4,  /* Stop configuration sync on standby node  */
    ISM_HA_ACTION_TERMINATE_STANDBY   = 5,  /* Terminate standby                        */
    ISM_HA_ACTION_SPLITBRAIN_RESTART  = 6,  /* Restart server                           */
} ismHA_Admin_Action_t;

/* Defines Admin HA info */
typedef struct ismAdminHAInfo_t
{
   ism_threadh_t       threadId;     /* Admin HA thread ID                 */
   pthread_mutex_t     threadMutex;  /* Admin HA thread mutex              */
   pthread_cond_t      threadCond;   /* Admin HA thread condition variable */
   pthread_spinlock_t  lock;         /* Admin HA spinlock                  */
   int                 role;         /* Current HA role                    */ 
   int                 sSyncStart;   /* Start SYNC on Standby Node         */
} ismAdminHAInfo_t;

extern ismAdminHAInfo_t *adminHAInfo;

/* Initialize Admin HA */
XAPI int32_t ism_adminHA_init(void);

/* Returns current role to admin
 *
 * The return buffer will be a JSON string and will include the following fields:
 *
 * { "NewRole":"PRIMARY","OldRole":"STANDBY","ActiveNodes":"2","SyncNodes":"2","ErrorCode":"0","ErrorString":"" }
 *
 * HA Roles could be one of the following:
 * UNSYNC    - Node is out of synchronization with the Primary. An attempt will be made to synchronize
 *             the node with the primary
 * PRIMARY   - Node is acting as the Primary
 * STANDBY   - Node is acting as a Standby
 * TERMINATE - Node terminated by the Primary (coordinated shutdown)
 * ERROR     - Node got out of synchronization due to a fatal error. NO attempt will be made to
 *             synchronize the node again. Store can no longer function
 *
 * ErrorCodes:
 * 1 - Config error - ErrorString will have name of the configuration item
 * 2 - Split brain
 * 9 - Internal Error - Node will be terminated
 *
 */
XAPI ismHA_Role_t ism_admin_getHArole(concat_alloc_t *output_buffer, int *rc);


/* Process admin message from primary */
XAPI int32_t ism_admin_ha_queueAdminAction(int actionType, int actionValue, ismHA_AdminMessage_t *adminMsg);

/* Invoke store method to transfer MQ Connectviity certificate */
XAPI int32_t ism_admin_ha_syncMQCertificates(void);

/**
 * Replicate a file from Primary to standby
 */
XAPI int32_t ism_admin_ha_syncFile(const char *dirname, const char *filename);

#ifdef __cplusplus
}
#endif

#endif

