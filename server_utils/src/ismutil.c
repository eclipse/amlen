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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define TRACE_COMP Util
#include <ismutil.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include "unicode/unum.h"
#include <sys/syscall.h>
#include "unicode/ustring.h"
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>
#include <fendian.h>
#include <config.h>

#ifndef _WIN32
#include <sys/prctl.h>
#endif
#ifndef XSTR
#define XSTR(s) STR(s)
#define STR(s) #s
#endif

#include <trace.h>
#include "tracemodule.h"

#define ISM_COMMON_THREADDATA_HOME  //This is the place we are going to actual define ism_common_threaddata so don't declare extern
#include "threadlocal.h"
#undef ISM_COMMON_THREADDATA_HOME

#define NANOS_IN_HOUR 3600000000000L

/* Names of function names we look for in in pluggable trace module  */
#define ISM_COMMON_TRACEINIT_FUNC             "ism_initTraceModule"
#define ISM_COMMON_TRACE_FUNC                 "ism_insertTrace"
#define ISM_COMMON_TRACEDATA_FUNC             "ism_insertTraceData"
#define ISM_COMMON_SETERROR_FUNC              "ism_insertSetError"
#define ISM_COMMON_SETERRORDATA_FUNC          "ism_insertSetErrorData"
#define ISM_COMMON_TRACEMODULECFGUPDATE_FUNC  "ism_insertCfgUpdate"

/*
 * Global variables
 */
static ism_prop_t * ism_g_config_props = NULL;
char         g_procname[20];
pthread_mutex_t     g_utillock;
pthread_mutex_t     trc_lock;
static ism_threadkey_t     ism_threadKey;
static char g_runtime_dir[PATH_MAX] = ""; //temporary directory retrieved via ism_common_getRuntimeTempDir() 

//thread local storage used for memory account, in memory tracing, other pervasive things
__thread ism_tls_t *ism_common_threaddata = NULL;


extern char * ismcli_ISMClient(char *user, char *protocol, char *command, int proctype);

/*
 * Thread local storage for utils
 */

/*----  start stuff for thread health monitoring  -----*/
typedef int (*admin_restart_t)(int, void *);
static double tooLongTH=300; // 5 Mins
static int traceLoops=5 ;
static int stopLoops=5 ;
static int i_am_the_main=0;
static double next_st_trace=0;
static int stack_trace_cnt;

/* Health categories */
typedef enum {
    HC_OTH,
    HC_ADM,   /* Admin */
    HC_IOP,   /* TCP   */
    HC_SEC,   /* Security */
    HC_STO,   /* Store */
    HC_MAX
} ism_health_category_t ;

typedef struct ism_tls_health_t {
    struct ism_tls_health_t *next;
    double                   shouldBeBackAt;
    pthread_t                myId;
    pid_t                    myLWP;
    ism_health_category_t    myCat;
    char                     myName[20];
} ism_tls_health_t;

static int healthAlert=0;
static int adminStop  =0;
static double shouldBeBackAt;
static ism_tls_health_t *healthHead=NULL;
static __thread ism_tls_health_t *threadHealth=NULL;

/*----  end   stuff for thread health monitoring  -----*/


static ism_tls_t * makeTLS(int need, const char * name);
static void constructMsgId(ism_rc_t code, char * buffer);

ism_common_TraceFunction_t traceFunction               = ism_common_trace;
ism_common_TraceDataFunction_t traceDataFunction       = ism_common_traceData;
ism_common_SetErrorFunction_t setErrorFunction         = ism_common_setError_int;
ism_common_SetErrorDataFunction_t setErrorDataFunction = ism_common_setErrorData_int;
ism_common_TraceModuleCfgUpdate_t traceModuleCfgUpdate = NULL;

// First failure data capture trace level
#define TRACE_FFDC               2

/*
 * External non-public interfaces
 */
XAPI void ism_common_initExecEnv(int type);

/*
 * Initialize the utilities.
 * This should be called only once
 */
XAPI void  ism_common_initUtil2(int type) {
    static int inited = 0;
    int    rc;
    if (!inited) {
        inited = 1;

        /* Initialize thread local storage for the main thread */
        rc = ism_common_createThreadKey(&ism_threadKey);
        makeTLS(1024, g_procname);

        /* Initialize properties and locks */
        ism_g_config_props = ism_common_newProperties(256);
        pthread_mutex_init(&g_utillock, 0);
        pthread_mutex_init(&trc_lock, 0);

        rc = prctl(PR_GET_NAME, (unsigned long)(uintptr_t)g_procname);
        if (rc)
            strcpy(g_procname, "imaserver");

        /* Work out a temp dir to use */
        if(getenv("IMASERVER_RUNTIME_DIR")) {
            if(snprintf(g_runtime_dir, sizeof(g_runtime_dir), "%s", getenv("IMASERVER_RUNTIME_DIR")) >= sizeof(g_runtime_dir)) {
                g_runtime_dir[0] = '\0';
            }
        }
        if (strlen(g_runtime_dir) == 0) {
            snprintf(g_runtime_dir, sizeof(g_runtime_dir), "/tmp");
        }

        /* Discover the execution environment */
        ism_common_initExecEnv(type);
    }
}

XAPI void  ism_common_initUtil(void) {
    ism_common_initUtil2(0);
}

XAPI void  ism_common_initUtilMain(void) {
    i_am_the_main = 1;
    ism_common_initUtil();
}

XAPI void ism_common_makeTLS(int need, const char * name) {
    makeTLS(need,name);
}
/*
 * Make the TLS for a thread
 */
static ism_tls_t * makeTLS(int need, const char * name) {
    ism_tls_t * tls = calloc(1, sizeof(ism_tls_t)+need);
    if (tls != NULL) {
        char namebuf[20] = "";
        ism_common_threaddata = tls;
        ism_common_initializeThreadMemUsage();
        tls->trc_ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
        tls->data_alloc = need;
        if (!name) {
            prctl(PR_GET_NAME, (unsigned long)(uintptr_t)namebuf);
            name = (const char *)namebuf;
        }
        ism_common_strlcpy(tls->tname, name, sizeof(tls->tname));
        tls->tname_len = (int)strlen(tls->tname);
        ism_common_setThreadKey(ism_threadKey, (void *)tls);
    }
    return tls;
}


/*
 * Get the name of the process
 */
const char * ism_common_getProcessName(void) {
    return g_procname;
}


/*
 * Get the build version.
 * This might be zero length for unofficial builds
 */
const char * ism_common_getVersion(void) {
    return XSTR(ISM_VERSION);
}


/*
 * Get the build label.
 * This might be zero length for unofficial builds
 */
const char * ism_common_getBuildLabel(void) {
    return XSTR(BUILD_LABEL);
}

/* 
 * Get info to uniquely identify level of source used in the build.
 * For example a git commit hash or tag.
 *
 * May be 0-length if IMA_SOURCELEVEL_INFO env var not set during
 * build.
 */
const char * ism_common_getSourceLevel(void) {
    return XSTR(IMA_SOURCELEVEL_INFO);
}

/*
 * Get the date and time of the build.
 * This date and time are local to where the build was done and do not contain a timezone.
 * This is the time the component is built an can vary between components of the same build.
 * This is set for all builds.
 */
const char * ism_common_getBuildTime(void) {
    return XSTR(ISMDATE) " " XSTR(ISMTIME);
}


/*
 * Get the configuration property object
 * @return the configuration properties object
 */
ism_prop_t * ism_common_getConfigProperties(void) {
    return ism_g_config_props;
}

/*
 * Trace static global variables
 */
static FILE * trcfile = NULL;
static char * trcFileName = NULL;
ism_domain_t  ism_defaultDomain = {"DOM", 0, "Default" };
ism_trclevel_t * ism_defaultTrace = &ism_defaultDomain.trace;
int           ismTraceLevel = 6;    /* Deprecated, do not use */
int           ismTraceBackup = 1;
char *        ismTraceBackupDestination = NULL;
int           ismTraceBackupCount = MAX_TRACE_FILES;
volatile uint64_t  g_totalNonFatalFFDCs = 0;                      ///< Overall count of nonfatal FFDCs

static int    trcFlush = 1;
static long   trcSize = -1;
static long   trcMax  = 20 * 1024 * 1024;
static int    trcMsgData = 0;
static int    trcOpt = TOPT_TIME | TOPT_THREAD | TOPT_WHERE;
static char * g_trc_clientaddr = NULL;
static char * g_trc_clientid = NULL;
static char * g_trc_endpoint = NULL;


/*
 * Reset trace routines so instead of using a trace module we use the builtin trace
 */
void ism_common_TraceModuleClear(void) {
    traceFunction        = ism_common_trace;
    traceDataFunction    = ism_common_traceData;
    setErrorFunction     = ism_common_setError_int;
    setErrorDataFunction = ism_common_setErrorData_int;
    traceModuleCfgUpdate = NULL;
}


/*
 * Load and initialize a trace module.
 * @param props  The server properties
 * @param errorBuffer A buffer to place an error string on failure
 * @param errorBufSize The size of the error buffer
 * @param trclevel  The trace level for the default domain.
 */
bool ism_common_loadTraceModule(ism_prop_t * props, char * errorBuffer, int errorBufSize, int * trclevel) {
    bool success = false;
    bool default_loc = false;

    const char *traceModuleLocation = ism_common_getStringProperty(props, TRACEMODULE_LOCATION_CFGITEM);

    if (   (traceModuleLocation == NULL)
        || (strcmp(traceModuleLocation, "0") == 0)
        || (strcmp(traceModuleLocation, "")  == 0)) {
        traceModuleLocation = TRACEMODULE_DEFAULT_LOCATION;
        default_loc = true;
    }

    dlerror();    /* clear any errors */

    void * traceModuleHandle = dlopen(traceModuleLocation, RTLD_NOW);

    if (traceModuleHandle != NULL) {
        ism_common_TraceInitModule_t initStub = dlsym(traceModuleHandle, ISM_COMMON_TRACEINIT_FUNC);

        if (!initStub)  {
            snprintf(errorBuffer, errorBufSize, "ism_initTraceModule: %s", dlerror());
            dlclose(traceModuleHandle);
            goto mod_exit;
        }

        ism_common_TraceFunction_t traceStub = dlsym(traceModuleHandle, ISM_COMMON_TRACE_FUNC);

        if (!traceStub)  {
            snprintf(errorBuffer, errorBufSize, "ism_insertTrace: %s", dlerror());
            dlclose(traceModuleHandle);
            goto mod_exit;
        }

        ism_common_TraceDataFunction_t traceDataStub = dlsym(traceModuleHandle, ISM_COMMON_TRACEDATA_FUNC);

        if (!traceDataStub)  {
            snprintf(errorBuffer, errorBufSize, "ism_insertTraceData: %s", dlerror());
            dlclose(traceModuleHandle);
            goto mod_exit;
        }

        ism_common_SetErrorFunction_t setErrorStub = dlsym(traceModuleHandle, ISM_COMMON_SETERROR_FUNC);

        if (!setErrorStub) {
            /* Optional function... not an error if it's not there just use the default */
            setErrorStub = ism_common_setError_int;
        }

        ism_common_SetErrorDataFunction_t setErrorDataStub = dlsym(traceModuleHandle, ISM_COMMON_SETERRORDATA_FUNC);

        if (!setErrorDataStub) {
            /* Optional function... not an error if it's not there just use the default */
            setErrorDataStub = ism_common_setErrorData_int;
        }

        ism_common_TraceModuleCfgUpdate_t cfgUpdateStub = dlsym(traceModuleHandle, ISM_COMMON_TRACEMODULECFGUPDATE_FUNC);
        /* Optional function... not an error if it's not there, we just don't call a function */


        /*
         * Found all the functions so call init.
         */
        int rc = initStub(props, errorBuffer, errorBufSize, trclevel);

        if (rc == 0) {
            traceFunction        = traceStub;
            traceDataFunction    = traceDataStub;
            setErrorFunction     = setErrorStub;
            setErrorDataFunction = setErrorDataStub;
            traceModuleCfgUpdate = cfgUpdateStub;
            success = true;
        } else {
            /* Init function will have filled in error string... */
            dlclose(traceModuleHandle);
        }
    } else {
        /*
         * We don't fill in error string in this case (get shell acccess and use strace to investigate if necessary)
         * as this is the "normal" case - there was no valid trace module
         */
    }

mod_exit:
    if (!success) {
        /* If we didn't successfully setup up a module, ensure we don't have pointers to an old one */
        ism_common_TraceModuleClear();
    }

    return (success || default_loc);       /* BEAM suppression: file leak */
}

int g_traceInited = 0;

/*
 * Initialize the trace.
 * This is the initial configuration of trace when only the static config file is available.
 * Much of the trace information will be modified later during dynnamic config but we need
 * to have some trace available during this period.
 */
void ism_common_initTrace(void) {
    const char * trcname = ism_common_getStringConfig("TraceFile");
    const char * trclvl  = ism_common_getStringConfig("TraceLevel");
    const char * trcsel  = ism_common_getStringConfig("TraceSelected");
    const char * trcopt  = ism_common_getStringConfig("TraceOptions");
    const char * trcmsgdata = ism_common_getStringConfig("TraceMessageData");
    const char * trcfilter  = ism_common_getStringConfig("TraceFilter");
    int   tmptrclvl;
    char traceModuleErrorBuffer[256] = {0};
    bool traceModuleLoaded = true;


    if (g_traceInited)
        return;
    g_traceInited = 1;

    /* Set trace options */
    if (trcopt)
        ism_common_setTraceOptions(trcopt);

    /* Set trace destination */
    if (trcname)
        ism_common_setTraceFile(trcname, trcOpt&TOPT_APPEND);

    /* Set the trace level */
    if (trclvl == NULL)
        trclvl = "7";
    ism_common_setTraceLevelX(ism_defaultTrace, trclvl);
    if (trcsel == NULL)
        trcsel = "7";
    ism_common_setTraceLevelX(&ism_defaultDomain.selected, trcsel);


    /* Set the maximum trace file size to very big as the config value is used later */
    trcMax = 2000*1024*1024;

    /*
     * Load a trace module if there is one
     */
    tmptrclvl = ism_common_getTraceLevel();   /* Note this is deprecated */
    traceModuleLoaded = ism_common_loadTraceModule( ism_common_getConfigProperties()
                                                        , traceModuleErrorBuffer
                                                        , sizeof traceModuleErrorBuffer
                                                        , &tmptrclvl);
    /* TODO: fix this as it uses deprecated function */
    if (tmptrclvl != ism_common_getTraceLevel())
        ism_common_setTraceLevel(tmptrclvl);

    if (trcmsgdata) {
        int val = strtoul(trcmsgdata, NULL, 0);
        if (val < 0)
            val = 0;
        trcMsgData = val;
    }

    /* Put out an initial trace entry */
    if (trclvl && trcname) {
        TRACE(2, "============ Initialize trace ============\n");
        trcFlush = 0;
    }

    /* Set any trace filter in static config */
    /* TODO: remove this */
    if (trcfilter)
        ism_common_setTraceFilter(trcfilter);

    if (!traceModuleLoaded) {
        TRACE(2, "Load of trace module failed: %s\n", traceModuleErrorBuffer);
    }
}


/*
 * Loads the trace module in response to changes in dynamic config
 */
void ism_common_TraceModuleDynamicUpdate(ism_prop_t * props) {
    char traceModuleErrorBuffer[256] = {0};;
    int localtrclevel = ism_common_getTraceLevel();    /* uses deprecated method */

    bool traceModuleLoaded = ism_common_loadTraceModule( props
                                                   , traceModuleErrorBuffer
                                                   , sizeof traceModuleErrorBuffer
                                                   , &localtrclevel);

    if (traceModuleLoaded) {
        /* TODO: Uses deprecated method */
        if (localtrclevel != ism_common_getTraceLevel())
            ism_common_setTraceLevel(localtrclevel);
        TRACE(2, "============ Initialized trace module============\n");
    } else {
        TRACE(2, "Load of trace module failed: %s\n", traceModuleErrorBuffer);
    }
}


/*
 * If the trace module wants to know about changes in config, this
 * function notifies it.
 */
void ism_common_TraceModuleConfigUpdate(ism_prop_t * props) {
    /* If the trace module has a props changed function then call it now */
    ism_common_TraceModuleCfgUpdate_t cfgUpdate = traceModuleCfgUpdate;

    if (cfgUpdate) {
        cfgUpdate(props);
    }
}

///////////////////////////////////////////////////////////////////////////////
///  @brief
///    This function is used to write a block of data about a failure to
///    the trace file.
///
///  @param[in] function        - Function name
///  @param[in] seqId           - Unique Id within function
///  @param[in] filename        - Filename
///  @param[in] lineNo          - line number
///  @param[in] label           - Text description of failure
///  @param[in] retcode         - Associated return code
///  @param[in] ...             - A variable number of arguments (grouped in
///                               sets of 3 terminated with a NULL)
///         1)  ptr (void *)    - Pointer to data to dump
///         2)  length (size_t) - Size of data
///         3)  label           - Text label of data
///
///////////////////////////////////////////////////////////////////////////////
void ism_common_ffdc( const char *function
              , uint32_t seqId
              , bool core
              , const char *filename
              , uint32_t lineNo
              , char *label
              , uint32_t retcode
              , ... )
{
    va_list ap;
    void *ptr;
    size_t length;
    char *datalabel;
    char retcodeName[64];
    char retcodeText[128];

    // If we are going to shutdown put a big obvious error into the trace otherwise be a little less scary
    if(core)
    {
        LOG( ERROR, Messaging, 3030, NULL, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        LOG( ERROR, Messaging, 3031, "%s%d", "!! Probe:    {0}:{1}", function, seqId);
        LOG( ERROR, Messaging, 3032, "%s%d", "!! Location: {0}:{1}", filename, lineNo);
        LOG( ERROR, Messaging, 3033, "%s", "!! Label:    {0}", label);
    }
    else
    {
        LOG( ERROR, Messaging,3035, "%s%d%s", "Error location: {0}({1}) : {2}", filename,lineNo,label);
    }

    if (retcode != 0)
    {
        ism_common_getErrorName(retcode, retcodeName, 64);
        ism_common_getErrorString(retcode, retcodeText, 128);
        LOG( ERROR, Messaging, 3034, "%s%d%s", "!! Retcode:  {0} ({1}) - {2}", retcodeName, retcode, retcodeText);
    }

    if (TRACE_FFDC <= TRACE_DOMAIN->trcComponentLevels[TRACECOMP_XSTR(TRACE_COMP)])
    {
        va_start(ap, retcode);

        do
        {
            datalabel=va_arg(ap, void *);
            if (datalabel != NULL)
            {
                ptr=va_arg(ap, void *);
                length=va_arg(ap, size_t);

                if (ptr == NULL) length = 0; // If a NULL pointer was provided
                traceDataFunction(datalabel, 0, filename, lineNo, ptr, length, length);
            }
        } while (datalabel != NULL);

        va_end(ap);
    }

    if (!core)
    {
        // Write a message to the log with details of the failure
        LOG( ERROR, Messaging, 3004, "%s%d",
             "A non-fatal failure has occurred at location {0}:{1}. The failure recording routine has been called.",
             function, seqId );

        __sync_add_and_fetch(&g_totalNonFatalFFDCs , 1);
    }
    else
    {
        #if 0
        // Mechanism to write a file when a core is being requested to allow a watching process to stop
        // looping -- see the change set that introduced this change for more information.
        FILE *fp = fopen("doneIt.failed", "w");
        fprintf(fp, "FAILED in %s probe %u\n", function, seqId);
        fclose(fp);
        #endif


        // If this error is terminal then bring down the server
        LOG( ERROR, Messaging, 3005, "%s%d",
             "An unrecoverable failure has occurred at location {0}:{1}. The failure recording routine has been called. The server will now stop and restart.",
             function, seqId );

        ism_common_shutdown(true);
    }

    return;
}

/*
 * Get the number of non fatal FFDCs that have occurred
 * Used to check that the memory monitoring is healthy and
 * not leaking memory.
 *
 * Returned along with the memory statistics in the proxy and
 * the gateway memory statistics
 */
uint64_t ism_common_get_ffdc_count(void){
    return g_totalNonFatalFFDCs;
}

/*
 * Get the domain object by ID.
 * @param id   The ID of the domain
 * @return a domain object or NULL to indicate the ID is not a valid domain ID.
 */
ism_domain_t * ism_common_getDomainID(uint32_t id) {
    if (id == 0)
        return &ism_defaultDomain;
    /* TODO: Implement when domains exist */
    return NULL;
}

/*
 * Get the domain object by name.
 * @param name The name of the domain
 * @return a domain object or NULL to indicate the name is not known.
 */
ism_domain_t * ism_common_getDomain(const char * name) {
    if (!name || !*name || !strcmp(name, "Default"))
        return &ism_defaultDomain;
    /* TODO: Implement when domains exist */
    return NULL;
}


/*
 * Return the value of a hex digit or -1 if not a valid hex digit
 */
static int hexValue(char ch) {
    if (ch <= '9' && ch >= '0')
        return ch-'0';
    if (ch >='A' && ch <= 'F')
        return ch-'A'+10;
    if (ch >='a' && ch <= 'f')
        return ch-'a'+10;
    return -1;
}

/*
 * Converts the thread affinity from 64 bit number to 64 bytes array
 * @param char - affinity as a string
 * @param char[1024] - affinity as byte array
 * @return index of last non-zero byte in the array
 */
int ism_common_parseThreadAffinity(const char *affStr, char affMap[CPU_SETSIZE]) {
    int len;
    int result = 0;
    const char * ptr;
    if (!affStr)
        return 0;
    if ((affStr[0] == '0') && ((affStr[1] == 'x') || (affStr[1] == 'X')))
        affStr += 2;
    len = strlen(affStr);
    if (len == 0)
        return 0;
    ptr = affStr + (len - 1);

    memset(affMap, 0, CPU_SETSIZE);

    do {
        int v = hexValue(*ptr);
        if(result < CPU_SETSIZE) {
            if(v & 1)
                affMap[result] = 1;
        }
        if(result + 1 < CPU_SETSIZE) {
            if(v & 2)
                affMap[result+1] = 1;
        }
        if(result + 2 < CPU_SETSIZE) {
            if(v & 4)
                affMap[result+2] = 1;
        }
        if(result + 3 < CPU_SETSIZE) {
            if(v & 8)
                affMap[result+3] = 1;
        }
        ptr--;
        result += 4;
    } while(ptr >= affStr && result < CPU_SETSIZE);
    return result;
}


/*
 * Use the logcategory.h header file to produce the name to id category mapping
 */
#define TRACECOMP_SPECIAL
int ism_common_getTraceComponentID(const char * component) {
    if (component==NULL || !strcmp(component, "."))
        return -1;
#undef  TRACECOMP_
#define TRACECOMP_(name, val) else if (!strcmpi(component, #name)) return TRACECOMP_##name;
#include <tracecomponent.h>
    return -1;
}

/*
 * Use the logcategory.h header file to produce the name to id category name mapping
 */
#define TRACECOMP_SPECIAL
const char * ism_common_getTraceComponentName(int id) {
    if (id == -1)
        return "unknown";
#undef  TRACECOMP_
#define TRACECOMP_(name, val) else if (id == val) return #name;
#include <tracecomponent.h>
    return "unknown";
}

/*
 * Return the max size of message data to trace
 */
int ism_common_getTraceMsgData(void) {
    return trcMsgData;
}

/*
 * Set the max size of message data to trace
 */
void ism_common_setTraceMsgData(int msgdata) {
    if (msgdata < 0)
        msgdata = 0;
    trcMsgData = msgdata;
}

/*
 * Set the trace options
 */
int ism_common_setTraceFilter(const char * str) {
    char * ostr;
    int  len = 0;
    char * token;
    const char * comptoken = NULL;
    char * compvaluetoken = NULL;
    char * eos;
    int    complevel = -1;
    int    compID;
    int    rc = 0;
    int    opt = trcOpt;
    int    tracemax = 0;

    if (str)
        len = (int)strlen(str);
    if (len) {
        ostr = alloca(len+1);
        strcpy(ostr, str);
        token = (char *)ism_common_getToken(ostr, " ,\t\n\r", " ,\t\n\r", &ostr);
        while (token) {
            comptoken = (char *)ism_common_getToken(token, " \t=:", " \t=:", &compvaluetoken);
            if (compvaluetoken && *compvaluetoken) {
                /* Check for tokens that accept non-integer values */
                if (!strcmpi(comptoken, "tracemax")) {
                    tracemax = ism_common_getBuffSize(comptoken, compvaluetoken, "0");
                    if ((tracemax == 0) || (tracemax >= 2000 * 1024 * 1024)) {
                        rc = ISMRC_BadPropertyValue;
                        ism_common_setErrorData(rc, "%s%s", comptoken, compvaluetoken);
                        break;
                    } else {
                        ism_common_setTraceMax(tracemax);
                    }
                } else {
                    /* Integer value tokens  */
                    complevel = strtoul(compvaluetoken, &eos, 0);
                    if (!*eos) {
                        compID = ism_common_getTraceComponentID(comptoken);

                        if (compID >=0 && compID <TRACECOMP_MAX) {
                            if (complevel>=0 && complevel<=9) {
                                /* Valid level. Set the level */
                                ism_defaultTrace->trcComponentLevels[compID] = complevel;
                            } else {
                                rc = ISMRC_BadPropertyValue;
                                ism_common_setErrorData(rc, "%s%s", comptoken, compvaluetoken);
                                break;
                            }
                        } else {
                            if (!strcmpi(comptoken, "msgdata")) {
                                if (complevel < 0)
                                    complevel = 0;
                                trcMsgData = complevel;
                            } else if (!strcmpi(comptoken, "time")) {
                                if (complevel<0 || complevel>1) {
                                    rc = ISMRC_BadPropertyValue;
                                    ism_common_setErrorData(rc, "%s%s", comptoken, compvaluetoken);
                                    break;
                                }
                                if (complevel != 0)
                                    opt |= TOPT_TIME;
                                else
                                    opt &= ~TOPT_TIME;
                            } else if (!strcmpi(comptoken, "where")) {
                                if (complevel<0 || complevel>1) {
                                    rc = ISMRC_BadPropertyValue;
                                    ism_common_setErrorData(rc, "%s%s", comptoken, compvaluetoken);
                                    break;
                                }
                                if (complevel != 0)
                                    opt |= TOPT_WHERE;
                                else
                                    opt &= ~TOPT_WHERE;
                            } else if (!strcmpi(comptoken, "thread")) {
                                if (complevel<0 || complevel>1) {
                                    rc = ISMRC_BadPropertyValue;
                                    ism_common_setErrorData(rc, "%s%s", comptoken, compvaluetoken);
                                    break;
                                }
                                if (complevel != 0)
                                    opt |= TOPT_THREAD;
                                else
                                    opt &= ~TOPT_THREAD;
                            } else {
                                rc = ISMRC_BadPropertyName;
                                ism_common_setErrorData(rc, "%s", comptoken);
                                break;
                            }
                        }
                    } else {
                        rc = ISMRC_BadPropertyValue;
                        ism_common_setErrorData(rc, "%s%s", comptoken, compvaluetoken);
                        break;
                    }
                }
            } else {
                rc = ISMRC_BadPropertyName;
                ism_common_setErrorData(rc, "%s", comptoken);
                break;
            }
            token = ism_common_getToken(ostr, " ,\t\n\r", " ,\t\n\r", &ostr);
        }
    }
    if (rc == 0) {
        pthread_mutex_lock(&trc_lock);
        trcOpt = opt;
        pthread_mutex_unlock(&trc_lock);
    }
    return rc;
}


/*
 * Set the trace max size.
 * @param tracemax   The maximum size to set.  This must be greater than zero.
 */
void ism_common_setTraceMax(uint64_t tracemax) {
    if (tracemax > 0) {
        if (tracemax < 250000)
            tracemax = 250000;       /* Set min size */
        trcMax = (long)tracemax;
    }

    TRACE(5, "Set the maximum trace size to: %ld\n", trcMax);
}

/*
 * Set the trace level to show.
 * Any trace entries at or the specified level or lower will be shown.
 * <p>
 * Normally the trace level is set up using the TraceLevel configuration value.
 *
 * @param level A level from 0 to 9.  A value of 0 disables all trace.
 * @return A return code: 0=good
 */
int ism_common_setTraceLevel(int level) {
    char clevel [2];
    if (level >= 0 && level <= 9) {
        clevel[0] = level+'0';
        clevel[1] = 0;
        ism_common_setTraceLevelX(ism_defaultTrace, clevel);
        return 0;
    }
    return 1;
}

/*
 * Set extended trace level.
 */
int ism_common_setTraceLevelX(ism_trclevel_t * trcl, const char * lvl) {
    int  count;
    int  level = 0;
    char * lp;
    char * eos;
    char * token;
    char * comptoken;
    char * compvaluetoken;
    int    complevel;
    int    compID;
    int    rc = 0;

    if (!lvl)
        return 1;
    lp = alloca(strlen(lvl)+1);
    strcpy(lp, lvl);

    token = ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
    if (!token)
        token = "5";
    level = strtoul(token, &eos, 0);
    if (*eos) {
        rc = ISMRC_BadPropertyValue;
        ism_common_setErrorData(rc, "%s%s", "TraceLevel", token);
        return rc;
    }
    trcl->trcLevel = level;

    /* Set the default level for all */
    for (count=0; count<TRACECOMP_MAX; count++) {
        trcl->trcComponentLevels[count] = level;
    }

    token = (char *)ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
    while (token) {
        comptoken = (char *)ism_common_getToken(token, " \t=:", " \t=:", &compvaluetoken);
        if (compvaluetoken && *compvaluetoken) {
            /* Check for tokens that accept non-integer values */
            complevel = strtoul(compvaluetoken, &eos, 0);
            if (!*eos) {
                compID = ism_common_getTraceComponentID(comptoken);

                if (compID >=0 && compID <TRACECOMP_MAX) {
                    if (complevel>=0 && complevel<=9) {
                        /* Valid level. Set the level */
                        trcl->trcComponentLevels[compID] = complevel;
                    } else {
                        rc = ISMRC_BadPropertyValue;
                        ism_common_setErrorData(rc, "%s%s", comptoken, compvaluetoken);
                        break;
                    }
                } else {
                    rc = ISMRC_BadPropertyValue;
                    ism_common_setErrorData(rc, "%s%s", comptoken, compvaluetoken);
                    break;
                }
            }
        } else {
            rc = ISMRC_BadPropertyName;
            ism_common_setErrorData(rc, "%s", comptoken);
            break;
        }
        token = ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
    }
    return rc;
}

/*
 * Set trace connection configuration
 */
int ism_common_setTraceConnection(const char * trcconn) {
    char * lp;
    char * token;
    char * comptoken;
    char * compvaluetoken;
    int    rc = 0;
    const char * trc_endpoint = NULL;
    const char * trc_clientid = NULL;
    const char * trc_clientaddr = NULL;

    if (!trcconn)
        return 1;
    lp = alloca(strlen(trcconn)+1);
    strcpy(lp, trcconn);

    token = (char *)ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
    while (token) {
        comptoken = (char *)ism_common_getToken(token, " \t=:", " \t=:", &compvaluetoken);
        if (!comptoken)
            comptoken = "";
        if (compvaluetoken) {
            if (*compvaluetoken==0)
                compvaluetoken = NULL;
            if (!strcmpi(comptoken, "endpoint")) {
                trc_endpoint = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),compvaluetoken);
            } else if (!strcmpi(comptoken, "clientid")) {
                trc_clientid = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),compvaluetoken);
            } else if (!strcmpi(comptoken, "clientaddr")) {
                trc_clientaddr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),compvaluetoken);
            } else {
                rc = ISMRC_BadPropertyName;
                ism_common_setErrorData(rc, "%s", comptoken);
                break;
            }
        } else {
            rc = ISMRC_BadPropertyValue;
            ism_common_setErrorData(rc, "%s%s", comptoken, "");
            break;
        }
        token = ism_common_getToken(lp, " ,\t\n\r", " ,\t\n\r", &lp);
    }
    if (rc == 0) {
        pthread_mutex_lock(&trc_lock);
        if (g_trc_endpoint)
            ism_common_free(ism_memory_utils_misc,g_trc_endpoint);
        g_trc_endpoint = (char *)trc_endpoint;
        if (g_trc_clientid)
            ism_common_free(ism_memory_utils_misc,g_trc_clientid);
        g_trc_clientid = (char *)trc_clientid;
        if (g_trc_clientaddr)
            ism_common_free(ism_memory_utils_misc,g_trc_clientaddr);
        g_trc_clientaddr = (char *)trc_clientaddr;
        pthread_mutex_unlock(&trc_lock);
    }
    return rc;
}

/*
 * Trace connection select by clientID
 */
int  ism_common_traceSelectClientID(const char * clientID) {
    int  rc = 0;

    if (g_trc_clientid == NULL || clientID == NULL)
        return 0;

    pthread_mutex_lock(&trc_lock);
    rc = ism_common_match(clientID, g_trc_clientid);
    pthread_mutex_unlock(&trc_lock);
    if (rc)
        TRACE(8, "ClientID %s selected.\n", clientID);
    return rc;
}

/*
 * Trace connection select by clientID
 */
int  ism_common_traceSelectClientAddr(const char * clientaddr) {
    int  rc = 0;

    if (g_trc_clientaddr == NULL || clientaddr == NULL)
        return 0;

    pthread_mutex_lock(&trc_lock);
    rc = ism_common_match(clientaddr, g_trc_clientaddr);
    pthread_mutex_unlock(&trc_lock);
    if (rc)
        TRACE(8, "ClientAddr %s selected.\n", clientaddr);
    return rc;
}

/*
 * Trace connection select by endpoint
 */
int  ism_common_traceSelectEndpoint(const char * endpoint) {
    int  rc = 0;
    if (g_trc_endpoint == NULL || endpoint == NULL)
        return 0;

    pthread_mutex_lock(&trc_lock);
    rc = ism_common_match(endpoint, g_trc_endpoint);
    pthread_mutex_unlock(&trc_lock);
    if (rc)
        TRACE(8, "Endpoint %s selected.\n", endpoint);
    return rc;
}

/*
 * Get the trace level.
 * *** Deprecated ***
 * This method is deprecated as it does not handle component level trace, selected trace, or
 * domain trace.
 *
 * Any trace entries at or the specified level or lower will be shown.
 * @return The current trace level 0 to 9.  A value of 0 means that trace is disabled.
 */
int ism_common_getTraceLevel(void) {
    return ism_defaultTrace->trcLevel;
}


/*
 * Set the trace file name.
 * Close any existing trace file and start writing to the specified file.
 * <p>
 * Normally the trace file is set up using the TraceFile configuration value.
 *
 * @param  The trace file name
 * @param  append If non-zero, append to an existing file if is exists
 * @return A return code: 0=good
 */
int ism_common_setTraceFile(const char * file, int append) {
    mode_t curMode;

   pthread_mutex_lock(&trc_lock);
    if (trcfile && trcfile != stdout && trcfile != stderr) {
        FILE * cfile = trcfile;
        trcfile = NULL;
        fclose(cfile);
    }

    curMode = umask(0011);  // New trace files can be read and written by everyone

    trcFileName = (char *)file;
    if (!strcmp(file, "stdout")) {
        trcfile = stdout;
        trcFlush = 1;
        trcSize = -1;
    } else if (!strcmp(file, "stderr")) {
        trcfile = stderr;
        trcFlush = 1;
        trcSize = -1;
    } else if (!file) {
        return 0;
    } else {
        trcfile = fopen(file, append? "ab" : "wb");
        if (trcfile) {
            trcFlush = 0;
            fseek(trcfile, 0, SEEK_END);
            trcSize = ftell(trcfile);
        } else {
            trcfile = stderr;
            trcFlush = 1;
            trcSize = -1;
        }
    }

    umask(curMode);

    pthread_mutex_unlock(&trc_lock);

    return trcfile == NULL;
}

/*
 * Set the trace backup mode.
 * 0 - single backup file with overwrite
 * 1 - n compressed backup files with timestamps
 * 2 - offloading n compressed backup files with timestamps
 *
 * @param traceBackup A mode from 0 to 2.
 */
void ism_common_setTraceBackup(int traceBackup) {
    pthread_mutex_lock(&trc_lock);
    ismTraceBackup = traceBackup;
    pthread_mutex_unlock(&trc_lock);
}

/*
 * Get the trace backup mode. If trace worker thread is not running,
 * always use mode 0.
 * @return  0 - single backup file with overwrite
 *      1 - n compressed backup files with timestamps
 *      2 - offloading n compressed backup files with timestamps
 */
int ism_common_getTraceBackup(void) {
    return (ism_trace_work_table == NULL)?0:ismTraceBackup;
}

/*
 * Set the trace backup destination (ignored for trace backup mode != 2).
 * @param destination - A valid ftp or scp destination.
 */
void ism_common_setTraceBackupDestination(const char * destination) {
    pthread_mutex_lock(&trc_lock);
    if (ismTraceBackupDestination) {
        ism_common_free(ism_memory_utils_misc,(char *)ismTraceBackupDestination);
    }
    ismTraceBackupDestination = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),destination);
    pthread_mutex_unlock(&trc_lock);
}

/*
 * Get the current trace backup destination.
 * @return A newly allocated buffer on heap that contains trace backup destination.
 */
char * ism_common_getTraceBackupDestination(void) {
    pthread_mutex_lock(&trc_lock);
    char *res = NULL;
    if (ismTraceBackupDestination) {
        res = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),ismTraceBackupDestination);
    }
    pthread_mutex_unlock(&trc_lock);

    return res;
}

/*
 * Set the max number of trace backup files.
 *
 * @param traceBackup A mode from 0 to 2.
 */
void ism_common_setTraceBackupCount(int count) {
    pthread_mutex_lock(&trc_lock);
    ismTraceBackupCount = count;
    pthread_mutex_unlock(&trc_lock);
}

/*
 * Get the max number of trace file backups (for modes 1 and 2).
 * @return  The max count of trace backup files.
 */
int ism_common_getTraceBackupCount(void) {
    pthread_mutex_lock(&trc_lock);
    int res = ismTraceBackupCount;
    pthread_mutex_unlock(&trc_lock);
    return res;
}

/*
 * Periodic flush of the trace file
 */
int ism_common_flushTrace(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    pthread_mutex_lock(&trc_lock);
    fflush(trcfile);
    pthread_mutex_unlock(&trc_lock);
    return 1;
}

/*
 * Set the flush time for the trace.
 * This can be called only once and must be after timers are initialized.
 */
int ism_common_traceFlush(int millis) {
    static int flush_active = 0;
    if (millis <= 0)
        return 0;
    if (millis < 100)
        millis = 100;
    if (!flush_active) {
        flush_active = 1;
        ism_common_setTimerRate(ISM_TIMER_LOW, ism_common_flushTrace, NULL, 150, millis, TS_MILLISECONDS);
    }
    return 0;
}


/*
 * Set the trace options
 */
XAPI int ism_common_setTraceOptions(const char * str) {
    int opt = 0;
    char * ostr;
    int  len = 0;
    const char * token;
    int  rc = 0;

    if (str)
        len = (int)strlen(str);
    if (len) {
        ostr = alloca(len+1);
        strcpy(ostr, str);
        token = ism_common_getToken(ostr, " ,\t\n\r", " ,\t\n\r", &ostr);
        while (token) {
            if (!strcmpi(token, "time")) {
                opt |= TOPT_TIME;
            } else if (!strcmpi(token, "thread")) {
                opt |= TOPT_THREAD;
            } else if (!strcmpi(token, "where")) {
                opt |= TOPT_WHERE;
            } else if (!strcmpi(token, "append")) {
                opt |= TOPT_APPEND;
            } else {
                TRACE(2, "The trace options are not valid: %s\n", str);
                rc = ISMRC_BadPropertyValue;
                ism_common_setErrorData(rc, "%s%s", "TraceOptions", str);
            }
            token = ism_common_getToken(ostr, " ,\t\n\r", " ,\t\n\r", &ostr);
        }
    }
    trcOpt = opt;
    return rc;
}

/*
 * Set trace append
 */
int ism_common_setTraceAppend(int append) {
    if (append)
        trcOpt |= TOPT_APPEND;
    else
        trcOpt &= ~TOPT_APPEND;
    return 0;
}


/*
 * Internal log file rotation
 */
int  ism_common_rotateTraceFileInternal(char ** backupTrace) {
    char *   newname;
    char *   dotpos;
    int      rc;
    int      fd = fileno(trcfile);
    char     datetime[100];
    struct stat buf;
    int      dtLen;

    int rotationStrategy = ism_common_getTraceBackup();

    if ((rotationStrategy != 0) && (fstat(fd, &buf) == 0)) {
      struct tm * cTime = localtime(&buf.st_ctime);
      dtLen = snprintf(datetime, sizeof(datetime),
          "_%04d%02d%02d_%02d%02d%02d",
          cTime->tm_year + 1900, cTime->tm_mon + 1, cTime->tm_mday,
          cTime->tm_hour, cTime->tm_min, cTime->tm_sec);
    } else {
      dtLen = snprintf(datetime, sizeof(datetime), "_prev");
    }

    newname = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,180),strlen(trcFileName) + dtLen + 1);
    *newname = 0;
    dotpos = strrchr(trcFileName, '.');

    if (dotpos) {
        int len = (int)(dotpos - trcFileName);
        if (len) {
            memcpy(newname, trcFileName, len);
            newname[len] = 0;
        }
        strcat(newname, datetime);
        strcat(newname, dotpos);
    } else {
        strcpy(newname, trcFileName);
        strcat(newname, datetime);
    }


    /*
     * Close and rename the file
     */
    fflush(trcfile);
    fclose(trcfile);

    unlink(newname);

    int stillNeedNewName = 0;
    rc = rename(trcFileName, newname);
    if (rc) {
        rc = errno;
    } else if (backupTrace && (rotationStrategy != 0)) {
        *backupTrace = newname;
        stillNeedNewName = 1;
    }

    trcfile = fopen(trcFileName, "wb");
    if (trcfile) {
        trcFlush = 0;
        trcSize = 0;
    } else {
        fprintf(stderr, "Unable to open trace file: file=%s errno=%u error=%s\n", trcFileName, errno, strerror(errno));
        trcfile = stderr;
        trcFlush = 1;
        trcSize = -1;
    }
    if (rc) {
        fprintf(trcfile, "Unable to rename trace file: from=%s to=%s errno=%d error=%s\n", trcFileName, newname, rc, strerror(rc));
    } else {
        fprintf(trcfile, "New trace file: %s\n", trcFileName);
    }

    if (!stillNeedNewName) {
      ism_common_free(ism_memory_utils_misc,newname);
    }

    return rc;
}


/*
 * Trace a signal and flush the trace buffer.
 * This is called in a signal handler when shutting down
 * We should do as little as possible here to preserve the core environment.
 */
void ism_common_traceSignal(int signal) {
    /*
     * If the trace lock is not held, print to the trace file and flush it.
     * If the trace lock is held, it is not safe to do this in a signal handler.
     */
    if (pthread_mutex_trylock(&trc_lock) == 0) {
        // fprintf(trcfile, "Signal received: %s (%u)\n", strsignal(signal), signal);
        fflush(trcfile);
        pthread_mutex_unlock(&trc_lock);
    }
}


/*
 * Reset the timestamp used for trace
 */
void ism_common_resetTrcTimestamp(void) {
    /* Do nothing for now.  We will reflect the timezone setting at the next hour crossing */
}

/*
 * Return the thread timestamp object with the current time set
 *
 * This can be used to format the current time.  This timestamp must only be used
 * in the current thread and only before a trace entry.
 *
 * This handles the issue of time zones changing at hour boundaries (summer, winter).
 *
 * This code updates the trace timestamp to the current time and returns it.  It is
 * optimized for sequential calls for a monitonically increasing time.
 */
ism_ts_t * ism_common_getThreadTimestamp(void) {
    ism_tls_t * tls = (ism_tls_t *) ism_common_getThreadKey(ism_threadKey, NULL);
    if (__builtin_expect(!tls, 0))                 /* Should not happen for real threads */
        tls = makeTLS(512, NULL);

    ism_time_t now = ism_common_currentTimeNanos();
    if ((now-tls->tz_set_time) < NANOS_IN_HOUR){
        ism_common_setTimestamp(tls->trc_ts, now);
    } else {
        ism_ts_t * oldTS = tls->trc_ts;
        tls->tz_set_time = now - (now % NANOS_IN_HOUR);
        tls->trc_ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
        ism_common_closeTimestamp(oldTS);
    }
    return tls->trc_ts;
}

/*
 * Create a trace entry
 */
void ism_common_trace(int level, int opt, const char * file, int lineno, const char * fmt, ...) {
    va_list  arglist;
    char  buf[4096];
    char * bufp;
    const char * fp;
    char * backupTrace = NULL;
    int   outlen;
    int   inlen  = 0;
    ism_tls_t * tls = (ism_tls_t *) ism_common_getThreadKey(ism_threadKey, NULL);
    if (__builtin_expect(!tls, 0))                 /* Should not happen for real threads */
        tls = makeTLS(512, NULL);

    if (__builtin_expect((trcfile == NULL), 0)) {
        pthread_mutex_lock(&trc_lock);
        if (trcfile == NULL)
            trcfile = stdout;
        pthread_mutex_unlock(&trc_lock);
    }

    if (!(opt&TOPT_NOGLOBAL)) {
        opt |= trcOpt;
    } else {
        opt &= ~TOPT_NOGLOBAL;
    }

    bufp = buf;
    if (opt & TOPT_TIME) {
        /*
         * Check for crossing an hour.
         * It is possible if not very likely that the timezone will change either
         * at a DST boundary, or when the timezone is modified.  We check every hour
         * even though this is not very common.
         */
        ism_time_t now = ism_common_currentTimeNanos();
        if ((now-tls->tz_set_time) < NANOS_IN_HOUR){
            ism_common_setTimestamp(tls->trc_ts, now);
        } else {
            ism_ts_t * oldTS = tls->trc_ts;
            tls->tz_set_time = now - (now % NANOS_IN_HOUR);
            tls->trc_ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
            ism_common_closeTimestamp(oldTS);
        }
        ism_common_formatTimestamp(tls->trc_ts, buf, 64, 7, ISM_TFF_ISO8601_2);
        bufp += strlen(bufp);
    }
    if (opt & TOPT_THREAD) {
        if (bufp != buf)
            *bufp++ = ' ';
        memcpy(bufp, tls->tname, tls->tname_len);
        bufp += tls->tname_len;
    }
    if (opt & TOPT_WHERE) {
        if (bufp != buf)
            *bufp++ = ' ';
        if (file) {
            fp = (char *)file + strlen(file);
            while (fp > file && fp[-1] != '/' && fp[-1] != '\\')
                fp--;
        } else {
            fp = "";
        }
        bufp += sprintf(bufp, "%s:%u", fp, (uint32_t)lineno);
    }
    if (opt & TOPT_CALLSTK) {
        /* TODO: callstack */
    }
    if (opt) {
        *bufp++ = ':';
        *bufp++ = ' ';
        *bufp = 0;
    }
    inlen = bufp-buf;
    if (__builtin_expect((fmt!=NULL),1)) {
        va_start(arglist, fmt);
        outlen = vsnprintf(bufp, sizeof(buf)-inlen, fmt, arglist) + inlen;
        va_end(arglist);
        if (__builtin_expect((outlen >= sizeof(buf)),0)) {
            bufp = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,182),outlen + inlen + 2);
            if (bufp) {
                if (inlen) {
                    memcpy(bufp, buf, inlen);
                }
                va_start(arglist, fmt);
                vsprintf(bufp+inlen, fmt, arglist);
                va_end(arglist);
                outlen = (int)strlen(bufp);
            } else {
                bufp = buf;
            }
        } else {
            bufp = buf;
        }
        pthread_mutex_lock(&trc_lock);
        if (__builtin_expect((trcSize >= 0), 1)) {
            trcSize += outlen;
            if (__builtin_expect((trcSize > trcMax),0)) {
                /* Rotate trace file */
                ism_common_rotateTraceFileInternal(&backupTrace);
                /* Increment trace size after rotate */
                if (trcSize >= 0)
                    trcSize += outlen;
            }
        }
        fwrite(bufp, outlen, 1, trcfile);
        if (trcFlush || level<=2)
            fflush(trcfile);
        pthread_mutex_unlock(&trc_lock);
        if (__builtin_expect((bufp != buf),0))
            ism_common_free(ism_memory_utils_misc,bufp);

        if (backupTrace) {
          /* Add a work item to the list to deal with this file */
          ism_trace_work_entry_t * entry = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,184),1, sizeof(ism_trace_work_entry_t));
          entry->type = 0;
          entry->fileName = backupTrace;

          ism_trace_add_work(entry);

        }
    }
}


#ifndef _WIN32

/* ********************************************************************
 *
 * Thread functions
 *
 **********************************************************************/

typedef void * (* thred_arg)(void * data, void * context, int value);

static ism_thread_properties_t *ism_common_getThreadProperties(const char *name) {
    ism_thread_properties_t *props;
    char affMaskStr[CPU_SETSIZE] = {0};
    char affMap[CPU_SETSIZE] = {0};
    int affLen = 0;
    int priority;
    char cfgName[64];

    /* Pick up affinity map */
    affLen = ism_config_autotune_getaffinity(name, affMap);

    ism_common_affMaskToStr(affMap, affLen, affMaskStr);

    /* Pick up priority */
    sprintf(cfgName, "Priority.%s", name);
    const char *prioStr = ism_common_getStringConfig(cfgName);
    if (prioStr) {
        char * endptr = NULL;
        priority = strtol(prioStr,&endptr,10);
        if (*endptr != '\0') {
            priority = 0;
        }
    } else {
        priority = 0;
    }

    if (affLen || priority) {
    	props = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,185),sizeof(ism_thread_properties_t)+affLen);
        props->affLen = affLen;
        props->priority = priority;
        memcpy((char *)(props+1), affMap, affLen);
    } else {
        props = NULL;
    }

    TRACE(5, "Thread properties set %s: affinity=%s priority=%d prioStr=%s\n",
          name, affMaskStr, priority, prioStr);

    return props;
}

/*
 * Set pthread properties thread name
 */
static void ism_common_setThreadProperties(ism_threadh_t thread, ism_thread_properties_t *props) {
    if (props->affLen) {
        ism_common_setAffinity(thread, (char *)(props+1), props->affLen);
    }

    if (props->priority) {
        struct sched_param param;
        int policy;
        pthread_getschedparam((pthread_t)thread, &policy, &param);
        param.sched_priority = props->priority;
        int rc = pthread_setschedparam((pthread_t)thread, SCHED_RR, &param);
        if (rc) {
            TRACE(4, "Failed to set thread priority to %d: error=%d\n", props->priority, rc);
        }
    }
}

/*
 * Startup to set thread name
 */
static void * ism_run_thread(void * xarg) {
    void * result = NULL;
    ism_thread_parm_t * arg = (ism_thread_parm_t *)xarg;
    ism_tls_t * tls;
    ism_threadh_t self = ism_common_threadSelf();

    /* Set the thread name */
    prctl(PR_SET_NAME, (unsigned long)(uintptr_t)&arg->tname);

    /* Create and set the utils TLS */
    tls = calloc(1, sizeof(ism_tls_t)+512);
    tls->data_alloc = 512;
    ism_common_threaddata = tls;

    //TODO check this worked
    ism_common_initializeThreadMemUsage();

    /* Set properties such as CPU affinity and priority */
    if (arg->properties) {
        ism_common_setThreadProperties(self, arg->properties);
        // free must be done after initializing thread mem usage (note this means most
        // threads will start with a negative mem usage for memory utils)
        ism_common_free(ism_memory_utils_misc,arg->properties);
        arg->properties = NULL;
    }

    tls->trc_ts = ism_common_openTimestamp(ISM_TZF_LOCAL);
    strcpy(tls->tname, arg->tname);
    tls->tname_len = (int)strlen(tls->tname);
    ism_common_setThreadKey(ism_threadKey, (void *)tls);

/*----  start stuff for thread health monitoring  -----*/
    if ( i_am_the_main )
      ism_common_add_my_health(arg->tname);
/*----  stop  stuff for thread health monitoring  -----*/

    TRACE(5, "Thread starting: name=%s tid=%lu self=%p\n", arg->tname, syscall(__NR_gettid), (void *)self);

    /* Invoke the thread method */
    result = arg->method(arg->data, arg->context, arg->value);

    TRACE(5, "Thread exiting: name=%s tid=%lu self=%p\n", arg->tname, syscall(__NR_gettid), (void *)self);

    tls = ism_common_getThreadKey(ism_threadKey, NULL);

    /* Invoke cleanup, if ismc_threadCleanup was set by the thread */
    if (tls && tls->thread_cleanup) {
      tls->thread_cleanup(tls->cleanup_parm);
    }

    /* Free up and return */
/*----  start stuff for thread health monitoring  -----*/
    ism_common_del_my_health();
/*----  stop  stuff for thread health monitoring  -----*/
    //this is all done before the memusage is set up for the thread so
    // need to free the raw parms
    ism_common_free_raw(ism_memory_utils_misc,xarg);
    tls = ism_common_getThreadKey(ism_threadKey, NULL);
    if(tls) {
        ism_common_setThreadKey(ism_threadKey, NULL);
        ism_common_closeTimestamp(tls->trc_ts);
        ism_common_destroyThreadMemUsage();

        //this is all done before the memusage is set up for the thread so
        // need to free the raw tls
        ism_common_free_raw(ism_memory_utils_misc,tls);
    }
    ism_common_endThread(result);
    return result;
}

/*
 * Change thread to working mode
 */
XAPI void ism_common_going2work(void) {
    if ( threadHealth )
        threadHealth->shouldBeBackAt = shouldBeBackAt ;
}

/*
 * Change thread to idle mode
 */
XAPI void ism_common_backHome(void) {
    if ( threadHealth )
        threadHealth->shouldBeBackAt = 0e0 ;
}

XAPI int  ism_common_add_my_health(char *myName) {
  if ((threadHealth = (ism_tls_health_t *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,190),sizeof(ism_tls_health_t))) == NULL ) {
      TRACE(1, "Failed to allocate TLS data for thread health monitoring for thread %s\n",myName);
      return ISMRC_AllocateError;
  }
  memset(threadHealth,0,sizeof(ism_tls_health_t));
  threadHealth->myLWP = syscall(SYS_gettid);
  threadHealth->myId = pthread_self();
  if (!memcmp(myName,"tcp",3) )
    threadHealth->myCat = HC_IOP ;
  else if (!memcmp(myName,"Admin",5) )
    threadHealth->myCat = HC_ADM ;
  else if (!memcmp(myName,"Security",8) )
    threadHealth->myCat = HC_SEC ;
  else if (!memcmp(myName,"diskUtil",8) )
    threadHealth->myCat = HC_STO ;
  else if (!memcmp(myName,"ha",2) && memcmp(myName,"haSyncCh",8) )
    threadHealth->myCat = HC_STO ;
  else if (!memcmp(myName,"store",4) )
    threadHealth->myCat = HC_STO ;
  else if ( strstr(myName,"Persist") )
    threadHealth->myCat = HC_STO ;
  else
    threadHealth->myCat = HC_OTH ;
  threadHealth->shouldBeBackAt = -1e0 ;
  ism_common_strlcpy(threadHealth->myName, myName, sizeof(threadHealth->myName));
  pthread_mutex_lock(&g_utillock);
  threadHealth->next = healthHead ;
  healthHead = threadHealth ;
  tooLongTH = (double)ism_common_getIntConfig("ThreadMonitoringTimeout",300);
  shouldBeBackAt = ism_common_readTSC() + tooLongTH ;
  pthread_mutex_unlock(&g_utillock);
  return ISMRC_OK;
}

XAPI int  ism_common_del_my_health(void)
{
  if ( threadHealth )
  {
    ism_tls_health_t *p ;
    pthread_mutex_lock(&g_utillock);
    for ( p=healthHead ; p ; p=p->next )
    {
      if ( p == threadHealth )
      {
        healthHead = p->next ;
        break ;
      }
      if ( p->next == threadHealth )
      {
        p->next = threadHealth->next ;
        break ;
      }
    }
    pthread_mutex_unlock(&g_utillock);
    if ( p )
    {
      ism_common_free(ism_memory_utils_misc,threadHealth) ;
      threadHealth = NULL ;
      return ISMRC_OK;
    }
    else
    {
      TRACE(1, "Failed to free TLS data for thread health monitoring for thread %s\n",threadHealth->myName);
      return ISMRC_Error;
    }
  }
  else
    return ISMRC_OK;
}

XAPI int  ism_common_stack_trace(int tmp)
{
   int rc ;
   char gdbcmds_fname[PATH_MAX+1];
   char line[1024];
   FILE *fp;
   const char *outDir = NULL;

   int cmdsfname_bytesneeded = snprintf(gdbcmds_fname, sizeof(gdbcmds_fname), "%s/gdb_cmds",
                                        ism_common_getRuntimeTempDir() );

   if (cmdsfname_bytesneeded > sizeof(gdbcmds_fname)) {
      TRACE(1,"%s: gdbcmds filename too long!\n",__func__);
      return -1;
   }

   pthread_mutex_lock(&g_utillock);
   if ( (fp = fopen(gdbcmds_fname,"we")) )
   {
     char nm[1024];
     int stackfnamebytes=0;
     
     if (!tmp) {
        outDir = ism_common_getStringConfig("Store.DiskPersistPath");
     }
     if (!outDir ) {
        outDir = ism_common_getRuntimeTempDir();
     }
     stackfnamebytes =  snprintf(nm,sizeof(nm),"%s/ISM_health_stack_%3.3u",
                                       outDir,(stack_trace_cnt++)&15);

     if (stackfnamebytes >= sizeof(nm)) {
        TRACE(1,"%s: stacktrace filename too long!\n",__func__);
        pthread_mutex_unlock(&g_utillock);
        return -1;
     }

     unlink(nm);
     fprintf(fp,"set logging file %s\n",nm);
     fprintf(fp,"set logging overwrite on\n");
     fprintf(fp,"set logging redirect  on\n");
     fprintf(fp,"set logging on\n");
     fprintf(fp,"thread apply all backtrace\n");
     fprintf(fp,"quit");
     fclose(fp);
     int linebytes = snprintf(line,sizeof(line),"gdb -batch -x %s -p %u", gdbcmds_fname, getpid());
     
     if (linebytes > sizeof(line)) {
        TRACE(1,"%s: gdb command too long!\n",__func__);
        pthread_mutex_unlock(&g_utillock);
        return -1;
     }

     rc = system(line);
     if ( rc != -1 ) rc = WEXITSTATUS(rc);
     TRACE(1,"After executing %s with output to %s: rc=%d\n",line,nm,rc);
     if ( (fp = fopen(nm,"re")) )
     {
       while ( fgets(line,sizeof(line),fp) )
       {
         TRACE(1, "gdb_out: %s",line);
       }
       fclose(fp);
     }
   }
   else
   {
     TRACE(1,"Failed to open %s ; errno = %d\n", gdbcmds_fname,errno);
     rc = -1 ;
   }
   pthread_mutex_unlock(&g_utillock);
   return rc ;
}

XAPI int ism_common_check_health(void)
{
  ism_tls_health_t *p ;
  int count[HC_MAX][2], bad=0;
  double now = ism_common_readTSC() ;
  memset(count,0,sizeof(count));
  pthread_mutex_lock(&g_utillock);
  shouldBeBackAt = now + tooLongTH ;
  for ( p=healthHead ; p ; p=p->next )
  {
    if ( p->shouldBeBackAt <  0e0 || p->myCat == HC_OTH ) continue ;
    if ( p->shouldBeBackAt == 0e0 ) count[p->myCat][0]++ ;
    else
    if ( p->shouldBeBackAt <  now )
    {
      count[p->myCat][1]++ ;
      bad++ ;
      TRACE(3,"Warning: Thread %s (LWP=%u) is too late coming back home (%f > %f secs)!!!\n",p->myName,p->myLWP,now-p->shouldBeBackAt+tooLongTH,tooLongTH);
    }
    else
      count[p->myCat][0]++ ;
  }
  pthread_mutex_unlock(&g_utillock);
  if ( next_st_trace < now )
  {
    if ( next_st_trace && stack_trace_cnt )
    {
      TRACE(5,"There have been %u stack trace so far\n", stack_trace_cnt);
    }
    next_st_trace = now + tooLongTH*2e-1 ; // 60 secs
  }
  if ( bad && (count[HC_SEC][1] > count[HC_SEC][0] || count[HC_IOP][1] || count[HC_ADM][1] || count[HC_STO][1]) )
  {
    if ( healthAlert < traceLoops )
    {
       //Write some traces to temporary directory in case reason we are unhealthy is we are struggling to write
       //to our disk persist path
       ism_common_stack_trace((healthAlert & 0x01 )? 1 : 0);
    }
    else
    if ( healthAlert < traceLoops + stopLoops )
    {
      if (!adminStop )
      {
        admin_restart_t admin_restart_fp = (admin_restart_t)(uintptr_t)ism_common_getLongConfig("_ism_admin_init_stop_fnptr", 0L);
        if ( admin_restart_fp )
        {
          admin_restart_fp(1,NULL);
          adminStop++ ;
        }
        else
        {
          TRACE(1,"Failed to retrieve function pointer of ism_admin_init_stop!\n");
        }
      }
    }
    else
      ism_common_shutdown(1);
    healthAlert++ ;
  }
  else
    healthAlert = 0;
  return healthAlert ;
}

XAPI char *ism_common_show_mutex_owner(pthread_mutex_t *mutex, char *res, size_t len)
{
  ism_tls_health_t *p ;
  
  if (!mutex->__data.__owner)
  {
    snprintf(res,len," unlocked!");
    return res;
  }
  pthread_mutex_lock(&g_utillock);
  for ( p=healthHead ; p && p->myLWP != mutex->__data.__owner ; p=p->next ) ; //empty body
  if ( p )
    snprintf(res,len," lwp=%u, tid=%lu, name=%s",p->myLWP,(unsigned long)p->myId,p->myName);
  else
    snprintf(res,len," lwp=%u, tid=?, name=?",mutex->__data.__owner);
  pthread_mutex_unlock(&g_utillock);
  return res;
}
  

/*----  end   stuff for thread health monitoring  -----*/

XAPI void ism_common_affMaskToStr(char affMask[CPU_SETSIZE], int len, char * buff) {
    static const char * hexdigit = "0123456789abcdef";
    if(len) {
        if(len%4)
            len = (len/4+1)*4;
        buff[0] = '0';
        buff[1] = 'x';
        buff += 2;
        do {
            char v = 0;
            len -= 4;
            if(affMask[len])
                v |= 1;
            if(affMask[len+1])
                v |= 2;
            if(affMask[len+2])
                v |= 4;
            if(affMask[len+3])
                v |= 8;
            *buff = hexdigit[v&0x0f];
            buff++;
        } while (len > 0);
    } else {
        *buff = '0';
        buff++;
    }
    *buff = '\0';
}

/*
 * Start a thread.
 * A thread is configured and started.
 * Three parameters are passed to the thread.  These are called data, context, and value, but what is
 * passed is not defined.
 *
 * @param  thread   The returned thread handle
 * @param  addr     The address of the function to execute
 * @param  data     The data parameter to pass to the thread.
 * @param  context  The context parameter to pass to the thread.
 * @param  value    The value parameter to pass to the thread.
 * @param  usage    A indication of the usage of the thread which can be used to set other performance values
 * @param  flags    A set of flags for this thread.  Use 0 if no flags are required.
 * @param  name     The name of the thread to create.  This name is used for debugging and should be 16 characters
 *                  or less.  The name is not required to be unique, but it is recommended that it be so.
 *                  If muliple threads are created which do the same processing, an counter can be appended to
 *                  the end of the name.
 * @param  description  The description of the thread for problem determination.  This does not need to be unique
 *                  if the name is unique.
 * @return A return code: 0=good
 */
XAPI int   ism_common_startThread(ism_threadh_t * handle, ism_thread_func_t addr, void * data, void * context, int value,
                 enum thread_usage_e usage, int flags, const char * name, const char * description) {
    int  rc;
    ism_thread_parm_t * parm;

    parm = calloc(1, sizeof(struct ism_thread_parm_t));
    parm->method = addr;
    parm->data = data;
    parm->context = context;
    parm->value = value;
    parm->properties = ism_common_getThreadProperties(name);
    ism_common_strlcpy(parm->tname, name, 16);

    rc = pthread_create(handle, NULL, ism_run_thread, (void *)parm);

    if (rc) {
        *handle = 0;
        //this is all done before the memusage is set up for the thread so
        // need to free the raw parms
        ism_common_free_raw(ism_memory_utils_misc,parm);
        TRACE(3, "Failed to create thread %s: error=%d\n",name, rc);
        return rc;
    }

    TRACE(5, "Create thread %s: [%s] handle=%p data=%p context=%p value=%d\n",
            name, description, (void *)*handle, data, context, value);

    return rc;
}

/*
 * Get the name of the thread.
 */
XAPI int ism_common_getThreadName(char * buf, int len) {
    int  rc;
    ism_tls_t * tls = (ism_tls_t *)ism_common_getThreadKey(ism_threadKey, NULL);
    if (__builtin_expect(!tls, 0))
        makeTLS(512, NULL);
    if (tls != NULL) {
      rc = ism_common_strlcpy(buf, tls->tname, len);
        if (len > rc)
          return rc;
        return len-1;
    }
    *buf = '\0';
    return 0;
}

/*
 * Return the thread handle for the current thread
 * @return The thread handle for the current thread
 */
ism_threadh_t ism_common_threadSelf(void) {
    return pthread_self();
}

/*
 * End this thread.
 * When a thread is ended it can return a value which is kept until the join is done by th3e parent.
 * If this thread is detached, the return value is not retained.
 * <p>
 * This call does not return.
 *
 * @param retval  The return value.  This is normally a scalar cast to a void *.  It can be a pointer
 *                and if it is care should be taken that the memory is kept until the thread is joined.
 */
void ism_common_endThread(void * retval) {
    pthread_exit(retval);
}


/*
 * Wait for a thread to terminate and capture its return value.
 * @param  thread  The thread handle to join
 * @param  retval  The location to capture the return value of the thread.
 * @return A return code 0=good
 */
int ism_common_joinThread(ism_threadh_t thread, void ** retvalptr) {
    int rc;
    rc = pthread_join((pthread_t)thread, retvalptr);
    return rc;
}

/*
 * Wait for a thread to terminate for a given number of nanoseconds and
 * capture it's return value.
 * @param  thread  The thread handle to join
 * @param  retval  The location to capture the return value of the thread.
 * @param  timeout The number of nanoseconds to wait for the thread to join.
 * @return A return code 0=good
 */
int ism_common_joinThreadTimed(ism_threadh_t thread, void ** retvalptr, ism_time_t timeout){
	int rc;
	struct timespec abstime;
	timeout += ism_common_currentTimeNanos();
	abstime.tv_sec = (time_t)(timeout / 1000000000);
	abstime.tv_nsec = (long)(timeout % 1000000000);
	rc = pthread_timedjoin_np(thread, retvalptr, &abstime);
	return rc;
}

/*
 * Cancel a thread.
 * This is a request for a thread to terminate itself.
 * @param  thread  The thread to cancel.
 * @return A return code: 0=good
 */
int ism_common_cancelThread(ism_threadh_t thread) {
    return pthread_cancel((pthread_t)thread);
}


/*
 * Detach a thread.
 * This is used after a thread is started and at a point where the thread will no longer be joined
 * in the case of a failure.  This can be done in either the parent or child thread.
 */
int ism_common_detachThread(ism_threadh_t thread) {
    return pthread_detach((pthread_t)thread);
}


/*
 * Set thread affinity.
 * For each byte in the processor map, if the byte is non-zero the associated processor
 * is available for scheduling this thread.
 * @param thread  The thread to modify
 * @param map     An array of bytes where each byte represents a processor thread
 * @param len     The length of the map
 */
XAPI int   ism_common_setAffinity(ism_threadh_t thread, char * map, int len) {
    int   i;
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  for (i=0; i<len; i++) {
      if (map[i])
          CPU_SET(i, &cpu_set);
  }
  pthread_setaffinity_np(thread, sizeof(cpu_set), &cpu_set);
  return 0;
}

/**
 * Sets a function for thread cleanup. This method needs to be called
 * from the thread to be cleaned up, NOT from the thread that called ism_common_startThread.
 * @param cleanup_parm - A parameter to be passed to the cleanup routine.
 */
XAPI void ism_common_setThreadCleanup(ism_thread_cleanup_func_t cleanup_function, void * cleanup_parm) {
    ism_tls_t * tls = (ism_tls_t *)ism_common_getThreadKey(ism_threadKey, NULL);
    if (__builtin_expect(!tls, 0))
        makeTLS(512, NULL);
    if (tls != NULL) {
      tls->thread_cleanup = cleanup_function;
      tls->cleanup_parm = cleanup_parm;
    }
}

/********************************************************
*                                                       *
* ism monotonic cond_timedwait functions                *
*                                                       *
********************************************************/

/**
 * @Return the monotonic time in nanoseconds since some unspecified starting point.
 */
XAPI ism_time_t ism_common_monotonicTimeNanos(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts) ;
  return (ism_time_t)ts.tv_sec * 1000000000 + ts.tv_nsec ;
}

/**
 * Init a pthread_cond_t object to use CLOCK_MONOTONIC
 * for time measurements in pthread_cond_timedwait()
 * @return 0 on success and pthread error o.w.
 */
XAPI int ism_common_cond_init_monotonic(pthread_cond_t *restrict cond) {
  int rc;
  pthread_condattr_t attr[1] ;

  if ( (rc = pthread_condattr_init(attr)) )
    return rc ;
  if ( (rc = pthread_condattr_setclock(attr, CLOCK_MONOTONIC)) )
    return rc ;
  if ( (rc = pthread_cond_init(cond, attr)) )
    return rc ;
  pthread_condattr_destroy(attr);
  return 0 ;
}


/**
 * A wraper function around pthread_cond_timedwait for
 * pthread_cond_t objects that were initialyzed to use
 * CLOCK_MONOTONIC for timing.
 * @param  fRelative = 0 => wtime contains the target absolute monotonic time
 *                   = 1 => wtime contains a relative time to wait
 * @return 0 on success and pthread error o.w.
 */
XAPI int ism_common_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict wtime, uint8_t fRelative) {
  uint64_t nn;
  struct timespec ts[1];
  if ( fRelative )
    clock_gettime(CLOCK_MONOTONIC, ts) ;
  else
    memset(ts,0,sizeof(ts)) ;
  ts->tv_sec += wtime->tv_sec ;
  nn = ts->tv_nsec + wtime->tv_nsec ;
  while ( nn >= 1000000000 )
  {
    ts->tv_sec++ ;
    nn -= 1000000000 ;
  }
  ts->tv_nsec = nn ;
  return pthread_cond_timedwait(cond, mutex, ts) ;
}


/*
 * Sleep for a time in microseconds.
 */
void ism_common_sleep(int micros) {
    struct timespec req , rem ;
    req.tv_sec  = micros / 1000000 ;
    req.tv_nsec = (micros % 1000000) * 1000 ;
    while ( nanosleep(&req, &rem) == -1 ) {
        if ( errno != EINTR )
            break;
        req.tv_sec  = rem.tv_sec ;
        req.tv_nsec = rem.tv_nsec ;
    }
}


/*
 * Method put in delay loop with various strategies.
 * @param stategy: 0=poll, 1=yield, 2=sleep;
 */
 XAPI void ism_common_delay(int strategy) {
    switch (strategy) {
    case 1:
        sched_yield();
        break;
    case 2:
      {
        struct timespec req , rem ;
        req.tv_sec  = 0;
        req.tv_nsec = 1000000;
        nanosleep(&req, &rem);
      }
      break;
    }
 }


/*
 * Emergency shutdown
 */
void  ism_common_shutdown_int(const char * file, int line, int core) {
    traceFunction(1, TOPT_WHERE|TOPT_THREAD, file, line, "Shutdown\n");   /* Force flush */
    if (core)
        raise(SIGABRT);
    raise(SIGKILL);
    _exit(1);
}
#endif   /* ifndef WIN32 */


/*
 * Get the thread local error information and extend it if required
 */
struct ism_tls_t * getErrorData(int needed) {
    int datasize;
    ism_tls_t * tls = (ism_tls_t *) ism_common_getThreadKey(ism_threadKey, NULL);
    if (__builtin_expect(!tls, 0))
        tls = makeTLS(needed, NULL);
    if (__builtin_expect(tls->data_alloc < needed, 0)) {
        datasize = tls->data_alloc + 1024;
        while (datasize < needed)
            datasize *= 4;
        // tls is allocated outside of memory policing (as it's required for memory policing)
        // so use a simple realloc
        tls = realloc(tls, sizeof(ism_tls_t)+datasize);
        tls->data_alloc = datasize;
        ism_common_setThreadKey(ism_threadKey, (void *) tls);
        ism_common_threaddata = tls;
    }
    return tls;
}


/*
 * Return the last error code.
 * If there is no error, a zero is returned.
 */
int ism_common_getLastError(void) {
    struct ism_tls_t * tls;
    tls = getErrorData(0);
    return tls->errcode;
}

/*
 * Get error string from the error code
 */
const char * ism_common_getErrorString(int code, char * buffer, int len) {
    const char * str = NULL;

    if (code == 0)
        return NULL;
    switch (code) {
#define ISMRC_SPECIAL
#undef __ISM_RC_DEFINED
#define RC(name, val, opt, desc) case name: str = desc;  break;
    /* Define a few MQTT errors */
    case 2: str = "The ClientID is not valid";           break;
    case 3: str = "The server is not available";         break;
    case 5: str = "The connection is not authorized";    break;

#include <ismrc.h>
    default: break;
    }
    if (str) {
        ism_common_strlcpy(buffer, str, len);
    } else {
        snprintf(buffer, len, "Error: %u", (uint32_t)code);
    }
    return buffer;
#undef RC
#define __ISM_RC_DEFINED
#undef ISMRC_SPECIAL
}

/*
 * Get error string from the error code
 */
const char * ism_common_getErrorName(ism_rc_t code, char * buffer, int len) {
    const char * str = NULL;

    if (code == 0)
        return NULL;
    switch (code) {
#define ISMRC_SPECIAL
#undef __ISM_RC_DEFINED
#define RC(name, val, opt, desc) case name: str = #name;  break;
#include <ismrc.h>
    default: break;
    }
    if (str) {
        ism_common_strlcpy(buffer, str, len);
    } else {
        snprintf(buffer, len, "Error: %u", code);
    }
    return buffer;
#undef RC
#define __ISM_RC_DEFINED
#undef ISMRC_SPECIAL
}


/*
 * Get error options from the error code
 */
int ism_common_getErrorOptions(ism_rc_t code) {
    int options = 0;

    switch (code) {
#define ISMRC_SPECIAL
#undef __ISM_RC_DEFINED
#define RC(name, val, opt, desc) case name: options = opt;  break;
#include <ismrc.h>
    default: break;
    }
    return options;
#undef RC
#define __ISM_RC_DEFINED
#undef ISMRC_SPECIAL
}


/*
 * Construct message id
 */
static void constructMsgId(ism_rc_t code, char * buffer) {
    sprintf(buffer, "%s%04d", ISM_MSG_PREFIX, code%10000);
}


/*
 * Get error string by locale for a particular error code.
 */
const char * ism_common_getErrorStringByLocale(ism_rc_t code, const char * locale, char * buffer, int len) {
    char  msgID[12];
    const char * msgf;

    /* Construct the message ID for the ICU bundle */
    constructMsgId(code, msgID);

    /* Look up the message */
    msgf = ism_common_getMessageByLocale(msgID, locale, buffer, len, NULL);

    /* If not found, use the default */
    if (!msgf)
        msgf = ism_common_getErrorString(code, buffer, len);
    return msgf;
}


/*
 * Format the last error
 */
int ism_common_formatLastError(char * buffer, int len) {
    return ism_common_formatLastErrorByLocale(NULL, buffer, len);
}

/*
 * Get a string from the buffer.
 */
const char * ism_common_getReplacementDataString(concat_alloc_t * buf)
{
    int32_t len;
    const char * ret;

    if (buf->pos + 4 > buf->used)
        return "";
    memcpy(&len, buf->buf+buf->pos, 4);
    len++;
    buf->pos += 4;
    if (len<0 || len+buf->pos > buf->used) {
        buf->pos = buf->used;
        return "";
    }
    ret = buf->buf + buf->pos;
    buf->pos += len;
    return ret;
}

/*
 * Get the count of replacement vars
 */
int replCnt(const char * msg) {
    int cnt = 0;
    while (*msg) {
        if (*msg == '{')
            cnt++;
        msg++;
    }
    return cnt;
}

/*
 * Get replacement data n from the per thread error buffer
 */
const char * ism_common_getErrorRepl(int which) {
    struct ism_tls_t * tls;
    int count;
    const char * ret = NULL;
    concat_alloc_t buf = {0};

    tls = getErrorData(0);
    count = tls->count;
    buf.buf = (char *)(tls+1);
    buf.used = tls->data_len;

    if (which < count) {
        int i;
        for (i=0; i<=which; i++)
            ret = ism_common_getReplacementDataString(&buf);
    }
    return ret;
}

/*
 * Format the last error by locale.
 */
int ism_common_formatLastErrorByLocale(const char * locale, char * buffer, int len) {
    struct ism_tls_t * tls;
    char   xbuf[1024];
    const char * repl[64];
    concat_alloc_t buf = {0};
    int    count;
    int    i;
    int    replcnt;
    const char * msgf;

    tls = getErrorData(0);
    if (locale)
        msgf = ism_common_getErrorStringByLocale(tls->errcode, locale, xbuf, sizeof(xbuf));
    else
        msgf = ism_common_getErrorString(tls->errcode, xbuf, sizeof(xbuf));
    if (!msgf) {
        strcpy(xbuf, "OK");
        msgf = xbuf;
    }
    buf.buf = (char *)(tls+1);
    buf.used = tls->data_len;
    count = tls->count;
    if (count > 64)
        count = 64;
    for (i=0; i<count; i++) {
        repl[i] = ism_common_getReplacementDataString(&buf);
    }

    /* If the message does not have replacement data, put in any data we have */
    if (count) {
        replcnt = replCnt(msgf);
        if (count > replcnt) {
            char mbuf[5*64];
            char *mp = mbuf;
            char mlen = strlen(xbuf);
            if (mlen>0 && xbuf[mlen-1]=='.')
                xbuf[mlen-1] = 0;
            if (replcnt == 0)
                *mp++ = ':';
            for (i=replcnt; i<count; i++) {
                sprintf(mp, " {%u}", (uint32_t)i);
                mp += strlen(mp);
            }
            ism_common_strlcat(xbuf, mbuf, sizeof xbuf);
        }
    }
    return ism_common_formatMessage(buffer, len, msgf, repl, count);
}


/*
 * Simple form of setError with no replacement data
 */
void ism_common_setError_int(ism_rc_t rc, const char * filename, int where) {
    struct ism_tls_t * tls;
    const char * fp;
    char xbuf[1024];

    if (filename) {
        fp = filename + strlen(filename);
        while (fp > filename && fp[-1] != '/' && fp[-1] != '\\')
            fp--;
    } else {
        fp = "";
    }
    if (rc == 0) {
        TRACE(7, "Reset last error at %s:%d\n", fp, where);
    } else {
        if ((rc < ISMRC_ConnectionComplete ? 7 : 5) <= ism_defaultTrace->trcComponentLevels[TRACECOMP_System]) {
            ism_common_getErrorString(rc, xbuf, sizeof xbuf);
            traceFunction(5, 0, fp, where, "Set error \"%s\" (%d)\n", xbuf, rc);
        }
    }
    tls = getErrorData(0);
    tls->errcode = (int) rc;
    tls->count   = 0;
    tls->lineno  = where;
    ism_common_strlcpy(tls->filename, fp, sizeof tls->filename);
}

extern int ism_log_parseReplacementData(concat_alloc_t * buf, const char * types, va_list args);

/*
 * Set the last error based on a return code and replacement string.
 * Each return code has a return code type associated with it which is used
 * to get the name of the return code.
 */
void ism_common_setErrorData_int(ism_rc_t rc, const char * filename, int where, const char * fmt, ...) {
    struct ism_tls_t * tls;
    va_list args;
    concat_alloc_t buf = {0};
    char xbuf[4096];
    const char * fp;
    int   count;

    /* Shorten the file name (no path) */
    if (filename) {
        fp = filename + strlen(filename);
        while (fp > filename && fp[-1] != '/' && fp[-1] != '\\')
            fp--;
    } else {
        fp = "";
    }

    /*
     * Convert the args to strings
     */
    buf.buf = xbuf;
    buf.len = sizeof(xbuf);
    va_start(args, fmt);
    count = ism_log_parseReplacementData(&buf, fmt, args);
    va_end(args);

    /*
     * Populate the tls
     */
    tls = getErrorData(buf.used);
    tls->errcode = (int) rc;
    tls->count   = count;
    tls->data_len = buf.used;
    memcpy((char *)(tls+1), buf.buf, buf.used);
    tls->lineno = where;
    ism_common_strlcpy(tls->filename, fp, sizeof(tls->filename));

    /* Trace the error data */
    if ((rc < ISMRC_ConnectionComplete ? 7 : 5) <= ism_defaultTrace->trcComponentLevels[TRACECOMP_System]) {
        ism_common_formatLastError(xbuf, sizeof xbuf);
        traceFunction(5, 0, fp, where, "Set error \"%s\" (%d)\n", xbuf, rc);
    }

    if (buf.inheap)
        ism_common_freeAllocBuffer(&buf);
}


/*
 * Put out the characters
 */
static int putchars(uint8_t * line, const uint8_t * data, int len) {
    int       i;
    uint8_t * cp = line;
    *cp++ = ' ';
    *cp++ = '[';
    for (i=0; i<len; i++) {
        if (data[i]<0x20 || data[i]>=0x7f)
            *cp++ = '.';
        else
            *cp++ = data[i];
    }
    *cp++ = ']';
    *cp++ = '\n';
    *cp   = 0;
    return cp-line;
}

/*
 * Trace data
 */
void ism_common_traceData(const char * label, int option, const char * file, int where, const void * vbuf, int buflen, int maxlen) {
    static const char * hexdigit = "0123456789abcdef";
    concat_alloc_t buf = {0};
    char      xbuf [8192];
    uint8_t   line [1024];
    const uint8_t * data;
    int       pos;
    int       i;
    uint8_t * cp;
    int       mod;

    buf.buf = xbuf;
    buf.len = sizeof xbuf;
    buf.used = 0;

    data = (uint8_t *) vbuf;
    if (buflen < maxlen)
        maxlen = buflen;
    if (!vbuf)
        maxlen = 0;

    snprintf((char *)line, sizeof(line), "%s: len=%d ", label, buflen);
    ism_common_allocBufferCopy(&buf, (const char *)line);
    buf.used--;
    if (maxlen < 96) {    /* TODO: back to 32 */
        cp = line;
        mod = 0;
        for (pos=0; pos<maxlen; pos++) {
            *cp++ = hexdigit[(data[pos]>>4)&0x0f];
            *cp++ = hexdigit[data[pos]&0x0f];
            if (mod++ == 3) {
                *cp++ = ' ';
                mod = 0;
            }
        }
        cp += putchars(cp, data, maxlen);
        ism_common_allocBufferCopyLen(&buf, (const char *)line, cp-line);
    } else {
        pos = 0;
        ism_common_allocBufferCopyLen(&buf, "\n", 1);
        while (pos < maxlen) {
            sprintf((char *)line, "%05u: ", (uint32_t)pos);
            cp = line + strlen((char *)line);
            mod = 0;
            for (i=0; i<32; i++, pos++) {
                if (pos >= maxlen) {
                    *cp++ = ' ';
                    *cp++ = ' ';
                } else {
                    *cp++ = hexdigit[(data[pos]>>4)&0x0f];
                    *cp++ = hexdigit[data[pos]&0x0f];
                }
                if (mod++ == 3) {
                    *cp++ = ' ';
                    mod = 0;
                }
            }
            cp += putchars(cp, data+pos-32, pos<=maxlen ? 32 : maxlen-pos+32);
            ism_common_allocBufferCopyLen(&buf, (const char *)line, cp-line);
        }
    }
    ism_common_allocBufferCopyLen(&buf, "", 1);
    ism_common_trace(2, option, file, where, "%s", buf.buf);
    ism_common_freeAllocBuffer(&buf);
}

/* ********************************************************************
 *
 * Buffer functions
 *
 **********************************************************************/
/*
 * Free an allocation buffer
 */
void ism_common_freeAllocBuffer(concat_alloc_t * buf) {
    if (buf->inheap) {
        if (buf->buf)
            ism_common_free(ism_memory_alloc_buffer,buf->buf);
        buf->buf = NULL;
        buf->len = 0;
        buf->inheap = 0;
    }
}

/*
 * Allocate space in an allocation buffer directly on the heap
 * if the buffer has already been allocated then dro down to standard alloc
 */
char * ism_common_allocAllocBufferOnHeap(concat_alloc_t * buf, int len)
{
	if( buf->used == 0 && buf->len == 0){
		char*  tbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_alloc_buffer,5),256);
		buf->buf = tbuf;
		buf->len = len;
		buf->inheap=1;
		return buf->buf;
	}
	else
	{
		return ism_common_allocAllocBuffer(buf,len,false);
	}

}

/*
 * Allocate space in an allocation buffer
 */
char * ism_common_allocAllocBuffer(concat_alloc_t * buf, int len, int aligned) {
    char * ret;
    if (buf->used + len + 7 > buf->len) {
        int newsize = 64*1024;
        while (newsize < buf->used + len + 7)
            newsize *= 2;
        if (buf->inheap) {
            char * tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_alloc_buffer,1),buf->buf, newsize);
            if (tmp)
            buf->buf = tmp;
            else
                return 0;
        } else {
            char * tmpbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_alloc_buffer,2),newsize);
            if (tmpbuf && buf->used)
                memcpy(tmpbuf, buf->buf, buf->used > buf->len ? buf->len : buf->used);
            buf->buf = tmpbuf;
        }
        if (!buf->buf)
            return 0;
        buf->inheap = 1;
        buf->len = newsize;
    }
    ret = buf->buf + buf->used;
    if (aligned) {
        ret = (char *)(((uintptr_t)(ret+7))&~7);
    }
    buf->used += len;
    return ret;
}


/*
 * Put out within a constrained buffer.
 * If the buffer is allocated at less than 64K, it is assumed to be on
 * the stack, otherwise it is on the heap.
 */
char * ism_common_allocBufferCopyLen(concat_alloc_t * buf, const char * newbuf, int len) {
    char * ret;
    if (buf->used + len  > buf->len) {
        int newsize = 64*1024;
        while (newsize < buf->used + len)
            newsize *= 2;
        if (buf->inheap) {
            char * tmp = ism_common_realloc(ISM_MEM_PROBE(ism_memory_alloc_buffer,3),buf->buf, newsize);
            if (tmp)
                buf->buf = tmp;
            else
                return 0;
        } else {
            char * tmpbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_alloc_buffer,4),newsize);
            if (tmpbuf && buf->used)
                memcpy(tmpbuf, buf->buf, buf->used > buf->len ? buf->len : buf->used);
            buf->buf = tmpbuf;
        }
        if (!buf->buf)
            return 0;
        buf->inheap = 1;
        buf->len = newsize;
    }
    ret = buf->buf+buf->used;
    memcpy(ret, newbuf, len);
    buf->used += len;
    return ret;
}


/*
 * Put out within a constrained buffer
 */
char * ism_common_allocBufferCopy(concat_alloc_t * buf, const char * newbuf) {
    return ism_common_allocBufferCopyLen(buf, newbuf, (int)strlen(newbuf)+1);
}

/* ********************************************************************
 *
 * String functions
 *
 **********************************************************************/

/*
 * Tokenize a string with separate leading and trailing characters.
 */
char * ism_common_getToken(char * from, const char * leading, const char * trailing, char * * more) {
    char * ret;
    if (!from)
        return NULL;
    while (*from && strchr(leading, *from))
        from++;
    if (!*from) {
        if (more)
            *more = NULL;
        return NULL;
    }
    ret = from;
    while (*from && !strchr(trailing, *from))
        from++;
    if (*from) {
        *from = 0;
        if (more)
            *more = from + 1;
    } else {
        if (more)
            *more = NULL;
    }
    return ret;
}

/*
 * Return the count of tokens in a string.
 * @param str   The string to look for tokens
 * @param delim The set of allowed delimiters
 * @return the count of tokens
 */
int ism_common_countTokens(const char * str, const char * delim) {
    int  count = 0;
        if (str) {
        while (*str) {
            while (*str && strchr(delim, *str))
                str++;
            if (!*str)
                break;
            count++;
            while (*str && !strchr(delim, *str))
                str++;
        }
    }
    return count;
}


/*
 * Implement strlcpy
 */
size_t ism_common_strlcpy(char *dst, const char *src, size_t size) {
    size_t len = strlen(src);
    if (len < size) {
        memcpy(dst, src, len+1);
    } else if (size > 0) {
        memcpy(dst, src, size-1);
        dst[size-1] = 0;
    }
    return len;
}


/*
 * Implement strlcat
 */
size_t ism_common_strlcat(char *dst, const char *src, size_t size) {
    size_t lensrc = strlen(src);
    char * pos = memchr(dst, 0, size);
    size_t lendst = pos ? pos-dst : size;
    size_t len = lensrc + lendst;

    if (len < size) {
        memcpy(dst+lendst, src, lensrc+1);
    } else if (lendst < size) {
        memcpy(dst+lendst, src, size-lendst-1);
        dst[size-1] = 0;
    }
    return len;
}

/* ********************************************************************
 *
 * Config functions
 *
 **********************************************************************/

/*
 * Get a configuration property as a string
 * @param name   The name of the config item
 * @return the value of the configuration property, or NULL if it is not set
 */
const char * ism_common_getStringConfig(const char * name) {
    return ism_common_getStringProperty(ism_common_getConfigProperties(), name);
}

/*
 * Get a configuration property as an integer
 * @param name   The name of the config item
 * @param defval The default value
 * @return the value of the configuration property as an integer, or the default value if the config property
 *         is not set or cannot be converted to an integer.
 */
int ism_common_getIntConfig(const char * name, int defval) {
    return ism_common_getIntProperty(ism_common_getConfigProperties(), name, defval);
}

/*
 * Get a configuration property as a boolean
 * @param name   The name of the config item
 * @param defval The default value
 * @return the value of the configuration property as a boolean (0=false, 1=true), or the default value if the config
 *         property is not set of cannot be converted to a boolean.
 */
int ism_common_getBooleanConfig(const char * name, int defval) {
    return ism_common_getBooleanProperty(ism_common_getConfigProperties(), name, defval);
}

/*
 * Get a configuration property as a long
 * @param name   The name of the config item
 * @param defval The default value
 * @return the value of the configuration property as a long (64 bit) integer, or the default value if the config
 *         property is not set of cannot be converted to a boolean.
 */
int64_t ism_common_getLongConfig(const char * name, int64_t defval) {
    return ism_common_getLongProperty(ism_common_getConfigProperties(), name, defval);
}

/* ********************************************************************
 *
 * Data conversion functions
 *
 **********************************************************************/

/*
 * Convert signed integer to ASCII.
 * @param ival Integer value to convert
 * @parm  buf  The output buffer.  This should be at least 12 bytes long
 */
char * ism_common_itoa(int32_t ival, char * buf) {
    char   bufr[32], * bp;
    char * outp = buf;
    uint32_t val;
    int      len;
    int      digit;

    if (ival<0) {
        val = (uint32_t)-ival;
        *buf++ = '-';
    } else {
        val = (uint32_t)ival;
    }

    bp = bufr+sizeof(bufr)-1;
    do {
        digit = val % 10;
        val /= 10;
        *bp-- = ('0'+digit);
    } while (val);
    len = (int)sizeof(bufr)-1-(int)(bp-bufr);
    memcpy(buf, bp+1, len);
    buf[len] = 0;
    return outp;
}


/*
 * Convert a signed 64 bit integer to ASCII.
 * @param lval Long (64) bit integer value
 * @param buf  The output buffer.  This should be at least 24 bytes long
 */
char * ism_common_ltoa(int64_t lval, char * buf) {
    char   bufr[32], * bp;
    char * outp = buf;
    uint64_t val;
    int      len;
    int      digit;

    if (lval<0) {
        val = (uint64_t)-lval;
        *buf++ = '-';
    } else {
        val = (uint64_t)lval;
    }

    bp = bufr+sizeof(bufr)-1;
    do {
        digit = (int)(val % 10);
        val /= 10;
        *bp-- = ('0'+digit);
    } while (val);
    len = (int)sizeof(bufr)-1-(int)(bp-bufr);
    memcpy(buf, bp+1, len);
    buf[len] = 0;
    return outp;
}

/*
 * Put out one hex digit
 */
static char * hexout(char * buf, int val) {
    static char * hexdigit = "0123456789ABCDEF";
    *buf++ = hexdigit[(val>>4)&0xf];
    *buf++ = hexdigit[val&0xf];
    return buf;
}


/*
 * Convert an unsigned 32 bit integer to hex
 */
char * ism_common_uitox(uint32_t uval, int shorten, char * buf) {
    char * sbuf = buf;
    int  val;

    val = (int)(uval>>24);
    if (!shorten || val)
    {
        shorten=false;
        sbuf = hexout(sbuf, val);
    }
    val = (int)((uval>>16) & 0xff);
    if (!shorten || val)
    {
        shorten=false;
        sbuf = hexout(sbuf, val);
    }
    val = (int)((uval>>8) & 0xff);
    if (!shorten || val)
    {
        shorten=false;
        sbuf = hexout(sbuf, (int)val);
    }
    sbuf = hexout(sbuf, uval);
    *sbuf = 0;
    return buf;
}


/*
 * Convert an unsigned 64 bit integer to hex
 */
char * ism_common_ultox(uint64_t uval, int shorten, char * buf) {
    char * sbuf = buf;
    int  val;

    val = (int)(uval>>56);
    if (!shorten || val)
    {
        shorten = false;
        sbuf = hexout(sbuf, val);
    }
    val = (int)((uval>>48) & 0xff);
    if (!shorten || val)
    {
        shorten = false;
        sbuf = hexout(sbuf, val);
    }
    val = (int)((uval>>40) & 0xff);
    if (!shorten || val)
    {
        shorten = false;
        sbuf = hexout(sbuf, val);
    }
    val = (int)((uval>>32) & 0xff);
    if (!shorten || val)
    {
        shorten = false;
        sbuf = hexout(sbuf, val);
    }
    val = (int)((uval>>24) & 0xff);
    if (!shorten || val)
    {
        shorten = false;
        sbuf = hexout(sbuf, val);
    }
    val = (int)((uval>>16) & 0xff);
    if (!shorten || val)
    {
        shorten = false;
        sbuf = hexout(sbuf, val);
    }
    val = (int)((uval>>8) & 0xff);
    if (!shorten || val)
    {
        shorten = false;
        sbuf = hexout(sbuf, val);
    }
    sbuf = hexout(sbuf, (int)uval);
    *sbuf = 0;
    return buf;
}


/*
 * Convert an unsigned 32 bit integer to ASCII.
 * @param  uval  The unsigned integer value.
 * @param  buf   The output buffer.  This should be at least 12 bytes long.
 */
char * ism_common_uitoa(uint32_t uval, char * buf) {
    char   bufr[32], * bp;
    int    len;
    int      digit;

    bp = bufr+sizeof(bufr)-1;
    do {
        digit = uval % 10;
        uval /= 10;
        *bp-- = ('0'+digit);
    } while (uval);
    len = (int)sizeof(bufr)-1-(int)(bp-bufr);
    memcpy(buf, bp+1, len);
    buf[len] = 0;
    return buf;
}


/*
 * Convert an unsigned 64 bit unsigned integer to ASCII.
 * @param ulval  The unsigned long value
 * @param buf    The output buffer.  This should be at least 24 bytes long.
 */
char * ism_common_ultoa(uint64_t ulval, char * buf) {
    char   bufr[32], * bp;
    int    len;
    int      digit;

    bp = bufr+sizeof(bufr)-1;
    do {
        digit  = (int)(ulval % 10);
        ulval /= 10;
        *bp-- = ('0'+digit);
    } while (ulval);
    len = (int)sizeof(bufr)-1-(int)(bp-bufr);
    memcpy(buf, bp+1, len);
    buf[len] = 0;
    return buf;
}

/*
 * Convert signed integer to ASCII wit separators.
 * @param ival Integer value to convert
 * @parm  buf  The output buffer.  This should be at least 12 bytes long
 */
char * ism_common_itoa_ts(int32_t ival, char * buf, char triad) {
    char   bufr[32], * bp;
    char * outp = buf;
    uint32_t val;
    int      len;
    int      digit;
    int      pos = 0;

    if (ival<0) {
        val = (uint32_t)-ival;
        *buf++ = '-';
    } else {
        val = (uint32_t)ival;
    }

    bp = bufr+sizeof(bufr)-1;
    do {
        if (pos==3) {
            *bp-- = triad;
            pos = 0;
        }
        pos++;
        digit = val % 10;
        val /= 10;
        *bp-- = ('0'+digit);
    } while (val);
    len = (int)sizeof(bufr)-1-(int)(bp-bufr);
    memcpy(buf, bp+1, len);
    buf[len] = 0;
    return outp;
}


/*
 * Convert a signed 64 bit integer to ASCII with separators.
 * @param lval Long (64) bit integer value
 * @param buf  The output buffer.  This should be at least 32 bytes long
 */
char * ism_common_ltoa_ts(int64_t lval, char * buf, char triad) {
    char   bufr[32], * bp;
    char * outp = buf;
    uint64_t val;
    int      len;
    int      pos = 0;
    int      digit;

    if (lval<0) {
        val = (uint64_t)-lval;
        *buf++ = '-';
    } else {
        val = (uint64_t)lval;
    }

    bp = bufr+sizeof(bufr)-1;
    do {
        if (pos==3) {
            *bp-- = triad;
            pos = 0;
        }
        pos++;
        digit  = (int)(val % 10);
        val /= 10;
        *bp-- = ('0'+digit);
    } while (val);
    len = (int)sizeof(bufr)-1-(int)(bp-bufr);
    memcpy(buf, bp+1, len);
    buf[len] = 0;
    return outp;
}



/*
 * Convert an unsigned 64 bit integer to ASCII with separators.
 * @param lval Unsigned long (64) bit integer value
 * @param buf  The output buffer.  This should be at least 32 bytes long
 */
char * ism_common_ultoa_ts(uint64_t lval, char * buf, char triad) {
    char   bufr[32], * bp;
    char * outp = buf;
    uint64_t val;
    int      len;
    int      pos = 0;
    int      digit;

    val = lval;
    bp = bufr+sizeof(bufr)-1;
    do {
        if (pos==3) {
            *bp-- = triad;
            pos = 0;
        }
        pos++;
        digit  = (int)(val % 10);
        val /= 10;
        *bp-- = ('0'+digit);
    } while (val);
    len = (int)sizeof(bufr)-1-(int)(bp-bufr);
    memcpy(buf, bp+1, len);
    buf[len] = 0;
    return outp;
}


/*
 * Convert a float to ASCII
 * @param flt  The float value (this is passed as a double in C)
 * @param buf  The output buffer.  This should be at least 32 bytes long.
 */
char * ism_common_ftoa(double flt, char * buf) {
    char * cp;
    char * outp = buf;
    double d2;
    int    x;
    double f;
    union{uint64_t l; double d;} up;
    union{uint32_t i; float f;} uf;

    if (flt<0.0) {
        *buf++ = '-';
        flt = -flt;
    }

    /*
     * Check for NaN and Infinity
     */
    uf.f = (float)flt;
    if ((uf.i&0x7f800000) == 0x7f800000) {
        if ((uf.i&0x7fffff) == 0) {
            memcpy(buf, "Infinity", 9);
        } else {
            memcpy(outp, "NaN", 4);   /* Ignore the sign */
        }
        return outp;
    }

    /*
     * For very large or small values use E notation
     */
    if (flt < 1E-5 || flt>=1E10) {
        if (flt == 0) {
            memcpy(buf, "0.0", 4);
        } else {
            sprintf(buf, "%.8g", flt);
            cp = buf+strlen(buf);
            while (cp[-1] == ' ')
                cp--;
            *cp = 0;
        }
        return outp;
    }

    /*
     * Handle integer
     */
    f = floor(flt);
    if ((float)floor(flt)==(float)flt && flt<1E9) {
        ism_common_itoa((int32_t)flt, buf);
        strcat(buf, ".0");
    } else {
        /* Handle 2 decimal digits */
        d2 = flt*100.0;
        up.d = d2;
        up.l &= 0x7ff0000000000000LL;
        up.l -= 0x0170000000000000LL;
        d2 += up.d;
        f = floor(d2);
        if (d2-f < up.d*2) {
            ism_common_uitoa((uint32_t)(f/100.0), buf);
            if (d2>2E9)
                x = (int)(((uint64_t)f)%100);
            else
                x = ((int)f)%100;
            cp = buf + strlen(buf);
            cp[0] = '.';
            cp[1] = (char)(x/10 + '0');
            cp[2] = (char)(x%10 + '0');
            if (cp[2]=='0') {
                cp[2] = 0;
            } else {
                cp[3] = 0;
            }
        } else {
            d2 = flt*10000.0;
            up.d = d2;
            up.l &= 0x7ff0000000000000LL;
            up.l -= 0x0170000000000000LL;
            d2 += up.d;
            f = floor(d2);
            if (d2-f < up.d*2) {
                ism_common_uitoa((uint32_t)(f/10000.0), buf);
                if (d2>2E9)
                    x = (int)(((uint64_t)f)%10000);
                else
                    x = ((int)f)%10000;
                cp = buf + strlen(buf);
                cp[0] = '.';
                cp[1] = (char)(x/1000 + '0');
                cp[2] = (char)(x/100%10 + '0');
                cp[3] = (char)(x/10%10 + '0');
                cp[4] = (char)(x%10 + '0');
                cp += 5;
                while (cp[-1] == '0')
                    cp--;
                *cp = 0;
            } else {
                 sprintf(buf, "%.9f", flt);
                 cp = buf+strlen(buf);
                 while (cp[-1]=='0')
                     cp--;
                 *cp = 0;
            }
        }
    }
    return outp;
}


/*
 * Convert a double to ASCII
 * @param dbl  The double value.
 * @param buf  The output buffer.  This should be at least 32 bytes long.
 */
char * ism_common_dtoa(double dbl, char * buf) {
    char * cp;
    char * outp = buf;
    double d2;
    int    x;
    union{uint64_t l; double d;} ud;

    if (dbl<0.0) {
        *buf++ = '-';
        dbl = -dbl;
    }

    /*
     * Check for NaN and Infinity
     */
    ud.d = dbl;
    if ((ud.l&0x7ff0000000000000LL) == 0x7ff0000000000000LL) {
        if ((ud.l&0xfffffffffffffLL) == 0) {
            memcpy(buf, "Infinity", 9);
        } else {
            memcpy(outp, "NaN", 4);                      /* Ignore the sign*/
        }
        return outp;
    }

    /*
     * For very large or small values or for NaN or infinity use E notation
     */
    if (dbl < 1E-5 || dbl>=1E10) {
        if (dbl == 0) {
            memcpy(buf, "0.0", 4);
        } else {
            sprintf(buf, "%.8g", dbl);
            cp = buf+strlen(buf);
            while (cp[-1] == ' ')
                cp--;
            *cp = 0;
        }
        return outp;
    }

    /*
     * Handle integer
     */
    if (floor(dbl)==dbl && dbl<1E9) {
        ism_common_itoa((int32_t)dbl, buf);
        strcat(buf, ".0");
    } else {
        /* Handle 2 decimal digits */
        if (dbl<0.0) {
            dbl = -dbl;
        }
        d2 = dbl*100.0;
        if (floor(d2) == d2) {
            ism_common_uitoa((uint32_t)(d2/100.0), buf);
            if (d2>2E9)
                x = (int)(((uint64_t)d2)%100);
            else
                x = ((int)d2)%100;
            cp = buf + strlen(buf);
            cp[0] = '.';
            cp[1] = (char)(x/10 + '0');
            cp[2] = (char)(x%10 + '0');
            if (cp[2]=='0') {
                cp[2] = 0;
            } else {
                cp[3] = 0;
            }
        } else {
            d2 = dbl*10000.0;
            if (floor(d2) == d2) {
                ism_common_uitoa((uint32_t)(d2/10000.0), buf);
                if (d2>2E9)
                    x = (int)(((uint64_t)d2)%10000);
                else
                    x = ((int)d2)%10000;
                cp = buf + strlen(buf);
                cp[0] = '.';
                cp[1] = (char)(x/1000 + '0');
                cp[2] = (char)(x/100%10 + '0');
                cp[3] = (char)(x/10%10 + '0');
                cp[4] = (char)(x%10 + '0');
                cp += 5;
                while (cp[-1] == '0')
                    cp--;
                *cp = 0;
            } else {
                 sprintf(buf, "%.9f", dbl);
                 cp = buf+strlen(buf);
                 while (cp[-1]=='0')
                     cp--;
                 *cp = 0;
            }
        }
    }
    return outp;
}

/*
 * Get a buffer size.
 * The buffer size is a string containing an optional suffix of K or M to indicate
 * thousands or millions.
 * @param cfgname  A configuration parameter name
 * @param ssize    A string representation of the parameter value
 * @param dsize    A string representation of the alternative parameter value
 * @return An integer value corresponding to ssize (or dsize, if ssize is NULL),
 *         if input is valid. Otherwise 0.
 */
int ism_common_getBuffSize(const char * cfgname, const char * ssize, const char * dsize) {
    uint64_t val;
    char * eos;

    if (!ssize || !*ssize)
        ssize = dsize;
    val = (uint32_t)strtoul(ssize, &eos, 10);
    if (eos) {
        while (*eos == ' ')
            eos++;
        if (*eos == 'k' || *eos == 'K')
            val *= 1024;
        else if (*eos == 'm' || *eos == 'M')
            val *= (1024 * 1024);
        else if (*eos)
            val = 0;
    }

    /* The default is zero, but anything else which resolves to zero is an error */
    if ((val == 0 && strcmp(ssize, "0")) || val >= INT_MAX) {
        TRACE(3, "The buffer size %s = %s is not correct and is ignored.", cfgname, ssize);
        val = 0;
    }
    return (int)val;
}


#ifdef _WIN32

XAPI int ism_common_createThreadKey(ism_threadkey_t * keyAddr) {
   *keyAddr = TlsAlloc();
   if (*keyAddr == TLS_OUT_OF_INDEXES) {
       return -1;
   } else {
       return 0;
   }
}

XAPI void * ism_common_getThreadKey(ism_threadkey_t key, int * rc) {
   void * data = NULL;

   data = TlsGetValue((DWORD)key);
   //check error
   if (data == 0 && GetLastError() != ERROR_SUCCESS) {
       *rc = -1;
   } else {
       *rc = 0;
   }
   return data;
}

XAPI int ism_common_setThreadKey(ism_threadkey_t key, void * data) {
   BOOL rc = 0;

   rc = TlsSetValue((DWORD)key, data);

   if (rc == 0) {
       return -1;
   } else {
       return 0;
   }
}

XAPI int ism_common_destroyThreadKey(ismthreadkey_t key) {
   BOOL rc = 0;

   rc = TlsFree((DWORD)key);

   if (rc != 0) {
       return -1;
   } else {
       return 0;
   }
}

#else

XAPI int ism_common_createThreadKey(ism_threadkey_t * keyAddr) {
   return pthread_key_create(keyAddr, NULL);
}

XAPI void * ism_common_getThreadKey(ism_threadkey_t key, int * rc) {
   void * data = NULL;

   data = pthread_getspecific(key);
   if (rc)
       *rc = 0;
   return data;
}

XAPI int ism_common_setThreadKey(ism_threadkey_t key, void * data) {
   int rc = 0;

   rc = pthread_setspecific(key, data);

   if (rc != 0) {
       return -1;
   } else {
       return 0;
   }
}

XAPI int ism_common_destroyThreadKey(ism_threadkey_t key) {
   int rc = 0;

   rc = pthread_key_delete(key);

   if (rc != 0) {
       return -1;
   } else {
       return 0;
   }
}

#endif

XAPI void ism_common_freeTLS(void) {
  ism_tls_t * tls = (ism_tls_t *) ism_common_getThreadKey(ism_threadKey, NULL);

  if (tls) {
        ism_common_closeTimestamp(tls->trc_ts);
        ism_common_destroyThreadMemUsage();
        //TLS is created outside of memory policing
        ism_common_free_raw(ism_memory_utils_misc,tls);

        ism_common_setThreadKey(ism_threadKey, NULL);
  }
}

/*
 * Returns a pointer to a (const) string that we use for
 * temporary data that is possibly (but not guarenteed) to be cleared
 * before our next start
 */
XAPI const char *ism_common_getRuntimeTempDir(void) {
    return g_runtime_dir;
}


static const char b64digit [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Convert from a base256 string to a base64 string.
 * @param from  The input string
 * @param to    The output location.  This should be allocated at least (fromlen+2)/3*4+1 bytes
 * @param fromlen The length of the input string
 * @return The size of the output, or a negative value to indicate an error
 */
XAPI int ism_common_toBase64(char * from, char * to, int fromlen) {
    int j = 0;
    int val = 0;
    int left = fromlen;
    int len =  (fromlen+2) / 3 * 3;
    int i;

    for (i=0; i<len; i += 3) {
        val = (int)(from[i]&0xff)<<16;
        if (left > 1)
            val |= (int)(from[i+1]&0xff)<<8;
        if (left > 2)
            val |= (int)(from[i+2]&0xff);
        to[j++] = b64digit[(val>>18)&0x3f];
        to[j++] = b64digit[(val>>12)&0x3f];
        to[j++] = left>1 ? b64digit[(val>>6)&0x3f] : '=';
        to[j++] = left>2 ? b64digit[val&0x3f] : '=';
        left -= 3;
    }
    to[j] = 0;
    return j;
}


/*
 * Map base64 digit to value
 */
static char b64val [] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* 00 - 0F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* 10 - 1F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,   /* 20 - 2F + / */
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1,   /* 30 - 3F 0 - 9 = */
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,   /* 40 - 4F A - O */
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,   /* 50 - 5F P - Z */
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,   /* 60 - 6F a - o */
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1    /* 70 - 7F p - z */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* 80 - 8F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* 90 - 9F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* A0 - AF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* B0 - BF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* C0 - CF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* D0 - DF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* E0 - EF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* F0 - FF */
};
/*
 * Convert from base64 to base 256.
 *
 * @param from  The input string in base64 which must be a multiple of 4 bytes
 * @param to    The output string which must be large enough to hold the result.
 *              This is commonly allocated by using a length equal to the input length.
 * @param fromlen the length of the from string.
 * @return The size of the output, or a negative value to indicate an error
 *
 */
XAPI int ism_common_fromBase64(char * from, char * to, int fromlen) {
    int i;
    int j = 0;
    int value = 0;
    int bits = 0;
    //int fromlen = (int)strlen(from);
    if (fromlen%4 != 0) {
        return -2;
    }
    for (i=0; i<fromlen; i++) {
        int ch = (int)(uint8_t)from[i];
        int bval = b64val[ch];
        if (ch == '=' && i+2 < fromlen)
            bval = -1;
        if (bval >= 0) {
            value = value<<6 | bval;
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                to[j++] = (char)((value >> bits) & 0xff);
            }
        } else {
            if (bval == -1)
                return -1;
        }
    }
    if (from[fromlen-1]=='=')
        j--;
    if (from[fromlen-2]=='=')
        j--;
    to[j] = 0;
    return j;
}


/*
 * Convert a float or double to ASCII based on the locale
 * @param locale  the locale
 * @param flt     The float value (this is passed as a double in C)
 * @param buf     The output buffer.  This should be at least 32 bytes long.
 * @return The total buffer size needed; if greater than size of buf, the output was truncated.
 */
int32_t ism_common_formatDouble(const char * locale, double flt, char * buf) {
  UNumberFormat *fmt;
    UErrorCode status = U_ZERO_ERROR;
    int32_t needed;
    UChar result[256];
    char * outp = buf;

  /* Create a formatter for the input locale*/
    fmt = unum_open(
          UNUM_DECIMAL,      /* style         */
          0,                 /* pattern       */
          0,                 /* patternLength */
          locale,           /* locale        */
          0,                 /* parseErr      */
          &status);

    if (U_FAILURE(status)) {
        TRACE(6, "Failed to open the formatter. StatusCode: %d.\n", status);
        unum_close(fmt);
        return 0;
    }

    needed = unum_formatDouble(fmt, flt, result, 256, NULL, &status);

    if (U_FAILURE(status)) {
        TRACE(6, "Failed to parse the number. StatusCode: %d.\n", status);
        unum_close(fmt);
        return 0;
    }

    u_UCharsToChars(result, outp, needed);

    /* Release the storage used by the formatter */
    unum_close(fmt);

    return needed;
}

/*
 * Format 64 bit integer to ASCII based on the locale.
 * @param lval Long (64) bit integer value
 * @param buf  The output buffer.  This should be at least 24 bytes long
 * @return The total buffer size needed; if greater than length of buf, the output was truncated.
 */
int32_t ism_common_formatInt64(const char * locale, int64_t lval, char * buf) {
  UNumberFormat *fmt;
    UErrorCode status = U_ZERO_ERROR;
    int32_t needed;
    UChar result[256];
    char * outp = buf;

    if (outp == NULL){
      return 0;
    }

  /* Create a formatter for the input locale*/
    fmt = unum_open(
          UNUM_DECIMAL,      /* style         */
          0,                 /* pattern       */
          0,                 /* patternLength */
          locale,           /* locale        */
          0,                 /* parseErr      */
          &status);

    if (U_FAILURE(status)) {
        TRACE(6, "Failed to open the formatter. StatusCode: %d.\n", status);
        unum_close(fmt);
        return 0;

    }

    needed =  unum_formatInt64(fmt, lval, result, 256, NULL, &status);

    if (U_FAILURE(status)) {
        TRACE(6, "Failed to parse the number. StatusCode: %d.\n", status);
        unum_close(fmt);
        return 0;
    }

    int len = strlen(buf);
  if (needed >= len) {
    u_UCharsToChars(result, outp, needed);
  }else{
    u_UCharsToChars(result, outp, len);
  }

    /* Release the storage used by the formatter */
    unum_close(fmt);

    return needed;
}


/*
 * Format an integer to ASCII based on the locale/
 * @param ival Integer value to convert
 * @parm  buf  The output buffer.  This should be at least 12 bytes long
 * @return The total buffer size needed; if greater than length of buf, the output was truncated.
 */
int32_t ism_common_formatInt(const char * locale, int32_t ival, char * buf) {
  UNumberFormat *fmt;
    UErrorCode status = U_ZERO_ERROR;
    int32_t needed;
    UChar result[256];
    char * outp = buf;

    if (outp==NULL) {
      return 0;
    }

  /* Create a formatter for the input locale*/
    fmt = unum_open(
          UNUM_DECIMAL,      /* style         */
          0,                 /* pattern       */
          0,                 /* patternLength */
          locale,           /* locale        */
          0,                 /* parseErr      */
          &status);

    if (U_FAILURE(status)) {
        TRACE(6, "Failed to open the formatter. StatusCode: %d.\n", status);
        return 0;
    }

    needed =  unum_format(fmt, ival, result, 256, NULL, &status);

    if (U_FAILURE(status)) {
        TRACE(6, "Failed to parse the number. StatusCode: %d.\n", status);
        return 0;
    }

   int len = strlen(buf);
  if (needed>=len) {
    u_UCharsToChars(result, outp, needed);
  } else {
    u_UCharsToChars(result, outp, len);
  }

    /* Release the storage used by the formatter */
    unum_close(fmt);

    return needed;
}

/*
 * Format an unformatted number from a string to the decimal and based on the locale.
 * @param locale  the locale
 * @param number  The number to format.
 * @param length  The length of the input number, or -1 if the input is nul-terminated.
 * @param buf     The output buffer.  This should be at least 32 bytes long.
 * @return The total buffer size needed; if greater than length of buf, the output was truncated.
 */
int32_t ism_common_formatDecimal(const char * locale, const char * number, int32_t length, char * buf) {
    UNumberFormat *fmt;
    UErrorCode status = U_ZERO_ERROR;
    int32_t needed;
    UChar result[256];
    char * outp = buf;

    if (outp == NULL) {
        return 0;
    }

  /* Create a formatter for the input locale*/
    fmt = unum_open(
          UNUM_DECIMAL,      /* style         */
          0,                 /* pattern       */
          0,                 /* patternLength */
          locale,            /* locale        */
          0,                 /* parseErr      */
          &status);

    if (U_FAILURE(status)) {
        TRACE(6, "Failed to open the formatter. StatusCode: %d.\n", status);
        unum_close(fmt);
        return 0;
    }

    needed = unum_formatDecimal (fmt, number, length, result, 256, NULL, &status);

    if (U_FAILURE(status)) {
        TRACE(6, "Failed to parse the number. StatusCode: %d.\n", status);
        unum_close(fmt);
        return 0;
    }

    int len = strlen(buf);
    if (needed>=len) {
        u_UCharsToChars(result, outp, needed);
    } else {
        u_UCharsToChars(result, outp, len);
    }

    /* Release the storage used by the formatter */
    unum_close(fmt);

    return needed;
}


/**
 * Get the Last Error replace data if any.
 * @param inbuf the concat_alloc_t object which contain the buffer of the replacement data
 * @return the size of the replace data
 */
int ism_common_getLastErrorReplData(concat_alloc_t * inbuf) {
    struct ism_tls_t * tls;
    int    count=0;
    char * replData=NULL;
    int    replDataLen=0;

    tls = getErrorData(0);
    replData= (char *)(tls+1);
    replDataLen = tls->data_len;
    count = tls->count;
    if (count > 64)
        count = 64;

    if (count>0){
        /*Copy the buffer.*/
        ism_common_allocBufferCopyLen(inbuf,replData,replDataLen);
    }

    return count;
}

int ism_common_readFile(const char * fileName, char ** pBuff, int *pLen) {
    FILE * f = fopen(fileName, "rb");
    int len = 0;
    if (!f) {
        return ISMRC_Error;
    }
    /* Get file length */
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    rewind(f);

    if (pBuff && len) {
        /*
         * Read the file
         */
        int n;
        char *buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,201),len+2);
        if (!buf) {
            fclose(f);
            return ISMRC_AllocateError;
        }
        n = fread(buf, 1, len, f);
        if (n != len) {
            ism_common_free(ism_memory_utils_misc,buf);
            fclose(f);
            return ISMRC_Error;
        }
        buf[n] = 0;
        *pBuff = buf;
    }
    fclose(f);
    if(pLen)
        *pLen = len;
    return ISMRC_OK;
}

static uint64_t g_uuid_node;

/*
 * Initialize the UUID to a MAC address from this system
 */
void ism_common_initUUID(void) {
    uint64_t rbuf = 0;

    struct ifaddrs * ifap;
    struct ifaddrs * p;
    struct sockaddr_ll * sll;
    int rc;

    rc = getifaddrs(&ifap);
    if (!rc) {
        for (p=ifap; p; p=p->ifa_next) {
            if (p->ifa_addr->sa_family == AF_PACKET) {
                sll = (struct sockaddr_ll *) p->ifa_addr;
                if (sll->sll_addr[0] && sll->sll_halen == 6) {
                    memcpy(((char *)&rbuf)+2, sll->sll_addr, 6);
                    break;
                }
            }
        }
        freeifaddrs(ifap);
    } else {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        RAND_pseudo_bytes(((uint8_t *)&rbuf)+2, 6);
#else
        RAND_bytes(((uint8_t *)&rbuf)+2, 6);
#endif
    }
    g_uuid_node = endian_int64(rbuf);

    TRACE(6, "Initialize UUID: host=%012lx\n", g_uuid_node);
}

/*
 * Put out a group of 4 hex digits
 */
static char * hexd = "0123456789abcdef";
static void hexdigs(char * buf, uint32_t val) {
    buf[0] = hexd[(val>>12)&0xf];
    buf[1] = hexd[(val>>8)&0xf];
    buf[2] = hexd[(val>>4)&0xf];
    buf[3] = hexd[val&0xf];
}

/*
 * Make a UUID
 */
const char * ism_common_newUUID(char * buf, int len, int type, ism_time_t time) {
    static int g_uuid_threadid = 0;
    if (len<37 && len != 16)
        return NULL;
    uint32_t uuid[4];
    if (type == 1 || type == 17) {
        uint32_t seq;
        ism_tls_t * tls = (ism_tls_t *) ism_common_getThreadKey(ism_threadKey, NULL);
        if (__builtin_expect(!tls, 0))                 /* Should not happen for real threads */
            tls = makeTLS(512, NULL);
        /* On first use of UUID in this thread, set up the thread local storage */
        if (tls->uuid_seq == 0) {
            uint64_t rbuf;
            uint8_t sbuf [2];
#if OPENSSL_VERSION_NUMBER < 0x10100000L
            RAND_pseudo_bytes(sbuf, 2);
#else
            RAND_bytes(sbuf, 2);
#endif
            seq = (((int)sbuf[0]<<8) | sbuf[1]);
            seq <<= 7;
            seq |= __sync_add_and_fetch(&g_uuid_threadid, 1);
            tls->uuid_seq = seq;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
            RAND_pseudo_bytes(((uint8_t *)&rbuf)+2, 6);
#else
            RAND_bytes(((uint8_t *)&rbuf)+2, 6);
#endif
            tls->uuid_rand = endian_int64(rbuf);
        } else {
            seq = (tls->uuid_seq += 128);
        }
        uint64_t node;
        if (type==1) {           /* Use MAC as node */
            node = g_uuid_node;
        } else {                 /* Use thread random as node */
            node = tls->uuid_rand;
            seq >>= 7;           /* Use full sequence number */
        }
        uint64_t utime = time ? ism_common_convertTimeToUTime(time) : ism_common_getUUIDtime();
        uuid[0] = (uint32_t)utime;
        uuid[1] = (uint32_t)(utime>>16) & 0xffff0000;
        uuid[1] |= (((uint32_t)(utime>>48)) & 0x0fff);
        uuid[1] |= 0x00001000;    /* Set version */
        uuid[2] = ((seq<<16)&0x3FFF0000) | (((uint32_t)(node>>32))&0x0000FFFF);
        uuid[2] |= 0x80000000;    /* Set variation */
        uuid[3] = (uint32_t)node;
    } else if (type == 4) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        RAND_pseudo_bytes((uint8_t *)uuid, 16);
#else
        RAND_bytes((uint8_t *)uuid, 16);
#endif
        uuid[1] &= 0xffff0fff;
        uuid[1] |= 0x00004000;    /* Set version */
        uuid[2] &= 0x3fffffff;
        uuid[2] |= 0x80000000;    /* Set variation */
    } else {
        return NULL;
    }

    if (len == 16) {
        uuid[0] = endian_int32(uuid[0]);
        uuid[1] = endian_int32(uuid[1]);
        uuid[2] = endian_int32(uuid[2]);
        uuid[3] = endian_int32(uuid[3]);
        memcpy(buf, (char *)uuid, 16);
    } else {
        memcpy(buf, "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx", 37);
        hexdigs(buf+0, uuid[0]>>16);
        hexdigs(buf+4, uuid[0]);
        hexdigs(buf+9, uuid[1]>>16);
        hexdigs(buf+14, uuid[1]);
        hexdigs(buf+19, uuid[2]>>16);
        hexdigs(buf+24, uuid[2]);
        hexdigs(buf+28, uuid[3]>>16);
        hexdigs(buf+32, uuid[3]);
    }
    return buf;
}

/*
 * Extract the UUID time from a string form type 1 UUID.
 * @param uuid  The UUID
 * @return the timestamp, or 0 if the UUID is not type 1 or is invalid.
 */
uint64_t ism_common_extractUUIDtime(const char * uuid) {
    uint64_t val = 0;
    int   i;

    if (!uuid)
        return 0;
    i= 0;
    while (i<16 && *uuid) {
        if (*uuid != '-') {
            int dig = hexValue(*uuid);
            i++;
            if (dig < 0)
                return 0;
            val <<= 4;
            val |= dig;
        }
        uuid++;
    }
    if (i != 16)    /* Found 16 hex digits */
        return 0;

    if ((val&0xf000) != 0x1000)   /* Check for type 1 */
        return 0;

    uint64_t ret = ((val>>32) & 0xffffffff) | ((val&0xffff0000)<<16) | ((val&0xfff)<<48);
    return ret;
}

/*
 * Convert a binary UUID to a string UUID
 */
const char * ism_common_UUIDtoString(const char * uuid_bin, char * buf, int len) {
    if (!uuid_bin  || len<37 || !buf)
        return NULL;
    uint32_t uuid[4];
    memcpy((char *)uuid, uuid_bin, 16);
    uuid[0] = endian_int32(uuid[0]);
    uuid[1] = endian_int32(uuid[1]);
    uuid[2] = endian_int32(uuid[2]);
    uuid[3] = endian_int32(uuid[3]);
    memcpy(buf, "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx", 37);
    hexdigs(buf+0, uuid[0]>>16);
    hexdigs(buf+4, uuid[0]);
    hexdigs(buf+9, uuid[1]>>16);
    hexdigs(buf+14, uuid[1]);
    hexdigs(buf+19, uuid[2]>>16);
    hexdigs(buf+24, uuid[2]);
    hexdigs(buf+28, uuid[3]>>16);
    hexdigs(buf+32, uuid[3]);
    return buf;
}

/*
 * Convert a string UUID to binary
 * This code just looks for 32 hex digits and allows any number of hyphen separators
 * @param uuid_str  The string format of the UUID
 * @param buf       The return buffer.  This must be at least 16 bytes
 * @return the buffer with the binary UUID or NULL to indicate an error
 */
const char * ism_common_UUIDtoBinary(const char * uuid_str, char * buf) {
    if (!uuid_str)
        return NULL;
    uint32_t uuid [4];
    int i;
    int j;

    for (j=0; j<4; j++) {
        uint32_t val = 0;
        i = 0;
        while (i<8 && *uuid_str) {
            if (*uuid_str != '-') {
                int dig = hexValue(*uuid_str);
                i++;
                if (dig < 0)
                    return NULL;
                val <<= 4;
                val |= dig;
            }
            uuid_str++;
        }
        uuid[j] = val;
    }
    uuid[0] = endian_int32(uuid[0]);
    uuid[1] = endian_int32(uuid[1]);
    uuid[2] = endian_int32(uuid[2]);
    uuid[3] = endian_int32(uuid[3]);
    memcpy(buf, uuid, 16);
    return buf;
}


/* 
 * We use Unix Domain Sockets and therefore the "IP address" for a socket is sometimes a filename
 *
 * We have to be careful over the location of such files as some distributed filesystems
 * that might be used for our datadir (CephFS) can't be used for them.
 * Therefore we allow the special string ${IMASERVER_RUNTIME_DIR} at the start of the string
 * and replace it with a suitable temporary directory here. 
 *
 * @return: On success returns number of replacements (0 or 1). On failure returns -1
 */
int ism_common_expandUDSPathVars(char *expandedString, int maxSize, const char *inString) {
    int rc = 0;

    if (strncmp(inString, "${IMASERVER_RUNTIME_DIR}", strlen("${IMASERVER_RUNTIME_DIR}")) == 0) {
    
        //Check the string (including terminating NULL) fits in the target buffer
        size_t newsize =  strlen(inString)
                    + strlen(ism_common_getRuntimeTempDir())
                    - strlen("${IMASERVER_RUNTIME_DIR}")
                    + 1;
    
        if (newsize > maxSize) {
            TRACE(4, "%s: ERROR: inString %s, PathLen %lu too long!\n", __FUNCTION__, inString, newsize);
            return -1;
        }
    
        //Replace ${IMASERVER_RUNTIME_DIR} with the actual temporary dir
        snprintf(expandedString, newsize, "%s%s", 
                 ism_common_getRuntimeTempDir(),
                 inString+strlen("${IMASERVER_RUNTIME_DIR}"));

        TRACE(9, "%s: Before %s, after %s\n", __FUNCTION__, inString, expandedString);
        rc = 1;
    } else {
        TRACE(9, "%s: No replacement vars in  %s\n", __FUNCTION__, inString);
    }
    return rc;
}
