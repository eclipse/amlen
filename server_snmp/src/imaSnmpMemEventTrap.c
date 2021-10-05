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
 * @brief Memory Event Trap interface for MessageSight
 * @date July
 *
 * This provides the interface of memory MIB Trap handler for MessageSight.
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
#include "imaSnmpMemEventMib.h"
#include "imaSnmpEventCommonMib.h"
#include "imaSnmpMemEventTrap.h"

static const oid snmptrap_oid[] = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0 };

static  const ima_snmp_col_desc_t memEvent_element_desc[] = {
   {NULL,imaSnmpCol_None, 0},  // imaSnmpMemEvent_NONE
   {"MemoryTotalBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpMemEvent_TOTAL_BYTES
   {"MemoryFreeBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpMemEvent_FREE_BYTES
   {"MemoryFreePercent",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpMemEvent_FREE_PERCENT
   {"ServerVirtualMemoryBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpMemEvent_SVR_VRTL_MEM_BYTES
   {"ServerResidentSetBytes",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpMemEvent_SVR_RSDT_SET_BYTES
   {"MessagePayloads",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpMemEvent_GRP_MSG_PAYLOADS
   {"PublishSubscribe",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpMemEvent_GRP_PUB_SUB
   {"Destinations",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpMemEvent_GRP_DESTS
   {"CurrentActivity",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpMemEvent_GRP_CUR_ACTIVITIES
   {"ClientStates",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpMemEvent_GRP_CLIENT_STATES
};


int  send_ibmImaNotificationMemoryUsageWarning_trap(ism_json_parse_t *pDataObj)
{
    netsnmp_variable_list *var_list = NULL;
    const oid   ibmImaNotificationMemoryUsageWarning_oid[] = {IMA_SNMP_MEM_USAGE_WARN_MIB};
    const oid   sample_oid[] = {IMA_SNMP_TRAP_MEM_TOTALBYTES_MIB};
    const oid ibmImaNotificationMemoryElements_oid[][OID_LENGTH(sample_oid)] = {
          {IMA_SNMP_MEM_USAGE_WARN_MIB},  //just a place holder for idx zero
          {IMA_SNMP_TRAP_MEM_TOTALBYTES_MIB},
          {IMA_SNMP_TRAP_MEM_FREEBYTES_MIB},
          {IMA_SNMP_TRAP_MEM_FREEPERCENT_MIB},
          {IMA_SNMP_TRAP_SVR_VRTL_MEM_BYTES_MIB},
          {IMA_SNMP_TRAP_SVR_RSDT_SET_BYTES_MIB},
          {IMA_SNMP_TRAP_GRP_MSG_PAYLOADS_MIB},
          {IMA_SNMP_TRAP_GRP_PUB_SUB_MIB},
          {IMA_SNMP_TRAP_GRP_DESTS_MIB},
          {IMA_SNMP_TRAP_GRP_CUR_ACTIVITIES_MIB},
          {IMA_SNMP_TRAP_GRP_CLIENT_STATES_MIB},
    };
    int rc = ISMRC_OK; 
    int i ;
    char *elementString;

    if (NULL == pDataObj)
    {
        TRACE(2, "null data object in memory warning event. \n");
        return ISMRC_NullPointer;
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&var_list,
                              snmptrap_oid, OID_LENGTH(snmptrap_oid),
                              ASN_OBJECT_ID,
                              ibmImaNotificationMemoryUsageWarning_oid,
                              sizeof
                              (ibmImaNotificationMemoryUsageWarning_oid));

    rc = ima_snmp_event_set_common_mibs(pDataObj, &var_list);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "Error in setting common elements for memory usage warning trap , rc = %d\n", rc);
    }

    /*
     * Set other vairables here.
     */
    for (i = imaSnmpMemEvent_MIN ; i < imaSnmpMemEvent_MAX; i++)
    {
        switch (memEvent_element_desc[i].colType)
        {
        case imaSnmpCol_String:
            if ((NULL == memEvent_element_desc[i].colName)|| 
                (0 == strlen(memEvent_element_desc[i].colName)))
               break;
            elementString =  (char *)ism_json_getString(pDataObj, 
                                     memEvent_element_desc[i].colName);
            if (NULL == elementString) break;
            snmp_varlist_add_variable(&var_list,
                              ibmImaNotificationMemoryElements_oid[i],
                              OID_LENGTH(ibmImaNotificationMemoryElements_oid[i]),
                              ASN_OCTET_STR,
                              elementString, 
                              ( strlen(elementString) > memEvent_element_desc[i].colSize)?
                               memEvent_element_desc[i].colSize : strlen(elementString));
            break;  
        case imaSnmpCol_Integer:
        default:
            TRACE(2,"data type %d is not supported yet. \n",memEvent_element_desc[i].colType);
            break;
        }

    }

    send_v2trap(var_list);
    snmp_free_varbind(var_list);

    return rc;
}

int  send_ibmImaNotificationMemoryUsageAlert_trap(ism_json_parse_t *pDataObj)
{
    netsnmp_variable_list *var_list = NULL;
    const oid   ibmImaNotificationMemoryUsageAlert_oid[] = {IMA_SNMP_MEM_USAGE_ALERT_MIB};
    const oid   sample_oid[] = {IMA_SNMP_TRAP_MEM_TOTALBYTES_MIB};
    const oid  ibmImaNotificationMemoryElements_oid[][OID_LENGTH(sample_oid)] = {
          {IMA_SNMP_MEM_USAGE_ALERT_MIB},  //just a place holder for idx zero
          {IMA_SNMP_TRAP_MEM_TOTALBYTES_MIB},
          {IMA_SNMP_TRAP_MEM_FREEBYTES_MIB},
          {IMA_SNMP_TRAP_MEM_FREEPERCENT_MIB},
          {IMA_SNMP_TRAP_SVR_VRTL_MEM_BYTES_MIB},
          {IMA_SNMP_TRAP_SVR_RSDT_SET_BYTES_MIB},
          {IMA_SNMP_TRAP_GRP_MSG_PAYLOADS_MIB},
          {IMA_SNMP_TRAP_GRP_PUB_SUB_MIB},
          {IMA_SNMP_TRAP_GRP_DESTS_MIB},
          {IMA_SNMP_TRAP_GRP_CUR_ACTIVITIES_MIB},
          {IMA_SNMP_TRAP_GRP_CLIENT_STATES_MIB},
    };
    int rc = ISMRC_OK; 
    int i ;
    char *elementString;

    if (NULL == pDataObj)
    {
        TRACE(2, "null data object in memory warning event. \n");
        return ISMRC_NullPointer;
    }

    /*
     * Set the snmpTrapOid.0 value
     */
    snmp_varlist_add_variable(&var_list,
                              snmptrap_oid, OID_LENGTH(snmptrap_oid),
                              ASN_OBJECT_ID,
                              ibmImaNotificationMemoryUsageAlert_oid,
                              sizeof
                              (ibmImaNotificationMemoryUsageAlert_oid));

    rc = ima_snmp_event_set_common_mibs(pDataObj, &var_list);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "Error in setting common elements for memory usage alert trap , rc = %d\n", rc);
    }

    /*
     * Set other vairables here.
     */
    for (i = imaSnmpMemEvent_MIN ; i < imaSnmpMemEvent_MAX; i++)
    {
        switch (memEvent_element_desc[i].colType)
        {
        case imaSnmpCol_String:
            if ((NULL == memEvent_element_desc[i].colName)|| 
                (0 == strlen(memEvent_element_desc[i].colName)))
               break;
            elementString =  (char *)ism_json_getString(pDataObj, 
                                     memEvent_element_desc[i].colName);
            if (NULL == elementString) break;
            snmp_varlist_add_variable(&var_list,
                              ibmImaNotificationMemoryElements_oid[i],
                              OID_LENGTH(ibmImaNotificationMemoryElements_oid[i]),
                              ASN_OCTET_STR,
                              elementString, 
                              ( strlen(elementString) > memEvent_element_desc[i].colSize)?
                               memEvent_element_desc[i].colSize : strlen(elementString));
            break;  
        case imaSnmpCol_Integer:
        default:
            TRACE(2,"data type %d is not supported yet. \n",memEvent_element_desc[i].colType);
            break;
        }

    }

    TRACE(9, "SNMP: send_v2trap - memory event.\n");
    send_v2trap(var_list);
    snmp_free_varbind(var_list);

    return rc;
}
