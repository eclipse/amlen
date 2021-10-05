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
 * Note: this header file provides control interface for MessageSight memory events management
 */

#ifndef _IMASNMPMEMEVENTCTRLMIB_H_
#define _IMASNMPMEMEVENTCTRLMIB_H_

#define MEMORY_USAGE_EVENT_NONE     0
#define MEMORY_USAGE_WARNING_EVENT  0x1
#define MEMORY_USAGE_ALERT_EVENT    0x2

#define MEM_USAGE_WARNING_ENABLE   1
#define MEM_USAGE_WARNING_DISABLE  2

#define MEM_USAGE_ALERT_ENABLE     1
#define MEM_USAGE_ALERT_DISABLE    2

#define MEM_USAGE_ALERT_TH         85
#define MEM_USAGE_WARNING_TH       65
#define MEM_USAGE_WARNING_DURN     86400

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

    int 	ima_snmp_init_mem_ctrl_mibs();
    void 	ima_snmp_reinit_mem_ctrl_mibs();
    int 	ima_snmp_mem_events_enabled();
    int 	ima_snmp_mem_events_check(int memUsagePercent);

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPMEMEVENTCTRLMIB_H_ */

