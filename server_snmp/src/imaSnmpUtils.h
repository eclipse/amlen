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


#ifndef _IMASNMPUTILS_H_
#define _IMASNMPUTILS_H_

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ismutil.h>
#include <ismrc.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/library/asn1.h>

/*
 * The wrapper function for SNMP set value.
 * It returns correct value based on the object-type defined in MIB
 */
int ima_snmp_set_var_typed_value(netsnmp_variable_list * requestvb,
    		                     u_char type,
    		                     char * strval,
    		                     size_t len);

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPUTILS_H_ */


