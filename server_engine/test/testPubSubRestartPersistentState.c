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
/* Module Name: testPubSubRestartPersistentState                     */
/*                                                                   */
/* Description: Part of testPubSubRestart that records state info to */
/* shared memory or to queues to survive a restart of the engine     */
/*                                                                   */
/* Shared memory is faster and doesn't rely on the ISM for storage   */
/* but doesn't survive a real restart of the box test                */
/*                                                                   */
/*                                                                   */
/*********************************************************************/

#include <stdbool.h>
#include <sys/mman.h>        /* shm_* stuff, and mmap() */
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <limits.h>
#include <assert.h>

#include "engineInternal.h"
#include "test_utils_log.h"
#include "test_utils_assert.h"
#include "test_utils_initterm.h"

#include "testPubSubRestart.h"

//Add the uid on the name of the shared memory set
#define SHMOBJ_PATH "/ism-test-testPubSubRestart::%u"

typedef struct tag_testState_t
{
    char StructId[4]; ///< TSHM
    uint32_t numPublishers;
    uint32_t numSubscribers;
    bool disablingSubbersAllowed; ///< Set to false when first "important" sub finishes
    //Have an array of publisher data with numPublisher elements
    persistablePublisherState_t pubbers[];
    //Afterwards have an array of subber data with numSubscribers elements
    //subscriberStateShm_t subbers[];
} testState_t;

#define TESTSTATE_STRUCTID "TSTE"

static testState_t *testState;
static bool useShm = true;


void shmRequired(void)
{
    //The non-SHM persistence mechanism that this test used to use was removed to
    //flaws in the way it used the store.... a new non-SHM mechanism could be added by
    //writing code whereever this function is called...
    fprintf(stderr, "At the moment SHM is required....\n");
    exit(10);
}

static uint64_t calcTestStateMemSize(  uint64_t numPublishers
                                    ,  uint64_t numSubscribers)
{
    uint64_t mem_size = sizeof(testState_t)
                         + (numPublishers * sizeof(persistablePublisherState_t))
                         + (numSubscribers * (  sizeof(persistableSubscriberState_t)
                                               + (numPublishers*sizeof(subInfoPerPublisher_t))));

    test_log(testLOGLEVEL_TESTPROGRESS, "calcTestStateMemSize: %lu bytes",mem_size);

    return mem_size;
}

static void initialiseTestMemoryState(  uint64_t numPublishers
                                     ,  uint64_t numSubscribers)
{
    uint64_t memSize = calcTestStateMemSize( numPublishers
                                             , numSubscribers);

    testState = calloc(1, memSize);

    TEST_ASSERT(testState != NULL, ("Unable to allocate %u bytes from teststate", memSize));

    testState->numPublishers = numPublishers;
    testState->numSubscribers = numSubscribers;
}

static void createSharedMem( bool keepMemAfterExit
                           , uint64_t numPublishers
                           , uint64_t numSubscribers)
{
    char shmName[NAME_MAX + 1] = "";
    int shmfd;
    uint64_t shared_seg_size = calcTestStateMemSize(numPublishers, numSubscribers);

    /* creating the shared memory object    --  shm_open()  */
    sprintf(shmName, SHMOBJ_PATH, getuid());
    char *qualifyShared = getenv("QUALIFY_SHARED");
    if (qualifyShared != NULL) strcat(shmName, qualifyShared);
    shmfd = shm_open(shmName, O_CREAT | O_TRUNC | O_RDWR, S_IRWXU | S_IRWXG);
    if (shmfd < 0) {
        perror("In shm_open()");
        exit(1);
    }
    test_log(testLOGLEVEL_TESTPROGRESS, "Created shared memory object %s", shmName);

    /* adjusting mapped file size (make room for the whole segment to map)      --  ftruncate() */
    int ftr_rc = ftruncate(shmfd, shared_seg_size);

    if (ftr_rc != 0) {
        perror("In ftruncate()");
        exit(1);
    }

    /* requesting the shared segment    --  mmap() */
    testState = (testState_t *)mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (testState == NULL) {
        perror("In mmap()");
        exit(1);
    }
    test_log(testLOGLEVEL_TESTPROGRESS,  "Shared memory segment allocated correctly (%lu bytes).", shared_seg_size);

    ismEngine_SetStructId(testState->StructId, TESTSTATE_STRUCTID);
    testState->numPublishers = numPublishers;
    testState->numSubscribers = numSubscribers;

    /* Ask for the segment to be destroyed when we exit if we are not going to restart */
    if (!keepMemAfterExit)
    {
        if (shm_unlink(shmName) != 0)
        {
            perror("In shm_unlink()");
            exit(1);
        }
    }
}

static void reconnectSharedMem( bool keepMemAfterExit
                              , uint64_t numPublishers
                              , uint64_t numSubscribers)
{
    char shmName[NAME_MAX + 1] = "";
    int shmfd;
    uint64_t shared_seg_size = calcTestStateMemSize(numPublishers, numSubscribers);

    /* reconnect to existing shared memory object    --  shm_open()  */
    sprintf(shmName, SHMOBJ_PATH, getuid());
    char *qualifyShared = getenv("QUALIFY_SHARED");
    if (qualifyShared != NULL) strcat(shmName, qualifyShared);
    shmfd = shm_open(shmName, O_RDWR, S_IRWXU | S_IRWXG);
    if (shmfd < 0)
    {
        perror("In shm_open()");
        exit(1);
    }
    test_log(testLOGLEVEL_TESTPROGRESS, "reconnected to shared memory object %s\n", shmName);


    /* requesting the shared segment    --  mmap() */
    testState = (testState_t *)mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (testState == NULL)
    {
        perror("In mmap()");
        exit(1);
    }
    test_log(testLOGLEVEL_TESTPROGRESS,  "Shared memory segment mapped correctly (%lu bytes).\n", shared_seg_size);

    ismEngine_CheckStructId(testState->StructId, TESTSTATE_STRUCTID, 1);
    TEST_ASSERT(testState->numPublishers == numPublishers,
                ("testState->numPublishers (%d) != numPublishers (%d)", testState->numPublishers, numPublishers));

    if (!keepMemAfterExit)
    {
        /* Ask for the segment to be destroyed when we exit */
        if (shm_unlink(shmName) != 0)
        {
            perror("In shm_unlink()");
            exit(1);
        }
    }
}

persistableSubscriberState_t *getFirstSubberPersState(void)
{
    persistableSubscriberState_t *persState = NULL;

    persState = (persistableSubscriberState_t *)(  ((char *)testState->pubbers)
                 +(testState->numPublishers * sizeof(persistablePublisherState_t)) );
    return persState;
}

persistableSubscriberState_t *getNextSubberPersState(persistableSubscriberState_t *curr)
{
    persistableSubscriberState_t *persState = NULL;

    persState = (persistableSubscriberState_t *)( ((char *)curr)
                     + sizeof(persistableSubscriberState_t)
                     + ((testState->numPublishers) * sizeof(subInfoPerPublisher_t)));

    return persState;
}

persistableSubscriberState_t *getSubberPersState(uint32_t subberNum)
{
    persistableSubscriberState_t *firstSubber  = getFirstSubberPersState();

    persistableSubscriberState_t *thisSubber = (persistableSubscriberState_t *)(((char *)firstSubber)
                                                 + (subberNum * (  sizeof(persistableSubscriberState_t)
                                                                 + ((testState->numPublishers) * sizeof(subInfoPerPublisher_t)))));
    return thisSubber;
}

persistablePublisherState_t *getFirstPubberPersState(void)
{
    persistablePublisherState_t *persState = NULL;

    persState = &(testState->pubbers[0]);

    return persState;
}

persistablePublisherState_t *getNextPubberPersState(persistablePublisherState_t *curr)
{
    persistablePublisherState_t *persState = NULL;

    persState = curr+1;

    return persState;
}

//SharedMemory is "persistent" without such an operation, so we only need this for non-SHM,,,
void persistSubberState(subscriberStateComplete_t *subber)
{
    if (!useShm)
    {
        shmRequired();
    }
}


//sharedMemory is "persistent" without such an operation, so we only need this for non-SHM,,,
void persistPubberState( ismEngine_SessionHandle_t hSession
                       , persistenceLayerPrivateData_t *persLayerData
                       , persistablePublisherState_t *pubState)
{
    if (!useShm)
    {
        shmRequired();
    }
}

void initialiseTestState( bool keepMemAfterExit
                        , uint64_t numPublishers
                        , uint64_t numSubscribers)
{
    if (useShm)
    {
        createSharedMem( keepMemAfterExit
                       , numPublishers
                       , numSubscribers);
    }
    else
    {
        initialiseTestMemoryState( numPublishers
                                 , numSubscribers);
    }
}

void restoreTestState( bool keepMemAfterExit
                     , uint64_t numPublishers
                     , uint64_t numSubscribers)
{
    if (useShm)
    {
        reconnectSharedMem( keepMemAfterExit
                          , numPublishers
                          , numSubscribers );
    }
    else
    {
        initialiseTestMemoryState( numPublishers
                                 , numSubscribers);

        shmRequired();
    }
}

void backupPersistedState(void)
{
    char *qualifyShared = getenv("QUALIFY_SHARED");

    // space for QUALIFY_SHARED value and com.ibm.ism::%u:store (and null terminator)
    char shmName[(qualifyShared?strlen(qualifyShared):0)+41];
    sprintf(shmName, "com.ibm.ism::%u:store%s", getuid(), qualifyShared ? qualifyShared : "");

    // backup the Store
    char backupDir[500] = {0};
    test_utils_getTempPath(backupDir);
    strcat(backupDir, "/storeBackup");

    // space for /dev/shm (and null terminator)
    char sourceName[strlen(shmName)+10];
    sprintf(sourceName, "/dev/shm/%s", shmName);

    // space for / (and null terminator)
    char backupName[strlen(backupDir)+strlen(shmName)+2];
    sprintf(backupName, "%s/%s", backupDir, shmName);

    // Allow space for 50 bytes of command (and null terminator)
    char cmd[strlen(sourceName)+strlen(backupName)+51];
    struct stat statBuf;

    if (stat(backupDir, &statBuf) != 0)
    {
        (void)mkdir(backupDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    if (stat(sourceName, &statBuf) == 0)
    {
        sprintf(cmd, "/bin/cp -f %s %s", sourceName, backupName);
        if (system(cmd) == 0)
        {
            printf("Copied store shm %s to %s\n", sourceName, backupName);
        }
    }

    if (useShm)
    {
        sprintf(shmName, "ism-test-testPubSubRestart::%u", getuid());
        if (qualifyShared != NULL) strcat(shmName, qualifyShared);
        sprintf(sourceName, "/dev/shm/%s", shmName);
        sprintf(backupName, "%s/%s", backupDir, shmName);

        if (stat(backupDir, &statBuf) != 0)
        {
            (void)mkdir(backupDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }

        if (stat(sourceName, &statBuf) == 0)
        {
            sprintf(cmd, "/bin/cp -f %s %s", sourceName, backupName);
            if (system(cmd) == 0)
            {
                printf("Copied test shm %s to %s\n", sourceName, backupName);
            }
        }
    }
}

void useSharedMemoryForPersistence(bool shmAllowed)
{
    if(!shmAllowed)
    {
       shmRequired();
    }
}
