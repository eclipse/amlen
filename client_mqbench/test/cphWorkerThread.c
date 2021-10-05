const static char sccsid[] = "%Z% %W% %I% %E% %U%";
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

#include <string.h>

#include "cphUtil.h"
#include "cphWorkerThread.h"


/*
** Method: cphWorkerThreadValidate
**
** This method validates and sets up the worker thread according to the module it is going
** to run (i.e. Publisher or Subsriber).
**
** Input Parameters: pConfig - pointer to the configuration control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphWorkerThreadValidate(CPH_CONFIG *pConfig) {
	CPH_TRACE *pTrc;
    char errorString[128];

    pTrc = pConfig->pTrc;
    #define CPH_ROUTINE "cphWorkerThreadValidate"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

	if (CPHTRUE != cphConfigGetString(pConfig, pConfig->clazzName, "tc")) {
		strcpy(errorString, "worker class [");
		strcat(errorString, pConfig->clazzName);
		strcat(errorString, "] not found");
		cphConfigInvalidParameter(pConfig, errorString);
		return(CPHFALSE);
	}

	if (0 == strcmp(pConfig->clazzName, "Publisher"))  pConfig->moduleType = mPUBLISHER;
	else if (0 == strcmp(pConfig->clazzName, "Subscriber")) pConfig->moduleType = mSUBSCRIBER;
	else if (0 == strcmp(pConfig->clazzName, "PublisherV6"))  pConfig->moduleType = mPUBLISHERV6;
	else if (0 == strcmp(pConfig->clazzName, "SubscriberV6")) pConfig->moduleType = mSUBSCRIBERV6;
	else if (0 == strcmp(pConfig->clazzName, "Dummy")) pConfig->moduleType = mDUMMY;
	else if (0 == strcmp(pConfig->clazzName, "Sender")) pConfig->moduleType = mSender;
	else if (0 == strcmp(pConfig->clazzName, "Receiver")) pConfig->moduleType = mReceiver;
	else if (0 == strcmp(pConfig->clazzName, "PutGet")) pConfig->moduleType = mPutGet;
	else if (0 == strcmp(pConfig->clazzName, "Requester")) pConfig->moduleType = mRequester;
	else if (0 == strcmp(pConfig->clazzName, "Responder")) pConfig->moduleType = mResponder;
    else if (0 == strcmp(pConfig->clazzName, "MQTTPublisher")) pConfig->moduleType = mMQTTPublisher;
    else if (0 == strcmp(pConfig->clazzName, "MQTTSubscriber")) pConfig->moduleType = mMQTTSubscriber;

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

	return CPHTRUE;
}

/*
** Method: cphWorkerThreadIni
**
** This method creates and initialises a worker thread control block
**
** Input parameters: pControlThread - a pointer to the control thread control structure
**
** Output Parameters: ppWorkerThread - a double pointer to the newly created worker thread control structure
**
*/
void cphWorkerThreadIni(CPH_WORKERTHREAD **ppWorkerThread, CPH_CONTROLTHREAD *pControlThread) {
	CPH_WORKERTHREAD *pWorkerThread;

	pWorkerThread = malloc(sizeof(CPH_WORKERTHREAD));

	pWorkerThread->threadId = 0;
	pWorkerThread->iterations = 0;
	
    /* Initialise the worker thread start and end times */
    cphUtilTimeIni(&pWorkerThread->startTime);
	cphUtilTimeIni(&pWorkerThread->endTime);

    pWorkerThread->state = sCREATED | sENDED;
	pWorkerThread->shutdown = CPHFALSE;
	pWorkerThread->pRunFn = NULL;
	pWorkerThread->pOneIterationFn = NULL;
	pWorkerThread->pConfig = pControlThread->pConfig;

	pWorkerThread->threadNum = pControlThread->nextThreadNum;
    /* cphControlThreadIncNextThreadNum(pControlThread); */

	pWorkerThread->pControlThread = pControlThread;
	*ppWorkerThread = pWorkerThread;
}

/*
** Method: cphWorkerThreadFree
**
** This methid frees the worker thread control structure and is called after the worker thread has finished
** executing
**
** Input Parameters: pWorkerThread - a double pointer to the worker thread control structure
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphWorkerThreadFree(CPH_WORKERTHREAD **ppWorkerThread) {
	if (NULL != *ppWorkerThread) {
		free(*ppWorkerThread);
		*ppWorkerThread = NULL;
		return(CPHTRUE);
	}
    return(CPHFALSE);
}

/*
** Method: cphWorkerThreadPace
**
** This method is a wrapper around cphWorkerThreadPaceControl and ensures the number of threads and thread state
** is set reliably.
**
** Input Parameters: pWorkerThread - pointer to the worker thread control structure
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphWorkerThreadPace(CPH_WORKERTHREAD *pWorker, void *provider) {
   CPH_CONFIG *pConfig;
   CPH_TRACE *pTrc;

   int status; 

   pConfig = pWorker->pConfig;
   pTrc = pConfig->pTrc;
   #define CPH_ROUTINE "cphWorkerThreadPace"
   CPHTRACEENTRY(pTrc, CPH_ROUTINE );

   //cphWorkerThreadSetState(pWorker, sRUNNING);
   cphControlThreadIncRunners(pWorker->pControlThread);
   status = cphWorkerThreadPaceControl(pWorker, provider);
   cphControlThreadDecRunners(pWorker->pControlThread);
   //cphWorkerThreadSetState(pWorker, (cphWorkerThreadGetState(pWorker) & sERROR) | sENDED);

   CPHTRACEEXIT(pTrc, CPH_ROUTINE);
   #undef CPH_ROUTINE

   return status;
}

/*
** Method: cphWorkerThreadPaceControl
**
** This method controls the execution rate of the worker thread. It calls the OneIteration function in the Publisher
** or subscriber at a prescribed rate according to the specified execution options.
**
** Input Parameters: pWorkerThread - pointer to the worker thread control structure
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphWorkerThreadPaceControl(CPH_WORKERTHREAD *pWorkerThread, void *provider) {

    CPH_CONFIG *pConfig;
    CPH_TRACE *pTrc;
	int rate;
	int messages;
	int yieldRate;
    int usingMG;

	/* Following variables used in ramping code */
	int rampTime;
	int granularity = 1000;
	CPH_TIME start, end;
    long taken;
	int rateI;
    int iterationBatchSize;
	long rampTimeI;
    CPH_TIME ramp_start;
	int number;

    int status;

    pConfig = pWorkerThread->pConfig;
    pTrc = pConfig->pTrc;
    #define CPH_ROUTINE "cphWorkerThreadPaceControl"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );


	if (CPHTRUE != cphConfigGetInt(pConfig, &rate, "rt")) 
		return(CPHFALSE);
	CPHTRACEMSG(pTrc, CPH_ROUTINE, "Msg Rate: %d.", rate);

	if (CPHTRUE != cphConfigGetInt(pConfig, &messages, "mg")) 
		return(CPHFALSE);
	CPHTRACEMSG(pTrc, CPH_ROUTINE, "Iterations : %d.", messages);

	usingMG = messages>0;


	if (CPHTRUE != cphConfigGetInt(pConfig, &yieldRate, "yd")) 
		return(CPHFALSE);
	CPHTRACEMSG(pTrc, CPH_ROUTINE, "Frequency to call yield: %d.", yieldRate);

	pWorkerThread->startTime = cphUtilGetCurrentTimeMillis();


		if ( rate==0 ) {
			/*  We are not trying to fix the rate */
			
			if ( usingMG ) {
				/* send n messages */
				while( !pWorkerThread->shutdown && messages-->0 && (CPHFALSE != (status = pWorkerThread->pOneIterationFn(provider))) ) {
                    if (CPHTRUE == status) pWorkerThread->iterations++;
					if ( yieldRate!=0 && (messages%yieldRate==0) ) {
						cphThreadYield();
					}
					/* No op */
				}
			} else {
				/* simple unfettered loop */
				while( !pWorkerThread->shutdown && (CPHFALSE != (status = pWorkerThread->pOneIterationFn(provider))) ) {
                    if (CPHTRUE == status) pWorkerThread->iterations++;
					if ( yieldRate!=0 && messages++%yieldRate==0 ) {
						cphThreadYield();
					}
				}				
			} /* end if messages */
		} else {

	       if (CPHTRUE != cphConfigGetInt(pConfig, &rampTime, "rp")) return(CPHFALSE);
		   rampTime *= 1000;
	       CPHTRACEMSG(pTrc, CPH_ROUTINE, "Ramptime is: %d.", rampTime);

			rateI = rate;
			iterationBatchSize = rate;
			rampTimeI = rampTime;
			ramp_start = cphUtilGetCurrentTimeMillis();

				while ( !pWorkerThread->shutdown ) {
					
					start = cphUtilGetCurrentTimeMillis();
					
					/* RAMPING CODE  */
					if ( rampTimeI > 0 ) {

						/* rampTimeI = rampTime - (start - ramp_start); */
						rampTimeI = rampTime - cphUtilGetTimeDifference(start, ramp_start);
						if ( rampTimeI <= 0 ) {
							/* Ramping over */
							rateI = rate;
                            CPHTRACEMSG(pTrc, CPH_ROUTINE, "Ramping over, rateI: %d.", rateI);

                        } else {
							rateI = (int)(rate * ( 1 - (double)rampTimeI/rampTime ));
                            CPHTRACEMSG(pTrc, CPH_ROUTINE, "Still ramping, rateI: %d.", rateI);
						}
						
						iterationBatchSize = rateI;  /* temporary */
					} /* end if ramping */
					
					number = iterationBatchSize;
					
					if ( usingMG ) {
						while( !pWorkerThread->shutdown && number-->0 &&  (CPHFALSE != (status = pWorkerThread->pOneIterationFn(provider))) && messages-->0 ) {
                            if (CPHTRUE == status) pWorkerThread->iterations++;
							if ( yieldRate!=0 && messages%yieldRate==0 ) {
								cphThreadYield();
							}
						} /* end while */
						if (messages<=0) return(CPHTRUE);
					} else {
						while( !pWorkerThread->shutdown && number-->0 &&  (CPHFALSE != (status = pWorkerThread->pOneIterationFn(provider))) ) {
                            if (CPHTRUE == status) pWorkerThread->iterations++;
                            if ( yieldRate!=0 && messages++%yieldRate==0 ) {
								cphThreadYield();
							}
						} /* end while */
					}
					end = cphUtilGetCurrentTimeMillis();
					taken = (int) cphUtilGetTimeDifference(end, start);
 					if ( taken < granularity ) {
                        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Sleeping for %d secs.", granularity - (int) taken);
						cphUtilSleep( granularity - (int) taken );
					} else {
						/* TODO we cannot keep up this rate */
					}
				} /* end while !shutdown */
				

		}

    pWorkerThread->endTime = cphUtilGetCurrentTimeMillis();

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
** Method: cphWorkerThreadGetState
**
** Return the current "state" of the worker thread. See cphWorkerThread.h for the different states 
** the worker thread can be in.
**
** Input Parameters: pWorkerThread - pointer to the worker thread control structure
**
** Returns: the worker thread state (int)
**
*/
int cphWorkerThreadGetState(CPH_WORKERTHREAD *pWorkerThread) {

    #define CPH_ROUTINE "cphWorkerThreadGetState"
    CPHTRACEENTRY(pWorkerThread->pConfig->pTrc, CPH_ROUTINE );

    CPHTRACEEXIT(pWorkerThread->pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
    
    return pWorkerThread->state;
}

/*
** Method: cphWorkerThreadSetState
**
** Set the current "state" of the worker thread. See cphWorkerThread.h for the different states
** the worker thread can be in.
**
** Input Parameters: pWorkerThread - pointer to the worker thread control structure
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphWorkerThreadSetState(CPH_WORKERTHREAD *pWorkerThread, int aState) {

    #define CPH_ROUTINE "cphWorkerThreadSetState"
    CPHTRACEENTRY(pWorkerThread->pConfig->pTrc, CPH_ROUTINE );

    pWorkerThread->state = aState;

    CPHTRACEEXIT(pWorkerThread->pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
** Method: cphWorkerThreadSignalShutdown
**
** This method is called by the control thread to tell a worker thread to close down
**
** Input Parameters: pWorkerThread - pointer to the worker thread control structure
**
*/
void cphWorkerThreadSignalShutdown(CPH_WORKERTHREAD *pWorkerThread) {

    #define CPH_ROUTINE "cphWorkerThreadSignalShutdown"
    CPHTRACEENTRY(pWorkerThread->pConfig->pTrc, CPH_ROUTINE );

	pWorkerThread->shutdown = CPHTRUE;

    CPHTRACEEXIT(pWorkerThread->pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
}

/*
** Method: cphWorkerThreadGetName
**
** This method returns the thread name of the worker thread. (This is name is used 
** in the logging and tracing).
**
** Input Parameters: pWorkerThread - pointer to the worker thread control structure
**
** Returns: a character string containing the thread name
**
*/
char *cphWorkerThreadGetName(CPH_WORKERTHREAD *pWorkerThread) {
    #define CPH_ROUTINE "cphWorkerThreadGetName"
    CPHTRACEENTRY(pWorkerThread->pConfig->pTrc, CPH_ROUTINE );

    CPHTRACEEXIT(pWorkerThread->pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

	return pWorkerThread->threadName;
}

/*
** Method: cphWorkerThreadGetIterations
**
** This method returns the current number of iterations executed by this worker thread.
**
** Input Parameters: pWorkerThread - pointer to the worker thread control structure
**
** Returns: the number of iterations executed by this worker thread
**
*/
int cphWorkerThreadGetIterations(CPH_WORKERTHREAD *pWorkerThread) {

    #define CPH_ROUTINE "cphWorkerThreadGetIterations"
    CPHTRACEENTRY(pWorkerThread->pConfig->pTrc, CPH_ROUTINE );

    CPHTRACEEXIT(pWorkerThread->pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    return (pWorkerThread->iterations);
}

/*
** Method: cphWorkerThreadGetStartTime
**
** This method returns the start time of the worker thread. This is set at the beginning of the 
** pace method. (cphWorkerThreadPace).
**
** Input Parameters:
**
** Returns: the start time of the worker thread as a CPH_TIME value
**
*/
CPH_TIME cphWorkerThreadGetStartTime(CPH_WORKERTHREAD *pWorkerThread) {

    #define CPH_ROUTINE "cphWorkerThreadGetStartTime"
    CPHTRACEENTRY(pWorkerThread->pConfig->pTrc, CPH_ROUTINE );

    CPHTRACEEXIT(pWorkerThread->pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    return pWorkerThread->startTime;
}

/*
** Method: cphWorkerThreadGetEndTime
**
** This method returns the end time of the worker thread. This is set at the end of the 
** pace method. (cphWorkerThreadPace).
**
** Input Parameters: pWorkerThread - pointer to the worker thread control structure
**
** Returns: the end time of the worker thread as a CPH_TIME value
**
*/
CPH_TIME cphWorkerThreadGetEndTime(CPH_WORKERTHREAD *pWorkerThread) {

    #define CPH_ROUTINE "cphWorkerThreadGetEndTime"
    CPHTRACEENTRY(pWorkerThread->pConfig->pTrc, CPH_ROUTINE );

    CPHTRACEEXIT(pWorkerThread->pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    return pWorkerThread->endTime;
}

/*
** Method: cphWorkerThreadSetThreadName
**
** This method sets the thread name for this thread. This is used in logging and tracing. A
** prefix is provided by the publisher or subscriber and this is appended with the thread
** number
**
** Input Parameters: pWorkerThread - pointer to the worker thread control structure
                     prefix - the character string denoting the prefix to the name (e.g. "Publisher").
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphWorkerThreadSetThreadName(CPH_WORKERTHREAD *pWorkerThread, char *prefix) {
	char sThreadNum[80];

    #define CPH_ROUTINE "cphWorkerThreadSetThreadName"
    CPHTRACEENTRY(pWorkerThread->pConfig->pTrc, CPH_ROUTINE );

	strcpy(pWorkerThread->threadName, prefix);
	sprintf(sThreadNum, "%d", pWorkerThread->threadNum);
	strcat(pWorkerThread->threadName, sThreadNum);
    CPHTRACEMSG(pWorkerThread->pConfig->pTrc, CPH_ROUTINE, "Threadname set to: %s.", pWorkerThread->threadName);

    CPHTRACEEXIT(pWorkerThread->pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    return (CPHTRUE);
}

/*
** Method: cphWorkerThreadGetTrace
**
** This method returns a pointer to the trace structure known to the worker thread structure. It is called by
** the publisher and subscribers.
**
** Input Parameters: pWorkerThread - a pointer to the worker thread conrol structure
**
** Returns: A pointer to the trace structure
**
*/
CPH_TRACE *cphWorkerThreadGetTrace(CPH_WORKERTHREAD *pWorkerThread) {

    #define CPH_ROUTINE "cphWorkerThreadGetTrace"
    CPHTRACEENTRY(pWorkerThread->pConfig->pTrc, CPH_ROUTINE );

    CPHTRACEEXIT(pWorkerThread->pConfig->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    return pWorkerThread->pConfig->pTrc;
}
