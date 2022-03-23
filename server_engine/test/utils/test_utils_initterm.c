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

//****************************************************************************
/// @file  test_utils_initterm.c
/// @brief Common initialization and termination routines for use by tests
//****************************************************************************
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ftw.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <ctype.h>

#include "engine.h"
#include "engineInternal.h"
#include "store.h"
#include "admin.h"
#include "selector.h"
#include "cluster.h"

#include "test_utils_initterm.h"
#include "test_utils_ismlogs.h"
#include "test_utils_file.h"
#include "test_utils_security.h"

extern const char *__progname;
typedef struct tag_envVars_t
{
    char *name;
    char *description;
    char *configName;
    char *propValue;
    ism_ConfigComponentType_t configtype;
} test_envVars_t;

test_engineInit_betweenInitAndStartFunc_t test_engineInit_betweenInitAndStartFunc = NULL;

test_envVars_t test_envVars[] = {
/*00*/  {"IMA_TEST_TRACE_LEVEL", "Set tracing level", "TraceLevel", NULL, ISM_CONFIG_COMP_LAST},
/*01*/  {"IMA_TEST_TRACE_OPTIONS", "Set tracing options", "TraceOptions", NULL, ISM_CONFIG_COMP_LAST},
/*02*/  {"IMA_TEST_TRACE_FILTER", "Set tracing filter", "TraceFilter", NULL, ISM_CONFIG_COMP_LAST},
/*03*/  {"IMA_TEST_TRACE_FILE", "Set the trace file", "TraceFile", NULL, ISM_CONFIG_COMP_LAST},
/*04*/  {"IMA_TEST_TRACE_MAX", "Set the trace maximum file size", "TraceMax", NULL, ISM_CONFIG_COMP_LAST},
/*05*/  {"IMA_TEST_USE_SPINLOCKS", "Force the use of spin locks by the engine", ismENGINE_CFGPROP_USE_SPIN_LOCKS, NULL, ISM_CONFIG_COMP_LAST},
/*06*/  {"IMA_TEST_ENABLE_CLUSTER", "Enable the cluster component during unit test", ismCLUSTER_CFG_ENABLECLUSTER, NULL, ISM_CONFIG_COMP_CLUSTER},
/*07*/  {"IMA_TEST_CLUSTER_CONTROLADDRESS", "Set the cluster control address", ismCLUSTER_CFG_CONTROLADDR, NULL, ISM_CONFIG_COMP_CLUSTER},
/*08*/  {"IMA_TEST_CLUSTER_CLUSTERNAME", "Set the cluster name", ismCLUSTER_CFG_CLUSTERNAME, NULL, ISM_CONFIG_COMP_CLUSTER},
/*09*/  {"IMA_TEST_CLUSTER_SERVERNAME", "Set the cluster server name", "Cluster.ServerName", NULL, ISM_CONFIG_COMP_CLUSTER},
/*10*/  {"IMA_TEST_CLUSTER_LOCALSERVERUID", "Set the cluster local server UID", "Cluster.LocalServerUID", NULL, ISM_CONFIG_COMP_CLUSTER},
/*11*/  {"IMA_TEST_CLUSTER_TESTSTANDALONE", "Tell the cluster to run in test stand-alone mode", ismCLUSTER_CFG_TESTSTANDALONE, NULL, ISM_CONFIG_COMP_CLUSTER},
/*12*/  {"QUALIFY_SHARED", "Qualify the shared memory name used by the store", "Store.SharedMemoryName", NULL, ISM_CONFIG_COMP_LAST},
/*13*/  {"STOREROOT", "Override the store's disk root path", "Store.DiskRootPath", NULL, ISM_CONFIG_COMP_LAST},
/*14*/  {"IMA_TEST_TEMP_PATH", "Set the directory to use for temporary files (def: /tmp)", NULL, NULL, ISM_CONFIG_COMP_LAST},
/*15*/  {"IMA_TEST_STORE_DISK_PERSIST", "Set whether disk persistence should be overridden", ismSTORE_CFG_DISK_ENABLEPERSIST, NULL, ISM_CONFIG_COMP_STORE},
/*16*/  {"IMA_TEST_USE_RECOVERY_METHOD", "Set which engine/store recovery method to use", ismENGINE_CFGPROP_USE_RECOVERY_METHOD, NULL, ISM_CONFIG_COMP_LAST},
/*17*/  {"IMA_TEST_FAKE_ASYNC_CALLBACK_CAPACITY", "Set the capacity to use for the engine's fake async CBQ", ismENGINE_CFGPROP_FAKE_ASYNC_CALLBACK_CAPACITY, NULL, ISM_CONFIG_COMP_LAST},
/*18*/  {"IMA_TEST_DISABLE_THREAD_JOB_QUEUES", "Force disable thread job queues by the engine", ismENGINE_CFGPROP_DISABLE_THREAD_JOB_QUEUES, NULL, ISM_CONFIG_COMP_LAST},
/*19*/  {"IMA_TEST_TRACE_BACKUP", "Set the trace backup setting", "TraceBackup", NULL, ISM_CONFIG_COMP_SERVER},
/*20*/  {"IMA_TEST_TRACE_BACKUP_COUNT", "Set the trace backup count", "TraceBackupCount", NULL, ISM_CONFIG_COMP_SERVER},
/*21*/  {"IMA_TEST_TRANSACTION_THREAD_JOBS_CLIENT_MINIMUM", "Force the value of TransactionThreadJobsClientMinimum", ismENGINE_CFGPROP_TRANSACTION_THREADJOBS_CLIENT_MINIMUM, NULL, ISM_CONFIG_COMP_LAST},
/*22*/  {"IMA_TEST_RESOURCESET_CLIENTID_REGEX", "Set a ClientID regular expression to use for resource sets", ismENGINE_CFGPROP_RESOURCESETDESCRIPTOR_DEFAULT_CLIENTID, NULL, ISM_CONFIG_COMP_LAST},
/*23*/  {"IMA_TEST_RESOURCESET_TOPICSTRING_REGEX", "Set a TopicString regular expression to use for resource sets", ismENGINE_CFGPROP_RESOURCESETDESCRIPTOR_DEFAULT_TOPIC, NULL, ISM_CONFIG_COMP_LAST},
/*24*/  {"IMA_TEST_RESOURCESET_MEMTRACE_SETID", "Wildcard SetId to track memory allocations and frees for", ismENGINE_CFGPROP_RESOURCESET_STATS_MEMTRACE_SETID, NULL, ISM_CONFIG_COMP_LAST},
/*25*/  {"IMA_TEST_RESOURCESET_TRACK_UNMATCHED", "Whether to track resources that don't match regular expressions", ismENGINE_CFGPROP_RESOURCESET_STATS_TRACK_UNMATCHED, NULL, ISM_CONFIG_COMP_LAST},
/*26*/  {"IMA_TEST_STORE_MEMTYPE", "Set which MemoryType to use for the store", ismSTORE_CFG_MEMTYPE, NULL, ISM_CONFIG_COMP_LAST},
/*27*/  {"IMA_TEST_RESOURCESET_STATS_MINUTES_PAST_HOUR", "At how many minutes past the hour a reporting cycle should start", ismENGINE_CFGPROP_RESOURCESET_STATS_MINUTES_PAST_HOUR, NULL, ISM_CONFIG_COMP_LAST},
/*28*/  {"IMA_TEST_PROTOCOL_ALLOW_MQTT_PROXY", "Whether to allow MQTT proxy protocol (means env is IoT)", ismENGINE_CFGPROP_ENVIRONMENT_IS_IOT, NULL, ISM_CONFIG_COMP_LAST},
/*29*/  {"IMA_TEST_THREAD_JOB_QUEUES_ALGORITHM", "How much work to put on JobQueues/How aggressive is scavenger", ismENGINE_CFGPROP_THREAD_JOB_QUEUES_ALGORITHM, NULL, ISM_CONFIG_COMP_LAST},
/*30*/  {"IMA_TEST_ASYNCCBSTATS_ENABLED", "Whether the store should track Async CBs", ismSTORE_CFG_ASYNCCB_STATSMODE},
/*31*/  {"IMA_TEST_SIZE_PROFILE", "Whether to specify an explicit size profiled or not", ismENGINE_CFGPROP_SIZE_PROFILE, NULL, ISM_CONFIG_COMP_LAST},
        {NULL, NULL, NULL, NULL, ISM_CONFIG_COMP_LAST}
};

// Forward declaration of a couple of functions we expect to call
extern void ism_adminHA_init_locks(void);
XAPI void ism_common_traceSignal(int signo);

//Global variable so we can easily tell if diskPersistence is enabled (set during engine start once we've read config)
bool usingDiskPersistence = false;

static bool      globalStoreInit = false;
static bool      globalClusterInit = false;
static int       globalAdminInit = 0;
static bool      globalDisableAuthorization = false;
static bool      globalDisableAutoQueueCreation = ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION;
static bool      globalTempAdminDir = false;
static uint32_t  globalSubListCacheCapacity = ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY;
static char     *globalAdminDir = NULL;
static char     *globalTempTraceFile = NULL;
static int       globalTempTraceFd = -1;
static char     *globalTraceProcessScript = NULL;

static void     *libismprotocol_hdl = NULL;

static ism_config_t *fakeSecurityConfigCallbackHandle = NULL;

static void (* memoryDumpStatsFn)(const char *) = NULL;

//****************************************************************************
/// @brief  Dummy ism_common_initPlatformDataFile for unit test.
//****************************************************************************
XAPI int ism_common_initPlatformDataFile(void)
{
    // Get a pointer to the real function in case we ever want to call it.
    static int (*real_ism_common_initPlatformDataFile)(void) = NULL;

    if (!real_ism_common_initPlatformDataFile)
    {
        real_ism_common_initPlatformDataFile = dlsym(RTLD_NEXT, "ism_common_initPlatformDataFile");
    }

    return ISMRC_OK;
}

//****************************************************************************
/// @brief Dummy ism_store_memValidateDiskSpace function to override return
///        code so that it doesn't fail for the unit tests.
//****************************************************************************
XAPI int32_t ism_store_memValidateDiskSpace(void)
{
    return ISMRC_OK;
}

//****************************************************************************
/// @brief  Dummy ism_common_getServerName for unit test.
//****************************************************************************
const char * ism_common_getServerName(void)
{
    return test_envVars[9].propValue ? test_envVars[9].propValue : "(none)";
}

//****************************************************************************
/// @brief  Dummy ism_common_getServerUID for unit test.
//****************************************************************************
const char * ism_common_getServerUID(void)
{
    return test_envVars[10].propValue ? test_envVars[10].propValue : "UNITTEST";
}

//****************************************************************************
/// @brief  Get whatever the temporary path is set to.
//****************************************************************************
void test_utils_getTempPath(char *tempDirectory)
{
    if (test_envVars[14].propValue == NULL)
    {
        test_envVars[14].propValue = getenv(test_envVars[14].name);

        if (test_envVars[14].propValue == NULL) test_envVars[14].propValue = "/tmp";

        assert(test_envVars[14].propValue != NULL);
    }

    if (tempDirectory != NULL)
    {
        strcpy(tempDirectory, test_envVars[14].propValue);
    }
}

//****************************************************************************
/// @brief  Copy key configuration files from the config source directory to
///         the specified configuration directory also appending some special
///         configuration entries for the unit tests.
///
/// @param[in]     targetDir  Target directory for the configuration.
//****************************************************************************
void test_copyConfigFiles(char *targetDir)
{
    char *configFileSource = NULL;
    char *extraConfigFileSource = NULL;
    char *targetFile = NULL;

    // Work out where to copy config files from
    char *sroot = getenv("SROOT");
    char *prjDir = getenv("PRJDIR");
    char *rootRel = getenv("ROOTREL");

    if (prjDir && rootRel)
    {
        configFileSource = malloc(strlen(prjDir)+1+strlen(rootRel)+1+200);
        extraConfigFileSource = malloc(strlen(prjDir)+1+strlen(rootRel)+1+400);

        if (configFileSource != NULL) sprintf(configFileSource, "%s/%s/server_main/config/", prjDir, rootRel);
        if (extraConfigFileSource != NULL) sprintf(extraConfigFileSource,
                                                   "%s/%s/server_engine/test/utils/test_extra_server_dynamic.json",
                                                   prjDir, rootRel);
    }
    else if (sroot)
    {
        configFileSource = malloc(strlen(sroot)+1+200);
        extraConfigFileSource = malloc(strlen(sroot)+1+400);

        if (configFileSource != NULL) sprintf(configFileSource, "%s/server_main/config/", sroot);
        if (extraConfigFileSource != NULL) sprintf(extraConfigFileSource,
                                                   "%s/server_engine/test/utils/test_extra_server_dynamic.json",
                                                   sroot);
    }

    if (configFileSource == NULL)
    {
        printf("WARNING: Unable to determine configuration source directory.\n");
        goto mod_exit;
    }

    targetFile = malloc(strlen(targetDir)+1+200);

    if (targetFile == NULL)
    {
        printf("WARNING: Unable to allocate space for target file name.\n");
        goto mod_exit;
    }

    size_t configSourceDirLen = strlen(configFileSource);

    // Get a server.cfg file
    strcpy(&configFileSource[configSourceDirLen], "server.cfg");
    (void)test_copyFile(configFileSource, targetDir);

    // Get a server_dynamic.json file
    strcpy(&configFileSource[configSourceDirLen], "server_dynamic.json");

    char enableDiskPersistenceValue[50];
    if (test_envVars[15].propValue != NULL)
    {
        sprintf(enableDiskPersistenceValue, "\"EnableDiskPersistence\":%s", test_envVars[15].propValue);
    }
    else
    {
        sprintf(enableDiskPersistenceValue, "\"EnableDiskPersistence\":false");
    }

    char storeMemTypeValue[50];
    if (test_envVars[26].propValue != NULL)
    {
        sprintf(storeMemTypeValue, "\"MemoryType\": %s", test_envVars[26].propValue);
    }
    else
    {
        sprintf(storeMemTypeValue, "\"MemoryType\": null");
    }

    testUtilsReplace_t replacements[] = { { "ADMIN_PORT", "9089" },
                                          { "ADMIN_INTERFACE", "127.0.0.1" },
                                          { "SYSLOG_HOST", "127.0.0.1" },
                                          { "SYSLOG_PORT", "514" },
                                          { "SYSLOG_PROTOCOL", "tcp" },
                                          { "SYSLOG_ENABLED", "false" },
                                          { "\"EnableDiskPersistence\": true", enableDiskPersistenceValue },
                                          { "\"MemoryType\": null", storeMemTypeValue },
                                          { NULL, NULL } };

    (void)test_copyFileWithReplace(configFileSource, targetDir, replacements);
    if (extraConfigFileSource != NULL)
    {
        sprintf(targetFile, "%s/server_dynamic.json", targetDir);
        (void)test_copyFileAppend(extraConfigFileSource, targetFile);
    }

    // Copy the schema
    strcpy(&configFileSource[configSourceDirLen], "serverConfigSchema.json");
    (void)test_copyFile(configFileSource, targetDir);

    // Write a dummy platform.dat
    sprintf(targetFile, "%s/platform.dat", targetDir);
    FILE *fp = fopen(targetFile, "w");
    if (fp == NULL)
    {
        printf("WARNING: Could not create a platform.dat\n");
        goto mod_exit;
    }
    fprintf(fp, "PLATFORM_ISVM:0\nPLATFORM_TYPE:11\nPLATFORM_VEND:Unknown\nPLATFORM_LICE:Production\nPLATFORM_SNUM:13e32ee\n");
    fclose(fp);

mod_exit:

    if (NULL != configFileSource) free(configFileSource);
    if (NULL != extraConfigFileSource) free(extraConfigFileSource);
    if (NULL != targetFile) free(targetFile);
}

//****************************************************************************
/// @brief  Copy key diagnostic files from the config source directory to
///         the specified diag directory.
///
/// @param[in]     targetDir  Target directory for the diag.
//****************************************************************************
void test_copyDiagFiles(char *targetDir)
{
    char *scriptFileSource = NULL;
    char *targetFile = NULL;

    // Work out where to copy config files from
    char *sroot = getenv("SROOT");
    char *prjDir = getenv("PRJDIR");
    char *rootRel = getenv("ROOTREL");

    if (prjDir && rootRel)
    {
        scriptFileSource = malloc(strlen(prjDir)+1+strlen(rootRel)+1+200);

        if (scriptFileSource != NULL) sprintf(scriptFileSource, "%s/%s/server_main/scripts", prjDir, rootRel);
    }
    else if (sroot)
    {
        scriptFileSource = malloc(strlen(sroot)+1+200);

        if (scriptFileSource != NULL) sprintf(scriptFileSource, "%s/server_main/scripts/", sroot);
    }

    if (scriptFileSource == NULL)
    {
        printf("WARNING: Unable to determine script source directory.\n");
        goto mod_exit;
    }

    targetFile = malloc(strlen(targetDir)+1+200);

    if (targetFile == NULL)
    {
        printf("WARNING: Unable to allocate space for target file name.\n");
        goto mod_exit;
    }

    size_t scriptSourceDirLen = strlen(scriptFileSource);

    // Copy the processTrace script
    strcpy(&scriptFileSource[scriptSourceDirLen], "process_trace.sh");
    testUtilsReplace_t replacements[] = { {"/var/messagesight/diag", targetDir},
                                          {NULL, NULL} };
    (void)test_copyFileWithReplace(scriptFileSource, targetDir, replacements);
    sprintf(targetFile, "%s/process_trace.sh", targetDir);
    test_chmod(targetFile, "500");

    if (globalTraceProcessScript == NULL) globalTraceProcessScript = strdup(targetFile);

mod_exit:

    if (NULL != scriptFileSource) free(scriptFileSource);
    if (NULL != targetFile) free(targetFile);
}

//****************************************************************************
/// @brief  Temporary function used to initalize protocol library
//****************************************************************************
static void test_setup_protocol(void)
{
    ismEngine_MessageSelectionCallback_t selectionFn;

    libismprotocol_hdl = dlopen("libismprotocol.so", RTLD_LAZY | RTLD_GLOBAL);
    if (libismprotocol_hdl == NULL)
    {
        fprintf(stderr, "Failed to load protocol library: %s\n", dlerror());
    }
    else
    {
        // Setup the message selection callback.
        selectionFn = dlsym(libismprotocol_hdl, "ism_protocol_selectMessage");

        if (selectionFn == NULL)
        {
            fprintf(stderr, "Failed to lookup ism_protocol_selectMessage function: %s\n", dlerror());
        }
        else
        {
            ism_engine_registerSelectionCallback(selectionFn);
        }
    }

    return;
}

// DUMMY functions to satisfy the requirement for ism_transport_disableClientSet
// and ism_transport_enableClientSet functions in unit tests.
int test_dummy_ism_transport_disableClientSet(const char * regex_str, int rc)
{
    return OK;
}

int test_dummy_ism_transport_enableClientSet(const char * regex_str)
{
    return OK;
}

//****************************************************************************
/// @brief  A dummy cluster->protocol event callback function to satisfy the
///         cluster component.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_dummyClusterProtocolEventCallback(PROTOCOL_RS_EVENT_TYPE_t eventType,
                                               ismProtocol_RemoteServerHandle_t hRemoteServer,
                                               const char *pServerName,
                                               const char *pServerUID,
                                               const char *pRemoteServerAddress,
                                               int remoteServerPort,
                                               uint8_t fUseTLS,
                                               ismCluster_RemoteServerHandle_t hClusterHandle,
                                               ismEngine_RemoteServerHandle_t hEngineHandle,
                                               void *pContext,
                                               ismProtocol_RemoteServerHandle_t *phProtocolHandle)
{
    return OK;
}

//****************************************************************************
/// @brief  Update the config properties with the ones read during startup
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_setConfigProperties(bool dynamic)
{
    int32_t rc = OK;
    ism_field_t f;

    f.type = VT_String;

    for(int32_t i=0; test_envVars[i].name != NULL; i++)
    {
        if (test_envVars[i].configName == NULL) continue;

        if (dynamic)
        {
            if (test_envVars[i].configtype == ISM_CONFIG_COMP_LAST) continue;
        }
        else
        {
            // For now just make everything go in static config
            #if 0
            if (test_envVars[i].configtype != ISM_CONFIG_COMP_LAST) continue;
            #endif
        }

        if (test_envVars[i].propValue != NULL)
        {
            ism_prop_t *configProperties = NULL;

            if (dynamic)
            {
                ism_config_t *configHandle = ism_config_getHandle(test_envVars[i].configtype, NULL);

                if (configHandle != NULL)
                {
                    // Get the policy information using the security component's config handle
                    configProperties = ism_config_getPropertiesDynamic(configHandle, NULL, NULL);
                }
            }
            else
            {
                configProperties = ism_common_getConfigProperties();
            }

            if (configProperties == NULL) continue;

            f.val.s = test_envVars[i].propValue;
            rc = ism_common_setProperty(configProperties, test_envVars[i].configName, &f);
            if (rc != OK)
            {
                printf("ERROR: ism_common_setProperty() for %s='%s', returned %d\n",
                       test_envVars[i].configName, f.val.s, rc);
                break;
            }

            // printf("%d Setting: \"%s\"=\"%s\" [%d]\n", dynamic, test_envVars[i].configName, test_envVars[i].propValue, test_envVars[i].configtype);
        }
    }

    return rc;
}

//****************************************************************************
/// @brief  Dump memory stats before an event
///
/// @param[in]     filenamePrefix  (Optional) filename prefix
/// @param[in]     pDumpCount      Dump counter to include & increment
///
/// @remark If no prefix is specifed, /var/tmp/memoryDumpStats_before is used.
//****************************************************************************
void test_memoryDumpStats_before(const char *filenamePrefix, uint32_t *pDumpCount)
{
    if (memoryDumpStatsFn != NULL)
    {
        char memoryDumpFilename[256];
        *pDumpCount += 1;
        sprintf(memoryDumpFilename, "%s_%d.txt",
                filenamePrefix ? filenamePrefix : "/var/tmp/memoryDumpStats_before", *pDumpCount);
        memoryDumpStatsFn((const char *)memoryDumpFilename);
    }
}

//****************************************************************************
/// @brief  Dump memory stats after an event
///
/// @param[in]     filenamePrefix  (Optional) filename prefix
/// @param[in]     dumpCount       Dump counter to include
///
/// @remark If no prefix is specifed, /var/tmp/memoryDumpStats_after is used.
//****************************************************************************
void test_memoryDumpStats_after(const char *filenamePrefix, uint32_t dumpCount)
{
    if (memoryDumpStatsFn != NULL)
    {
        char memoryDumpFilename[256];
        sprintf(memoryDumpFilename, "%s_%d.txt",
                filenamePrefix ? filenamePrefix : "/var/tmp/memoryDumpStats_after", dumpCount);
        memoryDumpStatsFn((const char *)memoryDumpFilename);
    }
}

//****************************************************************************
/// @brief  Tell the test utils code that the global admin dir it is using is
/// a temporary one (probably passed from a previous phase)
///
/// @param[in]     adminDir        Used to confirm the directory name
//****************************************************************************
void test_process_setGlobalTempAdminDir(const char *adminDir)
{
    if (strcmp(adminDir, globalAdminDir) != 0) exit(110);
    globalTempAdminDir = true;
}

//****************************************************************************
/// @brief  Perform process wide initialization for a test program
///
/// Call the various initialization functions required to start an ISM test
/// program (emulating the start of an ISM server).
///
/// @param[in]     traceLevel  Initial trace level to set
/// @param[in]     adminDir    Override the 'admin' directory for this process.
///
/// @remark The adminDir parameter enables the test program to override the
///         default directory used by unit tests for configuration information,
///         policies directory and the writing of log files.
///
///         If the value is 'mkdtemp:<template>' a temporary directory is created
///         using mkdtemp("<template>_XXXXXX") and used - this is then deleted at
///         test_processTerm if the removeTempAdminDir flag is specified.
///
///         The default used by unit tests is 'mkdtemp:/tmp/<PROGNAME>'.
///
/// @remark The following environment variables, if specified, will change the
///         way in which the process initializes:
///
///         TraceOptions - String - Supply TraceOptions configuration property
///         TraceLevel   - Number - Override the trace level in configuration
///                                 properties.
///         TraceFile    - String - Path to the file to which trace should be
///                                 written. If this is the value 'TEMP' then a
///                                 temporary file is created using mktemp - this
///                                 file is then deleted in test_processTerm.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_processInit(int32_t traceLevel, char *adminDir)
{
    char *defaultAdminDir = NULL;
    char *tempFilename = NULL;

    int32_t rc = OK;

    // Have access to a memory dumping routine if it's available
    memoryDumpStatsFn = dlsym(RTLD_NEXT, "memoryDumpStats");

    ism_common_initUtil();

    // Seed random numbers for this process
    srand(ism_common_currentTimeNanos());

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();

    // Override the trace level, either permanently if TESTCFG_TRACE_LEVEL is specified,
    // or just during InitServer.
    test_envVars[0].propValue = getenv(test_envVars[0].name);
    if (NULL == test_envVars[0].propValue)
    {
        test_envVars[0].propValue = "0";
    }
    traceLevel = atoi(test_envVars[0].propValue);

    // Set the trace options
    test_envVars[1].propValue = getenv(test_envVars[1].name);
    if (NULL == test_envVars[1].propValue)
    {
        test_envVars[1].propValue = "thread,time,where";
    }

    // Set the trace filter
    test_envVars[2].propValue = getenv(test_envVars[2].name);

    // Allow the overriding of the trace file
    test_envVars[3].propValue = getenv(test_envVars[3].name);

    // Allow the overriding of the trace max
    test_envVars[4].propValue = getenv(test_envVars[4].name);
    if (NULL == test_envVars[4].propValue && NULL != globalTempTraceFile)
    {
        test_envVars[4].propValue = "10M";
    }

    // Allow overriding of the UseSpinLocks property
    test_envVars[5].propValue = getenv(test_envVars[5].name);
    if (NULL == test_envVars[5].propValue)
    {
        // If not overridden, 25% of the time use mutexes, 75% use spinlocks
        test_envVars[5].propValue = (rand()%100 > 75) ? "0" : "1";
    }

    // Allow enabling of the cluster component
    test_envVars[6].propValue = getenv(test_envVars[6].name);

    if (NULL != test_envVars[6].propValue)
    {
        // Set up a control address
        test_envVars[7].propValue = getenv(test_envVars[7].name);
        if (NULL == test_envVars[7].propValue)
        {
            test_envVars[7].propValue = "127.0.0.1";
        }

        // Allow override of cluster name
        test_envVars[8].propValue = getenv(test_envVars[8].name);
        if (test_envVars[8].propValue == NULL)
        {
            test_envVars[8].propValue = "TestClusterName";
        }

        // Allow override of server name
        test_envVars[9].propValue = getenv(test_envVars[9].name);
        if (test_envVars[9].propValue == NULL)
        {
            test_envVars[9].propValue = "TestServerName";
        }

        // Allow override of server UID
        test_envVars[10].propValue = getenv(test_envVars[10].name);
        if (test_envVars[10].propValue == NULL)
        {
            test_envVars[10].propValue = "TestLocalServerUID";
        }

        // Allow override of TestStandAlone
        test_envVars[11].propValue = getenv(test_envVars[11].name);
        if (test_envVars[11].propValue == NULL)
        {
            test_envVars[11].propValue = "True";
        }
    }

    // Qualify the shared memory name used by the store
    char *qualifyShared = getenv(test_envVars[12].name);

    // Allow the BUILD_ENGINE definition to override the shared memory name
    if (NULL == qualifyShared) qualifyShared = getenv("BUILD_ENGINE");

    if (NULL != qualifyShared)
    {
        char storeNameBuffer[strlen(qualifyShared)+20];
        sprintf(storeNameBuffer, "store%s", qualifyShared);
        test_envVars[12].propValue = strdup(storeNameBuffer);
    }
    else
    {
        test_envVars[12].propValue = NULL;
    }

    // Allow the test to override the directory to use for temporary data
    test_utils_getTempPath(NULL);
    assert(test_envVars[14].propValue != NULL);

    // Allow the test to override the store default root path
    test_envVars[13].propValue = getenv(test_envVars[13].name);
    if (NULL == test_envVars[13].propValue)
    {
        char storeRootBuffer[64];
        sprintf(storeRootBuffer, "%s/unittest_%u_store", test_envVars[14].propValue, getuid());
        if (qualifyShared)
        {
            strcat(storeRootBuffer, qualifyShared);
        }
        test_envVars[13].propValue = strdup(storeRootBuffer);
    }

    // Allow the test to override whether disk persistence is used or not
    test_envVars[15].propValue = getenv(test_envVars[15].name);

    // Allow the test to override which recovery method to use
    test_envVars[16].propValue = getenv(test_envVars[16].name);

    // Allow the test to override the fake async callback queue capacity
    test_envVars[17].propValue = getenv(test_envVars[17].name);

    // Allow disabling of per-thread job queues
    test_envVars[18].propValue = getenv(test_envVars[18].name);

    // Allow the overriding of the trace backup
    test_envVars[19].propValue = getenv(test_envVars[19].name);

    // Allow the overriding of the trace backup count
    test_envVars[20].propValue = getenv(test_envVars[20].name);

    // Allow overriding of the transaction thread job client property
    test_envVars[21].propValue = getenv(test_envVars[21].name);
    if (NULL == test_envVars[21].propValue)
    {
        // If not overridden, 50% of the time override it anyway to between 0 and 20...
        // we use an artificially low number to ensure that many tests will use the
        // thread jobs.
        if (rand()%100 > 50)
        {
            char buffer[20];
            sprintf(buffer, "%d", rand()%21);
            test_envVars[21].propValue = strdup(buffer);
        }
    }

    // Allow overriding of the ResourceSetStats clientId regex
    test_envVars[22].propValue = getenv(test_envVars[22].name);

    // Allow overriding of the ResourceSetStats topicString regex
    test_envVars[23].propValue = getenv(test_envVars[23].name);

    // Allow overriding of the ResourceSetStats memory trace setid (MEMDEBUG only)
    test_envVars[24].propValue = getenv(test_envVars[24].name);

    // Allow overriding of the ResourceSetStats tracking of unmatched resources
    test_envVars[25].propValue = getenv(test_envVars[25].name);

    // Allow the test to override the store MemoryType
    test_envVars[26].propValue = getenv(test_envVars[26].name);

    // Allow overriding minutes past hour for automatic reporting of stats
    test_envVars[27].propValue = getenv(test_envVars[27].name);

    // Allow overriding of the Allow MQTT Proxy protocol setting (indication of IoT environment)
    test_envVars[28].propValue = getenv(test_envVars[28].name);

    // Allow overriding of the Thread Jobs Algorithm
    test_envVars[29].propValue = getenv(test_envVars[29].name);

    // Allow overriding of the AsyncCB stats mode
    test_envVars[30].propValue = getenv(test_envVars[30].name);

    // Allow overriding of the SizeProfile setting
    test_envVars[31].propValue = getenv(test_envVars[31].name);

    rc = test_setConfigProperties(false);

    if (rc != OK) goto mod_exit;

    ism_field_t f;

    #if 1
    //ism_prop_t *engineConfigProps = staticConfigProps;
    //ism_prop_t *storeConfigProps = staticConfigProps;
    //ism_prop_t *clusterConfigProps = staticConfigProps;
    #else
    //ism_prop_t *engineConfigProps = ism_config_getComponentProps(ISM_CONFIG_COMP_ENGINE);
    //ism_prop_t *storeConfigProps = ism_config_getComponentProps(ISM_CONFIG_COMP_STORE);
    //ism_prop_t *clusterConfigProps = ism_config_getComponentProps(ISM_CONFIG_COMP_CLUSTER);
    #endif

    // Set Cache Flush to on. This shouldn't effect our unit tests
    f.type = VT_Integer;
    f.val.i = 1;
    rc = ism_common_setProperty(staticConfigProps, ismSTORE_CFG_CACHEFLUSH_MODE, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d, returned %d\n", ismSTORE_CFG_CACHEFLUSH_MODE, f.val.i, rc);
        goto mod_exit;
    }

    // Set clean shutdown to true so that we can valgrind our unit tests
    f.type = VT_Boolean;
    f.val.i = 1;
    rc = ism_common_setProperty(staticConfigProps, ismENGINE_CFGPROP_CLEAN_SHUTDOWN, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d, returned %d\n", ismENGINE_CFGPROP_CLEAN_SHUTDOWN, f.val.i, rc);
        goto mod_exit;
    }

    // If no admin directory was specified, use the default one
    if (NULL == adminDir)
    {
        defaultAdminDir = malloc(strlen(__progname)+strlen(test_envVars[14].propValue)+18);
        if (defaultAdminDir == NULL)
        {
            printf("ERROR: Could not allocat storage for default admin directory\n");
            rc = ISMRC_AllocateError;
            goto mod_exit;
        }
        sprintf(defaultAdminDir, "mkdtemp:%s/%s", test_envVars[14].propValue, __progname);
        adminDir = defaultAdminDir;
    }

    // Set up the admin directory
    globalAdminDir = strdup(adminDir);

    if (globalAdminDir == NULL)
    {
        printf("ERROR: Could not allocate storage for overridden config directory\n");
        rc = ISMRC_AllocateError;
        goto mod_exit;
    }

    tempFilename = malloc(strlen(globalAdminDir)+100);

    if (tempFilename == NULL)
    {
        printf("ERROR: Could not allocate storage for temporary file name\n");
        rc = ISMRC_AllocateError;
        goto mod_exit;
    }

    // Modify the policy directory to create a temporary name if requested
    if (strstr(adminDir, "mkdtemp:") == adminDir)
    {
        sprintf(globalAdminDir, "%s_XXXXXX", adminDir+strlen("mkdtemp:"));
        if (mkdtemp(globalAdminDir) == NULL)
        {
            printf("ERROR: mkdtemp() failed with errno %d\n", errno);
            rc = errno;
            goto mod_exit;
        }

        globalTempAdminDir = true;
    }
    else
    {
        globalTempAdminDir = false;
    }

    (void)mkdir(globalAdminDir, S_IRUSR | S_IWUSR | S_IXUSR);

    // Override the diag directory
    sprintf(tempFilename, "%s/diag", globalAdminDir);
    (void)mkdir(tempFilename, S_IRUSR | S_IWUSR | S_IXUSR);

    f.type = VT_String;
    f.val.s = tempFilename;
    rc = ism_common_setProperty(staticConfigProps, "DiagDir", &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for DiagDir returned %d\n", rc);
        goto mod_exit;
    }

    // test_copyDiagFiles(tempFilename);

    // Override the data directory
    sprintf(tempFilename, "%s/data", globalAdminDir);
    (void)mkdir(tempFilename, S_IRUSR | S_IWUSR | S_IXUSR);

    f.type = VT_String;
    f.val.s = tempFilename;
    rc = ism_common_setProperty(staticConfigProps, "ExportDir", &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for ExportDir returned %d\n", rc);
        goto mod_exit;
    }

    // Override the export directory
    sprintf(tempFilename, "%s/data/export", globalAdminDir);
    (void)mkdir(tempFilename, S_IRUSR | S_IWUSR | S_IXUSR);

    f.type = VT_String;
    f.val.s = tempFilename;
    rc = ism_common_setProperty(staticConfigProps, "ExportDir", &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for ExportDir returned %d\n", rc);
        goto mod_exit;
    }

    // Override the import directory
    sprintf(tempFilename, "%s/data/import", globalAdminDir);
    (void)mkdir(tempFilename, S_IRUSR | S_IWUSR | S_IXUSR);

    f.type = VT_String;
    f.val.s = tempFilename;
    rc = ism_common_setProperty(staticConfigProps, "ImportDir", &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for ImportDir returned %d\n", rc);
        goto mod_exit;
    }

    // Override the config directory
    sprintf(tempFilename, "%s/data/config", globalAdminDir);
    (void)mkdir(tempFilename, S_IRUSR | S_IWUSR | S_IXUSR);

    f.type = VT_String;
    f.val.s = tempFilename;
    rc = ism_common_setProperty(staticConfigProps, "ConfigDir", &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for ConfigDir returned %d\n", rc);
        goto mod_exit;
    }

    // Create some default config files if using a temp directory
    if (globalTempAdminDir) test_copyConfigFiles(tempFilename);

    // Override the security policies directory
    sprintf(tempFilename, "%s/data/policies", globalAdminDir);
    (void)mkdir(tempFilename, S_IRUSR | S_IWUSR | S_IXUSR);

    f.type = VT_String;
    f.val.s = tempFilename;
    rc = ism_common_setProperty(staticConfigProps, "PolicyDir", &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for PolicyDir returned %d\n", rc);
        goto mod_exit;
    }

    // Indicate that we want to grab the log files
    f.type = VT_Integer;
    f.val.i = 1;
    rc = ism_common_setProperty(staticConfigProps, "LogUnitTest", &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for LogUnitTest returned %d\n", rc);
        goto mod_exit;
    }

    // Override the location of the log files
    sprintf(tempFilename, "%s/diag/logs", globalAdminDir);
    (void)mkdir(tempFilename, S_IRUSR | S_IWUSR | S_IXUSR);

    rc = test_setLogDir(tempFilename);
    if (rc != OK) goto mod_exit;
    rc = test_relocateLogFile("System", "Max");
    if (rc != OK) goto mod_exit;
    rc = test_relocateLogFile("Connection", "Max");
    if (rc != OK) goto mod_exit;
    rc = test_relocateLogFile("Security", "Max");
    if (rc != OK) goto mod_exit;
    rc = test_relocateLogFile("Admin", "Max");
    if (rc != OK) goto mod_exit;
    rc = test_relocateLogFile("MQConnectivity", "Max");
    if (rc != OK) goto mod_exit;

    sprintf(tempFilename, "%s/diag/logs/Cluster.log", globalAdminDir);
    f.type = VT_String;
    f.val.s = tempFilename;

    rc = ism_common_setProperty(staticConfigProps, ismCLUSTER_CFG_LOGFILE, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s='%s' returned %d\n",
               ismCLUSTER_CFG_LOGFILE, f.val.s, rc);
        goto mod_exit;
    }

    f.type = VT_Integer;
    f.val.i = traceLevel;
    rc = ism_common_setProperty(staticConfigProps, ismCLUSTER_CFG_LOGLEVEL, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s='%d' returned %d\n",
               ismCLUSTER_CFG_LOGLEVEL, f.val.i, rc);
        goto mod_exit;
    }

    rc = ism_common_setProperty(staticConfigProps, "TraceLevel", &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s='%d' returned %d\n",
               "TraceLevel", f.val.i, rc);
        goto mod_exit;
    }

    // Set up function pointer to our dummy ism_transport_disableClientSet (this
    // emulates code in main.c)
    f.type = VT_Long;
    f.val.l = (uint64_t)(uintptr_t)test_dummy_ism_transport_disableClientSet;
    rc = ism_common_setProperty(staticConfigProps, ismENGINE_CFGPROP_DISABLECLIENTSET_FNPTR, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s='%p' returned %d\n",
               "_ism_transport_disableClientSet_fnptr", (void *)f.val.l, rc);
        goto mod_exit;
    }

    // Set up function pointer to our dummy ism_transport_enableClientSet (this
    // emulates code in main.c)
    f.val.l = (uint64_t)(uintptr_t)test_dummy_ism_transport_enableClientSet;
    rc = ism_common_setProperty(staticConfigProps, ismENGINE_CFGPROP_ENABLECLIENTSET_FNPTR, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s='%p' returned %d\n",
               "_ism_transport_disableClientSet_fnptr", (void *)f.val.l, rc);
        goto mod_exit;
    }

    rc = ism_common_initServer();

    // Change the trace level to the requested level explicitly
    ism_common_setTraceLevel(traceLevel);

    if (test_envVars[4].propValue != NULL)
        ism_common_setTraceMax(strtoul(test_envVars[4].propValue,NULL,0));
    if (test_envVars[19].propValue != NULL)
        ism_common_setTraceBackup(strtod(test_envVars[19].propValue, NULL));
    if (test_envVars[20].propValue != NULL)
        ism_common_setTraceBackupCount(strtod(test_envVars[20].propValue, NULL));

    // In order to create a durable subscriber with properties I need
    // the content filtering support to be initialized so I need to load
    // and setup the protocol library.
    test_setup_protocol();

mod_exit:

    if (tempFilename != NULL) free(tempFilename);

    if (defaultAdminDir != NULL) free(defaultAdminDir);

    if (rc != OK) abort();

    return OK;
}

//****************************************************************************
/// @brief  Return a copy of the global admin directory if there is one.
///
/// @param[in,out]  buffer      Buffer to fill in.
/// @param[in]      bufferLen   Size of the passed in buffer.
///
/// @return true if an admin dir was found
//****************************************************************************
bool test_getGlobalAdminDir(char *buffer, size_t bufferLen)
{
    if (globalAdminDir != NULL)
    {
        if (strlen(globalAdminDir)+1 > bufferLen) abort();
        strcpy(buffer, globalAdminDir);
        return true;
    }

    return false;
}

int test_removeTempCallback(const char *fpath,
                            const struct stat *sb,
                            int typeflag,
                            struct FTW *ftwbuf)
{
    return remove(fpath);
}

//****************************************************************************
/// @brief  Remove the contents of the specified directory and all sub-directories
///
/// @param[in]  adminDir  The admin directory to remove.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void test_removeAdminDir(char *adminDir)
{
    nftw(adminDir,
         test_removeTempCallback,
         64,
         FTW_DEPTH | FTW_PHYS);
}

//****************************************************************************
/// @brief  Perform process wide termination processing for a test program
///
/// @param[in]  removeTempAdmin  If the test created a temporary admin directory,
///                              whether to remove it.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_processTerm(bool removeTempAdmin)
{
    if (globalTempAdminDir && globalAdminDir != NULL)
    {
        if (removeTempAdmin) test_removeAdminDir(globalAdminDir);

        free(globalAdminDir);
        globalAdminDir = NULL;
    }

    if (globalTempTraceFile)
    {
        if (globalTempTraceFd != -1)
        {
            close(globalTempTraceFd);
            globalTempTraceFd = -1;
        }
        unlink(globalTempTraceFile);
        strcat(globalTempTraceFile, "_prev");
        unlink(globalTempTraceFile);
        free(globalTempTraceFile);
        globalTempTraceFile = NULL;
    }

    if (ismLogMsgsSeen != NULL)
    {
        free(ismLogMsgsSeen);
        ismLogMsgsSeen = NULL;
        ismLogMsgsSeenCount = ismLogMsgsSeenTotal = 0;
    }

    // Clean up the protocol library if we loaded it.
    if (libismprotocol_hdl != NULL)
    {
        dlclose(libismprotocol_hdl);
    }

    // Get rid of the shared log lock we've been using
    char sharedLogLockName[100];

    strcpy(sharedLogLockName, "/com.ibm.ima::shared_log_lock");

    char *qualifyShared = getenv(test_envVars[12].name);

    // Allow the BUILD_ENGINE definition to override the shared memory name
    if (NULL == qualifyShared) qualifyShared = getenv("BUILD_ENGINE");

    if (NULL != qualifyShared)
    {
        strcat(sharedLogLockName, ":");
        strcat(sharedLogLockName, qualifyShared);
    }

    shm_unlink(sharedLogLockName);

    return OK;
}

//****************************************************************************
/// @brief  Catch a terminating signal and try to flush the trace file
//****************************************************************************
static void test_signalHandler(int signo)
{
    static volatile int in_handler = 0;

    signal(signo, SIG_DFL);

    if (!in_handler)
    {
        in_handler = 1;
        ism_common_traceSignal(signo);
    }

    raise(signo);
}

//****************************************************************************
/// @brief  Set the store management generation granule sizes in the config
///
/// @param[in]  smallSizeBytes    Value for MgmtSmallGranuleSizeBytes
/// @param[in]  sizeBytes         Value for MgmtGranuleSizeBytes
///
/// @remark This function enables us to configure the management generation for
///         a test in such a way that either more or less objects can be created.
///
///         The main reason for this is to ensure that even if the project changes
///         the default granule sizes (as they have recently) the unit test will
///         operate in the same way... as a side effect it tests the store with
///         non-default granule sizes.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_setStoreMgmtGranuleSizes(uint32_t smallSizeBytes, uint32_t sizeBytes)
{
    int32_t rc;

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();
    ism_field_t f;

    #if 1
    //ism_prop_t *storeConfigProps = staticConfigProps;
    #else
    //ism_prop_t *storeConfigProps = ism_config_getComponentProps(ISM_CONFIG_COMP_STORE);
    #endif

    f.type = VT_UInt;
    f.val.u = smallSizeBytes;
    rc = ism_common_setProperty(staticConfigProps, ismSTORE_CFG_MGMT_SMALL_GRANULE, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s='%u', returned %d\n", ismSTORE_CFG_MGMT_SMALL_GRANULE, f.val.u, rc);
        return rc;
    }

    f.val.u = sizeBytes;
    rc = ism_common_setProperty(staticConfigProps, ismSTORE_CFG_MGMT_GRANULE_SIZE, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s='%u', returned %d\n", ismSTORE_CFG_MGMT_GRANULE_SIZE, f.val.u, rc);
        return rc;
    }

    return rc;
}

//****************************************************************************
/// @brief  Configure the product parameters as part of test_engineInit
///
/// @param[in]  coldStart                 Whether to perform a store cold-start or not.
/// @param[in]  disableAuthorization      Whether authorization (and authority checks)
///                                       are disabled
/// @param[in]  disableAutoQueueCreation  Whether automatic named queue creation is
/// @param[in]  subListCacheCapacity      Subscriber list cache capacity to use
/// @param[in]  totalStoreMemSizeMB       The size to initialize the store to (use
///                                       testDEFAULT_STORE_SIZE for normal tests)
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_engineInit_Config(bool coldStart,
                               bool disableAuthorization,
                               bool disableAutoQueueCreation,
                               uint32_t subListCacheCapacity,
                               int32_t totalStoreMemSizeMB)
{
    int32_t rc = OK;

    // Generate a per-user 7 character 'PlatformSerialNumber'
    char SSNString[8];
    char buf[8];
    getlogin_r(buf, sizeof(buf));

    // sprintf(SSNString, "%-7.7s", getpwuid(geteuid())->pw_name);
    sprintf(SSNString, "%-7.7s", buf);

    for(int32_t i=0; i<7; i++)
    {
        if (!isalnum(SSNString[i])) SSNString[i]=('A'+i);
    }

    setenv("ISMSSN", SSNString, 1);

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();
    ism_field_t f;

    #if 1
    //ism_prop_t *storeConfigProps = staticConfigProps;
    //ism_prop_t *engineConfigProps = staticConfigProps;
    #else
    //ism_prop_t *storeConfigProps = ism_config_getComponentProps(ISM_CONFIG_COMP_STORE);
    //ism_prop_t *engineConfigProps = ism_config_getComponentProps(ISM_CONFIG_COMP_ENGINE);
    #endif

    // Set the store default root path
    f.type = VT_String;
    f.val.s = test_envVars[13].propValue;

    rc = ism_common_setProperty(staticConfigProps, test_envVars[13].configName, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s='%s', returned %d\n",
               test_envVars[13].configName, f.val.s, rc);
        goto mod_exit;
    }

    // Use the same path for the Disk persist path (this is contrary to the advice about performance
    // in store.h, but we are not particularly bothered about performance in our unit tests).
    rc = ism_common_setProperty(staticConfigProps, ismSTORE_CFG_DISK_PERSIST_PATH, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s='%s', returned %d\n", ismSTORE_CFG_DISK_PERSIST_PATH, f.val.s, rc);
    }

    // Force the store to be smaller than default so it reduces the initialisation time
    f.type = VT_Integer;
    f.val.i = totalStoreMemSizeMB;
    rc = ism_common_setProperty(staticConfigProps, ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d returned %d\n", ismSTORE_CFG_TOTAL_MEMSIZE_MB, f.val.i, rc);
        goto mod_exit;
    }

    // Force the store to be smaller than default so it reduces the initialisation time
    f.type = VT_Integer;
    f.val.i = 2*totalStoreMemSizeMB;
    rc = ism_common_setProperty(staticConfigProps, ismSTORE_CFG_RECOVERY_MEMSIZE_MB, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d returned %d\n", ismSTORE_CFG_RECOVERY_MEMSIZE_MB, f.val.i, rc);
        goto mod_exit;
    }

    // Force a cold start for the store if requested
    f.type = VT_Boolean;
    f.val.i = coldStart;
    rc = ism_common_setProperty(staticConfigProps, ismSTORE_CFG_COLD_START, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d returned %d\n", ismSTORE_CFG_COLD_START, f.val.i, rc);
        goto mod_exit;
    }

    globalDisableAuthorization = disableAuthorization;

    // Set authority checking requirement explicitly
    f.type = VT_String;
    f.val.s = disableAuthorization ? "DISABLED BY TEST" : ""; // Empty string means do not disable.
    rc = ism_common_setProperty(staticConfigProps, "DisableAuthorization", &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for DisableAuthorization returned %d\n", rc);
        goto mod_exit;
    }

    globalDisableAutoQueueCreation = disableAutoQueueCreation;

    f.type = VT_String;
    f.val.s = disableAutoQueueCreation ? "true" : "false";
    rc = ism_common_setProperty(staticConfigProps, ismENGINE_CFGPROP_DISABLE_AUTO_QUEUE_CREATION, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for " ismENGINE_CFGPROP_DISABLE_AUTO_QUEUE_CREATION " returned %d\n", rc);
        goto mod_exit;
    }

    char tempString[50];

    // Set the sublist cache capacity as a string (as it would be coming from config)
    globalSubListCacheCapacity = subListCacheCapacity;
    sprintf(tempString, "%u", subListCacheCapacity);
    f.type = VT_String;
    f.val.s = tempString;
    rc = ism_common_setProperty(staticConfigProps,
                                ismENGINE_CFGPROP_INITIAL_SUBLISTCACHE_CAPACITY,
                                &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s returned %d\n", ismENGINE_CFGPROP_INITIAL_SUBLISTCACHE_CAPACITY, rc);
        goto mod_exit;
    }

    // Disable updates to the server timestamp
    f.type = VT_Integer;
    f.val.i = 0;
    rc = ism_common_setProperty(staticConfigProps,
                                ismENGINE_CFGPROP_SERVER_TIMESTAMP_INTERVAL,
                                &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d returned %d\n", ismENGINE_CFGPROP_SERVER_TIMESTAMP_INTERVAL, f.val.i, rc);
        goto mod_exit;
    }

    // Disable retained minimum active orderId scans (so they can be done manually)
    rc = ism_common_setProperty(staticConfigProps,
                                ismENGINE_CFGPROP_RETAINED_MIN_ACTIVE_ORDERID_INTERVAL,
                                &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d returned %d\n", ismENGINE_CFGPROP_RETAINED_MIN_ACTIVE_ORDERID_INTERVAL, f.val.i, rc);
    }

    // Disable cluster retained synchronization checks (so they can be done manually)
    rc = ism_common_setProperty(staticConfigProps,
                                ismENGINE_CFGPROP_CLUSTER_RETAINED_SYNC_INTERVAL,
                                &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s=%d returned %d\n", ismENGINE_CFGPROP_CLUSTER_RETAINED_SYNC_INTERVAL, f.val.i, rc);
    }

    f.type = VT_String;
    f.val.s = test_envVars[15].propValue;
    if (f.val.s == NULL) f.val.s = "False"; // TEMPORARILY FORCE DISK PERSISTENCE OFF FOR ALL TESTS

    if (f.val.s != NULL)
    {
        rc = ism_common_setProperty(staticConfigProps,
                                    ismSTORE_CFG_DISK_ENABLEPERSIST,
                                    &f);
        if (rc != OK)
        {
            printf("ERROR: ism_common_setProperty() for %s=%d returned %d\n", ismSTORE_CFG_DISK_ENABLEPERSIST, f.val.i, rc);
        }
    }

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief  Perform the init portion of test_engineInit
///
/// @param[in,out] storeInitTime   How long ism_store_init took
/// @param[in,out] clusterInitTime How long ism_cluster_init took
/// @param[in,out] engineInitTime  How long ism_engine_init took
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_engineInit_Init(uint64_t *storeInitTime,
                             uint64_t *clusterInitTime,
                             uint64_t *engineInitTime)
{
    int32_t rc = OK;

    ism_time_t startTime, endTime;

    globalStoreInit = false;
    startTime = ism_common_currentTimeNanos();
    rc = ism_store_init();
    endTime = ism_common_currentTimeNanos();
    if (storeInitTime != NULL) *storeInitTime = endTime-startTime;
    if (rc == OK)
    {
        globalStoreInit = true;
    }
    else
    {
        printf("ERROR: ism_store_init() returned %d\n", rc);
        goto mod_exit;
    }

#if 0
    ism_field_t f;
    f.type = VT_Integer;
    printf("UCP: %d\n", useControlPort);
    f.val.i = useControlPort++;
    ism_common_setProperty(ism_common_getConfigProperties(), ismCLUSTER_CFG_CONTROLPORT, &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for %s='%d' returned %d\n",
               ismCLUSTER_CFG_CONTROLPORT, f.val.i, rc);
        goto mod_exit;
    }
#endif

    globalAdminInit = 0;

    if (globalDisableAuthorization == false)
    {
        // If we are using fake security routines, don't initialize the admin component
        bool *usingFake = dlsym(RTLD_LOCAL, "fakeSecurityRoutines");

        if (usingFake == NULL || *usingFake == false)
        {
            rc = ism_admin_init(ISM_PROTYPE_SERVER);

            if (rc != OK)
            {
                printf("ERROR: ism_admin_init() returned %d\n", rc);
                goto mod_exit;
            }

            globalAdminInit = 1;
        }
    }

    if (globalAdminInit == 0 && globalAdminDir != NULL)
    {
        ism_adminHA_init_locks();

        // Register a NULL security callback to ensure we have a handle the engine can use
        rc = ism_config_register(ISM_CONFIG_COMP_SECURITY,
                                 NULL,
                                 test_fake_security_configCallback,
                                 &fakeSecurityConfigCallbackHandle);

        if (rc != OK)
        {
            printf("ERROR: ism_config_register() returned %d\n", rc);
            goto mod_exit;
        }

        globalAdminInit = 2;
    }

    startTime = ism_common_currentTimeNanos();
    rc = ism_cluster_init();
    endTime = ism_common_currentTimeNanos();
    if (rc == OK)
    {
        globalClusterInit = true;
    }
    else if (rc == ISMRC_ClusterDisabled)
    {
        globalClusterInit = false;
    }
    else
    {
        printf("ERROR: ism_cluster_init() returned %d\n", rc);
        goto mod_exit;
    }

    if (globalClusterInit)
    {
        rc = ism_cluster_setLocalForwardingInfo("TestServerName",
                                                "TestLocalServerUID",
                                                "127.0.0.01",
                                                2222,
                                                false);
        if (rc != OK)
        {
            printf("ERROR: ism_cluster_setLocalForwardingInfo() returned %d\n", rc);
            goto mod_exit;
        }
    }
    if (clusterInitTime != NULL) *clusterInitTime = endTime-startTime;

    startTime = ism_common_currentTimeNanos();
    rc = ism_engine_init();
    endTime = ism_common_currentTimeNanos();
    if (engineInitTime != NULL) *engineInitTime = endTime-startTime;
    if (rc != OK)
    {
        printf("ERROR: ism_engine_init() returned %d\n", rc);
        goto mod_exit;
    }

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief  Perform the start portion of test_engineInit
///
/// @param[in,out] storeStartTime   How long ism_store_start took
/// @param[in,out] clusterStartTime How long ism_cluster_start took
/// @param[in,out] engineStartTime  How long ism_engine_start took
/// @param[in]     callThreadInit   Whether to call ism_engine_threadInit
/// @param[in]     allowFailureToStart Is it expected that start fails
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_engineInit_Start(uint64_t *storeStartTime,
                              uint64_t *clusterStartTime,
                              uint64_t *engineStartTime,
                              bool callThreadInit,
                              bool allowFailureToStart)
{
    int32_t rc = OK;

    ism_time_t startTime, endTime;

    rc = ism_common_startServer();
    if (rc != OK)
    {
        if (rc != 999) printf("ERROR: ism_common_startServer() returned %d\n", rc);
        goto mod_exit;
    }

    startTime = ism_common_currentTimeNanos();
    rc = ism_store_start();
    endTime = ism_common_currentTimeNanos();
    if (storeStartTime != NULL) *storeStartTime = endTime-startTime;
    if (rc != OK)
    {
        if (rc != 999) printf("ERROR: ism_store_start() returned %d\n", rc);
        goto mod_exit;
    }

    if (globalClusterInit)
    {
        startTime = ism_common_currentTimeNanos();
        rc = ism_cluster_start();
        endTime = ism_common_currentTimeNanos();
        if (rc != OK)
        {
            if (rc != 999) printf("ERROR: ism_cluster_start() returned %d\n", rc);
            goto mod_exit;
        }

        // Register a dummy protocol callback
        rc = ism_cluster_registerProtocolEventCallback(test_dummyClusterProtocolEventCallback, NULL);
        if (rc != OK)
        {
            if (rc != 999) printf("ERROR: ism_cluster_registerProtocolEventCallback() returned %d\n", rc);
            goto mod_exit;
        }
    }
    if (clusterStartTime != NULL) *clusterStartTime = endTime-startTime;

    startTime = ism_common_currentTimeNanos();
    rc = ism_engine_start();
    if (rc != OK)
    {
        if(!allowFailureToStart && rc != 999) printf("ERROR: ism_engine_start() returned %d\n", rc);
        goto mod_exit;
    }

    rc = ism_engine_startMessaging();
    endTime = ism_common_currentTimeNanos();
    if (engineStartTime != NULL) *engineStartTime = endTime-startTime;
    if (rc != OK)
    {
        if (rc != 999) printf("ERROR: ism_engine_startMessaging() returned %d\n", rc);
        goto mod_exit;
    }
    if (callThreadInit == true) ism_engine_threadInit(0);

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief  Perform initialization of the engine/store components
///
/// @param[in]  coldStart                 Whether to perform a store cold-start or not.
/// @param[in]  disableAuthorization      Whether authorization (and authority checks)
///                                       are disabled
/// @param[in]  disableAutoQueueCreation  Whether automatic named queue creation is
/// @param[in]  allowFailureToStart       If true, don't abort if start fails
/// @param[in]  subListCacheCapacity      Subscriber list cache capacity to use
/// @param[in]  totalStoreMemSizeMB       The size to initialize the store to (use
///                                       testDEFAULT_STORE_SIZE for normal tests)
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_engineInit(bool coldStart,
                        bool disableAuthorization,
                        bool disableAutoQueueCreation,
                        bool allowFailureToStart,
                        uint32_t subListCacheCapacity,
                        int32_t totalStoreMemSizeMB)
{
    int32_t rc;

    // Clean up any threads which were (presumably to recreate some failure)
    // orphaned and unterminated.
    if (ismEngine_serverGlobal.threadDataHead != NULL)
    {
        // Not taking locks here - we're going to pretend this thread is each
        // of the orphans and tidy them up
        while((ismEngine_threadData = ismEngine_serverGlobal.threadDataHead))
        {
            printf("Releasing orphaned thread data %p\n",ismEngine_threadData);
            ism_engine_threadTerm(0); // Note we don't want to call the store
        }

        // Reset the ended thread statistics
        memset(&ismEngine_serverGlobal.endedThreadStats, 0, sizeof(ismEngine_serverGlobal.endedThreadStats));
    }

    // Register a signal handler for SIGINT & SIGSEGV
    signal(SIGINT, test_signalHandler);
    signal(SIGSEGV, test_signalHandler);

    rc = test_engineInit_Config(coldStart,
                                disableAuthorization,
                                disableAutoQueueCreation,
                                subListCacheCapacity,
                                totalStoreMemSizeMB);

    if (rc != OK) goto mod_exit;

    /************************************************************/
    /* Init - Not interested in init times                      */
    /************************************************************/
    rc = test_engineInit_Init(NULL, NULL, NULL);

    if (rc != OK) goto mod_exit;

    /************************************************************/
    /* Randomly initialise this thread before or after the call */
    /* to ism_engine_start.                                     */
    /************************************************************/
    bool threadInitAfter = (rand()%100 > 50) ? true : false;

    if (threadInitAfter == false) ism_engine_threadInit(0);

    /************************************************************/
    /* If a function to set properties was supplied, call it    */
    /* at this stage before continuing with the engine start.   */
    /************************************************************/
    if (test_engineInit_betweenInitAndStartFunc != NULL)
    {
        rc = test_engineInit_betweenInitAndStartFunc();
        if (rc != OK) goto mod_exit;
    }

    // Need to call this once to establish whether a ResourceSetDescriptor is defined,
    // ordinarily it gets done under ism_admin_init - but we may not have called that.
    (void)ism_admin_isResourceSetDescriptorDefined(0);

    /************************************************************/
    /* Start - Not interested in start times                    */
    /************************************************************/
    rc = test_engineInit_Start(NULL, NULL, NULL,
               threadInitAfter, allowFailureToStart);

    if (rc != OK) goto mod_exit;

    // Looks like these properties are moving to per-component sets, get
    // ready for that, when it's done, use the 2nd form.
    #if 1
    ism_prop_t *storeProps = ism_common_getConfigProperties();
    size_t storePropNameOffset = 0;
    #else
    ism_prop_t *storeProps = ism_config_getComponentProps(ISM_CONFIG_COMP_STORE);
    size_t storePropNameOffset = strlen("Store.");
    #endif

    /************************************************************/
    /* Find out whether we're using diskpersistence             */
    /************************************************************/
    usingDiskPersistence = ism_common_getBooleanProperty(storeProps,
                                                         (char *)(&ismSTORE_CFG_DISK_ENABLEPERSIST + storePropNameOffset),
                                                         1);

mod_exit:

    if (!allowFailureToStart && rc != OK && rc != 999) abort();

    return rc;
}

//****************************************************************************
/// @brief  Perform termination of the engine/store components
///
/// @param[in]  dropStore         Whether to drop (delete) the store [see remark]
///
/// @remark dropping the store is currently disabled, as it is believed to be
///         part of the cause of rc==500 coming back from ism_store_start. The
///         issue appears to be the use of the store in the same process after
///         the store has been dropped earlier.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_engineTerm(bool dropStore)
{
    int32_t rc = OK;

    test_freeFakeSecurityData();

    rc = ism_engine_term();
    if (rc != OK)
    {
        printf("ERROR: ism_engine_term() returned %d\n", rc);
        goto mod_exit;
    }

    if (globalClusterInit)
    {
        rc = ism_cluster_term();
        if (rc != OK)
        {
            printf("ERROR: ism_cluster_term() returned %d\n", rc);
            goto mod_exit;
        }
        globalClusterInit = false;
    }

    if (globalStoreInit)
    {
        if (dropStore)
        {
            rc = ism_store_drop();
            if (rc != OK)
            {
                printf("ERROR: ism_store_drop() returned %d\n", rc);
                goto mod_exit;
            }
        }

        rc = ism_store_term();
        if (rc != OK)
        {
            printf("ERROR: ism_store_term() returned %d\n", rc);
            goto mod_exit;
        }
    }

    switch(globalAdminInit)
    {
        case 1:
            rc = ism_admin_term(ISM_PROTYPE_SERVER);
            if (rc != OK)
            {
                printf("ERROR: ism_admin_term() returned %d\n", rc);
                goto mod_exit;
            }
            break;
        case 2:
            // Don't want the config component to call us back any more
            if (fakeSecurityConfigCallbackHandle != NULL)
            {
                (void)ism_config_unregister(fakeSecurityConfigCallbackHandle);
                fakeSecurityConfigCallbackHandle = NULL;
            }
            break;
        default:
            break;
    }

mod_exit:

    // deregister the signal handlers set up in init
    signal(SIGINT, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);

    // Terminate the engine thread data
    ism_engine_threadTerm(1);

    if (rc != OK) abort();

    return OK;
}

//****************************************************************************
/// @brief  Simulate a stop and restart of the engine (and store)
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_bounceEngine(void)
{
    int32_t rc;
    ism_prop_t *savedPolicies[50];

    test_saveFakeSecurityData(savedPolicies);

    rc = test_engineTerm(false);

    if (rc != OK) goto mod_exit;  // Failure reported by test_engine_Term already

    // Reset the ended thread statistics
    memset(&ismEngine_serverGlobal.endedThreadStats, 0, sizeof(ismEngine_serverGlobal.endedThreadStats));

    // TODO: Could call a specified callback here to check things

    rc = test_engineInit(false, globalDisableAuthorization,
                         globalDisableAutoQueueCreation,
                         false, //recovery should complete ok
                         globalSubListCacheCapacity,
                         testDEFAULT_STORE_SIZE);

    if (rc != OK) goto mod_exit;  // Failure reported by test_engineInit already

    test_loadFakeSecurityData(savedPolicies);

mod_exit:

    return rc;
}

//****************************************************************************
/// @brief  Invoke execv after any required tidy up
//****************************************************************************
int test_execv(const char *file, char *const argv[])
{
    if (globalClusterInit) ism_cluster_term();
    return execv(file, argv);
}

//****************************************************************************
/// @brief  Take a core dump
//****************************************************************************
void test_kill_me(const char *file, char *const argv[])
{
    char *sroot = getenv("SROOT");
    char killMe[4096];

    if (sroot == NULL)
    {
        printf("Cannot locate script to take core\n");
        exit(99);
    }
    else
    {
        sprintf(killMe, "%s/server_engine/scripts/takecore %d", sroot, getpid());

        if (file != NULL && argv != NULL)
        {
            while(*argv != NULL)
            {
                strcat(killMe, " ");
                strcat(killMe, *argv);
                argv++;
            }
        }

        int rc = system(killMe);

        if (rc != 0)
        {
            printf("Cannot run script to take core\n");
            exit(99);
        }

        int seconds = sleep(60);

        printf("Interrupted with %d seconds left\n", seconds);
    }
}
