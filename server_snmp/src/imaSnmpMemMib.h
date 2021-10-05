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
 * Note: this header file provides definitions for MessageSight memory MIBs,
 *       as well as the interface to access memory MIBs. 
 */

#ifndef _IMASNMPMEMMIB_H_
#define _IMASNMPMEMMIB_H_

#define IMA_SNMP_MEM_OID 2
#define IMA_SNMP_MEM_MIB IMA_SNMP_STAT_MIB, IMA_SNMP_MEM_OID 

#define IMA_SNMP_MEM_TOTALBYTES_OID 1
#define IMA_SNMP_MEM_TOTALBYTES_MIB IMA_SNMP_MEM_MIB, IMA_SNMP_MEM_TOTALBYTES_OID

#define IMA_SNMP_MEM_FREEBYTES_OID 2
#define IMA_SNMP_MEM_FREEBYTES_MIB IMA_SNMP_MEM_MIB, IMA_SNMP_MEM_FREEBYTES_OID

#define IMA_SNMP_MEM_FREEPERCENT_OID 3
#define IMA_SNMP_MEM_FREEPERCENT_MIB IMA_SNMP_MEM_MIB, IMA_SNMP_MEM_FREEPERCENT_OID

#define IMA_SNMP_SVR_VRTL_MEM_BYTES_OID 4
#define IMA_SNMP_SVR_VRTL_MEM_BYTES_MIB IMA_SNMP_MEM_MIB, IMA_SNMP_SVR_VRTL_MEM_BYTES_OID

#define IMA_SNMP_SVR_RSDT_SET_BYTES_OID 5
#define IMA_SNMP_SVR_RSDT_SET_BYTES_MIB IMA_SNMP_MEM_MIB, IMA_SNMP_SVR_RSDT_SET_BYTES_OID

#define IMA_SNMP_GRP_MSG_PAYLOADS_OID 6
#define IMA_SNMP_GRP_MSG_PAYLOADS_MIB IMA_SNMP_MEM_MIB, IMA_SNMP_GRP_MSG_PAYLOADS_OID

#define IMA_SNMP_GRP_PUB_SUB_OID 7
#define IMA_SNMP_GRP_PUB_SUB_MIB IMA_SNMP_MEM_MIB, IMA_SNMP_GRP_PUB_SUB_OID

#define IMA_SNMP_GRP_DESTS_OID 8
#define IMA_SNMP_GRP_DESTS_MIB IMA_SNMP_MEM_MIB, IMA_SNMP_GRP_DESTS_OID

#define IMA_SNMP_GRP_CUR_ACTIVITIES_OID 9
#define IMA_SNMP_GRP_CUR_ACTIVITIES_MIB IMA_SNMP_MEM_MIB, IMA_SNMP_GRP_CUR_ACTIVITIES_OID

#define IMA_SNMP_GRP_CLIENT_STATES_OID 10
#define IMA_SNMP_GRP_CLIENT_STATES_MIB IMA_SNMP_MEM_MIB, IMA_SNMP_GRP_CLIENT_STATES_OID

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

       int ima_snmp_init_mem_mibs();

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPMEMMIB_H_ */

