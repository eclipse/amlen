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
 * @brief Event MIB interface for MessageSight
 * @date July
 *
 * This provides the interface of Event MIB initialization and registration for MessageSight.
 *
 */

#include <stdio.h>
#include <string.h>

#include <ismutil.h>
#include <ismrc.h>
#include <ismjson.h>

#include "imaSnmpMib.h"
#include "imaSnmpEventMib.h"
#include "imaSnmpMqttClient.h"
#include "imaSnmpMemEventMib.h"
#include "imaSnmpStoreEventMib.h"

int ima_snmp_init_event_mibs()
{
    int rc = ISMRC_OK;
#ifndef _SNMP_THREADED_
    rc = imaSnmpMqtt_init();
    if (rc != ISMRC_OK)
    {
       TRACE(2, "failed to init MQTT client for MessageSight SNMP agent \n");
       return rc;
    }
#endif
    rc = ima_snmp_init_mem_event_mibs();
    if (rc != 0)
    {
       TRACE(2, "failed to init mem event MIBs for MessageSight SNMP agent \n");
       return rc;
    }

    rc = ima_snmp_init_store_event_mibs();
    if (rc != 0)
    {
       TRACE(2, "failed to init store event MIBs for MessageSight SNMP agent \n");
       return rc;
    }

    return rc;
}

void ima_snmp_reinit_event_mibs()
{
	ima_snmp_reinit_mem_event_mibs();
	ima_snmp_reinit_store_event_mibs();
}

