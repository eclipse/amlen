/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
 * This is the main entry point for MQ Connectivity.
 * For debug purposes a small command line is created.  This will eventually be superseded by
 * admin and monitoring support.
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <assert.h>
#include <config.h>
#include <admin.h>
#include <trace.h>
#include <sys/prctl.h>

#include <mqcInternal.h>
#include <mqcbridgeExternal.h>

extern int ism_protocol_initWSBasic();
extern void ism_common_initTrace();
extern int ism_server_config(char * object, char * namex, ism_prop_t * props, ism_ConfigChangeType_t flag);
extern int mqc_testQueueManagerConnection(const char *qMgrName, const char *connName, const char *channelName, const char *sslCipher, const char *pUserName, const char *pUserPassword);
extern int ism_process_mqc_monitoring_action(const char * locale, const char *inpbuf, int inpbuflen, concat_alloc_t *output_buffer, int *rc);
extern void ism_common_initLoggers(ism_prop_t * props, int mqonly);

static char * g_configfile = NULL;
static char * g_configfile_default = IMA_SVR_DATA_PATH "/data/config/server.cfg";

extern int MQConnectivityEnabled;

#if !defined(_WIN32) && !defined(USE_EDITLINE) && !defined(NO_EDITLINE)
#define USE_EDITLINE 1
#endif
#if USE_EDITLINE
#include <termios.h>
#include <fcntl.h>
#define TERMFILE "/dev/tty"
int    g_fd = -1;
struct termios g_termios;
static void add_cmdhistory(char * cmd);
static void init_readline(void);
typedef  char * (* readline_t)(const char *);
typedef  void   (* add_history_t)(const char *);
readline_t    ism_readline = NULL;
add_history_t ism_add_history = NULL;
#endif

int ism_ha_store_send_admin_msg(void *pAdminMsg){return 0;}
int ism_ha_store_transfer_file(char *pPath, char *pFilename){return 0;}
int ism_store_getStatistics(void *pStatistics){return 0;}
int ism_ha_store_get_view(void *pView){return 0;}
int ism_ha_store_transfer_state_completed(int adminRC){return 0;}

/*
 * Forward references
 */
static void  process_args(int argc, char * * argv);
static int   process_config(const char * name);
static void  syntax_help(void);
static void  runcommands(void);
static int   docommand(char * buf);
static void  sigint_handler(int signo);
static void  sigterm_handler(int signo);

static bool waitingForTermination = false;
static bool terminationRequested = false;
static int trclvl = -1;
static char * trcfile = NULL;
static int daemon_mode = 0;
static ism_simpleServer_t configServer = NULL;

static void adminConnectCB(void) {
    TRACE(5, "Configuration channel to IMA server was established\n");
}

static void adminDisconnectCB(int rc) {
    TRACE(3, "Configuration channel to IMA server was broken with rc=%d. Terminating the process\n", rc);
    ism_common_simpleServer_stop(configServer);
    // Send ourselves a SIGTERM which will cause us to terminate gracefully
    kill(getpid(),SIGTERM);
}

/*
 * Start up MQ Connectivity
 */
int main (int argc, char * * argv) {
    int  rc = 0;

    int staticTraceLevel = 9;
    int dynamicTraceLevel = 5;

    ism_config_t *conHandle = NULL;
    ism_config_t *svrHandle = NULL;

    MQConnectivityEnabled = true;

    // Defect 38832: Commenting out the following fprintf calls - these messages show up on
    // appliance console inter-mixed with login prompt after appliance PXE Install/Upgrade
    //
    // fprintf(stderr, "" IMA_PRODUCTNAME_FULL " MQ Connectivity\n");
    // fprintf(stderr, "Copyright (c) 2012-2021 Contributors to the Eclipse Foundation\n\n");
    // fflush(stderr);

    if ( signal(SIGINT, sigint_handler) == SIG_ERR) {
        fprintf(stderr, "Error: Cannot handle SIGINT\n");
        exit(1);
    }

    // Ensure that if the parent is terminated, we also get terminated - the
    // parent is expected to be the startMQCBridge shell script which can be
    // controlled directly by the imaserver process.
    if ( prctl(PR_SET_PDEATHSIG, SIGTERM) != 0) {
        fprintf(stderr, "Error: Cannot set parent death signal to SIGTERM\n");
        exit(2);
    }

    if ( signal(SIGTERM, sigterm_handler) == SIG_ERR) {
        fprintf(stderr, "Error: Not handling SIGTERM\n");
        exit(3);
    }

    /*
     * Process arguments
     */
    process_args(argc, argv);

    if (g_configfile == NULL)
    {
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
    if (rc)
        return rc;

    /* Override trace level */
    if (trclvl != -1) {
        ism_field_t f;
        f.type = VT_Integer;
        f.val.i = trclvl;
        ism_common_setProperty( ism_common_getConfigProperties(), "TraceLevel", &f);
    }
    /* Override trace file */
    if (trcfile) {
        ism_field_t f;
        f.type = VT_String;
        f.val.s = trcfile;
        ism_common_setProperty( ism_common_getConfigProperties(), "TraceFile", &f);
    }

    ism_common_initTrace();
    TRACE(0, "" IMA_PRODUCTNAME_FULL " MQ Connectivity\n");
    TRACE(0, "Copyright (c) 2012-2021 Contributors to the Eclipse Foundation\n\n");
    ism_trace_startWorker();

    /*
     * To work around build order problem (server_admin is build before server_mqcbridge project,
     * store mqc_test_QueueManagerConnection(const() function pointer
     * in a temporary config item, so that server_admin can get the function pointer to
     * invoke the function at run time.
     */
    ism_field_t fptr;
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)mqc_testQueueManagerConnection;
    ism_common_setProperty(ism_common_getConfigProperties(), "_TEST_QueueManagerConnection_fnptr", &fptr);

    /* Set the function pointer into the ism_process_mqc_monitoring_action */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_process_mqc_monitoring_action;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_process_mqc_monitoring_action_fnptr", &fptr);


    /*
     * Start the admin server.
     */
    rc = ism_admin_init(ISM_PROTYPE_MQC);
    if ( rc )
    	return rc;

    /* We want to run with trace level of 9 - as            */
    /* set in the static configuration file - until it has  */
    /* completed its initialisation. However, the call to   */
    /* ism_server_config will set the trace level to the    */
    /* value in the dynamic config file. That is the value  */
    /* we want for mainstream execution but this is too     */
    /* soon for that setting so we have to save the static  */
    /* value, make the config call, reset the static value  */
    /* and finally set the dynamic value when we are about  */
    /* to process messages.                                 */
    staticTraceLevel = ism_common_getTraceLevel();

    /* Register for the server callbacks.  To get LogLevel and TraceLevel config data. */
    /* ism_admin_init will already have registered a callback, so replace it */
    ism_config_t *hdl = ism_config_getHandle(ISM_CONFIG_COMP_SERVER, NULL);
    ism_config_unregister(hdl);
    rc = ism_config_register(ISM_CONFIG_COMP_SERVER, NULL, mqc_serverCallback, &svrHandle);

    /* Register MQC configuration callback */
    rc = ism_config_register(ISM_CONFIG_COMP_MQCONNECTIVITY, NULL, mqc_adminCallback, &conHandle);
    if (rc)
        return rc;

    /* start mqc domain sockets based server for admin */
    configServer = ism_common_simpleServer_start("/tmp/.com.ibm.ima.mqcAdmin", ism_process_adminMQCRequest, adminConnectCB, adminDisconnectCB);
    if (configServer == NULL)
        return ISMRC_Error;

    /* wait for imaserver to process to send initial configuration
     * via domain sockets based channel.
     */
    rc = ism_admin_mqc_pause();

    /* Get the server dynamic config and run it */
    ism_prop_t *props = ism_config_getComponentProps(ISM_CONFIG_COMP_SERVER);

	/* Initialize loggers */
	ism_common_initLoggers(props, 1);

    ism_server_config(NULL, NULL, props, 0);
    ism_common_freeProperties(props);

    dynamicTraceLevel = ism_common_getTraceLevel();
    ism_common_setTraceLevel(staticTraceLevel);
    TRACE(5, "Set the trace level to %d\n", staticTraceLevel);

    /* Get the dynamic config for MQConnectivity */
    mqcMQConnectivity.props = ism_config_getComponentProps(ISM_CONFIG_COMP_MQCONNECTIVITY);

    /* Set MQConnectivity.MQConnectivityLog */
    ism_server_config(NULL, NULL, mqcMQConnectivity.props, 0);

    /* RC_Starting = 7027 */
    LOG(NOTICE, MQConnectivity, 7027, NULL, "" IMA_PRODUCTNAME_FULL " MQ Connectivity service is starting.");

    /*
     * Start MQ Connectivity
     */
    rc = mqc_start(dynamicTraceLevel);

    waitingForTermination = true;

    if (mqcMQConnectivity.ima.flags & IMA_PURGE_XIDS)
    {
        mqc_rollbackOrphanXIDs();
    }

    if (rc == 0) {
        if ( !daemon_mode ) {
            runcommands();
        } else {
            // Defect 38832: Commenting out the following fprintf call - these messages show up on
            // appliance console inter-mixed with login prompt after appliance PXE Install/Upgrade
            //
            // fprintf(stderr, "The MQ Connectivity process is running in daemon mode.\n");
            while(!(volatile bool)terminationRequested)
            {
                if (pause() == -1) break;
            }
        }
    }
    else {
        fprintf(stderr, "MQ Connectivity failed to start.\n");
    }

    rc = ism_config_unregister(conHandle);
    if (rc)
        return rc;

    /*
     * Terminate MQ Connectivity - if it is no longer enabled we want to tidy
     * up all resources, otherwise we want to just disconnect etc.
     */
    mqc_term(!MQConnectivityEnabled);

    return 0;
}



/*
 * Process args
 */
static void process_args(int argc, char * * argv) {
    int config_found = 1;
    int  i;

    for (i=1; i<argc; i++) {
        char * argp = argv[i];
        if (argp[0]=='-') {
            if (argp[1] >='0' && argp[1]<='9') {
                trclvl = argp[1]-'0';
            } else if (argp[1] == 'h' || argp[1]=='H' || argp[1]=='?' ) {
                config_found = 0;
                break;
            } else if (argp[1] == 't') {
                if (i+1 < argc) {
                    argp = argv[++i];
                    trcfile = argp;
                }
            } else if (argp[1] == 'd') {
                daemon_mode = 1;
            } else if (argp[1] == 'r') {
                mqcMQConnectivity.ima.flags |= IMA_PURGE_XIDS;
            } else {
                fprintf(stderr, "Unknown switch found: %s\n", argp);
                config_found = 0;
                break;
            }
        } else {
            if (g_configfile == NULL) {
                g_configfile = argp;
                config_found = 1;
            } else {
                fprintf(stderr, "Extra parameter found: %s\n", argp);
                config_found = 0;
            }
        }
    }

    if (!config_found) {
        syntax_help();
    }
}


/*
 * Syntax help
 */
void syntax_help(void) {
    fprintf(stderr, "mqcbridge  options  config_file\n\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "    -?         = Show this help\n");
    fprintf(stderr, "    -0 ... -9  = Set the trace level. 0=trace off\n");
    fprintf(stderr, "    -t fname   = Set the trace file name\n");
    fprintf(stderr, "    -d         = Set daemon mode\n");
    fprintf(stderr, "\nconfig_file:\n");
    fprintf(stderr, "    The config file contains a list of keyword=value pairs of\n");
    fprintf(stderr, "    configuration values.\n");
    exit (1);
}


/*
 * Process a config file as keyword=value
 */
int process_config(const char * name) {
    FILE * f;
    char linebuf[4096];
    char * line;
    char * keyword;
    char * value;
    char * more;
    ism_field_t var = {0};

    if (name == NULL)
        return 0;

    f = fopen(name, "rb");
    if (!f) {
        printf("Config file not found: %s\n", name);
        return 1;
    }

    line = fgets(linebuf, sizeof(linebuf), f);
    while (line) {
        keyword = ism_common_getToken(line, " \t\r\n", " =\t\r\n", &more);
        if (keyword && keyword[0]!='*' && keyword[0]!='#') {
            value   = ism_common_getToken(more, " =\t\r\n", "\r\n", &more);
            if (!value)
                value = "";
            var.type = VT_String;
            var.val.s = value;
            ism_common_canonicalName(keyword);
            ism_common_setProperty(ism_common_getConfigProperties(), keyword, &var);
           //printf("config: %s=%s\n", keyword, var.val.s);
        }
        line = fgets(linebuf, sizeof(linebuf), f);
    }

    fclose(f);

    return 0;
}


/*
 * Simple command line
 */
void runcommands(void) {
    char buf[1024];

#ifdef USE_EDITLINE
    /*
     * Initialize readline processing
     */
    init_readline();
#endif

    for (;;) {
        char * xbuf;
        fflush(stdout);
#ifdef USE_EDITLINE
        if (ism_readline) {
            char * xprompt = "> ";
            xbuf = ism_readline(xprompt);
            if (xbuf) {
                /*
                 * If this is a non-null line and not the same as one of
                 * the last two entries in the history file, add it
                 * to the history file.
                 */
                add_cmdhistory(xbuf);
                ism_common_strlcpy(buf, xbuf, sizeof(buf)-1);
                mqc_free(xbuf);
                xbuf = buf;
            }
        } else
#endif
        {
            fprintf(stderr, "\n> ");
            fflush(stderr);
            xbuf = fgets(buf, sizeof(buf), stdin);
        }
        if (docommand(buf) < 0)
            break;
    }
}

/*
 * Do a command
 */
int docommand(char * buf) {
    char * command;
    char * args;
    char   xbuf[256];

    command = ism_common_getToken(buf, " \t\r\n", " \t\r\n", &args);

    if (command) {
        /*
         * config: show configuration properties
         */
        if (!strcmpi(command, "config")) {
            int  i = 0;
            const char * name;
            while (ism_common_getPropertyIndex(ism_common_getConfigProperties(), i, &name)==0) {
                const char * value = ism_common_getStringConfig(name);
                printf("%s=%s\n", name, value);
                i++;
            }
        }
        /*
         * time: Show the time
         */
        else if (!strcmpi(command, "time")) {
            ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
            ism_common_formatTimestamp(ts, xbuf, sizeof(xbuf), 7, ISM_TFF_ISO8601);
            printf("%s\n", xbuf);
            ism_common_closeTimestamp(ts);
        }
        /*
         * tracedump: Dump out the in-memory trace if we're using it
         */
        else if (!strcmpi(command, "tracedump")) {
#ifdef ISM_INMEMORY_TRACE
            if(args != NULL && (strlen(args) > 0) && (args[0] != ' ')) {
                ism_common_memtrace_dump(args);
            } else {
                printf("Error command format should be: tracedump <filename>\n");
            }
#else
            printf("In memory trace is not enabled!\n");
#endif
        }
        /*
         * ima: Dump out IMA details
         */
        else if (!strcmpi(command, "ima")) {
            mqc_printIMA();
        }
        /*
         * ism: Report rename
         */
        else if (!strcmpi(command, "ism")) {
            printf("Note: command renamed to ima - use that instead.\n");
        }
        /*
         * qm: Dump out the qms
         */
        else if (!strcmpi(command, "qms")) {
            mqc_printQMs();
        }
        /*
         * rules: Dump out the rules
         */
        else if (!strcmpi(command, "rules")) {
            mqc_printRules();
        }
        /*
         * removexids: Remove xids that have no known queue manager
         */
        else if (!strcmpi(command, "removexids")) {
            mqc_rollbackOrphanXIDs();
        }
        /*
         * startrule: Start a rule
         */
        else if (!strcmpi(command, "startrule")) {
            if (args)
            {
                mqc_startRuleIndex(atoi(args));
            }
            else
            {
                printf("Error command format should be: startrule <ruleindex>\n");
            }
        }
        /*
         * testconn: Test a queue manager connection
         */
        else if (!strcmpi(command, "testconn")) {
            if (args)
            {
                int rc = 0;

                mqcQM_t *pQM;
                mqcQM_t *pTargetQM = NULL;

                /* Use the given name to scan the known queue managers  */
            	/* until we find the one we were given. Then attempt to */
            	/* connect to it.                                       */

                pthread_mutex_lock(&mqcMQConnectivity.qmListMutex);

                pQM = mqcMQConnectivity.pQMList;

                while (pQM)
                {
                    if (!strcmp(pQM -> pName, args))
                    {
                        pTargetQM = pQM;
                        break;
                    }
                    pQM = pQM -> pNext;
                }

                pthread_mutex_unlock(&mqcMQConnectivity.qmListMutex);

                if (pTargetQM == NULL)
                {
                    printf("Target QM %s not found\n", args);
                }
                else
                {
                    rc = mqc_testQueueManagerConnection(pTargetQM -> pQMName,
                                                        pTargetQM -> pConnectionName,
                                                        pTargetQM -> pChannelName,
                                                        pTargetQM -> pSSLCipherSpec,
                                                        NULL,
                                                        NULL);
                    if (rc == ISMRC_OK)
                    {
                        printf("Connection to %s SUCCEEDED\n", pTargetQM -> pQMName);
                    }
                    else
                    {
                        printf("Connection to %s FAILED\n", pTargetQM -> pQMName);                        
                    }
                }
            }
            else
            {
                printf("Error command format should be: testconn <connection name>\n");
            }
        }
        /*
         * termrule: Terminate a rule
         */
        else if (!strcmpi(command, "termrule")) {
            if (args)
            {
                mqc_termRuleIndex(atoi(args));
            }
            else
            {
                printf("Error command format should be: termrule <ruleindex>\n");
            }
        }
        /*
         * trace: Set trace level
         */
        else if (!strcmpi(command, "trace")) {
            if (args)
            {
                ism_common_setTraceLevel(atoi(args));
            }
            else
            {
                printf("Error command format should be: trace <level>\n");
            }
        }
        /*
         * traceadmin: Set new trace level while admin callback is active
         */
        else if (!strcmpi(command, "traceadmin")) {
            if (args)
            {
                mqcMQConnectivity.traceAdminLevel = atoi(args);
                mqcMQConnectivity.traceAdmin = true;
            }
            else
            {
                printf("Error command format should be: traceadmin <level>\n");
            }
        }
        /*
         * reconnect: Reconnect to IMA
         */
        else if (!strcmpi(command, "reconnect")) {
            mqc_term(false); /* user requested */

            mqc_start(-1); /* dynamic trace level */
        }
        /*
         * quit: Terminate MQ Connectivity
         */
        else if (!strcmpi(command, "quit")) {
        	return -99;
        }
        /*
         * list or help: List of available commands
         */
        else if (!strcmpi(command, "list") || !strcmpi(command, "help") || !strcmpi(command, "?")) {
            printf("List of supported commands:\n");
            printf(" config           Show configuration properties\n");
            printf(" quit             Terminate MQ Connectivity\n");
            printf(" time             Show MQ Connectivity time\n");
            printf(" ima              Dump our IMA details\n");
            printf(" qms              Dump out qms\n");
            printf(" rules            Dump out rules\n");
            printf(" startrule <i>    Start rule index <i>\n");
            printf(" termrule <i>     Terminate rule index <i>\n");
            printf(" trace <level>    Set trace level\n");
            printf(" traceadmin <lvl> Set trace level during admin callback\n");
            printf(" reconnect        Reconnect to IMA\n");
#ifdef ISM_INMEMORY_TRACE
            printf(" tracedump        Dump out in-memory trace buffer\n");
#endif
        }

        /*
         * anything else?
         */
        else {
            printf("Eh?\n");
        }
    }
    return 0;
}


static void sigint_handler(int signo) {
    exit(0);
}

static void sigterm_handler(int signo) {
    if ((volatile bool)waitingForTermination)
    {
        terminationRequested = true;
        return;
    }
    else
    {
        exit(0);
    }
}

#ifdef USE_EDITLINE

/* Some external symbols for readline */
char BC = 0;
char PC = 0;
char UP = 0;

/*
 * Initialize for readline by loading the library and finding
 * the symbols we use.  If readline is not avialable, use the
 * normal stdin processing.
 */
static void init_readline(void) {
    char  errbuf [512];
    LIBTYPE  rlmod;
    char * option;

    /*
     * If the environment variable USE_EDITLINE is set to zero,
     * use stdin processing instead of readline.
     */
    option = getenv("USE_EDITLINE");
    if (option && *option=='0')
        return;

    /*
     * Store current terminal settings
     */
    g_fd = open(TERMFILE, O_RDONLY | O_NOCTTY );
    if (g_fd <= 0) {
        printf("Can not open file %s to get the terminal settings", TERMFILE);
    } else {
        tcgetattr(g_fd, &g_termios);
    }


    /*
     * We do not care what version of the editline library we have so use any version
     */

    rlmod = dlopen("libedit.so" DLLOAD_OPT);/* Use the base symbolic link if it exists*/
    if (!rlmod) {
        rlmod = dlopen("libedit.so.0" DLLOAD_OPT); /* Use editline version in RHEL5 and SLES 10,11 */
    }

    if (rlmod) {
        ism_readline =    (readline_t)    dlsym(rlmod, "readline");
        ism_add_history = (add_history_t) dlsym(rlmod, "add_history");
        if (!ism_readline || !ism_add_history) {
            dlerror_string(errbuf, sizeof(errbuf));
            printf("The editline symbols could not be loaded: %s", errbuf);
        }
    } else {
        dlerror_string(errbuf, sizeof(errbuf));
        TRACE(2, "The editline library could not be loaded: %s", errbuf);
    }
}   /* BEAM suppression:  file leak */


/*
 * Add the string to the history if it is not one of the last two commands used
 */
static void add_cmdhistory(char * cmd) {
    static char cmd_history1[64];
    static char cmd_history2[64];
    int len;

    if (!cmd)
        return;
    len = strlen(cmd);
    if (len<2)
        return;
    if (len < 64) {
        if (!strcmp(cmd_history1, cmd) || !strcmp(cmd_history2, cmd))
            return;
        strcpy(cmd_history2, cmd_history1);
        strcpy(cmd_history1, cmd);
    }
    ism_add_history(cmd);
}
#endif

