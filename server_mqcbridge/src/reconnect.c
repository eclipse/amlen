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

#define MIN(a,b) (((a)<(b))?(a):(b))
#define TRIM_LOOP_COUNT 30
#define TRIM_PADDING (16 * 1024 * 1024)

void * mqc_reconnectThread(void * parm, void * context, int value)
{
    int rc = 0;
    mqcRule_t * pRule;
    mqcRuleQM_t * pRuleQM;
    time_t now, elapsed;
    bool reconnect;
    struct timespec ts={1,0};
    int trimLoopCount = TRIM_LOOP_COUNT;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    while (1)
    {
        pthread_mutex_lock(&mqcMQConnectivity.controlMutex);

        if (mqcMQConnectivity.flags & MQC_DISABLE)
        {
            pthread_mutex_unlock(&mqcMQConnectivity.controlMutex);
            break;
        }

        if (mqcMQConnectivity.flags & MQC_RECONNECT)
        {
            if (mqcMQConnectivity.flags & MQC_DISABLED)
            {
                time(&now);
                elapsed = now - mqcMQConnectivity.disabledTime;
                reconnect = false;

                TRACE(MQC_NORMAL_TRACE,
                      "MQC: elapsed(%ld) immediate %d:%d short(%d) %d:%d long(%d) %d:%d\n",
                      elapsed,
                      mqcMQConnectivity.immediateRetries, mqcMQConnectivity.immediateRetryCount,
                      mqcMQConnectivity.shortRetryInterval,
                      mqcMQConnectivity.shortRetries, mqcMQConnectivity.shortRetryCount,
                      mqcMQConnectivity.longRetryInterval,
                      mqcMQConnectivity.longRetries, mqcMQConnectivity.longRetryCount);

                if (mqcMQConnectivity.immediateRetries < mqcMQConnectivity.immediateRetryCount)
                {
                    mqcMQConnectivity.immediateRetries++;
                    reconnect = true;
                }
                else if (mqcMQConnectivity.shortRetries < mqcMQConnectivity.shortRetryCount)
                {
                    if (elapsed >= mqcMQConnectivity.shortRetryInterval)
                    {
                        mqcMQConnectivity.shortRetries++;
                        reconnect = true;
                    }
                }
                else if ((mqcMQConnectivity.longRetries < mqcMQConnectivity.longRetryCount) ||
                         (mqcMQConnectivity.longRetryCount == MQC_UNLIMITED))
                {
                    if (elapsed >= mqcMQConnectivity.longRetryInterval)
                    {
                        mqcMQConnectivity.longRetries++;
                        reconnect = true;
                    }

                    if (mqcMQConnectivity.longRetries == mqcMQConnectivity.longRetryCount)
                    {
                        TRACE(MQC_NORMAL_TRACE, "No long retry limit for IMA reconnect\n");
                        mqcMQConnectivity.longRetries = 0;
                    }
                }

                if (mqcMQConnectivity.flags & MQC_IMMEDIATE_RECONNECT)
                {
                    mqcMQConnectivity.flags &= ~MQC_IMMEDIATE_RECONNECT;
                    reconnect = true;
                }

                if (reconnect)
                {
                    mqc_start(-1); /* dynamic trace level */

                    pthread_mutex_lock(&mqcMQConnectivity.immediateMutex);
                    pthread_cond_signal(&mqcMQConnectivity.immediateCond);
                    pthread_mutex_unlock(&mqcMQConnectivity.immediateMutex);
                }
            }
            else
            {
                mqc_term(false); /* user requested */
                time(&mqcMQConnectivity.disabledTime);

                pthread_mutex_unlock(&mqcMQConnectivity.controlMutex);
                continue;
            }
        }

        pthread_mutex_unlock(&mqcMQConnectivity.controlMutex);

        pthread_mutex_lock(&mqcMQConnectivity.ruleListMutex);
        pRule = mqcMQConnectivity.pRuleList;

        while (pRule)
        {
            /* Rule reconnect */
            if (pRule -> flags & RULE_RECONNECT)
            {
                time(&now);
                elapsed = now - pRule -> disabledTime;
                reconnect = false;

                TRACE(MQC_NORMAL_TRACE,
                      "RULE %d: elapsed(%ld) immediate %d:%d short(%d) %d:%d long(%d) %d:%d\n",
                      pRule -> index,
                      elapsed,
                      pRule -> immediateRetries, mqcMQConnectivity.immediateRetryCount,
                      mqcMQConnectivity.shortRetryInterval,
                      pRule -> shortRetries, mqcMQConnectivity.shortRetryCount,
                      mqcMQConnectivity.longRetryInterval,
                      pRule -> longRetries, mqcMQConnectivity.longRetryCount);

                if (pRule -> immediateRetries < mqcMQConnectivity.immediateRetryCount)
                {
                    pRule -> immediateRetries++;
                    reconnect = true;
                }
                else if (pRule -> shortRetries < mqcMQConnectivity.shortRetryCount)
                {
                    if (elapsed >= mqcMQConnectivity.shortRetryInterval)
                    {
                        pRule -> shortRetries++;
                        reconnect = true;
                    }
                }
                else if ((pRule -> longRetries < mqcMQConnectivity.longRetryCount) ||
                         (mqcMQConnectivity.longRetryCount == MQC_UNLIMITED))
                {
                    if (elapsed >= mqcMQConnectivity.longRetryInterval)
                    {
                        pRule -> longRetries++;
                        reconnect = true;
                    }
                }
                else
                {
                    TRACE(MQC_NORMAL_TRACE, "Retry limit reached for Rule reconnect\n");
                    pRule -> immediateRetries = 0;
                    pRule -> shortRetries = 0;
                    pRule -> longRetries = 0;
                    pRule -> flags &= ~RULE_RECONNECT;
                }

                if (reconnect)
                {
                    mqc_startRule(pRule);
                }
            }

            /* RuleQM reconnect */
            pthread_mutex_lock(&pRule -> ruleQMListMutex);
            pRuleQM = pRule -> pRuleQMList;
            reconnect = false;

            while (pRuleQM)
            {
                pthread_mutex_lock(&pRuleQM -> controlMutex);

                if (pRuleQM -> flags & RULEQM_RECONNECT)
                {
                    time(&now);
                    elapsed = now - pRuleQM -> disabledTime;

                    TRACE(MQC_NORMAL_TRACE,
                          "RULEQM %d.%d: elapsed(%ld) immediate %d:%d short(%d) %d:%d long(%d) %d:%d\n",
                          pRule -> index, pRuleQM -> pQM -> index,
                          elapsed,
                          pRuleQM -> immediateRetries, mqcMQConnectivity.immediateRetryCount,
                          mqcMQConnectivity.shortRetryInterval,
                          pRuleQM -> shortRetries, mqcMQConnectivity.shortRetryCount,
                          mqcMQConnectivity.longRetryInterval,
                          pRuleQM -> longRetries, mqcMQConnectivity.longRetryCount);

                    if (pRuleQM -> immediateRetries < mqcMQConnectivity.immediateRetryCount)
                    {
                        pRuleQM -> immediateRetries++;
                        reconnect = true;
                    }
                    else if (pRuleQM -> shortRetries < mqcMQConnectivity.shortRetryCount)
                    {
                        if (elapsed >= mqcMQConnectivity.shortRetryInterval)
                        {
                            pRuleQM -> shortRetries++;
                            reconnect = true;
                        }
                    }
                    else if ((pRuleQM -> longRetries < mqcMQConnectivity.longRetryCount) ||
                             (mqcMQConnectivity.longRetryCount == MQC_UNLIMITED))
                    {
                        if (elapsed >= mqcMQConnectivity.longRetryInterval)
                        {
                            pRuleQM -> longRetries++;
                            reconnect = true;
                        }
                    }
                    else
                    {
                        TRACE(MQC_NORMAL_TRACE, "Retry limit reached for RuleQM reconnect\n");
                        pRuleQM -> immediateRetries = 0;
                        pRuleQM -> shortRetries = 0;
                        pRuleQM -> longRetries = 0;
                        pRuleQM -> flags &= ~RULEQM_RECONNECT;
                    }

                    if (reconnect)
                    {
                        mqc_startRuleQM(pRuleQM);
                    }
                }

                pthread_mutex_unlock(&pRuleQM -> controlMutex);

                pRuleQM = pRuleQM -> pNext;
            }
            pthread_mutex_unlock(&pRule -> ruleQMListMutex);

            pRule = pRule -> pNext;
        }
        pthread_mutex_unlock(&mqcMQConnectivity.ruleListMutex);

        if (--trimLoopCount <= 0)
        {
        	trimLoopCount = TRIM_LOOP_COUNT;
            TRACE(MQC_NORMAL_TRACE, "Executing malloc_trim\n");
            malloc_trim(TRIM_PADDING);
        }

        /* Wait for upto 1 second */

        pthread_mutex_lock(&mqcMQConnectivity.reconnectMutex);
        ism_common_cond_timedwait(&mqcMQConnectivity.reconnectCond,
                                  &mqcMQConnectivity.reconnectMutex, &ts, 1);
        pthread_mutex_unlock(&mqcMQConnectivity.reconnectMutex);
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return NULL;
}

int mqc_wakeReconnectThread(void)
{
    int rc = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pthread_mutex_lock(&mqcMQConnectivity.reconnectMutex);
    pthread_cond_signal(&mqcMQConnectivity.reconnectCond);
    pthread_mutex_unlock(&mqcMQConnectivity.reconnectMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_ruleReconnected(mqcRule_t * pRule)
{
    int rc = 0;
    int ruleindex = pRule -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d\n", __func__, ruleindex);

    pRule -> flags &= ~RULE_RECONNECTING;
    pRule -> immediateRetries = 0;
    pRule -> shortRetries = 0;
    pRule -> longRetries = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d rc=%d\n", __func__, ruleindex, rc);
    return rc;
}

int mqc_ruleQMReconnected(mqcRuleQM_t * pRuleQM)
{
    int rc = 0;
    mqcRule_t * pRule = pRuleQM -> pRule;
    mqcQM_t * pQM = pRuleQM -> pQM;
    int ruleindex = pRule -> index;
    int qmindex = pQM -> index;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "%d.%d\n", __func__, ruleindex, qmindex);

    pRuleQM -> flags &= ~RULEQM_RECONNECTING;
    pRuleQM -> immediateRetries = 0;
    pRuleQM -> shortRetries = 0;
    pRuleQM -> longRetries = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "%d.%d rc=%d\n", __func__, ruleindex, qmindex, rc);
    return rc;
}

int mqc_reconnected(void)
{
    int rc = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    mqcMQConnectivity.flags &= ~MQC_RECONNECTING;
    mqcMQConnectivity.immediateRetries = 0;
    mqcMQConnectivity.shortRetries = 0;
    mqcMQConnectivity.longRetries = 0;

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

int mqc_waitReconnectAttempt(void)
{
    int rc = 0;
    struct timespec ts={1,0};

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    mqc_wakeReconnectThread();

    /* Wait for upto 1 second */

    pthread_mutex_lock(&mqcMQConnectivity.immediateMutex);
    ism_common_cond_timedwait(&mqcMQConnectivity.immediateCond,
                              &mqcMQConnectivity.immediateMutex, &ts, 1);
    pthread_mutex_unlock(&mqcMQConnectivity.immediateMutex);

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

