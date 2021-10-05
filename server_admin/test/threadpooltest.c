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

/*
 * File: config_test.c
 * Component: server
 * SubComponent: server_admin
 *
 * Created on:
 *     Author:
 * --------------------------------------------------------------
 *
 *
 */
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <admin.h>
#include <threadpool.h>


void test_ism_security_initThreadPool(void)
{
    int rc = ism_security_initThreadPool(4, 1);
    CU_ASSERT(rc==0);

    int workercount = ism_security_getWorkerCount();
    CU_ASSERT(workercount==4);

    rc = ism_security_initThreadPool(-1, -1);
    CU_ASSERT(rc==0);

    workercount = ism_security_getWorkerCount();
    CU_ASSERT(workercount==1);
}

void test_ism_security_startThreadPool(void)
{
    int numThreads=10;
    int rc = ism_security_initThreadPool(numThreads, 0);
    CU_ASSERT(rc==0);

    int workercount = ism_security_getWorkerCount();
    CU_ASSERT(workercount==numThreads);

    rc = ism_security_startWorkers();
    CU_ASSERT(rc==0);

    /* Wait for the workers to become available */
    sleep(2);

    rc= ism_security_termWorkers();

    CU_ASSERT(rc==0);
    CU_ASSERT(ism_security_getWorker(1) == NULL);
}


void test_ism_security_getWorker(void)
{
    int numThreads=10;
    int numLTPAThreads = 2;
    int rc = ism_security_initThreadPool(numThreads, numLTPAThreads);
    CU_ASSERT(rc==0);

    int workercount = ism_security_getWorkerCount();
    CU_ASSERT(workercount==numThreads);

    rc = ism_security_startWorkers();
    CU_ASSERT(rc==0);

    /* Wait for the workers to become available */
    sleep(2);

    int workerSelection[numThreads];
    memset(&workerSelection, 0, sizeof(workerSelection));

    /* Add 200 requests varying the number that ask for LTPA auth */
    for(int i=0; i<200; i++) {
        int ltpaAuth = rand()%2;
        ism_worker_t * worker = ism_security_getWorker(ltpaAuth);
        CU_ASSERT(worker!=NULL);
        CU_ASSERT(worker->id <= numThreads);

        if (ltpaAuth) {
            /* LTPA should return 1 or 2 */
            CU_ASSERT(worker->id <= numLTPAThreads);
        } else {
            /* Non LTPA should return a thread not reserved for LTPA work*/
            CU_ASSERT(worker->id > numLTPAThreads);
        }
        workerSelection[worker->id-1] += 1;
    }

    /* Check for an even spread (LTPA) */
    int delta = workerSelection[0]-workerSelection[1];
    CU_ASSERT((delta >= -1 && delta <= 1));

    /* Check for an even spread (non LTPA) */
    int prevThread=0;
    for(int i=numLTPAThreads; i<numThreads; i++) {
        if (prevThread != 0) {
            delta = workerSelection[i]-workerSelection[prevThread];
            CU_ASSERT((delta >= -1 && delta <= 1));
        }
        prevThread = i;
    }

    rc= ism_security_termWorkers();
    CU_ASSERT(rc==0);
    CU_ASSERT(ism_security_getWorker(1) == NULL);

    /***** Try with just 2 threads *****/
    numThreads = 2;
    numLTPAThreads = 2; //Will be reduced to be <numThreads i.e. 1
    rc = ism_security_initThreadPool(numThreads, numLTPAThreads);
    CU_ASSERT(rc==0);

    workercount = ism_security_getWorkerCount();
    CU_ASSERT(workercount==numThreads);

    rc = ism_security_startWorkers();
    CU_ASSERT(rc==0);

    /* Wait for the workers to become available */
    sleep(2);

    /* Add 200 requests varying the number that ask for LTPA auth */
    for(int i=0; i<200; i++) {
        int ltpaAuth = rand()%2;
        ism_worker_t * worker = ism_security_getWorker(ltpaAuth);
        CU_ASSERT(worker!=NULL);
        if (ltpaAuth) {
            /* LTPA should always be 1 */
            CU_ASSERT(worker->id == 1);
        } else {
            /* Non LTPA should always be 2 */
            CU_ASSERT(worker->id == 2);
        }
    }

    rc= ism_security_termWorkers();
    CU_ASSERT(rc==0);


    /***** Try with no dedicated LTPAThreads: */
    numThreads = 2;
    numLTPAThreads = 0;
    rc = ism_security_initThreadPool(numThreads, numLTPAThreads);
    CU_ASSERT(rc==0);

    workercount = ism_security_getWorkerCount();
    CU_ASSERT(workercount==numThreads);

    rc = ism_security_startWorkers();
    CU_ASSERT(rc==0);

    /* Wait for the workers to become available */
    sleep(2);
    memset(&workerSelection, 0, sizeof(workerSelection));

    /* Add 200 requests varying the number that ask for LTPA auth */
    for(int i=0; i<200; i++) {
         int ltpaAuth = rand()%2;
         ism_worker_t * worker = ism_security_getWorker(ltpaAuth);
         CU_ASSERT(worker!=NULL);
         workerSelection[worker->id-1] += 1;
     }

    /* Check for an even spread  */
    for(int i=1; i<numThreads; i++) {
        delta = workerSelection[i]-workerSelection[i-1];
        CU_ASSERT((delta >= -1 && delta <= 1));
    }

    rc= ism_security_termWorkers();
    CU_ASSERT(rc==0);
}
