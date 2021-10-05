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

#ifndef _CPHWORKERTHREAD

#include "cphConfig.h"
#include "cphControlThread.h"
#include "cphThread.h"

#define sCREATED 1
#define sCONNECTING 2
#define sRUNNING 4
#define sERROR 8
#define sENDING 16
#define sENDED 32

#define mPUBLISHER  1
#define mSUBSCRIBER 2
#define mPUBLISHERV6 3
#define mSUBSCRIBERV6 4
#define mDUMMY 5
#define mSender 6
#define mReceiver 7
#define mPutGet 8
#define mRequester 9
#define mResponder 10
#define mMQTTPublisher 11
#define mMQTTSubscriber 12

typedef struct _cphWorkerThread {
   CPH_THREADID threadId;
   char threadName[80];
   int iterations;
   CPH_TIME startTime;
   CPH_TIME endTime;
   int state;
   int shutdown;
   int threadNum;
   /* int nextThreadNum; */
   char clazzName[80];

   /* CPH_STARTROUTINE (*pRunFn) ( void *); */
#if defined(WIN32)
   CPH_STARTROUTINE pRunFn;
#elif defined(AMQ_UNIX)
   void * (*pRunFn)(void *);
#endif
   /* void (*pRunFn) (void *); */

   int (*pOneIterationFn) (void *);
   CPH_CONFIG *pConfig;
   CPH_CONTROLTHREAD *pControlThread;
} CPH_WORKERTHREAD;

int cphWorkerThreadValidate(CPH_CONFIG *pConfig);
void cphWorkerThreadIni(CPH_WORKERTHREAD **ppWorkerThread, CPH_CONTROLTHREAD *pcontrolThread);
int cphWorkerThreadFree(CPH_WORKERTHREAD **ppWorkerThread);
int cphWorkerThreadPace(CPH_WORKERTHREAD *pWorkerThread, void *provider);
int cphWorkerThreadPaceControl(CPH_WORKERTHREAD *pWorkerThread, void *provider);
int cphWorkerThreadGetState(CPH_WORKERTHREAD *pWorkerThread);
int cphWorkerThreadSetState(CPH_WORKERTHREAD *pWorkerThread, int aState);
void cphWorkerThreadSignalShutdown(CPH_WORKERTHREAD *pWorkerThread);

/* Routine to retrieve the thread name from the worker thread instance */
char *cphWorkerThreadGetName(CPH_WORKERTHREAD *pWorkerThread);

int cphWorkerThreadGetIterations(CPH_WORKERTHREAD *pWorkerThread);
CPH_TIME cphWorkerThreadGetStartTime(CPH_WORKERTHREAD *pWorkerThread);
CPH_TIME cphWorkerThreadGetEndTime(CPH_WORKERTHREAD *pWorkerThread);
int cphWorkerThreadSetThreadName(CPH_WORKERTHREAD *pWorkerThread, char *prefix);
CPH_TRACE *cphWorkerThreadGetTrace(CPH_WORKERTHREAD *pWorkerThread);

#define _CPHWORKERTHREAD
#endif
