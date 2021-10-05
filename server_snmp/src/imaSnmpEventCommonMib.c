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
 * @brief Event Common Element MIB interface for MessageSight
 * @date August
 *
 * This provides API of building common event element MIBs  for MessageSight.
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
#include "imaSnmpEventCommonMib.h"

static  const ima_snmp_col_desc_t event_common_element_desc[] = {
   {NULL,imaSnmpCol_None, 0},  // imaSnmpEventCommon_NONE
   {"Version",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpEvent_IMA_VER
   {"NodeName",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpEvent_NODE_NAME
   {"TimeStamp",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpEvent_TIME
   {"ObjectType",imaSnmpCol_String, STR_SIZE_DEFAULT},  // imaSnmpEvent_OBJ_TYPE
};


int ima_snmp_event_set_common_mibs(ism_json_parse_t *pDataObj, netsnmp_variable_list **var_list)
{
    const oid   sample_oid[] = {IMA_SNMP_TRAP_TIME_MIB};
    const oid ibmImaNotificationCommonElements_oid[][OID_LENGTH(sample_oid)] = {
          {IMA_SNMP_TRAP_IMA_VER_MIB},  //just a place holder for idx zero
          {IMA_SNMP_TRAP_IMA_VER_MIB},
          {IMA_SNMP_TRAP_NODE_NAME_MIB},
          {IMA_SNMP_TRAP_TIME_MIB},
          {IMA_SNMP_TRAP_OBJ_TYPE_MIB},
    };
    int rc = ISMRC_OK; 
    int i ;
    char *elementString;

    if (NULL == pDataObj)
    {
        TRACE(2, "null data object for event common elements generation. \n");
        return ISMRC_NullPointer;
    }

    if (NULL == var_list)
    {
        TRACE(2, "null var list for event common elements generation. \n");
        return ISMRC_NullPointer;
    }

    /*
     * Set event common vairables here.
     */
    for (i = imaSnmpEventCommon_MIN ; i < imaSnmpEventCommon_MAX; i++)
    {
        switch (event_common_element_desc[i].colType)
        {
        case imaSnmpCol_String:
            if ((NULL == event_common_element_desc[i].colName)|| 
                (0 == strlen(event_common_element_desc[i].colName)))
               break;
            elementString =  (char *)ism_json_getString(pDataObj, 
                                     event_common_element_desc[i].colName);
            if (NULL == elementString) break;
            snmp_varlist_add_variable(var_list,
                              ibmImaNotificationCommonElements_oid[i],
                              OID_LENGTH(ibmImaNotificationCommonElements_oid[i]),
                              ASN_OCTET_STR,
                              elementString, 
                              ( strlen(elementString) > event_common_element_desc[i].colSize)?
                               event_common_element_desc[i].colSize : strlen(elementString));
            break;  
        case imaSnmpCol_Integer:
        default:
            TRACE(2,"data type %d is not supported yet. \n",event_common_element_desc[i].colType);
            break;
        }

    }

    return rc;
}

