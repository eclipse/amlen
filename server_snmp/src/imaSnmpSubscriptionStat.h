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

/*
 * Note: this header file provides definitions for MessageSight server stats,
 *       as well as the interface to access server stats.
 */

#ifndef IMASNMPSUBSCRIPTIONSTAT_H_
#define IMASNMPSUBSCRIPTIONSTAT_H_

#include "imaSnmpColumn.h"


#define MAX_STR_SIZE 256

#define COLUMN_IBMIMASUBSCRIPTION_TABLE_COL_INDEX_SIZE        10
#define COLUMN_IBMIMASUBSCRIPTION_SUBNAME_SIZE               256  //SubName can be 10240. will handle the real size in code
#define COLUMN_IBMIMASUBSCRIPTION_TOPICSTRING_SIZE           256  //TopicString can be 65535. will handle the real size in code
#define COLUMN_IBMIMASUBSCRIPTION_CLIENTID_SIZE              100  //ClientID can be 65535. will handle the real size in code
#define COLUMN_IBMIMASUBSCRIPTION_ISDURABLE_SIZE              10  //str bool
#define COLUMN_IBMIMASUBSCRIPTION_BUFFEREDMSGS_SIZE           10  //int
#define COLUMN_IBMIMASUBSCRIPTION_BUFFEREDMSGSHWM_SIZE        10  //int
#define COLUMN_IBMIMASUBSCRIPTION_BUFFEREDPERCENT_SIZE        20  //float
#define COLUMN_IBMIMASUBSCRIPTION_MAXMESSAGES_SIZE            10  //int
#define COLUMN_IBMIMASUBSCRIPTION_PUBLISHEDMSGS_SIZE          10  //int
#define COLUMN_IBMIMASUBSCRIPTION_REJECTEDMSGS_SIZE           10  //int
#define COLUMN_IBMIMASUBSCRIPTION_BUFFEREDHWMPERCENT_SIZE     20  //float
#define COLUMN_IBMIMASUBSCRIPTION_ISSHARED_SIZE               10  // bool
#define COLUMN_IBMIMASUBSCRIPTION_CONSUMERS_SIZE              10  //int
#define COLUMN_IBMIMASUBSCRIPTION_DISCARDEDMSGS_SIZE          10  //int
#define COLUMN_IBMIMASUBSCRIPTION_EXPIREDMSGS_SIZE            10  //int
#define COLUMN_IBMIMASUBSCRIPTION_MESSAGINGPOLICY_SIZE       100


typedef enum {
    imaSnmpSubscription_NONE                 = 0,
    imaSnmpSubscription_COLINDEX             = 1,
    imaSnmpSubscription_SUBNAME              = 2,
    imaSnmpSubscription_TOPICSTRING          = 3,
    imaSnmpSubscription_CLIENTID             = 4,
    imaSnmpSubscription_ISDURABLE            = 5,
    imaSnmpSubscription_BUFFEREDMSGS         = 6,
    imaSnmpSubscription_BUFFEREDMSGSHWM      = 7,
    imaSnmpSubscription_BUFFEREDPERCENT      = 8,
    imaSnmpSubscription_MAXMESSAGES          = 9,
    imaSnmpSubscription_PUBLISHEDMSGS        = 10,
    imaSnmpSubscription_REJECTEDMSGS         = 11,
    imaSnmpSubscription_BUFFEREDHWMPERCENT   = 12,
    imaSnmpSubscription_ISSHARED             = 13,
    imaSnmpSubscription_CONSUMERS            = 14,
    imaSnmpSubscription_DISCARDEDMSGS        = 15,
    imaSnmpSubscription_EXPIREDMSGS          = 16,
    imaSnmpSubscription_MESSAGINGPOLICY      = 17,
    imaSnmpSubscription_Col_MAX              = 18,
   
} imaSnmpSubscriptionColumn_t;

#define imaSnmpSubscription_Col_MIN  imaSnmpSubscription_COLINDEX

#define IMA_SNMP_SUBSCRIPTION_INDEX_DEFAULT ""
#define IMA_SNMP_SUBSCRIPTION_STR_DEFAULT   ""
#define IMA_SNMP_SUBSCRIPTION_INT_DEFAULT  0


typedef struct ima_snmp_subscription_tag {
    ima_snmp_col_val_t  subscriptionItem[imaSnmpSubscription_Col_MAX];
    struct ima_snmp_subscription_tag *next;            /* next node  */
} ima_snmp_subscription_t;

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

    int ima_snmp_get_subscription_stat();
    ima_snmp_subscription_t *ima_snmp_subscription_get_table_head();

#ifdef __cplusplus
    }
#endif

#endif /* IMASNMPSERVERSTAT_H_ */
