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
#define TRACE_COMP Util
#include <ismutil.h>
#include <stdio.h>
#include <perfstat.h>
#include <config.h>
#include "log.h"
#include <sys/resource.h>
#include <trace.h>

#include "tracemodule.h"

extern void ism_common_initTrace(void);
extern void ism_common_initTimers(void);
extern int  ism_common_ifmap_init(void);
extern void ism_common_setServerName(const char * name);
extern void ism_common_setServerUID(const char * uid);
void ism_common_initUUID(void);

extern ism_enumList enum_auxlogger_settings [];

static volatile int server_started = 0;


/*
 *	Internal function to initialize the Aux Loggers.
 */
void ism_common_initLoggers(ism_prop_t * props, int mqonly) {
    ism_log_init();

    ism_log_initSyslog(props);

    ism_log_createLogger(LOGGER_SysLog, props);
    if (!mqonly) {
        ism_log_createLogger(LOGGER_Connection, props);
        ism_log_createLogger(LOGGER_Security, props);
    }
    ism_log_createLogger(LOGGER_Admin, props);
    ism_log_createLogger(LOGGER_MQConnectivity, props);
}

/*
 * Init only the MQ loggers
 */
void ism_common_initAuxLoggerMQ(ism_prop_t * props) {
    ism_common_initLoggers(props, 1);
}

/*
 * Start the IMA server.
 * All initialization of the server is done in this function, but it is not run.
 * If this method returns a good return code the server will be run.
 * @return A return code, 0=good.
 */
int  ism_common_initServer(void) {
    struct rlimit rlim;

    /* Interface mapping */
    ism_common_ifmap_init();

    /* Platform data */
    ism_common_initPlatformDataFile();

    /*
     * Process file limits
     */
    int filelimit = (ism_common_getIntConfig("TcpMaxConnections", 1024) * 2) + 512;
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

    rlim.rlim_max = RLIM_INFINITY;
    rlim.rlim_cur = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rlim);
    getrlimit(RLIMIT_CORE, &rlim);
    TRACE(4, "Set core file size limit=%u\n", (uint32_t)rlim.rlim_cur);

    /*
     * Everything prior to this point is run single threaded.
     * After this point there are multiple threads.
     */

    /*
     * Start timer threads
     */
    ism_common_initTimers();

    /*
     * Start trace worker thread
     */
    ism_trace_startWorker();

    /*
     * Initialize performance stats
     */
    ism_perf_initPerfstat();

    /* Config hashMap lock type */
    ism_hashMapInit();

    ism_common_initUUID();
    return 0;
}

extern ISM_LOGLEV ism_g_loglevel;

/*
 * Change the setting of an aux logger.
 * We assume that config has already done the
 */
static void setAuxLog(ism_domain_t * domain, int logger, ism_prop_t * props, const char * logname) {
    int auxsetting;
    const char * setting = ism_common_getStringProperty(props, logname);
    if (!setting)
        setting = "Normal";
    auxsetting = ism_common_enumValue(enum_auxlogger_settings, setting);
    if (auxsetting != INVALID_ENUM) {
        domain->trace.logLevel[logger] = auxsetting;
        domain->selected.logLevel[logger] = auxsetting == AuxLogSetting_Max ? auxsetting : auxsetting+1;
        TRACE(6, "Set the log level: Domain=%s Log=%s Level=%s\n", domain->name, logname, setting);
    } else {
        ism_common_setErrorData(ISMRC_BadPropertyValue, "%s%s", logname, setting);
    }
}


/*
 * Do one time initialization which defaults to static config if the value
 * is not specified in the static config.
 */
XAPI int ism_server_initt(ism_prop_t * props) {
    const char * trcmax = ism_common_getStringProperty(props, "TraceMax");
    const char * trcopt  = ism_common_getStringProperty(props, "TraceOptions");
    const char * trcconn = ism_common_getStringProperty(props, "TraceConnection");
    int  trcsize;

    if (!trcmax) {
        trcmax = ism_common_getStringConfig("TraceMax");
    }

    trcsize = ism_common_getBuffSize("TraceMax", trcmax, "20M");
    ism_common_setTraceMax(trcsize);

    if (!trcopt) {
        trcopt = ism_common_getStringConfig("TraceOptions");
        if (trcopt)
            ism_common_setTraceOptions(trcopt);
    }

    if (!trcconn) {
        trcconn = ism_common_getStringConfig("TraceConnection");
        if (trcconn)
            ism_common_setTraceConnection(trcconn);
    }
    return 0;
}

/*
 * Validate a server UID.
 * A valid serverUID is 1 to 16 bytes containing only alpahnumerics, ',', and '-'.
 * Our generated serverUID has 8 bytes of alphanumeric with an optional '.'
 * followed by an integer.
 */
int ism_common_validServerUID(const char * s) {

    if (!s || !*s || strlen(s) > 16)
        return 0;
    while (*s) {
       if ((*s < 'A' || *s > 'Z') &&
           (*s < 'a' || *s > 'z') &&
           (*s < '0' || *s > '9') &&
           *s != '.' && *s != '-') {
           return 0;
       }
       s++;
    }
    return 1;
}

/*
 * Configuration callback for server.
 *
 * We ignore the flag as all of the server items are simple.
 */
XAPI int ism_server_config(char * object, char * namex, ism_prop_t * props, ism_ConfigChangeType_t flag) {
    int  rc = 0;
    int  i;
    const char * name;
    const char * value;
    int logLocationChanged = 0;
    int isSyslog = 0;
    const char logLocStr[] = "LogLocation.";
    const int logLocLen = sizeof(logLocStr) - 1;
    const char syslogStr[] = "Syslog.";
    const int syslogLen = sizeof(syslogStr) - 1;

    for (i = 0; ism_common_getPropertyIndex(props, i, &name) == 0; i++) {
        /* Change trace level */
        if (!strcmp(name, "TraceLevel")) {
            const char * trclvl = ism_common_getStringProperty(props, "TraceLevel");
            ism_common_setTraceLevelX(ism_defaultTrace, trclvl);
            TRACE(3, "Change TraceLevel=%s\n", trclvl);
        }
        /* Change trace selection level */
        else if (!strcmp(name, "TraceSelected")) {
            ism_common_setTraceLevelX(&ism_defaultDomain.selected, ism_common_getStringProperty(props, "TraceSelected"));
        }
    }

    for (i = 0; ism_common_getPropertyIndex(props, i, &name) == 0; i++) {
        /*
         * Change the trace filter
         */
        if (!strcmp(name, "TraceFilter")) {
            rc = ism_common_setTraceFilter(ism_common_getStringProperty(props, "TraceFilter"));
        }

        /*
         * Change the trace maximum size
         */
        else if (!strcmp(name, "TraceMax")) {
            int trcsize = ism_common_getBuffSize("TraceMax", ism_common_getStringProperty(props, "TraceMax"), "20M");
            ism_common_setTraceMax(trcsize);
        }

        /*
         * Change the trace options
         */
        else if (!strcmp(name, "TraceOptions")) {
            ism_common_setTraceOptions(ism_common_getStringProperty(props, "TraceOptions"));
        }

        /*
         * Change the trace message data
         */
        else if (!strcmp(name, "TraceMessageData")) {
            ism_common_setTraceMsgData(ism_common_getIntProperty(props, "TraceMessageData", 0));
        }

        /*
         * Change the trace message data
         */
        else if (!strcmp(name, "TraceSelected")) {
            ism_common_setTraceLevelX(&ism_defaultDomain.selected, ism_common_getStringProperty(props, "TraceSelected"));
        }

        /*
         * Change the trace message data
         */
        else if (!strcmp(name, "TraceConnection")) {
            ism_common_setTraceConnection(ism_common_getStringProperty(props, "TraceConnection"));
        }

        /*
         * Change trace backup mode
         */
        else if (!strcmp(name, "TraceBackup")) {
            ism_common_setTraceBackup(ism_common_getIntProperty(props, "TraceBackup", 1));
        }

        /*
         * Change trace backup destination
         */
        else if (!strcmp(name, "TraceBackupDestination")) {
            ism_common_setTraceBackupDestination(ism_common_getStringProperty(props, "TraceBackupDestination"));
        }

        /*
         * Change max trace backup file count
         */
        else if (!strcmp(name, "TraceBackupCount")) {
            ism_common_setTraceBackupCount(ism_common_getIntProperty(props, "TraceBackupCount", 3));
        }

        /*
         * Change the log level
         */
        else if (!strcmp(name, "LogLevel")) {
            setAuxLog(&ism_defaultDomain, LOGGER_SysLog, props, "LogLevel");
        }


        /*
         * Change Connection log setting
         */
        else if (!strcmp(name, "ConnectionLog")) {
            setAuxLog(&ism_defaultDomain, LOGGER_Connection, props, "ConnectionLog");
        }

        /*
         * Change Security Log Setting
         */
        else if (!strcmp(name, "SecurityLog")) {
            setAuxLog(&ism_defaultDomain, LOGGER_Security, props, "SecurityLog");
        }

        /*
         * Change Admin Log Setting
         */
        else if (!strcmp(name, "AdminLog")) {
            setAuxLog(&ism_defaultDomain, LOGGER_Admin, props, "AdminLog");
        }

        /*
         * Change MQConnectivity Log Setting
         */
        else if (!strcmp(name, "MQConnectivityLog")) {
            setAuxLog(&ism_defaultDomain, LOGGER_MQConnectivity, props, "MQConnectivityLog");
        }

        /*
         * Set the server name.  This can be changed at any time
         */
        else if (!strcmp(name, "ServerName")) {
            value = ism_common_getStringProperty(props, "ServerName");
            int len = (int)strlen(value);
            if (ism_common_validUTF8Restrict(value, len, UR_NoControl | UR_NoSpace | UR_NoNonchar) < 0) {
                if (server_started) {
                    rc = ISMRC_BadPropertyValue;
                    ism_common_setErrorData(rc, "%s%s", name, "(BAD)");  /* name not shown as it is invalid UTF-8 */
                } else {
                    TRACE(3, "There server name is not valid,\n");
                }
                break;
            }
            ism_common_setServerName(value);
        }

        /*
         * Set the licensed usage
         */
        else if (!strcmp(name, "LicensedUsage")) {
            value = ism_common_getStringProperty(props, "LicensedUsage");
            if (!value || *value == 0) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(rc, "%s%s", name, "NULL");
                break;
            }

        	if (ism_common_setPlatformLicenseType(value) != ISMRC_OK) {
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(rc, "%s%s", name, value);
                break;
        	}
        }

        /*
         * Set the server UID.  This can only be changed if it is currently unset
         */
        else if (!strcmp(name, "ServerUID")) {
            value = ism_common_getStringProperty(props, "ServerUID");
            if (!ism_common_validServerUID(value)) {
                const char * val;
                if (ism_common_validUTF8Restrict(value, strlen(value), UR_NoControl | UR_NoNonchar) < 1)
                    val = "(BAD)";   /* Invalid UTF-8 should not be put in trace */
                else
                    val = value;
                if (server_started) {
                    rc = ISMRC_BadPropertyValue;
                    ism_common_setErrorData(rc, "%s%s", name, val);  /* name not shown as it is invalid UTF-8 */
                } else {
                    TRACE(3, "The server UID is not valid: %s\n", val);
                }
                break;
            }
            if (ism_common_getServerUID() == NULL) {
                ism_common_setServerUID(ism_common_getStringProperty(props, "ServerUID"));
                ism_common_setServerName(ism_common_getServerName());
            }
        }

        /*
         * Has a trace module been installed
         */
        else if (!strcmp(name, TRACEMODULE_LOCATION_CFGITEM)) {
        	const char *traceModLocation = ism_common_getStringProperty(props, TRACEMODULE_LOCATION_CFGITEM);

        	if ((traceModLocation != NULL)
        		  && (strcmp(traceModLocation, "0") != 0)
        		  && (strcmp(traceModLocation, "")  != 0) ) {
        		ism_common_TraceModuleDynamicUpdate(props);
        	} else {
        		ism_common_TraceModuleClear();
        		TRACE(3, "Reset to normal trace routines.\n");
        	}
        }

        /*
         * Log location has been updated
         */
        else if (object && !logLocationChanged && !strncmp(name, logLocStr, logLocLen)) {
        	int which = LOGGER_SysLog;
        	if (!strcmp(namex, "ConnectionLog")) {
        		which = LOGGER_Connection;
        	} else if (!strcmp(namex, "AdminLog")) {
        		which = LOGGER_Admin;
        	} else if (!strcmp(namex, "SecurityLog")) {
        	    which = LOGGER_Security;
        	} else if (!strcmp(namex, "MQConnectivityLog")) {
        	    which = LOGGER_MQConnectivity;
        	}

        	TRACE(5, "Updating logger: %d\n", which);

            rc = ism_log_updateLogger(which, props);

            logLocationChanged = 1;
        }

        /*
         * Syslog info has been updated
         */
        else if (object && !strncmp(name, syslogStr, syslogLen)) {
        	isSyslog = 1;
        }
    }

    if (isSyslog) {
    	rc = ism_log_initSyslog(props);
    	if (rc == -1) {
    		rc = ISMRC_UnableToConnect;
    	}
    }

    ism_common_TraceModuleConfigUpdate(props);

    return rc;
}



/*
 * Run the IMA server.
 * This call starts the server, and should return to the invoker when the server is started.
 * If this method returns a bad return code, the server will be immediately terminated.
 * @return A return code, 0=good.
 */
int  ism_common_startServer(void) {
    int  i;
    ism_prop_t * props;
    const char * name;

    TRACE(1, "Start the " IMA_SVR_COMPONENT_NAME_FULL ": ServerName=[%s] ServerUID=%s\n",
            ism_common_getServerName(), ism_common_getServerUID());

    /*
     * Trace the configuration properties
     */
    props = ism_common_getConfigProperties();
    for (i=0;;i++) {
        if (ism_common_getPropertyIndex(props, i, &name))
            break;
        if (*name != '_')
            TRACEX(5, Util, TOPT_NOGLOBAL, "Config: %s = %s\n", name, ism_common_getStringConfig(name));
    }

    server_started = 1;
    return 0;
}


/*
 * Terminate the IMA server.
 * @return A return code, 0=good.
 */
int  ism_common_termServer(void) {
    TRACE(1, "Terminate the " IMA_SVR_COMPONENT_NAME_FULL "\n");
    return 0;
}
