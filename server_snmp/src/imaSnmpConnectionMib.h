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

#ifndef IMASNMPCONNECTIONMIB_H_
#define IMASNMPCONNECTIONMIB_H_

#define IMA_SNMP_CONNECTION_OID 7
#define IMA_SNMP_CONNECTION_MIB IMA_SNMP_STAT_MIB, IMA_SNMP_CONNECTION_OID



#define COLUMN_IBMIMACONNECTION_TABLE_COL_INDEX     1
#define COLUMN_IBMIMACONNECTION_NAME                2
#define COLUMN_IBMIMACONNECTION_PROTOCOL            3
#define COLUMN_IBMIMACONNECTION_CLIENTADDR          4
#define COLUMN_IBMIMACONNECTION_USERID              5
#define COLUMN_IBMIMACONNECTION_ENDPOINT            6
#define COLUMN_IBMIMACONNECTION_PORT                7
#define COLUMN_IBMIMACONNECTION_CONNECTTIME         8
#define COLUMN_IBMIMACONNECTION_DURATION            9
#define COLUMN_IBMIMACONNECTION_READBYTES          10
#define COLUMN_IBMIMACONNECTION_READMSG            11
#define COLUMN_IBMIMACONNECTION_WRITEBYTES         12
#define COLUMN_IBMIMACONNECTION_WRITEMSG           13



#define IBMIMASTATUSCONNECTIONTABLE_MIN_COL   COLUMN_IBMIMACONNECTION_TABLE_COL_INDEX
#define IBMIMASTATUSCONNECTIONTABLE_MAX_COL   COLUMN_IBMIMACONNECTION_WRITEMSG

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

       int ima_snmp_init_connection_table_mibs();

#ifdef __cplusplus
    }
#endif


#endif /* IMASNMPCONNECTIONMIB_H_ */
