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
 * This is the main entry point for the imabridge.  Its primary role is to start each of the
 * components.
 * For debug purposes a small command line is created.  In production this is normally not used.
 *
 * There is a dynamic config file which is used to write out the bridge config objects (forwarder
 * and connection) after a dynamic change.  This file is set using the DynamicConfig in the imabridge.cfg
 * config file which is in either the same directory as the executable or in the current directory.
 * This defaults to "bridge.cfg" in the current directory.
 *
 */
#include <ismutil.h>
#include <ismjson.h>
#ifndef NO_PXACT
#include <pxactivity.h>
#endif
#include <pxtransport.h>
#include <signal.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <dirent.h>
#include <linux/oom.h>
#include <sys/resource.h>
#include <log.h>
#include <tenant.h>
#include <selector.h>
#include <throttle.h>
#include <pxtcp.h>
#include <sys/prctl.h>
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
extern  int    g_isBridge;
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

xUNUSED static char * versionString = "version_string: imabridge " XSTR(ISM_VERSION) " " XSTR(BUILD_LABEL) " " XSTR(ISMDATE) " " XSTR(ISMTIME);

static const char * g_dynamic_config = "bridge.cfg";
static const char * g_extra_config = NULL;
static const char * g_static_config = NULL;
static const char * g_install_dir;
static int    g_config_found = 0;
extern char g_procname [20];
extern int g_need_dyn_write;

/*
 * Forward references
 */
static int  initBridge(void);
static int   process_args(int argc, char * * argv);
static void  syntax_help(void);
void * runcommands(void * param, void * context, int intparam);
static int   docommand(char * buf);
static void  sig_handler(int signo);
static void sigterm_handler(int signo);
static void siguser2_handler(int signo);
static const char * getInstallDir(void);

extern void ism_common_initTrace(void);
extern void ism_common_initTimers(void);
extern int ism_common_traceFlush(int millis);
extern void ism_proxy_notify_init(void);
extern int ism_log_startLogTableCleanUpTimerTask(void);
extern int ism_protocol_printACL(const char * name);
extern int ism_protocol_initACL(void);
int ism_bridge_swidtags(void);
int ism_proxy_getLicensedUsage(void);
int ism_bridge_getFileContents(const char * fname, concat_alloc_t * buf);

static char * trcfile = NULL;
static char * logfile = NULL;
static int daemon_mode = 0;
static const char * servername = NULL;
static int inpause = 0;
static volatile int doterm = 0;
static volatile int doUser2 = 0;
static int g_swiddone = 0;
static ism_threadh_t cmdthread;
int g_rc = -1;
extern const char * g_truststore;
ism_tenant_t * g_bridge_tenant;
extern int (* ism_bridge_swidtags_f)(void);

static int startMessagingTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata);
void ism_tenant_printUsers(const char * name);
int  ism_proxy_restConfigInit(void);
int  ism_proxy_process_config(const char * name, const char * dname, int complex, int checkonly, int keepgoing);
void ism_proxy_setAuxLog(ism_domain_t * domain, int logger, ism_prop_t * props, const char * logname);
void ism_proxy_nojavainit(void);
void ism_proxy_addSelf(ism_acl_t * acl);
void ism_proxy_shutdownWait(void);
void ism_proxy_shutdownDelay(void);
int  ism_transport_stopMessaging(void);
int ism_log_createLoggerSingle(ism_prop_t * props);
int ism_bridge_getForwarderList(const char * match, concat_alloc_t * buf, int json);
const char * ism_transport_makepw(const char * data, char * buf, int len, int dir);

extern const char * ism_bridge_getVersion(void);
extern void ism_ssl_init(int useFips, int useBufferPool);
extern void ism_transport_closeAllConnections(int notAdmin, int noKafka);
extern void ism_common_setDisableCRL(int disable);
extern int ism_common_setServerName(const char * name);
extern void ism_bridge_printForwarders(const char * pattern, int status);
extern void ism_bridge_printConnections(const char * pattern);
extern void ism_bridge_getDynamicConfig(ism_json_t *jobj);
extern int  ism_bridge_init(void);
extern int  ism_bridge_start(void);
extern int  ism_bridge_term(void);
extern void ism_proxy_setDynamicTime(int millis);
extern int ism_bridge_updateDynamicConfig(int doBackup);
extern int ism_common_setTraceAppend(int append);
extern int ism_proxy_getInfo(concat_alloc_t * buf, const char * which);
int ism_proxy_addNotifyDynamic(const char * name);
void ism_mhub_printMhub(struct ism_mhub_t * mhub, const char * xmhub);
void ism_mhub_printStats(void);
void ism_bridge_stopSource(void);
int ism_proxy_addNotifyTrust(const char * name);
void ism_common_initUtil2(int type);

/*
 * Start up the server
 */
int main (int argc, char * * argv) {
    int  rc;
    ism_field_t  f;
    char      thread_name [20];
    const char * install_dir;
    char * install_config;

    /*
     * Process arguments
     */
    process_args(argc, argv);
    fprintf(stderr, IMA_PRODUCTNAME_FULL " Bridge\n");
    prctl(PR_SET_NAME, (unsigned long)(uintptr_t)"imabridge");
    g_isBridge = 1;
    fprintf(stderr, "Copyright (C) 2013-2021 Contributors to the Eclipse Foundation.\n\n");
    fflush(stderr);

    /*
     * Initialize the utilities
     */
    ism_common_initUtil2(2);
    ism_common_setTraceLevel(9);
    ism_common_setTraceFile("stdout", 0);

    /* Catch various signals and flush the trace file */
    signal(SIGINT, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGTERM, sigterm_handler);
    signal(SIGUSR2, siguser2_handler);

    /*Ignore SIGPIPE signals for broken TCP connections.*/
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Cause a parent death to send up a SIGTERM */
    prctl(PR_SET_PDEATHSIG, SIGTERM);

    /* Run the install config file */
    install_dir = getInstallDir();
    install_config = alloca(strlen(install_dir) + 16);
    strcpy(install_config, install_dir);
    ism_bridge_swidtags_f = ism_bridge_swidtags;
    strcat(install_config, "imabridge.cfg");
    if (!access(install_config, R_OK)) {
        rc = ism_proxy_process_config(install_config, NULL, 0, 0, 1);
        if (!rc) {
            g_config_found = 1;
        }
    }
    if (g_config_found == 0) {
        syntax_help();
    }

    if (g_static_config) {
        rc = ism_proxy_process_config(g_static_config, NULL, 0, 0, 1);
        if (rc) {
            printf("The bridge static configuration file is not valid: %s\n", g_static_config);
            if (g_config_found == 0)
                exit(2);
        }
    }


    /* Override trace file */
    if (trcfile) {
        f.type = VT_String;
        f.val.s = trcfile;
        ism_common_setProperty(ism_common_getConfigProperties(), "TraceFile", &f);
    }

    /* Override log file */
    if (trcfile) {
        f.type = VT_String;
        f.val.s = logfile;
        ism_common_setProperty(ism_common_getConfigProperties(), "LogDestination", &f);
    }

    /* Allow override of the dynamic config file */
    const char * dyncfg = ism_common_getStringConfig("DynamicConfig");
    if (dyncfg)
        g_dynamic_config = dyncfg;


    /*
     * Initialize the trace and make the initial entries
     */
    ism_common_setTraceAppend(daemon_mode);    /* Default append if we are in daemon mode */
    ism_common_initTrace();

    TRACE(1, "Initialize " IMA_PRODUCTNAME_FULL " Bridge\n");
    TRACE(2, "Copyright (C) 2013, 2021 Contributors to the Eclipse Foundation\n");
    TRACE(2, "version   = %s %s %s\n", ism_common_getVersion(), ism_common_getBuildLabel(), ism_common_getBuildTime());
    TRACE(2, "brversion = %s\n", versionString);
    TRACE(3, "soversion = %s\n", ism_bridge_getVersion());
    TRACE(2, "platform  = %s %s\n", ism_common_getPlatformInfo(), ism_common_getKernelInfo());
    TRACE(2, "processor = %s\n", ism_common_getProcessorInfo());

    /* Set the thread name of the main thread in case it has been changed */
    rc = prctl(PR_GET_NAME, (unsigned long)(uintptr_t)thread_name);
    if (rc)
        strcpy(thread_name, "imabridge");

    /* Java connector is not used in bridge */
    ism_proxy_nojavainit();
    prctl(PR_SET_NAME, (unsigned long)(uintptr_t)thread_name);

    /*
     * Initialize TLS
     */
    ism_common_setDisableCRL(1);    /* Bridge does not handle CRLs */
    ism_ssl_init(0, 0);

    /*
     * Initialize the bridge environment.
     */
    rc = initBridge();
    if (rc) {
        LOG(CRIT, Server, 935, "%s", "Unable to initialize: {0}", "bridge");
        printf("Unable to initialize the bridge: rc=%d\n", rc);
        g_rc = 2;
        doterm = 1;
        ism_common_sleep(200000);
        goto terminate;
    }


    /*
     * Init the transport
     */
    rc = ism_transport_init();
    if (rc) {
        LOG(CRIT, Server, 935, "%s", "Unable to initialize: {0}", "transport");
        printf("Unable to initialize transport: rc=%d\n", rc);
        g_rc = 2;
        doterm = 1;
        ism_common_sleep(100000);
        goto terminate;
    }

    /*
     * Initialize bridge processing
     */
    ism_bridge_init();
    ism_proxy_addNotifyTrust(g_truststore);

    /*
     * Process dynamic config
     */
    ism_proxy_setDynamicTime(5000);   /* Ignore changes in the next 5 seconds */
    rc = ism_proxy_process_config(g_dynamic_config, NULL, 0x11, 0, 1);  /* keepgoing (treat invalid as warning) */
    if (rc) {
        if (rc >= ISMRC_Error) {
            int  lastrc = rc;
            char * xbuf = alloca(4096);
            ism_common_formatLastError(xbuf, 4096);
            LOG(CRIT, Server, 947, "%s%s%u", "Error processing the dynamic config file: Name={0} RC={2} Error={1}",
                    g_dynamic_config, xbuf, lastrc);
            printf("Error processing the dynamic config file %s: %s (%d)\n",
                    g_dynamic_config, xbuf, lastrc);

        }
        /* Log and print was done in process_config */
        if (access(g_dynamic_config, F_OK)) {      /* If file does not exist */
            FILE * ff = fopen(g_dynamic_config, "wb");
            if (ff) {
                fclose(ff);    /* If no dynamic config, make one and continue */
            } else {
                g_rc = 2;
                doterm = 1;
                ism_common_sleep(200000);
                goto terminate;      /* If cannot make dynamic config,   */
            }
        } else {
            g_rc = 2;
            doterm = 1;
            ism_common_sleep(200000);
            goto terminate;          /* If exists buf failed */
        }
    }

    if (g_need_dyn_write) {
        ism_bridge_updateDynamicConfig(0);
        g_need_dyn_write = 0;
    }
    ism_proxy_setDynamicTime(0);   /* Set the read time of the dynamic config file */
    ism_proxy_addNotifyDynamic(g_dynamic_config);  /* Add a notify */

    /*
     * If update config, run and terminate
     *
     * The return code is zero if everything worked.  Otherwise it is a ISMRC return code
     * if the return code is less than 256.
     */
    if (g_extra_config) {
        rc = ism_proxy_process_config(g_extra_config, NULL, 0x11, 0, 0);
        if (!rc) {
            rc = ism_bridge_updateDynamicConfig(1);
        }
        ism_common_sleep(400000);
        doterm = 1;
        g_rc = rc > 255 ? ISMRC_Error : rc;
        goto terminate;
    }

    /* If ITML not done during dynamic config, do it now */
    if (!g_swiddone) {
        ism_bridge_swidtags();
    }

    /*
     * Initialize admin REST
     */
    ism_proxy_restConfigInit();
    rc = ism_transport_start();
    if (rc) {
        LOG(CRIT, Server, 936, "%s", "Unable to start: {0}", "transport");
        printf("Unable to start transport: rc=%d\n", rc);
        ism_common_sleep(200000);
        exit(2);
    }

    /*
     * Start receiving messages
     */
    int  start_msg_delay = ism_common_getIntConfig("StartMessaging", 1000);
    if (start_msg_delay > 0) {
        ism_common_setTimerOnce(ISM_TIMER_HIGH, startMessagingTimer, NULL, ((uint64_t)start_msg_delay) * 1000000);
    }

    /*
     * Run the command line if not in daemon mode.  This is run in another thread
     */
    if (!daemon_mode) {
        ism_common_startThread(&cmdthread, runcommands, NULL, NULL, 0, ISM_TUSAGE_NORMAL, 0, "command",
            "Run commands for non-daemon mode");
    }

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

terminate:
    /*
     * Do graceful termination
     */
    if (doterm) {
        TRACE(1, "Terminate bridge - start controlled shutdown\n");
        ism_bridge_stopSource();
        ism_proxy_shutdownDelay();
        ism_transport_term();
        TRACE(2, "Terminate bridge - transport is disabled\n");
    }
    exit(g_rc >= 0 ? g_rc : 0);
}


/*
 * Get the install directory
 * We look up the location of the executable in /proc/self
 */
static const char * getInstallDir(void) {
    if (!g_install_dir) {
        char xbuf [4096];
        if (readlink("/proc/self/exe", xbuf, sizeof xbuf) >= 0) {
            char * pos = strrchr(xbuf, '/');
            if (pos) {
                pos[1] = 0;    /* Including trailing slash */
                int pathlen = strlen(xbuf);
                /* If in /debug/bin look set install dir to /bin */
                if (pathlen > 11 && !strcmp(pos-10, "/debug/bin/")) {
                    strcpy(pos-10, "/bin/");
                }
            } else {
                xbuf[0] = '/'; /* Root directory */
                xbuf[1] = 0;
            }
            g_install_dir = ism_common_strdup(ISM_MEM_PROBE(ism_memory_proxy_br_misc,1000),xbuf);
        }
    }
    return g_install_dir;
}


/*
 * Start messaging.  This is normally done on a timer, but can be set so that an
 * explicit start REST call is required.
 */
static int startMessagingTimer(ism_timer_t timer, ism_time_t timestamp, void * userdata) {
    ism_transport_startMessaging();
    ism_bridge_start();
    ism_common_cancelTimer(timer);
    return 0;
}

int ism_log_init2(int exetype);


/*
 * Start the bridge.
 * Initialization of the server is done in this function, but it is not run.
 * If this method returns a good return code the bridge will then be started.
 * @return A return code, 0=good.
 */
static int  initBridge(void) {
    struct rlimit rlim;
    ism_field_t var = {0};
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
     * Set the server name which is also used as the authentication realm
     */
    if (!servername)
        servername = ism_common_getStringConfig("ServerName");
    if (!servername)
        servername = "Bridge";

    var.type = VT_String;
    var.val.s = (char *)servername;
    ism_common_setProperty(ism_common_getConfigProperties(), "ServerName", &var);
    ism_common_setServerName(servername);

    /*
     * Initialize the loggers and log the startup
     */
    ism_log_init2(2);
    if (!ism_common_getStringConfig("LogDestinationType")) {
        var.type = VT_String;
        var.val.s = (char *)"file";
        ism_common_setProperty(ism_common_getConfigProperties(), "LogDestinationType", &var);
    }
    if (g_extra_config) {
        if (!ism_common_getStringConfig("LogDestinationUpdate")) {
            var.type = VT_String;
            var.val.s = (char *)"imabridge_update.log";
            ism_common_setProperty(ism_common_getConfigProperties(), "LogDestinationType", &var);
        }
    } else {
        if (!ism_common_getStringConfig("LogDestination")) {
            var.type = VT_String;
            var.val.s = (char *)"imabridge.log";
            ism_common_setProperty(ism_common_getConfigProperties(), "LogDestinationType", &var);
        }
    }
    ism_log_createLoggerSingle(ism_common_getConfigProperties());
    ism_proxy_setAuxLog(&ism_defaultDomain, LOGGER_SysLog, ism_common_getConfigProperties(), "LogLevel");


    LOG(INFO, Server, 902, "%s %s", "Start the " IMA_PRODUCTNAME_FULL " Bridge ({0} {1}).",
            ism_common_getVersion(), ism_common_getBuildLabel());

    /*
     * Start timer threads
     */
    ism_common_initTimers();
    /* Set trace flush after timers */
    ism_common_traceFlush(ism_common_getIntConfig("TraceFlush", 1000));

    /*
     * Hash the truststore
     */
    if (g_truststore) {
        ism_common_hashTrustDirectory(g_truststore, 0, 0);
    }

    /*
     * Start notify processing
     */
    ism_proxy_notify_init();

    return rc;
}


/*
 * Process args
 * At this point we process only the first config file (or imabridge.cfg if that is missing)
 */
static int process_args(int argc, char * * argv) {
    int config_found = g_config_found;
    int  i;

    for (i=1; i<argc; i++) {
        char * argp = argv[i];

        if (argp[0]=='-') {

            /* -h -H -? = syntax help */
            if (argp[1] == 'h' || argp[1]=='H' || argp[1]=='?' ) {
                config_found = -1;
                break;
            }

            else if (argp[1] == '-' && argp[2] == 'h') {
                config_found = -1;
                break;
            }

            /* -d = daemon mode (no command line) */
            else if (argp[1] == 'd') {
                daemon_mode = 1;
            }

            else if (argp[1] == 'l') {
                if (i+1 < argc) {
                    argp = argv[++i];
                    logfile = argp;
                }
            }

            /* -n = server name */
            else if (argp[1] == 'n') {
                if (i+1 < argc) {
                    argp = argv[++i];
                    servername = argp;
                }
            }

            /* -t = trace file */
            else if (argp[1] == 't') {
                if (i+1 < argc) {
                    argp = argv[++i];
                    trcfile = argp;
                }
            }

            /* -u = update config */
            else if (argp[1] == 'u') {
                if (i+1 < argc) {
                    argp = argv[++i];
                    g_extra_config = argp;
                }
            }

            /* Anything else is an error */
            else {
                fprintf(stderr, "Unknown switch found: %s\n", argp);
                config_found = -1;
                break;
            }
        } else {
            config_found++;
            if (config_found == 1) {
                g_static_config = argp;
            } else {
                fprintf(stderr, "Unknown argument found: %s\n", argp);
            }
        }
    }

    /* If update config mode, set trace file default */
    if (g_extra_config && !trcfile)
        trcfile = "imabridge_update.trc";

    if (config_found == 0) {
        if (!access("imabridge.cfg", R_OK)) {
            g_static_config = "imabridge.cfg";
            config_found = 1;
        }
    }
    if (config_found < 0) {
        syntax_help();
    }
    g_config_found = config_found;
    return config_found <= 0;
}


/*
 * Syntax help
 */
void syntax_help(void) {
    fprintf(stderr, "imabridge  options  config_file \n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "    -?         = Show this help\n");
    fprintf(stderr, "    --help     = Show this help\n");
    fprintf(stderr, "    -d         = Set daemon mode (no command line)\n");
    fprintf(stderr, "    -l fname   = Set the log file name\n");
    fprintf(stderr, "    -t fname   = Set the trace file name\n");
    fprintf(stderr, "    -u fname   = Update the dynamic config and quit");
    fprintf(stderr, "\nconfig_file:\n");
    fprintf(stderr, "    A config files in JSON format with comments. If no config file, imabridge.cfg is used.\n\n");
    fprintf(stderr, "    To run in test mode when an imabridge daemon is already running, you must set the log and\n");
    fprintf(stderr, "    trace file locations in the command, or by having a imabridge.cfg file in the working directory.\n");
    exit (1);
}


/*
 * Get the name of the swidtag file we want installed.
 * If NULL is returned we do not want any swidtag installed.
 */
const char * getITLMfile(const char * srcdir, char * buf, int buflen) {
    const char * ret = NULL;
    DIR * dir;
    struct dirent * dent;
    int usage = ism_proxy_getLicensedUsage();
    if (usage == LICENSE_None || usage == LICENSE_Developers)
        return NULL;
    dir = opendir(srcdir);
    if (!dir)
        return NULL;
    dent = readdir(dir);
    while (dent) {
        if (dent->d_type == DT_REG || dent->d_type == DT_LNK) {
            int dlen = strlen(dent->d_name);
            if (dlen > 8 && !strcmp(dent->d_name+dlen-8, ".swidtag")) {
                if (!strstr(dent->d_name, "Standby")) {
                    if (strstr(dent->d_name, "Non-Production")) {
                        if (usage == LICENSE_NonProduction) {
                            ism_common_strlcpy(buf, dent->d_name, buflen);
                            ret = buf;
                            break;
                        }
                    } else if (usage == LICENSE_Production) {
                        ism_common_strlcpy(buf, dent->d_name, buflen);
                        ret = buf;
                        break;
                    }
                }
            }
        }
        dent = readdir(dir);
    }
    closedir(dir);
    return ret;
}



/*
 * Update the swidtag directory
 */
int ism_bridge_swidtags(void) {
    const char * install_dir;
    char * swidtag_dir;
    char * config_dir;
    const char * swidtag_file;
    char   buf [512];
    DIR * dir;
    struct dirent * dent;
    int  found;
    char * fname;
    FILE * f;
    int    pathlen;

    g_swiddone = 1;
    /* Find the swidtag directory under the install directory */
    install_dir = getInstallDir();
    pathlen = strlen(install_dir);
    swidtag_dir = alloca(pathlen + 16);
    config_dir = alloca(pathlen + 16);

    /* Check if we are in a standard install location */
    strcpy(swidtag_dir, install_dir);
    if (pathlen < 4 || strcmp(swidtag_dir+pathlen-5, "/bin/"))
        return 1;
    swidtag_dir[pathlen-4] = 0;
    strcpy(config_dir, swidtag_dir);
    strcat(swidtag_dir, "swidtag");
    strcat(config_dir, "config");

    if (access(config_dir, R_OK))
        return 1;

    /* Determine what file we want in the swidtag directory */
    swidtag_file = getITLMfile(config_dir, buf, sizeof buf);

    /*
     * Look for the file we want and remove any unwanted entries in the swidtag directory
     */
    found = 0;
    if (!access(swidtag_dir, (R_OK | W_OK | X_OK))) {
        dir = opendir(swidtag_dir);
        if (dir) {
            fname = alloca(strlen(swidtag_dir) + 256);
            dent = readdir(dir);
            while (dent) {
                if (dent->d_type == DT_REG || dent->d_type == DT_LNK) {
                    int dlen = strlen(dent->d_name);
                    if (dlen > 8 && dlen < 255 && !strcmp(dent->d_name+dlen-8, ".swidtag")) {
                        if (swidtag_file && !strcmp(dent->d_name, swidtag_file)) {
                            found = 1;
                        } else {
                            strcpy(fname, swidtag_dir);
                            strcat(fname, "/");
                            strcat(fname, dent->d_name);
                            unlink(fname);
                        }
                    }
                }
                dent = readdir(dir);
            }
            closedir(dir);

            /*
             * If the file we want is not found, copy it to the swidtag directory
             */
            if (!found && swidtag_file) {
                char * xbuf = alloca(4096);
                concat_alloc_t cbuf = {xbuf, 4096};
                strcpy(fname, config_dir);
                strcat(fname, "/");
                strcat(fname, swidtag_file);
                if (!ism_bridge_getFileContents(fname, &cbuf)) {
                     strcpy(fname, swidtag_dir);
                     strcat(fname, "/");
                     strcat(fname, swidtag_file);
                     f = fopen(fname, "wb");
                     if (f) {
                         fwrite(cbuf.buf, 1, cbuf.used, f);
                         fclose(f);
                     } else {
                         TRACE(2, "Unable to write swidtag file: %s\n", fname);
                     }
                } else {
                    TRACE(2, "Unable to read swidtag file: %s\n", fname);
                }
            }
        } else {
            TRACE(2, "Unable to open swidtag directory: %s\n", swidtag_dir);
        }
    } else {
        TRACE(2, "Unable to access swidtag directory: %s\n", swidtag_dir);
    }
    return 0;
}

/*
 * External references used in commands
 */
void ism_bridge_topicMapper(concat_alloc_t * buf, const char * topic, const char * tmap);

/*
 * Simple command line.
 * This is used for testing but not in production
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
                ism_common_free(ism_memory_proxy_br_misc,xbuf);
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


/*
 * Do a command
 */
int docommand(char * buf) {
    char * command;
    char * args;
    char   xbuf[2048];
    int    rc;

    command = ism_common_getToken(buf, " \t\r\n", " \t\r\n", &args);

    if (command) {
        /*
         * config: show configuration properties
         */
        if (!strcmpi(command, "config")) {
            int  i = 0;
            int  lower = 0;
            const char * name;
            if (!args || !*args)
                args = "*";
            if (args[0]=='*' && args[1]=='*')
                lower = 1;
            while (ism_common_getPropertyIndex(ism_common_getConfigProperties(), i, &name)==0) {
                const char * value = ism_common_getStringConfig(name);
                if (ism_common_match(name, args) && (lower || !islower(name[0]))) {
                    printf("%s=%s\n", name, value);
                }
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
            printf(" connection [name*]   Show the connections\n");
            printf(" connectpw [password] Create a encryped connection password\n");
            printf(" endpoint [name*]     Show the endpoints\n");
            printf(" forwarder [name*]    Show the forwarders\n");
            printf(" info [name]          Show bridge info. name show just one\n" );
            printf(" help                 Show this help\n");
            printf(" mhub [name*]         Show MessageHub objects\n");
            printf(" mstat                Show MessageHub stats\n");
            printf(" password user ps     Create a hashed password in JSON format\n");
            printf(" quit                 Terminate the proxy\n");
            printf(" set key=value        Set a singleton config item\n");
            printf(" stat [fwd*]          Show short forwarder status\n");
            printf(" time                 Show proxy time\n");
            printf(" traceLevel           Show or set the trace level\n");
            printf(" user  [name*]        Show the global users\n");
        }

        /*
         * Print endpoint info
         */
        else if (!strcmpi(command, "endpoint")) {
            if (!args || !*args)
                args = "*";
            ism_transport_printEndpoints(args);
        }

        /*
         * Print user info
         */
        else if (!strcmpi(command, "user")) {
            if (!args || !*args)
                args = "*";
            ism_tenant_printUsers(args);
        }

        /*
         * Print forwarder config and stats
         */
        else if (!strcmpi(command, "forwarder")) {
            if (!args || !*args)
                args = "*";
            ism_bridge_printForwarders(args, 0);
        }

        /*
         * Print forwarder config and stats
         */
        else if (!strcmpi(command, "mhub")) {
            if (!args || !*args)
                args = "*";
            if (!g_bridge_tenant || !g_bridge_tenant->mhublist) {
                printf("No MessageHub objects\n");
            } else {
                ism_mhub_printMhub(g_bridge_tenant->mhublist, args);
            }
        }
        /*
         * Print forwarder config and stats
         */
        else if (!strcmpi(command, "mstat")) {
            ism_mhub_printStats();
        }

        /*
         * Print short form stats
         */
        else if (!strcmpi(command, "stat")) {
            if (!args || !*args)
                args = "*";
            ism_bridge_printForwarders(args, 1);
        }

        /*
         * Print JSON status
         */
        else if (!strcmpi(command, "jstatus")) {
            concat_alloc_t jbuf = {xbuf, sizeof xbuf};
            if (!args || !*args)
                args = "*";
            ism_bridge_getForwarderList(args, &jbuf, 2);
            printf("%s\n", jbuf.buf);
            ism_common_freeAllocBuffer(&jbuf);
        }

        /*
         * Print connection info
         */
        else if (!strcmpi(command, "connection")) {
            if (!args || !*args)
                args = "*";
            ism_bridge_printConnections(args);
        }

        /*
         * Print proxy or bridge execution environment info
         */
        else if (!strcmpi(command, "info")) {
            char * whichinfo = ism_common_getToken(args, " \t,", " \t,", &args);
            concat_alloc_t tbuf = {xbuf, sizeof xbuf};
            if (!whichinfo || !*whichinfo)
                whichinfo = "all";
            ism_proxy_getInfo(&tbuf, whichinfo);
            printf("%s\n", tbuf.buf);
            ism_common_freeAllocBuffer(&tbuf);
        }

        /*
         * Show the obfuscated user password (hashed)
         */
        else if (!strcmpi(command, "password")) {
            char * user = ism_common_getToken(args, " \t,", " \t,", &args);
            char * pw = ism_common_getToken(args, " \t,", " \t,", &args);
            if (!user || !*user) {
                printf("The username must be specified\n");
            } else {
                if (!pw || !*pw) {
                    printf("To use a non-null password, give the password after the user name\n");
                }
                ism_tenant_createObfus(user, pw, xbuf, sizeof xbuf, 1);
                printf("\"%s\": { \"Password\": \"%s\" },\n", user, xbuf);
            }
        }

        /*
         * Show the obfuscated connection password (encrypted)
         */
        else if (!strcmpi(command, "connectpw")) {
            char * pw = ism_common_getToken(args, " \t,", " \t,", &args);
            if (pw && *pw) {
                ism_transport_makepw(pw, xbuf, sizeof xbuf, 0);
                printf("connection password = \"%s\"\n", xbuf);
            }
        }

        /*
         * Set a singleton value
         */
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

        /*
         * Start messaging
         */
        else if (!strcmpi(command, "start") ) {
            if (ism_bridge_start()) {
                printf("Bridge processing has already started\n");
            } else {
                printf("Bridge processing started\n");
            }
        }

        /*
         * Set or show tracelevel
         */
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

        else if (!strcmpi(command, "rehash_full")) {
            if (!args || !*args) {
                args = (char *)ism_common_getStringConfig("TrustStore");
            }
            ism_common_hashTrustDirectory(args, 0, 3);
        }

        else if (!strcmpi(command, "rehash")) {
            if (!args || !*args) {
                args = (char *)ism_common_getStringConfig("TrustStore");
            }
            ism_common_hashTrustDirectory(args, 1, 3);
        }


        /*
         * Test (whatever needs testing now)
         */
        else if (!strcmpi(command, "test")) {
             concat_alloc_t tbuf = {xbuf, sizeof xbuf};
             ism_json_t xjobj = {0};
             ism_json_t * jobj = ism_json_newWriter(&xjobj, &tbuf, 4, 0);
             ism_bridge_getDynamicConfig(jobj);
             printf("%s\n", tbuf.buf);
             ism_common_freeAllocBuffer(&tbuf);
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


/*
 * SIGTERM signal handler
 */
static void sigterm_handler(int signo) {
    if (inpause) {
        __sync_or_and_fetch(&doterm, 1);
        if (g_rc < 0)
            g_rc = 1;      /* Continue loop in shell script */
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
