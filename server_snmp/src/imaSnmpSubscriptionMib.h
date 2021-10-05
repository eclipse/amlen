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
 * imaSnmpEdnpointMib.h
 *
 *  Created on: 2014-7-21
 *      Author:
 */

#ifndef IMASNMPSUBSCRIPTIONMIB_H_
#define IMASNMPSUBSCRIPTIONMIB_H_

#define IMA_SNMP_SUBSCRIPTION_OID 6
#define IMA_SNMP_SUBSCRIPTION_MIB IMA_SNMP_STAT_MIB, IMA_SNMP_SUBSCRIPTION_OID

#define COLUMN_IBMIMASUBSCRIPTION_TABLE_COL_INDEX      1
#define COLUMN_IBMIMASUBSCRIPTION_SUBNAME              2
#define COLUMN_IBMIMASUBSCRIPTION_TOPICSTRING          3
#define COLUMN_IBMIMASUBSCRIPTION_CLIENTID             4
#define COLUMN_IBMIMASUBSCRIPTION_ISDURABLE            5
#define COLUMN_IBMIMASUBSCRIPTION_BUFFEREDMSGS         6
#define COLUMN_IBMIMASUBSCRIPTION_BUFFEREDMSGSHWM      7
#define COLUMN_IBMIMASUBSCRIPTION_BUFFEREDPERCENT      8
#define COLUMN_IBMIMASUBSCRIPTION_MAXMESSAGES          9
#define COLUMN_IBMIMASUBSCRIPTION_PUBLISHEDMSGS       10
#define COLUMN_IBMIMASUBSCRIPTION_REJECTEDMSGS        11
#define COLUMN_IBMIMASUBSCRIPTION_BUFFEREDHWMPERCENT  12
#define COLUMN_IBMIMASUBSCRIPTION_ISSHARED            13
#define COLUMN_IBMIMASUBSCRIPTION_CONSUMERS           14
#define COLUMN_IBMIMASUBSCRIPTION_DISCARDEDMSGS       15
#define COLUMN_IBMIMASUBSCRIPTION_EXPIREDMSGS         16
#define COLUMN_IBMIMASUBSCRIPTION_MESSAGINGPOLICYE    17


#define IBMIMASTATUSSUBSCRIPTIONTABLE_MIN_COL   COLUMN_IBMIMASUBSCRIPTION_TABLE_COL_INDEX
#define IBMIMASTATUSSUBSCRIPTIONTABLE_MAX_COL   COLUMN_IBMIMASUBSCRIPTION_MESSAGINGPOLICYE

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

       int ima_snmp_init_subscription_table_mibs();

#ifdef __cplusplus
    }
#endif


#endif /* IMASNMPSUBSCRIPTIONMIB_H_ */
