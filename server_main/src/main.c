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
 * This is the main entry point for the imaserver. Its primary role is to start each of the
 * components.
 * For debug purposes a small command line is created.  This will eventually be superceded by
 * admin and monitoring support.
 */
#include <transport.h>
#include <protocol.h>
#include <store.h>
#include <engine.h>
#include <admin.h>
#include <adminHA.h>
#include <monitoring.h>
#include <imaSnmpAgentInit.h>
#include <tracemodule.h>
#include <config.h>
#include <signal.h>
#include <assert.h>
#include <limits.h>
#include <selector.h>
#include <cluster.h>
#include <linux/oom.h>
#include <curl/curl.h>
#include <alloca.h>

extern ism_secprof_t * ism_transport_getSecProfile(const char * name);
extern int ism_config_criticalConfigError(void);
extern int ism_admin_getStorePurgeState(void);
extern char * ism_admin_getStateStr(int type);
extern int ism_server_init2(void);
extern int ism_process_mqc_monitoring_action(const char * locale, const char *inpbuf, int inpbuflen, concat_alloc_t *output_buffer, int *rc);
extern int ism_transport_term_endpoints(void);
extern int ism_protocol_printACL(const char * name);

static char * g_configfile = NULL;
static int8_t debuglevel = 5;
static int32_t debuguserdata = 0;
static char debugfile[PATH_MAX+1] = "";
extern int adminInitError;
extern int initTermStoreHA;

extern void ism_common_traceSignal(int signo);
static int in_admin_pause = 0;
static int sigterm_count = 0;
static ism_handler_t sigtermHandler;
extern int g_doUser2;

const char * STORE_INITIALIZED_INDICATOR = IMA_SVR_DATA_PATH "/config/store.init";

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

/*
 * Forward references
 */
static void  setMainAffinity(void);
static void  process_args(int argc, char * * argv);
static int   process_config(const char * name, const char * dname);
static void  syntax_help(void);
static void  disableOOMKiller(void);
static void * runcommands(void * param, void * context, int admin);
static int   docommand(char * buf);
static void  sig_handler(int signo);
static void  siguser2_handler(int signo);
static void  sig_sigterm(int signo);
static void setAdminPause(void);
XAPI void ism_common_initTrace(void);
XAPI void ism_ssl_init(int useFips, int useBufferPool);
XAPI void ism_bufferPoolInit(void);

static int trclvl = -1;
static char * trcfile = NULL;
static char * servername = NULL;
static int daemon_mode = 0;
static ism_threadh_t cmdthread;
static pthread_t mainthread;

/*
 * Start up the server
 */
int main (int argc, char * * argv) {
    int  rc;
    int  rc2;
    int  fd;
    ism_field_t  f;
    double startTime = ism_common_readTSC();
    double recoveryStartTime = startTime; /* This gets updated when we get past standby */

    /*
     * Process arguments
     */
    process_args(argc, argv);
    fprintf(stderr, IMA_SVR_COMPONENT_NAME_FULL "\n");
    fprintf(stderr, "Copyright (C) 2012, 2021 Contributors to the Eclipse Foundation\n\n");
    fflush(stderr);
    mainthread = pthread_self();

    /*
     * Initialize the utilities
     */
    ism_common_initUtilMain();

    /* Catch various signals and flush the trace file */
    signal(SIGINT, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGTERM, sig_sigterm);
    signal(SIGUSR2, siguser2_handler);

    /* Ignore SIGPIPE signals for broken TCP connections. */
    signal(SIGPIPE, SIG_IGN);

    /*
     * Process the config file
     */
    rc = process_config(g_configfile, NULL);
    if (rc)
        return rc;

    /* Override trace level if specified on the command line */
    if (trclvl != -1) {
        f.type = VT_Integer;
        f.val.i = trclvl;
        ism_common_setProperty( ism_common_getConfigProperties(), "TraceLevel", &f);
    }

    /* Override trace file if specified on the command line */
    if (trcfile) {
        f.type = VT_String;
        f.val.s = trcfile;
        ism_common_setProperty( ism_common_getConfigProperties(), "TraceFile", &f);
    }

    /* Override the server name if specified on the command line */
    if (servername != NULL) {
        f.type = VT_String;
        f.val.s = servername;
        ism_common_setProperty( ism_common_getConfigProperties(), "Name", &f);
    }

    /* 
     * Create properties as a work around to allow components to violate build order dependencies
     */
    ism_field_t fptr;
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_transport_getSecProfile;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_transport_getSecProfile_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_transport_allocBytes;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_transport_allocBytes_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_protocol_termPlugin;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_protocol_termPlugin_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_protocol_restartPlugin;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_protocol_restartPlugin_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_protocol_isPluginServerRunning;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_protocol_isPluginServerRunning_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_transport_setConnectionExpire;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_transport_setConnectionExpire_fnptr", &fptr);

    /* Set the function pointer into the server properties of the monitoring startDisconnectedClientNotificationThread */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_monitoring_startDisconnectedClientNotificationThread;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_monitoring_startDisconnectedClientNotificationThread", &fptr);

    /* Set the function pointer for engine API ism_engine_completeGlobalTransaction */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_completeGlobalTransaction;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_completeGlobalTransaction", &fptr);

    /* Set the function pointer for engine API ism_engine_forgetGlobalTransaction */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_forgetGlobalTransaction;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_forgetGlobalTransaction", &fptr);

    /* Set the function pointer for close connection API */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_transport_closeConnection;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_transport_closeConnection_fnptr", &fptr);

    /* Set the function pointer for Engine getClientStateMonitor API */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_getClientStateMonitor;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_getClientStateMonitor_fnptr", &fptr);

    /* Set the function pointer for Engine getClientStateMonitor API */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_freeClientStateMonitor;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_freeClientStateMonitor_fnptr", &fptr);

    /* Set the function pointer for Engine destroyDisconnectedClientState API */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_destroyDisconnectedClientState;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_destroyDisconnectedClientState_fnptr", &fptr);

    /* Set the function pointer for Engine unsetRetainedMsgOnDest API */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_unsetRetainedMessageOnDestination;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_unsetRetainedMsgOnDest_fnptr", &fptr);

    /* Set the function pointer for Transport disableClientSet API */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_transport_disableClientSet;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_transport_disableClientSet_fnptr", &fptr);

    /* Set the function pointer for Transport enableClientSet API */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_transport_enableClientSet;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_transport_enableClientSet_fnptr", &fptr);

    /* Set the function pointer for Cluster getStatistics API */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_cluster_getStatistics;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_cluster_getStatistics_fnptr", &fptr);

    /* Set the function pointer for Engine threadInit API */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_threadInit;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_threadInit_fnptr", &fptr);

    /* Set the function pointer for Engine threadTerm API */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_threadTerm;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_threadTerm_fnptr", &fptr);

    /* Set the function pointer for Cluster removeLocalServer API */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_cluster_removeLocalServer;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_cluster_removeLocalServer_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_transport_newOutgoing;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_transport_newOutgoing_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_transport_connect;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_transport_connect_fnptr", &fptr);

    /*Set the function pointer into the ism_config_json_parseServiceRemoveInactiveClusterMember */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_monitoring_removeInactiveClusterMember;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_monitoring_removeInactiveClusterMember", &fptr);

    /* Set the function ptr for SNMP Enable Config Item */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t) (uintptr_t)ism_snmp_jsonConfig;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_snmp_config_fnptr", &fptr);

    /* Set the function ptr for SNMP Start Agent Service */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t) (uintptr_t)ism_snmp_startService;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_snmp_start_fnptr", &fptr);

    /* Set the function ptr for SNMP Stop Agent Service */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t) (uintptr_t)ism_snmp_stopService;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_snmp_stop_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t) (uintptr_t)ism_snmp_isServiceRunning;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_snmp_running_fnptr", &fptr);

    /* Set the function pointer into the ism_process_mqc_monitoring_action */
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_process_mqc_monitoring_action;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_process_mqc_monitoring_action", &fptr);

/*----  start stuff for thread health monitoring  -----*/
    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_admin_init_stop;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_admin_init_stop_fnptr", &fptr);
/*----  end   stuff for thread health monitoring  -----*/

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_diagnostics;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_diagnostics_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_freeDiagnosticsOutput;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_freeDiagnosticsOutput_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_monitoring_diagnostics;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_monitoring_diagnostics_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_monitoring_freeDiagnosticsOutput;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_monitoring_freeDiagnosticsOutput_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_importResources;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_importResources_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_engine_exportResources;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_engine_exportResources_fnptr", &fptr);

    fptr.type = VT_Long;
    fptr.val.l = (uint64_t)(uintptr_t)ism_transport_revokeConnectionsEndpoint;
    ism_common_setProperty(ism_common_getConfigProperties(), "_ism_transport_revokeConnectionsEndpoint_fnptr", &fptr);

    /*
     * Initialize the server.
     * Note that any syntax checking which would prevent the server from starting
     * should be done at init time.
     */
    bool fServerInit = false;
    bool fAdminInit = false;
    bool fClusterInit = false;
    bool fTransportInit = false;
    bool fProtocolInit = false;
    bool fStoreInit = false;
    bool fEngineInit = false;
    bool fMonitoringInit = false;

    bool cleanStoreStarted = false;        /* Initial clean store - should occur in production mode only */

    int useAdminPause = 0;

    ism_common_initTrace();   /* Init trace before doing ssl_init */

    /*
     * Initialize the trace and make the initial entries
     */
    ism_common_initTrace();
    TRACE(1, "Initialize the " IMA_SVR_COMPONENT_NAME_FULL "\n");
    TRACE(2, "version   = %s %s %s\n", ism_common_getVersion(), ism_common_getBuildLabel(), ism_common_getBuildTime());
    TRACE(2, "source = %s\n", ism_common_getSourceLevel());
    TRACE(2, "platform  = %s %s\n", ism_common_getPlatformInfo(), ism_common_getKernelInfo());
    TRACE(2, "processor = %s\n", ism_common_getProcessorInfo());

    /*
     * Autotune key areas of MessageSight configuration based on available CPU, memory, and disk
     */
    ism_config_autotune();

    /*
     * Initialize openSSL
     */
    ism_bufferPoolInit();
    int FIPSmode = ism_config_getFIPSConfig();
    int sslUseBufferPool = ism_common_getIntConfig("SslUseBuffersPool", 0);
    ism_ssl_init(FIPSmode, sslUseBufferPool);

    /*
     * Initialize curl - it's important that this is done while
     * we are still single threaded.
     */
    CURLcode cRC = curl_global_init(CURL_GLOBAL_DEFAULT);
    /* check the curl global init state */
    if (cRC != CURLE_OK) {
        TRACE(1, "Curl global init failed: %s\n", curl_easy_strerror(cRC));
    }

    serverState = ISM_SERVER_INITIALIZING;
    TRACE(1, "Elapsed time from startTime to %s: %.2f seconds\n",
            ism_admin_getStateStr(serverState), ism_common_readTSC()-startTime);
    storeNotAvailableError = 0;

    /*
     * We go multithreaded during this call
     */
    rc = ism_common_initServer();
    if (rc == 0) {
        /*
         * Check if this is a first start after PXE boot, reset or pristine install.
         * If yes, start with a clean store (overriding Store.ColdStart), otherwise
         * proceed normally.
         */
        if ((fd = open(STORE_INITIALIZED_INDICATOR, O_EXCL | O_CREAT, 0664)) > -1) {
            size_t written;
            TRACE(2, "Store cleanup is initiated - first boot\n");
            f.type = VT_Boolean;
            f.val.i = 1;
            rc = ism_common_setProperty( ism_common_getConfigProperties(), "Store.ColdStart", &f);
            written = write(fd, (void*)&f.val.i, sizeof(f.val.i));
            if ( written != sizeof(f.val.i) ) {
                TRACE(1, "%zu bytes written of %lu\n",written,sizeof(f.val.i));
            }
            close(fd);

            cleanStoreStarted = true;
        }

        fServerInit = true;
        setMainAffinity();                               /* Set the affinity for the main thread */
        rc = ism_admin_init(ISM_PROTYPE_SERVER);
        if (rc == 0) {
            fAdminInit = true;
            ism_server_init2();    /* Finish up server init after admin is initialized */

            rc = ism_cluster_init();
            if (rc == ISMRC_ClusterDisabled) {   /* This is a good return code but says cluster is not enabled */
                rc = 0;
            } else {
                if (rc == 0)
                    fClusterInit = true;
                else
                	ism_config_setAdminMode(1);
                rc = 0;
            }
            if (rc == 0) {
                rc = ism_transport_init();                   /* Initialize transport      */
                if (rc == 0) {
                    rc = ism_authentication_init();          /* Initialize authentication */
                }
                if ( rc == 0 ) {
                    fTransportInit = true;
                    rc = ism_protocol_init();                /* Initialize protocol       */
                    if (rc == 0) {
                        fProtocolInit = true;
                        rc = ism_store_init();               /* Initialize store          */
                        if (rc == 0) {
                            storeNotAvailableError=0;
                            fStoreInit = true;
                            rc = ism_engine_init();          /* Initialize the engine     */
                            if (rc == 0) {
                                fEngineInit = true;
                                rc = ism_monitoring_init();  /* Initialize monitoring     */
                                if (rc == 0) {
                                    fMonitoringInit = true;
                                    serverState = ISM_SERVER_INITIALIZED;
                                    TRACE(1, "Elapsed time from startTime to %s: %.2f seconds\n",
                                            ism_admin_getStateStr(serverState), ism_common_readTSC()-startTime);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    /* Check for any critical configuration error that may have occured during initialization.
     * If rc is not ISMRC_OK, set adminMode so that server can be
     * started in Maintenance mode to fix any configuration problem.
     */
    if (rc == ISMRC_OK) {
        rc = ism_config_criticalConfigError();
    }
    if ( rc != ISMRC_OK )
        adminMode = 1;

    /* check if server is administratively set to start in maintenance mode */
    if (!adminMode)
        adminMode = ism_admin_getmode();

    if ( fProtocolInit == true ) {
        rc = ism_common_startServer();
        if (rc == 0) {
            rc2 = ism_transport_start();
            if (rc == 0 && rc2 == 0) {
                serverState = ISM_TRANSPORT_STARTED;
                TRACE(1, "Elapsed time from startTime to %s: %.2f seconds\n",
                        ism_admin_getStateStr(serverState), ism_common_readTSC()-startTime);
            } else {
                adminMode = 1;
            }
            /* Start protocol even if transport fails as we need it for admin mode */
            rc2 = ism_protocol_start();
            if (rc == 0 && rc2 == 0) {
                serverState = ISM_PROTOCOL_STARTED;
                TRACE(1, "Elapsed time from startTime to %s: %.2f seconds\n",
                        ism_admin_getStateStr(serverState), ism_common_readTSC()-startTime);
                useAdminPause = 1;
            }
        }
    }

    /*
     * Start the server
     */
    if (rc == 0 && adminMode == 0 ) {
        rc = ism_snmp_start();		/* start SNMP */
		if ( rc == 0 ) {
			serverState = ISM_SERVER_STARTING_STORE;
			TRACE(1, "Elapsed time to from startTime %s: %.2f seconds\n",
			        ism_admin_getStateStr(serverState), ism_common_readTSC()-startTime);
			/* Store start Lock File*/
			const char * storeStartLockFileName = "/tmp/com.ibm.ism.storestart.lock";
			FILE* storeStartLockFile = fopen(storeStartLockFileName, "w+");
			rc = ism_store_start();
			if(storeStartLockFile){
				TRACE(9, "Delete Store start Lock file.\n");
				fclose(storeStartLockFile);
				unlink(storeStartLockFileName);
			}

			if (rc == 0) {
				serverState = ISM_STORE_STARTED;
				TRACE(1, "Elapsed time from startTime to %s: %.2f seconds\n",
				        ism_admin_getStateStr(serverState), ism_common_readTSC()-startTime);
				/* Get HA role and pause if standby */
				ismHA_Role_t role = ism_admin_getHArole(NULL, &rc);
				if ( role != ISM_HA_ROLE_PRIMARY && role != ISM_HA_ROLE_DISABLED ) {
					serverState = ISM_SERVER_STANDBY;
					TRACE(1, "Elapsed time from startTime to %s: %.2f seconds\n",
					        ism_admin_getStateStr(serverState), ism_common_readTSC()-startTime);
					setAdminPause();
					TRACE(4, "HighAvailability Standby node: waiting for a fail-over signal.\n");
					rc = ism_admin_pause();
				}
				recoveryStartTime = ism_common_readTSC();
				if (rc == 0) {
				    if (fClusterInit) {
					    rc = ism_cluster_start();  /* cluster should be started after store */
					    if (rc == ISMRC_ClusterDisabled)
					        rc = 0;
				    }
				    if ( rc == 0) {
					    serverState = ISM_SERVER_STARTING_ENGINE;
					    TRACE(1, "Elapsed time from recoveryStartTime to %s: %.2f seconds\n",
					            ism_admin_getStateStr(serverState), ism_common_readTSC()-recoveryStartTime);
					    rc = ism_engine_start();
				    }
					if (rc == 0) {
					    if (fClusterInit) {
						    rc = ism_cluster_startMessaging();
						    if (rc == ISMRC_ClusterDisabled)
						        rc = 0;
					    }
					    if (rc == 0) {
						    rc = ism_engine_startMessaging();
                            if (rc == 0) {
                                serverState = ISM_SERVER_STARTING_TRANSPORT;
                                TRACE(1, "Elapsed time from recoveryStartTime to %s: %.2f seconds\n",
                                        ism_admin_getStateStr(serverState), ism_common_readTSC()-recoveryStartTime);
                                rc = ism_transport_startMessaging();
                                if (rc == 0) {
                                    serverState = ISM_MESSAGING_STARTED;
                                    TRACE(1, "Elapsed time from recoveryStartTime to %s: %.2f seconds\n",
                                            ism_admin_getStateStr(serverState), ism_common_readTSC()-recoveryStartTime);
                                    /* Set flag in monitoring to start publishing monitoring data */
                                    ism_monitoring_startEngineEventFlag();
                                } else {
                                    fEngineInit = 0;
                                    xUNUSED int zrc = ism_engine_term();
                                    fStoreInit = 0;
                                    ism_store_term();
                                    if (!daemon_mode) {
                                        fprintf(stderr, "Transport start messaging failed: rc=%d\n", rc);
                                    }
                                    adminMode = 1;
                                }
                            } else {
                                fEngineInit = 0;
                                xUNUSED int zrc = ism_engine_term();
                                fStoreInit = 0;
                                ism_store_term();
                                if (!daemon_mode) {
                                    fprintf(stderr, "Engine start messaging failed: rc=%d\n", rc);
                                }
                                adminMode = 1;
                            }
                        } else {
                            fEngineInit = 0;
                            xUNUSED int zrc = ism_engine_term();
                            fStoreInit = 0;
                            ism_store_term();
                            if (!daemon_mode) {
                                fprintf(stderr, "Cluster start messaging failed: rc=%d\n", rc);
                            }
                            adminMode = 1;
                        }
                    } else {
                        fStoreInit = 0;
                        ism_store_term();
                        if (!daemon_mode) {
                            fprintf(stderr, "Start engine failed: rc=%d\n", rc);
                        }
                        adminMode = 1;
                    }
                }
            } else {
                if (!daemon_mode) {
                    fprintf(stderr, "Start store failed: rc=%d\n", rc);
                }
                adminMode = 1;
            }
        }
    }

    /*
     * Set to ADMIN mode if the Store is Corrupted.
     * The Administrator can clean the store from the command line.
     */
    if (rc == ISMRC_StoreUnrecoverable) {
         adminMode = 1;
         storeNotAvailableError = ISMRC_StoreUnrecoverable;
    } else if (rc == ISMRC_StoreNotAvailable) {
         adminMode = 1;
         storeNotAvailableError = ISMRC_StoreNotAvailable;
    } else if ( rc != ISMRC_OK ) {
        adminMode = 1;
    }

    if ( useAdminPause == 0 ) {
    	TRACE(1, "Server encountered a critical error during initialization. Server is stopping. ServerState=%d adminMode=%d RC=%d\n", serverState, adminMode, rc);
    	fprintf(stderr, "Server encountered a critical error during initialization. Server is stopping. ServerState=%d adminMode=%d RC=%d\n", serverState, adminMode, rc);
    	return 1;
    }

    TRACE(5, "Server init and start process is complete. ServerState=%d adminMode=%d RC=%d\n", serverState, adminMode, rc);

    /*
     * If admin mode is set - do not init rest of the components
     */
    int standbyRestart = 0;
    if ( adminMode && rc != ISMRC_Closed ) {
        /* If we tried to initialize the store and it failed for some reason
         * (ie., we are in maintenance mode), erase the indicator file */
        if ( cleanStoreStarted ) {
            TRACE(2, "Store cleanup cannot be completed right now, the server is not in production mode\n");
            remove(STORE_INITIALIZED_INDICATOR);
        }

        if (!daemon_mode) {
             fprintf(stderr, "The " IMA_SVR_COMPONENT_NAME_FULL " is in maintenance mode.\n");
        }

        if ( haRestartNeeded != 1 ) {
            serverState = ISM_SERVER_MAINTENANCE;
            TRACE(1, "Elapsed time from startTime to %s: %.2f seconds\n",
                    ism_admin_getStateStr(serverState), ism_common_readTSC()-startTime);
        }

        ism_admin_setLastRCAndError();
    } else if ( adminMode && rc == ISMRC_Closed ) {
    	standbyRestart = 1;
    }


    /*
     * Go into either test mode and run commands, or daemon mode and wait for a terminate.
     */
    int daemon_pause_exit = 0;
    if (rc == 0 || adminMode) {
        if (!daemon_mode) {
            if ( serverState != ISM_SERVER_MAINTENANCE ) {
                serverState = ISM_SERVER_RUNNING;
                TRACE(1, "Elapsed time from recoveryStartTime to %s: %.2f seconds\n",
                        ism_admin_getStateStr(serverState), ism_common_readTSC()-recoveryStartTime);
                adminInitError = ISMRC_OK;
            }
            ism_common_startThread(&cmdthread, runcommands, NULL, NULL, adminMode, ISM_TUSAGE_NORMAL, 0, "command",
                "Run commands for non-daemon mode");
        }
        if ( serverState != ISM_SERVER_STANDBY && haRestartNeeded == 0 && standbyRestart == 0 ) {
            if ( serverState != ISM_SERVER_MAINTENANCE ) {
                serverState = ISM_SERVER_RUNNING;
                TRACE(1, "Elapsed time from recoveryStartTime to %s: %.2f seconds\n",
                        ism_admin_getStateStr(serverState), ism_common_readTSC()-recoveryStartTime);
            }
            setAdminPause();
            rc = ism_admin_pause();
            daemon_pause_exit = 1;
        }
        if (!daemon_mode && sigterm_count == 1)
            fprintf(stderr, "\nTerminated by SIGTERM\n");
    } else {
        fprintf(stderr, "The " IMA_SVR_COMPONENT_NAME_FULL " failed to start.\n");
    }

    /*
     * Terminate the server
     */
    serverState = ISM_SERVER_STOPPING;
    in_admin_pause = 0;


    if (fMonitoringInit)
        ism_monitoring_term();
    if (fProtocolInit)
        ism_protocol_term();
    if (fTransportInit) {
    	ism_transport_term_endpoints();
    }
    if (fEngineInit) {
        xUNUSED int zrc = ism_engine_term();
    }
    if (fClusterInit)
        ism_cluster_term();

    TRACE(7, "Process Store Cleanup or BackupToDisk flag: %d\n", cleanStore);
    if (fStoreInit) {
        if ( initTermStoreHA == 1 ) {
        	TRACE(3, "Initialize store terminate in HA mode\n");
            ism_store_termHA();
        }
        /* if cleanStore is set to 2 - initiate store backup */
        if ( cleanStore == 2 ) {
            f.type = VT_Boolean;
            f.val.i = 1;
            rc = ism_common_setProperty( ism_common_getConfigProperties(), "Store.BackupToDisk", &f);
            TRACE(2, "Store Backup to disk is initiated.\n");
        }
        ism_store_term();
    }

    /* check if admin pause is signaled in daemon mode and rc is not ISMRC_Closed */
    if ( daemon_pause_exit == 1 && rc != ISMRC_Closed ) {
    	adminMode = 1;
    	TRACE(1, "" IMA_SVR_COMPONENT_NAME_FULL " is running in Maintenance mode. rc=%d\n", rc);
    	serverState = ISM_SERVER_MAINTENANCE;
        setAdminPause();
        rc = ism_admin_pause();
        in_admin_pause = 0;
    }

    serverState = ISM_SERVER_STOPPING;


    /*
     * Check the admin variable cleanStore to see if the clean store is needed
     * This variable is set in the admin component.
     */
    TRACE(4, "Check For Clean Store or BackupToDisk flag: %d\n", cleanStore);

    if (cleanStore == 1) {
        TRACE(4, "Store cleanup is initiated\n");

        /*Create Clean Store Lock File*/
        const char * cleanStoreLockFileName = "/tmp/com.ibm.ism.cleanstore.lock";
        FILE* cleanStoreLockFile = fopen(cleanStoreLockFileName, "w+");

        if(cleanStoreLockFile!=NULL){
            /*Put PID into the Clean Store Lock file*/
            pid_t pid=getpid();
            fprintf(cleanStoreLockFile, "%d", pid);
            fflush(cleanStoreLockFile);
            TRACE(9, "Clean Store Lock file is created. PID: %d\n", pid);
        }else{
            TRACE(5, "Failed to create Clean Store Lock File.\n");
        }

        /* Force a cold start or backup to disk for the store */
        int iserr=0;
        f.type = VT_Integer;
        f.val.i = 1;
        rc = ism_common_setProperty( ism_common_getConfigProperties(), "Store.ColdStart", &f);
        TRACE(2, "Clean Store started.\n");

        /* It is necessary to disable HA for ColdStart */
        ism_admin_ha_disableHA_for_cleanStore();

        rc = ism_store_init();
        if (rc == ISMRC_OK) {

             rc = ism_store_start();
             if (rc != ISMRC_OK) {
                 TRACE(3, "Clean Store failed. action=start, rc=%d.\n", rc);
                 iserr=1;
             }

             rc = ism_store_term();
             if (rc != ISMRC_OK) {
                 TRACE(3, "Clean Store failed. action=term, rc=%d.\n", rc);
                 iserr=1;
             }


        } else {
            TRACE(3, "Clean Store failed. action=init, rc=%d.\n", rc);
            iserr=1;
        }

        if (!iserr) {
             TRACE(4, "Clean Store completed.\n");
        } else {
             TRACE(3, "Clean Store completed with errors.\n");
        }

        cleanStore = 0;

        /*Delete Clean Store Lock File*/
        if(cleanStoreLockFile){
            fclose(cleanStoreLockFile);
            TRACE(4, "Clean Store Lock file is deleted.\n");
            unlink(cleanStoreLockFileName);
        }

        /* if purge option is set, restart server */
        if ( ism_admin_getStorePurgeState() == 1 ) {
            int ret;
            TRACE(4, "Restart server.\n");
            ret = system(IMA_SVR_INSTALL_PATH "/bin/resetServerConfig.sh 0 2 &");
            TRACE(4, "Restart server return code %d.\n",ret);
        }
    }

    if (fTransportInit) {
        ism_transport_term();
    }

    if (fServerInit)
        ism_common_termServer();

    /*
     * Cleanup curl
     */
    if (cRC == CURLE_OK) {
        curl_global_cleanup();
    }
    else {
        TRACE(5, "Skipping curl_global cleanup because curl was not successfully started (%s).\n", curl_easy_strerror(cRC));
    }

    if (fAdminInit)
        ism_admin_term(ISM_PROTYPE_SERVER);

    if ( restartServer == 1 ) {
        /* check if configuration reset is required */
        int ret;
        if ( resetConfig == 1 ) {
            TRACE(4, "Reset configuration to default and restart server\n");
            ret = system(IMA_SVR_INSTALL_PATH "/bin/resetServerConfig.sh 1 2 &");
        } else {
            TRACE(4, "Restart server.\n");
            ret = system(IMA_SVR_INSTALL_PATH "/bin/resetServerConfig.sh 0 2 &");
        }
        TRACE(4, "resetServerConfig return code %d", ret);
        sleep(2);
    }

    return 0;
}

/*
 * Dummy function to satisfy the compiler
 * This function is implemented and used in mqcbridge process.
 */
XAPI int ism_process_mqc_monitoring_action(const char * locale, const char *inpbuf, int inpbuflen, concat_alloc_t *output_buffer, int *rc) {
    /* Should not reach here for imaserver process */
    *rc = ISMRC_NotImplemented;
    return *rc;
}

/*
 * Assign the server main thread to the last CPU on the machine.
 * This is not a busy thread (except during start), but we want to make sure that it is off the
 * critical CPUs.
 */
static void setMainAffinity(void) {
    const char * affStr;
    char         affMap[64];
    int          affLen = 0;

    affStr = ism_common_getStringConfig("Affinity.imaserver");
    if (!affStr)
        affStr = ism_common_getStringConfig("Affinity.ismserver");
    if (affStr) {
        affLen = ism_common_parseThreadAffinity(affStr, affMap);
        if (affLen){
            ism_common_setAffinity(pthread_self(), affMap, affLen);
        }
    }
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
            /* -0 to -9 test trace level */
            if (argp[1] >='0' && argp[1]<='9') {
                trclvl = argp[1]-'0';
            }

            /* -a Admin mode */
            else if (argp[1] == 'a') {
                adminMode = 1;
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

            /* Anything else is an error */
            else {
                fprintf(stderr, "Unknown switch found: %s\n", argp);
                config_found = 0;
                break;
            }
        } else {
            /* The first positional arg is the config file */
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
    fprintf(stderr, "imaserver  options  config_file\n\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "    -?         = Show this help\n");
    fprintf(stderr, "    -a         = Enter admin mode\n");
    fprintf(stderr, "    -0 ... -9  = Set the trace level. 0=trace off\n");
    fprintf(stderr, "    -n name    = Set server instance name\n");
    fprintf(stderr, "    -t fname   = Set the trace file name\n");
    fprintf(stderr, "    -d         = Set daemon mode (no command line)\n");
    fprintf(stderr, "\nconfig_file:\n");
    fprintf(stderr, "    The file contains a list of keyword=value pairs of configuration values.\n");
    exit (1);
}


/*
 * Process a config file as keyword=value
 */
int process_config(const char * name, const char * dname) {
    FILE * f;
    char linebuf[4096];
    char * line;
    char * keyword;
    char * value;
    char * more;
    char * cp;
    ism_field_t var = {0};

    if (name == NULL)
        return 0;

    /*
     * Replace last segment of the name with the override name
     */
    if (dname) {
        char * namebuf = alloca(strlen(name) + strlen(dname) + 2);
        strcpy(namebuf, name);
        cp = namebuf+strlen(name) - 1;
        while (cp > namebuf && *cp != '/')
            cp--;
        if (*cp == '/')
            cp++;
        strcpy(cp, dname);
        name= namebuf;
    }
    f = fopen(name, "rb");
    if (!f) {
        printf("The config file is not found: %s\n", name);
        return 1;
    }

    line = fgets(linebuf, sizeof(linebuf), f);
    while (line) {
        keyword = ism_common_getToken(line, " \t\r\n", "=\r\n", &more);
        if (keyword && keyword[0]!='*' && keyword[0]!='#') {
            cp = keyword+strlen(keyword);     /* trim trailing spaces */
            while (cp>keyword && (cp[-1]==' ' || cp[-1]=='\t' ))
                cp--;
            *cp = 0;
            value   = ism_common_getToken(more, " =\t\r\n", "\r\n", &more);

            if (!value) {
                value = "";
            } else {
                /* Trim trailing spaces.  We do not allow trailing spaces in object names */
                cp = value+strlen(value);
                while (cp>value && (cp[-1]==' '))
                    cp--;
                *cp = 0;
            }

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


/*
 * Disable the OOM killer for the imaserver process.
 * The theory is that the imaserver is too important to the system to kill before everything else is gone.
 * This only works for the root.
 */
xUNUSED static void disableOOMKiller(void) {
    char fname [128];
    char val [32];
    int  fd;

    sprintf(fname, "/proc/%u/oomadj", getpid());
    sprintf(val, "%d", OOM_DISABLE);
    fd = open(fname, O_WRONLY);
    if (fd >= 0) {
        write(fd, val, strlen(val));
        close(fd);
    }
}


/*
 * Simple command line
 */
void * runcommands(void * param, void * context, int admin) {
    char buf[1024];

#ifdef USE_EDITLINE
    /*
     * Initialize editline processing
     */
    init_readline();
#endif

    char xprompt[68];

    for (;;) {
        char * xbuf;
        fflush(stdout);
#ifdef USE_EDITLINE
        if (ism_readline) {
            const char * server_name = ism_common_getServerName();
            ism_common_strlcpy(xprompt, server_name ? server_name : "", 64);
            strcat(xprompt, admin ? "$ " : "> ");
            xbuf = ism_readline(xprompt);
            if (xbuf) {
                /*
                 * If this is a non-null line and not the same as one of
                 * the last two entries in the history file, add it
                 * to the history file.
                 */
                add_cmdhistory(xbuf);
                ism_common_strlcpy(buf, xbuf, sizeof(buf)-1);
                ism_common_free_raw(ism_memory_main_misc,xbuf);
                xbuf = buf;
            }
        } else
#endif
        {
            fprintf(stderr, admin ? "\n$> " : "\n> ");
            fflush(stderr);
            xbuf = fgets(buf, sizeof(buf), stdin);
        }
        if (docommand(buf) < 0)
            break;
    }
    return NULL;
}

extern int32_t ism_config_set_dynamic(ism_json_parse_t *json); //Not included in any external headers and only used
                                                               //here in a developer testing debug routine

/*
 * Testing routine used to update the location of the tracemodule or trace module options file
 * (setting it to 0 clear the trace module)
 */
void set_tracemodoroptions(const char *cfgitem, const char *args) {


    if(   (strcmp(cfgitem, TRACEMODULE_LOCATION_CFGITEM) != 0)
       && (strcmp(cfgitem, TRACEMODULE_OPTIONS_CFGITEM) != 0)) {
        printf( "Illegal cfg item: %s\n", cfgitem);
        return;
    }

    char inputJSON[256];
    sprintf(inputJSON, "{ \"Action\":\"set\",\"User\":\"admin\",\"Component\":\"Server\",\"Item\":\"%s\",\"Value\":\"%s\" }",
                           cfgitem, args);

    ism_json_parse_t parseObj = { 0 };
    ism_json_entry_t ents[20];

    int rc =0;

    parseObj.ent = ents;
    parseObj.ent_alloc = (int)(sizeof(ents)/sizeof(ents[0]));
    parseObj.source = (char *)ism_common_strdup(ISM_MEM_PROBE(ism_memory_main_misc,1000),inputJSON);
    parseObj.src_len = strlen(parseObj.source);

    rc = ism_json_parse(&parseObj);

    if (rc != OK)
    {
        printf("ERROR: ism_json_parse() for inputJSON returned %d\n", rc);
        goto mod_exit;
    }

    // Actually don't need to add an action or user, these are fields that are
    // only interesting the the outer 'admin' wrapper function.

    rc = ism_config_set_dynamic(&parseObj);
    printf("rc was %d\n", rc);


    mod_exit:

    if (parseObj.source != NULL)
    {
        // NOTE: We are not freeing the storage which was allocated (via strdup)
        //       because the code we call is referencing the area.
        //
        //       This doesn't sound quite right - but we would much rather be able
        //       to perform address sanitization on the code than worry about, what
        //       is a minor memory leak code only used in developer testing.

        // ism_common_free(ism_memory_main_misc,parseObj.source);
    }
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
            const char * name;
            while (ism_common_getPropertyIndex(ism_common_getConfigProperties(), i, &name)==0) {
                const char * value = ism_common_getStringConfig(name);
                printf("%s=%s\n", name, value);
                i++;
            }
        }

        /*
         * stat:  Show statistics
         */
        else if (!strcmpi(command, "stat")) {
            char *stype = ism_common_getToken(args, "\t\r\n ", "\t\r\n ", &args);
            if ((stype == NULL) || (stype[0] == 'c') || (stype[0] == 'C')) {
                ism_transport_printStats(args);
            } else  if ((stype[0] == 'q') || (stype[0] == 'Q')) {
                ism_prop_t *pFilterProperties = NULL;
                ism_field_t f;

                char *pQueueName = ism_common_getToken(args, "\t\r\n", "\t\r\n", &args);
                if (pQueueName != NULL) {
                    pFilterProperties = ism_common_newProperties(1);
                    f.type = VT_String;
                    f.val.s = pQueueName;
                    ism_common_setProperty(pFilterProperties, ismENGINE_MONITOR_FILTER_QUEUENAME, &f);
                } else {
                    pQueueName = "*";
                }

                ismEngine_QueueMonitor_t *pMonitor = NULL;
                uint32_t results = 0;
                uint32_t i;

                rc = ism_engine_getQueueMonitor(&pMonitor,
                                                &results,
                                                ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                                10,
                                                pFilterProperties);
                if (rc == OK) {
                    ismEngine_QueueMonitor_t *pCurrent = pMonitor;
                    for (i = 0; i < results; i++, pCurrent++) {
                        printf("QueueName(%s) [%p]\n{\n", pCurrent->queueName, pCurrent->queue);
                        printf("    Scope               %d\n", pCurrent->scope);
                        printf("    Producers           %d\n", pCurrent->producers);
                        printf("    Consumers           %d\n", pCurrent->consumers);
                        printf("    BufferedMsgs        %lu\n", pCurrent->stats.BufferedMsgs);
                        printf("    BufferedMsgsHWM     %lu\n", pCurrent->stats.BufferedMsgsHWM);
                        printf("    MaxMessages         %lu\n", pCurrent->stats.MaxMessages);
                        printf("    BufferedPercent     %3.2f%%\n", pCurrent->stats.BufferedPercent);
                        printf("    RejectedMsgs        %lu\n", pCurrent->stats.RejectedMsgs);
                        printf("    InflightEnqs        %lu\n", pCurrent->stats.InflightEnqs);
                        printf("    InflightDeqs        %lu\n", pCurrent->stats.InflightDeqs);
                        printf("    EnqueueCount        %lu\n", pCurrent->stats.EnqueueCount);
                        printf("    DequeueCount        %lu\n", pCurrent->stats.DequeueCount);
                        printf("    QAvoidCount         %lu\n", pCurrent->stats.QAvoidCount);
                        printf("    ProducedMsgs        %lu\n", pCurrent->stats.ProducedMsgs);
                        printf("    ConsumedMsgs        %lu\n}\n\n", pCurrent->stats.ConsumedMsgs);
                    }

                    ism_engine_freeQueueMonitor(pMonitor);
                }

                if (results == 0) {
                    printf("No queues matching queue name '%s'\n\n", pQueueName);
                }

                ism_common_freeProperties(pFilterProperties);
            } else  if ((stype[0] == 's') || (stype[0] == 'S')) {
                ism_prop_t *pFilterProperties = NULL;
                ism_field_t f;

                char *pTopicString = ism_common_getToken(args, "\t\r\n", "\t\r\n", &args);
                if (pTopicString != NULL) {
                    pFilterProperties = ism_common_newProperties(1);
                    f.type = VT_String;
                    f.val.s = pTopicString;
                    ism_common_setProperty(pFilterProperties, ismENGINE_MONITOR_FILTER_TOPICSTRING, &f);
                } else {
                    pTopicString = "*";
                }

                ismEngine_SubscriptionMonitor_t *pMonitor = NULL;
                uint32_t results = 0;
                uint32_t i;

                rc = ism_engine_getSubscriptionMonitor(&pMonitor,
                                                       &results,
                                                       ismENGINE_MONITOR_HIGHEST_BUFFEREDMSGS,
                                                       20,
                                                       pFilterProperties);
                if (rc == OK) {
                    ismEngine_SubscriptionMonitor_t *pCurrent = pMonitor;
                    for (i = 0; i < results; i++, pCurrent++) {
                        printf("SubscriptionName(%s) [sub:%p, pol:%p]\n{\n", pCurrent->subName ? pCurrent->subName : "",
                                pCurrent->subscription, pCurrent->policyInfo);
                        printf("    ConfigType          \"%s\"\n", pCurrent->configType ? pCurrent->configType : "");
                        printf("    TopicString         \"%s\"\n", pCurrent->topicString);
                        printf("    ClientID            \"%s\"\n", pCurrent->clientId ? pCurrent->clientId : "");
                        if (pCurrent->messagingPolicyName) {
                            printf("    MessagingPolicyName \"%s\"\n", pCurrent->messagingPolicyName);
                        } else {
                            printf("    * DELETED / UNIQUE POLICY *\n");
                        }
                        printf("    Durable             %s\n", pCurrent->durable ? "true":"false");
                        printf("    Shared              %s\n", pCurrent->shared ? "true":"false");
                        printf("    InternalAttrs       %d\n", pCurrent->internalAttrs);
                        printf("    Consumers           %d\n", pCurrent->consumers);
                        printf("    BufferedMsgs        %lu\n", pCurrent->stats.BufferedMsgs);
                        printf("    BufferedMsgsHWM     %lu\n", pCurrent->stats.BufferedMsgsHWM);
                        printf("    MaxMessages         %lu\n", pCurrent->stats.MaxMessages);
                        printf("    BufferedPercent     %3.2f%%\n", pCurrent->stats.BufferedPercent);
                        printf("    RejectedMsgs        %lu\n", pCurrent->stats.RejectedMsgs);
                        printf("    InflightEnqs        %lu\n", pCurrent->stats.InflightEnqs);
                        printf("    InflightDeqs        %lu\n", pCurrent->stats.InflightDeqs);
                        printf("    EnqueueCount        %lu\n", pCurrent->stats.EnqueueCount);
                        printf("    DequeueCount        %lu\n", pCurrent->stats.DequeueCount);
                        printf("    QAvoidCount         %lu\n", pCurrent->stats.QAvoidCount);
                        printf("    ProducedMsgs        %lu\n", pCurrent->stats.ProducedMsgs);
                        printf("    ConsumedMsgs        %lu\n}\n\n", pCurrent->stats.ConsumedMsgs);
                    }

                    ism_engine_freeSubscriptionMonitor(pMonitor);
                }

                if (results == 0) {
                    printf("No subscriptions with topic string matching '%s'\n\n", pTopicString);
                }

                ism_common_freeProperties(pFilterProperties);
            } else  if ((stype[0] == 't') || (stype[0] == 'T')) {
                ism_prop_t *pFilterProperties = NULL;
                ism_field_t f;

                char *pTopicString = ism_common_getToken(args, "\t\r\n", "\t\r\n", &args);
                if (pTopicString != NULL) {
                    pFilterProperties = ism_common_newProperties(1);
                    f.type = VT_String;
                    f.val.s = pTopicString;
                    ism_common_setProperty(pFilterProperties, ismENGINE_MONITOR_FILTER_TOPICSTRING, &f);
                } else {
                    pTopicString = "*";
                }

                ismEngine_TopicMonitor_t *pMonitor = NULL;
                uint32_t results = 0;
                uint32_t i;

                rc = ism_engine_getTopicMonitor(&pMonitor,
                                                &results,
                                                ismENGINE_MONITOR_HIGHEST_PUBLISHEDMSGS,
                                                10,
                                                pFilterProperties);
                if (rc == OK) {
                    ism_ts_t *ts = ism_common_openTimestamp(ISM_TZF_UTC);
                    ismEngine_TopicMonitor_t *pCurrent = pMonitor;
                    for (i = 0; i < results; i++, pCurrent++) {
                        printf("TopicString(%s) [%p]\n{\n", pCurrent->topicString, pCurrent->node);
                        if (ts != NULL) {
                            char buffer[80];

                            ism_common_setTimestamp(ts, pCurrent->stats.ResetTime);
                            ism_common_formatTimestamp(ts, buffer, 80, 6, ISM_TFF_SPACE|ISM_TFF_ISO8601);

                            printf("    ResetTime           %s\n", buffer);
                        }
                        else {
                            printf("    ResetTime           %lu\n", pCurrent->stats.ResetTime);
                        }
                        printf("    Subscriptions       %lu\n", pCurrent->subscriptions);
                        printf("    PublishedMsgs       %lu\n", pCurrent->stats.PublishedMsgs);
                        printf("    FailedPublishes     %lu\n", pCurrent->stats.FailedPublishes);
                        printf("    RejectedMsgs        %lu\n}\n\n", pCurrent->stats.RejectedMsgs);
                    }

                    if (ts != NULL) {
                        ism_common_closeTimestamp(ts);
                    }

                    ism_engine_freeTopicMonitor(pMonitor);
                }

                if (results == 0) {
                    printf("No Topic Monitors with topic string matching '%s'\n\n", pTopicString);
                }

                ism_common_freeProperties(pFilterProperties);
            } else  if ((stype[0] == 'x') || (stype[0] == 'X')) {
                bool found = false;
                ism_prop_t *pFilterProperties = NULL;
                ism_field_t f;

                char *pClientId = ism_common_getToken(args, "\t\r\n", "\t\r\n", &args);
                if (pClientId == NULL) {
                    pClientId = "*"; // No filter implies match all
                }
                else {
                    pFilterProperties = ism_common_newProperties(10);
                    f.type = VT_String;
                    f.val.s = pClientId;
                    ism_common_setProperty(pFilterProperties, ismENGINE_MONITOR_FILTER_CLIENTID, &f);
                }

                ismEngine_ClientStateMonitor_t *pMonitor = NULL;
                uint32_t results;
                uint32_t i;

                rc = ism_engine_getClientStateMonitor(&pMonitor,
                                                      &results,
                                                      ismENGINE_MONITOR_OLDEST_LASTCONNECTEDTIME,
                                                      10,
                                                      pFilterProperties);
                if (rc == OK) {
                    ismEngine_ClientStateMonitor_t *pCurrent = pMonitor;
                    for (i = 0; i < results; i++, pCurrent++) {
                        found = true;
                        printf("Client(%s)\n", pCurrent->ClientId);
                    }

                    ism_engine_freeClientStateMonitor(pMonitor);
                }

                if (!found) {
                    printf("No client-states matching client ID: %s\n\n", pClientId);
                }

                ism_common_freeProperties(pFilterProperties);
            }
            else {
                printf("Usage: stat [C] [S <TopicString>] | [Q <QueueName>] | [T <TopicString>] | [X <ClientId>]\n");
            }
        }

        /*
         * tcpstat: show tcp stats
         */
        else if (!strcmpi(command, "tcpstat")) {
            char * params[8] = {0};
            char * arg;
            int i = 0;
            while((arg = ism_common_getToken(args, "\t\r\n ", "\t\r\n ", &args)) && (i < 8) ) {
                params[i++] = arg;
            }
            ism_transport_printStatsTCP(params, i);
        }

        /*
         * Forwarder monitoring rates
         */
        else if (!strcmp(command, "fwdmon")) {
            fwd_monstat_t monstat = {0};
            rc = ism_fwd_getMonitoringStats(&monstat, 0);
            printf("fmon: rc=%u count=%u recvrate=%u sendrate0=%u sendrate1=%u\n",
                    rc, monstat.channel_count, monstat.recvrate, monstat.sendrate0, monstat.sendrate1);
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
                int arc = ism_protocol_setACLfile(args, NULL);
                if (arc) {
                    printf("setacl: rc=%d\n", arc);
                }
            }
        }

        /*
         *  dumpStore: Dumps the contents of the store
         */
        else if (!strcmpi(command, "dumpStore")) {
            ism_store_dump(args);
        }
        else if (!strcmpi(command,"traceMod")) {
            set_tracemodoroptions(TRACEMODULE_LOCATION_CFGITEM, args);
        }
        else if (!strcmpi(command,"traceModOptions")) {
            set_tracemodoroptions(TRACEMODULE_OPTIONS_CFGITEM, args);
        }
        else if (!strcmpi(command, "dumpPobj")) {
            char *stype = ism_common_getToken(args, "\t\r\n ", "\t\r\n ", &args);
            int id = 1;
            if (stype != NULL) {
              id = atoi(stype);
            }
            ism_transport_dumpConnectionPObj(id);
        }
        else if (!strcmpi(command, "debugSet")) {
            char *keyword;
            while ((keyword=ism_common_getToken(args, "\t\r\n ", "\t\r\n ", &args))) {
                if (!strncmpi(keyword, "level=", strlen("level="))) {
                    debuglevel=atoi(&keyword[strlen("level=")]);
                }
                else if (!strncmpi(keyword, "file=", strlen("file="))) {
                    ism_common_strlcpy(debugfile, &keyword[strlen("file=")], sizeof(debugfile)-1);
                }
                else if (!strncmpi(keyword, "userdata=", strlen("userdata="))) {
                    debuguserdata = atoi(&keyword[strlen("userdata=")]);
                }
            }

        }
        else if (!strcmpi(command, "debugClient")) {
            char *clientId = ism_common_getToken(args, "\t\r\n", "\t\r\n", &args);
            ism_common_strlcpy(xbuf, debugfile, sizeof(xbuf)-10);
            rc = ism_engine_dumpClientState(clientId, debuglevel, debuguserdata, xbuf);

            if (rc == OK) {
                if (xbuf[0] != '\0') {
                    printf("Dump written to file '%s' - use imadumpformatter to format it\n",
                           xbuf);
                }
            }
            else {
                char *errstr = (char *)ism_common_getErrorString(rc, xbuf, sizeof(xbuf));
                printf("ism_engine_dumpClientState(\"%s\", ...) rc=%d: %s\n",
                       clientId ? clientId : "", rc, errstr ? errstr : "Unknown error");
            }
        }
        else if (!strcmpi(command, "debugTopic")) {
            char *topicString = ism_common_getToken(args, "\t\r\n", "\t\r\n", &args);
            if (topicString == NULL) {
                printf("Usage: debugTopic <topicString>\n e.g. debugTopic topic1\n\n");
            }
            else {
                ism_common_strlcpy(xbuf, debugfile, sizeof(xbuf)-10);
                rc = ism_engine_dumpTopic(topicString, debuglevel, debuguserdata, xbuf);

                if (rc == OK) {
                    if (xbuf[0] != '\0') {
                        printf("Dump written to file '%s' - use imadumpformatter to format it\n",
                               xbuf);
                    }
                }
                else {
                    char *errstr = (char *)ism_common_getErrorString(rc, xbuf, sizeof(xbuf));
                    printf("ism_engine_dumpTopic(\"%s\", ...) rc=%d: %s\n",
                            topicString, rc, errstr ? errstr : "Unknown error");
                }
            }
        }
        else if (!strcmpi(command, "debugTopicTree")) {
            char *topicString = ism_common_getToken(args, "\t\r\n", "\t\r\n", &args);
            ism_common_strlcpy(xbuf, debugfile, sizeof(xbuf)-10);
            rc = ism_engine_dumpTopicTree(topicString, debuglevel, debuguserdata, xbuf);

            if (rc == OK) {
                if (xbuf[0] != '\0') {
                    printf("Dump written to file '%s' - use imadumpformatter to format it\n",
                           xbuf);
                }
            }
            else {
                char *errstr = (char *)ism_common_getErrorString(rc, xbuf, sizeof(xbuf));
                printf("ism_engine_dumpTopicTree(\"%s\", ...) rc=%d: %s\n",
                        topicString, rc, errstr ? errstr : "Unknown error");
            }
        }
        else if (!strcmpi(command, "debugQueue")) {
            char *queueName = ism_common_getToken(args, "\t\r\n", "\t\r\n", &args);
            ism_common_strlcpy(xbuf, debugfile, sizeof(xbuf)-10);
            rc = ism_engine_dumpQueue(queueName, debuglevel, debuguserdata, xbuf);

            if (rc == OK) {
                if (xbuf[0] != '\0') {
                    printf("Dump written to file '%s' - use imadumpformatter to format it\n",
                           xbuf);
                }
            }
            else {
                char *errstr = (char *)ism_common_getErrorString(rc, xbuf, sizeof(xbuf));
                printf("ism_engine_dumpQueue(\"%s\", ...) rc=%d: %s\n",
                       queueName ? queueName : "", rc, errstr ? errstr : "Unknown error");
            }
        }
        else if (!strcmpi(command, "debugLocks")) {
            char *lockName = ism_common_getToken(args, "\t\r\n", "\t\r\n", &args);
            ism_common_strlcpy(xbuf, debugfile, sizeof(xbuf)-10);
            rc = ism_engine_dumpLocks(lockName, debuglevel, debuguserdata, xbuf);

            if (rc == OK) {
                if (xbuf[0] != '\0') {
                    printf("Dump written to file '%s' - use imadumpformatter to format it\n",
                           xbuf);
                }
            }
            else {
                char *errstr = (char *)ism_common_getErrorString(rc, xbuf, sizeof(xbuf));
                printf("ism_engine_dumpLocks(\"%s\", ...) rc=%d: %s\n",
                      lockName ? lockName : "", rc, errstr ? errstr : "Unknown error");
            }
        }
        /*
         * tracedump: Dump out the in-memory trace if we're using it
         */
        else if (!strcmpi(command, "traceDump")) {
#ifdef ISM_INMEMORY_TRACE
            if (args != NULL && (strlen(args) > 0) && (args[0] != ' ')) {
                ism_common_memtrace_dump(args);
            } else {
                printf("Error command format should be: tracedump <filename>\n");
            }
#else
            printf("In memory trace is not enabled!\n");
#endif
        }
        else if (!strcmpi(command, "mallocTrim")) {
            malloc_trim(0);
            printf("Malloc trim completed\n");
        }
        else if (!strcmpi(command, "mallocStats")) {
            malloc_stats();
        }
        else if (!strcmpi(command, "memleak")) {
            uint64_t allocSize = 0;
            uint64_t numAllocs = 0;

            char *allocSizeStr = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            if (allocSizeStr != NULL) {
                allocSize = strtoul(allocSizeStr, NULL, 0);
            }
            char *numAllocsStr = NULL;
            if (args != NULL) {
                numAllocsStr = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            }
            if (numAllocsStr != NULL) {
                numAllocs = strtoul(numAllocsStr, NULL, 0);
            }

            if (allocSize == 0 || numAllocs == 0) {
                printf("Usage: memleak <allocsize> <numallocs>\n\n");
            } else {
                uint64_t i=0;

                for (i=0; i < numAllocs; i++) {
                    char *membuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_main_misc,3),allocSize);

                    if (membuf != NULL) {
                        //write to each page of the membuf
                        for (uint64_t j = 0 ; j < allocSize; j += 1024) {
                            membuf[j] = 'a';
                        }
                    }
                    else
                    {
                        break;
                    }
                }                /* BEAM suppression: memory leak */
                printf("Allocated %lu blocks each of %lu bytes\n", i, allocSize);
            }
        }
        /*
         * quit: Terminate the server
         */
        else if (!strcmpi(command, "quit")) {
            __sync_or_and_fetch(&sigterm_count, 3);
            pthread_kill(mainthread, SIGTERM);
            return -99;
        }
        /*
         * quit: Terminate the server
         */
        else if (!strcmpi(command, "drop")) {
            cleanStore = 1;
            __sync_or_and_fetch(&sigterm_count, 3);
            pthread_kill(mainthread, SIGTERM);
            return -99;
        }

        /*
         * list or help: List of available commands
         */
        else if (!strcmpi(command, "list") || !strcmpi(command, "help") || !strcmpi(command, "?")) {
            printf("List of supported commands:\n");
            printf(" acl [name]           Show ACLs\n");
            printf(" certprof [name*]     Show certificate profiles\n");
            printf(" config               Show configuration properties\n");
            printf(" countDroppedMsgs     Count number of dropped messages\n");
            printf(" debugSet [level=0-9] Set debug verbosity\n");
            printf(" debugClient          Display debug client information\n");
            printf(" debugTopic           Display debug topic information\n");
            printf(" debugTopicTree       Display debug topic tree information\n");
            printf(" debugQueue           Display debug queue information\n");
            printf(" debugLocks           Display debug locks information\n");
            printf(" drop                 Drop the store and terminate the server\n");
            printf(" dumpStore            Dumps the contents of the store\n");
            printf(" endpoint [name*]     Show the endpoints\n");
            printf(" mallocTrim           See if we can free any memory back to the system\n\n");
            printf(" mallocStats          Prints some technical debug information from our underlying malloc\n\n");
            printf(" memLeak              Allocates memory that is never freed\n\n");
            printf(" quit                 Terminate the server\n");
            printf(" secprof [name*]      Show the security profiles\n");
            printf(" stat C               Show connection statistics\n");
            printf(" stat Q <queue>       Show queue statistics\n");
            printf(" stat S <topic>       Show subscription statistics for topic\n");
            printf(" stat T <topic>       Show topic monitor statistics for topic\n");
            printf(" stat X <clientId>    Show disconnected MQTT client statistics\n");
            printf(" time                 Show server time\n");
            printf(" traceLevel           Show or set the trace level\n");
            printf(" traceMod             Set a trace module location\n");
            printf(" traceModOptions      Set a trace module options file location\n\n");
        }

        else if (!strcmpi(command, "endpoint")) {
            if (!args || !*args)
                args = "*";
            ism_transport_printEndpoints(args);
        }
        else if (!strcmpi(command, "disconnect")) {
            ism_transport_disconnectEndpoint(99, "disconnect command", args);
        }
        else if (!strcmpi(command, "cluster")) {
            concat_alloc_t zbuf = {xbuf, sizeof xbuf};
            ism_fwd_getForwarderStats(&zbuf, 2);
            ism_common_allocBufferCopyLen(&zbuf, "", 1);
            zbuf.used--;
            printf("%s\n", zbuf.buf);
            ism_common_freeAllocBuffer(&zbuf);
        }
        else if (!strcmpi(command, "setendpoint")) {
            ism_transport_setEndpoint(args);
        }
        else if (!strcmpi(command, "secprof")) {
            if (!args || !*args)
                args = "*";
            ism_transport_printSecProfile(args);
        }
        else if (!strcmpi(command, "setsecprof")) {
            ism_transport_setSecProf(args);
        }
        else if (!strcmpi(command, "certprof")) {
            if (!args || !*args)
                args = "*";
            ism_transport_printCertProfile(args);
        }
        else if (!strcmpi(command, "setcertprof")) {
            ism_transport_setCertProf(args);
        }
        else if (!strcmpi(command, "delay")) {
            extern int64_t g_delayCount;
            extern int64_t g_delayMemory;
            printf("Delay count=%ld size=%ld\n", g_delayCount, g_delayMemory);
        }
        else if (!strcmpi(command, "msghub")) {
            if (!args || !*args)
                args = "*";
            ism_transport_printMsgHub(args);
        }
        else if (!strcmpi(command, "setmsghub")) {
            ism_transport_setMsgHub(args);
        }
        else if (!strcmpi(command, "tracelevel")) {
            if (args && *args) {
                rc = ism_common_setTraceLevelX(ism_defaultTrace, args);
                if (rc) {
                    printf("The trace level is not valid: rc=%d\n", rc);
                } else {
                    printf("trace level = %s\n", args);
                }
            }

        }
        else if (!strcmpi(command, "traceselected")) {
            if (args && *args) {
                rc = ism_common_setTraceLevelX(&ism_defaultDomain.selected, args);
                if (rc) {
                    printf("The trace selected level is not valid: rc=%d\n", rc);
                } else {
                    printf("trace selected = %s\n", args);
                }
            }
        }
        else if (!strcmpi(command, "traceconnection")) {
            if (args && *args) {
                rc = ism_common_setTraceConnection(args);
                if (rc) {
                    printf("The trace connection is not valid: rc=%d\n", rc);
                } else {
                    printf("trace connection = %s\n", args);
                }
            }
        }
        else if (!strcmpi(command, "tracefilter")) {
            if (args && *args) {
                rc = ism_common_setTraceFilter(args);
                if (rc) {
                    ism_common_formatLastError(xbuf, sizeof xbuf);
                    printf("%s\n", xbuf);
                }
            }
        }
        /*
         * countDroppedMsgs: Count number of dropped messages
         */
        else if (!strcmpi(command, "countDroppedMsgs")) {
            ismEngine_MessagingStatistics_t msgStats;
            ism_engine_getMessagingStatistics(&msgStats);
            if (msgStats.DroppedMessages == 0)
            {
                printf("There have been no dropped messages.\n");
            }
            else
            {
                printf("Number of dropped messages: %lu.\n", msgStats.DroppedMessages);
            }
            if (msgStats.ExpiredMessages == 0)
            {
                printf("There have been no expired messages.\n");
            }
            else
            {
                printf("Number of expired messages: %lu.\n", msgStats.ExpiredMessages);
            }
            printf("Total buffered messages: %lu.\n", msgStats.BufferedMessages);
            printf("Total retained messages: %lu.\n", msgStats.RetainedMessages);
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


/*
 * Check if a SIGTERM has been raised and start shutdown.
 * We cannot do this in the signal handler as locks cannot be used in a signal handler.
 */
static int sigterm_check(void * userdata) {
    if (__sync_add_and_fetch(&sigterm_count, 0)) {
        TRACE(1, "SIGTERM found, starting controlled shutdown\n");
        ism_admin_send_stop(ISM_ADMIN_STATE_STOP);
        sigtermHandler = NULL;
        return -1;
    }
    return 0;
}

/*
 * Start up a sigterm timer when we enter admin pause
 */
static void setAdminPause(void) {
    in_admin_pause = 1;
    TRACE(1, "Enable SIGTERM check\n");
    if (sigtermHandler == NULL) {
        sigtermHandler = ism_common_addUserHandler(sigterm_check, NULL);
    }
}

/*
 * Catch a terminating signal and try to flush the trace file.
 */
static void sig_handler(int signo) {
    static volatile int in_handler = 0;
    signal(signo, SIG_DFL);
    if (in_handler)
        raise(signo);
    in_handler = 1;
    ism_common_traceSignal(signo);
    raise(signo);
}

/*
 * Catch a terminating signal and try to flush the trace file.
 */
static void sig_sigterm(int signo) {
    signal(signo, SIG_DFL);
    if (!in_admin_pause)
        raise(signo);
    /* Set atomically */
    __sync_or_and_fetch(&sigterm_count, 1);
    __sync_or_and_fetch(&g_doUser2, 1);
}

/*
 * SIGUSER2 signal handler
 * Just set the bit and we check it after return from a pause
 */
static void siguser2_handler(int signo) {
    __sync_or_and_fetch(&g_doUser2, 1);
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
