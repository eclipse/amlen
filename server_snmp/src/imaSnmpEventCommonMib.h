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
 * Note: this header file provides Event common element MIB definitions for MessageSight,
 *       as well as the interface to build Event common elements. 
 */

#ifndef _IMASNMPEVENTCOMMONELEMENTMIB_H_
#define _IMASNMPEVENTCOMMONELEMENTMIB_H_

#define IMA_SNMP_EVENT_COMMON_ELEMENTS_OID 1
#define IMA_SNMP_EVENT_COMMON_ELEMENTS_MIB IMA_SNMP_NOTIFICATIONS_MIB, IMA_SNMP_EVENT_COMMON_ELEMENTS_OID 

#define IMA_SNMP_TRAP_IMA_VER_OID 1
#define IMA_SNMP_TRAP_IMA_VER_MIB IMA_SNMP_EVENT_COMMON_ELEMENTS_MIB, IMA_SNMP_TRAP_IMA_VER_OID

#define IMA_SNMP_TRAP_NODE_NAME_OID 2
#define IMA_SNMP_TRAP_NODE_NAME_MIB IMA_SNMP_EVENT_COMMON_ELEMENTS_MIB, IMA_SNMP_TRAP_NODE_NAME_OID

#define IMA_SNMP_TRAP_TIME_OID 3
#define IMA_SNMP_TRAP_TIME_MIB IMA_SNMP_EVENT_COMMON_ELEMENTS_MIB, IMA_SNMP_TRAP_TIME_OID

#define IMA_SNMP_TRAP_OBJ_TYPE_OID 4
#define IMA_SNMP_TRAP_OBJ_TYPE_MIB IMA_SNMP_EVENT_COMMON_ELEMENTS_MIB, IMA_SNMP_TRAP_OBJ_TYPE_OID



typedef enum {
    imaSnmpEventCommon_NONE               = 0,
    imaSnmpEvent_IMA_VER                  = 1,    //IMA Version
    imaSnmpEvent_NODE_NAME                = 2,    //Node name
    imaSnmpEvent_TIME                     = 3,    //Trap Time
    imaSnmpEvent_OBJ_TYPE                 = 4,    //Object Type
    imaSnmpEventCommon_MAX,

} imaSnmpEventCommonDataType_t;

#define imaSnmpEventCommon_MIN  imaSnmpEvent_IMA_VER

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

int ima_snmp_event_set_common_mibs(ism_json_parse_t *pDataObj, netsnmp_variable_list **var_list);

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPEVENTCOMMONELEMENTMIB_H_ */

