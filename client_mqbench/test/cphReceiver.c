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
*          30-10-09    Leigh Taylor   File Creation
*          07-01-10    Oliver Fenton Add read ahead support (-jy) to MQOPEN call
*/

/* This module is the implementation of the "Receiver" test suite. This provides a set of functions that are used to subscribe to messages under
** the control of the worker thread, which controls the rate and duration at which messages are consumed. 
*/

#include <string.h>
#include "cphReceiver.h"

static CPH_RUN cphReceiverRun( void * lpParam ); 
static int cphReceiverOneIteration(void *aProvider);


/*
** Method: cphReceiverIni
**
** Create and initialise the control block for a Receiver. The worker thread control structure
** is passed in as in input parameter and is stored within the Receiver - which is as close as we
** can get to creating a class derived from the worker thread.
**
** Input Parameters: pWorkerThread - the worker thread control structure
**
** Output parameters: ppReceiver - a double pointer to the newly created Receiver control structure
**
*/
void cphReceiverIni(CPH_Receiver **ppReceiver, CPH_WORKERTHREAD *pWorkerThread) {
    CPH_Receiver *pReceiver = NULL;
    CPH_CONFIG *pConfig;

    int  useRFH2;
    int  useMessageHandle;
    CPH_CONNECTION *pConnection;
    int timeout;
    int isReadAhead;

    MQGMO   gmo = {MQGMO_DEFAULT};   /* get message options           */
    MQBYTE *buffer;

    int status = CPHTRUE;

    CPH_TRACE *pTrc = cphWorkerThreadGetTrace(pWorkerThread);
    pConfig = pWorkerThread->pConfig;

#define CPH_ROUTINE "cphReceiverIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (NULL != pWorkerThread) {

        cphConnectionIni(&pConnection, pConfig);
        if (NULL == pConnection) {
            cphConfigInvalidParameter(pConfig, "Could not initialise cph connection control block");
            status = CPHFALSE;
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

        if ( (CPHTRUE == status) && (NULL == (buffer = malloc(CPH_Receiver_INITIALMSGBBUFFERSIZE)))) {
            cphConfigInvalidParameter(pConfig, "Cannot allocate memory for message buffer");
            status = CPHFALSE;
        }

        if ( (CPHTRUE == status) &&
            (NULL == (pReceiver = malloc(sizeof(CPH_Receiver)))) ) {
                cphConfigInvalidParameter(pConfig, "Cannot allocate memory for Receiver control block");
                status = CPHFALSE;
        } 

        gmo.Options =   MQGMO_WAIT       /* wait for new messages         */
            | MQGMO_CONVERT;   /* convert if necessary          */

        if (CPHTRUE == pConnection->isTransacted) gmo.Options |= MQGMO_SYNCPOINT;
        if (CPHTRUE == pReceiver->useRFH2) gmo.Options |= MQGMO_PROPERTIES_FORCE_MQRFH2;

        gmo.WaitInterval = timeout * 1000;   /* Set the timeout value in ms   */


        if (CPHTRUE == status) {
            pReceiver->pConnection = pConnection;      
            pReceiver->pWorkerThread = pWorkerThread;
            pReceiver->pWorkerThread->pRunFn = cphReceiverRun;
            pReceiver->pWorkerThread->pOneIterationFn = cphReceiverOneIteration;
            pReceiver->useMessageHandle = useMessageHandle;
            pReceiver->Hmsg = MQHM_NONE;
            pReceiver->timeout = timeout;
            pReceiver->buffer = buffer;
            pReceiver->bufflen = CPH_Receiver_INITIALMSGBBUFFERSIZE;
	    	pReceiver->isReadAhead = isReadAhead;

            pReceiver->Hobj = MQHO_NONE;
            pReceiver->pTrc = pTrc;

            memcpy(&(pReceiver->gmo), &gmo, sizeof(MQGMO));

            /* Set the thread name according to our prefix */
            cphWorkerThreadSetThreadName(pWorkerThread, CPH_Receiver_THREADNAME_PREFIX);
            *ppReceiver = pReceiver;
        }
    }


    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
}

/*
** Method: cphReceiverFree
**
** Free the Receiver control block. The routine that disposes of the control thread connection block calls 
** this function for every Receiver worker thread.
**
** Input/output Parameters: Double pointer to the Receiver. When the Receiver control
** structure is freed, the single pointer to the control structure is returned as NULL.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphReceiverFree(CPH_Receiver **ppReceiver) {
    CPH_WORKERTHREAD *pWorkerThread;
    CPH_TRACE *pTrc;
    int res = CPHFALSE;

    pWorkerThread = (*ppReceiver)->pWorkerThread;
    pTrc = cphWorkerThreadGetTrace(pWorkerThread);

#define CPH_ROUTINE "cphReceiverFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

    if (NULL != *ppReceiver) {
        if (NULL != (*ppReceiver)->pConnection) cphConnectionFree(&(*ppReceiver)->pConnection);
        if (NULL != (*ppReceiver)->buffer) free((*ppReceiver)->buffer);
        if (NULL != (*ppReceiver)->pWorkerThread) cphWorkerThreadFree( &(*ppReceiver)->pWorkerThread );
        free(*ppReceiver);
        *ppReceiver = NULL;
        res = CPHTRUE;
    }
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
    return(res);
}

/*
** Method: cphReceiverRun
**
** This is the main execution method for the Receiver thread. It makes the connection to MQ, 
** opens the target queue and then calls the worker thread pace method. The latter is responssible for 
** calling the cphReceiverOneIteration method at the correct rate according to the command line
** parameters. It then closes the connection to MQ and backs 
** out any messages consumed (if running transactionally) that were not committed.
**
** Input Parameters: lpParam - a void * pointer to the Receiver control structure. This has to be passed in as 
** a void pointer as a requirement of the thread creation routines.
**
** Returns: On windows - CPHTRUE on successful execution, CPHFALSE otherwise
**          On Linux   - NULL
**
*/
static CPH_RUN cphReceiverRun( void * lpParam ) {

    CPH_Receiver *pReceiver;
    CPH_WORKERTHREAD * pWorker;
    CPH_LOG *pLog;

    /*   Declare MQI structures needed                                */

    MQOD     od = {MQOD_DEFAULT};    /* Object Descriptor             */

    MQLONG   O_options;              /* MQOPEN options                */ 
    MQLONG   C_options;              /* MQCLOSE options               */

    MQLONG   CompCode;               /* completion code               */
    MQLONG   Reason;                 /* reason code                   */
    MQLONG   CReason;                /* reason code for MQCONN        */
    MQLONG   OpenCode;               /* MQOPEN completion code        */

    MQCNO ConnectOpts = {MQCNO_DEFAULT};

    MQCD     ClientConn = {MQCD_CLIENT_CONN_DEFAULT};

    MQCMHO   CrtMHOpts = {MQCMHO_DEFAULT};
    MQDMHO   DltMHOpts = {MQDMHO_DEFAULT};

    /* Client connection channel     */
    /* definition                    */

    int iterations;

    pReceiver = lpParam;
    pWorker = pReceiver->pWorkerThread;
    pLog = pWorker->pConfig->pLog;

#define CPH_ROUTINE "cphReceiverRun"
    CPHTRACEENTRY(pReceiver->pTrc, CPH_ROUTINE );

    sprintf(pReceiver->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : START");
    cphLogPrintLn(pLog, LOGINFO, pReceiver->messageString);

    cphConnectionSetConnectOpts(pReceiver->pConnection, &ClientConn, &ConnectOpts);

    /* Connect to queue manager */
    MQCONNX(pReceiver->pConnection->QMName,     /* queue manager                  */
        &ConnectOpts,
        &(pReceiver->Hcon),     /* connection handle              */
        &CompCode,               /* completion code                */
        &CReason);               /* reason code                    */

    /* report reason and stop if it failed     */
    if (CompCode == MQCC_FAILED) {
        sprintf(pReceiver->messageString, "MQCONNX ended with reason code %d", CReason);
        cphLogPrintLn(pLog, LOGERROR, pReceiver->messageString );
        exit( (int)CReason );
    }


    if (pReceiver->useMessageHandle)
    {
      MQCRTMH(pReceiver->Hcon,
              &CrtMHOpts,
              &pReceiver->Hmsg,
              &CompCode,
              &Reason);
      if (Reason != MQRC_NONE)
      {
        sprintf(pReceiver->messageString, "MQCRTMH ended with reason code %d", Reason);
        cphLogPrintLn(pLog, LOGERROR, pReceiver->messageString );
        exit( (int)Reason );
      }
      pReceiver->gmo.Version = MQGMO_VERSION_4;
      pReceiver->gmo.MsgHandle = pReceiver->Hmsg;
    }

	/******************************************************************/
   /*                                                                */
   /*   Open the target queue for input                              */
   /*                                                                */
   /******************************************************************/
   
  
	sprintf(pReceiver->messageString, "Open queue %s", pReceiver->pConnection->destination.queue);
    cphLogPrintLn(pLog, LOGINFO, pReceiver->messageString);

   strncpy(od.ObjectName, pReceiver->pConnection->destination.queue, 48);
   strncpy(od.ObjectQMgrName, pReceiver->pConnection->QMName, 48);

   od.ObjectType = MQOT_Q;
   od.Version = MQOD_VERSION_4;

   O_options =   MQOO_INPUT_SHARED      /* Receivers in parallel       */
               | MQOO_FAIL_IF_QUIESCING /* but not if MQM stopping   */
               ;                        /* = 0x2010 = 8208 decimal   */
   if (CPHTRUE == pReceiver->isReadAhead) O_options |= MQOO_READ_AHEAD; /* Read Ahead */

   MQOPEN(pReceiver->Hcon,                      /* connection handle            */
          &od,                       /* object descriptor for queue  */
          O_options,                 /* open options                 */
          &pReceiver->Hobj,                     /* object handle                */
          &OpenCode,                 /* MQOPEN completion code       */
          &Reason);                  /* reason code                  */

   /* report reason, if any; stop if failed      */
   if (Reason != MQRC_NONE)
   {
      sprintf(pReceiver->messageString, "MQOPEN ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pReceiver->messageString );
   }

    
   if (OpenCode != MQCC_FAILED) {
        
       cphWorkerThreadSetState(pWorker, sRUNNING);
        
       sprintf(pReceiver->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : Entering client loop");
       cphLogPrintLn(pLog, LOGVERBOSE, pReceiver->messageString);

       cphWorkerThreadPace(pWorker, pReceiver);

       /* Rollback any incomplete UOWs */
       iterations = cphWorkerThreadGetIterations(pWorker);
       if ( (CPHTRUE == pReceiver->pConnection->isTransacted) && (iterations != 0) && (iterations%pReceiver->pConnection->commitCount!=0) ) {
           CPHTRACEMSG(pReceiver->pTrc, CPH_ROUTINE, "Calling MQBACK");
           MQBACK(pReceiver->Hcon, &CompCode, &Reason);
       }

   }else {
       cphLogPrintLn(pLog, LOGERROR, "Open call failed" );
   }

   if (pReceiver->Hmsg != MQHM_NONE)
   {
     MQDLTMH(pReceiver->Hcon, &pReceiver->Hmsg, &DltMHOpts, &CompCode, &Reason);
   }

    /******************************************************************/
    /*                                                                */
    /*   Close the destination queue (if it was opened)               */
    /*                                                                */
    /******************************************************************/
   if (OpenCode != MQCC_FAILED)
   {
       C_options = MQCO_NONE;

       if (pReceiver->Hobj != MQHO_UNUSABLE_HOBJ)
           MQCLOSE(pReceiver->Hcon,  /* connection handle           */
           &pReceiver->Hobj,         /* object handle               */
           C_options,
           &CompCode,                  /* completion code             */
           &Reason);                   /* reason code                 */

       /* report reason, if any     */
       if (Reason != MQRC_NONE)
       {
           sprintf(pReceiver->messageString, "MQCLOSE ended with reason code %d", Reason);
           cphLogPrintLn(pLog, LOGERROR, pReceiver->messageString);
       }
   }


   /* Disconnect from the queue manager */
   if (CReason != MQRC_ALREADY_CONNECTED)  {
       MQDISC(&pReceiver->Hcon,   /* connection handle            */
           &CompCode,               /* completion code              */
           &Reason);                /* reason code                  */

       /* report reason, if any     */
       if (Reason != MQRC_NONE) {
           sprintf(pReceiver->messageString, "MQDISC ended with reason code %d", Reason);
           cphLogPrintLn(pLog, LOGERROR, pReceiver->messageString);
       }
   }

   sprintf(pReceiver->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : STOP");
   cphLogPrintLn(pWorker->pConfig->pLog, LOGINFO, pReceiver->messageString);

   //state = (state&sERROR)|sENDED;
   cphWorkerThreadSetState(pWorker, (cphWorkerThreadGetState(pWorker) & sERROR) | sENDED);

   CPHTRACEEXIT(pReceiver->pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE

   /* Return the right type of variable according to the o/s. (This return is not used) */
   return CPH_RUNRETURN;
}


/*
** Method: cphReceiverOneIteration
**
** This method is called by cphWorkerThreadPace to receive a single message. It is a
** critical method in terms of its implementation from a performance point of view. It performs
** a single MQ get from the target destination. (The connection to MQ has already been prepared in 
** in the cphReceiverRun method). If we are running transactionally and the number of received 
** messages has reached the commit count then the method also issues a commit call.
**
** Input Parameters: a void pointer to the Receiver control structure
**
** Returns: 
**
**      CPHTRUE - if a message was successfully received
**      CPH_RXTIMEDOUT - if no message was received in the wait interval
**      CPHFALSE       - if there was an error trying to receive a message
**
*/
static int cphReceiverOneIteration(void *aProvider) {
    CPH_Receiver *pReceiver;
    CPH_WORKERTHREAD *pWorkerThread;
    CPH_LOG *pLog;
    int iterations;
    int status = CPHTRUE;

    MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */

    MQLONG   CompCode;               /* completion code               */
    MQLONG   Reason;                 /* reason code                   */
    MQLONG   messlen;                /* message length received       */

    pReceiver = aProvider;
    pWorkerThread = pReceiver->pWorkerThread;
    pLog = pWorkerThread->pConfig->pLog;

#define CPH_ROUTINE "cphReceiverOneIteration"
    CPHTRACEENTRY(pReceiver->pTrc, CPH_ROUTINE );

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
    CPHTRACEMSG(pReceiver->pTrc, CPH_ROUTINE, "Calling MQGET : %d seconds wait time", pReceiver->gmo.WaitInterval / 1000);

    MQGET(pReceiver->Hcon, /* connection handle                 */
        pReceiver->Hobj,   /* object handle                     */
        &md,                 /* message descriptor                */
        &pReceiver->gmo,   /* get message options               */
        pReceiver->bufflen,/* buffer length                     */
        pReceiver->buffer, /* message buffer                    */
        &messlen,            /* message length                    */
        &CompCode,           /* completion code                   */
        &Reason);            /* reason code                       */

    /* report reason, if any     */
    if (Reason != MQRC_NONE)
    {
        if (Reason == MQRC_TRUNCATED_MSG_FAILED) {
            CPHTRACEMSG(pReceiver->pTrc, CPH_ROUTINE, "Msg truncated. Extending buffer to %d bytes", messlen);
            pReceiver->buffer = realloc( pReceiver->buffer, (size_t) messlen);
            if (NULL == pReceiver->buffer) {
                sprintf(pReceiver->messageString, "ERROR - cannot extend buffer to %d bytes.", messlen);
                cphLogPrintLn(pLog, LOGERROR, pReceiver->messageString);
                status = CPHFALSE;
            }
            else
                pReceiver->bufflen = messlen;
        } else if (Reason == MQRC_NO_MSG_AVAILABLE) {
            /* special report for normal end    */
            CPHTRACEMSG(pReceiver->pTrc, "%s", "no message received in wait interval");
            status = CPH_RXTIMEDOUT;
        } else {
            /* general report for other reasons */
            sprintf(pReceiver->messageString, "MQGET ended with reason code %d", Reason);
            cphLogPrintLn(pLog, LOGERROR, pReceiver->messageString);
            status = CPHFALSE;
        }
    } else {
        iterations = cphWorkerThreadGetIterations(pWorkerThread);
        if ( (CPHTRUE == pReceiver->pConnection->isTransacted) && ((iterations+1)%pReceiver->pConnection->commitCount==0) ) {
            CPHTRACEMSG(pReceiver->pTrc, CPH_ROUTINE, "Calling MQCMIT");
            MQCMIT(pReceiver->Hcon, &CompCode, &Reason);
        }

    }

    CPHTRACEEXIT(pReceiver->pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
    return(status);
}
