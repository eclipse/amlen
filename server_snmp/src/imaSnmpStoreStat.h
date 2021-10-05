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
 * Note: this header file provides definitions for MessageSight store stats,
 *       as well as the interface to access store stats. 
 */

#ifndef _IMASNMPSTORESTAT_H_
#define _IMASNMPSTORESTAT_H_

#include "imaSnmpColumn.h"

#define COLUMN_IMASTORE_DEFAULT_SIZE  50

#define COLUMN_IMASTORE_DISKUSEDPCT_SIZE STR_SIZE_PERCENT
#define COLUMN_IMASTORE_DISKFREEBYTES_SIZE  100
#define COLUMN_IMASTORE_MEMUSEDPCT_SIZE  STR_SIZE_PERCENT
#define COLUMN_IMASTORE_MEMTOTALBYTES_SIZE    COLUMN_IMASTORE_DEFAULT_SIZE
#define COLUMN_IMASTORE_POOL1TOTALBYTES_SIZE  COLUMN_IMASTORE_DEFAULT_SIZE
#define COLUMN_IMASTORE_POOL1USEDBYTES_SIZE  COLUMN_IMASTORE_DEFAULT_SIZE
#define COLUMN_IMASTORE_POOL1USEDPCT_SIZE  STR_SIZE_PERCENT
#define COLUMN_IMASTORE_POOL1RECLIMIT_SIZE  COLUMN_IMASTORE_DEFAULT_SIZE
#define COLUMN_IMASTORE_POOL1RECUSED_SIZE  COLUMN_IMASTORE_DEFAULT_SIZE
#define COLUMN_IMASTORE_POOL2TOTALBYTES_SIZE  COLUMN_IMASTORE_DEFAULT_SIZE
#define COLUMN_IMASTORE_POOL2USEDBYTES_SIZE  COLUMN_IMASTORE_DEFAULT_SIZE
#define COLUMN_IMASTORE_POOL2USEDPCT_SIZE STR_SIZE_PERCENT

typedef enum {
    imaSnmpStore_NONE                          = 0,
    imaSnmpStore_DISK_USED_PERCENT            = 1,    //StoreDiskUsagePercent
    imaSnmpStore_DISK_FREE_BYTES            = 2,    //StoreDiskFreeBytes
    imaSnmpStore_MEM_USED_PERCENT            = 3,       //StoreMemoryUsagePercent
    imaSnmpStore_MEM_TOTAL_BYTES  =4,
    imaSnmpStore_POOL1_TOTAL_BYTES  =5,
    imaSnmpStore_POOL1_USED_BYTES   =6,
    imaSnmpStore_POOL1_USED_PERCENT   =7,
    imaSnmpStore_POOL1_RECORDS_LIMITBYTES =8,
    imaSnmpStore_POOL1_RECORDS_USEDBYTES   =9,
    imaSnmpStore_POOL2_TOTAL_BYTES  =10,
    imaSnmpStore_POOL2_USED_BYTES =11,
    imaSnmpStore_POOL2_USED_PERCENT=12,
    imaSnmpStore_Col_MAX
} imaSnmpStoreDataType_t;

#define imaSnmpStore_Col_MIN imaSnmpStore_DISK_USED_PERCENT

typedef struct ima_snmp_store_tag {
    ima_snmp_col_val_t  storeItem[imaSnmpStore_Col_MAX];
    time_t  time_storeStats;
} ima_snmp_store_t;


/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

       int ima_snmp_get_store_stat(char *buf, int len, imaSnmpStoreDataType_t statType);

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPSTORESTAT_H_ */

