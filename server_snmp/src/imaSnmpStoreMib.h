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
 * Note: this header file provides definitions for MessageSight store MIBs,
 *       as well as the interface to access store MIBs. 
 */


#ifndef _IMASNMPSTOREMIB_H_
#define _IMASNMPSTOREMIB_H_

#define IMA_SNMP_STORE_OID 3
#define IMA_SNMP_STORE_MIB IMA_SNMP_STAT_MIB, IMA_SNMP_STORE_OID 



#define IMA_SNMP_STORE_DISKUSEDPCT_OID 1
#define IMA_SNMP_STORE_DISKUSEDPCT_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_DISKUSEDPCT_OID

#define IMA_SNMP_STORE_DISKFREEBYTES_OID 2
#define IMA_SNMP_STORE_DISKFREEBYTES_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_DISKFREEBYTES_OID

#define IMA_SNMP_STORE_MEMUSEDPCT_OID 3
#define IMA_SNMP_STORE_MEMUSEDPCT_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_MEMUSEDPCT_OID

#define IMA_SNMP_STORE_MEMTOTALBYTES_OID 4
#define IMA_SNMP_STORE_MEMTOTALBYTES_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_MEMTOTALBYTES_OID

#define IMA_SNMP_STORE_POOL1TOTALBYTES_OID 5
#define IMA_SNMP_STORE_POOL1TOTALBYTES_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_POOL1TOTALBYTES_OID

#define IMA_SNMP_STORE_POOL1USEDBYTES_OID 6
#define IMA_SNMP_STORE_POOL1USEDBYTES_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_POOL1USEDBYTES_OID

#define IMA_SNMP_STORE_POOL1USEDPERCENT_OID 7
#define IMA_SNMP_STORE_POOL1USEDPERCENT_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_POOL1USEDPERCENT_OID

#define IMA_SNMP_STORE_POOL1RECORDSLIMITBYTES_OID 8
#define IMA_SNMP_STORE_POOL1RECORDSLIMITBYTES_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_POOL1RECORDSLIMITBYTES_OID

#define IMA_SNMP_STORE_POOL1RECORDSUSEDBYTES_OID 9
#define IMA_SNMP_STORE_POOL1RECORDSUSEDBYTES_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_POOL1RECORDSUSEDBYTES_OID

#define IMA_SNMP_STORE_POOL2TOTALBYTES_OID 10
#define IMA_SNMP_STORE_POOL2TOTALBYTES_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_POOL2TOTALBYTES_OID

#define IMA_SNMP_STORE_POOL2USEDBYTES_OID 11
#define IMA_SNMP_STORE_POOL2USEDBYTES_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_POOL2USEDBYTES_OID

#define IMA_SNMP_STORE_POOL2USEDPERCENT_OID 12
#define IMA_SNMP_STORE_POOL2USEDPERCENT_MIB IMA_SNMP_STORE_MIB, IMA_SNMP_STORE_POOL2USEDPERCENT_OID

/*
 * static char *StoreDataType[] = { "", //imaSnmpStore_NONE
                                    "MemoryUsedPercent",
                                    "DiskUsedPercent",
                                    "DiskFreeBytes"
									"MemoryTotalBytes",
									"Pool1TotalBytes",
									"Pool1UsedBytes",
									"Pool1UsedPercent",
									"Pool1RecordsLimitBytes",
									"Pool1RecordsUsedBytes",
									"Pool2TotalBytes",
									"Pool2UsedBytes",
									"Pool2UsedPercent"};
 * */

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

       int ima_snmp_init_store_mibs();

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPSTOREMIB_H_ */

