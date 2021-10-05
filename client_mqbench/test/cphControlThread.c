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
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>
#include <inttypes.h>

#include "cphConfig.h"
#include "cphControlThread.h"
#include "cphWorkerThread.h"
#include "cphListIterator.h"
#include "cphPublisher.h"
//#include "cphPublisherV6.h"
#include "cphSubscriber.h"
//#include "cphSubscriberV6.h"
#include "cphDummy.h"
#include "cphSender.h"
#include "cphReceiver.h"
#include "cphPutGet.h"
#include "cphRequester.h"
#include "cphResponder.h"
#if 0
 #include "cphMQTTPublisher.h"
 #include "cphMQTTSubscriber.h"
#endif
#include "cphLog.h"
#include "cphUtil.h"
#include "cphStringBuffer.h"

/* This static variable is used by cphUtilSigInt when a control C is invoked */
int cphControlCInvoked = CPHFALSE;

/*
** Method: cphControlThreadIni
**
** This method initialises the control block used by the control thread
**
** Input Parameters: pConfig - a pointer to the configuration control block
**
** Output parameters: ppControlThread - a pointer to a pointer for the newly created control block
*/
void cphControlThreadIni(CPH_CONTROLTHREAD **ppControlThread, CPH_CONFIG *pConfig) {
    CPH_CONTROLTHREAD *pControlThread;
    CPH_TRACE *pTrc;

    pTrc = pConfig->pTrc;
#define CPH_ROUTINE "cphControlThreadIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    pControlThread = malloc(sizeof(CPH_CONTROLTHREAD));
    if (NULL != pControlThread) {
        pControlThread->controlThreadId = 0;
        pControlThread->timerThreadId = 0;
        pControlThread->statsThreadId = 0;
        pControlThread->nextThreadNum = 0;
        pControlThread->pConfig = pConfig;
        cphArrayListIni(&pControlThread->workers);
        pControlThread->runningWorkers=0;
        pControlThread->shutdown = CPHFALSE;
        pControlThread->reportTlf = CPHFALSE;
        cphDestinationFactoryIni(&(pControlThread->pDestinationFactory), pConfig);
        (void) cphSpinLock_Init( &(pControlThread->ThreadLock));
        cphUtilTimeIni(&pControlThread->startTime);
        cphUtilTimeIni(&pControlThread->endTime);
        #ifdef AMQ_UNIX
           cphThreadIniAttr(&pControlThread->threadAttr);
        #endif
    }

    *ppControlThread = pControlThread;

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
}


/*
** Method: cphControlThreadFree
**
** This method performs the following actions:
**
** - It provides a list of the thread ids of all threads in the program if running in trace mode. 
** - It disposes of the list of worker threads.
** - It disposes of the control thread control block.
** 
**
** Input Parameters: ppControlThread - A doubly indirect reference to the contol thread control block.
**                                     This is set to NULL after the control block is freed.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int  cphControlThreadFree(CPH_CONTROLTHREAD **ppControlThread) {
    CPH_TRACE *pTrc;
    int status = CPHFALSE;;

    pTrc = (*ppControlThread)->pConfig->pTrc;
#define CPH_ROUTINE "cphControlThreadFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (NULL != *ppControlThread) {

        if (CPHTRUE == cphTraceIsOn((*ppControlThread)->pConfig->pTrc))
            cphControlThreadDisplayThreads(*ppControlThread);
        cphDestinationFactoryFree(&(*ppControlThread)->pDestinationFactory);

        /* Dispose of all of the provider/subscriber control blocks */
        cphControlThreadFreeWorkers(*ppControlThread);
        cphArrayListFree(&(*ppControlThread)->workers);
        cphSpinLock_Destroy( &((*ppControlThread)->ThreadLock));
        free(*ppControlThread);
        *ppControlThread = NULL;
        status = CPHTRUE;
    }

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(status);
}

/*
** Method: cphStatsThreadIni
**
** This method creates and initialises the control block for the stats thread
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Output parameters: ppStatsThread - a pointer to a pointer to the new control block
**
*/
void cphStatsThreadIni(CPH_STATSTHREAD **ppStatsThread, CPH_CONTROLTHREAD *pControlThread) {
    CPH_TRACE *pTrc;
    CPH_STATSTHREAD *pStatsThread;

    pTrc = pControlThread->pConfig->pTrc;
#define CPH_ROUTINE "cphStatsThreadIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    pStatsThread = malloc(sizeof(CPH_STATSTHREAD));
    if (NULL != pStatsThread) {
        pStatsThread->pControlThread = pControlThread;
        pStatsThread->curr = NULL;
        pStatsThread->currLength = 0;
        pStatsThread->prev = NULL;
        pStatsThread->prevLength = 0;
    }
    *ppStatsThread = pStatsThread;

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

}


/*
** Method: cphStatsThreadFree
**
** This method creates and initialises the control block for the stats thread and releases the 
** memory used to hold the arrays for current and previous iteration counts
**
** Input Parameters: pControlThread - point to the control thread control block
**                                    This is set to NULL when the control block is freed
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int  cphStatsThreadFree(CPH_STATSTHREAD **ppStatsThread) {
    CPH_TRACE *pTrc;

    if (NULL != *ppStatsThread) {
        pTrc = (*ppStatsThread)->pControlThread->pConfig->pTrc;
#define CPH_ROUTINE "cphStatsThreadFree"
        CPHTRACEENTRY(pTrc, CPH_ROUTINE );


        if (NULL != (*ppStatsThread)->prev) free((*ppStatsThread)->prev);
        if (NULL != (*ppStatsThread)->curr) free((*ppStatsThread)->curr);
        free(*ppStatsThread);
        *ppStatsThread = NULL;

        CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

        return(CPHTRUE);
    }
    return(CPHFALSE);
}


/*
** Method: cphTimerThreadIni
**
** This method creates and initialises the control block for the timer thread
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Output parameters: ppTimerThread - a pointer to a pointer to the new control block
**
*/
void cphTimerThreadIni(CPH_TIMERTHREAD **ppTimerThread, CPH_CONTROLTHREAD *pControlThread) {
    CPH_TRACE *pTrc;
    CPH_TIMERTHREAD *pTimerThread;

    pTrc = pControlThread->pConfig->pTrc;
#define CPH_ROUTINE "cphTimerThreadIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    pTimerThread = malloc(sizeof(CPH_TIMERTHREAD));
    if (NULL != pTimerThread) {
        pTimerThread->pControlThread = pControlThread;
        pTimerThread->runLength = 0;
    }
    *ppTimerThread = pTimerThread;

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

}

/*
** Method: cphTimerThreadFree
**
** 
**
** Input Parameters:
**
** Output parameters:
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int  cphTimerThreadFree(CPH_TIMERTHREAD **ppTimerThread) {
    CPH_TRACE *pTrc;
    if (NULL != *ppTimerThread) {

        pTrc = (*ppTimerThread)->pControlThread->pConfig->pTrc;
#define CPH_ROUTINE "cphTimerThreadFree"
        CPHTRACEENTRY(pTrc, CPH_ROUTINE );

        free(*ppTimerThread);

        CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

        return(CPHTRUE);
    }
    return(CPHFALSE);
}

/*
** Method: cphControlThreadRun
**
** This is the main driving method for the control thread. 
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Returns: CPHTRUE on successful execution, CPHFALSE otherwise
**
*/
int cphControlThreadRun(CPH_CONTROLTHREAD *pControlThread) {
    int shutdownHandler = CPHFALSE;
    int numWorkers = 0;
    int statsInterval = 0;
    int doFinalSummary;
    int iterations = 0;
    double duration=0.0;
    int status = CPHTRUE;
    char sd[80];

    int threadStackSize;
    CPH_CONFIG *pConfig;
    CPH_TRACE *pTrc;
    CPH_LOG *pLog;
    CPH_LISTITERATOR *pIterator;
    CPH_ARRAYLISTITEM * pItem;
    CPH_WORKERTHREAD *pWorker = NULL;
    CPH_PUBLISHER *pPublisher;
    //CPH_PUBLISHERV6 *pPublisherV6;
    CPH_SUBSCRIBER *pSubscriber;
    //CPH_SUBSCRIBERV6 *pSubscriberV6;
    CPH_DUMMY *pDummy;
    CPH_Sender *pSender;
    CPH_Receiver *pReceiver;
    CPH_PutGet *pPutGet;
    CPH_Requester *pRequester;
    CPH_Responder *pResponder;
#if 0
    CPH_MQTTPUBLISHER *pMQTTPublisher;
    CPH_MQTTSUBSCRIBER *pMQTTSubscriber;
#endif
    CPH_TIME approxEndTime;
    CPH_TIME wtTime;
    CPH_STRINGBUFFER *pSb = NULL;

    pConfig = pControlThread->pConfig;
    pLog = pConfig->pLog;
    pTrc = pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadRun"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    /* Check for any unrecognised command line options and give help text if required */
    cphConfigMarkLoaded(pConfig);

    cphLogPrintLn(pLog, LOGVERBOSE, "controlThread START");

    /* Get the set duration option */
    if (CPHTRUE != cphConfigGetString(pConfig, sd, "sd")) exit(1);
    if (0 == strcmp(sd, "normal"))
        pControlThread->reportTlf = CPHFALSE;
    else if (0 == strcmp(sd, "tlf")) 
        pControlThread->reportTlf = CPHTRUE;
    else
        cphConfigInvalidParameter(pConfig, "-sd must be one of {normal,tlf}");

    if (CPHTRUE != cphConfigGetBoolean(pConfig, &shutdownHandler, "sh")) 
        status = CPHFALSE;
    else {
        if (CPHTRUE == shutdownHandler) signal(SIGINT, cphUtilSigInt);
    }

    if (CPHTRUE != cphConfigGetInt(pConfig, &numWorkers, "nt")) 
        status = CPHFALSE;
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Num worker threads: %d.", numWorkers);
    }

    if (CPHTRUE != cphConfigGetBoolean(pConfig, &doFinalSummary, "su")) 
        status = CPHFALSE;
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Display final summary: %d.", doFinalSummary);
    }

    if (CPHTRUE != cphConfigGetString(pConfig, pControlThread->procId, "id")) 
        status = CPHFALSE;
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Process Id: %s.", pControlThread->procId);
    }

    /* Remember our thread id (for use with trace summary */
    if (cphTraceIsOn(pConfig->pTrc)) pControlThread->controlThreadId = cphUtilGetThreadId();

    /* Start the stats thread if the specified interval is greater than zero */
    if (CPHTRUE != cphConfigGetInt(pConfig, &statsInterval, "ss")) 
        status = CPHFALSE;
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Stats interval: %d.", statsInterval);
        if (statsInterval > 0) cphControlThreadStartStatsThread(pControlThread);
    }
    if (CPHTRUE != cphConfigGetInt(pConfig, &threadStackSize, "ts")) 
        status = CPHFALSE;
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Thread stack size command line option: %d.", threadStackSize);
        if (threadStackSize > 0) 
        {
#ifdef AMQ_UNIX
           CPHTRACEMSG(pTrc, CPH_ROUTINE, "Setting per thread stack allocation to: %d.\n", 1024 * threadStackSize);
           cphThreadSetStackSize(&pControlThread->threadAttr, 1024 * threadStackSize);
#else
           CPHTRACEMSG(pTrc, CPH_ROUTINE, "-ts option is only supported on Unix.\n");
           cphConfigInvalidParameter(pConfig, "-ts is only supported on Unix");
#endif
        }
    }

    /* If the configuration is invalid at this point, print a message and exit before starting any worker threads */
    if ( (status == CPHFALSE) || (CPHTRUE == cphConfigIsInvalid(pConfig)) ) {
        cphLogPrintLn( pConfig->pLog, LOGERROR, "The current configuration is not valid.  Use -h to see available options." );
        status = CPHFALSE;
    }

    if (CPHTRUE == status) {
        if (CPHTRUE == (status = cphControlThreadSetWorkers(pControlThread, numWorkers))) {

            /* Set the approximate start time */
            pControlThread->startTime = cphUtilGetCurrentTimeMillis();

            cphControlThreadStartTimerThread(pControlThread);

            while ( (CPHFALSE == pControlThread->shutdown) && (pControlThread->runningWorkers > 0) ) {
                /* cphUtilSleep (5 * 1000); */
                cphUtilSleep(2000);
                if (CPHTRUE == cphControlCInvoked) cphControlThreadSignalShutdown(pControlThread);
            }

        }
        approxEndTime = cphUtilGetCurrentTimeMillis();
    }

    /* If the configuration is invalid at this point, print a message and exit before starting any worker threads */
    if (status == CPHTRUE && (CPHTRUE == cphConfigIsInvalid(pConfig)) ) {
        cphLogPrintLn( pConfig->pLog, LOGERROR, "The current configuration is not valid. Use -h to see available options." );
        status = CPHFALSE;
    }
    /* If the array of worker threads is empty then something has gone wrong */
    else if (cphArrayListIsEmpty(pControlThread->workers)) {
        cphLogPrintLn( pConfig->pLog, LOGERROR, "No worker threads were created, exiting. Use -h to see available options." );
        status = CPHFALSE;
    }

    if (CPHTRUE == status) {
        /* Wait for the workers to all be in the sENDED state. We need to do this if we have been control-Cd */
        cphControlThreadDoShutdown(pControlThread);

        pIterator = cphArrayListListIterator( pControlThread->workers);
        do {
            pItem = cphListIteratorNext(pIterator);

            /* ********* Item is actually a pointer to a publisher not a workerthread */
            if (mPUBLISHER == pConfig->moduleType) {
                pPublisher = pItem->item;
                pWorker = pPublisher->pWorkerThread;

            }
            else if (mSUBSCRIBER == pConfig->moduleType) {
                pSubscriber = pItem->item;
                pWorker = pSubscriber->pWorkerThread;
            }
#if 0            
            else if (mPUBLISHERV6 == pConfig->moduleType) {
                pPublisherV6 = pItem->item;
                pWorker = pPublisherV6->pWorkerThread;

            }
            else if (mSUBSCRIBERV6 == pConfig->moduleType) {
                pSubscriberV6 = pItem->item;
                pWorker = pSubscriberV6->pWorkerThread;
            }
#endif            
            else if (mDUMMY == pConfig->moduleType) {
                pDummy = pItem->item;
                pWorker = pDummy->pWorkerThread;
            }
            else if (mSender == pConfig->moduleType) {
                pSender = pItem->item;
                pWorker = pSender->pWorkerThread;
            }
            else if (mReceiver == pConfig->moduleType) {
                pReceiver = pItem->item;
                pWorker = pReceiver->pWorkerThread;

			}
            else if (mPutGet == pConfig->moduleType) {
                pPutGet = pItem->item;
                pWorker = pPutGet->pWorkerThread;

			}
            else if (mRequester == pConfig->moduleType) 
            {
                pRequester = pItem->item;
                pWorker = pRequester->pWorkerThread;

            } else if (mResponder == pConfig->moduleType) 
            {
                pResponder = pItem->item;
                pWorker = pResponder->pWorkerThread;
            }
#if 0
            else if (mMQTTPublisher == pConfig->moduleType) {
                pMQTTPublisher = pItem->item;
                pWorker = pMQTTPublisher->pWorkerThread;
            }
            else if (mMQTTSubscriber == pConfig->moduleType) {
                pMQTTSubscriber = pItem->item;
                pWorker = pMQTTSubscriber->pWorkerThread;

            }
#endif
            wtTime = cphWorkerThreadGetEndTime(pWorker);

            if (cphUtilTimeCompare(wtTime, pControlThread->endTime) > 0) {
                pControlThread->endTime=wtTime;
            }

            wtTime = cphWorkerThreadGetStartTime(pWorker);
            /* if ( wtTime!=0 && wtTime < startTime ) {  */
            if ( (CPHTRUE != cphUtilTimeIsZero(wtTime)) && (cphUtilTimeCompare(wtTime, pControlThread->startTime) < 0) ) {
                pControlThread->startTime=wtTime;
            }

        } while (cphListIteratorHasNext(pIterator));
        cphListIteratorFree(&pIterator);

        /* Override the above for different timing modes */
        if (CPHTRUE == pControlThread->reportTlf) {
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "(TLF) Using THREAD start time");
            pControlThread->startTime = pControlThread->threadStartTime;
        }

        /* If no one has a valid end time, use our rough value */
        if ( cphUtilTimeIsZero(pControlThread->endTime)) {
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "Using approximate value of end time");
            pControlThread->endTime = approxEndTime;
        }

        if (CPHTRUE == doFinalSummary) {
            duration = ( cphUtilGetTimeDifference(pControlThread->endTime, pControlThread->startTime))/1000.0;

            pIterator = cphArrayListListIterator( pControlThread->workers);
            do {
                pItem = cphListIteratorNext(pIterator);

                /* ********* Item is actually a pointer to a publisher not a workerthread */
                if (mPUBLISHER == pConfig->moduleType) {
                    pPublisher = pItem->item;
                    iterations += pPublisher->pWorkerThread->iterations;
                }
                else if (mSUBSCRIBER == pConfig->moduleType) {
                    pSubscriber = pItem->item;
                    iterations += pSubscriber->pWorkerThread->iterations;
                }
#if 0                
                else if (mPUBLISHERV6 == pConfig->moduleType) {
                    pPublisherV6 = pItem->item;
                    iterations += pPublisherV6->pWorkerThread->iterations;
                }
                else if (mSUBSCRIBERV6 == pConfig->moduleType) {
                    pSubscriberV6 = pItem->item;
                    iterations += pSubscriberV6->pWorkerThread->iterations;
                }
#endif                
                else if (mDUMMY == pConfig->moduleType) {
                    pDummy = pItem->item;
                    iterations += pDummy->pWorkerThread->iterations;
                }
                else if (mSender == pConfig->moduleType) {
                    pSender = pItem->item;
                    iterations += pSender->pWorkerThread->iterations;
                }
                else if (mReceiver == pConfig->moduleType) {
                    pReceiver = pItem->item;
                    iterations += pReceiver->pWorkerThread->iterations;
                }
                else if (mPutGet == pConfig->moduleType) {
                    pPutGet = pItem->item;
                    iterations += pPutGet->pWorkerThread->iterations;
                }
                else if (mRequester == pConfig->moduleType) {
                    pRequester = pItem->item;
                    iterations += pRequester->pWorkerThread->iterations;
                }
                else if (mResponder == pConfig->moduleType) {
                    pResponder = pItem->item;
                    iterations += pResponder->pWorkerThread->iterations;
                }
#if 0
                else if (mMQTTPublisher == pConfig->moduleType) {
                    pMQTTPublisher = pItem->item;
                    iterations += pMQTTPublisher->pWorkerThread->iterations;
                }
                else if (mMQTTSubscriber == pConfig->moduleType) {
                    pMQTTSubscriber = pItem->item;
                    iterations += pMQTTSubscriber->pWorkerThread->iterations;
                }
#endif
            } while (cphListIteratorHasNext(pIterator));
            cphListIteratorFree(&pIterator);

            cphStringBufferIni(&pSb);

            cphStringBufferAppend(pSb, "totalIterations=");
            cphStringBufferAppendInt(pSb, iterations);
            cphStringBufferAppend(pSb, ",totalSeconds=");
            cphStringBufferAppendDouble(pSb, duration);
            cphStringBufferAppend(pSb, ",avgRate=");
            cphStringBufferAppendDouble(pSb, (double) iterations / duration );

            /*  TODO iterations is only updated by stats thread.  Need to calculate it again HERE. */
            cphLogPrintLn(pLog, LOGWARNING, cphStringBufferToString(pSb) );

            cphStringBufferFree(&pSb);
        }

    }
    cphLogPrintLn(pLog, LOGVERBOSE, "controlThread STOP");

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(CPHTRUE);

}

/*
** Method: cphControlThreadStartStatsThread
**
** This method creates a control block for the stats thread and then starts the thread
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphControlThreadStartStatsThread(CPH_CONTROLTHREAD *pControlThread) {
    CPH_TRACE *pTrc;
    CPH_THREADID statsThreadId;
    CPH_STATSTHREAD *pStatsThread;

    pTrc = pControlThread->pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadStartStatsThread"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    cphStatsThreadIni(&pStatsThread, pControlThread);
#ifdef AMQ_UNIX
    cphThreadStart(&statsThreadId, cphControlThreadStatsThreadRun, pStatsThread, &pControlThread->threadAttr);
#else
    cphThreadStart(&statsThreadId, cphControlThreadStatsThreadRun, pStatsThread, NULL);
#endif

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
** Method: cphControlThreadStartTimerThread
**
** This method creates a control block for the timer thread and then starts the thread
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphControlThreadStartTimerThread(CPH_CONTROLTHREAD *pControlThread) {
    CPH_TRACE *pTrc;
    CPH_THREADID timerThreadId;
    CPH_TIMERTHREAD *pTimerThread;

    pTrc = pControlThread->pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadStartTimerThread"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    cphTimerThreadIni(&pTimerThread, pControlThread);

    if (CPHTRUE != cphConfigGetInt(pControlThread->pConfig, &pTimerThread->runLength, "rl")) exit(1);
    CPHTRACEMSG(pTrc, CPH_ROUTINE, "Run length: %d.", pTimerThread->runLength);

    if (pTimerThread->runLength > 0) 
#ifdef AMQ_UNIX
        cphThreadStart(&timerThreadId, cphControlThreadTimerThreadRun, pTimerThread, &pControlThread->threadAttr);
#else
        cphThreadStart(&timerThreadId, cphControlThreadTimerThreadRun, pTimerThread, NULL);
#endif

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
** Method: cphControlThreadStatsGetValues
**
** This method updates the iteration count array for each worker thread. It first remembers the current array
** as the previous array (freeing up the original previous array if it had been set) and then retrieves the number
** of iterations for each worker thread, storing them in the curr array.
**
** Input Parameters: pStatsThread - pointer to the stats thread control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphControlThreadStatsGetValues(CPH_STATSTHREAD *pStatsThread) {

    CPH_CONTROLTHREAD *pControlThread;
    CPH_CONFIG *pConfig;
    CPH_TRACE *pTrc;
    CPH_LISTITERATOR *pIterator;
    CPH_ARRAYLISTITEM * pItem;
    CPH_WORKERTHREAD *pWorker=NULL;
    CPH_PUBLISHER *pPublisher;
    //CPH_PUBLISHER *pPublisherV6;
    CPH_SUBSCRIBER *pSubscriber;
    //CPH_SUBSCRIBER *pSubscriberV6;

    CPH_DUMMY *pDummy;

    CPH_Sender *pSender;
    CPH_Receiver *pReceiver;
    CPH_PutGet *pPutGet;

    CPH_Requester *pRequester;
    CPH_Responder *pResponder;
#if 0
    CPH_MQTTPUBLISHER *pMQTTPublisher;
    CPH_MQTTSUBSCRIBER *pMQTTSubscriber;
#endif

    int i = 0;
    int nEntries;

    pControlThread = pStatsThread->pControlThread;
    pConfig = pControlThread->pConfig;
    pTrc = pControlThread->pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadStatsGetValues"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );


    if (NULL != pStatsThread->prev) 
    {
        free(pStatsThread->prev);
        pStatsThread->prev = NULL;
        pStatsThread->prevLength = 0;
    }

    if (NULL != pStatsThread->curr) 
    {
        pStatsThread->prev = pStatsThread->curr;
        pStatsThread->prevLength = pStatsThread->currLength;
    }
    nEntries = cphArrayListSize(pControlThread->workers);
    pStatsThread->curr = malloc(sizeof(int) * nEntries);
    pStatsThread->currLength = nEntries;

    if (CPHFALSE == cphArrayListIsEmpty(pControlThread->workers)) {
        pIterator = cphArrayListListIterator( pControlThread->workers);
        do {
            pItem = cphListIteratorNext(pIterator);

            /* ********* Item is actually a pointer to a publisher not a workerthread */
            if (mPUBLISHER == pConfig->moduleType) {
                pPublisher = pItem->item;
                pWorker = pPublisher->pWorkerThread;
            }
            else if (mSUBSCRIBER == pConfig->moduleType) {
                pSubscriber = pItem->item;
                pWorker = pSubscriber->pWorkerThread;
            }
#if 0            
            else if (mPUBLISHERV6 == pConfig->moduleType) {
                pPublisherV6 = pItem->item;
                pWorker = pPublisherV6->pWorkerThread;
            }
            else if (mSUBSCRIBERV6 == pConfig->moduleType) {
                pSubscriberV6 = pItem->item;
                pWorker = pSubscriberV6->pWorkerThread;
            }
#endif            
            else if (mDUMMY == pConfig->moduleType) {
                pDummy = pItem->item;
                pWorker = pDummy->pWorkerThread;
            }
            else if (mSender == pConfig->moduleType) {
                pSender = pItem->item;
                pWorker = pSender->pWorkerThread;
            }
            else if (mReceiver == pConfig->moduleType) {
                pReceiver = pItem->item;
                pWorker = pReceiver->pWorkerThread;
            }
            else if (mPutGet == pConfig->moduleType) {
                pPutGet = pItem->item;
                pWorker = pPutGet->pWorkerThread;
            }
            else if (mRequester == pConfig->moduleType) {
                pRequester = pItem->item;
                pWorker = pRequester->pWorkerThread;
            }
            else if (mResponder == pConfig->moduleType) {
                pResponder = pItem->item;
                pWorker = pResponder->pWorkerThread;
            }
#if 0             
            else if (mMQTTPublisher == pConfig->moduleType) {
                pMQTTPublisher = pItem->item;
                pWorker = pMQTTPublisher->pWorkerThread;
            }
            else if (mMQTTSubscriber == pConfig->moduleType) {
                pMQTTSubscriber = pItem->item;
                pWorker = pMQTTSubscriber->pWorkerThread;
            }
#endif
            /* JS! cphWorkerThreadGetIterations should lock the iteration count */
            if(pWorker != NULL)
            {
                pStatsThread->curr[i++] = cphWorkerThreadGetIterations(pWorker);
            }
            else
            {
                // We've somehow not got a pWorker so something has gone terribly wrong
                return CPHFALSE;
            }


        } while (cphListIteratorHasNext(pIterator));
        cphListIteratorFree(&pIterator);
    }

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
 * Create a TCP socket and perform a blocking connect to the destination IP and port provided
 * and send the buffer provided to the destination server
 *
 * @param[in]	dstIP  				=	the IPv4 address of the server to connect to
 * @param[in]	dstPort				= 	the port number that the server is listening on
 * @param[in]	buf					=   the buffer to send
 * @param[in]	buflen				=   the length of the buffer to send
 *
 * @return 0 = OK, anything else is an error
 */
int basicConnectAndSend(char *dstIP, uint16_t dstPort, char *buf, int buflen) {
    int rc = 0;

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1) {
		return 10;
	}

	struct hostent *entry;
	struct sockaddr_in server;

	if( (entry = gethostbyname(dstIP)) == NULL ) {
		close(fd);
		return 11;
	}

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	memcpy(&server.sin_addr.s_addr, entry->h_addr_list[0], entry->h_length);
	server.sin_port = htons(dstPort);

	/* Connect retry loop */
	int retryCount = 0;
	do {
		if (connect(fd, (struct sockaddr*)&server, sizeof(server)) != 0) {
			rc = 12;
		} else {
			rc = 0;
			break;  // successfully connected
		}

		retryCount++;
	} while(retryCount < 5);

	if(rc) {
		close(fd);
		return rc;
	}

	/* Send retry loop */
	int bytesSent = 0;
	int remainingLen = buflen;
	char *pBuf = buf;

	while(remainingLen > 0) {
		bytesSent = send(fd, pBuf, remainingLen, 0);
		if(bytesSent == -1) {
			rc = 13;
			break;
		}

		pBuf += bytesSent;
		remainingLen -= bytesSent;
	}

	close(fd);
	return rc;
}

/*
** Method: cphControlThreadStatsThreadRun
** 
** This is the routine that is started in the stats thread. For as long as the control 
** thread is running, it sleeps for the prescribed stats interval and then displays a 
** one line summary of the current statistics.
**
** Input Parameters: lpParam - a pointer to the stats thread control block (cast as void)
**
** Returns:
**    Windows - CPHTRUE on successful exection, CPHFALSE otherwise
**    Linux   - NULL
**
*/
CPH_RUN cphControlThreadStatsThreadRun(void * lpParam ) {

    CPH_CONFIG *pConfig;
    CPH_TRACE *pTrc;
    CPH_STATSTHREAD *pStatsThread;
    CPH_CONTROLTHREAD *pControlThread;
    CPH_LOG *pLog;
    CPH_STRINGBUFFER *pSb = NULL;

    int statsInterval;
    int do_perThread;
    char do_id[80];

    unsigned int total;
    int j, diff, shortest;

    pStatsThread = lpParam;
    pControlThread = pStatsThread->pControlThread;
    pConfig = pControlThread->pConfig;
    pTrc = pConfig->pTrc;
    pLog = pConfig->pLog;

#define CPH_ROUTINE "cphControlThreadStatsThreadRun"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (CPHTRUE != cphConfigGetInt(pControlThread->pConfig, &statsInterval, "ss")) return(CPHFALSE);
    CPHTRACEMSG(pTrc, CPH_ROUTINE, "Stats interval: %d.", statsInterval);

    if (CPHTRUE != cphConfigGetBoolean(pControlThread->pConfig, &do_perThread, "sp")) return(CPHFALSE);
    CPHTRACEMSG(pTrc, CPH_ROUTINE, "Display per-thread performance data %d.", do_perThread);

    if (CPHTRUE != cphConfigGetString(pControlThread->pConfig, do_id, "id")) return(CPHFALSE);
    CPHTRACEMSG(pTrc, CPH_ROUTINE, "Process identifier %s.", do_id);


    /* Remember the stats thread id (for use with trace summary */
    if (cphTraceIsOn(pConfig->pTrc)) pControlThread->statsThreadId = cphUtilGetThreadId();

    cphStringBufferIni(&pSb);

    char *graphiteHost = getenv("GraphiteIP");
    char *metricPath = getenv("GraphiteMetricRoot");
    if(!metricPath)
    	metricPath = "default";

    while (CPHFALSE == (pControlThread->shutdown)) {

        cphStringBufferSetLength(pSb, 0);

        cphUtilSleep(statsInterval * 1000);

        if (0 < strlen(do_id)) {
            cphStringBufferAppend(pSb, "id=");
            cphStringBufferAppend(pSb, do_id);
            cphStringBufferAppend(pSb, ",");
        }

        cphControlThreadStatsGetValues(pStatsThread);
        total=0;

        /* comment these out to avoid printing per-thread data */
        if ( CPHTRUE == do_perThread) cphStringBufferAppend(pSb, " (");
        shortest = pStatsThread->currLength<pStatsThread->prevLength?pStatsThread->currLength:pStatsThread->prevLength;
        for (j = 0; j < shortest; j++) {
            diff = pStatsThread->curr[j] - pStatsThread->prev[j];
            if (CPHTRUE == do_perThread) {
                cphStringBufferAppendInt(pSb, diff);
                cphStringBufferAppend(pSb, "\t");
            }
            total += diff;
        }
        /* Add on new threads */
        for (j = shortest; j<pStatsThread->currLength; j++ ) {
            if ( CPHTRUE == do_perThread) {
                cphStringBufferAppendInt(pSb, pStatsThread->curr[j]);
                cphStringBufferAppend(pSb, "\t");
            }
            total += pStatsThread->curr[j];
        }
        if ( CPHTRUE == do_perThread) cphStringBufferAppend(pSb, ") ");
        cphStringBufferAppend(pSb, "rate=");
        cphStringBufferAppendDouble(pSb, (double) total / statsInterval );
        cphStringBufferAppend(pSb, ",threads=");
        cphStringBufferAppendInt(pSb, pControlThread->runningWorkers);

        cphLogPrintLn(pLog, LOGINFO, cphStringBufferToString(pSb));

        if(graphiteHost) {
			int timeSecs = time(NULL);
			char value[64] = {0};
			snprintf(value, 64, "%f", (double) total / statsInterval);
			char metric[512] = {0};
			snprintf(metric, 512, "%s.mqbench.msgrate", metricPath);
			char gbuf[1024] = {0};
			snprintf(gbuf, 1024, "%s %s %d\n", metric, value, timeSecs);
			basicConnectAndSend(graphiteHost, 2003, gbuf, strlen(gbuf));
        }

    }

    cphStringBufferFree(&pSb);

    /* We can delete our own control structure now */
    cphStatsThreadFree(&pStatsThread);

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    /* Return the right type of variable for a thread entry function 
    This return value is not used by cph but is set in order to 
    conform to the o/s specific thread dispatch api call */
    return CPH_RUNRETURN;
}

/*
** Method: cphControlThreadTimerThreadRun
**
** This is the routine that is started in the timer thread. This thread is used to control the run length of
** cph, as specified by the "rl" command line option
**
** Input Parameters: lpParam - a pointer to the timer thread control block (cast as void)
**
** Returns:
**    Windows - CPHTRUE on successful exection, CPHFALSE otherwise
**    Linux   - NULL
**
*/
CPH_RUN cphControlThreadTimerThreadRun(void * lpParam ) {

    CPH_TRACE *pTrc;
    CPH_CONFIG *pConfig;
    CPH_TIMERTHREAD *pTimerThread;
    CPH_CONTROLTHREAD *pControlThread;
    CPH_LOG *pLog;

    pTimerThread = lpParam;
    pControlThread = pTimerThread->pControlThread;
    pConfig = pControlThread->pConfig;
    pLog = pConfig->pLog;
    pTrc = pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadTimerThreadRun"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    /* Remember the timer thread id (for use with trace summary */
    if (cphTraceIsOn(pConfig->pTrc)) pControlThread->timerThreadId = cphUtilGetThreadId();

    /* JS: Why is there a loop here? */
    while (!pControlThread->shutdown) {
        cphUtilSleep(pTimerThread->runLength * 1000);
        cphLogPrintLn(pLog, LOGVERBOSE, "Timer : Runlength expired" );
        cphControlThreadSignalShutdown(pControlThread);
    }

    /* We can delete our own control structure now */
    cphTimerThreadFree(&pTimerThread);

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    /* Return the right type of variable for a thread entry function 
    This return value is not used by cph but is set in order to 
    conform to the o/s specific thread dispatch api call */
    return CPH_RUNRETURN;
}


/*
** Method: cphControlThreadSetWorkers
**
** This method is used to request an additional number of worker threads. If the new number of threads is more
** than the existing number, then the additional amount are added by calling cphControlThreadAddWorkers. The latter method 
** then starts all threads it knows about which are not in the running state. 
**
** NB: This method is only currently called once as we do not yet support increasing the number of threads on the fly
**
** Input Parameters: pControlThread - pointer to the control thread control block
**                   n - the number of threads to add
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphControlThreadSetWorkers(CPH_CONTROLTHREAD *pControlThread, int number) {

    CPH_TRACE *pTrc;
    int status =  CPHTRUE;
    int initial = cphArrayListSize(pControlThread->workers);

    pTrc = pControlThread->pConfig->pTrc;
#define CPH_ROUTINE "cphControlThreadSetWorkers"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if ( number > initial ) {
        /* Create some more workers */
        status = cphControlThreadAddWorkers(pControlThread, number-initial );
    } else {
        /* remove workers */
        cphLogPrintLn(pControlThread->pConfig->pLog, LOGERROR, "thread reduction not yet implemented" );
    }
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
    return(status);
}


/*
** Method: cphControlThreadAddWorkers
**
** This method intialises and stores a specified number of worker thread control blocks and then 
** calls cphControlThreadStartWorkers to start all of the worker threads.
**
** Input Parameters: pControlThread - point to the control thread control block
**                   number - the number of thread control blocks to prepare
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphControlThreadAddWorkers(CPH_CONTROLTHREAD *pControlThread, int number ) {
    int i, status;

    CPH_TRACE *pTrc;
    CPH_WORKERTHREAD *pWorker = NULL;
    CPH_PUBLISHER *pPublisher = NULL;
    CPH_SUBSCRIBER *pSubscriber = NULL;
#if 0
    CPH_PUBLISHERV6 *pPublisherV6 = NULL;
    CPH_SUBSCRIBERV6 *pSubscriberV6 = NULL;
#endif
    CPH_DUMMY *pDummy = NULL;

    CPH_Sender *pSender = NULL;
    CPH_Receiver *pReceiver = NULL;
    CPH_PutGet *pPutGet = NULL;

    CPH_Requester *pRequester = NULL;
    CPH_Responder *pResponder = NULL;
#if 0
    CPH_MQTTPUBLISHER *pMQTTPublisher = NULL;
    CPH_MQTTSUBSCRIBER *pMQTTSubscriber = NULL;
#endif

    pTrc = pControlThread->pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadAddWorkers"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    for (i=0; i < number; i++) {

        cphWorkerThreadIni(&pWorker, pControlThread);
        pControlThread->nextThreadNum++;

        if (mPUBLISHER == pControlThread->pConfig->moduleType) {

            if (NULL != pWorker) {
                cphPublisherIni(&pPublisher, pWorker);
            }

            if (NULL != pPublisher) {
                cphArrayListAdd(pControlThread->workers, pPublisher);
            }

        }
        else if (mSUBSCRIBER == pControlThread->pConfig->moduleType) {

            if (NULL != pWorker) {
                cphSubscriberIni(&pSubscriber, pWorker);
            }

            if (NULL != pSubscriber) {
                cphArrayListAdd(pControlThread->workers, pSubscriber);
            }

        }
#if 0        
        else if (mPUBLISHERV6 == pControlThread->pConfig->moduleType) {

            if (NULL != pWorker) {
                cphPublisherV6Ini(&pPublisherV6, pWorker);
            }

            if (NULL != pPublisherV6) {
                cphArrayListAdd(pControlThread->workers, pPublisherV6);
            }

        }

        else if (mSUBSCRIBERV6 == pControlThread->pConfig->moduleType) {

            if (NULL != pWorker) {
                cphSubscriberV6Ini(&pSubscriberV6, pWorker);
            }

            if (NULL != pSubscriberV6) {
                cphArrayListAdd(pControlThread->workers, pSubscriberV6);
            }
        }
#endif
        else if (mDUMMY == pControlThread->pConfig->moduleType) {

            if (NULL != pWorker) {
                cphDummyIni(&pDummy, pWorker);
            }

            if (NULL != pDummy) {
                cphArrayListAdd(pControlThread->workers, pDummy);
            }

        }

        else if (mSender == pControlThread->pConfig->moduleType) {

            if (NULL != pWorker) {
                cphSenderIni(&pSender, pWorker);
            }

            if (NULL != pSender) {
                cphArrayListAdd(pControlThread->workers, pSender);
            }

        }
        else if (mReceiver == pControlThread->pConfig->moduleType) {

            if (NULL != pWorker) {
                cphReceiverIni(&pReceiver, pWorker);
            }

            if (NULL != pReceiver) {
                cphArrayListAdd(pControlThread->workers, pReceiver);
            }

        }
        else if (mPutGet == pControlThread->pConfig->moduleType) {

            if (NULL != pWorker) {
                cphPutGetIni(&pPutGet, pWorker);
            }

            if (NULL != pPutGet) {
                cphArrayListAdd(pControlThread->workers, pPutGet);
            }

        }
        else if (mRequester == pControlThread->pConfig->moduleType) {

            if (NULL != pWorker) {
                cphRequesterIni(&pRequester, pWorker);
            }

            if (NULL != pRequester) {
                cphArrayListAdd(pControlThread->workers, pRequester);
            }
        }
        else if (mResponder == pControlThread->pConfig->moduleType) {

            printf("cphControlThreadAddWorkers: RESPONDER INI CALL\n");

            if (NULL != pWorker) {
                cphResponderIni(&pResponder, pWorker);
            }

            if (NULL != pResponder) {
                cphArrayListAdd(pControlThread->workers, pResponder);
            }
        }
#if 0
        else if (mMQTTPublisher == pControlThread->pConfig->moduleType) {

            if (NULL != pWorker) {
                cphMQTTPublisherIni(&pMQTTPublisher, pWorker);
            }

            if (NULL != pMQTTPublisher) {
                cphArrayListAdd(pControlThread->workers, pMQTTPublisher);
            }
        }
        else if (mMQTTSubscriber == pControlThread->pConfig->moduleType) {

            if (NULL != pWorker) {
                cphMQTTSubscriberIni(&pMQTTSubscriber, pWorker);
            }

            if (NULL != pMQTTSubscriber) {
                cphArrayListAdd(pControlThread->workers, pMQTTSubscriber);
            }
        }
#endif
    }

    status = cphControlThreadStartWorkers(pControlThread);

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(status);
    /*
    Create array of this class
    */			
    /*
    synchronized ( workers ) {
    for (int i = 0; i < number; i++) {

    WorkerThread worker = WorkerThread.newInstance();
    synchronized ( workers ) {
    // synch to avoid inferior JVMs throwing ConcurrentModificationException during startup
    workers.add( worker );
    }

    } // End for threads
    }

    return startWorkers();
    */
}

/*
** Method: cphControlThreadStartWorkers
**
** This method starts the worker threads whose control blocks have been initialised and inserted into the 
** "workers" array list in the control block. For each worker thread that is started, the method polls for the
** number of times specified by the "wt" parameter to check that the thread has started. On each attempt to poll
** for the thread the method will sleep the number of seconds specified by the "wi" parameter. This effectively
** controls the pause between starting multiple threads.
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphControlThreadStartWorkers(CPH_CONTROLTHREAD *pControlThread) {
    int workerInc;
    int wtCount;
    int count;
    int state;
    int threadRunning;
    char errorString[512];
    int status = CPHTRUE;
    CPH_LISTITERATOR *pIterator = NULL;
    CPH_ARRAYLISTITEM * pItem;
    CPH_WORKERTHREAD *pWorker = NULL;
    CPH_PUBLISHER *pPublisher = NULL;
    CPH_SUBSCRIBER *pSubscriber = NULL;
#if 0
    CPH_PUBLISHERV6 *pPublisherV6;
    CPH_SUBSCRIBERV6 *pSubscriberV6;
#endif    
    CPH_DUMMY *pDummy = NULL;
    CPH_Sender *pSender = NULL;
    CPH_Receiver *pReceiver= NULL;
    CPH_PutGet *pPutGet = NULL;
    CPH_Requester *pRequester = NULL;
    CPH_Responder *pResponder = NULL;
#if 0
    CPH_MQTTPUBLISHER *pMQTTPublisher;
    CPH_MQTTSUBSCRIBER *pMQTTSubscriber;
#endif
    CPH_THREADID threadId;
    CPH_CONFIG *pConfig;
    CPH_TRACE *pTrc;
    CPH_THREAD_ATTR *pAttr;

    pConfig = pControlThread->pConfig;
    pTrc = pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadStartWorkers"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (CPHTRUE != cphConfigGetInt(pConfig, &workerInc, "wi")) 
        status = CPHFALSE;
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Worker Thread start interval is: %d.", workerInc);
    }

    if (CPHTRUE != cphConfigGetInt(pConfig, &wtCount, "wt")) 
        status = CPHFALSE;
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Number of times to poll thread on start up is: %d.", wtCount);
    }

    if (CPHTRUE == cphArrayListIsEmpty(pControlThread->workers)) {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Array List is empty.");
        status = CPHFALSE;
    }

    if (CPHTRUE != cphConfigGetInt(pConfig, &wtCount, "wt")) 
        status = CPHFALSE;
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "Number of times to poll thread on start up is: %d.", wtCount);
    }

    if (CPHTRUE == status) {
        pIterator = cphArrayListListIterator( pControlThread->workers);
        do  {
            pItem = cphListIteratorNext(pIterator);

            /* ********* Item is actually a pointer to a publisher not a workerthread */
            if (mPUBLISHER == pConfig->moduleType) {
                pPublisher = pItem->item;
                pWorker = pPublisher->pWorkerThread;
            }
            else if (mSUBSCRIBER == pConfig->moduleType) {
                pSubscriber = pItem->item;
                pWorker = pSubscriber->pWorkerThread;
            }
#if 0            
            else if (mPUBLISHERV6 == pConfig->moduleType) {
                pPublisherV6 = pItem->item;
                pWorker = pPublisherV6->pWorkerThread;
            }
            else if (mSUBSCRIBERV6 == pConfig->moduleType) {
                pSubscriberV6 = pItem->item;
                pWorker = pSubscriberV6->pWorkerThread;
            }
#endif            
            else if (mDUMMY == pConfig->moduleType) {
                pDummy = pItem->item;
                pWorker = pDummy->pWorkerThread;
            }
            else if (mSender == pConfig->moduleType) {
                pSender = pItem->item;
                pWorker = pSender->pWorkerThread;
            }
            else if (mReceiver == pConfig->moduleType) {
                pReceiver = pItem->item;
                pWorker = pReceiver->pWorkerThread;
            }
            else if (mPutGet == pConfig->moduleType) {
                pPutGet = pItem->item;
                pWorker = pPutGet->pWorkerThread;
            }
            else if (mRequester == pConfig->moduleType) {
                pRequester = pItem->item;
                pWorker = pRequester->pWorkerThread;
            }
            else if (mResponder == pConfig->moduleType) {
                pResponder = pItem->item;
                pWorker = pResponder->pWorkerThread;
            }
#if 0            
            else if (mMQTTPublisher == pConfig->moduleType) {
                pMQTTPublisher = pItem->item;
                pWorker = pMQTTPublisher->pWorkerThread;
            }
            else if (mMQTTSubscriber == pConfig->moduleType) {
                pMQTTSubscriber = pItem->item;
                pWorker = pMQTTSubscriber->pWorkerThread;
            }
#endif
            /* Skip any already running threads */
            if ( (cphWorkerThreadGetState(pWorker) & sCREATED) == 0) continue;

#ifdef AMQ_UNIX
            pAttr = &pControlThread->threadAttr;
#else
            pAttr = NULL;
#endif
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "Starting worker thread.");
            if (mPUBLISHER == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pPublisher, pAttr);
            else if (mSUBSCRIBER == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pSubscriber, pAttr);
#if 0
            else if (mPUBLISHERV6 == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pPublisherV6, pAttr);
            else if (mSUBSCRIBERV6 == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pSubscriberV6, pAttr);
#endif            
            else if (mDUMMY == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pDummy, pAttr);
            else if (mSender == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pSender,pAttr);
            else if (mReceiver == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pReceiver,pAttr);
            else if (mPutGet == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pPutGet,pAttr);
            else if (mRequester == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pRequester,pAttr);
            else if (mResponder == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pResponder,pAttr);
#if 0
            else if (mMQTTPublisher == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pMQTTPublisher, pAttr);
            else if (mMQTTSubscriber == pConfig->moduleType)
                cphThreadStart(&threadId, pWorker->pRunFn, pMQTTSubscriber,pAttr);
#endif

            pWorker->threadId = threadId;

            //if (CPHTRUE != cphConfigGetInt(pConfig, &wtCount, "wt")) 
            //    status = CPHFALSE;
            //else {
            //    CPHTRACEMSG(pTrc, CPH_ROUTINE, "Number of times to poll thread on start up is: %d.", wtCount);
            threadRunning = CPHFALSE;
            count = wtCount;
            while ((threadRunning == CPHFALSE) && (count != 0)) {
                CPHTRACEMSG(pTrc, CPH_ROUTINE, "Count: %d", count--);

                state = cphWorkerThreadGetState(pWorker);

                if ((state & sERROR) != 0) {
                    CPHTRACEMSG(pTrc, CPH_ROUTINE, "   State ERROR set.");
                    status = CPHFALSE;
                    break;
                }

                /* if it's up and running .... check next one */
                if ( (state & sCREATED)==0) {
                    CPHTRACEMSG(pTrc, CPH_ROUTINE, "   State created is not set.");
                    /*  Note, sCONNECTING is ignored (we wait until it is running) */
                    if ((state & sRUNNING) != 0) {
                        CPHTRACEMSG(pTrc, CPH_ROUTINE, "   State running is set.");
                        threadRunning = CPHTRUE;
                        /* Only go onto the next one if we haven't been round at least once to ensure we sleep at least
                        one workerInc interval */
                        if (count != wtCount - 1) continue;
                    } else if ( (state&(sENDING|sENDED)) !=0 ) {
                        CPHTRACEMSG(pTrc, CPH_ROUTINE, "   State ending or ended set.");
                        threadRunning = CPHTRUE;
                        continue;
                    }
                }
                else {
                    CPHTRACEMSG(pTrc, CPH_ROUTINE, "   State is still sCREATED.");
                }

                CPHTRACEMSG(pTrc, CPH_ROUTINE, "Sleeping for wi(%d) ms.", workerInc);
                cphUtilSleep(workerInc);
            }
            //}

            if (CPHFALSE == threadRunning) {
                sprintf(errorString, "%s%s%s", "cphControlThreadStartWorkers ", " : cannot start thread ", cphWorkerThreadGetName(pWorker));
                cphLogPrintLn(pControlThread->pConfig->pLog, LOGERROR, errorString);
                status = CPHFALSE;
                break;
            }

        } while (cphListIteratorHasNext(pIterator));

        cphListIteratorFree(&pIterator);
    }
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return status;
}

/*
** Method: cphControlThreadIncRunners
**
** Increment the number of running workers in the control thread control block
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphControlThreadIncRunners(CPH_CONTROLTHREAD *pControlThread) {
    CPH_TRACE *pTrc;

    pTrc = pControlThread->pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadIncRunners"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    cphSpinLock_Lock(pControlThread->ThreadLock);
    pControlThread->runningWorkers++;
    cphSpinLock_Unlock(pControlThread->ThreadLock);
    CPHTRACEMSG(pTrc, CPH_ROUTINE, "Num of running workers is now: %d.", pControlThread->runningWorkers);

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
** Method: cphControlThreadDecRunners
**
** Decrement the number of running workers in the control thread control block
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphControlThreadDecRunners(CPH_CONTROLTHREAD *pControlThread) {
    CPH_TRACE *pTrc;

    pTrc = pControlThread->pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadDecRunners"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    cphSpinLock_Lock(pControlThread->ThreadLock);
    pControlThread->runningWorkers--;
    cphSpinLock_Unlock(pControlThread->ThreadLock);
    CPHTRACEMSG(pTrc, CPH_ROUTINE, "Num of running workers is now: %d.", pControlThread->runningWorkers);

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
** Method: cphControlThreadSignalShutdown
**
** This routine sets the shutdown variable in the control thread control block to signal that
** cph is to shutdown.
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphControlThreadSignalShutdown(CPH_CONTROLTHREAD *pControlThread) {
    CPH_TRACE *pTrc;

    pTrc = pControlThread->pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadSignalShutdown"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    pControlThread->shutdown = CPHTRUE;

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
** Method: cphControlThreadDoShutdown
**
** This method requests all worker threads to shutdown and waits for the number of seconds
** specified by the "wk" parameter for them to finish. It displays a list each second that
** gives information about the threads that it is still waiting for. The threads are deemed 
** to have finished when they are in the sENDED state.
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphControlThreadDoShutdown(CPH_CONTROLTHREAD *pControlThread) {
    CPH_CONFIG *pConfig;
    CPH_TRACE *pTrc;
    CPH_LISTITERATOR *pIterator;
    CPH_ARRAYLISTITEM * pItem;
    CPH_WORKERTHREAD *pWorker = NULL;
    CPH_PUBLISHER *pPublisher;
    CPH_SUBSCRIBER *pSubscriber;
#if 0
    CPH_PUBLISHERV6 *pPublisherV6;
    CPH_SUBSCRIBERV6 *pSubscriberV6;
#endif
    CPH_DUMMY *pDummy; 

    CPH_Sender *pSender;
    CPH_Receiver *pReceiver;
    CPH_PutGet *pPutGet;
    CPH_Requester *pRequester;
    CPH_Responder *pResponder;
#if 0
    CPH_MQTTPUBLISHER *pMQTTPublisher;
    CPH_MQTTSUBSCRIBER *pMQTTSubscriber;
#endif

    CPH_STRINGBUFFER *pSb;
    CPH_STRINGBUFFER *pSb2;
    int count;

    pConfig = pControlThread->pConfig;
    pTrc = pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadDoShutdown"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    pIterator = cphArrayListListIterator( pControlThread->workers);
    do {
        pItem = cphListIteratorNext(pIterator);

        /* ********* Item is actually a pointer to a publisher not a workerthread */
        if (mPUBLISHER == pConfig->moduleType) {
            pPublisher = pItem->item;
            pWorker = pPublisher->pWorkerThread;
        }
        else if (mSUBSCRIBER == pConfig->moduleType) {
            pSubscriber = pItem->item;
            pWorker = pSubscriber->pWorkerThread;
        }
#if 0        
        else if (mPUBLISHERV6 == pConfig->moduleType) {
            pPublisherV6 = pItem->item;
            pWorker = pPublisherV6->pWorkerThread;
        }
        else if (mSUBSCRIBERV6 == pConfig->moduleType) {
            pSubscriberV6 = pItem->item;
            pWorker = pSubscriberV6->pWorkerThread;
        }
#endif        
        else if (mDUMMY == pConfig->moduleType) {
            pDummy = pItem->item;
            pWorker = pDummy->pWorkerThread;
        }
        else if (mSender == pConfig->moduleType) {
            pSender = pItem->item;
            pWorker = pSender->pWorkerThread;
        }
        else if (mReceiver == pConfig->moduleType) {
            pReceiver = pItem->item;
            pWorker = pReceiver->pWorkerThread;
        }
        else if (mPutGet == pConfig->moduleType) {
            pPutGet = pItem->item;
            pWorker = pPutGet->pWorkerThread;
        }
        else if (mRequester == pConfig->moduleType) {
            pRequester = pItem->item;
            pWorker = pRequester->pWorkerThread;
        }
        else if (mResponder == pConfig->moduleType) {
            pResponder = pItem->item;
            pWorker = pResponder->pWorkerThread;
        }
#if 0
        else if (mMQTTPublisher == pConfig->moduleType) {
            pMQTTPublisher = pItem->item;
            pWorker = pMQTTPublisher->pWorkerThread;
        }
        else if (mMQTTSubscriber == pConfig->moduleType) {
            pMQTTSubscriber = pItem->item;
            pWorker = pMQTTSubscriber->pWorkerThread;
        }
#endif
        cphWorkerThreadSignalShutdown(pWorker);


    } while (cphListIteratorHasNext(pIterator));
    cphListIteratorFree(&pIterator);

    /* Retrieve "wk" - the number of seconds to wait for the worker threads to shutdown */
    if (CPHTRUE != cphConfigGetInt(pConfig, &count, "wk")) {
        cphConfigInvalidParameter(pConfig, "Could not retreive shutdown wait option");
        return(CPHFALSE);
    }

    cphStringBufferIni(&pSb);
    cphStringBufferIni(&pSb2);

    /* Inspect the worker threads remaining every second until "wk" seconds have elapsed */
    while (count-- > 0) {

        pIterator = cphArrayListListIterator( pControlThread->workers);
        cphStringBufferSetLength(pSb, 0);

        /* Build a list of the worker threads still remaining */
        do {
            pItem = cphListIteratorNext(pIterator);

            /* ********* Item is actually a pointer to a publisher not a workerthread */
            if (mPUBLISHER == pConfig->moduleType) {
                pPublisher = pItem->item;
                pWorker = pPublisher->pWorkerThread;
            }
            else if (mSUBSCRIBER == pConfig->moduleType) {
                pSubscriber = pItem->item;
                pWorker = pSubscriber->pWorkerThread;
            }
#if 0            
            else if (mPUBLISHERV6 == pConfig->moduleType) {
                pPublisherV6 = pItem->item;
                pWorker = pPublisherV6->pWorkerThread;
            }
            else if (mSUBSCRIBERV6 == pConfig->moduleType) {
                pSubscriberV6 = pItem->item;
                pWorker = pSubscriberV6->pWorkerThread;
            }
#endif            
            else if (mDUMMY == pConfig->moduleType) {
                pDummy = pItem->item;
                pWorker = pDummy->pWorkerThread;
            }
            else if (mSender == pConfig->moduleType) {
                pSender = pItem->item;
                pWorker = pSender->pWorkerThread;
            }
            else if (mReceiver == pConfig->moduleType) {
                pReceiver = pItem->item;
                pWorker = pReceiver->pWorkerThread;
            }
            else if (mPutGet == pConfig->moduleType) {
                pPutGet = pItem->item;
                pWorker = pPutGet->pWorkerThread;
            }
            else if (mRequester == pConfig->moduleType) {
                pRequester = pItem->item;
                pWorker = pRequester->pWorkerThread;
            }
            else if (mResponder == pConfig->moduleType) {
                pResponder = pItem->item;
                pWorker = pResponder->pWorkerThread;
            }
#if 0
            else if (mMQTTSubscriber == pConfig->moduleType) {
                pMQTTSubscriber = pItem->item;
                pWorker = pMQTTSubscriber->pWorkerThread;
            }
            else if (mMQTTPublisher == pConfig->moduleType) {
                pMQTTPublisher = pItem->item;
                pWorker = pMQTTPublisher->pWorkerThread;
            }
#endif
            if ( (cphWorkerThreadGetState(pWorker) & sENDED)==0 ) {
                cphStringBufferAppend(pSb, cphWorkerThreadGetName(pWorker) );
                cphStringBufferAppend(pSb, " ");
            }

        } while (cphListIteratorHasNext(pIterator));
        cphListIteratorFree(&pIterator);

        if (cphStringBufferGetLength(pSb) > 0) {
            if (count % 10 == 0) {
                cphStringBufferSetLength(pSb2, 0);
                cphStringBufferAppend(pSb2, "cphControlThread : waiting for ( ");
                cphStringBufferAppend(pSb2, cphStringBufferToString(pSb));
                cphStringBufferAppend(pSb2, ")");
                cphLogPrintLn(pConfig->pLog, LOGWARNING, cphStringBufferToString(pSb2));

            }
        } else {
            /* No more threads left, we are finished */
            break;
        }

        /* Sleep for a second */
        cphUtilSleep(1000);
    }

    cphStringBufferFree(&pSb);
    cphStringBufferFree(&pSb2);

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
** Method: cphControlThreadDisplayThreads
**
** Write a summary list of all the threads the control thread has been running to the trace file if 
** trace has been requested
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int  cphControlThreadDisplayThreads(CPH_CONTROLTHREAD *pControlThread) {
    CPH_CONFIG *pConfig;
    CPH_LISTITERATOR *pIterator;
    CPH_ARRAYLISTITEM * pItem;
    CPH_WORKERTHREAD *pWorker=NULL;
    CPH_PUBLISHER *pPublisher;
    CPH_SUBSCRIBER *pSubscriber;
#if 0
    CPH_PUBLISHERV6 *pPublisherV6;
    CPH_SUBSCRIBERV6 *pSubscriberV6;
#endif    
    CPH_DUMMY *pDummy;
    CPH_Sender *pSender;
    CPH_Receiver *pReceiver;
    CPH_PutGet *pPutGet;
    CPH_Requester *pRequester;
    CPH_Responder *pResponder;
#if 0
    CPH_MQTTPUBLISHER *pMQTTPublisher;
    CPH_MQTTSUBSCRIBER *pMQTTSubscriber;
#endif
    CPH_TRACE *pTrc;

    pConfig = pControlThread->pConfig;
    pTrc = pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadDisplayThreads"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    fprintf(pTrc->tFp, "\n\nSUMMARY OF THREAD ACTIVITY.\n");
    fprintf(pTrc->tFp, "===========================\n\n\n");

    fprintf(pTrc->tFp, "ThreadId: %d\t\t\tControl thread.\n", pControlThread->controlThreadId);
    fprintf(pTrc->tFp, "ThreadId: %d\t\t\tStats thread.\n", pControlThread->statsThreadId);
    fprintf(pTrc->tFp, "ThreadId: %d\t\t\tTimer thread.\n\n", pControlThread->timerThreadId);

    if (CPHFALSE == cphArrayListIsEmpty(pControlThread->workers)) {
        pIterator = cphArrayListListIterator( pControlThread->workers);
        do {
            pItem = cphListIteratorNext(pIterator);

            /* ********* Item is actually a pointer to a publisher not a workerthread */
            if (mPUBLISHER == pConfig->moduleType) {
                pPublisher = pItem->item;
                pWorker = pPublisher->pWorkerThread;
            }
            else if (mSUBSCRIBER == pConfig->moduleType) {
                pSubscriber = pItem->item;
                pWorker = pSubscriber->pWorkerThread;
            }
#if 0            
            else if (mPUBLISHERV6 == pConfig->moduleType) {
                pPublisherV6 = pItem->item;
                pWorker = pPublisherV6->pWorkerThread;
            }
            else if (mSUBSCRIBERV6 == pConfig->moduleType) {
                pSubscriberV6 = pItem->item;
                pWorker = pSubscriberV6->pWorkerThread;
            }
#endif            
            else if (mDUMMY == pConfig->moduleType) {
                pDummy = pItem->item;
                pWorker = pDummy->pWorkerThread;
            }
            else if (mSender == pConfig->moduleType) {
                pSender = pItem->item;
                pWorker = pSender->pWorkerThread;
			}
            else if (mReceiver == pConfig->moduleType) {
                pReceiver = pItem->item;
                pWorker = pReceiver->pWorkerThread;
			}
            else if (mPutGet == pConfig->moduleType) {
                pPutGet = pItem->item;
                pWorker = pPutGet->pWorkerThread;
			}
            else if (mRequester == pConfig->moduleType) {
                pRequester = pItem->item;
                pWorker = pRequester->pWorkerThread;
			}
            else if (mResponder == pConfig->moduleType) {
                pResponder = pItem->item;
                pWorker = pResponder->pWorkerThread;
            }
#if 0           
            else if (mMQTTSubscriber == pConfig->moduleType) {
                pMQTTSubscriber = pItem->item;
                pWorker = pMQTTSubscriber->pWorkerThread;
            }
            else if (mMQTTPublisher == pConfig->moduleType) {
                pMQTTPublisher = pItem->item;
                pWorker = pMQTTPublisher->pWorkerThread;
			}
#endif
            if (pWorker != NULL) {
                fprintf(pTrc->tFp, "Thread Id: %d\t\t\tThread Name: %s.\n", (int)pWorker->threadId, pWorker->threadName);
            } else {
                fprintf(pTrc->tFp, "Failed to find thread.\n");
                CPHTRACEEXIT(pTrc, CPH_ROUTINE);
                return CPHFALSE;
            }

        } while (cphListIteratorHasNext(pIterator));
        cphListIteratorFree(&pIterator);
    }

    fprintf(pTrc->tFp, "\n\n");

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
int cphControlThreadIncNextThreadNum(CPH_CONTROLTHREAD *pControlThread) {
cphSpinLock_Lock(pControlThread->nextThreadNumLock);
pControlThread->nextThreadNum++;
cphSpinLock_Unlock(pControlThread->nextThreadNumLock);
return(CPHTRUE);
}
*/

/*
** Method: cphControlThreadGetRunningWorkers
**
** Return the number of running workers from the control thread control block
**
** Input Parameters: pControlThread - point to the control thread control block
**
** Returns: the number of running workers
**
*/
int cphControlThreadGetRunningWorkers(CPH_CONTROLTHREAD *pControlThread) {
    CPH_TRACE *pTrc;

    pTrc = pControlThread->pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadGetRunningWorkers"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(pControlThread->runningWorkers);
}

/*
** Method: cphControlThreadSetThreadStartTime
**
** This method sets the time the control thread started. It is called directly by the cph main
** program. This time is used as the start time when using the time of last fire option (-tlf) which
** includes the setup time in the reported duration. (This time is probably not very significant in the C 
** case.
**
** Input Parameters: pControlThread - pointer to the control thread control block
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphControlThreadSetThreadStartTime(CPH_CONTROLTHREAD *pControlThread) {
    CPH_TRACE *pTrc;

    pTrc = pControlThread->pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadSetThreadStartTime"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    pControlThread->threadStartTime = cphUtilGetCurrentTimeMillis();

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(CPHTRUE);
}

int cphControlThreadFreeWorkers(CPH_CONTROLTHREAD *pControlThread) {
    CPH_CONFIG *pConfig;
    CPH_TRACE *pTrc;
    CPH_LISTITERATOR *pIterator;
    CPH_ARRAYLISTITEM * pItem;

    CPH_PUBLISHER *pPublisher;
    CPH_SUBSCRIBER *pSubscriber;
#if 0
    CPH_PUBLISHERV6 *pPublisherV6;
    CPH_SUBSCRIBERV6 *pSubscriberV6;
#endif    
    CPH_DUMMY *pDummy;

    CPH_Sender *pSender;
    CPH_Receiver *pReceiver;
    CPH_PutGet *pPutGet;
    CPH_Requester *pRequester;
    CPH_Responder *pResponder;
#if 0 
    CPH_MQTTPUBLISHER *pMQTTPublisher;
    CPH_MQTTSUBSCRIBER *pMQTTSubscriber;
#endif
    int status = CPHTRUE;

    pConfig = pControlThread->pConfig;
    pTrc = pConfig->pTrc;

#define CPH_ROUTINE "cphControlThreadFreeWorkers"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    pIterator = cphArrayListListIterator( pControlThread->workers);

    while (cphListIteratorHasNext(pIterator)) {
        pItem = cphListIteratorNext(pIterator);

        /* ********* Item is actually a pointer to a publisher not a workerthread */
        if (mPUBLISHER == pConfig->moduleType) {
            pPublisher = pItem->item;
            cphPublisherFree(&pPublisher);
        }
        else if (mSUBSCRIBER == pConfig->moduleType) {
            pSubscriber = pItem->item;
            cphSubscriberFree(&pSubscriber);
        }
#if 0        
        else if (mPUBLISHERV6 == pConfig->moduleType) {
            pPublisherV6 = pItem->item;
            cphPublisherV6Free(&pPublisherV6);
        }
        else if (mSUBSCRIBERV6 == pConfig->moduleType) {
            pSubscriberV6 = pItem->item;
            cphSubscriberV6Free(&pSubscriberV6);
        }
#endif        
        else if (mDUMMY == pConfig->moduleType) {
            pDummy = pItem->item;
            cphDummyFree(&pDummy);
        }
        else if (mSender == pConfig->moduleType) {
            pSender = pItem->item;
            cphSenderFree(&pSender);
        }
        else if (mReceiver == pConfig->moduleType) {
            pReceiver = pItem->item;
            cphReceiverFree(&pReceiver);
        }
        else if (mPutGet == pConfig->moduleType) {
            pPutGet = pItem->item;
            cphPutGetFree(&pPutGet);
        }
        else if (mRequester == pConfig->moduleType) {
            pRequester = pItem->item;
            cphRequesterFree(&pRequester);
        }
        else if (mResponder == pConfig->moduleType) {
            pResponder = pItem->item;
            cphResponderFree(&pResponder);
        }
#if 0        
        else if (mMQTTPublisher == pConfig->moduleType) {
            pMQTTPublisher = pItem->item;
            cphMQTTPublisherFree(&pMQTTPublisher);
        }
        else if (mMQTTSubscriber == pConfig->moduleType) {
            pMQTTSubscriber = pItem->item;
            cphMQTTSubscriberFree(&pMQTTSubscriber);
        }
#endif        
    }
    cphListIteratorFree(&pIterator);

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(status);
}
