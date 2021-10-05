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

#ifndef IMASNMPSERVERSTAT_H_
#define IMASNMPSERVERSTAT_H_

#include "imaSnmpColumn.h"

#define COLUMN_IMASERVER_DEFAULT_SIZE  100

#define COLUMN_IMASERVER_SERVERSTATE_SIZE COLUMN_IMASERVER_DEFAULT_SIZE
#define COLUMN_IMASERVER_SERVERSTATESTR_SIZE COLUMN_IMASERVER_DEFAULT_SIZE
#define COLUMN_IMASERVER_ADMINSTATERC_SIZE COLUMN_IMASERVER_DEFAULT_SIZE
#define COLUMN_IMASERVER_SERVERUPTIME_SIZE COLUMN_IMASERVER_DEFAULT_SIZE
#define COLUMN_IMASERVER_SERVERUPTIMESTR_SIZE STR_SIZE_DEFAULT
#define COLUMN_IMASERVER_ISHAENABLED_SIZE COLUMN_IMASERVER_DEFAULT_SIZE
#define COLUMN_IMASERVER_NEWROLE_SIZE COLUMN_IMASERVER_DEFAULT_SIZE
#define COLUMN_IMASERVER_OLDROLE_SIZE COLUMN_IMASERVER_DEFAULT_SIZE
#define COLUMN_IMASERVER_ACTIVENODES_SIZE COLUMN_IMASERVER_DEFAULT_SIZE
#define COLUMN_IMASERVER_SYNCNODES_SIZE COLUMN_IMASERVER_DEFAULT_SIZE
#define COLUMN_IMASERVER_PRIMARYLASTTIME_SIZE STR_SIZE_DEFAULT
#define COLUMN_IMASERVER_PCTSYNCCOMPLETION_SIZE COLUMN_IMASERVER_DEFAULT_SIZE
#define COLUMN_IMASERVER_REASONCODE_SIZE COLUMN_IMASERVER_DEFAULT_SIZE

typedef enum {
    imaSnmpServer_NONE          	= 0,
    imaSnmpServer_SERVERSTATE    	= 1,
    imaSnmpServer_SERVERSTATESTR    = 2,
	imaSnmpServer_ADMINSTATERC	    = 3,
    imaSnmpServer_SERVERUPTIME		= 4,
    imaSnmpServer_SERVERUPTIMESTR   = 5,   //256
    imaSnmpServer_ISHAENABLED	    = 6,
    imaSnmpServer_NEWROLE	        = 7,
    imaSnmpServer_OLDROLE	        = 8,
    imaSnmpServer_ACTIVENODES	    = 9,
    imaSnmpServer_SYNCNODES	        = 10,
    imaSnmpServer_PRIMARYLASTTIME	= 11,  //256
    imaSnmpServer_PCTSYNCCOMPLETION	= 12,
    imaSnmpServer_REASONCODE	    = 13,
    imaSnmpServer_Col_MAX           = 14,
} imaSnmpServerDataType_t;

#define imaSnmpServer_Col_MIN imaSnmpServer_SERVERSTATE

#define IMA_SERVER_STATE_UNKNOWN       0
#define IMA_SERVER_STATE_MAINTENANCE  1
#define IMA_SERVER_STATUS_STOPPED     2
#define IMA_SERVER_STATE_STARTING_STORE 3

#define IMA_SERVER_MAINTENANCE_STR "Running (maintenance)"
#define IMA_SERVER_STOP_STR        "Stopped"
#define IMA_SERVER_START_STORE_STR "StoreStarting"
#define IMA_SERVER_UNKNOWN_STR      "Unknown"


typedef struct ima_snmp_server_tag {
    ima_snmp_col_val_t  serverItem[imaSnmpServer_Col_MAX];
    time_t  time_serverStats;
} ima_snmp_server_t;


/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

int ima_snmp_get_server_stat(char *buf, int len, imaSnmpServerDataType_t statType);
int ima_snmp_get_server_state_from_shell();

#ifdef __cplusplus
    }
#endif

#endif /* IMASNMPSERVERSTAT_H_ */
