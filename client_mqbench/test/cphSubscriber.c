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
*          07-01-10    Oliver Fenton Add read ahead support (-jy) to MQOPEN call
*/

/* This module is the implementation of the "Subscriber" test suite. This provides a set of functions that are used to subscribe to messages under
** the control of the worker thread, which controls the rate and duration at which messages are consumed. 
*/

#include <string.h>
#include "cphSubscriber.h"

/*
** Method: cphSubscriberIni
**
** Create and initialise the control block for a Subscriber. The worker thread control structure
** is passed in as in input parameter and is stored within the subscriber - which is as close as we
** can get to creating a class derived from the worker thread.
**
** Input Parameters: pWorkerThread - the worker thread control structure
**
** Output parameters: ppSubscriber - a double pointer to the newly created subscriber control structure
**
*/
void cphSubscriberIni(CPH_SUBSCRIBER **ppSubscriber, CPH_WORKERTHREAD *pWorkerThread) {
    CPH_SUBSCRIBER *pSubscriber = NULL;
    CPH_CONFIG *pConfig;

    int  whetherDurable;
    int  unSubscribe;
    int  useRFH2;
    int  useMessageHandle;
    int isReadAhead;
    CPH_CONNECTION *pConnection;
    int timeout;

    MQGMO   gmo = {MQGMO_DEFAULT};   /* get message options           */
    MQBYTE *buffer;

    int status = CPHTRUE;

    CPH_TRACE *pTrc = cphWorkerThreadGetTrace(pWorkerThread);
    pConfig = pWorkerThread->pConfig;

#define CPH_ROUTINE "cphSubscriberIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (NULL != pWorkerThread) {

        cphConnectionIni(&pConnection, pConfig);
        if (NULL == pConnection) {
            cphConfigInvalidParameter(pConfig, "Could not initialise cph connection control block");
            status = CPHFALSE;
        }

        if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetBoolean(pConfig, &whetherDurable, "du"))) {
            cphConfigInvalidParameter(pConfig, "Durable subscription flag cannot be retrieved");
            status = CPHFALSE;
        }
        else {
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "Durable: %d.", whetherDurable);
        }

        if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetBoolean(pConfig, &unSubscribe, "un"))) {
            cphConfigInvalidParameter(pConfig, "Unsubscribe flag cannot be retrieved");
            status = CPHFALSE;
        }
        else {
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "Unsubscribe: %d.", unSubscribe);
        }

        if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetBoolean(pConfig, &useRFH2, "rf"))) {
           cphConfigInvalidParameter(pConfig, "Cannot retrieve RFH2 option");
           status = CPHFALSE;
        }
        else {
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "useRFH2: %d.", useRFH2);
        }

        if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetBoolean(pConfig, &isReadAhead, "jy"))) {
		cphConfigInvalidParameter(pConfig, "Cannot retrieve Read Ahead option");
		status = CPHFALSE;
	}
        else {
		CPHTRACEMSG(pTrc, CPH_ROUTINE, "isReadAhead: %d.", isReadAhead);
        }

        if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetBoolean(pConfig, &useMessageHandle, "mh"))) {
           cphConfigInvalidParameter(pConfig, "Cannot retrieve message handle option");
           status = CPHFALSE;
        }
        else {
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "useMessageHandle: %d.", useMessageHandle);
        }

        if ( CPHTRUE == status ) {
            if (useMessageHandle) {
              useRFH2 = CPHFALSE;
            }
        }

        if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetInt(pConfig, &timeout, "to"))) {
            cphConfigInvalidParameter(pConfig, "Timeout value cannot be retrieved");
            status = CPHFALSE;
        }
        else {
            CPHTRACEMSG(pTrc, CPH_ROUTINE, "MQGET Timeout: %d.", timeout);
        }

        if ( (CPHTRUE == status) && (CPHTRUE != cphConnectionParseArgs(pConnection, pWorkerThread->pControlThread->pDestinationFactory))) {
            cphConfigInvalidParameter(pConfig, "Cannot parse options for MQ connection");
            status = CPHFALSE;

        }

        if ( (CPHTRUE == status) && (NULL == (buffer = malloc(CPH_SUBSCRIBER_INITIALMSGBBUFFERSIZE)))) {
            cphConfigInvalidParameter(pConfig, "Cannot allocate memory for message buffer");
            status = CPHFALSE;
        }

        if ( (CPHTRUE == status) &&
            (NULL == (pSubscriber = malloc(sizeof(CPH_SUBSCRIBER)))) ) {
                cphConfigInvalidParameter(pConfig, "Cannot allocate memory for subscriber control block");
                status = CPHFALSE;
        } 

        gmo.Options =   MQGMO_WAIT       /* wait for new messages         */
            | MQGMO_CONVERT;   /* convert if necessary          */

        if (CPHTRUE == pConnection->isTransacted) gmo.Options |= MQGMO_SYNCPOINT;
        if (CPHTRUE == useRFH2) gmo.Options |= MQGMO_PROPERTIES_FORCE_MQRFH2;

        gmo.WaitInterval = timeout * 1000;   /* Set the timeout value in ms   */


        if (CPHTRUE == status) {
            pSubscriber->pConnection = pConnection;      
            pSubscriber->pWorkerThread = pWorkerThread;
            pSubscriber->pWorkerThread->pRunFn = cphSubscriberRun;
            pSubscriber->pWorkerThread->pOneIterationFn = cphSubscriberOneIteration;
            pSubscriber->whetherDurable = whetherDurable;
            pSubscriber->useRFH2 = useRFH2;
            pSubscriber->useMessageHandle = useMessageHandle;
            pSubscriber->Hmsg = MQHM_NONE;
            pSubscriber->unSubscribe = unSubscribe;
            pSubscriber->timeout = timeout;
            pSubscriber->buffer = buffer;
            pSubscriber->bufflen = CPH_SUBSCRIBER_INITIALMSGBBUFFERSIZE;
            pSubscriber->isReadAhead = isReadAhead;

            pSubscriber->Hobj = MQHO_NONE;
            pSubscriber->pTrc = pTrc;

            memcpy(&(pSubscriber->gmo), &gmo, sizeof(MQGMO));

            /* Set the thread name according to our prefix */
            cphWorkerThreadSetThreadName(pWorkerThread, CPH_SUBSCRIBER_THREADNAME_PREFIX);
            *ppSubscriber = pSubscriber;
        }
    }


    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
}

/*
** Method: cphSubscriberFree
**
** Free the subscriber control block. The routine that disposes of the control thread connection block calls 
** this function for every subscriber worker thread.
**
** Input/output Parameters: Double pointer to the subscriber. When the subscriber control
** structure is freed, the single pointer to the control structure is returned as NULL.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphSubscriberFree(CPH_SUBSCRIBER **ppSubscriber) {
    CPH_WORKERTHREAD *pWorkerThread;
    CPH_TRACE *pTrc;
    int res = CPHFALSE;

    pWorkerThread = (*ppSubscriber)->pWorkerThread;
    pTrc = cphWorkerThreadGetTrace(pWorkerThread);

#define CPH_ROUTINE "cphSubscriberFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (NULL != *ppSubscriber) {
        if (NULL != (*ppSubscriber)->pConnection) cphConnectionFree(&(*ppSubscriber)->pConnection);
        if (NULL != (*ppSubscriber)->buffer) free((*ppSubscriber)->buffer);
        if (NULL != (*ppSubscriber)->pWorkerThread) cphWorkerThreadFree( &(*ppSubscriber)->pWorkerThread );
        free(*ppSubscriber);
        *ppSubscriber = NULL;
        res = CPHTRUE;
    }
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
    return(res);
}

/*
** Method: cphSubscriberRun
**
** This is the main execution method for the subscriber thread. It makes the connection to MQ, 
** subscribes to the target destination and then calls the worker thread pace method. The latter is responssible for 
** calling the cphSubscriberOneIteration method at the correct rate according to the command line
** parameters. Once the pace method exits, for durable subscriptions the function unsubscribers from the destination if
** the unsubscribe option is specified. It then closes the connection to MQ and backs 
** out any messages consumed (if running transactionally) that were not committed.
**
** Input Parameters: lpParam - a void * pointer to the subscriber control structure. This has to be passed in as 
** a void pointer as a requirement of the thread creation routines.
**
** Returns: On windows - CPHTRUE on successful execution, CPHFALSE otherwise
**          On Linux   - NULL
**
*/
CPH_RUN cphSubscriberRun( void * lpParam ) {

    CPH_SUBSCRIBER *pSubscriber;
    CPH_WORKERTHREAD * pWorker;
    CPH_LOG *pLog;

    /*   Declare MQI structures needed                                */
    MQSD     sd = {MQSD_DEFAULT};    /* Subscription Descriptor       */

    MQLONG   C_options;              /* MQCLOSE options               */

    MQLONG   CompCode;               /* completion code               */
    MQLONG   Reason;                 /* reason code                   */
    MQLONG   CReason;                /* reason code for MQCONN        */
    MQLONG   S_CompCode;             /* MQSUB completion code         */

    MQCNO ConnectOpts = {MQCNO_DEFAULT};

    MQCD     ClientConn = {MQCD_CLIENT_CONN_DEFAULT};

    MQCMHO   CrtMHOpts = {MQCMHO_DEFAULT};
    MQDMHO   DltMHOpts = {MQDMHO_DEFAULT};

    /* Client connection channel     */
    /* definition                    */

    int iterations;

    pSubscriber = lpParam;
    pWorker = pSubscriber->pWorkerThread;
    pLog = pWorker->pConfig->pLog;

#define CPH_ROUTINE "cphSubscriberRun"
    CPHTRACEENTRY(pSubscriber->pTrc, CPH_ROUTINE );

    sprintf(pSubscriber->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : START");
    cphLogPrintLn(pLog, LOGINFO, pSubscriber->messageString);

    cphConnectionSetConnectOpts(pSubscriber->pConnection, &ClientConn, &ConnectOpts);

    /* Connect to queue manager */
    MQCONNX(pSubscriber->pConnection->QMName,     /* queue manager                  */
        &ConnectOpts,
        &(pSubscriber->Hcon),     /* connection handle              */
        &CompCode,               /* completion code                */
        &CReason);               /* reason code                    */

    /* report reason and stop if it failed     */
    if (CompCode == MQCC_FAILED) {
        sprintf(pSubscriber->messageString, "MQCONNX ended with reason code %d", CReason);
        cphLogPrintLn(pLog, LOGERROR, pSubscriber->messageString );
        exit( (int)CReason );
    }


    if (pSubscriber->useMessageHandle)
    {
      MQCRTMH(pSubscriber->Hcon,
              &CrtMHOpts,
              &pSubscriber->Hmsg,
              &CompCode,
              &Reason);
      if (Reason != MQRC_NONE)
      {
        sprintf(pSubscriber->messageString, "MQCRTMH ended with reason code %d", Reason);
        cphLogPrintLn(pLog, LOGERROR, pSubscriber->messageString );
        exit( (int)Reason );
      }
      pSubscriber->gmo.Version = MQGMO_VERSION_4;
      pSubscriber->gmo.MsgHandle = pSubscriber->Hmsg;
    }

    /******************************************************************/
    /*                                                                */
    /*   Subscribe using a managed destination queue                  */
    /*                                                                */
    /******************************************************************/

    sd.Options =   MQSO_CREATE
        | MQSO_NON_DURABLE
        | MQSO_FAIL_IF_QUIESCING
        | MQSO_MANAGED;

    if (CPHTRUE == pSubscriber->whetherDurable) {

        sd.Options               = MQSO_CREATE 
            | MQSO_RESUME
            | MQSO_MANAGED
            | MQSO_DURABLE
            | MQSO_FAIL_IF_QUIESCING;

        cphSubscriberMakeDurableSubscriptionName(pSubscriber, pSubscriber->subscriberName);
        sd.SubName.VSPtr = pSubscriber->subscriberName;
        sd.SubName.VSLength = MQVS_NULL_TERMINATED;
        CPHTRACEMSG(pSubscriber->pTrc, CPH_ROUTINE, "Subscription name: %s", sd.SubName.VSPtr);
    }

    if (CPHTRUE == pSubscriber->isReadAhead) sd.Options |= MQSO_READ_AHEAD; /* Read Ahead */

    sd.ObjectString.VSPtr = pSubscriber->pConnection->destination.topic;
    sd.ObjectString.VSLength = MQVS_NULL_TERMINATED;

    pSubscriber->Hsub= MQHO_NONE;

    MQSUB(pSubscriber->Hcon,        /* connection handle            */
        &sd,                        /* object descriptor for queue  */
        &(pSubscriber->Hobj),       /* object handle (output)       */
        &(pSubscriber->Hsub),       /* object handle (output)       */
        &S_CompCode,                /* completion code              */
        &Reason);                   /* reason code                  */

    /* report reason, if any; stop if failed      */
    if (Reason != MQRC_NONE)
    {
        CPHTRACEMSG(pSubscriber->pTrc, CPH_ROUTINE, "MQSUB ended with reason code %d", Reason);
        cphWorkerThreadSetState(pWorker, cphWorkerThreadGetState(pWorker) | sERROR);
    }

    if (S_CompCode != MQCC_FAILED) {
        
        cphWorkerThreadSetState(pWorker, sRUNNING);
        
        sprintf(pSubscriber->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : Entering client loop");
        cphLogPrintLn(pLog, LOGVERBOSE, pSubscriber->messageString);

        cphWorkerThreadPace(pWorker, pSubscriber);

        /* Rollback any incomplete UOWs */
        iterations = cphWorkerThreadGetIterations(pWorker);
        if ( (CPHTRUE == pSubscriber->pConnection->isTransacted) && (iterations != 0) && (iterations%pSubscriber->pConnection->commitCount!=0) ) {
            CPHTRACEMSG(pSubscriber->pTrc, CPH_ROUTINE, "Calling MQBACK");
            MQBACK(pSubscriber->Hcon, &CompCode, &Reason);
        }

        /******************************************************************/
        /*                                                                */
        /*   Close the subscription handle                                */
        /*                                                                */
        /******************************************************************/
        if (S_CompCode != MQCC_FAILED)
        {
            C_options = MQCO_KEEP_SUB;        /* no close options             */

            if ( (CPHTRUE == pSubscriber->whetherDurable) && (CPHTRUE == pSubscriber->unSubscribe) ) {
                CPHTRACEMSG(pSubscriber->pTrc, CPH_ROUTINE, "Setting remove subscription option.\n");
                C_options = MQCO_REMOVE_SUB;
            }

            if (CPHTRUE == pSubscriber->whetherDurable) {
                CPHTRACEMSG(pSubscriber->pTrc, CPH_ROUTINE, "Closing subscription");
                MQCLOSE(pSubscriber->Hcon,   /* connection handle           */
                    &(pSubscriber->Hsub),    /* object handle               */
                    C_options,
                    &CompCode,               /* completion code             */
                    &Reason);                /* reason code                 */

                /* report reason, if any     */
                if (Reason != MQRC_NONE)
                {
                    sprintf(pSubscriber->messageString, "MQCLOSE of subscription ended with reason code %d", Reason);
                    cphLogPrintLn(pLog, LOGERROR, pSubscriber->messageString);
                }
            }
        }
    }else {
        cphLogPrintLn(pLog, LOGERROR, "Subscribe call failed" );
    }

    if (pSubscriber->Hmsg != MQHM_NONE)
    {
      MQDLTMH(pSubscriber->Hcon, &pSubscriber->Hmsg, &DltMHOpts, &CompCode, &Reason);
    }

    /******************************************************************/
    /*                                                                */
    /*   Close the managed destination queue (if it was opened)       */
    /*                                                                */
    /******************************************************************/
    if (S_CompCode != MQCC_FAILED)
    {
        C_options = MQCO_NONE;

        if (pSubscriber->Hobj != MQHO_UNUSABLE_HOBJ)
            MQCLOSE(pSubscriber->Hcon,  /* connection handle           */
            &pSubscriber->Hobj,         /* object handle               */
            C_options,
            &CompCode,                  /* completion code             */
            &Reason);                   /* reason code                 */

        /* report reason, if any     */
        if (Reason != MQRC_NONE)
        {
            sprintf(pSubscriber->messageString, "MQCLOSE ended with reason code %d", Reason);
            cphLogPrintLn(pLog, LOGERROR, pSubscriber->messageString);

        }
    }


    /* Disconnect from the queue manager */
    if (CReason != MQRC_ALREADY_CONNECTED)  {
        MQDISC(&pSubscriber->Hcon,   /* connection handle            */
            &CompCode,               /* completion code              */
            &Reason);                /* reason code                  */

        /* report reason, if any     */
        if (Reason != MQRC_NONE) {
            sprintf(pSubscriber->messageString, "MQDISC ended with reason code %d", Reason);
            cphLogPrintLn(pLog, LOGERROR, pSubscriber->messageString);

        }
    }

    sprintf(pSubscriber->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : STOP");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGINFO, pSubscriber->messageString);

    //state = (state&sERROR)|sENDED;
    cphWorkerThreadSetState(pWorker, (cphWorkerThreadGetState(pWorker) & sERROR) | sENDED);

    CPHTRACEEXIT(pSubscriber->pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    /* Return the right type of variable according to the o/s. (This return is not used) */
    return CPH_RUNRETURN;
}

int cphSubscriberMakeDurableSubscriptionName(void *aProvider, char *subName) {
    CPH_SUBSCRIBER *pSubscriber;
    CPH_WORKERTHREAD *pWorkerThread;
    CPH_TRACE *pTrc;

    pSubscriber = aProvider;
    pWorkerThread = pSubscriber->pWorkerThread;

    pTrc = cphWorkerThreadGetTrace(pWorkerThread);
#define CPH_ROUTINE "cphSubscriberMakeDurableSubscriptionName"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    strcpy(subName, pWorkerThread->pControlThread->procId);
    strcat(subName, "_");
    strcat(subName, cphWorkerThreadGetName(pWorkerThread));

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

    return(CPHTRUE);
}

/*
** Method: cphSubscriberOneIteration
**
** This method is called by cphWorkerThreadPace to receive a single message. It is a
** critical method in terms of its implementation from a performance point of view. It performs
** a single MQ get from the target destination. (The connection to MQ has already been prepared in 
** in the cphsubscriberRun method). If we are running transactionally and the number of received 
** messages has reached the commit count then the method also issues a commit call.
**
** Input Parameters: a void pointer to the subscriber control structure
**
** Returns: 
**
**      CPHTRUE - if a message was successfully received
**      CPH_RXTIMEDOUT - if no message was received in the wait interval
**      CPHFALSE       - if there was an error trying to receive a message
**
*/
int cphSubscriberOneIteration(void *aProvider) {
    CPH_SUBSCRIBER *pSubscriber;
    CPH_WORKERTHREAD *pWorkerThread;
    CPH_LOG *pLog;
    int iterations;
    int status = CPHTRUE;

    MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */

    MQLONG   CompCode;               /* completion code               */
    MQLONG   Reason;                 /* reason code                   */
    MQLONG   messlen;                /* message length received       */

    pSubscriber = aProvider;
    pWorkerThread = pSubscriber->pWorkerThread;
    pLog = pWorkerThread->pConfig->pLog;

#define CPH_ROUTINE "cphSubscriberOneIteration"
    CPHTRACEENTRY(pSubscriber->pTrc, CPH_ROUTINE );

    /****************************************************************/
    /* The following two statements are not required if the MQGMO   */
    /* version is set to MQGMO_VERSION_2 and and gmo.MatchOptions   */
    /* is set to MQGMO_NONE                                         */
    /****************************************************************/
    /*                                                              */
    /*   In order to read the messages in sequence, MsgId and       */
    /*   CorrelID must have the default value.  MQGET sets them     */
    /*   to the values in for message it returns, so re-initialise  */
    /*   them before every call                                     */
    /*                                                              */
    /****************************************************************/
    memcpy(md.MsgId, MQMI_NONE, sizeof(md.MsgId));
    memcpy(md.CorrelId, MQCI_NONE, sizeof(md.CorrelId));

    /****************************************************************/
    /*                                                              */
    /*   MQGET sets Encoding and CodedCharSetId to the values in    */
    /*   the message returned, so these fields should be reset to   */
    /*   the default values before every call, as MQGMO_CONVERT is  */
    /*   specified.                                                 */
    /*                                                              */
    /****************************************************************/

    md.Encoding       = MQENC_NATIVE;
    md.CodedCharSetId = MQCCSI_Q_MGR;
    CPHTRACEMSG(pSubscriber->pTrc, CPH_ROUTINE, "Calling MQGET : %d seconds wait time", pSubscriber->gmo.WaitInterval / 1000);

    MQGET(pSubscriber->Hcon, /* connection handle                 */
        pSubscriber->Hobj,   /* object handle                     */
        &md,                 /* message descriptor                */
        &pSubscriber->gmo,   /* get message options               */
        pSubscriber->bufflen,/* buffer length                     */
        pSubscriber->buffer, /* message buffer                    */
        &messlen,            /* message length                    */
        &CompCode,           /* completion code                   */
        &Reason);            /* reason code                       */

    /* report reason, if any     */
    if (Reason != MQRC_NONE)
    {
        if (Reason == MQRC_TRUNCATED_MSG_FAILED) {
            CPHTRACEMSG(pSubscriber->pTrc, CPH_ROUTINE, "Msg truncated. Extending buffer to %d bytes", messlen);
            pSubscriber->buffer = realloc( pSubscriber->buffer, (size_t) messlen);
            if (NULL == pSubscriber->buffer) {
                sprintf(pSubscriber->messageString, "ERROR - cannot extend buffer to %d bytes.", messlen);
                cphLogPrintLn(pLog, LOGERROR, pSubscriber->messageString);
                status = CPHFALSE;
            }
            else
                pSubscriber->bufflen = messlen;
        } else if (Reason == MQRC_NO_MSG_AVAILABLE) {
            /* special report for normal end    */
            CPHTRACEMSG(pSubscriber->pTrc, "%s", "no message received in wait interval");
            status = CPH_RXTIMEDOUT;
        } else {
            /* general report for other reasons */
            sprintf(pSubscriber->messageString, "MQGET ended with reason code %d", Reason);
            cphLogPrintLn(pLog, LOGERROR, pSubscriber->messageString);
            status = CPHFALSE;
        }
    } else {
        iterations = cphWorkerThreadGetIterations(pWorkerThread);
        if ( (CPHTRUE == pSubscriber->pConnection->isTransacted) && ((iterations+1)%pSubscriber->pConnection->commitCount==0) ) {
            CPHTRACEMSG(pSubscriber->pTrc, CPH_ROUTINE, "Calling MQCMIT");
            MQCMIT(pSubscriber->Hcon, &CompCode, &Reason);
        }

    }

    CPHTRACEEXIT(pSubscriber->pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
    return(status);
}
