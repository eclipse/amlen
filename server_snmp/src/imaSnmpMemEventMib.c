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
 * @brief Memory MIB interface for MessageSight
 * @date July
 *
 * This provides the interface of memory MIB initialization and registration for MessageSight.
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
#include "imaSnmpMemEventMib.h"
#include "imaSnmpMemEventCtrlMib.h"
#include "imaSnmpMemEventTrap.h"

#define  KEY_MEM_FREE_PERCENT "MemoryFreePercent"

int imaSnmpMemEventHandler(ism_json_parse_t *pDataObj)
{
    int rc = ISMRC_OK;
    char *memFreePct;
    int memUsage, memEvent;

    if (NULL == pDataObj)
    {
        TRACE(2, "NULL json object for memory event handler \n");
        return ISMRC_NullPointer;
    }

    memFreePct = (char *)ism_json_getString(pDataObj,KEY_MEM_FREE_PERCENT);
    if (NULL == memFreePct)
    {
        TRACE(2, "failed to find mem percentage in json message\n");
        return rc;
    }
    memUsage = 100 - atoi(memFreePct); 
    if ((memUsage > 100 ) || (memUsage < 0))
    {
        TRACE(2, "invalid mem usage percent %d \n",memUsage);
        return rc;
    }
    memEvent = ima_snmp_mem_events_check(memUsage);

    switch (memEvent)
    {
    case MEMORY_USAGE_EVENT_NONE:
        // no trap event happens
        break;

    case MEMORY_USAGE_ALERT_EVENT:
        TRACE(2,"memory usage alert trap happens, usage %d \n",memUsage);
        rc = send_ibmImaNotificationMemoryUsageAlert_trap(pDataObj);
        break;

    case MEMORY_USAGE_WARNING_EVENT:
        TRACE(2,"memory usage warning trap happens, usage %d \n",memUsage);
        rc = send_ibmImaNotificationMemoryUsageWarning_trap(pDataObj);
        break;

    default:
        TRACE(2, "unknown mem events %d \n",memEvent);
        break;
    }

    return rc;
}

int ima_snmp_init_mem_event_mibs()
{
    int rc = ISMRC_OK;
#ifndef _SNMP_THREADED_
    rc = imaSnmpMqtt_topic_handler_register(imaSnmpTopic_MEMORY, imaSnmpMemEventHandler);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to register memory event topic handler for SNMP agent \n");
        return rc;
    }
 
    rc = ima_snmp_init_mem_ctrl_mibs();
    if (rc != 0)
    {
        TRACE(2, "failed to init memEventCtrl MIBs for MessageSight SNMP service \n");
        return rc;
    }

    if (ima_snmp_mem_events_enabled())
    {
/*
        //setup mqtt client and subscribe to memory event topic.
        if (!imaSnmpMqtt_isConnected())
        {
         // initialize mqtt client and connect to server.
            imaSnmpMqtt_init();
        }
*/
        rc = imaSnmpMqtt_subscribe(imaSnmpTopic_MEMORY);
        if (rc !=  ISMRC_OK)
        {
            TRACE(2,"error in subscribe to memory topic at mem ctrl init\n");
            //return rc;
            //ERROR in subscription should not broken agent initialization procedure.
        }

    }
#else
    rc = imaSnmp_topic_handler_register(imaSnmpTopic_MEMORY, imaSnmpMemEventHandler);
    if (rc != ISMRC_OK)
    {
        TRACE(2, "failed to register memory event topic handler for SNMP agent \n");
        return rc;
    }

    rc = ima_snmp_init_mem_ctrl_mibs();
    if (rc != 0)
    {
        TRACE(2, "failed to init memEventCtrl MIBs for MessageSight SNMP service \n");
        return rc;
    }

    if (ima_snmp_mem_events_enabled())
    {
        rc = imaSnmp_subscribe(imaSnmpTopic_MEMORY);
        if (rc !=  ISMRC_OK)
            TRACE(2,"error in subscribe to memory topic at mem ctrl init\n");
    }
#endif
    return ISMRC_OK;
}

void ima_snmp_reinit_mem_event_mibs()
{
	ima_snmp_reinit_mem_ctrl_mibs();
}
