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
 * Note: this file provides global definitions for MessageSight MIBs,
 *       as well as their initialization interface. 
 */

#ifndef _IMASNMPMIB_H_
#define _IMASNMPMIB_H_

#include "imaSnmpUtils.h"

#define IMA_SNMP_AGENT  "ibmMessageSightSNMP"

#define IBM_PROD_MIB 1, 3, 6, 1, 4, 1, 2, 6
#define IMA_SNMP_OID 253
#define IMA_SNMP_MIB IBM_PROD_MIB, IMA_SNMP_OID 
#define IMA_SNMP_MGT_OID 3
#define IMA_SNMP_MGT_MIB IMA_SNMP_MIB, IMA_SNMP_MGT_OID
#define IMA_SNMP_STAT_OID 1
#define IMA_SNMP_STAT_MIB IMA_SNMP_MGT_MIB, IMA_SNMP_STAT_OID
#define IMA_SNMP_NOTIFICATIONS_OID  2
#define IMA_SNMP_NOTIFICATIONS_MIB IMA_SNMP_MGT_MIB, IMA_SNMP_NOTIFICATIONS_OID

#define IMA_SNMP_AGENT_REFRESH_CYCLE  2 //2 seconds

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

       int 	ima_snmp_init_mibs();
       void ima_snmp_reinit_mibs();

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPMIB_H_ */

