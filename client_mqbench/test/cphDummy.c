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

/*
* Change Log:
* -------- ----------- ------------- ------------------------------------
* Reason:  Date:       Originator:   Comments:
* -------- ----------- ------------- ------------------------------------
*          10-04-07    Jerry Stevens File Creation
*/


/* This module is the implementation of the "Dummy" test suite. This is provided as a test that cph is running correctly without the need for
** any configured messaging software
*/

#include "cphDummy.h"

/*
** Method: cphDummyIni
**
** Create and initialise the control block for the Dummy implementation. The worker thread control structure
** is passed in as in input parameter and is stored within the control block - which is as close as we
** can get to creating a class derived from the worker thread.
**
** Input Parameters: pWorkerThread - the worker thread control structure
**
** Output parameters: ppDummy - a double pointer to the newly created Dummy control structure
**
*/
void cphDummyIni(CPH_DUMMY **ppDummy, CPH_WORKERTHREAD *pWorkerThread) {
	CPH_DUMMY *pDummy = NULL;

    CPH_TRACE *pTrc = cphWorkerThreadGetTrace(pWorkerThread);
	
    #define CPH_ROUTINE "cphDummyIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

	if (NULL != pWorkerThread) {
 	   pDummy = malloc(sizeof(CPH_DUMMY));
	   pDummy->pWorkerThread = pWorkerThread;
       pDummy->pWorkerThread->pRunFn = cphDummyRun;
	   pDummy->pWorkerThread->pOneIterationFn = cphDummyOneIteration;

       /* Set the thread name according to our prefix */
       cphWorkerThreadSetThreadName(pWorkerThread, CPH_DUMMY_THREADNAME_PREFIX);
    }

	   *ppDummy = pDummy;

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
}

/*
** Method: cphDummyFree
**
** Free the Dummy control block. The routine that disposes of the control thread connection block calls 
** this function for every Dummy worker thread.
**
** Input/output Parameters: Double pointer to the Dummy control block. When the Dummy control
** structure is freed, the single pointer to the control structure is returned as NULL.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphDummyFree(CPH_DUMMY **ppDummy) {
    CPH_WORKERTHREAD *pWorkerThread;
    CPH_TRACE *pTrc;
    int res = CPHFALSE;

    pWorkerThread = (*ppDummy)->pWorkerThread;
    pTrc = cphWorkerThreadGetTrace(pWorkerThread);
	
    #define CPH_ROUTINE "cphDummyFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

	if (NULL != *ppDummy) {
        if (NULL != (*ppDummy)->pWorkerThread) cphWorkerThreadFree( &(*ppDummy)->pWorkerThread );
		free(*ppDummy);
		*ppDummy = NULL;
		res = CPHTRUE;
	}
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
	return(res);
}

/*
** Method: cphDummyRun
**
** This is the main execution method for the Dummy thread. After initial set up, it calls the worker thread pace 
** method. The latter is responssible for calling the cphDummyOneIteration method at the correct rate according to the command line
** parameters. 
**
** Input Parameters: lpParam - a void * pointer to the Dummy control structure. This has to be passed in as 
** a void pointer as a requirement of the thread creation routines.
**
** Returns: On windows - CPHTRUE on successful execution, CPHFALSE otherwise
**          On Linux   - NULL
**
*/
CPH_RUN cphDummyRun( void * lpParam) {

	CPH_DUMMY *pDummy;
	CPH_WORKERTHREAD * pWorker;
    CPH_TRACE *pTrc;
	char messageString[128];

	pDummy = lpParam;
	pWorker = pDummy->pWorkerThread;

    pTrc = cphWorkerThreadGetTrace(pWorker);
    #define CPH_ROUTINE "cphDummyRun"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );


	sprintf(messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : START");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGINFO, messageString);
    
    cphWorkerThreadSetState(pWorker, sRUNNING);

	sprintf(messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : Entering client loop");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGVERBOSE, messageString);

    cphWorkerThreadPace(pWorker, pDummy);
 
	sprintf(messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : STOP");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGINFO, messageString);

    //state = (state&sERROR)|sENDED;
    cphWorkerThreadSetState(pWorker, (cphWorkerThreadGetState(pWorker) & sERROR) | sENDED);

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    /* Return the right type of variable according to the o/s. (This return is not used) */
    return CPH_RUNRETURN;
}

/*
** Method: cphDummyOneIteration
**
** This method is called by cphWorkerThreadPace to perform a single iteration in the Dummy thread. It performs
** some simple floating point arithmetic to simulate the cpu load of a real messaging operation. 
**
** Input Parameters: a void pointer to the Dummy control structure
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphDummyOneIteration(void *aProvider) {
	CPH_DUMMY *pDummy;
    CPH_WORKERTHREAD *pWorkerThread;
    CPH_TRACE *pTrc;

	int i;
	double x = 2.0;

	pDummy = aProvider;
	pWorkerThread = pDummy->pWorkerThread;

    pTrc = cphWorkerThreadGetTrace(pWorkerThread);
    #define CPH_ROUTINE "cphDummyOneIteration"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );


	pDummy = aProvider;

	for (i=0; i < 10; i++) {
       x *= 3.14159;
	}

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
    return(CPHTRUE);
}
