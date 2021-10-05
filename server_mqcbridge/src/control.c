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
/// @file  control.c
/// @brief MQ Connectivity component rule control functions
//****************************************************************************

#include <mqcInternal.h>
#include <linux/futex.h>
#include <sys/syscall.h>

extern void ism_common_initTrace();

/* Static variables */
static bool Initialized = false;

/* Function prototypes */
int mqc_startXAThread(mqcRuleQM_t * pRuleQM);
int mqc_waitXAThread(mqcRuleQM_t * pRuleQM, bool requested);
int mqc_startReconnectThread(void);
int mqc_waitReconnectThread(void);

int mqc_startIMAXAConsumerThread(mqcRuleQM_t * pRuleQM);
int mqc_waitIMAXAConsumerThread(mqcRuleQM_t * pRuleQM, bool requested);

int mqc_startRuleIndex(int ruleindex)
{
    int rc = 0;
    mqcRule_t * pRule;
    bool found = false;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d\n", __func__, ruleindex);

    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);
    pRule = mqcMQConnectivity.pRuleList;

    while (pRule)
    {
        if (pRule -> index == ruleindex)
        {
            found = true;
            rc = mqc_startRule(pRule);
            break;
        }

        pRule = pRule -> pNext;
    }
    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);

    if (found == false)
    {
        TRACE(MQC_NORMAL_TRACE, "Unable to find rule with index '%d'\n", ruleindex);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d rc=%d\n", __func__, ruleindex, rc);
    return rc;
}

int mqc_termRuleIndex(int ruleindex)
{
    int rc = 0;
    mqcRule_t * pRule;
    bool found = false;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d\n", __func__, ruleindex);

    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);
    pRule = mqcMQConnectivity.pRuleList;

    while (pRule)
    {
        if (pRule -> index == ruleindex)
        {
            found = true;
            rc = mqc_termRule(pRule,
                              true); /* user requested */
            break;
        }

        pRule = pRule -> pNext;
    }
    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);

    if (found == false)
    {
        TRACE(MQC_NORMAL_TRACE, "Unable to find rule with index '%d'\n", ruleindex);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d rc=%d\n", __func__, ruleindex, rc);
    return rc;
}

//****************************************************************************
/// @brief Start a rule.
///
/// @param[in]     pRule          Pointer to internal rule structure
///
/// @retval 0
///
/// @remark On successful completion the rule, one or more ruleQMs, or
///         MQ Connectivity might be in reconnect.
//****************************************************************************
int mqc_startRule(mqcRule_t * pRule)
{
    int rc = 0;
    mqcRuleQM_t * pRuleQM;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if ((mqcMQConnectivity.flags & MQC_DISABLED) ||
        (mqcMQConnectivity.flags & MQC_RECONNECTING))
    {
        rc = RC_MQC_IMMEDIATE_RECONNECT;
        goto MOD_EXIT;
    }
    else if (pRule -> flags & RULE_DISABLED)
    {
        if (pRule -> flags & RULE_RECONNECT)
        {
            pRule -> flags &= ~RULE_RECONNECT;
            pRule -> flags |=  RULE_RECONNECTING;
        }

        pRule -> flags &= ~RULE_DISABLED;
    }
    else
    {
        TRACE(MQC_NORMAL_TRACE, "Rule already started\n");
        goto MOD_EXIT;
    }

    if (IMA_CONSUME_RULE(pRule))
    {
        /* Initialize Rule XA flags (reconnect) */
        pRule -> flags &= ~RULE_IMA_XA_STARTED;
        pRule -> flags &= ~RULE_IMA_XA_COMMIT;

        rc = mqc_createRuleConsumerIMA(pRule);
        if (rc)
        {
            /* Make sure none of the ruleQMs are marked STARTING */
            pthread_mutex_lock(&pRule -> ruleQMListMutex);
            pRuleQM = pRule -> pRuleQMList;

            while (pRuleQM)
            {
                pthread_mutex_lock(&pRuleQM -> controlMutex);
                pRuleQM -> flags &= ~RULEQM_STARTING;
                pthread_mutex_unlock(&pRuleQM -> controlMutex);
                pRuleQM = pRuleQM -> pNext;
            }

            pthread_mutex_unlock(&pRule -> ruleQMListMutex);

            goto MOD_EXIT;
        }
    }

    pthread_mutex_lock(&pRule -> ruleQMListMutex);
    pRuleQM = pRule -> pRuleQMList;

    while (pRuleQM)
    {
        rc = mqc_startRuleQM(pRuleQM);

        if (rc)
        {
            /* Make sure none of the ruleQMs are marked STARTING */
            while(pRuleQM)
            {
                pthread_mutex_lock(&pRuleQM -> controlMutex);
                pRuleQM -> flags &= ~RULEQM_STARTING;
                pthread_mutex_unlock(&pRuleQM -> controlMutex);
                pRuleQM = pRuleQM -> pNext;
            }

            pthread_mutex_unlock(&pRule -> ruleQMListMutex);
            goto MOD_EXIT;
        }

        pRuleQM = pRuleQM -> pNext;
    }

    pthread_mutex_unlock(&pRule -> ruleQMListMutex);

    /* Mark ourselves as reconnected */
    if (pRule -> flags & RULE_RECONNECTING)
    {
        rc = mqc_ruleReconnected(pRule);
        if (rc) goto MOD_EXIT;
    }

MOD_EXIT:

    if (rc == RC_MQC_RECONNECT)
    {
        mqcMQConnectivity.flags |= MQC_RECONNECT;
        mqc_wakeReconnectThread();
        rc = 0;
    }
    else if (rc == RC_MQC_IMMEDIATE_RECONNECT)
    {
        mqcMQConnectivity.flags |= MQC_IMMEDIATE_RECONNECT;
        mqc_wakeReconnectThread();
        rc = 0;
    }
    else if (rc)
    {
        mqc_termRule(pRule,
                     false); /* user requested */

        if (rc == RC_RULE_RECONNECT)
        {
            pRule -> flags |= RULE_RECONNECT;
            mqc_wakeReconnectThread();
            rc = 0;
        }
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief Terminate a rule.
///
/// @param[in]     pRule          Pointer to internal rule structure
/// @param[in]     requested      Whether the terminate was user requested
///
/// @retval 0
//****************************************************************************
int mqc_termRule(mqcRule_t * pRule, bool requested)
{
    int rc = 0;
    mqcRuleQM_t * pRuleQM;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&pRule -> ruleQMListMutex);
    pRuleQM = pRule -> pRuleQMList;

    while (pRuleQM)
    {
        mqc_termRuleQM(pRuleQM,
                       requested); /* user requested */

        pRuleQM = pRuleQM -> pNext;
    }

    pthread_mutex_unlock(&pRule -> ruleQMListMutex);

    if (IMA_CONSUME_RULE(pRule))
    {
        mqc_deleteRuleConsumerIMA(pRule, requested);
    }

    time(&pRule -> disabledTime);

    pRule -> flags &= ~RULE_RECONNECT;
    pRule -> flags &= ~RULE_RECONNECTING;

    pRule -> flags &= ~RULE_DISABLE;
    pRule -> flags &= ~RULE_USER_DISABLE;
    pRule -> flags |= RULE_DISABLED;

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief Start a ruleQM.
///
/// @param[in]     pRuleQM        Pointer to internal ruleQM structure
/// 
/// @retval 0
//****************************************************************************
int mqc_startRuleQM(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    pthread_mutex_lock(&pRuleQM -> controlMutex);

    if (pRuleQM -> flags & RULEQM_DISABLED)
    {
        if (pRuleQM -> flags & RULEQM_RECONNECT)
        {
            pRuleQM -> flags &= ~RULEQM_RECONNECT;
            pRuleQM -> flags |=  RULEQM_RECONNECTING;
        }
        else
        {
            pRuleQM -> flags |= RULEQM_STARTING;
            memset(pRuleQM -> errorInsert, 0, sizeof(pRuleQM -> errorInsert));
            pRuleQM -> errorMsgId = 0;
        }

        pRuleQM -> flags &= ~RULEQM_DISABLED;

    }
    else
    {
        TRACE(MQC_NORMAL_TRACE, "RuleQM already started\n");
        goto MOD_EXIT;
    }

    /* Initialize RuleQM flags and batch counts (reconnect) */
    pRuleQM -> flags &= (RULEQM_STARTING | RULEQM_RECONNECTING);

    pRuleQM -> batchMsgsSent = 0;
    pRuleQM -> batchDataSent = 0;

    if (MQ_CONSUME_RULE(pRule))
    {
        /* Start xa subscribe thread */
        rc = mqc_startXAThread(pRuleQM);
        if (rc) goto MOD_EXIT;
    }
    else
    {
        rc = mqc_startIMAXAConsumerThread(pRuleQM);
        if (rc) goto MOD_EXIT;
    }

MOD_EXIT:

    if (rc)
    {
        mqc_termRuleQM(pRuleQM,
                       false); /* user requested */

        if (rc == RC_RULEQM_RECONNECT)
        {
            pRuleQM -> flags |= RULEQM_RECONNECT;
            mqc_wakeReconnectThread();
            rc = 0;
        }
    }

    pthread_mutex_unlock(&pRuleQM -> controlMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

//****************************************************************************
/// @brief Terminate a ruleQM.
///
/// @param[in]     pRuleQM        Pointer to internal ruleQM structure
/// @param[in]     requested      Whether the terminate was user requested
/// 
/// @retval 0
//****************************************************************************
int mqc_termRuleQM(mqcRuleQM_t * pRuleQM, bool requested)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    pthread_mutex_lock(&pRuleQM -> controlMutex);

    if (MQ_CONSUME_RULE(pRule))
    {
        /* Wait for xa thread to end */
        mqc_waitXAThread(pRuleQM, requested);
    }
    else
    {
        /* End the mqc_IMAXAConsumerThread, if any */
        mqc_waitIMAXAConsumerThread(pRuleQM, requested);
    }

    time(&pRuleQM -> disabledTime);

    pRuleQM -> flags &= ~RULEQM_RECONNECT;
    pRuleQM -> flags &= ~RULEQM_RECONNECTING;

    pRuleQM -> flags &= ~RULEQM_DISABLE;
    pRuleQM -> flags &= ~RULEQM_USER_DISABLE;
    pRuleQM -> flags |=  RULEQM_DISABLED;

    pthread_mutex_unlock(&pRuleQM -> controlMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

//****************************************************************************
/// @brief Start MQ Connectivity.
///
/// @param[in]     dynamicTraceLevel
///
/// @retval 0
//****************************************************************************
int mqc_start(int dynamicTraceLevel)
{
    int rc = 0;
    mqcRule_t * pRule;
    bool resetTraceLevel = true;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.controlMutex);

    if (Initialized == false)
    {
        sigignore(SIGPIPE);

        // Use the correct installation path for MQ.
        mqcMQConnectivity.MQInstallationPath = getenv("MQ_INSTALLATION_PATH");

        // Default to /opt/mqm
        if (mqcMQConnectivity.MQInstallationPath == NULL)
        {
            mqcMQConnectivity.MQInstallationPath = "/opt/mqm";
        }

        /* Ensure we have a single ismc delivery thread */
        putenv("ISMMaxJMSDelThreads=1");

        /* Technically these two pthread_cond_t variables have already been initialyzed with the static default initialyzer, */
        /* but we believe it's safe to reinitialyze them since they haven't been used yet.  */
        rc = ism_common_cond_init_monotonic(&mqcMQConnectivity.reconnectCond);
        if (rc) goto MOD_EXIT;

        rc = ism_common_cond_init_monotonic(&mqcMQConnectivity.immediateCond);
        if (rc) goto MOD_EXIT;
        
        /* Start reconnect thread */
        rc = mqc_startReconnectThread();
        if (rc) goto MOD_EXIT;

        /* Load IMA */
        rc = mqc_loadIMA(&mqcMQConnectivity.ima);
        if (rc) goto MOD_EXIT;

        /* Load tuning parameters */
        rc = mqc_loadTuning();
        if (rc) goto MOD_EXIT;

        /* Load qms */
        rc = mqc_loadQMs(&mqcMQConnectivity.pQMList);
        if (rc) goto MOD_EXIT;

        /* Load rules */
        rc = mqc_loadRules(&mqcMQConnectivity.pRuleList,
                           mqcMQConnectivity.pQMList);
        if (rc) goto MOD_EXIT;

        Initialized = true;

        TRACE(5, "" IMA_PRODUCTNAME_FULL " MQ Connectivity service is initialized.\n");

    }

    if (mqcMQConnectivity.flags & MQC_DISABLED)
    {
        if (mqcMQConnectivity.flags & MQC_RECONNECT)
        {
            mqcMQConnectivity.flags &= ~MQC_RECONNECT;
            mqcMQConnectivity.flags |=  MQC_RECONNECTING;
        }

        mqcMQConnectivity.flags &= ~MQC_DISABLED;
    }
    else
    {
        TRACE(MQC_NORMAL_TRACE, "Already connected to IMA\n");
        goto MOD_EXIT;
    }

    /* Connect to IMA */
    rc = mqc_connectIMA();
    if (rc) goto MOD_EXIT;

    /* Load records from store -
       Our cached values need updating on reconnect */
    rc = mqc_loadRecords();
    if (rc) goto MOD_EXIT;

    /* Get unique object id (based on MAC address) */
    rc = mqc_setObjectIdentifier();
    if (rc) goto MOD_EXIT;

    /* Display whatever durable subscriptions the server already knows about. */
    rc = mqc_displayRecoveredSubscriptions();
    if (rc) goto MOD_EXIT;

    /* Mark ourselves as reconnected */
    if (mqcMQConnectivity.flags & MQC_RECONNECTING)
    {
        rc = mqc_reconnected();
        if (rc) goto MOD_EXIT;
    }

    if (dynamicTraceLevel >= 0)
    {
        TRACE(MQC_NORMAL_TRACE, "Set the trace level to %d\n", dynamicTraceLevel);
		ism_common_setTraceLevel(dynamicTraceLevel);
        mqc_setMQTrace(dynamicTraceLevel);
        resetTraceLevel = false;
    }

    /* Start each per rule qm */
    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);
    pRule = mqcMQConnectivity.pRuleList;

    while (pRule)
    {
        if (pRule -> Enabled)
        {
            rc = mqc_startRule(pRule);
            if (rc)
            {
                pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);
                goto MOD_EXIT;
            }
        }

        pRule = pRule -> pNext;
    }
    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);

MOD_EXIT:

    /* An error in the preceding code can cause us to skip the resetting of */
    /* the trace level, in which case do it here instead. (We had to make   */
    /* the attempt earlier so that it is done before message passing        */
    /* starts - if all goes well.)                                          */
    if (resetTraceLevel)
    {
        if (dynamicTraceLevel >= 0)
        {
            TRACE(MQC_NORMAL_TRACE, "Set the trace level to %d\n", dynamicTraceLevel);
            ism_common_setTraceLevel(dynamicTraceLevel);
            mqc_setMQTrace(dynamicTraceLevel);
            resetTraceLevel = false;
        }
    }

    if (rc == RC_MQC_RECONNECT)
    {
        time(&mqcMQConnectivity.disabledTime);
        mqcMQConnectivity.flags |= MQC_RECONNECT;
        mqc_wakeReconnectThread();
        rc = 0;
    }

    pthread_mutex_unlock(&mqcMQConnectivity.controlMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

//****************************************************************************
/// @brief Terminate MQ Connectivity.
///
/// @param[in]     requested      Whether the terminate was user requested
///
/// @retval 0
//****************************************************************************
int mqc_term(bool requested)
{
    int rc = 0;
    mqcRule_t * pRule;
    mqcRuleQM_t * pRuleQM;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "requested=%d\n", __func__, (int)requested);

    pthread_mutex_lock(&mqcMQConnectivity.controlMutex);

    if (mqcMQConnectivity.flags & MQC_DISABLED)
    {
        TRACE(MQC_NORMAL_TRACE, "Already disconnected from IMA\n");
        goto MOD_EXIT;
    }

    if (requested)
    {
        /* Wait for reconnnect thread to end */
        mqc_waitReconnectThread();
    }

    /* Set each rule to disable (notifies xathreads) */
    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);
    pRule = mqcMQConnectivity.pRuleList;

    while (pRule)
    {
        pRuleQM = pRule -> pRuleQMList;

        while (pRuleQM)
        {
            if (!(pRuleQM -> flags & RULEQM_DISABLED))
            {
                pRuleQM -> flags |= (requested) ?
                    RULEQM_USER_DISABLE : RULEQM_DISABLE;
            }

            pRuleQM = pRuleQM -> pNext;
        }

        pRule = pRule -> pNext;
    }
    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);

    /* Terminate each rule */
    pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);
    pRule = mqcMQConnectivity.pRuleList;

    while (pRule)
    {
        mqc_termRule(pRule,
                     requested); /* user requested */
        pRule = pRule -> pNext;
    }
    pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);

    /* Disconnect from IMA */
    mqc_disconnectIMA();

    if (requested)
    {
        mqc_setMQTrace(0);
    }

    mqcMQConnectivity.flags &= ~MQC_DISABLE;
    mqcMQConnectivity.flags |=  MQC_DISABLED;

MOD_EXIT:
    pthread_mutex_unlock(&mqcMQConnectivity.controlMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_startXAThread(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    char threadname[32];

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    sprintf(threadname, "xa.%d.%d.%d", ruleindex, qmindex, pRuleQM -> index);
    rc = ism_common_startThread(&pRuleQM -> xa_thread_handle,
                                mqc_MQXAConsumerThread,
                                pRuleQM,
                                NULL,
                                0,
                                ISM_TUSAGE_NORMAL,
                                0,
                                threadname,
                                "mqc_MQXAConsumerThread");
    TRACE(MQC_NORMAL_TRACE, "ism_common_startThread rc(%d)\n", rc);

    if (rc)
    {
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_waitXAThread(mqcRuleQM_t * pRuleQM, bool requested)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if (pRuleQM -> xa_thread_started)
    {
        pRuleQM -> flags |= (requested) ?
            RULEQM_USER_DISABLE : RULEQM_DISABLE;

        /* Cancel our MQGET(WAIT) */
        mqc_cancelWaiterQM(pRuleQM);

        pthread_mutex_unlock(&pRuleQM -> controlMutex);

        rc = ism_common_joinThread(pRuleQM -> xa_thread_handle, NULL);
        TRACE(MQC_NORMAL_TRACE, "ism_common_joinThread rc(%d)\n", rc);

        pthread_mutex_lock(&pRuleQM -> controlMutex);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_startReconnectThread(void)
{
    int rc = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (mqcMQConnectivity.reconnect_thread_started == true)
    {
        TRACE(MQC_NORMAL_TRACE, "Reconnect thread already started\n");
        goto MOD_EXIT;
    }

    rc = ism_common_startThread(&mqcMQConnectivity.reconnect_thread_handle,
                                mqc_reconnectThread,
                                NULL,
                                NULL,
                                0,
                                ISM_TUSAGE_NORMAL,
                                0,
                                "reconnect",
                                "mqc_reconnectThread");
    TRACE(MQC_NORMAL_TRACE, "ism_common_startThread rc(%d)\n", rc);

    if (rc == 0)
    {
        mqcMQConnectivity.reconnect_thread_started = true;
    }

MOD_EXIT:

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_waitReconnectThread(void)
{
    int rc = 0;
    void * retval;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (mqcMQConnectivity.reconnect_thread_started)
    {
        pthread_mutex_unlock(&mqcMQConnectivity.controlMutex);

        mqcMQConnectivity.flags |= MQC_DISABLE;

        rc = ism_common_joinThread(mqcMQConnectivity.reconnect_thread_handle, &retval);
        TRACE(MQC_NORMAL_TRACE, "ism_common_joinThread rc(%d)\n", rc);

        mqcMQConnectivity.reconnect_thread_started = false;

        pthread_mutex_lock(&mqcMQConnectivity.controlMutex);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_startIMAXAConsumerThread(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;
    char threadname[32];

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    sprintf(threadname, "xa.%d.%d.%d", ruleindex, qmindex, pRuleQM -> index);
    rc = ism_common_startThread(&pRuleQM -> xa_thread_handle,
                                mqc_IMAXAConsumerThread,
                                pRuleQM,
                                NULL,
                                0,
                                ISM_TUSAGE_NORMAL,
                                0,
                                threadname,
                                "mqc_IMAXAConsumerThread");
    TRACE(MQC_NORMAL_TRACE, "ism_common_startThread rc(%d)\n", rc);

    if (rc)
    {
        rc = RC_RULEQM_RECONNECT;
        goto MOD_EXIT;
    }

MOD_EXIT:
    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_waitIMAXAConsumerThread(mqcRuleQM_t * pRuleQM, bool requested)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    if (pRuleQM -> xa_thread_started)
    {
        pRuleQM -> flags |= (requested) ? RULEQM_USER_DISABLE : RULEQM_DISABLE;
    
        /* Use futex wake to tell the thread to stop. We wake all the threads */
        /* since the receive bit is a property of the rule rather than the    */
        /* ruleQM. If we are stopping the whole bridge then it doesn't matter */
        /* and if not then the other threads will see this as a spurious wake */
        /* and re-enter a futex wait.                                         */
        syscall(SYS_futex,
                &(pRule -> receiveActive),
                FUTEX_WAKE,        /* operation */
                (pRule -> ruleQMCount), /* wake all threads */
                NULL,              /* timeout - ignored */
                NULL,              /* uaddr2 - ignored  */
                0);                /* val3 - ignored    */
    
        rc = ism_common_joinThread(pRuleQM -> xa_thread_handle, NULL);
        TRACE(MQC_NORMAL_TRACE, "ism_common_joinThread rc(%d)\n", rc);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

