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

#ifndef IMASNMPCONNECTIONSTAT_H_
#define IMASNMPCONNECTIONSTAT_H_

#include "imaSnmpColumn.h"


#define MAX_STR_SIZE 256

#define COLUMN_IBMIMACONNECTION_TABLE_COL_INDEX_SIZE       10
#define COLUMN_IBMIMACONNECTION_NAME_SIZE                 256
#define COLUMN_IBMIMACONNECTION_PROTOCOL_SIZE             100
#define COLUMN_IBMIMACONNECTION_CLIENTADDR_SIZE            50
#define COLUMN_IBMIMACONNECTION_USERID_SIZE               256
#define COLUMN_IBMIMACONNECTION_ENDPOINT_SIZE             100
#define COLUMN_IBMIMACONNECTION_PORT_SIZE                  10
#define COLUMN_IBMIMACONNECTION_CONNECTTIME_SIZE          100
#define COLUMN_IBMIMACONNECTION_DURATION_SIZE             100
#define COLUMN_IBMIMACONNECTION_READBYTES_SIZE            100
#define COLUMN_IBMIMACONNECTION_READMSG_SIZE              100
#define COLUMN_IBMIMACONNECTION_WRITEBYTES_SIZE           100
#define COLUMN_IBMIMACONNECTION_WRITEMSG_SIZE             100

/* sample message
[{"Name":"SNMPAgent","Protocol":"mqtt","ClientAddr":"127.0.0.1","UserId":"",
	"Endpoint":"DemoMqttEndpoint","Port":1883,"ConnectTime":1406878643454542000,
	"Duration":296915899383000,"ReadBytes":24802,"ReadMsg":0,"WriteBytes":64141327,
	"WriteMsg":148418},]
	*/

typedef enum {
    imaSnmpConnection_NONE              = 0,
    imaSnmpConnection_COLINDEX          = 1,
    imaSnmpConnection_NAME              = 2,
    imaSnmpConnection_PROTOCOL          = 3,
    imaSnmpConnection_CLIENTADDR        = 4,
    imaSnmpConnection_USERID            = 5,
    imaSnmpConnection_ENDPOINT          = 6,
    imaSnmpConnection_PORT              = 7,
    imaSnmpConnection_CONNECTTIME       = 8,
    imaSnmpConnection_DURATION          = 9,
    imaSnmpConnection_READBYTES         = 10,
    imaSnmpConnection_READMSG           = 11,
    imaSnmpConnection_WRITEBYTES        = 12,
    imaSnmpConnection_WRITEMSG          = 13,
    imaSnmpConnection_Col_MAX           = 14,
   
} imaSnmpConnectionColumn_t;

#define imaSnmpConnection_Col_MIN  imaSnmpConnection_COLINDEX

#define IMA_SNMP_CONN_INDEX_DEFAULT "0"
#define IMA_SNMP_CONN_STR_DEFAULT   ""
#define IMA_SNMP_CONN_INT_DEFAULT  0


typedef struct ima_snmp_connection_tag {
    ima_snmp_col_val_t  connectionItem[imaSnmpConnection_Col_MAX];
    struct ima_snmp_connection_tag *next;            /* next node  */
} ima_snmp_connection_t;

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

    int ima_snmp_get_connection_stat();
    ima_snmp_connection_t *ima_snmp_connection_get_table_head();

#ifdef __cplusplus
    }
#endif

#endif /* IMASNMPSERVERSTAT_H_ */
