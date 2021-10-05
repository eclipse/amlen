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
 */

#define TRACE_COMP Admin

#include <threadpool.h>
#include <assert.h>

/* Thread local key */
static pthread_key_t 	worker_id;
static ism_worker_t * 	workerThreads;
static unsigned int   	next_worker;
static int        		worker_count;
static unsigned int     next_ltpaWorker;
static int              ltpaWorker_count;
static int              worker_status;



int ism_security_getWorkerCount(void){
    return worker_count;
}

/*
 * Start up the worker thread manager.
 *
 * The count of workers is normally set using the WorkerThreads configuration item.
 */
int ism_security_initThreadPool(int numWorkers, int numLTPAworkers) {
    /* If a zero or negative number is used, override it */
    if (numWorkers < 1) {
        numWorkers = 1;
    }

    worker_count = numWorkers;
    next_worker=0;

    if (numLTPAworkers < 0) {
        numLTPAworkers = 1;
    }
    /* We need some security threads that aren't reserved for LTPA work to do other work */
    if (numWorkers <= numLTPAworkers) {
        ltpaWorker_count = numWorkers -1;
    } else {
        ltpaWorker_count = numLTPAworkers;
    }

    next_ltpaWorker=0;

    pthread_key_create(&worker_id, NULL);

    TRACE(2, "The ISM Worker Manager is initialized.  Count=%d\n", worker_count);
    return 0;
}

/*
 * Start the work manager.
 *
 * At this time we create the workers including all fifos. 
 * The worker is not run until work is assigned to it.
 */
int ism_security_startWorkers(void) {
	int count;
	ism_worker_t * worker;
	char tname[16];
	workerThreads = (ism_worker_t *) ism_common_calloc(ISM_MEM_PROBE(ism_memory_admin_misc,45),worker_count, sizeof(ism_worker_t));
	worker = workerThreads;
    
	/*Assigned worker ID*/
	for (count = 0; count < worker_count; count++) {
	    worker->status = WORKER_STATUS_INACTIVE;
	  
		worker->id = count+1;
		
		pthread_mutex_init(&worker->authLock, 0);
    	pthread_cond_init(&worker->authCond, 0);
    	memset(&tname,0, sizeof(tname));
    	sprintf((char *)&tname, "Security.%d", worker->id);
    	ism_common_startThread(&worker->authThread, ism_security_ldapthreadfpool,
          (void *)worker, NULL, 0, ISM_TUSAGE_NORMAL, 0, tname, "The ISM Security LDAP thread");
    	//ism_common_sleep(1000);
	    worker++;
	}
	worker_status = WORKER_STATUS_ACTIVE;
	TRACE(2, "The ISM Security Thread Pool is started. Workers: %d \n", worker_count);
	return 0;
}


/*
 * Get a worker.
 */
ism_worker_t * ism_security_getWorker(int ltpaAuth) {
    unsigned int index;
    ism_worker_t * worker = NULL;
    if (worker_status != WORKER_STATUS_ACTIVE)
        return NULL;

    if (worker_count > 1)
    {
        //Don't allow other authentication to use any reserved LTPA threads.
        if(!ltpaAuth || ltpaWorker_count == 0) {
            unsigned int numNONLPTAWorkers = worker_count - ltpaWorker_count;

            index =   ltpaWorker_count
                    + (__sync_add_and_fetch(&next_worker, 1) % numNONLPTAWorkers);
        } else {
           index = (__sync_fetch_and_add(&next_ltpaWorker, 1) % ltpaWorker_count);
        }
    } else {
        index = 0;
    }

    assert(index < worker_count);
    worker = workerThreads + index;

    return worker;
}

/*
 * Terminate workers.
 * This cannot be called from a worker thread 
 */
int ism_security_termWorkers(void) {
	int count=0;
	
	//printf("Shutdown the authentication threads: %d\n", worker_count);
	
	if(workerThreads!=NULL){
		ism_worker_t * worker= workerThreads;
		for (count = 0; count < worker_count; count++) {
			pthread_mutex_lock(&worker->authLock);
			
			worker->status = WORKER_STATUS_INACTIVE;
		    
		    pthread_cond_signal(&worker->authCond);
		    
		    pthread_mutex_unlock(&worker->authLock);
		    
		    ism_common_sleep(1000);
		    
		    
		    pthread_mutex_lock(&worker->authLock);
		    /*Wait for the thread to shutdown*/
		    while(worker->status != WORKER_STATUS_SHUTDOWN)
		    {
		    	  pthread_mutex_unlock(&worker->authLock);
		    	  ism_common_sleep(1000);
		    	  pthread_mutex_lock(&worker->authLock);
		    }
	        
	        
	        pthread_mutex_unlock(&worker->authLock);
            
	        /*Release the resource*/
	        pthread_mutex_destroy(&worker->authLock);
	        pthread_cond_destroy(&worker->authCond);
	        
		    worker++;
		}
		ism_common_free(ism_memory_admin_misc,workerThreads);
	
	}
	workerThreads = NULL;
	worker_status = WORKER_STATUS_INACTIVE;

	TRACE(2, "The ISM Security Thread Pool is stopped.\n");
	return 0;
}

