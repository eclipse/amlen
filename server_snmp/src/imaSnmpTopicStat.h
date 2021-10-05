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

#ifndef IMASNMPTOPICSTAT_H_
#define IMASNMPTOPICSTAT_H_

#include "imaSnmpColumn.h"


#define MAX_STR_SIZE 256

#define COLUMN_IBMIMATOPIC_TABLE_COL_INDEX_SIZE       10
#define COLUMN_IBMIMATOPIC_TOPICSTRING_SIZE           256  ////TopicString can be 65535. will handle the real size in code
#define COLUMN_IBMIMATOPIC_SUBSCRIPTIONS_SIZE         10
#define COLUMN_IBMIMATOPIC_RESETTIME_SIZE             100
#define COLUMN_IBMIMATOPIC_PUBLISHEDMSGS_SIZE         20
#define COLUMN_IBMIMATOPIC_REJECTEDMSGS_SIZE          20
#define COLUMN_IBMIMATOPIC_FAILEDPUBLISHES_SIZE       20


/* sample message
[{"TopicString":"/Test/guotao/#","Subscriptions":0,"ResetTime":1408555705193394000,
"PublishedMsgs":0,"RejectedMsgs":0,"FailedPublishes":0}
	*/

typedef enum {
    imaSnmpTopic_NONE               = 0,
    imaSnmpTopic_COLINDEX           = 1,
    imaSnmpTopic_TOPICSTRING        = 2,
    imaSnmpTopic_SUBSCRIPTIONS      = 3,
    imaSnmpTopic_RESETTIME          = 4,
    imaSnmpTopic_PUBLISHEDMSGS      = 5,
    imaSnmpTopic_REJECTEDMSGS       = 6,
    imaSnmpTopic_FAILEDPUBLISHES    = 7,
    imaSnmpTopic_Col_MAX            = 8,
   
} imaSnmpTopicColumn_t;

#define imaSnmpTopic_Col_MIN  imaSnmpTopic_COLINDEX

#define IMA_SNMP_TOPIC_INDEX_DEFAULT ""
#define IMA_SNMP_TOPIC_STR_DEFAULT   ""
#define IMA_SNMP_TOPIC_INT_DEFAULT  0

typedef struct ima_snmp_topic_tag {
    ima_snmp_col_val_t  topicItem[imaSnmpTopic_Col_MAX];
    struct ima_snmp_topic_tag *next;            /* next node  */
} ima_snmp_topic_t;

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

    int ima_snmp_get_topic_stat();
    ima_snmp_topic_t *ima_snmp_topic_get_table_head();

#ifdef __cplusplus
    }
#endif

#endif
