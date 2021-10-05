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

#include <mqcInternal.h>
#include <assert.h>

#include <linux/futex.h>
#include <sys/syscall.h>

extern struct xa_switch_t MQRMIXASwitch;

#define XAOpen     MQRMIXASwitch.xa_open_entry
#define XAStart    MQRMIXASwitch.xa_start_entry
#define XAEnd      MQRMIXASwitch.xa_end_entry
#define XAClose    MQRMIXASwitch.xa_close_entry
#define XAForget   MQRMIXASwitch.xa_forget_entry
#define XAPrepare  MQRMIXASwitch.xa_prepare_entry
#define XAComplete MQRMIXASwitch.xa_complete_entry
#define XACommit   MQRMIXASwitch.xa_commit_entry
#define XARollback MQRMIXASwitch.xa_rollback_entry
#define XARecover  MQRMIXASwitch.xa_recover_entry

/* Constants */
#define XID_COUNT 16

/* Macros */
#define MQC_TRACE_XID(level, label, pXid)                              \
    TRACEDATA(level, label, 0, pXid,                                   \
              offsetof(ism_xid_t, data) +                              \
              (pXid) -> gtrid_length + (pXid) -> bqual_length,         \
              offsetof(ism_xid_t, data) +                              \
              (pXid) -> gtrid_length + (pXid) -> bqual_length);

/* XA Format ID "IMQM", taken from MQ */
static const MQINT32 XAFormatID = 0x494D514D;
static const MQINT32 XAFormatIDSwapped = 0x4D514D49;

/* Static variables */
static int rmid = 0;

static int offsetOfQMName = MQC_OBJECT_IDENTIFIER_LENGTH + sizeof(int) + sizeof(int)
        + sizeof(ism_time_t) + sizeof(uint64_t);

/* Function prototypes */
int mqc_commit(mqcRuleQM_t * pRuleQM);
int mqc_recover(mqcRuleQM_t * pRuleQM);
int mqc_rollback(mqcRuleQM_t * pRuleQM);
int mqc_XAClose(mqcRuleQM_t * pRuleQM, char * xainfo);

/* Functions */

void * mqc_MQXAConsumerThread(void * parm, void * context, int value)
{
    int rc = 0;
    mqcRuleQM_t * pRuleQM = parm;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    MQLONG messlen, CompCode, Reason;
    MQMD md = {MQMD_DEFAULT};
    MQGMO gmo = {MQGMO_DEFAULT};
    char xainfo[MAXINFOSIZE];

    bool requested, commit;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    pRuleQM -> xa_thread_started = true;

    /* Create IMA producer */
    rc = mqc_createProducerIMA(pRuleQM);
    if (rc) goto MOD_EXIT;

    /* Connect to queue manager */
    rc = mqc_connectSubscribeQM(pRuleQM);
    if (rc) goto MOD_EXIT;

    /* Check that no other instance is still connected. */
    rc = mqc_openSyncQueueExclusive(pRuleQM, pRuleQM -> hSubConn);
    if (rc) goto MOD_EXIT;

    /* XAOpen */
    sprintf(xainfo, "qmname=%s", pQM -> pQMName);
    if (pRuleQM -> flags & RULEQM_IMPLICIT_XA_END)
    {
        strcat(xainfo, ",@txnmdl=implxaend");
    }
    rc = XAOpen(xainfo, rmid, TMNOFLAGS);
    TRACE(MQC_MQAPI_TRACE, "XAOpen '%s' rc(%d)\n", xainfo, rc);

    if (rc)
    {
        rc = mqc_XAError(pRuleQM, "XAOpen", rc);
        goto MOD_EXIT;
    }

    pRuleQM -> flags |= RULEQM_MQ_XA_OPENED;

    /* Do recovery for any unresolved transactions remaining from a   */
    /* previous session.                                              */
    rc = mqc_recover(pRuleQM);
    if (rc) goto MOD_EXIT;

    /* Subscribe to topic / queue */
    rc = mqc_subscribeQM(pRuleQM);
    if (rc)
        goto MOD_EXIT;

    mqc_createXID(pRuleQM, &pRuleQM -> xid);

    /* IMA XAStart */
    rc = ismc_startGlobalTransaction(pRuleQM -> pSess, &pRuleQM -> xid);
    TRACE(MQC_ISMC_TRACE, "ismc_startGlobalTransaction rc(%d)\n", rc);

    if (rc)
    {
        rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_startGlobalTransaction", "");
        goto MOD_EXIT;
    }

    pRuleQM -> flags |= RULEQM_IMA_XA_STARTED;

    /* MQ XAStart */
    rc = XAStart((XID *)&pRuleQM -> xid, rmid, TMNOFLAGS);
    TRACE(MQC_MQAPI_TRACE, "XAStart rc(%d)\n", rc);

    if (rc)
    {
        rc = mqc_XAError(pRuleQM, "XAStart", rc);
        goto MOD_EXIT;
    }

    pRuleQM -> flags |= RULEQM_MQ_XA_STARTED;

    /* Let's start of with a 1024 byte buffer. However, always prefix */
    /* the buffer with enough space for an MQDLH in case we need to   */
    /* direct a message to the MQ dead letter queue.                  */
    if (pRuleQM -> pActualBuffer == NULL)
    {
    	pRuleQM -> pActualBuffer = mqc_malloc(1024 + sizeof(MQDLH));

        if (pRuleQM -> pActualBuffer)
        {
            pRuleQM -> bufferlen = 1024;
            pRuleQM -> actualBufferLength = 1024 + sizeof(MQDLH);
            pRuleQM -> buffer = pRuleQM -> pActualBuffer + sizeof(MQDLH);
        }
        else
        {
            mqc_ruleQMAllocError(pRuleQM,
                                 "malloc",
                                 1024 + sizeof(MQDLH),
                                 errno);
            rc = RC_RULEQM_DISABLE;
            goto MOD_EXIT;
        }
    }

    gmo.Version = MQGMO_VERSION_4;
    gmo.Options |= MQGMO_SYNCPOINT_IF_PERSISTENT | MQGMO_WAIT | MQGMO_PROPERTIES_IN_HANDLE;
    gmo.MsgHandle = pRuleQM -> hMsg;
    gmo.WaitInterval = mqcMQConnectivity.longBatchTimeout;

    messlen = pRuleQM -> bufferlen;

    while (true)
    {
        /* Have we been asked to stop ? */
        /* If so, quisce our consumers and drain the client read ahead buffer */
        if ((pRuleQM -> flags & RULEQM_DISABLE) ||
            (pRuleQM -> flags & RULEQM_USER_DISABLE))
        {
            if (!(pRuleQM -> flags & RULEQM_QUIESCING))
            {
                pRuleQM -> flags |= RULEQM_QUIESCING;

                rc = mqc_quiesceConsumeQM(pRuleQM);
                if (rc) goto MOD_EXIT;
            }

            if (pRuleQM -> flags & RULEQM_QUIESCED)
            {
                pRuleQM -> flags &= ~RULEQM_QUIESCING;
                pRuleQM -> flags &= ~RULEQM_QUIESCED;

                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
        }

        /* Do we need a larger buffer ? */
        if (messlen > pRuleQM -> bufferlen)
        {
        	void * newbuffer = ism_common_realloc(ISM_MEM_PROBE(ism_memory_mqcbridge_misc,1),pRuleQM -> pActualBuffer,
        			                   messlen + sizeof(MQDLH));

            if (newbuffer)
            {
            	pRuleQM -> pActualBuffer = newbuffer;
            	pRuleQM -> actualBufferLength = messlen + sizeof(MQDLH);
            	pRuleQM -> buffer = newbuffer + sizeof(MQDLH);
                pRuleQM -> bufferlen = messlen;
            }
            else
            {
                mqc_ruleQMAllocError(pRuleQM,
                                     "realloc",
                                     messlen + sizeof(MQDLH),
                                     errno);
                rc = RC_RULEQM_DISABLE;
                goto MOD_EXIT;
            }
        }

        /* Use MQMD V2 to ensure we don't get an MDE in our message */
        md.Version = MQMD_VERSION_2;

        memcpy(md.MsgId, MQMI_NONE, sizeof(md.MsgId));
        memcpy(md.CorrelId, MQCI_NONE, sizeof(md.CorrelId));

        /* MQ defect 21946 requires us to reset gmo.MsgHandle */
        gmo.MsgHandle = pRuleQM -> hMsg;

        MQGET(pRuleQM -> hSubConn,
              pRuleQM -> hObj,
              &md,
              &gmo,
              pRuleQM -> bufferlen,
              pRuleQM -> buffer,
              &messlen,
              &CompCode,
              &Reason);
        TRACE((Reason == MQRC_NO_MSG_AVAILABLE) ? MQC_HIGH_TRACE : MQC_MQAPI_TRACE,
              "MQGET '%s' '%s' CompCode(%d) Reason(%d) messlen(%d) Persistence(%d)\n",
              pRuleQM -> pQM -> pQMName, pRule -> pSource, CompCode, Reason, messlen, md.Persistence);

        if (Reason == MQRC_HOBJ_QUIESCED_NO_MSGS)
        {
            pRuleQM -> flags |= RULEQM_QUIESCED;
            continue;
        }

        if (Reason == MQRC_TRUNCATED_MSG_FAILED)
        {
            continue;
        }

        /* Is this a multicast subscription ? */
        if ((Reason == MQRC_OPTIONS_ERROR) &&
            (pRuleQM -> flags & RULEQM_NON_DURABLE) &&
           !(pRuleQM -> flags & RULEQM_NO_SYNCPOINT))
        {
            gmo.Options &= ~MQGMO_SYNCPOINT_IF_PERSISTENT;
            pRuleQM -> flags |= RULEQM_NO_SYNCPOINT;
            continue;
        }

        commit = false;

        if (Reason == MQRC_NONE)
        {
            rc = mqc_processMQMessage(pRuleQM, &md, messlen, pRuleQM -> buffer);
            if (rc) goto MOD_EXIT;

            pRuleQM -> batchMsgsSent += 1;
            pRuleQM -> batchDataSent += messlen;

            /* Mark ourselves as reconnected */
            if (pRuleQM -> flags & RULEQM_RECONNECTING)
            {
                rc = mqc_ruleQMReconnected(pRuleQM);
                if (rc) goto MOD_EXIT;
            }

            if (md.Persistence == MQPER_PERSISTENT)
            {
                if (!(pRuleQM -> flags & RULEQM_NO_SYNCPOINT))
                {
                    /* MQ persistent messages are transactional       */
                    /* (MQGMO_SYNCPOINT_IF_PERSISTENT)                */
                    pRuleQM -> flags |= RULEQM_MQ_XA_COMMIT;
                    /* IMA QoS 1/2 messages are transactional         */
                    /* (MQ QoS 1/2 messages are persistent)           */
                    pRuleQM -> flags |= RULEQM_IMA_XA_COMMIT;

                    gmo.WaitInterval = mqcMQConnectivity.shortBatchTimeout;
                }

                pRuleQM -> persistentCount++;
            }
            else
            {
                pRuleQM -> nonpersistentCount++;
            }

            /* Does that complete a batch ? */
            if ((pRuleQM -> batchMsgsSent >= pQM -> BatchSize) ||
                (pRuleQM -> batchDataSent >=
                 mqcMQConnectivity.batchDataLimit) ||
                (pRuleQM -> batchMsgsToResend))
            {
                commit = true;
            }
        }
        else if (Reason == MQRC_NO_MSG_AVAILABLE)
        {
            if (pRuleQM -> batchMsgsSent > 0) commit = true;
        }
        else
        {
            mqc_MQAPIError(pRuleQM, RC_MQAPIError, "MQGET", pRule -> pSource, Reason);
            rc = RC_RULEQM_RECONNECT;
            goto MOD_EXIT;
        }

        if (commit)
        {
            rc = mqc_commit(pRuleQM);
            if (rc) goto MOD_EXIT;

            gmo.WaitInterval = mqcMQConnectivity.longBatchTimeout;

            if (!(pRuleQM -> flags & RULEQM_IMA_XA_STARTED))
            {
                mqc_createXID(pRuleQM, &pRuleQM -> xid);
    
                /* IMA XAStart */
                rc = ismc_startGlobalTransaction(pRuleQM -> pSess, &pRuleQM -> xid);
                TRACE(MQC_ISMC_TRACE, "ismc_startGlobalTransaction rc(%d)\n", rc);
    
                if (rc)
                {
                    rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_startGlobalTransaction", "");
                    goto MOD_EXIT;
                }

                pRuleQM -> flags |= RULEQM_IMA_XA_STARTED;
            }

            if (!(pRuleQM -> flags & RULEQM_MQ_XA_STARTED))
            {
                /* MQ XAStart */
                rc = XAStart((XID *)&pRuleQM -> xid, rmid, TMNOFLAGS);
                TRACE(MQC_MQAPI_TRACE, "XAStart rc(%d)\n", rc);
    
                if (rc)
                {
                    rc = mqc_XAError(pRuleQM, "XAStart", rc);
                    goto MOD_EXIT;
                }
    
                pRuleQM -> flags |= RULEQM_MQ_XA_STARTED;
            }
        }
    }

MOD_EXIT:

    pthread_mutex_lock(&pRuleQM -> controlMutex);

    /* Rollback transactions */
    mqc_rollback(pRuleQM);

    /* Unsubscribe from qm */
    requested = (pRuleQM -> flags & RULEQM_USER_DISABLE) ? true : false;

    mqc_unsubscribeQM(pRuleQM,
                      requested); /* user requested */

    /* Close and delete qm sync queue */
    mqc_closeAndDeleteSyncQueue(pRuleQM, pRuleQM -> hSubConn);

    /* Disconnect from qm */
    mqc_disconnectSubscribeQM(pRuleQM);

    /* XAClose */
    mqc_XAClose(pRuleQM, xainfo);

    /* Delete IMA producer */
    mqc_deleteProducerIMA(pRuleQM);

    time(&pRuleQM -> disabledTime);

    pRuleQM -> flags &= ~RULEQM_DISABLE;
    pRuleQM -> flags &= ~RULEQM_USER_DISABLE;
    pRuleQM -> flags |=  RULEQM_DISABLED;

    if (rc == RC_RULEQM_RECONNECT)
    {
        /* We are going to exit this thread without a pthread_join so */
        /* mark the thread as detached to avoid a memory leak.        */
        ism_common_detachThread(ism_common_threadSelf());
        pRuleQM -> flags |= RULEQM_RECONNECT;
        mqc_wakeReconnectThread();
    }
    else if (rc == RC_MQC_RECONNECT)
    {
        /* We are going to exit this thread without a pthread_join so */
        /* mark the thread as detached to avoid a memory leak.        */
        ism_common_detachThread(ism_common_threadSelf());
        mqcMQConnectivity.flags |= MQC_RECONNECT;
        mqc_wakeReconnectThread();
    }

    pthread_mutex_unlock(&pRuleQM -> controlMutex);

    pRuleQM -> xa_thread_started = false;

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return NULL;
}

MQLONG mqc_eventHandler(MQHCONN hConn,
                        MQMD *pMsgDesc,
                        MQGMO *pGetMsgOpts,
                        MQBYTE *Buffer,
                        MQCBC *pContext)
{
    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    switch (pContext->Reason)
    {
    case MQRC_CONNECTION_BROKEN:
    {
        /* Set the flag bit to tell the main thread that the          */
        /* connection is broken.                                      */
        mqcRuleQM_t *pRuleQM = pContext -> CallbackArea;

        pRuleQM -> flags |= RULEQM_IDLE_CONN_BROKEN;

        break;
    }

    default:
        TRACE(MQC_MQAPI_TRACE,
              "Callback saw reason code = %d\n",
              pContext -> Reason);
        break;
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "\n", __func__);

    ism_common_freeTLS();

    return 0;
}

/* The mqc_IMAXAConsumerThread function runs in a separate thread,    */
/* with one instance per rule/hConn pair. It is hConn rather than QM  */
/* because we might have multiple connections to the same QM and each */
/* one will have a separate instance of this function/thread.         */

/* The function signature is defined by pthread_create, however ...   */
/* pArgs is actually a pointer to a Rule/QM block.                    */
/* The other two parameters are unused.                               */

void *mqc_IMAXAConsumerThread(void *pArgs, void *pContext, int value)
{
    int exitrc, rc = 0;
    int casStatus = 1;
    int futexStatus = 0;

    mqcRuleQM_t *pRuleQM = pArgs;
    mqcRule_t *pRule = pRuleQM -> pRule;
    mqcQM_t *pQM = pRuleQM -> pQM;

    char xainfo[MAXINFOSIZE] = "";
    bool receiveActiveOwned = false;

    MQCBD cbd = { MQCBD_DEFAULT }; /* callback descriptor */
    MQLONG CompCode;
    MQLONG Reason;

    TRACE(MQC_FNC_TRACE,
          FUNCTION_ENTRY "pArgs = %p, pContext = %p, value = %d\n",
          __func__,
          pArgs,
          pContext,
          value);
    TRACE(MQC_MQAPI_TRACE,
          "Rule index = %d; QM index = %d\n",
          pRule -> index,
          pQM -> index);

    pRuleQM -> xa_thread_started = true;

    /* Connect to the queue manager */
    rc = mqc_connectPublishQM(pRuleQM);
    if (rc)
        goto MOD_EXIT;

    /* Check that no other instance is still connected. */
    rc = mqc_openSyncQueueExclusive(pRuleQM, pRuleQM -> hPubConn);
    if (rc)
        goto MOD_EXIT;

    /* Register an event handler to detect a broken QM connection */
    cbd.CallbackType = MQCBT_EVENT_HANDLER;
    cbd.CallbackFunction = mqc_eventHandler;
    cbd.CallbackArea = pRuleQM;

    MQCB(pRuleQM -> hPubConn,
         MQOP_REGISTER,
         &cbd,
         MQHO_UNUSABLE_HOBJ,
         NULL,
         NULL,
         &CompCode,
         &Reason);
    TRACE(MQC_MQAPI_TRACE,
          "MQCB '%s' CompCode(%d) Reason(%d)\n",
          pQM -> pQMName, CompCode, Reason);

    if (CompCode == MQCC_FAILED)
    {
        mqc_MQAPIError(pRuleQM,
                       RC_MQCBError,
                       "MQCB",
                       pQM -> pQMName,
                       Reason);
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

    /* Open MQ publish/put destination */
    rc = mqc_openProducerQM(pRuleQM);
    if (rc) goto MOD_EXIT;

    /* Create IMA commit session */
    rc = mqc_createRuleQMSessionIMA(pRuleQM);
    if (rc) goto MOD_EXIT;

    if (!(pRuleQM -> flags & RULEQM_MULTICAST))
    {
        /* XAOpen */
        sprintf(xainfo, "qmname=%s", pQM -> pQMName);
        if (pRuleQM -> flags & RULEQM_IMPLICIT_XA_END)
        {
            strcat(xainfo, ",@txnmdl=implxaend");
        }
        rc = XAOpen(xainfo, rmid, TMNOFLAGS);
        TRACE(MQC_MQAPI_TRACE, "XAOpen '%s' rc(%d)\n", xainfo, rc);
    
        if (rc)
        {
            rc = mqc_XAError(pRuleQM, "XAOpen", rc);
            goto MOD_EXIT;
        }

        pRuleQM -> flags |= RULEQM_MQ_XA_OPENED;
    }

    /* Do recovery for any unresolved transactions remaining from a   */
    /* previous session.                                              */
    rc = mqc_recover(pRuleQM);
    if (rc) goto MOD_EXIT;

    /* All instances of this thread issue ismc_receive for the same   */
    /* subscription, however, only one can do so at any time.         */
    /* Therefore, we have a bit in the pRule block that says whether  */
    /* any thread is currently doing the receive. If not, then this   */
    /* thread locks the bit and becomes the active receiver. If it is */
    /* locked, this thread enters a futex wait state, until woken by  */
    /* a thread that has switched from receiving to committing.       */

    /* It should be zero before (meaning free) and set to one after   */
    /* (because we have locked it). The before value is returned in   */
    /* status, so, if that is zero then we have the lock. If it is 1  */
    /* then we must wait.                                             */

    while (!(pRuleQM -> flags & RULEQM_DISABLE) &&
           !(pRuleQM -> flags & RULEQM_USER_DISABLE))
    {
        /* Attempt to lock the receive bit.                           */
        casStatus
            = __sync_val_compare_and_swap(&(pRule -> receiveActive), 0, 1);
        if (casStatus == 1)
        {
            /* The bit was already set, so we must wait. No timeout   */
            /* because in normal operation we will be woken by an     */
            /* explicit futex call from another receiver thread. If   */
            /* we are shutting down, we will be woken by a wake all.  */
            futexStatus = EINTR;
            while ((futexStatus != 0) && (futexStatus != EWOULDBLOCK))
            {
                TRACE(MQC_MQAPI_TRACE,
                      "Entering futex wait: Rule index = %d, QM index = %d\n",
                      pRule -> index,
                      pQM -> index);

                /* Wait for the current receiver thread to signal     */
                /* they have finished and a new receiver is required. */
                futexStatus =
                    syscall(SYS_futex,
                            &(pRule -> receiveActive),
                            FUTEX_WAIT,
                            1,          /* Required initial value */
                            NULL,       /* Timeout interval - no timeout*/
                            NULL,       /* Ignored */
                            0);         /* Ignored */
                if (futexStatus < 0)
                {
                    futexStatus = errno;
                }

                /* At this point, futexStatus is one of -             */
                /* 0 - We were woken by an explicit futex wake.       */
                /*     Check whether we are stopping and either       */
                /*     comply or retry the compare-and-swap.          */
                /* EINTR - spurious wake, re-enter wait state         */
                /* ETIMEDOUT - should never happen                    */
                /* EWOULDBLOCK - initial value wasn't 1, ie the bit   */
                /*               is now unlocked.                     */

            } /* while ((futexStatus != 0) && (futexStatus != EWOULDBLOCK)) */
        }     /* End block for (casStatus === 1) */
        else
        {
        	int64_t receiveTimeout = mqcMQConnectivity.longBatchTimeout;
            receiveActiveOwned = true;

            /* We now own the receive bit. Forward some messages.     */
            TRACE(MQC_MQAPI_TRACE,
                  "Start ismc_receive messages: Rule index = %d, QM index = %d\n",
                  pRule -> index,
                  pQM -> index);

            if (!(pRule -> flags & RULE_IMA_XA_STARTED))
            {
                mqc_createXID(pRuleQM, &pRule -> xid);
    
                /* IMA XAStart */
                rc = ismc_startGlobalTransaction(pRule -> pSess, &pRule -> xid);
                TRACE(MQC_ISMC_TRACE, "ismc_startGlobalTransaction rc(%d)\n", rc);
    
                if (rc)
                {
                    rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_startGlobalTransaction", "");
                    goto MOD_EXIT;
                }

                pRule -> flags |= RULE_IMA_XA_STARTED;
            }

            while (!(pRuleQM -> flags & RULEQM_DISABLE) &&
                   !(pRuleQM -> flags & RULEQM_USER_DISABLE))
            {
                ismc_message_t *pMessage = NULL;

                if (pRule -> flags & RULE_MAXMESSAGES_UPDATED)
                {
                    /* Call ISM C function to change MaxMessages */
                    ism_prop_t *pProperties = ism_common_newProperties(1);

                    ism_field_t f;
                    f.type = VT_Integer;
                    f.val.i = pRule -> MaxMessages;
                    rc = ism_common_setProperty(pProperties,
                                                "MaxMessages",
                                                &f);

                    rc = ismc_updateSubscription(pRule -> pConsumer, pProperties);

                    TRACE(MQC_ISMC_TRACE, "ismc_updateSubscription returned rc = %d\n", rc);

                    if (rc != 0)
                    {
                        /* Log and trace the failure. */
                        char errorText[512] = "";
                        int ismc_rc = 0;

                        ismc_rc = ism_common_getLastError();
                        ism_common_formatLastError(errorText, sizeof(errorText));

                        TRACE(MQC_ISMC_TRACE, "Error text: %s. (%d)\n", errorText, ismc_rc);

                        LOG(ERROR,
                            MQConnectivity,
                            7030,
                            "%d%s",
                            "Rule index {0}. Unable to alter MaxMessages for subscription - {1}",
                            pRule -> index,
                            errorText);
                    }
                    else
                    {
                        pRule -> flags &= ~RULE_MAXMESSAGES_UPDATED;
                    }
                }

                rc = ismc_receive(pRule -> pConsumer,
                                  receiveTimeout,
                                  &pMessage);

                if (rc != ISMRC_OK)
                {
                    if (rc != ISMRC_TimeOut)
                    {
                        rc = mqc_ISMCRuleError(pRule, "ismc_receive", "");
                        goto MOD_EXIT;
                    }

                    if (pRuleQM -> flags & RULEQM_IDLE_CONN_BROKEN)
                    {
                        /* The MQ connection broke while we were      */
                        /* waiting in ismc_receive.                   */
                        TRACE(MQC_ISMC_TRACE, "Exiting due to callback detecting broken connection.\n");
                        mqc_MQAPIError(pRuleQM,
                                       RC_MQCONNXError,
                                       "via MQCB",
                                       pQM -> pQMName,
                                       MQRC_CONNECTION_BROKEN);
                        pRuleQM -> flags &= ~RULEQM_IDLE_CONN_BROKEN;
                        rc = RC_RULEQM_RECONNECT;
                        goto MOD_EXIT;
                    }

                    /* If we have already sent at least one message   */
                    /* then attempt to switch receive thread.         */
                    /* Otherwise, re-issue the receive.               */
                    if (pRuleQM -> batchMsgsSent > 0)
                    {
                        break;  /* Out of message receive while loop. */
                    }
                }
                else /* !(rc != ISMRC_OK) */
                {
                    assert(pMessage != NULL);

                    TRACE(MQC_ISMC_TRACE,
                          "ismc_receive OrderID(%ld) ack_sqn(%ld)\n",
                          ismc_getOrderID(pMessage), pMessage -> ack_sqn);

                    /* If we are going to put a transactional message */
                    /* we need to be XA started                       */

                    if (ismc_getQuality(pMessage))
                    {
                    	receiveTimeout = mqcMQConnectivity.shortBatchTimeout;
                    	if (!(pRuleQM -> flags & RULEQM_MQ_XA_STARTED))
                    	{
                    		memcpy(&pRuleQM -> xid, &pRule -> xid, sizeof(ism_xid_t));

                    		if (!(pRuleQM -> flags & RULEQM_MULTICAST))
                    		{
                    			rc = XAStart((XID *)&pRuleQM -> xid, rmid, TMNOFLAGS);
                    			TRACE(MQC_MQAPI_TRACE, "XAStart rc(%d)\n", rc);
                    			if (rc)
                    			{
                    				ismc_freeMessage(pMessage);
                    				rc = mqc_XAError(pRuleQM, "XAStart", rc);
                    				goto MOD_EXIT;
                    			}

                    			pRuleQM -> flags |= RULEQM_MQ_XA_STARTED;
                    		}
                    	}
                    }

                    /* MQPUT this message to MQ.                      */
                    rc = mqc_putMessageQM(pRuleQM,
                                          pMessage);
                    ismc_freeMessage(pMessage);
                    if (rc) goto MOD_EXIT;

                    /* If that message completes a batch then attempt */
                    /* to switch receive thread.                      */
                    if ((pRuleQM -> batchMsgsSent >= pQM -> BatchSize) ||
                        (pRuleQM -> batchDataSent >=
                         mqcMQConnectivity.batchDataLimit) ||
                        (pRuleQM -> batchMsgsToResend))
                    {
                        break;  /* Out of message receive while loop. */
                    }
                }
            } /* while ("stop not requested") */

            /* At this point, either a stop was requested, or a there */
            /* are messages at the queue manager waiting for a commit */

            if (!(pRuleQM -> flags & RULEQM_DISABLE) &&
                !(pRuleQM -> flags & RULEQM_USER_DISABLE))
            {
                if (pRule -> flags & RULE_IMA_XA_COMMIT)
                {
                    /* Pass ownership of the xid to our RuleQM session    */
                    rc = ismc_endGlobalTransaction(pRule -> pSess);
                    TRACE(MQC_ISMC_TRACE, "ismc_endGlobalTransaction rc(%d)\n", rc);
    
                    if (rc)
                    {
                        rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_endGlobalTransaction", "");
                        goto MOD_EXIT;
                    }

                    pRule -> flags &= ~RULE_IMA_XA_STARTED;

                    pRule -> flags &= ~RULE_IMA_XA_COMMIT;
                    pRuleQM -> flags |= RULEQM_IMA_XA_COMMIT;
                }

                /* receiveActive is 1 (because we have it). Decrement */
                /* it, and wake another receiver thread, if any exist */
                __sync_fetch_and_sub(&(pRule -> receiveActive), 1);
                syscall(SYS_futex,
                        &(pRule -> receiveActive),
                        FUTEX_WAKE,        /* operation */
                        1,                 /* wake one thread   */
                        NULL,              /* timeout - ignored */
                        NULL,              /* uaddr2 - ignored  */
                        0);                /* val3 - ignored    */

                receiveActiveOwned = false;

                /* If we sent a nonpersistent message in this   */
                /* batch we should issue an MQSTAT.             */
                if (pRuleQM -> flags & RULEQM_MQSTAT_REQUIRED)
                {
                    rc = mqc_MQSTAT(pRuleQM);

                    /* If we sent a transaction message in this */
                    /* batch we should rollback our transaction */
                    /* if we had an asynchronous MQPUT failure. */
                    if ((rc) &&
                        (pRuleQM -> flags & RULEQM_MQ_XA_COMMIT))
                    {
                        goto MOD_EXIT;
                    }

                    rc = 0;

                    pRuleQM -> flags &= ~RULEQM_MQSTAT_REQUIRED;
                }

                /* Commit whatever messages we have sent to MQ.       */
                rc = mqc_commit(pRuleQM);
                if (rc) goto MOD_EXIT;

                /* Mark ourselves as reconnected */
                if (pRuleQM -> flags & RULEQM_RECONNECTING)
                {
                    rc = mqc_ruleQMReconnected(pRuleQM);
                    if (rc) goto MOD_EXIT;
                }
            }
        } /* End block for (casStatus != 1) */
    } /* while ("stop not requested") */

MOD_EXIT:

    /* Rollback transactions */
    mqc_rollback(pRuleQM);

    if (receiveActiveOwned)
    {
        if (pRule -> flags & RULE_IMA_XA_STARTED)
        {
            exitrc = ismc_endGlobalTransaction(pRule -> pSess);
            TRACE(MQC_ISMC_TRACE, "ismc_endGlobalTransaction rc(%d)\n", exitrc);

            pRule -> flags &= ~RULE_IMA_XA_STARTED;

            exitrc = ismc_rollbackGlobalTransaction(pRule -> pSess, &pRule -> xid);
            TRACE(MQC_ISMC_TRACE, "ismc_rollbackGlobalTransaction rc(%d)\n", exitrc);
    
            pRule -> flags &= ~RULE_IMA_XA_COMMIT;
        }

        /* receiveActive is 1 (because we have it). Decrement */
        /* it, and wake another receiver thread, if any exist */
        __sync_fetch_and_sub(&(pRule -> receiveActive), 1);
        syscall(SYS_futex,
                &(pRule -> receiveActive),
                FUTEX_WAKE,        /* operation */
                1,                 /* wake one thread   */
                NULL,              /* timeout - ignored */
                NULL,              /* uaddr2 - ignored  */
                0);                /* val3 - ignored    */

        receiveActiveOwned = false;
    }

    /* Delete IMA commit session */
    mqc_deleteRuleQMSessionIMA(pRuleQM);

    /* Close qm producer */
    mqc_closeProducerQM(pRuleQM);

    /* Close and delete qm sync queue */
    mqc_closeAndDeleteSyncQueue(pRuleQM, pRuleQM -> hPubConn);

    /* Disconnect from publish qm */
    mqc_disconnectPublishQM(pRuleQM);

    /* XAClose */
    mqc_XAClose(pRuleQM, xainfo);

    /* Mark ourselves as disabled and set appropriate reconnect flags */
    time(&pRuleQM -> disabledTime);

    pRuleQM -> flags &= ~RULEQM_DISABLE;
    pRuleQM -> flags &= ~RULEQM_USER_DISABLE;
    pRuleQM -> flags |=  RULEQM_DISABLED;

    if (rc == RC_RULEQM_RECONNECT)
    {
        /* We are going to exit this thread without a pthread_join so */
        /* mark the thread as detached to avoid a memory leak.        */
        ism_common_detachThread(pRuleQM -> xa_thread_handle);
        pRuleQM -> flags |= RULEQM_RECONNECT;
        mqc_wakeReconnectThread();
    }
    else if (rc == RC_MQC_RECONNECT)
    {
        /* We are going to exit this thread without a pthread_join so */
        /* mark the thread as detached to avoid a memory leak.        */
        ism_common_detachThread(pRuleQM -> xa_thread_handle);
        mqcMQConnectivity.flags |= MQC_RECONNECT;
        mqc_wakeReconnectThread();
    }

    pRuleQM -> xa_thread_started = false;

    TRACE(MQC_FNC_TRACE,
            FUNCTION_EXIT "%d.%d rc=%d\n",
            __func__,
            pRule -> index,
            pQM -> index,
            rc);

    return NULL;
}

/* XID format.                                                        */
/* formatID = "MQMI" ie 0x494D514D                                    */
/* gtrid_length = length of first five elements of data field.        */
/* bqual_length = length of remaining element of data field.          */
/* data -                                                             */
/* char[12] : objectIdentifier (12 bytes)                             */
/* int    : rule index (4 bytes)                                      */
/* int    : QM index (4 bytes)                                        */
/* ism_time_t : time stamp (8 bytes)                                  */
/* uint64_t : sequence number (8 bytes)                               */
/* char[48] : queue manager name (48 bytes)                           */

int mqc_createXID(mqcRuleQM_t *pRuleQM, ism_xid_t *pXid)
{
    int rc = 0;

    mqcRule_t *pRule = pRuleQM -> pRule;
    mqcQM_t *pQM = pRuleQM -> pQM;

    uint8_t *pNextCh = pXid -> data;

    TRACE(MQC_FNC_TRACE,
          FUNCTION_ENTRY "pRuleQM = %p, pXid = %p\n",
          __func__,
          pRuleQM,
          pXid);

    if (pRuleQM -> xaSequenceNumber == 0)
    {
        pthread_mutex_lock(&pRule -> XIDTimestampMutex);
        do
        {
            pRuleQM -> xaInitialTime = ism_common_currentTimeNanos();
        }
        while(pRuleQM -> xaInitialTime == pRule -> mostRecentInitialTime);

        pRule -> mostRecentInitialTime = pRuleQM -> xaInitialTime;

        pthread_mutex_unlock(&pRule -> XIDTimestampMutex);
    }

    pXid -> formatID = XAFormatID;

    memcpy(pNextCh, &(mqcMQConnectivity.objectIdentifier), MQC_OBJECT_IDENTIFIER_LENGTH);
    pNextCh += MQC_OBJECT_IDENTIFIER_LENGTH;
    memcpy(pNextCh, &(pRule -> index), sizeof(int));
    pNextCh += sizeof(int);
    memcpy(pNextCh, &(pQM -> index), sizeof(int));
    pNextCh += sizeof(int);
    memcpy(pNextCh, &(pRuleQM -> xaInitialTime), sizeof(ism_time_t));
    pNextCh += sizeof(ism_time_t);
    memcpy(pNextCh, &(pRuleQM -> xaSequenceNumber), sizeof(uint64_t));
    pNextCh += sizeof(uint64_t);

    (pRuleQM -> xaSequenceNumber)++;
    pXid -> gtrid_length = (pNextCh - (pXid -> data));

    strncpy((char *) pNextCh, pQM -> pQMName, MQ_Q_MGR_NAME_LENGTH);
    pXid -> bqual_length = strlen(pQM -> pQMName);

    TRACE(MQC_MQAPI_TRACE,
          "pXid -> gtrid_length = %d, pXid -> bqual_length = %d\n",
          pXid -> gtrid_length,
          pXid -> bqual_length);

    MQC_TRACE_XID(MQC_MQAPI_TRACE, "Created XID ", pXid);

    TRACE(MQC_FNC_TRACE,
            FUNCTION_EXIT "rc=%d\n",
            __func__,
            rc);
    return rc;
}

/**
 * Convert an MQ XID to a printable string.
 *
 * The format is three strings separated by colons where the first is the decimal formatID,
 * the second is the global transaction ID in hex, and the third is the branch qualifier in hex.
 * dspmqtrn in MQ shows entries in this format, so this is meant to help identify transactions
 * there.
 *
 * @param xid   The XID structure
 * @param buf   The output buffer
 * @param len   The length of the buffer (should be 300 byes for max size)
 *
 * @return The buffer with the xid filled in or NULL for an error
 */
char * mqc_MQXIDToString(const ism_xid_t * xid, char * buf, int len) {
    char   out[300];
    char * outp = out;
    uint8_t * in;
    int    i;
    static char myhex [18] = "0123456789ABCDEF";

    if (!xid || (uint32_t)xid->bqual_length>64 || (uint32_t)xid->gtrid_length>64) {
        if (len)
            *buf = 0;
        return NULL;
    }

    /* Put out the format ID in decimal */
    outp += sprintf(outp, "%d", xid->formatID);

    *outp++ = ':';

    /* Put out the global transaction ID */
    in = (uint8_t *)xid->data;
    for (i=0; i<xid->gtrid_length; i++) {
        *outp++ = myhex[(*in>>4)&0xf];
        *outp++ = myhex[*in++&0xf];
    }
    *outp++ = ':';

    /* Put out the branch qualifier */
    for (i=0; i<xid->bqual_length; i++) {
        *outp++ = myhex[(*in>>4)&0xf];
        *outp++ = myhex[*in++&0xf];
    }
    *outp = 0;

    /* Copy into the output buffer */
    ism_common_strlcpy(buf, out, len);
    return buf;
}

/* Determine whether an XID was generated by the current IMA system,  */
/* and if it was, go on to check whether it is associated with the    */
/* given rule/QM pair or whether it is an orphan, meaning that the    */
/* rule that generated it no longer exists.                           */
/* The rule/QM check is skipped if that parameter is NULL.            */

mqcXIDType_t mqc_classifyXID(ism_xid_t *pXid, mqcRuleQM_t *pRuleQM)
{
    mqcXIDType_t result = mqcXID_Alien;
    bool lockedRuleListMutex = false;

    uint8_t *pXidNextCh = pXid -> data;
    uint8_t *pXidObjectIdentifier = pXidNextCh;

    TRACE(MQC_FNC_TRACE,
            FUNCTION_ENTRY "pXid = %p\n",
            __func__,
            pXid);

    /* To be one of ours it must have the correct formatID and the    */
    /* data field must contain our objectIdentifier.                  */

    if ((pXid -> formatID == XAFormatID)
            && (memcmp(pXidObjectIdentifier,
                       &(mqcMQConnectivity.objectIdentifier),
                       MQC_OBJECT_IDENTIFIER_LENGTH) == 0))
    {
        result = mqcXID_Local;

        uint8_t *pXidQMName = pXidObjectIdentifier + offsetOfQMName;
        int *pXidRuleIndex = (int *) (pXidObjectIdentifier + MQC_OBJECT_IDENTIFIER_LENGTH);
        int *pXidQMIndex = (int *) (((uint8_t *) pXidRuleIndex) + sizeof(int));

        bool foundRule = false;
        mqcRule_t *pRule = NULL;

        if (pRuleQM)
        {
            mqcQM_t *pQM = pRuleQM -> pQM;
            int ruleindex = pRuleQM -> pRule -> index;

            if ((pQM -> lengthOfQMName == pXid -> bqual_length)
                    && ((*pXidRuleIndex == ruleindex) || (ruleindex == -1))
                    && (*pXidQMIndex == pQM -> index)
                    && (memcmp(pQM -> pQMName,
                               pXidQMName,
                               pQM -> lengthOfQMName) == 0))
            {
                result = mqcXID_Match;
                goto MOD_EXIT;
            }
        }

        /* Try for 10 seconds to take the rule list lock */
        struct timespec abs_time;
        clock_gettime(CLOCK_REALTIME , &abs_time);
        abs_time.tv_sec += 10; /* 10 seconds */

        int osrc = pthread_mutex_timedlock(&mqcMQConnectivity.ruleListMutex, &abs_time);

        /* If we got the lock, walk the rule list looking for this XID so */
        /* we can positively identify the XID as having an unknown rule.  */
        if (osrc == 0)
        {
            lockedRuleListMutex = true;

            /* We were able to lock the list, check if this XID matches a rule */
            pRule = mqcMQConnectivity.pRuleList;
            while (pRule != NULL)
            {
                if (*pXidRuleIndex == (pRule -> index))
                {
                    foundRule = true;
                    break;
                }
                pRule = pRule -> pNext;
            }

            if (!foundRule)
            {
                result = mqcXID_UnknownRule;
                goto MOD_EXIT;
            }
        }
        /* We didn't manage to get the lock, so we assume it's an XID for */
        /* another rule.                                                  */
        else
        {
            MQC_TRACE_XID(3, "Could not classify XID ", pXid);
            assert(result == mqcXID_Local);
        }
    }

MOD_EXIT:

    if (lockedRuleListMutex)
    {
        pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);
    }

    TRACE(MQC_FNC_TRACE,
            FUNCTION_EXIT "result=%d\n",
            __func__,
            result);
    return result;
}

/* Determine whether an XID was generated by the current IMA system.  */
/* Note, there is no checking of the validity of the XID that is      */
/* provided. If the input parameter is not in fact a valid XID then   */
/* results are unpredictable. If the pRuleQM parameter is not NULL,   */
/* then this function also checks that the XID is for the queue       */
/* manager and rule identified by pRuleQM.                            */

bool mqc_localXID(ism_xid_t *pXid, mqcQM_t *pQM, int ruleindex)
{
    bool result = false;

    uint8_t *pNextCh = pXid -> data;
    uint8_t *pObjectIdentifier = pNextCh;

    TRACE(MQC_FNC_TRACE,
          FUNCTION_ENTRY "pXid = %p\n",
          __func__,
          pXid);

    /* To be one of ours it must have the correct formatID and the    */
    /* data field must contain our objectIdentifier.                  */

    if ((pXid -> formatID == XAFormatID) &&
        (memcmp(pObjectIdentifier, &(mqcMQConnectivity.objectIdentifier), MQC_OBJECT_IDENTIFIER_LENGTH) == 0))
    {
        if (pQM)
        {
            uint8_t *pQMName = pObjectIdentifier + offsetOfQMName;
            int *pRuleIndex = (int *) (pObjectIdentifier + MQC_OBJECT_IDENTIFIER_LENGTH);
            int *pQMIndex = (int *) (((uint8_t *) pRuleIndex) + sizeof(int));

            if ((pQM -> lengthOfQMName == pXid -> bqual_length) &&
                ((*pRuleIndex == ruleindex) || (ruleindex == -1)) &&
                (*pQMIndex == pQM -> index) &&
                (memcmp(pQM -> pQMName, pQMName, pQM -> lengthOfQMName) == 0))
            {
                result = true;
            }
        }
        else
        {
            result = true;
        }
    }

    TRACE(MQC_FNC_TRACE,
            FUNCTION_EXIT "result=%d\n",
            __func__,
            result);
    return result;
}

/* We assume that the two XIDs have already been checked to apply to  */
/* the same queue manager / IMA pair (ie by mqc_localXID) so at this  */
/* point it is sufficient to show that the timestamp and sequence     */
/* numbers are the same.                                              */
bool mqc_localXIDEqual(ism_xid_t *pXidLeft, ism_xid_t *pXidRight)
{
    bool result = false;
    uint8_t *pNextChLeft = pXidLeft -> data;
    uint8_t *pNextChRight = pXidRight -> data;

    ism_time_t *pTimestampLeft = (ism_time_t *) (pNextChLeft + MQC_OBJECT_IDENTIFIER_LENGTH + sizeof(int) + sizeof(int));
    uint64_t *pSequenceNumberLeft = (uint64_t *) (((uint8_t *) pTimestampLeft) + sizeof(ism_time_t));

    ism_time_t *pTimestampRight = (ism_time_t *) (pNextChRight + MQC_OBJECT_IDENTIFIER_LENGTH + sizeof(int) + sizeof(int));
    uint64_t *pSequenceNumberRight = (uint64_t *) (((uint8_t *) pTimestampLeft) + sizeof(ism_time_t));

    TRACE(MQC_FNC_TRACE,
          FUNCTION_ENTRY "pXidLeft = %p, pXidRight = %p\n",
          __func__,
          pXidLeft,
          pXidRight);

    if ((*pTimestampLeft == *pTimestampRight) &&
        (*pSequenceNumberLeft == *pSequenceNumberRight))
    {
        result = true;
    }

    TRACE(MQC_FNC_TRACE,
            FUNCTION_EXIT "result=%d\n",
            __func__,
            result);

    return result;
}

/* Use xa_recover to retrieve all the XIDs for prepared transactions  */
/* on a given queue manager that were originally generated by this    */
/* instance of MQ Connectivity (ie all the XIDs that we must resolve  */
/* on that queue manager). The list also includes all XIDs that are   */
/* associated with rules that no longer exist, because we must        */
/* resolve those too.                                                 */

/* This function requires that the correct XAOpen call has already    */
/* been made within the thread in which it executes. If this is not   */
/* true, the XARecover call will fail.                                */
int mqc_recoverMQXIDsForQM(mqcRuleQM_t *pRuleQM)
{
    int rc = 0;
    int i = 0;
    int recoveredXIDCount = 0;
    bool locked = false;
    mqcQM_t *pQM = pRuleQM -> pQM;

    mqcXIDListElement_t *pXIDLE = NULL;

    ism_xid_t xids[XID_COUNT];
    int flags = TMSTARTRSCAN;

    TRACE(MQC_FNC_TRACE,
          FUNCTION_ENTRY "pQM = %p\n",
          __func__,
          pQM);

    /* Remove any XIDs that are already recorded for this QM.         */
    pXIDLE = pRuleQM -> pRecoveredXIDs_MQ;
    while (pXIDLE != NULL)
    {
        /* delete the element */
        pRuleQM -> pRecoveredXIDs_MQ = pXIDLE -> pNext;
        mqc_free(pXIDLE);
        pXIDLE = pRuleQM -> pRecoveredXIDs_MQ;
    }

    /* It may require multiple calls to xa_recover to retrieve all    */
    /* the XIDs known to the QM and all those calls must be made from */
    /* a single thread of execution, so we use a mutex to ensure that */
    /* xa_recover calls are made from just one thread at a time for   */
    /* any given queue manager                                        */
    pthread_mutex_lock(&(pQM -> xa_recoverMutex));
    locked = true;

    do
    {
        rc = XARecover((XID *) xids, XID_COUNT, rmid, flags);
        TRACE(MQC_MQAPI_TRACE, "XARecover rc(%d)\n", rc);
        if (rc < 0)
        {
            rc = mqc_XAError(pRuleQM, "XARecover", rc);
            goto MOD_EXIT;
        }

        /* We may have as many as XID_COUNT (16) XIDs returned by the */
        /* queue manager, but they might have originated with other   */
        /* TMs (including other IMAs). Examine each one and discard   */
        /* the ones that did not originate from the current IMA       */
        /* instance.                                                  */

        recoveredXIDCount = rc;

        for (i = 0; i < recoveredXIDCount; i++)
        {
            ism_xid_t *pRecoverXid = &xids[i];

            /* MQ defect 25399 -                                      */
            /* XARecover can return byte swapped formatID             */
            if (pRecoverXid -> formatID == XAFormatIDSwapped)
            {
                TRACE(MQC_MQAPI_TRACE, "Found byte swapped formatID\n");
                pRecoverXid -> formatID = XAFormatID;
            }

            mqcXIDType_t xidType = mqc_classifyXID(pRecoverXid, pRuleQM);

            MQC_TRACE_XID(MQC_MQAPI_TRACE, "pRecoverXid ", pRecoverXid);

            if ((xidType == mqcXID_Match) || (xidType == mqcXID_UnknownRule))
            {
                pXIDLE = mqc_malloc(sizeof(mqcXIDListElement_t));
                if (pXIDLE == NULL)
                {
                    mqc_ruleQMAllocError(pRuleQM, "malloc", sizeof(mqcXIDListElement_t), errno);
                    rc = RC_RULEQM_DISABLE;
                    goto MOD_EXIT;
                }
                pXIDLE -> pNext = pRuleQM -> pRecoveredXIDs_MQ;
                pRuleQM -> pRecoveredXIDs_MQ = pXIDLE;
                pXIDLE -> xidType = xidType;
                memcpy(&(pXIDLE -> xid), pRecoverXid, sizeof(ism_xid_t));
            }
        }

        if (recoveredXIDCount < XID_COUNT)
        {
            rc = 0;
            break;
        }

        flags = TMNOFLAGS;
        
    } while (rc != 0);

MOD_EXIT:
    if (locked)
    {
        pthread_mutex_unlock(&(pQM -> xa_recoverMutex));
        locked = false;
    }

    TRACE(MQC_FNC_TRACE,
            FUNCTION_EXIT "rc=%d\n",
            __func__,
            rc);
    return rc;
}

int mqc_recoverIMAXIDsForQM(mqcRuleQM_t *pRuleQM)
{
    int rc = 0;
    int i = 0;
    int recoveredXIDCount = 0;
    bool locked = false;

    mqcXIDListElement_t *pXIDLE = NULL;
    mqcIMA_t *pIMA = &mqcMQConnectivity.ima;

    ism_xid_t xids[XID_COUNT];
    int flags = TMSTARTRSCAN;

    TRACE(MQC_FNC_TRACE,
          FUNCTION_ENTRY "\n",
          __func__);

    /* Remove any XIDs that are already recorded for this QM.         */
    pXIDLE = pRuleQM -> pRecoveredXIDs_IMA;
    while (pXIDLE != NULL)
    {
        /* delete the element */
        pRuleQM -> pRecoveredXIDs_IMA = pXIDLE -> pNext;
        mqc_free(pXIDLE);
        pXIDLE = pRuleQM -> pRecoveredXIDs_IMA;
    }

    /* It may require multiple calls to retrieve all the XIDs known   */
    /* to IMA and all those calls must be made from a single thread   */
    /* of execution, so we use a mutex to ensure that the calls are   */
    /* made from just one thread at a time.                           */

    pthread_mutex_lock(&(pIMA -> xa_recoverMutex));
    locked = true;

    do
    {
        rc = ismc_recoverGlobalTransactions(pRuleQM -> pSess, xids, XID_COUNT, flags);
        TRACE(MQC_MQAPI_TRACE,
              "ismc_recoverGlobalTransactions rc(%d)\n",
              rc);
        if (rc < 0)
        {
            rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_recoverGlobalTransactions", "");
            goto MOD_EXIT;
        }

        /* We may have as many as XID_COUNT (16) XIDs returned by IMA */
        /* but they might have originated with other TMs (including   */
        /* other IMAs). Examine each one and discard the ones that    */
        /* did not originate from the current IMA instance.           */

        recoveredXIDCount = rc;

        for (i = 0; i < recoveredXIDCount; i++)
        {
            ism_xid_t *pRecoverXid = &xids[i];
            mqcXIDType_t xidType = mqc_classifyXID(pRecoverXid, pRuleQM);

            MQC_TRACE_XID(MQC_MQAPI_TRACE, "pRecoverXid ", pRecoverXid);

            if ((xidType == mqcXID_Match) || (xidType == mqcXID_UnknownRule))
            {
                pXIDLE = mqc_malloc(sizeof(mqcXIDListElement_t));
                if (pXIDLE == NULL)
                {
                    mqc_ruleQMAllocError(pRuleQM, "malloc", sizeof(mqcXIDListElement_t), errno);
                    rc = RC_RULEQM_DISABLE;
                    goto MOD_EXIT;
                }
                pXIDLE -> pNext = pRuleQM -> pRecoveredXIDs_IMA;
                pRuleQM -> pRecoveredXIDs_IMA = pXIDLE;
                pXIDLE -> xidType = xidType;
                memcpy(&(pXIDLE -> xid), pRecoverXid, sizeof(ism_xid_t));
            }
        }

        if (recoveredXIDCount < XID_COUNT)
        {
            rc = 0;
            break;
        }

        flags = TMNOFLAGS;
        
    } while (rc != 0);

MOD_EXIT:
    if (locked)
    {
        pthread_mutex_unlock(&(pIMA -> xa_recoverMutex));
        locked = false;
    }

    TRACE(MQC_FNC_TRACE,
            FUNCTION_EXIT "rc=%d\n",
            __func__,
            rc);
    return rc;
}

int mqc_commit(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d %d\n",
          __func__, ruleindex, qmindex, pRuleQM -> batchMsgsSent);

    if (pRuleQM -> flags & RULEQM_IMA_XA_COMMIT)
    {
        if (pRuleQM -> flags & RULEQM_MQ_XA_COMMIT)
        {
            /* MQ XAEnd */
            if (!(pRuleQM -> flags & RULEQM_IMPLICIT_XA_END))
            {
                rc = XAEnd((XID *)&pRuleQM -> xid, rmid, TMNOFLAGS);
                TRACE(MQC_MQAPI_TRACE, "XAEnd rc(%d)\n", rc);
            
                if (rc)
                {
                    rc = mqc_XAError(pRuleQM, "XAEnd", rc);
                    goto MOD_EXIT;
                }

                pRuleQM -> flags &= ~RULEQM_MQ_XA_STARTED;
            }

            /* MQ XAPrepare */
            rc = XAPrepare((XID *)&pRuleQM -> xid, rmid, TMNOFLAGS);
            TRACE(MQC_MQAPI_TRACE, "XAPrepare rc(%d)\n", rc);

            if (rc)
            {
            	if (rc == XA_RDONLY)
            	{
            		LOG(ERROR,
            		    MQConnectivity,
            			7031,
            			"%-s%s",
            			"Destination mapping rule {0}. Queue manager connection {1}. XAPrepare returned XA_RDONLY.",
            			pRule -> pName,
            			pQM -> pName);

                    pRuleQM -> flags &= ~RULEQM_MQ_XA_COMMIT;

                    rc = ISMRC_OK;
            	}
            	else
            	{
            		rc = mqc_XAError(pRuleQM, "XAPrepare", rc);
            		goto MOD_EXIT;
            	}
            }

            /* In case IMPLICIT_XA_END is not set */
            pRuleQM -> flags &= ~RULEQM_MQ_XA_STARTED;
        }
        /* If all transactional MQPUTs returned MQRC_NO_SUBS_MATCH    */
        /* then we need to issue XARollback                           */
        else
        {
            if (pRuleQM -> flags & RULEQM_MQ_XA_STARTED)
            {
                // On z/OS we need to explicitly end the transaction on distributed we don't
                if (pRuleQM -> pQM -> platformType == MQPL_ZOS)
                {
                    rc = XAEnd((XID *)&pRuleQM -> xid, rmid, TMNOFLAGS);
                    TRACE(MQC_MQAPI_TRACE, "XAEnd rc(%d)\n", rc);

                    if (rc)
                    {
                        rc = mqc_XAError(pRuleQM, "XAEnd", rc);
                        goto MOD_EXIT;
                    }
                }

                /* MQ XA rollback */
                rc = XARollback((XID *)&pRuleQM -> xid, rmid, TMNOFLAGS);
                TRACE(MQC_MQAPI_TRACE, "XARollback rc(%d)\n", rc);
                pRuleQM -> rollbackCount++;

                if (rc)
                {
                    rc = mqc_XAError(pRuleQM, "XARollback", rc);
                    goto MOD_EXIT;
                }

                pRuleQM -> flags &= ~RULEQM_MQ_XA_STARTED;
            }
        }

        /* IMA XAend (if started on this session) */
        if (pRuleQM -> flags & RULEQM_IMA_XA_STARTED)
        {
            rc = ismc_endGlobalTransaction(pRuleQM -> pSess);
            TRACE(MQC_ISMC_TRACE, "ismc_endGlobalTransaction rc(%d)\n", rc);

            if (rc)
            {
                rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_endGlobalTransaction", "");
                goto MOD_EXIT;
            }

            pRuleQM -> flags &= ~RULEQM_IMA_XA_STARTED;
        }

        /* IMA XAPrepare */
        rc = ismc_prepareGlobalTransaction(pRuleQM -> pSess, &pRuleQM -> xid);
        TRACE(MQC_ISMC_TRACE, "ismc_prepareGlobalTransaction rc(%d)\n", rc);
    
        if (rc)
        {
            mqc_ISMCRuleQMError(pRuleQM, "ismc_prepareGlobalTransaction", "");
            goto MOD_EXIT;
        }

        /* MQ XACommit */
        if (pRuleQM -> flags & RULEQM_MQ_XA_COMMIT)
        {
            rc = XACommit((XID *)&pRuleQM -> xid, rmid, TMNOFLAGS);
            TRACE(MQC_MQAPI_TRACE, "XACommit rc(%d)\n", rc);
        
            if (rc)
            {
                rc = mqc_XAError(pRuleQM, "XACommit", rc);
                goto MOD_EXIT;
            }
            else
            {
                pRuleQM -> commitCount++;
                pRuleQM -> committedMessageCount += pRuleQM -> batchMsgsSent;
            }
        }

        pRuleQM -> flags &= ~RULEQM_MQ_XA_COMMIT;

        /* IMA XACommit */
        rc = ismc_commitGlobalTransaction(pRuleQM -> pSess, &pRuleQM -> xid, 0);
        TRACE(MQC_ISMC_TRACE, "ismc_commitGlobalTransaction rc(%d)\n", rc);
    
        if (rc)
        {
            mqc_ISMCRuleQMError(pRuleQM, "ismc_commitGlobalTransaction", "");
            goto MOD_EXIT;
        }

        pRuleQM -> flags &= ~RULEQM_IMA_XA_COMMIT;
    }

    pRuleQM -> batchMsgsSent = 0;
    pRuleQM -> batchDataSent = 0;

    /* After a batch failure we resend messages individually until    */
    /* we've successfully sent the number of messages that were in    */
    /* the failed batch.                                              */
    if (pRuleQM -> batchMsgsToResend)
    {
        pRuleQM -> batchMsgsToResend--;
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_rollback(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d %d\n",
          __func__, ruleindex, qmindex, pRuleQM -> batchMsgsSent);

    if (pRuleQM -> flags & RULEQM_MQ_XA_STARTED)
    {
        // On z/OS we need to explicitly end the transaction on distributed we don't
        if (pRuleQM -> pQM -> platformType == MQPL_ZOS)
        {
            rc = XAEnd((XID *)&pRuleQM -> xid, rmid, TMNOFLAGS);
            TRACE(MQC_MQAPI_TRACE, "XAEnd rc(%d)\n", rc);

            if (rc)
            {
                rc = mqc_XAError(pRuleQM, "XAEnd", rc);
                goto MOD_EXIT;
            }
        }

        /* XARollback */
        rc = XARollback((XID *)&pRuleQM -> xid, rmid, TMNOFLAGS);
        TRACE(MQC_MQAPI_TRACE, "XARollback rc(%d)\n", rc);
        pRuleQM -> rollbackCount++;

        pRuleQM -> flags &= ~RULEQM_MQ_XA_STARTED;
        pRuleQM -> flags &= ~RULEQM_MQ_XA_COMMIT;
    }

    if (pRuleQM -> flags & RULEQM_IMA_XA_STARTED)
    {
        rc = ismc_endGlobalTransaction(pRuleQM -> pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_endGlobalTransaction rc(%d)\n", rc);

        pRuleQM -> flags &= ~RULEQM_IMA_XA_STARTED;
    }

    if (pRuleQM -> flags & RULEQM_IMA_XA_COMMIT)
    {
        /* Rollback IMA transaction */
        rc = ismc_rollbackGlobalTransaction(pRuleQM -> pSess, &pRuleQM -> xid);
        TRACE(MQC_ISMC_TRACE, "ismc_rollbackGlobalTransaction rc(%d)\n", rc);

        pRuleQM -> flags &= ~RULEQM_IMA_XA_COMMIT;
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_XAClose(mqcRuleQM_t * pRuleQM, char * xainfo)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if (pRuleQM -> flags & RULEQM_MQ_XA_OPENED)
    {
        /* XAClose */
        rc = XAClose(xainfo, rmid, TMNOFLAGS);
        TRACE(MQC_MQAPI_TRACE, "XAClose '%s' rc(%d)\n", xainfo, rc);

        pRuleQM -> flags &= ~RULEQM_MQ_XA_OPENED;
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

void mqc_deleteEntry(mqcXIDListElement_t **ppCurrentXIDLE,
                     mqcXIDListElement_t *pPreviousXIDLE,
                     mqcXIDListElement_t **ppHead)
{
    assert(*ppCurrentXIDLE != NULL);
    assert(*ppHead != NULL);

    if (pPreviousXIDLE == NULL)
    {
        /* Previous is the mqcRuleQM_t block.             */
        *ppHead = (*ppCurrentXIDLE) -> pNext;
        mqc_free(*ppCurrentXIDLE);
        *ppCurrentXIDLE = *ppHead;
    }
    else
    {
        /* Previous is a real XIDLE block.                */
        pPreviousXIDLE -> pNext = (*ppCurrentXIDLE) -> pNext;
        mqc_free(*ppCurrentXIDLE);
        *ppCurrentXIDLE = pPreviousXIDLE -> pNext;
    }
}

int mqc_recover(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    mqcXIDListElement_t *pCurrentXIDLE_QM, *pPreviousXIDLE_QM;
    mqcXIDListElement_t *pCurrentXIDLE_IMA, *pPreviousXIDLE_IMA;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    /* Issue xa_recover to MQ                                         */
    rc = mqc_recoverMQXIDsForQM(pRuleQM);
    if (rc) goto MOD_EXIT;

    /* Issue xa_recover equivalent to IMA                             */
    rc = mqc_recoverIMAXIDsForQM(pRuleQM);
    if (rc) goto MOD_EXIT;

    /* Resolve according to the following pattern.                    */
    /* XID returned by MQ but not IMA -> Rollback MQ transaction.     */
    /* XID returned by both           -> Commit both                  */
    /* XID returned by IMA but not MQ -> Commit IMA transaction.      */

    pCurrentXIDLE_QM = pRuleQM -> pRecoveredXIDs_MQ;
    pPreviousXIDLE_QM = NULL;

    while (pCurrentXIDLE_QM != NULL)
    {
        bool foundOnIMA = false;
        pCurrentXIDLE_IMA = pRuleQM -> pRecoveredXIDs_IMA;
        pPreviousXIDLE_IMA = NULL;

        while (pCurrentXIDLE_IMA != NULL)
        {
            if (mqc_localXIDEqual(&(pCurrentXIDLE_QM -> xid), &(pCurrentXIDLE_IMA -> xid)))
            {
                /* XID is present in MQ and IMA so commit both.       */
                /* MQ commit must come first, since the recovery      */
                /* sequence relies on that fact.                      */

                MQC_TRACE_XID(MQC_MQAPI_TRACE, "XID ", &(pCurrentXIDLE_QM -> xid));
                TRACE(MQC_MQAPI_TRACE, "Found in QM and ISM. Committing both ...\n");

                rc = XACommit((XID *) &(pCurrentXIDLE_QM -> xid), rmid, TMNOFLAGS);
                TRACE(MQC_MQAPI_TRACE, "XACommit rc(%d)\n", rc);
    
                if (rc)
                {
                    rc = mqc_XAError(pRuleQM, "XACommit", rc);
                    goto MOD_EXIT;
                }
                else
                {
                    pRuleQM -> commitCount++;
                }

                /* Commit the current XID */
                rc = ismc_commitGlobalTransaction(pRule -> pSess, &(pCurrentXIDLE_QM -> xid), 0);
                TRACE(MQC_ISMC_TRACE, "ismc_commitGlobalTransaction rc(%d)\n", rc);

                if (rc)
                {
                    rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_commitGlobalTransaction", "");
                    goto MOD_EXIT;
                }

                /* Delete the associated LEs                          */
                /* IMA List.                                          */
                mqc_deleteEntry(&pCurrentXIDLE_IMA,
                                pPreviousXIDLE_IMA,
                                &(pRuleQM -> pRecoveredXIDs_IMA));

                /* QM List.                                           */
                mqc_deleteEntry(&pCurrentXIDLE_QM,
                                pPreviousXIDLE_QM,
                                &(pRuleQM -> pRecoveredXIDs_MQ));

                foundOnIMA = true;
                break;          /* while (pCurrentXIDLE_ISM != NULL)  */
            }
            pPreviousXIDLE_IMA = pCurrentXIDLE_IMA;
            pCurrentXIDLE_IMA = pCurrentXIDLE_IMA -> pNext;
        } /* while (pCurrentXIDLE_IMA != NULL) */

        if (foundOnIMA == false)
        {
            /* This QM XID has no match in IMA so roll it back.       */

            MQC_TRACE_XID(MQC_MQAPI_TRACE, "XID ", &(pCurrentXIDLE_QM -> xid));
            TRACE(MQC_MQAPI_TRACE, "QM XID not found in IMA. Rolling it back ...\n");

            rc = XARollback((XID *) &(pCurrentXIDLE_QM -> xid), rmid, TMNOFLAGS);
            TRACE(MQC_MQAPI_TRACE, "XARollback rc(%d)\n", rc);
            pRuleQM -> rollbackCount++;

            if (rc)
            {
                char XIDBuffer[300];
                const char *XIDString;

                // Generate the string representation of this transaction's XID
                XIDString = mqc_MQXIDToString(&(pCurrentXIDLE_QM->xid), XIDBuffer, sizeof(XIDBuffer));

                assert(XIDString == XIDBuffer);

                // Someone has heuristically rolled back the MQ transaction which is what
                // we planned to do anyway, so we can let them forget the transaction.
                if (rc == XA_HEURRB)
                {
                    TRACE(MQC_MQAPI_TRACE, "QM XID %s heuristically rolled back. Calling forget.\n", XIDString);

                    rc = XAForget((XID *)&(pCurrentXIDLE_QM->xid), rmid, TMNOFLAGS);
                    TRACE(MQC_MQAPI_TRACE, "XAForget rc(%d)\n", rc);

                    if (rc)
                    {
                        rc = mqc_XAError(pRuleQM, "XAForget", rc);
                        goto MOD_EXIT;
                    }
                }
                // Someone heuristically committed the MQ transaction which we were going
                // to roll back - we'll ignore this transaction but put out a log message
                // so that 'someone' can forget the MQ transaction.
                else if (rc == XA_HEURCOM)
                {
                    TRACE(MQC_MQAPI_TRACE, "QM XID %s heuristically committed. Logging error but continuing.\n", XIDString);

                    LOG(ERROR,
                        MQConnectivity,
                        7032,
                        "%-s%s%s",
                        "Destination mapping rule {0}. Queue manager connection {1}. XID {2}. XARollback returned XA_HEURCOM.",
                        pRule -> pName,
                        pQM -> pName,
                        XIDString);

                    rc = XA_OK;
                }
                else
                {
                    rc = mqc_XAError(pRuleQM, "XARollback", rc);
                    goto MOD_EXIT;
                }
            }

            /* Delete the associated LE.                              */
            mqc_deleteEntry(&pCurrentXIDLE_QM,
                            pPreviousXIDLE_QM,
                            &(pRuleQM -> pRecoveredXIDs_MQ));
        }
    } /* while (pCurrentXIDLE_QM != NULL) */

    pCurrentXIDLE_IMA = pRuleQM -> pRecoveredXIDs_IMA;
    pPreviousXIDLE_IMA = NULL;

    while (pCurrentXIDLE_IMA != NULL)
    {
        /* Every XID in this list has no matching XID in the QM       */
        /* Commit this XID within IMA.                                */

        MQC_TRACE_XID(MQC_MQAPI_TRACE, "XID ", &(pCurrentXIDLE_IMA -> xid));
        TRACE(MQC_MQAPI_TRACE, "IMA XID not found in QM. Committing it ...\n");

        /* Commit the current XID */
        rc = ismc_commitGlobalTransaction(pRuleQM -> pSess, &(pCurrentXIDLE_IMA -> xid), 0);
        TRACE(MQC_ISMC_TRACE, "ismc_commitGlobalTransaction rc(%d)\n", rc);

        if (rc)
        {
            rc = mqc_ISMCRuleQMError(pRuleQM, "ismc_commitGlobalTransaction", "");
            goto MOD_EXIT;
        }

        /* Delete the LE.                                             */
        mqc_deleteEntry(&pCurrentXIDLE_IMA,
                        pPreviousXIDLE_IMA,
                        &(pRuleQM -> pRecoveredXIDs_IMA));
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

/* Retrieve all the XIDs known to the IMA server and check for any  */
/* that were created by the bridge, but that are now orphaned,      */
/* meaning that we can no longer contact the associated queue       */
/* manager to resolve both sides of the transaction. This generally */
/* shouldn't happen, but might occur if, for example, a queue       */
/* manager is deleted while we still have unresolved transactions.  */
/* For each local XID that we find, the corresponding transaction   */
/* is an orphan if there is no queue manager with the same index or */
/* if there is a queue manager with that index but it has a         */
/* different queue manager name.                                    */

int mqc_rollbackOrphanXIDs(void)
{
    int rc = 0;
    int i = 0;
    int recoveredXIDCount = 0;
    bool lockedRecoverMutex = false;
    bool lockedQMListMutex = false;
    bool lockedRuleListMutex = false;

    int transacted = 1;
    int ackmode = SESSION_TRANSACTED_GLOBAL;
    ismc_session_t * pSession = NULL;
    mqcXIDListElement_t *pXIDLE = NULL;
    mqcIMA_t *pIMA = &mqcMQConnectivity.ima;

    ism_xid_t xids[XID_COUNT];
    int flags = TMSTARTRSCAN;

    TRACE(MQC_FNC_TRACE,
          FUNCTION_ENTRY "\n",
          __func__);

    /* Transacted session to allow rollback, if needed                */
    pSession = ismc_createSession(pIMA -> pConn, transacted, ackmode);
    if (!pSession)
    {
        /* This code is attempting to clear up some orphaned        */
        /* transactions. If it fails because IMA is unresponsive    */
        /* some other component will report that soon enough and we */
        /* will complete the clean up at a later time.              */

        TRACE(MQC_ISMC_TRACE,
              "ismc_createSession failed. Abandoning XID purge attempt.\n");
        goto MOD_EXIT;
    }

    /* It may require multiple calls to retrieve all the XIDs known   */
    /* to IMA and all those calls must be made from a single thread   */
    /* of execution, so we use a mutex to ensure that the calls are   */
    /* made from just one thread at a time.                           */
    pthread_mutex_lock(&(pIMA -> xa_recoverMutex));
    lockedRecoverMutex = true;

    /* Remove any XIDs that are already recorded for this QM.         */
    pXIDLE = pIMA -> pRecoveredXIDs_IMA;
    while (pXIDLE != NULL)
    {
        /* delete the element */
        pIMA -> pRecoveredXIDs_IMA = pXIDLE -> pNext;
        mqc_free(pXIDLE);
        pXIDLE = pIMA -> pRecoveredXIDs_IMA;
    }

    do
    {
        rc = ismc_recoverGlobalTransactions(pSession,
                                            xids,
                                            XID_COUNT,
                                            flags);
        TRACE(MQC_MQAPI_TRACE,
              "ismc_recoverGlobalTransactions rc(%d)\n",
              rc);
        if (rc < 0)
        {
            TRACE(MQC_ISMC_TRACE,
                  "ismc_recoverGlobalTransactions failed. Abandoning XID purge attempt.\n");
            goto MOD_EXIT;
        }

        /* We may have as many as XID_COUNT (16) XIDs returned by IMA */
        /* but they might have originated with other TMs (including   */
        /* other IMAs). Examine each one and discard the ones that    */
        /* did not originate from the current IMA instance.           */

        recoveredXIDCount = rc;

        for (i = 0; i < recoveredXIDCount; i++)
        {
            ism_xid_t *pRecoverXid = &xids[i];

            MQC_TRACE_XID(MQC_MQAPI_TRACE, "pRecoverXid ", pRecoverXid);

            if (mqc_localXID(pRecoverXid, NULL, -1))
            {
                pXIDLE = mqc_malloc(sizeof(mqcXIDListElement_t));
                if (pXIDLE == NULL)
                {
                    mqc_allocError("malloc", sizeof(mqcXIDListElement_t), errno);
                    rc = RC_MQC_DISABLE;
                    goto MOD_EXIT;
                }
                pXIDLE -> pNext = pIMA -> pRecoveredXIDs_IMA;
                pIMA -> pRecoveredXIDs_IMA = pXIDLE;
                memcpy(&(pXIDLE -> xid), pRecoverXid, sizeof(ism_xid_t));
            }
        }

        if (recoveredXIDCount < XID_COUNT) break;

        flags = TMNOFLAGS;
        
    } while (rc != 0);

    /* We now have a list of XIDs that have the IMA formatID and    */
    /* the correct objectIdentifier. Check each one to confirm that */
    /* the associated queue manager is still known to the bridge.   */

    pXIDLE = pIMA -> pRecoveredXIDs_IMA;
    while (pXIDLE != NULL)
    {

        uint8_t *pObjectIdentifier = (pXIDLE -> xid).data;
        uint8_t *pQMName = pObjectIdentifier + offsetOfQMName;
        int *pRuleIndex = (int *) (pObjectIdentifier + MQC_OBJECT_IDENTIFIER_LENGTH);
        int *pQMIndex = (int *) (((uint8_t *) pRuleIndex) + sizeof(int));

        mqcQM_t *pQM = NULL;
        mqcRule_t *pRule = NULL;
        bool foundQM = false;
        bool foundRule = false;

        /* Walk the QM list looking for this queue manager.         */
        pthread_mutex_lock(&mqcMQConnectivity.qmListMutex);
        lockedQMListMutex = true;

        pQM = mqcMQConnectivity.pQMList;
        while (pQM != NULL)
        {
            if (*pQMIndex == (pQM -> index))
            {
                if (strncmp((char *) pQMName, pQM -> pQMName, pQM -> lengthOfQMName) == 0)
                {
                    foundQM = true;
                    break;
                }
            }            
            pQM = pQM -> pNext;
        }

        if (!foundQM)
        {
            /* Rollback this transaction because we can't find its  */
            /* queue manager.                                       */
            rc = ismc_rollbackGlobalTransaction(pSession, &(pXIDLE -> xid));
            TRACE(MQC_ISMC_TRACE, "ismc_rollbackGlobalTransaction rc(%d)\n", rc);
            if (rc)
                goto MOD_EXIT;
        }

        pthread_mutex_unlock(&mqcMQConnectivity.qmListMutex);
        lockedQMListMutex = false;

        /* Re-scan the list looking for XIDs associated with a rule   */
        /* that no longer exists.                                     */

        /* Walk the rule list looking for this XID's rule. If we      */
        /* _don't_ find it, then we have to clear up the transaction. */
        pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);
        lockedRuleListMutex = true;

        pRule = mqcMQConnectivity.pRuleList;
        while (pRule != NULL)
        {
            if (*pRuleIndex == (pRule -> index))
            {
                foundQM = true;
                break;
            }            
            pRule = pRule -> pNext;
        }

        if (!foundRule)
        {
            /* Resolve this transaction since we no longer have a     */
            /* rule for it.                                           */

            /* At this point we want to connect to the relevant queue */
            /* manager and resolve this transaction in whatever way   */
            /* we would have done  */

        }

        pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);
        lockedRuleListMutex = false;

        /* Delete the element */
        pIMA -> pRecoveredXIDs_IMA = pXIDLE -> pNext;
        mqc_free(pXIDLE);
        pXIDLE = pIMA -> pRecoveredXIDs_IMA;
    }

MOD_EXIT:
    if (pSession)
    {
        rc = ismc_closeSession(pSession);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", rc);
        if (rc == 0) 
        {
            rc = ismc_free(pSession);
            TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", rc);
        }
    }

    if (lockedRuleListMutex)
    {
        pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);
        lockedRuleListMutex = false;
    }

    if (lockedQMListMutex)
    {
        pthread_mutex_unlock(&mqcMQConnectivity.qmListMutex);
        lockedQMListMutex = false;        
    }

    if (lockedRecoverMutex)
    {
        pthread_mutex_unlock(&(pIMA -> xa_recoverMutex));
        lockedRecoverMutex = false;
    }

    TRACE(MQC_FNC_TRACE,
            FUNCTION_EXIT "rc=%d\n",
            __func__,
            rc);
    return rc;
}

int mqc_checkIMAXIDsForQM(mqcQM_t * pQM, bool rollbackXIDs)
{
    int i, exitrc, rc = 0;
    int qmindex = pQM -> index;
    ismc_session_t * pSess = NULL;
    int transacted, ackmode;
    bool locked = false;
    ism_xid_t xids[XID_COUNT];
    int recoveredXIDCount = 0;
    int flags = TMSTARTRSCAN;
    bool first = true;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d\n", __func__, qmindex);

    while ((mqcMQConnectivity.flags & MQC_DISABLED) ||
           (mqcMQConnectivity.flags & MQC_RECONNECTING))
    {
        if (first)
        {
            mqcMQConnectivity.flags |= MQC_IMMEDIATE_RECONNECT;
            mqc_waitReconnectAttempt();
            first = false;
        }
        else
        {
            rc = RC_MQC_RECONNECT;
            goto MOD_EXIT;
        }
    }

    /* Create session */
    transacted = 1;
    ackmode    = SESSION_TRANSACTED_GLOBAL;

    pSess = ismc_createSession(mqcMQConnectivity.ima.pConn, transacted, ackmode);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession pSess <%p>\n", pSess);

    if (!pSess)
    {
        rc = mqc_ISMCError("ismc_createSession");
        goto MOD_EXIT;
    }

    /* Recover global transactions */
    pthread_mutex_lock(&mqcMQConnectivity.ima.xa_recoverMutex);
    locked = true;

    do
    {
        rc = ismc_recoverGlobalTransactions(pSess, xids, XID_COUNT, flags);
        TRACE(MQC_MQAPI_TRACE, "ismc_recoverGlobalTransactions rc(%d)\n", rc);

        if (rc < 0)
        {
            rc = mqc_ISMCError("ismc_recoverGlobalTransactions");
            goto MOD_EXIT;
        }

        recoveredXIDCount = rc;

        for (i = 0; i < recoveredXIDCount; i++)
        {
            ism_xid_t *pRecoverXid = &xids[i];

            MQC_TRACE_XID(MQC_MQAPI_TRACE, "pRecoverXid ", pRecoverXid);

            if (mqc_localXID(pRecoverXid, pQM, -1))
            {
                if (rollbackXIDs)
                {
                    /* Rollback XID */
                    rc = ismc_rollbackGlobalTransaction(pSess, pRecoverXid);
                    TRACE(MQC_ISMC_TRACE, "ismc_rollbackGlobalTransaction rc(%d)\n", rc);
    
                    if (rc)
                    {
                        rc = mqc_ISMCError("ismc_rollbackGlobalTransaction");
                        goto MOD_EXIT;
                    }
                }
                else
                {
                    rc = RC_UNRESOLVED;
                    goto MOD_EXIT;
                }
            }
        }

        if (recoveredXIDCount < XID_COUNT)
        {
            rc = 0;
            break;
        }

        flags = TMNOFLAGS;

    } while (rc != 0);

MOD_EXIT:

    if (locked)
    {
        pthread_mutex_unlock(&mqcMQConnectivity.ima.xa_recoverMutex);
    }

    if (pSess)
    {
        exitrc = ismc_closeSession(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", exitrc);
        if (exitrc == 0)
        {
            exitrc = ismc_free(pSess);
            TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", exitrc);
        }
        else
        {
            mqc_ISMCError("ismc_closeSession");
        }
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d rc=%d\n", __func__, qmindex, rc);
    return rc;
}

