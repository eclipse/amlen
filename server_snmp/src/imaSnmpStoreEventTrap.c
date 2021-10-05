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

/**
 * @brief Store Event Trap interface for MessageSight
 * @date August
 *
 * This provides the interface of store MIB Trap handler for MessageSight.
 *
 */

#include <stdio.h>
#include <string.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <ismrc.h>
#include <ismutil.h>
#include <ismjson.h>


#include "imaSnmpColumn.h"
#include "imaSnmpMib.h"
#include "imaSnmpStoreEventMib.h"
#include "imaSnmpEventCommonMib.h"
#include "imaSnmpStoreEventTrap.h"


static const oid snmptrap_oid[] = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0 };

static  const ima_snmp_col_desc_t storeEvent_element_desc[] = {
   {NULL,imaSnmpCol_None, 0},  // imaSnmpStoreEvent_NONE
   {"DiskUsedPercent",imaSnmpCol_String, STR_SIZE_PERCENT},  // imaSnmpStoreEvent_DISK_USED_PERCENT
   {"DiskFreeBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpStoreEvent_DISK_FREE_BYTES
   {"MemoryUsedPercent",imaSnmpCol_String, STR_SIZE_PERCENT},  // imaSnmpStoreEvent_MEMORY_USED_PERCENT
   {"MemoryTotalBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpStoreEvent_MEMORY_TOTAL_BYTES
   {"Pool1TotalBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpStoreEvent_POOL1_TOTAL_BYTES
   {"Pool1UsedBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpStoreEvent_POOL1_USED_BYTES
   {"Pool1UsedPercent",imaSnmpCol_String, STR_SIZE_PERCENT},  // imaSnmpStoreEvent_POOL1_USED_PERCENT
   {"Pool1RecordsLimitBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpStoreEvent_POOL1_RECORDS_LIMITBYTES
   {"Pool1RecordsUsedBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpStoreEvent_POOL1_RECORDS_USEDBYTES
   {"Pool2TotalBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpStoreEvent_POOL2_TOTAL_BYTES
   {"Pool2UsedBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpStoreEvent_POOL2_USED_BYTES
   {"Pool2UsedPercent",imaSnmpCol_String, STR_SIZE_PERCENT},  // imaSnmpStoreEvent_POOL2_USED_PERCENT
};


int  send_ibmImaNotificationStoreDiskUsageWarning_trap(ism_json_parse_t *pDataObj)
{
    netsnmp_variable_list *var_list = NULL;
    const oid   ibmImaNotificationStoreDiskUsageWarning_oid[] = {IMA_SNMP_STORE_DISK_USAGE_WARN_MIB};
    const oid   sample_oid[] = {IMA_SNMP_TRAP_STORE_DISKFREEBYTES_MIB};
    const oid ibmImaNotificationStoreElements_oid[][OID_LENGTH(sample_oid)] = {
          {IMA_SNMP_STORE_DISK_USAGE_WARN_MIB},  //just a place holder for idx zero
          {IMA_SNMP_TRAP_STORE_DISKUSEDPCT_MIB},
          {IMA_SNMP_TRAP_STORE_DISKFREEBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_MEMUSEDPCT_MIB},
          {IMA_SNMP_TRAP_STORE_MEMTOTALBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1TOTALBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1USEDBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1USEDPERCENT_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1RECORDSLIMITBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1RECORDSUSEDBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL2TOTALBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL2USEDBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL2USEDPERCENT_MIB},
    };
    int rc = ISMRC_OK; 
    int i ;
    char *elementString;

    if (NULL == pDataObj)
    {
        TRACE(2, "null data object in store usage warning event. \n");
        return ISMRC_NullPointer;
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&var_list,
                              snmptrap_oid, OID_LENGTH(snmptrap_oid),
                              ASN_OBJECT_ID,
                              ibmImaNotificationStoreDiskUsageWarning_oid,
                              sizeof
                              (ibmImaNotificationStoreDiskUsageWarning_oid));

    rc = ima_snmp_event_set_common_mibs(pDataObj, &var_list);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "Error in setting common elements for store disk usage warning trap , rc = %d\n", rc);
    }

    /*
     * Set other vairables here.
     */
    for (i = imaSnmpStoreEvent_MIN ; i < imaSnmpStoreEvent_MAX; i++)
    {
        switch (storeEvent_element_desc[i].colType)
        {
        case imaSnmpCol_String:
            if ((NULL == storeEvent_element_desc[i].colName)|| 
                (0 == strlen(storeEvent_element_desc[i].colName)))
               break;
            elementString =  (char *)ism_json_getString(pDataObj, 
                                     storeEvent_element_desc[i].colName);
            if (NULL == elementString) break;
            snmp_varlist_add_variable(&var_list,
                              ibmImaNotificationStoreElements_oid[i],
                              OID_LENGTH(ibmImaNotificationStoreElements_oid[i]),
                              ASN_OCTET_STR,
                              elementString, 
                              ( strlen(elementString) > storeEvent_element_desc[i].colSize)?
                               storeEvent_element_desc[i].colSize : strlen(elementString));
            break;  
        case imaSnmpCol_Integer:
        default:
            TRACE(2,"data type %d is not supported yet. \n",storeEvent_element_desc[i].colType);
            break;
        }

    }

    send_v2trap(var_list);
    snmp_free_varbind(var_list);

    return rc;
}

int  send_ibmImaNotificationStoreDiskUsageAlert_trap(ism_json_parse_t *pDataObj)
{
    netsnmp_variable_list *var_list = NULL;
    const oid   ibmImaNotificationStoreDiskUsageAlert_oid[] = {IMA_SNMP_STORE_DISK_USAGE_ALERT_MIB};
    const oid   sample_oid[] = {IMA_SNMP_TRAP_STORE_DISKFREEBYTES_MIB};
    const oid ibmImaNotificationStoreElements_oid[][OID_LENGTH(sample_oid)] = {
          {IMA_SNMP_STORE_DISK_USAGE_ALERT_MIB},  //just a place holder for idx zero
          {IMA_SNMP_TRAP_STORE_DISKUSEDPCT_MIB},
          {IMA_SNMP_TRAP_STORE_DISKFREEBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_MEMUSEDPCT_MIB},
          {IMA_SNMP_TRAP_STORE_MEMTOTALBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1TOTALBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1USEDBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1USEDPERCENT_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1RECORDSLIMITBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1RECORDSUSEDBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL2TOTALBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL2USEDBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL2USEDPERCENT_MIB},
    };
    int rc = ISMRC_OK; 
    int i ;
    char *elementString;

    if (NULL == pDataObj)
    {
        TRACE(2, "null data object in store warning event. \n");
        return ISMRC_NullPointer;
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&var_list,
                              snmptrap_oid, OID_LENGTH(snmptrap_oid),
                              ASN_OBJECT_ID,
                              ibmImaNotificationStoreDiskUsageAlert_oid,
                              sizeof
                              (ibmImaNotificationStoreDiskUsageAlert_oid));

    rc = ima_snmp_event_set_common_mibs(pDataObj, &var_list);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "Error in setting common elements for store disk usage alert trap , rc = %d\n", rc);
    }

    /*
     * Set other vairables here.
     */
    for (i = imaSnmpStoreEvent_MIN ; i < imaSnmpStoreEvent_MAX; i++)
    {
        switch (storeEvent_element_desc[i].colType)
        {
        case imaSnmpCol_String:
            if ((NULL == storeEvent_element_desc[i].colName)|| 
                (0 == strlen(storeEvent_element_desc[i].colName)))
               break;
            elementString =  (char *)ism_json_getString(pDataObj, 
                                     storeEvent_element_desc[i].colName);
            if (NULL == elementString) break;
            snmp_varlist_add_variable(&var_list,
                              ibmImaNotificationStoreElements_oid[i],
                              OID_LENGTH(ibmImaNotificationStoreElements_oid[i]),
                              ASN_OCTET_STR,
                              elementString, 
                              ( strlen(elementString) > storeEvent_element_desc[i].colSize)?
                               storeEvent_element_desc[i].colSize : strlen(elementString));
            break;  
        case imaSnmpCol_Integer:
        default:
            TRACE(2,"data type %d is not supported yet. \n",storeEvent_element_desc[i].colType);
            break;
        }

    }

    send_v2trap(var_list);
    snmp_free_varbind(var_list);

    return rc;
}

int  send_ibmImaNotificationStorePool1MemLowAlert_trap(ism_json_parse_t *pDataObj)
{
    netsnmp_variable_list *var_list = NULL;
    const oid   ibmImaNotificationStorePool1MemLowAlert_oid[] = {IMA_SNMP_TRAP_STORE_POOL1_MEM_LOW_ALERT_MIB};
    const oid   sample_oid[] = {IMA_SNMP_TRAP_STORE_POOL1_MEM_LOW_ALERT_MIB};
    const oid ibmImaNotificationStoreElements_oid[][OID_LENGTH(sample_oid)] = {
          {IMA_SNMP_TRAP_STORE_POOL1_MEM_LOW_ALERT_MIB},  //just a place holder for idx zero
          {IMA_SNMP_TRAP_STORE_DISKUSEDPCT_MIB},
          {IMA_SNMP_TRAP_STORE_DISKFREEBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_MEMUSEDPCT_MIB},
          {IMA_SNMP_TRAP_STORE_MEMTOTALBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1TOTALBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1USEDBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1USEDPERCENT_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1RECORDSLIMITBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL1RECORDSUSEDBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL2TOTALBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL2USEDBYTES_MIB},
          {IMA_SNMP_TRAP_STORE_POOL2USEDPERCENT_MIB},
    };
    int rc = ISMRC_OK; 
    int i ;
    char *elementString;

    if (NULL == pDataObj)
    {
        TRACE(2, "null data object in store warning event. \n");
        return ISMRC_NullPointer;
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&var_list,
                              snmptrap_oid, OID_LENGTH(snmptrap_oid),
                              ASN_OBJECT_ID,
                              ibmImaNotificationStorePool1MemLowAlert_oid,
                              sizeof
                              (ibmImaNotificationStorePool1MemLowAlert_oid));

    rc = ima_snmp_event_set_common_mibs(pDataObj, &var_list);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "Error in setting common elements for store pool1 memory low alert trap , rc = %d\n", rc);
    }

    /*
     * Set other vairables here.
     */
    for (i = imaSnmpStoreEvent_MIN ; i < imaSnmpStoreEvent_MAX; i++)
    {
        switch (storeEvent_element_desc[i].colType)
        {
        case imaSnmpCol_String:
            if ((NULL == storeEvent_element_desc[i].colName)|| 
                (0 == strlen(storeEvent_element_desc[i].colName)))
               break;
            elementString =  (char *)ism_json_getString(pDataObj, 
                                     storeEvent_element_desc[i].colName);
            if (NULL == elementString) break;
            snmp_varlist_add_variable(&var_list,
                              ibmImaNotificationStoreElements_oid[i],
                              OID_LENGTH(ibmImaNotificationStoreElements_oid[i]),
                              ASN_OCTET_STR,
                              elementString, 
                              ( strlen(elementString) > storeEvent_element_desc[i].colSize)?
                               storeEvent_element_desc[i].colSize : strlen(elementString));
            break;  
        case imaSnmpCol_Integer:
        default:
            TRACE(2,"data type %d is not supported yet. \n",storeEvent_element_desc[i].colType);
            break;
        }

    }
    
    TRACE(9, "SNMP: send_v2trap - store event.\n");

    send_v2trap(var_list);
    snmp_free_varbind(var_list);

    return rc;
}
