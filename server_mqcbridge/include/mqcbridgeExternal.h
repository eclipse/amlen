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

#include <transport.h>

/* Initial version of a function to return monitoring information to  */
/* the UI and CLI.                                                    */

typedef enum _mqcRuleStatus_t {
    mqcRule_Enabled = 0,
    mqcRule_Disabled = 1,
    mqcRule_Reconnecting = 2,
    mqcRule_Starting = 3
} mqcRuleStatus_t;

typedef struct _mqcStatistics_t {
    struct _mqcStatistics_t *pNext;
    char *RuleName;
    char *QueueManagerConnection;
    uint64_t nonpersistentCount;
    uint64_t persistentCount;
    uint64_t commitCount;
    uint64_t rollbackCount;
    uint64_t committedMessageCount;

    /* Status reporting for rule/QM pairs. */
    /* If status is 0 then the rule/QM pair is operating normally and */
    /* none of the subsequent values are meaningful.                  */
    /* If status is greater than 0 then                               */
    /* MQConnectivityRC is the message code for MQ Connectivity       */
    /*   identifying a message in ism_mqcbridge_tms.xml               */
    /* OtherRC is the associated reason code from MQ or XA            */
    /*   (Zero if there isn't one.)                                   */
    /* pReasonText is the text form of the symbol associated with the */
    /*   OtherRC eg "MQRC_Q_FULL", "XAER_DUPID"                       */
    /* pQueueName is the name of the queue associated with an error.  */
    /*   It has to be supplied because it might be a queue created by */
    /*   MQ Connectivity eg a sync queue.                             */
    /* pInsertn is a pointer to a text string insert for the MQCrc    */
    /*   message (up to 4).                                           */
    /* pExpandedMessage is a pointer to the fully expanded English    */
    /*   version of the message.                                      */

    uint32_t status;
    int32_t MQConnectivityRC; /* MQ Connectivity return code */
    int32_t OtherRC; /* MQLONG in cmqc.h or int for XA */
    char *pOtherRCSymbol;
    char *pErrorQueueName;
    char *pSubName;
    char *pErrorAPI;
    char *pExpandedMessage;
} mqcStatistics_t;
