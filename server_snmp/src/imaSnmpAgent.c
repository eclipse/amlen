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
 * This is a SNMP AgentX subagent that runs as a standalone server for MessageSight
 * SNMP service. It leverages the net-snmp agent API to attach itself to main SNMP agent 
 * and provide access to MessageSight MIBs which are compiled directly into the subagent.
 *
 */
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ismutil.h>
#include <admin.h>
#include <ismc.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "imaSnmpMib.h"

static int keep_running;

#define AGENT_X_SOCKET   "tcp6:[::]:705"

static const char * g_configfile_default = IMA_SVR_DATA_PATH "/data/config/server.cfg";
static const char * g_configfile = NULL;
static const char * trcFile = IMA_SVR_DATA_PATH "/diag/logs/imatracesnmp.log";
static int trclvl = 5;
static int port = 9065;

#undef DUMMY_LINKLOCAL_ADDR

#include <log.h>
#include <config.h>

extern int ism_server_config(char * object, char * namex, ism_prop_t * props, ism_ConfigChangeType_t flag);
extern void ism_common_initTrace();

extern int agentRefreshCycle;

void stop_agent(int a) {
    keep_running = 0;
}

/*
 *  * Process a config file as keyword=value
 *   */
int process_config(const char * name) {
    FILE * f;
    char linebuf[4096];
    char * line;
    char * keyword;
    char * value;
    char * more;
    ism_field_t var = {VT_Null};

    if (name == NULL)
        return 0;

    f = fopen(name, "rb");
    if (!f) {
        fprintf(stderr, "Config file not found: %s\n", name);
        fflush(stderr);
        return 1;
    }

    line = fgets(linebuf, sizeof(linebuf), f);
    while (line) {
        keyword = ism_common_getToken(line, " \t\r\n", " =\t\r\n", &more);
        if (keyword && keyword[0]!='*' && keyword[0]!='#') {
            value   = ism_common_getToken(more, " =\t\r\n", "\r\n", &more);
            if (!value)
                value = (char *)"";
            var.type = VT_String;
            var.val.s = value;
            ism_common_canonicalName(keyword);
            ism_common_setProperty(ism_common_getConfigProperties(), keyword, &var);
        }
        line = fgets(linebuf, sizeof(linebuf), f);
    }

    fclose(f);

    return 0;
}

int snmp_serverCallback(char * object, char * name, ism_prop_t * props, ism_ConfigChangeType_t type) {
    int rc = 0;
    int tmpInt;

    TRACE(8, ">>> %s: name: %s, type: %d\n", __func__, name, type);

    rc = ism_server_config(object, name, props, type);

    tmpInt = ism_common_getIntProperty(props, "TraceLevel", -1);
    if (tmpInt > 0) {
    	ism_field_t f;
    	f.type = VT_Integer;
    	f.val.i = tmpInt;
    	ism_common_setProperty(ism_common_getConfigProperties(), "TraceLevel", &f);
    }

    TRACE(8, "<<< %s, rc: %d\n", __func__, rc);

    return rc;
}


int main(int argc, char **argv) {
    int rc = 0;
    ism_config_t *svrHandle = NULL;
    ism_field_t f;
    //ism_prop_t * traceProps;
    int refresh_cycle = IMA_SNMP_AGENT_REFRESH_CYCLE;

    if (g_configfile == NULL) {
        g_configfile = g_configfile_default;
    }

    /*
     * Initialize the utilities
     */
    ism_common_initUtil();

    /*
     * Process the config file
     */
    rc = process_config(g_configfile);
    if (rc) {
        fprintf(stderr, "Cannot process config file\n");
        fflush(stderr);
        return rc;
    }

    /* Initialize trace. Override the trace file and set trace level to 5 for this process. */
    f.type = VT_String;
    f.val.s = (char *)trcFile;
    ism_common_setProperty(ism_common_getConfigProperties(), "TraceFile", &f);

    f.type = VT_Integer;
    f.val.i = trclvl;
    ism_common_setProperty(ism_common_getConfigProperties(), "TraceLevel", &f);

    ism_common_initTrace();
    TRACE(0, "IBM MessageSight SNMP Agent\n");
    TRACE(0, "Copyright (C) 2014-2021 Contributors to the Eclipse Foundation.\n\n");

    /*
     * Start the admin server.
     */
    TRACE(4, "Start the admin server for SNMP Agent\n");

    rc = ism_admin_init(ISM_PROTYPE_SNMP);
    if ( rc )
        return rc;

    ism_log_init();

    ism_prop_t * props = ism_config_getConfigProps(ISM_CONFIG_COMP_SERVER);
    ism_log_createLogger(LOGGER_SysLog, props);

    /* SNMPAgent_Starting = 8700 */
    TRACE(5, "SNMP: the IBM MessageSight SNMP agent is starting.\n");

    /* Override port */
    port = ism_common_getIntConfig("SnmpAgentPort", port);
    if (port <= 0) {
    	TRACE(1, "Cannot get SNMP agent port\n");
    	return -1;
    }

    ism_simpleServer_t configServer = ism_common_simpleServer_start("/tmp/.com.ibm.ima.snmpAdmin", ism_process_adminSNMPRequest, NULL, NULL);
    if (configServer == NULL)
        return ISMRC_Error;


    /* Register for the server callbacks. */
    rc = ism_config_register(ISM_CONFIG_COMP_SERVER, NULL, snmp_serverCallback, &svrHandle);
    if (rc) {
        TRACE(2, "config register failed: %d\n", rc);
        return rc;
    }

    /* AgentRefreshCycle */
    refresh_cycle = ism_common_getIntConfig("SnmpAgentRefreshCycle", refresh_cycle);
    if (refresh_cycle <= 0)
    {
        TRACE(5,"failed to get refresh cycle fron server config \n");
        agentRefreshCycle = IMA_SNMP_AGENT_REFRESH_CYCLE;
    }
    else agentRefreshCycle = refresh_cycle;
    TRACE(5,"snmpAgent refresh cycle : %d \n",agentRefreshCycle);

    TRACE(5,"start SNMP agent.\n");

    //Define this application to be a subagent
    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);
    netsnmp_ds_set_string( NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_X_SOCKET, AGENT_X_SOCKET);

    //Start the agent library
    init_agent(IMA_SNMP_AGENT);

    // Initialize MessageSight MIB 
    rc = ima_snmp_init_mibs();
    if (rc != 0)
    {
        TRACE(2,"Failed to init SNMP MIBS for MessageSight\n");
        return rc;
    }

    //Net-snmp agent initialization
    init_snmp(IMA_SNMP_AGENT);

    //Catch any any signals
    keep_running = 1;
    signal(SIGTERM, stop_agent);
    signal(SIGINT, stop_agent);

    TRACE(5,"Starting MessageSight SNMP subagent daemon\n");

    /* SNMPAgent_Started = 8701 */
    TRACE(5, "SNMP: the IBM MessageSight SNMP agent is started.\n");

    while (keep_running) {							/* BEAM suppression: infinite loop */
        //Monitor incoming requests continually
        agent_check_and_process(1);
    }

    /* SNMPAgent_Stop = 8702 */
    TRACE(5, "SNMP: the IBM MessageSight SNMP agent is stopped.\n");

    TRACE(5, "MessageSight SNMP Agent Stopped!\n");

    snmp_shutdown(IMA_SNMP_AGENT);

    rc = ism_config_unregister(svrHandle);

    return rc;
}
