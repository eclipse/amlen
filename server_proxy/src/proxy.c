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
 * This is the main entry point for the imaserver.  Its primary role is to start each of the
 * components.
 * For debug purposes a small command line is created.  This will eventually be superceded by
 * admin and monitoring support.
 */
#include <ismutil.h>
#include <ismjson.h>
#ifdef PX_CLIENTACTIVITY
#include <pxactivity.h>
#endif
#include <pxtransport.h>
#include <signal.h>
#include <assert.h>
#include <limits.h>
#include <linux/oom.h>
#include <sys/resource.h>
#include <log.h>
#include <tenant.h>
#include <selector.h>
#include <throttle.h>
#include <pxtcp.h>
#include <javaconfig.h>
#include <sys/prctl.h>
#include <curl/curl.h>
#ifndef TRACE_COMP
#define TRACE_COMP Proxy
#endif

#if !defined(_WIN32) && !defined(USE_EDITLINE) && !defined(NO_EDITLINE)
#define USE_EDITLINE 1
#endif
#if USE_EDITLINE
#include <termios.h>
#include <fcntl.h>
#define TERMFILE "/dev/tty"
int    g_fd = -1;
extern  int  g_bigLog;
struct termios g_termios;
static void add_cmdhistory(char * cmd);
static void init_readline(void);
typedef  char * (* readline_t)(const char *);
typedef  void   (* add_history_t)(const char *);
readline_t    ism_readline = NULL;
add_history_t ism_add_history = NULL;
#endif

#ifndef XSTR
#define XSTR(s) STR(s)
#define STR(s) #s
#endif

xUNUSED static char * versionString = "version_string: imaproxy " XSTR(ISM_VERSION) " " XSTR(BUILD_LABEL) " " XSTR(ISMDATE) " " XSTR(ISMTIME);

static int httpProxyEnabled = 0;
static int adminRestEnabled = 0;
static int mhubEnabled = 0;

/*
 * Forward references
 */
static int  initProxy(void);
static int   process_args(int argc, char * * argv, int pass);
static void  process_args2(int argc, char * * argv);
static void  syntax_help(void);
static void *  runcommands(void * param, void * conext, int intparam);
static int   docommand(char * buf);
static void  sig_handler(int signo);
static void sigterm_handler(int signo);
static void siguser2_handler(int signo);

extern void ism_common_initTrace(void);
extern void ism_common_initTimers(void);
extern void ism_proxy_notify_init(void);
extern int ism_log_startLogTableCleanUpTimerTask(void);
extern int ism_protocol_printACL(const char * name);
extern int ism_protocol_initACL(void);

static int trclvl = -1;
static char * trcfile = NULL;
static int daemon_mode = 0;
static const char * servername = NULL;
static int inpause = 0;
static volatile int doterm = 0;
static volatile int doUser2 = 0;
static ism_threadh_t cmdthread;
#ifdef PX_CLIENTACTIVITY
static int pxact=0;
void ism_proxy_setClientActivityMonitoring(int enable);
#endif

void ism_tenant_printTenants(const char * name);
void ism_tenant_printUsers(const char * name);
int  ism_protocol_initMQTT(void);
int  ism_iotrest_init(int step);
int  ism_proxy_restConfigInit(void);
int  ism_proxy_addConfigPath(ism_json_parse_t * parseobj, int where);
int  ism_proxy_process_config(const char * name, const char * dname, int complex, int checkonly, int keepgoing);
void ism_proxy_setAuxLog(ism_domain_t * domain, int logger, ism_prop_t * props, const char * logname);
int  ism_proxy_javainit(int step);
void ism_proxy_nojavainit(void);
int  ism_monitor_startMonitoring(void);
void ism_proxy_addSelf(ism_acl_t * acl);
void ism_common_initUUID(void);
void ism_proxy_shutdownWait(void);
void ism_proxy_shutdownDelay(void);
int  ism_transport_stopMessaging(void);
int  ism_common_getDisableCRL(void);

static int startMessagingTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata);
const char * ism_proxy_getVersion(void);
extern int ism_proxy_statsd_init(void);
int ism_kafka_setUseKafkaMetering(int useKafka);
extern int g_useKafka;
extern char g_procname [20];
extern void ism_tenant_term(void);
extern int ism_proxy_termAuth(void) ;
XAPI void ism_ssl_init(int useFips, int useBufferPool);
void ism_proxy_sendAllACLs(const char * aclsrc, int acllen);
void ism_transport_closeAllConnections(int notAdmin, int noKafka);
XAPI int ism_common_setServerName(const char * name);
int ism_mhub_init(void);
int ism_mhub_start(void);
void ism_mhub_printMhub(struct ism_mhub_t * mhub, const char * xmhub);
void ism_mhub_printStats(void);
int  ism_common_hashTrustDirectory(const char * dirpath, int leave_links, int verbose);
extern const char * g_truststore;
int ism_ssl_downloadCRL(char * uri, const char * org, uint64_t sincetime, long * filetime);
int ism_proxy_getJSONn(concat_alloc_t * buf, int otype, const char * name, const char * name2);

/*
 * Start up the server
 */
int main (int argc, char * * argv) {
    int  rc;
    ism_field_t  f;
    char      thread_name [20];

    /*
     * Process arguments
     */
    process_args(argc, argv, 1);
    fprintf(stderr, IMA_PRODUCTNAME_FULL " Proxy\n");
    fprintf(stderr, "Copyright (C) 2013, 2021 Contributors to the Eclipse Foundation.\n\n");
    fflush(stderr);

    /*
     * Initialize the utilities
     */
    ism_common_initUtil();
    ism_common_setTraceLevel(9);
    ism_common_setTraceFile("stdout", 0);

    /* Catch various signals and flush the trace file */
    signal(SIGINT, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGTERM, sigterm_handler);
    signal(SIGUSR2, siguser2_handler);

    /*Ignore SIGPIPE signals for broken TCP connections.*/
    sigignore(SIGPIPE);
    sigignore(SIGHUP);

    /* Cause a parent death to send up a SIGTERM */
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    if (process_args(argc, argv, 2)) {
        printf("The proxy configuration file is not valid.\n");
        exit(2);
    }

    /* Override trace level */
    if (trclvl != -1) {
        f.type = VT_Integer;
        f.val.i = trclvl;
        ism_common_setProperty(ism_common_getConfigProperties(), "TraceLevel", &f);
    }

    /* Override trace file */
    if (trcfile) {
        f.type = VT_String;
        f.val.s = trcfile;
        ism_common_setProperty(ism_common_getConfigProperties(), "TraceFile", &f);
    }
    
#ifdef PX_CLIENTACTIVITY
    pxact = ism_common_getIntConfig(PXACT_CFG_ENABLE_ACTIVITY_TRACKING, 0);
	f.type = VT_Integer;
    f.val.i = pxact;
    ism_common_setProperty(ism_common_getConfigProperties(), PXACT_CFG_ENABLE_ACTIVITY_TRACKING, &f);
#endif

    /*
     * Initialize the trace and make the initial entries
     */
    ism_common_initTrace();
    TRACE(1, "Initialize " IMA_PRODUCTNAME_FULL " Proxy\n");
    TRACE(2, "Copyright (C) 2013, 2021 Contributors to the Eclipse Foundation\n");
    TRACE(2, "version   = %s %s %s\n", ism_common_getVersion(), ism_common_getBuildLabel(), ism_common_getBuildTime());
    TRACE(2, "pxversion = %s\n", versionString);
    TRACE(3, "soversion = %s\n", ism_proxy_getVersion());
    TRACE(2, "platform  = %s %s\n", ism_common_getPlatformInfo(), ism_common_getKernelInfo());
    TRACE(2, "processor = %s\n", ism_common_getProcessorInfo());

#ifdef PX_CLIENTACTIVITY
    TRACE(2, "ActivityMonitoringEnable = %d\n", pxact);
    ism_proxy_setClientActivityMonitoring(pxact);
    if (pxact) {
        /* PXACT
         * Initialize the activity tracking, should be called before javaconfig.
         * It's important that this is done while we are still single threaded.
         */
        rc = ism_pxactivity_init();
        if (rc != ISMRC_OK) {         //The only failure possible here is ISMRC_AllocateError
            LOG(CRIT, Server, 935, "%s", "Unable to initialize: {0}", "pxactivity");
            printf("Unable to initialize pxactivity: rc=%d\n", rc);
            ism_common_sleep(100000);
            exit(2);
        }
    }
#endif

    /*
     * Start the Java config processing
     * Java messes with the thread name of the main thread so reset it after the java init.
     */
    rc = prctl(PR_GET_NAME, (unsigned long)(uintptr_t)thread_name);
    if (rc)
        strcpy(thread_name, "imaproxy");

    rc = ism_proxy_javainit(1);

    prctl(PR_SET_NAME, (unsigned long)(uintptr_t)thread_name);

    if (rc) {
        LOG(CRIT, Server, 935, "%s", "Unable to initialize: {0}", "javaconfig");
        printf("Unable to initialize java config: rc=%d\n", rc);
        ism_common_sleep(100000);
        exit(2);
    }

    /*
     * Initialize TLS
     */
    int sslUseBufferPool = ism_common_getIntConfig("TlsUseBufferPool", 0);
    TRACE(5, "sslUseBufferPool Configuration: %d\n", sslUseBufferPool);
    ism_ssl_init(0, sslUseBufferPool);

    /*
     * Initialize curl - it's important that this is done while
     * we are still single threaded.
     */
    CURLcode cRC = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (cRC != CURLE_OK) {
        TRACE(1, "Curl Global init failed: %s\n", curl_easy_strerror(cRC));
    }

    /*
     * Initialize the proxy.
     */
    rc = initProxy();
    if (rc) {
        LOG(CRIT, Server, 935, "%s", "Unable to initialize: {0}", "proxy");
        printf("Unable to initialize the proxy: rc=%d\n", rc);
        ism_common_sleep(100000);
        exit(2);
    }

    /*
     * Init the transport
     */
    rc = ism_transport_init();
    if (rc) {
        LOG(CRIT, Server, 935, "%s", "Unable to initialize: {0}", "transport");
        printf("Unable to initialize transport: rc=%d\n", rc);
        ism_common_sleep(100000);
        exit(2);
    }

    /* Initialize the MQTT protocol */
    ism_protocol_initMQTT();

    /* Initialize the http protocol */
    httpProxyEnabled = ism_common_getIntConfig("httpProxyEnabled", 1);
    if (httpProxyEnabled)
        ism_iotrest_init(1);

    adminRestEnabled = ism_common_getIntConfig("AdminRestEnabled", 1);
    if (adminRestEnabled)
        ism_proxy_restConfigInit();

    mhubEnabled = ism_common_getIntConfig("MessageHubEnabled", 0);
    if (mhubEnabled)
        ism_mhub_init();

    /* Start transport */
    rc = ism_transport_start();
    if (rc) {
        LOG(CRIT, Server, 936, "%s", "Unable to start: {0}", "transport");
        printf("Unable to start transport: rc=%d\n", rc);
        ism_common_sleep(100000);
        exit(2);
    }

    /*
     * Init complex objects
     */
    process_args2(argc, argv);

    if (httpProxyEnabled)
        ism_iotrest_init(2);


    /*
     * Initialize and start monitoring
     */
    ism_monitor_startMonitoring();

    ism_proxy_javainit(2);

    if(ism_common_getIntConfig("statsdClientEnabled", 0)) {
        ism_proxy_statsd_init();
    }

    if (mhubEnabled) {
        ism_mhub_start();
    }

    /*
     * Start receiving messages
     */
    int  start_msg_delay = ism_common_getIntConfig("StartMessaging", 1000);
    if (start_msg_delay > 0) {
        ism_common_setTimerOnce(ISM_TIMER_HIGH, startMessagingTimer, NULL, ((uint64_t)start_msg_delay) * 1000000);
    }

    /* Start LogTable Clean Up Task*/
    ism_log_startLogTableCleanUpTimerTask();

    /* Initialize the Delay Process */
    ism_throttle_initThrottle();

    /* Parse throttle configuration */
    ism_throttle_parseThrottleConfiguration();

    /* Start Task to clean up the client table.*/
    if (ism_throttle_isEnabled())
        ism_throttle_startDelayTableCleanUpTimerTask();

    /*
     * Run the command line
     */
    if (!daemon_mode) {
        ism_common_startThread(&cmdthread, runcommands, NULL, NULL, 0, ISM_TUSAGE_NORMAL, 0, "command",
            "Run commands for non-daemon mode");
    }
    inpause = 1;

    /*
     * Not much is done in the main thread but we do check for a SIGTERM
     * and for a user signal.  We use atomics for these as it is not done
     * often and there is a small chance that there is no memory barrier.
     */
    for (;;) {
        doterm = __sync_add_and_fetch(&doterm, 0);   /* Guarantee memory using atomic */
        if (doterm)
            break;
        while (__sync_fetch_and_and(&doUser2, 0)) {     /* We might get the user signal before the first pause */
            ism_common_runUserHandlers();
        }
        pause();
    }
    inpause = 0;

    /*
     * Do graceful termination
     */
    if (doterm) {
        TRACE(1, "Terminate proxy - start controlled shutdown\n");
        /* Wait for query from load balancer */
        ism_proxy_shutdownWait();
        ism_transport_stopMessaging();
        TRACE(2, "Terminate proxy - messaging is stopped\n");
        ism_proxy_shutdownDelay();
        ism_transport_term();
        TRACE(2, "Terminate proxy - transport is disabled\n");
        /* Terminate Auth */
        ism_proxy_termAuth();
        /* check the curl global init state */
        if (cRC == CURLE_OK) {
            curl_global_cleanup();
        }
#ifdef PX_CLIENTACTIVITY
        ism_pxactivity_term();
#endif
    }
}


/*
 * Start messaging
 */
static int startMessagingTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    ism_transport_startMessaging();
    ism_common_cancelTimer(timer);
    return 0;
}

int ism_log_init2(int exetype);

/*
 * Start the IMA server.
 * All initialization of the server is done in this function, but it is not run.
 * If this method returns a good return code the server will be run.
 * @return A return code, 0=good.
 */
static int  initProxy(void) {
    struct rlimit rlim;
    const char * trcopt;
    uint64_t  trcsize;
    int       rc = 0;

    /*
     * Process file limits
     */
    int filelimit = ism_common_getIntConfig("FileLimit", 0);
    if (filelimit > 0) {
        if (filelimit < 1024)
            filelimit = 1024;
        rlim.rlim_max = filelimit;
        rlim.rlim_cur = filelimit;
        setrlimit(RLIMIT_NOFILE, &rlim);
        getrlimit(RLIMIT_NOFILE, &rlim);
        if (rlim.rlim_cur < rlim.rlim_max) {
            rlim.rlim_cur = rlim.rlim_max;
            setrlimit(RLIMIT_NOFILE, &rlim);
            getrlimit(RLIMIT_NOFILE, &rlim);
        }
        TRACE(4, "Set file limit=%u\n", (uint32_t)rlim.rlim_cur);
    }

    /*
     * Do some additional trace config
     */
    trcopt = ism_common_getStringConfig("TraceMax");
    trcsize = ism_common_getBuffSize("TraceMax", trcopt, "20M");
    if (trcsize < 250000)
        trcsize = 250000;
    ism_common_setTraceMax(trcsize);
    TRACE(4, "Set the maximum trace size to: %llu\n", (ULL)trcsize);
    trcopt = ism_common_getStringConfig("TraceConnection");
    if (trcopt)
        ism_common_setTraceConnection(trcopt);

#if 0
    rlim.rlim_max = RLIM_INFINITY;
    rlim.rlim_cur = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rlim);
    getrlimit(RLIMIT_CORE, &rlim);
    TRACE(4, "Set core file size limit=%u\n", (uint32_t)rlim.rlim_cur);
#endif

    /*
     * Set the server name which is also used as the authentication realm
     */
    if (!servername)
        servername = ism_common_getStringConfig("ServerName");
    if (!servername)
        servername = "WatsonIoT";
    ism_field_t var = {0};
    var.type = VT_String;
    var.val.s = (char *)servername;
    ism_common_setProperty(ism_common_getConfigProperties(), "ServerName", &var);
    ism_common_setServerName(servername);

    /*
     * Initialize the loggers and log the startup
     */
    ism_log_init2(1);

    ism_proxy_setAuxLog(&ism_defaultDomain, LOGGER_SysLog, ism_common_getConfigProperties(), "LogLevel");
    ism_proxy_setAuxLog(&ism_defaultDomain, LOGGER_Connection, ism_common_getConfigProperties(), "ConnectionLogLevel");

    ism_log_initSyslog(ism_common_getConfigProperties());
    ism_log_createLogger(LOGGER_SysLog, ism_common_getConfigProperties());
    ism_log_createLogger(LOGGER_Connection, ism_common_getConfigProperties());

    LOG(INFO, Server, 900, "%s %s", "Start the IBM Watson IoT Platform proxy ({0} {1}).",
            ism_common_getVersion(), ism_common_getBuildLabel());

    /*
     * Start timer threads
     */
    ism_common_initTimers();

    /*
     * Init ACL
     */
    const char * aclfile = ism_common_getStringConfig("ACLfile");
    if (aclfile) {
        ism_protocol_setACLfile(aclfile, ism_proxy_addSelf);
    }

    /*
     * Hash the truststore
     */
    if (g_truststore) {
        char * trustdir = alloca(strlen(g_truststore) + 16);
        strcpy(trustdir, g_truststore);
        strcat(trustdir, "/client");
        ism_common_hashTrustDirectory(trustdir, 0, 0);
    }

    /*
     * Start notify processing
     */
    ism_proxy_notify_init();

    ism_common_initUUID();
    return rc;
}

/*
 * Process args
 */
static int process_args(int argc, char * * argv, int pass) {
    int config_found = 0;
    int  i;
    int  xrc = 0;
    int  rc;

    for (i=1; i<argc; i++) {
        char * argp = argv[i];
        if (argp[0]=='-') {
            /* -0 to -9 test trace level */
            if (argp[1] >='0' && argp[1]<='9') {
                trclvl = argp[1]-'0';
            }


            /* -h -H -? = syntax help */
            else if (argp[1] == 'h' || argp[1]=='H' || argp[1]=='?' ) {
                config_found = 0;
                break;
            }

            /* -t = trace file */
            else if (argp[1] == 't') {
                if (i+1 < argc) {
                    argp = argv[++i];
                    trcfile = argp;
                }
            }

            /* -d = daemon mode (no command line) */
            else if (argp[1] == 'd') {
                daemon_mode = 1;
            }

            /* -n = server name */
            else if (argp[1] == 'n') {
                argp = argv[++i];
                servername = argp;
            }

            //PXACT
            /* -a = enable pxActivity */
            else if (argp[1] == 'a') {
#ifdef PX_CLIENTACTIVITY
                pxact = 1;
#endif
            }

            /* Anything else is an error */
            else {
                fprintf(stderr, "Unknown switch found: %s\n", argp);
                config_found = 0;
                break;
            }
        } else {
            config_found = 1;
            if (pass > 1) {
                rc = ism_proxy_process_config(argp, NULL, 0, 0, 1);
                if (!xrc)
                    xrc = rc;
            }
        }
    }

    if (!config_found) {
        syntax_help();
    }
    return xrc;
}

/*
 * Process args after init, only complex objects are processed
 */
static void process_args2(int argc, char * * argv) {
    int  i;

    for (i=1; i<argc; i++) {
        char * argp = argv[i];
        if (argp[0] != '-') {
            ism_proxy_process_config(argp, NULL, 1, 0, 1);
        } else {
            if (argp[1] == 'n' || argp[1] == 't')
                i++;
        }
    }
}

/*
 * Syntax help
 */
void syntax_help(void) {
    fprintf(stderr, "imaproxy  options  config_files \n\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "    -?         = Show this help\n");
    fprintf(stderr, "    -0 ... -9  = Set the trace level. 0=trace off\n");
    fprintf(stderr, "    -t fname   = Set the trace file name\n");
    fprintf(stderr, "    -d         = Set daemon mode (no command line)\n");
    fprintf(stderr, "\nconfig_files:\n");
    fprintf(stderr, "    One or more config files in JSON format.\n");
    exit (1);
}


/*
 * Simple command line
 */
void * runcommands(void * param, void * context, int intparam) {
    char buf[1024];

#ifdef USE_EDITLINE
    /*
     * Initialize editline processing
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
                /*
                 * Buffer comes from dlsym
                 */
                ism_common_free_raw(ism_memory_proxy_utils,xbuf);
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
    return NULL;
}

static int test_callback(void * xtransport) {
    ism_transport_t * transport = (ism_transport_t *)xtransport;
    printf("handler %s\n", transport->name);
    if (*transport->name != '3')
        return -1;
    return 0;
}

/*
 * Do a command
 */
int docommand(char * buf) {
    char * command;
    char * args;
    char   xbuf[2048];
    int    rc;
    int    count;

    command = ism_common_getToken(buf, " \t\r\n", " \t\r\n", &args);

    if (command) {
        /*
         * config: show configuration properties
         */
        if (!strcmpi(command, "config")) {
            int  i = 0;
            const char * name;
            if (!args || !*args)
                args = "*";
            while (ism_common_getPropertyIndex(ism_common_getConfigProperties(), i, &name)==0) {
                const char * value = ism_common_getStringConfig(name);
                if (ism_common_match(name, args))
                    printf("%s=%s\n", name, value);
                i++;
            }
        }

        /*
         * ACL command
         */
        else if (!strcmpi(command, "acl")) {
            if (!args || !*args)
                args = "*";
            ism_protocol_printACL(args);
        }

        /*
         * SetACL command
         */
        else if (!strcmpi(command, "setacl")) {
            if (!args || !*args) {
                printf("setacl: file name required\n");
            } else {
                int sarc = ism_protocol_setACLfile2(args, ism_proxy_addSelf, ism_proxy_sendAllACLs);
                if (sarc) {
                    printf("setacl: rc=%d\n", sarc);
                }
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
         * quit: Terminate the server
         */
        else if (!strcmpi(command, "quit")) {
            raise(SIGTERM);
            doterm = 1;
            return -99;
        }

        /*
         * list or help: List of available commands
         */
        else if (!strcmpi(command, "list") || !strcmpi(command, "help") || !strcmpi(command, "?")) {
            printf("List of supported commands:\n");
            printf(" config               Show the global config properties\n");
            printf(" disconnect name*     Disconnect all connections from the named endpoints\n");
            printf(" endpoint [name*]     Show the endpoints\n");
            printf(" help                 Show this help\n");
            printf(" mhub tenant [name*]  Show the MessageHub objects\n");
            printf(" mstat                Show MessageHub stats\n");
            printf(" password user ps     Create a hashed password in JSON format\n");
            printf(" quit                 Terminate the proxy\n");
            printf(" server [name*]       Show the servers\n");
            printf(" tenant [name*]       Show the tenants\n");
            printf(" time                 Show proxy time\n");
            printf(" traceLevel           Show or set the trace level\n");
            printf(" tuser tenant [name*] Show the users for a tenant (use . for null tenant)\n");
            printf(" user  [name*]        Show the global users\n");
        }

        else if (!strcmpi(command, "endpoint")) {
            if (!args || !*args)
                args = "*";
            ism_transport_printEndpoints(args);
        }
        else if (!strcmpi(command, "server")) {
            if (!args || !*args)
                args = "*";
            ism_tenant_printServers(args);
        }
        else if (!strcmpi(command, "tenant")) {
            if (!args || !*args)
                args = "*";
            ism_tenant_printTenants(args);
        }
        else if (!strcmpi(command, "crl")) {
            char * crluri = ism_common_getToken(args, " \t,", " \t,", &args);
            char * timesec = ism_common_getToken(args, " \t,", " \t,", &args);
            uint64_t sincetime = 0;
            long filetime;
            if (!crluri) {
                printf("crl command format: uri seconds\n");
            } else {
                if (timesec && *timesec) {
                    sincetime = (ism_common_currentTimeNanos()/1000000000L) - atoi(timesec);
                }
                TRACE(4, "CRL command: %s %s\n", crluri, timesec ? timesec : "");
                int ec = ism_ssl_downloadCRL(crluri, "testorg", sincetime, &filetime);
                printf("download CRL: uri=%s ec=%d\n", crluri, ec);
                if (filetime) {
                    ism_ts_t * ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
                    ism_common_setTimestamp(ts, filetime*1000000000L);
                    ism_common_formatTimestamp(ts, xbuf, sizeof(xbuf), 7, ISM_TFF_ISO8601);
                    printf("CRL file time = %s\n", xbuf);
                    ism_common_closeTimestamp(ts);
                }
            }
        }
        else if (!strcmpi(command, "disableCRL")) {
            if (!args || !*args) {
                printf("Global disableCRL=%s\n", ism_common_getDisableCRL() ? "true" : "false");
            } else {
                ism_tenant_t * tenant = ism_tenant_getTenant(args);
                if (!tenant) {
                    printf("tenant not found: %s\n", args);
                } else {
                    printf("disableCRL for tenant %s: %s\n", tenant->name, tenant->disableCRL == 2 ? "default" : (tenant->disableCRL ? "true" : "false"));
                }
            }
        }
        else if (!strcmpi(command, "test")) {
            extern int ism_proxy_getTenantDynamicConfig(ism_json_t * jobj, ism_tenant_t * tenant, int outer);
            extern int ism_proxy_getDynamicConfig(ism_json_t * jobj);
            concat_alloc_t tbuf = {xbuf, sizeof xbuf};
            ism_json_t xjobj = {0};
            ism_json_t * jobj = ism_json_newWriter(&xjobj, &tbuf, 4, 0);
            if (!args || !*args) {
                ism_proxy_getDynamicConfig(jobj);
                printf("%s", tbuf.buf);
            } else {
                ism_tenant_t * tenant = ism_tenant_getTenant(args);
                if (!tenant) {
                    printf("tenant not found: %s\n", args);
                } else {
                    ism_proxy_getTenantDynamicConfig(jobj, tenant, 1);
                    printf("%s", tbuf.buf);
                }
            }
            ism_common_freeAllocBuffer(&tbuf);
        }
        else if (!strcmpi(command, "test2")) {
            ism_transport_t tx1 = {0};
            ism_transport_t tx2 = {0};
            ism_transport_t tx3 = {0};
            tx1.name = "1";
            tx2.name = "2";
            tx3.name = "3";
            xUNUSED ism_handler_t handler = ism_common_addUserHandler(test_callback, &tx1);
            handler = ism_common_addUserHandler(test_callback, &tx2);
            handler = ism_common_addUserHandler(test_callback, &tx3);
            ism_common_runUserHandlers();
            printf("handers run\n");
        }
        else if (!strcmpi(command, "mhub")) {
            char * xtenant = ism_common_getToken(args, " \t,", " \t,", &args);
            char * mhub = ism_common_getToken(args, " \t,", " \t,", &args);
            if (!xtenant || !*xtenant) {
                printf("No tenant specified\n");
            } else {
                ism_mhub_t * mhublist = NULL;
                ism_tenant_lock();
                ism_tenant_t * tenant = ism_tenant_getTenant(xtenant);
                if (tenant)
                    mhublist = tenant->mhublist;
                ism_tenant_unlock();
                if (!tenant) {
                    printf("Tenant not found: %s\n", xtenant);
                } else {
                    if (!mhublist) {
                         printf("No MessageHub objects for tenant: %s\n", xtenant);
                    } else {
                         ism_mhub_printMhub(mhublist, mhub);
                    }
                }
            }
        }
        else if (!strcmpi(command, "mstat")) {
            ism_mhub_printStats();
        }
        else if (!strcmpi(command, "user")) {
            if (!args || !*args)
                args = "*";
            ism_tenant_printUsers(args);
        }
        else if (!strcmpi(command, "tuser")) {
            if (!args || !*args)
                args = "*";
            ism_tenant_printTenantUsers(args);
        }
        else if (!strcmpi(command, "jget")) {
            int otype = 0;
            char * ots = ism_common_getToken(args, " ", " ", &args);
            char * name1 = ism_common_getToken(args, " ", " ", &args);
            char * name2 = ism_common_getToken(args, " ", " ", &args);
            if (name1 && !strcmp(name1, "-"))
                name1 = NULL;
            if (!strcmp(ots, "config"))
                otype = ImaProxyImpl_config;
            else if (!strcmp(ots, "config"))
                otype = ImaProxyImpl_config;
            else if (!strcmp(ots, "endpoint"))
                otype = ImaProxyImpl_endpoint;
            else if (!strcmp(ots, "tenant"))
                otype = ImaProxyImpl_tenant;
            else if (!strcmp(ots, "server"))
                otype = ImaProxyImpl_server;
            else if (!strcmp(ots, "user"))
                otype = ImaProxyImpl_user;
            else if (!strcmp(ots, "item"))
                otype = ImaProxyImpl_item;
            else if (!strcmp(ots, "endpointlist"))
                otype = ImaProxyImpl_endpoint + ImaProxyImpl_list;
            else if (!strcmp(ots, "tenantlist"))
                otype = ImaProxyImpl_tenant + ImaProxyImpl_list;
            else if (!strcmp(ots, "serverlist"))
                otype = ImaProxyImpl_server + ImaProxyImpl_list;
            else if (!strcmp(ots, "userlist"))
                otype = ImaProxyImpl_user + ImaProxyImpl_list;
            if (otype) {
                concat_alloc_t tbuf = {xbuf, sizeof xbuf};
                ism_proxy_getJSONn(&tbuf, otype, name1, name2);
                printf("%s\n", tbuf.buf);
                ism_common_freeAllocBuffer(&tbuf);
            }
        }

        else if (!strcmpi(command, "password")) {
            char   obuf[256];
            char * user = ism_common_getToken(args, " \t,", " \t,", &args);
            char * pw = ism_common_getToken(args, " \t,", " \t,", &args);
            if (!user || !*user) {
                printf("The username must be specified\n");
            } else {
                if (!pw || !*pw) {
                    printf("To use a non-null password, give the password after the user name\n");
                }
                ism_tenant_createObfus(user, pw, obuf, sizeof obuf, 1);
                printf("\"%s\": { \"Password\": \"%s\" },\n", user, obuf);
            }
        }
        else if (!strcmpi(command, "disconnect")) {
            count = ism_transport_closeConnection(NULL, NULL, NULL, args, NULL, NULL, 0);
            printf("Connections closed: %d\n", count);
        }
        else if (!strcmpi(command, "set")) {
            char * valu = NULL;
            if (args)
                valu = strchr(args, '=');
            if (valu) {
                char * endptr = NULL;
                ism_json_parse_t parseo = {};
                ism_json_entry_t ents[2];
                parseo.ent = ents;
                parseo.ent_count = 2;
                ents[1].name = args;
                *valu++ = 0;
                ents[1].value = valu;
                int ival = strtol(valu, &endptr, 10);
                if (*endptr) {
                    ents[1].objtype = JSON_String;
                } else {
                    ents[1].objtype = JSON_Integer;
                    ents[1].count = ival;
                }
                rc = ism_proxy_complexConfig(&parseo, 2, 0, 0);
                if (rc) {
                    ism_common_formatLastError(xbuf, sizeof xbuf);
                    printf("set failed: rc=%d reason=%s\n", rc, xbuf);
                }
            } else {
                printf("set must contain a key=value pair\n");
            }
        }
        else if (!strcmpi(command, "start")) {
            if (ism_transport_startMessaging()) {
                printf("Messaging was already started\n");
            } else {
                printf("Messaging started\n");
            }
        }

        else if (!strcmpi(command, "tracelevel")) {
            if (args && *args) {
                rc = ism_common_setTraceLevelX(ism_defaultTrace, args);
                if (rc) {
                    printf("The trace level is not valid: rc=%d\n", rc);
                } else {
                    ism_field_t var = {0};
                    var.type = VT_String;
                    var.val.s = args;
                    ism_common_setProperty(ism_common_getConfigProperties(), "TraceLevel", &var);
                    printf("trace level = %s\n", ism_common_getStringConfig("TraceLevel"));
                }
            } else {
                printf("trace level = %s\n", ism_common_getStringConfig("TraceLevel"));
            }

        }

        /*
         * anything else?
         */
        else {
            printf("Unknown command\n");
        }
    }
    return 0;
}

extern void ism_common_traceSignal(int signo);

/*
 * Catch a terminating signal and try to flush the trace file.
 */
static void sig_handler(int signo) {
    static volatile int in_handler = 0;
    signal(signo, SIG_DFL);
    if (in_handler)
        raise(signo);
    in_handler = 1;
    if(!daemon_mode)
        ism_common_traceSignal(signo);
    raise(signo);
}

int g_rc = -1;

/*
 * SIGTERM signal handler
 */
static void sigterm_handler(int signo) {
    if (inpause) {
        __sync_or_and_fetch(&doterm, 1);
        return;
    }
	signal(signo, SIG_DFL);
    if (g_rc >= 0) {
        exit(g_rc);
    } else {
    	raise(signo);
    }
}

/*
 * SIGUSER2 signal handler
 * Just set the bit and we check it after return from a pause
 */
static void siguser2_handler(int signo) {
   __sync_or_and_fetch(&doUser2, 1);     /* Use atomic just in case */
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
