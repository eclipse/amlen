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

#ifndef IMASNMPENDPOINTSTAT_H_
#define IMASNMPENDPOINTSTAT_H_

#include "imaSnmpColumn.h"

#define MAX_STR_SIZE 256
#define COLUMN_IBMIMAENDPOINT_TABLE_COL_INDEX_SIZE       10
#define COLUMN_IBMIMAENDPOINTNAME_SIZE                  256
#define COLUMN_IBMIMAENDPOINTIPADDR_SIZE                100
#define COLUMN_IBMIMAENDPOINTENABLED_SIZE                10
#define COLUMN_IBMIMAENDPOINTTOTAL_SIZE                 100
#define COLUMN_IBMIMAENDPOINTACTIVE_SIZE                100
#define COLUMN_IBMIMAENDPOINTMESSAGES_SIZE              100
#define COLUMN_IBMIMAENDPOINTBYTES_SIZE                 100
#define COLUMN_IBMIMAENDPOINTLASTERRORCODE_SIZE         100
#define COLUMN_IBMIMAENDPOINTCONFIGTIME_SIZE            100
#define COLUMN_IBMIMAENDPOINTRESETTIME_SIZE             100
#define COLUMN_IBMIMAENDPOINTBADCONNECTIONS_SIZE        100

typedef enum {
    imaSnmpEndpoint_NONE              = 0,
    imaSnmpEndpoint_COLINDEX          = 1,
    imaSnmpEndpoint_NAME              = 2,    //EndpointName
    imaSnmpEndpoint_IPADDR            = 3,    //Endpoint IP address
    imaSnmpEndpoint_ENABLED           = 4,    //Endpoint enable flag
    imaSnmpEndpoint_TOTAL             = 5,    //Connection count
    imaSnmpEndpoint_ACTIVE            = 6,    //Active connections
    imaSnmpEndpoint_MESSAGES          = 7,    //Message count
    imaSnmpEndpoint_BYTES             = 8,    //Message byte count
    imaSnmpEndpoint_LAST_ERROR_CODE   = 9,    //last errCode
    imaSnmpEndpoint_CONFIG_TIME       = 10,   //Endpoint config time
    imaSnmpEndpoint_RESET_TIME        = 11,   //Endpoint reset time
    imaSnmpEndpoint_BAD_CONNECTIONS   = 12,   //Bad connection count
    imaSnmpEndpoint_Col_MAX           = 13,
   
} imaSnmpEndpointColumn_t;

#define imaSnmpEndpoint_Col_MIN  imaSnmpEndpoint_COLINDEX

#define IMA_SNMP_ENDPOINT_INDEX_DEFAULT ""
#define IMA_SNMP_ENDPOINT_STR_DEFAULT   ""
#define IMA_SNMP_ENDPOINT_INT_DEFAULT  0

typedef struct ima_snmp_endpoint_tag {
    ima_snmp_col_val_t  endpointItem[imaSnmpEndpoint_Col_MAX];
    struct ima_snmp_endpoint_tag *next;            /* next node  */
} ima_snmp_endpoint_t;

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

    int ima_snmp_get_endpoint_stat();
    ima_snmp_endpoint_t *ima_snmp_endpoint_get_table_head();

#ifdef __cplusplus
    }
#endif

#endif /* IMASNMPENDPOINTSTAT_H_ */
