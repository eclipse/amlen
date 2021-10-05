/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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
* Change Log:
* -------- ----------- ------------- ------------------------------------
* Reason:  Date:       Originator:   Comments:
* -------- ----------- ------------- ------------------------------------
*          10-04-07    Jerry Stevens File Creation
*/

#include "cphWorkerThread.h"

#ifndef _CPHDUMMY

/* This is the prefix for the thread names used in logging and tracing */
#define CPH_DUMMY_THREADNAME_PREFIX "Dummy"

/* The Dummy control structure */
typedef struct _cphdummy {
   CPH_WORKERTHREAD *pWorkerThread; /* Pointer to the worker thread control structure associated with this thread */
} CPH_DUMMY;

/* Function prototypes */
void cphDummyIni(CPH_DUMMY **ppDummy, CPH_WORKERTHREAD *pWorkerThread);
int cphDummyFree(CPH_DUMMY **ppDummy);

CPH_RUN cphDummyRun( void * lpParam );

int cphDummyOneIteration(void *aProvider);

#define _CPHDUMMY
#endif
