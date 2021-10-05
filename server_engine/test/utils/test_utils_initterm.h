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
/// @file  test_utils_initterm.h
/// @brief Common initialization and termination routines for use by tests
//****************************************************************************
#ifndef __ISM_TEST_UTILS_INITTERM_H_DEFINED
#define __ISM_TEST_UTILS_INITTERM_H_DEFINED

#include <stdbool.h>
#include <stdint.h>
#include <sys/wait.h>
#include "engine.h"

static inline int test_fork_and_execvp(const char *file, char *const argv[])
{
    int rc = 0;

    // Fork a child process
    pid_t pid;
    switch(pid=fork())
    {
       case -1:
           fprintf(stderr, "ERROR: fork failed with %d\n", errno);
           rc = 5;
           break;
       case 0: // child
           execvp(file, argv);
           break;
       default: // parent
           {
               int status;
               if (wait(&status) == -1)
               {
                   fprintf(stderr, "ERROR: child process failed with status %d\n", status);
                   rc = 6;
               }
           }
           break;
    }

    return rc;
}

//****************************************************************************
/// @brief  Get whatever the temporary path is set to.
//****************************************************************************
void test_utils_getTempPath(char *tempDirectory);

//****************************************************************************
/// @brief  Copy key configuration files from the config source directory to
///         the specified configuration directory.
///
/// @param[in]     targetDir  Target directory for the configuration.
//****************************************************************************
void test_copyConfigFiles(char *targetDir);

//****************************************************************************
/// @brief  Update the config properties with the ones read during startup
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_setConfigProperties(bool dynamic);

//****************************************************************************
/// @brief  Dump memory stats before an event
///
/// @param[in]     filenamePrefix  (Optional) filename prefix
/// @param[in]     pDumpCount      Dump counter to include & increment
///
/// @remark If no prefix is specifed, /var/tmp/memoryDumpStats_before is used.
//****************************************************************************
void test_memoryDumpStats_before(const char *filenamePrefix, uint32_t *pDumpCount);

//****************************************************************************
/// @brief  Dump memory stats after an event
///
/// @param[in]     filenamePrefix  (Optional) filename prefix
/// @param[in]     dumpCount       Dump counter to include
///
/// @remark If no prefix is specifed, /var/tmp/memoryDumpStats_after is used.
//****************************************************************************
void test_memoryDumpStats_after(const char *filenamePrefix, uint32_t dumpCount);

//****************************************************************************
/// @brief  Tell the test utils code that the global admin dir it is using is
/// a temporary one (probably passed from a previous phase)
///
/// @param[in]     adminDir        Used to confirm the directory name
//****************************************************************************
void test_process_setGlobalTempAdminDir(const char *adminDir);

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
int32_t test_processInit(int32_t traceLevel, char *adminDir);

//****************************************************************************
/// @brief  Perform process wide termination processing for a test program
///
/// @param[in]  removeTempAdmin  If the test created a temporary admin directory,
///                              whether to remove it.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_processTerm(bool removeTempAdmin);

//****************************************************************************
/// @brief  Return a copy of the global admin directory if there is one.
///
/// @param[in,out]  buffer      Buffer to fill in.
/// @param[in]      bufferLen   Size of the passed in buffer.
///
/// @return true if an admin dir was found
//****************************************************************************
bool test_getGlobalAdminDir(char *buffer, size_t bufferLen);

//****************************************************************************
/// @brief  Remove the contents of the specified directory and all subdirectories
///
/// @param[in]  adminDir  The admin directory to remove.
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
void test_removeAdminDir(char *adminDir);

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
                               int32_t totalStoreMemSizeMB);

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
                             uint64_t *clusterStartTime,
                             uint64_t *engineInitTime);

//****************************************************************************
/// @brief  Perform the start portion of test_engineInit
///
/// @param[in,out] storeStartTime   How long ism_store_start took
/// @param[in,out] clusterStartTime How long ism_cluster_start took
/// @param[in,out] engineStartTime  How long ism_engine_start took
/// @param[in]     callThreadInit   Whether to call ism_engine_threadInit
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_engineInit_Start(uint64_t *storeStartTime,
                              uint64_t *clusterStartTime,
                              uint64_t *engineStartTime,
                              bool callThreadInit,
                              bool allowFailureToStart);

//****************************************************************************
/// @brief  Perform initialization of the engine/store components
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
int32_t test_setStoreMgmtGranuleSizes(uint32_t smallSizeBytes,
                                      uint32_t sizeBytes);

//****************************************************************************
/// @brief  Perform initialization of the engine/store components
///
/// @param[in]  coldStart                 Whether to perform a store cold-start or not.
/// @param[in]  disableAuthorization      Whether authorization (and authority checks)
///                                       are disabled
/// @param[in]  disableAutoQueueCreation  Whether automatic named queue creation is
///                                       disabled
/// @param[in]  subListCacheCapacity      Subscriber list cache capacity to use
/// @param[in]  totalStoreMemSizeMB       The size to initialize the store to (use
///                                       testDEFAULT_STORE_SIZE for normal tests)
///
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
#define testDEFAULT_STORE_SIZE 256 ///< The default value for Store.TotalMemSizeMB used by test_engineInit

int32_t test_engineInit(bool coldStart,
                        bool disableAuthorization,
                        bool disableAutoQueueCreation,
                        bool allowFailureToStart,
                        uint32_t subListCacheCapacity,
                        int32_t totalStoreMemSizeMB);

#define test_engineInit_DEFAULT test_engineInit(true, true, \
                                                ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION,   \
                                                false, /*recovery should complete ok*/           \
                                                ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY, \
                                                testDEFAULT_STORE_SIZE)

#define test_engineInit_COLD test_engineInit(true, true, \
                                             ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION, \
                                             false, /*recovery should complete ok*/           \
                                             ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY, \
                                             testDEFAULT_STORE_SIZE)

#define test_engineInit_COLDAUTO test_engineInit(true, true, \
                                                 false, \
                                                 false, /*recovery should complete ok*/           \
                                                 ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY, \
                                                 testDEFAULT_STORE_SIZE)

#define test_engineInit_WARM test_engineInit(false, true, \
                                             ismENGINE_DEFAULT_DISABLE_AUTO_QUEUE_CREATION, \
                                             false, /*recovery should complete ok*/           \
                                             ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY, \
                                             testDEFAULT_STORE_SIZE)

#define test_engineInit_WARMAUTO test_engineInit(false, true, \
                                                 false, \
                                                 false, /*recovery should complete ok*/           \
                                                 ismENGINE_DEFAULT_INITIAL_SUBLISTCACHE_CAPACITY, \
                                                 testDEFAULT_STORE_SIZE)

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
int32_t test_engineTerm(bool dropStore);

//****************************************************************************
/// @brief  Simulate a stop and restart of the engine/store components
/// @return OK on successful completion or an ISMRC_ value.
//****************************************************************************
int32_t test_bounceEngine(void);

//****************************************************************************
/// @brief  Invoke execv after any required tidy up
//****************************************************************************
int test_execv(const char *file, char *const argv[]);

//****************************************************************************
/// @brief  Kill the current process forcing it to take a core and optionally
///         invoking another instance of itself.
//****************************************************************************
void test_kill_me(const char *file, char *const argv[]);

//Global variable so we can easily tell if diskPersistence is enabled (set during engine start once we've read config)
extern bool usingDiskPersistence;

//Global variable so we can easily call a function between the initialization and start phases
//of test_engineInit -- thie means we can do things like use the REST interface to set configuration
// properties that need to be set before the engine starts, but after the admin component has
// initialized (where the REST interface schema is loaded).
typedef int (*test_engineInit_betweenInitAndStartFunc_t)(void);
extern test_engineInit_betweenInitAndStartFunc_t test_engineInit_betweenInitAndStartFunc;

#endif //end ifndef __ISM_TEST_UTILS_INITTERM_H_DEFINED
