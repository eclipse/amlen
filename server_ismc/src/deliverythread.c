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

/**
 * This source file contains variables functions related to asynchronous
 * delivery threads.
 */

#include <ismc.h>
#include <ismc_p.h>

/**
 * Structure containing task-related information.
 */
typedef struct ismc_delthread_tasks_t {
    ismc_consumer_t * consumer;          /**< A consumer with message listener */
    ismc_message_t * message;            /**< A message to consume             */
} ismc_delthread_tasks_t;

/**
 * Structure for delivery thread parameter passing.
 */
typedef struct ismc_delthread_parms_t {
    pthread_barrier_t * barrier;         /**< A barrier to indicate that delivery thread is started */
    int threadId;                        /**< A thread ID                                           */
} ismc_delthread_parms_t;

/**
 * Mutex for the initial setup of delivery thread-related variables.
 */
static pthread_mutex_t deliveryInitLock = PTHREAD_MUTEX_INITIALIZER;

static ism_threadh_t   * delThIDs;                     /**< Delivery thread IDs                    */
static pthread_mutex_t * delThLocks;                   /**< Per-thread mutexes                     */
static pthread_cond_t  * delThConds;                   /**< Condition for per-thread task
                                                            availability                           */
static pthread_barrier_t barrier;                      /**< Barrier for starting delivery threads  */

const int TASK_ARRAY_SIZE = 128*1024;                  /**< Maximum number of tasks for each
                                                            delivery thread                        */
static ismc_delthread_tasks_t * * taskArrays[2];       /**< Task lists for each delivery thread    */
static ismc_delthread_tasks_t * * currentTasks = NULL; /**< Pointer to a current task list for
                                                            each delivery thread                   */
static int * currSize;                                 /**< Number of tasks ready for each thread  */

static int * stopThread = NULL;                        /**< Stop indicators for each thread        */
static int DELIVERY_THREAD_NUM = 2;                    /**< Total number of delivery threads       */
static int currentThreadId     = -1;                   /**< Last used thread ID                    */
static int numInitializedThreads  = 0;                 /**< Number of initialized delivery threads */

void * deliveryThread(void * parm, void * context, int value);

/**
 * Allocate structures containing information about/for delivery threads.
 */
void ismc_allocateDeliveryThreads(void) {
    int i;

    pthread_mutex_lock(&deliveryInitLock);
    if (currentTasks == NULL) {
        char * numDelThreads = getenv("ISMMaxJMSDelThreads");
        if (numDelThreads != NULL) {
            DELIVERY_THREAD_NUM = atoi(numDelThreads);
        }
        if (DELIVERY_THREAD_NUM <= 0) {
            DELIVERY_THREAD_NUM = 2;
        }

        taskArrays[0] = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,80),DELIVERY_THREAD_NUM, sizeof(ismc_delthread_tasks_t *));
        taskArrays[1] = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,81),DELIVERY_THREAD_NUM, sizeof(ismc_delthread_tasks_t *));
        currentTasks = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,82),DELIVERY_THREAD_NUM, sizeof(ismc_delthread_tasks_t *));

        for (i = 0; i < DELIVERY_THREAD_NUM; i++) {
            taskArrays[0][i] = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,83),TASK_ARRAY_SIZE * sizeof(ismc_delthread_tasks_t));
            taskArrays[1][i] = ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,84),TASK_ARRAY_SIZE * sizeof(ismc_delthread_tasks_t));
        }

        currSize = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,85),DELIVERY_THREAD_NUM, sizeof(int));

        pthread_barrier_init(&barrier, NULL, 2);
        stopThread = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,86),sizeof(int), (DELIVERY_THREAD_NUM));
        delThIDs   = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,87),sizeof(pthread_t), (DELIVERY_THREAD_NUM));
        delThLocks = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,88),sizeof(pthread_mutex_t), (DELIVERY_THREAD_NUM));
        delThConds = ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,89),sizeof(pthread_cond_t), (DELIVERY_THREAD_NUM));
    }
    pthread_mutex_unlock(&deliveryInitLock);
}

/**
 * Get the ID of the next available delivery thread.
 * Start it, if not started yet.
 *
 * @return An ID of the available delivery thread.
 */
int ismc_getDeliveryThreadId(void) {
	ism_threadh_t tid;
    ismc_delthread_parms_t parms;
    int rc;

    pthread_mutex_lock(&deliveryInitLock);

    if (++currentThreadId >= DELIVERY_THREAD_NUM) {
        currentThreadId = 0;
    }
    if (currentThreadId >= numInitializedThreads) {
    	char buf[255];

        pthread_mutex_init(&delThLocks[currentThreadId], NULL);
        pthread_cond_init(&delThConds[currentThreadId], NULL);

        parms.barrier = &barrier;
        parms.threadId = currentThreadId;
        sprintf(buf, "ismcd%02d", currentThreadId);
        ism_common_startThread(&tid, deliveryThread, &parms, NULL, 0,
                    ISM_TUSAGE_NORMAL, 0, buf, "Message delivery thread for ISMC");
        pthread_barrier_wait(&barrier);
        numInitializedThreads++;

        delThIDs[currentThreadId] = tid;
        currentTasks[currentThreadId] = taskArrays[0][currentThreadId];
    }

    rc = currentThreadId;

    pthread_mutex_unlock(&deliveryInitLock);

    return rc;
}

/**
 * Add a task for a delivery thread to pass a message to a message listener
 * associated with given consumer.
 *
 * @param  threadId  An ID of the delivery thread to use.
 * @param  consumer  A message consumer to consume the message (via associated listener)
 * @param  message   A received message to consume.
 */
void ismc_addTask(int threadId, ismc_consumer_t * consumer, ismc_message_t * message) {

	if (threadId < 0)
		return;

    pthread_mutex_lock(&delThLocks[threadId]);

    if (!stopThread[threadId]) {
        while (currSize[threadId] == TASK_ARRAY_SIZE) {                      /* BEAM suppression: infinite loop */
            pthread_cond_wait(&delThConds[threadId], &delThLocks[threadId]);
        }

        if (!stopThread[threadId]) {
            ismc_delthread_tasks_t * task = &currentTasks[threadId][currSize[threadId]++];
            task->consumer = consumer;
            task->message = message;
            if (currSize[threadId] == 1) {
                pthread_cond_broadcast(&delThConds[threadId]);
            }
        }
    }

    pthread_mutex_unlock(&delThLocks[threadId]);
}

void * deliveryThread(void * parm, void * context, int value) {
    ismc_delthread_parms_t * parms = (struct ismc_delthread_parms_t *)parm;
    int threadId = parms->threadId;
    ismc_delthread_tasks_t * task;
    int curr = 0;

    pthread_barrier_wait(parms->barrier);

    while (!stopThread[threadId]) {                                         /* BEAM suppression: infinite loop */
        int getSize;
        ismc_delthread_tasks_t * tasks;
        int i;

        pthread_mutex_lock(&delThLocks[threadId]);

        tasks = taskArrays[curr][threadId];
        while(currSize[threadId] == 0) {                                    /* BEAM suppression: infinite loop */
            if (stopThread[threadId]) {
                pthread_mutex_unlock(&delThLocks[threadId]);
                return NULL;
            }
            pthread_cond_wait(&delThConds[threadId], &delThLocks[threadId]);
        }
        getSize = currSize[threadId];
        currSize[threadId] = 0;
        curr = (curr + 1) & 0x1;
        currentTasks[threadId] = taskArrays[curr][threadId];
        if (getSize == TASK_ARRAY_SIZE) {
            pthread_cond_broadcast(&delThConds[threadId]);
        }

        pthread_mutex_unlock(&delThLocks[threadId]);

        for (i = 0; i < getSize; i++) {
            uint64_t ack_sqn;
            ismc_onmessage_t onmessage;
            void * userdata;

            task = &tasks[i];

            ack_sqn = task->message->ack_sqn;

            /* Grab the pointer to the listener function and user data beforehand
             * to avoid problems if the listener is unset while calling
             * task->consumer->onmessage. */
            onmessage = task->consumer->onmessage;
            userdata = task->consumer->userdata;
            if (onmessage && !task->consumer->isClosed) {
            	int sendResume = 1;

                onmessage(task->message, task->consumer, userdata);

                if (!task->consumer->isClosed && !task->consumer->disableACK) {
                    if ((ack_sqn - task->consumer->lastDelivered) > 0) {
                        task->consumer->lastDelivered = ack_sqn;
                        task->consumer->session->lastDelivered = ack_sqn;
                    }
                    if (task->consumer->session->ackmode == SESSION_AUTO_ACKNOWLEDGE) {
                        ismc_acknowledgeInternalSync(task->consumer);
                        sendResume = (task->consumer->fullSize != 0);
                    }
                }

                ismc_consumerCachedMessageRemoved(task->consumer, __func__, task->message, sendResume);
            }

            ismc_freeMessage(task->message);

            task->consumer = NULL;
            task->message  = NULL;
        }
    }

    return NULL;
}

/**
 * Stop all delivery threads and release associated resources.
 */
void stopThreads(void) {
    pthread_mutex_lock(&deliveryInitLock);

    if (currentTasks != NULL) {
        int i, j, k;

        for (i = 0; i < DELIVERY_THREAD_NUM; i++) {
            stopThread[i] = 1;
            pthread_cond_broadcast(&delThConds[i]);
            ism_common_joinThread(delThIDs[i], NULL);

            pthread_mutex_destroy(&delThLocks[i]);
            pthread_cond_destroy(&delThConds[i]);
        }

        for (k = 0; k < 2; k++) {
            for (i = 0; i < DELIVERY_THREAD_NUM; i++) {
                for (j = 0; j < TASK_ARRAY_SIZE; j++) {
                    if (taskArrays[k][i] != NULL &&
                            taskArrays[k][i][j].message != NULL) {
                        ismc_freeMessage(taskArrays[k][i][j].message);
                    }
                }
                ism_common_free(ism_memory_ismc_misc,taskArrays[k][i]);
            }
            ism_common_free(ism_memory_ismc_misc,taskArrays[k]);
            taskArrays[k] = NULL;
        }
        ism_common_free(ism_memory_ismc_misc,currentTasks);
        currentTasks = NULL;

        ism_common_free(ism_memory_ismc_misc,currSize);

        pthread_barrier_destroy(&barrier);
        ism_common_free(ism_memory_ismc_misc,stopThread);
        ism_common_free(ism_memory_ismc_misc,delThIDs);
        ism_common_free(ism_memory_ismc_misc,delThLocks);
        ism_common_free(ism_memory_ismc_misc,delThConds);
    }

    pthread_mutex_unlock(&deliveryInitLock);

    pthread_mutex_destroy(&deliveryInitLock);
}
