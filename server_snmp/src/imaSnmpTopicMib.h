/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
 *
 *  Created on: 2014-7-21
 *      Author:
 */

#ifndef IMASNMPTOPICMIB_H_
#define IMASNMPTOPICMIB_H_

#define IMA_SNMP_TOPIC_OID 8
#define IMA_SNMP_TOPIC_MIB IMA_SNMP_STAT_MIB, IMA_SNMP_TOPIC_OID



#define COLUMN_IBMIMATOPIC_TABLE_COL_INDEX     1
#define COLUMN_IBMIMATOPIC_TOPICSTRING         2
#define COLUMN_IBMIMATOPIC_SUBSCRIPTIONS       3
#define COLUMN_IBMIMATOPIC_RESETTIME           4
#define COLUMN_IBMIMATOPIC_PUBLISHEDMSGS       5
#define COLUMN_IBMIMATOPIC_REJECTEDMSGS        6
#define COLUMN_IBMIMATOPIC_FAILEDPUBLISHES     7




#define IBMIMASTATUSTOPICTABLE_MIN_COL   COLUMN_IBMIMATOPIC_TABLE_COL_INDEX
#define IBMIMASTATUSTOPICTABLE_MAX_COL   COLUMN_IBMIMATOPIC_FAILEDPUBLISHES

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

       int ima_snmp_init_topic_table_mibs();

#ifdef __cplusplus
    }
#endif


#endif /* IMASNMPTOPICMIB_H_ */
