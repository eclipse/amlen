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
 * Note: this header file provides control interface for MessageSight store events management
 */

#ifndef _IMASNMPSTOREEVENTCTRLMIB_H_
#define _IMASNMPSTOREEVENTCTRLMIB_H_

#define STORE_USAGE_EVENT_NONE     0
#define STORE_USAGE_WARNING_EVENT  0x1
#define STORE_USAGE_ALERT_EVENT    0x2
#define STORE_POOL1MEMLOW_ALERT_EVENT    0x4

#define STORE_USAGE_WARNING_ENABLE   1
#define STORE_USAGE_WARNING_DISABLE  2

#define STORE_USAGE_ALERT_ENABLE     1
#define STORE_USAGE_ALERT_DISABLE    2

#define STORE_POOL1MEMLOW_ALERT_ENABLE     1
#define STORE_POOL1MEMLOW_ALERT_DISABLE    2

#define STORE_USAGE_ALERT_TH         90
#define STORE_USAGE_WARNING_TH       80
#define STORE_USAGE_WARNING_DURN     86400
#define STORE_POOL1MEMLOW_ALERT_TH   4096

/* function declarations */
#ifdef __cplusplus
    extern "C" {
#endif

    int 	ima_snmp_init_store_ctrl_mibs();
    void 	ima_snmp_reinit_store_ctrl_mibs();
    int 	ima_snmp_store_events_enabled();
    int 	ima_snmp_store_disk_events_check(int storeDiskUsagePercent);
    int 	ima_snmp_store_pool1_events_check(long storePool1UsedBytes, long storePool1LimitBytes);

#ifdef __cplusplus
    }
#endif

#endif /* _IMASNMPSTOREEVENTCTRLMIB_H_ */

