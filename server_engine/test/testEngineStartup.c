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

/*********************************************************************/
/*                                                                   */
/* Module Name: testEngineStartup.c                                  */
/*                                                                   */
/* Description: Test the performance of engine startup with varying  */
/*              amounts of store consumed.                           */
/*                                                                   */
/*********************************************************************/
#include <sys/stat.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>

#include "engine.h"
#include "engineInternal.h"
#include "ha.h"
#include "admin.h"
#include "adminHA.h"
#include "test_utils_initterm.h"
#include "test_utils_ismlogs.h"
#include "test_utils_file.h"
#include "test_utils_assert.h"
#include "test_utils_security.h"
#include "test_utils_options.h"
#include "test_utils_config.h"
#include "test_utils_task.h"

void *libismprotocol = NULL;
void *libismmonitoring = NULL;

static void pinToNextCPU(void);

#define CONFIG_SUBDIR_TEMPLATE "/testEngineStartup_XXXXXX"
#define FINAL_PHASE 2

typedef void * (*simFunction_t)(void *);
typedef struct tag_simFuncParms_t
{
    uint32_t phase;
    uint32_t instance;
    uint32_t msgsToPut;
    uint32_t msgSize;
    pthread_t thread_id;
}
simFuncParms_t;

void *simFunc_SimpleMsgsOnQueue(void *arg)
{
    static uint32_t END_PHASE = 2;

    int32_t rc = OK;
    char clientId[40];
    char queueName[40];
    char tempBuffer[1024];

    simFuncParms_t *parms = (simFuncParms_t *)arg;

    // Pin this thread to a CPU
    pinToNextCPU();

    // If called for phase '0' this means return the end phase for this function
    if (parms->phase == 0)
    {
        parms->phase = END_PHASE;
        goto mod_exit;
    }
    else if (parms->phase >= END_PHASE)
    {
        goto mod_exit;
    }

    ism_engine_threadInit(true);

    sprintf(clientId, "Client_%04u", parms->instance);
    sprintf(queueName, "Queue_%04u", parms->instance);

    // Create the queue during phase 1, expect it to be there for others
    if (parms->phase == 1)
    {
        sprintf(tempBuffer, "{\"Queue\":{\"%s\":{\"MaxMessages\":10000000}}}", queueName);
        rc = test_configProcessPost(tempBuffer);
        TEST_ASSERT(rc == OK, ("line=%d rc=%d",__LINE__, rc));
    }

    // Create a client
    ismEngine_ClientStateHandle_t hClient;

    rc = ism_engine_createClientState(clientId,
                                      testDEFAULT_PROTOCOL_ID,
                                      ismENGINE_CREATE_CLIENT_OPTION_DURABLE,
                                      NULL, NULL, NULL,
                                      &hClient,
                                      NULL,
                                      0,
                                      NULL);
    TEST_ASSERT(rc == OK, ("line=%d rc=%d",__LINE__, rc));
    TEST_ASSERT(hClient != NULL, ("hClient is NULL"));

    // Create a session
    ismEngine_SessionHandle_t hSession;

    rc = ism_engine_createSession(hClient,
                                  ismENGINE_CREATE_SESSION_OPTION_NONE,
                                  &hSession,
                                  NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("line=%d rc=%d",__LINE__, rc));
    TEST_ASSERT(hSession != NULL, ("hSession is NULL"));

    char *msgData = malloc(parms->msgSize);
    TEST_ASSERT(msgData != NULL, ("Unable to allocate storage for msgData"));

    // Put the number of requested messages
    for(uint32_t i=0; i<parms->msgsToPut; i++)
    {
        // Create a persistent message
        ismMessageHeader_t header = ismMESSAGE_HEADER_DEFAULT;
        ismMessageAreaType_t areaTypes[2] = {ismMESSAGE_AREA_PROPERTIES, ismMESSAGE_AREA_PAYLOAD};
        size_t areaLengths[2] = {0, parms->msgSize};
        void *areaData[2] = {NULL, msgData};
        ismEngine_MessageHandle_t hMessage;

        header.Reliability = ismMESSAGE_RELIABILITY_EXACTLY_ONCE;
        header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;

        rc = ism_engine_createMessage(&header,
                                      2,
                                      areaTypes,
                                      areaLengths,
                                      areaData,
                                      &hMessage);
        TEST_ASSERT(rc == OK, ("line=%d rc=%d",__LINE__, rc));

        // Put the message on the queue
        rc = ism_engine_putMessageOnDestination(hSession,
                                                ismDESTINATION_QUEUE, queueName,
                                                NULL,
                                                hMessage,
                                                NULL, 0, NULL);
        TEST_ASSERT(rc == OK, ("line=%d rc=%d",__LINE__, rc));
    }

    free(msgData);

    // Destroy the session
    rc = ism_engine_destroySession(hSession,
                                   NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("line=%d rc=%d",__LINE__, rc));

    // Destroy (but do not delete) the client
    rc = ism_engine_destroyClientState(hClient,
                                       ismENGINE_DESTROY_CLIENT_OPTION_NONE,
                                       NULL, 0, NULL);
    TEST_ASSERT(rc == OK, ("line=%d rc=%d",__LINE__, rc));

    ism_engine_threadTerm(1);

mod_exit:

    return NULL;
}

//****************************************************************************
/// @brief Get the number of CPUs
//****************************************************************************
static int getHardwareNumCPUs(void)
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}

//****************************************************************************
/// @brief Pin this thread to the next available CPU
//****************************************************************************
static void pinToNextCPU(void)
{
    char CPUs[128] = {0};

    static int hardwareNumCPUs = -1;
    static int nextCPU = 0;

    if (hardwareNumCPUs == -1) {
        hardwareNumCPUs = getHardwareNumCPUs();

        if (hardwareNumCPUs == -1) {
            fprintf(stderr, "ERROR: getHardwareNumCPUs returned %d\n", hardwareNumCPUs);
            return;
        }
    }

    int myCPU = (hardwareNumCPUs - (__sync_fetch_and_add(&nextCPU, 1) % hardwareNumCPUs)) - 1;

    if (myCPU < sizeof(CPUs)) {
        CPUs[myCPU] = 1;
        ism_common_setAffinity(ism_common_threadSelf(), CPUs, sizeof(CPUs));
    }
    else {
        fprintf(stderr, "ERROR: Unable to pin to CPU %d\n", myCPU);
    }
}

//****************************************************************************
/// @brief  Timed functions...
//****************************************************************************
int time_simple_func(const char *function)
{
    int (*func)(void) = NULL;

    func = dlsym(RTLD_NEXT, function);

    fprintf(stdout, ">> %s\n", function);
    uint64_t prevTime = ism_common_currentTimeNanos();
    int rc = func(); // Run the function
    uint64_t elapsedTime = ism_common_currentTimeNanos() - prevTime;
    double elapsedSecs = ((double)elapsedTime) / 1000000000;
    fprintf(stdout, "<< %s (rc=%d) took %.2f secs (%lu ns)\n", function, rc, elapsedSecs, elapsedTime);

    return rc;
}

int ism_store_init(void)
{
    return time_simple_func(__FUNCTION__);
}

int ism_cluster_init(void)
{
    return time_simple_func(__FUNCTION__);
}

int ism_engine_init(void)
{
    return time_simple_func(__FUNCTION__);
}

int ism_store_start(void)
{
    return time_simple_func(__FUNCTION__);
}

int ism_engine_start(void)
{
    return time_simple_func(__FUNCTION__);
}

int ierr_completeRecovery(void)
{
    return time_simple_func(__FUNCTION__);
}

//****************************************************************************
/// @brief Process Command line arguments
///
/// @param[in]     argc  Count of arguments
/// @param[in]     argv  Array of arguments
///
/// @return The index of the first non-option argument on success, -1 on error.
//****************************************************************************
int processArgs(int argc, char **argv,
                simFunction_t *simFunc,
                uint32_t *funcInstances,
                uint32_t *msgsPerInstance,
                uint32_t *msgSize,
                uint32_t *beginPhase,
                uint32_t *endPhase,
                bool *keepData,
                bool *storeColdStart,
                size_t *storeMemSizeMB,
                uint64_t *randomSeed,
                uint32_t *traceLevel,
                char **dataDir)
{
    int usage = 0;
    char opt;
    int long_index;
    char *serverConfig = NULL;

    const char short_options[] = "f:i:m:s:b:e:kr:d:M:CS:";

    struct option long_options[] = {
        { "simFunc", 1, NULL, short_options[0] },
        { "funcInstances", 1, NULL, short_options[2] },
        { "msgsPerInstances", 1, NULL, short_options[4] },
        { "msgSize", 1, NULL, short_options[6] },
        { "beginPhase", 1, NULL, short_options[8] },
        { "endPhase", 1, NULL, short_options[10] },
        { "keepData", 0, NULL, short_options[12] },
        { "randomSeed", 1, NULL, short_options[13] },
        { "traceLevel", 1, NULL, 0 },
        { "traceFile", 1, NULL, 0 },
        { "dataDir", 1, NULL, short_options[15] },
        { "storeMemSizeMB", 1, NULL, short_options[17] },
        { "storeColdStart", 0, NULL, short_options[19] },
        { "serverConfig", 1, NULL, short_options[20] },
        { NULL, 1, NULL, 0 } };

    // Basic test 10,000 10k messages just put to a queue
    *simFunc = simFunc_SimpleMsgsOnQueue;
    *funcInstances = 1;
    *msgsPerInstance = 10000;
    *msgSize = 10240;
    *beginPhase = 1;
    *endPhase = 0;

    *keepData = false;
    *storeColdStart = (argc == 1); // For default test run, want to start with clean store
    *randomSeed = ism_common_currentTimeNanos();
    *traceLevel = 0;
    *dataDir = NULL;
    if (argc == 1)
    {
        size_t physicalMemory = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE);
        *storeMemSizeMB = (physicalMemory / (1024*1024)) / 16;
    }
    else
    {
        *storeMemSizeMB = 0;
    }

    while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1)
    {
        // Turn long_index into long_options value
        if (opt == 0)
        {
            switch(long_index)
            {
                case 8: // traceLevel
                    *traceLevel = atoi(optarg);
                    break;
                case 9:
                    setenv("TraceFile", optarg, true);
                    break;
                default:
                    usage=1;
                    break;
            }
        }
        else
        {
            // Now process options
            switch (opt)
            {
                case 'f':
                    printf("simFunc requested = \"%s\"\n", optarg);
                    break;
                case 'i':
                    *funcInstances = atoi(optarg);
                    break;
                case 'm':
                    *msgsPerInstance = atoi(optarg);
                    break;
                case 's':
                    *msgSize = atoi(optarg);
                    break;
                case 'b':
                    *beginPhase = atoi(optarg);
                    break;
                case 'e':
                    *endPhase = atoi(optarg);
                    break;
                case 'r':
                    *randomSeed = strtoull(optarg,NULL,0);
                    break;
                case 'k':
                    *keepData = true;
                    break;
                case 'd':
                    *dataDir = strdup(optarg);
                    break;
                case 'M':
                    *storeMemSizeMB = (size_t)atoi(optarg);
                    break;
                case 'C':
                    *storeColdStart = true;
                    break;
                case 'S':
                    serverConfig = strdup(optarg);
                    break;
                case '?':
                default:
                    usage=1;
                    break;
            }
        }
    }

    if (simFunc == NULL) usage = 1;

    if (usage)
    {
        fprintf(stderr, "Usage: %s -f simFunc [-i funcInstances] [-m msgsPerInstance] [-s msgSize]\n", argv[0]);
        fprintf(stderr, "                     [-b beginPhase] [-e endPhase] [-d dataDir]\n");
        fprintf(stderr, "                     [-k] [-D dynamicConfig] [-M storeMemSizeMB] [-C]\n");
        fprintf(stderr, "                     [-r randomSeed] [--traceLevel] [--traceFile]\n");
        fprintf(stderr, "       -f - Specify the function to use when loading the engine\n");
        fprintf(stderr, "       -i - Specify the number of function instances to use\n");
        fprintf(stderr, "       -m - Specify the number of messages per function instance\n");
        fprintf(stderr, "       -s - Specify the size of each message\n");
        fprintf(stderr, "       -b - Specify the phase at which to begin\n");
        fprintf(stderr, "       -e - Specify the phase at which to end\n");
        fprintf(stderr, "       -k - Keep data directories after the end phase\n");
        fprintf(stderr, "       -S - Specify the server config file to use\n");
        fprintf(stderr, "       -M - Specify store memory size in MB\n");
        fprintf(stderr, "       -C - Cold start\n");
        fprintf(stderr, "       -r - Specify the random seed value\n");
        fprintf(stderr, "       -d - Specify the data directory\n");
    }
    else
    {
        // We don't have a data directory configured, so set one up.
        if (*dataDir == NULL)
        {
            char tempDir[500] = {0};

            test_utils_getTempPath(tempDir);

            if (tempDir[0] == 0)
            {
                fprintf(stderr, "ERROR: No temporary directory found\n");
                return -2;
            }

            *dataDir = malloc(strlen(tempDir)+strlen(CONFIG_SUBDIR_TEMPLATE)+1);

            if (*dataDir == NULL)
            {
                fprintf(stderr, "ERROR: Could not allocate storage for data directory name\n");
                return -1;
            }

            strcpy(*dataDir, tempDir);
            strcat(*dataDir, CONFIG_SUBDIR_TEMPLATE);

            if (mkdtemp(*dataDir) == NULL)
            {
                fprintf(stderr, "ERROR: Creating config directory failed with %d", errno);
                return -2;
            }

            (void)mkdir(*dataDir, S_IRUSR | S_IWUSR | S_IXUSR);

            size_t maxSize = strlen(*dataDir)+strlen("/policies")+1;

            char tempCfgDir[maxSize];

            snprintf(tempCfgDir, maxSize, "%s/config", *dataDir);
            (void)mkdir(tempCfgDir, S_IRUSR | S_IWUSR | S_IXUSR);

            // Copy configuration files to our config directory
            char sourceFileName[4096];
            char targetFileName[4096];

            // No serverConfig file specified append default server.cfg and server_dynamic.cfg
            // together to form one
            if (serverConfig == NULL)
            {
                char *configSource = NULL;

                // Work out where to copy config files from
                char *sroot = getenv("SROOT");
                char *prjDir = getenv("PRJDIR");
                char *rootRel = getenv("ROOTREL");

                if (prjDir && rootRel)
                {
                    configSource = malloc(strlen(prjDir)+1+strlen(rootRel)+1+200);
                    if (configSource != NULL) sprintf(configSource, "%s/%s/server_main/config/", prjDir, rootRel);
                }
                else if (sroot)
                {
                    configSource = malloc(strlen(sroot)+1+200);
                    if (configSource != NULL) sprintf(configSource, "%s/server_main/config/", sroot);
                }

                if (configSource == NULL)
                {
                    fprintf(stderr, "ERROR: Unable to determine source for config file\n");
                    return -3;
                }

                sprintf(sourceFileName, "%sserver.cfg", configSource);
                test_copyFileWithStrip(sourceFileName, tempCfgDir, ismSTORE_CFG_DISK_ROOT_PATH);

                sprintf(sourceFileName, "%sserver_dynamic.cfg", configSource);
                sprintf(targetFileName, "%s/server.cfg", tempCfgDir);
                test_copyFileAppend(sourceFileName, targetFileName);

                // Force the use of shared memory
                test_appendTextToFile(ismSTORE_CFG_MEMTYPE" = 0\n", targetFileName);

                free(configSource);
            }
            else
            {
                test_copyFileRename(serverConfig, tempCfgDir, "server.cfg");
            }
            sprintf(targetFileName, "%s/server_dynamic.cfg", tempCfgDir);
            test_copyFileAppend(NULL, targetFileName);

            snprintf(tempCfgDir, maxSize, "%s/policies", *dataDir);
            (void)mkdir(tempCfgDir, S_IRUSR | S_IWUSR | S_IXUSR);
            snprintf(tempCfgDir, maxSize, "%s/logs", *dataDir);
            (void)mkdir(tempCfgDir, S_IRUSR | S_IWUSR | S_IXUSR);
        }
    }

    if (serverConfig != NULL) free(serverConfig);

    return usage ? -1 : optind;
}

//****************************************************************************
/// @brief Initialize the test, including initializing and starting store & engine
///
/// @return OK on success.
//****************************************************************************
int test_init(uint32_t *phase,
              char *dataDir,
              int32_t traceLevel,
              bool storeColdStart,
              size_t storeSize,
              ismHA_Role_t *haRole)
{
    int rc;
    char tempBuffer[4096];
    ism_field_t f;

    ism_common_initUtil();

    ism_prop_t *staticConfigProps = ism_common_getConfigProperties();

    #if 0
    ism_prop_t *storeConfigProps = staticConfigProps;
    #else
    ism_prop_t *storeConfigProps = ism_config_getComponentProps(ISM_CONFIG_COMP_STORE);
    #endif

    // Read in the properties in our server.cfg file
    sprintf(tempBuffer, "%s/config/server.cfg", dataDir);

    test_readConfigFile(tempBuffer, staticConfigProps);

    // Override the config directory
    sprintf(tempBuffer, "%s/config", dataDir);
    f.type = VT_String;
    f.val.s = tempBuffer;
    rc = ism_common_setProperty(staticConfigProps, "ConfigDir", &f);
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_common_setProperty() for ConfigDir returned %d\n", rc);
        goto mod_exit;
    }

    // Override the policy directory
    sprintf(tempBuffer, "%s/policies", dataDir);
    f.type = VT_String;
    f.val.s = tempBuffer;
    rc = ism_common_setProperty(staticConfigProps, "PolicyDir", &f);
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_common_setProperty() for PolicyDir returned %d\n", rc);
        goto mod_exit;
    }

    // Override the log directory
    sprintf(tempBuffer, "%s/logs", dataDir);
    f.type = VT_Integer;
    f.val.i = 1;
    rc = ism_common_setProperty(staticConfigProps, "LogUnitTest", &f);
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_common_setProperty() for LogUnitTest returned %d\n", rc);
        goto mod_exit;
    }

    // Override the location of the log files
    rc = test_setLogDir(tempBuffer);
    if (rc != OK) goto mod_exit;
    rc = test_relocateLogFile("System", "Max");
    if (rc != OK) goto mod_exit;
    rc = test_relocateLogFile("Connection", "Max");
    if (rc != OK) goto mod_exit;
    rc = test_relocateLogFile("Security", "Max");
    if (rc != OK) goto mod_exit;
    rc = test_relocateLogFile("Admin", "Max");
    if (rc != OK) goto mod_exit;

    // Override the location of the trace file
    f.val.s = getenv("TraceFile");
    if (f.val.s != NULL)
    {
        f.type = VT_String;
        rc = ism_common_setProperty(staticConfigProps, "TraceFile", &f);
        if (rc != OK)
        {
            fprintf(stderr, "ERROR: ism_common_setProperty() for TraceFile='%s', returned %d\n", f.val.s, rc);
            goto mod_exit;
        }
    }

    // Override the trace level
    f.type = VT_Integer;
    f.val.i = traceLevel;
    rc = ism_common_setProperty(staticConfigProps, "TraceLevel", &f);
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_common_setProperty() for TraceLevel='%d', returned %d\n", f.val.i, rc);
        goto mod_exit;
    }

    if (ism_common_getStringProperty(storeConfigProps, ismSTORE_CFG_DISK_ROOT_PATH) == NULL)
    {
        // Override the store directory
        sprintf(tempBuffer, "%s/store", dataDir);
        (void)mkdir(tempBuffer, S_IRUSR | S_IWUSR | S_IXUSR);
        f.type = VT_String;
        f.val.s = tempBuffer;
        rc = ism_common_setProperty(storeConfigProps, ismSTORE_CFG_DISK_ROOT_PATH, &f);
        if (rc != OK)
        {
            fprintf(stderr, "ERROR: ism_common_setProperty() for %s='%s', returned %d\n",
                    ismSTORE_CFG_DISK_ROOT_PATH, f.val.s, rc);
            goto mod_exit;
        }
    }

    if (storeSize != 0)
    {
        // Override the store totalMemSizeMB
        f.type = VT_Integer;
        f.val.i = storeSize;
        rc = ism_common_setProperty(storeConfigProps, ismSTORE_CFG_TOTAL_MEMSIZE_MB, &f);
        if (rc != OK)
        {
            fprintf(stderr, "ERROR: ism_common_setProperty() for %s=%d returned %d\n",
                    ismSTORE_CFG_TOTAL_MEMSIZE_MB, f.val.i, rc);
            goto mod_exit;
        }

        // Override the store recoveryMemSizeMB
        f.type = VT_Integer;
        f.val.i = storeSize;
        rc = ism_common_setProperty(storeConfigProps, ismSTORE_CFG_RECOVERY_MEMSIZE_MB, &f);
        if (rc != OK)
        {
            fprintf(stderr, "ERROR: ism_common_setProperty() for %s=%d returned %d\n",
                    ismSTORE_CFG_RECOVERY_MEMSIZE_MB, f.val.i, rc);
            goto mod_exit;
        }
    }

    // Override the store coldStart
    if (storeColdStart)
    {
        f.type = VT_Boolean;
        f.val.i = true;
        rc = ism_common_setProperty(storeConfigProps, ismSTORE_CFG_COLD_START, &f);
        if (rc != OK)
        {
            fprintf(stderr, "ERROR: ism_common_setProperty() for %s=%d returned %d\n",
                    ismSTORE_CFG_COLD_START, f.val.i, rc);
            goto mod_exit;
        }
    }

    // Disable authority checking
    f.type = VT_String;
    f.val.s = "DISABLED BY TEST"; // Empty string means do not disable.
    rc = ism_common_setProperty( staticConfigProps, "DisableAuthorization", &f);
    if (rc != OK)
    {
        printf("ERROR: ism_common_setProperty() for DisableAuthorization returned %d\n", rc);
        goto mod_exit;
    }

    rc = ism_common_initServer();
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_common_initServer() returned %d\n", rc);
        goto mod_exit;
    }

    rc = ism_admin_init(ISM_PROTYPE_SERVER);
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_admin_init() returned %d\n", rc);
        goto mod_exit;
    }

    rc = ism_transport_init();
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_transport_init() returned %d\n", rc);
        goto mod_exit;
    }

    libismprotocol = dlopen("libismprotocol.so", RTLD_LAZY | RTLD_GLOBAL);

    if (libismprotocol != NULL)
    {
        int (*ism_protocol_init)(void) = dlsym(libismprotocol, "ism_protocol_init");

        if (ism_protocol_init != NULL)
        {
            rc = ism_protocol_init();
            if (rc != OK)
            {
                fprintf(stderr, "ERROR: ism_protocol_init() returned %d\n", rc);
                goto mod_exit;
            }
        }
    }

    if ((rc = ism_store_init()) != OK) goto mod_exit;
    if ((rc = ism_cluster_init()) != OK) goto mod_exit;
    if ((rc = ism_engine_init()) != OK) goto mod_exit;

    libismmonitoring = dlopen("libismmonitoring.so", RTLD_LAZY | RTLD_GLOBAL);

    if (libismmonitoring != NULL)
    {
        int (*ism_monitoring_init)(void) = dlsym(libismmonitoring, "ism_monitoring_init");

        rc = ism_monitoring_init();

        if (rc != OK)
        {
            fprintf(stderr, "ERROR: ism_monitoring_init() returned %d\n", rc);
            goto mod_exit;
        }
    }

    // Perform startup
    rc = ism_common_startServer();
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_common_startServer() returned %d\n", rc);
        goto mod_exit;
    }

    rc = ism_transport_start();
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_transport_start() returned %d\n", rc);
        goto mod_exit;
    }

    if (libismprotocol != NULL)
    {
        int (*ism_protocol_start)(void) = dlsym(libismprotocol, "ism_protocol_start");

        if (ism_protocol_start != NULL)
        {
            rc = ism_protocol_start();
            if (rc != OK)
            {
                fprintf(stderr, "ERROR: ism_protocol_start() returned %d\n", rc);
                goto mod_exit;
            }
        }
    }

    if ((rc = ism_store_start()) != OK) goto mod_exit;

    // Decide which HA role we are running - only continue to ism_engine_start if
    // we are acting as primary, or HA is disabled.
    *haRole = ism_admin_getHArole(NULL, &rc);

    switch(*haRole)
    {
        case ISM_HA_ROLE_DISABLED:
            fprintf(stdout, "== HA Role: Disabled\n");
            break;
        case ISM_HA_ROLE_PRIMARY:
            fprintf(stdout, "== HA Role: Primary\n");
            break;
        case ISM_HA_ROLE_STANDBY:
            fprintf(stdout, "== HA Role: Standby\n");
            break;
        default:
            fprintf(stdout, "== HA Role: %d\n", *haRole);
            break;
    }

    if (*haRole != ISM_HA_ROLE_PRIMARY &&  *haRole != ISM_HA_ROLE_DISABLED)
    {
        (*phase)++;

        while(*haRole != ISM_HA_ROLE_PRIMARY)
        {
            sleep(1);
            *haRole = ism_admin_getHArole(NULL, &rc);
        }
    }

    if ((rc = ism_engine_start()) != OK) goto mod_exit;

    rc = ism_transport_startMessaging();
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_transport_startMessaging() returned %d\n", rc);
        goto mod_exit;
    }

mod_exit:

    return rc;
}

int test_term(void)
{
    int32_t rc;

    if (libismmonitoring != NULL)
    {
        int (*ism_monitoring_term)(void) = dlsym(libismmonitoring, "ism_monitoring_term");

        rc = ism_monitoring_term();
        if (rc != OK)
        {
            fprintf(stderr, "ERROR: ism_monitoring_term() returned %d\n", rc);
            goto mod_exit;
        }
    }

    rc = ism_transport_term();
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_transport_term() returned %d\n", rc);
        goto mod_exit;
    }

    if (libismprotocol != NULL)
    {
        int (*ism_protocol_term)(void) = dlsym(libismprotocol, "ism_protocol_term");

        if (ism_protocol_term != NULL)
        {
            rc = ism_protocol_term();
            if (rc != OK)
            {
                fprintf(stderr, "ERROR: ism_protocol_term() returned %d\n", rc);
                goto mod_exit;
            }
        }
    }

    rc = ism_engine_term();
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_engine_term() returned %d\n", rc);
        goto mod_exit;
    }

    rc = ism_store_term();
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_store_term() returned %d\n", rc);
        goto mod_exit;
    }

    rc = ism_admin_term(ISM_PROTYPE_SERVER);
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_admin_term() returned %d\n", rc);
        goto mod_exit;
    }

    rc = ism_common_termServer();
    if (rc != OK)
    {
        fprintf(stderr, "ERROR: ism_common_termServer() returned %d\n", rc);
        goto mod_exit;
    }

mod_exit:

    return rc;
}

int main(int argc, char *argv[])
{
    int retval;

    simFunction_t simFunc = NULL;
    uint32_t funcInstances = 1;
    uint32_t msgsPerInstance = 0;
    uint32_t msgSize = 0;
    uint64_t randomSeed = 0;
    uint32_t traceLevel = 0;
    uint32_t phase = 0;
    uint32_t endPhase = 0;
    bool keepData = false;
    bool storeColdStart = false;
    char *dataDir = NULL;
    ismHA_Role_t haRole;
    size_t storeSize = 0;

    pinToNextCPU(); // Pin Main to CPU 0.

    retval = processArgs(argc, argv,
                         &simFunc,
                         &funcInstances,
                         &msgsPerInstance,
                         &msgSize,
                         &phase,
                         &endPhase,
                         &keepData,
                         &storeColdStart,
                         &storeSize,
                         &randomSeed,
                         &traceLevel,
                         &dataDir);

    if (retval == -1) goto mod_exit;

    srand(randomSeed);

    simFuncParms_t *simFuncParms = (simFuncParms_t *)malloc(sizeof(simFuncParms_t)*funcInstances);

    if (simFuncParms == NULL) goto mod_exit;

    fprintf(stdout, "== DataDir: %s\n", dataDir);
    fprintf(stdout, "== FuncInstances: %u\n", funcInstances);
    fprintf(stdout, "== MsgsPerInstance: %u\n", msgsPerInstance);
    fprintf(stdout, "== MsgSize: %u\n", msgSize);
    fprintf(stdout, "== StoreSize: %lu\n", storeSize);
    fprintf(stdout, "== StoreColdStart: %s\n", storeColdStart ? "true" : "false");
    fprintf(stdout, "== Phase: %u\n", phase);

    retval = test_init(&phase,
                       dataDir,
                       traceLevel,
                       storeColdStart,
                       storeSize,
                       &haRole);

    if (retval != OK) goto mod_exit;

    // No end phase provided, call the simulation function once to get an endPhase value
    if (endPhase == 0)
    {
        simFuncParms[0].phase = 0;
        simFunc(&simFuncParms[0]);
        endPhase = simFuncParms[0].phase;
    }

    pthread_attr_t threadAttr;

    pthread_attr_init(&threadAttr);

    fprintf(stdout, ">> simulation threads\n");
    uint64_t prevTime = ism_common_currentTimeNanos();

    // Start all of the threads
    for(int32_t i=0; i<funcInstances; i++)
    {
        simFuncParms[i].phase = phase;
        simFuncParms[i].instance = i;
        simFuncParms[i].msgsToPut = msgsPerInstance;
        simFuncParms[i].msgSize = msgSize;

        retval = test_task_startThread(&simFuncParms[i].thread_id,simFunc, &simFuncParms[i],"simFunc");

        if (retval != 0)
        {
            fprintf(stderr, "ERROR: Failed to create thread, errno=%d\n", errno);
            goto mod_exit;
        }
    }

    // Wait for all of the threads to finish
    for(int32_t i=0; i<funcInstances; i++)
    {
        retval = pthread_join(simFuncParms[i].thread_id, NULL);

        if (retval != 0)
        {
            fprintf(stderr, "ERROR: Failed to join thread, errno=%d\n", errno);
            goto mod_exit;
        }
    }
    uint64_t elapsedTime = ism_common_currentTimeNanos() - prevTime;
    double elapsedSecs = ((double)elapsedTime) / 1000000000;
    fprintf(stdout, "<< simulation threads took %.2f secs (%lu ns)\n",
            elapsedSecs, elapsedTime);

    pthread_attr_destroy(&threadAttr);

    test_term();

    if (phase == endPhase)
    {
        if (keepData == false)
        {
            test_removeAdminDir(dataDir);
        }
    }
    else if (haRole == ISM_HA_ROLE_DISABLED) // Don't perform restart for HA configurations
    {
        phase++;

        fprintf(stdout, "== Restarting for phase %d\n", phase);

        // Re-execute ourselves for the next phase
        int32_t newargc = 0;

        char **newargv = malloc(argc+10 * sizeof(char *));

        newargv[newargc++] = argv[0];

        for(int32_t i=newargc; i<argc; i++)
        {
            if ((strcmp(argv[i], "--dataDir") == 0) ||
                (strcmp(argv[i], "--beginPhase") == 0) ||
                (strcmp(argv[i], "-b") == 0))
            {
                i++;
            }
            else if ((strcmp(argv[i], "-C") == 0) ||
                     (strcmp(argv[i], "--storeColdStart") == 0))
            {
                // SKIP on restart
            }
            else
            {
                newargv[newargc++] = argv[i];
            }
        }

        char phaseString[50];
        char memSizeString[50];
        sprintf(phaseString, "%d", phase);
        sprintf(memSizeString, "%lu", storeSize);

        newargv[newargc++] = "--dataDir";
        newargv[newargc++] = dataDir;
        newargv[newargc++] = "--beginPhase";
        newargv[newargc++] = phaseString;
        if (argc == 1)
        {
            newargv[newargc++] = "--storeMemSizeMB";
            newargv[newargc++] = memSizeString;
        }
        newargv[newargc] = NULL;

        printf("== Command: ");
        for(int32_t i=0; i<newargc; i++)
        {
            printf("%s ", newargv[i]);
        }
        printf("\n");

        retval = execvp(newargv[0], newargv);

        fprintf(stderr, "ERROR: execv failed with %d when moving to phase %d\n", errno, phase);
    }

mod_exit:

    if (dataDir != NULL) free(dataDir);

    return retval;
}
