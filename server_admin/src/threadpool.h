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
#ifndef __ISM_THREADPOOL_DEFINED
#define __ISM_THREADPOOL_DEFINED

#include <pthread.h>
#include <ldaputil.h>


#define WORKER_STATUS_INACTIVE 0
#define WORKER_STATUS_ACTIVE   1
#define WORKER_STATUS_STARTING 2
#define WORKER_STATUS_SHUTDOWN 3
/*
 * Work Thread object
 */
typedef struct ism_worker_t {
	int       id;
	int       jobcount;
	short     status;
	int       cpu_id;
	short     stopped;
	short     isInited;
	ism_threadh_t tid;
	//pthread_spinlock_t lock __attribute__ ((aligned (64)));
	
	pthread_mutex_t    authLock;
	pthread_cond_t     authCond;
	ism_threadh_t      authThread;
	ismAuthEvent_t *    authHead;
	ismAuthEvent_t *    authTail;
}ism_worker_t;

int ism_security_initThreadPool(int numWorkers, int numLTPAWorkers);

int ism_security_startWorkers(void);

ism_worker_t * ism_security_getWorker(int ltpaAuth);

int ism_security_getWorkerCount();

int ism_security_termWorkers(void);

void * ism_security_ldapthreadfpool(void * param, void * context, int value) ;



#endif
