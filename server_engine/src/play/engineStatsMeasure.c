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
/* Module Name: engineStatsMeasure.c                                 */
/*                                                                   */
/* Description: Engine component that runs an operation in a loop and*/
/* times it                                                          */
/*                                                                   */
/*********************************************************************/

#include <stdio.h>
//#include <stdlib.h>
//#include <stdbool.h>
#include <pthread.h>
//#include <assert.h>
//
//#include <ismutil.h>
//#include <workers.h>
//#include <engine.h
#include "engineInternal.h"
#include "engineStats.h"

static  pthread_mutex_t engineStatsMtx;

/********************************************************************************************************/
/*                                                                                                      */
/* void measureInternal(void * pData)                                                                   */
/*                                                                                                      */
/* Special method provided for -measure option.  This method is submitted to a worker thread.           */
/* It calls the designated function in an iterative loop.                                               */
/* It determines the start and finish times and reports the average rate.                               */
/*                                                                                                      */
/********************************************************************************************************/
void measureInternal(void * pData)
{
    measureInternalData_t * userdata = (measureInternalData_t *)pData;
    int p = 0;
    ism_time_t start = 0;
    ism_time_t finish = 0;

    start = ism_common_currentTimeNanos();
    for (p = 0; p < userdata->iterations; p++) {
        userdata->pFn(userdata->pData);
    }
    finish = ism_common_currentTimeNanos();

    printf("Ave Rate for %'d iterations of %s: ", userdata->iterations, userdata->label);
    printf("%'.3f ns/call\n", ((finish - start) / (userdata->iterations * 1.0)));

    pthread_mutex_unlock(&engineStatsMtx);

}

/********************************************************************************************************/
/*                                                                                                      */
/* void measure_getCurrentWorkerID(void * pData)                                                        */
/*                                                                                                      */
/* Method passed using the the userdata configuration to worker thread.  This method is called inside   */
/* measureInternal()'s for loop.  And performance data is reported for it.                              */
/*                                                                                                      */
/*                                                                                                      */
/********************************************************************************************************/
void measure_getCurrentWorkerID(void * pData)
{
    int workerId = ism_common_getCurrentWorkerID();
    if (workerId != 0) {
        if (workerId == 1) {
            workerId = 0;
        }
    }
}

/********************************************************************************************************/
/*                                                                                                      */
/* void measure_calloc(void * pData)                                                                    */
/*                                                                                                      */
/* Method passed using the the userdata configuration to worker thread.  This method is called inside   */
/* measureInternal()'s for loop.  And performance data is reported for it.                              */
/*                                                                                                      */
/*                                                                                                      */
/********************************************************************************************************/
void measure_calloc(void * pData)
{
    void *buffer = calloc(1, sizeof(internalEngineData_t));
    free(buffer);
}

/********************************************************************************************************/
/*                                                                                                      */
/* void noopJob(void * pData)                                                                           */
/*                                                                                                      */
/* A no-op function used as the job for the submit job measurement.                                     */
/*                                                                                                      */
/*                                                                                                      */
/********************************************************************************************************/
void noopJob(void * pData)
{
    // Told you it was a no-op
    return;
}


/********************************************************************************************************/
/*                                                                                                      */
/* void measure_submitJob(void * pData)                                                                 */
/*                                                                                                      */
/* Method passed using the the userdata configuration to worker thread.  This method is called inside   */
/* measureInternal()'s for loop.  And performance data is reported for it.                              */
/*                                                                                                      */
/*                                                                                                      */
/********************************************************************************************************/
void measure_submitJob(void * pData)
{
    ism_common_submitJob((ism_worker_t *)pData, (ism_worker_job_t)noopJob, NULL, true);
}


/********************************************************************************************************/
/*                                                                                                      */
/* void measure_submitJob(void * pData)                                                                 */
/*                                                                                                      */
/* Method passed using the the userdata configuration to worker thread.  This method is called inside   */
/* measureInternal()'s for loop.  And performance data is reported for it.                              */
/*                                                                                                      */
/*                                                                                                      */
/********************************************************************************************************/
void measure_getWorkerAndSubmitJob(void * pData)
{
    ism_worker_t *hWorker = ism_common_getWorker(WORKER_CLASS_PROTOCOL);
    if (hWorker != NULL)
    {
        ism_common_submitJob(hWorker, (ism_worker_job_t)noopJob, NULL, true);
        ism_common_returnWorker(hWorker);
    }
    else
    {
        printf("ERROR:   ism_common_getWorker(WORKER_CLASS_PROTOCOL) returned %p.\n", hWorker);
    }
}


/********************************************************************************************************/
/*                                                                                                      */
/* XAPI int measure(int iterations)                                                                     */
/*                                                                                                      */
/* method provided for the -measure option.  This api submits jobs to the worker thread so that         */
/* the performance of specific segments of code can be measured.                                        */
/*                                                                                                      */
/********************************************************************************************************/

XAPI int measure(int iterations)
{
    ism_worker_job_t pFn = &measureInternal;
    measureInternalData_t userdata1 = {&measure_getCurrentWorkerID, iterations, "getCurrentWorkerID", NULL};
    measureInternalData_t userdata2 = {&measure_calloc, iterations, "calloc(1, sizeof(internalEngineData_t))", NULL};
    measureInternalData_t userdata3 = {&measure_submitJob, iterations, "ism_common_submitJob", NULL};
    measureInternalData_t userdata4 = {&measure_getWorkerAndSubmitJob, iterations, "ism_common_getWorker/submitJob", NULL};
    bool waitOnFull = true;
    int rc = 0;
    int rc2 = 0;

    /************************************************************************/
    /* initialize mutex                                                     */
    /************************************************************************/
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&engineStatsMtx, &attr);

    /* request a handle to a worker */
    ism_worker_t * hWorkerSession = ism_common_getWorker(WORKER_CLASS_ENGINE);

    if (hWorkerSession == NULL) {
        printf("ERROR:  ism_common_getWorker(WORKER_CLASS_ENGINE) returned %p\n", hWorkerSession);
    } else {

        /* submit the first performance measurement job to the worker */
        pthread_mutex_lock(&engineStatsMtx);
        rc = ism_common_submitJob(hWorkerSession, pFn, (void *)&userdata1, waitOnFull);
        if (rc != 0) {
            printf("ERROR:  ism_common_submitJob(hWorkerSession, pFn, (void *)&userdata1, waitOnFull) returned %d\n", rc);
        }
        pthread_mutex_wait(&engineStatsMtx);

        /* submit the second performance measurement job to the worker */
        pthread_mutex_lock(&engineStatsMtx);
        rc = ism_common_submitJob(hWorkerSession, pFn, (void *)&userdata2, waitOnFull);
        if (rc != 0) {
            printf("ERROR:  ism_common_submitJob(hWorkerSession, pFn, (void *)&userdata2, waitOnFull) returned %d\n", rc);
        }
        pthread_mutex_wait(&engineStatsMtx);

        /* submit the third performance measurement job to the worker */

        ism_worker_t *hWorkerMeasure = ism_common_getWorker(WORKER_CLASS_PROTOCOL);
        if (hWorkerMeasure == NULL) {
            printf("ERROR:   ism_common_getWorker(WORKER_CLASS_PROTOCOL) returned %p\n", hWorkerMeasure);
        }
        else
        {
            userdata3.pData = hWorkerMeasure;
            pthread_mutex_lock(&engineStatsMtx);
            rc = ism_common_submitJob(hWorkerSession, pFn, (void *)&userdata3, waitOnFull);
            if (rc != 0) {
                printf("ERROR:  ism_common_submitJob(hWorkerSession, pFn, (void *)&userdata3, waitOnFull) returned %d\n", rc);
            }
            pthread_mutex_wait(&engineStatsMtx);
        }

        /* submit the fourth performance measurement job to the worker */
        pthread_mutex_lock(&engineStatsMtx);
        rc = ism_common_submitJob(hWorkerSession, pFn, (void *)&userdata4, waitOnFull);
        if (rc != 0) {
            printf("ERROR:  ism_common_submitJob(hWorkerSession, pFn, (void *)&userdata4, waitOnFull) returned %d\n", rc);
        }
        pthread_mutex_wait(&engineStatsMtx);

    }

    /* return the worker handle */
    rc2 = ism_common_returnWorker(hWorkerSession);
    if (rc2 != 0) {
        printf("ERROR:  ism_common_returnWorker(hWorkerSession) returned %d\n", rc2);
        if (rc == 0)
            rc = rc2;
    }

    return rc;
}
