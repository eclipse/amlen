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
/* Module Name: lockManager.h                                        */
/*                                                                   */
/* Description: Engine component lock manager header file            */
/*                                                                   */
/*********************************************************************/
#ifndef __ISM_ENGINE_STATS_DEFINED
#define __ISM_ENGINE_STATS_DEFINED

/**********************************************************************************/
/* DEFAULT_MESSAGE_COUNT is the default number of messages used for the           */
/* message engine's producer/consumer performance test.                           */
/**********************************************************************************/
#define DEFAULT_MESSAGE_COUNT 1000

/**********************************************************************************/
/* DEFAULT_NUM_QUEUES is the default number of queues the messages are spread     */
/* across in the message engine's producer/consumer performance test.             */
/**********************************************************************************/
#define DEFAULT_NUM_QUEUES 1

/**********************************************************************************/
/* DEFAULT_MEASURE_ITERATIONS is the default number of iterations used for the    */
/* predefined code snippets performance tests.                                    */
/**********************************************************************************/
#define DEFAULT_MEASURE_ITERATIONS 1E6


/**********************************************************************************/
/* For asynchronous messages, how often should we ack? Measured in MICROSECONDS                                  */
/**********************************************************************************/
#define ENGINESTATS_ACK_INTERVAL 1000000



/**********************************************************************************/
/* Queue names will be of the form QUEUE_NAME_START suffixed with an integer,     */
/* MAX_QUEUE_NAME_LENGTH defines the buffer size needed to hold the queue name    */
/**********************************************************************************/
#define QUEUE_NAME_START "EngineStats"
#define MAX_QUEUE_NAME_LENGTH 64

/**********************************************************************************/
// pthread_mutex_wait(X) is a macro for waiting for a mutex to become available.  */
/**********************************************************************************/
#define pthread_mutex_wait(X) pthread_mutex_lock(X); pthread_mutex_unlock(X);


typedef struct {
    pthread_cond_t *completed_cond; //Broadcast on when correct num msgs arrived
    pthread_mutex_t *completed_mutex;
    uint32_t msgsRequired;
    volatile uint32_t msgsConsumed;
    volatile uint64_t lastMsgSeq;
    uint64_t lastAckedSeq;
} stats_nontrans_consdata_t;

/**********************************************************************************/
/* session_handles_t is a structure type containing the engine handles for        */
/* the producer and consumer.                                                     */
/**********************************************************************************/
typedef struct {
    pthread_mutex_t sessionMutex;
    ismEngine_session_t *hSession;
    ismEngine_transaction_t *hTran;
    ismEngine_consumer_t *hConsumer;
    ismEngine_producer_t *hProducer;
    stats_nontrans_consdata_t nontransConsInfo;
} session_handles_t;

/**********************************************************************************/
/* config_t is the structure type for the parsed commandline options.             */
/**********************************************************************************/
typedef struct {
    int prime;                    //Number of messages to Put & get from each queue before test starts
    int count;                    //Number of messages to measure
    int queues;                   //Number of queues to spread the messages across
    int numProducers;             //Number of threads producing messages per queue
    int prodSessions;             //Number of sessions per producer thread
    int prodTransSize;            //Size of transactions in producer threads
    int numConsumers;             //Number of threads consuming messages per queue
    int consSessions;             //Number of sessions per consumer threads
    int consTransSize;            //Size of transactions in consumer threads
    int numProdCons;              //Number of threads that produce and consume messages
    int prodconsTransactions;     //Size of transactions in prod+consume threads
    int prodconsSessions;         //Number of sessions in threads that produce and consume
    int measureIterations;
    bool measure;
    bool persistent;
    bool help;
    bool verbose;
} config_t;

/**********************************************************************************/
/* loop_config_t is the structure type for the producer/consumer                  */
/* execution options passed into the performance measurement loop                 */
/* that executes the putMessage and getMessages api calls.                        */
/**********************************************************************************/
typedef struct {
    char qname[MAX_QUEUE_NAME_LENGTH];
    int numPuts;
    int prodTransactionSize;
    int prodSessions;
    int numGets;
    int consTransactionSize;
    int consSessions;
    bool verbose;
    int cpu;
    ism_time_t duration_ns;
    bool doPhases;
    bool persistent;
} loop_config_t;

#define EMPTY_LOOP_CONFIG {{0}, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

/**********************************************************************************/
/* test_status_t is used for all the threads to co-ordinate their activity        */
/**********************************************************************************/
typedef enum tag_testPhase_t {setup, running, teardown}  testPhase_t;
typedef struct {
    testPhase_t phase;
    pthread_cond_t phase_cond;
    pthread_mutex_t phase_mutex;
    int numThreadsCompletedPhase;
    pthread_cond_t numthreads_cond;
    pthread_mutex_t numthreads_mutex;
    int errCode;
} testStatus_t;

#define TESTSTATUS_INITIAL { setup,                       \
                             PTHREAD_COND_INITIALIZER,    \
                             PTHREAD_MUTEX_INITIALIZER,   \
                             0,                           \
                             PTHREAD_COND_INITIALIZER,    \
                             PTHREAD_MUTEX_INITIALIZER,   \
                             0 }


/**********************************************************************************/
/* measureInternalData_t is the structure type for the execution jobs             */
/* associated with the -measure option.                                           */
/**********************************************************************************/
typedef struct {
    ism_worker_job_t pFn;
    int iterations;
    char * label;
    void * pData;
} measureInternalData_t;

/**********************************************************************************/
/* parseArgs                                                                      */
/*                                                                                */
/* Parses the commandline arguments                                               */
/**********************************************************************************/
int parseArgs(char *args, config_t *config);

/**********************************************************************************/
/* printConfig                                                                    */
/*                                                                                */
/* Prints the current config_t structure members.                                 */
/**********************************************************************************/
void printConfig(config_t config);

/**********************************************************************************/
/* print_usage                                                                    */
/*                                                                                */
/* Prints usage information to stdout                                             */
/**********************************************************************************/
void print_usage(char *, char *, bool);


/**********************************************************************************/
/* createClientCallback                                                           */
/*                                                                                */
/* callback for ism_engine_createClient api call                                  */
/**********************************************************************************/
void createClientCallback( int32_t, void *, void *);

/**********************************************************************************/
/* destroyClientCallback                                                          */
/*                                                                                */
/* callback for ism_engine_destroyClient api call                                 */
/**********************************************************************************/
void destroyClientCallback( int32_t, void *, void *);

/**********************************************************************************/
/* createSessionCallback                                                          */
/*                                                                                */
/* callback for ism_engine_createSession api call                                 */
/**********************************************************************************/
void createSessionCallback( int32_t, void *, void *);

/**********************************************************************************/
/* destroySessionCallback                                                         */
/*                                                                                */
/* callback for ism_engine_destroySession api call                                */
/**********************************************************************************/
void destroySessionCallback( int32_t, void *, void *);

/**********************************************************************************/
/* createLocalTransactionCallback                                                 */
/*                                                                                */
/* callback for ism_engine_createLocalTransaction api call                        */
/**********************************************************************************/
void createLocalTransactionCallback( int32_t, void *, void *);

/**********************************************************************************/
/* commitLocalProducerTransactionCallback                                         */
/*                                                                                */
/* callback for ism_engine_commitTransaction api call                             */
/**********************************************************************************/
void commitLocalProducerTransactionCallback( int32_t, void *, void *);

/**********************************************************************************/
/* commitLocalConsumerTransactionCallback                                         */
/*                                                                                */
/* callback for ism_engine_commitTransaction api call                             */
/**********************************************************************************/
void commitLocalConsumerTransactionCallback( int32_t, void *, void *);

/**********************************************************************************/
/* createProducerCallback                                                         */
/*                                                                                */
/* callback for ism_engine_createProducer api call                                */
/**********************************************************************************/
void createProducerCallback( int32_t, void *, void *);

/**********************************************************************************/
/* destroyProducerCallback                                                        */
/*                                                                                */
/* callback for ism_engine_destroyProducer api call                               */
/**********************************************************************************/
void destroyProducerCallback( int32_t, void *, void *);

/**********************************************************************************/
/* putMessageCallback                                                             */
/*                                                                                */
/* callback for ism_engine_putMessage api call                                    */
/**********************************************************************************/
void putMessageCallback( int32_t, void *, void *);

/**********************************************************************************/
/* createConsumerCallback                                                         */
/*                                                                                */
/* callback for ism_engine_createConsumer api call                                */
/**********************************************************************************/
void createConsumerCallback( int32_t, void *, void *);

/**********************************************************************************/
/* destroyConsumerCallback                                                        */
/*                                                                                */
/* callback for ism_engine_destroyConsumer api call                               */
/**********************************************************************************/
void destroyConsumerCallback( int32_t, void *, void *);

/**********************************************************************************/
/* getMessageCallback                                                             */
/*                                                                                */
/* callback for ism_engine_getMessage api call                                    */
/**********************************************************************************/
void getMessageCallback( int32_t, void *, void *);
//void getMessageCallbackNoMsg( int32_t, void *, void *);

/**********************************************************************************/
/* releaseMessageCallback                                                         */
/*                                                                                */
/* callback for ism_engine_releaseMessage api call                                */
/**********************************************************************************/
void releaseMessageCallback( int32_t, void *, void *);


/* Called when we received a message non-transactionally */
void nontransMsgArrivedCallback(int32_t retcode, void *handle, void *pContext);
void associateCallback(int32_t retcode, void *handle, void *pContext);


/**********************************************************************************/
/* measure (engineStatsMeasure.c)                                                 */
/*                                                                                */
/* Function for timing a small operation in a loop                                */
/**********************************************************************************/

XAPI int measure(int iterations);

#endif /* __ISM_ENGINE_STATS_DEFINED */
