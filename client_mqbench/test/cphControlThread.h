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

#ifndef _CPHCONTROLTHREAD

#include "cphArrayList.h"
#include "cphSpinLock.h"
#include "cphDestinationFactory.h"
#include "cphThread.h"

typedef struct _cphControlThread {
	CPH_CONFIG *pConfig;
    int controlThreadId;
    int timerThreadId;
    int statsThreadId;
	CPH_ARRAYLIST *workers;
	int nextThreadNum;
	int runningWorkers;
    int shutdown;
    CPH_TIME startTime;   /* This is the effective start time - either from the start of the main thread or the first actual iteration */
    CPH_TIME threadStartTime; /* This is the start time of the main thread. This is used for the actual start time if sd=tlf */
    CPH_TIME endTime;     
    char procId[80];
    int reportTlf;
    CPH_DESTINATIONFACTORY *pDestinationFactory; /* Pointer to the control thread's destination factory. This is used to generate 
                                                    multiple topic names if these have been requested */

    CPH_SPINLOCK *ThreadLock;

    /* This is the structure used for per thread attributes (Unix only) */
#ifdef AMQ_UNIX
    CPH_THREAD_ATTR threadAttr;
#endif

} CPH_CONTROLTHREAD;

typedef struct _cphStatsThread {
    CPH_CONTROLTHREAD *pControlThread;
    int *curr;
    int currLength;
    int *prev;
    int prevLength;
} CPH_STATSTHREAD;

typedef struct _cphTimerThread {
    CPH_CONTROLTHREAD *pControlThread;
    int runLength;
} CPH_TIMERTHREAD;

void cphControlThreadIni(CPH_CONTROLTHREAD **ppControlThread, CPH_CONFIG *pConfig);
int  cphControlThreadFree(CPH_CONTROLTHREAD **ppControlThread);

void cphStatsThreadIni(CPH_STATSTHREAD **ppStatsThread, CPH_CONTROLTHREAD *pControlThread);
int  cphStatsThreadFree(CPH_STATSTHREAD **ppStatsThread);

void cphTimerThreadIni(CPH_TIMERTHREAD **ppTimerThread, CPH_CONTROLTHREAD *pControlThread);
int  cphTimerThreadFree(CPH_TIMERTHREAD **ppTimerThread);

int cphControlThreadRun(CPH_CONTROLTHREAD *pControlThread);
int cphControlThreadSetWorkers(CPH_CONTROLTHREAD *pControlThread, int number);
int cphControlThreadAddWorkers(CPH_CONTROLTHREAD *pControlThread, int number);
int cphControlThreadStartWorkers(CPH_CONTROLTHREAD *pControlThread);
int cphControlThreadIncRunners(CPH_CONTROLTHREAD *pControlThread);
int cphControlThreadDecRunners(CPH_CONTROLTHREAD *pControlThread);
int cphControlThreadStartStatsThread(CPH_CONTROLTHREAD *pControlThread);

CPH_RUN cphControlThreadStatsThreadRun(void * lpParam );
CPH_RUN cphControlThreadTimerThreadRun(void * lpParam );

int cphControlThreadStartTimerThread(CPH_CONTROLTHREAD *pControlThread);
int cphControlThreadStatsGetValues(CPH_STATSTHREAD *pStatsThread);

int cphControlThreadDoShutdown(CPH_CONTROLTHREAD *pControlThread);
int cphControlThreadSignalShutdown(CPH_CONTROLTHREAD *pControlThread);

int  cphControlThreadDisplayThreads(CPH_CONTROLTHREAD *pControlThread);

int cphControlThreadGetRunningWorkers(CPH_CONTROLTHREAD *pControlThread);

int cphControlThreadSetThreadStartTime(CPH_CONTROLTHREAD *pControlThread);

int cphControlThreadFreeWorkers(CPH_CONTROLTHREAD *pControlThread);

#define _CPHCONTROLTHREAD
#endif
