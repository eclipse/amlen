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
/* Module Name: engineStats.c                                        */
/*                                                                   */
/* Description: Engine component performance test                    */
/*                                                                   */
/*********************************************************************/

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>

#include <ismutil.h>
#include <workers.h>
#include <engine.h>
#include "engineInternal.h"
#include "engineStats.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

//Also see #undef DEBUG_ENGINE_STATS at end of file
//#define DEBUG_ENGINE_STATS

volatile uint32_t releaseMessageCallbackCnt = 0;
volatile uint32_t putMessageCallbackCnt = 0;
volatile uint32_t submitGetMessageCnt = 0;
volatile uint32_t submitPutMessageCnt = 0;


void *hClient = NULL;


/**********************************************************************************/
/*                                                                                */
/* void print_usage(char *buf, char *buf2)                                        */
/*                                                                                */
/* Prints a usage statement to stdout                                             */
/*                                                                                */
/**********************************************************************************/
void print_usage(char *errorMsg1, char *errorMsg2, bool verbose)
{

    if (errorMsg1) {
        printf("engineStats %s >>> PARSE ERROR! ", errorMsg1);
        if (errorMsg2) {
            printf(" %s", errorMsg2);
        }
        printf("\n\n");
    }

    printf("Valid parameters:  [-prime <msg count>]\n");
    printf("                   [-m|-msgs <msg count>]\n");
    printf("                   [-p|-put <threads per queue> <sessions per thread> <tran size>]\n");
    printf("                   [-g|-get <threads per queue> <sessions per thread> <tran size>]\n");
    printf("                   [-q|-queues <num queues>]\n");
    printf("                   [-measure [<iterations>]]\n");
    printf("                   [-persistent|-pers]\n");
    printf("                   [-h|-?|-help]\n");
    printf("                   [-v|-verbose]\n");
    printf("\n");

    if ((!errorMsg1) && (verbose)) {
        printf("\nDetailed parameter explanation:\n");
        printf("\n");
        printf("  -prime <msg count>\n");
        printf("              Where <msg count> is the number of messages put and\n");
        printf("              retrieved from engine before test begins.\n");
        printf("              Must specify an integer value greater than 0.\n");
        printf("              Default value is 0.\n");
        printf("\n");
        printf("  -m|-msgs <msg count>\n");
        printf("              May specify -m or -msgs\n");
        printf("              Where <msg count> is the number of messages used for\n");
        printf("              performance measurement.\n");
        printf("              Must specify an integer value greater than 0.\n");
        printf("              Default value is %d.\n", DEFAULT_MESSAGE_COUNT);
        printf("\n");
        printf("  -q|-queues <num queues>\n");
        printf("              May specify -q or -queues\n");
        printf("              Specify number of queues used in the performance measurement.\n");
        printf("              Must specify an integer value greater than 0.\n");
        printf("              Default value is %d.\n", DEFAULT_NUM_QUEUES);
        printf("\n");
        printf("  -p|-put <threads per queue> <sessions per thread> <tran size>]\n");
        printf("              May specify -p or -put\n");
        printf("              Measure producer messages to engine\n");
        printf("              Specify number of messages put within each transaction.\n");
        printf("              and number of producer sessions \n");
        printf("              By default there is one message per transaction\n");
        printf("              and a single producer thread per queue.\n");
        printf("              If the transaction size is 0, no transaction is used.\n");
        printf("\n");
        printf("  -g|-get <threads per queue> <sessions per thread> <tran size>]\n");
        printf("              May specify -g or -get\n");
        printf("              Measure consumed messages from engine\n");
        printf("              Specify number of messages to get within each transaction.\n");
        printf("              and number of consumer sessions or the word 'shared' \n");
        printf("              By default there is one message per transaction\n");
        printf("              and a single consumer thread per queue.\n");
        printf("              If the transaction size is 0, no transaction is used.\n");
        printf("\n");
        printf("  -measure [<iterations>]\n");
        printf("              Where [<iterations>] is the number of iterations an action is \n");
        printf("              performed for the average measurements.\n");
        printf("              NOTE:  THIS IS A SEPARATE, INDEPENDENT MODE OF THIS COMMAND!\n");
        printf("              Instead of the a producer/consumer message test, predefined jobs\n");
        printf("              are submitted to a worker thread.  Each job executes a section\n");
        printf("              of code iteratively in a loop.  The average execution time is\n");
        printf("              reported in nanoseconds(ns).\n");
        printf("              May specify an _optional_ integer value greater than 0.\n");
        printf("              Default value is %'ld.\n", (long)DEFAULT_MEASURE_ITERATIONS);
        printf("\n");
        printf("  -persistent|-pers\n");
        printf("              May specify -persistent or -pers\n");
        printf("              Use persistent messages.\n");
        printf("\n");
        printf("  -h|-?|-help\n");
        printf("              May specify -h, -? or -help\n");
        printf("              Display command usage.\n");
        printf("              Include -verbose for additional help.\n");
        printf("\n");
        printf("  -v|-verbose\n");
        printf("              May specify -v or -verbose\n");
        printf("              Display additional output.\n");
        printf("\n\n");
        printf("Examples:\n");
        printf("\n");
        printf("  engineStats\n");
        printf("     Measures engine performance for put and get of %'d messages.\n", DEFAULT_MESSAGE_COUNT);
        printf("     One message is put within each put transaction.\n");
        printf("     One message is retrieved within each get transaction.\n");
        printf("     One session shared by both put and get.\n");
        printf("\n");
        printf("  engineStats -msgs 1000000\n");
        printf("     Measures engine performance for put and get of 1,000,000 messages.\n");
        printf("     One message is put within each put transaction.\n");
        printf("     One message is retrieved within each get transaction.\n");
        printf("     One session shared by both put and get.\n");
        printf("\n");
        printf("  engineStats -put 1 1 1\n");
        printf("     Measures engine performance for put of %'d messages.\n", DEFAULT_MESSAGE_COUNT);
        printf("     One message is put within each put transaction.\n");
        printf("\n");
        printf("  engineStats -prime 100000 -msgs 1000000 -put 1 3 2 -get 1 5 4\n");
        printf("     100,000 messages are put and retrieved from engine before test begins.\n");
        printf("     Measures engine performance for put and get of 1,000,000 messages.\n");
        printf("     Two message are put within each put transaction.\n");
        printf("     There are three put sessions, i.e. three producers.\n");
        printf("     Four messages are retrieved within each get transaction.\n");
        printf("     There are five get sessions, i.e. five consumers.\n");
        printf("\n\n");
        printf("Expected Output:\n");
        printf("     Put&Get: ### msgs in ### ms, Rate: ### msg/s, ### ns/msg\n");
        printf("\n");
        printf("     Output consists of a single line that specifies\n");
        printf("     - what actions were measured, Put, Get, or Put & Get\n");
        printf("     - number of messages measured\n");
        printf("     - length of time the test ran in ms\n");
        printf("     - message rate in messages/second\n");
        printf("     - message rate in ns/message\n");
        printf("\n");
        printf("Example Output:\n");
        printf("     Put&Get: 100,000 msgs in 1,465.386 ms, Rate: 68,241 msg/s, 14,653 ns/msg\n");
        printf("     Both put and get were measured for 100,000 messages\n");
        printf("     The test ran for 1,465.386 ms. The computed rate is 68,241 msg/s\n");
        printf("     and each message \"round trip\" on average took 14,653 ns\n");
        printf("\n\n");
    } else {
        printf("Specify -help -verbose for detailed parameter explanation.\n");
    }

}

/******************************************************************************************/
/*                                                                                        */
/* void printConfig(config_t config)                                                      */
/*                                                                                        */
/* Prints configuration data to stdout                                                    */
/*                                                                                        */
/******************************************************************************************/
void printConfig(config_t config)
{
    printf("config.prime=%d\n", config.prime);
    printf("config.count=%d\n", config.count);
    printf("config.numProducers=%d\n", config.numProducers);
    printf("config.prodTransSize=%d\n", config.prodTransSize);
    printf("config.prodSessions=%d\n", config.prodSessions);
    printf("config.numConsumers=%d\n", config.numConsumers);
    printf("config.consTransSize=%d\n", config.consTransSize);
    printf("config.consSessions=%d\n", config.consSessions);
    printf("config.measure=%d\n", config.measure);
    printf("config.measureIterations=%d\n", config.measureIterations);
    printf("config.help=%s\n", (config.help == false) ? "false" : "true");
    printf("config.verbose=%s\n\n", (config.verbose == false) ? "false" : "true");
}
/******************************************************************************************/
/*                                                                                        */
/* int parseArgs(char *args, config_t *config)                                            */
/*                                                                                        */
/* Parses command line parameters into the config_t structure.                            */
/*                                                                                        */
/******************************************************************************************/
int parseArgs(char *args, config_t *config)
{
    char errorMsg1[1024] = {0};
    char errorMsg2[1024] = {0};
    char * arg = NULL;
    char * arg2 = NULL;
    char * arg3 = NULL;
    char * arg4 = NULL;

    arg = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);

    config->numProducers = 0;
    config->numConsumers = 0;

    while (arg != NULL) {
        strcat(errorMsg1, arg);
        strcat(errorMsg1, " ");

        if (!strcmpi(arg, "-verbose") || !strcmpi(arg, "-v")) {
            config->verbose = true;
        } else if (!strcmpi(arg, "-help") || !strcmpi(arg, "-h") || !strcmpi(arg, "-?")) {
            config->help = true;
        } else if (!strcmpi(arg, "-measure")) {
            config->measure = true;
            config->measureIterations = DEFAULT_MEASURE_ITERATIONS;

            arg2 = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            if (arg2 != NULL) {
                strcat(errorMsg1, arg2);
                strcat(errorMsg1, " ");
                config->measureIterations = atol(arg2);
            }

        } else if (!strcmpi(arg, "-put") || !strcmpi(arg, "-p")) {
            config->numProducers = 0;
            config->prodSessions = 0;
            config->prodTransSize = 0;

            arg2 = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            if (arg2 != NULL) {
                strcat(errorMsg1, arg2);
                strcat(errorMsg1, " ");
                config->numProducers = atol(arg2);
            }

            if (config->numProducers <= 0) {
                sprintf(errorMsg2, "'%s' must be followed by a number greater than 0.", arg);
                print_usage(errorMsg1, errorMsg2, config->verbose);
                return -1;
            }

            arg3 = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            if (arg3 != NULL) {
                strcat(errorMsg1, arg3);
                strcat(errorMsg1, " ");
                config->prodSessions = atol(arg3);
            }

            if (config->prodSessions <= 0) {
                sprintf(errorMsg2, "'%s %s' must be followed by a number greater than 0.", arg, arg2);
                print_usage(errorMsg1, errorMsg2, config->verbose);
                return -1;
            }

            arg4 = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            if (arg4 != NULL) {
                strcat(errorMsg1, arg4);
                strcat(errorMsg1, " ");
                config->prodTransSize = atol(arg4);
            }

            if (config->prodTransSize < 0) {
                sprintf(errorMsg2, "'%s %s %s' must be followed by a number greater than or equal to 0.", arg, arg2, arg3);
                print_usage(errorMsg1, errorMsg2, config->verbose);
                return -1;
            }

        } else if (!strcmpi(arg, "-get") || !strcmpi(arg, "-g")) {
            config->numConsumers = 0;
            config->consSessions = 0;
            config->consTransSize = 0;

            arg2 = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            if (arg2 != NULL) {
                strcat(errorMsg1, arg2);
                strcat(errorMsg1, " ");
                config->numConsumers = atol(arg2);
            }

            if (config->numConsumers <= 0) {
                sprintf(errorMsg2, "'%s' must be followed by a number greater than 0.", arg);
                print_usage(errorMsg1, errorMsg2, config->verbose);
                return -1;
            }

            arg3 = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            if (arg3 != NULL) {
                strcat(errorMsg1, arg3);
                strcat(errorMsg1, " ");
                config->consSessions = atol(arg3);
            }

            if (config->consSessions <= 0) {
                sprintf(errorMsg2, "'%s' must be followed by a number greater than 0.", arg);
                print_usage(errorMsg1, errorMsg2, config->verbose);
                return -1;
            }
            arg4 = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            if (arg4 != NULL) {
                strcat(errorMsg1, arg4);
                strcat(errorMsg1, " ");
                config->consTransSize = atol(arg4);
            }

            if (config->consTransSize < 0) {
                sprintf(errorMsg2, "'%s %s %s' must be followed by a number greater than or equal to 0.", arg, arg2, arg3);
                print_usage(errorMsg1, errorMsg2, config->verbose);
                return -1;
            }
        } else if (!strcmpi(arg, "-msgs") || !strcmpi(arg, "-m")) {
            arg2 = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            if (arg2 != NULL) {
                strcat(errorMsg1, arg2);
                strcat(errorMsg1, " ");
                config->count = atol(arg2);
            }

            if (config->count <= 0) {
                sprintf(errorMsg2, "'%s' must be followed by a number greater than 0.", arg);
                print_usage(errorMsg1, errorMsg2, config->verbose);
                return -1;
            }
        } else if (!strcmpi(arg, "-q") || !strcmpi(arg, "-queues")) {
            arg2 = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            if (arg2 != NULL) {
                strcat(errorMsg1, arg2);
                strcat(errorMsg1, " ");
                config->queues = atol(arg2);
            }

            if (config->queues <= 0) {
                sprintf(errorMsg2, "'%s' must be followed by a number greater than 0.", arg);
                print_usage(errorMsg1, errorMsg2, config->verbose);
                return -1;
            }
        } else if (!strcmpi(arg, "-prime")) {
            arg2 = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
            if (arg2 != NULL) {
                strcat(errorMsg1, arg2);
                strcat(errorMsg1, " ");
                config->prime = atol(arg2);
            }

            if (config->prime <= 0) {
                sprintf(errorMsg2, "'%s' must be followed by a number greater than 0.", arg);
                print_usage(errorMsg1, errorMsg2, config->verbose);
                return -1;
            }
        } else if (!strcmpi(arg, "-persistent") || !strcmpi(arg, "-pers")) {
            config->persistent = true;

        } else {
            sprintf(errorMsg2, "'%s' is not a valid option.", arg);
            print_usage(errorMsg1, errorMsg2, config->verbose);
            return -1;
        }

        arg = ism_common_getToken(args, " \t\r\n", " \t\r\n", &args);
    }

    if (config->help) {
        print_usage(NULL, NULL, config->verbose);
        return 0;
    }

    if ((config->numProducers == 0) && (config->numConsumers == 0)) {
        config->numProducers = 1;
        config->numConsumers = 1;
    }

    return 0;
}

/*
 * Get the number of CPU threads available
 */
static int getHardwareNumCPUs(void)
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}

void setQueueName(int queuenumber, char *qnamebuf)
{
    sprintf(qnamebuf, "%s%d",QUEUE_NAME_START,queuenumber);
}

/*
 * This function locks the thread to a given CPU.
 * The weird name implies that CPU numbers count down from the
 * "last" CPU (CPU 1 = last) so it is less likely to conflict with
 * worker threads (better names welcomed ;)
 *
 * If this function is given CPU 0, it does nothing
 */
static void runOnlyOnCPUReversed(int cpunum)
{
    char CPUs[128] = {0};

    if(cpunum != 0) {
        int hardwareNumCPUs = getHardwareNumCPUs();
        if (hardwareNumCPUs == -1) {
            printf("ERROR:  getHardwareNumCPUs returned %d", hardwareNumCPUs);
        } else {
            int hardwareCPU = hardwareNumCPUs - cpunum;
            if ((hardwareCPU >= 0) && (hardwareCPU < sizeof(CPUs))) {
                CPUs[hardwareCPU] = 1;
                ism_common_setAffinity(ism_common_threadSelf(), CPUs, sizeof(CPUs));
            }
        }
    }
}

/**************** Small utility funcs for communicating between threads */
static testStatus_t globalStatus = TESTSTATUS_INITIAL;

//Called by each worker thread when they're ready for the next phase
int threadFinishedPhase(testPhase_t currPhase)
{
    int rc;

    //Shouldn't have changed phase as we haven't signalled we're ready
if(currPhase != globalStatus.phase) {
printf("thread phase %d, global phase %d",currPhase, globalStatus.phase );
}
    assert(currPhase == globalStatus.phase);

    //Broadcast that this thread has finished the current phase
    rc = pthread_mutex_lock(&(globalStatus.numthreads_mutex));

    if (OK == rc) {
        globalStatus.numThreadsCompletedPhase++;

        rc = pthread_cond_broadcast(&(globalStatus.numthreads_cond));

        int rc2 = pthread_mutex_unlock(&(globalStatus.numthreads_mutex));
        if ((rc2 != OK)&&(rc == OK)) {
            rc = rc2;
        }
    }

    //wait for the next phase to start
    if (rc == OK) {
        rc = pthread_mutex_lock(&(globalStatus.phase_mutex));

        while ((rc== OK) && (globalStatus.phase == currPhase)) {
            rc = pthread_cond_wait(&(globalStatus.phase_cond), &(globalStatus.phase_mutex));
        }
        int rc2 = pthread_mutex_unlock(&(globalStatus.phase_mutex));

        if ((rc2 != OK)&&(rc == OK)) {
            rc = rc2;
        }
    }

    return rc;
}

//Called by the main thread to move to the next phase when the worker threads
//have signalled that they've finished
int startNextPhaseWhenReady(int threadsExpected,
                            testPhase_t newPhase,
                            ism_time_t *phaseStartTime)
{
    int rc = OK;

    //wait for the required number of threads to be ready
    rc = pthread_mutex_lock(&(globalStatus.numthreads_mutex));

    while ((rc== OK) && (globalStatus.numThreadsCompletedPhase < threadsExpected)) {
        rc = pthread_cond_wait(&(globalStatus.numthreads_cond), &(globalStatus.numthreads_mutex));
    }

    int rc2 = pthread_mutex_unlock(&(globalStatus.numthreads_mutex));

    if ((rc2 != OK)&&(rc == OK)) {
        rc = rc2;
    }

    //Record the time we started the next phase
    *phaseStartTime = ism_common_currentTimeNanos();

    //Broadcast that everyone has finished the phase and we're moving on
    if (rc == OK ) {
        rc = pthread_mutex_lock(&(globalStatus.phase_mutex));

        if (OK == rc) {
            globalStatus.numThreadsCompletedPhase = 0;
            globalStatus.phase = newPhase;

            rc = pthread_cond_broadcast(&(globalStatus.phase_cond));

            int rc2 = pthread_mutex_unlock(&(globalStatus.phase_mutex));
            if ((rc2 != OK)&&(rc == OK)) {
                rc = rc2;
            }
        }
    }
    return rc;
}


void createMutexSessionTrans(int numSessions,
                             session_handles_t handles[],
                             bool verbose)
{
    ismCALLBACK createCB[numSessions];
    int i;

    /* Setup a mutex for each session*/
    pthread_mutexattr_t mutexattrs;
    /* Ensure that we have defined behaviour in the case of relocking a mutex */
    pthread_mutexattr_init(&mutexattrs);
    pthread_mutexattr_settype(&mutexattrs, PTHREAD_MUTEX_NORMAL);

    for (i = 0; i < numSessions; i++) {
        pthread_mutex_init(&(handles[i].sessionMutex), &mutexattrs);
    }
    /* Setup the sessions for this thread */
    for (i = 0; i < numSessions; i++) {
        handles[i].hSession = NULL;
        createCB[i].callbackFunction = createSessionCallback;
        createCB[i].pContext = (void *)&(handles[i]);
        pthread_mutex_lock(&(handles[i].sessionMutex));
        (void)ism_engine_createSession(hClient, &(createCB[i]));
    }

    /* Wait for the sessions to be created */
    for (i = 0; i < numSessions; i++) {
        pthread_mutex_wait(&(handles[i].sessionMutex));
        if (verbose) {
            printf("handles[%d].hSession is %p\n", i, handles[i].hSession);
        }
    }

    /* Setup the transactions for this thread */
    for (i = 0; i < numSessions; i++) {
        handles[i].hTran = NULL;
        createCB[i].callbackFunction = createLocalTransactionCallback;
        createCB[i].pContext = (void *)&(handles[i]);
        pthread_mutex_lock(&(handles[i].sessionMutex));
        (void)ism_engine_createLocalTransaction(handles[i].hSession, &(createCB[i]));
    }

    /* Wait for the transactions to be created */
    for (i = 0; i < numSessions; i++) {
        pthread_mutex_wait(&(handles[i].sessionMutex));
        if (verbose) {
            printf("handles[%d].hTran is %p\n", i, handles[i].hTran);
        }
    }
}

void destroyMutexSessionTrans(int numSessions, session_handles_t handles[])
{
    ismCALLBACK destroyCB[numSessions];
    int i;

    /* Destroy the sessions for this thread */
    for (i = 0; i < numSessions; i++) {
        destroyCB[i].callbackFunction = destroySessionCallback;
        destroyCB[i].pContext = (void *)&(handles[i]);
        pthread_mutex_lock(&(handles[i].sessionMutex));
        (void)ism_engine_destroySession(handles[i].hSession, &(destroyCB[i]));
    }

    /* Wait for the sessions to be destroyed */
    for (i = 0; i < numSessions; i++) {
        pthread_mutex_wait(&(handles[i].sessionMutex));
    }

    /* Finally destroy the mutexes */
    for (i = 0; i < numSessions; i++) {
        pthread_mutex_destroy(&(handles[i].sessionMutex));
    }
}


void createProducers(int numSessions,
                     char *qname,
                     session_handles_t producer[],
                     bool verbose)
{
    ismCALLBACK createCB[numSessions];
    int i;

    for (i = 0; i < numSessions; i++) {
        producer[i].hProducer = NULL;
        createCB[i].callbackFunction = createProducerCallback;
        createCB[i].pContext = (void *)&(producer[i]);

        pthread_mutex_lock(&(producer[i].sessionMutex));
        (void)ism_engine_createProducer(producer[i].hSession, ismDESTINATION_QUEUE, qname, &(createCB[i]));

    }

    /* Wait for the producers to be created */
    for (i = 0; i < numSessions; i++) {
        pthread_mutex_wait(&(producer[i].sessionMutex));
        if (verbose) {
            printf("producer[%d].hProducer is %p\n", i, producer[i].hProducer);
        }
    }
}

void destroyProducers(int numSessions, session_handles_t producer[])
{
    ismCALLBACK destroyCB[numSessions];
    int i;

    for (i = 0; i < numSessions; i++) {
        destroyCB[i].callbackFunction = destroyProducerCallback;
        destroyCB[i].pContext = (void *)&(producer[i]);
        pthread_mutex_lock(&(producer[i].sessionMutex));
        (void)ism_engine_destroyProducer(producer[i].hProducer, &(destroyCB[i]));

    }

    /* Wait for the producers to be destroyed */
    for (i = 0; i < numSessions; i++) {
        pthread_mutex_wait(&(producer[i].sessionMutex));
    }
}

void createConsumers(int numSessions,
                     char *qname,
                     session_handles_t consumer[],
                     bool verbose)
{
    ismCALLBACK createCB[numSessions];
    int i;

    for (i = 0; i < numSessions; i++) {
        consumer[i].hConsumer = NULL;
        createCB[i].callbackFunction = createConsumerCallback;
        createCB[i].pContext = (void *)&(consumer[i]);

        pthread_mutex_lock(&(consumer[i].sessionMutex));
        (void)ism_engine_createConsumer(consumer[i].hSession, ismDESTINATION_QUEUE, qname, &(createCB[i]));

    }

    /* Wait for the consumers to be created */
    for (i = 0; i < numSessions; i++) {
        pthread_mutex_wait(&(consumer[i].sessionMutex));
        if (verbose) {
            printf("consumer[%d].hConsumer is %p\n", i, consumer[i].hConsumer);
        }
    }
}

void destroyConsumers(int numSessions, session_handles_t consumer[])
{
    ismCALLBACK destroyCB[numSessions];
    int i;

    for (i = 0; i < numSessions; i++) {
        destroyCB[i].callbackFunction = destroyConsumerCallback;
        destroyCB[i].pContext = (void *)&(consumer[i]);
        pthread_mutex_lock(&(consumer[i].sessionMutex));
        (void)ism_engine_destroyConsumer(consumer[i].hConsumer, &(destroyCB[i]));

    }

    /* Wait for the producers to be destroyed */
    for (i = 0; i < numSessions; i++) {
        pthread_mutex_wait(&(consumer[i].sessionMutex));
    }
}

/********************************************************************************************************/
/*                                                                                                      */
/* void * producerLoop(void * pVoid)                                                                    */
/*                                                                                                      */
/* Put messages in transactions                                                                                      */
/********************************************************************************************************/
void * transactionalProducerLoop(void * pVoid, void * context, void * value)
{
    loop_config_t *pData = (loop_config_t *)pVoid;

    ismCALLBACK commitCB[pData->prodSessions];

    //We can just have one putMessageCB as we don't use the context
    ismCALLBACK putMessageCB = {putMessageCallback, NULL};

    int curSession=0;
    int numRemaining = pData->numPuts;

    session_handles_t producer[pData->prodSessions];
    int i;

    ismMESSAGE message = {ismMESSAGE_HEADER_DEFAULT, ismMESSAGE_FLAGS_CONTIGUOUS, 0, NULL, 0, NULL, 32,
        "12345678901234567890123456789012"};

    ism_time_t start, end;

    /* Set affinity of this thread */
    runOnlyOnCPUReversed(pData->cpu);

    /*Setup mutexes, sessions, transactions and producers */
    createMutexSessionTrans(pData->prodSessions, producer, pData->verbose);
    createProducers(pData->prodSessions,  pData->qname, producer, pData->verbose);

    /*Setup the contexts for each commit*/
    for (i=0; i<pData->prodSessions; i++) {
        commitCB[i].callbackFunction = commitLocalProducerTransactionCallback;
        commitCB[i].pContext = (void *)&(producer[i]);
    }

    /* If this is a persistent workload, make the message persistent */
    if(pData->persistent) {
        message.Header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    }

    /* If we are trying to synchronise setup/run/teardown amongst multiple threads */
    /* then record that we are ready and wait to start */
    if(pData->doPhases) {
        threadFinishedPhase(setup);
    }
    /* Store start time for performance test */
    start = ism_common_currentTimeNanos();

    /* while messages remain to be sent */
    while(numRemaining > 0) {
        /*Work out how many to send in the next transaction*/
        int sessionBatchSize = pData->prodTransactionSize;
        if (sessionBatchSize > numRemaining) {
            sessionBatchSize = numRemaining;
        }
        /*Ensure this session's transaction is not in use by earlier commit on it */
        /*the commit is unlocked when the commit for this session completes       */
        pthread_mutex_lock(&(producer[curSession].sessionMutex));

        for (i = 0; i < sessionBatchSize; i++) {
            __sync_add_and_fetch(&submitPutMessageCnt, 1);
            (void)ism_engine_putMessage(producer[curSession].hSession,
                                        producer[curSession].hProducer,
                                        producer[curSession].hTran,
                                        &message, &putMessageCB);
        }

        //when the callback is called it will unlock the appropriate session mutex
        (void)ism_engine_commitTransaction(producer[curSession].hSession,
                                                producer[curSession].hTran,
                                                &(commitCB[curSession]));

        numRemaining -= sessionBatchSize;

        //Now use the next session (note we don't wait for the commit on this
        //session to complete first so we have transactions in flight on
        //multiple sessions)
        curSession++;
        if(curSession >= pData->prodSessions) {
            curSession = 0;
        }
    }

    //We've now submitted all the commits we need to but need to wait for
    //the last commit in each session to complete
    for (i=0; i<pData->prodSessions; i++) {
        pthread_mutex_wait(&(producer[i].sessionMutex));
    }
    /* Work out the time for performance test */
    end = ism_common_currentTimeNanos();
    pData->duration_ns = end - start;

    /* If we need to, record that we have finished and are waiting */
    if(pData->doPhases) {
        threadFinishedPhase(running);
    }

    /*Setup mutexes, sessions, transactions and producers */
    destroyProducers(pData->prodSessions, producer);
    destroyMutexSessionTrans(pData->prodSessions, producer);

    return NULL;
}

void * nontransactionalProducerLoop(void * pVoid)
{
    loop_config_t *pData = (loop_config_t *)pVoid;


    //We can just have one putMessageCB as we don't use the context
    ismCALLBACK putMessageCB = {putMessageCallback, NULL};

    int curSession=0;
    int numRemaining = pData->numPuts;

    session_handles_t producer[pData->prodSessions];

    ismMESSAGE message = {ismMESSAGE_HEADER_DEFAULT, ismMESSAGE_FLAGS_CONTIGUOUS, 0, NULL, 0, NULL, 32,
        "12345678901234567890123456789012"};

    ism_time_t start, end;

    /* Set affinity of this thread */
    runOnlyOnCPUReversed(pData->cpu);

    /*Setup mutexes, sessions, transactions (unused) and producers */
    createMutexSessionTrans(pData->prodSessions, producer, pData->verbose);
    createProducers(pData->prodSessions,  pData->qname, producer, pData->verbose);

    /* If this is a persistent workload, make the message persistent */
    if(pData->persistent) {
        message.Header.Persistence = ismMESSAGE_PERSISTENCE_PERSISTENT;
    }

    /* If we are trying to synchronise setup/run/teardown amongst multiple threads */
    /* then record that we are ready and wait to start */
    if(pData->doPhases) {
        threadFinishedPhase(setup);
    }
    /* Store start time for performance test */
    start = ism_common_currentTimeNanos();

    /* while messages remain to be sent */
    while(numRemaining > 0) {
        __sync_add_and_fetch(&submitPutMessageCnt, 1);
        (void)ism_engine_putMessage(producer[curSession].hSession,
                producer[curSession].hProducer,
                NULL,
                &message, &putMessageCB);

        numRemaining -= 1;

        //Now use the next session
        curSession++;
        if(curSession >= pData->prodSessions) {
            curSession = 0;
        }
    }

    /* We'll declare we're finished at this point... depending on the threading
     * model the puts may or may not have completed, but as we have getter threads that will
     * wait until they have all the messages, this thread doesn't have to
     */

    /* Work out the time for performance test */
    end = ism_common_currentTimeNanos();
    pData->duration_ns = end - start;

    /* If we need to, record that we have finished and are waiting */
    if(pData->doPhases) {
        threadFinishedPhase(running);
    }

    /*Setup mutexes, sessions, transactions and producers */
    destroyProducers(pData->prodSessions, producer);
    destroyMutexSessionTrans(pData->prodSessions, producer);

    return NULL;
}


/********************************************************************************************************/
/*                                                                                                      */
/* void * nontransactionalConsumerLoop(void * pVoid)                                                                    */
/*                                                                                                      */
/* Get messages without using transactions                                                              */
/********************************************************************************************************/
void * transactionalConsumerLoop(void * pVoid)
{
    loop_config_t *pData = (loop_config_t *)pVoid;

    ismCALLBACK commitCB[pData->consSessions];
    ismCALLBACK getCB[pData->consSessions];


    int curSession=0;
    int numRemaining = pData->numGets;
    int numInEachTrans = 0;

    session_handles_t consumer[pData->consSessions];
    int i;

    ism_time_t start, end;

    /* Set affinity of this thread */
    runOnlyOnCPUReversed(pData->cpu);

    /*Setup mutexes, sessions, transactions and consumers */
    createMutexSessionTrans(pData->consSessions, consumer, pData->verbose);
    createConsumers(pData->consSessions, pData->qname, consumer, pData->verbose);

    /*Setup the contexts for each get and commit*/
    for (i=0; i<pData->consSessions; i++) {
        getCB[i].callbackFunction =getMessageCallback;
        getCB[i].pContext = (void *)&(consumer[i]);
        commitCB[i].callbackFunction = commitLocalConsumerTransactionCallback;
        commitCB[i].pContext = (void *)&(consumer[i]);
    }

    /* If we are trying to synchronise setup/run/teardown amongst multiple threads */
    /* then record that we are ready and wait to start */
    if(pData->doPhases) {
        threadFinishedPhase(setup);
    }
    /* Store start time for performance test */
    start = ism_common_currentTimeNanos();

    /* while messages remain to be got*/
    while (numRemaining > 0) {
        /* do gets while message remain to be got in this set of transactions */
        curSession = 0;
        numInEachTrans = 0;
        while ((numRemaining > 0)&&(numInEachTrans < pData->consTransactionSize)) {
            pthread_mutex_lock(&(consumer[curSession].sessionMutex));

            (void)ism_engine_getMessage(consumer[curSession].hSession,
                                        consumer[curSession].hConsumer,
                                        consumer[curSession].hTran,
                                        &(getCB[curSession]));
            numRemaining --;

            //Now use the next session (note we don't wait for the get on this
            //session to complete first so we have transactions in flight on
            //multiple sessions)
            curSession++;
            if(curSession >= pData->consSessions) {
                curSession = 0;

                //We've asked for a message in each transaction
                numInEachTrans++;
            }
        }

        //Commit all the transactions
        for (curSession = 0; curSession < pData->consSessions; curSession++) {
            /*Ensure the last get has completed for this session */
            pthread_mutex_lock(&(consumer[curSession].sessionMutex));
            //when the callback is called it will unlock the appropriate session mutex
            (void)ism_engine_commitTransaction(
                    consumer[curSession].hSession,
                    consumer[curSession].hTran,
                    &(commitCB[curSession]));
        }
    }

    //If we've got this far we need to wait for the last commits to finish
    for (i=0; i<pData->consSessions; i++) {
        pthread_mutex_wait(&(consumer[i].sessionMutex));
    }
    /* Work out the time for performance test */
    end = ism_common_currentTimeNanos();
    pData->duration_ns = end - start;

    /* If we need to, record that we have finished and are waiting */
    if(pData->doPhases) {
        threadFinishedPhase(running);
    }

    /*Setup mutexes, sessions, transactions and consumers */
    destroyConsumers(pData->consSessions, consumer);
    destroyMutexSessionTrans(pData->consSessions, consumer);

    return NULL;
}

/* count how many sessions are finished */
int consumersCompleted( int numSessions,
                        session_handles_t consumer[] )
{
    int sessionsComplete = 0;
    int i;

    for (i = 0; i< numSessions; i++) {
        if(   consumer[i].nontransConsInfo.msgsConsumed
                == consumer[i].nontransConsInfo.msgsRequired) {
            sessionsComplete++;
        }
    }

    return sessionsComplete;
}
/********************************************************************************************************/
/*                                                                                                      */
/* void * nontransactionalConsumerLoop(void * pVoid)                                                    */
/*                                                                                                      */
/* Consumes messages outside a transaction                                                              */
/********************************************************************************************************/
void *nontransactionalConsumerLoop(void * pVoid)
{
    loop_config_t *pData = (loop_config_t *)pVoid;
    ismCALLBACK associateCB[pData->consSessions];
    ismCALLBACK onMsgCB[pData->consSessions];

    int32_t rc;
    session_handles_t consumer[pData->consSessions];
    int i;
    pthread_mutex_t sessionCompletedMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t  sessionCompletedCond = PTHREAD_COND_INITIALIZER;
    ism_time_t start, end;

    /* Set affinity of this thread */
    runOnlyOnCPUReversed(pData->cpu);

    /*Setup mutexes, sessions, transactions and consumers */
    createMutexSessionTrans(pData->consSessions, consumer, pData->verbose);
    createConsumers(pData->consSessions, pData->qname, consumer, pData->verbose);

    /* Associate the msg recvd callbacks with the consumers*/
    for (i=0; i<pData->consSessions; i++) {
        onMsgCB[i].callbackFunction = nontransMsgArrivedCallback;
        onMsgCB[i].pContext = (void *)&(consumer[i]);

        associateCB[i].callbackFunction = associateCallback;
        associateCB[i].pContext = (void *)&(consumer[i]);

        int numForConsumer = pData->numGets / pData->consSessions;

        //If one consumer has to get more messages, make it the first one
        if (i == 0) {
            numForConsumer += ( pData->numGets
                               -(numForConsumer * pData->consSessions));
        }

        consumer[i].nontransConsInfo.msgsRequired = numForConsumer;
        consumer[i].nontransConsInfo.msgsConsumed = 0;
        consumer[i].nontransConsInfo.lastMsgSeq = 0;
        consumer[i].nontransConsInfo.lastAckedSeq = 0;
        consumer[i].nontransConsInfo.completed_cond = &sessionCompletedCond;
        consumer[i].nontransConsInfo.completed_mutex= &sessionCompletedMutex;

        pthread_mutex_lock(&(consumer[i].sessionMutex));

        ism_engine_associateMessageCallback(consumer[i].hSession,
                                            consumer[i].hConsumer,
                                            &(onMsgCB[i]),
                                            &(associateCB[i]));
    }

    /* Wait for the callbacks to be associated */
    for (i=0; i<pData->consSessions; i++) {
        pthread_mutex_wait(&(consumer[i].sessionMutex));
    }

    /* If we are trying to synchronise setup/run/teardown amongst multiple threads */
    /* then record that we are ready and wait to start */
    if(pData->doPhases) {
        rc = threadFinishedPhase(setup);
        assert(rc == OK);
    }
    /* Store start time for performance test */
    start = ism_common_currentTimeNanos();

    /* start the gets */
    for (i=0; i<pData->consSessions; i++) {
        ism_engine_startMessageDelivery(consumer[i].hSession, NULL);
    }

    /* loop until the sessions are all complete */
    do {
        /* wait for sessions to complete or a timeout to occur */
        struct timespec   waituntil;
        struct timeval    currtime;


        rc = pthread_mutex_lock(&sessionCompletedMutex);
        assert(rc == OK);

        rc =  gettimeofday(&currtime, NULL);
        assert(rc == OK);

        /* Convert from timeval to timespec and add ack interval*/
        waituntil.tv_sec  = currtime.tv_sec;
        waituntil.tv_nsec = (currtime.tv_usec + ENGINESTATS_ACK_INTERVAL) * 1000;
        if(waituntil.tv_nsec > 999999999) {
            waituntil.tv_sec += 1;
            waituntil.tv_nsec -= 1000000000;
        }
        if(consumersCompleted(pData->consSessions, consumer) < pData->consSessions) {
            rc = pthread_cond_timedwait(&sessionCompletedCond,
                    &sessionCompletedMutex,
                    &waituntil);
            if(rc == ETIMEDOUT) {
                rc = OK;
            } else {
                if( rc != OK) {
                    printf("pthread_cond_timedwait rc was %d\n", rc);
                }
                assert(rc == OK);
            }
        }
        rc = pthread_mutex_unlock(&sessionCompletedMutex);
        assert(rc == OK);

        /* fire acks for each session */
        for (i=0; i<pData->consSessions; i++) {
            //dirty read the sequence number
            uint64_t msgSeq = consumer[i].nontransConsInfo.lastMsgSeq;
            if ( consumer[i].nontransConsInfo.lastAckedSeq != msgSeq) {
                ism_engine_receiveFeedback(ISM_FEEDBACK_ACK,
                        consumer[i].hSession,
                        msgSeq,
                        true, NULL);

                consumer[i].nontransConsInfo.lastAckedSeq = msgSeq;
            }
        }


    } while(consumersCompleted(pData->consSessions, consumer) < pData->consSessions);

    /* Work out the time for performance test */
    end = ism_common_currentTimeNanos();
    pData->duration_ns = end - start;


    /* If we need to, record that we have finished and are waiting */
    if(pData->doPhases) {
        rc = threadFinishedPhase(running);
    }

    /*Setup mutexes, sessions, transactions and consumers */
    destroyConsumers(pData->consSessions, consumer);
    destroyMutexSessionTrans(pData->consSessions, consumer);

    return NULL;
}

/******************************************************************************/
/*                                                                            */
/* XAPI int ism_engine_stats(char* args, char ** pResult, int * pResultSize)  */
/*                                                                            */
/* Measure Message Engine's Performance Statistics                            */
/* Reports performance statics for message engine component.                  */
/* By default or when -msgs is specified, performance measurements            */
/* are provided for a producer/consumer test.                                 */
/* When -measure is specified, specific predefined jobs are submitted         */
/* to a worker thread for performance measurements.                           */
/*                                                                            */
/* @param args          buffer containing command line parameters             */
/* @param pResult       buffer provided for storing result                    */
/* @param pResultSize   pointer to size of pResult                            */
/*                                                                            */
/* @return A return code, OK=success                                          */
/*                                                                            */
/******************************************************************************/
XAPI int ism_engine_stats(char* args, char ** pResult, int * pResultSize)
{
    ismCALLBACK createClientCB = {createClientCallback, "<X>"};
    ismCALLBACK destroyClientCB = {destroyClientCallback, "<X>"};

    pthread_mutex_t engineStatsMtx;
    int i,j;

    int rc = OK;
    config_t config;
    config.prime = 0;
    config.count = DEFAULT_MESSAGE_COUNT;
    config.queues = DEFAULT_NUM_QUEUES;
    config.numProducers = 0; //If no producers or consumers, both run...
    config.prodSessions = 1;
    config.prodTransSize = 1;
    config.numConsumers = 0; //If no producers or consumers, both run...
    config.consSessions = 1;
    config.consTransSize = 1;
    config.numProdCons = 0;
    config.prodconsTransactions = 0;
    config.prodconsSessions = 0;
    config.measureIterations = 0;
    config.measure = false;
    config.persistent = false;
    config.help = false;
    config.verbose = false;
    ism_time_t start = 0;
    ism_time_t finish = 0;
    releaseMessageCallbackCnt = 0;
    putMessageCallbackCnt = 0;
    submitGetMessageCnt = 0;
    submitPutMessageCnt = 0;

    setlocale(LC_ALL, "");

    /************************************************************************/
    /* parse the args buffer                                                */
    /************************************************************************/

    rc = parseArgs(args, &config);

    if ((rc != OK) || (config.help)) {
        return (rc);
    }

    /************************************************************************/
    /* if -measure was specified then call "measure" api instead of         */
    /* the producer consumer test                                           */
    /************************************************************************/
    if (config.measure) {
        rc = measure(config.measureIterations);
        return (rc);
    }

    if (config.verbose) {
        printConfig(config);
    }

    /************************************************************************/
    /* initialize mutex                                                     */
    /************************************************************************/
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&engineStatsMtx, &attr);

    /************************************************************************/
    /* create Client handle                                                 */
    /************************************************************************/
    hClient = NULL;
    createClientCB.pContext = (void *)&engineStatsMtx;
    pthread_mutex_lock(&engineStatsMtx);
    (void)ism_engine_createClient(&createClientCB);
    pthread_mutex_wait(&engineStatsMtx);
    if (config.verbose) {
        printf("hClient is %p\n", hClient);
    }

    /************************************************************************/
    /* prime the engine per -prime <msg count> option                       */
    /************************************************************************/
    if (config.prime > 0) {
        if (config.verbose) {
            printf("Put & Get %'d messages before collecting stats.\n", config.prime);
        }
        int prime_per_q = config.prime/config.queues;

        for (i = 0; i < config.queues; i++) {
            //If we are on the last queue and the number of messages to
            //prime didn't divide nicely across the queues, do the remainder
            if(i == (config.queues-1)) {
                prime_per_q += config.prime - (prime_per_q * config.queues);
            }

            loop_config_t producerConfig=EMPTY_LOOP_CONFIG;
            setQueueName(i, producerConfig.qname);
            producerConfig.doPhases=false; //do the whole thing ASAP
            producerConfig.numPuts=prime_per_q;
            producerConfig.prodSessions=1;
            producerConfig.prodTransactionSize=100;
            producerConfig.verbose=config.verbose;
            producerConfig.persistent=config.persistent;
            transactionalProducerLoop(&producerConfig);

            loop_config_t consumerConfig=EMPTY_LOOP_CONFIG;
            setQueueName(i, consumerConfig.qname);
            consumerConfig.doPhases=false; //do the whole thing ASAP
            consumerConfig.numGets=prime_per_q;
            consumerConfig.consSessions=1;
            consumerConfig.consTransactionSize=100;
            consumerConfig.verbose=config.verbose;
            consumerConfig.persistent=config.persistent;
            transactionalConsumerLoop(&consumerConfig);
        }
    }

    /************************************************************************/
    /* if only testing "get" then load messages in advance                  */
    /************************************************************************/
    if (config.numProducers == 0) {
        if (config.verbose) {
            printf("Loading %'d messages.\n", config.count);
        }

        int msgs_per_q = config.count/config.queues;

        for (i = 0; i < config.queues; i++) {
            //If we are on the last queue and the number of messages to
            //put didn't divide nicely across the queues, do the remainder
            if(i == (config.queues-1)) {
                msgs_per_q += config.count - (msgs_per_q * config.queues);
            }

            loop_config_t producerConfig=EMPTY_LOOP_CONFIG;
            setQueueName(i, producerConfig.qname);
            producerConfig.doPhases=false; //do the whole thing ASAP
            producerConfig.numPuts=msgs_per_q;
            producerConfig.prodSessions=1;
            producerConfig.prodTransactionSize=100;
            producerConfig.verbose=config.verbose;
            producerConfig.persistent=config.persistent;

            transactionalProducerLoop(&producerConfig);
        }
    }

    /************************************************************************/
    /* measure engine performance                                           */
    /************************************************************************/
    if (config.verbose) {
        printf("Collecting stats for ");
        if ((config.numProducers > 0) && (config.numConsumers > 0)) {
            printf("Put & Get ");
        } else if (config.numProducers > 0) {
            printf("Put ");
        } else if (config.numConsumers > 0) {
            printf("Get ");
        }
        printf("of %'d messages.\n", config.count);
    }

    /**************************************************************************/
    /*   start the producer threads and let them get setup                    */
    /**************************************************************************/
    globalStatus.numThreadsCompletedPhase = 0;
    globalStatus.phase = setup;

    ism_threadh_t producer_threads[config.numProducers * config.queues];
    loop_config_t producerConfig[config.numProducers * config.queues];

    int cpuNum = 1;
    int totalCPUs = getHardwareNumCPUs();

    int msgs_per_q = config.count/config.queues;
    int threadNum = 0;

    for (i = 0; i < config.queues; i++) {
        //If we are on the last queue and the number of messages to
        //put didn't divide nicely across the queues, do the remainder
        if(i == (config.queues-1)) {
            msgs_per_q += config.count - (msgs_per_q * config.queues);
        }

        for (j=0; j<config.numProducers; j++) {
            char threadName[16];

            int numMsgs = msgs_per_q/config.numProducers;

            //If the threads aren't all going to put equal messages, make the first
            //one do the remainder
            if(j == 0) {
                numMsgs += (msgs_per_q - (config.numProducers * numMsgs));
            }
            setQueueName(i, producerConfig[threadNum].qname);
            producerConfig[threadNum].doPhases=true;
            producerConfig[threadNum].numPuts=numMsgs;
            producerConfig[threadNum].prodSessions=config.prodSessions;
            producerConfig[threadNum].prodTransactionSize=config.prodTransSize;
            producerConfig[threadNum].verbose=config.verbose;
            producerConfig[threadNum].cpu=cpuNum;
            producerConfig[threadNum].persistent=config.persistent;

            sprintf(threadName, "prod%d", threadNum);

            if (config.prodTransSize > 0) {
                rc = ism_common_startThread(&(producer_threads[threadNum]),
                        transactionalProducerLoop, &(producerConfig[threadNum]), NULL, 0,
                        ISM_TUSAGE_NORMAL, 0, threadName, "Transactional Producer Thread");
            } else {
                rc = ism_common_startThread(&(producer_threads[threadNum]),
                        nontransactionalProducerLoop, &(producerConfig[threadNum]), NULL, 0,
                        ISM_TUSAGE_NORMAL, 0, threadName, "Transactional Producer Thread");
            }
            threadNum++;
            cpuNum++;
            if(cpuNum > totalCPUs) {
                cpuNum = 1;
            }

            assert(rc == 0);
        }
    }

    /**************************************************************************/
    /*   start the consumer threads and let them get setup                    */
    /**************************************************************************/

    ism_threadh_t consumer_threads[config.numConsumers * config.queues];
    loop_config_t consumerConfig[config.numConsumers * config.queues];

    msgs_per_q = config.count/config.queues;
    threadNum = 0;

    for (i = 0; i < config.queues; i++) {
        //If we are on the last queue and the number of messages to
        //put didn't divide nicely across the queues, do the remainder
        if(i == (config.queues-1)) {
            msgs_per_q += config.count - (msgs_per_q * config.queues);
        }

        for (j=0; j<config.numConsumers; j++) {
            char threadName[16];

            int numMsgs = msgs_per_q/config.numConsumers;

            //If the threads aren't all going to get equal messages, make the first
            //one do the remainder

            if(j == 0) {
                numMsgs += (msgs_per_q - (config.numConsumers * numMsgs));
            }
            setQueueName(i, consumerConfig[threadNum].qname);
            consumerConfig[threadNum].doPhases=true;
            consumerConfig[threadNum].numGets=numMsgs;
            consumerConfig[threadNum].consSessions=config.consSessions;
            consumerConfig[threadNum].consTransactionSize=config.consTransSize;
            consumerConfig[threadNum].verbose=config.verbose;
            consumerConfig[threadNum].cpu=cpuNum;
            consumerConfig[threadNum].persistent=config.persistent;

            sprintf(threadName, "cons%d", threadNum);

            if (config.consTransSize > 0) {
                rc = ism_common_startThread(&(consumer_threads[threadNum]),
                        transactionalConsumerLoop, &(consumerConfig[threadNum]), NULL, 0,
                        ISM_TUSAGE_NORMAL, 0, threadName, "Consumer Thread");
            } else {
                rc = ism_common_startThread(&(consumer_threads[threadNum]),
                        nontransactionalConsumerLoop, &(consumerConfig[threadNum]), NULL, 0,
                        ISM_TUSAGE_NORMAL, 0, threadName, "Consumer Thread");
            }
            threadNum++;
            cpuNum++;
            if(cpuNum > totalCPUs) {
                cpuNum = 1;
            }
            assert(rc == 0);
        }
    }

    //Record start time and set the threads running
    rc = startNextPhaseWhenReady( (config.numProducers+config.numConsumers)*config.queues,
                                  running,
                                  &start);

    //When all the threads have done the work, record the finish time
    rc = startNextPhaseWhenReady( (config.numProducers+config.numConsumers)*config.queues,
                                  teardown,
                                  &finish);

    if (config.verbose) {
        printf("Engine stats collected...\n");
    }

    //Wait for the threads to finish
    for (i=0; i<config.numProducers*config.queues; i++) {
        void *retval;
        ism_common_joinThread(producer_threads[i], &retval);

        printf("Producer thread %d was working for %'.3f ms\n",
                i, producerConfig[i].duration_ns / 1000000.0);
    }
    if (config.verbose) {
        printf("Producer threads ended...\n");
    }
    for (i=0; i<config.numConsumers*config.queues; i++) {
        void *retval;
        ism_common_joinThread(consumer_threads[i], &retval);

        printf("Consumer thread %d was working for %'.3f ms\n",
                i, consumerConfig[i].duration_ns / 1000000.0);
    }
    if (config.verbose) {
        printf("Consumer threads ended...\n");
    }
    /************************************************************************/
    /* if only testing "put" then remove messages after test                */
    /************************************************************************/
    if (config.numConsumers == 0) {
        if (config.verbose) {
            printf("Removing %'d messages.\n", config.count);
        }

        int msgs_per_q = config.count/config.queues;

        for (i = 0; i < config.queues; i++) {
            //If we are on the last queue and the number of messages to
            //put didn't divide nicely across the queues, do the remainder
            if(i == (config.queues-1)) {
                msgs_per_q += config.count - (msgs_per_q * config.queues);
            }
            loop_config_t consumerConfig= EMPTY_LOOP_CONFIG;
            setQueueName(i, consumerConfig.qname);
            consumerConfig.doPhases=false; //do the whole thing ASAP
            consumerConfig.numGets=msgs_per_q;
            consumerConfig.consSessions=1;
            consumerConfig.consTransactionSize=100;
            consumerConfig.verbose=config.verbose;
            consumerConfig.persistent=config.persistent;
            transactionalConsumerLoop(&consumerConfig);
        }
    }

    /************************************************************************/
    /* cleanup client                                                       */
    /************************************************************************/
    if (config.verbose) {
        printf("----> destroy client handle\n");
    }

    destroyClientCB.pContext = (void *)&engineStatsMtx;
    pthread_mutex_lock(&engineStatsMtx);
    (void)ism_engine_destroyClient(hClient, &destroyClientCB);
    pthread_mutex_wait(&engineStatsMtx);

    if (config.verbose) {
        printf("----> client destroyed\n");
    }

    /************************************************************************/
    /* Report results of performance test                                   */
    /************************************************************************/

//    if ((config.testProd) && (submitPutMessageCnt != config.count)) {
//        printf("ERROR:  ism_engine_putMessage called %'d of %'d times\n", submitPutMessageCnt, config.count);
//        printf("        This is likely due to an error in the logic of the engineStats command.\n");
//    } else if ((config.testCons) && (submitGetMessageCnt != config.count)) {
//        printf("ERROR:  ism_engine_getMessage called %'d of %'d times\n", submitGetMessageCnt, config.count);
//        printf("        This is likely due to an error in the logic of the engineStats command.\n");
//    } else if ((config.testProd) && (submitPutMessageCnt != putMessageCallbackCnt)) {
//        printf("ERROR:  putMessageCallback called %'d of %'d times\n", putMessageCallbackCnt, submitPutMessageCnt);
//    } else if ((config.testCons) && (releaseMessageCallbackCnt != submitGetMessageCnt)) {
//        printf("ERROR:  releaseMessageCallback called %'d of %'d times - ", releaseMessageCallbackCnt,
//               submitGetMessageCnt);
//        printf("a MESSAGE LOSS of %'d.\n", submitPutMessageCnt - releaseMessageCallbackCnt);
//    }

    if ((config.numProducers > 0) && (config.numConsumers > 0)) {
        printf("Put&Get: ");
    } else if (config.numProducers > 0) {
        printf("Put: ");
    } else if (config.numConsumers > 0) {
        printf("Get: ");
    }
    printf("%'d msgs in %'.3f ms, Rate: %'.0f msg/s, %'ld ns/msg\n", config.count, (finish - start) / 1000000.0,
            (1000000000.0 * config.count) / (finish - start), (long)((finish - start) / config.count));

    /****************************************************************************/
    /* If results buffer provided then copy results of performance test into it */
    /* this is used for websvcs requests.                                       */
    /****************************************************************************/
    if ((pResult) && (*pResult) && (pResultSize)) {
        int resultsize = 1024;
        char result[resultsize];
        int size = 0;
        size = snprintf(result, resultsize, "%d, msgs, %.3f, ms, %.0f, msg/s, %'ld, ns/msg", config.count, (finish
                - start) / 1000000.0, (1000000000.0 * config.count) / (finish - start), (long)((finish - start)
                        / config.count));

        if ((size + 1) > *pResultSize) {
            free(*pResult);
            *pResult = malloc(size + 1);
            *pResultSize = size + 1;
        }
        strncpy(*pResult, result, size + 1);
    }
    /************************************************************************/
    /* finished                                                             */
    /************************************************************************/
    return 0;
}

/**********************************************************************************/
/*                                                                                */
/* void createClientCallback(int32_t retcode, void *handle, void *pContext)       */
/*                                                                                */
/* callback for ism_engine_createClient api call                                  */
/*                                                                                */
/**********************************************************************************/
void createClientCallback(int32_t retcode, void *handle, void *pContext)
{
#ifdef DEBUG_ENGINE_STATS
    printf("  createClientCallback: retcode %d, handle %p, pContext %p.\n", retcode, handles, pContext);
#endif
    if (retcode == OK) {
        hClient = (ismEngine_client_t *)handle;
    }
    pthread_mutex_unlock((pthread_mutex_t *)pContext);

    assert(retcode == OK);
}

/**********************************************************************************/
/*                                                                                */
/* void destroyClientCallback(int32_t retcode, void *handle, void *pContext)      */
/*                                                                                */
/* callback for ism_engine_destroyClient api call                                 */
/*                                                                                */
/**********************************************************************************/
void destroyClientCallback(int32_t retcode, void *handle, void *pContext)
{
#ifdef DEBUG_ENGINE_STATS
    printf("  destroyClientCallback: retcode %d, handle %p, pContext %p.\n", retcode, handles, pContext);
#endif
    if (retcode == OK) {
        hClient = NULL;
    }
    pthread_mutex_unlock((pthread_mutex_t *)pContext);

    assert(retcode == OK);
}

/**********************************************************************************/
/*                                                                                */
/* void createSessionCallback(int32_t retcode, void *handle, void *pContext)      */
/*                                                                                */
/* callback for ism_engine_createSession api call                                 */
/*                                                                                */
/**********************************************************************************/
void createSessionCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;

#ifdef DEBUG_ENGINE_STATS
    printf("  createSessionCallback: retcode %d, handle %p, pContext %p.\n", retcode, handles, pContext);
#endif
    if (retcode == OK) {
        handles->hSession = (ismEngine_session_t *)handle;
    }
    pthread_mutex_unlock(&(handles->sessionMutex));

    assert(retcode == OK);
}

/**********************************************************************************/
/*                                                                                */
/* void destroySessionCallback(int32_t retcode, void *handle, void *pContext)     */
/*                                                                                */
/* callback for ism_engine_destroySession api call                                */
/*                                                                                */
/**********************************************************************************/
void destroySessionCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;

#ifdef DEBUG_ENGINE_STATS
    printf("  destroySessionCallback: retcode %d, handle %p, pContext %p.\n", retcode, handles, pContext);
#endif
    if (retcode == OK) {
        handles->hSession = NULL;
    }
    pthread_mutex_unlock(&(handles->sessionMutex));

    assert(retcode == OK);
}

/***************************************************************************************/
/*                                                                                     */
/* void createLocalTransactionCallback(int32_t retcode, void *handle, void *pContext)  */
/*                                                                                     */
/* callback for ism_engine_createLocalTransaction api call                             */
/*                                                                                     */
/***************************************************************************************/
void createLocalTransactionCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;

#ifdef DEBUG_ENGINE_STATS
    printf("  createLocalTransactionCallback: retcode %d, handle %p, pContext %p.\n", retcode, handles, pContext);
#endif
    if (retcode == OK) {
        handles->hTran = (ismEngine_transaction_t *)handle;
    }
    pthread_mutex_unlock(&(handles->sessionMutex));

    assert(retcode == OK);
}

/***********************************************************************************************/
/*                                                                                             */
/* void commitLocalProducerTransactionCallback(int32_t retcode, void *handle, void *pContext)  */
/*                                                                                             */
/* callback for producer's ism_engine_commitTransaction api call                               */
/*                                                                                             */
/***********************************************************************************************/
void commitLocalProducerTransactionCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;

#ifdef DEBUG_ENGINE_STATS
    printf("  commitLocalProducerTransactionCallback: retcode %d, handle %p, pContext %p.\n", retcode, handle,
        pContext);
#endif

    pthread_mutex_unlock(&(handles->sessionMutex));

    assert(retcode == OK);
}
/***********************************************************************************************/
/*                                                                                             */
/* void commitLocalConsumerTransactionCallback(int32_t retcode, void *handle, void *pContext)  */
/*                                                                                             */
/* callback for consumers's ism_engine_commitTransaction api call                              */
/*                                                                                             */
/***********************************************************************************************/
void commitLocalConsumerTransactionCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;

#ifdef DEBUG_ENGINE_STATS
    printf("  commitLocalGetTransactionCallback: retcode %d, handle %p, pContext %p.\n", retcode, handle,
        pContext);
#endif

    pthread_mutex_unlock(&(handles->sessionMutex));

    assert(retcode == OK);
}

/**********************************************************************************/
/*                                                                                */
/* void createProducerCallback(int32_t retcode, void *handle, void *pContext)     */
/*                                                                                */
/* callback for ism_engine_createProducer api call                                */
/*                                                                                */
/**********************************************************************************/
void createProducerCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;

#ifdef DEBUG_ENGINE_STATS
    printf("  createProducerCallback: retcode %d, handle %p, pContext %p.\n", retcode, handles, pContext);
#endif
    if (retcode == OK) {
        handles->hProducer = (ismEngine_producer_t *)handle;
    }
    pthread_mutex_unlock(&(handles->sessionMutex));

    assert(retcode == OK);
}

/**********************************************************************************/
/*                                                                                */
/* void destroyProducerCallback(int32_t retcode, void *handle, void *pContext)    */
/*                                                                                */
/* callback for ism_engine_destroyProducer api call                               */
/*                                                                                */
/**********************************************************************************/
void destroyProducerCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;

#ifdef DEBUG_ENGINE_STATS
    printf("  destroyProducerCallback: retcode %d, handle %p, pContext %p.\n", retcode, handles, pContext);
#endif
    if (retcode == OK) {
        handles->hProducer = NULL;
    }
    pthread_mutex_unlock(&(handles->sessionMutex));

    assert(retcode == OK);
}

/**********************************************************************************/
/*                                                                                */
/* void putMessageCallback(int32_t retcode, void *handle, void *pContext)         */
/*                                                                                */
/* callback for ism_engine_putMessage api call                                    */
/*                                                                                */
/**********************************************************************************/
void putMessageCallback(int32_t retcode, void *handle, void *pContext)
{
#ifdef DEBUG_ENGINE_STATS
    printf("  putMessageCallback: retcode %d, handle %p, pContext %p.\n", retcode, handles, pContext);
#endif

    __sync_add_and_fetch(&putMessageCallbackCnt, 1);

    if (retcode)
        printf("  putMessageCallback: retcode %d, handle %p, pContext %p.\n", retcode, handle, pContext);

    assert(retcode == OK);
}

/**********************************************************************************/
/*                                                                                */
/* void createConsumerCallback(int32_t retcode, void *handle, void *pContext)     */
/*                                                                                */
/* callback for ism_engine_createConsumer api call                                */
/*                                                                                */
/**********************************************************************************/
void createConsumerCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;

#ifdef DEBUG_ENGINE_STATS
    printf("  createConsumerCallback: retcode %d, handle %p, pContext %p.\n", retcode, handles, pContext);
#endif
    if (retcode == OK) {
        handles->hConsumer = (ismEngine_consumer_t *)handle;
    }
    pthread_mutex_unlock(&(handles->sessionMutex));

    assert(retcode == OK);
}

/**********************************************************************************/
/*                                                                                */
/* void destroyConsumerCallback(int32_t retcode, void *handle, void *pContext)    */
/*                                                                                */
/* callback for ism_engine_detroyConsumer api call                                */
/*                                                                                */
/**********************************************************************************/
void destroyConsumerCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;

#ifdef DEBUG_ENGINE_STATS
    printf("  destroyConsumerCallback: retcode %d, handle %p, pContext %p.\n", retcode, handles, pContext);
#endif
    if (retcode == OK) {
        handles->hConsumer = NULL;
    }
    pthread_mutex_unlock(&(handles->sessionMutex));

    assert(retcode == OK);
}

/**********************************************************************************/
/*                                                                                */
/* void getMessageCallback(int32_t retcode, void *handle, void *pContext)         */
/*                                                                                */
/* callback for ism_engine_getMessage api call                                    */
/*                                                                                */
/**********************************************************************************/
void getMessageCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;

#ifdef DEBUG_ENGINE_STATS
    printf("  getMessageCallback: retcode %d, handle %p, pContext '%s'.\n", retcode, handles, (char *)pContext);
#endif
    if (retcode == OK) {
#ifdef DEBUG_ENGINE_STATS
        printf("GOT MESSAGE: '%s'.\n", (char *)pMessage->pPayload);
#endif
        pthread_mutex_unlock(&(handles->sessionMutex));
        ism_engine_releaseMessage(handle, NULL);
    } else {
        printf("ERROR Return from get: retcode %d.\n", retcode);
        assert(false);
    }
    assert(retcode == OK);
}

/* Called when we received a message non-transactionally */
void nontransMsgArrivedCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;
    ismMESSAGE *Message = (ismMESSAGE *)handle;
    uint32_t msgsArrived;
    int rc;

    if( retcode == OK ) {
        handles->nontransConsInfo.lastMsgSeq = Message->Header.SequenceNumber;
        msgsArrived = __sync_add_and_fetch(&(handles->nontransConsInfo.msgsConsumed), 1);

        if ( msgsArrived == handles->nontransConsInfo.msgsRequired ) {
            //As we have all the messages we want...stop delivery
            ism_engine_stopMessageDelivery(handles->hSession, NULL);

            //Broadcast that this thread has finished
            rc = pthread_mutex_lock(handles->nontransConsInfo.completed_mutex);

            if (OK == rc) {
                rc = pthread_cond_broadcast(handles->nontransConsInfo.completed_cond);

                int rc2 = pthread_mutex_unlock(handles->nontransConsInfo.completed_mutex);
                if ((rc2 != OK)&&(rc == OK)) {
                    rc = rc2;
                }
            }
        }

        ism_engine_releaseMessage(Message, NULL);
    } else {
        //The only other return code we expect the one we get when we receive
        //all the messages and disable ourselves
        assert (  (retcode == ismENGINERC_NO_MESSAGE_AVAILABLE)
                &&(    handles->nontransConsInfo.msgsConsumed
                    == handles->nontransConsInfo.msgsRequired) );
    }
}

/*Called we associate a non-transactional message listener with a consumer */
void associateCallback(int32_t retcode, void *handle, void *pContext)
{
    session_handles_t *handles = (session_handles_t *)pContext;

  #ifdef DEBUG_ENGINE_STATS
      printf("  associateCallback: retcode %d, handle %p, pContext %p.\n", retcode, handles, pContext);
  #endif
      assert(retcode == OK);
      pthread_mutex_unlock(&(handles->sessionMutex));
}

#ifdef DEBUG_ENGINE_STATS
#undef DEBUG_ENGINE_STATS
#endif

/************************************************************************/
/* End of file                                                          */
/************************************************************************/
