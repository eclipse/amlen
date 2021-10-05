/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
/* Note: this header file provides definitions for MessageSight's SNMP Thread. */

#ifndef _IMASNMPAGENTINIT_H_
#define _IMASNMPAGENTINIT_H_


/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

#include <ismutil.h>
XAPI int 	ism_snmp_start(void);
XAPI int 	ism_snmp_jsonConfig(int oldValue);
XAPI void 	ism_snmp_stopService(void);
XAPI int	ism_snmp_startService(void);
XAPI int	ism_snmp_isServiceRunning(void);

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPAGENTINIT_H_ */
