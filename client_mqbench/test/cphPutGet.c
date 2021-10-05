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
*          5-11 -09    Leigh Taylor  File Creation
*          07-01-10    Oliver Fenton Add read ahead support (-jy) to MQOPEN call
*/

/* This module is the implementation of the "PutGet" test suite. This provides a set of functions that are used to put messages under
** the control of the worker thread function, which controls the rate and duration for putting messages. 
*/

#include <string.h>
#include "cphPutGet.h"

static CPH_RUN cphPutGetRun( void * lpParam );
static int cphOneInteration(CPH_PutGet *pPutGet);




/*
** Method: cphPutGetIni
**
** Create and initialise the control block for a PutGet. The worker thread control structure
** is passed in as in input parameter and is stored within the Sender - which is as close as we
** can get to creating a class derived from the worker thread.
**
** Input Parameters: pWorkerThread - the worker thread control structure
**
** Output parameters: ppPutGet - a double pointer to the newly created Sender control structure
**
*/
void cphPutGetIni(CPH_PutGet **ppPutGet, CPH_WORKERTHREAD *pWorkerThread) {
	CPH_PutGet *pPutGet = NULL;
    CPH_CONFIG *pConfig;
    int messLen;
    int isPersistent;
    int isReadAhead;
    int useRFH2;
    size_t rfh2Len = 0;
    int useMessageHandle;
    char *putbuffer;
    CPH_CONNECTION *pConnection;
    MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */
    MQPMO   pmo = {MQPMO_DEFAULT};   /* put message options           */  
	
    int timeout;
	
    MQGMO   gmo = {MQGMO_DEFAULT};   /* get message options           */
    MQBYTE *getbuffer;
	
    int status = CPHTRUE;
	
    CPH_TRACE *pTrc = cphWorkerThreadGetTrace(pWorkerThread);
    pConfig = pWorkerThread->pConfig;
	
#define CPH_ROUTINE "cphPutGetIni"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );
	
	if (NULL == pWorkerThread) {
		CPHTRACEEXIT(pTrc, CPH_ROUTINE);
		return;
	}
	
    cphConnectionIni(&pConnection, pConfig);   
	if (NULL == pConnection) {
		cphConfigInvalidParameter(pConfig, "Could not initialise cph connection control block");
		status = CPHFALSE;
	}
	
	if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetInt(pConfig, &messLen, "ms"))) {
		cphConfigInvalidParameter(pConfig, "Cannot retrieve Message length");
		status = CPHFALSE;
	}
    else {
		CPHTRACEMSG(pTrc, CPH_ROUTINE, "Message length: %d.", messLen);
    }
	
    if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetBoolean(pConfig, &isPersistent, "pp"))) {
		cphConfigInvalidParameter(pConfig, "Cannot retrieve Persistency option");
		status = CPHFALSE;
	}
    else {
		CPHTRACEMSG(pTrc, CPH_ROUTINE, "isPersistent: %d.", isPersistent);
    }
	
    if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetBoolean(pConfig, &isReadAhead, "jy"))) {
		cphConfigInvalidParameter(pConfig, "Cannot retrieve Read Ahead option");
		status = CPHFALSE;
	}
    else {
		CPHTRACEMSG(pTrc, CPH_ROUTINE, "isReadAhead: %d.", isReadAhead);
    }

    if ( (CPHTRUE == status) && (CPHTRUE != cphConfigGetBoolean(pConfig, &useRFH2, "rf"))) {
		cphConfigInvalidParameter(pConfig, "Cannot retrieve RFH2 option");
		status = CPHFALSE;
    }
    else {
        CPHTRACEMSG(pTrc, CPH_ROUTINE, "useRFH2: %d.", useRFH2);
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
		
        if (useRFH2) {
            if ( NULL == (putbuffer = cphUtilMakeBigStringWithRFH2(messLen, &rfh2Len)) ) {
                cphConfigInvalidParameter(pConfig, "Cannot create message");
                status = CPHFALSE;
            }
        }
        else if ( NULL == (putbuffer = cphUtilMakeBigString(messLen)) ) {
            cphConfigInvalidParameter(pConfig, "Cannot create message");
            status = CPHFALSE;
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
	
	if ( (CPHTRUE == status) && (NULL == (getbuffer = malloc(messLen + (MQLONG)rfh2Len)))) {
		cphConfigInvalidParameter(pConfig, "Cannot allocate memory for message buffer");
		status = CPHFALSE;
	}
	
    if ( (CPHTRUE == status) &&
		(NULL == (pPutGet = malloc(sizeof(CPH_PutGet)))) ) {
		cphConfigInvalidParameter(pConfig, "Cannot allocate memory for Sender control block");
		status = CPHFALSE;
	} 
	
	if (useRFH2) {
		memcpy(md.Format,           /* RFH2 format                        */
            MQFMT_RF_HEADER_2, (size_t)MQ_FORMAT_LENGTH);
	}
	else {
		memcpy(md.Format,           /* character string format            */
			MQFMT_STRING, (size_t)MQ_FORMAT_LENGTH);
	}
	
	if (CPHTRUE == isPersistent) md.Persistence = MQPER_PERSISTENT;
	
	gmo.Options =   MQGMO_WAIT       /* wait for new messages         */
		| MQGMO_CONVERT    /* convert if necessary          */
		| MQGMO_ACCEPT_TRUNCATED_MSG;  /* Process msg even if buffer too small */
	
	if (CPHTRUE == pConnection->isTransacted) gmo.Options |= MQGMO_SYNCPOINT;
	if (CPHTRUE == pPutGet->useRFH2) gmo.Options |= MQGMO_PROPERTIES_FORCE_MQRFH2;
	
	gmo.WaitInterval = timeout * 1000;   /* Set the timeout value in ms   */

	/* Generate a unique CorrelId for this instance, in this case the address of the structure itself */

	memset(&pPutGet->CorrelId,0,sizeof(MQBYTE24));
	sprintf( (char *) &pPutGet->CorrelId,"%lx",(long unsigned int)&pPutGet->CorrelId);
	
	/***************************************************************************/
	/* Ensure a new MsgId is created for each message but not a new CorrelId.  */
	/***************************************************************************/
	
	pmo.Options |= MQPMO_NEW_MSG_ID;
	
	if (CPHTRUE == pConnection->isTransacted) pmo.Options |= MQPMO_SYNCPOINT;
	
	if (CPHTRUE == status) {
		pPutGet->pConnection = pConnection;      
		pPutGet->pWorkerThread = pWorkerThread;
		pPutGet->pWorkerThread->pRunFn = cphPutGetRun;
		pPutGet->pWorkerThread->pOneIterationFn = (int (*) (void *) ) cphOneInteration;
		pPutGet->put.buffer = putbuffer;
		pPutGet->put.messLen = messLen + (MQLONG)rfh2Len;
		pPutGet->put.isPersistent = isPersistent;
		pPutGet->get.isReadAhead = isReadAhead;
		pPutGet->useRFH2 = useRFH2;
		pPutGet->useMessageHandle = useMessageHandle;
		pPutGet->put.Hmsg = MQHM_NONE;
		pPutGet->pTrc = pTrc;
		memcpy(&(pPutGet->put.md), &md, sizeof(MQMD));
		pmo.Version = MQPMO_VERSION_3;
		memcpy(&(pPutGet->put.pmo), &pmo, sizeof(MQPMO));
		
		
		pPutGet->get.Hmsg = MQHM_NONE;
		pPutGet->get.timeout = timeout;
		pPutGet->get.buffer = getbuffer;
		pPutGet->get.bufflen = messLen + (MQLONG)rfh2Len;
		
		memcpy(&(pPutGet->get.gmo), &gmo, sizeof(MQGMO));
		
		/* Set the thread name according to our prefix */
		cphWorkerThreadSetThreadName(pWorkerThread, CPH_PutGet_THREADNAME_PREFIX);
		*ppPutGet = pPutGet;
	}

    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
}


/*
** Method: cphPutGetFree
**
** Free the Sender control block. The routine that disposes of the control thread connection block calls 
** this function for every Sender worker thread.
**
** Input/output Parameters: Double pointer to the Sender. When the Sender control
** structure is freed, the single pointer to the control structure is returned as NULL.
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/
int cphPutGetFree(CPH_PutGet **ppPutGet) {
    int res = CPHFALSE;
    CPH_TRACE *pTrc;

    /* Take a local copy of pTrc so we can call CPHTRACEEXIT after we have freed the Sender */
    pTrc = (*ppPutGet)->pTrc;
	
    #define CPH_ROUTINE "cphPutGetFree"
    CPHTRACEENTRY(pTrc, CPH_ROUTINE );

	if (NULL != *ppPutGet) {
        if (NULL != (*ppPutGet)->pConnection)
			cphConnectionFree(&(*ppPutGet)->pConnection);
        
		if (NULL != (*ppPutGet)->put.buffer)
            free((*ppPutGet)->put.buffer); 
        
		if (NULL != (*ppPutGet)->get.buffer) 
            free((*ppPutGet)->get.buffer); 

        if (NULL != (*ppPutGet)->pWorkerThread)
			cphWorkerThreadFree( &(*ppPutGet)->pWorkerThread );
        
		free(*ppPutGet);
		*ppPutGet = NULL;
		res = CPHTRUE;
	}
    CPHTRACEEXIT(pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
	return(res);
}


/*
** Method: cphPutGetRun
**
** This is the main execution method for the Sender thread. It makes the connection to MQ, 
** opens the destination and then calls the worker thread pace method. The latter is responssible for 
** calling the cphOneInteration method at the correct rate according to the command line
** parameters. Once the pace method exits, cphPutGetRun closes the connection to MQ and backs 
** out any messages put (if running transactionally) that were not already committed.
**
** Input Parameters: lpParam - a void * pointer to the Sender control structure. This has to be passed in as 
** a void pointer as a requirement of the thread creation routines.
**
** Returns: On windows - CPHTRUE on successful execution, CPHFALSE otherwise
**          On Linux   - NULL
**
*/
static CPH_RUN cphPutGetRun( void * lpParam ) {

	CPH_PutGet *pPutGet;
	CPH_WORKERTHREAD * pWorker;
    CPH_LOG *pLog;

    /*   Declare MQI structures needed                                */
    MQOD     od = {MQOD_DEFAULT};    /* Object Descriptor             */

    MQLONG   O_options;              /* MQOPEN options                */ 
    MQLONG   C_options;              /* MQCLOSE options               */
    
    MQLONG   CompCode;               /* completion code               */
    MQLONG   OpenCode;               /* MQOPEN completion code        */
    MQLONG   Reason;                 /* reason code                   */
    MQLONG   CReason;                /* reason code for MQCONN        */

    MQCNO ConnectOpts = {MQCNO_DEFAULT};

    MQCD     ClientConn = {MQCD_CLIENT_CONN_DEFAULT};
                                    /* Client connection channel     */
                                    /* definition                    */

    MQDMHO   DltMH_options = {MQDMHO_DEFAULT};

    int iterations;

	pPutGet = lpParam;
	pWorker = pPutGet->pWorkerThread;
    pLog = pWorker->pConfig->pLog;

    #define CPH_ROUTINE "cphPutGetRun"
    CPHTRACEENTRY(pPutGet->pTrc, CPH_ROUTINE );

	sprintf(pPutGet->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : START");
    cphLogPrintLn(pLog, LOGINFO, pPutGet->messageString);

    cphConnectionSetConnectOpts(pPutGet->pConnection, &ClientConn, &ConnectOpts);

	sprintf(pPutGet->messageString, "Connect to QM %s", pPutGet->pConnection->QMName);
    cphLogPrintLn(pLog, LOGINFO, pPutGet->messageString);

    MQCONNX(pPutGet->pConnection->QMName,     /* queue manager                  */
          &ConnectOpts,
          &(pPutGet->Hcon),     /* connection handle              */
          &CompCode,               /* completion code                */
          &CReason);               /* reason code                    */


    /* report reason and stop if it failed     */
    if (CompCode == MQCC_FAILED) {
      sprintf(pPutGet->messageString, "MQCONNX ended with reason code %d", CReason);
      cphLogPrintLn(pLog, LOGERROR, pPutGet->messageString );
      exit( (int)CReason );
    }

   /******************************************************************/
   /*                                                                */
   /*   Open the target queue for output                             */
   /*                                                                */
   /******************************************************************/
   
       //strncpy(od.ObjectName, pPutGet->topic, MQ_TOPIC_NAME_LENGTH);

   strncpy(od.ObjectName, pPutGet->pConnection->destination.queue, 48);
   strncpy(od.ObjectQMgrName, pPutGet->pConnection->QMName, 48);

   od.ObjectType = MQOT_Q;
   od.Version = MQOD_VERSION_4;

   O_options =  MQOO_OUTPUT            /* open queue for output     */
              | MQOO_INPUT_SHARED      /* Receivers in parallel       */
              | MQOO_FAIL_IF_QUIESCING; /* but not if MQM stopping   */

   if (CPHTRUE == pPutGet->get.isReadAhead) O_options |= MQOO_READ_AHEAD; /* Read Ahead */
    
	
	sprintf(pPutGet->messageString, "Open queue %s", pPutGet->pConnection->destination.queue);
    cphLogPrintLn(pLog, LOGINFO, pPutGet->messageString);

   MQOPEN(pPutGet->Hcon,                      /* connection handle            */
          &od,                       /* object descriptor for queue  */
          O_options,                 /* open options                 */
          &pPutGet->Hobj,      /* object handle                */
          &OpenCode,                 /* MQOPEN completion code       */
          &Reason);                  /* reason code                  */

   /* report reason, if any; stop if failed      */
   if (Reason != MQRC_NONE)
   {
      sprintf(pPutGet->messageString, "MQOPEN ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pPutGet->messageString );
   }

   if (OpenCode == MQCC_FAILED)
   {
      cphLogPrintLn(pLog, LOGERROR, "unable to open queue for output" );
      exit( (int)Reason );
   }

    cphWorkerThreadSetState(pWorker, sRUNNING);
	
    sprintf(pPutGet->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : Entering client loop");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGVERBOSE, pPutGet->messageString);

    cphWorkerThreadPace(pWorker, pPutGet);
 
    iterations = cphWorkerThreadGetIterations(pWorker);
    if ( (CPHTRUE == pPutGet->pConnection->isTransacted) && (iterations != 0) && (iterations%pPutGet->pConnection->commitCount!=0) ) {
        CPHTRACEMSG(pPutGet->pTrc, CPH_ROUTINE, "Calling MQBACK");
        MQBACK(pPutGet->Hcon, &CompCode, &Reason);
    }

    if (pPutGet->put.Hmsg != MQHM_NONE)
    {
      MQDLTMH(pPutGet->Hcon, &pPutGet->put.Hmsg, &DltMH_options, &CompCode, &Reason);
    }

   /******************************************************************/
   /*                                                                */
   /*   Close the target queue (if it was opened)                    */
   /*                                                                */
   /******************************************************************/
   if (OpenCode != MQCC_FAILED)
   {
       C_options = MQCO_NONE;        /* no close options             */

     MQCLOSE(pPutGet->Hcon,    /* connection handle            */
             &pPutGet->Hobj,   /* object handle                */
             C_options,
             &CompCode,              /* completion code              */
             &Reason);               /* reason code                  */

     /* report reason, if any     */
     if (Reason != MQRC_NONE)
     {
      sprintf(pPutGet->messageString, "MQCLOSE ended with reason code %d", Reason);
      cphLogPrintLn(pLog, LOGERROR, pPutGet->messageString );
     }
   }


    /* Disconnect from the queue manager */
    if (CReason != MQRC_ALREADY_CONNECTED)  {
       MQDISC(&pPutGet->Hcon,                   /* connection handle            */
              &CompCode,               /* completion code              */
              &Reason);                /* reason code                  */

       /* report reason, if any     */
       if (Reason != MQRC_NONE) {
          sprintf(pPutGet->messageString, "MQDISC ended with reason code %d", Reason);
          cphLogPrintLn(pLog, LOGERROR, pPutGet->messageString );
       }
    }

	sprintf(pPutGet->messageString, "%s%s", cphWorkerThreadGetName(pWorker), " : STOP");
    cphLogPrintLn(pWorker->pConfig->pLog, LOGINFO, pPutGet->messageString);

    //state = (state&sERROR)|sENDED;
    cphWorkerThreadSetState(pWorker, (cphWorkerThreadGetState(pWorker) & sERROR) | sENDED);

    CPHTRACEEXIT(pPutGet->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE

    /* Return the right type of variable according to the o/s. (This return is not used) */
    return CPH_RUNRETURN;
}


/*
** Method: cphPutOneInteration
**
** This method is called by cphWorkerThreadPace to perform a single put operation. It is a
** critical method in terms of its implementation from a performance point of view. It performs
** a single MQ put to the target destination. (The connection to MQ has already been prepared in 
** in the cphSenderRun method). If we are running transactionally and the number of put 
** messages has reached the commit count then the method also issues a commit call.
**
** Input Parameters: a void pointer to the Sender control structure
**
** Returns: CPHTRUE on successful exection, CPHFALSE otherwise
**
*/

static int cphPutOneInteration(CPH_PutGet *pPutGet) {
    MQCMHO   CrtMHOpts = {MQCMHO_DEFAULT};
    MQBMHO   BufMHOpts = {MQBMHO_DEFAULT};
    MQLONG   Rfh2Len;
    char     *Rfh2Buf = NULL;
    MQLONG   DataLen;
    MQLONG   CompCode;               /* completion code               */
    MQLONG   Reason;                 /* reason code                   */
	char * buffer2 = NULL;
	int userfh2=CPHFALSE;

    int status = CPHTRUE;
    #define CPH_ROUTINE "cphPutOneInteration"
    CPHTRACEENTRY(pPutGet->pTrc, CPH_ROUTINE );

    CPHTRACEMSG(pPutGet->pTrc, CPH_ROUTINE, "Message to be sent: %s", pPutGet->put.buffer);

    if ((pPutGet->useMessageHandle) && (pPutGet->put.Hmsg == MQHM_NONE))
    {
      MQCRTMH(pPutGet->Hcon,
              &CrtMHOpts,
              &pPutGet->put.Hmsg,
              &CompCode,
              &Reason);
      if (Reason != MQRC_NONE)
      {
        sprintf(pPutGet->messageString, "MQCRTMH ended with reason code %d", Reason);
        cphLogPrintLn(pPutGet->pWorkerThread->pConfig->pLog, LOGERROR, pPutGet->messageString );
        status = CPHFALSE;
      }

      if (status == CPHTRUE)
      {
        Rfh2Buf = cphBuildRFH2(&Rfh2Len);
        memcpy(pPutGet->put.md.Format, MQFMT_RF_HEADER_2, (size_t)MQ_FORMAT_LENGTH);

        MQBUFMH(pPutGet->Hcon, pPutGet->put.Hmsg, &BufMHOpts, &pPutGet->put.md, Rfh2Len, Rfh2Buf, &DataLen, &CompCode, &Reason);

        /* report reason, if any */
        if (Reason != MQRC_NONE) {
             sprintf(pPutGet->messageString, "MQBUFMH ended with reason code %d\n", Reason);
             cphLogPrintLn(pPutGet->pWorkerThread->pConfig->pLog, LOGERROR, pPutGet->messageString );
             status = CPHFALSE;
        }

        free(Rfh2Buf);
      }
    }
	else
	{
       buffer2 = malloc(pPutGet->put.messLen);
	   memcpy(buffer2, pPutGet->put.buffer, pPutGet->put.messLen);
	   userfh2 = CPHTRUE;
	}

    if (status == CPHTRUE)
    {

      memcpy(pPutGet->put.md.CorrelId, pPutGet->CorrelId, sizeof(MQBYTE24));  
      pPutGet->put.pmo.OriginalMsgHandle = pPutGet->put.Hmsg;
      MQPUT(pPutGet->Hcon,       /* connection handle               */
             pPutGet->Hobj,      /* object handle                   */
             &pPutGet->put.md,     /* message descriptor              */
             &pPutGet->put.pmo,    /* default options (datagram)      */
             pPutGet->put.messLen, /* message length                  */
			 (userfh2 ? buffer2 : pPutGet->put.buffer),  /* message buffer                  */
             &CompCode,           /* completion code                 */
             &Reason);            /* reason code                     */
    /* report reason, if any */
    if (Reason != MQRC_NONE) {
         sprintf(pPutGet->messageString, "MQPUT ended with reason code %d\n", Reason);
         cphLogPrintLn(pPutGet->pWorkerThread->pConfig->pLog, LOGERROR, pPutGet->messageString );
         status = CPHFALSE;
    } else {
        if ( (CPHTRUE == pPutGet->pConnection->isTransacted) && ((cphWorkerThreadGetIterations(pPutGet->pWorkerThread)+1)%pPutGet->pConnection->commitCount==0) ) {
            CPHTRACEMSG(pPutGet->pTrc, CPH_ROUTINE, "Calling MQCMIT");
            MQCMIT(pPutGet->Hcon, &CompCode, &Reason);
        }
    }
    }
    if (userfh2)
		free(buffer2);

    CPHTRACEEXIT(pPutGet->pTrc, CPH_ROUTINE);
    #undef CPH_ROUTINE
    return(status);
}



/*
** Method: cphGetOneInteration
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
static int cphGetOneInteration(CPH_PutGet *pPutGet) {
    CPH_WORKERTHREAD *pWorkerThread;
    CPH_LOG *pLog;
    int iterations;
    int status = CPHTRUE;

    MQMD     md = {MQMD_DEFAULT};    /* Message Descriptor            */

    MQLONG   CompCode;               /* completion code               */
    MQLONG   Reason;                 /* reason code                   */
    MQLONG   messlen;                /* message length received       */

    pWorkerThread = pPutGet->pWorkerThread;
    pLog = pWorkerThread->pConfig->pLog;

#define CPH_ROUTINE "cphOneInteration"
    CPHTRACEENTRY(pPutGet->pTrc, CPH_ROUTINE );

    /****************************************************************/
    /* The following two statements are not required if the MQGMO   */
    /* version is set to MQGMO_VERSION_2 and and gmo.MatchOptions   */
    /* is set to MQGMO_NONE                                         */
    /****************************************************************/
    /*                                                              */
    /*   In order to read the messages in sequence, MsgId must have */
    /*   the default value. CorrelID must be set to the same value  */
    /*   as for the put.                                            */
    /*                                                              */
    /****************************************************************/

    memcpy(md.MsgId, MQMI_NONE, sizeof(md.MsgId));
    memcpy(md.CorrelId, pPutGet->CorrelId, sizeof(md.CorrelId));


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
    CPHTRACEMSG(pPutGet->pTrc, CPH_ROUTINE, "Calling MQGET : %d seconds wait time", pPutGet->get.gmo.WaitInterval / 1000);

    MQGET(pPutGet->Hcon, /* connection handle                 */
        pPutGet->Hobj,   /* object handle                     */
        &md,                 /* message descriptor                */
        &pPutGet->get.gmo,   /* get message options               */
        pPutGet->get.bufflen,/* buffer length                     */
        pPutGet->get.buffer, /* message buffer                    */
        &messlen,            /* message length                    */
        &CompCode,           /* completion code                   */
        &Reason);            /* reason code                       */

    /* report reason, if any     */
    if (Reason != MQRC_NONE)
    {
        if (Reason == MQRC_TRUNCATED_MSG_ACCEPTED) {
            /* This test uses a matched get buffer size so log a message incase this is important */
            sprintf(pPutGet->messageString, "Received message size[%d] exceeds buffer size[%d]. Growing buffer.", messlen, pPutGet->get.bufflen);
            cphLogPrintLn(pLog, LOGERROR, pPutGet->messageString);

            CPHTRACEMSG(pPutGet->pTrc, CPH_ROUTINE, "Msg truncated. Extending buffer to %d bytes", messlen);
            pPutGet->get.buffer = realloc( pPutGet->get.buffer, (size_t) messlen);
            if (NULL == pPutGet->get.buffer) {
                sprintf(pPutGet->messageString, "ERROR - cannot extend buffer to %d bytes.", messlen);
                cphLogPrintLn(pLog, LOGERROR, pPutGet->messageString);
                status = CPHFALSE;
            }
            else
                pPutGet->get.bufflen = messlen;
        } else if (Reason == MQRC_NO_MSG_AVAILABLE) {
            /* special report for normal end    */
            CPHTRACEMSG(pPutGet->pTrc, "%s", "no message received in wait interval");
            status = CPH_RXTIMEDOUT;
        } else {
            /* general report for other reasons */
            sprintf(pPutGet->messageString, "MQGET ended with reason code %d", Reason);
            cphLogPrintLn(pLog, LOGERROR, pPutGet->messageString);
            status = CPHFALSE;
        }
    } else {
        iterations = cphWorkerThreadGetIterations(pWorkerThread);
        if ( (CPHTRUE == pPutGet->pConnection->isTransacted) && ((iterations+1)%pPutGet->pConnection->commitCount==0) ) {
            CPHTRACEMSG(pPutGet->pTrc, CPH_ROUTINE, "Calling MQCMIT");
            MQCMIT(pPutGet->Hcon, &CompCode, &Reason);
        }

    }

    CPHTRACEEXIT(pPutGet->pTrc, CPH_ROUTINE);
#undef CPH_ROUTINE
    return(status);
}

static int cphOneInteration(CPH_PutGet *pPutGet) {
    int status = cphPutOneInteration(pPutGet);
	if (status == CPHTRUE)
	{
        status = cphGetOneInteration(pPutGet);
	}
	return status;
}



