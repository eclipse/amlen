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
 * This is a initialization routine for SNMP AgentX subagent that can run as a thread in
 * imaserver process and communicate with SNMP AgentX master agent.
 *
 */
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <admin.h>
#include <engineInternal.h>
#include <ismutil.h>
#include <janssonConfig.h>
#include "imaSnmpAgentInit.h"
#include "imaSnmpMib.h"

static int keep_running;	/* protected by mutex */
static int snmp_agent_reinit; /* protected by mutex */

static ism_threadh_t snmpAgentThread = NULL;
static pthread_mutex_t snmp_agent_lock;
static pthread_mutex_t *snmp_agent_lock_p = NULL;

static pthread_cond_t snmp_agent_cond;
static pthread_mutex_t snmp_agent_cond_lock;

#define IMA_AGENTX_STR   "tcp6:[::1]:"
int snmp_port;
char *snmp_address = NULL;
char agentXStr[64];

extern int agentRefreshCycle;

static inline void ism_snmp_InitLocks()
{
	if (!snmp_agent_lock_p) {
		snmp_agent_lock_p = &snmp_agent_lock;
		pthread_mutex_init(snmp_agent_lock_p, NULL);
		pthread_mutex_init(&snmp_agent_cond_lock, NULL);
		pthread_cond_init(&snmp_agent_cond, NULL);
	}
}

void ism_snmp_stop_agent() {
	ism_engine_threadTerm(1);
    keep_running = 0;
    snmp_shutdown(IMA_SNMP_AGENT);
}


void *ismSnmpAgentRun(void *param, void *context, int value) {
    int rc = ISMRC_OK;
    int agentx_master = 0;  /* set this to 0 if you want to initialize sub-agent */

    /* AgentRefreshCycle - used to refresh stats cache */
    agentRefreshCycle = IMA_SNMP_AGENT_REFRESH_CYCLE; 

    	memset(agentXStr, 0, 64);
	
    if ( snmp_address ) {
        snprintf(agentXStr, sizeof(agentXStr), "%s%d", snmp_address, snmp_port);
    } else {
        snprintf(agentXStr, sizeof(agentXStr), "%s%d", IMA_AGENTX_STR, snmp_port);
    }

    TRACE(5, "SNMP: Start Agent\n");
    TRACE(5, "SNMP: AgentX Address: %s\n", agentXStr);
    TRACE(5, "SNMP: AgentX Ping Interval: 15 (seconds)\n");
    TRACE(5, "SNMP: Stats refresh interval: %d (seconds)\n", agentRefreshCycle);

    /* Initialize an engine thread for engine statistics */
    ism_engine_threadInit(0);
    if(!ismEngine_threadData)
    {
    	    TRACE(2,"SNMP: Could not create Engine thread\n");
        goto SNMP_THREAD_EXIT;
    }

    if ( agentx_master == 0 ) {
        //Define this application to be a subagent
        netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);
        netsnmp_ds_set_string( NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_X_SOCKET, agentXStr);
        netsnmp_ds_set_int(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_AGENTX_PING_INTERVAL, 15);
    }

    //Start the agent library
    init_agent(IMA_SNMP_AGENT);

    // Initialize MessageSight MIB
    rc = ima_snmp_init_mibs();

    if (rc != 0)
    {
        TRACE(2,"Failed to init SNMP MIBS for MessageSight. "
        		"SNMP Agent Initilization stopped with rc = %d\n", rc);
        return NULL;
    }
    //Net-snmp agent initialization
    init_snmp(IMA_SNMP_AGENT);

    //Catch any any signals
    signal(SIGTERM, ism_snmp_stop_agent);
    signal(SIGINT, ism_snmp_stop_agent);

SNMP_ReInit:
    keep_running = 1;

    /* SNMPAgent_Started = 8701 */
    TRACE(5, "SNMP: Agent is started.\n");

    while (keep_running) {							/* BEAM suppression: infinite loop */
        // Monitor incoming requests continually
        agent_check_and_process(1);
    }

    TRACE(2, "SNMP: Agent has stopped. MessageSight server will try to reinitialize SNMP service.\n");
    pthread_mutex_lock(&snmp_agent_cond_lock);
    while (!snmp_agent_reinit)
    	pthread_cond_wait(&snmp_agent_cond, &snmp_agent_cond_lock);
    ima_snmp_reinit_mibs();
    snmp_agent_reinit = 0;
    pthread_mutex_unlock(&snmp_agent_cond_lock);
    goto SNMP_ReInit;

SNMP_THREAD_EXIT:
    return NULL;
}

/* This routine is only called from main upon initial bring up of imaserver */
XAPI int ism_snmp_start(void)
{
	int rc = ISMRC_OK;

	if ( ism_admin_isSNMPConfigured(0) == 1 ) {
	    TRACE(4, "SNMP Service is enabled\n");
	    ism_snmp_InitLocks();
	    
	    /* Get SNMP Port if configured in server.cfg - default is 705 */
	    snmp_port = ism_common_getIntProperty(ism_common_getConfigProperties(), "SNMPPort", 705);
	    /* Get SNMP master agent address if configured in server.cfg */
	    snmp_address = (char *)ism_common_getStringProperty(ism_common_getConfigProperties(), "SNMPAddress");
	    
	    pthread_mutex_lock(snmp_agent_lock_p);
	    // start a threaded version of the snmp sub agent.
	    rc = ism_common_startThread(&snmpAgentThread, ismSnmpAgentRun, NULL,
								NULL, 0, ISM_TUSAGE_MONITOR, 0,
								"SnmpAgentThread", "SNMP subagent thread");
	    if (rc)
	    {
	        TRACE(2, "Failed to create SNMP subagent thread for IMA, rc = %d \n",rc);
	    }
	    pthread_mutex_unlock(snmp_agent_lock_p);
    } else {
	    TRACE(4, "SNMP Service is not enabled\n");
    }
	
	return rc;
}

/* Conditionally Start or Stop the SNMP Service based off config value */
XAPI int ism_snmp_jsonConfig(int oldValue)
{
	int rc = ISMRC_OK;
	int newValue = ism_admin_isSNMPConfigured(1);
	if (oldValue != newValue)
	{
		if (oldValue > newValue)
			ism_snmp_stopService();
		else
			rc = ism_snmp_startService();
	}
	return rc;
}

/* The following routines start or stop the SNMP Service if SNMPEnabled is set */

XAPI void ism_snmp_stopService(void)
{
	ism_snmp_InitLocks();
	pthread_mutex_lock(snmp_agent_lock_p);
	keep_running = 0;
	pthread_mutex_unlock(snmp_agent_lock_p);
}

XAPI int ism_snmp_startService(void)
{
	int rc = ISMRC_OK;
	ism_snmp_InitLocks();
	pthread_mutex_lock(snmp_agent_lock_p);

	if (keep_running)
		goto out;
	if (!snmpAgentThread)
	{
		snmp_port = ism_common_getIntProperty(ism_common_getConfigProperties(), "SNMPPort", 705);
		rc = ism_common_startThread(&snmpAgentThread, ismSnmpAgentRun, NULL,
									NULL, 0, ISM_TUSAGE_MONITOR, 0,
									"SnmpAgentThread", "SNMP subagent thread");
	}
	else
		snmp_agent_reinit = 1;
	if (rc)
	{
		TRACE(2, "Failed to create SNMP subagent thread for IMA, rc = %d \n",rc);
		goto out;
	}
	pthread_cond_signal(&snmp_agent_cond);
out:
	pthread_mutex_unlock(snmp_agent_lock_p);

	return rc;
}

XAPI int ism_snmp_isServiceRunning(void)
{
	int rc;

	ism_snmp_InitLocks();
	pthread_mutex_lock(snmp_agent_lock_p);
	rc = keep_running;
	pthread_mutex_unlock(snmp_agent_lock_p);

	return rc;
}
