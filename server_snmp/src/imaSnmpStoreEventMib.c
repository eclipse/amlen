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
 * @brief Store Event MIB interface for MessageSight
 * @date August
 *
 * This provides the interface of store event MIBs initialization and registration for MessageSight.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <ismutil.h>
#include <ismjson.h>
#include <ismrc.h>

#include "imaSnmpMib.h"
#include "imaSnmpMqttClient.h"
#include "imaSnmpStoreEventMib.h"
#include "imaSnmpStoreEventCtrlMib.h"
#include "imaSnmpStoreEventTrap.h"

#define  KEY_STORE_DISK_USAGE_PERCENT "DiskUsedPercent"
#define  KEY_STORE_POOL1_USED_BYTES "Pool1RecordsUsedBytes"
#define  KEY_STORE_POOL1_LIMIT_BYTES "Pool1RecordsLimitBytes"

int imaSnmpStoreEventHandler(ism_json_parse_t *pDataObj)
{
    int rc = ISMRC_OK;
    char *storeDiskUsagePct;
    int storeDiskUsage, storeDiskEvent;
    char *sStorePool1LimitBytes, *sStorePool1UsedBytes;
    long storePool1UsedBytes, storePool1LimitBytes;
    int  storePool1Event;

    if (NULL == pDataObj)
    {
        TRACE(2, "NULL json object for store event handler \n");
        return ISMRC_NullPointer;
    }

    storeDiskUsagePct = (char *)ism_json_getString(pDataObj,KEY_STORE_DISK_USAGE_PERCENT);
    if (NULL == storeDiskUsagePct)
    {
        TRACE(2, "failed to find store disk usage percentage in json message\n");
        return rc;
    }
    sStorePool1UsedBytes = (char *)ism_json_getString(pDataObj,KEY_STORE_POOL1_USED_BYTES);
    if (NULL == sStorePool1UsedBytes)
    {
        TRACE(2, "failed to find store pool1 memory record used bytes in json message\n");
        return rc;
    }
    sStorePool1LimitBytes = (char *)ism_json_getString(pDataObj,KEY_STORE_POOL1_LIMIT_BYTES);
    if (NULL == sStorePool1LimitBytes)
    {
        TRACE(2, "failed to find store pool1 memory record limit bytes in json message\n");
        return rc;
    }
    storePool1UsedBytes = atol(sStorePool1UsedBytes);
    storePool1LimitBytes = atol(sStorePool1LimitBytes);

    storePool1Event = ima_snmp_store_pool1_events_check(storePool1UsedBytes,storePool1LimitBytes);
    if (storePool1Event & STORE_POOL1MEMLOW_ALERT_EVENT)
    {
        TRACE(2,"store pool1 low memory alert trap happens, memoryUsed %lu , memoryLimit %lu\n",
              storePool1UsedBytes,storePool1LimitBytes);
        rc = send_ibmImaNotificationStorePool1MemLowAlert_trap(pDataObj);
    }

    storeDiskUsage = atoi(storeDiskUsagePct); 
    if ((storeDiskUsage > 100 ) || (storeDiskUsage < 0))
    {
        TRACE(2, "invalid store disk usage percent %d \n",storeDiskUsage);
        return rc;
    }
    storeDiskEvent = ima_snmp_store_disk_events_check(storeDiskUsage);

    switch (storeDiskEvent)
    {
    case STORE_USAGE_EVENT_NONE:
        // no trap event happens
        break;

    case STORE_USAGE_ALERT_EVENT:
        TRACE(2,"store disk usage alert trap happens, usage %d \n",storeDiskUsage);
        rc = send_ibmImaNotificationStoreDiskUsageAlert_trap(pDataObj);
        break;

    case STORE_USAGE_WARNING_EVENT:
        TRACE(2,"store disk usage warning trap happens, usage %d \n",storeDiskUsage);
        rc = send_ibmImaNotificationStoreDiskUsageWarning_trap(pDataObj);
        break;

    default:
        TRACE(2, "unknown store events %d \n",storeDiskEvent);
        break;
    }

    return rc;
}

int ima_snmp_init_store_event_mibs()
{
    int rc = ISMRC_OK;
#ifndef _SNMP_THREADED_
    rc = imaSnmpMqtt_topic_handler_register(imaSnmpTopic_STORE, imaSnmpStoreEventHandler);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to register store event topic handler for SNMP agent \n");
        return rc;
    }
 
    rc = ima_snmp_init_store_ctrl_mibs();
    if (rc != 0)
    {
        TRACE(2, "failed to init storeEventCtrl MIBs for MessageSight SNMP service \n");
        return rc;
    }

    if (ima_snmp_store_events_enabled())
    {
/*
        //setup mqtt client and subscribe to store event topic.
        if (!imaSnmpMqtt_isConnected())
        {
         // initialize mqtt client and connect to server.
            imaSnmpMqtt_init();
        }
*/
        rc = imaSnmpMqtt_subscribe(imaSnmpTopic_STORE);
        if (rc !=  ISMRC_OK)
        {
            TRACE(2,"error in subscribe to store topic at mem ctrl init\n");
            //return rc;
            //ERROR in subscription should not broken agent initialization procedure.
        }

    }
#else
    rc = imaSnmp_topic_handler_register(imaSnmpTopic_STORE, imaSnmpStoreEventHandler);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to register memory event topic handler for SNMP agent \n");
        return rc;
    }

    rc = ima_snmp_init_store_ctrl_mibs();
    if (rc != 0)
    {
        TRACE(2, "failed to init memEventCtrl MIBs for MessageSight SNMP service \n");
        return rc;
    }

    if (ima_snmp_store_events_enabled())
    {
        rc = imaSnmp_subscribe(imaSnmpTopic_STORE);
        if (rc !=  ISMRC_OK)
            TRACE(2,"error in subscribe to memory topic at mem ctrl init\n");
    }
#endif
    return ISMRC_OK;
}

void ima_snmp_reinit_store_event_mibs()
{
	ima_snmp_reinit_store_ctrl_mibs();
}
