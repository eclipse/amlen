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
 * Note: this header file provides definitions for MessageSight memory stats,
 *       as well as the interface to access memory stats. 
 */

#ifndef _IMASNMPMEMSTAT_H_
#define _IMASNMPMEMSTAT_H_

#include "imaSnmpColumn.h"
#define MAX_STR_SIZE 256
#define COLUMN_IMAMEMITEM_DEFAULT_SIZE  100

typedef enum {
    imaSnmpMem_NONE          	    = 0,
    imaSnmpMem_TOTAL_BYTES    		= 1,   	//MemoryTotalBytes
    imaSnmpMem_FREE_BYTES      		= 2,	//MemoryFreeBytes
    imaSnmpMem_FREE_PERCENT    		= 3,	//MemoryFreePercent
    imaSnmpMem_SVR_VRTL_MEM_BYTES	= 4,	//ServerVirtualMemoryBytes
    imaSnmpMem_SVR_RSDT_SET_BYTES	= 5,	//ServerResidentSetBytes
    imaSnmpMem_GRP_MSG_PAYLOADS		= 6,	//MessagePayloads
    imaSnmpMem_GRP_PUB_SUB		    = 7,	//PublishSubscribe
    imaSnmpMem_GRP_DESTS		    = 8,	//Destinations
    imaSnmpMem_GRP_CUR_ACTIVITIES	= 9,	//CurrentActivity
    imaSnmpMem_GRP_CLIENT_STATES	= 10,	//ClientStates
    imaSnmpMem_Col_MAX              = 11,
} imaSnmpMemDataType_t;

#define imaSnmpMem_Col_MIN imaSnmpMem_TOTAL_BYTES

typedef struct ima_snmp_mem_tag {
    ima_snmp_col_val_t  memItem[imaSnmpMem_Col_MAX];
    time_t  time_memStats;
} ima_snmp_mem_t;

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

       int ima_snmp_get_mem_stat(char *buf, int len, imaSnmpMemDataType_t statType);

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPMEMSTAT_H_ */

