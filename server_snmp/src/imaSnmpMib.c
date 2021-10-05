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
 * @brief MIB interface for MessageSight
 * @date July
 *
 * This provides the interface of MIB initialization and registration for MessageSight.
 *
 */

#include <stdio.h>
#include <string.h>

#include <ismutil.h>
#include <ismrc.h>
#include <ismjson.h>

#include "imaSnmpMib.h"
#include "imaSnmpServerMib.h"
#include "imaSnmpMemMib.h"
#include "imaSnmpStoreMib.h"
#include "imaSnmpCVMib.h"
#include "imaSnmpEndpointMib.h"
#include "imaSnmpSubscriptionMib.h"
#include "imaSnmpConnectionMib.h"
#include "imaSnmpTopicMib.h"
#include "imaSnmpEventMib.h"

int agentRefreshCycle;

int ima_snmp_init_mibs()
{
    int rc = 0;

    rc = ima_snmp_init_server_mibs();
    if (rc != 0)
    {
       TRACE(2, "SNMP: failed to init server MIB.\n");
       return rc;
    } else {
       TRACE(9, "SNMP: Server MIB is initialized.\n");
    } 

    rc = ima_snmp_init_mem_mibs();
    if (rc != 0)
    {
       TRACE(2, "SNMP: failed to init memory MIB.\n");
       return rc;
    } else {
       TRACE(9, "SNMP: Memory MIB is initialized.\n");
    } 

    rc = ima_snmp_init_store_mibs();
    if (rc != 0)
    {
       TRACE(2, "SNMP: failed to init store MIB.\n");
       return rc;
    } else {
       TRACE(9, "SNMP: Store MIB is initialized.\n");
    } 

    rc = ima_snmp_init_cv_mibs();
    if (rc != 0)
    {
       TRACE(2, "SNMP: failed to init CV MIB.\n");
       return rc;
    } else {
       TRACE(9, "SNMP: CV MIB is initialized.\n");
    } 

    rc = ima_snmp_init_endpoint_table_mibs();
    if (rc != 0)
    {
       TRACE(2, "SNMP: failed to init endpoint MIB.\n");
       return rc;
    } else {
       TRACE(9, "SNMP: Endpoint MIB is initialized.\n");
    } 

    rc = ima_snmp_init_subscription_table_mibs();
    if (rc != 0)
    {
       TRACE(2, "SNMP: failed to init subscription MIB.\n");
       return rc;
    } else {
       TRACE(9, "SNMP: Subscription MIB is initialized.\n");
    } 
    
    rc = ima_snmp_init_connection_table_mibs();
    if (rc != 0)
    {
       TRACE(2, "SNMP: failed to init connection MIB.\n");
       return rc;
    } else {
       TRACE(9, "SNMP: Connection MIB is initialized.\n");
    } 
    
    rc = ima_snmp_init_topic_table_mibs();
    if (rc != 0)
    {
       TRACE(2, "SNMP: failed to init topic MIB.\n");
       return rc;
    } else {
       TRACE(9, "SNMP: Topic MIB is initialized.\n");
    }
    
    TRACE(5, "SNMP: MessageSight MIBs are initialized.\n"); 

    rc = ima_snmp_init_event_mibs();
    if (rc != 0)
    {
       TRACE(2, "SNMP: failed to init trap event MIB.\n");
       return rc;
    }

    TRACE(5, "SNMP: Trap event MIBs are initialized.\n");
    return rc;

}


void ima_snmp_reinit_mibs()
{
    ima_snmp_reinit_event_mibs();
}
